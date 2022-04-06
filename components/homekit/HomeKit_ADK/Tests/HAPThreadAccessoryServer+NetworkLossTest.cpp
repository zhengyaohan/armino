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

#include "HAP+KeyValueStoreDomains.h"
#include "HAPBLESession.h"
#include "HAPPDU+NotificationConfiguration.h"
#include "HAPPDU+TLV.h"

#include "HAPPlatform+Init.h"
#include "HAPPlatformBLEPeripheralManager+Init.h"

#include "Harness/HAPAccessoryServerHelper.h"
#include "Harness/HAPCryptoHelper.h"
#include "Harness/HAPPlatformClockHelper.h"
#include "Harness/HAPPlatformTimerHelper.h"
#include "Harness/HAPTestController.h"
#include "Harness/IPTestHelpers.h"
#include "Harness/SecurityTestHelpers.h"
#include "Harness/TemplateDB.h"
#include "Harness/UnitTestFramework.h"

#include "HAPPlatformSetup+Init.h"

// Use TEST_ASSERT for mock assertions
#define MOCK_ASSERT TEST_ASSERT

#if !HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
int main() {
    return 0;
}
#else

#include "Harness/HAPPlatformBLEMock.h"
#include "Harness/HAPPlatformCoAPManagerMock.h"
#include "Harness/HAPPlatformRunLoopMock.h"
#include "Harness/HAPPlatformServiceDiscoveryMock.h"
#include "Harness/HAPPlatformThreadMock.h"
#include "Harness/HAPThreadSessionHelper.h"
#include "Harness/ThreadReattachHelper.h"

#include <stdlib.h>

#define kAppConfig_NumAccessoryServerSessionStorageBytes ((size_t) 256)
#define kAppConfig_NumThreadSessions                     ((size_t) kHAPThreadAccessoryServerStorage_MinSessions)
#define PREFERRED_ADVERTISING_INTERVAL                   (HAPBLEAdvertisingIntervalCreateFromMilliseconds(417.5f))
#define THREAD_JOIN_DELAY                                ((HAPTime)(5 * HAPSecond))
/** a smaller value than THREAD_JOIN_DELAY in order to test the accuracy of the delay  */
#define THREAD_JOIN_DELAY_FRACTION                       ((HAPTime)(500 * HAPMillisecond))
/**
 * AttachTimeout in milliseconds to use with Initiate Thread Joiner operation.
 *
 * Spec fixed it to 65 seconds.
 */
#define THREAD_JOIN_ATTACH_TIMEOUT                       65000
/**
 * AttachTimeout in milliseconds to use with Set Thread Parameters operation.
 *
 * Spec fixed to 65 seconds.
 */
#define THREAD_SET_PARAMS_ATTACH_TIMEOUT                 65000
/** Channel to use with Set Thread Parameters operation */
#define THREAD_SET_PARAMS_CHANNEL                        24
/** PAN ID to use with Set Thread Parameters operation */
#define THREAD_SET_PARAMS_PANID                          0x7654
/** thread reattempt period in milliseconds */
#define THREAD_REATTEMPT_PERIOD                          (1000lu * 60 * 60)
/** spec defined pair-setup timeout to clear Thread credentials */
#define PAIR_SETUP_TIMEOUT                               ((HAPTime)(5 * HAPMinute))
/** thread border router detection timeout */
#define THREAD_DETECTION_TIMEOUT                         (HAPSecond * 65)

extern "C" {
static const HAPLogObject logObject = { .subsystem = "com.apple.mfi.HomeKit.Core.Test", .category = "TestController" };
}

static HAPAccessory InitAccessory(void) {
    HAPAccessory accessory = {
        .aid = 1,
        .category = kHAPAccessoryCategory_Locks,
        .name = "Acme Test",
        .productData = "03d8a775e3644573",
        .manufacturer = "Acme",
        .model = "Test1,1",
        .serialNumber = "099DB48E9E28",
        .firmwareVersion = "1",
        .hardwareVersion = "1",
    };
    static const HAPService* services[] = {
        &accessoryInformationService, &hapProtocolInformationService, &pairingService, &threadManagementService, NULL,
    };
    accessory.services = services;
    accessory.callbacks.identify = IdentifyAccessoryHelper;
    return accessory;
}

static const HAPAccessory accessory = InitAccessory();

/**
 * Create a BLE and Thread accessory server
 *
 * @param deviceType              either kHAPPlatformThreadDeviceCapabilities_MED or
 * kHAPPlatformThreadDeviceCapabilities_SED
 * @param getNextReattachDelay    function to get reattach delay value
 * @param getNextReattachDelayContext context to pass to the getNextReattachDelay function
 * @return created accessory server
 */
static HAPAccessoryServer* CreateBleThreadAccessoryServer(
        HAPPlatformThreadDeviceCapabilities deviceType,
        HAPThreadGetReattachDelay _Nullable getNextReattachDelay,
        void* getNextReattachDelayContext) {
    HAPAccessoryServerOptions serverOptions;
    HAPRawBufferZero(&serverOptions, sizeof serverOptions);

    // Thread transport storage
    static uint8_t sessionStorageBytes[kAppConfig_NumAccessoryServerSessionStorageBytes];
    HAPRawBufferZero(sessionStorageBytes, sizeof sessionStorageBytes);
    static HAPThreadSession threadSessions[kAppConfig_NumThreadSessions];
    HAPRawBufferZero(threadSessions, sizeof threadSessions);
    static HAPThreadAccessoryServerStorage threadAccessoryServerStorage;
    threadAccessoryServerStorage = (HAPThreadAccessoryServerStorage) {
        .sessions = threadSessions,
        .numSessions = HAPArrayCount(threadSessions),
    };

    // BLE storage
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

    // Dependency objects
    static HAPPlatformThreadCoAPManager coapManager;
    static HAPPlatformServiceDiscovery threadServiceDiscovery;

    // Accessory server
    static HAPAccessoryServer accessoryServer;

    // Set server options
    serverOptions.maxPairings = kHAPPairingStorage_MinElements;

    serverOptions.sessionStorage.bytes = sessionStorageBytes;
    serverOptions.sessionStorage.numBytes = sizeof sessionStorageBytes;
    serverOptions.thread.transport = &kHAPAccessoryServerTransport_Thread;
    serverOptions.thread.accessoryServerStorage = &threadAccessoryServerStorage;

    serverOptions.ble.transport = &kHAPAccessoryServerTransport_BLE;
    serverOptions.ble.accessoryServerStorage = &bleAccessoryServerStorage;
    serverOptions.ble.preferredAdvertisingInterval = PREFERRED_ADVERTISING_INTERVAL;
    serverOptions.ble.preferredNotificationDuration = kHAPBLENotification_MinDuration;

    serverOptions.thread.deviceParameters.deviceType = deviceType;
    if (getNextReattachDelay) {
        serverOptions.thread.getNextReattachDelay = getNextReattachDelay;
        serverOptions.thread.getNextReattachDelayContext = getNextReattachDelayContext;
    } else {
        static HAPPlatformThreadDeviceCapabilities staticDeviceType;

        // Use the helper function when no custom function is specified
        serverOptions.thread.getNextReattachDelay = GetNextThreadReattachDelay;
        staticDeviceType = deviceType;
        serverOptions.thread.getNextReattachDelayContext = &staticDeviceType;
    }

    // Initialize Thread platform used in accessory server
    platform.thread.coapManager = &coapManager;
    platform.thread.serviceDiscovery = &threadServiceDiscovery;

    static const HAPAccessoryServerCallbacks callbacks = {
        .handleUpdatedState = HAPAccessoryServerHelperHandleUpdatedAccessoryServerState
    };
    HAPAccessoryServerCreate(&accessoryServer, &serverOptions, &platform, &callbacks, NULL);

    return &accessoryServer;
}

/* Common mock functions */
HAP_PLATFORM_BLE_MOCK(bleMock);
HAP_PLATFORM_COAP_MANAGER_MOCK(coapManagerMock);
HAP_PLATFORM_THREAD_MOCK(threadMock);

bool bleIsOn = false, threadIsOn = false;
HAPPlatformBLEPeripheralManager* blePeripheralManager = NULL;
HAPPlatformRunLoopCallback startJoinerCallback = NULL;
HAPAccessoryServer* startJoinerServer = NULL;
HAPPlatformThreadBorderRouterStateCallback borderRouterCallback = NULL;
void* borderRouterCallbackContext = NULL;

/**
 * Common test setup
 */
TEST_SETUP() {
    ALWAYS(bleMock, HAPPlatformBLEInitialize).Do([&](HAPPlatformBLEPeripheralManager* manager) {
        blePeripheralManager = manager;
        bleIsOn = true;
    });
    ALWAYS(bleMock, HAPPlatformBLEPeripheralManagerSetDelegate)
            .Do([&](HAPPlatformBLEPeripheralManagerRef manager,
                    const HAPPlatformBLEPeripheralManagerDelegate* _Nullable delegate) {
                TEST_ASSERT(manager);
                if (delegate) {
                    manager->delegate = *delegate;
                } else {
                    HAPRawBufferZero(&manager->delegate, sizeof manager->delegate);
                }
            });
    ALWAYS(bleMock, HAPPlatformBLEDeinitialize).Do([&]() { bleIsOn = false; });
    ALWAYS(threadMock, HAPPlatformThreadInitialize)
            .Do([&](void* server,
                    HAPPlatformThreadDeviceCapabilities deviceType,
                    uint32_t pollPeriod,
                    uint32_t childTimeout,
                    uint8_t txPower) { threadIsOn = true; });
    ALWAYS(threadMock, HAPPlatformThreadDeinitialize).Do([&](void* server) { threadIsOn = false; });
    ALWAYS(threadMock, HAPPlatformThreadStartJoiner)
            .Do([&](const char* passPhrase, HAPPlatformRunLoopCallback callback, void* server) {
                startJoinerCallback = callback;
                startJoinerServer = (HAPAccessoryServer*) server;
                return kHAPError_None;
            });
    ALWAYS(threadMock, HAPPlatformThreadRegisterBorderRouterStateCallback)
            .Do([&](HAPPlatformThreadBorderRouterStateCallback callback, void* context) {
                borderRouterCallback = callback;
                borderRouterCallbackContext = context;
            });
}

/**
 * Teardown step after a test so that next test can run in a clean state.
 */
TEST_TEARDOWN() {
    // Clear all pairings
    HAPError err = HAPPlatformKeyValueStorePurgeDomain(platform.keyValueStore, kHAPKeyValueStoreDomain_Pairings);
    TEST_ASSERT(!err);
    // Clears all timers
    HAPPlatformTimerDeregisterAll();

    // Verify mocks in case they are not verified in each test
    VERIFY_ALL(bleMock);
    VERIFY_ALL(threadMock);
    VERIFY_ALL(coapManagerMock);

    // Reset global variable states
    bleIsOn = false;
    threadIsOn = false;
    blePeripheralManager = NULL;
    startJoinerCallback = NULL;
    startJoinerServer = NULL;
    borderRouterCallback = NULL;
    borderRouterCallbackContext = NULL;
    bleMock.Reset();
    threadMock.Reset();
    coapManagerMock.Reset();
}

/**
 * Add a pairing entry into key value store if the entry does not exist.
 *
 * @param controllerPairingID  controller pairing ID
 */
static void AddPairingEntry(HAPPlatformKeyValueStoreKey controllerPairingID) {
    bool found;
    HAPError err = HAPPlatformKeyValueStoreGet(
            platform.keyValueStore, kHAPKeyValueStoreDomain_Pairings, controllerPairingID, NULL, 0, NULL, &found);
    HAPAssert(!err);
    if (found) {
        // Pairing already exists.
        return;
    }

    HAPControllerPublicKey controllerPublicKey;
    HAPControllerPairingIdentifier controllerPairingIdentifier;
    HAPRawBufferZero(&controllerPairingIdentifier, sizeof controllerPairingIdentifier);
    controllerPairingIdentifier.numBytes = sizeof controllerPairingIdentifier.bytes;
    HAPPlatformRandomNumberFill(controllerPairingIdentifier.bytes, sizeof controllerPairingIdentifier.bytes);

    HAPPlatformRandomNumberFill(controllerPublicKey.bytes, sizeof controllerPublicKey.bytes);

    err = HAPLegacyImportControllerPairing(
            platform.keyValueStore,
            controllerPairingID,
            &controllerPairingIdentifier,
            &controllerPublicKey,
            /* isAdmin: */ true);
    HAPAssert(!err);
}

/**
 * Test BLE Thread capable accessory handling of Thread connectivity loss when Thread role event was subscribed,
 * which will trigger sending event after connectivity loss.
 */
TEST(TestThreadConnectivityLossWithThreadRoleNotification) {
    // This test assumes Thread is commissioned.
    ALWAYS(threadMock, HAPPlatformThreadIsCommissioned).Return(true);

    // Add a pairing
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    AddPairingEntry(controllerPairingID);

    HAPAccessoryServer* server = CreateBleThreadAccessoryServer(kHAPPlatformThreadDeviceCapabilities_SED, NULL, NULL);

    // Start accessory server.
    EXPECT(threadMock, HAPPlatformThreadJoinCommissionedNetwork).AtLeast(1);
    EXPECT(bleMock, HAPPlatformBLEInitialize).AtMost(0);

    HAPPlatformThreadCoAPManagerRequestCallback pairVerifyRequestCallback = NULL;
    void* pairVerifyRequestContext = NULL;

    EXPECT(coapManagerMock, HAPPlatformThreadCoAPManagerAddResource)
            .If([&](HAPPlatformThreadCoAPManagerRef coapManager,
                    const char* uriPath,
                    HAPPlatformThreadCoAPManagerRequestCallback callback,
                    void* _Nullable context,
                    HAPPlatformThreadCoAPManagerResourceRef* coapResource) { return HAPStringAreEqual(uriPath, "2"); })
            .Do([&](HAPPlatformThreadCoAPManagerRef coapManager,
                    const char* uriPath,
                    HAPPlatformThreadCoAPManagerRequestCallback callback,
                    void* _Nullable context,
                    HAPPlatformThreadCoAPManagerResourceRef* coapResource) {
                pairVerifyRequestCallback = callback;
                pairVerifyRequestContext = context;
                *coapResource = (HAPPlatformThreadCoAPManagerResourceRef) "2";
                return kHAPError_None;
            })
            .AtLeast(1);

    HAPAccessoryServerStart(server, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(server), kHAPAccessoryServerState_Running);

    // Thread must have been started and enabled
    TEST_ASSERT(threadIsOn);

    // BLE must not have have been initialized
    TEST_ASSERT(!bleIsOn);

    VERIFY_ALL(bleMock);
    VERIFY_ALL(threadMock);

    // Border router detection in 10 seconds
    HAPPlatformClockAdvance(HAPSecond * 10);
    TEST_ASSERT(borderRouterCallback);
    borderRouterCallback(true, borderRouterCallbackContext);
    HAPPlatformClockAdvance(0);

    // BLE should not be initialized for a couple of minutes either
    VERIFY_ALL(bleMock);
    VERIFY_ALL(threadMock);
    EXPECT(bleMock, HAPPlatformBLEInitialize).AtMost(0);

    HAPPlatformClockAdvance(HAPMinute * 2);

    // Fake pair verify
    HAPPlatformThreadCoAPManagerPeer peer;
    peer.ipAddress.version = kHAPIPAddressVersion_IPv6;
    uint8_t address[] = {
        0x20, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
    };
    HAPRawBufferCopyBytes((void*) peer.ipAddress._.ipv6.bytes, (void*) address, sizeof address);
    peer.port = 0x1234;
    const HAPSessionKey accessoryToControllerKey = { {
            0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
            0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00,
    } };
    uint64_t accessoryToControllerNonce = 0;
    const HAPSessionKey controllerToAccessoryKey = { {
            0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
            0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00,
    } };
    uint64_t controllerToAccessoryNonce = 0;
    const HAPSessionKey accessoryEventKey = { {
            0x00, 0x11, 0x12, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
            0x00, 0x11, 0x12, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
    } };
    uint64_t accessoryEventNonce = 0;

    HAPThreadSessionCreateHelper(
            server,
            platform.thread.coapManager,
            &peer,
            pairVerifyRequestCallback,
            pairVerifyRequestContext,
            &accessoryToControllerKey,
            accessoryToControllerNonce,
            &controllerToAccessoryKey,
            controllerToAccessoryNonce,
            &accessoryEventKey,
            accessoryEventNonce);

    HAPPlatformClockAdvance(HAPSecond);

    // Queue a notification in order to test that thread session storages are cleared properly.

    // Fake notify enable
    HAPThreadSession* threadSession = HAPThreadSessionFindHelper(server, &peer);
    TEST_ASSERT(threadSession);
    HAPError err = HAPNotificationHandleRegisterRequest(
            server,
            &threadSession->hapSession,
            &threadManagementStatusCharacteristic,
            &threadManagementService,
            &accessory);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    TEST_ASSERT(threadIsOn);
    HAPPlatformClockAdvance(10 * HAPMinute);
    TEST_ASSERT(threadIsOn);

    // Border lost
    VERIFY_ALL(bleMock);
    VERIFY_ALL(threadMock);
    EXPECT(bleMock, HAPPlatformBLEInitialize).AtMost(0);
    EXPECT(threadMock, HAPPlatformThreadAddWakeLock)
            .If([&](HAPPlatformThreadWakeLock* lock, HAPTime timeout) {
                return lock == &server->thread.storage->networkJoinState.borderRouterDetectionWakeLock &&
                       timeout == 65 * HAPSecond;
            })
            .AtLeast(1);
    EXPECT(threadMock, HAPPlatformThreadRemoveWakeLock)
            .If([&](HAPPlatformThreadWakeLock* lock) {
                return lock == &server->thread.storage->networkJoinState.borderRouterDetectionWakeLock;
            })
            .AtMost(0);

    borderRouterCallback(false, borderRouterCallbackContext);

    // Thread must be up for border router detection timeout */
    VERIFY_ALL(bleMock);
    VERIFY_ALL(threadMock);
    EXPECT(bleMock, HAPPlatformBLEInitialize).AtMost(0);
    EXPECT(threadMock, HAPPlatformThreadAddWakeLock)
            .If([&](HAPPlatformThreadWakeLock* lock, HAPTime timeout) {
                return lock == &server->thread.storage->networkJoinState.borderRouterDetectionWakeLock &&
                       timeout == 65 * HAPSecond;
            })
            .AtMost(0);
    EXPECT(threadMock, HAPPlatformThreadRemoveWakeLock)
            .If([&](HAPPlatformThreadWakeLock* lock) {
                return lock == &server->thread.storage->networkJoinState.borderRouterDetectionWakeLock;
            })
            .AtMost(0);

    HAPPlatformClockAdvance(THREAD_DETECTION_TIMEOUT - HAPSecond);
    TEST_ASSERT(threadIsOn);

    VERIFY_ALL(bleMock);
    VERIFY_ALL(threadMock);
    VERIFY_ALL(coapManagerMock);

    // Raise the Thread role notification.
    EXPECT(threadMock, HAPPlatformThreadGetRole).Do([](HAPPlatformThreadDeviceRole* role) {
        *role = kHAPPlatformThreadDeviceRole_Detached;
        return kHAPError_None;
    });
    EXPECT(coapManagerMock, HAPPlatformThreadCoAPManagerSendRequest)
            .If([=](HAPPlatformThreadCoAPManagerRef coapManager,
                    const HAPPlatformThreadCoAPManagerPeer* peerDest,
                    const char* uriPath,
                    HAPPlatformThreadCoAPManagerRequestCode requestCode,
                    const void* requestBytes,
                    size_t numRequestBytes,
                    HAPPlatformThreadCoAPManagerSendRequestCompletionHandler completionHandler,
                    void* _Nullable context) {
                return (HAPIPAddressAreEqual(&peerDest->ipAddress, &peer.ipAddress) && peerDest->port == peer.port &&
                        HAPStringAreEqual(uriPath, ""));
            })
            .Do([&](HAPPlatformThreadCoAPManagerRef coapManager,
                    const HAPPlatformThreadCoAPManagerPeer* peer,
                    const char* uriPath,
                    HAPPlatformThreadCoAPManagerRequestCode requestCode,
                    const void* requestBytes,
                    size_t numRequestBytes,
                    HAPPlatformThreadCoAPManagerSendRequestCompletionHandler completionHandler,
                    void* _Nullable context) {
                // Verify decryption
                HAPDecryptEventNotificationHelper(
                        requestBytes,
                        numRequestBytes,
                        &accessoryEventKey,
                        accessoryEventNonce,
                        [](uint16_t iid, HAPTLV tlv) {
                            TEST_ASSERT_EQUAL(iid, threadManagementStatusCharacteristic.iid);
                            HAPLogBufferDebug(
                                    &logObject, tlv.value.bytes, tlv.value.numBytes, "Queued Thread role notification");
                        });
                TEST_ASSERT_EQUAL(err, kHAPError_None);

                // Note that completionHandler will not be called since Thread is already lost.

                return kHAPError_None;
            })
            .AtLeast(1);
    HAPAccessoryServerRaiseEvent(server, &threadManagementStatusCharacteristic, &threadManagementService, &accessory);

    // Detection timeout occurs
    EXPECT(threadMock, HAPPlatformThreadAddWakeLock)
            .If([&](HAPPlatformThreadWakeLock* lock, HAPTime timeout) {
                return lock == &server->thread.storage->networkJoinState.borderRouterDetectionWakeLock &&
                       timeout == 65 * HAPSecond;
            })
            .AtMost(0);
    EXPECT(threadMock, HAPPlatformThreadRemoveWakeLock)
            .If([&](HAPPlatformThreadWakeLock* lock) {
                return lock == &server->thread.storage->networkJoinState.borderRouterDetectionWakeLock;
            })
            .AtMost(0);
    EXPECT(threadMock, HAPPlatformThreadNetworkIsViable).Return(false);

    HAPPlatformClockAdvance(HAPSecond);

    // BLE must be up
    TEST_ASSERT(bleIsOn);

    // Thread must be down
    TEST_ASSERT(!threadIsOn);

    VERIFY_ALL(bleMock);
    VERIFY_ALL(threadMock);
    VERIFY_ALL(coapManagerMock);
}

int main(int argc, char** argv) {
    HAPPlatformCreate();
    HAPPlatformSetupDrivers();

    HAPAssert(HAPGetCompatibilityVersion() == HAP_COMPATIBILITY_VERSION);

    // Setup key value store
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    HAPControllerPublicKey controllerPublicKey;
    TestPrepareKeyValueStore(controllerPairingID, &controllerPublicKey);
#if (HAP_FEATURE_THREAD_CERTIFICATION_OVERRIDES)
    // If network viability tests are disabled, most of these tests are irrelevant.
#else
    return EXECUTE_TESTS(argc, (const char**) argv);
#endif
}

#endif
