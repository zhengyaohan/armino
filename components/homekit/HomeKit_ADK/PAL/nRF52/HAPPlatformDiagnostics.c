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

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "HAP.h"

#include "HAPPlatformLog+Init.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)

#include "HAPPlatformDiagnostics.h"
#include "HAPPlatformDiagnosticsLog.h"

#define kHAPPlatformDiagnostics_DequeueDataSizeBytes 100
static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "Diagnostics" };
static HAPAccessoryDiagnosticsConfig* _Nullable accessoryDiagnosticsConfig = NULL;
static HAPCharacteristicValue_SelectedDiagnosticsModes currentSelectedDiagnosticModes =
        kHAPCharacteristicValue_SelectedDiagnosticsModes_None;
static size_t bytesToTransfer = 0;
static bool stopLoggingToBuffer = false;

/**
 * Diagnostics data collection state
 */
HAP_ENUM_BEGIN(uint8_t, DiagnosticsDataCollectionState) {
    kDiagnosticsDataCollection_Idle,
    kDiagnosticsDataCollection_InProgress,
    kDiagnosticsDataCollection_Ready,
    kDiagnosticsDataCollection_Error
} HAP_ENUM_END(uint8_t, DiagnosticsDataCollectionState);

/**
 * Prepare diagnostics data context structure
 */
typedef struct {
    /** Data collection state. */
    DiagnosticsDataCollectionState dataState;

    /** Flag to abort diagnostics data collection. */
    bool didAbortPrepareDiagnosticsData;

    /** Callbacks to inform prepare data operation status to HAP. */
    const HAPDiagnosticsCallbacks* _Nonnull callbacks;

    /** Validated metadata passed to HDS Open request. */
    const HAPDataSendDataStreamProtocolOpenMetadata* _Nonnull metadata;
} PrepareDiagnosticsDataContext;

/**
 * Diagnostics log capture operation
 */
HAP_ENUM_BEGIN(uint8_t, DiagnosticsLogOperation) {
    kDiagnosticsLogOperation_Start,
    kDiagnosticsLogOperation_Stop
} HAP_ENUM_END(uint8_t, DiagnosticsLogOperation);

void SetupBTDiagnostics(DiagnosticsLogOperation operation) {
    HAPLogInfo(&logObject, "%s, operation %d", __func__, (int) operation);

    if (operation == kDiagnosticsLogOperation_Start) {
        // VENDOR-TODO: Start Bluetooth diagnostics capture in folder kHAPPlatformDiagnosticsBTLogsFolder
    }

    if (operation == kDiagnosticsLogOperation_Stop) {
        // VENDOR-TODO: Stop Bluetooth diagnostics capture
    }
}

/**
 * Initialize Diagnostics
 */
void HAPPlatformDiagnosticsInitialize(HAPAccessoryDiagnosticsConfig* _Nonnull config) {
    HAPPrecondition(config);
    HAPPrecondition(config->diagnosticsContext);
    HAPDiagnosticsContext* dgContext = (HAPDiagnosticsContext*) (config->diagnosticsContext);
    HAPLogInfo(&logObject, "%s", __func__);
    bool isSettingsUpdateNeeded = false;

    if (dgContext->diagnosticsSelectedMode == NULL) {
        currentSelectedDiagnosticModes = kHAPCharacteristicValue_SelectedDiagnosticsModes_None;
    } else {
        if (currentSelectedDiagnosticModes != *(dgContext->diagnosticsSelectedMode)) {
            currentSelectedDiagnosticModes = *(dgContext->diagnosticsSelectedMode);
        }
    }

    if (accessoryDiagnosticsConfig == NULL) {
        accessoryDiagnosticsConfig = config;
        HAPLogInfo(
                &kHAPLog_Default,
                "Current selected diagnostic modes [%u]",
                (unsigned int) currentSelectedDiagnosticModes);

        if (currentSelectedDiagnosticModes & kHAPCharacteristicValue_SelectedDiagnosticsModes_VerboseLogging) {
            SetupBTDiagnostics(kDiagnosticsLogOperation_Start);
        } else {
            SetupBTDiagnostics(kDiagnosticsLogOperation_Stop);
        }
    } else {
        // Diagnostics is already initialized
        if (isSettingsUpdateNeeded == true) {
            HAPLogInfo(
                    &kHAPLog_Default,
                    "Updating current selected diagnostic modes [%u]",
                    (unsigned int) currentSelectedDiagnosticModes);

            if (currentSelectedDiagnosticModes & kHAPCharacteristicValue_SelectedDiagnosticsModes_VerboseLogging) {
                SetupBTDiagnostics(kDiagnosticsLogOperation_Start);
            } else {
                SetupBTDiagnostics(kDiagnosticsLogOperation_Stop);
            }
        }
    }

    if (accessoryDiagnosticsConfig->diagnosticsSnapshotFormat != kHAPDiagnosticsSnapshotFormat_Text) {
        HAPLogError(
                &kHAPLog_Default,
                "Diagnostics snapshot format of type [%u] is not supported on this platform.",
                (unsigned int) accessoryDiagnosticsConfig->diagnosticsSnapshotFormat);
        return;
    }
    if (dgContext->logBuffer == NULL || dgContext->logBufferSizeBytes <= 0) {
        HAPLogError(&kHAPLog_Default, "Invalid log buffer. Disabling diagnostics capture.");
        return;
    }
    if (dgContext->logBufferSizeBytes < kHAPPlatformDiagnostics_DequeueDataSizeBytes) {
        HAPLogError(
                &kHAPLog_Default,
                "Log buffer too small. Must be at least [%u] bytes",
                (unsigned int) kHAPPlatformDiagnostics_DequeueDataSizeBytes);
        return;
    }

    HAPCircularQueueCreate(&(dgContext->logBufferQueue), dgContext->logBuffer, dgContext->logBufferSizeBytes);
}

/**
 * Deinitialize Diagnostics
 */
void HAPPlatformDiagnosticsDeinitialize(void) {
    HAPLogInfo(&logObject, "%s", __func__);

    SetupBTDiagnostics(kDiagnosticsLogOperation_Stop);
    if (accessoryDiagnosticsConfig == NULL) {
        return;
    }
    accessoryDiagnosticsConfig = NULL;
    currentSelectedDiagnosticModes = kHAPCharacteristicValue_SelectedDiagnosticsModes_None;
}

/**
 * Updates data collection state
 */
static void UpdateDiagnosticsDataCollectionState(
        PrepareDiagnosticsDataContext* _Nonnull prepareContext,
        DiagnosticsDataCollectionState state) {
    HAPPrecondition(prepareContext);
    prepareContext->dataState = state;
    HAPLogInfo(&logObject, "%s: Updated data collection state: %u", __func__, prepareContext->dataState);
    return;
}

/**
 * Checks if data collection was aborted
 */
static bool DidAbortPrepareDiagnosticsData(PrepareDiagnosticsDataContext* _Nullable prepareContext) {
    HAPPrecondition(prepareContext);

    bool isAbort = false;
    if (prepareContext->didAbortPrepareDiagnosticsData == true) {
        free(prepareContext);
        prepareContext = NULL;
        isAbort = true;
    } else {
        isAbort = false;
    }
    return isAbort;
}

/**
 * Start capturing logs to Diagnostics log file.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformDiagnosticsStartLogCaptureToFile(
        const char* _Nonnull folderName,
        const char* _Nonnull logFileName,
        const size_t maxLogFileSizeBytes,
        DiagnosticsLog* _Nonnull logContext) {
    HAPLogError(&logObject, "%s: This feature is not supported on this platform.", __func__);
    return kHAPError_Unknown;
}

/**
 * Writes log messages to file.
 */
HAP_PRINTFLIKE(2, 3)
void HAPPlatformDiagnosticsWriteToFile(bool disableLogRotation, const char* _Nullable format, ...) {
    /* no-op */
}

void HAPPlatformDiagnosticsClearCrashLog(void) {
    /* no-op */
}

HAP_PRINTFLIKE(1, 2)
void HAPPlatformDiagnosticsWriteToCrashLog(const char* _Nullable format, ...) {
    /* no-op */
}

/**
 * Writes log messages to file.
 */
HAP_PRINTFLIKE(1, 2)
void HAPPlatformDiagnosticsWriteToLogBuffer(const char* _Nullable format, ...) {
    if (accessoryDiagnosticsConfig == NULL || stopLoggingToBuffer == true) {
        return;
    }
    HAPDiagnosticsContext* dgContext = (HAPDiagnosticsContext*) (accessoryDiagnosticsConfig->diagnosticsContext);
    if (dgContext == NULL) {
        return;
    }

    char buffer[kHAPPlatformDiagnosticsLogMessageMaxBytes];
    HAPRawBufferZero(buffer, sizeof buffer);

    va_list args;
    va_start(args, format);
    HAPError err = HAPStringWithFormatAndArguments(buffer, kHAPPlatformDiagnosticsLogMessageMaxBytes, format, args);
    HAPAssert(!err);
    va_end(args);

    if (!err) {
        size_t bytes = (size_t) HAPStringGetNumBytes(buffer);
        if (bytes > dgContext->logBufferQueue.totalSize) {
            return;
        }
        while (bytes > HAPCircularQueueGetRemainingSpace(&(dgContext->logBufferQueue))) {
            uint8_t dequeueData[kHAPPlatformDiagnostics_DequeueDataSizeBytes];
            size_t dequeSize = sizeof dequeueData;
            if (dequeSize > dgContext->logBufferQueue.usedSpace) {
                dequeSize = dgContext->logBufferQueue.usedSpace;
            }
            err = HAPCircularQueueDequeue(&(dgContext->logBufferQueue), dequeueData, dequeSize);
            if (err) {
                fprintf(stderr, "%s: Dequeuing log bytes failed\n", __func__);
                return;
            }
        }

        err = HAPCircularQueueEnqueue(&(dgContext->logBufferQueue), (uint8_t*) buffer, bytes);
        if (err) {
            fprintf(stderr, "%s: Enqueuing log bytes failed.\n", __func__);
            return;
        }
    }
    return;
}

void HAPPlatformDiagnosticsFlushLog() {
    /* no-op */
}

HAP_PRINTFLIKE(1, 2)
void HAPPlatformDiagnosticsWriteToConfigLog(const char* _Nullable format, ...) {
    /* no-op */
}

/**
 * Stop capturing logs to Diagnostics log file.
 */
void HAPPlatformDiagnosticsStopLogCaptureToFile(DiagnosticsLog* _Nonnull logContext) {
    HAPLogError(&logObject, "%s: This feature is not supported on this platform.", __func__);
}

/**
 * Get the bytes to upload via HDS from Diagnostics logs.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformDiagnosticsGetBytesToUpload(
        uint8_t* _Nonnull buf,
        size_t bufSize,
        size_t* _Nonnull bytesCopied,
        bool* _Nonnull isEOF,
        void* _Nonnull prepareDiagnosticsDataContext) {
    HAPPrecondition(prepareDiagnosticsDataContext);
    HAPPrecondition(buf);
    HAPPrecondition(isEOF);
    HAPPrecondition(bytesCopied);
    HAPPrecondition(accessoryDiagnosticsConfig);

    HAPError err;
    HAPDiagnosticsContext* dgContext = (HAPDiagnosticsContext*) (accessoryDiagnosticsConfig->diagnosticsContext);
    HAPLogInfo(&logObject, "%s", __func__);

    if (accessoryDiagnosticsConfig->diagnosticsSnapshotFormat == kHAPDiagnosticsSnapshotFormat_Zip) {
        HAPLogError(&kHAPLog_Default, "Diagnostics format zip is not supported on this platform.");
        return kHAPError_InvalidData;
    } else if (accessoryDiagnosticsConfig->diagnosticsSnapshotFormat == kHAPDiagnosticsSnapshotFormat_Text) {
        HAPLogDebug(&kHAPLog_Default, "Diagnostics format text requested.");
    } else {
        HAPLogError(&kHAPLog_Default, "Unknown diagnostics format.");
        return kHAPError_Unknown;
    }

    if (bufSize < dgContext->logBufferQueue.usedSpace) {
        err = HAPCircularQueueDequeue(&(dgContext->logBufferQueue), buf, bufSize);
        HAPAssert(!err);
        if (err) {
            HAPLogError(&kHAPLog_Default, "%s: Dequeuing log bytes failed", __func__);
            return kHAPError_Unknown;
        }
        *bytesCopied = bufSize;

    } else {
        *bytesCopied = dgContext->logBufferQueue.usedSpace;
        err = HAPCircularQueueDequeue(&(dgContext->logBufferQueue), buf, *bytesCopied);
        HAPAssert(!err);
        if (err) {
            HAPLogError(&kHAPLog_Default, "%s: Dequeuing log bytes failed", __func__);
            return kHAPError_Unknown;
        }
    }

    bytesToTransfer -= *bytesCopied;
    if (bytesToTransfer == 0) {
        HAPLogDebug(&kHAPLog_Default, "%s: Full log buffer copied.", __func__);
        *isEOF = true;
        stopLoggingToBuffer = false;
    } else {
        HAPLogDebug(
                &kHAPLog_Default,
                "%s: [%u] bytes copied, [%u] bytes remaining.",
                __func__,
                (unsigned int) (*bytesCopied),
                (unsigned int) bytesToTransfer);
    }

    return kHAPError_None;
}

static char signedTimestamp[32];
static HAPDataSendDataStreamProtocolPacketDiagnosticsMetadataKeyValuePairs diagnosticsURLKeyValue[] = {
    { .key = "signed-timestamp", .value = signedTimestamp },
    { .key = "MAC", .value = "some_mac_address" }
};

static HAPDiagnosticsUrlParameters diagnosticsURLParameters = { .numUrlParameterPairs = 2,
                                                                .urlParameterPairs = diagnosticsURLKeyValue };

/**
 * Collect diagnostics data
 */
static void* _Nullable CollectDiagnosticData(void* _Nonnull context) {
    HAPPrecondition(context);
    HAPPrecondition(accessoryDiagnosticsConfig);
    const HAPDiagnosticsContext* dgContext = accessoryDiagnosticsConfig->diagnosticsContext;
    HAPPrecondition(dgContext);

    HAPLogInfo(&logObject, "%s: Diagnostics data collection started", __func__);

    PrepareDiagnosticsDataContext* prepareContext = (PrepareDiagnosticsDataContext*) context;
    UpdateDiagnosticsDataCollectionState(prepareContext, kDiagnosticsDataCollection_InProgress);
    HAPError err;

    // Abort if requested by HAP
    if (DidAbortPrepareDiagnosticsData(prepareContext) == true) {
        goto exit;
    }

    if (accessoryDiagnosticsConfig->diagnosticsSnapshotFormat == kHAPDiagnosticsSnapshotFormat_Zip) {
        HAPLogError(&kHAPLog_Default, "Diagnostics format zip is not supported on this platform.");
        UpdateDiagnosticsDataCollectionState(prepareContext, kDiagnosticsDataCollection_Error);
        err = prepareContext->callbacks->handleDiagnosticsDataCancel();
        HAPAssert(!err);
        goto exit;
    } else if (accessoryDiagnosticsConfig->diagnosticsSnapshotFormat == kHAPDiagnosticsSnapshotFormat_Text) {
        HAPLogDebug(&kHAPLog_Default, "Diagnostics format text requested.");
    } else {
        HAPLogError(&kHAPLog_Default, "Unknown diagnostics format.");
        UpdateDiagnosticsDataCollectionState(prepareContext, kDiagnosticsDataCollection_Error);
        err = prepareContext->callbacks->handleDiagnosticsDataCancel();
        HAPAssert(!err);
        goto exit;
    }

    if (accessoryDiagnosticsConfig->diagnosticsSnapshotFormat == kHAPDiagnosticsSnapshotFormat_Text) {
        char configurationBytes[kHAPDiagnostics_1KiloBytes];
        HAPRawBufferZero(configurationBytes, sizeof configurationBytes);

        HAPAccessoryServerGetAccessoryConfigurationString(
                dgContext->hapPlatform, dgContext->accessory, configurationBytes, sizeof configurationBytes);
        HAPPlatformDiagnosticsWriteToLogBuffer("%s", configurationBytes);
    }

    // Override max log size if requested in metadata
    uint64_t maxLogSizeBytes = kHAPDiagnostics_MaxDataSizeInMB * kHAPDiagnostics_MBToBytes;
    if (prepareContext->metadata->_.diagnostics.snapshot.maxLogSize > maxLogSizeBytes) {
        maxLogSizeBytes = prepareContext->metadata->_.diagnostics.snapshot.maxLogSize;
    }

    bytesToTransfer = dgContext->logBufferQueue.usedSpace;
    stopLoggingToBuffer = true;

    // Check the send buffer size
    if (bytesToTransfer > maxLogSizeBytes) {
        HAPLogError(
                &logObject,
                "Diagnostics log buffer size = %lu bytes. Exceeded maximum allowed size of %lu bytes.",
                (unsigned long) bytesToTransfer,
                (unsigned long) maxLogSizeBytes);
        UpdateDiagnosticsDataCollectionState(prepareContext, kDiagnosticsDataCollection_Error);
        err = prepareContext->callbacks->handleDiagnosticsDataCancel();
        HAPAssert(!err);
        goto exit;
    }

    UpdateDiagnosticsDataCollectionState(prepareContext, kDiagnosticsDataCollection_Ready);

    if (prepareContext->metadata->_.diagnostics.snapshot.snapshotType == kHAPDiagnosticsSnapshotType_Manufacturer) {
        // VENDOR-TODO: Provide the URL parameters and timestamp here
        HAPTime now = HAPPlatformClockGetCurrent();
        HAPLogInfo(&logObject, "Time-stamp: %lu", (unsigned long) now);
        err = HAPStringWithFormat(signedTimestamp, sizeof signedTimestamp, "%lu", (unsigned long) now);
        HAPAssert(!err);
        err = prepareContext->callbacks->handleDiagnosticsDataReady(bytesToTransfer, &diagnosticsURLParameters);
    } else if (prepareContext->metadata->_.diagnostics.snapshot.snapshotType == kHAPDiagnosticsSnapshotType_ADK) {
        err = prepareContext->callbacks->handleDiagnosticsDataReady(bytesToTransfer, NULL);
    } else {
        HAPLogError(&logObject, "%s: Unknown diagnostics snapshot type.", __func__);
        err = kHAPError_InvalidData;
    }
    HAPAssert(!err);
    HAPLogInfo(&logObject, "%s: Exiting diagnostic data collection.", __func__);
    return NULL;

exit:
    HAPLogInfo(&logObject, "%s: Exiting diagnostic data collection.", __func__);
    return NULL;
}

/**
 * Prepares the diagnostics data.
 */
void* _Nullable HAPPlatformDiagnosticsPrepareData(
        const HAPDiagnosticsCallbacks* _Nonnull diagnosticsCallbacks,
        const HAPDataSendDataStreamProtocolOpenMetadata* _Nonnull metadata) {
    HAPPrecondition(diagnosticsCallbacks);
    HAPPrecondition(diagnosticsCallbacks->handleDiagnosticsDataReady);
    HAPPrecondition(diagnosticsCallbacks->handleDiagnosticsDataCancel);
    HAPPrecondition(metadata);

    PrepareDiagnosticsDataContext* prepareContext =
            (PrepareDiagnosticsDataContext*) malloc(sizeof(PrepareDiagnosticsDataContext));
    HAPPrecondition(prepareContext);
    HAPRawBufferZero(prepareContext, sizeof(PrepareDiagnosticsDataContext));

    prepareContext->dataState = kDiagnosticsDataCollection_Idle;
    prepareContext->didAbortPrepareDiagnosticsData = false;
    prepareContext->callbacks = diagnosticsCallbacks;
    prepareContext->metadata = metadata;

    CollectDiagnosticData((void*) prepareContext);
    return prepareContext;
}

/**
 * Called by HAP when diagnostics data transfer to controller is complete.
 */
void HAPPlatformDiagnosticsDataTransferComplete(void* _Nullable prepareDiagnosticsDataContext) {
    HAPPrecondition(prepareDiagnosticsDataContext);
    HAPLogInfo(&logObject, "Diagnostics data transfer completed");
    free(prepareDiagnosticsDataContext);
    prepareDiagnosticsDataContext = NULL;
    stopLoggingToBuffer = false;
    bytesToTransfer = 0;
    return;
}

/**
 * Called by HAP when diagnostics data transfer to controller is aborted.
 */
void HAPPlatformDiagnosticsAbort(void* _Nullable prepareDiagnosticsDataContext) {
    HAPPrecondition(prepareDiagnosticsDataContext);
    PrepareDiagnosticsDataContext* prepareContext = (PrepareDiagnosticsDataContext*) prepareDiagnosticsDataContext;
    HAPLogInfo(&logObject, "Diagnostics data transfer aborted");
    if (prepareContext->dataState == kDiagnosticsDataCollection_Ready ||
        prepareContext->dataState == kDiagnosticsDataCollection_Error) {
        free(prepareContext);
        prepareContext = NULL;
    } else {
        prepareContext->didAbortPrepareDiagnosticsData = true;
    }
    stopLoggingToBuffer = false;
    bytesToTransfer = 0;
    return;
}

#endif // HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)
