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

// Faucet Example: Heater Cooler and Valve as linked services
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
//   3. The setup of the accessory HomeKit and the bridged accessories attribute database.
//
//   4. Device specific configuration, callbacks and valve services.
//
//   5. Signal handlers.
//
//   6. The callbacks that implement the actual behavior of the accessory. If the
//      accessory state has changed, the corresponding device functions are called.
//
//   7. The initialization of the accessory state.
//
//   8. A callback that gets called when the server state is updated.

#include <stdlib.h>
#include <unistd.h>

#include "HAP.h"

#include "HAPPlatform+Init.h"

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
#define kAppKeyValueStoreKey_Configuration_State ((HAPPlatformKeyValueStoreDomain) 0x00)

#define kHeatingCooling_SimulationInterval ((HAPTime)(5 * HAPSecond))

#define kValveDuration_SimulationInterval ((HAPTime)(1 * HAPSecond))

#define kDefault_Initial_Temperature 20.0F

#define kDefault_RemainingDuration_StepValue 1

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct {
    AppLEDIdentifier identifyPin;
} Device;

/**
 * Global accessory configuration.
 */
/**
 * Required Characteristics:
 * active characteristic
 * in use characteristic
 * valve type (1 ”Irrigation”)
 *
 * Optional Characteristics:
 * set duration
 * remaining duration
 * is configured
 * service label index
 * status fault
 * name
 *
 */
typedef struct {
    struct {
        uint8_t heaterCoolerActive;
        float heaterCoolerCurrentTemperature;
        uint8_t heaterCoolerCurrentState;
        uint8_t heaterCoolerTargetState;
        float heaterCoolerRotationSpeed;
        uint8_t heaterCoolerTemperatureDisplayUnits;
        uint8_t heaterCoolerSwingMode;
        float heaterCoolerHeatingThresholdTemperature;
        float heaterCoolerCoolingThresholdTemperature;
        uint8_t heaterCoolerLockPhysicalControls;
        uint8_t valveActive;
        uint8_t valveInUse;
        uint8_t valveType;
        uint32_t valveSetDuration;
        uint32_t valveRemainingDuration;
        uint8_t valveConfigured;
        uint8_t valveStatusFault;
        uint8_t faucetActive;
        uint8_t faucetStatusFault;
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
        uint32_t diagnosticsSelectedModeState;
#endif // (HAVE_DIAGNOSTICS_SERVICE == 1)
    } state;
    Device device;
    HAPPlatformTimerRef identifyTimer;
    bool restoreFactorySettingsRequested;
    HAPAccessoryServer* server;
    HAPPlatformKeyValueStoreRef keyValueStore;
    HAPPlatformTimerRef heatingCoolingSimulationTimer;
    HAPPlatformTimerRef valveDurationSimulationTimer;
} AccessoryConfiguration;

static AccessoryConfiguration accessoryConfiguration;

#if (HAVE_ACCESSORY_REACHABILITY == 1)
static HAPAccessoryReachabilityConfiguration accessoryReachabilityConfig = { .sleepIntervalInMs =
                                                                                     kAccessorySleepInterval };
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

//----------------------------------------------------------------------------------------------------------------------

/**
 * Initialize a default accessory state that validates with the device configuration.
 */
static void SetupDefaultAccessoryState(void) {
    accessoryConfiguration.state.heaterCoolerCurrentState = kHAPCharacteristicValue_CurrentHeaterCoolerState_Inactive;
    accessoryConfiguration.state.heaterCoolerTargetState = kHAPCharacteristicValue_TargetHeaterCoolerState_HeatOrCool;

    accessoryConfiguration.state.heaterCoolerCurrentTemperature = kDefault_Initial_Temperature;
    accessoryConfiguration.state.heaterCoolerTemperatureDisplayUnits =
            kHAPCharacteristicValue_TemperatureDisplayUnits_Celsius;
    accessoryConfiguration.state.heaterCoolerCoolingThresholdTemperature =
            heaterCoolerCoolingThresholdTemperatureCharacteristic.constraints.minimumValue;
    accessoryConfiguration.state.heaterCoolerHeatingThresholdTemperature =
            heaterCoolerHeatingThresholdTemperatureCharacteristic.constraints.maximumValue;
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
    HAPLogInfo(&kHAPLog_Default, "Saving Accessory State.");
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
 * HomeKit accessory that provides the Faucet service.
 */
static HAPAccessory accessory = { .aid = 1,
                                  .category = kHAPAccessoryCategory_Faucets,
                                  .name = "Acme Faucet",
                                  .productData = "03d8a775e3644573",
                                  .manufacturer = "Acme",
                                  .model = "Faucet1,1",
                                  .serialNumber = "099DB48E9E28",
                                  .firmwareVersion = "1",
                                  .hardwareVersion = "1",
                                  .services =
                                          (const HAPService* const[]) {
                                                  &accessoryInformationService,
                                                  &hapProtocolInformationService,
                                                  &pairingService,
                                                  &heaterCoolerService,
                                                  &valveService,
                                                  &faucetService,
#if (HAP_TESTING == 1)
                                                  &debugService,
#endif
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

void AccessoryNotification(const HAPCharacteristic* characteristic, const HAPService* service) {
    HAPLogInfo(&kHAPLog_Default, "Accessory Notification");

    HAPAccessoryServerRaiseEvent(accessoryConfiguration.server, characteristic, service, &accessory);
}

/**
 * Handle read request on the 'Active' characteristic of the Heater Cooler service
 */
HAP_RESULT_USE_CHECK
HAPError HandleHeaterCoolerActiveRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.heaterCoolerActive;
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, *value);
    return kHAPError_None;
}

/**
 * Handle write request on the 'Active' characteristic of the Heater Cooler service
 */
HAP_RESULT_USE_CHECK
HAPError HandleHeaterCoolerActiveWrite(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "Setting heater cooler active with value: %d", value);
    if (accessoryConfiguration.state.heaterCoolerActive != value) {
        accessoryConfiguration.state.heaterCoolerActive = value;
        SaveAccessoryState();
        AccessoryNotification(request->characteristic, &heaterCoolerService);
    }
    return kHAPError_None;
}

/**
 * Handle read request on the 'Current Temperature' characteristic of the Heater Cooler service
 */
HAP_RESULT_USE_CHECK
HAPError HandleHeaterCoolerCurrentTemperatureRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPFloatCharacteristicReadRequest* request HAP_UNUSED,
        float* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.heaterCoolerCurrentTemperature;
    HAPLogInfo(&kHAPLog_Default, "%s: %g", __func__, (double) *value);
    return kHAPError_None;
}

/**
 * Handle read request on the 'Current State' characteristic of the Heater Cooler service
 */
HAP_RESULT_USE_CHECK
HAPError HandleHeaterCoolerCurrentStateRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.heaterCoolerCurrentState;
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, *value);
    return kHAPError_None;
}

/**
 * Handle read request on the 'Target State' characteristic of the Heater Cooler service
 */
HAP_RESULT_USE_CHECK
HAPError HandleHeaterCoolerTargetStateRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.heaterCoolerTargetState;
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, *value);
    return kHAPError_None;
}

static void HandleHeatingCoolingSimulationTimerCallback(HAPPlatformTimerRef timer HAP_UNUSED, void* _Nullable context);

/**
 * Callback to enable the simulation via timer.
 */
static void EnableHeatingCoolingSimulationTimer(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    // Timer is already running
    if (accessoryConfiguration.heatingCoolingSimulationTimer) {
        return;
    }

    HAPError err = HAPPlatformTimerRegister(
            &accessoryConfiguration.heatingCoolingSimulationTimer,
            HAPPlatformClockGetCurrent() + kHeatingCooling_SimulationInterval,
            HandleHeatingCoolingSimulationTimerCallback,
            server);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPLogInfo(&kHAPLog_Default, "Failed to start heatingCoolingSimulationTimer");
        HAPFatalError();
    }
}

/**
 * Callback to disable the heating cooling simulation timer.
 */
static void DisableHeatingCoolingSimulationTimer(void) {
    // Disable if Timer is running
    if (accessoryConfiguration.heatingCoolingSimulationTimer) {
        HAPPlatformTimerDeregister(accessoryConfiguration.heatingCoolingSimulationTimer);
        accessoryConfiguration.heatingCoolingSimulationTimer = 0;
    }
}

HAPCharacteristicValue_CurrentHeaterCoolerState getCurrentState(HAPCharacteristicValue_TargetHeaterCoolerState state) {
    switch (state) {
        case kHAPCharacteristicValue_TargetHeaterCoolerState_HeatOrCool:
            return kHAPCharacteristicValue_CurrentHeaterCoolerState_Idle;

        case kHAPCharacteristicValue_TargetHeaterCoolerState_Heat:
            return kHAPCharacteristicValue_CurrentHeaterCoolerState_Heating;

        case kHAPCharacteristicValue_TargetHeaterCoolerState_Cool:
            return kHAPCharacteristicValue_CurrentHeaterCoolerState_Cooling;

        default:
            return kHAPCharacteristicValue_CurrentHeaterCoolerState_Inactive;
    }
}

void setTargetState(HAPCharacteristicValue_TargetHeaterCoolerState state) {
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, state);
    DisableHeatingCoolingSimulationTimer();
    accessoryConfiguration.state.heaterCoolerTargetState = state;
    AccessoryNotification(&heaterCoolerTargetStateCharacteristic, &heaterCoolerService);
    accessoryConfiguration.state.heaterCoolerCurrentState =
            getCurrentState(accessoryConfiguration.state.heaterCoolerTargetState);
    AccessoryNotification(&heaterCoolerCurrentStateCharacteristic, &heaterCoolerService);
    SaveAccessoryState();
    EnableHeatingCoolingSimulationTimer(accessoryConfiguration.server);
}

/**
 * Change the 'Current Temperature` and notify if the value has changed.
 */
static void changeCurrentTemperature(HAPAccessoryServer* server, float newTemperature) {
    HAPPrecondition(server);

    HAPLogInfo(&kHAPLog_Default, "%s: %g", __func__, (double) newTemperature);

    accessoryConfiguration.state.heaterCoolerCurrentTemperature = newTemperature;
    SaveAccessoryState();
    AccessoryNotification(&heaterCoolerCurrentTemperatureCharacteristic, &heaterCoolerService);
}

/**
 * Simple temperature simulation:
 * - Heat if the heater is active
 * - Cool if the cooler is active
 * - HeatOrCool - Start heating if current temperature < heating threshold temperature. Start cooling if current
 * temperature > cooling threshold temperature
 */
static void SimulateTemperature(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    float temperature = accessoryConfiguration.state.heaterCoolerCurrentTemperature;
    switch (accessoryConfiguration.state.heaterCoolerTargetState) {
        case kHAPCharacteristicValue_TargetHeaterCoolerState_Heat: {
            if (temperature < accessoryConfiguration.state.heaterCoolerHeatingThresholdTemperature) {
                temperature += 0.5F;
                changeCurrentTemperature(server, temperature);
            }
            if (temperature == accessoryConfiguration.state.heaterCoolerHeatingThresholdTemperature) {
                DisableHeatingCoolingSimulationTimer();
            }
            break;
        }
        case kHAPCharacteristicValue_TargetHeaterCoolerState_Cool: {
            if (temperature > accessoryConfiguration.state.heaterCoolerCoolingThresholdTemperature) {
                temperature -= 0.5F;
                changeCurrentTemperature(server, temperature);
            }
            if (temperature == accessoryConfiguration.state.heaterCoolerCoolingThresholdTemperature) {
                DisableHeatingCoolingSimulationTimer();
            }
            break;
        }
        case kHAPCharacteristicValue_TargetHeaterCoolerState_HeatOrCool: {
            if (temperature > accessoryConfiguration.state.heaterCoolerCoolingThresholdTemperature) {
                setTargetState(kHAPCharacteristicValue_TargetHeaterCoolerState_Cool);
            } else if (temperature < accessoryConfiguration.state.heaterCoolerHeatingThresholdTemperature) {
                setTargetState(kHAPCharacteristicValue_TargetHeaterCoolerState_Heat);
            }
            break;
        }
        default:
            HAPFatalError();
    }
}

/**
 * Callback for the heating cooling simulation timer to simulate temperature on each tick
 */
static void HandleHeatingCoolingSimulationTimerCallback(HAPPlatformTimerRef timer HAP_UNUSED, void* _Nullable context) {
    HAPPrecondition(context);

    HAPAccessoryServer* server = context;
    accessoryConfiguration.heatingCoolingSimulationTimer = 0;

    SimulateTemperature(server);

    // Restart timer
    EnableHeatingCoolingSimulationTimer(server);
}

HAP_RESULT_USE_CHECK
HAPError HandleHeaterCoolerTargetStateWrite(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicWriteRequest* request HAP_UNUSED,
        uint8_t value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, value);
    if (accessoryConfiguration.state.heaterCoolerTargetState != value) {
        accessoryConfiguration.state.heaterCoolerTargetState = value;
        setTargetState(accessoryConfiguration.state.heaterCoolerTargetState);
    }
    return kHAPError_None;
}

/**
 * Handle read request on the 'Rotation Speed' characteristic of the Heater Cooler service
 */
HAP_RESULT_USE_CHECK
HAPError HandleHeaterCoolerRotationSpeedRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPFloatCharacteristicReadRequest* request HAP_UNUSED,
        float* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.heaterCoolerRotationSpeed;
    HAPLogInfo(&kHAPLog_Default, "%s: %g", __func__, (double) *value);
    return kHAPError_None;
}

/**
 * Handle write request on the 'Rotation Speed' characteristic of the Heater Cooler service
 */
HAP_RESULT_USE_CHECK
HAPError HandleHeaterCoolerRotationSpeedWrite(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPFloatCharacteristicWriteRequest* request,
        float value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s: %g", __func__, (double) value);
    if (accessoryConfiguration.state.heaterCoolerRotationSpeed != value) {
        accessoryConfiguration.state.heaterCoolerRotationSpeed = value;
        SaveAccessoryState();
        AccessoryNotification(request->characteristic, &heaterCoolerService);
    }
    return kHAPError_None;
}

/**
 * Handle read request on the 'Temperature Display Units' characteristic of the Heater Cooler service
 */
HAP_RESULT_USE_CHECK
HAPError HandleHeaterCoolerTemperatureDisplayUnitsRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.heaterCoolerTemperatureDisplayUnits;
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, *value);
    return kHAPError_None;
}

/**
 * Handle write request on the 'Temperature Display Units' characteristic of the Heater Cooler service
 */
HAP_RESULT_USE_CHECK
HAPError HandleHeaterCoolerTemperatureDisplayUnitsWrite(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, value);
    if (accessoryConfiguration.state.heaterCoolerTemperatureDisplayUnits != value) {
        accessoryConfiguration.state.heaterCoolerTemperatureDisplayUnits = value;
        SaveAccessoryState();
        AccessoryNotification(request->characteristic, &heaterCoolerService);
    }
    return kHAPError_None;
}

/**
 * Handle read request on the 'Swing Mode' characteristic of the Heater Cooler service
 */
HAP_RESULT_USE_CHECK
HAPError HandleHeaterCoolerSwingModeRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.heaterCoolerSwingMode;
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, *value);
    return kHAPError_None;
}

/**
 * Handle write request on the 'Swing Mode' characteristic of the Heater Cooler service
 */
HAP_RESULT_USE_CHECK
HAPError HandleHeaterCoolerSwingModeWrite(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, value);
    if (accessoryConfiguration.state.heaterCoolerSwingMode != value) {
        accessoryConfiguration.state.heaterCoolerSwingMode = value;
        SaveAccessoryState();
        AccessoryNotification(request->characteristic, &heaterCoolerService);
    }
    return kHAPError_None;
}

/**
 * Handle read request on the 'Heating Threshold Temperature' characteristic of the Heater Cooler service
 */
HAP_RESULT_USE_CHECK
HAPError HandleHeaterCoolerHeatingThresholdTemperatureRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPFloatCharacteristicReadRequest* request HAP_UNUSED,
        float* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.heaterCoolerHeatingThresholdTemperature;
    HAPLogInfo(&kHAPLog_Default, "%s: %g", __func__, (double) *value);
    return kHAPError_None;
}

/**
 * Handle write request on the 'Heating Threshold Temperature' characteristic of the Heater Cooler service
 */
HAP_RESULT_USE_CHECK
HAPError HandleHeaterCoolerHeatingThresholdTemperatureWrite(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPFloatCharacteristicWriteRequest* request,
        float value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s: %g", __func__, (double) value);
    if (accessoryConfiguration.state.heaterCoolerHeatingThresholdTemperature != value) {
        accessoryConfiguration.state.heaterCoolerHeatingThresholdTemperature = value;
        AccessoryNotification(request->characteristic, &heaterCoolerService);
        if (accessoryConfiguration.state.heaterCoolerCurrentState ==
                    kHAPCharacteristicValue_TargetHeaterCoolerState_HeatOrCool &&
            accessoryConfiguration.state.heaterCoolerCurrentTemperature < value) {
            setTargetState(kHAPCharacteristicValue_TargetHeaterCoolerState_Heat);
        }
        SaveAccessoryState();
    }
    return kHAPError_None;
}

/**
 * Handle read request on the 'Cooling Threshold Temperature' characteristic of the Heater Cooler service
 */
HAP_RESULT_USE_CHECK
HAPError HandleHeaterCoolerCoolingThresholdTemperatureRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPFloatCharacteristicReadRequest* request HAP_UNUSED,
        float* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.heaterCoolerCoolingThresholdTemperature;
    HAPLogInfo(&kHAPLog_Default, "%s: %g", __func__, (double) *value);
    return kHAPError_None;
}

/**
 * Handle write request on the 'Cooling Threshold Temperature' characteristic of the Heater Cooler service
 */
HAP_RESULT_USE_CHECK
HAPError HandleHeaterCoolerCoolingThresholdTemperatureWrite(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPFloatCharacteristicWriteRequest* request,
        float value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s: %g", __func__, (double) value);
    if (accessoryConfiguration.state.heaterCoolerCoolingThresholdTemperature != value) {
        accessoryConfiguration.state.heaterCoolerCoolingThresholdTemperature = value;
        AccessoryNotification(request->characteristic, &heaterCoolerService);
        if (accessoryConfiguration.state.heaterCoolerCurrentState ==
                    kHAPCharacteristicValue_TargetHeaterCoolerState_HeatOrCool &&
            accessoryConfiguration.state.heaterCoolerCurrentTemperature > value) {
            setTargetState(kHAPCharacteristicValue_TargetHeaterCoolerState_Cool);
        }
        SaveAccessoryState();
    }
    return kHAPError_None;
}

/**
 * Handle read request on the 'Lock Physical Controls' characteristic of the Heater Cooler service
 */
HAP_RESULT_USE_CHECK
HAPError HandleHeaterCoolerLockPhysicalControlsRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.heaterCoolerLockPhysicalControls;
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, *value);
    return kHAPError_None;
}

/**
 * Handle write request on the 'Lock Physical Controls' characteristic of the Heater Cooler service
 */
HAP_RESULT_USE_CHECK
HAPError HandleHeaterCoolerLockPhysicalControlsWrite(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, value);
    if (accessoryConfiguration.state.heaterCoolerLockPhysicalControls != value) {
        accessoryConfiguration.state.heaterCoolerLockPhysicalControls = value;
        SaveAccessoryState();
        AccessoryNotification(request->characteristic, &heaterCoolerService);
    }
    return kHAPError_None;
}

/**
 * Handle read request on the 'Active' characteristic of the Valve service
 */
HAP_RESULT_USE_CHECK
HAPError HandleValveActiveRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.valveActive;
    HAPLogInfo(
            &kHAPLog_Default, "%s: %s", __func__, *value == kHAPCharacteristicValue_Active_Active ? "true" : "false");
    return kHAPError_None;
}

static void EnableValveDurationSimulationTimer(HAPAccessoryServer* server);

/**
 * Sets the valve "In Use" characteristics when valve active is set to true and set duration is greater than 0. Enables
 * the valve duration simulation timer and starts the countdown for the duration after which "In Use" is set to false.
 */
void HandleSetDuration(HAPAccessoryServer* server) {
    if (accessoryConfiguration.state.valveSetDuration > 0) {
        accessoryConfiguration.state.valveInUse = true;
        AccessoryNotification(&valveInUseCharacteristic, &valveService);
        accessoryConfiguration.state.valveRemainingDuration = accessoryConfiguration.state.valveSetDuration;
        EnableValveDurationSimulationTimer(server);
    }
}

/**
 * Callback to disable the valve duration simulation timer.
 */
static void DisableValveDurationSimulationTimer(void) {
    // Disable if Timer is running
    if (accessoryConfiguration.valveDurationSimulationTimer) {
        HAPPlatformTimerDeregister(accessoryConfiguration.valveDurationSimulationTimer);
        accessoryConfiguration.valveDurationSimulationTimer = 0;
    }
}

/**
 * Handle write request on the 'Active' characteristic of the Valve service
 */
HAP_RESULT_USE_CHECK
HAPError HandleValveActiveWrite(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, value);
    if (accessoryConfiguration.state.valveActive != value) {
        accessoryConfiguration.state.valveActive = value;
        AccessoryNotification(request->characteristic, &valveService);

        if (accessoryConfiguration.state.valveActive == kHAPCharacteristicValue_Active_Active) {
            HandleSetDuration(server);
        } else {
            DisableValveDurationSimulationTimer();
            accessoryConfiguration.state.valveInUse = false;
            AccessoryNotification(&valveInUseCharacteristic, &valveService);
            accessoryConfiguration.state.valveRemainingDuration = 0;
            AccessoryNotification(&valveRemainingDurationCharacteristic, &valveService);
        }
        SaveAccessoryState();
    }
    return kHAPError_None;
}

/**
 * Handle read request on the 'In Use' characteristic of the Valve service
 */
HAP_RESULT_USE_CHECK
HAPError HandleValveInUseRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.valveInUse;
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, *value);
    return kHAPError_None;
}

/**
 * Handle read request on the 'Is Configured' characteristic of the Valve service
 */
HAP_RESULT_USE_CHECK
HAPError HandleValveIsConfiguredRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.valveConfigured;
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, *value);
    return kHAPError_None;
}

/**
 * Handle write request on the 'Is Configured' characteristic of the Valve service
 */
HAP_RESULT_USE_CHECK
HAPError HandleValveIsConfiguredWrite(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicWriteRequest* request HAP_UNUSED,
        uint8_t value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, value);
    if (accessoryConfiguration.state.valveConfigured != value) {
        accessoryConfiguration.state.valveConfigured = value;
        SaveAccessoryState();
        AccessoryNotification(request->characteristic, &valveService);
    }
    return kHAPError_None;
}

/**
 * Handle read request on the 'Valve Type' characteristic of the Valve service
 */
HAP_RESULT_USE_CHECK
HAPError HandleValveTypeRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = kHAPCharacteristicValue_ValveType_WaterFaucet; // This indicates that the valve is a water faucet valve.
    HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "Water Faucet");
    return kHAPError_None;
}

/**
 * Handle read request on the 'Set Duration' characteristic of the Valve service
 */
HAP_RESULT_USE_CHECK
HAPError HandleValveSetDurationRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt32CharacteristicReadRequest* request HAP_UNUSED,
        uint32_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.valveSetDuration;
    HAPLogInfo(&kHAPLog_Default, "%s: %lu", __func__, (unsigned long) *value);
    return kHAPError_None;
}

/**
 * Callback for the valve duration countdown simulation timer to set the valveInUse characteristics of the valve for a
 * set period of time and reduce the remaining duration
 */
static void HandleValveDurationSimulationTimerCallback(
        HAPPlatformTimerRef timer HAP_UNUSED,
        void* _Nullable context HAP_UNUSED) {
    accessoryConfiguration.valveDurationSimulationTimer = 0;
    accessoryConfiguration.state.valveRemainingDuration -= 1;

    if (accessoryConfiguration.state.valveRemainingDuration == 0) {
        AccessoryNotification(&valveRemainingDurationCharacteristic, &valveService);
        accessoryConfiguration.state.valveInUse = false;
        AccessoryNotification(&valveInUseCharacteristic, &valveService);
        accessoryConfiguration.state.valveActive = kHAPCharacteristicValue_Active_Inactive;
        AccessoryNotification(&valveActiveCharacteristic, &valveService);
        SaveAccessoryState();

    } else {
        HAPError err = HAPPlatformTimerRegister(
                &accessoryConfiguration.valveDurationSimulationTimer,
                HAPPlatformClockGetCurrent() + kValveDuration_SimulationInterval,
                HandleValveDurationSimulationTimerCallback,
                NULL);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            HAPLogInfo(&kHAPLog_Default, "Failed to start valveDurationSimulationTimer");
            HAPFatalError();
        }
    }
}

/**
 * Callback to enable the set duration simulation timer to set the valveInUse characteristics of the valve
 */
static void EnableValveDurationSimulationTimer(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    // Timer is already running
    if (accessoryConfiguration.valveDurationSimulationTimer) {
        return;
    }

    HAPError err = HAPPlatformTimerRegister(
            &accessoryConfiguration.valveDurationSimulationTimer,
            HAPPlatformClockGetCurrent() + kValveDuration_SimulationInterval,
            HandleValveDurationSimulationTimerCallback,
            NULL);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPLogInfo(&kHAPLog_Default, "Failed to start valveDurationSimulationTimer");
        HAPFatalError();
    }
}

/**
 * Handle write request on the 'Set Duration' characteristic of the Valve service
 */
HAP_RESULT_USE_CHECK
HAPError HandleValveSetDurationWrite(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt32CharacteristicWriteRequest* request HAP_UNUSED,
        uint32_t value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s: %lu", __func__, (unsigned long) value);
    accessoryConfiguration.state.valveSetDuration = value;
    SaveAccessoryState();
    AccessoryNotification(request->characteristic, &valveService);
    return kHAPError_None;
}

/**
 * Handle read request on the 'Remaining Duration' characteristic of the Valve service
 */
HAP_RESULT_USE_CHECK
HAPError HandleValveRemainingDurationRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt32CharacteristicReadRequest* request HAP_UNUSED,
        uint32_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.valveRemainingDuration;
    HAPLogInfo(&kHAPLog_Default, "%s: %lu", __func__, (unsigned long) *value);
    return kHAPError_None;
}

/**
 * Handle read request on the 'Service Label Index' characteristic of the Valve service
 */
HAP_RESULT_USE_CHECK
HAPError HandleValveServiceLabelIndexRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = 1;
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, *value);
    return kHAPError_None;
}

/**
 * Handle read request on the 'Status Fault' characteristic of the Valve service
 */
HAP_RESULT_USE_CHECK
HAPError HandleValveStatusFaultRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.valveStatusFault;
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, *value);
    return kHAPError_None;
}

/**
 * Handle read request on the 'Active' characteristic of the Faucet service
 */
HAP_RESULT_USE_CHECK
HAPError HandleFaucetActiveRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.faucetActive;
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, *value);
    return kHAPError_None;
}

/**
 * Handle write request on the 'Active' characteristic of the Faucet service
 */
HAP_RESULT_USE_CHECK
HAPError HandleFaucetActiveWrite(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, value);
    if (accessoryConfiguration.state.faucetActive != value) {
        accessoryConfiguration.state.faucetActive = value;
        if (accessoryConfiguration.state.faucetActive == kHAPCharacteristicValue_Active_Inactive) {
            HAPLogInfo(
                    &kHAPLog_Default,
                    "Setting heater cooler active with value: %d",
                    kHAPCharacteristicValue_Active_Inactive);
            accessoryConfiguration.state.heaterCoolerActive = kHAPCharacteristicValue_Active_Inactive;
            AccessoryNotification(&heaterCoolerActiveCharacteristic, &heaterCoolerService);
        }
        SaveAccessoryState();
        AccessoryNotification(request->characteristic, &valveService);
    }
    return kHAPError_None;
}

/**
 * Handle read request on the 'Status Fault' characteristic of the Faucet service
 */
HAP_RESULT_USE_CHECK
HAPError HandleFaucetStatusFaultRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.faucetStatusFault;
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, *value);
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

#if (HAP_TESTING == 1)

void SetRemainingDuration(HAPAccessoryServer* server, uint32_t duration) {
    HAPPrecondition(server);
    uint32_t diff = (accessoryConfiguration.state.valveRemainingDuration > duration) ?
                            (accessoryConfiguration.state.valveRemainingDuration - duration) :
                            (duration - accessoryConfiguration.state.valveRemainingDuration);
    accessoryConfiguration.state.valveRemainingDuration = duration;
    SaveAccessoryState();
    if (diff > kDefault_RemainingDuration_StepValue) {
        AccessoryNotification(&valveRemainingDurationCharacteristic, &valveService);
    }
}

void ToggleFaucetStatusFault(void) {
    switch (accessoryConfiguration.state.faucetStatusFault) {
        case kHAPCharacteristicValue_StatusFault_None: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "StatusFault_General");
            accessoryConfiguration.state.faucetStatusFault = kHAPCharacteristicValue_StatusFault_General;
            break;
        }
        case kHAPCharacteristicValue_StatusFault_General: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "None");
            accessoryConfiguration.state.faucetStatusFault = kHAPCharacteristicValue_StatusFault_None;
            break;
        }
    }
    SaveAccessoryState();
    AccessoryNotification(&faucetStatusFaultCharacteristic, &faucetService);
}

void ToggleValveStatusFault(void) {
    switch (accessoryConfiguration.state.valveStatusFault) {
        case kHAPCharacteristicValue_StatusFault_None: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "StatusFault_General");
            accessoryConfiguration.state.valveStatusFault = kHAPCharacteristicValue_StatusFault_General;
            break;
        }
        case kHAPCharacteristicValue_StatusFault_General: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "None");
            accessoryConfiguration.state.valveStatusFault = kHAPCharacteristicValue_StatusFault_None;
            break;
        }
    }
    SaveAccessoryState();
    AccessoryNotification(&valveStatusFaultCharacteristic, &valveService);
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

void AppCreate(HAPAccessoryServer* server, HAPPlatformKeyValueStoreRef keyValueStore) {
    HAPPrecondition(server);
    HAPPrecondition(keyValueStore);

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    HAPRawBufferZero(&accessoryConfiguration, sizeof accessoryConfiguration);
    accessoryConfiguration.server = server;
    accessoryConfiguration.keyValueStore = keyValueStore;

    ConfigureIO();

    LoadAccessoryState();

#if (HAP_TESTING == 1)
    // Setup user input event handler
    AppUserInputRegisterCallback(HandleUserInputEventCallback);
#endif

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
    HAPPrecondition(server);

    switch (HAPAccessoryServerGetState(server)) {
        case kHAPAccessoryServerState_Stopping: {
            DisableHeatingCoolingSimulationTimer();
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
