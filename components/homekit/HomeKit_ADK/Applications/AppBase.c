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

/**
 * This file is used as a common base for all ADK applications to initialize
 * the platform and common protocols.
 * This file must be compiled separately for each application due to static
 * object allocation based on the attribute count of each profile.
 */

#if (HAP_TESTING == 1)
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include "stdio.h"
#include "string.h"
#endif
#include "mem_pub.h"//for os_malloc,os_free

// This need to be here to include some of the app specific macros defined in App.h of an application
#include "App.h"
#include "ApplicationFeatures.h"

#include "HAP.h"
#include "HAPPairingKVS.h"

#include "HAPPlatform+Init.h"
#include "HAPPlatformAccessorySetup+Init.h"
#include "HAPPlatformBLEPeripheralManager+Init.h"
#include "HAPPlatformKeyValueStore+Init.h"
#if (HAP_TESTING == 1)
#include "DebugCommandHandler.h"
#if HAP_FEATURE_ENABLED(HAP_FEATURE_FILE_STORAGE)
#include "DebugCommandFileMonitor.h"
#endif
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_FILE_STORAGE)
#include "HAPPlatformFileManager.h"
#endif

#if (HAVE_MFI_HW_AUTH == 1)
#include "HAPPlatformMFiHWAuth+Init.h"
#endif
#if (HAVE_MFI_TOKEN_AUTH == 1)
#include "HAPPlatformMFiTokenAuth+Init.h"
#endif
#include "HAPPlatformRunLoop+Init.h"
#include "HAPPlatformTimer.h"
#if (HAVE_IP == 1)
#include "HAPPlatformServiceDiscovery+Init.h"
#include "HAPPlatformSoftwareAccessPoint+Init.h"
#include "HAPPlatformTCPStreamManager+Init.h"
#include "HAPPlatformWiFiManager+Init.h"
#endif
#if (HAVE_NFC == 1)
#include "HAPPlatformAccessorySetupNFC+Init.h"
#endif
#include "HAPPlatformSetup+Init.h"
#if (HAVE_THREAD == 1)
#include "HAPPlatformServiceDiscovery+Init.h"
#include "HAPPlatformThreadCoAPManager+Init.h"
#include "HAPPlatformThreadUtils+Init.h"
#endif

#include "AppBase.h"
#include "AppUserInput.h"
#include "DB.h"
#if (HAP_TESTING == 1)
#include "AttributeCheck.h"
#include "DebugCommand.h"
#endif
#if (HAVE_THREAD == 1)
#include "ThreadManagementServiceDB.h"
#include "ThreadReattachHelper.h"
#endif

#if (HAVE_GATEWAY_PING == 1)
#include "GatewayPing.h"
#endif // (HAVE_GATEWAY_PING == 1)

static bool requestedFactoryReset = false;
static bool requestedFullStop = false;
static bool clearPairings = false;
static bool isReadyToPerformFactoryReset = false;

#define PREFERRED_ADVERTISING_INTERVAL (HAPBLEAdvertisingIntervalCreateFromMilliseconds(417.5f))

#if (HAVE_THREAD == 1)

/** Maximum number of CoAP resources to support */
#define MAX_NUM_COAP_RESOURCES 4

/** Maximum number of simultaneous coap messages the accessory
 *  can have in flight at a time. */
#define MAX_NUM_COAP_REQUESTS 8

/**
 * Child Timeout (in seconds)
 */
#define THREAD_CHILD_TIMEOUT_IN_SECONDS 240

/**
 * Number of concurrently supported Thread sessions.
 */
#define kAppConfig_NumThreadSessions ((size_t) kHAPThreadAccessoryServerStorage_MinSessions)

/**
 * Buffer size to allocate for HAP accessory server session storage buffer.
 *
 * - The optimal size may vary depending on the accessory's HomeKit attribute database, the number of sessions
 *   and the requests to be processed. Log messages are emitted to provide guidance for tuning of the buffer size.
 *
 * - Note: Buffer usage may vary across controller versions, so it is advised to not set the size too tightly.
 */
#define kAppConfig_NumAccessoryServerSessionStorageBytes ((size_t) 384)

/**
 * Session key valid duration till expiry - one week in milliseconds
 */
#define kAppConfig_SessionKeyExpiry (((HAPTime) 1000u) * 60u * 60u * 24u * 7u)

/**
 * Thread Tx Power in DB
 */
#define THREAD_TX_POWER_DBM (8)

// Thread app always includes HAVE_BLE so undef explicit HAVE_BLE flag
#undef HAVE_BLE
#define HAVE_BLE 1

/** Flag to indicate that non-restartable platform drivers are already initialized  */
static bool platformDriversAreInitialized = false;

#endif // HAVE_THREAD

#if (HAP_TESTING == 1)
struct adk_test_opts* _opts = NULL;
#endif

/**
 * Global platform objects.
 * Only tracks objects that will be released in DeinitializePlatform.
 */
static struct {
    HAPPlatformKeyValueStore keyValueStore;
    HAPAccessoryServerOptions hapAccessoryServerOptions;
    HAPPlatform hapPlatform;
    HAPAccessoryServerCallbacks hapAccessoryServerCallbacks;

#if (HAVE_NFC == 1)
    HAPPlatformAccessorySetupNFC setupNFC;
#endif

#if (HAVE_IP == 1)
    HAPPlatformTCPStreamManager tcpStreamManager;
#if (HAP_APP_USES_HDS == 1)
    HAPPlatformTCPStreamManager dataStreamTCPStreamManager;
#endif
#endif
#if (HAVE_MFI_HW_AUTH == 1)
    HAPPlatformMFiHWAuth mfiHWAuth;
#endif
#if (HAVE_MFI_TOKEN_AUTH == 1)
    HAPPlatformMFiTokenAuth mfiTokenAuth;
#endif
} platform;

/**
 * HomeKit accessory server that hosts the accessory.
 */
static HAPAccessoryServer accessoryServer;

/**
 * Path to an externally specified keyValueStore
 */
static char* homeKitStorePath = NULL;

void ADKSetHomeKitStorePath(char* path) {
    homeKitStorePath = path;
}

static HAPPairingStateChange currentPairingState = kHAPPairingStateChange_Unpaired;
static void (*currentPairingStateObserver)(bool);

bool ADKIsApplicationPaired(void) {
    return currentPairingState == kHAPPairingStateChange_Paired;
}

void ADKSetApplicationPairedObserver(void (*observer)(bool)) {
    currentPairingStateObserver = observer;
}

static void setCurrentPairingState(HAPPairingStateChange state) {
    bool notifyObserver = false;
    if (currentPairingState != state) {
        if (currentPairingStateObserver != NULL) {
            notifyObserver = true;
        }
    }
    currentPairingState = state;
    if (notifyObserver) {
        currentPairingStateObserver(ADKIsApplicationPaired());
    }
}

void HandleUpdatedState(HAPAccessoryServer* _Nonnull server, void* _Nullable context);
void HandlePairingStateChange(
        HAPAccessoryServer* _Nonnull server,
        HAPPairingStateChange state,
        void* _Nullable context);
void PerformFactoryReset(HAPAccessoryServer* _Nonnull server, void* _Nullable context);

#if (HAVE_THREAD == 1)
static void InitializeThread(void) {
    static uint8_t sessionStorageBytes[kAppConfig_NumAccessoryServerSessionStorageBytes];
    static HAPThreadSession threadSessions[kAppConfig_NumThreadSessions];
    static HAPThreadAccessoryServerStorage threadAccessoryServerStorage = {
        .sessions = threadSessions,
        .numSessions = HAPArrayCount(threadSessions),
    };

    platform.hapAccessoryServerOptions.maxPairings = kHAPPairingStorage_MinElements;
    platform.hapAccessoryServerOptions.sessionStorage.bytes = sessionStorageBytes;
    platform.hapAccessoryServerOptions.sessionStorage.numBytes = sizeof sessionStorageBytes;
    platform.hapAccessoryServerOptions.thread.transport = &kHAPAccessoryServerTransport_Thread;
    platform.hapAccessoryServerOptions.thread.accessoryServerStorage = &threadAccessoryServerStorage;
    platform.hapAccessoryServerOptions.thread.deviceParameters.deviceType = kThreadDeviceType;
    platform.hapAccessoryServerOptions.thread.deviceParameters.txPowerdbm = THREAD_TX_POWER_DBM;
    platform.hapAccessoryServerOptions.thread.getNextReattachDelay = GetNextThreadReattachDelay;
    platform.hapAccessoryServerOptions.thread.getNextReattachDelayContext =
            &platform.hapAccessoryServerOptions.thread.deviceParameters.deviceType;
    platform.hapAccessoryServerOptions.thread.deviceParameters.childTimeout = THREAD_CHILD_TIMEOUT_IN_SECONDS;
#if (HAVE_THREAD_DECOMMISSION_ON_UNPAIR == 1)
    platform.hapAccessoryServerOptions.thread.suppressUnpairedThreadAdvertising = true;
#else
    platform.hapAccessoryServerOptions.thread.suppressUnpairedThreadAdvertising = false;
#endif
}

/**
 * Callback to handle Thread device role change
 */
void HandleThreadRoleChange(void* _Nullable context HAP_UNUSED, size_t contextSize HAP_UNUSED) {
    HAPAccessoryServerRaiseEvent(
            &accessoryServer,
            &threadManagementStatusCharacteristic,
            &threadManagementService,
            accessoryServer.primaryAccessory);
}

#endif // HAVE_THREAD

#if (HAVE_ACCESSORY_REACHABILITY == 1 && HAVE_BLE == 1)
/**
 * Callback to handle heart beat value change
 */
void HandleHeartBeatChange(void* _Nullable context HAP_UNUSED) {
    HAPAccessoryServerRaiseEvent(
            &accessoryServer,
            &accessoryRuntimeInformationHeartBeatCharacteristic,
            &accessoryRuntimeInformationService,
            accessoryServer.primaryAccessory);
}
#endif

/**
 * Initialize global platform objects.
 */
static void InitializePlatform(void) {
#if (HAVE_THREAD == 1)
    if (!platformDriversAreInitialized)
#endif
    {
        // Initialize platform drivers
        HAPPlatformSetupDrivers();

        // Key-value store.
#if (HAP_TESTING == 1) && HAP_FEATURE_ENABLED(HAP_FEATURE_FILE_STORAGE)
        if (homeKitStorePath) {
            HAPLogDebug(&kHAPLog_Default, "Using HomeKitStore path - '%s'", homeKitStorePath);
            HAPPlatformKeyValueStoreCreate(
                    &platform.keyValueStore,
                    &(const HAPPlatformKeyValueStoreOptions) { .rootDirectory = homeKitStorePath });
        } else {
            HAPPlatformSetupInitKeyValueStore(&platform.keyValueStore);
        }
#else
        HAPPlatformSetupInitKeyValueStore(&platform.keyValueStore);
#endif

#if (HAP_TESTING == 1)
        // Setup user input driver
        AppUserInputInit();
#endif

#if (HAVE_THREAD == 1)
        platformDriversAreInitialized = true;
#endif
    }
    platform.hapPlatform.keyValueStore = &platform.keyValueStore;

#if HAP_FEATURE_ENABLED(HAP_FEATURE_FILE_STORAGE)
    HAPPlatformFileManagerSetKV(&platform.keyValueStore);
#endif

#if (HAVE_BLE == 1)
#if (HAVE_THREAD == 1)
    if (HAPPlatformSupportsBLE())
#endif
    {
        static HAPPlatformBLEPeripheralManager blePeripheralManager;
        HAPPlatformSetupInitBLE(&blePeripheralManager, &platform.keyValueStore);
        platform.hapPlatform.ble.blePeripheralManager = &blePeripheralManager;
    }
#endif // HAVE_BLE

    // Accessory setup manager. Depends on key-value store.
    static HAPPlatformAccessorySetup accessorySetup;
    HAPPlatformAccessorySetupCreate(
            &accessorySetup, &(const HAPPlatformAccessorySetupOptions) { .keyValueStore = &platform.keyValueStore });
    platform.hapPlatform.accessorySetup = &accessorySetup;

// Accessory setup programmable NFC tag.
#if (HAVE_NFC == 1)
    HAPPlatformSetupInitAccessorySetupNFC(&platform.setupNFC);
#endif

#if (HAVE_IP == 1)
    // TCP stream manager.
    HAPPlatformTCPStreamManagerCreate(
            &platform.tcpStreamManager,
            &(const HAPPlatformTCPStreamManagerOptions) {
                    .interfaceName = NULL,       // Listen on all available network interfaces.
                    .port = 2408, // Listen on unused port number from the ephemeral port range.
                    .maxConcurrentTCPStreams = kHAPIPSessionStorage_DefaultNumElements });

    // Service discovery.
    static HAPPlatformServiceDiscovery serviceDiscovery;
    HAPPlatformServiceDiscoveryCreate(
            &serviceDiscovery,
            &(const HAPPlatformServiceDiscoveryOptions) {
                    .interfaceName = "en1" /* Register services on all available network interfaces. */
            });
    platform.hapPlatform.ip.serviceDiscovery = &serviceDiscovery;

#if (HAVE_WAC == 1)
    // Wi-Fi manager.
    static HAPPlatformWiFiManager wiFiManager;
    HAPPlatformWiFiManagerCreate(&wiFiManager, &(const HAPPlatformWiFiManagerOptions) { .keyValueStore=&platform.keyValueStore,
        .interfaceName = NULL });
    platform.hapPlatform.ip.wiFi.wiFiManager = &wiFiManager;

    // Software access point manager. Depends on Wi-Fi manager.
    static HAPPlatformSoftwareAccessPoint softwareAccessPoint;
    HAPPlatformSoftwareAccessPointCreate(
            &softwareAccessPoint,
            &(const HAPPlatformSoftwareAccessPointOptions) { .ap_ssid="bk_ap" });
    platform.hapPlatform.ip.wiFi.softwareAccessPoint = &softwareAccessPoint;
#endif
#endif

#if (HAVE_WIFI_RECONFIGURATION == 1)
    if (HAPPlatformWiFiManagerIsConfigured(platform.hapPlatform.ip.wiFi.wiFiManager)) {
        HAPError err = HAPPlatformWiFiManagerSetUpdateStatus(HAPNonnull(platform.hapPlatform.ip.wiFi.wiFiManager), 0);
        if (err) {
            HAPLogError(&kHAPLog_Default, "Failed to update the wpa_supplicant");
        }
    }
#endif

#if (HAVE_THREAD == 1)
    static HAPPlatformThreadCoAPManagerResource coapResources[MAX_NUM_COAP_RESOURCES];
    static HAPPlatformThreadCoAPRequest coapRequests[MAX_NUM_COAP_REQUESTS];
    static HAPPlatformThreadCoAPManager coapManager;
    static HAPPlatformServiceDiscovery threadServiceDiscovery;
    HAPPlatformSetupInitThread(
            &accessoryServer,
            &coapManager,
            coapResources,
            HAPArrayCount(coapResources),
            coapRequests,
            HAPArrayCount(coapRequests),
            &threadServiceDiscovery);
    platform.hapPlatform.thread.coapManager = &coapManager;
    platform.hapPlatform.thread.serviceDiscovery = &threadServiceDiscovery;
#if (HAVE_IP == 1)
#error "IP and Thread cannot coexist yet"
#endif
#endif // HAVE_THREAD

#if (HAVE_MFI_HW_AUTH == 1)
    // Apple Authentication Coprocessor provider.
    HAPPlatformSetupInitMFiHWAuth(&platform.mfiHWAuth);
    platform.hapPlatform.authentication.mfiHWAuth = &platform.mfiHWAuth;
#endif

#if (HAVE_MFI_TOKEN_AUTH == 1)
    // Software Token provider. Depends on key-value store.
    HAPPlatformMFiTokenAuthCreate(
            &platform.mfiTokenAuth,
            &(const HAPPlatformMFiTokenAuthOptions) { .keyValueStore = &platform.keyValueStore });
#endif

    // Run loop.
    HAPPlatformRunLoopCreate(&(const HAPPlatformRunLoopOptions) { .keyValueStore = &platform.keyValueStore });

    platform.hapAccessoryServerOptions.maxPairings = kHAPPairingStorage_MinElements;
#if (HAVE_THREAD == 1)
    // Thread accessories must expire the session keys
    platform.hapAccessoryServerOptions.sessionKeyExpiry = kAppConfig_SessionKeyExpiry;
#endif

#if (HAVE_MFI_TOKEN_AUTH == 1)
    platform.hapPlatform.authentication.mfiTokenAuth =
            HAPPlatformMFiTokenAuthIsProvisioned(&platform.mfiTokenAuth) ? &platform.mfiTokenAuth : NULL;
#endif

    platform.hapAccessoryServerCallbacks.handleUpdatedState = HandleUpdatedState;
    platform.hapAccessoryServerCallbacks.handlePairingStateChange = HandlePairingStateChange;
    // Each application needing this can explicitly assign the callback
    platform.hapAccessoryServerCallbacks.handleControllerPairingStateChange = NULL;

#if (HAVE_NFC == 1)
    platform.hapPlatform.setupNFC = &platform.setupNFC;
#endif
}

/**
 * Deinitialize global platform objects.
 */
static void DeinitializePlatform(void) {
#if (HAVE_MFI_HW_AUTH == 1)
    // Apple Authentication Coprocessor provider.
    HAPPlatformMFiHWAuthRelease(&platform.mfiHWAuth);
#endif

#if (HAVE_IP == 1)
    // TCP stream manager.
    HAPPlatformTCPStreamManagerRelease(&platform.tcpStreamManager);
#if (HAP_APP_USES_HDS == 1)
    HAPPlatformTCPStreamManagerRelease(&platform.dataStreamTCPStreamManager);
#endif
#endif
    AppDeinitialize();

// Accessory setup programmable NFC tag.
#if (HAVE_NFC == 1)
    HAPPlatformAccessorySetupNFCRelease(&platform.setupNFC);
#endif

    // Run loop.
    HAPPlatformRunLoopRelease();
}

/**
 * Wrapper around app specific create method to perform some operations that may be common to all applications
 *
 * @param server        [description]
 * @param keyValueStore [description]
 */
void AppBaseCreate(HAPAccessoryServer* server, HAPPlatformKeyValueStoreRef keyValueStore) {
#if (HAVE_THREAD == 1)
    HAPPlatformThreadRegisterRoleUpdateCallback(HandleThreadRoleChange);
#endif

#if (HAVE_ACCESSORY_REACHABILITY == 1 && HAVE_BLE == 1)
    HAPError err = HAPAccessoryServerStartHeartBeat(server, HandleHeartBeatChange, NULL);
    HAPAssert(!err);
#endif

    AppCreate(server, keyValueStore);

#if (HAP_TESTING == 1)
    RegisterDebugCommandCallback(&ProcessCommandLine);
#if HAP_FEATURE_ENABLED(HAP_FEATURE_FILE_STORAGE)
    CreateCommandlineFile();
    DebugCommandFileMonitorStart();
#endif
#endif

#if (HAVE_GATEWAY_PING == 1)
    GatewayPingStart();
#endif // (HAVE_GATEWAY_PING == 1)
}

void AccessoryServerStart(void) {
    AppAccessoryServerStart();

    // Enter NFC pairing mode.
    if (HAVE_NFC) {
        HAPLogInfo(&kHAPLog_Default, "Entering NFC pairing mode for 5 minutes...");
        HAPAccessoryServerEnterNFCPairingMode(&accessoryServer);
    }
}

/**
 * Request a factory reset
 */
void FactoryReset(void) {
#if (HAVE_THREAD == 1)
    // Clear Thread parameters.
    // Note that the function is safe to call because the function
    // will schedule erase when transport has stopped.
    HAPPlatformThreadUnregisterRoleUpdateCallback();
    HAPError err = HAPPlatformThreadClearParameters(&accessoryServer);
    HAPAssert(!err);
#endif

    if (isReadyToPerformFactoryReset) {
        // Server is already down and ready for factory reset execution.
        isReadyToPerformFactoryReset = false;
        PerformFactoryReset(&accessoryServer, accessoryServer.context);
        return;
    }
    requestedFactoryReset = true;

    HAPAccessoryServerStop(&accessoryServer);
}

/**
 * Request to clear pairings
 */
void ClearPairings(void) {
    if (isReadyToPerformFactoryReset) {
        // In this state, there shouldn't be any pairing to clean.
        HAPAssert(!HAPAccessoryServerIsPaired(&accessoryServer));
        HAPLogDebug(&kHAPLog_Default, "Clear pairing request is ignored when server is down");
        return;
    }
    clearPairings = true;
    HAPAccessoryServerStop(&accessoryServer);
}

/**
 * Request to trigger pairing mode
 */
void TriggerPairingMode(void) {
#if (HAVE_NFC == 1)
    // For accessories with NFC programmable tag, enter pairing mode
    if (HAPAccessoryServerGetState(&accessoryServer) == kHAPAccessoryServerState_Idle) {
        AccessoryServerStart();
    } else {
        HAPAccessoryServerEnterNFCPairingMode(&accessoryServer);
    }
#else
    // For non-NFC accessories, start advertisement
    if (HAPAccessoryServerGetState(&accessoryServer) == kHAPAccessoryServerState_Idle) {
        AccessoryServerStart();
    }
#endif
}

/**
 * Restore platform specific factory settings.
 */
void HandlePlatformFactoryReset(void) {
#if (HAVE_IP == 1) && (HAVE_WAC == 1)
    if (HAPPlatformWiFiManagerIsConfigured(platform.hapPlatform.ip.wiFi.wiFiManager)) {
        HAPPlatformWiFiManagerClearConfiguration(platform.hapPlatform.ip.wiFi.wiFiManager);
    }
#endif
#if (HAVE_THREAD == 1)
    HAPPlatformThreadInitiateFactoryReset();
#endif
}

/**
 * Handle factory reset request triggered by user inputs such as signals or buttons
 */
void PerformFactoryReset(HAPAccessoryServer* _Nonnull server, void* _Nullable context) {
    HAPPrecondition(server);
    HAPLogInfo(&kHAPLog_Default, "A factory reset has been requested.");

    // Purge persistent state.
    HAPError err;
    HAPPlatformKeyValueStoreDomain domainsToPurge[] = {
        kAppKeyValueStoreDomain_Configuration,
#if (HAVE_FIRMWARE_UPDATE == 1)
        kAppKeyValueStoreDomain_FirmwareUpdate,
#endif
    };
    for (size_t i = 0; i < HAPArrayCount(domainsToPurge); i++) {
        err = HAPPlatformKeyValueStorePurgeDomain(&platform.keyValueStore, domainsToPurge[i]);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }
    }

    // Reset HomeKit state.
    err = HAPKeystoreRestoreFactorySettings(&platform.keyValueStore);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPFatalError();
    }

    // Restore platform specific factory settings.
    HandlePlatformFactoryReset();

    // Perform an application specific factory reset
    AppHandleFactoryReset(server, context);

    // De-initialize App.
    AppRelease(server, context);
#if (HAP_TESTING == 1) && HAP_FEATURE_ENABLED(HAP_FEATURE_FILE_STORAGE)
    DebugCommandFileMonitorStop();
#endif // (HAP_TESTING == 1) && HAP_FEATURE_ENABLED(HAP_FEATURE_FILE_STORAGE)

    // Re-initialize App.
    AppBaseCreate(server, &platform.keyValueStore);

    // Restart accessory server.
    AccessoryServerStart();
}

/**
 * Handle clear pairing request triggered by user inputs such as signals or buttons
 */
void PerformClearPairings(HAPAccessoryServer* _Nonnull server, void* _Nullable context) {
    HAPLogInfo(&kHAPLog_Default, "Remove pairing has been requested.");
    // Removes all pairing
    HAPError err = HAPPairingRemoveAll(server);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPFatalError();
    }

    // De-initialize App.
    AppRelease(server, context);
#if (HAP_TESTING == 1) && HAP_FEATURE_ENABLED(HAP_FEATURE_FILE_STORAGE)
    DebugCommandFileMonitorStop();
#endif // (HAP_TESTING == 1) && HAP_FEATURE_ENABLED(HAP_FEATURE_FILE_STORAGE)

    // Re-initialize App.
    AppBaseCreate(server, &platform.keyValueStore);

    // Restart accessory server.
    AccessoryServerStart();
}

/**
 * Either simply passes State handling to app, or processes Factory Reset
 */
void HandleUpdatedState(HAPAccessoryServer* _Nonnull server, void* _Nullable context) {
    switch (HAPAccessoryServerGetState(server)) {
        case kHAPAccessoryServerState_Idle: {
            HAPLogInfo(&kHAPLog_Default, "Accessory Server State did update: Idle.");
            if (requestedFactoryReset) {
                PerformFactoryReset(server, context);
                requestedFactoryReset = false;
                return;
            } else if (clearPairings) {
                PerformClearPairings(server, context);
                clearPairings = false;
                return;
            }

            AppHandleAccessoryServerStateUpdate(server, context);
            if (HAPAccessoryServerGetState(server) != kHAPAccessoryServerState_Idle) {
                return;
            }

            // Exiting the runloop would cause reboot in most of the platforms.
            // In case the accessory is unpaired, the accessory server might have stopped
            // to disallow pairing. Hence, the device must stay in this state till user reboots the device
            // or the user requests factory reset.
            if (HAPAccessoryServerIsPaired(server) || requestedFullStop) {
                HAPPlatformRunLoopStop(); // Signal run loop to exit
            } else {
                // Upon next factory reset request, factory reset must be performed without closing down the server.
                isReadyToPerformFactoryReset = true;
            }
            return;
        }
        case kHAPAccessoryServerState_Running: {
            HAPLogInfo(&kHAPLog_Default, "Accessory Server State did update: Running.");
            AppHandleAccessoryServerStateUpdate(server, context);
            return;
        }
        case kHAPAccessoryServerState_Stopping: {
            HAPLogInfo(&kHAPLog_Default, "Accessory Server State did update: Stopping.");
            AppHandleAccessoryServerStateUpdate(server, context);
            return;
        }
    }
}

/**
 * Process a pairing state change.
 */
void HandlePairingStateChange(
        HAPAccessoryServer* _Nonnull server,
        HAPPairingStateChange state,
        void* _Nullable context) {
    // Forward pairing state change for any application-specific handling.
    AppHandlePairingStateChange(server, state, context);
    setCurrentPairingState(state);
}

#if (HAVE_IP == 1)
static void InitializeIP(void) {
    // Prepare accessory server storage.
    static HAPIPSession ipSessions[kHAPIPSessionStorage_DefaultNumElements];
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 0)
    static uint8_t ipInboundBuffers[HAPArrayCount(ipSessions)][kHAPIPSession_DefaultInboundBufferSize];
    static uint8_t ipOutboundBuffers[HAPArrayCount(ipSessions)][kHAPIPSession_DefaultOutboundBufferSize];
    static HAPIPEventNotification ipEventNotifications[HAPArrayCount(ipSessions)][kAttributeCount];
#endif
    for (size_t i = 0; i < HAPArrayCount(ipSessions); i++) {
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 0)
        ipSessions[i].inboundBuffer.bytes = ipInboundBuffers[i];
        ipSessions[i].inboundBuffer.numBytes = sizeof ipInboundBuffers[i];
        ipSessions[i].outboundBuffer.bytes = ipOutboundBuffers[i];
        ipSessions[i].outboundBuffer.numBytes = sizeof ipOutboundBuffers[i];
        ipSessions[i].eventNotifications = ipEventNotifications[i];
        ipSessions[i].numEventNotifications = HAPArrayCount(ipEventNotifications[i]);
#else
        ipSessions[i].inboundBuffer.bytes = NULL;
        ipSessions[i].inboundBuffer.numBytes = 0;
        ipSessions[i].outboundBuffer.bytes = NULL;
        ipSessions[i].outboundBuffer.numBytes = 0;
        ipSessions[i].eventNotifications = NULL;
        ipSessions[i].numEventNotifications = 0;
#endif
    }
    static HAPIPReadContext ipReadContexts[kAttributeCount];
    static HAPIPWriteContext ipWriteContexts[kAttributeCount];
    static uint8_t ipScratchBuffer[kHAPIPSession_DefaultScratchBufferSize];
    static HAPIPAccessoryServerStorage ipAccessoryServerStorage = {
        .sessions = ipSessions,
        .numSessions = HAPArrayCount(ipSessions),
        .readContexts = ipReadContexts,
        .numReadContexts = HAPArrayCount(ipReadContexts),
        .writeContexts = ipWriteContexts,
        .numWriteContexts = HAPArrayCount(ipWriteContexts),
        .scratchBuffer = { .bytes = ipScratchBuffer, .numBytes = sizeof ipScratchBuffer
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 1)
        },
        .dynamicMemoryAllocation = { .allocateMemory = os_malloc, .reallocateMemory = os_realloc, .deallocateMemory = os_free
#endif
        }
    };

    platform.hapAccessoryServerOptions.ip.transport = &kHAPAccessoryServerTransport_IP;
    platform.hapAccessoryServerOptions.ip.accessoryServerStorage = &ipAccessoryServerStorage;
    platform.hapAccessoryServerOptions.ip.wac.available = HAVE_WAC ? true : false;

    platform.hapPlatform.ip.tcpStreamManager = &platform.tcpStreamManager;
}
#endif

#if (HAVE_BLE == 1)
static void InitializeBLE(void) {
    static HAPBLEGATTTableElement gattTableElements[kAttributeCount];
    static HAPPairingBLESessionCacheEntry sessionCacheElements[kHAPBLESessionCache_MinElements];
    static HAPSession session;
    static uint8_t procedureBytes[2048];
    static HAPBLEProcedure procedures[1];

    static HAPBLEAccessoryServerStorage bleAccessoryServerStorage = {
        .gattTableElements = gattTableElements,
        .numGATTTableElements = HAPArrayCount(gattTableElements),
        .sessionCacheElements = sessionCacheElements,
        .numSessionCacheElements = HAPArrayCount(sessionCacheElements),
        .session = &session,
        .procedures = procedures,
        .numProcedures = HAPArrayCount(procedures),
        .procedureBuffer = { .bytes = procedureBytes, .numBytes = sizeof procedureBytes },
    };

    platform.hapAccessoryServerOptions.ble.transport = &kHAPAccessoryServerTransport_BLE;
    platform.hapAccessoryServerOptions.ble.accessoryServerStorage = &bleAccessoryServerStorage;
    platform.hapAccessoryServerOptions.ble.preferredAdvertisingInterval = PREFERRED_ADVERTISING_INTERVAL;
    platform.hapAccessoryServerOptions.ble.preferredNotificationDuration = kHAPBLENotification_MinDuration;
}
#endif

#if (HAP_APP_USES_HDS == 1)
static void InitializeDataStream(void) {
    HAP_UNUSED static HAPDataStreamRef dataStreams[kApp_NumDataStreams];

#if (HAVE_IP == 1)

#if (HAP_TESTING == 1) && (HAP_APP_USES_HDS == 1) && (HAVE_HDS_TRANSPORT_OVER_HAP == 1)
    // If user forces an override for testing, then always take that.
    // (Only needed if both TCP & HAP are options)
    if (_opts && _opts->hds_over_hap_override) {
        // Set up HDS to operate over HAP (by just setting up the arrays but providing no TCP manager).
        platform.hapAccessoryServerOptions.dataStream.dataStreams = dataStreams;
        platform.hapAccessoryServerOptions.dataStream.numDataStreams = HAPArrayCount(dataStreams);
        return;
    }
#endif

    // Set up HDS to operate over TCP using this TCP Stream Manager.
    HAPPlatformTCPStreamManagerCreate(
            &platform.dataStreamTCPStreamManager,
            &(const HAPPlatformTCPStreamManagerOptions) {
                    .interfaceName = NULL,       // Listen on all available network interfaces.
                    .port = kHAPNetworkPort_Any, // Listen on unused port number from the ephemeral port range.
                    .maxConcurrentTCPStreams = kApp_NumDataStreams });

    platform.hapAccessoryServerOptions.ip.dataStream.tcpStreamManager = &platform.dataStreamTCPStreamManager;
    platform.hapAccessoryServerOptions.dataStream.dataStreams = dataStreams;
    platform.hapAccessoryServerOptions.dataStream.numDataStreams = HAPArrayCount(dataStreams);
#else // (HAVE_IP == 1)

#if (HAVE_HDS_TRANSPORT_OVER_HAP == 1)
#if (HAVE_BLE == 1) || (HAVE_THREAD == 1)
    // Set up HDS to operate over HAP (by just setting up the arrays but providing no TCP manager).
    platform.hapAccessoryServerOptions.dataStream.dataStreams = dataStreams;
    platform.hapAccessoryServerOptions.dataStream.numDataStreams = HAPArrayCount(dataStreams);
#endif
#endif

#endif // (HAVE_IP == 1)
}
#endif // HAP_APP_USES_HDS

void ADKStartupApplication(void) {
    HAPAssert(HAPGetCompatibilityVersion() == HAP_COMPATIBILITY_VERSION);

    // Initialize global platform objects.
    InitializePlatform();

#if (HAVE_IP == 1)
    InitializeIP();
#endif

#if (HAVE_BLE == 1)
#if (HAVE_THREAD == 1)
    if (HAPPlatformSupportsBLE())
#endif
    {
        InitializeBLE();
    }
#endif

#if (HAVE_THREAD == 1)
    InitializeThread();
#endif // HAVE_THREAD

#if (HAP_APP_USES_HDS == 1)
    InitializeDataStream();
#endif

#if (HAP_TESTING == 1) && (HAVE_IP == 1)
    platform.hapAccessoryServerOptions.ip.camera.mediaSourcePath = _opts ? _opts->media_path : NULL;
#endif
#if (HAVE_FIRMWARE_UPDATE == 1) && (HAP_TESTING == 1)
    platform.hapAccessoryServerOptions.firmwareUpdate.persistStaging = _opts ? _opts->fwup_persist_staging : false;
#endif
    // Perform Application-specific initializations such as setting up callbacks
    // and configure any additional unique platform dependencies
    AppInitialize(&platform.hapAccessoryServerOptions, &platform.hapPlatform, &platform.hapAccessoryServerCallbacks);

    // Initialize accessory server.
    HAPAccessoryServerCreate(
            &accessoryServer,
            &platform.hapAccessoryServerOptions,
            &platform.hapPlatform,
            &platform.hapAccessoryServerCallbacks,
            /* context: */ NULL);

    // Create app object.
    AppBaseCreate(&accessoryServer, &platform.keyValueStore);

#if (HAP_TESTING == 1)
    if (_opts && _opts->accessory_name) {
        HAPAccessory* accessory = (HAPAccessory*) AppGetAccessoryInfo();
        accessory->name = _opts->accessory_name;
    }

    if (_opts && _opts->firmware_version) {
        HAPAccessory* accessory = (HAPAccessory*) AppGetAccessoryInfo();
        accessory->firmwareVersion = _opts->firmware_version;
    }

#if (HAVE_NFC_ACCESS == 1)
    if (_opts) {
        HAPAccessory* accessory = (HAPAccessory*) AppGetAccessoryInfo();
        accessory->hardwareFinish = _opts->hardwareFinish;
    }
#endif
#endif

    // Start accessory server for App.
    AccessoryServerStart();

#if (HAP_TESTING == 1)
    // Check if kAttributeCount has correct value
    int actualAttributeCount = getTotalAttributeCount();
    if (kAttributeCount < actualAttributeCount) {
        HAPLogInfo(
                &kHAPLog_Default,
                "kAttributeCount is %d, but actual number of attributes is %d",
                kAttributeCount,
                actualAttributeCount);
        HAPAssert(false);
    }

    // Check if IIDs are unique
    if (checkForDuplicateIIDs()) {
        HAPLogInfo(&kHAPLog_Default, "Duplicate IIDs found");
        HAPAssert(false);
    }
#endif
}

void ADKRunApplicationLoop(void) {
    // Run main loop until explicitly stopped.
    HAPLogInfo(&kHAPLog_Default, "Entering run loop");
    HAPPlatformRunLoopRun();
    // Run loop stopped explicitly by calling function HAPPlatformRunLoopStop.
}

void ADKCleanupApplication(void) {
#if (HAVE_GATEWAY_PING == 1)
    GatewayPingStop();
#endif // (HAVE_GATEWAY_PING == 1)

    // Cleanup.
    AppRelease(&accessoryServer, NULL);
#if (HAP_TESTING == 1) && HAP_FEATURE_ENABLED(HAP_FEATURE_FILE_STORAGE)
    DebugCommandFileMonitorStop();
#endif // (HAP_TESTING == 1) && HAP_FEATURE_ENABLED(HAP_FEATURE_FILE_STORAGE)

    HAPAccessoryServerRelease(&accessoryServer);

    DeinitializePlatform();

    requestedFullStop = false;
}

/**
 * This function is used start and run ADK applications.
 */
static void __RunApplication(void) {
    ADKStartupApplication();
    ADKRunApplicationLoop();
    ADKCleanupApplication();
}

static void StopApplicationCallback(void* _Nullable context HAP_UNUSED, size_t contextSize HAP_UNUSED) {
    if (isReadyToPerformFactoryReset) {
        HAPPlatformRunLoopStop();
    } else {
        requestedFullStop = true;
        HAPAccessoryServerStop(&accessoryServer);
    }
}

void AdkStopApplication(void) {
    HAPLogInfo(&kHAPLog_Default, "Stopping ADK Application.");
    HAPError err;
    err = HAPPlatformRunLoopScheduleCallback(StopApplicationCallback, NULL, 0);
    if (err) {
        HAPLogError(&kHAPLog_Default, "ADK Stop event scheduling failed");
        HAPFatalError();
    }
}

/**
 * This symbol is the externally-visible entry point start an ADK application.
 * This function should be called from main() or from pthread_create() and
 * isn't expected to exit until shutdown/reset.
 */
void* AdkRunApplication(void* ctx HAP_UNUSED) {
#if (HAP_TESTING == 1)
    _opts = (struct adk_test_opts*) ctx;
    if (_opts && _opts->homeKitStorePath) {
        ADKSetHomeKitStorePath(_opts->homeKitStorePath);
    }
    if (_opts && _opts->commandFilePath) {
        SetCommandLineFilePath(_opts->commandFilePath);
    }
#endif

    __RunApplication();

    return 0;
}

#if (HAP_TESTING == 1)
HAPAccessoryServer* AppGetAccessoryServer(void) {
    return &accessoryServer;
}
#endif
