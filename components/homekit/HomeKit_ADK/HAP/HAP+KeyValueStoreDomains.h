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

#ifndef HAP_KEY_VALUE_STORE_DOMAINS_H
#define HAP_KEY_VALUE_STORE_DOMAINS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPPlatform.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * Statically provisioned data.
 *
 * Purged: Never.
 */
#define kHAPKeyValueStoreDomain_Provisioning ((HAPPlatformKeyValueStoreDomain) 0x80)

/**
 * Accessory configuration.
 *
 * Purged: On factory reset.
 */
#define kHAPKeyValueStoreDomain_Configuration ((HAPPlatformKeyValueStoreDomain) 0x90)

/**
 * HomeKit characteristic configuration.
 *
 * Purged: On factory reset.
 */
#define kHAPKeyValueStoreDomain_CharacteristicConfiguration ((HAPPlatformKeyValueStoreDomain) 0x92)

/**
 * HomeKit pairing data.
 *
 * Purged: On factory reset and on pairing reset.
 */
#define kHAPKeyValueStoreDomain_Pairings ((HAPPlatformKeyValueStoreDomain) 0xA0)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Device ID.
 *
 * Format: uint8_t[kHAPDeviceID_NumBytes].
 */
#define kHAPKeyValueStoreKey_Configuration_DeviceID ((HAPPlatformKeyValueStoreKey) 0x00)

/**
 * Firmware version.
 *
 * Format: <major : uint32_t> <minor : uint32_t> <revision : uint32_t>, little endian.
 */
#define kHAPKeyValueStoreKey_Configuration_FirmwareVersion ((HAPPlatformKeyValueStoreKey) 0x10)

/**
 * Compatibility version.
 *
 * Format: Undefined.
 */
#define kHAPKeyValueStoreKey_Configuration_CompatibilityVersion ((HAPPlatformKeyValueStoreKey) 0x11)

/**
 * Configuration number.
 *
 * Format: uint32_t, little endian.
 */
#define kHAPKeyValueStoreKey_Configuration_ConfigurationNumber ((HAPPlatformKeyValueStoreKey) 0x20)

/**
 * Ed25519 long-term secret key.
 *
 * Format: uint8_t[ED25519_SECRET_KEY_BYTES].
 */
#define kHAPKeyValueStoreKey_Configuration_LTSK ((HAPPlatformKeyValueStoreKey) 0x21)

/**
 * Unsuccessful authentication attempts counter.
 *
 * Format: uint8_t.
 */
#define kHAPKeyValueStoreKey_Configuration_NumUnsuccessfulAuthAttempts ((HAPPlatformKeyValueStoreKey) 0x22)

/**
 * IP Global State Number.
 *
 * Format: <gsn : uint16_t>, little endian.
 */
#define kHAPKeyValueStoreKey_Configuration_IPGSN ((HAPPlatformKeyValueStoreKey) 0x30)

/**
 * BLE Global State Number.
 *
 * Format: <gsn : uint16_t> <didIncrement (0x01) : uint8_t>, little endian.
 */
#define kHAPKeyValueStoreKey_Configuration_BLEGSN ((HAPPlatformKeyValueStoreKey) 0x40)

/**
 * BLE Broadcast Encryption Key and Advertising Identifier.
 *
 * Format: little endian
 *     <keyExpirationGSN : uint16_t>
 *     <key : uint8_t[32]>
 *     <hasAdvertisingID (0x01) : uint8_t>
 *     <advertisingID : uint8_t[kHAPDeviceID_NumBytes]>
 */
#define kHAPKeyValueStoreKey_Configuration_BLEBroadcastParameters ((HAPPlatformKeyValueStoreKey) 0x41)

#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_RECONFIGURATION)
/**
 * WiFi Reconfiguration Cookie
 *
 * Format: little endian
 *     <wifiReconfigurationCookie : uint16_t>
 */
#define kHAPKeyValueStoreKey_WiFiReconfigurationCookie ((HAPPlatformKeyValueStoreKey) 0x42)
#endif

/**
 * SRP key
 *
 * Format: <key> : uint8_t[256]
 */
#define kHAPKeyValueStoreKey_Configuration_SrpKey ((HAPPlatformKeyValueStoreKey) 0x50)

/**
 * SRP server
 *
 * Format: <rrtype : uint16_t>
 *         <address : HAPIPAddress >
 *         <port : HAPNetworkPort >
 */
#define kHAPKeyValueStoreKey_Configuration_SrpServer ((HAPPlatformKeyValueStoreKey) 0x51)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// kHAPKeyValueStoreDomain_CharacteristicConfiguration.
// Format: 2 bytes aid, n times <2 bytes cid + 1 byte broadcast interval>, little endian.
// Current implementation restriction: 42 cid's supported (2 + 42 * 3 = 128 bytes).
// Future format:
// - Add one more triple, cid == 0000 + 1 byte KVS key of continuation.
// - Replace aid with 0000 in continuations.
// Restricted to 16-bit aid / cid as Bluetooth LE does not support larger IDs.
// Could be worked around by re-using aid as version (currently aid is always 1).

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// kHAPKeyValueStoreDomain_Pairings.
// Format: little endian.
//     <identifier : uint8_t[36]>
//     <numIdentifierBytes : uint8_t>
//     <publicKey : uint8_t[ED25519_PUBLIC_KEY_BYTES]>
//     <permissions : uint8_t>

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
