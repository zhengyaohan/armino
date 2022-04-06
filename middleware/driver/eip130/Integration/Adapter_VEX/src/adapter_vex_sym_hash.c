/* adapter_vex_sym_hash.c
 *
 * Implementation of the VEX API.
 *
 * This file implements the symmetric crypto hash services.
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
#include "eip130_token_hash.h"      // Eip130Token_Command_Hash*()


/*----------------------------------------------------------------------------
 * vex_SymHash
 */
VexStatus_t
vex_SymHash(
        VexToken_Command_t * const CommandToken_p,
        VexToken_Result_t * const ResultToken_p)
{
    VexStatus_t funcres;
    uint64_t DataAddr = 0;
    bool fInitWithDefault;
    bool fFinalize;
    Eip130Token_Command_t CommandToken;
    Eip130Token_Result_t ResultToken;

    switch (CommandToken_p->Service.Hash.Mode)
    {
    default:
    case VEXTOKEN_MODE_HASH_MAC_INIT2FINAL:
        fInitWithDefault = true;
        fFinalize = true;
        break;

    case VEXTOKEN_MODE_HASH_MAC_CONT2FINAL:
        fInitWithDefault = false;
        fFinalize = true;
        break;

    case VEXTOKEN_MODE_HASH_MAC_INIT2CONT:
        fInitWithDefault = true;
        fFinalize = false;
        break;

    case VEXTOKEN_MODE_HASH_MAC_CONT2CONT:
        fInitWithDefault = false;
        fFinalize = false;
        break;
    }

    if (CommandToken_p->Service.Hash.DataSize != 0)
    {
        // Convert input buffer and get address for token
        DataAddr = BufManager_Map(CommandToken_p->fFromUserSpace,
                                  BUFMANAGER_BUFFERTYPE_IN,
                                  CommandToken_p->Service.Hash.Data_p,
                                  CommandToken_p->Service.Hash.DataSize,
                                  NULL);
        if (DataAddr == 0)
        {
            return VEX_NO_MEMORY;
        }
    }
    else if (!fInitWithDefault || !fFinalize)
    {
        return VEX_INVALID_LENGTH;
    }

    // Format command token
    ZEROINIT(CommandToken);
    Eip130Token_Command_Hash(&CommandToken,
                             (uint8_t)CommandToken_p->Service.Hash.Algorithm,
                             fInitWithDefault,
                             fFinalize,
                             DataAddr,
                             CommandToken_p->Service.Hash.DataSize);
    if (CommandToken_p->Service.Hash.TempAssetId)
    {
        Eip130Token_Command_Hash_SetTempDigestASID(
            &CommandToken,
            CommandToken_p->Service.Hash.TempAssetId);
    }
    else
    {
        Eip130Token_Command_Hash_CopyDigest(
            &CommandToken,
            CommandToken_p->Service.Hash.Digest,
            sizeof(CommandToken_p->Service.Hash.Digest));
    }
    Eip130Token_Command_Hash_SetTotalMessageLength(
        &CommandToken,
        CommandToken_p->Service.Hash.TotalMessageLength);
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
            (fFinalize || (CommandToken_p->Service.Hash.TempAssetId == 0)))
        {
            // Copy digest from result token
            Eip130Token_Result_Hash_CopyDigest(
                &ResultToken,
                sizeof(ResultToken_p->Service.Hash.Digest),
                ResultToken_p->Service.Hash.Digest);
        }
    }

    // Release used buffer, if needed
    (void)BufManager_Unmap(DataAddr, false);

    return funcres;
}


/* end of file adapter_vex_sym_hash.c */
