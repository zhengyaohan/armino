/* adapter_val_asym_pka.c
 *
 * Implementation of the VAL API.
 *
 * This file implements the asymmetric crypto services without Asset use.
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

#include "c_adapter_val.h"              // configuration

#include "basic_defs.h"
#include "clib.h"
#include "log.h"

#include "api_val_asym.h"               // the API to implement
#include "api_val_claim.h"              // Used API functions
#include "adapter_val_internal.h"       // Various defines and typedefs


/*----------------------------------------------------------------------------
 * val_AsymPkaClaim
 */
#ifndef VAL_REMOVE_ASYM_PK_WITHOUT_ASSET
ValStatus_t
val_AsymPkaClaim(
        const uint8_t Nwords,
        const uint8_t Mwords,
        const uint8_t Mmask)
{
    ValStatus_t funcres;

#ifdef VAL_STRICT_ARGS
    if ((Nwords == 0) && (Mwords == 0) && (Mmask == 0))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

#ifndef VAL_REMOVE_CLAIM
    funcres = val_Claim();
    if (funcres == VAL_SUCCESS)
#endif
    {
        VexToken_Command_t t_cmd;
        VexToken_Result_t t_res;

        // Format service request
        t_cmd.OpCode = VEXTOKEN_OPCODE_PUBLIC_KEY;
        t_cmd.SubCode = VEXTOKEN_SUBCODE_PK_NOASSETS;
        t_cmd.Service.PkOperation.Operation = VEXTOKEN_PK_OP_NUMSETN;
        t_cmd.Service.PkOperation.Nwords = Nwords;
        t_cmd.Service.PkOperation.Mwords = Mwords;
        t_cmd.Service.PkOperation.Mmask  = Mmask;
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
#ifndef VAL_REMOVE_CLAIM
        if (funcres != VAL_SUCCESS)
        {
            (void)val_ClaimRelease();
        }
#endif
    }

    return funcres;
}
#endif


/*----------------------------------------------------------------------------
 * val_AsymPkaLoadVector
 */
#ifndef VAL_REMOVE_ASYM_PK_WITHOUT_ASSET
ValStatus_t
val_AsymPkaLoadVector(
        const uint8_t Index,
        ValOctetsIn_t * const Vector_p,
        const ValSize_t VectorSize)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;

#ifdef VAL_STRICT_ARGS
    if ((Index > 15) ||
        (Vector_p == NULL) ||
        (VectorSize == 0))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_PUBLIC_KEY;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_PK_NOASSETS;
    t_cmd.Service.PkOperation.Operation = VEXTOKEN_PK_OP_NUMLOAD;
    t_cmd.Service.PkOperation.Index = Index;
    t_cmd.Service.PkOperation.InData_p = (const uint8_t *)Vector_p;
    t_cmd.Service.PkOperation.InDataSize = (uint32_t)VectorSize;
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
#endif


/*----------------------------------------------------------------------------
 * val_AsymPkaExecuteOperation
 */
#ifndef VAL_REMOVE_ASYM_PK_WITHOUT_ASSET
ValStatus_t
val_AsymPkaExecuteOperation(
        const ValAsymPkaOperation_t Operation,
        const uint32_t PublicExponent,
        ValOctetsIn_t * const InData_p,
        const ValSize_t InDataSize,
        ValOctetsOut_t * const OutData_p,
        ValSize_t * const OutDataSize_p)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_PUBLIC_KEY;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_PK_NOASSETS;
    t_cmd.Service.PkOperation.PublicExponent = 0;
    switch (Operation)
    {
    case VAL_ASYM_PKA_OP_MODEXPE:
        t_cmd.Service.PkOperation.Operation = VEXTOKEN_PK_OP_MODEXPE;
        t_cmd.Service.PkOperation.PublicExponent = PublicExponent;
        break;
    case VAL_ASYM_PKA_OP_MODEXPD:
        t_cmd.Service.PkOperation.Operation = VEXTOKEN_PK_OP_MODEXPD;
        break;
    case VAL_ASYM_PKA_OP_MODEXPCRT:
        t_cmd.Service.PkOperation.Operation = VEXTOKEN_PK_OP_MODEXPCRT;
        break;
    case VAL_ASYM_PKA_OP_ECMONTMUL:
        t_cmd.Service.PkOperation.Operation = VEXTOKEN_PK_OP_ECMONTMUL;
        break;
    case VAL_ASYM_PKA_OP_ECCMUL:
        t_cmd.Service.PkOperation.Operation = VEXTOKEN_PK_OP_ECCMUL;
        break;
    case VAL_ASYM_PKA_OP_ECCADD:
        t_cmd.Service.PkOperation.Operation = VEXTOKEN_PK_OP_ECCADD;
        break;
    case VAL_ASYM_PKA_OP_DSASIGN:
        t_cmd.Service.PkOperation.Operation = VEXTOKEN_PK_OP_DSASIGN;
        break;
    case VAL_ASYM_PKA_OP_DSAVERIFY:
        t_cmd.Service.PkOperation.Operation = VEXTOKEN_PK_OP_DSAVERIFY;
        break;
    case VAL_ASYM_PKA_OP_ECDSASIGN:
        t_cmd.Service.PkOperation.Operation = VEXTOKEN_PK_OP_ECDSASIGN;
        break;
    case VAL_ASYM_PKA_OP_ECDSAVERIFY:
        t_cmd.Service.PkOperation.Operation = VEXTOKEN_PK_OP_ECDSAVERIFY;
        break;
    default:
        return VAL_BAD_ARGUMENT;
    }
    t_cmd.Service.PkOperation.InData_p = (const uint8_t *)InData_p;
    t_cmd.Service.PkOperation.InDataSize = (uint32_t)InDataSize;
    t_cmd.Service.PkOperation.OutData_p = (uint8_t *)OutData_p;
    t_cmd.Service.PkOperation.OutDataSize = (uint32_t)*OutDataSize_p;
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;
    t_res.Service.PkOperation.OutDataSize = VEXTOKEN_RESULT_NO_ERROR;

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if (funcres == VAL_SUCCESS)
    {
        // Check for errors
        if (t_res.Result == VEXTOKEN_RESULT_NO_ERROR)
        {
            *OutDataSize_p = t_res.Service.PkOperation.OutDataSize;
        }
        else
        {
            funcres = (ValStatus_t)t_res.Result;
            LOG_WARN("Abort - %s()=%d\n", __func__, t_res.Result);
        }
    }

    return funcres;
}
#endif


/*----------------------------------------------------------------------------
 * val_AsymPkaClaimRelease
 */
#ifndef VAL_REMOVE_ASYM_PK_WITHOUT_ASSET
ValStatus_t
val_AsymPkaClaimRelease(void)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_PUBLIC_KEY;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_PK_NOASSETS;
    t_cmd.Service.PkOperation.Operation = VEXTOKEN_PK_OP_NUMSETN;
    t_cmd.Service.PkOperation.Nwords = 0;
    t_cmd.Service.PkOperation.Mwords = 0;
    t_cmd.Service.PkOperation.Mmask  = 0;
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
#ifndef VAL_REMOVE_CLAIM
    if (funcres == VAL_SUCCESS)
    {
        funcres = val_ClaimRelease();
    }
#endif

    return funcres;
}
#endif

/* end of file adapter_val_asym_pka.c */
