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

// Sprinkler Example: Adds Valves as services
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct {
    uint8_t active;
    uint8_t inUse;
    uint32_t setDuration;
    uint32_t remainingDuration;
    uint8_t isConfigured;
    HAPPlatformTimerRef timer;
} Valve;

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
        struct {
            uint8_t active;
            uint8_t programMode;
            uint8_t inUse;
        } sprinkler;
        Valve firstValve;
        Valve secondValve;
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
 * HomeKit accessory that provides the Sprinkler service.
 */
static HAPAccessory accessory = { .aid = 1,
                                  .category = kHAPAccessoryCategory_Sprinklers,
                                  .name = "Sprinkler System",
                                  .productData = "03d8a775e3644573",
                                  .manufacturer = "Uknown",
                                  .model = "Sprinkler1,1",
                                  .serialNumber = "099DB48E9E28",
                                  .firmwareVersion = "1",
                                  .hardwareVersion = "1",
                                  .services =
                                          (const HAPService* const[]) {
                                                  &accessoryInformationService,
                                                  &hapProtocolInformationService,
                                                  &pairingService,
                                                  &sprinklerService,
                                                  &valveService,
                                                  &secondValveService,
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
#if (HAP_TESTING == 1)
                                                  &debugService,
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
#define kValveSetDuration_Interval ((HAPTime)(1 * HAPSecond))

static void EnableValveSetDurationTimer(uint64_t iid);

/**
 * Notify a Valve characteristic based on the IID
 */
static void NotifyValveCharacteristic(uint64_t iid) {
    const HAPService* service = NULL;
    const HAPCharacteristic* characteristic = NULL;

    if (iid == kIID_ValveActive) {
        service = &valveService;
        characteristic = &valveActiveCharacteristic;
    } else if (iid == kIID_SecondValveActive) {
        service = &secondValveService;
        characteristic = &secondValveActiveCharacteristic;
    } else if (iid == kIID_ValveInUse) {
        service = &valveService;
        characteristic = &valveInUseCharacteristic;
    } else if (iid == kIID_SecondValveInUse) {
        service = &secondValveService;
        characteristic = &secondValveInUseCharacteristic;
    } else if (iid == kIID_ValveRemainingDuration) {
        service = &valveService;
        characteristic = &valveRemainingDurationCharacteristic;
    } else if (iid == kIID_SecondValveRemainingDuration) {
        service = &secondValveService;
        characteristic = &secondValveRemainingDurationCharacteristic;
    }

    HAPPrecondition(service);
    HAPPrecondition(characteristic);

    HAPAccessoryServerRaiseEvent(accessoryConfiguration.server, characteristic, service, &accessory);
}

/**
 * Update and notify the 'Active' characterisitic of the specified Valve service if changed
 */
static void UpdateValveActiveCharacteristic(uint64_t iid, uint8_t value) {
    uint8_t* valveActive = NULL;

    if (iid == kIID_Valve) {
        valveActive = &accessoryConfiguration.state.firstValve.active;
        iid = kIID_ValveActive;
    } else if (iid == kIID_SecondValve) {
        valveActive = &accessoryConfiguration.state.secondValve.active;
        iid = kIID_SecondValveActive;
    }

    HAPPrecondition(valveActive);

    if (*valveActive != value) {
        *valveActive = value;
        NotifyValveCharacteristic(iid);
        SaveAccessoryState();
    }
}

/**
 * Update and notify the 'In Use' characterisitic of the specified Valve service if changed
 */
static void UpdateValveInUseCharacteristic(uint64_t iid, uint8_t value) {
    uint8_t* valveInUse = NULL;

    if (iid == kIID_Valve) {
        valveInUse = &accessoryConfiguration.state.firstValve.inUse;
        iid = kIID_ValveInUse;
    } else if (iid == kIID_SecondValve) {
        valveInUse = &accessoryConfiguration.state.secondValve.inUse;
        iid = kIID_SecondValveInUse;
    }

    HAPPrecondition(valveInUse);

    if (*valveInUse != value) {
        *valveInUse = value;
        NotifyValveCharacteristic(iid);
        SaveAccessoryState();
    }
}

/**
 * Update and notify the 'Remaining Duration' characterisitic of the specified Valve service if changed
 */
static void UpdateValveRemainingDurationCharacteristic(uint64_t iid, uint32_t value) {
    uint32_t* valveRemainingDuration = NULL;

    if (iid == kIID_Valve) {
        valveRemainingDuration = &accessoryConfiguration.state.firstValve.remainingDuration;
        iid = kIID_ValveRemainingDuration;
    } else if (iid == kIID_SecondValve) {
        valveRemainingDuration = &accessoryConfiguration.state.secondValve.remainingDuration;
        iid = kIID_SecondValveRemainingDuration;
    }

    HAPPrecondition(valveRemainingDuration);

    if (*valveRemainingDuration != value) {
        *valveRemainingDuration = value;
        NotifyValveCharacteristic(iid);
        SaveAccessoryState();
    }
}

/**
 * Update and notify the Sprinkler characterisitics if changed
 */
static void UpdateSprinklerCharacteristics() {
    uint8_t prevActive = accessoryConfiguration.state.sprinkler.active;
    uint8_t prevInUse = accessoryConfiguration.state.sprinkler.inUse;

    // 'Active' characteristic of Sprinkler service is updated based on:
    //   - If all valves are Inactive, then sprinkler is Inactive
    //   - If any valve is Active, then sprinkler is Active
    if ((accessoryConfiguration.state.firstValve.active == kHAPCharacteristicValue_Active_Inactive) &&
        (accessoryConfiguration.state.secondValve.active == kHAPCharacteristicValue_Active_Inactive)) {
        accessoryConfiguration.state.sprinkler.active = kHAPCharacteristicValue_Active_Inactive;
    } else {
        accessoryConfiguration.state.sprinkler.active = kHAPCharacteristicValue_Active_Active;
    }

    if (prevActive != accessoryConfiguration.state.sprinkler.active) {
        HAPAccessoryServerRaiseEvent(
                accessoryConfiguration.server, &sprinklerActiveCharacteristic, &sprinklerService, &accessory);
        SaveAccessoryState();
    }

    // 'In Use' characteristic of Sprinkler service is updated based on:
    //   - If all valves are NotInUse, then sprinkler is NotInUse
    //   - If any valve is InUse, then sprinkler is InUse
    if ((accessoryConfiguration.state.firstValve.inUse == kHAPCharacteristicValue_InUse_NotInUse) &&
        (accessoryConfiguration.state.secondValve.inUse == kHAPCharacteristicValue_InUse_NotInUse)) {
        accessoryConfiguration.state.sprinkler.inUse = kHAPCharacteristicValue_InUse_NotInUse;
    } else {
        accessoryConfiguration.state.sprinkler.inUse = kHAPCharacteristicValue_InUse_InUse;
    }

    if (prevInUse != accessoryConfiguration.state.sprinkler.inUse) {
        HAPAccessoryServerRaiseEvent(
                accessoryConfiguration.server, &sprinklerInUseCharacteristic, &sprinklerService, &accessory);
        SaveAccessoryState();
    }
}

/**
 * Handle the internal states of the system when the set duration of a valve expires
 */
static void HandleValveSetDurationTimerCallback(HAPPlatformTimerRef timer HAP_UNUSED, void* _Nullable context) {
    HAPPrecondition(context);
    const HAPService* service = (const HAPService*) context;
    uint32_t* valveRemainingDuration = NULL;
    uint64_t valveRemainingDurationIid;

    HAPPrecondition(service);

    if (service->iid == kIID_Valve) {
        valveRemainingDuration = &accessoryConfiguration.state.firstValve.remainingDuration;
        valveRemainingDurationIid = kIID_ValveRemainingDuration;
        accessoryConfiguration.state.firstValve.timer = 0;
    } else if (service->iid == kIID_SecondValve) {
        valveRemainingDuration = &accessoryConfiguration.state.secondValve.remainingDuration;
        valveRemainingDurationIid = kIID_SecondValveRemainingDuration;
        accessoryConfiguration.state.secondValve.timer = 0;
    }

    HAPPrecondition(valveRemainingDuration);

    *valveRemainingDuration -= 1;
    if (*valveRemainingDuration == 0) {
        // Set duration count down reached 0
        NotifyValveCharacteristic(valveRemainingDurationIid);

        UpdateValveInUseCharacteristic(service->iid, kHAPCharacteristicValue_InUse_NotInUse);
        UpdateValveActiveCharacteristic(service->iid, kHAPCharacteristicValue_Active_Inactive);
        UpdateSprinklerCharacteristics();
    } else {
        EnableValveSetDurationTimer(service->iid);
    }
}

/**
 * Enable the valve set duration timer for the specified valve.
 */
static void EnableValveSetDurationTimer(uint64_t iid) {
    const HAPService* service = NULL;
    HAPPlatformTimerRef* valveTimer = NULL;
    HAPError error;

    if (iid == kIID_Valve) {
        service = &valveService;
        valveTimer = &accessoryConfiguration.state.firstValve.timer;
    } else if (iid == kIID_SecondValve) {
        service = &secondValveService;
        valveTimer = &accessoryConfiguration.state.secondValve.timer;
    }

    HAPPrecondition(service);
    HAPPrecondition(valveTimer);

    // Timer is already running
    if (*valveTimer) {
        return;
    }

    error = HAPPlatformTimerRegister(
            valveTimer,
            HAPPlatformClockGetCurrent() + kValveSetDuration_Interval,
            HandleValveSetDurationTimerCallback,
            (void*) service);
    if (error) {
        HAPLogInfo(&kHAPLog_Default, "Unable to start valve set duration timer");
        HAPAssert(error == kHAPError_OutOfResources);
        HAPFatalError();
    }
}

/**
 * Disable the valve set duration timer for the specified valve.
 */
static void DisableValveSetDurationTimer(uint64_t iid) {
    HAPPlatformTimerRef* valveTimer = NULL;

    if (iid == kIID_Valve) {
        valveTimer = &accessoryConfiguration.state.firstValve.timer;
    } else if (iid == kIID_SecondValve) {
        valveTimer = &accessoryConfiguration.state.secondValve.timer;
    }

    HAPPrecondition(valveTimer);

    // Disable if timer is running
    if (*valveTimer) {
        HAPPlatformTimerDeregister(*valveTimer);
        *valveTimer = 0;
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

HAP_RESULT_USE_CHECK
HAPError HandleValveInUseRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = kHAPCharacteristicValue_InUse_NotInUse;

    if (request->characteristic->iid == kIID_ValveInUse) {
        *value = accessoryConfiguration.state.firstValve.inUse;
    } else if (request->characteristic->iid == kIID_SecondValveInUse) {
        *value = accessoryConfiguration.state.secondValve.inUse;
    }

    switch (*value) {
        case kHAPCharacteristicValue_InUse_NotInUse: {
            HAPLogInfo(&kHAPLog_Default, "%s: NotInUse", __func__);
            break;
        }
        case kHAPCharacteristicValue_InUse_InUse: {
            HAPLogInfo(&kHAPLog_Default, "%s: InUse", __func__);
            break;
        }
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleValveIsConfiguredRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = 0;
    if (request->characteristic->iid == kIID_ValveIsConfigured) {
        *value = accessoryConfiguration.state.firstValve.isConfigured;
    } else if (request->characteristic->iid == kIID_SecondValveIsConfigured) {
        *value = accessoryConfiguration.state.secondValve.isConfigured;
    }
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, *value);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleValveIsConfiguredWrite(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicWriteRequest* request HAP_UNUSED,
        uint8_t value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    if (request->characteristic->iid == kIID_ValveIsConfigured) {
        accessoryConfiguration.state.firstValve.isConfigured = value;
    } else if (request->characteristic->iid == kIID_SecondValveIsConfigured) {
        accessoryConfiguration.state.secondValve.isConfigured = value;
    }
    HAPLogInfo(
            &kHAPLog_Default,
            "Configuring valve: %lu. With value: %d",
            (unsigned long) request->characteristic->iid,
            value);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleValveTypeRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = kHAPCharacteristicValue_ValveType_Irrigation;
    HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "Irrigation");
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleValveSetDurationRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt32CharacteristicReadRequest* request HAP_UNUSED,
        uint32_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = valveSetDurationCharacteristic.constraints.minimumValue;
    if (request->characteristic->iid == kIID_ValveSetDuration) {
        *value = accessoryConfiguration.state.firstValve.setDuration;
    } else if (request->characteristic->iid == kIID_SecondValveSetDuration) {
        *value = accessoryConfiguration.state.secondValve.setDuration;
    }
    HAPLogInfo(&kHAPLog_Default, "%s: %lu", __func__, (unsigned long) *value);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleValveSetDurationWrite(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt32CharacteristicWriteRequest* request HAP_UNUSED,
        uint32_t value,
        void* _Nullable context HAP_UNUSED) {
    if (request->characteristic->iid == kIID_ValveSetDuration) {
        accessoryConfiguration.state.firstValve.setDuration = value;
        HAPAccessoryServerRaiseEvent(server, &valveSetDurationCharacteristic, request->service, &accessory);
    } else if (request->characteristic->iid == kIID_SecondValveSetDuration) {
        accessoryConfiguration.state.secondValve.setDuration = value;
        HAPAccessoryServerRaiseEvent(server, &secondValveSetDurationCharacteristic, request->service, &accessory);
    }
    HAPLogInfo(&kHAPLog_Default, "%s: %lu", __func__, (unsigned long) value);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleValveActiveRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = 0;

    if (request->characteristic->iid == kIID_ValveActive) {
        *value = accessoryConfiguration.state.firstValve.active;
    } else if (request->characteristic->iid == kIID_SecondValveActive) {
        *value = accessoryConfiguration.state.secondValve.active;
    }

    switch (*value) {
        case kHAPCharacteristicValue_Active_Inactive: {
            HAPLogInfo(&kHAPLog_Default, "%s: Inactive", __func__);
            break;
        }
        case kHAPCharacteristicValue_Active_Active: {
            HAPLogInfo(&kHAPLog_Default, "%s: Active", __func__);
            break;
        }
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleValveActiveWrite(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicWriteRequest* request HAP_UNUSED,
        uint8_t value,
        void* _Nullable context HAP_UNUSED) {
    Valve* valve = NULL;

    if (request->characteristic->iid == kIID_ValveActive) {
        valve = &accessoryConfiguration.state.firstValve;
    } else if (request->characteristic->iid == kIID_SecondValveActive) {
        valve = &accessoryConfiguration.state.secondValve;
    }

    HAPPrecondition(valve);

    if ((*valve).active != value) {
        (*valve).active = value;
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, &accessory);

        if (value == kHAPCharacteristicValue_Active_Inactive) {
            HAPLogInfo(&kHAPLog_Default, "%s: Inactive", __func__);

            // Water is no longer flowing through the valve
            UpdateValveInUseCharacteristic(request->service->iid, kHAPCharacteristicValue_InUse_NotInUse);
            UpdateValveRemainingDurationCharacteristic(request->service->iid, 0);
            DisableValveSetDurationTimer(request->service->iid);
        } else if (value == kHAPCharacteristicValue_Active_Active) {
            HAPLogInfo(&kHAPLog_Default, "%s: Active", __func__);

            // There must be a valid duration set before valve can be turned on
            if ((*valve).setDuration > 0) {
                // For the purpose of this app, 'Active' directly results in 'In Use' but this behavior should be
                // dictated by the accessory, e.g. if valve 1 is 'Active' and 'In Use' and valve 2 is toggled to
                // 'Active', it may remain 'Not In Use' until valve 1 duration expires
                UpdateValveInUseCharacteristic(request->service->iid, kHAPCharacteristicValue_InUse_InUse);
                UpdateValveRemainingDurationCharacteristic(request->service->iid, (*valve).setDuration);
                EnableValveSetDurationTimer(request->service->iid);
            }
        }

        SaveAccessoryState();
        UpdateSprinklerCharacteristics();
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleValveRemainingDurationRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt32CharacteristicReadRequest* request,
        uint32_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = valveRemainingDurationCharacteristic.constraints.minimumValue;
    if (request->characteristic->iid == kIID_ValveRemainingDuration) {
        *value = accessoryConfiguration.state.firstValve.remainingDuration;
    } else if (request->characteristic->iid == kIID_SecondValveRemainingDuration) {
        *value = accessoryConfiguration.state.secondValve.remainingDuration;
    }
    HAPLogInfo(&kHAPLog_Default, "%s: %lu", __func__, (unsigned long) *value);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleValveServiceLabelIndexRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = 1;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleValveStatusFaultRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value HAP_UNUSED,
        void* _Nullable context HAP_UNUSED) {
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleSprinklerActiveRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.sprinkler.active;

    switch (*value) {
        case kHAPCharacteristicValue_Active_Inactive: {
            HAPLogInfo(&kHAPLog_Default, "%s: Inactive", __func__);
            break;
        }
        case kHAPCharacteristicValue_Active_Active: {
            HAPLogInfo(&kHAPLog_Default, "%s: Active", __func__);
            break;
        }
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleSprinklerActiveWrite(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicWriteRequest* request HAP_UNUSED,
        uint8_t value,
        void* _Nullable context HAP_UNUSED) {
    if (accessoryConfiguration.state.sprinkler.active != value) {
        accessoryConfiguration.state.sprinkler.active = value;
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, &accessory);

        // If the entire system is inactive, then all valves are inactive/not in use
        if (value == kHAPCharacteristicValue_Active_Inactive) {
            // Turn first valve off
            UpdateValveInUseCharacteristic(kIID_Valve, kHAPCharacteristicValue_InUse_NotInUse);
            UpdateValveActiveCharacteristic(kIID_Valve, kHAPCharacteristicValue_Active_Inactive);
            UpdateValveRemainingDurationCharacteristic(kIID_Valve, 0);
            DisableValveSetDurationTimer(kIID_Valve);

            // Turn second valve off
            UpdateValveInUseCharacteristic(kIID_SecondValve, kHAPCharacteristicValue_InUse_NotInUse);
            UpdateValveActiveCharacteristic(kIID_SecondValve, kHAPCharacteristicValue_Active_Inactive);
            UpdateValveRemainingDurationCharacteristic(kIID_SecondValve, 0);
            DisableValveSetDurationTimer(kIID_SecondValve);
        }

        SaveAccessoryState();
        UpdateSprinklerCharacteristics();
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleSprinklerInUseRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.sprinkler.inUse;

    switch (*value) {
        case kHAPCharacteristicValue_InUse_NotInUse: {
            HAPLogInfo(&kHAPLog_Default, "%s: NotInUse", __func__);
            break;
        }
        case kHAPCharacteristicValue_InUse_InUse: {
            HAPLogInfo(&kHAPLog_Default, "%s: InUse", __func__);
            break;
        }
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleSprinklerProgramModeRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.sprinkler.programMode;

    switch (*value) {
        case kHAPCharacteristicValue_ProgramMode_NotScheduled: {
            HAPLogInfo(&kHAPLog_Default, "%s: NotScheduled", __func__);
            break;
        }
        case kHAPCharacteristicValue_ProgramMode_Scheduled: {
            HAPLogInfo(&kHAPLog_Default, "%s: Scheduled", __func__);
            break;
        }
        case kHAPCharacteristicValue_ProgramMode_ScheduleOverriddenToManual: {
            HAPLogInfo(&kHAPLog_Default, "%s: ScheduledOverriddenToManual", __func__);
            break;
        }
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
    accessoryConfiguration.state.sprinkler.programMode = 0; // No program scheduled.
    SaveAccessoryState();

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
