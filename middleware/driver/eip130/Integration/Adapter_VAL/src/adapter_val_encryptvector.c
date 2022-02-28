/* adapter_val_encryptvector.c
 *
 * Implementation of the VAL API.
 *
 * This file implements the symmetric crypto encrypted vector for PKI services.
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

#include "c_adapter_val.h"              // configuration

#ifndef VAL_REMOVE_ENCRYPTED_VECTOR

#include "basic_defs.h"
#include "clib.h"
#include "log.h"

#include "api_val_symkeywrap.h"         // the API to implement
#include "api_val_sym.h"                // VAL_SYM_ALGO_AES_BLOCK_SIZE
#include "api_val_asset.h"              // Asset Management related information
#include "adapter_val_internal.h"       // val_ExchangeToken()
#include "adapter_vex.h"                // VexToken_Command_t, VexToken_Result_t


/*----------------------------------------------------------------------------
 * val_EncryptVectorForPKI
 */
ValStatus_t
val_EncryptVectorForPKI(
        const ValAssetId_t AssetId,
        const ValAssetNumber_t AssetNumber,
        ValOctetsOut_t * const EncVectorData_p,
        ValSize_t * const EncVectorDataSize_p)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;

#ifdef VAL_STRICT_ARGS
    if ((AssetNumber < 37) || (AssetNumber > 40) ||
        (AssetId == VAL_ASSETID_INVALID) ||
        (EncVectorDataSize_p == NULL) || (*EncVectorDataSize_p > 512) ||
        ((*EncVectorDataSize_p != 0) && (EncVectorData_p == NULL)))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_AESWRAP;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_ENCRYPTVECTOR;
    t_cmd.Service.EncVector.AssetId = (uint32_t)AssetId;
    t_cmd.Service.EncVector.AssetNumber = AssetNumber;
    t_cmd.Service.EncVector.Data_p = (uint8_t *)EncVectorData_p;
    t_cmd.Service.EncVector.DataSize = (uint32_t)*EncVectorDataSize_p;

    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;
    t_res.Service.EncVector.DataSize = 0;

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if (funcres == VAL_SUCCESS)
    {
        // Check for errors
        funcres = (ValStatus_t)t_res.Result;
        if ((funcres == VAL_SUCCESS) ||
            (funcres == VAL_INVALID_LENGTH))
        {
            // Copy real/desired size
            *EncVectorDataSize_p = t_res.Service.KeyWrap.Size;
       }
        else
        {
            LOG_WARN("Abort - %s()=%d\n", __func__, t_res.Result);
        }
    }

    return funcres;
}


#endif /* !VAL_REMOVE_ENCRYPTED_VECTOR */


/* end of file adapter_val_encryptvector.c */
