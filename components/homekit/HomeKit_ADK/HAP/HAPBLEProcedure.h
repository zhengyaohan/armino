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

#ifndef HAP_BLE_PROCEDURE_H
#define HAP_BLE_PROCEDURE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP+API.h"
#include "HAPBLETransaction.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)

/**
 * Procedure.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 7.3.5 HAP Procedures
 */
HAP_ENUM_BEGIN(uint8_t, HAPBLEProcedureMultiTransactionType) { /** No procedure in progress. */
                                                               kHAPBLEProcedureMultiTransactionType_None,

                                                               /** HAP Characteristic Timed Write Procedure. */
                                                               kHAPBLEProcedureMultiTransactionType_TimedWrite
} HAP_ENUM_END(uint8_t, HAPBLEProcedureMultiTransactionType);

/**
 * Procedure.
 */
typedef struct _HAPBLEProcedure {
    /**@cond */
    /** Accessory server that the procedure is attached to. */
    HAPAccessoryServer* server;

    /** Session that the procedure is attached to. */
    HAPSession* session;

    /** Characteristic that the procedure is attached to. */
    const HAPCharacteristic* characteristic;

    /** The service that contains the characteristic. */
    const HAPService* service;

    /** The accessory that provides the service. */
    const HAPAccessory* accessory;

    /** Transaction state. */
    HAPBLETransaction transaction;

    /** Value buffer. */
    void* scratchBytes;

    /** Value buffer length. */
    size_t numScratchBytes;

    /** Procedure timer. Starts on first GATT write. Ends on last GATT read. */
    HAPPlatformTimerRef procedureTimer;

    /** Active multi-transaction procedure. */
    HAPBLEProcedureMultiTransactionType multiTransactionType;

    /** Procedure is secure. */
    bool startedSecured;

    /**
     * Procedure specific elements.
     */
    union {
        /** No active procedure. */
        void* none;

        /**
         * HAP Characteristic Timed Write Procedure.
         */
        struct {
            HAPTime timedWriteStartTime; /**< Time when Timed Write request was received. */
            HAPTLVReader bodyReader;     /**< Timed Write Body Reader. */
        } timedWrite;
    } _;
    /**@endcond */
} HAPBLEProcedure;
HAP_STATIC_ASSERT(sizeof(HAPBLEProcedure) >= sizeof(HAPBLEProcedure), HAPBLEProcedure);

/**
 * Attaches a procedure to a characteristic.
 *
 * @param[out] bleProcedure         Procedure.
 * @param      scratchBytes         Value buffer to use.
 * @param      numScratchBytes      Capacity of value buffer.
 * @param      server               Accessory server to attach to.
 * @param      session              Session to attach to.
 * @param      characteristic       Characteristic to attach to.
 * @param      service              The service that contains the characteristic.
 * @param      accessory            The accessory that provides the service.
 */
void HAPBLEProcedureAttach(
        HAPBLEProcedure* bleProcedure,
        void* scratchBytes,
        size_t numScratchBytes,
        HAPAccessoryServer* server,
        HAPSession* session,
        const HAPCharacteristic* characteristic,
        const HAPService* service,
        const HAPAccessory* accessory);

/**
 * De-initializes a procedure.
 *
 * @param      bleProcedure         Procedure.
 */
void HAPBLEProcedureDestroy(HAPBLEProcedure* bleProcedure);

/**
 * Gets the characteristic that a procedure is attached to.
 *
 * @param      bleProcedure         Procedure.
 *
 * @return Attached characteristic.
 */
const HAPCharacteristic* HAPBLEProcedureGetAttachedCharacteristic(const HAPBLEProcedure* bleProcedure);

/**
 * Queries a procedure to determine whether a transaction is currently in progress.
 * When no transaction is in progress, it is safe to attach the procedure to a different characteristic
 * through another #HAPBLEProcedureAttach invocation without losing data.
 *
 * - This function only works for procedures that are attached to a characteristic.
 *
 * @param      bleProcedure         Procedure.
 *
 * @return true                     If a procedure is in progress.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPBLEProcedureIsInProgress(const HAPBLEProcedure* bleProcedure);

/**
 * Processes a GATT Write request.
 *
 * @param      bleProcedure         Procedure.
 * @param      bytes                Body of the GATT Write request.
 * @param      numBytes             Length of body.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If the request cannot be processed in the current state. Link should be terminated.
 * @return kHAPError_InvalidData    If the controller sent a malformed request. Link should be terminated.
 * @return kHAPError_OutOfResources If there are not enough resources to handle the request. Link should be terminated.
 */
HAP_RESULT_USE_CHECK
HAPError HAPBLEProcedureHandleGATTWrite(HAPBLEProcedure* bleProcedure, void* bytes, size_t numBytes);

/**
 * Processes a GATT Read request.
 *
 * @param      bleProcedure         Procedure.
 * @param[out] bytes                Buffer to serialize response into.
 * @param      maxBytes             Capacity of buffer.
 * @param[out] numBytes             Length of response put into buffer.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If the request cannot be processed in the current state. Link should be terminated.
 * @return kHAPError_OutOfResources If buffer not large enough.
 */
HAP_RESULT_USE_CHECK
HAPError HAPBLEProcedureHandleGATTRead(HAPBLEProcedure* bleProcedure, void* bytes, size_t maxBytes, size_t* numBytes);

#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
