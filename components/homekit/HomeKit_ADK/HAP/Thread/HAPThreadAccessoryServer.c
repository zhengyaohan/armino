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

#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)

#include "HAPAccessoryServer+Info.h"
#include "HAPAccessoryServer+Internal.h"
#include "HAPLogSubsystem.h"
#include "HAPNotification+Delivery.h"
#include "HAPPDU+NotificationConfiguration.h"
#include "HAPPDU.h"
#include "HAPPlatformThreadUtils+Init.h"
#include "HAPSession.h"
#include "HAPTLV+Internal.h"
#include "HAPThreadAccessoryServer+NetworkJoin.h"
#include "HAPThreadAccessoryServer+Reachability.h"
#include "HAPThreadAccessoryServer.h"

#include "util_base64.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "ThreadAccessoryServer" };

// Discovery.
// The Bonjour service type for Thread is: _hap._udp
//
// +------+---------------------------------------------------------------------------------------------------+
// | Key  | Description                                                                                       |
// +======+===================================================================================================+
// | "ip" | IP address where CoAP resources are being served.                                                 |
// |      | - Must be formatted as an IP address string value, e.g., FD11:0022:XXXX:XXXX:XXXX:XXXX:XXXX:XXXX. |
// +------+---------------------------------------------------------------------------------------------------+
//
// See HomeKit Accessory Protocol Specification R17
// Section 6.4 Discovery

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Delay between transport stop retry attempt */
#define kHAPThreadAccessoryServer_StopRetryTime ((HAPTime)(500 * HAPMillisecond))

/**
 * Thread sessions are selected for eviction based on whether
 * the session is active (Pair verified) and its idle time.
 *
 * Any session that is inactive and idle for > MaxInactiveTime
 * will be reaped first.
 *
 * If there are none, any session that is idle for > MaxIdleTime
 * will be chosen.
 *
 * If there still are none, the oldest inactive session will be
 * chosen.
 *
 * If there are still no valid sessions for removal, the request
 * for a new session will be denied.
 */
#define kHAPThreadAccessoryServer_MaxIdleTime     ((HAPTime)(12 * HAPSecond))
#define kHAPThreadAccessoryServer_MaxInactiveTime ((HAPTime)(2 * HAPSecond))

static HAPPlatformThreadWakeLock kPairSetupWakeLock;
static HAPPlatformThreadWakeLock kThreadStartWakeLock;

#ifndef UNPAIR_WAKELOCK_TIME
#define UNPAIR_WAKELOCK_TIME ((HAPTime)(5 * HAPMinute))
#endif
#ifndef REBOOT_WAKELOCK_TIME
#define REBOOT_WAKELOCK_TIME ((HAPTime)(5 * HAPMinute))
#endif

#define START_WAKELOCK_TIME ((HAPTime)(1 * HAPMinute))

#define MIN_RESP_PDU_SIZE 5

#define GENERATE_ERROR_RESPONSE_PDU(respBuf, rspBufSize, TID, err) \
    (respBuf)[0] = 0x02; \
    (respBuf)[1] = (TID); \
    (respBuf)[2] = (err); \
    (respBuf)[3] = 0x00; \
    (respBuf)[4] = 0x00; \
    *(rspBufSize) += MIN_RESP_PDU_SIZE;

static void StartThreadSession(
        HAPAccessoryServer* server,
        HAPThreadSession* threadSession,
        const HAPPlatformThreadCoAPManagerPeer* peer) {
    HAPPrecondition(server);
    HAPPrecondition(threadSession);
    HAPSession* session = &threadSession->hapSession;

    HAPLogBufferInfo(&logObject, peer, sizeof *peer, "[%p] Starting session.", (const void*) session);
    HAPRawBufferZero(HAPNonnull(threadSession), sizeof *threadSession);
    HAPRawBufferCopyBytes(&threadSession->peer, peer, sizeof threadSession->peer);
    threadSession->lastActivity = 1;
    threadSession->lastReceived = HAPPlatformClockGetCurrent();
    HAPSessionCreate(server, session, kHAPTransportType_Thread);
}

static void TerminateThreadSession(HAPAccessoryServer* server, HAPThreadSession* threadSession) {
    HAPPrecondition(server);
    HAPPrecondition(threadSession);
    HAPPrecondition(!threadSession->isSendingEvent);
    HAPSession* session = &threadSession->hapSession;

    HAPLogBufferInfo(
            &logObject,
            &threadSession->peer,
            sizeof threadSession->peer,
            "[%p] Terminating session.",
            (const void*) session);
    HAPSessionRelease(server, session);
    HAPRawBufferZero(threadSession, sizeof *threadSession);
}

HAP_RESULT_USE_CHECK
static HAPThreadSession* _Nullable ResumeThreadSessionForPeer(
        HAPAccessoryServer* server,
        const HAPPlatformThreadCoAPManagerPeer* peer,
        bool allowNewSession) {
    HAPPrecondition(server);
    HAPPrecondition(peer);

    HAPTime now = HAPPlatformClockGetCurrent();
    now = HAPMax(now, 1);

    char ipv6[41];
    HAPError hapErr = HAPIPv6AddressGetDescription(&peer->ipAddress._.ipv6, ipv6, sizeof(ipv6));
    HAPAssert(hapErr == kHAPError_None);

    size_t unusedSlot = 0;
    size_t idleSlot = 0;
    HAPTime idleTime = 0;
    size_t inactiveSlot = 0;
    HAPTime inactiveTime = 0;

    HAPLogInfo(&logObject, "Thread Session requested for peer [%s]:%u.", ipv6, peer->port);
    // If a session has already been started for the given peer, resume it.
    HAPThreadSession* _Nullable unusedSession_ = NULL;
    HAPThreadSession* _Nullable idleSession_ = NULL;
    HAPThreadSession* _Nullable inactiveSession_ = NULL;
    for (size_t i = 0; i < server->thread.storage->numSessions; i++) {
        HAPThreadSession* threadSession = &server->thread.storage->sessions[i];

        if (!threadSession->lastActivity) {
            HAPLogInfo(&logObject, "    Session Slot [%zu] Empty", i);
            if (!unusedSession_) {
                unusedSession_ = threadSession;
                unusedSlot = i;
            }
        } else if (HAPThreadAccessoryServerReleaseThreadSessionIfExpired(server, threadSession)) {
            // Session expired. Potentially reuse the slot.
            HAPLogInfo(&logObject, "    Session Slot [%zu] Expired and reusable", i);
            if (!unusedSession_) {
                unusedSession_ = threadSession;
                unusedSlot = i;
            }
        } else {

            hapErr = HAPIPv6AddressGetDescription(&threadSession->peer.ipAddress._.ipv6, ipv6, sizeof(ipv6));
            HAPAssert(hapErr == kHAPError_None);
            HAPTime delta = now - HAPNonnull(threadSession)->lastActivity;
            HAPLogInfo(
                    &logObject,
                    "    Session Slot [%zu] In use by [%s]:%u (last activity: %llu.%03llus ago, sending evt = %d, actv "
                    "%d",
                    i,
                    ipv6,
                    threadSession->peer.port,
                    (unsigned long long) (delta / HAPSecond),
                    (unsigned long long) (delta % HAPSecond),
                    threadSession->isSendingEvent,
                    threadSession->hapSession.hap.active);

            if (!idleSession_ || threadSession->lastActivity < HAPNonnull(idleSession_)->lastActivity) {
                if (delta >= kHAPThreadAccessoryServer_MaxIdleTime) {
                    if (!threadSession->isSendingEvent) {
                        idleSession_ = threadSession;
                        idleSlot = i;
                        idleTime = delta;
                    }
                }
            }

            if (!threadSession->hapSession.hap.active) {
                // Find the oldest inactive session
                if (!inactiveSession_ || threadSession->lastActivity < HAPNonnull(inactiveSession_)->lastActivity) {
                    inactiveSession_ = threadSession;
                    inactiveSlot = i;
                    inactiveTime = delta;
                }
            }

            if (!HAPIPAddressAreEqual(&peer->ipAddress, &threadSession->peer.ipAddress)) {
                continue;
            }
            if (peer->port != threadSession->peer.port) {
                continue;
            }

            HAPLogInfo(&logObject, "       Matches Request, continue with existing session");
            threadSession->lastActivity = now;
            return threadSession;
        }
    }

    // Only create a new session if endpoint allows
    if (allowNewSession) {
        // We couldn't find an unused session, but have potential replacements.  Choose the best one.
        if (!unusedSession_ && (inactiveSession_ || idleSession_)) {
            if (inactiveSession_ && (inactiveTime >= kHAPThreadAccessoryServer_MaxInactiveTime)) {
                TerminateThreadSession(server, HAPNonnull(inactiveSession_));
                unusedSession_ = inactiveSession_;
                unusedSlot = inactiveSlot;
            } else if (idleSession_ && (idleTime >= kHAPThreadAccessoryServer_MaxIdleTime)) {
                TerminateThreadSession(server, HAPNonnull(idleSession_));
                unusedSession_ = idleSession_;
                unusedSlot = idleSlot;
            } else if (inactiveSession_) {
                TerminateThreadSession(server, HAPNonnull(inactiveSession_));
                unusedSession_ = inactiveSession_;
                unusedSlot = inactiveSlot;
            } else {
                // Shouldn't be possible to get here.  If we had inactiveSession or idleSession we should have
                // satisfied one of the above cases.
                HAPLogError(&logObject, "Invalid Session Log State");
                HAPFatalError();
            }
            HAPLogInfo(&logObject, "Terminated inactive/Idle session [%zu] to allow accepting a new one.", unusedSlot);
        }

        if (unusedSession_) {
            HAPLogInfo(&logObject, "Accepting session to slot [%zu]", unusedSlot);
            StartThreadSession(server, HAPNonnull(unusedSession_), peer);
            HAPNonnull(unusedSession_)->lastActivity = now;
            return HAPNonnull(unusedSession_);
        }
    }
    HAPLogBuffer(&logObject, peer, sizeof *peer, "Cannot accept this session.");
    return NULL;
}

static void PurgeTransientSessions(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    for (size_t i = 0; i < server->thread.storage->numSessions; i++) {
        HAPThreadSession* threadSession = &server->thread.storage->sessions[i];
        HAPSession* hapSession = &threadSession->hapSession;

        if (hapSession->hap.isTransient) {
            TerminateThreadSession(server, HAPNonnull(threadSession));
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void Create(HAPAccessoryServer* server, const HAPAccessoryServerOptions* options) {
    HAPPrecondition(server);
    HAPPrecondition(options);

    HAPPrecondition(options->sessionStorage.bytes);

    HAPPrecondition(server->platform.thread.coapManager);
    HAPPrecondition(server->platform.thread.serviceDiscovery);

    // Initialize Thread storage.
    HAPPrecondition(options->thread.accessoryServerStorage);
    HAPThreadAccessoryServerStorage* storage = options->thread.accessoryServerStorage;
    HAPPrecondition(storage->sessions);
    HAPRawBufferZero(storage->sessions, storage->numSessions * sizeof storage->sessions[0]);

    HAPPrecondition(options->thread.getNextReattachDelay);
    storage->getNextReattachDelay = options->thread.getNextReattachDelay;
    storage->getNextReattachDelayContext = options->thread.getNextReattachDelayContext;

    server->thread.storage = storage;

#if HAP_FEATURE_ENABLED(HAP_FEATURE_ACCESS_CODE)
    // Copy Access Code service parameters
    if (options->accessCode.responseStorage) {
        HAPAssert(options->accessCode.handleOperation);

        HAPRawBufferCopyBytes(
                &server->accessCode.responseStorage,
                HAPNonnull(options->accessCode.responseStorage),
                sizeof server->accessCode.responseStorage);

        server->accessCode.handleOperation = options->accessCode.handleOperation;
        server->accessCode.operationCtx = options->accessCode.operationCtx;
    }
    server->accessCode.numResponseBytes = 0;
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_NFC_ACCESS)
    // Copy NFC Access service parameters
    if (options->nfcAccess.responseStorage) {
        HAPRawBufferCopyBytes(
                &server->nfcAccess.responseStorage,
                HAPNonnull(options->nfcAccess.responseStorage),
                sizeof server->nfcAccess.responseStorage);
    }
    server->nfcAccess.numResponseBytes = 0;
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void PrepareStart(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    HAPThreadAccessoryServerStorage* storage = HAPNonnull(server->thread.storage);
    HAPRawBufferZero(storage->sessions, storage->numSessions * sizeof storage->sessions[0]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void HandleReadyToSendRequest(HAPPlatformThreadCoAPManagerRef coapManager, void* _Nullable context) {
    HAPPrecondition(coapManager);
    HAPPrecondition(context);
    HAPAccessoryServer* server = context;

    HAPLogDebug(&logObject, "Ready to send another CoAP request.");
    HAPNotificationContinueSending(server);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void UpdateAdvertisingData(HAPAccessoryServer* server, bool updateTxtRecord);

HAP_RESULT_USE_CHECK
static HAPPlatformThreadCoAPManagerResponseCode ResponseCodeForError(HAPError error) {

    // return 404 for ANY error so as to leak less information.
    return error == kHAPError_None ? kHAPPlatformThreadCoAPManagerResponseCode_Changed :
                                     kHAPPlatformThreadCoAPManagerResponseCode_NotFound;
}

//----------------------------------------------------------------------------------------------------------------------

static void HandleIdentifyRequest(
        HAPPlatformThreadCoAPManagerRef coapManager,
        const HAPPlatformThreadCoAPManagerPeer* peer,
        HAPPlatformThreadCoAPManagerResourceRef coapResource,
        HAPPlatformThreadCoAPManagerRequestCode requestCode,
        void* requestBytes,
        size_t numRequestBytes,
        HAPPlatformThreadCoAPManagerResponseCode* responseCode,
        void* responseBytes,
        size_t maxResponseBytes HAP_UNUSED,
        size_t* numResponseBytes,
        HAPPlatformThreadCoAPManagerShortResponse* _Nullable shortResponse HAP_UNUSED,
        void* _Nullable context) {
    HAPPrecondition(coapManager);
    HAPPrecondition(peer);
    HAPPrecondition(coapResource);
    const char* resourceDescription = "identify";
    HAPPrecondition(requestBytes);
    HAPPrecondition(responseCode);
    HAPPrecondition(responseBytes);
    HAPPrecondition(numResponseBytes);
    HAPPrecondition(context);
    HAPAccessoryServer* server = context;
    HAPPrecondition(coapResource == server->thread.coap.identifyResource);

    HAPThreadSession* _Nullable threadSession = ResumeThreadSessionForPeer(server, peer, true);
    if (!threadSession) {
        *responseCode = kHAPPlatformThreadCoAPManagerResponseCode_NotFound;
        *numResponseBytes = 0;
        return;
    }
    HAPSession* session = &threadSession->hapSession;

    if (requestCode != kHAPPlatformThreadCoAPManagerRequestCode_POST) {
        HAPLog(&logObject,
               "[%p] Unexpected %s request code %d. Rejecting.",
               (const void*) session,
               resourceDescription,
               requestCode);
        *responseCode = kHAPPlatformThreadCoAPManagerResponseCode_NotFound;
        *numResponseBytes = 0;
        return;
    }

    // Unpaired identify is only available while unpaired.
    if (HAPAccessoryServerIsPaired(server)) {
        HAPLog(&logObject,
               "[%p] Rejecting %s request: Accessory server is paired.",
               (const void*) session,
               resourceDescription);
        *responseCode = kHAPPlatformThreadCoAPManagerResponseCode_NotFound;
        *numResponseBytes = 0;
        return;
    }

    // Validate request.
    if (numRequestBytes != 0) {
        HAPLogBuffer(
                &logObject,
                requestBytes,
                numRequestBytes,
                "[%p] Unexpected %s request.",
                (const void*) session,
                resourceDescription);
        *responseCode = kHAPPlatformThreadCoAPManagerResponseCode_NotFound;
        *numResponseBytes = 0;
        return;
    }
    HAPLogBufferDebug(
            &logObject,
            requestBytes,
            numRequestBytes,
            "[%p] Received %s request.",
            (const void*) session,
            resourceDescription);

    // Identify accessory.
    HAPAccessoryServerIdentify(server, session);

    // Send response.
    *responseCode = kHAPPlatformThreadCoAPManagerResponseCode_Changed;
    *numResponseBytes = 0;
    HAPLogBufferDebug(
            &logObject,
            responseBytes,
            *numResponseBytes,
            "[%p] Sending %s response.",
            (const void*) session,
            resourceDescription);
}

//----------------------------------------------------------------------------------------------------------------------

static void ProcessPairingRequest(
        HAPAccessoryServer* server,
        HAPPlatformThreadCoAPManagerRef coapManager,
        const HAPPlatformThreadCoAPManagerPeer* peer,
        HAPPlatformThreadCoAPManagerResourceRef coapResource,
        const char* resourceDescription,
        HAPError (*writeHandler)(HAPAccessoryServer*, HAPSession*, HAPTLVReader*),
        HAPError (*readHandler)(HAPAccessoryServer*, HAPSession*, HAPTLVWriter*),
        HAPPlatformThreadCoAPManagerRequestCode requestCode,
        void* requestBytes,
        size_t numRequestBytes,
        HAPPlatformThreadCoAPManagerResponseCode* responseCode,
        void* responseBytes,
        size_t maxResponseBytes,
        size_t* numResponseBytes) {
    HAPPrecondition(server);
    HAPPrecondition(coapManager);
    HAPPrecondition(peer);
    HAPPrecondition(coapResource);
    HAPPrecondition(resourceDescription);
    HAPPrecondition(writeHandler);
    HAPPrecondition(readHandler);
    HAPPrecondition(requestBytes);
    HAPPrecondition(responseCode);
    HAPPrecondition(responseBytes);
    HAPPrecondition(numResponseBytes);

    HAPError err;

    HAPThreadSession* _Nullable threadSession = ResumeThreadSessionForPeer(server, peer, true);
    if (!threadSession) {
        *responseCode = kHAPPlatformThreadCoAPManagerResponseCode_NotFound;
        *numResponseBytes = 0;
        return;
    }
    HAPSession* session = &threadSession->hapSession;

    if (requestCode != kHAPPlatformThreadCoAPManagerRequestCode_POST) {
        HAPLog(&logObject,
               "[%p] Unexpected %s request code %d. Rejecting.",
               (const void*) session,
               resourceDescription,
               requestCode);
        *responseCode = kHAPPlatformThreadCoAPManagerResponseCode_NotFound;
        *numResponseBytes = 0;
        return;
    }

    HAPLogBufferDebug(
            &logObject,
            requestBytes,
            numRequestBytes,
            "[%p] Received %s request.",
            (const void*) session,
            resourceDescription);

    // Move payload to larger buffer when available.
    HAPTLVReader requestReader;
    if (maxResponseBytes > numRequestBytes) {
        HAPRawBufferCopyBytes(responseBytes, requestBytes, numRequestBytes);
        requestBytes = responseBytes;

        HAPTLVReaderOptions options;
        HAPRawBufferZero(&options, sizeof options);
        options.bytes = requestBytes;
        options.numBytes = numRequestBytes;
        options.maxBytes = maxResponseBytes;
        HAPTLVReaderCreateWithOptions(&requestReader, &options);
    } else {
        HAPTLVReaderCreate(&requestReader, requestBytes, numRequestBytes);
    }

    // Handle request.
    err = writeHandler(server, session, &requestReader);
    if (err) {
        HAPAssert(err == kHAPError_InvalidState || err == kHAPError_InvalidData || err == kHAPError_OutOfResources);
        *responseCode = ResponseCodeForError(err);
        *numResponseBytes = 0;
        return;
    }

    // Get response.
    HAPTLVWriter responseWriter;
    HAPTLVWriterCreate(&responseWriter, responseBytes, maxResponseBytes);
    err = readHandler(server, session, &responseWriter);
    if (err) {
        HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources);
        *responseCode = ResponseCodeForError(err);
        *numResponseBytes = 0;
        return;
    }

    // Send response.
    void* bytes;
    size_t numBytes;
    HAPTLVWriterGetBuffer(&responseWriter, &bytes, &numBytes);
    HAPAssert(bytes == responseBytes);
    HAPAssert(numBytes <= maxResponseBytes);
    *responseCode = kHAPPlatformThreadCoAPManagerResponseCode_Changed;
    *numResponseBytes = numBytes;
    HAPLogBufferDebug(
            &logObject,
            responseBytes,
            *numResponseBytes,
            "[%p] Sending %s response.",
            (const void*) session,
            resourceDescription);
}

static void HandlePairSetupRequest(
        HAPPlatformThreadCoAPManagerRef coapManager,
        const HAPPlatformThreadCoAPManagerPeer* peer,
        HAPPlatformThreadCoAPManagerResourceRef coapResource,
        HAPPlatformThreadCoAPManagerRequestCode requestCode,
        void* requestBytes,
        size_t numRequestBytes,
        HAPPlatformThreadCoAPManagerResponseCode* responseCode,
        void* responseBytes,
        size_t maxResponseBytes,
        size_t* numResponseBytes,
        HAPPlatformThreadCoAPManagerShortResponse* _Nullable shortResponse HAP_UNUSED,
        void* _Nullable context) {
    HAPAccessoryServer* server = context;
    HAPPrecondition(coapResource == server->thread.coap.pairSetupResource);

    bool wasPaired = HAPAccessoryServerIsPaired(server);

    ProcessPairingRequest(
            server,
            coapManager,
            peer,
            coapResource,
            "Pair Setup",
            HAPSessionHandlePairSetupWrite,
            HAPSessionHandlePairSetupRead,
            requestCode,
            requestBytes,
            numRequestBytes,
            responseCode,
            responseBytes,
            maxResponseBytes,
            numResponseBytes);
    bool isPaired = HAPAccessoryServerIsPaired(server);

    if (isPaired) {
        PurgeTransientSessions(server);
    }

    if (isPaired != wasPaired) {
        if (isPaired) {
            // Transition from unpaired to paired.  Remove pair setup wakelock.
            HAPPlatformThreadRemoveWakeLock(&kPairSetupWakeLock);
        } else {
            // Transition from paired to unpaired.  Set pair setup wakelock with timeout.
            HAPPlatformThreadAddWakeLock(&kPairSetupWakeLock, UNPAIR_WAKELOCK_TIME);
        }
        UpdateAdvertisingData(server, true);
    }
}

static void HandlePairVerifyRequest(
        HAPPlatformThreadCoAPManagerRef coapManager,
        const HAPPlatformThreadCoAPManagerPeer* peer,
        HAPPlatformThreadCoAPManagerResourceRef coapResource,
        HAPPlatformThreadCoAPManagerRequestCode requestCode,
        void* requestBytes,
        size_t numRequestBytes,
        HAPPlatformThreadCoAPManagerResponseCode* responseCode,
        void* responseBytes,
        size_t maxResponseBytes,
        size_t* numResponseBytes,
        HAPPlatformThreadCoAPManagerShortResponse* _Nullable shortResponse HAP_UNUSED,
        void* _Nullable context) {
    HAPAccessoryServer* server = context;
    HAPPrecondition(coapResource == server->thread.coap.pairVerifyResource);

    ProcessPairingRequest(
            server,
            coapManager,
            peer,
            coapResource,
            "Pair Verify",
            HAPSessionHandlePairVerifyWrite,
            HAPSessionHandlePairVerifyRead,
            requestCode,
            requestBytes,
            numRequestBytes,
            responseCode,
            responseBytes,
            maxResponseBytes,
            numResponseBytes);

    // Now that Pair Verify has begun we can remove the start wakelock.
    // PV maintains its own internal wakelock.
    HAPPlatformThreadRemoveWakeLock(&kThreadStartWakeLock);
}

//----------------------------------------------------------------------------------------------------------------------

static void ProcessRequest(
        HAPAccessoryServer* server,
        HAPPlatformThreadCoAPManagerRef coapManager,
        const HAPPlatformThreadCoAPManagerPeer* peer,
        HAPPlatformThreadCoAPManagerResourceRef coapResource,
        const char* resourceDescription,
        HAPPlatformThreadCoAPManagerRequestCode requestCode,
        void* requestBytes,
        size_t numRequestBytes,
        HAPPlatformThreadCoAPManagerResponseCode* responseCode,
        void* responseBytes,
        size_t maxResponseBytes,
        size_t* numResponseBytes,
        HAPPlatformThreadCoAPManagerShortResponse* _Nullable shortResponse) {
    HAPPrecondition(server);
    HAPPrecondition(coapManager);
    HAPPrecondition(peer);
    HAPPrecondition(coapResource);
    HAPPrecondition(resourceDescription);
    HAPPrecondition(requestBytes);
    HAPPrecondition(responseCode);
    HAPPrecondition(responseBytes);
    HAPPrecondition(numResponseBytes);

    HAPError err;

    HAPThreadSession* _Nullable threadSession = ResumeThreadSessionForPeer(server, peer, false);
    if (!threadSession) {
        *responseCode = kHAPPlatformThreadCoAPManagerResponseCode_NotFound;
        *numResponseBytes = 0;
        return;
    }
    HAPSession* session = &threadSession->hapSession;

    if (requestCode != kHAPPlatformThreadCoAPManagerRequestCode_POST) {
        HAPLog(&logObject,
               "[%p] Unexpected %s request code %d. Rejecting.",
               (const void*) session,
               resourceDescription,
               requestCode);
        *responseCode = kHAPPlatformThreadCoAPManagerResponseCode_NotFound;
        *numResponseBytes = 0;
        return;
    }

    // Secure message is only available while session is secured.
    if (!HAPSessionIsSecured(session) || HAPSessionKeyExpired(session)) {
        HAPLog(&logObject,
               "[%p] Rejecting %s request: Session is not secured.",
               (const void*) session,
               resourceDescription);
        *responseCode = kHAPPlatformThreadCoAPManagerResponseCode_NotFound;
        *numResponseBytes = 0;
        if (HAPSessionIsSecured(session)) {
            // Key has expired. Purge session keys.
            HAPSessionClearControlKeys(session);
        } else {
            return;
        }
    }

    // Check if last received time has expired.
    if (HAPThreadAccessoryServerReleaseThreadSessionIfExpired(server, threadSession)) {
        HAPLog(&logObject, "[%p] Rejecting %s request: Session expired.", (const void*) session, resourceDescription);
        *responseCode = kHAPPlatformThreadCoAPManagerResponseCode_NotFound;
        *numResponseBytes = 0;
        // Note that HAPThreadAccessoryServerReleaseThreadSessionIfExpired() must have already released the session
        return;
    }

    // Validate request.
    if (numRequestBytes < CHACHA20_POLY1305_TAG_BYTES) {
        HAPLogBuffer(
                &logObject,
                requestBytes,
                numRequestBytes,
                "[%p] Unexpected %s request.",
                (const void*) session,
                resourceDescription);
        *responseCode = kHAPPlatformThreadCoAPManagerResponseCode_NotFound;
        *numResponseBytes = 0;
        return;
    }

    if (HAPSessionKeyExpired(session)) {
        // If key has expired, do not try decryption.
        // Decryption failure must release the session but in case control keys have expired,
        // Thread session must remain in order to allow notifications using its event key.
        // Hence, instead of calling HAPSessionDecryptControlMessage() below and
        // handling the result which may cause to release the session,
        // return here with NotFound response code.
        *responseCode = kHAPPlatformThreadCoAPManagerResponseCode_NotFound;
        *numResponseBytes = 0;
        return;
    }

    // Decrypt request.
    err = HAPSessionDecryptControlMessage(server, session, requestBytes, requestBytes, numRequestBytes);
    if (err) {
        HAPAssert(err == kHAPError_InvalidState || err == kHAPError_InvalidData);
        HAPLog(&logObject, "[%p] Failed to decrypt %s request.", (const void*) session, resourceDescription);
        HAPSessionRelease(server, session);
        *responseCode = kHAPPlatformThreadCoAPManagerResponseCode_NotFound;
        *numResponseBytes = 0;
        return;
    }
    numRequestBytes -= CHACHA20_POLY1305_TAG_BYTES;
    HAPLogBufferDebug(
            &logObject,
            requestBytes,
            numRequestBytes,
            "[%p] Received %s request.",
            (const void*) session,
            resourceDescription);

    // Check that there is enough space to encrypt the response.
    if (maxResponseBytes < CHACHA20_POLY1305_TAG_BYTES) {
        HAPLog(&logObject, "[%p] Not enough space to encrypt %s request.", (const void*) session, resourceDescription);
        *responseCode = kHAPPlatformThreadCoAPManagerResponseCode_NotFound;
        *numResponseBytes = 0;
        return;
    }
    *numResponseBytes = 0;
    size_t totalBytesConsumed = 0;

    // Save room for the CHACHA20 tag and a potential error PDU response.
    size_t responseBytesRemaining = maxResponseBytes - CHACHA20_POLY1305_TAG_BYTES - MIN_RESP_PDU_SIZE;

    bool isMessageValidated = true;
    err = HAPPDUVerifyMessage(requestBytes, numRequestBytes);
    if (err) {
        HAPAssert(err == kHAPError_InvalidData || err == kHAPError_InvalidState);

        HAPPDUStatus errStatus;
        if (err == kHAPError_InvalidState) {
            errStatus = kHAPPDUStatus_UnsupportedPDU;
        } else {
            errStatus = kHAPPDUStatus_InvalidRequest;
        }

        HAPLogError(&logObject, "Error during message verification: %d.", err);
        GENERATE_ERROR_RESPONSE_PDU((uint8_t*) responseBytes, numResponseBytes, 0, errStatus);
        isMessageValidated = false;
    }

    while (isMessageValidated && (totalBytesConsumed < numRequestBytes && err == kHAPError_None)) {
        size_t pduResponseBytes = 0;
        size_t pduBytesConsumed = 0;
        uint8_t* reqBuf = ((uint8_t*) requestBytes + totalBytesConsumed);
        uint8_t* respBuf = ((uint8_t*) responseBytes + *numResponseBytes);

        if (responseBytesRemaining < MIN_RESP_PDU_SIZE) {
            // There aren't enough bytes for a proper PDU response.  Return an out of resources error
            GENERATE_ERROR_RESPONSE_PDU(respBuf, numResponseBytes, reqBuf[2], kHAPPDUStatus_InsufficientResources);
            break;
        }

        // Handle request.
        bool requestSuccessful;
        err = HAPPDUProcedureHandleRequest(
                server,                               // Server
                session,                              // Session
                reqBuf,                               // requestBytes + totalBytesConsumed,    // Request Buffer
                numRequestBytes - totalBytesConsumed, // Size of request buffer remaining
                respBuf,                              // responseBytes + *numResponseBytes,    // Response buffer
                responseBytesRemaining,               // size of response Buffer
                &pduResponseBytes,                    // OUT - Bytes put in response
                &pduBytesConsumed,                    // OUT - PDU bytes consumed.
                &requestSuccessful);                  // OUT - True if request processed and not rejected
        if (err) {
            HAPPDUStatus errStatus = kHAPPDUStatus_InvalidRequest;
            switch (err) {
                case kHAPError_InvalidState: // Returned when provided opcode is not supported in this release
                    errStatus = kHAPPDUStatus_UnsupportedPDU;
                    break;
                case kHAPError_InvalidData: // returned when unable to parse PDU itself
                    errStatus = kHAPPDUStatus_InvalidRequest;
                    break;
                case kHAPError_OutOfResources: // shouldn't be returned as check above handles this, but kept for safety
                    errStatus = kHAPPDUStatus_InsufficientResources;
                    break;
                case kHAPError_NotAuthorized: // Session is dropped.
                    *responseCode = kHAPPlatformThreadCoAPManagerResponseCode_NotFound;
                    *numResponseBytes = 0;
                    return;
                case kHAPError_None:
                case kHAPError_Unknown:
                case kHAPError_Busy:
                default:
                    HAPLogError(&logObject, "Invalid PDU Procedure Error: %d.", err);
                    HAPFatalError();
            }
            GENERATE_ERROR_RESPONSE_PDU(respBuf, numResponseBytes, reqBuf[2], errStatus);
            break;
        }
        totalBytesConsumed += pduBytesConsumed;
        responseBytesRemaining -= pduResponseBytes;
        *numResponseBytes += pduResponseBytes;

        // Do not continue to process requests after an unsuccessful one
        if (!requestSuccessful) {
            if (totalBytesConsumed < numRequestBytes) {
                HAPLogDebug(
                        &logObject,
                        "Ignoring one or more additional messages after unsuccessful request [%zu consumed out of %zu "
                        "total bytes]",
                        totalBytesConsumed,
                        numRequestBytes);
            } else {
                HAPLogDebug(
                        &logObject,
                        "Unsuccessful request, no additional messages ignored [%zu consumed out of %zu total bytes]",
                        totalBytesConsumed,
                        numRequestBytes);
            }
            break;
        }
    }

    // Correct secure request received. Update the last received timestamp.
    threadSession->lastReceived = HAPPlatformClockGetCurrent();

    // Encrypt response.
    HAPLogBufferDebug(
            &logObject,
            responseBytes,
            *numResponseBytes,
            "[%p] Sending %s response.",
            (const void*) session,
            resourceDescription);
    HAPAssert(*numResponseBytes <= maxResponseBytes - CHACHA20_POLY1305_TAG_BYTES);
    err = HAPSessionEncryptControlMessage(server, session, responseBytes, responseBytes, *numResponseBytes);
    if (err) {
        HAPAssert(err == kHAPError_InvalidState);
        HAPLog(&logObject, "[%p] Failed to encrypt %s response.", (const void*) session, resourceDescription);
        *responseCode = kHAPPlatformThreadCoAPManagerResponseCode_NotFound;
        *numResponseBytes = 0;
        return;
    }
    *numResponseBytes += CHACHA20_POLY1305_TAG_BYTES;

    // Send response.
    *responseCode = kHAPPlatformThreadCoAPManagerResponseCode_Changed;

    if (shortResponse) {
        // For short response, generate PDU with insufficient resources
        shortResponse->numShortResponseBytes = 0;
        GENERATE_ERROR_RESPONSE_PDU(
                (uint8_t*) shortResponse->shortResponseBytes,
                &shortResponse->numShortResponseBytes,
                ((uint8_t*) requestBytes)[2],
                kHAPPDUStatus_InsufficientResources);

        // Revert nonce so that the same nonce is used for the short response which is a fallback for the response.
        session->hap.accessoryToController.controlChannel.nonce--;

        // Encrypt short response.
        HAPLogBufferDebug(
                &logObject,
                shortResponse->shortResponseBytes,
                shortResponse->numShortResponseBytes,
                "[%p] Short %s response in case the response fails for insufficient resources",
                (const void*) session,
                resourceDescription);
        HAPAssert(
                shortResponse->numShortResponseBytes <=
                shortResponse->maxShortResponseBytes - CHACHA20_POLY1305_TAG_BYTES);
        err = HAPSessionEncryptControlMessage(
                server,
                session,
                shortResponse->shortResponseBytes,
                shortResponse->shortResponseBytes,
                shortResponse->numShortResponseBytes);
        if (err) {
            HAPAssert(err == kHAPError_InvalidState);
            HAPLog(&logObject, "[%p] Failed to encrypt short %s response.", (const void*) session, resourceDescription);

            // Revert the nonce to the original state
            session->hap.accessoryToController.controlChannel.nonce++;
        } else {
            shortResponse->numShortResponseBytes += CHACHA20_POLY1305_TAG_BYTES;
            shortResponse->shortResponseCode = kHAPPlatformThreadCoAPManagerResponseCode_Changed;
            shortResponse->doTryShortResponse = true;
        }
    }
}

static void HandleSecureMessageRequest(
        HAPPlatformThreadCoAPManagerRef coapManager,
        const HAPPlatformThreadCoAPManagerPeer* peer,
        HAPPlatformThreadCoAPManagerResourceRef coapResource,
        HAPPlatformThreadCoAPManagerRequestCode requestCode,
        void* requestBytes,
        size_t numRequestBytes,
        HAPPlatformThreadCoAPManagerResponseCode* responseCode,
        void* responseBytes,
        size_t maxResponseBytes,
        size_t* numResponseBytes,
        HAPPlatformThreadCoAPManagerShortResponse* _Nullable shortResponse,
        void* _Nullable context) {
    HAPAccessoryServer* server = context;
    HAPPrecondition(coapResource == server->thread.coap.secureMessageResource);

    bool wasPaired = HAPAccessoryServerIsPaired(server);
    ProcessRequest(
            server,
            coapManager,
            peer,
            coapResource,
            "secure message",
            requestCode,
            requestBytes,
            numRequestBytes,
            responseCode,
            responseBytes,
            maxResponseBytes,
            numResponseBytes,
            shortResponse);
    bool isPaired = HAPAccessoryServerIsPaired(server);

    if (isPaired != wasPaired) {
        if (isPaired) {
            // Transition from unpaired to paired.  Remove pair setup wakelock.
            HAPPlatformThreadRemoveWakeLock(&kPairSetupWakeLock);
        } else {
            // Transition from paired to unpaired.  Set pair setup wakelock with timeout.
            HAPPlatformThreadAddWakeLock(&kPairSetupWakeLock, UNPAIR_WAKELOCK_TIME);
        }
        UpdateAdvertisingData(server, true);
    }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Handles Border Router state change
 *
 * @param borderRouterIsPresent  true if border router is present. false, otherwise.
 * @param context                accessory server
 */
static void HandleBorderRouterState(bool borderRouterIsPresent, void* context) {
    HAPPrecondition(context);
    HAPAccessoryServer* server = context;

    HAPLog(&logObject, borderRouterIsPresent ? "Thread border router is present" : "Thread border router is absent");

    if (!server->transports.thread) {
        // Server must have stopped.
        return;
    }

    if (!server->thread.isTransportRunning || server->thread.isTransportStopping) {
        // Transport is not running
        return;
    }

    HAPAssert(server->thread.storage);

    if (server->thread.storage->networkJoinState.inProgress) {
        // Hasn't joined the network yet.
        // The presence indication is probably caused by enabling the Thread stack.
        // Discard the result in such a case.
        return;
    }

    if (borderRouterIsPresent) {
        // Stop the border router detection timeout timer
        if (server->thread.storage->networkJoinState.borderRouterDetectTimer) {
            HAPPlatformTimerDeregister(server->thread.storage->networkJoinState.borderRouterDetectTimer);
            server->thread.storage->networkJoinState.borderRouterDetectTimer = 0;
            HAPPlatformThreadRemoveWakeLock(&server->thread.storage->networkJoinState.borderRouterDetectionWakeLock);
        }

        // Stop BLE transport unless user requested to keep BLE
        if (server->transports.ble && !server->ble.didUserRequestToKeepTransportOn) {
            // BLE transport must be stopped only when controller is not connected.
            HAPAccessoryServerStopBLETransportWhenDisconnected(server);
        }

        // Now that Thread network join was successful, reset the reattach attempt counter
        server->thread.storage->networkJoinState.reattachCount = 0;
    } else if (!server->thread.storage->networkJoinState.borderRouterDetectTimer) {
        // Note that it might take some time to detect border router presence
        // after joining Thread network and the border router might seem lost
        // when the border router is changing its configuration such as when it
        // removes srp-mdns-proxy service and adding it back.

        // Hence connectivity loss must be declared only after the border router
        // detection timeout.

        HAPThreadAccessoryServerStartBorderRouterDetectionTimer(server);
        HAPPlatformThreadAddWakeLock(
                &server->thread.storage->networkJoinState.borderRouterDetectionWakeLock,
                kHAPThreadAccessoryServerBorderRouterDetectionTimeout);
    }
}

/**
 * Indicates that the thread network connectivity has been lost.
 *  Allows Accessory Server to migrate to other transports if
 *  possible.
 *
 * @param _server Accessory Server
 */
static void HandleThreadNetworkLoss(void* _server) {
    HAPPrecondition(_server);
    HAPAccessoryServer* server = (HAPAccessoryServer*) _server;
    HAPThreadAccessoryServerHandleThreadConnectivityLoss(server);
}

static void Start(HAPAccessoryServer* server) {
    HAPPrecondition(server);
    HAPPlatformThreadCoAPManagerRef coapManager = HAPNonnull(server->platform.thread.coapManager);

    HAPError err;

    // TODO:  Thread requires reachability, but these are required so vanilla darwin does not build
    //        with reachability and break HCA.
    uint32_t sleepInterval = 0;

    if (server->primaryAccessory->reachabilityConfiguration) {
        sleepInterval = server->primaryAccessory->reachabilityConfiguration->sleepIntervalInMs;
    }

    // If we aren't sleepy, sleepInterval is 0.
    if ((server->thread.deviceParameters.deviceType & kHAPPlatformThreadDeviceCapabilities_SED) == 0) {
        sleepInterval = 0;
    }
    // Initialize Thread Platform
    HAPPlatformThreadInitialize(
            server,
            server->thread.deviceParameters.deviceType,
            sleepInterval,
            server->thread.deviceParameters.childTimeout,
            server->thread.deviceParameters.txPowerdbm);

    // Set CoAP delegate.
    HAPPlatformThreadCoAPManagerDelegate delegate;
    HAPRawBufferZero(&delegate, sizeof delegate);
    delegate.context = server;
    delegate.handleReadyToSendRequest = HandleReadyToSendRequest;
    HAPPlatformThreadCoAPManagerSetDelegate(coapManager, &delegate);

    // Start CoAP.
    HAPPlatformThreadCoAPManagerStart(coapManager);

    // Register request handlers.
    {
        HAPPrecondition(!server->thread.coap.identifyResource);
        err = HAPPlatformThreadCoAPManagerAddResource(
                coapManager,
                /* uriPath: */ "0",
                HandleIdentifyRequest,
                server,
                &server->thread.coap.identifyResource);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            HAPLogError(&logObject, "HAPPlatformThreadCoAPManagerAddResource failed: %d.", err);
            HAPFatalError();
        }

        HAPPrecondition(!server->thread.coap.pairSetupResource);
        err = HAPPlatformThreadCoAPManagerAddResource(
                coapManager,
                /* uriPath: */ "1",
                HandlePairSetupRequest,
                server,
                &server->thread.coap.pairSetupResource);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            HAPLogError(&logObject, "HAPPlatformThreadCoAPManagerAddResource failed: %d.", err);
            HAPFatalError();
        }

        HAPPrecondition(!server->thread.coap.pairVerifyResource);
        err = HAPPlatformThreadCoAPManagerAddResource(
                coapManager,
                /* uriPath: */ "2",
                HandlePairVerifyRequest,
                server,
                &server->thread.coap.pairVerifyResource);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            HAPLogError(&logObject, "HAPPlatformThreadCoAPManagerAddResource failed: %d.", err);
            HAPFatalError();
        }

        HAPPrecondition(!server->thread.coap.secureMessageResource);
        err = HAPPlatformThreadCoAPManagerAddResource(
                coapManager,
                /* uriPath: */ "",
                HandleSecureMessageRequest,
                server,
                &server->thread.coap.secureMessageResource);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            HAPLogError(&logObject, "HAPPlatformThreadCoAPManagerAddResource failed: %d.", err);
            HAPFatalError();
        }
    }

    // Note that service discovery is active by default
    // till the service discovery is separated out from lower layer where transport layer
    // could have more control over its state.
    server->thread.isServiceDiscoveryActive = true;

    server->thread.isTransportRunning = true;
    server->thread.isTransportStopping = false;
    server->thread.shouldStartTransport = false;

    // Register to get notified of border router state changes
    HAPPlatformThreadRegisterBorderRouterStateCallback(HandleBorderRouterState, server);
    HAPPlatformThreadRegisterNetworkLossCallback(HandleThreadNetworkLoss);

    // Kick off joining Thread network with the configured parameters
    if (!HAPAccessoryServerIsPaired(server)) {
        if (!HAPPlatformThreadIsCommissioned()) {
            // Transitioning from unpaired and Uncommissioned to unpaired and commissioned.
            // Wakelock without timeout until paired.
            HAPPlatformThreadAddWakeLock(&kPairSetupWakeLock, 0);
        } else {
            // Booting in unpaired and commissioned.  Wakelock for 5m to facilitate pairing.
            HAPPlatformThreadAddWakeLock(&kPairSetupWakeLock, REBOOT_WAKELOCK_TIME);
        }
    }
    HAPThreadAccessoryServerJoinNetwork(server);
    HAPPlatformThreadAddWakeLock(&kThreadStartWakeLock, START_WAKELOCK_TIME);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Thread reattachment timer callback function
 *
 * @param timer   timer
 * @param context accessory server
 */
static void HandleReattachTimeout(HAPPlatformTimerRef timer HAP_UNUSED, void* _Nullable context) {
    HAPPrecondition(context);
    HAPAccessoryServer* server = context;

    if (!server->transports.thread) {
        // Transport was shutdown. Do not proceed.
        HAPLogInfo(&logObject, "Reattach timeout ignored due to server shutdown");
        return;
    }

    if (server->thread.isTransportStopping) {
        // Already running and now stopping. Do not proceed.
        HAPLogInfo(&logObject, "Reattach timeout ignored for Thread transport is stopping");
        return;
    }

    HAPAssert(server->thread.storage);

    server->thread.storage->networkJoinState.reattachTimer = 0;

    // Note that by race condition reattach timeout handler can be triggered
    // after cancelling reattach. Hence, check the reattach is required.
    if (server->thread.storage->networkJoinState.shouldReattach) {
        // Start Thread transport
        HAPLogInfo(&logObject, "Thread reattach timeout: reattaching");
        server->thread.storage->networkJoinState.reattachCount++;
        HAPAccessoryServerStartThreadTransport(server);
    } else {
        HAPLogDebug(&logObject, "Thread reattach timeout: reattach stopped");
    }
}

/**
 * Handles reattempts to stop Thread
 *
 * @param timer    timer
 * @param context  accessory server
 */
static void HandleStopRetryTimer(HAPPlatformTimerRef timer HAP_UNUSED, void* _Nullable context) {
    HAPPrecondition(context);
    HAPAccessoryServer* server = context;

    bool didStop;
    if (!server->transports.thread) {
        // Transport is no longer valid.
        return;
    }

    HAPNonnull(server->transports.thread)->tryStop(context, &didStop);
}

static void TryStop(HAPAccessoryServer* server, bool* didStop) {
    HAPPrecondition(server);
    HAPPlatformThreadCoAPManagerRef coapManager = HAPNonnull(server->platform.thread.coapManager);
    HAPPlatformServiceDiscoveryRef serviceDiscovery = HAPNonnull(server->platform.thread.serviceDiscovery);
    HAPPrecondition(didStop);

    if (!server->thread.isTransportRunning) {
        // Already stopped
        *didStop = true;
        return;
    }

    server->thread.isTransportStopping = true;

    // Stop joiner timer if it is still running
    if (server->thread.storage->networkJoinState.joinerTimer) {
        HAPPlatformTimerDeregister(server->thread.storage->networkJoinState.joinerTimer);
        server->thread.storage->networkJoinState.joinerTimer = 0;
        server->thread.storage->networkJoinState.inProgress = false;
    }
    // Stop border router detect timer if running
    if (server->thread.storage->networkJoinState.borderRouterDetectTimer) {
        HAPPlatformTimerDeregister(server->thread.storage->networkJoinState.borderRouterDetectTimer);
        server->thread.storage->networkJoinState.borderRouterDetectTimer = 0;
        HAPPlatformThreadRemoveWakeLock(&server->thread.storage->networkJoinState.borderRouterDetectionWakeLock);
    }

    // Terminate all sessions.
    *didStop = true;
    for (size_t i = 0; i < server->thread.storage->numSessions; i++) {
        HAPThreadSession* threadSession = &server->thread.storage->sessions[i];

        if (threadSession->lastActivity) {
            if (threadSession->isSendingEvent && HAPPlatformThreadNetworkIsViable()) {
                HAPLogBuffer(
                        &logObject,
                        &threadSession->peer,
                        sizeof threadSession->peer,
                        "Delaying stop to finish sending event notification.");
                threadSession->pendingDestroy = true;
                *didStop = false;
            } else {
                threadSession->isSendingEvent = false;
                TerminateThreadSession(server, threadSession);
            }
        }
    }
    // If unabortable joining progress, such as reachability test, is in progress,
    // transport must not stop till it's over.
    if (server->thread.storage->networkJoinState.inProgress) {
        *didStop = false;
    }
    if (!*didStop) {
        HAPPlatformTimerRef timer;
        HAPError err = HAPPlatformTimerRegister(
                &timer,
                HAPPlatformClockGetCurrent() + kHAPThreadAccessoryServer_StopRetryTime,
                HandleStopRetryTimer,
                server);
        HAPAssert(!err);
        return;
    }

    // Handle Platform-dependent deinitialization
    HAPPlatformThreadDeinitialize(server);

    // Stop service discovery.
    if (server->thread.isServiceDiscoveryActive) {
        HAPPlatformServiceDiscoveryStop(serviceDiscovery);
        server->thread.isServiceDiscoveryActive = false;

        server->thread.discoverableService = kHAPIPServiceDiscoveryType_None;
    }

    // Clear any pending wakelocks
    HAPPlatformThreadPurgeWakeLocks();

    // Deregister border router state callback
    HAPPlatformThreadDeregisterBorderRouterStateCallback(HandleBorderRouterState);
    HAPPlatformThreadDeregisterNetworkLossCallback(HandleThreadNetworkLoss);

    // Stop CoAP manager.
    bool coapWasActive = false;
    if (server->thread.coap.identifyResource) {
        HAPPlatformThreadCoAPManagerRemoveResource(coapManager, server->thread.coap.identifyResource);
        server->thread.coap.identifyResource = 0;
        coapWasActive = true;
    }
    if (server->thread.coap.pairSetupResource) {
        HAPPlatformThreadCoAPManagerRemoveResource(coapManager, server->thread.coap.pairSetupResource);
        server->thread.coap.pairSetupResource = 0;
        coapWasActive = true;
    }
    if (server->thread.coap.pairVerifyResource) {
        HAPPlatformThreadCoAPManagerRemoveResource(coapManager, server->thread.coap.pairVerifyResource);
        server->thread.coap.pairVerifyResource = 0;
        coapWasActive = true;
    }
    if (server->thread.coap.secureMessageResource) {
        HAPPlatformThreadCoAPManagerRemoveResource(coapManager, server->thread.coap.secureMessageResource);
        server->thread.coap.secureMessageResource = 0;
        coapWasActive = true;
    }
    if (coapWasActive) {
        HAPPlatformThreadCoAPManagerStop(coapManager);
    }

    HAPPlatformThreadCoAPManagerSetDelegate(coapManager, NULL);

    server->thread.isTransportRunning = false;
    server->thread.isTransportStopping = false;

    if (server->state == kHAPAccessoryServerState_Running) {
        if (server->thread.shouldStartTransport) {
            // Transport has to be restarted
            HAPLogInfo(&logObject, "Thread transport restart was pending");
            HAPAccessoryServerStartThreadTransport(server);
        } else {
            if (server->thread.storage->networkJoinState.shouldReattach) {
                // Kicks off reattach timer
                HAPPrecondition(server->thread.storage->getNextReattachDelay);
                HAPTime reattachDelay = server->thread.storage->getNextReattachDelay(
                        server->thread.storage->getNextReattachDelayContext,
                        server->thread.storage->networkJoinState.reattachCount);
                HAPLogDebug(
                        &logObject,
                        "Reattach is scheduled %lu seconds later",
                        (unsigned long) (reattachDelay / HAPSecond));
                HAPError err = HAPPlatformTimerRegister(
                        &server->thread.storage->networkJoinState.reattachTimer,
                        HAPPlatformClockGetCurrent() + reattachDelay,
                        HandleReattachTimeout,
                        server);
                HAPAssert(!err);
            }

            if (server->transports.ble && server->ble.shouldStartTransport) {
                HAPLogInfo(&logObject, "Thread transport stopped. Switching to BLE.");
                HAPAccessoryServerStartBLETransport(server);
            } else {
                HAPLogInfo(&logObject, "Thread transport stopped.");
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void UpdateAdvertisingData(HAPAccessoryServer* server, bool updateTxtRecord) {
    HAPPrecondition(server);

    if (server->state == kHAPAccessoryServerState_Running) {
        HAPError err = HAPPlatformThreadRefreshAdvertisement(server, updateTxtRecord);
        HAPAssert(!err);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void WillInvalidate(HAPAccessoryServer* server, HAPSession* session) {
    HAPPrecondition(server);
    HAPPrecondition(session);

    // Transient sessions cannot have notifications.
    if (!session->hap.isTransient) {
        HAPNotificationDeregisterAll(server, session);
    }
    // Invalidate attached HomeKit Data Streams.
    HAPDataStreamInvalidateAllForHAPSession(server, session);
    // Clear session storage.
    HAPThreadSessionStorageClearData(server, session, kHAPSessionStorage_DataBuffer_TimedWrite);
    HAPThreadSessionStorageClearData(server, session, kHAPSessionStorage_DataBuffer_Notification);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void HandleSendEventComplete(
        HAPPlatformThreadCoAPManagerRef coapManager,
        HAPError error HAP_UNUSED,
        const HAPPlatformThreadCoAPManagerPeer* _Nullable peer,
        HAPPlatformThreadCoAPManagerResponseCode responseCode HAP_UNUSED,
        void* _Nullable responseBytes HAP_UNUSED,
        size_t numResponseBytes HAP_UNUSED,
        void* _Nullable context) {
    HAPPrecondition(coapManager);
    HAPPrecondition(context);
    HAPAccessoryServer* server = HAPNonnullVoid(context);
    HAPPrecondition(peer);

    HAPThreadSession* _Nullable threadSession = ResumeThreadSessionForPeer(server, peer, false);

    if (threadSession) {
        if (!threadSession->isSendingEvent) {
            // If this function is called a little bit later than the session is released,
            // the resumed session might have been created just now, in which case, ignore the callback
            // with respect to the session itself.
            // Session could have been released for various reasons such as reachability timeout
            // or control channel message decryption failure, etc.
            HAPLogBufferDebug(&logObject, peer, sizeof *peer, "Ignored event notification completion.");
        } else {
            threadSession->isSendingEvent = false;
            HAPLogBufferDebug(
                    &logObject,
                    &threadSession->peer,
                    sizeof threadSession->peer,
                    "Finished sending event notification.");

            if (threadSession->pendingDestroy || error != kHAPError_None) {
                TerminateThreadSession(server, threadSession);
            }
        }
    }

    HAPNotificationContinueSending(server);
}

HAP_RESULT_USE_CHECK
static HAPError SendEvent(HAPAccessoryServer* server, HAPSession* session, const void* bytes, size_t numBytes) {
    HAPPrecondition(server);
    HAPPlatformThreadCoAPManagerRef coapManager = HAPNonnull(server->platform.thread.coapManager);
    HAPPrecondition(session);
    HAPPrecondition(session->transportType == kHAPTransportType_Thread);
    HAPThreadSession* threadSession = HAPAccessoryServerGetThreadSessionWithHAPSession(server, session);

    HAPPrecondition(bytes);

    HAPError err;

    if (threadSession->isSendingEvent) {
        HAPLogBuffer(
                &logObject, &threadSession->peer, sizeof threadSession->peer, "Already sending event notification.");
        return kHAPError_Busy;
    }

    threadSession->isSendingEvent = true;
    err = HAPPlatformThreadCoAPManagerSendRequest(
            coapManager,
            &threadSession->peer,
            /* uriPath: */ "",
            kHAPPlatformThreadCoAPManagerRequestCode_PUT,
            bytes,
            numBytes,
            HandleSendEventComplete,
            server);
    if (err) {
        HAPAssert(err == kHAPError_Busy || err == kHAPError_OutOfResources);
        HAPLogBufferError(
                &logObject,
                &threadSession->peer,
                sizeof threadSession->peer,
                "%s failed: %u.",
                "HAPPlatformThreadCoAPManagerSendRequest",
                err);
        threadSession->isSendingEvent = false;
        return err;
    }

    return kHAPError_None;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const HAPThreadAccessoryServerTransport kHAPAccessoryServerTransport_Thread = {
    .create = Create,
    .prepareStart = PrepareStart,
    .start = Start,
    .tryStop = TryStop,
    .updateAdvertisingData = UpdateAdvertisingData,
    .session = { .willInvalidate = WillInvalidate, .sendEvent = SendEvent }
};

#endif
