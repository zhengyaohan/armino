/* eip130_token_pk.h
 *
 * Security Module Token helper functions
 * - Public key tokens related functions and definitions
 *
 * This module can convert a set of parameters into a Security Module Command
 * token, or parses a set of parameters from a Security Module Result token.
 */

/*****************************************************************************
* Copyright (c) 2014-2019 INSIDE Secure B.V. All Rights Reserved.
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

#ifndef INCLUDE_GUARD_EIP130TOKEN_PK_H
#define INCLUDE_GUARD_EIP130TOKEN_PK_H

#include "basic_defs.h"             // uint32_t, bool, inline, etc.
#include "eip130_token_common.h"    // Eip130Token_Command/Result_t

#define EIP130TOKEN_PK_CMD_MAX_HASH_SIZE  4095

// Commands for PK operations with assets
enum
{
    EIP130TOKEN_PK_ASSET_CMD_ECDH_ECDSA_KEY_CHECK = 0x01,
    EIP130TOKEN_PK_ASSET_CMD_DH_DSA_KEY_CHECK,
    EIP130TOKEN_PK_ASSET_CMD_ECDSA_SIGN = 0x06,
    EIP130TOKEN_PK_ASSET_CMD_ECDSA_VERIFY,
    EIP130TOKEN_PK_ASSET_CMD_RSA_PKCS_SIGN = 0x08,
    EIP130TOKEN_PK_ASSET_CMD_RSA_PKCS_VER,
    EIP130TOKEN_PK_ASSET_CMD_RSA_PKCS_SIGN_CRT,
    EIP130TOKEN_PK_ASSET_CMD_RSA_PSS_SIGN = 0x0C,
    EIP130TOKEN_PK_ASSET_CMD_RSA_PSS_VER,
    EIP130TOKEN_PK_ASSET_CMD_RSA_PSS_SIGN_CRT,
    EIP130TOKEN_PK_ASSET_CMD_DH_GEN_PUB_KEY = 0x10,
    EIP130TOKEN_PK_ASSET_CMD_DH_GEN_PRIV_PUB_KEY,
    EIP130TOKEN_PK_ASSET_CMD_DH_GEN_SINGLE_SH_SECR,
    EIP130TOKEN_PK_ASSET_CMD_DH_GEN_DUAL_SH_SECR,
    EIP130TOKEN_PK_ASSET_CMD_ECDH_ECDSA_GEN_PUB_KEY,
    EIP130TOKEN_PK_ASSET_CMD_ECDH_ECDSA_GEN_PRIV_PUB_KEY,
    EIP130TOKEN_PK_ASSET_CMD_ECDH_GEN_SINGLE_SH_SECR,
    EIP130TOKEN_PK_ASSET_CMD_ECDH_GEN_DUAL_SH_SECR,
    EIP130TOKEN_PK_ASSET_CMD_RSA_OAEP_WRAP_STRING = 0x18,
    EIP130TOKEN_PK_ASSET_CMD_RSA_OAEP_WRAP_HASHED,
    EIP130TOKEN_PK_ASSET_CMD_RSA_OAEP_UNWRAP_STRING,
    EIP130TOKEN_PK_ASSET_CMD_RSA_OAEP_UNWRAP_HASHED,
    EIP130TOKEN_PK_ASSET_CMD_ELGAMAL_ECC_ENCRYPT = 0x24,
    EIP130TOKEN_PK_ASSET_CMD_ELGAMAL_ECC_DECRYPT,
    EIP130TOKEN_PK_ASSET_CMD_CURVE25519_GEN_PUBKEY = 0x28,
    EIP130TOKEN_PK_ASSET_CMD_CURVE25519_GEN_KEYPAIR,
    EIP130TOKEN_PK_ASSET_CMD_CURVE25519_GEN_SHARED_SECRET,
    EIP130TOKEN_PK_ASSET_CMD_EDDSA_GEN_PUBKEY,
    EIP130TOKEN_PK_ASSET_CMD_EDDSA_GEN_KEYPAIR,
    EIP130TOKEN_PK_ASSET_CMD_EDDSA_SIGN_INITIAL,
    EIP130TOKEN_PK_ASSET_CMD_EDDSA_SIGN_UPDATE,
    EIP130TOKEN_PK_ASSET_CMD_EDDSA_SIGN_FINAL,
    EIP130TOKEN_PK_ASSET_CMD_EDDSA_VERIFY_INITIAL,
    EIP130TOKEN_PK_ASSET_CMD_EDDSA_VERIFY_FINAL,
};

// Commands for PK operations without assets
enum
{
    EIP130TOKEN_PK_CMD_NUMLOAD = 0x01,
    EIP130TOKEN_PK_CMD_NUMSETN = 0x03,
    EIP130TOKEN_PK_CMD_MODEXPE = 0x04,
    EIP130TOKEN_PK_CMD_MODEXPD,
    EIP130TOKEN_PK_CMD_MODEXPCRT,
    EIP130TOKEN_PK_CMD_ECMONTMUL = 0x0A,
    EIP130TOKEN_PK_CMD_ECCMUL,
    EIP130TOKEN_PK_CMD_ECCADD,
    EIP130TOKEN_PK_CMD_DSA_SIGN,
    EIP130TOKEN_PK_CMD_DSA_VERIFY,
    EIP130TOKEN_PK_CMD_ECDSA_SIGN,
    EIP130TOKEN_PK_CMD_ECDSA_VERIFY,
};

// Public Key sub-vector structure (Intended for reference)
struct Eip130Token_PK_SubVector
{
    uint16_t  SubVectorLength;
    uint8_t   SubVectorIndex;
    uint8_t   NrOfSubVectors;
    uint8_t   Data[4];                  // Placeholder has flexible length
};


/*----------------------------------------------------------------------------
 * Eip130Token_Command_Pk_Asset_Command
 *
 * Request to perform a PK operation with assets
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * Command
 *      Public Key operation (command) to perform.
 *
 * Nwords
 *      Number of words of the basic vector.
 *
 * Mwords
 *      Number of words of the alternate (shorter) vector.
 *
 * OtherLen
 *      Length of the other data in the token.
 *
 * KeyAssetId
 *     Asset ID of the main key asset.
 *
 * ParamAssetId (optional)
 *     Asset ID of the domain or curve parameters asset.
 *
 * IOAssetId (optional)
 *     Asset ID of the input and/or output of result asset.
 *
 * InputDataAddress
 *      Input data address.
 *
 * InputDataLengthInBytes
 *      Input data length.
 *
 * OutputDataAddress
 *      Output data address.
 *
 * OutputDataLengthInBytes
 *      Output data length.
 */
static inline void
Eip130Token_Command_Pk_Asset_Command(
        Eip130Token_Command_t * const CommandToken_p,
        const uint8_t Command,
        const uint8_t Nwords,
        const uint8_t Mwords,
        const uint8_t OtherLen,
        const uint32_t KeyAssetId,
        const uint32_t ParamAssetId,
        const uint32_t IOAssetId,
        const uint64_t InputDataAddress,
        const uint16_t InputDataLengthInBytes,
        const uint64_t OutputDataAddress,       // or Signature address
        const uint16_t OutputDataLengthInBytes) // or Signature length
{
    CommandToken_p->W[0]  = (EIP130TOKEN_OPCODE_PUBLIC_KEY << 24) |
                            (EIP130TOKEN_SUBCODE_PK_WITHASSETS << 28);
    CommandToken_p->W[2]  = (uint32_t)(Command | // PK operation to perform
                                       (Nwords << 16) |
                                       (Mwords << 24));
    CommandToken_p->W[3]  = (uint32_t)(OtherLen << 8);
    CommandToken_p->W[4]  = KeyAssetId; // asset containing x and y coordinates of pk
    CommandToken_p->W[5]  = ParamAssetId; // public key parameters:
                                          // p, a, b, n, base x, base y[, h]
    CommandToken_p->W[6]  = IOAssetId;
    CommandToken_p->W[7]  = ((MASK_12_BITS & OutputDataLengthInBytes) << 16 ) |
                             (MASK_12_BITS & InputDataLengthInBytes);
    CommandToken_p->W[8]  = (uint32_t)(InputDataAddress);
    CommandToken_p->W[9]  = (uint32_t)(InputDataAddress >> 32);
    CommandToken_p->W[10] = (uint32_t)(OutputDataAddress);
    CommandToken_p->W[11] = (uint32_t)(OutputDataAddress >> 32);
}


/*----------------------------------------------------------------------------
 * Eip130Token_Command_Pk_Asset_SetAdditionalData
 *
 * This function copies the additional data from the buffer provided by the
 * caller into the command token.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * AddData_p
 *      Additional input data address
 *
 * AddDataLengthInBytes
 *      Additional input data length
 */
static inline void
Eip130Token_Command_Pk_Asset_SetAdditionalData(
        Eip130Token_Command_t * const CommandToken_p,
        const uint8_t * const AddData_p,
        const uint8_t AddDataLengthInBytes)
{
    uint32_t offset = ((CommandToken_p->W[3] & 0xFF) + 3) & (uint32_t)~3;

    CommandToken_p->W[3] &= (uint32_t)~0xFF;
    CommandToken_p->W[3] |= (offset + AddDataLengthInBytes);
    Eip130Token_Command_WriteByteArray(CommandToken_p,
                                       (unsigned int)(12 + (offset / (uint32_t)sizeof(uint32_t))),
                                       AddData_p,
                                       (unsigned int)AddDataLengthInBytes);
}


/*----------------------------------------------------------------------------
 * Eip130Token_Command_Pk_Asset_SetAdditionalAssetId
 *
 * This function copies the specified AssetId to the additional data area of
 * the command token.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * AddAssetId
 *      Additional AssetId
 */
static inline void
Eip130Token_Command_Pk_Asset_SetAdditionalAssetId(
        Eip130Token_Command_t * const CommandToken_p,
        const uint32_t AddAssetId)
{
    uint32_t offset = ((CommandToken_p->W[3] & 0xFF) + 3) & (uint32_t)~3;

    CommandToken_p->W[3] &= (uint32_t)~0xFF;
    CommandToken_p->W[3] |= (offset + (uint32_t)sizeof(uint32_t));
    CommandToken_p->W[12 + (offset / sizeof(uint32_t))] = AddAssetId;
}


/*----------------------------------------------------------------------------
 * Eip130Token_Command_Pk_Asset_SetAdditionalLength
 *
 * This function copies the specified length to the additional data area of
 * the command token.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * AddLength
 *      Additional length information
 */
static inline void
Eip130Token_Command_Pk_Asset_SetAdditionalLength(
        Eip130Token_Command_t * const CommandToken_p,
        const uint64_t AddLength)
{
    uint32_t offset = ((CommandToken_p->W[3] & 0xFF) + 3) & (uint32_t)~3;

    CommandToken_p->W[3] &= (uint32_t)~0xFF;
    CommandToken_p->W[3] |= (offset + (2 * (uint32_t)sizeof(uint32_t)));
    CommandToken_p->W[12 + (offset / sizeof(uint32_t))] = (uint32_t)(AddLength);
    CommandToken_p->W[13 + (offset / sizeof(uint32_t))] = (uint32_t)(AddLength >> 32);
}


/*----------------------------------------------------------------------------
 * Eip130Token_Command_Pk_Asset_AddlenCorrection
 *
 * This function corrects the AddLen in the command token with provided
 * correction.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * Correction
 *      AddLen correction (bytes)
 */
static inline void
Eip130Token_Command_Pk_Asset_AddlenCorrection(
        Eip130Token_Command_t * const CommandToken_p,
        const uint8_t Correction)
{
    CommandToken_p->W[3] -= Correction;
}


/*----------------------------------------------------------------------------
 * Eip130Token_Command_Pk_Asset_SaveSharedSecret
 *
 * This function sets the indication in the token that the shared secret shall
 * be saved in an Asset for further processing for example to support special
 * key derivation methods.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * AddData_p
 *      Additional input data address
 *
 * AddDataLengthInBytes
 *      Additional input data length
 */
static inline void
Eip130Token_Command_Pk_Asset_SaveSharedSecret(
        Eip130Token_Command_t * const CommandToken_p)
{
    CommandToken_p->W[3] |= BIT_31;
}


/*----------------------------------------------------------------------------
 * Eip130Token_Command_Pk_Claim
 *
 * Request to claim the PKA engine for PK operations without assets
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * Nwords
 *      Number of words of the basic vector for NumSetN.
 *
 * Mwords
 *      Number of words of the alternate (shorter) vector for NumSetN.
 *
 * Mmask
 *      Bitmask specifying (with 1b’s) which of the first 8 vectors in Number
 *      Array are Mwords long for NumSetN.
 */
static inline void
Eip130Token_Command_Pk_Claim(
        Eip130Token_Command_t * const CommandToken_p,
        const uint8_t Nwords,
        const uint8_t Mwords,
        const uint8_t Mmask)
{
    CommandToken_p->W[0] = (EIP130TOKEN_OPCODE_PUBLIC_KEY << 24) |
                           (EIP130TOKEN_SUBCODE_PK_NOASSETS << 28);
    CommandToken_p->W[2] = (uint32_t)(EIP130TOKEN_PK_CMD_NUMSETN |
                                      (Mmask << 8) |
                                      (Nwords << 16) |
                                      (Mwords << 24));
}


/*----------------------------------------------------------------------------
 * Eip130Token_Command_Pk_NumLoad
 *
 * Request to load vectors for PK operations without assets
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * Index
 *     Selects the location(s) in which to store the input vector(s) for
 *     NumLoad.
 *
 * InputDataAddress
 *      Input data address.
 *
 * InputDataLengthInBytes
 *      Input data length.
 */
static inline void
Eip130Token_Command_Pk_NumLoad(
        Eip130Token_Command_t * const CommandToken_p,
        const uint8_t Index,
        const uint64_t InputDataAddress,
        const uint32_t InputDataLengthInBytes)
{
    CommandToken_p->W[0] = (EIP130TOKEN_OPCODE_PUBLIC_KEY << 24) |
                           (EIP130TOKEN_SUBCODE_PK_NOASSETS << 28);
    CommandToken_p->W[2] = EIP130TOKEN_PK_CMD_NUMLOAD |
                          ((MASK_4_BITS & Index) << 24);
    CommandToken_p->W[5] = (MASK_12_BITS & InputDataLengthInBytes);
    CommandToken_p->W[6] = (uint32_t)(InputDataAddress);
    CommandToken_p->W[7] = (uint32_t)(InputDataAddress >> 32);
}


/*----------------------------------------------------------------------------
 * Eip130Token_Command_ECDSA_Verify
 *
 * Request to perform a PK operations without assets
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * Command
 *      Public Key operation (command) to perform.
 *
 * PublicExponent
 *      Specifies exponent for RSA public key operation.
 *
 * InputDataAddress
 *      Input data address.
 *
 * InputDataLengthInBytes
 *      Input data length.
 *
 * OutputDataAddress
 *      Output data address.
 *
 * OutputDataLengthInBytes
 *      Output data length.
 */
static inline void
Eip130Token_Command_Pk_Operation(
        Eip130Token_Command_t * const CommandToken_p,
        const uint8_t Command,
        const uint32_t PublicExponent,
        const uint64_t InputDataAddress,
        const uint32_t InputDataLengthInBytes,
        const uint64_t OutputDataAddress,
        const uint32_t OutputDataLengthInBytes)
{
    CommandToken_p->W[0] = (EIP130TOKEN_OPCODE_PUBLIC_KEY << 24) |
                           (EIP130TOKEN_SUBCODE_PK_NOASSETS << 28);
    CommandToken_p->W[2] = (MASK_5_BITS & Command); // PK operation to perform
    CommandToken_p->W[3] = PublicExponent;
    CommandToken_p->W[5] = ((OutputDataLengthInBytes & MASK_12_BITS) << 16) |
                           (InputDataLengthInBytes & MASK_12_BITS);
    CommandToken_p->W[6] = (uint32_t)(InputDataAddress);
    CommandToken_p->W[7] = (uint32_t)(InputDataAddress >> 32);
    CommandToken_p->W[8] = (uint32_t)(OutputDataAddress);
    CommandToken_p->W[9] = (uint32_t)(OutputDataAddress >> 32);
}


/*----------------------------------------------------------------------------
 * Eip130Token_Command_Pk_Relaese
 *
 * Request to release the PKA engine for PK operations without assets
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 */
static inline void
Eip130Token_Command_Pk_Release(
        Eip130Token_Command_t * const CommandToken_p)
{
    CommandToken_p->W[0] = (EIP130TOKEN_OPCODE_PUBLIC_KEY << 24) |
                           (EIP130TOKEN_SUBCODE_PK_NOASSETS << 28);
    CommandToken_p->W[2] = EIP130TOKEN_PK_CMD_NUMSETN;
                                        // Note: Mmask M/Nwords are zeroized
}


#endif /* Include Guard */

/* end of file eip130_token_pk.h */
