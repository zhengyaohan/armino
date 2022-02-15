/* adapter_vex_asym_pka.c
 *
 * Implementation of the VEX API.
 *
 * This file implements the asymmetric crypto services for direct PKA/PKCP use.
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
 * vex_Asym_PkaNumSet
 */
VexStatus_t
vex_Asym_PkaNumSet(
        VexToken_Command_t * const CommandToken_p,
        VexToken_Result_t * const ResultToken_p)
{
    VexStatus_t funcres;
    Eip130Token_Command_t CommandToken;
    Eip130Token_Result_t ResultToken;

    // Format command token
    ZEROINIT(CommandToken);
    Eip130Token_Command_Pk_Claim(&CommandToken,
                                 CommandToken_p->Service.PkOperation.Nwords,
                                 CommandToken_p->Service.PkOperation.Mwords,
                                 CommandToken_p->Service.PkOperation.Mmask);
    Vex_Command_SetTokenID(&CommandToken, vex_DeviceGetTokenID());

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


/*----------------------------------------------------------------------------
 * vex_Asym_PkaNumLoad
 */
VexStatus_t
vex_Asym_PkaNumLoad(
        VexToken_Command_t * const CommandToken_p,
        VexToken_Result_t * const ResultToken_p)
{
    VexStatus_t funcres;
    uint64_t VectorAddr = 0;
    Eip130Token_Command_t CommandToken;
    Eip130Token_Result_t ResultToken;

    // Get Vector input address for token
    VectorAddr = BufManager_Map(CommandToken_p->fFromUserSpace,
                                BUFMANAGER_BUFFERTYPE_IN,
                                CommandToken_p->Service.PkOperation.InData_p,
                                CommandToken_p->Service.PkOperation.InDataSize,
                                NULL);
    if (VectorAddr == 0)
    {
        return VEX_NO_MEMORY;
    }

    // Format command token
    ZEROINIT(CommandToken);
    Eip130Token_Command_Pk_NumLoad(&CommandToken,
                                   CommandToken_p->Service.PkOperation.Index,
                                   VectorAddr,
                                   CommandToken_p->Service.PkOperation.InDataSize);
    Vex_Command_SetTokenID(&CommandToken, vex_DeviceGetTokenID());

    // Initialize result token
    ZEROINIT(ResultToken);

    // Exchange token with the EIP-13x HW
    funcres = vex_PhysicalTokenExchange(&CommandToken, &ResultToken);
    if (funcres == VEX_SUCCESS)
    {
        ResultToken_p->Result = Eip130Token_Result_Code(&ResultToken);
    }

    // Release used buffer
    (void)BufManager_Unmap(VectorAddr, false);

    return funcres;
}


/*----------------------------------------------------------------------------
 * vex_Asym_PkaOperation
 */
VexStatus_t
vex_Asym_PkaOperation(
        VexToken_Command_t * const CommandToken_p,
        VexToken_Result_t * const ResultToken_p)
{
    VexStatus_t funcres = VEX_NO_MEMORY;
    uint64_t InputAddr = 0;
    uint64_t OutputAddr = 0;
    size_t InputSize = 0;
    size_t OutputSize = 0;
    uint16_t TokenID;
    bool fCopy = false;
    Eip130Token_Command_t CommandToken;
    Eip130Token_Result_t ResultToken;

    // Get Vector input address for token
    if (CommandToken_p->Service.PkOperation.InData_p != NULL)
    {
        InputSize = CommandToken_p->Service.PkOperation.InDataSize;
        InputAddr = BufManager_Map(CommandToken_p->fFromUserSpace,
                                   BUFMANAGER_BUFFERTYPE_IN,
                                   CommandToken_p->Service.PkOperation.InData_p,
                                   InputSize,
                                   NULL);
        if (InputAddr == 0)
        {
            goto error_func_exit;
        }
    }

    TokenID = vex_DeviceGetTokenID();

    if (CommandToken_p->Service.PkOperation.OutData_p != NULL)
    {
        OutputAddr = BufManager_Map(CommandToken_p->fFromUserSpace,
                                    BUFMANAGER_BUFFERTYPE_OUT,
                                    CommandToken_p->Service.PkOperation.OutData_p,
                                    CommandToken_p->Service.PkOperation.OutDataSize,
                                    (void *)&TokenID);
        if (OutputAddr == 0)
        {
            goto error_func_exit;
        }
        OutputSize = BufManager_GetSize(OutputAddr);
    }

    // Format command token
    ZEROINIT(CommandToken);
    Eip130Token_Command_Pk_Operation(&CommandToken,
                                     (uint8_t)CommandToken_p->Service.PkOperation.Operation,
                                     CommandToken_p->Service.PkOperation.PublicExponent,
                                     InputAddr, (uint32_t)InputSize,
                                     OutputAddr, (uint32_t)OutputSize);
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
    // Release used buffer
    if (InputAddr != 0)
    {
        (void)BufManager_Unmap(InputAddr, false);
    }

    if (OutputAddr != 0)
    {
        int rc = BufManager_Unmap(OutputAddr, fCopy);
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



/* end of file adapter_vex_asym_pka.c */
