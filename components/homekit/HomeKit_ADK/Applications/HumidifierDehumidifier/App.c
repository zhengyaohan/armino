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

// An example that implements the Humidifier/Dehumidifier HomeKit service. It can serve as a basic
// implementation for any platform. The accessory logic implementation is reduced to internal state
// updates and log output.
//
// To enable user interaction, buttons on the development board are used:
//
// - LED 1 is used to simulate active state.
//         ON: Humidifier/Dehumidifier active
//         OFF: Humidifier/Dehumidifier inactive
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
//   4. Helper functions that modify the state of the sensor.
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
    AppLEDIdentifier activeLedPin;
    AppLEDIdentifier identifyPin;
} Device;

/**
 * Global accessory configuration.
 */
typedef struct {
    struct {
        /**
         * The Active characteristic indicates whether the service is currently active.
         */
        HAPCharacteristicValue_Active active;

        /**
         * This characteristic describes the current relative humidity of the accessoryʼs environment. The value is
         * expressed as a percentage (%).
         */
        float currentRelativeHumidity;

        /**
         * This characteristic describes the current state of a humidifier or/and a dehumidifier.
         */
        HAPCharacteristicValue_CurrentHumidifierDehumidifierState currentHumidifierDehumidifierState;

        /**
         * This characteristic describes the target state of a humidifier or/and a dehumidifier.
         */
        HAPCharacteristicValue_TargetHumidifierDehumidifierState targetHumidifierDehumidifierState;

        /**
         * This characteristic describes the relative humidity dehumidifier threshold. The value of this characteristic
         * represents the `maximum relative humidity` that must be reached before dehumidifier is turned on.
         */
        float relativeHumidityDehumidifierThreshold;

        /**
         * This characteristic describes the relative humidity humidifier threshold. The value of this characteristic
         * represents the `minimum relative humidity` that must be reached before humidifier is turned on.
         */
        float relativeHumidityHumidifierThreshold;
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
        uint32_t diagnosticsSelectedModeState;
#endif // (HAVE_DIAGNOSTICS_SERVICE == 1)
    } state;
    Device device;
    HAPPlatformTimerRef identifyTimer;
    HAPAccessoryServer* server;
    HAPPlatformKeyValueStoreRef keyValueStore;
    HAPPlatformTimerRef humiditySimulationTimer;
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
 * Macros used in application
 */
#define kHumidifierDehumidifier_InitialRelativeHumidity    50
#define kHumidifierDehumidifier_HumiditySimulationInterval ((HAPTime)(5 * HAPSecond))

static void EnableHumiditySimulationTimer(HAPAccessoryServer* server);
static void DisableHumiditySimulationTimer(void);

/**
 * Initialize a default accessory state that validates with the device configuration.
 */
static void SetupDefaultAccessoryState(void) {
    accessoryConfiguration.state.active = kHAPCharacteristicValue_Active_Inactive;
    accessoryConfiguration.state.currentRelativeHumidity = kHumidifierDehumidifier_InitialRelativeHumidity;
    accessoryConfiguration.state.currentHumidifierDehumidifierState =
            kHAPCharacteristicValue_CurrentHumidifierDehumidifierState_Inactive;
    accessoryConfiguration.state.targetHumidifierDehumidifierState =
            kHAPCharacteristicValue_TargetHumidifierDehumidifierState_HumidifierOrDehumidifier;
    accessoryConfiguration.state.relativeHumidityDehumidifierThreshold =
            humidifierDehumidifierRelativeHumidityDehumidifierThresholdCharacteristic.constraints.maximumValue;
    accessoryConfiguration.state.relativeHumidityHumidifierThreshold =
            humidifierDehumidifierRelativeHumidityHumidifierThresholdCharacteristic.constraints.minimumValue;
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
    accessoryConfiguration.state.diagnosticsSelectedModeState = kHAPCharacteristicValue_SelectedDiagnosticsModes_None;
#endif // (HAVE_DIAGNOSTICS_SERVICE == 1)
}

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
        SetupDefaultAccessoryState();
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
 * HomeKit accessory that provides the Humidifier/Dehumidifier service.
 */
static HAPAccessory accessory = { .aid = 1,
                                  .category = kHAPAccessoryCategory_Humidifiers,
                                  .name = "Acme Humidifier/Dehumidifier",
                                  .productData = "03d8a775e3644573",
                                  .manufacturer = "Acme",
                                  .model = "HumidifierDehumidifier1,1",
                                  .serialNumber = "099DB48E9E28",
                                  .firmwareVersion = "1",
                                  .hardwareVersion = "1",
                                  .services =
                                          (const HAPService* const[]) {
                                                  &accessoryInformationService,
                                                  &hapProtocolInformationService,
                                                  &pairingService,
                                                  &humidifierDehumidifierService,
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

#if (HAP_TESTING == 1)

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
    accessoryConfiguration.device.activeLedPin = kAppLEDIdentifier_1;
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

/**
 * Returns the string associated to 'Current Humidifier Dehumidifier State'.
 */
static const char* GetCurrentHumidifierDehumidifierStateDescription(void) {
    switch (accessoryConfiguration.state.currentHumidifierDehumidifierState) {
        case kHAPCharacteristicValue_CurrentHumidifierDehumidifierState_Inactive: {
            return "Inactive";
        }
        case kHAPCharacteristicValue_CurrentHumidifierDehumidifierState_Idle: {
            return "Idle";
        }
        case kHAPCharacteristicValue_CurrentHumidifierDehumidifierState_Humidifying: {
            return "Humidifying";
        }
        case kHAPCharacteristicValue_CurrentHumidifierDehumidifierState_Dehumidifying: {
            return "Dehumidifying";
        }
        default: {
            HAPFatalError();
        }
    }
}

/**
 * Determine if the minimum humidity value has been reached.
 */
static bool IsMinimumHumidityReached(void) {
    if (accessoryConfiguration.state.currentRelativeHumidity ==
        humidifierDehumidifierCurrentRelativeHumidityCharacteristic.constraints.minimumValue) {
        return true;
    }
    return false;
}

/**
 * Determine if the maximum humidity value has been reached.
 */
static bool IsMaximumHumidityReached(void) {
    if (accessoryConfiguration.state.currentRelativeHumidity ==
        humidifierDehumidifierCurrentRelativeHumidityCharacteristic.constraints.maximumValue) {
        return true;
    }
    return false;
}

/**
 * Determine if humidifier threshold has been met.
 */
static bool IsHumidifierThresholdReached(void) {
    // Threshold met if the current humidity is below the humidifier threshold
    if (accessoryConfiguration.state.currentRelativeHumidity <=
        accessoryConfiguration.state.relativeHumidityHumidifierThreshold) {
        return true;
    }
    return false;
}

/**
 * Determine if dehumidifier threshold has been reached.
 */
static bool IsDehumidifierThresholdReached(void) {
    // Threshold met if the current humidity is above the dehumidifier threshold
    if (accessoryConfiguration.state.currentRelativeHumidity >=
        accessoryConfiguration.state.relativeHumidityDehumidifierThreshold) {
        return true;
    }
    return false;
}

/**
 * Determine if currently in a humidifying target state.
 */
static bool IsHumidifyingTargetState(void) {
    if ((accessoryConfiguration.state.targetHumidifierDehumidifierState ==
         kHAPCharacteristicValue_TargetHumidifierDehumidifierState_HumidifierOrDehumidifier) ||
        (accessoryConfiguration.state.targetHumidifierDehumidifierState ==
         kHAPCharacteristicValue_TargetHumidifierDehumidifierState_Humidifier)) {
        return true;
    }
    return false;
}

/**
 * Determine if currently in a dehumidifying target state.
 */
static bool IsDehumidifyingTargetState(void) {
    if ((accessoryConfiguration.state.targetHumidifierDehumidifierState ==
         kHAPCharacteristicValue_TargetHumidifierDehumidifierState_HumidifierOrDehumidifier) ||
        (accessoryConfiguration.state.targetHumidifierDehumidifierState ==
         kHAPCharacteristicValue_TargetHumidifierDehumidifierState_Dehumidifier)) {
        return true;
    }
    return false;
}

/**
 * Update the 'Current Humidifier Dehumidifier State' accordingly
 */
static void UpdateCurrentHumidifierDehumidifierState(void) {
    switch (accessoryConfiguration.state.active) {
        case kHAPCharacteristicValue_Active_Inactive: {
            // 'Active' acts as a main power for the accessory so when it is inactive, then the
            // 'Current Humidifier Dehumidifier State' is inactive as well
            accessoryConfiguration.state.currentHumidifierDehumidifierState =
                    kHAPCharacteristicValue_CurrentHumidifierDehumidifierState_Inactive;
            break;
        }
        case kHAPCharacteristicValue_Active_Active: {
            // Set the 'Current Humidifier Dehumidifier State' based on whether thresholds have been met
            HAPCharacteristicValue_CurrentHumidifierDehumidifierState currentHumidifierDehumidifierState =
                    kHAPCharacteristicValue_CurrentHumidifierDehumidifierState_Idle;
            switch (accessoryConfiguration.state.targetHumidifierDehumidifierState) {
                case kHAPCharacteristicValue_TargetHumidifierDehumidifierState_HumidifierOrDehumidifier: {
                    bool humidifierThresholdReached = IsHumidifierThresholdReached();
                    bool dehumidifierThresholdReached = IsDehumidifierThresholdReached();
                    if (humidifierThresholdReached && !dehumidifierThresholdReached) {
                        currentHumidifierDehumidifierState =
                                kHAPCharacteristicValue_CurrentHumidifierDehumidifierState_Humidifying;
                    } else if (!humidifierThresholdReached && dehumidifierThresholdReached) {
                        currentHumidifierDehumidifierState =
                                kHAPCharacteristicValue_CurrentHumidifierDehumidifierState_Dehumidifying;
                    } else if (humidifierThresholdReached && dehumidifierThresholdReached) {
                        // This should never happen unless the threshold values are set up incorrectly so favor
                        // humidifying
                        currentHumidifierDehumidifierState =
                                kHAPCharacteristicValue_CurrentHumidifierDehumidifierState_Humidifying;
                    }
                    break;
                }
                case kHAPCharacteristicValue_TargetHumidifierDehumidifierState_Humidifier: {
                    if (IsHumidifierThresholdReached()) {
                        currentHumidifierDehumidifierState =
                                kHAPCharacteristicValue_CurrentHumidifierDehumidifierState_Humidifying;
                    }
                    break;
                }
                case kHAPCharacteristicValue_TargetHumidifierDehumidifierState_Dehumidifier: {
                    if (IsDehumidifierThresholdReached()) {
                        currentHumidifierDehumidifierState =
                                kHAPCharacteristicValue_CurrentHumidifierDehumidifierState_Dehumidifying;
                    }
                    break;
                }
            }

            accessoryConfiguration.state.currentHumidifierDehumidifierState = currentHumidifierDehumidifierState;
            break;
        }
    }

    HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, GetCurrentHumidifierDehumidifierStateDescription());
    SaveAccessoryState();
    HAPAccessoryServerRaiseEvent(
            accessoryConfiguration.server,
            &humidifierDehumidifierCurrentHumidifierDehumidifierStateCharacteristic,
            &humidifierDehumidifierService,
            &accessory);
}

/**
 * Function for clamping a float value between min and max
 */
static float Clamp(float min, float max, float value) {
    if (value < min) {
        return min;
    }
    if (value > max) {
        return max;
    }
    return value;
}

/**
 * Simulate humidity
 *  - If 'Current Humidifier Dehumidifier State' is Inactive, keep humidity at the initial value
 *  - If 'Current Humidifier Dehumidifier State' is Idle, keep humidity at the initial value unless either Humidifier or
 *    Dehumidifier Thresholds have been reached
 *  - If 'Current Humidifier Dehumidifier State' is Humidifying, increase humidity unless Dehumidifier Threshold reached
 *  - If 'Current Humidifier Dehumidifier State' is Dehumidifying, decrease humidity unless Humidifier Threshold reached
 */
static void SimulateHumidity(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    float humidity = accessoryConfiguration.state.currentRelativeHumidity;
    float change = 0.0F;

    switch (accessoryConfiguration.state.currentHumidifierDehumidifierState) {
        case kHAPCharacteristicValue_CurrentHumidifierDehumidifierState_Inactive: {
            // If not actively humidifying or dehumidifying, then drift towards the initial humidity
            if (humidity < kHumidifierDehumidifier_InitialRelativeHumidity) {
                change = 1.0F;
            } else if (humidity > kHumidifierDehumidifier_InitialRelativeHumidity) {
                change = -1.0F;
            }
            break;
        }
        case kHAPCharacteristicValue_CurrentHumidifierDehumidifierState_Idle: {
            if ((IsHumidifyingTargetState() && IsHumidifierThresholdReached()) ||
                (IsDehumidifyingTargetState() && IsDehumidifierThresholdReached())) {
                // If idle, check if either thresholds have been met and update the state to
                // humidifying or dehumidifying
                UpdateCurrentHumidifierDehumidifierState();
            } else {
                // If not actively humidifying or dehumidifying, then drift towards the initial humidity
                if (humidity < kHumidifierDehumidifier_InitialRelativeHumidity) {
                    change = 1.0F;
                } else if (humidity > kHumidifierDehumidifier_InitialRelativeHumidity) {
                    change = -1.0F;
                }
            }
            break;
        }
        case kHAPCharacteristicValue_CurrentHumidifierDehumidifierState_Humidifying: {
            if (IsMaximumHumidityReached() || (IsDehumidifyingTargetState() && IsDehumidifierThresholdReached())) {
                UpdateCurrentHumidifierDehumidifierState();
            } else {
                change = 2.0F;
            }
            break;
        }
        case kHAPCharacteristicValue_CurrentHumidifierDehumidifierState_Dehumidifying: {
            if (IsMinimumHumidityReached() || (IsHumidifyingTargetState() && IsHumidifierThresholdReached())) {
                UpdateCurrentHumidifierDehumidifierState();
            } else {
                change = -2.0F;
            }
            break;
        }
        default: {
            HAPFatalError();
        }
    }

    humidity += change;
    humidity =
            Clamp(humidifierDehumidifierCurrentRelativeHumidityCharacteristic.constraints.minimumValue,
                  humidifierDehumidifierCurrentRelativeHumidityCharacteristic.constraints.maximumValue,
                  humidity);

    accessoryConfiguration.state.currentRelativeHumidity = humidity;
    SaveAccessoryState();
    HAPAccessoryServerRaiseEvent(
            server,
            &humidifierDehumidifierCurrentRelativeHumidityCharacteristic,
            &humidifierDehumidifierService,
            &accessory);
}

/**
 * Callback to simulate humidity
 */
static void HandleHumiditySimulationTimerCallback(HAPPlatformTimerRef timer HAP_UNUSED, void* _Nullable context) {
    HAPPrecondition(context);

    HAPAccessoryServer* server = context;
    accessoryConfiguration.humiditySimulationTimer = 0;

    SimulateHumidity(server);

    // Restart timer
    EnableHumiditySimulationTimer(server);
}

/**
 * Enable the humidity simulation via timer.
 */
static void EnableHumiditySimulationTimer(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    HAPError err;

    // Timer is already running
    if (accessoryConfiguration.humiditySimulationTimer) {
        return;
    }

    err = HAPPlatformTimerRegister(
            &accessoryConfiguration.humiditySimulationTimer,
            HAPPlatformClockGetCurrent() + kHumidifierDehumidifier_HumiditySimulationInterval,
            HandleHumiditySimulationTimerCallback,
            server);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPFatalError();
    }
}

/**
 * Disable the humidity simulation via timer.
 */
static void DisableHumiditySimulationTimer(void) {
    // Disable if timer is running
    if (accessoryConfiguration.humiditySimulationTimer) {
        HAPPlatformTimerDeregister(accessoryConfiguration.humiditySimulationTimer);
    }
}

//----------------------------------------------------------------------------------------------------------------------

#if (HAP_TESTING == 1)
/**
 * Toggle the humidifier/dehumidifier active state.
 */
void ToggleActiveState(void) {
    switch (accessoryConfiguration.state.active) {
        case kHAPCharacteristicValue_Active_Inactive: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "Active");
            accessoryConfiguration.state.active = kHAPCharacteristicValue_Active_Active;
            DeviceEnableLED(accessoryConfiguration.device.activeLedPin);
            break;
        }
        case kHAPCharacteristicValue_Active_Active: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "Inactive");
            accessoryConfiguration.state.active = kHAPCharacteristicValue_Active_Inactive;
            DeviceDisableLED(accessoryConfiguration.device.activeLedPin);
            break;
        }
    }

    UpdateCurrentHumidifierDehumidifierState();
    SaveAccessoryState();
    HAPAccessoryServerRaiseEvent(
            accessoryConfiguration.server,
            &humidifierDehumidifierActiveCharacteristic,
            &humidifierDehumidifierService,
            &accessory);
}
#endif

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HandleHumidifierDehumidifierActiveRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.active;

    switch (*value) {
        case kHAPCharacteristicValue_Active_Active: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "Active");
            break;
        }
        case kHAPCharacteristicValue_Active_Inactive: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "Inactive");
            break;
        }
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleHumidifierDehumidifierActiveWrite(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicWriteRequest* request HAP_UNUSED,
        uint8_t value,
        void* _Nullable context HAP_UNUSED) {

    HAPCharacteristicValue_Active active = (HAPCharacteristicValue_Active) value;
    if (accessoryConfiguration.state.active != active) {
        accessoryConfiguration.state.active = active;

        switch (active) {
            case kHAPCharacteristicValue_Active_Active: {
                HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "Active");
                DeviceEnableLED(accessoryConfiguration.device.activeLedPin);
                break;
            }
            case kHAPCharacteristicValue_Active_Inactive: {
                HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "Inactive");
                DeviceDisableLED(accessoryConfiguration.device.activeLedPin);
                break;
            }
        }

        UpdateCurrentHumidifierDehumidifierState();
        SaveAccessoryState();
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleHumidifierDehumidifierCurrentRelativeHumidityRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPFloatCharacteristicReadRequest* request HAP_UNUSED,
        float* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.currentRelativeHumidity;
    HAPLogInfo(&kHAPLog_Default, "%s: %g", __func__, *value);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleHumidifierDehumidifierCurrentHumidifierDehumidifierStateRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.currentHumidifierDehumidifierState;

    HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, GetCurrentHumidifierDehumidifierStateDescription());

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleHumidifierDehumidifierTargetHumidifierDehumidifierStateRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.targetHumidifierDehumidifierState;

    switch (*value) {
        case kHAPCharacteristicValue_TargetHumidifierDehumidifierState_HumidifierOrDehumidifier: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "HumidifierOrDehumidifier");
            break;
        }
        case kHAPCharacteristicValue_TargetHumidifierDehumidifierState_Humidifier: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "Humidifier");
            break;
        }
        case kHAPCharacteristicValue_TargetHumidifierDehumidifierState_Dehumidifier: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "Dehumidifier");
            break;
        }
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleHumidifierDehumidifierTargetHumidifierDehumidifierStateWrite(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicWriteRequest* request HAP_UNUSED,
        uint8_t value,
        void* _Nullable context HAP_UNUSED) {

    HAPCharacteristicValue_TargetHumidifierDehumidifierState targetHumidifierDehumidifierState =
            (HAPCharacteristicValue_TargetHumidifierDehumidifierState) value;
    if (accessoryConfiguration.state.targetHumidifierDehumidifierState != targetHumidifierDehumidifierState) {
        accessoryConfiguration.state.targetHumidifierDehumidifierState = targetHumidifierDehumidifierState;

        switch (targetHumidifierDehumidifierState) {
            case kHAPCharacteristicValue_TargetHumidifierDehumidifierState_HumidifierOrDehumidifier: {
                HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "HumidifierOrDehumidifier");
                break;
            }
            case kHAPCharacteristicValue_TargetHumidifierDehumidifierState_Humidifier: {
                HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "Humidifier");
                break;
            }
            case kHAPCharacteristicValue_TargetHumidifierDehumidifierState_Dehumidifier: {
                HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "Dehumidifier");
                break;
            }
        }

        UpdateCurrentHumidifierDehumidifierState();
        SaveAccessoryState();
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleHumidifierDehumidifierRelativeHumidityDehumidifierThresholdRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPFloatCharacteristicReadRequest* request HAP_UNUSED,
        float* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.relativeHumidityDehumidifierThreshold;
    HAPLogInfo(&kHAPLog_Default, "%s: %g", __func__, *value);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleHumidifierDehumidifierRelativeHumidityDehumidifierThresholdWrite(
        HAPAccessoryServer* server,
        const HAPFloatCharacteristicWriteRequest* request,
        float value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s: %g", __func__, value);
    if (accessoryConfiguration.state.relativeHumidityDehumidifierThreshold != value) {
        accessoryConfiguration.state.relativeHumidityDehumidifierThreshold = value;
        UpdateCurrentHumidifierDehumidifierState();
        SaveAccessoryState();
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleHumidifierDehumidifierRelativeHumidityHumidifierThresholdRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPFloatCharacteristicReadRequest* request HAP_UNUSED,
        float* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.relativeHumidityHumidifierThreshold;
    HAPLogInfo(&kHAPLog_Default, "%s: %g", __func__, *value);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleHumidifierDehumidifierRelativeHumidityHumidifierThresholdWrite(
        HAPAccessoryServer* server,
        const HAPFloatCharacteristicWriteRequest* request,
        float value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s: %g", __func__, value);
    if (accessoryConfiguration.state.relativeHumidityHumidifierThreshold != value) {
        accessoryConfiguration.state.relativeHumidityHumidifierThreshold = value;
        UpdateCurrentHumidifierDehumidifierState();
        SaveAccessoryState();
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }
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

    switch (accessoryConfiguration.state.active) {
        case kHAPCharacteristicValue_Active_Active: {
            DeviceEnableLED(accessoryConfiguration.device.activeLedPin);
            break;
        }
        case kHAPCharacteristicValue_Active_Inactive: {
            DeviceDisableLED(accessoryConfiguration.device.activeLedPin);
            break;
        }
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
    HAPAccessoryServerStart(accessoryConfiguration.server, &accessory);
#if (HAVE_FIRMWARE_UPDATE == 1)
    FirmwareUpdateStart(accessoryConfiguration.server, &accessory);
#endif // HAVE_FIRMWARE_UPDATE
}

//----------------------------------------------------------------------------------------------------------------------

void AppHandleAccessoryServerStateUpdate(HAPAccessoryServer* server, void* _Nullable context HAP_UNUSED) {
    switch (HAPAccessoryServerGetState(server)) {
        case kHAPAccessoryServerState_Running: {
            EnableHumiditySimulationTimer(accessoryConfiguration.server);
            return;
        }
        case kHAPAccessoryServerState_Stopping: {
            DisableHumiditySimulationTimer();
            return;
        }
        default: {
            break;
        }
    }
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
