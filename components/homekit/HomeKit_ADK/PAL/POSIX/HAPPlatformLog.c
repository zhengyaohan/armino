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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/time.h>
#endif

#include "HAP.h"

#include "HAPPlatformCamera+Init.h"
#include "HAPPlatformConcurrency.h"
#include "HAPPlatformLog+Init.h"
#if HAP_LOG_LEVEL
#include "HAPPlatformLog+Camera.h"
#endif
#if HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)
#include "HAPPlatformDiagnostics.h"
#endif

#define MAX_BUFFERLOG_LEN 128

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "Log" };

#if HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)
#define FPRINTF(stream, fmt, ...) \
    do { \
        fprintf_stderr(stream, fmt, ##__VA_ARGS__); \
        HAPPlatformDiagnosticsWriteToFile(false, fmt, ##__VA_ARGS__); \
    } while (0)
#else
#define FPRINTF(stream, fmt, ...) \
    do { \
        fprintf_stderr(stream, fmt, ##__VA_ARGS__); \
    } while (0)
#endif

static void fprintf_stderr(FILE* _Nonnull stream, const char* _Nullable format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stream, format, args);
    va_end(args);
}

void HAPPlatformLogPOSIXError(
        HAPLogType type,
        const char* _Nonnull message,
        int errorNumber,
        const char* _Nonnull function,
        const char* _Nonnull file,
        int line) {
    HAPPrecondition(message);
    HAPPrecondition(function);
    HAPPrecondition(file);

    HAPError err;
    (void) err;

    // Get error message.
    char* errorString;
    char errorBytes[256];
#if !defined(__GLIBC__) || ((_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && !defined(_GNU_SOURCE))
    // XSI-compliant version of 'strerror_r'.
    int e = strerror_r(errorNumber, errorBytes, sizeof errorBytes);
    if (e == EINVAL) {
        err = HAPStringWithFormat(errorBytes, sizeof errorBytes, "Unknown error %d", errorNumber);
        HAPAssert(!err);
    } else if (e) {
        HAPAssert(e == ERANGE);
        HAPLog(&logObject, "strerror_r error: ERANGE.");
        return;
    }
    errorString = errorBytes;
#else
    // GNU-specific version of 'strerror_r'.
    // Error string may be truncated if buffer does not have enough capacity, but is always NULL-terminated.
    errorString = strerror_r(errorNumber, errorBytes, sizeof errorBytes);
#endif

    // Perform logging.
    HAPLogWithType(&logObject, type, "%s:%d:%s - %s @ %s:%d", message, errorNumber, errorString, function, file, line);
}

HAP_RESULT_USE_CHECK
HAPPlatformLogEnabledTypes HAPPlatformLogGetEnabledTypes(const HAPLogObject* _Nonnull log HAP_UNUSED) {
    switch (HAP_LOG_LEVEL) {
        case HAP_LOG_LEVEL_NONE: {
            return kHAPPlatformLogEnabledTypes_None;
        }
        case HAP_LOG_LEVEL_ERROR: {
            return kHAPPlatformLogEnabledTypes_Error;
        }
        case HAP_LOG_LEVEL_DEFAULT: {
            return kHAPPlatformLogEnabledTypes_Default;
        }
        case HAP_LOG_LEVEL_INFO: {
            return kHAPPlatformLogEnabledTypes_Info;
        }
        case HAP_LOG_LEVEL_DEBUG: {
            return kHAPPlatformLogEnabledTypes_Debug;
        }
        default:
            HAPFatalError();
    }
}

void HAPPlatformLogCapture(
        const HAPLogObject* _Nonnull log,
        HAPLogType type,
        const char* _Nonnull message,
        const void* _Nullable bufferBytes,
        size_t numBufferBytes) HAP_DIAGNOSE_ERROR(!bufferBytes && numBufferBytes, "empty buffer cannot have a length") {
    HAPPrecondition(log);
    HAPPrecondition(message);
    HAPPrecondition(!numBufferBytes || bufferBytes);

    // Format log message.
    bool logHandled = false;

// Handle camera logs.
#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)
#if HAP_LOG_LEVEL
    static const HAPPlatformLogCameraLogOptions cameraLogOptions = {
        .logVideoIDR = false, .logVideoRTP = false, .logAudioRTP = false, .logRTCP = false, .logStatistics = true
    };
    if (log == &kHAPRTPController_PacketLog) {
        HAPPlatformLogCameraRTPPacket(HAPNonnullVoid(bufferBytes), numBufferBytes, message, cameraLogOptions);
        logHandled = true;
    } else if (log == &kHAPPlatformCamera_VideoLog) {
        HAPPlatformLogCameraH264Payload(bufferBytes, numBufferBytes, message, cameraLogOptions);
        logHandled = true;
    } else if (log == &kHAPPlatformCamera_AudioLog) {
        HAPPlatformLogCameraPayload(bufferBytes, numBufferBytes, message, cameraLogOptions);
        logHandled = true;
    } else if (log == &kHAPPlatformCamera_SpeakerLog) {
        HAPPlatformLogCameraPayload(bufferBytes, numBufferBytes, message, cameraLogOptions);
        logHandled = true;
    }
#endif
#endif

    // Perform regular logging.
    if (!logHandled) {
        // Color.
        switch (type) {
            case kHAPLogType_Debug: {
                fprintf(stderr, "\x1B[0m");
                break;
            }
            case kHAPLogType_Info: {
                fprintf(stderr, "\x1B[32m");
                break;
            }
            case kHAPLogType_Default: {
                fprintf(stderr, "\x1B[35m");
                break;
            }
            case kHAPLogType_Error: {
                fprintf(stderr, "\x1B[31m");
                break;
            }
            case kHAPLogType_Fault: {
                fprintf(stderr, "\x1B[1m\x1B[31m");
                break;
            }
        }

// Time.
#ifdef _WIN32
        SYSTEMTIME now;
        GetSystemTime(&now);
        FPRINTF(stderr,
                "%04d-%02d-%02d'T'%02d:%02d:%02d'Z'",
                now.wYear,
                now.wMonth,
                now.wDay,
                now.wHour,
                now.wMinute,
                now.wSecond);
#else
        struct timeval now;
        int err = gettimeofday(&now, NULL);
        if (!err) {
            struct tm g;
            struct tm* gmt = localtime_r(&now.tv_sec, &g);
            int microsecondTail = now.tv_usec % 1000;
            int microsecondHead = now.tv_usec / 1000;

            if (gmt) {
                FPRINTF(stderr,
                        "%04d-%02d-%02d %02d:%02d:%02d.%03d%03d",
                        1900 + gmt->tm_year,
                        1 + gmt->tm_mon,
                        gmt->tm_mday,
                        gmt->tm_hour,
                        gmt->tm_min,
                        gmt->tm_sec,
                        microsecondHead,
                        microsecondTail);
            }
        }
#endif
        FPRINTF(stderr, "%s", "\t");

        // Type.
        switch (type) {
            case kHAPLogType_Debug: {
                FPRINTF(stderr, "%s", "Debug");
                break;
            }
            case kHAPLogType_Info: {
                FPRINTF(stderr, "%s", "Info");
                break;
            }
            case kHAPLogType_Default: {
                FPRINTF(stderr, "%s", "Default");
                break;
            }
            case kHAPLogType_Error: {
                FPRINTF(stderr, "%s", "Error");
                break;
            }
            case kHAPLogType_Fault: {
                FPRINTF(stderr, "%s", "Fault");
                break;
            }
        }
        FPRINTF(stderr, "%s", "\t");

        // Subsystem / Category.
        if (log->subsystem) {
            FPRINTF(stderr, "[%s", log->subsystem);
            if (log->category) {
                FPRINTF(stderr, ":%s", log->category);
            }
            FPRINTF(stderr, "%s", "] ");
        }

        // Log thread id if it's changed
        if (HAPPlatformConcurrencyHasTIDChanged()) {
            char* _Nullable threadId = HAPPlatformConcurrencyGetTIDString();
            HAPAssert(threadId); // If HasTIDChanged() returns true, this should never be null
            FPRINTF(stderr, "[ThreadID: %s] ", threadId);
        }

        // Message.
        FPRINTF(stderr, "%s", message);
        FPRINTF(stderr, "%s", "\n");

        // Buffer.
        if (bufferBytes) {
            size_t i, n;
            const uint8_t* b = bufferBytes;
            size_t length = numBufferBytes;
            if (length == 0) {
                FPRINTF(stderr, "%s", "\n");
            } else {
                i = 0;
                char line[MAX_BUFFERLOG_LEN] = { 0 };
                do {
                    size_t pos = 0;
                    pos += snprintf(&line[pos], MAX_BUFFERLOG_LEN - pos, "    %04zx ", i);
                    for (n = 0; n != 8 * 4; n++) {
                        if (n % 4 == 0) {
                            pos += snprintf(&line[pos], MAX_BUFFERLOG_LEN - pos, "%s", " ");
                        }
                        if ((n <= length) && (i < length - n)) {
                            pos += snprintf(&line[pos], MAX_BUFFERLOG_LEN - pos, "%02x", b[i + n] & 0xff);
                        } else {
                            pos += snprintf(&line[pos], MAX_BUFFERLOG_LEN - pos, "%s", "  ");
                        }
                    };
                    pos += snprintf(&line[pos], MAX_BUFFERLOG_LEN - pos, "%s", "    ");
                    for (n = 0; n != 8 * 4; n++) {
                        if (i != length) {
                            if ((32 <= b[i]) && (b[i] < 127)) {
                                pos += snprintf(&line[pos], MAX_BUFFERLOG_LEN - pos, "%c", b[i]);
                            } else {
                                pos += snprintf(&line[pos], MAX_BUFFERLOG_LEN - pos, "%s", ".");
                            }
                            i++;
                        }
                    }
                    FPRINTF(stderr, "%s\n", line);
                } while (i != length);
            }
        }

        // Reset color.
        fprintf(stderr, "\x1B[0m");
    }

    // Finish log.
    (void) fflush(stderr);
}
