/* eip130_token_service.h
 *
 * Security Module Token helper functions
 * - Service tokens related functions and definitions
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

#ifndef INCLUDE_GUARD_EIP130TOKEN_SERVICE_H
#define INCLUDE_GUARD_EIP130TOKEN_SERVICE_H

#include "basic_defs.h"             // uint32_t, bool, inline, etc.
#include "eip130_token_common.h"    // Eip130Token_Command/Result_t


/*----------------------------------------------------------------------------
 * Eip130Token_Command_RegisterWrite
 *
 * This token can be used to write the DMA controller configuration register.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * Address
 *     Address to write.
 *
 * Value
 *     Value to write.
 */
static inline void
Eip130Token_Command_RegisterWrite(
        Eip130Token_Command_t * const CommandToken_p,
        const uint16_t Address,
        const uint32_t Value)
{
    CommandToken_p->W[0] = (EIP130TOKEN_OPCODE_SERVICE << 24) |
                           (EIP130TOKEN_SUBCODE_REGISTERWRITE << 28);
    CommandToken_p->W[2] = 1;           // only one word
    CommandToken_p->W[3] = 0;           // Mask = 0 = Write all 32 bits
    CommandToken_p->W[4] = Address;     // Address to write to
    CommandToken_p->W[5] = Value;       // Value to write
}


/*----------------------------------------------------------------------------
 * Eip130Token_Command_RegisterRead
 *
 * This token can be used to read the DMA controller configuration register.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * Address
 *     Address to read.
 */
static inline void
Eip130Token_Command_RegisterRead(
        Eip130Token_Command_t * const CommandToken_p,
        const uint16_t Address)
{
    CommandToken_p->W[0] = (EIP130TOKEN_OPCODE_SERVICE << 24) |
                           (EIP130TOKEN_SUBCODE_REGISTERREAD << 28);
    CommandToken_p->W[2] = 1;           // only one word
    CommandToken_p->W[3] = Address;     // Address to read from
}


/*----------------------------------------------------------------------------
 * Eip130Token_Result_RegisterRead
 *
 * ResultToken_p
 *     Pointer to the result token buffer.
 *
 * Value_p
 *      Pointer to the variable in which the value must be returned.
 */
static inline void
Eip130Token_Result_RegisterRead(
        const Eip130Token_Result_t * const ResultToken_p,
        uint32_t * const Value_p)
{
    *Value_p = ResultToken_p->W[1];
}


/*----------------------------------------------------------------------------
 * Eip130Token_Command_ZeroizeOutputMailbox
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 */
static inline void
Eip130Token_Command_ZeroizeOutputMailbox(
        Eip130Token_Command_t * const CommandToken_p)
{
    //Eip130Token_Command_Clear(CommandToken_p);
    CommandToken_p->W[0] = (EIP130TOKEN_OPCODE_SERVICE << 24) |
                           (EIP130TOKEN_SUBCODE_ZEROOUTMAILBOX << 28);
}


/*----------------------------------------------------------------------------
 * Eip130Token_Command_ClockSwitch
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * ClocksForcedOn
 *     Bitmask specifying (with 1b’s) the clocks that needs to be forced ON
 *     (always ON).
 *
 * ClocksForcedOff
 *     Bitmask specifying (with 1b’s) the clocks that needs to be forced OFF
 *     (always OFF).
 */
static inline void
Eip130Token_Command_ClockSwitch(
        Eip130Token_Command_t * const CommandToken_p,
        const uint16_t ClocksForcedOn,
        const uint16_t ClocksForcedOff)
{
    //Eip130Token_Command_Clear(CommandToken_p);
    CommandToken_p->W[0] = (EIP130TOKEN_OPCODE_SERVICE << 24) |
                           (EIP130TOKEN_SUBCODE_CLOCKSWITCH << 28);
    CommandToken_p->W[2] = (((uint32_t)ClocksForcedOff) << 16) |
                           (uint32_t)ClocksForcedOn;
}


#endif /* Include Guard */

/* end of file eip130_token_service.h */
