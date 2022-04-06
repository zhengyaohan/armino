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

// An example that implements a Sensor HomeKit profile. It can serve as a basic implementation for
// any platform. The accessory logic implementation is reduced to internal state updates and log output.
//
// To enable user interaction, buttons on the development board are used:
//
// For Carbon Dioxide Sensor:
// - LED 1 is used to simulate carbon dioxide detected state.
//         ON: Carbon dioxide levels abnormal
//         OFF: Carbon dioxide levels normal
//
// For Carbon Monoxide Sensor:
// - LED 1 is used to simulate carbon monoxide detected state.
//         ON: Carbon monoxide levels abnormal
//         OFF: Carbon monoxide levels normal
//
// For Contact Sensor:
// - LED 1 is used to simulate contact sensor state.
//         ON: Contact is not detected
//         OFF: Contact is detected
//
// For Leak Sensor:
// - LED 1 is used to simulate leak sensor state.
//         ON: Leak is detected
//         OFF: Leak is not detected
//
// For Motion Sensor:
// - LED 1 is used to simulate motion detected state.
//         ON: Motion is detected
//         OFF: Motion is not detected
//
// For Occupancy Sensor:
// - LED 1 is used to simulate occupancy detected state.
//         ON: Occupancy is detected
//         OFF: Occupancy is not detected
//
// For Smoke Sensor:
// - LED 1 is used to simulate smoke detected state.
//         ON: Smoke is detected
//         OFF: Smoke is not detected
//
// - Button 1 or signal `SIGUSR1` to clear pairings.
// - Button 2 or signal `SIGUSR2` to trigger a factory reset.
// - Button 3 or Signal `SIGTERM` to trigger pairing mode.
// - Button 4 or Signal `SIGQUIT` to toggle motion detected state for Motion Sensor.
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
    AppLEDIdentifier ledPin;
    AppLEDIdentifier identifyPin;
} Device;

/**
 * Global accessory configuration.
 */
typedef struct {
    struct {
#if (SENSOR_AIR_QUALITY == 1)
        /**
         * This characteristic describes the subject assessment of air quality by an accessory.
         */
        HAPCharacteristicValue_AirQuality airQuality;
#endif
#if (SENSOR_CARBON_DIOXIDE == 1)
        /**
         * This characteristic indicates if a sensor detects abnormal levels of Carbon Dioxide. Value should revert to 0
         * after the Carbon Dioxide levels drop to normal levels.
         */
        HAPCharacteristicValue_CarbonDioxideDetected carbonDioxideDetected;
#endif
#if (SENSOR_CARBON_MONOXIDE == 1)
        /**
         * This characteristic indicates if a sensor detects abnormal levels of Carbon Monoxide. Value should revert to
         * 0 after the Carbon Monoxide levels drop to normal levels.
         */
        HAPCharacteristicValue_CarbonMonoxideDetected carbonMonoxideDetected;
#endif
#if (SENSOR_CONTACT == 1)
        /**
         * This characteristic describes the state of a door/window contact sensor. A value of 0 indicates that the
         * contact is detected. A value of 1 indicates that the contact is not detected.
         */
        HAPCharacteristicValue_ContactSensorState contactSensorState;
#endif
#if (SENSOR_HUMIDITY == 1)
        /**
         * This characteristic describes the current relative humidity of the accessory’s environment. The value is
         * expressed as a percentage (%).
         */
        float currentRelativeHumidity;
#endif
#if (SENSOR_LEAK == 1)
        /**
         * This characteristic indicates if a sensor detected a leak (e.g., water leak, gas leak). A value of 1
         * indicates that a leak is detected. Value should return to 0 when leak stops.
         */
        HAPCharacteristicValue_LeakDetected leakDetected;
#endif
#if (SENSOR_LIGHT == 1)
        /**
         * This characteristic indicates the current light level. The value is expressed in Lux units (lumens/m2)
         */
        float currentAmbientLightLevel;
#endif
#if (SENSOR_MOTION == 1)
        /**
         * This characteristic indicates if motion (e.g. a person moving) was detected.
         * */
        bool motionDetected;
#endif
#if (SENSOR_OCCUPANCY == 1)
        /**
         * This characteristic indicates if occupancy was detected (e.g. a person present). A value of 1 indicates
         * occupancy is detected. Value should return to 0 when occupancy is not detected.
         * */
        HAPCharacteristicValue_OccupancyDetected occupancyDetected;
#endif
#if (SENSOR_SMOKE == 1)
        /**
         * This characteristic indicates if a sensor detects abnormal levels of smoke. A value of 1 indicates that smoke
         * levels are abnormal. Value should return to 0 when smoke levels are normal.
         * */
        HAPCharacteristicValue_SmokeDetected smokeDetected;
#endif
#if (SENSOR_TEMPERATURE == 1)
        /**
         *This characteristic describes the current temperature of the environment in Celsius irrespective of display
         *units chosen in "Temperature Display Units".
         */
        float currentTemperature;
#endif
        bool statusActive;
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------------------------------------------------

/**
 * Macros used in application
 */
#if (SENSOR_HUMIDITY == 1)
#define kHumiditySensor_InitialRelativeHumidity 50
#endif
#if (SENSOR_LIGHT == 1)
#define kLightSensor_InitialAmbientLightLevel 0.01F
#endif
#if (SENSOR_TEMPERATURE == 1)
#define kTemperatureSensor_InitialTemperature 20
#endif

/**
 * Initialize a default accessory state that validates with the device configuration.
 */
static void SetupDefaultAccessoryState(void) {
#if (SENSOR_AIR_QUALITY == 1)
    accessoryConfiguration.state.airQuality = kHAPCharacteristicValue_AirQuality_Good;
#endif
#if (SENSOR_CARBON_DIOXIDE == 1)
    accessoryConfiguration.state.carbonDioxideDetected = kHAPCharacteristicValue_CarbonDioxideDetected_Normal;
#endif
#if (SENSOR_CARBON_MONOXIDE == 1)
    accessoryConfiguration.state.carbonMonoxideDetected = kHAPCharacteristicValue_CarbonMonoxideDetected_Normal;
#endif
#if (SENSOR_CONTACT == 1)
    accessoryConfiguration.state.contactSensorState = kHAPCharacteristicValue_ContactSensorState_Detected;
#endif
#if (SENSOR_HUMIDITY == 1)
    accessoryConfiguration.state.currentRelativeHumidity = kHumiditySensor_InitialRelativeHumidity;
#endif
#if (SENSOR_LEAK == 1)
    accessoryConfiguration.state.leakDetected = kHAPCharacteristicValue_LeakDetected_NotDetected;
#endif
#if (SENSOR_LIGHT == 1)
    accessoryConfiguration.state.currentAmbientLightLevel = kLightSensor_InitialAmbientLightLevel;
#endif
#if (SENSOR_MOTION == 1)
    accessoryConfiguration.state.motionDetected = false;
#endif
#if (SENSOR_OCCUPANCY == 1)
    accessoryConfiguration.state.occupancyDetected = kHAPCharacteristicValue_OccupancyDetected_NotDetected;
#endif
#if (SENSOR_SMOKE == 1)
    accessoryConfiguration.state.smokeDetected = kHAPCharacteristicValue_SmokeDetected_NotDetected;
#endif
#if (SENSOR_TEMPERATURE == 1)
    accessoryConfiguration.state.currentTemperature = kTemperatureSensor_InitialTemperature;
#endif
    accessoryConfiguration.state.statusActive = true;
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

#if (HAP_TESTING == 1) || (HAVE_DIAGNOSTICS_SERVICE == 1)
#if (SENSOR_AIR_QUALITY == 1) || (SENSOR_CARBON_DIOXIDE == 1) || (SENSOR_CARBON_MONOXIDE == 1) || \
        (SENSOR_CONTACT == 1) || (SENSOR_LEAK == 1) || (SENSOR_MOTION == 1) || (SENSOR_OCCUPANCY == 1) || \
        (SENSOR_SMOKE == 1) || (HAVE_DIAGNOSTICS_SERVICE == 1)
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
#endif
#endif

//----------------------------------------------------------------------------------------------------------------------

/**
 * HomeKit accessory that provides a sensor service.
 */
static HAPAccessory accessory = { .aid = 1,
                                  .category = kHAPAccessoryCategory_Sensors,
#if (SENSOR_AIR_QUALITY == 1)
                                  .name = "Acme Air Quality Sensor",
                                  .model = "AirQualitySensor1,1",
#endif
#if (SENSOR_CARBON_DIOXIDE == 1)
                                  .name = "Acme Carbon Dioxide Sensor",
                                  .model = "CarbonDioxideSensor1,1",
#endif
#if (SENSOR_CARBON_MONOXIDE == 1)
                                  .name = "Acme Carbon Monoxide Sensor",
                                  .model = "CarbonMonoxideSensor1,1",
#endif
#if (SENSOR_CONTACT == 1)
                                  .name = "Acme Contact Sensor",
                                  .model = "ContactSensor1,1",
#endif
#if (SENSOR_HUMIDITY == 1)
                                  .name = "Acme Humidity Sensor",
                                  .model = "HumiditySensor1,1",
#endif
#if (SENSOR_LEAK == 1)
                                  .name = "Acme Leak Sensor",
                                  .model = "LeakSensor1,1",
#endif
#if (SENSOR_LIGHT == 1)
                                  .name = "Acme Light Sensor",
                                  .model = "LightSensor1,1",
#endif
#if (SENSOR_MOTION == 1)
                                  .name = "Acme Motion Sensor",
                                  .model = "MotionSensor1,1",
#endif
#if (SENSOR_OCCUPANCY == 1)
                                  .name = "Acme Occupancy Sensor",
                                  .model = "OccupancySensor1,1",
#endif
#if (SENSOR_SMOKE == 1)
                                  .name = "Acme Smoke Sensor",
                                  .model = "SmokeSensor1,1",
#endif
#if (SENSOR_TEMPERATURE == 1)
                                  .name = "Acme Temperature Sensor",
                                  .model = "TemperatureSensor1,1",
#endif
                                  .productData = "03d8a775e3644573",
                                  .manufacturer = "Acme",
                                  .serialNumber = "099DB48E9E29",
                                  .firmwareVersion = "1",
                                  .hardwareVersion = "1",
                                  .services =
                                          (const HAPService* const[]) {
                                                  &accessoryInformationService,
                                                  &hapProtocolInformationService,
                                                  &pairingService,
#if (SENSOR_AIR_QUALITY == 1)
                                                  &airQualitySensorService,
#endif
#if (SENSOR_CARBON_DIOXIDE == 1)
                                                  &carbonDioxideSensorService,
#endif
#if (SENSOR_CARBON_MONOXIDE == 1)
                                                  &carbonMonoxideSensorService,
#endif
#if (SENSOR_CONTACT == 1)
                                                  &contactSensorService,
#endif
#if (SENSOR_HUMIDITY == 1)
                                                  &humiditySensorService,
#endif
#if (SENSOR_LEAK == 1)
                                                  &leakSensorService,
#endif
#if (SENSOR_LIGHT == 1)
                                                  &lightSensorService,
#endif
#if (SENSOR_MOTION == 1)
                                                  &motionSensorService,
#endif
#if (SENSOR_OCCUPANCY == 1)
                                                  &occupancySensorService,
#endif
#if (SENSOR_SMOKE == 1)
                                                  &smokeSensorService,
#endif
#if (SENSOR_TEMPERATURE == 1)
                                                  &temperatureSensorService,
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
#if (HAP_TESTING == 1)
                                                  &debugService,
#endif
                                                  &batteryService,
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
#if (SENSOR_MOTION == 1)
            ToggleMotionDetectedState();
#endif
            break;
        }
        default: {
            break;
        }
    }
}

#endif

#if (SENSOR_CARBON_DIOXIDE == 1) || (SENSOR_CARBON_MONOXIDE == 1) || (SENSOR_CONTACT == 1) || (SENSOR_LEAK == 1) || \
        (SENSOR_MOTION == 1) || (SENSOR_OCCUPANCY == 1) || (SENSOR_SMOKE == 1)
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
#endif

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
    accessoryConfiguration.device.ledPin = kAppLEDIdentifier_1;
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

#if (SENSOR_AIR_QUALITY == 1)
#if (HAP_TESTING == 1)
void SetAirQualityState(HAPCharacteristicValue_AirQuality newState) {
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, newState);

    if (accessoryConfiguration.state.airQuality != newState) {
        accessoryConfiguration.state.airQuality = newState;
        HAPAccessoryServerRaiseEvent(
                accessoryConfiguration.server,
                &airQualitySensorAirQualityCharacteristic,
                &airQualitySensorService,
                &accessory);
        SaveAccessoryState();
    }
}
#endif

HAP_RESULT_USE_CHECK
HAPError HandleAirQualitySensorAirQualityRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.airQuality;

    switch (*value) {
        case kHAPCharacteristicValue_AirQuality_Unknown: {
            HAPLogInfo(&kHAPLog_Default, "%s: Unknown", __func__);
            break;
        }
        case kHAPCharacteristicValue_AirQuality_Excellent: {
            HAPLogInfo(&kHAPLog_Default, "%s: Excellent", __func__);
            break;
        }
        case kHAPCharacteristicValue_AirQuality_Good: {
            HAPLogInfo(&kHAPLog_Default, "%s: Good", __func__);
            break;
        }
        case kHAPCharacteristicValue_AirQuality_Fair: {
            HAPLogInfo(&kHAPLog_Default, "%s: Fair", __func__);
            break;
        }
        case kHAPCharacteristicValue_AirQuality_Inferior: {
            HAPLogInfo(&kHAPLog_Default, "%s: Inferior", __func__);
            break;
        }
        case kHAPCharacteristicValue_AirQuality_Poor: {
            HAPLogInfo(&kHAPLog_Default, "%s: Poor", __func__);
            break;
        }
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleAirQualitySensorStatusActiveRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPBoolCharacteristicReadRequest* request HAP_UNUSED,
        bool* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.statusActive;
    HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, accessoryConfiguration.state.statusActive ? "true" : "false");
    return kHAPError_None;
}
#endif

#if (SENSOR_CARBON_DIOXIDE == 1)
#if (HAP_TESTING == 1)
void SetCarbonDioxideDetectedState(HAPCharacteristicValue_CarbonDioxideDetected newState) {
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, newState);

    if (accessoryConfiguration.state.carbonDioxideDetected != newState) {
        accessoryConfiguration.state.carbonDioxideDetected = newState;
        HAPAccessoryServerRaiseEvent(
                accessoryConfiguration.server,
                &carbonDioxideSensorCarbonDioxideDetectedCharacteristic,
                &carbonDioxideSensorService,
                &accessory);
        SaveAccessoryState();
    }

    switch (accessoryConfiguration.state.carbonDioxideDetected) {
        case kHAPCharacteristicValue_CarbonDioxideDetected_Normal: {
            HAPLogInfo(&kHAPLog_Default, "%s: Normal", __func__);
            DeviceDisableLED(accessoryConfiguration.device.ledPin);
            break;
        }
        case kHAPCharacteristicValue_CarbonDioxideDetected_Abnormal: {
            HAPLogInfo(&kHAPLog_Default, "%s: Abnormal", __func__);
            DeviceEnableLED(accessoryConfiguration.device.ledPin);
            break;
        }
        default: {
            break;
        }
    }
}
#endif

HAPError HandleCarbonDioxideSensorCarbonDioxideDetectedRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.carbonDioxideDetected;

    switch (*value) {
        case kHAPCharacteristicValue_CarbonDioxideDetected_Normal: {
            HAPLogInfo(&kHAPLog_Default, "%s: Normal", __func__);
            break;
        }
        case kHAPCharacteristicValue_CarbonDioxideDetected_Abnormal: {
            HAPLogInfo(&kHAPLog_Default, "%s: Abnormal", __func__);
            break;
        }
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleCarbonDioxideSensorStatusActiveRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPBoolCharacteristicReadRequest* request HAP_UNUSED,
        bool* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.statusActive;
    HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, accessoryConfiguration.state.statusActive ? "true" : "false");
    return kHAPError_None;
}
#endif

#if (SENSOR_CARBON_MONOXIDE == 1)
#if (HAP_TESTING == 1)
void SetCarbonMonoxideDetectedState(HAPCharacteristicValue_CarbonMonoxideDetected newState) {
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, newState);

    if (accessoryConfiguration.state.carbonMonoxideDetected != newState) {
        accessoryConfiguration.state.carbonMonoxideDetected = newState;
        HAPAccessoryServerRaiseEvent(
                accessoryConfiguration.server,
                &carbonMonoxideSensorCarbonMonoxideDetectedCharacteristic,
                &carbonMonoxideSensorService,
                &accessory);
        SaveAccessoryState();
    }

    switch (accessoryConfiguration.state.carbonMonoxideDetected) {
        case kHAPCharacteristicValue_CarbonMonoxideDetected_Normal: {
            HAPLogInfo(&kHAPLog_Default, "%s: Normal", __func__);
            DeviceDisableLED(accessoryConfiguration.device.ledPin);
            break;
        }
        case kHAPCharacteristicValue_CarbonMonoxideDetected_Abnormal: {
            HAPLogInfo(&kHAPLog_Default, "%s: Abnormal", __func__);
            DeviceEnableLED(accessoryConfiguration.device.ledPin);
            break;
        }
        default: {
            break;
        }
    }
}
#endif

HAPError HandleCarbonMonoxideSensorCarbonMonoxideDetectedRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.carbonMonoxideDetected;

    switch (*value) {
        case kHAPCharacteristicValue_CarbonMonoxideDetected_Normal: {
            HAPLogInfo(&kHAPLog_Default, "%s: Normal", __func__);
            break;
        }
        case kHAPCharacteristicValue_CarbonMonoxideDetected_Abnormal: {
            HAPLogInfo(&kHAPLog_Default, "%s: Abnormal", __func__);
            break;
        }
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleCarbonMonoxideSensorStatusActiveRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPBoolCharacteristicReadRequest* request HAP_UNUSED,
        bool* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.statusActive;
    HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, accessoryConfiguration.state.statusActive ? "true" : "false");
    return kHAPError_None;
}
#endif

#if (SENSOR_CONTACT == 1)
#if (HAP_TESTING == 1)
void SetContactSensorState(HAPCharacteristicValue_ContactSensorState newState) {
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, newState);

    if (accessoryConfiguration.state.contactSensorState != newState) {
        accessoryConfiguration.state.contactSensorState = newState;
        HAPAccessoryServerRaiseEvent(
                accessoryConfiguration.server,
                &contactSensorContactSensorStateCharacteristic,
                &contactSensorService,
                &accessory);
        SaveAccessoryState();
    }

    switch (accessoryConfiguration.state.contactSensorState) {
        case kHAPCharacteristicValue_ContactSensorState_Detected: {
            HAPLogInfo(&kHAPLog_Default, "%s: Detected", __func__);
            DeviceDisableLED(accessoryConfiguration.device.ledPin);
            break;
        }
        case kHAPCharacteristicValue_ContactSensorState_NotDetected: {
            HAPLogInfo(&kHAPLog_Default, "%s: Not detected", __func__);
            DeviceEnableLED(accessoryConfiguration.device.ledPin);
            break;
        }
        default: {
            break;
        }
    }
}
#endif

HAPError HandleContactSensorContactSensorStateRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.contactSensorState;

    switch (*value) {
        case kHAPCharacteristicValue_ContactSensorState_Detected: {
            HAPLogInfo(&kHAPLog_Default, "%s: Detected", __func__);
            break;
        }
        case kHAPCharacteristicValue_ContactSensorState_NotDetected: {
            HAPLogInfo(&kHAPLog_Default, "%s: Not detected", __func__);
            break;
        }
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleContactSensorStatusActiveRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPBoolCharacteristicReadRequest* request HAP_UNUSED,
        bool* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.statusActive;
    HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, accessoryConfiguration.state.statusActive ? "true" : "false");
    return kHAPError_None;
}
#endif

#if (SENSOR_HUMIDITY == 1)
HAP_RESULT_USE_CHECK
HAPError HandleHumiditySensorCurrentRelativeHumidityRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPFloatCharacteristicReadRequest* request HAP_UNUSED,
        float* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.currentRelativeHumidity;
    HAPLogInfo(&kHAPLog_Default, "%s: %g", __func__, *value);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleHumiditySensorStatusActiveRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPBoolCharacteristicReadRequest* request HAP_UNUSED,
        bool* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.statusActive;
    HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, accessoryConfiguration.state.statusActive ? "true" : "false");
    return kHAPError_None;
}
#endif

#if (SENSOR_LEAK == 1)
#if (HAP_TESTING == 1)
void SetLeakDetectedState(HAPCharacteristicValue_LeakDetected newState) {
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, newState);

    if (accessoryConfiguration.state.leakDetected != newState) {
        accessoryConfiguration.state.leakDetected = newState;
        HAPAccessoryServerRaiseEvent(
                accessoryConfiguration.server, &leakSensorLeakDetectedCharacteristic, &leakSensorService, &accessory);
        SaveAccessoryState();
    }

    switch (accessoryConfiguration.state.leakDetected) {
        case kHAPCharacteristicValue_LeakDetected_NotDetected: {
            HAPLogInfo(&kHAPLog_Default, "%s: Not detected", __func__);
            DeviceDisableLED(accessoryConfiguration.device.ledPin);
            break;
        }
        case kHAPCharacteristicValue_LeakDetected_Detected: {
            HAPLogInfo(&kHAPLog_Default, "%s: Detected", __func__);
            DeviceEnableLED(accessoryConfiguration.device.ledPin);
            break;
        }
        default: {
            break;
        }
    }
}
#endif

HAPError HandleLeakSensorLeakDetectedRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.leakDetected;

    switch (*value) {
        case kHAPCharacteristicValue_LeakDetected_NotDetected: {
            HAPLogInfo(&kHAPLog_Default, "%s: Not detected", __func__);
            break;
        }
        case kHAPCharacteristicValue_LeakDetected_Detected: {
            HAPLogInfo(&kHAPLog_Default, "%s: Detected", __func__);
            break;
        }
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleLeakSensorStatusActiveRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPBoolCharacteristicReadRequest* request HAP_UNUSED,
        bool* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.statusActive;
    HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, accessoryConfiguration.state.statusActive ? "true" : "false");
    return kHAPError_None;
}
#endif

#if (SENSOR_LIGHT == 1)
HAP_RESULT_USE_CHECK
HAPError HandleLightSensorCurrentAmbientLightLevelRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPFloatCharacteristicReadRequest* request HAP_UNUSED,
        float* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.currentAmbientLightLevel;
    HAPLogInfo(&kHAPLog_Default, "%s: %g", __func__, *value);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleLightSensorStatusActiveRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPBoolCharacteristicReadRequest* request HAP_UNUSED,
        bool* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.statusActive;
    HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, accessoryConfiguration.state.statusActive ? "true" : "false");
    return kHAPError_None;
}
#endif

#if (SENSOR_MOTION == 1)
#if (HAP_TESTING == 1)
void ToggleMotionDetectedState(void) {
    accessoryConfiguration.state.motionDetected = !accessoryConfiguration.state.motionDetected;

    HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, accessoryConfiguration.state.motionDetected ? "true" : "false");

    if (accessoryConfiguration.state.motionDetected) {
        DeviceEnableLED(accessoryConfiguration.device.ledPin);
    } else {
        DeviceDisableLED(accessoryConfiguration.device.ledPin);
    }

    SaveAccessoryState();

    HAPAccessoryServerRaiseEvent(
            accessoryConfiguration.server, &motionSensorMotionDetectedCharacteristic, &motionSensorService, &accessory);
}
#endif

HAPError HandleMotionSensorMotionDetectedRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPBoolCharacteristicReadRequest* request HAP_UNUSED,
        bool* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.motionDetected;
    HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, accessoryConfiguration.state.motionDetected ? "true" : "false");
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleMotionSensorStatusActiveRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPBoolCharacteristicReadRequest* request HAP_UNUSED,
        bool* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.statusActive;
    HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, accessoryConfiguration.state.statusActive ? "true" : "false");
    return kHAPError_None;
}
#endif

#if (SENSOR_OCCUPANCY == 1)
#if (HAP_TESTING == 1)
void SetOccupancyDetectedState(HAPCharacteristicValue_OccupancyDetected newState) {
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, newState);

    if (accessoryConfiguration.state.occupancyDetected != newState) {
        accessoryConfiguration.state.occupancyDetected = newState;
        HAPAccessoryServerRaiseEvent(
                accessoryConfiguration.server,
                &occupancySensorOccupancyDetectedCharacteristic,
                &occupancySensorService,
                &accessory);
        SaveAccessoryState();
    }

    switch (accessoryConfiguration.state.occupancyDetected) {
        case kHAPCharacteristicValue_OccupancyDetected_NotDetected: {
            HAPLogInfo(&kHAPLog_Default, "%s: Not detected", __func__);
            DeviceDisableLED(accessoryConfiguration.device.ledPin);
            break;
        }
        case kHAPCharacteristicValue_OccupancyDetected_Detected: {
            HAPLogInfo(&kHAPLog_Default, "%s: Detected", __func__);
            DeviceEnableLED(accessoryConfiguration.device.ledPin);
            break;
        }
        default: {
            break;
        }
    }
}
#endif

HAPError HandleOccupancySensorOccupancyDetectedRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.occupancyDetected;

    switch (*value) {
        case kHAPCharacteristicValue_OccupancyDetected_NotDetected: {
            HAPLogInfo(&kHAPLog_Default, "%s: Not detected", __func__);
            break;
        }
        case kHAPCharacteristicValue_OccupancyDetected_Detected: {
            HAPLogInfo(&kHAPLog_Default, "%s: Detected", __func__);
            break;
        }
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleOccupancySensorStatusActiveRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPBoolCharacteristicReadRequest* request HAP_UNUSED,
        bool* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.statusActive;
    HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, accessoryConfiguration.state.statusActive ? "true" : "false");
    return kHAPError_None;
}
#endif

#if (SENSOR_SMOKE == 1)
#if (HAP_TESTING == 1)
void SetSmokeDetectedState(HAPCharacteristicValue_SmokeDetected newState) {
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, newState);

    if (accessoryConfiguration.state.smokeDetected != newState) {
        accessoryConfiguration.state.smokeDetected = newState;
        HAPAccessoryServerRaiseEvent(
                accessoryConfiguration.server,
                &smokeSensorSmokeDetectedCharacteristic,
                &smokeSensorService,
                &accessory);
        SaveAccessoryState();
    }

    switch (accessoryConfiguration.state.smokeDetected) {
        case kHAPCharacteristicValue_SmokeDetected_NotDetected: {
            HAPLogInfo(&kHAPLog_Default, "%s: Not detected", __func__);
            DeviceDisableLED(accessoryConfiguration.device.ledPin);
            break;
        }
        case kHAPCharacteristicValue_SmokeDetected_Detected: {
            HAPLogInfo(&kHAPLog_Default, "%s: Detected", __func__);
            DeviceEnableLED(accessoryConfiguration.device.ledPin);
            break;
        }
        default: {
            break;
        }
    }
}
#endif

HAPError HandleSmokeSensorSmokeDetectedRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.smokeDetected;

    switch (*value) {
        case kHAPCharacteristicValue_SmokeDetected_NotDetected: {
            HAPLogInfo(&kHAPLog_Default, "%s: Not detected", __func__);
            break;
        }
        case kHAPCharacteristicValue_SmokeDetected_Detected: {
            HAPLogInfo(&kHAPLog_Default, "%s: Detected", __func__);
            break;
        }
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleSmokeSensorStatusActiveRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPBoolCharacteristicReadRequest* request HAP_UNUSED,
        bool* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.statusActive;
    HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, accessoryConfiguration.state.statusActive ? "true" : "false");
    return kHAPError_None;
}
#endif

#if (SENSOR_TEMPERATURE == 1)
HAP_RESULT_USE_CHECK
HAPError HandleTemperatureSensorCurrentTemperatureRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPFloatCharacteristicReadRequest* request HAP_UNUSED,
        float* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.currentTemperature;
    HAPLogInfo(&kHAPLog_Default, "%s: %g", __func__, *value);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleTemperatureSensorStatusActiveRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPBoolCharacteristicReadRequest* request HAP_UNUSED,
        bool* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.statusActive;
    HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, accessoryConfiguration.state.statusActive ? "true" : "false");
    return kHAPError_None;
}
#endif

/**
 * Handle read request to the 'Battery Level' characteristic of the Battery service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleBatteryLevelRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    uint8_t batteryLevel = 100;
    // Query the battery sub-system and report battery level
    *value = batteryLevel;
    return kHAPError_None;
}

/**
 * Handle read request to the 'Charging State' characteristic of the Battery service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleBatteryChargingStateRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    // Query the battery sub-system and report battery charging state
    *value = kHAPCharacteristicValue_ChargingState_NotCharging;
    return kHAPError_None;
}

/**
 * Handle read request to the 'Status Low Battery' characteristic of the Battery service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleBatteryStatusLowRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    // Query the battery sub-system and report low battery status
    *value = kHAPCharacteristicValue_StatusLowBattery_Normal;
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

#if (SENSOR_CARBON_DIOXIDE == 1)
    switch (accessoryConfiguration.state.carbonDioxideDetected) {
        case kHAPCharacteristicValue_CarbonDioxideDetected_Normal:
            DeviceDisableLED(accessoryConfiguration.device.ledPin);
            break;
        case kHAPCharacteristicValue_CarbonDioxideDetected_Abnormal:
            DeviceEnableLED(accessoryConfiguration.device.ledPin);
            break;
    }
#endif
#if (SENSOR_CARBON_MONOXIDE == 1)
    switch (accessoryConfiguration.state.carbonMonoxideDetected) {
        case kHAPCharacteristicValue_CarbonMonoxideDetected_Normal:
            DeviceDisableLED(accessoryConfiguration.device.ledPin);
            break;
        case kHAPCharacteristicValue_CarbonMonoxideDetected_Abnormal:
            DeviceEnableLED(accessoryConfiguration.device.ledPin);
            break;
    }
#endif
#if (SENSOR_CONTACT == 1)
    switch (accessoryConfiguration.state.contactSensorState) {
        case kHAPCharacteristicValue_ContactSensorState_Detected:
            DeviceDisableLED(accessoryConfiguration.device.ledPin);
            break;
        case kHAPCharacteristicValue_ContactSensorState_NotDetected:
            DeviceEnableLED(accessoryConfiguration.device.ledPin);
            break;
    }
#endif
#if (SENSOR_LEAK == 1)
    switch (accessoryConfiguration.state.leakDetected) {
        case kHAPCharacteristicValue_LeakDetected_NotDetected:
            DeviceDisableLED(accessoryConfiguration.device.ledPin);
            break;
        case kHAPCharacteristicValue_LeakDetected_Detected:
            DeviceEnableLED(accessoryConfiguration.device.ledPin);
            break;
    }
#endif
#if (SENSOR_MOTION == 1)
    if (accessoryConfiguration.state.motionDetected) {
        DeviceEnableLED(accessoryConfiguration.device.ledPin);
    } else {
        DeviceDisableLED(accessoryConfiguration.device.ledPin);
    }
#endif
#if (SENSOR_OCCUPANCY == 1)
    switch (accessoryConfiguration.state.occupancyDetected) {
        case kHAPCharacteristicValue_OccupancyDetected_NotDetected:
            DeviceDisableLED(accessoryConfiguration.device.ledPin);
            break;
        case kHAPCharacteristicValue_OccupancyDetected_Detected:
            DeviceEnableLED(accessoryConfiguration.device.ledPin);
            break;
    }
#endif
#if (SENSOR_SMOKE == 1)
    switch (accessoryConfiguration.state.smokeDetected) {
        case kHAPCharacteristicValue_SmokeDetected_NotDetected:
            DeviceDisableLED(accessoryConfiguration.device.ledPin);
            break;
        case kHAPCharacteristicValue_SmokeDetected_Detected:
            DeviceEnableLED(accessoryConfiguration.device.ledPin);
            break;
    }
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
