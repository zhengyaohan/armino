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
#include "HAPLogSubsystem.h"
#include "HAPPDU+CharacteristicValue.h"
#include "HAPPDU+TLV.h"
#include "HAPSession.h"
#include "HAPTLV+Internal.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "PDU" };

/**
 * Serializes Char Value field of HAP-Characteristic-Read-Response.
 *
 * @param      valueBytes           Characteristic value.
 * @param      numValueBytes        Length of characteristic value.
 * @param      responseWriter       TLV writer.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If writer does not have enough capacity.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 7.3.4.7 HAP-Characteristic-Read-Response
 */

HAP_RESULT_USE_CHECK
static HAPError SerializeBytes(const void* bytes, size_t numBytes, HAPTLVWriter* responseWriter, uint8_t tlvType) {
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes <= kHAPBLECharacteristic_MaxValueBytes);
    HAPPrecondition(responseWriter);

    HAPError err;

    err = HAPTLVWriterAppend(
            responseWriter, &(const HAPTLV) { .type = tlvType, .value = { .bytes = bytes, .numBytes = numBytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static HAPError SerializeCharValue(const void* valueBytes, size_t numValueBytes, HAPTLVWriter* responseWriter) {
    HAPPrecondition(valueBytes);
    HAPPrecondition(numValueBytes <= kHAPBLECharacteristic_MaxValueBytes);
    HAPPrecondition(responseWriter);
    return SerializeBytes(valueBytes, numValueBytes, responseWriter, kHAPPDUTLVType_Value);
}
HAP_RESULT_USE_CHECK
static HAPError SerializeContextData(const void* contextBytes, size_t numContextBytes, HAPTLVWriter* responseWriter) {
    HAPPrecondition(contextBytes);
    HAPPrecondition(numContextBytes <= kHAPBLECharacteristic_MaxValueBytes);
    HAPPrecondition(responseWriter);
    return SerializeBytes(contextBytes, numContextBytes, responseWriter, kHAPPDUTLVType_EventNotificationContext);
}
HAP_RESULT_USE_CHECK
HAPError HAPPDUReadAndSerializeCharacteristicValue(
        HAPAccessoryServer* server,
        HAPSession* session,
        const HAPCharacteristic* characteristic,
        const HAPService* service,
        const HAPAccessory* accessory,
        HAPTLVWriter* responseWriter) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(characteristic);
    HAPPrecondition(service);
    HAPPrecondition(accessory);
    HAPPrecondition(responseWriter);

    HAPError err;

    void* bytes;
    bool shouldSerializeValue = true;
    size_t maxBytes;
    HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);

    HAPTransportType transportType = session->transportType;
    // Context data callbacks, if applicable
    HAPCharacteristicContextCallbacks* contextCallbacks = NULL;
    // Fetch characteristic value.
    size_t numBytes;
    switch (*((const HAPCharacteristicFormat*) characteristic)) {
        case kHAPCharacteristicFormat_Data: {
            err = HAPDataCharacteristicHandleRead(
                    server,
                    &(const HAPDataCharacteristicReadRequest) { .transportType = transportType,
                                                                .session = session,
                                                                .characteristic = characteristic,
                                                                .service = service,
                                                                .accessory = accessory },
                    bytes,
                    HAPMin(maxBytes, kHAPBLECharacteristic_MaxValueBytes),
                    &numBytes,
                    server->context);
            if (err) {
                HAPAssert(
                        err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                        err == kHAPError_Busy);
                return err;
            }
            break;
        }
        case kHAPCharacteristicFormat_Bool: {
            if (maxBytes < sizeof(bool)) {
                HAPLog(&logObject, "Not enough space to read %s value.", "Bool");
                return kHAPError_OutOfResources;
            }
            bool characteristicValue;
            err = HAPBoolCharacteristicHandleRead(
                    server,
                    &(const HAPBoolCharacteristicReadRequest) { .transportType = transportType,
                                                                .session = session,
                                                                .characteristic = characteristic,
                                                                .service = service,
                                                                .accessory = accessory },
                    &characteristicValue,
                    server->context);
            if (err) {
                HAPAssert(
                        err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                        err == kHAPError_Busy);
                return err;
            }
            uint8_t* b = bytes;
            b[0] = (uint8_t)(characteristicValue ? 1 : 0);
            numBytes = sizeof(bool);
            break;
        }
        case kHAPCharacteristicFormat_UInt8: {
            if (maxBytes < sizeof(uint8_t)) {
                HAPLog(&logObject, "Not enough space to read %s value.", "UInt8");
                return kHAPError_OutOfResources;
            }
            uint8_t characteristicValue;
            err = HAPUInt8CharacteristicHandleRead(
                    server,
                    &(const HAPUInt8CharacteristicReadRequest) { .transportType = transportType,
                                                                 .session = session,
                                                                 .characteristic = characteristic,
                                                                 .service = service,
                                                                 .accessory = accessory },
                    &characteristicValue,
                    server->context);
            if (err) {
                if (err == kHAPError_InvalidState &&
                    HAPUUIDAreEqual(
                            ((HAPBaseCharacteristic*) characteristic)->characteristicType,
                            &kHAPCharacteristicType_ProgrammableSwitchEvent)) {
                    HAPLogInfo(
                            &logObject,
                            "Read executed on programmable switch event unsuccessfully. Should return null.");
                    HAPLogInfo(&logObject, "[Too much time has passed or event has already been read]");
                    shouldSerializeValue = false;
                    err = HAPTLVWriterAppend(
                            responseWriter,
                            &(const HAPTLV) { .type = kHAPPDUTLVType_Value,
                                              .value = { .bytes = NULL, .numBytes = 0 } });
                    if (err) {
                        HAPAssert(err == kHAPError_OutOfResources);
                        return err;
                    }
                } else {
                    HAPAssert(
                            err == kHAPError_Unknown || err == kHAPError_InvalidState ||
                            err == kHAPError_OutOfResources || err == kHAPError_Busy);
                    return err;
                }
            } else {
                uint8_t* b = bytes;
                b[0] = characteristicValue;
                numBytes = sizeof(uint8_t);
            }
            if (((HAPBaseCharacteristic*) characteristic)->properties.supportsEventNotificationContextInformation) {
                contextCallbacks = &((HAPUInt8Characteristic*) characteristic)->contextCallbacks;
            }
            break;
        }
        case kHAPCharacteristicFormat_UInt16: {
            if (maxBytes < sizeof(uint16_t)) {
                HAPLog(&logObject, "Not enough space to read %s value.", "UInt16");
                return kHAPError_OutOfResources;
            }
            uint16_t characteristicValue;
            err = HAPUInt16CharacteristicHandleRead(
                    server,
                    &(const HAPUInt16CharacteristicReadRequest) { .transportType = transportType,
                                                                  .session = session,
                                                                  .characteristic = characteristic,
                                                                  .service = service,
                                                                  .accessory = accessory },
                    &characteristicValue,
                    server->context);
            if (err) {
                HAPAssert(
                        err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                        err == kHAPError_Busy);
                return err;
            }
            uint8_t* b = bytes;
            HAPWriteLittleUInt16(b, characteristicValue);
            numBytes = sizeof(uint16_t);
            break;
        }
        case kHAPCharacteristicFormat_UInt32: {
            if (maxBytes < sizeof(uint32_t)) {
                HAPLog(&logObject, "Not enough space to read %s value.", "UInt32");
                return kHAPError_OutOfResources;
            }
            uint32_t characteristicValue;
            err = HAPUInt32CharacteristicHandleRead(
                    server,
                    &(const HAPUInt32CharacteristicReadRequest) { .transportType = transportType,
                                                                  .session = session,
                                                                  .characteristic = characteristic,
                                                                  .service = service,
                                                                  .accessory = accessory },
                    &characteristicValue,
                    server->context);
            if (err) {
                HAPAssert(
                        err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                        err == kHAPError_Busy);
                return err;
            }
            uint8_t* b = bytes;
            HAPWriteLittleUInt32(b, characteristicValue);
            numBytes = sizeof(uint32_t);
            break;
        }
        case kHAPCharacteristicFormat_UInt64: {
            if (maxBytes < sizeof(uint64_t)) {
                HAPLog(&logObject, "Not enough space to read %s value.", "UInt64");
                return kHAPError_OutOfResources;
            }
            uint64_t characteristicValue;
            err = HAPUInt64CharacteristicHandleRead(
                    server,
                    &(const HAPUInt64CharacteristicReadRequest) { .transportType = transportType,
                                                                  .session = session,
                                                                  .characteristic = characteristic,
                                                                  .service = service,
                                                                  .accessory = accessory },
                    &characteristicValue,
                    server->context);
            if (err) {
                HAPAssert(
                        err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                        err == kHAPError_Busy);
                return err;
            }
            uint8_t* b = bytes;
            HAPWriteLittleUInt64(b, characteristicValue);
            numBytes = sizeof(uint64_t);
            break;
        }
        case kHAPCharacteristicFormat_Int: {
            if (maxBytes < sizeof(int32_t)) {
                HAPLog(&logObject, "Not enough space to read %s value.", "Int32");
                return kHAPError_OutOfResources;
            }
            int32_t characteristicValue;
            err = HAPIntCharacteristicHandleRead(
                    server,
                    &(const HAPIntCharacteristicReadRequest) { .transportType = transportType,
                                                               .session = session,
                                                               .characteristic = characteristic,
                                                               .service = service,
                                                               .accessory = accessory },
                    &characteristicValue,
                    server->context);
            if (err) {
                HAPAssert(
                        err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                        err == kHAPError_Busy);
                return err;
            }
            uint8_t* b = bytes;
            HAPWriteLittleInt32(b, characteristicValue);
            numBytes = sizeof(int32_t);
            break;
        }
        case kHAPCharacteristicFormat_Float: {
            if (maxBytes < sizeof(float)) {
                HAPLog(&logObject, "Not enough space to read %s value.", "Float");
                return kHAPError_OutOfResources;
            }
            float characteristicValue;
            err = HAPFloatCharacteristicHandleRead(
                    server,
                    &(const HAPFloatCharacteristicReadRequest) { .transportType = transportType,
                                                                 .session = session,
                                                                 .characteristic = characteristic,
                                                                 .service = service,
                                                                 .accessory = accessory },
                    &characteristicValue,
                    server->context);
            if (err) {
                HAPAssert(
                        err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                        err == kHAPError_Busy);
                return err;
            }
            uint32_t bitPattern = HAPFloatGetBitPattern(characteristicValue);
            uint8_t* b = bytes;
            HAPWriteLittleUInt32(b, bitPattern);
            numBytes = sizeof(float);
            break;
        }
        case kHAPCharacteristicFormat_String: {
            err = HAPStringCharacteristicHandleRead(
                    server,
                    &(const HAPStringCharacteristicReadRequest) { .transportType = transportType,
                                                                  .session = session,
                                                                  .characteristic = characteristic,
                                                                  .service = service,
                                                                  .accessory = accessory },
                    bytes,
                    HAPMin(maxBytes, kHAPBLECharacteristic_MaxValueBytes),
                    server->context);
            if (err) {
                HAPAssert(
                        err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                        err == kHAPError_Busy);
                return err;
            }
            numBytes = HAPStringGetNumBytes(bytes);
            break;
        }
        case kHAPCharacteristicFormat_TLV8: {
            HAPTLVWriter writer;
            HAPTLVWriterCreate(&writer, bytes, HAPMin(maxBytes, kHAPBLECharacteristic_MaxValueBytes));
            err = HAPTLV8CharacteristicHandleRead(
                    server,
                    &(const HAPTLV8CharacteristicReadRequest) { .transportType = transportType,
                                                                .session = session,
                                                                .characteristic =
                                                                        (const HAPTLV8Characteristic*) characteristic,
                                                                .service = service,
                                                                .accessory = accessory },
                    &writer,
                    server->context);
            if (err) {
                HAPAssert(
                        err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                        err == kHAPError_Busy);
                return err;
            }

            void* tlvBytes;
            HAPTLVWriterGetBuffer(&writer, &tlvBytes, &numBytes);
            HAPAssert(tlvBytes == bytes);
            break;
        }
        default:
            HAPFatalError();
    }

    if (shouldSerializeValue) {
        HAPAssert(numBytes <= maxBytes);
        err = SerializeCharValue(bytes, numBytes, responseWriter);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
    }
    if (contextCallbacks != NULL) {
        HAPTLVWriter* writer;
        err = contextCallbacks->handleReadContextData(server, characteristic, &writer, server->context);
        if (err) {
            HAPLogError(&logObject, "Error while reading context data from characteristic");
            return err;
        }
        if (writer->numBytes != 0) {
            err = SerializeContextData(writer->bytes, writer->numBytes, responseWriter);
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }
        }
    }
    return kHAPError_None;
}

#endif
