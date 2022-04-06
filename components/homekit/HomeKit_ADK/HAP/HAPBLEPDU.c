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

#include "HAPBLEPDU.h"

#include "HAPLogSubsystem.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "BLEPDU" };

/**
 * Checks whether a value represents a valid PDU type.
 *
 * @param      value                Value to check.
 *
 * @return true                     If the value is valid.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
static bool HAPBLEPDUIsValidType(uint8_t value) {
    switch (value) {
        case kHAPBLEPDUType_Request:
        case kHAPBLEPDUType_Response: {
            return true;
        }
        default:
            return false;
    }
}

/**
 * Returns description of a PDU type.
 *
 * @param      type                 Value of which to get description.
 *
 * @return Description of the type.
 */
HAP_RESULT_USE_CHECK
static const char* HAPBLEPDUTypeDescription(HAPBLEPDUType type) {
    HAPPrecondition(HAPBLEPDUIsValidType(type));

    switch (type) {
        case kHAPBLEPDUType_Request:
            return "Request";
        case kHAPBLEPDUType_Response:
            return "Response";
        default:
            HAPFatalError();
    }
}

/**
 * Logs a HAP-BLE PDU.
 *
 * @param      pdu                  PDU to log.
 */
static void LogPDU(const HAPBLEPDU* pdu) {
    HAPPrecondition(pdu);
    HAPBLEPDUType type = pdu->controlField.type;
    HAPPrecondition(HAPBLEPDUIsValidType(type));

    switch (pdu->controlField.fragmentationStatus) {
        case kHAPBLEPDUFragmentationStatus_FirstFragment: {
            switch (type) {
                case kHAPBLEPDUType_Request: {
                    HAPPDUOpcode opcode = pdu->fixedParams.request.opcode;
                    HAPLogBufferDebug(
                            &logObject,
                            pdu->body.bytes,
                            pdu->body.numBytes,
                            "%s-%s (0x%02x):\n"
                            "    TID: 0x%02x\n"
                            "    IID: %u",
                            HAPPDUOpcodeIsValid(opcode) ? HAPPDUOpcodeGetDescription(opcode) : "Unknown",
                            HAPBLEPDUTypeDescription(type),
                            opcode,
                            pdu->fixedParams.request.tid,
                            pdu->fixedParams.request.iid);
                    break;
                }
                case kHAPBLEPDUType_Response: {
                    HAPPDUStatus status = pdu->fixedParams.response.status;
                    HAPLogBufferDebug(
                            &logObject,
                            pdu->body.bytes,
                            pdu->body.numBytes,
                            "%s:\n"
                            "    TID: 0x%02x\n"
                            "    Status: %s (0x%02x)",
                            HAPBLEPDUTypeDescription(type),
                            pdu->fixedParams.response.tid,
                            HAPPDUStatusIsValid(status) ? HAPPDUStatusGetDescription(status) : "Unknown",
                            pdu->fixedParams.response.status);
                    break;
                }
                default:
                    HAPFatalError();
            }
            break;
        }
        case kHAPBLEPDUFragmentationStatus_Continuation: {
            HAPLogBufferDebug(
                    &logObject,
                    pdu->body.bytes,
                    pdu->body.numBytes,
                    "%s (Continuation):\n"
                    "    TID: 0x%02x",
                    HAPBLEPDUTypeDescription(type),
                    pdu->fixedParams.response.tid);
            break;
        }
        default:
            HAPFatalError();
    }
}

/**
 * Attempts to deserialize the Control Field into a PDU structure.
 *
 * @param[out] pdu                  PDU.
 * @param      controlField         Serialized Control Field.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the controller sent a malformed request.
 */
HAP_RESULT_USE_CHECK
static HAPError DeserializeControlField(HAPBLEPDU* pdu, const uint8_t* controlField) {
    HAPPrecondition(pdu);
    HAPPrecondition(controlField);

    // Check that reserved bits are 0.
    if (controlField[0] & (1U << 6U | 1U << 5U | 1U << 4U)) {
        HAPLog(&logObject, "Invalid reserved bits in Control Field 0x%02x.", controlField[0]);
        return kHAPError_InvalidData;
    }

    // Fragmentation status.
    switch (controlField[0] & 1U << 7U) {
        case 0U << 7U: {
            pdu->controlField.fragmentationStatus = kHAPBLEPDUFragmentationStatus_FirstFragment;
            break;
        }
        case 1U << 7U: {
            pdu->controlField.fragmentationStatus = kHAPBLEPDUFragmentationStatus_Continuation;
            break;
        }
        default: {
            HAPLog(&logObject, "Invalid fragmentation status in Control Field 0x%02x.", controlField[0]);
            return kHAPError_InvalidData;
        };
    }

    // PDU Type.
    switch (controlField[0] & (1U << 3U | 1U << 2U | 1U << 1U)) {
        case 0U << 3U | 0U << 2U | 0U << 1U: {
            pdu->controlField.type = kHAPBLEPDUType_Request;
            break;
        }
        case 0U << 3U | 0U << 2U | 1U << 1U: {
            pdu->controlField.type = kHAPBLEPDUType_Response;
            break;
        }
        default: {
            HAPLog(&logObject, "Invalid PDU Type in Control Field 0x%02x.", controlField[0]);
            return kHAPError_InvalidData;
        }
    }

    // Length.
    switch (controlField[0] & 1U << 0U) {
        case 0U << 0U: {
            pdu->controlField.length = kHAPBLEPDUControlFieldLength_1Byte;
            break;
        }
        default: {
            HAPLog(&logObject, "Invalid length in Control Field 0x%02x.", controlField[0]);
            return kHAPError_InvalidData;
        }
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPBLEPDUDeserialize(HAPBLEPDU* pdu, const void* bytes, size_t numBytes) {
    HAPPrecondition(pdu);
    HAPPrecondition(bytes);

    HAPError err;

    const uint8_t* b = bytes;
    size_t remainingBytes = numBytes;

    // PDU Header - Control Field.
    if (remainingBytes < 1) {
        HAPLog(&logObject, "PDU not long enough to contain Control Field.");
        return kHAPError_InvalidData;
    }
    err = DeserializeControlField(pdu, b);
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }
    if (pdu->controlField.fragmentationStatus != kHAPBLEPDUFragmentationStatus_FirstFragment) {
        HAPLog(&logObject, "Unexpected PDU fragmentation status (expected: First fragment (or no fragmentation)).");
        return kHAPError_InvalidData;
    }
    b += 1;
    remainingBytes -= 1;

    // PDU Fixed Params.
    switch (pdu->controlField.type) {
        case kHAPBLEPDUType_Request: {
            if (remainingBytes < 4) {
                HAPLog(&logObject, "Request PDU not long enough to contain Fixed Params.");
                return kHAPError_InvalidData;
            }
            pdu->fixedParams.request.opcode = (HAPPDUOpcode) b[0];
            pdu->fixedParams.request.tid = b[1];
            pdu->fixedParams.request.iid = HAPReadLittleUInt16(&b[2]);
            b += 4;
            remainingBytes -= 4;
            break;
        }
        case kHAPBLEPDUType_Response: {
            if (remainingBytes < 2) {
                HAPLog(&logObject, "Response PDU not long enough to contain Fixed Params.");
                return kHAPError_InvalidData;
            }
            pdu->fixedParams.response.tid = b[0];
            pdu->fixedParams.response.status = (HAPPDUStatus) b[1];
            b += 2;
            remainingBytes -= 2;
            break;
        }
        default:
            HAPFatalError();
    }

    // PDU Body (Optional).
    if (!remainingBytes) {
        pdu->body.totalBodyBytes = 0;
        pdu->body.bytes = NULL;
        pdu->body.numBytes = 0;
    } else {
        if (remainingBytes < 2) {
            HAPLog(&logObject, "PDU not long enough to contain body length.");
            return kHAPError_InvalidData;
        }
        pdu->body.totalBodyBytes = HAPReadLittleUInt16(b);
        b += 2;
        remainingBytes -= 2;

        if (remainingBytes < pdu->body.totalBodyBytes) {
            // First fragment.
            pdu->body.numBytes = (uint16_t) remainingBytes;
        } else {
            // Complete body available.
            pdu->body.numBytes = pdu->body.totalBodyBytes;
        }
        pdu->body.bytes = b;
        b += pdu->body.numBytes;
        remainingBytes -= pdu->body.numBytes;
    }

    // All data read.
    if (remainingBytes) {
        HAPLog(&logObject, "Excess data after PDU.");
        return kHAPError_InvalidData;
    }
    (void) b;

    LogPDU(pdu);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPBLEPDUDeserializeContinuation(
        HAPBLEPDU* pdu,
        const void* bytes,
        size_t numBytes,
        HAPBLEPDUType typeOfFirstFragment,
        size_t totalBodyBytes,
        size_t totalBodyBytesSoFar) {
    HAPPrecondition(pdu);
    HAPPrecondition(bytes);
    HAPPrecondition(totalBodyBytes >= totalBodyBytesSoFar);
    HAPPrecondition(totalBodyBytes <= UINT16_MAX);

    HAPError err;

    const uint8_t* b = bytes;
    size_t remainingBytes = numBytes;

    // PDU Header - Control Field.
    if (remainingBytes < 1) {
        HAPLog(&logObject, "PDU not long enough to contain Control Field.");
        return kHAPError_InvalidData;
    }
    err = DeserializeControlField(pdu, b);
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }
    if (pdu->controlField.fragmentationStatus != kHAPBLEPDUFragmentationStatus_Continuation) {
        HAPLog(&logObject, "Unexpected PDU fragmentation status (expected: Continuation of fragmented PDU).");
        return kHAPError_InvalidData;
    }
    if (pdu->controlField.type != typeOfFirstFragment) {
        HAPLog(&logObject,
               "Unexpected PDU type (Continuation type: 0x%02x, First Fragment type: 0x%02x).",
               pdu->controlField.type,
               typeOfFirstFragment);
        return kHAPError_InvalidData;
    }
    b += 1;
    remainingBytes -= 1;

    // PDU Fixed Params.
    if (remainingBytes < 1) {
        HAPLog(&logObject, "Continuation PDU not long enough to contain Fixed Params.");
        return kHAPError_InvalidData;
    }
    pdu->fixedParams.continuation.tid = b[0];
    b += 1;
    remainingBytes -= 1;

    // PDU Body (Optional).
    pdu->body.totalBodyBytes = (uint16_t) totalBodyBytes;
    if (!remainingBytes) {
        pdu->body.bytes = NULL;
        pdu->body.numBytes = 0;
    } else {
        if (remainingBytes < totalBodyBytes - totalBodyBytesSoFar) {
            // Next fragment.
            pdu->body.numBytes = (uint16_t) remainingBytes;
        } else {
            // Complete body available.
            pdu->body.numBytes = (uint16_t)(totalBodyBytes - totalBodyBytesSoFar);
        }
        pdu->body.bytes = b;
        b += pdu->body.numBytes;
        remainingBytes -= pdu->body.numBytes;
    }

    // All data read.
    if (remainingBytes) {
        HAPLog(&logObject, "Excess data after PDU.");
        return kHAPError_InvalidData;
    }
    (void) b;

    LogPDU(pdu);
    return kHAPError_None;
}

/**
 * Checks whether a value represents a valid fragmentation status.
 *
 * @param      value                Value to check.
 *
 * @return true                     If the value is valid.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
static bool HAPBLEPDUIsValidFragmentStatus(uint8_t value) {
    switch (value) {
        case kHAPBLEPDUFragmentationStatus_FirstFragment:
        case kHAPBLEPDUFragmentationStatus_Continuation: {
            return true;
        }
        default:
            return false;
    }
}

/**
 * Checks whether a value represents a valid Control Field length.
 *
 * @param      value                Value to check.
 *
 * @return true                     If the value is valid.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
static bool HAPBLEPDUIsValidControlFieldLength(uint8_t value) {
    switch (value) { // NOLINT(hicpp-multiway-paths-covered)
        case kHAPBLEPDUControlFieldLength_1Byte: {
            return true;
        }
        default:
            return false;
    }
}

/**
 * Checks whether the Control Field of a PDU structure is valid.
 *
 * @param      pdu                  PDU with Control Field to check.
 *
 * @return true                     If the Control Field is valid.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
static bool HAPBLEPDUHasValidControlField(const HAPBLEPDU* pdu) {
    HAPPrecondition(pdu);

    return HAPBLEPDUIsValidFragmentStatus(pdu->controlField.fragmentationStatus) &&
           HAPBLEPDUIsValidType(pdu->controlField.type) && HAPBLEPDUIsValidControlFieldLength(pdu->controlField.length);
}

/**
 * Serializes the Control Field of a PDU structure.
 *
 * @param      pdu                  PDU with Control Field to serialize.
 * @param[out] controlFieldByte     Serialized Control Field.
 */
static void SerializeControlField(const HAPBLEPDU* pdu, uint8_t* controlFieldByte) {
    HAPPrecondition(pdu);
    HAPPrecondition(HAPBLEPDUHasValidControlField(pdu));
    HAPPrecondition(controlFieldByte);

    // Clear all bits.
    controlFieldByte[0] = 0;

    // Fragmentation status.
    switch (pdu->controlField.fragmentationStatus) {
        case kHAPBLEPDUFragmentationStatus_FirstFragment: {
            controlFieldByte[0] |= 0U << 7U;
            break;
        }
        case kHAPBLEPDUFragmentationStatus_Continuation: {
            controlFieldByte[0] |= 1U << 7U;
            break;
        }
        default:
            HAPFatalError();
    }

    // PDU Type.
    switch (pdu->controlField.type) {
        case kHAPBLEPDUType_Request: {
            controlFieldByte[0] |= 0U << 3U | 0U << 2U | 0U << 1U;
            break;
        }
        case kHAPBLEPDUType_Response: {
            controlFieldByte[0] |= 0U << 3U | 0U << 2U | 1U << 1U;
            break;
        }
        default:
            HAPFatalError();
    }

    // Length.
    switch (pdu->controlField.length) {
        case kHAPBLEPDUControlFieldLength_1Byte: {
            controlFieldByte[0] |= 0U << 0U;
            break;
        }
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPBLEPDUSerialize(const HAPBLEPDU* pdu, void* bytes, size_t maxBytes, size_t* numBytes) {
    HAPPrecondition(pdu);
    HAPPrecondition(HAPBLEPDUHasValidControlField(pdu));
    HAPPrecondition(pdu->body.numBytes <= pdu->body.totalBodyBytes);
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes);

    LogPDU(pdu);

    uint8_t* b = bytes;
    size_t remainingBytes = maxBytes;

    // PDU Header - Control Field.
    if (remainingBytes < 1) {
        HAPLog(&logObject, "Not enough capacity to serialize Control Field.");
        return kHAPError_OutOfResources;
    }
    SerializeControlField(pdu, b);
    b += 1;
    remainingBytes -= 1;

    // PDU Header - PDU Fixed Params.
    switch (pdu->controlField.fragmentationStatus) {
        case kHAPBLEPDUFragmentationStatus_FirstFragment: {
            switch (pdu->controlField.type) {
                case kHAPBLEPDUType_Request: {
                    if (remainingBytes < 4) {
                        HAPLog(&logObject, "Not enough capacity to serialize Request PDU Fixed Params.");
                        return kHAPError_OutOfResources;
                    }
                    b[0] = pdu->fixedParams.request.opcode;
                    b[1] = pdu->fixedParams.request.tid;
                    HAPWriteLittleUInt16(&b[2], pdu->fixedParams.request.iid);
                    b += 4;
                    remainingBytes -= 4;
                    break;
                }
                case kHAPBLEPDUType_Response: {
                    if (remainingBytes < 2) {
                        HAPLog(&logObject, "Not enough capacity to serialize Response PDU Fixed Params.");
                        return kHAPError_OutOfResources;
                    }
                    b[0] = pdu->fixedParams.response.tid;
                    b[1] = pdu->fixedParams.response.status;
                    b += 2;
                    remainingBytes -= 2;
                    break;
                }
                default:
                    HAPFatalError();
            }

            // PDU Body (Optional).
            if (pdu->body.bytes) {
                if (remainingBytes < 2) {
                    HAPLog(&logObject, "Not enough capacity to serialize PDU Body length.");
                    return kHAPError_OutOfResources;
                }
                HAPWriteLittleUInt16(&b[0], pdu->body.totalBodyBytes);
                b += 2;
                remainingBytes -= 2;

                if (remainingBytes < pdu->body.numBytes) {
                    HAPLog(&logObject, "Not enough capacity to serialize PDU Body.");
                    return kHAPError_OutOfResources;
                }
                HAPRawBufferCopyBytes(b, HAPNonnullVoid(pdu->body.bytes), pdu->body.numBytes);
                b += pdu->body.numBytes;
                remainingBytes -= pdu->body.numBytes;
            }
            goto done;
        };
        case kHAPBLEPDUFragmentationStatus_Continuation: {
            if (remainingBytes < 1) {
                HAPLog(&logObject, "Not enough capacity to serialize Continuation PDU Fixed Params.");
                return kHAPError_OutOfResources;
            }
            b[0] = pdu->fixedParams.continuation.tid;
            b += 1;
            remainingBytes -= 1;

            if (remainingBytes < pdu->body.numBytes) {
                HAPLog(&logObject, "Not enough capacity to serialize PDU Body.");
                return kHAPError_OutOfResources;
            }
            if (!pdu->body.numBytes) {
                HAPLog(&logObject, "Received empty continuation fragment.");
            } else {
                HAPAssert(pdu->body.bytes);
                HAPRawBufferCopyBytes(b, HAPNonnullVoid(pdu->body.bytes), pdu->body.numBytes);
                b += pdu->body.numBytes;
                remainingBytes -= pdu->body.numBytes;
            }
            goto done;
        };
    }
    HAPAssertionFailure();

done:
    // All data written.
    *numBytes = maxBytes - remainingBytes;
    (void) b;
    return kHAPError_None;
}

#endif
