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
// Copyright (C) 2015-2021 Apple Inc. All Rights Reserved.

#include "HAPCharacteristicTypes.h"
#include "HAPCharacteristicTypes+TLV.h"
#include "HAPLogSubsystem.h"
#include "HAPTLV+Internal.h"
#include "HAPUUID.h"
#include "HAPWACEngine.h"
#include "HAPWiFiRouter.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "CharacteristicTypes" };

const HAPUUID kHAPCharacteristicType_Brightness = HAPUUIDCreateAppleDefined(0x8U);

const HAPUUID kHAPCharacteristicType_CoolingThresholdTemperature = HAPUUIDCreateAppleDefined(0xDU);

const HAPUUID kHAPCharacteristicType_CurrentDoorState = HAPUUIDCreateAppleDefined(0xEU);

const HAPUUID kHAPCharacteristicType_CurrentHeatingCoolingState = HAPUUIDCreateAppleDefined(0xFU);

const HAPUUID kHAPCharacteristicType_CurrentRelativeHumidity = HAPUUIDCreateAppleDefined(0x10U);

const HAPUUID kHAPCharacteristicType_CurrentTemperature = HAPUUIDCreateAppleDefined(0x11U);

const HAPUUID kHAPCharacteristicType_HeatingThresholdTemperature = HAPUUIDCreateAppleDefined(0x12U);

const HAPUUID kHAPCharacteristicType_Hue = HAPUUIDCreateAppleDefined(0x13U);

const HAPUUID kHAPCharacteristicType_Identify = HAPUUIDCreateAppleDefined(0x14U);

const HAPUUID kHAPCharacteristicType_LockCurrentState = HAPUUIDCreateAppleDefined(0x1DU);

const HAPUUID kHAPCharacteristicType_LockTargetState = HAPUUIDCreateAppleDefined(0x1EU);

const HAPUUID kHAPCharacteristicType_Manufacturer = HAPUUIDCreateAppleDefined(0x20U);

const HAPUUID kHAPCharacteristicType_Model = HAPUUIDCreateAppleDefined(0x21U);

const HAPUUID kHAPCharacteristicType_MotionDetected = HAPUUIDCreateAppleDefined(0x22U);

const HAPUUID kHAPCharacteristicType_Name = HAPUUIDCreateAppleDefined(0x23U);

const HAPUUID kHAPCharacteristicType_ObstructionDetected = HAPUUIDCreateAppleDefined(0x24U);

const HAPUUID kHAPCharacteristicType_On = HAPUUIDCreateAppleDefined(0x25U);

const HAPUUID kHAPCharacteristicType_OutletInUse = HAPUUIDCreateAppleDefined(0x26U);

const HAPUUID kHAPCharacteristicType_RotationDirection = HAPUUIDCreateAppleDefined(0x28U);

const HAPUUID kHAPCharacteristicType_RotationSpeed = HAPUUIDCreateAppleDefined(0x29U);

const HAPUUID kHAPCharacteristicType_Saturation = HAPUUIDCreateAppleDefined(0x2FU);

const HAPUUID kHAPCharacteristicType_SerialNumber = HAPUUIDCreateAppleDefined(0x30U);

const HAPUUID kHAPCharacteristicType_TargetDoorState = HAPUUIDCreateAppleDefined(0x32U);

const HAPUUID kHAPCharacteristicType_TargetHeatingCoolingState = HAPUUIDCreateAppleDefined(0x33U);

const HAPUUID kHAPCharacteristicType_TargetRelativeHumidity = HAPUUIDCreateAppleDefined(0x34U);

const HAPUUID kHAPCharacteristicType_TargetTemperature = HAPUUIDCreateAppleDefined(0x35U);

const HAPUUID kHAPCharacteristicType_TemperatureDisplayUnits = HAPUUIDCreateAppleDefined(0x36U);

const HAPUUID kHAPCharacteristicType_Version = HAPUUIDCreateAppleDefined(0x37U);

const HAPUUID kHAPCharacteristicType_PairSetup = HAPUUIDCreateAppleDefined(0x4CU);

const HAPUUID kHAPCharacteristicType_PairVerify = HAPUUIDCreateAppleDefined(0x4EU);

const HAPUUID kHAPCharacteristicType_PairingFeatures = HAPUUIDCreateAppleDefined(0x4FU);

const HAPUUID kHAPCharacteristicType_PairingPairings = HAPUUIDCreateAppleDefined(0x50U);

const HAPUUID kHAPCharacteristicType_FirmwareRevision = HAPUUIDCreateAppleDefined(0x52U);

const HAPUUID kHAPCharacteristicType_HardwareRevision = HAPUUIDCreateAppleDefined(0x53U);

const HAPUUID kHAPCharacteristicType_AirParticulateDensity = HAPUUIDCreateAppleDefined(0x64U);

const HAPUUID kHAPCharacteristicType_AirParticulateSize = HAPUUIDCreateAppleDefined(0x65U);

const HAPUUID kHAPCharacteristicType_SecuritySystemCurrentState = HAPUUIDCreateAppleDefined(0x66U);

const HAPUUID kHAPCharacteristicType_SecuritySystemTargetState = HAPUUIDCreateAppleDefined(0x67U);

const HAPUUID kHAPCharacteristicType_BatteryLevel = HAPUUIDCreateAppleDefined(0x68U);

const HAPUUID kHAPCharacteristicType_CarbonMonoxideDetected = HAPUUIDCreateAppleDefined(0x69U);

const HAPUUID kHAPCharacteristicType_ContactSensorState = HAPUUIDCreateAppleDefined(0x6AU);

const HAPUUID kHAPCharacteristicType_CurrentAmbientLightLevel = HAPUUIDCreateAppleDefined(0x6BU);

const HAPUUID kHAPCharacteristicType_CurrentHorizontalTiltAngle = HAPUUIDCreateAppleDefined(0x6CU);

const HAPUUID kHAPCharacteristicType_CurrentPosition = HAPUUIDCreateAppleDefined(0x6DU);

const HAPUUID kHAPCharacteristicType_CurrentVerticalTiltAngle = HAPUUIDCreateAppleDefined(0x6EU);

const HAPUUID kHAPCharacteristicType_HoldPosition = HAPUUIDCreateAppleDefined(0x6FU);

const HAPUUID kHAPCharacteristicType_LeakDetected = HAPUUIDCreateAppleDefined(0x70U);

const HAPUUID kHAPCharacteristicType_OccupancyDetected = HAPUUIDCreateAppleDefined(0x71U);

const HAPUUID kHAPCharacteristicType_PositionState = HAPUUIDCreateAppleDefined(0x72U);

const HAPUUID kHAPCharacteristicType_ProgrammableSwitchEvent = HAPUUIDCreateAppleDefined(0x73U);

const HAPUUID kHAPCharacteristicType_StatusActive = HAPUUIDCreateAppleDefined(0x75U);

const HAPUUID kHAPCharacteristicType_SmokeDetected = HAPUUIDCreateAppleDefined(0x76U);

const HAPUUID kHAPCharacteristicType_StatusFault = HAPUUIDCreateAppleDefined(0x77U);

const HAPUUID kHAPCharacteristicType_StatusJammed = HAPUUIDCreateAppleDefined(0x78U);

const HAPUUID kHAPCharacteristicType_StatusLowBattery = HAPUUIDCreateAppleDefined(0x79U);

const HAPUUID kHAPCharacteristicType_StatusTampered = HAPUUIDCreateAppleDefined(0x7AU);

const HAPUUID kHAPCharacteristicType_TargetHorizontalTiltAngle = HAPUUIDCreateAppleDefined(0x7BU);

const HAPUUID kHAPCharacteristicType_TargetPosition = HAPUUIDCreateAppleDefined(0x7CU);

const HAPUUID kHAPCharacteristicType_TargetVerticalTiltAngle = HAPUUIDCreateAppleDefined(0x7DU);

const HAPUUID kHAPCharacteristicType_SecuritySystemAlarmType = HAPUUIDCreateAppleDefined(0x8EU);

const HAPUUID kHAPCharacteristicType_ChargingState = HAPUUIDCreateAppleDefined(0x8FU);

const HAPUUID kHAPCharacteristicType_CarbonMonoxideLevel = HAPUUIDCreateAppleDefined(0x90U);

const HAPUUID kHAPCharacteristicType_CarbonMonoxidePeakLevel = HAPUUIDCreateAppleDefined(0x91U);

const HAPUUID kHAPCharacteristicType_CarbonDioxideDetected = HAPUUIDCreateAppleDefined(0x92U);

const HAPUUID kHAPCharacteristicType_CarbonDioxideLevel = HAPUUIDCreateAppleDefined(0x93U);

const HAPUUID kHAPCharacteristicType_CarbonDioxidePeakLevel = HAPUUIDCreateAppleDefined(0x94U);

const HAPUUID kHAPCharacteristicType_AirQuality = HAPUUIDCreateAppleDefined(0x95U);

const HAPUUID kHAPCharacteristicType_ServiceSignature = HAPUUIDCreateAppleDefined(0xA5U);

const HAPUUID kHAPCharacteristicType_AccessoryFlags = HAPUUIDCreateAppleDefined(0xA6U);

const HAPUUID kHAPCharacteristicType_LockPhysicalControls = HAPUUIDCreateAppleDefined(0xA7U);

const HAPUUID kHAPCharacteristicType_TargetAirPurifierState = HAPUUIDCreateAppleDefined(0xA8U);

const HAPUUID kHAPCharacteristicType_CurrentAirPurifierState = HAPUUIDCreateAppleDefined(0xA9U);

const HAPUUID kHAPCharacteristicType_CurrentSlatState = HAPUUIDCreateAppleDefined(0xAAU);

const HAPUUID kHAPCharacteristicType_FilterLifeLevel = HAPUUIDCreateAppleDefined(0xABU);

const HAPUUID kHAPCharacteristicType_FilterChangeIndication = HAPUUIDCreateAppleDefined(0xACU);

const HAPUUID kHAPCharacteristicType_ResetFilterIndication = HAPUUIDCreateAppleDefined(0xADU);

const HAPUUID kHAPCharacteristicType_CurrentFanState = HAPUUIDCreateAppleDefined(0xAFU);

const HAPUUID kHAPCharacteristicType_Active = HAPUUIDCreateAppleDefined(0xB0U);

const HAPUUID kHAPCharacteristicType_CurrentHeaterCoolerState = HAPUUIDCreateAppleDefined(0xB1U);

const HAPUUID kHAPCharacteristicType_TargetHeaterCoolerState = HAPUUIDCreateAppleDefined(0xB2U);

const HAPUUID kHAPCharacteristicType_CurrentHumidifierDehumidifierState = HAPUUIDCreateAppleDefined(0xB3U);

const HAPUUID kHAPCharacteristicType_TargetHumidifierDehumidifierState = HAPUUIDCreateAppleDefined(0xB4U);

const HAPUUID kHAPCharacteristicType_WaterLevel = HAPUUIDCreateAppleDefined(0xB5U);

const HAPUUID kHAPCharacteristicType_SwingMode = HAPUUIDCreateAppleDefined(0xB6U);

const HAPUUID kHAPCharacteristicType_TargetFanState = HAPUUIDCreateAppleDefined(0xBFU);

const HAPUUID kHAPCharacteristicType_SlatType = HAPUUIDCreateAppleDefined(0xC0U);

const HAPUUID kHAPCharacteristicType_CurrentTiltAngle = HAPUUIDCreateAppleDefined(0xC1U);

const HAPUUID kHAPCharacteristicType_TargetTiltAngle = HAPUUIDCreateAppleDefined(0xC2U);

const HAPUUID kHAPCharacteristicType_OzoneDensity = HAPUUIDCreateAppleDefined(0xC3U);

const HAPUUID kHAPCharacteristicType_NitrogenDioxideDensity = HAPUUIDCreateAppleDefined(0xC4U);

const HAPUUID kHAPCharacteristicType_SulphurDioxideDensity = HAPUUIDCreateAppleDefined(0xC5U);

const HAPUUID kHAPCharacteristicType_PM2_5Density = HAPUUIDCreateAppleDefined(0xC6U);

const HAPUUID kHAPCharacteristicType_PM10Density = HAPUUIDCreateAppleDefined(0xC7U);

const HAPUUID kHAPCharacteristicType_VOCDensity = HAPUUIDCreateAppleDefined(0xC8U);

const HAPUUID kHAPCharacteristicType_RelativeHumidityDehumidifierThreshold = HAPUUIDCreateAppleDefined(0xC9U);

const HAPUUID kHAPCharacteristicType_RelativeHumidityHumidifierThreshold = HAPUUIDCreateAppleDefined(0xCAU);

const HAPUUID kHAPCharacteristicType_ServiceLabelIndex = HAPUUIDCreateAppleDefined(0xCBU);

const HAPUUID kHAPCharacteristicType_ServiceLabelNamespace = HAPUUIDCreateAppleDefined(0xCDU);

const HAPUUID kHAPCharacteristicType_ColorTemperature = HAPUUIDCreateAppleDefined(0xCEU);

const HAPUUID kHAPCharacteristicType_ProgramMode = HAPUUIDCreateAppleDefined(0xD1U);

const HAPUUID kHAPCharacteristicType_InUse = HAPUUIDCreateAppleDefined(0xD2U);

const HAPUUID kHAPCharacteristicType_SetDuration = HAPUUIDCreateAppleDefined(0xD3U);

const HAPUUID kHAPCharacteristicType_RemainingDuration = HAPUUIDCreateAppleDefined(0xD4U);

const HAPUUID kHAPCharacteristicType_ValveType = HAPUUIDCreateAppleDefined(0xD5U);

const HAPUUID kHAPCharacteristicType_IsConfigured = HAPUUIDCreateAppleDefined(0xD6U);

const HAPUUID kHAPCharacteristicType_ConfiguredName = HAPUUIDCreateAppleDefined(0xE3U);

const HAPUUID kHAPCharacteristicType_ActiveIdentifier = HAPUUIDCreateAppleDefined(0xE7U);

const HAPUUID kHAPCharacteristicType_SupportedVideoStreamConfiguration = HAPUUIDCreateAppleDefined(0x114U);

const HAPUUID kHAPCharacteristicType_SupportedAudioStreamConfiguration = HAPUUIDCreateAppleDefined(0x115U);

const HAPUUID kHAPCharacteristicType_SupportedRTPConfiguration = HAPUUIDCreateAppleDefined(0x116U);

const HAPUUID kHAPCharacteristicType_SelectedRTPStreamConfiguration = HAPUUIDCreateAppleDefined(0x117U);

const HAPUUID kHAPCharacteristicType_SetupEndpoints = HAPUUIDCreateAppleDefined(0x118U);

const HAPUUID kHAPCharacteristicType_Volume = HAPUUIDCreateAppleDefined(0x119U);

const HAPUUID kHAPCharacteristicType_Mute = HAPUUIDCreateAppleDefined(0x11AU);

const HAPUUID kHAPCharacteristicType_NightVision = HAPUUIDCreateAppleDefined(0x11BU);

const HAPUUID kHAPCharacteristicType_OpticalZoom = HAPUUIDCreateAppleDefined(0x11CU);

const HAPUUID kHAPCharacteristicType_DigitalZoom = HAPUUIDCreateAppleDefined(0x11DU);

const HAPUUID kHAPCharacteristicType_ImageRotation = HAPUUIDCreateAppleDefined(0x11EU);

const HAPUUID kHAPCharacteristicType_ImageMirroring = HAPUUIDCreateAppleDefined(0x11FU);

const HAPUUID kHAPCharacteristicType_StreamingStatus = HAPUUIDCreateAppleDefined(0x120U);

const HAPUUID kHAPCharacteristicType_TargetControlSupportedConfiguration = HAPUUIDCreateAppleDefined(0x123U);

const HAPUUID kHAPCharacteristicType_TargetControlList = HAPUUIDCreateAppleDefined(0x124U);

const HAPUUID kHAPCharacteristicType_ButtonEvent = HAPUUIDCreateAppleDefined(0x126U);

const HAPUUID kHAPCharacteristicType_SelectedAudioStreamConfiguration = HAPUUIDCreateAppleDefined(0x128U);

const HAPUUID kHAPCharacteristicType_SupportedDataStreamTransportConfiguration = HAPUUIDCreateAppleDefined(0x130U);

const HAPUUID kHAPCharacteristicType_SetupDataStreamTransport = HAPUUIDCreateAppleDefined(0x131U);

const HAPUUID kHAPCharacteristicType_SiriInputType = HAPUUIDCreateAppleDefined(0x132U);

const HAPUUID kHAPCharacteristicType_SupportedMetrics = HAPUUIDCreateAppleDefined(0x271U);

const HAPUUID kHAPCharacteristicType_MetricsBufferFullState = HAPUUIDCreateAppleDefined(0x272U);

const HAPUUID kHAPCharacteristicType_DataStreamHAPTransport = HAPUUIDCreateAppleDefined(0x138U);

const HAPUUID kHAPCharacteristicType_DataStreamHAPTransportInterrupt = HAPUUIDCreateAppleDefined(0x139U);

const HAPUUID kHAPCharacteristicType_SupportedCameraRecordingConfiguration = HAPUUIDCreateAppleDefined(0x205U);

const HAPUUID kHAPCharacteristicType_SupportedVideoRecordingConfiguration = HAPUUIDCreateAppleDefined(0x206U);

const HAPUUID kHAPCharacteristicType_SupportedAudioRecordingConfiguration = HAPUUIDCreateAppleDefined(0x207U);

const HAPUUID kHAPCharacteristicType_SelectedCameraRecordingConfiguration = HAPUUIDCreateAppleDefined(0x209U);

const HAPUUID kHAPCharacteristicType_NetworkClientProfileControl = HAPUUIDCreateAppleDefined(0x20CU);

const HAPUUID kHAPCharacteristicType_NfcAccessSupportedConfiguration = HAPUUIDCreateAppleDefined(0x265U);

static const HAPUInt16TLVFormat kHAPCharacteristicTLVFormat_NfcAccessSupportedConfiguration_UInt16 = {
    .type = kHAPTLVFormatType_UInt16,
    .constraints = { .minimumValue = 0, .maximumValue = UINT16_MAX }
};

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_NfcAccessSupportedConfiguration[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessSupportedConfiguration, maximumIssuerKeys),
            .tlvType = kHAPCharacteristicTLVType_NfcAccessSupportedConfiguration_MaximumIssuerKeys,
            .debugDescription = kHAPCharacteristicTLVDescription_NfcAccessSupportedConfiguration_MaximumIssuerKeys,
            .format = &kHAPCharacteristicTLVFormat_NfcAccessSupportedConfiguration_UInt16,
    },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(
                    HAPCharacteristicValue_NfcAccessSupportedConfiguration,
                    maximumSuspendedDeviceCredentialKeys),
            .tlvType = kHAPCharacteristicTLVType_NfcAccessSupportedConfiguration_MaximumSuspendedDeviceCredentialKeys,
            .debugDescription =
                    kHAPCharacteristicTLVDescription_NfcAccessSupportedConfiguration_MaximumSuspendedDeviceCredentialKeys,
            .format = &kHAPCharacteristicTLVFormat_NfcAccessSupportedConfiguration_UInt16,
    },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(
                    HAPCharacteristicValue_NfcAccessSupportedConfiguration,
                    maximumActiveDeviceCredentialKeys),
            .tlvType = kHAPCharacteristicTLVType_NfcAccessSupportedConfiguration_MaximumActiveDeviceCredentialKeys,
            .debugDescription =
                    kHAPCharacteristicTLVDescription_NfcAccessSupportedConfiguration_MaximumActiveDeviceCredentialKeys,
            .format = &kHAPCharacteristicTLVFormat_NfcAccessSupportedConfiguration_UInt16,
    },
    NULL
};

const HAPCharacteristicTLVFormat_NfcAccessSupportedConfiguration
        kHAPCharacteristicTLVFormat_NfcAccessSupportedConfiguration = {
            .type = kHAPTLVFormatType_Struct,
            .members = kHAPCharacteristicTLVMembers_NfcAccessSupportedConfiguration
        };

const HAPUUID kHAPCharacteristicType_NfcAccessControlPoint = HAPUUIDCreateAppleDefined(0x264U);

/* Generic formats for all NFC Access keys */
static const HAPUInt8TLVFormat kHAPCharacteristicTLVFormat_NfcAccessKey_Type = {
    .type = kHAPTLVFormatType_UInt8,
    .constraints = { .minimumValue = 0, .maximumValue = UINT8_MAX }
};

static const HAPDataTLVFormat kHAPCharacteristicTLVFormat_NfcAccessKey_Identifier = {
    .type = kHAPTLVFormatType_Data,
    .constraints = { .minLength = NFC_ACCESS_KEY_IDENTIFIER_BYTES, .maxLength = NFC_ACCESS_KEY_IDENTIFIER_BYTES }
};

static const HAPUInt8TLVFormat kHAPCharacteristicTLVFormat_NfcAccessKeyResponse_StatusCode = {
    .type = kHAPTLVFormatType_UInt8,
    .constraints = { .minimumValue = kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success,
                     .maximumValue = kHAPCharacteristicValue_NfcAccessResponseStatusCode_ErrorNotSupported }
};

/* NFC Access Issuer Key */
static const HAPDataTLVFormat kHAPCharacteristicTLVFormat_NfcAccessIssuerKeyRequest_Key = {
    .type = kHAPTLVFormatType_Data,
    .constraints = { .minLength = NFC_ACCESS_ISSUER_KEY_BYTES, .maxLength = NFC_ACCESS_ISSUER_KEY_BYTES },
};

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_NfcAccessIssuerKeyRequest[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessIssuerKeyRequest, type),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessIssuerKeyRequest, typeIsSet),
            .tlvType = kHAPCharacteristicTLVType_NfcAccessIssuerKeyRequest_Type,
            .debugDescription = kHAPCharacteristicTLVDescription_NfcAccessIssuerKeyRequest_Type,
            .format = &kHAPCharacteristicTLVFormat_NfcAccessKey_Type,
            .isOptional = true,
    },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessIssuerKeyRequest, key),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessIssuerKeyRequest, keyIsSet),
            .tlvType = kHAPCharacteristicTLVType_NfcAccessIssuerKeyRequest_Key,
            .debugDescription = kHAPCharacteristicTLVDescription_NfcAccessIssuerKeyRequest_Key,
            .format = &kHAPCharacteristicTLVFormat_NfcAccessIssuerKeyRequest_Key,
            .isOptional = true,
    },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessIssuerKeyRequest, identifier),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessIssuerKeyRequest, identifierIsSet),
            .tlvType = kHAPCharacteristicTLVType_NfcAccessIssuerKeyRequest_Identifier,
            .debugDescription = kHAPCharacteristicTLVDescription_NfcAccessIssuerKeyRequest_Identifier,
            .format = &kHAPCharacteristicTLVFormat_NfcAccessKey_Identifier,
            .isOptional = true,
    },
    NULL
};

const HAPCharacteristicTLVFormat_NfcAccessIssuerKeyRequest kHAPCharacteristicTLVFormat_NfcAccessIssuerKeyRequest = {
    .type = kHAPTLVFormatType_Struct,
    .members = kHAPCharacteristicTLVMembers_NfcAccessIssuerKeyRequest
};

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_NfcAccessIssuerKeyResponse[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessIssuerKeyResponse, identifier),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessIssuerKeyResponse, identifierIsSet),
            .tlvType = kHAPCharacteristicTLVType_NfcAccessIssuerKeyResponse_Identifier,
            .debugDescription = kHAPCharacteristicTLVDescription_NfcAccessIssuerKeyResponse_Identifier,
            .format = &kHAPCharacteristicTLVFormat_NfcAccessKey_Identifier,
            .isOptional = true,
    },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessIssuerKeyResponse, statusCode),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessIssuerKeyResponse, statusCodeIsSet),
            .tlvType = kHAPCharacteristicTLVType_NfcAccessIssuerKeyResponse_StatusCode,
            .debugDescription = kHAPCharacteristicTLVDescription_NfcAccessIssuerKeyResponse_StatusCode,
            .format = &kHAPCharacteristicTLVFormat_NfcAccessKeyResponse_StatusCode,
            .isOptional = true,
    },
    NULL
};

const HAPCharacteristicTLVFormat_NfcAccessIssuerKeyResponse kHAPCharacteristicTLVFormat_NfcAccessIssuerKeyResponse = {
    .type = kHAPTLVFormatType_Struct,
    .members = kHAPCharacteristicTLVMembers_NfcAccessIssuerKeyResponse
};

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_NfcAccessControlPoint_IssuerKeyResponse[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessControlPoint_IssuerKeyResponse, data),
            .tlvType = kHAPCharacteristicTLVType_NfcAccessControlPoint_IssuerKeyResponse,
            .debugDescription = kHAPCharacteristicTLVDescription_NfcAccessControlPoint_IssuerKeyResponse,
            .format = &kHAPCharacteristicTLVFormat_NfcAccessIssuerKeyResponse,
    },
    NULL
};

const HAPCharacteristicTLVFormat_NfcAccessControlPoint_IssuerKeyResponse
        kHAPCharacteristicTLVFormat_NfcAccessControlPoint_IssuerKeyResponse = {
            .type = kHAPTLVFormatType_Struct,
            .members = kHAPCharacteristicTLVMembers_NfcAccessControlPoint_IssuerKeyResponse
        };

/* NFC Access Device Credential Key */
static const HAPDataTLVFormat kHAPCharacteristicTLVFormat_NfcAccessDeviceCredentialKeyRequest_Key = {
    .type = kHAPTLVFormatType_Data,
    .constraints = { .minLength = NFC_ACCESS_DEVICE_CREDENTIAL_KEY_BYTES,
                     .maxLength = NFC_ACCESS_DEVICE_CREDENTIAL_KEY_BYTES },
};

static const HAPUInt8TLVFormat kHAPCharacteristicTLVFormat_NfcAccessDeviceCredentialKeyRequest_State = {
    .type = kHAPTLVFormatType_UInt8,
    .constraints = { .minimumValue = kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Suspended,
                     .maximumValue = kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Active },
};

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_NfcAccessDeviceCredentialKeyRequest[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessDeviceCredentialKeyRequest, type),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessDeviceCredentialKeyRequest, typeIsSet),
            .tlvType = kHAPCharacteristicTLVType_NfcAccessDeviceCredentialKeyRequest_Type,
            .debugDescription = kHAPCharacteristicTLVDescription_NfcAccessDeviceCredentialKeyRequest_Type,
            .format = &kHAPCharacteristicTLVFormat_NfcAccessKey_Type,
            .isOptional = true,
    },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessDeviceCredentialKeyRequest, key),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessDeviceCredentialKeyRequest, keyIsSet),
            .tlvType = kHAPCharacteristicTLVType_NfcAccessDeviceCredentialKeyRequest_Key,
            .debugDescription = kHAPCharacteristicTLVDescription_NfcAccessDeviceCredentialKeyRequest_Key,
            .format = &kHAPCharacteristicTLVFormat_NfcAccessDeviceCredentialKeyRequest_Key,
            .isOptional = true,
    },
    &(const HAPStructTLVMember) {
            .valueOffset =
                    HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessDeviceCredentialKeyRequest, issuerKeyIdentifier),
            .isSetOffset =
                    HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessDeviceCredentialKeyRequest, issuerKeyIdentifierIsSet),
            .tlvType = kHAPCharacteristicTLVType_NfcAccessDeviceCredentialKeyRequest_IssuerKeyIdentifier,
            .debugDescription =
                    kHAPCharacteristicTLVDescription_NfcAccessDeviceCredentialKeyRequest_IssuerKeyIdentifier,
            .format = &kHAPCharacteristicTLVFormat_NfcAccessKey_Identifier,
            .isOptional = true,
    },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessDeviceCredentialKeyRequest, state),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessDeviceCredentialKeyRequest, stateIsSet),
            .tlvType = kHAPCharacteristicTLVType_NfcAccessDeviceCredentialKeyRequest_State,
            .debugDescription = kHAPCharacteristicTLVDescription_NfcAccessDeviceCredentialKeyRequest_State,
            .format = &kHAPCharacteristicTLVFormat_NfcAccessDeviceCredentialKeyRequest_State,
            .isOptional = true,
    },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessDeviceCredentialKeyRequest, identifier),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessDeviceCredentialKeyRequest, identifierIsSet),
            .tlvType = kHAPCharacteristicTLVType_NfcAccessDeviceCredentialKeyRequest_Identifier,
            .debugDescription = kHAPCharacteristicTLVDescription_NfcAccessDeviceCredentialKeyRequest_Identifier,
            .format = &kHAPCharacteristicTLVFormat_NfcAccessKey_Identifier,
            .isOptional = true,
    },
    NULL
};

const HAPCharacteristicTLVFormat_NfcAccessDeviceCredentialKeyRequest
        kHAPCharacteristicTLVFormat_NfcAccessDeviceCredentialKeyRequest = {
            .type = kHAPTLVFormatType_Struct,
            .members = kHAPCharacteristicTLVMembers_NfcAccessDeviceCredentialKeyRequest
        };

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_NfcAccessDeviceCredentialKeyResponse[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessDeviceCredentialKeyResponse, identifier),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessDeviceCredentialKeyResponse, identifierIsSet),
            .tlvType = kHAPCharacteristicTLVType_NfcAccessDeviceCredentialKeyResponse_Identifier,
            .debugDescription = kHAPCharacteristicTLVDescription_NfcAccessDeviceCredentialKeyResponse_Identifier,
            .format = &kHAPCharacteristicTLVFormat_NfcAccessKey_Identifier,
            .isOptional = true,
    },
    &(const HAPStructTLVMember) {
            .valueOffset =
                    HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessDeviceCredentialKeyResponse, issuerKeyIdentifier),
            .isSetOffset =
                    HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessDeviceCredentialKeyResponse, issuerKeyIdentifierIsSet),
            .tlvType = kHAPCharacteristicTLVType_NfcAccessDeviceCredentialKeyResponse_IssuerKeyIdentifier,
            .debugDescription =
                    kHAPCharacteristicTLVDescription_NfcAccessDeviceCredentialKeyResponse_IssuerKeyIdentifier,
            .format = &kHAPCharacteristicTLVFormat_NfcAccessKey_Identifier,
            .isOptional = true,
    },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessDeviceCredentialKeyResponse, statusCode),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessDeviceCredentialKeyResponse, statusCodeIsSet),
            .tlvType = kHAPCharacteristicTLVType_NfcAccessDeviceCredentialKeyResponse_StatusCode,
            .debugDescription = kHAPCharacteristicTLVDescription_NfcAccessDeviceCredentialKeyResponse_StatusCode,
            .format = &kHAPCharacteristicTLVFormat_NfcAccessKeyResponse_StatusCode,
            .isOptional = true,
    },
    NULL
};

const HAPCharacteristicTLVFormat_NfcAccessDeviceCredentialKeyResponse
        kHAPCharacteristicTLVFormat_NfcAccessDeviceCredentialKeyResponse = {
            .type = kHAPTLVFormatType_Struct,
            .members = kHAPCharacteristicTLVMembers_NfcAccessDeviceCredentialKeyResponse
        };

static const HAPStructTLVMember* const
        kHAPCharacteristicTLVMembers_NfcAccessControlPoint_DeviceCredentialKeyResponse[] = {
            &(const HAPStructTLVMember) {
                    .valueOffset = HAP_OFFSETOF(
                            HAPCharacteristicValue_NfcAccessControlPoint_DeviceCredentialKeyResponse,
                            data),
                    .tlvType = kHAPCharacteristicTLVType_NfcAccessControlPoint_DeviceCredentialKeyResponse,
                    .debugDescription =
                            kHAPCharacteristicTLVDescription_NfcAccessControlPoint_DeviceCredentialKeyResponse,
                    .format = &kHAPCharacteristicTLVFormat_NfcAccessDeviceCredentialKeyResponse,
            },
            NULL
        };

const HAPCharacteristicTLVFormat_NfcAccessControlPoint_DeviceCredentialKeyResponse
        kHAPCharacteristicTLVFormat_NfcAccessControlPoint_DeviceCredentialKeyResponse = {
            .type = kHAPTLVFormatType_Struct,
            .members = kHAPCharacteristicTLVMembers_NfcAccessControlPoint_DeviceCredentialKeyResponse
        };

/* NFC Access Reader Key */
static const HAPDataTLVFormat kHAPCharacteristicTLVFormat_NfcAccessReaderKeyRequest_Key = {
    .type = kHAPTLVFormatType_Data,
    .constraints = { .minLength = NFC_ACCESS_READER_KEY_BYTES, .maxLength = NFC_ACCESS_READER_KEY_BYTES },
};

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_NfcAccessReaderKeyRequest[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessReaderKeyRequest, type),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessReaderKeyRequest, typeIsSet),
            .tlvType = kHAPCharacteristicTLVType_NfcAccessReaderKeyRequest_Type,
            .debugDescription = kHAPCharacteristicTLVDescription_NfcAccessReaderKeyRequest_Type,
            .format = &kHAPCharacteristicTLVFormat_NfcAccessKey_Type,
            .isOptional = true,
    },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessReaderKeyRequest, key),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessReaderKeyRequest, keyIsSet),
            .tlvType = kHAPCharacteristicTLVType_NfcAccessReaderKeyRequest_Key,
            .debugDescription = kHAPCharacteristicTLVDescription_NfcAccessReaderKeyRequest_Key,
            .format = &kHAPCharacteristicTLVFormat_NfcAccessReaderKeyRequest_Key,
            .isOptional = true,
    },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessReaderKeyRequest, readerIdentifier),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessReaderKeyRequest, readerIdentifierIsSet),
            .tlvType = kHAPCharacteristicTLVType_NfcAccessReaderKeyRequest_ReaderIdentifier,
            .debugDescription = kHAPCharacteristicTLVDescription_NfcAccessReaderKeyRequest_ReaderIdentifier,
            .format = &kHAPCharacteristicTLVFormat_NfcAccessKey_Identifier,
            .isOptional = true,
    },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessReaderKeyRequest, identifier),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessReaderKeyRequest, identifierIsSet),
            .tlvType = kHAPCharacteristicTLVType_NfcAccessReaderKeyRequest_Identifier,
            .debugDescription = kHAPCharacteristicTLVDescription_NfcAccessReaderKeyRequest_Identifier,
            .format = &kHAPCharacteristicTLVFormat_NfcAccessKey_Identifier,
            .isOptional = true,
    },
    NULL
};

const HAPCharacteristicTLVFormat_NfcAccessReaderKeyRequest kHAPCharacteristicTLVFormat_NfcAccessReaderKeyRequest = {
    .type = kHAPTLVFormatType_Struct,
    .members = kHAPCharacteristicTLVMembers_NfcAccessReaderKeyRequest
};

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_NfcAccessReaderKeyResponse[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessReaderKeyResponse, identifier),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessReaderKeyResponse, identifierIsSet),
            .tlvType = kHAPCharacteristicTLVType_NfcAccessReaderKeyResponse_Identifier,
            .debugDescription = kHAPCharacteristicTLVDescription_NfcAccessReaderKeyResponse_Identifier,
            .format = &kHAPCharacteristicTLVFormat_NfcAccessKey_Identifier,
            .isOptional = true,
    },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessReaderKeyResponse, statusCode),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessReaderKeyResponse, statusCodeIsSet),
            .tlvType = kHAPCharacteristicTLVType_NfcAccessReaderKeyResponse_StatusCode,
            .debugDescription = kHAPCharacteristicTLVDescription_NfcAccessReaderKeyResponse_StatusCode,
            .format = &kHAPCharacteristicTLVFormat_NfcAccessKeyResponse_StatusCode,
            .isOptional = true,
    },
    NULL
};

const HAPCharacteristicTLVFormat_NfcAccessReaderKeyResponse kHAPCharacteristicTLVFormat_NfcAccessReaderKeyResponse = {
    .type = kHAPTLVFormatType_Struct,
    .members = kHAPCharacteristicTLVMembers_NfcAccessReaderKeyResponse
};

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_NfcAccessControlPoint_ReaderKeyResponse[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NfcAccessControlPoint_ReaderKeyResponse, data),
            .tlvType = kHAPCharacteristicTLVType_NfcAccessControlPoint_ReaderKeyResponse,
            .debugDescription = kHAPCharacteristicTLVDescription_NfcAccessControlPoint_ReaderKeyResponse,
            .format = &kHAPCharacteristicTLVFormat_NfcAccessReaderKeyResponse,
    },
    NULL
};

const HAPCharacteristicTLVFormat_NfcAccessControlPoint_ReaderKeyResponse
        kHAPCharacteristicTLVFormat_NfcAccessControlPoint_ReaderKeyResponse = {
            .type = kHAPTLVFormatType_Struct,
            .members = kHAPCharacteristicTLVMembers_NfcAccessControlPoint_ReaderKeyResponse
        };

const HAPUUID kHAPCharacteristicType_ConfigurationState = HAPUUIDCreateAppleDefined(0x263U);

const HAPUUID kHAPCharacteristicType_HardwareFinish = HAPUUIDCreateAppleDefined(0x26CU);

static const HAPUInt32TLVFormat kHAPCharacteristicTLVFormat_HardwareFinish_UInt32 = {
    .type = kHAPTLVFormatType_UInt32,
    .constraints = { .minimumValue = 0, .maximumValue = UINT32_MAX }
};

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_HardwareFinish[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_HardwareFinish, rgbColorValue),
            .tlvType = kHAPCharacteristicTLVType_HardwareFinish_RgbColorValue,
            .debugDescription = kHAPCharacteristicTLVDescription_HardwareFinish_RgbColorValue,
            .format = &kHAPCharacteristicTLVFormat_HardwareFinish_UInt32,
    },
    NULL
};

const HAPCharacteristicTLVFormat_HardwareFinish kHAPCharacteristicTLVFormat_HardwareFinish = {
    .type = kHAPTLVFormatType_Struct,
    .members = kHAPCharacteristicTLVMembers_HardwareFinish
};

#if HAP_TESTING
const HAPUUID kAppCharacteristicType_ShortDebugCommand = HAPUUIDCreateDebug(0x40DF);
const HAPUUID kAppCharacteristicType_DebugCommand = HAPUUIDCreateDebug(0x40A3);
#endif

//----------------------------------------------------------------------------------------------------------------------

static const HAPSeparatorTLVFormat kHAPCharacteristicTLVFormat_Separator = { .type = kHAPTLVFormatType_None };

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_WiFiRouter_OperationStatus_IsValid(uint8_t value) {
    HAPAssert(sizeof value == sizeof(HAPCharacteristicValue_WiFiRouter_OperationStatus));
    switch ((HAPCharacteristicValue_WiFiRouter_OperationStatus) value) {
        case kHAPCharacteristicValue_WiFiRouter_OperationStatus_Success:
        case kHAPCharacteristicValue_WiFiRouter_OperationStatus_Unknown:
        case kHAPCharacteristicValue_WiFiRouter_OperationStatus_NotAllowed:
        case kHAPCharacteristicValue_WiFiRouter_OperationStatus_OutOfResources:
        case kHAPCharacteristicValue_WiFiRouter_OperationStatus_BulkOperationFailed:
        case kHAPCharacteristicValue_WiFiRouter_OperationStatus_InvalidParameters:
        case kHAPCharacteristicValue_WiFiRouter_OperationStatus_InvalidIdentifier:
        case kHAPCharacteristicValue_WiFiRouter_OperationStatus_Duplicate:
        case kHAPCharacteristicValue_WiFiRouter_OperationStatus_InvalidFirewallRule: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
static const char* HAPCharacteristicValue_WiFiRouter_OperationStatus_GetDescription(
        HAPCharacteristicValue_WiFiRouter_OperationStatus value) {
    HAPPrecondition(HAPCharacteristicValue_WiFiRouter_OperationStatus_IsValid(value));
    switch (value) {
        case kHAPCharacteristicValue_WiFiRouter_OperationStatus_Success:
            return "Success";
        case kHAPCharacteristicValue_WiFiRouter_OperationStatus_Unknown:
            return "Error - Unknown";
        case kHAPCharacteristicValue_WiFiRouter_OperationStatus_NotAllowed:
            return "Error - Not allowed";
        case kHAPCharacteristicValue_WiFiRouter_OperationStatus_OutOfResources:
            return "Error - Out of resources";
        case kHAPCharacteristicValue_WiFiRouter_OperationStatus_BulkOperationFailed:
            return "Error - Bulk operation failed";
        case kHAPCharacteristicValue_WiFiRouter_OperationStatus_InvalidParameters:
            return "Error - Invalid parameters";
        case kHAPCharacteristicValue_WiFiRouter_OperationStatus_InvalidIdentifier:
            return "Error - Invalid Identifier";
        case kHAPCharacteristicValue_WiFiRouter_OperationStatus_Duplicate:
            return "Error - Duplicate";
        case kHAPCharacteristicValue_WiFiRouter_OperationStatus_InvalidFirewallRule:
            return "Error - Invalid firewall rule";
        default:
            HAPFatalError();
    }
}

static const HAPCharacteristicTLVFormat_WiFiRouter_OperationStatus
        kHAPCharacteristicTLVFormat_WiFiRouter_OperationStatus = {
            .type = kHAPTLVFormatType_Enum,
            .callbacks = { .isValid = HAPCharacteristicValue_WiFiRouter_OperationStatus_IsValid,
                           .getDescription = HAPCharacteristicValue_WiFiRouter_OperationStatus_GetDescription }
        };

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
static HAPError
        HAPCharacteristicValue_WiFiRouter_IPv4Address_Decode(HAPIPv4Address* value, void* bytes, size_t numBytes) {
    HAPPrecondition(value);
    HAPRawBufferZero(value, sizeof *value);
    HAPPrecondition(bytes);

    if (numBytes != sizeof value->bytes) {
        return kHAPError_InvalidData;
    }
    HAPRawBufferCopyBytes(value->bytes, bytes, numBytes);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static HAPError HAPCharacteristicValue_WiFiRouter_IPv4Address_Encode(
        HAPIPv4Address* value,
        void* bytes,
        size_t maxBytes,
        size_t* numBytes) {
    HAPPrecondition(value);
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes);

    if (maxBytes < sizeof value->bytes) {
        return kHAPError_OutOfResources;
    }
    HAPRawBufferCopyBytes(bytes, value->bytes, sizeof value->bytes);
    *numBytes = sizeof value->bytes;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static HAPError HAPCharacteristicValue_WiFiRouter_IPv4Address_GetDescription(
        HAPIPv4Address* value,
        char* bytes,
        size_t maxBytes) {
    HAPPrecondition(value);
    HAPPrecondition(bytes);

    return HAPIPv4AddressGetDescription(value, bytes, maxBytes);
}

static const HAPIPv4AddressTLVFormat kHAPCharacteristicTLVFormat_WiFiRouter_IPv4Address = {
    .type = kHAPTLVFormatType_Value,
    .callbacks = { .decode = HAPCharacteristicValue_WiFiRouter_IPv4Address_Decode,
                   .encode = HAPCharacteristicValue_WiFiRouter_IPv4Address_Encode,
                   .getDescription = HAPCharacteristicValue_WiFiRouter_IPv4Address_GetDescription }
};

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
static HAPError
        HAPCharacteristicValue_WiFiRouter_IPv6Address_Decode(HAPIPv6Address* value, void* bytes, size_t numBytes) {
    HAPPrecondition(value);
    HAPRawBufferZero(value, sizeof *value);
    HAPPrecondition(bytes);

    if (numBytes != sizeof value->bytes) {
        return kHAPError_InvalidData;
    }
    HAPRawBufferCopyBytes(value->bytes, bytes, numBytes);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static HAPError HAPCharacteristicValue_WiFiRouter_IPv6Address_Encode(
        HAPIPv6Address* value,
        void* bytes,
        size_t maxBytes,
        size_t* numBytes) {
    HAPPrecondition(value);
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes);

    if (maxBytes < sizeof value->bytes) {
        return kHAPError_OutOfResources;
    }
    HAPRawBufferCopyBytes(bytes, value->bytes, sizeof value->bytes);
    *numBytes = sizeof value->bytes;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static HAPError HAPCharacteristicValue_WiFiRouter_IPv6Address_GetDescription(
        HAPIPv6Address* value,
        char* bytes,
        size_t maxBytes) {
    HAPPrecondition(value);
    HAPPrecondition(bytes);

    return HAPIPv6AddressGetDescription(value, bytes, maxBytes);
}

static const HAPIPv6AddressTLVFormat kHAPCharacteristicTLVFormat_WiFiRouter_IPv6Address = {
    .type = kHAPTLVFormatType_Value,
    .callbacks = { .decode = HAPCharacteristicValue_WiFiRouter_IPv6Address_Decode,
                   .encode = HAPCharacteristicValue_WiFiRouter_IPv6Address_Encode,
                   .getDescription = HAPCharacteristicValue_WiFiRouter_IPv6Address_GetDescription }
};

//----------------------------------------------------------------------------------------------------------------------

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_WiFiRouter_IPAddress[] = {
    &(const HAPStructTLVMember) { .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_IPAddress, ipv4Address),
                                  .isSetOffset =
                                          HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_IPAddress, ipv4AddressIsSet),
                                  .tlvType = kHAPCharacteristicTLVType_WiFiRouter_IPAddress_IPv4Address,
                                  .debugDescription = kHAPCharacteristicTLVDescription_WiFiRouter_IPAddress_IPv4Address,
                                  .format = &kHAPCharacteristicTLVFormat_WiFiRouter_IPv4Address,
                                  .isOptional = true },
    &(const HAPStructTLVMember) { .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_IPAddress, ipv6Address),
                                  .isSetOffset =
                                          HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_IPAddress, ipv6AddressIsSet),
                                  .tlvType = kHAPCharacteristicTLVType_WiFiRouter_IPAddress_IPv6Address,
                                  .debugDescription = kHAPCharacteristicTLVDescription_WiFiRouter_IPAddress_IPv6Address,
                                  .format = &kHAPCharacteristicTLVFormat_WiFiRouter_IPv6Address,
                                  .isOptional = true },
    NULL
};

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_WiFiRouter_IPAddress_IsValid(HAPCharacteristicValue_WiFiRouter_IPAddress* value) {
    HAPPrecondition(value);

    if (!value->ipv4AddressIsSet && !value->ipv6AddressIsSet) {
        HAPLog(&logObject, "IPv4 Address or IPv6 Address must be specified.");
        return false;
    }

    if (value->ipv4AddressIsSet && value->ipv6AddressIsSet) {
        if (!HAPIPv4AddressAreEqual(&value->ipv4Address, &kHAPIPAddress_IPv4Any._.ipv4) ||
            !HAPIPv6AddressAreEqual(&value->ipv6Address, &kHAPIPAddress_IPv6Any._.ipv6)) {
            HAPLog(&logObject, "Both IPv4 Address and IPv6 Address only allowed when they contain only zeros.");
            return false;
        }
    }

    return true;
}

static const HAPCharacteristicTLVFormat_WiFiRouter_IPAddress kHAPCharacteristicTLVFormat_WiFiRouter_IPAddress = {
    .type = kHAPTLVFormatType_Struct,
    .members = kHAPCharacteristicTLVMembers_WiFiRouter_IPAddress,
    .callbacks = { .isValid = HAPCharacteristicValue_WiFiRouter_IPAddress_IsValid }
};

//----------------------------------------------------------------------------------------------------------------------

static const HAPUInt16TLVFormat kHAPCharacteristicTLVFormat_WiFiRouter_NetworkPort = {
    .type = kHAPTLVFormatType_UInt16,
    .constraints = { .minimumValue = 0, .maximumValue = UINT16_MAX }
};

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
static HAPError
        HAPCharacteristicValue_WiFiRouter_MACAddress_Decode(HAPMACAddress* value, void* bytes, size_t numBytes) {
    HAPPrecondition(value);
    HAPRawBufferZero(value, sizeof *value);
    HAPPrecondition(bytes);

    if (numBytes != sizeof value->bytes) {
        return kHAPError_InvalidData;
    }
    HAPRawBufferCopyBytes(value->bytes, bytes, numBytes);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static HAPError HAPCharacteristicValue_WiFiRouter_MACAddress_Encode(
        HAPMACAddress* value,
        void* bytes,
        size_t maxBytes,
        size_t* numBytes) {
    HAPPrecondition(value);
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes);

    if (maxBytes < sizeof value->bytes) {
        return kHAPError_OutOfResources;
    }
    HAPRawBufferCopyBytes(bytes, value->bytes, sizeof value->bytes);
    *numBytes = sizeof value->bytes;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static HAPError HAPCharacteristicValue_WiFiRouter_MACAddress_GetDescription(
        HAPMACAddress* value,
        char* bytes,
        size_t maxBytes) {
    HAPPrecondition(value);
    HAPPrecondition(bytes);

    return HAPMACAddressGetDescription(value, bytes, maxBytes);
}

static const HAPMACAddressTLVFormat kHAPCharacteristicTLVFormat_WiFiRouter_MACAddress = {
    .type = kHAPTLVFormatType_Value,
    .callbacks = { .decode = HAPCharacteristicValue_WiFiRouter_MACAddress_Decode,
                   .encode = HAPCharacteristicValue_WiFiRouter_MACAddress_Encode,
                   .getDescription = HAPCharacteristicValue_WiFiRouter_MACAddress_GetDescription }
};

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_WiFiRouter_Credential_PSK_IsValid(const char* value) {
    HAPPrecondition(value);

    if (!HAPStringAreEqual(value, "")) {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP) || HAP_FEATURE_ENABLED(HAP_FEATURE_WAC) || \
        HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_ROUTER)
        if (!HAPWACEngineIsValidWPAPassphrase(value)) {
            return false;
        }
#else
        return false;
#endif
    }
    return true;
}

static const HAPStringTLVFormat kHAPCharacteristicTLVFormat_WiFiRouter_Credential_PSK = {
    .type = kHAPTLVFormatType_String,
    .constraints = { .minLength = 0, .maxLength = HAPMax(kHAPWiFiWPAPassphrase_MaxBytes, 2 * kHAPWiFiWPAPSK_NumBytes) },
    .callbacks = { .isValid = HAPCharacteristicValue_WiFiRouter_Credential_PSK_IsValid }
};

//----------------------------------------------------------------------------------------------------------------------

static const HAPUnionTLVVariant* const kHAPCharacteristicTLVVariants_WiFiRouter_Credential[] = {
    &(const HAPUnionTLVVariant) { .tlvType = kHAPCharacteristicValueType_WiFiRouter_Credential_MACAddress,
                                  .debugDescription =
                                          kHAPCharacteristicValueDescription_WiFiRouter_Credential_MACAddress,
                                  .format = &kHAPCharacteristicTLVFormat_WiFiRouter_MACAddress },
    &(const HAPUnionTLVVariant) { .tlvType = kHAPCharacteristicValueType_WiFiRouter_Credential_PSK,
                                  .debugDescription = kHAPCharacteristicValueDescription_WiFiRouter_Credential_PSK,
                                  .format = &kHAPCharacteristicTLVFormat_WiFiRouter_Credential_PSK },
    NULL
};

static const HAPCharacteristicTLVFormat_WiFiRouter_Credential kHAPCharacteristicTLVFormat_WiFiRouter_Credential = {
    .type = kHAPTLVFormatType_Union,
    .untaggedValueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_Credential, _),
    .variants = kHAPCharacteristicTLVVariants_WiFiRouter_Credential
};

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_WiFiRouter_ICMPProtocol_IsValid(uint8_t value) {
    HAPAssert(sizeof value == sizeof(HAPCharacteristicValue_WiFiRouter_ICMPProtocol));
    switch ((HAPCharacteristicValue_WiFiRouter_ICMPProtocol) value) {
        case kHAPCharacteristicValue_WiFiRouter_ICMPProtocol_ICMPv4:
        case kHAPCharacteristicValue_WiFiRouter_ICMPProtocol_ICMPv6: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
static const char* HAPCharacteristicValue_WiFiRouter_ICMPProtocol_GetDescription(
        HAPCharacteristicValue_WiFiRouter_ICMPProtocol value) {
    HAPPrecondition(HAPCharacteristicValue_WiFiRouter_ICMPProtocol_IsValid(value));
    switch (value) {
        case kHAPCharacteristicValue_WiFiRouter_ICMPProtocol_ICMPv4:
            return "ICMPv4";
        case kHAPCharacteristicValue_WiFiRouter_ICMPProtocol_ICMPv6:
            return "ICMPv6";
        default:
            HAPFatalError();
    }
}

static const HAPCharacteristicTLVFormat_WiFiRouter_ICMPProtocol kHAPCharacteristicTLVFormat_WiFiRouter_ICMPProtocol = {
    .type = kHAPTLVFormatType_Enum,
    .callbacks = { .isValid = HAPCharacteristicValue_WiFiRouter_ICMPProtocol_IsValid,
                   .getDescription = HAPCharacteristicValue_WiFiRouter_ICMPProtocol_GetDescription }
};

//----------------------------------------------------------------------------------------------------------------------

static const HAPUInt8TLVFormat kHAPCharacteristicTLVMetadata_WiFiRouter_ICMPTypeValue = {
    .type = kHAPTLVFormatType_UInt8,
    .constraints = { .minimumValue = 0, .maximumValue = UINT8_MAX }
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_WiFiRouter_ICMPType[] = {
    &(const HAPStructTLVMember) { .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_ICMPType, icmpProtocol),
                                  .tlvType = kHAPCharacteristicTLVType_WiFiRouter_ICMPType_Protocol,
                                  .debugDescription = kHAPCharacteristicTLVDescription_WiFiRouter_ICMPType_Protocol,
                                  .format = &kHAPCharacteristicTLVFormat_WiFiRouter_ICMPProtocol },
    &(const HAPStructTLVMember) { .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_ICMPType, typeValue),
                                  .isSetOffset =
                                          HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_ICMPType, typeValueIsSet),
                                  .tlvType = kHAPCharacteristicTLVType_WiFiRouter_ICMPType_TypeValue,
                                  .debugDescription = kHAPCharacteristicTLVDescription_WiFiRouter_ICMPType_TypeValue,
                                  .format = &kHAPCharacteristicTLVMetadata_WiFiRouter_ICMPTypeValue,
                                  .isOptional = true },
    NULL
};

static const HAPCharacteristicTLVFormat_WiFiRouter_ICMPType kHAPCharacteristicTLVFormat_WiFiRouter_ICMPType = {
    .type = kHAPTLVFormatType_Struct,
    .members = kHAPCharacteristicTLVMembers_WiFiRouter_ICMPType,
};

//----------------------------------------------------------------------------------------------------------------------

static const HAPCharacteristicRawTLVFormat_WiFiRouter_ICMPList kHAPCharacteristicRawTLVFormat_WiFiRouter_ICMPList = {
    .type = kHAPTLVFormatType_Sequence,
    .item = { .valueOffset = HAP_OFFSETOF(HAPCharacteristicRawValue_WiFiRouter_ICMPList, _),
              .tlvType = kHAPCharacteristicRawTLVType_WiFiRouter_ICMPList_ICMPType,
              .debugDescription = kHAPCharacteristicRawTLVDescription_WiFiRouter_ICMPList_ICMPType,
              .format = &kHAPCharacteristicTLVFormat_WiFiRouter_ICMPType },
    .separator = { .tlvType = kHAPCharacteristicTLVType_Separator,
                   .debugDescription = kHAPCharacteristicTLVDescription_Separator,
                   .format = &kHAPCharacteristicTLVFormat_Separator }
};

typedef struct {
    HAPCharacteristicValue_WiFiRouter_ICMPList* value;
    HAPError err;
} HAPCharacteristicContext_WiFiRouter_ICMPList;

static void HAPCharacteristicValue_WiFiRouter_ICMPList_EnumerateCallback(
        void* _Nullable context_,
        HAPCharacteristicValue_WiFiRouter_ICMPType* value,
        bool* shouldContinue) {
    HAPPrecondition(context_);
    HAPCharacteristicContext_WiFiRouter_ICMPList* context = context_;
    HAPPrecondition(context->value);
    HAPPrecondition(!context->err);
    HAPPrecondition(value);
    HAPPrecondition(shouldContinue);
    HAPPrecondition(*shouldContinue);

    if (context->value->numICMPTypes >= HAPArrayCount(context->value->icmpTypes)) {
        HAPLog(&logObject, "Too many ICMP Types specified (%zu maximum).", HAPArrayCount(context->value->icmpTypes));
        context->err = kHAPError_InvalidData;
        *shouldContinue = false;
        return;
    }

    HAPRawBufferCopyBytes(&context->value->icmpTypes[context->value->numICMPTypes], value, sizeof *value);
    context->value->numICMPTypes++;
}

HAP_RESULT_USE_CHECK
static HAPError HAPCharacteristicValue_WiFiRouter_ICMPList_Decode(
        HAPCharacteristicValue_WiFiRouter_ICMPList* value,
        void* bytes,
        size_t numBytes) {
    HAPPrecondition(value);
    HAPRawBufferZero(value, sizeof *value);
    HAPPrecondition(bytes);

    HAPError err;

    HAPTLVReader reader;
    HAPTLVReaderCreate(&reader, bytes, numBytes);

    HAPCharacteristicRawValue_WiFiRouter_ICMPList rawValue;
    err = HAPTLVReaderDecode(&reader, &kHAPCharacteristicRawTLVFormat_WiFiRouter_ICMPList, &rawValue);
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }

    HAPCharacteristicContext_WiFiRouter_ICMPList enumerateContext;
    HAPRawBufferZero(&enumerateContext, sizeof enumerateContext);
    enumerateContext.value = value;
    err = rawValue.enumerate(
            &rawValue.dataSource, HAPCharacteristicValue_WiFiRouter_ICMPList_EnumerateCallback, &enumerateContext);
    if (!err) {
        err = enumerateContext.err;
    }
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }
    if (!value->numICMPTypes) {
        HAPLog(&logObject, "No ICMP Types specified.");
        return kHAPError_InvalidData;
    }

    return kHAPError_None;
}

typedef struct {
    HAPCharacteristicValue_WiFiRouter_ICMPList* _Nullable value;
} HAPCharacteristicDataSource_WiFiRouter_ICMPList;
HAP_STATIC_ASSERT(
        sizeof(HAPCharacteristicDataSource_WiFiRouter_ICMPList) <= sizeof(HAPSequenceTLVDataSource),
        HAPCharacteristicDataSource_WiFiRouter_ICMPList);

HAP_RESULT_USE_CHECK
static HAPError HAPCharacteristicValue_WiFiRouter_ICMPList_Enumerate(
        HAPSequenceTLVDataSource* dataSource_,
        HAPCharacteristicValueCallback_WiFiRouter_ICMPList_ICMPType callback,
        void* _Nullable context) {
    HAPPrecondition(dataSource_);
    HAPCharacteristicDataSource_WiFiRouter_ICMPList* dataSource =
            (HAPCharacteristicDataSource_WiFiRouter_ICMPList*) dataSource_;
    HAPPrecondition(callback);

    if (!dataSource->value) {
        // Enumeration is no longer possible.
        return kHAPError_Unknown;
    }

    bool shouldContinue = true;
    for (size_t i = 0; shouldContinue && i < dataSource->value->numICMPTypes; i++) {
        callback(context, &dataSource->value->icmpTypes[i], &shouldContinue);
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static HAPError HAPCharacteristicValue_WiFiRouter_ICMPList_Encode(
        HAPCharacteristicValue_WiFiRouter_ICMPList* value,
        void* bytes,
        size_t maxBytes,
        size_t* numBytes) {
    HAPPrecondition(value);
    HAPPrecondition(value->numICMPTypes);
    HAPPrecondition(value->numICMPTypes <= HAPArrayCount(value->icmpTypes));
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes);

    HAPError err;

    HAPCharacteristicRawValue_WiFiRouter_ICMPList rawValue;
    HAPRawBufferZero(&rawValue, sizeof rawValue);
    HAPCharacteristicDataSource_WiFiRouter_ICMPList* dataSource =
            (HAPCharacteristicDataSource_WiFiRouter_ICMPList*) &rawValue.dataSource;
    dataSource->value = value;
    rawValue.enumerate = HAPCharacteristicValue_WiFiRouter_ICMPList_Enumerate;

    HAPTLVWriter writer;
    HAPTLVWriterCreate(&writer, bytes, maxBytes);
    err = HAPTLVWriterEncode(&writer, &kHAPCharacteristicRawTLVFormat_WiFiRouter_ICMPList, &rawValue);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                err == kHAPError_Busy);
        return err;
    }

    // Prevent enumeration after callback as it is no longer valid once we return.
    dataSource->value = NULL;

    void* tlvBytes;
    HAPTLVWriterGetBuffer(&writer, &tlvBytes, numBytes);
    HAPAssert(tlvBytes == bytes);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static HAPError HAPCharacteristicValue_WiFiRouter_ICMPList_GetDescription(
        HAPCharacteristicValue_WiFiRouter_ICMPList* value,
        char* bytes,
        size_t maxBytes) {
    HAPPrecondition(value);
    HAPPrecondition(value->numICMPTypes);
    HAPPrecondition(value->numICMPTypes <= HAPArrayCount(value->icmpTypes));
    HAPPrecondition(bytes);

    HAPStringBuilder stringBuilder;
    HAPStringBuilderCreate(&stringBuilder, bytes, maxBytes);
    HAPStringBuilderAppend(&stringBuilder, "[");
    for (size_t i = 0; i < value->numICMPTypes; i++) {
        if (i) {
            HAPStringBuilderAppend(&stringBuilder, ", ");
        }
        switch (value->icmpTypes[i].icmpProtocol) {
            case kHAPCharacteristicValue_WiFiRouter_ICMPProtocol_ICMPv4: {
                HAPStringBuilderAppend(&stringBuilder, "icmpv4");
                break;
            }
            case kHAPCharacteristicValue_WiFiRouter_ICMPProtocol_ICMPv6: {
                HAPStringBuilderAppend(&stringBuilder, "icmpv6");
                break;
            }
        }
        if (value->icmpTypes[i].typeValueIsSet) {
            HAPStringBuilderAppend(&stringBuilder, ":%u,any", value->icmpTypes[i].typeValue);
        } else {
            HAPStringBuilderAppend(&stringBuilder, ":any");
        }
    }
    HAPStringBuilderAppend(&stringBuilder, "]");

    if (HAPStringBuilderDidOverflow(&stringBuilder)) {
        return kHAPError_OutOfResources;
    }
    return kHAPError_None;
}

static const HAPCharacteristicTLVFormat_WiFiRouter_ICMPList kHAPCharacteristicTLVFormat_WiFiRouter_ICMPList = {
    .type = kHAPTLVFormatType_Value,
    .callbacks = { .decode = HAPCharacteristicValue_WiFiRouter_ICMPList_Decode,
                   .encode = HAPCharacteristicValue_WiFiRouter_ICMPList_Encode,
                   .getDescription = HAPCharacteristicValue_WiFiRouter_ICMPList_GetDescription }
};

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_WiFiRouter_FirewallType_IsValid(uint8_t value) {
    HAPAssert(sizeof value == sizeof(HAPCharacteristicValue_WiFiRouter_FirewallType));
    switch ((HAPCharacteristicValue_WiFiRouter_FirewallType) value) {
        case kHAPCharacteristicValue_WiFiRouter_FirewallType_FullAccess:
        case kHAPCharacteristicValue_WiFiRouter_FirewallType_Allowlist: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
static const char* HAPCharacteristicValue_WiFiRouter_FirewallType_GetDescription(
        HAPCharacteristicValue_WiFiRouter_FirewallType value) {
    HAPPrecondition(HAPCharacteristicValue_WiFiRouter_FirewallType_IsValid(value));
    switch (value) {
        case kHAPCharacteristicValue_WiFiRouter_FirewallType_FullAccess:
            return "Full Access";
        case kHAPCharacteristicValue_WiFiRouter_FirewallType_Allowlist:
            return "Allowlist";
        default:
            HAPFatalError();
    }
}

static const HAPCharacteristicTLVFormat_WiFiRouter_FirewallType kHAPCharacteristicTLVFormat_WiFiRouter_FirewallType = {
    .type = kHAPTLVFormatType_Enum,
    .callbacks = { .isValid = HAPCharacteristicValue_WiFiRouter_FirewallType_IsValid,
                   .getDescription = HAPCharacteristicValue_WiFiRouter_FirewallType_GetDescription }
};

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule_Protocol_IsValid(uint8_t value) {
    HAPAssert(sizeof value == sizeof(HAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule_Protocol));
    switch ((HAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule_Protocol) value) {
        case kHAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule_Protocol_TCP:
        case kHAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule_Protocol_UDP: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
static const char* HAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule_Protocol_GetDescription(
        HAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule_Protocol value) {
    HAPPrecondition(HAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule_Protocol_IsValid(value));
    switch (value) {
        case kHAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule_Protocol_TCP:
            return "TCP";
        case kHAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule_Protocol_UDP:
            return "UDP";
        default:
            HAPFatalError();
    }
}

static const HAPCharacteristicTLVFormat_WiFiRouter_WANFirewall_PortRule_Protocol
        kHAPCharacteristicTLVFormat_WiFiRouter_WANFirewall_PortRule_Protocol = {
            .type = kHAPTLVFormatType_Enum,
            .callbacks = { .isValid = HAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule_Protocol_IsValid,
                           .getDescription =
                                   HAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule_Protocol_GetDescription }
        };

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_WiFiRouter_HostDNSNamePattern_IsValid(const char* value) {
    HAPPrecondition(value);

#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_ROUTER)
    if (!HAPWiFiRouterHostDNSNamePatternIsValid(value)) {
        return false;
    }
    return true;
#else
    return false;
#endif
}

static const HAPStringTLVFormat kHAPCharacteristicTLVFormat_WiFiRouter_HostDNSNamePattern = {
    .type = kHAPTLVFormatType_String,
    .constraints = { .minLength = 1, .maxLength = SIZE_MAX },
    .callbacks = { .isValid = HAPCharacteristicValue_WiFiRouter_HostDNSNamePattern_IsValid }
};

//----------------------------------------------------------------------------------------------------------------------

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_WiFiRouter_WANFirewall_PortRule[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule, transportProtocol),
            .tlvType = kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_PortRule_Protocol,
            .debugDescription = kHAPCharacteristicTLVDescription_WiFiRouter_WANFirewall_PortRule_Protocol,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_WANFirewall_PortRule_Protocol },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule, hostDNSName),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule, hostDNSNameIsSet),
            .tlvType = kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_PortRule_HostDNSName,
            .debugDescription = kHAPCharacteristicTLVDescription_WiFiRouter_WANFirewall_PortRule_HostDNSName,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_HostDNSNamePattern,
            .isOptional = true },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule, hostIPStart),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule, hostIPStartIsSet),
            .tlvType = kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_PortRule_HostIPStart,
            .debugDescription = kHAPCharacteristicTLVDescription_WiFiRouter_WANFirewall_PortRule_HostIPStart,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_IPAddress,
            .isOptional = true },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule, hostIPEnd),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule, hostIPEndIsSet),
            .tlvType = kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_PortRule_HostIPEnd,
            .debugDescription = kHAPCharacteristicTLVDescription_WiFiRouter_WANFirewall_PortRule_HostIPEnd,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_IPAddress,
            .isOptional = true },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule, hostPortStart),
            .tlvType = kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_PortRule_HostPortStart,
            .debugDescription = kHAPCharacteristicTLVDescription_WiFiRouter_WANFirewall_PortRule_HostPortStart,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_NetworkPort },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule, hostPortEnd),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule, hostPortEndIsSet),
            .tlvType = kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_PortRule_HostPortEnd,
            .debugDescription = kHAPCharacteristicTLVDescription_WiFiRouter_WANFirewall_PortRule_HostPortEnd,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_NetworkPort,
            .isOptional = true },
    NULL
};

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule_IsValid(
        HAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule* value) {
    HAPPrecondition(value);

    if (value->hostDNSNameIsSet) {
        if (value->hostIPStartIsSet || value->hostIPEndIsSet) {
            HAPLog(&logObject, "Host DNS Name and Host IP Address Start / End cannot be both specified.");
            return false;
        }
    } else if (value->hostIPStartIsSet) {
        bool hostIPStartIsWildcard;
        if (value->hostIPStart.ipv4AddressIsSet) {
            hostIPStartIsWildcard =
                    HAPIPv4AddressAreEqual(&value->hostIPStart.ipv4Address, &kHAPIPAddress_IPv4Any._.ipv4);
        } else {
            HAPAssert(value->hostIPStart.ipv6AddressIsSet);
            hostIPStartIsWildcard =
                    HAPIPv6AddressAreEqual(&value->hostIPStart.ipv6Address, &kHAPIPAddress_IPv4Any._.ipv6);
        }

        if (value->hostIPEndIsSet) {
            bool hostIPEndIsWildcard;
            if (value->hostIPEnd.ipv4AddressIsSet) {
                hostIPEndIsWildcard =
                        HAPIPv4AddressAreEqual(&value->hostIPEnd.ipv4Address, &kHAPIPAddress_IPv4Any._.ipv4);
            } else {
                HAPAssert(value->hostIPEnd.ipv6AddressIsSet);
                hostIPEndIsWildcard =
                        HAPIPv6AddressAreEqual(&value->hostIPEnd.ipv6Address, &kHAPIPAddress_IPv4Any._.ipv6);
            }
            if (hostIPStartIsWildcard || hostIPEndIsWildcard) {
                HAPLog(&logObject, "Host IP Address End not allowed when using wildcard IP addresses.");
                return false;
            }

            if (value->hostIPStart.ipv4AddressIsSet != value->hostIPEnd.ipv4AddressIsSet ||
                value->hostIPStart.ipv6AddressIsSet != value->hostIPEnd.ipv6AddressIsSet) {
                HAPLog(&logObject, "Host IP Address End must have same IP address version as Host IP Address Start.");
                return false;
            }

            if (value->hostIPStart.ipv4AddressIsSet) {
                HAPAssert(value->hostIPEnd.ipv4AddressIsSet);
                if (HAPIPv4AddressCompare(&value->hostIPStart.ipv4Address, &value->hostIPEnd.ipv4Address) !=
                    kHAPComparisonResult_OrderedAscending) {
                    HAPLog(&logObject, "Host IP Address End must be after Host IP Address Start.");
                    return false;
                }
            } else {
                HAPAssert(value->hostIPStart.ipv6AddressIsSet);
                HAPAssert(value->hostIPEnd.ipv6AddressIsSet);
                if (HAPIPv6AddressCompare(&value->hostIPStart.ipv6Address, &value->hostIPEnd.ipv6Address) !=
                    kHAPComparisonResult_OrderedAscending) {
                    HAPLog(&logObject, "Host IP Address End must be after Host IP Address Start.");
                    return false;
                }
            }
        }
    } else {
        HAPLog(&logObject, "Host DNS Name or Host IP Address Start must be specified.");
        return false;
    }

    if (value->hostPortEndIsSet) {
        if (value->hostPortStart == kHAPNetworkPort_Any) {
            HAPLog(&logObject, "Host Port End not allowed when Host Port Start set to any port.");
            return false;
        }
        if (value->hostPortEnd <= value->hostPortStart) {
            HAPLog(&logObject, "Host Port End must be greater than Host Port Start.");
            return false;
        }
    }

    return true;
}

static const HAPCharacteristicTLVFormat_WiFiRouter_WANFirewall_PortRule
        kHAPCharacteristicTLVFormat_WiFiRouter_WANFirewall_PortRule = {
            .type = kHAPTLVFormatType_Struct,
            .members = kHAPCharacteristicTLVMembers_WiFiRouter_WANFirewall_PortRule,
            .callbacks = { .isValid = HAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule_IsValid }
        };

//----------------------------------------------------------------------------------------------------------------------

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_WiFiRouter_WANFirewall_ICMPRule[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_WANFirewall_ICMPRule, hostDNSName),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_WANFirewall_ICMPRule, hostDNSNameIsSet),
            .tlvType = kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_ICMPRule_HostDNSName,
            .debugDescription = kHAPCharacteristicTLVDescription_WiFiRouter_WANFirewall_ICMPRule_HostDNSName,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_HostDNSNamePattern,
            .isOptional = true },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_WANFirewall_ICMPRule, hostIPStart),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_WANFirewall_ICMPRule, hostIPStartIsSet),
            .tlvType = kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_ICMPRule_HostIPStart,
            .debugDescription = kHAPCharacteristicTLVDescription_WiFiRouter_WANFirewall_ICMPRule_HostIPStart,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_IPAddress,
            .isOptional = true },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_WANFirewall_ICMPRule, hostIPEnd),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_WANFirewall_ICMPRule, hostIPEndIsSet),
            .tlvType = kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_ICMPRule_HostIPEnd,
            .debugDescription = kHAPCharacteristicTLVDescription_WiFiRouter_WANFirewall_ICMPRule_HostIPEnd,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_IPAddress,
            .isOptional = true },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_WANFirewall_ICMPRule, icmpList),
            .tlvType = kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_ICMPRule_ICMPList,
            .debugDescription = kHAPCharacteristicTLVDescription_WiFiRouter_WANFirewall_ICMPRule_ICMPList,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_ICMPList },
    NULL
};

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_WiFiRouter_WANFirewall_ICMPRule_IsValid(
        HAPCharacteristicValue_WiFiRouter_WANFirewall_ICMPRule* value) {
    HAPPrecondition(value);

    if (value->hostDNSNameIsSet) {
        if (value->hostIPStartIsSet || value->hostIPEndIsSet) {
            HAPLog(&logObject, "Host DNS Name and Host IP Address Start / End cannot be both specified.");
            return false;
        }
    } else if (value->hostIPStartIsSet) {
        bool hostIPStartIsWildcard;
        if (value->hostIPStart.ipv4AddressIsSet) {
            hostIPStartIsWildcard =
                    HAPIPv4AddressAreEqual(&value->hostIPStart.ipv4Address, &kHAPIPAddress_IPv4Any._.ipv4);
        } else {
            HAPAssert(value->hostIPStart.ipv6AddressIsSet);
            hostIPStartIsWildcard =
                    HAPIPv6AddressAreEqual(&value->hostIPStart.ipv6Address, &kHAPIPAddress_IPv4Any._.ipv6);
        }

        if (value->hostIPEndIsSet) {
            bool hostIPEndIsWildcard;
            if (value->hostIPEnd.ipv4AddressIsSet) {
                hostIPEndIsWildcard =
                        HAPIPv4AddressAreEqual(&value->hostIPEnd.ipv4Address, &kHAPIPAddress_IPv4Any._.ipv4);
            } else {
                HAPAssert(value->hostIPEnd.ipv6AddressIsSet);
                hostIPEndIsWildcard =
                        HAPIPv6AddressAreEqual(&value->hostIPEnd.ipv6Address, &kHAPIPAddress_IPv4Any._.ipv6);
            }
            if (hostIPStartIsWildcard || hostIPEndIsWildcard) {
                HAPLog(&logObject, "Host IP Address End not allowed when using wildcard IP addresses.");
                return false;
            }

            if (value->hostIPStart.ipv4AddressIsSet != value->hostIPEnd.ipv4AddressIsSet ||
                value->hostIPStart.ipv6AddressIsSet != value->hostIPEnd.ipv6AddressIsSet) {
                HAPLog(&logObject, "Host IP Address End must have same IP address version as Host IP Address Start.");
                return false;
            }

            if (value->hostIPStart.ipv4AddressIsSet) {
                HAPAssert(value->hostIPEnd.ipv4AddressIsSet);
                if (HAPIPv4AddressCompare(&value->hostIPStart.ipv4Address, &value->hostIPEnd.ipv4Address) !=
                    kHAPComparisonResult_OrderedAscending) {
                    HAPLog(&logObject, "Host IP Address End must be after Host IP Address Start.");
                    return false;
                }
            } else {
                HAPAssert(value->hostIPStart.ipv6AddressIsSet);
                HAPAssert(value->hostIPEnd.ipv6AddressIsSet);
                if (HAPIPv6AddressCompare(&value->hostIPStart.ipv6Address, &value->hostIPEnd.ipv6Address) !=
                    kHAPComparisonResult_OrderedAscending) {
                    HAPLog(&logObject, "Host IP Address End must be after Host IP Address Start.");
                    return false;
                }
            }
        }
    } else {
        HAPLog(&logObject, "Host DNS Name or Host IP Address Start must be specified.");
        return false;
    }

    return true;
}

static const HAPCharacteristicTLVFormat_WiFiRouter_WANFirewall_ICMPRule
        kHAPCharacteristicTLVFormat_WiFiRouter_WANFirewall_ICMPRule = {
            .type = kHAPTLVFormatType_Struct,
            .members = kHAPCharacteristicTLVMembers_WiFiRouter_WANFirewall_ICMPRule,
            .callbacks = { .isValid = HAPCharacteristicValue_WiFiRouter_WANFirewall_ICMPRule_IsValid }
        };

//----------------------------------------------------------------------------------------------------------------------

static const HAPUnionTLVVariant* const kHAPCharacteristicTLVVariants_WiFiRouter_WANFirewall_Rule[] = {
    &(const HAPUnionTLVVariant) { .tlvType = kHAPCharacteristicValueType_WiFiRouter_WANFirewall_Rule_Port,
                                  .debugDescription =
                                          kHAPCharacteristicValueDescription_WiFiRouter_WANFirewall_Rule_Port,
                                  .format = &kHAPCharacteristicTLVFormat_WiFiRouter_WANFirewall_PortRule },
    &(const HAPUnionTLVVariant) { .tlvType = kHAPCharacteristicValueType_WiFiRouter_WANFirewall_Rule_ICMP,
                                  .debugDescription =
                                          kHAPCharacteristicValueDescription_WiFiRouter_WANFirewall_Rule_ICMP,
                                  .format = &kHAPCharacteristicTLVFormat_WiFiRouter_WANFirewall_ICMPRule },
    NULL
};

static const HAPCharacteristicTLVFormat_WiFiRouter_WANFirewall_Rule
        kHAPCharacteristicTLVFormat_WiFiRouter_WANFirewall_Rule = {
            .type = kHAPTLVFormatType_Union,
            .untaggedValueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_WANFirewall_Rule, _),
            .variants = kHAPCharacteristicTLVVariants_WiFiRouter_WANFirewall_Rule
        };

//----------------------------------------------------------------------------------------------------------------------

static const HAPCharacteristicTLVFormat_WiFiRouter_WANFirewall_RuleList
        kHAPCharacteristicTLVFormat_WiFiRouter_WANFirewall_RuleList = {
            .type = kHAPTLVFormatType_Sequence,
            .item = { .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_WANFirewall_RuleList, _),
                      .format = &kHAPCharacteristicTLVFormat_WiFiRouter_WANFirewall_Rule,
                      .isFlat = true },
            .separator = { .tlvType = kHAPCharacteristicTLVType_Separator,
                           .debugDescription = kHAPCharacteristicTLVDescription_Separator,
                           .format = &kHAPCharacteristicTLVFormat_Separator }
        };

//----------------------------------------------------------------------------------------------------------------------

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_WiFiRouter_WANFirewall[] = {
    &(const HAPStructTLVMember) { .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_WANFirewall, type),
                                  .tlvType = kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_Type,
                                  .debugDescription = kHAPCharacteristicTLVDescription_WiFiRouter_WANFirewall_Type,
                                  .format = &kHAPCharacteristicTLVFormat_WiFiRouter_FirewallType },
    &(const HAPStructTLVMember) { .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_WANFirewall, ruleList),
                                  .isSetOffset =
                                          HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_WANFirewall, ruleListIsSet),
                                  .tlvType = kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_RuleList,
                                  .debugDescription = kHAPCharacteristicTLVDescription_WiFiRouter_WANFirewall_RuleList,
                                  .format = &kHAPCharacteristicTLVFormat_WiFiRouter_WANFirewall_RuleList,
                                  .isOptional = true },
    NULL
};

HAP_RESULT_USE_CHECK
static bool
        HAPCharacteristicValue_WiFiRouter_WANFirewall_IsValid(HAPCharacteristicValue_WiFiRouter_WANFirewall* value) {
    HAPPrecondition(value);

    switch (value->type) {
        case kHAPCharacteristicValue_WiFiRouter_FirewallType_FullAccess: {
        }
            return true;
        case kHAPCharacteristicValue_WiFiRouter_FirewallType_Allowlist: {
            if (!value->ruleListIsSet) {
                HAPLog(&logObject, "Rule List must be defined if Firewall Type is Allowlist.");
                return false;
            }
        }
            return true;
        default:
            HAPFatalError();
    }
}

static const HAPCharacteristicTLVFormat_WiFiRouter_WANFirewall kHAPCharacteristicTLVFormat_WiFiRouter_WANFirewall = {
    .type = kHAPTLVFormatType_Struct,
    .members = kHAPCharacteristicTLVMembers_WiFiRouter_WANFirewall,
    .callbacks = { .isValid = HAPCharacteristicValue_WiFiRouter_WANFirewall_IsValid }
};

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction_IsValid(uint8_t value) {
    HAPAssert(sizeof value == sizeof(HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction));
    switch ((HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction) value) {
        case kHAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction_Outbound:
        case kHAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction_Inbound: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
static const char* HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction_GetDescription(
        HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction value) {
    HAPPrecondition(HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction_IsValid(value));
    switch (value) {
        case kHAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction_Outbound:
            return "Outbound";
        case kHAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction_Inbound:
            return "Inbound";
        default:
            HAPFatalError();
    }
}

static const HAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_Rule_Direction
        kHAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_Rule_Direction = {
            .type = kHAPTLVFormatType_Enum,
            .callbacks = { .isValid = HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction_IsValid,
                           .getDescription =
                                   HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction_GetDescription }
        };

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
static const char* _Nullable HAPCharacteristicValue_WiFiRouter_GroupIdentifier_GetDescription(uint32_t value) {
    switch (value) {
        case kHAPCharacteristicValue_WiFiRouter_GroupIdentifier_Main:
            return "Main";
        case kHAPCharacteristicValue_WiFiRouter_GroupIdentifier_Restricted:
            return "Restricted";
        default: {
            return NULL;
        }
    }
}

static const HAPUInt32TLVFormat kHAPCharacteristicTLVFormat_WiFiRouter_GroupIdentifier = {
    .type = kHAPTLVFormatType_UInt32,
    .constraints = { .minimumValue = 1, .maximumValue = UINT32_MAX },
    .callbacks = { .getDescription = HAPCharacteristicValue_WiFiRouter_GroupIdentifier_GetDescription }
};

//----------------------------------------------------------------------------------------------------------------------

static const HAPCharacteristicRawTLVFormat_WiFiRouter_LANFirewall_Rule_EndpointList
        kHAPCharacteristicRawTLVFormat_WiFiRouter_LANFirewall_Rule_EndpointList = {
            .type = kHAPTLVFormatType_Sequence,
            .item = { .valueOffset =
                              HAP_OFFSETOF(HAPCharacteristicRawValue_WiFiRouter_LANFirewall_Rule_EndpointList, _),
                      .tlvType = kHAPCharacteristicRawTLVType_WiFiRouter_LANFirewall_Rule_EndpointList_Group,
                      .debugDescription =
                              kHAPCharacteristicRawTLVDescription_WiFiRouter_LANFirewall_Rule_EndpointList_Group,
                      .format = &kHAPCharacteristicTLVFormat_WiFiRouter_GroupIdentifier },
            .separator = { .tlvType = kHAPCharacteristicTLVType_Separator,
                           .debugDescription = kHAPCharacteristicTLVDescription_Separator,
                           .format = &kHAPCharacteristicTLVFormat_Separator }
        };

typedef struct {
    HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_EndpointList* value;
    HAPError err;
} HAPCharacteristicContext_WiFiRouter_LANFirewall_Rule_EndpointList;

static void HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_EndpointList_EnumerateCallback(
        void* _Nullable context_,
        uint32_t* value, // NOLINT(readability-non-const-parameter)
        bool* shouldContinue) {
    HAPPrecondition(context_);
    HAPCharacteristicContext_WiFiRouter_LANFirewall_Rule_EndpointList* context = context_;
    HAPPrecondition(context->value);
    HAPPrecondition(!context->err);
    HAPPrecondition(value);
    HAPPrecondition(shouldContinue);
    HAPPrecondition(*shouldContinue);

    if (context->value->numGroupIdentifiers >= HAPArrayCount(context->value->groupIdentifiers)) {
        HAPLog(&logObject,
               "Too many Client Group Identifiers specified (%zu maximum).",
               HAPArrayCount(context->value->groupIdentifiers));
        context->err = kHAPError_InvalidData;
        *shouldContinue = false;
        return;
    }

    context->value->groupIdentifiers[context->value->numGroupIdentifiers] = *value;
    context->value->numGroupIdentifiers++;
}

HAP_RESULT_USE_CHECK
static HAPError HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_EndpointList_Decode(
        HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_EndpointList* value,
        void* bytes,
        size_t numBytes) {
    HAPPrecondition(value);
    HAPRawBufferZero(value, sizeof *value);
    HAPPrecondition(bytes);

    HAPError err;

    HAPTLVReader reader;
    HAPTLVReaderCreate(&reader, bytes, numBytes);

    HAPCharacteristicRawValue_WiFiRouter_LANFirewall_Rule_EndpointList rawValue;
    err = HAPTLVReaderDecode(
            &reader, &kHAPCharacteristicRawTLVFormat_WiFiRouter_LANFirewall_Rule_EndpointList, &rawValue);
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }

    HAPCharacteristicContext_WiFiRouter_LANFirewall_Rule_EndpointList enumerateContext;
    HAPRawBufferZero(&enumerateContext, sizeof enumerateContext);
    enumerateContext.value = value;
    err = rawValue.enumerate(
            &rawValue.dataSource,
            HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_EndpointList_EnumerateCallback,
            &enumerateContext);
    if (!err) {
        err = enumerateContext.err;
    }
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }
    if (!value->numGroupIdentifiers) {
        HAPLog(&logObject, "No Client Group Identifiers specified.");
        return kHAPError_InvalidData;
    }

    return kHAPError_None;
}

typedef struct {
    HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_EndpointList* _Nullable value;
} HAPCharacteristicDataSource_WiFiRouter_LANFirewall_Rule_EndpointList;
HAP_STATIC_ASSERT(
        sizeof(HAPCharacteristicDataSource_WiFiRouter_LANFirewall_Rule_EndpointList) <=
                sizeof(HAPSequenceTLVDataSource),
        HAPCharacteristicDataSource_WiFiRouter_LANFirewall_Rule_EndpointList);

HAP_RESULT_USE_CHECK
static HAPError HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_EndpointList_Enumerate(
        HAPSequenceTLVDataSource* dataSource_,
        HAPCharacteristicValueCallback_WiFiRouter_LANFirewall_Rule_EndpointList_Group callback,
        void* _Nullable context) {
    HAPPrecondition(dataSource_);
    HAPCharacteristicDataSource_WiFiRouter_LANFirewall_Rule_EndpointList* dataSource =
            (HAPCharacteristicDataSource_WiFiRouter_LANFirewall_Rule_EndpointList*) dataSource_;
    HAPPrecondition(callback);

    if (!dataSource->value) {
        // Enumeration is no longer possible.
        return kHAPError_Unknown;
    }

    bool shouldContinue = true;
    for (size_t i = 0; shouldContinue && i < dataSource->value->numGroupIdentifiers; i++) {
        callback(context, &dataSource->value->groupIdentifiers[i], &shouldContinue);
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static HAPError HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_EndpointList_Encode(
        HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_EndpointList* value,
        void* bytes,
        size_t maxBytes,
        size_t* numBytes) {
    HAPPrecondition(value);
    HAPPrecondition(value->numGroupIdentifiers);
    HAPPrecondition(value->numGroupIdentifiers <= HAPArrayCount(value->groupIdentifiers));
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes);

    HAPError err;

    HAPCharacteristicRawValue_WiFiRouter_LANFirewall_Rule_EndpointList rawValue;
    HAPRawBufferZero(&rawValue, sizeof rawValue);
    HAPCharacteristicDataSource_WiFiRouter_LANFirewall_Rule_EndpointList* dataSource =
            (HAPCharacteristicDataSource_WiFiRouter_LANFirewall_Rule_EndpointList*) &rawValue.dataSource;
    dataSource->value = value;
    rawValue.enumerate = HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_EndpointList_Enumerate;

    HAPTLVWriter writer;
    HAPTLVWriterCreate(&writer, bytes, maxBytes);
    err = HAPTLVWriterEncode(
            &writer, &kHAPCharacteristicRawTLVFormat_WiFiRouter_LANFirewall_Rule_EndpointList, &rawValue);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                err == kHAPError_Busy);
        return err;
    }

    // Prevent enumeration after callback as it is no longer valid once we return.
    dataSource->value = NULL;

    void* tlvBytes;
    HAPTLVWriterGetBuffer(&writer, &tlvBytes, numBytes);
    HAPAssert(tlvBytes == bytes);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static HAPError HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_EndpointList_GetDescription(
        HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_EndpointList* value,
        char* bytes,
        size_t maxBytes) {
    HAPPrecondition(value);
    HAPPrecondition(value->numGroupIdentifiers);
    HAPPrecondition(value->numGroupIdentifiers <= HAPArrayCount(value->groupIdentifiers));
    HAPPrecondition(bytes);

    HAPStringBuilder stringBuilder;
    HAPStringBuilderCreate(&stringBuilder, bytes, maxBytes);
    HAPStringBuilderAppend(&stringBuilder, "[");
    for (size_t i = 0; i < value->numGroupIdentifiers; i++) {
        if (i) {
            HAPStringBuilderAppend(&stringBuilder, ", ");
        }
        HAPStringBuilderAppend(&stringBuilder, "%llu", (unsigned long long) value->groupIdentifiers[i]);
    }
    HAPStringBuilderAppend(&stringBuilder, "]");

    if (HAPStringBuilderDidOverflow(&stringBuilder)) {
        return kHAPError_OutOfResources;
    }
    return kHAPError_None;
}

static const HAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_Rule_EndpointList
        kHAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_Rule_EndpointList = {
            .type = kHAPTLVFormatType_Value,
            .callbacks = { .decode = HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_EndpointList_Decode,
                           .encode = HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_EndpointList_Encode,
                           .getDescription =
                                   HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_EndpointList_GetDescription }
        };

//----------------------------------------------------------------------------------------------------------------------

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_WiFiRouter_LANFirewall_MulticastBridgingRule[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_LANFirewall_MulticastBridgingRule, direction),
            .tlvType = kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_MulticastBridgingRule_Direction,
            .debugDescription = kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_MulticastBridgingRule_Direction,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_Rule_Direction },
    &(const HAPStructTLVMember) {
            .valueOffset =
                    HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_LANFirewall_MulticastBridgingRule, endpointList),
            .tlvType = kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_MulticastBridgingRule_EndpointList,
            .debugDescription =
                    kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_MulticastBridgingRule_EndpointList,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_Rule_EndpointList },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_LANFirewall_MulticastBridgingRule, ipAddress),
            .tlvType = kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_MulticastBridgingRule_IPAddress,
            .debugDescription = kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_MulticastBridgingRule_IPAddress,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_IPAddress },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_LANFirewall_MulticastBridgingRule, port),
            .tlvType = kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_MulticastBridgingRule_Port,
            .debugDescription = kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_MulticastBridgingRule_Port,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_NetworkPort },
    NULL
};

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_WiFiRouter_LANFirewall_MulticastBridgingRule_IsValid(
        HAPCharacteristicValue_WiFiRouter_LANFirewall_MulticastBridgingRule* value) {
    HAPPrecondition(value);

    if (value->ipAddress.ipv4AddressIsSet && value->ipAddress.ipv6AddressIsSet) {
        HAPLog(&logObject, "IPv4/IPv6 wildcard address only allowed in context of WAN firewall rules.");
        return false;
    }

    return true;
}

static const HAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_MulticastBridgingRule
        kHAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_MulticastBridgingRule = {
            .type = kHAPTLVFormatType_Struct,
            .members = kHAPCharacteristicTLVMembers_WiFiRouter_LANFirewall_MulticastBridgingRule,
            .callbacks = { .isValid = HAPCharacteristicValue_WiFiRouter_LANFirewall_MulticastBridgingRule_IsValid }
        };

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule_Protocol_IsValid(uint8_t value) {
    HAPAssert(sizeof value == sizeof(HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule_Protocol));
    switch ((HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule_Protocol) value) {
        case kHAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule_Protocol_TCP:
        case kHAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule_Protocol_UDP: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
static const char* HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule_Protocol_GetDescription(
        HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule_Protocol value) {
    HAPPrecondition(HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule_Protocol_IsValid(value));
    switch (value) {
        case kHAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule_Protocol_TCP:
            return "TCP";
        case kHAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule_Protocol_UDP:
            return "UDP";
        default:
            HAPFatalError();
    }
}

static const HAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_StaticPortRule_Protocol
        kHAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_StaticPortRule_Protocol = {
            .type = kHAPTLVFormatType_Enum,
            .callbacks = { .isValid = HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule_Protocol_IsValid,
                           .getDescription =
                                   HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule_Protocol_GetDescription }
        };

//----------------------------------------------------------------------------------------------------------------------

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_WiFiRouter_LANFirewall_StaticPortRule[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule, direction),
            .tlvType = kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_StaticPortRule_Direction,
            .debugDescription = kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_StaticPortRule_Direction,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_Rule_Direction },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule, endpointList),
            .tlvType = kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_StaticPortRule_EndpointList,
            .debugDescription = kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_StaticPortRule_EndpointList,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_Rule_EndpointList },
    &(const HAPStructTLVMember) {
            .valueOffset =
                    HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule, transportProtocol),
            .tlvType = kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_StaticPortRule_Protocol,
            .debugDescription = kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_StaticPortRule_Protocol,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_StaticPortRule_Protocol },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule, portStart),
            .tlvType = kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_StaticPortRule_PortStart,
            .debugDescription = kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_StaticPortRule_PortStart,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_NetworkPort },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule, portEnd),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule, portEndIsSet),
            .tlvType = kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_StaticPortRule_PortEnd,
            .debugDescription = kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_StaticPortRule_PortEnd,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_NetworkPort,
            .isOptional = true },
    NULL
};

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule_IsValid(
        HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule* value) {
    HAPPrecondition(value);

    if (value->portEndIsSet) {
        if (value->portStart == kHAPNetworkPort_Any) {
            HAPLog(&logObject, "Destination Port End not allowed when Destination Port Start set to any port.");
            return false;
        }
        if (value->portEnd <= value->portStart) {
            HAPLog(&logObject, "Destination Port End must be greater than Destination Port Start.");
            return false;
        }
    }

    return true;
}

static const HAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_StaticPortRule
        kHAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_StaticPortRule = {
            .type = kHAPTLVFormatType_Struct,
            .members = kHAPCharacteristicTLVMembers_WiFiRouter_LANFirewall_StaticPortRule,
            .callbacks = { .isValid = HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule_IsValid }
        };

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule_Protocol_IsValid(uint8_t value) {
    HAPAssert(sizeof value == sizeof(HAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule_Protocol));
    switch ((HAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule_Protocol) value) {
        case kHAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule_Protocol_TCP:
        case kHAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule_Protocol_UDP: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
static const char* HAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule_Protocol_GetDescription(
        HAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule_Protocol value) {
    HAPPrecondition(HAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule_Protocol_IsValid(value));
    switch (value) {
        case kHAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule_Protocol_TCP:
            return "TCP";
        case kHAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule_Protocol_UDP:
            return "UDP";
        default:
            HAPFatalError();
    }
}

static const HAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_DynamicPortRule_Protocol
        kHAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_DynamicPortRule_Protocol = {
            .type = kHAPTLVFormatType_Enum,
            .callbacks = { .isValid = HAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule_Protocol_IsValid,
                           .getDescription =
                                   HAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule_Protocol_GetDescription }
        };

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_WiFiRouter_AdvertisementProtocol_IsValid(uint8_t value) {
    HAPAssert(sizeof value == sizeof(HAPCharacteristicValue_WiFiRouter_AdvertisementProtocol));
    switch ((HAPCharacteristicValue_WiFiRouter_AdvertisementProtocol) value) {
        case kHAPCharacteristicValue_WiFiRouter_AdvertisementProtocol_DNSSD:
        case kHAPCharacteristicValue_WiFiRouter_AdvertisementProtocol_SSDP: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
static const char* HAPCharacteristicValue_WiFiRouter_AdvertisementProtocol_GetDescription(
        HAPCharacteristicValue_WiFiRouter_AdvertisementProtocol value) {
    HAPPrecondition(HAPCharacteristicValue_WiFiRouter_AdvertisementProtocol_IsValid(value));
    switch (value) {
        case kHAPCharacteristicValue_WiFiRouter_AdvertisementProtocol_DNSSD:
            return "DNS-SD";
        case kHAPCharacteristicValue_WiFiRouter_AdvertisementProtocol_SSDP:
            return "SSDP";
        default:
            HAPFatalError();
    }
}

static const HAPCharacteristicTLVFormat_WiFiRouter_AdvertisementProtocol
        kHAPCharacteristicTLVFormat_WiFiRouter_AdvertisementProtocol = {
            .type = kHAPTLVFormatType_Enum,
            .callbacks = { .isValid = HAPCharacteristicValue_WiFiRouter_AdvertisementProtocol_IsValid,
                           .getDescription = HAPCharacteristicValue_WiFiRouter_AdvertisementProtocol_GetDescription }
        };

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
static const char* _Nullable HAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule_Flags_GetBitDescription(
        uint32_t optionValue) {
    switch (optionValue) { // NOLINT(hicpp-multiway-paths-covered)
        case kHAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule_Flags_AdvertisementOnly:
            return "Advertisement only";
        default:
            return NULL;
    }
}

static const HAPUInt32TLVFormat
    kHAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_DynamicPortRule_Flags =
{
    .type = kHAPTLVFormatType_UInt32,
    .constraints = {
        .minimumValue = 0,
        .maximumValue = UINT32_MAX,
    },
    .callbacks = {
        .getBitDescription = HAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule_Flags_GetBitDescription
    }
};

//----------------------------------------------------------------------------------------------------------------------

static const HAPStringTLVFormat kHAPCharacteristicTLVFormat_WiFiRouter_ServiceType_Name = {
    .type = kHAPTLVFormatType_String,
    .constraints = { .minLength = 0, .maxLength = SIZE_MAX }
};

//----------------------------------------------------------------------------------------------------------------------

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_WiFiRouter_ServiceType_Name[] = {
    &(const HAPStructTLVMember) { .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_ServiceType, name),
                                  .tlvType = kHAPCharacteristicTLVType_WiFiRouter_ServiceType_Name,
                                  .debugDescription = kHAPCharacteristicTLVDescription_WiFiRouter_ServiceType_Name,
                                  .format = &kHAPCharacteristicTLVFormat_WiFiRouter_ServiceType_Name },
    NULL
};

static const HAPCharacteristicTLVFormat_WiFiRouter_ServiceType kHAPCharacteristicTLVFormat_WiFiRouter_ServiceType = {
    .type = kHAPTLVFormatType_Struct,
    .members = kHAPCharacteristicTLVMembers_WiFiRouter_ServiceType_Name,
};

//----------------------------------------------------------------------------------------------------------------------

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_WiFiRouter_LANFirewall_DynamicPortRule[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule, direction),
            .tlvType = kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_DynamicPortRule_Direction,
            .debugDescription = kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_DynamicPortRule_Direction,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_Rule_Direction },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule, endpointList),
            .tlvType = kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_DynamicPortRule_EndpointList,
            .debugDescription = kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_DynamicPortRule_EndpointList,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_Rule_EndpointList },
    &(const HAPStructTLVMember) {
            .valueOffset =
                    HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule, transportProtocol),
            .tlvType = kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_DynamicPortRule_Protocol,
            .debugDescription = kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_DynamicPortRule_Protocol,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_DynamicPortRule_Protocol },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule, advertProtocol),
            .tlvType = kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_DynamicPortRule_AdvertProtocol,
            .debugDescription = kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_DynamicPortRule_AdvertProtocol,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_AdvertisementProtocol },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule, flags),
            .tlvType = kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_DynamicPortRule_Flags,
            .debugDescription = kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_DynamicPortRule_Flags,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_DynamicPortRule_Flags },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule, service),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule, serviceIsSet),
            .tlvType = kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_DynamicPortRule_Service,
            .debugDescription = kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_DynamicPortRule_Service,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_ServiceType,
            .isOptional = true },
    NULL
};

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule_IsValid(
        HAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule* value) {
    HAPPrecondition(value);

    if (value->serviceIsSet) {
        switch (value->advertProtocol) {
            case kHAPCharacteristicValue_WiFiRouter_AdvertisementProtocol_DNSSD: {
                size_t numNameBytes = HAPStringGetNumBytes(value->service.name);
                if (numNameBytes > 255) {
                    HAPLog(&logObject, "DNS-SD Service Type Name has invalid length: %zu.", numNameBytes);
                    return false;
                }
                break;
            }
            case kHAPCharacteristicValue_WiFiRouter_AdvertisementProtocol_SSDP: {
                break;
            }
        }
    }

    return true;
}

static const HAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_DynamicPortRule
        kHAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_DynamicPortRule = {
            .type = kHAPTLVFormatType_Struct,
            .members = kHAPCharacteristicTLVMembers_WiFiRouter_LANFirewall_DynamicPortRule,
            .callbacks = { .isValid = HAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule_IsValid }
        };

//----------------------------------------------------------------------------------------------------------------------

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_WiFiRouter_LANFirewall_StaticICMPRule[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticICMPRule, direction),
            .tlvType = kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_StaticICMPRule_Direction,
            .debugDescription = kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_StaticICMPRule_Direction,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_Rule_Direction },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticICMPRule, endpointList),
            .tlvType = kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_StaticICMPRule_EndpointList,
            .debugDescription = kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_StaticICMPRule_EndpointList,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_Rule_EndpointList },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticICMPRule, icmpList),
            .tlvType = kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_StaticICMPRule_ICMPList,
            .debugDescription = kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_StaticICMPRule_ICMPList,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_ICMPList },
    NULL
};

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticICMPRule_IsValid(
        HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticICMPRule* value) {
    HAPPrecondition(value);

    return true;
}

static const HAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_StaticICMPRule
        kHAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_StaticICMPRule = {
            .type = kHAPTLVFormatType_Struct,
            .members = kHAPCharacteristicTLVMembers_WiFiRouter_LANFirewall_StaticICMPRule,
            .callbacks = { .isValid = HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticICMPRule_IsValid }
        };

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

static const HAPUnionTLVVariant* const kHAPCharacteristicTLVVariants_WiFiRouter_LANFirewall_Rule[] = {
    &(const HAPUnionTLVVariant) {
            .tlvType = kHAPCharacteristicValueType_WiFiRouter_LANFirewall_Rule_MulticastBridging,
            .debugDescription = kHAPCharacteristicValueDescription_WiFiRouter_LANFirewall_Rule_MulticastBridging,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_MulticastBridgingRule },
    &(const HAPUnionTLVVariant) { .tlvType = kHAPCharacteristicValueType_WiFiRouter_LANFirewall_Rule_StaticPort,
                                  .debugDescription =
                                          kHAPCharacteristicValueDescription_WiFiRouter_LANFirewall_Rule_StaticPort,
                                  .format = &kHAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_StaticPortRule },
    &(const HAPUnionTLVVariant) { .tlvType = kHAPCharacteristicValueType_WiFiRouter_LANFirewall_Rule_DynamicPort,
                                  .debugDescription =
                                          kHAPCharacteristicValueDescription_WiFiRouter_LANFirewall_Rule_DynamicPort,
                                  .format = &kHAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_DynamicPortRule },
    &(const HAPUnionTLVVariant) { .tlvType = kHAPCharacteristicValueType_WiFiRouter_LANFirewall_Rule_StaticICMP,
                                  .debugDescription =
                                          kHAPCharacteristicValueDescription_WiFiRouter_LANFirewall_Rule_StaticICMP,
                                  .format = &kHAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_StaticICMPRule },
    NULL
};

static const HAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_Rule
        kHAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_Rule = {
            .type = kHAPTLVFormatType_Union,
            .untaggedValueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule, _),
            .variants = kHAPCharacteristicTLVVariants_WiFiRouter_LANFirewall_Rule
        };

//----------------------------------------------------------------------------------------------------------------------

static const HAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_RuleList
        kHAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_RuleList = {
            .type = kHAPTLVFormatType_Sequence,
            .item = { .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_LANFirewall_RuleList, _),
                      .format = &kHAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_Rule,
                      .isFlat = true },
            .separator = { .tlvType = kHAPCharacteristicTLVType_Separator,
                           .debugDescription = kHAPCharacteristicTLVDescription_Separator,
                           .format = &kHAPCharacteristicTLVFormat_Separator }
        };

//----------------------------------------------------------------------------------------------------------------------

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_WiFiRouter_LANFirewall[] = {
    &(const HAPStructTLVMember) { .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_LANFirewall, type),
                                  .tlvType = kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_Type,
                                  .debugDescription = kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_Type,
                                  .format = &kHAPCharacteristicTLVFormat_WiFiRouter_FirewallType },
    &(const HAPStructTLVMember) { .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_LANFirewall, ruleList),
                                  .isSetOffset =
                                          HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_LANFirewall, ruleListIsSet),
                                  .tlvType = kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_RuleList,
                                  .debugDescription = kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_RuleList,
                                  .format = &kHAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_RuleList,
                                  .isOptional = true },
    NULL
};

HAP_RESULT_USE_CHECK
static bool
        HAPCharacteristicValue_WiFiRouter_LANFirewall_IsValid(HAPCharacteristicValue_WiFiRouter_LANFirewall* value) {
    HAPPrecondition(value);

    switch (value->type) {
        case kHAPCharacteristicValue_WiFiRouter_FirewallType_FullAccess: {
        }
            return true;
        case kHAPCharacteristicValue_WiFiRouter_FirewallType_Allowlist: {
            if (!value->ruleListIsSet) {
                HAPLog(&logObject, "Rule List must be defined if Firewall Type is Allowlist.");
                return false;
            }
        }
            return true;
        default:
            HAPFatalError();
    }
}

static const HAPCharacteristicTLVFormat_WiFiRouter_LANFirewall kHAPCharacteristicTLVFormat_WiFiRouter_LANFirewall = {
    .type = kHAPTLVFormatType_Struct,
    .members = kHAPCharacteristicTLVMembers_WiFiRouter_LANFirewall,
    .callbacks = { .isValid = HAPCharacteristicValue_WiFiRouter_LANFirewall_IsValid }
};

//----------------------------------------------------------------------------------------------------------------------

static const HAPUInt32TLVFormat kHAPCharacteristicTLVFormat_WiFiRouter_ClientIdentifier = {
    .type = kHAPTLVFormatType_UInt32,
    .constraints = { .minimumValue = 1, .maximumValue = UINT32_MAX }
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_NetworkClientProfileControl_Config[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkClientProfileControl_Config, clientIdentifier),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkClientProfileControl_Config, clientIsSet),
            .tlvType = kHAPCharacteristicTLVType_NetworkClientProfileControl_Config_Identifier,
            .debugDescription = kHAPCharacteristicTLVDescription_NetworkClientProfileControl_Config_Identifier,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_ClientIdentifier,
            .isOptional = true },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkClientProfileControl_Config, groupIdentifier),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkClientProfileControl_Config, groupIsSet),
            .tlvType = kHAPCharacteristicTLVType_NetworkClientProfileControl_Config_Group,
            .debugDescription = kHAPCharacteristicTLVDescription_NetworkClientProfileControl_Config_Group,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_GroupIdentifier,
            .isOptional = true },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkClientProfileControl_Config, credential),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkClientProfileControl_Config, credentialIsSet),
            .tlvType = kHAPCharacteristicTLVType_NetworkClientProfileControl_Config_Credential,
            .debugDescription = kHAPCharacteristicTLVDescription_NetworkClientProfileControl_Config_Credential,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_Credential,
            .isOptional = true },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkClientProfileControl_Config, wanFirewall),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkClientProfileControl_Config, wanFirewallIsSet),
            .tlvType = kHAPCharacteristicTLVType_NetworkClientProfileControl_Config_WANFirewall,
            .debugDescription = kHAPCharacteristicTLVDescription_NetworkClientProfileControl_Config_WANFirewall,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_WANFirewall,
            .isOptional = true },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkClientProfileControl_Config, lanFirewall),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkClientProfileControl_Config, lanFirewallIsSet),
            .tlvType = kHAPCharacteristicTLVType_NetworkClientProfileControl_Config_LANFirewall,
            .debugDescription = kHAPCharacteristicTLVDescription_NetworkClientProfileControl_Config_LANFirewall,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_LANFirewall,
            .isOptional = true },
    NULL
};

static const HAPCharacteristicTLVFormat_NetworkClientProfileControl_Config
        kHAPCharacteristicTLVFormat_NetworkClientProfileControl_Config = {
            .type = kHAPTLVFormatType_Struct,
            .members = kHAPCharacteristicTLVMembers_NetworkClientProfileControl_Config
        };

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_IsValid(uint8_t value) {
    HAPAssert(sizeof value == sizeof(HAPCharacteristicValue_NetworkClientProfileControl_Operation_Type));
    switch ((HAPCharacteristicValue_NetworkClientProfileControl_Operation_Type) value) {
        case kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_List:
        case kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_Read:
        case kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_Add:
        case kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_Remove:
        case kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_Update: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
static const char* HAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_GetDescription(
        HAPCharacteristicValue_NetworkClientProfileControl_Operation_Type value) {
    HAPPrecondition(HAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_IsValid(value));
    switch (value) {
        case kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_List:
            return "List";
        case kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_Read:
            return "Read";
        case kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_Add:
            return "Add";
        case kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_Remove:
            return "Remove";
        case kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_Update:
            return "Update";
        default:
            HAPFatalError();
    }
}

static const HAPCharacteristicTLVFormat_NetworkClientProfileControl_Operation_Type
        kHAPCharacteristicTLVFormat_NetworkClientProfileControl_Operation_Type = {
            .type = kHAPTLVFormatType_Enum,
            .callbacks = { .isValid = HAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_IsValid,
                           .getDescription =
                                   HAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_GetDescription }
        };

//----------------------------------------------------------------------------------------------------------------------

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_NetworkClientProfileControl_Operation[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkClientProfileControl_Operation, operationType),
            .tlvType = kHAPCharacteristicTLVType_NetworkClientProfileControl_Operation_Type,
            .debugDescription = kHAPCharacteristicTLVDescription_NetworkClientProfileControl_Operation_Type,
            .format = &kHAPCharacteristicTLVFormat_NetworkClientProfileControl_Operation_Type },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkClientProfileControl_Operation, config),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkClientProfileControl_Operation, configIsSet),
            .tlvType = kHAPCharacteristicTLVType_NetworkClientProfileControl_Operation_Config,
            .debugDescription = kHAPCharacteristicTLVDescription_NetworkClientProfileControl_Operation_Config,
            .format = &kHAPCharacteristicTLVFormat_NetworkClientProfileControl_Config,
            .isOptional = true },
    NULL
};

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_NetworkClientProfileControl_Operation_IsValid(
        HAPCharacteristicValue_NetworkClientProfileControl_Operation* value) {
    HAPPrecondition(value);

    switch (value->operationType) {
        case kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_List: {
            // The Network Client Profile Configuration in the request is not required.
        }
            return true;
        case kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_Read:
        case kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_Remove: {
            if (!value->configIsSet) {
                HAPLog(&logObject, "Network Client Profile Configuration is required.");
                return false;
            }
            if (!value->config.clientIsSet) {
                HAPLog(&logObject, "Network Client Profile Identifier is required.");
                return false;
            }
            // All other TLV items in the request are ignored.
        }
            return true;
        case kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_Update: {
            if (!value->configIsSet) {
                HAPLog(&logObject, "Network Client Profile Configuration is required.");
                return false;
            }
            if (!value->config.clientIsSet) {
                HAPLog(&logObject, "Network Client Profile Identifier is required.");
                return false;
            }
            if (value->config.credentialIsSet) {
                switch (value->config.credential.type) {
                    case kHAPCharacteristicValueType_WiFiRouter_Credential_MACAddress: {
                        break;
                    }
                    case kHAPCharacteristicValueType_WiFiRouter_Credential_PSK: {
                        if (HAPStringAreEqual(value->config.credential._.psk, "")) {
                            HAPLog(&logObject, "PSK Credential must be non-empty.");
                            return false;
                        }
                        break;
                    }
                }
            }
            // All other TLV items in the request are optional.
        }
            return true;
        case kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_Add: {
            if (!value->configIsSet) {
                HAPLog(&logObject, "Network Client Profile Configuration is required.");
                return false;
            }
            if (value->config.clientIsSet) {
                // See HomeKit Accessory Protocol Specification R17
                // Section 11.142 Network Client Profile Control
                HAPLog(&logObject, "The Network Client Profile Identifier in the request must not be defined.");
                return false;
            }
            if (!value->config.groupIsSet) {
                HAPLog(&logObject, "Client Group Identifier is required.");
                return false;
            }
            if (!value->config.credentialIsSet) {
                HAPLog(&logObject, "Credential Data is required.");
                return false;
            }
            switch (value->config.credential.type) {
                case kHAPCharacteristicValueType_WiFiRouter_Credential_MACAddress: {
                    break;
                }
                case kHAPCharacteristicValueType_WiFiRouter_Credential_PSK: {
                    if (HAPStringAreEqual(value->config.credential._.psk, "")) {
                        HAPLog(&logObject, "PSK Credential must be non-empty.");
                        return false;
                    }
                    break;
                }
            }
            if (!value->config.wanFirewallIsSet) {
                HAPLog(&logObject, "WAN Firewall Configuration is required.");
                return false;
            }
            if (!value->config.lanFirewallIsSet) {
                HAPLog(&logObject, "LAN Firewall Configuration is required.");
                return false;
            }
        }
            return true;
        default:
            HAPFatalError();
    }
}

static const HAPCharacteristicTLVFormat_NetworkClientProfileControl_Operation
        kHAPCharacteristicTLVFormat_NetworkClientProfileControl_Operation = {
            .type = kHAPTLVFormatType_Struct,
            .members = kHAPCharacteristicTLVMembers_NetworkClientProfileControl_Operation,
            .callbacks = { .isValid = HAPCharacteristicValue_NetworkClientProfileControl_Operation_IsValid }
        };

//----------------------------------------------------------------------------------------------------------------------

const HAPCharacteristicTLVFormat_NetworkClientProfileControl kHAPCharacteristicTLVFormat_NetworkClientProfileControl = {
    .type = kHAPTLVFormatType_Sequence,
    .item = { .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkClientProfileControl, _),
              .tlvType = kHAPCharacteristicTLVType_NetworkClientProfileControl_Operation,
              .debugDescription = kHAPCharacteristicTLVDescription_NetworkClientProfileControl_Operation,
              .format = &kHAPCharacteristicTLVFormat_NetworkClientProfileControl_Operation },
    .separator = { .tlvType = kHAPCharacteristicTLVType_Separator,
                   .debugDescription = kHAPCharacteristicTLVDescription_Separator,
                   .format = &kHAPCharacteristicTLVFormat_Separator }
};

//----------------------------------------------------------------------------------------------------------------------

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_NetworkClientProfileControl_OperationRsp[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkClientProfileControl_OperationRsp, status),
            .tlvType = kHAPCharacteristicTLVType_NetworkClientProfileControl_OperationRsp_Status,
            .debugDescription = kHAPCharacteristicTLVDescription_NetworkClientProfileControl_OperationRsp_Status,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_OperationStatus },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkClientProfileControl_OperationRsp, config),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkClientProfileControl_OperationRsp, configIsSet),
            .tlvType = kHAPCharacteristicTLVType_NetworkClientProfileControl_OperationRsp_Config,
            .debugDescription = kHAPCharacteristicTLVDescription_NetworkClientProfileControl_OperationRsp_Config,
            .format = &kHAPCharacteristicTLVFormat_NetworkClientProfileControl_Config,
            .isOptional = true },
    NULL
};

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_NetworkClientProfileControl_OperationRsp_IsValid(
        HAPCharacteristicValue_NetworkClientProfileControl_OperationRsp* value) {
    HAPPrecondition(value);

    if (value->configIsSet) {
        if (value->config.credentialIsSet) {
            switch (value->config.credential.type) {
                case kHAPCharacteristicValueType_WiFiRouter_Credential_MACAddress: {
                    break;
                }
                case kHAPCharacteristicValueType_WiFiRouter_Credential_PSK: {
                    if (!HAPStringAreEqual(value->config.credential._.psk, "")) {
                        HAPLog(&logObject, "PSK Credential must be masked by replacing it with an empty buffer.");
                        return false;
                    }
                    break;
                }
            }
        }
    }

    return true;
}

static const HAPCharacteristicTLVFormat_NetworkClientProfileControl_OperationRsp
        kHAPCharacteristicTLVFormat_NetworkClientProfileControl_OperationRsp = {
            .type = kHAPTLVFormatType_Struct,
            .members = kHAPCharacteristicTLVMembers_NetworkClientProfileControl_OperationRsp,
            .callbacks = { .isValid = HAPCharacteristicValue_NetworkClientProfileControl_OperationRsp_IsValid }
        };

//----------------------------------------------------------------------------------------------------------------------

const HAPCharacteristicTLVFormat_NetworkClientProfileControl_Response
        kHAPCharacteristicTLVFormat_NetworkClientProfileControl_Response = {
            .type = kHAPTLVFormatType_Sequence,
            .item = { .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkClientProfileControl_Response, _),
                      .tlvType = kHAPCharacteristicTLVType_NetworkClientProfileControl_Response_OperationRsp,
                      .debugDescription =
                              kHAPCharacteristicTLVDescription_NetworkClientProfileControl_Response_OperationRsp,
                      .format = &kHAPCharacteristicTLVFormat_NetworkClientProfileControl_OperationRsp },
            .separator = { .tlvType = kHAPCharacteristicTLVType_Separator,
                           .debugDescription = kHAPCharacteristicTLVDescription_Separator,
                           .format = &kHAPCharacteristicTLVFormat_Separator }
        };

//----------------------------------------------------------------------------------------------------------------------

static const HAPCharacteristicTLVFormat_WiFiRouter_ClientList kHAPCharacteristicTLVFormat_WiFiRouter_ClientList = {
    .type = kHAPTLVFormatType_Sequence,
    .item = { .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiRouter_ClientList, _),
              .tlvType = kHAPCharacteristicTLVType_WiFiRouter_ClientList_Identifier,
              .debugDescription = kHAPCharacteristicTLVDescription_WiFiRouter_ClientList_Identifier,
              .format = &kHAPCharacteristicTLVFormat_WiFiRouter_ClientIdentifier },
    .separator = { .tlvType = kHAPCharacteristicTLVType_Separator,
                   .debugDescription = kHAPCharacteristicTLVDescription_Separator,
                   .format = &kHAPCharacteristicTLVFormat_Separator }
};

//----------------------------------------------------------------------------------------------------------------------

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_NetworkClientProfileControl_Event[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkClientProfileControl_Event, clientList),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkClientProfileControl_Event, clientListIsSet),
            .tlvType = kHAPCharacteristicTLVType_NetworkClientProfileControl_Event_ClientList,
            .debugDescription = kHAPCharacteristicTLVDescription_NetworkClientProfileControl_Event_ClientList,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_ClientList,
            .isOptional = true },
    NULL
};

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_NetworkClientProfileControl_Event_IsValid(
        HAPCharacteristicValue_NetworkClientProfileControl_Event* value) {
    HAPPrecondition(value);

    return true;
}

const HAPCharacteristicTLVFormat_NetworkClientProfileControl_Event
        kHAPCharacteristicTLVFormat_NetworkClientProfileControl_Event = {
            .type = kHAPTLVFormatType_Struct,
            .members = kHAPCharacteristicTLVMembers_NetworkClientProfileControl_Event,
            .callbacks = { .isValid = HAPCharacteristicValue_NetworkClientProfileControl_Event_IsValid }
        };

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const HAPUUID kHAPCharacteristicType_NetworkClientStatusControl = HAPUUIDCreateAppleDefined(0x20DU);

//----------------------------------------------------------------------------------------------------------------------

static const HAPUnionTLVVariant* const kHAPCharacteristicTLVVariants_NetworkClientStatusControl_Identifier[] = {
    &(const HAPUnionTLVVariant) {
            .tlvType = kHAPCharacteristicValueType_NetworkClientStatusControl_Identifier_Client,
            .debugDescription = kHAPCharacteristicValueDescription_NetworkClientStatusControl_Identifier_Client,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_ClientIdentifier },
    &(const HAPUnionTLVVariant) {
            .tlvType = kHAPCharacteristicValueType_NetworkClientStatusControl_Identifier_MACAddress,
            .debugDescription = kHAPCharacteristicValueDescription_NetworkClientStatusControl_Identifier_MACAddress,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_MACAddress },
    &(const HAPUnionTLVVariant) {
            .tlvType = kHAPCharacteristicValueType_NetworkClientStatusControl_Identifier_IPAddress,
            .debugDescription = kHAPCharacteristicValueDescription_NetworkClientStatusControl_Identifier_IPAddress,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_IPAddress },
    NULL
};

static const HAPCharacteristicTLVFormat_NetworkClientStatusControl_Identifier
        kHAPCharacteristicTLVFormat_NetworkClientStatusControl_Identifier = {
            .type = kHAPTLVFormatType_Union,
            .untaggedValueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkClientStatusControl_Identifier, _),
            .variants = kHAPCharacteristicTLVVariants_NetworkClientStatusControl_Identifier
        };

//----------------------------------------------------------------------------------------------------------------------

static const HAPCharacteristicTLVFormat_NetworkClientStatusControl_IdentifierList
        kHAPCharacteristicTLVFormat_NetworkClientStatusControl_IdentifierList = {
            .type = kHAPTLVFormatType_Sequence,
            .item = { .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkClientStatusControl_IdentifierList, _),
                      .tlvType = kHAPCharacteristicTLVType_NetworkClientStatusControl_IdentifierList_Identifier,
                      .debugDescription =
                              kHAPCharacteristicTLVDescription_NetworkClientStatusControl_IdentifierList_Identifier,
                      .format = &kHAPCharacteristicTLVFormat_NetworkClientStatusControl_Identifier },
            .separator = { .tlvType = kHAPCharacteristicTLVType_Separator,
                           .debugDescription = kHAPCharacteristicTLVDescription_Separator,
                           .format = &kHAPCharacteristicTLVFormat_Separator }
        };

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_NetworkClientStatusControl_OperationType_IsValid(uint8_t value) {
    HAPAssert(sizeof value == sizeof(HAPCharacteristicValue_NetworkClientStatusControl_OperationType));
    switch ((HAPCharacteristicValue_NetworkClientStatusControl_OperationType) value) {
        case kHAPCharacteristicValue_NetworkClientStatusControl_OperationType_Read: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
static const char* HAPCharacteristicValue_NetworkClientStatusControl_OperationType_GetDescription(
        HAPCharacteristicValue_NetworkClientStatusControl_OperationType value) {
    HAPPrecondition(HAPCharacteristicValue_NetworkClientStatusControl_OperationType_IsValid(value));
    switch (value) { // NOLINT(hicpp-multiway-paths-covered)
        case kHAPCharacteristicValue_NetworkClientStatusControl_OperationType_Read:
            return "Read network client status";
        default:
            HAPFatalError();
    }
}

static const HAPCharacteristicTLVFormat_NetworkClientStatusControl_OperationType
        kHAPCharacteristicTLVFormat_NetworkClientStatusControl_OperationType = {
            .type = kHAPTLVFormatType_Enum,
            .callbacks = { .isValid = HAPCharacteristicValue_NetworkClientStatusControl_OperationType_IsValid,
                           .getDescription =
                                   HAPCharacteristicValue_NetworkClientStatusControl_OperationType_GetDescription }
        };

//----------------------------------------------------------------------------------------------------------------------

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_NetworkClientStatusControl[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkClientStatusControl, operationType),
            .tlvType = kHAPCharacteristicTLVType_NetworkClientStatusControl_OperationType,
            .debugDescription = kHAPCharacteristicTLVDescription_NetworkClientStatusControl_OperationType,
            .format = &kHAPCharacteristicTLVFormat_NetworkClientStatusControl_OperationType },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkClientStatusControl, identifierList),
            .tlvType = kHAPCharacteristicTLVType_NetworkClientStatusControl_IdentifierList,
            .debugDescription = kHAPCharacteristicTLVDescription_NetworkClientStatusControl_IdentifierList,
            .format = &kHAPCharacteristicTLVFormat_NetworkClientStatusControl_IdentifierList },
    NULL
};

const HAPCharacteristicTLVFormat_NetworkClientStatusControl kHAPCharacteristicTLVFormat_NetworkClientStatusControl = {
    .type = kHAPTLVFormatType_Struct,
    .members = kHAPCharacteristicTLVMembers_NetworkClientStatusControl
};

//----------------------------------------------------------------------------------------------------------------------

static const HAPCharacteristicTLVFormat_NetworkClientStatusControl_IPAddressList
        kHAPCharacteristicTLVFormat_NetworkClientStatusControl_IPAddressList = {
            .type = kHAPTLVFormatType_Sequence,
            .item = { .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkClientStatusControl_IPAddressList, _),
                      .tlvType = kHAPCharacteristicTLVType_NetworkClientStatusControl_IPAddressList_IPAddress,
                      .debugDescription =
                              kHAPCharacteristicTLVDescription_NetworkClientStatusControl_IPAddressList_IPAddress,
                      .format = &kHAPCharacteristicTLVFormat_WiFiRouter_IPAddress },
            .separator = { .tlvType = kHAPCharacteristicTLVType_Separator,
                           .debugDescription = kHAPCharacteristicTLVDescription_Separator,
                           .format = &kHAPCharacteristicTLVFormat_Separator }
        };

//----------------------------------------------------------------------------------------------------------------------

static const HAPStringTLVFormat kHAPCharacteristicTLVFormat_NetworkClientStatusControl_Status_Name = {
    .type = kHAPTLVFormatType_String,
    .constraints = { .minLength = 0, .maxLength = SIZE_MAX }
};

//----------------------------------------------------------------------------------------------------------------------

static const HAPInt32TLVFormat kHAPCharacteristicTLVFormat_WiFiRouter_RSSI = {
    .type = kHAPTLVFormatType_Int32,
    .constraints = { .minimumValue = INT32_MIN, .maximumValue = INT32_MAX }
};

//----------------------------------------------------------------------------------------------------------------------

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_NetworkClientStatusControl_Status[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkClientStatusControl_Status, clientIdentifier),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkClientStatusControl_Status, clientIsSet),
            .tlvType = kHAPCharacteristicTLVType_NetworkClientStatusControl_Status_Client,
            .debugDescription = kHAPCharacteristicTLVDescription_NetworkClientStatusControl_Status_Client,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_ClientIdentifier,
            .isOptional = true },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkClientStatusControl_Status, macAddress),
            .tlvType = kHAPCharacteristicTLVType_NetworkClientStatusControl_Status_MACAddress,
            .debugDescription = kHAPCharacteristicTLVDescription_NetworkClientStatusControl_Status_MACAddress,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_MACAddress },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkClientStatusControl_Status, ipAddressList),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkClientStatusControl_Status, ipAddressListIsSet),
            .tlvType = kHAPCharacteristicTLVType_NetworkClientStatusControl_Status_IPAddressList,
            .debugDescription = kHAPCharacteristicTLVDescription_NetworkClientStatusControl_IPAddressList,
            .format = &kHAPCharacteristicTLVFormat_NetworkClientStatusControl_IPAddressList,
            .isOptional = true },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkClientStatusControl_Status, name),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkClientStatusControl_Status, nameIsSet),
            .tlvType = kHAPCharacteristicTLVType_NetworkClientStatusControl_Status_Name,
            .debugDescription = kHAPCharacteristicTLVDescription_NetworkClientStatusControl_Status_Name,
            .format = &kHAPCharacteristicTLVFormat_NetworkClientStatusControl_Status_Name,
            .isOptional = true },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkClientStatusControl_Status, rssi),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkClientStatusControl_Status, rssiIsSet),
            .tlvType = kHAPCharacteristicTLVType_NetworkClientStatusControl_Status_RSSI,
            .debugDescription = kHAPCharacteristicTLVDescription_NetworkClientStatusControl_Status_RSSI,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_RSSI,
            .isOptional = true },
    NULL
};

static const HAPCharacteristicTLVFormat_NetworkClientStatusControl_Status
        kHAPCharacteristicTLVFormat_NetworkClientStatusControl_Status = {
            .type = kHAPTLVFormatType_Struct,
            .members = kHAPCharacteristicTLVMembers_NetworkClientStatusControl_Status
        };

//----------------------------------------------------------------------------------------------------------------------

const HAPCharacteristicTLVFormat_NetworkClientStatusControl_Response
        kHAPCharacteristicTLVFormat_NetworkClientStatusControl_Response = {
            .type = kHAPTLVFormatType_Sequence,
            .item = { .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkClientStatusControl_Response, _),
                      .tlvType = kHAPCharacteristicTLVType_NetworkClientStatusControl_Response_Status,
                      .debugDescription = kHAPCharacteristicTLVDescription_NetworkClientStatusControl_Response_Status,
                      .format = &kHAPCharacteristicTLVFormat_NetworkClientStatusControl_Status },
            .separator = { .tlvType = kHAPCharacteristicTLVType_Separator,
                           .debugDescription = kHAPCharacteristicTLVDescription_Separator,
                           .format = &kHAPCharacteristicTLVFormat_Separator }
        };

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const HAPUUID kHAPCharacteristicType_RouterStatus = HAPUUIDCreateAppleDefined(0x20EU);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const HAPUUID kHAPCharacteristicType_SupportedRouterConfiguration = HAPUUIDCreateAppleDefined(0x210U);

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
static const char* _Nullable HAPCharacteristicValue_SupportedRouterConfiguration_Flags_GetBitDescription(
        uint32_t optionValue) {
    switch (optionValue) {
        case kHAPCharacteristicValue_SupportedRouterConfiguration_Flags_SupportsManagedNetwork:
            return "Supports Managed Network";
        case kHAPCharacteristicValue_SupportedRouterConfiguration_Flags_SupportsMACAddressCredentials:
            return "Supports MAC address based credentials";
        case kHAPCharacteristicValue_SupportedRouterConfiguration_Flags_SupportsPSKCredentials:
            return "Supports PSK based credentials";
        default:
            return NULL;
    }
}

static const HAPUInt32TLVFormat kHAPCharacteristicTLVFormat_SupportedRouterConfiguration_Flags = {
    .type = kHAPTLVFormatType_UInt32,
    .constraints = { .minimumValue = 0, .maximumValue = UINT32_MAX },
    .callbacks = { .getBitDescription = HAPCharacteristicValue_SupportedRouterConfiguration_Flags_GetBitDescription }
};

//----------------------------------------------------------------------------------------------------------------------

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_SupportedRouterConfiguration[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_SupportedRouterConfiguration, flags),
            .tlvType = kHAPCharacteristicTLVType_SupportedRouterConfiguration_Flags,
            .debugDescription = kHAPCharacteristicTLVDescription_SupportedRouterConfiguration_Flags,
            .format = &kHAPCharacteristicTLVFormat_SupportedRouterConfiguration_Flags },
    NULL
};

const HAPCharacteristicTLVFormat_SupportedRouterConfiguration
        kHAPCharacteristicTLVFormat_SupportedRouterConfiguration = {
            .type = kHAPTLVFormatType_Struct,
            .members = kHAPCharacteristicTLVMembers_SupportedRouterConfiguration
        };

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const HAPUUID kHAPCharacteristicType_WANConfigurationList = HAPUUIDCreateAppleDefined(0x211U);

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
static const char* _Nullable HAPCharacteristicValue_WiFiRouter_WANIdentifier_GetDescription(uint32_t value) {
    switch (value) { // NOLINT(hicpp-multiway-paths-covered)
        case kHAPCharacteristicValue_WiFiRouter_WANIdentifier_Main:
            return "Main WAN";
        default: {
            if (value >= 1024 && value <= 2047) {
                return "Vendor Specific";
            }
            return NULL;
        }
    }
}

static const HAPUInt32TLVFormat kHAPCharacteristicTLVFormat_WiFiRouter_WANIdentifier = {
    .type = kHAPTLVFormatType_UInt32,
    .constraints = { .minimumValue = 1, .maximumValue = UINT32_MAX },
    .callbacks = { .getDescription = HAPCharacteristicValue_WiFiRouter_WANIdentifier_GetDescription }
};

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_WANConfigurationList_Config_WANType_IsValid(uint8_t value) {
    HAPAssert(sizeof value == sizeof(HAPCharacteristicValue_WANConfigurationList_Config_WANType));
    switch ((HAPCharacteristicValue_WANConfigurationList_Config_WANType) value) {
        case kHAPCharacteristicValue_WANConfigurationList_Config_WANType_Unconfigured:
        case kHAPCharacteristicValue_WANConfigurationList_Config_WANType_Other:
        case kHAPCharacteristicValue_WANConfigurationList_Config_WANType_DHCP:
        case kHAPCharacteristicValue_WANConfigurationList_Config_WANType_BridgeMode: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
static const char* HAPCharacteristicValue_WANConfigurationList_Config_WANType_GetDescription(
        HAPCharacteristicValue_WANConfigurationList_Config_WANType value) {
    HAPPrecondition(HAPCharacteristicValue_WANConfigurationList_Config_WANType_IsValid(value));
    switch (value) {
        case kHAPCharacteristicValue_WANConfigurationList_Config_WANType_Unconfigured:
            return "Unconfigured";
        case kHAPCharacteristicValue_WANConfigurationList_Config_WANType_Other:
            return "Other";
        case kHAPCharacteristicValue_WANConfigurationList_Config_WANType_DHCP:
            return "DHCP";
        case kHAPCharacteristicValue_WANConfigurationList_Config_WANType_BridgeMode:
            return "Bridge Mode";
        default:
            HAPFatalError();
    }
}

static const HAPCharacteristicTLVFormat_WANConfigurationList_Config_WANType
        kHAPCharacteristicTLVFormat_WANConfigurationList_Config_WANType = {
            .type = kHAPTLVFormatType_Enum,
            .callbacks = { .isValid = HAPCharacteristicValue_WANConfigurationList_Config_WANType_IsValid,
                           .getDescription = HAPCharacteristicValue_WANConfigurationList_Config_WANType_GetDescription }
        };

//----------------------------------------------------------------------------------------------------------------------

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_WANConfigurationList_Config[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WANConfigurationList_Config, wanIdentifier),
            .tlvType = kHAPCharacteristicTLVType_WANConfigurationList_Config_Identifier,
            .debugDescription = kHAPCharacteristicTLVDescription_WANConfigurationList_Config_Identifier,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_WANIdentifier },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WANConfigurationList_Config, wanType),
            .tlvType = kHAPCharacteristicTLVType_WANConfigurationList_Config_WANType,
            .debugDescription = kHAPCharacteristicTLVDescription_WANConfigurationList_Config_WANType,
            .format = &kHAPCharacteristicTLVFormat_WANConfigurationList_Config_WANType },
    NULL
};

static const HAPCharacteristicTLVFormat_WANConfigurationList_Config
        kHAPCharacteristicTLVFormat_WANConfigurationList_Config = {
            .type = kHAPTLVFormatType_Struct,
            .members = kHAPCharacteristicTLVMembers_WANConfigurationList_Config
        };

//----------------------------------------------------------------------------------------------------------------------

const HAPCharacteristicTLVFormat_WANConfigurationList kHAPCharacteristicTLVFormat_WANConfigurationList = {
    .type = kHAPTLVFormatType_Sequence,
    .item = { .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WANConfigurationList, _),
              .tlvType = kHAPCharacteristicTLVType_WANConfigurationList_Config,
              .debugDescription = kHAPCharacteristicTLVDescription_WANConfigurationList_Config,
              .format = &kHAPCharacteristicTLVFormat_WANConfigurationList_Config },
    .separator = { .tlvType = kHAPCharacteristicTLVType_Separator,
                   .debugDescription = kHAPCharacteristicTLVDescription_Separator,
                   .format = &kHAPCharacteristicTLVFormat_Separator }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const HAPUUID kHAPCharacteristicType_WANStatusList = HAPUUIDCreateAppleDefined(0x212U);

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
static const char* _Nullable HAPCharacteristicValue_WANStatusList_Status_LinkStatus_GetDescription(uint64_t value) {
    switch (value) { // NOLINT(hicpp-multiway-paths-covered)
        case 0:
            return "Online";
        default:
            return NULL;
    }
}

HAP_RESULT_USE_CHECK
static const char* _Nullable HAPCharacteristicValue_WANStatusList_Status_LinkStatus_GetBitDescription(
        uint64_t optionValue) {
    switch (optionValue) {
        case kHAPCharacteristicValue_WANStatusList_Status_LinkStatus_Unknown:
            return "Unknown";
        case kHAPCharacteristicValue_WANStatusList_Status_LinkStatus_NoCableConnected:
            return "No cable connected";
        case kHAPCharacteristicValue_WANStatusList_Status_LinkStatus_NoIPAddress:
            return "No IP address";
        case kHAPCharacteristicValue_WANStatusList_Status_LinkStatus_NoGatewaySpecified:
            return "No gateway specified";
        case kHAPCharacteristicValue_WANStatusList_Status_LinkStatus_GatewayUnreachable:
            return "Gateway unreachable";
        case kHAPCharacteristicValue_WANStatusList_Status_LinkStatus_NoDNSServerSpecified:
            return "No DNS server(s) specified";
        case kHAPCharacteristicValue_WANStatusList_Status_LinkStatus_DNSServerUnreachable:
            return "DNS server(s) unreachable";
        case kHAPCharacteristicValue_WANStatusList_Status_LinkStatus_AuthenticationFailed:
            return "Authentication failed";
        case kHAPCharacteristicValue_WANStatusList_Status_LinkStatus_Walled:
            return "Walled";
        default:
            return NULL;
    }
}

static const HAPUInt64TLVFormat kHAPCharacteristicTLVFormat_WANStatusList_Status_LinkStatus = {
    .type = kHAPTLVFormatType_UInt64,
    .constraints = { .minimumValue = 0, .maximumValue = UINT64_MAX },
    .callbacks = { .getDescription = HAPCharacteristicValue_WANStatusList_Status_LinkStatus_GetDescription,
                   .getBitDescription = HAPCharacteristicValue_WANStatusList_Status_LinkStatus_GetBitDescription }
};

//----------------------------------------------------------------------------------------------------------------------

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_WANStatusList_Status[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WANStatusList_Status, wanIdentifier),
            .tlvType = kHAPCharacteristicTLVType_WANStatusList_Status_WANIdentifier,
            .debugDescription = kHAPCharacteristicTLVDescription_WANStatusList_Status_WANIdentifier,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_WANIdentifier },
    &(const HAPStructTLVMember) { .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WANStatusList_Status, linkStatus),
                                  .tlvType = kHAPCharacteristicTLVType_WANStatusList_Status_LinkStatus,
                                  .debugDescription = kHAPCharacteristicTLVDescription_WANStatusList_Status_LinkStatus,
                                  .format = &kHAPCharacteristicTLVFormat_WANStatusList_Status_LinkStatus },
    NULL
};

static const HAPCharacteristicTLVFormat_WANStatusList_Status kHAPCharacteristicTLVFormat_WANStatusList_Status = {
    .type = kHAPTLVFormatType_Struct,
    .members = kHAPCharacteristicTLVMembers_WANStatusList_Status
};

//----------------------------------------------------------------------------------------------------------------------

const HAPCharacteristicTLVFormat_WANStatusList kHAPCharacteristicTLVFormat_WANStatusList = {
    .type = kHAPTLVFormatType_Sequence,
    .item = { .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WANStatusList, _),
              .tlvType = kHAPCharacteristicTLVType_WANStatusList_Status,
              .debugDescription = kHAPCharacteristicTLVDescription_WANStatusList_Status,
              .format = &kHAPCharacteristicTLVFormat_WANStatusList_Status },
    .separator = { .tlvType = kHAPCharacteristicTLVType_Separator,
                   .debugDescription = kHAPCharacteristicTLVDescription_Separator,
                   .format = &kHAPCharacteristicTLVFormat_Separator }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const HAPUUID kHAPCharacteristicType_ManagedNetworkEnable = HAPUUIDCreateAppleDefined(0x215U);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const HAPUUID kHAPCharacteristicType_HomeKitCameraActive = HAPUUIDCreateAppleDefined(0x21BU);

const HAPUUID kHAPCharacteristicType_ThirdPartyCameraActive = HAPUUIDCreateAppleDefined(0x21CU);

const HAPUUID kHAPCharacteristicType_CameraOperatingModeIndicator = HAPUUIDCreateAppleDefined(0x21DU);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const HAPUUID kHAPCharacteristicType_WiFiSatelliteStatus = HAPUUIDCreateAppleDefined(0x21EU);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const HAPUUID kHAPCharacteristicType_NetworkAccessViolationControl = HAPUUIDCreateAppleDefined(0x21FU);

//----------------------------------------------------------------------------------------------------------------------

static const HAPUInt64TLVFormat kHAPCharacteristicTLVFormat_WiFiRouter_Timestamp = {
    .type = kHAPTLVFormatType_UInt64,
    .constraints = { .minimumValue = 0, .maximumValue = UINT64_MAX }
};

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_NetworkAccessViolationControl_OperationType_IsValid(uint8_t value) {
    HAPAssert(sizeof value == sizeof(HAPCharacteristicValue_NetworkAccessViolationControl_OperationType));
    switch ((HAPCharacteristicValue_NetworkAccessViolationControl_OperationType) value) {
        case kHAPCharacteristicValue_NetworkAccessViolationControl_OperationType_List:
        case kHAPCharacteristicValue_NetworkAccessViolationControl_OperationType_Reset: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
static const char* HAPCharacteristicValue_NetworkAccessViolationControl_OperationType_GetDescription(
        HAPCharacteristicValue_NetworkAccessViolationControl_OperationType value) {
    HAPPrecondition(HAPCharacteristicValue_NetworkAccessViolationControl_OperationType_IsValid(value));
    switch (value) {
        case kHAPCharacteristicValue_NetworkAccessViolationControl_OperationType_List:
            return "List";
        case kHAPCharacteristicValue_NetworkAccessViolationControl_OperationType_Reset:
            return "Reset";
        default:
            HAPFatalError();
    }
}

static const HAPCharacteristicTLVFormat_NetworkAccessViolationControl_OperationType
        kHAPCharacteristicTLVFormat_NetworkAccessViolationControl_OperationType = {
            .type = kHAPTLVFormatType_Enum,
            .callbacks = { .isValid = HAPCharacteristicValue_NetworkAccessViolationControl_OperationType_IsValid,
                           .getDescription =
                                   HAPCharacteristicValue_NetworkAccessViolationControl_OperationType_GetDescription }
        };

//----------------------------------------------------------------------------------------------------------------------

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_NetworkAccessViolationControl[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkAccessViolationControl, operationType),
            .tlvType = kHAPCharacteristicTLVType_NetworkAccessViolationControl_OperationType,
            .debugDescription = kHAPCharacteristicTLVDescription_NetworkAccessViolationControl_OperationType,
            .format = &kHAPCharacteristicTLVFormat_NetworkAccessViolationControl_OperationType },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkAccessViolationControl, clientList),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkAccessViolationControl, clientListIsSet),
            .tlvType = kHAPCharacteristicTLVType_NetworkAccessViolationControl_ClientList,
            .debugDescription = kHAPCharacteristicTLVDescription_NetworkAccessViolationControl_ClientList,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_ClientList,
            .isOptional = true },
    NULL
};

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_NetworkAccessViolationControl_IsValid(
        HAPCharacteristicValue_NetworkAccessViolationControl* value) {
    HAPPrecondition(value);

    switch (value->operationType) {
        case kHAPCharacteristicValue_NetworkAccessViolationControl_OperationType_List: {
            // All other TLV items in the request are ignored.
        }
            return true;
        case kHAPCharacteristicValue_NetworkAccessViolationControl_OperationType_Reset: {
            if (!value->clientListIsSet) {
                HAPLog(&logObject, "Client List required when Operation Type is Reset.");
                return false;
            }
        }
            return true;
        default:
            HAPFatalError();
    }
}

const HAPCharacteristicTLVFormat_NetworkAccessViolationControl
        kHAPCharacteristicTLVFormat_NetworkAccessViolationControl = {
            .type = kHAPTLVFormatType_Struct,
            .members = kHAPCharacteristicTLVMembers_NetworkAccessViolationControl,
            .callbacks = { .isValid = HAPCharacteristicValue_NetworkAccessViolationControl_IsValid }
        };

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_NetworkAccessViolationControl_Violation[] = {
    &(const HAPStructTLVMember) {
            .valueOffset =
                    HAP_OFFSETOF(HAPCharacteristicValue_NetworkAccessViolationControl_Violation, clientIdentifier),
            .tlvType = kHAPCharacteristicTLVType_NetworkAccessViolationControl_Violation_Client,
            .debugDescription = kHAPCharacteristicTLVDescription_NetworkAccessViolationControl_Violation_Client,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_ClientIdentifier },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkAccessViolationControl_Violation, lastTimestamp),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkAccessViolationControl_Violation, timestampIsSet),
            .tlvType = kHAPCharacteristicTLVType_NetworkAccessViolationControl_Violation_LastTimestamp,
            .debugDescription = kHAPCharacteristicTLVDescription_NetworkAccessViolationControl_Violation_LastTimestamp,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_Timestamp,
            .isOptional = true },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkAccessViolationControl_Violation, resetTimestamp),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkAccessViolationControl_Violation, resetIsSet),
            .tlvType = kHAPCharacteristicTLVType_NetworkAccessViolationControl_Violation_ResetTimestamp,
            .debugDescription = kHAPCharacteristicTLVDescription_NetworkAccessViolationControl_Violation_ResetTimestamp,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_Timestamp,
            .isOptional = true },
    NULL
};

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_NetworkAccessViolationControl_Violation_IsValid(
        HAPCharacteristicValue_NetworkAccessViolationControl_Violation* value) {
    HAPPrecondition(value);

    if (value->timestampIsSet && value->resetIsSet) {
        if (value->lastTimestamp <= value->resetTimestamp) {
            HAPLog(&logObject, "Last Violation Timestamp must be after Last Reset Timestamp.");
            return false;
        }
    }

    return true;
}

static const HAPCharacteristicTLVFormat_NetworkAccessViolationControl_Violation
        kHAPCharacteristicTLVFormat_NetworkAccessViolationControl_Violation = {
            .type = kHAPTLVFormatType_Struct,
            .members = kHAPCharacteristicTLVMembers_NetworkAccessViolationControl_Violation,
            .callbacks = { .isValid = HAPCharacteristicValue_NetworkAccessViolationControl_Violation_IsValid }
        };

//----------------------------------------------------------------------------------------------------------------------

const HAPCharacteristicTLVFormat_NetworkAccessViolationControl_Response
        kHAPCharacteristicTLVFormat_NetworkAccessViolationControl_Response = {
            .type = kHAPTLVFormatType_Sequence,
            .item = { .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkAccessViolationControl_Response, _),
                      .tlvType = kHAPCharacteristicTLVType_NetworkAccessViolationControl_Response_Violation,
                      .debugDescription =
                              kHAPCharacteristicTLVDescription_NetworkAccessViolationControl_Response_Violation,
                      .format = &kHAPCharacteristicTLVFormat_NetworkAccessViolationControl_Violation },
            .separator = { .tlvType = kHAPCharacteristicTLVType_Separator,
                           .debugDescription = kHAPCharacteristicTLVDescription_Separator,
                           .format = &kHAPCharacteristicTLVFormat_Separator }
        };

//----------------------------------------------------------------------------------------------------------------------

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_NetworkAccessViolationControl_Event[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkAccessViolationControl_Event, clientList),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_NetworkAccessViolationControl_Event, clientListIsSet),
            .tlvType = kHAPCharacteristicTLVType_NetworkAccessViolationControl_Event_ClientList,
            .debugDescription = kHAPCharacteristicTLVDescription_NetworkAccessViolationControl_Event_ClientList,
            .format = &kHAPCharacteristicTLVFormat_WiFiRouter_ClientList,
            .isOptional = true },
    NULL
};

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_NetworkAccessViolationControl_Event_IsValid(
        HAPCharacteristicValue_NetworkAccessViolationControl_Event* value) {
    HAPPrecondition(value);

    return true;
}

const HAPCharacteristicTLVFormat_NetworkAccessViolationControl_Event
        kHAPCharacteristicTLVFormat_NetworkAccessViolationControl_Event = {
            .type = kHAPTLVFormatType_Struct,
            .members = kHAPCharacteristicTLVMembers_NetworkAccessViolationControl_Event,
            .callbacks = { .isValid = HAPCharacteristicValue_NetworkAccessViolationControl_Event_IsValid }
        };

const HAPUUID kHAPCharacteristicType_CurrentTransport = HAPUUIDCreateAppleDefined(0x22BU);

const HAPUUID kHAPCharacteristicType_WiFiCapability = HAPUUIDCreateAppleDefined(0x22CU);

const HAPUUID kHAPCharacteristicType_WiFiConfigurationControl = HAPUUIDCreateAppleDefined(0x22DU);

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_WiFiConfigurationControl_OperationType_IsValid(uint8_t value) {
    HAPAssert(sizeof value == sizeof(HAPCharacteristicValue_WiFiConfigurationControl_OperationType));
    switch ((HAPCharacteristicValue_WiFiConfigurationControl_OperationType) value) {
        case kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Read:
        case kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Update_Simple:
        case kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Update_FailSafe:
        case kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Update_FailSafe_Commit: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
static const char* HAPCharacteristicValue_WiFiConfigurationControl_OperationType_GetDescription(
        HAPCharacteristicValue_WiFiConfigurationControl_OperationType value) {
    HAPPrecondition(HAPCharacteristicValue_WiFiConfigurationControl_OperationType_IsValid(value));
    switch (value) {
        case kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Read:
            return "Read";
        case kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Update_Simple:
            return "Simple Update";
        case kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Update_FailSafe:
            return "Failsafe Update";
        case kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Update_FailSafe_Commit:
            return "Fail Safe Commit message";
        default:
            HAPFatalError();
    }
}

static const HAPCharacteristicTLVFormat_WiFiConfigurationControl_OperationType
        kHAPCharacteristicTLVFormat_WiFiConfigurationControl_OperationType = {
            .type = kHAPTLVFormatType_Enum,
            .callbacks = { .isValid = HAPCharacteristicValue_WiFiConfigurationControl_OperationType_IsValid,
                           .getDescription =
                                   HAPCharacteristicValue_WiFiConfigurationControl_OperationType_GetDescription }
        };

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_WiFiConfigurationControl_StationConfig_SecurityMode_IsValid(uint8_t value) {
    HAPAssert(sizeof value == sizeof(HAPCharacteristicValue_WiFiConfigurationControl_StationConfig_SecurityMode));
    switch (value) {
        case kHAPCharacteristicValue_WiFiConfigurationControl_StationConfig_SecurityMode_None:
        case kHAPCharacteristicValue_WiFiConfigurationControl_StationConfig_SecurityMode_WPA2_PSK: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
static const char* HAPCharacteristicValue_WiFiConfigurationControl_StationConfig_SecurityMode_GetDescription(
        HAPCharacteristicValue_WiFiConfigurationControl_StationConfig_SecurityMode value) {
    HAPPrecondition(HAPCharacteristicValue_WiFiConfigurationControl_StationConfig_SecurityMode_IsValid(value));
    switch (value) {
        case kHAPCharacteristicValue_WiFiConfigurationControl_StationConfig_SecurityMode_None:
            return "Security Mode - Open";
        case kHAPCharacteristicValue_WiFiConfigurationControl_StationConfig_SecurityMode_WPA2_PSK:
            return "Security Mode - WPA2-PSK";
        default:
            HAPFatalError();
    }
}

static const HAPUInt16TLVFormat kHAPCharacteristicTLVFormat_WiFiConfigurationControl_Cookie = {
    .type = kHAPTLVFormatType_UInt16,
    .constraints = { .minimumValue = 0, .maximumValue = UINT16_MAX }
};

static const HAPStringTLVFormat kHAPCharacteristicTLVFormat_WiFiConfigurationControl_StationConfig_SSID = {
    .type = kHAPTLVFormatType_String,
    .constraints = { .minLength = 0, .maxLength = 32 }
};

static const HAPCharacteristicTLVFormat_WiFiConfigurationControl_StationConfig_SecurityMode
        kHAPCharacteristicTLVFormat_WiFiConfigurationControl_StationConfig_SecurityMode = {
            .type = kHAPTLVFormatType_Enum,
            .callbacks = { .isValid =
                                   HAPCharacteristicValue_WiFiConfigurationControl_StationConfig_SecurityMode_IsValid,
                           .getDescription =
                                   HAPCharacteristicValue_WiFiConfigurationControl_StationConfig_SecurityMode_GetDescription }
        };

static const HAPStringTLVFormat kHAPCharacteristicTLVFormat_WiFiConfigurationControl_StationConfig_PSK = {
    .type = kHAPTLVFormatType_String,
    .constraints = { .minLength = 0, .maxLength = 64 }
};

static const HAPUInt32TLVFormat kHAPCharacteristicTLVFormat_WiFiConfigurationControl_UpdateStatus = {
    .type = kHAPTLVFormatType_UInt32,
    .constraints = { .minimumValue = 0, .maximumValue = UINT32_MAX }
};

static const HAPUInt8TLVFormat kHAPCharacteristicTLVFormat_WiFiConfigurationControl_Operation_Timeout = {
    .type = kHAPTLVFormatType_UInt8,
    .constraints = { .minimumValue = 1, .maximumValue = 60 }
};

static const HAPStringTLVFormat kHAPCharacteristicTLVFormat_WiFiConfigurationControl_CountryCode = {
    .type = kHAPTLVFormatType_String,
    .constraints = { .minLength = 0, .maxLength = 3 }

};

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_WiFiConfigurationControl_StationConfig[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiConfigurationControl_StationConfig, ssid),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiConfigurationControl_StationConfig, ssidIsSet),
            .tlvType = kHAPCharacteristicTLVType_WiFiConfigurationControl_StationConfig_SSID,
            .debugDescription = kHAPCharacteristicTLVDescription_WiFiConfigurationControl_StationConfig_SSID,
            .format = &kHAPCharacteristicTLVFormat_WiFiConfigurationControl_StationConfig_SSID,
            .isOptional = true },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiConfigurationControl_StationConfig, securityMode),
            .isSetOffset =
                    HAP_OFFSETOF(HAPCharacteristicValue_WiFiConfigurationControl_StationConfig, securityModeIsSet),
            .tlvType = kHAPCharacteristicTLVType_WiFiConfigurationControl_StationConfig_SecurityMode,
            .debugDescription = kHAPCharacteristicTLVDescription_WiFiConfigurationControl_StationConfig_SecurityMode,
            .format = &kHAPCharacteristicTLVFormat_WiFiConfigurationControl_StationConfig_SecurityMode,
            .isOptional = true },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiConfigurationControl_StationConfig, psk),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiConfigurationControl_StationConfig, pskIsSet),
            .tlvType = kHAPCharacteristicTLVType_WiFiConfigurationControl_StationConfig_PSK,
            .debugDescription = kHAPCharacteristicTLVDescription_WiFiConfigurationControl_StationConfig_PSK,
            .format = &kHAPCharacteristicTLVFormat_WiFiConfigurationControl_StationConfig_PSK,
            .isOptional = true },
    NULL
};

static const HAPCharacteristicTLVFormat_WiFiConfigurationControl_StationConfig
        kHAPCharacteristicTLVFormat_WiFiConfigurationControl_StationConfig = {
            .type = kHAPTLVFormatType_Struct,
            .members = kHAPCharacteristicTLVMembers_WiFiConfigurationControl_StationConfig
        };

//----------------------------------------------------------------------------------------------------------------------

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_WiFiConfigurationControl[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiConfigurationControl, operationType),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiConfigurationControl, operationTypeIsSet),
            .tlvType = kHAPCharacteristicTLVType_WiFiConfigurationControl_OperationType,
            .debugDescription = kHAPCharacteristicTLVDescription_WiFiConfigurationControl_OperationType,
            .format = &kHAPCharacteristicTLVFormat_WiFiConfigurationControl_OperationType,
            .isOptional = true },
    &(const HAPStructTLVMember) { .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiConfigurationControl, cookie),
                                  .isSetOffset =
                                          HAP_OFFSETOF(HAPCharacteristicValue_WiFiConfigurationControl, cookieIsSet),
                                  .tlvType = kHAPCharacteristicTLVType_WiFiConfigurationControl_Cookie,
                                  .debugDescription = kHAPCharacteristicTLVDescription_WiFiConfigurationControl_Cookie,
                                  .format = &kHAPCharacteristicTLVFormat_WiFiConfigurationControl_Cookie,
                                  .isOptional = true },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiConfigurationControl, updateStatus),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiConfigurationControl, updateStatusIsSet),
            .tlvType = kHAPCharacteristicTLVType_WiFiConfigurationControl_UpdateStatus,
            .debugDescription = kHAPCharacteristicTLVDescription_WiFiConfigurationControl_UpdateStatus,
            .format = &kHAPCharacteristicTLVFormat_WiFiConfigurationControl_UpdateStatus,
            .isOptional = true },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiConfigurationControl, operationTimeout),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiConfigurationControl, operationTimeoutIsSet),
            .tlvType = kHAPCharacteristicTLVType_WiFiConfigurationControl_Operation_Timeout,
            .debugDescription = kHAPCharacteristicTLVDescription_WiFiConfigurationControl_Operation_Timeout,
            .format = &kHAPCharacteristicTLVFormat_WiFiConfigurationControl_Operation_Timeout,
            .isOptional = true },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiConfigurationControl, countryCode),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiConfigurationControl, countryCodeIsSet),
            .tlvType = kHAPCharacteristicTLVType_WiFiConfigurationControl_CountryCode,
            .debugDescription = kHAPCharacteristicTLVDescription_WiFiConfigurationControl_CountryCode,
            .format = &kHAPCharacteristicTLVFormat_WiFiConfigurationControl_CountryCode,
            .isOptional = true },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiConfigurationControl, stationConfig),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_WiFiConfigurationControl, stationConfigIsSet),
            .tlvType = kHAPCharacteristicTLVType_WiFiConfigurationControl_StationConfig,
            .debugDescription = kHAPCharacteristicTLVDescription_WiFiConfigurationControl_StationConfig,
            .format = &kHAPCharacteristicTLVFormat_WiFiConfigurationControl_StationConfig,
            .isOptional = true },
    NULL
};

const HAPCharacteristicTLVFormat_WiFiConfigurationControl kHAPCharacteristicTLVFormat_WiFiConfigurationControl = {
    .type = kHAPTLVFormatType_Struct,
    .members = kHAPCharacteristicTLVMembers_WiFiConfigurationControl
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const HAPUUID kHAPCharacteristicType_ProductData = HAPUUIDCreateAppleDefined(0x220U);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const HAPUUID kHAPCharacteristicType_EventSnapshotsActive = HAPUUIDCreateAppleDefined(0x223U);

const HAPUUID kHAPCharacteristicType_DiagonalFieldOfView = HAPUUIDCreateAppleDefined(0x224U);

const HAPUUID kHAPCharacteristicType_PeriodicSnapshotsActive = HAPUUIDCreateAppleDefined(0x225U);

const HAPUUID kHAPCharacteristicType_RecordingAudioActive = HAPUUIDCreateAppleDefined(0x226U);

const HAPUUID kHAPCharacteristicType_ManuallyDisabled = HAPUUIDCreateAppleDefined(0x227U);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const HAPUUID kHAPCharacteristicType_AccessCodeSupportedConfiguration = HAPUUIDCreateAppleDefined(0x261U);

HAP_RESULT_USE_CHECK
static const char* _Nullable HAPCharacteristicValue_AccessCodeSupportedConfiguration_CharacterSet_GetDescription(
        uint8_t value) {
    switch (value) { // NOLINT(hicpp-multiway-paths-covered)
        case kHAPCharacteristicValue_AccessCode_CharacterSet_ArabicNumerals:
            return "Arabic numerals";
        default:
            return NULL;
    }
}

static const HAPUInt8TLVFormat kHAPCharacteristicTLVFormat_AccessCodeSupportedConfiguration_CharacterSet = {
    .type = kHAPTLVFormatType_UInt8,
    .constraints = { .minimumValue = 1, .maximumValue = UINT8_MAX },
    .callbacks = { .getDescription =
                           HAPCharacteristicValue_AccessCodeSupportedConfiguration_CharacterSet_GetDescription }
};

static const HAPUInt8TLVFormat kHAPCharacteristicTLVFormat_AccessCodeSupportedConfiguration_UInt8 = {
    .type = kHAPTLVFormatType_UInt8,
    .constraints = { .minimumValue = 0, .maximumValue = UINT8_MAX }
};

static const HAPUInt16TLVFormat kHAPCharacteristicTLVFormat_AccessCodeSupportedConfiguration_UInt16 = {
    .type = kHAPTLVFormatType_UInt16,
    .constraints = { .minimumValue = 0, .maximumValue = UINT16_MAX }
};

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_AccessCodeSupportedConfiguration[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_AccessCodeSupportedConfiguration, characterSet),
            .tlvType = kHAPCharacteristicTLVType_AccessCodeSupportedConfiguration_CharacterSet,
            .debugDescription = kHAPCharacteristicTLVDescription_AccessCodeSupportedConfiguration_CharacterSet,
            .format = &kHAPCharacteristicTLVFormat_AccessCodeSupportedConfiguration_CharacterSet },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_AccessCodeSupportedConfiguration, minimumLength),
            .tlvType = kHAPCharacteristicTLVType_AccessCodeSupportedConfiguration_MinimumLength,
            .debugDescription = kHAPCharacteristicTLVDescription_AccessCodeSupportedConfiguration_MinimumLength,
            .format = &kHAPCharacteristicTLVFormat_AccessCodeSupportedConfiguration_UInt8 },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_AccessCodeSupportedConfiguration, maximumLength),
            .tlvType = kHAPCharacteristicTLVType_AccessCodeSupportedConfiguration_MaximumLength,
            .debugDescription = kHAPCharacteristicTLVDescription_AccessCodeSupportedConfiguration_MaximumLength,
            .format = &kHAPCharacteristicTLVFormat_AccessCodeSupportedConfiguration_UInt8 },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_AccessCodeSupportedConfiguration, maximumAccessCodes),
            .tlvType = kHAPCharacteristicTLVType_AccessCodeSupportedConfiguration_MaximumAccessCodes,
            .debugDescription = kHAPCharacteristicTLVDescription_AccessCodeSupportedConfiguration_MaximumAccessCodes,
            .format = &kHAPCharacteristicTLVFormat_AccessCodeSupportedConfiguration_UInt16 },
    NULL
};

const HAPCharacteristicTLVFormat_AccessCodeSupportedConfiguration
        kHAPCharacteristicTLVFormat_AccessCodeSupportedConfiguration = {
            .type = kHAPTLVFormatType_Struct,
            .members = kHAPCharacteristicTLVMembers_AccessCodeSupportedConfiguration
        };

const HAPUUID kHAPCharacteristicType_AccessCodeControlPoint = HAPUUIDCreateAppleDefined(0x262U);

static const HAPUInt8TLVFormat kHAPCharacteristicTLVFormat_AccessCodeControlPoint_OperationType = {
    .type = kHAPTLVFormatType_UInt8,
    .constraints = { .minimumValue = 0, .maximumValue = UINT8_MAX }
};

static const HAPUInt32TLVFormat kHAPCharacteristicTLVFormat_AccessCodeControlPoint_Identifier = {
    .type = kHAPTLVFormatType_UInt32,
    .constraints = { .minimumValue = 0, .maximumValue = UINT32_MAX }
};

static const HAPStringTLVFormat kHAPCharacteristicTLVFormat_AccessCodeControlPoint_AccessCode = {
    .type = kHAPTLVFormatType_String,
    .constraints = { .minLength = 1, .maxLength = SIZE_MAX }
};

static const HAPUInt32TLVFormat kHAPCharacteristicTLVFormat_AccessCodeControlPoint_Flags = {
    .type = kHAPTLVFormatType_UInt32,
    .constraints = { .minimumValue = 0, .maximumValue = UINT32_MAX }
};

static const HAPUInt8TLVFormat kHAPCharacteristicTLVFormat_AccessCodeControlPoint_Status = {
    .type = kHAPTLVFormatType_UInt8,
    .constraints = { .minimumValue = 0, .maximumValue = UINT8_MAX }
};

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_AccessCodeControlPointResponse[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_AccessCodeControlPointResponse, identifier),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_AccessCodeControlPointResponse, identifierIsSet),
            .tlvType = kHAPCharacteristicTLVType_AccessCodeControlPointResponse_Identifier,
            .debugDescription = kHAPCharacteristicTLVDescription_AccessCodeControlPointResponse_Identifier,
            .format = &kHAPCharacteristicTLVFormat_AccessCodeControlPoint_Identifier,
            .isOptional = true },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_AccessCodeControlPointResponse, accessCode),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_AccessCodeControlPointResponse, accessCodeIsSet),
            .tlvType = kHAPCharacteristicTLVType_AccessCodeControlPointResponse_AccessCode,
            .debugDescription = kHAPCharacteristicTLVDescription_AccessCodeControlPointResponse_AccessCode,
            .format = &kHAPCharacteristicTLVFormat_AccessCodeControlPoint_AccessCode,
            .isOptional = true },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_AccessCodeControlPointResponse, flags),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_AccessCodeControlPointResponse, flagsIsSet),
            .tlvType = kHAPCharacteristicTLVType_AccessCodeControlPointResponse_Flags,
            .debugDescription = kHAPCharacteristicTLVDescription_AccessCodeControlPointResponse_Flags,
            .format = &kHAPCharacteristicTLVFormat_AccessCodeControlPoint_Flags,
            .isOptional = true },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_AccessCodeControlPointResponse, status),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_AccessCodeControlPointResponse, statusIsSet),
            .tlvType = kHAPCharacteristicTLVType_AccessCodeControlPointResponse_Status,
            .debugDescription = kHAPCharacteristicTLVDescription_AccessCodeControlPointResponse_Status,
            .format = &kHAPCharacteristicTLVFormat_AccessCodeControlPoint_Status,
            .isOptional = true },
    NULL
};

const HAPCharacteristicTLVFormat_AccessCodeControlPointResponse
        kHAPCharacteristicTLVFormat_AccessCodeControlPointResponse = {
            .type = kHAPTLVFormatType_Struct,
            .members = kHAPCharacteristicTLVMembers_AccessCodeControlPointResponse
        };

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_AccessCodeControlPoint[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_AccessCodeControlPoint, operationType),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_AccessCodeControlPoint, operationTypeIsSet),
            .tlvType = kHAPCharacteristicTLVType_AccessCodeControlPoint_OperationType,
            .debugDescription = kHAPCharacteristicTLVDescription_AccessCodeControlPoint_OperationType,
            .format = &kHAPCharacteristicTLVFormat_AccessCodeControlPoint_OperationType,
            .isOptional = true },
    &(const HAPStructTLVMember) { .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_AccessCodeControlPoint, response),
                                  .isSetOffset =
                                          HAP_OFFSETOF(HAPCharacteristicValue_AccessCodeControlPoint, responseIsSet),
                                  .tlvType = kHAPCharacteristicTLVType_AccessCodeControlPoint_Response,
                                  .debugDescription = kHAPCharacteristicTLVDescription_AccessCodeControlPoint_Response,
                                  .format = &kHAPCharacteristicTLVFormat_AccessCodeControlPointResponse,
                                  .isOptional = true },
    NULL
};

const HAPCharacteristicTLVFormat_AccessCodeControlPoint kHAPCharacteristicTLVFormat_AccessCodeControlPoint = {
    .type = kHAPTLVFormatType_Struct,
    .members = kHAPCharacteristicTLVMembers_AccessCodeControlPoint
};

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_AccessCodeControlPointRequest[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_AccessCodeControlPointRequest, identifier),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_AccessCodeControlPointRequest, identifierIsSet),
            .tlvType = kHAPCharacteristicTLVType_AccessCodeControlPointRequest_Identifier,
            .debugDescription = kHAPCharacteristicTLVDescription_AccessCodeControlPointRequest_Identifier,
            .format = &kHAPCharacteristicTLVFormat_AccessCodeControlPoint_Identifier,
            .isOptional = true },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_AccessCodeControlPointRequest, accessCode),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_AccessCodeControlPointRequest, accessCodeIsSet),
            .tlvType = kHAPCharacteristicTLVType_AccessCodeControlPointRequest_AccessCode,
            .debugDescription = kHAPCharacteristicTLVDescription_AccessCodeControlPointRequest_AccessCode,
            .format = &kHAPCharacteristicTLVFormat_AccessCodeControlPoint_AccessCode,
            .isOptional = true },
    NULL
};

const HAPCharacteristicTLVFormat_AccessCodeControlPointRequest
        kHAPCharacteristicTLVFormat_AccessCodeControlPointRequest = {
            .type = kHAPTLVFormatType_Struct,
            .members = kHAPCharacteristicTLVMembers_AccessCodeControlPointRequest
        };

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const HAPUUID kHAPCharacteristicType_FirmwareUpdateReadiness = HAPUUIDCreateAppleDefined(0x234U);

HAP_RESULT_USE_CHECK
static const char* _Nullable HAPCharacteristicValue_FirmwareUpdateReadiness_StagingNotReady_GetBitDescription(
        uint32_t value) {
    switch (value) {
        case kHAPCharacteristicValue_FirmwareUpdateReadiness_StagingNotReady_Other:
            return "Other";
        case kHAPCharacteristicValue_FirmwareUpdateReadiness_StagingNotReady_LowBattery:
            return "Low Battery";
        case kHAPCharacteristicValue_FirmwareUpdateReadiness_StagingNotReady_Connectivity:
            return "Connectivity";
        case kHAPCharacteristicValue_FirmwareUpdateReadiness_StagingNotReady_ApplyNeeded:
            return "Apply Needed";
        default:
            return NULL;
    }
}

static const HAPUInt32TLVFormat kHAPCharacteristicTLVFormat_FirmwareUpdateReadiness_StagingNotReadyReason = {
    .type = kHAPTLVFormatType_UInt32,
    .constraints = { .minimumValue = 0, .maximumValue = UINT32_MAX },
    .callbacks = { .getBitDescription =
                           HAPCharacteristicValue_FirmwareUpdateReadiness_StagingNotReady_GetBitDescription }
};

HAP_RESULT_USE_CHECK
static const char* _Nullable HAPCharacteristicValue_FirmwareUpdateReadiness_UpdateNotReady_GetBitDescription(
        uint32_t value) {
    switch (value) {
        case kHAPCharacteristicValue_FirmwareUpdateReadiness_UpdateNotReady_Other:
            return "Other";
        case kHAPCharacteristicValue_FirmwareUpdateReadiness_UpdateNotReady_LowBattery:
            return "Low Battery";
        case kHAPCharacteristicValue_FirmwareUpdateReadiness_UpdateNotReady_StagedUnavailable:
            return "Staged Firmware Update Unavailable";
        case kHAPCharacteristicValue_FirmwareUpdateReadiness_UpdateNotReady_CriticalOperation:
            return "Critical Operation In-Progress";
        default:
            return NULL;
    }
}

static const HAPUInt32TLVFormat kHAPCharacteristicTLVFormat_FirmwareUpdateReadiness_UpdateNotReadyReason = {
    .type = kHAPTLVFormatType_UInt32,
    .constraints = { .minimumValue = 0, .maximumValue = UINT32_MAX },
    .callbacks = { .getBitDescription =
                           HAPCharacteristicValue_FirmwareUpdateReadiness_UpdateNotReady_GetBitDescription }
};

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_FirmwareUpdateReadiness[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_FirmwareUpdateReadiness, stagingNotReadyReason),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_FirmwareUpdateReadiness, stagingNotReadyReasonIsSet),
            .tlvType = kHAPCharacteristicTLVType_FirmwareUpdateReadiness_StagingNotReadyReason,
            .debugDescription = kHAPCharacteristicTLVDescription_FirmwareUpdateReadiness_StagingNotReadyReason,
            .format = &kHAPCharacteristicTLVFormat_FirmwareUpdateReadiness_StagingNotReadyReason,
            .isOptional = true },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_FirmwareUpdateReadiness, updateNotReadyReason),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_FirmwareUpdateReadiness, updateNotReadyReasonIsSet),
            .tlvType = kHAPCharacteristicTLVType_FirmwareUpdateReadiness_UpdateNotReadyReason,
            .debugDescription = kHAPCharacteristicTLVDescription_FirmwareUpdateReadiness_UpdateNotReadyReason,
            .format = &kHAPCharacteristicTLVFormat_FirmwareUpdateReadiness_UpdateNotReadyReason,
            .isOptional = true },
    NULL
};

const HAPCharacteristicTLVFormat_FirmwareUpdateReadiness kHAPCharacteristicTLVFormat_FirmwareUpdateReadiness = {
    .type = kHAPTLVFormatType_Struct,
    .members = kHAPCharacteristicTLVMembers_FirmwareUpdateReadiness
};

const HAPUUID kHAPCharacteristicType_FirmwareUpdateStatus = HAPUUIDCreateAppleDefined(0x235U);

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState_IsValid(uint8_t value) {
    HAPAssert(sizeof value == sizeof(HAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState));
    switch ((HAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState) value) {
        case kHAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState_Idle:
        case kHAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState_StagingInProgress:
        case kHAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState_StagingPaused:
        case kHAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState_StagingSucceeded:
        case kHAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState_UpdateInProgress: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
static const char* HAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState_GetDescription(
        HAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState value) {
    HAPPrecondition(HAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState_IsValid(value));
    switch (value) {
        case kHAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState_Idle:
            return "Idle";
        case kHAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState_StagingInProgress:
            return "Staging In-Progress";
        case kHAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState_StagingPaused:
            return "Staging Paused";
        case kHAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState_StagingSucceeded:
            return "Staging Succeeded";
        case kHAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState_UpdateInProgress:
            return "Update In-Progress";
        default:
            HAPFatalError();
    }
}

static const HAPCharacteristicTLVFormat_FirmwareUpdateStatus_FirmwareUpdateState
        kHAPCharacteristicTLVFormat_FirmwareUpdateStatus_FirmwareUpdateState = {
            .type = kHAPTLVFormatType_Enum,
            .callbacks = { .isValid = HAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState_IsValid,
                           .getDescription =
                                   HAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState_GetDescription }
        };

static const HAPUInt16TLVFormat kHAPCharacteristicTLVFormat_FirmwareUpdateStatus_UpdateDuration = {
    .type = kHAPTLVFormatType_UInt16,
    .constraints = { .minimumValue = 0, .maximumValue = UINT16_MAX }
};

static const HAPStringTLVFormat kHAPCharacteristicTLVFormat_FirmwareUpdateStatus_StagedFirmwareVersion = {
    .type = kHAPTLVFormatType_String,
    .constraints = { .minLength = 1, .maxLength = kHAPFirmwareVersion_MaxLength }
};

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_FirmwareUpdateStatus[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_FirmwareUpdateStatus, firmwareUpdateState),
            .tlvType = kHAPCharacteristicTLVType_FirmwareUpdateStatus_FirmwareUpdateState,
            .debugDescription = kHAPCharacteristicTLVDescription_FirmwareUpdateStatus_FirmwareUpdateState,
            .format = &kHAPCharacteristicTLVFormat_FirmwareUpdateStatus_FirmwareUpdateState },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_FirmwareUpdateStatus, updateDuration),
            .tlvType = kHAPCharacteristicTLVType_FirmwareUpdateStatus_UpdateDuration,
            .debugDescription = kHAPCharacteristicTLVDescription_FirmwareUpdateStatus_UpdateDuration,
            .format = &kHAPCharacteristicTLVFormat_FirmwareUpdateStatus_UpdateDuration },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_FirmwareUpdateStatus, stagedFirmwareVersion),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_FirmwareUpdateStatus, stagedFirmwareVersionIsSet),
            .tlvType = kHAPCharacteristicTLVType_FirmwareUpdateStatus_StagedFirmwareVersion,
            .debugDescription = kHAPCharacteristicTLVDescription_FirmwareUpdateStatus_StagedFirmwareVersion,
            .format = &kHAPCharacteristicTLVFormat_FirmwareUpdateStatus_StagedFirmwareVersion,
            .isOptional = true },
    NULL
};

const HAPCharacteristicTLVFormat_FirmwareUpdateStatus kHAPCharacteristicTLVFormat_FirmwareUpdateStatus = {
    .type = kHAPTLVFormatType_Struct,
    .members = kHAPCharacteristicTLVMembers_FirmwareUpdateStatus
};

const HAPUUID kHAPCharacteristicType_OperatingStateResponse = HAPUUIDCreateAppleDefined(0x232U);

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_OperatingStateResponse_StateType_IsValid(
        HAPCharacteristicValue_OperatingStateResponse_State value) {
    HAPAssert(sizeof value == sizeof(HAPCharacteristicValue_OperatingStateResponse_State));
    switch ((HAPCharacteristicValue_OperatingStateResponse_State) value) {
        case kHAPCharacteristicValue_OperatingStateResponse_State_Normal:
        case kHAPCharacteristicValue_OperatingStateResponse_State_LimitedFunctionality:
        case kHAPCharacteristicValue_OperatingStateResponse_State_ShutdownImminent: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
const char* HAPCharacteristicValue_OperatingStateResponse_StateType_GetDescription(
        HAPCharacteristicValue_OperatingStateResponse_State value) {
    HAPPrecondition(HAPCharacteristicValue_OperatingStateResponse_StateType_IsValid(value));
    switch (value) {
        case kHAPCharacteristicValue_OperatingStateResponse_State_Normal:
            return "Normal Operating State";
        case kHAPCharacteristicValue_OperatingStateResponse_State_LimitedFunctionality:
            return "Limited Functionality";
        case kHAPCharacteristicValue_OperatingStateResponse_State_ShutdownImminent:
            return "Shutdown Imminent";
        default:
            return "Incorrect";
    }
}

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_OperatingStateResponse_AbnormalReason_IsValid(
        HAPCharacteristicValue_OperatingStateResponse_AbnormalReason value) {
    HAPAssert(sizeof value == sizeof(HAPCharacteristicValue_OperatingStateResponse_AbnormalReason));
    switch ((HAPCharacteristicValue_OperatingStateResponse_AbnormalReason) value) {
        case kHAPCharacteristicValue_OperatingStateResponse_AbnormalReason_Other:
        case kHAPCharacteristicValue_OperatingStateResponse_AbnormalReason_LowTemperature:
        case kHAPCharacteristicValue_OperatingStateResponse_AbnormalReason_HighTemperature: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
const char* HAPCharacteristicValue_OperatingStateResponse_AbnormalReason_GetDescription(
        HAPCharacteristicValue_OperatingStateResponse_AbnormalReason value) {
    HAPPrecondition(HAPCharacteristicValue_OperatingStateResponse_AbnormalReason_IsValid(value));
    switch (value) {
        case kHAPCharacteristicValue_OperatingStateResponse_AbnormalReason_Other:
            return "Other";
        case kHAPCharacteristicValue_OperatingStateResponse_AbnormalReason_LowTemperature:
            return "Low Temperature";
        case kHAPCharacteristicValue_OperatingStateResponse_AbnormalReason_HighTemperature:
            return "High Temperature";
        default:
            return "Incorrect";
    }
}

const HAPUUID kHAPCharacteristicType_AccessoryRuntimeInformationSleepInterval = HAPUUIDCreateAppleDefined(0x23AU);
const HAPUUID kHAPCharacteristicType_AccessoryRuntimeInformationPing = HAPUUIDCreateAppleDefined(0x23CU);
const HAPUUID kHAPCharacteristicType_AccessoryRuntimeInformationHeartBeat = HAPUUIDCreateAppleDefined(0x24AU);

const HAPUUID kHAPCharacteristicType_SupportedDiagnosticsSnapshot = HAPUUIDCreateAppleDefined(0x238U);

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_SupportedDiagnosticsSnapshot_IsValid(
        HAPCharacteristicValue_SupportedDiagnosticsSnapshot value) {
    HAPAssert(sizeof value == sizeof(HAPCharacteristicValue_SupportedDiagnosticsSnapshot));
    switch ((HAPCharacteristicValue_SupportedDiagnosticsSnapshot) value) {
        case kHAPCharacteristicValue_SupportedDiagnosticsSnapshot_Format:
        case kHAPCharacteristicValue_SupportedDiagnosticsSnapshot_Options:
        case kHAPCharacteristicValue_SupportedDiagnosticsSnapshot_Type: {
            return true;
        }
    }
    return false;
}

HAP_RESULT_USE_CHECK
const char* HAPCharacteristicValue_SupportedDiagnosticsSnapshot_GetDescription(
        HAPCharacteristicValue_SupportedDiagnosticsSnapshot value) {
    HAPPrecondition(HAPCharacteristicValue_SupportedDiagnosticsSnapshot_IsValid(value));
    switch (value) {
        case kHAPCharacteristicValue_SupportedDiagnosticsSnapshot_Format:
            return "Format";
        case kHAPCharacteristicValue_SupportedDiagnosticsSnapshot_Type:
            return "Type";
        case kHAPCharacteristicValue_SupportedDiagnosticsSnapshot_Options:
            return "Options";
    }
    return NULL;
}

const HAPUUID kHAPCharacteristicType_SupportedDiagnosticsModes = HAPUUIDCreateAppleDefined(0x24CU);

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_SupportedDiagnosticsModes_IsValid(
        HAPCharacteristicValue_SupportedDiagnosticsModes value) {
    HAPAssert(sizeof value == sizeof(HAPCharacteristicValue_SupportedDiagnosticsModes));
    if (value == kHAPCharacteristicValue_SupportedDiagnosticsModes_None) {
        return true;
    } else if (value == kHAPCharacteristicValue_SupportedDiagnosticsModes_VerboseLogging) {
        return true;
    } else {
        return false;
    }
}

HAP_RESULT_USE_CHECK
const char* HAPCharacteristicValue_SupportedDiagnosticsModes_GetDescription(
        HAPCharacteristicValue_SupportedDiagnosticsModes value) {
    HAPPrecondition(HAPCharacteristicValue_SupportedDiagnosticsModes_IsValid(value));
    if (value == kHAPCharacteristicValue_SupportedDiagnosticsModes_None) {
        return "None";
    } else if (value == kHAPCharacteristicValue_SupportedDiagnosticsModes_VerboseLogging) {
        return "Verbose Logging";
    } else {
        return NULL;
    }
}

const HAPUUID kHAPCharacteristicType_SelectedDiagnosticsModes = HAPUUIDCreateAppleDefined(0x24DU);

HAP_RESULT_USE_CHECK
static bool
        HAPCharacteristicValue_SelectedDiagnosticsModes_IsValid(HAPCharacteristicValue_SelectedDiagnosticsModes value) {
    HAPAssert(sizeof value == sizeof(HAPCharacteristicValue_SelectedDiagnosticsModes));
    if (value == kHAPCharacteristicValue_SelectedDiagnosticsModes_None) {
        return true;
    } else if (value == kHAPCharacteristicValue_SelectedDiagnosticsModes_VerboseLogging) {
        return true;
    } else {
        return false;
    }
}

HAP_RESULT_USE_CHECK
const char* HAPCharacteristicValue_SelectedDiagnosticsModes_GetDescription(
        HAPCharacteristicValue_SelectedDiagnosticsModes value) {
    HAPPrecondition(HAPCharacteristicValue_SelectedDiagnosticsModes_IsValid(value));
    if (value == kHAPCharacteristicValue_SelectedDiagnosticsModes_None) {
        return "None";
    } else if (value == kHAPCharacteristicValue_SelectedDiagnosticsModes_VerboseLogging) {
        return "Verbose Logging";
    } else {
        return NULL;
    }
}

const HAPUUID kHAPCharacteristicType_ThreadManagementOpenThreadVersion = HAPUUIDCreateAppleDefined(0x706U);

const HAPUUID kHAPCharacteristicType_ThreadManagementNodeCapabilities = HAPUUIDCreateAppleDefined(0x702U);

const HAPUUID kHAPCharacteristicType_ThreadManagementStatus = HAPUUIDCreateAppleDefined(0x703U);

const HAPUUID kHAPCharacteristicType_ThreadManagementControl = HAPUUIDCreateAppleDefined(0x704U);

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_ThreadManagementControl_OperationType_IsValid(uint8_t value) {
    HAPAssert(sizeof value == sizeof(HAPCharacteristicValue_ThreadManagementControl_OperationType));
    switch ((HAPCharacteristicValue_ThreadManagementControl_OperationType) value) {
        case kHAPCharacteristicValue_ThreadManagementControl_OperationType_SetThreadParameters:   // HAP_FALLTHROUGH
        case kHAPCharacteristicValue_ThreadManagementControl_OperationType_ClearThreadParameters: // HAP_FALLTHROUGH
        case kHAPCharacteristicValue_ThreadManagementControl_OperationType_ReadThreadParameters:  // HAP_FALLTHROUGH
        case kHAPCharacteristicValue_ThreadManagementControl_OperationType_InitiateThreadJoiner: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
static const char* HAPCharacteristicValue_ThreadManagementControl_OperationType_GetDescription(
        HAPCharacteristicValue_ThreadManagementControl_OperationType value) {
    switch (value) {
        case kHAPCharacteristicValue_ThreadManagementControl_OperationType_SetThreadParameters:
            return "Set Thread Parameters";
        case kHAPCharacteristicValue_ThreadManagementControl_OperationType_ClearThreadParameters:
            return "Clear Thread Parameters";
        case kHAPCharacteristicValue_ThreadManagementControl_OperationType_ReadThreadParameters:
            return "Read Thread Parameters";
        case kHAPCharacteristicValue_ThreadManagementControl_OperationType_InitiateThreadJoiner:
            return "Initiate Thread Joiner";
        default:
            HAPFatalError();
    }
}

static const HAPCharacteristicTLVFormat_ThreadManagementControl_OperationType
    kHAPCharacteristicTLVFormat_ThreadManagementControl_OperationType = {
        .type = kHAPTLVFormatType_Enum,
        .callbacks = { .isValid = HAPCharacteristicValue_ThreadManagementControl_OperationType_IsValid,
            .getDescription = HAPCharacteristicValue_ThreadManagementControl_OperationType_GetDescription, },
    };

static const HAPStringTLVFormat kHAPCharacteristicTLVFormat_ThreadManagementControl_NetworkCredentials_NetworkName = {
    .type = kHAPTLVFormatType_String,
    .constraints = { .minLength = 1, .maxLength = SIZE_MAX }
};

static const HAPUInt16TLVFormat kHAPCharacteristicTLVFormat_ThreadManagementControl_NetworkCredentials_Channel = {
    .type = kHAPTLVFormatType_UInt16,
    .constraints = { .minimumValue = 11, .maximumValue = 26 },
};

static const HAPUInt16TLVFormat kHAPCharacteristicTLVFormat_ThreadManagementControl_NetworkCredentials_PanId = {
    .type = kHAPTLVFormatType_UInt16,
    .constraints = { .minimumValue = 0, .maximumValue = 0xFFFE },
};

static const HAPDataTLVFormat kHAPCharacteristicTLVFormat_ThreadManagementControl_NetworkCredentials_ExtPanId = {
    .type = kHAPTLVFormatType_Data,
    .constraints = { .minLength = 8, .maxLength = 8 },
};

static const HAPDataTLVFormat kHAPCharacteristicTLVFormat_ThreadManagementControl_NetworkCredentials_MasterKey = {
    .type = kHAPTLVFormatType_Data,
    .constraints = { .minLength = 16, .maxLength = 16 },
};

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_ThreadManagementControl_NetworkCredentials[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_ThreadManagementControl_NetworkCredentials, networkName),
            .tlvType = kHAPCharacteristicTLVType_ThreadManagementControl_NetworkCredentials_NetworkName,
            .debugDescription = kHAPCharacteristicTLVDescription_ThreadManagementControl_NetworkCredentials_NetworkName,
            .format = &kHAPCharacteristicTLVFormat_ThreadManagementControl_NetworkCredentials_NetworkName,
    },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_ThreadManagementControl_NetworkCredentials, channel),
            .tlvType = kHAPCharacteristicTLVType_ThreadManagementControl_NetworkCredentials_Channel,
            .debugDescription = kHAPCharacteristicTLVDescription_ThreadManagementControl_NetworkCredentials_Channel,
            .format = &kHAPCharacteristicTLVFormat_ThreadManagementControl_NetworkCredentials_Channel,
    },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_ThreadManagementControl_NetworkCredentials, panId),
            .tlvType = kHAPCharacteristicTLVType_ThreadManagementControl_NetworkCredentials_PanId,
            .debugDescription = kHAPCharacteristicTLVDescription_ThreadManagementControl_NetworkCredentials_PanId,
            .format = &kHAPCharacteristicTLVFormat_ThreadManagementControl_NetworkCredentials_PanId,
    },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_ThreadManagementControl_NetworkCredentials, extPanId),
            .tlvType = kHAPCharacteristicTLVType_ThreadManagementControl_NetworkCredentials_ExtPanId,
            .debugDescription = kHAPCharacteristicTLVDescription_ThreadManagementControl_NetworkCredentials_ExtPanId,
            .format = &kHAPCharacteristicTLVFormat_ThreadManagementControl_NetworkCredentials_ExtPanId,
    },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_ThreadManagementControl_NetworkCredentials, masterKey),
            .isSetOffset =
                    HAP_OFFSETOF(HAPCharacteristicValue_ThreadManagementControl_NetworkCredentials, masterKeyIsSet),
            .tlvType = kHAPCharacteristicTLVType_ThreadManagementControl_NetworkCredentials_MasterKey,
            .debugDescription = kHAPCharacteristicTLVDescription_ThreadManagementControl_NetworkCredentials_MasterKey,
            .format = &kHAPCharacteristicTLVFormat_ThreadManagementControl_NetworkCredentials_MasterKey,
            .isOptional = true,
    },
    NULL,
};

static const HAPStructTLVFormat kHAPCharacteristicTLVFormat_ThreadManagementControl_NetworkCredentials = {
    .type = kHAPTLVFormatType_Struct,
    .members = kHAPCharacteristicTLVMembers_ThreadManagementControl_NetworkCredentials
};

static const HAPUInt8TLVFormat kHAPCharacteristicTLVFormat_ThreadManagementControl_FormingAllowed = {
    .type = kHAPTLVFormatType_UInt8,
    .constraints = { .minimumValue = 0, .maximumValue = 1 },
};

static const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_ThreadManagementControl[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_ThreadManagementControl, operationType),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_ThreadManagementControl, operationTypeIsSet),
            .tlvType = kHAPCharacteristicTLVType_ThreadManagementControl_OperationType,
            .debugDescription = kHAPCharacteristicTLVDescription_ThreadManagementControl_OperationType,
            .format = &kHAPCharacteristicTLVFormat_ThreadManagementControl_OperationType,
            .isOptional = true },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_ThreadManagementControl, networkCredentials),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_ThreadManagementControl, networkCredentialsIsSet),
            .tlvType = kHAPCharacteristicTLVType_ThreadManagementControl_NetworkCredentials,
            .debugDescription = kHAPCharacteristicTLVDescription_ThreadManagementControl_NetworkCredentials,
            .format = &kHAPCharacteristicTLVFormat_ThreadManagementControl_NetworkCredentials,
            .isOptional = true },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_ThreadManagementControl, formingAllowed),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_ThreadManagementControl, formingAllowedIsSet),
            .tlvType = kHAPCharacteristicTLVType_ThreadManagementControl_FormingAllowed,
            .debugDescription = kHAPCharacteristicTLVDescription_ThreadManagementControl_FormingAllowed,
            .format = &kHAPCharacteristicTLVFormat_ThreadManagementControl_FormingAllowed,
            .isOptional = true },
    NULL,
};

const HAPCharacteristicTLVFormat_ThreadManagementControl kHAPCharacteristicTLVFormat_ThreadManagementControl = {
    .type = kHAPTLVFormatType_Struct,
    .members = kHAPCharacteristicTLVMembers_ThreadManagementControl
};

//----------------------------------------------------------------------------------------------------------------------

const HAPDataTLVFormat kHAPCharacteristicTLVFormat_VariableLengthInteger4 = {
    .type = kHAPTLVFormatType_Data,
    .constraints = { .minLength = 0, .maxLength = 4 },
};

const HAPDataTLVFormat kHAPCharacteristicTLVFormat_VariableLengthInteger8 = {
    .type = kHAPTLVFormatType_Data,
    .constraints = { .minLength = 0, .maxLength = 8 },
};

const HAPUInt32TLVFormat kHAPCharacteristicTLVFormat_Float4 = {
    .type = kHAPTLVFormatType_UInt32,
    .constraints = { .minimumValue = 0, .maximumValue = UINT32_MAX },
};

/**
 * Supported Transition Configuration.
 */
const HAPUUID kHAPCharacteristicType_SupportedTransitionConfiguration = HAPUUIDCreateAppleDefined(0x144U);

HAP_RESULT_USE_CHECK
static const char* HAPCharacteristicValue_SupportedTransitionConfiguration_SupportedTransition_Types_GetBitDescription(
        uint32_t value) {
    switch (value) {
        case kHAPCharacteristicValue_SupportedTransitionConfiguration_SupportedTransition_Type_Linear:
            return "Linear";
        case kHAPCharacteristicValue_SupportedTransitionConfiguration_SupportedTransition_Type_LinearDerived:
            return "Linear Derived";
        default:
            return NULL;
    }
}

const HAPUInt32TLVFormat kHAPCharacteristicTLVFormat_SupportedTransitionConfiguration_SupportedTransition_Type = {
    .type = kHAPTLVFormatType_UInt32,
    .constraints = { .minimumValue = 0, .maximumValue = UINT32_MAX },
    .callbacks = { .getBitDescription =
                           HAPCharacteristicValue_SupportedTransitionConfiguration_SupportedTransition_Types_GetBitDescription }
};

const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_SupportedTransition[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_SupportedTransition, HAPInstanceID),
            .tlvType = kHAPCharacteristicTLVType_SupportedTransitionConfiguration_SupportedTransition_HAPID,
            .debugDescription =
                    kHAPCharacteristicTLVDescription_SupportedTransitionConfiguration_SupportedTransition_HAPID,
            .format = &kHAPCharacteristicTLVFormat_VariableLengthInteger8 },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_SupportedTransition, types),
            .tlvType = kHAPCharacteristicTLVType_SupportedTransitionConfiguration_SupportedTransition_Types,
            .debugDescription =
                    kHAPCharacteristicTLVDescription_SupportedTransitionConfiguration_SupportedTransition_Types,
            .format = &kHAPCharacteristicTLVFormat_SupportedTransitionConfiguration_SupportedTransition_Type },
    NULL
};

const HAPCharacteristicTLVFormat_SupportedTransition kHAPCharacteristicTLVFormat_SupportedTransition = {
    .type = kHAPTLVFormatType_Struct,
    .members = kHAPCharacteristicTLVMembers_SupportedTransition
};

const HAPCharacteristicTLVFormat_SupportedTransitionConfigurations
        kHAPCharacteristicTLVFormat_SupportedTransitionConfigurations = {
            .type = kHAPTLVFormatType_Sequence,
            .item = { .valueOffset =
                              HAP_OFFSETOF(HAPCharacteristicValue_SupportedTransitionConfigurations, transitions),
                      .tlvType = kHAPCharacteristicTLVType_SupportedTransitionConfiguration_SupportedTransitions,
                      .debugDescription =
                              kHAPCharacteristicTLVDescription_SupportedTransitionConfiguration_SupportedTransitions,
                      .format = &kHAPCharacteristicTLVFormat_SupportedTransition },
            .separator = { .tlvType = kHAPCharacteristicTLVType_Separator,
                           .debugDescription = kHAPCharacteristicTLVDescription_Separator,
                           .format = &kHAPCharacteristicTLVFormat_Separator }
        };

/**
 * Transition Control.
 */
const HAPUUID kHAPCharacteristicType_TransitionControl = HAPUUIDCreateAppleDefined(0x143U);

const HAPDataTLVFormat kHAPCharacteristicTLVFormat_TransitionControl_ControllerContext = {
    .type = kHAPTLVFormatType_Data,
    .constraints = { .minLength = 0, .maxLength = 256 },
};

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_TransitionControl_Transition_EndBehavior_IsValid(uint8_t value) {
    HAPAssert(sizeof value == sizeof(HAPCharacteristicValue_TransitionControl_EndBehavior));
    switch ((HAPCharacteristicValue_TransitionControl_EndBehavior) value) {
        case kHAPCharacteristicValue_TransitionControl_EndBehavior_NoChange:
        case kHAPCharacteristicValue_TransitionControl_EndBehavior_Loop: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
const char* HAPCharacteristicValue_TransitionControl_Transition_EndBehavior_GetDescription(
        HAPCharacteristicValue_TransitionControl_EndBehavior value) {
    HAPPrecondition(HAPCharacteristicValue_TransitionControl_Transition_EndBehavior_IsValid(value));
    switch (value) {
        case kHAPCharacteristicValue_TransitionControl_EndBehavior_NoChange:
            return "No Change";
        case kHAPCharacteristicValue_TransitionControl_EndBehavior_Loop:
            return "Loop";
        default:
            HAPFatalError();
    }
}

const HAPCharacteristicTLVFormat_TransitionControl_Transition_EndBehavior
        kHAPCharacteristicTLVFormat_TransitionControl_Transition_EndBehavior = {
            .type = kHAPTLVFormatType_Enum,
            .callbacks = { .isValid = HAPCharacteristicValue_TransitionControl_Transition_EndBehavior_IsValid,
                           .getDescription =
                                   HAPCharacteristicValue_TransitionControl_Transition_EndBehavior_GetDescription }
        };

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_TransitionControl_StartCondition_IsValid(uint8_t value) {
    HAPAssert(sizeof value == sizeof(HAPCharacteristicValue_TransitionControl_StartCondition));
    switch ((HAPCharacteristicValue_TransitionControl_StartCondition) value) {
        case kHAPCharacteristicValue_TransitionControl_StartCondition_None:
        case kHAPCharacteristicValue_TransitionControl_StartCondition_Ascends:
        case kHAPCharacteristicValue_TransitionControl_StartCondition_Descends: {
            return true;
        }
        default:
            return false;
    }
}

/**
 * Linear Transition.
 */
HAP_RESULT_USE_CHECK
const char* HAPCharacteristicValue_TransitionControl_StartCondition_GetDescription(
        HAPCharacteristicValue_TransitionControl_StartCondition value) {
    HAPPrecondition(HAPCharacteristicValue_TransitionControl_StartCondition_IsValid(value));
    switch (value) {
        case kHAPCharacteristicValue_TransitionControl_StartCondition_None:
            return "Always set curve ";
        case kHAPCharacteristicValue_TransitionControl_StartCondition_Ascends:
            return "Set if curve value is greater ";
        case kHAPCharacteristicValue_TransitionControl_StartCondition_Descends:
            return "Set if curve value is lesser";
        default:
            HAPFatalError();
    }
}

const HAPCharacteristicTLVFormat_TransitionControl_LinearTransition_StartCondition
        kHAPCharacteristicTLVFormat_TransitionControl_LinearTransition_StartCondition = {
            .type = kHAPTLVFormatType_Enum,
            .callbacks = { .isValid = HAPCharacteristicValue_TransitionControl_StartCondition_IsValid,
                           .getDescription = HAPCharacteristicValue_TransitionControl_StartCondition_GetDescription }
        };

// Linear Transition
const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_TransitionControl_LinearTransition_Points[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_TransitionControl_LinearTransition_Point, targetValue),
            .tlvType = kHAPCharacteristicTLVType_TransitionControl_LinearTransitionPoint_TargetValue,
            .debugDescription = kHAPCharacteristicTLVDescription_TransitionControl_LinearTransitionPoint_TargetValue,
            .format = &kHAPCharacteristicTLVFormat_VariableLengthInteger4 },
    &(const HAPStructTLVMember) {
            .valueOffset =
                    HAP_OFFSETOF(HAPCharacteristicValue_TransitionControl_LinearTransition_Point, completionDuration),
            .isSetOffset = HAP_OFFSETOF(
                    HAPCharacteristicValue_TransitionControl_LinearTransition_Point,
                    completionDurationIsSet),
            .tlvType = kHAPCharacteristicTLVType_TransitionControl_LinearTransitionPoint_TargetCompletionDuration,
            .debugDescription =
                    kHAPCharacteristicTLVDescription_TransitionControl_LinearTransitionPoint_TargetCompletionDuration,
            .format = &kHAPCharacteristicTLVFormat_VariableLengthInteger8,
            .isOptional = true },
    &(const HAPStructTLVMember) {
            .valueOffset =
                    HAP_OFFSETOF(HAPCharacteristicValue_TransitionControl_LinearTransition_Point, startDelayDuration),
            .isSetOffset = HAP_OFFSETOF(
                    HAPCharacteristicValue_TransitionControl_LinearTransition_Point,
                    startDelayDurationIsSet),
            .tlvType = kHAPCharacteristicTLVType_TransitionControl_LinearTransitionPoint_StartDelayDuration,
            .debugDescription =
                    kHAPCharacteristicTLVDescription_TransitionControl_LinearTransitionPoint_StartDelayDuration,
            .format = &kHAPCharacteristicTLVFormat_VariableLengthInteger8,
            .isOptional = true },
    NULL
};

const HAPCharacteristicTLVFormat_TransitionControl_LinearTransition_Points
        kHAPCharacteristicTLVFormat_TransitionControl_LinearTransition_Points = {
            .type = kHAPTLVFormatType_Struct,
            .members = kHAPCharacteristicTLVMembers_TransitionControl_LinearTransition_Points
        };

const HAPCharacteristicTLVFormat_TransitionControl_LinearTransition_PointList
        kHAPCharacteristicTLVFormat_TransitionControl_LinearTransition_PointList = {
            .type = kHAPTLVFormatType_Sequence,
            .item = { .valueOffset =
                              HAP_OFFSETOF(HAPCharacteristicValue_TransitionControl_LinearTransition_PointList, point),
                      .tlvType = kHAPCharacteristicTLVType_TransitionControl_LinearTransition_Points,
                      .debugDescription = kHAPCharacteristicTLVDescription_TransitionControl_LinearTransition_Points,
                      .format = &kHAPCharacteristicTLVFormat_TransitionControl_LinearTransition_Points },
            .separator = { .tlvType = kHAPCharacteristicTLVType_Separator,
                           .debugDescription = kHAPCharacteristicTLVDescription_Separator,
                           .format = &kHAPCharacteristicTLVFormat_Separator }
        };

const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_TransitionControl_Transition_Linear[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_TransitionControl_LinearTransition, points),
            .tlvType = kHAPCharacteristicTLVType_TransitionControl_LinearTransition_Points,
            .debugDescription = kHAPCharacteristicTLVDescription_TransitionControl_LinearTransition_Points,
            .format = &kHAPCharacteristicTLVFormat_TransitionControl_LinearTransition_PointList },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_TransitionControl_LinearTransition, startCondition),
            .tlvType = kHAPCharacteristicTLVType_TransitionControl_LinearTransition_StartCondition,
            .debugDescription = kHAPCharacteristicTLVDescription_TransitionControl_LinearTransition_StartCondition,
            .format = &kHAPCharacteristicTLVFormat_TransitionControl_LinearTransition_StartCondition },
    NULL
};

const HAPCharacteristicTLVFormat_TransitionControl_Transition_Linear
        kHAPCharacteristicTLVFormat_TransitionControl_Transition_Linear = {
            .type = kHAPTLVFormatType_Struct,
            .members = kHAPCharacteristicTLVMembers_TransitionControl_Transition_Linear
        };

/**
 * Linear Derived Transition
 */
const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_TransitionControl_LinearDerivedTransition_Points[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_TransitionControl_LinearDerivedTransition_Point, scale),
            .tlvType = kHAPCharacteristicTLVType_TransitionControl_LinearDerivedTransitionPoint_Scale,
            .debugDescription = kHAPCharacteristicTLVDescription_TransitionControl_LinearDerivedTransitionPoint_Scale,
            .format = &kHAPCharacteristicTLVFormat_Float4 },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_TransitionControl_LinearDerivedTransition_Point, offset),
            .tlvType = kHAPCharacteristicTLVType_TransitionControl_LinearDerivedTransitionPoint_Offset,
            .debugDescription = kHAPCharacteristicTLVDescription_TransitionControl_LinearDerivedTransitionPoint_Offset,
            .format = &kHAPCharacteristicTLVFormat_Float4 },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(
                    HAPCharacteristicValue_TransitionControl_LinearDerivedTransition_Point,
                    completionDuration),
            .isSetOffset = HAP_OFFSETOF(
                    HAPCharacteristicValue_TransitionControl_LinearDerivedTransition_Point,
                    completionDurationIsSet),
            .tlvType =
                    kHAPCharacteristicTLVType_TransitionControl_LinearDerivedTransitionPoint_TargetCompletionDuration,
            .debugDescription =
                    kHAPCharacteristicTLVDescription_TransitionControl_LinearDerivedTransitionPoint_TargetCompletionDuration,
            .format = &kHAPCharacteristicTLVFormat_VariableLengthInteger8,
            .isOptional = true },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(
                    HAPCharacteristicValue_TransitionControl_LinearDerivedTransition_Point,
                    startDelayDuration),
            .isSetOffset = HAP_OFFSETOF(
                    HAPCharacteristicValue_TransitionControl_LinearDerivedTransition_Point,
                    startDelayDurationIsSet),
            .tlvType = kHAPCharacteristicTLVType_TransitionControl_LinearDerivedTransitionPoint_StartDelayDuration,
            .debugDescription =
                    kHAPCharacteristicTLVDescription_TransitionControl_LinearDerivedTransitionPoint_StartDelayDuration,
            .format = &kHAPCharacteristicTLVFormat_VariableLengthInteger8,
            .isOptional = true },
    NULL
};

const HAPCharacteristicTLVFormat_TransitionControl_LinearDerivedTransition_Points
        kHAPCharacteristicTLVFormat_TransitionControl_LinearDerivedTransition_Points = {
            .type = kHAPTLVFormatType_Struct,
            .members = kHAPCharacteristicTLVMembers_TransitionControl_LinearDerivedTransition_Points
        };

const HAPCharacteristicTLVFormat_TransitionControl_LinearDerivedTransition_PointList
        kHAPCharacteristicTLVFormat_TransitionControl_LinearDerivedTransition_PointList = {
            .type = kHAPTLVFormatType_Sequence,
            .item = { .valueOffset = HAP_OFFSETOF(
                              HAPCharacteristicValue_TransitionControl_LinearDerivedTransition_PointList,
                              point),
                      .tlvType = kHAPCharacteristicTLVType_TransitionControl_LinearDerivedTransition_Points,
                      .debugDescription =
                              kHAPCharacteristicTLVDescription_TransitionControl_LinearDerivedTransition_Points,
                      .format = &kHAPCharacteristicTLVFormat_TransitionControl_LinearDerivedTransition_Points },
            .separator = { .tlvType = kHAPCharacteristicTLVType_Separator,
                           .debugDescription = kHAPCharacteristicTLVDescription_Separator,
                           .format = &kHAPCharacteristicTLVFormat_Separator }
        };

/**
 * Source Value Range
 */
const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_TransitionControl_SourceValueRange[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_TransitionControl_SourceValueRange, lower),
            .tlvType = kHAPCharacteristicTLVType_TransitionControl_SourceValueRange_Lower,
            .debugDescription = kHAPCharacteristicTLVDescription_TransitionControl_SourceRange_Lower,
            .format = &kHAPCharacteristicTLVFormat_VariableLengthInteger4 },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_TransitionControl_SourceValueRange, upper),
            .tlvType = kHAPCharacteristicTLVType_TransitionControl_SourceValueRange_Upper,
            .debugDescription = kHAPCharacteristicTLVDescription_TransitionControl_SourceRange_Upper,
            .format = &kHAPCharacteristicTLVFormat_VariableLengthInteger4 },
    NULL
};

const HAPCharacteristicTLVFormat_TransitionControl_SourceValueRange
        kHAPCharacteristicTLVFormat_TransitionControl_SourceValueRange = {
            .type = kHAPTLVFormatType_Struct,
            .members = kHAPCharacteristicTLVMembers_TransitionControl_SourceValueRange
        };

const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_TransitionControl_Transition_LinearDerived[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_TransitionControl_LinearDerivedTransition, points),
            .tlvType = kHAPCharacteristicTLVType_TransitionControl_LinearDerivedTransition_Points,
            .debugDescription = kHAPCharacteristicTLVDescription_TransitionControl_LinearDerivedTransition_Points,
            .format = &kHAPCharacteristicTLVFormat_TransitionControl_LinearDerivedTransition_PointList },
    &(const HAPStructTLVMember) {
            .valueOffset =
                    HAP_OFFSETOF(HAPCharacteristicValue_TransitionControl_LinearDerivedTransition, HAPInstanceID),
            .tlvType = kHAPCharacteristicTLVType_TransitionControl_LinearDerivedTransition_SourceID,
            .debugDescription = kHAPCharacteristicTLVDescription_TransitionControl_LinearDerivedTransition_SourceID,
            .format = &kHAPCharacteristicTLVFormat_VariableLengthInteger8 },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_TransitionControl_LinearDerivedTransition, range),
            .tlvType = kHAPCharacteristicTLVType_TransitionControl_LinearDerivedTransition_SourceRange,
            .debugDescription = kHAPCharacteristicTLVDescription_TransitionControl_LinearDerivedTransition_SourceRange,
            .format = &kHAPCharacteristicTLVFormat_TransitionControl_SourceValueRange },
    NULL
};

const HAPCharacteristicTLVFormat_TransitionControl_Transition_LinearDerived
        kHAPCharacteristicTLVFormat_TransitionControl_Transition_LinearDerived = {
            .type = kHAPTLVFormatType_Struct,
            .members = kHAPCharacteristicTLVMembers_TransitionControl_Transition_LinearDerived
        };

//
const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_TransitionControl_Transition[] = {
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_TransitionControl_Transition, HAPInstanceID),
            .tlvType = kHAPCharacteristicTLVType_TransitionControl_Transition_HAPID,
            .debugDescription = kHAPCharacteristicTLVDescription_TransitionControl_Transition_HAPID,
            .format = &kHAPCharacteristicTLVFormat_VariableLengthInteger8 },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_TransitionControl_Transition, context),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_TransitionControl_Transition, contextIsSet),
            .tlvType = kHAPCharacteristicTLVType_TransitionControl_Transition_ControllerContext,
            .debugDescription = kHAPCharacteristicTLVDescription_TransitionControl_Transition_ControllerContext,
            .format = &kHAPCharacteristicTLVFormat_TransitionControl_ControllerContext,
            .isOptional = true },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_TransitionControl_Transition, endBehavior),
            .tlvType = kHAPCharacteristicTLVType_TransitionControl_Transition_EndBehavior,
            .debugDescription = kHAPCharacteristicTLVDescription_TransitionControl_Transition_EndBehavior,
            .format = &kHAPCharacteristicTLVFormat_TransitionControl_Transition_EndBehavior },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_TransitionControl_Transition, linearTransition),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_TransitionControl_Transition, linearTransitionIsSet),
            .tlvType = kHAPCharacteristicTLVType_TransitionControl_Transition_Linear,
            .debugDescription = kHAPCharacteristicTLVDescription_TransitionControl_Transition_Linear,
            .format = &kHAPCharacteristicTLVFormat_TransitionControl_Transition_Linear,
            .isOptional = true },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_TransitionControl_Transition, linearDerivedTransition),
            .isSetOffset =
                    HAP_OFFSETOF(HAPCharacteristicValue_TransitionControl_Transition, linearDerivedTransitionIsSet),
            .tlvType = kHAPCharacteristicTLVType_TransitionControl_Transition_LinearDerived,
            .debugDescription = kHAPCharacteristicTLVDescription_TransitionControl_Transition_LinearDerived,
            .format = &kHAPCharacteristicTLVFormat_TransitionControl_Transition_LinearDerived,
            .isOptional = true },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_TransitionControl_Transition, updateInterval),
            .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_TransitionControl_Transition, updateIntervalIsSet),
            .tlvType = kHAPCharacteristicTLVType_TransitionControl_Transition_ValueUpdateTimeInterval,
            .debugDescription = kHAPCharacteristicTLVDescription_TransitionControl_Transition_ValueUpdateTimeInterval,
            .format = &kHAPCharacteristicTLVFormat_VariableLengthInteger8,
            .isOptional = true },
    &(const HAPStructTLVMember) {
            .valueOffset =
                    HAP_OFFSETOF(HAPCharacteristicValue_TransitionControl_Transition, notifyValueChangeThreshold),
            .isSetOffset =
                    HAP_OFFSETOF(HAPCharacteristicValue_TransitionControl_Transition, notifyValueChangeThresholdIsSet),
            .tlvType = kHAPCharacteristicTLVType_TransitionControl_Transition_NotifyValueChangeThreshold,
            .debugDescription =
                    kHAPCharacteristicTLVDescription_TransitionControl_Transition_NotifyValueChangeThreshold,
            .format = &kHAPCharacteristicTLVFormat_VariableLengthInteger4,
            .isOptional = true },
    &(const HAPStructTLVMember) {
            .valueOffset =
                    HAP_OFFSETOF(HAPCharacteristicValue_TransitionControl_Transition, notifyTimeIntervalThreshold),
            .isSetOffset =
                    HAP_OFFSETOF(HAPCharacteristicValue_TransitionControl_Transition, notifyTimeIntervalThresholdIsSet),
            .tlvType = kHAPCharacteristicTLVType_TransitionControl_Transition_NotifyTimeIntervalThreshold,
            .debugDescription =
                    kHAPCharacteristicTLVDescription_TransitionControl_Transition_NotifyTimeIntervalThreshold,
            .format = &kHAPCharacteristicTLVFormat_VariableLengthInteger8,
            .isOptional = true },
    NULL
};

const HAPCharacteristicTLVFormat_TransitionControl_Transition
        kHAPCharacteristicTLVFormat_TransitionControl_Transition = {
            .type = kHAPTLVFormatType_Struct,
            .members = kHAPCharacteristicTLVMembers_TransitionControl_Transition
        };

const HAPCharacteristicTLVFormat_TransitionControl_Start kHAPCharacteristicTLVFormat_TransitionControl_Start = {
    .type = kHAPTLVFormatType_Sequence,
    .item = { .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_TransitionControl_Start, transitions),
              .tlvType = kHAPCharacteristicTLVType_TransitionControl_Start_Transitions,
              .debugDescription = kHAPCharacteristicTLVDescription_TransitionControl_Start_Transitions,
              .format = &kHAPCharacteristicTLVFormat_TransitionControl_Transition },
    .separator = { .tlvType = kHAPCharacteristicTLVType_Separator,
                   .debugDescription = kHAPCharacteristicTLVDescription_Separator,
                   .format = &kHAPCharacteristicTLVFormat_Separator }
};

const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_TransitionControl_Fetch[] = {
    &(const HAPStructTLVMember) { .valueOffset =
                                          HAP_OFFSETOF(HAPCharacteristicValue_TransitionControl_Fetch, HAPInstanceID),
                                  .tlvType = kHAPCharacteristicTLVType_TransitionControl_Fetch_HAPID,
                                  .debugDescription = kHAPCharacteristicTLVDescription_TransitionControl_Fetch_HAPID,
                                  .format = &kHAPCharacteristicTLVFormat_VariableLengthInteger8 },
    NULL
};

const HAPCharacteristicTLVFormat_TransitionControl_Fetch kHAPCharacteristicTLVFormat_TransitionControl_Fetch = {
    .type = kHAPTLVFormatType_Struct,
    .members = kHAPCharacteristicTLVMembers_TransitionControl_Fetch
};

const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_TransitionControl[] = {
    &(const HAPStructTLVMember) { .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_TransitionControl, fetch),
                                  .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_TransitionControl, fetchIsSet),
                                  .tlvType = kHAPCharacteristicTLVType_TransitionControl_Fetch,
                                  .debugDescription = kHAPCharacteristicTLVDescription_TransitionControl_Fetch,
                                  .format = &kHAPCharacteristicTLVFormat_TransitionControl_Fetch,
                                  .isOptional = true },
    &(const HAPStructTLVMember) { .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_TransitionControl, start),
                                  .isSetOffset = HAP_OFFSETOF(HAPCharacteristicValue_TransitionControl, startIsSet),
                                  .tlvType = kHAPCharacteristicTLVType_TransitionControl_Start,
                                  .debugDescription = kHAPCharacteristicTLVDescription_TransitionControl_Start,
                                  .format = &kHAPCharacteristicTLVFormat_TransitionControl_Start,
                                  .isOptional = true },
    NULL
};

const HAPCharacteristicTLVFormat_TransitionControl kHAPCharacteristicTLVFormat_TransitionControl = {
    .type = kHAPTLVFormatType_Struct,
    .members = kHAPCharacteristicTLVMembers_TransitionControl
};

const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_ActiveContext[] = {
    &(const HAPStructTLVMember) {
            .valueOffset =
                    HAP_OFFSETOF(HAPCharacteristicValue_TransitionControl_TransitionState_ActiveContext, HAPInstanceID),
            .tlvType = kHAPCharacteristicTLVType_TransitionState_ActiveContext_HAPID,
            .debugDescription = kHAPCharacteristicTLVDescription_TransitionState_ActiveContext_HAPID,
            .format = &kHAPCharacteristicTLVFormat_VariableLengthInteger8 },
    &(const HAPStructTLVMember) {
            .valueOffset =
                    HAP_OFFSETOF(HAPCharacteristicValue_TransitionControl_TransitionState_ActiveContext, context),
            .tlvType = kHAPCharacteristicTLVType_TransitionState_ActiveContext_ControllerContext,
            .debugDescription = kHAPCharacteristicTLVDescription_TransitionState_ActiveContext_ControllerContext,
            .format = &kHAPCharacteristicTLVFormat_TransitionControl_ControllerContext },
    &(const HAPStructTLVMember) {
            .valueOffset = HAP_OFFSETOF(
                    HAPCharacteristicValue_TransitionControl_TransitionState_ActiveContext,
                    timeElapsedSinceStart),
            .tlvType = kHAPCharacteristicTLVType_TransitionState_ActiveContext_TimeElapsedSinceStart,
            .debugDescription = kHAPCharacteristicTLVDescription_TransitionState_ActiveContext_TimeElapsedSinceStart,
            .format = &kHAPCharacteristicTLVFormat_VariableLengthInteger8 },
    NULL
};

const HAPCharacteristicTLVFormat_TransitionState_ActiveContext
        kHAPCharacteristicTLVFormat_TransitionState_ActiveContext = {
            .type = kHAPTLVFormatType_Struct,
            .members = kHAPCharacteristicTLVMembers_ActiveContext
        };

const HAPCharacteristicTLVFormat_TransitionState kHAPCharacteristicTLVFormat_TransitionState = {
    .type = kHAPTLVFormatType_Sequence,
    .item = { .valueOffset = HAP_OFFSETOF(HAPCharacteristicValue_TransitionState, activeContext),
              .tlvType = kHAPCharacteristicTLVType_TransitionState_ActiveContexts,
              .debugDescription = kHAPCharacteristicTLVDescription_TransitionState_ActiveContexts,
              .format = &kHAPCharacteristicTLVFormat_TransitionState_ActiveContext },
    .separator = { .tlvType = kHAPCharacteristicTLVType_Separator,
                   .debugDescription = kHAPCharacteristicTLVDescription_Separator,
                   .format = &kHAPCharacteristicTLVFormat_Separator }
};

/**
 * Transition Count.
 */
const HAPUUID kHAPCharacteristicType_TransitionCount = HAPUUIDCreateAppleDefined(0x24BU);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// 34AB8811-AC7F-4340-BAC3-FD6A85F9943B
const HAPUUID kHAPCharacteristicType_ADKVersion = {
    { 0x3B, 0x94, 0xF9, 0x85, 0x6A, 0xFD, 0xC3, 0xBA, 0x40, 0x43, 0x7F, 0xAC, 0x11, 0x88, 0xAB, 0x34 }
};
