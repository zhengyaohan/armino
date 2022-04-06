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

#ifndef HAP_BLE_ACCESSORY_SERVER_ADVERTISING_H
#define HAP_BLE_ACCESSORY_SERVER_ADVERTISING_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP+API.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)

/** Minimum possible BLE Advertising rate - 20ms  */
#define HAP_BLE_MINIMUM_ADVERTISING_RATE 20

/**
 * BLE: GSN state.
 */
typedef struct {
    uint16_t gsn;                             /**< Global State Number. */
    bool didIncrementInDisconnectedState : 1; /**< Whether GSN has been incremented in the disconnect cycle. */
} HAPBLEAccessoryServerGSN;

/**
 * BLE: Fetches GSN state.
 *
 * @param      keyValueStore        Key-value store
 * @param[out] gsn                  GSN.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If an I/O error occurred.
 */
HAP_RESULT_USE_CHECK
HAPError HAPBLEAccessoryServerGetGSN(HAPPlatformKeyValueStoreRef keyValueStore, HAPBLEAccessoryServerGSN* gsn);

/**
 * BLE: Get advertisement parameters.
 *
 * @param      server               Accessory server.
 * @param[out] shouldAdvertisementBeActive True if advertisement should be active, False otherwise.
 * @param[out] advertisingInterval  Advertising interval in milliseconds, if active.
 * @param[out] advertisingBytes     Advertising data, if active.
 * @param      maxAdvertisingBytes  Capacity of advertising data buffer. Must be at least 31.
 * @param[out] numAdvertisingBytes  Effective length of advertising data buffer, if active.
 * @param[out] scanResponseBytes    Scan response data, if active.
 * @param      maxScanResponseBytes Capacity of scan response data buffer. Should be >= 2.
 * @param      numScanResponseBytes Effective length of scan response data buffer, or 0, if not applicable, if active.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If an I/O error occurred.
 */
HAP_RESULT_USE_CHECK
HAPError HAPBLEAccessoryServerGetAdvertisingParameters(
        HAPAccessoryServer* server,
        bool* shouldAdvertisementBeActive,
        uint16_t* advertisingInterval,
        void* advertisingBytes,
        size_t maxAdvertisingBytes,
        size_t* numAdvertisingBytes,
        void* scanResponseBytes,
        size_t maxScanResponseBytes,
        size_t* numScanResponseBytes)
        HAP_DIAGNOSE_ERROR(maxAdvertisingBytes < 31, "maxAdvertisingBytes must be at least 31")
                HAP_DIAGNOSE_WARNING(maxScanResponseBytes < 2, "maxScanResponseBytes should be at least 2");

/**
 * BLE: Informs the accessory server that advertising has started.
 *
 * @param      server               Accessory server.
 */
void HAPBLEAccessoryServerDidStartAdvertising(HAPAccessoryServer* server);

/**
 * BLE: Informs the accessory server that a controller has connected.
 *
 * @param      server               Accessory server.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If a controller is already connected. Only 1 concurrent connection is allowed.
 * @return kHAPError_Unknown        If an I/O error occurred.
 */
HAP_RESULT_USE_CHECK
HAPError HAPBLEAccessoryServerDidConnect(HAPAccessoryServer* server);

/**
 * BLE: Informs the accessory server that a controller has disconnected.
 *
 * @param      server               Accessory server.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If no controller is connected.
 * @return kHAPError_Unknown        If an I/O error occurred.
 */
HAP_RESULT_USE_CHECK
HAPError HAPBLEAccessoryServerDidDisconnect(HAPAccessoryServer* server);

/**
 * BLE: Informs the accessory server that the value of a characteristic did change.
 *
 * @param      server               Accessory server.
 * @param      characteristic       The characteristic whose value has changed.
 * @param      service              The service that contains the characteristic.
 * @param      accessory            The accessory that provides the service.
 * @param      session              The session on which to raise the event.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If an I/O error occurred.
 */
HAP_RESULT_USE_CHECK
HAPError HAPBLEAccessoryServerDidRaiseEvent(
        HAPAccessoryServer* server,
        const HAPCharacteristic* characteristic,
        const HAPService* service,
        const HAPAccessory* accessory,
        HAPSession* _Nullable session);

/**
 * BLE: Informs the accessory server that the value of a characteristic which is registered for Bluetooth LE
 * indications changed.
 *
 * @param      server               Accessory server.
 * @param      characteristic       The characteristic whose value has changed.
 * @param      service              The service that contains the characteristic.
 * @param      accessory            The accessory that provides the service.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If an I/O error occurred.
 */
HAP_RESULT_USE_CHECK
HAPError HAPBLEAccessoryServerDidSendEventNotification(
        HAPAccessoryServer* server,
        const HAPCharacteristic* characteristic,
        const HAPService* service,
        const HAPAccessory* accessory);

/**
 * Remove a matching queued broadcast event if any
 *
 * @param server         accessory server
 * @param characteristic characteristic to remove
 * @param service        service of the characteristic to remove
 * @param accessory      accessory of the characteristic to remove
 */
void HAPBLEAccessoryServerRemoveQueuedBroadcastEvent(
        HAPAccessoryServer* server,
        const HAPCharacteristic* characteristic,
        const HAPService* service,
        const HAPAccessory* accessory);

#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
