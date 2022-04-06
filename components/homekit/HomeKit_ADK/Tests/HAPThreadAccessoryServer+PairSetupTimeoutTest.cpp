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

#include "HAP+KeyValueStoreDomains.h"
#include "HAPPDU+TLV.h"

#include "HAPPlatform+Init.h"

#include "Harness/IPTestHelpers.h"
#include "Harness/SecurityTestHelpers.h"
#include "Harness/TemplateDB.h"

#include "Harness/HAPAccessoryServerHelper.h"
#include "Harness/HAPPlatformClockHelper.h"
#include "Harness/HAPPlatformTimerHelper.h"
#include "Harness/ThreadReattachHelper.h"
#include "Harness/UnitTestFramework.h"

// Use TEST_ASSERT for mock assertions
#define MOCK_ASSERT TEST_ASSERT

#if !HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
int main() {
    return 0;
}
#else

#include "Harness/HAPPlatformBLEMock.h"
#include "Harness/HAPPlatformRunLoopMock.h"
#include "Harness/HAPPlatformServiceDiscoveryMock.h"
#include "Harness/HAPPlatformThreadMock.h"

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
/** spec defined pair-setup timeout to clear Thread credentials */
#define PAIR_SETUP_TIMEOUT                               ((HAPTime)(5 * HAPMinute))
/** thread border router detection timeout */
#define THREAD_DETECTION_TIMEOUT                         (HAPSecond * 65)

extern "C" {

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

} // extern "C"

/**
 * Creates a BLE and Thread accessory server
 *
 * @return created accessory server
 */
static HAPAccessoryServer* CreateBleThreadAccessoryServer() {
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

    static const HAPPlatformThreadDeviceCapabilities deviceType = kHAPPlatformThreadDeviceCapabilities_MED;
    serverOptions.thread.deviceParameters.deviceType = deviceType;
    serverOptions.thread.getNextReattachDelay = GetNextThreadReattachDelay;
    serverOptions.thread.getNextReattachDelayContext = (void*) &deviceType;

    // Initialize Thread platform used in accessory server
    platform.thread.coapManager = &coapManager;
    platform.thread.serviceDiscovery = &threadServiceDiscovery;

    static const HAPAccessoryServerCallbacks serverCallbacks = {
        .handleUpdatedState = HAPAccessoryServerHelperHandleUpdatedAccessoryServerState,
    };
    HAPAccessoryServerCreate(&accessoryServer, &serverOptions, &platform, &serverCallbacks, NULL);

    return &accessoryServer;
}

/**
 * Setup step before a test
 */
TEST_SETUP() {
    // Clear all pairings
    HAPError err = HAPPlatformKeyValueStorePurgeDomain(platform.keyValueStore, kHAPKeyValueStoreDomain_Pairings);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
}

/**
 * Teardown step after a test so that next test can run in a clean state.
 */
TEST_TEARDOWN() {
    // Clears all timers
    HAPPlatformTimerDeregisterAll();
}

/**
 * Adds a pairing entry into key value store if the entry does not exist.
 *
 * @param controllerPairingID  controller pairing ID
 */
static void AddPairingEntry(HAPPlatformKeyValueStoreKey controllerPairingID) {
    bool found;
    HAPError err = HAPPlatformKeyValueStoreGet(
            platform.keyValueStore, kHAPKeyValueStoreDomain_Pairings, controllerPairingID, NULL, 0, NULL, &found);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
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
    TEST_ASSERT_EQUAL(err, kHAPError_None);
}

/**
 * Emulate pairing adds while server is running
 *
 * @param      server        accessory server
 * @param[out] pairingIndex  pairing index of the newly added item
 */
static void TriggerPairingAdd(HAPAccessoryServer* server, HAPPairingIndex* pairingIndex) {
    HAPPairing pairing;
    HAPPlatformRandomNumberFill(pairing.identifier.bytes, sizeof pairing.identifier.bytes);
    pairing.numIdentifierBytes = sizeof pairing.identifier.bytes;
    HAPPlatformRandomNumberFill(pairing.publicKey.value, sizeof pairing.publicKey.value);
    pairing.permissions = 0;

    HAPError err = HAPPairingAdd(server, &pairing, pairingIndex);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
}

/**
 * Wait for Pair-Setup timer to expire and potentially Thread transport to stop in all cases
 */
void WaitForPairSetupTimerAndPotentialTransportStop() {
    // Wait for pair-setup timeout
    HAPPlatformClockAdvance(PAIR_SETUP_TIMEOUT);

    // The above timeout could have triggered a timer to stop Thread transport,
    // or it could have stopped Thread immediately depending on the state of the Thread transport.
    // For this common function, we wait for the longest to ensure that the transport state
    // has changed if it is supposed to.
    HAPPlatformClockAdvance(HAPSecond);
}

/**
 * Tests Thread BLE capable accessory handling pair-setup timeout upon removing pairing
 */
static void TestHandlingPairSetupTimeoutCommonRoutine(int variant) {
    HAP_PLATFORM_THREAD_MOCK(threadMock);
    HAP_PLATFORM_BLE_MOCK(bleMock);

    bool threadInitialized = false;
    bool threadEnabled = false;
    bool bleInitialized = false;

    ALWAYS(threadMock, HAPPlatformThreadInitialize)
            .Do([&](void* server,
                    HAPPlatformThreadDeviceCapabilities deviceType,
                    uint32_t pollPeriod,
                    uint32_t childTimeout,
                    uint8_t txPower) { threadInitialized = true; });
    ALWAYS(threadMock, HAPPlatformThreadDeinitialize).Do([&](void* server) {
        threadInitialized = false;
        threadEnabled = false;
    });
    ALWAYS(threadMock, HAPPlatformThreadJoinCommissionedNetwork).Do([&]() {
        TEST_ASSERT(threadInitialized);
        threadEnabled = true;
    });
    ALWAYS(bleMock, HAPPlatformBLEInitialize).Do([&](HAPPlatformBLEPeripheralManager* manager) {
        bleInitialized = true;
    });
    ALWAYS(bleMock, HAPPlatformBLEDeinitialize).Do([&]() { bleInitialized = false; });
    HAPPlatformThreadBorderRouterStateCallback borderRouterCallback = NULL;
    void* borderRouterCallbackContext = NULL;
    ALWAYS(threadMock, HAPPlatformThreadRegisterBorderRouterStateCallback)
            .Do([&](HAPPlatformThreadBorderRouterStateCallback callback, void* context) {
                borderRouterCallback = callback;
                borderRouterCallbackContext = context;
            });

    // Add a pairing
    AddPairingEntry(0);

    HAPAccessoryServer* server = CreateBleThreadAccessoryServer();

    // Start testing with commissioned state
    EXPECT(threadMock, HAPPlatformThreadIsCommissioned).Return(true);
    // No clear parameters call expected
    EXPECT(threadMock, HAPPlatformThreadClearParameters).AtMost(0);

    // Start accessory server.
    HAPAccessoryServerStart(server, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(server), kHAPAccessoryServerState_Running);
    TEST_ASSERT(threadInitialized);
    TEST_ASSERT(!bleInitialized);

    // Thread must have been enabled because it is commissioned.
    TEST_ASSERT(threadEnabled);

    // Border router presence
    TEST_ASSERT(borderRouterCallback);
    borderRouterCallback(true, borderRouterCallbackContext);

    // Wait for pair-setup timeout
    WaitForPairSetupTimerAndPotentialTransportStop();

    // Thread should still be up
    TEST_ASSERT(threadEnabled);
    TEST_ASSERT(!bleInitialized);

    HAPPlatformClockAdvance(PAIR_SETUP_TIMEOUT);

    // Remove pairing
    HAPError err = HAPPairingRemoveAll(server);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    // Advance close to pair setup timeout
    HAPPlatformClockAdvance(PAIR_SETUP_TIMEOUT - HAPSecond);

    // Add a pairing
    HAPPairingIndex pairingIndex;
    TriggerPairingAdd(server, &pairingIndex);

    // Wait for timeout
    WaitForPairSetupTimerAndPotentialTransportStop();

    // Thread must be still up
    TEST_ASSERT(threadEnabled);
    TEST_ASSERT(!bleInitialized);

    // Add another pairing
    HAPPairingIndex secondPairingIndex;
    TriggerPairingAdd(server, &secondPairingIndex);

    HAPPlatformClockAdvance(HAPMinute);

    // Remove the previous pairing
    err = HAPPairingRemove(server, NULL, pairingIndex);
    TEST_ASSERT(!err);

    // Wait for timeout
    WaitForPairSetupTimerAndPotentialTransportStop();

    // Thread must be still up
    TEST_ASSERT(threadEnabled);
    TEST_ASSERT(!bleInitialized);

    if (variant == 0) {
        // Remove the pairing via HAPPairingRemoveAll() call.
        err = HAPPairingRemoveAll(server);
    } else {
        // Remove the pairing via HAPPairingRemove() call.
        err = HAPPairingRemove(server, NULL, secondPairingIndex);
    }
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    // Thread must be still up
    TEST_ASSERT(threadEnabled);
    TEST_ASSERT(!bleInitialized);

    // Wait for timeout
    WaitForPairSetupTimerAndPotentialTransportStop();

    // Thread must be decommissioned and BLE must be up
    HAPAssert(threadInitialized);
    HAPAssert(!bleInitialized);
    VERIFY_ALL(threadMock);

    VERIFY_ALL(bleMock);
}

/**
 * Tests Thread BLE capable accessory handling pair-setup timeout upon removing pairing.
 * First variant.
 */
TEST(TestHandlingPairSetupTimeoutVariant_0) {
    TestHandlingPairSetupTimeoutCommonRoutine(0);
}

/**
 * Tests Thread BLE capable accessory handling pair-setup timeout upon removing pairing.
 * Second variant.
 */
TEST(TestHandlingPairSetupTimeoutVariant_1) {
    TestHandlingPairSetupTimeoutCommonRoutine(1);
}

int main(int argc, char** argv) {
    HAPPlatformCreate();

    TEST_ASSERT(HAPGetCompatibilityVersion() == HAP_COMPATIBILITY_VERSION);

    // Setup key value store
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    HAPControllerPublicKey controllerPublicKey;
    TestPrepareKeyValueStore(controllerPairingID, &controllerPublicKey);

    return EXECUTE_TESTS(argc, (const char**) argv);
}

#endif
