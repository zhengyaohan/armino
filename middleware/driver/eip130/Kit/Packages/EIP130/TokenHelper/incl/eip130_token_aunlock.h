/* eip130_token_aunlock.h
 *
 * Security Module Token helper functions
 * - Authenticated unlock tokens related functions and definitions
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

#ifndef INCLUDE_GUARD_EIP130TOKEN_AUNLOCK_H
#define INCLUDE_GUARD_EIP130TOKEN_AUNLOCK_H

#include "basic_defs.h"             // uint32_t, bool, inline, etc.
#include "eip130_token_common.h"    // Eip130Token_Command/Result_t


/*----------------------------------------------------------------------------
 * Eip130Token_Command_AUnlock_Start
 *
 * CommandToken_p
 *      Pointer to the command token buffer.
 *
 * StateAssetID
 *      ID of Asset of Authenticated Unlock State.
 *
 * KeyAssetID
 *      ID of Asset of Authenticated Unlock Key.
 */
static inline void
Eip130Token_Command_AUnlock_Start(
        Eip130Token_Command_t * const CommandToken_p,
        const uint32_t StateAssetID,
        const uint32_t KeyAssetID)
{
    CommandToken_p->W[0] = (EIP130TOKEN_OPCODE_AUTH_UNLOCK << 24) |
                           (EIP130TOKEN_SUBCODE_AUNLOCKSTART << 28);
    CommandToken_p->W[2] = StateAssetID;
    CommandToken_p->W[3] = KeyAssetID;
}


/*----------------------------------------------------------------------------
 * Eip130Token_Result_AUnlock_CopyNonce
 *
 * This function copies the Nonce from the result token to the buffer
 * provided by the caller. The Nonce length is always 16 bytes.
 *
 * ResultToken_p
 *     Pointer to the result token buffer.
 *
 * NonceData_p
 *      Pointer to the buffer to copy the nonce to.
 */
static inline void
Eip130Token_Result_AUnlock_CopyNonce(
        const Eip130Token_Result_t * const ResultToken_p,
        uint8_t * NonceData_p)
{
    Eip130Token_Result_ReadByteArray(ResultToken_p, 2, 16, NonceData_p);
}


/*----------------------------------------------------------------------------
 * Eip130Token_Command_AUnlock_Verify
 *
 * CommandToken_p
 *      Pointer to the command token buffer.
 *
 * StateAssetID
 *      ID of Asset of Authenticated Unlock State.
 *
 * NonceData_p
 *      Pointer to the buffer that holds the nonce.
 *
 * DataAddress
 *      DMA Address of the signature.
 *
 * DataLengthInBytes
 *      Size of the signature.
 */
static inline void
Eip130Token_Command_AUnlock_Verify(
        Eip130Token_Command_t * const CommandToken_p,
        const uint32_t StateAssetID,
        const uint8_t * const NonceData_p,
        const uint64_t DataAddress,
        const uint32_t DataLengthInBytes)
{
    CommandToken_p->W[0] = (EIP130TOKEN_OPCODE_AUTH_UNLOCK << 24) |
                           (EIP130TOKEN_SUBCODE_AUNLOCKVERIFY << 28);
    CommandToken_p->W[2] = StateAssetID;
    CommandToken_p->W[3] = (DataLengthInBytes & MASK_10_BITS);
    CommandToken_p->W[4] = (uint32_t)(DataAddress);
    CommandToken_p->W[5] = (uint32_t)(DataAddress >> 32);

    Eip130Token_Command_WriteByteArray(CommandToken_p, 6, NonceData_p, 16);
}


/*----------------------------------------------------------------------------
 * Eip130Token_Command_SetSecureDebug
 *
 * CommandToken_p
 *      Pointer to the command token buffer.
 *
 * StateAssetID
 *      ID of Asset of Authenticated Unlock State.
 *
 * Set
 *      Indication to set the port bits, if not set the port bits are cleared.
 */
static inline void
Eip130Token_Command_SetSecureDebug(
        Eip130Token_Command_t * const CommandToken_p,
        const uint32_t StateAssetID,
        const bool Set)
{
    CommandToken_p->W[0] = (EIP130TOKEN_OPCODE_AUTH_UNLOCK << 24) |
                           (EIP130TOKEN_SUBCODE_SETSECUREDEBUG << 28);
    CommandToken_p->W[2] = StateAssetID;
    CommandToken_p->W[3] = Set ? BIT_31 : 0;
}


#endif /* Include Guard */

/* end of file eip130_token_aunlock.h */
