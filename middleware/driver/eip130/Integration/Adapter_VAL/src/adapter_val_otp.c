/* adapter_val_otp.c
 *
 * Implementation of the VAL API.
 *
 * This file implements the OTP Data Write services.
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
 * val_OTPDataWrite
 */
#ifndef VAL_REMOVE_OTPDATAWRITE
ValStatus_t
val_OTPDataWrite(
        const ValAssetNumber_t StaticAssetNumber,
        const ValAssetNumber_t AssetPolicyNumber,
        const bool fAddCRC,
        ValOctetsIn_t * const AssociatedData_p,
        const ValSize_t AssociatedDataSize,
        ValOctetsIn_t * const KeyBlob_p,
        const ValSize_t KeyBlobSize)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;

#ifdef VAL_STRICT_ARGS
    if ((StaticAssetNumber > VAL_ASSET_NUMBER_MAX) ||
        (AssetPolicyNumber > VAL_POLICY_NUMBER_MAX) ||
        (AssociatedData_p == NULL) ||
        (AssociatedDataSize < VAL_KEYBLOB_AAD_MIN_SIZE) ||
        (AssociatedDataSize > VAL_KEYBLOB_AAD_MAX_SIZE) ||
        (KeyBlob_p == NULL) ||
        (KeyBlobSize == 0) ||
        (KeyBlobSize > VAL_OTP_KEYBLOB_SIZE(3072/8)))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_ASSETMANAGEMENT;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_OTPDATAWRITE;
    t_cmd.Service.OTPDataWrite.AssetNumber = (uint16_t)StaticAssetNumber;
    t_cmd.Service.OTPDataWrite.PolicyNumber = (uint16_t)AssetPolicyNumber;
    t_cmd.Service.OTPDataWrite.fAddCRC = fAddCRC;
    memcpy(t_cmd.Service.OTPDataWrite.AssociatedData, AssociatedData_p, AssociatedDataSize);
    t_cmd.Service.OTPDataWrite.AssociatedDataSize = (uint32_t)AssociatedDataSize;
    t_cmd.Service.OTPDataWrite.KeyBlob_p = (const uint8_t *)KeyBlob_p;
    t_cmd.Service.OTPDataWrite.KeyBlobSize = (uint32_t)KeyBlobSize;
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
#endif /* !VAL_REMOVE_OTPDATAWRITE */


/*----------------------------------------------------------------------------
 * val_ProvisionRandomRootKey
 */
#ifndef VAL_REMOVE_PROVISIONRANDOMROOTKEY
ValStatus_t
val_ProvisionRandomRootKey(
        const uint32_t Identity,
        const ValAssetNumber_t StaticAssetNumber,
        const bool f128bit,
        const bool fAddCRC,
        const uint8_t  AutoSeed,
        const uint16_t SampleCycles,
        const uint8_t  SampleDiv,
        const uint8_t  NoiseBlocks,
        ValOctetsIn_t * const AssociatedData_p,
        const ValSize_t AssociatedDataSize,
        ValOctetsOut_t * const KeyBlob_p,
        ValSize_t * const KeyBlobSize_p)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;

#ifdef VAL_STRICT_ARGS
    if ((Identity == 0) ||
        (StaticAssetNumber > 30))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_ASSETMANAGEMENT;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_PROVISIONRANDOMHUK;
    t_cmd.Service.ProvisionRandomHUK.Identity = Identity;
    t_cmd.Service.ProvisionRandomHUK.AssetNumber = (uint16_t)StaticAssetNumber;
    t_cmd.Service.ProvisionRandomHUK.f128bit = f128bit;
    t_cmd.Service.ProvisionRandomHUK.fAddCRC = fAddCRC;
    t_cmd.Service.ProvisionRandomHUK.AutoSeed = AutoSeed;
    t_cmd.Service.ProvisionRandomHUK.SampleCycles = SampleCycles;
    t_cmd.Service.ProvisionRandomHUK.SampleDiv = SampleDiv;
    t_cmd.Service.ProvisionRandomHUK.NoiseBlocks = NoiseBlocks;
    if ((KeyBlobSize_p != NULL) && (*KeyBlobSize_p > 0))
    {
#ifdef VAL_STRICT_ARGS
        if (f128bit)
        {
            if (*KeyBlobSize_p < VAL_OTP_KEYBLOB_SIZE(128/8))
            {
                return VAL_INVALID_LENGTH;
            }
        }
        else if (*KeyBlobSize_p < VAL_OTP_KEYBLOB_SIZE(256/8))
        {
            return VAL_INVALID_LENGTH;
        }

        if ((AssociatedData_p == NULL) ||
            (AssociatedDataSize < VAL_KEYBLOB_AAD_MIN_SIZE) ||
            (AssociatedDataSize > VAL_KEYBLOB_AAD_MAX_SIZE))
        {
            return VAL_BAD_ARGUMENT;
        }
#endif

        t_cmd.Service.ProvisionRandomHUK.KeyBlob_p = (const uint8_t *)KeyBlob_p;
        t_cmd.Service.ProvisionRandomHUK.KeyBlobSize = (uint32_t)*KeyBlobSize_p;
        t_cmd.Service.ProvisionRandomHUK.AssociatedDataSize = (uint8_t)AssociatedDataSize;
        memcpy(t_cmd.Service.ProvisionRandomHUK.AssociatedData, AssociatedData_p, AssociatedDataSize);
    }
    else
    {
        t_cmd.Service.ProvisionRandomHUK.KeyBlob_p = NULL;
        t_cmd.Service.ProvisionRandomHUK.KeyBlobSize = 0;
        t_cmd.Service.ProvisionRandomHUK.AssociatedDataSize = 0;
    }
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;
    t_res.Service.ProvisionRandomHUK.KeyBlobSize = 0;

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if (funcres == VAL_SUCCESS)
    {
        // Check for errors
        if (t_res.Result == VEXTOKEN_RESULT_NO_ERROR)
        {
            if ((t_cmd.Service.ProvisionRandomHUK.KeyBlobSize != 0) &&
                (KeyBlobSize_p != NULL))
            {
                *KeyBlobSize_p = t_res.Service.ProvisionRandomHUK.KeyBlobSize;
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
#endif /* !VAL_REMOVE_PROVISIONRANDOMROOTKEY */


/* end of file adapter_val_otp.c */
