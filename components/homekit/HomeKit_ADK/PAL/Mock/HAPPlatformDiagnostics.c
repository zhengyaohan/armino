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
// Copyright (C) 2021 Apple Inc. All Rights Reserved.

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

#if HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)

#include "HAPPlatformDiagnostics.h"
#include "HAPPlatformDiagnosticsLog.h"

#define kHAPPlatformDiagnostics_DequeueDataSizeBytes 100
static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "Diagnostics(Mock)" };
static DiagnosticsLog* _Nullable logContextPtr = NULL;
static HAPAccessoryDiagnosticsConfig* _Nullable accessoryDiagnosticsConfig = NULL;
static bool stopLoggingToBuffer = false;

/**
 * Initialize Diagnostics
 */
void HAPPlatformDiagnosticsInitialize(HAPAccessoryDiagnosticsConfig* _Nonnull config) {
    HAPPrecondition(config);
    HAPLogInfo(&logObject, "%s", __func__);
    accessoryDiagnosticsConfig = config;
}

/**
 * Deinitialize Diagnostics
 */
void HAPPlatformDiagnosticsDeinitialize(void) {
    HAPLogInfo(&logObject, "%s", __func__);
    if (accessoryDiagnosticsConfig == NULL) {
        return;
    }
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
    HAPPrecondition(folderName);
    HAPPrecondition(logFileName);
    HAPPrecondition(maxLogFileSizeBytes > 0);
    HAPPrecondition(logContext);
    char logFilePath[PATH_MAX];

    HAPLogInfo(&logObject, "%s", __func__);
    HAPError err;
    if (maxLogFileSizeBytes > (kHAPDiagnostics_MaxDataSizeInMB * kHAPDiagnostics_MBToBytes)) {
        HAPLogError(
                &kHAPLog_Default, "Maximum log file size cannot be more than %zu MB", kHAPDiagnostics_MaxDataSizeInMB);
        return kHAPError_InvalidData;
    }
    if (logContextPtr != NULL) {
        HAPLogError(&kHAPLog_Default, "Diagnostics HAP log capture in progress. Cannot start another HAP log capture.");
        return kHAPError_Busy;
    }
    logContextPtr = logContext;
    HAPRawBufferZero(logContextPtr, sizeof *logContextPtr);

    // Check if folder exists. If not create one
    struct stat st_buf = { 0 };
    if (stat(folderName, &st_buf) != 0) {
        if (mkdir(folderName, kHAPPlatformDiagnosticsFolderPermissions) != 0) {
            HAPLogError(&kHAPLog_Default, "Failed to create folder.");
            return kHAPError_InvalidData;
        }
    }
    err = HAPStringWithFormat(logFilePath, sizeof logFilePath, "%s/%s", folderName, logFileName);
    HAPAssert(!err);

    err = HAPPlatformSetupDiagnosticsLogFiles(
            logFilePath, kHAPPlatformDiagnosticsNumLogFiles, maxLogFileSizeBytes, logContext);
    HAPAssert(!err);

    return kHAPError_None;
}

/**
 * Writes log messages to file.
 */
HAP_PRINTFLIKE(2, 3)
void HAPPlatformDiagnosticsWriteToFile(bool disableLogRotation, const char* _Nullable format, ...) {
    if (logContextPtr == NULL || logContextPtr->logFileStream == NULL) {
        return;
    }

    char buffer[kHAPPlatformDiagnosticsLogMessageMaxBytes];
    HAPRawBufferZero(buffer, sizeof buffer);

    if (disableLogRotation == true) {
        va_list args;
        va_start(args, format);
        vfprintf(logContextPtr->logFileStream, format, args);
        va_end(args);
        fflush(logContextPtr->logFileStream);
        return;
    }

    va_list args;
    va_start(args, format);
    HAPError err = HAPStringWithFormatAndArguments(buffer, kHAPPlatformDiagnosticsLogMessageMaxBytes, format, args);
    HAPAssert(!err);
    va_end(args);

    if (!err) {
        int bytes = HAPStringGetNumBytes(buffer);
        err = HAPPlatformWriteToDiagnosticsLogFile(logContextPtr, buffer, (size_t) bytes);
        HAPAssert(!err);
    }
    return;
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

HAP_PRINTFLIKE(1, 2)
void HAPPlatformDiagnosticsWriteToConfigLog(const char* _Nullable format, ...) {
    char buffer[kHAPPlatformDiagnosticsLogMessageMaxBytes];
    HAPRawBufferZero(buffer, sizeof buffer);

    va_list args;
    va_start(args, format);
    HAPError err = HAPStringWithFormatAndArguments(buffer, kHAPPlatformDiagnosticsLogMessageMaxBytes, format, args);
    va_end(args);

    if (err) {
        return;
    }

    FILE* file = fopen(kHAPPlatformDiagnosticsConfigFile, "w");
    if (!file) {
        return;
    }

    fwrite(buffer, 1, HAPStringGetNumBytes(buffer), file);
    fprintf(file, "\n");
    fclose(file);
}

/**
 * Stop capturing logs to Diagnostics log file.
 */
void HAPPlatformDiagnosticsStopLogCaptureToFile(DiagnosticsLog* _Nonnull logContext) {
    HAPPrecondition(logContext);

    HAPLogInfo(&logObject, "%s", __func__);
    if (logContextPtr != logContext) {
        HAPLogError(&kHAPLog_Default, "Unknown context. Ignoring command.");
    }

    if (logContextPtr) {
        HAPError err = HAPPlatformCloseDiagnosticsLogFile(logContext);
        HAPAssert(!err);
        logContextPtr = NULL;
    }
    return;
}

/**
 * Get the bytes to upload via HDS from Diagnostics logs.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformDiagnosticsGetBytesToUpload(
        uint8_t* _Nonnull buf,
        size_t bufSize HAP_UNUSED,
        size_t* _Nonnull bytesCopied,
        bool* _Nonnull isEOF,
        void* _Nonnull prepareDiagnosticsDataContext) {
    HAPPrecondition(prepareDiagnosticsDataContext);
    HAPPrecondition(buf);
    HAPPrecondition(isEOF);
    HAPPrecondition(bytesCopied);

    HAPLogError(&logObject, "[NYI] %s.", __func__);
    HAPFatalError();
}

/**
 * Prepares the diagnostics data in a separate thread or process.
 * Returns from this function immediately.
 */
void* _Nullable HAPPlatformDiagnosticsPrepareData(
        const HAPDiagnosticsCallbacks* _Nonnull diagnosticsCallbacks,
        const HAPDataSendDataStreamProtocolOpenMetadata* _Nonnull metadata) {
    HAPPrecondition(metadata);
    HAPPrecondition(diagnosticsCallbacks);
    HAPPrecondition(diagnosticsCallbacks->handleDiagnosticsDataReady);
    HAPPrecondition(diagnosticsCallbacks->handleDiagnosticsDataCancel);

    HAPLogError(&logObject, "[NYI] %s.", __func__);
    HAPFatalError();
}

/**
 * Called by HAP when diagnostics data transfer to controller is complete.
 */
void HAPPlatformDiagnosticsDataTransferComplete(void* _Nullable prepareDiagnosticsDataContext) {
    HAPPrecondition(prepareDiagnosticsDataContext);
    HAPLogError(&logObject, "[NYI] %s.", __func__);
    HAPFatalError();
}

/**
 * Called by HAP when diagnostics data transfer to controller is aborted.
 */
void HAPPlatformDiagnosticsAbort(void* _Nullable prepareDiagnosticsDataContext) {
    HAPPrecondition(prepareDiagnosticsDataContext);
    HAPLogError(&logObject, "[NYI] %s.", __func__);
    HAPFatalError();
}

#endif // HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)
