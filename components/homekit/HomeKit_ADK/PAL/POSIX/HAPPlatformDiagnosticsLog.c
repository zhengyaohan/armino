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
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "HAP.h"

#include "HAPPlatformDiagnosticsLog.h"
#include "HAPPlatformLog+Init.h"

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "DiagnosticsLog" };

/**
 * Deletes all diagnostics logs.
 *
 * @param  context                    Diagnostics log context pointer.
 *
 * @return kHAPError_None             If successful.
 * @return kHAPError_OutOfResources   If out of resources.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformDeleteAllDiagnosticsLogFiles(DiagnosticsLog* _Nonnull context) {

    HAPPrecondition(context);
    HAPPrecondition(context->logFilePath);

    HAPLogInfo(&logObject, "%s", __func__);

    for (int i = context->numLogFiles; i > 0; i--) {
        char currFileName[PATH_MAX];
        HAPError err = 0;

        err = HAPStringWithFormat(currFileName, sizeof currFileName, "%s.%d", context->logFilePath, i - 1);
        if (err) {
            return kHAPError_OutOfResources;
        }

        HAPLogDebug(&logObject, "Deleting file %s", currFileName);
        remove(currFileName);
    }
    return kHAPError_None;
}

/**
 * Rotates all log files by one and opens a new file handle to the next active log file
 *
 * This function is called when the active log file size exceeds M/N bytes. Where
 * M is the total log size and N is the number of log files. If N files already exist
 * then the oldest file is deleted.
 */
static void HAPPlatformRotateLogFile(DiagnosticsLog* _Nonnull context) {

    HAPAssert(context);
    HAPAssert(context->logFilePath);
    HAPAssert(context->logFileStream);

    fclose(context->logFileStream);

    bool indexFileFound = false;

    // Rotate the files
    for (int i = context->numLogFiles; i > 0; i--) {
        char currFileName[PATH_MAX];
        struct stat st_buf = { 0 };
        HAPError err = 0;
        int retVal = 0;

        err = HAPStringWithFormat(currFileName, sizeof currFileName, "%s.%d", context->logFilePath, i - 1);
        HAPAssert(!err);
        if (err) {
            return;
        }

        retVal = stat(currFileName, &st_buf);
        if (retVal != 0) {
            if (indexFileFound == false) {
                // File does not exist, continue iteration.
                continue;
            } else {
                // Unexpected. Fail if a gap in index is found
                HAPAssert(0);
                return;
            }
        }

        // Found highest index of rotated logs. No more gaps allowed.
        indexFileFound = true;

        if (i == context->numLogFiles) {
            // First iteration. Delete the oldest file if it exists
            retVal = remove(currFileName);
            HAPAssert(!retVal);
            if (retVal) {
                return;
            }
        } else {
            char newFileName[PATH_MAX];
            err = HAPStringWithFormat(newFileName, sizeof newFileName, "%s.%d", context->logFilePath, i);
            HAPAssert(!err);
            if (err) {
                return;
            }

            // Bump up the log file by one spot
            retVal = rename(currFileName, newFileName);
            HAPAssert(!retVal);
            if (retVal) {
                return;
            }
        }
    }

    char currFileName[PATH_MAX];
    HAPError err = HAPStringWithFormat(currFileName, sizeof currFileName, "%s.%d", context->logFilePath, 0);
    HAPAssert(!err);
    if (err) {
        return;
    }

    context->logFileStream = fopen(currFileName, "wb");
    HAPAssert(context->logFileStream);
    if (context->logFileStream == NULL) {
        return;
    }
    context->remainingBytes = context->logSizePerFileInBytes;

    return;
}

/**
 * Set up logs and populate context structure for logging operations
 *
 * @param  logFilePath                Path of the active log file.
 * @param  numLogFiles                Number of log files to be created.
 * @param  totalLogSize               Total size of combined log files.
 * @param  context                    Log context pointer.
 *
 * @return kHAPError_None             If successful.
 * @return kHAPError_OutOfResources   If out of resources.
 * @return kHAPError_Unknown          If unknown error.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformSetupDiagnosticsLogFiles(
        const char* _Nonnull logFilePath,
        int numLogFiles,
        size_t totalLogSize,
        DiagnosticsLog* _Nonnull context) {

    HAPPrecondition(logFilePath);
    HAPPrecondition(context);
    HAPPrecondition(!(context->logFileStream));

    HAPLogInfo(&logObject, "%s", __func__);

    HAPError err = 0;

    context->numLogFiles = numLogFiles;
    context->logSizePerFileInBytes = totalLogSize / numLogFiles;
    err = HAPStringWithFormat(context->logFilePath, sizeof context->logFilePath, "%s", logFilePath);
    if (err) {
        return kHAPError_OutOfResources;
    }
    HAPLogInfo(&logObject, "Number of log files %d", context->numLogFiles);
    HAPLogInfo(&logObject, "Log size per rotated log %zu", context->logSizePerFileInBytes);
    HAPLogInfo(&logObject, "Log file path %s", context->logFilePath);

    bool deleteLogs = false;
    bool indexFileFound = false;

    for (int i = numLogFiles; i > 0; i--) {
        char currFileName[PATH_MAX];
        struct stat st_buf = { 0 };
        int retVal = 0;

        err = HAPStringWithFormat(currFileName, sizeof currFileName, "%s.%d", logFilePath, i - 1);
        if (err) {
            return kHAPError_OutOfResources;
        }

        HAPLogDebug(&logObject, "Checking for file %s", currFileName);
        retVal = stat(currFileName, &st_buf);
        if (retVal != 0) {
            if (indexFileFound == false) {
                HAPLogDebug(&logObject, "File not found. Continuing.");
                // File does not exist, continue iteration.
                continue;
            } else {
                HAPLogInfo(&logObject, "Inconsistency found in rotated logs");
                // Inconsistency found. Delete all existing logs
                deleteLogs = true;
                break;
            }
        }

        // Found highest index of rotated logs. No more gaps allowed.
        indexFileFound = true;

        // Check if the existing rotated logs are of acceptable size
        if ((size_t) st_buf.st_size > (context->logSizePerFileInBytes + kHAPPlatformDiagnosticsLog1KBytes)) {
            deleteLogs = true;
            HAPLogInfo(&logObject, "Existing rotated log found to be too large");
            break;
        }
    }

    if (deleteLogs == true) {
        HAPLogInfo(&logObject, "Deleting all existing logs");
        // Delete all existing rotated files
        err = HAPPlatformDeleteAllDiagnosticsLogFiles(context);
        if (err) {
            return kHAPError_Unknown;
        }
    }

    char currFileName[PATH_MAX];
    err = HAPStringWithFormat(currFileName, sizeof currFileName, "%s.%d", logFilePath, 0);
    if (err) {
        return kHAPError_OutOfResources;
    }

    context->logFileStream = fopen(currFileName, "a+b");
    if (context->logFileStream == NULL) {
        HAPLogError(&logObject, "Could not open file %s", currFileName);
        return kHAPError_Unknown;
    }

    struct stat st_buf = { 0 };
    if (stat(currFileName, &st_buf) != 0) {
        return kHAPError_Unknown;
    }

    context->remainingBytes = context->logSizePerFileInBytes - st_buf.st_size;
    if (context->remainingBytes <= 0) {
        HAPPlatformRotateLogFile(context);
    }

    return kHAPError_None;
}

/**
 * Set up diagnostics logs and populate context structure for logging operations
 *
 * @param  context                    Log context pointer.
 * @param  buffer                     Buffer containing log data.
 * @param  bytes                      Number of bytes from the buffer to write to file.
 *
 * @return kHAPError_None             If successful.
 * @return kHAPError_Unknown          If unknown error.
 * @return kHAPError_InvalidData      If invalid data is provided.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWriteToDiagnosticsLogFile(
        DiagnosticsLog* _Nonnull context,
        const char* _Nonnull buffer,
        size_t bytes) {
    if (!context || !context->logFileStream) {
        return kHAPError_InvalidData;
    }

    if (context->remainingBytes < (int) bytes) {
        HAPPlatformRotateLogFile(context);
    }

    size_t bytesWritten = fwrite(buffer, sizeof(char), bytes, context->logFileStream);
    if (bytes != bytesWritten) {
        return kHAPError_Unknown;
    }
    context->remainingBytes -= bytes;

    return kHAPError_None;
}

/**
 * Closes active log file stream.
 *
 * @param  context                    Log context pointer.
 *
 * @return kHAPError_None             If successful.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformCloseDiagnosticsLogFile(DiagnosticsLog* _Nonnull context) {
    HAPPrecondition(context);

    HAPLogInfo(&logObject, "%s", __func__);
    if (context->logFileStream == NULL) {
        return kHAPError_None;
    }
    fclose(context->logFileStream);
    context->logFileStream = NULL;

    return kHAPError_None;
}
