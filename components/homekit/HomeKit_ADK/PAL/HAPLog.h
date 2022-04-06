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

#ifndef HAP_LOG_H
#define HAP_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "HAPBase+CompilerAbstraction.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

// Validate log level.
// 0 - No logs are emitted. Default.
// 1 - Logs with type Error and Fault are emitted.
// 2 - Logs with type Default, Error and Fault are emitted.
// 3 - Logs with type Info, Default, Error and Fault are emitted.
// 4 - Logs with type Debug, Info, Default, Error and Fault are emitted. All logs.
#define HAP_LOG_LEVEL_NONE    0
#define HAP_LOG_LEVEL_FAULT   1
#define HAP_LOG_LEVEL_ERROR   1
#define HAP_LOG_LEVEL_DEFAULT 2
#define HAP_LOG_LEVEL_INFO    3
#define HAP_LOG_LEVEL_DEBUG   4

#ifndef HAP_LOG_LEVEL
#define HAP_LOG_LEVEL (HAP_LOG_LEVEL_DEBUG)
#endif
#if HAP_LOG_LEVEL < HAP_LOG_LEVEL_NONE || HAP_LOG_LEVEL > HAP_LOG_LEVEL_DEBUG
#error "Invalid HAP_LOG_LEVEL."
#endif

// Validate flag for including buffer data in logs.
#ifndef HAP_ENABLE_BUFFER_LOGS
#define HAP_ENABLE_BUFFER_LOGS (0)
#endif
#if HAP_ENABLE_BUFFER_LOGS < 0 || HAP_ENABLE_BUFFER_LOGS > 1
#error "Invalid HAP_ENABLE_BUFFER_LOGS."
#endif

// Validate flag for including sensitive information in logs.
#ifndef HAP_LOG_SENSITIVE
#define HAP_LOG_SENSITIVE (0)
#endif
#if HAP_LOG_SENSITIVE < 0 || HAP_LOG_SENSITIVE > 1
#error "Invalid HAP_LOG_SENSITIVE."
#endif

/**
 * Log object.
 */
typedef struct {
    /**
     * Subsystem that's performing logging.
     */
    const char* _Nullable subsystem;

    /**
     * A category within the specified subsystem.
     *
     * - If a category is defined, a subsystem must be specified as well.
     */
    const char* _Nullable category;
} HAPLogObject;

/**
 * Default log object.
 *
 * - Log messages are logged with NULL subsystem and category.
 */
extern const HAPLogObject kHAPLog_Default;

/**
 * Logging levels.
 */
HAP_ENUM_BEGIN(uint8_t, HAPLogType) {
    /**
     * Messages logged at this level contain information that may be useful during development or while troubleshooting
     * a specific problem.
     */
    kHAPLogType_Debug,

    /**
     * Use this level to capture information that may be helpful, but isn't essential, for troubleshooting errors.
     */
    kHAPLogType_Info,

    /**
     * Use this level to capture information about things that might result a failure.
     */
    kHAPLogType_Default,

    /**
     * Error-level messages are intended for reporting component-level errors.
     */
    kHAPLogType_Error,

    /**
     * Fault-level messages are intended for capturing system-level or multi-component errors only.
     */
    kHAPLogType_Fault
} HAP_ENUM_END(uint8_t, HAPLogType);

/**
 * Logs the contents of a buffer and a message at a specific logging level.
 *
 * @param      log                  Log object.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 * @param      type                 A log type constant, indicating the level of logging to perform.
 */
#define HAPLogBufferWithType(log, bytes, numBytes, type, ...) \
    do { \
        switch (type) { \
            case kHAPLogType_Debug: { \
                HAPLogBufferDebug(log, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Info: { \
                HAPLogBufferInfo(log, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Default: { \
                HAPLogBuffer(log, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Error: { \
                HAPLogBufferError(log, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Fault: { \
                HAPLogBufferFault(log, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
            default: \
                HAPFatalError(); \
        } \
    } while (0)

/**
 * Logs the contents of a buffer and a default-level message.
 *
 * @param      log                  Log object.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 */
#define HAPLogBuffer(log, bytes, numBytes, ...) \
    do { \
        if (HAP_LOG_LEVEL >= HAP_LOG_LEVEL_DEFAULT) { \
            if (HAP_ENABLE_BUFFER_LOGS) { \
                HAPLogBufferInternal(log, bytes, numBytes, __VA_ARGS__); \
            } else { \
                HAPLogInternal(log, __VA_ARGS__); \
            } \
        } \
    } while (0)

/**
 * Logs the contents of a buffer and an info-level message.
 *
 * @param      log                  Log object.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 */
#define HAPLogBufferInfo(log, bytes, numBytes, ...) \
    do { \
        if (HAP_LOG_LEVEL >= HAP_LOG_LEVEL_INFO) { \
            if (HAP_ENABLE_BUFFER_LOGS) { \
                HAPLogBufferInfoInternal(log, bytes, numBytes, __VA_ARGS__); \
            } else { \
                HAPLogInfoInternal(log, __VA_ARGS__); \
            } \
        } \
    } while (0)

/**
 * Logs the contents of a buffer and a debug-level message.
 *
 * @param      log                  Log object.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 */
#define HAPLogBufferDebug(log, bytes, numBytes, ...) \
    do { \
        if (HAP_LOG_LEVEL >= HAP_LOG_LEVEL_DEBUG) { \
            if (HAP_ENABLE_BUFFER_LOGS) { \
                HAPLogBufferDebugInternal(log, bytes, numBytes, __VA_ARGS__); \
            } else { \
                HAPLogDebugInternal(log, __VA_ARGS__); \
            } \
        } \
    } while (0)

/**
 * Logs the contents of a buffer and an error-level message.
 *
 * @param      log                  Log object.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 */
#define HAPLogBufferError(log, bytes, numBytes, ...) \
    do { \
        if (HAP_LOG_LEVEL >= HAP_LOG_LEVEL_ERROR) { \
            HAPLogBufferErrorInternal(log, bytes, numBytes, __VA_ARGS__); \
        } \
    } while (0)

/**
 * Logs the contents of a buffer and a fault-level message.
 *
 * @param      log                  Log object.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 */
#define HAPLogBufferFault(log, bytes, numBytes, ...) \
    do { \
        if (HAP_LOG_LEVEL >= HAP_LOG_LEVEL_FAULT) { \
            HAPLogBufferFaultInternal(log, bytes, numBytes, __VA_ARGS__); \
        } \
    } while (0)

/**
 * Logs a message at a specific logging level.
 *
 * @param      log                  Log object.
 * @param      type                 A log type constant, indicating the level of logging to perform.
 */
#define HAPLogWithType(log, type, ...) \
    do { \
        switch (type) { \
            case kHAPLogType_Debug: { \
                HAPLogDebug(log, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Info: { \
                HAPLogInfo(log, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Default: { \
                HAPLog(log, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Error: { \
                HAPLogError(log, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Fault: { \
                HAPLogFault(log, __VA_ARGS__); \
                break; \
            } \
            default: \
                HAPFatalError(); \
        } \
    } while (0)

/**
 * Logs a default-level message.
 *
 * @param      log                  Log object.
 */
#define HAPLog(log, ...) \
    do { \
        if (HAP_LOG_LEVEL >= HAP_LOG_LEVEL_DEFAULT) { \
            HAPLogInternal(log, __VA_ARGS__); \
        } \
    } while (0)

/**
 * Logs an info-level message.
 *
 * @param      log                  Log object.
 */
#define HAPLogInfo(log, ...) \
    do { \
        if (HAP_LOG_LEVEL >= HAP_LOG_LEVEL_INFO) { \
            HAPLogInfoInternal(log, __VA_ARGS__); \
        } \
    } while (0)

/**
 * Logs a debug-level message.
 *
 * @param      log                  Log object.
 */
#define HAPLogDebug(log, ...) \
    do { \
        if (HAP_LOG_LEVEL >= HAP_LOG_LEVEL_DEBUG) { \
            HAPLogDebugInternal(log, __VA_ARGS__); \
        } \
    } while (0)

/**
 * Logs an error-level message.
 *
 * @param      log                  Log object.
 */
#define HAPLogError(log, ...) \
    do { \
        if (HAP_LOG_LEVEL >= HAP_LOG_LEVEL_ERROR) { \
            HAPLogErrorInternal(log, __VA_ARGS__); \
        } \
    } while (0)

/**
 * Logs an info-level message with a va_list
 *
 * @param      log                  Log object.
 */
#define HAPVLogInfo(log, fmt, args) \
    do { \
        if (HAP_LOG_LEVEL >= HAP_LOG_LEVEL_INFO) { \
            HAPVLogInfoInternal(log, fmt, args); \
        } \
    } while (0)

/**
 * Logs a debug-level message with a va_list
 *
 * @param      log                  Log object.
 */
#define HAPVLogDebug(log, fmt, args) \
    do { \
        if (HAP_LOG_LEVEL >= HAP_LOG_LEVEL_DEBUG) { \
            HAPVLogDebugInternal(log, fmt, args); \
        } \
    } while (0)

/**
 * Logs an error-level message with a va_list
 *
 * @param      log                  Log object.
 */
#define HAPVLogError(log, fmt, args) \
    do { \
        if (HAP_LOG_LEVEL >= HAP_LOG_LEVEL_ERROR) { \
            HAPVLogErrorInternal(log, fmt, args); \
        } \
    } while (0)

/**
 * Logs a fault-level message.
 *
 * @param      log                  Log object.
 */
#define HAPLogFault(log, ...) \
    do { \
        if (HAP_LOG_LEVEL >= HAP_LOG_LEVEL_FAULT) { \
            HAPLogFaultInternal(log, __VA_ARGS__); \
        } \
    } while (0)

/**
 * Logs the contents of a buffer and a message at a specific logging level that may contain sensitive information.
 *
 * @param      log                  Log object.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 * @param      type                 A log type constant, indicating the level of logging to perform.
 */
#define HAPLogSensitiveBufferWithType(log, bytes, numBytes, type, ...) \
    do { \
        switch (type) { \
            case kHAPLogType_Debug: { \
                HAPLogSensitiveBufferDebug(log, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Info: { \
                HAPLogSensitiveBufferInfo(log, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Default: { \
                HAPLogSensitiveBuffer(log, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Error: { \
                HAPLogSensitiveBufferError(log, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Fault: { \
                HAPLogSensitiveBufferFault(log, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
        } \
    } while (0)

/**
 * Logs the contents of a buffer and a default-level message that may contain sensitive information.
 *
 * @param      log                  Log object.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 */
#define HAPLogSensitiveBuffer(log, bytes, numBytes, ...) \
    do { \
        if (HAP_LOG_SENSITIVE) { \
            HAPLogBuffer(log, bytes, numBytes, __VA_ARGS__); \
        } else { \
            HAP_DIAGNOSTIC_PUSH \
            HAP_DIAGNOSTIC_IGNORED_CLANG("-Wformat-extra-args") \
            HAP_DIAGNOSTIC_IGNORED_GCC("-Wformat-extra-args") \
            HAP_DIAGNOSTIC_IGNORED_ARMCC(225) \
            HAPLog(log, "<private> %s", __VA_ARGS__); \
            HAP_DIAGNOSTIC_POP \
        } \
    } while (0)

/**
 * Logs the contents of a buffer and an info-level message that may contain sensitive information.
 *
 * @param      log                  Log object.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 */
#define HAPLogSensitiveBufferInfo(log, bytes, numBytes, ...) \
    do { \
        if (HAP_LOG_SENSITIVE) { \
            HAPLogBufferInfo(log, bytes, numBytes, __VA_ARGS__); \
        } else { \
            HAP_DIAGNOSTIC_PUSH \
            HAP_DIAGNOSTIC_IGNORED_CLANG("-Wformat-extra-args") \
            HAP_DIAGNOSTIC_IGNORED_GCC("-Wformat-extra-args") \
            HAP_DIAGNOSTIC_IGNORED_ARMCC(225) \
            HAPLogInfo(log, "<private> %s", __VA_ARGS__); \
            HAP_DIAGNOSTIC_POP \
        } \
    } while (0)

/**
 * Logs the contents of a buffer and a debug-level message that may contain sensitive information.
 *
 * @param      log                  Log object.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 */
#define HAPLogSensitiveBufferDebug(log, bytes, numBytes, ...) \
    do { \
        if (HAP_LOG_SENSITIVE) { \
            HAPLogBufferDebug(log, bytes, numBytes, __VA_ARGS__); \
        } else { \
            HAP_DIAGNOSTIC_PUSH \
            HAP_DIAGNOSTIC_IGNORED_CLANG("-Wformat-extra-args") \
            HAP_DIAGNOSTIC_IGNORED_GCC("-Wformat-extra-args") \
            HAP_DIAGNOSTIC_IGNORED_ARMCC(225) \
            HAPLogDebug(log, "<private> %s", __VA_ARGS__); \
            HAP_DIAGNOSTIC_POP \
        } \
    } while (0)

/**
 * Logs the contents of a buffer and an error-level message that may contain sensitive information.
 *
 * @param      log                  Log object.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 */
#define HAPLogSensitiveBufferError(log, bytes, numBytes, ...) \
    do { \
        if (HAP_LOG_SENSITIVE) { \
            HAPLogBufferError(log, bytes, numBytes, __VA_ARGS__); \
        } else { \
            HAP_DIAGNOSTIC_PUSH \
            HAP_DIAGNOSTIC_IGNORED_CLANG("-Wformat-extra-args") \
            HAP_DIAGNOSTIC_IGNORED_GCC("-Wformat-extra-args") \
            HAP_DIAGNOSTIC_IGNORED_ARMCC(225) \
            HAPLogError(log, "<private> %s", __VA_ARGS__); \
            HAP_DIAGNOSTIC_POP \
        } \
    } while (0)

/**
 * Logs the contents of a buffer and a fault-level message that may contain sensitive information.
 *
 * @param      log                  Log object.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 */
#define HAPLogSensitiveBufferFault(log, bytes, numBytes, ...) \
    do { \
        if (HAP_LOG_SENSITIVE) { \
            HAPLogBufferFault(log, bytes, numBytes, __VA_ARGS__); \
        } else { \
            HAP_DIAGNOSTIC_PUSH \
            HAP_DIAGNOSTIC_IGNORED_CLANG("-Wformat-extra-args") \
            HAP_DIAGNOSTIC_IGNORED_GCC("-Wformat-extra-args") \
            HAP_DIAGNOSTIC_IGNORED_ARMCC(225) \
            HAPLogFault(log, "<private> %s", __VA_ARGS__); \
            HAP_DIAGNOSTIC_POP \
        } \
    } while (0)

/**
 * Logs a message at a specific logging level that may contain sensitive information.
 *
 * @param      log                  Log object.
 * @param      type                 A log type constant, indicating the level of logging to perform.
 */
#define HAPLogSensitiveWithType(log, type, ...) \
    do { \
        switch (type) { \
            case kHAPLogType_Debug: { \
                HAPLogSensitiveDebug(log, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Info: { \
                HAPLogSensitiveInfo(log, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Default: { \
                HAPLogSensitive(log, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Error: { \
                HAPLogSensitiveError(log, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Fault: { \
                HAPLogSensitiveFault(log, __VA_ARGS__); \
                break; \
            } \
        } \
    } while (0)

/**
 * Logs a default-level message that may contain sensitive information.
 *
 * @param      log                  Log object.
 */
#define HAPLogSensitive(log, ...) \
    do { \
        if (HAP_LOG_SENSITIVE) { \
            HAPLog(log, __VA_ARGS__); \
        } else { \
            HAP_DIAGNOSTIC_PUSH \
            HAP_DIAGNOSTIC_IGNORED_CLANG("-Wformat-extra-args") \
            HAP_DIAGNOSTIC_IGNORED_GCC("-Wformat-extra-args") \
            HAP_DIAGNOSTIC_IGNORED_ARMCC(225) \
            HAPLog(log, "<private> %s", __VA_ARGS__); \
            HAP_DIAGNOSTIC_POP \
        } \
    } while (0)

/**
 * Logs an info-level message that may contain sensitive information.
 *
 * @param      log                  Log object.
 */
#define HAPLogSensitiveInfo(log, ...) \
    do { \
        if (HAP_LOG_SENSITIVE) { \
            HAPLogInfo(log, __VA_ARGS__); \
        } else { \
            HAP_DIAGNOSTIC_PUSH \
            HAP_DIAGNOSTIC_IGNORED_CLANG("-Wformat-extra-args") \
            HAP_DIAGNOSTIC_IGNORED_GCC("-Wformat-extra-args") \
            HAP_DIAGNOSTIC_IGNORED_ARMCC(225) \
            HAPLogInfo(log, "<private> %s", __VA_ARGS__); \
            HAP_DIAGNOSTIC_POP \
        } \
    } while (0)

/**
 * Logs a debug-level message that may contain sensitive information.
 *
 * @param      log                  Log object.
 */
#define HAPLogSensitiveDebug(log, ...) \
    do { \
        if (HAP_LOG_SENSITIVE) { \
            HAPLogDebug(log, __VA_ARGS__); \
        } else { \
            HAP_DIAGNOSTIC_PUSH \
            HAP_DIAGNOSTIC_IGNORED_CLANG("-Wformat-extra-args") \
            HAP_DIAGNOSTIC_IGNORED_GCC("-Wformat-extra-args") \
            HAP_DIAGNOSTIC_IGNORED_ARMCC(225) \
            HAPLogDebug(log, "<private> %s", __VA_ARGS__); \
            HAP_DIAGNOSTIC_POP \
        } \
    } while (0)

/**
 * Logs an error-level message that may contain sensitive information.
 *
 * @param      log                  Log object.
 */
#define HAPLogSensitiveError(log, ...) \
    do { \
        if (HAP_LOG_SENSITIVE) { \
            HAPLogError(log, __VA_ARGS__); \
        } else { \
            HAP_DIAGNOSTIC_PUSH \
            HAP_DIAGNOSTIC_IGNORED_CLANG("-Wformat-extra-args") \
            HAP_DIAGNOSTIC_IGNORED_GCC("-Wformat-extra-args") \
            HAP_DIAGNOSTIC_IGNORED_ARMCC(225) \
            HAPLogError(log, "<private> %s", __VA_ARGS__); \
            HAP_DIAGNOSTIC_POP \
        } \
    } while (0)

/**
 * Logs a fault-level message that may contain sensitive information.
 *
 * @param      log                  Log object.
 */
#define HAPLogSensitiveFault(log, ...) \
    do { \
        if (HAP_LOG_SENSITIVE) { \
            HAPLogFault(log, __VA_ARGS__); \
        } else { \
            HAP_DIAGNOSTIC_PUSH \
            HAP_DIAGNOSTIC_IGNORED_CLANG("-Wformat-extra-args") \
            HAP_DIAGNOSTIC_IGNORED_GCC("-Wformat-extra-args") \
            HAP_DIAGNOSTIC_IGNORED_ARMCC(225) \
            HAPLogFault(log, "<private> %s", __VA_ARGS__); \
            HAP_DIAGNOSTIC_POP \
        } \
    } while (0)

//----------------------------------------------------------------------------------------------------------------------
// Internal functions. Do not use directly.

/**@cond */
HAP_PRINTFLIKE(4, 5)
void HAPLogBufferInternal(
        const HAPLogObject* _Nullable log,
        const void* _Nullable bufferBytes,
        size_t numBufferBytes,
        const char* format,
        ...);
HAP_DISALLOW_USE(HAPLogBufferInternal)

HAP_PRINTFLIKE(4, 5)
void HAPLogBufferInfoInternal(
        const HAPLogObject* _Nullable log,
        const void* _Nullable bufferBytes,
        size_t numBufferBytes,
        const char* format,
        ...);
HAP_DISALLOW_USE(HAPLogBufferInfoInternal)

HAP_PRINTFLIKE(4, 5)
void HAPLogBufferDebugInternal(
        const HAPLogObject* _Nullable log,
        const void* _Nullable bufferBytes,
        size_t numBufferBytes,
        const char* format,
        ...);
HAP_DISALLOW_USE(HAPLogBufferDebugInternal)

HAP_PRINTFLIKE(4, 5)
void HAPLogBufferErrorInternal(
        const HAPLogObject* _Nullable log,
        const void* _Nullable bufferBytes,
        size_t numBufferBytes,
        const char* format,
        ...);
HAP_DISALLOW_USE(HAPLogBufferErrorInternal)

HAP_PRINTFLIKE(4, 5)
void HAPLogBufferFaultInternal(
        const HAPLogObject* _Nullable log,
        const void* _Nullable bufferBytes,
        size_t numBufferBytes,
        const char* format,
        ...);
HAP_DISALLOW_USE(HAPLogBufferFaultInternal)

HAP_PRINTFLIKE(2, 3)
void HAPLogInternal(const HAPLogObject* _Nullable log, const char* format, ...);
HAP_DISALLOW_USE(HAPLogInternal)

HAP_PRINTFLIKE(2, 3)
void HAPLogInfoInternal(const HAPLogObject* _Nullable log, const char* format, ...);
HAP_DISALLOW_USE(HAPLogInfoInternal)

HAP_PRINTFLIKE(2, 3)
void HAPLogDebugInternal(const HAPLogObject* _Nullable log, const char* format, ...);
HAP_DISALLOW_USE(HAPLogDebugInternal)

HAP_PRINTFLIKE(2, 3)
void HAPLogErrorInternal(const HAPLogObject* _Nullable log, const char* format, ...);
HAP_DISALLOW_USE(HAPLogErrorInternal)

void HAPVLogInfoInternal(const HAPLogObject* _Nullable log, const char* format, va_list args);
HAP_DISALLOW_USE(HAPVLogInfoInternal)

void HAPVLogDebugInternal(const HAPLogObject* _Nullable log, const char* format, va_list args);
HAP_DISALLOW_USE(HAPVLogDebugInternal)

void HAPVLogErrorInternal(const HAPLogObject* _Nullable log, const char* format, va_list args);
HAP_DISALLOW_USE(HAPVLogErrorInternal)

HAP_PRINTFLIKE(2, 3)
void HAPLogFaultInternal(const HAPLogObject* _Nullable log, const char* format, ...);
HAP_DISALLOW_USE(HAPLogFaultInternal)
/**@endcond */

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
