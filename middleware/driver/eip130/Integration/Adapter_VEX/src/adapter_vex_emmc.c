/* adapter_vex_emmc.c
 *
 * Implementation of the VEX API.
 *
 * This file implements the eMMC related services.
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

#include "c_adapter_vex.h"          // configuration

#include "basic_defs.h"
#include "clib.h"

#include "adapter_vex_internal.h"   // API implementation
#include "adapter_bufmanager.h"     // BufManager_*()
#include "eip130_token_emmc.h"      // Eip130Token_Command_eMMC*()


/*----------------------------------------------------------------------------
 * vex_eMMC
 */
VexStatus_t
vex_eMMC(
        VexToken_Command_t * const CommandToken_p,
        VexToken_Result_t * const ResultToken_p)
{
    VexStatus_t funcres;
    uint64_t InputDataAddr = 0;
    Eip130Token_Command_t CommandToken;
    Eip130Token_Result_t ResultToken;
    bool fWriteRequest = false;
    bool fVerify = false;

    // Format command token
    ZEROINIT(CommandToken);
    switch (CommandToken_p->SubCode)
    {
    case VEXTOKEN_SUBCODE_EMMC_RDREQ:
        Eip130Token_Command_eMMC_ReadRequest(
            &CommandToken,
            CommandToken_p->Service.eMMCRdRequest.AssetId);
        break;

    case VEXTOKEN_SUBCODE_EMMC_RDVER:
        Eip130Token_Command_eMMC_ReadVerify(&CommandToken);
        fVerify = true;
        break;

    case VEXTOKEN_SUBCODE_EMMC_RDWRCNTREQ:
        Eip130Token_Command_eMMC_ReadWriteCounterRequest(
            &CommandToken,
            CommandToken_p->Service.eMMCRdRequest.AssetId);
        break;

    case VEXTOKEN_SUBCODE_EMMC_RDWRCNTVER:
        Eip130Token_Command_eMMC_ReadWriteCounterVerify(&CommandToken);
        fVerify = true;
        break;

    case VEXTOKEN_SUBCODE_EMMC_WRREQ:
        Eip130Token_Command_eMMC_WriteRequest(&CommandToken);
        fWriteRequest = true;
        fVerify = true;
        break;

    case VEXTOKEN_SUBCODE_EMMC_WRVER:
        Eip130Token_Command_eMMC_WriteVerify(&CommandToken);
        fVerify = true;
        break;

    default:
        return VEX_UNSUPPORTED;
    }
    if (fVerify)
    {
        InputDataAddr = BufManager_Map(CommandToken_p->fFromUserSpace,
                                       BUFMANAGER_BUFFERTYPE_IN,
                                       CommandToken_p->Service.eMMCRdVerifyWrReqVer.Data_p,
                                       CommandToken_p->Service.eMMCRdVerifyWrReqVer.DataSize,
                                       NULL);
        if (InputDataAddr == 0)
        {
            return VEX_NO_MEMORY;
        }

        Eip130Token_Command_eMMC_SetInputDataAndMACInfo(
            &CommandToken,
            InputDataAddr,
            CommandToken_p->Service.eMMCRdVerifyWrReqVer.DataSize);
        Eip130Token_Command_eMMC_SetStateAssetID(
            &CommandToken,
            CommandToken_p->Service.eMMCRdVerifyWrReqVer.StateAssetId);
        if (!fWriteRequest)
        {
            Eip130Token_Command_eMMC_CopyMAC(
                &CommandToken,
                CommandToken_p->Service.eMMCRdVerifyWrReqVer.Mac,
                CommandToken_p->Service.eMMCRdVerifyWrReqVer.MacSize);
        }
    }
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
            if (fVerify)
            {
                if (fWriteRequest)
                {
                    Eip130Token_Result_eMMC_CopyMAC(
                            &ResultToken,
                            32,
                            ResultToken_p->Service.eMMCRdVerifyWrReqVer.Mac);
                }
            }
            else
            {
                if (CommandToken_p->SubCode == VEXTOKEN_SUBCODE_EMMC_RDREQ)
                {
                    Eip130Token_Result_eMMC_ReadStateID(
                        &ResultToken,
                        &ResultToken_p->Service.eMMCRdRequest.AssetId);
                }
                else
                {
                    // Simply copy it from the command token
                    ResultToken_p->Service.eMMCRdRequest.AssetId =
                        CommandToken_p->Service.eMMCRdRequest.AssetId;
                }

                // Copy the Nonce from result token
                Eip130Token_Result_eMMC_ReadCopyNonce(
                    &ResultToken,
                    ResultToken_p->Service.eMMCRdRequest.Nonce);
            }
        }
    }

    // Release used buffers, if needed
    if (InputDataAddr != 0)
    {
        (void)BufManager_Unmap(InputDataAddr, false);
    }

    return funcres;
}

/* end of file adapter_vex_emmc.c */
