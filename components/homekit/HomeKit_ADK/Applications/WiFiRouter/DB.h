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

// Basic Wi-Fi Router database example. This header file, and the corresponding DB.c implementation in the ADK, is
// platform-independent.

#ifndef DB_H
#define DB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP.h"

#include "AccessoryInformationServiceDB.h"
#include "ApplicationFeatures.h"
#if (HAP_TESTING == 1)
#include "DebugCommandServiceDB.h"
#endif
#if (HAP_APP_USES_HDS == 1)
#include "DataStreamTransportManagementServiceDB.h"
#endif
#if (HAVE_FIRMWARE_UPDATE == 1)
#include "FirmwareUpdateServiceDB.h"
#endif
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
#include "DiagnosticsServiceDB.h"
#endif

#include "PairingServiceDB.h"
#include "ProtocolInformationServiceDB.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif
/**
 * The number of Wi-Fi satellite accessories.
 */
#define kAppState_NumSatellites ((size_t) 2)

/**
 * Total number of services and characteristics contained in the Wi-Fi router accessory and Wi-Fi satellite accessories.
 */
enum {
    kAttributeCount = kAccessoryInformationServiceAttributeCount             // Accessory Information service
                      + kProtocolInformationServiceAttributeCount            // Protocol Information service
                      + kPairingServiceAttributeCount                        // Pairing service
                      + 12                                                   // WiFi Router service
                      + (3                                                   // WiFI Satellite service
                         + kAccessoryInformationServiceBridgedAttributeCount // Accessory Information service
                         ) * kAppState_NumSatellites
#if (HAVE_WIFI_RECONFIGURATION == 1)
                      + 4 // WiFi Transport service
#endif
#if (HAP_TESTING == 1)
                      + kDebugCommandServiceAttributeCount // Debug command service
#endif
#if (HAP_APP_USES_HDS == 1)
                      + kDataStreamTransportManagementServiceAttributeCount // Data Stream Transport Management service
#endif
#if (HAVE_FIRMWARE_UPDATE == 1)
                      + kFirmwareUpdateServiceAttributeCount // Firmware Update service
#endif
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
                      + kDiagnosticsServiceAttributeCount // Accessory Diagnostics service
#endif
};

//----------------------------------------------------------------------------------------------------------------------

#if (HAVE_WIFI_RECONFIGURATION == 1)

/**
 * Wi-Fi Transport service.
 */
extern const HAPService wiFiTransportService;
#endif

/**
 * The Wi-Fi Router service.
 */
extern const HAPService wiFiRouterService;

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
 * The 'Configured Name' characteristic of the Wi-Fi Router service.
 */
extern const HAPStringCharacteristic wiFiRouterConfiguredNameCharacteristic;

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

//----------------------------------------------------------------------------------------------------------------------

/**
 * HomeKit Accessory Information service for the bridged accessories.
 */
extern const HAPService accessoryInformationServiceBridged;

//----------------------------------------------------------------------------------------------------------------------

/**
 * The Wi-Fi Satellite service.
 */
extern const HAPService wiFiSatelliteService;

/**
 * The 'Service Signature' characteristic of the Wi-Fi Satellite service.
 */
extern const HAPDataCharacteristic wiFiSatelliteServiceSignatureCharacteristic;

/**
 * The 'Wi-Fi Satellite Status' characteristic of the Wi-Fi Satellite service.
 */
extern const HAPUInt8Characteristic wiFiSatelliteWiFiSatelliteStatusCharacteristic;

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
