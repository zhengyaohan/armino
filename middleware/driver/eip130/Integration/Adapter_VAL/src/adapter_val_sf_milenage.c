/* adapter_val_sf_milenage.c
 *
 * Implementation of the VAL API.
 *
 * This file implements the Special Functions for the Milenage services.
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

#include "c_adapter_val.h"              // configuration

#ifndef VAL_REMOVE_SF_MILENAGE

#include "basic_defs.h"
#include "clib.h"
#include "log.h"

#include "api_val_specfuncs.h"          // the API to implement
#include "adapter_val_internal.h"       // val_ExchangeToken()
#include "adapter_vex.h"                // VexToken_Command_t, VexToken_Result_t


/*----------------------------------------------------------------------------
 * val_SFMilenageSqnAdminCreate
 */
ValStatus_t
val_SFMilenageSqnAdminCreate(
        const ValAssetNumber_t StaticAssetNumber,
        const bool fAMF_SBtest,
        ValAssetId_t * const AssetId_p)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;

#ifdef VAL_STRICT_ARGS
    if ((AssetId_p == NULL) ||
        (StaticAssetNumber > VAL_ASSET_NUMBER_MAX))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_SPECIALFUNCTIONS;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_SF_MILENAGE;
    t_cmd.Service.SF_Milenage.Operation = VEXTOKEN_MILENAGE_OP_SQNADMINCREATE;
    t_cmd.Service.SF_Milenage.AssetNumber = StaticAssetNumber;
    t_cmd.Service.SF_Milenage.fAMF_SBtest = (uint8_t)fAMF_SBtest;
    t_cmd.Service.SF_Milenage.AssetId = 0;  // Force to zero to indicate create
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;
    t_res.Service.SecureTimer.AssetId = 0;

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if (funcres == VAL_SUCCESS)
    {
        if (t_res.Result == VEXTOKEN_RESULT_NO_ERROR)
        {
            *AssetId_p = (ValAssetId_t)t_res.Service.SF_Milenage.AssetId;
        }
        else
        {
            funcres = (ValStatus_t)t_res.Result;
            LOG_WARN("Abort - %s()=%d\n", __func__, t_res.Result);
        }
    }

    return funcres;
}


/*----------------------------------------------------------------------------
 * val_SFMilenageSqnAdminReset
 */
ValStatus_t
val_SFMilenageSqnAdminReset(
        const ValAssetId_t AssetId)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;

#ifdef VAL_STRICT_ARGS
    if (AssetId == VAL_ASSETID_INVALID)
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_SPECIALFUNCTIONS;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_SF_MILENAGE;
    t_cmd.Service.SF_Milenage.Operation = VEXTOKEN_MILENAGE_OP_SQNADMINCREATE; // Includes reset
    t_cmd.Service.SF_Milenage.AssetId = (uint32_t)AssetId; // Indicates Reset
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if ((funcres == VAL_SUCCESS) &&
        (t_res.Result != VEXTOKEN_RESULT_NO_ERROR))
    {
        // Error
        funcres = (ValStatus_t)t_res.Result;
        LOG_WARN("Abort - %s()=%d\n", __func__, funcres);
    }

    return funcres;
}


/*----------------------------------------------------------------------------
 * val_SFMilenageSqnAdminExport
 */
ValStatus_t
val_SFMilenageSqnAdminExport(
        const ValAssetId_t AssetId,
        const ValAssetId_t KekAssetId,
        ValOctetsIn_t * const  AssociatedData_p,
        const ValSize_t AssociatedDataSize,
        ValOctetsOut_t * const DataBlob_p,
        ValSize_t * const SizeDataBlob_p)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;

#ifdef VAL_STRICT_ARGS
    if ((AssetId == VAL_ASSETID_INVALID) ||
        (KekAssetId == VAL_ASSETID_INVALID) ||
        (AssociatedData_p == NULL) ||
        (AssociatedDataSize < VAL_KEYBLOB_AAD_MIN_SIZE) ||
        (AssociatedDataSize > VAL_KEYBLOB_AAD_MAX_SIZE) ||
        (DataBlob_p == NULL) ||
        (SizeDataBlob_p == NULL) ||
        (*SizeDataBlob_p < VAL_KEYBLOB_SIZE(200)))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_SPECIALFUNCTIONS;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_SF_MILENAGE;
    t_cmd.Service.SF_MilenageExport.Operation = VEXTOKEN_MILENAGE_OP_SQNADMINEXPORT;
    t_cmd.Service.SF_MilenageExport.AssetId = (uint32_t)AssetId;
    t_cmd.Service.SF_MilenageExport.KekAssetId = (uint32_t)KekAssetId;
    memcpy(t_cmd.Service.SF_MilenageExport.AssociatedData, AssociatedData_p, AssociatedDataSize);
    t_cmd.Service.SF_MilenageExport.AssociatedDataSize = (uint32_t)AssociatedDataSize;
    t_cmd.Service.SF_MilenageExport.DataBlob_p = (uint8_t *)DataBlob_p;
    t_cmd.Service.SF_MilenageExport.DataBlobSize = (uint32_t)*SizeDataBlob_p;
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;
    t_res.Service.SF_MilenageExport.DataBlobSize = 0;

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if (funcres == VAL_SUCCESS)
    {
        // Check for errors
        if (t_res.Result == VEXTOKEN_RESULT_NO_ERROR)
        {
            *SizeDataBlob_p = t_res.Service.SF_MilenageExport.DataBlobSize;
        }
        else
        {
            funcres = (ValStatus_t)t_res.Result;
            LOG_WARN("Abort - %s()=%d\n", __func__, t_res.Result);
        }
    }

    return funcres;
}


/*----------------------------------------------------------------------------
 * val_SFMilenageAutnVerification
 */
ValStatus_t
val_SFMilenageAutnVerification(
        const ValAssetNumber_t StaticAssetNumber,
        ValOctetsIn_t * const RAND_p,
        ValOctetsIn_t * const AUTN_p,
        ValOctetsOut_t * const RES_p,
        ValOctetsOut_t * const CK_p,
        ValOctetsOut_t * const IK_p,
        ValOctetsOut_t * const SQN_p,
        ValOctetsOut_t * const AMF_p)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;

#ifdef VAL_STRICT_ARGS
    if ((StaticAssetNumber > VAL_ASSET_NUMBER_MAX) ||
        (RAND_p == 0) || (AUTN_p == 0) ||
        (RES_p == 0) || (CK_p == 0) || (IK_p == 0) ||
        (SQN_p == 0) || (AMF_p == 0))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_SPECIALFUNCTIONS;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_SF_MILENAGE;
    t_cmd.Service.SF_Milenage.Operation = VEXTOKEN_MILENAGE_OP_AUTNVERIFICATION;
    t_cmd.Service.SF_Milenage.AssetNumber = StaticAssetNumber;
    t_cmd.Service.SecureTimer.AssetId = 0; // Force to zero
    memcpy(t_cmd.Service.SF_Milenage.RAND, RAND_p, sizeof(t_cmd.Service.SF_Milenage.RAND));
    memcpy(t_cmd.Service.SF_Milenage.AUTN, AUTN_p, sizeof(t_cmd.Service.SF_Milenage.AUTN));
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;
    memset(&t_res.Service.SF_Milenage, 0, sizeof(t_res.Service.SF_Milenage));

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if (funcres == VAL_SUCCESS)
    {
        // Check for errors
        if (t_res.Result == VEXTOKEN_RESULT_NO_ERROR)
        {
            memcpy(RES_p, t_res.Service.SF_Milenage.RES, sizeof(t_res.Service.SF_Milenage.RES));
            memcpy(CK_p, t_res.Service.SF_Milenage.CK, sizeof(t_res.Service.SF_Milenage.CK));
            memcpy(IK_p, t_res.Service.SF_Milenage.IK, sizeof(t_res.Service.SF_Milenage.IK));
            memcpy(SQN_p, t_res.Service.SF_Milenage.SQN, sizeof(t_res.Service.SF_Milenage.SQN));
            memcpy(AMF_p, t_res.Service.SF_Milenage.AMF, sizeof(t_res.Service.SF_Milenage.AMF));
        }
        else
        {
            funcres = (ValStatus_t)t_res.Result;
            LOG_WARN("Abort - %s()=%d\n", __func__, t_res.Result);
        }
    }

    return funcres;
}


/*----------------------------------------------------------------------------
 * val_SFMilenageAutnVerificationSqn
 */
ValStatus_t
val_SFMilenageAutnVerificationSqn(
        const ValAssetId_t AssetId,
        ValOctetsIn_t * const RAND_p,
        ValOctetsIn_t * const AUTN_p,
        uint32_t * const EMMCause_p,
        ValOctetsOut_t * const RES_p,
        ValOctetsOut_t * const CK_p,
        ValOctetsOut_t * const IK_p,
        ValOctetsOut_t * const AUTS_p)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;

#ifdef VAL_STRICT_ARGS
    if ((AssetId == VAL_ASSETID_INVALID) ||
        (RAND_p == 0) || (AUTN_p == 0) ||
        (RES_p == 0) || (CK_p == 0) || (IK_p == 0) ||
        (EMMCause_p == 0) || (AUTS_p == 0))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_SPECIALFUNCTIONS;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_SF_MILENAGE;
    t_cmd.Service.SF_Milenage.Operation = VEXTOKEN_MILENAGE_OP_AUTNVERIFICATION;
    t_cmd.Service.SecureTimer.AssetId = AssetId;
    memcpy(t_cmd.Service.SF_Milenage.RAND, RAND_p, sizeof(t_cmd.Service.SF_Milenage.RAND));
    memcpy(t_cmd.Service.SF_Milenage.AUTN, AUTN_p, sizeof(t_cmd.Service.SF_Milenage.AUTN));
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;
    memset(&t_res.Service.SF_Milenage, 0, sizeof(t_res.Service.SF_Milenage));

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if (funcres == VAL_SUCCESS)
    {
        // Check for errors
        if (t_res.Result == VEXTOKEN_RESULT_NO_ERROR)
        {
            memcpy(RES_p, t_res.Service.SF_Milenage.RES, sizeof(t_res.Service.SF_Milenage.RES));
            memcpy(CK_p, t_res.Service.SF_Milenage.CK, sizeof(t_res.Service.SF_Milenage.CK));
            memcpy(IK_p, t_res.Service.SF_Milenage.IK, sizeof(t_res.Service.SF_Milenage.IK));
            *EMMCause_p = 0;
        }
        else
        {
            funcres = (ValStatus_t)t_res.Result;
            if (funcres == VAL_VERIFY_ERROR)
            {
                *EMMCause_p = t_res.Service.SF_Milenage.EMMCause;
                // no special test required because AUTS is zero when EMM Cause is not 21
                memcpy(AUTS_p, t_res.Service.SF_Milenage.AUTS, sizeof(t_res.Service.SF_Milenage.AUTS));
            }
            else
            {
                LOG_WARN("Abort - %s()=%d\n", __func__, t_res.Result);
            }
        }
    }

    return funcres;
}


/*----------------------------------------------------------------------------
 * val_SFMilenageAutnVerification
 */
ValStatus_t
val_SFMilenageAutsGeneration(
        const ValAssetNumber_t StaticAssetNumber,
        ValOctetsIn_t * const RAND_p,
        ValOctetsIn_t * const SQNms_p,
        ValOctetsIn_t * const AMF_p,
        ValOctetsOut_t * const AUTS_p)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;

#ifdef VAL_STRICT_ARGS
    if ((StaticAssetNumber > VAL_ASSET_NUMBER_MAX) ||
        (RAND_p == 0) || (SQNms_p == 0) || (AMF_p == 0) ||
        (AUTS_p == 0))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_SPECIALFUNCTIONS;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_SF_MILENAGE;
    t_cmd.Service.SF_Milenage.Operation = VEXTOKEN_MILENAGE_OP_AUTSGENERATION;
    t_cmd.Service.SF_Milenage.AssetNumber = StaticAssetNumber;
    memcpy(t_cmd.Service.SF_Milenage.RAND, RAND_p, sizeof(t_cmd.Service.SF_Milenage.RAND));
    memcpy(t_cmd.Service.SF_Milenage.SQN, SQNms_p, sizeof(t_cmd.Service.SF_Milenage.SQN));
    memcpy(t_cmd.Service.SF_Milenage.AMF, AMF_p, sizeof(t_cmd.Service.SF_Milenage.AMF));
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;
    memset(&t_res.Service.SF_Milenage, 0, sizeof(t_res.Service.SF_Milenage));

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if (funcres == VAL_SUCCESS)
    {
        // Check for errors
        if (t_res.Result == VEXTOKEN_RESULT_NO_ERROR)
        {
            memcpy(AUTS_p, t_res.Service.SF_Milenage.AUTS, sizeof(t_res.Service.SF_Milenage.AUTS));
        }
        else
        {
            funcres = (ValStatus_t)t_res.Result;
            LOG_WARN("Abort - %s()=%d\n", __func__, t_res.Result);
        }
    }

    return funcres;
}


/*----------------------------------------------------------------------------
 * val_SFMilenageAutnVerification
 */
ValStatus_t
val_SFMilenageConformance(
        ValOctetsIn_t * const RAND_p,
        ValOctetsIn_t * const SQN_p,
        ValOctetsIn_t * const AMF_p,
        ValOctetsIn_t * const K_p,
        ValOctetsIn_t * const OP_p,
        ValOctetsOut_t * const RES_p,
        ValOctetsOut_t * const CK_p,
        ValOctetsOut_t * const IK_p,
        ValOctetsOut_t * const MACA_p,
        ValOctetsOut_t * const MACS_p,
        ValOctetsOut_t * const AK_p,
        ValOctetsOut_t * const AKstar_p,
        ValOctetsOut_t * const OPc_p)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;

#ifdef VAL_STRICT_ARGS
    if ((RAND_p == 0) || (SQN_p == 0) || (AMF_p == 0) ||
        (K_p == 0) || (OP_p == 0) ||
        (RES_p == 0) || (CK_p == 0) || (IK_p == 0) ||
        (MACA_p == 0) || (MACS_p == 0) ||
        (AK_p == 0) || (AKstar_p == 0) || (OPc_p == 0))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_SPECIALFUNCTIONS;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_SF_MILENAGE;
    t_cmd.Service.SF_Milenage.Operation = VEXTOKEN_MILENAGE_OP_CONFORMANCECHECK;
    t_cmd.Service.SecureTimer.AssetId = 0; // Force to zero
    memcpy(t_cmd.Service.SF_Milenage.RAND, RAND_p, sizeof(t_cmd.Service.SF_Milenage.RAND));
    memcpy(t_cmd.Service.SF_Milenage.SQN, SQN_p, sizeof(t_cmd.Service.SF_Milenage.SQN));
    memcpy(t_cmd.Service.SF_Milenage.AMF, AMF_p, sizeof(t_cmd.Service.SF_Milenage.AMF));
    memcpy(t_cmd.Service.SF_Milenage.K, K_p, sizeof(t_cmd.Service.SF_Milenage.K));
    memcpy(t_cmd.Service.SF_Milenage.OP, OP_p, sizeof(t_cmd.Service.SF_Milenage.OP));
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;
    memset(&t_res.Service.SF_Milenage, 0, sizeof(t_res.Service.SF_Milenage));

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if (funcres == VAL_SUCCESS)
    {
        // Check for errors
        if (t_res.Result == VEXTOKEN_RESULT_NO_ERROR)
        {
            memcpy(RES_p, t_res.Service.SF_Milenage.RES, sizeof(t_res.Service.SF_Milenage.RES));
            memcpy(CK_p, t_res.Service.SF_Milenage.CK, sizeof(t_res.Service.SF_Milenage.CK));
            memcpy(IK_p, t_res.Service.SF_Milenage.IK, sizeof(t_res.Service.SF_Milenage.IK));
            memcpy(MACA_p, t_res.Service.SF_Milenage.MACA, sizeof(t_res.Service.SF_Milenage.MACA));
            memcpy(MACS_p, t_res.Service.SF_Milenage.MACS, sizeof(t_res.Service.SF_Milenage.MACS));
            memcpy(AK_p, t_res.Service.SF_Milenage.AK, sizeof(t_res.Service.SF_Milenage.AK));
            memcpy(AKstar_p, t_res.Service.SF_Milenage.AKstar, sizeof(t_res.Service.SF_Milenage.AKstar));
            memcpy(OPc_p, t_res.Service.SF_Milenage.OPc, sizeof(t_res.Service.SF_Milenage.OPc));
        }
        else
        {
            funcres = (ValStatus_t)t_res.Result;
            LOG_WARN("Abort - %s()=%d\n", __func__, t_res.Result);
        }
    }

    return funcres;
}


#endif /* !VAL_REMOVE_SF_MILENAGE */

/* end of file adapter_val_sf_milenage.c */
