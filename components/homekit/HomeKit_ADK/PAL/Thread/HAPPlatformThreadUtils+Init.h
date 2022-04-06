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
// Copyright (C) 2020 Apple Inc. All Rights Reserved.

#ifndef HAP_PLATFORM_THREAD_UTILS_INIT_H_
#define HAP_PLATFORM_THREAD_UTILS_INIT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPBase.h"
#include "HAPPlatformFeatures.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

//*****************************************************************************
// HAP Thread Utils - Init module.
//    This module provides functions that allow clients to initialize the Thread
//    accessory
//
//    There are two main components to Thread Initialization:
//    * Initialization of the Instance
//    * Initialization of Thread Link itself
//
// The instance must be initialized by the accessory upon boot, Whether the
// Thread transport will be immediately used or not.
//
// The Thread Link must be initialized before the Thread Transport will be used.
//*****************************************************************************

/**
 * Thread device type capabilities enumeration
 */
HAP_ENUM_BEGIN(uint8_t, HAPPlatformThreadDeviceCapabilities) {
    /** Border Router */
    kHAPPlatformThreadDeviceCapabilities_BR = 0x30,
    /** Router */
    kHAPPlatformThreadDeviceCapabilities_Router = 0x20,
    /** Router Eligible End Device */
    kHAPPlatformThreadDeviceCapabilities_REED = 0x8,
    /** Full End Device */
    kHAPPlatformThreadDeviceCapabilities_FED = 0x4,
    /** Minimal End Device */
    kHAPPlatformThreadDeviceCapabilities_MED = 0x1,
    /** Sleepy End Device */
    kHAPPlatformThreadDeviceCapabilities_SED = 0x2,
    /** Full Thread Device mask */
    kHAPPlatformThreadDeviceCapabilities_FTD =
            (kHAPPlatformThreadDeviceCapabilities_Router | kHAPPlatformThreadDeviceCapabilities_REED |
             kHAPPlatformThreadDeviceCapabilities_FED),
    /** Minimal Thread Device mask */
    kHAPPlatformThreadDeviceCapabilities_MTD =
            (kHAPPlatformThreadDeviceCapabilities_MED | kHAPPlatformThreadDeviceCapabilities_SED)
} HAP_ENUM_END(uint8_t, HAPPlatformThreadDeviceCapabilities);

//*****************************************************************************
// Thread Instance Functions
//*****************************************************************************
/**
 * Initializes the thread instance.  Must be called at boot for
 * any accessory that will ever support use of the Thread
 * Transport.
 */
void HAPPlatformThreadInitInstance(void);

/**
 * Returns whether or not thread instance was initialized
 *
 * @return true if thread instance was initialized
 */
HAP_RESULT_USE_CHECK
bool HAPPlatformThreadInstanceIsInitialized(void);

/**
 * Returns thread handle.  Handle is context specific, but must
 * at minimum contain the Thread Instance.
 *
 * @return void* context specific handle.
 */
void* HAPPlatformThreadGetHandle(void);

//*****************************************************************************
// Thread Link Functions
//*****************************************************************************

/**
 * Initializes the Thread Link.  Must be called before any other
 * Thread operations.  The Accessory Server's thread device parameters must be set
 * before calling
 *
 * @param server        accessory server:  Must have device Params set
 *                      HAPAccessoryServer* type is obscured due to header file dependencies.
 * @param deviceType    Indicates what kind of device this accessory will be
 * @param pollPeriod    Indicates how long a sleepy device will sleep between polls
 * @param childTimeout  Indicates how long this child can go before checking in with its parent
 *                      before being considered "missing"
 * @param txPower       thread tx power in dBm.
 */
void HAPPlatformThreadInitialize(
        void* server,
        HAPPlatformThreadDeviceCapabilities deviceType,
        uint32_t pollPeriod,
        uint32_t childTimeout,
        uint8_t txPower);

/**
 * Tears down the thread stack and deinitializes the thread
 * instance
 *
 * @param server        Accessory Server.
 *                      HAPAccessoryServer* type is obscured due
 *                      to header file dependencies
 */
void HAPPlatformThreadDeinitialize(void* server);

/**
 * Indicates whether thread has been initialized or not
 *
 * @return bool true if initialized, false otherwise
 */
bool HAPPlatformThreadIsInitialized(void);

//*****************************************************************************
// Thread Parameters functions
//*****************************************************************************

/**
 * Clears Thread network credentials when the Thread is in a safe state to clear the parameters.
 *
 * That is, it will be erased when transport layer isn't running while it could still be erased.
 * Some platforms such as ones relying on OpenThread might not be able to clear the parameters
 * unless the Thread stack is up and hence this call might not be effective till the Thread
 * stack is up again.
 *
 * @param server         accessory server
 *                       HAPAccessoryServer* type is obscured due to header file dependencies.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformThreadClearParameters(void* server);

/**
 * Returns the poll period set by the accessory
 *
 * @return uint32_t poll period in milliseconds
 */
uint32_t HAPPlatformThreadGetConfiguredPollPeriod(void);

/**
 * Initiates a HAP Thread Reset.
 *    Note:  This requests a Thread Clear Parameters.  The
 *    Thread network will not be fully cleared until shut down.
 *
 */
void HAPPlatformThreadInitiateFactoryReset(void);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
