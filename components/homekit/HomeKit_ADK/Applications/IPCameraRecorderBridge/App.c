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

// An accessory that represents an IP Camera Recorder Bridge.
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
//
// The code consists of multiple parts:
//
//   1. The definition of the accessory configuration, its internal, and bridged accessories state.
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
//   8. The setup of the accessory HomeKit and the bridged accessories attribute database.
//
//   9. The initialization of the accessory state.
//
//  10. A callback that gets called when the server state is updated.

#include <inttypes.h>
#if (HAP_TESTING == 1)
#include <string.h>
#endif

#include "HAP.h"
#include "HAPPlatform+Init.h"
#include "HAPPlatformCamera+Configuration.h"
#include "HAPPlatformCameraOperatingMode.h"
#include "HAPPlatformCameraRecorder+Init.h"
#include "HAPPlatformKeyValueStore+SDKDomains.h"

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
#if (HAVE_ACCESSORY_REACHABILITY == 1)
#include "AccessoryRuntimeInformationServiceDB.h"
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * Keys used in the key value store to store the configuration state of the first camera.
 * Subsequent keys are used for subsequent cameras.
 *
 * Purged: On factory reset.
 */
#define kAppKeyValueStoreKey_Configuration_State ((HAPPlatformKeyValueStoreKey) 0x00)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * HomeKit Accessory instance ID of bridge.
 */
#define kAppAID_Bridge ((size_t) 1)

/**
 * HomeKit Accessory instance ID of first bridged accessory.
 */
#define kAppAid_BridgedAccessories_Start ((size_t) 2)

#if (HAP_TESTING == 1)
/**
 * Memory for camera/microphone media path. This is used in testing when the user specifies a particular video
 * path to use for a command(such as triggerMotion -m <file_path>).
 */
char rootMediaPath[PATH_MAX];
#endif
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
    MotionSensor motionSensor[kAppState_NumCameras];
    struct {
        struct {
            bool mute;
            uint8_t volume;
        } microphone;
        struct {
            bool mute;
            uint8_t volume;
        } speaker;
    } cameraState[kAppState_NumCameras];
    int maxVideoRecordingResolution[kAppState_NumCameras];
    Device device;
    HAPPlatformTimerRef identifyTimer;
} AccessoryConfiguration;

static AccessoryConfiguration accessoryConfiguration;
static struct { PlatformBridgeCameraRecorder recorder; } appPlatform;

static const HAPAccessory* _Nonnull const* _Nullable bridgedAccessories;

HAP_STATIC_ASSERT(
        (kAppKeyValueStoreKey_Configuration_State + kAppState_NumCameras) < UINT8_MAX,
        KeyValueStore_NumCameraOverflow); // UINT8_MAX == HAPPlatformKeyValueStoreKey max value

/**
 * Obtain the camera index based on the HAP Accessory ID used.
 */
static size_t GetCameraIndex(const HAPAccessory* accessoryBridged) {
    HAPPrecondition(accessoryBridged);
    HAPPrecondition(accessoryBridged->aid >= kAppAid_BridgedAccessories_Start);
    HAPPrecondition((size_t)(accessoryBridged->aid - kAppAid_BridgedAccessories_Start) < kAppState_NumCameras);
    return accessoryBridged->aid - kAppAid_BridgedAccessories_Start;
}

/**
 * Obtain the camera index from a camera object.
 */
static size_t GetCameraIndexFromCamera(const HAPPlatformCamera* camera) {
    HAPPrecondition(camera);

    for (size_t i = 0; i < kAppState_NumCameras; i++) {
        if (appPlatform.recorder.cameras[i] == camera) {
            return i;
        }
    }
    HAPAssert(false);
    return 0;
}

//----------------------------------------------------------------------------------------------------------------------

static void HandleMotionSensorStatusActiveChanged(HAPPlatformCameraRef camera);
static void HandleVisualIndicationChanged(HAPPlatformCameraRef camera);
static void UpdatePlatformVolume(size_t cameraIndex);

/**
 * Initialize the default accessory state.
 */
static void SetupDefaultAccessoryState(size_t cameraIndex) {
    accessoryConfiguration.cameraState[cameraIndex].microphone.mute = false;
    accessoryConfiguration.cameraState[cameraIndex].microphone.volume = 100;

    accessoryConfiguration.cameraState[cameraIndex].speaker.mute = false;
    accessoryConfiguration.cameraState[cameraIndex].speaker.volume = 100;
}

/**
 * Update PAL with volume/mute value for microphone and speaker
 */
static void UpdatePlatformVolume(size_t cameraIndex) {
    HAPPrecondition(accessoryConfiguration.server);

    // Set volume and mute.
    HAPPlatformMicrophone* microphone = &appPlatform.recorder.microphone;
    HAPPlatformSpeaker* speaker = &appPlatform.recorder.speaker;
    HAPPlatformMicrophoneSetVolume(microphone, accessoryConfiguration.cameraState[cameraIndex].microphone.volume);
    HAPPlatformMicrophoneSetMuted(microphone, accessoryConfiguration.cameraState[cameraIndex].microphone.mute);
    HAPPlatformSpeakerSetVolume(speaker, accessoryConfiguration.cameraState[cameraIndex].speaker.volume);
    HAPPlatformSpeakerSetMuted(speaker, accessoryConfiguration.cameraState[cameraIndex].speaker.mute);
}

/**
 * Load the accessory state from persistent memory.
 */
static void LoadAccessoryState(size_t cameraIndex) {
    HAPPrecondition(accessoryConfiguration.keyValueStore);

    HAPError err;

    // Load persistent state if available.
    bool found;
    size_t numBytes;

    err = HAPPlatformKeyValueStoreGet(
            accessoryConfiguration.keyValueStore,
            kAppKeyValueStoreDomain_Configuration,
            (HAPPlatformKeyValueStoreKey)(kAppKeyValueStoreKey_Configuration_State + cameraIndex),
            &accessoryConfiguration.cameraState[cameraIndex],
            sizeof accessoryConfiguration.cameraState[cameraIndex],
            &numBytes,
            &found);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPFatalError();
    }
    if (!found || numBytes != sizeof accessoryConfiguration.cameraState[cameraIndex]) {
        if (found) {
            HAPLog(&kHAPLog_Default, "Unexpected app state found in key-value store. Resetting to default.");
        }
        SetupDefaultAccessoryState(cameraIndex);
    }

    accessoryConfiguration.motionSensor[cameraIndex].motionDetected = false;
    accessoryConfiguration.motionSensor[cameraIndex].statusActive = true;
    accessoryConfiguration.maxVideoRecordingResolution[cameraIndex] = 1080;
    HandleMotionSensorStatusActiveChanged(appPlatform.recorder.cameras[cameraIndex]);
    HandleVisualIndicationChanged(appPlatform.recorder.cameras[cameraIndex]);
}

/**
 * Save the accessory state to persistent memory.
 */
static void SaveAccessoryState(size_t cameraIndex) {
    HAPPrecondition(accessoryConfiguration.keyValueStore);

    HAPError err;
    err = HAPPlatformKeyValueStoreSet(
            accessoryConfiguration.keyValueStore,
            kAppKeyValueStoreDomain_Configuration,
            (HAPPlatformKeyValueStoreKey)(kAppKeyValueStoreKey_Configuration_State + cameraIndex),
            &accessoryConfiguration.cameraState[cameraIndex],
            sizeof accessoryConfiguration.cameraState[cameraIndex]);
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
    if (request->accessory->aid == 1) {
        HAPLogDebug(&kHAPLog_Default, "%s: Identifying Bridge.", __func__);
    } else {
        HAPLogDebug(
                &kHAPLog_Default,
                "%s: Identifying IPCameraRecorder [%llu].",
                __func__,
                (unsigned long long) request->accessory->aid);
    }
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
        const HAPBoolCharacteristicReadRequest* request,
        bool* value,
        void* _Nullable context HAP_UNUSED) {
    uint64_t id = request->accessory->aid - 1;
    size_t cameraIndex = GetCameraIndex(request->accessory);
    *value = accessoryConfiguration.cameraState[cameraIndex].microphone.mute;
    HAPLogInfo(&kHAPLog_Default, "%s: [%" PRIu64 "] %s", __func__, id, *value ? "true" : "false");
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
    uint64_t id = request->accessory->aid - 1;
    HAPLogInfo(&kHAPLog_Default, "%s: [%" PRIu64 "] %s", __func__, id, value ? "true" : "false");
    size_t cameraIndex = GetCameraIndex(request->accessory);
    if (accessoryConfiguration.cameraState[cameraIndex].microphone.mute != value) {
        accessoryConfiguration.cameraState[cameraIndex].microphone.mute = value;
        SaveAccessoryState(cameraIndex);
        HAPPlatformMicrophoneSetMuted(&appPlatform.recorder.microphone, value);
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
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    uint64_t id = request->accessory->aid - 1;
    size_t cameraIndex = GetCameraIndex(request->accessory);
    *value = accessoryConfiguration.cameraState[cameraIndex].microphone.volume;
    HAPLogInfo(&kHAPLog_Default, "%s: [%" PRIu64 "] %d", __func__, id, (int) *value);
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
    uint64_t id = request->accessory->aid - 1;
    HAPLogInfo(&kHAPLog_Default, "%s: [%" PRIu64 "] %d", __func__, id, (int) value);
    size_t cameraIndex = GetCameraIndex(request->accessory);
    if (accessoryConfiguration.cameraState[cameraIndex].microphone.volume != value) {
        accessoryConfiguration.cameraState[cameraIndex].microphone.volume = value;
        SaveAccessoryState(cameraIndex);
        HAPPlatformMicrophoneSetVolume(&appPlatform.recorder.microphone, value);
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
        const HAPBoolCharacteristicReadRequest* request,
        bool* value,
        void* _Nullable context HAP_UNUSED) {
    uint64_t id = request->accessory->aid - 1;
    size_t cameraIndex = GetCameraIndex(request->accessory);
    *value = accessoryConfiguration.cameraState[cameraIndex].speaker.mute;
    HAPLogInfo(&kHAPLog_Default, "%s: [%" PRIu64 "] %s", __func__, id, *value ? "true" : "false");
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
    uint64_t id = request->accessory->aid - 1;
    HAPLogInfo(&kHAPLog_Default, "%s: [%" PRIu64 "] %s", __func__, id, value ? "true" : "false");
    size_t cameraIndex = GetCameraIndex(request->accessory);
    if (accessoryConfiguration.cameraState[cameraIndex].speaker.mute != value) {
        accessoryConfiguration.cameraState[cameraIndex].speaker.mute = value;
        SaveAccessoryState(cameraIndex);
        HAPPlatformSpeakerSetMuted(&appPlatform.recorder.speaker, value);
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
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    uint64_t id = request->accessory->aid - 1;
    size_t cameraIndex = GetCameraIndex(request->accessory);
    *value = accessoryConfiguration.cameraState[cameraIndex].speaker.volume;
    HAPLogInfo(&kHAPLog_Default, "%s: [%" PRIu64 "] %d", __func__, id, (int) *value);
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
    uint64_t id = request->accessory->aid - 1;
    HAPLogInfo(&kHAPLog_Default, "%s: [%" PRIu64 "] %d", __func__, id, (int) value);
    size_t cameraIndex = GetCameraIndex(request->accessory);
    if (accessoryConfiguration.cameraState[cameraIndex].speaker.volume != value) {
        accessoryConfiguration.cameraState[cameraIndex].speaker.volume = value;
        SaveAccessoryState(cameraIndex);
        HAPPlatformSpeakerSetVolume(&appPlatform.recorder.speaker, value);
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }
    return kHAPError_None;
}

/**
 * Handle read request to the 'Motion Detected' characteristic of the Motion Sensor service.
 */
HAPError HandleMotionSensorMotionDetectedRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPBoolCharacteristicReadRequest* request,
        bool* value,
        void* _Nullable context HAP_UNUSED) {
    uint64_t id = request->accessory->aid - 1;
    size_t cameraIndex = GetCameraIndex(request->accessory);
    *value = accessoryConfiguration.motionSensor[cameraIndex].motionDetected;
    HAPLogInfo(&kHAPLog_Default, "%s: [%" PRIu64 "] %s", __func__, id, *value ? "true" : "false");
    return kHAPError_None;
}

/**
 * Handle read request to the 'Status Active' characteristic of the Motion Sensor service.
 */
HAPError HandleMotionSensorStatusActiveRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPBoolCharacteristicReadRequest* request,
        bool* value,
        void* _Nullable context HAP_UNUSED) {
    uint64_t id = request->accessory->aid - 1;
    size_t cameraIndex = GetCameraIndex(request->accessory);
    *value = accessoryConfiguration.motionSensor[cameraIndex].statusActive;
    HAPLogInfo(&kHAPLog_Default, "%s: [%" PRIu64 "] %s", __func__, id, *value ? "true" : "false");
    return kHAPError_None;
}

/**
 * Toggle the motion sensor state.
 */
void ToggleMotionSensorState(int cameraIndex) {
    if (!accessoryConfiguration.motionSensor[cameraIndex].statusActive) {
        return;
    }

    accessoryConfiguration.motionSensor[cameraIndex].motionDetected =
            !accessoryConfiguration.motionSensor[cameraIndex].motionDetected;

    HAPLogInfo(
            &kHAPLog_Default,
            "%s: %s",
            __func__,
            accessoryConfiguration.motionSensor[cameraIndex].motionDetected ? "true" : "false");

    if (accessoryConfiguration.motionSensor[cameraIndex].motionDetected) {
        HAPPlatformCameraTriggerRecording(HAPNonnull(appPlatform.recorder.cameras[cameraIndex]));
    }

    HAPAccessoryServerRaiseEvent(
            accessoryConfiguration.server,
            &motionSensorMotionDetectedCharacteristic,
            &motionSensorService,
            bridgedAccessories[cameraIndex]);
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

    int cameraIndex = GetCameraIndexFromCamera(camera);

    if (camera->recorder->videoEOF && accessoryConfiguration.motionSensor[cameraIndex].statusActive &&
        accessoryConfiguration.motionSensor[cameraIndex].motionDetected) {
        HAPLogInfo(&kHAPLog_Default, "%s: motionDetected set to false", __func__);
        accessoryConfiguration.motionSensor[cameraIndex].motionDetected = false;
        // Reset flag, since we are handing the videoEOF here.
        camera->recorder->videoEOF = false;
        HAPAccessoryServerRaiseEvent(
                accessoryConfiguration.server,
                &motionSensorMotionDetectedCharacteristic,
                &motionSensorService,
                bridgedAccessories[cameraIndex]);
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

    size_t cameraIndex = GetCameraIndexFromCamera(camera);

    if (active != accessoryConfiguration.motionSensor[cameraIndex].statusActive) {
        accessoryConfiguration.motionSensor[cameraIndex].statusActive = active;

        SaveAccessoryState(cameraIndex);

        HAPAccessoryServerRaiseEvent(
                accessoryConfiguration.server,
                &motionSensorStatusActiveCharacteristic,
                &motionSensorService,
                bridgedAccessories[cameraIndex]);
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
    HAPPrecondition(completionHandler);

    size_t cameraIndex = GetCameraIndex(accessory_);
    const HAPCameraRecordingSupportedConfiguration* config;

    if (accessoryConfiguration.maxVideoRecordingResolution[cameraIndex] >= 1080) {
        config = &supportedCameraRecordingConfiguration;
    } else if (accessoryConfiguration.maxVideoRecordingResolution[cameraIndex] >= 720) {
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

    size_t cameraIndex = GetCameraIndexFromCamera(camera);

    if (resolution != accessoryConfiguration.maxVideoRecordingResolution[cameraIndex]) {
        accessoryConfiguration.maxVideoRecordingResolution[cameraIndex] = resolution;

        HAPCameraAccessoryHandleSupportedRecordingConfigurationChange(
                accessoryConfiguration.server,
                &cameraEventRecordingManagementService,
                bridgedAccessories[cameraIndex],
                kHAPCameraSupportedRecordingConfigurationChange_Video);
    }
    return kHAPError_None;
}

/**
 * Simulate Manual Camera Disable.
 */
void SimulateSetCameraManuallyDisable(int cameraIndex, bool value) {
    HAPError err = kHAPError_None;
    bool wasDisabled = false;

    err = HAPPlatformCameraIsManuallyDisabled(appPlatform.recorder.cameras[cameraIndex], &wasDisabled);
    if (err) {
        HAPLogError(&kHAPLog_Default, "HAPPlatformCameraIsManuallyDisabled failed: %u.", err);
        return;
    }
    if (value == wasDisabled) {
        return;
    }

    err = HAPPlatformCameraSetManuallyDisabled(appPlatform.recorder.cameras[cameraIndex], value);
    if (err) {
        HAPLogError(&kHAPLog_Default, "HAPPlatformCameraSetManuallyDisabled failed: %u.", err);
        return;
    }

    HAPAccessoryServerRaiseEvent(
            accessoryConfiguration.server,
            &cameraOperatingModeManuallyDisabledCharacteristic,
            &cameraOperatingModeService,
            bridgedAccessories[cameraIndex]);

    SimulateSetThirdPartyActive(cameraIndex, !value);
}

/**
 * Simulate Third Party Active setting.
 */
void SimulateSetThirdPartyActive(int cameraIndex, bool value) {
    HAPError err = kHAPError_None;
    bool wasActive = true;

    err = HAPPlatformCameraIsThirdPartyCameraActive(appPlatform.recorder.cameras[cameraIndex], &wasActive);
    if (err) {
        HAPLogError(&kHAPLog_Default, "HAPPlatformCameraIsThirdPartyCameraActive failed: %u.", err);
        return;
    }

    if (value == wasActive) {
        return;
    }

    err = HAPPlatformCameraSetThirdPartyCameraActive(appPlatform.recorder.cameras[cameraIndex], value);
    if (err) {
        HAPLogError(&kHAPLog_Default, "HAPPlatformCameraSetThirdPartyCameraActive failed: %u.", err);
        return;
    }

    HAPAccessoryServerRaiseEvent(
            accessoryConfiguration.server,
            &cameraOperatingModeThirdPartyCameraActiveCharacteristic,
            &cameraOperatingModeService,
            bridgedAccessories[cameraIndex]);
}

/**
 * Change supported recording configuration.
 *
 * Cycles through the following configurations:
 * - high resolution   (up to 1080p)
 * - low resolution    (up to 720p)
 * - lowest resolution (up to 480p)
 */
void ChangeSupportedRecordingConfig(int cameraIndex) {
    if (accessoryConfiguration.maxVideoRecordingResolution[cameraIndex] > 720) {
        accessoryConfiguration.maxVideoRecordingResolution[cameraIndex] = 720;
    } else if (accessoryConfiguration.maxVideoRecordingResolution[cameraIndex] > 480) {
        accessoryConfiguration.maxVideoRecordingResolution[cameraIndex] = 480;
    } else {
        accessoryConfiguration.maxVideoRecordingResolution[cameraIndex] = 1080;
    }

    HAPCameraAccessoryHandleSupportedRecordingConfigurationChange(
            accessoryConfiguration.server,
            &cameraEventRecordingManagementService,
            bridgedAccessories[cameraIndex],
            kHAPCameraSupportedRecordingConfigurationChange_Video);
}

#if (HAP_TESTING == 1)
#ifdef DARWIN
/**
 * Return recorder associated with specified accessory camera.
 *
 * @param cameraIndex                           Index into accessory's array of cameras.
 *
 * @return HAPPlatformCameraRecorderRef         Accessory's recorder
 */
HAPPlatformCameraRecorderRef GetAccessoryCameraRecorder(int cameraIndex) {
    HAPPlatformCameraRef camera = appPlatform.recorder.cameras[cameraIndex];
    HAPAssert(camera);
    HAPPlatformCameraRecorderRef recorder = camera->recorder;
    HAPAssert(recorder);
    return recorder;
}

/**
 * Update accessory's root media path, used for specified camera and microphone.
 *
 * @param newMediaPath         Directory path for camera/microphone files.
 * @param cameraIndex          Index into accessory's array of cameras.
 *
 * @return HAPError            kHAPError_None if success, kHAPError_InvalidData if path is too long.
 */
HAPError ChangeMediaPathSetting(const char* newMediaPath, int cameraIndex) {
    HAPPrecondition(newMediaPath);

    HAPRawBufferZero(&rootMediaPath, sizeof(rootMediaPath));
    if (PATH_MAX < strlen(newMediaPath) + 1) {
        HAPLogError(
                &kHAPLog_Default,
                "Specified media path %lu is longer than available buffer %d.",
                strlen(newMediaPath) + 1,
                PATH_MAX);
        return kHAPError_InvalidData;
    }
    HAPRawBufferCopyBytes(&rootMediaPath, newMediaPath, strlen(newMediaPath) + 1);
    appPlatform.recorder.cameras[cameraIndex]->cameraInput->mediaPath = (char*) &rootMediaPath;
    appPlatform.recorder.cameras[cameraIndex]->microphone->mediaPath = (char*) &rootMediaPath;
    return kHAPError_None;
}
#endif
#endif

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
      .handleStreamAvailable = HAPPlatformCameraRecorderHandleDataSendStreamAvailable }
};
static HAPDataSendDataStreamProtocol dataSendDataStreamProtocol = {
    .base = &kHAPDataSendDataStreamProtocol_Base,
    .storage = { .numDataStreams = kApp_NumDataStreams,
                 .protocolContexts = dataSendDataStreamProtocolContexts,
                 .listeners = dataSendDataStreamProtocolListeners },
    .callbacks = { .numStreamAvailableCallbacks = 1,
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
 * HomeKit accessory that provides the streaming services and the Microphone and Speaker services.
 */
static HAPAccessory accessory = { .aid = kAppAID_Bridge,
                                  .category = kHAPAccessoryCategory_Bridges,
                                  .name = "Acme IP Camera Recorder Bridge",
                                  .productData = "03d8a775e3644573",
                                  .manufacturer = "Acme",
                                  .model = "RecorderBridge1,1",
                                  .serialNumber = "099DB48E9E28",
                                  .firmwareVersion = "1",
                                  .hardwareVersion = "1",
                                  .services = (const HAPService* const[]) { &accessoryInformationService,
                                                                            &hapProtocolInformationService,
                                                                            &pairingService,
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
                                                                            NULL },
#if (HAP_APP_USES_HDS == 1)
                                  .dataStream.delegate = { .callbacks = &kHAPDataStreamDispatcher_DataStreamCallbacks,
                                                           .context = &dataStreamDispatcher },
#endif
                                  .cameraStreamConfigurations = NULL,
                                  .callbacks = {
                                          .identify = IdentifyAccessory,
#if (HAVE_FIRMWARE_UPDATE == 1)
                                          .firmwareUpdate = { .getAccessoryState = FirmwareUpdateGetAccessoryState },
#endif
                                  } };

static const HAPAccessory accessory1 = { .aid = kAppAid_BridgedAccessories_Start,
                                         .category = kHAPAccessoryCategory_BridgedAccessory,
                                         .name = "Recorder 1",
                                         .manufacturer = "Acme",
                                         .model = "Recorder1,1",
                                         .serialNumber = "099DB48E9E29A",
                                         .firmwareVersion = "1",
                                         .hardwareVersion = "1",
                                         .services =
                                                 (const HAPService* const[]) {
                                                         &accessoryInformationServiceBridged,
                                                         &cameraRTPStreamManagement0Service,
                                                         &cameraRTPStreamManagement1Service,
                                                         &cameraEventRecordingManagementService,
                                                         &cameraOperatingModeService,
                                                         &microphoneService,
                                                         &speakerService,
                                                         &motionSensorService,
#if (HAP_APP_USES_HDS == 1)
                                                         &dataStreamTransportManagementService,
#endif
#if (HAVE_ACCESSORY_REACHABILITY == 1)
                                                         &accessoryRuntimeInformationService,
#endif
                                                         NULL,
                                                 },
#if (HAP_APP_USES_HDS == 1)
                                         .dataStream.delegate = { .callbacks =
                                                                          &kHAPDataStreamDispatcher_DataStreamCallbacks,
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
                                         } };

static const HAPAccessory accessory2 = { .aid = kAppAid_BridgedAccessories_Start + 1,
                                         .category = kHAPAccessoryCategory_BridgedAccessory,
                                         .name = "Recorder 2",
                                         .manufacturer = "Acme",
                                         .model = "Recorder1,1",
                                         .serialNumber = "099DB48E9E29B",
                                         .firmwareVersion = "1",
                                         .hardwareVersion = "1",
                                         .services =
                                                 (const HAPService* const[]) {
                                                         &accessoryInformationServiceBridged,
                                                         &cameraRTPStreamManagement0Service,
                                                         &cameraRTPStreamManagement1Service,
                                                         &cameraEventRecordingManagementService,
                                                         &cameraOperatingModeService,
                                                         &microphoneService,
                                                         &speakerService,
                                                         &motionSensorService,
#if (HAP_APP_USES_HDS == 1)
                                                         &dataStreamTransportManagementService,
#endif
#if (HAVE_ACCESSORY_REACHABILITY == 1)
                                                         &accessoryRuntimeInformationService,
#endif
                                                         NULL,
                                                 },
#if (HAP_APP_USES_HDS == 1)
                                         .dataStream.delegate = { .callbacks =
                                                                          &kHAPDataStreamDispatcher_DataStreamCallbacks,
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
                                         } };

/**
 * Array of bridged accessories exposed by the bridge accessory. NULL-terminated.
 */
static const HAPAccessory* _Nonnull const* _Nullable bridgedAccessories =
        (const HAPAccessory* const[]) { &accessory1, &accessory2, NULL };

//----------------------------------------------------------------------------------------------------------------------

void AppCreate(HAPAccessoryServer* server, HAPPlatformKeyValueStoreRef keyValueStore) {
    HAPPrecondition(server);
    HAPPrecondition(keyValueStore);

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    HAPRawBufferZero(&accessoryConfiguration, sizeof accessoryConfiguration);
    accessoryConfiguration.server = server;
    accessoryConfiguration.keyValueStore = keyValueStore;

    ConfigureIO();

    for (size_t i = 0; i < kAppState_NumCameras; i++) {
        // Load persistent state.
        // Load accessory state before calling HAPPlatformCameraReloadRecorderConfiguration, which
        // can trigger a SaveAccessoryState().
        LoadAccessoryState(i);

        // Install recorder delegate.
        HAPPlatformCameraRecorderDelegate delegate = { HandleResolutionChanged, HandleOperatingModeChanged };
        HAPPlatformCameraSetRecorderDelegate(HAPNonnull(appPlatform.recorder.cameras[i]), &delegate);

        HAPPlatformCameraReloadRecorderConfiguration(HAPNonnull(appPlatform.recorder.cameras[i]));

        UpdatePlatformVolume(i);
    }

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
#if (HAP_TESTING == 1)
    HAPRawBufferZero(&rootMediaPath, sizeof(rootMediaPath));
#endif
}

void AppRelease(HAPAccessoryServer* _Nonnull server HAP_UNUSED, void* _Nullable context HAP_UNUSED) {
    for (size_t i = 0; i < kAppState_NumCameras; i++) {
        HAPPlatformCameraSetRecorderDelegate(HAPNonnull(appPlatform.recorder.cameras[i]), NULL);
    }
    UnconfigureIO();
}

void AppAccessoryServerStart(void) {
    HAPAccessoryServerStartCameraBridge(
            accessoryConfiguration.server,
            &accessory,
            bridgedAccessories,
            appPlatform.recorder.cameras,
            /* configurationChanged: */ false);
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
        HAPAccessoryServer* server HAP_UNUSED,
        HAPPairingStateChange state,
        void* _Nullable context HAP_UNUSED) {
    switch (state) {
        case kHAPPairingStateChange_Unpaired: {
            // End any active streaming sessions before as we unpair the accessory
            for (size_t i = 0; i < kAppState_NumCameras; i++) {
                CameraEndStreaming(HAPNonnull(appPlatform.recorder.cameras[i]));
            }

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

            // Reload platform configuration. This is needed for the case where the controller removed the pairing.
            for (size_t i = 0; i < kAppState_NumCameras; i++) {
                SetupDefaultAccessoryState(i);
                UpdatePlatformVolume(i);
                HAPPlatformCameraReloadRecorderConfiguration(HAPNonnull(appPlatform.recorder.cameras[i]));
            }
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
    return bridgedAccessories;
}

void AppInitialize(
        HAPAccessoryServerOptions* hapAccessoryServerOptions,
        HAPPlatform* hapPlatform,
        HAPAccessoryServerCallbacks* hapAccessoryServerCallbacks HAP_UNUSED) {
    BridgeCameraRecorderInitialize(&appPlatform.recorder, hapAccessoryServerOptions, hapPlatform);
}

void AppDeinitialize(void) {
    BridgeCameraRecorderDeinitialize(&appPlatform.recorder);
}
