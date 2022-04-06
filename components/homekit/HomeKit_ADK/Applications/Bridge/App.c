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

// Basic bridge example: A bridge accessory that bridges two light bulbs (see kAppState_NumLightBulbs). It can serve
// as a basic implementation for any platform. The accessory logic implementation is reduced to internal state
// updates and log output.
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * Key used in the key value store to store the configuration state.
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
    Device device;
    HAPPlatformTimerRef identifyTimer;
    HAPAccessoryServer* server;
    HAPPlatformKeyValueStoreRef keyValueStore;
} AccessoryConfiguration;

static AccessoryConfiguration accessoryConfiguration;

/**
 * Global bridged accessory state.
 */
typedef struct {
    bool lightBulbOn;
} AccessoryState;

static AccessoryState accessoryState[kAppState_NumLightBulbs] = {
    { .lightBulbOn = true }, // Light bulb 0
    { .lightBulbOn = true }  // Light bulb 1
};

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

/**
 * Obtain the accessory state based on the HAP Accessory ID used.
 */
static AccessoryState* GetAccessoryState(const HAPAccessory* accessoryBridged) {
    HAPPrecondition(accessoryBridged);
    HAPPrecondition(accessoryBridged->aid >= kAppAid_BridgedAccessories_Start);
    HAPPrecondition((size_t)(accessoryBridged->aid - kAppAid_BridgedAccessories_Start) < kAppState_NumLightBulbs);
    return &accessoryState[accessoryBridged->aid - kAppAid_BridgedAccessories_Start];
}

/**
 * Global bridged accessory availability condition.
 */
typedef struct {
    bool reachable;
} AvailabilityCondition;

static AvailabilityCondition availabilityCondition[kAppState_NumLightBulbs] = {
    { .reachable = true }, // Light bulb 0
    { .reachable = true }  // Light bulb 1
};

/**
 * Obtain the accessory availability condition based on the HAP Accessory ID used.
 */
static AvailabilityCondition* GetAccessoryAvailability(const HAPAccessory* accessoryBridged) {
    HAPPrecondition(accessoryBridged);
    HAPPrecondition(accessoryBridged->aid >= kAppAid_BridgedAccessories_Start);
    HAPPrecondition((size_t)(accessoryBridged->aid - kAppAid_BridgedAccessories_Start) < kAppState_NumLightBulbs);
    return &availabilityCondition[accessoryBridged->aid - kAppAid_BridgedAccessories_Start];
}

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
            &accessoryState,
            sizeof accessoryState,
            &numBytes,
            &found);

    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPFatalError();
    }
    if (!found || numBytes != sizeof accessoryState) {
        if (found) {
            HAPLogError(&kHAPLog_Default, "Unexpected app state found in key-value store. Resetting to default.");
        }
        HAPRawBufferZero(&accessoryState, sizeof accessoryState);
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
            &accessoryState,
            sizeof accessoryState);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPFatalError();
    }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * HomeKit accessory that provides the Bridge service.
 */
static HAPAccessory accessory = { .aid = kAppAID_Bridge,
                                  .category = kHAPAccessoryCategory_Bridges,
                                  .name = "Acme Bridge",
                                  .productData = "03d8a775e3644573",
                                  .manufacturer = "Acme",
                                  .model = "Bridge1,1",
                                  .serialNumber = "099DB48E9E28",
                                  .firmwareVersion = "1",
                                  .hardwareVersion = "1",
                                  .services = (const HAPService* const[]) { &accessoryInformationService,
                                                                            &hapProtocolInformationService,
                                                                            &pairingService,
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
                                                                            NULL },
#if (HAP_APP_USES_HDS == 1)
                                  .dataStream.delegate = { .callbacks = &kHAPDataStreamDispatcher_DataStreamCallbacks,
                                                           .context = &dataStreamDispatcher },
#endif
                                  .callbacks = {
                                          .identify = IdentifyAccessory,
#if (HAVE_FIRMWARE_UPDATE == 1)
                                          .firmwareUpdate = { .getAccessoryState = FirmwareUpdateGetAccessoryState },
#endif
                                  } };

/**
 * Array of bridged accessories exposed by the bridge accessory. NULL-terminated.
 */
static const HAPAccessory* _Nonnull const* _Nullable bridgedAccessories = (const HAPAccessory* const[]) {
    &(const HAPAccessory) {
            .aid = kAppAid_BridgedAccessories_Start,
            .category = kHAPAccessoryCategory_BridgedAccessory,
            .name = "Acme Light Bulb",
            .productData = "03d8a775e3644573",
            .manufacturer = "Acme",
            .model = "LightBulb1,1",
            .serialNumber = "099DB48E9E29A",
            .firmwareVersion = "1",
            .hardwareVersion = "1",
            .services = (const HAPService* const[]) { &accessoryInformationServiceBridged, &lightBulbService, NULL },
            .callbacks = { .identify = IdentifyAccessory } },
    &(const HAPAccessory) {
            .aid = kAppAid_BridgedAccessories_Start + 1,
            .category = kHAPAccessoryCategory_BridgedAccessory,
            .name = "Acme Light Bulb",
            .productData = "03d8a775e3644573",
            .manufacturer = "Acme",
            .model = "LightBulb1,1",
            .serialNumber = "099DB48E9E29B",
            .firmwareVersion = "1",
            .hardwareVersion = "1",
            .services = (const HAPService* const[]) { &accessoryInformationServiceBridged, &lightBulbService, NULL },
            .callbacks = { .identify = IdentifyAccessory } },
    NULL
};

#if (HAP_TESTING == 1)

//----------------------------------------------------------------------------------------------------------------------

/**
 * Step through the enumeration of the given two (bridged) accessories' availability states.
 */
static void StepLightBulbsReachable(const HAPAccessory* accessoryBridgedA, const HAPAccessory* accessoryBridgedB) {
    HAPPrecondition(accessoryBridgedA);
    HAPPrecondition(accessoryBridgedB);

    AvailabilityCondition* conditionA = GetAccessoryAvailability(accessoryBridgedA);
    AvailabilityCondition* conditionB = GetAccessoryAvailability(accessoryBridgedB);
    if (conditionB->reachable) {
        conditionA->reachable = !conditionA->reachable;
    }
    conditionB->reachable = !conditionB->reachable;

    HAPLogInfo(
            &kHAPLog_Default,
            "%s light bulb %i: %s light bulb %i: %s",
            __func__,
            (int) (accessoryBridgedA->aid - kAppAid_BridgedAccessories_Start + 1), // First bridged light bulb is 1
            conditionA->reachable ? "here" : "gone",
            (int) (accessoryBridgedB->aid - kAppAid_BridgedAccessories_Start + 1), // Second bridged light bulb is 2
            conditionB->reachable ? "here" : "gone");
}

/**
 * Step through the enumeration of the given two (bridged) light bulbs' on states.
 */
static void StepLightBulbsState(const HAPAccessory* accessoryBridgedA, const HAPAccessory* accessoryBridgedB) {
    HAPPrecondition(accessoryBridgedA);
    HAPPrecondition(accessoryBridgedB);

    AccessoryState* stateA = GetAccessoryState(accessoryBridgedA);
    AccessoryState* stateB = GetAccessoryState(accessoryBridgedB);

    // Enumerate the possible states of the two light bulbs.
    if (stateB->lightBulbOn) {
        stateA->lightBulbOn = !stateA->lightBulbOn;
        HAPAccessoryServerRaiseEvent(
                accessoryConfiguration.server, &lightBulbOnCharacteristic, &lightBulbService, accessoryBridgedA);
    }
    stateB->lightBulbOn = !stateB->lightBulbOn;
    HAPAccessoryServerRaiseEvent(
            accessoryConfiguration.server, &lightBulbOnCharacteristic, &lightBulbService, accessoryBridgedB);

    HAPLogInfo(
            &kHAPLog_Default,
            "%s light bulb %i: %s light bulb %i: %s",
            __func__,
            (int) (accessoryBridgedA->aid - kAppAid_BridgedAccessories_Start + 1), // First bridged light bulb is 1
            stateA->lightBulbOn ? "on " : "off",
            (int) (accessoryBridgedB->aid - kAppAid_BridgedAccessories_Start + 1), // Second bridged light bulb is 2
            stateB->lightBulbOn ? "on " : "off");

    SaveAccessoryState();
}

void EnumerateLightBulbsReachable(void) {
    StepLightBulbsReachable(bridgedAccessories[0], bridgedAccessories[1]);
}

void EnumerateLightBulbsState(void) {
    StepLightBulbsState(bridgedAccessories[0], bridgedAccessories[1]);
}

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
    if (request->accessory->aid == 1) {
        HAPLogDebug(&kHAPLog_Default, "%s: Identifying Bridge.", __func__);
    } else {
        HAPLogDebug(
                &kHAPLog_Default,
                "%s: Identifying Lightbulb [%llu].",
                __func__,
                (unsigned long long) request->accessory->aid);
    }
    /* VENDOR-TODO: Trigger LED/sound notification to identify accessory */
    DeviceIdentify();
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleLightBulbOnRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPBoolCharacteristicReadRequest* request,
        bool* value,
        void* _Nullable context HAP_UNUSED) {
    HAPError error;
    uint64_t lightBulbID = request->accessory->aid - 1;
    AccessoryState* state = GetAccessoryState(request->accessory);
    if (GetAccessoryAvailability(request->accessory)->reachable) {
        *value = state->lightBulbOn;
        HAPLogInfo(
                &kHAPLog_Default,
                "%s: [Light bulb %lu] %s",
                __func__,
                (unsigned long) lightBulbID,
                *value ? "true" : "false");
        error = kHAPError_None;
    } else {
        HAPLogInfo(&kHAPLog_Default, "%s: [Light bulb %lu] %s", __func__, (unsigned long) lightBulbID, "not reachable");
        error = kHAPError_Unknown;
    }
    return error;
}

HAP_RESULT_USE_CHECK
HAPError HandleLightBulbOnWrite(
        HAPAccessoryServer* server,
        const HAPBoolCharacteristicWriteRequest* request,
        bool value,
        void* _Nullable context HAP_UNUSED) {
    HAPError error;
    uint64_t lightBulbID = request->accessory->aid - 1;
    HAPLogInfo(
            &kHAPLog_Default,
            "%s: [Light bulb %lu] %s",
            __func__,
            (unsigned long) lightBulbID,
            value ? "true" : "false");
    AccessoryState* state = GetAccessoryState(request->accessory);
    if (GetAccessoryAvailability(request->accessory)->reachable) {
        if (state->lightBulbOn != value) {
            state->lightBulbOn = value;
            HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
            SaveAccessoryState();
        }
        error = kHAPError_None;
    } else {
        HAPLogInfo(&kHAPLog_Default, "%s: [Light bulb %lu] %s", __func__, (unsigned long) lightBulbID, "not reachable");
        error = kHAPError_Unknown;
    }
    return error;
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
    HAPAccessoryServerStartBridge(
            accessoryConfiguration.server,
            &accessory,
            bridgedAccessories,
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
        HAPAccessoryServerOptions* hapAccessoryServerOptions HAP_UNUSED,
        HAPPlatform* hapPlatform HAP_UNUSED,
        HAPAccessoryServerCallbacks* hapAccessoryServerCallbacks HAP_UNUSED) {
}

void AppDeinitialize(void) {
    /*no-op*/
}
