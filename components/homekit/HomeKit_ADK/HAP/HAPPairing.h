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

#ifndef HAP_PAIRING_H
#define HAP_PAIRING_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP+API.h"
#include "HAPCrypto.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * Pairing identifier.
 *
 * iOS and HAT based controllers have been observed to use 128-bit upper case UUIDs as their identifier.
 * Format: XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX
 */
typedef struct {
    uint8_t bytes[36]; /**< Value. */
} HAPPairingID;
HAP_STATIC_ASSERT(sizeof(HAPPairingID) == 36, HAPPairingID);

/**
 * Pairing public key.
 */
typedef struct {
    /** Value. */
    uint8_t value[ED25519_PUBLIC_KEY_BYTES];
} HAPPairingPublicKey;
HAP_STATIC_ASSERT(sizeof(HAPPairingPublicKey) == 32, HAPPairingPublicKey);

/**
 * Pairing.
 */
typedef struct {
    HAPPairingID identifier;       /**< Pairing identifier. */
    uint8_t numIdentifierBytes;    /**< Length of the pairing identifier. */
    HAPPairingPublicKey publicKey; /**< Public key. */
    uint8_t permissions;           /**< Permission flags. */
} HAPPairing;

/**
 * PairingIndex.
 */
typedef uint8_t HAPPairingIndex;

/**
 * Methods.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Table 5-14 Methods
 */
HAP_ENUM_BEGIN(uint8_t, HAPPairingMethod) { /** Pair Setup. */
                                            kHAPPairingMethod_PairSetup = 0x00,

                                            /** Pair Setup with Auth. */
                                            kHAPPairingMethod_PairSetupWithAuth = 0x01,

                                            /** Pair Verify. */
                                            kHAPPairingMethod_PairVerify = 0x02,

                                            /** Add Pairing. */
                                            kHAPPairingMethod_AddPairing = 0x03,

                                            /** Remove Pairing. */
                                            kHAPPairingMethod_RemovePairing = 0x04,

                                            /** List Pairings. */
                                            kHAPPairingMethod_ListPairings = 0x05,

                                            /**
                                             * Pair Resume.
                                             *
                                             * @see HomeKit Accessory Protocol Specification R17
                                             *      Table 7-38 Defines Description
                                             */
                                            kHAPPairingMethod_PairResume = 0x06
} HAP_ENUM_END(uint8_t, HAPPairingMethod);

/**
 * Error Codes.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Table 5-16 Error Codes
 */
HAP_ENUM_BEGIN(uint8_t, HAPPairingError) {
    /** Generic error to handle unexpected errors. */
    kHAPPairingError_Unknown = 0x01,

    /** Setup code or signature verification failed. */
    kHAPPairingError_Authentication = 0x02,

    /**
     * Client must look at the retry delay TLV item and wait that many seconds before retrying.
     *
     * @remark Obsolete since R3.
     */
    kHAPPairingError_Backoff = 0x03,

    /** Server cannot accept any more pairings. */
    kHAPPairingError_MaxPeers = 0x04,

    /** Server reached its maximum number of authentication attempts. */
    kHAPPairingError_MaxTries = 0x05,

    /** Server pairing method is unavailable. */
    kHAPPairingError_Unavailable = 0x06,

    /** Server is busy and cannot accept a pairing request at this time. */
    kHAPPairingError_Busy = 0x07,

    /** Ownership Proof is needed but is either not provided or incorrect. */
    kHAPPairingError_OwnershipFailure = 0x08
} HAP_ENUM_END(uint8_t, HAPPairingError);

/**
 * TLV Values.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Table 5-17 TLV Values
 */
HAP_ENUM_BEGIN(uint8_t, HAPPairingTLVType) { /**
                                              * Method to use for pairing.
                                              * integer.
                                              */
                                             kHAPPairingTLVType_Method = 0x00,

                                             /**
                                              * Identifier for authentication.
                                              * UTF-8.
                                              */
                                             kHAPPairingTLVType_Identifier = 0x01,

                                             /**
                                              * 16+ bytes of random salt.
                                              * bytes.
                                              */
                                             kHAPPairingTLVType_Salt = 0x02,

                                             /**
                                              * Curve25519, SRP public key, or signed Ed25519 key.
                                              * bytes.
                                              */
                                             kHAPPairingTLVType_PublicKey = 0x03,

                                             /**
                                              * Ed25519 or SRP proof.
                                              * bytes.
                                              */
                                             kHAPPairingTLVType_Proof = 0x04,

                                             /**
                                              * Encrypted data with auth tag at end.
                                              * bytes.
                                              */
                                             kHAPPairingTLVType_EncryptedData = 0x05,

                                             /**
                                              * State of the pairing process. 1=M1, 2=M2, etc.
                                              * integer.
                                              */
                                             kHAPPairingTLVType_State = 0x06,

                                             /**
                                              * Error code. Must only be present if error code is not 0.
                                              * integer.
                                              */
                                             kHAPPairingTLVType_Error = 0x07,

                                             /**
                                              * Seconds to delay until retrying a setup code.
                                              * integer.
                                              *
                                              * @remark Obsolete since R3.
                                              */
                                             kHAPPairingTLVType_RetryDelay = 0x08,

                                             /**
                                              * X.509 Certificate.
                                              * bytes.
                                              */
                                             kHAPPairingTLVType_Certificate = 0x09,

                                             /**
                                              * Ed25519 or Apple Authentication Coprocessor signature.
                                              * bytes.
                                              */
                                             kHAPPairingTLVType_Signature = 0x0A,

                                             /**
                                              * Bit value describing permissions of the controller being added.
                                              * None (0x00): Regular user
                                              * Bit 1 (0x01): Admin that is able to add and remove pairings against the
                                              * accessory. integer.
                                              */
                                             kHAPPairingTLVType_Permissions = 0x0B,

                                             /**
                                              * Non-last fragment of data. If length is 0, it's an ACK.
                                              * bytes.
                                              *
                                              * @remark Obsolete since R7.
                                              *
                                              * @see HomeKit Accessory Protocol Specification R17
                                              *      Section 5.9 Fragmentation and Reassembly
                                              */
                                             kHAPPairingTLVType_FragmentData = 0x0C,

                                             /**
                                              * Last fragment of data.
                                              * bytes.
                                              *
                                              * @remark Obsolete since R7.
                                              *
                                              * @see HomeKit Accessory Protocol Specification R17
                                              *      Section 5.9 Fragmentation and Reassembly
                                              */
                                             kHAPPairingTLVType_FragmentLast = 0x0D,

                                             /**
                                              * Identifier to resume a session.
                                              *
                                              * @see HomeKit Accessory Protocol Specification R17
                                              *      Table 7-38 Defines Description
                                              */
                                             kHAPPairingTLVType_SessionID = 0x0E,

                                             /**
                                              * Pairing Type Flags (32 bit unsigned integer).
                                              * integer.
                                              */
                                             kHAPPairingTLVType_Flags = 0x13,

                                             /**
                                              * Ownership Proof.
                                              * bytes.
                                              */
                                             kHAPPairingTLVType_OwnershipProofToken = 0x1A,

                                             /**
                                              * Product Data information assigned to each Product Plan on the MFi Portal
                                              * upon Product Plan submission. bytes.
                                              */
                                             kHAPPairingTLVType_ProductData = 0x1C,

                                             /**
                                              * Zero-length TLV that separates different TLVs in a list.
                                              * null.
                                              */
                                             kHAPPairingTLVType_Separator = 0xFF
} HAP_ENUM_END(uint8_t, HAPPairingTLVType);

/**
 * Pairing Type Flags.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Table 5-17 Pairing Type Flags
 */
HAP_OPTIONS_BEGIN(uint32_t, HAPPairingFlag) {
    /**
     * Transient Pair Setup.
     *
     * Pair Setup M1 - M4 without exchanging public keys.
     */
    kHAPPairingFlag_Transient = (uint32_t) 1U << 4U,

    /**
     * Split Pair Setup.
     *
     * When set with kHAPPairingFlag_Transient save the SRP verifier used in this session,
     * and when only kHAPPairingFlag_Split is set, use the saved SRP verifier from previous session.
     */
    kHAPPairingFlag_Split = (uint32_t) 1U << 24U,

    /**
     * Ownership Proof Token Exchange Required.
     *
     * This flag indicates that the controller must include the ownership proof token in the payload for M5.
     */
    kHAPPairingFlag_OwnershipProofToken = (uint32_t) 1U << 30U
} HAP_OPTIONS_END(uint32_t, HAPPairingFlag);

/**
 * Reads a flags value up to UInt32 in size from a Pairing protocol TLV
 * containing its corresponding little-endian representation.
 *
 * - Excess bytes are ignored.
 *
 * @param      tlv                  TLV containing numeric value.
 *
 * @return Numeric value.
 */
HAP_RESULT_USE_CHECK
uint32_t HAPPairingReadFlags(const HAPTLV* tlv);

/**
 * Counts the number of bytes of a numeric value when serialized to a Pairing protocol TLV.
 *
 * @param      value                Numeric value.
 *
 * @return Number of bytes when serializing value to a Pairing protocol TLV.
 */
HAP_RESULT_USE_CHECK
size_t HAPPairingGetNumBytes(uint32_t value);

/**
 * Callback to be invoked for each Pairing.
 *
 * @param      context              Context.
 * @param      server               Accessory server.
 * @param      pairingIndex         Index of pairing.
 * @param[in,out] shouldContinue    True if enumeration shall continue, False otherwise. Is set to true on input.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        Otherwise.
 */
HAP_RESULT_USE_CHECK
typedef HAPError (*HAPPairingEnumerateCallback)(
        void* _Nullable context,
        HAPAccessoryServer* server,
        HAPPairingIndex pairingIndex,
        bool* shouldContinue);

/**
 * Enumerates pairings.
 *
 * @param      server               Accessory server.
 * @param      callback             Function to call on pairing.
 * @param      context              Context that is passed to the callback.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If persistent store access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPairingEnumerate(HAPAccessoryServer* server, HAPPairingEnumerateCallback callback, void* _Nullable context);

/**
 * Adds a pairing.
 *
 * @param      server               Accessory server.
 * @param      pairing              Pairing to be added.
 * @param[out] pairingIndex         Pairing index.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        On error.
 * @return kHAPError_OutOfResources If no space is available.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPairingAdd(HAPAccessoryServer* server, HAPPairing* pairing, HAPPairingIndex* _Nullable pairingIndex);

/**
 * Updates the permissions of the pairing at the pairing index.
 *
 * @param      server               Accessory server.
 * @param      pairingIndex         Pairing index.
 * @param      pairingPermissions   Permissions of pairing.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        Otherwise.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPairingUpdatePermissions(
        HAPAccessoryServer* server,
        HAPPairingIndex pairingIndex,
        uint8_t pairingPermissions);

/**
 * Gets the pairing for a pairing index.
 *
 * @param      server               Accessory server.
 * @param      pairingIndex         Pairing index.
 * @param[out] pairing              On output, if exists, pairing is stored.
 * @param[out] exists               True if pairing exists. False otherwise.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        Otherwise.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPairingGet(HAPAccessoryServer* server, HAPPairingIndex pairingIndex, HAPPairing* pairing, bool* exists);

/**
 * Remove a pairing.
 *
 * @param      server               Accessory server.
 * @param      pairing              Pairing to be removed.
 * @param      pairingIndex         Index of pairing to be removed.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        Otherwise.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPairingRemove(HAPAccessoryServer* server, HAPPairing* _Nullable pairing, HAPPairingIndex pairingIndex);

/**
 * Find a pairing based on the pairing identifier.
 *
 * @param      server               Accessory server.
 * @param      identifier           Identifier.
 * @param      numIdentifierBytes   Number of identifier bytes.
 * @param[out] pairing              Pairing.
 * @param[out] pairingIndex         Index of pairing.
 * @param[out] found                True if pairing has been found. False otherwise.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        Otherwise.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPairingFind(
        HAPAccessoryServer* server,
        const HAPPairingID* identifier,
        size_t numIdentifierBytes,
        HAPPairing* _Nullable pairing,
        HAPPairingIndex* _Nullable pairingIndex,
        bool* found);

/**
 * Purge all pairings.
 *
 * @param      server               Accessory server.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        Otherwise.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPairingRemoveAll(HAPAccessoryServer* server);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
