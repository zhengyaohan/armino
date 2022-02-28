/* adapter_vex_extservice.c
 *
 * Implementation of the VEX API.
 *
 * This file implements the external service services.
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

#include "c_adapter_vex.h"          // configuration

#include "basic_defs.h"
#include "clib.h"

#include "adapter_vex_internal.h"   // API implementation
#include "adapter_bufmanager.h"     // BufManager_*()
#include "eip130_token_extservice.h" // Eip130Token_Command/Result_ExtService*


/*----------------------------------------------------------------------------
 * vex_ExtService
 */
VexStatus_t
vex_ExtService(
        VexToken_Command_t * const CommandToken_p,
        VexToken_Result_t * const ResultToken_p)
{
    VexStatus_t funcres;
    uint64_t InputDataAddr = 0;
    Eip130Token_Command_t CommandToken;
    Eip130Token_Result_t ResultToken;

    // Convert input buffer and get address for token
    InputDataAddr = BufManager_Map(CommandToken_p->fFromUserSpace,
                                   BUFMANAGER_BUFFERTYPE_IN,
                                   CommandToken_p->Service.ExtService.Data_p,
                                   CommandToken_p->Service.ExtService.DataSize,
                                   NULL);
    if (InputDataAddr == 0)
    {
        return VEX_NO_MEMORY;
    }

    // Format command token
    ZEROINIT(CommandToken);
    Eip130Token_Command_ExtService(
        &CommandToken,
        (uint8_t)CommandToken_p->Service.ExtService.Algorithm,
        InputDataAddr,
        CommandToken_p->Service.ExtService.DataSize);
    Eip130Token_Command_ExtService_SetKeyAssetIDAndKeyLength(
        &CommandToken,
        CommandToken_p->Service.ExtService.KeyAssetId,
        CommandToken_p->Service.ExtService.KeySize);
    Eip130Token_Command_ExtService_SetStateAssetID(
        &CommandToken,
        CommandToken_p->Service.ExtService.StateAssetId);
    Eip130Token_Command_ExtService_SetAssociatedData(
        &CommandToken,
        CommandToken_p->Service.ExtService.AssociatedData,
        CommandToken_p->Service.ExtService.AssociatedDataSize);
    Eip130Token_Command_SetTokenID(&CommandToken, vex_DeviceGetTokenID(), false);

    // Initialize result token
    ZEROINIT(ResultToken);

    // Exchange token with the EIP-13x HW
    funcres = vex_PhysicalTokenExchange(&CommandToken, &ResultToken);
    if (funcres == VEX_SUCCESS)
    {
        ResultToken_p->Result = Eip130Token_Result_Code(&ResultToken);
        if (ResultToken_p->Result >= 0)
        {
            // Success
            Eip130Token_Result_ExtService_CopyMAC(
                &ResultToken,
                32,
                ResultToken_p->Service.ExtService.Mac);
        }
    }

    // Release used buffers, if needed
    if (InputDataAddr != 0)
    {
        (void)BufManager_Unmap(InputDataAddr, false);
    }

    return funcres;
}

/* end of file adapter_vex_extservice.c */
