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

#ifndef HAP_LEGACY_IMPORT_H
#define HAP_LEGACY_IMPORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * Device ID of an accessory server.
 */
typedef struct HAP_ATTRIBUTE_GCC(packed) {
    /**
     * Device ID.
     */
    uint8_t bytes[6];
} HAPAccessoryServerDeviceID;

/**
 * Imports a device ID into an un-provisioned key-value store.
 * This is useful to import legacy settings from a different HomeKit SDK.
 *
 * - This function must no longer be called after the initial HAPAccessoryServerCreate call.
 *
 * @param      keyValueStore        Un-provisioned key-value store.
 * @param      deviceID             DeviceID of the accessory server.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If an error occurred while accessing the key-value store.
 */
HAP_ATTRIBUTE_GCC(warn_unused_result)
HAPError HAPLegacyImportDeviceID(HAPPlatformKeyValueStore* keyValueStore, const HAPAccessoryServerDeviceID* deviceID);

/**
 * Imports a configuration number into an un-provisioned key-value store.
 * This is useful to import legacy settings from a different HomeKit SDK.
 *
 * - This function must no longer be called after the initial HAPAccessoryServerCreate call.
 *
 * @param      keyValueStore        Un-provisioned key-value store.
 * @param      configurationNumber  Configuration number.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If an error occurred while accessing the key-value store.
 */
HAP_ATTRIBUTE_GCC(warn_unused_result)
HAPError HAPLegacyImportConfigurationNumber(HAPPlatformKeyValueStore* keyValueStore, uint32_t configurationNumber);

/**
 * Ed25519 long-term secret key of an accessory server.
 */
typedef struct HAP_ATTRIBUTE_GCC(packed) {
    /**
     * Ed25519 long-term secret key.
     */
    uint8_t bytes[32];
} HAPAccessoryServerLongTermSecretKey;

/**
 * Imports a Ed25519 long-term secret key into an un-provisioned key-value store.
 * This is useful to import legacy settings from a different HomeKit SDK.
 *
 * - This function must no longer be called after the initial HAPAccessoryServerCreate call.
 *
 * @param      keyValueStore        Un-provisioned key-value store.
 * @param      longTermSecretKey    Long-term secret key of the accessory server.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If an error occurred while accessing the key-value store.
 */
HAP_ATTRIBUTE_GCC(warn_unused_result)
HAPError HAPLegacyImportLongTermSecretKey(
        HAPPlatformKeyValueStore* keyValueStore,
        const HAPAccessoryServerLongTermSecretKey* longTermSecretKey);

/**
 * Imports an unsuccessful authentication attempts counter into an un-provisioned key-value store.
 * This is useful to import legacy settings from a different HomeKit SDK.
 *
 * - This function must no longer be called after the initial HAPAccessoryServerCreate call.
 *
 * @param      keyValueStore        Un-provisioned key-value store.
 * @param      numAuthAttempts      Unsuccessful authentication attempts counter.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If an error occurred while accessing the key-value store.
 */
HAP_ATTRIBUTE_GCC(warn_unused_result)
HAPError HAPLegacyImportUnsuccessfulAuthenticationAttemptsCounter(
        HAPPlatformKeyValueStore* keyValueStore,
        uint8_t numAuthAttempts);

/**
 * Pairing identifier of a paired controller.
 */
typedef struct HAP_ATTRIBUTE_GCC(packed) {
    /**
     * Buffer containing pairing identifier.
     */
    uint8_t bytes[36];

    /**
     * Number of used bytes in buffer.
     */
    size_t numBytes;
} HAPControllerPairingIdentifier;

/**
 * Public key of a paired controller.
 */
typedef struct HAP_ATTRIBUTE_GCC(packed) {
    /**
     * Public key.
     */
    uint8_t bytes[32];
} HAPControllerPublicKey;

/**
 * Imports a controller pairing into an un-provisioned key-value store.
 * This is useful to import legacy settings from a different HomeKit SDK.
 *
 * - This function must no longer be called after the initial HAPAccessoryServerCreate call.
 *
 * @param      keyValueStore        Un-provisioned key-value store.
 * @param      pairingIndex         Key-value store pairing index. 0 ..< Max number of pairings that will be supported.
 * @param      pairingIdentifier    HomeKit pairing identifier.
 * @param      publicKey            Ed25519 long-term public key of the paired controller.
 * @param      isAdmin              Whether or not the added controller has admin permissions.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If an error occurred while accessing the key-value store.
 */
HAP_ATTRIBUTE_GCC(warn_unused_result)
HAPError HAPLegacyImportControllerPairing(
        HAPPlatformKeyValueStore* keyValueStore,
        HAPPlatformKeyValueStoreKey pairingIndex,
        const HAPControllerPairingIdentifier* pairingIdentifier,
        const HAPControllerPublicKey* publicKey,
        bool isAdmin);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
