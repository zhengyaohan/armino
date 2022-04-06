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
// Information Service, the Pairing service and finally the service signature exposed by the thermostat.

#include "DB.h"
#include "App.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------------------------------------------------------------

/**
 * The 'Service Signature' characteristic of the Thermostat service.
 */
static const HAPDataCharacteristic thermostatServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_ThermostatServiceSignature,
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
 * The 'Name' characteristic of the Thermostat service.
 */
static const HAPStringCharacteristic thermostatNameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_ThermostatName,
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
 * Characteristic for "Current Temperature"
 * Permissions: Paired Read, Notify
 */
const HAPFloatCharacteristic thermostatTemperatureCurrent = {
    .format = kHAPCharacteristicFormat_Float,
    .iid = kIID_ThermostatCurrentTemperature,
    .characteristicType = &kHAPCharacteristicType_CurrentTemperature,
    .debugDescription = kHAPCharacteristicDebugDescription_CurrentTemperature,
    .manufacturerDescription = NULL,
    .units = kHAPCharacteristicUnits_Celsius,
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
    .constraints = { .minimumValue = 0.0F, .maximumValue = 100.0F, .stepValue = 0.1F },
    .callbacks = { .handleRead = HandleThermostatCurrentTemperatureRead, .handleWrite = NULL }
};

/**
 * Characteristic for "Target Temperature"
 * Permissions: Paired Read, Paired Write, Notify
 */
const HAPFloatCharacteristic thermostatTemperatureTarget = {
    .format = kHAPCharacteristicFormat_Float,
    .iid = kIID_ThermostatTargetTemperature,
    .characteristicType = &kHAPCharacteristicType_TargetTemperature,
    .debugDescription = kHAPCharacteristicDebugDescription_TargetTemperature,
    .manufacturerDescription = NULL,
    .units = kHAPCharacteristicUnits_Celsius,
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
    .constraints = { .minimumValue = 10.0F, .maximumValue = 38.0F, .stepValue = 0.1F },
    .callbacks = { .handleRead = HandleThermostatTargetTemperatureRead,
                   .handleWrite = HandleThermostatTargetTemperatureWrite }
};

/**
 * Characteristic for "Temperature Display Units"
 * Permissions: Paired Read, Paired Write, Notify
 */
const HAPUInt8Characteristic thermostatTemperatureUnits = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_ThermostatTemperatureDisplayUnits,
    .characteristicType = &kHAPCharacteristicType_TemperatureDisplayUnits,
    .debugDescription = kHAPCharacteristicDebugDescription_TemperatureDisplayUnits,
    .manufacturerDescription = NULL,
    .properties = {
        .readable = true,
        .writable = true,
        .supportsEventNotification = true,
        .hidden = false,
        .requiresTimedWrite = false,
        .supportsAuthorizationData = false,
        .ip = {
            .controlPoint = false,
            .supportsWriteResponse = false
        },
        .ble = {
            .supportsBroadcastNotification = true,
            .supportsDisconnectedNotification = true,
            .readableWithoutSecurity = false,
            .writableWithoutSecurity = false
        }
    },
    .constraints = {
        .minimumValue = 0,
        .maximumValue = 1,
        .stepValue = 1,
        .validValues = (const uint8_t*[]) { // Optional
            &(const uint8_t){kHAPCharacteristicValue_TemperatureDisplayUnits_Celsius},
            &(const uint8_t){kHAPCharacteristicValue_TemperatureDisplayUnits_Fahrenheit},
            NULL
        },
        .validValuesRanges = NULL,
    },
    .callbacks = {
        .handleRead = HandleThermostatTemperatureDisplayUnitsRead,
        .handleWrite = HandleThermostatTemperatureDisplayUnitsWrite
    }
};

/**
 * Characteristic for "Current Heating Cooling State"
 * Permissions: Paired Read, Notify
 */
const HAPUInt8Characteristic thermostatHeatingCoolingCurrent = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_ThermostatCurrentHeatingCoolingState,
    .characteristicType = &kHAPCharacteristicType_CurrentHeatingCoolingState,
    .debugDescription = kHAPCharacteristicDebugDescription_CurrentHeatingCoolingState,
    .manufacturerDescription = NULL,
    .properties = {
        .readable = true,
        .writable = false,
        .supportsEventNotification = true,
        .hidden = false,
        .requiresTimedWrite = false,
        .supportsAuthorizationData = false,
        .ip = {
            .controlPoint = false,
            .supportsWriteResponse = false
        },
        .ble = {
            .supportsBroadcastNotification = true,
            .supportsDisconnectedNotification = true,
            .readableWithoutSecurity = false,
            .writableWithoutSecurity = false
        }
    },
    .constraints = {
        .minimumValue = 0,
        .maximumValue = 2,
        .stepValue = 1,
        .validValues = (const uint8_t*[]) { // Optional
            &(const uint8_t){kHAPCharacteristicValue_CurrentHeatingCoolingState_Off},
            &(const uint8_t){kHAPCharacteristicValue_CurrentHeatingCoolingState_Heat},
            &(const uint8_t){kHAPCharacteristicValue_CurrentHeatingCoolingState_Cool},
            NULL
        },
        .validValuesRanges = NULL,
    },
    .callbacks = {
        .handleRead = HandleThermostatCurrentHeatingCoolingStateRead,
        .handleWrite = NULL
    }
};

/**
 * Characteristic for "Target Heating Cooling State"
 * Permissions: Paired Read, Paired Write, Notify
 */
const HAPUInt8Characteristic thermostatHeatingCoolingTarget = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_ThermostatTargetHeatingCoolingState,
    .characteristicType = &kHAPCharacteristicType_TargetHeatingCoolingState,
    .debugDescription = kHAPCharacteristicDebugDescription_TargetHeatingCoolingState,
    .manufacturerDescription = NULL,
    .properties = {
        .readable = true,
        .writable = true,
        .supportsEventNotification = true,
        .hidden = false,
        .requiresTimedWrite = false,
        .supportsAuthorizationData = false,
        .ip = {
            .controlPoint = false,
            .supportsWriteResponse = false
        },
        .ble = {
            .supportsBroadcastNotification = true,
            .supportsDisconnectedNotification = true,
            .readableWithoutSecurity = false,
            .writableWithoutSecurity = false
        }
    },
    .constraints = {
        .minimumValue = 0,
        .maximumValue = 3,
        .stepValue = 1,
        .validValues = (const uint8_t*[]) { // Optional
            &(const uint8_t){kHAPCharacteristicValue_TargetHeatingCoolingState_Off},
            &(const uint8_t){kHAPCharacteristicValue_TargetHeatingCoolingState_Heat},
            &(const uint8_t){kHAPCharacteristicValue_TargetHeatingCoolingState_Cool},
            &(const uint8_t){kHAPCharacteristicValue_TargetHeatingCoolingState_Auto},
            NULL
        },
        .validValuesRanges = NULL,
    },
    .callbacks = {
        .handleRead = HandleThermostatTargetHeatingCoolingStateRead,
        .handleWrite = HandleThermostatTargetHeatingCoolingStateWrite
    }
};

/**
 * Characteristic for "Heating Threshold Temperature"
 * Permissions: Paired Read, Paired Write, Notify
 */
const HAPFloatCharacteristic thermostatTemperatureHeatingThreshold = {
    .format = kHAPCharacteristicFormat_Float,
    .iid = kIID_ThermostatHeatingThresholdTemperature,
    .characteristicType = &kHAPCharacteristicType_HeatingThresholdTemperature,
    .debugDescription = kHAPCharacteristicDebugDescription_HeatingThresholdTemperature,
    .manufacturerDescription = NULL,
    .units = kHAPCharacteristicUnits_Celsius,
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
    .constraints = { .minimumValue = 0.0F, .maximumValue = 25.0F, .stepValue = 0.1F },
    .callbacks = { .handleRead = HandleThermostatHeatingThresholdTemperatureRead,
                   .handleWrite = HandleThermostatHeatingThresholdTemperatureWrite }
};

/**
 * Characteristic for "Cooling Threshold Temperature"
 * Permissions: Paired Read, Paired Write, Notify
 */
const HAPFloatCharacteristic thermostatTemperatureCoolingThreshold = {
    .format = kHAPCharacteristicFormat_Float,
    .iid = kIID_ThermostatCoolingThresholdTemperature,
    .characteristicType = &kHAPCharacteristicType_CoolingThresholdTemperature,
    .debugDescription = kHAPCharacteristicDebugDescription_CoolingThresholdTemperature,
    .manufacturerDescription = NULL,
    .units = kHAPCharacteristicUnits_Celsius,
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
    .constraints = { .minimumValue = 10.0F, .maximumValue = 35.0F, .stepValue = 0.1F },
    .callbacks = { .handleRead = HandleThermostatCoolingThresholdTemperatureRead,
                   .handleWrite = HandleThermostatCoolingThresholdTemperatureWrite }
};

/**
 * Characteristic for "Current Relative Humidity"
 * Permissions: Paired Read, Notify
 */
const HAPFloatCharacteristic thermostatRelativeHumidityCurrent = {
    .format = kHAPCharacteristicFormat_Float,
    .iid = kIID_ThermostatCurrentRelativeHumidity,
    .characteristicType = &kHAPCharacteristicType_CurrentRelativeHumidity,
    .debugDescription = kHAPCharacteristicDebugDescription_CurrentRelativeHumidity,
    .manufacturerDescription = NULL,
    .units = kHAPCharacteristicUnits_Percentage,
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
    .constraints = { .minimumValue = 0.0F, .maximumValue = 100.0F, .stepValue = 1.0F },
    .callbacks = { .handleRead = HandleThermostatCurrentRelativeHumidityRead, .handleWrite = NULL }
};

/**
 * Characteristic for "Target Relative Humidity"
 * Permissions: Paired Read, Paired Write, Notify
 */
const HAPFloatCharacteristic thermostatRelativeHumidityTarget = {
    .format = kHAPCharacteristicFormat_Float,
    .iid = kIID_ThermostatTargetRelativeHumidity,
    .characteristicType = &kHAPCharacteristicType_TargetRelativeHumidity,
    .debugDescription = kHAPCharacteristicDebugDescription_TargetRelativeHumidity,
    .manufacturerDescription = NULL,
    .units = kHAPCharacteristicUnits_Percentage,
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
    .constraints = { .minimumValue = 0.0F, .maximumValue = 100.0F, .stepValue = 1.0F },
    .callbacks = { .handleRead = HandleThermostatTargetRelativeHumidityRead,
                   .handleWrite = HandleThermostatTargetRelativeHumidityWrite }
};

/**
 * The Thermostat service that contains the 'On' characteristic.
 */
const HAPService thermostatService = {
    .iid = kIID_Thermostat,
    .serviceType = &kHAPServiceType_Thermostat,
    .debugDescription = kHAPServiceDebugDescription_Thermostat,
    .name = "Thermostat",
    .properties = { .primaryService = true, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = (uint16_t const[]) { kIID_FilterMaintenance, 0 },
    .characteristics = (const HAPCharacteristic* const[]) { &thermostatServiceSignatureCharacteristic,
                                                            &thermostatNameCharacteristic,
                                                            &thermostatTemperatureCurrent,
                                                            &thermostatTemperatureTarget,
                                                            &thermostatTemperatureUnits,
                                                            &thermostatHeatingCoolingCurrent,
                                                            &thermostatHeatingCoolingTarget,
                                                            &thermostatTemperatureHeatingThreshold,
                                                            &thermostatTemperatureCoolingThreshold,
                                                            &thermostatRelativeHumidityCurrent,
                                                            &thermostatRelativeHumidityTarget,
                                                            NULL }
};

//----------------------------------------------------------------------------------------------------------------------
/**
 * The 'Service Signature' characteristic of the Filter Maintenance service.
 */
static const HAPDataCharacteristic filterMaintenanceServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_FilterMaintenanceServiceSignature,
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
 * The 'Name' characteristic of the Filter Maintenance service.
 */
static const HAPStringCharacteristic filterMaintenanceNameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_FilterMaintenanceName,
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
 * The 'Filter Change Indication' characteristic of the Filter Maintenance service.
 * Permissions: Paired Read, Notify
 */
const HAPUInt8Characteristic filterMaintenanceFilterChangeIndicationCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_FilterMaintenanceFilterChangeIndication,
    .characteristicType = &kHAPCharacteristicType_FilterChangeIndication,
    .debugDescription = kHAPCharacteristicDebugDescription_FilterChangeIndication,
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
    .callbacks = { .handleRead = HandleFilterMaintenanceFilterChangeIndicationRead, .handleWrite = NULL }
};

/**
 * The Filter Maintenance service that contains the following characteristics:
 * - 'Filter Change Indication'
 */
const HAPService filterMaintenanceService = {
    .iid = kIID_FilterMaintenance,
    .serviceType = &kHAPServiceType_FilterMaintenance,
    .debugDescription = kHAPServiceDebugDescription_FilterMaintenance,
    .name = "Filter Maintenance",
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &filterMaintenanceServiceSignatureCharacteristic,
                                                            &filterMaintenanceNameCharacteristic,
                                                            &filterMaintenanceFilterChangeIndicationCharacteristic,
                                                            NULL }
};
