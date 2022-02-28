/* eip130_token_common.c
 *
 * Security Module Token helper functions
 * - Common byte array related functions
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

#include "c_eip130_token.h"         // configuration options

#include "basic_defs.h"             // uint8_t, IDENTIFIER_NOT_USED, etc.

#include "eip130_token_common.h"    // the API to implement


/*----------------------------------------------------------------------------
 * Eip130Token_Command_WriteByteArray
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * StartWord
 *     The word index of read.
 *
 * Data_p
 *     Pointer to the buffer.
 *
 * DataLenInBytes
 *     The size of the buffer to copy to the token.
 */
void
Eip130Token_Command_WriteByteArray(
        Eip130Token_Command_t * const CommandToken_p,
        unsigned int StartWord,
        const uint8_t * Data_p,
        const unsigned int DataLenInBytes)
{
    const uint8_t * const Stop_p = Data_p + DataLenInBytes;

    if (CommandToken_p == NULL || Data_p == NULL)
    {
        return;
    }

    while (Data_p < Stop_p)
    {
        uint32_t W;

        if (StartWord >= EIP130TOKEN_RESULT_WORDS)
        {
            return;
        }

        // LSB-first
        W = (uint32_t)(*Data_p++);
        if (Data_p < Stop_p)
        {
            W |= (uint32_t)((*Data_p++) << 8);
            if (Data_p < Stop_p)
            {
                W |= (uint32_t)((*Data_p++) << 16);
                if (Data_p < Stop_p)
                {
                    W |= (uint32_t)((*Data_p++) << 24);
                }
            }
        }

        // Write word
        CommandToken_p->W[StartWord++] = W;
    }
}


/*----------------------------------------------------------------------------
 * Eip130Token_Result_ReadByteArray
 *
 * ResultToken_p
 *     Pointer to the result token buffer.
 *
 * StartWord
 *     The word index of read.
 *
 * DestLenInBytes
 *     The size of the buffer to copy the read info to.
 *
 * Dest_p
 *     Pointer to the buffer.
 */
void
Eip130Token_Result_ReadByteArray(
        const Eip130Token_Result_t * const ResultToken_p,
        unsigned int StartWord,
        unsigned int DestLenOutBytes,
        uint8_t * Dest_p)
{
    uint8_t * const Stop_p = Dest_p + DestLenOutBytes;

    if (ResultToken_p == NULL || Dest_p == NULL)
    {
        return;
    }

    while (Dest_p < Stop_p)
    {
        uint32_t W;

        if (StartWord >= EIP130TOKEN_RESULT_WORDS)
        {
            return;
        }

        // Read word
        W = ResultToken_p->W[StartWord++];

        // LSB-first
        *Dest_p++ = (uint8_t)W;
        if (Dest_p < Stop_p)
        {
            W >>= 8;
            *Dest_p++ = (uint8_t)W;
            if (Dest_p < Stop_p)
            {
                W >>= 8;
                *Dest_p++ = (uint8_t)W;
                if (Dest_p < Stop_p)
                {
                    W >>= 8;
                    *Dest_p++ = (uint8_t)W;
                }
            }
        }
    }
}

/* end of file eip130_token_common.c */
