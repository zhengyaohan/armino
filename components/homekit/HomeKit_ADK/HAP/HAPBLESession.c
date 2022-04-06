// Disclaimer: IMPORTANT: This Apple software is supplied to you, by Apple Inc. ("Apple"), in your
// capacity as a current, and in good standing, Licensee in the MFi Licensing Program. Use of this
// Apple software is governed by and subject to the terms and conditions of your MFi License,
// including, but not limited to, the restrictions specified in the provision entitled "Public
// Software", and is further subject to your agreement to the following additional terms, and your
// agreement that the use, installation, modification or redistribution of this Apple software
// constitutes acceptance of these additional terms. If you do not agree with these additional terms,
// you may not use, install, modify or redistribute this Apple software.
//
// Subject to all of these terms and in consideration of your agreement to abide by them, Apple grants
// you, for as long as you are a current and in good-standing MFi Licensee, a personal, non-exclusive
// license, under Apple's copyrights in this Apple software (the "Apple Software"), to use,
// reproduce, and modify the Apple Software in source form, and to use, reproduce, modify, and
// redistribute the Apple Software, with or without modifications, in binary form, in each of the
// foregoing cases to the extent necessary to develop and/or manufacture "Proposed Products" and
// "Licensed Products" in accordance with the terms of your MFi License. While you may not
// redistribute the Apple Software in source form, should you redistribute the Apple Software in binary
// form, you must retain this notice and the following text and disclaimers in all such redistributions
// of the Apple Software. Neither the name, trademarks, service marks, or logos of Apple Inc. may be
// used to endorse or promote products derived from the Apple Software without specific prior written
// permission from Apple. Except as expressly stated in this notice, no other rights or licenses,
// express or implied, are granted by Apple herein, including but not limited to any patent rights that
// may be infringed by your derivative works or by other works in which the Apple Software may be
// incorporated. Apple may terminate this license to the Apple Software by removing it from the list
// of Licensed Technology in the MFi License, or otherwise in accordance with the terms of such MFi License.
//
// Unless you explicitly state otherwise, if you provide any ideas, suggestions, recommendations, bug
// fixes or enhancements to Apple in connection with this software ("Feedback"), you hereby grant to
// Apple a non-exclusive, fully paid-up, perpetual, irrevocable, worldwide license to make, use,
// reproduce, incorporate, modify, display, perform, sell, make or have made derivative works of,
// distribute (directly or indirectly) and sublicense, such Feedback in connection with Apple products
// and services. Providing this Feedback is voluntary, but if you do provide Feedback to Apple, you
// acknowledge and agree that Apple may exercise the license granted above without the payment of
// royalties or further consideration to Participant.

// The Apple Software is provided by Apple on an "AS IS" basis. APPLE MAKES NO WARRANTIES, EXPRESS OR
// IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY
// AND FITNESS FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR
// IN COMBINATION WITH YOUR PRODUCTS.
//
// IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION
// AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
// (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (C) 2015-2021 Apple Inc. All Rights Reserved.

#include "HAPBLESession.h"
#include "HAPAccessoryServer+Internal.h"
#include "HAPLogSubsystem.h"
#include "HAPSession.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "BLESession" };

// Set this flag to disable all BLE session timeouts.
#define DEBUG_DISABLE_TIMEOUTS (false)
/**
 * Timeout after which it is assumed that pending responses has been sent by the BLE stack.
 *
 * - BLE stacks typically send responses asynchronously and do not inform the application when it is fully sent.
 *   When we want to disconnect we decide to give pending responses time to be fully sent by the BLE stack.
 *   This timeout specifies how long we wait until pending responses have been sent.
 */
#define kHAPBLESession_SafeToDisconnectTimeout ((HAPTime)(200 * HAPMillisecond))

#if !DEBUG_DISABLE_TIMEOUTS
static void LinkTimerOrPairingProcedureTimerExpired(HAPPlatformTimerRef timer, void* _Nullable context) {
    HAPPrecondition(context);
    HAPBLESession* bleSession = context;
    if (timer == bleSession->linkTimer) {
        HAPLogInfo(&logObject, "Link timeout expired.");
        bleSession->linkTimer = 0;
        bleSession->linkTimerDeadline = 0;
    } else if (timer == bleSession->pairingProcedureTimer) {
        HAPLogInfo(&logObject, "Pairing procedure timeout expired.");
        bleSession->pairingProcedureTimer = 0;
    } else {
        HAPPreconditionFailure();
    }

    // When link deadline or pairing procedure expires, invalidate security session and terminate BLE link.
    HAPSessionInvalidate(bleSession->server, bleSession->session, /* terminateLink: */ true);
}
#endif

static void SafeToDisconnectTimerExpired(HAPPlatformTimerRef timer, void* _Nullable context) {
    HAPPrecondition(context);
    HAPBLESession* bleSession = context;
    // This handles a race condition where the bleSession timer is canceled
    // while the timer handler was queued for execution.
    if (timer != bleSession->safeToDisconnectTimer) {
        HAPLogDebug(&logObject, "Safe to disconnect expired after timer was invalidated.");
        return;
    }

    HAPLogDebug(&logObject, "Safe to disconnect expired.");
    bleSession->safeToDisconnectTimer = 0;
    HAPPrecondition(bleSession->server);
    HAPAccessoryServer* server = bleSession->server;
    HAPPrecondition(server->platform.ble.blePeripheralManager);
    HAPPlatformBLEPeripheralManagerRef blePeripheralManager = server->platform.ble.blePeripheralManager;

    bleSession->isSafeToDisconnect = true;

    if (HAPBLESessionIsTerminal(bleSession)) {
        HAPLogInfo(
                &logObject,
                "Disconnecting BLE connection - Security session marked terminal (safe to disconnect timer).");
        HAPPlatformBLEPeripheralManagerCancelCentralConnection(
                blePeripheralManager, server->ble.connection.connectionHandle);
    } else if (server->state != kHAPAccessoryServerState_Running) {
        HAPLogInfo(&logObject, "Disconnecting BLE connection - Server is stopping (safe to disconnect timer).");
        HAPPlatformBLEPeripheralManagerCancelCentralConnection(
                blePeripheralManager, server->ble.connection.connectionHandle);
    } else if (!server->ble.isTransportRunning || server->ble.isTransportStopping) {
        HAPLogInfo(&logObject, "Disconnecting BLE connection - Transport is stopping (safe to disconnect timer).");
        HAPPlatformBLEPeripheralManagerCancelCentralConnection(
                blePeripheralManager, server->ble.connection.connectionHandle);
    } else {
        // Session is safe to disconnect (pending data was likely sent),
        // but it is not necessary to disconnect.
    }
}

void HAPBLESessionCreate(HAPAccessoryServer* server, HAPSession* session) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(session->transportType == kHAPTransportType_BLE);
    HAPBLESession* bleSession = &session->_.ble;

#if !DEBUG_DISABLE_TIMEOUTS
    HAPError err;
#endif

    HAPRawBufferZero(bleSession, sizeof *bleSession);

    bleSession->server = server;
    bleSession->session = session;

// 40. After a Bluetooth link is established the first HAP procedure must begin within 10 seconds. Accessories must
// drop the Bluetooth Link if the controller fails to start a HAP procedure within 10 seconds of establishing the
// Bluetooth link.
// See HomeKit Accessory Protocol Specification R17
// Section 7.5 Testing Bluetooth LE Accessories
#if !DEBUG_DISABLE_TIMEOUTS
    bleSession->linkTimerDeadline = HAPPlatformClockGetCurrent() + 10 * HAPSecond;
    err = HAPPlatformTimerRegister(
            &bleSession->linkTimer, bleSession->linkTimerDeadline, LinkTimerOrPairingProcedureTimerExpired, bleSession);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPLogError(&logObject, "Not enough timers available to register BLE link timer.");
        HAPFatalError();
    }
#endif
    bleSession->pairingProcedureTimer = 0;

    bleSession->isSafeToDisconnect = true;
    bleSession->safeToDisconnectTimer = 0;
}

void HAPBLESessionRelease(HAPBLESession* bleSession) {
    HAPPrecondition(bleSession);

    if (bleSession->linkTimer) {
        HAPPlatformTimerDeregister(bleSession->linkTimer);
        bleSession->linkTimer = 0;
        bleSession->linkTimerDeadline = 0;
    }
    if (bleSession->pairingProcedureTimer) {
        HAPPlatformTimerDeregister(bleSession->pairingProcedureTimer);
        bleSession->pairingProcedureTimer = 0;
    }
    if (bleSession->safeToDisconnectTimer) {
        HAPPlatformTimerDeregister(bleSession->safeToDisconnectTimer);
        bleSession->safeToDisconnectTimer = 0;
    }
}

void HAPBLESessionInvalidate(HAPAccessoryServer* server, HAPBLESession* bleSession, bool terminateLink) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.ble.blePeripheralManager);
    HAPPlatformBLEPeripheralManager* blePeripheralManager = server->platform.ble.blePeripheralManager;
    HAPPrecondition(bleSession);

    if (bleSession->linkTimer) {
        HAPPlatformTimerDeregister(bleSession->linkTimer);
        bleSession->linkTimer = 0;
        bleSession->linkTimerDeadline = 0;
    }

    if (terminateLink) {
        bleSession->isTerminal = true;
        if (HAPBLESessionIsSafeToDisconnect(bleSession) && server->ble.connection.connected) {
            HAPLogInfo(&logObject, "Disconnecting connection - Security session marked terminal.");
            HAPPlatformBLEPeripheralManagerCancelCentralConnection(
                    blePeripheralManager, server->ble.connection.connectionHandle);
        }
    }
    if (bleSession->pairingProcedureTimer) {
        HAPPlatformTimerDeregister(bleSession->pairingProcedureTimer);
        bleSession->pairingProcedureTimer = 0;
    }
}

HAP_RESULT_USE_CHECK
bool HAPBLESessionIsTerminalSoon(const HAPBLESession* bleSession) {
    HAPPrecondition(bleSession);

    if (bleSession->isTerminal) {
        return true;
    }

    if (bleSession->linkTimer) {
        HAPTime now = HAPPlatformClockGetCurrent();
        return bleSession->linkTimerDeadline < now || now - bleSession->linkTimerDeadline <= 200 * HAPMillisecond;
    }

    return false;
}

HAP_RESULT_USE_CHECK
bool HAPBLESessionIsTerminal(const HAPBLESession* bleSession) {
    HAPPrecondition(bleSession);

    return bleSession->isTerminal;
}

HAP_RESULT_USE_CHECK
bool HAPBLESessionIsSafeToDisconnect(const HAPBLESession* bleSession) {
    HAPPrecondition(bleSession);

    return bleSession->isSafeToDisconnect;
}

void HAPBLESessionDidSendGATTResponse(HAPAccessoryServer* server, HAPSession* session) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(session->transportType == kHAPTransportType_BLE);

    HAPError err;

    HAPBLESession* bleSession = &session->_.ble;

    bleSession->isSafeToDisconnect = false;

    // Set safe to disconnect timer to ensure response is being sent before disconnecting.
    if (bleSession->safeToDisconnectTimer) {
        HAPPlatformTimerDeregister(bleSession->safeToDisconnectTimer);
        bleSession->safeToDisconnectTimer = 0;
    }
    err = HAPPlatformTimerRegister(
            &bleSession->safeToDisconnectTimer,
            HAPPlatformClockGetCurrent() + kHAPBLESession_SafeToDisconnectTimeout,
            SafeToDisconnectTimerExpired,
            bleSession);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPLog(&logObject, "Not enough resources to consider safe to dc timer. Reporting safe to dc immediately!");
        bleSession->isSafeToDisconnect = true;
    }
}

void HAPBLESessionDidStartBLEProcedure(HAPAccessoryServer* server, HAPSession* session) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(session->transportType == kHAPTransportType_BLE);

#if !DEBUG_DISABLE_TIMEOUTS
    HAPError err;
#endif
    HAPLogDebug(&logObject, "%s", __func__);

    HAPBLESession* bleSession = &session->_.ble;

    if (bleSession->isTerminal) {
        return;
    }

    // 40. After a Bluetooth link is established the first HAP procedure must begin within 10 seconds. Accessories must
    // drop the Bluetooth Link if the controller fails to start a HAP procedure within 10 seconds of establishing the
    // Bluetooth link.
    // See HomeKit Accessory Protocol Specification R17
    // Section 7.5 Testing Bluetooth LE Accessories
    if (!HAPSessionIsSecured(session)) {
        if (bleSession->linkTimer) {
            HAPPlatformTimerDeregister(bleSession->linkTimer);
            bleSession->linkTimer = 0;
            bleSession->linkTimerDeadline = 0;
        }
    }

    // 5. Must implement a security session timeout and terminate the security session after 30 seconds of inactivity
    // (i.e., without any HAP Transactions).
    // See HomeKit Accessory Protocol Specification R17
    // Section 7.2 Accessory Requirements
    if (HAPSessionIsSecured(session)) {
#if !DEBUG_DISABLE_TIMEOUTS
        if (bleSession->linkTimer) {
            HAPPlatformTimerDeregister(bleSession->linkTimer);
            bleSession->linkTimer = 0;
            bleSession->linkTimerDeadline = 0;
        }
        bleSession->linkTimerDeadline = HAPPlatformClockGetCurrent() + 30 * HAPSecond;
        err = HAPPlatformTimerRegister(
                &bleSession->linkTimer,
                bleSession->linkTimerDeadline,
                LinkTimerOrPairingProcedureTimerExpired,
                bleSession);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            HAPLog(&logObject, "Not enough resources to start link timer. Invalidating session!");
            HAPSessionInvalidate(server, session, /* terminateLink: */ true);
        }
#endif
    }
}

void HAPBLESessionDidStartPairingProcedure(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPPairingProcedureType pairingProcedureType HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(session->transportType == kHAPTransportType_BLE);

    HAPBLESession* bleSession = &session->_.ble;

#if !DEBUG_DISABLE_TIMEOUTS
    HAPError err;
#endif
    HAPLogDebug(&logObject, "%s", __func__);

    if (bleSession->isTerminal) {
        return;
    }

    // 39. Accessories must implement a 10 second HAP procedure timeout, all HAP procedures [...] must complete within
    // 10 seconds, if a procedure fails to complete within the procedure timeout the accessory must drop the security
    // session and also drop the Bluetooth link.
    // See HomeKit Accessory Protocol Specification R17
    // Section 7.5 Testing Bluetooth LE Accessories
    if (!bleSession->pairingProcedureTimer) {
#if !DEBUG_DISABLE_TIMEOUTS
        err = HAPPlatformTimerRegister(
                &bleSession->pairingProcedureTimer,
                HAPPlatformClockGetCurrent() + 10 * HAPSecond,
                LinkTimerOrPairingProcedureTimerExpired,
                bleSession);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            HAPLog(&logObject, "Not enough resources to start pairing procedure timer. Invalidating session!");
            HAPSessionInvalidate(server, session, /* terminateLink: */ true);
        }
#endif
    }
}

void HAPBLESessionDidCompletePairingProcedure(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPPairingProcedureType pairingProcedureType) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(session->transportType == kHAPTransportType_BLE);

    HAPBLESession* bleSession = &session->_.ble;

#if !DEBUG_DISABLE_TIMEOUTS
    HAPError err;
#endif
    HAPLogDebug(&logObject, "%s", __func__);

    if (bleSession->isTerminal) {
        return;
    }

    // Reset pairing procedure timeout.
    if (bleSession->pairingProcedureTimer) {
        HAPPlatformTimerDeregister(bleSession->pairingProcedureTimer);
        bleSession->pairingProcedureTimer = 0;
    }

    if (pairingProcedureType == kHAPPairingProcedureType_PairVerify && HAPSessionIsSecured(session)) {
        // Start session security timeout.
#if !DEBUG_DISABLE_TIMEOUTS
        if (bleSession->linkTimer) {
            HAPPlatformTimerDeregister(bleSession->linkTimer);
            bleSession->linkTimer = 0;
            bleSession->linkTimerDeadline = 0;
        }
        bleSession->linkTimerDeadline = HAPPlatformClockGetCurrent() + 30 * HAPSecond;
        err = HAPPlatformTimerRegister(
                &bleSession->linkTimer,
                bleSession->linkTimerDeadline,
                LinkTimerOrPairingProcedureTimerExpired,
                bleSession);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            HAPLog(&logObject, "Not enough resources to start link timer. Invalidating session!");
            HAPSessionInvalidate(server, session, /* terminateLink: */ true);
        }
#endif
    }
}

void HAPBLESessionDidPairSetupProcedure(HAPAccessoryServer* server, HAPSession* session) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(session->transportType == kHAPTransportType_BLE);

    HAPBLESession* bleSession = &session->_.ble;

#if !DEBUG_DISABLE_TIMEOUTS

    HAPError err;
#endif
    HAPLogDebug(&logObject, "%s", __func__);

    if (bleSession->isTerminal) {
        return;
    }

#if !DEBUG_DISABLE_TIMEOUTS
    // Reset link timer
    if (bleSession->linkTimer) {
        HAPPlatformTimerDeregister(bleSession->linkTimer);
        bleSession->linkTimer = 0;
        bleSession->linkTimerDeadline = 0;
    }
    bleSession->linkTimerDeadline = HAPPlatformClockGetCurrent() + 30 * HAPSecond;
    err = HAPPlatformTimerRegister(
            &bleSession->linkTimer, bleSession->linkTimerDeadline, LinkTimerOrPairingProcedureTimerExpired, bleSession);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPLog(&logObject, "Not enough resources to start link timer. Invalidating session!");
        HAPSessionInvalidate(server, session, /* terminateLink: */ true);
    }
#endif
}

#endif
