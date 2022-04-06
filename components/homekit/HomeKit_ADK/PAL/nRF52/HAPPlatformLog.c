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

#include "SEGGER_RTT.h"

#include "HAPPlatform.h"
#if HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)
#include "HAPPlatformDiagnostics.h"
#endif

#define kRTT_LogChannel   ((unsigned int) 0)
#define MAX_BUFFERLOG_LEN 128

#if HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)
#define LOG_PRINTF(channel, fmt, ...) \
    do { \
        (void) SEGGER_RTT_printf(channel, fmt, ##__VA_ARGS__); \
        HAPPlatformDiagnosticsWriteToLogBuffer(fmt, ##__VA_ARGS__); \
    } while (0)
#else
#define LOG_PRINTF(channel, fmt, ...) \
    do { \
        (void) SEGGER_RTT_printf(channel, fmt, ##__VA_ARGS__); \
    } while (0)
#endif

HAP_RESULT_USE_CHECK
HAPPlatformLogEnabledTypes HAPPlatformLogGetEnabledTypes(const HAPLogObject* log HAP_UNUSED) {
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
        const HAPLogObject* log,
        HAPLogType type,
        const char* message,
        const void* _Nullable bufferBytes,
        size_t numBufferBytes) HAP_DIAGNOSE_ERROR(!bufferBytes && numBufferBytes, "empty buffer cannot have a length") {
    HAPError err;

    // Color.
    switch (type) {
        case kHAPLogType_Debug: {
            (void) SEGGER_RTT_printf(kRTT_LogChannel, "\x1B[0m");
            break;
        }
        case kHAPLogType_Info: {
            (void) SEGGER_RTT_printf(kRTT_LogChannel, "\x1B[32m");
            break;
        }
        case kHAPLogType_Default: {
            (void) SEGGER_RTT_printf(kRTT_LogChannel, "\x1B[35m");
            break;
        }
        case kHAPLogType_Error: {
            (void) SEGGER_RTT_printf(kRTT_LogChannel, "\x1B[31m");
            break;
        }
        case kHAPLogType_Fault: {
#if (HAP_DISABLE_ASSERTS == 0)
            // Change the RTT logging to blocking mode so that the fault log can be guaranteed
            // as far as the JLink connection is intact and the connected host is polling the trace buffer.
            // Note that changing the RTT buffer mode to blocking mode itself could crash the nRF52 firmware.
            // However, it is assumed that the program won't proceed further once fault log is generated.
            SEGGER_RTT_BUFFER_UP* bufferUp = &_SEGGER_RTT.aUp[kRTT_LogChannel];
            if ((bufferUp->Flags & SEGGER_RTT_MODE_MASK) != SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL) {
                bufferUp->Flags = (bufferUp->Flags & ~SEGGER_RTT_MODE_MASK) | SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL;
            }
#endif
            (void) SEGGER_RTT_printf(kRTT_LogChannel, "\x1B[1m\x1B[31m");
            break;
        }
    }

    // Time.
    HAPTime now = HAPPlatformClockGetCurrent();
    char timeString[64];
    err = HAPStringWithFormat(
            timeString,
            sizeof timeString,
            "%8llu.%03llu",
            (unsigned long long) (now / HAPSecond),
            (unsigned long long) (now % HAPSecond));
    HAPAssert(!err);
    LOG_PRINTF(kRTT_LogChannel, "%s", timeString);
    LOG_PRINTF(kRTT_LogChannel, "\t");

    // Type.
    switch (type) {
        case kHAPLogType_Debug: {
            LOG_PRINTF(kRTT_LogChannel, "Debug");
            break;
        }
        case kHAPLogType_Info: {
            LOG_PRINTF(kRTT_LogChannel, "Info");
            break;
        }
        case kHAPLogType_Default: {
            LOG_PRINTF(kRTT_LogChannel, "Default");
            break;
        }
        case kHAPLogType_Error: {
            LOG_PRINTF(kRTT_LogChannel, "Error");
            break;
        }
        case kHAPLogType_Fault: {
            LOG_PRINTF(kRTT_LogChannel, "Fault");
            break;
        }
    }
    LOG_PRINTF(kRTT_LogChannel, "\t");

    // Subsystem / Category.
    if (log->subsystem) {
        LOG_PRINTF(kRTT_LogChannel, "[%s", log->subsystem);
        if (log->category) {
            LOG_PRINTF(kRTT_LogChannel, ":%s", log->category);
        }
        LOG_PRINTF(kRTT_LogChannel, "] ");
    }

    // Message.
    if ((__get_IPSR() & 0xFF) != 0) {
        LOG_PRINTF(kRTT_LogChannel, "<!!! CALLED FROM INTERRUPT !!!> ");
    }
    LOG_PRINTF(kRTT_LogChannel, "%s", message);
    LOG_PRINTF(kRTT_LogChannel, "\n");

    // Buffer.
    if (bufferBytes) {
        size_t i, o;
        const uint8_t* b = bufferBytes;
        size_t length = numBufferBytes;
        if (length == 0) {
            LOG_PRINTF(kRTT_LogChannel, "\n");
        } else {
            i = 0;
            char line[MAX_BUFFERLOG_LEN] = { 0 };
            do {
                size_t pos = 0;
                pos += snprintf(&line[pos], MAX_BUFFERLOG_LEN - pos, "    %04x ", i);
                for (o = 0; o != 8 * 4; o++) {
                    if (o % 4 == 0) {
                        pos += snprintf(&line[pos], MAX_BUFFERLOG_LEN - pos, " ");
                    }
                    if ((o <= length) && (i < length - o)) {
                        pos += snprintf(&line[pos], MAX_BUFFERLOG_LEN - pos, "%02x", b[i + o] & 0xff);
                    } else {
                        pos += snprintf(&line[pos], MAX_BUFFERLOG_LEN - pos, "  ");
                    }
                };
                pos += snprintf(&line[pos], MAX_BUFFERLOG_LEN - pos, "    ");
                for (o = 0; o != 8 * 4; o++) {
                    if (i != length) {
                        if ((32 <= b[i]) && (b[i] < 127)) {
                            pos += snprintf(&line[pos], MAX_BUFFERLOG_LEN - pos, "%c", b[i]);
                        } else {
                            pos += snprintf(&line[pos], MAX_BUFFERLOG_LEN - pos, ".");
                        }
                        i++;
                    }
                }
                LOG_PRINTF(kRTT_LogChannel, "%s\n", line);
            } while (i != length);
        }
    }

    // Reset color.
    LOG_PRINTF(kRTT_LogChannel, "\x1B[0m");
}
