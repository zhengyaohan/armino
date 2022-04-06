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

// This file contains the accessory attribute database that defines the accessory information service, HAP Protocol
// Information Service, the Pairing service and finally the service signature exposed by the humidifier/dehumidifier.

#include "DB.h"
#include "App.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------------------------------------------------------------

/**
 * The 'Service Signature' characteristic of the Humidifier/Dehumidifier service.
 */
static const HAPDataCharacteristic humidifierDehumidifierServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_HumidifierDehumidifierServiceSignature,
    .characteristicType = &kHAPCharacteristicType_ServiceSignature,
    .debugDescription = kHAPCharacteristicDebugDescription_ServiceSignature,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 2097152 },
    .callbacks = { .handleRead = HAPHandleServiceSignatureRead, .handleWrite = NULL }
};

/**
 * The 'Name' characteristic of the Humidifier/Dehumidifier service.
 */
static const HAPStringCharacteristic humidifierDehumidifierNameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_HumidifierDehumidifierName,
    .characteristicType = &kHAPCharacteristicType_Name,
    .debugDescription = kHAPCharacteristicDebugDescription_Name,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleNameRead, .handleWrite = NULL }
};

/**
 * The 'Active' characteristic of the Humidifier/Dehumidifier service.
 * Permissions: Paired Read, Paired Write, Notify
 */
const HAPUInt8Characteristic humidifierDehumidifierActiveCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_HumidifierDehumidifierActive,
    .characteristicType = &kHAPCharacteristicType_Active,
    .debugDescription = kHAPCharacteristicDebugDescription_Active,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .units = kHAPCharacteristicUnits_None,
    .constraints = { .minimumValue = 0,
                     .maximumValue = 1,
                     .stepValue = 1,
                     .validValues = NULL,
                     .validValuesRanges = NULL },
    .callbacks = { .handleRead = HandleHumidifierDehumidifierActiveRead,
                   .handleWrite = HandleHumidifierDehumidifierActiveWrite }
};

/**
 * The 'Current Relative Humidity' characteristic of the Humidifier/Dehumidifier service.
 * Permissions: Paired Read, Notify
 */
const HAPFloatCharacteristic humidifierDehumidifierCurrentRelativeHumidityCharacteristic = {
    .format = kHAPCharacteristicFormat_Float,
    .iid = kIID_HumidifierDehumidifierCurrentRelativeHumidity,
    .characteristicType = &kHAPCharacteristicType_CurrentRelativeHumidity,
    .debugDescription = kHAPCharacteristicDebugDescription_CurrentRelativeHumidity,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .units = kHAPCharacteristicUnits_Percentage,
    .constraints = { .minimumValue = 0.0F, .maximumValue = 100.0F, .stepValue = 1.0F },
    .callbacks = { .handleRead = HandleHumidifierDehumidifierCurrentRelativeHumidityRead, .handleWrite = NULL }
};

/**
 * The 'Current Humidifier Dehumidifier State' characteristic of the Humidifier/Dehumidifier service.
 * Permissions: Paired Read, Notify
 */
const HAPUInt8Characteristic humidifierDehumidifierCurrentHumidifierDehumidifierStateCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_HumidifierDehumidifierCurrentHumidifierDehumidifierState,
    .characteristicType = &kHAPCharacteristicType_CurrentHumidifierDehumidifierState,
    .debugDescription = kHAPCharacteristicDebugDescription_CurrentHumidifierDehumidifierState,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .units = kHAPCharacteristicUnits_None,
    .constraints = { .minimumValue = 0,
                     .maximumValue = 3,
                     .stepValue = 1,
                     .validValues = NULL,
                     .validValuesRanges = NULL },
    .callbacks = { .handleRead = HandleHumidifierDehumidifierCurrentHumidifierDehumidifierStateRead,
                   .handleWrite = NULL }
};

/**
 * The 'Target Humidifier Dehumidifier State' characteristic of the Humidifier/Dehumidifier service.
 * Permissions: Paired Read, Paired Write, Notify
 */
const HAPUInt8Characteristic humidifierDehumidifierTargetHumidifierDehumidifierStateCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_HumidifierDehumidifierTargetHumidifierDehumidifierState,
    .characteristicType = &kHAPCharacteristicType_TargetHumidifierDehumidifierState,
    .debugDescription = kHAPCharacteristicDebugDescription_TargetHumidifierDehumidifierState,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .units = kHAPCharacteristicUnits_None,
    .constraints = { .minimumValue = 0,
                     .maximumValue = 2,
                     .stepValue = 1,
                     .validValues = NULL,
                     .validValuesRanges = NULL },
    .callbacks = { .handleRead = HandleHumidifierDehumidifierTargetHumidifierDehumidifierStateRead,
                   .handleWrite = HandleHumidifierDehumidifierTargetHumidifierDehumidifierStateWrite }
};

/**
 * The 'Relative Humidity Dehumidifier Threshold' characteristic of the Humidifier/Dehumidifier service.
 * Permissions: Paired Read, Paired Write, Notify
 */
const HAPFloatCharacteristic humidifierDehumidifierRelativeHumidityDehumidifierThresholdCharacteristic = {
    .format = kHAPCharacteristicFormat_Float,
    .iid = kIID_HumidifierDehumidifierRelativeHumidityDehumidifierThreshold,
    .characteristicType = &kHAPCharacteristicType_RelativeHumidityDehumidifierThreshold,
    .debugDescription = kHAPCharacteristicDebugDescription_RelativeHumidityDehumidifierThreshold,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .units = kHAPCharacteristicUnits_Percentage,
    .constraints = { .minimumValue = 0.0F, .maximumValue = 100.0F, .stepValue = 1.0F },
    .callbacks = { .handleRead = HandleHumidifierDehumidifierRelativeHumidityDehumidifierThresholdRead,
                   .handleWrite = HandleHumidifierDehumidifierRelativeHumidityDehumidifierThresholdWrite }
};

/**
 * The 'Relative Humidity Humidifier Threshold' characteristic of the Humidifier/Dehumidifier service.
 * Permissions: Paired Read, Paired Write, Notify
 */
const HAPFloatCharacteristic humidifierDehumidifierRelativeHumidityHumidifierThresholdCharacteristic = {
    .format = kHAPCharacteristicFormat_Float,
    .iid = kIID_HumidifierDehumidifierRelativeHumidityHumidifierThreshold,
    .characteristicType = &kHAPCharacteristicType_RelativeHumidityHumidifierThreshold,
    .debugDescription = kHAPCharacteristicDebugDescription_RelativeHumidityHumidifierThreshold,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .units = kHAPCharacteristicUnits_Percentage,
    .constraints = { .minimumValue = 0.0F, .maximumValue = 100.0F, .stepValue = 1.0F },
    .callbacks = { .handleRead = HandleHumidifierDehumidifierRelativeHumidityHumidifierThresholdRead,
                   .handleWrite = HandleHumidifierDehumidifierRelativeHumidityHumidifierThresholdWrite }
};

/**
 * The Humidifier/Dehumidifier service that contains the following characteristics:
 * - 'Active'
 * - 'Current Relative Humidity'
 * - 'Current Humidifier Dehumidifier State'
 * - 'Target Humidifier Dehumidifier State'
 * - 'Relative Humidity Dehumidifier Threshold'
 * - 'Relative Humidity Humidifier Threshold'
 */
const HAPService humidifierDehumidifierService = {
    .iid = kIID_HumidifierDehumidifier,
    .serviceType = &kHAPServiceType_HumidifierDehumidifier,
    .debugDescription = kHAPServiceDebugDescription_HumidifierDehumidifier,
    .name = "Humidifier/Dehumidifier",
    .properties = { .primaryService = true, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics =
            (const HAPCharacteristic* const[]) {
                    &humidifierDehumidifierServiceSignatureCharacteristic,
                    &humidifierDehumidifierNameCharacteristic,
                    &humidifierDehumidifierActiveCharacteristic,
                    &humidifierDehumidifierCurrentRelativeHumidityCharacteristic,
                    &humidifierDehumidifierCurrentHumidifierDehumidifierStateCharacteristic,
                    &humidifierDehumidifierTargetHumidifierDehumidifierStateCharacteristic,
                    &humidifierDehumidifierRelativeHumidityDehumidifierThresholdCharacteristic,
                    &humidifierDehumidifierRelativeHumidityHumidifierThresholdCharacteristic,
                    NULL }
};
