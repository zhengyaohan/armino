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
// Copyright (C) 2020-2021 Apple Inc. All Rights Reserved.

#include "HAPBase.h"
#include "HAPPlatformFeatures.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)

#include "HAPDiagnostics.h"
#include "HAPLogSubsystem.h"
#include "HAPPlatformDiagnostics.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
/**
 * Thread device wake lock timeout in milli seconds for Diagnostics HDS stream.
 */
#define DIAGNOSTICS_THREAD_WAKELOCK_TIMEOUT_MS 20000
#endif

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "HAPDiagnostics" };
static void UploadDiagnosticsData(HAPDiagnosticsContext* dgContext);
static void AbortDiagnosticsDataTimer(HAPDiagnosticsContext* dgContext);
#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
static HAPPlatformThreadWakeLock kHAPDiagnosticsThreadWakeLock;
#endif
HAPDiagnosticsContext* activeContext = NULL;

/**
 * Set up capturing of HAP logs to file
 */
HAP_RESULT_USE_CHECK
HAPError HAPDiagnosticsSetupLogCaptureToFile(
        const char* folderName,
        const char* logFileName,
        const size_t maxLogFileSizeMB,
        const HAPAccessory* accessory) {
    HAPPrecondition(accessory);
    HAPPrecondition(folderName);
    HAPPrecondition(logFileName);
    HAPPrecondition(maxLogFileSizeMB > 0);

    HAPLogInfo(&logObject, "%s", __func__);
    HAPError err;
    HAPAccessoryDiagnosticsConfig accessoryDiagnosticsConfig;
    HAPRawBufferZero(&accessoryDiagnosticsConfig, sizeof accessoryDiagnosticsConfig);
    HAPAssert(accessory->callbacks.diagnosticsConfig.getDiagnosticsConfig);

    err = accessory->callbacks.diagnosticsConfig.getDiagnosticsConfig(
            NULL, accessory, &accessoryDiagnosticsConfig, NULL);
    if (err) {
        return err;
    }

    HAPDiagnosticsContext* dgContext = (HAPDiagnosticsContext*) (accessoryDiagnosticsConfig.diagnosticsContext);
    HAPPrecondition(dgContext);

    err = HAPStringWithFormat(dgContext->folderName, sizeof dgContext->folderName, "%s", folderName);
    HAPAssert(!err);

    if (kHAPError_None != HAPPlatformDiagnosticsStartLogCaptureToFile(
                                  dgContext->folderName,
                                  logFileName,
                                  maxLogFileSizeMB * kHAPDiagnostics_MBToBytes,
                                  &(dgContext->logContext))) {
        HAPLogError(&kHAPLog_Default, "Failed to start diagnostics logging");
        return kHAPError_OutOfResources;
    }

    return kHAPError_None;
}

/**
 * Stop capturing HAP logs
 */
void HAPDiagnosticsStopLogCaptureToFile(const HAPAccessory* accessory) {
    HAPPrecondition(accessory);
    HAPLogInfo(&logObject, "%s", __func__);
    HAPError err;

    HAPAccessoryDiagnosticsConfig accessoryDiagnosticsConfig;
    HAPRawBufferZero(&accessoryDiagnosticsConfig, sizeof accessoryDiagnosticsConfig);

    HAPAssert(accessory->callbacks.diagnosticsConfig.getDiagnosticsConfig);

    err = accessory->callbacks.diagnosticsConfig.getDiagnosticsConfig(
            NULL, accessory, &accessoryDiagnosticsConfig, NULL);
    if (err) {
        // unexpected
        HAPLogError(&logObject, "Unexpected, Diagnostics config callback failed.");
        HAPAssert(!err);
    }

    HAPDiagnosticsContext* dgContext = (HAPDiagnosticsContext*) (accessoryDiagnosticsConfig.diagnosticsContext);
    HAPPrecondition(dgContext);
    HAPPlatformDiagnosticsStopLogCaptureToFile(&(dgContext->logContext));
    return;
}

/**
 * Abort diagnostics data timer callback to query PAL if diagnostics data is ready
 */
static void AbortDiagnosticsDataTimerCallback(HAPPlatformTimerRef timer HAP_UNUSED, void* _Nullable context) {
    if (context == NULL) {
        return;
    }

    HAPDiagnosticsContext* dgContext = (HAPDiagnosticsContext*) context;
    dgContext->abortDiagnosticsDataTimer = 0;

    HAPLogInfo(&logObject, "Timer expired. Aborting diagnostics data collection.");
    HAPPlatformDiagnosticsAbort(dgContext->prepareDataContext);
    HAPDiagnosticsDataStreamContext* dsContext = &(activeContext->dataStreamContext);
    HAPDataSendDataStreamProtocolCancelWithReason(
            dsContext->server,
            dsContext->dispatcher,
            dsContext->dataStream,
            dsContext->dataSendStream,
            kHAPDataSendDataStreamProtocolCancellationReason_UnexpectedFailure);
    return;
}

/**
 * Abort diagnostics data timer function will ask PAL to abort diagnostics data collection
 * after multiple query attempts.
 */
static void AbortDiagnosticsDataTimer(HAPDiagnosticsContext* dgContext) {
    if (dgContext == NULL) {
        return;
    }

    if (dgContext->abortDiagnosticsDataTimer) {
        return;
    }

    HAPError err = HAPPlatformTimerRegister(
            &(dgContext->abortDiagnosticsDataTimer),
            HAPPlatformClockGetCurrent() + kHAPDiagnostics_PrepareDataTimeout,
            AbortDiagnosticsDataTimerCallback,
            dgContext);
    if (err) {
        HAPLogError(&kHAPLog_Default, "%s: Unable to register timer for AbortDiagnosticsDataTimerCallback", __func__);
        HAPAssert(err == kHAPError_OutOfResources);
        HAPFatalError();
    }
    HAPLogInfo(&logObject, "Diagnostics data collection timer started.");
}

/**
 * Callback from HDS dataSend a packet is sent
 */
static void HandleSendDiagnosticDataComplete(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataSendDataStreamProtocol* dataStreamProtocol,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolStream* dataSendStream,
        HAPError error,
        void* scratchBytes,
        size_t numScratchBytes,
        void* _Nullable _context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol);
    HAPPrecondition(request);
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPPrecondition(dataSendStream);
    HAPPrecondition(scratchBytes);
    HAPPrecondition(numScratchBytes);
    HAPPrecondition(activeContext);
    HAPLogInfo(&logObject, "%s", __func__);

#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    HAPDiagnosticsDataStreamContext* dsContext = &(activeContext->dataStreamContext);
    HAPPrecondition(dsContext);
    if (dsContext->transportType == kHAPTransportType_Thread) {
        HAPPlatformThreadRemoveWakeLock(&kHAPDiagnosticsThreadWakeLock);
    }
#endif

    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        HAPLogError(
                &kHAPLog_Default,
                "%s: diagnostics dataStream = %u, dataSendStream = %p, "
                "error = HomeKit Data Stream is being invalidated.",
                __func__,
                dataStream,
                (const void*) dataSendStream);
        HAPPlatformDiagnosticsAbort(activeContext->prepareDataContext);
        return;
    }

    if (activeContext->isEOF != true) {
        UploadDiagnosticsData(activeContext);
    } else {
        HAPPlatformDiagnosticsDataTransferComplete(activeContext->prepareDataContext);
    }
}

/**
 * Sends diagnostic data packets over HDS
 */
static void UploadDiagnosticsData(HAPDiagnosticsContext* dgContext) {
    HAPPrecondition(dgContext);
    HAPLogInfo(&logObject, "%s", __func__);

    HAPDiagnosticsDataStreamContext* dsContext = &dgContext->dataStreamContext;
    HAPPrecondition(dsContext);
    HAPError err;
    uint8_t chunkBuf[kHAPDiagnostics_NumScratchBytes];
    size_t chunkBytes = kHAPDiagnostics_NumChunkBytes;
    size_t chunkBytesRead = 0;
    bool isEOF = false;

    if (dsContext->transportType == kHAPTransportType_BLE || dsContext->transportType == kHAPTransportType_Thread) {
        chunkBytes = kHAPDiagnostics_NumChunkBytesHDSOverHAP;
    }

#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    if (dsContext->transportType == kHAPTransportType_Thread) {
        HAPPlatformThreadAddWakeLock(&kHAPDiagnosticsThreadWakeLock, DIAGNOSTICS_THREAD_WAKELOCK_TIMEOUT_MS);
    }
#endif

    err = HAPPlatformDiagnosticsGetBytesToUpload(
            chunkBuf, chunkBytes, &chunkBytesRead, &isEOF, dgContext->prepareDataContext);
    if (err) {
        HAPDataSendDataStreamProtocolCancelWithReason(
                dsContext->server,
                dsContext->dispatcher,
                dsContext->dataStream,
                dsContext->dataSendStream,
                kHAPDataSendDataStreamProtocolCancellationReason_UnexpectedFailure);
        return;
    }
    HAPLogInfo(&logObject, "%zu bytes read, %zu bytes requested", chunkBytesRead, chunkBytes);
    dgContext->totalTransferBytes -= chunkBytesRead;

    HAPAssert(dgContext->totalTransferBytes >= 0);
    if (isEOF == true) {
        HAPAssert(dgContext->totalTransferBytes == 0);
        dgContext->isEOF = true;
        HAPLogInfo(&logObject, "End of file reached");
    }

    HAPDataSendDataStreamProtocolPacket packets[] = {
        { .data = { .bytes = chunkBuf, .numBytes = chunkBytesRead },
          .metadata = { .type = kHAPDataSendDataStreamProtocolType_Diagnostics_Snapshot,
                        ._.diagnostics.snapshot = { .dataSequenceNumber = dgContext->dataSequenceNumber,
                                                    .numUrlParameterPairs =
                                                            (dgContext->dataSequenceNumber == 1 &&
                                                             dgContext->urlParameters != NULL) ?
                                                                    dgContext->urlParameters->numUrlParameterPairs :
                                                                    0,
                                                    .urlParameterPairs =
                                                            (dgContext->dataSequenceNumber == 1 &&
                                                             dgContext->urlParameters != NULL) ?
                                                                    dgContext->urlParameters->urlParameterPairs :
                                                                    NULL } } }
    };

    dgContext->dataSequenceNumber++;
    err = HAPDataSendDataStreamProtocolSendData(
            dsContext->server,
            dsContext->dispatcher,
            dsContext->dataStream,
            dsContext->dataSendStream,
            dgContext->dataSendScratchBytes,
            sizeof dgContext->dataSendScratchBytes,
            packets,
            HAPArrayCount(packets),
            isEOF, // endOfStream
            HandleSendDiagnosticDataComplete);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPLogError(&kHAPLog_Default, "Scratch buffer too small. dataSendScratchBytes needs to be enlarged.");
        HAPFatalError();
    }
}

/**
 * Processes data events from PAL in the main thread
 */
static void ProcessDataEvents(void* _Nullable context HAP_UNUSED, size_t contextSize HAP_UNUSED) {
    HAPLogInfo(&logObject, "%s", __func__);

    if (activeContext == NULL || activeContext->dataStreamContext.dataSendStream == NULL) {
        HAPLogInfo(&kHAPLog_Default, "No active diagnostics data stream found. Ignoring event.");
        return;
    }

    HAPPrecondition(
            activeContext->eventType == kDiagnosticsDataEvent_Ready ||
            activeContext->eventType == kDiagnosticsDataEvent_Cancel);
    HAPLogInfo(&logObject, "eventType: %u", activeContext->eventType);

    // Cancel abort diagnostics data timer
    if (activeContext->abortDiagnosticsDataTimer) {
        HAPLogInfo(&kHAPLog_Default, "%s: Deregistering abort diagnostics data timer.", __func__);
        HAPPlatformTimerDeregister(activeContext->abortDiagnosticsDataTimer);
        activeContext->abortDiagnosticsDataTimer = 0;
    }

    HAPDiagnosticsDataStreamContext* dsContext = &(activeContext->dataStreamContext);
    switch (activeContext->eventType) {
        case kDiagnosticsDataEvent_Cancel: {
            HAPLogInfo(&kHAPLog_Default, "Cancelling data stream.");
            HAPPlatformDiagnosticsAbort(activeContext->prepareDataContext);
            HAPDataSendDataStreamProtocolCancelWithReason(
                    dsContext->server,
                    dsContext->dispatcher,
                    dsContext->dataStream,
                    dsContext->dataSendStream,
                    kHAPDataSendDataStreamProtocolCancellationReason_UnexpectedFailure);
            break;
        }
        case kDiagnosticsDataEvent_Ready: {
            HAPLogInfo(&kHAPLog_Default, "Uploading diagnostics data.");
            UploadDiagnosticsData(activeContext);
            break;
        }
        default: {
            HAPLogError(&kHAPLog_Default, "Unknown diagnostics data event.");
            HAPFatalError();
        }
    }
}

/**
 * Schedules cancel operation on the main thread.
 */
HAP_RESULT_USE_CHECK
HAPError HAPDiagnosticsDataCancel(void) {
    HAPLogInfo(&logObject, "%s", __func__);

    if (activeContext == NULL) {
        HAPLogError(&kHAPLog_Default, "No active diagnostics data stream found.");
        return kHAPError_Unknown;
    }

    activeContext->eventType = kDiagnosticsDataEvent_Cancel;

    // Synchronize execution to main thread.
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
    HAPError err;
    err = HAPPlatformRunLoopScheduleCallback(ProcessDataEvents, NULL, 0);
    if (err) {
        HAPLogError(&kHAPLog_Default, "Diagnostics event synchronization failed");
        HAPFatalError();
    }
    return kHAPError_None;
}

/**
 * Schedules upload diagnostics operation on the main thread.
 */
HAP_RESULT_USE_CHECK
HAPError HAPDiagnosticsDataReady(const size_t dataSizeBytes, HAPDiagnosticsUrlParameters* _Nullable urlParameters) {
    HAPPrecondition(dataSizeBytes > 0);
    HAPLogInfo(&logObject, "%s: Data size %zu bytes", __func__, dataSizeBytes);

    if (activeContext == NULL) {
        HAPLogError(&kHAPLog_Default, "No active diagnostics data stream found.");
        return kHAPError_Unknown;
    }

    activeContext->totalTransferBytes = dataSizeBytes;
    activeContext->urlParameters = urlParameters;
    activeContext->eventType = kDiagnosticsDataEvent_Ready;

    // Synchronize execution to main thread.
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
    HAPError err;
    err = HAPPlatformRunLoopScheduleCallback(ProcessDataEvents, NULL, 0);
    if (err) {
        HAPLogError(&kHAPLog_Default, "Diagnostics event synchronization failed");
        HAPFatalError();
    }
    return kHAPError_None;
}

static const HAPDiagnosticsCallbacks diagnosticsCallbacks = { .handleDiagnosticsDataReady = HAPDiagnosticsDataReady,
                                                              .handleDiagnosticsDataCancel = HAPDiagnosticsDataCancel };

/**
 * Callback from HDS dataSend when the stream is closed
 */
static void HandleDataSendStreamClose(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataSendDataStreamProtocol* dataStreamProtocol,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolStream* dataSendStream,
        HAPError error,
        HAPDataSendDataStreamProtocolCloseReason closeReason,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol);
    HAPPrecondition(request);
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPPrecondition(dataSendStream);

    HAPLogInfo(&logObject, "%s", __func__);

    HAPPrecondition(activeContext != NULL);
    HAPDiagnosticsContext* dgContext = activeContext;
    HAPAssert(dgContext);

    const char* errorDescription = NULL;
    switch (error) {
        case kHAPError_None: {
            errorDescription = "No error occurred.";
            break;
        }
        case kHAPError_InvalidState: {
            errorDescription = "HomeKit Data Stream is being invalidated.";
            break;
        }
        case kHAPError_InvalidData: {
            errorDescription = "Unexpected message has been received.";
            break;
        }
        case kHAPError_OutOfResources: {
            errorDescription = "Out of resources to receive message.";
            break;
        }
        case kHAPError_Unknown: {
            errorDescription = "Unknown error.";
            break;
        }
        case kHAPError_NotAuthorized: {
            errorDescription = "Not authorized to perform the operation.";
            break;
        }
        case kHAPError_Busy: {
            errorDescription = "Temporarily busy.";
            break;
        }
    }
    HAPAssert(errorDescription);

    const char* closeReasonDescription = NULL;
    switch (closeReason) {
        case kHAPDataSendDataStreamProtocolCloseReason_Normal: {
            closeReasonDescription = "Normal close.";
            break;
        }
        case kHAPDataSendDataStreamProtocolCloseReason_NotAllowed: {
            closeReasonDescription = "Controller will not allow the Accessory to send this transfer.";
            break;
        }
        case kHAPDataSendDataStreamProtocolCloseReason_Busy: {
            closeReasonDescription = "Controller cannot accept this transfer right now.";
            break;
        }
        case kHAPDataSendDataStreamProtocolCloseReason_Canceled: {
            closeReasonDescription = "Accessory will not finish the transfer.";
            break;
        }
        case kHAPDataSendDataStreamProtocolCloseReason_Unsupported: {
            closeReasonDescription = "Controller does not support this stream type.";
            break;
        }
        case kHAPDataSendDataStreamProtocolCloseReason_UnexpectedFailure: {
            closeReasonDescription = "Protocol error occurred and the stream has failed.";
            break;
        }
        case kHAPDataSendDataStreamProtocolCloseReason_Timeout: {
            closeReasonDescription = "Accessory could not start the session.";
            break;
        }
        case kHAPDataSendDataStreamProtocolCloseReason_BadData: {
            closeReasonDescription = "Controller failed to parse the data.";
            break;
        }
        case kHAPDataSendDataStreamProtocolCloseReason_ProtocolError: {
            closeReasonDescription = "A protocol error occurred.";
            break;
        }
        case kHAPDataSendDataStreamProtocolCloseReason_InvalidConfiguration: {
            closeReasonDescription = "Accessory not configured to perform the request.";
            break;
        }
    }
    HAPAssert(closeReasonDescription);

    HAPLogInfo(
            &kHAPLog_Default,
            "%s: Diagnostics, dataStream = %u, dataSendStream = %p, error = %s, closeReason = %s",
            __func__,
            dataStream,
            (const void*) dataSendStream,
            errorDescription,
            closeReasonDescription);

    HAPDiagnosticsDataStreamContext* dsContext = &(dgContext->dataStreamContext);
    HAPPrecondition(dsContext);

#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    if (dsContext->transportType == kHAPTransportType_Thread) {
        HAPPlatformThreadRemoveWakeLock(&kHAPDiagnosticsThreadWakeLock);
    }
#endif

    HAPRawBufferZero(dsContext, sizeof *dsContext);
    dgContext->dataSequenceNumber = 1;
    dgContext->isEOF = false;
    // Cancel abort diagnostics data timer if it exists
    if (activeContext->abortDiagnosticsDataTimer) {
        HAPLogInfo(&kHAPLog_Default, "%s: Deregistering abort diagnostics data timer.", __func__);
        HAPPlatformTimerDeregister(activeContext->abortDiagnosticsDataTimer);
        activeContext->abortDiagnosticsDataTimer = 0;

        // If timer is still active it implies PAL needs to be informed that the diagnostics data
        // transfer was aborted.
        HAPPlatformDiagnosticsAbort(activeContext->prepareDataContext);
    }
    activeContext = NULL;
}

/**
 * Callback from HDS dataSend when a stream is opened
 */
static void HandleDataSendStreamOpen(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataSendDataStreamProtocol* dataStreamProtocol,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolStream* dataSendStream,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol);
    HAPPrecondition(request);
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPPrecondition(dataSendStream);

    HAPLogInfo(&logObject, "%s", __func__);

    HAPError err;

    HAPAccessoryDiagnosticsConfig accessoryDiagnosticsConfig;
    HAPRawBufferZero(&accessoryDiagnosticsConfig, sizeof accessoryDiagnosticsConfig);

    HAPAssert(request->accessory->callbacks.diagnosticsConfig.getDiagnosticsConfig);

    err = request->accessory->callbacks.diagnosticsConfig.getDiagnosticsConfig(
            NULL, request->accessory, &accessoryDiagnosticsConfig, NULL);
    HAPAssert(!err);

    HAPDiagnosticsContext* dgContext = (HAPDiagnosticsContext*) (accessoryDiagnosticsConfig.diagnosticsContext);
    HAPAssert(dgContext);
    dgContext->dataSequenceNumber = 1;
    dgContext->isEOF = false;

    HAPPrecondition(activeContext == NULL);
    activeContext = dgContext;
    activeContext->eventType = kDiagnosticsDataEvent_Unknown;

    // Cancel abort diagnostics data timer if it exists
    if (activeContext->abortDiagnosticsDataTimer) {
        HAPLogInfo(&kHAPLog_Default, "%s: Deregistering abort diagnostics data timer.", __func__);
        HAPPlatformTimerDeregister(activeContext->abortDiagnosticsDataTimer);
        activeContext->abortDiagnosticsDataTimer = 0;
    }

    // Start abort diagnostics data timer
    AbortDiagnosticsDataTimer(dgContext);

    dgContext->prepareDataContext =
            HAPPlatformDiagnosticsPrepareData(&diagnosticsCallbacks, &(dgContext->dataSendOpenMetadata));
    if (dgContext->prepareDataContext == NULL) {
        HAPLog(&kHAPLog_Default, "Prepare diagnostics data failed.");
        if (activeContext->abortDiagnosticsDataTimer) {
            HAPLogInfo(&kHAPLog_Default, "%s: Deregistering abort diagnostics data timer.", __func__);
            HAPPlatformTimerDeregister(activeContext->abortDiagnosticsDataTimer);
            activeContext->abortDiagnosticsDataTimer = 0;
        }
        HAPDiagnosticsDataStreamContext* dsContext = &dgContext->dataStreamContext;
        HAPPrecondition(dsContext);
        HAPDataSendDataStreamProtocolCancelWithReason(
                dsContext->server,
                dsContext->dispatcher,
                dsContext->dataStream,
                dsContext->dataSendStream,
                kHAPDataSendDataStreamProtocolCancellationReason_UnexpectedFailure);
        return;
    }
}

static const HAPDataSendDataStreamProtocolStreamCallbacks dataSendStreamCallbacks = {
    .handleClose = HandleDataSendStreamClose,
    .handleOpen = HandleDataSendStreamOpen
};

/**
 * Data stream available handler.
 */
void HAPDiagnosticsHandleDataSendStreamAvailable(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataSendDataStreamProtocol* dataStreamProtocol,
        const HAPServiceRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolType type,
        HAPDataSendDataStreamProtocolOpenMetadata* _Nullable metadata,
        void* _Nullable inDataSendStreamCallbacks HAP_UNUSED,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol);
    HAPPrecondition(request);
    HAPPrecondition(request->session);
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPPrecondition(metadata);

    HAPLogInfo(&logObject, "%s", __func__);

    const char* protocolTypeDescription = NULL;
    switch (type) {
        case kHAPDataSendDataStreamProtocolType_Audio_Siri: {
            protocolTypeDescription = "Siri.";
            break;
        }
        case kHAPDataSendDataStreamProtocolType_IPCamera_Recording: {
            protocolTypeDescription = "IP Camera recording.";
            break;
        }
        case kHAPDataSendDataStreamProtocolType_Diagnostics_Snapshot: {
            protocolTypeDescription = "Diagnostics snapshot.";
            break;
        }
#if HAP_FEATURE_ENABLED(HAP_FEATURE_ACCESSORY_METRICS)
        case kHAPDataSendDataStreamProtocolType_Accessory_Metrics: {
            protocolTypeDescription = "Accessory metrics.";
            break;
        }
#endif
    }
    HAPAssert(protocolTypeDescription);

    HAPLogInfo(
            &kHAPLog_Default,
            "%s: Diagnostics, dataStream = %u, protocolType = %s",
            __func__,
            dataStream,
            protocolTypeDescription);

    if (type != kHAPDataSendDataStreamProtocolType_Diagnostics_Snapshot) {
        HAPLogError(&kHAPLog_Default, "Unsupported incoming \"dataSend\" stream type. Rejecting \"dataSend\" stream.");
        HAPDataSendDataStreamProtocolReject(
                server, dispatcher, dataStream, kHAPDataSendDataStreamProtocolRejectReason_Unsupported);
        return;
    }

    HAPError err;

    HAPAccessoryDiagnosticsConfig accessoryDiagnosticsConfig;
    HAPRawBufferZero(&accessoryDiagnosticsConfig, sizeof accessoryDiagnosticsConfig);

    HAPAssert(request->accessory->callbacks.diagnosticsConfig.getDiagnosticsConfig);

    err = request->accessory->callbacks.diagnosticsConfig.getDiagnosticsConfig(
            server, request->accessory, &accessoryDiagnosticsConfig, context);
    if (err) {
        HAPAssert(!err);
    }
    HAPDiagnosticsContext* dgContext = (HAPDiagnosticsContext*) (accessoryDiagnosticsConfig.diagnosticsContext);
    HAPPrecondition(dgContext);
    HAPDiagnosticsDataStreamContext* dsContext = &(dgContext->dataStreamContext);
    HAPPrecondition(dsContext);
    if (dsContext->server || dsContext->inProgress) {
        HAPLogError(&kHAPLog_Default, "\"dataSend\" stream already open. Rejecting \"dataSend\" stream.");
        HAPDataSendDataStreamProtocolReject(
                server, dispatcher, dataStream, kHAPDataSendDataStreamProtocolRejectReason_Busy);
        return;
    }

    // Validate metadata information.
    if (metadata->type == kHAPDataSendDataStreamProtocolType_Diagnostics_Snapshot) {
        bool isValidConfiguration = true;

        // Validate maxLogSize.
        if (metadata->_.diagnostics.snapshot.maxLogSize != 0) {
            if (metadata->_.diagnostics.snapshot.maxLogSize <
                        (kHAPDiagnostics_MaxDataSizeInMB * kHAPDiagnostics_MBToBytes) ||
                metadata->_.diagnostics.snapshot.maxLogSize > (1 * kHAPDiagnostics_GBToBytes) ||
                (accessoryDiagnosticsConfig.diagnosticsSnapshotOptions &
                 kHAPDiagnosticsSnapshotOptions_ConfigurableMaxLogSize) == 0) {
                isValidConfiguration = false;
                HAPLogError(&kHAPLog_Default, "maxLogSize validation failed.");
            }
        }

        // Validate snapshotType
        switch (metadata->_.diagnostics.snapshot.snapshotType) {
            case kHAPDiagnosticsSnapshotType_Manufacturer:
                HAPLogInfo(&kHAPLog_Default, "Manufacturer snapshot requested");
                break;
            case kHAPDiagnosticsSnapshotType_ADK:
                HAPLogInfo(&kHAPLog_Default, "ADK snapshot requested");
                break;
            default:
                HAPLogError(
                        &kHAPLog_Default,
                        "Unknown snapshot type: %d",
                        (int) metadata->_.diagnostics.snapshot.snapshotType);
                isValidConfiguration = false;
                break;
        }
        // Check if the accessory supports the requested snapshot type
        if ((metadata->_.diagnostics.snapshot.snapshotType & accessoryDiagnosticsConfig.diagnosticsSnapshotType) == 0) {
            isValidConfiguration = false;
            HAPLogError(&kHAPLog_Default, "snapshotType validation failed. Snapshot not supported.");
        }

        if (isValidConfiguration == false) {
            HAPDataSendDataStreamProtocolReject(
                    server, dispatcher, dataStream, kHAPDataSendDataStreamProtocolRejectReason_InvalidConfiguration);
            return;
        } else {
            dgContext->dataSendOpenMetadata = *metadata;
        }
    }

    // Mark dataStreamContext is in progress to prevent a concurrent diagnostics request.
    HAPDataSendDataStreamProtocolStream* dataSendStream = &(dgContext->dataSendStream);
    dgContext->dataStreamContext.inProgress = true;

    HAPDataSendDataStreamProtocolAccept(server, dispatcher, dataStream, dataSendStream, &dataSendStreamCallbacks);

    // Setup Data Stream context.
    HAPRawBufferZero(dsContext, sizeof *dsContext);
    dsContext->server = server;
    dsContext->dispatcher = dispatcher;
    dsContext->dataStream = dataStream;
    dsContext->dataSendStream = dataSendStream;
    dsContext->transportType = request->session->transportType;

    HAPLogInfo(&kHAPLog_Default, "Accepted \"dataSend\" stream: dataSendStream = %p.", (const void*) dataSendStream);
}

#endif // HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)
