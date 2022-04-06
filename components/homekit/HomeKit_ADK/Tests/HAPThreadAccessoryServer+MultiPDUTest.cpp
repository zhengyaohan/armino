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

#define SESSION_KEY_EXPIRY (1000ul * 60 * 60 * 24 * 7)

//---------------------------------------------------------------------------------------------
// Accessory Server Setup.

extern "C" {
static const HAPLogObject logObject = { .subsystem = "com.apple.mfi.HomeKit.Core.Test", .category = "TestController" };

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

static const HAPSessionKey accessoryToControllerKey = { {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
        0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00,
} };
static uint64_t accessoryToControllerNonce = 0;
static const HAPSessionKey controllerToAccessoryKey = { {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
        0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00,
} };
static uint64_t controllerToAccessoryNonce = 0;
static const HAPSessionKey accessoryEventKey = { {
        0x00, 0x11, 0x12, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
        0x00, 0x11, 0x12, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
} };
uint64_t accessoryEventNonce = 0;

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

//---------------------------------------------------------------------------------------------
// PDU Test helpers

// These macros construct PDUs for individual HAP commands.

#define READ_PDU(IID, TID)         0x00, 0x03, TID, (uint8_t)(IID & 0xFF), (uint8_t)((IID >> 8) & 0xFF), 0x00, 0x00
#define READ_PDU_NO_BODY(IID, TID) 0x00, 0x03, TID, (uint8_t)(IID & 0xFF), (uint8_t)((IID >> 8) & 0xFF)

#define BODY_32B_RESULTS(TID, RESULT) \
    { .tid = TID, .bodyLen = 6, .tlvLen = 4, .intResult = RESULT, .status = kHAPPDUStatus_Success }

#define WRITE_32B_PDU(IID, TID, VAL_32B) \
    0x00, 0x02, TID, (uint8_t)(IID & 0xFF), (uint8_t)((IID >> 8) & 0xFF), 0x06, 0x00, 0x01, 0x04, \
            (uint8_t)(VAL_32B & 0xFF), (uint8_t)((VAL_32B >> 8) & 0xFF), (uint8_t)((VAL_32B >> 16) & 0xFF), \
            (uint8_t)((VAL_32B >> 24) & 0xFF)

#define NO_BODY_RESULTS(TID) ConstructNoBodyResults(TID)

#define REGISTER_PDU(IID, TID) 0x00, 0x0B, TID, (uint8_t)(IID & 0xFF), (uint8_t)((IID >> 8) & 0xFF), 0x00, 0x00

// The expected return values for a PDU test
typedef struct {
    uint8_t tid;
    size_t bodyLen;
    size_t tlvLen;
    uint32_t intResult;
    HAPPDUStatus status;
} PduResult_t;

// A test consists of a PDU to send and the results for those PDUs
typedef struct {
    const uint8_t* pdu;
    size_t pduLen;
    const PduResult_t* results;
    size_t numResults;
} PduTest_t;

static PduResult_t ConstructNoBodyResults(uint8_t tid) {
    PduResult_t result = {
        .tid = tid,
        .bodyLen = 0,
        .tlvLen = 0,
    };
    result.status = kHAPPDUStatus_Success;
    return result;
}

//---------------------------------------------------------------------------------------------
// PDU tests.

// Read Brightness no body
static const uint8_t readBrightnessPDUNoBody[] = { READ_PDU_NO_BODY(lightBulbBrightnessCharacteristic.iid, 1) };
static const PduResult_t readBrightnessResultsNoBody[] = { BODY_32B_RESULTS(1, 50) };
static const PduTest_t readBrightnessTestNoBody = { .pdu = readBrightnessPDUNoBody,
                                                    .pduLen = sizeof(readBrightnessPDUNoBody),
                                                    .results = readBrightnessResultsNoBody,
                                                    .numResults = 1 };

// Read Brightness
static const uint8_t readBrightnessPDU[] = { READ_PDU(lightBulbBrightnessCharacteristic.iid, 1) };
static const PduResult_t readBrightnessResults[] = { BODY_32B_RESULTS(1, 50) };
static const PduTest_t readBrightnessTest = { .pdu = readBrightnessPDU,
                                              .pduLen = sizeof(readBrightnessPDU),
                                              .results = readBrightnessResults,
                                              .numResults = 1 };

// Read Temperature
static const uint8_t readTempPDU[] = { READ_PDU(lightBulbColorTemperatureCharacteristic.iid, 2) };
static const PduResult_t readTempResults[] = { BODY_32B_RESULTS(2, 200) };
static const PduTest_t readTempTest = { .pdu = readTempPDU,
                                        .pduLen = sizeof(readTempPDU),
                                        .results = readTempResults,
                                        .numResults = 1 };

// Read Brightness and Temperature Together
static const uint8_t readBrightAndTempPDU[] = { READ_PDU(lightBulbBrightnessCharacteristic.iid, 1),
                                                READ_PDU(lightBulbColorTemperatureCharacteristic.iid, 2) };
static const PduResult_t readBrightAndTempResults[] = {
    BODY_32B_RESULTS(1, 50),
    BODY_32B_RESULTS(2, 200),
};
static const PduTest_t readBrightAndTempTest = { .pdu = readBrightAndTempPDU,
                                                 .pduLen = sizeof(readBrightAndTempPDU),
                                                 .results = readBrightAndTempResults,
                                                 .numResults = 2 };

// Write Brightness
static const uint8_t writeBrightPDU[] = { WRITE_32B_PDU(lightBulbBrightnessCharacteristic.iid, 3, 90) };
static const PduResult_t writeBrightResults[] = { NO_BODY_RESULTS(3) };
static const PduTest_t writeBrightTest = { .pdu = writeBrightPDU,
                                           .pduLen = sizeof(writeBrightPDU),
                                           .results = writeBrightResults,
                                           .numResults = 1 };

// Write Temperature And Brightness together
static const uint8_t writeBrightAndTempPDU[] = { WRITE_32B_PDU(lightBulbBrightnessCharacteristic.iid, 4, 40),
                                                 WRITE_32B_PDU(lightBulbColorTemperatureCharacteristic.iid, 5, 250) };
static const PduResult_t writeBrightAndTempResults[] = { NO_BODY_RESULTS(4), NO_BODY_RESULTS(5) };
static const PduTest_t writeBrightAndTempTest = { .pdu = writeBrightAndTempPDU,
                                                  .pduLen = sizeof(writeBrightAndTempPDU),
                                                  .results = writeBrightAndTempResults,
                                                  .numResults = 2 };

// Read and write mixed together
static const uint8_t readWriteMixPDU[] = { READ_PDU(lightBulbBrightnessCharacteristic.iid, 6),
                                           WRITE_32B_PDU(lightBulbColorTemperatureCharacteristic.iid, 7, 300),
                                           READ_PDU(lightBulbColorTemperatureCharacteristic.iid, 8),
                                           WRITE_32B_PDU(lightBulbBrightnessCharacteristic.iid, 9, 20),
                                           READ_PDU(lightBulbBrightnessCharacteristic.iid, 10) };
static const PduResult_t readWriteMixResults[] = {
    BODY_32B_RESULTS(6, 40), NO_BODY_RESULTS(7), BODY_32B_RESULTS(8, 300), NO_BODY_RESULTS(9), BODY_32B_RESULTS(10, 20),
};
static const PduTest_t readWriteMixTest = { .pdu = readWriteMixPDU,
                                            .pduLen = sizeof(readWriteMixPDU),
                                            .results = readWriteMixResults,
                                            .numResults = 5 };

// Register for notifications
static const uint8_t registerPDU[] = { REGISTER_PDU(lightBulbBrightnessCharacteristic.iid, 11),
                                       REGISTER_PDU(lightBulbColorTemperatureCharacteristic.iid, 12) };
static const PduResult_t registerResults[] = {
    NO_BODY_RESULTS(11),
    NO_BODY_RESULTS(12),
};
static const PduTest_t registerTest = { .pdu = registerPDU,
                                        .pduLen = sizeof(registerPDU),
                                        .results = registerResults,
                                        .numResults = 2 };

//---------------------------------------------------------------------------------------------
// Test Helper Functions

// Sends PDU to ADK
static void
        SendPDU(HAPAccessoryServer* server,
                HAPPlatformThreadCoAPManagerPeer* peer,
                HAPPlatformThreadCoAPManagerRequestCallback secureMessageRequestCallback,
                void* secureMessageRequestContext,
                const uint8_t* pduBytes,
                size_t sizePduBytes,
                uint8_t* responseBytes,
                size_t sizeResponseBytes,
                size_t* numResponseBytes) {

    size_t encryptedRequestSize = sizePduBytes + CHACHA20_POLY1305_TAG_BYTES;
    uint8_t* encryptedRequestBytes = (uint8_t*) malloc(encryptedRequestSize);

    // Encrypt the PDUs
    HAPChaCha20Poly1305EncryptHelper(
            &controllerToAccessoryKey,
            controllerToAccessoryNonce,
            encryptedRequestBytes,
            pduBytes,
            sizePduBytes,
            NULL,
            0);
    controllerToAccessoryNonce++;

    // Send pdus to Accessory
    HAPPlatformThreadCoAPManagerResponseCode responseCode;
    secureMessageRequestCallback(
            platform.thread.coapManager,
            peer,
            server->thread.coap.secureMessageResource,
            kHAPPlatformThreadCoAPManagerRequestCode_POST,
            encryptedRequestBytes,
            encryptedRequestSize,
            &responseCode,
            responseBytes,
            sizeResponseBytes,
            numResponseBytes,
            NULL,
            secureMessageRequestContext);

    // Decrypt what we got back.
    HAPError err = HAPChaCha20Poly1305DecryptHelper(
            &accessoryToControllerKey,
            accessoryToControllerNonce,
            responseBytes,
            responseBytes,
            *numResponseBytes,
            NULL,
            0);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    accessoryToControllerNonce++;
    *numResponseBytes -= CHACHA20_POLY1305_TAG_BYTES;
    free(encryptedRequestBytes);
}

// Find the PDU result in the test that matches the tid.  Order back is not guaranteed.
const PduResult_t* GetExpectedResult(const PduTest_t* test, uint8_t tid) {
    const PduResult_t* result = NULL;
    for (size_t i = 0; i < test->numResults && !result; i++) {
        if (test->results[i].tid == tid) {
            result = &test->results[i];
        }
    }
    return result;
}

// Ensure the result matches expectations.
void ProcessResult(uint8_t* responseBytes, size_t numResponseBytes, const PduTest_t* test) {
    size_t index = 0;
    HAPTLVReader bodyReader;
    HAPError err;
    HAPTLV tlv;
    bool valid;
    uint8_t tid;
    HAPPDUStatus status;

    // Walk through all response bytes
    while (index < numResponseBytes) {
        // resultBytes[0] == control
        tid = responseBytes[index + 1];
        status = (HAPPDUStatus) responseBytes[index + 2];
        const PduResult_t* expectedResult = GetExpectedResult(test, tid);
        TEST_ASSERT(expectedResult != NULL);
        TEST_ASSERT_EQUAL(status, expectedResult->status);

        TEST_ASSERT(numResponseBytes - index >= 5);
        // [3] and [4] = size of data to follow.
        size_t bodyLength = (size_t) responseBytes[index + 3] + ((size_t) responseBytes[index + 4] << 8);
        TEST_ASSERT_EQUAL(bodyLength, expectedResult->bodyLen);

        // Read the TLV we got back
        if (bodyLength) {
            HAPTLVReaderCreate(&bodyReader, &responseBytes[index + 5], bodyLength);
            err = HAPTLVReaderGetNext(&bodyReader, &valid, &tlv);
            HAPAssert(!err);
            HAPAssert(valid);

            // Ensure it's data matches what we expect.
            TEST_ASSERT_EQUAL(tlv.value.numBytes, expectedResult->tlvLen);
            uint32_t val = HAPReadLittleUInt32(tlv.value.bytes);
            TEST_ASSERT_EQUAL(val, expectedResult->intResult);
        }
        index += (5 + bodyLength);
    }
}

// Send a set of PDUs and verify the data returned matches.
void RunPduTest(
        HAPAccessoryServer* server,
        HAPPlatformThreadCoAPManagerPeer* peer,
        HAPPlatformThreadCoAPManagerRequestCallback secureMessageRequestCallback,
        void* secureMessageRequestContext,
        const PduTest_t* test) {
    uint8_t responseBytes[512];
    size_t numResponseBytes;

    // Issue PDU
    SendPDU(server,
            peer,
            secureMessageRequestCallback,
            secureMessageRequestContext,
            test->pdu,
            test->pduLen,
            responseBytes,
            sizeof(responseBytes),
            &numResponseBytes);

    // Verify Results
    ProcessResult(responseBytes, numResponseBytes, test);
}

//---------------------------------------------------------------------------------------------
// Test functions.

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

// This test creates Coap Messages consisting of various numbers of PDUS.  It ensures that
// The expected PDU responses are returned.
TEST(TestPdu) {
    HAP_PLATFORM_THREAD_MOCK(threadMock);
    HAP_PLATFORM_COAP_MANAGER_MOCK(coapManagerMock);

    // Prep the environment.  Set up a Thread Accessory server
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

    // Register CoAP resources.  This lets us grab the secureMessage callback, which we will use to inject
    // CoAP messages to the ADK.
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

    // Also grab Pair Verify so we can complete our pseudo-pairing.
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

    // Begin running our actual tests.
    // Read One Item
    RunPduTest(server, &peer, secureMessageRequestCallback, secureMessageRequestContext, &readBrightnessTestNoBody);
    RunPduTest(server, &peer, secureMessageRequestCallback, secureMessageRequestContext, &readBrightnessTest);
    RunPduTest(server, &peer, secureMessageRequestCallback, secureMessageRequestContext, &readTempTest);
    // Read many items
    RunPduTest(server, &peer, secureMessageRequestCallback, secureMessageRequestContext, &readBrightAndTempTest);

    // Write One item
    RunPduTest(server, &peer, secureMessageRequestCallback, secureMessageRequestContext, &writeBrightTest);
    // Write Many items
    RunPduTest(server, &peer, secureMessageRequestCallback, secureMessageRequestContext, &writeBrightAndTempTest);

    // Read and Write many items mixed together (important for checking body vs no body)
    RunPduTest(server, &peer, secureMessageRequestCallback, secureMessageRequestContext, &readWriteMixTest);

    // Register for some events.
    RunPduTest(server, &peer, secureMessageRequestCallback, secureMessageRequestContext, &registerTest);

    // Check to see that we get those events when triggered
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
                            TEST_ASSERT(
                                    (iid == lightBulbBrightnessCharacteristic.iid) ||
                                    (iid == lightBulbColorTemperatureCharacteristic.iid));
                            HAPLogBufferDebug(
                                    &logObject, tlv.value.bytes, tlv.value.numBytes, "Notified lightbulb brightness");
                        });

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
            .AtLeast(2);

    // Raise one event
    HAPAccessoryServerRaiseEvent(server, &lightBulbBrightnessCharacteristic, &lightBulbService, &accessory);
    HAPPlatformTimerEmulateClockAdvances(HAPSecond);

    // And clear it.
    TEST_ASSERT(completeSendEvent);
    completeSendEvent();
    HAPPlatformTimerEmulateClockAdvances(60 * HAPSecond);

    // Raise the other event
    HAPAccessoryServerRaiseEvent(server, &lightBulbColorTemperatureCharacteristic, &lightBulbService, &accessory);

    // And clear that too
    completeSendEvent();
    HAPPlatformTimerEmulateClockAdvances(60 * HAPSecond);

    // Verify all notifications were received.
    VERIFY_ALL(coapManagerMock);
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
