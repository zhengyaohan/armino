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
// Information Service, the Pairing service, the service signature and the Doorbell, RTP stream, microphone and speaker
// services exposed by the Video Doorbell.

#include "DB.h"
#include "App.h"
#include "DataStreamTransportManagementServiceDB.h"
#if (HAP_TESTING == 1)
#include "DebugCommand.h"
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * IID constants.
 */
#define kIID_Doorbell                        ((uint64_t) 0x0030)
#define kIID_DoorbellServiceSignature        ((uint64_t) 0x0031)
#define kIID_DoorbellName                    ((uint64_t) 0x0032)
#define kIID_DoorbellProgrammableSwitchEvent ((uint64_t) 0x0033)
#if (HAVE_VIDEODOORBELL_SILENT_MODE == 1)
#define kIID_DoorbellChimeMute ((uint64_t) 0x0034)
#endif
#if (HAVE_VIDEODOORBELL_OPERATING_STATE == 1)
#define kIID_DoorbellOperatingStateResponse ((uint64_t) 0x0035)
#endif

#define kIID_CameraRTPStreamManagement0                                  ((uint64_t) 0x0040)
#define kIID_CameraRTPStreamManagement0ServiceSignature                  ((uint64_t) 0x0041)
#define kIID_CameraRTPStreamManagement0Active                            ((uint64_t) 0x0042)
#define kIID_CameraRTPStreamManagement0StreamingStatus                   ((uint64_t) 0x0043)
#define kIID_CameraRTPStreamManagement0SupportedVideoStreamConfiguration ((uint64_t) 0x0044)
#define kIID_CameraRTPStreamManagement0SupportedAudioStreamConfiguration ((uint64_t) 0x0045)
#define kIID_CameraRTPStreamManagement0SupportedRTPConfiguration         ((uint64_t) 0x0046)
#define kIID_CameraRTPStreamManagement0SetupEndpoints                    ((uint64_t) 0x0047)
#define kIID_CameraRTPStreamManagement0SelectedRTPStreamConfiguration    ((uint64_t) 0x0048)

#define kIID_CameraRTPStreamManagement1                                  ((uint64_t) 0x0050)
#define kIID_CameraRTPStreamManagement1ServiceSignature                  ((uint64_t) 0x0051)
#define kIID_CameraRTPStreamManagement1Active                            ((uint64_t) 0x0052)
#define kIID_CameraRTPStreamManagement1StreamingStatus                   ((uint64_t) 0x0053)
#define kIID_CameraRTPStreamManagement1SupportedVideoStreamConfiguration ((uint64_t) 0x0054)
#define kIID_CameraRTPStreamManagement1SupportedAudioStreamConfiguration ((uint64_t) 0x0055)
#define kIID_CameraRTPStreamManagement1SupportedRTPConfiguration         ((uint64_t) 0x0056)
#define kIID_CameraRTPStreamManagement1SetupEndpoints                    ((uint64_t) 0x0057)
#define kIID_CameraRTPStreamManagement1SelectedRTPStreamConfiguration    ((uint64_t) 0x0058)

#define kIID_CameraEventRecordingManagement                                      ((uint64_t) 0x00A0)
#define kIID_CameraEventRecordingManagementActive                                ((uint64_t) 0x00A2)
#define kIID_CameraEventRecordingManagementSupportedCameraRecordingConfiguration ((uint64_t) 0x00A3)
#define kIID_CameraEventRecordingManagementSupportedVideoRecordingConfiguration  ((uint64_t) 0x00A4)
#define kIID_CameraEventRecordingManagementSupportedAudioRecordingConfiguration  ((uint64_t) 0x00A5)
#define kIID_CameraEventRecordingManagementSelectedCameraRecordingConfiguration  ((uint64_t) 0x00A6)
#define kIID_CameraEventRecordingManagementRecordingAudioActive                  ((uint64_t) 0x00A7)

#define kIID_CameraOperatingMode                             ((uint64_t) 0x00B0)
#define kIID_CameraOperatingModeEventSnapshotsActive         ((uint64_t) 0x00B2)
#define kIID_CameraOperatingModePeriodicSnapshotsActive      ((uint64_t) 0x00B3)
#define kIID_CameraOperatingModeHomeKitCameraActive          ((uint64_t) 0x00B4)
#define kIID_CameraOperatingModeThirdPartyCameraActive       ((uint64_t) 0x00B5)
#define kIID_CameraOperatingModeCameraOperatingModeIndicator ((uint64_t) 0x00B6)
#define kIID_CameraOperatingModeManuallyDisabled             ((uint64_t) 0x00B7)

//----------------------------------------------------------------------------------------------------------------------
/**
 * The 'Service Signature' characteristic of the Doorbell service.
 */
static const HAPDataCharacteristic doorbellServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_DoorbellServiceSignature,
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
 * The 'Name' characteristic of the Doorbell service.
 */
static const HAPStringCharacteristic doorbellNameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_DoorbellName,
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
 * The 'Programmable Switch Event' characteristic of the Doorbell service.
 */
const HAPUInt8Characteristic doorbellProgrammableSwitchEventCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_DoorbellProgrammableSwitchEvent,
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
    .callbacks = { .handleRead = HandleDoorbellProgrammableSwitchEventRead, .handleWrite = NULL }
};

#if (HAVE_VIDEODOORBELL_SILENT_MODE == 1)
/**
 * The 'Mute' characteristic of the doorbell service.
 */
const HAPBoolCharacteristic doorbellChimeMuteCharacteristic = {
    .format = kHAPCharacteristicFormat_Bool,
    .iid = kIID_DoorbellChimeMute,
    .characteristicType = &kHAPCharacteristicType_Mute,
    .debugDescription = kHAPCharacteristicDebugDescription_Mute,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .readRequiresAdminPermissions = false,
                    .writeRequiresAdminPermissions = true,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HandleChimeMuteRead, .handleWrite = HandleChimeMuteWrite }
};
#endif

#if (HAVE_VIDEODOORBELL_OPERATING_STATE == 1)
/**
 * The 'Operating State Response' characteristic of the Video Doorbell service.
 */
const HAPTLV8Characteristic doorbellOperatingResponseStateCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_DoorbellOperatingStateResponse,
    .characteristicType = &kHAPCharacteristicType_OperatingStateResponse,
    .debugDescription = kHAPCharacteristicDebugDescription_OperatingStateResponse,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HandleVideoDoorbellOperatingStateResponseRead, .handleWrite = NULL }
};
#endif

/**
 * The Doorbell service that contains the 'Programmable Switch Event' characteristic.
 */
const HAPService doorbellService = {
    .iid = kIID_Doorbell,
    .serviceType = &kHAPServiceType_Doorbell,
    .debugDescription = kHAPServiceDebugDescription_Doorbell,
    .name = "Doorbell",
    .properties = { .primaryService = true, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &doorbellServiceSignatureCharacteristic,
                                                            &doorbellNameCharacteristic,
                                                            &doorbellProgrammableSwitchEventCharacteristic,
#if (HAVE_VIDEODOORBELL_SILENT_MODE == 1)
                                                            &doorbellChimeMuteCharacteristic,
#endif
#if (HAVE_VIDEODOORBELL_OPERATING_STATE == 1)
                                                            &doorbellOperatingResponseStateCharacteristic,
#endif
                                                            NULL }
};

//----------------------------------------------------------------------------------------------------------------------

/**
 * The 'Streaming Status' characteristic of the first Camera RTP Stream Management service.
 */
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
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HAPHandleCameraRTPStreamManagementStreamingStatusRead, .handleWrite = NULL }
};

const HAPDataCharacteristic cameraRTPStreamManagement0ServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_CameraRTPStreamManagement0ServiceSignature,
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

const HAPUInt8Characteristic cameraRTPStreamManagement0ActiveCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_CameraRTPStreamManagement0Active,
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
        .requiresTimedWrite = true,
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
        .handleRead = HAPHandleCameraRTPStreamManagementActiveRead,
        .handleWrite = HAPHandleCameraRTPStreamManagementActiveWrite,
        .handleSubscribe = NULL,
        .handleUnsubscribe = NULL
    }
};

/**
 * The 'Supported Video Stream Configuration' characteristic of the first Camera RTP Stream Management service.
 */
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
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HAPHandleCameraRTPStreamManagementSupportedVideoStreamConfigurationRead,
                   .handleWrite = NULL }
};

/**
 * The 'Supported Audio Stream Configuration' characteristic of the first Camera RTP Stream Management service.
 */
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
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HAPHandleCameraRTPStreamManagementSupportedAudioStreamConfigurationRead,
                   .handleWrite = NULL }
};

/**
 * The 'Supported RTP Configuration' characteristic of the first Camera RTP Stream Management service.
 */
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
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HAPHandleCameraRTPStreamManagementSupportedRTPConfigurationRead, .handleWrite = NULL }
};

/**
 * The 'Setup Endpoints' characteristic of the first Camera RTP Stream Management service.
 */
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
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HAPHandleCameraRTPStreamManagementSetupEndpointsRead,
                   .handleWrite = HAPHandleCameraRTPStreamManagementSetupEndpointsWrite }
};

/**
 * The 'Selected RTP Stream Configuration' characteristic of the first Camera RTP Stream Management service.
 */
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
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HAPHandleCameraRTPStreamManagementSelectedRTPStreamConfigurationRead,
                   .handleWrite = HAPHandleCameraRTPStreamManagementSelectedRTPStreamConfigurationWrite }
};

/**
 * The first Camera RTP Stream Management service that contains the characteristics
 * for the first Video Doorbell stream.
 */
const HAPService cameraRTPStreamManagement0Service = {
    .iid = kIID_CameraRTPStreamManagement0,
    .serviceType = &kHAPServiceType_CameraRTPStreamManagement,
    .debugDescription = kHAPServiceDebugDescription_CameraRTPStreamManagement,
    .name = NULL,
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics =
            (const HAPCharacteristic* const[]) {
                    &cameraRTPStreamManagement0ServiceSignatureCharacteristic,
                    &cameraRTPStreamManagement0ActiveCharacteristic,
                    &cameraRTPStreamManagement0StreamingStatusCharacteristic,
                    &cameraRTPStreamManagement0SupportedVideoStreamConfigurationCharacteristic,
                    &cameraRTPStreamManagement0SupportedAudioStreamConfigurationCharacteristic,
                    &cameraRTPStreamManagement0SupportedRTPConfigurationCharacteristic,
                    &cameraRTPStreamManagement0SetupEndpointsCharacteristic,
                    &cameraRTPStreamManagement0SelectedRTPStreamConfigurationCharacteristic,
                    NULL }
};

//----------------------------------------------------------------------------------------------------------------------

const HAPDataCharacteristic cameraRTPStreamManagement1ServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_CameraRTPStreamManagement1ServiceSignature,
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

const HAPUInt8Characteristic cameraRTPStreamManagement1ActiveCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_CameraRTPStreamManagement1Active,
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
        .requiresTimedWrite = true,
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
        .handleRead = HAPHandleCameraRTPStreamManagementActiveRead,
        .handleWrite = HAPHandleCameraRTPStreamManagementActiveWrite,
        .handleSubscribe = NULL,
        .handleUnsubscribe = NULL
    }
};

/**
 * The 'Streaming Status' characteristic of the second Camera RTP Stream Management service.
 */
const HAPTLV8Characteristic cameraRTPStreamManagement1StreamingStatusCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_CameraRTPStreamManagement1StreamingStatus,
    .characteristicType = &kHAPCharacteristicType_StreamingStatus,
    .debugDescription = kHAPCharacteristicDebugDescription_StreamingStatus,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HAPHandleCameraRTPStreamManagementStreamingStatusRead, .handleWrite = NULL }
};

/**
 * The 'Supported Video Stream Configuration' characteristic of the second Camera RTP Stream Management service.
 */
const HAPTLV8Characteristic cameraRTPStreamManagement1SupportedVideoStreamConfigurationCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_CameraRTPStreamManagement1SupportedVideoStreamConfiguration,
    .characteristicType = &kHAPCharacteristicType_SupportedVideoStreamConfiguration,
    .debugDescription = kHAPCharacteristicDebugDescription_SupportedVideoStreamConfiguration,
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
    .callbacks = { .handleRead = HAPHandleCameraRTPStreamManagementSupportedVideoStreamConfigurationRead,
                   .handleWrite = NULL }
};

/**
 * The 'Supported Audio Stream Configuration' characteristic of the second Camera RTP Stream Management service.
 */
const HAPTLV8Characteristic cameraRTPStreamManagement1SupportedAudioStreamConfigurationCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_CameraRTPStreamManagement1SupportedAudioStreamConfiguration,
    .characteristicType = &kHAPCharacteristicType_SupportedAudioStreamConfiguration,
    .debugDescription = kHAPCharacteristicDebugDescription_SupportedAudioStreamConfiguration,
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
    .callbacks = { .handleRead = HAPHandleCameraRTPStreamManagementSupportedAudioStreamConfigurationRead,
                   .handleWrite = NULL }
};

/**
 * The 'Supported RTP Configuration' characteristic of the second Camera RTP Stream Management service.
 */
const HAPTLV8Characteristic cameraRTPStreamManagement1SupportedRTPConfigurationCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_CameraRTPStreamManagement1SupportedRTPConfiguration,
    .characteristicType = &kHAPCharacteristicType_SupportedRTPConfiguration,
    .debugDescription = kHAPCharacteristicDebugDescription_SupportedRTPConfiguration,
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
    .callbacks = { .handleRead = HAPHandleCameraRTPStreamManagementSupportedRTPConfigurationRead, .handleWrite = NULL }
};

/**
 * The 'Setup Endpoints' characteristic of the second Camera RTP Stream Management service.
 */
const HAPTLV8Characteristic cameraRTPStreamManagement1SetupEndpointsCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_CameraRTPStreamManagement1SetupEndpoints,
    .characteristicType = &kHAPCharacteristicType_SetupEndpoints,
    .debugDescription = kHAPCharacteristicDebugDescription_SetupEndpoints,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HAPHandleCameraRTPStreamManagementSetupEndpointsRead,
                   .handleWrite = HAPHandleCameraRTPStreamManagementSetupEndpointsWrite }
};

/**
 * The 'Selected RTP Stream Configuration' characteristic of the second Camera RTP Stream Management service.
 */
const HAPTLV8Characteristic cameraRTPStreamManagement1SelectedRTPStreamConfigurationCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_CameraRTPStreamManagement1SelectedRTPStreamConfiguration,
    .characteristicType = &kHAPCharacteristicType_SelectedRTPStreamConfiguration,
    .debugDescription = kHAPCharacteristicDebugDescription_SelectedRTPStreamConfiguration,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HAPHandleCameraRTPStreamManagementSelectedRTPStreamConfigurationRead,
                   .handleWrite = HAPHandleCameraRTPStreamManagementSelectedRTPStreamConfigurationWrite }
};

/**
 * The second Camera RTP Stream Management service that contains the characteristics
 * for the second Video Doorbell stream.
 */
const HAPService cameraRTPStreamManagement1Service = {
    .iid = kIID_CameraRTPStreamManagement1,
    .serviceType = &kHAPServiceType_CameraRTPStreamManagement,
    .debugDescription = kHAPServiceDebugDescription_CameraRTPStreamManagement,
    .name = NULL,
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics =
            (const HAPCharacteristic* const[]) {
                    &cameraRTPStreamManagement1ServiceSignatureCharacteristic,
                    &cameraRTPStreamManagement1ActiveCharacteristic,
                    &cameraRTPStreamManagement1StreamingStatusCharacteristic,
                    &cameraRTPStreamManagement1SupportedVideoStreamConfigurationCharacteristic,
                    &cameraRTPStreamManagement1SupportedAudioStreamConfigurationCharacteristic,
                    &cameraRTPStreamManagement1SupportedRTPConfigurationCharacteristic,
                    &cameraRTPStreamManagement1SetupEndpointsCharacteristic,
                    &cameraRTPStreamManagement1SelectedRTPStreamConfigurationCharacteristic,
                    NULL }
};

//----------------------------------------------------------------------------------------------------------------------

const HAPUInt8Characteristic cameraEventRecordingManagementActiveCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_CameraEventRecordingManagementActive,
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
        .requiresTimedWrite = true,
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
        .handleRead = HAPHandleCameraEventRecordingManagementActiveRead,
        .handleWrite = HAPHandleCameraEventRecordingManagementActiveWrite,
        .handleSubscribe = NULL,
        .handleUnsubscribe = NULL
    }
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
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
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
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
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
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
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
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HAPHandleCameraEventRecordingManagementSelectedCameraRecordingConfigurationRead,
                   .handleWrite = HAPHandleCameraEventRecordingManagementSelectedCameraRecordingConfigurationWrite,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

const HAPUInt8Characteristic cameraEventRecordingManagementRecordingAudioActiveCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_CameraEventRecordingManagementRecordingAudioActive,
    .characteristicType = &kHAPCharacteristicType_RecordingAudioActive,
    .debugDescription = kHAPCharacteristicDebugDescription_RecordingAudioActive,
    .manufacturerDescription = NULL,
    .properties = {
        .readable = true,
        .writable = true,
        .supportsEventNotification = true,
        .hidden = false,
        .readRequiresAdminPermissions = false,
        .writeRequiresAdminPermissions = true,
        .requiresTimedWrite = true,
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
        .handleRead = HAPHandleCameraEventRecordingManagementRecordingAudioActiveRead,
        .handleWrite = HAPHandleCameraEventRecordingManagementRecordingAudioActiveWrite,
        .handleSubscribe = NULL,
        .handleUnsubscribe = NULL
    }
};

const HAPService cameraEventRecordingManagementService = {
    .iid = kIID_CameraEventRecordingManagement,
    .serviceType = &kHAPServiceType_CameraEventRecordingManagement,
    .debugDescription = kHAPServiceDebugDescription_CameraEventRecordingManagement,
    .name = NULL,
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = (uint16_t const[]) { kIID_DataStreamTransportManagement, kIID_MotionSensor, 0 },
    .characteristics =
            (const HAPCharacteristic* const[]) {
                    &cameraEventRecordingManagementActiveCharacteristic,
                    &cameraEventRecordingManagementSupportedCameraRecordingConfigurationCharacteristic,
                    &cameraEventRecordingManagementSupportedVideoRecordingConfigurationCharacteristic,
                    &cameraEventRecordingManagementSupportedAudioRecordingConfigurationCharacteristic,
                    &cameraEventRecordingManagementSelectedCameraRecordingConfigurationCharacteristic,
                    &cameraEventRecordingManagementRecordingAudioActiveCharacteristic,
                    NULL }
};

//----------------------------------------------------------------------------------------------------------------------

const HAPUInt8Characteristic cameraOperatingModeEventSnapshotsActiveCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_CameraOperatingModeEventSnapshotsActive,
    .characteristicType = &kHAPCharacteristicType_EventSnapshotsActive,
    .debugDescription = kHAPCharacteristicDebugDescription_EventSnapshotsActive,
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
        .handleRead = HAPHandleCameraOperatingModeEventSnapshotsActiveRead,
        .handleWrite = HAPHandleCameraOperatingModeEventSnapshotsActiveWrite,
        .handleSubscribe = NULL,
        .handleUnsubscribe = NULL
    }
};

const HAPUInt8Characteristic cameraOperatingModePeriodicSnapshotsActiveCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_CameraOperatingModePeriodicSnapshotsActive,
    .characteristicType = &kHAPCharacteristicType_PeriodicSnapshotsActive,
    .debugDescription = kHAPCharacteristicDebugDescription_PeriodicSnapshotsActive,
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
        .handleRead = HAPHandleCameraOperatingModePeriodicSnapshotsActiveRead,
        .handleWrite = HAPHandleCameraOperatingModePeriodicSnapshotsActiveWrite,
        .handleSubscribe = NULL,
        .handleUnsubscribe = NULL
    }
};

const HAPUInt8Characteristic cameraOperatingModeHomeKitCameraActiveCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_CameraOperatingModeHomeKitCameraActive,
    .characteristicType = &kHAPCharacteristicType_HomeKitCameraActive,
    .debugDescription = kHAPCharacteristicDebugDescription_HomeKitCameraActive,
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
        .handleRead = HAPHandleCameraOperatingModeHomeKitCameraActiveRead,
        .handleWrite = HAPHandleCameraOperatingModeHomeKitCameraActiveWrite,
        .handleSubscribe = NULL,
        .handleUnsubscribe = NULL
    }
};

const HAPUInt8Characteristic cameraOperatingModeThirdPartyCameraActiveCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_CameraOperatingModeThirdPartyCameraActive,
    .characteristicType = &kHAPCharacteristicType_ThirdPartyCameraActive,
    .debugDescription = kHAPCharacteristicDebugDescription_ThirdPartyCameraActive,
    .manufacturerDescription = NULL,
    .properties = {
        .readable = true,
        .writable = false,
        .supportsEventNotification = true,
        .hidden = false,
        .readRequiresAdminPermissions = false,
        .writeRequiresAdminPermissions = false,
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
        .handleRead = HAPHandleCameraOperatingModeThirdPartyCameraActiveRead,
        .handleWrite = NULL,
        .handleSubscribe = NULL,
        .handleUnsubscribe = NULL
    }
};

const HAPUInt8Characteristic cameraOperatingModeCameraOperatingModeIndicatorCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_CameraOperatingModeCameraOperatingModeIndicator,
    .characteristicType = &kHAPCharacteristicType_CameraOperatingModeIndicator,
    .debugDescription = kHAPCharacteristicDebugDescription_CameraOperatingModeIndicator,
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
        .handleRead = HAPHandleCameraOperatingModeCameraOperatingModeIndicatorRead,
        .handleWrite = HAPHandleCameraOperatingModeCameraOperatingModeIndicatorWrite,
        .handleSubscribe = NULL,
        .handleUnsubscribe = NULL
    }
};

const HAPUInt8Characteristic cameraOperatingModeManuallyDisabledCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_CameraOperatingModeManuallyDisabled,
    .characteristicType = &kHAPCharacteristicType_ManuallyDisabled,
    .debugDescription = kHAPCharacteristicDebugDescription_ManuallyDisabled,
    .manufacturerDescription = NULL,
    .properties = {
        .readable = true,
        .writable = false,
        .supportsEventNotification = true,
        .hidden = false,
        .readRequiresAdminPermissions = false,
        .writeRequiresAdminPermissions = false,
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
        .handleRead = HAPHandleCameraOperatingModeManuallyDisabledRead,
        .handleWrite = NULL,
        .handleSubscribe = NULL,
        .handleUnsubscribe = NULL
    }
};

const HAPService cameraOperatingModeService = {
    .iid = kIID_CameraOperatingMode,
    .serviceType = &kHAPServiceType_CameraOperatingMode,
    .debugDescription = kHAPServiceDebugDescription_CameraOperatingMode,
    .name = NULL,
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics =
            (const HAPCharacteristic* const[]) { &cameraOperatingModeEventSnapshotsActiveCharacteristic,
                                                 &cameraOperatingModePeriodicSnapshotsActiveCharacteristic,
                                                 &cameraOperatingModeHomeKitCameraActiveCharacteristic,
                                                 &cameraOperatingModeThirdPartyCameraActiveCharacteristic,
                                                 &cameraOperatingModeCameraOperatingModeIndicatorCharacteristic,
                                                 &cameraOperatingModeManuallyDisabledCharacteristic,
                                                 NULL }
};
