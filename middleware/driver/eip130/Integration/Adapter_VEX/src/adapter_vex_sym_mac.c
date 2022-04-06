/* adapter_vex_sym_mac.c
 *
 * Implementation of the VEX API.
 *
 * This file implements the symmetric crypto MAC services.
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
#include "eip130_token_mac.h"       // Eip130Token_Command_Mac*()

/*----------------------------------------------------------------------------
 * vexLocal_SymMacUpdate
 */
static VexStatus_t
vexLocal_SymMacUpdate(
        VexToken_Command_t * const CommandToken_p,
        VexToken_Result_t * const ResultToken_p)
{
    VexStatus_t funcres;
    uint64_t DataAddr;
    bool fInitWithDefault;
    Eip130Token_Command_t CommandToken;
    Eip130Token_Result_t ResultToken;

    // Convert input buffer and get address for token
    DataAddr = BufManager_Map(CommandToken_p->fFromUserSpace,
                              BUFMANAGER_BUFFERTYPE_IN,
                              CommandToken_p->Service.Mac.Data_p,
                              CommandToken_p->Service.Mac.DataSize,
                              NULL);
    if (DataAddr == 0)
    {
        return VEX_NO_MEMORY;
    }

    switch (CommandToken_p->Service.Mac.Mode)
    {
    default:
    case VEXTOKEN_MODE_HASH_MAC_CONT2FINAL:
    case VEXTOKEN_MODE_HASH_MAC_INIT2FINAL:
        return VEX_BAD_ARGUMENT;

    case VEXTOKEN_MODE_HASH_MAC_INIT2CONT:
        fInitWithDefault = true;
        break;

    case VEXTOKEN_MODE_HASH_MAC_CONT2CONT:
        fInitWithDefault = false;
        break;
    }

    // Format command token
    ZEROINIT(CommandToken);
    Eip130Token_Command_Mac(&CommandToken,
                            (uint8_t)CommandToken_p->Service.Mac.Algorithm,
                            fInitWithDefault,
                            false,
                            DataAddr,
                            CommandToken_p->Service.Mac.DataSize);

    // Set the intermediate MAC related information
    if (CommandToken_p->Service.Mac.TempAssetId)
    {
        // Intermediate MAC value via the Asset Store
        Eip130Token_Command_Mac_SetASTempMAC(
            &CommandToken,
            CommandToken_p->Service.Mac.TempAssetId);
    }
    else
    {
        // Intermediate MAC value via the token
        Eip130Token_Command_Mac_CopyMAC(
            &CommandToken,
            CommandToken_p->Service.Mac.TempMac,
            sizeof(CommandToken_p->Service.Mac.TempMac));
    }

    // Copy the key into the token, or set the Asset Store reference
    if (CommandToken_p->Service.Mac.KeyAssetId)
    {
        // Key from Asset Store
        Eip130Token_Command_Mac_SetASLoadKey(
            &CommandToken,
            CommandToken_p->Service.Mac.KeyAssetId);
    }
    else
    {
        // Key via token
        Eip130Token_Command_Mac_CopyKey(&CommandToken,
                                        CommandToken_p->Service.Mac.Key,
                                        CommandToken_p->Service.Mac.KeySize);
    }

    Eip130Token_Command_SetTokenID(&CommandToken,
                                   vex_DeviceGetTokenID(), false);

    // Initialize result token
    ZEROINIT(ResultToken);

    // Exchange token with the EIP-13x HW
   funcres = vex_PhysicalTokenExchange(&CommandToken, &ResultToken);
   if (funcres == VEX_SUCCESS)
   {
       ResultToken_p->Result = Eip130Token_Result_Code(&ResultToken);
       if (CommandToken_p->Service.Mac.TempAssetId == 0)
       {
           Eip130Token_Result_Mac_CopyMAC(
               &ResultToken,
               sizeof(ResultToken_p->Service.Mac.Mac),
               ResultToken_p->Service.Mac.Mac);
       }
   }

   // Release used buffer, if needed
   (void)BufManager_Unmap(DataAddr, false);

   return funcres;
}


/*----------------------------------------------------------------------------
 * vexLocal_SymMacFinalHMAC
 */
static VexStatus_t
vexLocal_SymMacFinalHMAC(
        VexToken_Command_t * const CommandToken_p,
        VexToken_Result_t * const ResultToken_p,
        const uint32_t MACSize)
{
    VexStatus_t funcres;
    uint64_t DataAddr = 0;
    bool fInitWithDefault;
    Eip130Token_Command_t CommandToken;
    Eip130Token_Result_t ResultToken;

    switch (CommandToken_p->Service.Mac.Mode)
    {
    default:
    case VEXTOKEN_MODE_HASH_MAC_CONT2CONT:
    case VEXTOKEN_MODE_HASH_MAC_INIT2CONT:
        return VEX_BAD_ARGUMENT;

    case VEXTOKEN_MODE_HASH_MAC_INIT2FINAL:
        fInitWithDefault = true;
        break;

    case VEXTOKEN_MODE_HASH_MAC_CONT2FINAL:
        fInitWithDefault = false;
        break;
    }

    // Zeroize MAC value
    memset(ResultToken_p->Service.Mac.Mac,
           0,
           sizeof(ResultToken_p->Service.Mac.Mac));

    if (CommandToken_p->Service.Mac.DataSize != 0)
    {
        // Convert input buffer and get address for token
        DataAddr = BufManager_Map(CommandToken_p->fFromUserSpace,
                                  BUFMANAGER_BUFFERTYPE_IN,
                                  CommandToken_p->Service.Mac.Data_p,
                                  ((CommandToken_p->Service.Mac.DataSize + 3) & (uint32_t)~3),
                                  NULL);
        if (DataAddr == 0)
        {
            return VEX_NO_MEMORY;
        }
    }
    else if (!fInitWithDefault)
    {
        return VEX_INVALID_LENGTH;
    }

    // Format command token
    ZEROINIT(CommandToken);
    Eip130Token_Command_Mac(&CommandToken,
                            (uint8_t)CommandToken_p->Service.Mac.Algorithm,
                            fInitWithDefault,
                            true,
                            DataAddr,
                            CommandToken_p->Service.Mac.DataSize);

    // Set the intermediate MAC related information
    if (CommandToken_p->Service.Mac.TempAssetId)
    {
        // Intermediate MAC value via the Asset Store
        Eip130Token_Command_Mac_SetASTempMAC(
            &CommandToken,
            CommandToken_p->Service.Mac.TempAssetId);
    }
    else
    {
        // Intermediate MAC value via the token
        Eip130Token_Command_Mac_CopyMAC(
            &CommandToken,
            CommandToken_p->Service.Mac.TempMac,
            sizeof(CommandToken_p->Service.Mac.TempMac));
    }

    // Copy the key into the token, or set the Asset Store reference
    if (CommandToken_p->Service.Mac.KeyAssetId)
    {
        // Key from Asset Store
        Eip130Token_Command_Mac_SetASLoadKey(
            &CommandToken,
            CommandToken_p->Service.Mac.KeyAssetId);
    }
    else
    {
        // Key via token
        Eip130Token_Command_Mac_CopyKey(&CommandToken,
                                        CommandToken_p->Service.Mac.Key,
                                        CommandToken_p->Service.Mac.KeySize);
    }

    if (CommandToken_p->SubCode == VEXTOKEN_SUBCODE_MACVERIFY)
    {
        // Copy the MAC value to verify into the token, or set the Asset Store
        // reference
        if (CommandToken_p->Service.Mac.MacAssetId)
        {
            // MAC value to verify from Asset Store
            Eip130Token_Command_Mac_SetASLoadMAC(
                &CommandToken,
                CommandToken_p->Service.Mac.MacAssetId);
        }
        else
        {
            // MAC value to verify via token
            Eip130Token_Command_Mac_CopyMAC(
                &CommandToken,
                CommandToken_p->Service.Mac.Mac,
                CommandToken_p->Service.Mac.MacSize);
        }
    }

    Eip130Token_Command_Mac_SetTotalMessageLength(
        &CommandToken,
        CommandToken_p->Service.Mac.TotalMessageLength);
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
           (CommandToken_p->SubCode == VEXTOKEN_SUBCODE_MACGENERATE))
       {
           // Copy the final MAC value from result token
           Eip130Token_Result_Mac_CopyMAC(&ResultToken,
                                          MACSize,
                                          ResultToken_p->Service.Mac.Mac);
       }
   }

   // Release used buffer, if needed
   (void)BufManager_Unmap(DataAddr, false);

   return funcres;
}

/*----------------------------------------------------------------------------
 * vexLocal_SymMacFinalCipherMAC
 */
static VexStatus_t
vexLocal_SymMacFinalCipherMAC(
        VexToken_Command_t * const CommandToken_p,
        VexToken_Result_t * const ResultToken_p,
        const uint32_t BlockSize)
{
    VexStatus_t funcres;
    int npadbytes;
    uint32_t DataSize;
    uint64_t DataAddr;
    uint8_t *Data_p;
    bool fInitWithDefault;
    Eip130Token_Command_t CommandToken;
    Eip130Token_Result_t ResultToken;

    switch (CommandToken_p->Service.Mac.Mode)
    {
    default:
    case VEXTOKEN_MODE_HASH_MAC_CONT2CONT:
    case VEXTOKEN_MODE_HASH_MAC_INIT2CONT:
        return VEX_BAD_ARGUMENT;

    case VEXTOKEN_MODE_HASH_MAC_INIT2FINAL:
        fInitWithDefault = true;
        break;

    case VEXTOKEN_MODE_HASH_MAC_CONT2FINAL:
        fInitWithDefault = false;
        break;
    }

    // Zeroize MAC value
    memset(ResultToken_p->Service.Mac.Mac,
           0,
           sizeof(ResultToken_p->Service.Mac.Mac));

    // Align data size, if needed
    DataSize = (CommandToken_p->Service.Mac.DataSize + BlockSize - 1) &
               ~(BlockSize - 1);
    if (DataSize == 0)
    {
        if (fInitWithDefault &&
            (CommandToken_p->Service.Mac.Algorithm == VEXTOKEN_ALGO_MAC_AES_CBC_MAC))
        {
            // Return immediately
            ResultToken_p->Result = VEXTOKEN_RESULT_NO_ERROR;

            return VEX_SUCCESS;
        }
        DataSize += BlockSize;
    }

    // Allocate an input buffer and get address for token
    DataAddr = BufManager_Alloc(CommandToken_p->fFromUserSpace,
                                BUFMANAGER_BUFFERTYPE_IN,
                                DataSize,
                                CommandToken_p->Service.Mac.Data_p,
                                CommandToken_p->Service.Mac.DataSize,
                                NULL);
    if (DataAddr == 0)
    {
        return VEX_NO_MEMORY;
    }

    // Pad data if needed
    npadbytes = (int)(DataSize - CommandToken_p->Service.Mac.DataSize);
    if (npadbytes)
    {
        // Get the host reference of the buffer
        Data_p = BufManager_GetHostAddress(DataAddr);
        if (Data_p == NULL)
        {
            funcres = VEX_NO_MEMORY;
            goto error_func_exit;
        }
        Data_p += CommandToken_p->Service.Mac.DataSize;

        memset(Data_p, 0, (size_t)npadbytes);
        if ((CommandToken_p->Service.Mac.Algorithm == VEXTOKEN_ALGO_MAC_AES_CMAC)
#ifdef VEX_ENABLE_SYM_ALGO_ARIA
            || (CommandToken_p->Service.Mac.Algorithm == VEXTOKEN_ALGO_MAC_ARIA_CMAC)
#endif
            )
        {
            *Data_p++ = 0x80;
            if (npadbytes == (int)BlockSize)
            {
                npadbytes--;
            }
        }
    }

    // Pass control on to device
    if (BufManager_PreDmaAddress(DataAddr) < 0)
    {
        funcres = VEX_NO_MEMORY;
        goto error_func_exit;
    }

    // Format command token
    ZEROINIT(CommandToken);
    Eip130Token_Command_Mac(&CommandToken,
                            (uint8_t)CommandToken_p->Service.Mac.Algorithm,
                            fInitWithDefault,
                            true,
                            DataAddr,
                            (uint32_t)BufManager_GetSize(DataAddr));

    // Set the intermediate MAC related information
    if (CommandToken_p->Service.Mac.TempAssetId)
    {
        // Intermediate MAC value via the Asset Store
        Eip130Token_Command_Mac_SetASTempMAC(
            &CommandToken,
            CommandToken_p->Service.Mac.TempAssetId);
    }
    else
    {
        // Intermediate MAC value via the token
        Eip130Token_Command_Mac_CopyMAC(
            &CommandToken,
            CommandToken_p->Service.Mac.TempMac,
            sizeof(CommandToken_p->Service.Mac.TempMac));
    }

    // Copy the key into the token, or set the Asset Store reference
    if (CommandToken_p->Service.Mac.KeyAssetId)
    {
        // Key from Asset Store
        Eip130Token_Command_Mac_SetASLoadKey(
            &CommandToken,
            CommandToken_p->Service.Mac.KeyAssetId);
    }
    else
    {
        // Key via token
        Eip130Token_Command_Mac_CopyKey(&CommandToken,
                                        CommandToken_p->Service.Mac.Key,
                                        CommandToken_p->Service.Mac.KeySize);
    }

    if (CommandToken_p->SubCode == VEXTOKEN_SUBCODE_MACVERIFY)
    {
        // Copy the MAC value to verify into the token, or set the Asset Store
        // reference
        if (CommandToken_p->Service.Mac.MacAssetId)
        {
            // MAC value to verify from Asset Store
            Eip130Token_Command_Mac_SetASLoadMAC(
                &CommandToken,
                CommandToken_p->Service.Mac.MacAssetId);
        }
        else
        {
            // MAC value to verify via token
            Eip130Token_Command_Mac_CopyMAC(
                &CommandToken,
                CommandToken_p->Service.Mac.Mac,
                CommandToken_p->Service.Mac.MacSize);
        }
    }

    Eip130Token_Command_Mac_SetTotalMessageLength(&CommandToken,
                                                  (uint64_t)npadbytes);
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
           (CommandToken_p->SubCode == VEXTOKEN_SUBCODE_MACGENERATE))
       {
           // Copy the final MAC value from result token
           Eip130Token_Result_Mac_CopyMAC(&ResultToken,
                                          BlockSize,
                                          ResultToken_p->Service.Mac.Mac);
       }
   }

error_func_exit:
   // Release used buffer, if needed
   (void)BufManager_Unmap(DataAddr, false);

   return funcres;
}


/*----------------------------------------------------------------------------
 * vex_SymMac
 */
VexStatus_t
vex_SymMac(
        VexToken_Command_t * const CommandToken_p,
        VexToken_Result_t * const ResultToken_p)
{
    VexStatus_t funcres;

    switch (CommandToken_p->SubCode)
    {
    case VEXTOKEN_SUBCODE_MACUPDATE:
        switch (CommandToken_p->Service.Mac.Algorithm)
        {
        case VEXTOKEN_ALGO_MAC_HMAC_SHA1:
        case VEXTOKEN_ALGO_MAC_HMAC_SHA224:
        case VEXTOKEN_ALGO_MAC_HMAC_SHA256:
#ifdef VEX_ENABLE_SYM_ALGO_SHA512
        case VEXTOKEN_ALGO_MAC_HMAC_SHA384:
        case VEXTOKEN_ALGO_MAC_HMAC_SHA512:
#endif
        case VEXTOKEN_ALGO_MAC_AES_CMAC:
        case VEXTOKEN_ALGO_MAC_AES_CBC_MAC:
#ifdef VEX_ENABLE_SYM_ALGO_ARIA
        case VEXTOKEN_ALGO_MAC_ARIA_CMAC:
        case VEXTOKEN_ALGO_MAC_ARIA_CBC_MAC:
#endif
#ifdef VEX_ENABLE_SYM_ALGO_POLY1305
        case VEXTOKEN_ALGO_MAC_POLY1305:
#endif
            funcres = vexLocal_SymMacUpdate(CommandToken_p, ResultToken_p);
            break;

        default:
            funcres = VEX_BAD_ARGUMENT;
            break;
        }
        break;

    case VEXTOKEN_SUBCODE_MACGENERATE:
    case VEXTOKEN_SUBCODE_MACVERIFY:
        switch (CommandToken_p->Service.Mac.Algorithm)
        {
        case VEXTOKEN_ALGO_MAC_HMAC_SHA1:
            funcres = vexLocal_SymMacFinalHMAC(CommandToken_p,
                                               ResultToken_p,
                                               (160/8));
            break;
        case VEXTOKEN_ALGO_MAC_HMAC_SHA224:
            funcres = vexLocal_SymMacFinalHMAC(CommandToken_p,
                                               ResultToken_p,
                                               (224/8));
            break;
        case VEXTOKEN_ALGO_MAC_HMAC_SHA256:
            funcres = vexLocal_SymMacFinalHMAC(CommandToken_p,
                                               ResultToken_p,
                                               (256/8));
            break;
#ifdef VEX_ENABLE_SYM_ALGO_SHA512
        case VEXTOKEN_ALGO_MAC_HMAC_SHA384:
            funcres = vexLocal_SymMacFinalHMAC(CommandToken_p,
                                               ResultToken_p,
                                               (384/8));
            break;
        case VEXTOKEN_ALGO_MAC_HMAC_SHA512:
            funcres = vexLocal_SymMacFinalHMAC(CommandToken_p,
                                               ResultToken_p,
                                               (512/8));
            break;
#endif
        case VEXTOKEN_ALGO_MAC_AES_CMAC:
        case VEXTOKEN_ALGO_MAC_AES_CBC_MAC:
#ifdef VEX_ENABLE_SYM_ALGO_ARIA
        case VEXTOKEN_ALGO_MAC_ARIA_CMAC:
        case VEXTOKEN_ALGO_MAC_ARIA_CBC_MAC:
#endif
            funcres = vexLocal_SymMacFinalCipherMAC(CommandToken_p,
                                                    ResultToken_p,
                                                    (128/8));
            break;
#ifdef VEX_ENABLE_SYM_ALGO_POLY1305
        case VEXTOKEN_ALGO_MAC_POLY1305:
            // For token processing it can be regarded as an HMAC
            funcres = vexLocal_SymMacFinalHMAC(CommandToken_p,
                                               ResultToken_p,
                                               (128/8));
            break;
#endif

        default:
            funcres = VEX_BAD_ARGUMENT;
            break;
        }
        break;

    default:
        funcres = VEX_INVALID_SUBCODE;
        break;
    }

    return funcres;
}


/* end of file adapter_vex_sym_mac.c */
