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

// An accessory that represents an IP Camera that supports event triggered recording.
//
// This implementation is intended for POSIX platforms.
//
// To enable user interaction, POSIX signals are used:
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
//   5. The camera event recording configuration.
//
//   6. The camera event recording data stream handling.
//
//   7. The signal handlers.
//
//   8. The setup of the accessory attribute database.
//
//   9. The initialization of the accessory state.
//
//  10. A callback that gets called when the server state is updated.
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
#if (HAP_APP_USES_HDS == 1)
#include "DataStreamTransportManagementServiceDB.h"
#endif
#if (HAVE_FIRMWARE_UPDATE == 1)
#include "FirmwareUpdate.h"
#include "FirmwareUpdateServiceDB.h"
#endif
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
#include "DiagnosticsService.h"
#include "DiagnosticsServiceDB.h"
#include "HAPDiagnostics.h"
#endif
#if (HAVE_ACCESSORY_REACHABILITY == 1)
#include "AccessoryRuntimeInformationServiceDB.h"
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

typedef struct {
    AppLEDIdentifier identifyPin;
} Device;

/**
 * Motion sensor state.
 */
typedef struct {
    bool motionDetected;
    bool statusActive;
} MotionSensor;

/**
 * Accessory configuration.
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
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
        uint32_t diagnosticsSelectedModeState;
#endif // (HAVE_DIAGNOSTICS_SERVICE == 1)
    } state;
    Device device;
    HAPPlatformTimerRef identifyTimer;
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

#ifdef DARWIN
// ADK Media related External API values
/**
 * Custom callbacks for dataSendStream, if requested
 */
static HAPDataSendDataStreamProtocolStreamCallbacks dataSendStreamCallbacks;
static bool isDataStreamAvailable = false;
static bool shouldSetSideloadedDataStream = false;
/**
 * If true, will suppress camera and microphone from starting
 */
static bool disableMedia = false;
static void (*isDataStreamAvailableObserver)(bool) = NULL;
static void (*motionStateObserver)(bool) = NULL;
static void setIsDataStreamAvailable(bool value);

static inline void notifyMotionStateObserver() {
    if (motionStateObserver) {
        motionStateObserver(accessoryConfiguration.motionSensor.motionDetected);
    }
}
#endif

static inline void setMotionDetected(bool value) {
#ifdef DARWIN
    // If on Darwin, we keep track of the last value in order to deliver a possible change to an observer
    bool priorValue = accessoryConfiguration.motionSensor.motionDetected;
#endif
    accessoryConfiguration.motionSensor.motionDetected = value;

    if (accessoryConfiguration.motionSensor.motionDetected) {
        HAPPlatformCameraTriggerRecording(HAPCameraAccessoryGetCamera(accessoryConfiguration.server));
    }

    HAPAccessoryServerRaiseEvent(
            accessoryConfiguration.server, &motionSensorMotionDetectedCharacteristic, &motionSensorService, &accessory);

#ifdef DARWIN
    if (priorValue != value) {
        notifyMotionStateObserver();
    }
#endif
}

//----------------------------------------------------------------------------------------------------------------------

static void HandleMotionSensorStatusActiveChanged(HAPPlatformCameraRef camera);
static void HandleVisualIndicationChanged(HAPPlatformCameraRef camera);
static void UpdatePlatformVolume(void);

/**
 * Initialize the default accessory state.
 */
static void SetupDefaultAccessoryState(void) {
    accessoryConfiguration.state.microphone.mute = false;
    accessoryConfiguration.state.microphone.volume = 100;

    accessoryConfiguration.state.speaker.mute = false;
    accessoryConfiguration.state.speaker.volume = 100;
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
    accessoryConfiguration.state.diagnosticsSelectedModeState = kHAPCharacteristicValue_SelectedDiagnosticsModes_None;
#endif // (HAVE_DIAGNOSTICS_SERVICE == 1)
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
 * Load the accessory state from persistent memory.
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

    setMotionDetected(false);
    accessoryConfiguration.motionSensor.statusActive = true;
    accessoryConfiguration.maxVideoRecordingResolution = 1080;
    HandleMotionSensorStatusActiveChanged(HAPCameraAccessoryGetCamera(accessoryConfiguration.server));
    HandleVisualIndicationChanged(HAPCameraAccessoryGetCamera(accessoryConfiguration.server));
}

/**
 * Save the accessory state to persistent memory.
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

    setMotionDetected(!accessoryConfiguration.motionSensor.motionDetected);
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
    HAPPrecondition(camera);
    if (!camera->recorder) {
        return;
    }

    if (camera->recorder->videoEOF && accessoryConfiguration.motionSensor.statusActive &&
        accessoryConfiguration.motionSensor.motionDetected) {
        HAPLogInfo(&kHAPLog_Default, "%s: motionDetected set to false", __func__);
        setMotionDetected(false);
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

        SaveAccessoryState();

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

//----------------------------------------------------------------------------------------------------------------------

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

void ChangeSupportedRecordingConfig(void) {
#if (HAP_TESTING == 1)
    SimulateChangeSupportedRecordingConfig(&accessoryConfiguration.maxVideoRecordingResolution);
#endif // (HAP_TESTING == 1)
}

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

#ifdef DARWIN
static void AppHandleDataSendStreamClose(
        HAPAccessoryServer* _Nullable server,
        HAPDataStreamDispatcher* _Nullable dispatcher,
        HAPDataSendDataStreamProtocol* _Nullable dataStreamProtocol,
        const HAPDataStreamRequest* _Nullable request,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolStream* _Nullable dataSendStream,
        HAPError error,
        HAPDataSendDataStreamProtocolCloseReason closeReason,
        void* _Nullable context HAP_UNUSED) {

    setIsDataStreamAvailable(false);
    HAPPlatformSideloadHandleDataSendStreamClose(
            server, dispatcher, dataStreamProtocol, request, dataStream, dataSendStream, error, closeReason, context);
}
#endif

void HandleDataSendStreamAvailable(
        HAPAccessoryServer* _Nonnull server,
        HAPDataStreamDispatcher* _Nonnull dispatcher,
        HAPDataSendDataStreamProtocol* _Nonnull dataStreamProtocol,
        const HAPServiceRequest* _Nonnull request,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolType type,
        HAPDataSendDataStreamProtocolOpenMetadata* _Nullable metadata HAP_UNUSED,
        void* _Nullable inDataSendStreamCallbacks HAP_UNUSED,
        void* _Nullable context HAP_UNUSED) {

#ifdef DARWIN
    dataSendStreamCallbacks.handleOpen = HAPPlatformSideloadHandleDataSendStreamOpen;
    dataSendStreamCallbacks.handleClose = AppHandleDataSendStreamClose;

    HAPPlatformCameraRecorderHandleDataSendStreamAvailable(
            server,
            dispatcher,
            dataStreamProtocol,
            request,
            dataStream,
            type,
            metadata,
            (void*) &dataSendStreamCallbacks,
            context);
#else
    HAPPlatformCameraRecorderHandleDataSendStreamAvailable(
            server, dispatcher, dataStreamProtocol, request, dataStream, type, metadata, NULL, context);
#endif

#ifdef DARWIN
    if (shouldSetSideloadedDataStream) {
        HAPPlatformSetCameraSideloadDataStreamContext(
                &appPlatform.recorder.recorder, &appPlatform.recorder.recorder.dataStreamContext);
    }
    setIsDataStreamAvailable(true);
#endif
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

static HAPDataSendDataStreamProtocolContext dataSendDataStreamProtocolContexts[kApp_NumDataStreams];
static HAPDataSendDataStreamProtocolListener dataSendDataStreamProtocolListeners[kApp_NumDataStreams];
static HAPDataSendStreamProtocolStreamAvailableCallbacks dataSendDataStreamProtocolAvailableCallbacks[] = {
    { .type = kHAPDataSendDataStreamProtocolType_IPCamera_Recording,
      .handleStreamAvailable = HandleDataSendStreamAvailable },
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
    { .type = kHAPDataSendDataStreamProtocolType_Diagnostics_Snapshot,
      .handleStreamAvailable = HAPDiagnosticsHandleDataSendStreamAvailable }
#endif
};
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

/**
 * The HomeKit accessory.
 */
static HAPAccessory accessory = { .aid = 1,
                                  .category = kHAPAccessoryCategory_IPCameras,
                                  .name = "Acme IP Camera Event Recorder",
                                  .productData = "03d8a775e3644573",
                                  .manufacturer = "Acme",
                                  .model = "IPCamera1,1",
                                  .serialNumber = "099DB48E9E28",
                                  .firmwareVersion = "1",
                                  .hardwareVersion = "1",
                                  .services = (const HAPService* const[]) { &accessoryInformationService,
                                                                            &hapProtocolInformationService,
                                                                            &pairingService,
                                                                            &cameraRTPStreamManagement0Service,
                                                                            &cameraRTPStreamManagement1Service,
                                                                            &cameraEventRecordingManagementService,
                                                                            &cameraOperatingModeService,
                                                                            &microphoneService,
                                                                            &speakerService,
                                                                            &motionSensorService,
#if (HAP_TESTING == 1)
                                                                            &debugService,
#endif
#if (HAVE_ACCESSORY_REACHABILITY == 1)
                                                                            &accessoryRuntimeInformationService,
#endif
#if (HAP_APP_USES_HDS == 1)
                                                                            &dataStreamTransportManagementService,
#endif
#if (HAVE_FIRMWARE_UPDATE == 1)
                                                                            &firmwareUpdateService,
#endif
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
                                                                            &diagnosticsService,
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
                                          .camera = { .getSupportedRecordingConfiguration =
                                                              HandleGetSupportedRecordingConfiguration },
#if (HAVE_FIRMWARE_UPDATE == 1)
                                          .firmwareUpdate = { .getAccessoryState = FirmwareUpdateGetAccessoryState },
#endif
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
                                          .diagnosticsConfig = { .getDiagnosticsConfig =
                                                                         GetAccessoryDiagnosticsConfig },
#endif
                                  } };

#ifdef DARWIN
// ADK media API functions
void ADKMediaInitializeMp4Sideload(uint8_t* inMediaBuffer, size_t inMediaBufferSize) {
    HAPPlatformInitializeCameraSideload(inMediaBuffer, inMediaBufferSize);
    shouldSetSideloadedDataStream = true;
    if (isDataStreamAvailable) {
        HAPPlatformSetCameraSideloadDataStreamContext(
                &appPlatform.recorder.recorder, &appPlatform.recorder.recorder.dataStreamContext);
    }
}

static void setIsDataStreamAvailable(bool value) {
    bool priorValue = isDataStreamAvailable;
    isDataStreamAvailable = value;
    if (isDataStreamAvailableObserver) {
        if (priorValue != isDataStreamAvailable) {
            isDataStreamAvailableObserver(isDataStreamAvailable);
        }
    }
}

void ADKSetIsDataStreamAvailableObserver(void (*observer)(bool)) {
    isDataStreamAvailableObserver = observer;
}

bool ADKIsDataStreamAvailable() {
    return isDataStreamAvailable;
}

void ADKSetDisableMedia(bool value) {
    disableMedia = value;
}

void ADKStopSendingFragment(void) {
    HAPPlatformStopSendingFragment();
}

bool ADKIsFragmentInQueue(void) {
    return HAPPlatformIsFragmentInQueue();
}

void ADKSetIsFragmentInQueueObserver(void (*observer)(bool)) {
    HAPPlatformSetIsFragmentInQueueObserver(observer);
}

void ADKCloseDataStream(void) {
    HAPPlatformCloseDataStream();
}

bool ADKAddSideloadedMP4Fragment(
        uint8_t* fragmentBytes,
        size_t fragmentSize,
        int64_t fragmentNumber,
        bool endOfStream) {
    return HAPPlatformAddSideloadedMP4Fragment(fragmentBytes, fragmentSize, fragmentNumber, endOfStream);
}

bool ADKSendSideloadedFragment(void) {
    return HAPPlatformSendSideloadedFragment();
}

bool ADKIsFragmentSendInProgress(void) {
    return HAPPlatformIsFragmentSendInProgress();
}

void ADKSetIsFragmentSendInProgressObserver(void (*observer)(bool)) {
    HAPPlatformSetIsFragmentSendInProgressObserver(observer);
}

int64_t ADKGetCurrentFragmentNumber(void) {
    return HAPPlatformGetCurrentFragmentNumber();
}

void ADKSetCurrentFragmentNumberObserver(void (*observer)(int64_t)) {
    HAPPlatformSetCurrentFragmentNumberObserver(observer);
}

void ADKSetMotionState(bool value) {
    if (!accessoryConfiguration.motionSensor.statusActive) {
        HAPLogError(&kHAPLog_Default, "ERROR: Motion sensor is not yet active.");
        return;
    }
    setMotionDetected(value);
}

bool ADKGetMotionState(void) {
    if (!accessoryConfiguration.motionSensor.statusActive) {
        HAPLogError(&kHAPLog_Default, "ERROR: Motion sensor is not yet active.");
        return false;
    }
    return accessoryConfiguration.motionSensor.motionDetected;
}

void ADKSetMotionStateObserver(void (*observer)(bool)) {
    motionStateObserver = observer;
}

#endif

//----------------------------------------------------------------------------------------------------------------------

void AppCreate(HAPAccessoryServer* server, HAPPlatformKeyValueStoreRef keyValueStore) {
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
}

void AppRelease(HAPAccessoryServer* _Nonnull server HAP_UNUSED, void* _Nullable context HAP_UNUSED) {
    HAPPlatformCameraSetRecorderDelegate(HAPCameraAccessoryGetCamera(accessoryConfiguration.server), NULL);
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
    HAPLogInfo(&kHAPLog_Default, "Resetting accessory configuration.");

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
#ifdef DARWIN
    CameraRecorderInitialize(&appPlatform.recorder, hapAccessoryServerOptions, hapPlatform, disableMedia);
#else
    CameraRecorderInitialize(&appPlatform.recorder, hapAccessoryServerOptions, hapPlatform, false);
#endif
}

void AppDeinitialize(void) {
    CameraRecorderDeinitialize(&appPlatform.recorder);
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
    DeinitializeDiagnostics(&accessory);
#endif // (HAVE_DIAGNOSTICS_SERVICE == 1)
}
