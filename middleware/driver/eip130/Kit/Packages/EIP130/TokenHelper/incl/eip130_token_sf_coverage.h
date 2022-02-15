/* eip130_token_sf_coverage.h
 *
 * Security Module Token helper functions
 * - Special Functions tokens related functions and definitions for Coverage
 *   operation
 * This module can convert a set of parameters into a Security Module Command
 * token, or parses a set of parameters from a Security Module Result token.
 */

/*****************************************************************************
* Copyright (c) 2017 INSIDE Secure B.V. All Rights Reserved.
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

#ifndef INCLUDE_GUARD_EIP130TOKEN_SF_COVERAGE_H
#define INCLUDE_GUARD_EIP130TOKEN_SF_COVERAGE_H

#include "c_eip130_token.h"         // configuration options
#include "basic_defs.h"             // uint32_t, bool, inline, etc.
#include "eip130_token_common.h"    // Eip130Token_Command/Result_t


/*----------------------------------------------------------------------------
 * Eip130Token_Command_SF_Coverage
 *
 * This function is used to intialize the token for the Special Functions
 * Code Coverage operation.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * OutputDataAddress
 *     Address where the risc-v should place the coverage data
 *
 * FileName
 *     Name of the object file
 *
 */
static inline void
Eip130Token_Command_SF_Coverage(
        Eip130Token_Command_t * const CommandToken_p,
        const uint64_t OutputDataAddress,
        uint8_t * FileName)
{
    CommandToken_p->W[0] = (EIP130TOKEN_OPCODE_SPECIALFUNCTIONS << 24) |
                           (EIP130TOKEN_SUBCODE_SF_COVERAGE << 28);

    CommandToken_p->W[2] = (uint32_t)(OutputDataAddress);
    CommandToken_p->W[3] = (uint32_t)(OutputDataAddress >> 32);

    Eip130Token_Command_WriteByteArray(CommandToken_p,  4, FileName, 32);
}

/*----------------------------------------------------------------------------
 * Eip130Token_Result_SF_Coverage
 *
 * ResultToken_p
 *     Pointer to the result token buffer.
 *
 * DataSize
 *      Pointer to the buffer to copy the size of returned coverage data.
 */
static inline void
Eip130Token_Result_SF_Coverage(
        const Eip130Token_Result_t * const ResultToken_p,
        uint32_t * DataSize)
{
    *DataSize = ResultToken_p->W[1];
}

#endif /* Include Guard */


/* end of file eip130_token_sf_coverage.h */