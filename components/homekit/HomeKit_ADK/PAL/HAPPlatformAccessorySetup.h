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

#ifndef HAP_PLATFORM_ACCESSORY_SETUP_H
#define HAP_PLATFORM_ACCESSORY_SETUP_H

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
 * Each accessory must be provisioned for use with HomeKit during manufacturing.
 * This platform module must be implemented to provide access to the provisioned information.
 *
 *
 * **Setup code:**
 *
 * During HomeKit pairing, a setup code is used by the controller to set up an encrypted link with the accessory.
 * - If the accessory has a display that supports showing a setup code, a random setup code is used while pairing.
 * - Otherwise, a static setup code has to be generated during manufacturing that is deployed to the accessory.
 *
 * If a static setup code is used, the following steps must be followed for each accessory:
 *
 * 1. A random setup code in format `XXX-XX-XXX` with `X` being a digit from `0-9` must be generated.
 *    - The setup code must be generated from a cryptographically secure random number generator.
 *    - Setup codes that only consist of a repeating digit are not allowed.
 *    - `123-45-678` and `876-54-321` are not allowed.
 *
 * 2. A random SRP salt (16 random bytes) must be generated.
 *    - The SRP salt must be generated from a cryptographically secure random number generator.
 *
 * 3. The corresponding SRP verifier is derived from the setup code and the SRP salt.
 *
 * 4. The SRP salt and SRP verifier are deployed to the accessory.
 *
 * 5. If the accessory has a programmable NFC tag, the setup code must also be deployed.
 *    Otherwise, the raw setup code must not be deployed.
 *
 *
 * **Setup ID:**
 *
 * To improve the setup experience, a random setup ID is generated and deployed to each accessory during manufacturing.
 * During pairing, the setup ID is used to identify the accessory to which a scanned label belongs.
 *
 * The following steps must be followed for each accessory:
 *
 * 1. A random setup ID in format `XXXX` with `X` being a digit from 0-9 or a character from A-Z must be generated.
 *    - The setup ID must be generated from a cryptographically secure random number generator.
 *    - Lowercase characters are not allowed.
 *
 * 2. The setup ID is deployed to the accessory.
 *
 * - If no setup ID is deployed, certain features like QR code displays or programmable NFC are unavailable.
 *
 *
 * **Labels:**
 *
 * If the accessory has a static setup code, a label must be affixed to the accessory and its packaging.
 * Labels are based on the setup payload that is derived from the setup ID and the setup code.
 *
 * There are different kinds of labels:
 * - Labels consisting of a QR code that encodes the setup payload, and containing the setup code.
 * - Labels with an embedded NFC tag that provides the setup payload when read, and containing the setup code.
 * - Legacy labels containing only the setup code.
 *
 * If an accessory has a programmable NFC tag, only QR code labels should be used.
 * If the accessory is a security class device and uses NFC pairing, it must have a programmable NFC tag.
 *
 * Expected behavior:
 *
 * The setup data may be stored in a file, directly in flash, or in EEPROM chips. The key-value store may be used for
 * this purpose.
 */

/**
 * Accessory setup manager.
 */
typedef struct HAPPlatformAccessorySetup HAPPlatformAccessorySetup;
typedef struct HAPPlatformAccessorySetup* HAPPlatformAccessorySetupRef;
HAP_NONNULL_SUPPORT(HAPPlatformAccessorySetup)

/**
 * Loads SRP salt and verifier of a static setup code.
 *
 * - If the accessory can display a dynamic setup code, implementation is not required.
 *
 * @param      accessorySetup       Accessory setup manager.
 * @param[out] setupInfo            Setup info.
 */
void HAPPlatformAccessorySetupLoadSetupInfo(HAPPlatformAccessorySetupRef accessorySetup, HAPSetupInfo* setupInfo);

/**
 * Loads statically provisioned setup code in plaintext.
 *
 * - Must only be implemented if the accessory is connected to a programmable NFC tag
 *   but can't display a dynamic setup code.
 *
 * - The static setup info must be loadable and must be compatible with the setup code.
 *
 * @param      accessorySetup       Accessory setup manager.
 * @param[out] setupCode            Setup code.
 */
void HAPPlatformAccessorySetupLoadSetupCode(HAPPlatformAccessorySetupRef accessorySetup, HAPSetupCode* setupCode);

/**
 * Loads statically provisioned setup ID.
 *
 * - Required to display QR codes and to use programmable NFC tags.
 *
 * @param      accessorySetup       Accessory setup manager.
 * @param[out] valid                True if a setup ID is available. False otherwise.
 * @param[out] setupID              Setup ID, if available.
 */
void HAPPlatformAccessorySetupLoadSetupID(
        HAPPlatformAccessorySetupRef accessorySetup,
        bool* valid,
        HAPSetupID* setupID);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
