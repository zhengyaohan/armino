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

#ifndef DATA_STREAM_TEST_HELPERS_H
#define DATA_STREAM_TEST_HELPERS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPDataStream.h"

#include "HAPPlatform+Init.h"
#include "HAPPlatformTCPStreamManager+Init.h"
#include "HAPPlatformTCPStreamManagerHelper.h"

#include "HAPOPACK.h"
#include "HAPTestController.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

typedef struct {
    // Values observed during an "handleAccept" call. (Validated in callback thereafter)
    HAPSession* _Nullable session;
    HAPDataStreamRef* _Nullable dataStream;

    // Tests around the "handleData" call.
    //   - test should set the following fields:
    //      - expectsData - to signal that the 'handleData' call is expected (validated)
    //      - numNextDataBytes - how many bytes to tell the stream to receive
    //      - nextDataBytes - where to receive into (or skip bytes if null)
    // When the 'handleCall' happens, the 'expectsData' value is checked.
    // Then either a 'ReceiveData' or 'SkipData' call is performed, pull the requested amount.
    // This also sets the 'expectsCompletion' flag.
    //
    // When the callback occurs, it validates the 'expectsCompletion' flag. Then it sets the
    // following fields, which should be checked and validated by the test:
    //      - error - whether the data was received with error or not
    //      - dataBytes - the pointer provided
    //      - numDataBytes - the length of the pointer
    //      - isComplete - whether the frame is complete (and decrypted successfully)
    bool expectsData : 1;
    void* _Nullable nextDataBytes;
    size_t numNextDataBytes;

    bool expectsCompletion : 1;
    HAPError error;
    void* _Nullable dataBytes;
    size_t numDataBytes;
    bool isComplete;

    void* _Nullable dataBytesToSend;
    size_t numDataBytesToSend;
} TestContext;

typedef struct {
    HAPSessionKey accessoryToControllerKey;
    HAPSessionKey controllerToAccessoryKey;

    uint64_t accessoryToControllerCount;
    uint64_t controllerToAccessoryCount;
} DataStreamEncryptionContext;

/** The amount of framing overhead required for the internal payload (the one byte for "header length"). */
#define HAP_DATASTREAM_PAYLOAD_HEADER_LENGTH (1)

/** The amount of framing overhead required for each frame. */
#define HAP_DATASTREAM_ENC_FRAME_MIN_LENGTH \
    (HAP_DATASTREAM_FRAME_HEADER_LENGTH + HAP_DATASTREAM_PAYLOAD_HEADER_LENGTH + CHACHA20_POLY1305_TAG_BYTES)

/** The amount of framing overhead required for each frame. */
#define HAP_DATASTREAM_UNENC_FRAME_MIN_LENGTH \
    (HAP_DATASTREAM_FRAME_HEADER_LENGTH + HAP_DATASTREAM_PAYLOAD_HEADER_LENGTH)

extern const HAPDataStreamCallbacks dataStreamTestHelpersDataStreamCallbacks;

void DataStreamTestHelpersHandleDataStreamAccept(
        HAPAccessoryServer* server,
        const HAPDataStreamRequest* request,
        HAPDataStreamRef* dataStream,
        void* _Nullable context);

void DataStreamTestHelpersHandleDataStreamInvalidate(
        HAPAccessoryServer* server,
        const HAPDataStreamRequest* request,
        HAPDataStreamRef* dataStream,
        void* _Nullable context);

void DataStreamTestHelpersHandleDataStreamData(
        HAPAccessoryServer* server,
        const HAPDataStreamRequest* request,
        HAPDataStreamRef* dataStream,
        size_t totalDataBytes,
        void* _Nullable context);

void DataStreamTestHelpersHandleDataStreamDataComplete(
        HAPAccessoryServer* server,
        const HAPDataStreamRequest* request,
        HAPDataStreamRef* dataStream,
        HAPError error,
        void* _Nullable dataBytes,
        size_t numDataBytes,
        bool isComplete,
        void* _Nullable context);

void DataStreamTestHelpersHandleDataStreamSendDataComplete(
        HAPAccessoryServer* server,
        const HAPDataStreamRequest* request,
        HAPDataStreamRef* dataStream,
        HAPError error,
        void* _Nullable dataBytes,
        size_t numDataBytes,
        bool isComplete,
        void* _Nullable context);

void DataStreamTestHelpersCheckVersion(
        HAPAccessoryServer* server,
        HAPAccessory* accessory,
        HAPSession* session,
        void* _Nullable context);

void DataStreamTestHelpersPrepareAndSendMutableData(
        HAPAccessoryServer* server,
        HAPDataStreamRef* dataStream,
        uint8_t* dataBytes,
        size_t numDataBytes,
        TestContext* test);

void DataStreamTestHelpersReadSupportedDataStreamTransportConfiguration(
        HAPAccessoryServer* server,
        HAPAccessory* accessory,
        HAPSession* session,
        bool* supportsTCP,
        bool* supportsHAP,
        void* _Nullable context);

void DataStreamTestHelpersDataStreamInitEncryptionContext(
        HAPSession* session,
        HAPDataStreamSalt const* controllerKeySalt,
        HAPDataStreamSalt const* accessoryKeySalt,
        DataStreamEncryptionContext* encryptionContext);

/**
 * Build a Data Stream frame, with encryption.
 *
 * @param encryptionContext  The keys/nonce to use (if NULL, no encryption is performed)
 * @param frame              The buffer to write the frame into.
 * @param frameSize          The size of the 'frame' buffer (must be exact!).
 * @param header             The buffer to write the header into.
 * @param headerSize         The size of the 'header' buffer.
 * @param payload            The buffer to write the payload into.
 * @param payloadSize        The size of the 'payload' buffer.
 * @param frame              The buffer to write the frame into
 */
void DataStreamTestHelpersDataStreamEncryptControllerPacketWithContext(
        DataStreamEncryptionContext* _Nullable encryptionContext,
        uint8_t* frame,
        size_t frameSize,
        const uint8_t* header,
        size_t headerSize,
        const uint8_t* _Nullable payload,
        size_t payloadSize);

void DataStreamTestHelpersDataStreamDecryptAccessoryPacketWithContext(
        DataStreamEncryptionContext* encryptionContext,
        uint8_t* frame,
        size_t frameSize,
        uint8_t* _Nonnull* _Nonnull header,
        size_t* headerSize,
        uint8_t* _Nonnull* _Nonnull payload,
        size_t* payloadSize);

/**
 * Perform a write to the accessory to start a "Setup" operation.
 *
 * @param  server             Accessory server.
 * @param  accessory          Accessory.
 * @param  session            Session.
 * @param  controllerKeySalt  The Salt the controller sends to accessory.
 * @param  context            Test context.
 */
HAP_RESULT_USE_CHECK HAPError DataStreamTestHelpersWriteSetupDataStreamTransport(
        HAPAccessoryServer* server,
        HAPAccessory* accessory,
        HAPSession* session,
        const HAPDataStreamSalt* _Nullable controllerKeySalt,
        void* _Nullable context);

/**
 * Read back the response the accessory generates in response to a "Setup" operation
 * (DataStreamTestHelpersWriteSetupDataStreamTransport).
 *
 * @param  server             Accessory server.
 * @param  accessory          Accessory.
 * @param  session            Session.
 * @param  status             (Out) the status received (should be 0 on no-error)
 * @param  listeningPort      (Out) the TCP listening port (set to kHAPNetworkPort_Any if none)
 * @param  accessoryKeySalt   (Out) the Salt the accessory sends back to controller (set to zeroes if none)
 * @param  sessionIdentifier  (Out) the Session Identifier chosen by the accessory (set to
 *                            kHAPDataStreamHAPSessionIdentifierNone if none)
 * @param  context            Test context.
 */
void DataStreamTestHelpersReadSetupDataStreamTransport(
        HAPAccessoryServer* server,
        HAPAccessory* accessory,
        HAPSession* session,
        HAPCharacteristicValue_SetupDataStreamTransport_Status* status,
        HAPNetworkPort* listeningPort,
        HAPDataStreamSalt* accessoryKeySalt,
        HAPDataStreamHAPSessionIdentifier* sessionIdentifier,
        void* _Nullable context);

typedef struct {
    bool dataSendAccepted : 1;
    HAPDataStreamHandle dataStream;

    HAPTargetControlDataStreamProtocolTargetIdentifier targetIdentifier;

    HAPDataSendDataStreamProtocolCloseReason closeReason;
    HAPError closeError;
    bool dataSendStreamValid : 1;
    bool dataSendStreamOpen : 1;

    bool dataSendStreamAvailable : 1;
    HAPDataSendDataStreamProtocolType availableStreamType;

    void* _Nullable scratchBytes;
    size_t numScratchBytes;
    bool expectingSendCompletion : 1;
} DataSendTestContext;

void DataStreamTestHelpersHandleTargetControlIdentifierUpdate(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPTargetControlDataStreamProtocol* dataStreamProtocol,
        const HAPServiceRequest* request,
        HAPDataStreamHandle dataStream,
        HAPTargetControlDataStreamProtocolTargetIdentifier targetIdentifier,
        void* _Nullable context);

void DataStreamTestHelpersSendHelloRequest(
        HAPPlatformTCPStreamManager* dataStreamTCPStreamManager,
        HAPPlatformTCPStreamRef* clientTCPStream,
        DataStreamEncryptionContext* encryptionContext);

void DataStreamTestHelpersReceiveHelloResponse(
        HAPPlatformTCPStreamManager* dataStreamTCPStreamManager,
        HAPPlatformTCPStreamRef* clientTCPStream,
        DataStreamEncryptionContext* encryptionContext);

void DataStreamTestHelpersSendVersionRequest(
        HAPPlatformTCPStreamManager* dataStreamTCPStreamManager,
        HAPPlatformTCPStreamRef* clientTCPStream,
        DataStreamEncryptionContext* encryptionContext,
        float version);

void DataStreamTestHelpersReceiveVersionResponse(
        HAPPlatformTCPStreamManager* dataStreamTCPStreamManager,
        HAPPlatformTCPStreamRef* clientTCPStream,
        DataStreamEncryptionContext* encryptionContext);

void DataStreamTestHelpersHandleDataSendAccept(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataSendDataStreamProtocol* dataStreamProtocol,
        const HAPServiceRequest* request,
        HAPDataStreamHandle dataStream,
        void* _Nullable context);

void DataStreamTestHelpersHandleDataSendInvalidate(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataSendDataStreamProtocol* dataStreamProtocol,
        const HAPServiceRequest* request,
        HAPDataStreamHandle dataStream,
        void* _Nullable context);

void DataStreamTestHelpersHandleDataSendStreamAvailable(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataSendDataStreamProtocol* dataStreamProtocol,
        const HAPServiceRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolType type,
        HAPDataSendDataStreamProtocolOpenMetadata* _Nullable metadata,
        void* _Nullable inDataSendStreamCallbacks,
        void* _Nullable context);

void DataStreamTestHelpersHandleDataSendStreamClose(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataSendDataStreamProtocol* dataStreamProtocol,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolStream* dataSendStream,
        HAPError error,
        HAPDataSendDataStreamProtocolCloseReason closeReason,
        void* _Nullable context);

void DataStreamTestHelpersHandleDataSendStreamOpen(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataSendDataStreamProtocol* dataStreamProtocol,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolStream* dataSendStream,
        void* _Nullable context);

void DataStreamTestHelpersHandleSendDataComplete(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataSendDataStreamProtocol* dataStreamProtocol,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolStream* dataSendStream,
        HAPError error,
        void* scratchBytes,
        size_t numScratchBytes,
        void* _Nullable context);

/**
 * Perform a write to the accessory HAP transport characteristic.
 *
 * @param  server             Accessory server.
 * @param  accessory          Accessory.
 * @param  session            Session.
 * @param  sessionIdentifier  Session Identifier (chosen by accessory during setup).
 * @param  payload            Controller HDS write payload.
 * @param  payloadLength      Controller HDS write payload length.
 * @param  forceClose         Data stream force close flag.
 * @param  context            Test context.
 */
HAP_RESULT_USE_CHECK HAPError DataStreamTestHelpersBuildControllerHAPTransportWrite(
        HAPAccessoryServer* server,
        HAPAccessory* accessory,
        HAPSession* session,
        HAPDataStreamHAPSessionIdentifier sessionIdentifier,
        const void* _Nullable payload,
        size_t payloadLength,
        bool forceClose,
        void* _Nullable context);

/**
 * Perform a write to the accessory to start a "Setup" operation.
 *
 * @param  server             Accessory server.
 * @param  accessory          Accessory.
 * @param  session            Session.
 * @param  sessionIdentifier  Session Identifier (chosen by accessory during setup).
 * @param  context            Test context.
 */
HAP_RESULT_USE_CHECK HAPError DataStreamTestHelpersPerformControllerHAPTransportRead(
        HAPAccessoryServer* server,
        HAPAccessory* accessory,
        HAPSession* session,
        uint8_t* _Nullable buffer,
        size_t bufferCapacity,
        size_t* bufferLength,
        bool* requestToSend,
        uint8_t* numResponseTLVs,
        void* _Nullable context);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif // DATA_STREAM_TEST_HELPERS_H
