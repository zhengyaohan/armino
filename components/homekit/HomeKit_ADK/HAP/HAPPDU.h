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

#ifndef HAP_PDU_H
#define HAP_PDU_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP+API.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * Opcode of a HAP PDU Request.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Table 7-8 HAP Opcode Description
 */
HAP_ENUM_BEGIN(uint8_t, HAPPDUOpcode) {
    /** HAP-Characteristic-Signature-Read. */
    kHAPPDUOpcode_CharacteristicSignatureRead = 0x01,

    /** HAP-Characteristic-Write. */
    kHAPPDUOpcode_CharacteristicWrite = 0x02,

    /** HAP-Characteristic-Read. */
    kHAPPDUOpcode_CharacteristicRead = 0x03,

    /** HAP-Characteristic-Timed-Write. */
    kHAPPDUOpcode_CharacteristicTimedWrite = 0x04,

    /** HAP-Characteristic-Execute-Write. */
    kHAPPDUOpcode_CharacteristicExecuteWrite = 0x05,

    /** HAP-Service-Signature-Read. */
    kHAPPDUOpcode_ServiceSignatureRead = 0x06,

    /** HAP-Characteristic-Configuration. */
    kHAPPDUOpcode_CharacteristicConfiguration = 0x07,

    /** HAP-Protocol-Configuration. */
    kHAPPDUOpcode_ProtocolConfiguration = 0x08,

    /** HAP-Accessory-Signature-Read. */
    kHAPPDUOpcode_AccessorySignatureRead = 0x09,

    /** HAP-Notification-Configuration-Read. */
    kHAPPDUOpcode_NotificationConfigurationRead = 0x0A,

    /** HAP-Notification-Register. */
    kHAPPDUOpcode_NotificationRegister = 0x0B,

    /** HAP-Notification-Deregister. */
    kHAPPDUOpcode_NotificationDeregister = 0x0C,

    /**
     * HAP-Token-Request / Response.
     *
     * @see HomeKit Accessory Protocol Specification R17
     *      Section 5.17 Software Authentication Procedure
     */
    kHAPPDUOpcode_Token = 0x10,

    /**
     * HAP-Token-Update-Request / Response.
     *
     * @see HomeKit Accessory Protocol Specification R17
     *      Section 5.17 Software Authentication Procedure
     */
    kHAPPDUOpcode_TokenUpdate = 0x11,

    /**
     * HAP-Info-Request / Response.
     *
     * @see HomeKit Accessory Protocol Specification R17
     *      Section 5.17 Software Authentication Procedure
     */
    kHAPPDUOpcode_Info = 0x12,
} HAP_ENUM_END(uint8_t, HAPPDUOpcode);

/**
 * Checks whether a value represents a valid HAP opcode.
 *
 * @param      value                Value to check.
 *
 * @return     true                 If the value is valid.
 * @return     false                Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPPDUOpcodeIsValid(uint8_t value);

/**
 * Returns the description of a HAP opcode.
 *
 * @param      opcode               Value of which to get description.
 *
 * @return Description of the opcode.
 */
HAP_RESULT_USE_CHECK
const char* HAPPDUOpcodeGetDescription(HAPPDUOpcode opcode);

/**
 * HAP opcode operation type.
 */
HAP_ENUM_BEGIN(uint8_t, HAPPDUOperationType) { /** Characteristic operation. */
                                               kHAPPDUOperationType_Characteristic = 1,

                                               /** Service operation. */
                                               kHAPPDUOperationType_Service,

                                               /** Accessory operation. */
                                               kHAPPDUOperationType_Accessory
} HAP_ENUM_END(uint8_t, HAPPDUOperationType);

/**
 * Returns the operation type of a HAP opcode.
 *
 * @param      opcode               Operation.
 *
 * @return Operation type.
 */
HAP_RESULT_USE_CHECK
HAPPDUOperationType HAPPDUOpcodeGetOperationType(HAPPDUOpcode opcode);

/**
 * Returns whether a HAP opcode is supported for a given transport type.
 *
 * @param      opcode               Operation.
 * @param      transportType        Transport type.
 *
 * @return true                     If opcode is supported for given transport type.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPPDUOpcodeIsSupportedForTransport(HAPPDUOpcode opcode, HAPTransportType transportType);

/**
 * Returns whether a HAP opcode is supported for a given characteristic.
 *
 * @param      opcode               Operation.
 * @param      characteristic       Characteristic.
 * @param      service              The service that contains the characteristic.
 * @param      accessory            The accessory that provides the service.
 *
 * @return true                     If opcode is supported for given characteristic.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPPDUOpcodeIsSupportedForCharacteristic(
        HAPPDUOpcode opcode,
        const HAPCharacteristic* characteristic,
        const HAPService* service,
        const HAPAccessory* accessory);

/**
 * Returns whether a HAP opcode is only supported on secure sessions.
 *
 * @param      opcode               Operation.
 *
 * @return true                     If opcode is only supported on secure sessions.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPPDUOpcodeRequiresSessionSecurity(HAPPDUOpcode opcode);

/**
 * Returns whether a HAP opcode is supported when using session security
 * based on transient Pair Setup (Software Authentication).
 *
 * @param      opcode               Operation.
 *
 * @return true                     If opcode is supported on transient sessions.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPPDUOpcodeIsSupportedOnTransientSessions(HAPPDUOpcode opcode);

/**
 * Returns whether a HAP Opcode has a body in the PDU or not.
 *
 * @param opcode                    Operation to check
 *
 * @return true                     If pdu will have a body
 * @return false                    If pdu only contains a
 *                                  header
 */
HAP_RESULT_USE_CHECK
bool HAPPDUOpcodeRequestHasBody(HAPPDUOpcode opcode);

/**
 * Returns the expected instance ID for a given operation, characteristic and transport type.
 *
 * @param      opcode               Operation.
 * @param      characteristic       Characteristic.
 * @param      service              The service that contains the characteristic.
 * @param      accessory            The accessory that provides the service.
 * @param      transportType        Transport type.
 *
 * @return Expected instance ID for specified operation.
 */
HAP_RESULT_USE_CHECK
uint64_t HAPPDUGetExpectedIIDForOperation(
        HAPPDUOpcode opcode,
        const HAPCharacteristic* characteristic,
        const HAPService* service,
        const HAPAccessory* accessory,
        HAPTransportType transportType);

/**
 * HAP status code.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Table 7-37 HAP Status Codes Description
 */
HAP_ENUM_BEGIN(uint8_t, HAPPDUStatus) {
    /** Success. */
    kHAPPDUStatus_Success = 0x00,

    /** Unsupported-PDU. */
    kHAPPDUStatus_UnsupportedPDU = 0x01,

    /** Max-Procedures. */
    kHAPPDUStatus_MaxProcedures = 0x02,

    /** Insufficient Authorization. */
    kHAPPDUStatus_InsufficientAuthorization = 0x03,

    /** Invalid instance ID. */
    kHAPPDUStatus_InvalidInstanceID = 0x04,

    /** Insufficient Authentication. */
    kHAPPDUStatus_InsufficientAuthentication = 0x05,

    /** Invalid Request. */
    kHAPPDUStatus_InvalidRequest = 0x06,

    /** Insufficient resources   */
    kHAPPDUStatus_InsufficientResources = 0x07,
} HAP_ENUM_END(uint8_t, HAPPDUStatus);

/**
 * Checks whether a value represents a valid HAP status code.
 *
 * @param      value                Value to check.
 *
 * @return true                     If the value is valid.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPPDUStatusIsValid(uint8_t value);

/**
 * Returns description of a HAP status code.
 *
 * @param      status               Value of which to get description.
 *
 * @return Description of the status.
 */
HAP_RESULT_USE_CHECK
const char* HAPPDUStatusGetDescription(HAPPDUStatus status);

/**
 * Parses a HAP request PDU without fragmentation.
 *
 * @param      pduBytes             Serialized PDU buffer.
 * @param      numPDUBytes          Length of PDU buffer.
 * @param[out] opcode               Operation.
 * @param[out] tid                  Transaction ID.
 * @param[out] iid                  Instance ID.
 * @param[out] bodyBytes            PDU body byte buffer, if available. NULL if none exists.
 * @param[out] numBodyBytes         Length of PDU body byte buffer.
 * @param[out] numHeaderBytes       Length of header bytes
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the PDU buffer does not encode a valid PDU.
 * @return kHAPError_InvalidState   If the PDU buffer does not contain a valid opcode
 *
 */
HAP_RESULT_USE_CHECK
HAPError HAPPDUParseRequestWithoutFragmentation(
        void* pduBytes,
        size_t numPDUBytes,
        uint8_t* opcode,
        uint8_t* tid,
        uint16_t* iid,
        void* _Nullable* _Nonnull bodyBytes,
        uint16_t* numBodyBytes,
        uint16_t* numHeaderBytes);

HAP_RESULT_USE_CHECK
HAPError HAPPDUVerifyMessage(void* requestBytes, size_t numRequestBytes);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
