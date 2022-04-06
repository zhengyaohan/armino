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

// This file contains the accessory attribute database that defines the accessory information service, HAP Protocol
// Information Service, the Pairing service and finally the service signature exposed by the
// Stateless Programmable Switch.
#include "DB.h"
#include "App.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * IID constants.
 */
#define kIID_ServiceLabel                      ((uint64_t) 0x0030)
#define kIID_ServiceLabelServiceSignature      ((uint64_t) 0x0031)
#define kIID_ServiceLabelServiceLabelNamespace ((uint64_t) 0x0032)

#define kIID_StatelessProgrammableSwitch0                        ((uint64_t) 0x0040)
#define kIID_StatelessProgrammableSwitch0ServiceSignature        ((uint64_t) 0x0041)
#define kIID_StatelessProgrammableSwitch0Name                    ((uint64_t) 0x0042)
#define kIID_StatelessProgrammableSwitch0ProgrammableSwitchEvent ((uint64_t) 0x0043)
#define kIID_StatelessProgrammableSwitch0ServiceLabelIndex       ((uint64_t) 0x0044)

#define kIID_StatelessProgrammableSwitch1                        ((uint64_t) 0x0050)
#define kIID_StatelessProgrammableSwitch1ServiceSignature        ((uint64_t) 0x0051)
#define kIID_StatelessProgrammableSwitch1Name                    ((uint64_t) 0x0052)
#define kIID_StatelessProgrammableSwitch1ProgrammableSwitchEvent ((uint64_t) 0x0053)
#define kIID_StatelessProgrammableSwitch1ServiceLabelIndex       ((uint64_t) 0x0054)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------------------------------------------------------------
/**
 * The 'Service Signature' characteristic of the Service Label service.
 */
static const HAPDataCharacteristic serviceLabelServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_ServiceLabelServiceSignature,
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
 * The 'Service Label Namespace' characteristic of the Service Label service.
 */
static const HAPUInt8Characteristic serviceLabelServiceLabelNamespaceCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_ServiceLabelServiceLabelNamespace,
    .characteristicType = &kHAPCharacteristicType_ServiceLabelNamespace,
    .debugDescription = kHAPCharacteristicDebugDescription_ServiceLabelNamespace,
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
    .callbacks = { .handleRead = HandleServiceLabelServiceLabelNamespaceRead, .handleWrite = NULL }
};

/**
 * The Service Label service that contains the 'Service Label Namespace' characteristic.
 */
const HAPService serviceLabelService = {
    .iid = kIID_ServiceLabel,
    .serviceType = &kHAPServiceType_ServiceLabel,
    .debugDescription = kHAPServiceDebugDescription_ServiceLabel,
    .name = NULL,
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &serviceLabelServiceSignatureCharacteristic,
                                                            &serviceLabelServiceLabelNamespaceCharacteristic,
                                                            NULL }
};

//----------------------------------------------------------------------------------------------------------------------

/**
 * The 'Service Signature' characteristic of the first Stateless Programmable Switch service.
 */
static const HAPDataCharacteristic statelessProgrammableSwitch0ServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_StatelessProgrammableSwitch0ServiceSignature,
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
 * The 'Name' characteristic of the first Stateless Programmable Switch service.
 */
static const HAPStringCharacteristic statelessProgrammableSwitch0NameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_StatelessProgrammableSwitch0Name,
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
 * The 'Programmable Switch Event' characteristic of the first Stateless Programmable Switch service.
 */
const HAPUInt8Characteristic statelessProgrammableSwitch0ProgrammableSwitchEventCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_StatelessProgrammableSwitch0ProgrammableSwitchEvent,
    .characteristicType = &kHAPCharacteristicType_ProgrammableSwitchEvent,
    .debugDescription = kHAPCharacteristicDebugDescription_ProgrammableSwitchEvent,
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
                     .validValues =
                             (const uint8_t*[]) {
                                     &(const uint8_t) { kHAPCharacteristicValue_ProgrammableSwitchEvent_SinglePress },
                                     &(const uint8_t) { kHAPCharacteristicValue_ProgrammableSwitchEvent_DoublePress },
                                     &(const uint8_t) { kHAPCharacteristicValue_ProgrammableSwitchEvent_LongPress },
                                     NULL },
                     .validValuesRanges = NULL },
    .callbacks = { .handleRead = HandleStatelessProgrammableSwitchProgrammableSwitchEventRead, .handleWrite = NULL }
};

/**
 * The 'Service Label Index' characteristic of the first Stateless Programmable Switch service.
 */
static const HAPUInt8Characteristic statelessProgrammableSwitch0ServiceLabelIndexCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_StatelessProgrammableSwitch0ServiceLabelIndex,
    .characteristicType = &kHAPCharacteristicType_ServiceLabelIndex,
    .debugDescription = kHAPCharacteristicDebugDescription_ServiceLabelIndex,
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
    .constraints = { .minimumValue = 1,
                     .maximumValue = UINT8_MAX,
                     .stepValue = 1,
                     .validValues = NULL,
                     .validValuesRanges = NULL },
    .callbacks = { .handleRead = HandleStatelessProgrammableSwitchServiceLabelIndexRead, .handleWrite = NULL }
};

/**
 * The first Stateless Programmable Switch service that contains the characteristics
 * for the first stateless programmable switch.
 */
const HAPService statelessProgrammableSwitch0Service = {
    .iid = kIID_StatelessProgrammableSwitch0,
    .serviceType = &kHAPServiceType_StatelessProgrammableSwitch,
    .debugDescription = kHAPServiceDebugDescription_StatelessProgrammableSwitch,
    .name = "Stateless Programmable Switch 1",
    .properties = { .primaryService = true, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = (uint16_t const[]) { kIID_ServiceLabel, 0 },
    .characteristics =
            (const HAPCharacteristic* const[]) { &statelessProgrammableSwitch0ServiceSignatureCharacteristic,
                                                 &statelessProgrammableSwitch0NameCharacteristic,
                                                 &statelessProgrammableSwitch0ProgrammableSwitchEventCharacteristic,
                                                 &statelessProgrammableSwitch0ServiceLabelIndexCharacteristic,
                                                 NULL }
};

//----------------------------------------------------------------------------------------------------------------------

/**
 * The 'Service Signature' characteristic of the second Stateless Programmable Switch service.
 */
static const HAPDataCharacteristic statelessProgrammableSwitch1ServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_StatelessProgrammableSwitch1ServiceSignature,
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
 * The 'Name' characteristic of the second Stateless Programmable Switch service.
 */
static const HAPStringCharacteristic statelessProgrammableSwitch1NameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_StatelessProgrammableSwitch1Name,
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
 * The 'Programmable Switch Event' characteristic of the second Stateless Programmable Switch service.
 */
const HAPUInt8Characteristic statelessProgrammableSwitch1ProgrammableSwitchEventCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_StatelessProgrammableSwitch1ProgrammableSwitchEvent,
    .characteristicType = &kHAPCharacteristicType_ProgrammableSwitchEvent,
    .debugDescription = kHAPCharacteristicDebugDescription_ProgrammableSwitchEvent,
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
                     .validValues =
                             (const uint8_t*[]) {
                                     &(const uint8_t) { kHAPCharacteristicValue_ProgrammableSwitchEvent_SinglePress },
                                     &(const uint8_t) { kHAPCharacteristicValue_ProgrammableSwitchEvent_DoublePress },
                                     &(const uint8_t) { kHAPCharacteristicValue_ProgrammableSwitchEvent_LongPress },
                                     NULL },
                     .validValuesRanges = NULL },
    .callbacks = { .handleRead = HandleStatelessProgrammableSwitchProgrammableSwitchEventRead, .handleWrite = NULL }
};

/**
 * The 'Service Label Index' characteristic of the second Stateless Programmable Switch service.
 */
static const HAPUInt8Characteristic statelessProgrammableSwitch1ServiceLabelIndexCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_StatelessProgrammableSwitch1ServiceLabelIndex,
    .characteristicType = &kHAPCharacteristicType_ServiceLabelIndex,
    .debugDescription = kHAPCharacteristicDebugDescription_ServiceLabelIndex,
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
    .constraints = { .minimumValue = 1,
                     .maximumValue = UINT8_MAX,
                     .stepValue = 1,
                     .validValues = NULL,
                     .validValuesRanges = NULL },
    .callbacks = { .handleRead = HandleStatelessProgrammableSwitchServiceLabelIndexRead, .handleWrite = NULL }
};

/**
 * The second Stateless Programmable Switch service that contains the characteristics
 * for the second stateless programmable switch.
 */
const HAPService statelessProgrammableSwitch1Service = {
    .iid = kIID_StatelessProgrammableSwitch1,
    .serviceType = &kHAPServiceType_StatelessProgrammableSwitch,
    .debugDescription = kHAPServiceDebugDescription_StatelessProgrammableSwitch,
    .name = "Stateless Programmable Switch 2",
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = (uint16_t const[]) { kIID_ServiceLabel, 0 },
    .characteristics =
            (const HAPCharacteristic* const[]) { &statelessProgrammableSwitch1ServiceSignatureCharacteristic,
                                                 &statelessProgrammableSwitch1NameCharacteristic,
                                                 &statelessProgrammableSwitch1ProgrammableSwitchEventCharacteristic,
                                                 &statelessProgrammableSwitch1ServiceLabelIndexCharacteristic,
                                                 NULL }
};
