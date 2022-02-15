/* adapter_vex_asset.c
 *
 * Implementation of the VEX API.
 *
 * This file implements the Asset Store services.
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

#include "c_adapter_vex.h"          // configuration

#include "basic_defs.h"
#include "clib.h"
#include "log.h"

#include "adapter_vex_internal.h"   // API implementation
#include "adapter_bufmanager.h"     // BufManager_*()
#include "eip130_token_asset.h"     // Eip130Token_Command/Result_Asset*()
#include "eip130_token_timer.h"     // Eip130Token_Command/Result_SecureTimer*()
#include "eip130_token_publicdata.h" // Eip130Token_Command/Result_PublicData_Read()
#include "eip130_token_otp.h"       // Eip130Token_Command/Result_OTP*()


/*----------------------------------------------------------------------------
 * vex_IsStaticAsset
 *
 * Determines type of Asset based on its AssetId.
 *
 * AssetId
 *     Reference to an Asset.
 *
 * Return Value:
 *     true  : Static Asset   (Asset resides in EIP-13x OTP)
 *     false : Dynamic Asset  (Asset resides in EIP-13x internal memory)
 */
static bool
vexLocal_IsStaticAsset(uint32_t AssetId)
{
    return (((AssetId & 0xFF03FF03) ^ 0x5A02A501) == 0);
}


/*----------------------------------------------------------------------------
 * vexLocal_AssetLoadCommon
 */
static VexStatus_t
vexLocal_AssetLoadCommon(
        VexToken_Command_t * const LogicalCommandToken_p,
        VexToken_Result_t * const LogicalResultToken_p,
        Eip130Token_Command_t * const CommandToken_p,
        Eip130Token_Result_t * const ResultToken_p,
        uint16_t * TokenID_p,
        const uint8_t * Input_p,
        uint32_t InputSize,
        const uint8_t * AddInput_p,
        uint32_t AddInputSize,
        uint8_t * Output_p,
        uint32_t OutputSize)
{
    VexStatus_t funcres = VEX_NO_MEMORY;
    uint64_t InputDataAddr = 0;
    uint64_t AddInputDataAddr = 0;
    uint64_t OutputDataAddr = 0;
    bool fCopy = false;

    if (Input_p != NULL)
    {
        // Get input address for token
        InputDataAddr = BufManager_Map(LogicalCommandToken_p->fFromUserSpace,
                                       BUFMANAGER_BUFFERTYPE_IN,
                                       Input_p, InputSize,
                                       NULL);
        if (InputDataAddr == 0)
        {
            goto error_func_exit;
        }

        Eip130Token_Command_AssetLoad_SetInput(CommandToken_p,
                                               InputDataAddr, InputSize);
    }

    if (Output_p != NULL)
    {
        // Get output address for token
        OutputDataAddr = BufManager_Map(LogicalCommandToken_p->fFromUserSpace,
                                        BUFMANAGER_BUFFERTYPE_OUT,
                                        Output_p, OutputSize,
                                        (void *)TokenID_p);
        if (OutputDataAddr == 0)
        {
            goto error_func_exit;
        }

        Eip130Token_Command_AssetLoad_SetOutput(CommandToken_p,
                                                OutputDataAddr,
                                                (uint32_t)BufManager_GetSize(OutputDataAddr));
        Vex_Command_SetTokenID(CommandToken_p, *TokenID_p);
    }
    else
    {
        if (AddInput_p != NULL)
        {
            // Get input address for token
            AddInputDataAddr = BufManager_Map(
                                   LogicalCommandToken_p->fFromUserSpace,
                                   BUFMANAGER_BUFFERTYPE_IN,
                                   AddInput_p, AddInputSize,
                                   NULL);
            if (AddInputDataAddr == 0)
            {
                goto error_func_exit;
            }

            Eip130Token_Command_AssetLoad_SetOutput(CommandToken_p,
                                                    AddInputDataAddr,
                                                    AddInputSize);
        }

        Eip130Token_Command_SetTokenID(CommandToken_p, *TokenID_p, false);
    }

    // Exchange token with the EIP-13x HW
    funcres = vex_PhysicalTokenExchange(CommandToken_p, ResultToken_p);
    if (funcres == VEX_SUCCESS)
    {
        LogicalResultToken_p->Result = Eip130Token_Result_Code(ResultToken_p);
        if ((LogicalResultToken_p->Result >= 0) && (OutputDataAddr != 0))
        {
            fCopy = true;
            Eip130Token_Result_AssetLoad_OutputSize(
                ResultToken_p,
                &LogicalResultToken_p->Service.AssetLoad.KeyBlobSize);
        }
    }

error_func_exit:
    // Release used buffers, if needed
    if (InputDataAddr != 0)
    {
        (void)BufManager_Unmap(InputDataAddr, false);
    }
    if (AddInputDataAddr != 0)
    {
        (void)BufManager_Unmap(AddInputDataAddr, false);
    }
    if (OutputDataAddr != 0)
    {
        int rc = BufManager_Unmap(OutputDataAddr, fCopy);
        if (rc != 0)
        {
            if (rc == -3)
            {
                funcres = VEX_DATA_TIMEOUT;
            }
            else
            {
                funcres = VEX_INTERNAL_ERROR;
            }
        }
    }

    return funcres;
}


/*----------------------------------------------------------------------------
 * vex_Asset
 */
VexStatus_t
vex_Asset(
        VexToken_Command_t * const CommandToken_p,
        VexToken_Result_t * const ResultToken_p)
{
    uint16_t TokenID = vex_DeviceGetTokenID();
    VexStatus_t funcres;
    Eip130Token_Command_t CommandToken;
    Eip130Token_Result_t ResultToken;
    uint64_t InputDataAddr = 0;
    uint64_t OutputDataAddr = 0;
    uint16_t OutputDataSize = 0;
    bool fCopy = false;

    // Initialize result token
    ZEROINIT(ResultToken);

    // Format command token
    ZEROINIT(CommandToken);
    switch (CommandToken_p->SubCode)
    {
    case VEXTOKEN_SUBCODE_ASSETSEARCH:
        Eip130Token_Command_AssetSearch(
            &CommandToken,
            CommandToken_p->Service.AssetSearch.Number);
        break;

    case VEXTOKEN_SUBCODE_ASSETCREATE:
        Eip130Token_Command_AssetCreate(
            &CommandToken,
            CommandToken_p->Service.AssetCreate.Policy,
            CommandToken_p->Service.AssetCreate.Size);
        if (CommandToken_p->Service.AssetCreate.fAllHosts)
        {
            Eip130Token_Command_AssetCreate_SetAllHosts(&CommandToken);
        }
        if (CommandToken_p->Service.AssetCreate.fRemoveSecure)
        {
            Eip130Token_Command_AssetCreate_SetRemoveSecure(&CommandToken);
        }
        if (CommandToken_p->Service.AssetCreate.LifetimeUse != 0)
        {
            Eip130Token_Command_AssetCreate_SetLifetime(
                &CommandToken,
                CommandToken_p->Service.AssetCreate.LifetimeUse,
                CommandToken_p->Service.AssetCreate.fLifetimeRelative,
                CommandToken_p->Service.AssetCreate.fLifetimeNoLoad,
                CommandToken_p->Service.AssetCreate.Lifetime);
        }
        break;

    case VEXTOKEN_SUBCODE_ASSETLOAD:
        switch(CommandToken_p->Service.AssetLoadPlaintext.Method)
        {
        case VEXTOKEN_ASSETLOAD_IMPORT:
            Eip130Token_Command_AssetLoad_Import(
                &CommandToken,
                CommandToken_p->Service.AssetLoadImport.AssetId,
                CommandToken_p->Service.AssetLoadImport.KekAssetId);
            Eip130Token_Command_AssetLoad_SetAad(
                &CommandToken,
                CommandToken_p->Service.AssetLoadImport.AssociatedData,
                CommandToken_p->Service.AssetLoadImport.AssociatedDataSize);
            funcres = vexLocal_AssetLoadCommon(
                          CommandToken_p, ResultToken_p,
                          &CommandToken, &ResultToken, &TokenID,
                          CommandToken_p->Service.AssetLoadImport.KeyBlob_p,
                          CommandToken_p->Service.AssetLoadImport.KeyBlobSize,
                          NULL, 0, NULL, 0);
            break;

        case VEXTOKEN_ASSETLOAD_DERIVE:
            Eip130Token_Command_AssetLoad_Derive(
                &CommandToken,
                CommandToken_p->Service.AssetLoadDerive.AssetId,
                CommandToken_p->Service.AssetLoadDerive.KdkAssetId,
                CommandToken_p->Service.AssetLoadDerive.fCounter,
                CommandToken_p->Service.AssetLoadDerive.fRFC5869);
            Eip130Token_Command_AssetLoad_SetAad(
                &CommandToken,
                CommandToken_p->Service.AssetLoadDerive.AssociatedData,
                CommandToken_p->Service.AssetLoadDerive.AssociatedDataSize);
            funcres = vexLocal_AssetLoadCommon(
                          CommandToken_p, ResultToken_p,
                          &CommandToken, &ResultToken, &TokenID,
                          CommandToken_p->Service.AssetLoadDerive.Salt_p,
                          CommandToken_p->Service.AssetLoadDerive.SaltSize,
                          CommandToken_p->Service.AssetLoadDerive.IV_p,
                          CommandToken_p->Service.AssetLoadDerive.IVSize,
                          NULL, 0);
            break;

        case VEXTOKEN_ASSETLOAD_PLAINTEXT:
            Eip130Token_Command_AssetLoad_Plaintext(
                &CommandToken,
                CommandToken_p->Service.AssetLoadPlaintext.AssetId);
            if (CommandToken_p->Service.AssetLoadPlaintext.KekAssetId)
            {
                Eip130Token_Command_AssetLoad_Export(
                    &CommandToken,
                    CommandToken_p->Service.AssetLoadPlaintext.KekAssetId);
                Eip130Token_Command_AssetLoad_SetAad(
                    &CommandToken,
                    CommandToken_p->Service.AssetLoadPlaintext.AssociatedData,
                    CommandToken_p->Service.AssetLoadPlaintext.AssociatedDataSize);
                funcres = vexLocal_AssetLoadCommon(
                              CommandToken_p, ResultToken_p,
                              &CommandToken, &ResultToken, &TokenID,
                              CommandToken_p->Service.AssetLoadPlaintext.Data_p,
                              CommandToken_p->Service.AssetLoadPlaintext.DataSize,
                              NULL, 0,
                              CommandToken_p->Service.AssetLoadPlaintext.KeyBlob_p,
                              CommandToken_p->Service.AssetLoadPlaintext.KeyBlobSize);
            }
            else
            {
                funcres = vexLocal_AssetLoadCommon(
                              CommandToken_p, ResultToken_p,
                              &CommandToken, &ResultToken, &TokenID,
                              CommandToken_p->Service.AssetLoadPlaintext.Data_p,
                              CommandToken_p->Service.AssetLoadPlaintext.DataSize,
                              NULL, 0, NULL, 0);
            }
            break;

        case VEXTOKEN_ASSETLOAD_RANDOM:
            Eip130Token_Command_AssetLoad_Random(
                &CommandToken,
                CommandToken_p->Service.AssetLoadRandom.AssetId);
            if (CommandToken_p->Service.AssetLoadRandom.KekAssetId)
            {
                Eip130Token_Command_AssetLoad_Export(
                    &CommandToken,
                    CommandToken_p->Service.AssetLoadRandom.KekAssetId);
                Eip130Token_Command_AssetLoad_SetAad(
                    &CommandToken,
                    CommandToken_p->Service.AssetLoadRandom.AssociatedData,
                    CommandToken_p->Service.AssetLoadRandom.AssociatedDataSize);
                funcres = vexLocal_AssetLoadCommon(
                              CommandToken_p, ResultToken_p,
                              &CommandToken, &ResultToken, &TokenID,
                              NULL, 0, NULL, 0,
                              CommandToken_p->Service.AssetLoadRandom.KeyBlob_p,
                              CommandToken_p->Service.AssetLoadRandom.KeyBlobSize);
            }
            else
            {
                funcres = vexLocal_AssetLoadCommon(
                              CommandToken_p, ResultToken_p,
                              &CommandToken, &ResultToken, &TokenID,
                              NULL, 0, NULL, 0, NULL, 0);
            }
            break;

        case VEXTOKEN_ASSETLOAD_AESUNWRAP:
            Eip130Token_Command_AssetLoad_AesUnwrap(
                &CommandToken,
                CommandToken_p->Service.AssetLoadAesunwrap.AssetId,
                CommandToken_p->Service.AssetLoadAesunwrap.KekAssetId);
            funcres = vexLocal_AssetLoadCommon(
                          CommandToken_p, ResultToken_p,
                          &CommandToken, &ResultToken, &TokenID,
                          CommandToken_p->Service.AssetLoadAesunwrap.KeyBlob_p,
                          CommandToken_p->Service.AssetLoadAesunwrap.KeyBlobSize,
                          NULL, 0, NULL, 0);
            break;

        default:
            return VEX_UNSUPPORTED;
        }
        return funcres;

    case VEXTOKEN_SUBCODE_ASSETDELETE:
        Eip130Token_Command_AssetDelete(
            &CommandToken,
            CommandToken_p->Service.AssetDelete.AssetId);
        break;

    case VEXTOKEN_SUBCODE_SECURETIMER:
        Eip130Token_Command_SecureTimer(
            &CommandToken,
            CommandToken_p->Service.SecureTimer.AssetId,
            CommandToken_p->Service.SecureTimer.fSecond,
            (uint16_t)CommandToken_p->Service.SecureTimer.Operation);
        break;

    case VEXTOKEN_SUBCODE_PUBLICDATA:
        // Get output address for token
        OutputDataAddr = BufManager_Map(CommandToken_p->fFromUserSpace,
                                        BUFMANAGER_BUFFERTYPE_OUT,
                                        CommandToken_p->Service.PublicData.Data_p,
                                        CommandToken_p->Service.PublicData.DataSize,
                                        &TokenID);
        if (OutputDataAddr == 0)
        {
            return VEX_NO_MEMORY;
        }

        Eip130Token_Command_PublicData_Read(
            &CommandToken,
            CommandToken_p->Service.PublicData.AssetId,
            OutputDataAddr, (uint32_t)BufManager_GetSize(OutputDataAddr));
        break;

    case VEXTOKEN_SUBCODE_MONOTONICREAD:
        if (vexLocal_IsStaticAsset(CommandToken_p->Service.MonotonicCounter.AssetId))
        {
            // Get output address for token
            OutputDataAddr = BufManager_Map(CommandToken_p->fFromUserSpace,
                                            BUFMANAGER_BUFFERTYPE_OUT,
                                            CommandToken_p->Service.MonotonicCounter.Data_p,
                                            CommandToken_p->Service.MonotonicCounter.DataSize,
                                            &TokenID);
            if (OutputDataAddr == 0)
            {
                return VEX_NO_MEMORY;
            }

            Eip130Token_Command_OTPMonotonicCounter_Read(
                &CommandToken,
                CommandToken_p->Service.MonotonicCounter.AssetId,
                OutputDataAddr, (uint32_t)BufManager_GetSize(OutputDataAddr));
        }
        else
        {
            return VEX_UNSUPPORTED;
        }
        break;

    case VEXTOKEN_SUBCODE_MONOTONICINCR:
        if (vexLocal_IsStaticAsset(CommandToken_p->Service.MonotonicCounter.AssetId))
        {
            Eip130Token_Command_OTPMonotonicCounter_Increment(
                &CommandToken,
                CommandToken_p->Service.MonotonicCounter.AssetId);
        }
        else
        {
            return VEX_UNSUPPORTED;
        }
        break;

    case VEXTOKEN_SUBCODE_OTPDATAWRITE:
        // Get input address for token
        InputDataAddr = BufManager_Map(CommandToken_p->fFromUserSpace,
                                       BUFMANAGER_BUFFERTYPE_IN,
                                       CommandToken_p->Service.OTPDataWrite.KeyBlob_p,
                                       CommandToken_p->Service.OTPDataWrite.KeyBlobSize,
                                       NULL);
        if (InputDataAddr == 0)
        {
            return VEX_NO_MEMORY;
        }

        Eip130Token_Command_OTPDataWrite(
            &CommandToken,
            CommandToken_p->Service.OTPDataWrite.AssetNumber,
            CommandToken_p->Service.OTPDataWrite.PolicyNumber,
            CommandToken_p->Service.OTPDataWrite.fAddCRC,
            InputDataAddr,
            (uint16_t)CommandToken_p->Service.OTPDataWrite.KeyBlobSize,
            CommandToken_p->Service.OTPDataWrite.AssociatedData,
            CommandToken_p->Service.OTPDataWrite.AssociatedDataSize);
        break;

    case VEXTOKEN_SUBCODE_PROVISIONRANDOMHUK:
        if (CommandToken_p->Service.ProvisionRandomHUK.KeyBlobSize != 0)
        {
            // Get output address for token
            OutputDataAddr = BufManager_Map(
                                 CommandToken_p->fFromUserSpace,
                                 BUFMANAGER_BUFFERTYPE_OUT,
                                 CommandToken_p->Service.ProvisionRandomHUK.KeyBlob_p,
                                 CommandToken_p->Service.ProvisionRandomHUK.KeyBlobSize,
                                 &TokenID);
            if (OutputDataAddr == 0)
            {
                return VEX_NO_MEMORY;
            }
            OutputDataSize = (uint16_t)BufManager_GetSize(OutputDataAddr);
        }

        Eip130Token_Command_ProvisionRandomHUK(
            &CommandToken,
            CommandToken_p->Service.ProvisionRandomHUK.AssetNumber,
            CommandToken_p->Service.ProvisionRandomHUK.f128bit,
            CommandToken_p->Service.ProvisionRandomHUK.fAddCRC,
            CommandToken_p->Service.ProvisionRandomHUK.AutoSeed,
            CommandToken_p->Service.ProvisionRandomHUK.SampleCycles,
            CommandToken_p->Service.ProvisionRandomHUK.SampleDiv,
            CommandToken_p->Service.ProvisionRandomHUK.NoiseBlocks,
            OutputDataAddr,
            OutputDataSize,
            CommandToken_p->Service.ProvisionRandomHUK.AssociatedData,
            CommandToken_p->Service.ProvisionRandomHUK.AssociatedDataSize);
        Eip130Token_Command_Identity(
            &CommandToken,
            CommandToken_p->Service.ProvisionRandomHUK.Identity);
        break;

    default:
        return VEX_UNSUPPORTED;
    }

    if (OutputDataAddr != 0)
    {
        Vex_Command_SetTokenID(&CommandToken, TokenID);
    }
    else
    {
        Eip130Token_Command_SetTokenID(&CommandToken, TokenID, false);
    }

    // Exchange token with the EIP-13x HW
    funcres = vex_PhysicalTokenExchange(&CommandToken, &ResultToken);
    if (funcres == VEX_SUCCESS)
    {
        ResultToken_p->Result = Eip130Token_Result_Code(&ResultToken);
        if (ResultToken_p->Result >= 0)
        {
            // Success
            switch (CommandToken_p->SubCode)
            {
            case VEXTOKEN_SUBCODE_ASSETSEARCH:
                Eip130Token_Result_AssetSearch(
                    &ResultToken,
                    &ResultToken_p->Service.AssetSearch.AssetId,
                    &ResultToken_p->Service.AssetSearch.Size);
                break;

            case VEXTOKEN_SUBCODE_ASSETCREATE:
                Eip130Token_Result_AssetCreate(
                    &ResultToken,
                    &ResultToken_p->Service.AssetCreate.AssetId);
                break;

            case VEXTOKEN_SUBCODE_SECURETIMER:
                if (CommandToken_p->Service.SecureTimer.Operation ==
                    VEXTOKEN_SECURETIMER_START)
                {
                    Eip130Token_Result_SecureTimer(
                        &ResultToken,
                        &ResultToken_p->Service.SecureTimer.AssetId,
                        NULL);
                }
                else
                {
                    Eip130Token_Result_SecureTimer(
                        &ResultToken,
                        NULL,
                        &ResultToken_p->Service.SecureTimer.ElapsedTime);
                }
                break;

            case VEXTOKEN_SUBCODE_PUBLICDATA:
                fCopy = true;
                Eip130Token_Result_Publicdata_Read(
                    &ResultToken,
                    &ResultToken_p->Service.PublicData.DataSize);
                break;

            case VEXTOKEN_SUBCODE_MONOTONICREAD:
                fCopy = true;
                Eip130Token_Result_OTPMonotonicCounter_Read(
                    &ResultToken,
                    &ResultToken_p->Service.MonotonicCounter.DataSize);
                break;

            case VEXTOKEN_SUBCODE_PROVISIONRANDOMHUK:
                if (OutputDataAddr != 0)
                {
                    fCopy = true;
                    Eip130Token_Result_ProvisionRandomHUK(
                        &ResultToken,
                        &ResultToken_p->Service.ProvisionRandomHUK.KeyBlobSize);
                }
                else
                {
                    ResultToken_p->Service.ProvisionRandomHUK.KeyBlobSize = 0;
                }
                break;

            default:
                // Default means no post processing required
                break;
            }
        }
    }

    if (InputDataAddr != 0)
    {
        (void)BufManager_Unmap(InputDataAddr, false);
    }

    if (OutputDataAddr != 0)
    {
        int rc = BufManager_Unmap(OutputDataAddr, fCopy);
        if (rc != 0)
        {
            if (rc == -3)
            {
                funcres = VEX_DATA_TIMEOUT;
            }
            else
            {
                funcres = VEX_INTERNAL_ERROR;
            }
        }
    }

    return funcres;
}


/* end of file adapter_vex_asset.c */
