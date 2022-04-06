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
#include <pthread.h>
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
#include "HAPPlatformFileManager.h"
#include "HAPPlatformLog+Init.h"

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "Diagnostics" };
static DiagnosticsLog* _Nullable logContextPtr = NULL;
static pthread_mutex_t diagnosticsDataCollectionLock = PTHREAD_MUTEX_INITIALIZER;
static HAPAccessoryDiagnosticsConfig* _Nullable accessoryDiagnosticsConfig = NULL;
static HAPCharacteristicValue_SelectedDiagnosticsModes currentSelectedDiagnosticModes =
        kHAPCharacteristicValue_SelectedDiagnosticsModes_None;
#define kDiagnosticsADKLogSizeTolerance ((float) 0.8)
#define kMaxNumCrashLogs                4
#define kTempCrashLogFile               kHAPPlatformDiagnosticsCrashLogFile ".tmp"

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

    /** Diagnostics Data file handle. */
    FILE* _Nullable fileHandle;

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
            isSettingsUpdateNeeded = true;
        }
    }

    if (accessoryDiagnosticsConfig == NULL) {
        accessoryDiagnosticsConfig = config;
        HAPLogInfo(&kHAPLog_Default, "Current selected diagnostic modes [%u]", currentSelectedDiagnosticModes);
        HAPError err;

        // Create diagnostics folder if it does not exist
        err = HAPPlatformFileManagerCreateDirectory(kHAPPlatformDiagnosticsFolder);
        HAPAssert(!err);
        if (err) {
            HAPLogError(&kHAPLog_Default, "Failed to create folder [%s]", kHAPPlatformDiagnosticsFolder);
        }
    } else {
        // Diagnostics is already initialized
        if (isSettingsUpdateNeeded == true) {
            HAPLogInfo(
                    &kHAPLog_Default,
                    "Updating current selected diagnostic modes [%u]",
                    currentSelectedDiagnosticModes);

            if (accessoryDiagnosticsConfig->diagnosticsSnapshotType & kHAPDiagnosticsSnapshotType_ADK) {
            }
        }
    }
}

/**
 * Deinitialize Diagnostics
 */
void HAPPlatformDiagnosticsDeinitialize(void) {
    HAPLogInfo(&logObject, "%s", __func__);
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
    pthread_mutex_lock(&diagnosticsDataCollectionLock);
    prepareContext->dataState = state;
    HAPLogInfo(&logObject, "%s: Updated data collection state: %u", __func__, prepareContext->dataState);
    pthread_mutex_unlock(&diagnosticsDataCollectionLock);
    return;
}

/**
 * Checks if data collection was aborted
 */
static bool DidAbortPrepareDiagnosticsData(PrepareDiagnosticsDataContext* _Nullable prepareContext) {
    HAPPrecondition(prepareContext);

    bool isAbort = false;
    pthread_mutex_lock(&diagnosticsDataCollectionLock);
    if (prepareContext->didAbortPrepareDiagnosticsData == true) {
        free(prepareContext);
        prepareContext = NULL;
        isAbort = true;
    } else {
        isAbort = false;
    }
    pthread_mutex_unlock(&diagnosticsDataCollectionLock);
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
    HAPPrecondition(folderName);
    HAPPrecondition(logFileName);
    HAPPrecondition(maxLogFileSizeBytes > 0);
    HAPPrecondition(logContext);
    char logFilePath[PATH_MAX];

    HAPLogInfo(&logObject, "%s", __func__);
    HAPError err;
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
    if (err) {
        return kHAPError_Unknown;
    }

    err = HAPPlatformSetupDiagnosticsLogFiles(
            logFilePath, kHAPPlatformDiagnosticsNumLogFiles, maxLogFileSizeBytes, logContext);
    HAPAssert(!err);
    if (err) {
        return kHAPError_Unknown;
    }

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
        if (err) {
            fprintf(stderr, "%s: Write to diagnostics log file failed", __func__);
            return;
        }
    }
    return;
}

// Check if the next line is a crash log separator line.
// endOfFile is set to true if file ended before a new line character.
static bool IsNextLineCrashLogSeparator(FILE* _Nonnull file, bool* _Nonnull endOfFile) {
    int c = fgetc(file);
    *endOfFile = false;
    if (c == EOF) {
        *endOfFile = true;
        return false;
    }
    if (c == '=') {
        do {
            c = fgetc(file);
        } while (c == '=');
        if (c == EOF) {
            *endOfFile = true;
            return false;
        }
        if (c == '\n') {
            return true;
        }
    }
    return false;
}

// Move the file position to the beginning of a next line
static void MoveToNextLine(FILE* _Nonnull file, bool* _Nonnull endOfFile) {
    *endOfFile = NULL;
    for (;;) {
        int c = getc(file);
        if (c == EOF) {
            *endOfFile = true;
            return;
        }
        if (c == '\n') {
            return;
        }
    }
}

void HAPPlatformDiagnosticsClearCrashLog(void) {
    FILE* file = fopen(kHAPPlatformDiagnosticsCrashLogFile, "rt");
    if (!file) {
        return;
    }
    // Count number of log entries
    size_t logCount = 0;
    long nextLogStartOffset = 0;

    bool endOfFile;

    // Skip the first line
    MoveToNextLine(file, &endOfFile);

    // Look for the next log entry
    if (!endOfFile) {
        logCount++;
        for (;;) {
            long potentialLogStartOffset = ftell(file);
            if (IsNextLineCrashLogSeparator(file, &endOfFile)) {
                nextLogStartOffset = potentialLogStartOffset;
                logCount++;
                break;
            }
            if (endOfFile) {
                break;
            }
        }
    }

    // Count log entries
    while (!endOfFile) {
        if (IsNextLineCrashLogSeparator(file, &endOfFile)) {
            logCount++;
        }
    }

    static const char separator[] = "================\n";

    if (logCount >= kMaxNumCrashLogs) {
        // Maximum number reached. Cut off the oldest log entry
        FILE* newFile = fopen(kTempCrashLogFile, "wt");
        if (!newFile) {
            // New file could not be created. Just remove the current file.
            remove(kHAPPlatformDiagnosticsCrashLogFile);
            return;
        }
        fseek(file, nextLogStartOffset, SEEK_SET);
        for (;;) {
            int c = fgetc(file);
            if (c == EOF) {
                break;
            }
            fputc(c, newFile);
        }
        fclose(file);
        fwrite(separator, sizeof separator - 1, 1, newFile);
        fflush(newFile);
        fclose(newFile);
        remove(kHAPPlatformDiagnosticsCrashLogFile);
        rename(kTempCrashLogFile, kHAPPlatformDiagnosticsCrashLogFile);
    } else {
        // Can add a new log without removing any entries
        fclose(file);
        file = fopen(kHAPPlatformDiagnosticsCrashLogFile, "at");
        if (!file) {
            return;
        }
        fwrite(separator, sizeof separator - 1, 1, file);
        fflush(file);
        fclose(file);
    }
}

HAP_PRINTFLIKE(1, 2)
void HAPPlatformDiagnosticsWriteToCrashLog(const char* _Nullable format, ...) {
    char buffer[kHAPPlatformDiagnosticsLogMessageMaxBytes];
    HAPRawBufferZero(buffer, sizeof buffer);

    // Note that calling another log or error could cause recursive crash logging.
    // Hence errors in this function should be silently discarded.

    va_list args;
    va_start(args, format);
    HAPError err = HAPStringWithFormatAndArguments(buffer, kHAPPlatformDiagnosticsLogMessageMaxBytes, format, args);
    va_end(args);

    if (err) {
        return;
    }

    FILE* file = fopen(kHAPPlatformDiagnosticsCrashLogFile, "at");
    if (!file) {
        return;
    }

    fwrite(buffer, 1, HAPStringGetNumBytes(buffer), file);
    fclose(file);
}

void HAPPlatformDiagnosticsFlushLog() {
    // Flush data to file
    if (logContextPtr != NULL && logContextPtr->logFileStream != NULL) {
        fflush(logContextPtr->logFileStream);
    }
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
        size_t bufSize,
        size_t* _Nonnull bytesCopied,
        bool* _Nonnull isEOF,
        void* _Nonnull prepareDiagnosticsDataContext) {
    HAPPrecondition(prepareDiagnosticsDataContext);
    HAPPrecondition(buf);
    HAPPrecondition(isEOF);
    HAPPrecondition(bytesCopied);
    HAPPrecondition(accessoryDiagnosticsConfig);

    PrepareDiagnosticsDataContext* prepareContext = (PrepareDiagnosticsDataContext*) prepareDiagnosticsDataContext;
    HAPLogInfo(&logObject, "%s", __func__);

    char* sendFileName = NULL;
    if (accessoryDiagnosticsConfig->diagnosticsSnapshotFormat == kHAPDiagnosticsSnapshotFormat_Zip) {
        sendFileName = kHAPPlatformDiagnosticsZipFileName;
    } else if (accessoryDiagnosticsConfig->diagnosticsSnapshotFormat == kHAPDiagnosticsSnapshotFormat_Text) {
        sendFileName = kHAPPlatformDiagnosticsTextFileName;
    } else {
        HAPLogError(&kHAPLog_Default, "Unknown diagnostics format.");
        return kHAPError_Unknown;
    }

    if (prepareContext->fileHandle == NULL) {
        prepareContext->fileHandle = fopen(sendFileName, "rb");
        if (!(prepareContext->fileHandle)) {
            HAPLogError(&logObject, "Failed to open diagnostics send file: %d.", errno);
            return kHAPError_Unknown;
        }
    }

    *bytesCopied = fread(buf, sizeof(uint8_t), bufSize, prepareContext->fileHandle);
    if (*bytesCopied != bufSize) {
        if (feof((FILE*) prepareContext->fileHandle)) {
            HAPLogInfo(&logObject, "End of file reached");
            *isEOF = true;
            fclose(prepareContext->fileHandle);
            prepareContext->fileHandle = NULL;
        } else {
            HAPLogError(&logObject, "Read error: %d.", errno);
            fclose(prepareContext->fileHandle);
            prepareContext->fileHandle = NULL;
            return kHAPError_Unknown;
        }
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
 * Concatenates split diagnostic files into a single file
 */
static char* _Nullable ConcatenateDiagnosticFiles(uint64_t maxFileSize) {
    HAPLogInfo(&logObject, "%s, max file size: %lu", __func__, (unsigned long) maxFileSize);

    if (logContextPtr == NULL) {
        return NULL;
    }

    struct stat st_buf = { 0 };
    if (stat(kHAPPlatformDiagnosticsADKLogsFolder, &st_buf) != 0) {
        return NULL;
    }

    HAPError err;
    char buf[PATH_MAX];
    char tempFilePath[PATH_MAX];
    HAPRawBufferZero(buf, sizeof buf);

    err = HAPStringWithFormat(tempFilePath, sizeof tempFilePath, "%s.temp", logContextPtr->logFilePath);
    HAPAssert(!err);
    if (err) {
        return NULL;
    }

    err = HAPStringWithFormat(
            buf, sizeof buf, "ls -rt %s.* | xargs cat > %s", logContextPtr->logFilePath, tempFilePath);
    HAPAssert(!err);
    if (err) {
        return NULL;
    }

    HAPLogInfo(&logObject, "Command : [%s]", buf);
    FILE* outputStream = popen(buf, "r");
    if (!outputStream) {
        int _errno = errno;
        HAPLogError(&logObject, "concatenation failed: %d", _errno);
        return NULL;
    }

    while (fread(buf, sizeof(char), sizeof(buf) - 1, outputStream) == sizeof(buf) - 1) {
        HAPLogInfo(&logObject, "%s", buf);
        HAPRawBufferZero(buf, sizeof buf);
    }
    HAPLogInfo(&logObject, "%s", buf);
    pclose(outputStream);

    // Check concatenated file size
    if (stat(tempFilePath, &st_buf) != 0) {
        HAPLogError(&kHAPLog_Default, "Diagnostics could not get concatenated file size.");
        return NULL;
    }
    size_t bytesToTrim = 0;
    int error = 0;
    if ((uint64_t) st_buf.st_size > maxFileSize) {
        bytesToTrim = st_buf.st_size - maxFileSize;
    }

    HAPLogInfo(&logObject, "Bytes to trim from ADK log file: %zu", bytesToTrim);

    if (bytesToTrim > 0) {
        FILE* newFD = fopen(logContextPtr->logFilePath, "w");
        if (newFD == NULL) {
            HAPLogError(&kHAPLog_Default, "Could not open file for writing.");
            return NULL;
        }
        FILE* oldFD = fopen(tempFilePath, "r");
        if (oldFD == NULL) {
            HAPLogError(&kHAPLog_Default, "Could not open file for reading.");
            fclose(newFD);
            return NULL;
        }
        error = fseek(oldFD, bytesToTrim, SEEK_SET);
        if (error) {
            fclose(oldFD);
            fclose(newFD);
            HAPAssert(!error);
            return NULL;
        }

        int c = fgetc(oldFD);
        while (c != EOF) {
            error = fputc(c, newFD);
            if (error == EOF) {
                fclose(oldFD);
                fclose(newFD);
                HAPAssert(!error);
                return NULL;
            }
            c = fgetc(oldFD);
        }
        fclose(oldFD);
        fclose(newFD);

        error = remove(tempFilePath);
        if (error) {
            HAPLogError(&kHAPLog_Default, "Could not remove temporary file.");
            HAPAssert(!error);
            return NULL;
        }
    } else {
        error = rename(tempFilePath, logContextPtr->logFilePath);
        if (error) {
            HAPLogError(&kHAPLog_Default, "Could not rename temporary file.");
            HAPAssert(!error);
            return NULL;
        }
    }

    return logContextPtr->logFilePath;
}

/**
 * Capture dmesg logs
 */
static void CaptureDmesgLogs(const char* _Nonnull dmesgLogFileName) {
    HAPPrecondition(dmesgLogFileName);
    HAPError err;
    char buf[PATH_MAX];
    HAPRawBufferZero(buf, sizeof buf);

    HAPLogInfo(&logObject, "%s: Capturing dmesg logs", __func__);

#ifdef DARWIN
    err = HAPStringWithFormat(buf, sizeof buf, "dmesg > %s", dmesgLogFileName);
#else
    err = HAPStringWithFormat(buf, sizeof buf, "dmesg -T > %s", dmesgLogFileName);
#endif
    HAPAssert(!err);
    HAPLogInfo(&logObject, "Command : [%s]", buf);

    FILE* outputStream = popen(buf, "r");
    if (!outputStream) {
        int _errno = errno;
        HAPLogError(&logObject, "dmesg capture failed: %d", _errno);
        return;
    }

    while (fread(buf, sizeof(char), sizeof(buf) - 1, outputStream) == sizeof(buf) - 1) {
        HAPLogInfo(&logObject, "%s", buf);
        HAPRawBufferZero(buf, sizeof buf);
    }
    HAPLogInfo(&logObject, "%s", buf);
    pclose(outputStream);
    return;
}

/**
 * Thread to collect diagnostics data
 */
static void* _Nullable CollectDiagnosticData(void* _Nonnull context) {
    HAPPrecondition(context);
    HAPPrecondition(accessoryDiagnosticsConfig);

    HAPLogInfo(&logObject, "%s: Diagnostics data collection thread started", __func__);
    if (pthread_detach(pthread_self()) != 0) {
        HAPLogError(&kHAPLog_Default, "Detach thread failed.");
        HAPFatalError();
    }

    PrepareDiagnosticsDataContext* prepareContext = (PrepareDiagnosticsDataContext*) context;
    UpdateDiagnosticsDataCollectionState(prepareContext, kDiagnosticsDataCollection_InProgress);
    HAPError err;

    // Abort if requested by HAP
    if (DidAbortPrepareDiagnosticsData(prepareContext) == true) {
        goto exit;
    }

    struct stat st_buf = { 0 };
    if (stat(kHAPPlatformDiagnosticsFolder, &st_buf) != 0) {
        HAPLogError(&kHAPLog_Default, "Diagnostics folder %s does not exist.", kHAPPlatformDiagnosticsFolder);
        UpdateDiagnosticsDataCollectionState(prepareContext, kDiagnosticsDataCollection_Error);
        err = prepareContext->callbacks->handleDiagnosticsDataCancel();
        HAPAssert(!err);
        goto exit;
    }

    char* sendFileName = NULL;
    if (accessoryDiagnosticsConfig->diagnosticsSnapshotFormat == kHAPDiagnosticsSnapshotFormat_Zip) {
        sendFileName = kHAPPlatformDiagnosticsZipFileName;
    } else if (accessoryDiagnosticsConfig->diagnosticsSnapshotFormat == kHAPDiagnosticsSnapshotFormat_Text) {
        sendFileName = kHAPPlatformDiagnosticsTextFileName;
    } else {
        HAPLogError(&kHAPLog_Default, "Unknown diagnostics format.");
        UpdateDiagnosticsDataCollectionState(prepareContext, kDiagnosticsDataCollection_Error);
        err = prepareContext->callbacks->handleDiagnosticsDataCancel();
        HAPAssert(!err);
        goto exit;
    }

    // Delete older diagnostic send file if one exists
    if (stat(sendFileName, &st_buf) == 0) {
        if (remove(sendFileName) != 0) {
            HAPLogError(&kHAPLog_Default, "Diagnostics could not remove old send file.");
            UpdateDiagnosticsDataCollectionState(prepareContext, kDiagnosticsDataCollection_Error);
            err = prepareContext->callbacks->handleDiagnosticsDataCancel();
            HAPAssert(!err);
            goto exit;
        }
    }

    // Abort if requested by HAP
    if (DidAbortPrepareDiagnosticsData(prepareContext) == true) {
        goto exit;
    }

    if (accessoryDiagnosticsConfig->diagnosticsSnapshotFormat == kHAPDiagnosticsSnapshotFormat_Text &&
        logContextPtr != NULL && logContextPtr->logFileStream != NULL) {
        const HAPDiagnosticsContext* dgContext = accessoryDiagnosticsConfig->diagnosticsContext;
        HAPPrecondition(dgContext);
        char configurationBytes[kHAPDiagnostics_1KiloBytes];
        HAPRawBufferZero(configurationBytes, sizeof configurationBytes);

        HAPAccessoryServerGetAccessoryConfigurationString(
                dgContext->hapPlatform, dgContext->accessory, configurationBytes, sizeof configurationBytes);
        err = HAPPlatformWriteToDiagnosticsLogFile(
                logContextPtr, configurationBytes, HAPStringGetNumBytes(configurationBytes));
        if (err) {
            UpdateDiagnosticsDataCollectionState(prepareContext, kDiagnosticsDataCollection_Error);
            err = prepareContext->callbacks->handleDiagnosticsDataCancel();
            HAPAssert(!err);
            goto exit;
        }
    }

    // Flush data to file
    if (logContextPtr != NULL && logContextPtr->logFileStream != NULL) {
        fflush(logContextPtr->logFileStream);
    }

    // Override max log size if requested in metadata
    uint64_t maxLogSizeBytes = kHAPDiagnostics_MaxDataSizeInMB * kHAPDiagnostics_MBToBytes;
    if (prepareContext->metadata->_.diagnostics.snapshot.maxLogSize > maxLogSizeBytes) {
        maxLogSizeBytes = prepareContext->metadata->_.diagnostics.snapshot.maxLogSize;
    }

    // Concatenate all diagnostic log files if necessary
    char* concatenatedFilePath = ConcatenateDiagnosticFiles(maxLogSizeBytes * kDiagnosticsADKLogSizeTolerance);

    if (accessoryDiagnosticsConfig->diagnosticsSnapshotFormat == kHAPDiagnosticsSnapshotFormat_Zip) {
        bool shouldAppendExclusionOption = true; // Flag to check if the -x option should be used
        // Zip the folder contents
        char buf[PATH_MAX];
        HAPRawBufferZero(buf, sizeof buf);
        if (prepareContext->metadata->_.diagnostics.snapshot.snapshotType == kHAPDiagnosticsSnapshotType_ADK) {
            err = HAPStringWithFormat(
                    buf,
                    sizeof buf,
                    "%s -r %s %s",
                    kHAPPlatformDiagnosticsZipUtility,
                    kHAPPlatformDiagnosticsZipFileName,
                    kHAPPlatformDiagnosticsFolder);
            shouldAppendExclusionOption = true;
        } else {
            err = HAPStringWithFormat(
                    buf,
                    sizeof buf,
                    "%s -r %s %s -x \"%s/*\" \"%s/*\"",
                    kHAPPlatformDiagnosticsZipUtility,
                    kHAPPlatformDiagnosticsZipFileName,
                    kHAPPlatformDiagnosticsFolder,
                    kHAPPlatformDiagnosticsAudioLogsFolder,
                    kHAPPlatformDiagnosticsBTLogsFolder);
            shouldAppendExclusionOption = false; // -x option already added, need not add again
        }
        if (err) {
            UpdateDiagnosticsDataCollectionState(prepareContext, kDiagnosticsDataCollection_Error);
            err = prepareContext->callbacks->handleDiagnosticsDataCancel();
            HAPAssert(!err);
            goto exit;
        }

        // Exclude split files from diagnostics logs as the concatenated file will be included
        if (concatenatedFilePath != NULL) {
            if (shouldAppendExclusionOption == true) {
                err = HAPStringWithFormat(buf, sizeof buf, "%s -x \"%s.*\"", buf, concatenatedFilePath);
            } else {
                err = HAPStringWithFormat(buf, sizeof buf, "%s \"%s.*\"", buf, concatenatedFilePath);
            }

            if (err) {
                UpdateDiagnosticsDataCollectionState(prepareContext, kDiagnosticsDataCollection_Error);
                err = prepareContext->callbacks->handleDiagnosticsDataCancel();
                HAPAssert(!err);
                goto exit;
            }
        }

        // Capture Dmesg logs
        if (prepareContext->metadata->_.diagnostics.snapshot.snapshotType == kHAPDiagnosticsSnapshotType_ADK) {
            CaptureDmesgLogs(kHAPPlatformDiagnosticsDmesgFile);
        }

        HAPLogInfo(&logObject, "Command : [%s]", buf);
        FILE* outputStream = popen(buf, "r");
        if (!outputStream) {
            int _errno = errno;
            HAPLogError(&logObject, "zip failed: %d", _errno);
            UpdateDiagnosticsDataCollectionState(prepareContext, kDiagnosticsDataCollection_Error);
            err = prepareContext->callbacks->handleDiagnosticsDataCancel();
            HAPAssert(!err);
            goto exit;
        }

        while (fread(buf, sizeof(char), sizeof(buf) - 1, outputStream) == sizeof(buf) - 1) {
            HAPLogInfo(&logObject, "%s", buf);
            HAPRawBufferZero(buf, sizeof buf);
        }
        HAPLogInfo(&logObject, "%s", buf);
        pclose(outputStream);
    }

    if (accessoryDiagnosticsConfig->diagnosticsSnapshotFormat == kHAPDiagnosticsSnapshotFormat_Text) {
        if (concatenatedFilePath != NULL) {
            if (rename(concatenatedFilePath, kHAPPlatformDiagnosticsTextFileName) != 0) {
                HAPLogError(&logObject, "Diagnostics could not rename text file");
                UpdateDiagnosticsDataCollectionState(prepareContext, kDiagnosticsDataCollection_Error);
                err = prepareContext->callbacks->handleDiagnosticsDataCancel();
                HAPAssert(!err);
                goto exit;
            }
        }
    }

    // Delete concatenated file if one exists
    if (concatenatedFilePath != NULL &&
        accessoryDiagnosticsConfig->diagnosticsSnapshotFormat == kHAPDiagnosticsSnapshotFormat_Zip) {
        if (stat(concatenatedFilePath, &st_buf) == 0) {
            if (remove(concatenatedFilePath) != 0) {
                HAPLogError(&kHAPLog_Default, "Diagnostics could not remove concatenated file.");
                UpdateDiagnosticsDataCollectionState(prepareContext, kDiagnosticsDataCollection_Error);
                err = prepareContext->callbacks->handleDiagnosticsDataCancel();
                HAPAssert(!err);
                goto exit;
            }
        }
    }

    // Check send file size
    if (stat(sendFileName, &st_buf) != 0) {
        HAPLogError(&kHAPLog_Default, "Diagnostics could not get diagnostics file size.");
        UpdateDiagnosticsDataCollectionState(prepareContext, kDiagnosticsDataCollection_Error);
        err = prepareContext->callbacks->handleDiagnosticsDataCancel();
        HAPAssert(!err);
        goto exit;
    }

    if ((uint64_t) st_buf.st_size > maxLogSizeBytes) {
        HAPLogError(
                &logObject,
                "Diagnostics file size = %lu bytes. Exceeded maximum allowed size of %lu bytes.",
                (unsigned long) st_buf.st_size,
                (unsigned long) maxLogSizeBytes);
        UpdateDiagnosticsDataCollectionState(prepareContext, kDiagnosticsDataCollection_Error);
        err = prepareContext->callbacks->handleDiagnosticsDataCancel();
        HAPAssert(!err);
        goto exit;
    }

    // Abort if requested by HAP
    if (DidAbortPrepareDiagnosticsData(prepareContext) == true) {
        goto exit;
    }

    UpdateDiagnosticsDataCollectionState(prepareContext, kDiagnosticsDataCollection_Ready);

    if (prepareContext->metadata->_.diagnostics.snapshot.snapshotType == kHAPDiagnosticsSnapshotType_Manufacturer) {
        // VENDOR-TODO: Provide the URL parameters and timestamp here
        HAPTime now = HAPPlatformClockGetCurrent();
        HAPLogInfo(&logObject, "Time-stamp: %lu", (unsigned long) now);
        err = HAPStringWithFormat(signedTimestamp, sizeof signedTimestamp, "%lu", (unsigned long) now);
        HAPAssert(!err);
        err = prepareContext->callbacks->handleDiagnosticsDataReady(st_buf.st_size, &diagnosticsURLParameters);
    } else if (prepareContext->metadata->_.diagnostics.snapshot.snapshotType == kHAPDiagnosticsSnapshotType_ADK) {
        err = prepareContext->callbacks->handleDiagnosticsDataReady(st_buf.st_size, NULL);
    } else {
        HAPLogError(&logObject, "%s: Unknown diagnostics snapshot type", __func__);
        err = kHAPError_InvalidData;
    }
    HAPAssert(!err);
    HAPLogInfo(&logObject, "%s: Exiting diagnostic data collection thread", __func__);
    return NULL;

exit:
    HAPLogInfo(&logObject, "%s: Exiting diagnostic data collection thread", __func__);
    return NULL;
}

/**
 * Prepares the diagnostics data in a separate thread or process.
 * Returns from this function immediately.
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

    pthread_mutex_lock(&diagnosticsDataCollectionLock);
    prepareContext->dataState = kDiagnosticsDataCollection_Idle;
    prepareContext->fileHandle = NULL;
    prepareContext->didAbortPrepareDiagnosticsData = false;
    prepareContext->callbacks = diagnosticsCallbacks;
    prepareContext->metadata = metadata;
    pthread_mutex_unlock(&diagnosticsDataCollectionLock);

    pthread_t threadID;
    HAPLogInfo(&logObject, "%s", __func__);
    int err = pthread_create(&threadID, NULL, CollectDiagnosticData, (void*) prepareContext);
    if (err) {
        HAPLogError(&logObject, "`pthread_create` failed to create diagnostic data collection thread (%d).", err);
        free(prepareContext);
        prepareContext = NULL;
        return NULL;
    }

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
    return;
}

/**
 * Called by HAP when diagnostics data transfer to controller is aborted.
 */
void HAPPlatformDiagnosticsAbort(void* _Nullable prepareDiagnosticsDataContext) {
    HAPPrecondition(prepareDiagnosticsDataContext);
    PrepareDiagnosticsDataContext* prepareContext = (PrepareDiagnosticsDataContext*) prepareDiagnosticsDataContext;
    HAPLogInfo(&logObject, "Diagnostics data transfer aborted");
    pthread_mutex_lock(&diagnosticsDataCollectionLock);
    if (prepareContext->dataState == kDiagnosticsDataCollection_Ready ||
        prepareContext->dataState == kDiagnosticsDataCollection_Error) {
        free(prepareContext);
        prepareContext = NULL;
    } else {
        prepareContext->didAbortPrepareDiagnosticsData = true;
    }
    pthread_mutex_unlock(&diagnosticsDataCollectionLock);
    return;
}

#endif // HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)
