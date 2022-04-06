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

#ifndef HAP_BLE_PERIPHERAL_MANAGER_H
#define HAP_BLE_PERIPHERAL_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP+API.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)

/**
 * Fallback procedure status.
 *
 * - Fallback procedures can only return very simple information and can't access characteristics.
 *   /!\ If this is ever extended, proper checking for transient Pair Setup procedures is necessary!
 */
HAP_ENUM_BEGIN(uint8_t, HAPBLEFallbackProcedureStatus) {
    /** Max-Procedures. */
    kHAPBLEFallbackProcedureStatus_MaxProcedures = 1,

    /** Invalid instance ID. */
    kHAPBLEFallbackProcedureStatus_InvalidInstanceID,

    /** Operation is service signature read, and instance ID was 0. */
    kHAPBLEFallbackProcedureStatus_ZeroInstanceIDServiceSignatureRead
} HAP_ENUM_END(uint8_t, HAPBLEFallbackProcedureStatus);

/**
 * Fallback procedure state.
 *
 * - This keeps track of procedures beyond the maximum procedure limit.
 */
typedef struct {
    /**
     * Timer after which the procedure expires.
     *
     * - If this is 0, the procedure is not active.
     */
    HAPPlatformTimerRef timer;

    /** Remaining body bytes in the request before a response may be sent. */
    uint16_t remainingBodyBytes;

    /** Transaction ID of the procedure. */
    uint8_t transactionID;

    /** Status of the procedure. */
    HAPBLEFallbackProcedureStatus status;
} HAPBLEFallbackProcedure;

HAP_STATIC_ASSERT(sizeof(HAPBLEFallbackProcedure) <= 16, HAPBLEFallbackProcedureMustBeKeptSmall);

typedef struct _HAPBLEGATTTableElement {
    /**
     * The linked HomeKit characteristic.
     *
     * - If this is NULL, the entry is only linked to a HomeKit service.
     */
    const HAPCharacteristic* _Nullable characteristic;

    /**
     * The linked HomeKit service.
     *
     * - If this is NULL, the table entry is not used.
     */
    const HAPService* _Nullable service;

    /**
     * The linked HomeKit accessory.
     *
     * - If this is NULL, the table entry is not used.
     */
    const HAPAccessory* _Nullable accessory;

    /**
     * Attribute handle of the Characteristic Value declaration.
     */
    HAPPlatformBLEPeripheralManagerAttributeHandle valueHandle;

    /**
     * Attribute handle of the added Client Characteristic Configuration descriptor.
     *
     * - This is only available for HomeKit characteristics that support HAP Events.
     *
     * - If BLE Indications are enabled, the value of this descriptor contains ((uint16_t) 0x0002) in little endian.
     *   If BLE Indications are disabled, the value of this descriptor contains ((uint16_t) 0x0000) in little endian.
     */
    HAPPlatformBLEPeripheralManagerAttributeHandle cccDescriptorHandle;

    /**
     * For HomeKit characteristics: Attribute handle of the added Characteristic Instance ID descriptor.
     * For HomeKit services: Characteristic Value declaration of the added Service Instance ID characteristic.
     */
    HAPPlatformBLEPeripheralManagerAttributeHandle iidHandle;

    /**
     * State related about the connected controller.
     */
    struct {
        /**
         * Fallback procedure in case there are not enough resources to use a full-featured one.
         */
        HAPBLEFallbackProcedure fallbackProcedure;

        /**
         * Whether or not the connected central subscribed to this characteristic.
         *
         * - This is only available for HomeKit characteristics that support HAP Events.
         */
        bool centralSubscribed : 1;

        /**
         * Whether or not the characteristic value changed since the last read by the connected controller.
         *
         * - This is only maintained for HomeKit characteristics that support HAP Events.
         */
        bool pendingEvent : 1;
    } connectionState;
} HAPBLEGATTTableElement;
HAP_NONNULL_SUPPORT(HAPBLEGATTTableElement)

/**
 * Releases all resources that have been allocated by the peripheral manager.
 *
 * @param      server               Accessory server.
 */
void HAPBLEPeripheralManagerRelease(HAPAccessoryServer* server);

/**
 * Registers the accessory server's GATT DB.
 *
 * @param      server               Accessory server.
 */
void HAPBLEPeripheralManagerRegister(HAPAccessoryServer* server);

/**
 * Raises an event notification for a given characteristic in a given service provided by a given accessory object.
 *
 * @param      server               Accessory server.
 * @param      characteristic       The characteristic whose value has changed.
 * @param      service              The service that contains the characteristic.
 * @param      accessory            The accessory that provides the service.
 */
void HAPBLEPeripheralManagerRaiseEvent(
        HAPAccessoryServer* server,
        const HAPCharacteristic* characteristic,
        const HAPService* service,
        const HAPAccessory* accessory);

/**
 * Informs the peripheral manager that a HomeKit Session was accepted.
 *
 * - This is called after the application has been informed that the session was accepted.
 *
 * @param      server               Accessory server.
 * @param      session              The newly accepted session.
 */
void HAPBLEPeripheralManagerHandleSessionAccept(HAPAccessoryServer* server, HAPSession* session);

/**
 * Informs the peripheral manager that a HomeKit Session was invalidated.
 *
 * - This is called before the application is informed that the session was invalidated.
 *
 * @param      server               Accessory server.
 * @param      session              The session being invalidated.
 */
void HAPBLEPeripheralManagerHandleSessionInvalidate(HAPAccessoryServer* server, HAPSession* session);

#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
