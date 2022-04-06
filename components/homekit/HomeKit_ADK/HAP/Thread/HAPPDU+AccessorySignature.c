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

#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)

#include "HAPAccessoryServer+Internal.h"
#include "HAPCharacteristic.h"
#include "HAPPDU+TLV.h"
#include "HAPPlatformFeatures.h"
#include "HAPTLV+Internal.h"

// Service and Characteristic Types.
// When Apple-defined UUIDs based on the HAP Base UUID 00000000-0000-1000-8000-0026BB765291 are encoded
// in short form, the -0000-1000-8000-0026BB765291 suffix is omitted and leading zero bytes are removed.
// The remaining bytes are sent in the same order as when sending a full UUID.
// To convert back to a full UUID, the process is reversed.
//
// Examples:
// - 00000000-0000-1000-8000-0026BB765291 -> []
// - 0000003E-0000-1000-8000-0026BB765291 -> [0x3E]
// - 00000001-0000-1000-8000-0026BB765291 -> [0x01]
// - 00000F25-0000-1000-8000-0026BB765291 -> [0x25, 0x0F]
// - 0000BBAB-0000-1000-8000-0026BB765291 -> [0xAB, 0xBB]
// - 00112233-0000-1000-8000-0026BB765291 -> [0x33, 0x22, 0x11]
// - 010004FF-0000-1000-8000-0026BB765291 -> [0xFF, 0x04, 0x00, 0x01]
// - FF000000-0000-1000-8000-0026BB765291 -> [0x00, 0x00, 0x00, 0xFF]
//
// See HomeKit Accessory Protocol Specification R17
// Section 6.6.1 Service and Characteristic Types

// HAP Opcode.
// - 0x09 / HAP-Accessory-Signature-Read
//
// See HomeKit Accessory Protocol Specification R17
// Section 7.3.3.2 HAP Request Format

// Additional Parameter Types.
// - 0x00 / Reserved to be used as a TLV separator
// - 0x13 / HAP-Param-Characteristic-Signature
// - 0x14 / HAP-Param-Characteristic-List
// - 0x15 / HAP-Param-Service-Signature
// - 0x16 / HAP-Param-Service-List
//
// See HomeKit Accessory Protocol Specification R17
// Section 7.3.3.4 HAP PDU Body

// HAP-Accessory-Signature-Read-Request.
//
// +-------------------------------------------------------+
// |                      PDU Header                       |
// +===============+==============+==========+=============+
// | Control Field | HAP PDU Type |   TID    | Instance ID |
// |   (1 Byte)    |   (1 Byte)   | (1 Byte) |  (2 Bytes)  |
// +---------------+--------------+----------+-------------+
// | 0b 0000 0000  |     0x09     |   0xXX   |   0x0000    |
// +---------------+--------------+----------+-------------+
//
// The instance ID in the HAP-Accessory-Signature-Read-Request is always set to 0.
//
// See HomeKit Accessory Protocol Specification R17
// Section 7.3.4.1 HAP-Characteristic-Signature-Read-Request

// HAP-Accessory-Signature-Read-Response.
//
// +-------------------------------------+------------------------------------------+
// |             PDU Header              |                 PDU Body                 |
// +===============+==========+==========+=============+============================+
// | Control Field |   TID    |  Status  | Body Length |            TLV             |
// |   (1 Byte)    | (1 Byte) | (1 Byte) |  (2 Bytes)  |       (Accessory-List)     |
// +---------------+----------+----------+-------------+----------------------------+
// | 0b 0000 0010  |   0xXX   |   0xXX   |    0xXXXX   |<0x18, 0xXX, Accessory-List>|
// +---------------+----------+----------+-------------+----------------------------+
//
// The HAP-PARAM-Accessory-List TLV contains a HAP-Param-Accessory-Signature sub TLV for each
// Accessory object.  Empty separator TLVs are included between HAP-Param-Accessory-Signature TLVs.
// +-----------------------------------------------------------------------------------------------+
// |                                  HAP-Param-Accessory-List                                     |
// +=================+==============+=================+==============+===========+=================+
// |       TLV       |     TLV      |       TLV       |     TLV      |    ...    |       TLV       |
// | (Acc Signature) | (Separator)  | (Acc Signature) | (Separator)  |           | (Acc Signature) |
// +-----------------+--------------+-----------------+--------------+-----------+-----------------+
// |  <0x19, 0xXX,   | <0x00, 0x00> |  <0x19, 0xXX,   | <0x00, 0x00> |   <...>   |  <0x19, 0xXX,   |
// | AccSignature1>  |              | AccSignature2>  |              |           | AccSignatureN>  |
// +-----------------+--------------+-----------------+--------------+-----------+-----------------+
//
// +----------------------------------+
// |   HAP-Param-Accessory-Signature  |
// +===============+==================+
// |      TLV      |       TLV        |
// |   (Acc ID)    |    (Svc List)    |
// +---------------+------------------+
// | <0x1A, 0x08,  |   <0x16, 0xXX,   |
// | 64-bit AccId> |  Service List  > |
// +---------------+------------------+
//
// The HAP-Param-Service-List TLV contains a HAP-Param-Service-Signature sub-TLV for each
// HAP service object. Empty separator TLVs are included between HAP-Param-Service-Signature TLVs.
//
// +-----------------------------------------------------------------------------------------------+
// |                                    HAP-Param-Service-List                                     |
// +=================+==============+=================+==============+===========+=================+
// |       TLV       |     TLV      |       TLV       |     TLV      |    ...    |       TLV       |
// | (Svc Signature) | (Separator)  | (Svc Signature) | (Separator)  |           | (Svc Signature) |
// +-----------------+--------------+-----------------+--------------+-----------+-----------------+
// |  <0x15, 0xXX,   | <0x00, 0x00> |  <0x15, 0xXX,   | <0x00, 0x00> |   <...>   |  <0x15, 0xXX,   |
// | SvcSignature1>  |              | SvcSignature2>  |              |           | SvcSignatureN>  |
// +-----------------+--------------+-----------------+--------------+-----------+-----------------+
//
// +----------------------------------------------------------------------------------------------------+
// |                                    HAP-Param-Service-Signature                                     |
// +===============+==================+==================+======================+=======================+
// |      TLV      |       TLV        |  TLV (Optional)  |    TLV (Optional)    |          TLV          |
// |   (Svc ID)    |    (Svc Type)    | (Svc Properties) |     (Linked Svc)     | (Characteristic-List) |
// +---------------+------------------+------------------+----------------------+-----------------------+
// | <0x07, 0x02,  |   <0x06, 0xXX,   |   <0x0F, 0x02,   | <0x10, 0xXX, SvcID1, |     <0x14, 0xXX,      |
// | 16-bit SvcID> | Short form UUID> |     0xXXXX>      | SvcID2, ..., SvcIDn> | Characteristic-List>  |
// +---------------+------------------+------------------+----------------------+-----------------------+
//
// The HAP-Param-Characteristic-List TLV contains a HAP-Param-Characteristic-Signature sub-TLV for each
// HAP characteristic object. Empty separator TLVs are included between HAP-Param-Characteristic-Signature TLVs.
//
// +-----------------------------------------------------------------------------------------------+
// |                                 HAP-Param-Characteristic-List                                 |
// +=================+==============+=================+==============+===========+=================+
// |       TLV       |     TLV      |       TLV       |     TLV      |    ...    |       TLV       |
// | (Chr Signature) | (Separator)  | (Chr Signature) | (Separator)  |           | (Chr Signature) |
// +-----------------+--------------+-----------------+--------------+-----------+-----------------+
// |  <0x13, 0xXX,   | <0x00, 0x00> |  <0x13, 0xXX,   | <0x00, 0x00> |   <...>   |  <0x13, 0xXX,   |
// | ChrSignature1>  |              | ChrSignature2>  |              |           | ChrSignatureN>  |
// +-----------------+--------------+-----------------+--------------+-----------+-----------------+
//
// +----------------------------------------------------------------------------------------------------------+
// |                                    HAP-Param-Characteristic-Signature                                    |
// +===============+==================+======================+================================+===============+
// |      TLV      |       TLV        |         TLV          |         TLV (Optional)         |      TLV      |
// |   (Chr ID)    |    (Chr Type)    | (HAP-Chr-Properties) |    (GATT-User-Description)     | (GATT-Format) |
// +---------------+------------------+----------------------+--------------------------------+---------------+
// | <0x05, 0x02,  |   <0x04, 0xXX,   |     <0x0A, 0x02,     |          <0x0B, 0xXX,          | <0x0C, 0x07,  |
// | 16-bit ChrID> | Short form UUID> | HAP-Chr-Properties>  | UTF-8 user description string> | GATT-Format>  |
// +---------------+------------------+----------------------+--------------------------------+---------------+
//
// +---------------------------------------------------------------------------------------------+
// |                       HAP-Param-Characteristic-Signature (continued)                        |
// +====================+==================+==========================+==========================+
// |   TLV (Optional)   |  TLV (Optional)  |      TLV (Optional)      |      TLV (Optional)      |
// | (GATT-Valid-Range) | (HAP-Step-Value) |    (HAP-Valid-Values)    | (HAP-Valid-Values-Range) |
// +--------------------+------------------+--------------------------+--------------------------+
// |    <0x0D, 0xXX,    |   <0x0E, 0xXX,   | <0x11, 0xXX, 0xAA, 0xBB, |    <0x12, 0xXX, 0xS1,    |
// | GATT-Valid-Range>  | HAP-Step-Value>  |        0xCC, ...>        |   0xE1, ...0xSN, 0xEN>   |
// +--------------------+------------------+--------------------------+--------------------------+
//
// In contrast to the HAP-Characteristic-Signature-Response and HAP-Service-Signature-Response:
// - The HAP-Param-Service-Instance-ID and HAP-Param-Service-Type fields are omitted in the
//   HAP-Param-Characteristic-Signature as they are already included in the embedding HAP-Param-Service-Signature.
// - Short form UUIDs are used when encoding HAP-Param-Characteristic-Type and HAP-Param-Service-Type fields.
//
// See HomeKit Accessory Protocol Specification R17
// Section 7.3.4.3 HAP-Characteristic-Signature-Read-Response (with Valid Values)
//
// See HomeKit Accessory Protocol Specification R17
// Section 7.3.4.13 HAP-Service-Signature-Read-Response

// HAP Accessory Signature Read Procedure.
// This procedure is used to read the signature of all HAP service and characteristic objects.
// The procedure must only be supported with a secure session.
//
// 1. HAP-PDU: HAP-Accessory-Signature-Read-Request
// 2. HAP-PDU: HAP-Accessory-Signature-Read-Response
//
// See HomeKit Accessory Protocol Specification R17
// Section 7.3.5.1 HAP Characteristic Signature Read Procedure

/**
 * Serializes HAP-Param-Characteristic-Signature.
 *
 * @param      characteristic       Characteristic.
 * @param      responseWriter       TLV writer.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If writer does not have enough capacity.
 */
HAP_RESULT_USE_CHECK
static HAPError
        SerializeCharacteristicSignature(const HAPCharacteristic* characteristic, HAPTLVWriter* responseWriter) {
    HAPPrecondition(characteristic);
    HAPPrecondition(responseWriter);

    HAPError err;

    HAPTLVWriter subWriter;
    {
        void* bytes;
        size_t maxBytes;
        HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);
        HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
    }

    err = HAPPDUValue_CharacteristicInstanceID_Encode(characteristic, &subWriter);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }
    err = HAPPDUValue_CharacteristicType_EncodeShortForm(characteristic, &subWriter);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }
    err = HAPPDUValue_HAPCharacteristicProperties_Encode(characteristic, &subWriter);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }
    err = HAPPDUValue_GATTUserDescription_Encode(characteristic, &subWriter);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }
    err = HAPPDUValue_GATTPresentationFormatDescriptor_Encode(characteristic, &subWriter);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }
    err = HAPPDUValue_GATTValidRange_Encode(characteristic, &subWriter);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }
    err = HAPPDUValue_HAPStepValueDescriptor_Encode(characteristic, &subWriter);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }
    err = HAPPDUValue_HAPValidValuesDescriptor_Encode(characteristic, &subWriter);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }
    err = HAPPDUValue_HAPValidValuesRangeDescriptor_Encode(characteristic, &subWriter);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // Finalize.
    void* bytes;
    size_t numBytes;
    HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPDUTLVType_CharacteristicSignature,
                              .value = { .bytes = bytes, .numBytes = numBytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    return kHAPError_None;
}

/**
 * Serializes HAP-Param-Characteristic-List.
 *
 * @param      service              Service.
 * @param      responseWriter       TLV writer.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If writer does not have enough capacity.
 */
HAP_RESULT_USE_CHECK
static HAPError SerializeCharacteristicList(const HAPService* service, HAPTLVWriter* responseWriter) {
    HAPPrecondition(service);
    HAPPrecondition(responseWriter);

    HAPError err;

    HAPTLVWriter subWriter;
    {
        void* bytes;
        size_t maxBytes;
        HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);
        HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
    }

    if (service->characteristics) {
        for (size_t i = 0; service->characteristics[i]; i++) {
            const HAPBaseCharacteristic* characteristic = service->characteristics[i];

            if (i) {
                err = HAPTLVWriterAppend(
                        &subWriter,
                        &(const HAPTLV) { .type = kHAPPDUTLVType_Separator,
                                          .value = { .bytes = NULL, .numBytes = 0 } });
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }
            }

            err = SerializeCharacteristicSignature(characteristic, &subWriter);
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }
        }
    }

    // Finalize.
    void* bytes;
    size_t numBytes;
    HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPDUTLVType_CharacteristicList,
                              .value = { .bytes = bytes, .numBytes = numBytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    return kHAPError_None;
}

/**
 * Serializes HAP-Param-Service-Signature.
 *
 * @param      service              Service.
 * @param      responseWriter       TLV writer.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If writer does not have enough capacity.
 */
HAP_RESULT_USE_CHECK
static HAPError SerializeServiceSignature(const HAPService* service, HAPTLVWriter* responseWriter) {
    HAPPrecondition(service);
    HAPPrecondition(responseWriter);

    HAPError err;

    HAPTLVWriter subWriter;
    {
        void* bytes;
        size_t maxBytes;
        HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);
        HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
    }

    err = HAPPDUValue_ServiceInstanceID_Encode(service, &subWriter);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }
    err = HAPBLEPDUTLVSerializeServiceTypeShortForm(service, &subWriter);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }
    err = HAPPDUValue_HAPServiceProperties_Encode(service, &subWriter);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }
    err = HAPPDUValue_HAPLinkedServices_Encode(service, &subWriter);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }
    err = SerializeCharacteristicList(service, &subWriter);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // Finalize.
    void* bytes;
    size_t numBytes;
    HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPDUTLVType_ServiceSignature,
                              .value = { .bytes = bytes, .numBytes = numBytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    return kHAPError_None;
}

/**
 * Serializes HAP-Param-Service-List.
 *
 * @param      accessory            Accessory.
 * @param      responseWriter       TLV writer.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If writer does not have enough capacity.
 */
HAP_RESULT_USE_CHECK
static HAPError SerializeServiceList(const HAPAccessory* accessory, HAPTLVWriter* responseWriter) {
    HAPPrecondition(accessory);
    HAPPrecondition(responseWriter);

    HAPError err;

    HAPTLVWriter subWriter;
    {
        void* bytes;
        size_t maxBytes;
        HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);
        HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
    }

    if (accessory->services) {
        for (size_t i = 0; accessory->services[i]; i++) {
            const HAPService* service = accessory->services[i];

            if (i) {
                err = HAPTLVWriterAppend(
                        &subWriter,
                        &(const HAPTLV) { .type = kHAPPDUTLVType_Separator,
                                          .value = { .bytes = NULL, .numBytes = 0 } });
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }
            }

            err = SerializeServiceSignature(service, &subWriter);
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }
        }
    }

    // Finalize.
    void* bytes;
    size_t numBytes;
    HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPDUTLVType_ServiceList, .value = { .bytes = bytes, .numBytes = numBytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    return kHAPError_None;
}

/**
 * Serializes an accessory
 *
 * @param accessory       Accessory to serialize
 * @param responseWriter  TLV writer
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If writer does not have enough capacity.
 */
HAP_RESULT_USE_CHECK
static HAPError SerializeAccessory(const HAPAccessory* accessory, HAPTLVWriter* responseWriter) {
    HAPPrecondition(accessory);
    HAPPrecondition(responseWriter);

    HAPError err;

    HAPTLVWriter subWriter;
    {
        void* bytes;
        size_t maxBytes;
        HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);
        HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
    }

    // Add accessory AID
    uint8_t aidBytes[] = { HAPExpandLittleUInt16((uint16_t) accessory->aid) };
    err = HAPTLVWriterAppend(
            &subWriter,
            &(const HAPTLV) { .type = kHAPPDUTLVType_AccessoryInstanceId,
                              .value = { .bytes = aidBytes, .numBytes = sizeof(aidBytes) } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // Add Service List
    err = SerializeServiceList(accessory, &subWriter);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // Finalize.
    void* bytes;
    size_t numBytes;
    HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPDUTLVType_AccessorySignature,
                              .value = { .bytes = bytes, .numBytes = numBytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPDUGetAccessorySignatureReadResponse(const HAPAccessoryServer* server, HAPTLVWriter* responseWriter) {
    HAPPrecondition(server);
    HAPPrecondition(responseWriter);

    HAPError err = kHAPError_None;
    int addedAccessories = 0;

    HAPTLVWriter subWriter;
    {
        void* bytes;
        size_t maxBytes;
        HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);
        HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
    }

    // Build our sub TLV out of accessories
    if (server->primaryAccessory) {
        err = SerializeAccessory(server->primaryAccessory, &subWriter);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
        addedAccessories++;
    }

    // Finalize.
    void* bytes;
    size_t numBytes;
    HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);

    // Add the Accessory List
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPDUTLVType_AccessoryList,
                              .value = { .bytes = bytes, .numBytes = numBytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
    }

    return err;
}

#endif
