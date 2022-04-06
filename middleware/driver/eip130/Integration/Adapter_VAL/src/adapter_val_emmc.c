/* adapter_val_emmc.c
 *
 * Implementation of the VAL API.
 *
 * This file implements the eMMC related services.
 */

/*****************************************************************************
* Copyright (c) 2016-2019 INSIDE Secure B.V. All Rights Reserved.
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

#include "api_val_emmc.h"               // the API to implement
#include "adapter_val_internal.h"       // val_ExchangeToken()
#include "adapter_vex.h"                // VexToken_Command_t, VexToken_Result_t


/*----------------------------------------------------------------------------
 * val_eMMCReadRequest
 */
#ifndef VAL_REMOVE_EMMC
ValStatus_t
val_eMMCReadRequest(
        const ValAssetId_t KeyAssetId,
        ValAssetId_t * const StateASId_p,
        ValOctetsOut_t * const Nonce_p,
        ValSize_t * const NonceLength_p)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;

#ifdef VAL_STRICT_ARGS
    if ((KeyAssetId == VAL_ASSETID_INVALID) ||
        (StateASId_p == NULL) ||
        (Nonce_p == NULL) ||
        (NonceLength_p == NULL))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    if (*NonceLength_p < 16)
    {
        *NonceLength_p = 16;
        return VAL_BUFFER_TOO_SMALL;
    }

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_EMMC;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_EMMC_RDREQ;
    t_cmd.Service.eMMCRdRequest.AssetId = KeyAssetId;
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;
    t_res.Service.eMMCRdRequest.AssetId = 0;
    ZEROINIT(t_res.Service.eMMCRdRequest.Nonce);

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if (funcres == VAL_SUCCESS)
    {
        if (t_res.Result == VEXTOKEN_RESULT_NO_ERROR)
        {
            *StateASId_p = t_res.Service.eMMCRdRequest.AssetId;

            // Copy Nonce
            memcpy(Nonce_p, t_res.Service.eMMCRdRequest.Nonce, 16);
            *NonceLength_p = 16;
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
 * val_eMMCReadVerify
 */
#ifndef VAL_REMOVE_EMMC
ValStatus_t
val_eMMCReadVerify(
        const ValAssetId_t StateASId,
        ValOctetsIn_t * const Data_p,
        const ValSize_t DataSize,
        ValOctetsIn_t * const MAC_p,
        const ValSize_t MACSize)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;

#ifdef VAL_STRICT_ARGS
    if ((StateASId == VAL_ASSETID_INVALID) ||
        (Data_p == NULL) ||
        (DataSize == 0) ||
        (MAC_p == NULL) ||
        (MACSize != (256/8)))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Format service request
    t_cmd.OpCode  = VEXTOKEN_OPCODE_EMMC;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_EMMC_RDVER;
    t_cmd.Service.eMMCRdVerifyWrReqVer.StateAssetId = StateASId;
    t_cmd.Service.eMMCRdVerifyWrReqVer.Data_p = (const uint8_t *)Data_p;
    t_cmd.Service.eMMCRdVerifyWrReqVer.DataSize = (uint32_t)DataSize;
    memcpy(t_cmd.Service.eMMCRdVerifyWrReqVer.Mac, MAC_p, MACSize);
    t_cmd.Service.eMMCRdVerifyWrReqVer.MacSize = (uint32_t)MACSize;

    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if ((funcres == VAL_SUCCESS) &&
        (t_res.Result != VEXTOKEN_RESULT_NO_ERROR))
    {
        // Error
        funcres = (ValStatus_t)t_res.Result;
        LOG_WARN("Abort - %s()=%d\n", __func__, t_res.Result);
    }

    return funcres;
}
#endif


/*----------------------------------------------------------------------------
 * val_eMMCReadWriteCounterRequest
 */
#ifndef VAL_REMOVE_EMMC
ValStatus_t
val_eMMCReadWriteCounterRequest(
        const ValAssetId_t StateASId,
        ValOctetsOut_t * const Nonce_p,
        ValSize_t * const NonceLength_p)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;

#ifdef VAL_STRICT_ARGS
    if ((StateASId == VAL_ASSETID_INVALID) ||
        (Nonce_p == NULL) ||
        (NonceLength_p == NULL))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    if (*NonceLength_p < 16)
    {
        *NonceLength_p = 16;
        return VAL_BUFFER_TOO_SMALL;
    }

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_EMMC;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_EMMC_RDWRCNTREQ;
    t_cmd.Service.eMMCRdRequest.AssetId = StateASId;
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;
    ZEROINIT(t_res.Service.eMMCRdRequest.Nonce);

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if (funcres == VAL_SUCCESS)
    {
        if (t_res.Result == VEXTOKEN_RESULT_NO_ERROR)
        {
            // Copy Nonce
            memcpy(Nonce_p, t_res.Service.eMMCRdRequest.Nonce, 16);
            *NonceLength_p = 16;
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
 * val_eMMCReadWriteCounterVerify
 */
#ifndef VAL_REMOVE_EMMC
ValStatus_t
val_eMMCReadWriteCounterVerify(
        const ValAssetId_t StateASId,
        ValOctetsIn_t * const Data_p,
        const ValSize_t DataSize,
        ValOctetsIn_t * const MAC_p,
        const ValSize_t MACSize)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;

#ifdef VAL_STRICT_ARGS
    if ((StateASId == VAL_ASSETID_INVALID) ||
        (Data_p == NULL) ||
        (DataSize == 0) ||
        (MAC_p == NULL) ||
        (MACSize != (256/8)))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Format service request
    t_cmd.OpCode  = VEXTOKEN_OPCODE_EMMC;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_EMMC_RDWRCNTVER;
    t_cmd.Service.eMMCRdVerifyWrReqVer.StateAssetId = StateASId;
    t_cmd.Service.eMMCRdVerifyWrReqVer.Data_p = (const uint8_t *)Data_p;
    t_cmd.Service.eMMCRdVerifyWrReqVer.DataSize = (uint32_t)DataSize;
    memcpy(t_cmd.Service.eMMCRdVerifyWrReqVer.Mac, MAC_p, MACSize);
    t_cmd.Service.eMMCRdVerifyWrReqVer.MacSize = (uint32_t)MACSize;

    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if ((funcres == VAL_SUCCESS) &&
        (t_res.Result != VEXTOKEN_RESULT_NO_ERROR))
    {
        // Error
        funcres = (ValStatus_t)t_res.Result;
        LOG_WARN("Abort - %s()=%d\n", __func__, t_res.Result);
    }

    return funcres;
}
#endif


/*----------------------------------------------------------------------------
 * val_eMMCWriteRequest
 */
#ifndef VAL_REMOVE_EMMC
ValStatus_t
val_eMMCWriteRequest(
        const ValAssetId_t StateASId,
        ValOctetsIn_t * const Data_p,
        const ValSize_t DataSize,
        ValOctetsOut_t * const MAC_p,
        ValSize_t * const MACSize_p)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;

#ifdef VAL_STRICT_ARGS
    if ((StateASId == VAL_ASSETID_INVALID) ||
        (Data_p == NULL) ||
        (DataSize == 0) ||
        (MAC_p == NULL) ||
        (MACSize_p == NULL))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    if (*MACSize_p < (256/8))
    {
        *MACSize_p = (256/8);
        return VAL_BUFFER_TOO_SMALL;
    }

    // Format service request
    t_cmd.OpCode  = VEXTOKEN_OPCODE_EMMC;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_EMMC_WRREQ;
    t_cmd.Service.eMMCRdVerifyWrReqVer.StateAssetId = StateASId;
    t_cmd.Service.eMMCRdVerifyWrReqVer.Data_p = (const uint8_t *)Data_p;
    t_cmd.Service.eMMCRdVerifyWrReqVer.DataSize = (uint32_t)DataSize;
    t_cmd.Service.eMMCRdVerifyWrReqVer.MacSize = (uint32_t)*MACSize_p;
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;
    ZEROINIT(t_res.Service.eMMCRdVerifyWrReqVer.Mac);

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if (funcres == VAL_SUCCESS)
    {
        if (t_res.Result == VEXTOKEN_RESULT_NO_ERROR)
        {
            // Copy generated MAC value
            memcpy(MAC_p, t_res.Service.eMMCRdVerifyWrReqVer.Mac, (256/8));
            *MACSize_p = (256/8);
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
 * val_eMMCWriteVerify
 */
#ifndef VAL_REMOVE_EMMC
ValStatus_t
val_eMMCWriteVerify(
        const ValAssetId_t StateASId,
        ValOctetsIn_t * const Data_p,
        const ValSize_t DataSize,
        ValOctetsIn_t * const MAC_p,
        const ValSize_t MACSize)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;

#ifdef VAL_STRICT_ARGS
    if ((StateASId == VAL_ASSETID_INVALID) ||
        (Data_p == NULL) ||
        (DataSize == 0) ||
        (MAC_p == NULL) ||
        (MACSize != (256/8)))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Format service request
    t_cmd.OpCode  = VEXTOKEN_OPCODE_EMMC;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_EMMC_WRVER;
    t_cmd.Service.eMMCRdVerifyWrReqVer.StateAssetId = StateASId;
    t_cmd.Service.eMMCRdVerifyWrReqVer.Data_p = (const uint8_t *)Data_p;
    t_cmd.Service.eMMCRdVerifyWrReqVer.DataSize = (uint32_t)DataSize;
    memcpy(t_cmd.Service.eMMCRdVerifyWrReqVer.Mac, MAC_p, MACSize);
    t_cmd.Service.eMMCRdVerifyWrReqVer.MacSize = (uint32_t)MACSize;

    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if ((funcres == VAL_SUCCESS) &&
        (t_res.Result != VEXTOKEN_RESULT_NO_ERROR))
    {
        // Error
        funcres = (ValStatus_t)t_res.Result;
        LOG_WARN("Abort - %s()=%d\n", __func__, t_res.Result);
    }

    return funcres;
}
#endif


/* end of file adapter_val_emmc.c */
