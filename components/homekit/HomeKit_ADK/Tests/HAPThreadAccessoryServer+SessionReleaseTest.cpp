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

#include "HAPPlatform+Init.h"

#include "HAP+KeyValueStoreDomains.h"
#include "HAPPDU+NotificationConfiguration.h"
#include "HAPPDU+TLV.h"
#include "HAPPlatformSetup+Init.h"
#include "Harness/HAPAccessoryServerHelper.h"
#include "Harness/HAPPlatformClockHelper.h"
#include "Harness/HAPPlatformTimerHelper.h"
#include "Harness/IPTestHelpers.h"
#include "Harness/SecurityTestHelpers.h"
#include "Harness/TemplateDB.h"
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
#include "Harness/HAPPlatformCoAPManagerMock.h"
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
 * Spec fixed it to 30 seconds.
 */
#define THREAD_JOIN_ATTACH_TIMEOUT                       65000
/**
 * AttachTimeout in milliseconds to use with Set Thread Parameters operation.
 *
 * Spec fixed to 30 seconds.
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
/** controller reachability timeout */
#define CONTROLLER_REACHABILITY_TIMEOUT                  ((HAPTime)(10 * HAPMinute))

extern "C" {
// static const HAPLogObject logObject = { .subsystem = "com.apple.mfi.HomeKit.Core.Test", .category = "TestController"
// };
}

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
 * Finds a Thread session corresponding to a peer address
 */
static HAPThreadSession* FindThreadSession(HAPAccessoryServer* server, const HAPPlatformThreadCoAPManagerPeer* peer) {
    for (size_t i = 0; i < server->thread.storage->numSessions; i++) {
        HAPThreadSession* threadSession = &server->thread.storage->sessions[i];
        if (threadSession->lastActivity && HAPIPAddressAreEqual(&peer->ipAddress, &threadSession->peer.ipAddress) &&
            peer->port == threadSession->peer.port) {
            return threadSession;
        }
    }
    return NULL;
}

/**
 * Emulates pair verify procedure.
 */
static void TriggerPairVerify(
        HAPAccessoryServer* server,
        const HAPPlatformThreadCoAPManagerPeer* peer,
        HAPPlatformThreadCoAPManagerRequestCallback requestCallback,
        void* requestContext) {
    // Inject M1 to use Thread session storage
    uint8_t m1RequestBytes[] = { 0x06, 0x01, 0x01, // kTLVType_State M1
                                 0x03, 0x20, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
                                 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55,
                                 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff };
    HAPPlatformThreadCoAPManagerResponseCode responseCode;
    uint8_t responseBytes[512];
    size_t numResponseBytes;
    requestCallback(
            platform.thread.coapManager,
            peer,
            server->thread.coap.pairVerifyResource,
            kHAPPlatformThreadCoAPManagerRequestCode_POST,
            m1RequestBytes,
            sizeof m1RequestBytes,
            &responseCode,
            responseBytes,
            sizeof responseBytes,
            &numResponseBytes,
            NULL,
            requestContext);
    TEST_ASSERT_EQUAL(responseCode, kHAPPlatformThreadCoAPManagerResponseCode_Changed);

    // For this test suite, skip pair verify steps and fake the success
    HAPThreadSession* threadSession = FindThreadSession(server, peer);
    TEST_ASSERT(threadSession);
    HAPRawBufferZero(&threadSession->hapSession.state.pairVerify, sizeof threadSession->hapSession.state.pairVerify);
    HAPRawBufferZero(&threadSession->hapSession.hap, sizeof threadSession->hapSession.hap);
    threadSession->hapSession.hap.active = true;
    threadSession->hapSession.hap.timestamp = HAPPlatformClockGetCurrent();
}

/**
 * Triggers a decryption failure
 */
static void TriggerDecryptionFailure(
        HAPAccessoryServer* server,
        const HAPPlatformThreadCoAPManagerPeer* peer,
        HAPPlatformThreadCoAPManagerRequestCallback requestCallback,
        void* requestContext) {
    // Bad text
    uint8_t requestBytes[] = "Some bad text that cannot be decrypted";
    HAPPlatformThreadCoAPManagerResponseCode responseCode;
    uint8_t responseBytes[512];
    size_t numResponseBytes;
    requestCallback(
            platform.thread.coapManager,
            peer,
            server->thread.coap.secureMessageResource,
            kHAPPlatformThreadCoAPManagerRequestCode_POST,
            requestBytes,
            sizeof requestBytes,
            &responseCode,
            responseBytes,
            sizeof responseBytes,
            &numResponseBytes,
            NULL,
            requestContext);
    TEST_ASSERT_EQUAL(responseCode, kHAPPlatformThreadCoAPManagerResponseCode_NotFound);
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
    // Clears all timers.
    HAPPlatformTimerDeregisterAll();
}

TEST(TestHandlingDecryptionFailure) {
    HAP_PLATFORM_THREAD_MOCK(threadMock);
    HAP_PLATFORM_COAP_MANAGER_MOCK(coapManagerMock);

    HAPPlatformThreadBorderRouterStateCallback borderRouterCallback = NULL;
    void* borderRouterCallbackContext = NULL;
    ALWAYS(threadMock, HAPPlatformThreadRegisterBorderRouterStateCallback)
            .Do([&](HAPPlatformThreadBorderRouterStateCallback callback, void* context) {
                borderRouterCallback = callback;
                borderRouterCallbackContext = context;
            });

    ALWAYS(threadMock, HAPPlatformThreadGetRole).Do([](HAPPlatformThreadDeviceRole* role) {
        *role = kHAPPlatformThreadDeviceRole_Child;
        return kHAPError_None;
    });

    // Assume a pairing entry already exists
    AddPairingEntry(0);

    HAPAccessoryServer* server = CreateBleThreadAccessoryServer();

    // Start testing with commissioned state
    EXPECT(threadMock, HAPPlatformThreadIsCommissioned).Return(true);
    // Thread enable expected
    EXPECT(threadMock, HAPPlatformThreadInitialize).AtLeast(1);
    EXPECT(threadMock, HAPPlatformThreadDeinitialize).AtMost(0);
    EXPECT(threadMock, HAPPlatformThreadJoinCommissionedNetwork).AtLeast(1);
    // No clear parameters call expected
    EXPECT(threadMock, HAPPlatformThreadClearParameters).AtMost(0);

    HAPPlatformThreadCoAPManagerRequestCallback secureMessageRequestCallback = NULL;
    void* secureMessageRequestContext = NULL;
    HAPPlatformThreadCoAPManagerRequestCallback pairVerifyRequestCallback = NULL;
    void* pairVerifyRequestContext = NULL;

    // CoAP resources must be registered
    EXPECT(coapManagerMock, HAPPlatformThreadCoAPManagerAddResource)
            .If([&](HAPPlatformThreadCoAPManagerRef coapManager,
                    const char* uriPath,
                    HAPPlatformThreadCoAPManagerRequestCallback callback,
                    void* _Nullable context,
                    HAPPlatformThreadCoAPManagerResourceRef* coapResource) { return HAPStringAreEqual(uriPath, ""); })
            .Do([&](HAPPlatformThreadCoAPManagerRef coapManager,
                    const char* uriPath,
                    HAPPlatformThreadCoAPManagerRequestCallback callback,
                    void* _Nullable context,
                    HAPPlatformThreadCoAPManagerResourceRef* coapResource) {
                secureMessageRequestCallback = callback;
                secureMessageRequestContext = context;
                *coapResource = (HAPPlatformThreadCoAPManagerResourceRef) "";
                return kHAPError_None;
            })
            .AtLeast(1);

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

    // Start accessory server.
    HAPAccessoryServerStart(server, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(server), kHAPAccessoryServerState_Running);

    VERIFY_ALL(threadMock);
    VERIFY_ALL(coapManagerMock);

    // No thread disable expected
    EXPECT(threadMock, HAPPlatformThreadDeinitialize).AtMost(0);

    // Border router presence
    TEST_ASSERT(borderRouterCallback);
    borderRouterCallback(true, borderRouterCallbackContext);

    // Fake pair verify
    HAPPlatformThreadCoAPManagerPeer peer;
    peer.ipAddress.version = kHAPIPAddressVersion_IPv6;
    uint8_t address[] = {
        0x20, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
    };
    HAPRawBufferCopyBytes((void*) peer.ipAddress._.ipv6.bytes, (void*) address, sizeof address);
    peer.port = 0x1234;
    TriggerPairVerify(server, &peer, pairVerifyRequestCallback, pairVerifyRequestContext);

    HAPPlatformClockAdvance(HAPSecond);

    // Queue a notification in order to test that thread session storages are cleared properly.

    // Fake notify enable
    HAPThreadSession* threadSession = FindThreadSession(server, &peer);
    TEST_ASSERT(threadSession);
    HAPError err = HAPNotificationHandleRegisterRequest(
            server,
            &threadSession->hapSession,
            &threadManagementStatusCharacteristic,
            &threadManagementService,
            &accessory);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    HAPPlatformClockAdvance(HAPSecond);

    // Fake CoAP busy so that notification will get queued in Thread session storage.
    EXPECT(coapManagerMock, HAPPlatformThreadCoAPManagerSendRequest).Return(kHAPError_Busy).AtLeast(1);

    // Fake a notification queued
    HAPAccessoryServerRaiseEvent(server, &threadManagementStatusCharacteristic, &threadManagementService, &accessory);

    // Decryption failure
    TriggerDecryptionFailure(server, &peer, secureMessageRequestCallback, secureMessageRequestContext);

    HAPPlatformClockAdvance(HAPSecond);

    // HAP session must have been released
    TEST_ASSERT(!threadSession->hapSession.hap.active);

    // Advance time so that controller reachability timer will be over
    HAPPlatformClockAdvance(CONTROLLER_REACHABILITY_TIMEOUT);

    // Fake pair verify with another port.
    // This step would trigger assertions on the reachability expired sessions
    // and would detect potential issue with session not being cleared properly above
    // when handling decryption failure.
    peer.port = 0x1235;
    TriggerPairVerify(server, &peer, pairVerifyRequestCallback, pairVerifyRequestContext);

    VERIFY_ALL(coapManagerMock);
    VERIFY_ALL(threadMock);

    // Bring down thread to verify that there is no issue in bringing down Thread
    // after the decryption failure above.

    // Make network non-viable.
    borderRouterCallback(false, borderRouterCallbackContext);

    // Thread should be kept on
    EXPECT(threadMock, HAPPlatformThreadDeinitialize).AtMost(0);

    // Let the border router detection timeout occur.
    HAPPlatformClockAdvance(THREAD_DETECTION_TIMEOUT);

    // The above must have triggered another timer to shutdown thread.
    // Advance one more second for that timer to expire.
    HAPPlatformClockAdvance(HAPSecond);

    VERIFY_ALL(threadMock);
}

int main(int argc, char** argv) {
    HAPPlatformCreate();
    HAPPlatformSetupDrivers();

    TEST_ASSERT(HAPGetCompatibilityVersion() == HAP_COMPATIBILITY_VERSION);

    // Setup key value store
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    HAPControllerPublicKey controllerPublicKey;
    TestPrepareKeyValueStore(controllerPairingID, &controllerPublicKey);

    return EXECUTE_TESTS(argc, (const char**) argv);
}

#endif
