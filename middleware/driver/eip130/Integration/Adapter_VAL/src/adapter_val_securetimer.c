/* adapter_val_securetimer.c
 *
 * Implementation of the VAL API.
 *
 * This file implements the Secure Timer services.
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

#include "api_val_securetimer.h"        // the API to implement
#include "api_val_asset.h"              // the API to implement
#include "adapter_val_internal.h"       // val_ExchangeToken()
#include "adapter_vex.h"                // VexToken_Command_t, VexToken_Result_t


/*----------------------------------------------------------------------------
 * val_SecureTimerStart
 */
#ifndef VAL_REMOVE_SECURETIMER
ValStatus_t
val_SecureTimerStart(
        const bool fSecond,
        ValAssetId_t * const AssetId_p)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;

#ifdef VAL_STRICT_ARGS
    if (AssetId_p == NULL)
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_ASSETMANAGEMENT;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_SECURETIMER;
    t_cmd.Service.SecureTimer.Operation = VEXTOKEN_SECURETIMER_START;
    t_cmd.Service.SecureTimer.AssetId = (uint32_t)*AssetId_p;
    t_cmd.Service.SecureTimer.fSecond = fSecond;
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;
    t_res.Service.SecureTimer.AssetId = 0;

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if (funcres == VAL_SUCCESS)
    {
        if (t_res.Result == VEXTOKEN_RESULT_NO_ERROR)
        {
            *AssetId_p = (ValAssetId_t)t_res.Service.SecureTimer.AssetId;
        }
        else
        {
            funcres = (ValStatus_t)t_res.Result;
            LOG_WARN("Abort - %s()=%d\n", __func__, t_res.Result);
        }
    }

    return funcres;
}
#endif /* !VAL_REMOVE_SECURETIMER */


/*----------------------------------------------------------------------------
 * val_SecureTimerStop
 */
#ifndef VAL_REMOVE_SECURETIMER
ValStatus_t
val_SecureTimerStop(
        const ValAssetId_t AssetId,
        uint32_t * const ElapsedTime_p)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;

#ifdef VAL_STRICT_ARGS
    if ((AssetId == VAL_ASSETID_INVALID) ||
        (ElapsedTime_p == NULL))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_ASSETMANAGEMENT;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_SECURETIMER;
    t_cmd.Service.SecureTimer.Operation = VEXTOKEN_SECURETIMER_STOP;
    t_cmd.Service.SecureTimer.fSecond = false; // not used
    t_cmd.Service.SecureTimer.AssetId = (uint32_t)AssetId;
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;
    t_res.Service.SecureTimer.ElapsedTime = 0;

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if (funcres == VAL_SUCCESS)
    {
        if(t_res.Result == VEXTOKEN_RESULT_NO_ERROR)
        {
            *ElapsedTime_p = (uint32_t)t_res.Service.SecureTimer.ElapsedTime;
        }
        else
        {
            funcres = (ValStatus_t)t_res.Result;
            LOG_WARN("Abort - %s()=%d\n", __func__, t_res.Result);
        }
    }

    return funcres;
}
#endif /* !VAL_REMOVE_SECURETIMER */


/*----------------------------------------------------------------------------
 * val_SecureTimerRead
 */
#ifndef VAL_REMOVE_SECURETIMER
ValStatus_t
val_SecureTimerRead(
        const ValAssetId_t AssetId,
        uint32_t * const ElapsedTime_p)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;

#ifdef VAL_STRICT_ARGS
    if ((AssetId == VAL_ASSETID_INVALID) ||
        (ElapsedTime_p == NULL))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_ASSETMANAGEMENT;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_SECURETIMER;
    t_cmd.Service.SecureTimer.Operation = VEXTOKEN_SECURETIMER_READ;
    t_cmd.Service.SecureTimer.fSecond = false; // not used
    t_cmd.Service.SecureTimer.AssetId = (uint32_t)AssetId;
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;
    t_res.Service.SecureTimer.ElapsedTime = 0;

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if (funcres == VAL_SUCCESS)
    {
        if(t_res.Result == VEXTOKEN_RESULT_NO_ERROR)
        {
            *ElapsedTime_p = (uint32_t)t_res.Service.SecureTimer.ElapsedTime;
        }
        else
        {
            funcres = (ValStatus_t)t_res.Result;
            LOG_WARN("Abort - %s()=%d\n", __func__, t_res.Result);
        }
    }

    return funcres;
}
#endif /* !VAL_REMOVE_SECURETIMER */


/* end of file adapter_val_securetimer.c */
