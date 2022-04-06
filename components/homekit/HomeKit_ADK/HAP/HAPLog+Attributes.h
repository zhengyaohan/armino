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
// Copyright (C) 2015-2020 Apple Inc. All Rights Reserved.

#ifndef HAP_LOG_ATTRIBUTES_H
#define HAP_LOG_ATTRIBUTES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPBase.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

// ISO C99 requires at least one argument for the "..." in a variadic macro.
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC system_header
#endif

/**
 * Logs the contents of a buffer and a message related to an accessory at a specific logging level.
 *
 * @param      logObject            Log object.
 * @param      accessory            Accessory.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 * @param      type                 A log type constant, indicating the level of logging to perform.
 */
#define HAPLogAccessoryBufferWithType(logObject, accessory, bytes, numBytes, type, ...) \
    do { \
        switch (type) { \
            case kHAPLogType_Debug: { \
                HAPLogAccessoryBufferDebug(logObject, accessory, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Info: { \
                HAPLogAccessoryBufferInfo(logObject, accessory, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Default: { \
                HAPLogAccessoryBuffer(logObject, accessory, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Error: { \
                HAPLogAccessoryBufferError(logObject, accessory, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Fault: { \
                HAPLogAccessoryBufferFault(logObject, accessory, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
        } \
    } while (0)

/**
 * Logs the contents of a buffer and a default-level message related to an accessory.
 *
 * @param      logObject            Log object.
 * @param      accessory            Accessory.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogAccessoryBuffer(logObject, accessory, bytes, numBytes, format, ...) \
    HAPLogBuffer( \
            logObject, \
            bytes, \
            numBytes, \
            "[%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            ##__VA_ARGS__)

/**
 * Logs the contents of a buffer and an info-level message related to an accessory.
 *
 * @param      logObject            Log object.
 * @param      accessory            Accessory.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogAccessoryBufferInfo(logObject, accessory, bytes, numBytes, format, ...) \
    HAPLogBufferInfo( \
            logObject, \
            bytes, \
            numBytes, \
            "[%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            ##__VA_ARGS__)

/**
 * Logs the contents of a buffer and a debug-level message related to an accessory.
 *
 * @param      logObject            Log object.
 * @param      accessory            Accessory.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogAccessoryBufferDebug(logObject, accessory, bytes, numBytes, format, ...) \
    HAPLogBufferDebug( \
            logObject, \
            bytes, \
            numBytes, \
            "[%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            ##__VA_ARGS__)

/**
 * Logs the contents of a buffer and an error-level message related to an accessory.
 *
 * @param      logObject            Log object.
 * @param      accessory            Accessory.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogAccessoryBufferError(logObject, accessory, bytes, numBytes, format, ...) \
    HAPLogBufferError( \
            logObject, \
            bytes, \
            numBytes, \
            "[%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            ##__VA_ARGS__)

/**
 * Logs the contents of a buffer and a fault-level message related to an accessory.
 *
 * @param      logObject            Log object.
 * @param      accessory            Accessory.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogAccessoryBufferFault(logObject, accessory, bytes, numBytes, format, ...) \
    HAPLogBufferFault( \
            logObject, \
            bytes, \
            numBytes, \
            "[%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            ##__VA_ARGS__)

/**
 * Logs a message related to an accessory at a specific logging level.
 *
 * @param      logObject            Log object.
 * @param      accessory            Accessory.
 * @param      type                 A log type constant, indicating the level of logging to perform.
 */
#define HAPLogAccessoryWithType(logObject, accessory, type, ...) \
    do { \
        switch (type) { \
            case kHAPLogType_Debug: { \
                HAPLogAccessoryDebug(logObject, accessory, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Info: { \
                HAPLogAccessoryInfo(logObject, accessory, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Default: { \
                HAPLogAccessory(logObject, accessory, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Error: { \
                HAPLogAccessoryError(logObject, accessory, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Fault: { \
                HAPLogAccessoryFault(logObject, accessory, __VA_ARGS__); \
                break; \
            } \
        } \
    } while (0)

/**
 * Logs a default-level message related to an accessory.
 *
 * @param      logObject            Log object.
 * @param      accessory            Accessory.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogAccessory(logObject, accessory, format, ...) \
    HAPLog(logObject, "[%016llX %s] " format, (unsigned long long) (accessory)->aid, (accessory)->name, ##__VA_ARGS__)

/**
 * Logs an info-level message related to an accessory.
 *
 * @param      logObject            Log object.
 * @param      accessory            Accessory.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogAccessoryInfo(logObject, accessory, format, ...) \
    HAPLogInfo( \
            logObject, \
            "[%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            ##__VA_ARGS__)

/**
 * Logs a debug-level message related to an accessory.
 *
 * @param      logObject            Log object.
 * @param      accessory            Accessory.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogAccessoryDebug(logObject, accessory, format, ...) \
    HAPLogDebug( \
            logObject, \
            "[%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            ##__VA_ARGS__)

/**
 * Logs an error-level message related to an accessory.
 *
 * @param      logObject            Log object.
 * @param      accessory            Accessory.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogAccessoryError(logObject, accessory, format, ...) \
    HAPLogError( \
            logObject, \
            "[%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            ##__VA_ARGS__)

/**
 * Logs a fault-level message related to an accessory.
 *
 * @param      logObject            Log object.
 * @param      accessory            Accessory.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogAccessoryFault(logObject, accessory, format, ...) \
    HAPLogFault( \
            logObject, \
            "[%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            ##__VA_ARGS__)

/**
 * Logs the contents of a buffer and a message related to an accessory
 * at a specific logging level that may contain sensitive information.
 *
 * @param      logObject            Log object.
 * @param      accessory            Accessory.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 * @param      type                 A log type constant, indicating the level of logging to perform.
 */
#define HAPLogSensitiveAccessoryBufferWithType(logObject, accessory, bytes, numBytes, type, ...) \
    do { \
        switch (type) { \
            case kHAPLogType_Debug: { \
                HAPLogSensitiveAccessoryBufferDebug(logObject, accessory, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Info: { \
                HAPLogSensitiveAccessoryBufferInfo(logObject, accessory, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Default: { \
                HAPLogSensitiveAccessoryBuffer(logObject, accessory, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Error: { \
                HAPLogSensitiveAccessoryBufferError(logObject, accessory, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Fault: { \
                HAPLogSensitiveAccessoryBufferFault(logObject, accessory, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
        } \
    } while (0)

/**
 * Logs the contents of a buffer and a default-level message related to an accessory
 * that may contain sensitive information.
 *
 * @param      logObject            Log object.
 * @param      accessory            Accessory.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogSensitiveAccessoryBuffer(logObject, accessory, bytes, numBytes, format, ...) \
    HAPLogSensitiveBuffer( \
            logObject, \
            bytes, \
            numBytes, \
            "[%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            ##__VA_ARGS__)

/**
 * Logs the contents of a buffer and an info-level message related to an accessory
 * that may contain sensitive information.
 *
 * @param      logObject            Log object.
 * @param      accessory            Accessory.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogSensitiveAccessoryBufferInfo(logObject, accessory, bytes, numBytes, format, ...) \
    HAPLogSensitiveBufferInfo( \
            logObject, \
            bytes, \
            numBytes, \
            "[%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            ##__VA_ARGS__)

/**
 * Logs the contents of a buffer and a debug-level message related to an accessory
 * that may contain sensitive information.
 *
 * @param      logObject            Log object.
 * @param      accessory            Accessory.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogSensitiveAccessoryBufferDebug(logObject, accessory, bytes, numBytes, format, ...) \
    HAPLogSensitiveBufferDebug( \
            logObject, \
            bytes, \
            numBytes, \
            "[%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            ##__VA_ARGS__)

/**
 * Logs the contents of a buffer and an error-level message related to an accessory
 * that may contain sensitive information.
 *
 * @param      logObject            Log object.
 * @param      accessory            Accessory.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogSensitiveAccessoryBufferError(logObject, accessory, bytes, numBytes, format, ...) \
    HAPLogSensitiveBufferError( \
            logObject, \
            bytes, \
            numBytes, \
            "[%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            ##__VA_ARGS__)

/**
 * Logs the contents of a buffer and a fault-level message related to an accessory
 * that may contain sensitive information.
 *
 * @param      logObject            Log object.
 * @param      accessory            Accessory.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogSensitiveAccessoryBufferFault(logObject, accessory, bytes, numBytes, format, ...) \
    HAPLogSensitiveBufferFault( \
            logObject, \
            bytes, \
            numBytes, \
            "[%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            ##__VA_ARGS__)

/**
 * Logs a message related to an accessory at a specific logging level that may contain sensitive information.
 *
 * @param      logObject            Log object.
 * @param      accessory            Accessory.
 * @param      type                 A log type constant, indicating the level of logging to perform.
 */
#define HAPLogSensitiveAccessoryWithType(logObject, accessory, type, ...) \
    do { \
        switch (type) { \
            case kHAPLogType_Debug: { \
                HAPLogSensitiveAccessoryDebug(logObject, accessory, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Info: { \
                HAPLogSensitiveAccessoryInfo(logObject, accessory, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Default: { \
                HAPLogSensitiveAccessory(logObject, accessory, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Error: { \
                HAPLogSensitiveAccessoryError(logObject, accessory, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Fault: { \
                HAPLogSensitiveAccessoryFault(logObject, accessory, __VA_ARGS__); \
                break; \
            } \
        } \
    } while (0)

/**
 * Logs a default-level message related to an accessory that may contain sensitive information.
 *
 * @param      logObject            Log object.
 * @param      accessory            Accessory.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogSensitiveAccessory(logObject, accessory, format, ...) \
    HAPLogSensitive( \
            logObject, \
            "[%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            ##__VA_ARGS__)

/**
 * Logs an info-level message related to an accessory that may contain sensitive information.
 *
 * @param      logObject            Log object.
 * @param      accessory            Accessory.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogSensitiveAccessoryInfo(logObject, accessory, format, ...) \
    HAPLogSensitiveInfo( \
            logObject, \
            "[%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            ##__VA_ARGS__)

/**
 * Logs a debug-level message related to an accessory that may contain sensitive information.
 *
 * @param      logObject            Log object.
 * @param      accessory            Accessory.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogSensitiveAccessoryDebug(logObject, accessory, format, ...) \
    HAPLogSensitiveDebug( \
            logObject, \
            "[%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            ##__VA_ARGS__)

/**
 * Logs an error-level message related to an accessory that may contain sensitive information.
 *
 * @param      logObject            Log object.
 * @param      accessory            Accessory.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogSensitiveAccessoryError(logObject, accessory, format, ...) \
    HAPLogSensitiveError( \
            logObject, \
            "[%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            ##__VA_ARGS__)

/**
 * Logs a fault-level message related to an accessory that may contain sensitive information.
 *
 * @param      logObject            Log object.
 * @param      accessory            Accessory.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogSensitiveAccessoryFault(logObject, accessory, format, ...) \
    HAPLogSensitive( \
            logObject, \
            "[%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            ##__VA_ARGS__)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Logs the contents of a buffer and a message related to a service at a specific logging level.
 *
 * @param      logObject            Log object.
 * @param      service              Service.
 * @param      accessory            The accessory that provides the service.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 * @param      type                 A log type constant, indicating the level of logging to perform.
 */
#define HAPLogServiceBufferWithType(logObject, service, accessory, bytes, numBytes, type, ...) \
    do { \
        switch (type) { \
            case kHAPLogType_Debug: { \
                HAPLogServiceBufferDebug(logObject, service, accessory, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Info: { \
                HAPLogServiceBufferInfo(logObject, service, accessory, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Default: { \
                HAPLogServiceBuffer(logObject, service, accessory, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Error: { \
                HAPLogServiceBufferError(logObject, service, accessory, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Fault: { \
                HAPLogServiceBufferFault(logObject, service, accessory, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
        } \
    } while (0)

/**
 * Logs the contents of a buffer and a default-level message related to a service.
 *
 * @param      logObject            Log object.
 * @param      service              Service.
 * @param      accessory            The accessory that provides the service.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogServiceBuffer(logObject, service, accessory, bytes, numBytes, format, ...) \
    HAPLogBuffer( \
            logObject, \
            bytes, \
            numBytes, \
            "[%016llX %s] [%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            (unsigned long long) (service)->iid, \
            (service)->debugDescription, \
            ##__VA_ARGS__)

/**
 * Logs the contents of a buffer and an info-level message related to a service.
 *
 * @param      logObject            Log object.
 * @param      service              Service.
 * @param      accessory            The accessory that provides the service.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogServiceBufferInfo(logObject, service, accessory, bytes, numBytes, format, ...) \
    HAPLogBufferInfo( \
            logObject, \
            bytes, \
            numBytes, \
            "[%016llX %s] [%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            (unsigned long long) (service)->iid, \
            (service)->debugDescription, \
            ##__VA_ARGS__)

/**
 * Logs the contents of a buffer and a debug-level message related to a service.
 *
 * @param      logObject            Log object.
 * @param      service              Service.
 * @param      accessory            The accessory that provides the service.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogServiceBufferDebug(logObject, service, accessory, bytes, numBytes, format, ...) \
    HAPLogBufferDebug( \
            logObject, \
            bytes, \
            numBytes, \
            "[%016llX %s] [%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            (unsigned long long) (service)->iid, \
            (service)->debugDescription, \
            ##__VA_ARGS__)

/**
 * Logs the contents of a buffer and an error-level message related to a service.
 *
 * @param      logObject            Log object.
 * @param      service              Service.
 * @param      accessory            The accessory that provides the service.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogServiceBufferError(logObject, service, accessory, bytes, numBytes, format, ...) \
    HAPLogBufferError( \
            logObject, \
            bytes, \
            numBytes, \
            "[%016llX %s] [%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            (unsigned long long) (service)->iid, \
            (service)->debugDescription, \
            ##__VA_ARGS__)

/**
 * Logs the contents of a buffer and a fault-level message related to a service.
 *
 * @param      logObject            Log object.
 * @param      service              Service.
 * @param      accessory            The accessory that provides the service.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogServiceBufferFault(logObject, service, accessory, bytes, numBytes, format, ...) \
    HAPLogBufferFault( \
            logObject, \
            bytes, \
            numBytes, \
            "[%016llX %s] [%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            (unsigned long long) (service)->iid, \
            (service)->debugDescription, \
            ##__VA_ARGS__)

/**
 * Logs a message related to a service at a specific logging level.
 *
 * @param      logObject            Log object.
 * @param      service              Service.
 * @param      accessory            The accessory that provides the service.
 * @param      type                 A log type constant, indicating the level of logging to perform.
 */
#define HAPLogServiceWithType(logObject, service, accessory, type, ...) \
    do { \
        switch (type) { \
            case kHAPLogType_Debug: { \
                HAPLogServiceDebug(logObject, service, accessory, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Info: { \
                HAPLogServiceInfo(logObject, service, accessory, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Default: { \
                HAPLogService(logObject, service, accessory, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Error: { \
                HAPLogServiceError(logObject, service, accessory, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Fault: { \
                HAPLogServiceFault(logObject, service, accessory, __VA_ARGS__); \
                break; \
            } \
        } \
    } while (0)

/**
 * Logs a default-level message related to a service.
 *
 * @param      logObject            Log object.
 * @param      service              Service.
 * @param      accessory            The accessory that provides the service.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogService(logObject, service, accessory, format, ...) \
    HAPLog(logObject, \
           "[%016llX %s] [%016llX %s] " format, \
           (unsigned long long) (accessory)->aid, \
           (accessory)->name, \
           (unsigned long long) (service)->iid, \
           (service)->debugDescription, \
           ##__VA_ARGS__)

/**
 * Logs an info-level message related to a service.
 *
 * @param      logObject            Log object.
 * @param      service              Service.
 * @param      accessory            The accessory that provides the service.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogServiceInfo(logObject, service, accessory, format, ...) \
    HAPLogInfo( \
            logObject, \
            "[%016llX %s] [%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            (unsigned long long) (service)->iid, \
            (service)->debugDescription, \
            ##__VA_ARGS__)

/**
 * Logs a debug-level message related to a service.
 *
 * @param      logObject            Log object.
 * @param      service              Service.
 * @param      accessory            The accessory that provides the service.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogServiceDebug(logObject, service, accessory, format, ...) \
    HAPLogDebug( \
            logObject, \
            "[%016llX %s] [%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            (unsigned long long) (service)->iid, \
            (service)->debugDescription, \
            ##__VA_ARGS__)

/**
 * Logs an error-level message related to a service.
 *
 * @param      logObject            Log object.
 * @param      service              Service.
 * @param      accessory            The accessory that provides the service.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogServiceError(logObject, service, accessory, format, ...) \
    HAPLogError( \
            logObject, \
            "[%016llX %s] [%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            (unsigned long long) (service)->iid, \
            (service)->debugDescription, \
            ##__VA_ARGS__)

/**
 * Logs a fault-level message related to a service.
 *
 * @param      logObject            Log object.
 * @param      service              Service.
 * @param      accessory            The accessory that provides the service.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogServiceFault(logObject, service, accessory, format, ...) \
    HAPLogFault( \
            logObject, \
            "[%016llX %s] [%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            (unsigned long long) (service)->iid, \
            (service)->debugDescription, \
            ##__VA_ARGS__)

/**
 * Logs the contents of a buffer and a message related to a service
 * at a specific logging level that may contain sensitive information.
 *
 * @param      logObject            Log object.
 * @param      service              Service.
 * @param      accessory            The accessory that provides the service.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 * @param      type                 A log type constant, indicating the level of logging to perform.
 */
#define HAPLogSensitiveServiceBufferWithType(logObject, service, accessory, bytes, numBytes, type, ...) \
    do { \
        switch (type) { \
            case kHAPLogType_Debug: { \
                HAPLogSensitiveServiceBufferDebug(logObject, service, accessory, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Info: { \
                HAPLogSensitiveServiceBufferInfo(logObject, service, accessory, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Default: { \
                HAPLogSensitiveServiceBuffer(logObject, service, accessory, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Error: { \
                HAPLogSensitiveServiceBufferError(logObject, service, accessory, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Fault: { \
                HAPLogSensitiveServiceBufferFault(logObject, service, accessory, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
        } \
    } while (0)

/**
 * Logs the contents of a buffer and a default-level message related to a service
 * that may contain sensitive information.
 *
 * @param      logObject            Log object.
 * @param      service              Service.
 * @param      accessory            The accessory that provides the service.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogSensitiveServiceBuffer(logObject, service, accessory, bytes, numBytes, format, ...) \
    HAPLogSensitiveBuffer( \
            logObject, \
            bytes, \
            numBytes, \
            "[%016llX %s] [%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            (unsigned long long) (service)->iid, \
            (service)->debugDescription, \
            ##__VA_ARGS__)

/**
 * Logs the contents of a buffer and an info-level message related to a service
 * that may contain sensitive information.
 *
 * @param      logObject            Log object.
 * @param      service              Service.
 * @param      accessory            The accessory that provides the service.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogSensitiveServiceBufferInfo(logObject, service, accessory, bytes, numBytes, format, ...) \
    HAPLogSensitiveBufferInfo( \
            logObject, \
            bytes, \
            numBytes, \
            "[%016llX %s] [%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            (unsigned long long) (service)->iid, \
            (service)->debugDescription, \
            ##__VA_ARGS__)

/**
 * Logs the contents of a buffer and a debug-level message related to a service
 * that may contain sensitive information.
 *
 * @param      logObject            Log object.
 * @param      service              Service.
 * @param      accessory            The accessory that provides the service.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogSensitiveServiceBufferDebug(logObject, service, accessory, bytes, numBytes, format, ...) \
    HAPLogSensitiveBufferDebug( \
            logObject, \
            bytes, \
            numBytes, \
            "[%016llX %s] [%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            (unsigned long long) (service)->iid, \
            (service)->debugDescription, \
            ##__VA_ARGS__)

/**
 * Logs the contents of a buffer and an error-level message related to a service
 * that may contain sensitive information.
 *
 * @param      logObject            Log object.
 * @param      service              Service.
 * @param      accessory            The accessory that provides the service.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogSensitiveServiceBufferError(logObject, service, accessory, bytes, numBytes, format, ...) \
    HAPLogSensitiveBufferError( \
            logObject, \
            bytes, \
            numBytes, \
            "[%016llX %s] [%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            (unsigned long long) (service)->iid, \
            (service)->debugDescription, \
            ##__VA_ARGS__)

/**
 * Logs the contents of a buffer and a fault-level message related to a service
 * that may contain sensitive information.
 *
 * @param      logObject            Log object.
 * @param      service              Service.
 * @param      accessory            The accessory that provides the service.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogSensitiveServiceBufferFault(logObject, service, accessory, bytes, numBytes, format, ...) \
    HAPLogSensitiveBufferFault( \
            logObject, \
            bytes, \
            numBytes, \
            "[%016llX %s] [%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            (unsigned long long) (service)->iid, \
            (service)->debugDescription, \
            ##__VA_ARGS__)

/**
 * Logs a message related to a service at a specific logging level that may contain sensitive information.
 *
 * @param      logObject            Log object.
 * @param      service              Service.
 * @param      accessory            The accessory that provides the service.
 * @param      type                 A log type constant, indicating the level of logging to perform.
 */
#define HAPLogSensitiveServiceWithType(logObject, service, accessory, type, ...) \
    do { \
        switch (type) { \
            case kHAPLogType_Debug: { \
                HAPLogSensitiveServiceDebug(logObject, service, accessory, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Info: { \
                HAPLogSensitiveServiceInfo(logObject, service, accessory, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Default: { \
                HAPLogSensitiveService(logObject, service, accessory, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Error: { \
                HAPLogSensitiveServiceError(logObject, service, accessory, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Fault: { \
                HAPLogSensitiveServiceFault(logObject, service, accessory, __VA_ARGS__); \
                break; \
            } \
        } \
    } while (0)

/**
 * Logs a default-level message related to a service that may contain sensitive information.
 *
 * @param      logObject            Log object.
 * @param      service              Service.
 * @param      accessory            The accessory that provides the service.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogSensitiveService(logObject, service, accessory, format, ...) \
    HAPLogSensitive( \
            logObject, \
            "[%016llX %s] [%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            (unsigned long long) (service)->iid, \
            (service)->debugDescription, \
            ##__VA_ARGS__)

/**
 * Logs an info-level message related to a service that may contain sensitive information.
 *
 * @param      logObject            Log object.
 * @param      service              Service.
 * @param      accessory            The accessory that provides the service.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogSensitiveServiceInfo(logObject, service, accessory, format, ...) \
    HAPLogSensitiveInfo( \
            logObject, \
            "[%016llX %s] [%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            (unsigned long long) (service)->iid, \
            (service)->debugDescription, \
            ##__VA_ARGS__)

/**
 * Logs a debug-level message related to a service that may contain sensitive information.
 *
 * @param      logObject            Log object.
 * @param      service              Service.
 * @param      accessory            The accessory that provides the service.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogSensitiveServiceDebug(logObject, service, accessory, format, ...) \
    HAPLogSensitiveDebug( \
            logObject, \
            "[%016llX %s] [%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            (unsigned long long) (service)->iid, \
            (service)->debugDescription, \
            ##__VA_ARGS__)

/**
 * Logs an error-level message related to a service that may contain sensitive information.
 *
 * @param      logObject            Log object.
 * @param      service              Service.
 * @param      accessory            The accessory that provides the service.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogSensitiveServiceError(logObject, service, accessory, format, ...) \
    HAPLogSensitiveError( \
            logObject, \
            "[%016llX %s] [%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            (unsigned long long) (service)->iid, \
            (service)->debugDescription, \
            ##__VA_ARGS__)

/**
 * Logs a fault-level message related to a service that may contain sensitive information.
 *
 * @param      logObject            Log object.
 * @param      service              Service.
 * @param      accessory            The accessory that provides the service.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogSensitiveServiceFault(logObject, service, accessory, format, ...) \
    HAPLogSensitive( \
            logObject, \
            "[%016llX %s] [%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            (unsigned long long) (service)->iid, \
            (service)->debugDescription, \
            ##__VA_ARGS__)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Logs the contents of a buffer and a message related to a characteristic at a specific logging level.
 *
 * @param      logObject            Log object.
 * @param      characteristic       Characteristic.
 * @param      service              The service that contains the characteristic.
 * @param      accessory            The accessory that provides the service.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 * @param      type                 A log type constant, indicating the level of logging to perform.
 */
#define HAPLogCharacteristicBufferWithType(logObject, characteristic, service, accessory, bytes, numBytes, type, ...) \
    do { \
        switch (type) { \
            case kHAPLogType_Debug: { \
                HAPLogCharacteristicBufferDebug( \
                        logObject, characteristic, service, accessory, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Info: { \
                HAPLogCharacteristicBufferInfo( \
                        logObject, characteristic, service, accessory, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Default: { \
                HAPLogCharacteristicBuffer( \
                        logObject, characteristic, service, accessory, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Error: { \
                HAPLogCharacteristicBufferError( \
                        logObject, characteristic, service, accessory, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Fault: { \
                HAPLogCharacteristicBufferFault( \
                        logObject, characteristic, service, accessory, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
        } \
    } while (0)

/**
 * Logs the contents of a buffer and a default-level message related to a characteristic.
 *
 * @param      logObject            Log object.
 * @param      characteristic       Characteristic.
 * @param      service              The service that contains the characteristic.
 * @param      accessory            The accessory that provides the service.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogCharacteristicBuffer(logObject, characteristic, service, accessory, bytes, numBytes, format, ...) \
    HAPLogBuffer( \
            logObject, \
            bytes, \
            numBytes, \
            "[%016llX %s] [%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            (unsigned long long) ((const HAPBaseCharacteristic*) (characteristic))->iid, \
            ((const HAPBaseCharacteristic*) (characteristic))->debugDescription, \
            ##__VA_ARGS__)

/**
 * Logs the contents of a buffer and an info-level message related to a characteristic.
 *
 * @param      logObject            Log object.
 * @param      characteristic       Characteristic.
 * @param      service              The service that contains the characteristic.
 * @param      accessory            The accessory that provides the service.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogCharacteristicBufferInfo(logObject, characteristic, service, accessory, bytes, numBytes, format, ...) \
    HAPLogBufferInfo( \
            logObject, \
            bytes, \
            numBytes, \
            "[%016llX %s] [%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            (unsigned long long) ((const HAPBaseCharacteristic*) (characteristic))->iid, \
            ((const HAPBaseCharacteristic*) (characteristic))->debugDescription, \
            ##__VA_ARGS__)

/**
 * Logs the contents of a buffer and a debug-level message related to a characteristic.
 *
 * @param      logObject            Log object.
 * @param      characteristic       Characteristic.
 * @param      service              The service that contains the characteristic.
 * @param      accessory            The accessory that provides the service.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogCharacteristicBufferDebug(logObject, characteristic, service, accessory, bytes, numBytes, format, ...) \
    HAPLogBufferDebug( \
            logObject, \
            bytes, \
            numBytes, \
            "[%016llX %s] [%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            (unsigned long long) ((const HAPBaseCharacteristic*) (characteristic))->iid, \
            ((const HAPBaseCharacteristic*) (characteristic))->debugDescription, \
            ##__VA_ARGS__)

/**
 * Logs the contents of a buffer and an error-level message related to a characteristic.
 *
 * @param      logObject            Log object.
 * @param      characteristic       Characteristic.
 * @param      service              The service that contains the characteristic.
 * @param      accessory            The accessory that provides the service.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogCharacteristicBufferError(logObject, characteristic, service, accessory, bytes, numBytes, format, ...) \
    HAPLogBufferError( \
            logObject, \
            bytes, \
            numBytes, \
            "[%016llX %s] [%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            (unsigned long long) ((const HAPBaseCharacteristic*) (characteristic))->iid, \
            ((const HAPBaseCharacteristic*) (characteristic))->debugDescription, \
            ##__VA_ARGS__)

/**
 * Logs the contents of a buffer and a fault-level message related to a characteristic.
 *
 * @param      logObject            Log object.
 * @param      characteristic       Characteristic.
 * @param      service              The service that contains the characteristic.
 * @param      accessory            The accessory that provides the service.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogCharacteristicBufferFault(logObject, characteristic, service, accessory, bytes, numBytes, format, ...) \
    HAPLogBufferFault( \
            logObject, \
            bytes, \
            numBytes, \
            "[%016llX %s] [%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            (unsigned long long) ((const HAPBaseCharacteristic*) (characteristic))->iid, \
            ((const HAPBaseCharacteristic*) (characteristic))->debugDescription, \
            ##__VA_ARGS__)

/**
 * Logs a message related to a characteristic at a specific logging level.
 *
 * @param      logObject            Log object.
 * @param      characteristic       Characteristic.
 * @param      service              The service that contains the characteristic.
 * @param      accessory            The accessory that provides the service.
 * @param      type                 A log type constant, indicating the level of logging to perform.
 */
#define HAPLogCharacteristicWithType(logObject, characteristic, service, accessory, type, ...) \
    do { \
        switch (type) { \
            case kHAPLogType_Debug: { \
                HAPLogCharacteristicDebug(logObject, characteristic, service, accessory, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Info: { \
                HAPLogCharacteristicInfo(logObject, characteristic, service, accessory, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Default: { \
                HAPLogCharacteristic(logObject, characteristic, service, accessory, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Error: { \
                HAPLogCharacteristicError(logObject, characteristic, service, accessory, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Fault: { \
                HAPLogCharacteristicFault(logObject, characteristic, service, accessory, __VA_ARGS__); \
                break; \
            } \
        } \
    } while (0)

/**
 * Logs a default-level message related to a characteristic.
 *
 * @param      logObject            Log object.
 * @param      characteristic       Characteristic.
 * @param      service              The service that contains the characteristic.
 * @param      accessory            The accessory that provides the service.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogCharacteristic(logObject, characteristic, service, accessory, format, ...) \
    HAPLog(logObject, \
           "[%016llX %s] [%016llX %s] " format, \
           (unsigned long long) (accessory)->aid, \
           (accessory)->name, \
           (unsigned long long) ((const HAPBaseCharacteristic*) (characteristic))->iid, \
           ((const HAPBaseCharacteristic*) (characteristic))->debugDescription, \
           ##__VA_ARGS__)

/**
 * Logs an info-level message related to a characteristic.
 *
 * @param      logObject            Log object.
 * @param      characteristic       Characteristic.
 * @param      service              The service that contains the characteristic.
 * @param      accessory            The accessory that provides the service.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogCharacteristicInfo(logObject, characteristic, service, accessory, format, ...) \
    HAPLogInfo( \
            logObject, \
            "[%016llX %s] [%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            (unsigned long long) ((const HAPBaseCharacteristic*) (characteristic))->iid, \
            ((const HAPBaseCharacteristic*) (characteristic))->debugDescription, \
            ##__VA_ARGS__)

/**
 * Logs a debug-level message related to a characteristic.
 *
 * @param      logObject            Log object.
 * @param      characteristic       Characteristic.
 * @param      service              The service that contains the characteristic.
 * @param      accessory            The accessory that provides the service.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogCharacteristicDebug(logObject, characteristic, service, accessory, format, ...) \
    HAPLogDebug( \
            logObject, \
            "[%016llX %s] [%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            (unsigned long long) ((const HAPBaseCharacteristic*) (characteristic))->iid, \
            ((const HAPBaseCharacteristic*) (characteristic))->debugDescription, \
            ##__VA_ARGS__)

/**
 * Logs an error-level message related to a characteristic.
 *
 * @param      logObject            Log object.
 * @param      characteristic       Characteristic.
 * @param      service              The service that contains the characteristic.
 * @param      accessory            The accessory that provides the service.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogCharacteristicError(logObject, characteristic, service, accessory, format, ...) \
    HAPLogError( \
            logObject, \
            "[%016llX %s] [%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            (unsigned long long) ((const HAPBaseCharacteristic*) (characteristic))->iid, \
            ((const HAPBaseCharacteristic*) (characteristic))->debugDescription, \
            ##__VA_ARGS__)

/**
 * Logs a fault-level message related to a characteristic.
 *
 * @param      logObject            Log object.
 * @param      characteristic       Characteristic.
 * @param      service              The service that contains the characteristic.
 * @param      accessory            The accessory that provides the service.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogCharacteristicFault(logObject, characteristic, service, accessory, format, ...) \
    HAPLogFault( \
            logObject, \
            "[%016llX %s] [%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            (unsigned long long) ((const HAPBaseCharacteristic*) (characteristic))->iid, \
            ((const HAPBaseCharacteristic*) (characteristic))->debugDescription, \
            ##__VA_ARGS__)

/**
 * Logs the contents of a buffer and a message related to a characteristic
 * at a specific logging level that may contain sensitive information.
 *
 * @param      logObject            Log object.
 * @param      characteristic       Characteristic.
 * @param      service              The service that contains the characteristic.
 * @param      accessory            The accessory that provides the service.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 * @param      type                 A log type constant, indicating the level of logging to perform.
 */
#define HAPLogSensitiveCharacteristicBufferWithType( \
        logObject, characteristic, service, accessory, bytes, numBytes, type, ...) \
    do { \
        switch (type) { \
            case kHAPLogType_Debug: { \
                HAPLogSensitiveCharacteristicBufferDebug( \
                        logObject, characteristic, service, accessory, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Info: { \
                HAPLogSensitiveCharacteristicBufferInfo( \
                        logObject, characteristic, service, accessory, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Default: { \
                HAPLogSensitiveCharacteristicBuffer( \
                        logObject, characteristic, service, accessory, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Error: { \
                HAPLogSensitiveCharacteristicBufferError( \
                        logObject, characteristic, service, accessory, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Fault: { \
                HAPLogSensitiveCharacteristicBufferFault( \
                        logObject, characteristic, service, accessory, bytes, numBytes, __VA_ARGS__); \
                break; \
            } \
        } \
    } while (0)

/**
 * Logs the contents of a buffer and a default-level message related to a characteristic
 * that may contain sensitive information.
 *
 * @param      logObject            Log object.
 * @param      characteristic       Characteristic.
 * @param      service              The service that contains the characteristic.
 * @param      accessory            The accessory that provides the service.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogSensitiveCharacteristicBuffer( \
        logObject, characteristic, service, accessory, bytes, numBytes, format, ...) \
    HAPLogSensitiveBuffer( \
            logObject, \
            bytes, \
            numBytes, \
            "[%016llX %s] [%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            (unsigned long long) ((const HAPBaseCharacteristic*) (characteristic))->iid, \
            ((const HAPBaseCharacteristic*) (characteristic))->debugDescription, \
            ##__VA_ARGS__)

/**
 * Logs the contents of a buffer and an info-level message related to a characteristic
 * that may contain sensitive information.
 *
 * @param      logObject            Log object.
 * @param      characteristic       Characteristic.
 * @param      service              The service that contains the characteristic.
 * @param      accessory            The accessory that provides the service.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogSensitiveCharacteristicBufferInfo( \
        logObject, characteristic, service, accessory, bytes, numBytes, format, ...) \
    HAPLogSensitiveBufferInfo( \
            logObject, \
            bytes, \
            numBytes, \
            "[%016llX %s] [%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            (unsigned long long) ((const HAPBaseCharacteristic*) (characteristic))->iid, \
            ((const HAPBaseCharacteristic*) (characteristic))->debugDescription, \
            ##__VA_ARGS__)

/**
 * Logs the contents of a buffer and a debug-level message related to a characteristic
 * that may contain sensitive information.
 *
 * @param      logObject            Log object.
 * @param      characteristic       Characteristic.
 * @param      service              The service that contains the characteristic.
 * @param      accessory            The accessory that provides the service.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogSensitiveCharacteristicBufferDebug( \
        logObject, characteristic, service, accessory, bytes, numBytes, format, ...) \
    HAPLogSensitiveBufferDebug( \
            logObject, \
            bytes, \
            numBytes, \
            "[%016llX %s] [%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            (unsigned long long) ((const HAPBaseCharacteristic*) (characteristic))->iid, \
            ((const HAPBaseCharacteristic*) (characteristic))->debugDescription, \
            ##__VA_ARGS__)

/**
 * Logs the contents of a buffer and an error-level message related to a characteristic
 * that may contain sensitive information.
 *
 * @param      logObject            Log object.
 * @param      characteristic       Characteristic.
 * @param      service              The service that contains the characteristic.
 * @param      accessory            The accessory that provides the service.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogSensitiveCharacteristicBufferError( \
        logObject, characteristic, service, accessory, bytes, numBytes, format, ...) \
    HAPLogSensitiveBufferError( \
            logObject, \
            bytes, \
            numBytes, \
            "[%016llX %s] [%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            (unsigned long long) ((const HAPBaseCharacteristic*) (characteristic))->iid, \
            ((const HAPBaseCharacteristic*) (characteristic))->debugDescription, \
            ##__VA_ARGS__)

/**
 * Logs the contents of a buffer and a fault-level message related to a characteristic
 * that may contain sensitive information.
 *
 * @param      logObject            Log object.
 * @param      characteristic       Characteristic.
 * @param      service              The service that contains the characteristic.
 * @param      accessory            The accessory that provides the service.
 * @param      bytes                Buffer containing data to log.
 * @param      numBytes             Length of buffer.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogSensitiveCharacteristicBufferFault( \
        logObject, characteristic, service, accessory, bytes, numBytes, format, ...) \
    HAPLogSensitiveBufferFault( \
            logObject, \
            bytes, \
            numBytes, \
            "[%016llX %s] [%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            (unsigned long long) ((const HAPBaseCharacteristic*) (characteristic))->iid, \
            ((const HAPBaseCharacteristic*) (characteristic))->debugDescription, \
            ##__VA_ARGS__)

/**
 * Logs a message related to a characteristic at a specific logging level that may contain sensitive information.
 *
 * @param      logObject            Log object.
 * @param      characteristic       Characteristic.
 * @param      service              The service that contains the characteristic.
 * @param      accessory            The accessory that provides the service.
 * @param      type                 A log type constant, indicating the level of logging to perform.
 */
#define HAPLogSensitiveCharacteristicWithType(logObject, characteristic, service, accessory, type, ...) \
    do { \
        switch (type) { \
            case kHAPLogType_Debug: { \
                HAPLogSensitiveCharacteristicDebug(logObject, characteristic, service, accessory, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Info: { \
                HAPLogSensitiveCharacteristicInfo(logObject, characteristic, service, accessory, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Default: { \
                HAPLogSensitiveCharacteristic(logObject, characteristic, service, accessory, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Error: { \
                HAPLogSensitiveCharacteristicError(logObject, characteristic, service, accessory, __VA_ARGS__); \
                break; \
            } \
            case kHAPLogType_Fault: { \
                HAPLogSensitiveCharacteristicFault(logObject, characteristic, service, accessory, __VA_ARGS__); \
                break; \
            } \
        } \
    } while (0)

/**
 * Logs a default-level message related to a characteristic that may contain sensitive information.
 *
 * @param      logObject            Log object.
 * @param      characteristic       Characteristic.
 * @param      service              The service that contains the characteristic.
 * @param      accessory            The accessory that provides the service.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogSensitiveCharacteristic(logObject, characteristic, service, accessory, format, ...) \
    HAPLogSensitive( \
            logObject, \
            "[%016llX %s] [%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            (unsigned long long) ((const HAPBaseCharacteristic*) (characteristic))->iid, \
            ((const HAPBaseCharacteristic*) (characteristic))->debugDescription, \
            ##__VA_ARGS__)

/**
 * Logs an info-level message related to a characteristic that may contain sensitive information.
 *
 * @param      logObject            Log object.
 * @param      characteristic       Characteristic.
 * @param      service              The service that contains the characteristic.
 * @param      accessory            The accessory that provides the service.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogSensitiveCharacteristicInfo(logObject, characteristic, service, accessory, format, ...) \
    HAPLogSensitiveInfo( \
            logObject, \
            "[%016llX %s] [%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            (unsigned long long) ((const HAPBaseCharacteristic*) (characteristic))->iid, \
            ((const HAPBaseCharacteristic*) (characteristic))->debugDescription, \
            ##__VA_ARGS__)

/**
 * Logs a debug-level message related to a characteristic that may contain sensitive information.
 *
 * @param      logObject            Log object.
 * @param      characteristic       Characteristic.
 * @param      service              The service that contains the characteristic.
 * @param      accessory            The accessory that provides the service.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogSensitiveCharacteristicDebug(logObject, characteristic, service, accessory, format, ...) \
    HAPLogSensitiveDebug( \
            logObject, \
            "[%016llX %s] [%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            (unsigned long long) ((const HAPBaseCharacteristic*) (characteristic))->iid, \
            ((const HAPBaseCharacteristic*) (characteristic))->debugDescription, \
            ##__VA_ARGS__)

/**
 * Logs an error-level message related to a characteristic that may contain sensitive information.
 *
 * @param      logObject            Log object.
 * @param      characteristic       Characteristic.
 * @param      service              The service that contains the characteristic.
 * @param      accessory            The accessory that provides the service.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogSensitiveCharacteristicError(logObject, characteristic, service, accessory, format, ...) \
    HAPLogSensitiveError( \
            logObject, \
            "[%016llX %s] [%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            (unsigned long long) ((const HAPBaseCharacteristic*) (characteristic))->iid, \
            ((const HAPBaseCharacteristic*) (characteristic))->debugDescription, \
            ##__VA_ARGS__)

/**
 * Logs a fault-level message related to a characteristic that may contain sensitive information.
 *
 * @param      logObject            Log object.
 * @param      characteristic       Characteristic.
 * @param      service              The service that contains the characteristic.
 * @param      accessory            The accessory that provides the service.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogSensitiveCharacteristicFault(logObject, characteristic, service, accessory, format, ...) \
    HAPLogSensitive( \
            logObject, \
            "[%016llX %s] [%016llX %s] " format, \
            (unsigned long long) (accessory)->aid, \
            (accessory)->name, \
            (unsigned long long) ((const HAPBaseCharacteristic*) (characteristic))->iid, \
            ((const HAPBaseCharacteristic*) (characteristic))->debugDescription, \
            ##__VA_ARGS__)

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
