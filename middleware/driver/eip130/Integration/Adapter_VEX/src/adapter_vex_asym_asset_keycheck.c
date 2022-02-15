/* adapter_vex_asym_asset_keycheck.c
 *
 * Implementation of the VEX API.
 *
 * This file implements the asymmetric crypto services for key checking.
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
 * vex_Asym_AssetKeyCheck
 */
VexStatus_t
vex_Asym_AssetKeyCheck(
        VexToken_Command_t * const CommandToken_p,
        VexToken_Result_t * const ResultToken_p)
{
    VexStatus_t funcres;
    Eip130Token_Command_t CommandToken;
    Eip130Token_Result_t ResultToken;

    // Format command token
    ZEROINIT(CommandToken);
    Eip130Token_Command_Pk_Asset_Command(
        &CommandToken,
        (uint8_t)CommandToken_p->Service.PkAssetKeyCheck.Method,
        (uint8_t)((CommandToken_p->Service.PkAssetKeyCheck.ModulusSizeInBits+31)/32),
        (uint8_t)((CommandToken_p->Service.PkAssetKeyCheck.DivisorSizeInBits+31)/32),
        0,
        CommandToken_p->Service.PkAssetKeyCheck.PubKeyAssetId,
        CommandToken_p->Service.PkAssetKeyCheck.DomainAssetId,
        CommandToken_p->Service.PkAssetKeyCheck.PrivKeyAssetId,
        0, 0,
        0, 0);
    Eip130Token_Command_SetTokenID(&CommandToken, vex_DeviceGetTokenID(), false);

    // Initialize result token
    ZEROINIT(ResultToken);

    // Exchange token with the EIP-13x HW
    funcres = vex_PhysicalTokenExchange(&CommandToken, &ResultToken);
    if (funcres == VEX_SUCCESS)
    {
        ResultToken_p->Result = Eip130Token_Result_Code(&ResultToken);
    }

    return funcres;
}


/* end of file adapter_vex_asym_asset_keycheck.c */
