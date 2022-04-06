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

#ifndef HAP_BLE_CHARACTERISTIC_BROADCAST_H
#define HAP_BLE_CHARACTERISTIC_BROADCAST_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP+API.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)

/**
 * Broadcast interval.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Table 7-30 Broadcast Interval
 */
HAP_ENUM_BEGIN(uint8_t, HAPBLECharacteristicBroadcastInterval) { /** 20 ms (Default). */
                                                                 kHAPBLECharacteristicBroadcastInterval_20Ms = 0x01,

                                                                 /** 1280 ms. */
                                                                 kHAPBLECharacteristicBroadcastInterval_1280Ms = 0x02,

                                                                 /** 2560 ms. */
                                                                 kHAPBLECharacteristicBroadcastInterval_2560Ms = 0x03
} HAP_ENUM_END(uint8_t, HAPBLECharacteristicBroadcastInterval);

/**
 * Checks whether a value represents a valid broadcast interval.
 *
 * @param      value                Value to check.
 *
 * @return true                     If the value is valid.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPBLECharacteristicIsValidBroadcastInterval(uint8_t value);

/**
 * Gets the broadcast configuration of a characteristic.
 *
 * @param      characteristic       Characteristic. Characteristic must support broadcasts.
 * @param      service              The service that contains the characteristic.
 * @param      accessory            The accessory that provides the service.
 * @param[out] broadcastsEnabled    Whether broadcast notifications are enabled.
 * @param[out] broadcastInterval    Broadcast interval, if broadcast notifications are enabled.
 * @param      keyValueStore        Key-value store.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If an I/O error occurred.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 7.3.5.8 HAP Characteristic Configuration Procedure
 */
HAP_RESULT_USE_CHECK
HAPError HAPBLECharacteristicGetBroadcastConfiguration(
        const HAPCharacteristic* characteristic,
        const HAPService* service,
        const HAPAccessory* accessory,
        bool* broadcastsEnabled,
        HAPBLECharacteristicBroadcastInterval* broadcastInterval,
        HAPPlatformKeyValueStoreRef keyValueStore);

/**
 * Enables broadcasts for a characteristic.
 *
 * @param      characteristic       Characteristic. Characteristic must support broadcasts.
 * @param      service              The service that contains the characteristic.
 * @param      accessory            The accessory that provides the service.
 * @param      broadcastInterval    Broadcast interval.
 * @param      keyValueStore        Key-value store.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If an I/O error occurred.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 7.3.5.8 HAP Characteristic Configuration Procedure
 */
HAP_RESULT_USE_CHECK
HAPError HAPBLECharacteristicEnableBroadcastNotifications(
        const HAPCharacteristic* characteristic,
        const HAPService* service,
        const HAPAccessory* accessory,
        HAPBLECharacteristicBroadcastInterval broadcastInterval,
        HAPPlatformKeyValueStoreRef keyValueStore);

/**
 * Disables broadcasts for a characteristic.
 *
 * @param      characteristic       Characteristic. Characteristic must support broadcasts.
 * @param      service              The service that contains the characteristic.
 * @param      accessory            The accessory that provides the service.
 * @param      keyValueStore        Key-value store.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If an I/O error occurred.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 7.3.5.8 HAP Characteristic Configuration Procedure
 */
HAP_RESULT_USE_CHECK
HAPError HAPBLECharacteristicDisableBroadcastNotifications(
        const HAPCharacteristic* characteristic,
        const HAPService* service,
        const HAPAccessory* accessory,
        HAPPlatformKeyValueStoreRef keyValueStore);

#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
