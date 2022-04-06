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

#ifndef HAP_BLE_TRANSACTION_H
#define HAP_BLE_TRANSACTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPPDU.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)

/**@cond */
/**
 * Transaction state.
 */
HAP_ENUM_BEGIN(uint8_t, HAPBLETransactionState) { /**
                                                   * Waiting for initial write.
                                                   */
                                                  kHAPBLETransactionState_WaitingForInitialWrite,

                                                  /**
                                                   * Reading request.
                                                   */
                                                  kHAPBLETransactionState_ReadingRequest,

                                                  /**
                                                   * Request has been retrieved. Waiting for response to be set.
                                                   */
                                                  kHAPBLETransactionState_HandlingRequest,

                                                  /**
                                                   * Waiting for initial read.
                                                   */
                                                  kHAPBLETransactionState_WaitingForInitialRead,

                                                  /**
                                                   * Writing response.
                                                   */
                                                  kHAPBLETransactionState_WritingResponse
} HAP_ENUM_END(uint8_t, HAPBLETransactionState);

/**
 * Transaction.
 */
typedef struct {
    HAPBLETransactionState state; /**< Transaction State. */
    union {
        struct {
            HAPPDUOpcode opcode; /**< HAP Opcode. */
            uint8_t tid;         /**< TID. Transaction Identifier. */
            uint16_t iid;        /**< CID. Characteristic / service instance ID. */

            void* _Nullable bodyBytes; /**< Combined body. */
            size_t maxBodyBytes;       /**< Combined body capacity. */
            size_t totalBodyBytes;     /**< Combined body length. */
            size_t bodyOffset;         /**< Combined body offset. */
        } request;
        struct {
            uint8_t tid;         /**< TID. Transaction Identifier. */
            HAPPDUStatus status; /**< Status. */

            void* _Nullable bodyBytes; /**< Combined body. */
            size_t totalBodyBytes;     /**< Combined body length. */
            size_t bodyOffset;         /**< Combined body offset. */
        } response;
    } _;
} HAPBLETransaction;
/**@endcond */

/**
 * Initializes a transaction with a body buffer.
 *
 * - Only one transaction can be processed. Re-initialization is required to process the next transaction.
 *
 * @param[out] bleTransaction       Transaction.
 * @param      bodyBytes            Buffer that may be used to store body data.
 * @param      numBodyBytes         Capacity of body buffer.
 */
void HAPBLETransactionCreate(HAPBLETransaction* bleTransaction, void* _Nullable bodyBytes, size_t numBodyBytes)
        HAP_DIAGNOSE_ERROR(!bodyBytes && numBodyBytes, "empty buffer cannot have a length");

/**
 * Processes incoming data. Once all data fragments have been received,
 * #HAPBLETransactionIsRequestAvailable returns true, and
 * #HAPBLETransactionGetRequest may be used to get the request data.
 *
 * @param      bleTransaction       Transaction.
 * @param      bytes                Buffer with data to process.
 * @param      numBytes             Length of buffer.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If the request cannot be processed in the current state.
 * @return kHAPError_InvalidData    If the controller sent a malformed or unexpected request.
 */
HAP_RESULT_USE_CHECK
HAPError HAPBLETransactionHandleWrite(HAPBLETransaction* bleTransaction, const void* bytes, size_t numBytes);

/**
 * Returns whether a complete request has been received and is ready to be fetched with #HAPBLETransactionGetRequest.
 *
 * @param      bleTransaction       Transaction.
 *
 * @return true  If a complete request has been received.
 * @return false Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPBLETransactionIsRequestAvailable(const HAPBLETransaction* bleTransaction);

/**
 * Request.
 */
typedef struct {
    /**
     * HAP Opcode.
     */
    HAPPDUOpcode opcode;

    /**
     * CID. Characteristic / service instance ID.
     *
     * - For Bluetooth LE, instance IDs cannot exceed UINT16_MAX.
     */
    uint16_t iid;

    /**
     * Reader that may be used to query the request's body.
     *
     * If the body did not fit into the buffer supplied to #HAPBLETransactionCreate,
     * the body is skipped and the reader returns no data.
     */
    HAPTLVReader bodyReader;
} HAPBLETransactionRequest;

/**
 * After #HAPBLETransactionHandleWrite indicates that a complete request has been received,
 * this function may be used to retrieve the most recent request. The function may only be called once per request.
 *
 * @param      bleTransaction       Transaction.
 * @param[out] request              Request.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the request did not fit into the receive buffer.
 */
HAP_RESULT_USE_CHECK
HAPError HAPBLETransactionGetRequest(HAPBLETransaction* bleTransaction, HAPBLETransactionRequest* request);

/**
 * Sets the response for sending with future #HAPBLETransactionHandleRead commands.
 *
 * @param      bleTransaction       Transaction.
 * @param      status               Response Status.
 * @param      bodyWriter           Writer containing the serialized response body. Optional. Maximum UINT16_MAX length.
 */
void HAPBLETransactionSetResponse(
        HAPBLETransaction* bleTransaction,
        HAPPDUStatus status,
        const HAPTLVWriter* _Nullable bodyWriter);

/**
 * Fills a buffer with the next response fragment to be sent.
 *
 * @param      bleTransaction       Transaction.
 * @param[out] bytes                Buffer to put fragment data into.
 * @param      maxBytes             Capacity of buffer.
 * @param[out] numBytes             Length of fragment put into buffer.
 * @param[out] isFinalFragment      true If all data fragments have been produced; false Otherwise.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If the request cannot be processed in the current state.
 * @return kHAPError_OutOfResources If buffer not large enough.
 */
HAP_RESULT_USE_CHECK
HAPError HAPBLETransactionHandleRead(
        HAPBLETransaction* bleTransaction,
        void* bytes,
        size_t maxBytes,
        size_t* numBytes,
        bool* isFinalFragment);

#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
