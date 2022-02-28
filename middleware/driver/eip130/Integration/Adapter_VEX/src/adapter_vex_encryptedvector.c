/* adapter_vex_encryptedvector.c
 *
 * Implementation of the VEX API.
 *
 * This file implements the encrypted vector for PKI services.
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

#ifdef VEX_ENABLE_ENCRYPTED_VECTOR

#include "basic_defs.h"
#include "clib.h"

#include "adapter_vex_internal.h"   // API implementation
#include "adapter_bufmanager.h"     // BufManager_*()
#include "eip130_token_wrap.h"    // Eip130Token_Command_Crypto*()


/*----------------------------------------------------------------------------
 * vex_EncryptedVector
 */
VexStatus_t
vex_EncryptedVector(
        VexToken_Command_t * const CommandToken_p,
        VexToken_Result_t * const ResultToken_p)
{
    VexStatus_t funcres = VEX_NO_MEMORY;
    uint16_t TokenID;
    uint64_t DataAddr = 0;
    bool fCopy = false;
    Eip130Token_Command_t CommandToken;
    Eip130Token_Result_t ResultToken;

    // Get output address for token
    TokenID = vex_DeviceGetTokenID();
    DataAddr = BufManager_Map(CommandToken_p->fFromUserSpace,
                              BUFMANAGER_BUFFERTYPE_OUT,
                              CommandToken_p->Service.EncVector.Data_p,
                              CommandToken_p->Service.EncVector.DataSize,
                              (void *)&TokenID);
    if (DataAddr == 0)
    {
        return VEX_INTERNAL_ERROR;
    }

    // Format command token
    ZEROINIT(CommandToken);
    Eip130Token_Command_EncryptedVectorPKI(&CommandToken,
                                           CommandToken_p->Service.EncVector.AssetId,
                                           CommandToken_p->Service.EncVector.AssetNumber,
                                           DataAddr,
                                           CommandToken_p->Service.EncVector.DataSize);
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

            Eip130Token_Result_EncryptedVectorPKI_GetDataLength(
                &ResultToken,
                &ResultToken_p->Service.EncVector.DataSize);
        }
        else if (ResultToken_p->Result == VEXTOKEN_RESULT_SEQ_INVALID_LENGTH)
        {
            Eip130Token_Result_EncryptedVectorPKI_GetDataLength(
                &ResultToken,
                &ResultToken_p->Service.EncVector.DataSize);
        }
    }

    // Release used buffer, if needed
    if (BufManager_Unmap(DataAddr, fCopy) != 0)
    {
        funcres = VEX_INTERNAL_ERROR;
    }

    return funcres;
}

#endif /* VEX_ENABLE_ENCRYPTED_VECTOR */

/* end of file adapter_vex_encryptedvector.c */
