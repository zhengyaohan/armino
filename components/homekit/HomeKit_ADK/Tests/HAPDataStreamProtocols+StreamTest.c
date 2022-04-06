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

#include "HAPOPACK.h"

#include "Harness/DataStreamTestHelpers.h"
#include "Harness/HAPAccessoryServerHelper.h"
#include "Harness/HAPPlatformClockHelper.h"
#include "Harness/IPTestHelpers.h"
#include "Harness/SecurityTestHelpers.h"

#define kStreamMaxSends            2
#define kStreamAppProtocolDataSize 256
#define kStreamMaxFrameSize        1024

HAP_ENUM_BEGIN(uint8_t, TestControllerIndex) {
    kAdminControllerIndex = 0,
    kNonAdminControllerIndex = 1,
    kNumControllers = 2,
} HAP_ENUM_END(uint8_t, TestControllerIndex);

#define kNumDataStreams (kNumControllers)

typedef struct {
    uint8_t scratchBytes[kStreamMaxFrameSize];
    bool inUse;
} StreamTestSend;

typedef struct {
    HAPStreamDataStreamApplicationProtocolDataSendCompletionHandler completionHandler;
    StreamTestSend send[kStreamMaxSends];
    bool streamValid : 1;
} StreamTestAppProtocolContext;

typedef struct {
    StreamTestAppProtocolContext loopback[kNumDataStreams];
    StreamTestAppProtocolContext reverse[kNumDataStreams];
} StreamTestContext;

typedef struct {
    DataStreamEncryptionContext encryptionContext;
    HAPPlatformTCPStreamRef clientTCPStream;
} DataStreamContext;

//----------------------------------------------------------------------------------------------------------------------
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
#if HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)
                                                                            &diagnosticsService,
#endif
                                                                            NULL },
                                  .callbacks = { .identify = IdentifyAccessoryHelper } };

//----------------------------------------------------------------------------------------------------------------------

#define kStreamAppProtocolName_Loopback "loopback"
#define kStreamRxBufferSize_Loopback    512

static void LoopbackDataSendCompletionHandler(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPStreamApplicationProtocol* streamAppProtocol,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPError error,
        void* scratchBytes,
        size_t numScratchBytes,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(streamAppProtocol);
    HAPPrecondition(request);
    HAPPrecondition(dataStream < kNumDataStreams);
    HAPPrecondition(!error);
    HAPPrecondition(context);

    StreamTestContext* test = (StreamTestContext*) context;
    HAPAssert(HAPStringAreEqual(streamAppProtocol->name, kStreamAppProtocolName_Loopback));

    StreamTestSend* send;
    for (int i = 0; i < kStreamMaxSends; i++) {
        send = &test->loopback[dataStream].send[i];
        if (send->scratchBytes == scratchBytes) {
            break;
        }
    }
    HAPAssert(send->scratchBytes == scratchBytes);
    HAPAssert(send->inUse);
    send->inUse = false;
    HAPLogInfo(&kHAPLog_Default, "Loopback data send (%p) complete", scratchBytes);
};

static void LoopbackReceiveData(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPStreamApplicationProtocol* streamAppProtocol,
        const HAPServiceRequest* request,
        HAPDataStreamHandle dataStream,
        void* _Nullable context,
        void* dataBytes,
        size_t numDataBytes) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(streamAppProtocol);
    HAPPrecondition(request);
    HAPPrecondition(dataStream < kNumDataStreams);
    HAPPrecondition(context);
    HAPPrecondition(dataBytes);

    StreamTestContext* test = (StreamTestContext*) context;
    HAPAssert(HAPStringAreEqual(streamAppProtocol->name, kStreamAppProtocolName_Loopback));

    HAPLogBufferDebug(&kHAPLog_Default, dataBytes, numDataBytes, "Loopback receive data");

    // Loopback the same data.
    StreamTestSend* send;
    for (int i = 0; i < kStreamMaxSends; i++) {
        send = &test->loopback[dataStream].send[i];
        if (!send->inUse) {
            break;
        }
    }
    HAPAssert(!send->inUse);
    send->inUse = true;

    HAPError err = HAPStreamDataStreamProtocolSendData(
            server,
            dispatcher,
            streamAppProtocol,
            dataStream,
            send->scratchBytes,
            sizeof send->scratchBytes,
            dataBytes,
            numDataBytes,
            test->loopback[dataStream].completionHandler);
    HAPAssert(!err);
    HAPLogInfo(&kHAPLog_Default, "Loopback data send (%p)", send->scratchBytes);
};

static void LoopbackHandleAccept(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPStreamApplicationProtocol* streamAppProtocol,
        const HAPServiceRequest* request,
        HAPDataStreamHandle dataStream,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(streamAppProtocol);
    HAPPrecondition(request);
    HAPPrecondition(dataStream < kNumDataStreams);
    HAPPrecondition(context);

    StreamTestContext* test = (StreamTestContext*) context;
    HAPAssert(HAPStringAreEqual(streamAppProtocol->name, kStreamAppProtocolName_Loopback));

    HAPAssert(!test->loopback[dataStream].streamValid);
    test->loopback[dataStream].streamValid = true;
    test->loopback[dataStream].completionHandler = LoopbackDataSendCompletionHandler;

    HAPLogInfo(&kHAPLog_Default, "Loopback Accept on Data Stream %d", dataStream);
}

static void LoopbackHandleInvalidate(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPStreamApplicationProtocol* streamAppProtocol,
        const HAPServiceRequest* request,
        HAPDataStreamHandle dataStream,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(streamAppProtocol);
    HAPPrecondition(request);
    HAPPrecondition(dataStream < kNumDataStreams);
    HAPPrecondition(context);

    StreamTestContext* test = (StreamTestContext*) context;
    HAPAssert(HAPStringAreEqual(streamAppProtocol->name, kStreamAppProtocolName_Loopback));

    test->loopback[dataStream].streamValid = false;

    HAPLogInfo(&kHAPLog_Default, "Loopback Invalidate on Data Stream %d", dataStream);
}

//----------------------------------------------------------------------------------------------------------------------

#define kStreamAppProtocolName_Reverse "reverse"
#define kStreamRxBufferSize_Reverse    512

static void ReverseDataSendCompletionHandler(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPStreamApplicationProtocol* streamAppProtocol,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPError error,
        void* scratchBytes,
        size_t numScratchBytes,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(streamAppProtocol);
    HAPPrecondition(request);
    HAPPrecondition(dataStream < kNumDataStreams);
    HAPPrecondition(!error);
    HAPPrecondition(context);

    StreamTestContext* test = (StreamTestContext*) context;
    HAPAssert(HAPStringAreEqual(streamAppProtocol->name, kStreamAppProtocolName_Reverse));

    StreamTestSend* send;
    for (int i = 0; i < kStreamMaxSends; i++) {
        send = &test->reverse[dataStream].send[i];
        if (send->scratchBytes == scratchBytes) {
            break;
        }
    }
    HAPAssert(send->scratchBytes == scratchBytes);
    HAPAssert(send->inUse);
    send->inUse = false;
    HAPLogInfo(&kHAPLog_Default, "Reverse data send (%p) complete", scratchBytes);
};

static void ReverseReceiveData(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPStreamApplicationProtocol* streamAppProtocol,
        const HAPServiceRequest* request,
        HAPDataStreamHandle dataStream,
        void* _Nullable context,
        void* dataBytes,
        size_t numDataBytes) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(streamAppProtocol);
    HAPPrecondition(request);
    HAPPrecondition(dataStream < kNumDataStreams);
    HAPPrecondition(context);
    HAPPrecondition(dataBytes);

    StreamTestContext* test = (StreamTestContext*) context;
    HAPAssert(HAPStringAreEqual(streamAppProtocol->name, kStreamAppProtocolName_Reverse));

    HAPLogBufferDebug(&kHAPLog_Default, dataBytes, numDataBytes, "Reverse receive data");

    // Reverse the data and send it back.
    uint8_t revDataBytes[numDataBytes];
    for (int i = numDataBytes; i > 0; i--) {
        revDataBytes[numDataBytes - i] = *((uint8_t*) dataBytes + i - 1);
    }

    StreamTestSend* send;
    for (int i = 0; i < kStreamMaxSends; i++) {
        send = &test->reverse[dataStream].send[i];
        if (!send->inUse) {
            break;
        }
    }
    HAPAssert(!send->inUse);
    send->inUse = true;

    HAPError err = HAPStreamDataStreamProtocolSendData(
            server,
            dispatcher,
            streamAppProtocol,
            dataStream,
            send->scratchBytes,
            sizeof send->scratchBytes,
            revDataBytes,
            numDataBytes,
            test->reverse[dataStream].completionHandler);
    HAPAssert(!err);
    HAPLogInfo(&kHAPLog_Default, "Reverse data send (%p)", send->scratchBytes);
};

static void ReverseHandleAccept(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPStreamApplicationProtocol* streamAppProtocol,
        const HAPServiceRequest* request,
        HAPDataStreamHandle dataStream,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(streamAppProtocol);
    HAPPrecondition(request);
    HAPPrecondition(dataStream < kNumDataStreams);
    HAPPrecondition(context);

    StreamTestContext* test = (StreamTestContext*) context;
    HAPAssert(HAPStringAreEqual(streamAppProtocol->name, kStreamAppProtocolName_Reverse));

    HAPAssert(!test->reverse[dataStream].streamValid);
    test->reverse[dataStream].streamValid = true;
    test->reverse[dataStream].completionHandler = ReverseDataSendCompletionHandler;

    HAPLogInfo(&kHAPLog_Default, "Reverse Accept on Data Stream %d", dataStream);
}

static void ReverseHandleInvalidate(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPStreamApplicationProtocol* streamAppProtocol,
        const HAPServiceRequest* request,
        HAPDataStreamHandle dataStream,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(streamAppProtocol);
    HAPPrecondition(request);
    HAPPrecondition(dataStream < kNumDataStreams);
    HAPPrecondition(context);

    StreamTestContext* test = (StreamTestContext*) context;
    HAPAssert(HAPStringAreEqual(streamAppProtocol->name, kStreamAppProtocolName_Reverse));

    test->reverse[dataStream].streamValid = false;

    HAPLogInfo(&kHAPLog_Default, "Reverse Invalidate on Data Stream %d", dataStream);
}

//----------------------------------------------------------------------------------------------------------------------

static HAPStreamDataStreamApplicationProtocolContext loopbackProtocolContexts[kApp_NumDataStreams];
static HAPStreamDataStreamApplicationProtocolSendContext loopbackProtocolSendContexts[kApp_NumDataStreams];
static uint8_t loopbackRxBuffer[kStreamRxBufferSize_Loopback * kNumDataStreams];
static HAPStreamApplicationProtocol loopbackProtocol = {
    .name = kStreamAppProtocolName_Loopback,
    .storage = { .streamAppProtocolContexts = loopbackProtocolContexts,
                 .streamAppProtocolSendContexts = loopbackProtocolSendContexts,
                 .numSendContexts = HAPArrayCount(loopbackProtocolSendContexts),
                 .rxBuffers = loopbackRxBuffer,
                 .rxBufferSize = kStreamRxBufferSize_Loopback,
                 .numRxBuffers = kNumDataStreams },
    .config = { .maxSendContextsPerDataStream = kStreamMaxSends, .requiresAdminPermissions = false },
    .callbacks = { .handleAccept = LoopbackHandleAccept,
                   .handleData = LoopbackReceiveData,
                   .handleInvalidate = LoopbackHandleInvalidate }
};

static HAPStreamDataStreamApplicationProtocolContext reverseProtocolContexts[kApp_NumDataStreams];
static HAPStreamDataStreamApplicationProtocolSendContext reverseProtocolSendContexts[kApp_NumDataStreams];
static uint8_t reverseRxBuffer[kStreamRxBufferSize_Reverse];
static HAPStreamApplicationProtocol reverseProtocol = {
    .name = kStreamAppProtocolName_Reverse,
    .storage = { .streamAppProtocolContexts = reverseProtocolContexts,
                 .streamAppProtocolSendContexts = reverseProtocolSendContexts,
                 .numSendContexts = HAPArrayCount(reverseProtocolSendContexts),
                 .rxBuffers = reverseRxBuffer,
                 .rxBufferSize = kStreamRxBufferSize_Reverse,
                 .numRxBuffers = 1 },
    .config = { .maxSendContextsPerDataStream = kStreamMaxSends, .requiresAdminPermissions = true },
    .callbacks = { .handleAccept = ReverseHandleAccept,
                   .handleData = ReverseReceiveData,
                   .handleInvalidate = ReverseHandleInvalidate }
};

static HAPStreamDataStreamProtocol streamDataStreamProtocol = {
    .base = &kHAPStreamDataStreamProtocol_Base,
    .numDataStreams = kApp_NumDataStreams,
    .applicationProtocols = (HAPStreamApplicationProtocol* const[]) { &loopbackProtocol, &reverseProtocol, NULL }
};

static HAPDataStreamDescriptor dataStreamDescriptors[kApp_NumDataStreams];
static HAPDataStreamDispatcherStorage dataStreamDispatcherStorage = {
    .numDataStreams = kApp_NumDataStreams,
    .dataStreamDescriptors = dataStreamDescriptors,
    .dataStreamProtocols = (HAPDataStreamProtocol* const[]) { &streamDataStreamProtocol, NULL }
};
static HAPDataStreamDispatcher dataStreamDispatcher;

//----------------------------------------------------------------------------------------------------------------------

static void SendProtocolMessage(
        const char* appProtocolName,
        HAPPlatformTCPStreamManager* dataStreamTCPStreamManager,
        DataStreamContext* hdsContext,
        void* msg,
        size_t msgSize) {
    HAPLog(&kHAPLog_Default, "Sending stream.%s message.", appProtocolName);
    // clang-format off
    const uint8_t _header[] = {
            kHAPOPACKTag_Dictionary2,
            kHAPOPACKTag_String8,
            'p', 'r', 'o', 't', 'o', 'c', 'o', 'l',
            kHAPOPACKTag_String6,
            's', 't', 'r', 'e', 'a', 'm',
            kHAPOPACKTag_String5,
            'e', 'v', 'e', 'n', 't',
    };
    const uint8_t dataHeader[] = {
            kHAPOPACKTag_Dictionary1,
            kHAPOPACKTag_String4,
            'd', 'a', 't', 'a',
            kHAPOPACKTag_DataUInt16, HAPExpandLittleUInt16(msgSize)
    };
    // clang-format on
    {
        uint8_t appProtocolStringTag = kHAPOPACKTag_String0 + HAPStringGetNumBytes(appProtocolName);
        uint8_t header[sizeof _header + sizeof appProtocolStringTag + HAPStringGetNumBytes(appProtocolName)];
        HAPRawBufferCopyBytes(&header[0], _header, sizeof _header);
        HAPRawBufferCopyBytes(&header[sizeof _header], &appProtocolStringTag, sizeof appProtocolStringTag);
        HAPRawBufferCopyBytes(
                &header[sizeof _header + sizeof appProtocolStringTag],
                appProtocolName,
                HAPStringGetNumBytes(appProtocolName));
        uint8_t message[sizeof dataHeader + msgSize];
        HAPRawBufferCopyBytes(&message[0], dataHeader, sizeof dataHeader);
        HAPRawBufferCopyBytes(&message[sizeof dataHeader], msg, msgSize);
        uint8_t frame[HAP_DATASTREAM_ENC_FRAME_MIN_LENGTH + sizeof header + sizeof message];
        DataStreamTestHelpersDataStreamEncryptControllerPacketWithContext(
                &hdsContext->encryptionContext, frame, sizeof(frame), header, sizeof(header), message, sizeof(message));

        size_t numBytes;
        HAPError err = HAPPlatformTCPStreamClientWrite(
                dataStreamTCPStreamManager, hdsContext->clientTCPStream, frame, sizeof frame, &numBytes);
        HAPAssert(!err);
        HAPAssert(numBytes == sizeof frame);
    }
}

static void SendPartialProtocolMessage(
        const char* appProtocolName,
        HAPPlatformTCPStreamManager* dataStreamTCPStreamManager,
        DataStreamContext* hdsContext,
        void* msg,
        size_t msgSize,
        uint8_t* frameBuffer,
        size_t frameBufferSize,
        size_t partialSendSize,
        size_t* remainingFrameSize) {
    HAPLog(&kHAPLog_Default, "Sending partial stream.%s message.", appProtocolName);
    // clang-format off
    const uint8_t _header[] = {
            kHAPOPACKTag_Dictionary2,
            kHAPOPACKTag_String8,
            'p', 'r', 'o', 't', 'o', 'c', 'o', 'l',
            kHAPOPACKTag_String6,
            's', 't', 'r', 'e', 'a', 'm',
            kHAPOPACKTag_String5,
            'e', 'v', 'e', 'n', 't',
    };
    const uint8_t dataHeader[] = {
            kHAPOPACKTag_Dictionary1,
            kHAPOPACKTag_String4,
            'd', 'a', 't', 'a',
            kHAPOPACKTag_DataUInt16, HAPExpandLittleUInt16(msgSize)
    };
    // clang-format on
    {
        uint8_t appProtocolStringTag = kHAPOPACKTag_String0 + HAPStringGetNumBytes(appProtocolName);
        uint8_t header[sizeof _header + sizeof appProtocolStringTag + HAPStringGetNumBytes(appProtocolName)];
        HAPRawBufferCopyBytes(&header[0], _header, sizeof _header);
        HAPRawBufferCopyBytes(&header[sizeof _header], &appProtocolStringTag, sizeof appProtocolStringTag);
        HAPRawBufferCopyBytes(
                &header[sizeof _header + sizeof appProtocolStringTag],
                appProtocolName,
                HAPStringGetNumBytes(appProtocolName));
        uint8_t message[sizeof dataHeader + msgSize];
        HAPRawBufferCopyBytes(&message[0], dataHeader, sizeof dataHeader);
        HAPRawBufferCopyBytes(&message[sizeof dataHeader], msg, msgSize);
        size_t frameSize = HAP_DATASTREAM_ENC_FRAME_MIN_LENGTH + sizeof header + sizeof message;
        HAPAssert(frameBufferSize >= frameSize);
        DataStreamTestHelpersDataStreamEncryptControllerPacketWithContext(
                &hdsContext->encryptionContext,
                frameBuffer,
                frameSize,
                header,
                sizeof(header),
                message,
                sizeof(message));

        size_t numBytes;
        HAPError err = HAPPlatformTCPStreamClientWrite(
                dataStreamTCPStreamManager, hdsContext->clientTCPStream, frameBuffer, partialSendSize, &numBytes);
        HAPAssert(!err);
        HAPAssert(numBytes == partialSendSize);
        *remainingFrameSize = frameSize - partialSendSize;
    }
}

static void SendRemainingProtocolMessage(
        const char* appProtocolName,
        HAPPlatformTCPStreamManager* dataStreamTCPStreamManager,
        DataStreamContext* hdsContext,
        uint8_t* remainingFrame,
        size_t remainingFrameSize) {
    HAPLog(&kHAPLog_Default, "Sending remaining stream.%s message.", appProtocolName);
    size_t numBytes;
    HAPError err = HAPPlatformTCPStreamClientWrite(
            dataStreamTCPStreamManager, hdsContext->clientTCPStream, remainingFrame, remainingFrameSize, &numBytes);
    HAPAssert(!err);
    HAPAssert(numBytes == remainingFrameSize);
}

static void ReceiveAndValidateProtocolMessage(
        const char* appProtocolName,
        HAPPlatformTCPStreamManager* dataStreamTCPStreamManager,
        DataStreamContext* hdsContext,
        uint8_t* expectMsg,
        size_t expectMsgNumBytes) {
    HAPLog(&kHAPLog_Default, "Receiving stream.%s message.", appProtocolName);
    uint8_t frameBytes[kStreamMaxFrameSize];

    // Read the frame header to extract the message length.
    size_t numBytes;
    HAPError err = HAPPlatformTCPStreamClientRead(
            dataStreamTCPStreamManager,
            hdsContext->clientTCPStream,
            frameBytes,
            HAP_DATASTREAM_FRAME_HEADER_LENGTH,
            &numBytes);
    HAPAssert(!err);
    HAPAssert(numBytes == HAP_DATASTREAM_FRAME_HEADER_LENGTH);
    size_t frameLength = HAPReadBigUInt24(&((uint8_t*) frameBytes)[1]);
    HAPAssert(sizeof frameBytes >= HAP_DATASTREAM_FRAME_HEADER_LENGTH + frameLength + CHACHA20_POLY1305_TAG_BYTES);

    // Read the remainder of the message.
    err = HAPPlatformTCPStreamClientRead(
            dataStreamTCPStreamManager,
            hdsContext->clientTCPStream,
            &((uint8_t*) frameBytes)[HAP_DATASTREAM_FRAME_HEADER_LENGTH],
            frameLength + CHACHA20_POLY1305_TAG_BYTES,
            &numBytes);
    HAPAssert(!err);
    HAPAssert(numBytes == frameLength + CHACHA20_POLY1305_TAG_BYTES);

    uint8_t* header;
    uint8_t* payload;
    size_t headerSize;
    size_t payloadSize;
    DataStreamTestHelpersDataStreamDecryptAccessoryPacketWithContext(
            &hdsContext->encryptionContext,
            frameBytes,
            HAP_DATASTREAM_FRAME_HEADER_LENGTH + frameLength + CHACHA20_POLY1305_TAG_BYTES,
            &header,
            &headerSize,
            &payload,
            &payloadSize);

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
        HAPAssert(HAPStringAreEqual(protocolName, "stream"));

        // Topic.
        HAPAssert(eventElement.value.exists);
        char* topic;
        err = HAPOPACKReaderGetNextString(&eventElement.value.reader, &topic);
        HAPAssert(!err);
        HAPAssert(HAPStringAreEqual(topic, appProtocolName));
    }

    // Message.
    {
        HAPOPACKReader messageReader;
        HAPOPACKReaderCreate(&messageReader, payload, payloadSize);
        HAPOPACKStringDictionaryElement dataElement;
        dataElement.key = "data";
        err = HAPOPACKStringDictionaryReaderGetAll(
                &messageReader, (HAPOPACKStringDictionaryElement* const[]) { &dataElement, NULL });
        HAPAssert(!err);

        // Parse data.
        HAPAssert(dataElement.value.exists);
        uint8_t* msg;
        size_t msgNumBytes;
        err = HAPOPACKReaderGetNextData(&dataElement.value.reader, (void**) &msg, &msgNumBytes);
        HAPAssert(!err);

        // Validate data.
        HAPLogBufferDebug(&kHAPLog_Default, msg, msgNumBytes, "Receive message data");
        HAPAssert(msgNumBytes == expectMsgNumBytes);
        if (HAPStringAreEqual(kStreamAppProtocolName_Loopback, appProtocolName)) {
            for (size_t i = 0; i < msgNumBytes; i++) {
                HAPAssert(*(expectMsg + i) == *(msg + i));
            }
        } else if (HAPStringAreEqual(kStreamAppProtocolName_Reverse, appProtocolName)) {
            for (size_t i = 0; i < msgNumBytes; i++) {
                HAPAssert(*(expectMsg + msgNumBytes - i - 1) == *(msg + i));
            }
        } else {
            HAPFatalError();
        }
    }
}

static void InitializeDataStreamForController(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPPlatformTCPStreamManager* dataStreamTCPStreamManager,
        DataStreamContext* hdsContext,
        void* _Nullable context) {
    HAPError err;

    // Check HomeKit Data Stream Version.
    DataStreamTestHelpersCheckVersion(server, &accessory, session, context);

    // Get Supported Data Stream Transport Configuration.
    bool supportsTCP;
    bool supportsHAP;
    DataStreamTestHelpersReadSupportedDataStreamTransportConfiguration(
            server, &accessory, session, &supportsTCP, &supportsHAP, context);
    HAPAssert(!supportsHAP);
    HAPAssert(supportsTCP);

    // Generate Controller Key Salt.
    HAPDataStreamSalt controllerKeySalt;
    HAPPlatformRandomNumberFill(controllerKeySalt.bytes, sizeof controllerKeySalt.bytes);

    // Setup HomeKit Data Stream Transport.
    err = DataStreamTestHelpersWriteSetupDataStreamTransport(server, &accessory, session, &controllerKeySalt, NULL);
    HAPAssert(!err);
    uint8_t setupStatus;
    HAPNetworkPort listeningPort;
    HAPDataStreamSalt accessoryKeySalt;
    HAPDataStreamHAPSessionIdentifier sessionIdentifier;
    DataStreamTestHelpersReadSetupDataStreamTransport(
            server,
            &accessory,
            session,
            (HAPCharacteristicValue_SetupDataStreamTransport_Status*) &setupStatus,
            &listeningPort,
            &accessoryKeySalt,
            &sessionIdentifier,
            NULL);
    HAPAssert(sessionIdentifier == kHAPDataStreamHAPSessionIdentifierNone);
    HAPAssert(setupStatus == 0);
    HAPAssert(listeningPort == HAPPlatformTCPStreamManagerGetListenerPort(dataStreamTCPStreamManager));
    HAPAssert(!HAPRawBufferIsZero(accessoryKeySalt.bytes, sizeof(accessoryKeySalt.bytes)));

    // Derive encryption keys.
    DataStreamTestHelpersDataStreamInitEncryptionContext(
            session, &controllerKeySalt, &accessoryKeySalt, &hdsContext->encryptionContext);

    // Connect HomeKit Data Stream.
    HAPLog(&kHAPLog_Default, "Connecting HomeKit Data Stream.");
    err = HAPPlatformTCPStreamManagerConnectToListener(dataStreamTCPStreamManager, &hdsContext->clientTCPStream);
    HAPAssert(!err);

    // Send control.hello request.
    DataStreamTestHelpersSendHelloRequest(
            dataStreamTCPStreamManager, &hdsContext->clientTCPStream, &hdsContext->encryptionContext);
    // Receive response for control.hello request.
    DataStreamTestHelpersReceiveHelloResponse(
            dataStreamTCPStreamManager, &hdsContext->clientTCPStream, &hdsContext->encryptionContext);
}

//----------------------------------------------------------------------------------------------------------------------

int main() {
    HAPError err;
    HAPPlatformCreate();

    StreamTestContext test;
    HAPRawBufferZero(&test, sizeof test);

    // Prepare key-value store.
    HAPPlatformKeyValueStoreKey controllerPairingID[kNumControllers] = { 0, 1 };
    HAPControllerPublicKey controllerPublicKey;
    TestPrepareKeyValueStore(controllerPairingID[0], &controllerPublicKey);
    TestKeyValueStoreAddControllerPairing(controllerPairingID[1], &controllerPublicKey, false);

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

    // Create fake security sessions.
    HAPSession session[kNumControllers];
    for (int i = 0; i < kNumControllers; i++) {
        TestCreateFakeSecuritySession(&session[i], &accessoryServer, controllerPairingID[i]);
    }

    // Open a data stream for each controller.
    DataStreamContext hdsContext[kNumControllers];
    for (int i = 0; i < kNumControllers; i++) {
        InitializeDataStreamForController(
                &accessoryServer, &session[i], &dataStreamTCPStreamManager, &hdsContext[i], &test);
    }

    // Verify loopback (non-admin) stream application protocol accept handler was called on all streams.
    HAPAssert(test.loopback[kAdminControllerIndex].streamValid);
    HAPAssert(test.loopback[kNonAdminControllerIndex].streamValid);

    // Verify reverse (admin) stream application protocol accept handler was called only for admin controller stream.
    HAPAssert(test.reverse[kAdminControllerIndex].streamValid);
    HAPAssert(!test.reverse[kNonAdminControllerIndex].streamValid);

    // Prepare data buffer with unique chunks of size kStreamAppProtocolDataSize
    uint8_t data[kStreamAppProtocolDataSize * kStreamMaxSends];
    uint8_t offset = 0;
    for (size_t i = 0; i < sizeof data; i++) {
        data[i] = (uint8_t)((i & 0xFF) + offset);
        if ((i + 1) % kStreamAppProtocolDataSize == 0) {
            offset++;
        }
    }
    HAPLogBufferDebug(&kHAPLog_Default, data, sizeof data, "test data buffer");

    // Send and receive 1 loopback message.
    SendProtocolMessage(
            kStreamAppProtocolName_Loopback,
            &dataStreamTCPStreamManager,
            &hdsContext[kAdminControllerIndex],
            data,
            kStreamAppProtocolDataSize);
    ReceiveAndValidateProtocolMessage(
            kStreamAppProtocolName_Loopback,
            &dataStreamTCPStreamManager,
            &hdsContext[kAdminControllerIndex],
            data,
            kStreamAppProtocolDataSize);

    // Send and receive 1 reverse message.
    SendProtocolMessage(
            kStreamAppProtocolName_Reverse,
            &dataStreamTCPStreamManager,
            &hdsContext[kAdminControllerIndex],
            data,
            kStreamAppProtocolDataSize);
    ReceiveAndValidateProtocolMessage(
            kStreamAppProtocolName_Reverse,
            &dataStreamTCPStreamManager,
            &hdsContext[kAdminControllerIndex],
            data,
            kStreamAppProtocolDataSize);

    // Send 1 loopback message.
    SendProtocolMessage(
            kStreamAppProtocolName_Loopback,
            &dataStreamTCPStreamManager,
            &hdsContext[kAdminControllerIndex],
            data,
            kStreamAppProtocolDataSize);
    // Send 1 reverse message.
    SendProtocolMessage(
            kStreamAppProtocolName_Reverse,
            &dataStreamTCPStreamManager,
            &hdsContext[kAdminControllerIndex],
            data,
            kStreamAppProtocolDataSize);
    // Receive 1 loopback message.
    ReceiveAndValidateProtocolMessage(
            kStreamAppProtocolName_Loopback,
            &dataStreamTCPStreamManager,
            &hdsContext[kAdminControllerIndex],
            data,
            kStreamAppProtocolDataSize);
    // Receive 1 reverse message.
    ReceiveAndValidateProtocolMessage(
            kStreamAppProtocolName_Reverse,
            &dataStreamTCPStreamManager,
            &hdsContext[kAdminControllerIndex],
            data,
            kStreamAppProtocolDataSize);

    // Send kStreamMaxSends loopback messages.
    for (int i = 0; i < kStreamMaxSends; i++) {
        SendProtocolMessage(
                kStreamAppProtocolName_Loopback,
                &dataStreamTCPStreamManager,
                &hdsContext[kAdminControllerIndex],
                &data[i * kStreamAppProtocolDataSize],
                kStreamAppProtocolDataSize);
    }
    // Receive kStreamMaxSends loopback messages.
    for (int i = 0; i < kStreamMaxSends; i++) {
        ReceiveAndValidateProtocolMessage(
                kStreamAppProtocolName_Loopback,
                &dataStreamTCPStreamManager,
                &hdsContext[kAdminControllerIndex],
                &data[i * kStreamAppProtocolDataSize],
                kStreamAppProtocolDataSize);
    }

    // Send kStreamMaxSends reverse messages.
    for (int i = 0; i < kStreamMaxSends; i++) {
        SendProtocolMessage(
                kStreamAppProtocolName_Reverse,
                &dataStreamTCPStreamManager,
                &hdsContext[kAdminControllerIndex],
                &data[i * kStreamAppProtocolDataSize],
                kStreamAppProtocolDataSize);
    }
    // Receive kStreamMaxSends reverse messages.
    for (int i = 0; i < kStreamMaxSends; i++) {
        ReceiveAndValidateProtocolMessage(
                kStreamAppProtocolName_Reverse,
                &dataStreamTCPStreamManager,
                &hdsContext[kAdminControllerIndex],
                &data[i * kStreamAppProtocolDataSize],
                kStreamAppProtocolDataSize);
    }

    // Send kStreamMaxSends loopback messages.
    for (int i = 0; i < kStreamMaxSends; i++) {
        SendProtocolMessage(
                kStreamAppProtocolName_Loopback,
                &dataStreamTCPStreamManager,
                &hdsContext[kAdminControllerIndex],
                &data[i * kStreamAppProtocolDataSize],
                kStreamAppProtocolDataSize);
    }
    // Send kStreamMaxSends reverse messages.
    for (int i = 0; i < kStreamMaxSends; i++) {
        SendProtocolMessage(
                kStreamAppProtocolName_Reverse,
                &dataStreamTCPStreamManager,
                &hdsContext[kAdminControllerIndex],
                &data[i * kStreamAppProtocolDataSize],
                kStreamAppProtocolDataSize);
    }
    // Receive kStreamMaxSends loopback messages.
    for (int i = 0; i < kStreamMaxSends; i++) {
        ReceiveAndValidateProtocolMessage(
                kStreamAppProtocolName_Loopback,
                &dataStreamTCPStreamManager,
                &hdsContext[kAdminControllerIndex],
                &data[i * kStreamAppProtocolDataSize],
                kStreamAppProtocolDataSize);
    }
    // Receive kStreamMaxSends reverse messages.
    for (int i = 0; i < kStreamMaxSends; i++) {
        ReceiveAndValidateProtocolMessage(
                kStreamAppProtocolName_Reverse,
                &dataStreamTCPStreamManager,
                &hdsContext[kAdminControllerIndex],
                &data[i * kStreamAppProtocolDataSize],
                kStreamAppProtocolDataSize);
    }

    // Verify protocol messages which require admin permissions are skipped on data streams opened by non-admin
    // controllers.
    SendProtocolMessage(
            kStreamAppProtocolName_Reverse,
            &dataStreamTCPStreamManager,
            &hdsContext[kNonAdminControllerIndex],
            data,
            kStreamAppProtocolDataSize);
    HAPLog(&kHAPLog_Default, "Receiving stream.reverse event (expected fail).");
    {
        uint8_t frameBytes[kStreamMaxFrameSize];
        size_t numBytes;
        err = HAPPlatformTCPStreamClientRead(
                &dataStreamTCPStreamManager,
                hdsContext[kNonAdminControllerIndex].clientTCPStream,
                frameBytes,
                sizeof frameBytes,
                &numBytes);
        HAPAssert(err == kHAPError_Busy);
    }

    // Verify protocol messages can be received into protocol rx buffers on multiple data streams simultaneously.
    {
        uint8_t frame[kStreamMaxFrameSize];
        size_t remainingFrameSize;
        // Send partial loopback message on data stream 0.
        SendPartialProtocolMessage(
                kStreamAppProtocolName_Loopback,
                &dataStreamTCPStreamManager,
                &hdsContext[kAdminControllerIndex],
                data,
                kStreamAppProtocolDataSize,
                &frame[0],
                HAPArrayCount(frame),
                kStreamAppProtocolDataSize - 1,
                &remainingFrameSize);
        // Send full loopback message on data stream 1.
        SendProtocolMessage(
                kStreamAppProtocolName_Loopback,
                &dataStreamTCPStreamManager,
                &hdsContext[kNonAdminControllerIndex],
                &data[kStreamAppProtocolDataSize],
                kStreamAppProtocolDataSize);
        // Send remaining loopback message on data stream 0.
        SendRemainingProtocolMessage(
                kStreamAppProtocolName_Loopback,
                &dataStreamTCPStreamManager,
                &hdsContext[kAdminControllerIndex],
                &frame[kStreamAppProtocolDataSize - 1],
                remainingFrameSize);
        // Receive and validate loopback messages on both data streams.
        ReceiveAndValidateProtocolMessage(
                kStreamAppProtocolName_Loopback,
                &dataStreamTCPStreamManager,
                &hdsContext[kAdminControllerIndex],
                data,
                kStreamAppProtocolDataSize);
        ReceiveAndValidateProtocolMessage(
                kStreamAppProtocolName_Loopback,
                &dataStreamTCPStreamManager,
                &hdsContext[kNonAdminControllerIndex],
                &data[kStreamAppProtocolDataSize],
                kStreamAppProtocolDataSize);
    }

    // Provide a single loopback rx buffer and confirm secondary message is skipped while in use.
    loopbackProtocol.storage.numRxBuffers = 1;
    {
        uint8_t frame[kStreamMaxFrameSize];
        size_t remainingFrameSize;
        // Send partial loopback message on data stream 0.
        SendPartialProtocolMessage(
                kStreamAppProtocolName_Loopback,
                &dataStreamTCPStreamManager,
                &hdsContext[kAdminControllerIndex],
                data,
                kStreamAppProtocolDataSize,
                &frame[0],
                HAPArrayCount(frame),
                kStreamAppProtocolDataSize - 1,
                &remainingFrameSize);
        // Send full loopback message on data stream 1.
        SendProtocolMessage(
                kStreamAppProtocolName_Loopback,
                &dataStreamTCPStreamManager,
                &hdsContext[kNonAdminControllerIndex],
                &data[kStreamAppProtocolDataSize],
                kStreamAppProtocolDataSize);
        // Send remaining loopback message on data stream 0.
        SendRemainingProtocolMessage(
                kStreamAppProtocolName_Loopback,
                &dataStreamTCPStreamManager,
                &hdsContext[kAdminControllerIndex],
                &frame[kStreamAppProtocolDataSize - 1],
                remainingFrameSize);
        // Receive and validate loopback message on data stream 0.
        ReceiveAndValidateProtocolMessage(
                kStreamAppProtocolName_Loopback,
                &dataStreamTCPStreamManager,
                &hdsContext[kAdminControllerIndex],
                data,
                kStreamAppProtocolDataSize);
        // Message on data stream 1 was skipped. Confirm stream read returns busy.
        {
            uint8_t frameBytes[kStreamMaxFrameSize];
            size_t numBytes;
            err = HAPPlatformTCPStreamClientRead(
                    &dataStreamTCPStreamManager,
                    hdsContext[kNonAdminControllerIndex].clientTCPStream,
                    frameBytes,
                    sizeof frameBytes,
                    &numBytes);
            HAPAssert(err == kHAPError_Busy);
        }
    }

    // Verify protocol messages can be received into a single protocol rx buffer and a data stream shared buffer
    // simultaneously.
    {
        uint8_t frame[kStreamMaxFrameSize];
        size_t remainingFrameSize;
        // Send partial loopback message on data stream 0.
        SendPartialProtocolMessage(
                kStreamAppProtocolName_Loopback,
                &dataStreamTCPStreamManager,
                &hdsContext[kAdminControllerIndex],
                data,
                kStreamAppProtocolDataSize,
                &frame[0],
                HAPArrayCount(frame),
                kStreamAppProtocolDataSize - 1,
                &remainingFrameSize);
        // Send full loopback message on data stream 1, which fits into shared buffer.
        SendProtocolMessage(
                kStreamAppProtocolName_Loopback,
                &dataStreamTCPStreamManager,
                &hdsContext[kNonAdminControllerIndex],
                &data[kStreamAppProtocolDataSize],
                kHAPDataStreamDispatcher_NumShortMessageBytes - kHAPStreamDataStreamProtocol_RxMsgHeaderBytes);
        // Send remaining loopback message on data stream 0.
        SendRemainingProtocolMessage(
                kStreamAppProtocolName_Loopback,
                &dataStreamTCPStreamManager,
                &hdsContext[kAdminControllerIndex],
                &frame[kStreamAppProtocolDataSize - 1],
                remainingFrameSize);
        // Receive and validate loopback messages on both data streams.
        ReceiveAndValidateProtocolMessage(
                kStreamAppProtocolName_Loopback,
                &dataStreamTCPStreamManager,
                &hdsContext[kAdminControllerIndex],
                data,
                kStreamAppProtocolDataSize);
        ReceiveAndValidateProtocolMessage(
                kStreamAppProtocolName_Loopback,
                &dataStreamTCPStreamManager,
                &hdsContext[kNonAdminControllerIndex],
                &data[kStreamAppProtocolDataSize],
                kHAPDataStreamDispatcher_NumShortMessageBytes - kHAPStreamDataStreamProtocol_RxMsgHeaderBytes);
    }

    // Remove loopback rx buffer to validate short event messages without protocol rx buffer.
    loopbackProtocol.storage.rxBuffers = NULL;
    SendProtocolMessage(
            kStreamAppProtocolName_Loopback,
            &dataStreamTCPStreamManager,
            &hdsContext[kAdminControllerIndex],
            data,
            kHAPDataStreamDispatcher_NumShortMessageBytes - kHAPStreamDataStreamProtocol_RxMsgHeaderBytes);
    ReceiveAndValidateProtocolMessage(
            kStreamAppProtocolName_Loopback,
            &dataStreamTCPStreamManager,
            &hdsContext[kAdminControllerIndex],
            data,
            kHAPDataStreamDispatcher_NumShortMessageBytes - kHAPStreamDataStreamProtocol_RxMsgHeaderBytes);

    // Messages larger than the data stream shared buffer length should be skipped.
    SendProtocolMessage(
            kStreamAppProtocolName_Loopback,
            &dataStreamTCPStreamManager,
            &hdsContext[kAdminControllerIndex],
            data,
            kStreamAppProtocolDataSize);
    HAPLog(&kHAPLog_Default, "Receiving stream.loopback event (expected fail).");
    {
        uint8_t frameBytes[kStreamMaxFrameSize];
        size_t numBytes;
        err = HAPPlatformTCPStreamClientRead(
                &dataStreamTCPStreamManager,
                hdsContext[kAdminControllerIndex].clientTCPStream,
                frameBytes,
                sizeof frameBytes,
                &numBytes);
        HAPAssert(err == kHAPError_Busy);
    }

    // Stop accessory server.
    HAPLog(&kHAPLog_Default, "Stopping accessory server.");
    HAPAccessoryServerForceStop(&accessoryServer);
    HAPPlatformClockAdvance(0);
    HAPPlatformClockAdvance(0);
    HAPAssert(HAPAccessoryServerGetState(&accessoryServer) == kHAPAccessoryServerState_Idle);

    // Verify stream application protocol invalidate handlers were called.
    for (int i = 0; i < kNumDataStreams; i++) {
        HAPAssert(!test.loopback[i].streamValid);
        HAPAssert(!test.reverse[i].streamValid);
    }

    // Check that TCP stream has been closed.
    {
        uint8_t bytes[kStreamMaxFrameSize];
        size_t numBytes;
        for (size_t i = 0; i < HAPArrayCount(hdsContext); i++) {
            err = HAPPlatformTCPStreamClientRead(
                    &dataStreamTCPStreamManager, hdsContext[i].clientTCPStream, bytes, sizeof bytes, &numBytes);
            HAPAssert(!err);
            HAPAssert(!numBytes);
            HAPPlatformTCPStreamManagerClientClose(&dataStreamTCPStreamManager, hdsContext[i].clientTCPStream);
        }
    }

    // Ensure TCP stream manager was properly cleaned up.
    HAPAssert(!HAPPlatformTCPStreamManagerIsListenerOpen(&dataStreamTCPStreamManager));
    return 0;
}
