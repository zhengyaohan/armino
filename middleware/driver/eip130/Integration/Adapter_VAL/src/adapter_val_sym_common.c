/* adapter_val_sym_common.c
 *
 * Implementation of the VAL API.
 *
 * This file implements the symmetric crypto common functions.
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

#if !defined(VAL_REMOVE_SYM_HASH) || \
    !defined(VAL_REMOVE_SYM_MAC) || \
    !defined(VAL_REMOVE_SYM_CIPHER)

#include "basic_defs.h"
#include "clib.h"
#include "log.h"

#include "api_val_sym.h"                // the API to implement
#include "api_val_asset.h"              // Asset Management related information
#include "api_val_system.h"             // val_IsAccessSecure()
#include "adapter_val_internal.h"       // val_ExchangeToken()
#include "adapter_vex.h"                // VexToken_Command_t, VexToken_Result_t


/*----------------------------------------------------------------------------
 * val_SymAlloc
 */
ValStatus_t
val_SymAlloc(
        ValSymAlgo_t Algorithm,
        ValSymMode_t Mode,
        bool fUseTokenTemp,
        ValSymContextPtr_t * Context_pp)
{
    ValSymContext_t * SymContext_p;

    if (Context_pp == NULL)
    {
        return VAL_BAD_ARGUMENT;
    }

    // Perform initialization based on specified algorithm
    switch (Algorithm)
    {
        // Hash algorithms
    case VAL_SYM_ALGO_HASH_SHA1:
    case VAL_SYM_ALGO_HASH_SHA224:
    case VAL_SYM_ALGO_HASH_SHA256:
#ifndef VAL_REMOVE_SYM_ALGO_SHA512
    case VAL_SYM_ALGO_HASH_SHA384:
    case VAL_SYM_ALGO_HASH_SHA512:
#endif
        Mode = VAL_SYM_MODE_NONE;      // Force mode
        break;

        // MAC algorithms
    case VAL_SYM_ALGO_MAC_HMAC_SHA1:
    case VAL_SYM_ALGO_MAC_HMAC_SHA224:
    case VAL_SYM_ALGO_MAC_HMAC_SHA256:
#ifndef VAL_REMOVE_SYM_ALGO_SHA512
    case VAL_SYM_ALGO_MAC_HMAC_SHA384:
    case VAL_SYM_ALGO_MAC_HMAC_SHA512:
#endif
    case VAL_SYM_ALGO_MAC_AES_CMAC:
    case VAL_SYM_ALGO_MAC_AES_CBC_MAC:
#ifndef VAL_REMOVE_SYM_ALGO_ARIA
    case VAL_SYM_ALGO_MAC_ARIA_CMAC:
    case VAL_SYM_ALGO_MAC_ARIA_CBC_MAC:
#endif
#ifndef VAL_REMOVE_SYM_ALGO_POLY1305
    case VAL_SYM_ALGO_MAC_POLY1305:
#endif
        Mode = VAL_SYM_MODE_NONE;      // Force mode
        break;

        // (Block)Cipher algorithms
    case VAL_SYM_ALGO_CIPHER_AES:
        switch (Mode)
        {
        case VAL_SYM_MODE_CIPHER_ECB:
            fUseTokenTemp = true;       /* Skip temp intermediate IV asset part */
        case VAL_SYM_MODE_CIPHER_CBC:
        case VAL_SYM_MODE_CIPHER_CTR:
        case VAL_SYM_MODE_CIPHER_ICM:
#ifndef VAL_REMOVE_SYM_ALGO_AES_F8
        case VAL_SYM_MODE_CIPHER_F8:
#endif
#ifndef VAL_REMOVE_SYM_ALGO_AES_CCM
        case VAL_SYM_MODE_CIPHER_CCM:
#endif
#ifndef VAL_REMOVE_SYM_ALGO_AES_XTS
        case VAL_SYM_MODE_CIPHER_XTS:
#endif
#ifndef VAL_REMOVE_SYM_ALGO_AES_GCM
        case VAL_SYM_MODE_CIPHER_GCM:
#endif
            break;

        default:
            return VAL_INVALID_MODE;
        }
        break;
#ifndef VAL_REMOVE_SYM_ALGO_DES
    case VAL_SYM_ALGO_CIPHER_DES:
#endif
#ifndef VAL_REMOVE_SYM_ALGO_3DES
    case VAL_SYM_ALGO_CIPHER_TRIPLE_DES:
#endif
#if !defined(VAL_REMOVE_SYM_ALGO_DES) || !defined(VAL_REMOVE_SYM_ALGO_3DES)
#ifndef VAL_REMOVE_SYM_ALGO_DES
        if (Algorithm == VAL_SYM_ALGO_CIPHER_DES)
        {
            fUseTokenTemp = true;       /* Skip temp intermediate IV asset part */
        }
#endif
        switch (Mode)
        {
        case VAL_SYM_MODE_CIPHER_ECB:
            fUseTokenTemp = true;       /* Skip temp intermediate IV asset part */
        case VAL_SYM_MODE_CIPHER_CBC:
            break;

        default:
            return VAL_INVALID_MODE;
        }
        break;
#endif
#ifndef VAL_REMOVE_SYM_ALGO_CHACHA20
    case VAL_SYM_ALGO_CIPHER_CHACHA20:
        switch (Mode)
        {
        case VAL_SYM_MODE_CIPHER_CHACHA20_ENC:
        case VAL_SYM_MODE_CIPHER_CHACHA20_AEAD:
            break;

        default:
            return VAL_INVALID_MODE;
        }
        break;
#endif
#ifndef VAL_REMOVE_SYM_ALGO_SM4
    case VAL_SYM_ALGO_CIPHER_SM4:
        switch (Mode)
        {
        case VAL_SYM_MODE_CIPHER_ECB:
            fUseTokenTemp = true;       /* Skip temp intermediate IV asset part */
        case VAL_SYM_MODE_CIPHER_CBC:
        case VAL_SYM_MODE_CIPHER_CTR:
            break;

        default:
            return VAL_INVALID_MODE;
        }
        break;
#endif
#ifndef VAL_REMOVE_SYM_ALGO_ARIA
    case VAL_SYM_ALGO_CIPHER_ARIA:
        switch (Mode)
        {
        case VAL_SYM_MODE_CIPHER_ECB:
            fUseTokenTemp = true;       /* Skip temp intermediate IV asset part */
        case VAL_SYM_MODE_CIPHER_CBC:
        case VAL_SYM_MODE_CIPHER_CTR:
        case VAL_SYM_MODE_CIPHER_ICM:
#ifndef VAL_REMOVE_SYM_ALGO_ARIA_F8
        case VAL_SYM_MODE_CIPHER_F8:
#endif
#ifndef VAL_REMOVE_SYM_ALGO_ARIA_CCM
        case VAL_SYM_MODE_CIPHER_CCM:
#endif
#ifndef VAL_REMOVE_SYM_ALGO_ARIA_XTS
        case VAL_SYM_MODE_CIPHER_XTS:
#endif
#ifndef VAL_REMOVE_SYM_ALGO_ARIA_GCM
        case VAL_SYM_MODE_CIPHER_GCM:
#endif
            break;

        default:
            return VAL_INVALID_MODE;
        }
        break;
#endif
    default:
        return VAL_INVALID_ALGORITHM;
    }

    // Allocate the symmetric crypto context
    SymContext_p = Adapter_Alloc(sizeof(ValSymContext_t));
    if (SymContext_p == NULL)
    {
        return VAL_NO_MEMORY;
    }

    // Initialize the symmetric crypto context
    memset(SymContext_p, 0, sizeof(ValSymContext_t));
    SymContext_p->Algorithm = Algorithm;
    SymContext_p->Mode = Mode;
    SymContext_p->fUseTokenTemp = fUseTokenTemp;
#ifndef VAL_REMOVE_SYM_ALGO_CHACHA20
    if (Algorithm == VAL_SYM_ALGO_CIPHER_CHACHA20)
    {
        SymContext_p->Service.Cipher.NonceLength = 12;
    }
#endif
    SymContext_p->MagicBegin = VALMARKER_SYMCONTEXT;
    SymContext_p->MagicEnd = VALMARKER_SYMCONTEXT;
    *Context_pp = (ValSymContextPtr_t)SymContext_p;

    return VAL_SUCCESS;
}

/*----------------------------------------------------------------------------
 * val_SymInitKey
 */
ValStatus_t
val_SymInitKey(
        ValSymContextPtr_t const Context_p,
        ValAssetId_Optional_t KeyAssetId,
        ValOctetsIn_Optional_t * const Key_p,
        const ValSize_t KeySize)
{
    ValSymContext_t * SymContext_p = (ValSymContext_t *)Context_p;
    uint8_t * ContextKey_p = NULL;

#ifdef VAL_STRICT_ARGS
    if ((SymContext_p == NULL) ||
        (SymContext_p->MagicBegin != VALMARKER_SYMCONTEXT) ||
        ((SymContext_p->MagicBegin ^ SymContext_p->MagicEnd) != 0))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Perform key initialization based on algorithm
    switch (SymContext_p->Algorithm)
    {
    case VAL_SYM_ALGO_MAC_HMAC_SHA1:
    case VAL_SYM_ALGO_MAC_HMAC_SHA224:
    case VAL_SYM_ALGO_MAC_HMAC_SHA256:
#ifndef VAL_REMOVE_SYM_ALGO_SHA512
    case VAL_SYM_ALGO_MAC_HMAC_SHA384:
    case VAL_SYM_ALGO_MAC_HMAC_SHA512:
#endif
        if (KeyAssetId != VAL_ASSETID_INVALID)
        {
            SymContext_p->Service.Mac.KeyAssetId = KeyAssetId;
            SymContext_p->Service.Mac.KeySize = 0; // Size is taken from the Asset
            return VAL_SUCCESS;
        }
        if (Key_p == NULL)
        {
            if (KeySize != 0)
            {
                return VAL_INVALID_KEYSIZE;
            }
            return VAL_SUCCESS;
        }

        switch (SymContext_p->Algorithm)
        {
        default:
        case VAL_SYM_ALGO_MAC_HMAC_SHA1:
        case VAL_SYM_ALGO_MAC_HMAC_SHA224:
        case VAL_SYM_ALGO_MAC_HMAC_SHA256:
            if (KeySize <= VAL_SYM_ALGO_MAX_SHA2_MAC_KEY_SIZE)
            {
                ContextKey_p = SymContext_p->Service.Mac.Key;
                SymContext_p->Service.Mac.KeySize = (uint32_t)KeySize;
                goto func_return;
            }
            break;

#ifndef VAL_REMOVE_SYM_ALGO_SHA512
        case VAL_SYM_ALGO_MAC_HMAC_SHA384:
        case VAL_SYM_ALGO_MAC_HMAC_SHA512:
            if (KeySize <= VAL_SYM_ALGO_MAX_SHA512_MAC_KEY_SIZE)
            {
                ContextKey_p = SymContext_p->Service.Mac.Key;
                SymContext_p->Service.Mac.KeySize = (uint32_t)KeySize;
                goto func_return;
            }
            break;
#endif
        }

        // Hash key
        {
            ValSymContextPtr_t TmpContext_p = NULL;
            ValSymAlgo_t Algorithm;
            ValStatus_t Status;

            switch (SymContext_p->Algorithm)
            {
            case VAL_SYM_ALGO_MAC_HMAC_SHA1:
                Algorithm = VAL_SYM_ALGO_HASH_SHA1;
                break;
            case VAL_SYM_ALGO_MAC_HMAC_SHA224:
                Algorithm = VAL_SYM_ALGO_HASH_SHA224;
                break;
            default:
            case VAL_SYM_ALGO_MAC_HMAC_SHA256:
                Algorithm = VAL_SYM_ALGO_HASH_SHA256;
                break;
#ifndef VAL_REMOVE_SYM_ALGO_SHA512
            case VAL_SYM_ALGO_MAC_HMAC_SHA384:
                Algorithm = VAL_SYM_ALGO_HASH_SHA384;
                break;
            case VAL_SYM_ALGO_MAC_HMAC_SHA512:
                Algorithm = VAL_SYM_ALGO_HASH_SHA512;
                break;
#endif
            }

            Status = val_SymAlloc(Algorithm, VAL_SYM_MODE_NONE, false, &TmpContext_p);
            if  (Status == VAL_SUCCESS)
            {
                uint8_t * CopyKey_p = Adapter_Alloc((unsigned int)KeySize);
                if (CopyKey_p != NULL)
                {
                    ValSize_t TmpKeySize;

                    memcpy(CopyKey_p, Key_p, KeySize);
                    TmpKeySize = sizeof(SymContext_p->Service.Mac.Key);
                    Status = val_SymHashFinal(TmpContext_p,
                                              CopyKey_p,
                                              KeySize,
                                              SymContext_p->Service.Mac.Key,
                                              &TmpKeySize);
                    Adapter_Free(CopyKey_p);
                    if  (Status == VAL_SUCCESS)
                    {
                        SymContext_p->Service.Mac.KeySize = (uint32_t)TmpKeySize;
                        return VAL_SUCCESS;
                    }
                }

                (void)val_SymRelease(TmpContext_p);
            }
            return VAL_INVALID_KEYSIZE;
        }
        break;

    case VAL_SYM_ALGO_MAC_AES_CMAC:
    case VAL_SYM_ALGO_MAC_AES_CBC_MAC:
#ifndef VAL_REMOVE_SYM_ALGO_ARIA
    case VAL_SYM_ALGO_MAC_ARIA_CMAC:
    case VAL_SYM_ALGO_MAC_ARIA_CBC_MAC:
#endif
        if (KeyAssetId != VAL_ASSETID_INVALID)
        {
            SymContext_p->Service.Mac.KeyAssetId = KeyAssetId;
            SymContext_p->Service.Mac.KeySize = 0; // Size is taken from the Asset
            return VAL_SUCCESS;
        }
        if ((KeySize != (128/8)) &&
            (KeySize != (192/8)) &&
            (KeySize != (256/8)))
        {
            return VAL_INVALID_KEYSIZE;
        }
        ContextKey_p = SymContext_p->Service.Mac.Key;
        SymContext_p->Service.Mac.KeySize = (uint32_t)KeySize;
        break;

#ifndef VAL_REMOVE_SYM_ALGO_POLY1305
    case VAL_SYM_ALGO_MAC_POLY1305:
        if (KeySize != (256/8))
        {
            return VAL_INVALID_KEYSIZE;
        }
        if (KeyAssetId != VAL_ASSETID_INVALID)
        {
            SymContext_p->Service.Mac.KeyAssetId = KeyAssetId;
            SymContext_p->Service.Mac.KeySize = (uint32_t)KeySize;
            return VAL_SUCCESS;
        }
        ContextKey_p = SymContext_p->Service.Mac.Key;
        SymContext_p->Service.Mac.KeySize = (uint32_t)KeySize;
        break;
#endif

    case VAL_SYM_ALGO_CIPHER_AES:
        switch (SymContext_p->Mode)
        {
#ifndef VAL_REMOVE_SYM_ALGO_AES_XTS
        case VAL_SYM_MODE_CIPHER_XTS:
            if ((KeySize != (256/8)) && // AES-128
                (KeySize != (512/8)))   // AES-256
            {
                return VAL_INVALID_KEYSIZE;
            }
            break;
#endif
#ifndef VAL_REMOVE_SYM_ALGO_AES_F8
        case VAL_SYM_MODE_CIPHER_F8:
            if (KeySize != (128/8))     // AES-128 only
            {
                return VAL_INVALID_KEYSIZE;
            }
            break;
#endif
        default:
            if ((KeySize != (128/8)) &&
                (KeySize != (192/8)) &&
                (KeySize != (256/8)))
            {
                return VAL_INVALID_KEYSIZE;
            }
            break;
        }
        if (KeyAssetId != VAL_ASSETID_INVALID)
        {
            SymContext_p->Service.Cipher.KeyAssetId = KeyAssetId;
            SymContext_p->Service.Cipher.KeySize = (uint32_t)KeySize;
            return VAL_SUCCESS;
        }
        ContextKey_p = SymContext_p->Service.Cipher.Key;
        SymContext_p->Service.Cipher.KeySize = (uint32_t)KeySize;
        break;

#ifndef VAL_REMOVE_SYM_ALGO_DES
    case VAL_SYM_ALGO_CIPHER_DES:
        if (KeySize != (64/8))
        {
            return VAL_INVALID_KEYSIZE;
        }
        if (KeyAssetId != VAL_ASSETID_INVALID)
        {
            return VAL_BAD_ARGUMENT;
        }
        ContextKey_p = SymContext_p->Service.Cipher.Key;
        SymContext_p->Service.Cipher.KeySize = (uint32_t)KeySize;
        break;
#endif

#ifndef VAL_REMOVE_SYM_ALGO_3DES
    case VAL_SYM_ALGO_CIPHER_TRIPLE_DES:
        if (KeySize != (3*(64/8)))
        {
            return VAL_INVALID_KEYSIZE;
        }
        if (KeyAssetId != VAL_ASSETID_INVALID)
        {
            SymContext_p->Service.Cipher.KeyAssetId = KeyAssetId;
            SymContext_p->Service.Cipher.KeySize = (uint32_t)KeySize;
            return VAL_SUCCESS;
        }
        ContextKey_p = SymContext_p->Service.Cipher.Key;
        SymContext_p->Service.Cipher.KeySize = (uint32_t)KeySize;
        break;
#endif

#ifndef VAL_REMOVE_SYM_ALGO_CHACHA20
    case VAL_SYM_ALGO_CIPHER_CHACHA20:
        switch (SymContext_p->Mode)
        {
        case VAL_SYM_MODE_CIPHER_CHACHA20_AEAD:
            if (KeySize != (256/8))
            {
                return VAL_INVALID_KEYSIZE;
            }
            break;

        default:
            if ((KeySize != (128/8)) &&
                (KeySize != (256/8)))
            {
                return VAL_INVALID_KEYSIZE;
            }
            break;
        }
        if (KeyAssetId != VAL_ASSETID_INVALID)
        {
            SymContext_p->Service.Cipher.KeyAssetId = KeyAssetId;
            SymContext_p->Service.Cipher.KeySize = (uint32_t)KeySize;
            return VAL_SUCCESS;
        }
        ContextKey_p = SymContext_p->Service.Cipher.Key;
        SymContext_p->Service.Cipher.KeySize = (uint32_t)KeySize;
        break;
#endif

#ifndef VAL_REMOVE_SYM_ALGO_SM4
    case VAL_SYM_ALGO_CIPHER_SM4:
        if (KeySize != (128/8))
        {
            return VAL_INVALID_KEYSIZE;
        }
        if (KeyAssetId != VAL_ASSETID_INVALID)
        {
            SymContext_p->Service.Cipher.KeyAssetId = KeyAssetId;
            SymContext_p->Service.Cipher.KeySize = (uint32_t)KeySize;
            return VAL_SUCCESS;
        }
        ContextKey_p = SymContext_p->Service.Cipher.Key;
        SymContext_p->Service.Cipher.KeySize = (uint32_t)KeySize;
        break;
#endif

#ifndef VAL_REMOVE_SYM_ALGO_ARIA
    case VAL_SYM_ALGO_CIPHER_ARIA:
        switch (SymContext_p->Mode)
        {
#ifndef VAL_REMOVE_SYM_ALGO_ARIA_XTS
        case VAL_SYM_MODE_CIPHER_XTS:
            if ((KeySize != (256/8)) && // AES-128
                (KeySize != (512/8)))   // AES-256
            {
                return VAL_INVALID_KEYSIZE;
            }
            break;
#endif
#ifndef VAL_REMOVE_SYM_ALGO_ARIA_F8
        case VAL_SYM_MODE_CIPHER_F8:
            if (KeySize != (128/8))     // AES-128 only
            {
                return VAL_INVALID_KEYSIZE;
            }
            break;
#endif
        default:
            if ((KeySize != (128/8)) &&
                (KeySize != (192/8)) &&
                (KeySize != (256/8)))
            {
                return VAL_INVALID_KEYSIZE;
            }
            break;
        }
        if (KeyAssetId != VAL_ASSETID_INVALID)
        {
            SymContext_p->Service.Cipher.KeyAssetId = KeyAssetId;
            SymContext_p->Service.Cipher.KeySize = (uint32_t)KeySize;
            return VAL_SUCCESS;
        }
        ContextKey_p = SymContext_p->Service.Cipher.Key;
        SymContext_p->Service.Cipher.KeySize = (uint32_t)KeySize;
        break;
#endif

    default:
        return VAL_UNSUPPORTED;
    }

func_return:
    if ((Key_p != NULL) && (ContextKey_p != NULL))
    {
        memcpy(ContextKey_p, Key_p, KeySize);
        return VAL_SUCCESS;
    }

    return VAL_BAD_ARGUMENT;
}


/*----------------------------------------------------------------------------
 * val_SymInitIV
 */
ValStatus_t
val_SymInitIV(
        ValSymContextPtr_t const Context_p,
        ValOctetsIn_t * const IV_p,
        const ValSize_t IVSize)
{
    ValSymContext_t * SymContext_p = (ValSymContext_t *)Context_p;
    uint8_t * ContextIV_p = NULL;
    ValSize_t CheckIVSize = 0;

#ifdef VAL_STRICT_ARGS
    if ((SymContext_p == NULL) ||
        (SymContext_p->MagicBegin != VALMARKER_SYMCONTEXT) ||
        ((SymContext_p->MagicBegin ^ SymContext_p->MagicEnd) != 0) ||
        (IV_p == NULL))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Perform key initialization based on algorithm
    switch (SymContext_p->Algorithm)
    {
    case VAL_SYM_ALGO_MAC_AES_CBC_MAC:
        ContextIV_p = SymContext_p->Service.Mac.Mac;
        CheckIVSize = VAL_SYM_ALGO_AES_IV_SIZE;
        break;

#ifndef VAL_REMOVE_SYM_ALGO_ARIA
    case VAL_SYM_ALGO_MAC_ARIA_CBC_MAC:
        ContextIV_p = SymContext_p->Service.Mac.Mac;
        CheckIVSize = VAL_SYM_ALGO_ARIA_IV_SIZE;
        break;
#endif

    case VAL_SYM_ALGO_CIPHER_AES:
        switch (SymContext_p->Mode)
        {
        case VAL_SYM_MODE_CIPHER_CBC:
        case VAL_SYM_MODE_CIPHER_CTR:
        case VAL_SYM_MODE_CIPHER_ICM:
#ifndef VAL_REMOVE_SYM_ALGO_AES_F8
        case VAL_SYM_MODE_CIPHER_F8:
#endif
#ifndef VAL_REMOVE_SYM_ALGO_AES_XTS
        case VAL_SYM_MODE_CIPHER_XTS:
#endif
            ContextIV_p = SymContext_p->Service.Cipher.IV;
            break;

#ifndef VAL_REMOVE_SYM_ALGO_AES_GCM
        case VAL_SYM_MODE_CIPHER_GCM:
#endif
            ContextIV_p = SymContext_p->Service.CipherAE.IV;
            break;

        default:
            return VAL_UNSUPPORTED;
        }
        CheckIVSize = VAL_SYM_ALGO_AES_IV_SIZE;
        break;

#if !defined(VAL_REMOVE_SYM_ALGO_DES) || !defined(VAL_REMOVE_SYM_ALGO_3DES)
#ifndef VAL_REMOVE_SYM_ALGO_DES
    case VAL_SYM_ALGO_CIPHER_DES:
#endif
#ifndef VAL_REMOVE_SYM_ALGO_3DES
    case VAL_SYM_ALGO_CIPHER_TRIPLE_DES:
#endif
        if (SymContext_p->Mode != VAL_SYM_MODE_CIPHER_CBC)
        {
            return VAL_UNSUPPORTED;
        }
        ContextIV_p = SymContext_p->Service.Cipher.IV;
        CheckIVSize = VAL_SYM_ALGO_DES_IV_SIZE;
        break;
#endif

#ifndef VAL_REMOVE_SYM_ALGO_CHACHA20
    case VAL_SYM_ALGO_CIPHER_CHACHA20:
        switch (SymContext_p->Mode)
        {
        case VAL_SYM_MODE_CIPHER_CHACHA20_ENC:
            ContextIV_p = SymContext_p->Service.Cipher.IV;
            break;

        default:
            return VAL_UNSUPPORTED;
        }
        CheckIVSize = VAL_SYM_ALGO_CHACHA20_IV_SIZE;
        break;
#endif

#ifndef VAL_REMOVE_SYM_ALGO_SM4
    case VAL_SYM_ALGO_CIPHER_SM4:
        switch (SymContext_p->Mode)
        {
        case VAL_SYM_MODE_CIPHER_CBC:
        case VAL_SYM_MODE_CIPHER_CTR:
            ContextIV_p = SymContext_p->Service.Cipher.IV;
            break;

        default:
            return VAL_UNSUPPORTED;
        }
        CheckIVSize = VAL_SYM_ALGO_SM4_IV_SIZE;
        break;
#endif

#ifndef VAL_REMOVE_SYM_ALGO_ARIA
    case VAL_SYM_ALGO_CIPHER_ARIA:
        switch (SymContext_p->Mode)
        {
        case VAL_SYM_MODE_CIPHER_CBC:
        case VAL_SYM_MODE_CIPHER_CTR:
        case VAL_SYM_MODE_CIPHER_ICM:
#ifndef VAL_REMOVE_SYM_ALGO_ARIA_F8
        case VAL_SYM_MODE_CIPHER_F8:
#endif
#ifndef VAL_REMOVE_SYM_ALGO_ARIA_XTS
        case VAL_SYM_MODE_CIPHER_XTS:
#endif
            ContextIV_p = SymContext_p->Service.Cipher.IV;
            break;

#ifndef VAL_REMOVE_SYM_ALGO_ARIA_GCM
        case VAL_SYM_MODE_CIPHER_GCM:
#endif
            ContextIV_p = SymContext_p->Service.CipherAE.IV;
            break;

        default:
            return VAL_UNSUPPORTED;
        }
        CheckIVSize = VAL_SYM_ALGO_ARIA_IV_SIZE;
        break;
#endif

    default:
        return VAL_UNSUPPORTED;
    }

    if ((ContextIV_p != NULL) && (IV_p != NULL))
    {
        if (IVSize != CheckIVSize)
        {
            return VAL_INVALID_LENGTH;
        }

        memcpy(ContextIV_p, IV_p, IVSize);
        return VAL_SUCCESS;
    }

    return VAL_BAD_ARGUMENT;
}


/*----------------------------------------------------------------------------
 * val_SymReadTokenTemp
 */
ValStatus_t
val_SymReadTokenTemp(
        ValSymContextPtr_t const Context_p,
        ValOctetsOut_t * const Temp_p,
        ValSize_t * const TempSize_p,
        ValSize_t * const TotalMessageLength_p)
{
    ValSymContext_t * SymContext_p = (ValSymContext_t *)Context_p;
    uint8_t * ContextTemp_p = NULL;
    ValSize_t TempNBytes = 0;
    uint64_t TotalMessageLength = 0;

#ifdef VAL_STRICT_ARGS
    if ((SymContext_p == NULL) ||
        (SymContext_p->MagicBegin != VALMARKER_SYMCONTEXT) ||
        ((SymContext_p->MagicBegin ^ SymContext_p->MagicEnd) != 0) ||
        !SymContext_p->fUseTokenTemp)
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Perform algorithm based setup for token related temporary information
    switch (SymContext_p->Algorithm)
    {
    case VAL_SYM_ALGO_HASH_SHA1:
        TotalMessageLength = SymContext_p->Service.Hash.TotalMessageLength;
        ContextTemp_p = SymContext_p->Service.Hash.Digest;
        TempNBytes = (160 / 8);
        break;
    case VAL_SYM_ALGO_HASH_SHA224:
    case VAL_SYM_ALGO_HASH_SHA256:
        TotalMessageLength = SymContext_p->Service.Hash.TotalMessageLength;
        ContextTemp_p = SymContext_p->Service.Hash.Digest;
        TempNBytes = (256 / 8);
        break;
#ifndef VAL_REMOVE_SYM_ALGO_SHA512
    case VAL_SYM_ALGO_HASH_SHA384:
    case VAL_SYM_ALGO_HASH_SHA512:
        TotalMessageLength = SymContext_p->Service.Hash.TotalMessageLength;
        ContextTemp_p = SymContext_p->Service.Hash.Digest;
        TempNBytes = (512 / 8);
        break;
#endif
    case VAL_SYM_ALGO_MAC_HMAC_SHA1:
        TotalMessageLength = SymContext_p->Service.Mac.TotalMessageLength;
        ContextTemp_p = SymContext_p->Service.Mac.Mac;
        TempNBytes = (160 / 8);
        break;
    case VAL_SYM_ALGO_MAC_HMAC_SHA224:
    case VAL_SYM_ALGO_MAC_HMAC_SHA256:
        TotalMessageLength = SymContext_p->Service.Mac.TotalMessageLength;
        ContextTemp_p = SymContext_p->Service.Mac.Mac;
        TempNBytes = (256 / 8);
        break;
#ifndef VAL_REMOVE_SYM_ALGO_SHA512
    case VAL_SYM_ALGO_MAC_HMAC_SHA384:
    case VAL_SYM_ALGO_MAC_HMAC_SHA512:
        TotalMessageLength = SymContext_p->Service.Mac.TotalMessageLength;
        ContextTemp_p = SymContext_p->Service.Mac.Mac;
        TempNBytes = (512 / 8);
        break;
#endif
    case VAL_SYM_ALGO_MAC_AES_CMAC:
    case VAL_SYM_ALGO_MAC_AES_CBC_MAC:
        ContextTemp_p = SymContext_p->Service.Mac.Mac;
        TempNBytes = VAL_SYM_ALGO_AES_IV_SIZE;
        break;
#ifndef VAL_REMOVE_SYM_ALGO_ARIA
    case VAL_SYM_ALGO_MAC_ARIA_CMAC:
    case VAL_SYM_ALGO_MAC_ARIA_CBC_MAC:
        ContextTemp_p = SymContext_p->Service.Mac.Mac;
        TempNBytes = VAL_SYM_ALGO_ARIA_IV_SIZE;
        break;
#endif
#ifndef VAL_REMOVE_SYM_ALGO_POLY1305
    case VAL_SYM_ALGO_MAC_POLY1305:
        ContextTemp_p = SymContext_p->Service.Mac.Mac;
        TempNBytes = 16;
        break;
#endif

    // Token related temporary information is not supported
    default:
        return VAL_UNSUPPORTED;
    }

    // Read token related temporary information from the symmetric context
    if ((ContextTemp_p != NULL) && (Temp_p != NULL) && (TempSize_p != NULL) &&
        (!TotalMessageLength || (TotalMessageLength_p != NULL)))
    {
        if (*TempSize_p < TempNBytes)
        {
            return VAL_INVALID_LENGTH;
        }

        memcpy(Temp_p, ContextTemp_p, TempNBytes);
        *TempSize_p = TempNBytes;
        if ((TotalMessageLength_p != NULL))
        {
            *TotalMessageLength_p = (ValSize_t)TotalMessageLength;
        }

        return VAL_SUCCESS;
    }

    return VAL_BAD_ARGUMENT;
}


/*----------------------------------------------------------------------------
 * val_SymWriteTokenTemp
 */
ValStatus_t
val_SymWriteTokenTemp(
        ValSymContextPtr_t const Context_p,
        ValOctetsIn_t * const Temp_p,
        const ValSize_t TempSize,
        const ValSize_t TotalMessageLength)
{
    ValSymContext_t * SymContext_p = (ValSymContext_t *)Context_p;
    uint64_t * TotalMessageLength_p = NULL;
    uint8_t * ContextTemp_p = NULL;
    ValSize_t TempNBytes = 0;

#ifdef VAL_STRICT_ARGS
    if ((SymContext_p == NULL) ||
        (SymContext_p->MagicBegin != VALMARKER_SYMCONTEXT) ||
        ((SymContext_p->MagicBegin ^ SymContext_p->MagicEnd) != 0) ||
        !SymContext_p->fUseTokenTemp)
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Perform algorithm based setup for token related temporary information
    switch (SymContext_p->Algorithm)
    {
    case VAL_SYM_ALGO_HASH_SHA1:
        TotalMessageLength_p = &SymContext_p->Service.Hash.TotalMessageLength;
        ContextTemp_p = SymContext_p->Service.Hash.Digest;
        TempNBytes = (160 / 8);
        break;
    case VAL_SYM_ALGO_HASH_SHA224:
    case VAL_SYM_ALGO_HASH_SHA256:
        TotalMessageLength_p = &SymContext_p->Service.Hash.TotalMessageLength;
        ContextTemp_p = SymContext_p->Service.Hash.Digest;
        TempNBytes = (256 / 8);
        break;
#ifndef VAL_REMOVE_SYM_ALGO_SHA512
    case VAL_SYM_ALGO_HASH_SHA384:
    case VAL_SYM_ALGO_HASH_SHA512:
        TotalMessageLength_p = &SymContext_p->Service.Hash.TotalMessageLength;
        ContextTemp_p = SymContext_p->Service.Hash.Digest;
        TempNBytes = (512 / 8);
        break;
#endif
    case VAL_SYM_ALGO_MAC_HMAC_SHA1:
        TotalMessageLength_p = &SymContext_p->Service.Mac.TotalMessageLength;
        ContextTemp_p = SymContext_p->Service.Mac.Mac;
        TempNBytes = (160 / 8);
        break;
    case VAL_SYM_ALGO_MAC_HMAC_SHA224:
    case VAL_SYM_ALGO_MAC_HMAC_SHA256:
        TotalMessageLength_p = &SymContext_p->Service.Mac.TotalMessageLength;
        ContextTemp_p = SymContext_p->Service.Mac.Mac;
        TempNBytes = (256 / 8);
        break;
#ifndef VAL_REMOVE_SYM_ALGO_SHA512
    case VAL_SYM_ALGO_MAC_HMAC_SHA384:
    case VAL_SYM_ALGO_MAC_HMAC_SHA512:
        TotalMessageLength_p = &SymContext_p->Service.Mac.TotalMessageLength;
        ContextTemp_p = SymContext_p->Service.Mac.Mac;
        TempNBytes = (512 / 8);
        break;
#endif
    case VAL_SYM_ALGO_MAC_AES_CMAC:
    case VAL_SYM_ALGO_MAC_AES_CBC_MAC:
        ContextTemp_p = SymContext_p->Service.Mac.Mac;
        TempNBytes = VAL_SYM_ALGO_AES_IV_SIZE;
        break;
#ifndef VAL_REMOVE_SYM_ALGO_ARIA
    case VAL_SYM_ALGO_MAC_ARIA_CMAC:
    case VAL_SYM_ALGO_MAC_ARIA_CBC_MAC:
        ContextTemp_p = SymContext_p->Service.Mac.Mac;
        TempNBytes = VAL_SYM_ALGO_ARIA_IV_SIZE;
        break;
#endif
#ifndef VAL_REMOVE_SYM_ALGO_POLY1305
    case VAL_SYM_ALGO_MAC_POLY1305:
        ContextTemp_p = SymContext_p->Service.Mac.Mac;
        TempNBytes = 16;
        break;
#endif

    // Token related temporary information is not supported
    default:
        return VAL_UNSUPPORTED;
    }

    // Write token related temporary information to the symmetric context
    if ((ContextTemp_p != NULL) && (Temp_p != NULL))
    {
        if (TempSize != TempNBytes)
        {
            return VAL_INVALID_LENGTH;
        }

        memcpy(ContextTemp_p, Temp_p, TempNBytes);
        if (TotalMessageLength_p != NULL)
        {
            *TotalMessageLength_p = (uint64_t)TotalMessageLength;
        }
        SymContext_p->fInitDone = true;
        return VAL_SUCCESS;
    }

    return VAL_BAD_ARGUMENT;
}


/*----------------------------------------------------------------------------
 * val_SymRelease
 */
ValStatus_t
val_SymRelease(
        ValSymContextPtr_t const Context_p)
{
    ValSymContext_t * SymContext_p = (ValSymContext_t *)Context_p;
    ValAssetId_t TempAssetId = VAL_ASSETID_INVALID;

#ifdef VAL_STRICT_ARGS
    if ((SymContext_p == NULL) ||
        (SymContext_p->MagicBegin != VALMARKER_SYMCONTEXT) ||
        ((SymContext_p->MagicBegin ^ SymContext_p->MagicEnd) != 0))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Perform reset based on algorithm
    switch (SymContext_p->Algorithm)
    {
        // Hash algorithms
    case VAL_SYM_ALGO_HASH_SHA1:
    case VAL_SYM_ALGO_HASH_SHA224:
    case VAL_SYM_ALGO_HASH_SHA256:
#ifndef VAL_REMOVE_SYM_ALGO_SHA512
    case VAL_SYM_ALGO_HASH_SHA384:
    case VAL_SYM_ALGO_HASH_SHA512:
#endif
        TempAssetId = SymContext_p->Service.Hash.TempAssetId;
        break;

        // MAC algorithms
    case VAL_SYM_ALGO_MAC_HMAC_SHA1:
    case VAL_SYM_ALGO_MAC_HMAC_SHA224:
    case VAL_SYM_ALGO_MAC_HMAC_SHA256:
#ifndef VAL_REMOVE_SYM_ALGO_SHA512
    case VAL_SYM_ALGO_MAC_HMAC_SHA384:
    case VAL_SYM_ALGO_MAC_HMAC_SHA512:
#endif
    case VAL_SYM_ALGO_MAC_AES_CMAC:
    case VAL_SYM_ALGO_MAC_AES_CBC_MAC:
#ifndef VAL_REMOVE_SYM_ALGO_ARIA
    case VAL_SYM_ALGO_MAC_ARIA_CMAC:
    case VAL_SYM_ALGO_MAC_ARIA_CBC_MAC:
#endif
#ifndef VAL_REMOVE_SYM_ALGO_POLY1305
    case VAL_SYM_ALGO_MAC_POLY1305:
#endif
        TempAssetId = SymContext_p->Service.Mac.TempAssetId;
        break;

        // (Block)Cipher algorithms
    case VAL_SYM_ALGO_CIPHER_AES:
#ifndef VAL_REMOVE_SYM_ALGO_DES
    case VAL_SYM_ALGO_CIPHER_DES:
#endif
#ifndef VAL_REMOVE_SYM_ALGO_3DES
    case VAL_SYM_ALGO_CIPHER_TRIPLE_DES:
#endif
#ifndef VAL_REMOVE_SYM_ALGO_CHACHA20
    case VAL_SYM_ALGO_CIPHER_CHACHA20:
#endif
#ifndef VAL_REMOVE_SYM_ALGO_SM4
    case VAL_SYM_ALGO_CIPHER_SM4:
#endif
#ifndef VAL_REMOVE_SYM_ALGO_ARIA
    case VAL_SYM_ALGO_CIPHER_ARIA:
#endif
        TempAssetId = SymContext_p->Service.Cipher.TempAssetId;
        break;

        // Unknown/Not Used/Initialized
    case VAL_SYM_ALGO_NONE:
    default:
        break;
    }

    if (TempAssetId != VAL_ASSETID_INVALID)
    {
        ValStatus_t funcres = val_AssetFree(TempAssetId);
        if (funcres != VAL_SUCCESS)
        {
            LOG_WARN("%s: Abort - AssetFree()=%d\n", __func__, funcres);
        }
    }

    memset(SymContext_p, 0, sizeof(ValSymContext_t));
    Adapter_Free(SymContext_p);

    return VAL_SUCCESS;
}


/*----------------------------------------------------------------------------
 * valInternal_SymCreateTempAsset
 */
ValStatus_t
valInternal_SymCreateTempAsset(
        ValSymContextPtr_t const Context_p,
        ValAssetId_t * const AssetId_p)
{

    ValStatus_t funcres;
    ValSymContext_t * SymContext_p = (ValSymContext_t *)Context_p;
    ValSize_t TempNBytes = 0;
    ValPolicyMask_t AssetPolicy = VAL_POLICY_TEMP_MAC;
    uint8_t * AssetData_p = NULL;

#ifdef VAL_STRICT_ARGS
    if ((SymContext_p == NULL) ||
        (SymContext_p->MagicBegin != VALMARKER_SYMCONTEXT) ||
        ((SymContext_p->MagicBegin ^ SymContext_p->MagicEnd) != 0) ||
        !SymContext_p->fUseTokenTemp)
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Perform algorithm based setup for token related temporary information
    switch (SymContext_p->Algorithm)
    {
    case VAL_SYM_ALGO_HASH_SHA1:
    case VAL_SYM_ALGO_MAC_HMAC_SHA1:
        TempNBytes = (160 / 8);
        AssetPolicy |= VAL_POLICY_SHA1;
        break;

    case VAL_SYM_ALGO_HASH_SHA224:
    case VAL_SYM_ALGO_MAC_HMAC_SHA224:
        TempNBytes = (256 / 8);
        // Note: intermediate digest is always 256 bits, final is 224 bits
        AssetPolicy |= VAL_POLICY_SHA224;
        break;

    case VAL_SYM_ALGO_HASH_SHA256:
    case VAL_SYM_ALGO_MAC_HMAC_SHA256:
        TempNBytes = (256 / 8);
        AssetPolicy |= VAL_POLICY_SHA256;
        break;

#ifndef VAL_REMOVE_SYM_ALGO_SHA512
    case VAL_SYM_ALGO_HASH_SHA384:
    case VAL_SYM_ALGO_MAC_HMAC_SHA384:
        TempNBytes = (512 / 8);
        // Note: intermediate digest is always 512 bits, final is 384 bits
        AssetPolicy |= VAL_POLICY_SHA384;
        break;

    case VAL_SYM_ALGO_HASH_SHA512:
    case VAL_SYM_ALGO_MAC_HMAC_SHA512:
        TempNBytes = (512 / 8);
        AssetPolicy |= VAL_POLICY_SHA512;
        break;
#endif

    case VAL_SYM_ALGO_MAC_AES_CMAC:
        TempNBytes = VAL_SYM_ALGO_AES_IV_SIZE;
        AssetPolicy |= VAL_POLICY_ALGO_CIPHER_AES | VAL_POLICY_CMAC;
        break;

    case VAL_SYM_ALGO_MAC_AES_CBC_MAC:
        TempNBytes = VAL_SYM_ALGO_AES_IV_SIZE;
        AssetPolicy |= VAL_POLICY_ALGO_CIPHER_AES;
        break;

#ifndef VAL_REMOVE_SYM_ALGO_ARIA
    case VAL_SYM_ALGO_MAC_ARIA_CMAC:
        TempNBytes = VAL_SYM_ALGO_ARIA_IV_SIZE;
        AssetPolicy |= VAL_POLICY_ALGO_CIPHER_ARIA | VAL_POLICY_CMAC;
        break;

    case VAL_SYM_ALGO_MAC_ARIA_CBC_MAC:
        TempNBytes = VAL_SYM_ALGO_ARIA_IV_SIZE;
        AssetPolicy |= VAL_POLICY_ALGO_CIPHER_ARIA;
        break;
#endif

#ifndef VAL_REMOVE_SYM_ALGO_POLY1305
    case VAL_SYM_ALGO_MAC_POLY1305:
        TempNBytes = 24;
        AssetPolicy |= VAL_POLICY_POLY1305;
        break;
#endif

    // Remaining algorithms are not supported
    default:
        return VAL_UNSUPPORTED;
    }


    // Allocate Asset for intermediate MAC value
    if (!val_IsAccessSecure())
    {
        AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
    }
    funcres = val_AssetAlloc(AssetPolicy, TempNBytes,
                             false, false,
                             VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                             AssetId_p);
    if (funcres != VAL_SUCCESS)
    {
        LOG_WARN("%s: Abort - AssetAlloc()=%d\n", __func__, funcres);
        return funcres;
    }

    // Initialize the intermediate MAC Asset data
    AssetData_p = Adapter_Alloc((unsigned int)TempNBytes);
    if (AssetData_p == NULL)
    {
        funcres = VAL_NO_MEMORY;
    }
    else
    {
        uint8_t * TempData_p = NULL;

        switch (SymContext_p->Algorithm)
        {
        case VAL_SYM_ALGO_HASH_SHA1:
        case VAL_SYM_ALGO_HASH_SHA224:
        case VAL_SYM_ALGO_HASH_SHA256:
#ifndef VAL_REMOVE_SYM_ALGO_SHA512
        case VAL_SYM_ALGO_HASH_SHA384:
        case VAL_SYM_ALGO_HASH_SHA512:
#endif
            TempData_p = SymContext_p->Service.Hash.Digest;
            break;

        case VAL_SYM_ALGO_MAC_HMAC_SHA1:
        case VAL_SYM_ALGO_MAC_HMAC_SHA224:
        case VAL_SYM_ALGO_MAC_HMAC_SHA256:
#ifndef VAL_REMOVE_SYM_ALGO_SHA512
        case VAL_SYM_ALGO_MAC_HMAC_SHA384:
        case VAL_SYM_ALGO_MAC_HMAC_SHA512:
#endif
        case VAL_SYM_ALGO_MAC_AES_CMAC:
        case VAL_SYM_ALGO_MAC_AES_CBC_MAC:
#ifndef VAL_REMOVE_SYM_ALGO_ARIA
        case VAL_SYM_ALGO_MAC_ARIA_CMAC:
        case VAL_SYM_ALGO_MAC_ARIA_CBC_MAC:
#endif
#ifndef VAL_REMOVE_SYM_ALGO_POLY1305
        case VAL_SYM_ALGO_MAC_POLY1305:
#endif
            TempData_p = SymContext_p->Service.Mac.Mac;
            break;

        // Remaining algorithms are not supported
        default:
            funcres = VAL_UNSUPPORTED;
            break;
        }

        if (funcres == VAL_SUCCESS)
        {
            memcpy(AssetData_p, TempData_p, TempNBytes);
            funcres = val_AssetLoadPlaintext(*AssetId_p, AssetData_p, TempNBytes);
        }
        Adapter_Free(AssetData_p);
    }
    if (funcres != VAL_SUCCESS)
    {
        LOG_WARN("%s: Abort - AssetLoadPlaintext()=%d\n",
                 __func__, funcres);

        // Free Asset again
        (void)val_AssetFree(*AssetId_p);
        *AssetId_p = VAL_ASSETID_INVALID;
    }

    return funcres;
}


#endif /* !VAL_REMOVE_SYM_HASH || !VAL_REMOVE_SYM_MAC || !VAL_REMOVE_SYM_CIPHER */

/* end of file adapter_val_sym_common.c */
