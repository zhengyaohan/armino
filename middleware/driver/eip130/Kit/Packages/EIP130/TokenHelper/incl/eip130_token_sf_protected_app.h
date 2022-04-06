/* eip130_token_sf_protected_app.h
 *
 * Security Module Token helper functions
 * - Special Functions tokens related functions and definitions for RISC-V protected app
 *
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

#ifndef INCLUDE_GUARD_EIP130TOKEN_SF_PROTECTED_APP_H
#define INCLUDE_GUARD_EIP130TOKEN_SF_PROTECTED_APP_H

#include "c_eip130_token.h"         // configuration options
#include "basic_defs.h"             // uint32_t, bool, inline, etc.
#include "eip130_token_common.h"    // Eip130Token_Command/Result_t


/*----------------------------------------------------------------------------
 * Eip130Token_Command_SF_Protected_App
 *
 * This function is used to intialize the token for the PRoT custom
 * protected application
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * ProtectedAppId
 *     Id of the protected application used at the PRoT.
 *
 * InputDataSize
 *     Number of bytes need to be sent from DataIn buffer to the PRoT.
 *
 * DataIn_p
 *     Pointer to the buffer with data required in PRoT's app
 */
static inline void
Eip130Token_Command_SF_Protected_App(
        Eip130Token_Command_t * const CommandToken_p,
        const uint32_t ProtectedAppId,
        const uint32_t InputDataSize,
        const uint8_t * const DataIn_p)
{
    CommandToken_p->W[0] = (EIP130TOKEN_OPCODE_SPECIALFUNCTIONS << 24) |
                           (ProtectedAppId << 28);

    CommandToken_p->W[2] = InputDataSize;
    Eip130Token_Command_WriteByteArray(CommandToken_p, 3, DataIn_p, InputDataSize);
}


/*----------------------------------------------------------------------------
 * Eip130Token_Result_SF_Protected_App
 *
 * ResultToken_p
 *     Pointer to the result token buffer.
 *
 * OutputDataSize_p
 *     Pointer to the Output Data Size for result token.
 *
 * DataOut_p
 *      Pointer to the buffer to copy the data from PRoT.
 */
static inline void
Eip130Token_Result_SF_Protected_App(
        const Eip130Token_Result_t * const ResultToken_p,
        uint32_t * OutputDataSize_p,
        uint8_t * DataOut_p)
{
    *OutputDataSize_p = ResultToken_p->W[1];
    Eip130Token_Result_ReadByteArray(ResultToken_p, 2, *OutputDataSize_p, DataOut_p);
}


#endif /* Include Guard */


/* end of file eip130_token_sf_protected_app.h */
