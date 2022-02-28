/* adapter_vex_asym_asset_genkey.c
 *
 * Implementation of the VEX API.
 *
 * This file implements the asymmetric crypto services for key generation.
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

#include "c_adapter_vex.h"              // configuration

#include "basic_defs.h"
#include "clib.h"

#include "adapter_vex_intern_asym.h"   // API implementation
#include "adapter_bufmanager.h"        // BufManager_*()
#include "eip130_token_pk.h"           // Eip130Token_Command_Pk_Asset*()


/*----------------------------------------------------------------------------
 * vex_Asym_AssetGenKeyPair
 */
VexStatus_t
vex_Asym_AssetGenKeyPair(
        VexToken_Command_t * const CommandToken_p,
        VexToken_Result_t * const ResultToken_p)
{
    VexStatus_t funcres = VEX_NO_MEMORY;
    bool fCopy = false;
    uint64_t KeyBlobAddr = 0;
    size_t KeyBlobSize = 0;
    uint64_t PubKeyAddr = 0;
    size_t PubKeySize = 0;
    uint16_t TokenID;
    Eip130Token_Command_t CommandToken;
    Eip130Token_Result_t ResultToken;

    TokenID = vex_DeviceGetTokenID();

    if (CommandToken_p->Service.PkAssetGenKey.KekAssetId != 0)
    {
        if ((CommandToken_p->Service.PkAssetGenKey.KeyBlob_p == NULL) ||
            (CommandToken_p->Service.PkAssetGenKey.AssociatedDataSize >= (256 - 4)))
        {
            return VEX_BAD_ARGUMENT;
        }

        // Get Key output address for token
        KeyBlobAddr = BufManager_Map(
                          CommandToken_p->fFromUserSpace,
                          BUFMANAGER_BUFFERTYPE_OUT,
                          CommandToken_p->Service.PkAssetGenKey.KeyBlob_p,
                          CommandToken_p->Service.PkAssetGenKey.KeyBlobSize,
                          (void *)&TokenID);
        if (KeyBlobAddr == 0)
        {
            goto error_func_exit;
        }
        KeyBlobSize = BufManager_GetSize(KeyBlobAddr);
    }

    if (CommandToken_p->Service.PkAssetGenKey.PubKey_p != NULL)
    {
        // Get public key output address for token
        PubKeySize = CommandToken_p->Service.PkAssetGenKey.PubKeySize;
        PubKeyAddr = BufManager_Map(
                            CommandToken_p->fFromUserSpace,
                            BUFMANAGER_BUFFERTYPE_OUT,
                            CommandToken_p->Service.PkAssetGenKey.PubKey_p,
                            PubKeySize,
                            (void *)&TokenID);
        if (PubKeyAddr == 0)
        {
            goto error_func_exit;
        }
    }

    // Format command token
    ZEROINIT(CommandToken);
    Eip130Token_Command_Pk_Asset_Command(
        &CommandToken,
        (uint8_t)CommandToken_p->Service.PkAssetGenKey.Method,
        (uint8_t)((CommandToken_p->Service.PkAssetGenKey.ModulusSizeInBits+31)/32),
        (uint8_t)((CommandToken_p->Service.PkAssetGenKey.DivisorSizeInBits+31)/32),
        0,
        CommandToken_p->Service.PkAssetGenKey.PrivKeyAssetId,
        CommandToken_p->Service.PkAssetGenKey.DomainAssetId,
        CommandToken_p->Service.PkAssetGenKey.PubKeyAssetId,
        KeyBlobAddr, (uint16_t)KeyBlobSize,
        PubKeyAddr, (uint16_t)PubKeySize);
    if (KeyBlobAddr != 0)
    {
        Eip130Token_Command_Pk_Asset_SetAdditionalAssetId(
            &CommandToken,
            CommandToken_p->Service.PkAssetGenKey.KekAssetId);
        Eip130Token_Command_Pk_Asset_SetAdditionalData(
            &CommandToken,
            CommandToken_p->Service.PkAssetGenKey.AssociatedData,
            (uint8_t)CommandToken_p->Service.PkAssetGenKey.AssociatedDataSize);
        Eip130Token_Command_Pk_Asset_AddlenCorrection(&CommandToken, sizeof(uint32_t));
    }

    if ((KeyBlobAddr == 0) && (PubKeyAddr == 0))
    {
        Eip130Token_Command_SetTokenID(&CommandToken, TokenID, false);
    }
    else
    {
        Vex_Command_SetTokenID(&CommandToken, TokenID);
    }

    // Initialize result token
    ZEROINIT(ResultToken);

    // Exchange token with the EIP-13x HW
    funcres = vex_PhysicalTokenExchange(&CommandToken, &ResultToken);
    if (funcres == VEX_SUCCESS)
    {
        ResultToken_p->Result = Eip130Token_Result_Code(&ResultToken);
        if (ResultToken_p->Result >= 0)
        {
            // Copy output data
            fCopy = true;
        }
    }

error_func_exit:
    // Release used buffers, if needed
    if (KeyBlobAddr != 0)
    {
        int rc = BufManager_Unmap(KeyBlobAddr, fCopy);
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
    if (PubKeyAddr != 0)
    {
        int rc = BufManager_Unmap(PubKeyAddr, fCopy);
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
 * vex_Asym_AssetGenKeyPublic
 */
VexStatus_t
vex_Asym_AssetGenKeyPublic(
        VexToken_Command_t * const CommandToken_p,
        VexToken_Result_t * const ResultToken_p)
{
    VexStatus_t funcres;
    bool fCopy = false;
    uint64_t PubKeyAddr = 0;
    size_t PubKeySize = 0;
    uint16_t TokenID;
    Eip130Token_Command_t CommandToken;
    Eip130Token_Result_t ResultToken;

    TokenID = vex_DeviceGetTokenID();

    if (CommandToken_p->Service.PkAssetGenKey.PubKey_p != NULL)
    {
        // Get public key output address for token
        PubKeyAddr = BufManager_Map(
                            CommandToken_p->fFromUserSpace,
                            BUFMANAGER_BUFFERTYPE_OUT,
                            CommandToken_p->Service.PkAssetGenKey.PubKey_p,
                            CommandToken_p->Service.PkAssetGenKey.PubKeySize,
                            (void *)&TokenID);
        if (PubKeyAddr == 0)
        {
            return VEX_NO_MEMORY;
        }
        PubKeySize = CommandToken_p->Service.PkAssetGenKey.PubKeySize;
    }

    // Format command token
    ZEROINIT(CommandToken);
    Eip130Token_Command_Pk_Asset_Command(
        &CommandToken,
        (uint8_t)CommandToken_p->Service.PkAssetGenKey.Method,
        (uint8_t)((CommandToken_p->Service.PkAssetGenKey.ModulusSizeInBits+31)/32),
        (uint8_t)((CommandToken_p->Service.PkAssetGenKey.DivisorSizeInBits+31)/32),
        0,
        CommandToken_p->Service.PkAssetGenKey.PrivKeyAssetId,
        CommandToken_p->Service.PkAssetGenKey.DomainAssetId,
        CommandToken_p->Service.PkAssetGenKey.PubKeyAssetId,
        0, 0,
        PubKeyAddr, (uint16_t)PubKeySize);
    if (PubKeyAddr == 0)
    {
        Eip130Token_Command_SetTokenID(&CommandToken, TokenID, false);
    }
    else
    {
        Vex_Command_SetTokenID(&CommandToken, TokenID);
    }

    // Initialize result token
    ZEROINIT(ResultToken);

    // Exchange token with the EIP-13x HW
    funcres = vex_PhysicalTokenExchange(&CommandToken, &ResultToken);
    if (funcres == VEX_SUCCESS)
    {
        ResultToken_p->Result = Eip130Token_Result_Code(&ResultToken);
        if (ResultToken_p->Result >= 0)
        {
            // Copy output data
            fCopy = true;
        }
    }

    // Release used buffer, if needed
    if (PubKeyAddr != 0)
    {
        int rc = BufManager_Unmap(PubKeyAddr, fCopy);
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


/* end of file adapter_vex_asym_asset_genkey.c */
