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
// Copyright (C) 2020-2021 Apple Inc. All Rights Reserved.

#include "BatteryServiceDB.h"

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HandleBatteryLevelRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED);

HAP_RESULT_USE_CHECK
HAPError HandleBatteryChargingStateRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED);

HAP_RESULT_USE_CHECK
HAPError HandleBatteryStatusLowRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED);

/**
 * The 'Name' characteristic of the Battery service.
 */
static const HAPStringCharacteristic batteryNameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_BatteryName,
    .characteristicType = &kHAPCharacteristicType_Name,
    .debugDescription = kHAPCharacteristicDebugDescription_Name,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .readRequiresAdminPermissions = false,
                    .writeRequiresAdminPermissions = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleNameRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

/**
 * The 'Battery Level' characteristic of the Battery service.
 */
static const HAPUInt8Characteristic batteryLevelCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_BatteryLevel,
    .characteristicType = &kHAPCharacteristicType_BatteryLevel,
    .debugDescription = kHAPCharacteristicDebugDescription_BatteryLevel,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .readRequiresAdminPermissions = false,
                    .writeRequiresAdminPermissions = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .units = kHAPCharacteristicUnits_Percentage,
    .constraints = { .minimumValue = 0,
                     .maximumValue = 100,
                     .stepValue = 1,
                     .validValues = NULL,
                     .validValuesRanges = NULL },
    .callbacks = { .handleRead = HandleBatteryLevelRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

/**
 * The 'Charging State' characteristic of the Battery service.
 */
static const HAPUInt8Characteristic batteryChargingStateCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_BatteryChargingState,
    .characteristicType = &kHAPCharacteristicType_ChargingState,
    .debugDescription = kHAPCharacteristicDebugDescription_ChargingState,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .readRequiresAdminPermissions = false,
                    .writeRequiresAdminPermissions = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .minimumValue = 0,
                     .maximumValue = 2,
                     .stepValue = 1,
                     .validValues = NULL,
                     .validValuesRanges = NULL },
    .callbacks = { .handleRead = HandleBatteryChargingStateRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

/**
 * The 'Status Low Battery' characteristic of the Battery service.
 */
const HAPUInt8Characteristic batteryStatusLowCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_BatteryStatusLow,
    .characteristicType = &kHAPCharacteristicType_StatusLowBattery,
    .debugDescription = kHAPCharacteristicDebugDescription_StatusLowBattery,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .readRequiresAdminPermissions = false,
                    .writeRequiresAdminPermissions = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .minimumValue = 0,
                     .maximumValue = 1,
                     .stepValue = 1,
                     .validValues = NULL,
                     .validValuesRanges = NULL },
    .callbacks = { .handleRead = HandleBatteryStatusLowRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPService batteryService = {
    .iid = kIID_Battery,
    .serviceType = &kHAPServiceType_BatteryService,
    .debugDescription = kHAPServiceDebugDescription_BatteryService,
    .name = "Battery",
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &batteryLevelCharacteristic,
                                                            &batteryChargingStateCharacteristic,
                                                            &batteryStatusLowCharacteristic,
                                                            &batteryNameCharacteristic,
                                                            NULL }
};
