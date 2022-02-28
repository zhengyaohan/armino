/* adapter_val_aunlock.c
 *
 * Implementation of the VAL API.
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

#include "c_adapter_val.h"              // configuration

#include "basic_defs.h"
#include "clib.h"
#include "log.h"

#include "api_val_aunlock.h"            // the API to implement
#include "api_val_asset.h"              // the API to implement
#include "api_val_system.h"             // val_IsAccessSecure()
#include "adapter_val_internal.h"       // val_ExchangeToken()
#include "adapter_vex.h"                // VexToken_Command_t, VexToken_Result_t


/*----------------------------------------------------------------------------
 * val_AuthenticatedUnlockStart
 */
#ifndef VAL_REMOVE_AUTH_UNLOCK
ValStatus_t
val_AuthenticatedUnlockStart(
        ValAssetNumber_t AuthKeyNumber,
        ValAssetId_t * AuthStateASId_p,
        ValOctetsOut_t * Nonce_p,
        ValSize_t * NonceLength_p)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValAssetId_t KeyAssetId;
    ValPolicyMask_t AssetPolicy = VAL_POLICY_TEMP_AUTH_STATE;
    ValStatus_t funcres;

#ifdef VAL_STRICT_ARGS
    if ((AuthStateASId_p == NULL) ||
        (Nonce_p == NULL) ||
        (NonceLength_p == NULL) ||
        (*NonceLength_p < sizeof(t_res.Service.AuthUnlock.Nonce)))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Get the AssetID of the authenticated unlock key
    funcres = val_AssetSearch(AuthKeyNumber, &KeyAssetId, NULL);
    if (funcres != VAL_SUCCESS)
    {
        goto error_func_exit;
    }

    // Allocate an Asset for the authenticated unlock state
    if (!val_IsAccessSecure())
    {
        AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
    }
    funcres = val_AssetAlloc(AssetPolicy, 20,
                             false, false,
                             VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                             AuthStateASId_p);
    if (funcres != VAL_SUCCESS)
    {
        goto error_func_exit;
    }

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_AUTH_UNLOCK;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_AUNLOCKSTART;
    t_cmd.Service.AuthUnlock.AuthKeyAssetId = KeyAssetId;
    t_cmd.Service.AuthUnlock.AuthStateAssetId = *AuthStateASId_p;
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;
    ZEROINIT(t_res.Service.AuthUnlock.Nonce);

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if (funcres == VAL_SUCCESS)
    {
        if (t_res.Result == VEXTOKEN_RESULT_NO_ERROR)
        {
            memcpy(Nonce_p,
                   t_res.Service.AuthUnlock.Nonce,
                   sizeof(t_res.Service.AuthUnlock.Nonce));
            *NonceLength_p = sizeof(t_res.Service.AuthUnlock.Nonce);
        }
        else
        {
            funcres = (ValStatus_t)t_res.Result;

            (void)val_AssetFree(*AuthStateASId_p);
            *AuthStateASId_p = VAL_ASSETID_INVALID;
        }
    }

error_func_exit:
    return funcres;
}
#endif /* !VAL_REMOVE_AUTH_UNLOCK */


/*----------------------------------------------------------------------------
 * val_AuthenticatedUnlockVerify
 */
#ifndef VAL_REMOVE_AUTH_UNLOCK
ValStatus_t
val_AuthenticatedUnlockVerify(
        const ValAssetId_t AuthStateASId,
        ValAsymBigInt_t * const Signature_p,
        ValOctetsIn_t * const Nonce_p,
        const ValSize_t NonceLength)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;

#ifdef VAL_STRICT_ARGS
    if ((AuthStateASId == VAL_ASSETID_INVALID) ||
        (Signature_p == NULL) ||
        (Signature_p->Data_p == NULL) ||
        (Signature_p->ByteDataSize < VAL_ASYM_DATA_SIZE_B2B(1024)) ||
        (Signature_p->ByteDataSize > VAL_ASYM_DATA_SIZE_B2B(VAL_ASYM_RSA_MAX_BITS)) ||
        (Nonce_p == NULL) ||
        (NonceLength != sizeof(t_cmd.Service.AuthUnlock.Nonce)))
    {
        return VAL_BAD_ARGUMENT;
    }
#else
    IDENTIFIER_NOT_USED(NonceLength);
#endif

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_AUTH_UNLOCK;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_AUNLOCKVERIFY;
    t_cmd.Service.AuthUnlock.AuthStateAssetId = (uint32_t)AuthStateASId;
    t_cmd.Service.AuthUnlock.Sign_p = Signature_p->Data_p;
    t_cmd.Service.AuthUnlock.SignSize = Signature_p->ByteDataSize;
    memcpy(t_cmd.Service.AuthUnlock.Nonce,
           Nonce_p,
           sizeof(t_cmd.Service.AuthUnlock.Nonce));
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if (funcres == VAL_SUCCESS)
    {
        // Copy possible service error
        funcres = (ValStatus_t)t_res.Result;
    }

    return funcres;
}
#endif /* !VAL_REMOVE_AUTH_UNLOCK */


/*----------------------------------------------------------------------------
 * val_SecureDebug
 */
#ifndef VAL_REMOVE_AUTH_UNLOCK
#ifndef VAL_REMOVE_SECURE_DEBUG
ValStatus_t
val_SecureDebug(
        const ValAssetId_t AuthStateASId,
        const bool fSet)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;

#ifdef VAL_STRICT_ARGS
    if (AuthStateASId == VAL_ASSETID_INVALID)
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_AUTH_UNLOCK;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_SETSECUREDEBUG;
    t_cmd.Service.SecureDebug.AuthStateAssetId = (uint32_t)AuthStateASId;
    t_cmd.Service.SecureDebug.fSet = fSet;
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if (funcres == VAL_SUCCESS)
    {
        // Copy possible service error
        funcres = (ValStatus_t)t_res.Result;
    }

    return funcres;
}
#endif /* !VAL_REMOVE_SECURE_DEBUG */
#endif /* !VAL_REMOVE_AUTH_UNLOCK */


/* end of file adapter_val_aunlock.c */
