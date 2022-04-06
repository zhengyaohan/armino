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
// Information Service, the Pairing service and finally the service signature exposed by the window covering opener.

#include "DB.h"
#include "App.h"

#include "HAPServiceTypes.h"

#include "ServiceIIDs.h"

/**
 * IID constants.
 */
#define kIID_WindowCovering                      ((uint64_t) 0x100)
#define kIID_WindowCoveringServiceSignature      ((uint64_t) 0x101)
#define kIID_WindowCoveringCurrentPosition       ((uint64_t) 0x102)
#define kIID_WindowCoveringTargetPosition        ((uint64_t) 0x103)
#define kIID_WindowCoveringPositionState         ((uint64_t) 0x104)
#define kIID_WindowCoveringHoldPosition          ((uint64_t) 0x105)
#define kIID_WindowCoveringCurrentHorizontalTilt ((uint64_t) 0x106)
#define kIID_WindowCoveringTargetHorizontalTilt  ((uint64_t) 0x107)
#define kIID_WindowCoveringCurrentVerticalTilt   ((uint64_t) 0x108)
#define kIID_WindowCoveringTargetVerticalTilt    ((uint64_t) 0x109)
#define kIID_WindowCoveringObstructionDetected   ((uint64_t) 0x110)
#define kIID_WindowCoveringName                  ((uint64_t) 0x111)

/**
 * The 'Service Signature' characteristic of the Window Covering service.
 */
const HAPDataCharacteristic windowCoveringServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_WindowCoveringServiceSignature,
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
    .callbacks = { .handleRead = HAPHandleServiceSignatureRead }
};

/**
 * The 'Current Position' characteristic of the Window Covering service.
 */
const HAPUInt8Characteristic windowCoveringCurrentPositionCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_WindowCoveringCurrentPosition,
    .characteristicType = &kHAPCharacteristicType_CurrentPosition,
    .debugDescription = kHAPCharacteristicDebugDescription_CurrentPosition,
    .manufacturerDescription = NULL,
    .units = kHAPCharacteristicUnits_Percentage,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = true,
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
    .constraints = { .minimumValue = 0, .maximumValue = 100, .stepValue = 1 },
    .callbacks = { .handleRead = HAPHandleWindowCoveringCurrentPositionRead }
};

/**
 * The 'Target Position' characteristic of the Window Covering service.
 */
const HAPUInt8Characteristic windowCoveringTargetPositionCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_WindowCoveringTargetPosition,
    .characteristicType = &kHAPCharacteristicType_TargetPosition,
    .debugDescription = kHAPCharacteristicDebugDescription_TargetPosition,
    .manufacturerDescription = NULL,
    .units = kHAPCharacteristicUnits_Percentage,
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = true,
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
    .constraints = { .minimumValue = 0, .maximumValue = 100, .stepValue = 1 },
    .callbacks = { .handleRead = HAPHandleWindowCoveringTargetPositionRead,
                   .handleWrite = HAPHandleWindowCoveringTargetPositionWrite }
};

/**
 * The 'Position State' characteristic of the Window Covering service.
 */
const HAPUInt8Characteristic windowCoveringPositionStateCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_WindowCoveringPositionState,
    .characteristicType = &kHAPCharacteristicType_PositionState,
    .debugDescription = kHAPCharacteristicDebugDescription_PositionState,
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
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .minimumValue = 0, .maximumValue = 2, .stepValue = 1 },
    .callbacks = { .handleRead = HAPHandleWindowCoveringPositionStateRead }
};

/**
 * The 'Hold Position' characteristic of the Window Covering service.
 */
const HAPBoolCharacteristic windowCoveringHoldPositionCharacteristic = {
    .format = kHAPCharacteristicFormat_Bool,
    .iid = kIID_WindowCoveringHoldPosition,
    .characteristicType = &kHAPCharacteristicType_HoldPosition,
    .debugDescription = kHAPCharacteristicDebugDescription_HoldPosition,
    .manufacturerDescription = NULL,
    .properties = { .readable = false,
                    .writable = true,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleWrite = HAPHandleWindowCoveringHoldPositionWrite }
};

/**
 * The 'Current Horizontal Tilt' characteristic of the Window Covering service.
 */
const HAPIntCharacteristic windowCoveringCurrentHorizontalTiltCharacteristic = {
    .format = kHAPCharacteristicFormat_Int,
    .iid = kIID_WindowCoveringCurrentHorizontalTilt,
    .characteristicType = &kHAPCharacteristicType_CurrentHorizontalTiltAngle,
    .debugDescription = kHAPCharacteristicDebugDescription_CurrentHorizontalTiltAngle,
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
    .units = kHAPCharacteristicUnits_ArcDegrees,
    .constraints = { .minimumValue = -90, .maximumValue = 90, .stepValue = 1 },
    .callbacks = { .handleRead = HAPHandleWindowCoveringCurrentHorizontalTiltRead }
};

/**
 * The 'Target Horizontal Tilt' characteristic of the Window Covering service.
 */
const HAPIntCharacteristic windowCoveringTargetHorizontalTiltCharacteristic = {
    .format = kHAPCharacteristicFormat_Int,
    .iid = kIID_WindowCoveringTargetHorizontalTilt,
    .characteristicType = &kHAPCharacteristicType_TargetHorizontalTiltAngle,
    .debugDescription = kHAPCharacteristicDebugDescription_TargetHorizontalTiltAngle,
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
    .units = kHAPCharacteristicUnits_ArcDegrees,
    .constraints = { .minimumValue = -90, .maximumValue = 90, .stepValue = 1 },
    .callbacks = { .handleRead = HAPHandleWindowCoveringTargetHorizontalTiltRead,
                   .handleWrite = HAPHandleWindowCoveringTargetHorizontalTiltWrite }
};

/**
 * The 'Current Vertical Tilt' characteristic of the Window Covering service.
 */
const HAPIntCharacteristic windowCoveringCurrentVerticalTiltCharacteristic = {
    .format = kHAPCharacteristicFormat_Int,
    .iid = kIID_WindowCoveringCurrentVerticalTilt,
    .characteristicType = &kHAPCharacteristicType_CurrentVerticalTiltAngle,
    .debugDescription = kHAPCharacteristicDebugDescription_CurrentVerticalTiltAngle,
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
    .units = kHAPCharacteristicUnits_ArcDegrees,
    .constraints = { .minimumValue = -90, .maximumValue = 90, .stepValue = 1 },
    .callbacks = { .handleRead = HAPHandleWindowCoveringCurrentVerticalTiltRead }
};

/**
 * The 'Target Vertical Tilt' characteristic of the Window Covering service.
 */
const HAPIntCharacteristic windowCoveringTargetVerticalTiltCharacteristic = {
    .format = kHAPCharacteristicFormat_Int,
    .iid = kIID_WindowCoveringTargetVerticalTilt,
    .characteristicType = &kHAPCharacteristicType_TargetVerticalTiltAngle,
    .debugDescription = kHAPCharacteristicDebugDescription_TargetVerticalTiltAngle,
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
    .units = kHAPCharacteristicUnits_ArcDegrees,
    .constraints = { .minimumValue = -90, .maximumValue = 90, .stepValue = 1 },
    .callbacks = { .handleRead = HAPHandleWindowCoveringTargetVerticalTiltRead,
                   .handleWrite = HAPHandleWindowCoveringTargetVerticalTiltWrite }
};

/**
 * The 'Obstruction Detected' characteristic of the Window Covering service.
 */
const HAPBoolCharacteristic windowCoveringObstructionDetectedCharacteristic = {
    .format = kHAPCharacteristicFormat_Bool,
    .iid = kIID_WindowCoveringObstructionDetected,
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
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HAPHandleWindowCoveringObstructionDetectedRead }
};

/**
 * The 'Name' characteristic of the Window Covering service.
 */
const HAPStringCharacteristic windowCoveringNameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_WindowCoveringName,
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
    .callbacks = { .handleRead = HAPHandleNameRead }
};

/**
 * The Window Covering service.
 */
const HAPService windowCoveringService = {
    .iid = kIID_WindowCovering,
    .serviceType = &kHAPServiceType_WindowCovering,
    .debugDescription = kHAPServiceDebugDescription_WindowCovering,
    .name = "Window Covering service",
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &windowCoveringServiceSignatureCharacteristic,
                                                            &windowCoveringCurrentPositionCharacteristic,
                                                            &windowCoveringTargetPositionCharacteristic,
                                                            &windowCoveringPositionStateCharacteristic,
                                                            &windowCoveringHoldPositionCharacteristic,
                                                            &windowCoveringCurrentHorizontalTiltCharacteristic,
                                                            &windowCoveringTargetHorizontalTiltCharacteristic,
                                                            &windowCoveringCurrentVerticalTiltCharacteristic,
                                                            &windowCoveringTargetVerticalTiltCharacteristic,
                                                            &windowCoveringObstructionDetectedCharacteristic,
                                                            &windowCoveringNameCharacteristic,
                                                            NULL }
};
