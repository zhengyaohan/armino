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

#ifndef HAP_PROCEDURE_H
#define HAP_PROCEDURE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP+API.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)

/** Procedure state (stored in each HAPSession). */
typedef struct {
    /** Timed write state. */
    struct {
        HAPTime startTime; /**< Time when timed write was started. */
        uint16_t iid;      /**< Instance ID of accessed characteristic. 0 if no timed write pending. */
        uint8_t ttl;       /**< TTL byte (100ms) */
    } timedWrite;

    /** Information about the event notification that is being sent. */
    struct {
        uint16_t iid; /**< Instance ID of originating characteristic. 0 if no notification pending. */
    } notification;
} HAPPDUProcedureSessionState;

/**
 * Processes a non-fragmented HAP PDU request.
 *
 * @param      server               Accessory server.
 * @param      session              Session over which the PDU has been received.
 * @param      requestBytes         Buffer that contains the request PDU.
 * @param      numRequestBytes      Length of data in request buffer.
 * @param[out] responseBytes        Buffer to fill response PDU into. May overlap with request buffer.
 * @param      maxResponseBytes     Capacity of response buffer.
 * @param[out] numResponseBytes     Length of data that was filled into response buffer.
 * @param[out] pduBytesConsumed     The number of bytes consumed
 *                                  by the PDU parsed
 * @param[out] requestSuccessful    True if the request was successfully handled and not rejected
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the given data does not represent a valid request.
 * @return kHAPError_OutOfResources If not enough memory is available to process the request.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPDUProcedureHandleRequest(
        HAPAccessoryServer* server,
        HAPSession* session,
        void* requestBytes,
        size_t numRequestBytes,
        void* responseBytes,
        size_t maxResponseBytes,
        size_t* numResponseBytes,
        size_t* pduBytesConsumed,
        bool* requestSuccessful);

/**
 * Processes a non-fragmented HAP PDU request that is locked to a specific characteristic.
 *
 * - The HAP PDU is rejected unless its indicated instance ID matches the locked to characteristic.
 *
 * @param      server               Accessory server.
 * @param      session              Session over which the PDU has been received.
 * @param      characteristic       The characteristic to which this HAP PDU shall be locked.
 * @param      service              The service that contains the characteristic.
 * @param      accessory            The accessory that provides the service.
 * @param      requestBytes         Buffer that contains the request PDU.
 * @param      numRequestBytes      Length of data in request buffer.
 * @param[out] responseBytes        Buffer to fill response PDU into. May overlap with request buffer.
 * @param      maxResponseBytes     Capacity of response buffer.
 * @param[out] numResponseBytes     Length of data that was filled into response buffer.
 * @param[out] pduBytesConsumed     The number of bytes consumed
 *                                  by the PDU parsed
 * @param[out] requestSuccessful    True if the request was successfully handled and not rejected
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the given data does not represent a valid request.
 * @return kHAPError_OutOfResources If not enough memory is available to process the request.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPDUProcedureHandleRequestForCharacteristic(
        HAPAccessoryServer* server,
        HAPSession* session,
        const HAPCharacteristic* characteristic,
        const HAPService* service,
        const HAPAccessory* accessory,
        void* requestBytes,
        size_t numRequestBytes,
        void* responseBytes,
        size_t maxResponseBytes,
        size_t* numResponseBytes,
        size_t* pduBytesConsumed,
        bool* requestSuccessful);

#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
