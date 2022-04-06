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

#include "HAPPlatformFeatures.h"

#include "HAPAccessoryServer+Internal.h"
#include "HAPBLEAccessoryServer.h"
#include "HAPIPAccessoryServer.h"
#include "HAPLogSubsystem.h"
#include "HAPPairingPairSetup.h"
#include "HAPPairingPairVerify.h"
#include "HAPPairingPairings.h"
#include "HAPSession.h"
#if HAP_FEATURE_ENABLED(HAP_FEATURE_ACCESS_CODE)
#include "HAPRequestHandlers+AccessCode.h"
#endif
#if HAP_FEATURE_ENABLED(HAP_FEATURE_NFC_ACCESS)
#include "HAPRequestHandlers+NfcAccess.h"
#endif
#include "HAPThreadSessionStorage.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "Session" };

static void HAPSessionInvalidateTransportSpecificState(HAPAccessoryServer* server, HAPSession* session) {
    HAPPrecondition(server);
    HAPPrecondition(session);

    // Invalidate transport specific dependent state.
    switch (session->transportType) {
        case kHAPTransportType_IP: {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
            HAPAssert(server->transports.ip);
            HAPNonnull(server->transports.ip)->session.invalidateDependentIPState(server, session);
            break;
#endif
        }
        case kHAPTransportType_BLE: {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
            HAPAssert(server->transports.ble);
            // To support Pair Resume, DataStreams are not invalidated unless a pairing has been evicted from the BLE
            // cache.
            break;
#endif
        }
        case kHAPTransportType_Thread: {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
            HAPAssert(server->transports.thread);
            break;
#endif
        }
        default:
            HAPFatalError();
    }
}

/**
 * Update session state and notify Accessory Server, based on Accessory Server pairing status before and after
 * a function call.
 *
 * If an Accessory Server has no pairings after a function call (but was paired before), the transport
 * specific state needs to be invalidated. If an Accessory Server's pairing status has changed, the Accessory
 * Server needs to be informed.
 *
 *
 * @param server              Accessory server.
 * @param session             Session.
 * @param wasPaired           true if Accessory Server was paired before function call, false otherwise.
 * @param isPaired            true if Accessory server is paired after function call, false otherwise.
 */
static void HAPSessionAccessoryServerPairingUpdate(
        HAPAccessoryServer* server,
        HAPSession* session,
        bool wasPaired,
        bool isPaired) {
    HAPPrecondition(server);
    HAPPrecondition(session);

    if (wasPaired && !isPaired) {
        HAPSessionInvalidateTransportSpecificState(server, session);
    }

    if (wasPaired != isPaired) {
        HAPAccessoryServerDelegateScheduleHandleUpdatedState(server);
    }
}

/**
 * Check if session key has expired
 *
 * @param      session              Session
 * @return true if active session key has expired. false, otherwise
 */
static bool SessionKeyExpired(HAPSession* session) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPAccessoryServer* server = session->server;

    return (server->sessionKeyExpiry > 0 && session->hap.active &&
            HAPPlatformClockGetCurrent() >= session->hap.timestamp + server->sessionKeyExpiry);
}

HAP_RESULT_USE_CHECK
bool HAPSessionKeyExpired(const HAPSession* session) {
    return SessionKeyExpired((HAPSession*) session);
}

HAP_RESULT_USE_CHECK
bool HAPTransportTypeIsValid(HAPTransportType value) {
    switch (value) {
        case kHAPTransportType_IP:
        case kHAPTransportType_BLE:
        case kHAPTransportType_Thread: {
            return true;
        }
    }
    return false;
}

void HAPSessionCreate(HAPAccessoryServer* server, HAPSession* session, HAPTransportType transportType) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(HAPTransportTypeIsValid(transportType));

    HAPLogDebug(&logObject, "%s", __func__);

    HAPRawBufferZero(session, sizeof *session);
    session->server = server;
    session->transportType = transportType;

    // Initialize session state.
    HAPPairingPairVerifyReset(session);
    HAPPairingPairingsReset(session);

    // Initialize transport specific part.
    switch (transportType) {
        case kHAPTransportType_IP: {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
            HAPAssert(server->transports.ip);
            return;
#endif
        }
        case kHAPTransportType_BLE: {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
            HAPAssert(server->transports.ble);
            HAPNonnull(server->transports.ble)->session.create(server, session);
            return;
#endif
        }
        case kHAPTransportType_Thread: {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
            HAPAssert(server->transports.thread);
            HAPAssert(HAPThreadSessionStorageIsEmpty(server, session));
            return;
#endif
        }
        default:
            HAPFatalError();
    }
}

void HAPSessionRelease(HAPAccessoryServer* server, HAPSession* session) {
    HAPPrecondition(server);
    HAPPrecondition(session);

    HAPLogDebug(&logObject, "%s", __func__);

    // Invalidate session.
    HAPSessionInvalidate(server, session, /* terminateLink: */ true);

    // Deinitialize transport specific part and reset session.
    switch (session->transportType) {
        case kHAPTransportType_IP: {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
            HAPAssert(server->transports.ip);
            HAPRawBufferZero(session, sizeof *session);
            return;
#endif
        }
        case kHAPTransportType_BLE: {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
            HAPAssert(server->transports.ble);
            HAPNonnull(server->transports.ble)->session.release(&session->_.ble);
            HAPRawBufferZero(session, sizeof *session);
            return;
#endif
        }
        case kHAPTransportType_Thread: {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
            HAPAssert(server->transports.thread);
            HAPAssert(HAPThreadSessionStorageIsEmpty(server, session));
            for (size_t i = 0; i < server->thread.storage->numSessions; i++) {
                HAPThreadSession* threadSession = &server->thread.storage->sessions[i];
                if (session == &threadSession->hapSession) {
                    HAPRawBufferZero(threadSession, sizeof *threadSession);
                    break;
                }
            }
            HAPRawBufferZero(session, sizeof *session);
            return;
#endif
        }
        default:
            HAPFatalError();
    }
}

void HAPSessionInvalidate(HAPAccessoryServer* server, HAPSession* session, bool terminateLink HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(session);

    HAPLogDebug(&logObject, "%s", __func__);

    // If Session is active, Invalidate dependent state.
    if (session->hap.active) {
        // Invalidate transport specific dependent state.
        switch (session->transportType) {
            case kHAPTransportType_IP: {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
                HAPAssert(server->transports.ip);
                HAPNonnull(server->transports.ip)->session.invalidateDependentIPState(server, session);
                break;
#endif
            }
            case kHAPTransportType_BLE: {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
                HAPAssert(server->transports.ble);
                // To support Pair Resume, DataStreams are not invalidated unless a pairing has been evicted from the
                // BLE cache.
                break;
#endif
            }
            case kHAPTransportType_Thread: {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
                HAPAssert(server->transports.thread);
                HAPNonnull(server->transports.thread)->session.willInvalidate(server, session);
                HAPAssert(HAPThreadSessionStorageIsEmpty(server, session));
                break;
#endif
            }
            default:
                HAPFatalError();
        }

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
        // Inform application.
        if (server->transports.ble) {
            HAPNonnull(server->transports.ble)->peripheralManager.handleSessionInvalidate(server, session);
        }
#endif
        if (server->callbacks.handleSessionInvalidate) {
            server->callbacks.handleSessionInvalidate(server, session, server->context);
        }
    }

#if HAP_FEATURE_ENABLED(HAP_FEATURE_ACCESS_CODE)
    // Let access code service handle session close.
    // An alternative is to have the access code supporting app to call the following instead
    // in its respective handleSessionInvalidate() callback.
    HAPHandleAccessCodeSessionInvalidate(server, session);
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_NFC_ACCESS)
    // Let NFC access service handle session close
    HAPHandleNfcAccessSessionInvalidate(server, session);
#endif

    // Handle pending actions upon session invalidation
    HAPPairingPairingsHandleSessionInvalidation(server, session);

    // Clear security state.
    HAPPairingPairSetupResetForSession(server, session);
    HAPRawBufferZero(&session->hap, sizeof session->hap);
    HAPRawBufferZero(&session->state, sizeof session->state);

    // Re-initialize session state.
    HAPPairingPairVerifyReset(session);
    HAPPairingPairingsReset(session);

    // Invalidate transport-specific state, regardless of whether session is active (or not).
    switch (session->transportType) {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
        case kHAPTransportType_IP: {
            HAPAssert(server->transports.ip);
            return;
        }
#endif
#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
        case kHAPTransportType_BLE: {
            HAPAssert(server->transports.ble);
            HAPNonnull(server->transports.ble)->session.invalidate(server, &session->_.ble, terminateLink);
            return;
        }
#endif
#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
        case kHAPTransportType_Thread: {
            HAPAssert(server->transports.thread);
            HAPAssert(HAPThreadSessionStorageIsEmpty(server, session));
            return;
        }
#endif
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
bool HAPSessionIsSecured(const HAPSession* session) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);

    HAPError err;

    // Pairing is active when the Pair Verify procedure ran through.
    if (!session->hap.active) {
        return false;
    }

    // Check for transient session.
    if (HAPSessionIsTransient(session)) {
        return true;
    }

    // To detect concurrent Remove Pairing operations, the persistent cache is also checked.
    HAPAssert(session->hap.pairingID >= 0);

    bool exists;
    HAPPairing pairing;
    err = HAPPairingGet(session->server, (HAPPairingIndex) session->hap.pairingID, &pairing, &exists);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return false;
    }
    if (!exists) {
        return false;
    }

    return true;
}

HAP_RESULT_USE_CHECK
bool HAPSessionIsTransient(const HAPSession* session) {
    HAPPrecondition(session);

    if (!session->hap.active) {
        return false;
    }

    if (session->hap.isTransient) {
        HAPAssert(!HAPAccessoryServerIsPaired(session->server));
    }
    return session->hap.isTransient;
}

HAP_RESULT_USE_CHECK
bool HAPSessionControllerIsAdmin(const HAPSession* session) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);

    HAPError err;

    if (!session->hap.active) {
        return false;
    }
    if (HAPSessionIsTransient(session)) {
        return false;
    }

    HAPAssert(session->hap.pairingID >= 0);

    bool exists;
    HAPPairing pairing;
    err = HAPPairingGet(session->server, (HAPPairingIndex) session->hap.pairingID, &pairing, &exists);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return false;
    }
    if (!exists) {
        return false;
    }
    return (pairing.permissions & 0x01U) == 0x01;
}

HAP_RESULT_USE_CHECK
HAPError HAPSessionHandlePairSetupWrite(HAPAccessoryServer* server, HAPSession* session, HAPTLVReader* requestReader) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(requestReader);

    HAPError err;

    bool wasPaired = HAPAccessoryServerIsPaired(server);
    err = HAPPairingPairSetupHandleWrite(server, session, requestReader);

    if (err) {
        HAPAssert(err == kHAPError_InvalidState || err == kHAPError_InvalidData || err == kHAPError_OutOfResources);
        return err;
    }

    bool isPaired = HAPAccessoryServerIsPaired(server);

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    if (session->transportType == kHAPTransportType_BLE && isPaired) {
        HAPAssert(server->transports.ble);
        HAPNonnull(server->transports.ble)->session.didPairSetupProcedure(server, session);
    }
#endif

    HAPSessionAccessoryServerPairingUpdate(server, session, wasPaired, isPaired);

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPSessionHandlePairSetupRead(HAPAccessoryServer* server, HAPSession* session, HAPTLVWriter* responseWriter) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(responseWriter);

    HAPError err;

    bool wasPaired = HAPAccessoryServerIsPaired(server);
    err = HAPPairingPairSetupHandleRead(server, session, responseWriter);
    if (err) {
        HAPAssert(err == kHAPError_InvalidState || err == kHAPError_Unknown || err == kHAPError_OutOfResources);
        return err;
    }
    bool isPaired = HAPAccessoryServerIsPaired(server);

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    if (session->transportType == kHAPTransportType_BLE && isPaired) {
        HAPAssert(server->transports.ble);
        HAPNonnull(server->transports.ble)->session.didPairSetupProcedure(server, session);
    }
#endif

    HAPSessionAccessoryServerPairingUpdate(server, session, wasPaired, isPaired);

    return kHAPError_None;
}

/**
 * Reports the start of a pairing procedure.
 *
 * @param      server              Accessory server.
 * @param      session             Session.
 * @param      pairingProcedureType Pairing procedure type.
 */
static void HAPSessionStartPairingProcedure(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPPairingProcedureType pairingProcedureType HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(session);

    switch (session->transportType) {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
        case kHAPTransportType_IP: {
            HAPAssert(server->transports.ip);
            return;
        }
#endif
#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
        case kHAPTransportType_BLE: {
            HAPAssert(server->transports.ble);
            HAPNonnull(server->transports.ble)->session.didStartPairingProcedure(server, session, pairingProcedureType);
            return;
        }
#endif
#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
        case kHAPTransportType_Thread: {
            HAPAssert(server->transports.thread);
            return;
        }
#endif
        default:
            HAPFatalError();
    }
}

/**
 * Reports the completion of a pairing procedure.
 *
 * @param      server              Accessory server.
 * @param      session             Session.
 * @param      pairingProcedureType Pairing procedure type.
 */
static void HAPSessionCompletePairingProcedure(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPPairingProcedureType pairingProcedureType HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(session);

    switch (session->transportType) {
        case kHAPTransportType_IP: {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
            HAPAssert(server->transports.ip);
            return;
#endif
        }
        case kHAPTransportType_BLE: {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
            HAPAssert(server->transports.ble);
            HAPNonnull(server->transports.ble)
                    ->session.didCompletePairingProcedure(server, session, pairingProcedureType);
            return;
#endif
        }
        case kHAPTransportType_Thread: {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
            HAPAssert(server->transports.thread);
            return;
#endif
        }
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPSessionHandlePairVerifyWrite(HAPAccessoryServer* server, HAPSession* session, HAPTLVReader* requestReader) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(requestReader);

    HAPError err;

    if (session->state.pairVerify.state) {
        HAPSessionStartPairingProcedure(server, session, kHAPPairingProcedureType_PairVerify);
    }

    err = HAPPairingPairVerifyHandleWrite(server, session, requestReader);
    if (err) {
        HAPAssert(err == kHAPError_InvalidState || err == kHAPError_InvalidData || err == kHAPError_OutOfResources);
        return err;
    }

    if (!session->state.pairVerify.state) {
        HAPSessionCompletePairingProcedure(server, session, kHAPPairingProcedureType_PairVerify);
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPSessionHandlePairVerifyRead(HAPAccessoryServer* server, HAPSession* session, HAPTLVWriter* responseWriter) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(responseWriter);

    HAPError err;
    if (session->state.pairVerify.state) {
        HAPSessionStartPairingProcedure(server, session, kHAPPairingProcedureType_PairVerify);
    }

    err = HAPPairingPairVerifyHandleRead(server, session, responseWriter);
    if (err) {
        HAPAssert(err == kHAPError_InvalidState || err == kHAPError_OutOfResources);
        return err;
    }

    if (!session->state.pairVerify.state) {
        HAPSessionCompletePairingProcedure(server, session, kHAPPairingProcedureType_PairVerify);
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPSessionHandlePairingsWrite(HAPAccessoryServer* server, HAPSession* session, HAPTLVReader* requestReader) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(requestReader);

    HAPError err;

    if (session->state.pairings.state) {
        HAPSessionStartPairingProcedure(server, session, kHAPPairingProcedureType_PairingPairings);
    }

    bool wasPaired = HAPAccessoryServerIsPaired(server);
    err = HAPPairingPairingsHandleWrite(server, session, requestReader);
    if (err) {
        HAPAssert(err == kHAPError_InvalidState || err == kHAPError_InvalidData);
        return err;
    }
    bool isPaired = HAPAccessoryServerIsPaired(server);

    HAPSessionAccessoryServerPairingUpdate(server, session, wasPaired, isPaired);

    if (!session->state.pairings.state) {
        HAPSessionCompletePairingProcedure(server, session, kHAPPairingProcedureType_PairingPairings);
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPSessionHandlePairingsRead(HAPAccessoryServer* server, HAPSession* session, HAPTLVWriter* responseWriter) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(responseWriter);

    HAPError err;

    if (session->state.pairings.state) {
        HAPSessionStartPairingProcedure(server, session, kHAPPairingProcedureType_PairingPairings);
    }

    bool wasPaired = HAPAccessoryServerIsPaired(server);
    err = HAPPairingPairingsHandleRead(server, session, responseWriter);
    if (err) {
        HAPAssert(err == kHAPError_InvalidState || err == kHAPError_OutOfResources);
        return err;
    }
    bool isPaired = HAPAccessoryServerIsPaired(server);

    HAPSessionAccessoryServerPairingUpdate(server, session, wasPaired, isPaired);

    if (!session->state.pairings.state) {
        HAPSessionCompletePairingProcedure(server, session, kHAPPairingProcedureType_PairingPairings);
    }

    return kHAPError_None;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HAP_RESULT_USE_CHECK
static HAPError
        Encrypt(HAPSessionChannelState* channel,
                void* encryptedBytes_,
                const void* plaintextBytes_,
                size_t numPlaintextBytes,
                const void* _Nullable aadBytes_,
                size_t numAADBytes) {
    HAPPrecondition(channel);
    HAPPrecondition(encryptedBytes_);
    uint8_t* encryptedBytes = encryptedBytes_;
    HAPPrecondition(plaintextBytes_);
    const uint8_t* plaintextBytes = plaintextBytes_;
    HAPPrecondition(!numAADBytes || aadBytes_);
    const uint8_t* _Nullable aadBytes = aadBytes_;

    // Encrypt message. Tag is appended to cipher text.
    uint8_t nonce[] = { HAPExpandLittleUInt64(channel->nonce) };
    if (aadBytes) {
        HAP_chacha20_poly1305_encrypt_aad(
                /* tag: */ &encryptedBytes[numPlaintextBytes],
                encryptedBytes,
                plaintextBytes,
                numPlaintextBytes,
                aadBytes,
                numAADBytes,
                nonce,
                sizeof nonce,
                channel->key.bytes);
    } else {
        HAP_chacha20_poly1305_encrypt(
                /* tag: */ &encryptedBytes[numPlaintextBytes],
                encryptedBytes,
                plaintextBytes,
                numPlaintextBytes,
                nonce,
                sizeof nonce,
                channel->key.bytes);
    }

    // Increment message counter.
    channel->nonce++;

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPSessionEncryptControlMessage(
        const HAPAccessoryServer* server,
        HAPSession* session,
        void* encryptedBytes,
        const void* plaintextBytes,
        size_t numPlaintextBytes) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(encryptedBytes);
    HAPPrecondition(plaintextBytes);

    if (!session->hap.active) {
        HAPLog(&logObject, "Cannot encrypt message: Session not active.");
        return kHAPError_InvalidState;
    }

    if (SessionKeyExpired(session)) {
        HAPLog(&logObject, "Cannot encrypt message: Session key expired.");
        return kHAPError_InvalidState;
    }

    return Encrypt(
            &session->hap.accessoryToController.controlChannel,
            encryptedBytes,
            plaintextBytes,
            numPlaintextBytes,
            /* aadBytes: */ NULL,
            /* numAADBytes: */ 0);
}

HAP_RESULT_USE_CHECK
HAPError HAPSessionEncryptControlMessageWithAAD(
        const HAPAccessoryServer* server,
        HAPSession* session,
        void* encryptedBytes,
        const void* plaintextBytes,
        size_t numPlaintextBytes,
        const void* aadBytes,
        size_t numAADBytes) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(encryptedBytes);
    HAPPrecondition(plaintextBytes);
    HAPPrecondition(aadBytes);

    if (!session->hap.active) {
        HAPLog(&logObject, "Cannot encrypt message: Session not active.");
        return kHAPError_InvalidState;
    }

    if (SessionKeyExpired(session)) {
        HAPLog(&logObject, "Cannot encrypt message: Session key expired.");
        return kHAPError_InvalidState;
    }

    return Encrypt(
            &session->hap.accessoryToController.controlChannel,
            encryptedBytes,
            plaintextBytes,
            numPlaintextBytes,
            aadBytes,
            numAADBytes);
}

HAP_RESULT_USE_CHECK
HAPError HAPSessionEncryptEventMessage(
        const HAPAccessoryServer* server,
        HAPSession* session,
        void* encryptedBytes,
        const void* plaintextBytes,
        size_t numPlaintextBytes) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(encryptedBytes);
    HAPPrecondition(plaintextBytes);

    if (!session->hap.active) {
        HAPLog(&logObject, "Cannot encrypt event message: Session not active.");
        return kHAPError_InvalidState;
    }
    if (HAPSessionIsTransient(session)) {
        HAPLog(&logObject, "Cannot encrypt event message: Session is transient.");
        return kHAPError_InvalidState;
    }

    return Encrypt(
            &session->hap.accessoryToController.eventChannel,
            encryptedBytes,
            plaintextBytes,
            numPlaintextBytes,
            /* aadBytes: */ NULL,
            /* numAADBytes: */ 0);
}

void HAPSessionClearControlKeys(HAPSession* session) {
    HAPPrecondition(session);

    HAPRawBufferZero(
            &session->hap.accessoryToController.controlChannel,
            sizeof session->hap.accessoryToController.controlChannel);
    HAPRawBufferZero(
            &session->hap.controllerToAccessory.controlChannel,
            sizeof session->hap.controllerToAccessory.controlChannel);
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
static HAPError
        Decrypt(HAPSessionChannelState* channel,
                void* plaintextBytes_,
                const void* encryptedBytes_,
                size_t numEncryptedBytes,
                const void* _Nullable aadBytes_,
                size_t numAADBytes) {
    HAPPrecondition(channel);
    HAPPrecondition(plaintextBytes_);
    uint8_t* plaintextBytes = plaintextBytes_;
    HAPPrecondition(encryptedBytes_);
    const uint8_t* encryptedBytes = encryptedBytes_;
    HAPPrecondition(!numAADBytes || aadBytes_);
    const uint8_t* aadBytes = aadBytes_;

    // Decrypt message. Tag is appended to cipher text.
    if (numEncryptedBytes < CHACHA20_POLY1305_TAG_BYTES) {
        HAPLog(&logObject, "Ciphertext not long enough for auth tag (length %zu).", numEncryptedBytes);
        return kHAPError_InvalidData;
    }

    uint8_t nonce[] = { HAPExpandLittleUInt64(channel->nonce) };
    if (aadBytes) {
        int e = HAP_chacha20_poly1305_decrypt_aad(
                /* tag: */ &encryptedBytes[numEncryptedBytes - CHACHA20_POLY1305_TAG_BYTES],
                plaintextBytes,
                encryptedBytes,
                /* c_len: */ numEncryptedBytes - CHACHA20_POLY1305_TAG_BYTES,
                aadBytes,
                numAADBytes,
                nonce,
                sizeof nonce,
                channel->key.bytes);
        if (e) {
            HAPAssert(e == -1);
            HAPLog(&logObject, "Decryption of message %llu failed.", (unsigned long long) channel->nonce);
            HAPLogSensitiveBuffer(&logObject, channel->key.bytes, sizeof channel->key.bytes, "Decryption key.");
            return kHAPError_InvalidData;
        }
    } else {
        int e = HAP_chacha20_poly1305_decrypt(
                /* tag: */ &encryptedBytes[numEncryptedBytes - CHACHA20_POLY1305_TAG_BYTES],
                plaintextBytes,
                encryptedBytes,
                /* c_len: */ numEncryptedBytes - CHACHA20_POLY1305_TAG_BYTES,
                nonce,
                sizeof nonce,
                channel->key.bytes);
        if (e) {
            HAPAssert(e == -1);
            HAPLog(&logObject, "Decryption of message %llu failed.", (unsigned long long) channel->nonce);
            HAPLogSensitiveBuffer(&logObject, channel->key.bytes, sizeof channel->key.bytes, "Decryption key.");
            return kHAPError_InvalidData;
        }
    }

    // Increment message counter.
    channel->nonce++;

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPSessionDecryptControlMessage(
        HAPAccessoryServer* server,
        HAPSession* session,
        void* plaintextBytes,
        const void* encryptedBytes,
        size_t numEncryptedBytes) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(plaintextBytes);
    HAPPrecondition(encryptedBytes);

    HAPError err;

    if (!session->hap.active) {
        HAPLog(&logObject, "Cannot decrypt message: Session not active.");
        return kHAPError_InvalidState;
    }

    if (SessionKeyExpired(session)) {
        HAPLog(&logObject, "Cannot decrypt message: Session key expired.");
        return kHAPError_InvalidState;
    }

    err =
            Decrypt(&session->hap.controllerToAccessory.controlChannel,
                    plaintextBytes,
                    encryptedBytes,
                    numEncryptedBytes,
                    /* aadBytes: */ NULL,
                    /* numAADBytes: */ 0);
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPSessionDecryptControlMessageWithAAD(
        HAPAccessoryServer* server,
        HAPSession* session,
        void* plaintextBytes,
        const void* encryptedBytes,
        size_t numEncryptedBytes,
        const void* aadBytes,
        size_t numAADBytes) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(plaintextBytes);
    HAPPrecondition(encryptedBytes);
    HAPPrecondition(aadBytes);

    HAPError err;

    if (!session->hap.active) {
        HAPLog(&logObject, "Cannot decrypt message: Session not active.");
        return kHAPError_InvalidState;
    }

    if (SessionKeyExpired(session)) {
        HAPLog(&logObject, "Cannot decrypt message: Session key expired.");
        return kHAPError_InvalidState;
    }

    err =
            Decrypt(&session->hap.controllerToAccessory.controlChannel,
                    plaintextBytes,
                    encryptedBytes,
                    numEncryptedBytes,
                    aadBytes,
                    numAADBytes);
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }

    return kHAPError_None;
}
