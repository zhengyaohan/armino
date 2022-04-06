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
// Copyright (C) 2021 Apple Inc. All Rights Reserved.

// An example that implements the Security System HomeKit service. It can serve as a basic implementation for
// any platform. The accessory logic implementation is reduced to internal state updates and log output.
//
// To enable user interaction, buttons on the development board are used:
//
// - LED 1 is used to simulate alarm triggered state.
//         ON: Security system alarm triggered
//         OFF: Security system alarm not triggered
//
// - Button 1 or signal `SIGUSR1` to clear pairings.
// - Button 2 or signal `SIGUSR2` to trigger a factory reset.
// - Button 3 or Signal `SIGTERM` to trigger pairing mode.
// - Button 4 or Signal `SIGQUIT` unused.
//
// This implementation is platform-independent.
//
// The code consists of multiple parts:
//
//   1. The definition of the accessory configuration and its internal state.
//
//   2. Helper functions to load and save the state of the accessory.
//
//   3. The definitions for the HomeKit attribute database.
//
//   4. Helper functions that modify the state of the security system.
//
//   5. The callbacks that implement the actual behavior of the accessory, in this
//      case here they merely access the global accessory state variable and write
//      to the log to make the behavior easily observable.
//
//   6. The initialization of the accessory state.
//
//   7. Callbacks that notify the server in case their associated value has changed.
//
//   8. Signal handlers.

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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct {
    AppLEDIdentifier alarmTriggeredLedPin;
    AppLEDIdentifier identifyPin;
} Device;

/**
 * Global accessory configuration.
 */
typedef struct {
    struct {
        HAPCharacteristicValue_SecuritySystemCurrentState securitySystemCurrentState;
        HAPCharacteristicValue_SecuritySystemTargetState securitySystemTargetState;

#if (HAVE_DIAGNOSTICS_SERVICE == 1)
        uint32_t diagnosticsSelectedModeState;
#endif // (HAVE_DIAGNOSTICS_SERVICE == 1)
    } state;
    Device device;
    HAPPlatformTimerRef identifyTimer;
    HAPAccessoryServer* server;
    HAPPlatformKeyValueStoreRef keyValueStore;
} AccessoryConfiguration;

static AccessoryConfiguration accessoryConfiguration;

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

//----------------------------------------------------------------------------------------------------------------------

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
 * HomeKit accessory that provides the SecuritySystem service.
 */
static HAPAccessory accessory = { .aid = 1,
                                  .category = kHAPAccessoryCategory_SecuritySystems,
                                  .name = "Acme Security System",
                                  .productData = "03d8a775e3644573",
                                  .manufacturer = "Acme",
                                  .model = "SecuritySystem1,1",
                                  .serialNumber = "099DB48E9E28",
                                  .firmwareVersion = "1",
                                  .hardwareVersion = "1",
                                  .services =
                                          (const HAPService* const[]) {
                                                  &accessoryInformationService,
                                                  &hapProtocolInformationService,
                                                  &pairingService,
                                                  &securitySystemService,
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
                                                  NULL,
                                          },
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
 * Enable LED on the device
 */
static void DeviceEnableLED(AppLEDIdentifier pin) {
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    AppLEDSet(pin, true);
}

/**
 * Disable LED on the device
 */
static void DeviceDisableLED(AppLEDIdentifier pin) {
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    AppLEDSet(pin, false);
}

#if (HAP_TESTING == 1)

/**
 * Trigger security system alarm if not already triggered
 */
void TriggerAlarm(void) {
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    if (accessoryConfiguration.state.securitySystemCurrentState !=
        kHAPCharacteristicValue_SecuritySystemCurrentState_AlarmTriggered) {
        accessoryConfiguration.state.securitySystemCurrentState =
                kHAPCharacteristicValue_SecuritySystemCurrentState_AlarmTriggered;
        DeviceEnableLED(accessoryConfiguration.device.alarmTriggeredLedPin);
        SaveAccessoryState();
    }

    HAPAccessoryServerRaiseEvent(
            accessoryConfiguration.server,
            &securitySystemCurrentStateCharacteristic,
            &securitySystemService,
            &accessory);
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
 * Set the security system target state
 */
void SetSecuritySystemTargetState(HAPCharacteristicValue_SecuritySystemTargetState targetState) {
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, targetState);

    HAPCharacteristicValue_SecuritySystemCurrentState newCurrentState =
            kHAPCharacteristicValue_SecuritySystemCurrentState_Disarmed;
    switch (targetState) {
        case kHAPCharacteristicValue_SecuritySystemTargetState_StayArm:
            newCurrentState = kHAPCharacteristicValue_SecuritySystemCurrentState_StayArm;
            break;
        case kHAPCharacteristicValue_SecuritySystemTargetState_AwayArm:
            newCurrentState = kHAPCharacteristicValue_SecuritySystemCurrentState_AwayArm;
            break;
        case kHAPCharacteristicValue_SecuritySystemTargetState_NightArm:
            newCurrentState = kHAPCharacteristicValue_SecuritySystemCurrentState_NightArm;
            break;
        case kHAPCharacteristicValue_SecuritySystemTargetState_Disarm:
            newCurrentState = kHAPCharacteristicValue_SecuritySystemCurrentState_Disarmed;
            break;
    }

    if (accessoryConfiguration.state.securitySystemTargetState != targetState) {
        accessoryConfiguration.state.securitySystemTargetState = targetState;
        HAPAccessoryServerRaiseEvent(
                accessoryConfiguration.server,
                &securitySystemTargetStateCharacteristic,
                &securitySystemService,
                &accessory);
        SaveAccessoryState();
    }

    if (accessoryConfiguration.state.securitySystemCurrentState != newCurrentState) {
        accessoryConfiguration.state.securitySystemCurrentState = newCurrentState;

        if (accessoryConfiguration.state.securitySystemCurrentState ==
            kHAPCharacteristicValue_SecuritySystemCurrentState_AlarmTriggered) {
            DeviceEnableLED(accessoryConfiguration.device.alarmTriggeredLedPin);
        } else {
            DeviceDisableLED(accessoryConfiguration.device.alarmTriggeredLedPin);
        }

        HAPAccessoryServerRaiseEvent(
                accessoryConfiguration.server,
                &securitySystemCurrentStateCharacteristic,
                &securitySystemService,
                &accessory);
        SaveAccessoryState();
    }
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
    accessoryConfiguration.device.alarmTriggeredLedPin = kAppLEDIdentifier_1;
    accessoryConfiguration.device.identifyPin = kAppLEDIdentifier_2;

    // Initialize LED driver
    AppLEDInit();
}

/**
 * Unconfigure platform specific IO.
 */
static void UnconfigureIO(void) {
    HAPLogDebug(&kHAPLog_Default, "%s", __func__);

    AppLEDDeinit();
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HandleSecuritySystemCurrentStateRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.securitySystemCurrentState;

    switch (*value) {
        case kHAPCharacteristicValue_SecuritySystemCurrentState_StayArm: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "StayArm");
            break;
        }
        case kHAPCharacteristicValue_SecuritySystemCurrentState_AwayArm: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "AwayArm");
            break;
        }
        case kHAPCharacteristicValue_SecuritySystemCurrentState_NightArm: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "NightArm");
            break;
        }
        case kHAPCharacteristicValue_SecuritySystemCurrentState_Disarmed: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "Disarmed");
            break;
        }
        case kHAPCharacteristicValue_SecuritySystemCurrentState_AlarmTriggered: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "AlarmTriggered");
            break;
        }
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleSecuritySystemTargetStateRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.securitySystemTargetState;

    switch (*value) {
        case kHAPCharacteristicValue_SecuritySystemTargetState_StayArm: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "StayArm");
            break;
        }
        case kHAPCharacteristicValue_SecuritySystemTargetState_AwayArm: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "AwayArm");
            break;
        }
        case kHAPCharacteristicValue_SecuritySystemTargetState_NightArm: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "NightArm");
            break;
        }
        case kHAPCharacteristicValue_SecuritySystemTargetState_Disarm: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "Disarm");
            break;
        }
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleSecuritySystemTargetStateWrite(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicWriteRequest* request HAP_UNUSED,
        uint8_t value,
        void* _Nullable context HAP_UNUSED) {

    SetSecuritySystemTargetState((HAPCharacteristicValue_SecuritySystemTargetState) value);
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

//----------------------------------------------------------------------------------------------------------------------

void AppCreate(HAPAccessoryServer* server, HAPPlatformKeyValueStoreRef keyValueStore) {
    HAPPrecondition(server);
    HAPPrecondition(keyValueStore);

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    HAPRawBufferZero(&accessoryConfiguration, sizeof accessoryConfiguration);
    accessoryConfiguration.server = server;
    accessoryConfiguration.keyValueStore = keyValueStore;

    ConfigureIO();

    LoadAccessoryState();

    if (accessoryConfiguration.state.securitySystemCurrentState ==
        kHAPCharacteristicValue_SecuritySystemCurrentState_AlarmTriggered) {
        DeviceEnableLED(accessoryConfiguration.device.alarmTriggeredLedPin);
    } else {
        DeviceDisableLED(accessoryConfiguration.device.alarmTriggeredLedPin);
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
    AppLEDDeinit();
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
