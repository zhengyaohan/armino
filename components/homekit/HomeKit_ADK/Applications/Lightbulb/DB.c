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

// This file contains the accessory attribute database that defines the accessory information service, HAP Protocol
// Information Service, the Pairing service and finally the service signature exposed by the light bulb.

#include "DB.h"
#include "App.h"

#if (HAVE_ADAPTIVE_LIGHT == 1)
#include "AdaptiveLight.h"
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * IID constants.
 */
#define kIID_LightBulb                 ((uint64_t) 0x0030)
#define kIID_LightBulbServiceSignature ((uint64_t) 0x0031)
#define kIID_LightBulbName             ((uint64_t) 0x0032)
#define kIID_LightBulbOn               ((uint64_t) 0x0033)
#define kIID_LightBulbBrightness       ((uint64_t) 0x0034)
#define kIID_LightBulbHue              ((uint64_t) 0x0035)
#define kIID_LightBulbSaturation       ((uint64_t) 0x0036)
#define kIID_LightBulbColorTemp        ((uint64_t) 0x0037)
#if (HAVE_ADAPTIVE_LIGHT == 1)
#define kIID_LightBulbTransitionControl                ((uint64_t) 0x0038)
#define kIID_LightBulbSupportedTransitionConfiguration ((uint64_t) 0x0039)
#define kIID_LightBulbTransitionCount                  ((uint64_t) 0x003A)
#endif

//----------------------------------------------------------------------------------------------------------------------

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

/**
 * The 'Name' characteristic of the Light Bulb service.
 */
static const HAPStringCharacteristic lightBulbNameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_LightBulbName,
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
 * The 'On' characteristic of the Light Bulb service.
 */
const HAPBoolCharacteristic lightBulbOnCharacteristic = {
    .format = kHAPCharacteristicFormat_Bool,
    .iid = kIID_LightBulbOn,
    .characteristicType = &kHAPCharacteristicType_On,
    .debugDescription = kHAPCharacteristicDebugDescription_On,
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
    .callbacks = { .handleRead = HandleLightBulbOnRead, .handleWrite = HandleLightBulbOnWrite }
};

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
    .callbacks = { .handleRead = HandleLightBulbBrightnessRead, .handleWrite = HandleLightBulbBrightnessWrite }
};

/**
 * The 'Hue' characteristic of the Light Bulb service.
 */
const HAPFloatCharacteristic lightBulbHueCharacteristic = {
    .format = kHAPCharacteristicFormat_Float,
    .iid = kIID_LightBulbHue,
    .characteristicType = &kHAPCharacteristicType_Hue,
    .debugDescription = kHAPCharacteristicDebugDescription_Hue,
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
    .constraints = { .minimumValue = 0.0F, .maximumValue = 360.0F, .stepValue = 1.0F },
    .callbacks = { .handleRead = HandleLightBulbHueRead, .handleWrite = HandleLightBulbHueWrite }
};

/**
 * The 'Saturation' characteristic of the Light Bulb service.
 */
const HAPFloatCharacteristic lightBulbSaturationCharacteristic = {
    .format = kHAPCharacteristicFormat_Float,
    .iid = kIID_LightBulbSaturation,
    .characteristicType = &kHAPCharacteristicType_Saturation,
    .debugDescription = kHAPCharacteristicDebugDescription_Saturation,
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
    .constraints = { .minimumValue = 0.0F, .maximumValue = 100.0F, .stepValue = 1.0F },
    .callbacks = { .handleRead = HandleLightBulbSaturationRead, .handleWrite = HandleLightBulbSaturationWrite }
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
    .callbacks = { .handleRead = HandleLightBulbColorTemperatureRead,
                   .handleWrite = HandleLightBulbColorTemperatureWrite }
};

#if (HAVE_ADAPTIVE_LIGHT == 1)
/**
 * The 'Supported Characteristic Value Transition Configuration' characteristic of the Light Bulb service.
 */
const HAPTLV8Characteristic lightBulbSupportedTransitionConfigurationCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_LightBulbSupportedTransitionConfiguration,
    .characteristicType = &kHAPCharacteristicType_SupportedTransitionConfiguration,
    .debugDescription = kHAPCharacteristicDebugDescription_SupportedTransitionConfiguration,
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
    .callbacks = { .handleRead = HAPHandleSupportedTransitionConfigurationRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

/**
 * The 'Characteristic Value Transition Control' characteristic of the Light Bulb service.
 */
const HAPTLV8Characteristic lightBulbTransitionControlCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_LightBulbTransitionControl,
    .characteristicType = &kHAPCharacteristicType_TransitionControl,
    .debugDescription = kHAPCharacteristicDebugDescription_TransitionControl,
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
    .callbacks = { .handleRead = HAPHandleTransitionControlRead,
                   .handleWrite = HAPHandleTransitionControlWrite,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

/**
 * The 'Characteristic Value Transition Count' characteristic of the Light Bulb service.
 */
const HAPUInt8Characteristic lightBulbTransitionCountCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_LightBulbTransitionCount,
    .characteristicType = &kHAPCharacteristicType_TransitionCount,
    .debugDescription = kHAPCharacteristicDebugDescription_TransitionCount,
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
    .constraints = { .minimumValue = 0, .maximumValue = kLightbulb_MaxSupportedTransitions, .stepValue = 1 },
    .callbacks = { .handleRead = HAPHandleTransitionCountRead,
                   .handleWrite = NULL,
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
                                                            &lightBulbNameCharacteristic,
                                                            &lightBulbOnCharacteristic,
#if (HAVE_COLOR_TEMPERATURE == 1)
                                                            &lightBulbColorTemperatureCharacteristic,
#else
                                                            &lightBulbHueCharacteristic,
                                                            &lightBulbSaturationCharacteristic,
#endif
                                                            &lightBulbBrightnessCharacteristic,
#if (HAVE_ADAPTIVE_LIGHT == 1)
// If Adaptive Light feature is on, the Lightbulb service must advertise Hue, Saturation, and Color Temp
// characteristics. Based on HAVE_COLOR_TEMPERATURE feature flag earlier, we advertised either (Hue+Saturation) or Color
// Temperature. Now advertise remaining characteristic(s).
#if (HAVE_COLOR_TEMPERATURE == 1)
                                                            &lightBulbHueCharacteristic,
                                                            &lightBulbSaturationCharacteristic,
#else
                                                            &lightBulbColorTemperatureCharacteristic,
#endif
                                                            &lightBulbSupportedTransitionConfigurationCharacteristic,
                                                            &lightBulbTransitionControlCharacteristic,
                                                            &lightBulbTransitionCountCharacteristic,
#endif
                                                            NULL }
};
