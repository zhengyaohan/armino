/* adapter_val_asym_rsa.c
 *
 * Implementation of the VAL API.
 *
 * This file implements the asymmetric crypto RSA related services.
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

#include "basic_defs.h"
#include "clib.h"
#include "log.h"

#include "api_val_asym.h"               // the API to implement
#include "api_val_asset.h"              // Asset Management related information
#include "api_val_sym.h"                // ValSymContextPtr_Optional_t, ValSymAlgo_t
#include "api_val_system.h"             // val_IsAccessSecure()
#include "adapter_val_internal.h"       // val_ExchangeToken(), valInternal_*()
#include "adapter_vex.h"                // VexToken_Command_t, VexToken_Result_t

/*----------------------------------------------------------------------------
 * valLocal_AsymRsaAllocKeyAsset
 */
#if !defined(VAL_REMOVE_ASYM_RSAPKCS1V15_SIGN) || \
    !defined(VAL_REMOVE_ASYM_RSAPKCS1V15_VERIFY) || \
    !defined(VAL_REMOVE_ASYM_RSAPSS_SIGN) || \
    !defined(VAL_REMOVE_ASYM_RSAPSS_VERIFY) || \
    !defined(VAL_REMOVE_ASYM_RSA_OAEP) || \
    !defined(VAL_REMOVE_ASYM_RSA_PKCS1WRAP)
static ValStatus_t
valLocal_AsymRsaAllocKeyAsset(
        const ValSize_t ModulusSizeBits,
        const ValSize_t ExponentSizeBits,
        const ValSymAlgo_t HashAlgorithm,
        ValPolicyMask_t AssetPolicy,
        const bool fCrossDomain,
        const bool fExportAllowed,
        ValAssetId_t * const AssetId_p)
{
#ifdef VAL_STRICT_ARGS
    if ((AssetId_p == NULL) ||
        (ModulusSizeBits < VAL_ASYM_RSA_MIN_BITS) ||
        (ModulusSizeBits > VAL_ASYM_RSA_MAX_BITS))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Determine Asset policy
    switch (HashAlgorithm)
    {
    case VAL_SYM_ALGO_HASH_SHA1:
        AssetPolicy |= VAL_POLICY_SHA1;
        break;
    case VAL_SYM_ALGO_HASH_SHA224:
        AssetPolicy |= VAL_POLICY_SHA224;
        break;
    case VAL_SYM_ALGO_HASH_SHA256:
        AssetPolicy |= VAL_POLICY_SHA256;
        break;
#ifndef VAL_REMOVE_SYM_ALGO_SHA512
    case VAL_SYM_ALGO_HASH_SHA384:
        AssetPolicy |= VAL_POLICY_SHA384;
        break;
    case VAL_SYM_ALGO_HASH_SHA512:
        AssetPolicy |= VAL_POLICY_SHA512;
        break;
#endif
#ifndef VAL_REMOVE_ASYM_RSA_PKCS1WRAP
    case VAL_SYM_ALGO_NONE:
        if ((AssetPolicy & VAL_POLICY_PK_RSA_PKCS1_WRAP) != 0)
        {
            break;
        }
        return VAL_INVALID_ALGORITHM;
#endif
    default:
        return VAL_INVALID_ALGORITHM;
    }
    if (fCrossDomain)
    {
        AssetPolicy |= VAL_POLICY_CROSS_DOMAIN;
    }
    if (fExportAllowed)
    {
        AssetPolicy |= VAL_POLICY_EXPORT;
    }
    if (!val_IsAccessSecure())
    {
        AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
    }

    // Determine the Asset size and allocate the Asset
    return val_AssetAlloc(AssetPolicy,
                          (VAL_ASYM_DATA_SIZE_VWB(ModulusSizeBits) +
                           VAL_ASYM_DATA_SIZE_VWB(ExponentSizeBits)),
                          false, false,
                          VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                          AssetId_p);
}
#endif


/*----------------------------------------------------------------------------
 * val_AsymRsaPkcs1v15AllocPrivateKeyAsset
 */
#ifndef VAL_REMOVE_ASYM_RSAPKCS1V15_SIGN
ValStatus_t
val_AsymRsaPkcs1v15AllocPrivateKeyAsset(
        const ValSize_t ModulusSizeBits,
        const ValSize_t ExponentSizeBits,
        const ValSymAlgo_t HashAlgorithm,
        const bool fCrossDomain,
        const bool fExportAllowed,
        ValAssetId_t * const AssetId_p)
{
#ifdef VAL_STRICT_ARGS
    if ((ExponentSizeBits < VAL_ASYM_RSA_MIN_BITS) ||
        (ExponentSizeBits > VAL_ASYM_RSA_MAX_BITS))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    return valLocal_AsymRsaAllocKeyAsset(ModulusSizeBits, ExponentSizeBits,
                                         HashAlgorithm,
                                         VAL_POLICY_PK_RSA_PKCS1_SIGN,
                                         fCrossDomain, fExportAllowed,
                                         AssetId_p);
}
#endif


/*----------------------------------------------------------------------------
 * val_AsymRsaPkcs1v15AllocPublicKeyAsset
 */
#ifndef VAL_REMOVE_ASYM_RSAPKCS1V15_VERIFY
ValStatus_t
val_AsymRsaPkcs1v15AllocPublicKeyAsset(
        const ValSize_t ModulusSizeBits,
        const ValSize_t ExponentSizeBits,
        const ValSymAlgo_t HashAlgorithm,
        const bool fCrossDomain,
        const bool fExportAllowed,
        ValAssetId_t * const AssetId_p)
{
#ifdef VAL_STRICT_ARGS
    if ((ExponentSizeBits < VAL_ASYM_RSA_EXPO_MIN_BITS) ||
        (ExponentSizeBits > VAL_ASYM_RSA_EXPO_MAX_BITS))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    return valLocal_AsymRsaAllocKeyAsset(ModulusSizeBits, ExponentSizeBits,
                                         HashAlgorithm,
                                         VAL_POLICY_PUBLIC_KEY|VAL_POLICY_PK_RSA_PKCS1_SIGN,
                                         fCrossDomain, fExportAllowed,
                                         AssetId_p);
}
#endif


/*----------------------------------------------------------------------------
 * val_AsymRsaPssAllocPrivateKeyAsset
 */
#ifndef VAL_REMOVE_ASYM_RSAPSS_SIGN
ValStatus_t
val_AsymRsaPssAllocPrivateKeyAsset(
        const ValSize_t ModulusSizeBits,
        const ValSize_t ExponentSizeBits,
        const ValSymAlgo_t HashAlgorithm,
        const bool fCrossDomain,
        const bool fExportAllowed,
        ValAssetId_t * const AssetId_p)
{
#ifdef VAL_STRICT_ARGS
    if ((ExponentSizeBits < VAL_ASYM_RSA_MIN_BITS) ||
        (ExponentSizeBits > VAL_ASYM_RSA_MAX_BITS))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    return valLocal_AsymRsaAllocKeyAsset(ModulusSizeBits, ExponentSizeBits,
                                         HashAlgorithm,
                                         VAL_POLICY_PK_RSA_PSS_SIGN,
                                         fCrossDomain, fExportAllowed,
                                         AssetId_p);
}
#endif


/*----------------------------------------------------------------------------
 * val_AsymRsaPssAllocPublicKeyAsset
 */
#ifndef VAL_REMOVE_ASYM_RSAPSS_VERIFY
ValStatus_t
val_AsymRsaPssAllocPublicKeyAsset(
        const ValSize_t ModulusSizeBits,
        const ValSize_t ExponentSizeBits,
        const ValSymAlgo_t HashAlgorithm,
        const bool fCrossDomain,
        const bool fExportAllowed,
        ValAssetId_t * const AssetId_p)
{
#ifdef VAL_STRICT_ARGS
    if ((ExponentSizeBits < VAL_ASYM_RSA_EXPO_MIN_BITS) ||
        (ExponentSizeBits > VAL_ASYM_RSA_EXPO_MAX_BITS))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    return valLocal_AsymRsaAllocKeyAsset(ModulusSizeBits, ExponentSizeBits,
                                         HashAlgorithm,
                                         VAL_POLICY_PUBLIC_KEY|VAL_POLICY_PK_RSA_PSS_SIGN,
                                         fCrossDomain, fExportAllowed,
                                         AssetId_p);
}
#endif


/*----------------------------------------------------------------------------
 * val_AsymRsaOaepAllocPrivateKeyAsset
 */
#ifndef VAL_REMOVE_ASYM_RSA_OAEP
ValStatus_t
val_AsymRsaOaepAllocPrivateKeyAsset(
        const ValSize_t ModulusSizeBits,
        const ValSize_t ExponentSizeBits,
        const ValSymAlgo_t HashAlgorithm,
        const bool fCrossDomain,
        const bool fExportAllowed,
        ValAssetId_t * const AssetId_p)
{
#ifdef VAL_STRICT_ARGS
    if ((ExponentSizeBits < VAL_ASYM_RSA_MIN_BITS) ||
        (ExponentSizeBits > VAL_ASYM_RSA_MAX_BITS))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    return valLocal_AsymRsaAllocKeyAsset(ModulusSizeBits, ExponentSizeBits,
                                         HashAlgorithm,
                                         VAL_POLICY_PK_RSA_OAEP_WRAP,
                                         fCrossDomain, fExportAllowed,
                                         AssetId_p);
}
#endif


/*----------------------------------------------------------------------------
 * val_AsymRsaOaepAllocPublicKeyAsset
 */
#ifndef VAL_REMOVE_ASYM_RSA_OAEP
ValStatus_t
val_AsymRsaOaepAllocPublicKeyAsset(
        const ValSize_t ModulusSizeBits,
        const ValSize_t ExponentSizeBits,
        const ValSymAlgo_t HashAlgorithm,
        const bool fCrossDomain,
        const bool fExportAllowed,
        ValAssetId_t * const AssetId_p)
{
#ifdef VAL_STRICT_ARGS
    if ((ExponentSizeBits < VAL_ASYM_RSA_EXPO_MIN_BITS) ||
        (ExponentSizeBits > VAL_ASYM_RSA_EXPO_MAX_BITS))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    return valLocal_AsymRsaAllocKeyAsset(ModulusSizeBits, ExponentSizeBits,
                                         HashAlgorithm,
                                         VAL_POLICY_PUBLIC_KEY|VAL_POLICY_PK_RSA_OAEP_WRAP,
                                         fCrossDomain, fExportAllowed,
                                         AssetId_p);
}
#endif


/*----------------------------------------------------------------------------
 * val_AsymRsaPkcs1v15WrapAllocPrivateKeyAsset
 */
#ifndef VAL_REMOVE_ASYM_RSA_PKCS1WRAP
ValStatus_t
val_AsymRsaPkcs1v15WrapAllocPrivateKeyAsset(
        const ValSize_t ModulusSizeBits,
        const ValSize_t ExponentSizeBits,
        const bool fCrossDomain,
        const bool fExportAllowed,
        ValAssetId_t * const AssetId_p)
{
#ifdef VAL_STRICT_ARGS
    if ((ExponentSizeBits < VAL_ASYM_RSA_MIN_BITS) ||
        (ExponentSizeBits > VAL_ASYM_RSA_MAX_BITS))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    return valLocal_AsymRsaAllocKeyAsset(ModulusSizeBits, ExponentSizeBits,
                                         VAL_SYM_ALGO_NONE,
                                         VAL_POLICY_PK_RSA_PKCS1_WRAP,
                                         fCrossDomain, fExportAllowed,
                                         AssetId_p);
}
#endif


/*----------------------------------------------------------------------------
 * val_AsymRsaPkcs1v15WrapAllocPublicKeyAsset
 */
#ifndef VAL_REMOVE_ASYM_RSA_PKCS1WRAP
ValStatus_t
val_AsymRsaPkcs1v15WrapAllocPublicKeyAsset(
        const ValSize_t ModulusSizeBits,
        const ValSize_t ExponentSizeBits,
        const bool fCrossDomain,
        const bool fExportAllowed,
        ValAssetId_t * const AssetId_p)
{
#ifdef VAL_STRICT_ARGS
    if ((ExponentSizeBits < VAL_ASYM_RSA_EXPO_MIN_BITS) ||
        (ExponentSizeBits > VAL_ASYM_RSA_EXPO_MAX_BITS))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    return valLocal_AsymRsaAllocKeyAsset(ModulusSizeBits, ExponentSizeBits,
                                         VAL_SYM_ALGO_NONE,
                                         VAL_POLICY_PUBLIC_KEY|VAL_POLICY_PK_RSA_PKCS1_WRAP,
                                         fCrossDomain, fExportAllowed,
                                         AssetId_p);
}
#endif


/*----------------------------------------------------------------------------
 * val_AsymRsaLoadKeyAssetPlaintext
 */
#if !defined(VAL_REMOVE_ASYM_RSAPKCS1V15_SIGN) || \
    !defined(VAL_REMOVE_ASYM_RSAPKCS1V15_VERIFY) || \
    !defined(VAL_REMOVE_ASYM_RSAPSS_SIGN) || \
    !defined(VAL_REMOVE_ASYM_RSAPSS_VERIFY) || \
    !defined(VAL_REMOVE_ASYM_RSA_OAEP) || \
    !defined(VAL_REMOVE_ASYM_RSA_PKCS1WRAP)
ValStatus_t
val_AsymRsaLoadKeyAssetPlaintext(
        const ValAsymBigInt_t * const Modulus_p,
        const ValSize_t ModulusSizeBits,
        const ValAsymBigInt_t * const Exponent_p,
        const ValSize_t ExponentSizeBits,
        const ValAssetId_t TargetAssetId,
        const ValAssetId_Optional_t KekAssetId,
        ValOctetsIn_Optional_t * const AssociatedData_p,
        const ValSize_t AssociatedDataSize,
        ValOctetsOut_Optional_t * const KeyBlob_p,
        ValSize_t * const KeyBlobSize_p)
{
    ValStatus_t funcres = VAL_INTERNAL_ERROR;
    uint8_t * AssetData_p;
    ValSize_t AssetSize;
    ValSize_t ModulusSize;

#ifdef VAL_STRICT_ARGS
    if ((Modulus_p == NULL) ||
        (ModulusSizeBits < VAL_ASYM_RSA_MIN_BITS) ||
        (ModulusSizeBits > VAL_ASYM_RSA_MAX_BITS) ||
        (Modulus_p->Data_p == NULL) ||
        (Modulus_p->ByteDataSize == 0) ||
        (Modulus_p->ByteDataSize > VAL_ASYM_DATA_SIZE_B2B(ModulusSizeBits)) ||
        (Exponent_p == NULL) ||
        (ExponentSizeBits < VAL_ASYM_RSA_EXPO_MIN_BITS) ||
        (ExponentSizeBits > VAL_ASYM_RSA_MAX_BITS) ||
        (Exponent_p->Data_p == NULL) ||
        (Exponent_p->ByteDataSize == 0) ||
        (Exponent_p->ByteDataSize > VAL_ASYM_DATA_SIZE_B2B(ExponentSizeBits)) ||
        (TargetAssetId == VAL_ASSETID_INVALID))
    {
        return VAL_BAD_ARGUMENT;
    }

    if ((KekAssetId != VAL_ASSETID_INVALID) &&
        ((AssociatedData_p == NULL) ||
         (AssociatedDataSize < VAL_KEYBLOB_AAD_MIN_SIZE) ||
         (AssociatedDataSize > VAL_KEYBLOB_AAD_MAX_SIZE) ||
         (KeyBlobSize_p == NULL)))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Determine the Asset size
    ModulusSize =  VAL_ASYM_DATA_SIZE_VWB(ModulusSizeBits);
    AssetSize = ModulusSize + VAL_ASYM_DATA_SIZE_VWB(ExponentSizeBits);
    if ((KeyBlobSize_p != NULL) &&
        ((KeyBlob_p == NULL) || (*KeyBlobSize_p < VAL_KEYBLOB_SIZE(AssetSize))))
    {
        *KeyBlobSize_p = VAL_KEYBLOB_SIZE(AssetSize);
        return VAL_BUFFER_TOO_SMALL;
    }

    // Allocate buffer for the Asset data
    AssetData_p = Adapter_Alloc((unsigned int)AssetSize);
    if (AssetData_p == NULL)
    {
        return VAL_NO_MEMORY;
    }

    // Create HW RSA Private/public Key representation for plaintext asset load
    valInternal_AsymBigIntToHW(Modulus_p, ModulusSizeBits,
                               0, 2, AssetData_p);
    valInternal_AsymBigIntToHW(Exponent_p, ExponentSizeBits,
                               1, 2, (AssetData_p + ModulusSize));

    // Initialize the asset
    if (KekAssetId == VAL_ASSETID_INVALID)
    {
        funcres = val_AssetLoadPlaintext(TargetAssetId, AssetData_p, AssetSize);
    }
    else
    {
        funcres = val_AssetLoadPlaintextExport(TargetAssetId,
                                               AssetData_p, AssetSize,
                                               KekAssetId,
                                               AssociatedData_p, AssociatedDataSize,
                                               KeyBlob_p, KeyBlobSize_p);
    }

    // Clean-up
    Adapter_Free(AssetData_p);

    return funcres;
}
#endif


/*----------------------------------------------------------------------------
 * valLocal_AsymRsaSignVerify
 */
#if !defined(VAL_REMOVE_ASYM_RSAPKCS1V15_SIGN) || \
    !defined(VAL_REMOVE_ASYM_RSAPKCS1V15_VERIFY) || \
    !defined(VAL_REMOVE_ASYM_RSAPSS_SIGN) || \
    !defined(VAL_REMOVE_ASYM_RSAPSS_VERIFY)
static ValStatus_t
valLocal_AsymRsaSignVerify(
        VexTokenPkAsset_t method,
        ValAsymKey_t * const KeyContext_p,
        ValAsymBigInt_t * const Signature_p,
        ValOctetsIn_t * const HashMessage_p,
        const ValSize_t HashMessageSize,
        ValSymContextPtr_Optional_t const HashContext_p,
        const ValSize_t SaltSize)
{
    ValStatus_t funcres = VAL_SUCCESS;
    ValOctetsIn_t * HashData_p = (ValOctetsIn_t *)HashMessage_p;
    ValSize_t HashDataSize = HashMessageSize;
    ValSymContext_t * SymContext_p = (ValSymContext_t *)HashContext_p;

#ifdef VAL_STRICT_ARGS
    if ((KeyContext_p == NULL) ||
        (KeyContext_p->KeyAssetId == VAL_ASSETID_INVALID) ||
        (KeyContext_p->DomainAssetId != VAL_ASSETID_INVALID) ||
        (KeyContext_p->ModulusSizeBits < VAL_ASYM_RSA_MIN_BITS) ||
        (KeyContext_p->ModulusSizeBits > VAL_ASYM_RSA_MAX_BITS) ||
        (HashMessage_p == NULL) ||
        (HashMessageSize == 0) ||
        (Signature_p == NULL) ||
        (Signature_p->Data_p == NULL) ||
        (Signature_p->ByteDataSize < VAL_ASYM_DATA_SIZE_B2B(KeyContext_p->ModulusSizeBits)))
    {
        return VAL_BAD_ARGUMENT;
    }

    switch (KeyContext_p->HashAlgorithm)
    {
    case VAL_SYM_ALGO_HASH_SHA1:
    case VAL_SYM_ALGO_HASH_SHA224:
    case VAL_SYM_ALGO_HASH_SHA256:
#ifndef VAL_REMOVE_SYM_ALGO_SHA512
    case VAL_SYM_ALGO_HASH_SHA384:
    case VAL_SYM_ALGO_HASH_SHA512:
#endif
        break;
    default:
        return VAL_BAD_ARGUMENT;
    }
#endif

    if (HashDataSize > VAL_ASYM_CMD_MAX_HASH_SIZE)
    {
        // Perform a partial hash first, so that remaining size is less than
        // the allowed maximum for the RSA sign operation
        if (SymContext_p == NULL)
        {
            // Allocate hash context
            funcres = val_SymAlloc(KeyContext_p->HashAlgorithm,
                                   VAL_SYM_MODE_NONE, false,
                                   (ValSymContextPtr_t *)&SymContext_p);
            if (funcres != VAL_SUCCESS)
            {
                return funcres;
            }
        }

        while (HashDataSize > VAL_ASYM_CMD_MAX_HASH_SIZE)
        {
            // Hash a block of data
            ValSize_t Size = (HashMessageSize - (4 * VAL_SYM_ALGO_HASH_BLOCK_SIZE)) &
                             (VAL_SIZE_MAX_DMA & ~(VAL_SYM_ALGO_HASH_BLOCK_SIZE -1));

            funcres = val_SymHashUpdate((ValSymContextPtr_t const)SymContext_p,
                                        (ValOctetsIn_t *)HashData_p,
                                        Size);
            if (funcres != VAL_SUCCESS)
            {
                goto error_func_exit;
            }

            // Adjust references
            HashData_p   += Size;
            HashDataSize -= Size;
        }
    }

    {
        VexToken_Command_t t_cmd;
        VexToken_Result_t t_res;
        uint8_t * SigData_p;
        uint32_t SignatureSize = (uint32_t)VAL_ASYM_DATA_SIZE_VWB(KeyContext_p->ModulusSizeBits);
        ValAssetId_t TempAssetId = VAL_ASSETID_INVALID;

        // Allocate a temporary buffer for the signature
        SigData_p = Adapter_Alloc(SignatureSize);
        if (SigData_p == NULL)
        {
            funcres = VAL_NO_MEMORY;
            goto error_func_exit;
        }

        if ((method == VEXTOKEN_PKASSET_RSA_PKCS1V1_5_VERIFY) ||
            (method == VEXTOKEN_PKASSET_RSA_PSS_VERIFY))
        {
            // Convert the signature to from application to HW format
            valInternal_AsymBigIntToHW(Signature_p,
                                       KeyContext_p->ModulusSizeBits,
                                       0, 1, SigData_p);
        }

        // Format service request
        t_cmd.OpCode = VEXTOKEN_OPCODE_PUBLIC_KEY;
        t_cmd.SubCode = VEXTOKEN_SUBCODE_PK_WITHASSETS;
        t_cmd.Service.PkAssetSignVerify.Method = method;
        t_cmd.Service.PkAssetSignVerify.ModulusSizeInBits = (uint32_t)KeyContext_p->ModulusSizeBits;
        t_cmd.Service.PkAssetSignVerify.KeyAssetId = KeyContext_p->KeyAssetId;
        t_cmd.Service.PkAssetSignVerify.DomainAssetId = VAL_ASSETID_INVALID;
        if (SymContext_p == NULL)
        {
            t_cmd.Service.PkAssetSignVerify.DigestAssetId = VAL_ASSETID_INVALID;
            t_cmd.Service.PkAssetSignVerify.TotalMessageSize = HashDataSize;
        }
        else
        {
            if (SymContext_p->Service.Hash.TempAssetId != VAL_ASSETID_INVALID)
            {
                t_cmd.Service.PkAssetSignVerify.DigestAssetId =
                    SymContext_p->Service.Hash.TempAssetId;
            }
            else
            {
                funcres = valInternal_SymCreateTempAsset(SymContext_p, &TempAssetId);
                if (funcres != VAL_SUCCESS)
                {
                    goto error_func_exit;
                }
                t_cmd.Service.PkAssetSignVerify.DigestAssetId = TempAssetId;
            }
            t_cmd.Service.PkAssetSignVerify.TotalMessageSize =
                SymContext_p->Service.Hash.TotalMessageLength + (uint64_t)HashDataSize;
        }
        t_cmd.Service.PkAssetSignVerify.HashData_p = (const uint8_t *)HashData_p;
        t_cmd.Service.PkAssetSignVerify.HashDataSize = (uint32_t)HashDataSize;
        if ((method == VEXTOKEN_PKASSET_RSA_PSS_SIGN) ||
            (method == VEXTOKEN_PKASSET_RSA_PSS_VERIFY))
        {
            t_cmd.Service.PkAssetSignVerify.SaltSize = (uint32_t)SaltSize;
        }
        else
        {
            t_cmd.Service.PkAssetSignVerify.SaltSize = 0;
        }
        t_cmd.Service.PkAssetSignVerify.Sign_p = SigData_p;
        t_cmd.Service.PkAssetSignVerify.SignSize = SignatureSize;
        t_res.Result = VEXTOKEN_RESULT_NO_ERROR;

        // Exchange service request with the next driver level
        funcres = val_ExchangeToken(&t_cmd, &t_res);
        if (funcres == VAL_SUCCESS)
        {
            // Check for errors
            if (t_res.Result == VEXTOKEN_RESULT_NO_ERROR)
            {
                if ((method == VEXTOKEN_PKASSET_RSA_PKCS1V1_5_SIGN) ||
                    (method == VEXTOKEN_PKASSET_RSA_PSS_SIGN))
                {
                    // Convert the signature from HW to application format
                    valInternal_AsymBigIntFromHW(SigData_p,
                                                 KeyContext_p->ModulusSizeBits,
                                                 Signature_p);
                }
            }
            else
            {
                funcres = (ValStatus_t)t_res.Result;
                LOG_WARN("Abort - %s()=%d\n", __func__, t_res.Result);
            }
        }

        if (TempAssetId != VAL_ASSETID_INVALID)
        {
            (void)val_AssetFree(TempAssetId);
        }
        Adapter_Free(SigData_p);
    }

    // Clean-up
error_func_exit:
    if (((funcres == VAL_SUCCESS) || (HashContext_p == NULL)) &&
        (SymContext_p != NULL))
    {
        (void)val_SymRelease((ValSymContextPtr_t const)SymContext_p);
    }

    return funcres;
}
#endif


/*----------------------------------------------------------------------------
 * val_AsymRsaPkcs1v15Sign
 */
#ifndef VAL_REMOVE_ASYM_RSAPKCS1V15_SIGN
ValStatus_t
val_AsymRsaPkcs1v15Sign(
        ValAsymKey_t * const KeyContext_p,
        ValAsymBigInt_t * const Signature_p,
        ValOctetsIn_t * const HashMessage_p,
        const ValSize_t HashMessageSize,
        ValSymContextPtr_Optional_t const HashContext_p)
{
    return valLocal_AsymRsaSignVerify(VEXTOKEN_PKASSET_RSA_PKCS1V1_5_SIGN,
                                      KeyContext_p, Signature_p,
                                      HashMessage_p, HashMessageSize,
                                      HashContext_p, 0);
}
#endif


/*----------------------------------------------------------------------------
 * val_AsymRsaPkcs1v15Verify
 */
#ifndef VAL_REMOVE_ASYM_RSAPKCS1V15_VERIFY
ValStatus_t
val_AsymRsaPkcs1v15Verify(
        ValAsymKey_t * const KeyContext_p,
        ValAsymBigInt_t * const Signature_p,
        ValOctetsIn_t * const HashMessage_p,
        const ValSize_t HashMessageSize,
        ValSymContextPtr_Optional_t const HashContext_p)
{
    return valLocal_AsymRsaSignVerify(VEXTOKEN_PKASSET_RSA_PKCS1V1_5_VERIFY,
                                      KeyContext_p, Signature_p,
                                      HashMessage_p, HashMessageSize,
                                      HashContext_p, 0);
}
#endif


/*----------------------------------------------------------------------------
 * val_AsymRsaPssSign
 */
#ifndef VAL_REMOVE_ASYM_RSAPSS_SIGN
ValStatus_t
val_AsymRsaPssSign(
        ValAsymKey_t * const KeyContext_p,
        ValAsymBigInt_t * const Signature_p,
        ValOctetsIn_t * const HashMessage_p,
        const ValSize_t HashMessageSize,
        ValSymContextPtr_Optional_t const HashContext_p,
        const ValSize_t SaltSize)
{
    return valLocal_AsymRsaSignVerify(VEXTOKEN_PKASSET_RSA_PSS_SIGN,
                                      KeyContext_p, Signature_p,
                                      HashMessage_p, HashMessageSize,
                                      HashContext_p, SaltSize);
}
#endif


/*----------------------------------------------------------------------------
 * val_AsymRsaPssVerify
 */
#ifndef VAL_REMOVE_ASYM_RSAPSS_VERIFY
ValStatus_t
val_AsymRsaPssVerify(
        ValAsymKey_t * const KeyContext_p,
        ValAsymBigInt_t * const Signature_p,
        ValOctetsIn_t * const HashMessage_p,
        const ValSize_t HashMessageSize,
        ValSymContextPtr_Optional_t const HashContext_p,
        const ValSize_t SaltSize)
{
    return valLocal_AsymRsaSignVerify(VEXTOKEN_PKASSET_RSA_PSS_VERIFY,
                                      KeyContext_p, Signature_p,
                                      HashMessage_p, HashMessageSize,
                                      HashContext_p, SaltSize);
}
#endif


/*----------------------------------------------------------------------------
 * valLocal_AsymRsaWrapUnwrap
 */
#if !defined(VAL_REMOVE_ASYM_RSA_OAEP) || \
    !defined(VAL_REMOVE_ASYM_RSAPKCS1V15_VERIFY)
static ValStatus_t
valLocal_AsymRsaWrapUnwrap(
        const VexTokenPkAsset_t method,
        ValAsymKey_t * const KeyContext_p,
        const ValAssetId_t AssetId,
        ValOctetsIn_t * const AdditionalInput_p,
        const ValSize_t AdditionalInputSize,
        uint8_t * const WrappedData_p,
        ValSize_t * const DataSize_p)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres = VAL_SUCCESS;

#ifdef VAL_STRICT_ARGS
    if ((KeyContext_p == NULL) ||
        (KeyContext_p->KeyAssetId == VAL_ASSETID_INVALID) ||
        (KeyContext_p->DomainAssetId != VAL_ASSETID_INVALID) ||
        (KeyContext_p->ModulusSizeBits < VAL_ASYM_RSA_MIN_BITS) ||
        (KeyContext_p->ModulusSizeBits > VAL_ASYM_RSA_MAX_BITS) ||
        (AdditionalInputSize > 208))
    {
        return VAL_BAD_ARGUMENT;
    }

    switch (KeyContext_p->HashAlgorithm)
    {
    case VAL_SYM_ALGO_HASH_SHA1:
        if (((method == VEXTOKEN_PKASSET_RSA_OAEP_WRAP_HASH) ||
             (method == VEXTOKEN_PKASSET_RSA_OAEP_UNWRAP_HASH)) &&
            (AdditionalInputSize != (160 / 8)))
        {
            return VAL_BAD_ARGUMENT;
        }
        break;
    case VAL_SYM_ALGO_HASH_SHA224:
        if (((method == VEXTOKEN_PKASSET_RSA_OAEP_WRAP_HASH) ||
             (method == VEXTOKEN_PKASSET_RSA_OAEP_UNWRAP_HASH)) &&
            (AdditionalInputSize != (224 / 8)))
        {
            return VAL_BAD_ARGUMENT;
        }
        break;
    case VAL_SYM_ALGO_HASH_SHA256:
        if (((method == VEXTOKEN_PKASSET_RSA_OAEP_WRAP_HASH) ||
             (method == VEXTOKEN_PKASSET_RSA_OAEP_UNWRAP_HASH)) &&
            (AdditionalInputSize != (256 / 8)))
        {
            return VAL_BAD_ARGUMENT;
        }
        break;
#ifndef VAL_REMOVE_SYM_ALGO_SHA512
    case VAL_SYM_ALGO_HASH_SHA384:
        if (((method == VEXTOKEN_PKASSET_RSA_OAEP_WRAP_HASH) ||
             (method == VEXTOKEN_PKASSET_RSA_OAEP_UNWRAP_HASH)) &&
            (AdditionalInputSize != (384 / 8)))
        {
            return VAL_BAD_ARGUMENT;
        }
        break;
    case VAL_SYM_ALGO_HASH_SHA512:
        if (((method == VEXTOKEN_PKASSET_RSA_OAEP_WRAP_HASH) ||
             (method == VEXTOKEN_PKASSET_RSA_OAEP_UNWRAP_HASH)) &&
            (AdditionalInputSize != (512 / 8)))
        {
            return VAL_BAD_ARGUMENT;
        }
        break;
#endif
#ifndef VAL_REMOVE_ASYM_RSA_PKCS1WRAP
    case VAL_SYM_ALGO_NONE:
        if ((method == VEXTOKEN_PKASSET_RSA_PKCS1V15_WRAP) ||
            (method == VEXTOKEN_PKASSET_RSA_PKCS1V15_UNWRAP))
        {
            break;
        }
        return VAL_INVALID_ALGORITHM;
#endif
    default:
        return VAL_INVALID_ALGORITHM;
    }
#endif

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_PUBLIC_KEY;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_PK_WITHASSETS;
    t_cmd.Service.PkAssetWrap.Method = method;
    t_cmd.Service.PkAssetWrap.ModulusSizeInBits = (uint32_t)KeyContext_p->ModulusSizeBits;
    t_cmd.Service.PkAssetWrap.KeyAssetId = KeyContext_p->KeyAssetId;
    t_cmd.Service.PkAssetWrap.AssetId = AssetId;
    t_cmd.Service.PkAssetWrap.AdditionalInputSize = (uint32_t)AdditionalInputSize;
    if (AdditionalInputSize > 0)
    {
        memcpy(t_cmd.Service.PkAssetWrap.AdditionalInput, AdditionalInput_p, AdditionalInputSize);
    }
    t_cmd.Service.PkAssetWrap.Data_p = (const uint8_t *)WrappedData_p;
    t_cmd.Service.PkAssetWrap.DataSize = (uint32_t)*DataSize_p;
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if ((funcres == VAL_SUCCESS) &&
        (t_res.Result != VEXTOKEN_RESULT_NO_ERROR))
    {
        // Error
        funcres = (ValStatus_t)t_res.Result;
        LOG_WARN("Abort - %s()=%d\n", __func__, funcres);
    }

    return funcres;
}
#endif


#ifndef VAL_REMOVE_ASYM_RSA_OAEP
ValStatus_t
val_AsymRsaOaepWrapString(
        ValAsymKey_t * const KeyContext_p,
        const ValAssetId_t AssetId,
        ValOctetsIn_t * const AdditionalInput_p,
        const ValSize_t AdditionalInputSize,
        ValOctetsOut_t * const WrappedData_p,
        ValSize_t * const WrappedDataSize_p)
{
    return valLocal_AsymRsaWrapUnwrap(VEXTOKEN_PKASSET_RSA_OAEP_WRAP_STRING,
                                      KeyContext_p, AssetId,
                                      AdditionalInput_p, AdditionalInputSize,
                                      WrappedData_p, WrappedDataSize_p);
}
#endif

#ifndef VAL_REMOVE_ASYM_RSA_OAEP
ValStatus_t
val_AsymRsaOaepWrapHash(
        ValAsymKey_t * const KeyContext_p,
        const ValAssetId_t AssetId,
        ValOctetsIn_t * const AdditionalInput_p,
        const ValSize_t AdditionalInputSize,
        ValOctetsOut_t * const WrappedData_p,
        ValSize_t * const WrappedDataSize_p)
{
    return valLocal_AsymRsaWrapUnwrap(VEXTOKEN_PKASSET_RSA_OAEP_WRAP_HASH,
                                      KeyContext_p, AssetId,
                                      AdditionalInput_p, AdditionalInputSize,
                                      WrappedData_p, WrappedDataSize_p);
}
#endif

#ifndef VAL_REMOVE_ASYM_RSA_OAEP
ValStatus_t
val_AsymRsaOaepUnwrapString(
        ValAsymKey_t * const KeyContext_p,
        const ValAssetId_t AssetId,
        ValOctetsIn_t * const AdditionalInput_p,
        const ValSize_t AdditionalInputSize,
        ValOctetsIn_t * const WrappedData_p,
        const ValSize_t WrappedDataSize)
{
    union
    {
        ValOctetsIn_t * c_p;
        uint8_t * n_p;
    } ConvWrappedData;
    ValSize_t ConvWrappedDataSize = WrappedDataSize;
    ConvWrappedData.c_p = WrappedData_p;
    return valLocal_AsymRsaWrapUnwrap(VEXTOKEN_PKASSET_RSA_OAEP_UNWRAP_STRING,
                                      KeyContext_p, AssetId,
                                      AdditionalInput_p, AdditionalInputSize,
                                      ConvWrappedData.n_p, &ConvWrappedDataSize);
}
#endif

#ifndef VAL_REMOVE_ASYM_RSA_OAEP
ValStatus_t
val_AsymRsaOaepUnwrapHash(
        ValAsymKey_t * const KeyContext_p,
        const ValAssetId_t AssetId,
        ValOctetsIn_t * const AdditionalInput_p,
        const ValSize_t AdditionalInputSize,
        ValOctetsIn_t * const WrappedData_p,
        const ValSize_t WrappedDataSize)
{
    union
    {
        ValOctetsIn_t * c_p;
        uint8_t * n_p;
    } ConvWrappedData;
    ValSize_t ConvWrappedDataSize = WrappedDataSize;
    ConvWrappedData.c_p = WrappedData_p;
    return valLocal_AsymRsaWrapUnwrap(VEXTOKEN_PKASSET_RSA_OAEP_UNWRAP_HASH,
                                      KeyContext_p, AssetId,
                                      AdditionalInput_p, AdditionalInputSize,
                                      ConvWrappedData.n_p, &ConvWrappedDataSize);
}
#endif


#ifndef VAL_REMOVE_ASYM_RSA_PKCS1WRAP
ValStatus_t
val_AsymRsaPkcs1v15Wrap(
        ValAsymKey_t * const KeyContext_p,
        const ValAssetId_t AssetId,
        ValOctetsOut_t * const WrappedData_p,
        ValSize_t * const WrappedDataSize_p)
{
    return valLocal_AsymRsaWrapUnwrap(VEXTOKEN_PKASSET_RSA_PKCS1V15_WRAP,
                                      KeyContext_p, AssetId,
                                      NULL, 0,
                                      WrappedData_p, WrappedDataSize_p);
}
#endif

#ifndef VAL_REMOVE_ASYM_RSA_PKCS1WRAP
ValStatus_t
val_AsymRsaPkcs1v15Unwrap(
        ValAsymKey_t * const KeyContext_p,
        const ValAssetId_t AssetId,
        ValOctetsIn_t * const WrappedData_p,
        const ValSize_t WrappedDataSize)
{
    union
    {
        ValOctetsIn_t * c_p;
        uint8_t * n_p;
    } ConvWrappedData;
    ValSize_t ConvWrappedDataSize = WrappedDataSize;
    ConvWrappedData.c_p = WrappedData_p;
    return valLocal_AsymRsaWrapUnwrap(VEXTOKEN_PKASSET_RSA_PKCS1V15_UNWRAP,
                                      KeyContext_p, AssetId,
                                      NULL, 0,
                                      ConvWrappedData.n_p, &ConvWrappedDataSize);
}
#endif


/* end of file adapter_val_asym_rsa.c */
