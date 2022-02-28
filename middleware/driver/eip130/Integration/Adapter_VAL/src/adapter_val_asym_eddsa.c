/* adapter_val_asym_eddsa.c
 *
 * Implementation of the VAL API.
 *
 * This file implements the asymmetric crypto Edwards DSA related services.
 */

/*****************************************************************************
* Copyright (c) 2016-2019 INSIDE Secure B.V. All Rights Reserved.
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

#if !defined(VAL_REMOVE_ASYM_EDSA_SIGN) || \
    !defined(VAL_REMOVE_ASYM_EDDSA_VERIFY)

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
 * val_AsymEddsaAllocKeyAsset
 */
ValStatus_t
val_AsymEddsaAllocKeyAsset(
        const bool fPublicKey,
        const bool fCrossDomain,
        const bool fExportAllowed,
        ValAssetId_t * const AssetId_p)
{
    ValPolicyMask_t AssetPolicy;

#ifdef VAL_STRICT_ARGS
    if (AssetId_p == NULL)
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Determine Asset policy
    AssetPolicy = VAL_POLICY_PK_ECC_ECDSA_SIGN + VAL_POLICY_SHA512;
    if (fPublicKey)
    {
        AssetPolicy |= VAL_POLICY_PUBLIC_KEY;
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
    return val_AssetAlloc(AssetPolicy, VAL_ASYM_DATA_SIZE_VWB(255),
                          false, false,
                          VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                          AssetId_p);
}


/*----------------------------------------------------------------------------
 * val_AsymEddsaLoadKeyAssetPlaintext
 */
ValStatus_t
val_AsymEddsaLoadKeyAssetPlaintext(
        const ValAsymBigInt_t * const KeyValue_p,
        const ValAssetId_t TargetAssetId,
        const ValAssetId_Optional_t KekAssetId,
        ValOctetsIn_Optional_t * const AssociatedData_p,
        const ValSize_t AssociatedDataSize,
        ValOctetsOut_Optional_t * const KeyBlob_p,
        ValSize_t * const KeyBlobSize_p)
{
    return val_AsymEccLoadPrivateKeyAssetPlaintext(KeyValue_p, 255,
                                                   TargetAssetId,
                                                   KekAssetId,
                                                   AssociatedData_p, AssociatedDataSize,
                                                   KeyBlob_p, KeyBlobSize_p);
}


/*----------------------------------------------------------------------------
 * val_AsymEddsaGenKeyPair
 */
ValStatus_t
val_AsymEddsaGenKeyPair(
        ValAsymKey_t * const PubKey_p,
        ValAsymKey_t * const PrivKey_p,
        const ValAssetId_Optional_t KekAssetId,
        ValOctetsIn_Optional_t * const AssociatedData_p,
        const ValSize_t AssociatedDataSize,
        ValAsymBigInt_t * const PubKeyValue_p,
        ValOctetsOut_Optional_t * const PrivKeyBlob_p,
        ValSize_t * const PrivKeyBlobSize_p)
{
    ValStatus_t funcres;
    uint8_t * Data_p = NULL;
    ValSize_t PrvKeyDataSize = 0;
    ValSize_t PubKeyDataSize = 0;
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;

#ifdef VAL_STRICT_ARGS
    if ((PubKey_p == NULL) ||
        (PubKey_p->KeyAssetId == VAL_ASSETID_INVALID) ||
        (PubKey_p->DomainAssetId == VAL_ASSETID_INVALID) ||
        (PubKey_p->ModulusSizeBits != 255) ||
        (PrivKey_p == NULL) ||
        (PrivKey_p->KeyAssetId == VAL_ASSETID_INVALID) ||
        (PrivKey_p->DomainAssetId != PubKey_p->DomainAssetId) ||
        (PrivKey_p->ModulusSizeBits != PubKey_p->ModulusSizeBits))
    {
        return VAL_BAD_ARGUMENT;
    }

    if ((KekAssetId != VAL_ASSETID_INVALID) &&
        ((AssociatedData_p == NULL) ||
         (AssociatedDataSize < VAL_KEYBLOB_AAD_MIN_SIZE) ||
         (AssociatedDataSize > VAL_KEYBLOB_AAD_MAX_SIZE) ||
         (PrivKeyBlobSize_p == NULL)))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Determine the keyblob size for the private key
    PrvKeyDataSize = VAL_KEYBLOB_SIZE(VAL_ASYM_DATA_SIZE_VWB(PrivKey_p->ModulusSizeBits));
    if ((PrivKeyBlobSize_p != NULL) &&
        ((PrivKeyBlob_p == NULL) || (*PrivKeyBlobSize_p < PrvKeyDataSize)))
    {
        *PrivKeyBlobSize_p = PrvKeyDataSize;
        return VAL_BUFFER_TOO_SMALL;
    }

    if (PubKeyValue_p != NULL)
    {
        // Validate application format public key data size
#ifdef VAL_STRICT_ARGS
        PubKeyDataSize = VAL_ASYM_DATA_SIZE_B2B(PubKey_p->ModulusSizeBits);
        if ((PubKeyValue_p->Data_p == NULL) ||
            (PubKeyValue_p->ByteDataSize < PubKeyDataSize))
        {
            return VAL_BAD_ARGUMENT;
        }
#endif

        // Determine the HW size for the public key
        PubKeyDataSize = VAL_ASYM_DATA_SIZE_VWB(PubKey_p->ModulusSizeBits);

        // Allocate buffer for the public key data
        Data_p = Adapter_Alloc((unsigned int)PubKeyDataSize);
        if (Data_p == NULL)
        {
            return VAL_NO_MEMORY;
        }
    }

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_PUBLIC_KEY;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_PK_WITHASSETS;
    t_cmd.Service.PkAssetGenKey.Method = VEXTOKEN_PKASSET_EDDSA_GEN_KEYPAIR;
    t_cmd.Service.PkAssetGenKey.ModulusSizeInBits = (uint32_t)PubKey_p->ModulusSizeBits;
    t_cmd.Service.PkAssetGenKey.DivisorSizeInBits = (uint32_t)PubKey_p->ModulusSizeBits;
    t_cmd.Service.PkAssetGenKey.PubKeyAssetId = PubKey_p->KeyAssetId;
    t_cmd.Service.PkAssetGenKey.PrivKeyAssetId = PrivKey_p->KeyAssetId;
    t_cmd.Service.PkAssetGenKey.DomainAssetId = PubKey_p->DomainAssetId;
    t_cmd.Service.PkAssetGenKey.PubKey_p = (uint8_t *)Data_p;
    t_cmd.Service.PkAssetGenKey.PubKeySize = (uint32_t)PubKeyDataSize;
    t_cmd.Service.PkAssetGenKey.KekAssetId = (uint32_t)KekAssetId;
    if ((KekAssetId != VAL_ASSETID_INVALID) &&
        (PrivKeyBlob_p != NULL) && (PrivKeyBlobSize_p != NULL))
    {
        memcpy(t_cmd.Service.PkAssetGenKey.AssociatedData, AssociatedData_p, AssociatedDataSize);
        t_cmd.Service.PkAssetGenKey.AssociatedDataSize = (uint32_t)AssociatedDataSize;
        t_cmd.Service.PkAssetGenKey.KeyBlob_p = (uint8_t *)PrivKeyBlob_p;
        t_cmd.Service.PkAssetGenKey.KeyBlobSize = (uint32_t)*PrivKeyBlobSize_p;
    }
    else
    {
        t_cmd.Service.PkAssetGenKey.AssociatedDataSize = 0;
        t_cmd.Service.PkAssetGenKey.KeyBlob_p = NULL;
        t_cmd.Service.PkAssetGenKey.KeyBlobSize = 0;
    }
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if (funcres == VAL_SUCCESS)
    {
        // Check for errors
        if (t_res.Result == VEXTOKEN_RESULT_NO_ERROR)
        {
            // Convert the public key from HW to application format
            if (PubKeyValue_p != NULL)
            {
                valInternal_AsymBigIntFromHW(Data_p,
                                             PubKey_p->ModulusSizeBits,
                                             PubKeyValue_p);
            }

            // Set the actual keyblob size
            if (PrivKeyBlobSize_p != NULL)
            {
                *PrivKeyBlobSize_p = PrvKeyDataSize;
            }
        }
        else
        {
            funcres = (ValStatus_t)t_res.Result;
            LOG_WARN("Abort - %s()=%d\n", __func__, t_res.Result);
        }
    }

    // Clean-up
    if (Data_p != NULL)
    {
        Adapter_Free(Data_p);
    }

    return funcres;
}


/*----------------------------------------------------------------------------
 * val_AsymEddsaGenPublicKey
 */
ValStatus_t
val_AsymEddsaGenPublicKey(
        ValAsymKey_t * const PubKey_p,
        ValAsymKey_t * const PrivKey_p,
        ValAsymBigInt_t * const PubKeyValue_p)
{
    ValStatus_t funcres;
    uint8_t * Data_p = NULL;
    ValSize_t DataSize = 0;
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;

#ifdef VAL_STRICT_ARGS
    if ((PubKey_p == NULL) ||
        (PubKey_p->KeyAssetId == VAL_ASSETID_INVALID) ||
        (PubKey_p->DomainAssetId == VAL_ASSETID_INVALID) ||
        (PubKey_p->ModulusSizeBits != 255) ||
        (PrivKey_p == NULL) ||
        (PrivKey_p->KeyAssetId == VAL_ASSETID_INVALID) ||
        (PrivKey_p->DomainAssetId != PubKey_p->DomainAssetId) ||
        (PrivKey_p->ModulusSizeBits != PubKey_p->ModulusSizeBits))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    if (PubKeyValue_p != NULL)
    {
#ifdef VAL_STRICT_ARGS
        // Validate application format public key data size
        DataSize = VAL_ASYM_DATA_SIZE_B2B(PubKey_p->ModulusSizeBits);
        if ((PubKeyValue_p->Data_p == NULL) ||
            (PubKeyValue_p->ByteDataSize < DataSize))
        {
            return VAL_BAD_ARGUMENT;
        }
#endif

        // Determine the HW size for the public key
        DataSize = VAL_ASYM_DATA_SIZE_VWB(PubKey_p->ModulusSizeBits);

        // Allocate buffer for the public key data
        Data_p = Adapter_Alloc((unsigned int)DataSize);
        if (Data_p == NULL)
        {
            return VAL_NO_MEMORY;
        }
    }

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_PUBLIC_KEY;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_PK_WITHASSETS;
    t_cmd.Service.PkAssetGenKey.Method = VEXTOKEN_PKASSET_EDDSA_GEN_PUBKEY;
    t_cmd.Service.PkAssetGenKey.ModulusSizeInBits = (uint32_t)PubKey_p->ModulusSizeBits;
    t_cmd.Service.PkAssetGenKey.DivisorSizeInBits = (uint32_t)PubKey_p->ModulusSizeBits;
    t_cmd.Service.PkAssetGenKey.PubKeyAssetId = PubKey_p->KeyAssetId;
    t_cmd.Service.PkAssetGenKey.PrivKeyAssetId = PrivKey_p->KeyAssetId;
    t_cmd.Service.PkAssetGenKey.DomainAssetId = PubKey_p->DomainAssetId;
    t_cmd.Service.PkAssetGenKey.PubKey_p = (uint8_t *)Data_p;
    t_cmd.Service.PkAssetGenKey.PubKeySize = (uint32_t)DataSize;
    t_cmd.Service.PkAssetGenKey.KekAssetId = VAL_ASSETID_INVALID;
    t_cmd.Service.PkAssetGenKey.AssociatedDataSize = 0;
    t_cmd.Service.PkAssetGenKey.KeyBlob_p = NULL;
    t_cmd.Service.PkAssetGenKey.KeyBlobSize = 0;
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if (funcres == VAL_SUCCESS)
    {
        // Check for errors
        if (t_res.Result == VEXTOKEN_RESULT_NO_ERROR)
        {
            // Convert the public key from HW to application format
            if (PubKeyValue_p != NULL)
            {
                valInternal_AsymBigIntFromHW(Data_p,
                                             PubKey_p->ModulusSizeBits,
                                             PubKeyValue_p);
            }
        }
        else
        {
            funcres = (ValStatus_t)t_res.Result;
            LOG_WARN("Abort - %s()=%d\n", __func__, t_res.Result);
        }
    }

    // Clean-up
    if (Data_p != NULL)
    {
        Adapter_Free(Data_p);
    }

    return funcres;
}


/*----------------------------------------------------------------------------
 * val_AsymEddsaSign
 */
#ifndef VAL_REMOVE_ASYM_EDDSA_SIGN
ValStatus_t
val_AsymEddsaSign(
        ValAsymKey_t * const PrvKeyContext_p,
        ValAsymKey_t * const PubKeyContext_p,
        ValAsymSign_t * const Signature_p,
        ValOctetsIn_t * const HashMessage_p,
        const ValSize_t HashMessageSize)
{
    ValStatus_t funcres = VAL_SUCCESS;
    ValPolicyMask_t AssetPolicy = VAL_POLICY_TEMP_MAC|VAL_POLICY_SHA512;
    ValAssetId_t StateAssetId = VAL_ASSETID_INVALID;
    ValSymContext_t * SymContext_p = NULL;
    ValOctetsIn_t * HashData_p;
    ValSize_t HashDataSize;
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;

#ifdef VAL_STRICT_ARGS
    if ((PrvKeyContext_p == NULL) || (PubKeyContext_p == NULL) ||
        (PrvKeyContext_p->KeyAssetId == VAL_ASSETID_INVALID) ||
        (PrvKeyContext_p->DomainAssetId == VAL_ASSETID_INVALID) ||
        (PrvKeyContext_p->ModulusSizeBits != 255) ||
        (PubKeyContext_p->KeyAssetId == VAL_ASSETID_INVALID) ||
        (PubKeyContext_p->DomainAssetId != PrvKeyContext_p->DomainAssetId) ||
        (PubKeyContext_p->ModulusSizeBits != PrvKeyContext_p->ModulusSizeBits) ||
        (HashMessage_p == NULL) || (HashMessageSize == 0) ||
        (Signature_p == NULL) ||
        (Signature_p->r.Data_p == NULL) ||
        (Signature_p->r.ByteDataSize < VAL_ASYM_DATA_SIZE_B2B(PrvKeyContext_p->ModulusSizeBits)) ||
        (Signature_p->s.Data_p == NULL) ||
        (Signature_p->s.ByteDataSize < VAL_ASYM_DATA_SIZE_B2B(PrvKeyContext_p->ModulusSizeBits)))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Allocate hash context
    funcres = val_SymAlloc(VAL_SYM_ALGO_HASH_SHA512, VAL_SYM_MODE_NONE, false,
                           (ValSymContextPtr_t *)&SymContext_p);
    if (funcres != VAL_SUCCESS)
    {
        return funcres;
    }

    // Allocate Asset for intermediate digest and
    // mark hash setup done (initial hash is done during EdDSA operation)
    if (!val_IsAccessSecure())
    {
        AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
    }
    funcres = val_AssetAlloc(AssetPolicy, (512 / 8),
                             false, false,
                             VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                             &SymContext_p->Service.Hash.TempAssetId);
    if (funcres != VAL_SUCCESS)
    {
        goto error_func_exit;
    }
    SymContext_p->fInitDone = true;

    // Set initial message size
    HashData_p = (ValOctetsIn_t *)HashMessage_p;
    if (HashMessageSize > 96)
    {
        HashDataSize = 96;
    }
    else
    {
        HashDataSize = HashMessageSize;
    }

    // Format SIGN-Initial request
    t_cmd.OpCode = VEXTOKEN_OPCODE_PUBLIC_KEY;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_PK_WITHASSETS;
    t_cmd.Service.PkAssetSignVerify.Method = VEXTOKEN_PKASSET_EDDSA_SIGN_INITIAL;
    t_cmd.Service.PkAssetSignVerify.ModulusSizeInBits = (uint32_t)PrvKeyContext_p->ModulusSizeBits;
    t_cmd.Service.PkAssetSignVerify.KeyAssetId = PrvKeyContext_p->KeyAssetId;
    t_cmd.Service.PkAssetSignVerify.DomainAssetId = PrvKeyContext_p->DomainAssetId;
    t_cmd.Service.PkAssetSignVerify.DigestAssetId = SymContext_p->Service.Hash.TempAssetId;
    t_cmd.Service.PkAssetSignVerify.HashData_p = (const uint8_t *)HashData_p;
    t_cmd.Service.PkAssetSignVerify.HashDataSize = (uint32_t)HashDataSize;
    t_cmd.Service.PkAssetSignVerify.TotalMessageSize = 0;
    t_cmd.Service.PkAssetSignVerify.SaltSize = 0;
    t_cmd.Service.PkAssetSignVerify.Sign_p = NULL;
    t_cmd.Service.PkAssetSignVerify.SignSize = 0;
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;
    t_res.Service.PkAssetSignVerify.StateAssetId = VAL_ASSETID_INVALID;

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if (funcres == VAL_SUCCESS)
    {
        // Check for errors
        if (t_res.Result == VEXTOKEN_RESULT_NO_ERROR)
        {
            // Get State asset of the operation
            StateAssetId = t_res.Service.PkAssetSignVerify.StateAssetId;
        }
        else
        {
            funcres = (ValStatus_t)t_res.Result;
            LOG_WARN("Abort - %s()=%d\n", __func__, t_res.Result);
            goto error_func_exit;
        }
    }

    // Hash the middle section of the message if needed
    if (HashMessageSize > 96)
    {
        HashData_p  += 96;
        HashDataSize = HashMessageSize - 96;
    }
    while (HashDataSize > VAL_ASYM_CMD_MAX_HASH_SIZE)
    {
        // Hash a block of data
        ValSize_t Size = (HashDataSize - (4 * VAL_SYM_ALGO_HASH_SHA512_BLOCK_SIZE)) &
                         (VAL_SIZE_MAX_DMA & ~(VAL_SYM_ALGO_HASH_SHA512_BLOCK_SIZE -1));

        funcres = val_SymHashUpdate((ValSymContextPtr_t const)SymContext_p,
                                    (ValOctetsIn_t *)HashData_p, Size);
        if (funcres != VAL_SUCCESS)
        {
            goto error_func_exit;
        }

        // Adjust references
        HashData_p   += Size;
        HashDataSize -= Size;
    }

    // Format SIGN-Update request
    t_cmd.Service.PkAssetSignVerify.Method = VEXTOKEN_PKASSET_EDDSA_SIGN_UPDATE;
    t_cmd.Service.PkAssetSignVerify.KeyAssetId = PubKeyContext_p->KeyAssetId;
    t_cmd.Service.PkAssetSignVerify.DomainAssetId = VAL_ASSETID_INVALID;
    t_cmd.Service.PkAssetSignVerify.DigestAssetId = StateAssetId;
    t_cmd.Service.PkAssetSignVerify.HashData_p = (const uint8_t *)HashData_p;
    t_cmd.Service.PkAssetSignVerify.HashDataSize = (uint32_t)HashDataSize;
    t_cmd.Service.PkAssetSignVerify.TotalMessageSize = HashMessageSize;

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if ((funcres == VAL_SUCCESS) && (t_res.Result != VEXTOKEN_RESULT_NO_ERROR))
    {
        funcres = (ValStatus_t)t_res.Result;
        LOG_WARN("Abort - %s()=%d\n", __func__, t_res.Result);
        goto error_func_exit;
    }

    // Hash the middle section of the message if needed
    HashData_p   = (ValOctetsIn_t *)HashMessage_p;
    HashDataSize = HashMessageSize;
    if (HashMessageSize > 64)
    {
        HashData_p   += 64;
        HashDataSize -= 64;
    }
    while (HashDataSize > VAL_ASYM_CMD_MAX_HASH_SIZE)
    {
        // Hash a block of data
        ValSize_t Size = (HashDataSize - (4 * VAL_SYM_ALGO_HASH_SHA512_BLOCK_SIZE)) &
                         (VAL_SIZE_MAX_DMA & ~(VAL_SYM_ALGO_HASH_SHA512_BLOCK_SIZE -1));

        funcres = val_SymHashUpdate((ValSymContextPtr_t const)SymContext_p,
                                    (ValOctetsIn_t *)HashData_p, Size);
        if (funcres != VAL_SUCCESS)
        {
            goto error_func_exit;
        }

        // Adjust references
        HashData_p   += Size;
        HashDataSize -= Size;
    }

    {
        uint8_t * SigData_p;
        uint32_t SignatureSize = (uint32_t)(2 * VAL_ASYM_DATA_SIZE_VWB(PrvKeyContext_p->ModulusSizeBits));

        // Allocate a temporary buffer for the signature
        SigData_p = Adapter_Alloc(SignatureSize);
        if (SigData_p == NULL)
        {
            funcres = VAL_NO_MEMORY;
            goto error_func_exit;
        }

        // Format SIGN-Final request
        t_cmd.Service.PkAssetSignVerify.Method = VEXTOKEN_PKASSET_EDDSA_SIGN_FINAL;
        t_cmd.Service.PkAssetSignVerify.KeyAssetId = VAL_ASSETID_INVALID;
        t_cmd.Service.PkAssetSignVerify.HashData_p = (const uint8_t *)HashData_p;
        t_cmd.Service.PkAssetSignVerify.HashDataSize = (uint32_t)HashDataSize;
        t_cmd.Service.PkAssetSignVerify.Sign_p = SigData_p;
        t_cmd.Service.PkAssetSignVerify.SignSize = SignatureSize;

        // Exchange service request with the next driver level
        funcres = val_ExchangeToken(&t_cmd, &t_res);
        if (funcres == VAL_SUCCESS)
        {
            // Check for errors
            if (t_res.Result == VEXTOKEN_RESULT_NO_ERROR)
            {
                // State asset is deleted
                StateAssetId = VAL_ASSETID_INVALID;

                // Convert the signature from HW to application format
                valInternal_AsymDsaSignatureFromHW(SigData_p,
                                                   PrvKeyContext_p->ModulusSizeBits,
                                                   Signature_p);
            }
            else
            {
                funcres = (ValStatus_t)t_res.Result;
                LOG_WARN("Abort - %s()=%d\n", __func__, t_res.Result);
            }
        }

        Adapter_Free(SigData_p);
    }

    // Clean-up
error_func_exit:
    if (StateAssetId != VAL_ASSETID_INVALID)
    {
        (void)val_AssetFree(StateAssetId);
    }
    if (SymContext_p != NULL)
    {
        (void)val_SymRelease((ValSymContextPtr_t const)SymContext_p);
    }

    return funcres;
}
#endif /* !VAL_REMOVE_ASYM_EDDSA_SIGN */


/*----------------------------------------------------------------------------
 * val_AsymEddsaVerify
 */
#ifndef VAL_REMOVE_ASYM_EDDSA_VERIFY
ValStatus_t
val_AsymEddsaVerify(
        ValAsymKey_t * const PubKeyContext_p,
        ValAsymSign_t * const Signature_p,
        ValOctetsIn_t * const HashMessage_p,
        const ValSize_t HashMessageSize)
{
    ValStatus_t funcres = VAL_SUCCESS;
    ValPolicyMask_t AssetPolicy = VAL_POLICY_TEMP_MAC|VAL_POLICY_SHA512;
    ValAssetId_t StateAssetId = VAL_ASSETID_INVALID;
    ValSymContext_t * SymContext_p = NULL;
    ValOctetsIn_t * HashData_p;
    ValSize_t HashDataSize = 0;
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;

#ifdef VAL_STRICT_ARGS
    if ((PubKeyContext_p == NULL) ||
        (PubKeyContext_p->KeyAssetId == VAL_ASSETID_INVALID) ||
        (PubKeyContext_p->DomainAssetId == VAL_ASSETID_INVALID) ||
        (PubKeyContext_p->ModulusSizeBits != 255) ||
        (HashMessage_p == NULL) || (HashMessageSize == 0) ||
        (Signature_p == NULL) ||
        (Signature_p->r.Data_p == NULL) ||
        (Signature_p->r.ByteDataSize < VAL_ASYM_DATA_SIZE_B2B(PubKeyContext_p->ModulusSizeBits)) ||
        (Signature_p->s.Data_p == NULL) ||
        (Signature_p->s.ByteDataSize < VAL_ASYM_DATA_SIZE_B2B(PubKeyContext_p->ModulusSizeBits)))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Allocate hash context
    funcres = val_SymAlloc(VAL_SYM_ALGO_HASH_SHA512, VAL_SYM_MODE_NONE, false,
                           (ValSymContextPtr_t *)&SymContext_p);
    if (funcres != VAL_SUCCESS)
    {
        return funcres;
    }

    // Allocate Asset for intermediate digest and
    // mark hash setup done (initial hash is done during EdDSA operation)
    if (!val_IsAccessSecure())
    {
        AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
    }
    funcres = val_AssetAlloc(AssetPolicy, (512 / 8),
                             false, false,
                             VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                             &SymContext_p->Service.Hash.TempAssetId);
    if (funcres != VAL_SUCCESS)
    {
        goto error_func_exit;
    }
    SymContext_p->fInitDone = true;

    // Set initial message size
    HashData_p = (ValOctetsIn_t *)HashMessage_p;
    if (HashMessageSize > 64)
    {
        HashDataSize = 64;
    }
    else
    {
        HashDataSize = HashMessageSize;
    }

    {
        uint8_t * SigData_p;
        uint32_t SignatureSize = (uint32_t)(2 * VAL_ASYM_DATA_SIZE_VWB(PubKeyContext_p->ModulusSizeBits));

        // Allocate a temporary buffer for the signature
        SigData_p = Adapter_Alloc(SignatureSize);
        if (SigData_p == NULL)
        {
            funcres = VAL_NO_MEMORY;
            goto error_func_exit;
        }

        // Convert the signature to from application to HW format
        valInternal_AsymDsaSignatureToHW(Signature_p,
                                         PubKeyContext_p->ModulusSizeBits,
                                         SigData_p);

        // Format SIGN-Initial request
        t_cmd.OpCode = VEXTOKEN_OPCODE_PUBLIC_KEY;
        t_cmd.SubCode = VEXTOKEN_SUBCODE_PK_WITHASSETS;
        t_cmd.Service.PkAssetSignVerify.Method = VEXTOKEN_PKASSET_EDDSA_VERIFY_INITIAL;
        t_cmd.Service.PkAssetSignVerify.ModulusSizeInBits = (uint32_t)PubKeyContext_p->ModulusSizeBits;
        t_cmd.Service.PkAssetSignVerify.KeyAssetId = PubKeyContext_p->KeyAssetId;
        t_cmd.Service.PkAssetSignVerify.DomainAssetId = PubKeyContext_p->DomainAssetId;
        t_cmd.Service.PkAssetSignVerify.DigestAssetId = SymContext_p->Service.Hash.TempAssetId;
        t_cmd.Service.PkAssetSignVerify.HashData_p = (const uint8_t *)HashData_p;
        t_cmd.Service.PkAssetSignVerify.HashDataSize = (uint32_t)HashDataSize;
        t_cmd.Service.PkAssetSignVerify.TotalMessageSize = 0;
        t_cmd.Service.PkAssetSignVerify.SaltSize = 0;
        t_cmd.Service.PkAssetSignVerify.Sign_p = SigData_p;
        t_cmd.Service.PkAssetSignVerify.SignSize = SignatureSize;
        t_res.Result = VEXTOKEN_RESULT_NO_ERROR;
        t_res.Service.PkAssetSignVerify.StateAssetId = 0;

        // Exchange service request with the next driver level
        funcres = val_ExchangeToken(&t_cmd, &t_res);
        Adapter_Free(SigData_p);
        if (funcres == VAL_SUCCESS)
        {
            // Check for errors
            if (t_res.Result == VEXTOKEN_RESULT_NO_ERROR)
            {
                // Get State asset of the operation
                StateAssetId = t_res.Service.PkAssetSignVerify.StateAssetId;
            }
            else
            {
                funcres = (ValStatus_t)t_res.Result;
                LOG_WARN("Abort - %s()=%d\n", __func__, t_res.Result);
                goto error_func_exit;
            }
        }
    }

    // Hash the middle section of the message if needed
    if (HashMessageSize > 64)
    {
        HashData_p  += 64;
        HashDataSize = HashMessageSize - 64;
    }
    while (HashDataSize > VAL_ASYM_CMD_MAX_HASH_SIZE)
    {
        // Hash a block of data
        ValSize_t Size = (HashDataSize - (4 * VAL_SYM_ALGO_HASH_SHA512_BLOCK_SIZE)) &
                         (VAL_SIZE_MAX_DMA & ~(VAL_SYM_ALGO_HASH_SHA512_BLOCK_SIZE -1));

        funcres = val_SymHashUpdate((ValSymContextPtr_t const)SymContext_p,
                                    (ValOctetsIn_t *)HashData_p, Size);
        if (funcres != VAL_SUCCESS)
        {
            goto error_func_exit;
        }

        // Adjust references
        HashData_p   += Size;
        HashDataSize -= Size;
    }

    // Format SIGN-Final request
    t_cmd.Service.PkAssetSignVerify.Method = VEXTOKEN_PKASSET_EDDSA_VERIFY_FINAL;
    t_cmd.Service.PkAssetSignVerify.KeyAssetId = VAL_ASSETID_INVALID;
    t_cmd.Service.PkAssetSignVerify.DomainAssetId = VAL_ASSETID_INVALID;
    t_cmd.Service.PkAssetSignVerify.DigestAssetId = StateAssetId;
    t_cmd.Service.PkAssetSignVerify.HashData_p = (const uint8_t *)HashData_p;
    t_cmd.Service.PkAssetSignVerify.HashDataSize = (uint32_t)HashDataSize;
    t_cmd.Service.PkAssetSignVerify.TotalMessageSize = HashMessageSize;
    t_cmd.Service.PkAssetSignVerify.Sign_p = NULL;
    t_cmd.Service.PkAssetSignVerify.SignSize = 0;
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if (funcres == VAL_SUCCESS)
    {
        // Check for errors
        if (t_res.Result == VEXTOKEN_RESULT_NO_ERROR)
        {
            // State asset is deleted
            StateAssetId = VAL_ASSETID_INVALID;
        }
        else
        {
            funcres = (ValStatus_t)t_res.Result;
            LOG_WARN("Abort - %s()=%d\n", __func__, t_res.Result);
        }
    }

    // Clean-up
error_func_exit:
    if (StateAssetId != VAL_ASSETID_INVALID)
    {
        (void)val_AssetFree(StateAssetId);
    }
    if (SymContext_p != NULL)
    {
        (void)val_SymRelease((ValSymContextPtr_t const)SymContext_p);
    }

    return funcres;
}
#endif /* !VAL_REMOVE_ASYM_EDDSA_VERIFY */


#endif /* !VAL_REMOVE_ASYM_EDDSA_SIGN || !VAL_REMOVE_ASYM_EDDSA_VERIFY  */

/* end of file adapter_val_asym_eddsa.c */
