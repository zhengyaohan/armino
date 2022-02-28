/* adapter_val_asym_common.c
 *
 * Implementation of the VAL API.
 *
 * This file implements the asymmetric crypto related common services.
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
#include "adapter_val_internal.h"       // Various defines and typedefs


/*----------------------------------------------------------------------------
 * val_AsymInitKey
 */
ValStatus_t
val_AsymInitKey(
        const ValAssetId_Optional_t KeyAssetId,
        const ValAssetId_Optional_t DomainAssetId,
        const ValSize_t ModulusSizeBits,
        const ValSymAlgo_t HashAlgorithm,
        ValAsymKey_t * const KeyContext_p)
{
#ifdef VAL_STRICT_ARGS
    if (((KeyAssetId == VAL_ASSETID_INVALID) &&
         (DomainAssetId == VAL_ASSETID_INVALID)) ||
        (ModulusSizeBits == 0) ||
        (KeyContext_p == NULL))
    {
        return VAL_BAD_ARGUMENT;
    }
#endif

    switch (HashAlgorithm)
    {
        // DSA, ECDSA or RSA key
    case VAL_SYM_ALGO_HASH_SHA1:
#ifdef VAL_STRICT_ARGS
        if (ModulusSizeBits < 160)
        {
            return VAL_BAD_ARGUMENT;
        }
#endif
        break;
    case VAL_SYM_ALGO_HASH_SHA224:
#ifdef VAL_STRICT_ARGS
        if (ModulusSizeBits < 224)
        {
            return VAL_BAD_ARGUMENT;
        }
#endif
        break;
    case VAL_SYM_ALGO_HASH_SHA256:
#ifdef VAL_STRICT_ARGS
        if (ModulusSizeBits < 256)
        {
            return VAL_BAD_ARGUMENT;
        }
#endif
        break;
#ifndef VAL_REMOVE_SYM_ALGO_SHA512
    case VAL_SYM_ALGO_HASH_SHA384:
#ifdef VAL_STRICT_ARGS
        if (ModulusSizeBits < 384)
        {
            return VAL_BAD_ARGUMENT;
        }
#endif
        break;
    case VAL_SYM_ALGO_HASH_SHA512:
#ifdef VAL_STRICT_ARGS
        if ((ModulusSizeBits < 512) && (ModulusSizeBits != 255))
        {
            return VAL_BAD_ARGUMENT;
        }
#endif
        break;
#endif

        // DH, ECDH, ECC El-Gamal or Curve25519 key
    case VAL_SYM_ALGO_NONE:
        break;

    default:
        return VAL_INVALID_ALGORITHM;
    }

    // Initialize key structure
    KeyContext_p->KeyAssetId = KeyAssetId;
    KeyContext_p->DomainAssetId = DomainAssetId;
    KeyContext_p->ModulusSizeBits = ModulusSizeBits;
    KeyContext_p->HashAlgorithm = HashAlgorithm;

    return VAL_SUCCESS;
}


/*----------------------------------------------------------------------------
 * valInternal_AsymDsaSignatureToHW
 */
void
valInternal_AsymDsaSignatureToHW(
        const ValAsymSign_t * const Signature_p,
        const ValSize_t ModulusSizeBits,
        uint8_t * const Blob_p)
{
    // Convert Signature from application to HW format
    // - Signature.r
    valInternal_AsymBigIntToHW(&Signature_p->r, ModulusSizeBits, 0, 2,
                               Blob_p);
    // - Signature.s
    valInternal_AsymBigIntToHW(&Signature_p->s, ModulusSizeBits, 1, 2,
                               Blob_p + VAL_ASYM_DATA_SIZE_VWB(ModulusSizeBits));
}


/*----------------------------------------------------------------------------
 * valInternal_AsymDsaSignatureFromHW
 */
void
valInternal_AsymDsaSignatureFromHW(
        const uint8_t * const Blob_p,
        const ValSize_t ModulusSizeBits,
        ValAsymSign_t * const Signature_p)
{
    // Convert Signature from HW to application format
    valInternal_AsymBigIntFromHW(Blob_p,
                                 ModulusSizeBits, &Signature_p->r);
    valInternal_AsymBigIntFromHW(Blob_p + VAL_ASYM_DATA_VHEADER + VAL_ASYM_DATA_SIZE_B2WB(ModulusSizeBits),
                                 ModulusSizeBits, &Signature_p->s);
}


/*----------------------------------------------------------------------------
 * valInternal_AsymECPointToHW
 */
void
valInternal_AsymECPointToHW(
        const ValAsymECCPoint_t * const Point_p,
        const ValSize_t ModulusSizeBits,
        const uint8_t BeginItem,
        const uint8_t Items,
        uint8_t * const Blob_p)
{
    // Convert EC Point from application to HW format
    // - Point.x
    valInternal_AsymBigIntToHW(&Point_p->x, ModulusSizeBits,
                               BeginItem, Items,
                               Blob_p);
    // - Point.y
    valInternal_AsymBigIntToHW(&Point_p->y, ModulusSizeBits,
                               (uint8_t)(BeginItem + 1), Items,
                               Blob_p + VAL_ASYM_DATA_SIZE_VWB(ModulusSizeBits));
}


/*----------------------------------------------------------------------------
 * valInternal_AsymECPointFromHW
 */
void
valInternal_AsymECPointFromHW(
        const uint8_t * const Blob_p,
        const ValSize_t ModulusSizeBits,
        ValAsymECCPoint_t * const Point_p)
{
    // Convert EC Point from HW to application format
    valInternal_AsymBigIntFromHW(Blob_p,
                                 ModulusSizeBits, &Point_p->x);
    valInternal_AsymBigIntFromHW(Blob_p + VAL_ASYM_DATA_VHEADER + VAL_ASYM_DATA_SIZE_B2WB(ModulusSizeBits),
                                 ModulusSizeBits, &Point_p->y);
}


/*----------------------------------------------------------------------------
 * valInternal_AsymBigIntToHW
 */
void
valInternal_AsymBigIntToHW(
        const ValAsymBigInt_t * const BigInt_p,
        const ValSize_t ModulusSizeBits,
        const uint8_t BeginItem,
        const uint8_t Items,
        uint8_t * const Blob_p)
{
    uint8_t * ptr = (uint8_t *)Blob_p;
    uint32_t RemainingLength = (uint32_t)VAL_ASYM_DATA_SIZE_B2WB(ModulusSizeBits);
    uint32_t CopySize = BigInt_p->ByteDataSize;

    // Convert big integer from application to HW format
    // - Initialize header
    *ptr++ = (uint8_t)(ModulusSizeBits);
    *ptr++ = (uint8_t)(ModulusSizeBits >> 8);
    *ptr++ = (uint8_t)(BeginItem);
    *ptr++ = (uint8_t)(Items);
    // - Copy data
#ifdef VAL_STRICT_ARGS
    if (CopySize > RemainingLength)
    {
        // Prevent buffer overrun
        LOG_WARN("Warning: Truncated BigInt data to HW conversion (%u > %u)\n",
                 CopySize, RemainingLength);
        CopySize = RemainingLength;
    }
#endif
    valInternal_ReverseMemCpy(ptr, BigInt_p->Data_p, CopySize);
    RemainingLength -= CopySize;
    if (RemainingLength != 0)
    {
        ptr += CopySize;
        memset(ptr, 0, RemainingLength);
    }
}


/*----------------------------------------------------------------------------
 * valInternal_AsymBigIntFromHW
 */
void
valInternal_AsymBigIntFromHW(
        const uint8_t * const Blob_p,
        const ValSize_t ModulusSizeBits,
        ValAsymBigInt_t * const BigInt_p)
{
    uint32_t Size = (uint32_t)VAL_ASYM_DATA_SIZE_B2B(ModulusSizeBits);

    // Convert big integer from HW to application format
#ifdef VAL_STRICT_ARGS
    if (Size > BigInt_p->ByteDataSize)
    {
        LOG_WARN("Warning: Truncated BigInt data from HW conversion (%u > %u)\n",
                 Size, BigInt_p->ByteDataSize);
        Size = BigInt_p->ByteDataSize;
    }
#endif
    valInternal_ReverseMemCpy(BigInt_p->Data_p, (Blob_p + VAL_ASYM_DATA_VHEADER), Size);
    BigInt_p->ByteDataSize = Size;
}


/*----------------------------------------------------------------------------
 * valInternal_AsymKeyCheck
 */
#if !defined(VAL_REMOVE_ASYM_ECC_KEY_CHECK) || \
    !defined(VAL_REMOVE_ASYM_DH_KEY_CHECK) || \
    !defined(VAL_REMOVE_ASYM_DSA_KEY_CHECK)
ValStatus_t
valInternal_AsymKeyCheck(
        ValAsymKey_t * const PubKey_p,
        ValAsymKey_t * const PrivKey_p,
        const bool fEcc)
{
    ValStatus_t funcres;
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValAssetId_t DomainAssetId;

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_PUBLIC_KEY;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_PK_WITHASSETS;
    if (fEcc)
    {
        t_cmd.Service.PkAssetKeyCheck.Method = VEXTOKEN_PKASSET_ECDH_ECDSA_KEYCHK;
    }
    else
    {
        t_cmd.Service.PkAssetKeyCheck.Method = VEXTOKEN_PKASSET_DH_DSA_KEYCHK;
    }
    if (PubKey_p != NULL)
    {
        t_cmd.Service.PkAssetKeyCheck.PubKeyAssetId = PubKey_p->KeyAssetId;
        t_cmd.Service.PkAssetKeyCheck.ModulusSizeInBits = (uint32_t)PubKey_p->ModulusSizeBits;
        DomainAssetId = PubKey_p->DomainAssetId;
    }
    else
    {
        t_cmd.Service.PkAssetKeyCheck.PubKeyAssetId = VAL_ASSETID_INVALID;
        t_cmd.Service.PkAssetKeyCheck.ModulusSizeInBits = 0;
        DomainAssetId = VAL_ASSETID_INVALID;
    }
    if (PrivKey_p != NULL)
    {
        t_cmd.Service.PkAssetKeyCheck.PrivKeyAssetId = PrivKey_p->KeyAssetId;
        t_cmd.Service.PkAssetKeyCheck.DivisorSizeInBits = (uint32_t)PrivKey_p->ModulusSizeBits;

        if (t_cmd.Service.PkAssetKeyCheck.ModulusSizeInBits == 0)
        {
            t_cmd.Service.PkAssetKeyCheck.ModulusSizeInBits =
                t_cmd.Service.PkAssetKeyCheck.DivisorSizeInBits;
        }
#ifdef VAL_STRICT_ARGS
        else if (fEcc &&
                 (t_cmd.Service.PkAssetKeyCheck.ModulusSizeInBits !=
                  t_cmd.Service.PkAssetKeyCheck.DivisorSizeInBits))
        {
            return VAL_BAD_ARGUMENT;
        }
#endif

        if (DomainAssetId == VAL_ASSETID_INVALID)
        {
            DomainAssetId = PrivKey_p->DomainAssetId;
        }
#ifdef VAL_STRICT_ARGS
        else if (DomainAssetId != PrivKey_p->DomainAssetId)
        {
            return VAL_BAD_ARGUMENT;
        }
#endif
    }
    else
    {
        t_cmd.Service.PkAssetKeyCheck.PrivKeyAssetId = VAL_ASSETID_INVALID;
        t_cmd.Service.PkAssetKeyCheck.DivisorSizeInBits =
            t_cmd.Service.PkAssetKeyCheck.ModulusSizeInBits;
    }
    t_cmd.Service.PkAssetKeyCheck.DomainAssetId = DomainAssetId;
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


/* end of file adapter_val_asym_common.c */
