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

#include "HAPOPACK.h"
#include "HAPPlatformFeatures.h"

#include "Harness/DataStreamTestHelpers.h"
#include "Harness/HAPAccessoryServerHelper.h"
#include "Harness/HAPPlatformClockHelper.h"
#include "Harness/IPTestHelpers.h"
#include "Harness/SecurityTestHelpers.h"

#include "Harness/UnitTestFramework.h"

typedef struct {
    DataStreamEncryptionContext encryptionContext;
    HAPPlatformTCPStreamRef clientTCPStream;
} DataStreamTCPContext;

typedef struct {
    HAPAccessory* accessory;
    HAPSession session;
    DataStreamTCPContext tcpContext;
} TestStreamContext;

//----------------------------------------------------------------------------------------------------------------------

#define kStreamMaxSends 2

/* This function is a callback for the accessory. It is the dataStream.delegate.handleInvalidate callback.*/
static void HandleDataStreamInvalidate(
        HAPAccessoryServer* server,
        const HAPDataStreamRequest* request,
        HAPDataStreamRef* dataStream,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->session);
    HAPPrecondition(request->service == &dataStreamTransportManagementService);
    HAPPrecondition(request->accessory);
    HAPPrecondition(dataStream);
    HAPPrecondition(context);
    TestContext* test = (TestContext*) context;
    HAPLogError(&kHAPLog_Default, "HomeKit Data Stream invalidated.");
    HAPRawBufferZero(test, sizeof *test);
}
const HAPDataStreamCallbacks hdsDataStreamCallbacks = { .handleAccept = DataStreamTestHelpersHandleDataStreamAccept,
                                                        .handleInvalidate = HandleDataStreamInvalidate,
                                                        .handleData = DataStreamTestHelpersHandleDataStreamData };

// Controller IDs for KVS
#define kAdminControllerID    1
#define kNonAdminControllerID 4

// IP Accessory Server Storage
static HAPAccessoryServer ipAccessoryServer;
static HAPAccessoryServerCallbacks ipAccessoryServerCallbacks;
static HAPIPSession ipSessions[kHAPIPSessionStorage_DefaultNumElements];
static IPSessionState ipSessionStates[HAPArrayCount(ipSessions)];
static HAPIPReadContext ipReadContexts[kAttributeCount];
static HAPIPWriteContext ipWriteContexts[kAttributeCount];
static uint8_t ipScratchBuffer[kHAPIPSession_DefaultScratchBufferSize];
static HAPIPAccessoryServerStorage ipAccessoryServerStorage;

static HAPDataStreamRef ipDataStreams[kApp_NumDataStreams];
static TestContext ipTestContext;

// BLE specific variables are only used wth HDS over HAP.
static HAPDataStreamRef bleDataStreams[kApp_NumDataStreams];
static TestContext bleTestContext;

static HAPAccessoryServer bleAccessoryServer;
static HAPAccessoryServerCallbacks bleAccessoryServerCallbacks;
// Prepare accessory server storage.
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

extern "C" {

static HAPAccessory InitAccessory(void) {
    HAPAccessory accessory;
    HAPRawBufferZero(&accessory, sizeof(accessory));
    accessory.aid = 1;
    accessory.category = kHAPAccessoryCategory_Other;
    accessory.name = "Acme Test";
    accessory.productData = "03d8a775e3644573";
    accessory.manufacturer = "Acme";
    accessory.model = "Test1,1";
    accessory.serialNumber = "099DB48E9E28";
    accessory.firmwareVersion = "1";
    accessory.hardwareVersion = "1";
    static const HAPService* services[] = { &accessoryInformationService,
                                            &hapProtocolInformationService,
                                            &pairingService,
                                            &dataStreamTransportManagementService,
                                            NULL };
    accessory.services = services;
    accessory.callbacks.identify = IdentifyAccessoryHelper;
    return accessory;
}

static HAPAccessory ipAccessory = InitAccessory();

static HAPAccessory bleAccessory = InitAccessory();

} // extern "C"
//----------------------------------------------------------------------------------------------------------------------

static void TestCreateHAPDataStream(
        HAPAccessoryServer* server,
        HAPAccessory* accessory,
        HAPSession* session,
        TestContext* test) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);
    HAPPrecondition(session);
    HAPPrecondition(test);

    HAPError err;
    // Setup HomeKit Data Stream Transport.
    err = DataStreamTestHelpersWriteSetupDataStreamTransport(server, accessory, session, NULL, test);
    HAPAssert(!err);

    uint8_t setupStatus;
    HAPNetworkPort listeningPort;
    HAPDataStreamSalt accessoryKeySalt;

    HAPDataStreamHAPSessionIdentifier sessionIdentifier;
    DataStreamTestHelpersReadSetupDataStreamTransport(
            server,
            accessory,
            session,
            (HAPCharacteristicValue_SetupDataStreamTransport_Status*) &setupStatus,
            &listeningPort,
            &accessoryKeySalt,
            &sessionIdentifier,
            test);
    TEST_ASSERT_EQUAL(setupStatus, 0);
    TEST_ASSERT(sessionIdentifier != kHAPDataStreamHAPSessionIdentifierNone);

    TEST_ASSERT_EQUAL(listeningPort, kHAPNetworkPort_Any);

    TEST_ASSERT(HAPRawBufferIsZero(accessoryKeySalt.bytes, sizeof(accessoryKeySalt.bytes)));

    // Connect HomeKit Data Stream.
    HAPLog(&kHAPLog_Default, "Connecting HomeKit Data Stream.");

    // Send control.hello request.
    HAPLog(&kHAPLog_Default, "Sending control.hello request.");
    {
        // clang-format off
        const uint8_t header[] = {
                0xE3,
                0x48, 'p',  'r', 'o', 't', 'o',  'c', 'o',  'l',
                0x47, 'c', 'o', 'n',  't',  'r', 'o', 'l',
                0x47, 'r', 'e',  'q', 'u',  'e', 's',  't',
                0x45, 'h', 'e', 'l', 'l',  'o',
                0x42, 'i', 'd',
                0x08
        };
        HAP_UNUSED const uint8_t message[] = {
                0xE0
        };
        size_t const receiveFrameSize = HAP_DATASTREAM_PAYLOAD_HEADER_LENGTH + sizeof header + sizeof message;
        uint8_t frame[HAP_DATASTREAM_FRAME_HEADER_LENGTH + receiveFrameSize];
        // clang-format on
        {
            DataStreamTestHelpersDataStreamEncryptControllerPacketWithContext(
                    NULL, frame, sizeof(frame), header, sizeof(header), message, sizeof(message));
            // Ignore next frame.
            test->expectsData = true;
            test->nextDataBytes = NULL;
            test->numNextDataBytes = receiveFrameSize;

            err = DataStreamTestHelpersBuildControllerHAPTransportWrite(
                    server, accessory, session, sessionIdentifier, frame, sizeof frame, false, test);
            HAPAssert(!err);
        }
        {
            HAPAssert(test->dataStream);
            HAPAssert(test->session == session);

            HAPAssert(!test->expectsData);
            HAPAssert(!test->expectsCompletion);
            HAPAssert(!test->error);
            HAPAssert(test->dataBytes == NULL);
            HAPAssert(test->numDataBytes == receiveFrameSize);
            HAPAssert(test->isComplete);
        }
    }
    {
        HAPLog(&kHAPLog_Default, "Sending initial frame response (Accessory -> Controller).");
        const uint8_t dataBytes[] = "hello, world";
        const uint8_t aadBytes[] = { kHAPDataStreamFrameType_UnencryptedMessage, HAPExpandBigUInt24(sizeof dataBytes) };
        uint8_t frameBytes[HAP_DATASTREAM_FRAME_HEADER_LENGTH + sizeof dataBytes];
        {
            // Prepare sending data.
            //   - make a copy (technically the SendMutableData is allowed to modify this buffer)
            uint8_t sentDataBytes[sizeof dataBytes];
            HAPRawBufferCopyBytes(sentDataBytes, dataBytes, sizeof dataBytes);

            DataStreamTestHelpersPrepareAndSendMutableData(
                    server, HAPNonnull(test->dataStream), sentDataBytes, sizeof sentDataBytes, test);

            size_t sentLength;
            bool requestToSend;
            uint8_t numResponseTLVs;
            err = DataStreamTestHelpersPerformControllerHAPTransportRead(
                    server,
                    accessory,
                    session,
                    frameBytes,
                    sizeof frameBytes,
                    &sentLength,
                    &requestToSend,
                    &numResponseTLVs,
                    NULL);
            HAPAssert(!err);
            HAPAssert(sentLength == sizeof frameBytes);
            HAPAssert(!requestToSend);
            // Verify transmission completed.
            HAPAssert(!test->expectsCompletion);
            HAPAssert(!test->error);
            HAPAssert(test->dataBytes == sentDataBytes);
            HAPAssert(test->numDataBytes == sizeof sentDataBytes);
            HAPAssert(test->isComplete);
        }
        {
            // Verify frame.
            HAPAssert(HAPRawBufferAreEqual(&frameBytes[0], aadBytes, sizeof aadBytes));
            HAPAssert(HAPRawBufferAreEqual(&frameBytes[sizeof aadBytes], dataBytes, sizeof dataBytes));
        }
    }
}

/*BLE functions are only used wth HDS over HAP.*/
static void BLEAccessoryServerSetup(HAPAccessoryServer* server, HAPAccessory* accessory) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);
    HAPError err;

    // Initialize accessory server.
    HAPAccessoryServerOptions bleAccessoryServerOptions;
    HAPRawBufferZero(&bleAccessoryServerOptions, sizeof(bleAccessoryServerOptions));
    bleAccessoryServerOptions.maxPairings = kHAPPairingStorage_MinElements;
    bleAccessoryServerOptions.ble.transport = &kHAPAccessoryServerTransport_BLE;
    bleAccessoryServerOptions.ble.accessoryServerStorage = &bleAccessoryServerStorage;
    bleAccessoryServerOptions.ble.preferredAdvertisingInterval = kHAPBLEAdvertisingInterval_Minimum;
    bleAccessoryServerOptions.ble.preferredNotificationDuration = kHAPBLENotification_MinDuration;
    bleAccessoryServerCallbacks = { .handleUpdatedState = HAPAccessoryServerHelperHandleUpdatedAccessoryServerState };

    // Initialize accessory server .
    bleAccessoryServerOptions.dataStream.dataStreams = bleDataStreams;
    bleAccessoryServerOptions.dataStream.numDataStreams = HAPArrayCount(bleDataStreams);
    HAPAccessoryServerCreate(
            server, &bleAccessoryServerOptions, &platform, &bleAccessoryServerCallbacks, &bleTestContext);
    accessory->dataStream.delegate.callbacks = &hdsDataStreamCallbacks;
    accessory->dataStream.delegate.context = NULL;

    // Start accessory server.
    HAPAccessoryServerStart(server, accessory);
    HAPPlatformClockAdvance(0);
    HAPAssert(HAPAccessoryServerGetState(server) == kHAPAccessoryServerState_Running);

    // Discover BLE accessory server.
    HAPAccessoryServerInfo serverInfo;
    HAPPlatformBLEPeripheralManagerDeviceAddress deviceAddress;
    err = HAPDiscoverBLEAccessoryServer(HAPNonnull(platform.ble.blePeripheralManager), &serverInfo, &deviceAddress);
    HAPAssert(!err);
    HAPAssert(!serverInfo.statusFlags.isNotPaired);
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
static int RetrieveNumberDataStreamsForPairing(HAPDataStreamRef* dataStreams, int numDataStreams, int pairingID) {
    HAPPrecondition(dataStreams);
    // Keep track of how many data streams exist with this pairing.
    int counter = 0;

    // Loop through the data streams, and try to find the specified pairing.
    for (int index = 0; index < numDataStreams; index++) {
        // The stream's pairing ID only matters if the dataStream has been initalized.
        if (dataStreams[index].hap.state != HAPDataStreamHAPState_Uninitialized) {
            if (dataStreams[index].hap.pairingID == pairingID) {
                counter++;
            }
        }
    }
    return counter;
}

//----------------------------------------------------------------------------------------------------------------------

static void IPAccessoryServerSetup(HAPAccessoryServer* server, HAPAccessory* accessory) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);
    HAPError err;

    // Prepare accessory server storage.
    InitializeIPSessions(ipSessions, ipSessionStates, HAPArrayCount(ipSessions));
    ipAccessoryServerStorage = { .sessions = ipSessions,
                                 .numSessions = HAPArrayCount(ipSessions),
                                 .readContexts = ipReadContexts,
                                 .numReadContexts = HAPArrayCount(ipReadContexts),
                                 .writeContexts = ipWriteContexts,
                                 .numWriteContexts = HAPArrayCount(ipWriteContexts),
                                 .scratchBuffer = { .bytes = ipScratchBuffer, .numBytes = sizeof ipScratchBuffer } };

    // Initialize accessory server.
    HAPAssert(HAPGetCompatibilityVersion() == HAP_COMPATIBILITY_VERSION);
    HAPAccessoryServerOptions ipAccessoryServerOptions;
    HAPRawBufferZero(&ipAccessoryServerOptions, sizeof(ipAccessoryServerOptions));
    ipAccessoryServerOptions.maxPairings = kHAPPairingStorage_MinElements;
    ipAccessoryServerOptions.dataStream.dataStreams = ipDataStreams;
    ipAccessoryServerOptions.dataStream.numDataStreams = HAPArrayCount(ipDataStreams);
    ipAccessoryServerOptions.ip.transport = &kHAPAccessoryServerTransport_IP;
    ipAccessoryServerOptions.ip.accessoryServerStorage = &ipAccessoryServerStorage;

    // Initialize accessory server - HDS OVER HAP specific options.
    ipAccessoryServerOptions.dataStream.dataStreams = ipDataStreams;
    ipAccessoryServerOptions.dataStream.numDataStreams = HAPArrayCount(ipDataStreams);
    ipAccessoryServerCallbacks = { .handleUpdatedState = HAPAccessoryServerHelperHandleUpdatedAccessoryServerState };

    // Create Accessory Server for HDS over HAP.
    HAPAccessoryServerCreate(server, &ipAccessoryServerOptions, &platform, &ipAccessoryServerCallbacks, &ipTestContext);

    accessory->dataStream.delegate.callbacks = &hdsDataStreamCallbacks;
    accessory->dataStream.delegate.context = NULL;
    // Start accessory server.
    HAPAccessoryServerStart(server, accessory);

    // Process events.
    HAPPlatformClockAdvance(0);
    HAPAssert(HAPAccessoryServerGetState(server) == kHAPAccessoryServerState_Running);

    // Discover IP accessory server.
    HAPAccessoryServerInfo serverInfo;
    HAPNetworkPort serverPort;
    err = HAPDiscoverIPAccessoryServer(HAPNonnull(platform.ip.serviceDiscovery), &serverInfo, &serverPort);
    HAPAssert(!err);
    HAPAssert(!serverInfo.statusFlags.isNotPaired);
}

static void TestCreateDataStream(
        HAPAccessoryServer* server,
        HAPAccessory* accessory,
        HAPPlatformKeyValueStoreKey controllerPairingID,
        TestContext* testContext,
        TestStreamContext* streamContext) {
    // Create Fake Security Session
    TestCreateFakeSecuritySession(&(streamContext->session), server, controllerPairingID);

    // Create the Homekit Data Stream.
    HAPPrecondition(testContext);
    HAPPrecondition(streamContext);

    TestCreateHAPDataStream(server, accessory, &(streamContext->session), testContext);
}

//----------------------------------------------------------------------------------------------------------------------

static void CheckHDSTransportConfiguration(HAPAccessoryServer* server, HAPAccessory* accessory) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);

    HAPSession session;
    // Get Supported Data Stream Transport Configuration.
    bool supportsTCP;
    bool supportsHAP;
    DataStreamTestHelpersReadSupportedDataStreamTransportConfiguration(
            server, accessory, &session, &supportsTCP, &supportsHAP, NULL);
    HAPAssert(supportsHAP);
    HAPAssert(!supportsTCP);
}

static void TestAddRemovePairings(
        HAPAccessoryServer* server,
        HAPAccessory* accessory,
        TestContext* testContext,
        HAPDataStreamRef* dataStreams,
        int numDataStreams,
        TestStreamContext* adminControllerStreamContext,
        int numAdminControllerStreams,
        TestStreamContext* nonAdminControllerStreamContext,
        int numNonAdminControllerStreams,
        bool removeAdminPairing) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);
    HAPPrecondition(testContext);
    HAPPrecondition(dataStreams);
    HAPPrecondition(adminControllerStreamContext);
    HAPPrecondition(nonAdminControllerStreamContext);

    // Holds the number of streams for a controller pairing ID.
    int numStreamsForPairing;

    // Make sure there are no streams for the Admin Controller pairing.
    numStreamsForPairing = RetrieveNumberDataStreamsForPairing(dataStreams, numDataStreams, kAdminControllerID);
    TEST_ASSERT(!numStreamsForPairing);

    // Make sure there are no streams for the nonAdmin Controller pairing.
    numStreamsForPairing = RetrieveNumberDataStreamsForPairing(dataStreams, numDataStreams, kNonAdminControllerID);
    TEST_ASSERT(!numStreamsForPairing);

    // Create data streams for the Admin Controller.
    for (int index = 0; index < numAdminControllerStreams; index++) {
        TestCreateDataStream(server, accessory, kAdminControllerID, testContext, &adminControllerStreamContext[index]);
    }

    // Create data streams for the nonAdmin Controller.
    for (int index = 0; index < numNonAdminControllerStreams; index++) {
        TestCreateDataStream(
                server, accessory, kNonAdminControllerID, testContext, &nonAdminControllerStreamContext[index]);
    }

    // Make sure the correct number of streams exist for the Admin Controller pairing.
    numStreamsForPairing = RetrieveNumberDataStreamsForPairing(dataStreams, numDataStreams, kAdminControllerID);
    TEST_ASSERT(numStreamsForPairing == numAdminControllerStreams);
    // Make sure the correct number of streams exist for the nonAdmin Controller pairing.
    numStreamsForPairing = RetrieveNumberDataStreamsForPairing(dataStreams, numDataStreams, kNonAdminControllerID);
    TEST_ASSERT(numStreamsForPairing == numNonAdminControllerStreams);

    if (removeAdminPairing) {
        // Invalidate data streams for Admin Controller. Note that at the interface level we are testing, removing the
        // only Admin controller will not remove all other controllers. That is done one level above the interface we
        // are testing.
        HAPDataStreamInvalidateAllForHAPPairingID(server, kAdminControllerID);
        // Make sure no data streams with the Admin Controller pairing exist,
        numStreamsForPairing = RetrieveNumberDataStreamsForPairing(dataStreams, numDataStreams, kAdminControllerID);
        TEST_ASSERT(!numStreamsForPairing);
        // Make sure the correct number of data streams exist for the nonAdmin Controller pairing.
        numStreamsForPairing = RetrieveNumberDataStreamsForPairing(dataStreams, numDataStreams, kNonAdminControllerID);
        TEST_ASSERT(numStreamsForPairing == numNonAdminControllerStreams);
    } else {

        // Invalidate data streams for non Admin Controller.
        HAPDataStreamInvalidateAllForHAPPairingID(server, kNonAdminControllerID);
        // Make sure no data streams with the nonAdmin Controller pairing exist,
        numStreamsForPairing = RetrieveNumberDataStreamsForPairing(dataStreams, numDataStreams, kNonAdminControllerID);
        TEST_ASSERT(!numStreamsForPairing);
        // Make sure the correct number of data streams exist for the Admin Controller pairing.
        numStreamsForPairing = RetrieveNumberDataStreamsForPairing(dataStreams, numDataStreams, kAdminControllerID);
        TEST_ASSERT(numStreamsForPairing == numAdminControllerStreams);
    }
}

static void TestSuiteSetup() {
    HAPPlatformCreate();

    // Prepare key-value store. Set up Admin and nonAdmin Controllers.
    HAPControllerPublicKey controllerPublicKey;
    TestPrepareKeyValueStore(kAdminControllerID, &controllerPublicKey);
    TestKeyValueStoreAddControllerPairing(kNonAdminControllerID, &controllerPublicKey, false);

    HAPRawBufferZero(&ipTestContext, sizeof ipTestContext);
    IPAccessoryServerSetup(&ipAccessoryServer, &ipAccessory);

    // BLE Accessory server is only used for HAP over HDS.
    HAPRawBufferZero(&bleTestContext, sizeof bleTestContext);
    BLEAccessoryServerSetup(&bleAccessoryServer, &bleAccessory);
}

static void TestSuiteTeardown() {
    // Stop accessory server.
    HAPLog(&kHAPLog_Default, "Stopping IP accessory server.");
    HAPAccessoryServerForceStop(&ipAccessoryServer);
    HAPPlatformClockAdvance(0);
    HAPPlatformClockAdvance(0);
    HAPAssert(HAPAccessoryServerGetState(&ipAccessoryServer) == kHAPAccessoryServerState_Idle);

    // BLE Accessory server is only used for HAP over HDS .
    HAPLog(&kHAPLog_Default, "Stopping BLE accessory server.");
    HAPAccessoryServerForceStop(&bleAccessoryServer);
    HAPPlatformClockAdvance(0);
    HAPPlatformClockAdvance(0);
    HAPAssert(HAPAccessoryServerGetState(&bleAccessoryServer) == kHAPAccessoryServerState_Idle);
}

TEST_TEARDOWN() {
    // Runs after every TEST()
    HAPDataStreamInvalidateAllForHAPSession(&ipAccessoryServer, NULL);

    // BLE Accessory server is only used for HAP over HDS .
    HAPDataStreamInvalidateAllForHAPSession(&bleAccessoryServer, NULL);
}

TEST(CheckHDSVersionIP) {
    // Check HomeKit Data Stream Version for IP.
    HAPSession ipSession;
    DataStreamTestHelpersCheckVersion(&ipAccessoryServer, &ipAccessory, &ipSession, NULL);
}

TEST(CheckHDSTransportConfigurationIP) {
    // Check HomeKit Data Stream Transport Configuration for IP.
    CheckHDSTransportConfiguration(&ipAccessoryServer, &ipAccessory);
}

TEST(TestRemoveAdminPairingIP) {
    // Number of streams per controller.
    int numAdminControllerStreams = 2;
    int numNonAdminControllerStreams = 3;

    // Contexts for each Admin Controller HDS.
    TestStreamContext adminControllerStreamContext[numAdminControllerStreams];
    // Contexts for each non Admin Controller HDS.
    TestStreamContext nonAdminControllerStreamContext[numNonAdminControllerStreams];

    TestAddRemovePairings(
            &ipAccessoryServer,
            &ipAccessory,
            &ipTestContext,
            (HAPDataStreamRef*) &ipDataStreams,
            HAPArrayCount(ipDataStreams),
            (TestStreamContext*) &adminControllerStreamContext,
            numAdminControllerStreams,
            (TestStreamContext*) &nonAdminControllerStreamContext,
            numNonAdminControllerStreams,
            true);
}

TEST(TestRemoveNonAdminPairingIP) {
    // Number of streams per controller.
    int numAdminControllerStreams = 2;
    int numNonAdminControllerStreams = 3;

    // Contexts for each Admin Controller HDS.
    TestStreamContext adminControllerStreamContext[numAdminControllerStreams];
    // Contexts for each non Admin Controller HDS.
    TestStreamContext nonAdminControllerStreamContext[numNonAdminControllerStreams];

    TestAddRemovePairings(
            &ipAccessoryServer,
            &ipAccessory,
            &ipTestContext,
            (HAPDataStreamRef*) &ipDataStreams,
            HAPArrayCount(ipDataStreams),
            (TestStreamContext*) &adminControllerStreamContext,
            numAdminControllerStreams,
            (TestStreamContext*) &nonAdminControllerStreamContext,
            numNonAdminControllerStreams,
            false);
}

/*BLE Test functions are only used wth HDS over HAP.*/
TEST(CheckHDSVersionBLE) {
    // Check HomeKit Data Stream Version for BLE.
    HAPSession bleSession;
    DataStreamTestHelpersCheckVersion(&bleAccessoryServer, &bleAccessory, &bleSession, NULL);
}

TEST(CheckHDSTransportConfigurationBLE) {
    // Check HomeKit Data Stream Transport Configuration for BLE.
    CheckHDSTransportConfiguration(&bleAccessoryServer, &bleAccessory);
}

TEST(TestRemoveAdminPairingBLE) {
    // Number of streams per controller.
    int numAdminControllerStreams = 2;
    int numNonAdminControllerStreams = 3;

    // Contexts for each Admin Controller HDS.
    TestStreamContext adminControllerStreamContext[numAdminControllerStreams];
    // Contexts for each non Admin Controller HDS.
    TestStreamContext nonAdminControllerStreamContext[numNonAdminControllerStreams];

    TestAddRemovePairings(
            &bleAccessoryServer,
            &bleAccessory,
            &bleTestContext,
            (HAPDataStreamRef*) &bleDataStreams,
            HAPArrayCount(bleDataStreams),
            (TestStreamContext*) &adminControllerStreamContext,
            numAdminControllerStreams,
            (TestStreamContext*) &nonAdminControllerStreamContext,
            numNonAdminControllerStreams,
            true);
}

TEST(TestRemoveNonAdminPairingBLE) {
    // Number of streams per controller.
    int numAdminControllerStreams = 2;
    int numNonAdminControllerStreams = 3;

    // Contexts for each Admin Controller HDS.
    TestStreamContext adminControllerStreamContext[numAdminControllerStreams];
    // Contexts for each non Admin Controller HDS.
    TestStreamContext nonAdminControllerStreamContext[numNonAdminControllerStreams];

    TestAddRemovePairings(
            &bleAccessoryServer,
            &bleAccessory,
            &bleTestContext,
            (HAPDataStreamRef*) &bleDataStreams,
            HAPArrayCount(bleDataStreams),
            (TestStreamContext*) &adminControllerStreamContext,
            numAdminControllerStreams,
            (TestStreamContext*) &nonAdminControllerStreamContext,
            numNonAdminControllerStreams,
            false);
}
//----------------------------------------------------------------------------------------------------------------------

int main(int argc, char** argv) {
    int execute_result;

    TestSuiteSetup();
    execute_result = EXECUTE_TESTS(argc, (const char**) argv);
    TestSuiteTeardown();

    return execute_result;
}
