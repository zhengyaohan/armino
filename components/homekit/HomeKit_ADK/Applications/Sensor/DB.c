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
// Information Service, the Pairing service and finally the service signature exposed by a sensor.

#include "DB.h"
#include "App.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------------------------------------------------------------

#if (SENSOR_AIR_QUALITY == 1)
/**
 * The 'Service Signature' characteristic of the Air Quality Sensor service.
 */
static const HAPDataCharacteristic airQualitySensorServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_AirQualitySensorServiceSignature,
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
 * The 'Name' characteristic of the Air Quality Sensor service.
 */
static const HAPStringCharacteristic airQualitySensorNameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_AirQualitySensorName,
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
 * The 'Air Quality' characteristic of the Air Quality Sensor service.
 * Permissions: Paired Read, Notify
 */
const HAPUInt8Characteristic airQualitySensorAirQualityCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_AirQualitySensorAirQuality,
    .characteristicType = &kHAPCharacteristicType_AirQuality,
    .debugDescription = kHAPCharacteristicDebugDescription_AirQuality,
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
                     .maximumValue = 5,
                     .stepValue = 1,
                     .validValues = NULL,
                     .validValuesRanges = NULL },
    .callbacks = { .handleRead = HandleAirQualitySensorAirQualityRead, .handleWrite = NULL }
};

/**
 * The 'Status Active' characteristic of the Air Quality Sensor service.
 * Permissions: Paired Read, Notify
 */
const HAPBoolCharacteristic airQualitySensorStatusActiveCharacteristic = {
    .format = kHAPCharacteristicFormat_Bool,
    .iid = kIID_AirQualitySensorStatusActive,
    .characteristicType = &kHAPCharacteristicType_StatusActive,
    .debugDescription = kHAPCharacteristicDebugDescription_StatusActive,
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
    .callbacks = { .handleRead = HandleAirQualitySensorStatusActiveRead, .handleWrite = NULL }
};

/**
 * The Air Quality Sensor service that contains the following characteristics:
 * - 'Air Quality'
 * - 'Status Active'
 */
const HAPService airQualitySensorService = {
    .iid = kIID_AirQualitySensor,
    .serviceType = &kHAPServiceType_AirQualitySensor,
    .debugDescription = kHAPServiceDebugDescription_AirQualitySensor,
    .name = "Air Quality Sensor",
    .properties = { .primaryService = true, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &airQualitySensorServiceSignatureCharacteristic,
                                                            &airQualitySensorNameCharacteristic,
                                                            &airQualitySensorAirQualityCharacteristic,
                                                            &airQualitySensorStatusActiveCharacteristic,
                                                            NULL }
};
#endif

#if (SENSOR_CARBON_DIOXIDE == 1)
/**
 * The 'Service Signature' characteristic of the Carbon Dioxide Sensor service.
 */
static const HAPDataCharacteristic carbonDioxideSensorServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_CarbonDioxideSensorServiceSignature,
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
 * The 'Name' characteristic of the Carbon Dioxide Sensor service.
 */
static const HAPStringCharacteristic carbonDioxideSensorNameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_CarbonDioxideSensorName,
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
 * The 'Carbon Dioxide Detected' characteristic of the Carbon Dioxide Sensor service.
 * Permissions: Paired Read, Notify
 */
const HAPUInt8Characteristic carbonDioxideSensorCarbonDioxideDetectedCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_CarbonDioxideSensorCarbonDioxideDetected,
    .characteristicType = &kHAPCharacteristicType_CarbonDioxideDetected,
    .debugDescription = kHAPCharacteristicDebugDescription_CarbonDioxideDetected,
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
                     .maximumValue = 1,
                     .stepValue = 1,
                     .validValues = NULL,
                     .validValuesRanges = NULL },
    .callbacks = { .handleRead = HandleCarbonDioxideSensorCarbonDioxideDetectedRead, .handleWrite = NULL }
};

/**
 * The 'Status Active' characteristic of the Carbon Dioxide Sensor service.
 * Permissions: Paired Read, Notify
 */
const HAPBoolCharacteristic carbonDioxideSensorStatusActiveCharacteristic = {
    .format = kHAPCharacteristicFormat_Bool,
    .iid = kIID_CarbonDioxideSensorStatusActive,
    .characteristicType = &kHAPCharacteristicType_StatusActive,
    .debugDescription = kHAPCharacteristicDebugDescription_StatusActive,
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
    .callbacks = { .handleRead = HandleCarbonDioxideSensorStatusActiveRead, .handleWrite = NULL }
};

/**
 * The Carbon Dioxide Sensor service that contains the following characteristics:
 * - 'Carbon Dioxide Detected'
 * - 'Status Active'
 */
const HAPService carbonDioxideSensorService = {
    .iid = kIID_CarbonDioxideSensor,
    .serviceType = &kHAPServiceType_CarbonDioxideSensor,
    .debugDescription = kHAPServiceDebugDescription_CarbonDioxideSensor,
    .name = "Carbon Dioxide Sensor",
    .properties = { .primaryService = true, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &carbonDioxideSensorServiceSignatureCharacteristic,
                                                            &carbonDioxideSensorNameCharacteristic,
                                                            &carbonDioxideSensorCarbonDioxideDetectedCharacteristic,
                                                            &carbonDioxideSensorStatusActiveCharacteristic,
                                                            NULL }
};
#endif

#if (SENSOR_CARBON_MONOXIDE == 1)
/**
 * The 'Service Signature' characteristic of the Carbon Monoxide Sensor service.
 */
static const HAPDataCharacteristic carbonMonoxideSensorServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_CarbonMonoxideSensorServiceSignature,
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
 * The 'Name' characteristic of the Carbon Monoxide Sensor service.
 */
static const HAPStringCharacteristic carbonMonoxideSensorNameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_CarbonMonoxideSensorName,
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
 * The 'Carbon Monoxide Detected' characteristic of the Carbon Monoxide Sensor service.
 * Permissions: Paired Read, Notify
 */
const HAPUInt8Characteristic carbonMonoxideSensorCarbonMonoxideDetectedCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_CarbonMonoxideSensorCarbonMonoxideDetected,
    .characteristicType = &kHAPCharacteristicType_CarbonMonoxideDetected,
    .debugDescription = kHAPCharacteristicDebugDescription_CarbonMonoxideDetected,
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
                     .maximumValue = 1,
                     .stepValue = 1,
                     .validValues = NULL,
                     .validValuesRanges = NULL },
    .callbacks = { .handleRead = HandleCarbonMonoxideSensorCarbonMonoxideDetectedRead, .handleWrite = NULL }
};

/**
 * The 'Status Active' characteristic of the Carbon Monoxide Sensor service.
 * Permissions: Paired Read, Notify
 */
const HAPBoolCharacteristic carbonMonoxideSensorStatusActiveCharacteristic = {
    .format = kHAPCharacteristicFormat_Bool,
    .iid = kIID_CarbonMonoxideSensorStatusActive,
    .characteristicType = &kHAPCharacteristicType_StatusActive,
    .debugDescription = kHAPCharacteristicDebugDescription_StatusActive,
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
    .callbacks = { .handleRead = HandleCarbonMonoxideSensorStatusActiveRead, .handleWrite = NULL }
};

/**
 * The Carbon Monoxide Sensor service that contains the following characteristics:
 * - 'Carbon Monoxide Detected'
 * - 'Status Active'
 */
const HAPService carbonMonoxideSensorService = {
    .iid = kIID_CarbonMonoxideSensor,
    .serviceType = &kHAPServiceType_CarbonMonoxideSensor,
    .debugDescription = kHAPServiceDebugDescription_CarbonMonoxideSensor,
    .name = "Carbon Monoxide Sensor",
    .properties = { .primaryService = true, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &carbonMonoxideSensorServiceSignatureCharacteristic,
                                                            &carbonMonoxideSensorNameCharacteristic,
                                                            &carbonMonoxideSensorCarbonMonoxideDetectedCharacteristic,
                                                            &carbonMonoxideSensorStatusActiveCharacteristic,
                                                            NULL }
};
#endif

#if (SENSOR_CONTACT == 1)
/**
 * The 'Service Signature' characteristic of the Contact Sensor service.
 */
static const HAPDataCharacteristic contactSensorServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_ContactSensorServiceSignature,
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
 * The 'Name' characteristic of the Contact Sensor service.
 */
static const HAPStringCharacteristic contactSensorNameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_ContactSensorName,
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
 * The 'Contact Sensor State' characteristic of the Contact Sensor service.
 * Permissions: Paired Read, Notify
 */
const HAPUInt8Characteristic contactSensorContactSensorStateCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_ContactSensorContactSensorState,
    .characteristicType = &kHAPCharacteristicType_ContactSensorState,
    .debugDescription = kHAPCharacteristicDebugDescription_ContactSensorState,
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
                     .maximumValue = 1,
                     .stepValue = 1,
                     .validValues = NULL,
                     .validValuesRanges = NULL },
    .callbacks = { .handleRead = HandleContactSensorContactSensorStateRead, .handleWrite = NULL }
};

/**
 * The 'Status Active' characteristic of the Contact Sensor service.
 * Permissions: Paired Read, Notify
 */
const HAPBoolCharacteristic contactSensorStatusActiveCharacteristic = {
    .format = kHAPCharacteristicFormat_Bool,
    .iid = kIID_ContactSensorStatusActive,
    .characteristicType = &kHAPCharacteristicType_StatusActive,
    .debugDescription = kHAPCharacteristicDebugDescription_StatusActive,
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
    .callbacks = { .handleRead = HandleContactSensorStatusActiveRead, .handleWrite = NULL }
};

/**
 * The Contact Sensor service that contains the following characteristics:
 * - 'Contact Sensor State'
 * - 'Status Active'
 */
const HAPService contactSensorService = {
    .iid = kIID_ContactSensor,
    .serviceType = &kHAPServiceType_ContactSensor,
    .debugDescription = kHAPServiceDebugDescription_ContactSensor,
    .name = "Contact Sensor",
    .properties = { .primaryService = true, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &contactSensorServiceSignatureCharacteristic,
                                                            &contactSensorNameCharacteristic,
                                                            &contactSensorContactSensorStateCharacteristic,
                                                            &contactSensorStatusActiveCharacteristic,
                                                            NULL }
};
#endif

#if (SENSOR_HUMIDITY == 1)
/**
 * The 'Service Signature' characteristic of the Humidity Sensor service.
 */
static const HAPDataCharacteristic humiditySensorServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_HumiditySensorServiceSignature,
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
 * The 'Name' characteristic of the Humidity Sensor service.
 */
static const HAPStringCharacteristic humiditySensorNameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_HumiditySensorName,
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
 * The 'Current Relative Humidity' characteristic of the Humidity Sensor service.
 * Permissions: Paired Read, Notify
 */
const HAPFloatCharacteristic humiditySensorCurrentRelativeHumidityCharacteristic = {
    .format = kHAPCharacteristicFormat_Float,
    .iid = kIID_HumiditySensorCurrentRelativeHumidity,
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
    .callbacks = { .handleRead = HandleHumiditySensorCurrentRelativeHumidityRead, .handleWrite = NULL }
};

/**
 * The 'Status Active' characteristic of the Humidity Sensor service.
 * Permissions: Paired Read, Notify
 */
const HAPBoolCharacteristic humiditySensorStatusActiveCharacteristic = {
    .format = kHAPCharacteristicFormat_Bool,
    .iid = kIID_HumiditySensorStatusActive,
    .characteristicType = &kHAPCharacteristicType_StatusActive,
    .debugDescription = kHAPCharacteristicDebugDescription_StatusActive,
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
    .callbacks = { .handleRead = HandleHumiditySensorStatusActiveRead, .handleWrite = NULL }
};

/**
 * The Humidity Sensor service that contains the following characteristics:
 * - 'Current Relative Humidity'
 * - 'Status Active'
 */
const HAPService humiditySensorService = {
    .iid = kIID_HumiditySensor,
    .serviceType = &kHAPServiceType_HumiditySensor,
    .debugDescription = kHAPServiceDebugDescription_HumiditySensor,
    .name = "Humidity Sensor",
    .properties = { .primaryService = true, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &humiditySensorServiceSignatureCharacteristic,
                                                            &humiditySensorNameCharacteristic,
                                                            &humiditySensorCurrentRelativeHumidityCharacteristic,
                                                            &humiditySensorStatusActiveCharacteristic,
                                                            NULL }
};
#endif

#if (SENSOR_LEAK == 1)
/**
 * The 'Service Signature' characteristic of the Leak Sensor service.
 */
static const HAPDataCharacteristic leakSensorServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_LeakSensorServiceSignature,
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
 * The 'Name' characteristic of the Leak Sensor service.
 */
static const HAPStringCharacteristic leakSensorNameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_LeakSensorName,
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
 * The 'Leak Detected' characteristic of the Leak Sensor service.
 * Permissions: Paired Read, Notify
 */
const HAPUInt8Characteristic leakSensorLeakDetectedCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_LeakSensorLeakDetected,
    .characteristicType = &kHAPCharacteristicType_LeakDetected,
    .debugDescription = kHAPCharacteristicDebugDescription_LeakDetected,
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
                     .maximumValue = 1,
                     .stepValue = 1,
                     .validValues = NULL,
                     .validValuesRanges = NULL },
    .callbacks = { .handleRead = HandleLeakSensorLeakDetectedRead, .handleWrite = NULL }
};

/**
 * The 'Status Active' characteristic of the Leak Sensor service.
 * Permissions: Paired Read, Notify
 */
const HAPBoolCharacteristic leakSensorStatusActiveCharacteristic = {
    .format = kHAPCharacteristicFormat_Bool,
    .iid = kIID_LeakSensorStatusActive,
    .characteristicType = &kHAPCharacteristicType_StatusActive,
    .debugDescription = kHAPCharacteristicDebugDescription_StatusActive,
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
    .callbacks = { .handleRead = HandleLeakSensorStatusActiveRead, .handleWrite = NULL }
};

/**
 * The Leak Sensor service that contains the following characteristics:
 * - 'Leak Detected'
 * - 'Status Active'
 */
const HAPService leakSensorService = {
    .iid = kIID_LeakSensor,
    .serviceType = &kHAPServiceType_LeakSensor,
    .debugDescription = kHAPServiceDebugDescription_LeakSensor,
    .name = "Leak Sensor",
    .properties = { .primaryService = true, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &leakSensorServiceSignatureCharacteristic,
                                                            &leakSensorNameCharacteristic,
                                                            &leakSensorLeakDetectedCharacteristic,
                                                            &leakSensorStatusActiveCharacteristic,
                                                            NULL }
};
#endif

#if (SENSOR_LIGHT == 1)
/**
 * The 'Service Signature' characteristic of the Light Sensor service.
 */
static const HAPDataCharacteristic lightSensorServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_LightSensorServiceSignature,
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
 * The 'Name' characteristic of the Light Sensor service.
 */
static const HAPStringCharacteristic lightSensorNameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_LightSensorName,
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
 * The 'Current Ambient Light Level' characteristic of the Light Sensor service.
 * Permissions: Paired Read, Notify
 */
const HAPFloatCharacteristic lightSensorCurrentAmbientLightLevelCharacteristic = {
    .format = kHAPCharacteristicFormat_Float,
    .iid = kIID_LightSensorCurrentAmbientLightLevel,
    .characteristicType = &kHAPCharacteristicType_CurrentAmbientLightLevel,
    .debugDescription = kHAPCharacteristicDebugDescription_CurrentAmbientLightLevel,
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
    .units = kHAPCharacteristicUnits_Lux,
    .constraints = { .minimumValue = 0.0001F, .maximumValue = 100000.0F },
    .callbacks = { .handleRead = HandleLightSensorCurrentAmbientLightLevelRead, .handleWrite = NULL }
};

/**
 * The 'Status Active' characteristic of the Light Sensor service.
 * Permissions: Paired Read, Notify
 */
const HAPBoolCharacteristic lightSensorStatusActiveCharacteristic = {
    .format = kHAPCharacteristicFormat_Bool,
    .iid = kIID_LightSensorStatusActive,
    .characteristicType = &kHAPCharacteristicType_StatusActive,
    .debugDescription = kHAPCharacteristicDebugDescription_StatusActive,
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
    .callbacks = { .handleRead = HandleLightSensorStatusActiveRead, .handleWrite = NULL }
};

/**
 * The Light Sensor service that contains the following characteristics:
 * - 'Current Ambient Light Level'
 * - 'Status Active'
 */
const HAPService lightSensorService = {
    .iid = kIID_LightSensor,
    .serviceType = &kHAPServiceType_LightSensor,
    .debugDescription = kHAPServiceDebugDescription_LightSensor,
    .name = "Light Sensor",
    .properties = { .primaryService = true, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &lightSensorServiceSignatureCharacteristic,
                                                            &lightSensorNameCharacteristic,
                                                            &lightSensorCurrentAmbientLightLevelCharacteristic,
                                                            &lightSensorStatusActiveCharacteristic,
                                                            NULL }
};
#endif

#if (SENSOR_OCCUPANCY == 1)
/**
 * The 'Service Signature' characteristic of the Occupancy Sensor service.
 */
static const HAPDataCharacteristic occupancySensorServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_OccupancySensorServiceSignature,
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
 * The 'Name' characteristic of the Occupancy Sensor service.
 */
static const HAPStringCharacteristic occupancySensorNameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_OccupancySensorName,
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
 * The 'Occupancy Detected' characteristic of the Occupancy Sensor service.
 * Permissions: Paired Read, Notify
 */
const HAPUInt8Characteristic occupancySensorOccupancyDetectedCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_OccupancySensorOccupancyDetected,
    .characteristicType = &kHAPCharacteristicType_OccupancyDetected,
    .debugDescription = kHAPCharacteristicDebugDescription_OccupancyDetected,
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
                     .maximumValue = 1,
                     .stepValue = 1,
                     .validValues = NULL,
                     .validValuesRanges = NULL },
    .callbacks = { .handleRead = HandleOccupancySensorOccupancyDetectedRead, .handleWrite = NULL }
};

/**
 * The 'Status Active' characteristic of the Occupancy Sensor service.
 * Permissions: Paired Read, Notify
 */
const HAPBoolCharacteristic occupancySensorStatusActiveCharacteristic = {
    .format = kHAPCharacteristicFormat_Bool,
    .iid = kIID_OccupancySensorStatusActive,
    .characteristicType = &kHAPCharacteristicType_StatusActive,
    .debugDescription = kHAPCharacteristicDebugDescription_StatusActive,
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
    .callbacks = { .handleRead = HandleOccupancySensorStatusActiveRead, .handleWrite = NULL }
};

/**
 * The Occupancy Sensor service that contains the following characteristics:
 * - 'Occupancy Detected'
 * - 'Status Active'
 */
const HAPService occupancySensorService = {
    .iid = kIID_OccupancySensor,
    .serviceType = &kHAPServiceType_OccupancySensor,
    .debugDescription = kHAPServiceDebugDescription_OccupancySensor,
    .name = "Occupancy Sensor",
    .properties = { .primaryService = true, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &occupancySensorServiceSignatureCharacteristic,
                                                            &occupancySensorNameCharacteristic,
                                                            &occupancySensorOccupancyDetectedCharacteristic,
                                                            &occupancySensorStatusActiveCharacteristic,
                                                            NULL }
};
#endif

#if (SENSOR_SMOKE == 1)
/**
 * The 'Service Signature' characteristic of the Smoke Sensor service.
 */
static const HAPDataCharacteristic smokeSensorServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_SmokeSensorServiceSignature,
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
 * The 'Name' characteristic of the Smoke Sensor service.
 */
static const HAPStringCharacteristic smokeSensorNameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_SmokeSensorName,
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
 * The 'Smoke Detected' characteristic of the Smoke Sensor service.
 * Permissions: Paired Read, Notify
 */
const HAPUInt8Characteristic smokeSensorSmokeDetectedCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_SmokeSensorSmokeDetected,
    .characteristicType = &kHAPCharacteristicType_SmokeDetected,
    .debugDescription = kHAPCharacteristicDebugDescription_SmokeDetected,
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
                     .maximumValue = 1,
                     .stepValue = 1,
                     .validValues = NULL,
                     .validValuesRanges = NULL },
    .callbacks = { .handleRead = HandleSmokeSensorSmokeDetectedRead, .handleWrite = NULL }
};

/**
 * The 'Status Active' characteristic of the Smoke Sensor service.
 * Permissions: Paired Read, Notify
 */
const HAPBoolCharacteristic smokeSensorStatusActiveCharacteristic = {
    .format = kHAPCharacteristicFormat_Bool,
    .iid = kIID_SmokeSensorStatusActive,
    .characteristicType = &kHAPCharacteristicType_StatusActive,
    .debugDescription = kHAPCharacteristicDebugDescription_StatusActive,
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
    .callbacks = { .handleRead = HandleSmokeSensorStatusActiveRead, .handleWrite = NULL }
};

/**
 * The Smoke Sensor service that contains the following characteristics:
 * - 'Smoke Detected'
 * - 'Status Active'
 */
const HAPService smokeSensorService = {
    .iid = kIID_SmokeSensor,
    .serviceType = &kHAPServiceType_SmokeSensor,
    .debugDescription = kHAPServiceDebugDescription_SmokeSensor,
    .name = "Smoke Sensor",
    .properties = { .primaryService = true, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &smokeSensorServiceSignatureCharacteristic,
                                                            &smokeSensorNameCharacteristic,
                                                            &smokeSensorSmokeDetectedCharacteristic,
                                                            &smokeSensorStatusActiveCharacteristic,
                                                            NULL }
};
#endif

#if (SENSOR_TEMPERATURE == 1)
/**
 * The 'Service Signature' characteristic of the Temperature Sensor service.
 */
static const HAPDataCharacteristic temperatureSensorServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_TemperatureSensorServiceSignature,
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
 * The 'Name' characteristic of the Temperature Sensor service.
 */
static const HAPStringCharacteristic temperatureSensorNameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_TemperatureSensorName,
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
 * The 'Current Temperature' characteristic of the Temperature Sensor service.
 * Permissions: Paired Read, Notify
 */
const HAPFloatCharacteristic temperatureSensorCurrentTemperatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Float,
    .iid = kIID_TemperatureSensorCurrentTemperature,
    .characteristicType = &kHAPCharacteristicType_CurrentTemperature,
    .debugDescription = kHAPCharacteristicDebugDescription_CurrentTemperature,
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
    .units = kHAPCharacteristicUnits_Celsius,
    .constraints = { .minimumValue = 0.0F, .maximumValue = 100.0F, .stepValue = 0.1F },
    .callbacks = { .handleRead = HandleTemperatureSensorCurrentTemperatureRead, .handleWrite = NULL }
};

/**
 * The 'Status Active' characteristic of the Temperature Sensor service.
 * Permissions: Paired Read, Notify
 */
const HAPBoolCharacteristic temperatureSensorStatusActiveCharacteristic = {
    .format = kHAPCharacteristicFormat_Bool,
    .iid = kIID_TemperatureSensorStatusActive,
    .characteristicType = &kHAPCharacteristicType_StatusActive,
    .debugDescription = kHAPCharacteristicDebugDescription_StatusActive,
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
    .callbacks = { .handleRead = HandleTemperatureSensorStatusActiveRead, .handleWrite = NULL }
};

/**
 * The Temperature Sensor service that contains the following characteristics:
 * - 'Current Temperature'
 * - 'Status Active'
 */
const HAPService temperatureSensorService = {
    .iid = kIID_TemperatureSensor,
    .serviceType = &kHAPServiceType_TemperatureSensor,
    .debugDescription = kHAPServiceDebugDescription_TemperatureSensor,
    .name = "Temperature Sensor",
    .properties = { .primaryService = true, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &temperatureSensorServiceSignatureCharacteristic,
                                                            &temperatureSensorNameCharacteristic,
                                                            &temperatureSensorCurrentTemperatureCharacteristic,
                                                            &temperatureSensorStatusActiveCharacteristic,
                                                            NULL }
};
#endif
