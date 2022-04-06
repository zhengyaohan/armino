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

#ifndef HAP_PLATFORM_THREAD_UTILS_COMMISSIONING_H
#define HAP_PLATFORM_THREAD_UTILS_COMMISSIONING_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPBase.h"
#include "HAPPlatformFeatures.h"
#include "HAPPlatformRunLoop.h"

//*****************************************************************************
// HAP Thread Utils - Commissioning module.
//    This module provides operations that allow clients to commission the
//    accessory to a Thread Network.  Commissioning can be performed by:
//    * Providing commissioning parameters directly
//    * Using Thread's Joiner system
//    * Statically - For Test / debug purposes only
//*****************************************************************************
#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/** Ext PAN ID length in number of bytes */
#define kHAPPlatformThreadExtPanIdNumBytes 8

/** Master key length in number of bytes */
#define kHAPPlatformThreadThreadExtPanIdNumBytes 16

/** Maximum Network name length including NULL character in number of bytes */
#define kHAPPlatformThreadNetworkNameNumBytes 17

/**
 * Network parameters
 */
typedef struct {
    /** channel */
    uint16_t channel;
    /** PAN ID */
    uint16_t panId;
    /** Ext PAN ID */
    uint8_t extPanId[kHAPPlatformThreadExtPanIdNumBytes];
    /** Master Key */
    uint8_t masterKey[kHAPPlatformThreadThreadExtPanIdNumBytes];
    /** network name */
    char networkName[kHAPPlatformThreadNetworkNameNumBytes];
} HAPPlatformThreadNetworkParameters;

/**
 * Checks whether Thread node was commissioned
 */
bool HAPPlatformThreadIsCommissioned(void);

/**
 * Commissions Thread network parameters
 *
 * @param parameters    network parameters
 */
void HAPPlatformThreadSetNetworkParameters(const HAPPlatformThreadNetworkParameters* parameters);

/**
 * Enables Thread stack which kicks off joining Thread network with commissioned parameters.
 */
void HAPPlatformThreadJoinCommissionedNetwork(void);

/**
 * Enables Thread stack which kicks off joining Thread network with statically commissioned parameters when available.
 *
 * If statically commissioned parameters are not available, do not enable Thread stack and instead return false.
 *
 * @return true if statically commissioned parameters are available and hence Thread stack has been enabled.
 *         false, otherwise.
 */
bool HAPPlatformThreadJoinStaticCommissioninedNetwork(void);

/**
 * Reads currently active network parameters.
 *
 * @param parameters     buffer to store parameters. Note that master key is not returned.
 * @return
 *                       kHAPError_None if no errors.
 *                       kHAPError_InvalidState if no parameters
 *                       have been set
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformThreadGetNetworkParameters(HAPPlatformThreadNetworkParameters* parameters);

/**
 * Starts Thread joiner (in band commissioning)
 *
 * @param passphrase    passphrase for joiner or NULL if setup code isn't available
 *                      and the passphrase must be obtained from platform specific way.
 * @param callback      runloop callback to call upon completion.
 * @param server        accessory server<br>
 *                      HAPAccessoryServer* type is obscured due to header file dependencies.
 *
 * @return kHAPError_None when successful. kHAPError_Unknown if no passphrase is available.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformThreadStartJoiner(const char* passphrase, HAPPlatformRunLoopCallback callback, void* server);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
