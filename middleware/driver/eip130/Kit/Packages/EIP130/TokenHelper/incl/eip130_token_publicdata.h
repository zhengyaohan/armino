/* eip130_token_publicdata.h
 *
 * Security Module Token helper functions
 * - Public Data token related functions and definitions
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

#ifndef INCLUDE_GUARD_EIP130TOKEN_PUBLICDATA_H
#define INCLUDE_GUARD_EIP130TOKEN_PUBLICDATA_H

#include "basic_defs.h"             // uint32_t, bool, inline, etc.
#include "eip130_token_common.h"    // Eip130Token_Command/Result_t


/*----------------------------------------------------------------------------
 * Eip130Token_Command_Publicdata_Read
 *
 * CommandToken_p
 *      Pointer to the command token buffer.
 *
 * AssetID
 *      ID of Asset of the public data object to read
 *
 * DataAddress
 *      Output data address
 *
 * DataLengthInBytes
 *      Output data length
 *      Must be a multiple of 4.
 */
static inline void
Eip130Token_Command_PublicData_Read(
        Eip130Token_Command_t * const CommandToken_p,
        const uint32_t AssetID,
        const uint64_t DataAddress,
        const uint32_t DataLengthInBytes)
{
    CommandToken_p->W[0] = (EIP130TOKEN_OPCODE_ASSETMANAGEMENT << 24) |
                           (EIP130TOKEN_SUBCODE_PUBLICDATA << 28);
    CommandToken_p->W[2] = AssetID;
    CommandToken_p->W[3] = DataLengthInBytes;
    CommandToken_p->W[4] = (uint32_t)(DataAddress);
    CommandToken_p->W[5] = (uint32_t)(DataAddress >> 32);
}


/*----------------------------------------------------------------------------
 * Eip130Token_Result_Publicdata_Read
 *
 * ResultToken_p
 *     Pointer to the result token buffer.
 *
 * DataLengthInBytes_p
 *      Pointer to the variable in which the data length must be returned.
 */
static inline void
Eip130Token_Result_Publicdata_Read(
        const Eip130Token_Result_t * const ResultToken_p,
        uint32_t * const DataLengthInBytes_p)
{
    *DataLengthInBytes_p = ResultToken_p->W[1] & MASK_10_BITS;
}


#endif /* Include Guard */

/* end of file eip130_token_publicdata.h */
