/* eip130_token_nop.h
 *
 * Security Module Token helper functions
 * - NOP token related functions and definitions
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

#ifndef INCLUDE_GUARD_EIP130TOKEN_NOP_H
#define INCLUDE_GUARD_EIP130TOKEN_NOP_H

#include "basic_defs.h"             // uint32_t, bool, inline, etc.
#include "eip130_token_common.h"    // Eip130Token_Command_t


/*----------------------------------------------------------------------------
 * Eip130Token_Command_Nop
 *
 * CommandToken_p
 *      Pointer to the command token buffer.
 *
 * InputDataAddress
 *     Address of the input data buffer.
 *
 * InputDataLengthInBytes
 *     Size of the input data buffer being the number of bytes to copy.
 *     Must be a multiple of 4.
 *
 * OutputDataAddress
 *     Address of the output data buffer.
 *
 * OutputDataLengthInBytes
 *     Size of the output data buffer.
 *     Must be a multiple of 4.
 */
static inline void
Eip130Token_Command_Nop(
        Eip130Token_Command_t * const CommandToken_p,
        const uint64_t InputDataAddress,
        const uint32_t InputDataLengthInBytes,
        const uint64_t OutputDataAddress,
        const uint32_t OutputDataLengthInBytes)
{
    CommandToken_p->W[0] = (EIP130TOKEN_OPCODE_NOP << 24);
    CommandToken_p->W[2] = InputDataLengthInBytes;
    CommandToken_p->W[3] = (uint32_t)(InputDataAddress);
    CommandToken_p->W[4] = (uint32_t)(InputDataAddress >> 32);
    CommandToken_p->W[5] = InputDataLengthInBytes;
    CommandToken_p->W[6] = (uint32_t)(OutputDataAddress);
    CommandToken_p->W[7] = (uint32_t)(OutputDataAddress >> 32);
    CommandToken_p->W[8] = OutputDataLengthInBytes;
}


#endif /* Include Guard */

/* end of file eip130_token_nop.h */
