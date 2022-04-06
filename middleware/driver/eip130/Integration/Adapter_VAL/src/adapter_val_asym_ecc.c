/* adapter_val_asym_ecc.c
 *
 * Implementation of the VAL API.
 *
 * This file implements the asymmetric crypto related common ECC services.
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


/*----------------------------------------------------------------------------
 * val_AsymEccAllocDomainAsset
 */
#if !defined(VAL_REMOVE_ASYM_ECC_GEN_KEYPAIR) || \
    !defined(VAL_REMOVE_ASYM_ECC_GEN_PUBLICKEY) || \
    !defined(VAL_REMOVE_ASYM_ECDSA_SIGN) || \
    !defined(VAL_REMOVE_ASYM_ECDSA_VERIFY) || \
    !defined(VAL_REMOVE_ASYM_EDSA_SIGN) || \
    !defined(VAL_REMOVE_ASYM_EDDSA_VERIFY) || \
    !defined(VAL_REMOVE_ASYM_ECC_ELGAMAL_ENCRYPT) || \
    !defined(VAL_REMOVE_ASYM_ECC_ELGAMAL_DECRYPT) || \
    !defined(VAL_REMOVE_ASYM_ECDH_GEN_SHAREDSECRET_SINGLE) || \
    !defined(VAL_REMOVE_ASYM_ECDH_GEN_SHAREDSECRET_DUAL) || \
    !defined(VAL_REMOVE_ASYM_CURVE25519)
ValStatus_t
val_AsymEccAllocDomainAsset(
        const ValSize_t ModulusSizeBits,
        ValAssetId_t * const AssetId_p)
{
    ValPolicyMask_t AssetPolicy;

#ifdef VAL_STRICT_ARGS
    if ((AssetId_p == NULL) ||
        (ModulusSizeBits < VAL_ASYM_ECP_MIN_BITS) ||
        (ModulusSizeBits > VAL_ASYM_ECP_MAX_BITS))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Determine Asset policy
    AssetPolicy = VAL_POLICY_PUBLIC_KEY_PARAM;
    if (!val_IsAccessSecure())
    {
        AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
    }

    // Determine the Asset size and allocate the Asset
    return val_AssetAlloc(AssetPolicy,
                          ((6 * VAL_ASYM_DATA_SIZE_VWB(ModulusSizeBits)) + 8),
                          false, false,
                          VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                          AssetId_p);
}
#endif


/*----------------------------------------------------------------------------
 * val_AsymEccLoadDomainAssetPlaintext
 */
#if !defined(VAL_REMOVE_ASYM_ECC_GEN_KEYPAIR) || \
    !defined(VAL_REMOVE_ASYM_ECC_GEN_PUBLICKEY) || \
    !defined(VAL_REMOVE_ASYM_ECDSA_SIGN) || \
    !defined(VAL_REMOVE_ASYM_ECDSA_VERIFY) || \
    !defined(VAL_REMOVE_ASYM_EDSA_SIGN) || \
    !defined(VAL_REMOVE_ASYM_EDDSA_VERIFY) || \
    !defined(VAL_REMOVE_ASYM_ECC_ELGAMAL_ENCRYPT) || \
    !defined(VAL_REMOVE_ASYM_ECC_ELGAMAL_DECRYPT) || \
    !defined(VAL_REMOVE_ASYM_ECDH_GEN_SHAREDSECRET_SINGLE) || \
    !defined(VAL_REMOVE_ASYM_ECDH_GEN_SHAREDSECRET_DUAL) || \
    !defined(VAL_REMOVE_ASYM_CURVE25519)
ValStatus_t
val_AsymEccLoadDomainAssetPlaintext(
        const ValAsymECDomainParam_t * const Domain_p,
        const ValAssetId_t TargetAssetId,
        const ValAssetId_Optional_t KekAssetId,
        ValOctetsIn_Optional_t * const AssociatedData_p,
        const ValSize_t AssociatedDataSize,
        ValOctetsOut_Optional_t * const KeyBlob_p,
        ValSize_t * const KeyBlobSize_p)
{
    ValStatus_t funcres = VAL_INTERNAL_ERROR;
    uint8_t * AssetData_p;
    uint8_t * ptr;
    ValSize_t AssetSize;
    uint32_t ByteLength;
    int RemainingLength;
    uint8_t bits = 1;
    uint8_t CoFactor = 1;

#ifdef VAL_STRICT_ARGS
    if ((Domain_p == NULL) ||
        (TargetAssetId == VAL_ASSETID_INVALID) ||
        (Domain_p->Modulus.Data_p == NULL) ||
        (Domain_p->ModulusSizeBits < VAL_ASYM_ECP_MIN_BITS) ||
        (Domain_p->ModulusSizeBits > VAL_ASYM_ECP_MAX_BITS) ||
        (Domain_p->Modulus.ByteDataSize < VAL_ASYM_DATA_SIZE_B2B(Domain_p->ModulusSizeBits)) ||
        (Domain_p->a.Data_p == NULL) ||
        (Domain_p->a.ByteDataSize == 0) ||
        (Domain_p->b.Data_p == NULL) ||
        (Domain_p->b.ByteDataSize == 0) ||
        (Domain_p->Order.Data_p == NULL) ||
        (Domain_p->Order.ByteDataSize == 0) ||
        (Domain_p->BasePoint.x.Data_p == NULL) ||
        (Domain_p->BasePoint.x.ByteDataSize == 0) ||
        (Domain_p->BasePoint.y.Data_p == NULL) ||
        (Domain_p->BasePoint.y.ByteDataSize == 0))
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
    ByteLength = (uint32_t)VAL_ASYM_DATA_SIZE_B2WB(Domain_p->ModulusSizeBits);
    AssetSize = (6 * VAL_ASYM_DATA_SIZE_VWB(Domain_p->ModulusSizeBits)) + 8;
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
    ptr = AssetData_p;

    // Create domain parameters representation for plaintext asset load
    RemainingLength = (int)(ByteLength - Domain_p->Modulus.ByteDataSize);
    if (RemainingLength < 0)
    {
        goto error_func_exit;
    }
    *ptr++ = (uint8_t)(Domain_p->ModulusSizeBits);
    *ptr++ = (uint8_t)(Domain_p->ModulusSizeBits >> 8);
    *ptr++ = 0;
    *ptr++ = 7;
    valInternal_ReverseMemCpy(ptr,
                              Domain_p->Modulus.Data_p,
                              Domain_p->Modulus.ByteDataSize);
    ptr += Domain_p->Modulus.ByteDataSize;
    if (RemainingLength > 0)
    {
        memset(ptr, 0, (size_t)RemainingLength);
        ptr += RemainingLength;
    }

    RemainingLength = (int)(ByteLength - Domain_p->a.ByteDataSize);
    if (RemainingLength < 0)
    {
        goto error_func_exit;
    }
    *ptr++ = (uint8_t)(Domain_p->ModulusSizeBits);
    *ptr++ = (uint8_t)(Domain_p->ModulusSizeBits >> 8);
    *ptr++ = 1;
    *ptr++ = 7;
    valInternal_ReverseMemCpy(ptr,
                              Domain_p->a.Data_p,
                              Domain_p->a.ByteDataSize);
    ptr += Domain_p->a.ByteDataSize;
    if (RemainingLength > 0)
    {
        memset(ptr, 0, (size_t)RemainingLength);
        ptr += RemainingLength;
    }

    RemainingLength = (int)(ByteLength - Domain_p->b.ByteDataSize);
    if (RemainingLength < 0)
    {
        goto error_func_exit;
    }
    *ptr++ = (uint8_t)(Domain_p->ModulusSizeBits);
    *ptr++ = (uint8_t)(Domain_p->ModulusSizeBits >> 8);
    *ptr++ = 2;
    *ptr++ = 7;
    valInternal_ReverseMemCpy(ptr,
                              Domain_p->b.Data_p,
                              Domain_p->b.ByteDataSize);
    ptr += Domain_p->b.ByteDataSize;
    if (RemainingLength > 0)
    {
        memset(ptr, 0, (size_t)RemainingLength);
        ptr += RemainingLength;
    }

    RemainingLength = (int)(ByteLength - Domain_p->Order.ByteDataSize);
    if (RemainingLength < 0)
    {
        goto error_func_exit;
    }
    *ptr++ = (uint8_t)(Domain_p->ModulusSizeBits);
    *ptr++ = (uint8_t)(Domain_p->ModulusSizeBits >> 8);
    *ptr++ = 3;
    *ptr++ = 7;
    valInternal_ReverseMemCpy(ptr,
                              Domain_p->Order.Data_p,
                              Domain_p->Order.ByteDataSize);
    ptr += Domain_p->Order.ByteDataSize;
    if (RemainingLength > 0)
    {
        memset(ptr, 0, (size_t)RemainingLength);
        ptr += RemainingLength;
    }

    RemainingLength = (int)(ByteLength - Domain_p->BasePoint.x.ByteDataSize);
    if (RemainingLength < 0)
    {
        goto error_func_exit;
    }
    *ptr++ = (uint8_t)(Domain_p->ModulusSizeBits);
    *ptr++ = (uint8_t)(Domain_p->ModulusSizeBits >> 8);
    *ptr++ = 4;
    *ptr++ = 7;
    valInternal_ReverseMemCpy(ptr,
                              Domain_p->BasePoint.x.Data_p,
                              Domain_p->BasePoint.x.ByteDataSize);
    ptr += Domain_p->BasePoint.x.ByteDataSize;
    if (RemainingLength > 0)
    {
        memset(ptr, 0, (size_t)RemainingLength);
        ptr += RemainingLength;
    }

    RemainingLength = (int)(ByteLength - Domain_p->BasePoint.y.ByteDataSize);
    if (RemainingLength < 0)
    {
        goto error_func_exit;
    }
    *ptr++ = (uint8_t)(Domain_p->ModulusSizeBits);
    *ptr++ = (uint8_t)(Domain_p->ModulusSizeBits >> 8);
    *ptr++ = 5;
    *ptr++ = 7;
    valInternal_ReverseMemCpy(ptr,
                              Domain_p->BasePoint.y.Data_p,
                              Domain_p->BasePoint.y.ByteDataSize);
    ptr += Domain_p->BasePoint.y.ByteDataSize;
    if (RemainingLength > 0)
    {
        memset(ptr, 0, (size_t)RemainingLength);
        ptr += RemainingLength;
    }

    if (Domain_p->CoFactor != 0)
    {
        uint8_t n = Domain_p->CoFactor;

        bits = 0;
        while (n != 0)
        {
            bits++;
            n >>= 1;
        }
        CoFactor = Domain_p->CoFactor;
    }
    *ptr++ = bits;
    *ptr++ = 0;
    *ptr++ = 6;
    *ptr++ = 7;
    *ptr++ = CoFactor;
    *ptr++ = 0;
    *ptr++ = 0;
    *ptr++ = 0;

    // Initialize the asset
    if (KekAssetId == VAL_ASSETID_INVALID)
    {
        funcres = val_AssetLoadPlaintext(TargetAssetId,
                                         (ValOctetsIn_t * const)AssetData_p,
                                         AssetSize);
    }
    else
    {
        funcres = val_AssetLoadPlaintextExport(TargetAssetId,
                                               (ValOctetsIn_t * const)AssetData_p,
                                               AssetSize,
                                               KekAssetId,
                                               AssociatedData_p,
                                               AssociatedDataSize,
                                               KeyBlob_p,
                                               KeyBlobSize_p);
    }

    // Clean-up
error_func_exit:
    Adapter_Free(AssetData_p);

    return funcres;
}
#endif


/*----------------------------------------------------------------------------
 * val_AsymEccLoadPrivateKeyAssetPlaintext
 */
#if !defined(VAL_REMOVE_ASYM_ECDSA_SIGN) || \
    !defined(VAL_REMOVE_ASYM_ECC_ELGAMAL_DECRYPT) || \
    !defined(VAL_REMOVE_ASYM_ECDH_GEN_SHAREDSECRET_SINGLE) || \
    !defined(VAL_REMOVE_ASYM_ECDH_GEN_SHAREDSECRET_DUAL) || \
    !defined(VAL_REMOVE_ASYM_EDDSA_SIGN) || \
    !defined(VAL_REMOVE_ASYM_EDDSA_VERIFY)
ValStatus_t
val_AsymEccLoadPrivateKeyAssetPlaintext(
        const ValAsymBigInt_t * const Key_p,
        const ValSize_t ModulusSizeBits,
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

#ifdef VAL_STRICT_ARGS
    if ((Key_p == NULL) ||
        (Key_p->Data_p == NULL) ||
        (ModulusSizeBits < VAL_ASYM_ECP_MIN_BITS) ||
        (ModulusSizeBits > VAL_ASYM_ECP_MAX_BITS) ||
        (Key_p->ByteDataSize == 0) ||
        (Key_p->ByteDataSize > VAL_ASYM_DATA_SIZE_B2B(ModulusSizeBits)) ||
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
    AssetSize = VAL_ASYM_DATA_SIZE_VWB(ModulusSizeBits);
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

    // Create HW private key representation for plaintext asset load
    valInternal_AsymBigIntToHW(Key_p, ModulusSizeBits, 0, 1, AssetData_p);

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
 * val_AsymEccLoadPublicKeyAssetPlaintext
 */
#if !defined(VAL_REMOVE_ASYM_ECDSA_VERIFY) || \
    !defined(VAL_REMOVE_ASYM_ECC_ELGAMAL_ENCRYPT) || \
    !defined(VAL_REMOVE_ASYM_ECDH_GEN_SHAREDSECRET_SINGLE) || \
    !defined(VAL_REMOVE_ASYM_ECDH_GEN_SHAREDSECRET_DUAL)
ValStatus_t
val_AsymEccLoadPublicKeyAssetPlaintext(
        const ValAsymECCPoint_t * const KeyPoint_p,
        const ValSize_t ModulusSizeBits,
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

#ifdef VAL_STRICT_ARGS
    if ((KeyPoint_p == NULL) ||
        (KeyPoint_p->x.Data_p == NULL) ||
        (KeyPoint_p->x.ByteDataSize == 0) ||
        (KeyPoint_p->y.Data_p == NULL) ||
        (KeyPoint_p->y.ByteDataSize == 0) ||
        (TargetAssetId == VAL_ASSETID_INVALID) ||
        (ModulusSizeBits < VAL_ASYM_ECP_MIN_BITS) ||
        (ModulusSizeBits > VAL_ASYM_ECP_MAX_BITS))
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
    AssetSize = 2 * VAL_ASYM_DATA_SIZE_VWB(ModulusSizeBits);
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

    // Create Public Key representation for plaintext asset load
    valInternal_AsymECPointToHW(KeyPoint_p, ModulusSizeBits, 0, 2, AssetData_p);

    // Initialize the asset
    if (KekAssetId == VAL_ASSETID_INVALID)
    {
        funcres = val_AssetLoadPlaintext(TargetAssetId,
                                         AssetData_p, AssetSize);
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
 * val_AsymEccGenKeyPair
 */
#ifndef VAL_REMOVE_ASYM_ECC_GEN_KEYPAIR
ValStatus_t
val_AsymEccGenKeyPair(
        ValAsymKey_t * const PubKey_p,
        ValAsymKey_t * const PrivKey_p,
        const ValAssetId_Optional_t KekAssetId,
        ValOctetsIn_Optional_t * const AssociatedData_p,
        const ValSize_t AssociatedDataSize,
        ValAsymECCPoint_t * const PubKeyPoint_p,
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
        (PubKey_p->ModulusSizeBits < VAL_ASYM_ECP_MIN_BITS) ||
        (PubKey_p->ModulusSizeBits > VAL_ASYM_ECP_MAX_BITS) ||
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

    if (PubKeyPoint_p != NULL)
    {
        // Validate application format public key data size
#ifdef VAL_STRICT_ARGS
        PubKeyDataSize = VAL_ASYM_DATA_SIZE_B2B(PubKey_p->ModulusSizeBits);
        if ((PubKeyPoint_p->x.Data_p == NULL) ||
            (PubKeyPoint_p->x.ByteDataSize < PubKeyDataSize) ||
            (PubKeyPoint_p->y.Data_p == NULL) ||
            (PubKeyPoint_p->y.ByteDataSize < PubKeyDataSize))
        {
            return VAL_BAD_ARGUMENT;
        }
#endif

        // Determine the HW size for the public key
        PubKeyDataSize = 2 * VAL_ASYM_DATA_SIZE_VWB(PubKey_p->ModulusSizeBits);

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
    t_cmd.Service.PkAssetGenKey.Method = VEXTOKEN_PKASSET_ECDH_ECDSA_GEN_KEYPAIR;
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
            if (PubKeyPoint_p != NULL)
            {
                valInternal_AsymECPointFromHW(Data_p,
                                              PubKey_p->ModulusSizeBits,
                                              PubKeyPoint_p);
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
#endif /* !VAL_REMOVE_ASYM_ECC_GEN_KEYPAIR */


/*----------------------------------------------------------------------------
 * val_AsymEccGenPublicKey
 */
#ifndef VAL_REMOVE_ASYM_ECC_GEN_PUBLICKEY
ValStatus_t
val_AsymEccGenPublicKey(
        ValAsymKey_t * const PubKey_p,
        ValAsymKey_t * const PrivKey_p,
        ValAsymECCPoint_t * PubKeyPoint_p)
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
        (PubKey_p->ModulusSizeBits < VAL_ASYM_ECP_MIN_BITS) ||
        (PubKey_p->ModulusSizeBits > VAL_ASYM_ECP_MAX_BITS) ||
        (PrivKey_p == NULL) ||
        (PrivKey_p->KeyAssetId == VAL_ASSETID_INVALID) ||
        (PrivKey_p->DomainAssetId != PubKey_p->DomainAssetId) ||
        (PrivKey_p->ModulusSizeBits != PubKey_p->ModulusSizeBits))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    if (PubKeyPoint_p != NULL)
    {
#ifdef VAL_STRICT_ARGS
        // Validate application format public key data size
        DataSize = VAL_ASYM_DATA_SIZE_B2B(PubKey_p->ModulusSizeBits);
        if ((PubKeyPoint_p->x.Data_p == NULL) ||
            (PubKeyPoint_p->x.ByteDataSize < DataSize) ||
            (PubKeyPoint_p->y.Data_p == NULL) ||
            (PubKeyPoint_p->y.ByteDataSize < DataSize))
        {
            return VAL_BAD_ARGUMENT;
        }
#endif

        // Determine the HW size for the public key
        DataSize = 2 * VAL_ASYM_DATA_SIZE_VWB(PubKey_p->ModulusSizeBits);

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
    t_cmd.Service.PkAssetGenKey.Method = VEXTOKEN_PKASSET_ECDH_ECDSA_GEN_PUBKEY;
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
            if (PubKeyPoint_p != NULL)
            {
                valInternal_AsymECPointFromHW(Data_p,
                                              PubKey_p->ModulusSizeBits,
                                              PubKeyPoint_p);
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
#endif /* !VAL_REMOVE_ASYM_ECC_GEN_PUBLICKEY */


/*----------------------------------------------------------------------------
 * val_AsymEccKeyCheck
 */
#ifndef VAL_REMOVE_ASYM_ECC_KEY_CHECK
ValStatus_t
val_AsymEccKeyCheck(
        ValAsymKey_t * const PubKey_p,
        ValAsymKey_t * const PrivKey_p)
{
#ifdef VAL_STRICT_ARGS
    if (((PubKey_p == NULL) && (PrivKey_p == NULL)) ||
        ((PubKey_p != NULL) &&
         ((PubKey_p->DomainAssetId == VAL_ASSETID_INVALID) ||
          (PubKey_p->ModulusSizeBits < VAL_ASYM_ECP_MIN_BITS) ||
          (PubKey_p->ModulusSizeBits > VAL_ASYM_ECP_MAX_BITS))) ||
        ((PrivKey_p != NULL) &&
         ((PrivKey_p->DomainAssetId == VAL_ASSETID_INVALID) ||
          (PrivKey_p->ModulusSizeBits < VAL_ASYM_ECP_MIN_BITS) ||
          (PrivKey_p->ModulusSizeBits > VAL_ASYM_ECP_MAX_BITS))))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    return valInternal_AsymKeyCheck(PubKey_p, PrivKey_p, true);
}
#endif /* !VAL_REMOVE_ASYM_ECC_KEY_CHECK */


/* end of file adapter_val_asym_ecc.c */
