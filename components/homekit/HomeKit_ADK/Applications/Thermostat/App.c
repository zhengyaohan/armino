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

// An example that implements the thermostat HomeKit profile. It can serve as a basic implementation for
// any platform. The accessory logic implementation is reduced to internal state updates and log output.
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
//   4. Helper functions that modify the state of the current heating and cooling state
//      based on the accessory state.
//
//   5. The callbacks that implement the actual behavior of the accessory, in this
//      case here they merely access the global accessory state variable and write
//      to the log to make the behavior easily observable.
//
//   6. The initialization of the accessory state.
//
//   7. Callbacks that notify the server in case their associated value has changed.

#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include "HAP.h"

#include "AccessoryInformationServiceDB.h"
#include "App.h"
#include "AppBase.h"
#include "AppLED.h"
#if (HAP_TESTING == 1)
#include "AppUserInput.h"
#endif
#include "ApplicationFeatures.h"
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
#if (HAVE_ACCESSORY_METRICS == 1)
#include "AppThreadSafeMetrics.h"
#include "HAPMetrics.h"
#include "MetricsCapture.h"
#include "MetricsHelper.h"
#include "MetricsServiceDB.h"
#endif
#if (HAVE_UARP_SUPPORT == 1)
#include "UARP.h"
#endif

#if (HAP_APP_USES_HDS_STREAM == 1)
#include "HAPDataStreamProtocols+Stream.h"
#endif
#if (HAVE_DISPLAY == 1)
#include "HAPPlatformAccessorySetupDisplay+Init.h"
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
    AppLEDIdentifier identifyPin;
} Device;

/**
 * Global accessory configuration.
 */
typedef struct {
    struct {
        // Thermostat characteristics
        struct {
            /** This value describes the current mode of a device that supports cooling or heating its environment */
            HAPCharacteristicValue_CurrentHeatingCoolingState current;

            /** This value describes the target mode of a device that supports heating/cooling, e.g., a thermostat. */
            HAPCharacteristicValue_TargetHeatingCoolingState target;
        } heatingCoolingState;

        struct {
            /**
             * This value describes the current temperature of the environment in Celsius irrespective of display units
             * chosen.
             */
            float current;

            /**
             * This characteristic describes the target temperature in Celsius that the device is actively attempting to
             * reach.
             */
            float target;

            /** This characteristic describes units of temperature used for presentation purposes. */
            HAPCharacteristicValue_TemperatureDisplayUnits displayUnits;

            /** This value represents the 'maximum temperature' that must be reached before cooling is turned on. */
            float coolingThreshold;

            /** This value represents the 'minimum temperature' that must be reached before heating is turned on. */
            float heatingThreshold;
        } temperature;

        struct {
            /**
             * This value describes the current relative humidity of the environment that contains the device. The value
             * is expressed in percentage (%).
             */
            float current;

            /**
             * This value describes the target relative humidity that the device is actively attempting to reach. The
             * value is expressed in percentage (%).
             */
            float target;
        } relativeHumidity;

        // Filter Maintenance characteristics
        HAPCharacteristicValue_FilterChangeIndication filterChangeIndication;
#if (HAVE_ACCESSORY_METRICS == 1)
        uint8_t metricsActiveValue;
        HAPMetricsSupportedMetrics supportedMetrics;
#endif
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
        uint32_t diagnosticsSelectedModeState;
#endif // #if (HAVE_DIAGNOSTICS_SERVICE == 1)
    } state;
    Device device;
    HAPPlatformTimerRef identifyTimer;
    HAPAccessoryServer* server;
    HAPPlatformKeyValueStoreRef keyValueStore;
    HAPPlatformTimerRef simulationTimer;
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
    .applicationProtocols =
            (HAPStreamApplicationProtocol* const[]) {
#if (HAVE_UARP_SUPPORT == 1)
                    &streamProtocolUARP,
#endif
                    NULL,
            },
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
void SaveAccessoryState(void);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

#define FIRMWARE_VERSION "1"

/**
 * HomeKit accessory that provides the Thermostat service.
 */
static HAPAccessory accessory = { .aid = 1,
                                  .category = kHAPAccessoryCategory_Thermostats,
                                  .name = "Acme Thermostat",
                                  .productData = "03d8a775e3644573",
                                  .manufacturer = "Acme",
                                  .model = "Thermostat,1",
                                  .serialNumber = "099DB48E9E29",
                                  .firmwareVersion = FIRMWARE_VERSION,
                                  .hardwareVersion = "1",
                                  .services =
                                          (const HAPService* const[]) {
                                                  &accessoryInformationService,
                                                  &hapProtocolInformationService,
                                                  &pairingService,
                                                  &thermostatService,
                                                  &filterMaintenanceService,
#if (HAP_APP_USES_HDS == 1)
                                                  &dataStreamTransportManagementService,
#endif
#if (HAVE_THREAD == 1)
                                                  &threadManagementService,
#endif
#if (HAVE_ACCESSORY_REACHABILITY == 1)
                                                  &accessoryRuntimeInformationService,
#endif
#if (HAVE_FIRMWARE_UPDATE == 1)
                                                  &firmwareUpdateService,
#endif
#if (HAP_TESTING == 1)
                                                  &debugService,
#endif
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
                                                  &diagnosticsService,
#endif
#if (HAVE_ACCESSORY_METRICS == 1)
                                                  &metricsService,
#endif
                                                  NULL,
                                          },
#if (HAP_APP_USES_HDS == 1)
                                  .dataStream.delegate = { .callbacks = &kHAPDataStreamDispatcher_DataStreamCallbacks,
                                                           .context = &dataStreamDispatcher, },
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
#if (HAVE_ACCESSORY_METRICS == 1)
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 0)
static HAPMetricsEventBuffers metricsBuffers[kHAPMetrics_MaxStoredEvents];
#endif
static HAPMetricsContext metricsContext = { .supportedMetricsStorage = &accessoryConfiguration.state.supportedMetrics,
                                            .metricsActiveValueStorage =
                                                    &accessoryConfiguration.state.metricsActiveValue,
                                            .saveAccessoryStateCallback = &SaveAccessoryState,
                                            .bufferFullStateCallback = &MetricsBufferFullStateCallback,
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 0)
                                            .eventBuffers = metricsBuffers,
                                            .numEventBuffers = kHAPMetrics_MaxStoredEvents,
#endif
                                            .accessory = &accessory };
#endif // HAVE_ACCESSORY_METRICS == 1

//----------------------------------------------------------------------------------------------------------------------

/**
 * Macros used in application
 */
#define kThermostat_InitialTempC       20
#define kThermostat_SimulationInterval ((HAPTime)(5 * HAPSecond))

/**
 * Initialize a default accessory state that validates with the device configuration.
 */
static void SetupDefaultAccessoryState(void) {
    accessoryConfiguration.state.heatingCoolingState.current = kHAPCharacteristicValue_CurrentHeatingCoolingState_Off;
    accessoryConfiguration.state.heatingCoolingState.target = kHAPCharacteristicValue_TargetHeatingCoolingState_Off;

    accessoryConfiguration.state.temperature.current = kThermostat_InitialTempC; // Setting initial temperature
    accessoryConfiguration.state.temperature.target = thermostatTemperatureTarget.constraints.minimumValue;
    accessoryConfiguration.state.temperature.displayUnits = kHAPCharacteristicValue_TemperatureDisplayUnits_Celsius;
    accessoryConfiguration.state.temperature.coolingThreshold =
            thermostatTemperatureCoolingThreshold.constraints.maximumValue;
    accessoryConfiguration.state.temperature.heatingThreshold =
            thermostatTemperatureHeatingThreshold.constraints.minimumValue;

    accessoryConfiguration.state.relativeHumidity.current = thermostatRelativeHumidityCurrent.constraints.minimumValue;
    accessoryConfiguration.state.relativeHumidity.target = thermostatRelativeHumidityTarget.constraints.minimumValue;

#if (HAVE_ACCESSORY_METRICS == 1)
    accessoryConfiguration.state.metricsActiveValue = kHAPCharacteristicValue_Active_Inactive;
    MetricsGetDefaultSupportedMetricsConfiguration(&accessoryConfiguration.state.supportedMetrics);
#endif
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
    accessoryConfiguration.state.diagnosticsSelectedModeState = kHAPCharacteristicValue_SelectedDiagnosticsModes_None;
#endif // HAVE_DIAGNOSTICS_SERVICE
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
 * Save the accessory state to persistent memory
 */
void SaveAccessoryState(void) {
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
 * Helper function: returns the absolute value of a float
 */
static float Abs(float value) {
    if (value < 0.0F) {
        return -1.0F * value;
    }
    return value;
}

/**
 * Returns the string associated to 'Current Heating Cooling State'.
 */
static const char* GetCurrentHeatingCoolingStateDescription(HAPCharacteristicValue_CurrentHeatingCoolingState state) {
    switch (state) {
        case kHAPCharacteristicValue_CurrentHeatingCoolingState_Off: {
            return "Off.";
        }
        case kHAPCharacteristicValue_CurrentHeatingCoolingState_Heat: {
            return "Heat. The Heater is currently on.";
        }
        case kHAPCharacteristicValue_CurrentHeatingCoolingState_Cool: {
            return "Cool. Cooler is currently on.";
        }
        default:
            HAPFatalError();
    }
}

/**
 * Returns the string associated to 'Target Heating Cooling State'.
 */
static const char* GetTargetHeatingCoolingStateDescription(HAPCharacteristicValue_TargetHeatingCoolingState state) {
    switch (state) {
        case kHAPCharacteristicValue_TargetHeatingCoolingState_Off: {
            return "Off";
        }
        case kHAPCharacteristicValue_TargetHeatingCoolingState_Heat: {
            return "Heat. If the current temperature is below the target temperature then turn on heating.";
        }
        case kHAPCharacteristicValue_TargetHeatingCoolingState_Cool: {
            return "Cool. If the current temperature is above the target temperature then turn on cooling.";
        }
        case kHAPCharacteristicValue_TargetHeatingCoolingState_Auto: {
            return "Auto. Turn on heating or cooling to maintain temperature within"
                   " the heating and cooling threshold of the target temperature.";
        }
        default:
            HAPFatalError();
    }
}

/**
 * Returns the string associated to 'Temperature Display Units'.
 */
static const char* GetTemperatureDisplayUnitsDescription(HAPCharacteristicValue_TemperatureDisplayUnits units) {
    switch (units) {
        case kHAPCharacteristicValue_TemperatureDisplayUnits_Celsius: {
            return "Celsius";
        }
        case kHAPCharacteristicValue_TemperatureDisplayUnits_Fahrenheit: {
            return "Fahrenheit";
        }
        default:
            HAPFatalError();
    }
}

/**
 * Change the 'Current Heating Cooling State` and notify if the value has changed.
 */
static void ThermostatChangeHeatingCoolingStateCurrent(
        HAPAccessoryServer* server,
        HAPCharacteristicValue_CurrentHeatingCoolingState newValue) {
    HAPPrecondition(server);

    if (newValue != accessoryConfiguration.state.heatingCoolingState.current) {
        HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, GetCurrentHeatingCoolingStateDescription(newValue));

        accessoryConfiguration.state.heatingCoolingState.current = newValue;
        HAPAccessoryServerRaiseEvent(server, &thermostatHeatingCoolingCurrent, &thermostatService, &accessory);
    }
}

/**
 * Disable the thermostat
 */
static void ThermostatOff(HAPAccessoryServer* server) {
    ThermostatChangeHeatingCoolingStateCurrent(server, kHAPCharacteristicValue_CurrentHeatingCoolingState_Off);
}

/**
 * Heat if the current temperature is below the target temperature
 */
static void ThermostatHeat(HAPAccessoryServer* server) {
    if (accessoryConfiguration.state.temperature.current < accessoryConfiguration.state.temperature.target) {
        ThermostatChangeHeatingCoolingStateCurrent(server, kHAPCharacteristicValue_CurrentHeatingCoolingState_Heat);
    } else {
        ThermostatOff(server);
    }
}

/**
 * Cool if the current temperature is above the target temperature
 */
static void ThermostatCool(HAPAccessoryServer* server) {
    if (accessoryConfiguration.state.temperature.current > accessoryConfiguration.state.temperature.target) {
        ThermostatChangeHeatingCoolingStateCurrent(server, kHAPCharacteristicValue_CurrentHeatingCoolingState_Cool);
    } else {
        ThermostatOff(server);
    }
}

/**
 * Auto mode for the thermostat
 * - Heat if the current temperature is below the heating threshold
 * - Cool if the current temperature is above the heating threshold
 */
static void ThermostatAuto(HAPAccessoryServer* server) {
    float currentTemperature = accessoryConfiguration.state.temperature.current;

    if (currentTemperature < accessoryConfiguration.state.temperature.heatingThreshold) {
        ThermostatChangeHeatingCoolingStateCurrent(server, kHAPCharacteristicValue_CurrentHeatingCoolingState_Heat);
    } else if (currentTemperature > accessoryConfiguration.state.temperature.coolingThreshold) {
        ThermostatChangeHeatingCoolingStateCurrent(server, kHAPCharacteristicValue_CurrentHeatingCoolingState_Cool);
    } else {
        ThermostatOff(server);
    }
}

/**
 * Update the accessory state
 */
static void UpdateAccessoryState(HAPAccessoryServer* server) {
    switch (accessoryConfiguration.state.heatingCoolingState.target) {
        case kHAPCharacteristicValue_TargetHeatingCoolingState_Off: {
            ThermostatOff(server);
            break;
        }
        case kHAPCharacteristicValue_TargetHeatingCoolingState_Heat: {
            ThermostatHeat(server);
            break;
        }
        case kHAPCharacteristicValue_TargetHeatingCoolingState_Cool: {
            ThermostatCool(server);
            break;
        }
        case kHAPCharacteristicValue_TargetHeatingCoolingState_Auto: {
            ThermostatAuto(server);
            break;
        }
    }
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
static void EnableSimulationTimer(HAPAccessoryServer* server);
static void DisableSimulationTimer(void);

/**
 * Control duration of Identify indication.
 */
static void IdentifyTimerExpired(HAPPlatformTimerRef timer, void* _Nullable context) {
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

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
 * Handle read request on the 'Current Temperature' characteristic of the Thermostat service
 */
HAP_RESULT_USE_CHECK
HAPError HandleThermostatCurrentTemperatureRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPFloatCharacteristicReadRequest* request HAP_UNUSED,
        float* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.temperature.current;
    HAPLogInfo(&kHAPLog_Default, "%s: %g", __func__, (double) *value);
    return kHAPError_None;
}

/**
 * Handle read request on the 'Target Temperature' characteristic of the Thermostat service
 */
HAP_RESULT_USE_CHECK
HAPError HandleThermostatTargetTemperatureRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPFloatCharacteristicReadRequest* request HAP_UNUSED,
        float* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.temperature.target;
    HAPLogInfo(&kHAPLog_Default, "%s: %g", __func__, (double) *value);
    return kHAPError_None;
}

/**
 * Handle write request on the 'Target Temperature' characteristic of the Thermostat service
 */
HAP_RESULT_USE_CHECK
HAPError HandleThermostatTargetTemperatureWrite(
        HAPAccessoryServer* server,
        const HAPFloatCharacteristicWriteRequest* request,
        float value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s: %g", __func__, (double) value);
    if (Abs(accessoryConfiguration.state.temperature.target - value) > FLT_MIN) {
        accessoryConfiguration.state.temperature.target = value;
        UpdateAccessoryState(server);
        SaveAccessoryState();
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }
    return kHAPError_None;
}

/**
 * Handle read request on the 'Temperature Display Units' characteristic of the Thermostat service
 */
HAP_RESULT_USE_CHECK
HAPError HandleThermostatTemperatureDisplayUnitsRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = (HAPCharacteristicValue_TemperatureDisplayUnits) accessoryConfiguration.state.temperature.displayUnits;
    HAPLogInfo(
            &kHAPLog_Default,
            "%s: %s",
            __func__,
            GetTemperatureDisplayUnitsDescription((HAPCharacteristicValue_TemperatureDisplayUnits) *value));
    return kHAPError_None;
}

/**
 * Handle write request on the 'Temperature Display Units' characteristic of the Thermostat service
 */
HAP_RESULT_USE_CHECK
HAPError HandleThermostatTemperatureDisplayUnitsWrite(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value,
        void* _Nullable context HAP_UNUSED) {
    HAPCharacteristicValue_TemperatureDisplayUnits displayUnits =
            (HAPCharacteristicValue_TemperatureDisplayUnits) value;

    HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, GetTemperatureDisplayUnitsDescription(displayUnits));
    if (accessoryConfiguration.state.temperature.displayUnits != displayUnits) {
        accessoryConfiguration.state.temperature.displayUnits = displayUnits;
        UpdateAccessoryState(server);
        SaveAccessoryState();
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }
    return kHAPError_None;
}

/**
 * Handle read request on the 'Current Heating Cooling State' characteristic of the Thermostat service
 */
HAP_RESULT_USE_CHECK
HAPError HandleThermostatCurrentHeatingCoolingStateRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = (uint8_t) accessoryConfiguration.state.heatingCoolingState.current;
    HAPLogInfo(
            &kHAPLog_Default,
            "%s: %s",
            __func__,
            GetCurrentHeatingCoolingStateDescription((HAPCharacteristicValue_CurrentHeatingCoolingState) *value));
    return kHAPError_None;
}

/**
 * Handle read request on the 'Target Heating Cooling State' characteristic of the Thermostat service
 */
HAP_RESULT_USE_CHECK
HAPError HandleThermostatTargetHeatingCoolingStateRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = (uint8_t) accessoryConfiguration.state.heatingCoolingState.target;
    HAPLogInfo(
            &kHAPLog_Default,
            "%s: %s",
            __func__,
            GetTargetHeatingCoolingStateDescription((HAPCharacteristicValue_TargetHeatingCoolingState) *value));
    return kHAPError_None;
}

/**
 * Handle write request on the 'Target Heating Cooling State"' characteristic of the Thermostat service
 */
HAP_RESULT_USE_CHECK
HAPError HandleThermostatTargetHeatingCoolingStateWrite(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value,
        void* _Nullable context HAP_UNUSED) {
    HAPCharacteristicValue_TargetHeatingCoolingState state = (HAPCharacteristicValue_TargetHeatingCoolingState) value;

    HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, GetTargetHeatingCoolingStateDescription(state));
    if (accessoryConfiguration.state.heatingCoolingState.target != state) {
        accessoryConfiguration.state.heatingCoolingState.target = state;
        UpdateAccessoryState(server);
        SaveAccessoryState();
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }
    return kHAPError_None;
}

/**
 * Handle read request on the 'Cooling Threshold Temperature' characteristic of the Thermostat service
 */
HAP_RESULT_USE_CHECK
HAPError HandleThermostatCoolingThresholdTemperatureRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPFloatCharacteristicReadRequest* request HAP_UNUSED,
        float* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.temperature.coolingThreshold;
    HAPLogInfo(&kHAPLog_Default, "%s: %g", __func__, (double) *value);
    return kHAPError_None;
}

/**
 * Handle write request on the 'Cooling Threshold Temperature' characteristic of the Thermostat service
 */
HAP_RESULT_USE_CHECK
HAPError HandleThermostatCoolingThresholdTemperatureWrite(
        HAPAccessoryServer* server,
        const HAPFloatCharacteristicWriteRequest* request,
        float value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s: %g", __func__, (double) value);
    if (Abs(accessoryConfiguration.state.temperature.coolingThreshold - value) > FLT_MIN) {
        accessoryConfiguration.state.temperature.coolingThreshold = value;
        UpdateAccessoryState(server);
        SaveAccessoryState();
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }
    return kHAPError_None;
}

/**
 * Handle read request on the 'Heating Threshold Temperature' characteristic of the Thermostat service
 */
HAP_RESULT_USE_CHECK
HAPError HandleThermostatHeatingThresholdTemperatureRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPFloatCharacteristicReadRequest* request HAP_UNUSED,
        float* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.temperature.heatingThreshold;
    HAPLogInfo(&kHAPLog_Default, "%s: %g", __func__, (double) *value);
    return kHAPError_None;
}

/**
 * Handle write request on the 'Heating Threshold Temperature' characteristic of the Thermostat service
 */
HAP_RESULT_USE_CHECK
HAPError HandleThermostatHeatingThresholdTemperatureWrite(
        HAPAccessoryServer* server,
        const HAPFloatCharacteristicWriteRequest* request,
        float value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s: %g", __func__, (double) value);
    if (Abs(accessoryConfiguration.state.temperature.heatingThreshold - value) > FLT_MIN) {
        accessoryConfiguration.state.temperature.heatingThreshold = value;
        UpdateAccessoryState(server);
        SaveAccessoryState();
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }
    return kHAPError_None;
}

/**
 * Handle read request on the 'Current Relative Humidity' characteristic of the Thermostat service
 */
HAP_RESULT_USE_CHECK
HAPError HandleThermostatCurrentRelativeHumidityRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPFloatCharacteristicReadRequest* request HAP_UNUSED,
        float* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.relativeHumidity.current;
    HAPLogInfo(&kHAPLog_Default, "%s: %g", __func__, (double) *value);
    return kHAPError_None;
}

/**
 * Handle read request on the 'Target Relative Humidity' characteristic of the Thermostat service
 */
HAP_RESULT_USE_CHECK
HAPError HandleThermostatTargetRelativeHumidityRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPFloatCharacteristicReadRequest* request HAP_UNUSED,
        float* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.relativeHumidity.target;
    HAPLogInfo(&kHAPLog_Default, "%s: %g", __func__, (double) *value);
    return kHAPError_None;
}

/**
 * Handle write request on the 'Target Relative Humidity' characteristic of the Thermostat service
 */
HAP_RESULT_USE_CHECK
HAPError HandleThermostatTargetRelativeHumidityWrite(
        HAPAccessoryServer* server,
        const HAPFloatCharacteristicWriteRequest* request,
        float value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s: %g", __func__, (double) value);
    if (Abs(accessoryConfiguration.state.relativeHumidity.target - value) > FLT_MIN) {
        accessoryConfiguration.state.relativeHumidity.target = value;
        UpdateAccessoryState(server);
        SaveAccessoryState();
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleFilterMaintenanceFilterChangeIndicationRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.filterChangeIndication;

    switch (*value) {
        case kHAPCharacteristicValue_FilterChangeIndication_Ok: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "Ok");
            break;
        }
        case kHAPCharacteristicValue_FilterChangeIndication_Change: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "Change");
            break;
        }
    }

    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

void AppCreate(HAPAccessoryServer* server, HAPPlatformKeyValueStoreRef keyValueStore) {
    HAPPrecondition(server);
    HAPPrecondition(keyValueStore);

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    HAPRawBufferZero(&accessoryConfiguration, sizeof accessoryConfiguration);
    accessoryConfiguration.server = server;
    accessoryConfiguration.keyValueStore = keyValueStore;
    LoadAccessoryState();
#if (HAVE_ACCESSORY_METRICS == 1)
    metricsContext.server = server;
    MetricsInitialize(&metricsContext);
#endif

#if (HAVE_DISPLAY == 1)
    static HAPPlatformAccessorySetupDisplay setupDisplay;
    HAPPlatformAccessorySetupDisplayCreate(
            &setupDisplay, &(const HAPPlatformAccessorySetupDisplayOptions) { .server = server });
    server->platform.setupDisplay = &setupDisplay;
#endif
    ConfigureIO();

#if (HAVE_UARP_SUPPORT == 1)
    UARPInitialize(accessoryConfiguration.server, &accessory);
#endif

#if (HAVE_FIRMWARE_UPDATE == 1)
    FirmwareUpdateOptions fwupOptions = { 0 };
#if (HAP_TESTING == 1)
    fwupOptions.persistStaging = server->firmwareUpdate.persistStaging;
#endif
    FirmwareUpdateInitialize(
            accessoryConfiguration.server, &accessory, accessoryConfiguration.keyValueStore, &fwupOptions);
#endif // HAVE_FIRMWARE_UPDATE

#if (HAP_APP_USES_HDS == 1)
    // Initialize HomeKit Data Stream dispatcher.
    HAPDataStreamDispatcherCreate(
            server,
            &dataStreamDispatcher,
            &(const HAPDataStreamDispatcherOptions) { .storage = &dataStreamDispatcherStorage });
#endif
}

void AppRelease(HAPAccessoryServer* _Nonnull server HAP_UNUSED, void* _Nullable context HAP_UNUSED) {

    UnconfigureIO();

#if (HAVE_ACCESSORY_METRICS == 1)
    MetricsDeinitialize(&metricsContext);
#endif
}

void AppAccessoryServerStart(void) {
    HAPAccessoryServerStart(accessoryConfiguration.server, &accessory);
#if (HAVE_FIRMWARE_UPDATE == 1)
    FirmwareUpdateStart(accessoryConfiguration.server, &accessory);
#endif // HAVE_FIRMWARE_UPDATE
}

//----------------------------------------------------------------------------------------------------------------------

void AppHandleAccessoryServerStateUpdate(HAPAccessoryServer* server, void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);

    switch (HAPAccessoryServerGetState(server)) {
        case kHAPAccessoryServerState_Running: {
            EnableSimulationTimer(accessoryConfiguration.server);
            break;
        }
        case kHAPAccessoryServerState_Stopping: {
            DisableSimulationTimer();
            break;
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

    if (state == kHAPPairingStateChange_Unpaired) {
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
        DiagnosticsHandlePairingStateChange(
                state, &accessoryConfiguration.state.diagnosticsSelectedModeState, &SaveAccessoryState);
#endif // (HAVE_DIAGNOSTICS_SERVICE == 1)
#if (HAVE_ACCESSORY_METRICS == 1)
        MetricsHandlePairingStateChange(
                state,
                &accessoryConfiguration.state.metricsActiveValue,
                &accessoryConfiguration.state.supportedMetrics,
                &SaveAccessoryState);
#endif
    }
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

/**
 * Change the 'Current Temperature` and notify if the value has changed.
 */
static void ThermostatChangeTemperatureCurrent(HAPAccessoryServer* server, float newTemperature) {
    HAPPrecondition(server);

    if (Abs(newTemperature - accessoryConfiguration.state.temperature.current) > FLT_MIN) {
        HAPLogInfo(&kHAPLog_Default, "%s: %g", __func__, (double) newTemperature);

        accessoryConfiguration.state.temperature.current = newTemperature;
        SaveAccessoryState();

        HAPAccessoryServerRaiseEvent(server, &thermostatTemperatureCurrent, &thermostatService, &accessory);
    }
}

/**
 * Change the 'Current Humidity` and notify if the value has changed.
 */
static void ThermostatChangeRelativeHumidityCurrent(HAPAccessoryServer* server, float newRelativeHumidity) {
    HAPPrecondition(server);

    if (Abs(newRelativeHumidity - accessoryConfiguration.state.relativeHumidity.current) > FLT_MIN) {
        HAPLogInfo(&kHAPLog_Default, "%s: %g", __func__, (double) newRelativeHumidity);

        accessoryConfiguration.state.relativeHumidity.current = newRelativeHumidity;
        SaveAccessoryState();

        HAPAccessoryServerRaiseEvent(server, &thermostatRelativeHumidityCurrent, &thermostatService, &accessory);
    }
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
 * Truncate a value to 0.1F accuracy
 */
static float Truncate(float value) {
    return ((int) (10.0F * value + (value >= 0.0F ? 0.5F : -0.5F))) / 10.0F;
}

/**
 * Simple temperature simulation:
 * - Heat if the heater is active
 * - Cool if the cooler is active
 */
static void SimulateTemperature(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    float temperature = accessoryConfiguration.state.temperature.current;
    switch (accessoryConfiguration.state.heatingCoolingState.current) {
        case kHAPCharacteristicValue_CurrentHeatingCoolingState_Heat: {
            temperature += 1.3F;
            break;
        }
        case kHAPCharacteristicValue_CurrentHeatingCoolingState_Cool: {
            temperature -= 1.1F;
            break;
        }
        case kHAPCharacteristicValue_CurrentHeatingCoolingState_Off: {
            // Initial temperature is set to 20C. Without heating or cooling,
            // the temperature will drift to ambient 15C
            if (temperature < 14.9F) {
                temperature += 0.1F;
            } else if (temperature > 15.1F) {
                temperature -= 0.1F;
            }
            break;
        }
        default:
            HAPFatalError();
    }
    temperature = Truncate(temperature);
    temperature =
            Clamp(thermostatTemperatureCurrent.constraints.minimumValue,
                  thermostatTemperatureCurrent.constraints.maximumValue,
                  temperature);
    ThermostatChangeTemperatureCurrent(server, temperature);
}

/**
 * Simulate humidity
 *  - Increase humidity if below threshold
 *  - Decrease humidity if above threshold
 */
static void SimulateHumidity(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    float humidity = accessoryConfiguration.state.relativeHumidity.current;
    const float target = accessoryConfiguration.state.relativeHumidity.target;
    if (humidity < target) {
        humidity += 1.0F;
    } else if (humidity > target) {
        humidity -= 1.0F;
    }
    humidity = Truncate(humidity);
    humidity =
            Clamp(thermostatRelativeHumidityCurrent.constraints.minimumValue,
                  thermostatRelativeHumidityCurrent.constraints.maximumValue,
                  humidity);
    ThermostatChangeRelativeHumidityCurrent(server, humidity);
}

/**
 * Callback for the simulation timer to simulate humidity and temperature on each tick
 */
static void HandleSimulationTimerCallback(HAPPlatformTimerRef timer HAP_UNUSED, void* _Nullable context) {
    HAPPrecondition(context);

    HAPAccessoryServer* server = context;
    accessoryConfiguration.simulationTimer = 0;

    SimulateTemperature(server);
    SimulateHumidity(server);
    UpdateAccessoryState(server);

    // Restart timer
    EnableSimulationTimer(server);
}

/**
 * Callback to enable the simulation via timer.
 */
static void EnableSimulationTimer(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    HAPError err;

    // Timer is already running
    if (accessoryConfiguration.simulationTimer) {
        return;
    }

    err = HAPPlatformTimerRegister(
            &accessoryConfiguration.simulationTimer,
            HAPPlatformClockGetCurrent() + kThermostat_SimulationInterval,
            HandleSimulationTimerCallback,
            server);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPFatalError();
    }
}

/**
 * Callback to disable the simulation via timer.
 */
static void DisableSimulationTimer(void) {
    // Disable if Timer is running
    if (accessoryConfiguration.simulationTimer) {
        HAPPlatformTimerDeregister(accessoryConfiguration.simulationTimer);
    }
}
