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

#include "HAPAccessoryServer+Internal.h"
#include "HAPCharacteristic.h"
#include "HAPLog+Attributes.h"
#include "HAPLogSubsystem.h"
#include "HAPPDU+CharacteristicValue.h"
#include "HAPPDU+TLV.h"
#include "HAPSession.h"
#include "HAPTLV+Internal.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "PDU" };

/**
 * Structure representing the Char Value TLV item.
 */
typedef struct {
    void* bytes;     /**< Start of value. */
    size_t numBytes; /**< Length of value. */
    size_t maxBytes; /**< Capacity of value, including free memory after the value. */
} HAPPDUValue_CharacteristicValue;

/**
 * Parses the body of a a HAP-Characteristic-Write-Request.
 *
 * @param      characteristic_      Characteristic that received the request.
 * @param      requestReader        Reader to parse Characteristic value from. Reader content will become invalid.
 * @param[out] value                Char Value TLV item.
 * @param[out] remote               Whether the request appears to be sent remotely.
 * @param[out] authDataBytes        Additional authorization data.
 * @param[out] numAuthDataBytes     Length of additional authorization data.
 * @param[out] ttl                  TTL If a TTL TLV item was present; 0 Otherwise.
 * @param[out] hasReturnResponse    True If a Return-Response TLV item was present and set to 1; False Otherwise.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the controller sent a malformed request.
 */
HAP_RESULT_USE_CHECK
static HAPError ParseRequest(
        const HAPCharacteristic* characteristic_,
        HAPTLVReader* requestReader,
        HAPPDUValue_CharacteristicValue* value,
        bool* remote,
        const void** authDataBytes,
        size_t* numAuthDataBytes,
        const void** contextDataBytes,
        size_t* numContextDataBytes,
        uint8_t* ttl,
        bool* hasReturnResponse) {
    HAPPrecondition(characteristic_);
    const HAPBaseCharacteristic* characteristic = characteristic_;
    HAPPrecondition(requestReader);
    HAPPrecondition(value);
    HAPPrecondition(remote);
    HAPPrecondition(authDataBytes);
    HAPPrecondition(numAuthDataBytes);
    HAPPrecondition(ttl);
    HAPPrecondition(hasReturnResponse);

    HAPError err;

    // Retrieve buffer from the TLV reader (must be done before parsing, as parsing modifies internal state).
    // Buffer will be re-used as part of the output value.
    uint8_t* bytes = requestReader->bytes;
    size_t maxBytes = requestReader->maxBytes;

    HAPTLV valueTLV, authDataTLV, originTLV;
    valueTLV.type = kHAPPDUTLVType_Value;
    authDataTLV.type = kHAPPDUTLVType_AdditionalAuthorizationData;
    originTLV.type = kHAPPDUTLVType_Origin;

    // See HomeKit Accessory Protocol Specification R17
    // Section 7.3.5.4 HAP Characteristic Timed Write Procedure
    HAPTLV ttlTLV;
    ttlTLV.type = kHAPPDUTLVType_TTL;

    // See HomeKit Accessory Protocol Specification R17
    // Section 7.3.5.5 HAP Characteristic Write-With-Response Procedure
    HAPTLV returnResponseTLV;
    returnResponseTLV.type = kHAPPDUTLVType_ReturnResponse;

    // TODO: add spec ref
    HAPTLV contextDataTLV;
    contextDataTLV.type = kHAPPDUTLVType_ContextIdentifier;

    err = HAPTLVReaderGetAll(
            requestReader,
            (HAPTLV* const[]) {
                    &valueTLV, &authDataTLV, &originTLV, &ttlTLV, &returnResponseTLV, &contextDataTLV, NULL });
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }

    // HAP-Param-Value.
    if (!valueTLV.value.bytes) {
        HAPLog(&logObject, "HAP-Param-Value missing.");
        return kHAPError_InvalidData;
    }

    // HAP-Param-Origin.
    if (originTLV.value.bytes) {
        if (originTLV.value.numBytes != 1) {
            HAPLog(&logObject, "HAP-Param-Origin has invalid length (%lu).", (unsigned long) originTLV.value.numBytes);
            return kHAPError_InvalidData;
        }
        uint8_t origin = ((const uint8_t*) originTLV.value.bytes)[0];

        switch (origin) {
            case 0: {
                *remote = false;
                break;
            }
            case 1: {
                *remote = true;
                break;
            }
            default: {
                HAPLog(&logObject, "HAP-Param-Origin invalid: %u.", origin);
            }
                return kHAPError_InvalidData;
        }
    } else {
        *remote = false;
    }

    // HAP-Param-Additional-Authorization-Data.
    *authDataBytes = NULL;
    *numAuthDataBytes = 0;
    if (characteristic->properties.supportsAuthorizationData) {
        if (authDataTLV.value.bytes) {
            if (!originTLV.value.bytes) {
                // When additional authorization data is present it is included as an additional type
                // to the TLV8 format along with the Value and Remote TLV types.
                // See HomeKit Accessory Protocol Specification R17
                // Section 7.3.3.4 HAP PDU Body
                //
                // Certain controller versions (e.g., iOS 13) omit the Remote TLV type (HAP-Param-Origin)
                // when presenting additional authorization data. We choose to ignore this protocol violation
                // to increase compatibility with controllers.
                HAPLog(&logObject,
                       "Controller presented HAP-Param-Additional-Authorization-Data without HAP-Param-Origin.");
            }

            *authDataBytes = authDataTLV.value.bytes;
            *numAuthDataBytes = authDataTLV.value.numBytes;
        }
    } else if (authDataTLV.value.bytes) {
        HAPLog(&logObject, "HAP-Param-Additional-Authorization-Data present but characteristic does not support it.");
    } else {
        // Characteristic does not support additional authorization data
        // and HAP-Param-Additional-Authorization-Data is not present.
    }

    // HAP-Param-TTL.
    if (ttlTLV.value.bytes) {
        if (ttlTLV.value.numBytes != 1) {
            HAPLog(&logObject, "HAP-Param-TTL has invalid length (%lu).", (unsigned long) ttlTLV.value.numBytes);
            return kHAPError_InvalidData;
        }
        *ttl = ((const uint8_t*) ttlTLV.value.bytes)[0];
    } else {
        *ttl = 0;
    }

    // HAP-Param-Return-Response.
    if (returnResponseTLV.value.bytes) {
        if (returnResponseTLV.value.numBytes != 1) {
            HAPLog(&logObject,
                   "HAP-Param-Return-Response has invalid length (%lu).",
                   (unsigned long) returnResponseTLV.value.numBytes);
            return kHAPError_InvalidData;
        }
        uint8_t returnResponse = ((const uint8_t*) returnResponseTLV.value.bytes)[0];
        if (returnResponse != 1) {
            HAPLog(&logObject, "HAP-Param-Return-Response invalid: %u.", returnResponse);
            return kHAPError_InvalidData;
        }
        *hasReturnResponse = true;
    } else {
        *hasReturnResponse = false;
    }
    // HAP-Param-Context-Data
    *numContextDataBytes = 0;
    if (contextDataTLV.value.bytes) {
        *contextDataBytes = contextDataTLV.value.bytes;
        *numContextDataBytes = contextDataTLV.value.numBytes;
    }
    // Optimize memory. We want as much free space as possible after the value.
    // TLV values are always NULL terminated to simplify string handling. This property should be retained.
    // The NULL terminator is not counted in the TLV value's numBytes.
    // Case 1: [  AAD  |  VAL  | empty ]
    // Case 2: [  VAL  | empty |  AAD  ]
    // Case 3: [  VAL  |     empty     ]
    //         AAD and VAL fields contain an additional NULL byte.
    HAPRawBufferZero(value, sizeof *value);
    if (*authDataBytes) {
        size_t numValueBytes = valueTLV.value.numBytes;
        size_t numValueBytesWithNull = numValueBytes + 1;
        size_t numAuthDataBytesWithNull = *numAuthDataBytes + 1;

        void* valueStart;
        void* authDataStart;
        if (*authDataBytes < valueTLV.value.bytes) {
            // Case 1.
            authDataStart = &bytes[0];
            valueStart = &bytes[numAuthDataBytesWithNull];
        } else {
            // Case 2.
            authDataStart = &bytes[maxBytes - numAuthDataBytesWithNull];
            valueStart = &bytes[0];
        }

        // Move AAD.
        HAPRawBufferCopyBytes(authDataStart, *authDataBytes, numAuthDataBytesWithNull);
        *authDataBytes = authDataStart;

        // Move VAL.
        HAPRawBufferCopyBytes(valueStart, HAPNonnullVoid(valueTLV.value.bytes), numValueBytesWithNull);
        value->bytes = valueStart;
        value->numBytes = numValueBytes;
        value->maxBytes = maxBytes - numAuthDataBytesWithNull;
    } else {
        size_t numValueBytes = valueTLV.value.numBytes;
        size_t numValueBytesWithNULL = numValueBytes + 1;

        // Case 3.
        void* valueStart = &bytes[0];

        // Move VAL.
        HAPRawBufferCopyBytes(valueStart, HAPNonnullVoid(valueTLV.value.bytes), numValueBytesWithNULL);
        value->bytes = valueStart;
        value->numBytes = numValueBytes;
        value->maxBytes = maxBytes;
    }

    HAPAssert(((const char*) value->bytes)[value->numBytes] == '\0');
    if (*authDataBytes) {
        HAPAssert(((const char*) *authDataBytes)[*numAuthDataBytes] == '\0');
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPDUParseAndWriteCharacteristicValue(
        HAPAccessoryServer* server,
        HAPSession* session,
        const HAPCharacteristic* characteristic,
        const HAPService* service,
        const HAPAccessory* accessory,
        HAPTLVReader* requestReader,
        const HAPTime* _Nullable timedWriteStartTime,
        bool* hasExpired,
        bool* hasReturnResponse) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(characteristic);
    HAPPrecondition(service);
    HAPPrecondition(accessory);
    HAPPrecondition(requestReader);
    HAPPrecondition(hasExpired);
    HAPPrecondition(hasReturnResponse);

    HAPError err;

    HAPTransportType transportType = session->transportType;

    *hasExpired = false;
    *hasReturnResponse = false;

    HAPPDUValue_CharacteristicValue value = { .bytes = NULL, .numBytes = 0, .maxBytes = 0 };
    bool remote = false;
    const void* authDataBytes = NULL;
    size_t numAuthDataBytes = 0;
    uint8_t ttl = 0;
    const void* contextDataBytes = NULL;
    size_t numContextDataBytes = 0;
    err = ParseRequest(
            characteristic,
            requestReader,
            &value,
            &remote,
            &authDataBytes,
            &numAuthDataBytes,
            &contextDataBytes,
            &numContextDataBytes,
            &ttl,
            hasReturnResponse);

    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }

    // Handle Timed Write.
    if (timedWriteStartTime) {
        if (!ttl) {
            HAPLog(&logObject, "Timed Write Request did not include valid TTL.");
            return kHAPError_InvalidData;
        }

        HAPTime now = HAPPlatformClockGetCurrent();
        *hasExpired = now >= ttl * 100 * HAPMillisecond && now - ttl * 100 * HAPMillisecond > *timedWriteStartTime;
        if (*hasExpired) {
            return kHAPError_None;
        }
    }

    // Check against maximum length.
    if (value.numBytes > kHAPBLECharacteristic_MaxValueBytes) {
        HAPLog(&logObject,
               "Value exceeds maximum allowed length (%zu / %zu bytes).",
               value.numBytes,
               kHAPBLECharacteristic_MaxValueBytes);
        return kHAPError_InvalidData;
    }

    // Parse value and handle write.
    uint8_t* bytes = value.bytes;
    size_t numBytes = value.numBytes;
    switch (*((const HAPCharacteristicFormat*) characteristic)) {
        case kHAPCharacteristicFormat_Data: {
            err = HAPDataCharacteristicHandleWrite(
                    server,
                    &(const HAPDataCharacteristicWriteRequest) {
                            .transportType = transportType,
                            .session = session,
                            .characteristic = characteristic,
                            .service = service,
                            .accessory = accessory,
                            .remote = remote,
                            .authorizationData = { .bytes = authDataBytes, .numBytes = numAuthDataBytes } },
                    bytes,
                    numBytes,
                    server->context);
            if (err) {
                HAPAssert(
                        err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                        err == kHAPError_OutOfResources || err == kHAPError_NotAuthorized || err == kHAPError_Busy);
                return err;
            }
        }
            return kHAPError_None;
        case kHAPCharacteristicFormat_Bool: {
            if (numBytes != sizeof(bool)) {
                HAPLogCharacteristic(
                        &logObject,
                        characteristic,
                        service,
                        accessory,
                        "Unexpected value length: %lu.",
                        (unsigned long) numBytes);
                return kHAPError_InvalidData;
            }
            if (bytes[0] != 0 && bytes[0] != 1) {
                HAPLogCharacteristic(
                        &logObject, characteristic, service, accessory, "Unexpected bool value: %u.", bytes[0]);
                return kHAPError_InvalidData;
            }
            err = HAPBoolCharacteristicHandleWrite(
                    server,
                    &(const HAPBoolCharacteristicWriteRequest) {
                            .transportType = transportType,
                            .session = session,
                            .characteristic = characteristic,
                            .service = service,
                            .accessory = accessory,
                            .remote = remote,
                            .authorizationData = { .bytes = authDataBytes, .numBytes = numAuthDataBytes } },
                    bytes[0] != 0,
                    server->context);
            if (err) {
                HAPAssert(
                        err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                        err == kHAPError_OutOfResources || err == kHAPError_NotAuthorized || err == kHAPError_Busy);
                return err;
            }
        }
            return kHAPError_None;
        case kHAPCharacteristicFormat_UInt8: {
            if (numBytes != sizeof(uint8_t)) {
                HAPLogCharacteristic(
                        &logObject,
                        characteristic,
                        service,
                        accessory,
                        "Unexpected value length: %lu.",
                        (unsigned long) numBytes);
                return kHAPError_InvalidData;
            }
            err = HAPUInt8CharacteristicHandleWrite(
                    server,
                    &(const HAPUInt8CharacteristicWriteRequest) {
                            .transportType = transportType,
                            .session = session,
                            .characteristic = characteristic,
                            .service = service,
                            .accessory = accessory,
                            .remote = remote,

                            .authorizationData = { .bytes = authDataBytes, .numBytes = numAuthDataBytes },
                            .contextData = { .bytes = contextDataBytes, .numBytes = numContextDataBytes },
                    },
                    bytes[0],
                    server->context);
            if (err) {
                HAPAssert(
                        err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                        err == kHAPError_OutOfResources || err == kHAPError_NotAuthorized || err == kHAPError_Busy);
                return err;
            }
        }
            return kHAPError_None;
        case kHAPCharacteristicFormat_UInt16: {
            if (numBytes != sizeof(uint16_t)) {
                HAPLogCharacteristic(
                        &logObject,
                        characteristic,
                        service,
                        accessory,
                        "Unexpected value length: %lu.",
                        (unsigned long) numBytes);
                return kHAPError_InvalidData;
            }
            err = HAPUInt16CharacteristicHandleWrite(
                    server,
                    &(const HAPUInt16CharacteristicWriteRequest) {
                            .transportType = transportType,
                            .session = session,
                            .characteristic = characteristic,
                            .service = service,
                            .accessory = accessory,
                            .remote = remote,
                            .authorizationData = { .bytes = authDataBytes, .numBytes = numAuthDataBytes } },
                    HAPReadLittleUInt16(bytes),
                    server->context);
            if (err) {
                HAPAssert(
                        err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                        err == kHAPError_OutOfResources || err == kHAPError_NotAuthorized || err == kHAPError_Busy);
                return err;
            }
        }
            return kHAPError_None;
        case kHAPCharacteristicFormat_UInt32: {
            if (numBytes != sizeof(uint32_t)) {
                HAPLogCharacteristic(&logObject, characteristic, service, accessory, "Unexpected value length.");
                return kHAPError_InvalidData;
            }
            err = HAPUInt32CharacteristicHandleWrite(
                    server,
                    &(const HAPUInt32CharacteristicWriteRequest) {
                            .transportType = transportType,
                            .session = session,
                            .characteristic = characteristic,
                            .service = service,
                            .accessory = accessory,
                            .remote = remote,
                            .authorizationData = { .bytes = authDataBytes, .numBytes = numAuthDataBytes } },
                    HAPReadLittleUInt32(bytes),
                    server->context);
            if (err) {
                HAPAssert(
                        err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                        err == kHAPError_OutOfResources || err == kHAPError_NotAuthorized || err == kHAPError_Busy);
                return err;
            }
        }
            return kHAPError_None;
        case kHAPCharacteristicFormat_UInt64: {
            if (numBytes != sizeof(uint64_t)) {
                HAPLogCharacteristic(
                        &logObject,
                        characteristic,
                        service,
                        accessory,
                        "Unexpected value length: %lu.",
                        (unsigned long) numBytes);
                return kHAPError_InvalidData;
            }
            err = HAPUInt64CharacteristicHandleWrite(
                    server,
                    &(const HAPUInt64CharacteristicWriteRequest) {
                            .transportType = transportType,
                            .session = session,
                            .characteristic = characteristic,
                            .service = service,
                            .accessory = accessory,
                            .remote = remote,
                            .authorizationData = { .bytes = authDataBytes, .numBytes = numAuthDataBytes } },
                    HAPReadLittleUInt64(bytes),
                    server->context);
            if (err) {
                HAPAssert(
                        err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                        err == kHAPError_OutOfResources || err == kHAPError_NotAuthorized || err == kHAPError_Busy);
                return err;
            }
        }
            return kHAPError_None;
        case kHAPCharacteristicFormat_Int: {
            if (numBytes != sizeof(int32_t)) {
                HAPLogCharacteristic(
                        &logObject,
                        characteristic,
                        service,
                        accessory,
                        "Unexpected value length: %lu.",
                        (unsigned long) numBytes);
                return kHAPError_InvalidData;
            }
            err = HAPIntCharacteristicHandleWrite(
                    server,
                    &(const HAPIntCharacteristicWriteRequest) {
                            .transportType = transportType,
                            .session = session,
                            .characteristic = characteristic,
                            .service = service,
                            .accessory = accessory,
                            .remote = remote,
                            .authorizationData = { .bytes = authDataBytes, .numBytes = numAuthDataBytes } },
                    HAPReadLittleInt32(bytes),
                    server->context);
            if (err) {
                HAPAssert(
                        err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                        err == kHAPError_OutOfResources || err == kHAPError_NotAuthorized || err == kHAPError_Busy);
                return err;
            }
        }
            return kHAPError_None;
        case kHAPCharacteristicFormat_Float: {
            if (numBytes != sizeof(float)) {
                HAPLogCharacteristic(
                        &logObject,
                        characteristic,
                        service,
                        accessory,
                        "Unexpected value length: %lu.",
                        (unsigned long) numBytes);
                return kHAPError_InvalidData;
            }
            uint32_t bitPattern = HAPReadLittleUInt32(bytes);
            err = HAPFloatCharacteristicHandleWrite(
                    server,
                    &(const HAPFloatCharacteristicWriteRequest) {
                            .transportType = transportType,
                            .session = session,
                            .characteristic = characteristic,
                            .service = service,
                            .accessory = accessory,
                            .remote = remote,
                            .authorizationData = { .bytes = authDataBytes, .numBytes = numAuthDataBytes } },
                    HAPFloatFromBitPattern(bitPattern),
                    server->context);
            if (err) {
                HAPAssert(
                        err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                        err == kHAPError_OutOfResources || err == kHAPError_NotAuthorized || err == kHAPError_Busy);
                return err;
            }
        }
            return kHAPError_None;
        case kHAPCharacteristicFormat_String: {
            if (numBytes != HAPStringGetNumBytes(value.bytes)) {
                HAPLogSensitiveCharacteristicBuffer(
                        &logObject,
                        characteristic,
                        service,
                        accessory,
                        bytes,
                        numBytes,
                        "Unexpected string value (contains NULL bytes).");
                return kHAPError_InvalidData;
            }
            if (!HAPUTF8IsValidData(bytes, numBytes)) {
                HAPLogSensitiveCharacteristicBuffer(
                        &logObject,
                        characteristic,
                        service,
                        accessory,
                        bytes,
                        numBytes,
                        "Unexpected string value (invalid UTF-8 encoding).");
                return kHAPError_InvalidData;
            }
            err = HAPStringCharacteristicHandleWrite(
                    server,
                    &(const HAPStringCharacteristicWriteRequest) {
                            .transportType = transportType,
                            .session = session,
                            .characteristic = characteristic,
                            .service = service,
                            .accessory = accessory,
                            .remote = remote,
                            .authorizationData = { .bytes = authDataBytes, .numBytes = numAuthDataBytes } },
                    (const char*) bytes,
                    server->context);
            if (err) {
                HAPAssert(
                        err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                        err == kHAPError_OutOfResources || err == kHAPError_NotAuthorized || err == kHAPError_Busy);
                return err;
            }
        }
            return kHAPError_None;
        case kHAPCharacteristicFormat_TLV8: {
            HAPTLVReader reader;
            HAPTLVReaderCreateWithOptions(
                    &reader,
                    &(const HAPTLVReaderOptions) { .bytes = bytes, .numBytes = numBytes, .maxBytes = value.maxBytes });

            err = HAPTLV8CharacteristicHandleWrite(
                    server,
                    &(const HAPTLV8CharacteristicWriteRequest) {
                            .transportType = transportType,
                            .session = session,
                            .characteristic = characteristic,
                            .service = service,
                            .accessory = accessory,
                            .remote = remote,
                            .authorizationData = { .bytes = authDataBytes, .numBytes = numAuthDataBytes } },
                    &reader,
                    server->context);
            if (err) {
                HAPAssert(
                        err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                        err == kHAPError_OutOfResources || err == kHAPError_NotAuthorized || err == kHAPError_Busy);
                return err;
            }
        }
            return kHAPError_None;
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPPDUParseWriteCharacteristicValueTTL(
        const HAPCharacteristic* characteristic,
        HAPTLVReader* requestReader,
        uint8_t* ttl) {
    HAPPrecondition(characteristic);
    HAPPrecondition(requestReader);
    HAPPrecondition(ttl);

    HAPError err;

    HAPPDUValue_CharacteristicValue value = { .bytes = NULL, .numBytes = 0, .maxBytes = 0 };
    bool remote = false;
    const void* authDataBytes = NULL;
    size_t numAuthDataBytes = 0;
    bool hasReturnResponse;
    const void* contextDataBytes = NULL;
    size_t numContextDataBytes = 0;
    err = ParseRequest(
            characteristic,
            requestReader,
            &value,
            &remote,
            &authDataBytes,
            &numAuthDataBytes,
            &contextDataBytes,
            &numContextDataBytes,
            ttl,
            &hasReturnResponse);

    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
    }
    return err;
}

#endif
