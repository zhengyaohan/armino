/* adapter_vex_aunlock.c
 *
 * Implementation of the VEX API.
 *
 * This file implements the Authenticated Unlock and Secure Debug services.
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
#include "eip130_token_aunlock.h"   // Eip130Token_Command_RegisterRead/Write()
                                    // Eip130Token_Command_ZeroizeOutputMailbox()
#include "eip130_token_otp.h"       // Eip130Token_Command_OTP(Select)Zeroize()


/*----------------------------------------------------------------------------
 * vex_AUnlock
 */
VexStatus_t
vex_AUnlock(
        VexToken_Command_t * const CommandToken_p,
        VexToken_Result_t * const ResultToken_p)
{
    VexStatus_t funcres;
    Eip130Token_Command_t CommandToken;
    Eip130Token_Result_t ResultToken;
    uint64_t SignAddr = 0;

    // Format command token
    ZEROINIT(CommandToken);
    switch (CommandToken_p->SubCode)
    {
    case VEXTOKEN_SUBCODE_AUNLOCKSTART:
        Eip130Token_Command_AUnlock_Start(
            &CommandToken,
            CommandToken_p->Service.AuthUnlock.AuthStateAssetId,
            CommandToken_p->Service.AuthUnlock.AuthKeyAssetId);
        break;

    case VEXTOKEN_SUBCODE_AUNLOCKVERIFY:
        if (CommandToken_p->Service.AuthUnlock.Sign_p == NULL)
        {
            return VEX_BAD_ARGUMENT;
        }

        // Get signature output address for token
        SignAddr = BufManager_Map(CommandToken_p->fFromUserSpace,
                                  BUFMANAGER_BUFFERTYPE_IN,
                                  CommandToken_p->Service.AuthUnlock.Sign_p,
                                  CommandToken_p->Service.AuthUnlock.SignSize,
                                  NULL);
        if (SignAddr == 0)
        {
            return VEX_NO_MEMORY;
        }

        Eip130Token_Command_AUnlock_Verify(
            &CommandToken,
            CommandToken_p->Service.AuthUnlock.AuthStateAssetId,
            CommandToken_p->Service.AuthUnlock.Nonce,
            SignAddr, CommandToken_p->Service.AuthUnlock.SignSize);
        break;

    case VEXTOKEN_SUBCODE_SETSECUREDEBUG:
        Eip130Token_Command_SetSecureDebug(
            &CommandToken,
            CommandToken_p->Service.SecureDebug.AuthStateAssetId,
            CommandToken_p->Service.SecureDebug.fSet);
        break;

    default:
        return VEX_UNSUPPORTED;
    }
    Eip130Token_Command_SetTokenID(&CommandToken,
                                   vex_DeviceGetTokenID(),
                                   false);

    // Initialize result token
    ZEROINIT(ResultToken);

    // Exchange token with the EIP-13x HW
    funcres = vex_PhysicalTokenExchange(&CommandToken, &ResultToken);
    if (funcres == VEX_SUCCESS)
    {
        ResultToken_p->Result = Eip130Token_Result_Code(&ResultToken);
        if ((ResultToken_p->Result >= 0) &&
            (CommandToken_p->SubCode == VEXTOKEN_SUBCODE_AUNLOCKSTART))
        {
            // Success - Copy AssetId and Nonce from result token
            Eip130Token_Result_AUnlock_CopyNonce(&ResultToken,
                                                 ResultToken_p->Service.AuthUnlock.Nonce);
        }
    }

    if (SignAddr != 0)
    {
        (void)BufManager_Unmap(SignAddr, false);
    }

    return funcres;
}

/* end of file adapter_vex_aunlock.c */
