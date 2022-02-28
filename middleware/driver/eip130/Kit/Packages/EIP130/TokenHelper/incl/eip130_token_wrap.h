/* eip130_token_wrap.h
 *
 * Security Module Token helper functions
 * - AES Wrap/Unwrap token related functions and definitions
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

#ifndef INCLUDE_GUARD_EIP130TOKEN_WRAP_H
#define INCLUDE_GUARD_EIP130TOKEN_WRAP_H

#include "basic_defs.h"             // uint32_t, bool, inline, etc.
#include "cs_eip130_token.h"        // EIP130TOKEN_ENABLE_*
#include "eip130_token_common.h"    // Eip130Token_Command/Result_t


/*----------------------------------------------------------------------------
 * Eip130Token_Command_WrapUnwrap
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * fWrap
 *     true  = Wrap
 *     false = Unwrap
 *
 * InputDataAddress
 *     Address of data to be wrapped or unwrapped.
 *
 * InputDataLengthInBytes
 *     Number of bytes to process.
 *     Must be at least 16, at most 1024 and always a multiple of 8.
 *
 * OutputDataAddress
 *     Address of the buffer for data that has been wrapped or unwrapped.
 */
static inline void
Eip130Token_Command_WrapUnwrap(
        Eip130Token_Command_t * const CommandToken_p,
        const bool fWrap,
        const uint64_t InputDataAddress,
        const uint16_t InputDataLengthInBytes,
        const uint64_t OutputDataAddress)
{
    CommandToken_p->W[0] = (EIP130TOKEN_OPCODE_AESWRAP << 24);
    CommandToken_p->W[2] = (MASK_11_BITS & InputDataLengthInBytes);
    CommandToken_p->W[3] = (uint32_t)(InputDataAddress);
    CommandToken_p->W[4] = (uint32_t)(InputDataAddress >> 32);
    CommandToken_p->W[5] = (uint32_t)(OutputDataAddress);
    CommandToken_p->W[6] = (uint32_t)(OutputDataAddress >> 32);
    if (fWrap)
    {
        CommandToken_p->W[7] = BIT_15;
    }
    else
    {
        CommandToken_p->W[7] = 0;
    }
}


/*----------------------------------------------------------------------------
 * Eip130Token_Command_WrapUnwrap_SetKeyLength
 *
 * This function sets the coded length of the AES key.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * KeyLengthInBytes
 *     The length of the key.
 */
static inline void
Eip130Token_Command_WrapUnwrap_SetKeyLength(
        Eip130Token_Command_t * const CommandToken_p,
        const unsigned int KeyLengthInBytes)
{
    uint32_t CodedKeyLen = 0;

    // key length only needed for AES
    switch (KeyLengthInBytes)
    {
    case 128 / 8:
        CodedKeyLen = 1;
        break;

    case 192 / 8:
        CodedKeyLen = 2;
        break;

    case 256 / 8:
        CodedKeyLen = 3;
        break;

    default:
        break;
    }
    CommandToken_p->W[7] |= (CodedKeyLen << 16);
}


/*----------------------------------------------------------------------------
 * Eip130Token_Command_WrapUnwrap_SetASLoadKey
 *
 * This function sets the Asset Store Load location for the key and activates
 * its use. This also disables the use of the key via the token.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * AssetId
 *      Asset ID of the Key Encryption Key Asset to use.
 */
static inline void
Eip130Token_Command_WrapUnwrap_SetASLoadKey(
        Eip130Token_Command_t * const CommandToken_p,
        const uint32_t AssetId)
{
    CommandToken_p->W[7] |= BIT_8;
    CommandToken_p->W[8] = AssetId;
}


/*----------------------------------------------------------------------------
 * Eip130Token_Command_WrapUnwrap_CopyKey
 *
 * This function copies the key from the buffer provided by the caller into
 * the command token.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * Key_p
 *     Key data to copy to the token.
 *
 * KeyLengthInBytes
 *     The length of the key data.
 */
static inline void
Eip130Token_Command_WrapUnwrap_CopyKey(
        Eip130Token_Command_t * const CommandToken_p,
        const uint8_t * const Key_p,
        const uint32_t KeyLengthInBytes)
{
    CommandToken_p->W[7] &= ~BIT_8;
    Eip130Token_Command_WriteByteArray(CommandToken_p, 8,
                                       Key_p, KeyLengthInBytes);
}


/*----------------------------------------------------------------------------
 * Eip130Token_Result_WrapUnwrap_GetDataLength
 *
 * This function returns the length of the result object from the result token.
 *
 * ResultToken_p
 *     Pointer to the result token buffer.
 *
 * DataLengthInBytes_p
 *      Pointer to the variable in which the data length must be returned.
 */
static inline void
Eip130Token_Result_WrapUnwrap_GetDataLength(
        const Eip130Token_Result_t * const ResultToken_p,
        uint32_t * const DataLengthInBytes_p)
{
    *DataLengthInBytes_p = (MASK_11_BITS & ResultToken_p->W[1]);
}


/*----------------------------------------------------------------------------
 * Eip130Token_Command_EncryptedVectorPKI
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * AssetId
 *      Asset ID of the Asset to encrypt.
 *
 * AssetNumber
 *      Asset number of the KDK Static Asset.
 *
 * OutputDataAddress
 *     Output data Address.
 *
 * OutputDataLengthInBytes
 *     Output data length.
 */
#ifdef EIP130TOKEN_ENABLE_ENCRYPTED_VECTOR
static inline void
Eip130Token_Command_EncryptedVectorPKI(
        Eip130Token_Command_t * const CommandToken_p,
        const uint32_t AssetId,
        const uint16_t AssetNumber,
        const uint64_t OutputDataAddress,
        const uint16_t OutputDataLengthInBytes)
{
    CommandToken_p->W[0] = (EIP130TOKEN_OPCODE_AESWRAP << 24) |
                           (EIP130TOKEN_SUBCODE_ENCRYPTVECTOR << 28);
    CommandToken_p->W[2] = AssetId;
    CommandToken_p->W[3] = ((MASK_6_BITS & AssetNumber) << 16) |
                           (MASK_11_BITS& OutputDataLengthInBytes);
    CommandToken_p->W[4] = (uint32_t)(OutputDataAddress);
    CommandToken_p->W[5] = (uint32_t)(OutputDataAddress >> 32);
}
#endif


/*----------------------------------------------------------------------------
 * Eip130Token_Result_EncryptedVectorPKI_GetDataLength
 *
 * This function returns the length of the result object from the result token.
 *
 * ResultToken_p
 *     Pointer to the result token buffer.
 *
 * DataLengthInBytes_p
 *      Pointer to the variable in which the data length must be returned.
 */
#ifdef EIP130TOKEN_ENABLE_ENCRYPTED_VECTOR
static inline void
Eip130Token_Result_EncryptedVectorPKI_GetDataLength(
        const Eip130Token_Result_t * const ResultToken_p,
        uint32_t * const DataLengthInBytes_p)
{
    *DataLengthInBytes_p = (MASK_11_BITS & ResultToken_p->W[1]);
}
#endif


#endif /* Include Guard */

/* end of file eip130_token_wrap.h */
