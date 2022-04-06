/* adapter_vex_nop.c
 *
 * Implementation of the VEX API.
 *
 * This file implements the NOP crypto aka Data Copy.
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

#include "adapter_vex_internal.h"   // API implementation
#include "adapter_bufmanager.h"     // BufManager_*()
#include "eip130_token_nop.h"       // Eip130Token_Command_Nop()


/*----------------------------------------------------------------------------
 * vex_Nop
 */
VexStatus_t
vex_Nop(
        VexToken_Command_t * const CommandToken_p,
        VexToken_Result_t * const ResultToken_p)
{
    VexStatus_t funcres = VEX_NO_MEMORY;
    uint16_t TokenID;
    uint64_t SrcDataAddr = 0;
    uint64_t DstDataAddr = 0;
    bool fCopy = false;
    Eip130Token_Command_t CommandToken;
    Eip130Token_Result_t ResultToken;

    // Convert input buffer and get address for token
    SrcDataAddr = BufManager_Map(CommandToken_p->fFromUserSpace,
                                 BUFMANAGER_BUFFERTYPE_IN,
                                 CommandToken_p->Service.Nop.In_p,
                                 CommandToken_p->Service.Nop.Size,
                                 NULL);
    if (SrcDataAddr == 0)
    {
        goto error_func_exit;
    }

    // Get output address for token
    TokenID = vex_DeviceGetTokenID();
    DstDataAddr = BufManager_Map(CommandToken_p->fFromUserSpace,
                                 BUFMANAGER_BUFFERTYPE_OUT,
                                 CommandToken_p->Service.Nop.Out_p,
                                 CommandToken_p->Service.Nop.Size,
                                 (void *)&TokenID);
    if (DstDataAddr == 0)
    {
        goto error_func_exit;
    }

    // Format command token
    ZEROINIT(CommandToken);
    Eip130Token_Command_Nop(&CommandToken,
                            SrcDataAddr, CommandToken_p->Service.Nop.Size,
                            DstDataAddr, (uint32_t)BufManager_GetSize(DstDataAddr));
    Vex_Command_SetTokenID(&CommandToken, TokenID);

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
    if (SrcDataAddr != 0)
    {
        (void)BufManager_Unmap(SrcDataAddr, false);
    }
    if (DstDataAddr != 0)
    {
        int rc = BufManager_Unmap(DstDataAddr, fCopy);
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


/* end of file adapter_vex_nop.c */
