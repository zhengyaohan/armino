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

#include "HAPPairingPairSetup.h"

#include "HAP+KeyValueStoreDomains.h"
#include "HAPAccessory.h"
#include "HAPAccessoryServer+Internal.h"
#include "HAPAccessorySetupInfo.h"
#include "HAPBLEAccessoryServer.h"
#include "HAPDeviceID.h"
#include "HAPLogSubsystem.h"
#include "HAPMFiAuth.h"
#include "HAPTLV+Internal.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "PairingPairSetup" };

static HAPPlatformThreadWakeLock kPairSetupWakeLock;
#define PAIR_SETUP_WAKELOCK_TIMEOUT_MS 5000

void HAPPairingPairSetupResetForSession(HAPAccessoryServer* server, HAPSession* session) {
    HAPPrecondition(server);
    HAPPrecondition(session);

    // Reset session-specific Pair Setup procedure state that is stored in shared memory.
    if (server->pairSetup.sessionThatIsCurrentlyPairing == session) {
        bool keepSetupInfo = server->pairSetup.keepSetupInfo;
        HAPRawBufferZero(&server->pairSetup, sizeof server->pairSetup);
        HAPAccessorySetupInfoHandlePairingStop(server, keepSetupInfo);
    }

    // Reset session-specific Pair Setup procedure state.
    HAPRawBufferZero(&session->state.pairSetup, sizeof session->state.pairSetup);
}

/**
 * Pair Setup M1 TLVs.
 */
typedef struct {
    HAPTLV* stateTLV;  /**< kTLVType_State. */
    HAPTLV* methodTLV; /**< kTLVType_Method. */
    HAPTLV* flagsTLV;  /**< kTLVType_Flags. */
} HAPPairingPairSetupM1TLVs;

/**
 * Processes Pair Setup M1.
 *
 * @param      server              Accessory server.
 * @param      session             The session over which the request has been received.
 * @param      tlvs                 TLVs.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If a different request is expected in the current state.
 * @return kHAPError_InvalidData    If the controller sent a malformed request.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPPairingPairSetupProcessM1(
        HAPAccessoryServer* server,
        HAPSession* session,
        const HAPPairingPairSetupM1TLVs* tlvs) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(session->state.pairSetup.state == 1);
    HAPPrecondition(!session->state.pairSetup.error);
    HAPPrecondition(tlvs);
    HAPPrecondition(tlvs->stateTLV);
    HAPPrecondition(tlvs->stateTLV->type == kHAPPairingTLVType_State);
    HAPPrecondition(tlvs->methodTLV);
    HAPPrecondition(tlvs->methodTLV->type == kHAPPairingTLVType_Method);
    HAPPrecondition(tlvs->flagsTLV);
    HAPPrecondition(tlvs->flagsTLV->type == kHAPPairingTLVType_Flags);

    // See HomeKit Accessory Protocol Specification R17
    // Section 5.7.1 M1: iOS Device -> Accessory - `SRP Start Request'

    HAPLogDebug(&logObject, "Pair Setup M1: SRP Start Request.");

    // Validate kTLVType_State.
    if (!tlvs->stateTLV->value.bytes) {
        HAPLog(&logObject, "Pair Setup M1: kTLVType_State missing.");
        return kHAPError_InvalidData;
    }
    if (tlvs->stateTLV->value.numBytes != 1) {
        HAPLog(&logObject, "Pair Setup M1: kTLVType_State has invalid length (%zu).", tlvs->stateTLV->value.numBytes);
        return kHAPError_InvalidData;
    }
    uint8_t state = ((const uint8_t*) tlvs->stateTLV->value.bytes)[0];
    if (state != 1) {
        HAPLog(&logObject, "Pair Setup M1: kTLVType_State invalid: %u.", state);
        return kHAPError_InvalidData;
    }

    // Validate kTLVType_Method.
    if (!tlvs->methodTLV->value.bytes) {
        HAPLog(&logObject, "Pair Setup M1: kTLVType_Method missing.");
        return kHAPError_InvalidData;
    }
    if (tlvs->methodTLV->value.numBytes != 1) {
        HAPLog(&logObject, "Pair Setup M1: kTLVType_Method has invalid length (%zu).", tlvs->methodTLV->value.numBytes);
        return kHAPError_InvalidData;
    }
    uint8_t method = ((const uint8_t*) tlvs->methodTLV->value.bytes)[0];
    if (method != kHAPPairingMethod_PairSetupWithAuth && method != kHAPPairingMethod_PairSetup) {
        HAPLog(&logObject, "Pair Setup M1: kTLVType_Method invalid: %u.", method);
        return kHAPError_InvalidData;
    }

    // Store method.
    HAPLogDebug(&logObject, "Pair Setup M1: kTLVType_Method = %u.", method);
    session->state.pairSetup.method = method;

    // Validate and store kTLVType_Flags.
    if (tlvs->flagsTLV->value.bytes) {
        if (tlvs->flagsTLV->value.numBytes > sizeof(uint32_t)) {
            HAPLog(&logObject,
                   "Pair Setup M1: kTLVType_Flags has invalid length (%zu).",
                   tlvs->flagsTLV->value.numBytes);
            return kHAPError_InvalidData;
        }
        if (server->pairSetup.sessionThatIsCurrentlyPairing == session) {
            server->pairSetup.flagsPresent = true;
            server->pairSetup.flags = HAPPairingReadFlags(tlvs->flagsTLV);
        }
    } else {
        if (server->pairSetup.sessionThatIsCurrentlyPairing == session) {
            server->pairSetup.flagsPresent = false;
            server->pairSetup.flags = 0;
        }
    }

    // Log unexpected parameter combinations.
    if (HAPAccessoryServerSupportsMFiHWAuth(server)) {
        if (method != kHAPPairingMethod_PairSetupWithAuth) {
            HAPLog(&logObject,
                   "Pair Setup M1: Accessory supports %s but controller requested kTLVType_Method %d.",
                   "Apple Authentication Coprocessor",
                   method);
        }
    } else if (HAPAccessoryServerSupportsMFiTokenAuth(server)) {
        if (method != kHAPPairingMethod_PairSetup) {
            HAPLog(&logObject,
                   "Pair Setup M1: Accessory supports %s but controller requested kTLVType_Method %d.",
                   "Software Authentication",
                   method);
        } else {
            if (!tlvs->flagsTLV->value.bytes) {
                HAPLog(&logObject,
                       "Pair Setup M1: Accessory supports %s but controller did not provide kTLVType_Flags.",
                       "Software Authentication");
            }
        }
    } else {
        if (method != kHAPPairingMethod_PairSetup) {
            HAPLog(&logObject,
                   "Pair Setup M1: Accessory does not support %s but controller requested kTLVType_Method %d.",
                   "MFi authentication",
                   method);
        }
    }

    return kHAPError_None;
}

/**
 * Processes Pair Setup M2.
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
        HAPPairingPairSetupGetM2(HAPAccessoryServer* server, HAPSession* session, HAPTLVWriter* responseWriter) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(session->state.pairSetup.state == 2);
    HAPPrecondition(!session->state.pairSetup.error);
    HAPPrecondition(responseWriter);

    HAPError err;

    // See HomeKit Accessory Protocol Specification R17
    // Section 5.7.2 M2: Accessory -> iOS Device - `SRP Start Response'

    HAPLogDebug(&logObject, "Pair Setup M2: SRP Start Response.");

    // Check if the accessory is already paired.
    if (!server->pairSetup.sessionThatIsCurrentlyPairing || HAPAccessoryServerIsPaired(server)) {
        HAPLog(&logObject, "Pair Setup M2: Accessory is already paired.");
        session->state.pairSetup.error = kHAPPairingError_Unavailable;
        return kHAPError_None;
    }

    // Check if the accessory has received more than HAPAuthAttempts_Max unsuccessful authentication attempts.
    bool found;
    size_t numBytes;
    uint8_t numAuthAttemptsBytes[sizeof(uint8_t)];
    err = HAPPlatformKeyValueStoreGet(
            server->platform.keyValueStore,
            kHAPKeyValueStoreDomain_Configuration,
            kHAPKeyValueStoreKey_Configuration_NumUnsuccessfulAuthAttempts,
            numAuthAttemptsBytes,
            sizeof numAuthAttemptsBytes,
            &numBytes,
            &found);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }
    if (!found) {
        HAPRawBufferZero(numAuthAttemptsBytes, sizeof numAuthAttemptsBytes);
    } else {
        if (numBytes != sizeof numAuthAttemptsBytes) {
            HAPLog(&logObject, "Invalid authentication attempts counter length: %zu.", numBytes);
            return kHAPError_Unknown;
        }
    }
    uint8_t numAuthAttempts = numAuthAttemptsBytes[0];
    if (numAuthAttempts >= HAPAuthAttempts_Max) {
        HAPLog(&logObject,
               "Pair Setup M2: Accessory has received more than %d unsuccessful authentication attempts.",
               HAPAuthAttempts_Max);
        session->state.pairSetup.error = kHAPPairingError_MaxTries;
        return kHAPError_None;
    }

    // Check if the accessory is currently performing a Pair Setup procedure with a different controller.
    if (server->pairSetup.sessionThatIsCurrentlyPairing != session) {
        HAPLog(&logObject,
               "Pair Setup M2: Accessory is performing a Pair Setup procedure with a different controller.");
        session->state.pairSetup.error = kHAPPairingError_Busy;
        return kHAPError_None;
    }

    // Get pairing flags.
    uint32_t otherFlags = 0;
    bool isTransient = false;
    bool isSplit = false;
    if (server->pairSetup.flagsPresent) {
        uint32_t flags = server->pairSetup.flags;
        if (flags & (uint32_t) kHAPPairingFlag_Transient) {
            if (session->state.pairSetup.method == kHAPPairingMethod_PairSetupWithAuth) {
                HAPLog(&logObject,
                       "Pair Setup M2: Ignoring %s because Pair Setup with Auth was requested.",
                       "kPairingFlag_Transient");
            } else {
                HAPAssert(session->state.pairSetup.method == kHAPPairingMethod_PairSetup);
                isTransient = true;
            }
            flags &= ~(uint32_t) kHAPPairingFlag_Transient;
        }
        if (flags & (uint32_t) kHAPPairingFlag_Split) {
            if (session->state.pairSetup.method == kHAPPairingMethod_PairSetupWithAuth) {
                HAPLog(&logObject,
                       "Pair Setup M2: Ignoring %s because Pair Setup with Auth was requested.",
                       "kPairingFlag_Split");
            } else {
                HAPAssert(session->state.pairSetup.method == kHAPPairingMethod_PairSetup);
                isSplit = true;
            }
            flags &= ~(uint32_t) kHAPPairingFlag_Split;
        }
        if (flags) {
            HAPLog(&logObject, "Pair Setup M2: Ignoring unrecognized kTLVType_Flags: 0x%08lX.", (unsigned long) flags);
        }
    }
    HAPLogDebug(
            &logObject,
            "Pair Setup M2: Processing using %s = %s / %s = %s.",
            "kPairingFlag_Transient",
            isTransient ? "true" : "false",
            "kPairingFlag_Split",
            isSplit ? "true" : "false");

    // Recover setup info if requested.
    HAPSetupInfo* _Nullable setupInfo =
            HAPAccessorySetupInfoGetSetupInfo(server, /* restorePrevious: */ !isTransient && isSplit);
    if (!setupInfo) {
        HAPLog(&logObject, "Pair Setup M2: kPairingFlag_Split requested but no previous setup info found.");
        session->state.pairSetup.error = kHAPPairingError_Authentication;
        return kHAPError_None;
    }
    HAPLogBufferDebug(&logObject, setupInfo->salt, sizeof setupInfo->salt, "Pair Setup M2: salt.");
    HAPLogSensitiveBufferDebug(&logObject, setupInfo->verifier, sizeof setupInfo->verifier, "Pair Setup M2: verifier.");

    // Generate private key b.
    HAPPlatformRandomNumberFill(server->pairSetup.b, sizeof server->pairSetup.b);
    HAPLogSensitiveBufferDebug(&logObject, server->pairSetup.b, sizeof server->pairSetup.b, "Pair Setup M2: b.");

    // Derive public key B.
    HAP_srp_public_key(server->pairSetup.B, server->pairSetup.b, setupInfo->verifier);
    HAPLogBufferDebug(&logObject, server->pairSetup.B, sizeof server->pairSetup.B, "Pair Setup M2: B.");

    // kTLVType_State.
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPairingTLVType_State,
                              .value = { .bytes = &session->state.pairSetup.state, .numBytes = 1 } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // kTLVType_PublicKey.
    // Skip leading zeros.
    size_t size = SRP_PUBLIC_KEY_BYTES;
    uint8_t* B = server->pairSetup.B;
    while (size && !(*B)) {
        B++;
        size--;
    }
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPairingTLVType_PublicKey, .value = { .bytes = B, .numBytes = size } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // kTLVType_Salt.
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPairingTLVType_Salt,
                              .value = { .bytes = setupInfo->salt, .numBytes = sizeof setupInfo->salt } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // kTLVType_Flags.
    uint32_t flags = otherFlags;
    if (isSplit) {
        flags |= (uint32_t) kHAPPairingFlag_Split;
        if (isTransient) {
            flags |= (uint32_t) kHAPPairingFlag_Transient;
        }
    }
    if (HAPAccessorySetupOwnershipIsTokenRequired(&server->setupOwnership)) {
        flags |= (uint32_t) kHAPPairingFlag_OwnershipProofToken;
    }
    if (flags) {
        uint8_t flagsBytes[] = { HAPExpandLittleUInt32(flags) };
        err = HAPTLVWriterAppend(
                responseWriter,
                &(const HAPTLV) { .type = kHAPPairingTLVType_Flags,
                                  .value = { .bytes = flagsBytes, .numBytes = HAPPairingGetNumBytes(flags) } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
    }

    return kHAPError_None;
}

/**
 * Pair Setup M3 TLVs.
 */
typedef struct {
    HAPTLV* stateTLV;     /**< kTLVType_State. */
    HAPTLV* publicKeyTLV; /**< kTLVType_PublicKey. */
    HAPTLV* proofTLV;     /**< kTLVType_Proof. */
} HAPPairingPairSetupM3TLVs;

/**
 * Processes Pair Setup M3.
 *
 * @param      server              Accessory server.
 * @param      session             The session over which the request has been received.
 * @param      tlvs                 TLVs.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the controller sent a malformed request.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPPairingPairSetupProcessM3(
        HAPAccessoryServer* server,
        HAPSession* session,
        const HAPPairingPairSetupM3TLVs* tlvs) {
    HAPPrecondition(server);
    HAPPrecondition(server->pairSetup.sessionThatIsCurrentlyPairing == session);
    HAPPrecondition(session);
    HAPPrecondition(session->state.pairSetup.state == 3);
    HAPPrecondition(!session->state.pairSetup.error);
    HAPPrecondition(tlvs);
    HAPPrecondition(tlvs->stateTLV);
    HAPPrecondition(tlvs->stateTLV->type == kHAPPairingTLVType_State);
    HAPPrecondition(tlvs->publicKeyTLV);
    HAPPrecondition(tlvs->publicKeyTLV->type == kHAPPairingTLVType_PublicKey);
    HAPPrecondition(tlvs->proofTLV);
    HAPPrecondition(tlvs->proofTLV->type == kHAPPairingTLVType_Proof);

    // See HomeKit Accessory Protocol Specification R17
    // Section 5.7.3 M3: iOS Device -> Accessory - `SRP Verify Request'

    HAPLogDebug(&logObject, "Pair Setup M3: SRP Verify Request.");

    // Validate kTLVType_State.
    if (!tlvs->stateTLV->value.bytes) {
        HAPLog(&logObject, "Pair Setup M3: kTLVType_State missing.");
        return kHAPError_InvalidData;
    }
    if (tlvs->stateTLV->value.numBytes != 1) {
        HAPLog(&logObject, "Pair Setup M3: kTLVType_State has invalid length (%zu).", tlvs->stateTLV->value.numBytes);
        return kHAPError_InvalidData;
    }
    uint8_t state = ((const uint8_t*) tlvs->stateTLV->value.bytes)[0];
    if (state != 3) {
        HAPLog(&logObject, "Pair Setup M3: kTLVType_State invalid: %u.", state);
        return kHAPError_InvalidData;
    }

    // Validate kTLVType_PublicKey.
    if (!tlvs->publicKeyTLV->value.bytes) {
        HAPLog(&logObject, "Pair Setup M3: kTLVType_PublicKey missing.");
        return kHAPError_InvalidData;
    }
    if (tlvs->publicKeyTLV->value.numBytes > sizeof server->pairSetup.A) {
        HAPLog(&logObject,
               "Pair Setup M3: kTLVType_PublicKey has invalid length (%zu).",
               tlvs->publicKeyTLV->value.numBytes);
        return kHAPError_InvalidData;
    }

    // Validate kTLVType_Proof.
    if (!tlvs->proofTLV->value.bytes) {
        HAPLog(&logObject, "Pair Setup M3: kTLVType_Proof missing.");
        return kHAPError_InvalidData;
    }
    if (tlvs->proofTLV->value.numBytes != sizeof server->pairSetup.M1) {
        HAPLog(&logObject, "Pair Setup M3: kTLVType_Proof has invalid length (%zu).", tlvs->proofTLV->value.numBytes);
        return kHAPError_InvalidData;
    }

    // Copy public key to A and zero-extend big-endian.
    void* A = &server->pairSetup.A[sizeof server->pairSetup.A - tlvs->publicKeyTLV->value.numBytes];
    HAPRawBufferZero(server->pairSetup.A, sizeof server->pairSetup.A - tlvs->publicKeyTLV->value.numBytes);
    HAPRawBufferCopyBytes(A, HAPNonnullVoid(tlvs->publicKeyTLV->value.bytes), tlvs->publicKeyTLV->value.numBytes);
    HAPLogBufferDebug(&logObject, server->pairSetup.A, sizeof server->pairSetup.A, "Pair Setup M3: A.");

    // Copy proof.
    HAPRawBufferCopyBytes(
            server->pairSetup.M1, HAPNonnullVoid(tlvs->proofTLV->value.bytes), tlvs->proofTLV->value.numBytes);
    HAPLogBufferDebug(&logObject, server->pairSetup.M1, sizeof server->pairSetup.M1, "Pair Setup M3: M1.");

    return kHAPError_None;
}

/**
 * Processes Pair Setup M4.
 *
 * @param      server              Accessory server.
 * @param      session             The session over which the response will be sent.
 * @param      responseWriter       TLV writer for serializing the response.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If communication with Apple Auth Coprocessor or persistent store access failed.
 * @return kHAPError_InvalidState   If a different request is expected in the current state.
 * @return kHAPError_OutOfResources If response writer does not have enough capacity.
 */
HAP_RESULT_USE_CHECK
static HAPError
        HAPPairingPairSetupGetM4(HAPAccessoryServer* server, HAPSession* session, HAPTLVWriter* responseWriter) {
    HAPPrecondition(server);
    HAPPrecondition(server->pairSetup.sessionThatIsCurrentlyPairing == session);
    HAPPrecondition(session);
    HAPPrecondition(session->state.pairSetup.state == 4);
    HAPPrecondition(!session->state.pairSetup.error);
    HAPPrecondition(responseWriter);

    HAPError err;

    // See HomeKit Accessory Protocol Specification R17
    // Section 5.7.4 M4: Accessory -> iOS Device - `SRP Verify Response'

    HAPLogDebug(&logObject, "Pair Setup M4: SRP Verify Response.");

    // Compute SRP shared secret key.
    {
        void* bytes;
        size_t maxBytes;
        HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);

        void* u = HAPTLVScratchBufferAlloc(&bytes, &maxBytes, SRP_SCRAMBLING_PARAMETER_BYTES);
        void* S = HAPTLVScratchBufferAlloc(&bytes, &maxBytes, SRP_PREMASTER_SECRET_BYTES);
        void* M1 = HAPTLVScratchBufferAlloc(&bytes, &maxBytes, SRP_PROOF_BYTES);
        if (!u || !S || !M1) {
            HAPLog(&logObject, "Pair Setup M4: Not enough memory to allocate u / S / M1.");
            return kHAPError_OutOfResources;
        }

        HAP_srp_scrambling_parameter(u, server->pairSetup.A, server->pairSetup.B);
        HAPLogSensitiveBufferDebug(&logObject, u, SRP_SCRAMBLING_PARAMETER_BYTES, "Pair Setup M4: u.");

        bool restorePrevious = false;
        if (session->state.pairSetup.method == kHAPPairingMethod_PairSetup && server->pairSetup.flagsPresent) {
            restorePrevious = !(server->pairSetup.flags & (uint32_t) kHAPPairingFlag_Transient) &&
                              server->pairSetup.flags & (uint32_t) kHAPPairingFlag_Split;
        }
        HAPSetupInfo* _Nullable setupInfo = HAPAccessorySetupInfoGetSetupInfo(server, restorePrevious);
        HAPAssert(setupInfo);

        int e = HAP_srp_premaster_secret(S, server->pairSetup.A, server->pairSetup.b, u, setupInfo->verifier);
        if (e) {
            HAPAssert(e == 1);
            // Illegal key A.
            HAPLog(&logObject, "Pair Setup M4: Illegal key A.");
            session->state.pairSetup.error = kHAPPairingError_Authentication;
            return kHAPError_None;
        }
        HAPLogSensitiveBufferDebug(&logObject, S, SRP_PREMASTER_SECRET_BYTES, "Pair Setup M4: S.");

        HAP_srp_session_key(server->pairSetup.K, S);
        HAPLogSensitiveBufferDebug(&logObject, server->pairSetup.K, sizeof server->pairSetup.K, "Pair Setup M4: K.");

        static const uint8_t userName[] = "Pair-Setup";
        HAP_srp_proof_m1(
                M1,
                userName,
                sizeof userName - 1,
                setupInfo->salt,
                server->pairSetup.A,
                server->pairSetup.B,
                server->pairSetup.K);
        HAPLogSensitiveBufferDebug(&logObject, M1, SRP_PROOF_BYTES, "Pair Setup M4: M1");

        // Verify the controller's SRP proof.
        if (!HAPRawBufferAreEqual(M1, server->pairSetup.M1, SRP_PROOF_BYTES)) {
            bool found;
            size_t numBytes;
            uint8_t numAuthAttemptsBytes[sizeof(uint8_t)];
            err = HAPPlatformKeyValueStoreGet(
                    server->platform.keyValueStore,
                    kHAPKeyValueStoreDomain_Configuration,
                    kHAPKeyValueStoreKey_Configuration_NumUnsuccessfulAuthAttempts,
                    numAuthAttemptsBytes,
                    sizeof numAuthAttemptsBytes,
                    &numBytes,
                    &found);
            if (err) {
                HAPAssert(err == kHAPError_Unknown);
                return err;
            }
            if (!found) {
                HAPRawBufferZero(numAuthAttemptsBytes, sizeof numAuthAttemptsBytes);
            } else {
                if (numBytes != sizeof numAuthAttemptsBytes) {
                    HAPLog(&logObject, "Invalid authentication attempts counter length: %zu.", numBytes);
                    return kHAPError_Unknown;
                }
            }
            uint8_t numAuthAttempts = numAuthAttemptsBytes[0];
            HAPAssert(numAuthAttempts < UINT8_MAX);
            numAuthAttempts++;
            numAuthAttemptsBytes[0] = numAuthAttempts;
            err = HAPPlatformKeyValueStoreSet(
                    server->platform.keyValueStore,
                    kHAPKeyValueStoreDomain_Configuration,
                    kHAPKeyValueStoreKey_Configuration_NumUnsuccessfulAuthAttempts,
                    numAuthAttemptsBytes,
                    sizeof numAuthAttemptsBytes);
            if (err) {
                HAPAssert(err == kHAPError_Unknown);
                return err;
            }
            HAPLog(&logObject,
                   "Pair Setup M4: Incorrect setup code. Unsuccessful authentication attempts = %u / %u.",
                   numAuthAttempts,
                   HAPAuthAttempts_Max);
            session->state.pairSetup.error = kHAPPairingError_Authentication;
            return kHAPError_None;
        }

        // Reset authentication attempts counter.
        err = HAPPlatformKeyValueStoreRemove(
                server->platform.keyValueStore,
                kHAPKeyValueStoreDomain_Configuration,
                kHAPKeyValueStoreKey_Configuration_NumUnsuccessfulAuthAttempts);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            return err;
        }

        // Generate accessory-side SRP proof.
        HAP_srp_proof_m2(server->pairSetup.M2, server->pairSetup.A, M1, server->pairSetup.K);
        HAPLogBufferDebug(&logObject, server->pairSetup.M2, sizeof server->pairSetup.M2, "Pair Setup M4: M2.");

        // Derive the symmetric session encryption key.
        static const uint8_t salt[] = "Pair-Setup-Encrypt-Salt";
        static const uint8_t info[] = "Pair-Setup-Encrypt-Info";
        HAP_hkdf_sha512(
                server->pairSetup.SessionKey,
                sizeof server->pairSetup.SessionKey,
                server->pairSetup.K,
                sizeof server->pairSetup.K,
                salt,
                sizeof salt - 1,
                info,
                sizeof info - 1);
        HAPLogSensitiveBufferDebug(
                &logObject,
                server->pairSetup.SessionKey,
                sizeof server->pairSetup.SessionKey,
                "Pair Setup M4: SessionKey");
    }

    // kTLVType_State.
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPairingTLVType_State,
                              .value = { .bytes = &session->state.pairSetup.state, .numBytes = 1 } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // kTLVType_Proof.
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPairingTLVType_Proof,
                              .value = { .bytes = server->pairSetup.M2, .numBytes = sizeof server->pairSetup.M2 } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // kTLVType_EncryptedData.
    if (session->state.pairSetup.method == kHAPPairingMethod_PairSetupWithAuth ||
        HAPNonnull(server->primaryAccessory)->productData) {
        // Construct sub-TLV writer.
        HAPTLVWriter subWriter;
        {
            void* bytes;
            size_t maxBytes;
            HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);
            if (maxBytes < CHACHA20_POLY1305_TAG_BYTES) {
                HAPLog(&logObject, "Pair Setup M4: Not enough memory for kTLVType_EncryptedData auth tag.");
                return kHAPError_OutOfResources;
            }
            maxBytes -= CHACHA20_POLY1305_TAG_BYTES;
            HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
        }

        if (session->state.pairSetup.method == kHAPPairingMethod_PairSetupWithAuth) {
            HAPMFiAuth mfiAuth;

            {
                if (!server->platform.authentication.mfiHWAuth || !HAPAccessoryServerSupportsMFiHWAuth(server)) {
                    HAPLog(&logObject, "Pair Setup M4: Apple Authentication Coprocessor is not available.");
                    return kHAPError_InvalidState;
                }
                HAPLogInfo(&logObject, "Using Apple Authentication Coprocessor.");
                mfiAuth.copyCertificate = HAPMFiHWAuthCopyCertificate;
                mfiAuth.createSignature = HAPMFiHWAuthCreateSignature;
            }

            // kTLVType_Signature.
            {
                void* bytes;
                size_t maxBytes;
                HAPTLVWriterGetScratchBytes(&subWriter, &bytes, &maxBytes);

                const size_t numChallengeBytes = 32;
                void* challengeBytes = HAPTLVScratchBufferAlloc(&bytes, &maxBytes, numChallengeBytes);
                const size_t maxMFiProofBytes = maxBytes;
                void* mfiProofBytes = HAPTLVScratchBufferAllocUnaligned(&bytes, &maxBytes, maxMFiProofBytes);
                if (!challengeBytes || !mfiProofBytes) {
                    HAPLog(&logObject, "Pair Setup M4: Not enough memory to allocate MFiChallenge / MFi Proof.");
                    return kHAPError_OutOfResources;
                }

                // Generate MFi challenge.
                static const uint8_t salt[] = "MFi-Pair-Setup-Salt";
                static const uint8_t info[] = "MFi-Pair-Setup-Info";
                HAP_hkdf_sha512(
                        challengeBytes,
                        numChallengeBytes,
                        server->pairSetup.K,
                        sizeof server->pairSetup.K,
                        salt,
                        sizeof salt - 1,
                        info,
                        sizeof info - 1);
                HAPLogSensitiveBufferDebug(
                        &logObject, challengeBytes, numChallengeBytes, "Pair Setup M4: MFiChallenge.");

                // Generate the MFi proof.
                size_t numMFiProofBytes;
                err = mfiAuth.createSignature(
                        server, challengeBytes, numChallengeBytes, mfiProofBytes, maxMFiProofBytes, &numMFiProofBytes);
                if (err) {
                    HAPAssert(err == kHAPError_Unknown);
                    return err;
                }
                HAPLogSensitiveBufferDebug(
                        &logObject, mfiProofBytes, numMFiProofBytes, "Pair Setup M4: kTLVType_Signature.");

                // kTLVType_Signature.
                err = HAPTLVWriterAppend(
                        &subWriter,
                        &(const HAPTLV) { .type = kHAPPairingTLVType_Signature,
                                          .value = { .bytes = mfiProofBytes, .numBytes = numMFiProofBytes } });
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }
            }

            // kTLVType_Certificate.
            {
                void* bytes;
                size_t maxBytes;
                HAPTLVWriterGetScratchBytes(&subWriter, &bytes, &maxBytes);

                const size_t maxCertificateBytes = maxBytes;
                void* certificateBytes = HAPTLVScratchBufferAllocUnaligned(&bytes, &maxBytes, maxCertificateBytes);
                if (!certificateBytes) {
                    HAPLog(&logObject, "Pair Setup M4: Not enough memory to allocate Accessory Certificate.");
                    return kHAPError_OutOfResources;
                }

                // Read the Accessory Certificate.
                size_t numCertificateBytes;
                err = mfiAuth.copyCertificate(server, certificateBytes, maxCertificateBytes, &numCertificateBytes);
                if (err) {
                    HAPAssert(err == kHAPError_Unknown);
                    return err;
                }
                HAPLogSensitiveBufferDebug(
                        &logObject, certificateBytes, numCertificateBytes, "Pair Setup M4: kTLVType_Certificate.");

                // kTLVType_Certificate.
                err = HAPTLVWriterAppend(
                        &subWriter,
                        &(const HAPTLV) { .type = kHAPPairingTLVType_Certificate,
                                          .value = { .bytes = certificateBytes, .numBytes = numCertificateBytes } });
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }
            }
        }

        if (HAPNonnull(server->primaryAccessory)->productData) {
            // kTLVType_ProductData.
            HAPAccessoryProductData productData;
            HAPAccessoryGetProductData(HAPNonnull(server->primaryAccessory), &productData);
            HAPLogSensitiveBufferDebug(
                    &logObject, productData.bytes, sizeof productData.bytes, "Pair Setup M4: kTLVType_ProductData.");
            err = HAPTLVWriterAppend(
                    &subWriter,
                    &(const HAPTLV) { .type = kHAPPairingTLVType_ProductData,
                                      .value = { .bytes = productData.bytes, .numBytes = sizeof productData.bytes } });
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }
        }

        // Encrypt the sub-TLV.
        void* bytes;
        size_t numBytes;
        HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
        static const uint8_t nonce[] = "PS-Msg04";
        HAP_chacha20_poly1305_encrypt(
                &((uint8_t*) bytes)[numBytes],
                bytes,
                bytes,
                numBytes,
                nonce,
                sizeof nonce - 1,
                server->pairSetup.SessionKey);
        numBytes += CHACHA20_POLY1305_TAG_BYTES;
        HAPLogBufferDebug(&logObject, bytes, numBytes, "Pair Setup M4: kTLVType_EncryptedData.");

        // kTLVType_EncryptedData.
        err = HAPTLVWriterAppend(
                responseWriter,
                &(const HAPTLV) { .type = kHAPPairingTLVType_EncryptedData,
                                  .value = { .bytes = bytes, .numBytes = numBytes } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
    }
    if (session->state.pairSetup.method == kHAPPairingMethod_PairSetup && server->pairSetup.flagsPresent &&
        server->pairSetup.flags & (uint32_t) kHAPPairingFlag_Transient) {
        // Initialize HAP session.
        HAPRawBufferZero(&session->hap, sizeof session->hap);

        // Derive encryption keys.
        static const uint8_t salt[] = "SplitSetupSalt";
        {
            static const uint8_t info[] = "AccessoryEncrypt-Control";
            HAP_hkdf_sha512(
                    session->hap.accessoryToController.controlChannel.key.bytes,
                    sizeof session->hap.accessoryToController.controlChannel.key.bytes,
                    server->pairSetup.K,
                    sizeof server->pairSetup.K,
                    salt,
                    sizeof salt - 1,
                    info,
                    sizeof info - 1);
            HAPLogSensitiveBufferDebug(
                    &logObject,
                    session->hap.accessoryToController.controlChannel.key.bytes,
                    sizeof session->hap.accessoryToController.controlChannel.key.bytes,
                    "Transient Pair Setup Start Session: AccessoryEncryptKey");
        }
        {
            static const uint8_t info[] = "ControllerEncrypt-Control";
            HAP_hkdf_sha512(
                    session->hap.controllerToAccessory.controlChannel.key.bytes,
                    sizeof session->hap.controllerToAccessory.controlChannel.key.bytes,
                    server->pairSetup.K,
                    sizeof server->pairSetup.K,
                    salt,
                    sizeof salt - 1,
                    info,
                    sizeof info - 1);
            HAPLogSensitiveBufferDebug(
                    &logObject,
                    session->hap.controllerToAccessory.controlChannel.key.bytes,
                    sizeof session->hap.controllerToAccessory.controlChannel.key.bytes,
                    "Transient Pair Setup Start Session: ControllerEncryptKey");
        }
        session->hap.accessoryToController.controlChannel.nonce = 0;
        session->hap.controllerToAccessory.controlChannel.nonce = 0;

        // Activate session.
        session->hap.isTransient = true;
        session->hap.active = true;
        session->hap.timestamp = HAPPlatformClockGetCurrent();

        // Persist setup info for next Pair Setup procedure if requested.
        if (server->pairSetup.flags & (uint32_t) kHAPPairingFlag_Split) {
            server->pairSetup.keepSetupInfo = true;
        } else {
            HAPLog(&logObject, "Transient Pair Setup procedure requested without kHAPPairingFlag_Split.");
        }

        // Reset Pair Setup procedure.
        HAPPairingPairSetupResetForSession(server, session);

        HAPLogInfo(&logObject, "Transient Pair Setup procedure completed.");

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

    return kHAPError_None;
}

/**
 * Pair Setup M5 TLVs.
 */
typedef struct {
    HAPTLV* stateTLV;         /**< kTLVType_State. */
    HAPTLV* encryptedDataTLV; /**< kTLVType_EncryptedData. */
} HAPPairingPairSetupM5TLVs;

/**
 * Processes Pair Setup M5.
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
static HAPError HAPPairingPairSetupProcessM5(
        HAPAccessoryServer* server,
        HAPSession* session,
        void* scratchBytes,
        size_t numScratchBytes,
        const HAPPairingPairSetupM5TLVs* tlvs) {
    HAPPrecondition(server);
    HAPPrecondition(server->pairSetup.sessionThatIsCurrentlyPairing == session);
    HAPPrecondition(session);
    HAPPrecondition(session->state.pairSetup.state == 5);
    HAPPrecondition(!session->state.pairSetup.error);
    HAPPrecondition(tlvs);
    HAPPrecondition(tlvs->stateTLV);
    HAPPrecondition(tlvs->stateTLV->type == kHAPPairingTLVType_State);
    HAPPrecondition(tlvs->encryptedDataTLV);
    HAPPrecondition(tlvs->encryptedDataTLV->type == kHAPPairingTLVType_EncryptedData);

    HAPError err;

    // See HomeKit Accessory Protocol Specification R17
    // Section 5.7.5 M5: iOS Device -> Accessory - `Exchange Request'

    HAPLogDebug(&logObject, "Pair Setup M5: Exchange Request.");

    // Validate kTLVType_State.
    if (!tlvs->stateTLV->value.bytes) {
        HAPLog(&logObject, "Pair Setup M5: kTLVType_State missing.");
        return kHAPError_InvalidData;
    }
    if (tlvs->stateTLV->value.numBytes != 1) {
        HAPLog(&logObject, "Pair Setup M5: kTLVType_State has invalid length (%zu).", tlvs->stateTLV->value.numBytes);
        return kHAPError_InvalidData;
    }
    uint8_t state = ((const uint8_t*) tlvs->stateTLV->value.bytes)[0];
    if (state != 5) {
        HAPLog(&logObject, "Pair Setup M5: kTLVType_State invalid: %u.", state);
        return kHAPError_InvalidData;
    }

    // Validate kTLVType_EncryptedData.
    if (!tlvs->encryptedDataTLV->value.bytes) {
        HAPLog(&logObject, "Pair Setup M5: kTLVType_EncryptedData missing.");
        return kHAPError_InvalidData;
    }
    if (tlvs->encryptedDataTLV->value.numBytes < CHACHA20_POLY1305_TAG_BYTES) {
        HAPLog(&logObject,
               "Pair Setup M5: kTLVType_EncryptedData has invalid length (%zu).",
               tlvs->encryptedDataTLV->value.numBytes);
        return kHAPError_InvalidData;
    }

    // Verify auth tag and decrypt.
    HAPLogBufferDebug(
            &logObject,
            tlvs->encryptedDataTLV->value.bytes,
            tlvs->encryptedDataTLV->value.numBytes,
            "Pair Setup M5: kTLVType_EncryptedData.");
    void* bytes = (void*) (uintptr_t) tlvs->encryptedDataTLV->value.bytes;
    size_t numBytes = tlvs->encryptedDataTLV->value.numBytes - CHACHA20_POLY1305_TAG_BYTES;
    static const uint8_t nonce[] = "PS-Msg05";
    int e = HAP_chacha20_poly1305_decrypt(
            &((uint8_t*) bytes)[numBytes],
            bytes,
            bytes,
            numBytes,
            nonce,
            sizeof nonce - 1,
            server->pairSetup.SessionKey);
    if (e) {
        HAPAssert(e == -1);
        HAPLog(&logObject, "Pair Setup M5: Failed to decrypt kTLVType_EncryptedData.");
        session->state.pairSetup.error = kHAPPairingError_Authentication;
        return kHAPError_None;
    }
    HAPLogSensitiveBufferDebug(&logObject, bytes, numBytes, "Pair Setup M5: kTLVType_EncryptedData (decrypted).");

    // Parse sub-TLV.
    HAPTLV identifierTLV, publicKeyTLV, signatureTLV, ownershipProofTokenTLV;
    identifierTLV.type = kHAPPairingTLVType_Identifier;
    publicKeyTLV.type = kHAPPairingTLVType_PublicKey;
    signatureTLV.type = kHAPPairingTLVType_Signature;
    ownershipProofTokenTLV.type = kHAPPairingTLVType_OwnershipProofToken;
    {
        HAPTLVReader subReader;
        HAPTLVReaderCreate(&subReader, bytes, numBytes);

        err = HAPTLVReaderGetAll(
                &subReader,
                (HAPTLV* const[]) { &identifierTLV, &publicKeyTLV, &signatureTLV, &ownershipProofTokenTLV, NULL });
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            return err;
        }

        // Validate kTLVType_Identifier.
        if (!identifierTLV.value.bytes) {
            HAPLog(&logObject, "Pair Setup M5: kTLVType_Identifier missing.");
            return kHAPError_InvalidData;
        }
        if (identifierTLV.value.numBytes > sizeof(HAPPairingID)) {
            HAPLog(&logObject,
                   "Pair Setup M5: kTLVType_Identifier has invalid length (%zu).",
                   identifierTLV.value.numBytes);
            return kHAPError_InvalidData;
        }

        // Validate kTLVType_PublicKey.
        if (!publicKeyTLV.value.bytes) {
            HAPLog(&logObject, "Pair Setup M5: kTLVType_PublicKey missing.");
            return kHAPError_InvalidData;
        }
        if (publicKeyTLV.value.numBytes != sizeof(HAPPairingPublicKey)) {
            HAPLog(&logObject,
                   "Pair Setup M5: kTLVType_PublicKey has invalid length (%zu).",
                   publicKeyTLV.value.numBytes);
            return kHAPError_InvalidData;
        }

        // Validate kTLVType_Signature.
        if (!signatureTLV.value.bytes) {
            HAPLog(&logObject, "Pair Setup M5: kTLVType_Signature missing.");
            return kHAPError_InvalidData;
        }
        if (signatureTLV.value.numBytes != ED25519_BYTES) {
            HAPLog(&logObject,
                   "Pair Setup M5: kTLVType_Signature has invalid length (%zu).",
                   signatureTLV.value.numBytes);
            return kHAPError_InvalidData;
        }

        // Validate kTLVType_OwnershipProofToken.
        if (HAPAccessorySetupOwnershipIsTokenRequired(&server->setupOwnership)) {
            if (!ownershipProofTokenTLV.value.bytes) {
                HAPLog(&logObject, "Pair Setup M5: kTLVType_OwnershipProofToken missing.");
                session->state.pairSetup.error = kHAPPairingError_OwnershipFailure;
                HAPAccessorySetupOwnershipInvalidateToken(&server->setupOwnership);
                return kHAPError_None;
            }
            HAPLogSensitiveBufferDebug(
                    &logObject,
                    ownershipProofTokenTLV.value.bytes,
                    ownershipProofTokenTLV.value.numBytes,
                    "Pair Setup M5: kTLVType_OwnershipProofToken.");
            HAPAccessorySetupOwnershipProofToken ownershipToken;
            if (ownershipProofTokenTLV.value.numBytes != sizeof ownershipToken.bytes) {
                HAPLog(&logObject,
                       "Pair Setup M5: kTLVType_OwnershipProofToken has invalid length (%zu).",
                       ownershipProofTokenTLV.value.numBytes);
                session->state.pairSetup.error = kHAPPairingError_OwnershipFailure;
                HAPAccessorySetupOwnershipInvalidateToken(&server->setupOwnership);
                return kHAPError_None;
            }
            HAPRawBufferCopyBytes(
                    ownershipToken.bytes,
                    HAPNonnullVoid(ownershipProofTokenTLV.value.bytes),
                    ownershipProofTokenTLV.value.numBytes);
            if (!HAPAccessorySetupOwnershipIsTokenValid(&server->setupOwnership, &ownershipToken)) {
                HAPLog(&logObject, "Pair Setup M5: kTLVType_OwnershipProofToken invalid.");
                session->state.pairSetup.error = kHAPPairingError_OwnershipFailure;
                HAPAccessorySetupOwnershipInvalidateToken(&server->setupOwnership);
                return kHAPError_None;
            }
            HAPAccessorySetupOwnershipInvalidateToken(&server->setupOwnership);
        }
    }

    const size_t XLength = 32;
    void* X = HAPTLVScratchBufferAlloc(&scratchBytes, &numScratchBytes, XLength);
    void* pairingID = HAPTLVScratchBufferAllocUnaligned(&scratchBytes, &numScratchBytes, identifierTLV.value.numBytes);
    void* ltpk = HAPTLVScratchBufferAllocUnaligned(&scratchBytes, &numScratchBytes, ED25519_PUBLIC_KEY_BYTES);
    if (!X || !pairingID || !ltpk) {
        HAPLog(&logObject,
               "Pair Setup M5: Not enough memory to allocate iOSDeviceX / iOSDevicePairingID / iOSDeviceLTPK.");
        return kHAPError_OutOfResources;
    }

    // Derive iOSDeviceX from the SRP shared secret.
    static const uint8_t salt[] = "Pair-Setup-Controller-Sign-Salt";
    static const uint8_t info[] = "Pair-Setup-Controller-Sign-Info";
    HAP_hkdf_sha512(
            X, XLength, server->pairSetup.K, sizeof server->pairSetup.K, salt, sizeof salt - 1, info, sizeof info - 1);

    // Construct iOSDeviceInfo: iOSDeviceX, iOSDevicePairingID, iOSDeviceLTPK.
    HAPRawBufferCopyBytes(pairingID, HAPNonnullVoid(identifierTLV.value.bytes), identifierTLV.value.numBytes);
    HAPRawBufferCopyBytes(ltpk, HAPNonnullVoid(publicKeyTLV.value.bytes), publicKeyTLV.value.numBytes);

    // Finalize info.
    void* infoBytes = X;
    size_t numInfoBytes = XLength + identifierTLV.value.numBytes + ED25519_PUBLIC_KEY_BYTES;
    HAPLogSensitiveBufferDebug(&logObject, infoBytes, numInfoBytes, "Pair Setup M5: iOSDeviceInfo.");

    // Verify signature.
    HAPLogSensitiveBufferDebug(
            &logObject, signatureTLV.value.bytes, signatureTLV.value.numBytes, "Pair Setup M5: kTLVType_Signature.");
    e = HAP_ed25519_verify(signatureTLV.value.bytes, infoBytes, numInfoBytes, ltpk);
    if (e) {
        HAPAssert(e == -1);
        HAPLog(&logObject, "Pair Setup M5: iOSDeviceInfo signature is incorrect.");
        session->state.pairSetup.error = kHAPPairingError_Authentication;
        return kHAPError_None;
    }

    // Persistently save the iOSDevicePairingID and iOSDeviceLTPK as a pairing.
    HAPPairing pairing;
    HAPRawBufferZero(&pairing, sizeof pairing);
    HAPAssert(identifierTLV.value.numBytes <= sizeof pairing.identifier.bytes);
    HAPRawBufferCopyBytes(
            pairing.identifier.bytes, HAPNonnullVoid(identifierTLV.value.bytes), identifierTLV.value.numBytes);
    pairing.numIdentifierBytes = (uint8_t) identifierTLV.value.numBytes;
    HAPRawBufferCopyBytes(
            pairing.publicKey.value, HAPNonnullVoid(publicKeyTLV.value.bytes), publicKeyTLV.value.numBytes);
    pairing.permissions = 0x01;

    HAPPairingIndex pairingIndex;
    err = HAPPairingAdd(server, &pairing, &pairingIndex);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }
    return kHAPError_None;
}

/**
 * Processes Pair Setup M6.
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
        HAPPairingPairSetupGetM6(HAPAccessoryServer* server, HAPSession* session, HAPTLVWriter* responseWriter) {
    HAPPrecondition(server);
    HAPPrecondition(server->pairSetup.sessionThatIsCurrentlyPairing == session);
    HAPPrecondition(session);
    HAPPrecondition(session->state.pairSetup.state == 6);
    HAPPrecondition(!session->state.pairSetup.error);
    HAPPrecondition(responseWriter);

    HAPError err;

    // See HomeKit Accessory Protocol Specification R17
    // Section 5.7.6 M6: Accessory -> iOS Device - `Exchange Response'

    HAPLogDebug(&logObject, "Pair Setup M6: Exchange Response.");

    // Accessory Long Term Keys are already generated earlier.

    // kTLVType_State.
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPairingTLVType_State,
                              .value = { .bytes = &session->state.pairSetup.state, .numBytes = 1 } });
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
            HAPLog(&logObject, "Pair Setup M4: Not enough memory for kTLVType_EncryptedData auth tag.");
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
    size_t numDeviceIDBytes = HAPStringGetNumBytes(deviceIDString.stringValue);
    err = HAPTLVWriterAppend(
            &subWriter,
            &(const HAPTLV) { .type = kHAPPairingTLVType_Identifier,
                              .value = { .bytes = deviceIDString.stringValue, .numBytes = numDeviceIDBytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // kTLVType_PublicKey.
    {
        HAPLogSensitiveBufferDebug(
                &logObject,
                server->identity.ed_LTSK.bytes,
                sizeof server->identity.ed_LTSK.bytes,
                "Pair Setup M6: ed_LTSK.");
    }
    HAPLogSensitiveBufferDebug(
            &logObject, server->identity.ed_LTPK, sizeof server->identity.ed_LTPK, "Pair Setup M6: ed_LTPK.");

    err = HAPTLVWriterAppend(
            &subWriter,
            &(const HAPTLV) {
                    .type = kHAPPairingTLVType_PublicKey,
                    .value = { .bytes = server->identity.ed_LTPK, .numBytes = sizeof server->identity.ed_LTPK } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // kTLVType_Signature.
    {
        void* bytes;
        size_t maxBytes;
        HAPTLVWriterGetScratchBytes(&subWriter, &bytes, &maxBytes);

        const size_t XLength = 32;
        void* X = HAPTLVScratchBufferAlloc(&bytes, &maxBytes, XLength);
        void* pairingID = HAPTLVScratchBufferAllocUnaligned(&bytes, &maxBytes, numDeviceIDBytes);
        void* ltpk = HAPTLVScratchBufferAllocUnaligned(&bytes, &maxBytes, ED25519_PUBLIC_KEY_BYTES);
        void* signature = HAPTLVScratchBufferAlloc(&bytes, &maxBytes, ED25519_BYTES);
        if (!X || !pairingID || !ltpk || !signature) {
            HAPLog(&logObject,
                   "Pair Setup M6: Not enough memory to allocate "
                   "AccessoryX / AccessoryPairingID / AccessoryLTPK / Signature.");
            return kHAPError_OutOfResources;
        }

        // Derive AccessoryX from the SRP shared secret.
        static const uint8_t salt[] = "Pair-Setup-Accessory-Sign-Salt";
        static const uint8_t info[] = "Pair-Setup-Accessory-Sign-Info";
        HAP_hkdf_sha512(
                X,
                XLength,
                server->pairSetup.K,
                sizeof server->pairSetup.K,
                salt,
                sizeof salt - 1,
                info,
                sizeof info - 1);

        // Construct AccessoryDeviceInfo: AccessoryX, AccessoryPairingID, AccessoryLTPK.
        HAPRawBufferCopyBytes(pairingID, deviceIDString.stringValue, numDeviceIDBytes);
        HAPRawBufferCopyBytes(ltpk, server->identity.ed_LTPK, sizeof server->identity.ed_LTPK);

        // Finalize info.
        void* infoBytes = X;
        size_t numInfoBytes = XLength + numDeviceIDBytes + ED25519_PUBLIC_KEY_BYTES;

        // Generate signature.
        {
            HAP_ed25519_sign(
                    signature, infoBytes, numInfoBytes, server->identity.ed_LTSK.bytes, server->identity.ed_LTPK);
        }
        HAPLogSensitiveBufferDebug(&logObject, infoBytes, numInfoBytes, "Pair Setup M6: AccessoryDeviceInfo.");
        HAPLogSensitiveBufferDebug(&logObject, signature, ED25519_BYTES, "Pair Setup M6: kTLVType_Signature.");

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

    // Encrypt the sub-TLV.
    void* bytes;
    size_t numBytes;
    HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
    static const uint8_t nonce[] = "PS-Msg06";
    HAP_chacha20_poly1305_encrypt(
            &((uint8_t*) bytes)[numBytes],
            bytes,
            bytes,
            numBytes,
            nonce,
            sizeof nonce - 1,
            server->pairSetup.SessionKey);
    numBytes += CHACHA20_POLY1305_TAG_BYTES;
    HAPLogBufferDebug(&logObject, bytes, numBytes, "Pair Setup M6: kTLVType_EncryptedData.");

    // kTLVType_EncryptedData.
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPairingTLVType_EncryptedData,
                              .value = { .bytes = bytes, .numBytes = numBytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // Reset Pair Setup procedure.
    HAPPairingPairSetupResetForSession(server, session);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPairingPairSetupHandleWrite(HAPAccessoryServer* server, HAPSession* session, HAPTLVReader* requestReader) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(requestReader);

    HAPError err;

    // Parse request.
    HAPTLV methodTLV, publicKeyTLV, proofTLV, encryptedDataTLV, stateTLV, flagsTLV;
    methodTLV.type = kHAPPairingTLVType_Method;
    publicKeyTLV.type = kHAPPairingTLVType_PublicKey;
    proofTLV.type = kHAPPairingTLVType_Proof;
    encryptedDataTLV.type = kHAPPairingTLVType_EncryptedData;
    stateTLV.type = kHAPPairingTLVType_State;
    flagsTLV.type = kHAPPairingTLVType_Flags;

    // Each time we receive a setup message, renew our wakelock.  This will either timeout or be stopped
    // when the verify is successful
    if (session->transportType == kHAPTransportType_Thread) {
        HAPPlatformThreadAddWakeLock(&kPairSetupWakeLock, PAIR_SETUP_WAKELOCK_TIMEOUT_MS);
    }

    err = HAPTLVReaderGetAll(
            requestReader,
            (HAPTLV* const[]) { &methodTLV, &publicKeyTLV, &proofTLV, &encryptedDataTLV, &stateTLV, &flagsTLV, NULL });
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        HAPPairingPairSetupResetForSession(server, session);
        return err;
    }

    // Certain controllers sometimes forget that pairing attempt is in progress and restart Pair Setup procedure at M1.
    // When this situation happens, we would regularly reject the request.
    // However, followup issues lead to those controllers forgetting to send the Pair Setup M3 message
    // after the setup code has been entered by the user.
    // As a mitigation, we cancel an ongoing Pair Setup procedure if the same controller sends Pair Setup M1 again.
    // Observed on iOS 12.1.
    if (server->pairSetup.sessionThatIsCurrentlyPairing == session && stateTLV.value.bytes &&
        stateTLV.value.numBytes == 1 && ((const uint8_t*) stateTLV.value.bytes)[0] == 1) {
        HAPLog(&logObject, "Received Pair Setup M1 during ongoing Pair Setup procedure. Aborting previous procedure.");
        server->pairSetup.keepSetupInfo = true;
        HAPPairingPairSetupResetForSession(server, session);
        server->accessorySetup.state.keepSetupInfo = false;
    }

    // Try to claim Pair Setup procedure.
    if (!session->state.pairSetup.state && !HAPAccessoryServerIsPaired(server)) {
        if (server->pairSetup.sessionThatIsCurrentlyPairing &&
            server->pairSetup.sessionThatIsCurrentlyPairing != session) {
            HAPTime now = HAPPlatformClockGetCurrent();
            HAPTime deadline = server->pairSetup.operationStartTime + kHAPPairing_PairSetupProcedureTimeout;
            if (now >= deadline) {
                HAPLog(&logObject,
                       "Pair Setup: Resetting Pair Setup procedure after %llu seconds.",
                       (unsigned long long) ((now - server->pairSetup.operationStartTime) / HAPSecond));
                server->pairSetup.keepSetupInfo = true;
                HAPPairingPairSetupResetForSession(server, HAPNonnull(server->pairSetup.sessionThatIsCurrentlyPairing));
                server->accessorySetup.state.keepSetupInfo = false;
            }
        }
        if (!server->pairSetup.sessionThatIsCurrentlyPairing) {
            server->pairSetup.sessionThatIsCurrentlyPairing = session;
            server->pairSetup.operationStartTime = HAPPlatformClockGetCurrent();
            HAPAccessorySetupInfoHandlePairingStart(server);
        }
    }

    // Get free memory.
    void* bytes;
    size_t maxBytes;
    HAPTLVReaderGetScratchBytes(requestReader, &bytes, &maxBytes);

    // Process request.
    switch (session->state.pairSetup.state) {
        case 0: {
            session->state.pairSetup.state++;
            err = HAPPairingPairSetupProcessM1(
                    server,
                    session,
                    &(const HAPPairingPairSetupM1TLVs) {
                            .stateTLV = &stateTLV, .methodTLV = &methodTLV, .flagsTLV = &flagsTLV });
            if (err) {
                HAPAssert(err == kHAPError_InvalidData);
            }
            break;
        }
        case 2: {
            session->state.pairSetup.state++;
            err = HAPPairingPairSetupProcessM3(
                    server,
                    session,
                    &(const HAPPairingPairSetupM3TLVs) {
                            .stateTLV = &stateTLV, .publicKeyTLV = &publicKeyTLV, .proofTLV = &proofTLV });
            if (err) {
                HAPAssert(err == kHAPError_InvalidData);
            }
            break;
        }
        case 4: {
            session->state.pairSetup.state++;
            err = HAPPairingPairSetupProcessM5(
                    server,
                    session,
                    bytes,
                    maxBytes,
                    &(const HAPPairingPairSetupM5TLVs) { .stateTLV = &stateTLV,
                                                         .encryptedDataTLV = &encryptedDataTLV });
            if (err) {
                HAPAssert(err == kHAPError_InvalidData || err == kHAPError_OutOfResources);
            }
            break;
        }
        default: {
            HAPLog(&logObject, "Received unexpected Pair Setup write in state M%u.", session->state.pairSetup.state);
            err = kHAPError_InvalidState;
            break;
        }
    }
    if (err) {
        HAPPairingPairSetupResetForSession(server, session);
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
static HAPError HAPPairingPairSetupGetErrorResponse(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPTLVWriter* responseWriter) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(responseWriter);
    HAPPrecondition(session->state.pairSetup.error);

    HAPError err;

    // kTLVType_State.
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPairingTLVType_State,
                              .value = { .bytes = &session->state.pairSetup.state, .numBytes = 1 } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // kTLVType_Error.
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPairingTLVType_Error,
                              .value = { .bytes = &session->state.pairSetup.error, .numBytes = 1 } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPairingPairSetupHandleRead(HAPAccessoryServer* server, HAPSession* session, HAPTLVWriter* responseWriter) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(responseWriter);

    HAPError err;

    // Handle pending error.
    if (session->state.pairSetup.error) {
        // Advance state.
        session->state.pairSetup.state++;

        err = HAPPairingPairSetupGetErrorResponse(server, session, responseWriter);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            HAPPairingPairSetupResetForSession(server, session);
            return err;
        }

        // Reset session.
        HAPPairingPairSetupResetForSession(server, session);
        return kHAPError_None;
    }

    // Process request.
    switch (session->state.pairSetup.state) {
        case 1: {
            session->state.pairSetup.state++;
            err = HAPPairingPairSetupGetM2(server, session, responseWriter);
            if (err) {
                HAPAssert(err == kHAPError_Unknown || err == kHAPError_OutOfResources);
            }
            break;
        }
        case 3: {
            session->state.pairSetup.state++;
            err = HAPPairingPairSetupGetM4(server, session, responseWriter);
            if (err) {
                HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources);
            }
            break;
        }
        case 5: {
            session->state.pairSetup.state++;
            err = HAPPairingPairSetupGetM6(server, session, responseWriter);
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
            }
            break;
        }
        default: {
            HAPLog(&logObject, "Received unexpected Pair Setup read in state M%u.", session->state.pairSetup.state);
            err = kHAPError_InvalidState;
            break;
        }
    }
    if (err) {
        HAPPairingPairSetupResetForSession(server, session);
        return err;
    }

    // Handle pending error.
    if (session->state.pairSetup.error) {
        err = HAPPairingPairSetupGetErrorResponse(server, session, responseWriter);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            HAPPairingPairSetupResetForSession(server, session);
            return err;
        }

        // Reset session.
        HAPPairingPairSetupResetForSession(server, session);
        return kHAPError_None;
    }

    return kHAPError_None;
}
