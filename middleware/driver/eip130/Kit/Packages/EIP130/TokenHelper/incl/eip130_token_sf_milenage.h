/* eip130_token_sf_milenage.h
 *
 * Security Module Token helper functions
 * - Special Functions tokens related functions and definitions for Milenage
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

#ifndef INCLUDE_GUARD_EIP130TOKEN_SF_MILENAGE_H
#define INCLUDE_GUARD_EIP130TOKEN_SF_MILENAGE_H

#include "c_eip130_token.h"         // configuration options
#include "basic_defs.h"             // uint32_t, bool, inline, etc.
#include "eip130_token_common.h"    // Eip130Token_Command/Result_t


/*----------------------------------------------------------------------------
 * Eip130Token_Command_SF_MilenageSqnAdminCreate
 *
 * This function is used to intialize the token for the Special Functions
 * Milenage SQN administration creation.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * AssetNumber
 *      Asset number of the Static Asset with the K and OPc.
 *
 * fAMF_SBtest
 *     Indication to enable the AMF Separation Bit test.
 */
static inline void
Eip130Token_Command_SF_MilenageSqnAdminCreate(
        Eip130Token_Command_t * const CommandToken_p,
        const uint16_t AssetNumber,
        const bool fAMF_SBtest)
{
    CommandToken_p->W[0] = (EIP130TOKEN_OPCODE_SPECIALFUNCTIONS << 24) |
                           (EIP130TOKEN_SUBCODE_SF_MILENAGE << 28);
    CommandToken_p->W[2] = 0;           // SQN Administration create
    CommandToken_p->W[2] |= (MASK_6_BITS & AssetNumber) << 16;
    if (fAMF_SBtest)
    {
        CommandToken_p->W[2] |= BIT_31; // Enable AMF Separation Bit test
    }
    CommandToken_p->W[3] = 0;           // Force AssetId to zero
}

/*----------------------------------------------------------------------------
 * Eip130Token_Result_SF_MilenageSqnAdminCreate
 *
 * ResultToken_p
 *     Pointer to the result token buffer.
 *
 * AssetId_p
 *      Pointer to the variable in which the AssetId must be returned.
 */
static inline void
Eip130Token_Result_SF_MilenageSqnAdminCreate(
        const Eip130Token_Result_t * const ResultToken_p,
        uint32_t * const AssetId_p)
{
    *AssetId_p = ResultToken_p->W[1];
}

/*----------------------------------------------------------------------------
 * Eip130Token_Command_SF_MilenageSqnAdminReset
 *
 * This function is used to intialize the token for the Special Functions
 * Milenage SQN administration reset.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * AssetId
 *      Asset ID of the Asset to reset.
 */
static inline void
Eip130Token_Command_SF_MilenageSqnAdminReset(
        Eip130Token_Command_t * const CommandToken_p,
        const uint32_t AssetId)
{
    CommandToken_p->W[0] = (EIP130TOKEN_OPCODE_SPECIALFUNCTIONS << 24) |
                           (EIP130TOKEN_SUBCODE_SF_MILENAGE << 28);
    CommandToken_p->W[2] = 0;           // SQN Administration reset
    CommandToken_p->W[3] = AssetId;
}

/*----------------------------------------------------------------------------
 * Eip130Token_Command_SF_MilenageSqnAdminReset
 *
 * This function is used to intialize the token for the Special Functions
 * Milenage SQN administration export.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * AssetId
 *      Asset ID of the Asset to export.
 *
 * KekAssetId
 *      Asset ID of the Key Encryption Key Asset to use.
 *
 * DataBlobAddress
 *      Data Blob buffer address.
 *
 * DataBlobLengthInBytes
 *      Data Blob buffer length.
 *
 * AssociatedData_p
 *      Associated Data address.
 *
 * AssociatedDataSizeInBytes
 *      Associated Data length.
 */
static inline void
Eip130Token_Command_SF_MilenageSqnAdminExport(
        Eip130Token_Command_t * const CommandToken_p,
        const uint32_t AssetId,
        const uint32_t KekAssetId,
        const uint64_t DataBlobAddress,
        const uint32_t DataBlobLengthInBytes,
        const uint8_t * const AssociatedData_p,
        const uint32_t AssociatedDataSizeInBytes)
{
    CommandToken_p->W[0] = (EIP130TOKEN_OPCODE_SPECIALFUNCTIONS << 24) |
                           (EIP130TOKEN_SUBCODE_SF_MILENAGE << 28);
    CommandToken_p->W[2] = 3;           // SQN Administration export
    CommandToken_p->W[3] = AssetId;     // Asset to export
    CommandToken_p->W[4] = ((MASK_8_BITS & AssociatedDataSizeInBytes) << 16) |
                           ((MASK_10_BITS & DataBlobLengthInBytes));
    CommandToken_p->W[5] = (uint32_t)(DataBlobAddress);
    CommandToken_p->W[6] = (uint32_t)(DataBlobAddress >> 32);
    CommandToken_p->W[7]  = KekAssetId; // AES-SIV Key Encryption Key

    Eip130Token_Command_WriteByteArray(CommandToken_p, 8,
                                       AssociatedData_p,
                                       AssociatedDataSizeInBytes);
}

/*----------------------------------------------------------------------------
 * Eip130Token_Result_SF_MilenageSqnAdminExport
 *
 * ResultToken_p
 *     Pointer to the result token buffer.
 *
 * DataBlobLengthInBytes_p
 *      Pointer to the variable in which the Data Blob size must be returned.
 */
static inline void
Eip130Token_Result_SF_MilenageSqnAdminExport(
        const Eip130Token_Result_t * const ResultToken_p,
        uint32_t * const DataBlobLengthInBytes_p)
{
    *DataBlobLengthInBytes_p = ResultToken_p->W[1] & MASK_10_BITS;
}

/*----------------------------------------------------------------------------
 * Eip130Token_Command_SF_MilenageAutnVerificationSqn
 *
 * This function is used to intialize the token for the Special Functions
 * Milenage AUTN Verification using the SQN Administration to do Sequence
 * Number checking.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * AssetId
 *      Asset ID of the SQN Administration Asset.
 *
 * RAND_p
 *      RAND data address. Note: 16 bytes data size is assumed.
 *
 * AUTN_p
 *      AUTN data address. Note: 16 bytes data size is assumed.
 */
static inline void
Eip130Token_Command_SF_MilenageAutnVerificationSqn(
        Eip130Token_Command_t * const CommandToken_p,
        const uint32_t AssetId,
        const uint8_t * const RAND_p,
        const uint8_t * const AUTN_p)
{
    CommandToken_p->W[0] = (EIP130TOKEN_OPCODE_SPECIALFUNCTIONS << 24) |
                           (EIP130TOKEN_SUBCODE_SF_MILENAGE << 28);
    CommandToken_p->W[2] = 1;           // AUTN Verification
    CommandToken_p->W[3] = AssetId;     // using SQN Administration

    Eip130Token_Command_WriteByteArray(CommandToken_p, 4, RAND_p, 16);
    Eip130Token_Command_WriteByteArray(CommandToken_p, 8, AUTN_p, 16);
}


/*----------------------------------------------------------------------------
 * Eip130Token_Command_SF_MilenageAutnVerification
 *
 * This function is used to intialize the token for the Special Functions
 * Milenage AUTN Verification (no Sequence Number checking).
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * AssetNumber
 *      Asset number of the Static Asset with the K and OPc.
 *
 * RAND_p
 *      RAND data address. Note: 16 bytes data size is assumed.
 *
 * AUTN_p
 *      AUTN data address. Note: 16 bytes data size is assumed.
 */
static inline void
Eip130Token_Command_SF_MilenageAutnVerification(
        Eip130Token_Command_t * const CommandToken_p,
        const uint16_t AssetNumber,
        const uint8_t * const RAND_p,
        const uint8_t * const AUTN_p)
{
    CommandToken_p->W[0] = (EIP130TOKEN_OPCODE_SPECIALFUNCTIONS << 24) |
                           (EIP130TOKEN_SUBCODE_SF_MILENAGE << 28);
    CommandToken_p->W[2] = 1;           // AUTN Verification
    CommandToken_p->W[2] |= (MASK_6_BITS & AssetNumber) << 16;
    CommandToken_p->W[3] = 0;           // Force AssetId to zero

    Eip130Token_Command_WriteByteArray(CommandToken_p, 4, RAND_p, 16);
    Eip130Token_Command_WriteByteArray(CommandToken_p, 8, AUTN_p, 16);
}

/*----------------------------------------------------------------------------
 * Eip130Token_Result_SF_MilenageAutnVerificationEMMCause
 *
 * ResultToken_p
 *     Pointer to the result token buffer.
 *
 * EMMCause_p
 *      Pointer to the variable in which the EMM Cause must be returned.
 */
static inline void
Eip130Token_Result_SF_MilenageAutnVerificationEMMCause(
        const Eip130Token_Result_t * const ResultToken_p,
        uint8_t * const EMMCause_p)
{
    *EMMCause_p = (uint8_t)(ResultToken_p->W[1] & MASK_8_BITS);
}

/*----------------------------------------------------------------------------
 * Eip130Token_Result_SF_MilenageAutnVerification
 *
 * ResultToken_p
 *     Pointer to the result token buffer.
 *
 * RES_p
 *      RES data address. Note: 8 bytes data size is assumed.
 *
 * CK_p
 *      CK data address. Note: 16 bytes data size is assumed.
 *
 * IK_p
 *      IK data address. Note: 16 bytes data size is assumed.
 */
static inline void
Eip130Token_Result_SF_MilenageAutnVerification(
        const Eip130Token_Result_t * const ResultToken_p,
        uint8_t * const RES_p,
        uint8_t * const CK_p,
        uint8_t * const IK_p)
{
    Eip130Token_Result_ReadByteArray(ResultToken_p, 2,  8, RES_p);
    Eip130Token_Result_ReadByteArray(ResultToken_p, 4, 16, CK_p);
    Eip130Token_Result_ReadByteArray(ResultToken_p, 8, 16, IK_p);
}

/*----------------------------------------------------------------------------
 * Eip130Token_Result_SF_MilenageAutnVerificationSQNAMF
 *
 * ResultToken_p
 *     Pointer to the result token buffer.
 *
 * SQN_AMF_p
 *      SQN and AMF data address. Note: 8 bytes data size is assumed.
 */
static inline void
Eip130Token_Result_SF_MilenageAutnVerificationSQNAMF(
        const Eip130Token_Result_t * const ResultToken_p,
        uint8_t * const SQN_AMF_p)
{
    Eip130Token_Result_ReadByteArray(ResultToken_p, 12, 8, SQN_AMF_p);
}

/*----------------------------------------------------------------------------
 * Eip130Token_Command_SF_MilenageAutsGeneration
 *
 * This function is used to intialize the token for the Special Functions
 * Milenage AUTS Generation.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * AssetNumber
 *      Asset number of the Static Asset with the K and OPc.
 *
 * RAND_p
 *      RAND data address. Note: 16 bytes data size is assumed.
 *
 * SQNms_AMF_p
 *      SQNms_AMF_p data address. Note: 8 bytes data size is assumed.
 */
static inline void
Eip130Token_Command_SF_MilenageAutsGeneration(
        Eip130Token_Command_t * const CommandToken_p,
        const uint16_t AssetNumber,
        const uint8_t * const RAND_p,
        const uint8_t * const SQNms_AMF_p)
{
    CommandToken_p->W[0] = (EIP130TOKEN_OPCODE_SPECIALFUNCTIONS << 24) |
                           (EIP130TOKEN_SUBCODE_SF_MILENAGE << 28);
    CommandToken_p->W[2] = 2;           // AUTS Generation
    CommandToken_p->W[2] |= (MASK_6_BITS & AssetNumber) << 16;

    Eip130Token_Command_WriteByteArray(CommandToken_p, 4, RAND_p, 16);
    Eip130Token_Command_WriteByteArray(CommandToken_p, 8, SQNms_AMF_p, 8);
}

/*----------------------------------------------------------------------------
 * Eip130Token_Result_SF_MilenageAuts
 *
 * ResultToken_p
 *     Pointer to the result token buffer.
 *
 * AUTS_p
 *      AUTS data address. Note: 12 bytes data size is assumed.
 */
static inline void
Eip130Token_Result_SF_MilenageAuts(
        const Eip130Token_Result_t * const ResultToken_p,
        uint8_t * const AUTS_p)
{
    Eip130Token_Result_ReadByteArray(ResultToken_p, 2, 14, AUTS_p);
}

/*----------------------------------------------------------------------------
 * Eip130Token_Command_SF_MilenageConformance
 *
 * This function is used to intialize the token for the Special Functions
 * Milenage conformance check.
 *
 * CommandToken_p
 *     Pointer to the command token buffer.
 *
 * RAND_p
 *      RAND data address. Note: 16 bytes data size is assumed.
 *
 * SQN_AMF_p
 *      SQN_AMF data address. Note: 8 bytes data size is assumed.
 *
 * K_p
 *      K data address. Note: 16 bytes data size is assumed.
 *
 * OP_p
 *      OP data address. Note: 16 bytes data size is assumed.
 */
static inline void
Eip130Token_Command_SF_MilenageConformance(
        Eip130Token_Command_t * const CommandToken_p,
        const uint8_t * const RAND_p,
        const uint8_t * const SQN_AMF_p,
        const uint8_t * const K_p,
        const uint8_t * const OP_p)
{
    CommandToken_p->W[0] = (EIP130TOKEN_OPCODE_SPECIALFUNCTIONS << 24) |
                           (EIP130TOKEN_SUBCODE_SF_MILENAGE << 28);
    CommandToken_p->W[2] = 4;           // Conformance check

    Eip130Token_Command_WriteByteArray(CommandToken_p,  4, RAND_p, 16);
    Eip130Token_Command_WriteByteArray(CommandToken_p,  8, SQN_AMF_p, 8);
    Eip130Token_Command_WriteByteArray(CommandToken_p, 10, K_p, 16);
    Eip130Token_Command_WriteByteArray(CommandToken_p, 14, OP_p, 16);
}


/*----------------------------------------------------------------------------
 * Eip130Token_Result_SF_MilenageConformance
 *
 * ResultToken_p
 *     Pointer to the result token buffer.
 *
 * RES_p
 *      RES data address. Note: 8 bytes data size is assumed.
 *
 * CK_p
 *      CK data address. Note: 16 bytes data size is assumed.
 *
 * IK_p
 *      IK data address. Note: 16 bytes data size is assumed.
 *
 * MACA_p
 *      RES data address. Note: 8 bytes data size is assumed.
 *
 * MACS_p
 *      CK data address. Note: 8 bytes data size is assumed.
 *
 * AK_p
 *      AK data address. Note: 6 bytes data size is assumed.
 *
 * AKstar_p
 *      AKstar data address. Note: 6 bytes data size is assumed.
 *
 * OPc_p
 *      OPc data address. Note: 16 bytes data size is assumed.
 */
static inline void
Eip130Token_Result_SF_MilenageConformance(
        const Eip130Token_Result_t * const ResultToken_p,
        uint8_t * const RES_p,
        uint8_t * const CK_p,
        uint8_t * const IK_p,
        uint8_t * const MACA_p,
        uint8_t * const MACS_p,
        uint8_t * const AK_p,
        uint8_t * const AKstar_p,
        uint8_t * const OPc_p)
{
    Eip130Token_Result_ReadByteArray(ResultToken_p,  2,  8, RES_p);
    Eip130Token_Result_ReadByteArray(ResultToken_p,  4, 16, CK_p);
    Eip130Token_Result_ReadByteArray(ResultToken_p,  8, 16, IK_p);
    Eip130Token_Result_ReadByteArray(ResultToken_p, 12,  8, MACA_p);
    Eip130Token_Result_ReadByteArray(ResultToken_p, 14,  8, MACS_p);
    Eip130Token_Result_ReadByteArray(ResultToken_p, 16,  6, AK_p);
    Eip130Token_Result_ReadByteArray(ResultToken_p, 18,  6, AKstar_p);
    Eip130Token_Result_ReadByteArray(ResultToken_p, 20, 16, OPc_p);
}


#endif /* Include Guard */


/* end of file eip130_token_sf_milenage.h */
