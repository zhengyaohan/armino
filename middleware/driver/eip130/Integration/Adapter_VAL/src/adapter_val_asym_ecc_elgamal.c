/* adapter_val_asym_ecc_elgamal.c
 *
 * Implementation of the VAL API.
 *
 * This file implements the asymmetric crypto ECC El-Gamal related services.
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
 * val_AsymEccElGamalAllocPrivateKeyAsset
 */
#ifndef VAL_REMOVE_ASYM_ECC_ELGAMAL_DECRYPT
ValStatus_t
val_AsymEccElGamalAllocPrivateKeyAsset(
        const ValSize_t ModulusSizeBits,
        const bool fCrossDomain,
        const bool fExportAllowed,
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
    AssetPolicy = VAL_POLICY_PK_ECC_ELGAMAL_ENC;
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
                          VAL_ASYM_DATA_SIZE_VWB(ModulusSizeBits),
                          false, false,
                          VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                          AssetId_p);
}
#endif


/*----------------------------------------------------------------------------
 * val_AsymEccElGamalAllocPublicKeyAsset
 */
#ifndef VAL_REMOVE_ASYM_ECC_ELGAMAL_ENCRYPT
ValStatus_t
val_AsymEccElGamalAllocPublicKeyAsset(
        const ValSize_t ModulusSizeBits,
        const bool fCrossDomain,
        const bool fExportAllowed,
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
    AssetPolicy = VAL_POLICY_PUBLIC_KEY | VAL_POLICY_PK_ECC_ELGAMAL_ENC;
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
                          (2 * VAL_ASYM_DATA_SIZE_VWB(ModulusSizeBits)),
                          false, false,
                          VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                          AssetId_p);
}
#endif


/*----------------------------------------------------------------------------
 * val_AsymEncryptEccElGamal
 */
#ifndef VAL_REMOVE_ASYM_ECC_ELGAMAL_ENCRYPT
ValStatus_t
val_AsymEccElGamalEncrypt(
        ValAsymKey_t * const KeyContext_p,
        ValAsymECCPoint_t * const MessagePoint_p,
        ValAsymECCPoint_t * const EncryptedPoint1_p,
        ValAsymECCPoint_t * const EncryptedPoint2_p)
{
    ValStatus_t funcres;
    uint8_t * SrcPoint_p = NULL;
    uint8_t * DstPoint_p = NULL;
    uint32_t PointSize;
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;

#ifdef VAL_STRICT_ARGS
    if (KeyContext_p == NULL)
    {
        return VAL_BAD_ARGUMENT;
    }
    PointSize = (uint32_t)VAL_ASYM_DATA_SIZE_B2B(KeyContext_p->ModulusSizeBits);
    if ((KeyContext_p->KeyAssetId == VAL_ASSETID_INVALID) ||
        (KeyContext_p->DomainAssetId == VAL_ASSETID_INVALID) ||
        (KeyContext_p->ModulusSizeBits < VAL_ASYM_ECP_MIN_BITS) ||
        (KeyContext_p->ModulusSizeBits > VAL_ASYM_ECP_MAX_BITS) ||
        (KeyContext_p->HashAlgorithm != VAL_SYM_ALGO_NONE) ||
        (MessagePoint_p == NULL) ||
        (MessagePoint_p->x.Data_p == NULL) ||
        (MessagePoint_p->x.ByteDataSize < PointSize) ||
        (MessagePoint_p->y.Data_p == NULL) ||
        (MessagePoint_p->y.ByteDataSize < PointSize) ||
        (EncryptedPoint1_p == NULL) ||
        (EncryptedPoint1_p->x.Data_p == NULL) ||
        (EncryptedPoint1_p->x.ByteDataSize < PointSize) ||
        (EncryptedPoint1_p->y.Data_p == NULL) ||
        (EncryptedPoint1_p->y.ByteDataSize < PointSize) ||
        (EncryptedPoint2_p == NULL) ||
        (EncryptedPoint2_p->x.Data_p == NULL) ||
        (EncryptedPoint2_p->x.ByteDataSize < PointSize) ||
        (EncryptedPoint2_p->y.Data_p == NULL) ||
        (EncryptedPoint2_p->y.ByteDataSize < PointSize))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Allocate temporary buffers for the EC points
    PointSize = (uint32_t)(2 * VAL_ASYM_DATA_SIZE_VWB(KeyContext_p->ModulusSizeBits));
    SrcPoint_p = Adapter_Alloc(PointSize);
    if (SrcPoint_p == NULL)
    {
        return VAL_NO_MEMORY;
    }

    DstPoint_p = Adapter_Alloc(2 * PointSize);
    if (DstPoint_p == NULL)
    {
        funcres = VAL_NO_MEMORY;
        goto error_func_exit;
    }

    // Convert Message Point to HW format
    valInternal_AsymECPointToHW(MessagePoint_p,
                                KeyContext_p->ModulusSizeBits,
                                0, 2, SrcPoint_p);

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_PUBLIC_KEY;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_PK_WITHASSETS;
    t_cmd.Service.PkAssetEncrypt.Method = VEXTOKEN_PKASSET_ECC_ELGAMAL_ENCRYPT;
    t_cmd.Service.PkAssetEncrypt.ModulusSizeInBits = (uint32_t)KeyContext_p->ModulusSizeBits;
    t_cmd.Service.PkAssetEncrypt.KeyAssetId = KeyContext_p->KeyAssetId;
    t_cmd.Service.PkAssetEncrypt.DomainAssetId = KeyContext_p->DomainAssetId;
    t_cmd.Service.PkAssetEncrypt.SrcData_p = SrcPoint_p;
    t_cmd.Service.PkAssetEncrypt.SrcDataSize = PointSize;
    t_cmd.Service.PkAssetEncrypt.DstData_p = DstPoint_p;
    t_cmd.Service.PkAssetEncrypt.DstDataSize = (2 * PointSize);
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if (funcres == VAL_SUCCESS)
    {
        // Check for errors
        if (t_res.Result == VEXTOKEN_RESULT_NO_ERROR)
        {
            // Convert the Encrypted Message Point (P1, P2) from HW to
            // application format
            valInternal_AsymECPointFromHW(DstPoint_p,
                                          KeyContext_p->ModulusSizeBits,
                                          EncryptedPoint1_p);
            valInternal_AsymECPointFromHW((DstPoint_p + PointSize),
                                          KeyContext_p->ModulusSizeBits,
                                          EncryptedPoint2_p);
        }
        else
        {
            funcres = (ValStatus_t)t_res.Result;
            LOG_WARN("Abort - %s()=%d\n", __func__, t_res.Result);
        }
    }

    // Clean-up
error_func_exit:
    if (SrcPoint_p != NULL)
    {
        Adapter_Free(SrcPoint_p);
    }
    if (DstPoint_p != NULL)
    {
        Adapter_Free(DstPoint_p);
    }
    return funcres;
}
#endif /* !VAL_REMOVE_ASYM_ECC_ELGAMAL_ENCRYPT */


/*----------------------------------------------------------------------------
 * val_AsymEccElGamalDecrypt
 */
#ifndef VAL_REMOVE_ASYM_ECC_ELGAMAL_DECRYPT
ValStatus_t
val_AsymEccElGamalDecrypt(
        ValAsymKey_t * const KeyContext_p,
        ValAsymECCPoint_t * const EncryptedPoint1_p,
        ValAsymECCPoint_t * const EncryptedPoint2_p,
        ValAsymECCPoint_t * const MessagePoint_p)
{
    ValStatus_t funcres;
    uint8_t * SrcPoint_p = NULL;
    uint8_t * DstPoint_p = NULL;
    uint32_t PointSize;
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;

#ifdef VAL_STRICT_ARGS
    if (KeyContext_p == NULL)
    {
        return VAL_BAD_ARGUMENT;
    }
    PointSize = (uint32_t)VAL_ASYM_DATA_SIZE_B2B(KeyContext_p->ModulusSizeBits);
    if ((KeyContext_p->KeyAssetId == VAL_ASSETID_INVALID) ||
        (KeyContext_p->DomainAssetId == VAL_ASSETID_INVALID) ||
        (KeyContext_p->ModulusSizeBits < VAL_ASYM_ECP_MIN_BITS) ||
        (KeyContext_p->ModulusSizeBits > VAL_ASYM_ECP_MAX_BITS) ||
        (KeyContext_p->HashAlgorithm != VAL_SYM_ALGO_NONE) ||
        (EncryptedPoint1_p == NULL) ||
        (EncryptedPoint1_p->x.Data_p == NULL) ||
        (EncryptedPoint1_p->x.ByteDataSize < PointSize) ||
        (EncryptedPoint1_p->y.Data_p == NULL) ||
        (EncryptedPoint1_p->y.ByteDataSize < PointSize) ||
        (EncryptedPoint2_p == NULL) ||
        (EncryptedPoint2_p->x.Data_p == NULL) ||
        (EncryptedPoint2_p->x.ByteDataSize < PointSize) ||
        (EncryptedPoint2_p->y.Data_p == NULL) ||
        (EncryptedPoint2_p->y.ByteDataSize < PointSize) ||
        (MessagePoint_p == NULL) ||
        (MessagePoint_p->x.Data_p == NULL) ||
        (MessagePoint_p->x.ByteDataSize < PointSize) ||
        (MessagePoint_p->y.Data_p == NULL) ||
        (MessagePoint_p->y.ByteDataSize < PointSize))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    // Allocate temporary buffers for the EC points
    PointSize = (uint32_t)(2 * VAL_ASYM_DATA_SIZE_VWB(KeyContext_p->ModulusSizeBits));
    SrcPoint_p = Adapter_Alloc(2 * PointSize);
    if (SrcPoint_p == NULL)
    {
        return VAL_NO_MEMORY;
    }

    DstPoint_p = Adapter_Alloc(PointSize);
    if (DstPoint_p == NULL)
    {
        funcres = VAL_NO_MEMORY;
        goto error_func_exit;
    }
    memset(DstPoint_p, 0, PointSize);

    // Convert Encrypted Message Point (P1, P2) to HW format
    valInternal_AsymECPointToHW(EncryptedPoint1_p,
                                KeyContext_p->ModulusSizeBits,
                                0, 4, SrcPoint_p);
    valInternal_AsymECPointToHW(EncryptedPoint2_p,
                                KeyContext_p->ModulusSizeBits,
                                2, 4, (SrcPoint_p + PointSize));

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_PUBLIC_KEY;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_PK_WITHASSETS;
    t_cmd.Service.PkAssetEncrypt.Method = VEXTOKEN_PKASSET_ECC_ELGAMAL_DECRYPT;
    t_cmd.Service.PkAssetEncrypt.ModulusSizeInBits = (uint32_t)KeyContext_p->ModulusSizeBits;
    t_cmd.Service.PkAssetEncrypt.KeyAssetId = KeyContext_p->KeyAssetId;
    t_cmd.Service.PkAssetEncrypt.DomainAssetId = KeyContext_p->DomainAssetId;
    t_cmd.Service.PkAssetEncrypt.SrcData_p = SrcPoint_p;
    t_cmd.Service.PkAssetEncrypt.SrcDataSize = (2 * PointSize);
    t_cmd.Service.PkAssetEncrypt.DstData_p = DstPoint_p;
    t_cmd.Service.PkAssetEncrypt.DstDataSize = PointSize;
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if (funcres == VAL_SUCCESS)
    {
        // Check for errors
        if (t_res.Result == VEXTOKEN_RESULT_NO_ERROR)
        {
            // Convert the Message Point from HW to application format
            valInternal_AsymECPointFromHW(DstPoint_p,
                                          KeyContext_p->ModulusSizeBits,
                                          MessagePoint_p);
        }
        else
        {
            funcres = (ValStatus_t)t_res.Result;
            LOG_WARN("Abort - %s()=%d\n", __func__, t_res.Result);
        }
    }

    // Clean-up
error_func_exit:
    if (SrcPoint_p != NULL)
    {
        Adapter_Free(SrcPoint_p);
    }
    if (DstPoint_p != NULL)
    {
        Adapter_Free(DstPoint_p);
    }
    return funcres;
}
#endif /* !VAL_REMOVE_ASYM_ECC_ELGAMAL_DECRYPT */


/* end of file adapter_val_asym_ecc_elgamal.c */
