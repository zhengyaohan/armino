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

#include "HAPPlatformFeatures.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_ACCESS_CODE)

#include "HAPAccessoryServer+Internal.h"
#include "HAPCharacteristicTypes+TLV.h"
#include "HAPCharacteristicTypes.h"
#include "HAPLogSubsystem.h"
#include "HAPServiceTypes.h"
#include "HAPTLV+Internal.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "RequestHandlers" };

/**
 * Maximum length of an access code
 */
#define kAccessCodeMaximumLength 8

//----------------------------------------------------------------------------------------------------------------------

/**
 * Write the response for Access Code Control Point. The caller will be responsible for setting all necessary values for
 * accessControlPoint.
 *
 * @param   responseWriter       TLV writer to produce output
 * @param   accessControlPoint   Access Code Control Point to write
 */
static HAPError WriteControlPointResponse(
        HAPTLVWriter* responseWriter,
        HAPCharacteristicValue_AccessCodeControlPoint* accessControlPoint) {
    HAPError err =
            HAPTLVWriterEncode(responseWriter, &kHAPCharacteristicTLVFormat_AccessCodeControlPoint, accessControlPoint);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                err == kHAPError_Busy);
        return err;
    }

    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPHandleAccessCodeControlPointRead(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_AccessCodeControlPoint));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_AccessCode));
    HAPPrecondition(request->accessory->aid == 1);
    HAPPrecondition(responseWriter);

    HAPError err;

    // Check if this read is called in response to a previous write from the same session
    if (server->accessCode.hapSession == request->session) {
        server->accessCode.hapSession = NULL;
        if (server->accessCode.numResponseBytes > 0) {
            HAPAssert(server->accessCode.responseStorage.bytes);

            // Copy over TLV constructed during the write
            err = HAPTLVWriterExtend(
                    responseWriter,
                    server->accessCode.responseStorage.bytes,
                    server->accessCode.numResponseBytes,
                    kHAPCharacteristicTLVType_AccessCodeControlPoint_Response);

            if (err) {
                HAPAssert(
                        err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                        err == kHAPError_Busy);
            }

            // Done with the write response. Invalidate the queued response
            server->accessCode.numResponseBytes = 0;

            return err;
        }
        // Empty response
        return kHAPError_None;
    }

    // No previous write response so read is not allowed
    return kHAPError_InvalidState;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Handles Access Code Control Point Request read operation
 *
 * @param      server               accessory server
 * @param      configRequest        received Access Code Control Point Request
 * @param[out] accessControlPoint   Access Code Control Point to build as the result of the operation.
 *
 * @result kHAPError_None if the response must be sent out, including a case where
 *                        error status must be sent. Other error values, otherwise.
 */
HAP_RESULT_USE_CHECK
static HAPError HandleControlPointRequestReadOp(
        HAPAccessoryServer* server,
        HAPCharacteristicValue_AccessCodeControlPointRequest* configRequest,
        HAPCharacteristicValue_AccessCodeControlPoint* accessControlPoint) {
    HAPPrecondition(server);
    HAPPrecondition(configRequest);
    HAPPrecondition(accessControlPoint);

    accessControlPoint->responseIsSet = true;
    accessControlPoint->response.statusIsSet = true;

    if (!configRequest->identifierIsSet) {
        HAPLog(&logObject, "Operation requires identifier");
        accessControlPoint->response.status =
                kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorInvalidRequest;
        return kHAPError_None;
    }

    HAPAssert(server->accessCode.handleOperation);
    HAPAccessCodeOperation operation = {
        .type = kHAPAccessCodeOperationType_Read,
        .identifier = configRequest->identifier,
    };
    HAPError err = server->accessCode.handleOperation(&operation, server->accessCode.operationCtx);
    if (err) {
        return err;
    }

    if (!operation.found) {
        accessControlPoint->response.status =
                kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorDoesNotExist;
        accessControlPoint->response.identifier = operation.identifier;
        accessControlPoint->response.identifierIsSet = true;
    } else {
        accessControlPoint->response.status = kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success;
        accessControlPoint->response.identifier = operation.identifier;
        accessControlPoint->response.identifierIsSet = true;
        accessControlPoint->response.accessCode = operation.accessCode;
        accessControlPoint->response.accessCodeIsSet = true;
        accessControlPoint->response.flags = operation.flags;
        accessControlPoint->response.flagsIsSet = true;
    }

    return kHAPError_None;
}

/**
 * Converts returned operation error flags into error status code
 */
HAP_RESULT_USE_CHECK
static HAPCharacteristicValue_AccessCodeControlPointResponse_Status
        ConvertOperationFlagsIntoStatus(const HAPAccessCodeOperation* _Nonnull op) {
    HAPPrecondition(op);

    if (op->outOfResources) {
        return kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ExceededMaximumAllowedAccessCodes;
    }
    if (op->duplicate) {
        return kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorDuplicate;
    }
    if (op->parameterTooShort) {
        return kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorSmallerThanMinimumLength;
    }
    if (op->parameterTooLong) {
        return kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorLargerThanMaximumLength;
    }
    if (op->invalidCharacterSet) {
        return kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorInvalidCharacter;
    }
    if (!op->found) {
        return kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorDoesNotExist;
    }

    return kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success;
}
/**
 * Handles Access Code Control Point Request add operation
 *
 * @param      server               accessory server
 * @param      configRequest        received Access Code Control Point Request
 * @param[out] accessControlPoint   Access Code Control Point to build as the result of the operation.
 *
 * @result kHAPError_None if the response must be sent out, including a case where
 *                        error status must be sent. Other error values, otherwise.
 */
HAP_RESULT_USE_CHECK
static HAPError HandleControlPointRequestAddOp(
        HAPAccessoryServer* server,
        HAPCharacteristicValue_AccessCodeControlPointRequest* configRequest,
        HAPCharacteristicValue_AccessCodeControlPoint* accessControlPoint) {
    HAPPrecondition(server);
    HAPPrecondition(configRequest);
    HAPPrecondition(accessControlPoint);

    accessControlPoint->responseIsSet = true;
    accessControlPoint->response.statusIsSet = true;

    if (!configRequest->accessCodeIsSet) {
        HAPLog(&logObject, "Operation requires access code");
        accessControlPoint->response.status =
                kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorInvalidRequest;
        return kHAPError_None;
    }

    HAPAssert(server->accessCode.handleOperation);
    HAPAccessCodeOperation operation = {
        .type = kHAPAccessCodeOperationType_QueueAdd,
        .accessCode = configRequest->accessCode,
    };
    HAPError err = server->accessCode.handleOperation(&operation, server->accessCode.operationCtx);
    if (err) {
        return err;
    }

    // This flag does not apply to add operations
    operation.found = true;
    accessControlPoint->response.status = ConvertOperationFlagsIntoStatus(&operation);
    if ((accessControlPoint->response.status == kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success) ||
        (accessControlPoint->response.status ==
         kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorDuplicate)) {
        accessControlPoint->response.identifier = operation.identifier;
        accessControlPoint->response.identifierIsSet = true;
        accessControlPoint->response.accessCode = operation.accessCode;
        accessControlPoint->response.accessCodeIsSet = true;
        accessControlPoint->response.flags = operation.flags;
        accessControlPoint->response.flagsIsSet = true;
    } else {
        accessControlPoint->response.accessCode = operation.accessCode;
        accessControlPoint->response.accessCodeIsSet = true;
    }

    return kHAPError_None;
}

/**
 * Handles Access Code Control Point Request update operation
 *
 * @param      server               accessory server
 * @param      configRequest        received Access Code Control Point Request
 * @param[out] accessControlPoint   Access Code Control Point to build as the result of the operation.
 *
 * @result kHAPError_None if the response must be sent out, including a case where
 *                        error status must be sent. Other error values, otherwise.
 */
HAP_RESULT_USE_CHECK
static HAPError HandleControlPointRequestUpdateOp(
        HAPAccessoryServer* server,
        HAPCharacteristicValue_AccessCodeControlPointRequest* configRequest,
        HAPCharacteristicValue_AccessCodeControlPoint* accessControlPoint) {
    HAPPrecondition(server);
    HAPPrecondition(configRequest);
    HAPPrecondition(accessControlPoint);

    accessControlPoint->responseIsSet = true;
    accessControlPoint->response.statusIsSet = true;

    if (!configRequest->identifierIsSet) {
        HAPLog(&logObject, "Operation requires identifier");
        accessControlPoint->response.status =
                kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorInvalidRequest;
        return kHAPError_None;
    }

    if (!configRequest->accessCodeIsSet) {
        HAPLog(&logObject, "Operation requires access code");
        accessControlPoint->response.status =
                kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorInvalidRequest;
        return kHAPError_None;
    }

    HAPAssert(server->accessCode.handleOperation);
    HAPAccessCodeOperation operation = { .type = kHAPAccessCodeOperationType_QueueUpdate,
                                         .identifier = configRequest->identifier,
                                         .accessCode = configRequest->accessCode };
    HAPError err = server->accessCode.handleOperation(&operation, server->accessCode.operationCtx);
    if (err) {
        return err;
    }

    accessControlPoint->response.status = ConvertOperationFlagsIntoStatus(&operation);
    if ((accessControlPoint->response.status == kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success) ||
        (accessControlPoint->response.status ==
         kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorDuplicate)) {
        accessControlPoint->response.identifier = operation.identifier;
        accessControlPoint->response.identifierIsSet = true;
        accessControlPoint->response.accessCode = operation.accessCode;
        accessControlPoint->response.accessCodeIsSet = true;
        accessControlPoint->response.flags = operation.flags;
        accessControlPoint->response.flagsIsSet = true;
    } else {
        accessControlPoint->response.identifier = operation.identifier;
        accessControlPoint->response.identifierIsSet = true;
        accessControlPoint->response.accessCode = operation.accessCode;
        accessControlPoint->response.accessCodeIsSet = true;
    }

    return kHAPError_None;
}

/**
 * Handles Access Code Control Point Request remove operation
 *
 * @param      server               accessory server
 * @param      configRequest        received Access Code Control Point Request
 * @param[out] accessControlPoint   Access Code Control Point to build as the result of the operation
 * @param[out] accessCode           Pre-allocated to store the removed access code for the response
 *
 * @result kHAPError_None if the response must be sent out, including a case where
 *                        error status must be sent. Other error values, otherwise.
 */
HAP_RESULT_USE_CHECK
static HAPError HandleControlPointRequestRemoveOp(
        HAPAccessoryServer* server,
        HAPCharacteristicValue_AccessCodeControlPointRequest* configRequest,
        HAPCharacteristicValue_AccessCodeControlPoint* accessControlPoint,
        char* accessCode) {
    HAPPrecondition(server);
    HAPPrecondition(configRequest);
    HAPPrecondition(accessControlPoint);
    HAPPrecondition(accessCode);

    accessControlPoint->responseIsSet = true;
    accessControlPoint->response.statusIsSet = true;

    if (!configRequest->identifierIsSet) {
        HAPLog(&logObject, "Operation requires identifier");
        accessControlPoint->response.status =
                kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorInvalidRequest;
        return kHAPError_None;
    }

    HAPAssert(server->accessCode.handleOperation);
    HAPAccessCodeOperation operation = { .type = kHAPAccessCodeOperationType_QueueRemove,
                                         .identifier = configRequest->identifier,
                                         .accessCode = accessCode };
    HAPError err = server->accessCode.handleOperation(&operation, server->accessCode.operationCtx);
    if (err) {
        return err;
    }

    accessControlPoint->response.status = ConvertOperationFlagsIntoStatus(&operation);
    if (accessControlPoint->response.status == kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success) {
        accessControlPoint->response.identifier = operation.identifier;
        accessControlPoint->response.identifierIsSet = true;
        accessControlPoint->response.accessCode = operation.accessCode;
        accessControlPoint->response.accessCodeIsSet = true;
        accessControlPoint->response.flags = operation.flags;
        accessControlPoint->response.flagsIsSet = true;
    } else {
        accessControlPoint->response.identifier = operation.identifier;
        accessControlPoint->response.identifierIsSet = true;
    }

    return kHAPError_None;
}

/**
 * Handles an Access Code Control Point Request.
 *
 * @param      server                  Accessory Server instance
 * @param      operation               requested operation
 * @param      requestTLV              decoded Access Code Control Point Request TLV
 * @param      tlv8_writer             TLV writer to produce output
 *
 * @return kHAPError_None if successful.<br>
 *         kHAPError_InvalidData if the request TLV has bad data.<br>
 *         Other unexpected errors.
 */
HAP_RESULT_USE_CHECK
static HAPError HandleControlPointRequest(
        HAPAccessoryServer* server,
        HAPCharacteristicValue_AccessCodeControlPoint_Operation_Type operation,
        const HAPTLV* requestTLV,
        HAPTLVWriter* tlv8_writer) {
    HAPPrecondition(server);
    HAPPrecondition(requestTLV);
    HAPPrecondition(tlv8_writer);

    // Decode received configuration request TLVs
    HAPTLVReader tlv8_reader;

    // Note that HAPTLVReaderDecode() modifies the original input buffer and hence
    // to allow the requestTLV buffer to be read again repeatedly, the input buffer is copied.
    uint8_t buffer[requestTLV->value.numBytes];
    HAPRawBufferCopyBytes(buffer, requestTLV->value.bytes, requestTLV->value.numBytes);

    HAPTLVReaderCreate(&tlv8_reader, buffer, requestTLV->value.numBytes);
    HAPCharacteristicValue_AccessCodeControlPointRequest configValue;
    HAPError err =
            HAPTLVReaderDecode(&tlv8_reader, &kHAPCharacteristicTLVFormat_AccessCodeControlPointRequest, &configValue);
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }

    // This needs to be available until the response is written
    char accessCode[kAccessCodeMaximumLength + 1];

    HAPCharacteristicValue_AccessCodeControlPoint accessControlPoint;
    HAPRawBufferZero(&accessControlPoint, sizeof accessControlPoint);
    accessControlPoint.response.statusIsSet = true;

    switch (operation) {
        case kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Read: {
            err = HandleControlPointRequestReadOp(server, &configValue, &accessControlPoint);
            break;
        }
        case kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Add: {
            err = HandleControlPointRequestAddOp(server, &configValue, &accessControlPoint);
            break;
        }
        case kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Update: {
            err = HandleControlPointRequestUpdateOp(server, &configValue, &accessControlPoint);
            break;
        }
        case kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Remove: {
            err = HandleControlPointRequestRemoveOp(server, &configValue, &accessControlPoint, accessCode);
            break;
        }
        case kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_List: {
            // Not reachable
            HAPAssertionFailure();
        }
        default: {
            HAPLog(&logObject, "Operation not yet implemented");
            return kHAPError_InvalidData;
        }
    }

    if (err) {
        // Handling of operation has failed in a way that response shouldn't even be sent.
        return err;
    }

    // Write the result of the operation response
    return WriteControlPointResponse(tlv8_writer, &accessControlPoint);
}

/**
 * Context for Access Code identifier enumeration callback to build list operation response
 */
typedef struct {
    /**
     * TLV Writer for response
     */
    HAPTLVWriter* writer;

    /**
     * Indicates whether a TLV separator should be inserted before inserting an Access Code Configuration Response TLV
     */
    bool separatorIsRequired;

    /**
     * Error which has occurred during enumeration
     */
    HAPError error;
} ListOperationResponseWriterContext;

/**
 * Callback function to pass to Access Code identifier enumeration operation to build list operation response
 *
 * @param identifier   identifier of a present access code
 * @param arg          context of the callback
 */
static void AddIdentifierToAccessCodeList(uint32_t identifier, void* arg) {
    HAPPrecondition(arg);

    ListOperationResponseWriterContext* context = arg;

    if (context->error != kHAPError_None) {
        // Error has occurred. No need to move on.
        return;
    }

    if (context->separatorIsRequired) {
        // Separator has to be inserted before identifier
        context->error = HAPTLVWriterAppend(
                context->writer,
                &(const HAPTLV) { .type = kHAPCharacteristicTLVType_Separator,
                                  .value = { .bytes = NULL, .numBytes = 0 } });
        if (context->error != kHAPError_None) {
            return;
        }
    }

    HAPCharacteristicValue_AccessCodeControlPoint accessControlPoint;
    HAPRawBufferZero(&accessControlPoint, sizeof accessControlPoint);
    accessControlPoint.responseIsSet = true;
    accessControlPoint.response.identifier = identifier;
    accessControlPoint.response.identifierIsSet = true;

    // Write the list operation response
    context->error = WriteControlPointResponse(context->writer, &accessControlPoint);

    // Upon next callback to add another identifier, a TLV separator has to be added
    context->separatorIsRequired = true;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleAccessCodeControlPointWrite(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicWriteRequest* request,
        HAPTLVReader* requestReader,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_AccessCodeControlPoint));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_AccessCode));
    HAPPrecondition(request->accessory->aid == 1);
    HAPPrecondition(requestReader);

    if (!server->accessCode.responseStorage.bytes) {
        // Storage was never allocated.
        // Perhaps access code service invocation should fail instead.
        // kHAPIPAccessoryServerStatusCode_OutOfResources thru ConvertCharacteristicReadErrorToStatusCode();
        return kHAPError_OutOfResources;
    }

    server->accessCode.numResponseBytes = 0;
    HAPError err;
    HAPTLVWriter tlv8_writer;
    HAPTLVWriterCreate(
            &tlv8_writer, server->accessCode.responseStorage.bytes, server->accessCode.responseStorage.maxBytes);

    HAPCharacteristicValue_AccessCodeControlPoint_Operation_Type operation =
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_List;
    bool operationIsSet = false;
    bool bulkOperationInProgress = false;
    size_t bulkOperationCount = 0;

    // Handle each Access Code Control Point Request
    for (;;) {
        HAPTLV tlv;
        bool valid;
        err = HAPTLVReaderGetNext(requestReader, &valid, &tlv);
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            goto exit_no_response;
        }

        if (!valid) {
            break;
        }

        switch (tlv.type) {
            case kHAPCharacteristicTLVType_AccessCodeControlPoint_OperationType: {
                if (operationIsSet) {
                    HAPLog(&logObject, "[%02x] Duplicate TLV.", tlv.type);
                    err = kHAPError_InvalidData;
                    goto exit_no_response;
                }
                operationIsSet = true;
                operation = HAPReadUInt8(tlv.value.bytes);

                if (operation == kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Read ||
                    operation == kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Add ||
                    operation == kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Remove ||
                    operation == kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Update) {
                    // Starts bulk operation
                    HAPAssert(server->accessCode.handleOperation);
                    err = server->accessCode.handleOperation(
                            &((HAPAccessCodeOperation) { .type = kHAPAccessCodeOperationType_BulkOperationStart }),
                            server->accessCode.operationCtx);
                    if (err == kHAPError_Busy) {
                        // Only one bulk operation allowed at a time
                        HAPCharacteristicValue_AccessCodeControlPoint accessControlPoint;
                        HAPRawBufferZero(&accessControlPoint, sizeof accessControlPoint);
                        accessControlPoint.operationType = operation;
                        accessControlPoint.operationTypeIsSet = true;
                        accessControlPoint.responseIsSet = true;
                        accessControlPoint.response.status =
                                kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorTooManyBulkOperations;
                        accessControlPoint.response.statusIsSet = true;
                        err = WriteControlPointResponse(&tlv8_writer, &accessControlPoint);
                        if (err) {
                            goto exit_no_response;
                        }
                        return kHAPError_None;
                    } else if (err) {
                        goto exit_no_response;
                    }
                    bulkOperationInProgress = true;
                }

                // Write the operation type for any response appended
                HAPCharacteristicValue_AccessCodeControlPoint accessControlPoint;
                HAPRawBufferZero(&accessControlPoint, sizeof accessControlPoint);
                accessControlPoint.operationType = operation;
                accessControlPoint.operationTypeIsSet = true;
                err = WriteControlPointResponse(&tlv8_writer, &accessControlPoint);
                if (err) {
                    goto exit_no_response;
                }
                break;
            }
            case kHAPCharacteristicTLVType_AccessCodeControlPoint_Request: {
                if (!operationIsSet) {
                    HAPLog(&logObject, "Request TLV prior to Operation Type TLV");
                    err = kHAPError_InvalidData;
                    goto exit_no_response;
                }
                if (operation == kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_List) {
                    HAPLog(&logObject, "Unexpected Request TLV for List operation");
                    err = kHAPError_InvalidData;
                    goto exit_no_response;
                }

                // Insert TLV delimiter if necessary
                if (bulkOperationCount > 0) {
                    err = HAPTLVWriterAppend(
                            &tlv8_writer,
                            &(const HAPTLV) { .type = kHAPCharacteristicTLVType_Separator,
                                              .value = { .bytes = NULL, .numBytes = 0 } });
                    if (err) {
                        HAPAssert(
                                err == kHAPError_Unknown || err == kHAPError_InvalidState ||
                                err == kHAPError_OutOfResources || err == kHAPError_Busy);
                        goto exit_no_response;
                    }
                }

                // Handle the request and perform the operation
                err = HandleControlPointRequest(server, operation, &tlv, &tlv8_writer);
                if (err) {
                    HAPAssert(err == kHAPError_InvalidData || err == kHAPError_OutOfResources);

                    // If the number of requests is more than max allowed, commit what has been processed so far
                    if (err == kHAPError_OutOfResources) {
                        goto bulk_requests_processed;
                    }
                    goto exit_no_response;
                }

                if (bulkOperationInProgress) {
                    bulkOperationCount++;
                }
                break;
            }
            case kHAPCharacteristicTLVType_Separator:
                break;
        }
    }

bulk_requests_processed:
    if (operationIsSet) {
        if (operation == kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_List) {
            // Generate list
            ListOperationResponseWriterContext callbackContext = { .writer = &tlv8_writer,
                                                                   .separatorIsRequired = false,
                                                                   .error = kHAPError_None };

            HAPAssert(server->accessCode.handleOperation);
            err = server->accessCode.handleOperation(
                    &((HAPAccessCodeOperation) { .type = kHAPAccessCodeOperationType_EnumerateIdentifiers,
                                                 .enumerateCallback = AddIdentifierToAccessCodeList,
                                                 .enumerateContext = &callbackContext }),
                    server->accessCode.operationCtx);
            if (err || callbackContext.error) {
                return err;
            }

        } else if (bulkOperationInProgress) {
            // All done processing bulk operation requests so commit the changes
            HAPAssert(server->accessCode.handleOperation);
            err = server->accessCode.handleOperation(
                    &((HAPAccessCodeOperation) { .type = kHAPAccessCodeOperationType_BulkOperationCommit }),
                    server->accessCode.operationCtx);
            if (err) {
                return err;
            }
        }

        // Update storage session info
        server->accessCode.hapSession = request->session;
        void* bytes;
        HAPTLVWriterGetBuffer(&tlv8_writer, &bytes, &server->accessCode.numResponseBytes);
    }

    return kHAPError_None;

exit_no_response:
    // Some error occurred such that handler should exit without a response
    if (bulkOperationInProgress) {
        // Abort ongoing bulk operation
        HAPAssert(server->accessCode.handleOperation);
        err = server->accessCode.handleOperation(
                &((HAPAccessCodeOperation) { .type = kHAPAccessCodeOperationType_BulkOperationAbort }),
                server->accessCode.operationCtx);
        HAPAssert(!err);
        server->accessCode.hapSession = NULL;
    }
    return err;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPBuildAccessCodeSupportedConfigurationResponse(
        HAPTLVWriter* responseWriter,
        uint8_t characterSet,
        uint8_t minimumLength,
        uint8_t maximumLength,
        uint16_t maximumAccessCodes) {
    HAPPrecondition(responseWriter);

    HAPError err;

    HAPCharacteristicValue_AccessCodeSupportedConfiguration value;
    HAPRawBufferZero(&value, sizeof value);

    // The following are specific to this application helper implementation.
    value.characterSet = characterSet;
    value.minimumLength = minimumLength;
    value.maximumLength = maximumLength;
    value.maximumAccessCodes = maximumAccessCodes;

    err = HAPTLVWriterEncode(responseWriter, &kHAPCharacteristicTLVFormat_AccessCodeSupportedConfiguration, &value);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                err == kHAPError_Busy);
        return err;
    }

    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

void HAPHandleAccessCodeSessionInvalidate(HAPAccessoryServer* server, HAPSession* session) {
    HAPPrecondition(server);
    HAPPrecondition(session);

    if (server->accessCode.hapSession == session) {
        // Clear the write response in case the session is closed in between.
        // This only prevents ill behaving BLE conroller from reading off the last controller
        // write-response.
        server->accessCode.numResponseBytes = 0;
    }
}

#endif
