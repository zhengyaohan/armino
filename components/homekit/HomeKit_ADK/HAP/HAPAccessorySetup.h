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

#ifndef HAP_ACCESSORY_SETUP_H
#define HAP_ACCESSORY_SETUP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP+API.h"
#include "HAPAccessory.h"
#include "HAPDeviceID.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * Checks whether a string represents a valid setup code.
 *
 * @param      stringValue          Value to check. NULL-terminated.
 *
 * @return true                     If the string is a valid setup code.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPAccessorySetupIsValidSetupCode(const char* stringValue);

/**
 * Generates a random setup code.
 *
 * @param[out] setupCode            Setup code.
 */
void HAPAccessorySetupGenerateRandomSetupCode(HAPSetupCode* setupCode);

/**
 * Checks whether a string represents a valid setup ID.
 *
 * @param      stringValue          Value to check. NULL-terminated.
 *
 * @return true                     If the string is a valid setup ID.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPAccessorySetupIsValidSetupID(const char* stringValue);

/**
 * Generates a random setup ID.
 *
 * @param[out] setupID              Setup ID.
 */
void HAPAccessorySetupGenerateRandomSetupID(HAPSetupID* setupID);

/**
 * Setup payload flags.
 */
typedef struct {
    /**
     * Accessory is paired with a controller.
     * (only for accessories using programmable NFC tags to advertise the setup payload).
     *
     * If paired, no setup code or setup ID must be encoded.
     */
    bool isPaired : 1;

    /** Accessory supports HAP over IP transport; MUST be ON if WAC is ON. */
    bool ipSupported : 1;

    /** Accessory supports HAP over BLE transport. */
    bool bleSupported : 1;

    /** Accessory supports Wi-Fi Accessory Configuration (WAC) for configuring Wi-Fi credentials. */
    bool wacSupported : 1;

    /** Accessory supports HAP over Thread transport */
    bool threadSupported : 1;

} HAPAccessorySetupSetupPayloadFlags;

/**
 * Generates the joiner passphrase for a given setup code.
 *
 * @param[out] passphrase joining passphrase
 * @param setupCode Setup code
 */
void HAPAccessorySetupGenerateJoinerPassphrase(
        HAPJoinerPassphrase* passphrase,
        const HAPSetupCode* _Nullable setupCode);

/**
 * Generates the setup payload for a given setup code and setup ID.
 *
 * @param[out] setupPayload         Setup payload.
 * @param      setupCode            Setup code.
 * @param      setupID              Setup ID.
 * @param      eui                  IEEE Factory assigned EUI
 * @param      product Data         Setup Product Data
 * @param      flags                Setup payload flags.
 * @param      category             Accessory category.
 */
void HAPAccessorySetupGetSetupPayload(
        HAPSetupPayload* setupPayload,
        const HAPSetupCode* _Nullable setupCode,
        const HAPSetupID* _Nullable setupID,
        const HAPEui64* _Nullable eui,
        const HAPAccessoryProductData* productData,
        HAPAccessorySetupSetupPayloadFlags flags,
        HAPAccessoryCategory category);

/**
 * Setup hash.
 */
typedef struct {
    uint8_t bytes[4]; /**< Value. */
} HAPAccessorySetupSetupHash;
HAP_STATIC_ASSERT(sizeof(HAPAccessorySetupSetupHash) == 4, HAPAccessorySetupSetupHash);

/**
 * Derives the setup hash for a given setup ID and Device ID.
 *
 * @param[out] setupHash            Setup hash.
 * @param      setupID              Setup ID.
 * @param      deviceIDString       Device ID.
 */
void HAPAccessorySetupGetSetupHash(
        HAPAccessorySetupSetupHash* setupHash,
        const HAPSetupID* setupID,
        const HAPDeviceIDString* deviceIDString);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
