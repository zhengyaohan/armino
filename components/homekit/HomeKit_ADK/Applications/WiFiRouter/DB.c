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
// Information Service, the Pairing service and the Wi-Fi Router service.

#include "DB.h"
#include "App.h"
#if (HAP_TESTING == 1)
#include "DebugCommand.h"
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * IID constants.
 */
#define kIID_WiFiRouter                              ((uint64_t) 0x0030)
#define kIID_WiFiRouterServiceSignature              ((uint64_t) 0x0031)
#define kIID_WiFiRouterVersion                       ((uint64_t) 0x0033)
#define kIID_WiFiRouterSupportedRouterConfiguration  ((uint64_t) 0x0034)
#define kIID_WiFiRouterConfiguredName                ((uint64_t) 0x0035)
#define kIID_WiFiRouterRouterStatus                  ((uint64_t) 0x0036)
#define kIID_WiFiRouterWANConfigurationList          ((uint64_t) 0x0037)
#define kIID_WiFiRouterWANStatusList                 ((uint64_t) 0x0038)
#define kIID_WiFiRouterManagedNetworkEnable          ((uint64_t) 0x0039)
#define kIID_WiFiRouterNetworkClientProfileControl   ((uint64_t) 0x003A)
#define kIID_WiFiRouterNetworkClientStatusControl    ((uint64_t) 0x003B)
#define kIID_WiFiRouterNetworkAccessViolationControl ((uint64_t) 0x003C)

#define kIID_WiFiSatellite                    ((uint64_t) 0x0040)
#define kIID_WiFiSatelliteServiceSignature    ((uint64_t) 0x0041)
#define kIID_WiFiSatelliteWiFiSatelliteStatus ((uint64_t) 0x0043)

#if (HAVE_WIFI_RECONFIGURATION == 1)
// WiFi router will not have WiFi configuration control characteristics
#define kIID_WiFiTransport                 ((uint64_t) 0x0050)
#define kIID_WiFiTransportServiceSignature ((uint64_t) 0x0051)
#define kIID_WiFiTransportCurrentTransport ((uint64_t) 0x0052)
#define kIID_WiFiTransportWiFiCapability   ((uint64_t) 0x0053)
#endif

//----------------------------------------------------------------------------------------------------------------------

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

const HAPStringCharacteristic wiFiRouterConfiguredNameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_WiFiRouterConfiguredName,
    .characteristicType = &kHAPCharacteristicType_ConfiguredName,
    .debugDescription = kHAPCharacteristicDebugDescription_ConfiguredName,
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
    .constraints = { .maxLength = 32 },
    .callbacks = { .handleRead = HandleWiFiRouterConfiguredNameRead,
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
    .properties = { .primaryService = true, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &wiFiRouterServiceSignatureCharacteristic,
                                                            &wiFiRouterVersionCharacteristic,
                                                            &wiFiRouterSupportedRouterConfigurationCharacteristic,
                                                            &wiFiRouterConfiguredNameCharacteristic,
                                                            &wiFiRouterRouterStatusCharacteristic,
                                                            &wiFiRouterWANConfigurationListCharacteristic,
                                                            &wiFiRouterWANStatusListCharacteristic,
                                                            &wiFiRouterManagedNetworkEnableCharacteristic,
                                                            &wiFiRouterNetworkClientProfileControlCharacteristic,
                                                            &wiFiRouterNetworkClientStatusControlCharacteristic,
                                                            &wiFiRouterNetworkAccessViolationControlCharacteristic,
                                                            NULL }
};

//----------------------------------------------------------------------------------------------------------------------

const HAPDataCharacteristic wiFiSatelliteServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_WiFiSatelliteServiceSignature,
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

const HAPUInt8Characteristic wiFiSatelliteWiFiSatelliteStatusCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_WiFiSatelliteWiFiSatelliteStatus,
    .characteristicType = &kHAPCharacteristicType_WiFiSatelliteStatus,
    .debugDescription = kHAPCharacteristicDebugDescription_WiFiSatelliteStatus,
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
                     .maximumValue = 2,
                     .stepValue = 1,
                     .validValues = NULL,
                     .validValuesRanges = NULL },
    .callbacks = { .handleRead = HAPHandleWiFiRouterWiFiSatelliteStatusRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPService wiFiSatelliteService = {
    .iid = kIID_WiFiSatellite,
    .serviceType = &kHAPServiceType_WiFiSatellite,
    .debugDescription = kHAPServiceDebugDescription_WiFiSatellite,
    .name = NULL,
    .properties = { .primaryService = true, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &wiFiSatelliteServiceSignatureCharacteristic,
                                                            &wiFiSatelliteWiFiSatelliteStatusCharacteristic,
                                                            NULL }
};

#if (HAVE_WIFI_RECONFIGURATION == 1)
static const HAPBoolCharacteristic wiFiTransportCurrentTransportCharacteristic = {
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

static const HAPUInt32Characteristic wiFiTransportWiFiCapabilityCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt32,
    .iid = kIID_WiFiTransportWiFiCapability,
    .characteristicType = &kHAPCharacteristicType_WiFiCapability,
    .debugDescription = kHAPCharacteristicDebugDescription_WiFiCapability,
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
    .constraints = { .minimumValue = 0, .maximumValue = 15, .stepValue = 1 },
    .callbacks = { .handleRead = HAPHandleWiFiTransportWiFiCapabilityRead,
                   .handleWrite = NULL,
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
                                                            NULL }
};
#endif
