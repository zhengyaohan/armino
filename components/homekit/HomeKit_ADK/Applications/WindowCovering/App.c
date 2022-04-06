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
// Copyright (C) 2020-2021 Apple Inc. All Rights Reserved.

// An example that implements the Window Covering app. It can serve as a basic implementation for
// any platform. The accessory logic implementation is reduced to internal state updates and log output.
// The example covers the Window Covering service..
//
// To enable user interaction following POSIX signals or Buttons are used:
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
//   3. The definitions for the HomeKit attribute database.
//
//   4. The callbacks that implement the actual behavior of the accessory, in this
//      case here they merely access the global accessory state variable and write
//      to the log to make the behavior easily observable.
//
//   5. The initialization of the accessory state.
//
//   6. Callbacks that notify the server in case their associated value has changed.

#include "HAP.h"

#include "AccessoryInformationServiceDB.h"
#include "App.h"
#include "AppBase.h"
#include "AppLED.h"
#if (HAP_TESTING == 1)
#include "AppUserInput.h"
#endif
#include "DB.h"
#if (HAP_APP_USES_HDS == 1)
#include "DataStreamTransportManagementServiceDB.h"
#endif
#if (HAVE_FIRMWARE_UPDATE == 1)
#include "FirmwareUpdate.h"
#include "FirmwareUpdateServiceDB.h"
#endif
#if (HAVE_THREAD == 1)
#include "ThreadManagementServiceDB.h"
#endif
#if (HAVE_ACCESSORY_REACHABILITY == 1)
#include "AccessoryRuntimeInformationServiceDB.h"
#endif
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
#include "DiagnosticsService.h"
#include "DiagnosticsServiceDB.h"
#include "HAPDiagnostics.h"
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * Key used in the key value store to store the configuration state.
 *
 * Purged: On factory reset.
 */
#define kAppKeyValueStoreKey_Configuration_State ((HAPPlatformKeyValueStoreKey) 0x00)

#define kCurrentPosition_SimulationInterval ((HAPTime)(0.5 * HAPSecond))

#define kCurrentPosition_Simulation_StepValue 5

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct {
    AppLEDIdentifier identifyPin;
} Device;

/**
 * Global accessory configuration.
 */
typedef struct {
    struct {
        uint8_t currentPosition;
        uint8_t targetPosition;
        uint8_t positionState;
        bool holdPosition;
        int32_t currentHorizontalTilt;
        int32_t targetHorizontalTilt;
        int32_t currentVerticalTilt;
        int32_t targetVerticalTilt;
        bool isObstructionDetected;
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
        uint32_t diagnosticsSelectedModeState;
#endif // (HAVE_DIAGNOSTICS_SERVICE == 1)
    } state;
    Device device;
    HAPPlatformTimerRef identifyTimer;
    HAPAccessoryServer* server;
    HAPPlatformKeyValueStoreRef keyValueStore;
    HAPPlatformTimerRef currentPositionSimulationTimer;

} AccessoryConfiguration;

static AccessoryConfiguration accessoryConfiguration;
static HAPAccessory accessory;

#if (HAVE_ACCESSORY_REACHABILITY == 1)
static HAPAccessoryReachabilityConfiguration accessoryReachabilityConfig = {
    .sleepIntervalInMs = kAccessorySleepInterval,
};
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if (HAP_APP_USES_HDS == 1)
#if (HAP_APP_USES_HDS_STREAM == 1)
static HAPStreamDataStreamProtocol streamDataStreamProtocol = {
    .base = &kHAPStreamDataStreamProtocol_Base,
    .numDataStreams = kApp_NumDataStreams,
    .applicationProtocols = (HAPStreamApplicationProtocol* const[]) { &streamProtocolUARP, NULL }
};
#endif // HAP_APP_USES_HDS_STREAM

#if (HAVE_DIAGNOSTICS_SERVICE == 1)
static HAPDataSendStreamProtocolStreamAvailableCallbacks dataSendDataStreamProtocolAvailableCallbacks[] = {
    { .type = kHAPDataSendDataStreamProtocolType_Diagnostics_Snapshot,
      .handleStreamAvailable = HAPDiagnosticsHandleDataSendStreamAvailable }
};
static HAPDataSendDataStreamProtocolContext dataSendDataStreamProtocolContexts[kApp_NumDataStreams];
static HAPDataSendDataStreamProtocolListener dataSendDataStreamProtocolListeners[kApp_NumDataStreams];
static HAPDataSendDataStreamProtocol
        dataSendDataStreamProtocol = { .base = &kHAPDataSendDataStreamProtocol_Base,
                                       .storage = { .numDataStreams = kApp_NumDataStreams,
                                                    .protocolContexts = dataSendDataStreamProtocolContexts,
                                                    .listeners = dataSendDataStreamProtocolListeners },
                                       .callbacks = {
                                               .numStreamAvailableCallbacks = 1,
                                               .streamAvailableCallbacks = dataSendDataStreamProtocolAvailableCallbacks,
                                       },
};
#endif // (HAVE_DIAGNOSTICS_SERVICE == 1)

static HAPDataStreamDescriptor dataStreamDescriptors[kApp_NumDataStreams];
static HAPDataStreamDispatcher dataStreamDispatcher;
const HAPDataStreamDispatcherStorage dataStreamDispatcherStorage = {
    .numDataStreams = kApp_NumDataStreams,
    .dataStreamDescriptors = dataStreamDescriptors,
    .dataStreamProtocols =
            (HAPDataStreamProtocol* const[]) {
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
                    &dataSendDataStreamProtocol,
#endif // (HAVE_DIAGNOSTICS_SERVICE == 1)
#if (HAP_APP_USES_HDS_STREAM == 1)
                    &streamDataStreamProtocol,
#endif
                    NULL,
            },
};
#endif // HAP_APP_USES_HDS

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------------------------------------
/**
 * Load the accessory state from persistent memory.
 */
static void LoadAccessoryState(void) {
    HAPPrecondition(accessoryConfiguration.keyValueStore);

    HAPError err;

    // Load persistent state if available
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
            HAPLogError(&kHAPLog_Default, "Unexpected app state found in key-value store. Resetting to default.");
        }
        HAPRawBufferZero(&accessoryConfiguration.state, sizeof accessoryConfiguration.state);
    }
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

static void HandleCurrentPositionSimulationTimerCallback(HAPPlatformTimerRef timer HAP_UNUSED, void* _Nullable context);

/**
 * Callback to enable the simulation via timer.
 */
static void EnableCurrentPositionSimulationTimer(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    // Timer is already running
    if (accessoryConfiguration.currentPositionSimulationTimer) {
        return;
    }

    HAPError err = HAPPlatformTimerRegister(
            &accessoryConfiguration.currentPositionSimulationTimer,
            HAPPlatformClockGetCurrent() + kCurrentPosition_SimulationInterval,
            HandleCurrentPositionSimulationTimerCallback,
            server);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPLogInfo(&kHAPLog_Default, "Failed to start currentPositionSimulationTimer");
        HAPFatalError();
    }
}

/**
 * Callback to disable the current position simulation timer.
 */
static void DisableCurrentPositionSimulationTimer(void) {
    // Disable if Timer is running
    if (accessoryConfiguration.currentPositionSimulationTimer) {
        HAPPlatformTimerDeregister(accessoryConfiguration.currentPositionSimulationTimer);
        accessoryConfiguration.currentPositionSimulationTimer = 0;
    }
}

/**
 * Simple current position simulation:
 * - Decrease current position by 'kCurrentPosition_Simulation_StepValue' if position state is
 * kHAPCharacteristicValue_PositionState_GoingToMinimum
 * - Increase current position by 'kCurrentPosition_Simulation_StepValue' if position state is
 * kHAPCharacteristicValue_PositionState_GoingToMaximum
 * - Cancel the timer otherwise
 */
static void SimulateCurrentPosition(HAPAccessoryServer* server) {
    HAPPrecondition(server);
    switch (accessoryConfiguration.state.positionState) {
        case kHAPCharacteristicValue_PositionState_GoingToMinimum: {
            uint8_t diff = accessoryConfiguration.state.currentPosition - accessoryConfiguration.state.targetPosition;
            if (diff >= kCurrentPosition_Simulation_StepValue) {
                accessoryConfiguration.state.currentPosition -= kCurrentPosition_Simulation_StepValue;
            } else {
                accessoryConfiguration.state.currentPosition = accessoryConfiguration.state.targetPosition;
                AccessoryNotification(&accessory, &windowCoveringCurrentPositionCharacteristic, &windowCoveringService);
                accessoryConfiguration.state.positionState = kHAPCharacteristicValue_PositionState_Stopped;
                AccessoryNotification(&accessory, &windowCoveringPositionStateCharacteristic, &windowCoveringService);
                DisableCurrentPositionSimulationTimer();
            }
            HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, accessoryConfiguration.state.currentPosition);
            SaveAccessoryState();
            break;
        }
        case kHAPCharacteristicValue_PositionState_GoingToMaximum: {
            uint8_t diff = accessoryConfiguration.state.targetPosition - accessoryConfiguration.state.currentPosition;
            if (diff >= kCurrentPosition_Simulation_StepValue) {
                accessoryConfiguration.state.currentPosition += kCurrentPosition_Simulation_StepValue;
            } else {
                accessoryConfiguration.state.currentPosition = accessoryConfiguration.state.targetPosition;
                AccessoryNotification(&accessory, &windowCoveringCurrentPositionCharacteristic, &windowCoveringService);
                accessoryConfiguration.state.positionState = kHAPCharacteristicValue_PositionState_Stopped;
                AccessoryNotification(&accessory, &windowCoveringPositionStateCharacteristic, &windowCoveringService);
                DisableCurrentPositionSimulationTimer();
            }
            HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, accessoryConfiguration.state.currentPosition);
            SaveAccessoryState();
            break;
        }
        case kHAPCharacteristicValue_PositionState_Stopped: {
            DisableCurrentPositionSimulationTimer();
            break;
        }
        default:
            HAPFatalError();
    }
}

/**
 * Callback for the current position simulation timer to simulate window current position on each tick
 */
static void
        HandleCurrentPositionSimulationTimerCallback(HAPPlatformTimerRef timer HAP_UNUSED, void* _Nullable context) {
    HAPPrecondition(context);

    HAPAccessoryServer* server = context;
    accessoryConfiguration.currentPositionSimulationTimer = 0;

    SimulateCurrentPosition(server);

    // Restart timer
    EnableCurrentPositionSimulationTimer(server);
}

/**
 * Set the window covering target position.
 */
static void SetTargetPosition(HAPAccessoryServer* server, uint8_t targetPosition) {
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    if (accessoryConfiguration.state.holdPosition) {
        accessoryConfiguration.state.holdPosition = false;
    }
    accessoryConfiguration.state.targetPosition = targetPosition;
    if (accessoryConfiguration.state.currentPosition > targetPosition) {
        accessoryConfiguration.state.positionState = kHAPCharacteristicValue_PositionState_GoingToMinimum;
    } else if (accessoryConfiguration.state.currentPosition < targetPosition) {
        accessoryConfiguration.state.positionState = kHAPCharacteristicValue_PositionState_GoingToMaximum;
    } else {
        accessoryConfiguration.state.positionState = kHAPCharacteristicValue_PositionState_Stopped;
    }
    AccessoryNotification(&accessory, &windowCoveringPositionStateCharacteristic, &windowCoveringService);
    SaveAccessoryState();
    EnableCurrentPositionSimulationTimer(server);
}

#if (HAP_TESTING == 1)

/**
 * Toggle the obstruction detected for Window Covering
 */
void ToggleObstructionDetected(void) {
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    if (accessoryConfiguration.state.isObstructionDetected) {
        accessoryConfiguration.state.isObstructionDetected = false;
    } else {
        accessoryConfiguration.state.isObstructionDetected = true;
    }

    HAPLogInfo(
            &kHAPLog_Default,
            "%s: %s",
            __func__,
            accessoryConfiguration.state.isObstructionDetected ? "true" : "false");

    AccessoryNotification(&accessory, &windowCoveringObstructionDetectedCharacteristic, &windowCoveringService);
}

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
 * HomeKit accessory that provides the Window Covering service.
 */
static HAPAccessory accessory = { .aid = 1,
                                  .category = kHAPAccessoryCategory_WindowCoverings,
                                  .name = "Acme Window Covering",
                                  .productData = "03d8a775e3644573",
                                  .manufacturer = "Acme",
                                  .model = "WindowCovering1,1",
                                  .serialNumber = "099DB48E9E28",
                                  .firmwareVersion = "1",
                                  .hardwareVersion = "1",
                                  .services = (const HAPService* const[]) { &accessoryInformationService,
                                                                            &hapProtocolInformationService,
                                                                            &pairingService,
                                                                            &windowCoveringService,
#if (HAVE_THREAD == 1)
                                                                            &threadManagementService,
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
#if (HAP_TESTING == 1)
                                                                            &debugService,
#endif
                                                                            NULL },
#if (HAP_APP_USES_HDS == 1)
                                  .dataStream.delegate = { .callbacks = &kHAPDataStreamDispatcher_DataStreamCallbacks,
                                                           .context = &dataStreamDispatcher },
#endif
#if (HAVE_ACCESSORY_REACHABILITY == 1)
                                  .reachabilityConfiguration = &accessoryReachabilityConfig,
#endif
                                  .callbacks = {
                                          .identify = IdentifyAccessory,
#if (HAVE_FIRMWARE_UPDATE == 1)
                                          .firmwareUpdate = { .getAccessoryState = FirmwareUpdateGetAccessoryState },
#endif
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
                                          .diagnosticsConfig = { .getDiagnosticsConfig =
                                                                         GetAccessoryDiagnosticsConfig },
#endif
                                  } };

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
 * Handle read request to the 'Current Position' characteristic of the Window Covering service.
 */
HAP_RESULT_USE_CHECK
HAPError HAPHandleWindowCoveringCurrentPositionRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.currentPosition;
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, *value);
    return kHAPError_None;
}

/**
 * Handle read request to the 'Target Position' characteristic of the Window Covering service.
 */
HAP_RESULT_USE_CHECK
HAPError HAPHandleWindowCoveringTargetPositionRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.targetPosition;
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, *value);
    return kHAPError_None;
}

/**
 * Handle write request to the 'Target Position' characteristic of the Window Covering service.
 */
HAP_RESULT_USE_CHECK
HAPError HAPHandleWindowCoveringTargetPositionWrite(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicWriteRequest* request HAP_UNUSED,
        uint8_t value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    SetTargetPosition(server, value);
    return kHAPError_None;
}

/**
 * Handle read request to the 'Position State' characteristic of the Window Covering service.
 */
HAP_RESULT_USE_CHECK
HAPError HAPHandleWindowCoveringPositionStateRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    *value = accessoryConfiguration.state.positionState;
    switch (*value) {
        case kHAPCharacteristicValue_PositionState_GoingToMinimum: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "PositionState_GoingToMinimum");
            break;
        }
        case kHAPCharacteristicValue_PositionState_GoingToMaximum: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "PositionState_GoingToMaximum");
            break;
        }
        case kHAPCharacteristicValue_PositionState_Stopped: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "PositionState_Stopped");
            break;
        }
    }
    return kHAPError_None;
}

/**
 * Handle write request to the 'Hold Position' characteristic of the Window Covering service.
 */
HAP_RESULT_USE_CHECK
HAPError HAPHandleWindowCoveringHoldPositionWrite(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPBoolCharacteristicWriteRequest* request HAP_UNUSED,
        bool value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    accessoryConfiguration.state.holdPosition = value;

    if (accessoryConfiguration.state.holdPosition) {
        DisableCurrentPositionSimulationTimer();
        accessoryConfiguration.state.positionState = kHAPCharacteristicValue_PositionState_Stopped;
        AccessoryNotification(&accessory, &windowCoveringCurrentPositionCharacteristic, &windowCoveringService);
        AccessoryNotification(&accessory, &windowCoveringPositionStateCharacteristic, &windowCoveringService);
        SaveAccessoryState();
    }
    return kHAPError_None;
}

/**
 * Handle read request to the 'Current Horizontal Tilt' characteristic of the Window Covering service.
 */
HAP_RESULT_USE_CHECK
HAPError HAPHandleWindowCoveringCurrentHorizontalTiltRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPIntCharacteristicReadRequest* request HAP_UNUSED,
        int32_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.currentHorizontalTilt;
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, (int) *value);
    return kHAPError_None;
}

/**
 * Handle read request to the 'Target Horizontal Tilt' characteristic of the Window Covering service.
 */
HAP_RESULT_USE_CHECK
HAPError HAPHandleWindowCoveringTargetHorizontalTiltRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPIntCharacteristicReadRequest* request HAP_UNUSED,
        int32_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.targetHorizontalTilt;
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, (int) *value);
    return kHAPError_None;
}

/**
 * Handle write request to the 'Target Horizontal Tilt' characteristic of the Window Covering service.
 */
HAP_RESULT_USE_CHECK
HAPError HAPHandleWindowCoveringTargetHorizontalTiltWrite(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPIntCharacteristicWriteRequest* request HAP_UNUSED,
        int32_t value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    accessoryConfiguration.state.targetHorizontalTilt = value;
    AccessoryNotification(&accessory, &windowCoveringTargetHorizontalTiltCharacteristic, &windowCoveringService);
    if (accessoryConfiguration.state.currentHorizontalTilt != value) {
        accessoryConfiguration.state.currentHorizontalTilt = value;
        AccessoryNotification(&accessory, &windowCoveringCurrentHorizontalTiltCharacteristic, &windowCoveringService);
    }
    SaveAccessoryState();
    return kHAPError_None;
}

/**
 * Handle read request to the 'Current Vertical Tilt' characteristic of the Window Covering service.
 */
HAP_RESULT_USE_CHECK
HAPError HAPHandleWindowCoveringCurrentVerticalTiltRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPIntCharacteristicReadRequest* request HAP_UNUSED,
        int32_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.currentVerticalTilt;
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, (int) *value);
    return kHAPError_None;
}

/**
 * Handle read request to the 'Target Vertical Tilt' characteristic of the Window Covering service.
 */
HAP_RESULT_USE_CHECK
HAPError HAPHandleWindowCoveringTargetVerticalTiltRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPIntCharacteristicReadRequest* request HAP_UNUSED,
        int32_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.targetVerticalTilt;
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, (int) *value);
    return kHAPError_None;
}

/**
 * Handle write request to the 'Target Vertical Tilt' characteristic of the Window Covering service.
 */
HAP_RESULT_USE_CHECK
HAPError HAPHandleWindowCoveringTargetVerticalTiltWrite(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPIntCharacteristicWriteRequest* request HAP_UNUSED,
        int32_t value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    accessoryConfiguration.state.targetVerticalTilt = value;
    AccessoryNotification(&accessory, &windowCoveringTargetVerticalTiltCharacteristic, &windowCoveringService);
    if (accessoryConfiguration.state.currentVerticalTilt != value) {
        accessoryConfiguration.state.currentVerticalTilt = value;
        AccessoryNotification(&accessory, &windowCoveringCurrentVerticalTiltCharacteristic, &windowCoveringService);
    }
    SaveAccessoryState();
    return kHAPError_None;
}

/**
 * Handle read request to the 'Obstruction Detected' characteristic of the Window Covering service.
 */
HAP_RESULT_USE_CHECK
HAPError HAPHandleWindowCoveringObstructionDetectedRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPBoolCharacteristicReadRequest* request HAP_UNUSED,
        bool* value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    *value = accessoryConfiguration.state.isObstructionDetected;
    return kHAPError_None;
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

void AccessoryNotification(
        const HAPAccessory* accessory,
        const HAPCharacteristic* characteristic,
        const HAPService* service) {
    HAPLogInfo(&kHAPLog_Default, "Accessory Notification");

    HAPAccessoryServerRaiseEvent(accessoryConfiguration.server, characteristic, service, accessory);
}

void AppCreate(HAPAccessoryServer* server, HAPPlatformKeyValueStoreRef keyValueStore) {
    HAPPrecondition(server);
    HAPPrecondition(keyValueStore);

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    HAPRawBufferZero(&accessoryConfiguration, sizeof accessoryConfiguration);
    accessoryConfiguration.server = server;
    accessoryConfiguration.keyValueStore = keyValueStore;

    ConfigureIO();

    LoadAccessoryState();

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
    HAPAccessoryServerStart(accessoryConfiguration.server, &accessory);
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
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
    DiagnosticsHandlePairingStateChange(
            state, &accessoryConfiguration.state.diagnosticsSelectedModeState, &SaveAccessoryState);
#endif // (HAVE_DIAGNOSTICS_SERVICE == 1)
}

const HAPAccessory* AppGetAccessoryInfo(void) {
    return &accessory;
}

const HAPAccessory* _Nonnull const* _Nullable AppGetBridgeAccessoryInfo(void) {
    return NULL;
}

void AppInitialize(
        HAPAccessoryServerOptions* hapAccessoryServerOptions HAP_UNUSED,
        HAPPlatform* hapPlatform HAP_UNUSED,
        HAPAccessoryServerCallbacks* hapAccessoryServerCallbacks HAP_UNUSED) {
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
    InitializeDiagnostics(&accessoryConfiguration.state.diagnosticsSelectedModeState, &accessory, hapPlatform);
#endif // (HAVE_DIAGNOSTICS_SERVICE == 1)
}

void AppDeinitialize(void) {
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
    DeinitializeDiagnostics(&accessory);
#endif // (HAVE_DIAGNOSTICS_SERVICE == 1)
}
