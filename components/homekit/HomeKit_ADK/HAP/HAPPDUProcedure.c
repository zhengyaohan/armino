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

#include "HAPAccessory+Info.h"
#include "HAPAccessoryServer+Internal.h"
#include "HAPBLECharacteristic.h"
#include "HAPCharacteristic.h"
#include "HAPLog+Attributes.h"
#include "HAPLogSubsystem.h"
#include "HAPMFiTokenAuth.h"
#include "HAPPDU+AccessorySignature.h"
#include "HAPPDU+Characteristic.h"
#include "HAPPDU+CharacteristicConfiguration.h"
#include "HAPPDU+CharacteristicSignature.h"
#include "HAPPDU+CharacteristicValue.h"
#include "HAPPDU+NotificationConfiguration.h"
#include "HAPPDU+ProtocolConfiguration.h"
#include "HAPPDU+Service.h"
#include "HAPPDU+ServiceSignature.h"
#include "HAPPDU.h"
#include "HAPTLV+Internal.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "Procedure" };

// Some macros are defined in a separate file because `#pragma GCC system_header` is only available in included files.
#include "HAPPDUProcedure+Macros.h"

HAP_RESULT_USE_CHECK
static HAPError DeriveCharacteristicFromAddressedIID(
        HAPAccessoryServer* server,
        uint16_t iid,
        const HAPCharacteristic* _Nonnull* _Nonnull characteristic,
        const HAPService* _Nonnull* _Nonnull service,
        const HAPAccessory* _Nonnull* _Nonnull accessory) {
    HAPPrecondition(server);
    HAPPrecondition(characteristic);
    HAPPrecondition(service);
    HAPPrecondition(accessory);

    // The request IID can address various attributes and is mapped as follows:
    // - Characteristic instance ID => Request targets the addressed characteristic.
    // - Service instance ID => Request targets the addressed service's Service Signature characteristic.
    // - 0 => Request targets the HAP Protocol Information service's Service Signature characteristic.
    //
    // For requests addressing the service instance ID:
    // See HomeKit Accessory Protocol Specification R17
    // Section 7.3.4.12 HAP-Service-Signature-Read-Request
    // See HomeKit Accessory Protocol Specification R17
    // Section 7.3.4.16 HAP-Protocol-Configuration-Request
    //
    // For requests addressing instance ID 0:
    // See HomeKit Accessory Protocol Specification R17
    // Section 5.17 Software Authentication Procedure

    const HAPAccessory* a = HAPNonnull(server->primaryAccessory);
    if (a->services) {
        for (size_t i = 0; a->services[i]; i++) {
            const HAPService* s = a->services[i];
            if (s->characteristics) {
                for (size_t j = 0; s->characteristics[j]; j++) {
                    const HAPBaseCharacteristic* c = s->characteristics[j];
                    bool isMatch = false;
                    if (iid == c->iid) {
                        isMatch = true;
                    } else {
                        if (HAPPDUAreServiceProceduresSupportedOnCharacteristic(c)) {
                            if (iid == s->iid) {
                                isMatch = true;
                            } else {
                                if (HAPPDUAreAccessoryProceduresSupportedOnService(s)) {
                                    if (!iid) {
                                        isMatch = true;
                                    }
                                }
                            }
                        }
                    }
                    if (isMatch) {
                        *characteristic = c;
                        *service = s;
                        *accessory = a;
                        return kHAPError_None;
                    }
                }
            }
        }
    }
    return kHAPError_InvalidData;
}

HAP_RESULT_USE_CHECK
static HAPError HandleRequest( // NOLINT(readability-function-size)
        HAPAccessoryServer* server,
        HAPSession* session,
        const HAPCharacteristic* _Nullable characteristic_,
        const HAPService* _Nullable service,
        const HAPAccessory* _Nullable accessory,
        void* requestBytes_,
        size_t numRequestBytes,
        void* responseBytes_,
        size_t maxResponseBytes,
        size_t* numResponseBytes,
        size_t* pduBytesConsumed,
        bool* requestSuccessful) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition((characteristic_ == NULL) == (service == NULL) && (service == NULL) == (accessory == NULL));
    const HAPBaseCharacteristic* _Nullable characteristic = characteristic_;
    HAPPrecondition(requestBytes_);
    uint8_t* requestBytes = requestBytes_;
    HAPPrecondition(responseBytes_);
    uint8_t* responseBytes = responseBytes_;
    HAPPrecondition(numResponseBytes);

    HAPError err;

    // Parse PDU.
    uint8_t opcode_;
    uint8_t tid;
    uint16_t iid;
    void* _Nullable requestBodyBytes;
    uint16_t numRequestBodyBytes;
    uint16_t numHeaderBytes;
    *requestSuccessful = false;

    err = HAPPDUParseRequestWithoutFragmentation(
            requestBytes,
            numRequestBytes,
            &opcode_,
            &tid,
            &iid,
            &requestBodyBytes,
            &numRequestBodyBytes,
            &numHeaderBytes);
    if (err) {
        HAPAssert(err == kHAPError_InvalidData || err == kHAPError_InvalidState);
        return err;
    }

    *pduBytesConsumed = numHeaderBytes + numRequestBodyBytes;

    // Response variables.
    HAPPDUStatus status;
    void* _Nullable responseBodyBytes = NULL;
    size_t numResponseBodyBytes = 0;

    // Local variables. These have to be defined here to avoid ArmCC compiler warnings
    // when a variable initialization is jumped over with a goto statement (even when the variable is unused).
    bool sessionIsSecured = HAPSessionIsSecured(session);
    bool sessionIsTransient = HAPSessionIsTransient(session);
    bool controllerIsAdmin = HAPSessionControllerIsAdmin(session);
    bool isTimedWritePending = session->procedure.timedWrite.iid != 0;
    HAPPDUOpcode opcode = (HAPPDUOpcode) opcode_;
    uint64_t expectedIID = 0;
    bool returnResponse = true;

#define SEND_ERROR_AND_RETURN(rejectStatus, ...) \
    do { \
        LOG_OPCODE(__VA_ARGS__); \
        status = kHAPPDUStatus_##rejectStatus; \
        goto SendResponse; \
    } while (0)

    // If this request was received without being locked to a specific characteristic,
    // look up the addressed HAP attribute from the given instance ID.
    if (!characteristic_) {
        err = DeriveCharacteristicFromAddressedIID(server, iid, &characteristic_, &service, &accessory);
        if (err) {
            // Note that for HAP-Service-Signature-Read-Response an exception is defined to cope with
            // certain controller versions sending this request with invalid instance IDs (eg., 0).
            // In this case, this lookup would fail and the request rejected with an error
            // instead of responding with the special response as mandated by the specification.
            //
            // However, this behaviour only occurs on certain BLE controllers, but for BLE all requests
            // are locked to a specific characteristics (i.e, this code path cannot be reached from BLE).
            // See HomeKit Accessory Protocol Specification R17
            // Section 7.3.4.13 HAP-Service-Signature-Read-Response
            HAPAssert(session->transportType != kHAPTransportType_BLE);

            HAPAssert(err == kHAPError_InvalidData);
            SEND_ERROR_AND_RETURN(InvalidInstanceID, "No matching characteristic for request IID.");
        }
        characteristic = characteristic_;
    }
    HAPAssert(characteristic_);
    HAPAssert(characteristic == characteristic_);

    // Validate opcode.
    // If an accessory receives a HAP PDU with an opcode that it does not support it shall reject the PDU and
    // respond with a status code Unsupported PDU in its HAP response.
    // See HomeKit Accessory Protocol Specification R17
    // Section 7.3.3.2 HAP Request Format
    if (!HAPPDUOpcodeIsValid(opcode_)) {
        SEND_ERROR_AND_RETURN(UnsupportedPDU, "Opcode unknown.");
    }
    if (!HAPPDUOpcodeIsSupportedForTransport(opcode, session->transportType)) {
        SEND_ERROR_AND_RETURN(UnsupportedPDU, "Opcode not supported for transport %d.", session->transportType);
    }
    if (!HAPPDUOpcodeIsSupportedForCharacteristic(
                opcode, HAPNonnullVoid(characteristic_), HAPNonnull(service), HAPNonnull(accessory))) {
        SEND_ERROR_AND_RETURN(UnsupportedPDU, "Opcode not supported on this characteristic.");
    }

    // Validate received instance ID against instance ID of addressed characteristic.
    expectedIID = HAPPDUGetExpectedIIDForOperation(
            opcode,
            HAPNonnullVoid(characteristic_),
            HAPNonnull(service),
            HAPNonnull(accessory),
            session->transportType);
    if (iid != expectedIID) {
        if (opcode == kHAPPDUOpcode_ServiceSignatureRead) {
            // If the accessory receives an invalid (eg., 0) service instance ID in the
            // HAP-Service-Signature-Read-Request, it must respond with a valid
            // HAP-Service-Signature-Read-Response with Svc Properties set to 0 and
            // Linked Svc (if applicable) set to 0 length.
            // See HomeKit Accessory Protocol Specification R17
            // Section 7.3.4.13 HAP-Service-Signature-Read-Response
            LOG_OPCODE("Unexpected Request IID.");
        } else {
            SEND_ERROR_AND_RETURN(InvalidInstanceID, "Unexpected Request IID.");
        }
    }

    // When a timed write is pending, only allow execute write requests. Otherwise, don't allow them.
    if (opcode == kHAPPDUOpcode_CharacteristicExecuteWrite) {
        if (!isTimedWritePending) {
            SEND_ERROR_AND_RETURN(InvalidRequest, "No timed write pending.");
        }
    } else {
        // 10. Accessory must support only one HAP procedure on a characteristic at any point in time.
        // See HomeKit Accessory Protocol Specification R17
        // Section 7.5 Testing Bluetooth LE Accessories
        if (isTimedWritePending && opcode != kHAPPDUOpcode_CharacteristicTimedWrite) {
            // Per 7.3.5.4 HAP Characteristic Timed Write Procedure, the session is dropped.
            HAPSessionRelease(server, session);
            return kHAPError_NotAuthorized;
        }
    }

    // Check session security requirements.
    if (!sessionIsSecured && HAPPDUOpcodeRequiresSessionSecurity(opcode)) {
        SEND_ERROR_AND_RETURN(InsufficientAuthentication, "Only secure access is permitted.");
    }
    if (sessionIsTransient && !HAPPDUOpcodeIsSupportedOnTransientSessions(opcode)) {
        SEND_ERROR_AND_RETURN(InsufficientAuthentication, "Opcode not supported on transient sessions.");
    }

    // Handle request.
    switch (opcode) {
#define READ_REQUEST_BODY(requestBodyReader) \
    do { \
        HAPTLVReaderCreate(requestBodyReader, requestBodyBytes, numRequestBodyBytes); \
    } while (0)

#define START_RESPONSE_BODY_OR_REJECT_AND_RETURN(responseBodyWriter) \
    do { \
        if (maxResponseBytes < 5) { \
            SEND_ERROR_AND_RETURN( \
                    InsufficientResources, "Not enough memory (response buffer - %zu bytes).", maxResponseBytes); \
        } \
        HAPTLVWriterCreate(responseBodyWriter, &responseBytes[5], HAPMin(maxResponseBytes - 5, UINT16_MAX)); \
    } while (0)
#define FINALIZE_RESPONSE_BODY(responseBodyWriter) \
    do { \
        HAPTLVWriterGetBuffer(responseBodyWriter, &responseBodyBytes, &numResponseBodyBytes); \
    } while (0)
        case kHAPPDUOpcode_AccessorySignatureRead: {
            // Get response.
            HAPTLVWriter responseBodyWriter;
            START_RESPONSE_BODY_OR_REJECT_AND_RETURN(&responseBodyWriter);
            {
                err = HAPPDUGetAccessorySignatureReadResponse(HAPNonnull(server), &responseBodyWriter);
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    SEND_ERROR_AND_RETURN(InvalidRequest, "Failed to get response: %u.", err);
                }
            }
            FINALIZE_RESPONSE_BODY(&responseBodyWriter);
            status = kHAPPDUStatus_Success;
        }
            goto SendResponse;
        case kHAPPDUOpcode_ServiceSignatureRead: {
            // See HomeKit Accessory Protocol Specification R17
            // Section 7.4.4.5.4 Service Signature Characteristic

            // If the accessory receives an invalid (eg., 0) service instance ID in the
            // HAP-Service-Signature-Read-Request, it must respond with a valid HAP-Service-Signature-Read-Response
            // with Svc Properties set to 0 and Linked Svc (if applicable) set to 0 length.
            // See HomeKit Accessory Protocol Specification R17
            // Section 7.3.4.13 HAP-Service-Signature-Read-Response
            bool iidIsValid = iid == expectedIID;

            // Get response.
            HAPTLVWriter responseBodyWriter;
            START_RESPONSE_BODY_OR_REJECT_AND_RETURN(&responseBodyWriter);
            {
                err = HAPPDUGetServiceSignatureReadResponse(iidIsValid ? service : NULL, &responseBodyWriter);
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    SEND_ERROR_AND_RETURN(InvalidRequest, "Failed to get response: %u.", err);
                }
            }
            FINALIZE_RESPONSE_BODY(&responseBodyWriter);
            status = kHAPPDUStatus_Success;
        }
            goto SendResponse;
        case kHAPPDUOpcode_CharacteristicSignatureRead: {
            // See HomeKit Accessory Protocol Specification R17
            // Section 7.3.5.1 HAP Characteristic Signature Read Procedure

            // Note: On BLE, the security session is dropped when `Pair Setup`, `Pair Verify` or `Pairing Features`
            // is accessed. This is necessary to support the case of re-establishing session security without having
            // to reconnect. This also leads to the restriction that on BLE those characteristics don't support
            // secure characteristic signature reads.
            //
            // For other transports this restriction does not make sense because establishment of a new session
            // is not triggered by accessing certain characteristics:
            // - HAP over IP: New session starts by establishing a new TCP stream.
            // - HAP over Thread: New session starts by sending M1 to the Pair Setup or Pair Verify endpoint.
            //
            // The characteristics `Pair Setup`, `Pair Verify` and `Pairing Features` of `Pairing Service`
            // do not support "Paired Read" and "Paired Write" and only support the
            // `HAP Characteristic Signature Read Procedure` without a secure session.
            // See HomeKit Accessory Protocol Specification R17
            // Section 7.3.5.1 HAP Characteristic Signature Read Procedure
            if (session->transportType == kHAPTransportType_BLE &&
                HAPBLECharacteristicDropsSecuritySession(HAPNonnullVoid(characteristic_))) {
                HAPAssert(!sessionIsSecured);
            }

            // Get response.
            HAPTLVWriter responseBodyWriter;
            START_RESPONSE_BODY_OR_REJECT_AND_RETURN(&responseBodyWriter);
            {
                err = HAPPDUGetCharacteristicSignatureReadResponse(
                        HAPNonnullVoid(characteristic_), HAPNonnull(service), &responseBodyWriter);
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    SEND_ERROR_AND_RETURN(InvalidRequest, "Failed to get response: %u.", err);
                }
            }
            FINALIZE_RESPONSE_BODY(&responseBodyWriter);
            status = kHAPPDUStatus_Success;
        }
            goto SendResponse;
        case kHAPPDUOpcode_CharacteristicConfiguration: {
            // See HomeKit Accessory Protocol Specification R17
            // Section 7.3.5.8 HAP Characteristic Configuration Procedure

            // Handle request.
            HAPTLVReader requestBodyReader;
            READ_REQUEST_BODY(&requestBodyReader);
            {
                err = HAPPDUHandleCharacteristicConfigurationRequest(
                        HAPNonnullVoid(characteristic_),
                        HAPNonnull(service),
                        HAPNonnull(accessory),
                        &requestBodyReader,
                        server->platform.keyValueStore);
                if (err) {
                    HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidData);
                    SEND_ERROR_AND_RETURN(InvalidRequest, "Failed to handle request: %u.", err);
                }
            }

            // Get response.
            HAPTLVWriter responseBodyWriter;
            START_RESPONSE_BODY_OR_REJECT_AND_RETURN(&responseBodyWriter);
            {
                err = HAPPDUGetCharacteristicConfigurationResponse(
                        HAPNonnullVoid(characteristic_),
                        HAPNonnull(service),
                        HAPNonnull(accessory),
                        &responseBodyWriter,
                        server->platform.keyValueStore);
                if (err) {
                    HAPAssert(err == kHAPError_Unknown || err == kHAPError_OutOfResources);
                    SEND_ERROR_AND_RETURN(InvalidRequest, "Failed to get response: %u.", err);
                }
            }
            FINALIZE_RESPONSE_BODY(&responseBodyWriter);
            status = kHAPPDUStatus_Success;
        }
            goto SendResponse;
        case kHAPPDUOpcode_ProtocolConfiguration: {
            // See HomeKit Accessory Protocol Specification R17
            // Section 7.3.5.9 HAP Protocol Configuration Procedure

            if (!service->properties.ble.supportsConfiguration) {
                SEND_ERROR_AND_RETURN(UnsupportedPDU, "Service does not support configuration.");
            }

            // Handle request.
            bool didRequestGetAll;
            HAPTLVReader requestBodyReader;
            READ_REQUEST_BODY(&requestBodyReader);
            {
                err = HAPPDUHandleProtocolConfigurationRequest(
                        server,
                        session,
                        HAPNonnull(service),
                        HAPNonnull(accessory),
                        &requestBodyReader,
                        &didRequestGetAll,
                        server->platform.keyValueStore);
                if (err) {
                    HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidData);
                    SEND_ERROR_AND_RETURN(InvalidRequest, "Failed to handle request: %u.", err);
                }
            }
            if (!didRequestGetAll) {
                status = kHAPPDUStatus_Success;
                goto SendResponse;
            }
            // Get response.
            HAPTLVWriter responseBodyWriter;
            START_RESPONSE_BODY_OR_REJECT_AND_RETURN(&responseBodyWriter);
            {
                err = HAPPDUGetProtocolConfigurationResponse(
                        server,
                        session,
                        HAPNonnull(service),
                        HAPNonnull(accessory),
                        &responseBodyWriter,
                        server->platform.keyValueStore);
                if (err) {
                    HAPAssert(err == kHAPError_Unknown || err == kHAPError_OutOfResources);
                    SEND_ERROR_AND_RETURN(InvalidRequest, "Failed to get response: %u.", err);
                }
            }
            FINALIZE_RESPONSE_BODY(&responseBodyWriter);
            status = kHAPPDUStatus_Success;
        }
            goto SendResponse;
        case kHAPPDUOpcode_Token: {
            // See HomeKit Accessory Protocol Specification R17
            // Section 5.17.1 HAP-Token-Request

            // Get response.
            HAPTLVWriter responseBodyWriter;
            START_RESPONSE_BODY_OR_REJECT_AND_RETURN(&responseBodyWriter);
            {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_MFI_TOKEN_AUTH)
                err = HAPMFiTokenAuthGetTokenResponse(server, session, HAPNonnull(accessory), &responseBodyWriter);
                if (err) {
                    HAPAssert(
                            err == kHAPError_Unknown || err == kHAPError_InvalidState ||
                            err == kHAPError_OutOfResources);
                    SEND_ERROR_AND_RETURN(InvalidRequest, "Failed to get response: %u.", err);
                }
#else
                SEND_ERROR_AND_RETURN(InvalidRequest, "MFi token auth unsupported");
#endif
            }
            FINALIZE_RESPONSE_BODY(&responseBodyWriter);
            status = kHAPPDUStatus_Success;
        }
            goto SendResponse;
        case kHAPPDUOpcode_TokenUpdate: {
            // See HomeKit Accessory Protocol Specification R17
            // Section 5.17.3 HAP-Token-Update-Request

            // Handle request.
            HAPTLVReader requestBodyReader;
            READ_REQUEST_BODY(&requestBodyReader);
            {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_MFI_TOKEN_AUTH)
                err = HAPMFiTokenAuthHandleTokenUpdateRequest(
                        server, session, HAPNonnull(accessory), &requestBodyReader);
                if (err) {
                    HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidData);
                    SEND_ERROR_AND_RETURN(InvalidRequest, "Failed to handle request: %u.", err);
                }
#else
                SEND_ERROR_AND_RETURN(InvalidRequest, "MFi token auth unsupported");
#endif
            }

            status = kHAPPDUStatus_Success;
        }
            goto SendResponse;
        case kHAPPDUOpcode_Info: {
            // See HomeKit Accessory Protocol Specification R17
            // Section 5.17.5 HAP-Info-Request

            // Get response.
            HAPTLVWriter responseBodyWriter;
            START_RESPONSE_BODY_OR_REJECT_AND_RETURN(&responseBodyWriter);
            {
                err = HAPAccessoryGetInfoResponse(server, session, HAPNonnull(accessory), &responseBodyWriter);
                if (err) {
                    HAPAssert(err == kHAPError_Unknown || err == kHAPError_OutOfResources);
                    SEND_ERROR_AND_RETURN(InvalidRequest, "Failed to get response: %u.", err);
                }
            }
            FINALIZE_RESPONSE_BODY(&responseBodyWriter);
            status = kHAPPDUStatus_Success;
        }
            goto SendResponse;
        case kHAPPDUOpcode_NotificationConfigurationRead: {
            // Get response.
            HAPTLVWriter responseBodyWriter;
            START_RESPONSE_BODY_OR_REJECT_AND_RETURN(&responseBodyWriter);
            {
                err = HAPNotificationGetConfigurationReadResponse(
                        server,
                        session,
                        HAPNonnullVoid(characteristic_),
                        HAPNonnull(service),
                        HAPNonnull(accessory),
                        &responseBodyWriter);
                if (err) {
                    HAPAssert(err == kHAPError_InvalidState || err == kHAPError_OutOfResources);
                    SEND_ERROR_AND_RETURN(InvalidRequest, "Failed to get response: %u.", err);
                }
            }
            FINALIZE_RESPONSE_BODY(&responseBodyWriter);
            status = kHAPPDUStatus_Success;
        }
            goto SendResponse;
        case kHAPPDUOpcode_NotificationRegister: {
            // Handle request.
            err = HAPNotificationHandleRegisterRequest(
                    server, session, HAPNonnullVoid(characteristic_), HAPNonnull(service), HAPNonnull(accessory));
            if (err) {
                HAPAssert(err == kHAPError_InvalidState);
                SEND_ERROR_AND_RETURN(InvalidRequest, "Failed to handle request: %u.", err);
            }

            status = kHAPPDUStatus_Success;
        }
            goto SendResponse;
        case kHAPPDUOpcode_NotificationDeregister: {
            // Handle request.
            err = HAPNotificationHandleDeregisterRequest(
                    server, session, HAPNonnullVoid(characteristic_), HAPNonnull(service), HAPNonnull(accessory));
            if (err) {
                HAPAssert(err == kHAPError_InvalidState);
                SEND_ERROR_AND_RETURN(InvalidRequest, "Failed to handle request: %u.", err);
            }

            status = kHAPPDUStatus_Success;
        }
            goto SendResponse;
        case kHAPPDUOpcode_CharacteristicTimedWrite: {
            // See HomeKit Accessory Protocol Specification R17
            // Section 7.3.5.4 HAP Characteristic Timed Write Procedure

            // Cache body to be restored on HAP-Characteristic-Execute-Write.
            HAPAssert(characteristic->iid <= UINT16_MAX);
            if (isTimedWritePending) {
                HAPThreadSessionStorageClearData(server, session, kHAPSessionStorage_DataBuffer_TimedWrite);
            }
            err = HAPThreadSessionStorageSetData(
                    server,
                    session,
                    kHAPSessionStorage_DataBuffer_TimedWrite,
                    HAPNonnullVoid(requestBodyBytes),
                    numRequestBodyBytes);
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                SEND_ERROR_AND_RETURN(InvalidRequest, "Failed to cache request: %u.", err);
            }

            // Parse and retrieve TTL
            HAPTLVReader requestBodyReader;
            READ_REQUEST_BODY(&requestBodyReader);
            uint8_t ttl = 0;
            err = HAPPDUParseWriteCharacteristicValueTTL(HAPNonnullVoid(characteristic_), &requestBodyReader, &ttl);
            if (err || ttl == 0) {
                HAPAssert(!err || err == kHAPError_InvalidData);
                HAPRawBufferZero(&session->procedure.timedWrite, sizeof session->procedure.timedWrite);
                HAPThreadSessionStorageClearData(server, session, kHAPSessionStorage_DataBuffer_TimedWrite);
                SEND_ERROR_AND_RETURN(InvalidRequest, "Invalid request: %u.", err);
            }

            session->procedure.timedWrite.startTime = HAPPlatformClockGetCurrent();
            session->procedure.timedWrite.iid = (uint16_t) characteristic->iid;
            session->procedure.timedWrite.ttl = ttl;

            status = kHAPPDUStatus_Success;
        }
            goto SendResponse;
        case kHAPPDUOpcode_CharacteristicExecuteWrite: {
            // See HomeKit Accessory Protocol Specification R17
            // Section 7.3.5.4 HAP Characteristic Timed Write Procedure

            uint16_t timedWriteIID = session->procedure.timedWrite.iid;
            if (timedWriteIID != characteristic->iid) {
                SEND_ERROR_AND_RETURN(
                        InvalidRequest,
                        "Timed write was started with different IID [%016llX].",
                        (unsigned long long) timedWriteIID);
            }

            // Restore cached request body from HAP-Characteristic-Timed-Write.
            // Although undocumented the pending Timed Write request may also include
            // the Return Response flag and additional authorization data.
            // This means that it can be processed like regular writes.
            size_t numBytes;
            HAPThreadSessionStorageGetData(
                    server, session, kHAPSessionStorage_DataBuffer_TimedWrite, &requestBodyBytes, &numBytes);
            HAPAssert(numBytes <= UINT16_MAX);
            numRequestBodyBytes = (uint16_t) numBytes;
            HAP_FALLTHROUGH;
        }
        case kHAPPDUOpcode_CharacteristicWrite: {
            // See HomeKit Accessory Protocol Specification R17
            // Section 7.3.5.2 HAP Characteristic Write Procedure

            bool supportsWrite = characteristic->properties.ble.writableWithoutSecurity;
            bool supportsSecureWrite = characteristic->properties.writable;
            if (HAPUUIDAreEqual(characteristic->characteristicType, &kHAPCharacteristicType_Identify)) {
                // Unpaired Identify must be allowed only if the accessory is unpaired,
                // i.e., it has no paired controllers.
                // See HomeKit Accessory Protocol Specification R17
                // Section 7.4.1.9 Unpaired Identify
                supportsWrite = !HAPAccessoryServerIsPaired(server);
            }

            if (!sessionIsSecured && !supportsWrite) {
                if (supportsSecureWrite) {
                    SEND_ERROR_AND_RETURN(InsufficientAuthentication, "Only secure writes are permitted.");
                } else {
                    SEND_ERROR_AND_RETURN(UnsupportedPDU, "Secure writes not supported on this characteristic.");
                }
            }
            if (sessionIsSecured && !supportsSecureWrite) {
                if (supportsWrite) {
                    SEND_ERROR_AND_RETURN(UnsupportedPDU, "Only non-secure writes are permitted.");
                } else {
                    SEND_ERROR_AND_RETURN(UnsupportedPDU, "Non-secure writes not supported on this characteristic.");
                }
            }

            if (!controllerIsAdmin && HAPCharacteristicWriteRequiresAdminPermissions(HAPNonnullVoid(characteristic_))) {
                SEND_ERROR_AND_RETURN(InvalidRequest, "Only writes from admin controllers are permitted.");
            }

            bool isTimedWrite = opcode == kHAPPDUOpcode_CharacteristicExecuteWrite;
            bool requiresTimedWrite = characteristic->properties.requiresTimedWrite;
            if (!isTimedWrite && requiresTimedWrite) {
                SEND_ERROR_AND_RETURN(InvalidRequest, "Only timed writes are permitted.");
            }

            // Handle request.
            HAPTime timedWriteStartTime = session->procedure.timedWrite.startTime;
            bool didExpire;
            HAPTLVReader requestBodyReader;
            READ_REQUEST_BODY(&requestBodyReader);
            {
                err = HAPPDUParseAndWriteCharacteristicValue(
                        server,
                        session,
                        HAPNonnullVoid(characteristic_),
                        HAPNonnull(service),
                        HAPNonnull(accessory),
                        &requestBodyReader,
                        isTimedWrite ? &timedWriteStartTime : NULL,
                        &didExpire,
                        &returnResponse);
                if (err) {
                    if (err == kHAPError_NotAuthorized) {
                        SEND_ERROR_AND_RETURN(InsufficientAuthorization, "Insufficient authorization for write.");
                    }
                    HAPAssert(
                            err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                            err == kHAPError_OutOfResources || err == kHAPError_Busy);
                    if (HAPUUIDAreEqual(
                                characteristic->characteristicType, &kHAPCharacteristicType_TransitionControl) &&
                        (err == kHAPError_OutOfResources)) {
                        SEND_ERROR_AND_RETURN(InsufficientResources, "Insufficient resources for write.");
                    } else {
                        SEND_ERROR_AND_RETURN(InvalidRequest, "Failed to handle write: %u.", err);
                    }
                }
            }
            if (didExpire) {
                HAPTime delta = HAPPlatformClockGetCurrent() - timedWriteStartTime;
                SEND_ERROR_AND_RETURN(
                        InvalidRequest,
                        "Timed write (started %llu.%03llus ago) expired.",
                        (unsigned long long) (delta / HAPSecond),
                        (unsigned long long) (delta % HAPSecond));
            }
            if (isTimedWrite) {
                HAPRawBufferZero(&session->procedure.timedWrite, sizeof session->procedure.timedWrite);
                HAPThreadSessionStorageClearData(server, session, kHAPSessionStorage_DataBuffer_TimedWrite);
            }

            bool supportsRead = characteristic->properties.ble.readableWithoutSecurity;
            bool supportsSecureRead = characteristic->properties.readable;

            if ((!returnResponse && !characteristic->properties.ip.supportsWriteResponse) ||
                (!sessionIsSecured && !supportsRead) || (sessionIsSecured && !supportsSecureRead)) {
                // While any characteristic that supports write must support write-with-response
                // some are write only (ie:  no read).  They support write-with-response by providing
                // an empty response
                status = kHAPPDUStatus_Success;
                goto SendResponse;
            }
            // See HomeKit Accessory Protocol Specification R17
            // Section 7.3.5.5 HAP Characteristic Write-with-Response Procedure.
            if (!returnResponse) {
                // The supportsWriteResponse characteristic property provides a guarantee to the application
                // that the characteristic's handleRead callback is always called after a successful handleWrite.
                // Whether the controller actually requested write response is hidden from the application.
                // Although write response is mainly used in the HAP over IP transport it makes sense
                // to follow the same behaviour when such a characteristic is accessed using other transports.
                HAPAssert(characteristic->properties.ip.supportsWriteResponse);
                HAPLog(&logObject, "Calling read handler implicitly (characteristic supports write response).");
            }
            HAP_FALLTHROUGH;
        }
        case kHAPPDUOpcode_CharacteristicRead: {
            // See HomeKit Accessory Protocol Specification R17
            // Section 7.3.5.3 HAP Characteristic Read Procedure
            bool supportsRead = characteristic->properties.ble.readableWithoutSecurity;
            bool supportsSecureRead = characteristic->properties.readable;

            if (!sessionIsSecured && !supportsRead) {
                if (supportsSecureRead) {
                    SEND_ERROR_AND_RETURN(InsufficientAuthentication, "Only secure reads are permitted.");
                } else {
                    SEND_ERROR_AND_RETURN(UnsupportedPDU, "Secure reads not supported on this characteristic.");
                }
            }
            if (sessionIsSecured && !supportsSecureRead) {
                if (supportsRead) {
                    SEND_ERROR_AND_RETURN(UnsupportedPDU, "Only non-secure reads are permitted.");
                } else {
                    SEND_ERROR_AND_RETURN(UnsupportedPDU, "Non-secure reads not supported on this characteristic.");
                }
            }

            if (!controllerIsAdmin && HAPCharacteristicReadRequiresAdminPermissions(HAPNonnullVoid(characteristic_))) {
                SEND_ERROR_AND_RETURN(InvalidRequest, "Only reads from admin controllers are permitted.");
            }
            // Get response.
            HAPTLVWriter responseBodyWriter;
            START_RESPONSE_BODY_OR_REJECT_AND_RETURN(&responseBodyWriter);
            {
                err = HAPPDUReadAndSerializeCharacteristicValue(
                        server,
                        session,
                        HAPNonnullVoid(characteristic_),
                        HAPNonnull(service),
                        HAPNonnull(accessory),
                        &responseBodyWriter);
                if (err) {
                    HAPAssert(
                            err == kHAPError_Unknown || err == kHAPError_InvalidState ||
                            err == kHAPError_OutOfResources || err == kHAPError_Busy);
                    if (err == kHAPError_OutOfResources) {
                        SEND_ERROR_AND_RETURN(InsufficientResources, "Failed to handle request, out of resources");
                    } else {
                        SEND_ERROR_AND_RETURN(InvalidRequest, "Failed to handle read: %u.", err);
                    }
                }
            }
            FINALIZE_RESPONSE_BODY(&responseBodyWriter);
            if (!returnResponse) {
                HAPLog(&logObject, "Discarding write response (HAP-Param-Return-Response not set).");
                numResponseBodyBytes = 0;
            }
            status = kHAPPDUStatus_Success;
            goto SendResponse;
        }
#undef FINALIZE_RESPONSE_BODY
#undef START_RESPONSE_BODY_OR_REJECT_AND_RETURN
    }
    HAPFatalError();

#undef SEND_ERROR_AND_RETURN

SendResponse:
    if (!responseBodyBytes) {
        *numResponseBytes = 5;
        numResponseBodyBytes = 0;
    } else {
        *numResponseBytes = 5 + numResponseBodyBytes;
        HAPAssert(responseBodyBytes == &responseBytes[5]);
    }
    if (maxResponseBytes < *numResponseBytes) {
        HAPLog(&logObject, "Insufficient procedure buffer capacity (%zu bytes).", maxResponseBytes);
        return kHAPError_OutOfResources;
    }
    responseBytes[0] = (uint8_t)(
            (uint8_t)(0U << 7U) |                                             // First Fragment (or no fragmentation).
            (uint8_t)(0U << 4U) |                                             // 16 bit IIDs (or IID = 0).
            (uint8_t)(0U << 3U) | (uint8_t)(0U << 2U) | (uint8_t)(1U << 1U) | // Response.
            (uint8_t)(0U << 0U));                                             // 1 Byte Control Field.
    responseBytes[1] = tid;
    responseBytes[2] = status;
    HAPAssert(numResponseBodyBytes <= UINT16_MAX);
    HAPWriteLittleUInt16(&responseBytes[3], (uint16_t) numResponseBodyBytes);
    if (status) {
        HAPRawBufferZero(&session->procedure.timedWrite, sizeof session->procedure.timedWrite);
        HAPThreadSessionStorageClearData(server, session, kHAPSessionStorage_DataBuffer_TimedWrite);
        *requestSuccessful = false;
    } else {
        *requestSuccessful = true;
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPDUProcedureHandleRequest(
        HAPAccessoryServer* server,
        HAPSession* session,
        void* requestBytes,
        size_t numRequestBytes,
        void* responseBytes,
        size_t maxResponseBytes,
        size_t* numResponseBytes,
        size_t* pduBytesConsumed,
        bool* requestSuccessful) {

    return HandleRequest(
            server,
            session,
            NULL,
            NULL,
            NULL,
            requestBytes,
            numRequestBytes,
            responseBytes,
            maxResponseBytes,
            numResponseBytes,
            pduBytesConsumed,
            requestSuccessful);
}

HAP_RESULT_USE_CHECK
HAPError HAPPDUProcedureHandleRequestForCharacteristic(
        HAPAccessoryServer* server,
        HAPSession* session,
        const HAPCharacteristic* characteristic,
        const HAPService* service,
        const HAPAccessory* accessory,
        void* requestBytes,
        size_t numRequestBytes,
        void* responseBytes,
        size_t maxResponseBytes,
        size_t* numResponseBytes,
        size_t* pduBytesConsumed,
        bool* requestSuccessful) {
    return HandleRequest(
            server,
            session,
            characteristic,
            service,
            accessory,
            requestBytes,
            numRequestBytes,
            responseBytes,
            maxResponseBytes,
            numResponseBytes,
            pduBytesConsumed,
            requestSuccessful);
}

#endif // HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
