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

#include "TemplateDB.h"
#include "HAPCharacteristicTypes.h"
#if (HAVE_ACCESS_CODE == 1)
#include "HAPRequestHandlers+AccessCode.h"
#endif
#include "HAPRequestHandlers+AccessoryInformation.h"
#include "HAPRequestHandlers+CameraEventRecordingManagement.h"
#include "HAPRequestHandlers+CameraRTPStreamManagement.h"
#include "HAPRequestHandlers+DataStreamTransportManagement.h"
#include "HAPRequestHandlers+HAPProtocolInformation.h"
#if (HAVE_NFC_ACCESS == 1)
#include "HAPRequestHandlers+NfcAccess.h"
#endif
#include "HAPRequestHandlers+Pairing.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_RECONFIGURATION)
#include "HAPRequestHandlers+WiFiReconfiguration.h"
#endif

#include "HAPRequestHandlers+WiFiRouter.h"

#include "HAPRequestHandlers+FirmwareUpdate.h"

#include "HAPRequestHandlers+DataStreamHAPTransport.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)
#include "HAPRequestHandlers+Diagnostics.h"
#endif

#include "HAPRequestHandlers+ThreadManagement.h"
#include "HAPRequestHandlers.h"
#include "HAPServiceTypes.h"

#ifdef HAVE_ADAPTIVE_LIGHT
#undef HAVE_ADAPTIVE_LIGHT
#endif
#define HAVE_ADAPTIVE_LIGHT 1

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * IID constants.
 */
#define kIID_AccessoryInformation                 ((uint64_t) 0x0001)
#define kIID_AccessoryInformationIdentify         ((uint64_t) 0x0002)
#define kIID_AccessoryInformationManufacturer     ((uint64_t) 0x0003)
#define kIID_AccessoryInformationModel            ((uint64_t) 0x0004)
#define kIID_AccessoryInformationName             ((uint64_t) 0x0005)
#define kIID_AccessoryInformationSerialNumber     ((uint64_t) 0x0006)
#define kIID_AccessoryInformationFirmwareRevision ((uint64_t) 0x0007)
#define kIID_AccessoryInformationHardwareRevision ((uint64_t) 0x0008)
#define kIID_AccessoryInformationADKVersion       ((uint64_t) 0x0009)
#define kIID_AccessoryInformationProductData      ((uint64_t) 0x000A)

#define kIID_HAPProtocolInformation                 ((uint64_t) 0x0010)
#define kIID_HAPProtocolInformationServiceSignature ((uint64_t) 0x0011)
#define kIID_HAPProtocolInformationVersion          ((uint64_t) 0x0012)

#define kIID_Pairing                ((uint64_t) 0x0020)
#define kIID_PairingPairSetup       ((uint64_t) 0x0022)
#define kIID_PairingPairVerify      ((uint64_t) 0x0023)
#define kIID_PairingPairingFeatures ((uint64_t) 0x0024)
#define kIID_PairingPairingPairings ((uint64_t) 0x0025)

#define kIID_CameraEventRecordingManagement                                      ((uint64_t) 0x0040)
#define kIID_CameraEventRecordingManagementActive                                ((uint64_t) 0x0042)
#define kIID_CameraEventRecordingManagementSupportedCameraRecordingConfiguration ((uint64_t) 0x0043)
#define kIID_CameraEventRecordingManagementSupportedVideoRecordingConfiguration  ((uint64_t) 0x0044)
#define kIID_CameraEventRecordingManagementSupportedAudioRecordingConfiguration  ((uint64_t) 0x0045)
#define kIID_CameraEventRecordingManagementSelectedCameraRecordingConfiguration  ((uint64_t) 0x0046)

#define kIID_CameraRTPStreamManagement0                                  ((uint64_t) 0x0050)
#define kIID_CameraRTPStreamManagement0Active                            ((uint64_t) 0x0052)
#define kIID_CameraRTPStreamManagement0SupportedVideoStreamConfiguration ((uint64_t) 0x0053)
#define kIID_CameraRTPStreamManagement0SupportedAudioStreamConfiguration ((uint64_t) 0x0054)
#define kIID_CameraRTPStreamManagement0SupportedRTPConfiguration         ((uint64_t) 0x0055)
#define kIID_CameraRTPStreamManagement0SelectedRTPStreamConfiguration    ((uint64_t) 0x0056)
#define kIID_CameraRTPStreamManagement0SetupEndpoints                    ((uint64_t) 0x0057)
#define kIID_CameraRTPStreamManagement0StreamingStatus                   ((uint64_t) 0x0058)

#define kIID_DataStreamTransportManagement                                          ((uint64_t) 0x0060)
#define kIID_DataStreamTransportManagementSupportedDataStreamTransportConfiguration ((uint64_t) 0x0062)
#define kIID_DataStreamTransportManagementSetupDataStreamTransport                  ((uint64_t) 0x0063)
#define kIID_DataStreamTransportManagementVersion                                   ((uint64_t) 0x0064)
#define kIID_DataStreamTransportManagementDataStreamHAPTransport                    ((uint64_t) 0x0065)
#define kIID_DataStreamTransportManagementDataStreamHAPTransportInterrupt           ((uint64_t) 0x0066)

#define kIID_WiFiRouter                              ((uint64_t) 0x0070)
#define kIID_WiFiRouterServiceSignature              ((uint64_t) 0x0071)
#define kIID_WiFiRouterVersion                       ((uint64_t) 0x0073)
#define kIID_WiFiRouterSupportedRouterConfiguration  ((uint64_t) 0x0074)
#define kIID_WiFiRouterRouterStatus                  ((uint64_t) 0x0076)
#define kIID_WiFiRouterWANConfigurationList          ((uint64_t) 0x0077)
#define kIID_WiFiRouterWANStatusList                 ((uint64_t) 0x0078)
#define kIID_WiFiRouterManagedNetworkEnable          ((uint64_t) 0x0079)
#define kIID_WiFiRouterNetworkClientProfileControl   ((uint64_t) 0x007A)
#define kIID_WiFiRouterNetworkClientStatusControl    ((uint64_t) 0x007B)
#define kIID_WiFiRouterNetworkAccessViolationControl ((uint64_t) 0x007C)

#if (HAVE_ACCESS_CODE == 1)
// Test borrows access code helper function
#include "AccessCodeHelper.c"

#define kIID_AccessCode                       ((uint64_t) 0x0080)
#define kIID_AccessCodeServiceSignature       ((uint64_t) 0x0081)
#define kIID_AccessCodeSupportedConfiguration ((uint64_t) 0x0082)
#define kIID_AccessCodeControlPoint           ((uint64_t) 0x0083)
#define kIID_AccessCodeConfigurationState     ((uint64_t) 0x0084)
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_RECONFIGURATION)
#define kIID_WiFiTransport                         ((uint64_t) 0x0090)
#define kIID_WiFiTransportServiceSignature         ((uint64_t) 0x0091)
#define kIID_WiFiTransportCurrentTransport         ((uint64_t) 0x0092)
#define kIID_WiFiTransportWiFiCapability           ((uint64_t) 0x0093)
#define kIID_WiFiTransportWiFiConfigurationControl ((uint64_t) 0x0094)
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_UARP_SUPPORT)
#define kIID_FirmwareUpdate                 ((uint64_t) 0x00A0)
#define kIID_FirmwareUpdateServiceSignature ((uint64_t) 0x00A1)
#define kIID_FirmwareUpdateReadiness        ((uint64_t) 0x00A2)
#define kIID_FirmwareUpdateStatus           ((uint64_t) 0x00A3)
#endif

#define kIID_ThreadManagement                  ((uint64_t) 0x00B0)
#define kIID_ThreadManagementServiceSignature  ((uint64_t) 0x00B1)
#define kIID_ThreadManagementNodeCapabilities  ((uint64_t) 0x00B3)
#define kIID_ThreadManagementOpenThreadVersion ((uint64_t) 0x00B4)
#define kIID_ThreadManagementStatus            ((uint64_t) 0x00B5)
#define kIID_ThreadManagementCurrentTransport  ((uint64_t) 0x00B6)
#define kIID_ThreadManagementControl           ((uint64_t) 0x00B7)

#if HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)
#define kIID_Diagnostics                  ((uint64_t) 0x00C0)
#define kIID_SupportedDiagnosticsSnapshot ((uint64_t) 0x00C1)
#endif

#define kIID_LightBulb                 ((uint64_t) 0x00D0)
#define kIID_LightBulbServiceSignature ((uint64_t) 0x00D1)
#define kIID_LightBulbName             ((uint64_t) 0x00D2)
#define kIID_LightBulbOn               ((uint64_t) 0x00D3)
#define kIID_LightBulbBrightness       ((uint64_t) 0x00D4)
#define kIID_LightBulbHue              ((uint64_t) 0x00D5)
#define kIID_LightBulbSaturation       ((uint64_t) 0x00D6)
#define kIID_LightBulbColorTemp        ((uint64_t) 0x00D7)
#if (HAVE_ADAPTIVE_LIGHT == 1)
#include "../../Applications/Common/Helper/AdaptiveLight.c"
#define kIID_LightBulbTransitionControl                ((uint64_t) 0x00D8)
#define kIID_LightBulbSupportedTransitionConfiguration ((uint64_t) 0x00D9)
#define kIID_LightBulbTransitionCount                  ((uint64_t) 0x00DA)
#endif

#define kIID_Battery              ((uint64_t) 0x00E0)
#define kIID_BatteryLevel         ((uint64_t) 0x00E1)
#define kIID_BatteryChargingState ((uint64_t) 0x00E2)
#define kIID_BatteryStatusLow     ((uint64_t) 0x00E3)
#define kIID_BatteryName          ((uint64_t) 0x00E4)

#define kIID_AccessoryRuntimeInformation                 ((uint64_t) 0x00F0)
#define kIID_AccessoryRuntimeInformationSleepInterval    ((uint64_t) 0x00F1)
#define kIID_AccessoryRuntimeInformationPing             ((uint64_t) 0x00F2)
#define kIID_AccessoryRuntimeInformationServiceSignature ((uint64_t) 0x00F4)
#define kIID_AccessoryRuntimeInformationHeartBeat        ((uint64_t) 0x00F5)

#if (HAVE_NFC_ACCESS == 1)
#define kIID_NfcAccess                       ((uint64_t) 0x0200)
#define kIID_NfcAccessSupportedConfiguration ((uint64_t) 0x0201)
#define kIID_NfcAccessControlPoint           ((uint64_t) 0x0202)
#define kIID_NfcAccessConfigurationState     ((uint64_t) 0x0203)
#endif
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const HAPBoolCharacteristic accessoryInformationIdentifyCharacteristic = {
    .format = kHAPCharacteristicFormat_Bool,
    .iid = kIID_AccessoryInformationIdentify,
    .characteristicType = &kHAPCharacteristicType_Identify,
    .debugDescription = kHAPCharacteristicDebugDescription_Identify,
    .manufacturerDescription = NULL,
    .properties = { .readable = false,
                    .writable = true,
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
    .callbacks = { .handleRead = NULL,
                   .handleWrite = HAPHandleAccessoryInformationIdentifyWrite,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPStringCharacteristic accessoryInformationManufacturerCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_AccessoryInformationManufacturer,
    .characteristicType = &kHAPCharacteristicType_Manufacturer,
    .debugDescription = kHAPCharacteristicDebugDescription_Manufacturer,
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
    .callbacks = { .handleRead = HAPHandleAccessoryInformationManufacturerRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPStringCharacteristic accessoryInformationModelCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_AccessoryInformationModel,
    .characteristicType = &kHAPCharacteristicType_Model,
    .debugDescription = kHAPCharacteristicDebugDescription_Model,
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
    .callbacks = { .handleRead = HAPHandleAccessoryInformationModelRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPStringCharacteristic accessoryInformationNameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_AccessoryInformationName,
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
    .callbacks = { .handleRead = HAPHandleAccessoryInformationNameRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPStringCharacteristic accessoryInformationSerialNumberCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_AccessoryInformationSerialNumber,
    .characteristicType = &kHAPCharacteristicType_SerialNumber,
    .debugDescription = kHAPCharacteristicDebugDescription_SerialNumber,
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
    .callbacks = { .handleRead = HAPHandleAccessoryInformationSerialNumberRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPStringCharacteristic accessoryInformationFirmwareRevisionCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_AccessoryInformationFirmwareRevision,
    .characteristicType = &kHAPCharacteristicType_FirmwareRevision,
    .debugDescription = kHAPCharacteristicDebugDescription_FirmwareRevision,
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
    .callbacks = { .handleRead = HAPHandleAccessoryInformationFirmwareRevisionRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPStringCharacteristic accessoryInformationHardwareRevisionCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_AccessoryInformationHardwareRevision,
    .characteristicType = &kHAPCharacteristicType_HardwareRevision,
    .debugDescription = kHAPCharacteristicDebugDescription_HardwareRevision,
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
    .callbacks = { .handleRead = HAPHandleAccessoryInformationHardwareRevisionRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPStringCharacteristic accessoryInformationADKVersionCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_AccessoryInformationADKVersion,
    .characteristicType = &kHAPCharacteristicType_ADKVersion,
    .debugDescription = kHAPCharacteristicDebugDescription_ADKVersion,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = true,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleAccessoryInformationADKVersionRead, .handleWrite = NULL }
};

const HAPDataCharacteristic accessoryInformationProductDataCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_AccessoryInformationProductData,
    .characteristicType = &kHAPCharacteristicType_ProductData,
    .debugDescription = kHAPCharacteristicDebugDescription_ProductData,
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
    .constraints = { .maxLength = 8 },
    .callbacks = { .handleRead = HAPHandleAccessoryInformationProductDataRead, .handleWrite = NULL }
};

const HAPService accessoryInformationService = {
    .iid = kIID_AccessoryInformation,
    .serviceType = &kHAPServiceType_AccessoryInformation,
    .debugDescription = kHAPServiceDebugDescription_AccessoryInformation,
    .name = NULL,
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &accessoryInformationIdentifyCharacteristic,
                                                            &accessoryInformationManufacturerCharacteristic,
                                                            &accessoryInformationModelCharacteristic,
                                                            &accessoryInformationNameCharacteristic,
                                                            &accessoryInformationSerialNumberCharacteristic,
                                                            &accessoryInformationFirmwareRevisionCharacteristic,
                                                            &accessoryInformationHardwareRevisionCharacteristic,
                                                            &accessoryInformationADKVersionCharacteristic,
                                                            &accessoryInformationProductDataCharacteristic,
                                                            NULL }
};

//----------------------------------------------------------------------------------------------------------------------

static const HAPDataCharacteristic hapProtocolInformationServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_HAPProtocolInformationServiceSignature,
    .characteristicType = &kHAPCharacteristicType_ServiceSignature,
    .debugDescription = kHAPCharacteristicDebugDescription_ServiceSignature,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .readRequiresAdminPermissions = false,
                    .writeRequiresAdminPermissions = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 2097152 },
    .callbacks = { .handleRead = HAPHandleServiceSignatureRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

static const HAPStringCharacteristic hapProtocolInformationVersionCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_HAPProtocolInformationVersion,
    .characteristicType = &kHAPCharacteristicType_Version,
    .debugDescription = kHAPCharacteristicDebugDescription_Version,
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
    .callbacks = { .handleRead = HAPHandleHAPProtocolInformationVersionRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPService hapProtocolInformationService = {
    .iid = kIID_HAPProtocolInformation,
    .serviceType = &kHAPServiceType_HAPProtocolInformation,
    .debugDescription = kHAPServiceDebugDescription_HAPProtocolInformation,
    .name = NULL,
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = true } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &hapProtocolInformationServiceSignatureCharacteristic,
                                                            &hapProtocolInformationVersionCharacteristic,
                                                            NULL }
};

//----------------------------------------------------------------------------------------------------------------------

static const HAPTLV8Characteristic pairingPairSetupCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_PairingPairSetup,
    .characteristicType = &kHAPCharacteristicType_PairSetup,
    .debugDescription = kHAPCharacteristicDebugDescription_PairSetup,
    .manufacturerDescription = NULL,
    .properties = { .readable = false,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .readRequiresAdminPermissions = false,
                    .writeRequiresAdminPermissions = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = true,
                             .writableWithoutSecurity = true } },
    .callbacks = { .handleRead = HAPHandlePairingPairSetupRead,
                   .handleWrite = HAPHandlePairingPairSetupWrite,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

static const HAPTLV8Characteristic pairingPairVerifyCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_PairingPairVerify,
    .characteristicType = &kHAPCharacteristicType_PairVerify,
    .debugDescription = kHAPCharacteristicDebugDescription_PairVerify,
    .manufacturerDescription = NULL,
    .properties = { .readable = false,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .readRequiresAdminPermissions = false,
                    .writeRequiresAdminPermissions = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = true,
                             .writableWithoutSecurity = true } },
    .callbacks = { .handleRead = HAPHandlePairingPairVerifyRead,
                   .handleWrite = HAPHandlePairingPairVerifyWrite,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

static const HAPUInt8Characteristic pairingPairingFeaturesCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_PairingPairingFeatures,
    .characteristicType = &kHAPCharacteristicType_PairingFeatures,
    .debugDescription = kHAPCharacteristicDebugDescription_PairingFeatures,
    .manufacturerDescription = NULL,
    .properties = { .readable = false,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .readRequiresAdminPermissions = false,
                    .writeRequiresAdminPermissions = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsDisconnectedNotification = false,
                             .supportsBroadcastNotification = false,
                             .readableWithoutSecurity = true,
                             .writableWithoutSecurity = false } },
    .units = kHAPCharacteristicUnits_None,
    .constraints = { .minimumValue = 0,
                     .maximumValue = UINT8_MAX,
                     .stepValue = 0,
                     .validValues = NULL,
                     .validValuesRanges = NULL },
    .callbacks = { .handleRead = HAPHandlePairingPairingFeaturesRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

static const HAPTLV8Characteristic pairingPairingPairingsCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_PairingPairingPairings,
    .characteristicType = &kHAPCharacteristicType_PairingPairings,
    .debugDescription = kHAPCharacteristicDebugDescription_PairingPairings,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .readRequiresAdminPermissions = false,
                    .writeRequiresAdminPermissions = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HAPHandlePairingPairingPairingsRead,
                   .handleWrite = HAPHandlePairingPairingPairingsWrite,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPService pairingService = {
    .iid = kIID_Pairing,
    .serviceType = &kHAPServiceType_Pairing,
    .debugDescription = kHAPServiceDebugDescription_Pairing,
    .name = NULL,
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &pairingPairSetupCharacteristic,
                                                            &pairingPairVerifyCharacteristic,
                                                            &pairingPairingFeaturesCharacteristic,
                                                            &pairingPairingPairingsCharacteristic,
                                                            NULL }
};

//----------------------------------------------------------------------------------------------------------------------

#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_RECONFIGURATION)

const HAPBoolCharacteristic wiFiTransportCurrentTransportCharacteristic = {
    .format = kHAPCharacteristicFormat_Bool,
    .iid = kIID_WiFiTransportCurrentTransport,
    .characteristicType = &kHAPCharacteristicType_CurrentTransport,
    .debugDescription = kHAPCharacteristicDebugDescription_CurrentTransport,
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
    .callbacks = { .handleRead = HAPHandleWiFiTransportCurrentTransportRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPUInt32Characteristic wiFiTransportWiFiCapabilityCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt32,
    .iid = kIID_WiFiTransportWiFiCapability,
    .characteristicType = &kHAPCharacteristicType_WiFiCapability,
    .debugDescription = kHAPCharacteristicDebugDescription_WiFiCapability,
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
    .constraints = { .minimumValue = 0, .maximumValue = 15, .stepValue = 1 },
    .callbacks = { .handleRead = HAPHandleWiFiTransportWiFiCapabilityRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPTLV8Characteristic wiFiTransportWiFiConfigurationControlCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_WiFiTransportWiFiConfigurationControl,
    .characteristicType = &kHAPCharacteristicType_WiFiConfigurationControl,
    .debugDescription = kHAPCharacteristicDebugDescription_WiFiConfigurationControl,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .readRequiresAdminPermissions = true,
                    .writeRequiresAdminPermissions = true,
                    .requiresTimedWrite = true,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true, .supportsWriteResponse = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HAPHandleWiFiTransportWiFiConfigurationControlRead,
                   .handleWrite = HAPHandleWiFiTransportWiFiConfigurationControlWrite,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPService wiFiTransportService = {
    .iid = kIID_WiFiTransport,
    .serviceType = &kHAPServiceType_WiFiTransport,
    .debugDescription = kHAPServiceDebugDescription_WiFiTransport,
    .name = NULL,
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &wiFiTransportCurrentTransportCharacteristic,
                                                            &wiFiTransportWiFiCapabilityCharacteristic,
                                                            &wiFiTransportWiFiConfigurationControlCharacteristic,
                                                            NULL }
};

#endif

//----------------------------------------------------------------------------------------------------------------------
#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)
const HAPUInt8Characteristic cameraEventRecordingManagementActiveCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_CameraEventRecordingManagementActive,
    .characteristicType = &kHAPCharacteristicType_Active,
    .debugDescription = kHAPCharacteristicDebugDescription_Active,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .readRequiresAdminPermissions = false,
                    .writeRequiresAdminPermissions = true,
                    .requiresTimedWrite = true,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsDisconnectedNotification = false,
                             .supportsBroadcastNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .units = kHAPCharacteristicUnits_None,
    .constraints = { .minimumValue = 0,
                     .maximumValue = 1,
                     .stepValue = 1,
                     .validValues = NULL,
                     .validValuesRanges = NULL },
    .callbacks = { .handleRead = HAPHandleCameraEventRecordingManagementActiveRead,
                   .handleWrite = HAPHandleCameraEventRecordingManagementActiveWrite,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPTLV8Characteristic cameraEventRecordingManagementSupportedCameraRecordingConfigurationCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_CameraEventRecordingManagementSupportedCameraRecordingConfiguration,
    .characteristicType = &kHAPCharacteristicType_SupportedCameraRecordingConfiguration,
    .debugDescription = kHAPCharacteristicDebugDescription_SupportedCameraRecordingConfiguration,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .readRequiresAdminPermissions = false,
                    .writeRequiresAdminPermissions = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HAPHandleCameraEventRecordingManagementSupportedCameraRecordingConfigurationRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPTLV8Characteristic cameraEventRecordingManagementSupportedVideoRecordingConfigurationCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_CameraEventRecordingManagementSupportedVideoRecordingConfiguration,
    .characteristicType = &kHAPCharacteristicType_SupportedVideoRecordingConfiguration,
    .debugDescription = kHAPCharacteristicDebugDescription_SupportedVideoRecordingConfiguration,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .readRequiresAdminPermissions = false,
                    .writeRequiresAdminPermissions = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HAPHandleCameraEventRecordingManagementSupportedVideoRecordingConfigurationRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPTLV8Characteristic cameraEventRecordingManagementSupportedAudioRecordingConfigurationCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_CameraEventRecordingManagementSupportedAudioRecordingConfiguration,
    .characteristicType = &kHAPCharacteristicType_SupportedAudioRecordingConfiguration,
    .debugDescription = kHAPCharacteristicDebugDescription_SupportedAudioRecordingConfiguration,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .readRequiresAdminPermissions = false,
                    .writeRequiresAdminPermissions = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HAPHandleCameraEventRecordingManagementSupportedAudioRecordingConfigurationRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPTLV8Characteristic cameraEventRecordingManagementSelectedCameraRecordingConfigurationCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_CameraEventRecordingManagementSelectedCameraRecordingConfiguration,
    .characteristicType = &kHAPCharacteristicType_SelectedCameraRecordingConfiguration,
    .debugDescription = kHAPCharacteristicDebugDescription_SelectedCameraRecordingConfiguration,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .readRequiresAdminPermissions = false,
                    .writeRequiresAdminPermissions = true,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HAPHandleCameraEventRecordingManagementSelectedCameraRecordingConfigurationRead,
                   .handleWrite = HAPHandleCameraEventRecordingManagementSelectedCameraRecordingConfigurationWrite,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPService cameraEventRecordingManagementService = {
    .iid = kIID_CameraEventRecordingManagement,
    .serviceType = &kHAPServiceType_CameraEventRecordingManagement,
    .debugDescription = kHAPServiceDebugDescription_CameraEventRecordingManagement,
    .name = NULL,
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics =
            (const HAPCharacteristic* const[]) {
                    &cameraEventRecordingManagementActiveCharacteristic,
                    &cameraEventRecordingManagementSupportedCameraRecordingConfigurationCharacteristic,
                    &cameraEventRecordingManagementSupportedVideoRecordingConfigurationCharacteristic,
                    &cameraEventRecordingManagementSupportedAudioRecordingConfigurationCharacteristic,
                    &cameraEventRecordingManagementSelectedCameraRecordingConfigurationCharacteristic,
                    NULL }
};

//----------------------------------------------------------------------------------------------------------------------

const HAPUInt8Characteristic cameraRTPStreamManagement0ActiveCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_CameraRTPStreamManagement0Active,
    .characteristicType = &kHAPCharacteristicType_Active,
    .debugDescription = kHAPCharacteristicDebugDescription_Active,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .readRequiresAdminPermissions = false,
                    .writeRequiresAdminPermissions = true,
                    .requiresTimedWrite = true,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsDisconnectedNotification = false,
                             .supportsBroadcastNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .units = kHAPCharacteristicUnits_None,
    .constraints = { .minimumValue = 0,
                     .maximumValue = 1,
                     .stepValue = 1,
                     .validValues = NULL,
                     .validValuesRanges = NULL },
    .callbacks = { .handleRead = HAPHandleCameraRTPStreamManagementActiveRead,
                   .handleWrite = HAPHandleCameraRTPStreamManagementActiveWrite,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPTLV8Characteristic cameraRTPStreamManagement0SupportedVideoStreamConfigurationCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_CameraRTPStreamManagement0SupportedVideoStreamConfiguration,
    .characteristicType = &kHAPCharacteristicType_SupportedVideoStreamConfiguration,
    .debugDescription = kHAPCharacteristicDebugDescription_SupportedVideoStreamConfiguration,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .readRequiresAdminPermissions = false,
                    .writeRequiresAdminPermissions = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HAPHandleCameraRTPStreamManagementSupportedVideoStreamConfigurationRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPTLV8Characteristic cameraRTPStreamManagement0SupportedAudioStreamConfigurationCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_CameraRTPStreamManagement0SupportedAudioStreamConfiguration,
    .characteristicType = &kHAPCharacteristicType_SupportedAudioStreamConfiguration,
    .debugDescription = kHAPCharacteristicDebugDescription_SupportedAudioStreamConfiguration,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .readRequiresAdminPermissions = false,
                    .writeRequiresAdminPermissions = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HAPHandleCameraRTPStreamManagementSupportedAudioStreamConfigurationRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPTLV8Characteristic cameraRTPStreamManagement0SupportedRTPConfigurationCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_CameraRTPStreamManagement0SupportedRTPConfiguration,
    .characteristicType = &kHAPCharacteristicType_SupportedRTPConfiguration,
    .debugDescription = kHAPCharacteristicDebugDescription_SupportedRTPConfiguration,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .readRequiresAdminPermissions = false,
                    .writeRequiresAdminPermissions = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HAPHandleCameraRTPStreamManagementSupportedRTPConfigurationRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPTLV8Characteristic cameraRTPStreamManagement0SelectedRTPStreamConfigurationCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_CameraRTPStreamManagement0SelectedRTPStreamConfiguration,
    .characteristicType = &kHAPCharacteristicType_SelectedRTPStreamConfiguration,
    .debugDescription = kHAPCharacteristicDebugDescription_SelectedRTPStreamConfiguration,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .readRequiresAdminPermissions = false,
                    .writeRequiresAdminPermissions = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HAPHandleCameraRTPStreamManagementSelectedRTPStreamConfigurationRead,
                   .handleWrite = HAPHandleCameraRTPStreamManagementSelectedRTPStreamConfigurationWrite,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPTLV8Characteristic cameraRTPStreamManagement0SetupEndpointsCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_CameraRTPStreamManagement0SetupEndpoints,
    .characteristicType = &kHAPCharacteristicType_SetupEndpoints,
    .debugDescription = kHAPCharacteristicDebugDescription_SetupEndpoints,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .readRequiresAdminPermissions = false,
                    .writeRequiresAdminPermissions = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HAPHandleCameraRTPStreamManagementSetupEndpointsRead,
                   .handleWrite = HAPHandleCameraRTPStreamManagementSetupEndpointsWrite,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPTLV8Characteristic cameraRTPStreamManagement0StreamingStatusCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_CameraRTPStreamManagement0StreamingStatus,
    .characteristicType = &kHAPCharacteristicType_StreamingStatus,
    .debugDescription = kHAPCharacteristicDebugDescription_StreamingStatus,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .readRequiresAdminPermissions = false,
                    .writeRequiresAdminPermissions = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HAPHandleCameraRTPStreamManagementStreamingStatusRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPService cameraRTPStreamManagement0Service = {
    .iid = kIID_CameraRTPStreamManagement0,
    .serviceType = &kHAPServiceType_CameraRTPStreamManagement,
    .debugDescription = kHAPServiceDebugDescription_CameraRTPStreamManagement,
    .name = NULL,
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics =
            (const HAPCharacteristic* const[]) {
                    &cameraRTPStreamManagement0SupportedVideoStreamConfigurationCharacteristic,
                    &cameraRTPStreamManagement0SupportedAudioStreamConfigurationCharacteristic,
                    &cameraRTPStreamManagement0SupportedRTPConfigurationCharacteristic,
                    &cameraRTPStreamManagement0SelectedRTPStreamConfigurationCharacteristic,
                    &cameraRTPStreamManagement0SelectedRTPStreamConfigurationCharacteristic,
                    &cameraRTPStreamManagement0SetupEndpointsCharacteristic,
                    &cameraRTPStreamManagement0StreamingStatusCharacteristic,
                    NULL }
};
#endif //#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)
//----------------------------------------------------------------------------------------------------------------------

const HAPTLV8Characteristic dataStreamTransportManagementSupportedDataStreamTransportConfigurationCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_DataStreamTransportManagementSupportedDataStreamTransportConfiguration,
    .characteristicType = &kHAPCharacteristicType_SupportedDataStreamTransportConfiguration,
    .debugDescription = kHAPCharacteristicDebugDescription_SupportedDataStreamTransportConfiguration,
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
    .callbacks = { .handleRead = HAPHandleDataStreamTransportManagementSupportedDataStreamTransportConfigurationRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPTLV8Characteristic dataStreamTransportManagementSetupDataStreamTransportCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_DataStreamTransportManagementSetupDataStreamTransport,
    .characteristicType = &kHAPCharacteristicType_SetupDataStreamTransport,
    .debugDescription = kHAPCharacteristicDebugDescription_SetupDataStreamTransport,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .readRequiresAdminPermissions = false,
                    .writeRequiresAdminPermissions = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true, .supportsWriteResponse = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HAPHandleDataStreamTransportManagementSetupDataStreamTransportRead,
                   .handleWrite = HAPHandleDataStreamTransportManagementSetupDataStreamTransportWrite,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPTLV8Characteristic dataStreamTransportManagementDataStreamHAPTransportCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_DataStreamTransportManagementDataStreamHAPTransport,
    .characteristicType = &kHAPCharacteristicType_DataStreamHAPTransport,
    .debugDescription = kHAPCharacteristicDebugDescription_DataStreamHAPTransport,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true, .supportsWriteResponse = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HAPHandleDataStreamTransportManagementDataStreamHAPTransportRead,
                   .handleWrite = HAPHandleDataStreamTransportManagementDataStreamHAPTransportWrite }
};

static const HAPTLV8Characteristic dataStreamTransportManagementDataStreamHAPTransportInterruptCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_DataStreamTransportManagementDataStreamHAPTransportInterrupt,
    .characteristicType = &kHAPCharacteristicType_DataStreamHAPTransportInterrupt,
    .debugDescription = kHAPCharacteristicDebugDescription_DataStreamHAPTransportInterrupt,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HAPHandleDataStreamTransportManagementDataStreamHAPTransportInterruptRead }
};

const HAPStringCharacteristic dataStreamTransportManagementVersionCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_DataStreamTransportManagementVersion,
    .characteristicType = &kHAPCharacteristicType_Version,
    .debugDescription = kHAPCharacteristicDebugDescription_Version,
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
    .callbacks = { .handleRead = HAPHandleDataStreamTransportManagementVersionRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPService dataStreamTransportManagementService = {
    .iid = kIID_DataStreamTransportManagement,
    .serviceType = &kHAPServiceType_DataStreamTransportManagement,
    .debugDescription = kHAPServiceDebugDescription_DataStreamTransportManagement,
    .name = NULL,
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics =
            (const HAPCharacteristic* const[]) {
                    &dataStreamTransportManagementSupportedDataStreamTransportConfigurationCharacteristic,
                    &dataStreamTransportManagementSetupDataStreamTransportCharacteristic,
                    &dataStreamTransportManagementVersionCharacteristic,
                    &dataStreamTransportManagementDataStreamHAPTransportCharacteristic,
                    &dataStreamTransportManagementDataStreamHAPTransportInterruptCharacteristic,
                    NULL }
};

//----------------------------------------------------------------------------------------------------------------------
#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_ROUTER)
const HAPDataCharacteristic wiFiRouterServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_WiFiRouterServiceSignature,
    .characteristicType = &kHAPCharacteristicType_ServiceSignature,
    .debugDescription = kHAPCharacteristicDebugDescription_ServiceSignature,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .readRequiresAdminPermissions = false,
                    .writeRequiresAdminPermissions = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 2097152 },
    .callbacks = { .handleRead = HAPHandleServiceSignatureRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPStringCharacteristic wiFiRouterVersionCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_WiFiRouterVersion,
    .characteristicType = &kHAPCharacteristicType_Version,
    .debugDescription = kHAPCharacteristicDebugDescription_Version,
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
    .callbacks = { .handleRead = HAPHandleWiFiRouterVersionRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPTLV8Characteristic wiFiRouterSupportedRouterConfigurationCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_WiFiRouterSupportedRouterConfiguration,
    .characteristicType = &kHAPCharacteristicType_SupportedRouterConfiguration,
    .debugDescription = kHAPCharacteristicDebugDescription_SupportedRouterConfiguration,
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
    .callbacks = { .handleRead = HAPHandleWiFiRouterSupportedRouterConfigurationRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPUInt8Characteristic wiFiRouterRouterStatusCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_WiFiRouterRouterStatus,
    .characteristicType = &kHAPCharacteristicType_RouterStatus,
    .debugDescription = kHAPCharacteristicDebugDescription_RouterStatus,
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
    .constraints = { .minimumValue = 0,
                     .maximumValue = 1,
                     .stepValue = 1,
                     .validValues = NULL,
                     .validValuesRanges = NULL },
    .callbacks = { .handleRead = HAPHandleWiFiRouterRouterStatusRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPTLV8Characteristic wiFiRouterWANConfigurationListCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_WiFiRouterWANConfigurationList,
    .characteristicType = &kHAPCharacteristicType_WANConfigurationList,
    .debugDescription = kHAPCharacteristicDebugDescription_WANConfigurationList,
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
    .callbacks = { .handleRead = HAPHandleWiFiRouterWANConfigurationListRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPTLV8Characteristic wiFiRouterWANStatusListCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_WiFiRouterWANStatusList,
    .characteristicType = &kHAPCharacteristicType_WANStatusList,
    .debugDescription = kHAPCharacteristicDebugDescription_WANStatusList,
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
    .callbacks = { .handleRead = HAPHandleWiFiRouterWANStatusListRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPUInt8Characteristic wiFiRouterManagedNetworkEnableCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_WiFiRouterManagedNetworkEnable,
    .characteristicType = &kHAPCharacteristicType_ManagedNetworkEnable,
    .debugDescription = kHAPCharacteristicDebugDescription_ManagedNetworkEnable,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .readRequiresAdminPermissions = false,
                    .writeRequiresAdminPermissions = true,
                    .requiresTimedWrite = true,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .minimumValue = 0,
                     .maximumValue = 1,
                     .stepValue = 1,
                     .validValues = NULL,
                     .validValuesRanges = NULL },
    .callbacks = { .handleRead = HAPHandleWiFiRouterManagedNetworkEnableRead,
                   .handleWrite = HAPHandleWiFiRouterManagedNetworkEnableWrite,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPTLV8Characteristic wiFiRouterNetworkClientProfileControlCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_WiFiRouterNetworkClientProfileControl,
    .characteristicType = &kHAPCharacteristicType_NetworkClientProfileControl,
    .debugDescription = kHAPCharacteristicDebugDescription_NetworkClientProfileControl,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .readRequiresAdminPermissions = true,
                    .writeRequiresAdminPermissions = true,
                    .requiresTimedWrite = true,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true, .supportsWriteResponse = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HAPHandleWiFiRouterNetworkClientProfileControlRead,
                   .handleWrite = HAPHandleWiFiRouterNetworkClientProfileControlWrite,
                   .handleSubscribe = HAPHandleWiFiRouterNetworkClientProfileControlSubscribe,
                   .handleUnsubscribe = HAPHandleWiFiRouterNetworkClientProfileControlUnsubscribe }
};

const HAPTLV8Characteristic wiFiRouterNetworkClientStatusControlCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_WiFiRouterNetworkClientStatusControl,
    .characteristicType = &kHAPCharacteristicType_NetworkClientStatusControl,
    .debugDescription = kHAPCharacteristicDebugDescription_NetworkClientStatusControl,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .readRequiresAdminPermissions = true,
                    .writeRequiresAdminPermissions = true,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true, .supportsWriteResponse = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HAPHandleWiFiRouterNetworkClientStatusControlRead,
                   .handleWrite = HAPHandleWiFiRouterNetworkClientStatusControlWrite,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPTLV8Characteristic wiFiRouterNetworkAccessViolationControlCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_WiFiRouterNetworkAccessViolationControl,
    .characteristicType = &kHAPCharacteristicType_NetworkAccessViolationControl,
    .debugDescription = kHAPCharacteristicDebugDescription_NetworkAccessViolationControl,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .readRequiresAdminPermissions = true,
                    .writeRequiresAdminPermissions = true,
                    .requiresTimedWrite = true,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true, .supportsWriteResponse = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HAPHandleWiFiRouterNetworkAccessViolationControlRead,
                   .handleWrite = HAPHandleWiFiRouterNetworkAccessViolationControlWrite,
                   .handleSubscribe = HAPHandleWiFiRouterNetworkAccessViolationControlSubscribe,
                   .handleUnsubscribe = HAPHandleWiFiRouterNetworkAccessViolationControlUnsubscribe }
};

const HAPService wiFiRouterService = {
    .iid = kIID_WiFiRouter,
    .serviceType = &kHAPServiceType_WiFiRouter,
    .debugDescription = kHAPServiceDebugDescription_WiFiRouter,
    .name = NULL,
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &wiFiRouterServiceSignatureCharacteristic,
                                                            &wiFiRouterVersionCharacteristic,
                                                            &wiFiRouterSupportedRouterConfigurationCharacteristic,
                                                            &wiFiRouterRouterStatusCharacteristic,
                                                            &wiFiRouterWANConfigurationListCharacteristic,
                                                            &wiFiRouterWANStatusListCharacteristic,
                                                            &wiFiRouterManagedNetworkEnableCharacteristic,
                                                            &wiFiRouterNetworkClientProfileControlCharacteristic,
                                                            &wiFiRouterNetworkClientStatusControlCharacteristic,
                                                            &wiFiRouterNetworkAccessViolationControlCharacteristic,
                                                            NULL }
};
#endif //#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_ROUTER)

#if (HAVE_ACCESS_CODE == 1)
/**
 * The 'Service Signature' characteristic of the Access Code service.
 */
static const HAPDataCharacteristic accessCodeServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_AccessCodeServiceSignature,
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

const HAPTLV8Characteristic accessCodeSupportedConfigurationCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_AccessCodeSupportedConfiguration,
    .characteristicType = &kHAPCharacteristicType_AccessCodeSupportedConfiguration,
    .debugDescription = kHAPCharacteristicDebugDescription_AccessCodeSupportedConfiguration,
    .manufacturerDescription = NULL,
    .properties = {
        .readable = true,
        .writable = false,
        .supportsEventNotification = false,
        .hidden = false,
        .requiresTimedWrite = false,
        .supportsAuthorizationData = false,
        .ip = {
            .controlPoint = false,
            .supportsWriteResponse = false,
        },
        .ble = {
            .supportsBroadcastNotification = false,
            .supportsDisconnectedNotification = false,
            .readableWithoutSecurity = false,
            .writableWithoutSecurity = false
        }
    },
    .callbacks = {
        .handleRead = HandleAccessCodeSupportedConfigurationRead,
        .handleWrite = NULL,
    }
};

const HAPTLV8Characteristic accessCodeControlPointCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_AccessCodeControlPoint,
    .characteristicType = &kHAPCharacteristicType_AccessCodeControlPoint,
    .debugDescription = kHAPCharacteristicDebugDescription_AccessCodeControlPoint,
    .manufacturerDescription = NULL,
    .properties = {
        .readable = true,
        .writable = true,
        .supportsEventNotification = false,
        .hidden = false,
        .requiresTimedWrite = false,
        .readRequiresAdminPermissions = true,
        .writeRequiresAdminPermissions = true,
        .supportsAuthorizationData = false,
        .ip = {
            .controlPoint = true,
            .supportsWriteResponse = true,
        },
        .ble = {
            .supportsBroadcastNotification = false,
            .supportsDisconnectedNotification = false,
            .readableWithoutSecurity = false,
            .writableWithoutSecurity = false
        }
    },
    .callbacks = {
        .handleRead = HAPHandleAccessCodeControlPointRead,
        .handleWrite = HAPHandleAccessCodeControlPointWrite,
    }
};

/**
 * The 'Configuration State' characteristic of the Access Code service.
 */
const HAPUInt16Characteristic accessCodeConfigurationStateCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt16,
    .iid = kIID_AccessCodeConfigurationState,
    .characteristicType = &kHAPCharacteristicType_ConfigurationState,
    .debugDescription = kHAPCharacteristicDebugDescription_ConfigurationState,
    .manufacturerDescription = NULL,
    .properties = {
        .readable = true,
        .writable = false,
        .supportsEventNotification = true,
        .hidden = false,
        .requiresTimedWrite = false,
        .ip = {
            .controlPoint = false,
            .supportsWriteResponse = false,
        },
        .ble = {
            .supportsBroadcastNotification = true,
            .supportsDisconnectedNotification = true,
            .readableWithoutSecurity = false,
            .writableWithoutSecurity = false,
        }
    },
    .constraints = {
        .minimumValue = 0,
        .maximumValue = UINT16_MAX,
    },
    .callbacks = {
        .handleRead = HandleAccessCodeConfigurationStateRead,
        .handleWrite = NULL,
    }
};

const HAPService accessCodeService = {
    .iid = kIID_AccessCode,
    .serviceType = &kHAPServiceType_AccessCode,
    .debugDescription = kHAPServiceDebugDescription_AccessCode,
    .name = NULL,
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &accessCodeServiceSignatureCharacteristic,
                                                            &accessCodeSupportedConfigurationCharacteristic,
                                                            &accessCodeControlPointCharacteristic,
                                                            &accessCodeConfigurationStateCharacteristic,
                                                            NULL }
};
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_UARP_SUPPORT)
static const HAPDataCharacteristic firmwareUpdateServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_FirmwareUpdateServiceSignature,
    .characteristicType = &kHAPCharacteristicType_ServiceSignature,
    .debugDescription = kHAPCharacteristicDebugDescription_ServiceSignature,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .readRequiresAdminPermissions = false,
                    .writeRequiresAdminPermissions = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 2097152 },
    .callbacks = { .handleRead = HAPHandleServiceSignatureRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

static const HAPTLV8Characteristic firmwareUpdateReadinessCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_FirmwareUpdateReadiness,
    .characteristicType = &kHAPCharacteristicType_FirmwareUpdateReadiness,
    .debugDescription = kHAPCharacteristicDebugDescription_FirmwareUpdateReadiness,
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
            .supportsWriteResponse = false,
        },
        .ble = {
            .supportsBroadcastNotification = false,
            .supportsDisconnectedNotification = true,
            .readableWithoutSecurity = false,
            .writableWithoutSecurity = false
        }
    },
    .callbacks = {
        .handleRead = HAPHandleFirmwareUpdateReadinessRead,
        .handleWrite = NULL,
        .handleSubscribe = NULL,
        .handleUnsubscribe = NULL
    }
};

static const HAPTLV8Characteristic firmwareUpdateStatusCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_FirmwareUpdateStatus,
    .characteristicType = &kHAPCharacteristicType_FirmwareUpdateStatus,
    .debugDescription = kHAPCharacteristicDebugDescription_FirmwareUpdateStatus,
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
            .supportsWriteResponse = false,
        },
        .ble = {
            .supportsBroadcastNotification = false,
            .supportsDisconnectedNotification = true,
            .readableWithoutSecurity = false,
            .writableWithoutSecurity = false
        }
    },
    .callbacks = {
        .handleRead = HAPHandleFirmwareUpdateStatusRead,
        .handleWrite = NULL,
        .handleSubscribe = NULL,
        .handleUnsubscribe = NULL
    }
};

const HAPService firmwareUpdateService = {
    .iid = kIID_FirmwareUpdate,
    .serviceType = &kHAPServiceType_FirmwareUpdate,
    .debugDescription = kHAPServiceDebugDescription_FirmwareUpdate,
    .name = NULL,
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &firmwareUpdateServiceSignatureCharacteristic,
                                                            &firmwareUpdateReadinessCharacteristic,
                                                            &firmwareUpdateStatusCharacteristic,
                                                            NULL }
};
#endif // HAP_FEATURE_ENABLED(HAP_FEATURE_UARP_SUPPORT)

#if HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)
const HAPTLV8Characteristic diagnosticsSupportedDiagnosticsSnapshot = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_SupportedDiagnosticsSnapshot,
    .characteristicType = &kHAPCharacteristicType_SupportedDiagnosticsSnapshot,
    .debugDescription = kHAPCharacteristicDebugDescription_SupportedDiagnosticsSnapshot,
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
    .callbacks = { .handleRead = HAPHandleSupportedDiagnosticsSnapshotRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPService diagnosticsService = {
    .iid = kIID_Diagnostics,
    .serviceType = &kHAPServiceType_Diagnostics,
    .debugDescription = kHAPServiceDebugDescription_Diagnostics,
    .name = NULL,
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &diagnosticsSupportedDiagnosticsSnapshot, NULL }
};
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
/**
 * The 'Service Signature' characteristic of the Thread Management service.
 */
static const HAPDataCharacteristic threadManagementServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_ThreadManagementServiceSignature,
    .characteristicType = &kHAPCharacteristicType_ServiceSignature,
    .debugDescription = kHAPCharacteristicDebugDescription_ServiceSignature,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .readRequiresAdminPermissions = false,
                    .writeRequiresAdminPermissions = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 2097152 },
    .callbacks = { .handleRead = HAPHandleServiceSignatureRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

/**
 * The 'Thread Role' characteristic of the Thread Management service.
 */
const HAPUInt16Characteristic threadManagementStatusCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt16,
    .iid = kIID_ThreadManagementStatus,
    .characteristicType = &kHAPCharacteristicType_ThreadManagementStatus,
    .debugDescription = kHAPCharacteristicDebugDescription_ThreadManagementStatus,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .readRequiresAdminPermissions = true,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false, },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .minimumValue = 0, .maximumValue = 0x7F, .stepValue = 1, },
    .callbacks = { .handleRead = HAPHandleThreadManagementStatusRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL, },
};

/**
 * The 'Control' characteristic of the Thread Management service.
 */
const HAPTLV8Characteristic threadManagementControlCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_ThreadManagementControl,
    .characteristicType = &kHAPCharacteristicType_ThreadManagementControl,
    .debugDescription = kHAPCharacteristicDebugDescription_ThreadManagementControl,
    .manufacturerDescription = NULL,
    .properties = {
        .readable = true,
        .writable = true,
        .supportsEventNotification = false,
        .hidden = false,
        .requiresTimedWrite = false,
        .readRequiresAdminPermissions = true,
        .writeRequiresAdminPermissions = true,
        .supportsAuthorizationData = false,
        .ip = {
            .controlPoint = true,
            .supportsWriteResponse = true,
        },
        .ble = {
            .supportsBroadcastNotification = false,
            .supportsDisconnectedNotification = false,
            .readableWithoutSecurity = false,
            .writableWithoutSecurity = false
        }
    },
    .callbacks = {
        .handleRead = HAPHandleThreadManagementControlRead,
        .handleWrite = HAPHandleThreadManagementControlWrite,
        .handleSubscribe = NULL,
        .handleUnsubscribe = NULL
    }
};

/**
 * The 'Thread Current Transport' characteristic of the Thread Management service.
 */
const HAPBoolCharacteristic threadManagementCurrentTransportCharacteristic = {
    .format = kHAPCharacteristicFormat_Bool,
    .iid = kIID_ThreadManagementCurrentTransport,
    .characteristicType = &kHAPCharacteristicType_CurrentTransport,
    .debugDescription = kHAPCharacteristicDebugDescription_CurrentTransport,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .readRequiresAdminPermissions = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false, },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HAPHandleThreadManagementCurrentTransportRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL, },
};

/**
 * Thread Management service that contains the above Thread Management characteristics.
 */
const HAPService threadManagementService = {
    .iid = kIID_ThreadManagement,
    .serviceType = &kHAPServiceType_ThreadManagement,
    .debugDescription = kHAPServiceDebugDescription_ThreadManagement,
    .name = NULL,
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics =
            (const HAPCharacteristic* const[]) {
                    &threadManagementServiceSignatureCharacteristic,
                    &threadManagementStatusCharacteristic,
                    &threadManagementCurrentTransportCharacteristic,
                    &threadManagementControlCharacteristic,
                    NULL,
            },
};

#endif

static const HAPDataCharacteristic accessoryRuntimeInformationServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_AccessoryRuntimeInformationServiceSignature,
    .characteristicType = &kHAPCharacteristicType_ServiceSignature,
    .debugDescription = kHAPCharacteristicDebugDescription_ServiceSignature,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .readRequiresAdminPermissions = false,
                    .writeRequiresAdminPermissions = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 2097152 },
    .callbacks = { .handleRead = HAPHandleServiceSignatureRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPUInt32Characteristic accessoryRuntimeInformationHeartBeatCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt32,
    .iid = kIID_AccessoryRuntimeInformationHeartBeat,
    .characteristicType = &kHAPCharacteristicType_AccessoryRuntimeInformationHeartBeat,
    .debugDescription = kHAPCharacteristicDebugDescription_AccessoryRuntimeInformationHeartBeat,
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
    .constraints = { .minimumValue = 0, .maximumValue = UINT32_MAX, .stepValue = 1 },
    .callbacks = { .handleRead = HAPHandleAccessoryRuntimeInformationHeartBeatRead, .handleWrite = NULL }
};

const HAPService accessoryRuntimeInformationService = {
    .iid = kIID_AccessoryRuntimeInformation,
    .serviceType = &kHAPServiceType_AccessoryRuntimeInformation,
    .debugDescription = kHAPServiceDebugDescription_AccessoryRuntimeInformation,
    .name = "Accessory Runtime Information",
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics =
            (const HAPCharacteristic* const[]) {
                    &accessoryRuntimeInformationServiceSignatureCharacteristic,
                    &accessoryRuntimeInformationHeartBeatCharacteristic,
                    NULL,
            },
};

/**
 * The 'Service Signature' characteristic of the Light Bulb service.
 */
static const HAPDataCharacteristic lightBulbServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_LightBulbServiceSignature,
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

static int32_t brightness = 50;
HAP_RESULT_USE_CHECK
HAPError TestHandleLightBulbBrightnessRead(
        HAPAccessoryServer* _Nullable server HAP_UNUSED,
        const HAPIntCharacteristicReadRequest* _Nullable request HAP_UNUSED,
        int32_t* _Nullable value,
        void* _Nullable context HAP_UNUSED) {
    if (value) {
        // Have to set some value in order not to cause undefined behavior.
        *value = brightness;
    }
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, (int) *value);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError TestHandleLightBulbBrightnessWrite(
        HAPAccessoryServer* _Nullable server,
        const HAPIntCharacteristicWriteRequest* _Nullable request HAP_UNUSED,
        int32_t value,
        void* _Nullable context HAP_UNUSED) {
    HAPError err = kHAPError_None;
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, (int) value);
    brightness = value;
    return err;
}

static uint32_t temperature = 200;
HAP_RESULT_USE_CHECK
HAPError TestHandleLightBulbColorTemperatureRead(
        HAPAccessoryServer* _Nullable server HAP_UNUSED,
        const HAPUInt32CharacteristicReadRequest* _Nullable request HAP_UNUSED,
        uint32_t* _Nullable value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s: %u", __func__, (unsigned int) *value);
    if (value) {
        *value = temperature;
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError TestHandleLightBulbColorTemperatureWrite(
        HAPAccessoryServer* _Nullable server HAP_UNUSED,
        const HAPUInt32CharacteristicWriteRequest* _Nullable request HAP_UNUSED,
        uint32_t value,
        void* _Nullable context HAP_UNUSED) {
    HAPError err = kHAPError_None;
    HAPLogInfo(&kHAPLog_Default, "%s: %u", __func__, (unsigned int) value);
    temperature = value;
    return err;
}

/**
 * The 'Brightness' characteristic of the Light Bulb service.
 */
const HAPIntCharacteristic lightBulbBrightnessCharacteristic = {
    .format = kHAPCharacteristicFormat_Int,
    .iid = kIID_LightBulbBrightness,
    .characteristicType = &kHAPCharacteristicType_Brightness,
    .debugDescription = kHAPCharacteristicDebugDescription_Brightness,
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
    .constraints = { .minimumValue = 0, .maximumValue = 100, .stepValue = 1 },
    .callbacks = { .handleRead = TestHandleLightBulbBrightnessRead, .handleWrite = TestHandleLightBulbBrightnessWrite }
};

/**
 * The 'Color Temperature' characteristic of the Light Bulb service.
 */
const HAPUInt32Characteristic lightBulbColorTemperatureCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt32,
    .iid = kIID_LightBulbColorTemp,
    .characteristicType = &kHAPCharacteristicType_ColorTemperature,
    .debugDescription = kHAPCharacteristicDebugDescription_ColorTemperature,
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

#if (HAVE_ADAPTIVE_LIGHT == 1)
    // Adaptive Light feature requires different range for Color Temperature
    .constraints = { .minimumValue = 166, .maximumValue = 500, .stepValue = 1 },
#else
    .constraints = { .minimumValue = 50, .maximumValue = 400, .stepValue = 1 },
#endif
    .callbacks = { .handleRead = TestHandleLightBulbColorTemperatureRead,
                   .handleWrite = TestHandleLightBulbColorTemperatureWrite }
};

#if (HAVE_ADAPTIVE_LIGHT == 1)

/**
 * The 'Supported Characteristic Value Transition Configuration' characteristic of the Light Bulb service.
 */
static const HAPTLV8Characteristic lightBulbSupportedTransitionConfigurationCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_LightBulbSupportedTransitionConfiguration,
    .characteristicType = &kHAPCharacteristicType_SupportedTransitionConfiguration,
    .debugDescription = kHAPCharacteristicDebugDescription_SupportedTransitionConfiguration,
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
    .callbacks = { .handleRead = HAPHandleSupportedTransitionConfigurationRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

/**
 * The 'Characteristic Value Transition Control' characteristic of the Light Bulb service.
 */
static const HAPTLV8Characteristic lightBulbTransitionControlCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_LightBulbTransitionControl,
    .characteristicType = &kHAPCharacteristicType_TransitionControl,
    .debugDescription = kHAPCharacteristicDebugDescription_TransitionControl,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .readRequiresAdminPermissions = false,
                    .writeRequiresAdminPermissions = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true, .supportsWriteResponse = true },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HAPHandleTransitionControlRead,
                   .handleWrite = HAPHandleTransitionControlWrite,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};
#endif

/**
 * The Light Bulb service that contains the 'On' characteristic.
 */
const HAPService lightBulbService = {
    .iid = kIID_LightBulb,
    .serviceType = &kHAPServiceType_LightBulb,
    .debugDescription = kHAPServiceDebugDescription_LightBulb,
    .name = "Light Bulb",
    .properties = { .primaryService = true, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &lightBulbServiceSignatureCharacteristic,
                                                            &lightBulbBrightnessCharacteristic,
                                                            &lightBulbColorTemperatureCharacteristic,

#if (HAVE_ADAPTIVE_LIGHT == 1)
                                                            &lightBulbSupportedTransitionConfigurationCharacteristic,
                                                            &lightBulbTransitionControlCharacteristic,
#endif
                                                            NULL }
};

/**
 * The 'Name' characteristic of the Battery service.
 */
static const HAPStringCharacteristic batteryNameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_BatteryName,
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

static uint8_t batteryLevel = 72;
static HAP_RESULT_USE_CHECK HAPError TestHandleBatteryLevelRead(
        HAPAccessoryServer* _Nonnull server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* _Nonnull request HAP_UNUSED,
        uint8_t* _Nullable value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s: %u", __func__, (unsigned int) *value);
    if (value) {
        *value = batteryLevel;
    }
    return kHAPError_None;
}

/**
 * The 'Battery Level' characteristic of the Battery service.
 */
const HAPUInt8Characteristic batteryLevelCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_BatteryLevel,
    .characteristicType = &kHAPCharacteristicType_BatteryLevel,
    .debugDescription = kHAPCharacteristicDebugDescription_BatteryLevel,
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
    .units = kHAPCharacteristicUnits_Percentage,
    .constraints = { .minimumValue = 0,
                     .maximumValue = 100,
                     .stepValue = 1,
                     .validValues = NULL,
                     .validValuesRanges = NULL },
    .callbacks = { .handleRead = TestHandleBatteryLevelRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

/**
 * Handle read request to the 'Charging State' characteristic of the Battery service.
 */
static HAP_RESULT_USE_CHECK HAPError TestHandleBatteryChargingStateRead(
        HAPAccessoryServer* _Nonnull server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* _Nonnull request,
        uint8_t* _Nonnull value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    // Query the battery sub-system and report battery charging state
    *value = kHAPCharacteristicValue_ChargingState_NotCharging;
    return kHAPError_None;
}

/**
 * The 'Charging State' characteristic of the Battery service.
 */
const HAPUInt8Characteristic batteryChargingStateCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_BatteryChargingState,
    .characteristicType = &kHAPCharacteristicType_ChargingState,
    .debugDescription = kHAPCharacteristicDebugDescription_ChargingState,
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
    .constraints = { .minimumValue = 0,
                     .maximumValue = 2,
                     .stepValue = 1,
                     .validValues = NULL,
                     .validValuesRanges = NULL },
    .callbacks = { .handleRead = TestHandleBatteryChargingStateRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

/**
 * Handle read request to the 'Status Low Battery' characteristic of the Battery service.
 */
static HAP_RESULT_USE_CHECK HAPError TestHandleBatteryStatusLowRead(
        HAPAccessoryServer* _Nonnull server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* _Nonnull request,
        uint8_t* _Nonnull value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    // Query the battery sub-system and report low battery status
    *value = kHAPCharacteristicValue_StatusLowBattery_Normal;
    return kHAPError_None;
}

/**
 * The 'Status Low Battery' characteristic of the Battery service.
 */
const HAPUInt8Characteristic batteryStatusLowCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_BatteryStatusLow,
    .characteristicType = &kHAPCharacteristicType_StatusLowBattery,
    .debugDescription = kHAPCharacteristicDebugDescription_StatusLowBattery,
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
    .constraints = { .minimumValue = 0,
                     .maximumValue = 1,
                     .stepValue = 1,
                     .validValues = NULL,
                     .validValuesRanges = NULL },
    .callbacks = { .handleRead = TestHandleBatteryStatusLowRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPService batteryService = {
    .iid = kIID_Battery,
    .serviceType = &kHAPServiceType_BatteryService,
    .debugDescription = kHAPServiceDebugDescription_BatteryService,
    .name = "Battery",
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &batteryLevelCharacteristic,
                                                            &batteryChargingStateCharacteristic,
                                                            &batteryStatusLowCharacteristic,
                                                            &batteryNameCharacteristic,
                                                            NULL }
};

#if (HAVE_NFC_ACCESS == 1)
const HAPTLV8Characteristic nfcAccessSupportedConfigurationCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_NfcAccessSupportedConfiguration,
    .characteristicType = &kHAPCharacteristicType_NfcAccessSupportedConfiguration,
    .debugDescription = kHAPCharacteristicDebugDescription_NfcAccessSupportedConfiguration,
    .manufacturerDescription = NULL,
    .properties = {
        .readable = true,
        .writable = false,
        .supportsEventNotification = false,
        .hidden = false,
        .requiresTimedWrite = false,
        .supportsAuthorizationData = false,
        .ip = {
            .controlPoint = false,
            .supportsWriteResponse = false,
        },
        .ble = {
            .supportsBroadcastNotification = false,
            .supportsDisconnectedNotification = false,
            .readableWithoutSecurity = false,
            .writableWithoutSecurity = false
        }
    },
    .callbacks = {
        .handleRead = HAPHandleNfcAccessSupportedConfigurationRead,
        .handleWrite = NULL,
    }
};

const HAPTLV8Characteristic nfcAccessControlPointCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_NfcAccessControlPoint,
    .characteristicType = &kHAPCharacteristicType_NfcAccessControlPoint,
    .debugDescription = kHAPCharacteristicDebugDescription_NfcAccessControlPoint,
    .manufacturerDescription = NULL,
    .properties = {
        .readable = true,
        .writable = true,
        .supportsEventNotification = false,
        .hidden = false,
        .requiresTimedWrite = false,
        .readRequiresAdminPermissions = true,
        .writeRequiresAdminPermissions = true,
        .supportsAuthorizationData = false,
        .ip = {
            .controlPoint = true,
            .supportsWriteResponse = true,
        },
        .ble = {
            .supportsBroadcastNotification = false,
            .supportsDisconnectedNotification = false,
            .readableWithoutSecurity = false,
            .writableWithoutSecurity = false
        }
    },
    .callbacks = {
        .handleRead = HAPHandleNfcAccessControlPointRead,
        .handleWrite = HAPHandleNfcAccessControlPointWrite,
    }
};

const HAPUInt16Characteristic nfcAccessConfigurationStateCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt16,
    .iid = kIID_NfcAccessConfigurationState,
    .characteristicType = &kHAPCharacteristicType_ConfigurationState,
    .debugDescription = kHAPCharacteristicDebugDescription_ConfigurationState,
    .manufacturerDescription = NULL,
    .properties = {
        .readable = true,
        .writable = false,
        .supportsEventNotification = true,
        .hidden = false,
        .requiresTimedWrite = false,
        .ip = {
            .controlPoint = false,
            .supportsWriteResponse = false,
        },
        .ble = {
            .supportsBroadcastNotification = true,
            .supportsDisconnectedNotification = true,
            .readableWithoutSecurity = false,
            .writableWithoutSecurity = false,
        }
    },
    .constraints = {
        .minimumValue = 0,
        .maximumValue = UINT16_MAX,
    },
    .callbacks = {
        .handleRead = HAPHandleNfcAccessConfigurationStateRead,
        .handleWrite = NULL,
    }
};

const HAPService nfcAccessService = {
    .iid = kIID_NfcAccess,
    .serviceType = &kHAPServiceType_NfcAccess,
    .debugDescription = kHAPServiceDebugDescription_NfcAccess,
    .name = NULL,
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &nfcAccessSupportedConfigurationCharacteristic,
                                                            &nfcAccessControlPointCharacteristic,
                                                            &nfcAccessConfigurationStateCharacteristic,
                                                            NULL }
};
#endif
