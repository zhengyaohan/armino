/* eip130_token_emmc.h
 *
 * Security Module Token helper functions
 * - eMMC token related functions and definitions
 *
 * This module can convert a set of parameters into a Security Module Command
 * token, or parses a set of parameters from a Security Module Result token.
 */

/*****************************************************************************
* Copyright (c) 2016-2018 INSIDE Secure B.V. All Rights Reserved.
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

#ifndef INCLUDE_GUARD_EIP130TOKEN_EMMC_H
#define INCLUDE_GUARD_EIP130TOKEN_EMMC_H

#include "c_eip130_token.h"         // configuration options
#include "basic_defs.h"             // uint32_t, bool, inline, etc.
#include "eip130_token_common.h"    // Eip130Token_Command/Result_t
#include "eip130_token_mac.h"       // EIP130TOKEN_MAC_ALGORITHM_HMAC_SHA256


/*----------------------------------------------------------------------------
 * Eip130Token_Command_eMMC_ReadRequest
 *
 * This function initializes the eMMC Read Request token.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * AuthKeyAssetId
 *     Asset ID of the Authentication Key (the key that will be used for
 *     signature verification).
 */
static inline void
Eip130Token_Command_eMMC_ReadRequest(
        Eip130Token_Command_t * const CommandToken_p,
        const uint32_t AuthKeyAssetId)
{
    CommandToken_p->W[0] = (EIP130TOKEN_OPCODE_EMMC << 24) |
                           (EIP130TOKEN_SUBCODE_EMMC_RDREQ << 28);
    CommandToken_p->W[2] = AuthKeyAssetId;
}

/*----------------------------------------------------------------------------
 * Eip130Token_Command_eMMC_ReadWriteCounterRequest
 *
 * This function initializes the eMMC Read Write-Counter Request token.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * StateAssetId
 *     Asset ID of the eMMC State.
 */
static inline void
Eip130Token_Command_eMMC_ReadWriteCounterRequest(
        Eip130Token_Command_t * const CommandToken_p,
        const uint32_t StateAssetId)
{
    CommandToken_p->W[0] = (EIP130TOKEN_OPCODE_EMMC << 24) |
                           (EIP130TOKEN_SUBCODE_EMMC_RDWRCNTREQ << 28);
    CommandToken_p->W[2] = StateAssetId;
}

/*----------------------------------------------------------------------------
 * Eip130Token_Result_eMMC_ReadStateID
 *
 * This function reads the eMMC State Asset ID from the result token.
 *
 * ResultToken_p
 *     Pointer to the result token buffer.
 *
 * StateAssetId_p
 *      Pointer to the variable in which the eMMC State AssetId must be
 *      returned.
 */
static inline void
Eip130Token_Result_eMMC_ReadStateID(
        Eip130Token_Result_t * const ResultToken_p,
        uint32_t * const StateAssetId_p)
{
    *StateAssetId_p = ResultToken_p->W[1];
}

/*----------------------------------------------------------------------------
 * Eip130Token_Result_eMMC_ReadCopyNonce
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
Eip130Token_Result_eMMC_ReadCopyNonce(
        const Eip130Token_Result_t * const ResultToken_p,
        uint8_t * NonceData_p)
{
    Eip130Token_Result_ReadByteArray(ResultToken_p, 2, 16, NonceData_p);
}


/*----------------------------------------------------------------------------
 * Eip130Token_Command_eMMC_ReadVerify
 *
 * This function initializes the eMMC Read Verify token.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 */
static inline void
Eip130Token_Command_eMMC_ReadVerify(
        Eip130Token_Command_t * const CommandToken_p)
{
    CommandToken_p->W[0] = (EIP130TOKEN_OPCODE_EMMC << 24) |
                           (EIP130TOKEN_SUBCODE_EMMC_RDVER << 28);
}

/*----------------------------------------------------------------------------
 * Eip130Token_Command_eMMC_ReadWriteCounterVerify
 *
 * This function initializes the eMMC Read Write-Counter Verify token.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 */
static inline void
Eip130Token_Command_eMMC_ReadWriteCounterVerify(
        Eip130Token_Command_t * const CommandToken_p)
{
    CommandToken_p->W[0] = (EIP130TOKEN_OPCODE_EMMC << 24) |
                           (EIP130TOKEN_SUBCODE_EMMC_RDWRCNTVER << 28);
}

/*----------------------------------------------------------------------------
 * Eip130Token_Command_eMMC_WriteRequest
 *
 * This function initializes the eMMC Write Request token.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 */
static inline void
Eip130Token_Command_eMMC_WriteRequest(
        Eip130Token_Command_t * const CommandToken_p)
{
    CommandToken_p->W[0] = (EIP130TOKEN_OPCODE_EMMC << 24) |
                           (EIP130TOKEN_SUBCODE_EMMC_WRREQ << 28);
}

/*----------------------------------------------------------------------------
 * Eip130Token_Command_eMMC_WriteVerify
 *
 * This function initializes the eMMC Write Verify token.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * InputDataAddress
 *     Address of data to be MACed.
 *
 * InputDataLengthInBytes
 *     Size/Length of the data block to be MACed.
 */
static inline void
Eip130Token_Command_eMMC_WriteVerify(
        Eip130Token_Command_t * const CommandToken_p)
{
    CommandToken_p->W[0] = (EIP130TOKEN_OPCODE_EMMC << 24) |
                           (EIP130TOKEN_SUBCODE_EMMC_WRVER << 28);
}

/*----------------------------------------------------------------------------
 * Eip130Token_Command_eMMC_SetInputDataAndMACInfo
 *
 * This function sets the input data and MAC related information.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * InputDataAddress
 *     Address of data to be MACed.
 *
 * InputDataLengthInBytes
 *     Size/Length of the data block to be MACed.
 */
static inline void
Eip130Token_Command_eMMC_SetInputDataAndMACInfo(
        Eip130Token_Command_t * const CommandToken_p,
        const uint64_t InputDataAddress,
        const uint32_t InputDataLengthInBytes)
{
    CommandToken_p->W[2] = InputDataLengthInBytes;
    CommandToken_p->W[3] = (uint32_t)(InputDataAddress);
    CommandToken_p->W[4] = (uint32_t)(InputDataAddress >> 32);
    CommandToken_p->W[5] = InputDataLengthInBytes;
    // Notes:
    // - Algorithm is always HMAC-SHA-256
    // - Mode is always 00
    // - KeyLength is always 32 bytes
    // - TotalMessageLengthInBytes is always the InputDataLengthInBytes
    CommandToken_p->W[6] = (32 << 16) | EIP130TOKEN_MAC_ALGORITHM_HMAC_SHA256;
    CommandToken_p->W[24] = (uint32_t)(InputDataLengthInBytes);
    CommandToken_p->W[25] = 0;
}

/*----------------------------------------------------------------------------
 * Eip130Token_Command_eMMC_SetStateAssetID
 *
 * This function sets the eMMC State Asset ID.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * AssetId
 *     Asset ID of the eMMC State asset.
 */
static inline void
Eip130Token_Command_eMMC_SetStateAssetID(
        Eip130Token_Command_t * const CommandToken_p,
        const uint32_t AssetId)
{
    CommandToken_p->W[7] = AssetId;
}

/*----------------------------------------------------------------------------
 * Eip130Token_Command_eMMC_CopyMAC
 *
 * This function copies the MAC from the buffer provided by the caller into
 * the command token. The requested number of bytes are copied (length depends
 * on the algorithm that will be used).
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * MAC_p
 *     Pointer to the MAC buffer.
 *
 * MACLenInBytes
 *     The size of the MAC to copy.
 */
static inline void
Eip130Token_Command_eMMC_CopyMAC(
        Eip130Token_Command_t * const CommandToken_p,
        const uint8_t * const MAC_p,
        const uint32_t MACLenInBytes)
{
    Eip130Token_Command_WriteByteArray(CommandToken_p, 8, MAC_p, MACLenInBytes);
}

/*----------------------------------------------------------------------------
 * Eip130Token_Result_eMMC_CopyMAC
 *
 * This function copies the MAC from the result token to the buffer provided
 * by the caller. The requested number of bytes are copied (length depends on
 * the algorithm that was used).
 *
 * The MAC is written to the destination buffer, Byte 0 first.
 *
 * ResultToken_p
 *     Pointer to the result token buffer.
 *
 * MACLenInBytes
 *     The size of the MAC to copy.
 *
 * MAC_p
 *     Pointer to the MAC buffer.
 */
static inline void
Eip130Token_Result_eMMC_CopyMAC(
        Eip130Token_Result_t * const ResultToken_p,
        const uint32_t MACLenInBytes,
        uint8_t * MAC_p)
{
    Eip130Token_Result_ReadByteArray(ResultToken_p, 2, MACLenInBytes, MAC_p);
}


#endif /* Include Guard */

/* end of file eip130_token_emmc.h */
