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

// An example that implements a Wi-Fi Router. It can serve as a basic implementation for any platform. The accessory
// logic implementation is reduced to internal state updates and log output.
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
//   1. The definition of the accessory configuration and its internal state.
//
//   2. The callbacks that implement the actual behavior of the accessory.
//
//   3. The signal handlers.
//
//   4. The setup of the accessory attribute database.
//
//   5. The initialization of the accessory state.
//
//   6. A callback that gets called when the server state is updated.

#include <unistd.h>

#include "HAP.h"

#include "HAPPlatform+Init.h"
#include "HAPPlatformWiFiRouter+Init.h"

#include "AccessoryInformationServiceDB.h"
#include "App.h"
#include "AppBase.h"
#include "AppLED.h"
#if (HAP_TESTING == 1)
#include "AppUserInput.h"
#endif
#include "DB.h"
#if (HAP_TESTING == 1)
#include "DebugCommandHandler.h"
#endif
#if (HAP_APP_USES_HDS == 1)
#include "DataStreamTransportManagementServiceDB.h"
#endif
#if (HAVE_FIRMWARE_UPDATE == 1)
#include "FirmwareUpdate.h"
#include "FirmwareUpdateServiceDB.h"
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

/**
 * HomeKit Accessory instance ID of the Wi-Fi router accessory.
 */
#define kAppAID_Router ((size_t) 1)

/**
 * HomeKit Accessory instance ID of the first Wi-Fi satellite accessory.
 */
#define kAppAID_Satellites_Start ((size_t) 2)

static void DisableManagedNetwork(HAPPlatformWiFiRouterRef wiFiRouter);

typedef struct {
    AppLEDIdentifier identifyPin;
} Device;

/**
 * Accessory configuration.
 */
typedef struct {
    HAPAccessoryServer* server;
    HAPPlatformKeyValueStoreRef keyValueStore;
    HAPPlatformWiFiRouterRef wiFiRouter;
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
    struct {
        uint32_t diagnosticsSelectedModeState;
    } state;
#endif // (HAVE_DIAGNOSTICS_SERVICE == 1)
    Device device;
    HAPPlatformTimerRef identifyTimer;
} AccessoryConfiguration;

/**
 * Global accessory configuration.
 */
static AccessoryConfiguration accessoryConfiguration;

#if (HAVE_IP == 1)
static struct { HAPPlatformWiFiRouter wiFiRouter; } appPlatform;
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
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
/**
 * Initialize a default accessory state that validates with the device configuration.
 */
static void SetupDefaultAccessoryState(void) {
    HAPRawBufferZero(&accessoryConfiguration.state, sizeof accessoryConfiguration.state);
    accessoryConfiguration.state.diagnosticsSelectedModeState = kHAPCharacteristicValue_SelectedDiagnosticsModes_None;
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
#endif // (HAVE_DIAGNOSTICS_SERVICE == 1)

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

//----------------------------------------------------------------------------------------------------------------------

/**
 * Handle read request to the 'Configured Name' characteristic of the Wi-Fi Router service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleWiFiRouterConfiguredNameRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPStringCharacteristicReadRequest* request HAP_UNUSED,
        char* value,
        size_t maxValueBytes,
        void* _Nullable context HAP_UNUSED) {
    const char* stringToCopy = "AcmeTestSSID";
    size_t numBytes = HAPStringGetNumBytes(stringToCopy);
    if (numBytes >= maxValueBytes) {
        HAPLog(&kHAPLog_Default,
               "Not enough space to store Configured Name (needed: %zu, available: %zu).",
               numBytes + 1,
               maxValueBytes);
        return kHAPError_OutOfResources;
    }
    HAPRawBufferCopyBytes(value, stringToCopy, numBytes);
    value[numBytes] = '\0';
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

void GenerateOwnershipProofToken(void) {
    HAPAccessoryServerSetOwnershipProofTokenRequired(accessoryConfiguration.server, true);

    HAPAccessorySetupOwnershipProofToken ownershipToken;
    HAPError err = HAPAccessoryServerGenerateOwnershipProofToken(accessoryConfiguration.server, &ownershipToken);
    if (err) {
        HAPAssert(err == kHAPError_InvalidState);
        HAPLogError(&kHAPLog_Default, "Failed to generate ownership proof token for accessory setup.");
        return;
    }

    HAPLogBufferInfo(
            &kHAPLog_Default,
            ownershipToken.bytes,
            sizeof ownershipToken.bytes,
            "Ownership proof token for accessory setup.");
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
 * The Wi-Fi router accessory.
 */
#define accessory routerAccessory
static HAPAccessory
        routerAccessory = { .aid = kAppAID_Router,
                            .category = kHAPAccessoryCategory_WiFiRouters,
                            .name = "Acme Wi-Fi Router",
                            .productData = "03d8a775e3644573",
                            .manufacturer = "Acme",
                            .model = "WiFiRouter1,1",
                            .serialNumber = "099DB48E9E28",
                            .firmwareVersion = "1",
                            .hardwareVersion = "1",
                            .services = (const HAPService* const[]) { &accessoryInformationService,
                                                                      &hapProtocolInformationService,
                                                                      &pairingService,
                                                                      &wiFiRouterService,
#if (HAP_TESTING == 1)
                                                                      &debugService,
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
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
                                    .diagnosticsConfig = { .getDiagnosticsConfig = GetAccessoryDiagnosticsConfig },
#endif
                            } };

/**
 * Array of bridged Wi-FI satellite accessories. NULL-terminated.
 */
static const HAPAccessory* _Nonnull const* _Nullable satelliteAccessories = (const HAPAccessory* const[]) {
    &(const HAPAccessory) {
            .aid = kAppAID_Satellites_Start,
            .category = kHAPAccessoryCategory_BridgedAccessory,
            .name = "Acme Wi-Fi Satellite",
            .manufacturer = "Acme",
            .model = "WiFiSatellite1,1",
            .serialNumber = "099DB48E9E29A",
            .firmwareVersion = "1",
            .hardwareVersion = "1",
            .services =
                    (const HAPService* const[]) { &accessoryInformationServiceBridged, &wiFiSatelliteService, NULL },
            .callbacks = { .identify = IdentifyAccessory } },
    &(const HAPAccessory) {
            .aid = kAppAID_Satellites_Start + 1,
            .category = kHAPAccessoryCategory_BridgedAccessory,
            .name = "Acme Wi-Fi Satellite",
            .manufacturer = "Acme",
            .model = "WiFiSatellite1,1",
            .serialNumber = "099DB48E9E29B",
            .firmwareVersion = "1",
            .hardwareVersion = "1",
            .services =
                    (const HAPService* const[]) { &accessoryInformationServiceBridged, &wiFiSatelliteService, NULL },
            .callbacks = { .identify = IdentifyAccessory } },
    NULL
};

//----------------------------------------------------------------------------------------------------------------------

void AppCreate(HAPAccessoryServer* server, HAPPlatformKeyValueStoreRef keyValueStore) {
    HAPPrecondition(server);
    HAPPrecondition(keyValueStore);

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    HAPRawBufferZero(&accessoryConfiguration, sizeof accessoryConfiguration);
    accessoryConfiguration.server = server;
    accessoryConfiguration.keyValueStore = keyValueStore;
#if (HAVE_IP == 1)
    accessoryConfiguration.wiFiRouter = &appPlatform.wiFiRouter;
#endif

    ConfigureIO();

#if (HAVE_DIAGNOSTICS_SERVICE == 1)
    LoadAccessoryState();
#endif

#if (HAP_APP_USES_HDS == 1)
    // Initialize HomeKit Data Stream dispatcher.
    HAPDataStreamDispatcherCreate(
            server,
            &dataStreamDispatcher,
            &(const HAPDataStreamDispatcherOptions) { .storage = &dataStreamDispatcherStorage });
#endif

#if (HAVE_FIRMWARE_UPDATE == 1)
    UARPInitialize(accessoryConfiguration.server, &routerAccessory);

    FirmwareUpdateOptions fwupOptions = { 0 };
#if (HAP_TESTING == 1)
    fwupOptions.persistStaging = server->firmwareUpdate.persistStaging;
#endif
    FirmwareUpdateInitialize(
            accessoryConfiguration.server, &routerAccessory, accessoryConfiguration.keyValueStore, &fwupOptions);
#endif // HAVE_FIRMWARE_UPDATE
}

void AppRelease(HAPAccessoryServer* _Nonnull server HAP_UNUSED, void* _Nullable context HAP_UNUSED) {
    UnconfigureIO();
}

void AppAccessoryServerStart(void) {
    size_t numSatellites = kAppState_NumSatellites;
    if (numSatellites) {
        size_t numSatelliteAccessories = 0;
        while (satelliteAccessories[numSatelliteAccessories]) {
            numSatelliteAccessories++;
        }
        HAPPrecondition(numSatelliteAccessories == numSatellites);
        HAPAccessoryServerStartBridge(
                accessoryConfiguration.server,
                &routerAccessory,
                satelliteAccessories,
                /* configurationChanged: */ false);
    } else {
        HAPAccessoryServerStart(accessoryConfiguration.server, &routerAccessory);
    }
#if (HAVE_FIRMWARE_UPDATE == 1)
    FirmwareUpdateStart(accessoryConfiguration.server, &accessory);
#endif // HAVE_FIRMWARE_UPDATE
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Tries to disable the managed network.
 *
 * @param      wiFiRouter           Wi-Fi router.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If network configuration access failed.
 * @return kHAPError_Busy           If network configuration is currently unavailable.
 */
HAP_RESULT_USE_CHECK
static HAPError TryDisableManagedNetwork(HAPPlatformWiFiRouterRef wiFiRouter) {
    HAPPrecondition(wiFiRouter);

    HAPError err;

    bool hasSharedConfigurationAccess = false;
    bool hasExclusiveConfigurationAccess = false;

    err = HAPPlatformWiFiRouterAcquireSharedConfigurationAccess(wiFiRouter);
    if (err) {
        HAPAssert(err == kHAPError_Busy);
        HAPLogError(&kHAPLog_Default, "HAPPlatformWiFiRouterAcquireSharedConfigurationAccess failed");
        goto cleanup;
    }
    hasSharedConfigurationAccess = true;
    {
        bool isEnabled;
        err = HAPPlatformWiFiRouterIsManagedNetworkEnabled(wiFiRouter, &isEnabled);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPLogError(&kHAPLog_Default, "HAPPlatformWiFiRouterIsManagedNetworkEnabled failed");
            goto cleanup;
        }
        if (isEnabled) {
            err = HAPPlatformWiFiRouterAcquireExclusiveConfigurationAccess(wiFiRouter);
            if (err) {
                HAPAssert(err == kHAPError_Busy);
                HAPLogError(&kHAPLog_Default, "HAPPlatformWiFiRouterAcquireExclusiveConfigurationAccess failed");
                goto cleanup;
            }
            hasExclusiveConfigurationAccess = true;
            {
                HAPLogInfo(&kHAPLog_Default, "Disabling the managed network.");

                err = HAPPlatformWiFiRouterSetManagedNetworkEnabled(wiFiRouter, !isEnabled);
                if (err) {
                    HAPAssert(err == kHAPError_Unknown);
                    HAPLogError(&kHAPLog_Default, "HAPPlatformWiFiRouterSetManagedNetworkEnabled failed");
                    goto cleanup;
                }
            }
        }
    }

cleanup : {
    if (hasExclusiveConfigurationAccess) {
        HAPPlatformWiFiRouterReleaseExclusiveConfigurationAccess(wiFiRouter);
    }
    if (hasSharedConfigurationAccess) {
        HAPPlatformWiFiRouterReleaseSharedConfigurationAccess(wiFiRouter);
    }
}
    return err;
}

/**
 * Disables the managed network.
 *
 * @param      wiFiRouter           Wi-Fi router.
 */
static void DisableManagedNetwork(HAPPlatformWiFiRouterRef wiFiRouter) {
    HAPPrecondition(wiFiRouter);

    HAPError err;

    uint32_t numRetries = 0;
    for (;;) {
        err = TryDisableManagedNetwork(accessoryConfiguration.wiFiRouter);
        if (err == kHAPError_None) {
            break;
        } else if (err == kHAPError_Unknown) {
            HAPLogError(&kHAPLog_Default, "Network configuration access failed");
            HAPFatalError();
        } else {
            HAPAssert(err == kHAPError_Busy);
            HAPLogError(&kHAPLog_Default, "Network configuration currently unavailable, retrying...");

            unsigned int delay /* seconds */;
            if (numRetries < 1) {
                delay = 0;
            } else if (numRetries < 2) {
                delay = 1;
            } else if (numRetries < 4) {
                delay = 2;
            } else if (numRetries < 8) {
                delay = 4;
            } else if (numRetries < 16) {
                delay = 8;
            } else {
                HAPAssert(numRetries < 32);
                delay = 16;
            }
            do {
                delay = sleep(delay);
            } while (delay);
            numRetries = (numRetries + 1) % 32;
        }
    }
}

void AppHandleAccessoryServerStateUpdate(HAPAccessoryServer* server HAP_UNUSED, void* _Nullable context HAP_UNUSED) {
}

void AppHandleFactoryReset(HAPAccessoryServer* serve HAP_UNUSED, void* _Nullable context HAP_UNUSED) {
    DisableManagedNetwork(accessoryConfiguration.wiFiRouter);
}

void AppHandlePairingStateChange(
        HAPAccessoryServer* server HAP_UNUSED,
        HAPPairingStateChange state,
        void* _Nullable context HAP_UNUSED) {
    switch (state) {
        case kHAPPairingStateChange_Unpaired: {
            DisableManagedNetwork(accessoryConfiguration.wiFiRouter);
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
            DiagnosticsHandlePairingStateChange(
                    state, &accessoryConfiguration.state.diagnosticsSelectedModeState, &SaveAccessoryState);
#endif // (HAVE_DIAGNOSTICS_SERVICE == 1)
            break;
        }
        default: {
            break;
        }
    }
}

const HAPAccessory* AppGetAccessoryInfo(void) {
    return &routerAccessory;
}

const HAPAccessory* _Nonnull const* _Nullable AppGetBridgeAccessoryInfo(void) {
    return satelliteAccessories;
}

void AppInitialize(
        HAPAccessoryServerOptions* hapAccessoryServerOptions HAP_UNUSED,
        HAPPlatform* hapPlatform HAP_UNUSED,
        HAPAccessoryServerCallbacks* hapAccessoryServerCallbacks HAP_UNUSED) {
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
    InitializeDiagnostics(&accessoryConfiguration.state.diagnosticsSelectedModeState, &accessory, hapPlatform);
#endif // (HAVE_DIAGNOSTICS_SERVICE == 1)
#if (HAVE_IP == 1)
    // Wi-Fi router.
    HAPPlatformWiFiRouterCreate(
            &appPlatform.wiFiRouter, &(const HAPPlatformWiFiRouterOptions) { .dbFile = "HAPPlatformWiFiRouter.db" });
    hapPlatform->ip.wiFiRouter = &appPlatform.wiFiRouter;
#if (HAP_TESTING == 1)
    debugCommandLineContext.wiFiRouter = &appPlatform.wiFiRouter;
#endif
#endif
}

void AppDeinitialize(void) {
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
    DeinitializeDiagnostics(&accessory);
#endif // (HAVE_DIAGNOSTICS_SERVICE == 1)
}
