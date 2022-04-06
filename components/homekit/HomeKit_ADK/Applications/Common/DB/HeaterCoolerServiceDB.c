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

#include "HeaterCoolerServiceDB.h"

//----------------------------------------------------------------------------------------------------------------------
/**
 * Handle read request on the 'Active' characteristic of the Heater Cooler service
 */
HAP_RESULT_USE_CHECK
HAPError HandleHeaterCoolerActiveRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED);

/**
 * Handle write request on the 'Active' characteristic of the Heater Cooler service
 */
HAP_RESULT_USE_CHECK
HAPError HandleHeaterCoolerActiveWrite(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value,
        void* _Nullable context HAP_UNUSED);

/**
 * Handle read request on the 'Current Temperature' characteristic of the Heater Cooler service
 */
HAP_RESULT_USE_CHECK
HAPError HandleHeaterCoolerCurrentTemperatureRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPFloatCharacteristicReadRequest* request,
        float* value,
        void* _Nullable context);

/**
 * Handle read request on the 'Current State' characteristic of the Heater Cooler service
 */
HAP_RESULT_USE_CHECK
HAPError HandleHeaterCoolerCurrentStateRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED);

/**
 * Handle read request on the 'Target State' characteristic of the Heater Cooler service
 */
HAP_RESULT_USE_CHECK
HAPError HandleHeaterCoolerTargetStateRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED);

/**
 * Handle write request on the 'Target State' characteristic of the Heater Cooler service
 */
HAP_RESULT_USE_CHECK
HAPError HandleHeaterCoolerTargetStateWrite(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value,
        void* _Nullable context HAP_UNUSED);

/**
 * Handle read request on the 'Rotation Speed' characteristic of the Heater Cooler service
 */
HAP_RESULT_USE_CHECK
HAPError HandleHeaterCoolerRotationSpeedRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPFloatCharacteristicReadRequest* request,
        float* value,
        void* _Nullable context);

/**
 * Handle write request on the 'Rotation Speed' characteristic of the Heater Cooler service
 */
HAP_RESULT_USE_CHECK
HAPError HandleHeaterCoolerRotationSpeedWrite(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPFloatCharacteristicWriteRequest* request,
        float value,
        void* _Nullable context);

/**
 * Handle read request on the 'Temperature Display Units' characteristic of the Heater Cooler service
 */
HAP_RESULT_USE_CHECK
HAPError HandleHeaterCoolerTemperatureDisplayUnitsRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED);

/**
 * Handle write request on the 'Temperature Display Units' characteristic of the Heater Cooler service
 */
HAP_RESULT_USE_CHECK
HAPError HandleHeaterCoolerTemperatureDisplayUnitsWrite(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value,
        void* _Nullable context HAP_UNUSED);

/**
 * Handle read request on the 'Swing Mode' characteristic of the Heater Cooler service
 */
HAP_RESULT_USE_CHECK
HAPError HandleHeaterCoolerSwingModeRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED);

/**
 * Handle write request on the 'Swing Mode' characteristic of the Heater Cooler service
 */
HAP_RESULT_USE_CHECK
HAPError HandleHeaterCoolerSwingModeWrite(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value,
        void* _Nullable context HAP_UNUSED);

/**
 * Handle read request on the 'Heating Threshold Temperature' characteristic of the Heater Cooler service
 */
HAP_RESULT_USE_CHECK
HAPError HandleHeaterCoolerHeatingThresholdTemperatureRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPFloatCharacteristicReadRequest* request,
        float* value,
        void* _Nullable context);

/**
 * Handle write request on the 'Heating Threshold Temperature' characteristic of the Heater Cooler service
 */
HAP_RESULT_USE_CHECK
HAPError HandleHeaterCoolerHeatingThresholdTemperatureWrite(
        HAPAccessoryServer* server,
        const HAPFloatCharacteristicWriteRequest* request,
        float value,
        void* _Nullable context);

/**
 * Handle read request on the 'Cooling Threshold Temperature' characteristic of the Heater Cooler service
 */
HAP_RESULT_USE_CHECK
HAPError HandleHeaterCoolerCoolingThresholdTemperatureRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPFloatCharacteristicReadRequest* request,
        float* value,
        void* _Nullable context);

/**
 * Handle write request on the 'Cooling Threshold Temperature' characteristic of the Heater Cooler service
 */
HAP_RESULT_USE_CHECK
HAPError HandleHeaterCoolerCoolingThresholdTemperatureWrite(
        HAPAccessoryServer* server,
        const HAPFloatCharacteristicWriteRequest* request,
        float value,
        void* _Nullable context);

/**
 * Handle read request on the 'Lock Physical Controls' characteristic of the Heater Cooler service
 */
HAP_RESULT_USE_CHECK
HAPError HandleHeaterCoolerLockPhysicalControlsRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED);

/**
 * Handle write request on the 'Lock Physical Controls' characteristic of the Heater Cooler service
 */
HAP_RESULT_USE_CHECK
HAPError HandleHeaterCoolerLockPhysicalControlsWrite(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value,
        void* _Nullable context HAP_UNUSED);

/**
 * The 'Name' characteristic of the HeaterCooler service.
 */
static const HAPStringCharacteristic heaterCoolerNameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_HeaterCoolerName,
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
 * The 'Service Signature' characteristic of the HeaterCooler service.
 */
static const HAPDataCharacteristic heaterCoolerServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_HeaterCoolerServiceSignature,
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
    .callbacks = { .handleRead = HAPHandleServiceSignatureRead }
};

/**
 * The 'Active' characteristic of the HeaterCooler service.
 */
const HAPUInt8Characteristic heaterCoolerActiveCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_HeaterCoolerActive,
    .characteristicType = &kHAPCharacteristicType_Active,
    .debugDescription = kHAPCharacteristicDebugDescription_Active,
    .manufacturerDescription = NULL,
    .properties = {
        .readable = true,
        .writable = true,
        .supportsEventNotification = true,
        .hidden = false,
        .readRequiresAdminPermissions = false,
        .writeRequiresAdminPermissions = true,
        .requiresTimedWrite = false,
        .supportsAuthorizationData = false,
        .ip = {
            .controlPoint = false,
            .supportsWriteResponse = false
        },
        .ble = {
            .supportsBroadcastNotification = false,
            .supportsDisconnectedNotification = false,
            .readableWithoutSecurity = false,
            .writableWithoutSecurity = false
        }
    },
    .constraints = {
        .minimumValue = 0,
        .maximumValue = 1,
        .stepValue = 1,
    },
    .callbacks = {
        .handleRead = HandleHeaterCoolerActiveRead,
        .handleWrite = HandleHeaterCoolerActiveWrite,
        .handleSubscribe = NULL,
        .handleUnsubscribe = NULL
    }
};

/**
 * The 'Current Temperature' characteristic of the HeaterCooler service.
 */
const HAPFloatCharacteristic heaterCoolerCurrentTemperatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Float,
    .iid = kIID_HeaterCoolerCurrentTemperature,
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
    .callbacks = { .handleRead = HandleHeaterCoolerCurrentTemperatureRead }
};

/**
 * The 'Current Heater Cooler State' characteristic of the HeaterCooler service.
 */
const HAPUInt8Characteristic heaterCoolerCurrentStateCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_HeaterCoolerCurrentState,
    .characteristicType = &kHAPCharacteristicType_CurrentHeaterCoolerState,
    .debugDescription = kHAPCharacteristicDebugDescription_CurrentHeatingCoolingState,
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
    .constraints = { .minimumValue = 0, .maximumValue = 3, .stepValue = 1 },
    .callbacks = { .handleRead = HandleHeaterCoolerCurrentStateRead }
};

/**
 * The 'Target Heater Cooler State' characteristic of the HeaterCooler service.
 */
const HAPUInt8Characteristic heaterCoolerTargetStateCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_HeaterCoolerTargetState,
    .characteristicType = &kHAPCharacteristicType_TargetHeaterCoolerState,
    .debugDescription = kHAPCharacteristicDebugDescription_TargetHeaterCoolerState,
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
    .constraints = { .minimumValue = 0, .maximumValue = 2, .stepValue = 1 },
    .callbacks = { .handleRead = HandleHeaterCoolerTargetStateRead, .handleWrite = HandleHeaterCoolerTargetStateWrite }
};

/**
 * The 'Rotation Speed' characteristic of the HeaterCooler service.
 */
const HAPFloatCharacteristic heaterCoolerRotationSpeedCharacteristic = {
    .format = kHAPCharacteristicFormat_Float,
    .iid = kIID_HeaterCoolerRotationSpeed,
    .characteristicType = &kHAPCharacteristicType_RotationSpeed,
    .debugDescription = kHAPCharacteristicDebugDescription_RotationSpeed,
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
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .minimumValue = 0.0F, .maximumValue = 100.0F, .stepValue = 1.0F },
    .callbacks = { .handleRead = HandleHeaterCoolerRotationSpeedRead,
                   .handleWrite = HandleHeaterCoolerRotationSpeedWrite }
};

/**
 * The 'Temperature Display Units' characteristic of the HeaterCooler service.
 */
const HAPUInt8Characteristic heaterCoolerTemperatureDisplayUnitsCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_HeaterCoolerTemperatureDisplayUnits,
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
        .handleRead = HandleHeaterCoolerTemperatureDisplayUnitsRead,
        .handleWrite = HandleHeaterCoolerTemperatureDisplayUnitsWrite
    }
};

/**
 * The 'Swing Mode' characteristic of the HeaterCooler service.
 */
const HAPUInt8Characteristic heaterCoolerSwingModeCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_HeaterCoolerSwingMode,
    .characteristicType = &kHAPCharacteristicType_SwingMode,
    .debugDescription = kHAPCharacteristicDebugDescription_SwingMode,
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
    .callbacks = { .handleRead = HandleHeaterCoolerSwingModeRead, .handleWrite = HandleHeaterCoolerSwingModeWrite }
};

/**
 * The 'Heating Threshold Temperature' characteristic of the HeaterCooler service.
 */
const HAPFloatCharacteristic heaterCoolerHeatingThresholdTemperatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Float,
    .iid = kIID_HeaterCoolerHeatingThresholdTemperature,
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
    .callbacks = { .handleRead = HandleHeaterCoolerHeatingThresholdTemperatureRead,
                   .handleWrite = HandleHeaterCoolerHeatingThresholdTemperatureWrite }
};

/**
 * The 'Cooling Threshold Temperature' characteristic of the HeaterCooler service.
 */
const HAPFloatCharacteristic heaterCoolerCoolingThresholdTemperatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Float,
    .iid = kIID_HeaterCoolerCoolingThresholdTemperature,
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
    .callbacks = { .handleRead = HandleHeaterCoolerCoolingThresholdTemperatureRead,
                   .handleWrite = HandleHeaterCoolerCoolingThresholdTemperatureWrite }
};

/**
 * The 'Lock Physical Controls' characteristic of the HeaterCooler service.
 */
const HAPUInt8Characteristic heaterCoolerLockPhysicalControlsCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_HeaterCoolerLockPhysicalControls,
    .characteristicType = &kHAPCharacteristicType_LockPhysicalControls,
    .debugDescription = kHAPCharacteristicDebugDescription_LockPhysicalControls,
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
    .callbacks = { .handleRead = HandleHeaterCoolerLockPhysicalControlsRead,
                   .handleWrite = HandleHeaterCoolerLockPhysicalControlsWrite }
};

const HAPService heaterCoolerService = {
    .iid = kIID_HeaterCooler,
    .serviceType = &kHAPServiceType_HeaterCooler,
    .debugDescription = kHAPServiceDebugDescription_HeaterCooler,
    .name = "HeaterCooler",
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &heaterCoolerNameCharacteristic,
                                                            &heaterCoolerServiceSignatureCharacteristic,
                                                            &heaterCoolerActiveCharacteristic,
                                                            &heaterCoolerCurrentTemperatureCharacteristic,
                                                            &heaterCoolerCurrentStateCharacteristic,
                                                            &heaterCoolerTargetStateCharacteristic,
                                                            &heaterCoolerRotationSpeedCharacteristic,
                                                            &heaterCoolerTemperatureDisplayUnitsCharacteristic,
                                                            &heaterCoolerSwingModeCharacteristic,
                                                            &heaterCoolerHeatingThresholdTemperatureCharacteristic,
                                                            &heaterCoolerCoolingThresholdTemperatureCharacteristic,
                                                            &heaterCoolerLockPhysicalControlsCharacteristic,
                                                            NULL }
};
