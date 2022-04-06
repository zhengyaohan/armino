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

#ifndef HAP_BLE_PDU_H
#define HAP_BLE_PDU_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPPDU.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)

/**
 * Fragmentation status of a HAP PDU.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Table 7-3 Control Field Bit 7 Values
 */
HAP_ENUM_BEGIN(uint8_t, HAPBLEPDUFragmentationStatus) { /** First fragment (or no fragmentation). */
                                                        kHAPBLEPDUFragmentationStatus_FirstFragment = 0x00,

                                                        /** Continuation of fragmented PDU. */
                                                        kHAPBLEPDUFragmentationStatus_Continuation = 0x01
} HAP_ENUM_END(uint8_t, HAPBLEPDUFragmentationStatus);

/**
 * Instance ID size of a HAP PDU.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Table 7-4 Control Field Bit 4 Values
 */
HAP_ENUM_BEGIN(uint8_t, HAPBLEPDUInstanceIDSize) { /** 16-bit IIDs (or IID = 0). */
                                                   kHAPBLEPDUInstanceIDSize_16Bit = 0x00,

                                                   /** 64-bit IIDs. */
                                                   kHAPBLEPDUInstanceIDSize_64Bit = 0x01
} HAP_ENUM_END(uint8_t, HAPBLEPDUInstanceIDSize);

/**
 * Type of a HAP PDU.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Table 7-5 Control Field Bit 1-3 Values
 */
HAP_ENUM_BEGIN(uint8_t, HAPBLEPDUType) { /** Request. */
                                         kHAPBLEPDUType_Request,

                                         /** Response. */
                                         kHAPBLEPDUType_Response
} HAP_ENUM_END(uint8_t, HAPBLEPDUType);

/**
 * Length of a HAP PDU Control Field.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Table 7-6 Control Field Bit 0 Values
 */
HAP_ENUM_BEGIN(uint8_t, HAPBLEPDUControlFieldLength) { /** 1 Byte Control Field. */
                                                       kHAPBLEPDUControlFieldLength_1Byte
} HAP_ENUM_END(uint8_t, HAPBLEPDUControlFieldLength);

/**
 * Header length of a HAP-BLE Request.
 */
#define kHAPBLEPDU_NumRequestHeaderBytes ((size_t)(1 + 4))

/**
 * Header length of a HAP-BLE Response.
 */
#define kHAPBLEPDU_NumResponseHeaderBytes ((size_t)(1 + 2))

/**
 * Header length of a continuation of a fragmented HAP-BLE PDU.
 */
#define kHAPBLEPDU_NumContinuationHeaderBytes ((size_t)(1 + 1))

/**
 * Additional header length of a PDU with a body.
 * Only applies to the first fragment of a HAP-BLE PDU.
 */
#define kHAPBLEPDU_NumBodyHeaderBytes ((size_t)(2))

/**
 * HAP PDU.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 7.3.3 HAP PDU Format
 */
typedef struct {
    /**
     * Control Field.
     *
     * Defines how the PDU and the rest of the bytes in the PDU are interpreted.
     *
     * @see HomeKit Accessory Protocol Specification R17
     *      Section 7.3.3.1 HAP PDU Header - Control Field
     */
    struct {
        /** Fragmentation status. */
        HAPBLEPDUFragmentationStatus fragmentationStatus;

        /** PDU type. */
        HAPBLEPDUType type;

        /** Control Field length. */
        HAPBLEPDUControlFieldLength length;
    } controlField;

    /**
     * PDU Fixed Params.
     *
     * Contains fixed params depending on the Control Field.
     */
    union {
        /**
         * HAP Request.
         *
         * @see HomeKit Accessory Protocol Specification R17
         *      Section 7.3.3.2 HAP Request Format
         */
        struct {
            HAPPDUOpcode opcode; /**< HAP Opcode. */
            uint8_t tid;         /**< TID. Transaction Identifier. */
            uint16_t iid;        /**< CharID / SvcID. Characteristic / service instance ID. */
        } request;

        /**
         * HAP Response.
         *
         * @see HomeKit Accessory Protocol Specification R17
         *      Section 7.3.3.3 HAP Response Format
         */
        struct {
            uint8_t tid;         /**< TID. Transaction Identifier. */
            HAPPDUStatus status; /**< Status. */
        } response;

        /**
         * Continuation of fragmented PDU.
         *
         * @see HomeKit Accessory Protocol Specification R17
         *      Section 7.3.3.5 HAP PDU Fragmentation Scheme
         */
        struct {
            uint8_t tid; /**< TID. Transaction Identifier. */
        } continuation;
    } fixedParams;

    /**
     * HAP-BLE PDU Body.
     *
     * - For PDUs without a body:
     *   - @c totalBodyBytes is 0.
     *   - @c bytes is NULL.
     *   - @c numBytes is 0.
     * - For PDUs that include a body:
     *   - @c totalBodyBytes contains the total PDU Body Length across all
     *     potential fragments.
     *   - @c bytes points to the start of the current body fragment.
     *   - @c numBytes contains the length of the current body fragment.
     *
     * @see HomeKit Accessory Protocol Specification R17
     *      Section 7.3.3.4 HAP PDU Body
     *
     * @see #HAPPDUTLVType
     */
    struct {
        uint16_t totalBodyBytes;     /**< PDU Body Length. */
        const void* _Nullable bytes; /**< Additional Params and Values in TLV8s. */
        uint16_t numBytes;           /**< Length of the body fragment. */
    } body;
} HAPBLEPDU;

/**
 * Deserialize the content of a buffer into a HAP-BLE PDU structure.
 * The buffer should contain the complete serialized PDU, or its first fragment.
 *
 * @param[out] pdu                  Deserialized PDU.
 * @param      bytes                Start of the memory region to deserialize.
 * @param      numBytes             Length of the memory region to deserialize.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the controller sent a malformed request.
 *
 * @remark To deserialize continuations of fragmented PDUs, use #HAPBLEPDUDeserializeContinuation.
 */
HAP_RESULT_USE_CHECK
HAPError HAPBLEPDUDeserialize(HAPBLEPDU* pdu, const void* bytes, size_t numBytes);

/**
 * Deserialize the content of a buffer into a HAP-BLE PDU structure.
 * The buffer should contain the serialized continuation of a fragmented PDU. Otherwise, an error is returned.
 *
 * - To deserialize complete PDUs or their first fragment, use #HAPBLEPDUDeserialize.
 *
 * @param[out] pdu                  Deserialized PDU.
 * @param      bytes                Start of the memory region to deserialize.
 * @param      numBytes             Length of the memory region to deserialize.
 * @param      typeOfFirstFragment  Type of the first fragment of the PDU.
 * @param      totalBodyBytes       Total body length (as received in first fragment).
 * @param      totalBodyBytesSoFar  Combined length of preceding body fragments.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the controller sent a malformed request.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 7.3.3.5 HAP PDU Fragmentation Scheme
 */
HAP_RESULT_USE_CHECK
HAPError HAPBLEPDUDeserializeContinuation(
        HAPBLEPDU* pdu,
        const void* bytes,
        size_t numBytes,
        HAPBLEPDUType typeOfFirstFragment,
        size_t totalBodyBytes,
        size_t totalBodyBytesSoFar);

/**
 * Serialize a HAP-BLE PDU structure.
 *
 * - For continuations of fragmented PDUs, @c pdu->body.totalBodyBytes is not validated.
 *
 * @param      pdu                  PDU to serialize.
 * @param      bytes                Start of the memory region to serialize into.
 * @param      maxBytes             Capacity of the memory region to serialize into.
 * @param[out] numBytes             Effective length of the serialized PDU.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough.
 */
HAP_RESULT_USE_CHECK
HAPError HAPBLEPDUSerialize(const HAPBLEPDU* pdu, void* bytes, size_t maxBytes, size_t* numBytes);

#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
