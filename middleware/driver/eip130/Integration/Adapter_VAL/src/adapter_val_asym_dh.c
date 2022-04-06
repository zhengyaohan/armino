/* adapter_val_asym_dh.c
 *
 * Implementation of the VAL API.
 *
 * This file implements the asymmetric crypto Diffie-Hellman related services.
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
 * val_AsymDhAllocPrivateKeyAsset
 */
#ifndef VAL_REMOVE_ASYM_DH_GEN_KEYPAIR
ValStatus_t
val_AsymDhAllocPrivateKeyAsset(
        const ValSize_t DivisorSizeBits,
        const bool fCrossDomain,
        const bool fExportAllowed,
        ValAssetId_t * const AssetId_p)
{
    ValPolicyMask_t AssetPolicy;

#ifdef VAL_STRICT_ARGS
    if ((AssetId_p == NULL) ||
        (DivisorSizeBits < VAL_ASYM_DH_DSA_DIVISOR_MIN_BITS) ||
        (DivisorSizeBits > VAL_ASYM_DH_DSA_DIVISOR_MAX_BITS))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Determine Asset policy
    AssetPolicy = VAL_POLICY_PK_DH_KEY;
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
                          VAL_ASYM_DATA_SIZE_VWB(DivisorSizeBits),
                          false, false,
                          VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                          AssetId_p);
}
#endif


/*----------------------------------------------------------------------------
 * val_AsymDhAllocPublicKeyAsset
 */
#if !defined(VAL_REMOVE_ASYM_DH_GEN_KEYPAIR) || \
    !defined(VAL_REMOVE_ASYM_DH_GEN_PUBLICKEY)
ValStatus_t
val_AsymDhAllocPublicKeyAsset(
        const ValSize_t PrimeSizeBits,
        const bool fCrossDomain,
        const bool fExportAllowed,
        ValAssetId_t * const AssetId_p)
{
    ValPolicyMask_t AssetPolicy;

#ifdef VAL_STRICT_ARGS
    if ((AssetId_p == NULL) ||
        (PrimeSizeBits < VAL_ASYM_DH_DSA_MIN_BITS) ||
        (PrimeSizeBits > VAL_ASYM_DH_DSA_MAX_BITS))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Determine Asset policy
    AssetPolicy = VAL_POLICY_PUBLIC_KEY | VAL_POLICY_PK_DH_KEY;
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
                          VAL_ASYM_DATA_SIZE_VWB(PrimeSizeBits),
                          false, false,
                          VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                          AssetId_p);
}
#endif


/*----------------------------------------------------------------------------
 * val_AsymDhGenKeyPair
 */
#ifndef VAL_REMOVE_ASYM_DH_GEN_KEYPAIR
ValStatus_t
val_AsymDhGenKeyPair(
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
        (PubKey_p->ModulusSizeBits < VAL_ASYM_DH_DSA_MIN_BITS) ||
        (PubKey_p->ModulusSizeBits > VAL_ASYM_DH_DSA_MAX_BITS) ||
        (PrivKey_p == NULL) ||
        (PrivKey_p->KeyAssetId == VAL_ASSETID_INVALID) ||
        (PrivKey_p->DomainAssetId != PubKey_p->DomainAssetId) ||
        (PrivKey_p->ModulusSizeBits < VAL_ASYM_DH_DSA_DIVISOR_MIN_BITS) ||
        (PrivKey_p->ModulusSizeBits > VAL_ASYM_DH_DSA_DIVISOR_MAX_BITS))
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
    t_cmd.Service.PkAssetGenKey.Method = VEXTOKEN_PKASSET_DH_GEN_KEYPAIR;
    t_cmd.Service.PkAssetGenKey.ModulusSizeInBits = (uint32_t)PubKey_p->ModulusSizeBits;
    t_cmd.Service.PkAssetGenKey.DivisorSizeInBits = (uint32_t)PrivKey_p->ModulusSizeBits;
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
#endif


/*----------------------------------------------------------------------------
 * val_AsymDhGenPublicKey
 */
#ifndef VAL_REMOVE_ASYM_DH_GEN_PUBLICKEY
ValStatus_t
val_AsymDhGenPublicKey(
        ValAsymKey_t * const PubKey_p,
        ValAsymKey_t * const PrivKey_p,
        ValAsymBigInt_t * PubKeyValue_p)
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
        (PubKey_p->ModulusSizeBits < VAL_ASYM_DH_DSA_MIN_BITS) ||
        (PubKey_p->ModulusSizeBits > VAL_ASYM_DH_DSA_MAX_BITS) ||
        (PrivKey_p == NULL) ||
        (PrivKey_p->KeyAssetId == VAL_ASSETID_INVALID) ||
        (PrivKey_p->DomainAssetId != PubKey_p->DomainAssetId) ||
        (PrivKey_p->ModulusSizeBits < VAL_ASYM_DH_DSA_DIVISOR_MIN_BITS) ||
        (PrivKey_p->ModulusSizeBits > VAL_ASYM_DH_DSA_DIVISOR_MAX_BITS))
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
    t_cmd.Service.PkAssetGenKey.Method = VEXTOKEN_PKASSET_DH_GEN_PUBKEY;
    t_cmd.Service.PkAssetGenKey.ModulusSizeInBits = (uint32_t)PubKey_p->ModulusSizeBits;
    t_cmd.Service.PkAssetGenKey.DivisorSizeInBits = (uint32_t)PrivKey_p->ModulusSizeBits;
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
#endif


/*----------------------------------------------------------------------------
 * val_AsymDhGenSharedSecretSingle
 */
#ifndef VAL_REMOVE_ASYM_DH_GEN_SHAREDSECRET_SINGLE
ValStatus_t
val_AsymDhGenSharedSecretSingle(
        ValAsymKey_t * const PubKey_p,
        ValAsymKey_t * const PrivKey_p,
        const bool fSaveSharedSecret,
        ValOctetsIn_Optional_t * const OtherInfo_p,
        const ValSize_t OtherInfoSize,
        ValAssetId_t * const AssetIdList_p,
        const ValSize_t NumberOfAssets)
{
    ValStatus_t funcres;
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;

#ifdef VAL_STRICT_ARGS
    if ((PubKey_p == NULL) ||
        (PubKey_p->KeyAssetId == VAL_ASSETID_INVALID) ||
        (PubKey_p->DomainAssetId == VAL_ASSETID_INVALID) ||
        (PubKey_p->ModulusSizeBits < VAL_ASYM_DH_DSA_MIN_BITS) ||
        (PubKey_p->ModulusSizeBits > VAL_ASYM_DH_DSA_MAX_BITS) ||
        (PrivKey_p == NULL) ||
        (PrivKey_p->KeyAssetId == VAL_ASSETID_INVALID) ||
        (PrivKey_p->DomainAssetId != PubKey_p->DomainAssetId) ||
        (PrivKey_p->ModulusSizeBits < VAL_ASYM_DH_DSA_DIVISOR_MIN_BITS) ||
        (PrivKey_p->ModulusSizeBits > VAL_ASYM_DH_DSA_DIVISOR_MAX_BITS) ||
        (!fSaveSharedSecret && (OtherInfo_p == NULL) && (OtherInfoSize != 0))  ||
        (AssetIdList_p == NULL) || (NumberOfAssets == 0) ||
        (fSaveSharedSecret && (NumberOfAssets != 1)))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_PUBLIC_KEY;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_PK_WITHASSETS;
    t_cmd.Service.PkAssetGenSharedSecret.Method = VEXTOKEN_PKASSET_DH_GEN_SHARED_SECRET_SKEYPAIR;
    t_cmd.Service.PkAssetGenSharedSecret.ModulusSizeInBits = (uint32_t)PubKey_p->ModulusSizeBits;
    t_cmd.Service.PkAssetGenSharedSecret.DivisorSizeInBits = (uint32_t)PrivKey_p->ModulusSizeBits;
    t_cmd.Service.PkAssetGenSharedSecret.PubKeyAssetId = (uint32_t)PubKey_p->KeyAssetId;
    t_cmd.Service.PkAssetGenSharedSecret.PrivKeyAssetId = (uint32_t)PrivKey_p->KeyAssetId;
    t_cmd.Service.PkAssetGenSharedSecret.DomainAssetId = (uint32_t)PubKey_p->DomainAssetId;
    t_cmd.Service.PkAssetGenSharedSecret.PubKey2AssetId = VAL_ASSETID_INVALID;
    t_cmd.Service.PkAssetGenSharedSecret.PrivKey2AssetId = VAL_ASSETID_INVALID;
    t_cmd.Service.PkAssetGenSharedSecret.OtherInfo_p = OtherInfo_p;
    t_cmd.Service.PkAssetGenSharedSecret.OtherInfoSize = (uint32_t)OtherInfoSize;
    t_cmd.Service.PkAssetGenSharedSecret.AssetIdList_p = (const uint32_t *)AssetIdList_p;
    t_cmd.Service.PkAssetGenSharedSecret.AssetIdListSize = (uint32_t)NumberOfAssets;
    t_cmd.Service.PkAssetGenSharedSecret.fSaveSharedSecret = fSaveSharedSecret;
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


/*----------------------------------------------------------------------------
 * val_AsymDhGenSharedSecretDual
 */
#ifndef VAL_REMOVE_ASYM_DH_GEN_SHAREDSECRET_DUAL
ValStatus_t
val_AsymDhGenSharedSecretDual(
        ValAsymKey_t * const PubKey_p,
        ValAsymKey_t * const PrivKey_p,
        ValAsymKey_t * const PubKey2_p,
        ValAsymKey_t * const PrivKey2_p,
        const bool fSaveSharedSecret,
        ValOctetsIn_Optional_t * const OtherInfo_p,
        const ValSize_t OtherInfoSize,
        ValAssetId_t * const AssetIdList_p,
        const ValSize_t NumberOfAssets)
{
    ValStatus_t funcres;
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;

#ifdef VAL_STRICT_ARGS
    if ((PubKey_p == NULL) ||
        (PubKey_p->KeyAssetId == VAL_ASSETID_INVALID) ||
        (PubKey_p->DomainAssetId == VAL_ASSETID_INVALID) ||
        (PubKey_p->ModulusSizeBits < VAL_ASYM_DH_DSA_MIN_BITS) ||
        (PubKey_p->ModulusSizeBits > VAL_ASYM_DH_DSA_MAX_BITS) ||
        (PrivKey_p == NULL) ||
        (PrivKey_p->KeyAssetId == VAL_ASSETID_INVALID) ||
        (PrivKey_p->DomainAssetId != PubKey_p->DomainAssetId) ||
        (PrivKey_p->ModulusSizeBits < VAL_ASYM_DH_DSA_DIVISOR_MIN_BITS) ||
        (PrivKey_p->ModulusSizeBits > VAL_ASYM_DH_DSA_DIVISOR_MAX_BITS) ||
        (PubKey2_p == NULL) ||
        (PubKey2_p->KeyAssetId == VAL_ASSETID_INVALID) ||
        (PubKey2_p->DomainAssetId != PubKey_p->DomainAssetId) ||
        (PubKey2_p->ModulusSizeBits < VAL_ASYM_DH_DSA_MIN_BITS) ||
        (PubKey2_p->ModulusSizeBits > VAL_ASYM_DH_DSA_MAX_BITS) ||
        (PrivKey2_p == NULL) ||
        (PrivKey2_p->KeyAssetId == VAL_ASSETID_INVALID) ||
        (PrivKey2_p->DomainAssetId != PubKey_p->DomainAssetId) ||
        (PrivKey2_p->ModulusSizeBits < VAL_ASYM_DH_DSA_DIVISOR_MIN_BITS) ||
        (PrivKey2_p->ModulusSizeBits > VAL_ASYM_DH_DSA_DIVISOR_MAX_BITS) ||
        (!fSaveSharedSecret && (OtherInfo_p == NULL) && (OtherInfoSize != 0))  ||
        (AssetIdList_p == NULL) || (NumberOfAssets == 0) ||
        (fSaveSharedSecret && (NumberOfAssets != 1)))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_PUBLIC_KEY;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_PK_WITHASSETS;
    t_cmd.Service.PkAssetGenSharedSecret.Method = VEXTOKEN_PKASSET_DH_GEN_SHARED_SECRET_DKEYPAIR;
    t_cmd.Service.PkAssetGenSharedSecret.ModulusSizeInBits = (uint32_t)PubKey_p->ModulusSizeBits;
    t_cmd.Service.PkAssetGenSharedSecret.DivisorSizeInBits = (uint32_t)PrivKey_p->ModulusSizeBits;
    t_cmd.Service.PkAssetGenSharedSecret.PubKeyAssetId = (uint32_t)PubKey_p->KeyAssetId;
    t_cmd.Service.PkAssetGenSharedSecret.PrivKeyAssetId = (uint32_t)PrivKey_p->KeyAssetId;
    t_cmd.Service.PkAssetGenSharedSecret.DomainAssetId = (uint32_t)PubKey_p->DomainAssetId;
    t_cmd.Service.PkAssetGenSharedSecret.PubKey2AssetId = (uint32_t)PubKey2_p->KeyAssetId;
    t_cmd.Service.PkAssetGenSharedSecret.PrivKey2AssetId = (uint32_t)PrivKey2_p->KeyAssetId;
    t_cmd.Service.PkAssetGenSharedSecret.OtherInfo_p = OtherInfo_p;
    t_cmd.Service.PkAssetGenSharedSecret.OtherInfoSize = (uint32_t)OtherInfoSize;
    t_cmd.Service.PkAssetGenSharedSecret.AssetIdList_p = (const uint32_t *)AssetIdList_p;
    t_cmd.Service.PkAssetGenSharedSecret.AssetIdListSize = (uint32_t)NumberOfAssets;
    t_cmd.Service.PkAssetGenSharedSecret.fSaveSharedSecret = fSaveSharedSecret;
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


/* end of file adapter_val_asym_dh.c */
