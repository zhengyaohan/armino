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

#include "HAPPlatformFeatures.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_UARP_SUPPORT)

#include "Harness/UARP.h"

#include "Harness/DataStreamTestHelpers.h"
#include "Harness/HAPAccessoryServerHelper.h"
#include "Harness/HAPPlatformClockHelper.h"
#include "Harness/IPTestHelpers.h"
#include "Harness/SecurityTestHelpers.h"
#include "Harness/UnitTestFramework.h"

#include "HAPOPACK.h"

#include "CoreUARPPlatform.h"

#define kAssetTag                   0xABCD0123
#define kDefaultAssetVersionMajor   0x00000001
#define kDefaultAssetVersionMinor   0x00000002
#define kDefaultAssetVersionRelease 0x00000003
#define kDefaultAssetVersionBuild   0x00000000
#define kDefaultAssetNumPayloads    1
#define kAssetMetadataLen           16
#define kPayloadMetadataLen         24
#define kMaxTLVLen                  32
#define kPayloadDataChunks          8
#define kPayloadLen                 (kPayloadDataChunks * kUARPDataChunkSize + 1)
#define kSuperBinaryPayloadIndex    0
#define kMaxPayloads                2

#define UARP_MSG_HEADER_SIZE          (sizeof(struct UARPMsgHeader))
#define UARP_MSG_ASSET_DATA_RESP_SIZE (sizeof(struct UARPMsgAssetDataResponse))
#define UARP_MSG_ASSET_DATA_REQ_SIZE  (sizeof(struct UARPMsgAssetDataRequest))
#define UARP_SB_HEADER_SIZE           (sizeof(struct UARPSuperBinaryHeader))
#define UARP_PAYLOAD_HEADER_SIZE      (sizeof(struct UARPPayloadHeader))
#define UARP_TLV_HEADER_SIZE          (sizeof(struct UARPTLVHeader))

/**
 * Potential Test Improvements:
 *   - Asynchronous events
 *       - Controller staging pause/resume
 *       - Accessory staging pause/resume/abandon with pending data response
 *   - Payload data length edge conditions
 *   - Controller cannot fulfill entire data request (num bytes responded != num bytes requested)
 */

//----------------------------------------------------------------------------------------------------------------------

typedef struct {
    DataStreamEncryptionContext encryptionContext;
    HAPPlatformTCPStreamRef clientTCPStream;
} DataStreamContext;

typedef struct {
    DataStreamContext hds;
    uint32_t rxMsgID;
    uint32_t txMsgID;
    uint16_t assetID;
} UARPControllerContext;

typedef struct {
    UARPVersion assetVersion;
    uint32_t assetTag;
    uint32_t assetLength;
    uint16_t assetNumPayloads;
    struct UARPSuperBinaryHeader header;
    struct UARPPayloadHeader payload[kMaxPayloads];
} UARPTestSuperBinary;

typedef struct {
    bool expectAssetOfferedCallback;
    bool expectAssetMetadataCallback;
    bool expectAssetMetadataCompleteCallback;
    bool expectPayloadReadyCallback;
    bool expectPayloadMetadataCallback;
    bool expectPayloadMetadataCompleteCallback;
    bool expectPayloadDataCallback;
    bool expectPayloadDataCompleteCallback;
    bool expectAssetStateChangeCallback;
    bool expectAssetApplyCallback;
    bool expectLastErrorCallback;

    struct {
        uint32_t tlvType;
        uint32_t tlvLength;
        uint8_t tlvValue[kMaxTLVLen];
    } expectedMetadata;

    struct {
        uint32_t payloadTag;
        uint32_t offset;
        uint8_t data[kPayloadLen];
        size_t dataLength;
    } expectedPayload;

    UARPAssetStateChange expectedState;

    struct {
        uint32_t action;
        uint32_t error;
    } expectedError;

    struct {
        bool refuseRequest;
    } accessoryApplyConfig;

    struct {
        bool abandon;
        bool skipPayload;
    } accessoryPayloadReadyConfig;

    struct {
        bool abandon;
        bool skipPayload;
    } accessoryPayloadMetadataCompleteConfig;

    struct {
        bool setPayloadDataOffset;
        uint32_t payloadDataOffset;
    } payloadDataOffsetConfig;

    bool initiateAccessoryPause;
    bool initiateAccessoryPauseResume;
    bool initiateAccessoryAbandon;
    bool handledControllerPause;

    struct {
        UARPVersion version;
        uint32_t activePayload;
        bool firmwareStagingInProgress;
        bool firmwareStaged;
    } state;

    UARPTestSuperBinary sb;

    // This holds the index for the controller
    // which is staging an asset.
    size_t firmwareStagingControllerIndex;
} UARPTestContext;

typedef struct {
    bool testAccessoryApplyRefusal;
    bool testAccessoryPause;
    bool testAccessoryPauseResume;
    bool testAccessoryAbandon;
    bool testAccessoryAbandonInPayloadMetadataComplete;
    bool testAccessoryAbandonInPayloadReady;
    bool testAccessoryPayloadSubset;
    bool testAccessoryPayloadSubsetWithMetadata;
    bool testAssetCorrupt;
    bool testAssetMerge;
    bool testControllerPause;
    bool testControllerPauseResume;
    bool testControllerTimeout;
    bool testControllerTimeoutResume;
    bool testUARPOfferRefusal;
    bool testAccessorySetPayloadDataOffset;

    bool skipApplyStagedAssets;
    UARPVersion* assetVersion;
    bool setAssetNumPayloads;
    uint16_t assetNumPayloads;
    uint32_t assetMergePayloadOffset;
    size_t controllerIndex;
    int32_t payloadDataOffset;
} UARPTestSuperBinaryOptions;

//----------------------------------------------------------------------------------------------------------------------

extern UARPAccessory uarpAccessory;
static UARPTestContext test;

static HAPStreamDataStreamProtocol streamDataStreamProtocol = {
    .base = &kHAPStreamDataStreamProtocol_Base,
    .numDataStreams = kApp_NumDataStreams,
    .applicationProtocols = (HAPStreamApplicationProtocol* const[]) { &streamProtocolUARP, NULL }
};

static HAPDataStreamDescriptor dataStreamDescriptors[kApp_NumDataStreams];
static const HAPDataStreamDispatcherStorage dataStreamDispatcherStorage = {
    .numDataStreams = kApp_NumDataStreams,
    .dataStreamDescriptors = dataStreamDescriptors,
    .dataStreamProtocols = (HAPDataStreamProtocol* const[]) { &streamDataStreamProtocol, NULL }
};
static HAPDataStreamDispatcher dataStreamDispatcher;

static UARPControllerContext uarpControllers[kUARPNumMaxControllers];
static UARPAccessoryFirmwareAssetCallbacks uarpFirmwareAssetCallbacks;
static UARPTestSuperBinaryOptions testOptions;
static UARPVersion testAssetVersion;

// Accessory Server Storage
static HAPAccessoryServer accessoryServer;
static HAPAccessoryServerCallbacks accessoryServerCallbacks;
static HAPIPSession ipSessions[kHAPIPSessionStorage_DefaultNumElements];
static IPSessionState ipSessionStates[HAPArrayCount(ipSessions)];
static HAPIPReadContext ipReadContexts[kAttributeCount];
static HAPIPWriteContext ipWriteContexts[kAttributeCount];
static uint8_t ipScratchBuffer[kHAPIPSession_DefaultScratchBufferSize];
static HAPIPAccessoryServerStorage ipAccessoryServerStorage;

static HAPDataStreamRef dataStreams[kApp_NumDataStreams];

static HAPPlatformTCPStreamManager dataStreamTCPStreamManager;
static HAPPlatformTCPStream dataStreamTCPStreams[kApp_NumDataStreams];

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

static HAPAccessory accessory = InitAccessory();

} // extern "C"
//----------------------------------------------------------------------------------------------------------------------

static void SendUARPMessage(
        HAPPlatformTCPStreamManager* dataStreamTCPStreamManager,
        DataStreamContext* hdsContext,
        void* uarpMessage,
        size_t uarpMessageSize) {
    // clang-format off
    const uint8_t header[] = {
            kHAPOPACKTag_Dictionary2,
            kHAPOPACKTag_String8,
            'p', 'r', 'o', 't', 'o', 'c', 'o', 'l',
            kHAPOPACKTag_String6,
            's', 't', 'r', 'e', 'a', 'm',
            kHAPOPACKTag_String5,
            'e', 'v', 'e', 'n', 't',
            kHAPOPACKTag_String4,
            'U', 'A', 'R', 'P'
    };
    const uint8_t dataHeader[] = {
            kHAPOPACKTag_Dictionary1,
            kHAPOPACKTag_String4,
            'd', 'a', 't', 'a',
            kHAPOPACKTag_DataUInt16, HAPExpandLittleUInt16(uarpMessageSize)
    };
    // clang-format on
    {
        uint8_t message[sizeof dataHeader + uarpMessageSize];
        HAPRawBufferCopyBytes(&message[0], dataHeader, sizeof dataHeader);
        HAPRawBufferCopyBytes(&message[sizeof dataHeader], uarpMessage, uarpMessageSize);
        uint8_t frame[HAP_DATASTREAM_ENC_FRAME_MIN_LENGTH + sizeof header + sizeof message];
        DataStreamTestHelpersDataStreamEncryptControllerPacketWithContext(
                &(hdsContext->encryptionContext),
                frame,
                sizeof(frame),
                header,
                sizeof(header),
                message,
                sizeof(message));

        size_t numBytes;
        HAPError err = HAPPlatformTCPStreamClientWrite(
                dataStreamTCPStreamManager, hdsContext->clientTCPStream, frame, sizeof frame, &numBytes);
        TEST_ASSERT(!err);
        TEST_ASSERT_EQUAL(numBytes, sizeof(frame));
    }
}

static void ReceiveUARPMessage(
        HAPPlatformTCPStreamManager* dataStreamTCPStreamManager,
        DataStreamContext* hdsContext,
        void* frameBytes,
        size_t frameBytesSize,
        void** uarpMessage,
        size_t* uarpMessageNumBytes) {

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
    HAPAssert(frameBytesSize >= HAP_DATASTREAM_FRAME_HEADER_LENGTH + frameLength + CHACHA20_POLY1305_TAG_BYTES);

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
            &(hdsContext->encryptionContext),
            (uint8_t*) (frameBytes),
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

        HAPOPACKStringDictionaryElement* stringDictionaryElement[] = { &protocolElement, &eventElement, NULL };
        err = HAPOPACKStringDictionaryReaderGetAll(&headerReader, stringDictionaryElement);
        TEST_ASSERT(!err);

        // Protocol name.
        TEST_ASSERT(protocolElement.value.exists);
        char* protocolName;
        err = HAPOPACKReaderGetNextString(&protocolElement.value.reader, &protocolName);
        TEST_ASSERT(!err);
        TEST_ASSERT(HAPStringAreEqual(protocolName, "stream"));

        // Topic.
        TEST_ASSERT(eventElement.value.exists);
        char* topic;
        err = HAPOPACKReaderGetNextString(&eventElement.value.reader, &topic);
        TEST_ASSERT(!err);
        TEST_ASSERT(HAPStringAreEqual(topic, kHAPStreamAppProtocolName_UARP));
    }

    // Message.
    {
        HAPOPACKReader messageReader;
        HAPOPACKReaderCreate(&messageReader, payload, payloadSize);
        HAPOPACKStringDictionaryElement dataElement;
        dataElement.key = "data";

        HAPOPACKStringDictionaryElement* stringDictionaryElement[] = { &dataElement, NULL };
        err = HAPOPACKStringDictionaryReaderGetAll(&messageReader, stringDictionaryElement);
        TEST_ASSERT(!err);

        // Parse data.
        TEST_ASSERT(dataElement.value.exists);
        err = HAPOPACKReaderGetNextData(&dataElement.value.reader, uarpMessage, uarpMessageNumBytes);
        TEST_ASSERT(!err);
    }
}

static const char* GetAssetProcessingNotificationNameFromType(uint16_t notificationType) {
    switch (notificationType) {
        case kUARPAssetProcessingFlagsUploadComplete:
            return "Complete";
        case kUARPAssetProcessingFlagsUploadDenied:
            return "Denied";
        case kUARPAssetProcessingFlagsUploadAbandoned:
            return "Abandoned";
        case kUARPAssetProcessingFlagsAssetCorrupt:
            return "Corrupt";
        default:
            HAPFatalError();
    }
}

static void ReceiveAssetProcessingNotification(
        HAPPlatformTCPStreamManager* dataStreamTCPStreamManager,
        UARPControllerContext* uarpControllers,
        int controllerIndex,
        uint16_t notificationType) {
    // Retrieve the context for the controller associated with the asset processing notification.
    DataStreamContext* hdsContextController = &(uarpControllers[controllerIndex].hds);
    uint32_t* rxMsgID = &(uarpControllers[controllerIndex].rxMsgID);

    HAPLog(&kHAPLog_Default,
           "Receiving UARP Asset Processing Notification (%s).",
           GetAssetProcessingNotificationNameFromType(notificationType));
    {
        uint8_t frameBytes[1024];
        struct UARPMsgAssetProcessingNotification* uarpMsg;
        size_t uarpMsgNumBytes;
        ReceiveUARPMessage(
                dataStreamTCPStreamManager,
                hdsContextController,
                (void*) frameBytes,
                sizeof(frameBytes),
                (void**) &uarpMsg,
                &uarpMsgNumBytes);

        HAPLogBufferDebug(&kHAPLog_Default, uarpMsg, uarpMsgNumBytes, "UARP Asset Processing Notification");
        TEST_ASSERT_EQUAL(uarpMsgNumBytes, sizeof(*uarpMsg));
        TEST_ASSERT_EQUAL(uarpMsg->msgHdr.msgType, uarpHtons(kUARPMsgAssetProcessingNotification));
        TEST_ASSERT_EQUAL(
                uarpMsg->msgHdr.msgPayloadLength,
                uarpHtons(sizeof(struct UARPMsgAssetProcessingNotification) - UARP_MSG_HEADER_SIZE));
        TEST_ASSERT_EQUAL(uarpMsg->msgHdr.msgID, uarpHtons((*rxMsgID)++));
        TEST_ASSERT_EQUAL(uarpNtohs(uarpMsg->assetProcessingFlags) & 0xFF, notificationType);
    }
}

static void ReceiveAssetAvailableNotificationAck(
        HAPPlatformTCPStreamManager* dataStreamTCPStreamManager,
        UARPControllerContext* uarpControllers,
        UARPTestSuperBinaryOptions* testOptions,
        struct UARPMsgAssetAvailableNotification* assetAvailMsg) {
    DataStreamContext* hdsContext = &(uarpControllers[testOptions->controllerIndex].hds);
    uint32_t* rxMsgID = &(uarpControllers[testOptions->controllerIndex].rxMsgID);

    HAPLog(&kHAPLog_Default, "Receiving UARP Asset Available Notification Ack.");
    {
        uint8_t frameBytes[1024];
        struct UARPMsgAssetAvailableNotification* uarpMsg;
        size_t uarpMsgNumBytes;
        ReceiveUARPMessage(
                dataStreamTCPStreamManager,
                hdsContext,
                (void*) frameBytes,
                sizeof(frameBytes),
                (void**) &uarpMsg,
                &uarpMsgNumBytes);

        HAPLogBufferDebug(&kHAPLog_Default, uarpMsg, uarpMsgNumBytes, "UARP Asset Available Notification Ack");
        TEST_ASSERT_EQUAL(uarpMsgNumBytes, sizeof(*uarpMsg));
        TEST_ASSERT_EQUAL(uarpMsg->msgHdr.msgType, uarpHtons(kUARPMsgAssetAvailableNotificationAck));
        TEST_ASSERT_EQUAL(
                uarpMsg->msgHdr.msgPayloadLength,
                uarpHtons(sizeof(struct UARPMsgAssetAvailableNotification) - UARP_MSG_HEADER_SIZE));
        TEST_ASSERT_EQUAL(uarpMsg->msgHdr.msgID, uarpHtons((*rxMsgID)++));
        TEST_ASSERT_EQUAL(uarpMsg->assetTag, assetAvailMsg->assetTag);
        TEST_ASSERT_EQUAL(uarpMsg->assetFlags, assetAvailMsg->assetFlags);
        TEST_ASSERT_EQUAL(uarpMsg->assetID, assetAvailMsg->assetID);
        TEST_ASSERT_EQUAL(uarpMsg->assetVersion.major, assetAvailMsg->assetVersion.major);
        TEST_ASSERT_EQUAL(uarpMsg->assetVersion.minor, assetAvailMsg->assetVersion.minor);
        TEST_ASSERT_EQUAL(uarpMsg->assetVersion.release, assetAvailMsg->assetVersion.release);
        TEST_ASSERT_EQUAL(uarpMsg->assetVersion.build, assetAvailMsg->assetVersion.build);
        TEST_ASSERT_EQUAL(uarpMsg->assetLength, assetAvailMsg->assetLength);
        TEST_ASSERT_EQUAL(uarpMsg->assetNumPayloads, assetAvailMsg->assetNumPayloads);
    }
}

static void SendVersionDiscoveryRequest(
        HAPPlatformTCPStreamManager* dataStreamTCPStreamManager,
        UARPControllerContext* uarpControllers,
        int controllerIndex) {
    DataStreamContext* hdsContext = &(uarpControllers[controllerIndex].hds);
    uint32_t* txMsgID = &(uarpControllers[controllerIndex].txMsgID);

    HAPLog(&kHAPLog_Default, "Sending UARP version discovery request.");
    {
        struct UARPMsgVersionDiscoveryRequest uarpMsg = {
            .msgHdr = { .msgType = uarpHtons(kUARPMsgVersionDiscoveryRequest),
                        .msgPayloadLength =
                                uarpHtons(sizeof(struct UARPMsgVersionDiscoveryRequest) - UARP_MSG_HEADER_SIZE),
                        .msgID = uarpHtons((*txMsgID)++) },
            .protocolVersionController = uarpHtons(UARP_PROTOCOL_VERSION)
        };
        SendUARPMessage(dataStreamTCPStreamManager, hdsContext, (void*) &uarpMsg, sizeof(uarpMsg));
    }
}

static void ReceiveVersionDiscoveryResponse(
        HAPPlatformTCPStreamManager* dataStreamTCPStreamManager,
        UARPControllerContext* uarpControllers,
        int controllerIndex) {
    DataStreamContext* hdsContext = &(uarpControllers[controllerIndex].hds);
    uint32_t* rxMsgID = &(uarpControllers[controllerIndex].rxMsgID);

    HAPLog(&kHAPLog_Default, "Receiving UARP version discovery response.");
    {
        uint8_t frameBytes[1024];
        struct UARPMsgVersionDiscoveryResponse* uarpMsg;
        size_t uarpMsgNumBytes;
        ReceiveUARPMessage(
                dataStreamTCPStreamManager,
                hdsContext,
                (void*) frameBytes,
                sizeof(frameBytes),
                (void**) &uarpMsg,
                &uarpMsgNumBytes);

        HAPLogBufferDebug(&kHAPLog_Default, uarpMsg, uarpMsgNumBytes, "UARP Version Discovery Response");
        TEST_ASSERT_EQUAL(uarpMsgNumBytes, sizeof(*uarpMsg));
        TEST_ASSERT_EQUAL(uarpMsg->msgHdr.msgType, uarpHtons(kUARPMsgVersionDiscoveryResponse));
        TEST_ASSERT_EQUAL(
                uarpMsg->msgHdr.msgPayloadLength,
                uarpHtons(sizeof(struct UARPMsgVersionDiscoveryResponse) - UARP_MSG_HEADER_SIZE));
        TEST_ASSERT_EQUAL(uarpMsg->msgHdr.msgID, uarpHtons((*rxMsgID)++));
        TEST_ASSERT_EQUAL(uarpMsg->status, uarpHtons(kUARPStatusSuccess));
        TEST_ASSERT_EQUAL(uarpMsg->protocolVersionAccessory, uarpHtons(UARP_PROTOCOL_VERSION));
    }
}

static void SendSyncMessage(
        HAPPlatformTCPStreamManager* dataStreamTCPStreamManager,
        UARPControllerContext* uarpControllers,
        int controllerIndex) {
    DataStreamContext* hdsContext = &(uarpControllers[controllerIndex].hds);
    uint32_t* txMsgID = &(uarpControllers[controllerIndex].txMsgID);

    HAPLog(&kHAPLog_Default, "Sending UARP sync message.");
    {
        struct UARPMsgHeader uarpMsg = { .msgType = uarpHtons(kUARPMsgSync),
                                         .msgPayloadLength = 0,
                                         .msgID = uarpHtons((*txMsgID)++) };
        SendUARPMessage(dataStreamTCPStreamManager, hdsContext, (void*) &uarpMsg, sizeof(uarpMsg));
    }
}

static void ReceiveSyncMessage(
        HAPPlatformTCPStreamManager* dataStreamTCPStreamManager,
        UARPControllerContext* uarpControllers,
        int controllerIndex) {
    DataStreamContext* hdsContext = &(uarpControllers[controllerIndex].hds);
    uint32_t* rxMsgID = &(uarpControllers[controllerIndex].rxMsgID);

    HAPLog(&kHAPLog_Default, "Receiving UARP Sync message.");
    {
        uint8_t frameBytes[1024];
        struct UARPMsgHeader* uarpMsg;
        size_t uarpMsgNumBytes;
        ReceiveUARPMessage(
                dataStreamTCPStreamManager,
                hdsContext,
                (void*) frameBytes,
                sizeof(frameBytes),
                (void**) &uarpMsg,
                &uarpMsgNumBytes);

        HAPAssert(uarpMsgNumBytes == sizeof(*uarpMsg));
        HAPAssert(uarpMsg->msgType == uarpHtons(kUARPMsgSync));
        HAPAssert(uarpMsg->msgPayloadLength == 0);
        HAPAssert(uarpMsg->msgID == uarpHtons((*rxMsgID)++));
    }
}

static void ReceiveAssetDataTransferNotificationAck(
        HAPPlatformTCPStreamManager* dataStreamTCPStreamManager,
        UARPControllerContext* uarpControllers,
        int controllerIndex,
        struct UARPMsgAssetDataTransferNotification* assetDataTransferMsg) {
    DataStreamContext* hdsContext = &(uarpControllers[controllerIndex].hds);
    uint32_t* rxMsgID = &(uarpControllers[controllerIndex].rxMsgID);

    HAPLog(&kHAPLog_Default, "Receiving UARP Asset Data Transfer Notification Ack.");
    {
        uint8_t frameBytes[1024];
        struct UARPMsgAssetDataTransferNotification* uarpMsg;
        size_t uarpMsgNumBytes;
        ReceiveUARPMessage(
                dataStreamTCPStreamManager,
                hdsContext,
                (void*) frameBytes,
                sizeof(frameBytes),
                (void**) &uarpMsg,
                &uarpMsgNumBytes);

        HAPLogBufferDebug(&kHAPLog_Default, uarpMsg, uarpMsgNumBytes, "UARP Asset Data Transfer Notification Ack");
        TEST_ASSERT_EQUAL(uarpMsgNumBytes, sizeof(*uarpMsg));
        TEST_ASSERT_EQUAL(uarpMsg->msgHdr.msgType, uarpHtons(kUARPMsgAssetDataTransferNotificationAck));
        TEST_ASSERT_EQUAL(
                uarpMsg->msgHdr.msgPayloadLength,
                uarpHtons(sizeof(struct UARPMsgAssetDataTransferNotification) - UARP_MSG_HEADER_SIZE));
        TEST_ASSERT_EQUAL(uarpMsg->msgHdr.msgID, uarpHtons((*rxMsgID)++));
        TEST_ASSERT_EQUAL(uarpMsg->assetTransferFlags, assetDataTransferMsg->assetTransferFlags);
    }
}

static void SendAssetDataTransferPauseNotification(
        HAPPlatformTCPStreamManager* dataStreamTCPStreamManager,
        UARPControllerContext* uarpControllers,
        int controllerIndex) {
    DataStreamContext* hdsContext = &(uarpControllers[controllerIndex].hds);
    uint32_t* txMsgID = &(uarpControllers[controllerIndex].txMsgID);

    HAPLog(&kHAPLog_Default, "Sending UARP Transfer Notification (pause).");
    {
        struct UARPMsgAssetDataTransferNotification uarpMsg = {
            .msgHdr = { .msgType = uarpHtons(kUARPMsgAssetDataTransferNotification),
                        .msgPayloadLength =
                                uarpHtons(sizeof(struct UARPMsgAssetDataTransferNotification) - UARP_MSG_HEADER_SIZE),
                        .msgID = uarpHtons((*txMsgID)++) },
            .assetTransferFlags = uarpHtons(kUARPAssetDataTransferPause)
        };
        SendUARPMessage(dataStreamTCPStreamManager, hdsContext, (void*) &uarpMsg, sizeof(uarpMsg));
        ReceiveAssetDataTransferNotificationAck(dataStreamTCPStreamManager, uarpControllers, controllerIndex, &uarpMsg);
    }
}

static void SendAssetDataTransferResumeNotification(
        HAPPlatformTCPStreamManager* dataStreamTCPStreamManager,
        UARPControllerContext* uarpControllers,
        int controllerIndex) {
    DataStreamContext* hdsContext = &(uarpControllers[controllerIndex].hds);
    uint32_t* txMsgID = &(uarpControllers[controllerIndex].txMsgID);

    HAPLog(&kHAPLog_Default, "Sending UARP Transfer Notification (resume).");
    {
        struct UARPMsgAssetDataTransferNotification uarpMsg = {
            .msgHdr = { .msgType = uarpHtons(kUARPMsgAssetDataTransferNotification),
                        .msgPayloadLength =
                                uarpHtons(sizeof(struct UARPMsgAssetDataTransferNotification) - UARP_MSG_HEADER_SIZE),
                        .msgID = uarpHtons((*txMsgID)++) },
            .assetTransferFlags = uarpHtons(kUARPAssetDataTransferResume)
        };
        SendUARPMessage(dataStreamTCPStreamManager, hdsContext, (void*) &uarpMsg, sizeof(uarpMsg));
        ReceiveAssetDataTransferNotificationAck(dataStreamTCPStreamManager, uarpControllers, controllerIndex, &uarpMsg);
    }
}

static void TestAccessoryInfoQueryHelperFwVersion(
        HAPPlatformTCPStreamManager* dataStreamTCPStreamManager,
        UARPControllerContext* uarpControllers,
        uint32_t tlvType,
        UARPVersion* expectedVersion,
        UARPTestSuperBinaryOptions* testOptions) {

    DataStreamContext* hdsContext = &(uarpControllers[testOptions->controllerIndex].hds);
    uint32_t* rxMsgID = &(uarpControllers[testOptions->controllerIndex].rxMsgID);
    uint32_t* txMsgID = &(uarpControllers[testOptions->controllerIndex].txMsgID);

    // Send UARP Accessory Information Request.
    HAPLog(&kHAPLog_Default, "Sending UARP accessory information request.");
    {
        struct UARPMsgAccessoryInformationRequest uarpMsg = {
            .msgHdr = { .msgType = uarpHtons(kUARPMsgAccessoryInformationRequest),
                        .msgPayloadLength =
                                uarpHtons(sizeof(struct UARPMsgAccessoryInformationRequest) - UARP_MSG_HEADER_SIZE),
                        .msgID = uarpHtons((*txMsgID)++) },
            .informationOption = uarpHtonl(tlvType)
        };
        SendUARPMessage(dataStreamTCPStreamManager, hdsContext, (void*) &uarpMsg, sizeof(uarpMsg));
    }
    HAPLog(&kHAPLog_Default, "Receiving UARP accessory information response.");
    {
        uint8_t frameBytes[1024];
        struct UARPMsgAccessoryInformationResponse* uarpMsg;
        size_t uarpMsgNumBytes;
        ReceiveUARPMessage(
                dataStreamTCPStreamManager,
                hdsContext,
                (void*) frameBytes,
                sizeof(frameBytes),
                (void**) &uarpMsg,
                &uarpMsgNumBytes);

        HAPLogBufferDebug(&kHAPLog_Default, uarpMsg, uarpMsgNumBytes, "UARP Accessory Information Response");
        TEST_ASSERT_EQUAL(uarpMsg->msgHdr.msgType, uarpHtons(kUARPMsgAccessoryInformationResponse));
        TEST_ASSERT_EQUAL(uarpMsg->msgHdr.msgID, uarpHtons((*rxMsgID)++));
        TEST_ASSERT_EQUAL(uarpMsg->status, uarpHtons(kUARPStatusSuccess));

        // TLV header follows the UARPMsgAccessoryInformationResponse structure.
        struct UARPTLVHeader* tlvHeader = (struct UARPTLVHeader*) &((uint8_t*) uarpMsg)[sizeof(*uarpMsg)];
        TEST_ASSERT_EQUAL(tlvHeader->tlvType, uarpHtonl(tlvType));
        TEST_ASSERT_EQUAL(tlvHeader->tlvLength, uarpHtonl(sizeof(struct UARPVersion)));

        // TLV value follows the TLV header.
        struct UARPVersion* version = (struct UARPVersion*) &((uint8_t*) tlvHeader)[sizeof(*tlvHeader)];
        TEST_ASSERT_EQUAL(version->major, uarpHtonl(expectedVersion->major));
        TEST_ASSERT_EQUAL(version->minor, uarpHtonl(expectedVersion->minor));
        TEST_ASSERT_EQUAL(version->release, uarpHtonl(expectedVersion->release));
        TEST_ASSERT_EQUAL(version->build, uarpHtonl(expectedVersion->build));

        // Check sizes now that we've parsed the TLV length.
        TEST_ASSERT_EQUAL(uarpMsgNumBytes, (sizeof(*uarpMsg) + sizeof(*tlvHeader) + sizeof(*version)));
        TEST_ASSERT_EQUAL(
                uarpMsg->msgHdr.msgPayloadLength,
                uarpHtons(sizeof(*uarpMsg) - UARP_MSG_HEADER_SIZE + sizeof(*tlvHeader) + sizeof(*version)));
    }
}

static void TestSuperBinaryTransfer(
        HAPPlatformTCPStreamManager* dataStreamTCPStreamManager,
        UARPControllerContext* uarpControllers,
        UARPTestSuperBinaryOptions* options) {

    uint32_t accessoryPauseChunk = 0;
    uint32_t accessoryPauseResumeChunk = 0;
    uint32_t accessoryAbandonChunk = 0;
    uint32_t controllerTimeoutChunk = 0;
    uint32_t controllerPauseChunk = 0;
    uint32_t controllerPauseResumeChunk = 0;
    size_t controllerIndex = 0;

    // Prepare the test.
    if (options->testAccessoryPause) {
        // Get a payload chunk in the range 1:kPayloadDataChunks.
        HAPPlatformRandomNumberFill(&accessoryPauseChunk, sizeof(accessoryPauseChunk));
        accessoryPauseChunk %= kPayloadDataChunks;
        accessoryPauseChunk++;
    } else if (options->testAccessoryPauseResume) {
        // Get a payload chunk in the range 1:kPayloadDataChunks.
        HAPPlatformRandomNumberFill(&accessoryPauseResumeChunk, sizeof(accessoryPauseResumeChunk));
        accessoryPauseResumeChunk %= kPayloadDataChunks;
        accessoryPauseResumeChunk++;
    } else if (options->testAccessoryAbandon) {
        // Get a payload chunk in the range 1:kPayloadDataChunks.
        HAPPlatformRandomNumberFill(&accessoryAbandonChunk, sizeof(accessoryAbandonChunk));
        accessoryAbandonChunk %= kPayloadDataChunks;
        accessoryAbandonChunk++;
    } else if ((options->testControllerTimeout) || (options->testControllerTimeoutResume)) {
        // Get a payload chunk in the range 1:kPayloadDataChunks.
        HAPPlatformRandomNumberFill(&controllerTimeoutChunk, sizeof(controllerTimeoutChunk));
        controllerTimeoutChunk %= kPayloadDataChunks;
        controllerTimeoutChunk++;
    } else if (options->testControllerPause) {
        // Get a payload chunk in the range 1:kPayloadDataChunks.
        HAPPlatformRandomNumberFill(&controllerPauseChunk, sizeof(controllerPauseChunk));
        controllerPauseChunk %= kPayloadDataChunks;
        controllerPauseChunk++;
    } else if (options->testControllerPauseResume) {
        // Get a payload chunk in the range 1:kPayloadDataChunks.
        HAPPlatformRandomNumberFill(&controllerPauseResumeChunk, sizeof(controllerPauseResumeChunk));
        controllerPauseResumeChunk %= kPayloadDataChunks;
        controllerPauseResumeChunk++;
    }

    // Save so we can access this information later in FwUpPayloadMetadataComplete() and FwUpPayloadData()
    if (options->testAccessorySetPayloadDataOffset) {
        test.payloadDataOffsetConfig.setPayloadDataOffset = true;
        test.payloadDataOffsetConfig.payloadDataOffset = options->payloadDataOffset;
    }

    controllerIndex = options->controllerIndex;
    test.state.activePayload = kSuperBinaryPayloadIndex;

    // Build out the SuperBinary information to be used by the test.
    if (options->assetVersion != NULL) {
        HAPRawBufferCopyBytes(&test.sb.assetVersion, options->assetVersion, sizeof(test.sb.assetVersion));
    } else {
        test.sb.assetVersion.major = kDefaultAssetVersionMajor;
        test.sb.assetVersion.minor = kDefaultAssetVersionMinor;
        test.sb.assetVersion.release = kDefaultAssetVersionRelease;
        test.sb.assetVersion.build = kDefaultAssetVersionBuild;
    }
    test.sb.assetTag = kAssetTag;
    if (options->setAssetNumPayloads) {
        test.sb.assetNumPayloads = options->assetNumPayloads;
    } else {
        test.sb.assetNumPayloads = kDefaultAssetNumPayloads;
    }
    test.sb.assetLength = UARP_SB_HEADER_SIZE + kAssetMetadataLen +
                          (test.sb.assetNumPayloads * (UARP_PAYLOAD_HEADER_SIZE + kPayloadMetadataLen + kPayloadLen));

    test.sb.header.superBinaryFormatVersion = kUARPSuperBinaryFormatVersion;
    test.sb.header.superBinaryHeaderLength = options->testAssetCorrupt ? UARP_SB_HEADER_SIZE - 1 : UARP_SB_HEADER_SIZE;
    test.sb.header.superBinaryLength = test.sb.assetLength;
    test.sb.header.superBinaryVersion = test.sb.assetVersion;
    test.sb.header.superBinaryMetadataOffset = UARP_SB_HEADER_SIZE;
    test.sb.header.superBinaryMetadataLength = kAssetMetadataLen;
    test.sb.header.payloadHeadersOffset = UARP_SB_HEADER_SIZE + kAssetMetadataLen;
    test.sb.header.payloadHeadersLength = test.sb.assetNumPayloads * UARP_PAYLOAD_HEADER_SIZE;

    for (int i = 0; i < test.sb.assetNumPayloads; i++) {
        test.sb.payload[i].payloadHeaderLength = UARP_PAYLOAD_HEADER_SIZE;
        // Set the version of payload index 0 to the asset version.
        // For additional payloads, increment the version release field to provide unique versions.
        test.sb.payload[i].payloadVersion.major = test.sb.assetVersion.major;
        test.sb.payload[i].payloadVersion.minor = test.sb.assetVersion.minor;
        test.sb.payload[i].payloadVersion.release = test.sb.assetVersion.release + i;
        test.sb.payload[i].payloadVersion.build = test.sb.assetVersion.build;
        // Generate unique payload tags 'ABCD', 'BCDE', ...
        uint8_t payloadTag[4] = { static_cast<uint8_t>('A' + i),
                                  static_cast<uint8_t>('B' + i),
                                  static_cast<uint8_t>('C' + i),
                                  static_cast<uint8_t>('D' + i) };
        test.sb.payload[i].payloadTag = uarpPayloadTagPack(payloadTag);
        // All payloads use the same metadata and data lengths to ease offset calculations.
        // We are relying on the version and tag to ensure the correct payload is being pulled.
        test.sb.payload[i].payloadMetadataOffset = UARP_SB_HEADER_SIZE + kAssetMetadataLen +
                                                   (test.sb.assetNumPayloads * UARP_PAYLOAD_HEADER_SIZE) +
                                                   (i * kPayloadMetadataLen);
        test.sb.payload[i].payloadMetadataLength = kPayloadMetadataLen;
        test.sb.payload[i].payloadOffset =
                UARP_SB_HEADER_SIZE + kAssetMetadataLen +
                (test.sb.assetNumPayloads * (UARP_PAYLOAD_HEADER_SIZE + kPayloadMetadataLen)) + (i * kPayloadLen);
        test.sb.payload[i].payloadLength = kPayloadLen;
    }

    // Guard against test limitations.
    if (testOptions.testAccessorySetPayloadDataOffset) {
        // This setting has not been tested with any other
        // test option. Proceed with these enabled at your own risk.
        TEST_ASSERT(!testOptions.testAccessoryAbandon);
        TEST_ASSERT(!testOptions.testAccessoryAbandonInPayloadReady);
        TEST_ASSERT(!testOptions.testAccessoryAbandonInPayloadMetadataComplete);
        TEST_ASSERT(!testOptions.testAccessoryApplyRefusal);
        TEST_ASSERT(!testOptions.testAccessoryPause);
        TEST_ASSERT(!testOptions.testAccessoryPauseResume);
        TEST_ASSERT(!testOptions.testAccessoryPayloadSubset);
        TEST_ASSERT(!testOptions.testAccessoryPayloadSubsetWithMetadata);
        TEST_ASSERT(!testOptions.testAssetCorrupt);
        TEST_ASSERT(!testOptions.testAssetMerge);
        TEST_ASSERT(!testOptions.testControllerTimeout);
        TEST_ASSERT(!testOptions.testControllerTimeoutResume);
        TEST_ASSERT(!testOptions.testUARPOfferRefusal);
    }
    if (testOptions.testAssetMerge) {
        // Testing of asset merge is only supported with single payload.
        TEST_ASSERT_EQUAL(test.sb.assetNumPayloads, 1);
    }
    if (testOptions.testAccessoryPayloadSubset || testOptions.testAccessoryPayloadSubsetWithMetadata) {
        // Testing of a payload subset requires multiple payloads.
        TEST_ASSERT(test.sb.assetNumPayloads > 1);
    }

    HAPLog(&kHAPLog_Default,
           "\n***** Test configuration: *****\n"
           "   accessory pause/resume = %s (%lu)\n"
           "   accessory pause = %s (%lu)\n"
           "   accessory abandon = %s (%lu)\n"
           "   asset corrupt = %s\n"
           "   asset merge = %s (%lu)\n"
           "   controller pause/resume = %s (%lu)\n"
           "   controller pause = %s (%lu)\n"
           "   controller timeout = %s (%lu)\n"
           "   controller timeout resume = %s (%lu)\n"
           "   uarp offer refusal = %s\n"
           "   accessory payload data offset = %s (%ld)\n"
           "\n"
           "   apply refusal = %s\n"
           "   skip apply = %s\n"
           "   asset version = %lu.%lu.%lu.%lu\n"
           "   asset num payloads = %u\n"
           "   controller index = %zu",
           options->testAccessoryPauseResume ? "true" : "false",
           (unsigned long) accessoryPauseResumeChunk,
           options->testAccessoryPause ? "true" : "false",
           (unsigned long) accessoryPauseChunk,
           options->testAccessoryAbandon ? "true" : "false",
           (unsigned long) accessoryAbandonChunk,
           options->testAssetCorrupt ? "true" : "false",
           options->testAssetMerge ? "true" : "false",
           (unsigned long) (options->assetMergePayloadOffset),
           options->testControllerPauseResume ? "true" : "false",
           (unsigned long) controllerPauseResumeChunk,
           options->testControllerPause ? "true" : "false",
           (unsigned long) controllerPauseChunk,
           options->testControllerTimeout ? "true" : "false",
           (unsigned long) controllerTimeoutChunk,
           options->testControllerTimeoutResume ? "true" : "false",
           (unsigned long) controllerTimeoutChunk,
           options->testUARPOfferRefusal ? "true" : "false",
           test.payloadDataOffsetConfig.setPayloadDataOffset ? "true" : "false",
           (long int) test.payloadDataOffsetConfig.payloadDataOffset,
           options->testAccessoryApplyRefusal ? "true" : "false",
           options->skipApplyStagedAssets ? "true" : "false",
           (unsigned long) (test.sb.assetVersion.major),
           (unsigned long) (test.sb.assetVersion.minor),
           (unsigned long) (test.sb.assetVersion.release),
           (unsigned long) (test.sb.assetVersion.build),
           test.sb.assetNumPayloads,
           controllerIndex);

    DataStreamContext* hdsContext = &(uarpControllers[controllerIndex].hds);
    uint32_t* rxMsgID = &(uarpControllers[controllerIndex].rxMsgID);
    uint32_t* txMsgID = &(uarpControllers[controllerIndex].txMsgID);
    uint16_t* assetID = &(uarpControllers[controllerIndex].assetID);

    // Send UARP Asset Available Notification.
    HAPLog(&kHAPLog_Default, "Sending UARP Asset Available Notification.");
    {
        struct UARPMsgAssetAvailableNotification uarpMsg = {
            .msgHdr = { .msgType = uarpHtons(kUARPMsgAssetAvailableNotification),
                        .msgPayloadLength =
                                uarpHtons(sizeof(struct UARPMsgAssetAvailableNotification) - UARP_MSG_HEADER_SIZE),
                        .msgID = uarpHtons((*txMsgID)++) },
            // No endianness conversion for tag bytes.
            .assetTag = test.sb.assetTag,
            .assetFlags = uarpHtons(kUARPAssetFlagsAssetTypeSuperBinary),
            .assetID = uarpHtons(*assetID),
            .assetVersion = { .major = uarpHtonl(test.sb.assetVersion.major),
                              .minor = uarpHtonl(test.sb.assetVersion.minor),
                              .release = uarpHtonl(test.sb.assetVersion.release),
                              .build = uarpHtonl(test.sb.assetVersion.build) },
            .assetLength = uarpHtonl(test.sb.assetLength),
            .assetNumPayloads = uarpHtons(test.sb.assetNumPayloads)
        };

        if (options->testAssetMerge) {
            test.expectAssetStateChangeCallback = true;
            test.expectedState = kUARPAssetStateChange_StagingResumed;
        } else if (!options->testUARPOfferRefusal) {
            test.expectAssetOfferedCallback = true;
        }
        SendUARPMessage(dataStreamTCPStreamManager, hdsContext, (void*) &uarpMsg, sizeof(uarpMsg));
        ReceiveAssetAvailableNotificationAck(dataStreamTCPStreamManager, uarpControllers, options, &uarpMsg);
        if (options->testAssetMerge) {
            TEST_ASSERT(!test.expectAssetStateChangeCallback);
            if (test.state.firmwareStagingInProgress) {
                // Retrieve the context for the controller that was staging the asset.
                ReceiveAssetProcessingNotification(
                        dataStreamTCPStreamManager,
                        (UARPControllerContext*) uarpControllers,
                        test.firmwareStagingControllerIndex,
                        kUARPAssetProcessingFlagsUploadAbandoned);
                test.state.firmwareStagingInProgress = false;
            }
        } else if (options->testUARPOfferRefusal) {
            ReceiveAssetProcessingNotification(
                    dataStreamTCPStreamManager,
                    (UARPControllerContext*) uarpControllers,
                    controllerIndex,
                    kUARPAssetProcessingFlagsUploadDenied);
            // Asset being offered by this function has been denied, there is nothing else to do.
            return;
        } else {
            // FwUpAssetOffered was not called if this asserts
            TEST_ASSERT(!test.expectAssetOfferedCallback);

            // If another asset is already being staged, FwUpAssetOffered has abandoned it, and
            // triggered an abandoned notifcation. We must receive the Abandoned notification,
            // so UARP can free the transmit buffers associated with the message.
            if (test.state.firmwareStagingInProgress) {
                // Retrieve the context for the controller that was staging the asset.
                ReceiveAssetProcessingNotification(
                        dataStreamTCPStreamManager,
                        (UARPControllerContext*) uarpControllers,
                        test.firmwareStagingControllerIndex,
                        kUARPAssetProcessingFlagsUploadAbandoned);
                test.state.firmwareStagingInProgress = false;
            }
        }
    }
    // For asset merges, the SuperBinary header/metadata is not re-requested.
    if (!options->testAssetMerge) {
        HAPLog(&kHAPLog_Default, "Receiving UARP Asset Data Request.");
        {
            uint8_t frameBytes[1024];
            struct UARPMsgAssetDataRequest* uarpMsg;
            size_t uarpMsgNumBytes;
            ReceiveUARPMessage(
                    dataStreamTCPStreamManager,
                    hdsContext,
                    (void*) frameBytes,
                    sizeof(frameBytes),
                    (void**) &uarpMsg,
                    &uarpMsgNumBytes);

            HAPLogBufferDebug(
                    &kHAPLog_Default, uarpMsg, uarpMsgNumBytes, "UARP Asset Data Request (SuperBinary Header)");
            TEST_ASSERT_EQUAL(uarpMsgNumBytes, sizeof(*uarpMsg));
            TEST_ASSERT_EQUAL(uarpMsg->msgHdr.msgType, uarpHtons(kUARPMsgAssetDataRequest));
            TEST_ASSERT_EQUAL(
                    uarpMsg->msgHdr.msgPayloadLength, uarpHtons(UARP_MSG_ASSET_DATA_REQ_SIZE - UARP_MSG_HEADER_SIZE));
            TEST_ASSERT_EQUAL(uarpMsg->msgHdr.msgID, uarpHtons((*rxMsgID)++));
            TEST_ASSERT_EQUAL(uarpMsg->assetID, uarpHtons(*assetID));
            TEST_ASSERT_EQUAL(uarpMsg->dataOffset, uarpHtonl(0));
            TEST_ASSERT_EQUAL(uarpMsg->numBytesRequested, uarpHtons(UARP_SB_HEADER_SIZE));
        }

        // Send UARP SuperBinary Header.
        HAPLog(&kHAPLog_Default, "Sending UARP SuperBinary Header.");
        {
            uint8_t uarpMsg[UARP_MSG_ASSET_DATA_RESP_SIZE + UARP_SB_HEADER_SIZE];
            struct UARPMsgAssetDataResponse* msg = (struct UARPMsgAssetDataResponse*) uarpMsg;

            msg->msgHdr.msgType = uarpHtons(kUARPMsgAssetDataResponse);
            msg->msgHdr.msgPayloadLength =
                    uarpHtons(UARP_MSG_ASSET_DATA_RESP_SIZE - UARP_MSG_HEADER_SIZE + UARP_SB_HEADER_SIZE);
            msg->msgHdr.msgID = uarpHtons((*txMsgID)++);
            msg->status = uarpHtons(kUARPStatusSuccess);
            msg->assetID = uarpHtons(*assetID);
            msg->dataOffset = uarpHtonl(0);
            msg->numBytesRequested = uarpHtons(UARP_SB_HEADER_SIZE);
            msg->numBytesResponded = uarpHtons(UARP_SB_HEADER_SIZE);

            struct UARPSuperBinaryHeader* superBinHdr =
                    (struct UARPSuperBinaryHeader*) &uarpMsg[UARP_MSG_ASSET_DATA_RESP_SIZE];
            superBinHdr->superBinaryFormatVersion = uarpHtonl(test.sb.header.superBinaryFormatVersion);
            superBinHdr->superBinaryHeaderLength = uarpHtonl(test.sb.header.superBinaryHeaderLength);
            superBinHdr->superBinaryLength = uarpHtonl(test.sb.header.superBinaryLength);
            superBinHdr->superBinaryVersion =
                    (UARPVersion) { .major = uarpHtonl(test.sb.header.superBinaryVersion.major),
                                    .minor = uarpHtonl(test.sb.header.superBinaryVersion.minor),
                                    .release = uarpHtonl(test.sb.header.superBinaryVersion.release),
                                    .build = uarpHtonl(test.sb.header.superBinaryVersion.build) };
            superBinHdr->superBinaryMetadataOffset = uarpHtonl(test.sb.header.superBinaryMetadataOffset);
            superBinHdr->superBinaryMetadataLength = uarpHtonl(test.sb.header.superBinaryMetadataLength);
            superBinHdr->payloadHeadersOffset = uarpHtonl(test.sb.header.payloadHeadersOffset);
            superBinHdr->payloadHeadersLength = uarpHtonl(test.sb.header.payloadHeadersLength);

            if (options->testAssetCorrupt) {
                test.expectAssetStateChangeCallback = true;
                test.expectedState = kUARPAssetStateChange_AssetCorrupt;
            }
            SendUARPMessage(dataStreamTCPStreamManager, hdsContext, (void*) &uarpMsg, sizeof(uarpMsg));
            if (options->testAssetCorrupt) {
                TEST_ASSERT(!test.expectAssetStateChangeCallback);
                ReceiveAssetProcessingNotification(
                        dataStreamTCPStreamManager,
                        (UARPControllerContext*) uarpControllers,
                        controllerIndex,
                        kUARPAssetProcessingFlagsAssetCorrupt);
                return;
            }
        }
        HAPLog(&kHAPLog_Default, "Receiving UARP Asset Data Request (SuperBinary Metadata).");
        {
            uint8_t frameBytes[1024];
            struct UARPMsgAssetDataRequest* uarpMsg;
            size_t uarpMsgNumBytes;
            ReceiveUARPMessage(
                    dataStreamTCPStreamManager,
                    hdsContext,
                    (void*) frameBytes,
                    sizeof(frameBytes),
                    (void**) &uarpMsg,
                    &uarpMsgNumBytes);

            HAPLogBufferDebug(&kHAPLog_Default, uarpMsg, uarpMsgNumBytes, "UARP Asset Data Request");
            TEST_ASSERT_EQUAL(uarpMsgNumBytes, sizeof(*uarpMsg));
            TEST_ASSERT_EQUAL(uarpMsg->msgHdr.msgType, uarpHtons(kUARPMsgAssetDataRequest));
            TEST_ASSERT_EQUAL(
                    uarpMsg->msgHdr.msgPayloadLength, uarpHtons(UARP_MSG_ASSET_DATA_REQ_SIZE - UARP_MSG_HEADER_SIZE));
            TEST_ASSERT_EQUAL(uarpMsg->msgHdr.msgID, uarpHtons((*rxMsgID)++));
            TEST_ASSERT_EQUAL(uarpMsg->assetID, uarpHtons(*assetID));
            TEST_ASSERT_EQUAL(uarpMsg->dataOffset, uarpHtonl(test.sb.header.superBinaryMetadataOffset));
            TEST_ASSERT_EQUAL(uarpMsg->numBytesRequested, uarpHtons(test.sb.header.superBinaryMetadataLength));
        }
        // Send UARP SuperBinary Metadata
        HAPLog(&kHAPLog_Default, "Sending UARP SuperBinary Metadata.");
        {
            uint8_t uarpMsg[UARP_MSG_ASSET_DATA_RESP_SIZE + kAssetMetadataLen];
            struct UARPMsgAssetDataResponse* msg = (struct UARPMsgAssetDataResponse*) uarpMsg;

            msg->msgHdr.msgType = uarpHtons(kUARPMsgAssetDataResponse);
            msg->msgHdr.msgPayloadLength =
                    uarpHtons(UARP_MSG_ASSET_DATA_RESP_SIZE - UARP_MSG_HEADER_SIZE + kAssetMetadataLen);
            msg->msgHdr.msgID = uarpHtons((*txMsgID)++);
            msg->status = uarpHtons(kUARPStatusSuccess);
            msg->assetID = uarpHtons(*assetID);
            msg->dataOffset = uarpHtonl(UARP_SB_HEADER_SIZE);
            msg->numBytesRequested = uarpHtons(test.sb.header.superBinaryMetadataLength);
            msg->numBytesResponded = uarpHtons(test.sb.header.superBinaryMetadataLength);

            struct UARPTLVHeader* tlvHeader = (struct UARPTLVHeader*) &uarpMsg[UARP_MSG_ASSET_DATA_RESP_SIZE];
            tlvHeader->tlvType = 1;
            tlvHeader->tlvLength = kAssetMetadataLen - UARP_TLV_HEADER_SIZE;
            uint8_t* tlvValue = &((uint8_t*) tlvHeader)[sizeof(*tlvHeader)];
            HAPPlatformRandomNumberFill(tlvValue, tlvHeader->tlvLength);

            test.expectAssetMetadataCallback = true;
            test.expectedMetadata.tlvType = tlvHeader->tlvType;
            test.expectedMetadata.tlvLength = tlvHeader->tlvLength;
            HAPRawBufferCopyBytes(test.expectedMetadata.tlvValue, tlvValue, test.expectedMetadata.tlvLength);
            test.expectAssetMetadataCompleteCallback = true;

            tlvHeader->tlvType = uarpHtonl(tlvHeader->tlvType);
            tlvHeader->tlvLength = uarpHtonl(tlvHeader->tlvLength);
            TEST_ASSERT(!test.state.firmwareStagingInProgress);
            SendUARPMessage(dataStreamTCPStreamManager, hdsContext, (void*) &uarpMsg, sizeof(uarpMsg));
            TEST_ASSERT(!test.expectAssetMetadataCallback);
            TEST_ASSERT(!test.expectAssetMetadataCompleteCallback);
            // Firmware Staging is now in progress. Save the controller index used for staging,
            // in case we need to abandon this asset after this function has exited.
            TEST_ASSERT(test.state.firmwareStagingInProgress);
            test.firmwareStagingControllerIndex = controllerIndex;
            test.state.version = test.sb.assetVersion;
        }
    }

    // Iterate the payloads in the SuperBinary.
    // This relies upon the accessory implementation in this test file also requesting the payloads in-order.
    for (int payload = 0; payload < test.sb.assetNumPayloads; payload++) {
        // For asset merges, the Payload header/metadata is not re-requested.
        if (!options->testAssetMerge) {
            HAPLog(&kHAPLog_Default, "Receiving UARP Asset Data Request (Payload Header).");
            {
                uint8_t frameBytes[1024];
                struct UARPMsgAssetDataRequest* uarpMsg;
                size_t uarpMsgNumBytes;
                ReceiveUARPMessage(
                        dataStreamTCPStreamManager,
                        hdsContext,
                        (void*) frameBytes,
                        sizeof(frameBytes),
                        (void**) &uarpMsg,
                        &uarpMsgNumBytes);

                HAPLogBufferDebug(&kHAPLog_Default, uarpMsg, uarpMsgNumBytes, "UARP Asset Data Request");
                TEST_ASSERT_EQUAL(uarpMsgNumBytes, sizeof(*uarpMsg));
                TEST_ASSERT_EQUAL(uarpMsg->msgHdr.msgType, uarpHtons(kUARPMsgAssetDataRequest));
                TEST_ASSERT_EQUAL(
                        uarpMsg->msgHdr.msgPayloadLength,
                        uarpHtons(UARP_MSG_ASSET_DATA_REQ_SIZE - UARP_MSG_HEADER_SIZE));
                TEST_ASSERT_EQUAL(uarpMsg->msgHdr.msgID, uarpHtons((*rxMsgID)++));
                TEST_ASSERT_EQUAL(uarpMsg->assetID, uarpHtons(*assetID));
                TEST_ASSERT_EQUAL(
                        uarpMsg->dataOffset,
                        uarpHtonl(test.sb.header.payloadHeadersOffset + (payload * UARP_PAYLOAD_HEADER_SIZE)));
                TEST_ASSERT_EQUAL(uarpMsg->numBytesRequested, uarpHtons(UARP_PAYLOAD_HEADER_SIZE));
            }
            // Send UARP Payload Header.
            HAPLog(&kHAPLog_Default, "Sending UARP Payload Header.");
            {
                uint8_t uarpMsg[UARP_MSG_ASSET_DATA_RESP_SIZE + UARP_PAYLOAD_HEADER_SIZE];
                struct UARPMsgAssetDataResponse* msg = (struct UARPMsgAssetDataResponse*) uarpMsg;

                msg->msgHdr.msgType = uarpHtons(kUARPMsgAssetDataResponse);
                msg->msgHdr.msgPayloadLength =
                        uarpHtons(UARP_MSG_ASSET_DATA_RESP_SIZE - UARP_MSG_HEADER_SIZE + UARP_PAYLOAD_HEADER_SIZE);
                msg->msgHdr.msgID = uarpHtons((*txMsgID)++);
                msg->status = uarpHtons(kUARPStatusSuccess);
                msg->assetID = uarpHtons(*assetID);
                msg->dataOffset = uarpHtonl(test.sb.header.payloadHeadersOffset + (payload * UARP_PAYLOAD_HEADER_SIZE));
                msg->numBytesRequested = uarpHtons(UARP_PAYLOAD_HEADER_SIZE);
                msg->numBytesResponded = uarpHtons(UARP_PAYLOAD_HEADER_SIZE);

                struct UARPPayloadHeader* payloadHdr =
                        (struct UARPPayloadHeader*) &uarpMsg[UARP_MSG_ASSET_DATA_RESP_SIZE];
                payloadHdr->payloadHeaderLength = uarpHtonl(sizeof(struct UARPPayloadHeader));
                // No endianness conversion for tag bytes.
                payloadHdr->payloadTag = test.sb.payload[test.state.activePayload].payloadTag;
                payloadHdr->payloadVersion = (UARPVersion) {
                    .major = uarpHtonl(test.sb.payload[test.state.activePayload].payloadVersion.major),
                    .minor = uarpHtonl(test.sb.payload[test.state.activePayload].payloadVersion.minor),
                    .release = uarpHtonl(test.sb.payload[test.state.activePayload].payloadVersion.release),
                    .build = uarpHtonl(test.sb.payload[test.state.activePayload].payloadVersion.build)
                };
                payloadHdr->payloadMetadataOffset =
                        uarpHtonl(test.sb.payload[test.state.activePayload].payloadMetadataOffset);
                payloadHdr->payloadMetadataLength =
                        uarpHtonl(test.sb.payload[test.state.activePayload].payloadMetadataLength);
                payloadHdr->payloadOffset = uarpHtonl(test.sb.payload[test.state.activePayload].payloadOffset);
                payloadHdr->payloadLength = uarpHtonl(test.sb.payload[test.state.activePayload].payloadLength);

                test.expectPayloadReadyCallback = true;
                if (options->testAccessoryPayloadSubset && payload == 0) {
                    HAPLog(&kHAPLog_Default, "Testing accessory skipping of requested payload");
                    test.accessoryPayloadReadyConfig.skipPayload = true;
                } else if (options->testAccessoryAbandonInPayloadReady) {
                    HAPLog(&kHAPLog_Default, "Testing accessory abandoning asset in payloadReady");
                    test.accessoryPayloadReadyConfig.abandon = true;
                }

                SendUARPMessage(dataStreamTCPStreamManager, hdsContext, (void*) &uarpMsg, sizeof(uarpMsg));
                TEST_ASSERT(!test.expectPayloadReadyCallback);

                if (options->testAccessoryAbandonInPayloadReady) {
                    ReceiveAssetProcessingNotification(
                            dataStreamTCPStreamManager,
                            (UARPControllerContext*) uarpControllers,
                            controllerIndex,
                            kUARPAssetProcessingFlagsUploadAbandoned);
                    return;
                }
            }

            if (test.accessoryPayloadReadyConfig.skipPayload) {
                test.accessoryPayloadReadyConfig.skipPayload = false;
                continue;
            }

            HAPLog(&kHAPLog_Default, "Receiving UARP Asset Data Request (Payload Metadata).");
            {
                uint8_t frameBytes[1024];
                struct UARPMsgAssetDataRequest* uarpMsg;
                size_t uarpMsgNumBytes;
                ReceiveUARPMessage(
                        dataStreamTCPStreamManager,
                        hdsContext,
                        (void*) frameBytes,
                        sizeof(frameBytes),
                        (void**) &uarpMsg,
                        &uarpMsgNumBytes);

                HAPLogBufferDebug(&kHAPLog_Default, uarpMsg, uarpMsgNumBytes, "UARP Asset Data Request");
                TEST_ASSERT_EQUAL(uarpMsgNumBytes, sizeof(*uarpMsg));
                TEST_ASSERT_EQUAL(uarpMsg->msgHdr.msgType, uarpHtons(kUARPMsgAssetDataRequest));
                TEST_ASSERT_EQUAL(
                        uarpMsg->msgHdr.msgPayloadLength,
                        uarpHtons(UARP_MSG_ASSET_DATA_REQ_SIZE - UARP_MSG_HEADER_SIZE));
                TEST_ASSERT_EQUAL(uarpMsg->msgHdr.msgID, uarpHtons((*rxMsgID)++));
                TEST_ASSERT_EQUAL(uarpMsg->assetID, uarpHtons(*assetID));
                TEST_ASSERT_EQUAL(
                        uarpMsg->dataOffset,
                        uarpHtonl(test.sb.payload[test.state.activePayload].payloadMetadataOffset));
                TEST_ASSERT_EQUAL(
                        uarpMsg->numBytesRequested,
                        uarpHtons(test.sb.payload[test.state.activePayload].payloadMetadataLength));
            }
            // Send UARP Payload Metadata
            HAPLog(&kHAPLog_Default, "Sending UARP Payload Metadata.");
            {
                uint8_t uarpMsg[UARP_MSG_ASSET_DATA_RESP_SIZE + kPayloadMetadataLen];
                struct UARPMsgAssetDataResponse* msg = (struct UARPMsgAssetDataResponse*) uarpMsg;

                msg->msgHdr.msgType = uarpHtons(kUARPMsgAssetDataResponse);
                msg->msgHdr.msgPayloadLength =
                        uarpHtons(UARP_MSG_ASSET_DATA_RESP_SIZE - UARP_MSG_HEADER_SIZE + kPayloadMetadataLen);
                msg->msgHdr.msgID = uarpHtons((*txMsgID)++);
                msg->status = uarpHtons(kUARPStatusSuccess);
                msg->assetID = uarpHtons(*assetID);
                msg->dataOffset = uarpHtonl(test.sb.payload[test.state.activePayload].payloadMetadataOffset);
                msg->numBytesRequested = uarpHtons(test.sb.payload[test.state.activePayload].payloadMetadataLength);
                msg->numBytesResponded = uarpHtons(test.sb.payload[test.state.activePayload].payloadMetadataLength);

                struct UARPTLVHeader* tlvHeader = (struct UARPTLVHeader*) &uarpMsg[UARP_MSG_ASSET_DATA_RESP_SIZE];
                tlvHeader->tlvType = 1;
                tlvHeader->tlvLength = kPayloadMetadataLen - UARP_TLV_HEADER_SIZE;
                uint8_t* tlvValue = &((uint8_t*) tlvHeader)[sizeof(*tlvHeader)];
                HAPPlatformRandomNumberFill(tlvValue, tlvHeader->tlvLength);

                test.expectPayloadMetadataCallback = true;
                test.expectedMetadata.tlvType = tlvHeader->tlvType;
                test.expectedMetadata.tlvLength = tlvHeader->tlvLength;
                HAPRawBufferCopyBytes(test.expectedMetadata.tlvValue, tlvValue, test.expectedMetadata.tlvLength);

                tlvHeader->tlvType = uarpHtonl(tlvHeader->tlvType);
                tlvHeader->tlvLength = uarpHtonl(tlvHeader->tlvLength);

                if (options->testAccessoryAbandonInPayloadMetadataComplete) {
                    HAPLog(&kHAPLog_Default, "Testing accessory abandoning asset in payloadMetadataComplete");
                    test.accessoryPayloadMetadataCompleteConfig.abandon = true;
                } else if (options->testAccessoryPayloadSubsetWithMetadata && payload == 0) {
                    HAPLog(&kHAPLog_Default, "Testing accessory skipping of requested payload after metadata");
                    test.accessoryPayloadMetadataCompleteConfig.skipPayload = true;
                }

                SendUARPMessage(dataStreamTCPStreamManager, hdsContext, (void*) &uarpMsg, sizeof(uarpMsg));
                TEST_ASSERT(!test.expectPayloadMetadataCallback);
                TEST_ASSERT(!test.expectPayloadMetadataCompleteCallback);

                if (options->testAccessoryAbandonInPayloadMetadataComplete) {
                    ReceiveAssetProcessingNotification(
                            dataStreamTCPStreamManager,
                            (UARPControllerContext*) uarpControllers,
                            controllerIndex,
                            kUARPAssetProcessingFlagsUploadAbandoned);
                    return;
                } else if (test.accessoryPayloadMetadataCompleteConfig.skipPayload) {
                    test.accessoryPayloadMetadataCompleteConfig.skipPayload = false;
                    continue;
                }
            }
        }
        {
            uint32_t payloadChunk = 1;
            uint32_t remainingBytes;
            uint32_t requestedBytes;

            if (test.payloadDataOffsetConfig.setPayloadDataOffset) {
                // If we are starting the payload data request from a particular offset, that
                // should adjust the amount of remaining bytes.
                remainingBytes = kPayloadLen - test.payloadDataOffsetConfig.payloadDataOffset;
                test.expectedPayload.offset = test.payloadDataOffsetConfig.payloadDataOffset;
            } else {
                remainingBytes =
                        options->testAssetMerge ? (kPayloadLen - options->assetMergePayloadOffset) : kPayloadLen;
                test.expectedPayload.offset = options->testAssetMerge ? options->assetMergePayloadOffset : 0;
            }
            test.expectedPayload.payloadTag = test.sb.payload[test.state.activePayload].payloadTag;

            while (remainingBytes > 0) {
                requestedBytes = remainingBytes >= kUARPDataChunkSize ? kUARPDataChunkSize : remainingBytes;

                HAPLog(&kHAPLog_Default, "Receiving UARP Asset Data Request (Payload).");
                {
                    uint8_t frameBytes[1024];
                    struct UARPMsgAssetDataRequest* uarpMsg;
                    size_t uarpMsgNumBytes;
                    ReceiveUARPMessage(
                            dataStreamTCPStreamManager,
                            hdsContext,
                            (void*) frameBytes,
                            sizeof(frameBytes),
                            (void**) &uarpMsg,
                            &uarpMsgNumBytes);

                    HAPLogBufferDebug(&kHAPLog_Default, uarpMsg, uarpMsgNumBytes, "UARP Asset Data Request");
                    TEST_ASSERT_EQUAL(uarpMsgNumBytes, sizeof(*uarpMsg));
                    TEST_ASSERT_EQUAL(uarpMsg->msgHdr.msgType, uarpHtons(kUARPMsgAssetDataRequest));
                    TEST_ASSERT_EQUAL(
                            uarpMsg->msgHdr.msgPayloadLength,
                            uarpHtons(UARP_MSG_ASSET_DATA_REQ_SIZE - UARP_MSG_HEADER_SIZE));
                    TEST_ASSERT_EQUAL(uarpMsg->msgHdr.msgID, uarpHtons((*rxMsgID)++));
                    TEST_ASSERT_EQUAL(uarpMsg->assetID, uarpHtons(*assetID));

                    TEST_ASSERT_EQUAL(
                            uarpMsg->dataOffset,
                            uarpHtonl(
                                    test.sb.payload[test.state.activePayload].payloadOffset +
                                    test.sb.payload[test.state.activePayload].payloadLength - remainingBytes));
                    TEST_ASSERT_EQUAL(uarpMsg->numBytesRequested, uarpHtons(requestedBytes));
                }

                if (payloadChunk == accessoryPauseChunk) {
                    test.initiateAccessoryPause = true;
                } else if (payloadChunk == accessoryPauseResumeChunk) {
                    test.initiateAccessoryPauseResume = true;
                } else if (payloadChunk == accessoryAbandonChunk) {
                    test.initiateAccessoryAbandon = true;
                } else if (payloadChunk == controllerTimeoutChunk) {
                    test.expectAssetStateChangeCallback = true;
                    test.expectedState = kUARPAssetStateChange_StagingPaused;

                    // Advance clock to trigger data response timeout.
                    HAPLog(&kHAPLog_Default, "Testing controller timeout.");
                    HAPPlatformClockAdvance((HAPTime)(1 * HAPMinute));
                    TEST_ASSERT(!test.expectAssetStateChangeCallback);

                    if (!options->testControllerTimeoutResume) {
                        return;
                    }
                } else if (
                        !test.handledControllerPause &&
                        ((payloadChunk == controllerPauseChunk) || (payloadChunk == controllerPauseResumeChunk))) {
                    test.handledControllerPause = true;
                    test.expectAssetStateChangeCallback = true;
                    test.expectedState = kUARPAssetStateChange_StagingPaused;
                    SendAssetDataTransferPauseNotification(
                            dataStreamTCPStreamManager, (UARPControllerContext*) uarpControllers, controllerIndex);
                    TEST_ASSERT(!test.expectAssetStateChangeCallback);

                    if (options->testControllerPauseResume) {
                        HAPPlatformClockAdvance((HAPTime)(1 * HAPMinute));
                        test.expectAssetStateChangeCallback = true;
                        test.expectedState = kUARPAssetStateChange_StagingResumed;
                        SendAssetDataTransferResumeNotification(
                                dataStreamTCPStreamManager, (UARPControllerContext*) uarpControllers, controllerIndex);
                        TEST_ASSERT(!test.expectAssetStateChangeCallback);
                        // Controller drops pending data request when pausing.
                        // Continue to re-request dropped data chunk.
                        continue;
                    } else {
                        // Exit transfer in paused-by-controller state.
                        return;
                    }
                }

                HAPLog(&kHAPLog_Default, "Sending UARP Payload Chunk %lu.", (unsigned long) payloadChunk);
                {
                    uint8_t uarpMsg[kUARPMaxRxMessageSize];
                    struct UARPMsgAssetDataResponse* msg = (struct UARPMsgAssetDataResponse*) uarpMsg;

                    msg->msgHdr.msgType = uarpHtons(kUARPMsgAssetDataResponse);
                    msg->msgHdr.msgPayloadLength =
                            uarpHtons(UARP_MSG_ASSET_DATA_RESP_SIZE - UARP_MSG_HEADER_SIZE + requestedBytes);
                    msg->msgHdr.msgID = uarpHtons((*txMsgID)++);
                    msg->status = uarpHtons(kUARPStatusSuccess);
                    msg->assetID = uarpHtons(*assetID);
                    msg->dataOffset = uarpHtonl(
                            test.sb.payload[test.state.activePayload].payloadOffset +
                            test.sb.payload[test.state.activePayload].payloadLength - remainingBytes);
                    msg->numBytesRequested = uarpHtons(requestedBytes);
                    msg->numBytesResponded = uarpHtons(requestedBytes);

                    uint8_t* payload = (uint8_t*) &uarpMsg[UARP_MSG_ASSET_DATA_RESP_SIZE];
                    HAPPlatformRandomNumberFill(payload, requestedBytes);

                    test.expectPayloadDataCallback = true;
                    test.expectedPayload.dataLength = requestedBytes;
                    HAPRawBufferCopyBytes(test.expectedPayload.data, payload, test.expectedPayload.dataLength);

                    remainingBytes -= requestedBytes;
                    if (remainingBytes == 0) {
                        test.expectPayloadDataCompleteCallback = true;
                    } else if (payloadChunk == controllerTimeoutChunk) {
                        test.expectAssetStateChangeCallback = true;
                        test.expectedState = kUARPAssetStateChange_StagingResumed;
                    }

                    SendUARPMessage(
                            dataStreamTCPStreamManager,
                            hdsContext,
                            (void*) &uarpMsg,
                            UARP_MSG_ASSET_DATA_RESP_SIZE + requestedBytes);

                    TEST_ASSERT(!test.expectPayloadDataCallback);
                    if (payloadChunk == controllerTimeoutChunk) {
                        TEST_ASSERT(!test.expectAssetStateChangeCallback);
                    }
                }

                if (payloadChunk == accessoryPauseChunk) {
                    return;
                } else if (payloadChunk == accessoryPauseResumeChunk) {
                    // Advance clock to trigger timer for resume request.
                    HAPPlatformClockAdvance(0);
                } else if (payloadChunk == accessoryAbandonChunk) {
                    ReceiveAssetProcessingNotification(
                            dataStreamTCPStreamManager,
                            (UARPControllerContext*) uarpControllers,
                            controllerIndex,
                            kUARPAssetProcessingFlagsUploadAbandoned);
                    test.state.firmwareStagingInProgress = false;
                    return;
                }
                test.expectedPayload.offset += requestedBytes;
                payloadChunk++;
            }
        }
        TEST_ASSERT(!test.expectPayloadDataCompleteCallback);
    }

    // Receive asset staged notification
    ReceiveAssetProcessingNotification(
            dataStreamTCPStreamManager,
            (UARPControllerContext*) uarpControllers,
            controllerIndex,
            kUARPAssetProcessingFlagsUploadComplete);

    // Query the staged firmware version
    UARPVersion version = { .major = test.sb.assetVersion.major,
                            .minor = test.sb.assetVersion.minor,
                            .release = test.sb.assetVersion.release,
                            .build = test.sb.assetVersion.build };
    TestAccessoryInfoQueryHelperFwVersion(
            dataStreamTCPStreamManager,
            uarpControllers,
            kUARPTLVAccessoryInformationStagedFirmwareVersion,
            &version,
            options);

    if (options->skipApplyStagedAssets == false) {
        // Send UARP Apply Staged Asset Request
        HAPLog(&kHAPLog_Default, "Sending UARP Apply Staged Asset Request");
        {
            struct UARPMsgApplyStagedAssetsRequest uarpMsg = {
                .msgHdr = { .msgType = uarpHtons(kUARPMsgApplyStagedAssetsRequest),
                            .msgPayloadLength =
                                    uarpHtons(sizeof(struct UARPMsgApplyStagedAssetsRequest) - UARP_MSG_HEADER_SIZE),
                            .msgID = uarpHtons((*txMsgID)++) }
            };

            test.expectAssetApplyCallback = true;
            if (options->testAccessoryApplyRefusal) {
                HAPLog(&kHAPLog_Default, "Testing accessory apply request refusal");
                test.accessoryApplyConfig.refuseRequest = true;
            }
            SendUARPMessage(dataStreamTCPStreamManager, hdsContext, (void*) &uarpMsg, sizeof(uarpMsg));
            TEST_ASSERT(!test.expectAssetApplyCallback);
        }

        // Receive apply staged asset response
        HAPLog(&kHAPLog_Default, "Receiving UARP Apply Staged Asset Response.");
        {
            uint8_t frameBytes[1024];
            struct UARPMsgApplyStagedAssetsResponse* uarpMsg;
            size_t uarpMsgNumBytes;
            ReceiveUARPMessage(
                    dataStreamTCPStreamManager,
                    hdsContext,
                    (void*) frameBytes,
                    sizeof(frameBytes),
                    (void**) &uarpMsg,
                    &uarpMsgNumBytes);

            HAPLogBufferDebug(&kHAPLog_Default, uarpMsg, uarpMsgNumBytes, "UARP Asset Staged Response");
            TEST_ASSERT_EQUAL(uarpMsgNumBytes, sizeof(*uarpMsg));
            TEST_ASSERT_EQUAL(uarpMsg->msgHdr.msgType, uarpHtons(kUARPMsgApplyStagedAssetsResponse));
            TEST_ASSERT_EQUAL(
                    uarpMsg->msgHdr.msgPayloadLength,
                    uarpHtons(sizeof(struct UARPMsgApplyStagedAssetsResponse) - UARP_MSG_HEADER_SIZE));
            TEST_ASSERT_EQUAL(uarpMsg->msgHdr.msgID, uarpHtons((*rxMsgID)++));
            TEST_ASSERT_EQUAL(uarpMsg->status, uarpHtons(kUARPStatusSuccess));
            if (options->testAccessoryApplyRefusal) {
                TEST_ASSERT_EQUAL(uarpMsg->flags, uarpHtons(kUARPApplyStagedAssetsFlagsInUse));
            } else {
                TEST_ASSERT_EQUAL(uarpMsg->flags, uarpHtons(kUARPApplyStagedAssetsFlagsSuccess));
            }
        }
    }

    // Increment controller's asset ID for subsequent transfer.
    (*assetID)++;
}

static void TestAccessoryInfoQueryHelperStringTLV(
        HAPPlatformTCPStreamManager* dataStreamTCPStreamManager,
        UARPControllerContext* uarpControllers,
        uint32_t tlvType,
        const char* expectedString,
        UARPTestSuperBinaryOptions* testOptions) {

    DataStreamContext* hdsContext = &(uarpControllers[testOptions->controllerIndex].hds);
    uint32_t* rxMsgID = &(uarpControllers[testOptions->controllerIndex].rxMsgID);
    uint32_t* txMsgID = &(uarpControllers[testOptions->controllerIndex].txMsgID);

    // Send UARP Accessory Information Request.
    HAPLog(&kHAPLog_Default, "Sending UARP accessory information request.");
    {
        struct UARPMsgAccessoryInformationRequest uarpMsg = {
            .msgHdr = { .msgType = uarpHtons(kUARPMsgAccessoryInformationRequest),
                        .msgPayloadLength =
                                uarpHtons(sizeof(struct UARPMsgAccessoryInformationRequest) - UARP_MSG_HEADER_SIZE),
                        .msgID = uarpHtons((*txMsgID)++) },
            .informationOption = uarpHtonl(tlvType)
        };
        SendUARPMessage(dataStreamTCPStreamManager, hdsContext, (void*) &uarpMsg, sizeof(uarpMsg));
    }
    HAPLog(&kHAPLog_Default, "Receiving UARP accessory information response.");
    {
        uint8_t frameBytes[1024];
        struct UARPMsgAccessoryInformationResponse* uarpMsg;
        size_t uarpMsgNumBytes;
        ReceiveUARPMessage(
                dataStreamTCPStreamManager,
                hdsContext,
                (void*) frameBytes,
                sizeof(frameBytes),
                (void**) &uarpMsg,
                &uarpMsgNumBytes);

        HAPLogBufferDebug(&kHAPLog_Default, uarpMsg, uarpMsgNumBytes, "UARP Accessory Information Response");
        TEST_ASSERT_EQUAL(uarpMsg->msgHdr.msgType, uarpHtons(kUARPMsgAccessoryInformationResponse));
        TEST_ASSERT_EQUAL(uarpMsg->msgHdr.msgID, uarpHtons((*rxMsgID)++));
        TEST_ASSERT_EQUAL(uarpMsg->status, uarpHtons(kUARPStatusSuccess));

        // TLV header follows the UARPMsgAccessoryInformationResponse structure.
        struct UARPTLVHeader* tlvHeader = (struct UARPTLVHeader*) &((uint8_t*) uarpMsg)[sizeof(*uarpMsg)];
        TEST_ASSERT_EQUAL(tlvHeader->tlvType, uarpHtonl(tlvType));
        TEST_ASSERT_EQUAL(tlvHeader->tlvLength, uarpHtonl(HAPStringGetNumBytes(expectedString)));

        // TLV value follows the TLV header.
        size_t length = uarpNtohl(tlvHeader->tlvLength);
        char tlvString[length + 1];
        HAPRawBufferZero(tlvString, length + 1);
        HAPRawBufferCopyBytes(tlvString, (char*) &((uint8_t*) tlvHeader)[sizeof(*tlvHeader)], length);
        TEST_ASSERT(HAPStringAreEqual(tlvString, expectedString));

        // Check sizes now that we've parsed the TLV length.
        TEST_ASSERT_EQUAL(uarpMsgNumBytes, (sizeof(*uarpMsg) + sizeof(*tlvHeader) + length));
        TEST_ASSERT_EQUAL(
                uarpMsg->msgHdr.msgPayloadLength,
                uarpHtons(sizeof(*uarpMsg) - UARP_MSG_HEADER_SIZE + sizeof(*tlvHeader) + length));
    }
}

static void TestAccessoryInfoQueryHelperLastError(
        HAPPlatformTCPStreamManager* dataStreamTCPStreamManager,
        UARPControllerContext* uarpControllers,
        UARPTestSuperBinaryOptions* testOptions) {

    DataStreamContext* hdsContext = &(uarpControllers[testOptions->controllerIndex].hds);
    uint32_t* rxMsgID = &(uarpControllers[testOptions->controllerIndex].rxMsgID);
    uint32_t* txMsgID = &(uarpControllers[testOptions->controllerIndex].txMsgID);

    // Send UARP Accessory Information Request.
    HAPLog(&kHAPLog_Default, "Sending UARP accessory information request.");
    {
        struct UARPMsgAccessoryInformationRequest uarpMsg = {
            .msgHdr = { .msgType = uarpHtons(kUARPMsgAccessoryInformationRequest),
                        .msgPayloadLength =
                                uarpHtons(sizeof(struct UARPMsgAccessoryInformationRequest) - UARP_MSG_HEADER_SIZE),
                        .msgID = uarpHtons((*txMsgID)++) },
            .informationOption = uarpHtonl(kUARPTLVAccessoryInformationLastError)
        };

        test.expectLastErrorCallback = true;
        HAPPlatformRandomNumberFill(&test.expectedError.action, sizeof(test.expectedError.action));
        HAPPlatformRandomNumberFill(&test.expectedError.error, sizeof(test.expectedError.error));

        SendUARPMessage(dataStreamTCPStreamManager, hdsContext, (void*) &uarpMsg, sizeof(uarpMsg));
        TEST_ASSERT(!test.expectLastErrorCallback);
    }
    HAPLog(&kHAPLog_Default, "Receiving UARP accessory information response.");
    {
        uint8_t frameBytes[1024];
        struct UARPMsgAccessoryInformationResponse* uarpMsg;
        size_t uarpMsgNumBytes;
        ReceiveUARPMessage(
                dataStreamTCPStreamManager,
                hdsContext,
                (void*) frameBytes,
                sizeof(frameBytes),
                (void**) &uarpMsg,
                &uarpMsgNumBytes);

        HAPLogBufferDebug(&kHAPLog_Default, uarpMsg, uarpMsgNumBytes, "UARP Accessory Information Response");
        TEST_ASSERT_EQUAL(uarpMsg->msgHdr.msgType, uarpHtons(kUARPMsgAccessoryInformationResponse));
        TEST_ASSERT_EQUAL(uarpMsg->msgHdr.msgID, uarpHtons((*rxMsgID)++));
        TEST_ASSERT_EQUAL(uarpMsg->status, uarpHtons(kUARPStatusSuccess));

        // TLV header follows the UARPMsgAccessoryInformationResponse structure.
        struct UARPTLVHeader* tlvHeader = (struct UARPTLVHeader*) &((uint8_t*) uarpMsg)[sizeof(*uarpMsg)];
        TEST_ASSERT_EQUAL(tlvHeader->tlvType, uarpHtonl(kUARPTLVAccessoryInformationLastError));
        TEST_ASSERT_EQUAL(tlvHeader->tlvLength, uarpHtonl(sizeof(struct UARPLastErrorAction)));

        // TLV value follows the TLV header.
        struct UARPLastErrorAction* lastErrorAction =
                (struct UARPLastErrorAction*) &((uint8_t*) tlvHeader)[sizeof(*tlvHeader)];
        TEST_ASSERT_EQUAL(lastErrorAction->lastAction, uarpHtonl(test.expectedError.action));
        TEST_ASSERT_EQUAL(lastErrorAction->lastError, uarpHtonl(test.expectedError.error));

        // Check sizes now that we've parsed the TLV length.
        TEST_ASSERT_EQUAL(uarpMsgNumBytes, (sizeof(*uarpMsg) + sizeof(*tlvHeader) + sizeof(*lastErrorAction)));
        TEST_ASSERT_EQUAL(
                uarpMsg->msgHdr.msgPayloadLength,
                uarpHtons(sizeof(*uarpMsg) - UARP_MSG_HEADER_SIZE + sizeof(*tlvHeader) + sizeof(*lastErrorAction)));
    }
}

static void TestAccessoryInfoQuery(
        HAPPlatformTCPStreamManager* dataStreamTCPStreamManager,
        UARPControllerContext* uarpControllers,
        UARPTestSuperBinaryOptions* testOptions) {

    typedef struct {
        uint32_t tlvType;
        const char* expectedString;
    } AccessoryInformationStringTLV;

    AccessoryInformationStringTLV stringTlvs[] = {
        { kUARPTLVAccessoryInformationManufacturerName, accessory.manufacturer },
        { kUARPTLVAccessoryInformationModelName, accessory.model },
        { kUARPTLVAccessoryInformationHardwareVersion, accessory.hardwareVersion },
        { kUARPTLVAccessoryInformationSerialNumber, accessory.serialNumber }
    };

    for (uint32_t i = 0; i < (sizeof(stringTlvs) / sizeof(stringTlvs[0])); i++) {
        TestAccessoryInfoQueryHelperStringTLV(
                dataStreamTCPStreamManager,
                uarpControllers,
                stringTlvs[i].tlvType,
                stringTlvs[i].expectedString,
                testOptions);
    }

    TestAccessoryInfoQueryHelperLastError(dataStreamTCPStreamManager, uarpControllers, testOptions);

    int major = 0;
    int minor = 0;
    int revision = 0;
    sscanf(accessory.firmwareVersion, "%d.%d.%d", &major, &minor, &revision);
    UARPVersion version = { .major = static_cast<uint32_t>(major),
                            .minor = static_cast<uint32_t>(minor),
                            .release = static_cast<uint32_t>(revision),
                            .build = 0 };
    TestAccessoryInfoQueryHelperFwVersion(
            dataStreamTCPStreamManager,
            uarpControllers,
            kUARPTLVAccessoryInformationFirmwareVersion,
            &version,
            testOptions);

    // No staged asset.
    HAPRawBufferZero(&version, sizeof(version));
    TestAccessoryInfoQueryHelperFwVersion(
            dataStreamTCPStreamManager,
            uarpControllers,
            kUARPTLVAccessoryInformationStagedFirmwareVersion,
            &version,
            testOptions);
}

//----------------------------------------------------------------------------------------------------------------------

static void ResumeStaging(HAPPlatformTimerRef timer, void* _Nullable context_) {
    // Resume the transfer.
    HAPError err = UARPRequestFirmwareAssetStagingStateChange(kUARPAssetStagingStateChangeRequest_Resume);
    TEST_ASSERT(!err);
    test.initiateAccessoryPauseResume = false;
}

static void FwUpAssetOffered(
        HAPAccessoryServer* server,
        const HAPAccessory* accessory,
        UARPVersion* assetVersion,
        uint32_t assetTag,
        uint32_t assetLength,
        uint16_t assetNumPayloads,
        bool* shouldAccept) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);
    HAPPrecondition(assetVersion);
    HAPPrecondition(shouldAccept);

    HAPLogInfo(
            &kHAPLog_Default,
            "%s: accepting asset with version %lu.%lu.%lu.%lu",
            __func__,
            (unsigned long) (assetVersion->major),
            (unsigned long) (assetVersion->minor),
            (unsigned long) (assetVersion->release),
            (unsigned long) (assetVersion->build));

    TEST_ASSERT(test.expectAssetOfferedCallback);
    TEST_ASSERT_EQUAL(test.sb.assetVersion.major, assetVersion->major);
    TEST_ASSERT_EQUAL(test.sb.assetVersion.minor, assetVersion->minor);
    TEST_ASSERT_EQUAL(test.sb.assetVersion.release, assetVersion->release);
    TEST_ASSERT_EQUAL(test.sb.assetVersion.build, assetVersion->build);
    TEST_ASSERT_EQUAL(test.sb.assetTag, assetTag);
    TEST_ASSERT_EQUAL(test.sb.assetLength, assetLength);
    TEST_ASSERT_EQUAL(test.sb.assetNumPayloads, assetNumPayloads);
    test.expectAssetOfferedCallback = false;
    if (test.state.firmwareStaged || test.state.firmwareStagingInProgress) {
        HAPError err = UARPRequestFirmwareAssetStagingStateChange(kUARPAssetStagingStateChangeRequest_Abandon);
        if (err != kHAPError_None) {
            HAPLogError(&kHAPLog_Default, "%s: Failed to abandon staged asset", __func__);
        }
    }
    *shouldAccept = true;
}

static void FwUpAssetMetadataTLV(
        HAPAccessoryServer* server,
        const HAPAccessory* accessory,
        uint32_t tlvType,
        uint32_t tlvLength,
        uint8_t* _Nullable tlvValue) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);

    HAPLogInfo(
            &kHAPLog_Default,
            "%s: Received SuperBinary metadata with type %u, length %u",
            __func__,
            (unsigned int) tlvType,
            (unsigned int) tlvLength);

    TEST_ASSERT(test.expectAssetMetadataCallback);
    TEST_ASSERT_EQUAL(tlvType, test.expectedMetadata.tlvType);
    TEST_ASSERT_EQUAL(tlvLength, test.expectedMetadata.tlvLength);

    for (uint32_t i = 0; i < tlvLength; i++) {
        TEST_ASSERT_EQUAL(tlvValue[i], test.expectedMetadata.tlvValue[i]);
    }
    test.state.firmwareStagingInProgress = true;
    test.expectAssetMetadataCallback = false;
}

static void FwUpAssetMetadataComplete(HAPAccessoryServer* server, const HAPAccessory* accessory) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    TEST_ASSERT(test.expectAssetMetadataCompleteCallback);
    test.expectAssetMetadataCompleteCallback = false;

    HAPError err = UARPRequestFirmwareAssetPayloadByIndex(test.state.activePayload);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
}

static void FwUpPayloadReady(
        HAPAccessoryServer* server,
        const HAPAccessory* accessory,
        UARPVersion* payloadVersion,
        uint32_t payloadTag,
        uint32_t payloadLength) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);
    HAPPrecondition(payloadVersion);

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    TEST_ASSERT(test.expectPayloadReadyCallback);
    TEST_ASSERT_EQUAL(test.sb.payload[test.state.activePayload].payloadVersion.major, payloadVersion->major);
    TEST_ASSERT_EQUAL(test.sb.payload[test.state.activePayload].payloadVersion.minor, payloadVersion->minor);
    TEST_ASSERT_EQUAL(test.sb.payload[test.state.activePayload].payloadVersion.release, payloadVersion->release);
    TEST_ASSERT_EQUAL(test.sb.payload[test.state.activePayload].payloadVersion.build, payloadVersion->build);
    TEST_ASSERT_EQUAL(test.sb.payload[test.state.activePayload].payloadTag, payloadTag);
    TEST_ASSERT_EQUAL(test.sb.payload[test.state.activePayload].payloadLength, payloadLength);
    test.expectPayloadReadyCallback = false;

    if (test.accessoryPayloadReadyConfig.abandon) {
        HAPError err = UARPRequestFirmwareAssetStagingStateChange(kUARPAssetStagingStateChangeRequest_Abandon);
        TEST_ASSERT_EQUAL(err, kHAPError_None);
    } else if (test.accessoryPayloadReadyConfig.skipPayload) {
        test.state.activePayload++;
        TEST_ASSERT(test.state.activePayload < test.sb.assetNumPayloads);
        HAPLogInfo(&kHAPLog_Default, "%s: Requesting payload index %d", __func__, test.state.activePayload);
        HAPError err = UARPRequestFirmwareAssetPayloadByIndex(test.state.activePayload);
        TEST_ASSERT_EQUAL(err, kHAPError_None);
    }
}

static void FwUpPayloadMetadataTLV(
        HAPAccessoryServer* server,
        const HAPAccessory* accessory,
        uint32_t tlvType,
        uint32_t tlvLength,
        uint8_t* _Nullable tlvValue) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);

    HAPLogInfo(
            &kHAPLog_Default,
            "%s: Received Payload metadata with type %u, length %u",
            __func__,
            (unsigned int) tlvType,
            (unsigned int) tlvLength);

    TEST_ASSERT(test.expectPayloadMetadataCallback);
    TEST_ASSERT_EQUAL(tlvType, test.expectedMetadata.tlvType);
    TEST_ASSERT_EQUAL(tlvLength, test.expectedMetadata.tlvLength);
    for (uint32_t i = 0; i < tlvLength; i++) {
        TEST_ASSERT_EQUAL(tlvValue[i], test.expectedMetadata.tlvValue[i]);
    }
    test.expectPayloadMetadataCallback = false;
    test.expectPayloadMetadataCompleteCallback = true;
}

static void FwUpPayloadMetadataComplete(HAPAccessoryServer* server, const HAPAccessory* accessory) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    TEST_ASSERT(test.expectPayloadMetadataCompleteCallback);
    test.expectPayloadMetadataCompleteCallback = false;

    if (test.accessoryPayloadMetadataCompleteConfig.abandon) {
        HAPError err = UARPRequestFirmwareAssetStagingStateChange(kUARPAssetStagingStateChangeRequest_Abandon);
        TEST_ASSERT_EQUAL(err, kHAPError_None);
    } else if (test.accessoryPayloadMetadataCompleteConfig.skipPayload) {
        test.state.activePayload++;
        TEST_ASSERT(test.state.activePayload < test.sb.assetNumPayloads);
        HAPLogInfo(&kHAPLog_Default, "%s: Requesting payload index %d", __func__, test.state.activePayload);
        HAPError err = UARPRequestFirmwareAssetPayloadByIndex(test.state.activePayload);
        TEST_ASSERT_EQUAL(err, kHAPError_None);
    } else if (test.payloadDataOffsetConfig.setPayloadDataOffset) {
        HAPLogInfo(
                &kHAPLog_Default,
                "%s: Setting payload offset to %ld.",
                __func__,
                (long int) test.payloadDataOffsetConfig.payloadDataOffset);
        HAPError err = UARPSetFirmwareAssetPayloadDataOffset(test.payloadDataOffsetConfig.payloadDataOffset);
        TEST_ASSERT_EQUAL(err, kHAPError_None);
    }
}

static void FwUpPayloadData(
        HAPAccessoryServer* server,
        const HAPAccessory* accessory,
        uint32_t payloadTag,
        uint32_t offset,
        uint8_t* buffer,
        uint32_t bufferLength) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);
    HAPPrecondition(buffer);

    HAPLogInfo(
            &kHAPLog_Default,
            "%s: Received %u bytes of data for payload (0x%08X) at offset %u",
            __func__,
            (unsigned int) bufferLength,
            (unsigned int) payloadTag,
            (unsigned int) offset);

    TEST_ASSERT(test.expectPayloadDataCallback);
    TEST_ASSERT_EQUAL(payloadTag, test.expectedPayload.payloadTag);
    TEST_ASSERT_EQUAL(offset, test.expectedPayload.offset);

    TEST_ASSERT_EQUAL(bufferLength, test.expectedPayload.dataLength);
    for (uint32_t i = 0; i < bufferLength; i++) {
        TEST_ASSERT_EQUAL(buffer[i], test.expectedPayload.data[i]);
    }
    test.expectPayloadDataCallback = false;

    if (test.initiateAccessoryPauseResume) {
        HAPLog(&kHAPLog_Default, "Testing accessory pause/resume.");
        HAPError err = UARPRequestFirmwareAssetStagingStateChange(kUARPAssetStagingStateChangeRequest_Pause);
        TEST_ASSERT(!err);

        // Set timer to resume.
        HAPPlatformTimerRef timer;
        err = HAPPlatformTimerRegister(&timer, HAPPlatformClockGetCurrent(), ResumeStaging, NULL);
        TEST_ASSERT(!err);
        test.initiateAccessoryPauseResume = false;
    } else if (test.initiateAccessoryAbandon) {
        HAPLog(&kHAPLog_Default, "Testing accessory abandon.");
        HAPError err = UARPRequestFirmwareAssetStagingStateChange(kUARPAssetStagingStateChangeRequest_Abandon);
        TEST_ASSERT(!err);
        test.initiateAccessoryAbandon = false;
    } else if (test.initiateAccessoryPause) {
        HAPLog(&kHAPLog_Default, "Testing accessory pause.");
        HAPError err = UARPRequestFirmwareAssetStagingStateChange(kUARPAssetStagingStateChangeRequest_Pause);
        TEST_ASSERT(!err);
        test.initiateAccessoryPause = false;
    }
}

static void FwUpPayloadDataComplete(HAPAccessoryServer* server, const HAPAccessory* accessory) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);

    TEST_ASSERT(test.expectPayloadDataCompleteCallback);
    test.expectPayloadDataCompleteCallback = false;

    // Iterate all payloads based on index.
    if (test.state.activePayload < (uint32_t)(test.sb.assetNumPayloads - 1)) {
        test.state.activePayload++;
        HAPLogInfo(&kHAPLog_Default, "%s: Requesting payload index %d", __func__, test.state.activePayload);
        HAPError err = UARPRequestFirmwareAssetPayloadByIndex(test.state.activePayload);
        TEST_ASSERT_EQUAL(err, kHAPError_None);
    } else {
        HAPLogInfo(&kHAPLog_Default, "%s: Asset staging completed", __func__);
        test.state.firmwareStaged = true;
        test.state.firmwareStagingInProgress = false;
        HAPError err = UARPFirmwareAssetFullyStaged(&test.state.version);
        TEST_ASSERT_EQUAL(err, kHAPError_None);
    }
}

static void
        FwUpAssetStateChange(HAPAccessoryServer* server, const HAPAccessory* accessory, UARPAssetStateChange state) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);

    TEST_ASSERT(test.expectAssetStateChangeCallback);
    TEST_ASSERT_EQUAL(state, test.expectedState);
    switch (state) {
        case kUARPAssetStateChange_StagingPaused: {
            HAPLogInfo(&kHAPLog_Default, "%s: Asset staging paused", __func__);
            break;
        }
        case kUARPAssetStateChange_StagingResumed: {
            HAPLogInfo(&kHAPLog_Default, "%s: Asset staging resumed", __func__);
            break;
        }
        case kUARPAssetStateChange_AssetRescinded: {
            HAPLogInfo(&kHAPLog_Default, "%s: Asset staging abandoned", __func__);
            break;
        }
        case kUARPAssetStateChange_AssetCorrupt: {
            HAPLogInfo(&kHAPLog_Default, "%s: Asset detected as corrupt", __func__);
            break;
        }
        default:
            HAPFatalError();
    }
    test.expectAssetStateChangeCallback = false;
}

static void FwUpApplyStagedAssets(HAPAccessoryServer* server, const HAPAccessory* accessory, bool* requestRefused) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);
    HAPPrecondition(requestRefused);

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    TEST_ASSERT(test.expectAssetApplyCallback);
    test.expectAssetApplyCallback = false;

    if (test.accessoryApplyConfig.refuseRequest) {
        *requestRefused = true;
    } else {
        *requestRefused = false;
        test.state.firmwareStaged = false;
    }
    test.accessoryApplyConfig.refuseRequest = false;
}

static void FwUpRetrieveLastError(
        HAPAccessoryServer* server,
        const HAPAccessory* accessory,
        UARPLastErrorAction* lastErrorAction) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);
    HAPPrecondition(lastErrorAction);

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    TEST_ASSERT(test.expectLastErrorCallback);
    test.expectLastErrorCallback = false;

    lastErrorAction->lastAction = test.expectedError.action;
    lastErrorAction->lastError = test.expectedError.error;
}

static void InitializeControllerDataStreams(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPPlatformTCPStreamManager* dataStreamTCPStreamManager,
        UARPControllerContext* uarpControllers,
        size_t numControllers) {
    HAPError err;

    for (size_t i = 0; i < numControllers; i++) {
        uarpControllers[i].rxMsgID = 1;
        uarpControllers[i].txMsgID = 1;
        uarpControllers[i].assetID = 1;

        // Generate Controller Key Salt.
        HAPDataStreamSalt controllerKeySalt;
        HAPPlatformRandomNumberFill(controllerKeySalt.bytes, sizeof controllerKeySalt.bytes);

        // Setup HomeKit Data Stream Transport.
        err = DataStreamTestHelpersWriteSetupDataStreamTransport(server, &accessory, session, &controllerKeySalt, NULL);
        TEST_ASSERT(!err);
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
        TEST_ASSERT_EQUAL(sessionIdentifier, kHAPDataStreamHAPSessionIdentifierNone);
        TEST_ASSERT_EQUAL(setupStatus, 0);
        TEST_ASSERT_EQUAL(listeningPort, HAPPlatformTCPStreamManagerGetListenerPort(dataStreamTCPStreamManager));
        TEST_ASSERT(!HAPRawBufferIsZero(accessoryKeySalt.bytes, sizeof(accessoryKeySalt.bytes)));

        // Derive encryption keys.
        DataStreamTestHelpersDataStreamInitEncryptionContext(
                session, &controllerKeySalt, &accessoryKeySalt, &(uarpControllers[i].hds.encryptionContext));

        // Connect HomeKit Data Stream.
        HAPLog(&kHAPLog_Default, "Connecting HomeKit Data Stream.");
        err = HAPPlatformTCPStreamManagerConnectToListener(
                dataStreamTCPStreamManager, &uarpControllers[i].hds.clientTCPStream);
        TEST_ASSERT(!err);

        // Send control.hello request.
        DataStreamTestHelpersSendHelloRequest(
                dataStreamTCPStreamManager,
                &(uarpControllers[i].hds.clientTCPStream),
                &(uarpControllers[i].hds.encryptionContext));
        // Receive response for control.hello request.
        DataStreamTestHelpersReceiveHelloResponse(
                dataStreamTCPStreamManager,
                &(uarpControllers[i].hds.clientTCPStream),
                &(uarpControllers[i].hds.encryptionContext));

        // When an accessory is added on the controller side, a sync message is generated followed by the version
        // discovery request. This pair of messages is the first expected traffic on the HDS session.
        SendSyncMessage(dataStreamTCPStreamManager, uarpControllers, i);
        SendVersionDiscoveryRequest(dataStreamTCPStreamManager, uarpControllers, i);

        // When the above messages trigger a controller add on the accessory side, a sync message should be generated
        // along with a response to the version discovery.
        ReceiveSyncMessage(dataStreamTCPStreamManager, uarpControllers, i);
        ReceiveVersionDiscoveryResponse(dataStreamTCPStreamManager, uarpControllers, i);
    }
}

//----------------------------------------------------------------------------------------------------------------------
static void TestSuiteSetup() {
    HAPError err;
    HAPPlatformCreate();
    HAPRawBufferZero(&test, sizeof test);

    // Prepare key-value store.
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    HAPControllerPublicKey controllerPublicKey;
    TestPrepareKeyValueStore(controllerPairingID, &controllerPublicKey);

    // Prepare accessory server storage.
    InitializeIPSessions(ipSessions, ipSessionStates, HAPArrayCount(ipSessions));
    ipAccessoryServerStorage = { .sessions = ipSessions,
                                 .numSessions = HAPArrayCount(ipSessions),
                                 .readContexts = ipReadContexts,
                                 .numReadContexts = HAPArrayCount(ipReadContexts),
                                 .writeContexts = ipWriteContexts,
                                 .numWriteContexts = HAPArrayCount(ipWriteContexts),
                                 .scratchBuffer = { .bytes = ipScratchBuffer, .numBytes = sizeof ipScratchBuffer } };

    // Prepare HomeKit Data Stream.
    HAPPlatformTCPStreamManagerOptions dataStreamManagerOptions = {
        .tcpStreams = dataStreamTCPStreams, .numTCPStreams = HAPArrayCount(dataStreamTCPStreams), .numBufferBytes = 8
    };
    HAPPlatformTCPStreamManagerCreate(&dataStreamTCPStreamManager, &dataStreamManagerOptions);

    // Initialize accessory server.
    HAPAssert(HAPGetCompatibilityVersion() == HAP_COMPATIBILITY_VERSION);
    HAPAccessoryServerOptions accessoryServerOptions;
    HAPRawBufferZero(&accessoryServerOptions, sizeof(accessoryServerOptions));
    accessoryServerOptions.maxPairings = kHAPPairingStorage_MinElements;
    accessoryServerOptions.dataStream.dataStreams = dataStreams;
    accessoryServerOptions.dataStream.numDataStreams = HAPArrayCount(dataStreams);
    accessoryServerOptions.ip.transport = &kHAPAccessoryServerTransport_IP;
    accessoryServerOptions.ip.accessoryServerStorage = &ipAccessoryServerStorage;
    accessoryServerOptions.ip.dataStream.tcpStreamManager = &dataStreamTCPStreamManager;
    accessoryServerCallbacks = { .handleUpdatedState = HAPAccessoryServerHelperHandleUpdatedAccessoryServerState };

    HAPAccessoryServerCreate(&accessoryServer, &accessoryServerOptions, &platform, &accessoryServerCallbacks, NULL);

    // Initialize UARP.
    UARPInitialize(&accessoryServer, &accessory);

    // Register for firmware assets.
    UARPVersion stagedVersion;
    HAPRawBufferZero(&stagedVersion, sizeof(stagedVersion));
    uarpFirmwareAssetCallbacks = { .assetOffered = FwUpAssetOffered,
                                   .assetMetadataTLV = FwUpAssetMetadataTLV,
                                   .assetMetadataComplete = FwUpAssetMetadataComplete,
                                   .payloadReady = FwUpPayloadReady,
                                   .payloadMetadataTLV = FwUpPayloadMetadataTLV,
                                   .payloadMetadataComplete = FwUpPayloadMetadataComplete,
                                   .payloadData = FwUpPayloadData,
                                   .payloadDataComplete = FwUpPayloadDataComplete,
                                   .assetStateChange = FwUpAssetStateChange,
                                   .applyStagedAssets = FwUpApplyStagedAssets,
                                   .retrieveLastError = FwUpRetrieveLastError };
    err = UARPRegisterFirmwareAsset(&uarpFirmwareAssetCallbacks, &stagedVersion);
    HAPAssert(!err);

    HAPDataStreamDispatcherOptions dataStreamDispatacherOptions = { .storage = &dataStreamDispatcherStorage };
    // Initialize HomeKit Data Stream dispatcher.
    HAPDataStreamDispatcherCreate(&accessoryServer, &dataStreamDispatcher, &dataStreamDispatacherOptions);

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
    static HAPSession session;
    TestCreateFakeSecuritySession(&session, &accessoryServer, controllerPairingID);

    // Check HomeKit Data Stream Version.
    DataStreamTestHelpersCheckVersion(&accessoryServer, &accessory, &session, NULL);

    // Get Supported Data Stream Transport Configuration.
    bool supportsTCP;
    bool supportsHAP;
    DataStreamTestHelpersReadSupportedDataStreamTransportConfiguration(
            &accessoryServer, &accessory, &session, &supportsTCP, &supportsHAP, NULL);
    HAPAssert(!supportsHAP);
    HAPAssert(supportsTCP);

    InitializeControllerDataStreams(
            &accessoryServer, &session, &dataStreamTCPStreamManager, uarpControllers, HAPArrayCount(uarpControllers));
}

static void TestSuiteTeardown() {
    HAPError err;

    // Stop accessory server.
    HAPLog(&kHAPLog_Default, "Stopping accessory server.");
    HAPAccessoryServerForceStop(&accessoryServer);
    HAPPlatformClockAdvance(0);
    HAPPlatformClockAdvance(0);
    HAPAssert(HAPAccessoryServerGetState(&accessoryServer) == kHAPAccessoryServerState_Idle);

    // Check that TCP streams have been closed.
    {
        uint8_t bytes[1024];
        size_t numBytes;
        for (size_t i = 0; i < HAPArrayCount(uarpControllers); i++) {
            err = HAPPlatformTCPStreamClientRead(
                    &dataStreamTCPStreamManager,
                    uarpControllers[i].hds.clientTCPStream,
                    bytes,
                    sizeof bytes,
                    &numBytes);
            HAPAssert(!err);
            HAPAssert(!numBytes);
            HAPPlatformTCPStreamManagerClientClose(&dataStreamTCPStreamManager, uarpControllers[i].hds.clientTCPStream);
        }
    }

    // Ensure TCP stream manager was properly cleaned up.
    HAPAssert(!HAPPlatformTCPStreamManagerIsListenerOpen(&dataStreamTCPStreamManager));

    // Ensure UARP has released controller buffers associated with the HDS streams.
    TEST_ASSERT(!uarpAccessory.uarpPlatformControllerBufferIsAllocated);
}

static void SetTestOptionsToDefault() {
    HAPRawBufferZero(&testOptions, sizeof(testOptions));
    testOptions.assetVersion = &testAssetVersion;
    testAssetVersion.major = kDefaultAssetVersionMajor;
    testAssetVersion.minor = kDefaultAssetVersionMinor;
    testAssetVersion.release = kDefaultAssetVersionRelease;
    testAssetVersion.build = kDefaultAssetVersionBuild;
}

static void VerifyCleanTestState() {
    // Clean test state is currently characterized as:
    // 1. No pending sends to any controllers.
    // 2. UARP has released all expected buffers.
    // 3. The UARP firmware version matches accessory.firmwareVersion.
    // 4. There is no staged asset.
    // 5. No accessory callbacks are expected.
    // Note: If any other common errors are encountered between tests, please add a check in this function.
    SetTestOptionsToDefault();

    // Ensure there are no pending sends to any controllers.
    UARPController* controller;
    for (size_t i = 0; i < kUARPNumMaxControllers; i++) {
        controller = &(uarpAccessory.controllers[i]);
        TEST_ASSERT(controller);
        for (size_t j = 0; j < kUARPMaxTxBuffersPerController; j++) {
            // If this asserts, there is a pending send from the Accessory to the Controller (application layer
            // message).
            TEST_ASSERT(!controller->uarpTxBuffer[j]);
            // If this asserts, there is a pending send from the Accessory to the Controller (serializing the encoded
            // message).
            TEST_ASSERT(!controller->streamTxScratchBuffer[j]);
        }
    }

    // Ensure UARP has released all expected buffers.
    TEST_ASSERT(!uarpAccessory.uarpPlatformAssetBufferIsAllocated);
    TEST_ASSERT(!uarpAccessory.uarpDataWindowBufferIsAllocated);

    // Since we are not actually applying assets to the accessory (see FwUpApplyStagedAssets),
    // make sure the accessory firmware version remains unchanged from it's initial value.
    int major = 0;
    int minor = 0;
    int revision = 0;
    sscanf(accessory.firmwareVersion, "%d.%d.%d", &major, &minor, &revision);
    UARPVersion expectedVersion = { .major = static_cast<uint32_t>(major),
                                    .minor = static_cast<uint32_t>(minor),
                                    .release = static_cast<uint32_t>(revision),
                                    .build = 0 };
    TestAccessoryInfoQueryHelperFwVersion(
            &dataStreamTCPStreamManager,
            uarpControllers,
            kUARPTLVAccessoryInformationFirmwareVersion,
            &expectedVersion,
            &testOptions);

    // Verify that there is no staged asset.
    HAPRawBufferZero(&expectedVersion, sizeof(expectedVersion));
    TestAccessoryInfoQueryHelperFwVersion(
            &dataStreamTCPStreamManager,
            uarpControllers,
            kUARPTLVAccessoryInformationStagedFirmwareVersion,
            &expectedVersion,
            &testOptions);

    // Ensure there are no outstanding accessory callbacks expected.
    TEST_ASSERT(!test.expectAssetOfferedCallback);
    TEST_ASSERT(!test.expectAssetMetadataCallback);
    TEST_ASSERT(!test.expectAssetMetadataCompleteCallback);
    TEST_ASSERT(!test.expectPayloadMetadataCallback);
    TEST_ASSERT(!test.expectPayloadMetadataCompleteCallback);
    TEST_ASSERT(!test.expectPayloadDataCallback);
    TEST_ASSERT(!test.expectPayloadDataCompleteCallback);
    TEST_ASSERT(!test.expectAssetStateChangeCallback);
    TEST_ASSERT(!test.expectAssetApplyCallback);
    TEST_ASSERT(!test.expectLastErrorCallback);
}

TEST_SETUP() {
    // Runs before every TEST()
    HAPRawBufferZero(&test, sizeof test);
    VerifyCleanTestState();
    SetTestOptionsToDefault();
}

TEST_TEARDOWN() {
    // Runs after every TEST()
    VerifyCleanTestState();
}

TEST(TestStandardTransfer) {
    // Test standard SuperBinary transfer flow.
    TestSuperBinaryTransfer(&dataStreamTCPStreamManager, uarpControllers, &testOptions);
}

TEST(TestUARPOfferRefusal) {
    // Test uarp asset offer refusal.
    // Staged asset version is newer then offered asset version.
    testOptions.skipApplyStagedAssets = true;
    TestSuperBinaryTransfer(&dataStreamTCPStreamManager, uarpControllers, &testOptions);
    SetTestOptionsToDefault();
    testOptions.testUARPOfferRefusal = true;
    testOptions.assetVersion->release -= 1;
    TestSuperBinaryTransfer(&dataStreamTCPStreamManager, uarpControllers, &testOptions);

    // This is to clear out the staged version that has not been applied.
    HAPError err = UARPRequestFirmwareAssetStagingStateChange(kUARPAssetStagingStateChangeRequest_Abandon);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
}

TEST(TestAccessoryAssetApplyRefusal) {
    // Test accessory asset apply refusal.
    testOptions.testAccessoryApplyRefusal = true;
    TestSuperBinaryTransfer(&dataStreamTCPStreamManager, uarpControllers, &testOptions);
    HAPError err = UARPRequestFirmwareAssetStagingStateChange(kUARPAssetStagingStateChangeRequest_Abandon);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
}

TEST(TestAbandonStagedAssetForNewAsset) {
    // Test abandoning of staged asset to accept new asset.
    testOptions.skipApplyStagedAssets = true;
    TestSuperBinaryTransfer(&dataStreamTCPStreamManager, uarpControllers, &testOptions);
    SetTestOptionsToDefault();
    testOptions.assetVersion->release += 1;
    TestSuperBinaryTransfer(&dataStreamTCPStreamManager, uarpControllers, &testOptions);
}

TEST(TestAbandonPausedAssetForNewAssetVersion) {
    // Test newer asset version from a different controller, while staging an asset.
    testOptions.testAccessoryPause = true;
    TestSuperBinaryTransfer(&dataStreamTCPStreamManager, uarpControllers, &testOptions);
    SetTestOptionsToDefault();
    testOptions.assetVersion->release += 1;
    testOptions.controllerIndex += 1;

    // This test is checking UARP's evaluation of asset versions.
    // To sanity check that UARP is not evaluating this asset as a candidate for an asset merge, we
    // change the assetNumPayloads field in the AssetAvailable notification. This value is eventually
    // used to evaluate asset merge candidates, and must be the same for a valid asset merge candidate.
    testOptions.setAssetNumPayloads = true;
    testOptions.assetNumPayloads = kDefaultAssetNumPayloads + 1;
    TestSuperBinaryTransfer(&dataStreamTCPStreamManager, uarpControllers, &testOptions);
}

TEST(TestKeepPausedAssetDenyOlderAssetVersion) {
    // Test older asset version from a different controller, while staging an asset.
    int pausedControllerIndex = 0;
    testOptions.controllerIndex = pausedControllerIndex;
    testOptions.testAccessoryPause = true;
    TestSuperBinaryTransfer(&dataStreamTCPStreamManager, uarpControllers, &testOptions);
    SetTestOptionsToDefault();
    testOptions.testUARPOfferRefusal = true;
    testOptions.assetVersion->release -= 1;
    testOptions.controllerIndex = pausedControllerIndex + 1;

    // This test is checking UARP's evaluation of asset versions.
    // To sanity check that UARP is not evaluating this asset as a candidate for an asset merge, we
    // change the assetNumPayloads field in the AssetAvailable notification. This value is eventually
    // used to evaluate asset merge candidates, and must be the same for a valid asset merge candidate.
    testOptions.setAssetNumPayloads = true;
    testOptions.assetNumPayloads = kDefaultAssetNumPayloads + 1;
    TestSuperBinaryTransfer(&dataStreamTCPStreamManager, uarpControllers, &testOptions);

    // This is to clear out the paused asset.
    HAPError err = UARPRequestFirmwareAssetStagingStateChange(kUARPAssetStagingStateChangeRequest_Abandon);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    // Abandoning a paused asset sends a notification to the controller. We must receive this to free
    // any transmit buffers that are in use.
    ReceiveAssetProcessingNotification(
            &dataStreamTCPStreamManager,
            (UARPControllerContext*) &uarpControllers,
            pausedControllerIndex,
            kUARPAssetProcessingFlagsUploadAbandoned);
}

TEST(TestKeepPausedAssetDenySameAssetVersion) {
    // Test same asset version from a different controller, while staging an asset.
    int pausedControllerIndex = 0;
    testOptions.controllerIndex = pausedControllerIndex;
    testOptions.testAccessoryPause = true;
    TestSuperBinaryTransfer(&dataStreamTCPStreamManager, uarpControllers, &testOptions);
    SetTestOptionsToDefault();
    testOptions.testUARPOfferRefusal = true;
    testOptions.controllerIndex = pausedControllerIndex + 1;

    // This test is checking UARP's evaluation of asset versions.
    // To sanity check that UARP is not evaluating this asset as a candidate for an asset merge, we
    // change the assetNumPayloads field in the AssetAvailable notification. This value is eventually
    // used to evaluate asset merge candidates, and must be the same for a valid asset merge candidate.
    testOptions.setAssetNumPayloads = true;
    testOptions.assetNumPayloads = kDefaultAssetNumPayloads + 1;
    TestSuperBinaryTransfer(&dataStreamTCPStreamManager, uarpControllers, &testOptions);

    // This is to clear out the paused asset.
    HAPError err = UARPRequestFirmwareAssetStagingStateChange(kUARPAssetStagingStateChangeRequest_Abandon);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    // Abandoning a paused asset sends a notification to the controller. We must receive this to free
    // any transmit buffers that are in use.
    ReceiveAssetProcessingNotification(
            &dataStreamTCPStreamManager,
            (UARPControllerContext*) &uarpControllers,
            pausedControllerIndex,
            kUARPAssetProcessingFlagsUploadAbandoned);
}

TEST(TestAccessoryInitiatedPauseResume) {
    // Test accessory-initiated pause and resume.
    testOptions.testAccessoryPauseResume = true;
    TestSuperBinaryTransfer(&dataStreamTCPStreamManager, uarpControllers, &testOptions);
}

TEST(TestAccessoryInitiatedAbandon) {
    // Test accessory-initiated abandon.
    testOptions.testAccessoryAbandon = true;
    TestSuperBinaryTransfer(&dataStreamTCPStreamManager, uarpControllers, &testOptions);
}

TEST(TestControllerRemovalAfterStagingAsset) {
    // Test controller removal after staging asset.
    testOptions.skipApplyStagedAssets = true;
    TestSuperBinaryTransfer(&dataStreamTCPStreamManager, uarpControllers, &testOptions);
    HAPError err = UARPRequestFirmwareAssetStagingStateChange(kUARPAssetStagingStateChangeRequest_Abandon);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
}

TEST(TestControllerTimeoutResume) {
    // Test controller timeout (resume).
    testOptions.testControllerTimeoutResume = true;
    TestSuperBinaryTransfer(&dataStreamTCPStreamManager, uarpControllers, &testOptions);
}

TEST(TestControllerTimeoutAssetMerge) {
    // Test controller timeout (asset merge).
    testOptions.testControllerTimeout = true;
    TestSuperBinaryTransfer(&dataStreamTCPStreamManager, uarpControllers, &testOptions);
    HAPRawBufferZero(&testOptions, sizeof(testOptions));
    testOptions.controllerIndex = 1;
    testOptions.testAssetMerge = true;
    testOptions.assetMergePayloadOffset = test.expectedPayload.offset;
    TestSuperBinaryTransfer(&dataStreamTCPStreamManager, uarpControllers, &testOptions);
}

TEST(TestAssetCorrupt) {
    // Test UARP stack detecting SB header is corrupt/invalid.
    testOptions.testAssetCorrupt = true;
    TestSuperBinaryTransfer(&dataStreamTCPStreamManager, uarpControllers, &testOptions);
}

TEST(TestMultiPayloadSuperBinary) {
    // Test staging of a SB containing multiple payloads.
    testOptions.setAssetNumPayloads = true;
    testOptions.assetNumPayloads = 2;
    TestSuperBinaryTransfer(&dataStreamTCPStreamManager, uarpControllers, &testOptions);
}

TEST(TestStagingSubsetMultiPayloadSuperBinary) {
    // Test staging of a subset of payloads of a SB containing multiple payloads.
    testOptions.setAssetNumPayloads = true;
    testOptions.assetNumPayloads = 2;
    testOptions.testAccessoryPayloadSubset = true;
    TestSuperBinaryTransfer(&dataStreamTCPStreamManager, uarpControllers, &testOptions);
}

TEST(TestStagingSubsetMultiPayloadSuperBinaryWithMetadata) {
    // Test staging of a subset of payloads of a SB containing multiple payloads.
    // Specifically, tests requesting a new payload index (1) after receiving payload metadata for the initial
    // payload index (0).
    testOptions.setAssetNumPayloads = true;
    testOptions.assetNumPayloads = 2;
    testOptions.testAccessoryPayloadSubsetWithMetadata = true;
    TestSuperBinaryTransfer(&dataStreamTCPStreamManager, uarpControllers, &testOptions);
}

TEST(TestStagingAtPayloadDataOffset) {
    // Test staging that begins at a certain firmware asset payload data offset
    testOptions.testAccessorySetPayloadDataOffset = true;
    testOptions.payloadDataOffset = kUARPDataChunkSize;
    TestSuperBinaryTransfer(&dataStreamTCPStreamManager, uarpControllers, &testOptions);
}

TEST(TestAccessoryInfo) {
    // Test accessory info.
    TestAccessoryInfoQuery(&dataStreamTCPStreamManager, uarpControllers, &testOptions);
}

TEST(TestTxBufferManagement) {
    // This test verifies we are properly reserving transmit buffers for a uarp session which is actively staging an
    // asset, ensuring resource constraints do not interrupt staging if controllers flood the accessory with uarp
    // sessions.

    // Start staging from controller 0.
    // The testAccessoryPause test option is simply used as an exit point to return with an active staging session.
    int pausedControllerIndex = 0;
    testOptions.controllerIndex = pausedControllerIndex;
    testOptions.testAccessoryPause = true;
    TestSuperBinaryTransfer(&dataStreamTCPStreamManager, uarpControllers, &testOptions);

    // kUARPMaxTxBuffersPerController tx buffers should now be reserved for controller 0's session.
    // Send (kUARPNumTxBuffers - kUARPMaxTxBuffersPerController) version discovery requests from different controllers,
    // which results in an equivalent number of scratch buffers being allocated and held.
    int controllerIndex = 1;
    for (int i = 0; i < kUARPNumTxBuffers - kUARPMaxTxBuffersPerController; i++, controllerIndex++) {
        SendVersionDiscoveryRequest(&dataStreamTCPStreamManager, uarpControllers, controllerIndex);
    }

    // Sending one more should result in the corresponding version discovery response being dropped.
    // A read of the corresponding TCP stream should therefore return busy.
    SendVersionDiscoveryRequest(&dataStreamTCPStreamManager, uarpControllers, controllerIndex);
    {
        uint8_t frameBytes[1024];
        size_t numBytes;
        HAPError err = HAPPlatformTCPStreamClientRead(
                &dataStreamTCPStreamManager,
                uarpControllers[controllerIndex].hds.clientTCPStream,
                frameBytes,
                sizeof frameBytes,
                &numBytes);
        TEST_ASSERT_EQUAL(err, kHAPError_Busy);
    }

    // But sending one from our active staging session should succeed due to the reserved tx buffers.
    // Note this isn't really a practical messaging flow. Rather it allows us to easily verify the staging session has
    // the expected access to necessary buffers.
    SendVersionDiscoveryRequest(&dataStreamTCPStreamManager, uarpControllers, 0);
    ReceiveVersionDiscoveryResponse(&dataStreamTCPStreamManager, uarpControllers, 0);

    // The version discovery responses used to reach the edge condition should be correctly received as well.
    controllerIndex = 1;
    for (int i = 0; i < kUARPNumTxBuffers - kUARPMaxTxBuffersPerController; i++, controllerIndex++) {
        ReceiveVersionDiscoveryResponse(&dataStreamTCPStreamManager, uarpControllers, controllerIndex);
    }

    // This is to clear out the paused asset.
    HAPError err = UARPRequestFirmwareAssetStagingStateChange(kUARPAssetStagingStateChangeRequest_Abandon);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    // Abandoning a paused asset sends a notification to the controller. We must receive this to free any
    // transmit buffers that are in use.
    ReceiveAssetProcessingNotification(
            &dataStreamTCPStreamManager,
            uarpControllers,
            pausedControllerIndex,
            kUARPAssetProcessingFlagsUploadAbandoned);
}

TEST(TestAccessoryInitiatedAbandonInPayloadReady) {
    // Test accessory-initiated abandon in the payloadReady callback.
    testOptions.testAccessoryAbandonInPayloadReady = true;
    TestSuperBinaryTransfer(&dataStreamTCPStreamManager, uarpControllers, &testOptions);
}

TEST(TestAccessoryInitiatedAbandonInPayloadMetadataComplete) {
    // Test accessory-initiated abandon in the payloadMetadataComplete callback.
    testOptions.testAccessoryAbandonInPayloadMetadataComplete = true;
    TestSuperBinaryTransfer(&dataStreamTCPStreamManager, uarpControllers, &testOptions);
}

TEST(TestControllerInitiatedPauseResume) {
    // Test controller-initiated pause and resume.
    testOptions.testControllerPauseResume = true;
    TestSuperBinaryTransfer(&dataStreamTCPStreamManager, uarpControllers, &testOptions);
}

TEST(TestAbandonAssetWithPausedControllerForSameAssetVersion) {
    // Test offering same asset version from a different controller, while original controller has requested a
    // transfer pause. The paused asset should be abandoned with the new asset merged to resume.
    int pausedControllerIndex = 0;
    testOptions.testControllerPause = true;
    TestSuperBinaryTransfer(&dataStreamTCPStreamManager, uarpControllers, &testOptions);

    SetTestOptionsToDefault();
    testOptions.controllerIndex = pausedControllerIndex + 1;
    testOptions.testAssetMerge = true;
    testOptions.assetMergePayloadOffset = test.expectedPayload.offset;
    TestSuperBinaryTransfer(&dataStreamTCPStreamManager, uarpControllers, &testOptions);

    // The paused controller needs to send a resume notification to allow data transfer in any subsequent tests.
    SendAssetDataTransferResumeNotification(
            &dataStreamTCPStreamManager, (UARPControllerContext*) uarpControllers, pausedControllerIndex);
}

TEST(TestDenyAssetWithPausedControllerForOlderAssetVersion) {
    // Test offering older asset version from a different controller, while original controller has requested a
    // transfer pause. The new asset should be denied.
    int pausedControllerIndex = 0;
    testOptions.testControllerPause = true;
    TestSuperBinaryTransfer(&dataStreamTCPStreamManager, uarpControllers, &testOptions);

    SetTestOptionsToDefault();
    testOptions.controllerIndex = pausedControllerIndex + 1;
    testOptions.assetVersion->release -= 1;
    testOptions.testUARPOfferRefusal = true;
    TestSuperBinaryTransfer(&dataStreamTCPStreamManager, uarpControllers, &testOptions);

    // This is to clear out the paused asset.
    HAPError err = UARPRequestFirmwareAssetStagingStateChange(kUARPAssetStagingStateChangeRequest_Abandon);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    // Abandoning a paused asset sends a notification to the controller. We must receive this to free
    // any transmit buffers that are in use.
    ReceiveAssetProcessingNotification(
            &dataStreamTCPStreamManager,
            (UARPControllerContext*) &uarpControllers,
            pausedControllerIndex,
            kUARPAssetProcessingFlagsUploadAbandoned);
    // The paused controller needs to send a resume notification to allow data transfer in any subsequent tests.
    SendAssetDataTransferResumeNotification(
            &dataStreamTCPStreamManager, (UARPControllerContext*) uarpControllers, pausedControllerIndex);
}

TEST(TestAbandonAssetWithPausedControllerForNewAssetVersion) {
    // Test offering newer asset version from a different controller, while original controller has requested a
    // transfer pause. The paused asset should be abandoned and the new asset accepted.
    int pausedControllerIndex = 0;
    testOptions.testControllerPause = true;
    TestSuperBinaryTransfer(&dataStreamTCPStreamManager, uarpControllers, &testOptions);

    SetTestOptionsToDefault();
    testOptions.controllerIndex = pausedControllerIndex + 1;
    testOptions.assetVersion->release += 1;
    TestSuperBinaryTransfer(&dataStreamTCPStreamManager, uarpControllers, &testOptions);
}

#endif // HAP_FEATURE_ENABLED(HAP_FEATURE_UARP_SUPPORT)

//----------------------------------------------------------------------------------------------------------------------

int main(int argc, char** argv) {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_UARP_SUPPORT)
    int execute_result;

    TestSuiteSetup();
    execute_result = EXECUTE_TESTS(argc, (const char**) argv);
    TestSuiteTeardown();

    return execute_result;
#else
    HAPLog(&kHAPLog_Default, "This test is not supported.");
    return 0;
#endif
}
