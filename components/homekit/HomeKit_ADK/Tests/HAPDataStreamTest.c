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

#include "HAPPlatform.h"

#include "Harness/DataStreamTestHelpers.h"
#include "Harness/HAPAccessoryServerHelper.h"
#include "Harness/HAPPlatformClockHelper.h"
#include "Harness/IPTestHelpers.h"
#include "Harness/SecurityTestHelpers.h"

static HAPAccessory accessory = { .aid = 1,
                                  .category = kHAPAccessoryCategory_Other,
                                  .name = "Acme Test",
                                  .productData = "03d8a775e3644573",
                                  .manufacturer = "Acme",
                                  .model = "Test1,1",
                                  .serialNumber = "099DB48E9E28",
                                  .firmwareVersion = "1",
                                  .hardwareVersion = "1",
                                  .services = (const HAPService* const[]) { &accessoryInformationService,
                                                                            &hapProtocolInformationService,
                                                                            &pairingService,
                                                                            &dataStreamTransportManagementService,
                                                                            NULL },
                                  .callbacks = { .identify = IdentifyAccessoryHelper } };

int main() {
    HAPError err;
    HAPPlatformCreate();

    TestContext test;
    HAPRawBufferZero(&test, sizeof test);

    // Prepare key-value store.
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    HAPControllerPublicKey controllerPublicKey;
    TestPrepareKeyValueStore(controllerPairingID, &controllerPublicKey);

    // Prepare accessory server storage.
    HAPIPSession ipSessions[kHAPIPSessionStorage_DefaultNumElements];
    IPSessionState ipSessionStates[HAPArrayCount(ipSessions)];
    HAPIPReadContext ipReadContexts[kAttributeCount];
    HAPIPWriteContext ipWriteContexts[kAttributeCount];
    uint8_t ipScratchBuffer[kHAPIPSession_DefaultScratchBufferSize];
    InitializeIPSessions(ipSessions, ipSessionStates, HAPArrayCount(ipSessions));
    HAPIPAccessoryServerStorage ipAccessoryServerStorage = { .sessions = ipSessions,
                                                             .numSessions = HAPArrayCount(ipSessions),
                                                             .readContexts = ipReadContexts,
                                                             .numReadContexts = HAPArrayCount(ipReadContexts),
                                                             .writeContexts = ipWriteContexts,
                                                             .numWriteContexts = HAPArrayCount(ipWriteContexts),
                                                             .scratchBuffer = { .bytes = ipScratchBuffer,
                                                                                .numBytes = sizeof ipScratchBuffer } };

    // Prepare HomeKit Data Stream.
    HAPPlatformTCPStreamManager dataStreamTCPStreamManager;
    HAPPlatformTCPStream dataStreamTCPStreams[kApp_NumDataStreams];
    HAPPlatformTCPStreamManagerCreate(
            &dataStreamTCPStreamManager,
            &(const HAPPlatformTCPStreamManagerOptions) { .tcpStreams = dataStreamTCPStreams,
                                                          .numTCPStreams = HAPArrayCount(dataStreamTCPStreams),
                                                          .numBufferBytes = 1 });
    HAPDataStreamRef dataStreams[kApp_NumDataStreams];

    // Initialize accessory server.
    HAPAssert(HAPGetCompatibilityVersion() == HAP_COMPATIBILITY_VERSION);
    HAPAccessoryServer accessoryServer;
    HAPAccessoryServerCreate(
            &accessoryServer,
            &(const HAPAccessoryServerOptions) {
                    .maxPairings = kHAPPairingStorage_MinElements,
                    .dataStream = { .dataStreams = dataStreams, .numDataStreams = HAPArrayCount(dataStreams) },
                    .ip = { .transport = &kHAPAccessoryServerTransport_IP,
                            .accessoryServerStorage = &ipAccessoryServerStorage,
                            .dataStream = { .tcpStreamManager = &dataStreamTCPStreamManager } } },
            &platform,
            &(const HAPAccessoryServerCallbacks) { .handleUpdatedState =
                                                           HAPAccessoryServerHelperHandleUpdatedAccessoryServerState },
            &test);

    // Start accessory server.
    accessory.dataStream.delegate.callbacks = &dataStreamTestHelpersDataStreamCallbacks;
    accessory.dataStream.delegate.context = NULL;
    HAPAccessoryServerStart(&accessoryServer, &accessory);

    // Process events.
    HAPPlatformClockAdvance(0);
    HAPAssert(HAPAccessoryServerGetState(&accessoryServer) == kHAPAccessoryServerState_Running);

    // Discover IP accessory server.
    HAPAccessoryServerInfo serverInfo;
    HAPNetworkPort serverPort;
    err = HAPDiscoverIPAccessoryServer(HAPNonnull(platform.ip.serviceDiscovery), &serverInfo, &serverPort);
    HAPAssert(!err);
    HAPAssert(!serverInfo.statusFlags.isNotPaired);

    // Create fake security session.
    HAPSession session;
    TestCreateFakeSecuritySession(&session, &accessoryServer, controllerPairingID);

    // Check HomeKit Data Stream Version.
    DataStreamTestHelpersCheckVersion(&accessoryServer, &accessory, &session, &test);

    // Get Supported Data Stream Transport Configuration.
    bool supportsTCP;
    bool supportsHAP;
    DataStreamTestHelpersReadSupportedDataStreamTransportConfiguration(
            &accessoryServer, &accessory, &session, &supportsTCP, &supportsHAP, &test);
    HAPAssert(!supportsHAP);
    HAPAssert(supportsTCP);

    // Generate Controller Key Salt.
    HAPDataStreamSalt controllerKeySalt;
    HAPPlatformRandomNumberFill(controllerKeySalt.bytes, sizeof controllerKeySalt.bytes);

    // Setup HomeKit Data Stream Transport.
    err = DataStreamTestHelpersWriteSetupDataStreamTransport(
            &accessoryServer, &accessory, &session, &controllerKeySalt, &test);
    HAPAssert(!err);
    HAPCharacteristicValue_SetupDataStreamTransport_Status setupStatus;
    HAPNetworkPort listeningPort;
    HAPDataStreamSalt accessoryKeySalt;
    HAPDataStreamHAPSessionIdentifier sessionIdentifier;
    DataStreamTestHelpersReadSetupDataStreamTransport(
            &accessoryServer,
            &accessory,
            &session,
            &setupStatus,
            &listeningPort,
            &accessoryKeySalt,
            &sessionIdentifier,
            &test);
    HAPAssert(sessionIdentifier == kHAPDataStreamHAPSessionIdentifierNone);
    HAPAssert(setupStatus == 0);
    HAPAssert(listeningPort == HAPPlatformTCPStreamManagerGetListenerPort(&dataStreamTCPStreamManager));
    HAPAssert(!HAPRawBufferIsZero(accessoryKeySalt.bytes, sizeof(accessoryKeySalt.bytes)));

    // Derive encryption keys.
    DataStreamEncryptionContext encryptionContext;
    DataStreamTestHelpersDataStreamInitEncryptionContext(
            &session, &controllerKeySalt, &accessoryKeySalt, &encryptionContext);

    // Connect HomeKit Data Stream.
    HAPLog(&kHAPLog_Default, "Connecting HomeKit Data Stream.");
    HAPPlatformTCPStreamRef clientTCPStream;
    err = HAPPlatformTCPStreamManagerConnectToListener(&dataStreamTCPStreamManager, &clientTCPStream);
    HAPAssert(!err);

    // Skip frame sent over HomeKit Data Stream (Controller -> Accessory).
    HAPLog(&kHAPLog_Default, "Skipping frame received over HomeKit Data Stream (Controller -> Accessory)");
    {
        const uint8_t frameType = kHAPDataStreamFrameType_EncryptedMessage;
        const uint8_t dataBytes[] = "hello, world";
        const uint8_t aadBytes[] = { frameType, HAPExpandBigUInt24(sizeof dataBytes) };
        {
            // Encrypt.
            uint8_t nonce[] = { HAPExpandLittleUInt64(encryptionContext.controllerToAccessoryCount) };
            uint8_t authTag[CHACHA20_POLY1305_TAG_BYTES];
            uint8_t encryptedDataBytes[sizeof dataBytes];
            HAP_chacha20_poly1305_encrypt_aad(
                    authTag,
                    encryptedDataBytes,
                    dataBytes,
                    sizeof dataBytes,
                    aadBytes,
                    sizeof aadBytes,
                    nonce,
                    sizeof nonce,
                    encryptionContext.controllerToAccessoryKey.bytes);
            encryptionContext.controllerToAccessoryCount++;

            // Ignore next frame.
            test.expectsData = true;
            test.nextDataBytes = NULL;
            test.numNextDataBytes = sizeof dataBytes;

            // Send frame.
            size_t numBytes;
            err = HAPPlatformTCPStreamClientWrite(
                    &dataStreamTCPStreamManager, clientTCPStream, aadBytes, sizeof aadBytes, &numBytes);
            HAPAssert(!err);
            HAPAssert(numBytes == sizeof aadBytes);
            err = HAPPlatformTCPStreamClientWrite(
                    &dataStreamTCPStreamManager,
                    clientTCPStream,
                    encryptedDataBytes,
                    sizeof encryptedDataBytes,
                    &numBytes);
            HAPAssert(!err);
            HAPAssert(numBytes == sizeof encryptedDataBytes);
            err = HAPPlatformTCPStreamClientWrite(
                    &dataStreamTCPStreamManager, clientTCPStream, authTag, sizeof authTag, &numBytes);
            HAPAssert(!err);
            HAPAssert(numBytes == sizeof authTag);
        }
        {
            HAPAssert(test.dataStream);
            HAPAssert(test.session == &session);

            HAPAssert(!test.expectsData);
            HAPAssert(!test.expectsCompletion);
            HAPAssert(!test.error);
            HAPAssert(test.dataBytes == NULL);
            HAPAssert(test.numDataBytes == sizeof dataBytes);
            HAPAssert(test.isComplete);
        }
    }

    // Accept frame sent over HomeKit Data Stream (Controller -> Accessory).
    HAPLog(&kHAPLog_Default, "Accepting frame received over HomeKit Data Stream (Controller -> Accessory)");
    {
        const uint8_t frameType = kHAPDataStreamFrameType_EncryptedMessage;
        const uint8_t dataBytes[] = "hello, world";
        const uint8_t aadBytes[] = { frameType, HAPExpandBigUInt24(sizeof dataBytes) };
        uint8_t receivedDataBytes[sizeof dataBytes];
        {
            // Encrypt.
            uint8_t nonce[] = { HAPExpandLittleUInt64(encryptionContext.controllerToAccessoryCount) };
            uint8_t authTag[CHACHA20_POLY1305_TAG_BYTES];
            uint8_t encryptedDataBytes[sizeof dataBytes];
            HAP_chacha20_poly1305_encrypt_aad(
                    authTag,
                    encryptedDataBytes,
                    dataBytes,
                    sizeof dataBytes,
                    aadBytes,
                    sizeof aadBytes,
                    nonce,
                    sizeof nonce,
                    encryptionContext.controllerToAccessoryKey.bytes);
            encryptionContext.controllerToAccessoryCount++;

            // Accept next frame.
            test.expectsData = true;
            test.nextDataBytes = receivedDataBytes;
            test.numNextDataBytes = sizeof receivedDataBytes;

            // Send frame.
            size_t numBytes;
            err = HAPPlatformTCPStreamClientWrite(
                    &dataStreamTCPStreamManager, clientTCPStream, aadBytes, sizeof aadBytes, &numBytes);
            HAPAssert(!err);
            HAPAssert(numBytes == sizeof aadBytes);
            err = HAPPlatformTCPStreamClientWrite(
                    &dataStreamTCPStreamManager,
                    clientTCPStream,
                    encryptedDataBytes,
                    sizeof encryptedDataBytes,
                    &numBytes);
            HAPAssert(!err);
            HAPAssert(numBytes == sizeof encryptedDataBytes);
            err = HAPPlatformTCPStreamClientWrite(
                    &dataStreamTCPStreamManager, clientTCPStream, authTag, sizeof authTag, &numBytes);
            HAPAssert(!err);
            HAPAssert(numBytes == sizeof authTag);
        }
        {
            HAPAssert(!test.expectsData);
            HAPAssert(!test.expectsCompletion);
            HAPAssert(!test.error);
            HAPAssert(test.dataBytes == receivedDataBytes);
            HAPAssert(test.numDataBytes == sizeof receivedDataBytes);
            HAPAssert(test.isComplete);

            // Verify payload.
            HAPAssert(HAPRawBufferAreEqual(dataBytes, receivedDataBytes, sizeof dataBytes));
        }
    }

    // Send frame over HomeKit Data Stream (Accessory -> Controller).
    HAPLog(&kHAPLog_Default, "Sending frame over HomeKit Data Stream (Accessory -> Controller).");
    {
        const uint8_t frameType = kHAPDataStreamFrameType_EncryptedMessage;
        const uint8_t dataBytes[] = "hello, world";
        const uint8_t aadBytes[] = { frameType, HAPExpandBigUInt24(sizeof dataBytes) };
        uint8_t frameBytes[sizeof aadBytes + sizeof dataBytes + CHACHA20_POLY1305_TAG_BYTES];
        {
            size_t o = 0;
            size_t n;

            // Prepare sending data.
            test.expectsCompletion = true;
            HAPDataStreamPrepareData(
                    &accessoryServer,
                    HAPNonnull(test.dataStream),
                    sizeof dataBytes,
                    DataStreamTestHelpersHandleDataStreamDataComplete);
            err = HAPPlatformTCPStreamClientRead(
                    &dataStreamTCPStreamManager, clientTCPStream, &frameBytes[o], sizeof frameBytes - o, &n);
            HAPAssert(!err);
            o += n;

            // Verify prepare sending data completed.
            HAPAssert(!test.expectsCompletion);
            HAPAssert(!test.error);
            HAPAssert(!test.dataBytes);
            HAPAssert(!test.numDataBytes);
            HAPAssert(!test.isComplete);

            // Send data.
            test.expectsCompletion = true;
            uint8_t sentDataBytes[sizeof dataBytes];
            HAPRawBufferCopyBytes(sentDataBytes, dataBytes, sizeof dataBytes);
            HAPDataStreamSendMutableData(
                    &accessoryServer,
                    HAPNonnull(test.dataStream),
                    sentDataBytes,
                    sizeof sentDataBytes,
                    DataStreamTestHelpersHandleDataStreamDataComplete);
            err = HAPPlatformTCPStreamClientRead(
                    &dataStreamTCPStreamManager, clientTCPStream, &frameBytes[o], sizeof frameBytes - o, &n);
            HAPAssert(!err);

            // Verify transmission completed.
            HAPAssert(!test.expectsCompletion);
            HAPAssert(!test.error);
            HAPAssert(test.dataBytes == sentDataBytes);
            HAPAssert(test.numDataBytes == sizeof sentDataBytes);
            HAPAssert(test.isComplete);
        }
        {
            // Decrypt.
            uint8_t nonce[] = { HAPExpandLittleUInt64(encryptionContext.accessoryToControllerCount) };
            int e = HAP_chacha20_poly1305_decrypt_aad(
                    &frameBytes[sizeof frameBytes - CHACHA20_POLY1305_TAG_BYTES],
                    &frameBytes[sizeof aadBytes],
                    &frameBytes[sizeof aadBytes],
                    sizeof frameBytes - sizeof aadBytes - CHACHA20_POLY1305_TAG_BYTES,
                    &frameBytes[0],
                    sizeof aadBytes,
                    nonce,
                    sizeof nonce,
                    encryptionContext.accessoryToControllerKey.bytes);
            HAPAssert(!e);
            encryptionContext.accessoryToControllerCount++;

            // Verify frame.
            HAPAssert(HAPRawBufferAreEqual(&frameBytes[0], aadBytes, sizeof aadBytes));
            HAPAssert(HAPRawBufferAreEqual(&frameBytes[sizeof aadBytes], dataBytes, sizeof dataBytes));
        }
    }

    // Stop accessory server.
    HAPLog(&kHAPLog_Default, "Stopping accessory server.");
    HAPAccessoryServerForceStop(&accessoryServer);
    HAPPlatformClockAdvance(0);
    HAPPlatformClockAdvance(0);
    HAPAssert(HAPAccessoryServerGetState(&accessoryServer) == kHAPAccessoryServerState_Idle);

    // Check that HomeKit Data Stream has been invalidated.
    HAPAssert(!test.dataStream);

    // Check that TCP stream has been closed.
    {
        uint8_t bytes[1024];
        size_t numBytes;
        err = HAPPlatformTCPStreamClientRead(
                &dataStreamTCPStreamManager, clientTCPStream, bytes, sizeof bytes, &numBytes);
        HAPAssert(!err);
        HAPAssert(!numBytes);
        HAPPlatformTCPStreamManagerClientClose(&dataStreamTCPStreamManager, clientTCPStream);
    }

    // Ensure TCP stream manager was properly cleaned up.
    HAPAssert(!HAPPlatformTCPStreamManagerIsListenerOpen(&dataStreamTCPStreamManager));
}
