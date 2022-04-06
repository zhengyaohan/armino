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

#ifdef __cplusplus
extern "C" {
#endif
#include "DataStreamTestHelpers.h"

#include "HAP+API.h"
#include "HAPCharacteristic.h"
#include "HAPDataStream+Internal.h"
#include "HAPDataStream.h"
#include "HAPLog+Attributes.h"
#include "HAPLogSubsystem.h"

#include "HAPPlatform+Init.h"
#include "HAPPlatformTCPStreamManager+Init.h"
#include "HAPPlatformTCPStreamManagerHelper.h"

#include "HAPOPACK.h"
#include "HAPTestController.h"
#include "TemplateDB.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

void DataStreamTestHelpersHandleDataStreamAccept(
        HAPAccessoryServer* server,
        const HAPDataStreamRequest* request,
        HAPDataStreamRef* dataStream,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->transportType == kHAPTransportType_IP);
    HAPPrecondition(request->session);
    HAPPrecondition(request->service == &dataStreamTransportManagementService);
    HAPPrecondition(request->accessory);
    HAPPrecondition(dataStream);
    HAPPrecondition(context);
    TestContext* test = context;

    HAPLogInfo(&kHAPLog_Default, "[%p] HomeKit Data Stream accepted.", (void*) dataStream);
    test->session = request->session;
    test->dataStream = dataStream;
}

void DataStreamTestHelpersHandleDataStreamInvalidate(
        HAPAccessoryServer* server,
        const HAPDataStreamRequest* request,
        HAPDataStreamRef* dataStream,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->transportType == kHAPTransportType_IP);
    HAPPrecondition(request->session);
    HAPPrecondition(request->service == &dataStreamTransportManagementService);
    HAPPrecondition(request->accessory);
    HAPPrecondition(dataStream);
    HAPPrecondition(context);
    TestContext* test = context;
    HAPPrecondition(request->session == test->session);
    HAPPrecondition(test->dataStream == dataStream);

    HAPLogInfo(&kHAPLog_Default, "HomeKit Data Stream invalidated.");
    HAPRawBufferZero(test, sizeof *test);
}

void DataStreamTestHelpersHandleDataStreamData(
        HAPAccessoryServer* server,
        const HAPDataStreamRequest* request,
        HAPDataStreamRef* dataStream,
        size_t totalDataBytes,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->transportType == kHAPTransportType_IP);
    HAPPrecondition(request->session);
    HAPPrecondition(request->service == &dataStreamTransportManagementService);
    HAPPrecondition(request->accessory);
    HAPPrecondition(dataStream);
    HAPPrecondition(context);
    TestContext* test = context;
    HAPPrecondition(request->session == test->session);
    HAPPrecondition(test->dataStream == dataStream);

    HAPLogInfo(&kHAPLog_Default, "HomeKit Data Stream data (%zu bytes).", totalDataBytes);
    HAPAssert(test->expectsData);
    test->expectsData = false;
    HAPAssert(test->numNextDataBytes <= totalDataBytes);
    HAPAssert(!test->expectsCompletion);
    test->expectsCompletion = true;
    if (test->nextDataBytes) {
        HAPDataStreamReceiveData(
                server,
                dataStream,
                HAPNonnullVoid(test->nextDataBytes),
                test->numNextDataBytes,
                DataStreamTestHelpersHandleDataStreamDataComplete);
    } else {
        HAPDataStreamSkipData(
                server, dataStream, test->numNextDataBytes, DataStreamTestHelpersHandleDataStreamDataComplete);
    }
}

const HAPDataStreamCallbacks dataStreamTestHelpersDataStreamCallbacks = {
    .handleAccept = DataStreamTestHelpersHandleDataStreamAccept,
    .handleInvalidate = DataStreamTestHelpersHandleDataStreamInvalidate,
    .handleData = DataStreamTestHelpersHandleDataStreamData
};

void DataStreamTestHelpersHandleDataStreamDataComplete(
        HAPAccessoryServer* server,
        const HAPDataStreamRequest* request,
        HAPDataStreamRef* dataStream,
        HAPError error,
        void* _Nullable dataBytes,
        size_t numDataBytes,
        bool isComplete,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->transportType == kHAPTransportType_IP);
    HAPPrecondition(request->session);
    HAPPrecondition(request->service == &dataStreamTransportManagementService);
    HAPPrecondition(request->accessory);
    HAPPrecondition(dataStream);
    HAPPrecondition(!error);
    HAPPrecondition(context);
    TestContext* test = context;
    HAPPrecondition(request->session == test->session);
    HAPPrecondition(test->dataStream == dataStream);

    if (isComplete) {
        HAPLogInfo(&kHAPLog_Default, "[%p] Transmission completed.", dataStream);
    } else {
        HAPLogInfo(&kHAPLog_Default, "[%p] Partial transmission completed (%zu bytes).", dataStream, numDataBytes);
    }
    HAPAssert(test->expectsCompletion);
    test->expectsCompletion = false;
    test->error = error;
    test->dataBytes = dataBytes;
    test->numDataBytes = numDataBytes;
    test->isComplete = isComplete;
}

void DataStreamTestHelpersHandleDataStreamSendDataComplete(
        HAPAccessoryServer* server,
        const HAPDataStreamRequest* request,
        HAPDataStreamRef* dataStream,
        HAPError error,
        void* _Nullable dataBytes,
        size_t numDataBytes,
        bool isComplete,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->transportType == kHAPTransportType_IP);
    HAPPrecondition(request->session);
    HAPPrecondition(request->service == &dataStreamTransportManagementService);
    HAPPrecondition(request->accessory);
    HAPPrecondition(dataStream);
    HAPPrecondition(!error);
    HAPPrecondition(context);
    TestContext* test = context;
    HAPPrecondition(request->session == test->session);
    HAPPrecondition(test->dataStream == dataStream);

    if (isComplete) {
        HAPLogInfo(&kHAPLog_Default, "[%p] Transmission completed.", dataStream);
    }
    HAPAssert(test->expectsCompletion);

    HAPAssert(!error);
    HAPAssert(!dataBytes);
    HAPAssert(!numDataBytes);
    HAPAssert(!isComplete);

    // Do the send
    HAPAssert(test->dataBytesToSend); // Test needed to have set this.
    test->expectsCompletion = true;
    HAPDataStreamSendMutableData(
            server,
            dataStream,
            test->dataBytesToSend,
            test->numDataBytesToSend,
            DataStreamTestHelpersHandleDataStreamDataComplete);
}

void DataStreamTestHelpersCheckVersion(
        HAPAccessoryServer* server,
        HAPAccessory* accessory,
        HAPSession* session,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);
    HAPPrecondition(session);
    HAPError err;

    HAPLogInfo(&kHAPLog_Default, "Reading Version.");

    char bytes[64];

    const HAPStringCharacteristicReadRequest request = { .transportType = kHAPTransportType_IP,
                                                         .session = session,
                                                         .characteristic =
                                                                 &dataStreamTransportManagementVersionCharacteristic,
                                                         .service = &dataStreamTransportManagementService,
                                                         .accessory = accessory };
    err = HAPHandleDataStreamTransportManagementVersionRead(server, &request, bytes, sizeof bytes, context);
    HAPAssert(!err);

    // Check returned HomeKit Data Stream version.
    HAPAssert(HAPStringAreEqual(bytes, "1.0"));
}

void DataStreamTestHelpersPrepareAndSendMutableData(
        HAPAccessoryServer* server,
        HAPDataStreamRef* dataStream,
        uint8_t* dataBytes,
        size_t numDataBytes,
        TestContext* test) {

    // Prepare sending data.
    test->expectsCompletion = true;
    test->dataBytesToSend = dataBytes;
    test->numDataBytesToSend = numDataBytes;
    HAPDataStreamPrepareData(server, dataStream, numDataBytes, DataStreamTestHelpersHandleDataStreamSendDataComplete);
}

void DataStreamTestHelpersReadSupportedDataStreamTransportConfiguration(
        HAPAccessoryServer* server,
        HAPAccessory* accessory,
        HAPSession* session,
        bool* supportsTCP,
        bool* supportsHAP,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);
    HAPPrecondition(session);
    HAPPrecondition(supportsTCP);
    HAPPrecondition(supportsHAP);
    HAPError err;

    HAPLogInfo(&kHAPLog_Default, "Reading Supported Data Stream Transport Configuration.");
    *supportsTCP = false;
    *supportsHAP = false;

    uint8_t bytes[1024];
    HAPTLVWriter responseWriter;
    HAPTLVWriterCreate(&responseWriter, bytes, sizeof bytes);

    const HAPTLV8CharacteristicReadRequest request = {
        .transportType = kHAPTransportType_IP,
        .session = session,
        .characteristic = &dataStreamTransportManagementSupportedDataStreamTransportConfigurationCharacteristic,
        .service = &dataStreamTransportManagementService,
        .accessory = accessory
    };
    err = HAPHandleDataStreamTransportManagementSupportedDataStreamTransportConfigurationRead(
            server, &request, &responseWriter, context);
    HAPAssert(!err);

    void* responseBytes;
    size_t numResponseBytes;
    HAPTLVWriterGetBuffer(&responseWriter, &responseBytes, &numResponseBytes);

    HAPTLVReader responseReader;
    HAPTLVReaderCreate(&responseReader, responseBytes, numResponseBytes);

    // Read configurations.
    for (;;) {
        HAPTLV tlv;
        bool valid;
        err = HAPTLVReaderGetNext(&responseReader, &valid, &tlv);
        HAPAssert(!err);

        if (!valid) {
            break;
        }

        switch (tlv.type) {
            case 1: {
                // Transfer Transport Configuration.
                HAPTLVReader subReader;
                HAPTLVReaderCreate(&subReader, (void*) (uintptr_t) tlv.value.bytes, tlv.value.numBytes);

                HAPTLV transportTypeTLV;
                transportTypeTLV.type = 1;
                err = HAPTLVReaderGetAll(&subReader, (HAPTLV* const[]) { &transportTypeTLV, NULL });
                HAPAssert(!err);

                // Transport Type.
                HAPAssert(transportTypeTLV.value.bytes);
                HAPAssert(transportTypeTLV.value.numBytes == sizeof(uint8_t));
                uint8_t transportType = *(const uint8_t*) transportTypeTLV.value.bytes;
                switch (transportType) {
                    case 0: {
                        HAPLogCharacteristicInfo(
                                &kHAPLog_Default,
                                request.characteristic,
                                request.service,
                                request.accessory,
                                "- HomeKit Data Stream over TCP.");
                        HAPAssert(!*supportsTCP);
                        *supportsTCP = true;
                        break;
                    }
                    case 1: {
                        HAPLogCharacteristicInfo(
                                &kHAPLog_Default,
                                request.characteristic,
                                request.service,
                                request.accessory,
                                "- HomeKit Data Stream over HAP.");
                        HAPAssert(!*supportsHAP);
                        *supportsHAP = true;
                        break;
                    }
                    default: {
                        HAPLogCharacteristicError(
                                &kHAPLog_Default,
                                request.characteristic,
                                request.service,
                                request.accessory,
                                "Unexpected Transport Type: %u.",
                                transportType);
                    }
                        HAPFatalError();
                }
                break;
            }
            default: {
                HAPLogCharacteristic(
                        &kHAPLog_Default,
                        request.characteristic,
                        request.service,
                        request.accessory,
                        "Unexpected TLV type: 0x%02X. Treating as separator.",
                        tlv.type);
                break;
            }
        }
    }
}

void DataStreamTestHelpersDataStreamInitEncryptionContext(
        HAPSession* session,
        HAPDataStreamSalt const* controllerKeySalt,
        HAPDataStreamSalt const* accessoryKeySalt,
        DataStreamEncryptionContext* encryptionContext) {
    uint8_t salt[64];

    HAPRawBufferZero(encryptionContext, sizeof *encryptionContext);
    HAPRawBufferCopyBytes(&salt[0], controllerKeySalt->bytes, sizeof controllerKeySalt->bytes);
    HAPRawBufferCopyBytes(&salt[32], accessoryKeySalt->bytes, sizeof accessoryKeySalt->bytes);
    {
        const uint8_t info[] = "HDS-Read-Encryption-Key";
        HAP_hkdf_sha512(
                encryptionContext->accessoryToControllerKey.bytes,
                sizeof encryptionContext->accessoryToControllerKey.bytes,
                ((HAPSession*) session)->hap.cv_KEY,
                sizeof(((HAPSession*) session)->hap.cv_KEY),
                salt,
                sizeof salt,
                info,
                sizeof info - 1);
        HAPLogBuffer(
                &kHAPLog_Default,
                encryptionContext->accessoryToControllerKey.bytes,
                sizeof encryptionContext->accessoryToControllerKey.bytes,
                "AccessoryToControllerEncryptionKey");
    }
    {
        const uint8_t info[] = "HDS-Write-Encryption-Key";
        HAP_hkdf_sha512(
                encryptionContext->controllerToAccessoryKey.bytes,
                sizeof encryptionContext->controllerToAccessoryKey.bytes,
                ((HAPSession*) session)->hap.cv_KEY,
                sizeof(((HAPSession*) session)->hap.cv_KEY),
                salt,
                sizeof salt,
                info,
                sizeof info - 1);
        HAPLogBuffer(
                &kHAPLog_Default,
                encryptionContext->controllerToAccessoryKey.bytes,
                sizeof encryptionContext->controllerToAccessoryKey.bytes,
                "ControllerToAccessoryEncryptionKey");
    }
}

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
        size_t payloadSize) {

    size_t index = 0;

    // Type.
    if (encryptionContext) {
        // Construct the Packet Buffer for an encrypted payload:
        //     Frame Header (4 bytes)
        //         Type (1 byte)
        //         Length (3 bytes)
        //     Payload      (N bytes -- encrypt this)
        //     Auth Tag     (16 bytes)
        HAPAssert(frameSize == HAP_DATASTREAM_ENC_FRAME_MIN_LENGTH + headerSize + payloadSize);
        frame[index++] = kHAPDataStreamFrameType_EncryptedMessage;
    } else {
        // Construct the Packet Buffer for an unencrypted payload:
        //     Frame Header (4 bytes)
        //         Type (1 byte)
        //         Length (3 bytes)
        //     Payload      (N bytes)
        HAPLog(&kHAPLog_Default,
               "frameSize=%zu and headerSize=%zu and payloadSize=%zu",
               frameSize,
               headerSize,
               payloadSize);
        HAPAssert(frameSize == HAP_DATASTREAM_UNENC_FRAME_MIN_LENGTH + headerSize + payloadSize);
        frame[index++] = kHAPDataStreamFrameType_UnencryptedMessage;
    }

    // Length.
    uint8_t payloadLength[3] = { HAPExpandBigUInt24(1 + headerSize + payloadSize) };
    HAPRawBufferCopyBytes(frame + index, payloadLength, sizeof(payloadLength));
    index += sizeof(payloadLength);

    // Payload Header Length (must be 8 bits!)
    HAPAssert(headerSize < 256);
    frame[index++] = (uint8_t) headerSize;

    // Payload Header
    HAPRawBufferCopyBytes(frame + index, header, headerSize);
    index += headerSize;

    // Payload Body
    if (payload) {
        HAPRawBufferCopyBytes(frame + index, payload, payloadSize);
        index += payloadSize;
    }

    // Now do the crypto.
    if (encryptionContext) {
        HAPAssert(index + CHACHA20_POLY1305_TAG_BYTES == frameSize);

        uint8_t* encryptedPayloadPtr = frame + 4;
        size_t encryptedPayloadSize = index - 4;

        uint8_t nonce[] = { HAPExpandLittleUInt64(encryptionContext->controllerToAccessoryCount) };
        encryptionContext->controllerToAccessoryCount++;

        // Init crypto.
        HAP_chacha20_poly1305_ctx crypto;
        HAP_chacha20_poly1305_init(&crypto, nonce, sizeof nonce, encryptionContext->controllerToAccessoryKey.bytes);

        HAP_chacha20_poly1305_update_enc_aad(
                &crypto, frame, 4, nonce, sizeof nonce, encryptionContext->controllerToAccessoryKey.bytes);

        HAP_chacha20_poly1305_update_enc(
                &crypto,
                encryptedPayloadPtr,
                encryptedPayloadPtr,
                encryptedPayloadSize,
                nonce,
                sizeof nonce,
                encryptionContext->controllerToAccessoryKey.bytes);

        HAP_chacha20_poly1305_final_enc(&crypto, frame + index);
    }
}

void DataStreamTestHelpersDataStreamDecryptAccessoryPacketWithContext(
        DataStreamEncryptionContext* encryptionContext,
        uint8_t* frame,
        size_t frameSize,
        uint8_t* _Nonnull* _Nonnull header,
        size_t* headerSize,
        uint8_t* _Nonnull* _Nonnull payload,
        size_t* payloadSize) {
    //
    uint8_t nonce[] = { HAPExpandLittleUInt64(encryptionContext->accessoryToControllerCount) };
    encryptionContext->accessoryToControllerCount++;

    // Decrypt.
    HAPAssert(frameSize >= 4 + CHACHA20_POLY1305_TAG_BYTES);
    int e = HAP_chacha20_poly1305_decrypt_aad(
            &frame[frameSize - CHACHA20_POLY1305_TAG_BYTES],
            &frame[4],
            &frame[4],
            frameSize - CHACHA20_POLY1305_TAG_BYTES - 4,
            &frame[0],
            4,
            nonce,
            sizeof nonce,
            encryptionContext->accessoryToControllerKey.bytes);
    HAPAssert(!e);

    // Type.
    HAPAssert(frame[0] == 0x01);

    // Length.
    size_t frameLength = HAPReadBigUInt24(&frame[1]);
    HAPAssert(frameLength == frameSize - CHACHA20_POLY1305_TAG_BYTES - 4);

    // HeaderLen.
    *header = &frame[5];
    *headerSize = frame[4];
    HAPAssert(*headerSize <= frameSize - CHACHA20_POLY1305_TAG_BYTES - 4 - 1);

    *payload = *header + *headerSize;
    *payloadSize = frameSize - CHACHA20_POLY1305_TAG_BYTES - *headerSize - 4 - 1;
}

HAP_RESULT_USE_CHECK HAPError DataStreamTestHelpersWriteSetupDataStreamTransport(
        HAPAccessoryServer* server,
        HAPAccessory* accessory,
        HAPSession* session,
        const HAPDataStreamSalt* _Nullable controllerKeySalt,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);
    HAPPrecondition(session);
    HAPError err;

    HAPLogInfo(&kHAPLog_Default, "Writing Setup Data Stream Transport.");

    uint8_t bytes[1024];
    HAPTLVWriter requestWriter;
    HAPTLVWriterCreate(&requestWriter, bytes, sizeof bytes);

    // Session Command Type.
    uint8_t sessionCommandTypeBytes[] = {
        kHAPCharacteristicValue_SetupDataStreamTransport_SessionCommandType_StartSession
    };
    err = HAPTLVWriterAppend(
            &requestWriter,
            &(const HAPTLV) {
                    .type = 1,
                    .value = { .bytes = sessionCommandTypeBytes, .numBytes = sizeof sessionCommandTypeBytes } });
    HAPAssert(!err);

    // Transport Type.
    uint8_t transportTypeBytes[] = { kHAPCharacteristicValue_DataStreamTransport_TransportType_TCP };
    if (HAPDataStreamGetActiveTransport(server, kHAPTransportType_IP) == kHAPDataStreamTransport_HAP) {
        transportTypeBytes[0] = kHAPCharacteristicValue_DataStreamTransport_TransportType_HAP;
    }
    err = HAPTLVWriterAppend(
            &requestWriter,
            &(const HAPTLV) { .type = 2,
                              .value = { .bytes = transportTypeBytes, .numBytes = sizeof transportTypeBytes } });
    HAPAssert(!err);

    // Controller Key Salt.
    if (controllerKeySalt) {
        HAPAssert(sizeof controllerKeySalt->bytes == 32);
        err = HAPTLVWriterAppend(
                &requestWriter,
                &(const HAPTLV) {
                        .type = 3,
                        .value = { .bytes = controllerKeySalt->bytes, .numBytes = sizeof controllerKeySalt->bytes } });
        HAPAssert(!err);
    }

    void* requestBytes;
    size_t numRequestBytes;
    HAPTLVWriterGetBuffer(&requestWriter, &requestBytes, &numRequestBytes);
    HAPTLVReader requestReader;
    HAPTLVReaderCreate(&requestReader, requestBytes, numRequestBytes);

    const HAPTLV8CharacteristicWriteRequest request = {
        .transportType = kHAPTransportType_IP,
        .session = session,
        .characteristic = &dataStreamTransportManagementSetupDataStreamTransportCharacteristic,
        .service = &dataStreamTransportManagementService,
        .accessory = accessory,
        .remote = false,
        .authorizationData = { .bytes = NULL, .numBytes = 0 }
    };
    return HAPHandleDataStreamTransportManagementSetupDataStreamTransportWrite(
            server, &request, &requestReader, context);
}

void DataStreamTestHelpersReadSetupDataStreamTransport(
        HAPAccessoryServer* server,
        HAPAccessory* accessory,
        HAPSession* session,
        HAPCharacteristicValue_SetupDataStreamTransport_Status* status,
        HAPNetworkPort* listeningPort,
        HAPDataStreamSalt* accessoryKeySalt,
        HAPDataStreamHAPSessionIdentifier* sessionIdentifier,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);
    HAPPrecondition(session);
    HAPPrecondition(status);
    HAPPrecondition(listeningPort);
    HAPPrecondition(accessoryKeySalt);
    HAPPrecondition(sessionIdentifier);
    HAPError err;

    HAPLogInfo(&kHAPLog_Default, "Reading Setup Data Stream Transport.");

    uint8_t bytes[1024];
    HAPTLVWriter responseWriter;
    HAPTLVWriterCreate(&responseWriter, bytes, sizeof bytes);

    const HAPTLV8CharacteristicReadRequest request = {
        .transportType = kHAPTransportType_IP,
        .session = session,
        .characteristic = &dataStreamTransportManagementSetupDataStreamTransportCharacteristic,
        .service = &dataStreamTransportManagementService,
        .accessory = accessory
    };
    err = HAPHandleDataStreamTransportManagementSetupDataStreamTransportRead(
            server, &request, &responseWriter, context);
    HAPAssert(!err);

    void* responseBytes;
    size_t numResponseBytes;
    HAPTLVWriterGetBuffer(&responseWriter, &responseBytes, &numResponseBytes);

    HAPTLVReader responseReader;
    HAPTLVReaderCreate(&responseReader, responseBytes, numResponseBytes);

    HAPTLV statusTLV, sessionParametersTLV, accessoryKeySaltTLV;
    statusTLV.type = kHAPCharacteristicValue_SetupDataStreamTransport_Response_Status;
    sessionParametersTLV.type = kHAPCharacteristicValue_SetupDataStreamTransport_Response_SessionParameters;
    accessoryKeySaltTLV.type = kHAPCharacteristicValue_SetupDataStreamTransport_Response_AccessoryKeySalt;
    err = HAPTLVReaderGetAll(
            &responseReader, (HAPTLV* const[]) { &statusTLV, &sessionParametersTLV, &accessoryKeySaltTLV, NULL });
    HAPAssert(!err);

    // Status.
    HAPAssert(statusTLV.value.bytes);
    HAPAssert(statusTLV.value.numBytes == sizeof(uint8_t));
    *status = *(const uint8_t*) statusTLV.value.bytes;

    // Transport Type Session Parameters
    HAPTLVReader subReader;
    HAPTLVReaderCreate(
            &subReader, (void*) (uintptr_t) sessionParametersTLV.value.bytes, sessionParametersTLV.value.numBytes);

    HAPTLV listeningPortTLV;
    listeningPortTLV.type = kHAPCharacteristicValue_DataStreamTransport_SessionParameter_TCP_ListeningPort;
    HAPTLV sessionIdentifierTLV;
    sessionIdentifierTLV.type = kHAPCharacteristicValue_DataStreamTransport_SessionParameter_SessionIdentifier;
    err = HAPTLVReaderGetAll(&subReader, (HAPTLV* const[]) { &listeningPortTLV, &sessionIdentifierTLV, NULL });
    HAPAssert(!err);

    // TCP Listening Port.
    if (listeningPortTLV.value.bytes) {
        HAPAssert(listeningPortTLV.value.numBytes == sizeof *listeningPort);
        HAPAssert(sizeof *listeningPort == sizeof(uint16_t));
        *listeningPort = HAPReadLittleUInt16(listeningPortTLV.value.bytes);
    } else {
        *listeningPort = kHAPNetworkPort_Any;
    }

    // Accessory Key Salt.
    if (accessoryKeySaltTLV.value.bytes) {
        HAPAssert(accessoryKeySaltTLV.value.numBytes == sizeof accessoryKeySalt->bytes);
        HAPAssert(sizeof accessoryKeySalt->bytes == 32);
        HAPRawBufferCopyBytes(
                accessoryKeySalt->bytes,
                HAPNonnullVoid(accessoryKeySaltTLV.value.bytes),
                accessoryKeySaltTLV.value.numBytes);
    } else {
        HAPRawBufferZero(accessoryKeySalt->bytes, sizeof(accessoryKeySalt->bytes));
    }

    // Session Identifier.
    if (sessionIdentifierTLV.value.bytes) {
        HAPAssert(sessionIdentifierTLV.value.numBytes == sizeof *sessionIdentifier);
        HAPAssert(sizeof *sessionIdentifier == sizeof(uint8_t));
        *sessionIdentifier = HAPReadUInt8(sessionIdentifierTLV.value.bytes);
    } else {
        *sessionIdentifier = kHAPDataStreamHAPSessionIdentifierNone;
    }
}

void DataStreamTestHelpersHandleTargetControlIdentifierUpdate(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPTargetControlDataStreamProtocol* dataStreamProtocol,
        const HAPServiceRequest* request,
        HAPDataStreamHandle dataStream,
        HAPTargetControlDataStreamProtocolTargetIdentifier targetIdentifier,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol);
    HAPPrecondition(request);
    HAPPrecondition(dataStream < kApp_NumDataStreams);
    HAPPrecondition(context);
    DataSendTestContext* test = context;

    HAPLogInfo(&kHAPLog_Default, "Target Control identifier updated: %lu.", (unsigned long) targetIdentifier);
    HAPAssert(test->dataSendAccepted);
    HAPAssert(dataStream == test->dataStream);
    test->targetIdentifier = targetIdentifier;
}

void DataStreamTestHelpersSendHelloRequest(
        HAPPlatformTCPStreamManager* dataStreamTCPStreamManager,
        HAPPlatformTCPStreamRef* clientTCPStream,
        DataStreamEncryptionContext* encryptionContext) {
    HAPPrecondition(dataStreamTCPStreamManager);
    HAPPrecondition(clientTCPStream);
    HAPPrecondition(encryptionContext);

    HAPLogDebug(&kHAPLog_Default, "Sending control.hello request.");
    // clang-format off
    const uint8_t header[] = {
            0xE3,
            0x48, 'p', 'r', 'o', 't', 'o', 'c', 'o', 'l',
            0x47, 'c', 'o', 'n', 't', 'r', 'o', 'l',
            0x47, 'r', 'e', 'q', 'u', 'e', 's', 't',
            0x45, 'h', 'e', 'l', 'l', 'o',
            0x42, 'i', 'd',
            0x08
    };
    const uint8_t message[] = {
            0xE0
    };
    // clang-format on
    {
        uint8_t frame[HAP_DATASTREAM_ENC_FRAME_MIN_LENGTH + sizeof header + sizeof message];
        DataStreamTestHelpersDataStreamEncryptControllerPacketWithContext(
                encryptionContext, frame, sizeof(frame), header, sizeof(header), message, sizeof(message));

        size_t numBytes;
        HAPError err = HAPPlatformTCPStreamClientWrite(
                dataStreamTCPStreamManager, *clientTCPStream, frame, sizeof frame, &numBytes);
        HAPAssert(!err);
        HAPAssert(numBytes == sizeof frame);
    }
}

void DataStreamTestHelpersReceiveHelloResponse(
        HAPPlatformTCPStreamManager* dataStreamTCPStreamManager,
        HAPPlatformTCPStreamRef* clientTCPStream,
        DataStreamEncryptionContext* encryptionContext) {
    HAPPrecondition(dataStreamTCPStreamManager);
    HAPPrecondition(clientTCPStream);
    HAPPrecondition(encryptionContext);
    HAPLogDebug(&kHAPLog_Default, "Receiving control.hello response.");
    uint8_t frameBytes[1024];
    size_t numBytes;

    // Read the frame header to extract the message length.
    HAPError err = HAPPlatformTCPStreamClientRead(
            dataStreamTCPStreamManager, *clientTCPStream, frameBytes, HAP_DATASTREAM_FRAME_HEADER_LENGTH, &numBytes);
    HAPAssert(!err);
    HAPAssert(numBytes == HAP_DATASTREAM_FRAME_HEADER_LENGTH);
    size_t frameLength = HAPReadBigUInt24(&frameBytes[1]);

    // Read the remainder of the message.
    err = HAPPlatformTCPStreamClientRead(
            dataStreamTCPStreamManager,
            *clientTCPStream,
            &frameBytes[HAP_DATASTREAM_FRAME_HEADER_LENGTH],
            frameLength + CHACHA20_POLY1305_TAG_BYTES,
            &numBytes);
    HAPAssert(!err);
    HAPAssert(numBytes == frameLength + CHACHA20_POLY1305_TAG_BYTES);

    uint8_t* header;
    uint8_t* payload;
    size_t headerSize;
    size_t payloadSize;
    DataStreamTestHelpersDataStreamDecryptAccessoryPacketWithContext(
            encryptionContext,
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
        HAPAssert(HAPStringAreEqual(protocolName, "control"));

        // Topic.
        HAPAssert(responseElement.value.exists);
        char* topic;
        err = HAPOPACKReaderGetNextString(&responseElement.value.reader, &topic);
        HAPAssert(!err);
        HAPAssert(HAPStringAreEqual(topic, "hello"));

        // Request ID.
        HAPAssert(idElement.value.exists);
        int64_t requestID;
        err = HAPOPACKReaderGetNextInt(&idElement.value.reader, &requestID);
        HAPAssert(!err);
        HAPAssert(requestID == 0);

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
        HAPOPACKStringDictionaryElement capabilityVersionElement;
        capabilityVersionElement.key = "capability-version";
        err = HAPOPACKStringDictionaryReaderGetAll(
                &messageReader, (HAPOPACKStringDictionaryElement* const[]) { &capabilityVersionElement, NULL });

        // Capability version.
        HAPAssert(capabilityVersionElement.value.exists);
        bool capabilityVersionValue;
        err = HAPOPACKReaderGetNextBool(&capabilityVersionElement.value.reader, &capabilityVersionValue);
        HAPAssert(!err);
        HAPAssert(capabilityVersionValue == true);
    }
}

void DataStreamTestHelpersSendVersionRequest(
        HAPPlatformTCPStreamManager* dataStreamTCPStreamManager,
        HAPPlatformTCPStreamRef* clientTCPStream,
        DataStreamEncryptionContext* encryptionContext,
        float version) {
    HAPPrecondition(dataStreamTCPStreamManager);
    HAPPrecondition(clientTCPStream);
    HAPPrecondition(encryptionContext);

    HAPLogDebug(&kHAPLog_Default, "Sending control.version request.");
    // clang-format off
    const uint8_t header[] = {
            0xE3,
            0x48, 'p', 'r', 'o', 't', 'o', 'c', 'o', 'l',
            0x47, 'c', 'o', 'n', 't', 'r', 'o', 'l',
            0x47, 'r', 'e', 'q', 'u', 'e', 's', 't',
            0x47, 'v', 'e', 'r', 's', 'i', 'o', 'n',
            0x42, 'i', 'd',
            0x09,
    };
    union {
        int32_t intValue;
        float floatValue;
    } versionValue;
    versionValue.floatValue = version;
    const uint8_t message[] = {
            0xE1,
            0x47, 'v', 'e', 'r', 's', 'i', 'o', 'n',
            0x35, HAPExpandLittleInt32(versionValue.intValue)
    };
    // clang-format on
    {
        uint8_t frame[HAP_DATASTREAM_ENC_FRAME_MIN_LENGTH + sizeof header + sizeof message];
        DataStreamTestHelpersDataStreamEncryptControllerPacketWithContext(
                encryptionContext, frame, sizeof(frame), header, sizeof(header), message, sizeof(message));

        size_t numBytes;
        HAPError err = HAPPlatformTCPStreamClientWrite(
                dataStreamTCPStreamManager, *clientTCPStream, frame, sizeof frame, &numBytes);
        HAPAssert(!err);
        HAPAssert(numBytes == sizeof frame);
    }
}

void DataStreamTestHelpersReceiveVersionResponse(
        HAPPlatformTCPStreamManager* dataStreamTCPStreamManager,
        HAPPlatformTCPStreamRef* clientTCPStream,
        DataStreamEncryptionContext* encryptionContext) {
    HAPPrecondition(dataStreamTCPStreamManager);
    HAPPrecondition(clientTCPStream);
    HAPPrecondition(encryptionContext);
    HAPLogDebug(&kHAPLog_Default, "Receiving control.version response.");
    uint8_t frameBytes[1024];
    size_t numBytes;

    // Read the frame header to extract the message length.
    HAPError err = HAPPlatformTCPStreamClientRead(
            dataStreamTCPStreamManager, *clientTCPStream, frameBytes, HAP_DATASTREAM_FRAME_HEADER_LENGTH, &numBytes);
    HAPAssert(!err);
    HAPAssert(numBytes == HAP_DATASTREAM_FRAME_HEADER_LENGTH);
    size_t frameLength = HAPReadBigUInt24(&frameBytes[1]);

    // Read the remainder of the message.
    err = HAPPlatformTCPStreamClientRead(
            dataStreamTCPStreamManager,
            *clientTCPStream,
            &frameBytes[HAP_DATASTREAM_FRAME_HEADER_LENGTH],
            frameLength + CHACHA20_POLY1305_TAG_BYTES,
            &numBytes);
    HAPAssert(!err);
    HAPAssert(numBytes == frameLength + CHACHA20_POLY1305_TAG_BYTES);

    uint8_t* header;
    uint8_t* payload;
    size_t headerSize;
    size_t payloadSize;
    DataStreamTestHelpersDataStreamDecryptAccessoryPacketWithContext(
            encryptionContext,
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
        HAPAssert(HAPStringAreEqual(protocolName, "control"));

        // Topic.
        HAPAssert(responseElement.value.exists);
        char* topic;
        err = HAPOPACKReaderGetNextString(&responseElement.value.reader, &topic);
        HAPAssert(!err);
        HAPAssert(HAPStringAreEqual(topic, "version"));

        // Request ID.
        HAPAssert(idElement.value.exists);
        int64_t requestID;
        err = HAPOPACKReaderGetNextInt(&idElement.value.reader, &requestID);
        HAPAssert(!err);
        HAPAssert(requestID == 1);

        // Response status.
        HAPAssert(statusElement.value.exists);
        int64_t status;
        err = HAPOPACKReaderGetNextInt(&statusElement.value.reader, &status);
        HAPAssert(!err);
        HAPAssert(status == 0);
    }
}

void DataStreamTestHelpersHandleDataSendAccept(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataSendDataStreamProtocol* dataStreamProtocol,
        const HAPServiceRequest* request,
        HAPDataStreamHandle dataStream,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol);
    HAPPrecondition(request);
    HAPPrecondition(dataStream < kApp_NumDataStreams);
    HAPPrecondition(context);
    DataSendTestContext* test = context;

    HAPLogInfo(&kHAPLog_Default, "'dataSend' accepted.");
    HAPAssert(!test->dataSendAccepted);
    test->dataSendAccepted = true;
    test->dataStream = dataStream;
}

void DataStreamTestHelpersHandleDataSendInvalidate(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataSendDataStreamProtocol* dataStreamProtocol,
        const HAPServiceRequest* request,
        HAPDataStreamHandle dataStream,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol);
    HAPPrecondition(request);
    HAPPrecondition(dataStream < kApp_NumDataStreams);
    HAPPrecondition(context);
    DataSendTestContext* test = context;

    HAPLogInfo(&kHAPLog_Default, "'dataSend' invalidated.");
    HAPAssert(!test->dataSendStreamAvailable);
    HAPAssert(test->dataSendAccepted);
    test->dataSendAccepted = false;
    test->dataStream = 0;
    test->targetIdentifier = 0;
}

void DataStreamTestHelpersHandleDataSendStreamAvailable(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataSendDataStreamProtocol* dataStreamProtocol,
        const HAPServiceRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolType type,
        HAPDataSendDataStreamProtocolOpenMetadata* _Nullable metadata,
        void* _Nullable inDataSendStreamCallbacks HAP_UNUSED,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol);
    HAPPrecondition(request);
    HAPPrecondition(dataStream < kApp_NumDataStreams);
    HAPPrecondition(metadata);
    HAPPrecondition(context);
    DataSendTestContext* test = context;

    HAPLogInfo(&kHAPLog_Default, "'dataSend' stream available.");
    HAPAssert(test->dataSendAccepted);
    HAPAssert(!test->dataSendStreamAvailable);

    if (type != kHAPDataSendDataStreamProtocolType_Audio_Siri &&
        type != kHAPDataSendDataStreamProtocolType_IPCamera_Recording) {
        HAPAssert(false);
    }
    test->dataSendStreamAvailable = true;
    test->availableStreamType = type;
}

void DataStreamTestHelpersHandleDataSendStreamClose(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataSendDataStreamProtocol* dataStreamProtocol,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolStream* dataSendStream,
        HAPError error,
        HAPDataSendDataStreamProtocolCloseReason closeReason,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol);
    HAPPrecondition(request);
    HAPPrecondition(dataStream < kApp_NumDataStreams);
    HAPPrecondition(dataSendStream);
    HAPPrecondition(context);
    DataSendTestContext* test = context;

    HAPLogInfo(&kHAPLog_Default, "dataSend stream closed.");
    HAPAssert(test->dataSendStreamValid);
    HAPAssert(!test->expectingSendCompletion);
    HAPAssert(error == test->closeError);
    test->dataSendStreamValid = false;
    test->dataSendStreamOpen = false;
    test->closeReason = closeReason;
}

void DataStreamTestHelpersHandleDataSendStreamOpen(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataSendDataStreamProtocol* dataStreamProtocol,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolStream* dataSendStream,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol);
    HAPPrecondition(request);
    HAPPrecondition(dataStream < kApp_NumDataStreams);
    HAPPrecondition(dataSendStream);
    HAPPrecondition(context);
    DataSendTestContext* test = context;

    HAPLogInfo(&kHAPLog_Default, "dataSend stream opened.");

    HAPAssert(test->dataSendStreamValid);
    test->dataSendStreamOpen = true;
}

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
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol);
    HAPPrecondition(request);
    HAPPrecondition(dataStream < kApp_NumDataStreams);
    HAPPrecondition(dataSendStream);
    HAPPrecondition(!error);
    HAPPrecondition(context);
    DataSendTestContext* test = context;

    HAPLogInfo(&kHAPLog_Default, "Completed sending data over dataSend stream.");

    HAPAssert(test->dataSendStreamValid);
    HAPAssert(test->dataSendStreamOpen);
    HAPAssert(scratchBytes == test->scratchBytes);
    HAPAssert(numScratchBytes == test->numScratchBytes);
    HAPAssert(test->expectingSendCompletion);
    test->expectingSendCompletion = false;
}

HAP_RESULT_USE_CHECK HAPError DataStreamTestHelpersBuildControllerHAPTransportWrite(
        HAPAccessoryServer* server,
        HAPAccessory* accessory,
        HAPSession* session,
        HAPDataStreamHAPSessionIdentifier sessionIdentifier,
        const void* _Nullable payload,
        size_t payloadLength,
        bool forceClose,
        void* _Nullable context) {
    HAPPrecondition(payload || !payloadLength);
    HAPPrecondition(server);
    HAPPrecondition(accessory);
    HAPPrecondition(session);

    HAPError err;

    HAPTLVWriter requestWriter;
    uint8_t frame[20 + payloadLength]; // a bit bigger for all the TLV overhead.
    HAPTLVWriterCreate(&requestWriter, frame, sizeof frame);

    // Session Identifier.
    uint8_t sessionIdentifierBytes[] = { sessionIdentifier };
    err = HAPTLVWriterAppend(
            &requestWriter,
            &(const HAPTLV) {
                    .type = kHAPCharacteristicValue_DataStreamHAPTransport_Request_SessionIdentifier,
                    .value = { .bytes = sessionIdentifierBytes, .numBytes = sizeof sessionIdentifierBytes } });
    HAPAssert(!err);

    // Payload.
    err = HAPTLVWriterAppend(
            &requestWriter,
            &(const HAPTLV) { .type = kHAPCharacteristicValue_DataStreamHAPTransport_Request_Payload,
                              .value = { .bytes = payload, .numBytes = payloadLength } });
    HAPAssert(!err);

    // Force Close.
    if (forceClose) {
        uint8_t forceCloseByte = 1;
        err = HAPTLVWriterAppend(
                &requestWriter,
                &(const HAPTLV) { .type = kHAPCharacteristicValue_DataStreamHAPTransport_Request_ForceClose,
                                  .value = { .bytes = &forceCloseByte, .numBytes = sizeof forceCloseByte } });
        HAPAssert(!err);
    }

    void* requestBytes;
    size_t numRequestBytes;
    HAPTLVWriterGetBuffer(&requestWriter, &requestBytes, &numRequestBytes);
    HAPTLVReader requestReader;
    HAPTLVReaderCreate(&requestReader, requestBytes, numRequestBytes);

    const HAPTLV8CharacteristicWriteRequest request = {
        .transportType = kHAPTransportType_IP,
        .session = session,
        .characteristic = &dataStreamTransportManagementDataStreamHAPTransportCharacteristic,
        .service = &dataStreamTransportManagementService,
        .accessory = accessory,
        .remote = false,
        .authorizationData = { .bytes = NULL, .numBytes = 0 }
    };
    return HAPHandleDataStreamTransportManagementDataStreamHAPTransportWrite(server, &request, &requestReader, context);
}

HAP_RESULT_USE_CHECK HAPError DataStreamTestHelpersPerformControllerHAPTransportRead(
        HAPAccessoryServer* server,
        HAPAccessory* accessory,
        HAPSession* session,
        uint8_t* _Nullable buffer,
        size_t bufferCapacity,
        size_t* bufferLength,
        bool* requestToSend,
        uint8_t* numResponseTLVs,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);
    HAPPrecondition(session);
    HAPPrecondition(bufferLength);
    HAPPrecondition(numResponseTLVs);
    HAPError err;

    uint8_t bytes[1024];
    HAPTLVWriter responseWriter;
    HAPTLVWriterCreate(&responseWriter, bytes, sizeof bytes);
    *numResponseTLVs = 0;

    const HAPTLV8CharacteristicReadRequest request = {
        .transportType = kHAPTransportType_IP,
        .session = session,
        .characteristic = &dataStreamTransportManagementDataStreamHAPTransportCharacteristic,
        .service = &dataStreamTransportManagementService,
        .accessory = accessory
    };
    err = HAPHandleDataStreamTransportManagementDataStreamHAPTransportRead(server, &request, &responseWriter, context);
    if (err) {
        return err;
    }

    void* responseBytes;
    size_t numResponseBytes;
    HAPTLVWriterGetBuffer(&responseWriter, &responseBytes, &numResponseBytes);

    HAPTLVReader responseReader;
    HAPTLVReaderCreate(&responseReader, responseBytes, numResponseBytes);

    HAPTLV payloadTLV, requestToSendTLV;
    payloadTLV.type = kHAPCharacteristicValue_DataStreamHAPTransport_Response_Payload;
    requestToSendTLV.type = kHAPCharacteristicValue_DataStreamHAPTransport_Response_AccessoryRequestToSend;
    err = HAPTLVReaderGetAll(&responseReader, (HAPTLV* const[]) { &payloadTLV, &requestToSendTLV, NULL });
    HAPAssert(!err);

    // Request To Send.
    if (requestToSendTLV.value.numBytes > 0) {
        HAPAssert(requestToSendTLV.value.bytes);
        HAPAssert(requestToSendTLV.value.numBytes == sizeof(uint8_t));
        *requestToSend = *(const uint8_t*) requestToSendTLV.value.bytes;
        (*numResponseTLVs)++;
    } else {
        *requestToSend = false;
    }

    // Payload.
    if (payloadTLV.value.numBytes > 0) {
        HAPAssert(payloadTLV.value.bytes);
        HAPLogInfo(
                &kHAPLog_Default,
                " Payload (%zu bytes) into buffer (%zu bytes).",
                payloadTLV.value.numBytes,
                bufferCapacity);

        HAPAssert(payloadTLV.value.numBytes <= bufferCapacity);
        if (buffer) {
            HAPRawBufferCopyBytes(buffer, payloadTLV.value.bytes, payloadTLV.value.numBytes);
        }
        (*numResponseTLVs)++;
    }
    *bufferLength = payloadTLV.value.numBytes;

    return kHAPError_None;
}

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif
