/* valtest_asym_ecdsa.c
 *
 * Description: ECDSA tests
 */

/*****************************************************************************
* Copyright (c) 2014-2018 INSIDE Secure B.V. All Rights Reserved.
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

#include "valtest_internal.h"
#define VALTEST_DOKEYCHECK

/* Test vectors. */
#include "testvectors_ecdsa.h"

/* Test helper function. */
int
test_AsymEccArguments(void);


int
test_AsymEccLoadDomain(
        const TestVector_ECC_Curve_Rec_t * Curve_p,
        ValAssetId_t * DomainAssetId_p,
        bool ftestExport);


static int
DoEccEcdsaParamsTest(
        int VectorIndex,
        TestVector_ECDSA_t Vector_p)
{
    int rc;
    ValStatus_t Status;
    ValAsymKey_t PrivateKey;
    ValAsymKey_t PublicKey;
    ValAssetId_t DomainAssetId = VAL_ASSETID_INVALID;

    IDENTIFIER_NOT_USED(VectorIndex);

    /* Load domain parameters */
    rc = test_AsymEccLoadDomain(Vector_p->Curve_p, &DomainAssetId, true);
    if (rc != END_TEST_SUCCES)
    {
        return rc;
    }

    /* Dummy key entries */
    Status = val_AsymInitKey(VAL_ASSETID_INVALID, DomainAssetId,
                             Vector_p->Curve_p->CurveBits, VAL_SYM_ALGO_HASH_SHA1,
                             &PrivateKey);
    fail_if(Status != VAL_SUCCESS, "val_AsymInitKey()=", Status);

    Status = val_AsymInitKey(VAL_ASSETID_INVALID, DomainAssetId,
                             Vector_p->Curve_p->CurveBits, VAL_SYM_ALGO_HASH_SHA1,
                             &PublicKey);
    fail_if(Status != VAL_SUCCESS, "val_AsymInitKey()=", Status);

    /* Check public key (curve) parameters  */
    Status = val_AsymEccKeyCheck(&PublicKey, &PrivateKey);
    fail_if(Status != VAL_SUCCESS, "val_AsymEccKeyCheck()=", Status);

    /* Clean-up*/
    Status = val_AssetFree(DomainAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree()=", Status);

    if (valtest_StrictArgsCheck())
    {
        Status = val_AsymEccKeyCheck(NULL, NULL);
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccKeyCheck(BadArgument1&2)=", Status);

        (void)val_AsymInitKey(VAL_ASSETID_INVALID, DomainAssetId,
                              (VAL_ASYM_ECP_MIN_BITS - 1), VAL_SYM_ALGO_HASH_SHA1,
                              &PublicKey);
        Status = val_AsymEccKeyCheck(&PublicKey, &PrivateKey);
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccKeyCheck(BadArgument1I)=", Status);
        Status = val_AsymEccKeyCheck(&PrivateKey, &PublicKey);
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccKeyCheck(BadArgument2I)=", Status);

        (void)val_AsymInitKey(VAL_ASSETID_INVALID, DomainAssetId,
                              (VAL_ASYM_ECP_MAX_BITS + 1), VAL_SYM_ALGO_HASH_SHA1,
                              &PublicKey);
        Status = val_AsymEccKeyCheck(&PublicKey, &PrivateKey);
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccKeyCheck(BadArgument1I)=", Status);
        Status = val_AsymEccKeyCheck(&PrivateKey, &PublicKey);
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccKeyCheck(BadArgument2I)=", Status);

        (void)val_AsymInitKey(VAL_ASSETID_INVALID, VAL_ASSETID_INVALID,
                              VAL_ASYM_ECP_MIN_BITS, VAL_SYM_ALGO_HASH_SHA1,
                              &PublicKey);
        Status = val_AsymEccKeyCheck(&PublicKey, &PrivateKey);
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccKeyCheck(BadArgument1I)=", Status);
        Status = val_AsymEccKeyCheck(&PrivateKey, &PublicKey);
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccKeyCheck(BadArgument2I)=", Status);
    }

    return END_TEST_SUCCES;
}


static int
DoEccEcdsaGenKeyPairTest(
        int VectorIndex,
        TestVector_ECDSA_t Vector_p)
{
    int rc;
    uint8_t Buffer[(521+7)/8];
    uint8_t * PrivKeyBlob_p;
    ValSize_t PrivKeyBlobSize;
    ValSize_t tempPrivKeyBlobSize;
    uint8_t * AssociatedData_p;
    ValSize_t AssociatedDataSize = strlen(g_ValTestAssociatedData_p);
    ValStatus_t Status;
    ValAsymKey_t PrivateKey;
    ValAsymKey_t PublicKey;
    ValAsymECCPoint_t Point;
    ValAssetId_t RootAssetId;
    ValAssetId_t KEKAssetId = VAL_ASSETID_INVALID;
    ValAssetId_t DomainAssetId = VAL_ASSETID_INVALID;
    ValAssetId_t PrivateKeyAssetId = VAL_ASSETID_INVALID;
    ValAssetId_t PublicKeyAssetId = VAL_ASSETID_INVALID;
    ValSymAlgo_t HashAlgorithm;
    ValPolicyMask_t AssetPolicy;
    uint32_t ModulusSizeBits;

    IDENTIFIER_NOT_USED(VectorIndex);

    //  Get root key
    RootAssetId = val_AssetGetRootKey();
    unsupported_if((RootAssetId == VAL_ASSETID_INVALID), "No Root key");

    AssociatedData_p = (uint8_t *)SFZUTF_MALLOC(AssociatedDataSize);
    fail_if(AssociatedData_p == NULL, "Allocation ", (int)AssociatedDataSize);
    memcpy(AssociatedData_p, g_ValTestAssociatedData_p, AssociatedDataSize);

    // Create and derive KEK Asset
    AssetPolicy = VAL_POLICY_TRUSTED_WRAP | VAL_POLICY_ENCRYPT | VAL_POLICY_DECRYPT;
    if (!val_IsAccessSecure())
    {
        AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
    }
    Status = val_AssetAlloc(AssetPolicy, 64,
                            false, false,
                            VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                            &KEKAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(KEK)=", Status);

    Status = val_AssetLoadDerive(KEKAssetId, RootAssetId,
                                 AssociatedData_p, AssociatedDataSize,
                                 false, false, NULL, 0, NULL, 0);
    fail_if(Status != VAL_SUCCESS, "val_AssetLoadDerive(KEK)=", Status);

    /* Load domain parameters */
    rc = test_AsymEccLoadDomain(Vector_p->Curve_p, &DomainAssetId, false);
    if (rc != END_TEST_SUCCES)
    {
        return rc;
    }
    ModulusSizeBits = Vector_p->Curve_p->CurveBits;

    /* Determine Hash algorithm */
    switch (Vector_p->HashBits)
    {
    default:
    case 160:
        HashAlgorithm = VAL_SYM_ALGO_HASH_SHA1;
        break;
    case 224:
        HashAlgorithm = VAL_SYM_ALGO_HASH_SHA224;
        break;
    case 256:
        HashAlgorithm = VAL_SYM_ALGO_HASH_SHA256;
        break;
    }

    /* Allocate the private key asset and initialize key structure */
    Status = val_AsymEcdsaAllocPrivateKeyAsset(ModulusSizeBits,
                                               HashAlgorithm,
                                               false, false,
                                               &PrivateKeyAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AsymEcdsaAllocPrivateKeyAsset()=", Status);

    Status = val_AsymInitKey(PrivateKeyAssetId, DomainAssetId,
                             ModulusSizeBits, HashAlgorithm,
                             &PrivateKey);
    fail_if(Status != VAL_SUCCESS, "val_AsymInitKey()=", Status);

    /* Allocate the public key asset and initialize key structure */
    Status = val_AsymEcdsaAllocPublicKeyAsset(ModulusSizeBits,
                                              HashAlgorithm,
                                              false, false,
                                              &PublicKeyAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AsymEcdsaAllocPublicKeyAsset()=", Status);

    Status = val_AsymInitKey(PublicKeyAssetId, DomainAssetId,
                             ModulusSizeBits, HashAlgorithm,
                             &PublicKey);
    fail_if(Status != VAL_SUCCESS, "val_AsymInitKey()=", Status);

    /* Initialize ECC point buffer for public key */
    Point.x.Data_p = Buffer;
    Point.x.ByteDataSize = sizeof(Buffer);
    Point.y.Data_p = Buffer;
    Point.y.ByteDataSize = sizeof(Buffer);

    /* Retrieve private key keyblob size */
    PrivKeyBlobSize = 0;
    Status = val_AsymEccGenKeyPair(&PublicKey, &PrivateKey,
                                   KEKAssetId,
                                   AssociatedData_p, AssociatedDataSize,
                                   &Point,
                                   AssociatedData_p, &PrivKeyBlobSize);
    fail_if(Status != VAL_BUFFER_TOO_SMALL, "val_AsymEccGenKeyPair()=", Status);

    PrivKeyBlobSize = 0;
    Status = val_AsymEccGenKeyPair(&PublicKey, &PrivateKey,
                                   KEKAssetId,
                                   AssociatedData_p, AssociatedDataSize,
                                   &Point,
                                   NULL, &PrivKeyBlobSize);
    fail_if(Status != VAL_BUFFER_TOO_SMALL, "val_AsymEccGenKeyPair()=", Status);
    fail_if(PrivKeyBlobSize != VAL_KEYBLOB_SIZE(VAL_ASYM_DATA_SIZE_B2WB(ModulusSizeBits)+4),
            "Size mismatch ", (int)VAL_KEYBLOB_SIZE(VAL_ASYM_DATA_SIZE_B2WB(ModulusSizeBits)+4));

    PrivKeyBlob_p = (uint8_t *)SFZUTF_MALLOC(PrivKeyBlobSize);
    fail_if(PrivKeyBlob_p == NULL, "Allocation ", (int)PrivKeyBlobSize);

    /* Switch keys */
    tempPrivKeyBlobSize = PrivKeyBlobSize;
    Status = val_AsymEccGenKeyPair(&PrivateKey, &PublicKey,
                                   KEKAssetId,
                                   AssociatedData_p, AssociatedDataSize,
                                   &Point,
                                   PrivKeyBlob_p, &tempPrivKeyBlobSize);
    fail_if(Status == VAL_SUCCESS, "val_AsymEccGenKeyPair()=", Status);

    /* Generate a key pair */
    Status = val_AsymEccGenKeyPair(&PublicKey, &PrivateKey,
                                   KEKAssetId,
                                   AssociatedData_p, AssociatedDataSize,
                                   &Point,
                                   PrivKeyBlob_p, &PrivKeyBlobSize);
    fail_if(Status != VAL_SUCCESS, "val_AsymEccGenKeyPair()=", Status);
    fail_if(Point.x.ByteDataSize != VAL_ASYM_DATA_SIZE_B2B(ModulusSizeBits),
            "Size mismatch ", (int)VAL_ASYM_DATA_SIZE_B2B(ModulusSizeBits));
    fail_if(Point.y.ByteDataSize != VAL_ASYM_DATA_SIZE_B2B(ModulusSizeBits),
            "Size mismatch ", (int)VAL_ASYM_DATA_SIZE_B2B(ModulusSizeBits));

    /* Clean-up*/
    Status = val_AssetFree(PrivateKeyAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree()=", Status);

    Status = val_AssetFree(PublicKeyAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree()=", Status);

    Status = val_AssetFree(DomainAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree()=", Status);

    SFZUTF_FREE(PrivKeyBlob_p);
    SFZUTF_FREE(AssociatedData_p);

    return END_TEST_SUCCES;
}


static int
DoEcdsaTest(
        int VectorIndex,
        TestVector_ECDSA_t Vector_p,
        bool fTestKeyPair,
        bool fGenerateKeyPair,
        bool fGeneratePublicKey,
        bool fUseTokenHashContext)
{
    int rc;
    uint8_t * AssociatedData_p;
    ValSize_t AssociatedDataSize = strlen(g_ValTestAssociatedData_p);
    uint8_t * InCopy_p;
    ValStatus_t Status;
    ValAsymKey_t PrivateKey;
    ValAsymKey_t PublicKey;
    ValAsymSign_t Signature;
    ValAssetId_t RootAssetId = VAL_ASSETID_INVALID;
    ValAssetId_t KEKAssetId = VAL_ASSETID_INVALID;
    ValAssetId_t DomainAssetId = VAL_ASSETID_INVALID;
    ValAssetId_t PrivateKeyAssetId = VAL_ASSETID_INVALID;
    ValAssetId_t PublicKeyAssetId = VAL_ASSETID_INVALID;
    ValSymAlgo_t HashAlgorithm;
    uint32_t ModulusSizeBits;

    unsupported_unless(valtest_IsTrngActive(true), "TRNG not activated");

    AssociatedData_p = (uint8_t *)SFZUTF_MALLOC(AssociatedDataSize);
    fail_if(AssociatedData_p == NULL, "Allocation ", (int)AssociatedDataSize);
    memcpy(AssociatedData_p, g_ValTestAssociatedData_p, AssociatedDataSize);

    if ((VectorIndex & 1) != 0)
    {
        RootAssetId = val_AssetGetRootKey();
        if (RootAssetId != VAL_ASSETID_INVALID)
        {
            ValPolicyMask_t AssetPolicy;

            // Create and derive KEK Asset
            AssetPolicy = VAL_POLICY_TRUSTED_WRAP | VAL_POLICY_ENCRYPT | VAL_POLICY_DECRYPT;
            if (!val_IsAccessSecure())
            {
                AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
            }
            Status = val_AssetAlloc(AssetPolicy, 64,
                                    false, false,
                                    VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                                    &KEKAssetId);
            fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(KEK)=", Status);

            Status = val_AssetLoadDerive(KEKAssetId, RootAssetId,
                                         AssociatedData_p, AssociatedDataSize,
                                         false, false, NULL, 0, NULL, 0);
            fail_if(Status != VAL_SUCCESS, "val_AssetLoadDerive(KEK)=", Status);
        }
    }

    /* Load domain parameters */
    rc = test_AsymEccLoadDomain(Vector_p->Curve_p, &DomainAssetId, false);
    if (rc != END_TEST_SUCCES)
    {
        return rc;
    }
    ModulusSizeBits = Vector_p->Curve_p->CurveBits;

    /* Determine Hash algorithm */
    switch (Vector_p->HashBits)
    {
    case 160:
        HashAlgorithm = VAL_SYM_ALGO_HASH_SHA1;
        break;
    case 224:
        HashAlgorithm = VAL_SYM_ALGO_HASH_SHA224;
        break;
    case 256:
        HashAlgorithm = VAL_SYM_ALGO_HASH_SHA256;
        break;
#ifdef VALTEST_SYM_ALGO_SHA512
    case 384:
        HashAlgorithm = VAL_SYM_ALGO_HASH_SHA384;
        break;
    case 512:
        HashAlgorithm = VAL_SYM_ALGO_HASH_SHA512;
        break;
#endif
    default:
        return END_TEST_UNSUPPORTED;
    }

    /* */
    Status = val_AsymEcdsaAllocPrivateKeyAsset(ModulusSizeBits,
                                               VAL_SYM_ALGO_HASH,
                                               false, false,
                                               &PrivateKeyAssetId);
    fail_if(Status != VAL_INVALID_ALGORITHM, "val_AsymEcdsaAllocPrivateKeyAsset(Wrong Hash)=", Status);

    Status = val_AsymEcdsaAllocPublicKeyAsset(ModulusSizeBits,
                                              VAL_SYM_ALGO_HASH,
                                              false, false,
                                              &PublicKeyAssetId);
    fail_if(Status != VAL_INVALID_ALGORITHM, "val_AsymEcdsaAllocPublicKeyAsset(Wrong Hash)=", Status);

    /* Allocate the private key asset and initialize key structure */
    Status = val_AsymEcdsaAllocPrivateKeyAsset(ModulusSizeBits,
                                               HashAlgorithm,
                                               false, false,
                                               &PrivateKeyAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AsymEcdsaAllocPrivateKeyAsset()=", Status);

    Status = val_AsymInitKey(PrivateKeyAssetId, DomainAssetId,
                             ModulusSizeBits, HashAlgorithm,
                             &PrivateKey);
    fail_if(Status != VAL_SUCCESS, "val_AsymInitKey()=", Status);

    /* Allocate the public key asset and initialize key structure */
    Status = val_AsymEcdsaAllocPublicKeyAsset(ModulusSizeBits,
                                              HashAlgorithm,
                                              false, false,
                                              &PublicKeyAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AsymEcdsaAllocPublicKeyAsset()=", Status);

    Status = val_AsymInitKey(PublicKeyAssetId, DomainAssetId,
                             ModulusSizeBits, HashAlgorithm,
                             &PublicKey);
    fail_if(Status != VAL_SUCCESS, "val_AsymInitKey()=", Status);

    if (fGenerateKeyPair)
    {
        /* Generate a key pair */
        Status = val_AsymEccGenKeyPair(&PublicKey, &PrivateKey,
                                       VAL_ASSETID_INVALID,
                                       NULL, 0, NULL, NULL, NULL);
        fail_if(Status != VAL_SUCCESS, "val_AsymEccGenKeyPair()=", Status);

        fTestKeyPair = true;            // Force key pair check
    }
    else
    {
        ValAsymBigInt_t BigIntPrivateKey;
        ValAsymECCPoint_t ECCPointPublicKey;

        /* Initialize the private key asset */
        BigIntPrivateKey.Data_p = sfzutf_discard_const(Vector_p->PrivateKey_p);
        BigIntPrivateKey.ByteDataSize = Vector_p->PrivateKeyLen;
        if (KEKAssetId != VAL_ASSETID_INVALID)
        {
            uint8_t * KeyBlob_p;
            ValSize_t KeyBlobSize = 0;

            Status = val_AsymEccLoadPrivateKeyAssetPlaintext(&BigIntPrivateKey,
                                                             ModulusSizeBits,
                                                             PrivateKeyAssetId,
                                                             KEKAssetId,
                                                             AssociatedData_p, AssociatedDataSize,
                                                             AssociatedData_p, &KeyBlobSize);
            fail_if(Status != VAL_BUFFER_TOO_SMALL, "val_AsymEccLoadPrivateKeyAssetPlaintext()=", Status);

            KeyBlobSize = 0;
            Status = val_AsymEccLoadPrivateKeyAssetPlaintext(&BigIntPrivateKey,
                                                             ModulusSizeBits,
                                                             PrivateKeyAssetId,
                                                             KEKAssetId,
                                                             AssociatedData_p, AssociatedDataSize,
                                                             NULL, &KeyBlobSize);
            fail_if(Status != VAL_BUFFER_TOO_SMALL, "val_AsymEccLoadPrivateKeyAssetPlaintext()=", Status);

            KeyBlob_p = (uint8_t *)SFZUTF_MALLOC(KeyBlobSize);
            fail_if(KeyBlob_p == NULL, "Allocation ", (int)KeyBlobSize);

            Status = val_AsymEccLoadPrivateKeyAssetPlaintext(&BigIntPrivateKey,
                                                             ModulusSizeBits,
                                                             PrivateKeyAssetId,
                                                             KEKAssetId,
                                                             AssociatedData_p, AssociatedDataSize,
                                                             KeyBlob_p, &KeyBlobSize);
            fail_if(Status != VAL_SUCCESS, "val_AsymEccLoadPrivateKeyAssetPlaintext()=", Status);

            SFZUTF_FREE(KeyBlob_p);
        }
        else
        {
            Status = val_AsymEccLoadPrivateKeyAssetPlaintext(&BigIntPrivateKey,
                                                             ModulusSizeBits,
                                                             PrivateKeyAssetId,
                                                             VAL_ASSETID_INVALID,
                                                             NULL, 0, NULL, NULL);
            fail_if(Status != VAL_SUCCESS, "val_AsymEccLoadPrivateKeyAssetPlaintext()=", Status);
        }

        /* Perform a private key check */
#ifdef VALTEST_DOKEYCHECK
        Status = val_AsymEccKeyCheck(NULL, &PrivateKey);
        fail_if(Status != VAL_SUCCESS, "val_AsymEccKeyCheck()=", Status);
#endif

        if (fGeneratePublicKey)
        {
            /* Generate a public key */
            ECCPointPublicKey.x.ByteDataSize = VAL_ASYM_DATA_SIZE_B2B(ModulusSizeBits);
            ECCPointPublicKey.y.ByteDataSize = ECCPointPublicKey.x.ByteDataSize;
            ECCPointPublicKey.x.Data_p = (uint8_t *)SFZUTF_MALLOC(ECCPointPublicKey.x.ByteDataSize);
            ECCPointPublicKey.y.Data_p = (uint8_t *)SFZUTF_MALLOC(ECCPointPublicKey.y.ByteDataSize);
            fail_if((ECCPointPublicKey.x.Data_p == NULL) ||
                    (ECCPointPublicKey.y.Data_p == NULL),
                    "Allocation ", (int)ECCPointPublicKey.y.ByteDataSize);
            memset(ECCPointPublicKey.x.Data_p, 0, ECCPointPublicKey.x.ByteDataSize);
            memset(ECCPointPublicKey.y.Data_p, 0, ECCPointPublicKey.y.ByteDataSize);

            if ((VectorIndex & 1) != 0)
            {
                Status = val_AsymEccGenPublicKey(&PublicKey, &PrivateKey, NULL);
                fail_if(Status != VAL_SUCCESS, "val_AsymEccGenPublicKey()=", Status);
            }
            else
            {
                Status = val_AsymEccGenPublicKey(&PublicKey, &PrivateKey, &ECCPointPublicKey);
                fail_if(Status != VAL_SUCCESS, "val_AsymEccGenPublicKey()=", Status);

                /* try to load a second time to improve coverage */
                Status = val_AsymEccGenPublicKey(&PublicKey, &PrivateKey, NULL);
                fail_if(Status != VAL_INVALID_ASSET, "val_AsymEccGenPublicKey()=", Status);
            }

            fTestKeyPair = true;            // Force key pair check

            SFZUTF_FREE(ECCPointPublicKey.x.Data_p);
            SFZUTF_FREE(ECCPointPublicKey.y.Data_p);
        }
        else
        {
            ECCPointPublicKey.x.Data_p = sfzutf_discard_const(Vector_p->PublicKeyX_p);
            ECCPointPublicKey.x.ByteDataSize = Vector_p->PublicKeyXLen;
            ECCPointPublicKey.y.Data_p = sfzutf_discard_const(Vector_p->PublicKeyY_p);
            ECCPointPublicKey.y.ByteDataSize = Vector_p->PublicKeyYLen;

            if (KEKAssetId != VAL_ASSETID_INVALID)
            {
                uint8_t * KeyBlob_p;
                ValSize_t KeyBlobSize = 0;

                Status = val_AsymEccLoadPublicKeyAssetPlaintext(&ECCPointPublicKey,
                                                                ModulusSizeBits,
                                                                PublicKeyAssetId,
                                                                KEKAssetId,
                                                                AssociatedData_p, AssociatedDataSize,
                                                                AssociatedData_p, &KeyBlobSize);
                fail_if(Status != VAL_BUFFER_TOO_SMALL, "val_AsymEccLoadPublicKeyAssetPlaintext()=", Status);

                KeyBlobSize = 0;
                Status = val_AsymEccLoadPublicKeyAssetPlaintext(&ECCPointPublicKey,
                                                                ModulusSizeBits,
                                                                PublicKeyAssetId,
                                                                KEKAssetId,
                                                                AssociatedData_p, AssociatedDataSize,
                                                                NULL, &KeyBlobSize);
                fail_if(Status != VAL_BUFFER_TOO_SMALL, "val_AsymEccLoadPublicKeyAssetPlaintext()=", Status);

                KeyBlob_p = (uint8_t *)SFZUTF_MALLOC(KeyBlobSize);
                fail_if(KeyBlob_p == NULL, "Allocation ", (int)KeyBlobSize);

                Status = val_AsymEccLoadPublicKeyAssetPlaintext(&ECCPointPublicKey,
                                                                ModulusSizeBits,
                                                                PublicKeyAssetId,
                                                                KEKAssetId,
                                                                AssociatedData_p, AssociatedDataSize,
                                                                KeyBlob_p, &KeyBlobSize);
                fail_if(Status != VAL_SUCCESS, "val_AsymEccLoadPublicKeyAssetPlaintext()=", Status);

                SFZUTF_FREE(KeyBlob_p);

            }
            else
            {
                /* Initialize the public key asset */
                Status = val_AsymEccLoadPublicKeyAssetPlaintext(&ECCPointPublicKey,
                                                                ModulusSizeBits,
                                                                PublicKeyAssetId,
                                                                VAL_ASSETID_INVALID,
                                                                NULL, 0, NULL, NULL);
                fail_if(Status != VAL_SUCCESS, "val_AsymEccLoadPublicKeyAssetPlaintext()=", Status);
            }

            /* Perform a public key check */
#ifdef VALTEST_DOKEYCHECK
            Status = val_AsymEccKeyCheck(&PublicKey, NULL);
            fail_if(Status != VAL_SUCCESS, "val_AsymEccKeyCheck()=", Status);
#endif
        }
    }

    if (KEKAssetId != VAL_ASSETID_INVALID)
    {
        // Release the involved Assets
        Status = val_AssetFree(KEKAssetId);
        fail_if(Status != VAL_SUCCESS, "val_AssetFree(KEK)=", Status);
    }

    /* Perform a full key pair check */
#ifdef VALTEST_DOKEYCHECK
    Status = val_AsymEccKeyCheck(&PublicKey, &PrivateKey);
    fail_if(Status != VAL_SUCCESS, "val_AsymEccKeyCheck()=", Status);

    Status = val_AsymEccKeyCheck(&PrivateKey, &PrivateKey);
    fail_if(Status != VAL_INVALID_ASSET, "val_AsymEccKeyCheck()=", Status);
#endif

#ifdef SFZUTF_USERMODE
    InCopy_p = sfzutf_discard_const(Vector_p->Message_p);
#else
    InCopy_p = (uint8_t *)SFZUTF_MALLOC(Vector_p->MessageLen);
    fail_if(InCopy_p == NULL, "Allocation ", (int)Vector_p->MessageLen);
    memcpy(InCopy_p, Vector_p->Message_p, Vector_p->MessageLen);
#endif

    if (fTestKeyPair)
    {
        ValSymContextPtr_t HashContext_p = NULL;
        uint8_t * Data_p = InCopy_p;
        ValSize_t DataLen = Vector_p->MessageLen;

        /* If success expected (test vector contains successful data),
           try test vector also via Sign-Verify. */
        Signature.r.ByteDataSize = VAL_ASYM_DATA_SIZE_B2B(ModulusSizeBits);
        Signature.s.ByteDataSize = VAL_ASYM_DATA_SIZE_B2B(ModulusSizeBits);

        Signature.r.Data_p = (uint8_t *)SFZUTF_MALLOC(Signature.r.ByteDataSize);
        Signature.s.Data_p = (uint8_t *)SFZUTF_MALLOC(Signature.s.ByteDataSize);
        fail_if((Signature.r.Data_p == NULL) || (Signature.s.Data_p == NULL),
                "Allocation ", (int)Signature.r.ByteDataSize);
        memset(Signature.r.Data_p, 0, Signature.r.ByteDataSize);
        memset(Signature.s.Data_p, 0, Signature.s.ByteDataSize);

        if (fUseTokenHashContext && (DataLen > 4096))
        {
            // Allocate hash context
            Status = val_SymAlloc(HashAlgorithm, VAL_SYM_MODE_NONE, true, &HashContext_p);
            fail_if(Status != VAL_SUCCESS, "val_SymAlloc()=", Status);

            // Hash a block of data
            Status = val_SymHashUpdate(HashContext_p, Data_p, 4096);
            fail_if(Status != VAL_SUCCESS, "val_SymAlloc()=", Status);

            // Adjust references
            Data_p  += 4096;
            DataLen -= 4096;
        }

        /* Wrong key */
        Status = val_AsymEcdsaSign(&PublicKey, &Signature,
                                   Data_p, DataLen, HashContext_p);
        fail_if(Status != VAL_INVALID_ASSET, "val_AsymEcdsaVerify()=", Status);

        Status = val_AsymEcdsaSign(&PrivateKey, &Signature,
                                   Data_p, DataLen, HashContext_p);
        fail_if(Status != VAL_SUCCESS, "val_AsymEcdsaSign()=", Status);
        fail_if(Signature.r.ByteDataSize != VAL_ASYM_DATA_SIZE_B2B(ModulusSizeBits),
                "Size mismatch ", VectorIndex);
        fail_if(Signature.s.ByteDataSize != VAL_ASYM_DATA_SIZE_B2B(ModulusSizeBits),
                "Size mismatch ", VectorIndex);

        if (fUseTokenHashContext && (DataLen > 4096))
        {
            // Restore hash context
            Status = val_SymAlloc(HashAlgorithm, VAL_SYM_MODE_NONE, true, &HashContext_p);
            fail_if(Status != VAL_SUCCESS, "val_SymAlloc()=", Status);

            // Hash a block of data
            Status = val_SymHashUpdate(HashContext_p, InCopy_p, 4096);
            fail_if(Status != VAL_SUCCESS, "val_SymAlloc()=", Status);
        }

        Signature.r.Data_p[1]++;
        Status = val_AsymEcdsaVerify(&PublicKey, &Signature,
                                     Data_p, DataLen, HashContext_p);
        fail_if(Status != VAL_VERIFY_ERROR, "val_AsymEcdsaVerify()=", Status);

        Signature.r.Data_p[1]--;
        Status = val_AsymEcdsaVerify(&PublicKey, &Signature,
                                     Data_p, DataLen, HashContext_p);
        fail_if(Status != VAL_SUCCESS, "val_AsymEcdsaVerify()=", Status);

        SFZUTF_FREE(Signature.r.Data_p);
        SFZUTF_FREE(Signature.s.Data_p);
    }
    else
    {
        /* Build expected result. */
        Signature.r.Data_p = sfzutf_discard_const(Vector_p->SignatureR_p);
        Signature.r.ByteDataSize = Vector_p->SignatureRLen;
        Signature.s.Data_p = sfzutf_discard_const(Vector_p->SignatureS_p);
        Signature.s.ByteDataSize = Vector_p->SignatureSLen;

        /* Wrong key */
        Status = val_AsymEcdsaVerify(&PrivateKey, &Signature,
                                     InCopy_p, Vector_p->MessageLen,
                                     NULL);
        fail_if(Status != VAL_INVALID_ASSET, "val_AsymEcdsaVerify()=", Status);

        /* Verify test vector. */
        Status = val_AsymEcdsaVerify(&PublicKey, &Signature,
                                     InCopy_p, Vector_p->MessageLen,
                                     NULL);
        fail_if(Status != VAL_SUCCESS, "val_AsymEcdsaVerify()=", Status);
    }

#ifndef SFZUTF_USERMODE
    SFZUTF_FREE(InCopy_p);
#endif

    SFZUTF_FREE(AssociatedData_p);

    /* Clean-up*/
    Status = val_AssetFree(PrivateKeyAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree()=", Status);

    Status = val_AssetFree(PublicKeyAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree()=", Status);

    Status = val_AssetFree(DomainAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree()=", Status);

    return END_TEST_SUCCES;
}


START_TEST (test_AsymEcdsaVectorsParamsCheck)
{
    int ndx;
    int Failed = 0;

    unsupported_unless(valtest_IsTrngActive(true), "TRNG not activated");

    for (ndx = 0; ; ndx++)
    {
        TestVector_ECDSA_t vector_p;

        vector_p = test_vectors_ecdsa_get(ndx);
        if (vector_p == NULL)
        {
            break;
        }

        if (vector_p->Curve_p->CurveBits >= VAL_ASYM_ECP_MIN_BITS)
        {
            if (DoEccEcdsaParamsTest(ndx, vector_p) != END_TEST_SUCCES)
            {
                LOG_CRIT("Process vector %d\n", ndx);
                Failed++;
            }
        }

        test_vectors_ecdsa_release(vector_p);
    }

    fail_if(Failed, "#wrong tests", Failed);
}
END_TEST


START_TEST (test_AsymEcdsaGenKeyPair)
{
    int ndx;
    int Failed = 0;

    unsupported_unless(valtest_IsTrngActive(true), "TRNG not activated");

    for (ndx = 0; ; ndx++)
    {
        TestVector_ECDSA_t vector_p;

        vector_p = test_vectors_ecdsa_get(ndx);
        if (vector_p == NULL)
        {
            break;
        }

        if ((vector_p->Curve_p->CurveBits >= VAL_ASYM_ECP_MIN_BITS) &&
            (vector_p->Curve_p->CurveBits >= vector_p->HashBits))
        {
            int rc = DoEccEcdsaGenKeyPairTest(ndx, vector_p);
            if ((rc != END_TEST_SUCCES) && (rc != END_TEST_UNSUPPORTED))
            {
                LOG_CRIT("Process vector %d\n", ndx);
                Failed++;
            }
        }

        test_vectors_ecdsa_release(vector_p);
    }

    fail_if(Failed, "#wrong tests", Failed);
}
END_TEST


START_TEST (test_AsymEcdsaVectorsCheck)
{
    int ndx;
    int Failed = 0;

    unsupported_unless(valtest_IsTrngActive(true), "TRNG not activated");

    for (ndx = 0; ; ndx++)
    {
        TestVector_ECDSA_t vector_p;

        vector_p = test_vectors_ecdsa_get(ndx);
        if (vector_p == NULL)
        {
            break;
        }

        if ((vector_p->Curve_p->CurveBits >= VAL_ASYM_ECP_MIN_BITS) &&
            (vector_p->Curve_p->CurveBits >= vector_p->HashBits))
        {
            if (DoEcdsaTest(ndx, vector_p, false, false, false, false) != END_TEST_SUCCES)
            {
                LOG_CRIT("Process vector %d\n", ndx);
                Failed++;
            }
        }

        test_vectors_ecdsa_release(vector_p);
    }

    fail_if(Failed, "#wrong tests", Failed);
}
END_TEST


START_TEST (test_AsymEcdsaVectorsKeyPairCheck)
{
    int ndx;
    int Failed = 0;

    unsupported_unless(valtest_IsTrngActive(true), "TRNG not activated");

    for (ndx = 0; ; ndx++)
    {
        TestVector_ECDSA_t vector_p;

        vector_p = test_vectors_ecdsa_get(ndx);
        if (vector_p == NULL)
        {
            break;
        }

        if ((vector_p->Curve_p->CurveBits >= VAL_ASYM_ECP_MIN_BITS) &&
            (vector_p->Curve_p->CurveBits >= vector_p->HashBits))
        {
            if (DoEcdsaTest(ndx, vector_p, true, false, false, false) != END_TEST_SUCCES)
            {
                LOG_CRIT("Process vector %d\n", ndx);
                Failed++;
            }
        }

        test_vectors_ecdsa_release(vector_p);
    }

    fail_if(Failed, "#wrong tests", Failed);
}
END_TEST


START_TEST (test_AsymEcdsaVectorsKeyPairGenCheck)
{
    int ndx;
    int Failed = 0;

    unsupported_unless(valtest_IsTrngActive(true), "TRNG not activated");

    for (ndx = 0; ; ndx++)
    {
        TestVector_ECDSA_t vector_p;

        vector_p = test_vectors_ecdsa_get(ndx);
        if (vector_p == NULL)
        {
            break;
        }

        if ((vector_p->Curve_p->CurveBits >= VAL_ASYM_ECP_MIN_BITS) &&
            (vector_p->Curve_p->CurveBits >= vector_p->HashBits))
        {
            if (DoEcdsaTest(ndx, vector_p, true, true, false, false) != END_TEST_SUCCES)
            {
                LOG_CRIT("Process vector %d\n", ndx);
                Failed++;
            }
        }

        test_vectors_ecdsa_release(vector_p);
    }

    fail_if(Failed, "#wrong tests", Failed);
}
END_TEST


START_TEST (test_AsymEcdsaVectorsPublicKeyGenCheck)
{
    int ndx;
    int Failed = 0;

    unsupported_unless(valtest_IsTrngActive(true), "TRNG not activated");

    for (ndx = 0; ; ndx++)
    {
        TestVector_ECDSA_t vector_p;

        vector_p = test_vectors_ecdsa_get(ndx);
        if (vector_p == NULL)
        {
            break;
        }

        if ((vector_p->Curve_p->CurveBits >= VAL_ASYM_ECP_MIN_BITS) &&
            (vector_p->Curve_p->CurveBits >= vector_p->HashBits))
        {
            if (DoEcdsaTest(ndx, vector_p, true, false, true, false) != END_TEST_SUCCES)
            {
                LOG_CRIT("Process vector %d\n", ndx);
                Failed++;
            }
        }

        test_vectors_ecdsa_release(vector_p);
    }

    fail_if(Failed, "#wrong tests", Failed);
}
END_TEST

START_TEST (test_AsymEcdsaLongMessageCheck)
{
    bool fUseTokenDigest = false;
    int ndx;
    int Failed = 0;

    unsupported_unless(valtest_IsTrngActive(true), "TRNG not activated");

    for (ndx = 0; ; ndx++)
    {
        TestVector_ECDSA_t vector_p;

        vector_p = test_vectors_ecdsa_get(ndx);
        if (vector_p == NULL)
        {
            break;
        }

        if ((vector_p->Curve_p->CurveBits >= VAL_ASYM_ECP_MIN_BITS) &&
            (vector_p->Curve_p->CurveBits >= vector_p->HashBits))
        {
            TestVector_ECDSA_Rec_t Vector;
            uint32_t MessageLen = 10 * 1024;
            uint8_t * Message_p = (uint8_t *)SFZUTF_MALLOC(MessageLen);
            fail_if(Message_p == NULL, "Allocation ", (int)MessageLen);

            memset(Message_p, 'a', MessageLen);
            Vector.Message_p = Message_p;
            Vector.MessageLen = MessageLen;
            Vector.Curve_p = vector_p->Curve_p;
            Vector.PrivateKey_p = vector_p->PrivateKey_p;
            Vector.PrivateKeyLen = vector_p->PrivateKeyLen;
            Vector.PublicKeyX_p = vector_p->PublicKeyX_p;
            Vector.PublicKeyXLen = vector_p->PublicKeyXLen;
            Vector.PublicKeyY_p = vector_p->PublicKeyY_p;
            Vector.PublicKeyYLen = vector_p->PublicKeyYLen;
            Vector.HashBits = vector_p->HashBits;
            Vector.SignatureR_p = NULL;
            Vector.SignatureRLen = 0;
            Vector.SignatureS_p = NULL;
            Vector.SignatureSLen = 0;

            if (DoEcdsaTest(ndx, &Vector, true, false, false, fUseTokenDigest) != END_TEST_SUCCES)
            {
                LOG_CRIT("Process vector %d\n", ndx);
                Failed++;
            }
            else if (fUseTokenDigest)
            {
                fUseTokenDigest = false;
            }
            else
            {
                fUseTokenDigest = true;
            }

            SFZUTF_FREE(Message_p);
        }

        test_vectors_ecdsa_release(vector_p);
    }

    fail_if(Failed, "#wrong tests", Failed);
}
END_TEST


START_TEST(test_AsymEcdsaArguments)
{
    uint8_t Buffer[4];
    ValSize_t Size = 32;
    ValAssetId_t AssetId = 0x5001;
    ValAsymKey_t Key;
    ValAsymSign_t Sign;
    ValStatus_t Status;

    // ECC part
    if (test_AsymEccArguments() != END_TEST_SUCCES)
    {
        return END_TEST_FAIL;
    }

    // ECDSA part
    // - val_AsymEcdsaAllocPrivateKeyAsset
    Status = val_AsymEcdsaAllocPrivateKeyAsset(VAL_ASYM_ECP_MIN_BITS-1, VAL_SYM_ALGO_HASH_SHA1, false, false, &AssetId);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaAllocPrivateKeyAsset(BadArgument1L)=", Status);

    Status = val_AsymEcdsaAllocPrivateKeyAsset(VAL_ASYM_ECP_MAX_BITS+1, VAL_SYM_ALGO_HASH_SHA1, false, false, &AssetId);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaAllocPrivateKeyAsset(BadArgument1H)=", Status);

    Status = val_AsymEcdsaAllocPrivateKeyAsset(VAL_ASYM_ECP_MIN_BITS, VAL_SYM_ALGO_HASH_SHA224, false, false, &AssetId);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaAllocPrivateKeyAsset(BadArgument2I)=", Status);

    Status = val_AsymEcdsaAllocPrivateKeyAsset(VAL_ASYM_ECP_MIN_BITS, VAL_SYM_ALGO_HASH_SHA256, false, false, &AssetId);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaAllocPrivateKeyAsset(BadArgument2I)=", Status);

#ifdef VALTEST_SYM_ALGO_SHA512
    Status = val_AsymEcdsaAllocPrivateKeyAsset(VAL_ASYM_ECP_MIN_BITS, VAL_SYM_ALGO_HASH_SHA384, false, false, &AssetId);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaAllocPrivateKeyAsset(BadArgument2I)=", Status);

    Status = val_AsymEcdsaAllocPrivateKeyAsset(VAL_ASYM_ECP_MIN_BITS, VAL_SYM_ALGO_HASH_SHA512, false, false, &AssetId);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaAllocPrivateKeyAsset(BadArgument2I)=", Status);
#endif

    Status = val_AsymEcdsaAllocPrivateKeyAsset(VAL_ASYM_ECP_MIN_BITS, VAL_SYM_ALGO_HASH_SHA1, false, false, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaAllocPrivateKeyAsset(BadArgument3)=", Status);

    // - val_AsymEcdsaAllocPublicKeyAsset
    Status = val_AsymEcdsaAllocPublicKeyAsset(VAL_ASYM_ECP_MIN_BITS-1, VAL_SYM_ALGO_HASH_SHA1, false, false, &AssetId);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaAllocPublicKeyAsset(BadArgument1L)=", Status);

    Status = val_AsymEcdsaAllocPublicKeyAsset(VAL_ASYM_ECP_MAX_BITS+1, VAL_SYM_ALGO_HASH_SHA1, false, false, &AssetId);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaAllocPublicKeyAsset(BadArgument1H)=", Status);

    Status = val_AsymEcdsaAllocPublicKeyAsset(VAL_ASYM_ECP_MIN_BITS, VAL_SYM_ALGO_HASH_SHA224, false, false, &AssetId);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaAllocPublicKeyAsset(BadArgument2I)=", Status);

    Status = val_AsymEcdsaAllocPublicKeyAsset(VAL_ASYM_ECP_MIN_BITS, VAL_SYM_ALGO_HASH_SHA256, false, false, &AssetId);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaAllocPublicKeyAsset(BadArgument2I)=", Status);

#ifdef VALTEST_SYM_ALGO_SHA512
    Status = val_AsymEcdsaAllocPublicKeyAsset(VAL_ASYM_ECP_MIN_BITS, VAL_SYM_ALGO_HASH_SHA384, false, false, &AssetId);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaAllocPublicKeyAsset(BadArgument2I)=", Status);

    Status = val_AsymEcdsaAllocPublicKeyAsset(VAL_ASYM_ECP_MIN_BITS, VAL_SYM_ALGO_HASH_SHA512, false, false, &AssetId);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaAllocPublicKeyAsset(BadArgument2I)=", Status);
#endif

    Status = val_AsymEcdsaAllocPublicKeyAsset(VAL_ASYM_ECP_MIN_BITS, VAL_SYM_ALGO_HASH_SHA1, false, false, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaAllocPublicKeyAsset(BadArgument3)=", Status);

    // - val_AsymEcdsaSign/val_AsymEcdsaVerify
    Sign.r.Data_p = Buffer;
    Sign.r.ByteDataSize = VAL_ASYM_DATA_SIZE_B2B(VAL_ASYM_ECP_MIN_BITS);
    Sign.s.Data_p = Buffer;
    Sign.s.ByteDataSize = VAL_ASYM_DATA_SIZE_B2B(VAL_ASYM_ECP_MIN_BITS);

    Status = val_AsymEcdsaSign(NULL, &Sign, Buffer, Size, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaSign(BadArgument1)=", Status);
    Status = val_AsymEcdsaVerify(NULL, &Sign, Buffer, Size, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaVerify(BadArgument1)=", Status);

    Key.KeyAssetId = VAL_ASSETID_INVALID;
    Key.DomainAssetId = AssetId;
    Key.ModulusSizeBits = VAL_ASYM_ECP_MIN_BITS;
    Key.HashAlgorithm = VAL_SYM_ALGO_HASH_SHA1;

    Status = val_AsymEcdsaSign(&Key, &Sign, Buffer, Size, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaSign(BadArgument1I)=", Status);
    Status = val_AsymEcdsaVerify(&Key, &Sign, Buffer, Size, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaVerify(BadArgument1I)=", Status);
    Key.KeyAssetId = AssetId;

    Key.DomainAssetId = VAL_ASSETID_INVALID;
    Status = val_AsymEcdsaSign(&Key, &Sign, Buffer, Size, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaSign(BadArgument1I)=", Status);
    Status = val_AsymEcdsaVerify(&Key, &Sign, Buffer, Size, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaVerify(BadArgument1I)=", Status);
    Key.DomainAssetId = AssetId;

    Key.ModulusSizeBits = VAL_ASYM_ECP_MIN_BITS-1;
    Status = val_AsymEcdsaSign(&Key, &Sign, Buffer, Size, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaSign(BadArgument1I)=", Status);
    Status = val_AsymEcdsaVerify(&Key, &Sign, Buffer, Size, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaVerify(BadArgument1I)=", Status);

    Key.ModulusSizeBits = VAL_ASYM_ECP_MAX_BITS+1;
    Status = val_AsymEcdsaSign(&Key, &Sign, Buffer, Size, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaSign(BadArgument1I)=", Status);
    Status = val_AsymEcdsaVerify(&Key, &Sign, Buffer, Size, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaVerify(BadArgument1I)=", Status);
    Key.ModulusSizeBits = VAL_ASYM_ECP_MIN_BITS;

    Key.HashAlgorithm = VAL_SYM_ALGO_HASH_MAX;
    Status = val_AsymEcdsaSign(&Key, &Sign, Buffer, Size, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaSign(BadArgument1I)=", Status);
    Status = val_AsymEcdsaVerify(&Key, &Sign, Buffer, Size, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaVerify(BadArgument1I)=", Status);

    Size = 8096;
    Status = val_AsymEcdsaSign(&Key, &Sign, Buffer, Size, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaSign(BadArgument1I)=", Status);
    Status = val_AsymEcdsaVerify(&Key, &Sign, Buffer, Size, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaVerify(BadArgument1I)=", Status);
    Size = 32;

    Key.HashAlgorithm = VAL_SYM_ALGO_HASH_SHA224;
    Status = val_AsymEcdsaSign(&Key, &Sign, Buffer, Size, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaSign(BadArgument1I)=", Status);
    Status = val_AsymEcdsaVerify(&Key, &Sign, Buffer, Size, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaVerify(BadArgument1I)=", Status);

    Key.HashAlgorithm = VAL_SYM_ALGO_HASH_SHA256;
    Status = val_AsymEcdsaSign(&Key, &Sign, Buffer, Size, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaSign(BadArgument1I)=", Status);
    Status = val_AsymEcdsaVerify(&Key, &Sign, Buffer, Size, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaVerify(BadArgument1I)=", Status);

#ifdef VALTEST_SYM_ALGO_SHA512
    Key.HashAlgorithm = VAL_SYM_ALGO_HASH_SHA384;
    Status = val_AsymEcdsaSign(&Key, &Sign, Buffer, Size, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaSign(BadArgument1I)=", Status);
    Status = val_AsymEcdsaVerify(&Key, &Sign, Buffer, Size, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaVerify(BadArgument1I)=", Status);

    Key.HashAlgorithm = VAL_SYM_ALGO_HASH_SHA512;
    Status = val_AsymEcdsaSign(&Key, &Sign, Buffer, Size, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaSign(BadArgument1I)=", Status);
    Status = val_AsymEcdsaVerify(&Key, &Sign, Buffer, Size, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaVerify(BadArgument1I)=", Status);
#endif

    Key.HashAlgorithm = VAL_SYM_ALGO_HASH_SHA1;

    Status = val_AsymEcdsaSign(&Key, NULL, Buffer, Size, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaSign(BadArgument2)=", Status);
    Status = val_AsymEcdsaVerify(&Key, NULL, Buffer, Size, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaVerify(BadArgument2)=", Status);

    Sign.r.Data_p = NULL;
    Status = val_AsymEcdsaSign(&Key, &Sign, Buffer, Size, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaSign(BadArgument2I)=", Status);
    Status = val_AsymEcdsaVerify(&Key, &Sign, Buffer, Size, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaVerify(BadArgument2I)=", Status);
    Sign.r.Data_p = Buffer;

    Sign.r.ByteDataSize = VAL_ASYM_DATA_SIZE_B2B(VAL_ASYM_ECP_MIN_BITS) - 1;
    Status = val_AsymEcdsaSign(&Key, &Sign, Buffer, Size, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaSign(BadArgument2I)=", Status);
    Status = val_AsymEcdsaVerify(&Key, &Sign, Buffer, Size, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaVerify(BadArgument2I)=", Status);
    Sign.r.ByteDataSize = VAL_ASYM_DATA_SIZE_B2B(VAL_ASYM_ECP_MIN_BITS);

    Sign.s.Data_p = NULL;
    Status = val_AsymEcdsaSign(&Key, &Sign, Buffer, Size, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaSign(BadArgument2I)=", Status);
    Status = val_AsymEcdsaVerify(&Key, &Sign, Buffer, Size, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaVerify(BadArgument2I)=", Status);
    Sign.s.Data_p = Buffer;

    Sign.s.ByteDataSize = VAL_ASYM_DATA_SIZE_B2B(VAL_ASYM_ECP_MIN_BITS) - 1;
    Status = val_AsymEcdsaSign(&Key, &Sign, Buffer, Size, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaSign(BadArgument2I)=", Status);
    Status = val_AsymEcdsaVerify(&Key, &Sign, Buffer, Size, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaVerify(BadArgument2I)=", Status);
    Sign.s.ByteDataSize = VAL_ASYM_DATA_SIZE_B2B(VAL_ASYM_ECP_MIN_BITS);

    Status = val_AsymEcdsaSign(&Key, &Sign, NULL, Size, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaSign(BadArgument2)=", Status);
    Status = val_AsymEcdsaVerify(&Key, &Sign, NULL, Size, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaVerify(BadArgument2)=", Status);

    Status = val_AsymEcdsaSign(&Key, &Sign, Buffer, 0, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaSign(BadArgument4L)=", Status);
    Status = val_AsymEcdsaVerify(&Key, &Sign, Buffer, 0, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEcdsaVerify(BadArgument4L)=", Status);
}
END_TEST

START_TEST(test_AsymEcdsaVectorsInitKey)
{
    ValAssetId_t AssetId = 0x5001;
    ValAsymKey_t Key;
    ValStatus_t Status;

    // General function
    // - val_AsymInitKey
    Status = val_AsymInitKey(AssetId, AssetId, VAL_ASYM_ECP_MIN_BITS, VAL_SYM_ALGO_HASH, &Key);
    fail_if(Status != VAL_INVALID_ALGORITHM, "val_AsymInitKey(InvalidAlgorithm4)=", Status);

    return END_TEST_SUCCES;
}
END_TEST


int
suite_add_test_AsymEcdsa(
        struct TestSuite * TestSuite_p)
{
    struct TestCase * TestCase_p;

    TestCase_p = sfzutf_tcase_create(TestSuite_p, "AsymCrypto_ECDSA_Vectors");
    if (TestCase_p != NULL)
    {
        if (sfzutf_tcase_add_fixture(TestCase_p, valtest_initialize, valtest_terminate) != 0)
        {
             goto FuncErrorReturn;
        }

        if (sfzutf_test_add(TestCase_p, test_AsymEcdsaVectorsParamsCheck) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add(TestCase_p, test_AsymEcdsaGenKeyPair) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add(TestCase_p, test_AsymEcdsaVectorsCheck) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add(TestCase_p, test_AsymEcdsaVectorsKeyPairCheck) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add(TestCase_p, test_AsymEcdsaVectorsKeyPairGenCheck) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add(TestCase_p, test_AsymEcdsaVectorsPublicKeyGenCheck) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add(TestCase_p, test_AsymEcdsaLongMessageCheck) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add(TestCase_p, test_AsymEcdsaVectorsInitKey) != 0) goto FuncErrorReturn;
        if (valtest_StrictArgsCheck())
        {
            if (sfzutf_test_add(TestCase_p, test_AsymEcdsaArguments) != 0) goto FuncErrorReturn;
        }
        return 0;
    }

FuncErrorReturn:
    return -1;
}

/* end of file valtest_asym_ecdsa.c */
