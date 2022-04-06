/* adapter_val_sym_keywrap.c
 *
 * Implementation of the VAL API.
 *
 * This file implements the symmetric crypto key wrap services.
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

#include "c_adapter_val.h"              // configuration

#ifndef VAL_REMOVE_SYM_AESKEYWRAP

#include "basic_defs.h"
#include "clib.h"
#include "log.h"

#include "api_val_symkeywrap.h"         // the API to implement
#include "api_val_sym.h"                // VAL_SYM_ALGO_AES_BLOCK_SIZE
#include "api_val_asset.h"              // Asset Management related information
#include "api_val_system.h"             // val_IsAccessSecure()
#include "adapter_val_internal.h"       // val_ExchangeToken()
#include "adapter_vex.h"                // VexToken_Command_t, VexToken_Result_t


#define VAL_KEYWRAP_IV_SIZE         (64/8)
#define VAL_KEYWRAP_DATA_SIZE(x)    (((x) + VAL_KEYWRAP_IV_SIZE - 1) & (unsigned int)~(VAL_KEYWRAP_IV_SIZE - 1))
#define VAL_KEYWRAP_ENCDATA_SIZE(x) (VAL_KEYWRAP_DATA_SIZE(x) + VAL_KEYWRAP_IV_SIZE)
#define VAL_KEYWRAP_DECDATA_SIZE(x) (VAL_KEYWRAP_DATA_SIZE(x) - VAL_KEYWRAP_IV_SIZE)


/*----------------------------------------------------------------------------
 * valLocal_AesKeyWrap
 */
static ValStatus_t
valLocal_AesKeyWrap(
        bool fWrap,
        ValAssetId_Optional_t KeyAssetId,
        ValOctetsIn_Optional_t * const Key_p,
        const ValSize_t KeySize,
        ValOctetsIn_t * const SrcData_p,
        const ValSize_t SrcDataSize,
        ValOctetsOut_t * const DstData_p,
        ValSize_t * const DstDataSize_p)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_AESWRAP;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_AESKEYWRAP;
    t_cmd.Service.KeyWrap.fWrap = (uint8_t)fWrap;
    t_cmd.Service.KeyWrap.KeySize = (uint32_t)KeySize;
    t_cmd.Service.KeyWrap.KeyAssetId = (uint32_t)KeyAssetId;
    if (KeyAssetId == VAL_ASSETID_INVALID)
    {
        memcpy(t_cmd.Service.KeyWrap.Key, Key_p, KeySize);
    }
    t_cmd.Service.KeyWrap.SrcData_p = (const uint8_t *)SrcData_p;
    t_cmd.Service.KeyWrap.SrcDataSize = (uint32_t)SrcDataSize;
    t_cmd.Service.KeyWrap.DstData_p = (uint8_t *)DstData_p;
    t_cmd.Service.KeyWrap.DstDataSize = (uint32_t)*DstDataSize_p;

    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;
    t_res.Service.KeyWrap.Size = 0;

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if (funcres == VAL_SUCCESS)
    {
        // Check for errors
        if (t_res.Result == VEXTOKEN_RESULT_NO_ERROR)
        {
            // Copy real size
            *DstDataSize_p = t_res.Service.KeyWrap.Size;
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
 * val_SymAesKeyWrap
 */
ValStatus_t
val_SymAesKeyWrap(
        ValAssetId_Optional_t KeyAssetId,
        ValOctetsIn_Optional_t * const Key_p,
        const ValSize_t KeySize,
        ValOctetsIn_t * const SrcData_p,
        const ValSize_t SrcDataSize,
        ValOctetsOut_t * const DstData_p,
        ValSize_t * const DstDataSize_p)
{
#ifdef VAL_STRICT_ARGS
    if (((KeySize != (128/8)) && (KeySize != (192/8)) && (KeySize != (256/8))) ||
        ((KeyAssetId == VAL_ASSETID_INVALID) && (Key_p == NULL)) ||
        (SrcData_p == NULL) ||
        (DstDataSize_p == NULL) ||
        (SrcDataSize == 0) || (SrcDataSize > 1024))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    if ((DstData_p == NULL) ||
        (*DstDataSize_p < VAL_KEYWRAP_ENCDATA_SIZE(SrcDataSize)))
    {
        *DstDataSize_p = VAL_KEYWRAP_ENCDATA_SIZE(SrcDataSize);
        return VAL_BUFFER_TOO_SMALL;
    }

    return valLocal_AesKeyWrap(true,
                               KeyAssetId, Key_p, KeySize,
                               SrcData_p, SrcDataSize,
                               DstData_p, DstDataSize_p);
}


/*----------------------------------------------------------------------------
 * val_SymAesKeyUnwrap
 */
ValStatus_t
val_SymAesKeyUnwrap(
        ValAssetId_Optional_t KeyAssetId,
        ValOctetsIn_Optional_t * const Key_p,
        const ValSize_t KeySize,
        ValOctetsIn_t * const SrcData_p,
        const ValSize_t SrcDataSize,
        ValOctetsOut_t * const DstData_p,
        ValSize_t * const DstDataSize_p)
{
    ValSize_t OutputSize;

#ifdef VAL_STRICT_ARGS
    if (((KeySize != (128/8)) && (KeySize != (192/8)) && (KeySize != (256/8))) ||
        ((KeyAssetId == VAL_ASSETID_INVALID) && (Key_p == NULL)) ||
        (SrcData_p == NULL) ||
        (DstDataSize_p == NULL) ||
        (SrcDataSize < (VAL_KEYWRAP_IV_SIZE + VAL_SYM_ALGO_AES_BLOCK_SIZE)) ||
        (SrcDataSize != VAL_KEYWRAP_DATA_SIZE(SrcDataSize)))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    OutputSize = VAL_KEYWRAP_DECDATA_SIZE(SrcDataSize);
    if ((DstData_p == NULL) ||
        (OutputSize > *DstDataSize_p))
    {
        *DstDataSize_p = OutputSize;
        return VAL_BUFFER_TOO_SMALL;
    }
    *DstDataSize_p = OutputSize;

    return valLocal_AesKeyWrap(false,
                               KeyAssetId, Key_p, KeySize,
                               SrcData_p, SrcDataSize,
                               DstData_p, DstDataSize_p);
}


#endif /* !VAL_REMOVE_SYM_AESKEYWRAP */


/* end of file adapter_val_sym_keywrap.c */
