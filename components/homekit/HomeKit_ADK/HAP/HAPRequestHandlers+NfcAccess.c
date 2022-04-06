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

#include "HAP+API.h"

#include "HAPPlatformFeatures.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_NFC_ACCESS)

#include "HAPAccessoryServer+Internal.h"
#include "HAPCharacteristicTypes+TLV.h"
#include "HAPCharacteristicTypes.h"
#include "HAPLogSubsystem.h"
#include "HAPServiceTypes.h"
#include "HAPTLV+Internal.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "RequestHandlers" };

//----------------------------------------------------------------------------------------------------------------------

/**
 * Convert an internal status code to corresponding NFC Access Response Status Code
 *
 * @param    statusCode   Internal status code to convert
 *
 * @return   Corresponding NFC Access Response Status Code
 */
HAP_RESULT_USE_CHECK
static HAPCharacteristicValue_NfcAccessResponseStatusCode ConvertStatusCode(NfcAccessStatusCode statusCode) {
    switch (statusCode) {
        case NFC_ACCESS_STATUS_CODE_SUCCESS:
            return kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success;
        case NFC_ACCESS_STATUS_CODE_OUT_OF_RESOURCES:
            return kHAPCharacteristicValue_NfcAccessResponseStatusCode_ErrorOutOfResources;
        case NFC_ACCESS_STATUS_CODE_DUPLICATE:
            return kHAPCharacteristicValue_NfcAccessResponseStatusCode_ErrorDuplicate;
        case NFC_ACCESS_STATUS_CODE_DOES_NOT_EXIST:
            return kHAPCharacteristicValue_NfcAccessResponseStatusCode_ErrorDoesNotExist;
        case NFC_ACCESS_STATUS_CODE_NOT_SUPPORTED:
            return kHAPCharacteristicValue_NfcAccessResponseStatusCode_ErrorNotSupported;
    }

    return kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success;
}

/**
 * Handles NFC Access Issuer Key Request list operation
 *
 * @param      server           Accessory server instance
 * @param      responseWriter   TLV writer to produce output
 * @param      keyRequest       NFC Access Issuer Key Request
 * @param[out] keyResponse      NFC Access Control Point with Issuer Key Response to the request
 *
 * @return   kHAPError_None   If TLV response encoded, including error cases
 * @return   Other errors     No TLV response is encoded
 */
HAP_RESULT_USE_CHECK
static HAPError HandleNfcAccessIssuerKeyRequestList(
        HAPAccessoryServer* server HAP_UNUSED,
        HAPTLVWriter* responseWriter,
        const HAPCharacteristicValue_NfcAccessIssuerKeyRequest* keyRequest,
        HAPCharacteristicValue_NfcAccessControlPoint_IssuerKeyResponse* keyResponse) {
    HAPPrecondition(keyRequest);
    HAPPrecondition(keyResponse);

    // Allocate memory to store all identifiers to be listed
    uint16_t issuerKeyListSize = HAPPlatformNfcAccessGetMaximumIssuerKeys();
    HAPAssert(issuerKeyListSize != 0);
    uint8_t identifier[issuerKeyListSize][NFC_ACCESS_KEY_IDENTIFIER_BYTES];
    HAPPlatformNfcAccessIssuerKey issuerKeyList[issuerKeyListSize];
    for (uint8_t i = 0; i < issuerKeyListSize; i++) {
        issuerKeyList[i].identifier = identifier[i];
    }

    uint8_t numIssuerKeys;
    NfcAccessStatusCode statusCode;
    HAPError err = HAPPlatformNfcAccessIssuerKeyList(
            (HAPPlatformNfcAccessIssuerKey * _Nonnull) & issuerKeyList, &numIssuerKeys, &statusCode);
    if (err != kHAPError_None) {
        return err;
    }

    // Write the response for each issuer key
    for (uint8_t i = 0; i < numIssuerKeys; i++) {
        keyResponse->data.identifier.bytes = issuerKeyList[i].identifier;
        keyResponse->data.identifier.numBytes = NFC_ACCESS_KEY_IDENTIFIER_BYTES;
        keyResponse->data.identifierIsSet = true;
        keyResponse->data.statusCode = ConvertStatusCode(statusCode);
        keyResponse->data.statusCodeIsSet = true;

        err = HAPTLVWriterEncode(
                responseWriter, &kHAPCharacteristicTLVFormat_NfcAccessControlPoint_IssuerKeyResponse, keyResponse);
        if (err) {
            HAPAssert(
                    err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                    err == kHAPError_Busy);
            return err;
        }

        // Write a separator between each issuer key response
        if (i != (numIssuerKeys - 1)) {
            err = HAPTLVWriterAppend(
                    responseWriter,
                    &(const HAPTLV) { .type = kHAPCharacteristicTLVType_Separator,
                                      .value = { .bytes = NULL, .numBytes = 0 } });
            if (err != kHAPError_None) {
                return err;
            }
        }
    }

    return kHAPError_None;
}

/**
 * Handles NFC Access Issuer Key Request add operation
 *
 * @param      server           Accessory server instance
 * @param      responseWriter   TLV writer to produce output
 * @param      keyRequest       NFC Access Issuer Key Request
 * @param[out] keyResponse      NFC Access Control Point with Issuer Key Response to the request
 *
 * @return   kHAPError_None          If TLV response encoded, including error cases
 * @return   kHAPError_InvalidData   If the request TLV has bad data, no TLV response is encoded
 * @return   Other errors            No TLV response is encoded
 */
HAP_RESULT_USE_CHECK
static HAPError HandleNfcAccessIssuerKeyRequestAdd(
        HAPAccessoryServer* server HAP_UNUSED,
        HAPTLVWriter* responseWriter,
        const HAPCharacteristicValue_NfcAccessIssuerKeyRequest* keyRequest,
        HAPCharacteristicValue_NfcAccessControlPoint_IssuerKeyResponse* keyResponse) {
    HAPPrecondition(keyRequest);
    HAPPrecondition(keyResponse);

    if (!keyRequest->typeIsSet) {
        HAPLogError(&logObject, "%s: Type required", __func__);
        return kHAPError_InvalidData;
    }

    if (!keyRequest->keyIsSet) {
        HAPLogError(&logObject, "%s: Key required", __func__);
        return kHAPError_InvalidData;
    }

    // Key size depends on the key type
    uint8_t keyNumBytes = 0;
    switch (keyRequest->type) {
        case kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_Ed25519:
            keyNumBytes = ED25519_PUBLIC_KEY_BYTES;
            break;
        default:
            HAPLogError(&logObject, "%s: Type not supported=[0x%02X]", __func__, keyRequest->type);
            keyResponse->data.statusCode = ConvertStatusCode(NFC_ACCESS_STATUS_CODE_NOT_SUPPORTED);
            keyResponse->data.statusCodeIsSet = true;
            goto write_response;
    }

    // Double check that the key size is expected for the key type
    if (keyRequest->key.numBytes != keyNumBytes) {
        HAPLogError(
                &logObject,
                "%s: Key size mismatch: actual = %zu, expected=%u",
                __func__,
                keyRequest->key.numBytes,
                keyNumBytes);
        return kHAPError_InvalidData;
    }

    HAPPlatformNfcAccessIssuerKey issuerKey = { .type = keyRequest->type,
                                                .key = keyRequest->key.bytes,
                                                .keyNumBytes = keyRequest->key.numBytes };
    NfcAccessStatusCode statusCode;
    HAPError err =
            HAPPlatformNfcAccessIssuerKeyAdd(&issuerKey, NFC_ACCESS_ISSUER_KEY_CACHE_TYPE_REQUEST_ADD, &statusCode);
    if (err != kHAPError_None) {
        return err;
    }

    keyResponse->data.statusCode = ConvertStatusCode(statusCode);
    keyResponse->data.statusCodeIsSet = true;

write_response:
    // Write the response to this add operation
    err = HAPTLVWriterEncode(
            responseWriter, &kHAPCharacteristicTLVFormat_NfcAccessControlPoint_IssuerKeyResponse, keyResponse);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                err == kHAPError_Busy);
        return err;
    }

    return kHAPError_None;
}

/**
 * Handles NFC Access Issuer Key Request remove operation
 *
 * @param      server           Accessory server instance
 * @param      responseWriter   TLV writer to produce output
 * @param      keyRequest       NFC Access Issuer Key Request
 * @param[out] keyResponse      NFC Access Control Point with Issuer Key Response to the request
 *
 * @return   kHAPError_None          If TLV response encoded, including error cases
 * @return   kHAPError_InvalidData   If the request TLV has bad data, no TLV response is encoded
 * @return   Other errors            No TLV response is encoded
 */
HAP_RESULT_USE_CHECK
static HAPError HandleNfcAccessIssuerKeyRequestRemove(
        HAPAccessoryServer* server HAP_UNUSED,
        HAPTLVWriter* responseWriter,
        const HAPCharacteristicValue_NfcAccessIssuerKeyRequest* keyRequest,
        HAPCharacteristicValue_NfcAccessControlPoint_IssuerKeyResponse* keyResponse) {
    HAPPrecondition(keyRequest);
    HAPPrecondition(keyResponse);

    if (!keyRequest->identifierIsSet) {
        HAPLogError(&logObject, "%s: Identifier required", __func__);
        return kHAPError_InvalidData;
    }

    HAPPlatformNfcAccessIssuerKey issuerKey = {
        .identifier = keyRequest->identifier.bytes,
    };
    NfcAccessStatusCode statusCode;
    HAPError err = HAPPlatformNfcAccessIssuerKeyRemove(
            &issuerKey, NFC_ACCESS_ISSUER_KEY_CACHE_TYPE_REQUEST_REMOVE, &statusCode);
    if (err != kHAPError_None) {
        return err;
    }

    keyResponse->data.statusCode = ConvertStatusCode(statusCode);
    keyResponse->data.statusCodeIsSet = true;

    // Write the response to this add operation
    err = HAPTLVWriterEncode(
            responseWriter, &kHAPCharacteristicTLVFormat_NfcAccessControlPoint_IssuerKeyResponse, keyResponse);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                err == kHAPError_Busy);
        return err;
    }

    return kHAPError_None;
}

/**
 * Handles an NFC Access Issuer Key Request
 *
 * @param   server           Accessory server instance
 * @param   operation        Requested operation
 * @param   requestTLV       Decoded NFC Access Issuer Key Request TLV
 * @param   responseWriter   TLV writer to produce output
 *
 * @return   kHAPError_None          If TLV response encoded, including errors
 * @return   kHAPError_InvalidData   If the request TLV has bad data, no TLV response is encoded
 * @return   Other errors            No TLV response is encoded
 */
HAP_RESULT_USE_CHECK
static HAPError HandleNfcAccessIssuerKeyRequest(
        HAPAccessoryServer* server,
        HAPCharacteristicValue_NfcAccessControlPoint_OperationType operation,
        const HAPTLV* requestTLV,
        HAPTLVWriter* responseWriter) {
    HAPPrecondition(server);
    HAPPrecondition(requestTLV);
    HAPPrecondition(responseWriter);

    HAPTLVReader subReader;
    HAPTLVReaderCreate(&subReader, (void*) (uintptr_t) requestTLV->value.bytes, requestTLV->value.numBytes);

    HAPCharacteristicValue_NfcAccessIssuerKeyRequest keyRequest;
    HAPError err = HAPTLVReaderDecode(&subReader, &kHAPCharacteristicTLVFormat_NfcAccessIssuerKeyRequest, &keyRequest);
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }

    HAPCharacteristicValue_NfcAccessControlPoint_IssuerKeyResponse keyResponse;
    HAPRawBufferZero(&keyResponse, sizeof keyResponse);

    switch (operation) {
        case kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List: {
            err = HandleNfcAccessIssuerKeyRequestList(server, responseWriter, &keyRequest, &keyResponse);
            break;
        }
        case kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add: {
            err = HandleNfcAccessIssuerKeyRequestAdd(server, responseWriter, &keyRequest, &keyResponse);
            break;
        }
        case kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Remove: {
            err = HandleNfcAccessIssuerKeyRequestRemove(server, responseWriter, &keyRequest, &keyResponse);
            break;
        }
        default: {
            HAPLogInfo(&logObject, "%s: Invalid operation=[0x%02X]", __func__, operation);
            return kHAPError_InvalidData;
        }
    }

    // Handling of operation has failed in a way that response should not be sent
    if (err) {
        return err;
    }

    return kHAPError_None;
}

/**
 * Handles NFC Access Device Credential Key Request list operation
 *
 * @param      server           Accessory server instance
 * @param      responseWriter   TLV writer to produce output
 * @param      keyRequest       NFC Access Device Credential Key Request
 * @param[out] keyResponse      NFC Access Control Point with Device Credential Key Response to the request
 *
 * @return   kHAPError_None   If TLV response encoded, including error cases
 * @return   Other errors     No TLV response is encoded
 */
HAP_RESULT_USE_CHECK
static HAPError HandleNfcAccessDeviceCredentialKeyRequestList(
        HAPAccessoryServer* server HAP_UNUSED,
        HAPTLVWriter* responseWriter,
        const HAPCharacteristicValue_NfcAccessDeviceCredentialKeyRequest* keyRequest,
        HAPCharacteristicValue_NfcAccessControlPoint_DeviceCredentialKeyResponse* keyResponse) {
    HAPPrecondition(keyRequest);
    HAPPrecondition(keyResponse);

    if (!keyRequest->stateIsSet) {
        HAPLogError(&logObject, "%s: State required", __func__);
        return kHAPError_InvalidData;
    }

    // Allocate memory to store all identifiers to be listed
    uint16_t deviceCredentialKeyListSize = 0;
    switch (keyRequest->state) {
        case kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Suspended:
            deviceCredentialKeyListSize = HAPPlatformNfcAccessGetMaximumSuspendedDeviceCredentialKeys();
            break;
        case kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Active:
            deviceCredentialKeyListSize = HAPPlatformNfcAccessGetMaximumActiveDeviceCredentialKeys();
            break;
    }
    HAPAssert(deviceCredentialKeyListSize != 0);
    uint8_t identifier[deviceCredentialKeyListSize][NFC_ACCESS_KEY_IDENTIFIER_BYTES];
    uint8_t issuerKeyIdentifier[deviceCredentialKeyListSize][NFC_ACCESS_KEY_IDENTIFIER_BYTES];
    HAPPlatformNfcAccessDeviceCredentialKey deviceCredentialKeyList[deviceCredentialKeyListSize];
    for (uint8_t i = 0; i < deviceCredentialKeyListSize; i++) {
        deviceCredentialKeyList[i].identifier = identifier[i];
        deviceCredentialKeyList[i].issuerKeyIdentifier = issuerKeyIdentifier[i];
        deviceCredentialKeyList[i].state = keyRequest->state;
    }

    uint8_t numDeviceCredentialKeys;
    NfcAccessStatusCode statusCode;
    HAPError err = HAPPlatformNfcAccessDeviceCredentialKeyList(
            (HAPPlatformNfcAccessDeviceCredentialKey * _Nonnull) & deviceCredentialKeyList,
            &numDeviceCredentialKeys,
            &statusCode);
    if (err != kHAPError_None) {
        return err;
    }

    // Write the response for each device credential key
    for (uint8_t i = 0; i < numDeviceCredentialKeys; i++) {
        keyResponse->data.identifier.bytes = deviceCredentialKeyList[i].identifier;
        keyResponse->data.identifier.numBytes = NFC_ACCESS_KEY_IDENTIFIER_BYTES;
        keyResponse->data.identifierIsSet = true;
        keyResponse->data.issuerKeyIdentifier.bytes = deviceCredentialKeyList[i].issuerKeyIdentifier;
        keyResponse->data.issuerKeyIdentifier.numBytes = NFC_ACCESS_KEY_IDENTIFIER_BYTES;
        keyResponse->data.issuerKeyIdentifierIsSet = true;
        keyResponse->data.statusCode = ConvertStatusCode(statusCode);
        keyResponse->data.statusCodeIsSet = true;

        err = HAPTLVWriterEncode(
                responseWriter,
                &kHAPCharacteristicTLVFormat_NfcAccessControlPoint_DeviceCredentialKeyResponse,
                keyResponse);
        if (err) {
            HAPAssert(
                    err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                    err == kHAPError_Busy);
            return err;
        }

        // Write a separator between each issuer key response
        if (i != (numDeviceCredentialKeys - 1)) {
            err = HAPTLVWriterAppend(
                    responseWriter,
                    &(const HAPTLV) { .type = kHAPCharacteristicTLVType_Separator,
                                      .value = { .bytes = NULL, .numBytes = 0 } });
            if (err != kHAPError_None) {
                return err;
            }
        }
    }

    return kHAPError_None;
}

/**
 * Handles NFC Access Device Credential Key Request add operation
 *
 * @param      server           Accessory server instance
 * @param      responseWriter   TLV writer to produce output
 * @param      keyRequest       NFC Access Device Credential Key Request
 * @param[out] keyResponse      NFC Access Control Point with Device Credential Key Response to the request
 *
 * @return   kHAPError_None          If TLV response encoded, including error cases
 * @return   kHAPError_InvalidData   If the request TLV has bad data, no TLV response is encoded
 * @return   Other errors            No TLV response is encoded
 */
HAP_RESULT_USE_CHECK
static HAPError HandleNfcAccessDeviceCredentialKeyRequestAdd(
        HAPAccessoryServer* server HAP_UNUSED,
        HAPTLVWriter* responseWriter,
        const HAPCharacteristicValue_NfcAccessDeviceCredentialKeyRequest* keyRequest,
        HAPCharacteristicValue_NfcAccessControlPoint_DeviceCredentialKeyResponse* keyResponse) {
    HAPPrecondition(keyRequest);
    HAPPrecondition(keyResponse);

    if (!keyRequest->typeIsSet) {
        HAPLogError(&logObject, "%s: Type required", __func__);
        return kHAPError_InvalidData;
    }

    if (!keyRequest->keyIsSet) {
        HAPLogError(&logObject, "%s: Key required", __func__);
        return kHAPError_InvalidData;
    }

    if (!keyRequest->issuerKeyIdentifierIsSet) {
        HAPLogError(&logObject, "%s: Issuer Key Identifier required", __func__);
        return kHAPError_InvalidData;
    }

    if (!keyRequest->stateIsSet) {
        HAPLogError(&logObject, "%s: State required", __func__);
        return kHAPError_InvalidData;
    }

    // Key size depends on the key type
    uint8_t keyNumBytes = 0;
    switch (keyRequest->type) {
        case kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256:
            keyNumBytes = NIST256_PUBLIC_KEY_BYTES;
            break;
        default:
            HAPLogError(&logObject, "%s: Type not supported=[0x%02X]", __func__, keyRequest->type);
            keyResponse->data.statusCode = ConvertStatusCode(NFC_ACCESS_STATUS_CODE_NOT_SUPPORTED);
            keyResponse->data.statusCodeIsSet = true;
            goto write_response;
    }

    // Double check that the key size is expected for the key type
    if (keyRequest->key.numBytes != keyNumBytes) {
        HAPLogError(
                &logObject,
                "%s: Key size mismatch: actual = %zu, expected=%u",
                __func__,
                keyRequest->key.numBytes,
                keyNumBytes);
        return kHAPError_InvalidData;
    }

    HAPPlatformNfcAccessDeviceCredentialKey deviceCredentialKey = { .type = keyRequest->type,
                                                                    .key = keyRequest->key.bytes,
                                                                    .keyNumBytes = keyRequest->key.numBytes,
                                                                    .issuerKeyIdentifier =
                                                                            keyRequest->issuerKeyIdentifier.bytes,
                                                                    .state = keyRequest->state };
    NfcAccessStatusCode statusCode;
    HAPError err = HAPPlatformNfcAccessDeviceCredentialKeyAdd(&deviceCredentialKey, &statusCode);
    if (err != kHAPError_None) {
        return err;
    }

    keyResponse->data.statusCode = ConvertStatusCode(statusCode);
    keyResponse->data.statusCodeIsSet = true;

write_response:
    // Write the response to this add operation
    err = HAPTLVWriterEncode(
            responseWriter,
            &kHAPCharacteristicTLVFormat_NfcAccessControlPoint_DeviceCredentialKeyResponse,
            keyResponse);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                err == kHAPError_Busy);
        return err;
    }

    return kHAPError_None;
}

/**
 * Handles NFC Access Device Credential Key Request remove operation
 *
 * @param      server           Accessory server instance
 * @param      responseWriter   TLV writer to produce output
 * @param      keyRequest       NFC Access Device Credential Key Request
 * @param[out] keyResponse      NFC Access Control Point with Device Credential Key Response to the request
 *
 * @return   kHAPError_None          If TLV response encoded, including error cases
 * @return   kHAPError_InvalidData   If the request TLV has bad data, no TLV response is encoded
 * @return   Other errors            No TLV response is encoded
 */
HAP_RESULT_USE_CHECK
static HAPError HandleNfcAccessDeviceCredentialKeyRequestRemove(
        HAPAccessoryServer* server HAP_UNUSED,
        HAPTLVWriter* responseWriter,
        const HAPCharacteristicValue_NfcAccessDeviceCredentialKeyRequest* keyRequest,
        HAPCharacteristicValue_NfcAccessControlPoint_DeviceCredentialKeyResponse* keyResponse) {
    HAPPrecondition(keyRequest);
    HAPPrecondition(keyResponse);

    if (!keyRequest->identifierIsSet) {
        HAPLogError(&logObject, "%s: Identifier required", __func__);
        return kHAPError_InvalidData;
    }

    HAPPlatformNfcAccessDeviceCredentialKey deviceCredentialKey = {
        .identifier = keyRequest->identifier.bytes,
    };
    NfcAccessStatusCode statusCode;
    HAPError err = HAPPlatformNfcAccessDeviceCredentialKeyRemove(&deviceCredentialKey, &statusCode);
    if (err != kHAPError_None) {
        return err;
    }

    keyResponse->data.statusCode = ConvertStatusCode(statusCode);
    keyResponse->data.statusCodeIsSet = true;

    // Write the response to this add operation
    err = HAPTLVWriterEncode(
            responseWriter,
            &kHAPCharacteristicTLVFormat_NfcAccessControlPoint_DeviceCredentialKeyResponse,
            keyResponse);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                err == kHAPError_Busy);
        return err;
    }

    return kHAPError_None;
}

/**
 * Handles an NFC Access Device Credential Key Request
 *
 * @param   server           Accessory server instance
 * @param   operation        Requested operation
 * @param   requestTLV       Decoded NFC Access Device Credential Key Request TLV
 * @param   responseWriter   TLV writer to produce output
 *
 * @return   kHAPError_None          If TLV response encoded, including errors
 * @return   kHAPError_InvalidData   If the request TLV has bad data, no TLV response is encoded
 * @return   Other errors            No TLV response is encoded
 */
HAP_RESULT_USE_CHECK
static HAPError HandleNfcAccessDeviceCredentialKeyRequest(
        HAPAccessoryServer* server,
        HAPCharacteristicValue_NfcAccessControlPoint_OperationType operation,
        const HAPTLV* requestTLV,
        HAPTLVWriter* responseWriter) {
    HAPPrecondition(server);
    HAPPrecondition(requestTLV);
    HAPPrecondition(responseWriter);

    HAPTLVReader subReader;
    HAPTLVReaderCreate(&subReader, (void*) (uintptr_t) requestTLV->value.bytes, requestTLV->value.numBytes);

    HAPCharacteristicValue_NfcAccessDeviceCredentialKeyRequest keyRequest;
    HAPError err = HAPTLVReaderDecode(
            &subReader, &kHAPCharacteristicTLVFormat_NfcAccessDeviceCredentialKeyRequest, &keyRequest);
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }

    HAPCharacteristicValue_NfcAccessControlPoint_DeviceCredentialKeyResponse keyResponse;
    HAPRawBufferZero(&keyResponse, sizeof keyResponse);

    switch (operation) {
        case kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List: {
            err = HandleNfcAccessDeviceCredentialKeyRequestList(server, responseWriter, &keyRequest, &keyResponse);
            break;
        }
        case kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add: {
            err = HandleNfcAccessDeviceCredentialKeyRequestAdd(server, responseWriter, &keyRequest, &keyResponse);
            break;
        }
        case kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Remove: {
            err = HandleNfcAccessDeviceCredentialKeyRequestRemove(server, responseWriter, &keyRequest, &keyResponse);
            break;
        }
        default: {
            HAPLogInfo(&logObject, "%s: Invalid operation=[0x%02X]", __func__, operation);
            return kHAPError_InvalidData;
        }
    }

    // Handling of operation has failed in a way that response should not be sent
    if (err) {
        return err;
    }

    return kHAPError_None;
}

/**
 * Handles NFC Access Reader Key Request list operation
 *
 * @param      server           Accessory server instance
 * @param      responseWriter   TLV writer to produce output
 * @param      keyRequest       NFC Access Reader Key Request
 * @param[out] keyResponse      NFC Access Control Point with Reader Key Response to the request
 *
 * @return   kHAPError_None   If TLV response encoded, including error cases
 * @return   Other errors     No TLV response is encoded
 */
HAP_RESULT_USE_CHECK
static HAPError HandleNfcAccessReaderKeyRequestList(
        HAPAccessoryServer* server HAP_UNUSED,
        HAPTLVWriter* responseWriter,
        const HAPCharacteristicValue_NfcAccessReaderKeyRequest* keyRequest,
        HAPCharacteristicValue_NfcAccessControlPoint_ReaderKeyResponse* keyResponse) {
    HAPPrecondition(keyRequest);
    HAPPrecondition(keyResponse);

    // Allocate memory to store identifier to be listed
    uint8_t identifier[NFC_ACCESS_KEY_IDENTIFIER_BYTES];
    HAPPlatformNfcAccessReaderKey readerKey;
    readerKey.identifier = identifier;

    bool readerKeyFound = false;
    NfcAccessStatusCode statusCode;
    HAPError err = HAPPlatformNfcAccessReaderKeyList(&readerKey, &readerKeyFound, &statusCode);
    if (err != kHAPError_None) {
        return err;
    }

    // Write the response for reader key only if one exists
    if (readerKeyFound) {
        keyResponse->data.identifier.bytes = readerKey.identifier;
        keyResponse->data.identifier.numBytes = NFC_ACCESS_KEY_IDENTIFIER_BYTES;
        keyResponse->data.identifierIsSet = true;
        keyResponse->data.statusCode = ConvertStatusCode(statusCode);
        keyResponse->data.statusCodeIsSet = true;

        err = HAPTLVWriterEncode(
                responseWriter, &kHAPCharacteristicTLVFormat_NfcAccessControlPoint_ReaderKeyResponse, keyResponse);
        if (err) {
            HAPAssert(
                    err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                    err == kHAPError_Busy);
            return err;
        }
    }

    return kHAPError_None;
}

/**
 * Handles NFC Access Reader Key Request add operation
 *
 * @param      server           Accessory server instance
 * @param      responseWriter   TLV writer to produce output
 * @param      keyRequest       NFC Access Reader Key Request
 * @param[out] keyResponse      NFC Access Control Point with Reader Key Response to the request
 *
 * @return   kHAPError_None          If TLV response encoded, including error cases
 * @return   kHAPError_InvalidData   If the request TLV has bad data, no TLV response is encoded
 * @return   Other errors            No TLV response is encoded
 */
HAP_RESULT_USE_CHECK
static HAPError HandleNfcAccessReaderKeyRequestAdd(
        HAPAccessoryServer* server HAP_UNUSED,
        HAPTLVWriter* responseWriter,
        const HAPCharacteristicValue_NfcAccessReaderKeyRequest* keyRequest,
        HAPCharacteristicValue_NfcAccessControlPoint_ReaderKeyResponse* keyResponse) {
    HAPPrecondition(keyRequest);
    HAPPrecondition(keyResponse);

    if (!keyRequest->typeIsSet) {
        HAPLogError(&logObject, "%s: Type required", __func__);
        return kHAPError_InvalidData;
    }

    if (!keyRequest->keyIsSet) {
        HAPLogError(&logObject, "%s: Key required", __func__);
        return kHAPError_InvalidData;
    }

    if (!keyRequest->readerIdentifierIsSet) {
        HAPLogError(&logObject, "%s: Reader identifier required", __func__);
        return kHAPError_InvalidData;
    }

    // Key size depends on the key type
    uint8_t keyNumBytes = 0;
    switch (keyRequest->type) {
        case kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256:
            keyNumBytes = NIST256_PRIVATE_KEY_BYTES;
            break;
        default:
            HAPLogError(&logObject, "%s: Type not supported=[0x%02X]", __func__, keyRequest->type);
            keyResponse->data.statusCode = ConvertStatusCode(NFC_ACCESS_STATUS_CODE_NOT_SUPPORTED);
            keyResponse->data.statusCodeIsSet = true;
            goto write_response;
    }

    // Double check that the key size is expected for the key type
    if (keyRequest->key.numBytes != keyNumBytes) {
        HAPLogError(
                &logObject,
                "%s: Key size mismatch: actual = %zu, expected=%u",
                __func__,
                keyRequest->key.numBytes,
                keyNumBytes);
        return kHAPError_InvalidData;
    }

    HAPPlatformNfcAccessReaderKey readerKey = { .type = keyRequest->type,
                                                .key = keyRequest->key.bytes,
                                                .keyNumBytes = keyRequest->key.numBytes,
                                                .readerIdentifier = keyRequest->readerIdentifier.bytes };
    NfcAccessStatusCode statusCode;
    HAPError err = HAPPlatformNfcAccessReaderKeyAdd(&readerKey, &statusCode);
    if (err != kHAPError_None) {
        return err;
    }

    keyResponse->data.statusCode = ConvertStatusCode(statusCode);
    keyResponse->data.statusCodeIsSet = true;

write_response:
    // Write the response to this add operation
    err = HAPTLVWriterEncode(
            responseWriter, &kHAPCharacteristicTLVFormat_NfcAccessControlPoint_ReaderKeyResponse, keyResponse);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                err == kHAPError_Busy);
        return err;
    }

    return kHAPError_None;
}

/**
 * Handles NFC Access Reader Key Request remove operation
 *
 * @param      server           Accessory server instance
 * @param      responseWriter   TLV writer to produce output
 * @param      keyRequest       NFC Access Reader Key Request
 * @param[out] keyResponse      NFC Access Control Point with Reader Key Response to the request
 *
 * @return   kHAPError_None          If TLV response encoded, including error cases
 * @return   kHAPError_InvalidData   If the request TLV has bad data, no TLV response is encoded
 * @return   Other errors            No TLV response is encoded
 */
HAP_RESULT_USE_CHECK
static HAPError HandleNfcAccessReaderKeyRequestRemove(
        HAPAccessoryServer* server HAP_UNUSED,
        HAPTLVWriter* responseWriter,
        const HAPCharacteristicValue_NfcAccessReaderKeyRequest* keyRequest,
        HAPCharacteristicValue_NfcAccessControlPoint_ReaderKeyResponse* keyResponse) {
    HAPPrecondition(keyRequest);
    HAPPrecondition(keyResponse);

    if (!keyRequest->identifierIsSet) {
        HAPLogError(&logObject, "%s: Identifier required", __func__);
        return kHAPError_InvalidData;
    }

    HAPPlatformNfcAccessReaderKey readerKey = {
        .identifier = keyRequest->identifier.bytes,
    };
    NfcAccessStatusCode statusCode;
    HAPError err = HAPPlatformNfcAccessReaderKeyRemove(&readerKey, &statusCode);
    if (err != kHAPError_None) {
        return err;
    }

    keyResponse->data.statusCode = ConvertStatusCode(statusCode);
    keyResponse->data.statusCodeIsSet = true;

    // Write the response to this add operation
    err = HAPTLVWriterEncode(
            responseWriter, &kHAPCharacteristicTLVFormat_NfcAccessControlPoint_ReaderKeyResponse, keyResponse);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                err == kHAPError_Busy);
        return err;
    }

    return kHAPError_None;
}

/**
 * Handles an NFC Access Reader Key Request
 *
 * @param   server           Accessory server instance
 * @param   operation        Requested operation
 * @param   requestTLV       Decoded NFC Access Reader Key Request TLV
 * @param   responseWriter   TLV writer to produce output
 *
 * @return   kHAPError_None          If TLV response encoded, including errors
 * @return   kHAPError_InvalidData   If the request TLV has bad data, no TLV response is encoded
 * @return   Other errors            No TLV response is encoded
 */
HAP_RESULT_USE_CHECK
static HAPError HandleNfcAccessReaderKeyRequest(
        HAPAccessoryServer* server,
        HAPCharacteristicValue_NfcAccessControlPoint_OperationType operation,
        const HAPTLV* requestTLV,
        HAPTLVWriter* responseWriter) {
    HAPPrecondition(server);
    HAPPrecondition(requestTLV);
    HAPPrecondition(responseWriter);

    HAPTLVReader subReader;
    HAPTLVReaderCreate(&subReader, (void*) (uintptr_t) requestTLV->value.bytes, requestTLV->value.numBytes);

    HAPCharacteristicValue_NfcAccessReaderKeyRequest keyRequest;
    HAPError err = HAPTLVReaderDecode(&subReader, &kHAPCharacteristicTLVFormat_NfcAccessReaderKeyRequest, &keyRequest);
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }

    HAPCharacteristicValue_NfcAccessControlPoint_ReaderKeyResponse keyResponse;
    HAPRawBufferZero(&keyResponse, sizeof keyResponse);

    switch (operation) {
        case kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List: {
            err = HandleNfcAccessReaderKeyRequestList(server, responseWriter, &keyRequest, &keyResponse);
            break;
        }
        case kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add: {
            err = HandleNfcAccessReaderKeyRequestAdd(server, responseWriter, &keyRequest, &keyResponse);
            break;
        }
        case kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Remove: {
            err = HandleNfcAccessReaderKeyRequestRemove(server, responseWriter, &keyRequest, &keyResponse);
            break;
        }
        default: {
            HAPLogInfo(&logObject, "%s: Invalid operation=[0x%02X]", __func__, operation);
            return kHAPError_InvalidData;
        }
    }

    // Handling of operation has failed in a way that response should not be sent
    if (err) {
        return err;
    }

    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPHandleNfcAccessSupportedConfigurationRead(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_NfcAccessSupportedConfiguration));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_NfcAccess));
    HAPPrecondition(request->accessory->aid == 1);
    HAPPrecondition(responseWriter);

    HAPError err;

    HAPCharacteristicValue_NfcAccessSupportedConfiguration supportedConfiguration;
    HAPRawBufferZero(&supportedConfiguration, sizeof supportedConfiguration);

    // Get the supported configuration parameter values from PAL
    supportedConfiguration.maximumIssuerKeys = HAPPlatformNfcAccessGetMaximumIssuerKeys();
    supportedConfiguration.maximumSuspendedDeviceCredentialKeys =
            HAPPlatformNfcAccessGetMaximumSuspendedDeviceCredentialKeys();
    supportedConfiguration.maximumActiveDeviceCredentialKeys =
            HAPPlatformNfcAccessGetMaximumActiveDeviceCredentialKeys();

    err = HAPTLVWriterEncode(
            responseWriter, &kHAPCharacteristicTLVFormat_NfcAccessSupportedConfiguration, &supportedConfiguration);
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
HAPError HAPHandleNfcAccessControlPointRead(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_NfcAccessControlPoint));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_NfcAccess));
    HAPPrecondition(request->accessory->aid == 1);
    HAPPrecondition(responseWriter);

    HAPError err;

    // Check if this read is called for a previous write from the same session
    if (server->nfcAccess.hapSession == request->session) {
        server->nfcAccess.hapSession = NULL;

        if (server->nfcAccess.numResponseBytes > 0) {
            HAPAssert(server->nfcAccess.responseStorage.bytes);

            // Copy over TLV constructed during the write
            err = HAPTLVWriterExtend(
                    responseWriter,
                    server->nfcAccess.responseStorage.bytes,
                    server->nfcAccess.numResponseBytes,
                    server->nfcAccess.responseTlvType);

            if (err) {
                HAPAssert(
                        err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                        err == kHAPError_Busy);
            }

            // Done with the write response so invalidate the queued response
            server->nfcAccess.numResponseBytes = 0;

            return err;
        }

        // Empty response
        return kHAPError_None;
    }

    // No previous write response so read is not allowed
    return kHAPError_InvalidState;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPHandleNfcAccessControlPointWrite(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicWriteRequest* request,
        HAPTLVReader* requestReader,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_NfcAccessControlPoint));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_NfcAccess));
    HAPPrecondition(request->accessory->aid == 1);
    HAPPrecondition(requestReader);

    // Storage not allocated
    if (!server->nfcAccess.responseStorage.bytes) {
        return kHAPError_OutOfResources;
    }

    server->nfcAccess.numResponseBytes = 0;
    HAPError err;
    HAPTLVWriter responseWriter;
    HAPTLVWriterCreate(
            &responseWriter, server->nfcAccess.responseStorage.bytes, server->nfcAccess.responseStorage.maxBytes);

    HAPCharacteristicValue_NfcAccessControlPoint_OperationType operation =
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List;
    bool operationIsSet = false;
    HAPTLVType responseTlvType = 0;

    // Handle each NFC Access Control TLV
    for (;;) {
        HAPTLV tlv;
        bool found;
        err = HAPTLVReaderGetNext(requestReader, &found, &tlv);
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            HAPLogInfo(&logObject, "Parsing error for NFC Access Control");
            return err;
        }

        if (!found) {
            break;
        }

        // Make sure any key requests come after operation is specified
        if ((tlv.type != kHAPCharacteristicTLVType_NfcAccessControlPoint_OperationType) && !operationIsSet) {
            HAPLogInfo(&logObject, "Request TLV prior to Operation Type TLV: [%02x]", tlv.type);
            return kHAPError_InvalidData;
        }

        switch (tlv.type) {
            case kHAPCharacteristicTLVType_NfcAccessControlPoint_OperationType: {
                if (operationIsSet) {
                    HAPLogInfo(&logObject, "Duplicate Operation Type TLV: [%02x]", tlv.type);
                    return kHAPError_InvalidData;
                }
                operation = HAPReadUInt8(tlv.value.bytes);
                operationIsSet = true;
                break;
            }
            case kHAPCharacteristicTLVType_NfcAccessControlPoint_IssuerKeyRequest: {
                err = HandleNfcAccessIssuerKeyRequest(server, operation, &tlv, &responseWriter);
                if (err) {
                    HAPAssert(err == kHAPError_InvalidData);
                    return err;
                }
                responseTlvType = kHAPCharacteristicTLVType_NfcAccessControlPoint_IssuerKeyResponse;
                break;
            }
            case kHAPCharacteristicTLVType_NfcAccessControlPoint_DeviceCredentialKeyRequest: {
                err = HandleNfcAccessDeviceCredentialKeyRequest(server, operation, &tlv, &responseWriter);
                if (err) {
                    HAPAssert(err == kHAPError_InvalidData);
                    return err;
                }
                responseTlvType = kHAPCharacteristicTLVType_NfcAccessControlPoint_DeviceCredentialKeyResponse;
                break;
            }
            case kHAPCharacteristicTLVType_NfcAccessControlPoint_ReaderKeyRequest: {
                err = HandleNfcAccessReaderKeyRequest(server, operation, &tlv, &responseWriter);
                if (err) {
                    HAPAssert(err == kHAPError_InvalidData);
                    return err;
                }
                responseTlvType = kHAPCharacteristicTLVType_NfcAccessControlPoint_ReaderKeyResponse;
                break;
            }
        }
    }

    if (operationIsSet) {
        // Update storage session info for subsequent read as a result of write
        server->nfcAccess.hapSession = request->session;
        server->nfcAccess.responseTlvType = responseTlvType;
        void* bytes;
        HAPTLVWriterGetBuffer(&responseWriter, &bytes, &server->nfcAccess.numResponseBytes);
    }

    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPHandleNfcAccessConfigurationStateRead(
        HAPAccessoryServer* server,
        const HAPUInt16CharacteristicReadRequest* request HAP_UNUSED,
        uint16_t* value,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(value);

    *value = HAPPlatformNfcAccessGetConfigurationState();
    HAPLogDebug(&logObject, "NFC Access Configuration State read: %u", (unsigned) *value);

    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

void HAPHandleNfcAccessSessionInvalidate(HAPAccessoryServer* server, HAPSession* session) {
    HAPPrecondition(server);
    HAPPrecondition(session);

    if (server->nfcAccess.hapSession == session) {
        // Clear the write response in case the session is closed in between.
        // This only prevents ill behaving BLE conroller from reading off the last controller
        // write-response.
        server->nfcAccess.numResponseBytes = 0;
    }
}

#endif
