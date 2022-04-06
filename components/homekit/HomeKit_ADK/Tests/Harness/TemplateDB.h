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

#ifndef TEMPLATE_DB_H
#define TEMPLATE_DB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP+API.h"
#include "HAPLogSubsystem.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * Total number of services and characteristics contained in the accessory.
 * This template is used by all tests, which include a variety of profiles and corresponding service/characteristic
 * collections. This value needs to represent the maximum required by the various tests.
 */
enum { kAttributeCount = 500 };

/**
 * HomeKit Accessory Information service
 */
extern const HAPService accessoryInformationService;

/**
 * Characteristics to expose accessory information and configuration (associated with accessory information service)
 */
extern const HAPBoolCharacteristic accessoryInformationIdentifyCharacteristic;
extern const HAPStringCharacteristic accessoryInformationManufacturerCharacteristic;
extern const HAPStringCharacteristic accessoryInformationModelCharacteristic;
extern const HAPStringCharacteristic accessoryInformationNameCharacteristic;
extern const HAPStringCharacteristic accessoryInformationSerialNumberCharacteristic;
extern const HAPStringCharacteristic accessoryInformationFirmwareRevisionCharacteristic;
extern const HAPStringCharacteristic accessoryInformationHardwareRevisionCharacteristic;
extern const HAPStringCharacteristic accessoryInformationADKVersionCharacteristic;
extern const HAPDataCharacteristic accessoryInformationProductDataCharacteristic;

/**
 * HAP Protocol Information Service
 */
extern const HAPService hapProtocolInformationService;

/**
 * Pairing Service
 */
extern const HAPService pairingService;

#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_RECONFIGURATION)

/**
 * The 'Current Transport' characteristic of the WiFi Transport service.
 */
extern const HAPBoolCharacteristic wiFiTransportCurrentTransportCharacteristic;

/**
 * The 'WiFi Capability' characteristic of the WiFi Transport service.
 */
extern const HAPUInt32Characteristic wiFiTransportWiFiCapabilityCharacteristic;

/**
 * The 'WiFi Configuration Control' characteristic of the WiFi Transport service.
 */
extern const HAPTLV8Characteristic wiFiTransportWiFiConfigurationControlCharacteristic;

/**
 * WiFi Transport service.
 */
extern const HAPService wiFiTransportService;

#endif

/**
 * The 'Active' characteristic of the Camera Event Recording Management service.
 */
extern const HAPUInt8Characteristic cameraEventRecordingManagementActiveCharacteristic;

/**
 * The 'Supported Camera Recording Configuration' characteristic of the Camera Event Recording Management service.
 */
extern const HAPTLV8Characteristic cameraEventRecordingManagementSupportedCameraRecordingConfigurationCharacteristic;

/**
 * The 'Supported Video Recording Configuration' characteristic of the Camera Event Recording Management service.
 */
extern const HAPTLV8Characteristic cameraEventRecordingManagementSupportedVideoRecordingConfigurationCharacteristic;

/**
 * The 'Supported Audio Recording Configuration' characteristic of the Camera Event Recording Management service.
 */
extern const HAPTLV8Characteristic cameraEventRecordingManagementSupportedAudioRecordingConfigurationCharacteristic;

/**
 * The 'Selected Camera Recording Configuration' characteristic of the Camera Event Recording Management service.
 */
extern const HAPTLV8Characteristic cameraEventRecordingManagementSelectedCameraRecordingConfigurationCharacteristic;

/**
 * Camera Event Recording Management Service
 */
extern const HAPService cameraEventRecordingManagementService;

/**
 * The 'Active' characteristic of the first Camera RTP Stream Management service.
 */
extern const HAPUInt8Characteristic cameraRTPStreamManagement0ActiveCharacteristic;

/**
 * The 'Supported Video Stream Configuration' characteristic of the first Camera RTP Stream Management service.
 */
extern const HAPTLV8Characteristic cameraRTPStreamManagement0SupportedVideoStreamConfigurationCharacteristic;

/**
 * The 'Supported Audio Stream Configuration' characteristic of the first Camera RTP Stream Management service.
 */
extern const HAPTLV8Characteristic cameraRTPStreamManagement0SupportedAudioStreamConfigurationCharacteristic;

/**
 * The 'Supported RTP Configuration' characteristic of the first Camera RTP Stream Management service.
 */
extern const HAPTLV8Characteristic cameraRTPStreamManagement0SupportedRTPConfigurationCharacteristic;

/**
 * The 'Selected RTP Stream Configuration' characteristic of the first Camera RTP Stream Management service.
 */
extern const HAPTLV8Characteristic cameraRTPStreamManagement0SelectedRTPStreamConfigurationCharacteristic;

/**
 * The 'Setup Endpoints' characteristic of the first Camera RTP Stream Management service.
 */
extern const HAPTLV8Characteristic cameraRTPStreamManagement0SetupEndpointsCharacteristic;

/**
 * The 'Streaming Status' characteristic of the first Camera RTP Stream Management service.
 */
extern const HAPTLV8Characteristic cameraRTPStreamManagement0StreamingStatusCharacteristic;

/**
 * First Camera RTP Stream Management Service
 */
extern const HAPService cameraRTPStreamManagement0Service;

/**
 * The 'Supported Data Stream Transport Configuration' characteristic of the Data Stream Transport Management service.
 */
extern const HAPTLV8Characteristic dataStreamTransportManagementSupportedDataStreamTransportConfigurationCharacteristic;

/**
 * The 'Setup Data Stream Transport' characteristic of the Data Stream Transport Management service.
 */
extern const HAPTLV8Characteristic dataStreamTransportManagementSetupDataStreamTransportCharacteristic;

/**
 * The 'Data Stream HAP Transport' characteristic of the Data Stream Transport Management service.
 */
extern const HAPTLV8Characteristic dataStreamTransportManagementDataStreamHAPTransportCharacteristic;

/**
 * The 'Version' characteristic of the Data Stream Transport Management service.
 */
extern const HAPStringCharacteristic dataStreamTransportManagementVersionCharacteristic;

/**
 * Data Stream Transport Management Service
 */
extern const HAPService dataStreamTransportManagementService;

/**
 * The 'Service Signature' characteristic of the Wi-Fi Router service.
 */
extern const HAPDataCharacteristic wiFiRouterServiceSignatureCharacteristic;

/**
 * The 'Version' characteristic of the Wi-Fi Router service.
 */
extern const HAPStringCharacteristic wiFiRouterVersionCharacteristic;

/**
 * The 'Supported Router Configuration' characteristic of the Wi-Fi Router service.
 */
extern const HAPTLV8Characteristic wiFiRouterSupportedRouterConfigurationCharacteristic;

/**
 * The 'Router Status' characteristic of the Wi-Fi Router service.
 */
extern const HAPUInt8Characteristic wiFiRouterRouterStatusCharacteristic;

/**
 * The 'WAN Configuration List' characteristic of the Wi-Fi Router service.
 */
extern const HAPTLV8Characteristic wiFiRouterWANConfigurationListCharacteristic;

/**
 * The 'WAN Status List' characteristic of the Wi-Fi Router service.
 */
extern const HAPTLV8Characteristic wiFiRouterWANStatusListCharacteristic;

/**
 * The 'Managed Network Enable' characteristic of the Wi-Fi Router service.
 */
extern const HAPUInt8Characteristic wiFiRouterManagedNetworkEnableCharacteristic;

/**
 * The 'Network Client Profile Control' characteristic of the Wi-Fi Router service.
 */
extern const HAPTLV8Characteristic wiFiRouterNetworkClientProfileControlCharacteristic;

/**
 * The 'Network Client Status Control' characteristic of the Wi-Fi Router service.
 */
extern const HAPTLV8Characteristic wiFiRouterNetworkClientStatusControlCharacteristic;

/**
 * The 'Network Access Violation Control' characteristic of the Wi-Fi Router service.
 */
extern const HAPTLV8Characteristic wiFiRouterNetworkAccessViolationControlCharacteristic;

/**
 * Wi-Fi Router Service
 */
extern const HAPService wiFiRouterService;

#if (HAVE_ACCESS_CODE == 1)

/**
 * The 'Access Code Supported Configuration' characteristic of the Access Code service.
 */
extern const HAPTLV8Characteristic accessCodeSupportedConfigurationCharacteristic;

/**
 * The 'Access Code Control Point' characteristic of the Access Code service.
 */
extern const HAPTLV8Characteristic accessCodeControlPointCharacteristic;

/**
 * The 'Configuration State' characteristic of the Access Code service.
 */
extern const HAPUInt16Characteristic accessCodeConfigurationStateCharacteristic;

/**
 * Access Code service.
 */
extern const HAPService accessCodeService;
#endif

/**
 * The 'Thread Role' characteristic of the Thread Management service.
 */
extern const HAPUInt16Characteristic threadManagementStatusCharacteristic;

/**
 * The 'Control' characteristic of the Thread Management service.
 */
extern const HAPTLV8Characteristic threadManagementControlCharacteristic;

/**
 * The 'Thread Current Transport' characteristic of the Thread Management service.
 */
extern const HAPBoolCharacteristic threadManagementCurrentTransportCharacteristic;

/**
 * Thread Management service
 */
extern const HAPService threadManagementService;

/**
 * The 'Heart Beat' characteristic of the Accessory Runtime Information service.
 */
extern const HAPUInt32Characteristic accessoryRuntimeInformationHeartBeatCharacteristic;

/**
 * Accessory Runtime Information service
 */
extern const HAPService accessoryRuntimeInformationService;

/**
 * The 'Brightness' characteristic of the Light Bulb service.
 */
extern const HAPIntCharacteristic lightBulbBrightnessCharacteristic;

/**
 * The 'Color Temperature' characteristic of the Light Bulb service.
 */
extern const HAPUInt32Characteristic lightBulbColorTemperatureCharacteristic;

/**
 * The Light Bulb service
 */
extern const HAPService lightBulbService;

/**
 * The 'Battery Level' characteristic of the Battery service.
 */
extern const HAPUInt8Characteristic batteryLevelCharacteristic;

/**
 * The 'Charging State' characteristic of the Battery service.
 */
extern const HAPUInt8Characteristic batteryChargingStateCharacteristic;

/**
 * The 'Status Low Battery' characteristic of the Battery service.
 */
extern const HAPUInt8Characteristic batteryStatusLowCharacteristic;

/**
 * The Battery service
 */
extern const HAPService batteryService;

#if HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)
extern const HAPTLV8Characteristic diagnosticsSupportedDiagnosticsSnapshot;
extern const HAPService diagnosticsService;
#endif

#if (HAVE_NFC_ACCESS == 1)

/**
 * The 'NFC Access Supported Configuration' characteristic of the NFC Access service.
 */
extern const HAPTLV8Characteristic nfcAccessSupportedConfigurationCharacteristic;

/**
 * The 'NFC Access Control Point' characteristic of the NFC Access service.
 */
extern const HAPTLV8Characteristic nfcAccessControlPointCharacteristic;

/**
 * The 'Configuration State' characteristic of the NFC Access service.
 */
extern const HAPUInt16Characteristic nfcAccessConfigurationStateCharacteristic;

/**
 * NFC Access service.
 */
extern const HAPService nfcAccessService;
#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
