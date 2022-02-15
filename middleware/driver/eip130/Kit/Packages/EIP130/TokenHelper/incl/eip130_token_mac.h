/* eip130_token_mac.h
 *
 * Security Module Token helper functions
 * - MAC token related functions and definitions
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

#ifndef INCLUDE_GUARD_EIP130TOKEN_MAC_H
#define INCLUDE_GUARD_EIP130TOKEN_MAC_H

#include "c_eip130_token.h"         // configuration options
#include "basic_defs.h"             // uint32_t, bool, inline, etc.
#include "eip130_token_common.h"    // Eip130Token_Command/Result_t


enum
{
    EIP130TOKEN_MAC_ALGORITHM_HMAC_SHA1 = 1,
    EIP130TOKEN_MAC_ALGORITHM_HMAC_SHA224,
    EIP130TOKEN_MAC_ALGORITHM_HMAC_SHA256,
    EIP130TOKEN_MAC_ALGORITHM_HMAC_SHA384,
    EIP130TOKEN_MAC_ALGORITHM_HMAC_SHA512,
    EIP130TOKEN_MAC_ALGORITHM_AES_CMAC = 8,
    EIP130TOKEN_MAC_ALGORITHM_AES_CBCMAC,
    EIP130TOKEN_MAC_ALGORITHM_ARIA_CMAC = 8,
    EIP130TOKEN_MAC_ALGORITHM_ARIA_CBCMAC,
    EIP130TOKEN_MAC_ALGORITHM_POLY1305 = 14,
};


/*----------------------------------------------------------------------------
 * Eip130Token_Command_Mac
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * MacAlgo
 *     Mac algorithm selection. Use one of EIP130TOKEN_MAC_ALGORITHM_*.
 *
 * fInit
 *     Set to true to have the MAC initialized with the default value
 *     according to the specification for the selected algorithm.
 *
 * fFinalize
 *     Set to true to have the hash finalized.
 *
 * InputDataAddress
 *     Input address of data to be MACed.
 *
 * InputDataLengthInBytes
 *     Input size/Length of the data block to be MACed.
 *     Note: For non-final MAC (fFinalize == false) this must be a multiple
 *           of the block size bytes, otherwise the request will be rejected.
 */
static inline void
Eip130Token_Command_Mac(
        Eip130Token_Command_t * const CommandToken_p,
        const uint8_t MacAlgo,
        const bool fInit,
        const bool fFinalize,
        const uint64_t InputDataAddress,
        const uint32_t InputDataLengthInBytes)
{
    CommandToken_p->W[0] = (EIP130TOKEN_OPCODE_MAC << 24);
    CommandToken_p->W[2] = InputDataLengthInBytes;
    CommandToken_p->W[3] = (uint32_t)(InputDataAddress);
    CommandToken_p->W[4] = (uint32_t)(InputDataAddress >> 32);
    CommandToken_p->W[5] = (uint32_t)((InputDataLengthInBytes + 3) & (uint32_t)~3);
    CommandToken_p->W[6] = (MASK_4_BITS & MacAlgo);
    if (!fInit)
    {
        CommandToken_p->W[6] |= BIT_4;
    }
    if (!fFinalize)
    {
        CommandToken_p->W[6] |= BIT_5;
    }
}


/*----------------------------------------------------------------------------
 * Eip130Token_Command_Mac_SetTotalMessageLength
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * TotalMessageLengthInBytes = Bits 60:0
 *     This is the total message length c.q. the length of all data blocks
 *     that are MACed, required for when MAC is finalized.
 */
static inline void
Eip130Token_Command_Mac_SetTotalMessageLength(
        Eip130Token_Command_t * const CommandToken_p,
        const uint64_t TotalMessageLengthInBytes)
{
    CommandToken_p->W[24] = (uint32_t)(TotalMessageLengthInBytes);
    CommandToken_p->W[25] = (uint32_t)(TotalMessageLengthInBytes >> 32);
}


/*----------------------------------------------------------------------------
 * Eip130Token_Command_Mac_SetASLoadKey
 *
 * This function sets the Asset Store Load location for the key and activates
 * its use. This also disables the use of the key via the token.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * AssetId
 *     Asset ID of the key to use.
 */
static inline void
Eip130Token_Command_Mac_SetASLoadKey(
        Eip130Token_Command_t * const CommandToken_p,
        const uint32_t AssetId)
{
    CommandToken_p->W[6] |= BIT_8;
    CommandToken_p->W[28] = AssetId;
}


/*----------------------------------------------------------------------------
 * Eip130Token_Command_Mac_SetASLoadMAC
 *
 * This function sets the Asset Store Load location for the MAC and activates
 * its use. This also disables the use of the MAC via the token.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * AssetId
 *     Asset ID of the final MAC asset to verify.
 */
static inline void
Eip130Token_Command_Mac_SetASLoadMAC(
        Eip130Token_Command_t * const CommandToken_p,
        const uint32_t AssetId)
{
    CommandToken_p->W[6] |= BIT_9;
    CommandToken_p->W[8] = AssetId;
}


/*----------------------------------------------------------------------------
 * Eip130Token_Command_Mac_SetASTempMAC
 *
 * This function sets the Asset Store Save location for the MAC and activates
 * its use.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * AssetId
 *     Asset ID of the intermediate MAC asset.
 */
static inline void
Eip130Token_Command_Mac_SetASTempMAC(
        Eip130Token_Command_t * const CommandToken_p,
        const uint32_t AssetId)
{
    CommandToken_p->W[7] = AssetId;
}


/*----------------------------------------------------------------------------
 * Eip130Token_Command_Mac_CopyKey
 *
 * This function copies the key from the buffer provided by the caller into
 * the command token.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * Key_p
 *     Pointer to the buffer with the key.
 *
 * KeyLengthInBytes
 *     The size of the key to copy.
 */
static inline void
Eip130Token_Command_Mac_CopyKey(
        Eip130Token_Command_t * const CommandToken_p,
        const uint8_t * const Key_p,
        const uint32_t KeyLengthInBytes)
{
    CommandToken_p->W[6] |= ((MASK_8_BITS & KeyLengthInBytes) << 16);

    Eip130Token_Command_WriteByteArray(CommandToken_p,
                                       28,
                                       Key_p, KeyLengthInBytes);
}


/*----------------------------------------------------------------------------
 * Eip130Token_Command_Mac_CopyMAC
 *
 * This function copies the MAC from the buffer provided by the caller into
 * the command token. The requested number of bytes are copied (length depends
 * on the algorithm that will be used).
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * MAC_p
 *     Pointer to the MAC buffer.
 *
 * MACLenInBytes
 *     The size of the MAC to copy.
 */
static inline void
Eip130Token_Command_Mac_CopyMAC(
        Eip130Token_Command_t * const CommandToken_p,
        const uint8_t * const MAC_p,
        const uint32_t MACLenInBytes)
{
    Eip130Token_Command_WriteByteArray(CommandToken_p, 8,
                                       MAC_p, MACLenInBytes);
}


/*----------------------------------------------------------------------------
 * Eip130Token_Result_Mac_CopyMAC
 *
 * This function copies the MAC from the result token to the buffer provided
 * by the caller. The requested number of bytes are copied (length depends on
 * the algorithm that was used).
 *
 * The MAC is written to the destination buffer, Byte 0 first.
 *
 * ResultToken_p
 *     Pointer to the result token buffer.
 *
 * MACLenInBytes
 *     The size of the MAC to copy.
 *
 * MAC_p
 *     Pointer to the MAC buffer.
 */
static inline void
Eip130Token_Result_Mac_CopyMAC(
        Eip130Token_Result_t * const ResultToken_p,
        const uint32_t MACLenInBytes,
        uint8_t * MAC_p)
{
    Eip130Token_Result_ReadByteArray(ResultToken_p, 2, MACLenInBytes, MAC_p);
}


#endif /* Include Guard */

/* end of file eip130_token_mac.h */
