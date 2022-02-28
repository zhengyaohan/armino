/* eip130_token_hash.h
 *
 * Security Module Token helper functions
 * - Hash token related functions and definitions
 *
 * This module can convert a set of parameters into a Security Module Command
 * token, or parses a set of parameters from a Security Module Result token.
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

#ifndef INCLUDE_GUARD_EIP130TOKEN_HASH_H
#define INCLUDE_GUARD_EIP130TOKEN_HASH_H

#include "c_eip130_token.h"         // configuration options
#include "basic_defs.h"             // uint32_t, bool, inline, etc.
#include "eip130_token_common.h"    // Eip130Token_Command/Result_t


enum
{
    EIP130TOKEN_HASH_ALGORITHM_SHA1 = 1,
    EIP130TOKEN_HASH_ALGORITHM_SHA224,
    EIP130TOKEN_HASH_ALGORITHM_SHA256,
    EIP130TOKEN_HASH_ALGORITHM_SHA384,
    EIP130TOKEN_HASH_ALGORITHM_SHA512,
};


/*----------------------------------------------------------------------------
 * Eip130Token_Command_Hash
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * HashAlgo
 *     Hash algorithm selection. Use one of EIP130TOKEN_HASH_ALGORITHM_*
 *
 * fInitWithDefault
 *     Set to true to have the digest initialized with the default value
 *     according to the specification for the selected hash algorithm.
 *
 * fFinalize
 *     Set to true to have the hash finalized.
 *
 * InputDataAddress
 *     Address of data to be hashed.
 *
 * InputDataLengthInBytes
 *     Size/Length of the data block to be hashed.
 *     Note: For non-final hash (fFinalize == false) this must be a multiple
 *           of 64 bytes, otherwise the request will be rejected.
 *           For final hash, this can be any value.
 */
static inline void
Eip130Token_Command_Hash(
        Eip130Token_Command_t * const CommandToken_p,
        const uint8_t HashAlgo,
        const bool fInitWithDefault,
        const bool fFinalize,
        const uint64_t InputDataAddress,
        const uint32_t InputDataLengthInBytes)
{
    CommandToken_p->W[0] = (EIP130TOKEN_OPCODE_HASH << 24);
    CommandToken_p->W[2] = InputDataLengthInBytes;
    CommandToken_p->W[3] = (uint32_t)(InputDataAddress);
    CommandToken_p->W[4] = (uint32_t)(InputDataAddress >> 32);
    CommandToken_p->W[5] = InputDataLengthInBytes;
    CommandToken_p->W[6] = (MASK_4_BITS & HashAlgo);
    if (!fInitWithDefault)
    {
        CommandToken_p->W[6] |= BIT_4;
    }
    if (!fFinalize)
    {
        CommandToken_p->W[6] |= BIT_5;
    }
}


/*----------------------------------------------------------------------------
 * Eip130Token_Command_Hash_SetTempDigestASID
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * AssetId
 *     Asset ID of the intermediate digest (MAC) asset.
 */
static inline void
Eip130Token_Command_Hash_SetTempDigestASID(
        Eip130Token_Command_t * const CommandToken_p,
        const uint32_t AssetId)
{
    CommandToken_p->W[7] = AssetId;
}


/*----------------------------------------------------------------------------
 * Eip130Token_Command_Hash_SetTotalMessageLength
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * TotalMessageLengthInBytes = Bits 60:0
 *     This is the total message length c.q. the length of all data blocks
 *     that are hashed, required for when hash is finalized.
 */
static inline void
Eip130Token_Command_Hash_SetTotalMessageLength(
        Eip130Token_Command_t * const CommandToken_p,
        const uint64_t TotalMessageLengthInBytes)
{
    CommandToken_p->W[24] = (uint32_t)(TotalMessageLengthInBytes);
    CommandToken_p->W[25] = (uint32_t)(TotalMessageLengthInBytes >> 32);
}


/*----------------------------------------------------------------------------
 * Eip130Token_Command_Hash_CopyDigest
 *
 * This function copies the digest from the buffer provided by the caller into
 * the command token. The requested number of bytes are copied (length depends
 * on the algorithm that will be used).
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * Digest_p
 *     Pointer to the digest buffer.
 *
 * DigestLenInBytes
 *     The size of the digest to copy.
 */
static inline void
Eip130Token_Command_Hash_CopyDigest(
        Eip130Token_Command_t * const CommandToken_p,
        const uint8_t * const Digest_p,
        const uint32_t DigestLenInBytes)
{
    Eip130Token_Command_WriteByteArray(CommandToken_p, 8,
                                       Digest_p, DigestLenInBytes);
}


/*----------------------------------------------------------------------------
 * Eip130Token_Result_Hash_CopyDigest
 *
 * This function copies the digest from the result token to the buffer
 * provided by the caller. The requested number of bytes are copied (length
 * depends on the algorithm that was used).
 *
 * The digest is written to the destination buffer, Byte 0 first.
 *
 * ResultToken_p
 *     Pointer to the result token buffer.
 *
 * DigestLenInBytes
 *     The size of the digest to copy.
 *
 * Digest_p
 *     Pointer to the digest buffer.
 */
static inline void
Eip130Token_Result_Hash_CopyDigest(
        Eip130Token_Result_t * const ResultToken_p,
        const uint32_t DigestLenInBytes,
        uint8_t * Digest_p)
{
    Eip130Token_Result_ReadByteArray(ResultToken_p, 2,
                                     DigestLenInBytes, Digest_p);
}


#endif /* Include Guard */

/* end of file eip130_token_hash.h */
