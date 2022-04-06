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

#include "HAPAccessoryServer+Internal.h"
#include "HAPDataStream+Internal.h"
#include "HAPDataStream+TCP.h"
#include "HAPDataStream.h"
#include "HAPDataStreamRef.h"
#include "HAPLogSubsystem.h"
#include "HAPServiceTypes.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "DataStream" };

static void ClearSendingContext(HAPAccessoryServer* server, HAPDataStreamRef* dataStream_);

/**
 * Returns the HomeKit Data Stream for which setup has been prepared.
 *
 * @param      server              Accessory server.
 *
 * @return HomeKit Data Stream      If successful.
 * @return NULL                     If HomeKit Data Stream setup has not been prepared or has timed out.
 */
static HAPDataStreamTCP* _Nullable HAPDataStreamSetupGetDataStream(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    for (size_t i = 0; i < server->dataStream.numDataStreams; i++) {
        HAPDataStreamRef* dataStream_ = &server->dataStream.dataStreams[i];
        HAPDataStreamTCP* dataStream = &dataStream_->tcp;

        if (!dataStream->isMatchmakingComplete && dataStream->session && !dataStream->setupRequested) {
            return dataStream;
        }
    }

    return NULL;
}

/** Reset a Data Stream that has begun setup but not completed it. */
static void ResetDataStreamInSetup(HAPDataStreamTCP* _Nonnull dataStream) {
    HAPPrecondition(dataStream);

    HAPRawBufferZero(&dataStream->accessoryToController._, sizeof dataStream->accessoryToController._);
    HAPRawBufferZero(&dataStream->controllerToAccessory._, sizeof dataStream->controllerToAccessory._);
    dataStream->session = NULL;
    dataStream->serviceTypeIndex = 0;
    dataStream->setupTTL = 0;
    dataStream->setupRequested = false;
}

#define GetTransportState(server) (&((server)->dataStream.transportState.tcp))

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Updates shared HomeKit Data Stream state.
 *
 * - This checks for expired connect timeouts and updates the reference time for the TTL fields.
 * - This starts / stops listening for incoming connections depending on presence of unmatched HomeKit Data Streams.
 *
 * @param      server               Accessory server.
 */
static void UpdateMatchmakingState(HAPAccessoryServer* server);

static void HandlePendingTCPStream(HAPPlatformTCPStreamManagerRef tcpStreamManager, void* _Nullable context);

static void HandleMatchmakingTimerExpired(HAPPlatformTimerRef timer, void* _Nullable context) {
    HAPPrecondition(context);
    HAPAccessoryServer* const server = context;
    HAPPrecondition(timer == GetTransportState(server)->matchmakingTimer);
    HAPLogDebug(&logObject, "HomeKit Data Stream matchmaking timer expired.");
    GetTransportState(server)->matchmakingTimer = 0;

    UpdateMatchmakingState(server);
}

static void UpdateMatchmakingState(HAPAccessoryServer* server) {
    HAPPrecondition(server);
    HAPDataStreamTCPTransportState* const transportState = GetTransportState(server);

    HAPError err;

    // Abort ongoing timer.
    if (transportState->matchmakingTimer) {
        HAPPlatformTimerDeregister(transportState->matchmakingTimer);
        transportState->matchmakingTimer = 0;
    }

    // Advance time.
    HAPTime now = HAPPlatformClockGetCurrent();
    HAPTime delta = now - transportState->referenceTime;
    transportState->referenceTime = now;
    if (delta >= 1 * HAPMillisecond) {
        HAPLogDebug(
                &logObject,
                "Advancing HomeKit Data Stream reference time by %llu.%03llus.",
                (unsigned long long) (delta / HAPSecond),
                (unsigned long long) (delta % HAPSecond));
    }

    // Since the delegate is only informed about HomeKit Data Streams that went through matchmaking successfully,
    // it is not necessary to worry about calling delegate functions in here.

    HAPTime minTTL = HAPTime_Max;

    // Enumerate unmatched setup requests.
    bool needsListener = false;
    for (size_t i = 0; i < server->dataStream.numDataStreams; i++) {
        HAPDataStreamRef* const dataStream_ = &server->dataStream.dataStreams[i];
        HAPDataStreamTCP* const dataStream = &dataStream_->tcp;

        // If already matched, matchmaking timeouts no longer apply.
        if (dataStream->isMatchmakingComplete) {
            HAPAssert(!dataStream->setupTTL);
            continue;
        }

        // Enforce timeouts for unmatched setup requests.
        if (!dataStream->session) {
            continue;
        }

        // Check if setup time expired; if so, kill the stream.
        if (dataStream->setupTTL <= delta) {
            HAPLog(&logObject,
                   "[%p] Aborting pending HomeKit Data Stream setup request (%s).",
                   (const void*) dataStream_,
                   dataStream->setupRequested ? "no matching connection" : "setup not completed");
            if (!dataStream->setupRequested) {
                HAPAssert(dataStream == HAPDataStreamSetupGetDataStream(server));
            }
            ResetDataStreamInSetup(dataStream);
            continue;
        }

        // Update remaining time.
        HAPAssert(delta <= UINT16_MAX);
        dataStream->setupTTL -= (uint16_t) delta;
        minTTL = HAPMin(dataStream->setupTTL, minTTL);
        needsListener = true;
    }

    // Enumerate unmatched TCP streams.
    for (size_t i = 0; i < server->dataStream.numDataStreams; i++) {
        HAPDataStreamRef* dataStream_ = &server->dataStream.dataStreams[i];
        HAPDataStreamTCP* dataStream = &dataStream_->tcp;

        // If already matched, matchmaking timeouts no longer apply.
        if (dataStream->isMatchmakingComplete) {
            HAPAssert(!dataStream->tcpStreamTTL);
            continue;
        }

        // Enforce timeouts for unmatched TCP streams.
        if (!dataStream->tcpStreamIsConnected) {
            continue;
        }

        // Check if setup time expired; if so, kill the stream.
        if (!needsListener || dataStream->tcpStreamTTL <= delta) {
            HAPLog(&logObject,
                   "[%p] Aborting pending TCP stream (%s).",
                   (const void*) dataStream_,
                   needsListener ? "did not receive frame" : "no active setup requests");
            HAPPlatformTCPStreamClose(HAPNonnull(transportState->tcpStreamManager), dataStream->tcpStream);
            HAPRawBufferZero(&dataStream->accessoryToController.tx, sizeof dataStream->accessoryToController.tx);
            HAPRawBufferZero(&dataStream->accessoryToController.nonce, sizeof dataStream->accessoryToController.nonce);
            HAPRawBufferZero(&dataStream->controllerToAccessory.tx, sizeof dataStream->controllerToAccessory.tx);
            HAPRawBufferZero(&dataStream->controllerToAccessory.nonce, sizeof dataStream->controllerToAccessory.nonce);
            dataStream->tcpStream = 0;
            dataStream->tcpStreamTTL = 0;
            dataStream->tcpStreamIsConnected = false;
            dataStream->streamPriority = kHAPStreamPriority_NoPriority;
            continue;
        }

        // Update remaining time.
        HAPAssert(delta <= UINT16_MAX);
        dataStream->tcpStreamTTL -= (uint16_t) delta;
        minTTL = HAPMin(dataStream->tcpStreamTTL, minTTL);
    }

    // Update TCP stream manager status.
    bool hasListener = HAPPlatformTCPStreamManagerIsListenerOpen(HAPNonnull(transportState->tcpStreamManager));
    if (needsListener && !hasListener) {
        HAPPlatformTCPStreamManagerOpenListener(
                HAPNonnull(transportState->tcpStreamManager), HandlePendingTCPStream, server);
        HAPLog(&logObject,
               "Started to listen for HomeKit Data Stream connections (port %u).",
               HAPPlatformTCPStreamManagerGetListenerPort(HAPNonnull(transportState->tcpStreamManager)));
    } else if (!needsListener && hasListener) {
        HAPLog(&logObject,
               "Stopping to listen for HomeKit Data Stream connections (port %u).",
               HAPPlatformTCPStreamManagerGetListenerPort(HAPNonnull(transportState->tcpStreamManager)));
        HAPPlatformTCPStreamManagerCloseListener(HAPNonnull(transportState->tcpStreamManager));
    } else {
        HAPAssert((needsListener && hasListener) || (!needsListener && !hasListener));
    }

    // Schedule timer.
    if (minTTL != HAPTime_Max) {
        // NOLINTNEXTLINE(bugprone-branch-clone)
        HAPAssert(minTTL <= HAPMax(kHAPDataStream_SetupTimeout, kHAPDataStream_TCPStreamTimeout));
        err = HAPPlatformTimerRegister(
                &GetTransportState(server)->matchmakingTimer, now + minTTL, HandleMatchmakingTimerExpired, server);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            HAPLogError(&logObject, "Not enough timers available (HomeKit Data Stream matchmaking timer).");
            HAPFatalError();
        }
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HAP_RESULT_USE_CHECK
HAPError HAPDataStreamTCPSetupBegin(
        HAPAccessoryServer* server,
        const HAPService* service,
        const HAPAccessory* accessory,
        HAPSession* session,
        const HAPDataStreamControllerSetupParams* setupParams) {
    HAPPrecondition(server);
    HAPPrecondition(service);
    HAPPrecondition(accessory);
    HAPPrecondition(HAPSessionIsSecured(session) && !HAPSessionIsTransient(session));
    HAPPrecondition(setupParams);

    // An encrypted HDS requires a key-salt from the controller.
    if (!setupParams->controllerKeySalt) {
        HAPLogError(&logObject, "Setup failed: Controller Key Salt missing.");
        return kHAPError_InvalidData;
    }

    // Update reference time for timeouts.
    UpdateMatchmakingState(server);

    // Only one HomeKit Data Stream may be set up at a time.
    HAPPrecondition(!HAPDataStreamSetupGetDataStream(server));

    // Find memory to register setup request.
    for (size_t i = 0; i < server->dataStream.numDataStreams; i++) {
        HAPDataStreamRef* dataStream_ = &server->dataStream.dataStreams[i];
        HAPDataStreamTCP* dataStream = &dataStream_->tcp;

        if (dataStream->session) {
            continue;
        }

        HAPAssert(!dataStream->isMatchmakingComplete);
        HAPAssert(HAPRawBufferIsZero(&dataStream->accessoryToController._, sizeof dataStream->accessoryToController._));
        HAPAssert(HAPRawBufferIsZero(&dataStream->controllerToAccessory._, sizeof dataStream->controllerToAccessory._));
        HAPAssert(!dataStream->session);
        HAPAssert(!dataStream->serviceTypeIndex);
        HAPAssert(!dataStream->setupTTL);
        HAPAssert(!dataStream->setupRequested);

        // Register TTL.
        dataStream->setupTTL = kHAPDataStream_SetupTimeout;

        // Register setup request.
        dataStream->session = session;
        dataStream->serviceTypeIndex = HAPAccessoryServerGetServiceTypeIndex(server, service, accessory);

        // Register Controller Key Salt.
        HAPAssert(sizeof dataStream->controllerToAccessory._.salt == sizeof *setupParams->controllerKeySalt);
        HAPRawBufferCopyBytes(
                &dataStream->controllerToAccessory._.salt,
                setupParams->controllerKeySalt,
                sizeof dataStream->controllerToAccessory._.salt);
        HAPLogSensitiveBufferDebug(
                &logObject,
                dataStream->controllerToAccessory._.salt.bytes,
                sizeof dataStream->controllerToAccessory._.salt.bytes,
                "[%p] Controller Key Salt.",
                (const void*) dataStream_);

        // Generate Accessory Key Salt.
        HAPPlatformRandomNumberFill(
                dataStream->accessoryToController._.salt.bytes, sizeof dataStream->accessoryToController._.salt.bytes);
        HAPLogSensitiveBufferDebug(
                &logObject,
                dataStream->accessoryToController._.salt.bytes,
                sizeof dataStream->accessoryToController._.salt.bytes,
                "[%p] Accessory Key Salt.",
                (const void*) dataStream_);

        // Start enforcing TTL and ensure TCP stream listener is open.
        HAPLogInfo(&logObject, "[%p] HomeKit Data Stream setup request allocated.", (const void*) dataStream_);
        UpdateMatchmakingState(server);
        if (dataStream->session) {
            HAPAssert(
                    HAPPlatformTCPStreamManagerIsListenerOpen(HAPNonnull(GetTransportState(server)->tcpStreamManager)));
        }
        return kHAPError_None;
    }

    HAPLogError(
            &logObject,
            "No space to allocate HomeKit Data Stream setup request (%zu max). Adjust HAPAccessoryServerOptions.",
            server->dataStream.numDataStreams);
    return kHAPError_OutOfResources;
}

void HAPDataStreamTCPSetupCancel(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    HAPDataStreamTCP* _Nullable dataStream = HAPDataStreamSetupGetDataStream(server);
    if (!dataStream) {
        return;
    }

    HAPLog(&logObject,
           "[%p] Canceling pending HomeKit Data Stream setup request (setup not completed).",
           (const void*) dataStream);

    ResetDataStreamInSetup(dataStream);

    UpdateMatchmakingState(server);
}

static void FillInAccessorySetupParamsForTCP(
        HAPAccessoryServer* server,
        HAPDataStreamTCP* dataStream,
        HAPDataStreamAccessorySetupParams* setupParams) {
    HAPDataStreamTCPTransportState* const transportState = GetTransportState(server);

    // Pick the listener port.
    HAPAssert(HAPPlatformTCPStreamManagerIsListenerOpen(HAPNonnull(transportState->tcpStreamManager)));
    setupParams->listenerPort =
            HAPPlatformTCPStreamManagerGetListenerPort(HAPNonnull(transportState->tcpStreamManager));
    setupParams->hasListenerPort = true;

    // The TCP port range for HDS must be a in a certain range.
    if (setupParams->listenerPort < kHAPDataStream_TCPMinimumPort) {
        HAPLogError(
                &logObject,
                "The TCP port range for HDS must be >= %u, but the port number is %u.",
                kHAPDataStream_TCPMinimumPort,
                setupParams->listenerPort);
        HAPFatalError();
    }

    // Fill out the salt.
    HAPAssert(sizeof setupParams->accessoryKeySalt == sizeof dataStream->accessoryToController._.salt);
    HAPRawBufferCopyBytes(
            &setupParams->accessoryKeySalt,
            &dataStream->accessoryToController._.salt,
            sizeof setupParams->accessoryKeySalt);
    setupParams->hasAccessoryKeySalt = true;
}

HAP_RESULT_USE_CHECK
HAPError HAPDataStreamTCPSetupComplete(HAPAccessoryServer* server, HAPDataStreamAccessorySetupParams* setupParams) {
    HAPPrecondition(server);
    HAPPrecondition(setupParams);

    // Reset them all to zero just in case.
    HAPRawBufferZero(setupParams, sizeof *setupParams);

    HAPDataStreamTCP* _Nullable dataStream = HAPDataStreamSetupGetDataStream(server);
    if (!dataStream) {
        HAPLog(&logObject, "HomeKit Data Stream setup has not been prepared or has timed out.");
        return kHAPError_InvalidState;
    }

    FillInAccessorySetupParamsForTCP(server, dataStream, setupParams);

    HAPLogInfo(&logObject, "[%p] HomeKit Data Stream setup request registered.", (const void*) dataStream);

    // Complete setup.
    dataStream->setupRequested = true;

    // Derive encryption keys.
    // See HomeKit Accessory Protocol Specification R17
    // Section 9.1.1 Data Stream Transport Security
    {
        HAPSession* session = dataStream->session;

        const HAPDataStreamSalt* controllerKeySalt = &dataStream->controllerToAccessory._.salt;
        const HAPDataStreamSalt* accessoryKeySalt = &dataStream->accessoryToController._.salt;

        // Derive salt.
        uint8_t salt[2 * sizeof(HAPDataStreamSalt)];
        HAPRawBufferCopyBytes(&salt[0], controllerKeySalt->bytes, sizeof controllerKeySalt->bytes);
        HAPRawBufferCopyBytes(
                &salt[sizeof(HAPDataStreamSalt)], accessoryKeySalt->bytes, sizeof accessoryKeySalt->bytes);
        HAPLogSensitiveBufferDebug(
                &logObject,
                session->hap.cv_KEY,
                sizeof session->hap.cv_KEY,
                "[%p] Current HAP session Shared Secret.",
                (const void*) dataStream);
        HAPLogSensitiveBufferDebug(&logObject, salt, sizeof salt, "[%p] Salt.", (const void*) dataStream);
        HAPRawBufferZero(&dataStream->accessoryToController, sizeof dataStream->accessoryToController);
        HAPRawBufferZero(&dataStream->controllerToAccessory, sizeof dataStream->controllerToAccessory);

        // Accessory to controller encryption key.
        HAP_hkdf_sha512(
                dataStream->accessoryToController._.key.bytes,
                sizeof dataStream->accessoryToController._.key.bytes,
                session->hap.cv_KEY,
                sizeof session->hap.cv_KEY,
                salt,
                sizeof salt,
                (const uint8_t*) kHAPDataStream_AccessoryToControllerKeyInfo,
                sizeof kHAPDataStream_AccessoryToControllerKeyInfo - 1);
        HAPLogSensitiveBufferDebug(
                &logObject,
                dataStream->accessoryToController._.key.bytes,
                sizeof dataStream->accessoryToController._.key.bytes,
                "[%p] AccessoryToControllerBulkTransferEncryptionKey.",
                (const void*) dataStream);

        // Controller to accessory encryption key.
        HAP_hkdf_sha512(
                dataStream->controllerToAccessory._.key.bytes,
                sizeof dataStream->controllerToAccessory._.key.bytes,
                session->hap.cv_KEY,
                sizeof session->hap.cv_KEY,
                salt,
                sizeof salt,
                (const uint8_t*) kHAPDataStream_ControllerToAccessoryKeyInfo,
                sizeof kHAPDataStream_ControllerToAccessoryKeyInfo - 1);
        HAPLogSensitiveBufferDebug(
                &logObject,
                dataStream->controllerToAccessory._.key.bytes,
                sizeof dataStream->controllerToAccessory._.key.bytes,
                "[%p] ControllerToAccessoryBulkTransferEncryptionKey.",
                (const void*) dataStream);

        // Erase sensitive information.
        HAPRawBufferZero(salt, sizeof salt);
    }

    return kHAPError_None;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void HAPDataStreamTCPGetRequestContext(
        HAPAccessoryServer* server,
        HAPDataStreamRef* dataStream_,
        HAPDataStreamRequest* request) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream_);
    HAPDataStreamTCP* dataStream = &dataStream_->tcp;
    HAPPrecondition(dataStream->isMatchmakingComplete);
    HAPPrecondition(request);

    HAPRawBufferZero(request, sizeof *request);

    request->transportType = kHAPTransportType_IP;
    request->session = HAPNonnull(dataStream->session);
    HAPAccessoryServerGetServiceFromServiceTypeIndex(
            server,
            &kHAPServiceType_DataStreamTransportManagement,
            dataStream->serviceTypeIndex,
            &request->service,
            &request->accessory);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void HAPDataStreamTCPUpdateInterests(HAPAccessoryServer* server, HAPDataStreamRef* dataStream_);

/**
 * Prepares invalidation of a HomeKit Data Stream.
 *
 * @param      server               Accessory server.
 * @param      dataStream_          HomeKit Data Stream.
 */
static void SetPendingDestroy(HAPAccessoryServer* server, HAPDataStreamRef* dataStream_) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream_);
    HAPDataStreamTCP* dataStream = &dataStream_->tcp;
    HAPPrecondition(dataStream->tcpStreamIsConnected);

    dataStream->pendingDestroy = true;
    HAPDataStreamTCPUpdateInterests(server, dataStream_);
}

static void BeginReceiving(HAPAccessoryServer* server, HAPDataStreamRef* dataStream_) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream_);
    HAPDataStreamTCP* dataStream = &dataStream_->tcp;
    HAPPrecondition(dataStream->tcpStreamIsConnected);

    HAPDataStreamTransmission* tx = &dataStream->controllerToAccessory.tx;
    HAPAssert(tx->state == kHAPDataStreamTransmissionState_Idle);
    tx->state = kHAPDataStreamTransmissionState_PrepareFrame;
    HAPDataStreamTCPUpdateInterests(server, dataStream_);
}

static void NotifyDelegateOfCompletion(
        HAPAccessoryServer* _Nonnull server,
        HAPDataStreamRef* _Nonnull dataStream_,
        HAPError error,
        HAPDataStreamTCPUnidirectionalState* _Nonnull state,
        bool isComplete) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream_);
    HAPPrecondition(state);

    HAPDataStreamTransmission* const tx = &state->tx;

    HAPDataStreamDataCompletionHandler completionHandler = tx->completionHandler;
    void* _Nullable dataBytes = tx->data.mutableBytes;
    size_t numDataBytes = (size_t) tx->numDataBytes;

    if (isComplete) {
        // Reset transmission for cases when callback reenters.
        HAPRawBufferZero(tx, sizeof *tx);
    } else {
        // Prepare transmission for next chunk for cases when callback reenters.
        tx->completionHandler = NULL;
        tx->dataIsMutable = false;
        tx->data.mutableBytes = NULL;
        tx->numDataBytes = 0;
    }

    // Inform delegate.
    HAPDataStreamRequest request;
    HAPDataStreamGetRequestContext(server, dataStream_, &request);
    HAPAssert(completionHandler);
    completionHandler(server, &request, dataStream_, error, dataBytes, numDataBytes, isComplete, server->context);
}

/**
 * Reports completion of an accessory to controller transmission.
 *
 * @param      server              Accessory server.
 * @param      dataStream_          Data stream.
 * @param      error                kHAPError_None           If successful.
 *                                  kHAPError_InvalidState   If the HomeKit Data Stream is being invalidated.
 */
static void CompleteAccessoryToControllerTransmission(
        HAPAccessoryServer* server,
        HAPDataStreamRef* dataStream_,
        HAPError error) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream_);
    HAPDataStreamTCP* const dataStream = &dataStream_->tcp;
    HAPPrecondition(dataStream->isMatchmakingComplete);

    HAPDataStreamTCPUnidirectionalState* const state = &dataStream->accessoryToController;

    // Increment nonce.
    state->nonce++;

    NotifyDelegateOfCompletion(server, dataStream_, error, state, /* isComplete */ true);
}

/**
 * Reports completion of a controller to accessory transmission.
 *
 * @param      server              Accessory server.
 * @param      dataStream_          Data stream.
 * @param      error                kHAPError_None           If successful.
 *                                  kHAPError_InvalidState   If the HomeKit Data Stream is being invalidated.
 */
static void CompleteControllerToAccessoryTransmission(
        HAPAccessoryServer* server,
        HAPDataStreamRef* dataStream_,
        HAPError error) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream_);
    HAPDataStreamTCP* const dataStream = &dataStream_->tcp;
    HAPPrecondition(dataStream->isMatchmakingComplete);

    HAPLogDebug(&logObject, "[%p] Accessory <- Controller transmission complete.", (const void*) dataStream_);

    HAPDataStreamTCPUnidirectionalState* state = &dataStream->controllerToAccessory;

    // Increment nonce.
    state->nonce++;

    NotifyDelegateOfCompletion(server, dataStream_, error, state, /* isComplete */ true);

    // Begin receiving next packet.
    if (!dataStream->pendingDestroy) {
        BeginReceiving(server, dataStream_);
    }
}

void HAPDataStreamTCPInvalidate(HAPAccessoryServer* server, HAPDataStreamRef* dataStream_) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream_);
    HAPDataStreamTCP* const dataStream = &dataStream_->tcp;
    HAPPrecondition(dataStream->tcpStreamIsConnected);

    SetPendingDestroy(server, dataStream_);

    // If TCP stream events are being handled, HomeKit Data Stream state will still be accessed.
    // Therefore, resources may not yet be released. HAPDataStreamInvalidate is called again after events are handled.
    if (dataStream->isHandlingEvent) {
        HAPLogDebug(
                &logObject,
                "[%p] TCP stream events are being handled. Delaying invalidation.",
                (const void*) dataStream_);
        return;
    }

    // To prevent memory leak, clear sending context before abruptly clearing all sending state.
    ClearSendingContext(server, dataStream_);

    if (dataStream->isMatchmakingComplete) {
        HAPLogInfo(&logObject, "[%p] Invalidating HomeKit Data Stream.", (const void*) dataStream_);

        // Abort transmissions.
        dataStream->isHandlingEvent = true;
        if (dataStream->accessoryToController.tx.completionHandler) {
            CompleteAccessoryToControllerTransmission(server, dataStream_, kHAPError_InvalidState);
        }
        if (dataStream->controllerToAccessory.tx.completionHandler) {
            CompleteControllerToAccessoryTransmission(server, dataStream_, kHAPError_InvalidState);
        }
        dataStream->isHandlingEvent = false;

        // Close TCP stream.
        HAPPlatformTCPStreamClose(HAPNonnull(GetTransportState(server)->tcpStreamManager), dataStream->tcpStream);
        dataStream->tcpStream = 0;
        dataStream->tcpStreamIsConnected = false;
        dataStream->streamPriority = kHAPStreamPriority_NoPriority;

        // Save arguments for callback invocation.
        HAPDataStreamRequest request;
        HAPDataStreamGetRequestContext(server, dataStream_, &request);

        // Invalidate HomeKit Data Stream.
        HAPRawBufferZero(dataStream, sizeof *dataStream);

        // Inform delegate.
        HAPAssert(request.accessory->dataStream.delegate.callbacks);
        HAPAssert(request.accessory->dataStream.delegate.callbacks->handleInvalidate);
        request.accessory->dataStream.delegate.callbacks->handleInvalidate(
                server, &request, dataStream_, server->context);
    } else {
        HAPLogInfo(&logObject, "[%p] Invalidating TCP stream.", (const void*) dataStream_);

        HAPPlatformTCPStreamClose(HAPNonnull(GetTransportState(server)->tcpStreamManager), dataStream->tcpStream);
        HAPRawBufferZero(&dataStream->accessoryToController.tx, sizeof dataStream->accessoryToController.tx);
        HAPRawBufferZero(&dataStream->accessoryToController.nonce, sizeof dataStream->accessoryToController.nonce);
        HAPRawBufferZero(&dataStream->controllerToAccessory.tx, sizeof dataStream->controllerToAccessory.tx);
        HAPRawBufferZero(&dataStream->controllerToAccessory.nonce, sizeof dataStream->controllerToAccessory.nonce);
        dataStream->tcpStream = 0;
        dataStream->tcpStreamTTL = 0;
        dataStream->tcpStreamIsConnected = false;
        dataStream->streamPriority = kHAPStreamPriority_NoPriority;

        dataStream->pendingDestroy = false;

        UpdateMatchmakingState(server);
    }
}

void HAPDataStreamTCPInvalidateAllForHAPPairingID(
        HAPAccessoryServer* server,
        int pairingID,
        HAPDataStreamRef* dataStream_) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream_);

    HAPDataStreamTCP* const dataStream = &dataStream_->tcp;

    // DataStream is only valid if a session exists for it.
    if (!dataStream->session) {
        return;
    }

    // The session holds the originating pairing.
    if (dataStream->session->hap.pairingID != pairingID) {
        return;
    }

    HAPPrecondition(!dataStream->isHandlingEvent);

    HAPDataStreamInvalidate(server, dataStream_);
}

void HAPDataStreamTCPInvalidateAllForHAPSession(
        HAPAccessoryServer* server,
        HAPSession* _Nullable session,
        HAPDataStreamRef* dataStream_) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream_);

    HAPDataStreamTCP* const dataStream = &dataStream_->tcp;

    if (!dataStream->session) {
        return;
    }

    if (session != NULL && dataStream->session != session) {
        return;
    }

    HAPPrecondition(!dataStream->isHandlingEvent);

    if (dataStream->isMatchmakingComplete) {
        HAPDataStreamInvalidate(server, dataStream_);
    } else {
        HAPLogInfo(&logObject, "[%p] Invalidating HomeKit Data Stream setup request.", (const void*) dataStream_);

        ResetDataStreamInSetup(dataStream);

        UpdateMatchmakingState(server);
    }
}

/**
 * Complete matchmaking procedure.
 *
 * After receiving the first encrypted frame, loop through all the streams that are currently
 * waiting for their first frame. Try to decrypt using each of those keys. If one matches, then
 * the stream has been properly matched.
 *
 * After matching, merge the two Data Stream objects (the first being the one created during the
 * "Setup" procedure, and the second created due to the TCP Accept event), and reset one of them.
 *
 * @return true if matchmaking finds a good match.
 */
static bool CompleteMatchmaking(
        HAPAccessoryServer* server,
        HAPDataStreamRef* dataStream_,
        HAPDataStreamTransmission* tx,
        const uint8_t* nonce) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream_);
    HAPDataStreamTCP* const dataStream = &dataStream_->tcp;

    // Enumerate unmatched setup requests.
    for (size_t i = 0; i < server->dataStream.numDataStreams; i++) {
        HAPDataStreamRef* otherDataStream_ = &server->dataStream.dataStreams[i];
        HAPDataStreamTCP* otherDataStream = &otherDataStream_->tcp;
        if (otherDataStream->isMatchmakingComplete || !otherDataStream->setupRequested || !otherDataStream->session) {
            continue;
        }

        // Attempt decryption.
        // Crypto context for accessory -> controller communication is used to decrypt
        // because the controller -> accessory crypto context contains the data from initial frame.
        // Accessory -> controller crypto context is always unused at this point
        // as the delegate has not yet been informed about existence of this HomeKit Data Stream.
        HAP_chacha20_poly1305_ctx* crypto = &dataStream->accessoryToController.tx._.crypto;

        HAP_chacha20_poly1305_init(crypto, nonce, sizeof(uint64_t), otherDataStream->controllerToAccessory._.key.bytes);

        uint8_t frameTypeBytes[] = { tx->_.initialFrame.frameType };
        HAP_chacha20_poly1305_update_dec_aad(
                crypto,
                frameTypeBytes,
                sizeof frameTypeBytes,
                nonce,
                sizeof(uint64_t),
                otherDataStream->controllerToAccessory._.key.bytes);

        uint8_t totalDataBytesBytes[] = { HAPExpandBigUInt24((size_t) tx->numDataBytes) };
        HAP_chacha20_poly1305_update_dec_aad(
                crypto,
                totalDataBytesBytes,
                sizeof totalDataBytesBytes,
                nonce,
                sizeof(uint64_t),
                otherDataStream->controllerToAccessory._.key.bytes);

        uint8_t dataBytes[kHAPDataStream_MaxInitialDataBytes];
        HAP_chacha20_poly1305_update_dec(
                crypto,
                dataBytes,
                tx->data.mutableBytes,
                (size_t) tx->numDataBytes,
                nonce,
                sizeof(uint64_t),
                otherDataStream->controllerToAccessory._.key.bytes);

        // Check if decryption succeeded.
        int e = HAP_chacha20_poly1305_final_dec(crypto, tx->scratchBytes);
        HAPRawBufferZero(crypto, sizeof *crypto);
        if (e) {
            continue;
        }
        HAPAssert(tx->dataIsMutable);
        HAPRawBufferCopyBytes(HAPNonnullVoid(tx->data.mutableBytes), dataBytes, (size_t) tx->numDataBytes);

        HAPLogSensitiveBufferDebug(
                &logObject,
                nonce,
                sizeof(uint64_t),
                "[%p] Accessory <- Controller (%s)",
                (const void*) dataStream_,
                "nonce");
        HAPLogSensitiveBufferDebug(
                &logObject,
                frameTypeBytes,
                sizeof frameTypeBytes,
                "[%p] Accessory <- Controller (%s)",
                (const void*) dataStream_,
                "authenticated");
        HAPLogSensitiveBufferDebug(
                &logObject,
                totalDataBytesBytes,
                sizeof totalDataBytesBytes,
                "[%p] Accessory <- Controller (%s)",
                (const void*) dataStream_,
                "authenticated");
        HAPLogSensitiveBufferDebug(
                &logObject,
                tx->data.mutableBytes,
                (size_t) tx->numDataBytes,
                "[%p] Accessory <- Controller (%s)",
                (const void*) dataStream_,
                "encrypted");
        HAPLogSensitiveBufferDebug(
                &logObject,
                tx->scratchBytes,
                CHACHA20_POLY1305_TAG_BYTES,
                "[%p] Accessory <- Controller (%s)",
                (const void*) dataStream_,
                "plaintext");

        // Swap in setup request.
        HAPLogInfo(
                &logObject,
                "[%p] TCP stream matched with HomeKit Data Stream setup request [%p].",
                (const void*) dataStream_,
                (const void*) otherDataStream_);
        {
            HAPAssert(sizeof dataStream->accessoryToController._.key == sizeof(HAPSessionKey));
            HAPAssert(sizeof otherDataStream->accessoryToController._.key == sizeof(HAPSessionKey));
            HAPSessionKey accessoryToControllerKey;
            HAPRawBufferCopyBytes(
                    &accessoryToControllerKey, &otherDataStream->accessoryToController._.key, sizeof(HAPSessionKey));
            HAPRawBufferCopyBytes(
                    &otherDataStream->accessoryToController._.key,
                    &dataStream->accessoryToController._.key,
                    sizeof(HAPSessionKey));
            HAPRawBufferCopyBytes(
                    &dataStream->accessoryToController._.key, &accessoryToControllerKey, sizeof(HAPSessionKey));
            HAPRawBufferZero(&accessoryToControllerKey, sizeof accessoryToControllerKey);
        }
        {
            HAPAssert(sizeof dataStream->controllerToAccessory._.key == sizeof(HAPSessionKey));
            HAPAssert(sizeof otherDataStream->controllerToAccessory._.key == sizeof(HAPSessionKey));
            HAPSessionKey controllerToAccessoryKey;
            HAPRawBufferCopyBytes(
                    &controllerToAccessoryKey, &otherDataStream->controllerToAccessory._.key, sizeof(HAPSessionKey));
            HAPRawBufferCopyBytes(
                    &otherDataStream->controllerToAccessory._.key,
                    &dataStream->controllerToAccessory._.key,
                    sizeof(HAPSessionKey));
            HAPRawBufferCopyBytes(
                    &dataStream->controllerToAccessory._.key, &controllerToAccessoryKey, sizeof(HAPSessionKey));
            HAPRawBufferZero(&controllerToAccessoryKey, sizeof controllerToAccessoryKey);
        }
        {
            HAPSession* session = otherDataStream->session;
            otherDataStream->session = dataStream->session;
            dataStream->session = session;
        }
        {
            HAPServiceTypeIndex serviceTypeIndex = otherDataStream->serviceTypeIndex;
            otherDataStream->serviceTypeIndex = dataStream->serviceTypeIndex;
            dataStream->serviceTypeIndex = serviceTypeIndex;
        }
        {
            uint16_t setupTTL = otherDataStream->setupTTL;
            otherDataStream->setupTTL = dataStream->setupTTL;
            dataStream->setupTTL = setupTTL;
        }
        {
            bool setupRequested = otherDataStream->setupRequested;
            otherDataStream->setupRequested = dataStream->setupRequested;
            dataStream->setupRequested = setupRequested;
        }

        // Complete matchmaking.
        dataStream->setupTTL = 0;
        dataStream->tcpStreamTTL = 0;
        dataStream->isMatchmakingComplete = true;

        return true;
    }

    // No match.
    return false;
}

/**
 * Encryption type for a part of a frame.
 */
HAP_ENUM_BEGIN(uint8_t, HAPDataStreamTransmissionEncryptionType) {
    /** Neither encrypted nor authenticated. */
    kHAPDataStreamTransmissionEncryptionType_None,

    /** Authenticated but not encrypted. */
    kHAPDataStreamTransmissionEncryptionType_Authenticated,

    /** Encrypted. */
    kHAPDataStreamTransmissionEncryptionType_Encrypted
} HAP_ENUM_END(uint8_t, HAPDataStreamTransmissionEncryptionType);

/**
 * Continues receiving data over a HomeKit Data Stream.
 *
 * @param      server              Accessory server.
 * @param      dataStream_          HomeKit Data Stream.
 */
static void ContinueReceiving( // NOLINT(readability-function-size)
        HAPAccessoryServer* server,
        HAPDataStreamRef* dataStream_) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream_);
    HAPDataStreamTCP* const dataStream = &dataStream_->tcp;
    HAPPrecondition(!dataStream->pendingDestroy);
    HAPPrecondition(dataStream->tcpStreamIsConnected);

    HAPError err;

    HAPDataStreamTCPUnidirectionalState* state = &dataStream->controllerToAccessory;
    HAPDataStreamTransmission* tx = &state->tx;

// This receives a buffer incrementally.
// If not enough data is available, HAPDataStreamTCPUpdateInterests is called and the ContinueReceiving function
// returns. After receiving all data of the buffer, the buffer is decrypted and the macro returns normally. Note that a
// NULL buffer may be supplied. In this case, bytes will be decrypted but will not be saved.
#define RECEIVE_OR_YIELD(bytes_, maxBytes_, encryptionType) \
    do { \
        while (tx->numProgressBytes < (maxBytes_)) { \
            uint8_t* b; \
            size_t n = (maxBytes_) - (size_t) tx->numProgressBytes; \
            if ((bytes_) != NULL) { \
                b = &((uint8_t*) (bytes_))[tx->numProgressBytes]; \
            } else { \
                /* If bytes_ == NULL, use the temporary buffer and ignore received data after decryption. */ \
                b = tx->scratchBytes; \
                n = HAPMin(n, sizeof tx->scratchBytes); \
            } \
            HAPAssert(n); \
\
            /* Receive data. */ \
            size_t numBytes; \
            err = HAPPlatformTCPStreamRead( \
                    HAPNonnull(GetTransportState(server)->tcpStreamManager), dataStream->tcpStream, b, n, &numBytes); \
            if (err == kHAPError_Unknown) { \
                HAPLog(&logObject, \
                       "[%p] Failed to read from TCP stream. Invalidating HomeKit Data Stream.", \
                       (const void*) dataStream_); \
                SetPendingDestroy(server, dataStream_); \
                return; \
            } \
            if (err == kHAPError_Busy) { \
                /* Buffer hit empty before we got everything wanted. */ \
                HAPDataStreamTCPUpdateInterests(server, dataStream_); \
                return; \
            } \
            HAPAssert(!err); \
            if (!numBytes) { \
                HAPLog(&logObject, \
                       "[%p] Peer has closed TCP stream. Invalidating HomeKit Data Stream.", \
                       (const void*) dataStream_); \
                SetPendingDestroy(server, dataStream_); \
                return; \
            } \
            HAPAssert(numBytes <= n); \
            tx->numProgressBytes += numBytes; \
\
            /* Decrypt. */ \
            if (dataStream->isMatchmakingComplete) { \
                size_t numLogBytes = HAPMin(numBytes, HAP_HDS_RX_BUFFER_LOG_MAX_BYTES); \
                switch (encryptionType) { \
                    case kHAPDataStreamTransmissionEncryptionType_None: { \
                        HAPLogSensitiveBufferDebug( \
                                &logObject, \
                                b, \
                                numLogBytes, \
                                "[%p] Accessory <- Controller (%s) (%zu of %zu bytes)", \
                                (const void*) dataStream_, \
                                "plaintext", \
                                numLogBytes, \
                                numBytes); \
                        break; \
                    } \
                    case kHAPDataStreamTransmissionEncryptionType_Authenticated: { \
                        HAP_chacha20_poly1305_update_dec_aad( \
                                &tx->_.crypto, b, numBytes, nonce, sizeof nonce, state->_.key.bytes); \
                        HAPLogSensitiveBufferDebug( \
                                &logObject, \
                                b, \
                                numLogBytes, \
                                "[%p] Accessory <- Controller (%s) (%zu of %zu bytes)", \
                                (const void*) dataStream_, \
                                "authenticated", \
                                numLogBytes, \
                                numBytes); \
                        break; \
                    } \
                    case kHAPDataStreamTransmissionEncryptionType_Encrypted: { \
                        HAP_chacha20_poly1305_update_dec( \
                                &tx->_.crypto, b, b, numBytes, nonce, sizeof nonce, state->_.key.bytes); \
                        HAPLogSensitiveBufferDebug( \
                                &logObject, \
                                b, \
                                numLogBytes, \
                                "[%p] Accessory <- Controller (%s) (%zu of %zu bytes)", \
                                (const void*) dataStream_, \
                                "encrypted", \
                                numLogBytes, \
                                numBytes); \
                        break; \
                    } \
                    default: \
                        HAPFatalError(); \
                } \
            } else { \
                HAPLogDebug( \
                        &logObject, \
                        "[%p] Delaying decryption of incoming data (%zu bytes) until matchmaking completes.", \
                        (const void*) dataStream_, \
                        numBytes); \
            } \
        } \
        HAPAssert(tx->numProgressBytes == (maxBytes_)); \
        tx->numProgressBytes = 0; \
    } while (0)

    // See HomeKit Accessory Protocol Specification R17
    // Section 9.1.2 Frame Format
    while (tx->state != kHAPDataStreamTransmissionState_Idle) {
        const uint8_t nonce[] = { HAPExpandLittleUInt64(state->nonce) };
        bool isInitialFrame = !state->nonce;

        switch ((HAPDataStreamTransmissionState) tx->state) {
            case kHAPDataStreamTransmissionState_Idle:
            case kHAPDataStreamTransmissionState_DataEncrypted: {
                HAPFatalError();
            }
            case kHAPDataStreamTransmissionState_PrepareFrame: {
                if (dataStream->isMatchmakingComplete) {
                    HAPLogSensitiveBufferDebug(
                            &logObject,
                            nonce,
                            sizeof nonce,
                            "[%p] Accessory <- Controller (%s)",
                            (const void*) dataStream_,
                            "nonce");
                    HAP_chacha20_poly1305_init(&tx->_.crypto, nonce, sizeof nonce, state->_.key.bytes);
                }

                // Advance state.
                tx->state = kHAPDataStreamTransmissionState_FrameHeader;
                HAP_FALLTHROUGH;
            }
            case kHAPDataStreamTransmissionState_FrameHeader: {
                RECEIVE_OR_YIELD(
                        &tx->scratchBytes[0],
                        HAP_DATASTREAM_FRAME_HEADER_LENGTH,
                        kHAPDataStreamTransmissionEncryptionType_Authenticated);

                // Handle Type.
                HAPDataStreamFrameType const frameType = tx->scratchBytes[0];
                bool acceptedFrameType = false;
                switch (frameType) {
                    case kHAPDataStreamFrameType_EncryptedMessage:
                        // Only supported frame type.
                        acceptedFrameType = true;
                        break;
                    case kHAPDataStreamFrameType_UnencryptedMessage:
                        // Not accepted.
                        break;
                }
                if (!acceptedFrameType) {
                    HAPLog(&logObject,
                           "[%p] Received packet with unexpected frame type: 0x%02X (%d bytes).",
                           (const void*) dataStream_,
                           frameType,
                           tx->totalDataBytes);
                    SetPendingDestroy(server, dataStream_);
                    break;
                }
                if (!dataStream->isMatchmakingComplete) {
                    tx->_.initialFrame.frameType = tx->scratchBytes[0];
                }

                // Handle Length.
                uint32_t totalDataBytes = HAPReadBigUInt24(&tx->scratchBytes[1]);
                if (totalDataBytes > kHAPDataStream_MaxPayloadBytes) {
                    HAPLog(&logObject,
                           "[%p] Received frame with unsupported Length: %lu.",
                           (const void*) dataStream_,
                           (unsigned long) totalDataBytes);
                    SetPendingDestroy(server, dataStream_);
                    return;
                }
                if (!dataStream->isMatchmakingComplete) {
                    if (totalDataBytes > sizeof tx->_.initialFrame.bytes) {
                        HAPLog(&logObject,
                               "[%p] Received initial frame with unexpected Length: %lu.",
                               (const void*) dataStream_,
                               (unsigned long) totalDataBytes);
                        SetPendingDestroy(server, dataStream_);
                        return;
                    }
                }
                // FIXME: What do the next 3 lines do?
                tx->totalDataBytes = kHAPDataStream_MaxPayloadBytes;
                tx->totalDataBytes++;
                HAPAssert(!tx->totalDataBytes);

                // Advance state.
                tx->totalDataBytes = totalDataBytes;
                tx->state = kHAPDataStreamTransmissionState_DataReadyToAccept;
                HAP_FALLTHROUGH;
            }
            case kHAPDataStreamTransmissionState_DataReadyToAccept: {
                // Advance state.
                tx->state = kHAPDataStreamTransmissionState_Data;

                // Inform delegate.
                if (dataStream->isMatchmakingComplete) {
                    HAPLogInfo(
                            &logObject,
                            "[%p] Ready to accept data (%zu bytes).",
                            (const void*) dataStream_,
                            (size_t) tx->totalDataBytes);
                    HAPDataStreamRequest request;
                    HAPDataStreamGetRequestContext(server, dataStream_, &request);
                    HAPAssert(request.accessory->dataStream.delegate.callbacks);
                    HAPAssert(request.accessory->dataStream.delegate.callbacks->handleData);
                    request.accessory->dataStream.delegate.callbacks->handleData(
                            server, &request, dataStream_, (size_t) tx->totalDataBytes, server->context);
                    if (dataStream->pendingDestroy) {
                        return;
                    }
                } else {
                    // Simulate HAPDataStreamReceiveData call.
                    tx->dataIsMutable = true;
                    tx->data.mutableBytes = tx->_.initialFrame.bytes;
                    tx->numDataBytes = tx->totalDataBytes;
                }
                HAP_FALLTHROUGH;
            }
            case kHAPDataStreamTransmissionState_Data: {
                // Completion handler must always be called at least once, even if frame is empty.
                // Otherwise there is no way to inform the delegate about decryption issues.
                do {
                    if (dataStream->isMatchmakingComplete) {
                        if (!tx->completionHandler) {
                            HAPDataStreamTCPUpdateInterests(server, dataStream_);
                            return;
                        }
                    }
                    HAPAssert(tx->dataIsMutable == (tx->data.mutableBytes != NULL));
                    HAPAssert(tx->numDataBytes <= tx->totalDataBytes);
                    if (dataStream->isMatchmakingComplete && isInitialFrame) {
                        HAPAssert(tx->totalDataBytes <= sizeof tx->_.initialFrame.bytes);
                        if (tx->data.mutableBytes) {
                            HAPRawBufferCopyBytes(
                                    HAPNonnullVoid(tx->data.mutableBytes),
                                    tx->_.initialFrame.bytes,
                                    (size_t) tx->numDataBytes);
                            HAPRawBufferCopyBytes(
                                    &tx->_.initialFrame.bytes[0],
                                    &tx->_.initialFrame.bytes[tx->numDataBytes],
                                    (size_t) tx->totalDataBytes - (size_t) tx->numDataBytes);
                        }
                    } else {
                        RECEIVE_OR_YIELD(
                                tx->data.mutableBytes,
                                (size_t) tx->numDataBytes,
                                kHAPDataStreamTransmissionEncryptionType_Encrypted);
                    }
                    tx->totalDataBytes -= tx->numDataBytes;

                    // If there is more data incoming, inform delegate that current chunk is done.
                    // Otherwise, wait until Auth Tag has been verified.
                    if (tx->totalDataBytes) {
                        // Data from initial frame is always read in one chunk.
                        HAPAssert(dataStream->isMatchmakingComplete);

                        NotifyDelegateOfCompletion(server, dataStream_, kHAPError_None, state, /* isComplete */ false);

                        if (dataStream->pendingDestroy) {
                            return;
                        }
                    } else {
                        // Advance state.
                        tx->state = kHAPDataStreamTransmissionState_AuthTag;
                    }
                } while (tx->totalDataBytes);
                HAP_FALLTHROUGH;
            }
            case kHAPDataStreamTransmissionState_AuthTag: {
                if (dataStream->isMatchmakingComplete) {
                    // Initial frame has already been decrypted.
                    if (!isInitialFrame) {
                        HAPAssert(sizeof tx->scratchBytes >= CHACHA20_POLY1305_TAG_BYTES);
                        RECEIVE_OR_YIELD(
                                tx->scratchBytes,
                                CHACHA20_POLY1305_TAG_BYTES,
                                kHAPDataStreamTransmissionEncryptionType_None);

                        int e = HAP_chacha20_poly1305_final_dec(&tx->_.crypto, tx->scratchBytes);
                        if (e) {
                            HAPLog(&logObject, "[%p] Failed to decrypt frame.", (const void*) dataStream_);
                            SetPendingDestroy(server, dataStream_);
                            return;
                        }
                    }

                    // Advance state.
                    tx->state = kHAPDataStreamTransmissionState_Idle;
                    HAPDataStreamTCPUpdateInterests(server, dataStream_);
                    CompleteControllerToAccessoryTransmission(server, dataStream_, kHAPError_None);
                    if (dataStream->pendingDestroy) {
                        return;
                    }
                } else {
                    HAPAssert(sizeof tx->scratchBytes >= CHACHA20_POLY1305_TAG_BYTES);
                    RECEIVE_OR_YIELD(
                            tx->scratchBytes,
                            CHACHA20_POLY1305_TAG_BYTES,
                            kHAPDataStreamTransmissionEncryptionType_None);

                    HAPLogInfo(
                            &logObject,
                            "[%p] Received initial frame. Starting matchmaking.",
                            (const void*) dataStream_);
                    HAPAssert(isInitialFrame);

                    bool completed = CompleteMatchmaking(server, dataStream_, tx, nonce);
                    if (!completed) {
                        HAPLog(&logObject,
                               "[%p] No matching HomeKit Data Stream setup request found for TCP stream.",
                               (const void*) dataStream_);
                        SetPendingDestroy(server, dataStream_);
                        return;
                    }

                    // Stop TCP listener if all sessions have been matched.
                    UpdateMatchmakingState(server);
                    HAPAssert(dataStream->isMatchmakingComplete);

                    // Rewind state to transition to normal operation.
                    // This allows the "handleData" to provide the delegate with the data.
                    // See DataReadyToAccept for that.
                    tx->scratchBytes[0] = tx->_.initialFrame.frameType;
                    tx->totalDataBytes = tx->numDataBytes;

                    // Allow delegate to plug in own completion handler.
                    HAPAssert(!tx->completionHandler);
                    tx->dataIsMutable = false;
                    tx->data.mutableBytes = NULL;
                    tx->numDataBytes = 0;

                    // Inform delegate.
                    HAPDataStreamRequest request;
                    HAPDataStreamGetRequestContext(server, dataStream_, &request);
                    HAPAssert(request.accessory->dataStream.delegate.callbacks);
                    HAPAssert(request.accessory->dataStream.delegate.callbacks->handleAccept);
                    request.accessory->dataStream.delegate.callbacks->handleAccept(
                            server, &request, dataStream_, server->context);
                    if (dataStream->pendingDestroy) {
                        return;
                    }

                    // Advance state.
                    tx->state = kHAPDataStreamTransmissionState_DataReadyToAccept;
                }
                continue;
            }
            default:
                HAPFatalError();
        }
    }

#undef RECEIVE_OR_YIELD
}

/**
 * Continues sending data over a HomeKit Data Stream.
 *
 * @param      server              Accessory server.
 * @param      dataStream_          HomeKit Data Stream.
 */
static void ContinueSending(HAPAccessoryServer* server, HAPDataStreamRef* dataStream_) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream_);
    HAPDataStreamTCP* const dataStream = &dataStream_->tcp;
    HAPPrecondition(!dataStream->pendingDestroy);
    HAPPrecondition(dataStream->isMatchmakingComplete);

    HAPError err;

    HAPDataStreamTCPUnidirectionalState* state = &dataStream->accessoryToController;
    HAPDataStreamTransmission* tx = &state->tx;

#define ENCRYPT(bytes_, numBytes_, encryptionType) \
    do { \
        uint8_t* b = (bytes_); \
        size_t n = (numBytes_); \
\
        switch (encryptionType) { \
            case kHAPDataStreamTransmissionEncryptionType_None: { \
                break; \
            } \
            case kHAPDataStreamTransmissionEncryptionType_Authenticated: { \
                HAP_chacha20_poly1305_update_enc_aad(&tx->_.crypto, b, n, nonce, sizeof nonce, state->_.key.bytes); \
                break; \
            } \
            case kHAPDataStreamTransmissionEncryptionType_Encrypted: { \
                HAP_chacha20_poly1305_update_enc(&tx->_.crypto, b, b, n, nonce, sizeof nonce, state->_.key.bytes); \
                break; \
            } \
            default: \
                HAPFatalError(); \
        } \
    } while (0)

// This sends a buffer incrementally.
// If not enough space is available, HAPDataStreamTCPUpdateInterests is called and the ContinueSending function returns.
// Data must be encrypted before the initial invocation of this macro.
// This is necessary because this function cannot distinguish the case if the initial write returns busy error,
// and therefore does not know whether or not encryption must still be applied.
#define SEND_OR_YIELD(bytes_, maxBytes_, progressOffset_) \
    do { \
        HAP_DIAGNOSTIC_PUSH \
        HAP_DIAGNOSTIC_IGNORED_ARMCC(186) \
        HAPAssert((progressOffset_) <= tx->numProgressBytes); \
        HAP_DIAGNOSTIC_POP \
        while ((size_t) tx->numProgressBytes - (progressOffset_) < (maxBytes_)) { \
            const uint8_t* b = &((const uint8_t*) (bytes_))[(size_t) tx->numProgressBytes - (progressOffset_)]; \
            size_t n = (maxBytes_) - ((size_t) tx->numProgressBytes - (progressOffset_)); \
            HAPAssert(n); \
\
            /* Send data. */ \
            size_t numBytes; \
            err = HAPPlatformTCPStreamWrite( \
                    HAPNonnull(GetTransportState(server)->tcpStreamManager), dataStream->tcpStream, b, n, &numBytes); \
            if (err == kHAPError_Unknown) { \
                HAPLog(&logObject, \
                       "[%p] Failed to write to TCP stream. Invalidating HomeKit Data Stream.", \
                       (const void*) dataStream_); \
                SetPendingDestroy(server, dataStream_); \
                return; \
            } \
            if (err == kHAPError_Busy) { \
                HAPDataStreamTCPUpdateInterests(server, dataStream_); \
                return; \
            } \
            HAPAssert(!err); \
            HAPAssert(numBytes); \
            HAPAssert(numBytes <= n); \
            tx->numProgressBytes += numBytes; \
        } \
        HAPAssert((size_t) tx->numProgressBytes - (progressOffset_) == (maxBytes_)); \
        tx->numProgressBytes = (progressOffset_); \
    } while (0)

    // See HomeKit Accessory Protocol Specification R17
    // Section 9.1.2 Frame Format
    while (tx->state != kHAPDataStreamTransmissionState_Idle) {
        const uint8_t nonce[] = { HAPExpandLittleUInt64(state->nonce) };

        switch ((HAPDataStreamTransmissionState) tx->state) {
            case kHAPDataStreamTransmissionState_Idle:
            case kHAPDataStreamTransmissionState_DataReadyToAccept: {
            }
                HAPFatalError();
            case kHAPDataStreamTransmissionState_PrepareFrame: {
                HAP_chacha20_poly1305_init(&tx->_.crypto, nonce, sizeof nonce, state->_.key.bytes);

                // Type. The only supported one.
                tx->scratchBytes[0] = kHAPDataStreamFrameType_EncryptedMessage;

                // Length has been set to tx->totalDataBytes in HAPDataStreamPrepareData.
                HAPAssert(tx->totalDataBytes <= kHAPDataStream_MaxPayloadBytes);
                HAPWriteBigUInt24(&tx->scratchBytes[1], (size_t) tx->totalDataBytes);

                // Encrypt both.
                ENCRYPT(&tx->scratchBytes[0],
                        HAP_DATASTREAM_FRAME_HEADER_LENGTH,
                        kHAPDataStreamTransmissionEncryptionType_Authenticated);

                // Advance state.
                tx->state = kHAPDataStreamTransmissionState_FrameHeader;
                HAP_FALLTHROUGH;
            }
            case kHAPDataStreamTransmissionState_FrameHeader: {
                SEND_OR_YIELD(&tx->scratchBytes[0], HAP_DATASTREAM_FRAME_HEADER_LENGTH, 0);

                // Advance state.
                tx->state = kHAPDataStreamTransmissionState_Data;

                HAPAssert(!tx->dataIsMutable);
                HAPAssert(!tx->data.mutableBytes);
                HAPAssert(!tx->numDataBytes);

                // Signal that the Prepare has been completed and the delegate can start pushing data.
                NotifyDelegateOfCompletion(server, dataStream_, kHAPError_None, state, /* isComplete */ false);

                if (dataStream->pendingDestroy) {
                    return;
                }
                HAP_FALLTHROUGH;
            }
            case kHAPDataStreamTransmissionState_Data:
            case kHAPDataStreamTransmissionState_DataEncrypted: {
                // HAPDataStreamSendData / HAPDataStreamSendMutableData must always be called, even if frame is empty.
                // This is for consistency with the receive flow.

                do {
                    if (!tx->completionHandler) {
                        HAPDataStreamTCPUpdateInterests(server, dataStream_);
                        return;
                    }

                    if (tx->dataIsMutable) {
                        HAPAssert(tx->data.mutableBytes);
                        if (tx->state == kHAPDataStreamTransmissionState_Data) {
                            ENCRYPT(tx->data.mutableBytes,
                                    (size_t) tx->numDataBytes,
                                    kHAPDataStreamTransmissionEncryptionType_Encrypted);
                            tx->state = kHAPDataStreamTransmissionState_DataEncrypted;
                        }
                        SEND_OR_YIELD(tx->data.mutableBytes, (size_t) tx->numDataBytes, 0);
                    } else {
                        // In this case, we have to use the temporary buffer because original buffer is immutable.
                        HAPAssert(tx->data.bytes);
                        while (tx->numProgressBytes < (size_t) tx->numDataBytes) {
                            // Recompute how far full chunks have been sent.
                            size_t progressOffset = (size_t) tx->numProgressBytes -
                                                    (size_t) tx->numProgressBytes % sizeof tx->scratchBytes;

                            // Recompute size of next chunk.
                            size_t numChunkBytes = HAPMin(
                                    (size_t) tx->numDataBytes - (size_t) progressOffset, sizeof tx->scratchBytes);

                            // Encrypt next chunk if necessary.
                            if (tx->state == kHAPDataStreamTransmissionState_Data) {
                                HAPAssert(progressOffset == tx->numProgressBytes);
                                HAPRawBufferCopyBytes(
                                        tx->scratchBytes,
                                        &((const uint8_t*) tx->data.bytes)[progressOffset],
                                        numChunkBytes);
                                ENCRYPT(tx->scratchBytes,
                                        numChunkBytes,
                                        kHAPDataStreamTransmissionEncryptionType_Encrypted);
                                tx->state = kHAPDataStreamTransmissionState_DataEncrypted;
                            }

                            // Send encrypted chunk.
                            SEND_OR_YIELD(tx->scratchBytes, numChunkBytes, progressOffset);
                            tx->state = kHAPDataStreamTransmissionState_Data;
                            tx->numProgressBytes += numChunkBytes;
                        }
                        tx->numProgressBytes = 0;
                    }
                    HAPAssert(tx->numDataBytes <= tx->totalDataBytes);
                    tx->totalDataBytes -= tx->numDataBytes;
                    tx->state = kHAPDataStreamTransmissionState_Data;

                    // If there is more data outgoing, inform delegate that current chunk is done.
                    // Otherwise, wait until Auth Tag has been sent.
                    if (tx->totalDataBytes) {
                        NotifyDelegateOfCompletion(server, dataStream_, kHAPError_None, state, /* isComplete */ false);

                        if (dataStream->pendingDestroy) {
                            return;
                        }
                    } else {
                        // Compute Auth Tag.
                        HAPAssert(sizeof tx->scratchBytes >= CHACHA20_POLY1305_TAG_BYTES);
                        HAP_chacha20_poly1305_final_enc(&tx->_.crypto, tx->scratchBytes);

                        // Advance state.
                        tx->state = kHAPDataStreamTransmissionState_AuthTag;
                        ENCRYPT(tx->scratchBytes,
                                CHACHA20_POLY1305_TAG_BYTES,
                                kHAPDataStreamTransmissionEncryptionType_None);
                    }
                } while (tx->totalDataBytes);
                HAP_FALLTHROUGH;
            }
            case kHAPDataStreamTransmissionState_AuthTag: {
                SEND_OR_YIELD(tx->scratchBytes, CHACHA20_POLY1305_TAG_BYTES, 0);

                // Advance state.
                tx->state = kHAPDataStreamTransmissionState_Idle;
                HAPDataStreamTCPUpdateInterests(server, dataStream_);
                CompleteAccessoryToControllerTransmission(server, dataStream_, kHAPError_None);
                if (dataStream->pendingDestroy) {
                    return;
                }
                continue;
            }
            default:
                HAPFatalError();
        }
    }

#undef SEND_OR_YIELD
#undef ENCRYPT
}

/**
 * Clear context of sending over a HomeKit Data Stream.
 *
 * This function deallocates all memory blocks associated with the sending context.
 *
 * @param      server              Accessory server.
 * @param      dataStream_          HomeKit Data Stream.
 */
static void ClearSendingContext(HAPAccessoryServer* server, HAPDataStreamRef* dataStream_) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream_);
    HAPDataStreamTCP* const dataStream = &dataStream_->tcp;

    HAPDataStreamTCPUnidirectionalState* state = &dataStream->accessoryToController;
    HAPDataStreamTransmission* tx = &state->tx;

    HAPLogDebug(&logObject, "%s: tx state = %llu", __func__, (unsigned long long) tx->state);
    if (tx->state == kHAPDataStreamTransmissionState_FrameHeader || tx->state == kHAPDataStreamTransmissionState_Data ||
        tx->state == kHAPDataStreamTransmissionState_DataEncrypted) {
        HAP_chacha20_poly1305_final_enc(&tx->_.crypto, tx->scratchBytes);
        // This function must be called before the tx state is reset and hence
        // in theory, the following assignment is not necessary,
        // but the state is set just in case.
        tx->state = kHAPDataStreamTransmissionState_Idle;
    }
}

/**
 * Returns the HomeKit Data Stream that manages a given TCP stream.
 *
 * @param      server              Accessory server.
 * @param      tcpStream            TCP stream.
 *
 * @return HomeKit Data Stream that manages the given TCP stream.
 */
static HAPDataStreamRef* DataStreamForTCPStream(HAPAccessoryServer* server, HAPPlatformTCPStreamRef tcpStream) {
    HAPPrecondition(server);

    for (size_t i = 0; i < server->dataStream.numDataStreams; i++) {
        HAPDataStreamRef* dataStream_ = &server->dataStream.dataStreams[i];
        HAPDataStreamTCP* dataStream = &dataStream_->tcp;
        if (!dataStream->tcpStreamIsConnected) {
            continue;
        }
        if (dataStream->tcpStream == tcpStream) {
            return dataStream_;
        }
    }
    HAPFatalError();
}

static void HandleTCPStreamEvent(
        HAPPlatformTCPStreamManagerRef tcpStreamManager,
        HAPPlatformTCPStreamRef tcpStream,
        HAPPlatformTCPStreamEvent event,
        void* _Nullable context) {
    HAPPrecondition(tcpStreamManager);
    HAPPrecondition(context);
    HAPAccessoryServer* const server = context;
    HAPPrecondition(tcpStreamManager == GetTransportState(server)->tcpStreamManager);

    HAPDataStreamRef* dataStream_ = DataStreamForTCPStream(server, tcpStream);
    HAPDataStreamTCP* dataStream = &dataStream_->tcp;
    HAPPrecondition(!dataStream->pendingDestroy);
    HAPPrecondition(dataStream->tcpStreamIsConnected);

    HAPAssert(!dataStream->isHandlingEvent);
    dataStream->isHandlingEvent = true;
    if (event.hasSpaceAvailable && !dataStream->pendingDestroy) {
        ContinueSending(server, dataStream_);
        // Update HAP session last used timestamp
        if (dataStream->session) {
            HAPAccessoryServerUpdateIPSessionLastUsedTimestamp(dataStream->session->server, dataStream->session);
        }
    }
    if (event.hasBytesAvailable && !dataStream->pendingDestroy) {
        ContinueReceiving(server, dataStream_);
        // Update HAP session last used timestamp
        if (dataStream->session) {
            HAPAccessoryServerUpdateIPSessionLastUsedTimestamp(dataStream->session->server, dataStream->session);
        }
    }
    HAPAssert(dataStream->isHandlingEvent);
    dataStream->isHandlingEvent = false;
    if (dataStream->pendingDestroy) {
        HAPDataStreamInvalidate(server, dataStream_);
    }
}

/**
 * Updates TCP stream interests for a HomeKit Data Stream.
 *
 * - HandleTCPStreamEvent will be called once data or space is available.
 */
static void HAPDataStreamTCPUpdateInterests(HAPAccessoryServer* server, HAPDataStreamRef* dataStream_) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream_);
    HAPDataStreamTCP* const dataStream = &dataStream_->tcp;
    HAPPrecondition(dataStream->tcpStreamIsConnected);

    HAPPlatformTCPStreamEvent interests;
    HAPRawBufferZero(&interests, sizeof interests);

    if (!dataStream->pendingDestroy) {
        // Accessory to controller.
        {
            HAPDataStreamTransmission* tx = &dataStream->accessoryToController.tx;
            if (tx->state != kHAPDataStreamTransmissionState_Idle) {
                if (tx->state != kHAPDataStreamTransmissionState_Data || tx->numProgressBytes != tx->numDataBytes) {
                    interests.hasSpaceAvailable = true;
                }
            }
        }

        // Controller to accessory.
        {
            HAPDataStreamTransmission* tx = &dataStream->controllerToAccessory.tx;
            if (tx->state != kHAPDataStreamTransmissionState_Idle) {
                if (tx->state != kHAPDataStreamTransmissionState_Data || tx->numProgressBytes != tx->numDataBytes) {
                    interests.hasBytesAvailable = true;
                }
            }
        }
    }

    HAPPlatformTCPStreamUpdateInterests(
            HAPNonnull(GetTransportState(server)->tcpStreamManager),
            dataStream->tcpStream,
            interests,
            HandleTCPStreamEvent,
            server);
}

static void HandlePendingTCPStream(HAPPlatformTCPStreamManagerRef tcpStreamManager, void* _Nullable context) {
    HAPPrecondition(tcpStreamManager);
    HAPAccessoryServer* const server = context;
    HAPPrecondition(server);
    HAPPrecondition(tcpStreamManager == GetTransportState(server)->tcpStreamManager);

    HAPError err;

    HAPLogInfo(&logObject, "TCP stream connected.");

    // Accept TCP stream.
    HAPPlatformTCPStreamRef tcpStream;
    err = HAPPlatformTCPStreamManagerAcceptTCPStream(tcpStreamManager, &tcpStream);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLog(&logObject,
               "Failed to accept TCP connection for HomeKit Data Stream (listener port %u).",
               HAPPlatformTCPStreamManagerGetListenerPort(tcpStreamManager));
        return;
    }

    // Update reference time for timeouts.
    UpdateMatchmakingState(server);

    // Find memory to register TCP stream.
    for (size_t i = 0; i < server->dataStream.numDataStreams; i++) {
        HAPDataStreamRef* dataStream_ = &server->dataStream.dataStreams[i];
        HAPDataStreamTCP* dataStream = &dataStream_->tcp;

        if (dataStream->tcpStreamIsConnected) {
            continue;
        }

        HAPAssert(!dataStream->isMatchmakingComplete);
        HAPAssert(
                HAPRawBufferIsZero(&dataStream->accessoryToController.tx, sizeof dataStream->accessoryToController.tx));
        HAPAssert(HAPRawBufferIsZero(
                &dataStream->accessoryToController.nonce, sizeof dataStream->accessoryToController.nonce));
        HAPAssert(
                HAPRawBufferIsZero(&dataStream->controllerToAccessory.tx, sizeof dataStream->controllerToAccessory.tx));
        HAPAssert(HAPRawBufferIsZero(
                &dataStream->controllerToAccessory.nonce, sizeof dataStream->controllerToAccessory.nonce));
        HAPAssert(!dataStream->tcpStream);
        HAPAssert(!dataStream->tcpStreamTTL);
        HAPAssert(!dataStream->tcpStreamIsConnected);

        // Register TTL.
        dataStream->tcpStreamTTL = kHAPDataStream_TCPStreamTimeout;

        // Register TCP stream.
        dataStream->tcpStream = tcpStream;
        dataStream->tcpStreamIsConnected = true;

        // Start enforcing TTL.
        HAPLogInfo(&logObject, "[%p] TCP stream allocated.", (const void*) dataStream_);
        UpdateMatchmakingState(server);
        if (dataStream->tcpStreamIsConnected) {
            // Start receiving first frame.
            BeginReceiving(server, dataStream_);
        }
        return;
    }

    HAPLogError(
            &logObject,
            "No space to allocate TCP stream (%zu max). Adjust HAPAccessoryServerOptions.",
            server->dataStream.numDataStreams);
    HAPPlatformTCPStreamClose(tcpStreamManager, tcpStream);
}

HAP_RESULT_USE_CHECK
HAPDataStreamTransmission* HAPDataStreamTCPGetReceiveTransmission(HAPDataStreamRef* dataStream_) {
    HAPDataStreamTCP* dataStream = &dataStream_->tcp;
    HAPPrecondition(dataStream->isMatchmakingComplete);
    HAPPrecondition(!dataStream->pendingDestroy);

    return &dataStream->controllerToAccessory.tx;
}

HAP_RESULT_USE_CHECK
HAPDataStreamTransmission* HAPDataStreamTCPGetSendTransmission(HAPDataStreamRef* dataStream_) {
    HAPDataStreamTCP* dataStream = &dataStream_->tcp;
    HAPPrecondition(dataStream->isMatchmakingComplete);
    HAPPrecondition(!dataStream->pendingDestroy);

    return &dataStream->accessoryToController.tx;
}

void HAPDataStreamTCPDoReceive(HAPAccessoryServer* server, HAPDataStreamRef* dataStream_) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream_);

    HAPDataStreamTCP* const dataStream = &dataStream_->tcp;

    HAPDataStreamTCPUpdateInterests(server, dataStream_);

    // If the initial frame is being received (nonce == 0), its data is already available in memory.
    // Manually call the TCP stream event handler to simulate that the data has just been received.
    if (!dataStream->isHandlingEvent && !dataStream->controllerToAccessory.nonce) {
        HAPPlatformTCPStreamEvent event = { .hasBytesAvailable = true, .hasSpaceAvailable = false };
        HandleTCPStreamEvent(GetTransportState(server)->tcpStreamManager, dataStream->tcpStream, event, server);
    }
}

void HAPDataStreamTCPDoSend(HAPAccessoryServer* server, HAPDataStreamRef* dataStream) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream);

    HAPDataStreamTCPUpdateInterests(server, dataStream);
}

void HAPDataStreamTCPSetMinimumPriority(
        HAPAccessoryServer* server,
        HAPDataStreamRef* dataStream_,
        HAPStreamPriority priority) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream_);

    HAPDataStreamTCP* const dataStream = &dataStream_->tcp;

    if (dataStream->streamPriority >= priority) {
        HAPLogDebug(
                &logObject,
                "%s: [%p] Current priority %u is higher than or equal to requested %u",
                __func__,
                (void*) dataStream,
                (unsigned) dataStream->streamPriority,
                (unsigned) priority);
        return;
    }

    dataStream->streamPriority = priority;
    HAPLogDebug(&logObject, "%s: [%p] Set priority to %u", __func__, (void*) dataStream, (unsigned) priority);
    HAPPlatformTCPStreamSetPriority(
            HAPNonnull(GetTransportState(server)->tcpStreamManager), dataStream->tcpStream, priority);
}

#endif // HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
