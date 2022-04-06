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

#include "HAPPairingPairings.h"

#include "HAPAccessoryServer+Internal.h"
#include "HAPBLEAccessoryServer.h"
#include "HAPLogSubsystem.h"
#include "HAPPairing.h"
#include "HAPSession.h"
#include "HAPTLV+Internal.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "PairingPairings" };

void HAPPairingPairingsReset(HAPSession* session) {
    HAPPrecondition(session);

    // Reset Pairings state.
    HAPRawBufferZero(&session->state.pairings, sizeof session->state.pairings);
}

void HAPPairingPairingsHandleSessionInvalidation(HAPAccessoryServer* server, HAPSession* session) {
    HAPPrecondition(server);
    HAPPrecondition(session);

    // Remove pairing if the remove pairing command was received but not yet responded to.
    if (session->state.pairings.state == 1 && session->state.pairings.method == kHAPPairingMethod_RemovePairing) {
        HAPTLVWriter writer;
        uint8_t buffer[16]; // Arbitrary size. It shouldn't matter if the buffer runs short.
        HAPTLVWriterCreate(&writer, buffer, sizeof buffer);
        HAPError err = HAPPairingPairingsHandleRead(server, session, &writer);
        if (err) {
            // err is OK and is expected.
            HAPLogInfo(
                    &logObject,
                    "Ignore HAPPairingPairingsHandleRead() failure. "
                    "The function was called to clean the internal state.");
        }
    }
}

/**
 * Add Pairing M1 TLVs.
 */
typedef struct {
    HAPTLV* stateTLV;       /**< kTLVType_State. */
    HAPTLV* methodTLV;      /**< kTLVType_Method. */
    HAPTLV* identifierTLV;  /**< kTLVType_Identifier. */
    HAPTLV* publicKeyTLV;   /**< kTLVType_PublicKey. */
    HAPTLV* permissionsTLV; /**< kTLVType_Permissions. */
} HAPPairingPairingsAddPairingM1TLVs;

/**
 * Processes Add Pairing M1.
 *
 * @param      server              Accessory server.
 * @param      session             The session over which the request has been received.
 * @param      tlvs                 TLVs.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If a different request is expected in the current state.
 * @return kHAPError_Unknown        If persistent store access failed.
 * @return kHAPError_InvalidData    If the controller sent a malformed request.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPPairingPairingsAddPairingProcessM1(
        HAPAccessoryServer* server,
        HAPSession* session,
        const HAPPairingPairingsAddPairingM1TLVs* tlvs) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(session->state.pairings.state == 1);
    HAPPrecondition(session->state.pairings.method == kHAPPairingMethod_AddPairing);
    HAPPrecondition(!session->state.pairings.error);
    HAPPrecondition(session->hap.active);
    HAPPrecondition(tlvs);
    HAPPrecondition(tlvs->stateTLV);
    HAPPrecondition(tlvs->stateTLV->type == kHAPPairingTLVType_State);
    HAPPrecondition(tlvs->methodTLV);
    HAPPrecondition(tlvs->methodTLV->type == kHAPPairingTLVType_Method);
    HAPPrecondition(tlvs->identifierTLV);
    HAPPrecondition(tlvs->identifierTLV->type == kHAPPairingTLVType_Identifier);
    HAPPrecondition(tlvs->publicKeyTLV);
    HAPPrecondition(tlvs->publicKeyTLV->type == kHAPPairingTLVType_PublicKey);
    HAPPrecondition(tlvs->permissionsTLV);
    HAPPrecondition(tlvs->permissionsTLV->type == kHAPPairingTLVType_Permissions);

    HAPError err;

    // See HomeKit Accessory Protocol Specification R17
    // Section 5.11.1 M1: iOS Device -> Accessory - `Add Pairing Request'

    HAPLogDebug(&logObject, "Add Pairing M1: Add Pairing Request.");

    // Validate kTLVType_State.
    if (!tlvs->stateTLV->value.bytes) {
        HAPLog(&logObject, "Add Pairing M1: kTLVType_State missing.");
        return kHAPError_InvalidData;
    }
    if (tlvs->stateTLV->value.numBytes != 1) {
        HAPLog(&logObject,
               "Add Pairing M1: kTLVType_State has invalid length (%lu).",
               (unsigned long) tlvs->stateTLV->value.numBytes);
        return kHAPError_InvalidData;
    }
    uint8_t state = ((const uint8_t*) tlvs->stateTLV->value.bytes)[0];
    if (state != 1) {
        HAPLog(&logObject, "Add Pairing M1: kTLVType_State invalid: %u.", state);
        return kHAPError_InvalidData;
    }

    // Validate kTLVType_Method.
    if (!tlvs->methodTLV->value.bytes) {
        HAPLog(&logObject, "Add Pairing M1: kTLVType_Method missing.");
        return kHAPError_InvalidData;
    }
    if (tlvs->methodTLV->value.numBytes != 1) {
        HAPLog(&logObject,
               "Add Pairing M1: kTLVType_Method has invalid length (%lu).",
               (unsigned long) tlvs->methodTLV->value.numBytes);
        return kHAPError_InvalidData;
    }
    uint8_t method = ((const uint8_t*) tlvs->methodTLV->value.bytes)[0];
    if (method != kHAPPairingMethod_AddPairing) {
        HAPLog(&logObject, "Add Pairing M1: kTLVType_Method invalid: %u.", method);
        return kHAPError_InvalidData;
    }

    // Validate kTLVType_Identifier.
    if (!tlvs->identifierTLV->value.bytes) {
        HAPLog(&logObject, "Add Pairing M1: kTLVType_Identifier missing.");
        return kHAPError_InvalidData;
    }
    if (tlvs->identifierTLV->value.numBytes > sizeof(HAPPairingID)) {
        HAPLog(&logObject,
               "Add Pairing M1: kTLVType_Identifier has invalid length (%lu).",
               (unsigned long) tlvs->identifierTLV->value.numBytes);
        return kHAPError_InvalidData;
    }

    // Validate kTLVType_PublicKey.
    if (!tlvs->publicKeyTLV->value.bytes) {
        HAPLog(&logObject, "Add Pairing M1: kTLVType_Identifier missing.");
        return kHAPError_InvalidData;
    }
    if (tlvs->publicKeyTLV->value.numBytes != sizeof(HAPPairingPublicKey)) {
        HAPLog(&logObject,
               "Add Pairing M1: kTLVType_Identifier has invalid length (%lu).",
               (unsigned long) tlvs->publicKeyTLV->value.numBytes);
        return kHAPError_InvalidData;
    }

    // Validate kTLVType_Permissions.
    if (!tlvs->permissionsTLV->value.bytes) {
        HAPLog(&logObject, "Add Pairing M1: kTLVType_Permissions missing.");
        return kHAPError_InvalidData;
    }
    if (tlvs->permissionsTLV->value.numBytes != 1) {
        HAPLog(&logObject,
               "Add Pairing M1: kTLVType_Permissions has invalid length (%lu).",
               (unsigned long) tlvs->permissionsTLV->value.numBytes);
        return kHAPError_InvalidData;
    }
    uint8_t permissions = ((const uint8_t*) tlvs->permissionsTLV->value.bytes)[0];
    if (permissions & ~1U) {
        HAPLog(&logObject, "Add Pairing M1: kTLVType_Permissions invalid: %u.", permissions);
        return kHAPError_InvalidData;
    }

    // Check if a pairing for the additional controller's pairing identifier exists.
    HAPPairingID identifier;
    HAPAssert(tlvs->identifierTLV->value.numBytes <= sizeof identifier.bytes);
    HAPRawBufferCopyBytes(
            identifier.bytes, HAPNonnullVoid(tlvs->identifierTLV->value.bytes), tlvs->identifierTLV->value.numBytes);
    size_t numIdentifierBytes = tlvs->identifierTLV->value.numBytes;

    bool found;
    HAPPairing pairing;
    HAPPairingIndex pairingIndex;
    err = HAPPairingFind(server, &identifier, numIdentifierBytes, &pairing, &pairingIndex, &found);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }
    if (found) {
        // Check if the additional controller's long-term public key matches the
        // stored public key for the additional controller's pairing identifier.

        if (!HAPRawBufferAreEqual(
                    pairing.publicKey.value,
                    HAPNonnullVoid(tlvs->publicKeyTLV->value.bytes),
                    sizeof pairing.publicKey.value)) {
            HAPLog(&logObject,
                   "Add Pairing M1: Additional controller's long-term public key does not match "
                   "the stored public key for the additional controller's pairing identifier.");
            session->state.pairings.error = kHAPPairingError_Unknown;
            return kHAPError_None;
        }

        // Update the permissions of the controller.
        pairing.permissions = permissions;
        err = HAPPairingUpdatePermissions(server, pairingIndex, permissions);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            return err;
        }

        // If the admin controller pairing is removed, all pairings on the accessory must be removed.
        err = HAPAccessoryServerCleanupPairings(server);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPLog(&logObject, "Add Pairing M1: Failed to cleanup pairings.");
            session->state.pairings.error = kHAPPairingError_Unknown;
            return kHAPError_None;
        }
    } else {
        // Add pairing.
        HAPRawBufferZero(&pairing, sizeof pairing);

        HAPRawBufferCopyBytes(
                pairing.identifier.bytes,
                HAPNonnullVoid(tlvs->identifierTLV->value.bytes),
                tlvs->identifierTLV->value.numBytes);
        HAPAssert(tlvs->identifierTLV->value.numBytes <= sizeof pairing.identifier.bytes);
        pairing.numIdentifierBytes = (uint8_t) tlvs->identifierTLV->value.numBytes;
        HAPRawBufferCopyBytes(
                pairing.publicKey.value,
                HAPNonnullVoid(tlvs->publicKeyTLV->value.bytes),
                tlvs->publicKeyTLV->value.numBytes);
        pairing.permissions = permissions;

        err = HAPPairingAdd(server, &pairing, &pairingIndex);
        if (err) {
            if (err == kHAPError_OutOfResources) {
                HAPLog(&logObject, "Add Pairing M1: No space for additional pairings.");
                session->state.pairings.error = kHAPPairingError_MaxPeers;
                return kHAPError_None;
            }

            HAPAssert(err == kHAPError_Unknown);
            HAPLog(&logObject, "Add Pairing M1: Failed to add pairing.");
            session->state.pairings.error = kHAPPairingError_Unknown;
            return kHAPError_None;
        }
    }

    return kHAPError_None;
}

/**
 * Processes Add Pairing M2.
 *
 * @param      server               Accessory server.
 * @param      session             The session over which the response will be sent.
 * @param      responseWriter       TLV writer for serializing the response.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If a different request is expected in the current state.
 * @return kHAPError_OutOfResources If response writer does not have enough capacity.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPPairingPairingsAddPairingGetM2(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPTLVWriter* responseWriter) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(session->state.pairings.state == 2);
    HAPPrecondition(session->state.pairings.method == kHAPPairingMethod_AddPairing);
    HAPPrecondition(!session->state.pairings.error);
    HAPPrecondition(session->hap.active);
    HAPPrecondition(responseWriter);

    HAPError err;

    // See HomeKit Accessory Protocol Specification R17
    // Section 5.11.2 M2: Accessory -> iOS Device - `Add Pairing Response'

    HAPLogDebug(&logObject, "Add Pairing M2: Add Pairing Response.");

    // kTLVType_State.
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPairingTLVType_State,
                              .value = { .bytes = &session->state.pairings.state, .numBytes = 1 } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // Reset Pairings session.
    HAPPairingPairingsReset(session);
    return kHAPError_None;
}

/**
 * Remove Pairing M1 TLVs.
 */
typedef struct {
    HAPTLV* stateTLV;      /**< kTLVType_State. */
    HAPTLV* methodTLV;     /**< kTLVType_Method. */
    HAPTLV* identifierTLV; /**< kTLVType_Identifier. */
} HAPPairingPairingsRemovePairingM1TLVs;

/**
 * Processes Remove Pairing M1.
 *
 * @param      server               Accessory server.
 * @param      session             The session over which the request has been received.
 * @param      tlvs                 TLVs.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If a different request is expected in the current state.
 * @return kHAPError_InvalidData    If the controller sent a malformed request.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPPairingPairingsRemovePairingProcessM1(
        HAPAccessoryServer* server,
        HAPSession* session,
        const HAPPairingPairingsRemovePairingM1TLVs* tlvs) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(session->state.pairings.state == 1);
    HAPPrecondition(session->state.pairings.method == kHAPPairingMethod_RemovePairing);
    HAPPrecondition(!session->state.pairings.error);
    HAPPrecondition(session->hap.active);
    HAPPrecondition(tlvs);
    HAPPrecondition(tlvs->stateTLV);
    HAPPrecondition(tlvs->stateTLV->type == kHAPPairingTLVType_State);
    HAPPrecondition(tlvs->methodTLV);
    HAPPrecondition(tlvs->methodTLV->type == kHAPPairingTLVType_Method);
    HAPPrecondition(tlvs->identifierTLV);
    HAPPrecondition(tlvs->identifierTLV->type == kHAPPairingTLVType_Identifier);

    // See HomeKit Accessory Protocol Specification R17
    // Section 5.12.1 M1: iOS Device -> Accessory - `Remove Pairing Request'

    HAPLogDebug(&logObject, "Remove Pairing M1: Remove Pairing Request.");

    // Validate kTLVType_State.
    if (!tlvs->stateTLV->value.bytes) {
        HAPLog(&logObject, "Remove Pairing M1: kTLVType_State missing.");
        return kHAPError_InvalidData;
    }
    if (tlvs->stateTLV->value.numBytes != 1) {
        HAPLog(&logObject,
               "Remove Pairing M1: kTLVType_State has invalid length (%lu).",
               (unsigned long) tlvs->stateTLV->value.numBytes);
        return kHAPError_InvalidData;
    }
    uint8_t state = ((const uint8_t*) tlvs->stateTLV->value.bytes)[0];
    if (state != 1) {
        HAPLog(&logObject, "Remove Pairing M1: kTLVType_State invalid: %u.", state);
        return kHAPError_InvalidData;
    }

    // Validate kTLVType_Method.
    if (!tlvs->methodTLV->value.bytes) {
        HAPLog(&logObject, "Remove Pairing M1: kTLVType_Method missing.");
        return kHAPError_InvalidData;
    }
    if (tlvs->methodTLV->value.numBytes != 1) {
        HAPLog(&logObject,
               "Remove Pairing M1: kTLVType_Method has invalid length (%lu).",
               (unsigned long) tlvs->methodTLV->value.numBytes);
        return kHAPError_InvalidData;
    }
    uint8_t method = ((const uint8_t*) tlvs->methodTLV->value.bytes)[0];
    if (method != kHAPPairingMethod_RemovePairing) {
        HAPLog(&logObject, "Remove Pairing M1: kTLVType_Method invalid: %u.", method);
        return kHAPError_InvalidData;
    }

    // Validate kTLVType_Identifier.
    if (!tlvs->identifierTLV->value.bytes) {
        HAPLog(&logObject, "Remove Pairing M1: kTLVType_Identifier missing.");
        return kHAPError_InvalidData;
    }
    if (tlvs->identifierTLV->value.numBytes > sizeof(HAPPairingID)) {
        HAPLog(&logObject,
               "Remove Pairing M1: kTLVType_Identifier has invalid length (%lu).",
               (unsigned long) tlvs->identifierTLV->value.numBytes);
        return kHAPError_InvalidData;
    }

    // Store pairing to remove.
    HAPRawBufferCopyBytes(
            session->state.pairings.removedPairingID.bytes,
            HAPNonnullVoid(tlvs->identifierTLV->value.bytes),
            tlvs->identifierTLV->value.numBytes);
    session->state.pairings.removedPairingIDLength = tlvs->identifierTLV->value.numBytes;
    return kHAPError_None;
}

/**
 * Processes Remove Pairing M2.
 *
 * @param      server              Accessory server.
 * @param      session             The session over which the response will be sent.
 * @param      responseWriter       TLV writer for serializing the response.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If a different request is expected in the current state.
 * @return kHAPError_Unknown        If persistent store access failed.
 * @return kHAPError_OutOfResources If response writer does not have enough capacity.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPPairingPairingsRemovePairingGetM2(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPTLVWriter* responseWriter) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(session->state.pairings.state == 2);
    HAPPrecondition(session->state.pairings.method == kHAPPairingMethod_RemovePairing);
    HAPPrecondition(!session->state.pairings.error);
    HAPPrecondition(session->hap.active);
    HAPPrecondition(responseWriter);

    HAPError err;

    // See HomeKit Accessory Protocol Specification R17
    // Section 5.12.2 M2: Accessory -> iOS Device - `Remove Pairing Response'

    HAPLogDebug(&logObject, "Remove Pairing M2: Remove Pairing Response.");

    // Find pairing.
    bool found;
    HAPPairing pairing;
    HAPPairingIndex pairingIndex;
    err = HAPPairingFind(
            server,
            &session->state.pairings.removedPairingID,
            session->state.pairings.removedPairingIDLength,
            &pairing,
            &pairingIndex,
            &found);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    // If the pairing exists, remove RemovedControllerPairingIdentifier and its corresponding long-term public
    // key from persistent storage. If a pairing for RemovedControllerPairingIdentifier does not exist, the
    // accessory must return success.
    if (found) {
        // Remove the pairing.
        err = HAPPairingRemove(server, &pairing, pairingIndex);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPLog(&logObject, "Remove Pairing M2: Failed to remove pairing.");
            session->state.pairings.error = kHAPPairingError_Unknown;
            return kHAPError_None;
        }

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
        // BLE: Remove all Pair Resume cache entries related to this pairing.
        if (server->transports.ble) {
            HAPNonnull(server->transports.ble)->sessionCache.invalidateEntriesForPairing(server, (int) pairingIndex);
        }
#endif

        // Invalidate attached HomeKit Data Streams for pairing.
        HAPDataStreamInvalidateAllForHAPPairingID(server, (int) pairingIndex);

        // If the admin controller pairing is removed, all pairings on the accessory must be removed.
        err = HAPAccessoryServerCleanupPairings(server);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPLog(&logObject, "Remove Pairing M2: Failed to cleanup pairings.");
            session->state.pairings.error = kHAPPairingError_Unknown;
            return kHAPError_None;
        }
    }

    // kTLVType_State.
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPairingTLVType_State,
                              .value = { .bytes = &session->state.pairings.state, .numBytes = 1 } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // Reset Pairings session.
    HAPPairingPairingsReset(session);
    return kHAPError_None;
}

/**
 * List Pairings M1 TLVs.
 */
typedef struct {
    HAPTLV* stateTLV;  /**< kTLVType_State. */
    HAPTLV* methodTLV; /**< kTLVType_Method. */
} HAPPairingPairingsListPairingsM1TLVs;

/**
 * Processes List Pairings M1.
 *
 * @param      server               Accessory server.
 * @param      session             The session over which the request has been received.
 * @param      tlvs                 TLVs.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If a different request is expected in the current state.
 * @return kHAPError_InvalidData    If the controller sent a malformed request.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPPairingPairingsListPairingsProcessM1(
        HAPAccessoryServer* server,
        HAPSession* session,
        const HAPPairingPairingsListPairingsM1TLVs* tlvs) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(session->state.pairings.state == 1);
    HAPPrecondition(session->state.pairings.method == kHAPPairingMethod_ListPairings);
    HAPPrecondition(!session->state.pairings.error);
    HAPPrecondition(session->hap.active);
    HAPPrecondition(tlvs);
    HAPPrecondition(tlvs->stateTLV);
    HAPPrecondition(tlvs->stateTLV->type == kHAPPairingTLVType_State);
    HAPPrecondition(tlvs->methodTLV);
    HAPPrecondition(tlvs->methodTLV->type == kHAPPairingTLVType_Method);

    // See HomeKit Accessory Protocol Specification R17
    // Section 5.13.1 M1: iOS Device -> Accessory - `List Pairings Request'

    HAPLogDebug(&logObject, "List Pairings M1: List Pairings Request.");

    // Validate kTLVType_State.
    if (!tlvs->stateTLV->value.bytes) {
        HAPLog(&logObject, "List Pairings M1: kTLVType_State missing.");
        return kHAPError_InvalidData;
    }
    if (tlvs->stateTLV->value.numBytes != 1) {
        HAPLog(&logObject,
               "List Pairings M1: kTLVType_State has invalid length (%lu).",
               (unsigned long) tlvs->stateTLV->value.numBytes);
        return kHAPError_InvalidData;
    }
    uint8_t state = ((const uint8_t*) tlvs->stateTLV->value.bytes)[0];
    if (state != 1) {
        HAPLog(&logObject, "List Pairings M1: kTLVType_State invalid: %u.", state);
        return kHAPError_InvalidData;
    }

    // Validate kTLVType_Method.
    if (!tlvs->methodTLV->value.bytes) {
        HAPLog(&logObject, "List Pairings M1: kTLVType_Method missing.");
        return kHAPError_InvalidData;
    }
    if (tlvs->methodTLV->value.numBytes != 1) {
        HAPLog(&logObject,
               "List Pairings M1: kTLVType_Method has invalid length (%lu).",
               (unsigned long) tlvs->methodTLV->value.numBytes);
        return kHAPError_InvalidData;
    }
    uint8_t method = ((const uint8_t*) tlvs->methodTLV->value.bytes)[0];
    if (method != kHAPPairingMethod_ListPairings) {
        HAPLog(&logObject, "List Pairings M1: kTLVType_Method invalid: %u.", method);
        return kHAPError_InvalidData;
    }

    return kHAPError_None;
}

typedef struct {
    HAPTLVWriter* responseWriter;
    bool needsSeparator;
    HAPError err;
} ListPairingsEnumerateContext;

HAP_RESULT_USE_CHECK
static HAPError ListPairingsEnumerateCallback(
        void* _Nullable context,
        HAPAccessoryServer* server,
        HAPPairingIndex pairingIndex,
        bool* shouldContinue) {
    HAPPrecondition(context);
    ListPairingsEnumerateContext* arguments = context;
    HAPPrecondition(server);
    HAPPrecondition(arguments->responseWriter);
    HAPPrecondition(!arguments->err);
    HAPPrecondition(shouldContinue);

    HAPError err;

    bool exists;
    HAPPairing pairing;
    err = HAPPairingGet(server, pairingIndex, &pairing, &exists);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }
    HAPAssert(exists);

    if (pairing.numIdentifierBytes > sizeof pairing.identifier.bytes) {
        HAPLogError(&logObject, "Invalid pairing 0x%02X ID size %u.", pairingIndex, pairing.numIdentifierBytes);
        return kHAPError_Unknown;
    }

    // Write separator if necessary.
    if (arguments->needsSeparator) {
        // kTLVType_Separator.
        err = HAPTLVWriterAppend(
                arguments->responseWriter,
                &(const HAPTLV) { .type = kHAPPairingTLVType_Separator, .value = { .bytes = NULL, .numBytes = 0 } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            arguments->err = err;
            *shouldContinue = false;
            return kHAPError_None;
        }
    }
    arguments->needsSeparator = true;

    // Write pairing.
    err = HAPTLVWriterAppend(
            arguments->responseWriter,
            &(const HAPTLV) { .type = kHAPPairingTLVType_Identifier,
                              .value = { .bytes = pairing.identifier.bytes, .numBytes = pairing.numIdentifierBytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        arguments->err = err;
        *shouldContinue = false;
        return kHAPError_None;
    }
    err = HAPTLVWriterAppend(
            arguments->responseWriter,
            &(const HAPTLV) {
                    .type = kHAPPairingTLVType_PublicKey,
                    .value = { .bytes = pairing.publicKey.value, .numBytes = sizeof pairing.publicKey.value } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        arguments->err = err;
        *shouldContinue = false;
        return kHAPError_None;
    }
    err = HAPTLVWriterAppend(
            arguments->responseWriter,
            &(const HAPTLV) { .type = kHAPPairingTLVType_Permissions,
                              .value = { .bytes = &pairing.permissions, .numBytes = 1 } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        arguments->err = err;
        *shouldContinue = false;
        return kHAPError_None;
    }

    return kHAPError_None;
}

/**
 * Processes List Pairings M2.
 *
 * @param      server              Accessory server.
 * @param      session             The session over which the response will be sent.
 * @param      responseWriter       TLV writer for serializing the response.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If a different request is expected in the current state.
 * @return kHAPError_OutOfResources If response writer does not have enough capacity.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPPairingPairingsListPairingsGetM2(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPTLVWriter* responseWriter) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(session->state.pairings.state == 2);
    HAPPrecondition(session->state.pairings.method == kHAPPairingMethod_ListPairings);
    HAPPrecondition(!session->state.pairings.error);
    HAPPrecondition(session->hap.active);
    HAPPrecondition(responseWriter);

    HAPError err;

    // See HomeKit Accessory Protocol Specification R17
    // Section 5.13.2 M2: Accessory -> iOS Device - `List Pairings Response'

    HAPLogDebug(&logObject, "List Pairings M2: List Pairings Response.");

    // kTLVType_State.
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPairingTLVType_State,
                              .value = { .bytes = &session->state.pairings.state, .numBytes = 1 } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // List pairings.
    ListPairingsEnumerateContext context;
    HAPRawBufferZero(&context, sizeof context);
    context.responseWriter = responseWriter;
    context.needsSeparator = false;
    context.err = kHAPError_None;
    err = HAPPairingEnumerate(server, ListPairingsEnumerateCallback, &context);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }
    if (context.err) {
        return context.err;
    }

    // Reset Pairings session.
    HAPPairingPairingsReset(session);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPairingPairingsHandleWrite(HAPAccessoryServer* server, HAPSession* session, HAPTLVReader* requestReader) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(requestReader);

    HAPError err;

    // Parse request.
    HAPTLV methodTLV, identifierTLV, publicKeyTLV, stateTLV, permissionsTLV;
    methodTLV.type = kHAPPairingTLVType_Method;
    identifierTLV.type = kHAPPairingTLVType_Identifier;
    publicKeyTLV.type = kHAPPairingTLVType_PublicKey;
    stateTLV.type = kHAPPairingTLVType_State;
    permissionsTLV.type = kHAPPairingTLVType_Permissions;
    err = HAPTLVReaderGetAll(
            requestReader,
            (HAPTLV* const[]) { &methodTLV, &identifierTLV, &publicKeyTLV, &stateTLV, &permissionsTLV, NULL });
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        HAPPairingPairingsReset(session);
        return err;
    }

    // Process request.
    switch (session->state.pairings.state) {
        case 0: {
            session->state.pairings.state++;
            if (!methodTLV.value.bytes || methodTLV.value.numBytes != 1) {
                err = kHAPError_InvalidData;
                break;
            }
            uint8_t method = ((const uint8_t*) methodTLV.value.bytes)[0];
            if (method != kHAPPairingMethod_AddPairing && method != kHAPPairingMethod_RemovePairing &&
                method != kHAPPairingMethod_ListPairings) {
                err = kHAPError_InvalidData;
                break;
            }
            session->state.pairings.method = method;

            // Admin access only.
            if (!session->hap.active) {
                HAPLog(&logObject, "Pairings M1: Rejected access from non-secure session.");
                session->state.pairings.error = kHAPPairingError_Authentication;
                err = kHAPError_None;
                break;
            }
            HAPAssert(session->hap.pairingID >= 0);

            bool exists;
            HAPPairing pairing;
            err = HAPPairingGet(server, (HAPPairingIndex) session->hap.pairingID, &pairing, &exists);
            if (err) {
                HAPAssert(err == kHAPError_Unknown);
                break;
            }
            if (!exists) {
                err = kHAPError_Unknown;
                break;
            }
            if (!(pairing.permissions & 0x01U)) {
                HAPLog(&logObject, "Pairings M1: Rejected access from non-admin controller.");
                session->state.pairings.error = kHAPPairingError_Authentication;
                err = kHAPError_None;
                break;
            }

            switch (session->state.pairings.method) {
                case kHAPPairingMethod_AddPairing: {
                    err = HAPPairingPairingsAddPairingProcessM1(
                            server,
                            session,
                            &(const HAPPairingPairingsAddPairingM1TLVs) { .stateTLV = &stateTLV,
                                                                          .methodTLV = &methodTLV,
                                                                          .identifierTLV = &identifierTLV,
                                                                          .publicKeyTLV = &publicKeyTLV,
                                                                          .permissionsTLV = &permissionsTLV });
                    if (err) {
                        HAPAssert(err == kHAPError_InvalidData);
                    }
                    break;
                }
                case kHAPPairingMethod_RemovePairing: {
                    err = HAPPairingPairingsRemovePairingProcessM1(
                            server,
                            session,
                            &(const HAPPairingPairingsRemovePairingM1TLVs) {
                                    .stateTLV = &stateTLV,
                                    .methodTLV = &methodTLV,
                                    .identifierTLV = &identifierTLV,
                            });
                    if (err) {
                        HAPAssert(err == kHAPError_InvalidData);
                    }
                    break;
                }
                case kHAPPairingMethod_ListPairings: {
                    err = HAPPairingPairingsListPairingsProcessM1(
                            server,
                            session,
                            &(const HAPPairingPairingsListPairingsM1TLVs) { .stateTLV = &stateTLV,
                                                                            .methodTLV = &methodTLV });
                    if (err) {
                        HAPAssert(err == kHAPError_InvalidData);
                    }
                    break;
                }
                default:
                    HAPFatalError();
            }
            break;
        }
        default: {
            HAPLog(&logObject, "Received unexpected Pairings write in state M%d.", session->state.pairings.state);
            err = kHAPError_InvalidState;
            break;
        }
    }
    if (err) {
        HAPPairingPairingsReset(session);
        return err;
    }
    return kHAPError_None;
}

/**
 * Writes the error of a session.
 *
 * @param      server               Accessory server.
 * @param      session             The session over which the response will be sent.
 * @param      responseWriter       TLV writer for serializing the response.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If no error is pending.
 * @return kHAPError_OutOfResources If response writer does not have enough capacity.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPPairingPairingsGetErrorResponse(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPTLVWriter* responseWriter) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(responseWriter);
    HAPPrecondition(session->state.pairings.error);

    HAPError err;

    // kTLVType_State.
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPairingTLVType_State,
                              .value = { .bytes = &session->state.pairings.state, .numBytes = 1 } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // kTLVType_Error.
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPairingTLVType_Error,
                              .value = { .bytes = &session->state.pairings.error, .numBytes = 1 } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPairingPairingsHandleRead(HAPAccessoryServer* server, HAPSession* session, HAPTLVWriter* responseWriter) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(responseWriter);

    HAPError err;

    // Handle pending error.
    if (session->state.pairings.error) {
        // Advance state.
        session->state.pairings.state++;

        err = HAPPairingPairingsGetErrorResponse(server, session, responseWriter);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            HAPPairingPairingsReset(session);
            return err;
        }

        // Reset session.
        HAPPairingPairingsReset(session);
        return kHAPError_None;
    }

    // Process request.
    switch (session->state.pairings.state) {
        case 1: {
            session->state.pairings.state++;

            // Admin access only.
            if (!session->hap.active) {
                HAPLog(&logObject, "Pairings M1: Rejected access from non-secure session.");
                session->state.pairings.error = kHAPPairingError_Authentication;
                err = kHAPError_None;
                break;
            }
            HAPAssert(session->hap.pairingID >= 0);

            bool exists;
            HAPPairing pairing;
            err = HAPPairingGet(server, (HAPPairingIndex) session->hap.pairingID, &pairing, &exists);
            if (err) {
                HAPAssert(err == kHAPError_Unknown);
                break;
            }
            if (!exists) {
                err = kHAPError_Unknown;
                break;
            }

            if (!(pairing.permissions & 0x01U)) {
                HAPLog(&logObject, "Pairings M1: Rejected access from non-admin controller.");
                session->state.pairings.error = kHAPPairingError_Authentication;
                err = kHAPError_None;
                break;
            }

            switch (session->state.pairings.method) {
                case kHAPPairingMethod_AddPairing: {
                    err = HAPPairingPairingsAddPairingGetM2(server, session, responseWriter);
                    if (err) {
                        HAPAssert(err == kHAPError_OutOfResources);
                    }
                    break;
                }
                case kHAPPairingMethod_RemovePairing: {
                    err = HAPPairingPairingsRemovePairingGetM2(server, session, responseWriter);
                    if (err) {
                        HAPAssert(err == kHAPError_OutOfResources);
                    }
                    break;
                }
                case kHAPPairingMethod_ListPairings: {
                    err = HAPPairingPairingsListPairingsGetM2(server, session, responseWriter);
                    if (err) {
                        HAPAssert(err == kHAPError_OutOfResources);
                    }
                    break;
                }
                default:
                    HAPFatalError();
            }
            break;
        }
        default: {
            HAPLog(&logObject, "Received unexpected Pairings read in state M%d.", session->state.pairings.state);
            err = kHAPError_InvalidState;
            break;
        }
    }
    if (err) {
        HAPPairingPairingsReset(session);
        return err;
    }

    // Handle pending error.
    if (session->state.pairings.error) {
        err = HAPPairingPairingsGetErrorResponse(server, session, responseWriter);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            HAPPairingPairingsReset(session);
            return err;
        }

        // Reset session.
        HAPPairingPairingsReset(session);
        return kHAPError_None;
    }

    return kHAPError_None;
}
