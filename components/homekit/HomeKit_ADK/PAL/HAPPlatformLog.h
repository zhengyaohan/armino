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

#ifndef HAP_PLATFORM_LOG_H
#define HAP_PLATFORM_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPBase.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * @file
 *
 * \rst
 * The logging concept is based on Apple’s Unified Logging, see
 * `Apple Developer Logging Documentation <https://developer.apple.com/documentation/os/logging?language=objc>`_.
 * \endrst
 *
 * For producing log output, please use the front-end logging functions in HAPLog.h, their implementation internally
 * uses `HAPPlatformLog.h`.
 *
 * The HAP Library writes no security-critical information to the log (e.g., no key data in plain text).
 *
 * **Expected behavior:**
 *
 * Two functions must be provided by the PAL.
 *
 * - `HAPPlatformLogGetEnabledTypes` indicates whether a specific type of logging, such as default, *info*, *debug*,
 * *error*, or *fault*, is enabled for a specific subsystem / category. This allows the logging for different subsystems
 * / categories to be controlled individually. The default, error and fault level messages are always enabled. Log
 * levels are documented for type `HAPLogType` in the header file HAPLog.h. Here a summary:
 *
 *    - Debug-level messages contain information that may be useful during developing, or for troubleshooting a specific
 *    problem.
 *    - Info-level messages contain information that may be helpful, but isn’t essential, for troubleshooting errors.
 *    - Default-level messages contain information about things that may result in a failure.
 *    - Error-level messages are intended for reporting component-level errors.
 *    - Fault-level messages are intended for reporting system-level errors (more than one component)
 *
 * - `HAPPlatformLogCapture` should write a log message to the target platform. If no log output is desired, its
 * function body should be made empty. Small SoCs typically do not have sufficient memory, RAM nor flash, to store
 * log output. Thus log output on such chipsets is usually forwarded directly to a serial port, typically a UART port.
 * To observe an accessory’s behavior, a Mac or PC with a terminal program can be attached. On the smallest systems,
 * there may not be sufficient flash memory available, for them the _min variants of the HAP Library binaries are
 * provided that emit no log output at all and have considerably smaller code sizes.
 *
 * The default maximum length of a log message is *2KB*. On platforms that do not have sufficient memory the size can
 * be modified using the compile option `HAP_LOG_MESSAGE_MAX_BYTES`.
 */

/**
 * Enabled log types.
 */
HAP_ENUM_BEGIN(uint8_t, HAPPlatformLogEnabledTypes) {
    /**
     * No messages are captured.
     */
    kHAPPlatformLogEnabledTypes_None,

    /**
     * Only Error-level and fault-level messages are captured.
     *
     *   Error-level messages are intended for reporting component-level errors.
     *   Fault-level messages are intended for capturing system-level or multi-component errors only.
     */
    kHAPPlatformLogEnabledTypes_Error,

    /**
     * Default-level, error-level and fault-level messages are captured.
     *
     * - Default-level messages contain information about things that might result a failure.
     */
    kHAPPlatformLogEnabledTypes_Default,

    /**
     * Error-level, fault-level, default-level and info-level messages are captured.
     *
     * - Info-level messages contain information that may be helpful, but isn't essential, for troubleshooting errors.
     */
    kHAPPlatformLogEnabledTypes_Info,

    /**
     * Default-level, error-level, fault-level, info-level, and debug-level messages are captured.
     *
     * - Messages logged at debug level contain information that may be useful during development or while
     *   troubleshooting a specific problem.
     */
    kHAPPlatformLogEnabledTypes_Debug
} HAP_ENUM_END(uint8_t, HAPPlatformLogEnabledTypes);

/**
 * Indicates whether a specific type of logging, such as default, info, debug, error, or fault, is enabled
 * for a specific log object. Different log objects may have different configurations.
 *
 * - Log levels are described in the documentation of HAPLogType in file HAPLog.h.
 *
 * @param      log                  Log object.
 *
 * @return Logging levels that shall be enabled for the given subsystem / category.
 */
HAP_RESULT_USE_CHECK
HAPPlatformLogEnabledTypes HAPPlatformLogGetEnabledTypes(const HAPLogObject* log);

/**
 * Logs a message.
 *
 * @param      log                  Log object.
 * @param      type                 Logging level.
 * @param      message              A log message. NULL-terminated.
 * @param      bufferBytes          Optional buffer containing related data to log.
 * @param      numBufferBytes       Length of buffer.
 */
void HAPPlatformLogCapture(
        const HAPLogObject* log,
        HAPLogType type,
        const char* message,
        const void* _Nullable bufferBytes,
        size_t numBufferBytes) HAP_DIAGNOSE_ERROR(!bufferBytes && numBufferBytes, "empty buffer cannot have a length");

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
