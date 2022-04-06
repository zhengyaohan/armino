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

// An accessory that represents an IP Camera Bridge.
//
// This camera bridge sample simulates two bridged cameras, physically using a camera directly
// attached to the Raspberry Pi on which the bridge sample is running. It can be used as a starting
// point for implementing a true camera bridge, where the cameras are separate from the bridge accessory.
// Typically, the cameras will use some proprietary protocol for communicating their audio/video stream
// data to the bridge accessory. For such a camera bridge, the PAL APIs HAPPlatformCameraInput,
// HAPPlatformMicrophone and HAPPlatformSpeaker are not relevant. Whether the higher-level PAL API
// HAPPlatformCamera can be reused as is, or has to be modified to some degree, depends on the division
// of processing between cameras and bridge.
//
// In order to use multiple cameras with one bridge, set the following constant in the ADK
// CameraBridge sample to the maximum number of cameras to be supported in parallel:
//
//  kAppState_NumCameras
//
// and provide a suitably sized
//
//  bridgedAccessories
//
// array variable.
//
// Note that the CameraBridge sample can simulate an arbitrary number of cameras on the Raspberry Pi
// reference platform in principle. However, at most two simultaneous camera input streams are feasible
// on this platform. Two cameras already require four input streams according to the specification.
// Nevertheless, the sample running on a Raspberry Pi works as long as only two streams are open at the
// same time, no matter how many cameras are simulated - even if the two streams come from different
// (simulated) cameras.
//
// This implementation is intended for POSIX platforms.
//
// To enable user interaction following POSIX signals or Buttons are used:
// - Button 1 or signal `SIGUSR1` to clear pairings.
// - Button 2 or signal `SIGUSR2` to trigger a factory reset.
// - Button 3 or Signal `SIGTERM` to trigger pairing mode.
// - Button 4 or Signal `SIGQUIT` unused.
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
//   5. The signal handlers.
//
//   6. The setup of the accessory HomeKit and the bridged accessories attribute database.
//
//   7. The initialization of the accessory state.
//
//   8. A callback that gets called when the server state is updated.

#include <inttypes.h>

#include "HAP.h"
#include "HAPAccessory+Camera.h"
#include "HAPPlatform+Init.h"
#include "HAPPlatformCamera+Init.h"
#include "HAPPlatformCameraInput+Init.h"
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

typedef struct {
    AppLEDIdentifier identifyPin;
} Device;

/**
 * Global accessory configuration.
 */
typedef struct {
    HAPAccessoryServer* server;
    HAPPlatformKeyValueStoreRef keyValueStore;

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
    } cameraState[kAppState_NumCameras];

    Device device;
    HAPPlatformTimerRef identifyTimer;
    bool restoreFactorySettingsRequested;
} AccessoryConfiguration;

static AccessoryConfiguration accessoryConfiguration;

/**
 * App platform objects.
 */
static struct { PlatformBridgeCamera camera; } appPlatform;

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

//----------------------------------------------------------------------------------------------------------------------

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
                "%s: Identifying Camera [%llu].",
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
        HAPPlatformMicrophoneSetMuted(&appPlatform.camera.microphone, value);
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
        HAPPlatformMicrophoneSetVolume(&appPlatform.camera.microphone, value);
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
        HAPPlatformSpeakerSetMuted(&appPlatform.camera.speaker, value);
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
        HAPPlatformSpeakerSetVolume(&appPlatform.camera.speaker, value);
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }
    return kHAPError_None;
}

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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if (HAP_APP_USES_HDS == 1)
#if (HAP_APP_USES_HDS_STREAM == 1)
static HAPStreamDataStreamProtocol streamDataStreamProtocol = {
    .base = &kHAPStreamDataStreamProtocol_Base,
    .numDataStreams = kApp_NumDataStreams,
    .applicationProtocols = (HAPStreamApplicationProtocol* const[]) { &streamProtocolUARP, NULL }
};
#endif // HAP_APP_USES_HDS_STREAM

static HAPDataStreamDescriptor dataStreamDescriptors[kApp_NumDataStreams];
static HAPDataStreamDispatcher dataStreamDispatcher;
const HAPDataStreamDispatcherStorage dataStreamDispatcherStorage = {
    .numDataStreams = kApp_NumDataStreams,
    .dataStreamDescriptors = dataStreamDescriptors,
    .dataStreamProtocols =
            (HAPDataStreamProtocol* const[]) {
#if (HAP_APP_USES_HDS_STREAM == 1)
                    &streamDataStreamProtocol,
#endif
                    NULL,
            },
};
#endif // HAP_APP_USES_HDS

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------------------------------------------------

/**
 * HomeKit accessory that provides the streaming services and the Microphone and Speaker services.
 */
static HAPAccessory accessory = { .aid = kAppAID_Bridge,
                                  .category = kHAPAccessoryCategory_Bridges,
                                  .name = "Acme IP Camera Bridge",
                                  .productData = "03d8a775e3644573",
                                  .manufacturer = "Acme",
                                  .model = "CameraBridge1,1",
                                  .serialNumber = "099DB48E9E28",
                                  .firmwareVersion = "1",
                                  .hardwareVersion = "1",
                                  .services = (const HAPService* const[]) { &accessoryInformationService,
                                                                            &hapProtocolInformationService,
                                                                            &pairingService,
#if (HAVE_ACCESSORY_REACHABILITY == 1)
                                                                            &accessoryRuntimeInformationService,
#endif
#if (HAP_APP_USES_HDS == 1)
                                                                            &dataStreamTransportManagementService,
#endif
#if (HAVE_FIRMWARE_UPDATE == 1)
                                                                            &firmwareUpdateService,
#endif
#if (HAP_TESTING == 1)
                                                                            &debugService,
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
                                         .name = "IP Camera 1",
                                         .manufacturer = "Acme",
                                         .model = "IPCamera1,1",
                                         .serialNumber = "099DB48E9E29A",
                                         .firmwareVersion = "1",
                                         .hardwareVersion = "1",
                                         .services = (const HAPService* const[]) { &accessoryInformationServiceBridged,
                                                                                   &cameraRTPStreamManagement0Service,
                                                                                   &cameraRTPStreamManagement1Service,
                                                                                   &microphoneService,
                                                                                   &speakerService,
#if (HAVE_ACCESSORY_REACHABILITY == 1)
                                                                                   &accessoryRuntimeInformationService,
#endif
                                                                                   NULL },
                                         .cameraStreamConfigurations =
                                                 (const HAPCameraStreamSupportedConfigurations* const[]) {
                                                         &supportedCameraStreamConfigurations0,
                                                         &supportedCameraStreamConfigurations1,
                                                         NULL },
                                         .callbacks = {
                                                 .identify = IdentifyAccessory,
                                         } };

static const HAPAccessory accessory2 = { .aid = kAppAid_BridgedAccessories_Start + 1,
                                         .category = kHAPAccessoryCategory_BridgedAccessory,
                                         .name = "IP Camera 2",
                                         .manufacturer = "Acme",
                                         .model = "IPCamera1,1",
                                         .serialNumber = "099DB48E9E29B",
                                         .firmwareVersion = "1",
                                         .hardwareVersion = "1",
                                         .services = (const HAPService* const[]) { &accessoryInformationServiceBridged,
                                                                                   &cameraRTPStreamManagement0Service,
                                                                                   &cameraRTPStreamManagement1Service,
                                                                                   &microphoneService,
                                                                                   &speakerService,
#if (HAVE_ACCESSORY_REACHABILITY == 1)
                                                                                   &accessoryRuntimeInformationService,
#endif
                                                                                   NULL },
                                         .cameraStreamConfigurations =
                                                 (const HAPCameraStreamSupportedConfigurations* const[]) {
                                                         &supportedCameraStreamConfigurations0,
                                                         &supportedCameraStreamConfigurations1,
                                                         NULL },
                                         .callbacks = {
                                                 .identify = IdentifyAccessory,
                                         } };

/**
 * Array of bridged accessories exposed by the bridge accessory. NULL-terminated.
 */
static const HAPAccessory* _Nonnull const* _Nullable bridgedAccessories =
        (const HAPAccessory* const[]) { &accessory1, &accessory2, NULL };
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

    ConfigureIO();

    for (size_t i = 0; i < kAppState_NumCameras; i++) {
        LoadAccessoryState(i);

        // Set volume and mute.
        HAPPlatformMicrophone* microphone = &appPlatform.camera.microphone;
        HAPPlatformMicrophoneSetVolume(microphone, accessoryConfiguration.cameraState[i].microphone.volume);
        HAPPlatformMicrophoneSetMuted(microphone, accessoryConfiguration.cameraState[i].microphone.mute);
        HAPPlatformSpeaker* speaker = &appPlatform.camera.speaker;
        HAPPlatformSpeakerSetVolume(speaker, accessoryConfiguration.cameraState[i].speaker.volume);
        HAPPlatformSpeakerSetMuted(speaker, accessoryConfiguration.cameraState[i].speaker.mute);
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
}

void AppRelease(HAPAccessoryServer* _Nonnull server HAP_UNUSED, void* _Nullable context HAP_UNUSED) {
    UnconfigureIO();
}

void AppAccessoryServerStart(void) {
    HAPAccessoryServerStartCameraBridge(
            accessoryConfiguration.server,
            &accessory,
            bridgedAccessories,
            appPlatform.camera.cameras,
            /* configurationChanged: */ false);
#if (HAVE_FIRMWARE_UPDATE == 1)
    FirmwareUpdateStart(accessoryConfiguration.server, &accessory);
#endif // HAVE_FIRMWARE_UPDATE
}

//----------------------------------------------------------------------------------------------------------------------

void AppHandleAccessoryServerStateUpdate(HAPAccessoryServer* server HAP_UNUSED, void* _Nullable context HAP_UNUSED) {
}

void AppHandleFactoryReset(HAPAccessoryServer* server HAP_UNUSED, void* _Nullable context HAP_UNUSED) {
}

void AppHandlePairingStateChange(
        HAPAccessoryServer* server HAP_UNUSED,
        HAPPairingStateChange state HAP_UNUSED,
        void* _Nullable context HAP_UNUSED) {
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
    BridgeCameraInitialize(&appPlatform.camera, hapAccessoryServerOptions, hapPlatform);
}

void AppDeinitialize(void) {
    BridgeCameraDeinitialize(&appPlatform.camera);
}
