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

#include "HAPAccessoryServer+Internal.h"
#include "HAPBLEAccessoryServer.h"
#include "HAPDeviceID.h"
#include "HAPLogSubsystem.h"
#include "HAPPairingBLESessionCache.h"
#include "HAPSession.h"
#include "HAPTLV+Internal.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "PairingPairVerify" };

// Session security.
// Once the controller and accessory have established an authenticated ephemeral shared secret using Pair Verify,
// both the controller and the accessory use the shared secret to derive the encryption keys for session security.
//
// The following key derivation scheme is used for control messages (requests and responses):
//
// ControlAccessoryToControllerKey = HKDF-SHA-512 of
//     InputKey   = <Pair Verify shared secret>
//     Salt       = "Control-Salt"
//     Info       = "Control-Read-Encryption-Key"
//     OutputSize = 32 bytes
//
// ControlControllerToAccessoryKey = HKDF-SHA512 of
//     InputKey   = <Pair Verify shared secret>
//     Salt       = "Control-Salt"
//     Info       = "Control-Write-Encryption-Key"
//     OutputSize = 32 bytes
//
// The controller and accessory use the derived keys in the following manner:
//
//              +---------------------------------+---------------------------------+
//              |         Encryption Key          |         Decryption Key          |
// +------------+---------------------------------+---------------------------------+
// | Accessory  | ControlAccessoryToControllerKey | ControlControllerToAccessoryKey |
// +------------+---------------------------------+---------------------------------+
// | Controller | ControlControllerToAccessoryKey | ControlAccessoryToControllerKey |
// +------------+---------------------------------+---------------------------------+
//
// The following key derivation scheme is used for event messages:
//
// EventAccessoryToControllerKey = HKDF-SHA-512 of
//     InputKey   = <Pair Verify shared secret>
//     Salt       = "Event-Salt"
//     Info       = "Event-Read-Encryption-Key"
//     OutputSize = 32 bytes
//
// The controller and accessory use the derived key in the following manner:
//
//              +-------------------------------+-------------------------------+
//              |        Encryption Key         |        Decryption Key         |
// +------------+-------------------------------+-------------------------------+
// | Accessory  | EventAccessoryToControllerKey |              n/a              |
// +------------+-------------------------------+-------------------------------+
// | Controller |              n/a              | EventAccessoryToControllerKey |
// +------------+-------------------------------+-------------------------------+
//
// Each message is secured with the AEAD algorithm AEAD_CHACHA20_POLY1305 as specified in Section 2.8 of RFC 7539.
// The 32-bit fixed-common part of the 96-bit nonce is all zeros: 00 00 00 00
//
// Once session security is established, if the accessory encounters a decryption failure then it must immediately
// tear down the security session. Accessories must support multiple iterations of Pair Verify from a single origin.
// When a new Pair Verify request is received from the same origin, the accessory must tear down its security session
// and respond to any pending transactions with an error.
//
// See HomeKit Accessory Protocol Specification R17
// Section 6.5.2 Session Security
//
// See HomeKit Accessory Protocol Specification R17
// Section 7.4.7.2 Session Security
//
// See HomeKit Accessory Protocol Specification R17
// Section 7.4.7.5 Securing HAP PDUs

static HAPPlatformThreadWakeLock kPairVerifyWakeLock;
#define PAIR_VERIFY_WAKELOCK_TIMEOUT_MS 5000

void HAPPairingPairVerifyReset(HAPSession* session) {
    HAPPrecondition(session);

    // Reset Pair Verify procedure state.
    HAPRawBufferZero(&session->state.pairVerify, sizeof session->state.pairVerify);
    session->state.pairVerify.pairingID = -1;
}

/**
 * Starts the HAP session after successful Pair Verify / Pair Resume.
 *
 * @param      session             Session.
 */
static void HAPPairingPairVerifyStartSession(HAPSession* session) {
    HAPPrecondition(session);
    HAPPrecondition(session->state.pairVerify.pairingID >= 0);

    // Initialize HAP session.
    HAPRawBufferZero(&session->hap, sizeof session->hap);

    // See HomeKit Accessory Protocol Specification R17
    // Section 6.5.2 Session Security
    // See HomeKit Accessory Protocol Specification R17
    // Section 7.4.7.2 Session Security
    // Refresh the wakelock.
    // Normally this is where the wakelock would be removed, but pair verify is almost always followed
    // by a read or write request that we will also want to service, so we are going to do a little
    // branch prediction here.
    HAPPlatformThreadAddWakeLock(&kPairVerifyWakeLock, PAIR_VERIFY_WAKELOCK_TIMEOUT_MS);

    // Derive encryption keys.
    {
        static const uint8_t salt[] = "Control-Salt";
        {
            static const uint8_t info[] = "Control-Read-Encryption-Key";
            HAP_hkdf_sha512(
                    session->hap.accessoryToController.controlChannel.key.bytes,
                    sizeof session->hap.accessoryToController.controlChannel.key.bytes,
                    session->state.pairVerify.cv_KEY,
                    sizeof session->state.pairVerify.cv_KEY,
                    salt,
                    sizeof salt - 1,
                    info,
                    sizeof info - 1);
            HAPLogSensitiveBufferDebug(
                    &logObject,
                    session->hap.accessoryToController.controlChannel.key.bytes,
                    sizeof session->hap.accessoryToController.controlChannel.key.bytes,
                    "Pair Verify Start Session: ControlAccessoryToControllerKey");
        }
        {
            static const uint8_t info[] = "Control-Write-Encryption-Key";
            HAP_hkdf_sha512(
                    session->hap.controllerToAccessory.controlChannel.key.bytes,
                    sizeof session->hap.controllerToAccessory.controlChannel.key.bytes,
                    session->state.pairVerify.cv_KEY,
                    sizeof session->state.pairVerify.cv_KEY,
                    salt,
                    sizeof salt - 1,
                    info,
                    sizeof info - 1);
            HAPLogSensitiveBufferDebug(
                    &logObject,
                    session->hap.controllerToAccessory.controlChannel.key.bytes,
                    sizeof session->hap.controllerToAccessory.controlChannel.key.bytes,
                    "Pair Verify Start Session: ControlControllerToAccessoryKey");
        }
        session->hap.accessoryToController.controlChannel.nonce = 0;
        session->hap.controllerToAccessory.controlChannel.nonce = 0;
    }

    {
        static const uint8_t salt[] = "Event-Salt";
        {
            static const uint8_t info[] = "Event-Read-Encryption-Key";
            HAP_hkdf_sha512(
                    session->hap.accessoryToController.eventChannel.key.bytes,
                    sizeof session->hap.accessoryToController.eventChannel.key.bytes,
                    session->state.pairVerify.cv_KEY,
                    sizeof session->state.pairVerify.cv_KEY,
                    salt,
                    sizeof salt - 1,
                    info,
                    sizeof info - 1);
            HAPLogSensitiveBufferDebug(
                    &logObject,
                    session->hap.accessoryToController.controlChannel.key.bytes,
                    sizeof session->hap.accessoryToController.controlChannel.key.bytes,
                    "Pair Verify Start Session: EventAccessoryToControllerKey");
        }
        session->hap.accessoryToController.eventChannel.nonce = 0;
    }

    // Copy shared secret.
    // See HomeKit Accessory Protocol Specification R17
    // Section 7.4.7.3 Broadcast Encryption Key Generation
    // See HomeKit Accessory Protocol Specification R17
    // Section 9.1.1 Data Stream Transport Security
    HAPRawBufferCopyBytes(session->hap.cv_KEY, session->state.pairVerify.cv_KEY, sizeof session->hap.cv_KEY);

    // Copy pairing ID.
    session->hap.pairingID = session->state.pairVerify.pairingID;

    // Activate session.
    session->hap.active = true;
    session->hap.timestamp = HAPPlatformClockGetCurrent();

    // Reset Pair Verify procedure.
    HAPPairingPairVerifyReset(session);

    HAPLogInfo(&logObject, "Pair Verify procedure completed (pairing ID %d).", session->hap.pairingID);

    HAPAccessoryServer* server = session->server;

    // Inform application.
    if (server->callbacks.handleSessionAccept) {
        server->callbacks.handleSessionAccept(server, session, server->context);
    }
#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    if (server->transports.ble) {
        HAPNonnull(server->transports.ble)->peripheralManager.handleSessionAccept(server, session);
    }
#endif
}

/**
 * Pair Verify M1 TLVs.
 */
typedef struct {
    HAPTLV* stateTLV;     /**< kTLVType_State. */
    HAPTLV* publicKeyTLV; /**< kTLVType_PublicKey. */

    // Pair Resume.
    HAPTLV* methodTLV;        /**< kTLVType_Method. */
    HAPTLV* sessionIDTLV;     /**< kTLVType_SessionID. */
    HAPTLV* encryptedDataTLV; /**< kTLVType_EncryptedData. */
} HAPPairingPairVerifyM1TLVs;

/**
 * Processes Pair Verify M1.
 *
 * @param      server              Accessory server.
 * @param      session             The session over which the request has been received.
 * @param      scratchBytes         Free memory.
 * @param      numScratchBytes      Length of free memory buffer.
 * @param      tlvs                 TLVs.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If a different request is expected in the current state.
 * @return kHAPError_InvalidData    If the controller sent a malformed request.
 * @return kHAPError_OutOfResources If the free memory buffer does not have enough capacity.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPPairingPairVerifyProcessM1(
        HAPAccessoryServer* server,
        HAPSession* session,
        void* scratchBytes,
        size_t numScratchBytes,
        const HAPPairingPairVerifyM1TLVs* tlvs) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(session->state.pairVerify.state == 1);
    HAPPrecondition(!session->state.pairVerify.error);
    HAPPrecondition(scratchBytes);
    HAPPrecondition(tlvs);
    HAPPrecondition(tlvs->stateTLV);
    HAPPrecondition(tlvs->stateTLV->type == kHAPPairingTLVType_State);
    HAPPrecondition(tlvs->publicKeyTLV);
    HAPPrecondition(tlvs->publicKeyTLV->type == kHAPPairingTLVType_PublicKey);
    HAPPrecondition(tlvs->methodTLV);
    HAPPrecondition(tlvs->methodTLV->type == kHAPPairingTLVType_Method);
    HAPPrecondition(tlvs->sessionIDTLV);
    HAPPrecondition(tlvs->sessionIDTLV->type == kHAPPairingTLVType_SessionID);
    HAPPrecondition(tlvs->encryptedDataTLV);
    HAPPrecondition(tlvs->encryptedDataTLV->type == kHAPPairingTLVType_EncryptedData);

    // See HomeKit Accessory Protocol Specification R17
    // Section 5.8.1 M1: iOS Device -> Accessory - `Verify Start Request'

    HAPLogDebug(&logObject, "Pair Verify M1: Verify Start Request.");

    // Validate kTLVType_State.
    if (!tlvs->stateTLV->value.bytes) {
        HAPLog(&logObject, "Pair Verify M1: kTLVType_State missing.");
        return kHAPError_InvalidData;
    }
    if (tlvs->stateTLV->value.numBytes != 1) {
        HAPLog(&logObject,
               "Pair Verify M1: kTLVType_State has invalid length (%lu).",
               (unsigned long) tlvs->stateTLV->value.numBytes);
        return kHAPError_InvalidData;
    }
    uint8_t state = ((const uint8_t*) tlvs->stateTLV->value.bytes)[0];
    if (state != 1) {
        HAPLog(&logObject, "Pair Verify M1: kTLVType_State invalid: %u.", state);
        return kHAPError_InvalidData;
    }

    // Validate kTLVType_Method.
    uint8_t method = kHAPPairingMethod_PairVerify;
    if (tlvs->methodTLV->value.bytes) {
        if (tlvs->methodTLV->value.numBytes != 1) {
            HAPLog(&logObject,
                   "Pair Verify M1: kTLVType_Method has invalid length (%lu).",
                   (unsigned long) tlvs->methodTLV->value.numBytes);
            return kHAPError_InvalidData;
        }
        method = ((const uint8_t*) tlvs->methodTLV->value.bytes)[0];
        if (method != kHAPPairingMethod_PairResume) {
            HAPLog(&logObject, "Pair Verify M1: kTLVType_Method invalid: %u.", method);
            return kHAPError_InvalidData;
        }

        // Validate kTLVType_SessionID.
        if (!tlvs->sessionIDTLV->value.bytes) {
            HAPLog(&logObject, "Pair Verify M1: kTLVType_SessionID missing.");
            return kHAPError_InvalidData;
        }
        if (tlvs->sessionIDTLV->value.numBytes != sizeof(HAPPairingBLESessionID)) {
            HAPLog(&logObject,
                   "Pair Verify M1: kTLVType_SessionID has invalid length (%lu).",
                   (unsigned long) tlvs->sessionIDTLV->value.numBytes);
            return kHAPError_InvalidData;
        }

        // Validate kTLVType_EncryptedData.
        if (!tlvs->encryptedDataTLV->value.bytes) {
            HAPLog(&logObject, "Pair Verify M1: kTLVType_EncryptedData missing.");
            return kHAPError_InvalidData;
        }
        if (tlvs->encryptedDataTLV->value.numBytes != CHACHA20_POLY1305_TAG_BYTES) {
            HAPLog(&logObject,
                   "Pair Verify M1: kTLVType_EncryptedData has invalid length (%lu).",
                   (unsigned long) tlvs->encryptedDataTLV->value.numBytes);
            return kHAPError_InvalidData;
        }

        if (session->transportType != kHAPTransportType_BLE) {
            HAPLog(&logObject, "Pair Verify M1: Pair Resume requested over non-BLE transport.");
            return kHAPError_InvalidData;
        }
    }

    // Validate kTLVType_PublicKey.
    if (!tlvs->publicKeyTLV->value.bytes) {
        HAPLog(&logObject, "Pair Verify M1: kTLVType_PublicKey missing.");
        return kHAPError_InvalidData;
    }
    if (tlvs->publicKeyTLV->value.numBytes != X25519_BYTES) {
        HAPLog(&logObject,
               "Pair Verify M1: kTLVType_PublicKey has invalid length (%lu).",
               (unsigned long) tlvs->publicKeyTLV->value.numBytes);
        return kHAPError_InvalidData;
    }

    // Store method.
    HAPLogDebug(&logObject, "Pair Verify M1: kTLVType_Method = %u.", method);
    session->state.pairVerify.method = method;

    // Copy public key.
    HAPRawBufferCopyBytes(
            session->state.pairVerify.Controller_cv_PK,
            HAPNonnullVoid(tlvs->publicKeyTLV->value.bytes),
            tlvs->publicKeyTLV->value.numBytes);
    HAPLogBufferDebug(
            &logObject,
            session->state.pairVerify.Controller_cv_PK,
            sizeof session->state.pairVerify.Controller_cv_PK,
            "Pair Verify M1: Controller_cv_PK.");

    // BLE: Handle Pair Resume.
    if (session->state.pairVerify.method == kHAPPairingMethod_PairResume) {
        // See HomeKit Accessory Protocol Specification R17
        // Section 7.3.7.4.1 M1: Controller -> Accessory - Resume Request

        HAPLogDebug(&logObject, "Pair Resume M1: Resume Request.");

        HAPLogBufferDebug(
                &logObject,
                tlvs->sessionIDTLV->value.bytes,
                tlvs->sessionIDTLV->value.numBytes,
                "Pair Resume M1: kTLVType_SessionID.");

        session->state.pairVerify.pairingID = -1;
#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
        if (server->transports.ble) {
            HAPNonnull(server->transports.ble)
                    ->sessionCache.fetch(
                            server,
                            HAPNonnullVoid(tlvs->sessionIDTLV->value.bytes),
                            session->state.pairVerify.cv_KEY,
                            &session->state.pairVerify.pairingID);
        }
#endif

        if (session->state.pairVerify.pairingID >= 0) {
            HAPLogSensitiveBufferDebug(
                    &logObject,
                    session->state.pairVerify.cv_KEY,
                    sizeof session->state.pairVerify.cv_KEY,
                    "Pair Resume M1: cv_KEY.");

            void* key = HAPTLVScratchBufferAlloc(&scratchBytes, &numScratchBytes, CHACHA20_POLY1305_KEY_BYTES);
            void* salt = HAPTLVScratchBufferAlloc(&scratchBytes, &numScratchBytes, X25519_BYTES);
            void* sessionID =
                    HAPTLVScratchBufferAllocUnaligned(&scratchBytes, &numScratchBytes, sizeof(HAPPairingBLESessionID));
            if (!key || !salt || !sessionID) {
                HAPLog(&logObject, "Pair Resume M1: Not enough memory to allocate RequestKey / PublicKey / SessionID.");
                // The session cache fetch cleared the corresponding cache entry.
                // Failure to proceed with pair resume requires releasing corresponding resources.
                HAPDataStreamInvalidateAllForHAPPairingID(server, session->state.pairVerify.pairingID);
                return kHAPError_OutOfResources;
            }

            // Derive request encryption key.
            HAPRawBufferCopyBytes(
                    salt, HAPNonnullVoid(tlvs->publicKeyTLV->value.bytes), tlvs->publicKeyTLV->value.numBytes);
            HAPRawBufferCopyBytes(
                    sessionID, HAPNonnullVoid(tlvs->sessionIDTLV->value.bytes), tlvs->sessionIDTLV->value.numBytes);
            HAPLogSensitiveBufferDebug(
                    &logObject, salt, X25519_BYTES + sizeof(HAPPairingBLESessionID), "Pair Resume M1: Salt.");
            static const uint8_t info[] = "Pair-Resume-Request-Info";
            HAP_hkdf_sha512(
                    key,
                    CHACHA20_POLY1305_KEY_BYTES,
                    session->state.pairVerify.cv_KEY,
                    sizeof session->state.pairVerify.cv_KEY,
                    salt,
                    X25519_BYTES + sizeof(HAPPairingBLESessionID),
                    info,
                    sizeof info - 1);
            HAPLogSensitiveBufferDebug(&logObject, key, CHACHA20_POLY1305_KEY_BYTES, "Pair Resume M1: RequestKey.");

            // Decrypt data.
            HAPLogBufferDebug(
                    &logObject,
                    tlvs->encryptedDataTLV->value.bytes,
                    tlvs->encryptedDataTLV->value.numBytes,
                    "Pair Resume M1: kTLVType_EncryptedData.");
            static const uint8_t nonce[] = "PR-Msg01";
            int e = HAP_chacha20_poly1305_decrypt(
                    tlvs->encryptedDataTLV->value.bytes, NULL, NULL, 0, nonce, sizeof nonce - 1, key);
            if (e) {
                HAPAssert(e == -1);
                HAPLog(&logObject, "Pair Resume M1: Failed to verify auth tag of kTLVType_EncryptedData.");
                session->state.pairVerify.error = kHAPPairingError_Authentication;
                // The session cache fetch cleared the corresponding cache entry.
                // Failure to proceed with pair resume requires releasing corresponding resources.
                HAPDataStreamInvalidateAllForHAPPairingID(server, session->state.pairVerify.pairingID);
                return kHAPError_None;
            }
        } else {
            // Not found. Fall back to Pair Verify.
            HAPLog(&logObject, "Pair Resume M1: Pair Resume Shared Secret not found. Falling back to Pair Verify.");
            session->state.pairVerify.method = kHAPPairingMethod_PairVerify;
        }
    }

    return kHAPError_None;
}

/**
 * Processes Pair Verify M2.
 *
 * @param      server              Accessory server.
 * @param      session             The session over which the response will be sent.
 * @param      responseWriter       TLV writer for serializing the response.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If persistent store access failed.
 * @return kHAPError_InvalidState   If a different request is expected in the current state.
 * @return kHAPError_OutOfResources If response writer does not have enough capacity.
 */
HAP_RESULT_USE_CHECK
static HAPError
        HAPPairingPairVerifyGetM2(HAPAccessoryServer* server, HAPSession* session, HAPTLVWriter* responseWriter) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(session->state.pairVerify.state == 2);
    HAPPrecondition(!session->state.pairVerify.error);
    HAPPrecondition(responseWriter);

    HAPError err;

    // See HomeKit Accessory Protocol Specification R17
    // Section 5.8.2 M2: Accessory -> iOS Device - `Verify Start Response'

    HAPLogDebug(&logObject, "Pair Verify M2: Verify Start Response.");

    // Create new, random key pair.
    HAPPlatformRandomNumberFill(session->state.pairVerify.cv_SK, sizeof session->state.pairVerify.cv_SK);
    HAP_X25519_scalarmult_base(session->state.pairVerify.cv_PK, session->state.pairVerify.cv_SK);
    HAPLogSensitiveBufferDebug(
            &logObject,
            session->state.pairVerify.cv_SK,
            sizeof session->state.pairVerify.cv_SK,
            "Pair Verify M2: cv_SK.");
    HAPLogBufferDebug(
            &logObject,
            session->state.pairVerify.cv_PK,
            sizeof session->state.pairVerify.cv_PK,
            "Pair Verify M2: cv_PK.");

    // Generate the shared secret.
    HAP_X25519_scalarmult(
            session->state.pairVerify.cv_KEY,
            session->state.pairVerify.cv_SK,
            session->state.pairVerify.Controller_cv_PK);
    HAPLogSensitiveBufferDebug(
            &logObject,
            session->state.pairVerify.cv_KEY,
            sizeof session->state.pairVerify.cv_KEY,
            "Pair Verify M2: cv_KEY.");

    // kTLVType_State.
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPairingTLVType_State,
                              .value = { .bytes = &session->state.pairVerify.state, .numBytes = 1 } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // kTLVType_PublicKey.
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPairingTLVType_PublicKey,
                              .value = { .bytes = session->state.pairVerify.cv_PK,
                                         .numBytes = sizeof session->state.pairVerify.cv_PK } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // Construct sub-TLV writer.
    HAPTLVWriter subWriter;
    {
        void* bytes;
        size_t maxBytes;
        HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);
        if (maxBytes < CHACHA20_POLY1305_TAG_BYTES) {
            HAPLog(&logObject, "Pair Verify M2: Not enough memory for kTLVType_EncryptedData auth tag.");
            return kHAPError_OutOfResources;
        }
        maxBytes -= CHACHA20_POLY1305_TAG_BYTES;
        HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
    }

    // kTLVType_Identifier.
    HAPDeviceIDString deviceIDString;
    err = HAPDeviceIDGetAsString(server, &deviceIDString);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }
    size_t numDeviceIDStringBytes = HAPStringGetNumBytes(deviceIDString.stringValue);
    err = HAPTLVWriterAppend(
            &subWriter,
            &(const HAPTLV) { .type = kHAPPairingTLVType_Identifier,
                              .value = { .bytes = deviceIDString.stringValue, .numBytes = numDeviceIDStringBytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // kTLVType_Signature.
    {
        void* bytes;
        size_t maxBytes;
        HAPTLVWriterGetScratchBytes(&subWriter, &bytes, &maxBytes);

        void* accessoryCvPK = HAPTLVScratchBufferAlloc(&bytes, &maxBytes, X25519_BYTES);
        void* accessoryPairingID = HAPTLVScratchBufferAllocUnaligned(&bytes, &maxBytes, numDeviceIDStringBytes);
        void* iOSDeviceCvPK = HAPTLVScratchBufferAllocUnaligned(&bytes, &maxBytes, X25519_BYTES);
        void* signature = HAPTLVScratchBufferAlloc(&bytes, &maxBytes, ED25519_BYTES);
        if (!accessoryCvPK || !accessoryPairingID || !iOSDeviceCvPK || !signature) {
            HAPLog(&logObject,
                   "Pair Verify M2: Not enough memory to allocate "
                   "AccessoryCvPK / AccessoryPairingID / iOSDeviceCvPK / Signature.");
            return kHAPError_OutOfResources;
        }

        // Construct AccessoryInfo: AccessoryCvPK, AccessoryPairingID, iOSDeviceCvPK.
        HAPRawBufferCopyBytes(accessoryCvPK, session->state.pairVerify.cv_PK, sizeof session->state.pairVerify.cv_PK);
        HAPRawBufferCopyBytes(accessoryPairingID, deviceIDString.stringValue, numDeviceIDStringBytes);
        HAPRawBufferCopyBytes(
                iOSDeviceCvPK,
                session->state.pairVerify.Controller_cv_PK,
                sizeof session->state.pairVerify.Controller_cv_PK);

        // Finalize info.
        void* infoBytes = accessoryCvPK;
        size_t numInfoBytes = X25519_BYTES + numDeviceIDStringBytes + X25519_BYTES;

        {
            HAP_ed25519_sign(
                    signature, infoBytes, numInfoBytes, server->identity.ed_LTSK.bytes, server->identity.ed_LTPK);
        }
        HAPLogSensitiveBufferDebug(&logObject, infoBytes, numInfoBytes, "Pair Verify M2: AccessoryInfo");
        HAPLogSensitiveBufferDebug(&logObject, signature, ED25519_BYTES, "Pair Verify M2: kTLVType_Signature");

        // kTLVType_Signature.
        err = HAPTLVWriterAppend(
                &subWriter,
                &(const HAPTLV) { .type = kHAPPairingTLVType_Signature,
                                  .value = { .bytes = signature, .numBytes = ED25519_BYTES } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
    }

    // Derive the symmetric session encryption key.
    static const uint8_t salt[] = "Pair-Verify-Encrypt-Salt";
    static const uint8_t info[] = "Pair-Verify-Encrypt-Info";
    HAP_hkdf_sha512(
            session->state.pairVerify.SessionKey,
            sizeof session->state.pairVerify.SessionKey,
            session->state.pairVerify.cv_KEY,
            sizeof session->state.pairVerify.cv_KEY,
            salt,
            sizeof salt - 1,
            info,
            sizeof info - 1);
    HAPLogSensitiveBufferDebug(
            &logObject,
            session->state.pairVerify.SessionKey,
            sizeof session->state.pairVerify.SessionKey,
            "Pair Verify M2: SessionKey");

    // Encrypt the sub-TLV.
    void* bytes;
    size_t numBytes;
    HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
    static const uint8_t nonce[] = "PV-Msg02";
    HAP_chacha20_poly1305_encrypt(
            &((uint8_t*) bytes)[numBytes],
            bytes,
            bytes,
            numBytes,
            nonce,
            sizeof nonce - 1,
            session->state.pairVerify.SessionKey);
    numBytes += CHACHA20_POLY1305_TAG_BYTES;
    HAPLogBufferDebug(&logObject, bytes, numBytes, "Pair Verify M2: kTLVType_EncryptedData.");

    // kTLVType_EncryptedData.
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPairingTLVType_EncryptedData,
                              .value = { .bytes = bytes, .numBytes = numBytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    return kHAPError_None;
}

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
/**
 * Processes Pair Resume M2.
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
static HAPError HAPPairingPairVerifyGetM2ForBLEPairResume(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPTLVWriter* responseWriter) {
    HAPPrecondition(server);
    HAPPrecondition(server->transports.ble);
    HAPPrecondition(session);
    HAPPrecondition(session->transportType == kHAPTransportType_BLE);
    HAPPrecondition(session->state.pairVerify.state == 2);
    HAPPrecondition(!session->state.pairVerify.error);
    HAPPrecondition(responseWriter);

    HAPError err;

    // See HomeKit Accessory Protocol Specification R17
    // Section 7.3.7.4.2 M2: Accessory -> Controller - Resume Response

    HAPLogDebug(&logObject, "Pair Resume M2: Resume Response.");

    void* bytes;
    size_t maxBytes;
    HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);

    void* key = HAPTLVScratchBufferAlloc(&bytes, &maxBytes, CHACHA20_POLY1305_KEY_BYTES);
    void* salt = HAPTLVScratchBufferAlloc(&bytes, &maxBytes, X25519_BYTES);
    void* sessionID = HAPTLVScratchBufferAllocUnaligned(&bytes, &maxBytes, sizeof(HAPPairingBLESessionID));
    if (!key || !salt || !sessionID) {
        HAPLog(&logObject, "Pair Resume M2: Not enough memory to allocate ResponseKey / PublicKey / SessionID.");
        return kHAPError_OutOfResources;
    }

    // Generate new session ID.
    HAPPlatformRandomNumberFill(sessionID, sizeof(HAPPairingBLESessionID));

    // Derive response encryption key.
    HAPRawBufferCopyBytes(
            salt, session->state.pairVerify.Controller_cv_PK, sizeof session->state.pairVerify.Controller_cv_PK);
    HAPLogSensitiveBufferDebug(
            &logObject, salt, X25519_BYTES + sizeof(HAPPairingBLESessionID), "Pair Resume M2: Salt.");
    {
        static const uint8_t info[] = "Pair-Resume-Response-Info";
        HAP_hkdf_sha512(
                key,
                CHACHA20_POLY1305_KEY_BYTES,
                session->state.pairVerify.cv_KEY,
                sizeof session->state.pairVerify.cv_KEY,
                salt,
                X25519_BYTES + sizeof(HAPPairingBLESessionID),
                info,
                sizeof info - 1);
        HAPLogSensitiveBufferDebug(&logObject, key, CHACHA20_POLY1305_KEY_BYTES, "Pair Resume M2: ResponseKey.");
    }

    // Encrypt empty data.
    uint8_t tag[CHACHA20_POLY1305_TAG_BYTES];
    static const uint8_t nonce[] = "PR-Msg02";
    HAP_chacha20_poly1305_encrypt(tag, NULL, NULL, 0, nonce, sizeof nonce - 1, key);

    // Store shared secret associated with session being resumed.
    uint8_t resumedSessionSecret[X25519_BYTES];
    HAPRawBufferCopyBytes(resumedSessionSecret, session->state.pairVerify.cv_KEY, sizeof resumedSessionSecret);

    // Generate new shared secret.
    // See HomeKit Accessory Protocol Specification R17
    // Section 7.3.7.5 Compute Shared Secret
    {
        static const uint8_t info[] = "Pair-Resume-Shared-Secret-Info";
        HAP_hkdf_sha512(
                session->state.pairVerify.cv_KEY,
                sizeof session->state.pairVerify.cv_KEY,
                session->state.pairVerify.cv_KEY,
                sizeof session->state.pairVerify.cv_KEY,
                salt,
                X25519_BYTES + sizeof(HAPPairingBLESessionID),
                info,
                sizeof info - 1);
        HAPLogSensitiveBufferDebug(
                &logObject,
                session->state.pairVerify.cv_KEY,
                sizeof session->state.pairVerify.cv_KEY,
                "Pair Resume M2: cv_KEY.");
    }

    int evictedSessionPairingID = -1;
    // Save shared secret.
    HAPNonnull(server->transports.ble)
            ->sessionCache.save(
                    server,
                    sessionID,
                    session->state.pairVerify.cv_KEY,
                    session->state.pairVerify.pairingID,
                    &evictedSessionPairingID);

    // If no session is found in the cache while processing the Pair Resume M1 request, we fall back to Pair Verify.
    // If the session is found in the cache, the corresponding cache entry is cleared. So the save operation here
    // should have found an available cache entry.
    HAPAssert(evictedSessionPairingID == -1);

    // kTLVType_State.
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPairingTLVType_State,
                              .value = { .bytes = &session->state.pairVerify.state, .numBytes = 1 } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // kTLVType_Method.
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPairingTLVType_Method,
                              .value = { .bytes = &session->state.pairVerify.method, .numBytes = 1 } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // kTLVType_SessionID.
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPairingTLVType_SessionID,
                              .value = { .bytes = sessionID, .numBytes = sizeof(HAPPairingBLESessionID) } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // kTLVType_EncryptedData.
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPairingTLVType_EncryptedData,
                              .value = { .bytes = tag, .numBytes = CHACHA20_POLY1305_TAG_BYTES } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // Update HDS state associated with resumed session.
    HAPDataStreamPrepareSessionResume(server, resumedSessionSecret, session);

    // Start HAP session.
    HAPPairingPairVerifyStartSession(session);
    return kHAPError_None;
}
#endif

/**
 * Pair Verify M3 TLVs.
 */
typedef struct {
    HAPTLV* stateTLV;         /**< kTLVType_State. */
    HAPTLV* encryptedDataTLV; /**< kTLVType_EncryptedData. */
} HAPPairingPairVerifyM3TLVs;

/**
 * Processes Pair Verify M3.
 *
 * @param      server              Accessory server.
 * @param      session             The session over which the request has been received.
 * @param      scratchBytes         Free memory.
 * @param      numScratchBytes      Length of free memory buffer.
 * @param      tlvs                 TLVs.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If a different request is expected in the current state.
 * @return kHAPError_InvalidData    If the controller sent a malformed request.
 * @return kHAPError_OutOfResources If the free memory buffer does not have enough capacity.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPPairingPairVerifyProcessM3(
        HAPAccessoryServer* server,
        HAPSession* session,
        void* scratchBytes,
        size_t numScratchBytes,
        const HAPPairingPairVerifyM3TLVs* tlvs) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(session->state.pairVerify.state == 3);
    HAPPrecondition(!session->state.pairVerify.error);
    HAPPrecondition(scratchBytes);
    HAPPrecondition(tlvs);
    HAPPrecondition(tlvs->stateTLV);
    HAPPrecondition(tlvs->stateTLV->type == kHAPPairingTLVType_State);
    HAPPrecondition(tlvs->encryptedDataTLV);
    HAPPrecondition(tlvs->encryptedDataTLV->type == kHAPPairingTLVType_EncryptedData);

    HAPError err;

    // See HomeKit Accessory Protocol Specification R17
    // Section 5.8.3 M3: iOS Device -> Accessory - `Verify Finish Request'

    HAPLogDebug(&logObject, "Pair Verify M3: Verify Finish Request.");

    // Validate kTLVType_State.
    if (!tlvs->stateTLV->value.bytes) {
        HAPLog(&logObject, "Pair Verify M3: kTLVType_State missing.");
        return kHAPError_InvalidData;
    }
    if (tlvs->stateTLV->value.numBytes != 1) {
        HAPLog(&logObject,
               "Pair Setup M3: kTLVType_State has invalid length (%lu).",
               (unsigned long) tlvs->stateTLV->value.numBytes);
        return kHAPError_InvalidData;
    }
    uint8_t state = ((const uint8_t*) tlvs->stateTLV->value.bytes)[0];
    if (state != 3) {
        HAPLog(&logObject, "Pair Verify M3: kTLVType_State invalid: %u.", state);
        return kHAPError_InvalidData;
    }

    // Validate kTLVType_EncryptedData.
    if (!tlvs->encryptedDataTLV->value.bytes) {
        HAPLog(&logObject, "Pair Verify M3: kTLVType_EncryptedData missing.");
        return kHAPError_InvalidData;
    }
    if (tlvs->encryptedDataTLV->value.numBytes < CHACHA20_POLY1305_TAG_BYTES) {
        HAPLog(&logObject,
               "Pair Verify M3: kTLVType_EncryptedData has invalid length (%lu).",
               (unsigned long) tlvs->encryptedDataTLV->value.numBytes);
        return kHAPError_InvalidData;
    }

    // Verify auth tag and decrypt.
    HAPLogBufferDebug(
            &logObject,
            tlvs->encryptedDataTLV->value.bytes,
            tlvs->encryptedDataTLV->value.numBytes,
            "Pair Verify M3: kTLVType_EncryptedData.");
    void* bytes = (void*) (uintptr_t) tlvs->encryptedDataTLV->value.bytes;
    size_t numBytes = tlvs->encryptedDataTLV->value.numBytes - CHACHA20_POLY1305_TAG_BYTES;
    static const uint8_t nonce[] = "PV-Msg03";
    int e = HAP_chacha20_poly1305_decrypt(
            &((uint8_t*) bytes)[numBytes],
            bytes,
            bytes,
            numBytes,
            nonce,
            sizeof nonce - 1,
            session->state.pairVerify.SessionKey);
    if (e) {
        HAPAssert(e == -1);
        HAPLog(&logObject, "Pair Verify M3: Failed to decrypt kTLVType_EncryptedData.");
        session->state.pairVerify.error = kHAPPairingError_Authentication;
        return kHAPError_None;
    }

    // Parse sub-TLV.
    HAPTLV identifierTLV, signatureTLV;
    identifierTLV.type = kHAPPairingTLVType_Identifier;
    signatureTLV.type = kHAPPairingTLVType_Signature;
    {
        HAPTLVReader subReader;
        HAPTLVReaderCreate(&subReader, bytes, numBytes);

        err = HAPTLVReaderGetAll(&subReader, (HAPTLV* const[]) { &identifierTLV, &signatureTLV, NULL });
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            return err;
        }

        // Validate kTLVType_Identifier.
        if (!identifierTLV.value.bytes) {
            HAPLog(&logObject, "Pair Verify M3: kTLVType_Identifier missing.");
            return kHAPError_InvalidData;
        }
        if (identifierTLV.value.numBytes > sizeof(HAPPairingID)) {
            HAPLog(&logObject,
                   "Pair Verify M3: kTLVType_Identifier has invalid length (%lu).",
                   (unsigned long) identifierTLV.value.numBytes);
            return kHAPError_InvalidData;
        }

        // Validate kTLVType_Signature.
        if (!signatureTLV.value.bytes) {
            HAPLog(&logObject, "Pair Verify M3: kTLVType_Signature missing.");
            return kHAPError_InvalidData;
        }
        if (signatureTLV.value.numBytes != ED25519_BYTES) {
            HAPLog(&logObject,
                   "Pair Verify M3: kTLVType_Signature has invalid length (%lu).",
                   (unsigned long) signatureTLV.value.numBytes);
            return kHAPError_InvalidData;
        }
    }

    // Fetch pairing ID.
    HAPPairingID identifier;
    HAPAssert(identifierTLV.value.numBytes <= sizeof identifier.bytes);
    HAPRawBufferCopyBytes(identifier.bytes, HAPNonnullVoid(identifierTLV.value.bytes), identifierTLV.value.numBytes);
    size_t numIdentifierBytes = identifierTLV.value.numBytes;

    // Find pairing.
    bool found;
    HAPPairing pairing;
    HAPPairingIndex pairingIndex;
    err = HAPPairingFind(server, &identifier, numIdentifierBytes, &pairing, &pairingIndex, &found);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }
    if (!found) {
        // Pairing not found.
        HAPLog(&logObject, "Pair Verify M3: Pairing not found.");
        session->state.pairVerify.error = kHAPPairingError_Authentication;
        return kHAPError_None;
    }

    session->state.pairVerify.pairingID = (int) pairingIndex;

    void* iOSDeviceCvPK = HAPTLVScratchBufferAlloc(&scratchBytes, &numScratchBytes, X25519_BYTES);
    void* iOSDevicePairingID =
            HAPTLVScratchBufferAllocUnaligned(&scratchBytes, &numScratchBytes, identifierTLV.value.numBytes);
    void* accessoryCvPK = HAPTLVScratchBufferAllocUnaligned(&scratchBytes, &numScratchBytes, X25519_BYTES);
    if (!iOSDeviceCvPK || !iOSDevicePairingID || !accessoryCvPK) {
        HAPLog(&logObject,
               "Pair Verify M3: Not enough memory to allocate"
               " iOSDeviceCvPK / iOSDevicePairingID / AccessoryCvPK.");
        return kHAPError_OutOfResources;
    }

    // Construct iOSDeviceInfo: iOSDeviceCvPK, iOSDevicePairingID, AccessoryCvPK.
    HAPRawBufferCopyBytes(
            iOSDeviceCvPK,
            session->state.pairVerify.Controller_cv_PK,
            sizeof session->state.pairVerify.Controller_cv_PK);
    HAPRawBufferCopyBytes(iOSDevicePairingID, HAPNonnullVoid(identifierTLV.value.bytes), identifierTLV.value.numBytes);
    HAPRawBufferCopyBytes(accessoryCvPK, session->state.pairVerify.cv_PK, sizeof session->state.pairVerify.cv_PK);

    // Finalize info.
    void* infoBytes = iOSDeviceCvPK;
    size_t numInfoBytes = X25519_BYTES + identifierTLV.value.numBytes + X25519_BYTES;
    HAPLogSensitiveBufferDebug(&logObject, infoBytes, numInfoBytes, "Pair Verify M3: iOSDeviceInfo.");

    // Verify signature.
    HAPLogSensitiveBufferDebug(
            &logObject, signatureTLV.value.bytes, signatureTLV.value.numBytes, "Pair Verify M3: kTLVType_Signature.");
    e = HAP_ed25519_verify(signatureTLV.value.bytes, infoBytes, numInfoBytes, pairing.publicKey.value);
    if (e) {
        HAPAssert(e == -1);
        HAPLog(&logObject, "Pair Verify M3: iOSDeviceInfo signature is incorrect.");
        session->state.pairVerify.error = kHAPPairingError_Authentication;
        return kHAPError_None;
    }

    return kHAPError_None;
}

/**
 * Processes Pair Verify M4.
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
static HAPError
        HAPPairingPairVerifyGetM4(HAPAccessoryServer* server, HAPSession* session, HAPTLVWriter* responseWriter) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(session->state.pairVerify.state == 4);
    HAPPrecondition(!session->state.pairVerify.error);
    HAPPrecondition(responseWriter);

    HAPError err;

    // See HomeKit Accessory Protocol Specification R17
    // Section 5.8.4 M4: Accessory -> iOS Device - `Verify Finish Response'

    HAPLogDebug(&logObject, "Pair Verify M4: Verify Finish Response.");

    // kTLVType_State.
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPairingTLVType_State,
                              .value = { .bytes = &session->state.pairVerify.state, .numBytes = 1 } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    // BLE: Handle Pair Resume.
    if (session->transportType == kHAPTransportType_BLE) {
        // See HomeKit Accessory Protocol Specification R17
        // Section 7.3.7.3 Initial SessionID
        HAPAssert(server->transports.ble);

        void* bytes;
        size_t maxBytes;
        HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);

        void* sessionID = HAPTLVScratchBufferAllocUnaligned(&bytes, &maxBytes, sizeof(HAPPairingBLESessionID));
        if (!sessionID) {
            HAPLog(&logObject, "Pair Verify M4: Not enough memory to allocate initial SessionID.");
            return kHAPError_OutOfResources;
        }

        // Derive initial session ID.
        static const uint8_t salt[] = "Pair-Verify-ResumeSessionID-Salt";
        static const uint8_t info[] = "Pair-Verify-ResumeSessionID-Info";
        HAP_hkdf_sha512(
                sessionID,
                sizeof(HAPPairingBLESessionID),
                session->state.pairVerify.cv_KEY,
                sizeof session->state.pairVerify.cv_KEY,
                salt,
                sizeof salt - 1,
                info,
                sizeof info - 1);
        HAPLogSensitiveBufferDebug(
                &logObject, sessionID, sizeof(HAPPairingBLESessionID), "Pair Verify M4: ResumeSessionID.");

        int evictedSessionPairingID = -1;
        // Save shared secret.
        HAPNonnull(server->transports.ble)
                ->sessionCache.save(
                        server,
                        sessionID,
                        session->state.pairVerify.cv_KEY,
                        session->state.pairVerify.pairingID,
                        &evictedSessionPairingID);

        // Invalidate attached HomeKit Data Streams for evicted pairing.
        if (evictedSessionPairingID != -1) {
            HAPDataStreamInvalidateAllForHAPPairingID(server, evictedSessionPairingID);
        }
    }
#endif

    // Start HAP session.
    HAPPairingPairVerifyStartSession(session);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPairingPairVerifyHandleWrite(HAPAccessoryServer* server, HAPSession* session, HAPTLVReader* requestReader) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(requestReader);

    HAPError err;

    // Parse request.
    HAPTLV stateTLV, publicKeyTLV, methodTLV, sessionIDTLV, encryptedDataTLV;
    stateTLV.type = kHAPPairingTLVType_State;
    publicKeyTLV.type = kHAPPairingTLVType_PublicKey;
    methodTLV.type = kHAPPairingTLVType_Method;
    sessionIDTLV.type = kHAPPairingTLVType_SessionID;
    encryptedDataTLV.type = kHAPPairingTLVType_EncryptedData;

    // Each time we receive a verify message, renew our wakelock.  This will either timeout or be stopped
    // when the verify is successful
    if (session->transportType == kHAPTransportType_Thread) {
        HAPPlatformThreadAddWakeLock(&kPairVerifyWakeLock, PAIR_VERIFY_WAKELOCK_TIMEOUT_MS);
    }
    err = HAPTLVReaderGetAll(
            requestReader,
            (HAPTLV* const[]) { &stateTLV, &publicKeyTLV, &methodTLV, &sessionIDTLV, &encryptedDataTLV, NULL });
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        HAPPairingPairVerifyReset(session);
        return err;
    }

    // Get free memory.
    void* bytes;
    size_t maxBytes;
    HAPTLVReaderGetScratchBytes(requestReader, &bytes, &maxBytes);

    // If a subsequent Pair Verify request from the same controller occurs
    // in the middle of the Pair Verify procedure then the accessory must
    // immediately tear down the existing session with the controller and
    // must accept the newest request.
    // See HomeKit Accessory Protocol Specification R17
    // Section 5.8.4 M4: Accessory -> iOS Device - `Verify Finish Response'
    if (stateTLV.value.bytes && stateTLV.value.numBytes == 1 && ((const uint8_t*) stateTLV.value.bytes)[0] == 1) {
        HAPPairingPairVerifyReset(session);
    }

    // Process request.
    switch (session->state.pairVerify.state) {
        case 0: {
            session->state.pairVerify.state++;
            err = HAPPairingPairVerifyProcessM1(
                    server,
                    session,
                    bytes,
                    maxBytes,
                    &(const HAPPairingPairVerifyM1TLVs) { .stateTLV = &stateTLV,
                                                          .publicKeyTLV = &publicKeyTLV,
                                                          .methodTLV = &methodTLV,
                                                          .sessionIDTLV = &sessionIDTLV,
                                                          .encryptedDataTLV = &encryptedDataTLV });
            if (err) {
                HAPAssert(err == kHAPError_InvalidData || err == kHAPError_OutOfResources);
            }
            break;
        }
        case 2: {
            session->state.pairVerify.state++;
            err = HAPPairingPairVerifyProcessM3(
                    server,
                    session,
                    bytes,
                    maxBytes,
                    &(const HAPPairingPairVerifyM3TLVs) { .stateTLV = &stateTLV,
                                                          .encryptedDataTLV = &encryptedDataTLV });
            if (err) {
                HAPAssert(err == kHAPError_InvalidData || err == kHAPError_OutOfResources);
            }
            break;
        }
        default: {
            HAPLog(&logObject, "Received unexpected Pair Verify write in state M%d.", session->state.pairVerify.state);
            err = kHAPError_InvalidState;
            break;
        }
    }
    if (err) {
        HAPPairingPairVerifyReset(session);
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
static HAPError HAPPairingPairVerifyGetErrorResponse(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPTLVWriter* responseWriter) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(responseWriter);
    HAPPrecondition(session->state.pairVerify.error);

    HAPError err;

    // kTLVType_State.
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPairingTLVType_State,
                              .value = { .bytes = &session->state.pairVerify.state, .numBytes = 1 } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // kTLVType_Error.
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPairingTLVType_Error,
                              .value = { .bytes = &session->state.pairVerify.error, .numBytes = 1 } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPairingPairVerifyHandleRead(HAPAccessoryServer* server, HAPSession* session, HAPTLVWriter* responseWriter) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(responseWriter);

    HAPError err;

    // Handle pending error.
    if (session->state.pairVerify.error) {
        // Advance state.
        session->state.pairVerify.state++;

        err = HAPPairingPairVerifyGetErrorResponse(server, session, responseWriter);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            HAPPairingPairVerifyReset(session);
            return err;
        }

        // Reset session.
        HAPPairingPairVerifyReset(session);
        return kHAPError_None;
    }

    // Process request.
    switch (session->state.pairVerify.state) {
        case 1: {
            session->state.pairVerify.state++;
            if (session->state.pairVerify.method == kHAPPairingMethod_PairResume) {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
                err = HAPPairingPairVerifyGetM2ForBLEPairResume(server, session, responseWriter);
#else
                err = kHAPError_InvalidState;
#endif
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                }
            } else {
                err = HAPPairingPairVerifyGetM2(server, session, responseWriter);
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                }
            }
            break;
        }
        case 3: {
            session->state.pairVerify.state++;
            err = HAPPairingPairVerifyGetM4(server, session, responseWriter);
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
            }
            break;
        }
        default: {
            HAPLog(&logObject, "Received unexpected Pair Verify read in state M%u.", session->state.pairVerify.state);
            err = kHAPError_InvalidState;
            break;
        }
    }
    if (err) {
        HAPPairingPairVerifyReset(session);
        return err;
    }

    // Handle pending error.
    if (session->state.pairVerify.error) {
        err = HAPPairingPairVerifyGetErrorResponse(server, session, responseWriter);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            HAPPairingPairVerifyReset(session);
            return err;
        }

        // Reset session.
        HAPPairingPairVerifyReset(session);
        return kHAPError_None;
    }

    return kHAPError_None;
}
