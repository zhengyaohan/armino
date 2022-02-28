/* eip130_token_sf_bluetooth.h
 *
 * Security Module Token helper functions
 * - Special Functions tokens related functions and definitions for Bluetooth
 *
 * This module can convert a set of parameters into a Security Module Command
 * token, or parses a set of parameters from a Security Module Result token.
 */

/*****************************************************************************
* Copyright (c) 2017-2018 INSIDE Secure B.V. All Rights Reserved.
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

#ifndef INCLUDE_GUARD_EIP130TOKEN_SF_BLUETOOTH_H
#define INCLUDE_GUARD_EIP130TOKEN_SF_BLUETOOTH_H

#include "c_eip130_token.h"         // configuration options
#include "basic_defs.h"             // uint32_t, bool, inline, etc.
#include "eip130_token_common.h"    // Eip130Token_Command/Result_t


/*----------------------------------------------------------------------------
 * Eip130Token_Command_SF_BluetoothLE_f5
 *
 * This function is used to intialize the token for the Special Functions
 * Bluetooth LE Secure Connections Key Generation Function f5.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * SharedSecretAssetId
 *     Shared Secret Asset Identifier.
 *
 * LongTermKeyAssetId
 *     Long term key (LTK) Asset Identifier.
 *
 * N1_p
 *     Pointer to N1 (128-bit value).
 *
 * N2_p
 *     Pointer to N2 (128-bit value).
 *
 * A1_p
 *     Pointer to A1 (56-bit value).
 *
 * A2_p
 *     Pointer to A2 (56-bit value).
 */
static inline void
Eip130Token_Command_SF_BluetoothLE_f5(
        Eip130Token_Command_t * const CommandToken_p,
        const uint32_t SharedSecretAssetId,
        const uint32_t LongTermKeyAssetId,
        const uint8_t * const N1_p,
        const uint8_t * const N2_p,
        const uint8_t * const A1_p,
        const uint8_t * const A2_p)
{
    CommandToken_p->W[0] = (EIP130TOKEN_OPCODE_SPECIALFUNCTIONS << 24) |
                           (EIP130TOKEN_SUBCODE_SF_BLUETOOTH << 28);
    CommandToken_p->W[2] = SharedSecretAssetId;
    CommandToken_p->W[3] = LongTermKeyAssetId;

    Eip130Token_Command_WriteByteArray(CommandToken_p,  4, N1_p, 16);
    Eip130Token_Command_WriteByteArray(CommandToken_p,  8, N2_p, 16);
    Eip130Token_Command_WriteByteArray(CommandToken_p, 12, A1_p, 7);
    Eip130Token_Command_WriteByteArray(CommandToken_p, 14, A2_p, 7);
}


/*----------------------------------------------------------------------------
 * Eip130Token_Result_SF_BluetoothLE_f5
 *
 * ResultToken_p
 *     Pointer to the result token buffer.
 *
 * MacKey_p
 *      Pointer to the buffer to copy the MAC key to.
 */
static inline void
Eip130Token_Result_SF_BluetoothLE_f5(
        const Eip130Token_Result_t * const ResultToken_p,
        uint8_t * MacKey_p)
{
    Eip130Token_Result_ReadByteArray(ResultToken_p, 1, 16, MacKey_p);
}


#endif /* Include Guard */


/* end of file eip130_token_sf_bluetooth.h */
