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
// Information Service, the Pairing service and finally the service signature exposed by the fan.

#include "DB.h"
#include "App.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------------------------------------------------------------

/**
 * The 'Service Signature' characteristic of the Fan service.
 */
static const HAPDataCharacteristic fanServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_FanServiceSignature,
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
 * The 'Name' characteristic of the Fan service.
 */
static const HAPStringCharacteristic fanNameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_FanName,
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
 * The 'Active' characteristic of the Fan service.
 * Permissions: Paired Read, Paired Write, Notify
 */
const HAPUInt8Characteristic fanActiveCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_FanActive,
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
    .callbacks = { .handleRead = HandleFanActiveRead, .handleWrite = HandleFanActiveWrite }
};

/**
 * The 'Current Fan State' characteristic of the Fan service.
 * Permissions: Paired Read, Notify
 */
const HAPUInt8Characteristic fanCurrentFanStateCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_FanCurrentFanState,
    .characteristicType = &kHAPCharacteristicType_CurrentFanState,
    .debugDescription = kHAPCharacteristicDebugDescription_CurrentFanState,
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
                     .maximumValue = 2,
                     .stepValue = 1,
                     .validValues = NULL,
                     .validValuesRanges = NULL },
    .callbacks = { .handleRead = HandleFanCurrentFanStateRead, .handleWrite = NULL }
};

/**
 * The 'Target Fan State' characteristic of the Fan service.
 * Permissions: Paired Read, Paired Write, Notify
 */
const HAPUInt8Characteristic fanTargetFanStateCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_FanTargetFanState,
    .characteristicType = &kHAPCharacteristicType_TargetFanState,
    .debugDescription = kHAPCharacteristicDebugDescription_TargetFanState,
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
    .callbacks = { .handleRead = HandleFanTargetFanStateRead, .handleWrite = HandleFanTargetFanStateWrite }
};

/**
 * The Fan service that contains the following characteristics:
 * - 'Active'
 * - 'Current Fan State'
 * - 'Target Fan State'
 */
const HAPService fanService = {
    .iid = kIID_Fan,
    .serviceType = &kHAPServiceType_Fan,
    .debugDescription = kHAPServiceDebugDescription_Fan,
    .name = "Fan",
    .properties = { .primaryService = true, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = (uint16_t const[]) { kIID_Slat, 0 },
    .characteristics = (const HAPCharacteristic* const[]) { &fanServiceSignatureCharacteristic,
                                                            &fanNameCharacteristic,
                                                            &fanActiveCharacteristic,
                                                            &fanCurrentFanStateCharacteristic,
                                                            &fanTargetFanStateCharacteristic,
                                                            NULL }
};

//----------------------------------------------------------------------------------------------------------------------
/**
 * The 'Service Signature' characteristic of the Slat service.
 */
static const HAPDataCharacteristic slatServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_SlatServiceSignature,
    .characteristicType = &kHAPCharacteristicType_ServiceSignature,
    .debugDescription = kHAPCharacteristicDebugDescription_ServiceSignature,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true, },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false, }, },
    .constraints = { .maxLength = 2097152, },
    .callbacks = { .handleRead = HAPHandleServiceSignatureRead, .handleWrite = NULL, }, };

/**
 * The 'Name' characteristic of the Slat service.
 */
static const HAPStringCharacteristic slatNameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_SlatName,
    .characteristicType = &kHAPCharacteristicType_Name,
    .debugDescription = kHAPCharacteristicDebugDescription_Name,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false, },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false, }, },
    .constraints = { .maxLength = 64, },
    .callbacks = { .handleRead = HAPHandleNameRead, .handleWrite = NULL, }, };

/**
 * The 'Current Slat State' characteristic of the Slat service.
 * Permissions: Paired Read, Notify
 */
const HAPUInt8Characteristic slatCurrentSlatStateCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_SlatCurrentSlatState,
    .characteristicType = &kHAPCharacteristicType_CurrentSlatState,
    .debugDescription = kHAPCharacteristicDebugDescription_CurrentSlatState,
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
                     .maximumValue = 2,
                     .stepValue = 1,
                     .validValues = NULL,
                     .validValuesRanges = NULL },
    .callbacks = { .handleRead = HandleSlatCurrentSlatStateRead, .handleWrite = NULL }
};

/**
 * The 'Slat Type' characteristic of the Slat service.
 * Permissions: Paired Read
 */
const HAPUInt8Characteristic slatSlatTypeCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_SlatSlatType,
    .characteristicType = &kHAPCharacteristicType_SlatType,
    .debugDescription = kHAPCharacteristicDebugDescription_SlatType,
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
    .units = kHAPCharacteristicUnits_None,
    .constraints = { .minimumValue = 0,
                     .maximumValue = 1,
                     .stepValue = 1,
                     .validValues = NULL,
                     .validValuesRanges = NULL },
    .callbacks = { .handleRead = HandleSlatSlatTypeRead, .handleWrite = NULL }
};

/**
 * The Slat service that contains the following characteristics:
 * - 'Current Slat State'
 * - 'Slat Type'
 */
const HAPService slatService = {
    .iid = kIID_Slat,
    .serviceType = &kHAPServiceType_Slat,
    .debugDescription = kHAPServiceDebugDescription_Slat,
    .name = "Slat",
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &slatServiceSignatureCharacteristic,
                                                            &slatNameCharacteristic,
                                                            &slatCurrentSlatStateCharacteristic,
                                                            &slatSlatTypeCharacteristic,
                                                            NULL }
};
