/* adapter_vex_asym_asset_genshared.c
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
#ifdef MODULE
#include "adapter_alloc.h"             // Adapter_Alloc(), Adapter_Free()
#include <linux/uaccess.h>             // copy_to_user, copy_from_user
#endif


/*----------------------------------------------------------------------------
 * vex_Asym_AssetGenSharedSecret
 */
VexStatus_t
vex_Asym_AssetGenSharedSecret(
        VexToken_Command_t * const CommandToken_p,
        VexToken_Result_t * const ResultToken_p)
{
    VexStatus_t funcres = VEX_NO_MEMORY;
    uint64_t OtherInfoAddr = 0;
    size_t OtherInfoSize = 0;
    Eip130Token_Command_t CommandToken;
    Eip130Token_Result_t ResultToken;

    if ((CommandToken_p->Service.PkAssetGenSharedSecret.OtherInfo_p != NULL) &&
        (CommandToken_p->Service.PkAssetGenSharedSecret.OtherInfoSize != 0))
    {
        // Get OtherInfo input address for token
        OtherInfoSize = CommandToken_p->Service.PkAssetGenSharedSecret.OtherInfoSize;
        OtherInfoAddr = BufManager_Map(
                            CommandToken_p->fFromUserSpace,
                            BUFMANAGER_BUFFERTYPE_IN,
                            CommandToken_p->Service.PkAssetGenSharedSecret.OtherInfo_p,
                            OtherInfoSize,
                            NULL);
        if (OtherInfoAddr == 0)
        {
            goto error_func_exit;
        }
    }

    // Format command token
    ZEROINIT(CommandToken);
    Eip130Token_Command_Pk_Asset_Command(
        &CommandToken,
        (uint8_t)CommandToken_p->Service.PkAssetGenSharedSecret.Method,
        (uint8_t)((CommandToken_p->Service.PkAssetGenSharedSecret.ModulusSizeInBits+31)/32),
        (uint8_t)((CommandToken_p->Service.PkAssetGenSharedSecret.DivisorSizeInBits+31)/32),
        (uint8_t)CommandToken_p->Service.PkAssetGenSharedSecret.AssetIdListSize,
        CommandToken_p->Service.PkAssetGenSharedSecret.PrivKeyAssetId,
        CommandToken_p->Service.PkAssetGenSharedSecret.DomainAssetId,
        CommandToken_p->Service.PkAssetGenSharedSecret.PubKeyAssetId,
        OtherInfoAddr, (uint16_t)OtherInfoSize,
        0, 0);

    if ((CommandToken_p->Service.PkAssetGenSharedSecret.Method == VEXTOKEN_PKASSET_DH_GEN_SHARED_SECRET_DKEYPAIR) ||
        (CommandToken_p->Service.PkAssetGenSharedSecret.Method == VEXTOKEN_PKASSET_ECDH_GEN_SHARED_SECRET_DKEYPAIR))
    {
        if ((CommandToken_p->Service.PkAssetGenSharedSecret.PubKey2AssetId == 0) ||
            (CommandToken_p->Service.PkAssetGenSharedSecret.PrivKey2AssetId == 0))
        {
            funcres = VEX_BAD_ARGUMENT;
            goto error_func_exit;
        }

        Eip130Token_Command_Pk_Asset_SetAdditionalAssetId(
            &CommandToken,
            CommandToken_p->Service.PkAssetGenSharedSecret.PrivKey2AssetId);
        Eip130Token_Command_Pk_Asset_SetAdditionalAssetId(
            &CommandToken,
            CommandToken_p->Service.PkAssetGenSharedSecret.PubKey2AssetId);
    }

#ifdef MODULE
    if (CommandToken_p->fFromUserSpace)
    {
        uint32_t i;
        uint32_t * AssetIdList_p;
        uint32_t DataSize = sizeof(uint32_t) * CommandToken_p->Service.PkAssetGenSharedSecret.AssetIdListSize;

        AssetIdList_p = (uint32_t *)Adapter_Alloc(DataSize);
        if (AssetIdList_p == NULL)
        {
            goto error_func_exit;
        }
        if (copy_from_user(AssetIdList_p,
                           CommandToken_p->Service.PkAssetGenSharedSecret.AssetIdList_p,
                           DataSize) != 0)
        {
            // Internal error
            Adapter_Free(AssetIdList_p);
            goto error_func_exit;
        }
        for (i = 0; i < CommandToken_p->Service.PkAssetGenSharedSecret.AssetIdListSize; i++)
        {
            Eip130Token_Command_Pk_Asset_SetAdditionalAssetId(&CommandToken, AssetIdList_p[i]);
        }
        Adapter_Free(AssetIdList_p);
    }
    else
#endif
    {
        uint32_t i;
        const uint32_t * AssetIdList_p = CommandToken_p->Service.PkAssetGenSharedSecret.AssetIdList_p;

        for (i = 0; i < CommandToken_p->Service.PkAssetGenSharedSecret.AssetIdListSize; i++)
        {
            Eip130Token_Command_Pk_Asset_SetAdditionalAssetId(&CommandToken, AssetIdList_p[i]);
        }
    }
    if (CommandToken_p->Service.PkAssetGenSharedSecret.fSaveSharedSecret)
    {
        Eip130Token_Command_Pk_Asset_SaveSharedSecret(&CommandToken);
    }

    Eip130Token_Command_SetTokenID(&CommandToken, vex_DeviceGetTokenID(), false);

    // Initialize result token
    ZEROINIT(ResultToken);

    // Exchange token with the EIP-13x HW
    funcres = vex_PhysicalTokenExchange(&CommandToken, &ResultToken);
    if (funcres == VEX_SUCCESS)
    {
        ResultToken_p->Result = Eip130Token_Result_Code(&ResultToken);
    }

error_func_exit:
    // Release used buffers, if needed
    if (OtherInfoAddr != 0)
    {
        (void)BufManager_Unmap(OtherInfoAddr, false);
    }

    return funcres;
}


/* end of file adapter_vex_asym_asset_genshared.c */
