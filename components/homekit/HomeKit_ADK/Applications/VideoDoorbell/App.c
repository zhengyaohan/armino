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

// An accessory that represents a Video Doorbell.
//
// This implementation is intended for POSIX platforms.
//
// The following signals are used:
//
// - Button 1 or signal `SIGUSR1` to clear pairings.
// - Button 2 or signal `SIGUSR2` to trigger a factory reset.
// - Button 3 or Signal `SIGTERM` to trigger pairing mode.
// - Button 4 or Signal `SIGQUIT` unused.
//
// The code consists of multiple parts:
//
//   1. The definition of the accessory configuration and its internal state.
//
//   2. Helper functions to load and save the state of the accessory.
//
//   3. The callbacks that implement the actual behavior of the accessory.
//
//   4. The video stream configurations for stream #1 and #2.
//
//   5. The signal handlers.
//
//   6. The setup of the accessory HomeKit and the bridged accessories attribute database.
//
//   7. The initialization of the accessory state.
//
//   8. A callback that gets called when the server state is updated.

#include "ApplicationFeatures.h"

#if (HAP_TESTING == 1)
#include <string.h>
#endif

#include "HAP.h"
#include "HAPAccessory+Camera.h"

#include "HAPPlatform+Init.h"
#include "HAPPlatformCamera+Configuration.h"
#include "HAPPlatformCamera+Init.h"
#include "HAPPlatformCameraInput+Init.h"
#include "HAPPlatformCameraOperatingMode.h"
#include "HAPPlatformCameraRecorder+Init.h"
#include "HAPPlatformKeyValueStore+SDKDomains.h"
#include "HAPPlatformMicrophone+Init.h"
#include "HAPPlatformSpeaker+Init.h"
#include "HAPPlatformTCPStreamManager+Init.h"

#include "AccessoryInformationServiceDB.h"
#include "App.h"
#include "AppBase.h"
#include "AppLED.h"
#if (HAP_TESTING == 1)
#include "AppUserInput.h"
#endif
#include "CameraConfiguration.h"
#include "DB.h"
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
#include "DiagnosticsService.h"
#include "DiagnosticsServiceDB.h"
#include "HAPDiagnostics.h"
#endif
#if (HAVE_ACCESSORY_REACHABILITY == 1)
#include "AccessoryRuntimeInformationServiceDB.h"
#endif
#if (HAP_APP_USES_HDS == 1)
#include "DataStreamTransportManagementServiceDB.h"
#endif
#if (HAVE_FIRMWARE_UPDATE == 1)
#include "FirmwareUpdate.h"
#include "FirmwareUpdateServiceDB.h"
#endif
#include "CameraSimulationHandlers.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * Key used in the key value store to store the configuration state.
 *
 * Purged: On factory reset.
 */
#define kAppKeyValueStoreKey_Configuration_State ((HAPPlatformKeyValueStoreKey) 0x00)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Next button press event to simulate.
 */
HAP_ENUM_BEGIN(uint8_t, NextPressEvent) { kNextPressEvent_Button_SinglePress,
                                          kNextPressEvent_Button_DoublePress,
                                          kNextPressEvent_Button_LongPress } HAP_ENUM_END(uint8_t, NextPressEvent);

/**
 * Motion sensor state.
 */
typedef struct {
    bool motionDetected;
    bool statusActive;
} MotionSensor;

typedef struct {
    AppLEDIdentifier identifyPin;
} Device;

/**
 * Global accessory configuration.
 */
typedef struct {
    HAPAccessoryServer* server;
    HAPPlatformKeyValueStoreRef keyValueStore;
    MotionSensor motionSensor;

    /**
     * Camera characteristics state.
     */
    struct {
        struct {
            bool mute;
            uint8_t volume;
        } microphone;
        struct {
            bool mute;
            uint8_t volume;
        } speaker;
#if (HAVE_VIDEODOORBELL_SILENT_MODE == 1)
        struct {
            bool mute;
        } chime;
#endif
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
        uint32_t diagnosticsSelectedModeState;
#endif // #if (HAVE_DIAGNOSTICS_SERVICE == 1)
    } state;

#if (HAVE_VIDEODOORBELL_OPERATING_STATE == 1)
    struct {
        uint8_t type;
        uint8_t reason;
    } operatingState;
#endif

    Device device;
    HAPPlatformTimerRef identifyTimer;
    HAPCharacteristicValue_ProgrammableSwitchEvent eventCache;
    NextPressEvent nextPressEvent;
    int maxVideoRecordingResolution;
} AccessoryConfiguration;

/**
 * Global accessory configuration.
 */
static AccessoryConfiguration accessoryConfiguration;
/**
 * App platform objects.
 */
static struct { PlatformCameraRecorder recorder; } appPlatform;

/**
 * Forward declaration of the global HomeKit accessory.
 */
static HAPAccessory accessory;

//----------------------------------------------------------------------------------------------------------------------

static void HandleMotionSensorStatusActiveChanged(HAPPlatformCameraRef camera);
static void HandleVisualIndicationChanged(HAPPlatformCameraRef camera);
static void UpdatePlatformVolume(void);

/**
 * Initialize the default accessory state
 */
static void SetupDefaultAccessoryState(void) {
    accessoryConfiguration.state.microphone.mute = false;
    accessoryConfiguration.state.microphone.volume = 100;
    accessoryConfiguration.state.speaker.mute = false;
    accessoryConfiguration.state.speaker.volume = 100;
#if (HAVE_VIDEODOORBELL_SILENT_MODE == 1)
    accessoryConfiguration.state.chime.mute = false;
#endif
#if (HAVE_VIDEODOORBELL_OPERATING_STATE == 1)
    accessoryConfiguration.operatingState.type = kHAPCharacteristicValue_OperatingStateResponse_State_Normal;
    accessoryConfiguration.operatingState.reason = kHAPCharacteristicValue_OperatingStateResponse_AbnormalReason_Other;
#endif
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
    accessoryConfiguration.state.diagnosticsSelectedModeState = kHAPCharacteristicValue_SelectedDiagnosticsModes_None;
#endif // HAVE_DIAGNOSTICS_SERVICE
}

/**
 * Update PAL with volume/mute value for microphone and speaker
 */
static void UpdatePlatformVolume(void) {
    HAPPrecondition(accessoryConfiguration.server);

    // Set volume and mute.
    HAPPlatformMicrophone* microphone = HAPCameraAccessoryGetCamera(accessoryConfiguration.server)->microphone;
    HAPPlatformMicrophoneSetVolume(microphone, accessoryConfiguration.state.microphone.volume);
    HAPPlatformMicrophoneSetMuted(microphone, accessoryConfiguration.state.microphone.mute);
    HAPPlatformSpeaker* speaker = HAPCameraAccessoryGetCamera(accessoryConfiguration.server)->speaker;
    HAPPlatformSpeakerSetVolume(speaker, accessoryConfiguration.state.speaker.volume);
    HAPPlatformSpeakerSetMuted(speaker, accessoryConfiguration.state.speaker.mute);
}

/**
 * Load the accessory state from persistent memory
 */
static void LoadAccessoryState(void) {
    HAPPrecondition(accessoryConfiguration.keyValueStore);

    HAPError err;

    // Load persistent state if available.
    bool found;
    size_t numBytes;

    err = HAPPlatformKeyValueStoreGet(
            accessoryConfiguration.keyValueStore,
            kAppKeyValueStoreDomain_Configuration,
            kAppKeyValueStoreKey_Configuration_State,
            &accessoryConfiguration.state,
            sizeof accessoryConfiguration.state,
            &numBytes,
            &found);

    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPFatalError();
    }
    if (!found || numBytes != sizeof accessoryConfiguration.state) {
        if (found) {
            HAPLog(&kHAPLog_Default, "Unexpected app state found in key-value store. Resetting to default.");
        }
        SetupDefaultAccessoryState();
    }

    accessoryConfiguration.motionSensor.motionDetected = false;
    accessoryConfiguration.motionSensor.statusActive = true;
    accessoryConfiguration.maxVideoRecordingResolution = 1080;
    HandleMotionSensorStatusActiveChanged(HAPCameraAccessoryGetCamera(accessoryConfiguration.server));
    HandleVisualIndicationChanged(HAPCameraAccessoryGetCamera(accessoryConfiguration.server));
}

/**
 * Save the accessory state to persistent memory
 */
static void SaveAccessoryState(void) {
    HAPPrecondition(accessoryConfiguration.keyValueStore);

    HAPError err;
    err = HAPPlatformKeyValueStoreSet(
            accessoryConfiguration.keyValueStore,
            kAppKeyValueStoreDomain_Configuration,
            kAppKeyValueStoreKey_Configuration_State,
            &accessoryConfiguration.state,
            sizeof accessoryConfiguration.state);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPFatalError();
    }
}

//----------------------------------------------------------------------------------------------------------------------
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
/**
 * Handle write request to the 'Selected Diagnostics Mode' characteristic of the Diagnostics service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleSelectedDiagnosticsModesWrite(
        HAPAccessoryServer* server,
        const HAPUInt32CharacteristicWriteRequest* request,
        uint32_t value,
        void* _Nullable context HAP_UNUSED) {
    return HandleSelectedDiagnosticsModesWriteHelper(
            server,
            request,
            value,
            context,
            &accessoryConfiguration.state.diagnosticsSelectedModeState,
            &SaveAccessoryState);
}
#endif // (HAVE_DIAGNOSTICS_SERVICE == 1)

//----------------------------------------------------------------------------------------------------------------------

/**
 * Control duration of Identify indication.
 */
static void IdentifyTimerExpired(HAPPlatformTimerRef timer, void* _Nullable context) {
    HAPLogDebug(&kHAPLog_Default, "%s", __func__);

    HAPPrecondition(!context);
    HAPPrecondition(timer == accessoryConfiguration.identifyTimer);

    accessoryConfiguration.identifyTimer = 0;

    AppLEDSet(accessoryConfiguration.device.identifyPin, false);
}

/**
 * Performs the Identify routine.
 */
static void DeviceIdentify(void) {
    HAPError err;

    if (accessoryConfiguration.identifyTimer) {
        HAPPlatformTimerDeregister(accessoryConfiguration.identifyTimer);
        accessoryConfiguration.identifyTimer = 0;
    }
    err = HAPPlatformTimerRegister(
            &accessoryConfiguration.identifyTimer,
            HAPPlatformClockGetCurrent() + 3 * HAPSecond,
            IdentifyTimerExpired,
            NULL);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
    }

    AppLEDSet(accessoryConfiguration.device.identifyPin, true);
}

/**
 * Identify routine. Used to locate the accessory.
 */
HAP_RESULT_USE_CHECK
HAPError IdentifyAccessory(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPAccessoryIdentifyRequest* request HAP_UNUSED,
        void* _Nullable context HAP_UNUSED) {
    HAPLogDebug(&kHAPLog_Default, "%s", __func__);
    /* VENDOR-TODO: Trigger LED/sound notification to identify accessory */
    DeviceIdentify();
    return kHAPError_None;
}

/**
 * Handle read request on the 'Programmable Switch Event' characteristic of the Doorbell service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleDoorbellProgrammableSwitchEventRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.eventCache;
    HAPLogInfo(&kHAPLog_Default, "%s: Event=%u (0-single, 1-double, 2-long)", __func__, *value);
    return kHAPError_None;
}

/**
 * Handle read request to the 'Mute' characteristic of the Microphone service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleMicrophoneMuteRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPBoolCharacteristicReadRequest* request HAP_UNUSED,
        bool* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.microphone.mute;
    HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, *value ? "true" : "false");
    return kHAPError_None;
}

/**
 * Handle write request to the 'Mute' characteristic of the Microphone service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleMicrophoneMuteWrite(
        HAPAccessoryServer* server,
        const HAPBoolCharacteristicWriteRequest* request,
        bool value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, value ? "true" : "false");
    if (accessoryConfiguration.state.microphone.mute != value) {
        accessoryConfiguration.state.microphone.mute = value;
        SaveAccessoryState();
        HAPPlatformMicrophoneSetMuted(HAPCameraAccessoryGetCamera(server)->microphone, value);
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }
    return kHAPError_None;
}

#if (HAVE_VIDEODOORBELL_SILENT_MODE == 1)
/**
 * Handle read request to the 'Mute' characteristic of the Doorbell service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleChimeMuteRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPBoolCharacteristicReadRequest* request HAP_UNUSED,
        bool* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.chime.mute;
    HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, *value ? "true" : "false");
    return kHAPError_None;
}

/**
 * Handle write request to the 'Mute' characteristic of the Doorbell service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleChimeMuteWrite(
        HAPAccessoryServer* server,
        const HAPBoolCharacteristicWriteRequest* request,
        bool value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, value ? "true" : "false");
    if (accessoryConfiguration.state.chime.mute != value) {
        accessoryConfiguration.state.chime.mute = value;
        SaveAccessoryState();
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }
    return kHAPError_None;
}
#endif

/**
 * Handle read request to the 'Volume' characteristic of the Microphone service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleMicrophoneVolumeRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.microphone.volume;
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, (int) *value);
    return kHAPError_None;
}

/**
 * Handle write request to the 'Volume' characteristic of the Microphone service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleMicrophoneVolumeWrite(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, (int) value);
    if (accessoryConfiguration.state.microphone.volume != value) {
        accessoryConfiguration.state.microphone.volume = value;
        SaveAccessoryState();
        HAPPlatformMicrophoneSetVolume(HAPCameraAccessoryGetCamera(server)->microphone, value);
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }
    return kHAPError_None;
}

/**
 * Handle read request to the 'Mute' characteristic of the Speaker service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleSpeakerMuteRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPBoolCharacteristicReadRequest* request HAP_UNUSED,
        bool* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.speaker.mute;
    HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, *value ? "true" : "false");
    return kHAPError_None;
}

/**
 * Handle write request to the 'Mute' characteristic of the Speaker service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleSpeakerMuteWrite(
        HAPAccessoryServer* server,
        const HAPBoolCharacteristicWriteRequest* request,
        bool value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, value ? "true" : "false");
    if (accessoryConfiguration.state.speaker.mute != value) {
        accessoryConfiguration.state.speaker.mute = value;
        SaveAccessoryState();
        HAPPlatformSpeakerSetMuted(HAPCameraAccessoryGetCamera(server)->speaker, value);
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }
    return kHAPError_None;
}

/**
 * Handle read request to the 'Volume' characteristic of the Speaker service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleSpeakerVolumeRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.speaker.volume;
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, (int) *value);
    return kHAPError_None;
}

/**
 * Handle write request to the 'Volume' characteristic of the Speaker service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleSpeakerVolumeWrite(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, (int) value);
    if (accessoryConfiguration.state.speaker.volume != value) {
        accessoryConfiguration.state.speaker.volume = value;
        SaveAccessoryState();
        HAPPlatformSpeakerSetVolume(HAPCameraAccessoryGetCamera(server)->speaker, value);
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }
    return kHAPError_None;
}

#if (HAVE_VIDEODOORBELL_OPERATING_STATE == 1)
HAP_RESULT_USE_CHECK
HAPError HandleVideoDoorbellOperatingStateResponseRead(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->accessory);
    HAPPrecondition(responseWriter);

    return HandleOperatingStateResponse(
            request,
            responseWriter,
            accessoryConfiguration.operatingState.type,
            accessoryConfiguration.operatingState.reason);
}

HAP_RESULT_USE_CHECK
HAPError HandleOperatingStateResponse(
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        uint8_t operatingStateType,
        uint8_t abnormalReason) {
    HAPPrecondition(request);
    HAPPrecondition(responseWriter);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_OperatingStateResponse));

    HAPError err = kHAPError_None;

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    // Operating State Type
    {
        uint8_t operatingStateTypeBytes[] = { operatingStateType };
        err = HAPTLVWriterAppend(
                responseWriter,
                &(const HAPTLV) {
                        .type = kHAPCharacteristicTLVType_OperatingStateResponseType_State,
                        .value = { .bytes = operatingStateTypeBytes, .numBytes = sizeof operatingStateTypeBytes } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
        HAPLogInfo(
                &kHAPLog_Default,
                "%s Operating State = %s",
                __func__,
                HAPCharacteristicValue_OperatingStateResponse_StateType_GetDescription(operatingStateType));
    }

    // Abnormal reason
    if (operatingStateType != kHAPCharacteristicValue_OperatingStateResponse_State_Normal) {
        uint8_t abnormalReasonBytes[] = { abnormalReason };
        err = HAPTLVWriterAppend(
                responseWriter,
                &(const HAPTLV) { .type = kHAPCharacteristicTLVType_OperatingStateResponseType_AbnormalReasons,
                                  .value = { .bytes = abnormalReasonBytes, .numBytes = sizeof abnormalReasonBytes } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
        HAPLogInfo(
                &kHAPLog_Default,
                "%s Operating State Reason = %s",
                __func__,
                HAPCharacteristicValue_OperatingStateResponse_AbnormalReason_GetDescription(abnormalReason));
    }
    return err;
}
#endif

/**
 * Handle read request to the 'Motion Detected' characteristic of the Motion Sensor service.
 */
HAPError HandleMotionSensorMotionDetectedRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPBoolCharacteristicReadRequest* request HAP_UNUSED,
        bool* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.motionSensor.motionDetected;
    HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, *value ? "true" : "false");
    return kHAPError_None;
}

/**
 * Handle read request to the 'Status Active' characteristic of the Motion Sensor service.
 */
HAPError HandleMotionSensorStatusActiveRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPBoolCharacteristicReadRequest* request HAP_UNUSED,
        bool* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.motionSensor.statusActive;
    HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, *value ? "true" : "false");
    return kHAPError_None;
}

/**
 * Toggle the motion sensor state.
 */
void ToggleMotionSensorState(void) {
    if (!accessoryConfiguration.motionSensor.statusActive) {
        return;
    }

    accessoryConfiguration.motionSensor.motionDetected = !accessoryConfiguration.motionSensor.motionDetected;

    HAPLogInfo(
            &kHAPLog_Default,
            "%s: %s",
            __func__,
            accessoryConfiguration.motionSensor.motionDetected ? "true" : "false");

    if (accessoryConfiguration.motionSensor.motionDetected) {
        HAPPlatformCameraTriggerRecording(HAPCameraAccessoryGetCamera(accessoryConfiguration.server));
    }

    HAPAccessoryServerRaiseEvent(
            accessoryConfiguration.server, &motionSensorMotionDetectedCharacteristic, &motionSensorService, &accessory);
}

#ifdef DARWIN
/**
 * Darwin specific function for handling video EOF.
 *
 * For Darwin, when instructed to record by the controller, we are no longer looping a video file
 * continuously. Instead, we are playing it to the end and then terminating the data stream. However, unless
 * the motion sensor is also toggled to the "off" position, the controller will keep requesting recording data
 * from the accessory. Therefore, when we reach the EOF for the video, we need to set motionDetected to false.
 * However, that is only valid if 1) The motionSensor is active, and 2) The motionSensor is currently on.
 *
 * @param camera    The accessory's camera
 */
static void HandleVideoEOFReached(HAPPlatformCameraRef camera) {
    if (!camera->recorder) {
        return;
    }

    if (camera->recorder->videoEOF && accessoryConfiguration.motionSensor.statusActive &&
        accessoryConfiguration.motionSensor.motionDetected) {
        HAPLogInfo(&kHAPLog_Default, "%s: motionDetected set to false", __func__);
        accessoryConfiguration.motionSensor.motionDetected = false;
        // Reset flag, since we are handing the videoEOF here.
        camera->recorder->videoEOF = false;
        HAPAccessoryServerRaiseEvent(
                accessoryConfiguration.server,
                &motionSensorMotionDetectedCharacteristic,
                &motionSensorService,
                &accessory);
    }
}
#endif

static void HandleMotionSensorStatusActiveChanged(HAPPlatformCameraRef camera) {
    // If the camera accessory exposes a Motion Sensor Service, the accessory must disable the motion sensor
    // when HomeKit Camera Active is set to Off by setting the Status Active characteristic on the Motion
    // Sensor Service to False.
    // See HomeKit Accessory Protocol Specification R17
    // Section 11.131 HomeKit Camera Active
    bool isHomeKitCameraActive;
    HAPError err = HAPPlatformCameraIsHomeKitCameraActive(camera, &isHomeKitCameraActive);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&kHAPLog_Default, "HAPPlatformCameraIsHomeKitCameraActive failed: %u.", err);
        return;
    }

    // If camera is turned off with a physical button, it will override both HK and third party operating modes.
    // See HomeKit Accessory Protocol Specification R17
    // Section 11.136 Manually Disabled
    bool isManuallyDisabled;
    err = HAPPlatformCameraIsManuallyDisabled(camera, &isManuallyDisabled);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&kHAPLog_Default, "HAPPlatformCameraIsManuallyDisabled failed: %u.", err);
        return;
    }

    bool active = isHomeKitCameraActive & !isManuallyDisabled;

    if (active != accessoryConfiguration.motionSensor.statusActive) {
        accessoryConfiguration.motionSensor.statusActive = active;

        HAPAccessoryServerRaiseEvent(
                accessoryConfiguration.server,
                &motionSensorStatusActiveCharacteristic,
                &motionSensorService,
                &accessory);
    }
}

static void HandleVisualIndicationChanged(HAPPlatformCameraRef camera) {
    HAPPrecondition(camera);

    bool isIndicatorEnabled;
    HAPError err = HAPPlatformCameraIsOperatingModeIndicatorEnabled(camera, &isIndicatorEnabled);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&kHAPLog_Default, "HAPPlatformCameraIsOperatingModeIndicatorEnabled failed: %u.", err);
        return;
    }

    if (!isIndicatorEnabled) {
        HAPLog(&kHAPLog_Default, "LED disabled.");
    } else if (HAPPlatformCameraIsRecordingEnabled(camera) || HAPPlatformCameraIsStreamingInUse(camera)) {
        HAPLog(&kHAPLog_Default, "LED red.");
    } else if (HAPPlatformCameraIsStreamingEnabled(camera)) {
        HAPLog(&kHAPLog_Default, "LED blue.");
    } else {
        HAPLog(&kHAPLog_Default, "LED off.");
    }
}

static void HandleOperatingModeChanged(HAPPlatformCameraRef camera, bool configurationChanged) {
    HAPPrecondition(camera);

    if (configurationChanged) {
        if (HAPPlatformCameraIsRecordingEnabled(camera)) {
            HAPError err = HAPPlatformCameraConfigureRecording(camera);
            if (err) {
                HAPLogError(&kHAPLog_Default, "Configuring camera recording stream failed.");
            }
        } else {
            HAPPlatformCameraDisableRecording(camera);
        }
    }
#ifdef DARWIN
    HandleVideoEOFReached(camera);
#endif
    HandleMotionSensorStatusActiveChanged(camera);
    HandleVisualIndicationChanged(camera);
}

static void HandleGetSupportedRecordingConfiguration(
        HAPAccessoryServer* server,
        const HAPService* service,
        const HAPAccessory* accessory_,
        HAPCameraAccessoryGetSupportedRecordingConfigurationCompletionHandler completionHandler,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(service == &cameraEventRecordingManagementService);
    HAPPrecondition(accessory_ == &accessory);
    HAPPrecondition(completionHandler);

    const HAPCameraRecordingSupportedConfiguration* config;
    if (accessoryConfiguration.maxVideoRecordingResolution >= 1080) {
        config = &supportedCameraRecordingConfiguration;
    } else if (accessoryConfiguration.maxVideoRecordingResolution >= 720) {
        config = &supportedLowResCameraRecordingConfiguration;
    } else {
        config = &supportedLowestResCameraRecordingConfiguration;
    }

    completionHandler(server, service, accessory_, config);
}

HAP_RESULT_USE_CHECK
static HAPError HandleResolutionChanged(HAPPlatformCameraRef camera, const HAPVideoAttributes* _Nullable attributes) {
    // Reduce recording resolution if needed to avoid camera overload.
    // Only needed if the camera input is restricted to two streams.
    int resolution;
    if (attributes && HAPMin(attributes->width, attributes->height) > 720) {
        // A high resolution RTP stream is requested.
        if (HAPPlatformCameraGetRecordingResolution(camera) > 720) {
            // We cannot reduce the recording resolution if a high resolution stream is already running.
            return kHAPError_InvalidData;
        }
        resolution = 720;
    } else {
        resolution = 1080;
    }

    if (resolution != accessoryConfiguration.maxVideoRecordingResolution) {
        accessoryConfiguration.maxVideoRecordingResolution = resolution;

        HAPCameraAccessoryHandleSupportedRecordingConfigurationChange(
                accessoryConfiguration.server,
                &cameraEventRecordingManagementService,
                &accessory,
                kHAPCameraSupportedRecordingConfigurationChange_Video);
    }
    return kHAPError_None;
}

/**
 * Change supported recording configuration.
 *
 * Cycles through the following configurations:
 * - high resolution   (up to 1080p)
 * - low resolution    (up to 720p)
 * - lowest resolution (up to 480p)
 */
void ChangeSupportedRecordingConfig(void) {
#if (HAP_TESTING == 1)
    SimulateChangeSupportedRecordingConfig(&accessoryConfiguration.maxVideoRecordingResolution);
#endif // (HAP_TESTING == 1)
}

//----------------------------------------------------------------------------------------------------------------------

#if (HAP_APP_USES_HDS == 1)
#if (HAP_APP_USES_HDS_STREAM == 1)
static HAPStreamDataStreamProtocol streamDataStreamProtocol = {
    .base = &kHAPStreamDataStreamProtocol_Base,
    .numDataStreams = kApp_NumDataStreams,
    .applicationProtocols = (HAPStreamApplicationProtocol* const[]) { &streamProtocolUARP, NULL }
};
#endif // HAP_APP_USES_HDS_STREAM

static HAPDataSendStreamProtocolStreamAvailableCallbacks dataSendDataStreamProtocolAvailableCallbacks[] = {
    { .type = kHAPDataSendDataStreamProtocolType_IPCamera_Recording,
      .handleStreamAvailable = HAPPlatformCameraRecorderHandleDataSendStreamAvailable },
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
    { .type = kHAPDataSendDataStreamProtocolType_Diagnostics_Snapshot,
      .handleStreamAvailable = HAPDiagnosticsHandleDataSendStreamAvailable }
#endif
};

static HAPDataSendDataStreamProtocolContext dataSendDataStreamProtocolContexts[kApp_NumDataStreams];
static HAPDataSendDataStreamProtocolListener dataSendDataStreamProtocolListeners[kApp_NumDataStreams];
static HAPDataSendDataStreamProtocol dataSendDataStreamProtocol = {
    .base = &kHAPDataSendDataStreamProtocol_Base,
    .storage = { .numDataStreams = kApp_NumDataStreams,
                 .protocolContexts = dataSendDataStreamProtocolContexts,
                 .listeners = dataSendDataStreamProtocolListeners },
    .callbacks = { .numStreamAvailableCallbacks = 1
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
                                                  + 1
#endif
                   ,
                   .streamAvailableCallbacks = dataSendDataStreamProtocolAvailableCallbacks }
};

static HAPDataStreamDescriptor dataStreamDescriptors[kApp_NumDataStreams];
static HAPDataStreamDispatcher dataStreamDispatcher;
const HAPDataStreamDispatcherStorage dataStreamDispatcherStorage = {
    .numDataStreams = kApp_NumDataStreams,
    .dataStreamDescriptors = dataStreamDescriptors,
    .dataStreamProtocols =
            (HAPDataStreamProtocol* const[]) {
                    &dataSendDataStreamProtocol,
#if (HAP_APP_USES_HDS_STREAM == 1)
                    &streamDataStreamProtocol,
#endif
                    NULL,
            },
};
#endif // HAP_APP_USES_HDS

//----------------------------------------------------------------------------------------------------------------------

/**
 * HomeKit accessory that provides the streaming services and the Microphone and Speaker services.
 */
static HAPAccessory accessory = { .aid = 1,
                                  .category = kHAPAccessoryCategory_VideoDoorbells,
                                  .name = "Acme Video Doorbell",
                                  .productData = "03d8a775e3644573",
                                  .manufacturer = "Acme",
                                  .model = "VideoDoorbell1,1",
                                  .serialNumber = "099DB48E9E28",
                                  .firmwareVersion = "1",
                                  .hardwareVersion = "1",
                                  .services = (const HAPService* const[]) { &accessoryInformationService,
                                                                            &hapProtocolInformationService,
                                                                            &pairingService,
                                                                            &doorbellService,
                                                                            &cameraRTPStreamManagement0Service,
                                                                            &cameraRTPStreamManagement1Service,
                                                                            &microphoneService,
                                                                            &speakerService,
                                                                            &cameraEventRecordingManagementService,
                                                                            &cameraOperatingModeService,
                                                                            &motionSensorService,
#if (HAVE_ACCESSORY_REACHABILITY == 1)
                                                                            &accessoryRuntimeInformationService,
#endif
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
                                                                            &diagnosticsService,
#endif
#if (HAP_TESTING == 1)
                                                                            &debugService,
#endif
#if (HAP_APP_USES_HDS == 1)
                                                                            &dataStreamTransportManagementService,
#endif
#if (HAVE_FIRMWARE_UPDATE == 1)
                                                                            &firmwareUpdateService,
#endif
                                                                            NULL },
#if (HAP_APP_USES_HDS == 1)
                                  .dataStream.delegate = { .callbacks = &kHAPDataStreamDispatcher_DataStreamCallbacks,
                                                           .context = &dataStreamDispatcher },
#endif
                                  .cameraStreamConfigurations =
                                          (const HAPCameraStreamSupportedConfigurations* const[]) {
                                                  &supportedCameraStreamConfigurations0,
                                                  &supportedCameraStreamConfigurations1,
                                                  NULL },
                                  .callbacks = {
                                          .identify = IdentifyAccessory,
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
                                          .diagnosticsConfig = { .getDiagnosticsConfig =
                                                                         GetAccessoryDiagnosticsConfig },
#endif
                                          .camera = { .getSupportedRecordingConfiguration =
                                                              HandleGetSupportedRecordingConfiguration },
#if (HAVE_FIRMWARE_UPDATE == 1)
                                          .firmwareUpdate = { .getAccessoryState = FirmwareUpdateGetAccessoryState },
#endif
                                  } };

//----------------------------------------------------------------------------------------------------------------------

/**
 * Simulate button press events in a loop.
 */
void SimulateSinglePressEvent(void) {
    HAPCharacteristicValue_ProgrammableSwitchEvent value = 0;
    value = kHAPCharacteristicValue_ProgrammableSwitchEvent_SinglePress;

#if (HAVE_VIDEODOORBELL_SILENT_MODE == 1)
    if (!accessoryConfiguration.state.chime.mute) {
        HAPLogInfo(&kHAPLog_Default, "%s: Chiming sound", __func__);
    } else {
        HAPLogInfo(&kHAPLog_Default, "%s: Chiming sound muted", __func__);
    }
#endif
    accessoryConfiguration.eventCache = value;
    HAPLogInfo(&kHAPLog_Default, "%s: eventCache=%u", __func__, value);
    HAPAccessoryServerRaiseEvent(
            accessoryConfiguration.server,
            &doorbellProgrammableSwitchEventCharacteristic,
            &doorbellService,
            &accessory);
}

/**
 * Simulate button press events in a loop.
 */
void SimulateDoublePressEvent(void) {
    HAPCharacteristicValue_ProgrammableSwitchEvent value = 0;
    value = kHAPCharacteristicValue_ProgrammableSwitchEvent_DoublePress;

#if (HAVE_VIDEODOORBELL_SILENT_MODE == 1)
    if (!accessoryConfiguration.state.chime.mute) {
        HAPLogInfo(&kHAPLog_Default, "%s: Chiming sound", __func__);
    } else {
        HAPLogInfo(&kHAPLog_Default, "%s: Chiming sound muted", __func__);
    }
#endif
    accessoryConfiguration.eventCache = value;
    HAPLogInfo(&kHAPLog_Default, "%s: eventCache=%u", __func__, value);
    HAPAccessoryServerRaiseEvent(
            accessoryConfiguration.server,
            &doorbellProgrammableSwitchEventCharacteristic,
            &doorbellService,
            &accessory);
}

/**
 * Simulate button press events in a loop.
 */
void SimulateLongPressEvent(void) {
    HAPCharacteristicValue_ProgrammableSwitchEvent value = 0;
    value = kHAPCharacteristicValue_ProgrammableSwitchEvent_LongPress;

#if (HAVE_VIDEODOORBELL_SILENT_MODE == 1)
    if (!accessoryConfiguration.state.chime.mute) {
        HAPLogInfo(&kHAPLog_Default, "%s: Chiming sound", __func__);
    } else {
        HAPLogInfo(&kHAPLog_Default, "%s: Chiming sound muted", __func__);
    }
#endif
    accessoryConfiguration.eventCache = value;
    HAPLogInfo(&kHAPLog_Default, "%s: eventCache=%u", __func__, value);
    HAPAccessoryServerRaiseEvent(
            accessoryConfiguration.server,
            &doorbellProgrammableSwitchEventCharacteristic,
            &doorbellService,
            &accessory);
}

#if (HAVE_VIDEODOORBELL_OPERATING_STATE == 1)
/**
 * Simulate various Thermal Operating States
 */
void SimulateOperatingStates(void) {

    if (accessoryConfiguration.operatingState.type == kHAPCharacteristicValue_OperatingStateResponse_State_Normal) {
        accessoryConfiguration.operatingState.type =
                kHAPCharacteristicValue_OperatingStateResponse_State_LimitedFunctionality;
        accessoryConfiguration.operatingState.reason =
                kHAPCharacteristicValue_OperatingStateResponse_AbnormalReason_Other;
    } else if (
            accessoryConfiguration.operatingState.reason ==
            kHAPCharacteristicValue_OperatingStateResponse_AbnormalReason_Other) {
        accessoryConfiguration.operatingState.reason =
                kHAPCharacteristicValue_OperatingStateResponse_AbnormalReason_LowTemperature;
    } else if (
            accessoryConfiguration.operatingState.reason ==
            kHAPCharacteristicValue_OperatingStateResponse_AbnormalReason_LowTemperature) {
        accessoryConfiguration.operatingState.reason =
                kHAPCharacteristicValue_OperatingStateResponse_AbnormalReason_HighTemperature;
    } else if (
            accessoryConfiguration.operatingState.reason ==
            kHAPCharacteristicValue_OperatingStateResponse_AbnormalReason_HighTemperature) {
        accessoryConfiguration.operatingState.type++;
        accessoryConfiguration.operatingState.reason =
                kHAPCharacteristicValue_OperatingStateResponse_AbnormalReason_Other;
        if (accessoryConfiguration.operatingState.type >
            kHAPCharacteristicValue_OperatingStateResponse_State_ShutdownImminent) {
            accessoryConfiguration.operatingState.type = kHAPCharacteristicValue_OperatingStateResponse_State_Normal;
        } else if (
                accessoryConfiguration.operatingState.type ==
                kHAPCharacteristicValue_OperatingStateResponse_State_ShutdownImminent) {
            HAPLogInfo(&kHAPLog_Default, "%s: accessory may shutdown soon", __func__);
        }
    }

    HAPLogInfo(
            &kHAPLog_Default,
            "%s: Operating State = %s",
            __func__,
            HAPCharacteristicValue_OperatingStateResponse_StateType_GetDescription(
                    accessoryConfiguration.operatingState.type));
    if (accessoryConfiguration.operatingState.type != kHAPCharacteristicValue_OperatingStateResponse_State_Normal) {
        HAPLogInfo(
                &kHAPLog_Default,
                "%s: Operating State reason = %s",
                __func__,
                HAPCharacteristicValue_OperatingStateResponse_AbnormalReason_GetDescription(
                        accessoryConfiguration.operatingState.reason));
    }
    HAPAccessoryServerRaiseEvent(
            accessoryConfiguration.server, &doorbellOperatingResponseStateCharacteristic, &doorbellService, &accessory);
}
#endif

#if (HAP_TESTING == 1)

//----------------------------------------------------------------------------------------------------------------------
/**
 * Global user input handler.
 *
 * Button and signal mapping to kAppUserInputIdentifier can be found at Applications/Common/AppUserInput.h
 */
static void HandleUserInputEventCallback(void* _Nullable context, size_t contextSize) {
    HAPPrecondition(context);
    HAPAssert(contextSize == sizeof(AppUserInputEvent));

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    AppUserInputEvent buttonEvent = *((AppUserInputEvent*) context);
    switch (buttonEvent.id) {
        case kAppUserInputIdentifier_4: { // SIGQUIT or Button 4
            break;
        }
        default: {
            break;
        }
    }
}

#endif

/**
 * Unconfigure platform specific IO.
 */
static void UnconfigureIO(void) {
    HAPLogDebug(&kHAPLog_Default, "%s", __func__);

    AppLEDDeinit();
}

/**
 * Configure platform specific IO.
 */
static void ConfigureIO(void) {
    HAPLogDebug(&kHAPLog_Default, "%s", __func__);

#if (HAP_TESTING == 1)
    // Setup user input event handler
    AppUserInputRegisterCallback(HandleUserInputEventCallback);
#endif

    // Assign LEDs.
    accessoryConfiguration.device.identifyPin = kAppLEDIdentifier_2;

    // Initialize LED driver
    AppLEDInit();
}

//----------------------------------------------------------------------------------------------------------------------

void AppCreate(HAPAccessoryServer* server, HAPPlatformKeyValueStoreRef keyValueStore) {
#if (HAVE_BLE == 1)
    HAPLogError(&kHAPLog_Default, "%s: Application not supported on BLE", __func__);
    HAPFatalError();
#endif // BLE

    HAPPrecondition(server);
    HAPPrecondition(keyValueStore);

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    HAPRawBufferZero(&accessoryConfiguration, sizeof accessoryConfiguration);
    accessoryConfiguration.server = server;
    accessoryConfiguration.keyValueStore = keyValueStore;

    // Load persistent state.
    // Load accessory state before calling HAPPlatformCameraReloadRecorderConfiguration, which
    // can trigger a SaveAccessoryState().
    LoadAccessoryState();

    // Install recorder delegate.
    HAPPlatformCameraRecorderDelegate delegate = { HandleResolutionChanged, HandleOperatingModeChanged };
    HAPPlatformCameraSetRecorderDelegate(HAPCameraAccessoryGetCamera(server), &delegate);

    HAPPlatformCameraReloadRecorderConfiguration(HAPCameraAccessoryGetCamera(server));

    ConfigureIO();

    UpdatePlatformVolume();

#if (HAP_APP_USES_HDS == 1)
    // Initialize HomeKit Data Stream dispatcher.
    HAPDataStreamDispatcherCreate(
            server,
            &dataStreamDispatcher,
            &(const HAPDataStreamDispatcherOptions) { .storage = &dataStreamDispatcherStorage });
#endif

#if (HAVE_FIRMWARE_UPDATE == 1)
    UARPInitialize(accessoryConfiguration.server, &accessory);

    FirmwareUpdateOptions fwupOptions = { 0 };
#if (HAP_TESTING == 1)
    fwupOptions.persistStaging = server->firmwareUpdate.persistStaging;
#endif
    FirmwareUpdateInitialize(
            accessoryConfiguration.server, &accessory, accessoryConfiguration.keyValueStore, &fwupOptions);
#endif // HAVE_FIRMWARE_UPDATE

    // Update supported recording configurations with trigger type as doorbell
    supportedCameraRecordingConfiguration.recording.eventTriggerTypes |= kHAPCameraEventTriggerTypes_Doorbell;
    supportedLowResCameraRecordingConfiguration.recording.eventTriggerTypes |= kHAPCameraEventTriggerTypes_Doorbell;
    supportedLowestResCameraRecordingConfiguration.recording.eventTriggerTypes |= kHAPCameraEventTriggerTypes_Doorbell;
}

void AppRelease(HAPAccessoryServer* _Nonnull server HAP_UNUSED, void* _Nullable context HAP_UNUSED) {
    UnconfigureIO();
}

void AppAccessoryServerStart(void) {
    HAPAccessoryServerStart(accessoryConfiguration.server, &accessory);
#if (HAVE_FIRMWARE_UPDATE == 1)
    FirmwareUpdateStart(accessoryConfiguration.server, &accessory);
#endif // HAVE_FIRMWARE_UPDATE
}

//----------------------------------------------------------------------------------------------------------------------

void AppHandleAccessoryServerStateUpdate(HAPAccessoryServer* server HAP_UNUSED, void* _Nullable context HAP_UNUSED) {
}

void AppHandleFactoryReset(HAPAccessoryServer* server HAP_UNUSED, void* _Nullable context HAP_UNUSED) {
    // Purge persistent state.
    HAPPlatformKeyValueStoreDomain domainsToPurge[] = { kSDKKeyValueStoreDomain_IPCameraOperatingMode,
                                                        kSDKKeyValueStoreDomain_IPCameraPairedOperatingMode,
                                                        kSDKKeyValueStoreDomain_IPCameraRecorderConfiguration };
    for (size_t i = 0; i < HAPArrayCount(domainsToPurge); i++) {
        HAPError err = HAPPlatformKeyValueStorePurgeDomain(accessoryConfiguration.keyValueStore, domainsToPurge[i]);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }
    }
}

void AppHandlePairingStateChange(
        HAPAccessoryServer* server,
        HAPPairingStateChange state,
        void* _Nullable context HAP_UNUSED) {
    switch (state) {
        case kHAPPairingStateChange_Unpaired: {
            // End any active streaming sessions before as we unpair the accessory
            CameraEndStreaming(HAPCameraAccessoryGetCamera(server));

            HAPPlatformKeyValueStoreDomain domainsToPurge[] = { kSDKKeyValueStoreDomain_IPCameraPairedOperatingMode,
                                                                kSDKKeyValueStoreDomain_IPCameraRecorderConfiguration };
            for (size_t i = 0; i < HAPArrayCount(domainsToPurge); i++) {
                HAPError err =
                        HAPPlatformKeyValueStorePurgeDomain(accessoryConfiguration.keyValueStore, domainsToPurge[i]);
                if (err) {
                    HAPAssert(err == kHAPError_Unknown);
                    HAPFatalError();
                }
            }
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
            DiagnosticsHandlePairingStateChange(
                    state, &accessoryConfiguration.state.diagnosticsSelectedModeState, &SaveAccessoryState);
#endif // (HAVE_DIAGNOSTICS_SERVICE == 1)

            // Reload platform configuration. This is needed for the case where the controller removed the pairing.
            SetupDefaultAccessoryState();
            UpdatePlatformVolume();
            HAPPlatformCameraReloadRecorderConfiguration(HAPCameraAccessoryGetCamera(server));
            break;
        }
        default: {
            break;
        }
    }
}

const HAPAccessory* AppGetAccessoryInfo(void) {
    return &accessory;
}

const HAPAccessory* _Nonnull const* _Nullable AppGetBridgeAccessoryInfo(void) {
    return NULL;
}

void AppInitialize(
        HAPAccessoryServerOptions* hapAccessoryServerOptions,
        HAPPlatform* hapPlatform,
        HAPAccessoryServerCallbacks* hapAccessoryServerCallbacks HAP_UNUSED) {
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
    InitializeDiagnostics(&accessoryConfiguration.state.diagnosticsSelectedModeState, &accessory, hapPlatform);
#endif // (HAVE_DIAGNOSTICS_SERVICE == 1)

    CameraRecorderInitialize(&appPlatform.recorder, hapAccessoryServerOptions, hapPlatform, false);
}

void AppDeinitialize(void) {
    CameraRecorderDeinitialize(&appPlatform.recorder);
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
    DeinitializeDiagnostics(&accessory);
#endif // (HAVE_DIAGNOSTICS_SERVICE == 1)
}