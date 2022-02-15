/* eip130_token_crypto.h
 *
 * Security Module Token helper functions
 * - Crypto token related functions and definitions
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

#ifndef INCLUDE_GUARD_EIP130TOKEN_CRYPTO_H
#define INCLUDE_GUARD_EIP130TOKEN_CRYPTO_H

#include "basic_defs.h"             // uint32_t, bool, inline, etc.
#include "cs_eip130_token.h"        // EIP130TOKEN_ENABLE_*
#include "eip130_token_common.h"    // Eip130Token_Command/Result_t


enum
{
    EIP130TOKEN_CRYPTO_ALGO_AES      = 0,
    EIP130TOKEN_CRYPTO_ALGO_DES      = 1,
    EIP130TOKEN_CRYPTO_ALGO_3DES     = 2,
    EIP130TOKEN_CRYPTO_ALGO_CHACHA20 = 7,
    EIP130TOKEN_CRYPTO_ALGO_SM4      = 8,
    EIP130TOKEN_CRYPTO_ALGO_ARIA     = 9,
};

enum
{
    EIP130TOKEN_CRYPTO_MODE_ECB     = 0,
    EIP130TOKEN_CRYPTO_MODE_CBC     = 1,
    EIP130TOKEN_CRYPTO_MODE_CTR     = 2,
    EIP130TOKEN_CRYPTO_MODE_ICM     = 3,
    EIP130TOKEN_CRYPTO_MODE_f8      = 4,
    EIP130TOKEN_CRYPTO_MODE_CCM     = 5,
    EIP130TOKEN_CRYPTO_MODE_XTS     = 6,
    EIP130TOKEN_CRYPTO_MODE_GCM     = 7,
    EIP130TOKEN_CRYPTO_MODE_ENCRYPT = 0,
    EIP130TOKEN_CRYPTO_MODE_AEAD    = 1,
};


/*----------------------------------------------------------------------------
 * Eip130Token_Command_Crypto_Operation
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * Algorithm
 *     Operation algorithm. Must be one of EIP130TOKEN_CRYPTO_ALGO_*
 *
 * Mode
 *     Mode of operation. Must be one of EIP130TOKEN_CRYPTO_MODE_*
 *
 * fEncrypt
 *     true = Encrypt
 *     false = Decrypt
 *
 * DataLength
 *     Number of bytes to process.
 *     Must be a multiple of the blocksize.
 */
static inline void
Eip130Token_Command_Crypto_Operation(
        Eip130Token_Command_t * const CommandToken_p,
        const uint8_t Algorithm,
        const uint8_t Mode,
        const bool fEncrypt,
        const uint32_t DataLengthInBytes)
{
    CommandToken_p->W[0] = (EIP130TOKEN_OPCODE_ENCRYPTION << 24);
    CommandToken_p->W[2] = DataLengthInBytes;

    // Algorithm, Mode and direction
    CommandToken_p->W[11] = (MASK_4_BITS & Algorithm) +
                            ((MASK_4_BITS & Mode) << 4);
    if (fEncrypt)
    {
        CommandToken_p->W[11] |= BIT_15;
    }
}


/*----------------------------------------------------------------------------
 * Eip130Token_Command_Crypto_SetDataAddresses
 *
 * This function sets the Data address for Input and Output to use.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * InputDataAddress
 *     Input data Address.
 *
 * InputDataLengthInBytes
 *     Input data length.
 *
 * OutputDataAddress
 *     Output data Address.
 *
 * OutputDataLengthInBytes
 *     Output data length.
 */
static inline void
Eip130Token_Command_Crypto_SetDataAddresses(
        Eip130Token_Command_t * const CommandToken_p,
        const uint64_t InputDataAddress,
        const uint32_t InputDataLengthInBytes,
        const uint64_t OutputDataAddress,
        const uint32_t OutputDataLengthInBytes)
{
    CommandToken_p->W[3] = (uint32_t)(InputDataAddress);
    CommandToken_p->W[4] = (uint32_t)(InputDataAddress >> 32);
    CommandToken_p->W[5] = InputDataLengthInBytes;
    CommandToken_p->W[6] = (uint32_t)(OutputDataAddress);
    CommandToken_p->W[7] = (uint32_t)(OutputDataAddress >> 32);
    CommandToken_p->W[8] = OutputDataLengthInBytes;
}


/*----------------------------------------------------------------------------
 * Eip130Token_Command_Crypto_SetKeyLength
 *
 * This function sets the coded length of the AES or ARIA key.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * KeyLengthInBytes
 *     Key length.
 */
static inline void
Eip130Token_Command_Crypto_SetKeyLength(
        Eip130Token_Command_t * const CommandToken_p,
        const uint32_t KeyLengthInBytes)
{
    uint32_t CodedKeyLen = 0;

    // Coded key length only needed for AES and ARIA
    switch (KeyLengthInBytes)
    {
    case (128 / 8):
        CodedKeyLen = 1;
        break;

    case (192 / 8):
        CodedKeyLen = 2;
        break;

    case (256 / 8):
        CodedKeyLen = 3;
        break;

    default:
        break;
    }
    CommandToken_p->W[11] |= (CodedKeyLen << 16);
}


#ifdef EIP130TOKEN_ENABLE_SYM_ALGO_CHACHA20
/*----------------------------------------------------------------------------
 * Eip130Token_Command_Crypto_ChaCha20_SetKeyLength
 *
 * This function sets the coded length of the ChaCha20 key.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * KeyLengthInBytes
 *     Key length.
 */
static inline void
Eip130Token_Command_Crypto_ChaCha20_SetKeyLength(
        Eip130Token_Command_t * const CommandToken_p,
        const uint32_t KeyLengthInBytes)
{
    uint32_t CodedKeyLen = 0;

    // Key length for ChaCha20
    if (KeyLengthInBytes == (128 / 8))
    {
        CodedKeyLen = 1;
    }
    CommandToken_p->W[11] |= (CodedKeyLen << 16);
}
#endif


/*----------------------------------------------------------------------------
 * Eip130Token_Command_Crypto_SetASLoadKey
 *
 * This function sets the Asset Store Load location for the key and activates
 * its use. This also disables the use of the key via the token.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * AssetId
 *     Key Asset Identifier.
 */
static inline void
Eip130Token_Command_Crypto_SetASLoadKey(
        Eip130Token_Command_t * const CommandToken_p,
        const uint32_t AssetId)
{
    CommandToken_p->W[11] |= BIT_8;
    CommandToken_p->W[17]  = AssetId;
}


/*----------------------------------------------------------------------------
 * Eip130Token_Command_Crypto_SetASLoadIV
 *
 * This function sets the Asset Store Load location for the IV and
 * activates its use. This also disables the use of the IV via the token.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * AssetId
 *     Input IV Asset Identifier.
 */
static inline void
Eip130Token_Command_Crypto_SetASLoadIV(
        Eip130Token_Command_t * const CommandToken_p,
        const uint32_t AssetId)
{
    CommandToken_p->W[11] |= BIT_9;
    CommandToken_p->W[13]  = AssetId;
}


/*----------------------------------------------------------------------------
 * Eip130Token_Command_Crypto_SetASSaveIV
 *
 * This function sets the Asset Store Save location for the IV and
 * activates its use.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * AssetId
 *     Output IV Asset Identifier.
 */
static inline void
Eip130Token_Command_Crypto_SetASSaveIV(
        Eip130Token_Command_t * const CommandToken_p,
        const uint32_t AssetId)
{
    CommandToken_p->W[11] |= BIT_12;
    CommandToken_p->W[12]  = AssetId;
}


#if defined(EIP130TOKEN_ENABLE_SYM_ALGO_AES_GCM) || \
    defined(EIP130TOKEN_ENABLE_SYM_ALGO_ARIA_GCM)
/*----------------------------------------------------------------------------
 * Eip130Token_Command_Crypto_SetGcmMode
 *
 * This function sets the GCM operation mode to use.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * GCMMode
 *     GCM operation mode.
 */
static inline void
Eip130Token_Command_Crypto_SetGcmMode(
        Eip130Token_Command_t * const CommandToken_p,
        const uint8_t GCMMode)
{
    CommandToken_p->W[11] |= (uint32_t)(GCMMode << 13);
}
#endif


#ifdef EIP130TOKEN_ENABLE_SYM_ALGO_CHACHA20
/*----------------------------------------------------------------------------
 * Eip130Token_Command_Crypto_SetNonceLength
 *
 * This function sets the Nonce length for ChaCha20.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * NonceLengthInBytes
 *     Nonce length.
 */
static inline void
Eip130Token_Command_Crypto_SetNonceLength(
        Eip130Token_Command_t * const CommandToken_p,
        const uint8_t NonceLengthInBytes)
{
    CommandToken_p->W[11] |= ((MASK_4_BITS & NonceLengthInBytes) << 20);
}
#endif


/*----------------------------------------------------------------------------
 * Eip130Token_Command_Crypto_CopyKey
 *
 * This function copies the key from the buffer provided by the caller into
 * the command token.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * Key_p
 *     Pointer to the key buffer.
 *
 * KeyLengthInBytes
 *     Key length.
 */
static inline void
Eip130Token_Command_Crypto_CopyKey(
        Eip130Token_Command_t * const CommandToken_p,
        const uint8_t * const Key_p,
        const uint32_t KeyLengthInBytes)
{
    Eip130Token_Command_WriteByteArray(CommandToken_p, 17,
                                       Key_p, KeyLengthInBytes);
}


/*----------------------------------------------------------------------------
 * Eip130Token_Command_Crypto_CopyIV
 *
 * This function copies the IV from the buffer provided by the caller into
 * the command token. The IV length is always 16 bytes.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * IV_p
 *     Pointer to the IV buffer.
 */
static inline void
Eip130Token_Command_Crypto_CopyIV(
        Eip130Token_Command_t * const CommandToken_p,
        const uint8_t * const IV_p)
{
    Eip130Token_Command_WriteByteArray(CommandToken_p, 13, IV_p, 16);
}


#ifdef EIP130TOKEN_ENABLE_SYM_ALGO_SM4
/*----------------------------------------------------------------------------
 * Eip130Token_Command_Crypto_CopyParameter
 *
 * This function copies the system parameter from the buffer provided by the caller into
 * the command token. The system parameter length is always 16 bytes.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * Parameter_p
 *     Pointer to the system parameter buffer.
 */
static inline void
Eip130Token_Command_Crypto_CopyParameter(
        Eip130Token_Command_t * const CommandToken_p,
        const uint8_t * const Parameter_p)
{
    CommandToken_p->W[11] |= BIT_11;
    Eip130Token_Command_WriteByteArray(CommandToken_p, 37, Parameter_p, 16);
}
#endif


#if defined(EIP130TOKEN_ENABLE_SYM_ALGO_AES_CCM) || \
    defined(EIP130TOKEN_ENABLE_SYM_ALGO_AES_GCM) || \
    defined(EIP130TOKEN_ENABLE_SYM_ALGO_ARIA_CCM) || \
    defined(EIP130TOKEN_ENABLE_SYM_ALGO_ARIA_GCM) || \
    defined(EIP130TOKEN_ENABLE_SYM_ALGO_CHACHA20)
/*----------------------------------------------------------------------------
 * Eip130Token_Command_Crypto_SetADAddress
 *
 * This function sets the Associated Data address and length provided by the
 * caller into the command token.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * InputDataAddress
 *     Input data address of the Associated Data (AAD).
 *
 * DataLengthInBytes
 *     Associated Data length.
 */
static inline void
Eip130Token_Command_Crypto_SetADAddress(
        Eip130Token_Command_t * const CommandToken_p,
        const uint64_t InputDataAddress,
        const uint16_t DataLengthInBytes)
{
    CommandToken_p->W[9]  = (uint32_t)(InputDataAddress);
    CommandToken_p->W[10] = (uint32_t)(InputDataAddress >> 32);
    CommandToken_p->W[25] = DataLengthInBytes;
}
#endif


#if defined(EIP130TOKEN_ENABLE_SYM_ALGO_AES_CCM) || \
    defined(EIP130TOKEN_ENABLE_SYM_ALGO_AES_GCM) || \
    defined(EIP130TOKEN_ENABLE_SYM_ALGO_ARIA_CCM) || \
    defined(EIP130TOKEN_ENABLE_SYM_ALGO_ARIA_GCM) || \
    defined(EIP130TOKEN_ENABLE_SYM_ALGO_CHACHA20)
/*----------------------------------------------------------------------------
 * Eip130Token_Command_Crypto_CopyNonce
 *
 * This function copies the Nonce from the buffer provided by the caller into
 * the command token.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * Nonce_p
 *     Pointer to the nonce buffer.
 *
 * NonceLengthInBytes
 *     Nonce length.
 */
static inline void
Eip130Token_Command_Crypto_CopyNonce(
        Eip130Token_Command_t * const CommandToken_p,
        const uint8_t * const Nonce_p,
        const uint32_t NonceLengthInBytes)
{
    CommandToken_p->W[11] |= ((MASK_4_BITS & NonceLengthInBytes) << 20);

    Eip130Token_Command_WriteByteArray(CommandToken_p, 29,
                                       Nonce_p, NonceLengthInBytes);
}
#endif


#if defined(EIP130TOKEN_ENABLE_SYM_ALGO_AES_GCM) || \
    defined(EIP130TOKEN_ENABLE_SYM_ALGO_ARIA_GCM)
/*----------------------------------------------------------------------------
 * Eip130Token_Command_Crypto_CopyHashKey
 *
 * This function copies the Hash Key from the buffer provided by the caller
 * into the command token.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * HashKey_p
 *     Pointer to the hash key buffer.
 *
 * HashKeyLengthInBytes
 *     Hash key length.
 */
static inline void
Eip130Token_Command_Crypto_CopyHashKey(
        Eip130Token_Command_t * const CommandToken_p,
        const uint8_t * const HashKey_p,
        const uint32_t HashKeyLengthInBytes)
{
    Eip130Token_Command_WriteByteArray(CommandToken_p, 26,
                                       HashKey_p, HashKeyLengthInBytes);
}
#endif


#if defined(EIP130TOKEN_ENABLE_SYM_ALGO_AES_CCM) || \
    defined(EIP130TOKEN_ENABLE_SYM_ALGO_AES_GCM) || \
    defined(EIP130TOKEN_ENABLE_SYM_ALGO_ARIA_CCM) || \
    defined(EIP130TOKEN_ENABLE_SYM_ALGO_ARIA_GCM) || \
    defined(EIP130TOKEN_ENABLE_SYM_ALGO_CHACHA20)
/*----------------------------------------------------------------------------
 * Eip130Token_Command_Crypto_CopyTag
 *
 * This function copies the Tag from the buffer provided by the caller into
 * the command token.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * Tag_p
 *     Pointer to the TAG buffer.
 *
 * TagLengthInBytes
 *     TAG length.
 */
static inline void
Eip130Token_Command_Crypto_CopyTag(
        Eip130Token_Command_t * const CommandToken_p,
        const uint8_t * const Tag_p,
        const uint32_t TagLengthInBytes)
{
    CommandToken_p->W[11] |= ((MASK_5_BITS & TagLengthInBytes) << 24);

    if ((CommandToken_p->W[11] & BIT_15) == 0)
    {
        // Decrypt operation, so tag is input
        Eip130Token_Command_WriteByteArray(CommandToken_p, 33,
                                           Tag_p, TagLengthInBytes);
    }
}
#endif


#ifdef EIP130TOKEN_ENABLE_SYM_ALGO_AES_F8
/*----------------------------------------------------------------------------
 * Eip130Token_Command_Crypto_Copyf8SaltKey
 *
 * This function copies the f8 salt key from the buffer provided by the caller
 * into the command token.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * Key_p
 *     Pointer to the f8 salt key buffer.
 *
 * KeyLengthInBytes
 *     The f8 salt key length.
 */
static inline void
Eip130Token_Command_Crypto_Copyf8SaltKey(
        Eip130Token_Command_t * const CommandToken_p,
        const uint8_t * const Key_p,
        const uint32_t KeyLengthInBytes)
{
    CommandToken_p->W[11] |= ((MASK_5_BITS & KeyLengthInBytes) << 24);

    Eip130Token_Command_WriteByteArray(CommandToken_p, 29,
                                       Key_p, KeyLengthInBytes);
}
#endif


#ifdef EIP130TOKEN_ENABLE_SYM_ALGO_AES_F8
/*----------------------------------------------------------------------------
 * Eip130Token_Command_Crypto_Copyf8IV
 *
 * This function copies the f8 IV from the buffer provided by the caller into
 * the command token. The f8 IV length is always 16 bytes.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * f8IV_p
 *     Pointer to the f8 IV buffer.
 */
static inline void
Eip130Token_Command_Crypto_Copyf8IV(
        Eip130Token_Command_t * const CommandToken_p,
        const uint8_t * const f8IV_p)
{
    Eip130Token_Command_WriteByteArray(CommandToken_p, 25, f8IV_p, 16);
}
#endif


/*----------------------------------------------------------------------------
 * Eip130Token_Result_Crypto_CopyIV
 *
 * This function copies the IV from the result token to the buffer
 * provided by the caller. The IV length is always 16 bytes.
 *
 * ResultToken_p
 *     Pointer to the result token buffer.
 *
 * Dest_p
 *      Pointer to the buffer to copy the IV to.
 */
static inline void
Eip130Token_Result_Crypto_CopyIV(
        const Eip130Token_Result_t * const ResultToken_p,
        uint8_t * Dest_p)
{
    Eip130Token_Result_ReadByteArray(ResultToken_p, 2, 16, Dest_p);
}


#if defined(EIP130TOKEN_ENABLE_SYM_ALGO_AES_CCM) || \
    defined(EIP130TOKEN_ENABLE_SYM_ALGO_AES_GCM) || \
    defined(EIP130TOKEN_ENABLE_SYM_ALGO_ARIA_CCM) || \
    defined(EIP130TOKEN_ENABLE_SYM_ALGO_ARIA_GCM) || \
    defined(EIP130TOKEN_ENABLE_SYM_ALGO_CHACHA20)
/*----------------------------------------------------------------------------
 * Eip130Token_Result_Crypto_CopyTag
 *
 * This function copies the Tag from the result token to the buffer
 * provided by the caller. The Tag length is always 16 bytes.
 *
 * ResultToken_p
 *     Pointer to the result token buffer.
 *
 * Dest_p
 *      Pointer to the buffer to copy the Tag to.
 */
static inline void
Eip130Token_Result_Crypto_CopyTag(
        const Eip130Token_Result_t * const ResultToken_p,
        uint8_t * Dest_p)
{
    Eip130Token_Result_ReadByteArray(ResultToken_p, 6, 16, Dest_p);
}
#endif


#endif /* Include Guard */

/* end of file eip130_token_crypto.h */
