/* eip130_token_timer.h
 *
 * Security Module Token helper functions
 * - Secure Timer related functions and definitions
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

#ifndef INCLUDE_GUARD_EIP130TOKEN_TIMER_H
#define INCLUDE_GUARD_EIP130TOKEN_TIMER_H

#include "basic_defs.h"             // uint32_t, bool, inline, etc.

#include "eip130_token_common.h"    // Eip130Token_Command/Result_t


/*----------------------------------------------------------------------------
 * Eip130Token_Command_SecureTimer
 *
 * This function initializes the command token for a Secure Timer (Re)Start,
 * Stop and read operation.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * AssetId (optional for start)
 *      AssetId of the secure timer to stop, read or restart.
 *
 * fSecond
 *      Second timer indication otherwise 100 us timer is used.
 *
 * Operation
 *      Secure timer operation to perform (re)start, stop or read.
 */
static inline void
Eip130Token_Command_SecureTimer(
        Eip130Token_Command_t * const CommandToken_p,
        const uint32_t AssetId,
        const bool fSecond,
        const uint16_t Operation)
{
    CommandToken_p->W[0] = (EIP130TOKEN_OPCODE_ASSETMANAGEMENT << 24) |
                           (EIP130TOKEN_SUBCODE_SECURETIMER << 28);
    CommandToken_p->W[2] = AssetId;
    CommandToken_p->W[3] = Operation & MASK_2_BITS;
    if (fSecond)
    {
        CommandToken_p->W[3] |= BIT_15;
    }
}


/*----------------------------------------------------------------------------
 * Eip130Token_Result_SecureTimer
 *
 * ResultToken_p
 *     Pointer to the result token buffer.
 *
 * AssetId_p (optional)
 *      Pointer to the variable in which the AssetId must be returned.
 *
 * ElapsedTime_p (optional)
 *      Pointer to the variable in which the elapsed time must be returned.
 */
static inline void
Eip130Token_Result_SecureTimer(
        const Eip130Token_Result_t * const ResultToken_p,
        uint32_t * const AssetId_p,
        uint32_t * const ElapsedTime_p)
{
    if (AssetId_p != NULL)
    {
        *AssetId_p = ResultToken_p->W[1];
    }
    if (ElapsedTime_p != NULL)
    {
        *ElapsedTime_p = ResultToken_p->W[2];
    }
}


#endif /* Include Guard */

/* end of file eip130_token_timer.h */
