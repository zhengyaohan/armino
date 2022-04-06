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

#include "Harness/DataStreamTestHelpers.h"
#include "Harness/HAPAccessoryServerHelper.h"
#include "Harness/HAPPlatformClockHelper.h"
#include "Harness/IPTestHelpers.h"
#include "Harness/SecurityTestHelpers.h"

#include "HAPDataStream+HAP.h"

HAP_UNUSED
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

// TODO: Edge cases that would be great to test:
//   - connection timeout
//   - session identifiers are always unique
//   - session identifiers get recycled reasonably well
//   - invalidation during handling event
//   - invalidation while not handling event
//   - invalidation for receiving HDS frame type 0x01 (encrypted)
//   - handling multiple active data streams (eg, interrupts)

/**
 * Check whether a session identifier is flagged as a Request-To-Send
 */
static bool CheckInterrupt(HAPAccessoryServer* _Nonnull server, HAPDataStreamHAPSessionIdentifier sessionIdentifier) {
    uint8_t rtsSessionIdentifiers[256];
    size_t sessionIdentifierCount = 0;
    HAPDataStreamInterruptSequenceNumber interruptSequenceNumber;
    HAPError err = HAPDataStreamHAPTransportGetInterruptState(
            server,
            rtsSessionIdentifiers,
            sizeof rtsSessionIdentifiers,
            &sessionIdentifierCount,
            &interruptSequenceNumber);
    HAPAssert(!err);
    if (!sessionIdentifierCount) {
        return false;
    }

    // For now, we only allow one entry and it must be the one specified.
    // In future, as we expand this test, we probably want this to be more flexible.
    HAPAssert(sessionIdentifierCount == 1);
    return (rtsSessionIdentifiers[0] == sessionIdentifier);
}

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
    HAPDataStreamRef dataStreams[kApp_NumDataStreams];

    // TODO: prepare the HAP characteristics?

    // Initialize accessory server.
    HAPAssert(HAPGetCompatibilityVersion() == HAP_COMPATIBILITY_VERSION);
    HAPAccessoryServer accessoryServer;
    HAPAccessoryServerCreate(
            &accessoryServer,
            &(const HAPAccessoryServerOptions) {
                    .maxPairings = kHAPPairingStorage_MinElements,
                    .dataStream = { .dataStreams = dataStreams, .numDataStreams = HAPArrayCount(dataStreams) },
                    .ip = { .transport = &kHAPAccessoryServerTransport_IP,
                            .accessoryServerStorage = &ipAccessoryServerStorage } },
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
    HAPAssert(!supportsTCP);
    HAPAssert(supportsHAP);

    // Setup HomeKit Data Stream Transport.
    err = DataStreamTestHelpersWriteSetupDataStreamTransport(&accessoryServer, &accessory, &session, NULL, &test);
    HAPAssert(!err);
    HAPDataStreamHAPSessionIdentifier sessionIdentifier;
    uint8_t setupStatus;
    HAPNetworkPort listeningPort;
    HAPDataStreamSalt accessoryKeySalt;
    DataStreamTestHelpersReadSetupDataStreamTransport(
            &accessoryServer,
            &accessory,
            &session,
            &setupStatus,
            &listeningPort,
            &accessoryKeySalt,
            &sessionIdentifier,
            &test);
    HAPAssert(setupStatus == 0);
    HAPAssert(sessionIdentifier != kHAPDataStreamHAPSessionIdentifierNone);
    HAPAssert(listeningPort == kHAPNetworkPort_Any);
    HAPAssert(HAPRawBufferIsZero(accessoryKeySalt.bytes, sizeof(accessoryKeySalt.bytes)));

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
            test.expectsData = true;
            test.nextDataBytes = NULL;
            test.numNextDataBytes = receiveFrameSize;

            err = DataStreamTestHelpersBuildControllerHAPTransportWrite(
                    &accessoryServer, &accessory, &session, sessionIdentifier, frame, sizeof frame, false, &test);
            HAPAssert(!err);
        }
        {
            HAPAssert(test.dataStream);
            HAPAssert(test.session == &session);

            HAPAssert(!test.expectsData);
            HAPAssert(!test.expectsCompletion);
            HAPAssert(!test.error);
            HAPAssert(test.dataBytes == NULL);
            HAPAssert(test.numDataBytes == receiveFrameSize);
            HAPAssert(test.isComplete);
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
                    &accessoryServer, HAPNonnull(test.dataStream), sentDataBytes, sizeof sentDataBytes, &test);

            size_t sentLength;
            bool requestToSend;
            uint8_t numResponseTLVs;
            err = DataStreamTestHelpersPerformControllerHAPTransportRead(
                    &accessoryServer,
                    &accessory,
                    &session,
                    frameBytes,
                    sizeof frameBytes,
                    &sentLength,
                    &requestToSend,
                    &numResponseTLVs,
                    &test);
            HAPAssert(!err);
            HAPAssert(sentLength == sizeof frameBytes);
            HAPAssert(!requestToSend);

            // Verify transmission completed.
            HAPAssert(!test.expectsCompletion);
            HAPAssert(!test.error);
            HAPAssert(test.dataBytes == sentDataBytes);
            HAPAssert(test.numDataBytes == sizeof sentDataBytes);
            HAPAssert(test.isComplete);
        }
        {
            // Verify frame.
            HAPAssert(HAPRawBufferAreEqual(&frameBytes[0], aadBytes, sizeof aadBytes));
            HAPAssert(HAPRawBufferAreEqual(&frameBytes[sizeof aadBytes], dataBytes, sizeof dataBytes));
        }
    }

    // Accept frame sent over HomeKit Data Stream (Controller -> Accessory).
    HAPLog(&kHAPLog_Default, "Accepting frame received over HomeKit Data Stream (Controller -> Accessory)");
    {
        const uint8_t dataBytes[] = "hello, world";

        size_t const receiveFrameSize = HAP_DATASTREAM_PAYLOAD_HEADER_LENGTH + sizeof dataBytes;
        uint8_t frame[HAP_DATASTREAM_FRAME_HEADER_LENGTH + receiveFrameSize];

        uint8_t receivedDataBytes[receiveFrameSize];
        {
            DataStreamTestHelpersDataStreamEncryptControllerPacketWithContext(
                    NULL, frame, sizeof(frame), dataBytes, sizeof(dataBytes), dataBytes, 0);

            // Accept next frame.
            test.expectsData = true;
            test.nextDataBytes = receivedDataBytes;
            test.numNextDataBytes = sizeof receivedDataBytes;

            // Send frame.
            err = DataStreamTestHelpersBuildControllerHAPTransportWrite(
                    &accessoryServer, &accessory, &session, sessionIdentifier, frame, sizeof frame, false, &test);
            HAPAssert(!err);
        }
        {
            HAPAssert(!test.expectsData);
            HAPAssert(!test.expectsCompletion);
            HAPAssert(!test.error);
            HAPAssert(test.dataBytes == receivedDataBytes);
            HAPAssert(test.numDataBytes == sizeof receivedDataBytes);
            HAPAssert(test.isComplete);

            // Verify payload.
            const uint8_t* innerPayload = frame + HAP_DATASTREAM_FRAME_HEADER_LENGTH;
            HAPAssert(HAPRawBufferAreEqual(innerPayload, receivedDataBytes, sizeof dataBytes));

            // Verify the interrupt is not flagging anything.
            HAPAssert(!CheckInterrupt(&accessoryServer, sessionIdentifier));
        }
    }
    {
        // Send frame over HomeKit Data Stream (Accessory -> Controller).
        HAPLog(&kHAPLog_Default, "Sending frame over HomeKit Data Stream (Accessory -> Controller).");
        const uint8_t dataBytes[] = "hello, world";
        const uint8_t aadBytes[] = { kHAPDataStreamFrameType_UnencryptedMessage, HAPExpandBigUInt24(sizeof dataBytes) };
        uint8_t frameBytes[HAP_DATASTREAM_FRAME_HEADER_LENGTH + sizeof dataBytes];
        {
            // Prepare sending data.
            //   - make a copy (technically the SendMutableData is allowed to modify this buffer)
            uint8_t sentDataBytes[sizeof dataBytes];
            HAPRawBufferCopyBytes(sentDataBytes, dataBytes, sizeof dataBytes);

            DataStreamTestHelpersPrepareAndSendMutableData(
                    &accessoryServer, HAPNonnull(test.dataStream), sentDataBytes, sizeof sentDataBytes, &test);

            size_t sentLength;
            bool requestToSend;
            uint8_t numResponseTLVs;
            err = DataStreamTestHelpersPerformControllerHAPTransportRead(
                    &accessoryServer,
                    &accessory,
                    &session,
                    frameBytes,
                    sizeof frameBytes,
                    &sentLength,
                    &requestToSend,
                    &numResponseTLVs,
                    &test);
            HAPAssert(!err);
            HAPLog(&kHAPLog_Default, "%zu", sentLength);
            HAPAssert(sentLength == sizeof frameBytes);
            HAPAssert(!requestToSend);

            // Verify transmission completed.
            HAPAssert(!test.expectsCompletion);
            HAPAssert(!test.error);
            HAPAssert(test.dataBytes == sentDataBytes);
            HAPAssert(test.numDataBytes == sizeof sentDataBytes);
            HAPAssert(test.isComplete);
        }
        {
            // Verify frame.
            HAPAssert(HAPRawBufferAreEqual(&frameBytes[0], aadBytes, sizeof aadBytes));
            HAPAssert(HAPRawBufferAreEqual(&frameBytes[sizeof aadBytes], dataBytes, sizeof dataBytes));

            // Verify the interrupt is not flagging anything.
            HAPAssert(!CheckInterrupt(&accessoryServer, sessionIdentifier));
        }
    }

    // Accept frame sent over HomeKit Data Stream (Controller -> Accessory).
    HAPLog(&kHAPLog_Default,
           "Accepting frame received over HomeKit Data Stream, sent 1 byte at a time (Controller -> Accessory)");
    {
        const uint8_t dataBytes[] = "hello, world";

        size_t const receiveFrameSize = HAP_DATASTREAM_PAYLOAD_HEADER_LENGTH + sizeof dataBytes;
        uint8_t frame[HAP_DATASTREAM_FRAME_HEADER_LENGTH + receiveFrameSize];

        uint8_t receivedDataBytes[receiveFrameSize];
        {
            DataStreamTestHelpersDataStreamEncryptControllerPacketWithContext(
                    NULL, frame, sizeof(frame), dataBytes, sizeof(dataBytes), dataBytes, 0);

            // Send frame -- one byte at a time.
            for (size_t i = 0; i < sizeof frame; ++i) {
                // Accept next frame.
                test.expectsData = true;
                test.nextDataBytes = receivedDataBytes;
                test.numNextDataBytes = sizeof receivedDataBytes;

                err = DataStreamTestHelpersBuildControllerHAPTransportWrite(
                        &accessoryServer, &accessory, &session, sessionIdentifier, frame, sizeof frame, false, &test);
                HAPAssert(!err);

                // Do the read (but expect nothing).
                size_t sentLength;
                bool requestToSend;
                uint8_t numResponseTLVs;
                err = DataStreamTestHelpersPerformControllerHAPTransportRead(
                        &accessoryServer,
                        &accessory,
                        &session,
                        NULL,
                        0,
                        &sentLength,
                        &requestToSend,
                        &numResponseTLVs,
                        &test);
                HAPAssert(!err);
                HAPAssert(sentLength == 0);
                HAPAssert(!requestToSend);

                // Verify the interrupt is not flagging anything.
                HAPAssert(!CheckInterrupt(&accessoryServer, sessionIdentifier));
            }
        }
        {
            HAPAssert(!test.expectsData);
            HAPAssert(!test.expectsCompletion);
            HAPAssert(!test.error);
            HAPAssert(test.dataBytes == receivedDataBytes);
            HAPAssert(test.numDataBytes == sizeof receivedDataBytes);
            HAPAssert(test.isComplete);

            // Verify payload.
            const uint8_t* innerPayload = frame + HAP_DATASTREAM_FRAME_HEADER_LENGTH;
            HAPAssert(HAPRawBufferAreEqual(innerPayload, receivedDataBytes, sizeof dataBytes));

            // Verify the interrupt is not flagging anything.
            HAPAssert(!CheckInterrupt(&accessoryServer, sessionIdentifier));
        }
    }

    // Send frame over HomeKit Data Stream (Accessory -> Controller).
    HAPLog(&kHAPLog_Default,
           "Sending frame over HomeKit Data Stream, sent 1 byte at a time (Accessory -> Controller).");
    {
        const uint8_t dataBytes[] = "hello, world";
        const uint8_t aadBytes[] = { kHAPDataStreamFrameType_UnencryptedMessage, HAPExpandBigUInt24(sizeof dataBytes) };
        uint8_t frameBytes[HAP_DATASTREAM_FRAME_HEADER_LENGTH + sizeof dataBytes];
        {
            // Prepare sending data.
            //   - make a copy (technically the SendMutableData is allowed to modify this buffer)
            uint8_t sentDataBytes[sizeof dataBytes];
            HAPRawBufferCopyBytes(sentDataBytes, dataBytes, sizeof dataBytes);

            HAPDataStreamPrepareData(
                    &accessoryServer,
                    test.dataStream,
                    sizeof sentDataBytes,
                    DataStreamTestHelpersHandleDataStreamSendDataComplete);

            // Here is a transition from "nothing to send" to "need to send", therefore
            // the interrupt should be flagged!
            HAPAssert(true == CheckInterrupt(&accessoryServer, sessionIdentifier));

            // Send frame -- one byte at a time.
            size_t receivedIndex = 0;
            for (size_t i = 0; i < sizeof sentDataBytes; ++i) {
                // Do a dummy write to pull the data.
                test.expectsData = false;
                test.nextDataBytes = NULL;
                test.numNextDataBytes = 0;

                err = DataStreamTestHelpersBuildControllerHAPTransportWrite(
                        &accessoryServer, &accessory, &session, sessionIdentifier, NULL, 0, false, &test);
                HAPAssert(!err);

                // Then do the Read.
                test.expectsCompletion = true;
                test.dataBytesToSend = sentDataBytes + i;
                test.numDataBytesToSend = 1;
                // The initial "Prepare" triggered a callback that enqueued the first byte. After that, we have to
                // manually pump it.
                if (i > 0) {
                    HAPDataStreamSendMutableData(
                            &accessoryServer,
                            test.dataStream,
                            test.dataBytesToSend,
                            test.numDataBytesToSend,
                            DataStreamTestHelpersHandleDataStreamDataComplete);
                }

                size_t sentLength;
                bool requestToSend;
                uint8_t numResponseTLVs;
                err = DataStreamTestHelpersPerformControllerHAPTransportRead(
                        &accessoryServer,
                        &accessory,
                        &session,
                        frameBytes + receivedIndex,
                        sizeof frameBytes - receivedIndex,
                        &sentLength,
                        &requestToSend,
                        &numResponseTLVs,
                        &test);
                HAPAssert(!err);
                HAPLog(&kHAPLog_Default, "%zu", sentLength);
                if (i == 0) {
                    // First frame has the header and then the one byte.
                    HAPAssert(sentLength == 5);
                    HAPAssert(requestToSend);
                    HAPAssert(!test.isComplete);
                } else if (i < sizeof sentDataBytes - 1) {
                    // Middle bytes.
                    HAPAssert(sentLength == 1);
                    HAPAssert(requestToSend);
                    HAPAssert(!test.isComplete);
                } else {
                    // Final byte.
                    HAPAssert(sentLength == 1);
                    HAPAssert(!requestToSend);
                    HAPAssert(test.isComplete);
                }
                receivedIndex += sentLength;

                // Verify transmission completed.
                HAPAssert(!test.expectsCompletion);
                HAPAssert(!test.error);
                HAPAssert(test.dataBytes == sentDataBytes + i);
                HAPAssert(test.numDataBytes == 1);

                // Verify the interrupt is not flagging anything.
                // In all cases, the RTS flag was communicated to controller so there is nothing
                // to do.
                HAPAssert(!CheckInterrupt(&accessoryServer, sessionIdentifier));
            }
        }
        {
            // Verify frame.
            HAPAssert(HAPRawBufferAreEqual(&frameBytes[0], aadBytes, sizeof aadBytes));
            HAPAssert(HAPRawBufferAreEqual(&frameBytes[sizeof aadBytes], dataBytes, sizeof dataBytes));
        }
    }

    // Close the HomeKit Data Stream.
    HAPLog(&kHAPLog_Default, "Sending force close (Controller -> Accessory)");
    {
        err = DataStreamTestHelpersBuildControllerHAPTransportWrite(
                &accessoryServer, &accessory, &session, sessionIdentifier, NULL, 0, true, &test);
        HAPAssert(!err);
    }
    {
        // Do the read (expect empty response).
        size_t sentLength;
        bool requestToSend;
        uint8_t numResponseTLVs;
        err = DataStreamTestHelpersPerformControllerHAPTransportRead(
                &accessoryServer, &accessory, &session, NULL, 0, &sentLength, &requestToSend, &numResponseTLVs, &test);
        HAPAssert(!err);
        HAPAssert(numResponseTLVs == 0);

        // Verify test context was cleared by handleInvalidate callback.
        HAPAssert(HAPRawBufferIsZero(&test, sizeof test));
    }

    // Stop accessory server.
    HAPLog(&kHAPLog_Default, "Stopping accessory server.");
    HAPAccessoryServerForceStop(&accessoryServer);
    HAPPlatformClockAdvance(0);
    HAPPlatformClockAdvance(0);
    HAPAssert(HAPAccessoryServerGetState(&accessoryServer) == kHAPAccessoryServerState_Idle);
}
