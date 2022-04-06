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

#include "DB.h"
#include "App.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * IID constants.
 */
//----------------------------------------------------------------------------------------------------------------------
/**
 * The 'Service Signature' characteristic of the Sprinkler service.
 */
static const HAPDataCharacteristic sprinklerServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_SprinklerServiceSignature,
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
    .callbacks = { .handleRead = HAPHandleServiceSignatureRead, .handleWrite = NULL }
};

/**
 * The 'Service Signature' characteristic of the second Valve service.
 */
static const HAPDataCharacteristic secondValveServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_SecondValveServiceSignature,
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
    .callbacks = { .handleRead = HAPHandleServiceSignatureRead, .handleWrite = NULL }
};

/**
 * The 'Name' characteristic of the Sprinkler service.
 */
static const HAPStringCharacteristic sprinklerNameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_SprinklerName,
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
    .callbacks = { .handleRead = HAPHandleNameRead, .handleWrite = NULL }
};

/**
 * The 'Name' characteristic of the second Valve service.
 */
static const HAPStringCharacteristic secondValveNameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_SecondValveName,
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
    .callbacks = { .handleRead = HAPHandleNameRead, .handleWrite = NULL }
};

/**
 * The 'Active' characteristic of the Sprinkler service.
 */
const HAPUInt8Characteristic sprinklerActiveCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_SprinklerActive,
    .characteristicType = &kHAPCharacteristicType_Active,
    .debugDescription = kHAPCharacteristicDebugDescription_Active,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false, },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false, }, },
	.constraints = { .minimumValue = 0, .maximumValue = 1, .stepValue = 0, },
    .callbacks = { .handleRead = HandleSprinklerActiveRead, .handleWrite = HandleSprinklerActiveWrite }
};
/**
 * The 'Active' characteristic of the second Valve service.
 */
const HAPUInt8Characteristic secondValveActiveCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_SecondValveActive,
    .characteristicType = &kHAPCharacteristicType_Active,
    .debugDescription = kHAPCharacteristicDebugDescription_Active,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false, },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false, }, },
    .constraints = { .minimumValue = 0, .maximumValue = 1, .stepValue = 0, },
    .callbacks = { .handleRead = HandleValveActiveRead, .handleWrite = HandleValveActiveWrite }
};

/**
 * The 'In Use' characteristic of the second Valve service.
 */
const HAPUInt8Characteristic secondValveInUseCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_SecondValveInUse,
    .characteristicType = &kHAPCharacteristicType_InUse,
    .debugDescription = kHAPCharacteristicDebugDescription_InUse,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false, },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false, }, },
    .constraints = { .minimumValue = 0, .maximumValue = 1, .stepValue = 1 },
    .callbacks = { .handleRead = HandleValveInUseRead, .handleWrite = NULL }
};

/**
 * The 'In Use' characteristic of the Sprinkler service.
 */
const HAPUInt8Characteristic sprinklerInUseCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_SprinklerInUse,
    .characteristicType = &kHAPCharacteristicType_InUse,
    .debugDescription = kHAPCharacteristicDebugDescription_InUse,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false, },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false, }, },
    .constraints = { .minimumValue = 0, .maximumValue = 1, .stepValue = 1 },
    .callbacks = { .handleRead = HandleSprinklerInUseRead, .handleWrite = NULL }
};

/**
 * The 'Is Configured' characteristic of the second Valve service.
 */
const HAPUInt8Characteristic secondValveIsConfiguredCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_SecondValveIsConfigured,
    .characteristicType = &kHAPCharacteristicType_IsConfigured,
    .debugDescription = kHAPCharacteristicDebugDescription_IsConfigured,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false, },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false, }, },
    .constraints = { .minimumValue = 0, .maximumValue = 1, .stepValue = 1, },
    .callbacks = { .handleRead = HandleValveIsConfiguredRead, .handleWrite = HandleValveIsConfiguredWrite }
};

/**
 * The 'Valve Type' characteristic of the second Valve service.
 *
 */
const HAPUInt8Characteristic secondValveTypeCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_SecondValveType,
    .characteristicType = &kHAPCharacteristicType_ValveType,
    .debugDescription = kHAPCharacteristicDebugDescription_ValveType,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false, },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false, }, },
    .units = kHAPCharacteristicUnits_None,
    .constraints = { .minimumValue = 0,
                     .maximumValue = UINT8_MAX,
                     .stepValue = 0,
                     .validValues = NULL,
                     .validValuesRanges = NULL, },
    .callbacks = { .handleRead = HandleValveTypeRead, .handleWrite = NULL }
};

/**
 * The 'Set Duration' characteristic of the second Valve service.
 *
 */
const HAPUInt32Characteristic secondValveSetDurationCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt32,
    .iid = kIID_SecondValveSetDuration,
    .characteristicType = &kHAPCharacteristicType_SetDuration,
    .debugDescription = kHAPCharacteristicDebugDescription_SetDuration,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false, },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false, }, },
    .units = kHAPCharacteristicUnits_None,
    .constraints = { .minimumValue = 0, .maximumValue = 3600, .stepValue = 1, },
    .callbacks = { .handleRead = HandleValveSetDurationRead, .handleWrite = HandleValveSetDurationWrite }
};

/**
 * The 'Program Mode' characteristic of the Sprinkler service.
 */
const HAPUInt8Characteristic sprinklerProgramModeCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_SprinklerProgramMode,
    .characteristicType = &kHAPCharacteristicType_ProgramMode,
    .debugDescription = kHAPCharacteristicDebugDescription_ProgramMode,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false, },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false, }, },
    .constraints = { .minimumValue = 0, .maximumValue = 2, .stepValue = 1, },
    .callbacks = { .handleRead = HandleSprinklerProgramModeRead, .handleWrite = NULL }
};

/**
 * The 'Remaining Duration' characteristic of the second Valve service.
 */
const HAPUInt32Characteristic secondValveRemainingDurationCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt32,
    .iid = kIID_SecondValveRemainingDuration,
    .characteristicType = &kHAPCharacteristicType_RemainingDuration,
    .debugDescription = kHAPCharacteristicDebugDescription_RemainingDuration,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false, },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false, }, },
    .units = kHAPCharacteristicUnits_None,
    .constraints = { .minimumValue = 0, .maximumValue = 3600, .stepValue = 1, },
    .callbacks = { .handleRead = HandleValveRemainingDurationRead, .handleWrite = NULL }
};

/**
 * The second Valve service that contains the following characteristics:
 * - 'Active'
 * - 'In Use'
 * - 'Valve Type'
 * - 'Is Configured'
 * - 'Set Duration'
 * - 'Remaining Duration'
 */
const HAPService secondValveService = {
    .iid = kIID_SecondValve,
    .serviceType = &kHAPServiceType_Valve,
    .debugDescription = kHAPServiceDebugDescription_Valve,
    .name = "Second Valve",
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false, }, },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &secondValveServiceSignatureCharacteristic,
                                                            &secondValveNameCharacteristic,
                                                            &secondValveActiveCharacteristic,
                                                            &secondValveInUseCharacteristic,
                                                            &secondValveTypeCharacteristic,
                                                            &secondValveIsConfiguredCharacteristic,
                                                            &secondValveSetDurationCharacteristic,
                                                            &secondValveRemainingDurationCharacteristic,
                                                            NULL }
};

/**
 * The Sprinkler service that contains the following characteristics:
 * - 'Active'
 * - 'Program Mode
 * - 'In Use'
 */
const HAPService sprinklerService = {
    .iid = kIID_Sprinkler,
    .serviceType = &kHAPServiceType_IrrigationSystem,
    .debugDescription = kHAPServiceDebugDescription_IrrigationSystem,
    .name = "Sprinkler",
    .properties = { .primaryService = true, .hidden = false, .ble = { .supportsConfiguration = false, }, },
    .linkedServices = (uint16_t const[]) { kIID_Valve, kIID_SecondValve, 0 },
    .characteristics = (const HAPCharacteristic* const[]) { &sprinklerServiceSignatureCharacteristic,
                                                            &sprinklerNameCharacteristic,
                                                            &sprinklerActiveCharacteristic,
                                                            &sprinklerProgramModeCharacteristic,
                                                            &sprinklerInUseCharacteristic,
                                                            NULL }
};
