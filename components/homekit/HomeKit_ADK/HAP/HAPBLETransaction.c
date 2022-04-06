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

#include "HAPBLETransaction.h"

#include "HAPBLEPDU.h"
#include "HAPLogSubsystem.h"
#include "HAPTLV+Internal.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "BLETransaction" };

void HAPBLETransactionCreate(HAPBLETransaction* bleTransaction, void* _Nullable bodyBytes, size_t numBodyBytes)
        HAP_DIAGNOSE_ERROR(!bodyBytes && numBodyBytes, "empty buffer cannot have a length") {
    HAPPrecondition(bleTransaction);
    HAPPrecondition(!numBodyBytes || bodyBytes);

    HAPRawBufferZero(bleTransaction, sizeof *bleTransaction);
    bleTransaction->_.request.bodyBytes = bodyBytes;
    bleTransaction->_.request.maxBodyBytes = numBodyBytes;
}

/**
 * Appends a body fragment to the combined body in a transaction.
 * If the transaction buffer is not large enough, input fragment is discarded.
 *
 * @param      bleTransaction       Transaction.
 * @param      fragmentBytes        Buffer to append.
 * @param      numFragmentBytes     Length of buffer.
 */
static void TryAppendBodyFragment(
        HAPBLETransaction* bleTransaction,
        const void* _Nullable const fragmentBytes,
        size_t numFragmentBytes) {
    HAPPrecondition(bleTransaction);
    HAPPrecondition(!numFragmentBytes || fragmentBytes);

    if (!fragmentBytes) {
        return;
    }

    if (bleTransaction->_.request.totalBodyBytes > bleTransaction->_.request.maxBodyBytes) {
        HAPLogInfo(
                &logObject,
                "Discarding body fragment as transaction buffer is not large enough (%lu/%lu).",
                (unsigned long) bleTransaction->_.request.totalBodyBytes,
                (unsigned long) bleTransaction->_.request.maxBodyBytes);
    } else {
        HAPAssert(bleTransaction->_.request.bodyBytes);
        uint8_t* bodyBytes = bleTransaction->_.request.bodyBytes;
        HAPAssert(numFragmentBytes <= bleTransaction->_.request.maxBodyBytes - bleTransaction->_.request.bodyOffset);
        HAPRawBufferCopyBytes(&bodyBytes[bleTransaction->_.request.bodyOffset], fragmentBytes, numFragmentBytes);
    }
    bleTransaction->_.request.bodyOffset += numFragmentBytes;
}

HAP_RESULT_USE_CHECK
HAPError HAPBLETransactionHandleWrite(HAPBLETransaction* bleTransaction, const void* bytes, size_t numBytes) {
    HAPPrecondition(bleTransaction);
    HAPPrecondition(bytes);

    HAPError err;

    switch (bleTransaction->state) {
        case kHAPBLETransactionState_WaitingForInitialWrite: {
            bleTransaction->state = kHAPBLETransactionState_ReadingRequest;

            // Read packet. It has to be a HAP Request.
            // See HomeKit Accessory Protocol Specification R17
            // Section 7.3.5 HAP Procedures
            HAPBLEPDU pdu;
            err = HAPBLEPDUDeserialize(&pdu, bytes, numBytes);
            if (err) {
                HAPAssert(err == kHAPError_InvalidData);
                return err;
            }
            if (pdu.controlField.type != kHAPBLEPDUType_Request) {
                HAPLog(&logObject, "Expected HAP-BLE request but got PDU with different type.");
                return kHAPError_InvalidData;
            }

            // Cache request header. Potential continuations do not include it.
            bleTransaction->_.request.opcode = pdu.fixedParams.request.opcode;
            bleTransaction->_.request.tid = pdu.fixedParams.request.tid;
            bleTransaction->_.request.iid = pdu.fixedParams.request.iid;
            bleTransaction->_.request.totalBodyBytes = pdu.body.totalBodyBytes;
            bleTransaction->_.request.bodyOffset = 0;

            // Append body.
            TryAppendBodyFragment(bleTransaction, pdu.body.bytes, pdu.body.numBytes);
        }
            return kHAPError_None;
        case kHAPBLETransactionState_ReadingRequest: {
            // Read continuation. It has the same tid as the previous fragments.
            // See HomeKit Accessory Protocol Specification R17
            // Section 7.3.3.5 HAP PDU Fragmentation Scheme
            // See HomeKit Accessory Protocol Specification R17
            // Section 7.3.5.6 HAP Fragmented Writes
            HAPBLEPDU pdu;
            err = HAPBLEPDUDeserializeContinuation(
                    &pdu,
                    bytes,
                    numBytes,
                    kHAPBLEPDUType_Request,
                    bleTransaction->_.request.totalBodyBytes,
                    bleTransaction->_.request.bodyOffset);
            if (err) {
                HAPAssert(err == kHAPError_InvalidData);
                return err;
            }
            if (pdu.fixedParams.continuation.tid != bleTransaction->_.request.tid) {
                HAPLog(&logObject, "Continuation fragment has different TID as the previous fragments.");
                return kHAPError_InvalidData;
            }

            // Append body.
            TryAppendBodyFragment(bleTransaction, pdu.body.bytes, pdu.body.numBytes);
        }
            return kHAPError_None;
        case kHAPBLETransactionState_HandlingRequest:
        case kHAPBLETransactionState_WaitingForInitialRead: {
            // Full request received, response has been set.
            // However, there may still be writes with empty fragments before the first read request.
            HAPBLEPDU pdu;
            err = HAPBLEPDUDeserializeContinuation(
                    &pdu,
                    bytes,
                    numBytes,
                    kHAPBLEPDUType_Request,
                    /* totalBodyBytes: */ 0,
                    /* totalBodyBytesSoFar: */ 0);
            if (err) {
                HAPAssert(err == kHAPError_InvalidData);
                return err;
            }
            if (pdu.fixedParams.continuation.tid != bleTransaction->_.response.tid) {
                HAPLog(&logObject, "Continuation fragment has different TID as the previous fragments.");
                return kHAPError_InvalidData;
            }
        }
            return kHAPError_None;
        case kHAPBLETransactionState_WritingResponse: {
            HAPLog(&logObject, "Received write while writing response.");
        }
            return kHAPError_InvalidState;
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
bool HAPBLETransactionIsRequestAvailable(const HAPBLETransaction* bleTransaction) {
    HAPPrecondition(bleTransaction);

    return bleTransaction->state == kHAPBLETransactionState_ReadingRequest &&
           bleTransaction->_.request.bodyOffset == bleTransaction->_.request.totalBodyBytes;
}

HAP_RESULT_USE_CHECK
HAPError HAPBLETransactionGetRequest(HAPBLETransaction* bleTransaction, HAPBLETransactionRequest* request) {
    HAPPrecondition(bleTransaction);
    HAPPrecondition(HAPBLETransactionIsRequestAvailable(bleTransaction));
    HAPPrecondition(request);

    bleTransaction->state = kHAPBLETransactionState_HandlingRequest;

    if (bleTransaction->_.request.totalBodyBytes > bleTransaction->_.request.maxBodyBytes) {
        HAPLog(&logObject,
               "Transaction buffer was not large enough to hold request. (%lu/%lu).",
               (unsigned long) bleTransaction->_.request.totalBodyBytes,
               (unsigned long) bleTransaction->_.request.maxBodyBytes);
        return kHAPError_OutOfResources;
    }

    request->opcode = bleTransaction->_.request.opcode;
    request->iid = bleTransaction->_.request.iid;
    HAPTLVReaderCreateWithOptions(
            &request->bodyReader,
            &(const HAPTLVReaderOptions) { .bytes = bleTransaction->_.request.bodyBytes,
                                           .numBytes = bleTransaction->_.request.totalBodyBytes,
                                           .maxBytes = bleTransaction->_.request.maxBodyBytes });

    return kHAPError_None;
}

void HAPBLETransactionSetResponse(
        HAPBLETransaction* bleTransaction,
        HAPPDUStatus status,
        const HAPTLVWriter* _Nullable bodyWriter) {
    HAPPrecondition(bleTransaction);
    HAPPrecondition(bleTransaction->state == kHAPBLETransactionState_HandlingRequest);

    void* bytes = NULL;
    size_t numBytes = 0;
    if (bodyWriter) {
        HAPTLVWriterGetBuffer(HAPNonnull(bodyWriter), &bytes, &numBytes);
    }
    // Maximum HAP-BLE PDU Body length == UINT16_MAX.
    HAPPrecondition(numBytes <= UINT16_MAX);

    uint8_t tid = bleTransaction->_.request.tid;
    bleTransaction->state = kHAPBLETransactionState_WaitingForInitialRead;
    bleTransaction->_.response.tid = tid;
    bleTransaction->_.response.status = status;
    if (bodyWriter) {
        HAPTLVWriterGetBuffer(
                HAPNonnull(bodyWriter),
                &bleTransaction->_.response.bodyBytes,
                &bleTransaction->_.response.totalBodyBytes);
        bleTransaction->_.response.bodyOffset = 0;
    } else {
        bleTransaction->_.response.bodyBytes = NULL;
        bleTransaction->_.response.totalBodyBytes = 0;
        bleTransaction->_.response.bodyOffset = 0;
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPBLETransactionHandleRead(
        HAPBLETransaction* bleTransaction,
        void* bytes,
        size_t maxBytes,
        size_t* numBytes,
        bool* isFinalFragment) {
    HAPPrecondition(bleTransaction);
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes);
    HAPPrecondition(isFinalFragment);

    HAPError err;

    switch (bleTransaction->state) {
        case kHAPBLETransactionState_WaitingForInitialRead: {
            bleTransaction->state = kHAPBLETransactionState_WritingResponse;

            // Calculate header length.
            size_t numHeaderBytes = kHAPBLEPDU_NumResponseHeaderBytes;
            if (bleTransaction->_.response.totalBodyBytes) {
                numHeaderBytes += kHAPBLEPDU_NumBodyHeaderBytes;
            }
            if (maxBytes < numHeaderBytes) {
                HAPLog(&logObject, "Not enough capacity for Response PDU header.");
                return kHAPError_OutOfResources;
            }

            // Calculate body fragment length.
            size_t numFragmentBytes = bleTransaction->_.response.totalBodyBytes;
            HAPAssert(numFragmentBytes <= UINT16_MAX);
            if (maxBytes - numHeaderBytes < numFragmentBytes) {
                numFragmentBytes = maxBytes - numHeaderBytes;
            }

            // Serialize HAP-BLE PDU.
            HAPBLEPDU pdu;
            HAPRawBufferZero(&pdu, sizeof pdu);
            pdu.controlField.fragmentationStatus = kHAPBLEPDUFragmentationStatus_FirstFragment;
            pdu.controlField.type = kHAPBLEPDUType_Response;
            pdu.controlField.length = kHAPBLEPDUControlFieldLength_1Byte;
            pdu.fixedParams.response.tid = bleTransaction->_.response.tid;
            pdu.fixedParams.response.status = bleTransaction->_.response.status;
            pdu.body.totalBodyBytes = (uint16_t) bleTransaction->_.response.totalBodyBytes;
            pdu.body.bytes = bleTransaction->_.response.bodyBytes;
            pdu.body.numBytes = (uint16_t) numFragmentBytes;

            err = HAPBLEPDUSerialize(&pdu, bytes, maxBytes, numBytes);
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }

            // Advance buffer.
            bleTransaction->_.response.bodyOffset += numFragmentBytes;
            *isFinalFragment = bleTransaction->_.response.bodyOffset == bleTransaction->_.response.totalBodyBytes;
        }
            return kHAPError_None;
        case kHAPBLETransactionState_WritingResponse: {
            // Send next response fragment.
            // Calculate header length.
            size_t numHeaderBytes = kHAPBLEPDU_NumContinuationHeaderBytes;
            if (maxBytes < numHeaderBytes) {
                HAPLog(&logObject, "Not enough capacity for Continuation PDU header.");
                return kHAPError_OutOfResources;
            }

            // Calculate body fragment length.
            size_t numFragmentBytes = bleTransaction->_.response.totalBodyBytes - bleTransaction->_.response.bodyOffset;
            HAPAssert(numFragmentBytes <= UINT16_MAX);
            if (maxBytes - numHeaderBytes < numFragmentBytes) {
                numFragmentBytes = maxBytes - numHeaderBytes;
            }
            HAPAssert(bleTransaction->_.response.bodyBytes);

            // Serialize HAP-BLE PDU.
            HAPBLEPDU pdu;
            HAPRawBufferZero(&pdu, sizeof pdu);
            pdu.controlField.fragmentationStatus = kHAPBLEPDUFragmentationStatus_Continuation;
            pdu.controlField.type = kHAPBLEPDUType_Response;
            pdu.controlField.length = kHAPBLEPDUControlFieldLength_1Byte;
            pdu.fixedParams.continuation.tid = bleTransaction->_.response.tid;
            pdu.body.totalBodyBytes = (uint16_t) bleTransaction->_.response.totalBodyBytes;
            pdu.body.bytes = (uint8_t*) bleTransaction->_.response.bodyBytes + bleTransaction->_.response.bodyOffset;
            pdu.body.numBytes = (uint16_t) numFragmentBytes;

            err = HAPBLEPDUSerialize(&pdu, bytes, maxBytes, numBytes);
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }

            // Advance buffer.
            bleTransaction->_.response.bodyOffset += numFragmentBytes;
            *isFinalFragment = bleTransaction->_.response.bodyOffset == bleTransaction->_.response.totalBodyBytes;
        }
            return kHAPError_None;
        case kHAPBLETransactionState_WaitingForInitialWrite:
        case kHAPBLETransactionState_ReadingRequest:
        case kHAPBLETransactionState_HandlingRequest: {
        }
            return kHAPError_InvalidState;
        default:
            HAPFatalError();
    }
}

#endif
