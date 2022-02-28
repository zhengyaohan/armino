/* adapter_val_monotonic.c
 *
 * Implementation of the VAL API.
 *
 * This file implements the monotonic counter services.
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

#include "api_val_asset.h"              // the API to implement
#include "adapter_val_internal.h"       // val_ExchangeToken()
#include "adapter_vex.h"                // VexToken_Command_t, VexToken_Result_t


/*----------------------------------------------------------------------------
 * val_MonotonicCounterRead
 */
#ifndef VAL_REMOVE_MONOTONIC_COUNTER_READ
ValStatus_t
val_MonotonicCounterRead(
        const ValAssetId_t AssetId,
        ValOctetsOut_t * const Data_p,
        const ValSize_t DataSize)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;

#ifdef VAL_STRICT_ARGS
    if ((AssetId == VAL_ASSETID_INVALID) ||
        (Data_p == NULL) ||
        (DataSize == 0))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_ASSETMANAGEMENT;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_MONOTONICREAD;
    t_cmd.Service.MonotonicCounter.AssetId = (uint32_t)AssetId;
    t_cmd.Service.MonotonicCounter.Data_p = (const uint8_t *)Data_p;
    t_cmd.Service.MonotonicCounter.DataSize = (uint32_t)DataSize;
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;
    t_res.Service.MonotonicCounter.DataSize = 0;

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if (funcres == VAL_SUCCESS)
    {
        if (t_res.Result == VEXTOKEN_RESULT_NO_ERROR)
        {
            uint32_t i = t_res.Service.MonotonicCounter.DataSize;
            for (; i < DataSize; i++)
            {
                Data_p[i] = 0;
            }
        }
        else
        {
            funcres = (ValStatus_t)t_res.Result;
            LOG_WARN("Abort - %s()=%d\n", __func__, t_res.Result);
        }
    }

    return funcres;
}
#endif /* !VAL_REMOVE_MONOTONIC_COUNTER_READ */


/*----------------------------------------------------------------------------
 * val_MonotonicCounterIncrement
 */
#ifndef VAL_REMOVE_MONOTONIC_COUNTER_INC
ValStatus_t
val_MonotonicCounterIncrement(
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
    t_cmd.OpCode = VEXTOKEN_OPCODE_ASSETMANAGEMENT;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_MONOTONICINCR;
    t_cmd.Service.MonotonicCounter.AssetId = (uint32_t)AssetId;
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
#endif /* !VAL_REMOVE_MONOTONIC_COUNTER_INC */


/* end of file adapter_val_monotonic.c */
