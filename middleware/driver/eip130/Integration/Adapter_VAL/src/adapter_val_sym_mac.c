/* adapter_val_sym_mac.c
 *
 * Implementation of the VAL API.
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

#include "c_adapter_val.h"              // configuration

#ifndef VAL_REMOVE_SYM_MAC

#include "basic_defs.h"
#include "clib.h"
#include "log.h"

#include "api_val_sym.h"                // the API to implement
#include "api_val_asset.h"              // Asset Management related information
#include "api_val_system.h"             // val_IsAccessSecure()
#include "adapter_val_internal.h"       // val_ExchangeToken()
#include "adapter_vex.h"                // VexToken_Command_t, VexToken_Result_t



/*----------------------------------------------------------------------------
 * val_SymMacUpdate
 */
ValStatus_t
val_SymMacUpdate(
        ValSymContextPtr_t const Context_p,
        ValOctetsIn_t * const Data_p,
        const ValSize_t DataSize)
{
    ValSymContext_t * SymContext_p = (ValSymContext_t *)Context_p;
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;
    ValPolicyMask_t AssetPolicy = VAL_POLICY_TEMP_MAC;
    ValSize_t MacNBytes = 0;

#ifdef VAL_STRICT_ARGS
    if ((SymContext_p == NULL) ||
        (SymContext_p->MagicBegin != VALMARKER_SYMCONTEXT) ||
        ((SymContext_p->MagicBegin ^ SymContext_p->MagicEnd) != 0) ||
        (Data_p == NULL) ||
        (DataSize == 0))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    ZEROINIT(t_cmd);

    // Check and set MAC algorithm
    switch (SymContext_p->Algorithm)
    {
    case VAL_SYM_ALGO_MAC_HMAC_SHA1:
#ifdef VAL_STRICT_ARGS
        if ((DataSize & (VAL_SYM_ALGO_HASH_BLOCK_SIZE-1)) != 0)
        {
            return VAL_BAD_ARGUMENT;
        }
#endif
        t_cmd.Service.Mac.Algorithm = VEXTOKEN_ALGO_MAC_HMAC_SHA1;
        MacNBytes = (160 / 8);
        AssetPolicy |= VAL_POLICY_SHA1;
        break;

    case VAL_SYM_ALGO_MAC_HMAC_SHA224:
#ifdef VAL_STRICT_ARGS
        if ((DataSize & (VAL_SYM_ALGO_HASH_BLOCK_SIZE-1)) != 0)
        {
            return VAL_BAD_ARGUMENT;
        }
#endif
        t_cmd.Service.Mac.Algorithm = VEXTOKEN_ALGO_MAC_HMAC_SHA224;
        MacNBytes = (256 / 8);
        // Note: intermediate digest is always 256 bits, final is 224 bits
        AssetPolicy |= VAL_POLICY_SHA224;
        break;

    case VAL_SYM_ALGO_MAC_HMAC_SHA256:
#ifdef VAL_STRICT_ARGS
        if ((DataSize & (VAL_SYM_ALGO_HASH_BLOCK_SIZE-1)) != 0)
        {
            return VAL_BAD_ARGUMENT;
        }
#endif
        t_cmd.Service.Mac.Algorithm = VEXTOKEN_ALGO_MAC_HMAC_SHA256;
        MacNBytes = (256 / 8);
        AssetPolicy |= VAL_POLICY_SHA256;
        break;

#ifndef VAL_REMOVE_SYM_ALGO_SHA512
    case VAL_SYM_ALGO_MAC_HMAC_SHA384:
#ifdef VAL_STRICT_ARGS
        if ((DataSize & (VAL_SYM_ALGO_HASH_SHA512_BLOCK_SIZE-1)) != 0)
        {
            return VAL_BAD_ARGUMENT;
        }
#endif
        t_cmd.Service.Mac.Algorithm = VEXTOKEN_ALGO_MAC_HMAC_SHA384;
        MacNBytes = (512 / 8);
        AssetPolicy |= VAL_POLICY_SHA384;
        break;

    case VAL_SYM_ALGO_MAC_HMAC_SHA512:
#ifdef VAL_STRICT_ARGS
        if ((DataSize & (VAL_SYM_ALGO_HASH_SHA512_BLOCK_SIZE-1)) != 0)
        {
            return VAL_BAD_ARGUMENT;
        }
#endif
        t_cmd.Service.Mac.Algorithm = VEXTOKEN_ALGO_MAC_HMAC_SHA512;
        MacNBytes = (512 / 8);
        AssetPolicy |= VAL_POLICY_SHA512;
        break;
#endif

    case VAL_SYM_ALGO_MAC_AES_CMAC:
#ifdef VAL_STRICT_ARGS
        if ((DataSize & (VAL_SYM_ALGO_AES_BLOCK_SIZE-1)) != 0)
        {
            return VAL_BAD_ARGUMENT;
        }
#endif
        t_cmd.Service.Mac.Algorithm = VEXTOKEN_ALGO_MAC_AES_CMAC;
        MacNBytes = VAL_SYM_ALGO_AES_IV_SIZE;
        AssetPolicy |= VAL_POLICY_ALGO_CIPHER_AES | VAL_POLICY_CMAC;
        break;

    case VAL_SYM_ALGO_MAC_AES_CBC_MAC:
#ifdef VAL_STRICT_ARGS
        if ((DataSize & (VAL_SYM_ALGO_AES_BLOCK_SIZE-1)) != 0)
        {
            return VAL_BAD_ARGUMENT;
        }
#endif
        t_cmd.Service.Mac.Algorithm = VEXTOKEN_ALGO_MAC_AES_CBC_MAC;
        MacNBytes = VAL_SYM_ALGO_AES_IV_SIZE;
        AssetPolicy |= VAL_POLICY_ALGO_CIPHER_AES;
        break;

#ifndef VAL_REMOVE_SYM_ALGO_ARIA
    case VAL_SYM_ALGO_MAC_ARIA_CMAC:
#ifdef VAL_STRICT_ARGS
        if ((DataSize & (VAL_SYM_ALGO_ARIA_BLOCK_SIZE-1)) != 0)
        {
            return VAL_BAD_ARGUMENT;
        }
#endif
        t_cmd.Service.Mac.Algorithm = VEXTOKEN_ALGO_MAC_ARIA_CMAC;
        MacNBytes = VAL_SYM_ALGO_ARIA_IV_SIZE;
        AssetPolicy |= VAL_POLICY_ALGO_CIPHER_ARIA | VAL_POLICY_CMAC;
        break;

    case VAL_SYM_ALGO_MAC_ARIA_CBC_MAC:
#ifdef VAL_STRICT_ARGS
        if ((DataSize & (VAL_SYM_ALGO_ARIA_BLOCK_SIZE-1)) != 0)
        {
            return VAL_BAD_ARGUMENT;
        }
#endif
        t_cmd.Service.Mac.Algorithm = VEXTOKEN_ALGO_MAC_ARIA_CBC_MAC;
        MacNBytes = VAL_SYM_ALGO_ARIA_IV_SIZE;
        AssetPolicy |= VAL_POLICY_ALGO_CIPHER_ARIA;
        break;
#endif

#ifndef VAL_REMOVE_SYM_ALGO_POLY1305
    case VAL_SYM_ALGO_MAC_POLY1305:
#ifdef VAL_STRICT_ARGS
        if ((DataSize & (VAL_SYM_ALGO_POLY1305_BLOCK_SIZE-1)) != 0)
        {
            return VAL_BAD_ARGUMENT;
        }
#endif
        t_cmd.Service.Mac.Algorithm = VEXTOKEN_ALGO_MAC_POLY1305;
        MacNBytes = 24;
        AssetPolicy |= VAL_POLICY_POLY1305;
        break;
#endif

    default:
        return VAL_INVALID_ALGORITHM;
    }

    // Set service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_MAC;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_MACUPDATE;

    // Set MAC mode
    if (!SymContext_p->fInitDone)
    {
        t_cmd.Service.Mac.Mode = VEXTOKEN_MODE_HASH_MAC_INIT2CONT;

        if (!SymContext_p->fUseTokenTemp &&
            (SymContext_p->Service.Mac.TempAssetId == VAL_ASSETID_INVALID))
        {
            // Allocate Asset for intermediate MAC value
            if (!val_IsAccessSecure())
            {
                AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
            }
            funcres = val_AssetAlloc(AssetPolicy, MacNBytes,
                                     false, false,
                                     VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                                     &SymContext_p->Service.Mac.TempAssetId);
            if (funcres != VAL_SUCCESS)
            {
                LOG_WARN("%s: Abort - AssetAlloc()=%d\n", __func__, funcres);
                return funcres;
            }

            if (SymContext_p->Algorithm == VAL_SYM_ALGO_MAC_AES_CBC_MAC)
            {
                // Initialize the intermediate MAC Asset data
                uint8_t * AssetData_p = Adapter_Alloc((unsigned int)MacNBytes);
                if (AssetData_p == NULL)
                {
                    funcres = VAL_NO_MEMORY;
                }
                else
                {
                    memcpy(AssetData_p, SymContext_p->Service.Mac.Mac, MacNBytes);
                    funcres = val_AssetLoadPlaintext(SymContext_p->Service.Mac.TempAssetId,
                                                     AssetData_p, MacNBytes);
                    Adapter_Free(AssetData_p);
                }
                if (funcres != VAL_SUCCESS)
                {
                    LOG_WARN("%s: Abort - AssetLoadPlaintext()=%d\n",
                             __func__, funcres);

                    // Free Asset again
                    (void)val_AssetFree(SymContext_p->Service.Mac.TempAssetId);
                    SymContext_p->Service.Mac.TempAssetId = VAL_ASSETID_INVALID;
                    return funcres;
                }
            }
        }
    }
    else
    {
        t_cmd.Service.Mac.Mode = VEXTOKEN_MODE_HASH_MAC_CONT2CONT;
    }

    // Set temporary Asset or intermediate MAC, Key, Data and TotalMessageLength=0
    if (SymContext_p->fUseTokenTemp)
    {
        if (SymContext_p->Service.Mac.KeyAssetId != VAL_ASSETID_INVALID)
        {
            // Token intermediate MAC and Key from Asset Store is not allowed
            return VAL_INVALID_ALGORITHM;
        }

        t_cmd.Service.Mac.TempAssetId = VAL_ASSETID_INVALID;
        if (SymContext_p->fInitDone)
        {
            memcpy(t_cmd.Service.Mac.TempMac,
                   SymContext_p->Service.Mac.Mac,
                   sizeof(t_cmd.Service.Mac.TempMac));
        }
    }
    else
    {
        t_cmd.Service.Mac.TempAssetId = SymContext_p->Service.Mac.TempAssetId;
    }
    t_cmd.Service.Mac.KeyAssetId = SymContext_p->Service.Mac.KeyAssetId;
    t_cmd.Service.Mac.KeySize = SymContext_p->Service.Mac.KeySize;
    if (t_cmd.Service.Mac.KeyAssetId == VAL_ASSETID_INVALID)
    {
        memcpy(t_cmd.Service.Mac.Key,
               SymContext_p->Service.Mac.Key,
               SymContext_p->Service.Mac.KeySize);
    }
    t_cmd.Service.Mac.Data_p = (const uint8_t *)Data_p;
    t_cmd.Service.Mac.DataSize = (uint32_t)DataSize;
    t_cmd.Service.Mac.TotalMessageLength = 0;

    // Initialize result part
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;
    if (SymContext_p->fUseTokenTemp)
    {
        ZEROINIT(t_res.Service.Mac.Mac);
    }

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if (funcres == VAL_SUCCESS)
    {
        // Check for errors
        if (t_res.Result == VEXTOKEN_RESULT_NO_ERROR)
        {
            SymContext_p->Service.Mac.TotalMessageLength += (uint64_t)DataSize;

            if (SymContext_p->fUseTokenTemp)
            {
                // Save the intermediate MAC for the next operation
                memcpy(SymContext_p->Service.Mac.Mac, t_res.Service.Mac.Mac,
                       sizeof(SymContext_p->Service.Mac.Mac));
            }

            // Mark initialization done
            SymContext_p->fInitDone = true;
        }
        else
        {
            funcres = (ValStatus_t)t_res.Result;
            LOG_WARN("Abort - %s()=%d\n", __func__, t_res.Result);
        }
    }

    return funcres;
}


/*----------------------------------------------------------------------------
 * val_SymMacGenerate
 */
ValStatus_t
val_SymMacGenerate(
        ValSymContextPtr_t const Context_p,
        ValOctetsIn_t * const Data_p,
        const ValSize_t DataSize,
        ValOctetsOut_t * const Mac_p,
        ValSize_t * const MacSize_p)
{
    ValSymContext_t * SymContext_p = (ValSymContext_t *)Context_p;
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;
    uint32_t MacNBytes = 0;

#ifdef VAL_STRICT_ARGS
    if ((SymContext_p == NULL) ||
        (SymContext_p->MagicBegin != VALMARKER_SYMCONTEXT) ||
        ((SymContext_p->MagicBegin ^ SymContext_p->MagicEnd) != 0) ||
        (Mac_p == NULL) ||
        (MacSize_p == NULL) ||
        ((Data_p == NULL) && (DataSize != 0)))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    ZEROINIT(t_cmd);

    // Check and set MAC algorithm and TotalMessageLength
    switch (SymContext_p->Algorithm)
    {
    case VAL_SYM_ALGO_MAC_HMAC_SHA1:
        t_cmd.Service.Mac.Algorithm = VEXTOKEN_ALGO_MAC_HMAC_SHA1;
        t_cmd.Service.Mac.TotalMessageLength =
            SymContext_p->Service.Mac.TotalMessageLength + (uint64_t)DataSize;
        MacNBytes = (160 / 8);
        break;

    case VAL_SYM_ALGO_MAC_HMAC_SHA224:
        t_cmd.Service.Mac.Algorithm = VEXTOKEN_ALGO_MAC_HMAC_SHA224;
        t_cmd.Service.Mac.TotalMessageLength =
            SymContext_p->Service.Mac.TotalMessageLength + (uint64_t)DataSize;
        MacNBytes = (224 / 8);
        break;

    case VAL_SYM_ALGO_MAC_HMAC_SHA256:
        t_cmd.Service.Mac.Algorithm = VEXTOKEN_ALGO_MAC_HMAC_SHA256;
        t_cmd.Service.Mac.TotalMessageLength =
            SymContext_p->Service.Mac.TotalMessageLength + (uint64_t)DataSize;
        MacNBytes = (256 / 8);
        break;

#ifndef VAL_REMOVE_SYM_ALGO_SHA512
    case VAL_SYM_ALGO_MAC_HMAC_SHA384:
        t_cmd.Service.Mac.Algorithm = VEXTOKEN_ALGO_MAC_HMAC_SHA384;
        t_cmd.Service.Mac.TotalMessageLength =
            SymContext_p->Service.Mac.TotalMessageLength + (uint64_t)DataSize;
        MacNBytes = (384 / 8);
        break;

    case VAL_SYM_ALGO_MAC_HMAC_SHA512:
        t_cmd.Service.Mac.Algorithm = VEXTOKEN_ALGO_MAC_HMAC_SHA512;
        t_cmd.Service.Mac.TotalMessageLength =
            SymContext_p->Service.Mac.TotalMessageLength + (uint64_t)DataSize;
        MacNBytes = (512 / 8);
        break;
#endif

    case VAL_SYM_ALGO_MAC_AES_CMAC:
        // TotalMessageLength and last block padding are handled in VEX part
        t_cmd.Service.Mac.Algorithm = VEXTOKEN_ALGO_MAC_AES_CMAC;
        MacNBytes = VAL_SYM_ALGO_AES_BLOCK_SIZE;
        break;

    case VAL_SYM_ALGO_MAC_AES_CBC_MAC:
        // TotalMessageLength and last block padding are handled in VEX part
        t_cmd.Service.Mac.Algorithm = VEXTOKEN_ALGO_MAC_AES_CBC_MAC;
        MacNBytes = VAL_SYM_ALGO_AES_BLOCK_SIZE;
        break;

#ifndef VAL_REMOVE_SYM_ALGO_ARIA
    case VAL_SYM_ALGO_MAC_ARIA_CMAC:
        // TotalMessageLength and last block padding are handled in VEX part
        t_cmd.Service.Mac.Algorithm = VEXTOKEN_ALGO_MAC_ARIA_CMAC;
        MacNBytes = VAL_SYM_ALGO_ARIA_BLOCK_SIZE;
        break;

    case VAL_SYM_ALGO_MAC_ARIA_CBC_MAC:
        // TotalMessageLength and last block padding are handled in VEX part
        t_cmd.Service.Mac.Algorithm = VEXTOKEN_ALGO_MAC_ARIA_CBC_MAC;
        MacNBytes = VAL_SYM_ALGO_ARIA_BLOCK_SIZE;
        break;
#endif

#ifndef VAL_REMOVE_SYM_ALGO_POLY1305
    case VAL_SYM_ALGO_MAC_POLY1305:
        // TotalMessageLength is not used
        t_cmd.Service.Mac.Algorithm = VEXTOKEN_ALGO_MAC_POLY1305;
        MacNBytes = 16;
        break;
#endif

    default:
        return VAL_INVALID_ALGORITHM;
    }

    if (MacNBytes > *MacSize_p)
    {
        // Final digest does not fit in buffer
        *MacSize_p = MacNBytes;
        return VAL_BUFFER_TOO_SMALL;
    }

    // Set service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_MAC;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_MACGENERATE;

    // Set MAC mode and temporary Asset or intermediate MAC
    if (SymContext_p->fInitDone)
    {
        t_cmd.Service.Mac.Mode = VEXTOKEN_MODE_HASH_MAC_CONT2FINAL;
        if (SymContext_p->fUseTokenTemp)
        {
            t_cmd.Service.Mac.TempAssetId = VAL_ASSETID_INVALID;
            memcpy(t_cmd.Service.Mac.TempMac, SymContext_p->Service.Mac.Mac,
                   sizeof(t_cmd.Service.Mac.TempMac));
        }
        else
        {
            t_cmd.Service.Mac.TempAssetId = SymContext_p->Service.Mac.TempAssetId;
        }
    }
    else
    {
        t_cmd.Service.Mac.Mode = VEXTOKEN_MODE_HASH_MAC_INIT2FINAL;
        t_cmd.Service.Mac.TempAssetId = VAL_ASSETID_INVALID;
    }

    // Set other relevant information
    t_cmd.Service.Mac.KeyAssetId = SymContext_p->Service.Mac.KeyAssetId;
    t_cmd.Service.Mac.KeySize = SymContext_p->Service.Mac.KeySize;
    if (t_cmd.Service.Mac.KeyAssetId == VAL_ASSETID_INVALID)
    {
        memcpy(t_cmd.Service.Mac.Key,
               SymContext_p->Service.Mac.Key,
               SymContext_p->Service.Mac.KeySize);
    }
    t_cmd.Service.Mac.Data_p = (const uint8_t *)Data_p;
    t_cmd.Service.Mac.DataSize = (uint32_t)DataSize;

    // Initialize result part
    ZEROINIT(t_res);

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if (funcres == VAL_SUCCESS)
    {
        // Check for errors
        if (t_res.Result == VEXTOKEN_RESULT_NO_ERROR)
        {
            // Copy generated MAC value
            memcpy(Mac_p, t_res.Service.Mac.Mac, MacNBytes);
            *MacSize_p = MacNBytes;

            // Release symmetric crypto context
            (void)val_SymRelease(Context_p);
        }
        else
        {
            funcres = (ValStatus_t)t_res.Result;
            LOG_WARN("Abort - %s()=%d\n", __func__, t_res.Result);
        }
    }

    return funcres;
}


/*----------------------------------------------------------------------------
 * val_SymMacVerify
 */
ValStatus_t
val_SymMacVerify(
        ValSymContextPtr_t const Context_p,
        ValOctetsIn_t * const Data_p,
        const ValSize_t DataSize,
        ValAssetId_Optional_t const MacAssetId,
        ValOctetsIn_Optional_t * const Mac_p,
        const ValSize_t MacSize)
{
    ValSymContext_t * SymContext_p = (ValSymContext_t *)Context_p;
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;

#ifdef VAL_STRICT_ARGS
    if ((SymContext_p == NULL) ||
        (SymContext_p->MagicBegin != VALMARKER_SYMCONTEXT) ||
        ((SymContext_p->MagicBegin ^ SymContext_p->MagicEnd) != 0) ||
        ((MacAssetId == VAL_ASSETID_INVALID) && (Mac_p == NULL)) ||
        ((Data_p == NULL) && (DataSize != 0)))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_MAC;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_MACVERIFY;
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;

    // Set MAC algorithm and TotalMessageLength
    switch (SymContext_p->Algorithm)
    {
    case VAL_SYM_ALGO_MAC_HMAC_SHA1:
        t_cmd.Service.Mac.Algorithm = VEXTOKEN_ALGO_MAC_HMAC_SHA1;
        t_cmd.Service.Mac.TotalMessageLength =
            SymContext_p->Service.Mac.TotalMessageLength + (uint64_t)DataSize;
        break;

    case VAL_SYM_ALGO_MAC_HMAC_SHA224:
        t_cmd.Service.Mac.Algorithm = VEXTOKEN_ALGO_MAC_HMAC_SHA224;
        t_cmd.Service.Mac.TotalMessageLength =
            SymContext_p->Service.Mac.TotalMessageLength + (uint64_t)DataSize;
        break;

    case VAL_SYM_ALGO_MAC_HMAC_SHA256:
        t_cmd.Service.Mac.Algorithm = VEXTOKEN_ALGO_MAC_HMAC_SHA256;
        t_cmd.Service.Mac.TotalMessageLength =
            SymContext_p->Service.Mac.TotalMessageLength + (uint64_t)DataSize;
        break;

#ifndef VAL_REMOVE_SYM_ALGO_SHA512
    case VAL_SYM_ALGO_MAC_HMAC_SHA384:
        t_cmd.Service.Mac.Algorithm = VEXTOKEN_ALGO_MAC_HMAC_SHA384;
        t_cmd.Service.Mac.TotalMessageLength =
            SymContext_p->Service.Mac.TotalMessageLength + (uint64_t)DataSize;
        break;

    case VAL_SYM_ALGO_MAC_HMAC_SHA512:
        t_cmd.Service.Mac.Algorithm = VEXTOKEN_ALGO_MAC_HMAC_SHA512;
        t_cmd.Service.Mac.TotalMessageLength =
            SymContext_p->Service.Mac.TotalMessageLength + (uint64_t)DataSize;
        break;
#endif

    case VAL_SYM_ALGO_MAC_AES_CMAC:
        // TotalMessageLength and last block padding are handled in VEX part
        t_cmd.Service.Mac.Algorithm = VEXTOKEN_ALGO_MAC_AES_CMAC;
        break;

    case VAL_SYM_ALGO_MAC_AES_CBC_MAC:
        // TotalMessageLength and last block padding are handled in VEX part
        t_cmd.Service.Mac.Algorithm = VEXTOKEN_ALGO_MAC_AES_CBC_MAC;
        break;

#ifndef VAL_REMOVE_SYM_ALGO_ARIA
    case VAL_SYM_ALGO_MAC_ARIA_CMAC:
        // TotalMessageLength and last block padding are handled in VEX part
        t_cmd.Service.Mac.Algorithm = VEXTOKEN_ALGO_MAC_ARIA_CMAC;
        break;

    case VAL_SYM_ALGO_MAC_ARIA_CBC_MAC:
        // TotalMessageLength and last block padding are handled in VEX part
        t_cmd.Service.Mac.Algorithm = VEXTOKEN_ALGO_MAC_ARIA_CBC_MAC;
        break;
#endif

#ifndef VAL_REMOVE_SYM_ALGO_POLY1305
    case VAL_SYM_ALGO_MAC_POLY1305:
        // TotalMessageLength is not used
        t_cmd.Service.Mac.Algorithm = VEXTOKEN_ALGO_MAC_POLY1305;
        break;
#endif

    default:
        return VAL_INVALID_ALGORITHM;
    }

    // Set MAC mode and temporary Asset or intermediate MAC
    if (SymContext_p->fInitDone)
    {
        t_cmd.Service.Mac.Mode = VEXTOKEN_MODE_HASH_MAC_CONT2FINAL;
        if (SymContext_p->fUseTokenTemp)
        {
            t_cmd.Service.Mac.TempAssetId = VAL_ASSETID_INVALID;
            memcpy(t_cmd.Service.Mac.TempMac, SymContext_p->Service.Mac.Mac,
                   sizeof(t_cmd.Service.Mac.TempMac));
        }
        else
        {
            t_cmd.Service.Mac.TempAssetId = SymContext_p->Service.Mac.TempAssetId;
        }
    }
    else
    {
        t_cmd.Service.Mac.Mode = VEXTOKEN_MODE_HASH_MAC_INIT2FINAL;
        t_cmd.Service.Mac.TempAssetId = VAL_ASSETID_INVALID;
    }

    // Set Key, Data and final MAC (Asset)
    t_cmd.Service.Mac.KeyAssetId = SymContext_p->Service.Mac.KeyAssetId;
    t_cmd.Service.Mac.KeySize = SymContext_p->Service.Mac.KeySize;
    if (t_cmd.Service.Mac.KeyAssetId == VAL_ASSETID_INVALID)
    {
        memcpy(t_cmd.Service.Mac.Key,
               SymContext_p->Service.Mac.Key,
               SymContext_p->Service.Mac.KeySize);
    }
    t_cmd.Service.Mac.Data_p = (const uint8_t *)Data_p;
    t_cmd.Service.Mac.DataSize = (uint32_t)DataSize;
    t_cmd.Service.Mac.MacAssetId = MacAssetId;
    if (MacAssetId == VAL_ASSETID_INVALID)
    {
        memcpy(t_cmd.Service.Mac.Mac, Mac_p, MacSize);
        t_cmd.Service.Mac.MacSize = (uint32_t)MacSize;
    }

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if (funcres == VAL_SUCCESS)
    {
        // Check for errors
        funcres = (ValStatus_t)t_res.Result;
        if ((t_res.Result == VEXTOKEN_RESULT_NO_ERROR) ||
            (t_res.Result == VEXTOKEN_RESULT_SEQ_VERIFY_ERROR))
        {
            // Release symmetric crypto context
            (void)val_SymRelease(Context_p);
        }
        else
        {
            LOG_WARN("Abort - %s()=%d\n", __func__, t_res.Result);
        }
    }

    return funcres;
}


#endif /* !VAL_REMOVE_SYM_MAC */

/* end of file adapter_val_sym_mac.c */
