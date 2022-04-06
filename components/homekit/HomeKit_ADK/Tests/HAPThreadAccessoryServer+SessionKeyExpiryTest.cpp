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
#include "Harness/HAPCharacteristicCryptoHelper.h"
#include "Harness/HAPCryptoHelper.h"
#include "Harness/HAPPlatformClockHelper.h"
#include "Harness/HAPPlatformTimerHelper.h"
#include "Harness/HAPThreadSessionHelper.h"
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
/** session key expiry timeout = one week in milliseconds */
#define SESSION_KEY_EXPIRY                               (1000ul * 60 * 60 * 24 * 7)

extern "C" {
static const HAPLogObject logObject = { .subsystem = "com.apple.mfi.HomeKit.Core.Test", .category = "TestController" };
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
        &accessoryInformationService,
        &hapProtocolInformationService,
        &pairingService,
        &lightBulbService,
        &threadManagementService,
        NULL,
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

    // Set session key expiry
    serverOptions.sessionKeyExpiry = SESSION_KEY_EXPIRY;

    static const HAPAccessoryServerCallbacks serverCallbacks = {
        .handleUpdatedState = HAPAccessoryServerHelperHandleUpdatedAccessoryServerState,
    };
    HAPAccessoryServerCreate(&accessoryServer, &serverOptions, &platform, &serverCallbacks, NULL);

    return &accessoryServer;
}

/**
 * Emulates a valid control message sent by the controller
 */
static void TriggerReadThreadRoleCharacteristic(
        HAPAccessoryServer* server,
        const HAPPlatformThreadCoAPManagerPeer* peer,
        HAPPlatformThreadCoAPManagerRequestCallback requestCallback,
        void* requestContext,
        uint8_t transactionId,
        const HAPSessionKey* encryptKey,
        uint64_t& encryptNonce,
        const HAPSessionKey* decryptKey,
        uint64_t& decryptNonce,
        std::function<void(HAPPlatformThreadCoAPManagerResponseCode)> handleResponseCode,
        std::function<void(uint8_t)> handleResponseStatus,
        std::function<void(uint16_t)> handleResponseValue) {
    HAPReadCharacteristicOverThreadHelper(
            server,
            platform.thread.coapManager,
            peer,
            requestCallback,
            requestContext,
            transactionId,
            threadManagementStatusCharacteristic.iid,
            encryptKey,
            encryptNonce,
            decryptKey,
            decryptNonce,
            handleResponseCode,
            handleResponseStatus,
            [&](HAPTLV tlv) {
                // The value response must contain data uint16 value
                TEST_ASSERT_EQUAL(tlv.value.numBytes, sizeof(uint16_t));
                uint16_t value = HAPReadLittleUInt16(tlv.value.bytes);
                handleResponseValue(value);
            });
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

// Test session key expiry
// The goal of the test is to ensure that after session key expires, control PDUs are neither encrypted or decrypted
// and that event/notification PDUs are still being encrypted correctly.
TEST(TestSessionKeyExpiry) {
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
            server, &threadSession->hapSession, &lightBulbBrightnessCharacteristic, &lightBulbService, &accessory);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    // Repeatedly read a characteristic every hour till session key expires
    uint8_t tid = 0;
    for (size_t i = 0; i < SESSION_KEY_EXPIRY / (1000 * 60 * 60); i++) {
        EXPECT(threadMock, HAPPlatformThreadGetRole)
                .Do([](HAPPlatformThreadDeviceRole* role) {
                    *role = kHAPPlatformThreadDeviceRole_Child;
                    return kHAPError_None;
                })
                .AtLeast(1);

        TriggerReadThreadRoleCharacteristic(
                server,
                &peer,
                secureMessageRequestCallback,
                secureMessageRequestContext,
                ++tid,
                &controllerToAccessoryKey,
                controllerToAccessoryNonce,
                &accessoryToControllerKey,
                accessoryToControllerNonce,
                [](HAPPlatformThreadCoAPManagerResponseCode code) {
                    TEST_ASSERT_EQUAL(code, kHAPPlatformThreadCoAPManagerResponseCode_Changed);
                },
                [](uint8_t status) { TEST_ASSERT_EQUAL(status, 0); },
                [](uint16_t value) { TEST_ASSERT_EQUAL(value, 8); }); // child

        // Also check that a notification is sent successfully.
        std::function<void()> completeSendEvent = NULL;
        EXPECT(coapManagerMock, HAPPlatformThreadCoAPManagerSendRequest)
                .If([=](HAPPlatformThreadCoAPManagerRef coapManager,
                        const HAPPlatformThreadCoAPManagerPeer* peerDest,
                        const char* uriPath,
                        HAPPlatformThreadCoAPManagerRequestCode requestCode,
                        const void* requestBytes,
                        size_t numRequestBytes,
                        HAPPlatformThreadCoAPManagerSendRequestCompletionHandler completionHandler,
                        void* _Nullable context) {
                    return (HAPIPAddressAreEqual(&peerDest->ipAddress, &peer.ipAddress) &&
                            peerDest->port == peer.port && HAPStringAreEqual(uriPath, ""));
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
                                TEST_ASSERT_EQUAL(iid, lightBulbBrightnessCharacteristic.iid);
                                HAPLogBufferDebug(
                                        &logObject,
                                        tlv.value.bytes,
                                        tlv.value.numBytes,
                                        "Notified lightbulb brightness");
                            });

                    TEST_ASSERT_EQUAL(err, kHAPError_None);

                    // Set up a callback after some delay to emulate real world notification completion.
                    completeSendEvent = [=]() {
                        completionHandler(
                                coapManager,
                                kHAPError_None,
                                peer,
                                kHAPPlatformThreadCoAPManagerResponseCode_Changed,
                                NULL,
                                0,
                                context);
                    };
                    return kHAPError_None;
                })
                .AtLeast(1);
        HAPAccessoryServerRaiseEvent(server, &lightBulbBrightnessCharacteristic, &lightBulbService, &accessory);
        HAPPlatformTimerEmulateClockAdvances(HAPSecond);
        VERIFY_ALL(coapManagerMock);

        // Emulate completion of the event sending
        TEST_ASSERT(completeSendEvent);
        completeSendEvent();

        // Advance an hour in total
        HAPPlatformTimerEmulateClockAdvances(HAPMinute * 60 - HAPSecond);

        VERIFY_ALL(threadMock);
    }

    // After session key expires, characteristic read request should fail
    EXPECT(threadMock, HAPPlatformThreadGetRole)
            .Do([](HAPPlatformThreadDeviceRole* role) {
                *role = kHAPPlatformThreadDeviceRole_Child;
                return kHAPError_None;
            })
            .AtMost(0);

    TriggerReadThreadRoleCharacteristic(
            server,
            &peer,
            secureMessageRequestCallback,
            secureMessageRequestContext,
            ++tid,
            &controllerToAccessoryKey,
            controllerToAccessoryNonce,
            &accessoryToControllerKey,
            accessoryToControllerNonce,
            [](HAPPlatformThreadCoAPManagerResponseCode code) {
                TEST_ASSERT_EQUAL(code, kHAPPlatformThreadCoAPManagerResponseCode_NotFound);
            },
            NULL,
            NULL);

    VERIFY_ALL(threadMock);

    // Notification should continue to succeed till session is released due to unreachability (24 hours).
    // Note that close one hour has passed since last controller read already and hence at i == 23,
    // it will be exact 24 hours since the last read.
    for (size_t i = 0; i < 24; i++) {
        // Check that a notification is sent successfully.
        std::function<void()> completeSendEvent = NULL;
        EXPECT(coapManagerMock, HAPPlatformThreadCoAPManagerSendRequest)
                .If([=](HAPPlatformThreadCoAPManagerRef coapManager,
                        const HAPPlatformThreadCoAPManagerPeer* peerDest,
                        const char* uriPath,
                        HAPPlatformThreadCoAPManagerRequestCode requestCode,
                        const void* requestBytes,
                        size_t numRequestBytes,
                        HAPPlatformThreadCoAPManagerSendRequestCompletionHandler completionHandler,
                        void* _Nullable context) {
                    return (HAPIPAddressAreEqual(&peerDest->ipAddress, &peer.ipAddress) &&
                            peerDest->port == peer.port && HAPStringAreEqual(uriPath, ""));
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
                                TEST_ASSERT_EQUAL(iid, lightBulbBrightnessCharacteristic.iid);
                                HAPLogBufferDebug(
                                        &logObject,
                                        tlv.value.bytes,
                                        tlv.value.numBytes,
                                        "Notified lightbulb brightness");
                            });

                    completeSendEvent = [=]() {
                        completionHandler(
                                coapManager,
                                kHAPError_None,
                                peer,
                                kHAPPlatformThreadCoAPManagerResponseCode_Changed,
                                NULL,
                                0,
                                context);
                    };
                    return kHAPError_None;
                })
                .AtLeast(1);
        HAPAccessoryServerRaiseEvent(server, &lightBulbBrightnessCharacteristic, &lightBulbService, &accessory);
        HAPPlatformTimerEmulateClockAdvances(HAPSecond);
        VERIFY_ALL(coapManagerMock);

        // Emulate completion of the event sending
        TEST_ASSERT(completeSendEvent);
        completeSendEvent();

        // Advance an hour in total
        HAPPlatformTimerEmulateClockAdvances(HAPMinute * 60 - HAPSecond);
    }

    // Check that a notification is not sent successfully as the session must have been released.
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
            .Return(kHAPError_None)
            .AtMost(0);
    HAPAccessoryServerRaiseEvent(server, &lightBulbBrightnessCharacteristic, &lightBulbService, &accessory);
    HAPPlatformTimerEmulateClockAdvances(HAPSecond);
    VERIFY_ALL(coapManagerMock);
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
