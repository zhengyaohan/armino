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

#ifndef HAP_PLATFORM_MFI_HW_AUTH_H
#define HAP_PLATFORM_MFI_HW_AUTH_H

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
 * For hardware-based authentication, byte blocks from a HomeKit controller are forwarded to an accessory's Apple
 * Authentication Coprocessor via its I2C interface, and vice versa. The platform developer does not need to understand
 * the format or meaning of the data being forwarded.
 *
 * **Expected behavior:**
 *
 * Apart from a few functions for enabling/disabling the Apple Authentication Coprocessor, two functions must be
 * implemented: one for writing and one for reading a byte block over I2C. Apple Authentication Coprocessors *2.0C* and
 * *3.0* are supported by the HAP Library.
 */

/**
 * Apple Authentication Coprocessor provider.
 */
typedef struct HAPPlatformMFiHWAuth HAPPlatformMFiHWAuth;
typedef struct HAPPlatformMFiHWAuth* HAPPlatformMFiHWAuthRef;
HAP_NONNULL_SUPPORT(HAPPlatformMFiHWAuth)

/**
 * Returns whether the Apple Authentication Coprocessor is powered on.
 *
 * @param      mfiHWAuth            Apple Authentication Coprocessor provider.
 *
 * @return true                     If the Apple Authentication Coprocessor is enabled.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPPlatformMFiHWAuthIsPoweredOn(HAPPlatformMFiHWAuthRef mfiHWAuth);

/**
 * Powers on and prepares the Apple Authentication Coprocessor for usage.
 *
 * @param      mfiHWAuth            Apple Authentication Coprocessor provider.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If communication with the Apple Authentication Coprocessor failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformMFiHWAuthPowerOn(HAPPlatformMFiHWAuthRef mfiHWAuth);

/**
 * Powers off the Apple Authentication Coprocessor.
 *
 * @param      mfiHWAuth            Apple Authentication Coprocessor provider.
 */
void HAPPlatformMFiHWAuthPowerOff(HAPPlatformMFiHWAuthRef mfiHWAuth);

/**
 * Writes data to the Apple Authentication Coprocessor.
 *
 * @param      mfiHWAuth            Apple Authentication Coprocessor provider.
 * @param      bytes                Buffer to write.
 * @param      numBytes             Length of buffer. Minimum 1. Maximum 128.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If communication with the Apple Authentication Coprocessor failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformMFiHWAuthWrite(HAPPlatformMFiHWAuthRef mfiHWAuth, const void* bytes, size_t numBytes);

/**
 * Reads a register from the Apple Authentication Coprocessor.
 *
 * @param      mfiHWAuth            Apple Authentication Coprocessor provider.
 * @param      registerAddress      Address of the Apple Authentication Coprocessor register to read.
 * @param      bytes                Result buffer.
 * @param      numBytes             Length of buffer. Minimum 1. Maximum 128.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If communication with the Apple Authentication Coprocessor failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformMFiHWAuthRead(
        HAPPlatformMFiHWAuthRef mfiHWAuth,
        uint8_t registerAddress,
        void* bytes,
        size_t numBytes);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
