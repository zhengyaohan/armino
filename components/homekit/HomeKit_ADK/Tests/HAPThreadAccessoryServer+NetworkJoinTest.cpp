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
#include "HAPPDU+TLV.h"

#include "HAPPlatform+Init.h"
#include "HAPPlatformBLEPeripheralManager+Init.h"

#include "Harness/HAPAccessoryServerHelper.h"
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
#include "Harness/HAPPlatformRunLoopMock.h"
#include "Harness/HAPPlatformServiceDiscoveryMock.h"
#include "Harness/HAPPlatformThreadMock.h"
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

extern "C" {

/**
 * Custom function to get reattach delay
 */
static HAPTime GetCustomReattachDelay(void* _Nullable context, uint32_t reattachCount) {
    HAPPrecondition(context);
    uint32_t* reattemptPeriod = (uint32_t*) context;
    return HAPMillisecond * (*reattemptPeriod);
}
}

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

/**
 * Create a BLE and Thread accessory server
 *
 * @param reattemptPeriod  Thread reattempt period in milliseconds
 * @return created accessory server
 */
static HAPAccessoryServer* CreateBleThreadAccessoryServer(uint32_t reattemptPeriod) {
    static uint32_t constantReattemptPeriod = reattemptPeriod;
    return CreateBleThreadAccessoryServer(
            kHAPPlatformThreadDeviceCapabilities_MED, GetCustomReattachDelay, (void*) &constantReattemptPeriod);
}

/**
 * Create a Thread only accessory server
 */
static HAPAccessoryServer* CreateThreadAccessoryServer(void) {
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

    static const HAPPlatformThreadDeviceCapabilities deviceType = kHAPPlatformThreadDeviceCapabilities_MED;
    serverOptions.thread.deviceParameters.deviceType = deviceType;
    serverOptions.thread.getNextReattachDelay = GetNextThreadReattachDelay;
    serverOptions.thread.getNextReattachDelayContext = (void*) &deviceType;

    // Initialize Thread platform used in accessory server
    platform.thread.coapManager = &coapManager;
    platform.thread.serviceDiscovery = &threadServiceDiscovery;

    static const HAPAccessoryServerCallbacks callbacks = {
        .handleUpdatedState = HAPAccessoryServerHelperHandleUpdatedAccessoryServerState
    };
    HAPAccessoryServerCreate(&accessoryServer, &serverOptions, &platform, &callbacks, NULL);

    return &accessoryServer;
}

static HAPSession CreateFakeThreadHAPSession(HAPAccessoryServer* server, HAPPlatformKeyValueStoreKey pairingKey) {
    HAPSession session = {
        .server = server,
    };
    session.hap.active = true;
    session.hap.pairingID = pairingKey;
    session.transportType = kHAPTransportType_Thread;
    return session;
}

static HAPSession CreateFakeBLEHAPSession(HAPAccessoryServer* server, HAPPlatformKeyValueStoreKey pairingKey) {
    HAPSession session = {
        .server = server,
    };
    session.hap.active = true;
    session.hap.pairingID = pairingKey;
    session.transportType = kHAPTransportType_BLE;
    return session;
}

/* Common mock functions */
HAP_PLATFORM_BLE_MOCK(bleMock);
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
 * Set up fake session keys
 *
 * @param sessions HAPSession array
 * @param count    number of sessions in the array
 */
static void SetupFakeSessionKeys(HAPSession* sessions, size_t count) {
    for (size_t i = 0; i < count; i++) {
        HAPPlatformRandomNumberFill(sessions[i].state.pairVerify.cv_SK, sizeof(sessions[i].state.pairVerify.cv_SK));
        HAP_X25519_scalarmult_base(sessions[i].state.pairVerify.Controller_cv_PK, sessions[i].state.pairVerify.cv_SK);
        HAPPlatformRandomNumberFill(sessions[i].state.pairVerify.cv_SK, sizeof(sessions[i].state.pairVerify.cv_SK));
        HAP_X25519_scalarmult_base(sessions[i].state.pairVerify.cv_PK, sessions[i].state.pairVerify.cv_SK);
        HAP_X25519_scalarmult(
                sessions[i].state.pairVerify.cv_KEY,
                sessions[i].state.pairVerify.cv_SK,
                sessions[i].state.pairVerify.Controller_cv_PK);
        HAPRawBufferCopyBytes(
                sessions[i].hap.cv_KEY, sessions[i].state.pairVerify.cv_KEY, sizeof(sessions[i].hap.cv_KEY));
        HAPRawBufferZero(&sessions[i].state.pairVerify, sizeof(sessions[i].state.pairVerify));
    }
}

/**
 * Make a callback and clears the state to receive potentially another start joiner call
 * @param joined  true to pretend that Thread joined the network. false, otherwise.
 */
static void
        SetReachabilityAndMakeCallback(HAPPlatformRunLoopCallback* callback, HAPAccessoryServer* server, bool joined) {
    HAPPrecondition(callback);

    HAPAccessoryServerSetThreadNetworkReachable(server, joined, false);
    // In order to verify that the callback might have been set again from within the callback,
    // the callback is copied and the pointer is set to NULL;
    HAPPlatformRunLoopCallback callbackCopy = *callback;
    *callback = NULL;
    void* runLoopArg = &server;
    callbackCopy(runLoopArg, sizeof runLoopArg);
}

/**
 * Emulate a Initiate Thread Joiner command to accessory
 *
 * @param session        HAP session
 * @param server         accessory server
 * @param transactionId  transaction ID of the message to send
 */
static void
        TriggerInitiateThreadJoinerOperation(HAPSession* session, HAPAccessoryServer* server, uint8_t transactionId) {
    uint8_t requestBuffer[1024];
    HAPTLVWriter requestWriter;
    void* requestBytes;
    size_t numRequestBytes;

    HAPTLVWriterCreate(&requestWriter, requestBuffer, sizeof requestBuffer);

    static const uint8_t tlvBytes[] = { 0x04 };
    static const HAPTLV tlv = { .value = {
                                        .bytes = tlvBytes,
                                        .numBytes = 1,
                                },
                                .type = 1, // operation type
                              };
    HAPError err = HAPTLVWriterAppend(&requestWriter, &tlv);
    HAPAssert(!err);

    HAPTLVWriterGetBuffer(&requestWriter, &requestBytes, &numRequestBytes);

    uint8_t requestPdu[numRequestBytes + 12];
    requestPdu[0] = 0x00;          // Write request
    requestPdu[1] = 0x02;          // HAP PDU Type
    requestPdu[2] = transactionId; // TID
    requestPdu[3] = threadManagementControlCharacteristic.iid & 0xff;
    requestPdu[4] = (threadManagementControlCharacteristic.iid >> 8) & 0xff;
    requestPdu[5] = (numRequestBytes + 5) & 0xff; // body length
    requestPdu[6] = ((numRequestBytes + 5) >> 8) & 0xff;
    requestPdu[7] = kHAPPDUTLVType_ReturnResponse; // HAP-Param-Return-Response
    requestPdu[8] = 0x01;                          // length
    requestPdu[9] = 1;                             // value = set
    requestPdu[10] = kHAPPDUTLVType_Value;         // HAP-Param-Value
    requestPdu[11] = numRequestBytes & 0xff;
    HAPAssert(numRequestBytes < 256);
    HAPRawBufferCopyBytes(&requestPdu[12], requestBytes, numRequestBytes);

    uint8_t responseBytes[1024];
    size_t numResponseBytes;
    size_t bytesConsumed;
    bool requestSuccessful; // TODO: Can this be updated to assert this value?
    err = HAPPDUProcedureHandleRequest(
            server,
            session,
            requestPdu,
            sizeof requestPdu,
            responseBytes,
            sizeof responseBytes,
            &numResponseBytes,
            &bytesConsumed,
            &requestSuccessful);
    HAPAssert(!err);

    HAPAssert(numResponseBytes >= 3);
    HAPAssert(responseBytes[0] == 0x02); // Write response
    HAPAssert(responseBytes[1] == transactionId);
    HAPAssert(responseBytes[2] == 0); // status

    if (numResponseBytes > 3) {
        HAPAssert(numResponseBytes >= 5);
        size_t bodyLength = (size_t) responseBytes[3] + ((size_t) responseBytes[4] << 8);
        HAPAssert(numResponseBytes - 5 == bodyLength);

        HAPTLVReader bodyReader;
        HAPTLVReaderCreate(&bodyReader, &responseBytes[5], bodyLength);

        for (;;) {
            HAPTLV tlv;
            bool valid;
            err = HAPTLVReaderGetNext(&bodyReader, &valid, &tlv);
            TEST_ASSERT(!err);
            if (!valid) {
                break;
            }
            switch (tlv.type) {
                case kHAPPDUTLVType_Value: {
                    // The value response should be empty
                    TEST_ASSERT_EQUAL(tlv.value.numBytes, 0u);
                    break;
                }
                default: {
                    TEST_FAIL("Unexpected TLV 0x%02x included in the response @ %s:%d", tlv.type, HAP_FILE, __LINE__);
                }
            }
        }
    }
}

/**
 * Emulate a Set Thread Parameters command to accessory
 *
 * @param session        HAP session
 * @param server         accessory server
 * @param transactionId  transaction ID of the message to send
 * @param expectedStatus expected status result
 */
static void TriggerSetThreadParametersOperation(
        HAPSession* session,
        HAPAccessoryServer* server,
        uint8_t transactionId,
        uint8_t expectedStatus) {
    uint8_t requestBuffer[1024];
    HAPTLVWriter requestWriter;
    void* requestBytes;
    size_t numRequestBytes;

    HAPTLVWriterCreate(&requestWriter, requestBuffer, sizeof requestBuffer);

    static const uint8_t tlvBytes[] = { 0x01 };
    static const HAPTLV tlv = { .value = {
                                        .bytes = tlvBytes,
                                        .numBytes = 1,
                                },
                                .type = 1, // operation type
                              };

    HAPError err = HAPTLVWriterAppend(&requestWriter, &tlv);
    HAPAssert(!err);

    uint8_t subBuffer[1024];
    HAPTLVWriter subWriter;
    HAPTLVWriterCreate(&subWriter, subBuffer, sizeof subBuffer);
    static const HAPTLV networkNameTLV = { .value = {
                                                   .bytes = "My Network",
                                                   .numBytes = 10,
                                           },
                                           .type = 1, // network name
                                         };
    err = HAPTLVWriterAppend(&subWriter, &networkNameTLV);
    HAPAssert(!err);

    static const uint8_t channelBytes[] = {
        THREAD_SET_PARAMS_CHANNEL & 0xff,
        (THREAD_SET_PARAMS_CHANNEL >> 8) & 0xff,
    };
    static const HAPTLV channelTLV = { .value = {
                                               .bytes = channelBytes,
                                               .numBytes = 2,
                                       },
                                       .type = 2, // channel
                                     };
    err = HAPTLVWriterAppend(&subWriter, &channelTLV);
    HAPAssert(!err);

    static const uint8_t panIdBytes[] = {
        THREAD_SET_PARAMS_PANID & 0xff,
        (THREAD_SET_PARAMS_PANID >> 8) & 0xff,
    };
    static const HAPTLV panIdTLV = { .value = {
                                             .bytes = panIdBytes,
                                             .numBytes = 2,
                                     },
                                     .type = 3, // pan id
                                   };
    err = HAPTLVWriterAppend(&subWriter, &panIdTLV);
    HAPAssert(!err);

    static const uint8_t exPanIdBytes[] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 };
    static const HAPTLV exPanIdTLV = {
                                       .value = {
                                               .bytes = exPanIdBytes,
                                               .numBytes = 8,
                                       },
                                       .type = 4, // ext pan id
                                     };
    err = HAPTLVWriterAppend(&subWriter, &exPanIdTLV);
    HAPAssert(!err);

    static const uint8_t masterKeyBytes[] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                                              0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00 };
    static const HAPTLV masterKeyTLV = { .value = {
                                                 .bytes = masterKeyBytes,
                                                 .numBytes = 16,
                                         },
                                         .type = 5, // master key
                                       };
    err = HAPTLVWriterAppend(&subWriter, &masterKeyTLV);
    HAPAssert(!err);

    HAPTLVWriterGetBuffer(&subWriter, &requestBytes, &numRequestBytes);

    HAPTLV networkCredentialsTLV = { .value = {
                                             .bytes = requestBytes,
                                             .numBytes = numRequestBytes,
                                     },
                                     .type = 2, // network credentials
                                   };
    err = HAPTLVWriterAppend(&requestWriter, &networkCredentialsTLV);

    static const uint8_t formingAllowedBytes[] = { 0 };
    static const HAPTLV formingAllowedTLV = { .value = {
                                                      .bytes = formingAllowedBytes,
                                                      .numBytes = 1,
                                              },
                                              .type = 3, // forming allowed
                                            };
    err = HAPTLVWriterAppend(&requestWriter, &formingAllowedTLV);
    HAPAssert(!err);

    HAPTLVWriterGetBuffer(&requestWriter, &requestBytes, &numRequestBytes);

    uint8_t requestPdu[numRequestBytes + 12];
    requestPdu[0] = 0x00;          // Write request
    requestPdu[1] = 0x02;          // HAP PDU Type
    requestPdu[2] = transactionId; // TID
    requestPdu[3] = threadManagementControlCharacteristic.iid & 0xff;
    requestPdu[4] = (threadManagementControlCharacteristic.iid >> 8) & 0xff;
    requestPdu[5] = (numRequestBytes + 5) & 0xff; // body length
    requestPdu[6] = ((numRequestBytes + 5) >> 8) & 0xff;
    requestPdu[7] = kHAPPDUTLVType_ReturnResponse; // HAP-Param-Return-Response
    requestPdu[8] = 0x01;                          // length
    requestPdu[9] = 1;                             // value = set
    requestPdu[10] = kHAPPDUTLVType_Value;         // HAP-Param-Value
    requestPdu[11] = numRequestBytes & 0xff;
    HAPAssert(numRequestBytes < 256);
    HAPRawBufferCopyBytes(&requestPdu[12], requestBytes, numRequestBytes);

    uint8_t responseBytes[1024];
    size_t numResponseBytes;
    size_t bytesConsumed;
    bool requestSuccessful; // TODO: Can this be updated to assert this value?
    err = HAPPDUProcedureHandleRequest(
            server,
            session,
            requestPdu,
            sizeof requestPdu,
            responseBytes,
            sizeof responseBytes,
            &numResponseBytes,
            &bytesConsumed,
            &requestSuccessful);
    TEST_ASSERT(!err);

    TEST_ASSERT(numResponseBytes >= 3);
    TEST_ASSERT_EQUAL(responseBytes[0], 0x02); // Write response
    TEST_ASSERT_EQUAL(responseBytes[1], transactionId);
    TEST_ASSERT_EQUAL(responseBytes[2], expectedStatus); // status

    if (numResponseBytes > 3) {
        TEST_ASSERT(numResponseBytes >= 5);
        size_t bodyLength = (size_t) responseBytes[3] + ((size_t) responseBytes[4] << 8);
        TEST_ASSERT_EQUAL(numResponseBytes - 5, bodyLength);

        HAPTLVReader bodyReader;
        HAPTLVReaderCreate(&bodyReader, &responseBytes[5], bodyLength);

        for (;;) {
            HAPTLV tlv;
            bool valid;
            err = HAPTLVReaderGetNext(&bodyReader, &valid, &tlv);
            TEST_ASSERT(!err);
            if (!valid) {
                break;
            }
            switch (tlv.type) {
                case kHAPPDUTLVType_Value: {
                    // The value response should be empty
                    TEST_ASSERT_EQUAL(tlv.value.numBytes, 0u);
                    break;
                }
                default: {
                    TEST_FAIL("Unexpected TLV 0x%02x included in the response @ %s:%d", tlv.type, HAP_FILE, __LINE__);
                }
            }
        }
    }
}

/**
 * Emulate an empty Set Thread Parameters command to accessory
 *
 * @param session        HAP session
 * @param server         accessory server
 * @param transactionId  transaction ID of the message to send
 * @param expectedStatus expected status result
 */
static void TriggerEmptySetThreadParametersOperation(
        HAPSession* session,
        HAPAccessoryServer* server,
        uint8_t transactionId,
        uint8_t expectedStatus) {
    uint8_t requestBuffer[1024];
    HAPTLVWriter requestWriter;
    void* requestBytes;
    size_t numRequestBytes;

    HAPTLVWriterCreate(&requestWriter, requestBuffer, sizeof requestBuffer);

    static const uint8_t tlvBytes[] = { 0x01 };
    static const HAPTLV tlv = { .value = {
                                        .bytes = tlvBytes,
                                        .numBytes = 1,
                                },
                                .type = 1, // operation type
                              };

    HAPError err = HAPTLVWriterAppend(&requestWriter, &tlv);
    HAPAssert(!err);
    HAPTLVWriterGetBuffer(&requestWriter, &requestBytes, &numRequestBytes);

    uint8_t requestPdu[numRequestBytes + 12];
    requestPdu[0] = 0x00;          // Write request
    requestPdu[1] = 0x02;          // HAP PDU Type
    requestPdu[2] = transactionId; // TID
    requestPdu[3] = threadManagementControlCharacteristic.iid & 0xff;
    requestPdu[4] = (threadManagementControlCharacteristic.iid >> 8) & 0xff;
    requestPdu[5] = (numRequestBytes + 5) & 0xff; // body length
    requestPdu[6] = ((numRequestBytes + 5) >> 8) & 0xff;
    requestPdu[7] = kHAPPDUTLVType_ReturnResponse; // HAP-Param-Return-Response
    requestPdu[8] = 0x01;                          // length
    requestPdu[9] = 1;                             // value = set
    requestPdu[10] = kHAPPDUTLVType_Value;         // HAP-Param-Value
    requestPdu[11] = numRequestBytes & 0xff;
    HAPAssert(numRequestBytes < 256);
    HAPRawBufferCopyBytes(&requestPdu[12], requestBytes, numRequestBytes);

    uint8_t responseBytes[1024];
    size_t numResponseBytes;
    size_t bytesConsumed;
    bool requestSuccessful; // TODO: Can this be updated to assert this value?
    err = HAPPDUProcedureHandleRequest(
            server,
            session,
            requestPdu,
            sizeof requestPdu,
            responseBytes,
            sizeof responseBytes,
            &numResponseBytes,
            &bytesConsumed,
            &requestSuccessful);
    TEST_ASSERT(!err);

    TEST_ASSERT(numResponseBytes >= 3);
    TEST_ASSERT_EQUAL(responseBytes[0], 0x02); // Write response
    TEST_ASSERT_EQUAL(responseBytes[1], transactionId);
    TEST_ASSERT_EQUAL(responseBytes[2], expectedStatus); // status

    if (numResponseBytes > 3) {
        TEST_ASSERT(numResponseBytes >= 5);
        size_t bodyLength = (size_t) responseBytes[3] + ((size_t) responseBytes[4] << 8);
        TEST_ASSERT_EQUAL(numResponseBytes - 5, bodyLength);

        HAPTLVReader bodyReader;
        HAPTLVReaderCreate(&bodyReader, &responseBytes[5], bodyLength);

        for (;;) {
            HAPTLV tlv;
            bool valid;
            err = HAPTLVReaderGetNext(&bodyReader, &valid, &tlv);
            TEST_ASSERT(!err);
            if (!valid) {
                break;
            }
            switch (tlv.type) {
                case kHAPPDUTLVType_Value: {
                    // The value response should be empty
                    TEST_ASSERT_EQUAL(tlv.value.numBytes, 0u);
                    break;
                }
                default: {
                    TEST_FAIL("Unexpected TLV 0x%02x included in the response @ %s:%d", tlv.type, HAP_FILE, __LINE__);
                }
            }
        }
    }
}

/**
 * Emulate an empty Thread Control command to accessory
 *
 * @param session        HAP session
 * @param server         accessory server
 * @param transactionId  transaction ID of the message to send
 * @param expectedStatus expected status result
 */
static void TriggerEmptyThreadControlOperation(
        HAPSession* session,
        HAPAccessoryServer* server,
        uint8_t transactionId,
        uint8_t expectedStatus) {
    int err;
    uint8_t requestPdu[12];
    uint16_t numRequestBytes = 0;
    requestPdu[0] = 0x00;          // Write request
    requestPdu[1] = 0x02;          // HAP PDU Type
    requestPdu[2] = transactionId; // TID
    requestPdu[3] = threadManagementControlCharacteristic.iid & 0xff;
    requestPdu[4] = (threadManagementControlCharacteristic.iid >> 8) & 0xff;
    requestPdu[5] = (numRequestBytes + 5) & 0xff; // body length
    requestPdu[6] = ((numRequestBytes + 5) >> 8) & 0xff;
    requestPdu[7] = kHAPPDUTLVType_ReturnResponse; // HAP-Param-Return-Response
    requestPdu[8] = 0x01;                          // length
    requestPdu[9] = 1;                             // value = set
    requestPdu[10] = kHAPPDUTLVType_Value;         // HAP-Param-Value
    requestPdu[11] = numRequestBytes & 0xff;

    uint8_t responseBytes[1024];
    size_t numResponseBytes;
    size_t bytesConsumed;
    bool requestSuccessful; // TODO: Can this be updated to assert this value?
    err = HAPPDUProcedureHandleRequest(
            server,
            session,
            requestPdu,
            sizeof requestPdu,
            responseBytes,
            sizeof responseBytes,
            &numResponseBytes,
            &bytesConsumed,
            &requestSuccessful);
    TEST_ASSERT(!err);

    TEST_ASSERT(numResponseBytes >= 3);
    TEST_ASSERT_EQUAL(responseBytes[0], 0x02); // Write response
    TEST_ASSERT_EQUAL(responseBytes[1], transactionId);
    TEST_ASSERT_EQUAL(responseBytes[2], expectedStatus); // status

    if (numResponseBytes > 3) {
        TEST_ASSERT(numResponseBytes >= 5);
        size_t bodyLength = (size_t) responseBytes[3] + ((size_t) responseBytes[4] << 8);
        TEST_ASSERT_EQUAL(numResponseBytes - 5, bodyLength);

        HAPTLVReader bodyReader;
        HAPTLVReaderCreate(&bodyReader, &responseBytes[5], bodyLength);

        for (;;) {
            HAPTLV tlv;
            bool valid;
            err = HAPTLVReaderGetNext(&bodyReader, &valid, &tlv);
            TEST_ASSERT(!err);
            if (!valid) {
                break;
            }
            switch (tlv.type) {
                case kHAPPDUTLVType_Value: {
                    // The value response should be empty
                    TEST_ASSERT_EQUAL(tlv.value.numBytes, 0u);
                    break;
                }
                default: {
                    TEST_FAIL("Unexpected TLV 0x%02x included in the response @ %s:%d", tlv.type, HAP_FILE, __LINE__);
                }
            }
        }
    }
}

/**
 * Emulate a Clear Thread Parameters command to accessory
 *
 * @param session        HAP session
 * @param server         accessory server
 * @param transactionId  transaction ID of the message to send
 */
static void
        TriggerClearThreadParametersOperation(HAPSession* session, HAPAccessoryServer* server, uint8_t transactionId) {
    uint8_t requestBuffer[1024];
    HAPTLVWriter requestWriter;
    void* requestBytes;
    size_t numRequestBytes;

    HAPTLVWriterCreate(&requestWriter, requestBuffer, sizeof requestBuffer);

    static const uint8_t tlvBytes[] = { 0x02 };
    static const HAPTLV tlv = { .value = {
                                        .bytes = tlvBytes,
                                        .numBytes = 1,
                                },
                                .type = 1, // operation type
                              };
    HAPError err = HAPTLVWriterAppend(&requestWriter, &tlv);
    HAPAssert(!err);

    HAPTLVWriterGetBuffer(&requestWriter, &requestBytes, &numRequestBytes);

    uint8_t requestPdu[numRequestBytes + 12];
    requestPdu[0] = 0x00;          // Write request
    requestPdu[1] = 0x02;          // HAP PDU Type
    requestPdu[2] = transactionId; // TID
    requestPdu[3] = threadManagementControlCharacteristic.iid & 0xff;
    requestPdu[4] = (threadManagementControlCharacteristic.iid >> 8) & 0xff;
    requestPdu[5] = (numRequestBytes + 5) & 0xff; // body length
    requestPdu[6] = ((numRequestBytes + 5) >> 8) & 0xff;
    requestPdu[7] = kHAPPDUTLVType_ReturnResponse; // HAP-Param-Return-Response
    requestPdu[8] = 0x01;                          // length
    requestPdu[9] = 1;                             // value = set
    requestPdu[10] = kHAPPDUTLVType_Value;         // HAP-Param-Value
    requestPdu[11] = numRequestBytes & 0xff;
    HAPAssert(numRequestBytes < 256);
    HAPRawBufferCopyBytes(&requestPdu[12], requestBytes, numRequestBytes);

    uint8_t responseBytes[1024];
    size_t numResponseBytes;
    size_t bytesConsumed;
    bool requestSuccessful; // TODO: Can this be updated to assert this value?
    err = HAPPDUProcedureHandleRequest(
            server,
            session,
            requestPdu,
            sizeof requestPdu,
            responseBytes,
            sizeof responseBytes,
            &numResponseBytes,
            &bytesConsumed,
            &requestSuccessful);
    TEST_ASSERT(!err);

    TEST_ASSERT(numResponseBytes >= 3);
    TEST_ASSERT_EQUAL(responseBytes[0], 0x02); // Write response
    TEST_ASSERT_EQUAL(responseBytes[1], transactionId);
    TEST_ASSERT_EQUAL(responseBytes[2], 0); // status

    if (numResponseBytes > 3) {
        TEST_ASSERT(numResponseBytes >= 5);
        size_t bodyLength = (size_t) responseBytes[3] + ((size_t) responseBytes[4] << 8);
        TEST_ASSERT_EQUAL(numResponseBytes - 5, bodyLength);

        HAPTLVReader bodyReader;
        HAPTLVReaderCreate(&bodyReader, &responseBytes[5], bodyLength);

        for (;;) {
            HAPTLV tlv;
            bool valid;
            err = HAPTLVReaderGetNext(&bodyReader, &valid, &tlv);
            TEST_ASSERT(!err);
            if (!valid) {
                break;
            }
            switch (tlv.type) {
                case kHAPPDUTLVType_Value: {
                    // The value response should be empty
                    TEST_ASSERT_EQUAL(tlv.value.numBytes, 0u);
                    break;
                }
                default: {
                    TEST_FAIL("Unexpected TLV 0x%02x included in the response @ %s:%d", tlv.type, HAP_FILE, __LINE__);
                }
            }
        }
    }
}

/**
 * Emulate a Read Thread Parameters command to accessory
 *
 * @param session              HAP session
 * @param server               accessory server
 * @param transactionId        transaction ID of the message to send
 * @param handleResponseStatus block to handle response status
 * @param handleResponsevalue  block to handle response value
 */
static void TriggerReadThreadParametersOperation(
        HAPSession* session,
        HAPAccessoryServer* server,
        uint8_t transactionId,
        std::function<void(uint8_t)> handleResponseStatus,
        std::function<void(const void*, size_t)> handleResponseValue) {
    uint8_t requestBuffer[1024];
    HAPTLVWriter requestWriter;
    void* requestBytes;
    size_t numRequestBytes;

    HAPTLVWriterCreate(&requestWriter, requestBuffer, sizeof requestBuffer);

    static const uint8_t tlvBytes[] = { 0x03 };
    static const HAPTLV tlv = { .value = {
                                        .bytes = tlvBytes,
                                        .numBytes = 1,
                                },
                                .type = 1, // operation type
                              };
    HAPError err = HAPTLVWriterAppend(&requestWriter, &tlv);
    HAPAssert(!err);

    HAPTLVWriterGetBuffer(&requestWriter, &requestBytes, &numRequestBytes);

    uint8_t requestPdu[numRequestBytes + 12];
    requestPdu[0] = 0x00;          // Write request
    requestPdu[1] = 0x02;          // HAP PDU Type
    requestPdu[2] = transactionId; // TID
    requestPdu[3] = threadManagementControlCharacteristic.iid & 0xff;
    requestPdu[4] = (threadManagementControlCharacteristic.iid >> 8) & 0xff;
    requestPdu[5] = (numRequestBytes + 5) & 0xff; // body length
    requestPdu[6] = ((numRequestBytes + 5) >> 8) & 0xff;
    requestPdu[7] = kHAPPDUTLVType_ReturnResponse; // HAP-Param-Return-Response
    requestPdu[8] = 0x01;                          // length
    requestPdu[9] = 1;                             // value = set
    requestPdu[10] = kHAPPDUTLVType_Value;         // HAP-Param-Value
    requestPdu[11] = numRequestBytes & 0xff;
    HAPAssert(numRequestBytes < 256);
    HAPRawBufferCopyBytes(&requestPdu[12], requestBytes, numRequestBytes);

    uint8_t responseBytes[1024];
    size_t numResponseBytes;
    size_t bytesConsumed;
    bool requestSuccessful; // TODO: Can this be updated to assert this value?
    err = HAPPDUProcedureHandleRequest(
            server,
            session,
            requestPdu,
            sizeof requestPdu,
            responseBytes,
            sizeof responseBytes,
            &numResponseBytes,
            &bytesConsumed,
            &requestSuccessful);
    TEST_ASSERT(!err);

    TEST_ASSERT(numResponseBytes >= 3);
    TEST_ASSERT_EQUAL(responseBytes[0], 0x02); // Write response
    TEST_ASSERT_EQUAL(responseBytes[1], transactionId);
    handleResponseStatus(responseBytes[2]);

    if (numResponseBytes > 3) {
        TEST_ASSERT(numResponseBytes >= 5);
        size_t bodyLength = (size_t) responseBytes[3] + ((size_t) responseBytes[4] << 8);
        TEST_ASSERT_EQUAL(numResponseBytes - 5, bodyLength);

        HAPTLVReader bodyReader;
        HAPTLVReaderCreate(&bodyReader, &responseBytes[5], bodyLength);

        for (;;) {
            HAPTLV tlv;
            bool valid;
            err = HAPTLVReaderGetNext(&bodyReader, &valid, &tlv);
            TEST_ASSERT(!err);
            if (!valid) {
                break;
            }
            switch (tlv.type) {
                case kHAPPDUTLVType_Value: {
                    // The value response must contain data
                    handleResponseValue(tlv.value.bytes, tlv.value.numBytes);
                    break;
                }
                default: {
                    HAPLogError(&logObject, "Unexpected TLV 0x%02x included in the response", tlv.type);
                    HAPFatalError();
                    break;
                }
            }
        }
    }
}

/**
 * Emulate reading Thread Role characteristic value
 *
 * @param session              HAP session
 * @param server               accessory server
 * @param transactionId        transaction ID of the message to send
 * @param handleResponseStatus block to handle response status
 * @param handleResponseValue  block to handle response value
 */
static void TriggerReadThreadRoleCharacteristic(
        HAPSession* session,
        HAPAccessoryServer* server,
        uint8_t transactionId,
        std::function<void(uint8_t)> handleResponseStatus,
        std::function<void(uint16_t)> handleResponseValue) {

    uint8_t requestPdu[5];
    requestPdu[0] = 0x00;          // Read request
    requestPdu[1] = 0x03;          // HAP PDU Type
    requestPdu[2] = transactionId; // TID
    requestPdu[3] = threadManagementStatusCharacteristic.iid & 0xff;
    requestPdu[4] = (threadManagementStatusCharacteristic.iid >> 8) & 0xff;

    uint8_t responseBytes[1024];
    size_t numResponseBytes;
    size_t bytesConsumed;
    bool requestSuccessful; // TODO: Can this be updated to assert this value?
    HAPError err = HAPPDUProcedureHandleRequest(
            server,
            session,
            requestPdu,
            sizeof requestPdu,
            responseBytes,
            sizeof responseBytes,
            &numResponseBytes,
            &bytesConsumed,
            &requestSuccessful);
    TEST_ASSERT(!err);

    TEST_ASSERT(numResponseBytes >= 3);
    TEST_ASSERT_EQUAL(responseBytes[0], 0x02); // Read response
    TEST_ASSERT_EQUAL(responseBytes[1], transactionId);
    handleResponseStatus(responseBytes[2]);

    if (numResponseBytes > 3) {
        TEST_ASSERT(numResponseBytes >= 5);
        size_t bodyLength = (size_t) responseBytes[3] + ((size_t) responseBytes[4] << 8);
        TEST_ASSERT_EQUAL(numResponseBytes - 5, bodyLength);

        HAPTLVReader bodyReader;
        HAPTLVReaderCreate(&bodyReader, &responseBytes[5], bodyLength);

        for (;;) {
            HAPTLV tlv;
            bool valid;
            err = HAPTLVReaderGetNext(&bodyReader, &valid, &tlv);
            TEST_ASSERT(!err);
            if (!valid) {
                break;
            }
            switch (tlv.type) {
                case kHAPPDUTLVType_Value: {
                    // The value response must contain data uint16 value
                    TEST_ASSERT_EQUAL(tlv.value.numBytes, sizeof(uint16_t));
                    uint16_t value = HAPReadLittleUInt16(tlv.value.bytes);
                    handleResponseValue(value);
                    break;
                }
                default: {
                    HAPLogError(&logObject, "Unexpected TLV 0x%02x included in the response", tlv.type);
                    HAPFatalError();
                    break;
                }
            }
        }
    }
}

/**
 * Test BLE Thread capable accessory NOT starting Thread joiner upon server starting
 */
TEST(TestBleThreadDeviceJoinerUponServerStart) {
    // Commissioner must not be called
    EXPECT(threadMock, HAPPlatformThreadJoinCommissionedNetwork).AtMost(0);
    EXPECT(threadMock, HAPPlatformThreadStartJoiner).AtMost(0);

    HAPAccessoryServer* server = CreateBleThreadAccessoryServer(THREAD_REATTEMPT_PERIOD);

    // Remove all pairing
    HAPError err = HAPPairingRemoveAll(server);
    TEST_ASSERT(!err);

    // Start accessory server.
    HAPAccessoryServerStart(server, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(server), kHAPAccessoryServerState_Running);

    VERIFY_ALL(bleMock);
    VERIFY_ALL(threadMock);

    // Joiner should not run
    EXPECT(threadMock, HAPPlatformThreadJoinCommissionedNetwork).AtMost(0);
    EXPECT(threadMock, HAPPlatformThreadStartJoiner).AtMost(0);

    HAPPlatformClockAdvance(THREAD_JOIN_DELAY);

    // BLE should be on after a while
    TEST_ASSERT(bleIsOn);
    TEST_ASSERT(!threadIsOn);

    VERIFY_ALL(bleMock);
    VERIFY_ALL(threadMock);

    EXPECT(threadMock, HAPPlatformThreadJoinCommissionedNetwork).AtMost(0);
    EXPECT(threadMock, HAPPlatformThreadStartJoiner).AtMost(0);

    HAPAccessoryServerForceStop(server);

    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(server), kHAPAccessoryServerState_Idle);

    // Both transports should be off when server stops
    TEST_ASSERT(!bleIsOn);
    TEST_ASSERT(!threadIsOn);

    VERIFY_ALL(bleMock);
    VERIFY_ALL(threadMock);

    EXPECT(threadMock, HAPPlatformThreadJoinCommissionedNetwork).AtMost(0);
    EXPECT(threadMock, HAPPlatformThreadStartJoiner).AtMost(0);

    // Add a pairing
    AddPairingEntry(0);

    // Start the accessory server
    HAPAccessoryServerStart(server, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(server), kHAPAccessoryServerState_Running);

    // Wait 5 seconds which corresponds to joiner intervals
    HAPPlatformClockAdvance(THREAD_JOIN_DELAY);

    // If server starts again with a pairing, BLE must be on.
    TEST_ASSERT(bleIsOn);
    TEST_ASSERT(!threadIsOn);

    VERIFY_ALL(bleMock);
    VERIFY_ALL(threadMock);
}

/**
 * Test BLE Thread capable accessory handling of Thread Management service Initiate Thread Joiner command
 */
TEST(TestBleThreadDeviceJoinerUponHapCommand) {
    // Add a pairing
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    AddPairingEntry(controllerPairingID);

    HAPAccessoryServer* server = CreateBleThreadAccessoryServer(THREAD_REATTEMPT_PERIOD);

    // Create a fake security session
    HAPSession sessions[] = {
        CreateFakeThreadHAPSession(server, controllerPairingID),
    };

    // Init session keys
    SetupFakeSessionKeys(sessions, HAPArrayCount(sessions));

    // We don't expect joining with commissioned parameters
    EXPECT(threadMock, HAPPlatformThreadJoinCommissionedNetwork).AtMost(0);

    // Start accessory server.
    HAPAccessoryServerStart(server, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(server), kHAPAccessoryServerState_Running);
    TEST_ASSERT(bleIsOn);

    // Wait 5 seconds which corresponds to joiner intervals
    HAPPlatformClockAdvance(THREAD_JOIN_DELAY);

    // Start joiner must not have been called
    TEST_ASSERT(!startJoinerCallback);

    // Thread must have been disabled
    TEST_ASSERT(!threadIsOn);

    // BLE must have been initialized
    TEST_ASSERT(bleIsOn);

    VERIFY_ALL(threadMock);

    // We don't expect joining if joiner fails in the next step
    EXPECT(threadMock, HAPPlatformThreadJoinCommissionedNetwork).AtMost(0);

    uint8_t tid = 0;

    // Emulate Initiate Thread Joiner operation
    TriggerInitiateThreadJoinerOperation(&sessions[0], server, tid);

    // Joiner must run for the specified duration
    for (size_t i = 0; i < (size_t)(HAPMillisecond * THREAD_JOIN_ATTACH_TIMEOUT / THREAD_JOIN_DELAY); i++) {
        // Wait 5 seconds which corresponds to joiner intervals

        HAPPlatformClockAdvance(THREAD_JOIN_DELAY - THREAD_JOIN_DELAY_FRACTION);
        TEST_ASSERT(i == 0 || !startJoinerCallback); // First one could have been started right off
        HAPPlatformClockAdvance(THREAD_JOIN_DELAY_FRACTION);

        // Joiner should have been called by now.
        // Timeout boundary doesn't have to be accurate. That is, it depends on where the timer exactly started.
        TEST_ASSERT(i >= (HAPMillisecond * THREAD_JOIN_ATTACH_TIMEOUT / THREAD_JOIN_DELAY - 1) || startJoinerCallback);

        // Make a callback to tell HAP that accessory hasn't joined yet, to move forward again
        if (startJoinerCallback) {
            SetReachabilityAndMakeCallback(&startJoinerCallback, startJoinerServer, false);
        }
    }

    HAPPlatformClockAdvance(THREAD_JOIN_DELAY);

    // Thread must have been disabled
    TEST_ASSERT(!threadIsOn);

    VERIFY_ALL(threadMock);

    // We expect joining when joiner succeeds in the next step
    EXPECT(threadMock, HAPPlatformThreadJoinCommissionedNetwork).AtLeast(1);

    // Store border router callback when it's registered later
    borderRouterCallback = NULL;
    borderRouterCallbackContext = NULL;

    // Emulate Another Initiate Thread Joiner operation
    TriggerInitiateThreadJoinerOperation(&sessions[0], server, ++tid);

    HAPPlatformClockAdvance(THREAD_JOIN_DELAY);

    // Thread must have been started
    TEST_ASSERT(threadIsOn);
    TEST_ASSERT(borderRouterCallback);

    // BLE must still be running
    TEST_ASSERT(bleIsOn);

    // Joiner must have been started
    TEST_ASSERT(startJoinerCallback);

    // Emulate successful joiner result
    SetReachabilityAndMakeCallback(&startJoinerCallback, startJoinerServer, true);

    HAPPlatformClockAdvance(0);

    // Thread must have been enabled
    TEST_ASSERT(threadIsOn);

    VERIFY_ALL(threadMock);

    HAPPlatformClockAdvance(HAPSecond);

    // BLE must be still on
    TEST_ASSERT(bleIsOn);
    VERIFY_ALL(threadMock);

    // Make border router detection callback
    borderRouterCallback(true, borderRouterCallbackContext);
    HAPPlatformClockAdvance(0);

    // BLE must be off
    TEST_ASSERT(!bleIsOn);
    TEST_ASSERT(threadIsOn);
    VERIFY_ALL(threadMock);

    // Test reading device status
    HAPPlatformThreadDeviceRole deviceRole = kHAPPlatformThreadDeviceRole_Child;
    EXPECT(threadMock, HAPPlatformThreadGetRole).Do([&](HAPPlatformThreadDeviceRole* role) {
        *role = deviceRole;
        return kHAPError_None;
    });
    TriggerReadThreadRoleCharacteristic(
            &sessions[0],
            server,
            ++tid,
            [](uint8_t status) { TEST_ASSERT_EQUAL(status, 0); },
            [](uint16_t value) {
                // Child
                TEST_ASSERT_EQUAL(value, 0x8);
            });

    deviceRole = kHAPPlatformThreadDeviceRole_Router;
    TriggerReadThreadRoleCharacteristic(
            &sessions[0],
            server,
            ++tid,
            [](uint8_t status) { TEST_ASSERT_EQUAL(status, 0); },
            [](uint16_t value) {
                // Router
                TEST_ASSERT_EQUAL(value, 0x10);
            });

    deviceRole = kHAPPlatformThreadDeviceRole_Leader;
    TriggerReadThreadRoleCharacteristic(
            &sessions[0],
            server,
            ++tid,
            [](uint8_t status) { TEST_ASSERT_EQUAL(status, 0); },
            [](uint16_t value) {
                // Leader
                TEST_ASSERT_EQUAL(value, 0x20);
            });

    deviceRole = kHAPPlatformThreadDeviceRole_BR;
    TriggerReadThreadRoleCharacteristic(
            &sessions[0],
            server,
            ++tid,
            [](uint8_t status) { TEST_ASSERT_EQUAL(status, 0); },
            [](uint16_t value) {
                // Border Router
                TEST_ASSERT_EQUAL(value, 0x40);
            });

    deviceRole = kHAPPlatformThreadDeviceRole_Disabled;
    TriggerReadThreadRoleCharacteristic(
            &sessions[0],
            server,
            ++tid,
            [](uint8_t status) { TEST_ASSERT_EQUAL(status, 0); },
            [](uint16_t value) {
                // Disabled
                TEST_ASSERT_EQUAL(value, 0x1);
            });

    deviceRole = kHAPPlatformThreadDeviceRole_Detached;
    TriggerReadThreadRoleCharacteristic(
            &sessions[0],
            server,
            ++tid,
            [](uint8_t status) { TEST_ASSERT_EQUAL(status, 0); },
            [](uint16_t value) {
                // Detached
                TEST_ASSERT_EQUAL(value, 0x2);
            });

    deviceRole = kHAPPlatformThreadDeviceRole_Joining;
    TriggerReadThreadRoleCharacteristic(
            &sessions[0],
            server,
            ++tid,
            [](uint8_t status) { TEST_ASSERT_EQUAL(status, 0); },
            [](uint16_t value) {
                // Joining
                TEST_ASSERT_EQUAL(value, 0x4);
            });

    VERIFY_ALL(threadMock);
}

/**
 * Common routine to test BLE Thread capable accessory handling of Thread Set Parameters command
 */
static void TestSetThreadParametersCommandCommonRoutine(int variant) {
    HAPTime reachabilityTestDuration = 0;
    HAPPlatformRunLoopCallback reachabilityTestCallback = NULL;
    HAPAccessoryServer* reachabilityTestServer = NULL;
    ALWAYS(threadMock, HAPPlatformThreadTestNetworkReachability)
            .Do([&](const HAPPlatformThreadNetworkParameters* parameters,
                    HAPTime testDuration,
                    HAPPlatformRunLoopCallback callback,
                    void* server) {
                TEST_ASSERT(parameters);
                TEST_ASSERT_EQUAL(parameters->channel, THREAD_SET_PARAMS_CHANNEL);
                TEST_ASSERT_EQUAL(parameters->panId, THREAD_SET_PARAMS_PANID);
                reachabilityTestDuration = testDuration;
                reachabilityTestCallback = callback;
                reachabilityTestServer = (HAPAccessoryServer*) server;
            });

    // Add a pairing
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    AddPairingEntry(controllerPairingID);

    HAPAccessoryServer* server = CreateBleThreadAccessoryServer(THREAD_REATTEMPT_PERIOD);

    // Create a fake security session
    HAPSession sessions[] = {
        CreateFakeThreadHAPSession(server, controllerPairingID),
        CreateFakeBLEHAPSession(server, controllerPairingID),
    };

    // Init session keys
    SetupFakeSessionKeys(sessions, HAPArrayCount(sessions));

    // No thread commissioning expected
    EXPECT(threadMock, HAPPlatformThreadJoinCommissionedNetwork).AtMost(0);

    // Start accessory server.
    HAPAccessoryServerStart(server, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(server), kHAPAccessoryServerState_Running);
    TEST_ASSERT(bleIsOn);

    // Wait 5 seconds which corresponds to joiner intervals
    HAPPlatformClockAdvance(THREAD_JOIN_DELAY);

    // Start joiner must not have been called
    TEST_ASSERT(!startJoinerCallback);

    // Thread must have been disabled
    TEST_ASSERT(!threadIsOn);

    // BLE must have been initialized
    TEST_ASSERT(bleIsOn);

    VERIFY_ALL(threadMock);

    // No thread commissioning expected when joiner fails
    EXPECT(threadMock, HAPPlatformThreadJoinCommissionedNetwork).AtMost(0);

    // BLE should not be off at all while joiner is running
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);

    uint8_t tid = 0;

    // Emulate Set Thread Parameters operation
    TriggerSetThreadParametersOperation(&sessions[1], server, tid, 0);

    HAPPlatformClockAdvance(0);
    HAPTime timeRemaining = HAPMillisecond * THREAD_SET_PARAMS_ATTACH_TIMEOUT;

    // Reachability test must be performed
    while (timeRemaining > THREAD_JOIN_DELAY) {
        TEST_ASSERT(reachabilityTestCallback);
        HAPPlatformClockAdvance(reachabilityTestDuration);
        timeRemaining -= reachabilityTestDuration;
        // Make a callback to tell HAP that network isn't reachable yet.
        SetReachabilityAndMakeCallback(&reachabilityTestCallback, reachabilityTestServer, false);
    }
    HAPPlatformClockAdvance(timeRemaining); // Finish the timeout
    SetReachabilityAndMakeCallback(&reachabilityTestCallback, reachabilityTestServer, false);
    HAPPlatformClockAdvance(HAPSecond);

    // No more reachability test callback
    TEST_ASSERT(!reachabilityTestCallback);

    // Thread must have been disabled
    TEST_ASSERT(!threadIsOn);

    // BLE must be still on
    TEST_ASSERT(bleIsOn);

    VERIFY_ALL(bleMock);
    VERIFY_ALL(threadMock);

    // Thread joining expected when joiner succeeds
    EXPECT(threadMock, HAPPlatformThreadJoinCommissionedNetwork).AtLeast(1);

    // Wait for the reattach period
    HAPPlatformClockAdvance(HAPMillisecond * THREAD_REATTEMPT_PERIOD);

    // BLE must be still on
    TEST_ASSERT(bleIsOn);
    // Thread must be on
    TEST_ASSERT(threadIsOn);
    TEST_ASSERT(borderRouterCallback);

    // Reachability test must have been started
    TEST_ASSERT(reachabilityTestCallback);

    // Emulate successful reachability test result
    SetReachabilityAndMakeCallback(&reachabilityTestCallback, reachabilityTestServer, true);

    HAPPlatformClockAdvance(0);

    // Thread must have been enabled
    TEST_ASSERT(threadIsOn);
    TEST_ASSERT(bleIsOn);

    VERIFY_ALL(threadMock);
    VERIFY_ALL(bleMock);

    if (variant == 1 || variant == 2) {
        // Test the case where BLE connection is on

        // We don't expect BLE to be disconnected
        EXPECT(bleMock, HAPPlatformBLEPeripheralManagerCancelCentralConnection).AtMost(0);

        // Set connection on
        TEST_ASSERT(blePeripheralManager);
        TEST_ASSERT(blePeripheralManager->delegate.handleConnectedCentral);
        blePeripheralManager->delegate.handleConnectedCentral(blePeripheralManager, 0, server);

        // Pretend that pairing is done
        HAPSession* session = server->ble.storage->session;
        session->hap.isTransient = false;
        session->hap.active = true;
        HAPBLESessionDidStartBLEProcedure(server, session);
    }

    HAPPlatformClockAdvance(HAPSecond);
    // BLE must be still on
    TEST_ASSERT(bleIsOn);

    // Make border router detection callback
    borderRouterCallback(true, borderRouterCallbackContext);
    HAPPlatformClockAdvance(0);

    TEST_ASSERT(threadIsOn);

    if (variant == 0) {
        // BLE must have been stopped
        TEST_ASSERT(!bleIsOn);
    } else if (variant == 1) {
        // BLE must be still on when the BLE connection remains
        TEST_ASSERT(bleIsOn);

        // We don't expect BLE to be disconnected
        VERIFY_ALL(bleMock);
        EXPECT(bleMock, HAPPlatformBLEPeripheralManagerCancelCentralConnection).AtMost(0);

        // BLE should be up upto 30 seconds.
        HAPPlatformClockAdvance(28 * HAPSecond);
        TEST_ASSERT(bleIsOn);
        TEST_ASSERT(threadIsOn);

        // After 30 seconds in total, BLE must be disconnected.
        // We don't expect BLE to be disconnected
        VERIFY_ALL(bleMock);
        EXPECT(bleMock, HAPPlatformBLEPeripheralManagerCancelCentralConnection).AtLeast(1);

        HAPPlatformClockAdvance(1 * HAPSecond);
        TEST_ASSERT(threadIsOn);

        VERIFY_ALL(bleMock);

        // Notify HAP of the BLE disconnection
        HAPPrecondition(blePeripheralManager);
        blePeripheralManager->delegate.handleDisconnectedCentral(blePeripheralManager, 0, server);

        // BLE must be down
        HAPPlatformClockAdvance(0);
        TEST_ASSERT(!bleIsOn);
        TEST_ASSERT(threadIsOn);

    } else if (variant == 2) {
        // Thread network is gone instead of BLE
        HAPPlatformClockAdvance(HAPSecond);
        borderRouterCallback(0, borderRouterCallbackContext);
        HAPPlatformClockAdvance(0);

        // Keep the BLE alive
        HAPSession* session = server->ble.storage->session;
        HAPBLESessionDidStartBLEProcedure(server, session);
        HAPPlatformClockAdvance(28 * HAPSecond);
        HAPBLESessionDidStartBLEProcedure(server, session);
        HAPPlatformClockAdvance(28 * HAPSecond);
        HAPBLESessionDidStartBLEProcedure(server, session);

        // Thread must timeout
        HAPPlatformClockAdvance(9 * HAPSecond);
        TEST_ASSERT(bleIsOn);
        TEST_ASSERT(threadIsOn);

        // Let the BLE connection time out
        VERIFY_ALL(bleMock);
        EXPECT(bleMock, HAPPlatformBLEPeripheralManagerCancelCentralConnection).AtLeast(1);
        HAPPlatformClockAdvance(21 * HAPSecond);
        TEST_ASSERT(threadIsOn);
        VERIFY_ALL(bleMock);

        // Notify HAP of the BLE disconnection
        HAPPrecondition(blePeripheralManager);
        blePeripheralManager->delegate.handleDisconnectedCentral(blePeripheralManager, 0, server);

        // BLE must be still up
        HAPPlatformClockAdvance(0);
        TEST_ASSERT(bleIsOn);
        TEST_ASSERT(threadIsOn);
    }

    // Throughout the test, joiner must never have been called
    TEST_ASSERT(!startJoinerCallback);

    if (variant == 0 || variant == 1) {
        // Set Thread Parameters command should fail when sent over Thread session
        TriggerSetThreadParametersOperation(&sessions[0], server, tid, kHAPPDUStatus_InvalidRequest);
    }

    VERIFY_ALL(bleMock);
    VERIFY_ALL(threadMock);
}

/**
 * First variant to test BLE Thread capable accessory handling of Thread Set Parameters command.
 *
 * In this variant, BLE connection is disconnected before Thread transport starts
 * upon Thread Set Parameters command.
 */
TEST(TestSetThreadParametersCommandVariant_0) {
    TestSetThreadParametersCommandCommonRoutine(0);
}

/**
 * Second variant to test BLE Thread capable accessory handling of Thread Set Parameters command
 *
 * In this variant, BLE connection is maintained when Thread transport starts upon Thread Set Parameters
 * command and then the BLE connection is disconnected upon link timer expiration.
 */
TEST(TestSetThreadParametersCommandVariant_1) {
    TestSetThreadParametersCommandCommonRoutine(1);
}

/**
 * Third variant to test BLE Thread capable accessory handling of Thread Set Parameters command.
 *
 * In this variant, after accessory server joins Thread network per Thread Set Parameters command,
 * Thread network is lost again while the BLE connection lasts beyond the border router detection
 * timer expires and Thread transport stops.
 */
TEST(TestSetThreadParametersCommandVariant_2) {
    TestSetThreadParametersCommandCommonRoutine(2);
}

/**
 * Test bad Set Thread Parameters command
 */
TEST(TestBadSetThreadParametersCommand) {
    HAPTime reachabilityTestDuration = 0;
    HAPPlatformRunLoopCallback reachabilityTestCallback = NULL;
    HAPAccessoryServer* reachabilityTestServer = NULL;
    ALWAYS(threadMock, HAPPlatformThreadTestNetworkReachability)
            .Do([&](const HAPPlatformThreadNetworkParameters* parameters,
                    HAPTime testDuration,
                    HAPPlatformRunLoopCallback callback,
                    void* server) {
                TEST_ASSERT(parameters);
                TEST_ASSERT_EQUAL(parameters->channel, THREAD_SET_PARAMS_CHANNEL);
                TEST_ASSERT_EQUAL(parameters->panId, THREAD_SET_PARAMS_PANID);
                reachabilityTestDuration = testDuration;
                reachabilityTestCallback = callback;
                reachabilityTestServer = (HAPAccessoryServer*) server;
            });

    // Add a pairing
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    AddPairingEntry(controllerPairingID);

    HAPAccessoryServer* server = CreateBleThreadAccessoryServer(THREAD_REATTEMPT_PERIOD);

    // Create a fake security session
    HAPSession sessions[] = {
        CreateFakeThreadHAPSession(server, controllerPairingID),
        CreateFakeBLEHAPSession(server, controllerPairingID),
    };

    // Init session keys
    SetupFakeSessionKeys(sessions, HAPArrayCount(sessions));

    // No thread commissioning expected
    EXPECT(threadMock, HAPPlatformThreadJoinCommissionedNetwork).AtMost(0);

    // Start accessory server.
    HAPAccessoryServerStart(server, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(server), kHAPAccessoryServerState_Running);
    TEST_ASSERT(bleIsOn);

    // Wait 5 seconds which corresponds to joiner intervals
    HAPPlatformClockAdvance(THREAD_JOIN_DELAY);

    // Start joiner must not have been called
    TEST_ASSERT(!startJoinerCallback);

    // Thread must have been disabled
    TEST_ASSERT(!threadIsOn);

    // BLE must have been initialized
    TEST_ASSERT(bleIsOn);

    VERIFY_ALL(threadMock);

    // No thread commissioning expected
    EXPECT(threadMock, HAPPlatformThreadJoinCommissionedNetwork).AtMost(0);

    // BLE should not be turned off at all
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);

    uint8_t tid = 0;

    // Emulate an empty Set Thread Parameters operation
    TriggerEmptySetThreadParametersOperation(&sessions[1], server, tid, kHAPPDUStatus_InvalidRequest);

    HAPPlatformClockAdvance(60);

    // Thread must have been disabled
    TEST_ASSERT(!threadIsOn);

    // BLE must be still on
    TEST_ASSERT(bleIsOn);

    VERIFY_ALL(bleMock);
    VERIFY_ALL(threadMock);

    // No thread commissioning expected
    EXPECT(threadMock, HAPPlatformThreadJoinCommissionedNetwork).AtMost(0);

    // BLE should not be turned off at all
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);

    tid++;

    // Emulate an empty Thread control message operation
    TriggerEmptyThreadControlOperation(&sessions[1], server, tid, kHAPPDUStatus_InvalidRequest);

    HAPPlatformClockAdvance(60);

    // Thread must have been disabled
    TEST_ASSERT(!threadIsOn);

    // BLE must be still on
    TEST_ASSERT(bleIsOn);

    VERIFY_ALL(bleMock);
    VERIFY_ALL(threadMock);
}

/**
 * Test BLE Thread capable accessory handling race condition of the
 * border router detection callback while running reachability test
 */
TEST(TestBorderRouterCallbackRace) {
    HAPTime reachabilityTestDuration = 0;
    HAPPlatformRunLoopCallback reachabilityTestCallback = NULL;
    HAPAccessoryServer* reachabilityTestServer = NULL;
    ALWAYS(threadMock, HAPPlatformThreadTestNetworkReachability)
            .Do([&](const HAPPlatformThreadNetworkParameters* parameters,
                    HAPTime testDuration,
                    HAPPlatformRunLoopCallback callback,
                    void* server) {
                TEST_ASSERT(parameters);
                TEST_ASSERT_EQUAL(parameters->channel, THREAD_SET_PARAMS_CHANNEL);
                TEST_ASSERT_EQUAL(parameters->panId, THREAD_SET_PARAMS_PANID);
                reachabilityTestDuration = testDuration;
                reachabilityTestCallback = callback;
                reachabilityTestServer = (HAPAccessoryServer*) server;
            });

    // Add a pairing
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    AddPairingEntry(controllerPairingID);

    HAPAccessoryServer* server = CreateBleThreadAccessoryServer(THREAD_REATTEMPT_PERIOD);

    // Create a fake security session
    HAPSession sessions[] = {
        CreateFakeThreadHAPSession(server, controllerPairingID),
        CreateFakeBLEHAPSession(server, controllerPairingID),
    };

    // Init session keys
    SetupFakeSessionKeys(sessions, HAPArrayCount(sessions));

    // BLE shouldn't be de-initialized in the next few startup steps
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);

    // Thread must not join in the next few startup steps
    EXPECT(threadMock, HAPPlatformThreadJoinCommissionedNetwork).AtMost(0);

    // Start accessory server.
    HAPAccessoryServerStart(server, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(server), kHAPAccessoryServerState_Running);
    TEST_ASSERT(bleIsOn);

    // Wait 5 seconds which corresponds to joiner intervals
    HAPPlatformClockAdvance(THREAD_JOIN_DELAY);

    // Start joiner must not have been called
    TEST_ASSERT(!startJoinerCallback);

    // Thread must have been disabled
    TEST_ASSERT(!threadIsOn);

    // BLE must have been initialized
    TEST_ASSERT(bleIsOn);

    VERIFY_ALL(bleMock);
    VERIFY_ALL(threadMock);

    uint8_t tid = 0;

    // Emulate Set Thread Parameters operation
    TriggerSetThreadParametersOperation(&sessions[1], server, tid, 0);

    HAPPlatformClockAdvance(0);
    TEST_ASSERT(borderRouterCallback);

    // BLE must be still on
    TEST_ASSERT(bleIsOn);
    // Thread must be on
    TEST_ASSERT(threadIsOn);
    TEST_ASSERT(borderRouterCallback);

    // Reachability test must have been started
    TEST_ASSERT(reachabilityTestCallback);

    // Emulate border router callback racing
    borderRouterCallback(false, borderRouterCallbackContext);

    HAPPlatformClockAdvance(0);

    // Thread must be still active
    TEST_ASSERT(threadIsOn);
    TEST_ASSERT(bleIsOn);

    // Emulate unsuccessful reachability test result
    HAPPlatformClockAdvance(reachabilityTestDuration);
    SetReachabilityAndMakeCallback(&reachabilityTestCallback, reachabilityTestServer, false);

    HAPPlatformClockAdvance(0);

    // Reachability test must have been continued
    TEST_ASSERT(reachabilityTestCallback);

    // Emulate successful reachability test result
    EXPECT(threadMock, HAPPlatformThreadJoinCommissionedNetwork).AtLeast(1);

    HAPPlatformClockAdvance(reachabilityTestDuration);
    SetReachabilityAndMakeCallback(&reachabilityTestCallback, reachabilityTestServer, true);

    // Thread must have been enabled
    TEST_ASSERT(threadIsOn);
    TEST_ASSERT(bleIsOn);
    VERIFY_ALL(threadMock);

    HAPPlatformClockAdvance(HAPSecond);

    // BLE must be still on
    TEST_ASSERT(bleIsOn);

    // Make border router detection callback
    borderRouterCallback(true, borderRouterCallbackContext);
    HAPPlatformClockAdvance(0);

    // BLE must have been stopped
    TEST_ASSERT(!bleIsOn);
    TEST_ASSERT(threadIsOn);

    // Throughout the test, joiner must never have been called
    TEST_ASSERT(!startJoinerCallback);
}

/**
 * Test BLE Thread capable accessory handling of Thread connectivity loss
 */
static void TestThreadConnectivityLossCommonRoutine(int variant) {
    // Throughout the test, pretend that Thread is commissioned.
    ALWAYS(threadMock, HAPPlatformThreadIsCommissioned).Return(true);

    // Add a pairing
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    AddPairingEntry(controllerPairingID);

    uint32_t reattemptPeriod = THREAD_REATTEMPT_PERIOD;
    HAPAccessoryServer* server;

    if (variant == 0) {
        server = CreateBleThreadAccessoryServer(reattemptPeriod);
    } else if (variant == 1) {
        // Non-sleepy end device with default reattach delays
        server = CreateBleThreadAccessoryServer(kHAPPlatformThreadDeviceCapabilities_MED, NULL, NULL);
    } else if (variant == 2) {
        // Sleepy end device with default reattach delays
        server = CreateBleThreadAccessoryServer(kHAPPlatformThreadDeviceCapabilities_SED, NULL, NULL);
    } else {
        TEST_FAIL("Unexpected variant");
    }

    // Since Thread is commissioned already, accessory should join the network
    // when server starts while BLE shouldn't be turned on ever.
    EXPECT(threadMock, HAPPlatformThreadJoinCommissionedNetwork).AtLeast(1);
    EXPECT(bleMock, HAPPlatformBLEInitialize).AtMost(0);

    // Start accessory server.
    HAPAccessoryServerStart(server, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(server), kHAPAccessoryServerState_Running);

    // Thread must have been started and enabled
    TEST_ASSERT(threadIsOn);

    // BLE must not have have been initialized
    TEST_ASSERT(!bleIsOn);

    // BLE should not be initialized in the next step either.
    VERIFY_ALL(bleMock);
    VERIFY_ALL(threadMock);
    EXPECT(bleMock, HAPPlatformBLEInitialize).AtMost(0);

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

    // Wakelock will be added and not removed when border router is lost in the next step
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

    // Border lost
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

    // Border router found again. Wakelock is expected to be removed.
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
            .AtLeast(1);

    borderRouterCallback(true, borderRouterCallbackContext);
    HAPPlatformClockAdvance(HAPSecond);
    TEST_ASSERT(threadIsOn);

    // Advance detection timeout again.
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

    HAPPlatformClockAdvance(THREAD_DETECTION_TIMEOUT);
    TEST_ASSERT(threadIsOn);

    // Border lost again
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

    // Detection timeout occurs
    VERIFY_ALL(bleMock);
    VERIFY_ALL(threadMock);
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

    HAPPlatformClockAdvance(HAPSecond);

    // BLE must be up
    TEST_ASSERT(bleIsOn);

    if (variant == 0 || variant == 1) {
        // Thread must still be up
        TEST_ASSERT(threadIsOn);
    } else {
        // Thread must be down
        TEST_ASSERT(!threadIsOn);
    }

    VERIFY_ALL(bleMock);
    VERIFY_ALL(threadMock);

    // Thread must be reattempted. Try a few times
    if (variant == 2) {
        const HAPTime reattachPeriods[] = {
            HAPMinute + HAPSecond * 10,       HAPMinute * 2 + HAPSecond * 10,
            HAPMinute * 4 + HAPSecond * 10,   HAPMinute * 8 + HAPSecond * 10,
            HAPMinute * 16 + HAPSecond * 10,  HAPMinute * 32 + HAPSecond * 10,
            HAPMinute * 64 + HAPSecond * 10,  HAPMinute * 128 + HAPSecond * 10,
            HAPMinute * 256 + HAPSecond * 10, HAPMinute * 360,
        };
        size_t minIndex = 0;
        size_t maxIndex = 6;
        if (variant > 1) {
            minIndex = 2;
            maxIndex = 9;
        }
        for (size_t i = 0; i < 8; i++) {
            // Reattach period should not have been reached yet
            HAPPlatformClockAdvance(reattachPeriods[HAPMin(minIndex + i, maxIndex)] - HAPSecond * 11);
            TEST_ASSERT(!threadIsOn);
            TEST_ASSERT(bleIsOn);
            // Reattach period should have been reached
            HAPPlatformClockAdvance(HAPSecond * 11);
            TEST_ASSERT(threadIsOn);
            TEST_ASSERT(bleIsOn);
            // Wait till join fails
            HAPPlatformClockAdvance(2 * HAPMinute);
            TEST_ASSERT(!threadIsOn);
            TEST_ASSERT(bleIsOn);
        }
        HAPPlatformClockAdvance(reattachPeriods[HAPMin(minIndex + 8, maxIndex)]);
    }

    // Let the Thread join
    TEST_ASSERT(threadIsOn);
    TEST_ASSERT(bleIsOn);

    // Let the Thread transport get out of the reachability test state.
    HAPPlatformClockAdvance(0);
    TEST_ASSERT(threadIsOn);
    TEST_ASSERT(bleIsOn);

    borderRouterCallback(true, borderRouterCallbackContext);
    HAPPlatformClockAdvance(0);

    // BLE must be down and Thread should be up
    TEST_ASSERT(threadIsOn);

    if (variant == 2) {
        // Test that the reattach period is reset after successful join
        HAPPlatformClockAdvance(HAPMinute);
        borderRouterCallback(false, borderRouterCallbackContext);
        HAPPlatformClockAdvance(THREAD_DETECTION_TIMEOUT);
        TEST_ASSERT(!threadIsOn);

        const HAPTime reattachPeriods[] = {
            HAPMinute + HAPSecond * 10,       HAPMinute * 2 + HAPSecond * 10,
            HAPMinute * 4 + HAPSecond * 10,   HAPMinute * 8 + HAPSecond * 10,
            HAPMinute * 16 + HAPSecond * 10,  HAPMinute * 32 + HAPSecond * 10,
            HAPMinute * 64 + HAPSecond * 10,  HAPMinute * 128 + HAPSecond * 10,
            HAPMinute * 256 + HAPSecond * 10, HAPMinute * 360,
        };
        size_t minIndex = 0;
        size_t maxIndex = 6;
        if (variant > 1) {
            minIndex = 2;
            maxIndex = 9;
        }
        for (size_t i = 0; i < 8; i++) {
            // Reattach period should not have been reached yet
            HAPPlatformClockAdvance(reattachPeriods[HAPMin(minIndex + i, maxIndex)] - HAPSecond * 11);
            TEST_ASSERT(!threadIsOn);
            TEST_ASSERT(bleIsOn);
            // Reattach period should have been reached
            HAPPlatformClockAdvance(HAPSecond * 11);
            TEST_ASSERT(threadIsOn);
            TEST_ASSERT(bleIsOn);
            // Wait till join fails
            HAPPlatformClockAdvance(2 * HAPMinute);
            TEST_ASSERT(!threadIsOn);
            TEST_ASSERT(bleIsOn);
        }
    }
    VERIFY_ALL(bleMock);
    VERIFY_ALL(threadMock);
}

/**
 * Test BLE Thread capable accessory handling of Thread connectivity loss with a constant reattach period
 */
TEST(TestThreadConnectivityLossVariant_0) {
    TestThreadConnectivityLossCommonRoutine(0);
}

/**
 * Test BLE Thread capable accessory handling of Thread connectivity loss with default reattach period for non-sleepy
 * device
 */
TEST(TestThreadConnectivityLossVariant_1) {
    TestThreadConnectivityLossCommonRoutine(1);
}

/**
 * Test BLE Thread capable accessory handling of Thread connectivity loss with default reattach period for sleepy device
 */
TEST(TestThreadConnectivityLossVariant_2) {
    TestThreadConnectivityLossCommonRoutine(2);
}

/**
 * Test BLE Thread capable accessory handling of Thread connectivity loss right off the start of Thread
 */
TEST(TestThreadConnectivityLossOffStart) {
    // This test assumes Thread is commissioned.
    ALWAYS(threadMock, HAPPlatformThreadIsCommissioned).Return(true);

    // Add a pairing
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    AddPairingEntry(controllerPairingID);

    uint32_t reattemptPeriod = THREAD_REATTEMPT_PERIOD;
    HAPAccessoryServer* server = CreateBleThreadAccessoryServer(reattemptPeriod);

    // Start accessory server.
    EXPECT(threadMock, HAPPlatformThreadJoinCommissionedNetwork).AtLeast(1);
    EXPECT(bleMock, HAPPlatformBLEInitialize).AtMost(0);

    HAPAccessoryServerStart(server, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(server), kHAPAccessoryServerState_Running);

    // Thread must have been started and enabled
    TEST_ASSERT(threadIsOn);

    // BLE must not have have been initialized
    TEST_ASSERT(!bleIsOn);

    VERIFY_ALL(bleMock);
    VERIFY_ALL(threadMock);

    // No border router detection for one minute
    HAPPlatformClockAdvance(2 * HAPMinute);

    // BLE must be up
    TEST_ASSERT(bleIsOn);

    TEST_ASSERT(threadIsOn);
    HAPPlatformClockAdvance(10 * HAPMinute);
    TEST_ASSERT(threadIsOn);
}

/**
 * Test BLE Thread capable accessory handling of Thread decommissioning
 */
TEST(TestClearThreadParameters) {
    // This test assumes Thread is commissioned.
    ALWAYS(threadMock, HAPPlatformThreadIsCommissioned).Return(true);

    // Add a pairing
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    AddPairingEntry(controllerPairingID);

    HAPAccessoryServer* server = CreateBleThreadAccessoryServer(THREAD_REATTEMPT_PERIOD);

    // Create a fake security session
    HAPSession sessions[] = {
        CreateFakeThreadHAPSession(server, controllerPairingID),
    };

    // Init session keys
    SetupFakeSessionKeys(sessions, HAPArrayCount(sessions));

    // Start accessory server.
    EXPECT(threadMock, HAPPlatformThreadJoinCommissionedNetwork).AtLeast(1);
    EXPECT(bleMock, HAPPlatformBLEInitialize).AtMost(0);

    HAPAccessoryServerStart(server, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(server), kHAPAccessoryServerState_Running);

    // Thread must have been started and enabled
    TEST_ASSERT(threadIsOn);
    TEST_ASSERT(borderRouterCallback);

    VERIFY_ALL(bleMock);
    VERIFY_ALL(threadMock);

    // Border router detection in 10 seconds
    HAPPlatformClockAdvance(HAPSecond * 10);
    HAPPlatformThreadBorderRouterStateCallback callback = borderRouterCallback;
    borderRouterCallback = NULL;
    callback(true, borderRouterCallbackContext);
    HAPPlatformClockAdvance(0);

    // Thread must still be up
    TEST_ASSERT(threadIsOn);

    // BLE must not be up
    TEST_ASSERT(!bleIsOn);

    // Same in 1 minute
    HAPPlatformClockAdvance(HAPMinute);
    TEST_ASSERT(threadIsOn);
    TEST_ASSERT(!bleIsOn);

    // Send Clear Thread Parameters command
    EXPECT(threadMock, HAPPlatformThreadClearParameters).AtLeast(1);

    uint8_t tid = 0;
    TriggerClearThreadParametersOperation(&sessions[0], server, tid);

    // Wait 5 seconds to wait for transport to time out sending response.
    HAPPlatformClockAdvance(HAPSecond * 5);

    // BLE must be up and Thread must be down
    TEST_ASSERT(bleIsOn);
    TEST_ASSERT(!threadIsOn);

    VERIFY_ALL(bleMock);
    VERIFY_ALL(threadMock);
}

/**
 * Test Read Thread Parameters command
 */
TEST(TestReadThreadParameters) {
    // Assume that Thread is commissioned
    ALWAYS(threadMock, HAPPlatformThreadIsCommissioned).Return(true);

    // Add a pairing
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    AddPairingEntry(controllerPairingID);

    uint32_t threadReattemptPeriod = THREAD_REATTEMPT_PERIOD;
    HAPAccessoryServer* server = CreateBleThreadAccessoryServer(threadReattemptPeriod);

    // Create a fake security session
    HAPSession sessions[] = {
        CreateFakeThreadHAPSession(server, controllerPairingID),
    };

    // Init session keys
    SetupFakeSessionKeys(sessions, HAPArrayCount(sessions));

    // Start accessory server.
    EXPECT(threadMock, HAPPlatformThreadJoinCommissionedNetwork).AtLeast(1);

    HAPAccessoryServerStart(server, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(server), kHAPAccessoryServerState_Running);

    // Thread must be up
    TEST_ASSERT(threadIsOn);

    // BLE must be down
    TEST_ASSERT(!bleIsOn);

    VERIFY_ALL(bleMock);
    VERIFY_ALL(threadMock);

    // Border router detection in 10 seconds
    HAPPlatformClockAdvance(HAPSecond * 10);
    borderRouterCallback(true, borderRouterCallbackContext);
    HAPPlatformClockAdvance(0);

    // BLE must be still down
    TEST_ASSERT(!bleIsOn);

    static const char* networkName = "Some Network";
    static const uint8_t extPanId[] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 };

    // Set up response for read parameters
    EXPECT(threadMock, HAPPlatformThreadGetNetworkParameters)
            .Do([&](HAPPlatformThreadNetworkParameters* params) {
                HAPPrecondition(params);
                params->channel = THREAD_SET_PARAMS_CHANNEL;
                params->panId = THREAD_SET_PARAMS_PANID;
                HAPRawBufferCopyBytes(params->extPanId, extPanId, sizeof extPanId);
                HAPRawBufferCopyBytes(params->networkName, networkName, HAPStringGetNumBytes(networkName) + 1);
                return kHAPError_None;
            })
            .AtLeast(1);

    // Read parameters
    uint8_t tid = 0;
    TriggerReadThreadParametersOperation(
            &sessions[0],
            server,
            tid,
            [](uint8_t status) { TEST_ASSERT_EQUAL(status, 0); },
            [&](const void* bytes, size_t numBytes) {
                TEST_ASSERT(bytes);
                HAPTLVReader reader;
                HAPTLVReaderCreate(&reader, (void*) bytes, numBytes);
                bool parametersArePresent = false;
                for (;;) {
                    HAPTLV tlv;
                    bool valid;
                    HAPError err = HAPTLVReaderGetNext(&reader, &valid, &tlv);
                    TEST_ASSERT(!err);
                    if (!valid) {
                        break;
                    }
                    switch (tlv.type) {
                        case 2: { // Thread Network Credentials
                            parametersArePresent = true;
                            HAPTLVReader paramsReader;
                            HAPTLVReaderCreate(&paramsReader, (void*) tlv.value.bytes, tlv.value.numBytes);
                            bool receivedTlvTypes[7];
                            HAPRawBufferZero(receivedTlvTypes, sizeof receivedTlvTypes);
                            for (;;) {
                                err = HAPTLVReaderGetNext(&paramsReader, &valid, &tlv);
                                if (!valid) {
                                    break;
                                }
                                if (tlv.type <= sizeof receivedTlvTypes) {
                                    receivedTlvTypes[tlv.type] = true;
                                }
                                switch (tlv.type) {
                                    case 1: { // network name
                                        TEST_ASSERT_EQUAL(tlv.value.numBytes, HAPStringGetNumBytes(networkName));
                                        TEST_ASSERT(
                                                HAPRawBufferAreEqual(tlv.value.bytes, networkName, tlv.value.numBytes));
                                        break;
                                    }
                                    case 2: { // channel
                                        TEST_ASSERT_EQUAL(tlv.value.numBytes, 2u);
                                        TEST_ASSERT(
                                                ((uint8_t*) tlv.value.bytes)[0] == (THREAD_SET_PARAMS_CHANNEL & 0xff));
                                        TEST_ASSERT(
                                                ((uint8_t*) tlv.value.bytes)[1] ==
                                                ((THREAD_SET_PARAMS_CHANNEL >> 8) & 0xff));
                                        break;
                                    }
                                    case 3: { // pan id
                                        TEST_ASSERT_EQUAL(tlv.value.numBytes, 2u);
                                        TEST_ASSERT_EQUAL(
                                                ((uint8_t*) tlv.value.bytes)[0], (THREAD_SET_PARAMS_PANID & 0xff));
                                        TEST_ASSERT(
                                                ((uint8_t*) tlv.value.bytes)[1] ==
                                                ((THREAD_SET_PARAMS_PANID >> 8) & 0xff));
                                        break;
                                    }
                                    case 4: { // ext pan id
                                        TEST_ASSERT_EQUAL(tlv.value.numBytes, sizeof extPanId);
                                        TEST_ASSERT(
                                                HAPRawBufferAreEqual(tlv.value.bytes, extPanId, tlv.value.numBytes));
                                        break;
                                    }
                                    case 5: { // master key
                                        TEST_FAIL(
                                                "Unexpected master key in the Read Thread Parameters response @ %s:%d",
                                                HAP_FILE,
                                                __LINE__);
                                    }
                                    case 6: { // reattach period
                                        TEST_ASSERT(tlv.value.numBytes = sizeof(uint32_t));
                                        uint8_t threadReattemptPeriodInLittleEndian[4] = {
                                            (uint8_t)(threadReattemptPeriod & 0xff),
                                            (uint8_t)((threadReattemptPeriod >> 8) & 0xff),
                                            (uint8_t)((threadReattemptPeriod >> 16) & 0xff),
                                            (uint8_t)((threadReattemptPeriod >> 24) & 0xff),
                                        };
                                        TEST_ASSERT(HAPRawBufferAreEqual(
                                                tlv.value.bytes,
                                                threadReattemptPeriodInLittleEndian,
                                                tlv.value.numBytes));
                                        break;
                                    }
                                    default: {
                                        TEST_FAIL(
                                                "Unexpected TLV type %u in the Thread Network Credentials TLV @ %s:%d",
                                                tlv.type,
                                                HAP_FILE,
                                                __LINE__);
                                    }
                                }
                            }

                            // Master key must not have been received and hence just set it here before checking
                            // all required TLVs.
                            receivedTlvTypes[5] = true;

                            // Check that all required TLVs were present
                            for (size_t i = 1; i < 5; i++) {
                                TEST_ASSERT(receivedTlvTypes[i]);
                            }
                            break;
                        }
                        default: {
                            TEST_FAIL(
                                    "Unexpected TLV in the ReadThreadParameters response %u @ %s:%d",
                                    tlv.type,
                                    HAP_FILE,
                                    __LINE__);
                        }
                    }
                }
                TEST_ASSERT(parametersArePresent);
            });

    VERIFY_ALL(bleMock);
    VERIFY_ALL(threadMock);
}

/**
 * Common subroutine for testing Thread only accessory starting Thread joiner upon server starting
 */
static void ThreadOnlyDeviceJoinerUponServerStartCommon(int variant) {
    HAPAccessoryServer* server = CreateThreadAccessoryServer();

    // Remove all pairing
    HAPError err = HAPPairingRemoveAll(server);
    TEST_ASSERT(!err);

    // Start accessory server.
    EXPECT(bleMock, HAPPlatformBLEInitialize).AtMost(0);

    HAPAccessoryServerStart(server, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(server), kHAPAccessoryServerState_Running);
    TEST_ASSERT(threadIsOn);

    VERIFY_ALL(bleMock);
    VERIFY_ALL(threadMock);

    // Joiner must run till unpaired state timer expires. Let's test for 9 minutes.
    // BLE must not be turned on and Thread must not be joined throughout.
    EXPECT(bleMock, HAPPlatformBLEInitialize).AtMost(0);
    EXPECT(threadMock, HAPPlatformThreadJoinCommissionedNetwork).AtMost(0);

    for (size_t i = 0; i < (size_t)(HAPMinute * 9 / THREAD_JOIN_DELAY); i++) {
        // Wait 5 seconds which corresponds to joiner intervals

        HAPPlatformClockAdvance(THREAD_JOIN_DELAY - THREAD_JOIN_DELAY_FRACTION);
        TEST_ASSERT(!startJoinerCallback);
        HAPPlatformClockAdvance(THREAD_JOIN_DELAY_FRACTION);

        // Joiner should have been called by now
        TEST_ASSERT(startJoinerCallback);

        // Make a callback to tell HAP that accessory hasn't joined yet, to move forward again
        SetReachabilityAndMakeCallback(&startJoinerCallback, startJoinerServer, false);
    }

    VERIFY_ALL(bleMock);
    VERIFY_ALL(threadMock);

    if (variant == 0) {
        // Stop server by force
        HAPAccessoryServerForceStop(server);
    } else {
        // Wait till server stops on its own due to unpair state timer expiry.
        HAPPlatformTimerEmulateClockAdvances(HAPMinute);
    }

    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(server), kHAPAccessoryServerState_Idle);
    TEST_ASSERT(!threadIsOn);

    VERIFY_ALL(bleMock);
    VERIFY_ALL(threadMock);

    // Add a pairing
    AddPairingEntry(0);

    // Prepare to receive start joiner call
    startJoinerCallback = NULL;

    EXPECT(bleMock, HAPPlatformBLEInitialize).AtMost(0);
    EXPECT(threadMock, HAPPlatformThreadJoinCommissionedNetwork).AtMost(0);

    // Start the accessory server
    HAPAccessoryServerStart(server, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(server), kHAPAccessoryServerState_Running);

    // Joiner must run forever even if there is a pairing. Let's test for an hour worth.
    for (size_t i = 0; i < (size_t)(HAPMinute * 60 / THREAD_JOIN_DELAY); i++) {
        // Wait 5 seconds which corresponds to joiner intervals

        HAPPlatformClockAdvance(THREAD_JOIN_DELAY - THREAD_JOIN_DELAY_FRACTION);
        TEST_ASSERT(!startJoinerCallback);
        HAPPlatformClockAdvance(THREAD_JOIN_DELAY_FRACTION);

        // Joiner should have been called by now
        TEST_ASSERT(startJoinerCallback);

        // Make a callback to tell HAP that accessory hasn't joined yet, to move forward again
        SetReachabilityAndMakeCallback(&startJoinerCallback, startJoinerServer, false);
    }

    VERIFY_ALL(bleMock);
    VERIFY_ALL(threadMock);
}

/**
 * Test Thread only accessory starting Thread joiner upon server starting
 */
TEST(TestThreadOnlyDeviceJoinerUponServerStart) {
    ThreadOnlyDeviceJoinerUponServerStartCommon(0);
}

/**
 * Test Thread only accessory starting Thread joiner upon server starting with slight variation to wait for the unpaired
 * state timer to expire.
 */
TEST(TestThreadOnlyDeviceJoinerUponServerStartTillUnpairStateTimerExpiry) {
    ThreadOnlyDeviceJoinerUponServerStartCommon(0);
}

/**
 * Test Thread only accessory clearing commission upon no pair-setup
 */
TEST(TestThreadOnlyHandlingNoPairSetup) {
    HAPAccessoryServer* server = CreateThreadAccessoryServer();

    // Remove all pairing
    HAPError err = HAPPairingRemoveAll(server);
    TEST_ASSERT(!err);

    // Start testing with commissioned state
    EXPECT(threadMock, HAPPlatformThreadIsCommissioned).Return(true);
    EXPECT(threadMock, HAPPlatformThreadClearParameters).AtMost(0);
    EXPECT(threadMock, HAPPlatformThreadJoinCommissionedNetwork).AtLeast(1);

    // Start accessory server.
    HAPAccessoryServerStart(server, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(server), kHAPAccessoryServerState_Running);
    TEST_ASSERT(threadIsOn);

    // There shouldn't have been any joiner call
    TEST_ASSERT(!startJoinerCallback);

    VERIFY_ALL(threadMock);

    // Wait for pair-setup timeout
    EXPECT(threadMock, HAPPlatformThreadClearParameters).AtMost(0);
    HAPPlatformClockAdvance(PAIR_SETUP_TIMEOUT);

    // Thread must still be enabled
    TEST_ASSERT(threadIsOn);

    VERIFY_ALL(bleMock);
    VERIFY_ALL(threadMock);
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
