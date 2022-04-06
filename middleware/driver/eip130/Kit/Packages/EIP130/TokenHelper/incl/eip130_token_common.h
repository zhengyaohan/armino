/* eip130_token_common.h
 *
 * Security Module Token helper functions
 * - Common token related functions and definitions
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

#ifndef INCLUDE_GUARD_EIP130TOKEN_COMMON_H
#define INCLUDE_GUARD_EIP130TOKEN_COMMON_H

#include "basic_defs.h"             // uint32_t, bool, inline, etc.

// Command Token
#define EIP130TOKEN_COMMAND_WORDS   64
typedef struct
{
    uint32_t W[EIP130TOKEN_COMMAND_WORDS];
} Eip130Token_Command_t;

// Result Token
#define EIP130TOKEN_RESULT_WORDS    64
typedef struct
{
    uint32_t W[EIP130TOKEN_RESULT_WORDS];
} Eip130Token_Result_t;

// Token operation codes
#define EIP130TOKEN_OPCODE_NOP                    0U
#define EIP130TOKEN_OPCODE_ENCRYPTION             1U
#define EIP130TOKEN_OPCODE_HASH                   2U
#define EIP130TOKEN_OPCODE_MAC                    3U
#define EIP130TOKEN_OPCODE_TRNG                   4U
#define EIP130TOKEN_OPCODE_SPECIALFUNCTIONS       5U
#define EIP130TOKEN_OPCODE_AESWRAP                6U
#define EIP130TOKEN_OPCODE_ASSETMANAGEMENT        7U
#define EIP130TOKEN_OPCODE_AUTH_UNLOCK            8U
#define EIP130TOKEN_OPCODE_PUBLIC_KEY             9U
#define EIP130TOKEN_OPCODE_EMMC                   10U
#define EIP130TOKEN_OPCODE_EXT_SERVICE            11U
#define EIP130TOKEN_OPCODE_RESERVED12             12U
#define EIP130TOKEN_OPCODE_RESERVED13             13U
#define EIP130TOKEN_OPCODE_SERVICE                14U
#define EIP130TOKEN_OPCODE_SYSTEM                 15U

// Token sub codes
// TRNG operations
#define EIP130TOKEN_SUBCODE_RANDOMNUMBER          0U
#define EIP130TOKEN_SUBCODE_TRNGCONFIG            1U
#define EIP130TOKEN_SUBCODE_VERIFYDRBG            2U
#define EIP130TOKEN_SUBCODE_VERIFYNRBG            3U
// Asset Management operations
#define EIP130TOKEN_SUBCODE_ASSETSEARCH           0U
#define EIP130TOKEN_SUBCODE_ASSETCREATE           1U
#define EIP130TOKEN_SUBCODE_ASSETLOAD             2U
#define EIP130TOKEN_SUBCODE_ASSETDELETE           3U
#define EIP130TOKEN_SUBCODE_PUBLICDATA            4U
#define EIP130TOKEN_SUBCODE_MONOTONICREAD         5U
#define EIP130TOKEN_SUBCODE_MONOTONICINCR         6U
#define EIP130TOKEN_SUBCODE_OTPDATAWRITE          7U
#define EIP130TOKEN_SUBCODE_SECURETIMER           8U
#define EIP130TOKEN_SUBCODE_PROVISIONRANDOMHUK    9U
// KeyWrap and Encrypt vector operations
#define EIP130TOKEN_SUBCODE_AESKEYWRAP            0U
#define EIP130TOKEN_SUBCODE_ENCRYPTVECTOR         1U
// Special Functions operations
#define EIP130TOKEN_SUBCODE_SF_MILENAGE           0U
#define EIP130TOKEN_SUBCODE_SF_BLUETOOTH          1U
#define EIP130TOKEN_SUBCODE_SF_COVERAGE           4U
// Authenticated Unlock operations
#define EIP130TOKEN_SUBCODE_AUNLOCKSTART          0U
#define EIP130TOKEN_SUBCODE_AUNLOCKVERIFY         1U
#define EIP130TOKEN_SUBCODE_SETSECUREDEBUG        2U
// Public key operations
#define EIP130TOKEN_SUBCODE_PK_NOASSETS           0U
#define EIP130TOKEN_SUBCODE_PK_WITHASSETS         1U
// eMMC operations
#define EIP130TOKEN_SUBCODE_EMMC_RDREQ            0U
#define EIP130TOKEN_SUBCODE_EMMC_RDVER            1U
#define EIP130TOKEN_SUBCODE_EMMC_RDWRCNTREQ       2U
#define EIP130TOKEN_SUBCODE_EMMC_RDWRCNTVER       3U
#define EIP130TOKEN_SUBCODE_EMMC_WRREQ            4U
#define EIP130TOKEN_SUBCODE_EMMC_WRVER            5U
// Service operations
#define EIP130TOKEN_SUBCODE_REGISTERREAD          0U
#define EIP130TOKEN_SUBCODE_REGISTERWRITE         1U
#define EIP130TOKEN_SUBCODE_CLOCKSWITCH           2U
#define EIP130TOKEN_SUBCODE_ZEROOUTMAILBOX        3U
#define EIP130TOKEN_SUBCODE_SELECTOTPZERO         4U
#define EIP130TOKEN_SUBCODE_ZEROIZEOTP            5U
// System operations
#define EIP130TOKEN_SUBCODE_SYSTEMINFO            0U
#define EIP130TOKEN_SUBCODE_SELFTEST              1U
#define EIP130TOKEN_SUBCODE_RESET                 2U
#define EIP130TOKEN_SUBCODE_DEFINEUSERS           3U
#define EIP130TOKEN_SUBCODE_SLEEP                 4U
#define EIP130TOKEN_SUBCODE_RESUMEFROMSLEEP       5U
#define EIP130TOKEN_SUBCODE_HIBERNATION           6U
#define EIP130TOKEN_SUBCODE_RESUMEFROMHIBERNATION 7U
#define EIP130TOKEN_SUBCODE_SETTIME               8U

// Token/HW/Algorithm related limits
#define EIP130TOKEN_DMA_MAXLENGTH           0x001FFFFF  // 2 MB - 1 bytes
#define EIP130TOKEN_DMA_TOKENID_SIZE        4           // bytes
#define EIP130TOKEN_DMA_ARC4_STATE_BUF_SIZE 256         // bytes

// DMA data block must be an integer multiple of a work block size (in bytes)
#define EIP130TOKEN_DMA_ALGO_BLOCKSIZE_HASH 64
#define EIP130TOKEN_DMA_ALGO_BLOCKSIZE_AES  16
#define EIP130TOKEN_DMA_ALGO_BLOCKSIZE_DES  8
#define EIP130TOKEN_DMA_ALGO_BLOCKSIZE_ARC4 4
#define EIP130TOKEN_DMA_ALGO_BLOCKSIZE_NOP  4


/*----------------------------------------------------------------------------
 * Eip130Token_Command_Clear
 *
 * This function initializes a Eip130Token_Command_t array with a known
 * pattern. This helps debugging.
 *
 * CommandToken_p
 *      Pointer to the command token buffer.
 */
static inline void
Eip130Token_Command_Clear(
        Eip130Token_Command_t * const CommandToken_p)
{
    unsigned int i;

    for (i = 0; i < EIP130TOKEN_COMMAND_WORDS; i++)
    {
        CommandToken_p->W[i] = 0xAAAAAAAA;
    }
}


/*----------------------------------------------------------------------------
 * Eip130Token_Command_Identity
 *
 * This function sets the token identity.
 *
 * CommandToken_p
 *      Pointer to the command token buffer.
 *
 * Identity
 *      A 32-bit value to identify the user.
 */
static inline void
Eip130Token_Command_Identity(
        Eip130Token_Command_t * const CommandToken_p,
        uint32_t Identity)
{
    CommandToken_p->W[1] = Identity;
}


/*----------------------------------------------------------------------------
 * Eip130Token_Command_SetTokenID
 *
 * This function sets the TokenID related field in the Command Token. The SM
 * will copy this value to the Result Token.
 * This can also be used for data stream synchronization: the TokenID is
 * appended to the end of the DMA stream. This word must be initialized to a
 * different value and then polled until the expected TokenID value/pattern
 * shows up.
 *
 * CommandToken_p
 *      Pointer to the command token buffer.
 *
 * TokenIDValue
 *      The 16 bit TokenID value that the SM will write to the Result Token
 *      and optionally appends to the end of the output DMA data stream.
 *
 * fWriteTokenID
 *      false  do not append TokenID to DMA data stream.
 *      true   write TokenID at end of DMA data stream. The stream will first
 *             be padded with zeros until a complete 32bit word before an extra
 *             32bit word is output with the TokenID in it.
 */
static inline void
Eip130Token_Command_SetTokenID(
        Eip130Token_Command_t * const CommandToken_p,
        const uint16_t TokenIDValue,
        const bool fWriteTokenID)
{
    // Replace TokenID field (word 0, lowest 16 bits) with TokenIDValue aand
    // reset Write Token ID indication
    CommandToken_p->W[0] &= ((MASK_16_BITS << 16) - BIT_18);
    CommandToken_p->W[0] |= TokenIDValue;
    if (fWriteTokenID)
    {
        // Set Write Token ID field (word 0, bit 18)
        CommandToken_p->W[0] |= BIT_18;
    }
}


/*----------------------------------------------------------------------------
 * Eip130Token_Result_Code
 *
 * This function extracts the result information from the result token.
 * This function can be used on any result token.
 *
 * ResultToken_p
 *     Pointer to the result token buffer.
 *
 * Return Value
 *     0    No error or warning
 *     <0   Error code
 *     >0   warning code
 */
static inline int
Eip130Token_Result_Code(
        Eip130Token_Result_t * const ResultToken_p)
{
    int rv = (int)(ResultToken_p->W[0] >> 24);
    if ((rv & (int)BIT_7) != 0)
    {
        return -(rv & (int)MASK_7_BITS);
    }
    return rv;
}


/*----------------------------------------------------------------------------
 * Eip130Token_Command_WriteByteArray
 *
 * This function fills a consecutive number of words in the command token with
 * bytes from an array. Four consecutive bytes form a 32bit word, LSB-first.
 *
 * CommandToken_p
 *      Pointer to the command token buffer.
 *
 * StartWord
 *      Start index in command token.
 *
 * Data_p
 *      Pointer to the data to write to the command token.
 *
 * DataLenInBytes
 *      Number of bytes to write.
 */
void
Eip130Token_Command_WriteByteArray(
        Eip130Token_Command_t * const CommandToken_p,
        unsigned int StartWord,
        const uint8_t * const Data_p,
        const unsigned int DataLenInBytes);


/*----------------------------------------------------------------------------
 * Eip130Token_Result_ReadByteArray
 *
 * This function reads a number of consecutive words from the result token
 * and writes these to a byte array, breaking down each word into bytes, LSB
 * first.
 *
 * ResultToken_p
 *     Pointer to the result token buffer.
 *
 * StartWord
 *      Start index in result token.
 *
 * DestLenInBytes
 *      Number of bytes to write.
 *
 * Dest_p
 *      Pointer to the data buffer to copy the data from the result token into.
 */
void
Eip130Token_Result_ReadByteArray(
        const Eip130Token_Result_t * const ResultToken_p,
        unsigned int StartWord,
        unsigned int DestLenOutBytes,
        uint8_t * Dest_p);


#endif /* Include Guard */

/* end of file eip130_token_common.h */
