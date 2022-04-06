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

#include "HAP+API.h"

#include "HAPCharacteristic.h"
#include "HAPCharacteristicTypes.h"
#include "HAPDataStream+HAP.h"
#include "HAPDataStream.h"
#include "HAPLog+Attributes.h"
#include "HAPLogSubsystem.h"
#include "HAPServiceTypes.h"
#include "HAPSession.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "RequestHandlers" };

/**
 * Reserve enough memory to account for additional TLV items that may be appended to
 * the TLV buffer.
 * Additional items may include TLV items such as "Force Close", "Accessory Request to Send More Data",
 * "Controller Clear to Send More Data", BLE PDU related overhead, etc.
 */
#define kHAPDataStreamHAPTransport_ReadOverheadBytes 32

#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
/**
 * Maximum buffer size to be used when the accessory sends data using thread transport
 */
#define kHAPDataStreamHAPTransport_ThreadMaxBufferSizeBytes 1024
#endif

/**
 * Parse the TLVs in the "HAP Transport" write request.
 *
 * @param      requestReader        TLV reader for the setup request.
 * @param      setupParams          Output parameter to hold the parsed fields.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the data types are incorrect (bad enum values, etc).
 */
HAP_RESULT_USE_CHECK
static HAPError ParseDataStreamHAPTransportWriteRequest(
        const HAPTLV8CharacteristicWriteRequest* request,
        HAPTLVReader* requestReader,
        HAPDataStreamHAPSessionIdentifier* sessionIdentifier,
        const uint8_t** payload,
        size_t* payloadSize,
        bool* forceClose) {
    HAPPrecondition(request);
    HAPPrecondition(requestReader);
    HAPPrecondition(sessionIdentifier);
    HAPPrecondition(payload);
    HAPPrecondition(payloadSize);
    HAPPrecondition(forceClose);

    HAPError err;

    HAPTLV sessionIdentifierTLV, payloadTLV, forceCloseTLV;
    sessionIdentifierTLV.type = kHAPCharacteristicValue_DataStreamHAPTransport_Request_SessionIdentifier;
    payloadTLV.type = kHAPCharacteristicValue_DataStreamHAPTransport_Request_Payload;
    forceCloseTLV.type = kHAPCharacteristicValue_DataStreamHAPTransport_Request_ForceClose;
    err = HAPTLVReaderGetAll(
            requestReader, (HAPTLV* const[]) { &sessionIdentifierTLV, &payloadTLV, &forceCloseTLV, NULL });
    if (err) {
        HAPLogCharacteristic(
                &logObject, request->characteristic, request->service, request->accessory, "Parse Failure.");
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }

    // Session Identifier. (Required)
    if (!sessionIdentifierTLV.value.bytes) {
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Session Identifier missing.");
        return kHAPError_InvalidData;
    }
    if (sessionIdentifierTLV.value.numBytes > sizeof(uint8_t)) {
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Session Identifier has invalid length (%lu).",
                (unsigned long) sessionIdentifierTLV.value.numBytes);
        return kHAPError_InvalidData;
    }
    *sessionIdentifier = HAPReadUIntMax8(sessionIdentifierTLV.value.bytes, sessionIdentifierTLV.value.numBytes);

    // Payload.
    if (!payloadTLV.value.bytes) {
        HAPAssert(!payloadTLV.value.numBytes);
    }
    *payload = payloadTLV.value.bytes;
    *payloadSize = payloadTLV.value.numBytes;

    HAPLogCharacteristic(
            &logObject,
            request->characteristic,
            request->service,
            request->accessory,
            "DataStream HAP Payload: %zu bytes",
            payloadTLV.value.numBytes);

    // Force Close.
    if (forceCloseTLV.value.bytes) {
        uint8_t* bytes = (uint8_t*) forceCloseTLV.value.bytes;
        if (forceCloseTLV.value.numBytes != 1) {
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "Force Close has invalid length (%lu).",
                    (unsigned long) forceCloseTLV.value.numBytes);
            return kHAPError_InvalidData;
        } else if ((bytes[0] != 0) && (bytes[0] != 1)) {
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "Force Close has unexpected bool value (%u).",
                    bytes[0]);
            return kHAPError_InvalidData;
        } else {
            *forceClose = (bytes[0] != 0);
        }
    } else {
        *forceClose = false;
    }

    return kHAPError_None;
}

/**
 * Serialize the parameters for the "HAP Transport" write response.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If out of resources to process request.
 */
HAP_RESULT_USE_CHECK
static HAPError SerializeDataStreamHAPTransportWriteResponse(
        const uint8_t* _Nonnull payload,
        size_t payloadSize,
        bool requestToSend,
        bool sendEmptyResponse,
        HAPTLVWriter* responseWriter) {
    HAPPrecondition(responseWriter);

    HAPError err;

    // Payload.
    if ((!sendEmptyResponse) && (payloadSize > 0)) {
        err = HAPTLVWriterAppend(
                responseWriter,
                &(const HAPTLV) { .type = kHAPCharacteristicValue_DataStreamHAPTransport_Response_Payload,
                                  .value = { .bytes = payload, .numBytes = payloadSize } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
    }

    // Request to Send.
    if (!sendEmptyResponse) {
        uint8_t requestToSendBytes[] = { requestToSend };
        err = HAPTLVWriterAppend(
                responseWriter,
                &(const HAPTLV) {
                        .type = kHAPCharacteristicValue_DataStreamHAPTransport_Response_AccessoryRequestToSend,
                        .value = { .bytes = requestToSendBytes, .numBytes = sizeof requestToSendBytes } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
    }

    return kHAPError_None;
}

/**
 * Serialize the parameters for the "HAP Transport Interrupt" read request.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If out of resources to process request.
 */
HAP_RESULT_USE_CHECK
static HAPError SerializeDataStreamHAPTransportInterruptReadRequest(
        const void* requestToSendSessionIdentifiers,
        size_t requestToSendSessionIdentifiersSize,
        HAPDataStreamInterruptSequenceNumber interruptSequenceNumber,
        HAPTLVWriter* responseWriter) {
    HAPPrecondition(responseWriter);

    HAPError err;

    // Request To Send Session Identifiers.
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPCharacteristicValue_DataStreamHAPTransportInterrupt_RequestToSendIdentifiers,
                              .value = { .bytes = requestToSendSessionIdentifiers,
                                         .numBytes = requestToSendSessionIdentifiersSize } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // Sequence Number.
    uint8_t interruptSequenceNumberBytes[] = { interruptSequenceNumber };
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPCharacteristicValue_DataStreamHAPTransportInterrupt_SequenceNumber,
                              .value = { .bytes = &interruptSequenceNumberBytes,
                                         .numBytes = sizeof interruptSequenceNumberBytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleDataStreamTransportManagementDataStreamHAPTransportRead(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_DataStreamHAPTransport));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_DataStreamTransportManagement));
    HAPPrecondition(responseWriter);

    HAPError err;

    // State errors can only happen here if a regular write -> read pair has been used to access this characteristic
    // instead of an atomic write response transaction.

    // Get a buffer that the read-payload can be built into.
    void* payload;
    size_t payloadSize;
    HAPTLVWriterGetScratchBytesForTLVValue(responseWriter, &payload, &payloadSize);

#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    // Reduce payload size to kHAPDataStreamHAPTransport_ThreadMaxBufferSizeBytes if using thread transport.
    if ((request->session->transportType == kHAPTransportType_Thread) &&
        (payloadSize > kHAPDataStreamHAPTransport_ThreadMaxBufferSizeBytes)) {
        payloadSize = kHAPDataStreamHAPTransport_ThreadMaxBufferSizeBytes;
    }
#endif

    // Do not completely fill the buffer with data since additional TLV items will be added to the responseWriter.
    // Additional items may include TLV items such as "Force Close", "Accessory Request to Send More Data",
    // "Controller Clear to Send More Data", BLE PDU related overhead, etc.
    HAPAssert(payloadSize > kHAPDataStreamHAPTransport_ReadOverheadBytes);
    payloadSize -= kHAPDataStreamHAPTransport_ReadOverheadBytes;

    bool requestToSend;
    bool sendEmptyResponse;
    err = HAPDataStreamHAPTransportHandleRead(
            server, request->session, payload, payloadSize, &payloadSize, &requestToSend, &sendEmptyResponse);
    if (err) {
        return err;
    }

    return SerializeDataStreamHAPTransportWriteResponse(
            payload, payloadSize, requestToSend, sendEmptyResponse, responseWriter);
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleDataStreamTransportManagementDataStreamHAPTransportWrite(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicWriteRequest* request,
        HAPTLVReader* requestReader,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->session);
    HAPPrecondition(HAPSessionIsSecured(request->session) && !HAPSessionIsTransient(request->session));
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_DataStreamHAPTransport));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_DataStreamTransportManagement));
    HAPPrecondition(requestReader);

    HAPError err;

    HAPDataStreamHAPSessionIdentifier sessionIdentifier;
    const uint8_t* payload;
    size_t payloadSize;
    bool forceClose;
    err = ParseDataStreamHAPTransportWriteRequest(
            request, requestReader, &sessionIdentifier, &payload, &payloadSize, &forceClose);
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        // No log (the function logs on every path already)
        return err;
    }

    return HAPDataStreamHAPTransportHandleWrite(
            server, request->session, sessionIdentifier, payload, payloadSize, forceClose);
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleDataStreamTransportManagementDataStreamHAPTransportInterruptRead(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_DataStreamHAPTransportInterrupt));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_DataStreamTransportManagement));
    HAPPrecondition(responseWriter);

    HAPError err;

    // Get a place to write the RTS identifiers into.
    void* requestToSendSessionIdentifiers;
    size_t requestToSendSessionIdentifierSize;
    HAPTLVWriterGetScratchBytesForTLVValue(
            responseWriter, &requestToSendSessionIdentifiers, &requestToSendSessionIdentifierSize);

    HAPDataStreamInterruptSequenceNumber interruptSequenceNumber;

    err = HAPDataStreamHAPTransportGetInterruptState(
            server,
            requestToSendSessionIdentifiers,
            requestToSendSessionIdentifierSize,
            &requestToSendSessionIdentifierSize,
            &interruptSequenceNumber);
    if (err) {
        return err;
    }

    return SerializeDataStreamHAPTransportInterruptReadRequest(
            requestToSendSessionIdentifiers,
            requestToSendSessionIdentifierSize,
            interruptSequenceNumber,
            responseWriter);
}

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif
