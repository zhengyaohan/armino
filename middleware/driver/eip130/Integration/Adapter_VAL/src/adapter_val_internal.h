/* adapter_val_internal.h
 *
 * VAL API Internal interfaces and definitions.
 */

/*****************************************************************************
* Copyright (c) 2014-2018 INSIDE Secure B.V. All Rights Reserved.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 2 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#ifndef INCLUDE_GUARD_ADAPTER_VAL_INTERNAL_H
#define INCLUDE_GUARD_ADAPTER_VAL_INTERNAL_H

#include "clib.h"                   // size_t
#include "api_val_result.h"         // ValStatus_t
#include "api_val_asset.h"          // ValAssetId_t
#include "api_val_buffers.h"        // ValSize_t
#include "api_val_asym.h"           // ValAsymKey_t, ValAsymSign_t
#include "api_val_sym.h"            // ValSymAlgo_t, ValSymMode_t
#include "adapter_alloc.h"          // Adapter_Alloc(), Adapter_Free()
#include "adapter_vex.h"            // VexToken_Command_t, VexToken_Result_t


/*----------------------------------------------------------------------------
 * VAL_ASYM_DATA_V_HEADER
 *
 * The size of the HW asymmetric crypto element header.
 * bit 15..0  = SubVectorLength (in bits)
 * bit 23..16 = SubVectorIndex
 * bit 31..24 = NrOfSubVectors
 */
#define VAL_ASYM_DATA_VHEADER       (4)

/*----------------------------------------------------------------------------
 * VAL_ASYM_DATA_SIZE_VWB
 *
 * The size of a HW asymmetric crypto element in bytes based on a bits size.
 */
#define VAL_ASYM_DATA_SIZE_VWB(x)   (VAL_ASYM_DATA_VHEADER + VAL_ASYM_DATA_SIZE_B2WB(x))


/*----------------------------------------------------------------------------
 * VAL_ASYM_CMD_MAX_HASH_SIZE
 *
 * Maximum hash message size when the hash is performed in an asymmetric
 * crypto operation.
 */
#define VAL_ASYM_CMD_MAX_HASH_SIZE  4095


/*----------------------------------------------------------------------------
 * ValSymContext_t - Context structure for symmetric crypto services.
 *
 * Note that all context information is regarded to be under control of the
 * service.
 */
typedef struct
{
    uint32_t MagicBegin;                // Magic Beginmarker
    ValSymAlgo_t Algorithm;             // Algorithm
    ValSymMode_t Mode;                  // Mode
    bool fInitDone;                     // Initialization done indication
    bool fUseTokenTemp;                 // Use intermediate state via token

    union
    {
        struct
        {
            // Asset references
            ValAssetId_t TempAssetId;   // Intermediate digest
            ValAssetId_t NotUsedAssetId;

            uint8_t Digest[VAL_SYM_ALGO_MAX_DIGEST_SIZE]; // Intermediate digest buffer

            uint64_t TotalMessageLength; // TotalMessageLength
        } Hash;
        struct
        {
            // Asset references
            ValAssetId_t TempAssetId;   // Intermediate MAC
            ValAssetId_t KeyAssetId;    // MAC key

            uint32_t KeySize;           // Key Size
            uint8_t Key[VAL_SYM_ALGO_MAX_MAC_KEY_SIZE]; // Key buffer

            uint8_t Mac[VAL_SYM_ALGO_MAX_DIGEST_SIZE]; // Initial/intermediate MAC buffer

            uint64_t TotalMessageLength; // TotalMessageLength
        } Mac;
        struct
        {
            // Asset references
            ValAssetId_t TempAssetId;   // Intermediate IV
            ValAssetId_t KeyAssetId;    // Cipher key

            bool fEncrypt;              // Encrypt indication

            uint32_t KeySize;           // Key Size
            uint8_t Key[2*VAL_SYM_ALGO_MAX_CIPHER_KEY_SIZE]; // Key buffer

            uint8_t IV[VAL_SYM_ALGO_MAX_IV_SIZE]; // Initial/intermediate IV buffer

            uint8_t Parameter[VAL_SYM_ALGO_PARAMETER_SIZE]; // System parameter buffer

            uint8_t f8_IV[16];          // IV for AES f8 mode
            uint8_t f8_SaltKey[16];     // Salt key for AES f8 mode
            uint8_t f8_SaltKeySize;     // Salt key size for AES f8 mode

            uint8_t NonceLength;        // Nonce length for ChaCha20
            uint8_t fParameter;         // Write system parameter
        } Cipher;
        struct
        {
            // Asset references
            ValAssetId_t NotUsedAssetId;
            ValAssetId_t KeyAssetId;    // Cipher key

            bool fEncrypt;              // Encrypt indication

            uint32_t KeySize;           // Key Size
            uint8_t Key[VAL_SYM_ALGO_MAX_CIPHER_KEY_SIZE]; // Key buffer

            uint8_t IV[VAL_SYM_ALGO_MAX_IV_SIZE]; // Initial/intermediate IV buffer

            // Nonce buffer [7..13] or GHASH key
            uint8_t NonceHashKey[VAL_SYM_ALGO_MAX_IV_SIZE];
            uint8_t NonceHashKeySize;  // Actual Nonce/GHASH key size

            uint8_t GCMMode;            // GCM Mode
            uint8_t TagSize;            // Tag size
        } CipherAE;
    } Service;
    uint32_t MagicEnd;                  // Magic EndMarker
} ValSymContext_t;

#define VALMARKER_SYMCONTEXT 0x5653796d


/*----------------------------------------------------------------------------
 * valInternal_ReverseMemCpy
 *
 * This function copies data from buffer to another buffer in a reverse way.
 *
 * Dest_p
 *     Pointer to the buffer in which the copied data must be written.
 *
 * Src_p
 *     Pointer to the buffer with the data that needs to be copied.
 *
 * Size
 *     The size of data that needs to be copied in bytes.
 *
 * Return Value:
 *     Pointer to the destination buffer (Dest_p).
 */
void *
valInternal_ReverseMemCpy(
        void * Dest_p,
        const void * Src_p,
        size_t Size);


/*----------------------------------------------------------------------------
 * valInternal_AsymDsaSignatureToHW
 *
 * This function converts the (EC/Ed)DSA signature to HW format.
 *
 * Signature_p
 *     Pointer to the DSA, ECDSA or EdDSA signature in application format.
 *
 * ModulusSizeBits
 *     Modulus size in bits
 *
 * Blob_p
 *     Pointer to the buffer in which the signature in HW format must be
 *     returned.
 *     Note: Function assumes buffer is big enough.
 *
 * Return Value:
 */
void
valInternal_AsymDsaSignatureToHW(
        const ValAsymSign_t * const Signature_p,
        const ValSize_t ModulusSizeBits,
        uint8_t * const Blob_p);


/*----------------------------------------------------------------------------
 * valInternal_AsymDsaSignatureFromHW
 *
 * This function converts the signature from HW to application format.
 *
 * Blob_p
 *     Pointer to the buffer with the signature in HW format.
 *
 * ModulusSizeBits
 *     Modulus size in bits
 *
 * Signature_p
 *     Pointer to the application DSA, ECDSA or EdDSA signature, in which it
 *     must be returned.
 *
 * Return Value:
 *     N/A
 */
void
valInternal_AsymDsaSignatureFromHW(
        const uint8_t * const Blob_p,
        const ValSize_t ModulusSizeBits,
        ValAsymSign_t * const Signature_p);


/*----------------------------------------------------------------------------
 * valInternal_AsymECPointToHW
 *
 * This function converts an EC point from application to HW format.
 *
 * Point_p
 *     Pointer to the EC point in application format.
 *
 * ModulusSizeBits
 *     Modulus size in bits
 *
 * BeginItem
 *     First item number.
 *
 * Items
 *     The number of items.
 *
 * Blob_p
 *     Pointer to the buffer in which the EC point in HW format must be
 *     returned.
 *     Note: Function assumes buffer is big enough.
 *
 * Return Value:
 *     N/A
 */
void
valInternal_AsymECPointToHW(
        const ValAsymECCPoint_t * const Point_p,
        const ValSize_t ModulusSizeBits,
        const uint8_t BeginItem,
        const uint8_t Items,
        uint8_t * const Blob_p);


/*----------------------------------------------------------------------------
 * valInternal_AsymECPointFromHW
 *
 * This function converts an EC point from HW to application format.
 *
 * Blob_p
 *     Pointer to the buffer with the EC point in HW format.
 *
 * ModulusSizeBits
 *     Modulus size in bits
 *
 * Point_p
 *     Pointer to the EC point in application format.
 *
 * Return Value:
 *     N/A
 */
void
valInternal_AsymECPointFromHW(
        const uint8_t * const Blob_p,
        const ValSize_t ModulusSizeBits,
        ValAsymECCPoint_t * const Point_p);


/*----------------------------------------------------------------------------
 * valInternal_AsymBigIntFromHW
 *
 * This function converts a big integer from HW to application format.
 *
 * Blob_p
 *     Pointer to the buffer with the big integer in HW format.
 *
 * ModulusSizeBits
 *     Modulus size in bits
 *
 * BigInt_p
 *     Pointer to the big integer in application format.
 *
 * Return Value:
 *     N/A
 */
void
valInternal_AsymBigIntFromHW(
        const uint8_t * const Blob_p,
        const ValSize_t ModulusSizeBits,
        ValAsymBigInt_t * const BigInt_p);


/*----------------------------------------------------------------------------
 * valInternal_AsymBigIntToHW
 *
 * This function converts a big integer from application to HW format.
 *
 * BigInt_p
 *     Pointer to the big integer in application format.
 *
 * ModulusSizeBits
 *     Modulus size in bits
 *
 * BeginItem
 *     First item number.
 *
 * Items
 *     The number of items.
 *
 * Blob_p
 *     Pointer to the buffer in which the big integer in HW format must be
 *     returned.
 *     Note: Function assumes buffer is big enough.
 *
 * Return Value:
 *     N/A
 */
void
valInternal_AsymBigIntToHW(
        const ValAsymBigInt_t * const BigInt_p,
        const ValSize_t ModulusSizeBits,
        const uint8_t BeginItem,
        const uint8_t Items,
        uint8_t * const Blob_p);


/*----------------------------------------------------------------------------
 * valInternal_AsymKeyCheck
 *
 * This function checks the provided keys are a pair or if only one
 * key (private or public) is given, the basic checks are performed.
 *
 * PubKey_p
 *     Pointer to the asymmetric crypto key structure of public key.
 *
 * PrivKey_p
 *     Pointer to the asymmetric crypto key structure of private key.
 *
 * fEcc
 *     Indication that the Key(s) are ECC based.
 *
 * Return Value:
 *     One of the ValStatus_t values.
 */
ValStatus_t
valInternal_AsymKeyCheck(
        ValAsymKey_t * const PubKey_p,
        ValAsymKey_t * const PrivKey_p,
        const bool fEcc);


/*----------------------------------------------------------------------------
 * valInternal_SymCreateTempAsset
 *
 * This function creates a temporary Asset for intermediate data that is
 * handled via the token.
 *
 * Context_p
 *     Pointer to the symmetric crypto context.
 *
 * AssetId_p
 *      Pointer to the memory location where the AssetId will be written when
 *      the entry was successfully allocated. The AssetId is needed for asset
 *      related operation with this Asset.
 *
 * Return Value:
 *     One of the ValStatus_t values.
 */
ValStatus_t
valInternal_SymCreateTempAsset(
        ValSymContextPtr_t const Context_p,
        ValAssetId_t * const AssetId_p);


/*----------------------------------------------------------------------------
 * val_ExchangeToken
 *
 * This function sends the logical VAL API token to the next driver level.
 *
 * CommandToken_p
 *     Pointer to the buffer with the service request.
 *
 * ResultToken_p
 *     Pointer to the buffer in which the service result needs to be returned.
 *
 * Return Value:
 *     One of the ValStatus_t values.
 */
ValStatus_t
val_ExchangeToken(
        VexToken_Command_t * const CommandToken_p,
        VexToken_Result_t * const  ResultToken_p);


#endif /* Include Guard */


/* end of file adapter_val_internal.h */
