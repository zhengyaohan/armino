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

#include "HAPPDU.h"

#include "HAPCharacteristic.h"
#include "HAPLogSubsystem.h"
#include "HAPPDU+Characteristic.h"
#include "HAPPDU+Service.h"
#include "HAPSession.h"

static const size_t PDU_HEADER_SIZE_NO_BODY = 5;
static const size_t PDU_HEADER_SIZE_WITH_BODY = 7;
static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "PDU" };

HAP_RESULT_USE_CHECK
bool HAPPDUOpcodeIsValid(uint8_t value) {
    HAPAssert(sizeof(HAPPDUOpcode) == sizeof value);
    switch ((HAPPDUOpcode) value) {
        case kHAPPDUOpcode_CharacteristicSignatureRead:
        case kHAPPDUOpcode_CharacteristicWrite:
        case kHAPPDUOpcode_CharacteristicRead:
        case kHAPPDUOpcode_CharacteristicTimedWrite:
        case kHAPPDUOpcode_CharacteristicExecuteWrite:
        case kHAPPDUOpcode_ServiceSignatureRead:
        case kHAPPDUOpcode_CharacteristicConfiguration:
        case kHAPPDUOpcode_ProtocolConfiguration:
        case kHAPPDUOpcode_AccessorySignatureRead:
        case kHAPPDUOpcode_NotificationConfigurationRead:
        case kHAPPDUOpcode_NotificationRegister:
        case kHAPPDUOpcode_NotificationDeregister:
        case kHAPPDUOpcode_Token:
        case kHAPPDUOpcode_TokenUpdate:
        case kHAPPDUOpcode_Info: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
const char* HAPPDUOpcodeGetDescription(HAPPDUOpcode opcode) {
    HAPPrecondition(HAPPDUOpcodeIsValid(opcode));

    switch (opcode) {
        case kHAPPDUOpcode_CharacteristicSignatureRead:
            return "HAP-Characteristic-Signature-Read";
        case kHAPPDUOpcode_CharacteristicWrite:
            return "HAP-Characteristic-Write";
        case kHAPPDUOpcode_CharacteristicRead:
            return "HAP-Characteristic-Read";
        case kHAPPDUOpcode_CharacteristicTimedWrite:
            return "HAP-Characteristic-Timed-Write";
        case kHAPPDUOpcode_CharacteristicExecuteWrite:
            return "HAP-Characteristic-Execute-Write";
        case kHAPPDUOpcode_ServiceSignatureRead:
            return "HAP-Service-Signature-Read";
        case kHAPPDUOpcode_CharacteristicConfiguration:
            return "HAP-Characteristic-Configuration";
        case kHAPPDUOpcode_ProtocolConfiguration:
            return "HAP-Protocol-Configuration";
        case kHAPPDUOpcode_AccessorySignatureRead:
            return "HAP-Accessory-Signature-Read";
        case kHAPPDUOpcode_NotificationConfigurationRead:
            return "HAP-Notification-Configuration-Read";
        case kHAPPDUOpcode_NotificationRegister:
            return "HAP-Notification-Register";
        case kHAPPDUOpcode_NotificationDeregister:
            return "HAP-Notification-Deregister";
        case kHAPPDUOpcode_Token:
            return "HAP-Token";
        case kHAPPDUOpcode_TokenUpdate:
            return "HAP-Token-Update";
        case kHAPPDUOpcode_Info:
            return "HAP-Info";
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
HAPPDUOperationType HAPPDUOpcodeGetOperationType(HAPPDUOpcode opcode) {
    HAPPrecondition(HAPPDUOpcodeIsValid(opcode));

    switch (opcode) {
        case kHAPPDUOpcode_CharacteristicSignatureRead:
            return kHAPPDUOperationType_Characteristic;
        case kHAPPDUOpcode_CharacteristicWrite:
            return kHAPPDUOperationType_Characteristic;
        case kHAPPDUOpcode_CharacteristicRead:
            return kHAPPDUOperationType_Characteristic;
        case kHAPPDUOpcode_CharacteristicTimedWrite:
            return kHAPPDUOperationType_Characteristic;
        case kHAPPDUOpcode_CharacteristicExecuteWrite:
            return kHAPPDUOperationType_Characteristic;
        case kHAPPDUOpcode_ServiceSignatureRead:
            return kHAPPDUOperationType_Service;
        case kHAPPDUOpcode_CharacteristicConfiguration:
            return kHAPPDUOperationType_Characteristic;
        case kHAPPDUOpcode_ProtocolConfiguration:
            return kHAPPDUOperationType_Service;
        case kHAPPDUOpcode_AccessorySignatureRead:
            return kHAPPDUOperationType_Accessory;
        case kHAPPDUOpcode_NotificationConfigurationRead:
            return kHAPPDUOperationType_Characteristic;
        case kHAPPDUOpcode_NotificationRegister:
            return kHAPPDUOperationType_Characteristic;
        case kHAPPDUOpcode_NotificationDeregister:
            return kHAPPDUOperationType_Characteristic;
        case kHAPPDUOpcode_Token:
            return kHAPPDUOperationType_Accessory;
        case kHAPPDUOpcode_TokenUpdate:
            return kHAPPDUOperationType_Accessory;
        case kHAPPDUOpcode_Info:
            return kHAPPDUOperationType_Accessory;
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
bool HAPPDUOpcodeIsSupportedForTransport(HAPPDUOpcode opcode, HAPTransportType transportType) {
    HAPPrecondition(HAPPDUOpcodeIsValid(opcode));
    HAPPrecondition(HAPTransportTypeIsValid(transportType));

    switch (opcode) {
        case kHAPPDUOpcode_CharacteristicSignatureRead:
            return transportType != kHAPTransportType_IP;
        case kHAPPDUOpcode_CharacteristicWrite:
            return transportType != kHAPTransportType_IP;
        case kHAPPDUOpcode_CharacteristicRead:
            return transportType != kHAPTransportType_IP;
        case kHAPPDUOpcode_CharacteristicTimedWrite:
            return transportType != kHAPTransportType_IP;
        case kHAPPDUOpcode_CharacteristicExecuteWrite:
            return transportType != kHAPTransportType_IP;
        case kHAPPDUOpcode_ServiceSignatureRead:
            return transportType != kHAPTransportType_IP;
        case kHAPPDUOpcode_CharacteristicConfiguration:
            return transportType != kHAPTransportType_IP;
        case kHAPPDUOpcode_ProtocolConfiguration:
            return transportType != kHAPTransportType_IP;
        case kHAPPDUOpcode_AccessorySignatureRead:
            return transportType == kHAPTransportType_Thread;
        case kHAPPDUOpcode_NotificationConfigurationRead:
            return transportType == kHAPTransportType_Thread;
        case kHAPPDUOpcode_NotificationRegister:
            return transportType == kHAPTransportType_Thread;
        case kHAPPDUOpcode_NotificationDeregister:
            return transportType == kHAPTransportType_Thread;
        case kHAPPDUOpcode_Token:
            return true;
        case kHAPPDUOpcode_TokenUpdate:
            return true;
        case kHAPPDUOpcode_Info:
            return true;
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
bool HAPPDUOpcodeIsSupportedForCharacteristic(
        HAPPDUOpcode opcode,
        const HAPCharacteristic* characteristic,
        const HAPService* service,
        const HAPAccessory* accessory) {
    HAPPrecondition(HAPPDUOpcodeIsValid(opcode));
    HAPPrecondition(characteristic);
    HAPPrecondition(service);
    HAPPrecondition(accessory);

    switch (HAPPDUOpcodeGetOperationType(opcode)) {
        case kHAPPDUOperationType_Characteristic: {
            return true;
        }
        case kHAPPDUOperationType_Service: {
            return HAPPDUAreServiceProceduresSupportedOnCharacteristic(characteristic);
        }
        case kHAPPDUOperationType_Accessory: {
            return HAPPDUAreServiceProceduresSupportedOnCharacteristic(characteristic) &&
                   HAPPDUAreAccessoryProceduresSupportedOnService(service);
        }
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
bool HAPPDUOpcodeRequiresSessionSecurity(HAPPDUOpcode opcode) {
    HAPPrecondition(HAPPDUOpcodeIsValid(opcode));

    switch (opcode) {
        case kHAPPDUOpcode_CharacteristicSignatureRead: {
            // The signature read procedure must be supported with and without a secure session.
            // See HomeKit Accessory Protocol Specification R17
            // Section 7.3.5.1 HAP Characteristic Signature Read Procedure
            return false;
        }
        case kHAPPDUOpcode_CharacteristicWrite: {
            // Depends on the properties of the addressed characteristic.
            return false;
        }
        case kHAPPDUOpcode_CharacteristicRead: {
            // Depends on the properties of the addressed characteristic.
            return false;
        }
        case kHAPPDUOpcode_CharacteristicTimedWrite: {
            // Depends on the properties of the addressed characteristic.
            return false;
        }
        case kHAPPDUOpcode_CharacteristicExecuteWrite: {
            // Depends on the properties of the addressed characteristic.
            return false;
        }
        case kHAPPDUOpcode_ServiceSignatureRead: {
            // There is no specific documentation about the security properties for service signature reads.
            // However, it makes sense to handle these consistently with characteristic signature reads.
            //
            // The signature read procedure must be supported with and without a secure session.
            // See HomeKit Accessory Protocol Specification R17
            // Section 7.3.5.1 HAP Characteristic Signature Read Procedure
            return false;
        }
        case kHAPPDUOpcode_CharacteristicConfiguration: {
            return true;
        }
        case kHAPPDUOpcode_ProtocolConfiguration: {
            return true;
        }
        case kHAPPDUOpcode_AccessorySignatureRead: {
            return true;
        }
        case kHAPPDUOpcode_NotificationConfigurationRead: {
            return true;
        }
        case kHAPPDUOpcode_NotificationRegister: {
            return true;
        }
        case kHAPPDUOpcode_NotificationDeregister: {
            return true;
        }
        case kHAPPDUOpcode_Token: {
            return true;
        }
        case kHAPPDUOpcode_TokenUpdate: {
            return true;
        }
        case kHAPPDUOpcode_Info: {
            return true;
        }
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
bool HAPPDUOpcodeIsSupportedOnTransientSessions(HAPPDUOpcode opcode) {
    HAPPrecondition(HAPPDUOpcodeIsValid(opcode));

    // The base specification does not define what opcodes are allowed for transient sessions.
    // Therefore, we refer to the Cedar draft specification.
    //
    // See HomeKit Accessory Protocol Specification R17
    // Section 5.7.4 M4: Accessory -> iOS Device - `SRP Verify Response'
    switch (opcode) {
        case kHAPPDUOpcode_CharacteristicSignatureRead: {
            // Required to verify authenticity of GATT database for HAP over Bluetooth LE.
            return true;
        }
        case kHAPPDUOpcode_CharacteristicWrite: {
            return false;
        }
        case kHAPPDUOpcode_CharacteristicRead: {
            return false;
        }
        case kHAPPDUOpcode_CharacteristicTimedWrite: {
            return false;
        }
        case kHAPPDUOpcode_CharacteristicExecuteWrite: {
            return false;
        }
        case kHAPPDUOpcode_ServiceSignatureRead: {
            // Required to verify authenticity of GATT database for HAP over Bluetooth LE.
            return true;
        }
        case kHAPPDUOpcode_CharacteristicConfiguration: {
            return false;
        }
        case kHAPPDUOpcode_ProtocolConfiguration: {
            return false;
        }
        case kHAPPDUOpcode_AccessorySignatureRead: {
            // This operation returns the same information that is available via the
            // Characteristic Signature Read and Service Signature Read procedures,
            // but in a more efficiently packed format.
            // It makes sense to inherit the same security properties as for the other procedures.
            return true;
        }
        case kHAPPDUOpcode_NotificationConfigurationRead: {
            return false;
        }
        case kHAPPDUOpcode_NotificationRegister: {
            return false;
        }
        case kHAPPDUOpcode_NotificationDeregister: {
            return false;
        }
        case kHAPPDUOpcode_Token: {
            // Required for Software Authentication.
            return true;
        }
        case kHAPPDUOpcode_TokenUpdate: {
            // Required for Software Authentication.
            return true;
        }
        case kHAPPDUOpcode_Info: {
            // Required for Software Authentication.
            return true;
        }
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
uint64_t HAPPDUGetExpectedIIDForOperation(
        HAPPDUOpcode opcode,
        const HAPCharacteristic* characteristic_,
        const HAPService* service,
        const HAPAccessory* accessory,
        HAPTransportType transportType) {
    HAPPrecondition(HAPPDUOpcodeIsValid(opcode));
    HAPPrecondition(characteristic_);
    const HAPBaseCharacteristic* characteristic = characteristic_;
    HAPPrecondition(service);
    HAPPrecondition(accessory);
    HAPPrecondition(HAPTransportTypeIsValid(transportType));

    switch (HAPPDUOpcodeGetOperationType(opcode)) {
        case kHAPPDUOperationType_Characteristic: {
            return characteristic->iid;
        }
        case kHAPPDUOperationType_Service: {
            return service->iid;
        }
        case kHAPPDUOperationType_Accessory: {
            return transportType == kHAPTransportType_BLE ? characteristic->iid : 0;
        }
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
bool HAPPDUStatusIsValid(uint8_t value) {
    switch (value) {
        case kHAPPDUStatus_Success:
        case kHAPPDUStatus_UnsupportedPDU:
        case kHAPPDUStatus_MaxProcedures:
        case kHAPPDUStatus_InsufficientAuthorization:
        case kHAPPDUStatus_InvalidInstanceID:
        case kHAPPDUStatus_InsufficientAuthentication:
        case kHAPPDUStatus_InvalidRequest: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
const char* HAPPDUStatusGetDescription(HAPPDUStatus status) {
    HAPPrecondition(HAPPDUStatusIsValid(status));

    switch (status) {
        case kHAPPDUStatus_Success:
            return "Success";
        case kHAPPDUStatus_UnsupportedPDU:
            return "Unsupported-PDU";
        case kHAPPDUStatus_MaxProcedures:
            return "Max-Procedures";
        case kHAPPDUStatus_InsufficientAuthorization:
            return "Insufficient Authorization";
        case kHAPPDUStatus_InvalidInstanceID:
            return "Invalid Instance ID";
        case kHAPPDUStatus_InsufficientAuthentication:
            return "Insufficient Authentication";
        case kHAPPDUStatus_InvalidRequest:
            return "Invalid Request";
        default:
            HAPFatalError();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HAP_RESULT_USE_CHECK
HAPError HAPPDUParseRequestWithoutFragmentation(
        void* pduBytes_,
        size_t numPDUBytes,
        uint8_t* opcode,
        uint8_t* tid,
        uint16_t* iid,
        void* _Nullable* _Nonnull bodyBytes,
        uint16_t* numBodyBytes,
        uint16_t* numHeaderBytes) {
    HAPPrecondition(pduBytes_);
    uint8_t* pduBytes = pduBytes_;
    HAPPrecondition(opcode);
    HAPPrecondition(tid);
    HAPPrecondition(iid);
    HAPPrecondition(bodyBytes);
    HAPPrecondition(numBodyBytes);

    if (numPDUBytes < 5) {
        HAPLogBuffer(&logObject, pduBytes_, numPDUBytes, "Invalid request PDU (%s).", "too short");
        return kHAPError_InvalidData;
    }

    // Control field.
    uint8_t expectedControlField = (uint8_t)(
            (uint8_t)(0U << 7U) |                                             // First Fragment (or no fragmentation).
            (uint8_t)(0U << 4U) |                                             // 16 bit IIDs (or IID = 0).
            (uint8_t)(0U << 3U) | (uint8_t)(0U << 2U) | (uint8_t)(0U << 1U) | // Request.
            (uint8_t)(0U << 0U));                                             // 1 Byte Control Field.
    if (pduBytes[0] != expectedControlField) {
        HAPLogBuffer(&logObject, pduBytes_, numPDUBytes, "Invalid request PDU (%s).", "control field");
        return kHAPError_InvalidData;
    }

    // Header.
    *opcode = pduBytes[1];
    *tid = pduBytes[2];
    *iid = HAPReadLittleUInt16(&pduBytes[3]);

    if (!HAPPDUOpcodeIsValid(*opcode)) {
        HAPLogError(&logObject, "Invalid PDU opcode (%d)", *opcode);
        return kHAPError_InvalidState;
    }

    // Body.  Do we have a 5 byte Header or a 7 byte header?
    if (numPDUBytes == 5) {
        *numBodyBytes = 0;
        *bodyBytes = NULL;
        *numHeaderBytes = 5;
    } else if (numPDUBytes >= 7) {
        *numBodyBytes = HAPReadLittleUInt16(&pduBytes[5]);
        *bodyBytes = *numBodyBytes > 0 ? &pduBytes[7] : NULL;
        *numHeaderBytes = 7;
    } else {
        HAPLogBuffer(&logObject, pduBytes_, numPDUBytes, "Invalid request PDU (%s).", "body length");
        return kHAPError_InvalidData;
    }

    return kHAPError_None;
}

HAPError HAPPDUVerifyMessage(void* requestBytes, size_t numRequestBytes) {
    size_t totalBytesConsumed = 0;
    size_t totalPduSize = 0;
    size_t bytesRemaining = 0;
    bool isFirstPdu = true;

    HAPError err = kHAPError_None;

    while (totalBytesConsumed < numRequestBytes && err == kHAPError_None) {
        // Parse PDU.
        uint8_t opcode_;
        uint8_t tid;
        uint16_t iid;
        void* _Nullable requestBodyBytes;
        uint16_t numRequestBodyBytes;
        uint16_t numHeaderBytes;

        bytesRemaining = numRequestBytes - totalBytesConsumed;

        // Make sure there are enough bytes remaining to parse a header
        if (bytesRemaining < PDU_HEADER_SIZE_NO_BODY) {
            HAPLogError(&logObject, "Last PDU has insufficient space for header.");
            return kHAPError_InvalidData;
        } else if (bytesRemaining > PDU_HEADER_SIZE_NO_BODY && bytesRemaining < PDU_HEADER_SIZE_WITH_BODY) {
            HAPLogError(&logObject, "Last PDU has invalid length.");
            return kHAPError_InvalidData;
        }

        err = HAPPDUParseRequestWithoutFragmentation(
                (void*) ((uint8_t*) requestBytes + totalBytesConsumed),
                numRequestBytes - totalBytesConsumed,
                &opcode_,
                &tid,
                &iid,
                &requestBodyBytes,
                &numRequestBodyBytes,
                &numHeaderBytes);

        if (err) {
            HAPAssert(err == kHAPError_InvalidState || err == kHAPError_InvalidData);
            HAPLogError(&logObject, "Invalid PDU failed to parse during preprocessing: %d.", err);

            if (isFirstPdu && err == kHAPError_InvalidState) {
                // If opcode is bad, and we are on the first pdu, indicate bad opcode. Otherwise, we don't know
                // whether opcode is unsupported or pdu is malformed, so fall through to the next error case.
                return kHAPError_InvalidState;
            }

            // Malformed pdu, or uncertain opcode state
            return kHAPError_InvalidData;
        }

        totalPduSize = numHeaderBytes + numRequestBodyBytes;
        // Make sure body length is either present, or header size == total response size
        if (numHeaderBytes == PDU_HEADER_SIZE_NO_BODY) { // This header does not contain a body
            if (totalBytesConsumed != 0 ||
                numRequestBytes != PDU_HEADER_SIZE_NO_BODY) { // If we are not on the first pdu, or if this pdu is
                                                              // larger than the header
                HAPLogError(&logObject, "Body length not found: Body length is required for multiple PDU messages.");
                return kHAPError_InvalidData;
            }
        } else { // This header contains a body
            if (totalPduSize + totalBytesConsumed > numRequestBytes) {
                HAPLogError(&logObject, "Message overflow: Reported body length too large.");
                return kHAPError_InvalidData;
            }
        }
        totalBytesConsumed += totalPduSize;
        isFirstPdu = false;
    }

    // This condition should be impossible, but kept for caution.
    if (totalBytesConsumed != numRequestBytes) {
        HAPLogError(&logObject, "Message underflow: Reported pdu lengths too short.");
        return kHAPError_InvalidData;
    }

    return kHAPError_None;
}
