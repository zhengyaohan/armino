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

static HAPTargetControlDataStreamProtocol targetControlDataStreamProtocol = {
    .base = &kHAPTargetControlDataStreamProtocol_Base,
    .callbacks = { .handleIdentifierUpdate = DataStreamTestHelpersHandleTargetControlIdentifierUpdate }
};

static HAPDataSendDataStreamProtocolContext dataSendDataStreamProtocolContexts[kApp_NumDataStreams];
static HAPDataSendDataStreamProtocolListener dataSendDataStreamProtocolListeners[kApp_NumDataStreams];
static HAPDataSendStreamProtocolStreamAvailableCallbacks dataSendDataStreamProtocolAvailableCallbacks[] = {
    { .type = kHAPDataSendDataStreamProtocolType_IPCamera_Recording,
      .handleStreamAvailable = DataStreamTestHelpersHandleDataSendStreamAvailable }
};
static HAPDataSendDataStreamProtocol dataSendDataStreamProtocol = {
    .base = &kHAPDataSendDataStreamProtocol_Base,
    .storage = { .numDataStreams = kApp_NumDataStreams,
                 .protocolContexts = dataSendDataStreamProtocolContexts,
                 .listeners = dataSendDataStreamProtocolListeners },
    .callbacks = { .handleAccept = DataStreamTestHelpersHandleDataSendAccept,
                   .handleInvalidate = DataStreamTestHelpersHandleDataSendInvalidate,
                   .numStreamAvailableCallbacks = 1,
                   .streamAvailableCallbacks = dataSendDataStreamProtocolAvailableCallbacks }
};

static const HAPDataSendDataStreamProtocolStreamCallbacks dataSendStreamCallbacks = {
    .handleClose = DataStreamTestHelpersHandleDataSendStreamClose,
    .handleOpen = DataStreamTestHelpersHandleDataSendStreamOpen
};

static HAPDataStreamDescriptor dataStreamDescriptors[kApp_NumDataStreams];
static HAPDataStreamDispatcherStorage dataStreamDispatcherStorage = {
    .numDataStreams = kApp_NumDataStreams,
    .dataStreamDescriptors = dataStreamDescriptors,
    .dataStreamProtocols =
            (HAPDataStreamProtocol* const[]) { &targetControlDataStreamProtocol, &dataSendDataStreamProtocol, NULL }
};
static HAPDataStreamDispatcher dataStreamDispatcher;

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

    DataSendTestContext test;
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
                                                          .numBufferBytes = 8 });
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

    // Initialize HomeKit Data Stream dispatcher.
    HAPDataStreamDispatcherCreate(
            &accessoryServer,
            &dataStreamDispatcher,
            &(const HAPDataStreamDispatcherOptions) { .storage = &dataStreamDispatcherStorage });

    // Start accessory server.
    accessory.dataStream.delegate.callbacks = &kHAPDataStreamDispatcher_DataStreamCallbacks;
    accessory.dataStream.delegate.context = &dataStreamDispatcher;
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
    uint8_t setupStatus;
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

    // Send control.hello request.
    DataStreamTestHelpersSendHelloRequest(&dataStreamTCPStreamManager, &clientTCPStream, &encryptionContext);
    // Receive response for control.hello request.
    DataStreamTestHelpersReceiveHelloResponse(&dataStreamTCPStreamManager, &clientTCPStream, &encryptionContext);

    HAPAssert(test.dataSendAccepted);
    // Send targetControl.whoami event.
    HAPLog(&kHAPLog_Default, "Sending targetControl.whoami event.");
    HAPTargetControlDataStreamProtocolTargetIdentifier targetIdentifier;
    HAPPlatformRandomNumberFill(&targetIdentifier, sizeof targetIdentifier);
    {
        // clang-format off
        const uint8_t header[] = {
                0xE2,
                0x48, 'p', 'r', 'o',  't', 'o', 'c', 'o', 'l',
                0x4D, 't', 'a', 'r',  'g',  'e', 't', 'C',  'o', 'n', 't', 'r', 'o', 'l',
                0x45, 'e', 'v',  'e',  'n', 't',
                0x46, 'w', 'h', 'o', 'a', 'm', 'i'
        };
        const uint8_t message[] = {
                0xE1,
                0x4A, 'i', 'd', 'e', 'n',  't', 'i',  'f',  'i', 'e', 'r',
                0x33, HAPExpandLittleInt64((int64_t) targetIdentifier)
        };
        // clang-format on
        {
            uint8_t frame[HAP_DATASTREAM_ENC_FRAME_MIN_LENGTH + sizeof header + sizeof message];
            DataStreamTestHelpersDataStreamEncryptControllerPacketWithContext(
                    &encryptionContext, frame, sizeof(frame), header, sizeof(header), message, sizeof(message));

            size_t numBytes;
            err = HAPPlatformTCPStreamClientWrite(
                    &dataStreamTCPStreamManager, clientTCPStream, frame, sizeof frame, &numBytes);
            HAPAssert(!err);
            HAPAssert(numBytes == sizeof frame);
        }
    }
    HAPAssert(test.targetIdentifier == targetIdentifier);

    // Open dataSend stream.
    HAPLog(&kHAPLog_Default, "Opening dataSend stream.");
    test.dataSendStreamValid = true;
    HAPDataSendDataStreamProtocolStream dataSendStream;
    {
        HAPDataSendDataStreamProtocolOpen(
                &accessoryServer,
                HAPNonnullVoid(accessory.dataStream.delegate.context),
                test.dataStream,
                &dataSendStream,
                kHAPDataSendDataStreamProtocolType_Audio_Siri,
                &dataSendStreamCallbacks);
    }
    int64_t streamID;
    {
        HAPLog(&kHAPLog_Default, "Receiving dataSend.open request.");
        int64_t requestID;
        {
            uint8_t frameBytes[1024];
            size_t numBytes;
            err = HAPPlatformTCPStreamClientRead(
                    &dataStreamTCPStreamManager, clientTCPStream, frameBytes, sizeof frameBytes, &numBytes);
            HAPAssert(!err);

            uint8_t* header;
            uint8_t* payload;
            size_t headerSize;
            size_t payloadSize;
            DataStreamTestHelpersDataStreamDecryptAccessoryPacketWithContext(
                    &encryptionContext, frameBytes, numBytes, &header, &headerSize, &payload, &payloadSize);

            // Header.
            {
                HAPOPACKReader headerReader;
                HAPOPACKReaderCreate(&headerReader, header, headerSize);
                HAPOPACKStringDictionaryElement protocolElement, requestElement, idElement;
                protocolElement.key = "protocol";
                requestElement.key = "request";
                idElement.key = "id";
                err = HAPOPACKStringDictionaryReaderGetAll(
                        &headerReader,
                        (HAPOPACKStringDictionaryElement* const[]) {
                                &protocolElement, &requestElement, &idElement, NULL });
                HAPAssert(!err);

                // Protocol name.
                HAPAssert(protocolElement.value.exists);
                char* protocolName;
                err = HAPOPACKReaderGetNextString(&protocolElement.value.reader, &protocolName);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(protocolName, "dataSend"));

                // Topic.
                HAPAssert(requestElement.value.exists);
                char* topic;
                err = HAPOPACKReaderGetNextString(&requestElement.value.reader, &topic);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(topic, "open"));

                // Request ID.
                HAPAssert(idElement.value.exists);
                err = HAPOPACKReaderGetNextInt(&idElement.value.reader, &requestID);
                HAPAssert(!err);
            }

            // Message.
            {
                HAPOPACKReader messageReader;
                HAPOPACKReaderCreate(&messageReader, payload, payloadSize);
                HAPOPACKStringDictionaryElement targetElement, typeElement;
                targetElement.key = "target";
                typeElement.key = "type";
                err = HAPOPACKStringDictionaryReaderGetAll(
                        &messageReader,
                        (HAPOPACKStringDictionaryElement* const[]) { &targetElement, &typeElement, NULL });
                HAPAssert(!err);

                // Parse target.
                HAPAssert(targetElement.value.exists);
                char* target;
                err = HAPOPACKReaderGetNextString(&targetElement.value.reader, &target);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(target, "controller"));

                // Parse type.
                HAPAssert(typeElement.value.exists);
                char* type;
                err = HAPOPACKReaderGetNextString(&typeElement.value.reader, &type);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(type, "audio.siri"));
            }
        }
        HAPPlatformClockAdvance(1 * HAPSecond);
        HAPLog(&kHAPLog_Default, "Sending dataSend.open response.");
        HAPPlatformRandomNumberFill(&streamID, sizeof streamID);
        {
            // clang-format off
            const uint8_t header[] = {
                    0xE4,
                    0x48, 'p',  'r', 'o', 't', 'o',  'c', 'o',  'l',
                    0x48, 'd', 'a',  't', 'a',  'S', 'e',  'n', 'd',
                    0x48, 'r',  'e', 's',  'p', 'o',  'n', 's',  'e',
                    0x44, 'o', 'p',  'e', 'n',
                    0x42, 'i',  'd',
                    0x33, HAPExpandLittleInt64(requestID),
                    0x46, 's', 't',  'a', 't',  'u', 's',
                    0x08
            };
            const uint8_t message[] = {
                    0xE1,
                    0x48, 's', 't', 'r',  'e', 'a',  'm',  'I', 'd',
                    0x33, HAPExpandLittleInt64(streamID)
            };
            // clang-format on
            {
                uint8_t frame[HAP_DATASTREAM_ENC_FRAME_MIN_LENGTH + sizeof header + sizeof message];
                DataStreamTestHelpersDataStreamEncryptControllerPacketWithContext(
                        &encryptionContext, frame, sizeof(frame), header, sizeof(header), message, sizeof(message));

                size_t numBytes;
                err = HAPPlatformTCPStreamClientWrite(
                        &dataStreamTCPStreamManager, clientTCPStream, frame, sizeof frame, &numBytes);
                HAPAssert(!err);
                HAPAssert(numBytes == sizeof frame);
            }
        }
    }
    HAPAssert(test.dataSendStreamOpen);

    // Send packets over dataSend stream.
    size_t totalPackets = 10;
    size_t nextSequenceNumber = 0;
    bool endOfStreamSet = false;
    for (size_t i = 0; i < totalPackets; i++) {
        HAPLog(&kHAPLog_Default, "Sending packets over dataSend stream (%zu).", i);
        uint8_t scratchBytes[1024];
        test.expectingSendCompletion = true;
        test.scratchBytes = scratchBytes;
        test.numScratchBytes = sizeof scratchBytes;
        {
            uint8_t packetBytes[] = { HAPExpandLittleUInt64((uint64_t) i * 1),
                                      HAPExpandLittleUInt64((uint64_t) i * 2),
                                      HAPExpandLittleUInt64((uint64_t) i * 4),
                                      HAPExpandLittleUInt64((uint64_t) i * 8) };
            HAPDataSendDataStreamProtocolPacket packets[] = {
                { .data = { .bytes = packetBytes, .numBytes = sizeof packetBytes },
                  .metadata = { .type = kHAPDataSendDataStreamProtocolType_Audio_Siri,
                                ._.audio.siri = { .rms = 0.01F, .sequenceNumber = (int64_t)(2 * i) } } },
                { .data = { .bytes = packetBytes, .numBytes = sizeof packetBytes / 2 },
                  .metadata = { .type = kHAPDataSendDataStreamProtocolType_Audio_Siri,
                                ._.audio.siri = { .rms = 0.01F, .sequenceNumber = (int64_t)(2 * i + 1) } } }
            };
            err = HAPDataSendDataStreamProtocolSendData(
                    &accessoryServer,
                    HAPNonnullVoid(accessory.dataStream.delegate.context),
                    test.dataStream,
                    &dataSendStream,
                    scratchBytes,
                    sizeof scratchBytes,
                    packets,
                    HAPArrayCount(packets),
                    i == totalPackets - 1,
                    DataStreamTestHelpersHandleSendDataComplete);
            HAPAssert(!err);
        }
        HAPLog(&kHAPLog_Default, "Receiving dataSend.data event.");
        {
            HAPAssert(!endOfStreamSet);

            uint8_t frameBytes[1024];
            size_t numBytes;
            err = HAPPlatformTCPStreamClientRead(
                    &dataStreamTCPStreamManager, clientTCPStream, frameBytes, sizeof frameBytes, &numBytes);
            HAPAssert(!err);

            uint8_t* header;
            uint8_t* payload;
            size_t headerSize;
            size_t payloadSize;
            DataStreamTestHelpersDataStreamDecryptAccessoryPacketWithContext(
                    &encryptionContext, frameBytes, numBytes, &header, &headerSize, &payload, &payloadSize);

            // Header.
            {
                HAPOPACKReader headerReader;
                HAPOPACKReaderCreate(&headerReader, header, headerSize);
                HAPOPACKStringDictionaryElement protocolElement, eventElement;
                protocolElement.key = "protocol";
                eventElement.key = "event";
                err = HAPOPACKStringDictionaryReaderGetAll(
                        &headerReader,
                        (HAPOPACKStringDictionaryElement* const[]) { &protocolElement, &eventElement, NULL });
                HAPAssert(!err);

                // Protocol name.
                HAPAssert(protocolElement.value.exists);
                char* protocolName;
                err = HAPOPACKReaderGetNextString(&protocolElement.value.reader, &protocolName);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(protocolName, "dataSend"));

                // Topic.
                HAPAssert(eventElement.value.exists);
                char* topic;
                err = HAPOPACKReaderGetNextString(&eventElement.value.reader, &topic);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(topic, "data"));
            }

            // Message.
            {
                HAPOPACKReader messageReader;
                HAPOPACKReaderCreate(&messageReader, payload, payloadSize);
                HAPOPACKStringDictionaryElement streamIdElement, packetsElement, endOfStreamElement;
                streamIdElement.key = "streamId";
                packetsElement.key = "packets";
                endOfStreamElement.key = "endOfStream";
                err = HAPOPACKStringDictionaryReaderGetAll(
                        &messageReader,
                        (HAPOPACKStringDictionaryElement* const[]) {
                                &streamIdElement, &packetsElement, &endOfStreamElement, NULL });
                HAPAssert(!err);

                // Parse streamId.
                HAPAssert(streamIdElement.value.exists);
                int64_t streamId;
                err = HAPOPACKReaderGetNextInt(&streamIdElement.value.reader, &streamId);
                HAPAssert(!err);
                HAPAssert(streamId == streamID);

                // Parse packets.
                HAPAssert(packetsElement.value.exists);
                HAPOPACKReader packetsReader;
                err = HAPOPACKReaderGetNextArray(&packetsElement.value.reader, &packetsReader);
                HAPAssert(!err);

                for (;;) {
                    bool found;
                    err = HAPOPACKReaderPeekNextType(&packetsReader, &found, NULL);
                    HAPAssert(!err);
                    if (!found) {
                        break;
                    }
                    HAPOPACKReader packetReader;
                    err = HAPOPACKReaderGetNext(&packetsReader, &packetReader);
                    HAPAssert(!err);

                    HAPOPACKStringDictionaryElement dataElement, metadataElement;
                    dataElement.key = "data";
                    metadataElement.key = "metadata";
                    err = HAPOPACKStringDictionaryReaderGetAll(
                            &packetReader,
                            (HAPOPACKStringDictionaryElement* const[]) { &dataElement, &metadataElement, NULL });
                    HAPAssert(!err);

                    // Parse data.
                    HAPAssert(dataElement.value.exists);
                    void* dataBytes;
                    size_t numDataBytes;
                    err = HAPOPACKReaderGetNextData(&dataElement.value.reader, &dataBytes, &numDataBytes);
                    HAPAssert(!err);
                    if (nextSequenceNumber % 2 == 0) {
                        HAPAssert(numDataBytes == 4 * sizeof(uint64_t));
                        HAPAssert(HAPReadLittleUInt64(&((uint8_t*) dataBytes)[0x00]) == i * 1);
                        HAPAssert(HAPReadLittleUInt64(&((uint8_t*) dataBytes)[0x08]) == i * 2);
                        HAPAssert(HAPReadLittleUInt64(&((uint8_t*) dataBytes)[0x10]) == i * 4);
                        HAPAssert(HAPReadLittleUInt64(&((uint8_t*) dataBytes)[0x18]) == i * 8);
                    } else {
                        HAPAssert(numDataBytes == 2 * sizeof(uint64_t));
                        HAPAssert(HAPReadLittleUInt64(&((uint8_t*) dataBytes)[0x00]) == i * 1);
                        HAPAssert(HAPReadLittleUInt64(&((uint8_t*) dataBytes)[0x08]) == i * 2);
                    }

                    // Parse metadata.
                    HAPAssert(metadataElement.value.exists);
                    HAPOPACKStringDictionaryElement rmsElement, sequenceNumberElement;
                    rmsElement.key = "rms";
                    sequenceNumberElement.key = "sequenceNumber";
                    err = HAPOPACKStringDictionaryReaderGetAll(
                            &metadataElement.value.reader,
                            (HAPOPACKStringDictionaryElement* const[]) { &rmsElement, &sequenceNumberElement, NULL });
                    HAPAssert(!err);

                    // Parse rms.
                    HAPAssert(rmsElement.value.exists);
                    double rms;
                    err = HAPOPACKReaderGetNextFloat(&rmsElement.value.reader, &rms);
                    HAPAssert(!err);
                    HAPAssert(rms >= -1 && rms <= 1);

                    // Parse sequenceNumber.
                    HAPAssert(sequenceNumberElement.value.exists);
                    int64_t sequenceNumber;
                    err = HAPOPACKReaderGetNextInt(&sequenceNumberElement.value.reader, &sequenceNumber);
                    HAPAssert(!err);
                    HAPAssert(sequenceNumber >= 0);
                    HAPAssert((size_t) sequenceNumber == nextSequenceNumber);
                    nextSequenceNumber++;
                }

                // Parse endOfStream.
                if (endOfStreamElement.value.exists) {
                    bool endOfStream;
                    err = HAPOPACKReaderGetNextBool(&endOfStreamElement.value.reader, &endOfStream);
                    HAPAssert(!err);
                    HAPAssert(!endOfStreamSet);
                    endOfStreamSet = endOfStream;
                }
            }
        }
        HAPAssert(!test.expectingSendCompletion);
    }
    HAPAssert(nextSequenceNumber == 2 * totalPackets);
    HAPAssert(endOfStreamSet);
    HAPAssert(test.dataSendStreamValid);
    HAPAssert(test.dataSendStreamOpen);

    // Finish transfer.
    HAPLog(&kHAPLog_Default, "Sending dataSend.ack event.");
    {
        // clang-format off
        const uint8_t header[] = {
                0xE2,
                0x48, 'p', 'r', 'o', 't', 'o', 'c', 'o', 'l',
                0x48, 'd', 'a', 't', 'a', 'S', 'e',  'n', 'd',
                0x45, 'e', 'v', 'e', 'n', 't',
                0x43, 'a', 'c', 'k'
        };
        const uint8_t message[] = {
                0xE2,
                0x48, 's', 't', 'r', 'e', 'a', 'm', 'I', 'd',
                0x33, HAPExpandLittleInt64(streamID),
                0x4B, 'e', 'n',  'd',  'O', 'f', 'S', 't', 'r', 'e', 'a', 'm',
                0x01
        };
        // clang-format on
        {
            uint8_t frame[HAP_DATASTREAM_ENC_FRAME_MIN_LENGTH + sizeof header + sizeof message];
            DataStreamTestHelpersDataStreamEncryptControllerPacketWithContext(
                    &encryptionContext, frame, sizeof(frame), header, sizeof(header), message, sizeof(message));

            size_t numBytes;
            err = HAPPlatformTCPStreamClientWrite(
                    &dataStreamTCPStreamManager, clientTCPStream, frame, sizeof frame, &numBytes);
            HAPAssert(!err);
            HAPAssert(numBytes == sizeof frame);
        }
    }
    HAPLog(&kHAPLog_Default, "Receiving dataSend.close event.");
    {
        uint8_t frameBytes[1024];
        size_t numBytes;
        err = HAPPlatformTCPStreamClientRead(
                &dataStreamTCPStreamManager, clientTCPStream, frameBytes, sizeof frameBytes, &numBytes);
        HAPAssert(!err);

        uint8_t* header;
        uint8_t* payload;
        size_t headerSize;
        size_t payloadSize;
        DataStreamTestHelpersDataStreamDecryptAccessoryPacketWithContext(
                &encryptionContext, frameBytes, numBytes, &header, &headerSize, &payload, &payloadSize);

        // Header.
        {
            HAPOPACKReader headerReader;
            HAPOPACKReaderCreate(&headerReader, header, headerSize);
            HAPOPACKStringDictionaryElement protocolElement, eventElement;
            protocolElement.key = "protocol";
            eventElement.key = "event";
            err = HAPOPACKStringDictionaryReaderGetAll(
                    &headerReader,
                    (HAPOPACKStringDictionaryElement* const[]) { &protocolElement, &eventElement, NULL });
            HAPAssert(!err);

            // Protocol name.
            HAPAssert(protocolElement.value.exists);
            char* protocolName;
            err = HAPOPACKReaderGetNextString(&protocolElement.value.reader, &protocolName);
            HAPAssert(!err);
            HAPAssert(HAPStringAreEqual(protocolName, "dataSend"));

            // Topic.
            HAPAssert(eventElement.value.exists);
            char* topic;
            err = HAPOPACKReaderGetNextString(&eventElement.value.reader, &topic);
            HAPAssert(!err);
            HAPAssert(HAPStringAreEqual(topic, "close"));
        }

        // Message.
        {
            HAPOPACKReader messageReader;
            HAPOPACKReaderCreate(&messageReader, payload, payloadSize);
            HAPOPACKStringDictionaryElement streamIdElement, reasonElement;
            streamIdElement.key = "streamId";
            reasonElement.key = "reason";
            err = HAPOPACKStringDictionaryReaderGetAll(
                    &messageReader,
                    (HAPOPACKStringDictionaryElement* const[]) { &streamIdElement, &reasonElement, NULL });
            HAPAssert(!err);

            // Parse streamId.
            HAPAssert(streamIdElement.value.exists);
            int64_t streamId;
            err = HAPOPACKReaderGetNextInt(&streamIdElement.value.reader, &streamId);
            HAPAssert(!err);
            HAPAssert(streamId == streamID);

            // Parse reason.
            if (reasonElement.value.exists) {
                int64_t reason;
                err = HAPOPACKReaderGetNextInt(&reasonElement.value.reader, &reason);
                HAPAssert(!err);
                HAPAssert(reason == 0);
            }
        }
    }
    HAPAssert(!test.expectingSendCompletion);
    HAPAssert(!test.dataSendStreamValid);
    HAPAssert(!test.dataSendStreamOpen);
    HAPAssert(!test.closeReason);

    // Open dataSend stream (controller > accessory).
    HAPLog(&kHAPLog_Default, "Sending dataSend.open request.");
    HAPAssert(!test.dataSendStreamAvailable);
    HAPPlatformRandomNumberFill(&streamID, sizeof streamID);
    int64_t requestID;
    HAPPlatformRandomNumberFill(&requestID, sizeof requestID);
    {
        // clang-format off
        const uint8_t header[] = {
                0xE3,
                0x48, 'p', 'r', 'o', 't', 'o', 'c', 'o', 'l',
                0x48, 'd', 'a', 't', 'a', 'S', 'e', 'n', 'd',
                0x47, 'r', 'e', 'q', 'u', 'e', 's', 't',
                0x44, 'o', 'p', 'e', 'n',
                0x42, 'i', 'd',
                0x33, HAPExpandLittleInt64(requestID)
        };
        const uint8_t message[] = {
                0xE3,
                0x46, 't', 'a', 'r', 'g', 'e', 't',
                0x4A, 'c', 'o', 'n', 't', 'r', 'o', 'l', 'l', 'e', 'r',
                0x44, 't', 'y', 'p', 'e',
                0x52, 'i', 'p', 'c', 'a', 'm', 'e', 'r', 'a', '.', 'r', 'e', 'c', 'o', 'r', 'd', 'i', 'n', 'g',
                0x48, 's', 't', 'r', 'e', 'a', 'm', 'I', 'd',
                0x33, HAPExpandLittleInt64(streamID)
        };
        // clang-format on
        {
            uint8_t frame[HAP_DATASTREAM_ENC_FRAME_MIN_LENGTH + sizeof header + sizeof message];
            DataStreamTestHelpersDataStreamEncryptControllerPacketWithContext(
                    &encryptionContext, frame, sizeof(frame), header, sizeof(header), message, sizeof(message));

            size_t numBytes;
            err = HAPPlatformTCPStreamClientWrite(
                    &dataStreamTCPStreamManager, clientTCPStream, frame, sizeof frame, &numBytes);
            HAPAssert(!err);
            HAPAssert(numBytes == sizeof frame);
        }
    }
    HAPAssert(test.dataSendStreamAvailable);
    HAPAssert(test.availableStreamType == kHAPDataSendDataStreamProtocolType_IPCamera_Recording);

    // Reject dataSend stream.
    HAPLog(&kHAPLog_Default, "Rejecting dataSend.open request.");
    {
        test.dataSendStreamAvailable = false;
        test.availableStreamType = (HAPDataSendDataStreamProtocolType) 0;
        HAPDataSendDataStreamProtocolReject(
                &accessoryServer,
                HAPNonnullVoid(accessory.dataStream.delegate.context),
                test.dataStream,
                kHAPDataSendDataStreamProtocolRejectReason_NotAllowed);
    }
    HAPLog(&kHAPLog_Default, "Receiving dataSend.open response.");
    {
        uint8_t frameBytes[1024];
        size_t numBytes;
        err = HAPPlatformTCPStreamClientRead(
                &dataStreamTCPStreamManager, clientTCPStream, frameBytes, sizeof frameBytes, &numBytes);
        HAPAssert(!err);

        uint8_t* header;
        uint8_t* payload;
        size_t headerSize;
        size_t payloadSize;
        DataStreamTestHelpersDataStreamDecryptAccessoryPacketWithContext(
                &encryptionContext, frameBytes, numBytes, &header, &headerSize, &payload, &payloadSize);

        // Header.
        {
            HAPOPACKReader headerReader;
            HAPOPACKReaderCreate(&headerReader, header, headerSize);
            HAPOPACKStringDictionaryElement protocolElement, responseElement, idElement, statusElement;
            protocolElement.key = "protocol";
            responseElement.key = "response";
            idElement.key = "id";
            statusElement.key = "status";
            err = HAPOPACKStringDictionaryReaderGetAll(
                    &headerReader,
                    (HAPOPACKStringDictionaryElement* const[]) {
                            &protocolElement, &responseElement, &idElement, &statusElement, NULL });
            HAPAssert(!err);

            // Protocol name.
            HAPAssert(protocolElement.value.exists);
            char* protocolName;
            err = HAPOPACKReaderGetNextString(&protocolElement.value.reader, &protocolName);
            HAPAssert(!err);
            HAPAssert(HAPStringAreEqual(protocolName, "dataSend"));

            // Topic.
            HAPAssert(responseElement.value.exists);
            char* topic;
            err = HAPOPACKReaderGetNextString(&responseElement.value.reader, &topic);
            HAPAssert(!err);
            HAPAssert(HAPStringAreEqual(topic, "open"));

            // Request ID.
            HAPAssert(idElement.value.exists);
            int64_t responseRequestID;
            err = HAPOPACKReaderGetNextInt(&idElement.value.reader, &responseRequestID);
            HAPAssert(!err);
            HAPAssert(responseRequestID == requestID);

            // Response status.
            HAPAssert(statusElement.value.exists);
            int64_t status;
            err = HAPOPACKReaderGetNextInt(&statusElement.value.reader, &status);
            HAPAssert(!err);
            HAPAssert(status == 6);
        }

        // Message.
        {
            HAPOPACKReader messageReader;
            HAPOPACKReaderCreate(&messageReader, payload, payloadSize);
            HAPOPACKStringDictionaryElement statusElement;
            statusElement.key = "status";
            err = HAPOPACKStringDictionaryReaderGetAll(
                    &messageReader, (HAPOPACKStringDictionaryElement* const[]) { &statusElement, NULL });
            HAPAssert(!err);

            // Parse status.
            HAPAssert(statusElement.value.exists);
            int64_t status;
            err = HAPOPACKReaderGetNextInt(&statusElement.value.reader, &status);
            HAPAssert(!err);
            HAPAssert(status == 1);
        }
    }

    HAPAssert(!test.dataSendStreamAvailable);
    HAPAssert(!test.expectingSendCompletion);
    HAPAssert(!test.dataSendStreamValid);
    HAPAssert(!test.dataSendStreamOpen);
    HAPAssert(!test.closeReason);

    // Open dataSend stream (controller > accessory).
    HAPLog(&kHAPLog_Default, "Sending dataSend.open request.");
    HAPAssert(!test.dataSendStreamAvailable);
    HAPPlatformRandomNumberFill(&streamID, sizeof streamID);
    HAPPlatformRandomNumberFill(&requestID, sizeof requestID);
    {
        // clang-format off
        const uint8_t header[] = {
                0xE3,
                0x48, 'p', 'r', 'o', 't', 'o', 'c', 'o', 'l',
                0x48, 'd', 'a', 't', 'a', 'S', 'e', 'n', 'd',
                0x47, 'r', 'e', 'q', 'u', 'e', 's', 't',
                0x44, 'o', 'p', 'e', 'n', 0x42, 'i', 'd',
                0x33, HAPExpandLittleInt64(requestID)
        };
        const uint8_t message[] = {
                0xE3,
                0x46, 't', 'a', 'r', 'g', 'e', 't',
                0x4A, 'c', 'o', 'n', 't', 'r', 'o', 'l', 'l', 'e', 'r',
                0x44, 't', 'y', 'p', 'e',
                0x52, 'i', 'p', 'c', 'a', 'm', 'e', 'r', 'a', '.', 'r', 'e', 'c', 'o', 'r', 'd', 'i', 'n', 'g',
                0x48, 's', 't', 'r', 'e', 'a', 'm', 'I', 'd',
                0x33, HAPExpandLittleInt64(streamID)
        };
        // clang-format on
        {
            uint8_t frame[HAP_DATASTREAM_ENC_FRAME_MIN_LENGTH + sizeof header + sizeof message];
            DataStreamTestHelpersDataStreamEncryptControllerPacketWithContext(
                    &encryptionContext, frame, sizeof(frame), header, sizeof(header), message, sizeof(message));

            size_t numBytes;
            err = HAPPlatformTCPStreamClientWrite(
                    &dataStreamTCPStreamManager, clientTCPStream, frame, sizeof frame, &numBytes);
            HAPAssert(!err);
            HAPAssert(numBytes == sizeof frame);
        }
    }
    HAPAssert(test.dataSendStreamAvailable);
    HAPAssert(test.availableStreamType == kHAPDataSendDataStreamProtocolType_IPCamera_Recording);

    // Accept dataSend stream.
    HAPLog(&kHAPLog_Default, "Accepting dataSend.open request.");
    test.dataSendStreamValid = true;
    {
        test.dataSendStreamAvailable = false;
        test.availableStreamType = (HAPDataSendDataStreamProtocolType) 0;
        HAPDataSendDataStreamProtocolAccept(
                &accessoryServer,
                HAPNonnullVoid(accessory.dataStream.delegate.context),
                test.dataStream,
                &dataSendStream,
                &dataSendStreamCallbacks);
    }
    HAPLog(&kHAPLog_Default, "Receiving dataSend.open response.");
    {
        uint8_t frameBytes[1024];
        size_t numBytes;
        err = HAPPlatformTCPStreamClientRead(
                &dataStreamTCPStreamManager, clientTCPStream, frameBytes, sizeof frameBytes, &numBytes);
        HAPAssert(!err);

        uint8_t* header;
        uint8_t* payload;
        size_t headerSize;
        size_t payloadSize;
        DataStreamTestHelpersDataStreamDecryptAccessoryPacketWithContext(
                &encryptionContext, frameBytes, numBytes, &header, &headerSize, &payload, &payloadSize);

        // Header.
        {
            HAPOPACKReader headerReader;
            HAPOPACKReaderCreate(&headerReader, header, headerSize);
            HAPOPACKStringDictionaryElement protocolElement, responseElement, idElement, statusElement;
            protocolElement.key = "protocol";
            responseElement.key = "response";
            idElement.key = "id";
            statusElement.key = "status";
            err = HAPOPACKStringDictionaryReaderGetAll(
                    &headerReader,
                    (HAPOPACKStringDictionaryElement* const[]) {
                            &protocolElement, &responseElement, &idElement, &statusElement, NULL });
            HAPAssert(!err);

            // Protocol name.
            HAPAssert(protocolElement.value.exists);
            char* protocolName;
            err = HAPOPACKReaderGetNextString(&protocolElement.value.reader, &protocolName);
            HAPAssert(!err);
            HAPAssert(HAPStringAreEqual(protocolName, "dataSend"));

            // Topic.
            HAPAssert(responseElement.value.exists);
            char* topic;
            err = HAPOPACKReaderGetNextString(&responseElement.value.reader, &topic);
            HAPAssert(!err);
            HAPAssert(HAPStringAreEqual(topic, "open"));

            // Request ID.
            HAPAssert(idElement.value.exists);
            int64_t responseRequestID;
            err = HAPOPACKReaderGetNextInt(&idElement.value.reader, &responseRequestID);
            HAPAssert(!err);
            HAPAssert(responseRequestID == requestID);

            // Response status.
            HAPAssert(statusElement.value.exists);
            int64_t status;
            err = HAPOPACKReaderGetNextInt(&statusElement.value.reader, &status);
            HAPAssert(!err);
            HAPAssert(status == 0);
        }

        // Message.
        {
            HAPOPACKReader messageReader;
            HAPOPACKReaderCreate(&messageReader, payload, payloadSize);
            HAPOPACKStringDictionaryElement statusElement;
            statusElement.key = "status";
            err = HAPOPACKStringDictionaryReaderGetAll(
                    &messageReader, (HAPOPACKStringDictionaryElement* const[]) { &statusElement, NULL });
            HAPAssert(!err);
            // The reponse message may not contain "status" key, if the request was successful
            HAPAssert(!statusElement.value.exists);
        }
    }
    HAPAssert(test.dataSendStreamOpen);

    // Send packets over dataSend stream.
    totalPackets = 10;
    nextSequenceNumber = 1;
    endOfStreamSet = false;
    for (size_t i = 0; i < totalPackets; i++) {
        HAPLog(&kHAPLog_Default, "Sending packets over dataSend stream (%zu).", i);
        uint8_t scratchBytes[1024];
        test.expectingSendCompletion = true;
        test.scratchBytes = scratchBytes;
        test.numScratchBytes = sizeof scratchBytes;
        {
            uint8_t packetBytes[] = { HAPExpandLittleUInt64((uint64_t) i * 1),
                                      HAPExpandLittleUInt64((uint64_t) i * 2),
                                      HAPExpandLittleUInt64((uint64_t) i * 4),
                                      HAPExpandLittleUInt64((uint64_t) i * 8) };
            HAPDataSendDataStreamProtocolPacket packets[] = {
                { .data = { .bytes = packetBytes, .numBytes = sizeof packetBytes },
                  .metadata = { .type = kHAPDataSendDataStreamProtocolType_IPCamera_Recording,
                                ._.ipCamera
                                        .recording = { .dataType =
                                                               i % 2 == 0 ?
                                                                       kHAPDataSendDataStreamProtocolIPCameraRecordingDataType_MediaInitialization :
                                                                       kHAPDataSendDataStreamProtocolIPCameraRecordingDataType_MediaFragment,
                                                       .dataSequenceNumber = (int64_t)(i + 1),
                                                       .isLastDataChunk = false,
                                                       .dataChunkSequenceNumber = 1 } } },
                { .data = { .bytes = packetBytes, .numBytes = sizeof packetBytes },
                  .metadata = { .type = kHAPDataSendDataStreamProtocolType_IPCamera_Recording,
                                ._.ipCamera
                                        .recording = { .dataType =
                                                               i % 2 == 0 ?
                                                                       kHAPDataSendDataStreamProtocolIPCameraRecordingDataType_MediaInitialization :
                                                                       kHAPDataSendDataStreamProtocolIPCameraRecordingDataType_MediaFragment,
                                                       .dataSequenceNumber = (int64_t)(i + 1),
                                                       .isLastDataChunk = true,
                                                       .dataChunkSequenceNumber = 2 } } }
            };
            err = HAPDataSendDataStreamProtocolSendData(
                    &accessoryServer,
                    HAPNonnullVoid(accessory.dataStream.delegate.context),
                    test.dataStream,
                    &dataSendStream,
                    scratchBytes,
                    sizeof scratchBytes,
                    packets,
                    HAPArrayCount(packets),
                    i == totalPackets - 1,
                    DataStreamTestHelpersHandleSendDataComplete);
            HAPAssert(!err);
        }
        HAPLog(&kHAPLog_Default, "Receiving dataSend.data event.");
        {
            HAPAssert(!endOfStreamSet);

            uint8_t frameBytes[1024];
            size_t numBytes;
            err = HAPPlatformTCPStreamClientRead(
                    &dataStreamTCPStreamManager, clientTCPStream, frameBytes, sizeof frameBytes, &numBytes);
            HAPAssert(!err);

            uint8_t* header;
            uint8_t* payload;
            size_t headerSize;
            size_t payloadSize;
            DataStreamTestHelpersDataStreamDecryptAccessoryPacketWithContext(
                    &encryptionContext, frameBytes, numBytes, &header, &headerSize, &payload, &payloadSize);

            // Header.
            {
                HAPOPACKReader headerReader;
                HAPOPACKReaderCreate(&headerReader, header, headerSize);
                HAPOPACKStringDictionaryElement protocolElement, eventElement;
                protocolElement.key = "protocol";
                eventElement.key = "event";
                err = HAPOPACKStringDictionaryReaderGetAll(
                        &headerReader,
                        (HAPOPACKStringDictionaryElement* const[]) { &protocolElement, &eventElement, NULL });
                HAPAssert(!err);

                // Protocol name.
                HAPAssert(protocolElement.value.exists);
                char* protocolName;
                err = HAPOPACKReaderGetNextString(&protocolElement.value.reader, &protocolName);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(protocolName, "dataSend"));

                // Topic.
                HAPAssert(eventElement.value.exists);
                char* topic;
                err = HAPOPACKReaderGetNextString(&eventElement.value.reader, &topic);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(topic, "data"));
            }

            // Message.
            {
                HAPOPACKReader messageReader;
                HAPOPACKReaderCreate(&messageReader, payload, payloadSize);
                HAPOPACKStringDictionaryElement streamIdElement, packetsElement, endOfStreamElement;
                streamIdElement.key = "streamId";
                packetsElement.key = "packets";
                endOfStreamElement.key = "endOfStream";
                err = HAPOPACKStringDictionaryReaderGetAll(
                        &messageReader,
                        (HAPOPACKStringDictionaryElement* const[]) {
                                &streamIdElement, &packetsElement, &endOfStreamElement, NULL });
                HAPAssert(!err);

                // Parse streamId.
                HAPAssert(streamIdElement.value.exists);
                int64_t streamId;
                err = HAPOPACKReaderGetNextInt(&streamIdElement.value.reader, &streamId);
                HAPAssert(!err);
                HAPAssert(streamId == streamID);

                // Parse packets.
                HAPAssert(packetsElement.value.exists);
                HAPOPACKReader packetsReader;
                err = HAPOPACKReaderGetNextArray(&packetsElement.value.reader, &packetsReader);
                HAPAssert(!err);

                for (size_t p = 0; true; p++) {
                    bool found;
                    err = HAPOPACKReaderPeekNextType(&packetsReader, &found, NULL);
                    HAPAssert(!err);
                    if (!found) {
                        break;
                    }
                    HAPOPACKReader packetReader;
                    err = HAPOPACKReaderGetNext(&packetsReader, &packetReader);
                    HAPAssert(!err);

                    HAPOPACKStringDictionaryElement dataElement, metadataElement;
                    dataElement.key = "data";
                    metadataElement.key = "metadata";
                    err = HAPOPACKStringDictionaryReaderGetAll(
                            &packetReader,
                            (HAPOPACKStringDictionaryElement* const[]) { &dataElement, &metadataElement, NULL });
                    HAPAssert(!err);

                    // Parse data.
                    HAPAssert(dataElement.value.exists);
                    void* dataBytes;
                    size_t numDataBytes;
                    err = HAPOPACKReaderGetNextData(&dataElement.value.reader, &dataBytes, &numDataBytes);
                    HAPAssert(!err);
                    HAPAssert(numDataBytes == 4 * sizeof(uint64_t));
                    HAPAssert(HAPReadLittleUInt64(&((uint8_t*) dataBytes)[0x00]) == i * 1);
                    HAPAssert(HAPReadLittleUInt64(&((uint8_t*) dataBytes)[0x08]) == i * 2);
                    HAPAssert(HAPReadLittleUInt64(&((uint8_t*) dataBytes)[0x10]) == i * 4);
                    HAPAssert(HAPReadLittleUInt64(&((uint8_t*) dataBytes)[0x18]) == i * 8);

                    // Parse metadata.
                    HAPAssert(metadataElement.value.exists);
                    HAPOPACKStringDictionaryElement dataTypeElement, dataSequenceNumberElement;
                    HAPOPACKStringDictionaryElement isLastDataChunkElement, dataChunkSequenceNumberElement;
                    dataTypeElement.key = "dataType";
                    dataSequenceNumberElement.key = "dataSequenceNumber";
                    isLastDataChunkElement.key = "isLastDataChunk";
                    dataChunkSequenceNumberElement.key = "dataChunkSequenceNumber";
                    err = HAPOPACKStringDictionaryReaderGetAll(
                            &metadataElement.value.reader,
                            (HAPOPACKStringDictionaryElement* const[]) { &dataTypeElement,
                                                                         &dataSequenceNumberElement,
                                                                         &isLastDataChunkElement,
                                                                         &dataChunkSequenceNumberElement,
                                                                         NULL });
                    HAPAssert(!err);

                    // Parse dataType.
                    HAPAssert(dataTypeElement.value.exists);
                    char* dataType;
                    err = HAPOPACKReaderGetNextString(&dataTypeElement.value.reader, &dataType);
                    HAPAssert(!err);
                    if (!(i % 2)) {
                        HAPAssert(HAPStringAreEqual(dataType, "mediaInitialization"));
                    } else {
                        HAPAssert(HAPStringAreEqual(dataType, "mediaFragment"));
                    }

                    // Parse dataSequenceNumber.
                    HAPAssert(dataSequenceNumberElement.value.exists);
                    int64_t dataSequenceNumber;
                    err = HAPOPACKReaderGetNextInt(&dataSequenceNumberElement.value.reader, &dataSequenceNumber);
                    HAPAssert(!err);
                    HAPAssert(dataSequenceNumber >= 1);
                    HAPAssert((size_t) dataSequenceNumber == nextSequenceNumber);

                    // Parse isLastDataChunk.
                    HAPAssert(isLastDataChunkElement.value.exists);
                    bool isLastDataChunk;
                    err = HAPOPACKReaderGetNextBool(&isLastDataChunkElement.value.reader, &isLastDataChunk);
                    HAPAssert(!err);
                    if (p == 0) {
                        HAPAssert(!isLastDataChunk);
                    } else {
                        HAPAssert(p == 1);
                        HAPAssert(isLastDataChunk);
                    }
                    if (isLastDataChunk) {
                        nextSequenceNumber++;
                    }

                    // Parse dataChunkSequenceNumber.
                    HAPAssert(dataChunkSequenceNumberElement.value.exists);
                    int64_t dataChunkSequenceNumber;
                    err = HAPOPACKReaderGetNextInt(
                            &dataChunkSequenceNumberElement.value.reader, &dataChunkSequenceNumber);
                    HAPAssert(!err);
                    HAPAssert(dataChunkSequenceNumber >= 1);
                    HAPAssert((size_t) dataChunkSequenceNumber == p + 1);
                }

                // Parse endOfStream.
                if (endOfStreamElement.value.exists) {
                    bool endOfStream;
                    err = HAPOPACKReaderGetNextBool(&endOfStreamElement.value.reader, &endOfStream);
                    HAPAssert(!err);
                    HAPAssert(!endOfStreamSet);
                    endOfStreamSet = endOfStream;
                }
            }
        }
        HAPAssert(!test.expectingSendCompletion);
    }
    HAPAssert(nextSequenceNumber == totalPackets + 1);
    HAPAssert(endOfStreamSet);
    HAPAssert(test.dataSendStreamValid);
    HAPAssert(test.dataSendStreamOpen);

    // Finish transfer.
    HAPLog(&kHAPLog_Default, "Sending dataSend.ack event.");
    {
        // clang-format off
        const uint8_t header[] = {
                0xE2,
                0x48, 'p', 'r', 'o',  't', 'o', 'c', 'o', 'l',
                0x48, 'd', 'a', 't', 'a', 'S',  'e',  'n', 'd',
                0x45, 'e', 'v', 'e', 'n', 't',
                0x43, 'a', 'c', 'k'
        };
        const uint8_t message[] = {
                0xE2,
                0x48, 's', 't', 'r', 'e', 'a', 'm', 'I', 'd',
                0x33, HAPExpandLittleInt64(streamID),
                0x4B, 'e', 'n',  'd',  'O', 'f', 'S', 't', 'r', 'e', 'a', 'm',
                0x01
        };
        // clang-format on
        {
            uint8_t frame[HAP_DATASTREAM_ENC_FRAME_MIN_LENGTH + sizeof header + sizeof message];
            DataStreamTestHelpersDataStreamEncryptControllerPacketWithContext(
                    &encryptionContext, frame, sizeof(frame), header, sizeof(header), message, sizeof(message));

            size_t numBytes;
            err = HAPPlatformTCPStreamClientWrite(
                    &dataStreamTCPStreamManager, clientTCPStream, frame, sizeof frame, &numBytes);
            HAPAssert(!err);
            HAPAssert(numBytes == sizeof frame);
        }
    }
    HAPLog(&kHAPLog_Default, "Receiving dataSend.close event.");
    {
        uint8_t frameBytes[1024];
        size_t numBytes;
        err = HAPPlatformTCPStreamClientRead(
                &dataStreamTCPStreamManager, clientTCPStream, frameBytes, sizeof frameBytes, &numBytes);
        HAPAssert(!err);

        uint8_t* header;
        uint8_t* payload;
        size_t headerSize;
        size_t payloadSize;
        DataStreamTestHelpersDataStreamDecryptAccessoryPacketWithContext(
                &encryptionContext, frameBytes, numBytes, &header, &headerSize, &payload, &payloadSize);

        // Header.
        {
            HAPOPACKReader headerReader;
            HAPOPACKReaderCreate(&headerReader, header, headerSize);
            HAPOPACKStringDictionaryElement protocolElement, eventElement;
            protocolElement.key = "protocol";
            eventElement.key = "event";
            err = HAPOPACKStringDictionaryReaderGetAll(
                    &headerReader,
                    (HAPOPACKStringDictionaryElement* const[]) { &protocolElement, &eventElement, NULL });
            HAPAssert(!err);

            // Protocol name.
            HAPAssert(protocolElement.value.exists);
            char* protocolName;
            err = HAPOPACKReaderGetNextString(&protocolElement.value.reader, &protocolName);
            HAPAssert(!err);
            HAPAssert(HAPStringAreEqual(protocolName, "dataSend"));

            // Topic.
            HAPAssert(eventElement.value.exists);
            char* topic;
            err = HAPOPACKReaderGetNextString(&eventElement.value.reader, &topic);
            HAPAssert(!err);
            HAPAssert(HAPStringAreEqual(topic, "close"));
        }

        // Message.
        {
            HAPOPACKReader messageReader;
            HAPOPACKReaderCreate(&messageReader, payload, payloadSize);
            HAPOPACKStringDictionaryElement streamIdElement, reasonElement;
            streamIdElement.key = "streamId";
            reasonElement.key = "reason";
            err = HAPOPACKStringDictionaryReaderGetAll(
                    &messageReader,
                    (HAPOPACKStringDictionaryElement* const[]) { &streamIdElement, &reasonElement, NULL });
            HAPAssert(!err);

            // Parse streamId.
            HAPAssert(streamIdElement.value.exists);
            int64_t streamId;
            err = HAPOPACKReaderGetNextInt(&streamIdElement.value.reader, &streamId);
            HAPAssert(!err);
            HAPAssert(streamId == streamID);

            // Parse reason.
            if (reasonElement.value.exists) {
                int64_t reason;
                err = HAPOPACKReaderGetNextInt(&reasonElement.value.reader, &reason);
                HAPAssert(!err);
                HAPAssert(reason == 0);
            }
        }
    }
    HAPAssert(!test.expectingSendCompletion);
    HAPAssert(!test.dataSendStreamValid);
    HAPAssert(!test.dataSendStreamOpen);
    HAPAssert(!test.closeReason);

    // test DataSend.Open error response. Error = 1 (controller OOM)
    // Open dataSend stream.
    HAPLog(&kHAPLog_Default, "Opening dataSend stream.");
    test.dataSendStreamValid = true;
    {
        HAPDataSendDataStreamProtocolOpen(
                &accessoryServer,
                HAPNonnullVoid(accessory.dataStream.delegate.context),
                test.dataStream,
                &dataSendStream,
                kHAPDataSendDataStreamProtocolType_Audio_Siri,
                &dataSendStreamCallbacks);
    }
    test.closeError = 3;
    {
        HAPLog(&kHAPLog_Default, "Receiving dataSend.open request.");
        int64_t requestID;
        {
            uint8_t frameBytes[1024];
            size_t numBytes;
            err = HAPPlatformTCPStreamClientRead(
                    &dataStreamTCPStreamManager, clientTCPStream, frameBytes, sizeof frameBytes, &numBytes);
            HAPAssert(!err);

            uint8_t* header;
            uint8_t* payload;
            size_t headerSize;
            size_t payloadSize;
            DataStreamTestHelpersDataStreamDecryptAccessoryPacketWithContext(
                    &encryptionContext, frameBytes, numBytes, &header, &headerSize, &payload, &payloadSize);

            // Header.
            {
                HAPOPACKReader headerReader;
                HAPOPACKReaderCreate(&headerReader, header, headerSize);
                HAPOPACKStringDictionaryElement protocolElement, requestElement, idElement;
                protocolElement.key = "protocol";
                requestElement.key = "request";
                idElement.key = "id";
                err = HAPOPACKStringDictionaryReaderGetAll(
                        &headerReader,
                        (HAPOPACKStringDictionaryElement* const[]) {
                                &protocolElement, &requestElement, &idElement, NULL });
                HAPAssert(!err);

                // Protocol name.
                HAPAssert(protocolElement.value.exists);
                char* protocolName;
                err = HAPOPACKReaderGetNextString(&protocolElement.value.reader, &protocolName);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(protocolName, "dataSend"));

                // Topic.
                HAPAssert(requestElement.value.exists);
                char* topic;
                err = HAPOPACKReaderGetNextString(&requestElement.value.reader, &topic);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(topic, "open"));

                // Request ID.
                HAPAssert(idElement.value.exists);
                err = HAPOPACKReaderGetNextInt(&idElement.value.reader, &requestID);
                HAPAssert(!err);
            }

            // Message.
            {
                HAPOPACKReader messageReader;
                HAPOPACKReaderCreate(&messageReader, payload, payloadSize);
                HAPOPACKStringDictionaryElement targetElement, typeElement;
                targetElement.key = "target";
                typeElement.key = "type";
                err = HAPOPACKStringDictionaryReaderGetAll(
                        &messageReader,
                        (HAPOPACKStringDictionaryElement* const[]) { &targetElement, &typeElement, NULL });
                HAPAssert(!err);

                // Parse target.
                HAPAssert(targetElement.value.exists);
                char* target;
                err = HAPOPACKReaderGetNextString(&targetElement.value.reader, &target);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(target, "controller"));

                // Parse type.
                HAPAssert(typeElement.value.exists);
                char* type;
                err = HAPOPACKReaderGetNextString(&typeElement.value.reader, &type);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(type, "audio.siri"));
            }
        }
        HAPPlatformClockAdvance(1 * HAPSecond);
        HAPLog(&kHAPLog_Default, "Sending dataSend.open response error 1.");
        HAPPlatformRandomNumberFill(&streamID, sizeof streamID);
        {
            // clang-format off
            const uint8_t header[] = {
                    0xE4,
                    0x48, 'p',  'r', 'o', 't', 'o',  'c', 'o',  'l',
                    0x48, 'd', 'a',  't', 'a',  'S', 'e',  'n', 'd',
                    0x48, 'r',  'e', 's',  'p', 'o',  'n', 's',  'e',
                    0x44, 'o', 'p',  'e', 'n',
                    0x42, 'i',  'd',
                    0x33, HAPExpandLittleInt64(requestID),
                    0x46, 's', 't',  'a', 't',  'u', 's',
                    0x09 /* 1 */
            };
            // clang-format on
            {
                uint8_t frame[HAP_DATASTREAM_ENC_FRAME_MIN_LENGTH + sizeof header];
                DataStreamTestHelpersDataStreamEncryptControllerPacketWithContext(
                        &encryptionContext, frame, sizeof(frame), header, sizeof(header), NULL, 0);

                size_t numBytes;
                err = HAPPlatformTCPStreamClientWrite(
                        &dataStreamTCPStreamManager, clientTCPStream, frame, sizeof frame, &numBytes);
                HAPAssert(!err);
                HAPAssert(numBytes == sizeof frame);
            }
        }
    }
    HAPAssert(test.closeReason == 5);
    HAPAssert(!test.dataSendStreamOpen);

    // test DataSend.Open error response. Error = 6 (protocol error) (error 9: Invalid Configuration)
    // Open dataSend stream.
    HAPLog(&kHAPLog_Default, "Opening dataSend stream.");
    test.dataSendStreamValid = true;
    {
        HAPDataSendDataStreamProtocolOpen(
                &accessoryServer,
                HAPNonnullVoid(accessory.dataStream.delegate.context),
                test.dataStream,
                &dataSendStream,
                kHAPDataSendDataStreamProtocolType_Audio_Siri,
                &dataSendStreamCallbacks);
    }
    test.closeError = 2;
    {
        HAPLog(&kHAPLog_Default, "Receiving dataSend.open request.");
        int64_t requestID;
        {
            uint8_t frameBytes[1024];
            size_t numBytes;
            err = HAPPlatformTCPStreamClientRead(
                    &dataStreamTCPStreamManager, clientTCPStream, frameBytes, sizeof frameBytes, &numBytes);
            HAPAssert(!err);

            uint8_t* header;
            uint8_t* payload;
            size_t headerSize;
            size_t payloadSize;
            DataStreamTestHelpersDataStreamDecryptAccessoryPacketWithContext(
                    &encryptionContext, frameBytes, numBytes, &header, &headerSize, &payload, &payloadSize);

            // Header.
            {
                HAPOPACKReader headerReader;
                HAPOPACKReaderCreate(&headerReader, header, headerSize);
                HAPOPACKStringDictionaryElement protocolElement, requestElement, idElement;
                protocolElement.key = "protocol";
                requestElement.key = "request";
                idElement.key = "id";
                err = HAPOPACKStringDictionaryReaderGetAll(
                        &headerReader,
                        (HAPOPACKStringDictionaryElement* const[]) {
                                &protocolElement, &requestElement, &idElement, NULL });
                HAPAssert(!err);

                // Protocol name.
                HAPAssert(protocolElement.value.exists);
                char* protocolName;
                err = HAPOPACKReaderGetNextString(&protocolElement.value.reader, &protocolName);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(protocolName, "dataSend"));

                // Topic.
                HAPAssert(requestElement.value.exists);
                char* topic;
                err = HAPOPACKReaderGetNextString(&requestElement.value.reader, &topic);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(topic, "open"));

                // Request ID.
                HAPAssert(idElement.value.exists);
                err = HAPOPACKReaderGetNextInt(&idElement.value.reader, &requestID);
                HAPAssert(!err);
            }

            // Message.
            {
                HAPOPACKReader messageReader;
                HAPOPACKReaderCreate(&messageReader, payload, payloadSize);
                HAPOPACKStringDictionaryElement targetElement, typeElement;
                targetElement.key = "target";
                typeElement.key = "type";
                err = HAPOPACKStringDictionaryReaderGetAll(
                        &messageReader,
                        (HAPOPACKStringDictionaryElement* const[]) { &targetElement, &typeElement, NULL });
                HAPAssert(!err);

                // Parse target.
                HAPAssert(targetElement.value.exists);
                char* target;
                err = HAPOPACKReaderGetNextString(&targetElement.value.reader, &target);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(target, "controller"));

                // Parse type.
                HAPAssert(typeElement.value.exists);
                char* type;
                err = HAPOPACKReaderGetNextString(&typeElement.value.reader, &type);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(type, "audio.siri"));
            }
        }
        HAPPlatformClockAdvance(1 * HAPSecond);
        HAPLog(&kHAPLog_Default, "Sending dataSend.open response error 6, status 9.");
        HAPPlatformRandomNumberFill(&streamID, sizeof streamID);
        {
            // clang-format off
            const uint8_t header[] = {
                    0xE4,
                    0x48, 'p',  'r', 'o', 't', 'o',  'c', 'o',  'l',
                    0x48, 'd', 'a',  't', 'a',  'S', 'e',  'n', 'd',
                    0x48, 'r',  'e', 's',  'p', 'o',  'n', 's',  'e',
                    0x44, 'o', 'p',  'e', 'n',
                    0x42, 'i',  'd',
                    0x33, HAPExpandLittleInt64(requestID),
                    0x46, 's', 't',  'a', 't',  'u', 's',
                    0x0E /* 6 - Protocol Error */
            };
            const uint8_t message[] = {
                    0xE1,
                    0x46, 's', 't', 'a',  't', 'u',  's',
                    0x11 /* 9 - Invalid Configuration */
            };
            // clang-format on
            {
                uint8_t frame[HAP_DATASTREAM_ENC_FRAME_MIN_LENGTH + sizeof header + sizeof message];
                DataStreamTestHelpersDataStreamEncryptControllerPacketWithContext(
                        &encryptionContext, frame, sizeof(frame), header, sizeof(header), message, sizeof(message));

                size_t numBytes;
                err = HAPPlatformTCPStreamClientWrite(
                        &dataStreamTCPStreamManager, clientTCPStream, frame, sizeof frame, &numBytes);
                HAPAssert(!err);
                HAPAssert(numBytes == sizeof frame);
            }
        }
    }
    HAPLog(&kHAPLog_Default, "test.closeReason = %d", test.closeReason);
    HAPAssert(test.closeReason == 8);
    HAPAssert(!test.dataSendStreamOpen);

    // Stop accessory server.
    HAPLog(&kHAPLog_Default, "Stopping accessory server.");
    HAPAccessoryServerForceStop(&accessoryServer);
    HAPPlatformClockAdvance(0);
    HAPPlatformClockAdvance(0);
    HAPAssert(HAPAccessoryServerGetState(&accessoryServer) == kHAPAccessoryServerState_Idle);

    // Check that HomeKit Data Stream has been invalidated.
    HAPAssert(!test.dataSendAccepted);

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
