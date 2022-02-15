/* adapter_val_sym_cipher.c
 *
 * Implementation of the VAL API.
 *
 * This file implements the symmetric crypto cipher services.
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

#ifndef VAL_REMOVE_SYM_CIPHER

#include "basic_defs.h"
#include "clib.h"
#include "log.h"

#include "api_val_sym.h"                // the API to implement
#include "api_val_asset.h"              // Asset Management related information
#include "api_val_system.h"             // val_IsAccessSecure()
#include "adapter_val_internal.h"       // val_ExchangeToken()
#include "adapter_vex.h"                // VexToken_Command_t, VexToken_Result_t


/*----------------------------------------------------------------------------
 * valLocal_HandleIV
 */
static ValStatus_t
valLocal_HandleIV(
        ValSymContext_t * const SymContext_p,
        VexToken_Command_t * const t_cmd_p,
        const ValSize_t IVSize,
        ValPolicyMask_t AssetPolicy)
{
    // Handle IV
    if (!SymContext_p->fInitDone &&
        !SymContext_p->fUseTokenTemp &&
        (SymContext_p->Service.Cipher.TempAssetId == VAL_ASSETID_INVALID))
    {
        uint8_t * AssetData_p;
        ValStatus_t funcres;

        // Allocate Asset for IV
        if (!val_IsAccessSecure())
        {
            AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
        }
        funcres = val_AssetAlloc(AssetPolicy, IVSize,
                                 false, false,
                                 VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                                 &SymContext_p->Service.Cipher.TempAssetId);
        if (funcres != VAL_SUCCESS)
        {
            LOG_WARN("%s: Abort - AssetAlloc()=%d\n", __func__, funcres);
            return funcres;
        }

        // Initialize the IV Asset data
        AssetData_p = Adapter_Alloc((unsigned int)IVSize);
        if (AssetData_p == NULL)
        {
            funcres = VAL_NO_MEMORY;
        }
        else
        {
            memcpy(AssetData_p, SymContext_p->Service.Cipher.IV, IVSize);
            funcres = val_AssetLoadPlaintext(SymContext_p->Service.Cipher.TempAssetId,
                                             AssetData_p, IVSize);
            Adapter_Free(AssetData_p);
        }
        if (funcres != VAL_SUCCESS)
        {
            LOG_WARN("%s: Abort - AssetLoadPlaintext()=%d\n", __func__, funcres);

            // Free Asset again
            (void)val_AssetFree(SymContext_p->Service.Cipher.TempAssetId);
            SymContext_p->Service.Cipher.TempAssetId = VAL_ASSETID_INVALID;
            return funcres;
        }
    }
    else if ((SymContext_p->Service.Cipher.TempAssetId == VAL_ASSETID_INVALID) &&
             (SymContext_p->Mode != VAL_SYM_MODE_CIPHER_ECB))
    {
        // Use initial/intermediate IV
        memcpy(t_cmd_p->Service.Cipher.IV, SymContext_p->Service.Cipher.IV, IVSize);
    }

    return VAL_SUCCESS;
}


/*----------------------------------------------------------------------------
 * valLocal_AES
 */
static ValStatus_t
valLocal_AES(
        ValSymContext_t * const SymContext_p,
        VexToken_Command_t * const t_cmd_p,
        const ValSize_t SrcDataSize)
{
    ValPolicyMask_t AssetPolicy = 0;

#ifdef VAL_STRICT_ARGS
    if ((SrcDataSize & (VAL_SYM_ALGO_AES_BLOCK_SIZE-1)) != 0)
    {
        return VAL_BAD_ARGUMENT;
    }
#else
    IDENTIFIER_NOT_USED(SrcDataSize);
#endif

    // Set algorithm and mode after check
    switch (SymContext_p->Mode)
    {
    case VAL_SYM_MODE_CIPHER_ECB:
        t_cmd_p->Service.Cipher.Mode = VEXTOKEN_MODE_CIPHER_ECB;
        return VAL_SUCCESS;

    case VAL_SYM_MODE_CIPHER_CBC:
        t_cmd_p->Service.Cipher.Mode = VEXTOKEN_MODE_CIPHER_CBC;
        AssetPolicy = VAL_POLICY_ALGO_CIPHER_AES|VAL_POLICY_TEMP_IV;
        break;

    case VAL_SYM_MODE_CIPHER_CTR:
        t_cmd_p->Service.Cipher.Mode = VEXTOKEN_MODE_CIPHER_CTR;
        AssetPolicy = VAL_POLICY_ALGO_CIPHER_AES|VAL_POLICY_TEMP_COUNTER;
        break;

    case VAL_SYM_MODE_CIPHER_ICM:
        t_cmd_p->Service.Cipher.Mode = VEXTOKEN_MODE_CIPHER_ICM;
        AssetPolicy = VAL_POLICY_ALGO_CIPHER_AES|VAL_POLICY_TEMP_COUNTER;
        break;

#ifndef VAL_REMOVE_SYM_ALGO_AES_F8
    case VAL_SYM_MODE_CIPHER_F8:
        t_cmd_p->Service.Cipher.Mode = VEXTOKEN_MODE_CIPHER_F8;
        break;
#endif

#ifndef VAL_REMOVE_SYM_ALGO_AES_XTS
    case VAL_SYM_MODE_CIPHER_XTS:
        t_cmd_p->Service.Cipher.Mode = VEXTOKEN_MODE_CIPHER_XTS;
        AssetPolicy = VAL_POLICY_ALGO_CIPHER_AES|VAL_POLICY_TEMP_IV;
        break;
#endif

    default:
        LOG_WARN("%s: Abort - Invalid Mode\n", __func__);
        return VAL_INVALID_MODE;
    }

    // Handle IV
    return valLocal_HandleIV(SymContext_p, t_cmd_p,
                             VAL_SYM_ALGO_AES_IV_SIZE, AssetPolicy);
}


/*----------------------------------------------------------------------------
 * valLocal_DES_3DES
 */
#if !defined(VAL_REMOVE_SYM_ALGO_DES) || !defined(VAL_REMOVE_SYM_ALGO_3DES)
static ValStatus_t
valLocal_DES_3DES(
        ValSymContext_t * const SymContext_p,
        VexToken_Command_t * const t_cmd_p,
        const ValSize_t SrcDataSize)
{
#ifdef VAL_STRICT_ARGS
    if ((SrcDataSize & (VAL_SYM_ALGO_DES_BLOCK_SIZE-1)) != 0)
    {
        return VAL_BAD_ARGUMENT;
    }
#else
    IDENTIFIER_NOT_USED(SrcDataSize);
#endif

    // Set algorithm and mode after check
    switch (SymContext_p->Mode)
    {
    case VAL_SYM_MODE_CIPHER_ECB:
        t_cmd_p->Service.Cipher.Mode = VEXTOKEN_MODE_CIPHER_ECB;
        return VAL_SUCCESS;

    case VAL_SYM_MODE_CIPHER_CBC:
        t_cmd_p->Service.Cipher.Mode = VEXTOKEN_MODE_CIPHER_CBC;
        break;

    default:
        LOG_WARN("%s: Abort - Invalid Mode\n", __func__);
        return VAL_INVALID_MODE;
    }

    // Handle IV
    return valLocal_HandleIV(SymContext_p, t_cmd_p, VAL_SYM_ALGO_DES_IV_SIZE,
                             (VAL_POLICY_ALGO_CIPHER_TRIPLE_DES|VAL_POLICY_TEMP_IV));
}
#endif


/*----------------------------------------------------------------------------
 * valLocal_ChaCha20
 */
#ifndef VAL_REMOVE_SYM_ALGO_CHACHA20
static ValStatus_t
valLocal_ChaCha20(
        ValSymContext_t * const SymContext_p,
        VexToken_Command_t * const t_cmd_p,
        const ValSize_t SrcDataSize)
{
#ifdef VAL_STRICT_ARGS
    if ((SrcDataSize & (VAL_SYM_ALGO_CHACHA20_BLOCK_SIZE - 1)) != 0)
    {
        return VAL_BAD_ARGUMENT;
    }
#else
    IDENTIFIER_NOT_USED(SrcDataSize);
#endif

    // Set algorithm and mode after check
    if (SymContext_p->Mode != VAL_SYM_MODE_CIPHER_CHACHA20_ENC)
    {
        LOG_WARN("%s: Abort - Invalid Mode\n", __func__);
        return VAL_INVALID_MODE;
    }
    t_cmd_p->Service.Cipher.Mode = VEXTOKEN_MODE_CIPHER_CHACHA20_ENC;

    // Handle IV
    return valLocal_HandleIV(SymContext_p, t_cmd_p, VAL_SYM_ALGO_CHACHA20_IV_SIZE,
                             (VAL_POLICY_ALGO_CIPHER_CHACHA20|VAL_POLICY_TEMP_IV));
}
#endif

/*----------------------------------------------------------------------------
 * valLocal_SM4
 */
#ifndef VAL_REMOVE_SYM_ALGO_SM4
static ValStatus_t
valLocal_SM4(
        ValSymContext_t * const SymContext_p,
        VexToken_Command_t * const t_cmd_p,
        const ValSize_t SrcDataSize)
{
    ValPolicyMask_t AssetPolicy = 0;

#ifdef VAL_STRICT_ARGS
    if ((SrcDataSize & (VAL_SYM_ALGO_SM4_BLOCK_SIZE-1)) != 0)
    {
        return VAL_BAD_ARGUMENT;
    }
#else
    IDENTIFIER_NOT_USED(SrcDataSize);
#endif

    // Set algorithm and mode after check
    switch (SymContext_p->Mode)
    {
    case VAL_SYM_MODE_CIPHER_ECB:
        t_cmd_p->Service.Cipher.Mode = VEXTOKEN_MODE_CIPHER_ECB;
        return VAL_SUCCESS;

    case VAL_SYM_MODE_CIPHER_CBC:
        t_cmd_p->Service.Cipher.Mode = VEXTOKEN_MODE_CIPHER_CBC;
        AssetPolicy = VAL_POLICY_ALGO_CIPHER_SM4|VAL_POLICY_TEMP_IV;
        break;

    case VAL_SYM_MODE_CIPHER_CTR:
        t_cmd_p->Service.Cipher.Mode = VEXTOKEN_MODE_CIPHER_CTR;
        AssetPolicy = VAL_POLICY_ALGO_CIPHER_SM4|VAL_POLICY_TEMP_COUNTER;
        break;

    default:
        LOG_WARN("%s: Abort - Invalid Mode\n", __func__);
        return VAL_INVALID_MODE;
    }

    // Handle IV
    return valLocal_HandleIV(SymContext_p, t_cmd_p,
                             VAL_SYM_ALGO_SM4_IV_SIZE, AssetPolicy);
}
#endif

/*----------------------------------------------------------------------------
 * valLocal_ARIA
 */
#ifndef VAL_REMOVE_SYM_ALGO_ARIA
static ValStatus_t
valLocal_ARIA(
        ValSymContext_t * const SymContext_p,
        VexToken_Command_t * const t_cmd_p,
        const ValSize_t SrcDataSize)
{
    ValPolicyMask_t AssetPolicy = 0;

#ifdef VAL_STRICT_ARGS
    if ((SrcDataSize & (VAL_SYM_ALGO_ARIA_BLOCK_SIZE-1)) != 0)
    {
        return VAL_BAD_ARGUMENT;
    }
#else
    IDENTIFIER_NOT_USED(SrcDataSize);
#endif

    // Set algorithm and mode after check
    switch (SymContext_p->Mode)
    {
    case VAL_SYM_MODE_CIPHER_ECB:
        t_cmd_p->Service.Cipher.Mode = VEXTOKEN_MODE_CIPHER_ECB;
        return VAL_SUCCESS;

    case VAL_SYM_MODE_CIPHER_CBC:
        t_cmd_p->Service.Cipher.Mode = VEXTOKEN_MODE_CIPHER_CBC;
        AssetPolicy = VAL_POLICY_ALGO_CIPHER_ARIA|VAL_POLICY_TEMP_IV;
        break;

    case VAL_SYM_MODE_CIPHER_CTR:
        t_cmd_p->Service.Cipher.Mode = VEXTOKEN_MODE_CIPHER_CTR;
        AssetPolicy = VAL_POLICY_ALGO_CIPHER_ARIA|VAL_POLICY_TEMP_COUNTER;
        break;

    case VAL_SYM_MODE_CIPHER_ICM:
        t_cmd_p->Service.Cipher.Mode = VEXTOKEN_MODE_CIPHER_ICM;
        AssetPolicy = VAL_POLICY_ALGO_CIPHER_ARIA|VAL_POLICY_TEMP_COUNTER;
        break;

#ifndef VAL_REMOVE_SYM_ALGO_ARIA_F8
    case VAL_SYM_MODE_CIPHER_F8:
        t_cmd_p->Service.Cipher.Mode = VEXTOKEN_MODE_CIPHER_F8;
        break;
#endif

#ifndef VAL_REMOVE_SYM_ALGO_ARIA_XTS
    case VAL_SYM_MODE_CIPHER_XTS:
        t_cmd_p->Service.Cipher.Mode = VEXTOKEN_MODE_CIPHER_XTS;
        AssetPolicy = VAL_POLICY_ALGO_CIPHER_ARIA|VAL_POLICY_TEMP_IV;
        break;
#endif

    default:
        LOG_WARN("%s: Abort - Invalid Mode\n", __func__);
        return VAL_INVALID_MODE;
    }

    // Handle IV
    return valLocal_HandleIV(SymContext_p, t_cmd_p,
                             VAL_SYM_ALGO_ARIA_IV_SIZE, AssetPolicy);
}
#endif

/*----------------------------------------------------------------------------
 * val_SymCipherInitEncrypt
 */
ValStatus_t
val_SymCipherInitEncrypt(
        ValSymContextPtr_t const Context_p)
{
    ValSymContext_t * SymContext_p = (ValSymContext_t *)Context_p;

#ifdef VAL_STRICT_ARGS
    if ((SymContext_p == NULL) ||
        (SymContext_p->MagicBegin != VALMARKER_SYMCONTEXT) ||
        ((SymContext_p->MagicBegin ^ SymContext_p->MagicEnd) != 0))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    switch (SymContext_p->Algorithm)
    {
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
        if (SymContext_p->fInitDone)
        {
            return VAL_OPERATION_FAILED;
        }
        SymContext_p->Service.Cipher.fEncrypt = true;
        break;

    default:
        return VAL_INVALID_ALGORITHM;
    }
    return VAL_SUCCESS;
}


/*----------------------------------------------------------------------------
 * val_SymCipherInitParameter
 */
ValStatus_t
val_SymCipherInitParameter(
        ValSymContextPtr_t const Context_p,
        ValOctetsIn_t * const Parameter_p,
        const ValSize_t ParameterSize)
{
    ValSymContext_t * SymContext_p = (ValSymContext_t *)Context_p;

#ifdef VAL_STRICT_ARGS
    if ((SymContext_p == NULL) ||
        (SymContext_p->MagicBegin != VALMARKER_SYMCONTEXT) ||
        ((SymContext_p->MagicBegin ^ SymContext_p->MagicEnd) != 0))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    switch (SymContext_p->Algorithm)
    {
#ifndef VAL_REMOVE_SYM_ALGO_SM4
    case VAL_SYM_ALGO_CIPHER_SM4:
        if (Parameter_p == NULL)
        {
            return VAL_BAD_ARGUMENT;
        }
        if (ParameterSize != VAL_SYM_ALGO_PARAMETER_SIZE)
        {
            return VAL_INVALID_LENGTH;
        }
        memcpy(SymContext_p->Service.Cipher.Parameter, Parameter_p, ParameterSize);
        SymContext_p->Service.Cipher.fParameter = true;
        break;
#endif

    default:
        return VAL_INVALID_ALGORITHM;
    }
    return VAL_SUCCESS;
}


/*----------------------------------------------------------------------------
 * val_SymCipherInitF8
 */
#if !defined(VAL_REMOVE_SYM_ALGO_AES_F8) || \
    !defined(VAL_REMOVE_SYM_ALGO_ARIA_F8)
ValStatus_t
val_SymCipherInitF8(
        ValSymContextPtr_t const Context_p,
        ValOctetsIn_t * const f8SaltKey_p,
        const ValSize_t f8SaltKeySize,
        ValOctetsIn_t * const f8IV_p,
        const ValSize_t f8IVSize)
{
    ValSymContext_t * SymContext_p = (ValSymContext_t *)Context_p;

#ifdef VAL_STRICT_ARGS
    if ((SymContext_p == NULL) ||
        (SymContext_p->MagicBegin != VALMARKER_SYMCONTEXT) ||
        ((SymContext_p->MagicBegin ^ SymContext_p->MagicEnd) != 0) ||
        ((SymContext_p->Algorithm != VAL_SYM_ALGO_CIPHER_AES) &&
         (SymContext_p->Algorithm != VAL_SYM_ALGO_CIPHER_ARIA) )||
        (SymContext_p->Mode != VAL_SYM_MODE_CIPHER_F8) ||
        (f8IVSize != VAL_SYM_ALGO_AES_BLOCK_SIZE) ||
        (f8SaltKeySize > 16))
    {
        return VAL_BAD_ARGUMENT;
    }
    if (SymContext_p->Mode != VAL_SYM_MODE_CIPHER_F8)
    {
        return VAL_UNSUPPORTED;
    }
#endif

    if (SymContext_p->fInitDone)
    {
        return VAL_OPERATION_FAILED;
    }

    memcpy(SymContext_p->Service.Cipher.f8_SaltKey, f8SaltKey_p, f8SaltKeySize);
    memcpy(SymContext_p->Service.Cipher.f8_IV, f8IV_p, f8IVSize);
    SymContext_p->Service.Cipher.f8_SaltKeySize = (uint8_t)f8SaltKeySize;

    return VAL_SUCCESS;
}
#endif


/*----------------------------------------------------------------------------
 * val_SymCipherInitChaCha20
 */
#ifndef VAL_REMOVE_SYM_ALGO_CHACHA20
ValStatus_t
val_SymCipherInitChaCha20(
        ValSymContextPtr_t const Context_p,
        const ValSize_t NonceLength)
{
    ValSymContext_t * SymContext_p = (ValSymContext_t *)Context_p;

#ifdef VAL_STRICT_ARGS
    if ((SymContext_p == NULL) ||
        (SymContext_p->MagicBegin != VALMARKER_SYMCONTEXT) ||
        ((SymContext_p->MagicBegin ^ SymContext_p->MagicEnd) != 0) ||
        (SymContext_p->Algorithm != VAL_SYM_ALGO_CIPHER_CHACHA20) ||
        ((NonceLength != 8) && (NonceLength != 12)))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    if (SymContext_p->fInitDone)
    {
        return VAL_OPERATION_FAILED;
    }

    SymContext_p->Service.Cipher.NonceLength = (uint8_t)NonceLength;

    return VAL_SUCCESS;
}
#endif


/*----------------------------------------------------------------------------
 * val_SymCipherUpdate
 */
ValStatus_t
val_SymCipherUpdate(
        ValSymContextPtr_t const Context_p,
        ValOctetsIn_t * const SrcData_p,
        const ValSize_t SrcDataSize,
        ValOctetsOut_t * const DstData_p,
        ValSize_t * const DstDataSize_p)
{
    ValSymContext_t * SymContext_p = (ValSymContext_t *)Context_p;
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;
    ValSize_t IVSize;

#ifdef VAL_STRICT_ARGS
    if ((SymContext_p == NULL) ||
        (SymContext_p->MagicBegin != VALMARKER_SYMCONTEXT) ||
        ((SymContext_p->MagicBegin ^ SymContext_p->MagicEnd) != 0) ||
        (DstData_p == NULL) || (DstDataSize_p == NULL))
    {
        return VAL_BAD_ARGUMENT;
    }

    switch (SymContext_p->Mode)
    {
#if !defined(VAL_REMOVE_SYM_ALGO_AES_XTS) || \
    !defined(VAL_REMOVE_SYM_ALGO_AES_F8)  || \
    !defined(VAL_REMOVE_SYM_ALGO_ARIA_XTS) || \
    !defined(VAL_REMOVE_SYM_ALGO_ARIA_F8)
#if !defined(VAL_REMOVE_SYM_ALGO_AES_XTS) || \
    !defined(VAL_REMOVE_SYM_ALGO_ARIA_XTS)
    case VAL_SYM_MODE_CIPHER_XTS:
#endif
#if !defined(VAL_REMOVE_SYM_ALGO_AES_F8) || \
    !defined(VAL_REMOVE_SYM_ALGO_ARIA_F8)
    case VAL_SYM_MODE_CIPHER_F8:
#endif
        if (!SymContext_p->fInitDone)
        {
            return VAL_UNSUPPORTED;
        }
        break;
#endif
#if !defined(VAL_REMOVE_SYM_ALGO_AES_CCM) || \
    !defined(VAL_REMOVE_SYM_ALGO_AES_GCM) || \
    !defined(VAL_REMOVE_SYM_ALGO_ARIA_CCM) || \
    !defined(VAL_REMOVE_SYM_ALGO_ARIA_GCM) || \
    !defined(VAL_REMOVE_SYM_ALGO_CHACHA20)
#if !defined(VAL_REMOVE_SYM_ALGO_AES_CCM) || \
    !defined(VAL_REMOVE_SYM_ALGO_ARIA_CCM)
    case VAL_SYM_MODE_CIPHER_CCM:
#endif
#if !defined(VAL_REMOVE_SYM_ALGO_AES_GCM) || \
    !defined(VAL_REMOVE_SYM_ALGO_ARIA_GCM)
    case VAL_SYM_MODE_CIPHER_GCM:
#endif
#ifndef VAL_REMOVE_SYM_ALGO_CHACHA20
    case VAL_SYM_MODE_CIPHER_CHACHA20_AEAD:
#endif
        return VAL_UNSUPPORTED;
#endif
#ifndef VAL_REMOVE_SYM_ALGO_CHACHA20
    case VAL_SYM_MODE_CIPHER_CHACHA20_ENC:
        if (SrcData_p == NULL)
        {
            if ((SrcDataSize != 0) || !SymContext_p->fInitDone)
            {
                return VAL_BAD_ARGUMENT;
            }
            // Poly1305 one-time key generation
        }
        else if (SrcDataSize == 0)
        {
            return VAL_BAD_ARGUMENT;
        }
        break;
#endif
    default:
        if ((SrcData_p == NULL) || (SrcDataSize == 0))
        {
            return VAL_BAD_ARGUMENT;
        }
        break;
    }
#endif

#ifndef VAL_REMOVE_SYM_ALGO_CHACHA20
    if ((SymContext_p->Mode == VAL_SYM_MODE_CIPHER_CHACHA20_ENC) &&
        (SrcDataSize == 0))
    {
        if (*DstDataSize_p < (256/8))
        {
            // Poly1305 one-time key
            *DstDataSize_p = (256/8);
            return VAL_BUFFER_TOO_SMALL;
        }
    }
    else
#endif
    if (*DstDataSize_p < SrcDataSize)
    {
        *DstDataSize_p = SrcDataSize;
        return VAL_BUFFER_TOO_SMALL;
    }

    // Format service request
    t_cmd.OpCode  = VEXTOKEN_OPCODE_ENCRYPTION;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_ENCRYPT;

    switch (SymContext_p->Algorithm)
    {
    case VAL_SYM_ALGO_CIPHER_AES:
        t_cmd.Service.Cipher.Algorithm = VEXTOKEN_ALGO_CIPHER_AES;
        funcres = valLocal_AES(SymContext_p, &t_cmd, SrcDataSize);
        IVSize = VAL_SYM_ALGO_AES_IV_SIZE;
        break;
#ifndef VAL_REMOVE_SYM_ALGO_DES
    case VAL_SYM_ALGO_CIPHER_DES:
        t_cmd.Service.Cipher.Algorithm = VEXTOKEN_ALGO_CIPHER_DES;
        funcres = valLocal_DES_3DES(SymContext_p, &t_cmd, SrcDataSize);
        IVSize = VAL_SYM_ALGO_DES_IV_SIZE;
        break;
#endif
#ifndef VAL_REMOVE_SYM_ALGO_3DES
    case VAL_SYM_ALGO_CIPHER_TRIPLE_DES:
        t_cmd.Service.Cipher.Algorithm = VEXTOKEN_ALGO_CIPHER_3DES;
        funcres = valLocal_DES_3DES(SymContext_p, &t_cmd, SrcDataSize);
        IVSize = VAL_SYM_ALGO_DES_IV_SIZE;
        break;
#endif
#ifndef VAL_REMOVE_SYM_ALGO_CHACHA20
    case VAL_SYM_ALGO_CIPHER_CHACHA20:
        t_cmd.Service.Cipher.Algorithm = VEXTOKEN_ALGO_CIPHER_CHACHA20;
        t_cmd.Service.Cipher.NonceLength = SymContext_p->Service.Cipher.NonceLength;
        funcres = valLocal_ChaCha20(SymContext_p, &t_cmd, SrcDataSize);
        IVSize = VAL_SYM_ALGO_CHACHA20_IV_SIZE;
        // Note: If SrcDataSize == 0 then Poly1305 one-time key generation
        //       is assumed, this means that at VEX layer the DstData_p
        //       buffer size will be set to 256-bit (32-byte).
        break;
#endif
#ifndef VAL_REMOVE_SYM_ALGO_SM4
    case VAL_SYM_ALGO_CIPHER_SM4:
        t_cmd.Service.Cipher.Algorithm = VEXTOKEN_ALGO_CIPHER_SM4;
        funcres = valLocal_SM4(SymContext_p, &t_cmd, SrcDataSize);
        IVSize = VAL_SYM_ALGO_SM4_IV_SIZE;
        break;
#endif
#ifndef VAL_REMOVE_SYM_ALGO_ARIA
    case VAL_SYM_ALGO_CIPHER_ARIA:
        t_cmd.Service.Cipher.Algorithm = VEXTOKEN_ALGO_CIPHER_ARIA;
        funcres = valLocal_ARIA(SymContext_p, &t_cmd, SrcDataSize);
        IVSize = VAL_SYM_ALGO_ARIA_IV_SIZE;
        break;
#endif

    default:
        return VAL_INVALID_ALGORITHM;
    }
    if (funcres != VAL_SUCCESS)
    {
        return funcres;
    }

    // Handle general token part
    t_cmd.Service.Cipher.KeySize = SymContext_p->Service.Cipher.KeySize;
    t_cmd.Service.Cipher.KeyAssetId = SymContext_p->Service.Cipher.KeyAssetId;
    if (SymContext_p->Service.Cipher.KeyAssetId == VAL_ASSETID_INVALID)
    {
        memcpy(t_cmd.Service.Cipher.Key,
               SymContext_p->Service.Cipher.Key,
               t_cmd.Service.Cipher.KeySize);
    }
    t_cmd.Service.Cipher.TempAssetId = SymContext_p->Service.Cipher.TempAssetId;
    t_cmd.Service.Cipher.SrcData_p = (const uint8_t *)SrcData_p;
    t_cmd.Service.Cipher.DstData_p = (uint8_t *)DstData_p;
    t_cmd.Service.Cipher.DataSize = (uint32_t)SrcDataSize;
    t_cmd.Service.Cipher.fEncrypt = SymContext_p->Service.Cipher.fEncrypt;
#if !defined(VAL_REMOVE_SYM_ALGO_AES_F8) || \
    !defined(VAL_REMOVE_SYM_ALGO_ARIA_F8)
    if (SymContext_p->Mode == VAL_SYM_MODE_CIPHER_F8)
    {
        t_cmd.Service.Cipher.f8_SaltKeySize = SymContext_p->Service.Cipher.f8_SaltKeySize;
        memcpy(t_cmd.Service.Cipher.f8_SaltKey,
               SymContext_p->Service.Cipher.f8_SaltKey,
               SymContext_p->Service.Cipher.f8_SaltKeySize);
        memcpy(t_cmd.Service.Cipher.f8_IV,
               SymContext_p->Service.Cipher.f8_IV,
               IVSize);
    }
#endif
#ifndef VAL_REMOVE_SYM_ALGO_SM4
    if (SymContext_p->Service.Cipher.fParameter)
    {
        memcpy(t_cmd.Service.Cipher.Parameter,
               SymContext_p->Service.Cipher.Parameter,
               sizeof(t_cmd.Service.Cipher.Parameter));
        t_cmd.Service.Cipher.fParameter = true;
    }
    else
    {
        t_cmd.Service.Cipher.fParameter = false;
    }
#else
    t_cmd.Service.Cipher.fParameter = false;
#endif
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;
    ZEROINIT(t_res.Service.Cipher.IV);

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if (funcres == VAL_SUCCESS)
    {
        // Check for errors
        if (t_res.Result == VEXTOKEN_RESULT_NO_ERROR)
        {
            // Mark initialization done and set output size
            SymContext_p->fInitDone = true;
#ifndef VAL_REMOVE_SYM_ALGO_CHACHA20
            if ((SrcDataSize == 0) &&
                (SymContext_p->Algorithm ==  VAL_SYM_ALGO_CIPHER_CHACHA20))
            {
                *DstDataSize_p = (256/8);
            }
            else
            {
                *DstDataSize_p = SrcDataSize;
            }
#else
            *DstDataSize_p = SrcDataSize;
#endif

            if ((SymContext_p->Service.Cipher.TempAssetId == VAL_ASSETID_INVALID) &&
                (SymContext_p->Mode != VAL_SYM_MODE_CIPHER_ECB))
            {
                // Copy the intermediate IV
                memcpy(SymContext_p->Service.Cipher.IV,
                       t_res.Service.Cipher.IV,
                       IVSize);
            }
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
 * val_SymCipherFinal
 */
ValStatus_t
val_SymCipherFinal(
        ValSymContextPtr_t const Context_p,
        ValOctetsIn_t * const SrcData_p,
        const ValSize_t SrcDataSize,
        ValOctetsOut_t * const DstData_p,
        ValSize_t * const DstDataSize_p)
{
    ValSymContext_t * SymContext_p = (ValSymContext_t *)Context_p;
    ValStatus_t funcres;

#ifdef VAL_STRICT_ARGS
    if ((SymContext_p == NULL) ||
        (SymContext_p->MagicBegin != VALMARKER_SYMCONTEXT) ||
        ((SymContext_p->MagicBegin ^ SymContext_p->MagicEnd) != 0))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    SymContext_p->fInitDone = true;
    funcres = val_SymCipherUpdate(Context_p,
                                  SrcData_p, SrcDataSize,
                                  DstData_p, DstDataSize_p);
    if (funcres == VAL_SUCCESS)
    {
        // Release symmetric crypto context
        (void)val_SymRelease(Context_p);
    }

    return funcres;
}


/*----------------------------------------------------------------------------
 * val_SymCipherAEInit
 */
#if !defined(VAL_REMOVE_SYM_ALGO_AES_CCM) || \
    !defined(VAL_REMOVE_SYM_ALGO_AES_GCM) || \
    !defined(VAL_REMOVE_SYM_ALGO_ARIA_CCM) || \
    !defined(VAL_REMOVE_SYM_ALGO_ARIA_GCM) || \
    !defined(VAL_REMOVE_SYM_ALGO_CHACHA20)
ValStatus_t
val_SymCipherAEInit(
        ValSymContextPtr_t const Context_p,
        ValOctetsIn_t * const NonceHashKey_p,
        const ValSize_t NonceHashKeySize,
        const ValSize_t TagSize,
        const ValSymGCMMode_t GCMMode)
{
    ValSymContext_t * SymContext_p = (ValSymContext_t *)Context_p;

#ifdef VAL_STRICT_ARGS
    // General argument checks
    if ((SymContext_p == NULL) ||
        (SymContext_p->MagicBegin != VALMARKER_SYMCONTEXT) ||
        ((SymContext_p->MagicBegin ^ SymContext_p->MagicEnd) != 0))
    {
        return VAL_BAD_ARGUMENT;
    }

    switch (SymContext_p->Algorithm)
    {
#if !defined(VAL_REMOVE_SYM_ALGO_AES_CCM) || \
    !defined(VAL_REMOVE_SYM_ALGO_AES_GCM)
    case VAL_SYM_ALGO_CIPHER_AES:
#endif
#if !defined(VAL_REMOVE_SYM_ALGO_ARIA_CCM) || \
    !defined(VAL_REMOVE_SYM_ALGO_ARIA_GCM)
    case VAL_SYM_ALGO_CIPHER_ARIA:
#endif
#ifndef VAL_REMOVE_SYM_ALGO_CHACHA20
    case VAL_SYM_ALGO_CIPHER_CHACHA20:
#endif
        break;

    default:
        return VAL_INVALID_ALGORITHM;
    }

    // Mode specific argument checks
    switch (SymContext_p->Mode)
    {
#if !defined(VAL_REMOVE_SYM_ALGO_AES_CCM) || \
    !defined(VAL_REMOVE_SYM_ALGO_ARIA_CCM)
    case VAL_SYM_MODE_CIPHER_CCM:
        if ((NonceHashKey_p == NULL) ||
            (NonceHashKeySize < 7) || (NonceHashKeySize > 13) ||
            (TagSize < 4) || (TagSize > 16) || ((TagSize & 1) != 0))
        {
            return VAL_BAD_ARGUMENT;
        }
        break;
#endif
#if !defined(VAL_REMOVE_SYM_ALGO_AES_GCM) || \
    !defined(VAL_REMOVE_SYM_ALGO_ARIA_GCM)
    case VAL_SYM_MODE_CIPHER_GCM:
        if ((GCMMode < VAL_SYM_MODE_GCM_GHASH) || (GCMMode >= VAL_SYM_MODE_GCM_MAX) ||
            ((GCMMode != VAL_SYM_MODE_GCM_AUTO) &&
             ((NonceHashKey_p == NULL) || (NonceHashKeySize != 16))) ||
            ((TagSize < 12) && (TagSize != 4) && (TagSize != 8)) || (TagSize > 16))
        {
            return VAL_BAD_ARGUMENT;
        }
        break;
#endif
#ifndef VAL_REMOVE_SYM_ALGO_CHACHA20
    case VAL_SYM_MODE_CIPHER_CHACHA20_AEAD:
        if ((NonceHashKey_p == NULL) || (NonceHashKeySize != 12) ||
            (TagSize != 16))
        {
            return VAL_BAD_ARGUMENT;
        }
        break;
#endif
    default:
        return VAL_INVALID_ALGORITHM;
    }
#endif

    memcpy(SymContext_p->Service.CipherAE.NonceHashKey, NonceHashKey_p, NonceHashKeySize);
    SymContext_p->Service.CipherAE.NonceHashKeySize = (uint8_t)NonceHashKeySize;
    SymContext_p->Service.CipherAE.TagSize = (uint8_t)TagSize;
#if !defined(VAL_REMOVE_SYM_ALGO_AES_GCM) || \
    !defined(VAL_REMOVE_SYM_ALGO_ARIA_GCM)
    if (SymContext_p->Mode == VAL_SYM_MODE_CIPHER_GCM)
    {
        SymContext_p->Service.CipherAE.GCMMode = (uint8_t)(GCMMode - VAL_SYM_MODE_GCM_GHASH);
    }
    else
    {
        SymContext_p->Service.CipherAE.GCMMode = 0;
    }
#else
    IDENTIFIER_NOT_USED(GCMMode);
    SymContext_p->Service.CipherAE.GCMMode = 0;
#endif

    return VAL_SUCCESS;
}
#endif


/*----------------------------------------------------------------------------
 * val_SymCipherAEFinal
 */
#if !defined(VAL_REMOVE_SYM_ALGO_AES_CCM) || \
    !defined(VAL_REMOVE_SYM_ALGO_AES_GCM) || \
    !defined(VAL_REMOVE_SYM_ALGO_ARIA_CCM) || \
    !defined(VAL_REMOVE_SYM_ALGO_ARIA_GCM) || \
    !defined(VAL_REMOVE_SYM_ALGO_CHACHA20)
ValStatus_t
val_SymCipherAEFinal(
        ValSymContextPtr_t const Context_p,
        ValOctetsIn_t * const AAD_p,
        const ValSize_t AADSize,
        ValOctetsIn_t * const SrcData_p,
        const ValSize_t SrcDataSize,
        ValOctetsOut_t * const DstData_p,
        ValSize_t * const DstDataSize_p,
        ValOctetsInOut_t * const Tag_p,
        ValSize_t * const TagSize_p)
{
    ValSymContext_t * SymContext_p = (ValSymContext_t *)Context_p;
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;

#ifdef VAL_STRICT_ARGS
    // General argument checks
    if ((SymContext_p == NULL) ||
        (SymContext_p->MagicBegin != VALMARKER_SYMCONTEXT) ||
        ((SymContext_p->MagicBegin ^ SymContext_p->MagicEnd) != 0) ||
        ((AADSize != 0) && (AAD_p == NULL)) ||
        ((SrcDataSize != 0) && (SrcData_p == NULL)) ||
        (Tag_p == NULL) || (TagSize_p == NULL) ||
        (*TagSize_p < (ValSize_t)SymContext_p->Service.CipherAE.TagSize))
    {
        return VAL_BAD_ARGUMENT;
    }

    switch (SymContext_p->Algorithm)
    {
#if !defined(VAL_REMOVE_SYM_ALGO_AES_CCM) || \
    !defined(VAL_REMOVE_SYM_ALGO_AES_GCM)
    case VAL_SYM_ALGO_CIPHER_AES:
#endif
#if !defined(VAL_REMOVE_SYM_ALGO_ARIA_CCM) || \
    !defined(VAL_REMOVE_SYM_ALGO_ARIA_GCM)
    case VAL_SYM_ALGO_CIPHER_ARIA:
#endif
#ifndef VAL_REMOVE_SYM_ALGO_CHACHA20
    case VAL_SYM_ALGO_CIPHER_CHACHA20:
#endif
        break;

    default:
        return VAL_INVALID_ALGORITHM;
    }

    // Mode specific argument checks
    switch (SymContext_p->Mode)
    {
#if !defined(VAL_REMOVE_SYM_ALGO_AES_CCM) || \
    !defined(VAL_REMOVE_SYM_ALGO_ARIA_CCM) || \
    !defined(VAL_REMOVE_SYM_ALGO_CHACHA20)
#if !defined(VAL_REMOVE_SYM_ALGO_AES_CCM) || \
    !defined(VAL_REMOVE_SYM_ALGO_ARIA_CCM)
    case VAL_SYM_MODE_CIPHER_CCM:
#endif
#ifndef VAL_REMOVE_SYM_ALGO_CHACHA20
    case VAL_SYM_MODE_CIPHER_CHACHA20_AEAD:
#endif
        if ((DstData_p == NULL) || (DstDataSize_p == NULL))
        {
            return VAL_BAD_ARGUMENT;
        }
        break;
#endif
#if !defined(VAL_REMOVE_SYM_ALGO_AES_GCM) || \
    !defined(VAL_REMOVE_SYM_ALGO_ARIA_GCM)
    case VAL_SYM_MODE_CIPHER_GCM:
        if ((SymContext_p->Service.CipherAE.GCMMode != 0) &&
            ((DstData_p == NULL) || (DstDataSize_p == NULL)))
        {
            return VAL_BAD_ARGUMENT;
        }
        break;
#endif
    default:
        return VAL_INVALID_ALGORITHM;
    }
#endif

    // Format service request
    switch (SymContext_p->Mode)
    {
#if !defined(VAL_REMOVE_SYM_ALGO_AES_CCM) || \
    !defined(VAL_REMOVE_SYM_ALGO_ARIA_CCM)
    case VAL_SYM_MODE_CIPHER_CCM:
        if (*DstDataSize_p < SrcDataSize)
        {
            *DstDataSize_p = SrcDataSize;
            return VAL_BUFFER_TOO_SMALL;
        }
        switch (SymContext_p->Algorithm)
        {
#if !defined(VAL_REMOVE_SYM_ALGO_AES_CCM)
        case VAL_SYM_ALGO_CIPHER_AES:
            t_cmd.Service.CipherAE.Algorithm = VEXTOKEN_ALGO_CIPHER_AES;
            break;
#endif
#if !defined(VAL_REMOVE_SYM_ALGO_ARIA_CCM)
        case VAL_SYM_ALGO_CIPHER_ARIA:
            t_cmd.Service.CipherAE.Algorithm = VEXTOKEN_ALGO_CIPHER_ARIA;
            break;
#endif
        default:
            return VAL_INVALID_ALGORITHM;
        }
        t_cmd.Service.CipherAE.Mode = VEXTOKEN_MODE_CIPHER_CCM;
        break;
#endif
#if !defined(VAL_REMOVE_SYM_ALGO_AES_GCM) || \
    !defined(VAL_REMOVE_SYM_ALGO_ARIA_GCM)
    case VAL_SYM_MODE_CIPHER_GCM:
        if ((SymContext_p->Service.CipherAE.GCMMode != 0) &&
            (*DstDataSize_p < SrcDataSize))
        {
            *DstDataSize_p = SrcDataSize;
            return VAL_BUFFER_TOO_SMALL;
        }
        switch (SymContext_p->Algorithm)
        {
#if !defined(VAL_REMOVE_SYM_ALGO_AES_GCM)
        case VAL_SYM_ALGO_CIPHER_AES:
            t_cmd.Service.CipherAE.Algorithm = VEXTOKEN_ALGO_CIPHER_AES;
            memcpy(t_cmd.Service.CipherAE.IV,
                   SymContext_p->Service.CipherAE.IV,
                   VAL_SYM_ALGO_AES_IV_SIZE);
            break;
#endif
#if !defined(VAL_REMOVE_SYM_ALGO_ARIA_GCM)
        case VAL_SYM_ALGO_CIPHER_ARIA:
            t_cmd.Service.CipherAE.Algorithm = VEXTOKEN_ALGO_CIPHER_ARIA;
            memcpy(t_cmd.Service.CipherAE.IV,
                   SymContext_p->Service.CipherAE.IV,
                   VAL_SYM_ALGO_ARIA_IV_SIZE);
            break;
#endif
        default:
            return VAL_INVALID_ALGORITHM;
        }
        t_cmd.Service.CipherAE.Mode = VEXTOKEN_MODE_CIPHER_GCM;
        break;
#endif
#ifndef VAL_REMOVE_SYM_ALGO_CHACHA20
    case VAL_SYM_MODE_CIPHER_CHACHA20_AEAD:
        if (*DstDataSize_p < SrcDataSize)
        {
            *DstDataSize_p = SrcDataSize;
            return VAL_BUFFER_TOO_SMALL;
        }
        t_cmd.Service.CipherAE.Algorithm = VEXTOKEN_ALGO_CIPHER_CHACHA20;
        t_cmd.Service.CipherAE.Mode = VEXTOKEN_MODE_CIPHER_CHACHA20_AEAD;
        break;
#endif
    default:
        return VAL_INVALID_ALGORITHM;
    }

    t_cmd.OpCode  = VEXTOKEN_OPCODE_ENCRYPTION;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_AUTHENCRYPT;

    t_cmd.Service.CipherAE.KeySize = SymContext_p->Service.CipherAE.KeySize;
    t_cmd.Service.CipherAE.KeyAssetId = SymContext_p->Service.CipherAE.KeyAssetId;
    if (SymContext_p->Service.CipherAE.KeyAssetId == VAL_ASSETID_INVALID)
    {
        memcpy(t_cmd.Service.CipherAE.Key,
               SymContext_p->Service.CipherAE.Key,
               t_cmd.Service.CipherAE.KeySize);
    }
    t_cmd.Service.CipherAE.AAD_p = (const uint8_t *)AAD_p;
    t_cmd.Service.CipherAE.AADSize = (uint32_t)AADSize;
    t_cmd.Service.CipherAE.SrcData_p = (const uint8_t *)SrcData_p;
    t_cmd.Service.CipherAE.DstData_p = (uint8_t *)DstData_p;
    t_cmd.Service.CipherAE.DataSize = (uint32_t)SrcDataSize;
    t_cmd.Service.CipherAE.NonceHashKeySize = SymContext_p->Service.CipherAE.NonceHashKeySize;
    memcpy(t_cmd.Service.CipherAE.NonceHashKey,
           SymContext_p->Service.CipherAE.NonceHashKey,
           t_cmd.Service.CipherAE.NonceHashKeySize);
    t_cmd.Service.CipherAE.GCMMode = SymContext_p->Service.CipherAE.GCMMode;
    t_cmd.Service.CipherAE.TagSize = (uint8_t)*TagSize_p;
    if (SymContext_p->Service.CipherAE.fEncrypt)
    {
        t_cmd.Service.CipherAE.fEncrypt = true;
    }
    else
    {
        t_cmd.Service.CipherAE.fEncrypt = false;
        memcpy(t_cmd.Service.CipherAE.Tag, Tag_p, *TagSize_p);
    }
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;
    ZEROINIT(t_res.Service.CipherAE.Tag);

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if (funcres == VAL_SUCCESS)
    {
        // Check for errors
        if (t_res.Result == VEXTOKEN_RESULT_NO_ERROR)
        {
            // Mark initialization done and set output size
            SymContext_p->fInitDone = true;
            *DstDataSize_p = SrcDataSize;

            if (SymContext_p->Service.CipherAE.fEncrypt)
            {
                memcpy(Tag_p,
                       t_res.Service.CipherAE.Tag,
                       sizeof(t_res.Service.CipherAE.Tag));
                *TagSize_p = (ValSize_t)SymContext_p->Service.CipherAE.TagSize;
            }
        }
        else
        {
            funcres = (ValStatus_t)t_res.Result;
            LOG_WARN("Abort - %s()=%d\n", __func__, t_res.Result);
        }
    }

    if (funcres == VAL_SUCCESS)
    {
        // Release symmetric crypto context
        (void)val_SymRelease(Context_p);
    }

    return funcres;
}
#endif


#endif /* !VAL_REMOVE_SYM_CIPHER */

/* end of file adapter_val_sym_cipher.c */
