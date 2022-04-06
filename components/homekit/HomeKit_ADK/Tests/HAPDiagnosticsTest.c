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

#include "HAP.h"

#include "Harness/DataStreamTestHelpers.h"
#include "Harness/HAPAccessoryServerHelper.h"
#include "Harness/HAPPlatformClockHelper.h"
#include "Harness/IPTestHelpers.h"
#include "Harness/SecurityTestHelpers.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)

#include "HAPDiagnostics.h"
#include "HAPPlatformDiagnostics.h"
#define kApp_NumDataStreams ((size_t) 32)

static const HAPDataSendDataStreamProtocolStreamCallbacks dataSendStreamCallbacks = {
    .handleClose = DataStreamTestHelpersHandleDataSendStreamClose,
    .handleOpen = DataStreamTestHelpersHandleDataSendStreamOpen
};

static const HAPDataSendDataStreamProtocolOpenMetadata dataSendOpenMetadata = {
    .type = kHAPDataSendDataStreamProtocolType_Diagnostics_Snapshot,
    ._.diagnostics.snapshot = { .maxLogSize = 8388608, .snapshotType = kHAPDiagnosticsSnapshotType_ADK }
};

static bool isMetaDataProvided = false;

void HAPDiagnosticsTestHandleDataSendStreamAvailable(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataSendDataStreamProtocol* dataStreamProtocol,
        const HAPServiceRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolType type,
        HAPDataSendDataStreamProtocolOpenMetadata* _Nullable metadata,
        void* _Nullable inDataSendStreamCallbacks HAP_UNUSED,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol);
    HAPPrecondition(request);
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPPrecondition(metadata);

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    if (metadata->type == kHAPDataSendDataStreamProtocolType_Diagnostics_Snapshot) {
        HAPLog(&kHAPLog_Default, "Metadata : maxLogSize : %d", (int) (metadata->_.diagnostics.snapshot.maxLogSize));
        HAPLog(&kHAPLog_Default, "Metadata : snapshotType : %d", (int) (metadata->_.diagnostics.snapshot.snapshotType));

        if (isMetaDataProvided == true) {
            HAPAssert(
                    metadata->_.diagnostics.snapshot.maxLogSize ==
                    dataSendOpenMetadata._.diagnostics.snapshot.maxLogSize);
            HAPAssert(
                    metadata->_.diagnostics.snapshot.snapshotType ==
                    dataSendOpenMetadata._.diagnostics.snapshot.snapshotType);
        } else {
            HAPAssert(metadata->_.diagnostics.snapshot.maxLogSize == 0);
            HAPAssert(metadata->_.diagnostics.snapshot.snapshotType == kHAPDiagnosticsSnapshotType_Manufacturer);
        }
    }

    if (type != kHAPDataSendDataStreamProtocolType_Diagnostics_Snapshot) {
        HAPLog(&kHAPLog_Default, "Unsupported incoming \"dataSend\" stream type. Rejecting \"dataSend\" stream.");
        HAPDataSendDataStreamProtocolReject(
                server, dispatcher, dataStream, kHAPDataSendDataStreamProtocolRejectReason_Unsupported);
        return;
    }

    HAPError err;

    HAPAccessoryDiagnosticsConfig accessoryDiagnosticsConfig;
    HAPRawBufferZero(&accessoryDiagnosticsConfig, sizeof accessoryDiagnosticsConfig);

    HAPAssert(request->accessory->callbacks.diagnosticsConfig.getDiagnosticsConfig);

    err = request->accessory->callbacks.diagnosticsConfig.getDiagnosticsConfig(
            server, request->accessory, &accessoryDiagnosticsConfig, context);
    if (err) {
        HAPAssert(!err);
    }
    HAPDiagnosticsContext* dgContext = (HAPDiagnosticsContext*) (accessoryDiagnosticsConfig.diagnosticsContext);
    HAPPrecondition(dgContext);
    HAPDiagnosticsDataStreamContext* dsContext = &(dgContext->dataStreamContext);
    HAPPrecondition(dsContext);
    if (dsContext->server || dsContext->inProgress) {
        HAPLog(&kHAPLog_Default, "\"dataSend\" stream already open. Rejecting \"dataSend\" stream.");
        HAPDataSendDataStreamProtocolReject(
                server, dispatcher, dataStream, kHAPDataSendDataStreamProtocolRejectReason_Busy);
        return;
    }

    // Mark dataStreamContext is in progress to prevent a concurrent diagnostics request.
    HAPDataSendDataStreamProtocolStream* dataSendStream = &(dgContext->dataSendStream);
    dgContext->dataStreamContext.inProgress = true;

    HAPDataSendDataStreamProtocolAccept(server, dispatcher, dataStream, dataSendStream, &dataSendStreamCallbacks);

    // Setup Data Stream context.
    HAPRawBufferZero(dsContext, sizeof *dsContext);
    dsContext->server = server;
    dsContext->dispatcher = dispatcher;
    dsContext->dataStream = dataStream;
    dsContext->dataSendStream = dataSendStream;

    HAPLogInfo(&kHAPLog_Default, "Accepted \"dataSend\" stream: dataSendStream = %p.", (const void*) dataSendStream);
}

static void HandleSendDiagnosticDataComplete(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataSendDataStreamProtocol* dataStreamProtocol,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolStream* dataSendStream,
        HAPError error,
        void* scratchBytes,
        size_t numScratchBytes,
        void* _Nullable _context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol);
    HAPPrecondition(request);
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPPrecondition(dataSendStream);
    HAPPrecondition(scratchBytes);
    HAPPrecondition(numScratchBytes);

    HAPError err;

    HAPAccessoryDiagnosticsConfig accessoryDiagnosticsConfig;
    HAPRawBufferZero(&accessoryDiagnosticsConfig, sizeof accessoryDiagnosticsConfig);

    HAPAssert(request->accessory->callbacks.diagnosticsConfig.getDiagnosticsConfig);

    err = request->accessory->callbacks.diagnosticsConfig.getDiagnosticsConfig(
            NULL, request->accessory, &accessoryDiagnosticsConfig, NULL);
    HAPAssert(!err);

    HAPDiagnosticsContext* dgContext = (HAPDiagnosticsContext*) (accessoryDiagnosticsConfig.diagnosticsContext);
    HAPPrecondition(dgContext);

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
}

static HAPDiagnosticsContext diagnosticsContext;
static HAPDataSendDataStreamProtocolPacketDiagnosticsMetadataKeyValuePairs diagnosticsURLKeyValue[] = {
    { .key = "signed-timestamp", .value = "123abc" },
    { .key = "MAC", .value = "some_mac_address" }
};

static HAPDiagnosticsUrlParameters diagnosticsURLParameters = {
    .numUrlParameterPairs = 2,
    .urlParameterPairs = diagnosticsURLKeyValue,
};

static char* diagnosticsFolderName = "Diagnostics_Test";
static char* diagnosticsLogFileName = "Test.log";
static size_t diagnosticsMaxLogSizeInMB = 1;
static HAPAccessoryDiagnosticsConfig accessoryDiagnosticsConfig = {
    .diagnosticsSnapshotFormat = kHAPDiagnosticsSnapshotFormat_Zip,
    .diagnosticsSnapshotType = kHAPDiagnosticsSnapshotType_Manufacturer,
};

static HAPTargetControlDataStreamProtocol targetControlDataStreamProtocol = {
    .base = &kHAPTargetControlDataStreamProtocol_Base,
    .callbacks = { .handleIdentifierUpdate = DataStreamTestHelpersHandleTargetControlIdentifierUpdate }
};

static HAPDataSendDataStreamProtocolContext dataSendDataStreamProtocolContexts[kApp_NumDataStreams];
static HAPDataSendDataStreamProtocolListener dataSendDataStreamProtocolListeners[kApp_NumDataStreams];
static HAPDataSendStreamProtocolStreamAvailableCallbacks dataSendDataStreamProtocolAvailableCallbacks[] = {
    { .type = kHAPDataSendDataStreamProtocolType_Diagnostics_Snapshot,
      .handleStreamAvailable = HAPDiagnosticsTestHandleDataSendStreamAvailable }

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
                                                                            &diagnosticsService,
                                                                            NULL },
                                  .callbacks = { .identify = IdentifyAccessoryHelper } };

HAP_RESULT_USE_CHECK
HAPError GetAccessoryDiagnosticsConfig(
        HAPAccessoryServer* _Nullable server HAP_UNUSED,
        const HAPAccessory* _Nullable accessory HAP_UNUSED,
        HAPAccessoryDiagnosticsConfig* diagnosticsConfigStruct,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    HAPPrecondition(diagnosticsConfigStruct);

    HAPRawBufferCopyBytes(diagnosticsConfigStruct, &accessoryDiagnosticsConfig, sizeof accessoryDiagnosticsConfig);
    diagnosticsContext.urlParameters = &diagnosticsURLParameters;
    diagnosticsConfigStruct->diagnosticsContext = &diagnosticsContext;
    return kHAPError_None;
}

static void TestSupportedDiagnosticsSnapshot(HAPSession* session, HAPAccessoryServer* server) {

    HAPLog(&kHAPLog_Default, "Test SupportedDiagnosticsSnapshot read handler");

    uint8_t bytes[1024];
    HAPError err;
    HAPTLVWriter responseWriter;
    HAPTLVWriterCreate(&responseWriter, bytes, sizeof bytes);
    const HAPTLV8CharacteristicReadRequest request = { .transportType = kHAPTransportType_IP,
                                                       .session = session,
                                                       .characteristic = &diagnosticsSupportedDiagnosticsSnapshot,
                                                       .service = &diagnosticsService,
                                                       .accessory = &accessory };
    err = request.characteristic->callbacks.handleRead(server, &request, &responseWriter, NULL);
    HAPAssert(!err);
    void* responseBytes;
    size_t numResponseBytes;
    HAPTLVWriterGetBuffer(&responseWriter, &responseBytes, &numResponseBytes);
    HAPTLVReader responseReader;
    HAPTLVReaderCreate(&responseReader, responseBytes, numResponseBytes);

    bool tlvRecv[4] = { false };

    for (;;) {
        HAPTLV tlv;
        bool valid;
        err = HAPTLVReaderGetNext(&responseReader, &valid, &tlv);
        HAPAssert(!err);
        if (!valid) {
            break;
        }

        HAPAssert(tlv.type != 0 && tlv.type <= sizeof tlvRecv / sizeof tlvRecv[0]);
        HAPAssert(!tlvRecv[tlv.type - 1]);
        tlvRecv[tlv.type - 1] = true;

        // Validate TLV data
        switch (tlv.type) {
            case 1: {
                // kHAPCharacteristicValue_SupportedDiagnosticsSnapshot_Format
                HAPAssert(HAPReadUInt8(tlv.value.bytes) == accessoryDiagnosticsConfig.diagnosticsSnapshotFormat);
            } break;
            case 2: {
                // kHAPCharacteristicValue_SupportedDiagnosticsSnapshot_Type
                HAPAssert(HAPReadUInt8(tlv.value.bytes) == accessoryDiagnosticsConfig.diagnosticsSnapshotType);
            } break;
            case 4: {
                // kHAPCharacteristicValue_SupportedDiagnosticsSnapshot_Options
                HAPAssert(HAPReadUInt8(tlv.value.bytes) == accessoryDiagnosticsConfig.diagnosticsSnapshotOptions);
            } break;
        }
    }

    // Validate expected TLV(s) were received
    HAPAssert(tlvRecv[0]);
    HAPAssert(tlvRecv[1]);
    HAPAssert(tlvRecv[3]);
}

/**
 * Test1: Capture diagnostics HAP log messages to file
 */
static void Test1_LogCapture() {
    HAPLog(&kHAPLog_Default, "Test 1: Log capture to file");
    HAPError err;
    err = HAPDiagnosticsSetupLogCaptureToFile(
            diagnosticsFolderName, diagnosticsLogFileName, diagnosticsMaxLogSizeInMB, &accessory);
    HAPAssert(!err);
    HAPLog(&kHAPLog_Default, "Writing test message to file");
    HAPDiagnosticsStopLogCaptureToFile(&accessory);
}

/**
 * Test2: Provide invalid log file size to HAP Diagnostics
 */
static void Test2_InvalidLogFileSize() {
    HAPLog(&kHAPLog_Default, "Test 2: Invalid log file size");
    HAPError err;
    size_t maxLogSizeInMB = 10;
    err = HAPDiagnosticsSetupLogCaptureToFile(
            diagnosticsFolderName, diagnosticsLogFileName, maxLogSizeInMB, &accessory);
    HAPAssert(err);
    HAPDiagnosticsStopLogCaptureToFile(&accessory);
}

/**
 * Test3: Controller requests accessory for diagnostics file
 *        The diagnostics file will be zipped and sent to the
 *        controller from the accessory
 *
 * @param accessoryServer              Accessory server
 * @param session                      Session relevant to the controller
 * @param test                         Data Send test context
 * @param dataStreamTCPStreamManager   TCP stream manager instance
 */
static void Test3_DiagnosticsSendFile(
        HAPAccessoryServer* accessoryServer,
        HAPSession* session,
        DataSendTestContext* test,
        HAPPlatformTCPStreamManager* dataStreamTCPStreamManager) {
    HAPLog(&kHAPLog_Default, "Test 3: Trigger diagnostics upload to controller");

    HAPError err;

    // Get Supported Data Stream Transport Configuration.
    bool supportsTCP;
    bool supportsHAP;
    DataStreamTestHelpersReadSupportedDataStreamTransportConfiguration(
            accessoryServer, &accessory, session, &supportsTCP, &supportsHAP, test);
    HAPAssert(!supportsHAP);
    HAPAssert(supportsTCP);

    // Generate Controller Key Salt.
    HAPDataStreamSalt controllerKeySalt;
    HAPPlatformRandomNumberFill(controllerKeySalt.bytes, sizeof controllerKeySalt.bytes);

    // Setup HomeKit Data Stream Transport.
    err = DataStreamTestHelpersWriteSetupDataStreamTransport(
            accessoryServer, &accessory, session, &controllerKeySalt, test);
    HAPAssert(!err);
    uint8_t setupStatus;
    HAPNetworkPort listeningPort;
    HAPDataStreamSalt accessoryKeySalt;
    HAPDataStreamHAPSessionIdentifier sessionIdentifier;
    DataStreamTestHelpersReadSetupDataStreamTransport(
            accessoryServer,
            &accessory,
            session,
            &setupStatus,
            &listeningPort,
            &accessoryKeySalt,
            &sessionIdentifier,
            test);
    HAPAssert(sessionIdentifier == kHAPDataStreamHAPSessionIdentifierNone);
    HAPAssert(setupStatus == 0);
    HAPAssert(listeningPort == HAPPlatformTCPStreamManagerGetListenerPort(dataStreamTCPStreamManager));
    HAPAssert(!HAPRawBufferIsZero(accessoryKeySalt.bytes, sizeof(accessoryKeySalt.bytes)));

    // Derive encryption keys.
    DataStreamEncryptionContext encryptionContext;
    DataStreamTestHelpersDataStreamInitEncryptionContext(
            session, &controllerKeySalt, &accessoryKeySalt, &encryptionContext);

    // Connect HomeKit Data Stream.
    HAPLog(&kHAPLog_Default, "Connecting HomeKit Data Stream.");
    HAPPlatformTCPStreamRef clientTCPStream;
    err = HAPPlatformTCPStreamManagerConnectToListener(dataStreamTCPStreamManager, &clientTCPStream);
    HAPAssert(!err);

    // Send control.hello request.
    DataStreamTestHelpersSendHelloRequest(dataStreamTCPStreamManager, &clientTCPStream, &encryptionContext);
    // Receive response for control.hello request.
    DataStreamTestHelpersReceiveHelloResponse(dataStreamTCPStreamManager, &clientTCPStream, &encryptionContext);

    HAPAssert(test->dataSendAccepted);

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
                    dataStreamTCPStreamManager, clientTCPStream, frame, sizeof frame, &numBytes);
            HAPAssert(!err);
            HAPAssert(numBytes == sizeof frame);
        }
    }
    HAPAssert(test->targetIdentifier == targetIdentifier);

    // Open dataSend stream.
    HAPLog(&kHAPLog_Default, "Opening dataSend stream.");
    test->dataSendStreamValid = true;

    int64_t streamID;
    int64_t requestID;
    HAPPlatformRandomNumberFill(&streamID, sizeof streamID);
    HAPPlatformRandomNumberFill(&requestID, sizeof requestID);
    {
        // Open dataSend stream (controller > accessory).
        HAPLog(&kHAPLog_Default, "Sending dataSend.open request.");

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
        isMetaDataProvided = true;
        const uint8_t message[] = {
                0xE4,
                0x46, 't', 'a', 'r', 'g', 'e', 't',
                0x4A, 'c', 'o', 'n', 't', 'r', 'o', 'l', 'l', 'e', 'r',
                0x44, 't', 'y', 'p', 'e',
                0x54, 'd', 'i', 'a', 'g', 'n', 'o', 's', 't', 'i', 'c', 's', '.', 's', 'n', 'a', 'p', 's', 'h', 'o', 't',
                0x48, 's', 't', 'r', 'e', 'a', 'm', 'I', 'd',
                0x33, HAPExpandLittleInt64(streamID),
                0x48, 'm', 'e', 't', 'a', 'd', 'a', 't', 'a',
                0xE2,
                0x4A, 'm', 'a', 'x', 'L', 'o', 'g', 'S', 'i', 'z', 'e',
                0x33, HAPExpandLittleUInt64(dataSendOpenMetadata._.diagnostics.snapshot.maxLogSize),
                0x4C, 's', 'n', 'a', 'p', 's', 'h', 'o', 't', 'T', 'y', 'p', 'e',
                0x30, (uint8_t) kHAPDiagnosticsSnapshotType_ADK
        };

        // clang-format on
        {
            uint8_t frame[HAP_DATASTREAM_ENC_FRAME_MIN_LENGTH + sizeof header + sizeof message];
            DataStreamTestHelpersDataStreamEncryptControllerPacketWithContext(
                    &encryptionContext, frame, sizeof(frame), header, sizeof(header), message, sizeof(message));

            size_t numBytes;
            err = HAPPlatformTCPStreamClientWrite(
                    dataStreamTCPStreamManager, clientTCPStream, frame, sizeof frame, &numBytes);
            HAPAssert(!err);
            HAPAssert(numBytes == sizeof frame);
        }
    }

    HAPLog(&kHAPLog_Default, "Receiving dataSend.open response.");
    uint8_t frameBytes[kHAPDiagnostics_NumScratchBytes];
    size_t numBytes;
    err = HAPPlatformTCPStreamClientRead(
            dataStreamTCPStreamManager, clientTCPStream, frameBytes, sizeof frameBytes, &numBytes);
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

    // Attempt to send another request. Expect to get busy response error
    int64_t streamID2;
    int64_t requestID2;
    HAPPlatformRandomNumberFill(&streamID2, sizeof streamID2);
    HAPPlatformRandomNumberFill(&requestID2, sizeof requestID);
    {
        // Open dataSend stream (controller > accessory).
        HAPLog(&kHAPLog_Default, "Sending dataSend.open request.");

        // clang-format off
        const uint8_t header[] = {
                0xE3,
                0x48, 'p', 'r', 'o', 't', 'o', 'c', 'o', 'l',
                0x48, 'd', 'a', 't', 'a', 'S', 'e', 'n', 'd',
                0x47, 'r', 'e', 'q', 'u', 'e', 's', 't',
                0x44, 'o', 'p', 'e', 'n',
                0x42, 'i', 'd',
                0x33, HAPExpandLittleInt64(requestID2)
        };
        isMetaDataProvided = false;
        const uint8_t message[] = {
                0xE3,
                0x46, 't', 'a', 'r', 'g', 'e', 't',
                0x4A, 'c', 'o', 'n', 't', 'r', 'o', 'l', 'l', 'e', 'r',
                0x44, 't', 'y', 'p', 'e',
                0x54, 'd', 'i', 'a', 'g', 'n', 'o', 's', 't', 'i', 'c', 's', '.', 's', 'n', 'a', 'p', 's', 'h', 'o', 't',
                0x48, 's', 't', 'r', 'e', 'a', 'm', 'I', 'd',
                0x33, HAPExpandLittleInt64(streamID2)
        };
        // clang-format on
        {
            uint8_t frame[HAP_DATASTREAM_ENC_FRAME_MIN_LENGTH + sizeof header + sizeof message];
            DataStreamTestHelpersDataStreamEncryptControllerPacketWithContext(
                    &encryptionContext, frame, sizeof(frame), header, sizeof(header), message, sizeof(message));

            size_t numBytes;
            err = HAPPlatformTCPStreamClientWrite(
                    dataStreamTCPStreamManager, clientTCPStream, frame, sizeof frame, &numBytes);
            HAPAssert(!err);
            HAPAssert(numBytes == sizeof frame);
        }
    }

    HAPLog(&kHAPLog_Default, "Receiving dataSend.open response.");
    err = HAPPlatformTCPStreamClientRead(
            dataStreamTCPStreamManager, clientTCPStream, frameBytes, sizeof frameBytes, &numBytes);
    HAPAssert(!err);

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
        HAPAssert(responseRequestID == requestID2);

        // Response status.
        HAPAssert(statusElement.value.exists);
        int64_t status;
        err = HAPOPACKReaderGetNextInt(&statusElement.value.reader, &status);
        HAPAssert(!err);
        HAPAssert(status != 0);
    }

    HAPLog(&kHAPLog_Default, "Sending diagnostics log.");
    uint8_t chunkBuf[kHAPDiagnostics_NumScratchBytes];
    size_t chunkBytesRead = 100;
    bool isEOF = true;

    diagnosticsContext.dataSequenceNumber++;
    HAPDataSendDataStreamProtocolPacket packets[] = {
        { .data = { .bytes = chunkBuf, .numBytes = chunkBytesRead },
          .metadata = { .type = kHAPDataSendDataStreamProtocolType_Diagnostics_Snapshot,
                        ._.diagnostics
                                .snapshot = { .dataSequenceNumber = diagnosticsContext.dataSequenceNumber,
                                              .numUrlParameterPairs =
                                                      (diagnosticsContext.dataSequenceNumber == 1 &&
                                                       diagnosticsContext.urlParameters != NULL) ?
                                                              diagnosticsContext.urlParameters->numUrlParameterPairs :
                                                              0,
                                              .urlParameterPairs =
                                                      (diagnosticsContext.dataSequenceNumber == 1 &&
                                                       diagnosticsContext.urlParameters != NULL) ?
                                                              diagnosticsContext.urlParameters->urlParameterPairs :
                                                              NULL } } }
    };

    err = HAPDataSendDataStreamProtocolSendData(
            diagnosticsContext.dataStreamContext.server,
            diagnosticsContext.dataStreamContext.dispatcher,
            diagnosticsContext.dataStreamContext.dataStream,
            diagnosticsContext.dataStreamContext.dataSendStream,
            diagnosticsContext.dataSendScratchBytes,
            sizeof diagnosticsContext.dataSendScratchBytes,
            packets,
            HAPArrayCount(packets),
            isEOF, // endOfStream
            HandleSendDiagnosticDataComplete);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPLogError(&kHAPLog_Default, "Scratch buffer too small. dataSendScratchBytes needs to be enlarged.");
        HAPFatalError();
    }

    HAPLog(&kHAPLog_Default, "Receiving diagnostics log.");
    err = HAPPlatformTCPStreamClientRead(
            dataStreamTCPStreamManager, clientTCPStream, frameBytes, sizeof frameBytes, &numBytes);
    HAPAssert(!err);

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
                &headerReader, (HAPOPACKStringDictionaryElement* const[]) { &protocolElement, &eventElement, NULL });
        HAPAssert(!err);

        // Protocol name.
        HAPAssert(protocolElement.value.exists);
        char* protocolName;
        err = HAPOPACKReaderGetNextString(&protocolElement.value.reader, &protocolName);
        HAPAssert(!err);
        HAPAssert(HAPStringAreEqual(protocolName, "dataSend"));

        // Event.
        HAPAssert(eventElement.value.exists);
        char* event;
        err = HAPOPACKReaderGetNextString(&eventElement.value.reader, &event);
        HAPAssert(!err);
        HAPAssert(HAPStringAreEqual(event, "data"));
    }

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
                    dataStreamTCPStreamManager, clientTCPStream, frame, sizeof frame, &numBytes);
            HAPAssert(!err);
            HAPAssert(numBytes == sizeof frame);
        }
    }
    HAPLog(&kHAPLog_Default, "Receiving dataSend.close event.");
    {
        uint8_t frameBytes[1024];
        size_t numBytes;
        err = HAPPlatformTCPStreamClientRead(
                dataStreamTCPStreamManager, clientTCPStream, frameBytes, sizeof frameBytes, &numBytes);
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

    HAPPlatformTCPStreamManagerClientClose(dataStreamTCPStreamManager, clientTCPStream);

    // Ensure TCP stream manager was properly cleaned up.
    HAPAssert(!HAPPlatformTCPStreamManagerIsListenerOpen(dataStreamTCPStreamManager));
}

/**
 * Test4: Write to diagnostics log buffer
 */
static void Test4_LogBufferWrites() {
    HAPLog(&kHAPLog_Default, "Write to diagnostics log buffer test.");
    uint8_t testLogBuffer[1000];
    char buffer[50];

    HAPRawBufferZero(buffer, sizeof buffer);
    HAPRawBufferZero(testLogBuffer, sizeof testLogBuffer);

    diagnosticsContext.logBuffer = testLogBuffer;
    diagnosticsContext.logBufferSizeBytes = sizeof testLogBuffer;
    HAPCircularQueueCreate(
            &(diagnosticsContext.logBufferQueue), diagnosticsContext.logBuffer, diagnosticsContext.logBufferSizeBytes);

    accessoryDiagnosticsConfig.diagnosticsContext = &diagnosticsContext;
    HAPPlatformDiagnosticsInitialize(&accessoryDiagnosticsConfig);

    HAPError err = HAPStringWithFormat(buffer, sizeof buffer, "%s", "TestMsg-1");
    HAPAssert(!err);
    HAPPlatformDiagnosticsWriteToLogBuffer("%s", buffer);

    HAPAssert(diagnosticsContext.logBufferQueue.totalSize == sizeof testLogBuffer);
    HAPAssert(diagnosticsContext.logBufferQueue.usedSpace == HAPStringGetNumBytes(buffer));
    HAPAssert(
            diagnosticsContext.logBufferQueue.remainingSpace == (sizeof testLogBuffer - HAPStringGetNumBytes(buffer)));
    HAPPlatformDiagnosticsDeinitialize();
}

/**
 * Test5: Circular functionality of diagnostics log buffer
 */
static void Test5_LogBufferOverflowWrite() {
    HAPLog(&kHAPLog_Default, "Test circular logging of diagnostics log buffer.");
    uint8_t testLogBuffer[1000];
    char buffer[50];

    HAPRawBufferZero(buffer, sizeof buffer);
    HAPRawBufferZero(testLogBuffer, sizeof testLogBuffer);

    diagnosticsContext.logBuffer = testLogBuffer;
    diagnosticsContext.logBufferSizeBytes = sizeof testLogBuffer;
    HAPCircularQueueCreate(
            &(diagnosticsContext.logBufferQueue), diagnosticsContext.logBuffer, diagnosticsContext.logBufferSizeBytes);

    accessoryDiagnosticsConfig.diagnosticsContext = &diagnosticsContext;
    HAPPlatformDiagnosticsInitialize(&accessoryDiagnosticsConfig);

    for (int i = 0; i < 20; i++) {
        HAPError err = HAPStringWithFormat(buffer, sizeof buffer, "%s", "TestMsg-1");
        HAPAssert(!err);
        HAPPlatformDiagnosticsWriteToLogBuffer("%s", buffer);
    }

    HAPAssert(diagnosticsContext.logBufferQueue.totalSize == sizeof testLogBuffer);
    HAPAssert(diagnosticsContext.logBufferQueue.usedSpace > HAPStringGetNumBytes(buffer));
    HAPPlatformDiagnosticsDeinitialize();
}

/**
 * Test6: Circular functionality of diagnostics log buffer
 */
static void Test6_LogBufferLargeBufferWrite() {
    HAPLog(&kHAPLog_Default, "Attempt to write a bigger buffer than supported.");

    uint8_t testLogBuffer[10];
    char buffer[50];

    HAPRawBufferZero(buffer, sizeof buffer);
    HAPRawBufferZero(testLogBuffer, sizeof testLogBuffer);

    diagnosticsContext.logBuffer = testLogBuffer;
    diagnosticsContext.logBufferSizeBytes = sizeof testLogBuffer;
    HAPCircularQueueCreate(
            &(diagnosticsContext.logBufferQueue), diagnosticsContext.logBuffer, diagnosticsContext.logBufferSizeBytes);

    accessoryDiagnosticsConfig.diagnosticsContext = &diagnosticsContext;
    HAPPlatformDiagnosticsInitialize(&accessoryDiagnosticsConfig);

    HAPError err = HAPStringWithFormat(buffer, sizeof buffer, "%s", "TestMsg-1");
    HAPAssert(!err);
    HAPPlatformDiagnosticsWriteToLogBuffer("%s,%s", buffer, buffer);

    HAPAssert(diagnosticsContext.logBufferQueue.totalSize == sizeof testLogBuffer);
    HAPAssert(diagnosticsContext.logBufferQueue.usedSpace == 0);
    HAPAssert(diagnosticsContext.logBufferQueue.remainingSpace == sizeof testLogBuffer);
    HAPPlatformDiagnosticsDeinitialize();
}

static void RemoveDiagnosticsFiles() {
    HAPLog(&kHAPLog_Default, "Removing Diagnostics files and folder");

    HAPError err;
    err = HAPPlatformDeleteAllDiagnosticsLogFiles(&diagnosticsContext.logContext);
    HAPAssert(!err);

    remove(diagnosticsFolderName);
    remove(kHAPPlatformDiagnosticsZipFileName);
}

int main() {
    HAPLog(&kHAPLog_Default, "Starting test accessory.");
    HAPPlatformCreate();

    DataSendTestContext test;
    HAPRawBufferZero(&test, sizeof test);

    accessory.callbacks.diagnosticsConfig.getDiagnosticsConfig = GetAccessoryDiagnosticsConfig;

    // Prepare key-value store.
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    HAPControllerPublicKey controllerPublicKey;
    TestPrepareKeyValueStore(controllerPairingID, &controllerPublicKey);

    // Prepare accessory server storage.
    HAPIPSession ipSessions[kHAPIPSessionStorage_DefaultNumElements];
    uint8_t ipInboundBuffers[HAPArrayCount(ipSessions)][kHAPIPSession_DefaultInboundBufferSize];
    uint8_t ipOutboundBuffers[HAPArrayCount(ipSessions)][kHAPIPSession_DefaultOutboundBufferSize];
    HAPIPEventNotification ipEventNotifications[HAPArrayCount(ipSessions)][kAttributeCount];
    for (size_t i = 0; i < HAPArrayCount(ipSessions); i++) {
        ipSessions[i].inboundBuffer.bytes = ipInboundBuffers[i];
        ipSessions[i].inboundBuffer.numBytes = sizeof ipInboundBuffers[i];
        ipSessions[i].outboundBuffer.bytes = ipOutboundBuffers[i];
        ipSessions[i].outboundBuffer.numBytes = sizeof ipOutboundBuffers[i];
        ipSessions[i].eventNotifications = ipEventNotifications[i];
        ipSessions[i].numEventNotifications = HAPArrayCount(ipEventNotifications[i]);
    }
    HAPIPReadContext ipReadContexts[kAttributeCount];
    HAPIPWriteContext ipWriteContexts[kAttributeCount];
    uint8_t ipScratchBuffer[kHAPIPSession_DefaultScratchBufferSize];
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
    HAPPlatformClockAdvance(0);
    HAPAssert(HAPAccessoryServerGetState(&accessoryServer) == kHAPAccessoryServerState_Running);

    // Create fake security session.
    HAPSession session;
    TestCreateFakeSecuritySession(&session, &accessoryServer, controllerPairingID);

    // Check HomeKit Data Stream Version.
    DataStreamTestHelpersCheckVersion(&accessoryServer, &accessory, &session, &test);

    accessoryDiagnosticsConfig.diagnosticsContext = &diagnosticsContext;

    // SupportedDiagnosticsSnapshot characteristic read handler
    TestSupportedDiagnosticsSnapshot(&session, &accessoryServer);

    // Run unit tests
    Test1_LogCapture();
    Test2_InvalidLogFileSize();
    Test3_DiagnosticsSendFile(&accessoryServer, &session, &test, &dataStreamTCPStreamManager);
    Test4_LogBufferWrites();
    Test5_LogBufferOverflowWrite();
    Test6_LogBufferLargeBufferWrite();
    RemoveDiagnosticsFiles();

    // Stop accessory server.
    HAPLog(&kHAPLog_Default, "Stopping accessory server.");
    HAPAccessoryServerForceStop(&accessoryServer);
    HAPPlatformClockAdvance(0);
    HAPPlatformClockAdvance(0);
    HAPAssert(HAPAccessoryServerGetState(&accessoryServer) == kHAPAccessoryServerState_Idle);
}
#else
int main() {
    HAPLogInfo(&kHAPLog_Default, "This test is not supported");
    return 0;
}
#endif // HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)
