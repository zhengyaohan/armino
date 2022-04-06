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

// This file contains the accessory attribute database that defines the accessory information service, HAP Protocol
// Information Service, the Pairing service and finally the service signature exposed by the garage door opener.

#include "DB.h"
#include "App.h"

#include "HAPServiceTypes.h"

#include "ServiceIIDs.h"

/**
 * IID constants.
 */
#define kIID_GarageDoorOpener                    ((uint64_t) 0x100)
#define kIID_GarageDoorOpenerServiceSignature    ((uint64_t) 0x101)
#define kIID_GarageDoorOpenerCurrentDoorState    ((uint64_t) 0x102)
#define kIID_GarageDoorOpenerTargetDoorState     ((uint64_t) 0x103)
#define kIID_GarageDoorOpenerObstructionDetected ((uint64_t) 0x104)
#define kIID_GarageDoorOpenerLockCurrentState    ((uint64_t) 0x105)
#define kIID_GarageDoorOpenerLockTargetState     ((uint64_t) 0x106)
#define kIID_GarageDoorOpenerName                ((uint64_t) 0x107)

/**
 * The 'Service Signature' characteristic of the Garage Door Opener service.
 */
const HAPDataCharacteristic garageDoorOpenerServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_GarageDoorOpenerServiceSignature,
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
 * The 'Current Door State' characteristic of the Garage Door Opener service.
 */
const HAPUInt8Characteristic garageDoorOpenerCurrentDoorStateCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_GarageDoorOpenerCurrentDoorState,
    .characteristicType = &kHAPCharacteristicType_CurrentDoorState,
    .debugDescription = kHAPCharacteristicDebugDescription_CurrentDoorState,
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
    .constraints = { .minimumValue = 0, .maximumValue = 4, .stepValue = 1 },
    .callbacks = { .handleRead = HAPHandleGarageDoorOpenerCurrentDoorStateRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

/**
 * The 'Target Door State' characteristic of the Garage Door Opener service.
 */
const HAPUInt8Characteristic garageDoorOpenerTargetDoorStateCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_GarageDoorOpenerTargetDoorState,
    .characteristicType = &kHAPCharacteristicType_TargetDoorState,
    .debugDescription = kHAPCharacteristicDebugDescription_TargetDoorState,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = true,
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
    .constraints = { .minimumValue = 0, .maximumValue = 1, .stepValue = 1 },
    .callbacks = { .handleRead = HAPHandleGarageDoorOpenerTargetDoorStateRead,
                   .handleWrite = HAPHandleGarageDoorOpenerTargetDoorStateWrite,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

/**
 * The 'Obstruction Detected' characteristic of the Garage Door Opener service.
 */
const HAPBoolCharacteristic garageDoorOpenerObstructionDetectedCharacteristic = {
    .format = kHAPCharacteristicFormat_Bool,
    .iid = kIID_GarageDoorOpenerObstructionDetected,
    .characteristicType = &kHAPCharacteristicType_ObstructionDetected,
    .debugDescription = kHAPCharacteristicDebugDescription_ObstructionDetected,
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
    .callbacks = { .handleRead = HAPHandleGarageDoorOpenerObstructionDetectedRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

/**
 * The 'Lock Current State' characteristic of the Garage Door Opener service.
 */
const HAPUInt8Characteristic garageDoorOpenerLockCurrentStateCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_GarageDoorOpenerLockCurrentState,
    .characteristicType = &kHAPCharacteristicType_LockCurrentState,
    .debugDescription = kHAPCharacteristicDebugDescription_LockCurrentState,
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
    .callbacks = { .handleRead = HAPHandleGarageDoorOpenerLockCurrentStateRead, .handleWrite = NULL }
};

/**
 * The 'Lock Target State' characteristic of the Garage Door Opener service.
 */
const HAPUInt8Characteristic garageDoorOpenerLockTargetStateCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_GarageDoorOpenerLockTargetState,
    .characteristicType = &kHAPCharacteristicType_LockTargetState,
    .debugDescription = kHAPCharacteristicDebugDescription_LockTargetState,
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
    .callbacks = { .handleRead = HAPHandleGarageDoorOpenerLockTargetStateRead,
                   .handleWrite = HAPHandleGarageDoorOpenerLockTargetStateWrite }
};

/**
 * The 'Name' characteristic of the Garage Door Opener service.
 */
const HAPStringCharacteristic garageDoorOpenerNameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_GarageDoorOpenerName,
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
 * The Garage Door Opener service.
 */
const HAPService garageDoorOpenerService = {
    .iid = kIID_GarageDoorOpener,
    .serviceType = &kHAPServiceType_GarageDoorOpener,
    .debugDescription = kHAPServiceDebugDescription_GarageDoorOpener,
    .name = "Garage Door Opener service",
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &garageDoorOpenerServiceSignatureCharacteristic,
                                                            &garageDoorOpenerCurrentDoorStateCharacteristic,
                                                            &garageDoorOpenerTargetDoorStateCharacteristic,
                                                            &garageDoorOpenerObstructionDetectedCharacteristic,
                                                            &garageDoorOpenerLockCurrentStateCharacteristic,
                                                            &garageDoorOpenerLockTargetStateCharacteristic,
                                                            &garageDoorOpenerNameCharacteristic,
                                                            NULL }
};
