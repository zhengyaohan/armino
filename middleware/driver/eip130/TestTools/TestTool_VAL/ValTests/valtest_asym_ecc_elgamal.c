/* valtest_asym_ecc_elgamal.c
 *
 * Description: ECC El-Gamal Test Vectors
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
#include "testvectors_ecc_elgamal.h"


/* Test helper function. */
int
test_AsymEccArguments(void);


int
test_AsymEccLoadDomain(
        const TestVector_ECC_Curve_Rec_t * Curve_p,
        ValAssetId_t * DomainAssetId_p,
        bool ftestExport);


static int
DoEccElGmalParamsTest(
        int VectorIndex,
        TestVector_ECC_ElGamal_t Vector_p)
{
    int rc;
    ValStatus_t Status;
    ValAsymKey_t PrivateKey;
    ValAsymKey_t PublicKey;
    ValAssetId_t DomainAssetId;

    IDENTIFIER_NOT_USED(VectorIndex);

    /* Load domain parameters */
    rc = test_AsymEccLoadDomain(Vector_p->Curve_p, &DomainAssetId, true);
    if (rc != END_TEST_SUCCES)
    {
        return rc;
    }

    /* Dummy key entries */
    Status = val_AsymInitKey(VAL_ASSETID_INVALID, DomainAssetId,
                             Vector_p->Curve_p->CurveBits, VAL_SYM_ALGO_NONE,
                             &PrivateKey);
    fail_if(Status != VAL_SUCCESS, "val_AsymInitKey()=", Status);

    Status = val_AsymInitKey(VAL_ASSETID_INVALID, DomainAssetId,
                             Vector_p->Curve_p->CurveBits, VAL_SYM_ALGO_NONE,
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
                              (VAL_ASYM_ECP_MIN_BITS - 1), VAL_SYM_ALGO_NONE,
                              &PublicKey);
        Status = val_AsymEccKeyCheck(&PublicKey, &PrivateKey);
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccKeyCheck(BadArgument1I)=", Status);
        Status = val_AsymEccKeyCheck(&PrivateKey, &PublicKey);
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccKeyCheck(BadArgument2I)=", Status);

        (void)val_AsymInitKey(VAL_ASSETID_INVALID, DomainAssetId,
                              (VAL_ASYM_ECP_MAX_BITS + 1), VAL_SYM_ALGO_NONE,
                              &PublicKey);
        Status = val_AsymEccKeyCheck(&PublicKey, &PrivateKey);
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccKeyCheck(BadArgument1I)=", Status);
        Status = val_AsymEccKeyCheck(&PrivateKey, &PublicKey);
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccKeyCheck(BadArgument2I)=", Status);

        (void)val_AsymInitKey(VAL_ASSETID_INVALID, VAL_ASSETID_INVALID,
                              VAL_ASYM_ECP_MIN_BITS, VAL_SYM_ALGO_NONE,
                              &PublicKey);
        Status = val_AsymEccKeyCheck(&PublicKey, &PrivateKey);
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccKeyCheck(BadArgument1I)=", Status);
        Status = val_AsymEccKeyCheck(&PrivateKey, &PublicKey);
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccKeyCheck(BadArgument2I)=", Status);
    }

    return END_TEST_SUCCES;
}


static int
DoEccElGmalTest(
        int VectorIndex,
        TestVector_ECC_ElGamal_t Vector_p,
        bool fTestKeyPair,
        bool fGenerateKeyPair,
        bool fGeneratePublicKey,
        ValStatus_t ExpectedStatus)
{
    int rc;
    ValStatus_t Status;
    ValAsymKey_t PrivateKey;
    ValAsymKey_t PublicKey;
    ValAssetId_t DomainAssetId;
    ValAssetId_t PrivateKeyAssetId;
    ValAssetId_t PublicKeyAssetId;
    ValAsymECCPoint_t MessagePoint;
    ValAsymECCPoint_t EncryptedPoint1;
    ValAsymECCPoint_t EncryptedPoint2;
    uint32_t ModulusSizeBits;

    unsupported_unless(valtest_IsTrngActive(true), "TRNG not activated");

    /* Load domain parameters */
    rc = test_AsymEccLoadDomain(Vector_p->Curve_p, &DomainAssetId, false);
    if (rc != END_TEST_SUCCES)
    {
        return rc;
    }
    ModulusSizeBits = Vector_p->Curve_p->CurveBits;

    /* Allocate the private key asset and initialize key structure */
    Status = val_AsymEccElGamalAllocPrivateKeyAsset(ModulusSizeBits, false, false, &PrivateKeyAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AsymEccElGamalAllocPrivateKeyAsset()=", Status);

    Status = val_AsymInitKey(PrivateKeyAssetId, DomainAssetId,
                             ModulusSizeBits, VAL_SYM_ALGO_NONE,
                             &PrivateKey);
    fail_if(Status != VAL_SUCCESS, "val_AsymInitKey()=", Status);

    /* Allocate the public key asset and initialize key structure */
    Status = val_AsymEccElGamalAllocPublicKeyAsset(ModulusSizeBits, false, false, &PublicKeyAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AsymEccElGamalAllocPublicKeyAsset()=", Status);

    Status = val_AsymInitKey(PublicKeyAssetId, DomainAssetId,
                             ModulusSizeBits, VAL_SYM_ALGO_NONE,
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
        Status = val_AsymEccLoadPrivateKeyAssetPlaintext(&BigIntPrivateKey,
                                                         ModulusSizeBits,
                                                         PrivateKeyAssetId,
                                                         VAL_ASSETID_INVALID,
                                                         NULL, 0, NULL, NULL);
        fail_if(Status != VAL_SUCCESS,
                "val_AsymEccLoadPrivateKeyAssetPlaintext()=", Status);

        /* Perform a private key check */
#ifdef VALTEST_DOKEYCHECK
        Status = val_AsymEccKeyCheck(NULL, &PrivateKey);
        fail_if(Status != VAL_SUCCESS, "val_AsymEccKeyCheck()=", Status);
#endif

        if (fGeneratePublicKey)
        {
            /* Generate a public key */
            Status = val_AsymEccGenPublicKey(&PublicKey, &PrivateKey, NULL);
            fail_if(Status != VAL_SUCCESS, "val_AsymEccGenPublicKey()=", Status);

            fTestKeyPair = true;            // Force key pair check
        }
        else
        {
            /* Initialize the public key asset */
            ECCPointPublicKey.x.Data_p = sfzutf_discard_const(Vector_p->PublicKeyX_p);
            ECCPointPublicKey.x.ByteDataSize = Vector_p->PublicKeyXLen;
            ECCPointPublicKey.y.Data_p = sfzutf_discard_const(Vector_p->PublicKeyY_p);
            ECCPointPublicKey.y.ByteDataSize = Vector_p->PublicKeyYLen;
            Status = val_AsymEccLoadPublicKeyAssetPlaintext(&ECCPointPublicKey,
                                                            ModulusSizeBits,
                                                            PublicKeyAssetId,
                                                            VAL_ASSETID_INVALID,
                                                            NULL, 0, NULL, NULL);
            fail_if(Status != VAL_SUCCESS,
                    "val_AsymEccLoadPublicKeyAssetPlaintext()=", Status);

            /* Perform a public key check */
#ifdef VALTEST_DOKEYCHECK
            Status = val_AsymEccKeyCheck(&PublicKey, NULL);
            fail_if(Status != VAL_SUCCESS, "val_AsymEccKeyCheck()=", Status);
#endif
        }
    }

    /* Perform a full key check */
#ifdef VALTEST_DOKEYCHECK
    Status = val_AsymEccKeyCheck(&PublicKey, &PrivateKey);
    fail_if(Status != VAL_SUCCESS, "val_AsymEccKeyCheck()=", Status);

    Status = val_AsymEccKeyCheck(&PublicKey, &PublicKey);
    fail_if(Status != VAL_INVALID_ASSET, "val_AsymEccKeyCheck()=", Status);
#endif

    if (fTestKeyPair)
    {
        fail_if(Vector_p->MessageXLen > Vector_p->Curve_p->OrderLen,
                "Size mismatch ", VectorIndex);
        fail_if(Vector_p->MessageYLen > Vector_p->Curve_p->OrderLen,
                "Size mismatch ", VectorIndex);

        MessagePoint.x.Data_p = (uint8_t *)SFZUTF_MALLOC(Vector_p->Curve_p->OrderLen);
        MessagePoint.x.ByteDataSize = Vector_p->MessageXLen;
        MessagePoint.y.Data_p = (uint8_t *)SFZUTF_MALLOC(Vector_p->Curve_p->OrderLen);
        MessagePoint.y.ByteDataSize = Vector_p->MessageYLen;
        fail_if((MessagePoint.x.Data_p == NULL) ||
                (MessagePoint.y.Data_p == NULL),
                "Allocation ", (int)Vector_p->Curve_p->OrderLen);
        memcpy(MessagePoint.x.Data_p, Vector_p->MessageX_p, Vector_p->MessageXLen);
        memcpy(MessagePoint.y.Data_p, Vector_p->MessageY_p, Vector_p->MessageYLen);

        EncryptedPoint1.x.Data_p = (uint8_t *)SFZUTF_MALLOC(Vector_p->Curve_p->OrderLen);
        EncryptedPoint1.x.ByteDataSize = Vector_p->Curve_p->OrderLen;
        EncryptedPoint1.y.Data_p = (uint8_t *)SFZUTF_MALLOC(Vector_p->Curve_p->OrderLen);
        EncryptedPoint1.y.ByteDataSize = Vector_p->Curve_p->OrderLen;
        fail_if((EncryptedPoint1.x.Data_p == NULL) ||
                (EncryptedPoint1.y.Data_p == NULL),
                "Allocation ", (int)Vector_p->Curve_p->OrderLen);
        memset(EncryptedPoint1.x.Data_p, 0, EncryptedPoint1.x.ByteDataSize);
        memset(EncryptedPoint1.y.Data_p, 0, EncryptedPoint1.y.ByteDataSize);

        EncryptedPoint2.x.Data_p = (uint8_t *)SFZUTF_MALLOC(Vector_p->Curve_p->OrderLen);
        EncryptedPoint2.x.ByteDataSize = Vector_p->Curve_p->OrderLen;
        EncryptedPoint2.y.Data_p = (uint8_t *)SFZUTF_MALLOC(Vector_p->Curve_p->OrderLen);
        EncryptedPoint2.y.ByteDataSize = Vector_p->Curve_p->OrderLen;
        fail_if((EncryptedPoint2.x.Data_p == NULL) ||
                (EncryptedPoint2.y.Data_p == NULL),
                "Allocation ", (int)Vector_p->Curve_p->OrderLen);
        memset(EncryptedPoint2.x.Data_p, 0, EncryptedPoint2.x.ByteDataSize);
        memset(EncryptedPoint2.y.Data_p, 0, EncryptedPoint2.y.ByteDataSize);

        Status = val_AsymEccElGamalEncrypt(&PublicKey, &MessagePoint,
                                           &EncryptedPoint1, &EncryptedPoint2);
        fail_if(Status != VAL_SUCCESS, "val_AsymEccElGamalEncrypt()=", Status);
        fail_if(EncryptedPoint1.x.ByteDataSize > Vector_p->Curve_p->OrderLen,
                "Size mismatch ", VectorIndex);
        fail_if(EncryptedPoint1.y.ByteDataSize > Vector_p->Curve_p->OrderLen,
                "Size mismatch ", VectorIndex);
        fail_if(EncryptedPoint2.x.ByteDataSize > Vector_p->Curve_p->OrderLen,
                "Size mismatch ", VectorIndex);
        fail_if(EncryptedPoint2.y.ByteDataSize > Vector_p->Curve_p->OrderLen,
                "Size mismatch ", VectorIndex);

        Status = val_AsymEccElGamalDecrypt(&PrivateKey,
                                           &EncryptedPoint1, &EncryptedPoint2,
                                           &MessagePoint);
        fail_if(Status != ExpectedStatus, "val_AsymEccElGamalDecrypt()=", Status);
        fail_if(MessagePoint.x.ByteDataSize != Vector_p->MessageXLen,
                "Size mismatch ", VectorIndex);
        fail_if(MessagePoint.y.ByteDataSize != Vector_p->MessageYLen,
                "Size mismatch ", VectorIndex);
        fail_if(memcmp(MessagePoint.x.Data_p, Vector_p->MessageX_p, Vector_p->MessageXLen),
                "Data mismatch ", VectorIndex);

        // Coverage improvement
        Status = val_AsymEccElGamalEncrypt(&PrivateKey, &MessagePoint,
                                           &EncryptedPoint1, &EncryptedPoint2);
        fail_if(Status != VAL_INVALID_ASSET, "val_AsymEccElGamalEncrypt(Wrong Key)=", Status);

        Status = val_AsymEccElGamalDecrypt(&PublicKey,
                                           &EncryptedPoint1, &EncryptedPoint2,
                                           &MessagePoint);
        fail_if(Status != VAL_INVALID_ASSET, "val_AsymEccElGamalDecrypt(Wrong Key)=", Status);

        SFZUTF_FREE(EncryptedPoint2.x.Data_p);
        SFZUTF_FREE(EncryptedPoint2.y.Data_p);
        SFZUTF_FREE(EncryptedPoint1.x.Data_p);
        SFZUTF_FREE(EncryptedPoint1.y.Data_p);
        SFZUTF_FREE(MessagePoint.x.Data_p);
        SFZUTF_FREE(MessagePoint.y.Data_p);
    }
    else
    {
        fail_if(Vector_p->CipherTextC_XLen > Vector_p->Curve_p->OrderLen,
                "Size mismatch ", VectorIndex);
        fail_if(Vector_p->CipherTextC_YLen > Vector_p->Curve_p->OrderLen,
                "Size mismatch ", VectorIndex);
        fail_if(Vector_p->CipherTextD_XLen > Vector_p->Curve_p->OrderLen,
                "Size mismatch ", VectorIndex);
        fail_if(Vector_p->CipherTextD_YLen > Vector_p->Curve_p->OrderLen,
                "Size mismatch ", VectorIndex);

        EncryptedPoint1.x.Data_p = (uint8_t *)SFZUTF_MALLOC(Vector_p->Curve_p->OrderLen);
        EncryptedPoint1.x.ByteDataSize = Vector_p->Curve_p->OrderLen;
        EncryptedPoint1.y.Data_p = (uint8_t *)SFZUTF_MALLOC(Vector_p->Curve_p->OrderLen);
        EncryptedPoint1.y.ByteDataSize = Vector_p->Curve_p->OrderLen;
        fail_if((EncryptedPoint1.x.Data_p == NULL) ||
                (EncryptedPoint1.y.Data_p == NULL),
                "Allocation ", (int)Vector_p->Curve_p->OrderLen);
        memcpy(EncryptedPoint1.x.Data_p, Vector_p->CipherTextC_X_p, Vector_p->CipherTextC_XLen);
        memcpy(EncryptedPoint1.y.Data_p, Vector_p->CipherTextC_Y_p, Vector_p->CipherTextC_YLen);

        EncryptedPoint2.x.Data_p = (uint8_t *)SFZUTF_MALLOC(Vector_p->Curve_p->OrderLen);
        EncryptedPoint2.x.ByteDataSize = Vector_p->Curve_p->OrderLen;
        EncryptedPoint2.y.Data_p = (uint8_t *)SFZUTF_MALLOC(Vector_p->Curve_p->OrderLen);
        EncryptedPoint2.y.ByteDataSize = Vector_p->Curve_p->OrderLen;
        fail_if((EncryptedPoint2.x.Data_p == NULL) ||
                (EncryptedPoint2.y.Data_p == NULL),
                "Allocation ", (int)Vector_p->Curve_p->OrderLen);
        memcpy(EncryptedPoint2.x.Data_p, Vector_p->CipherTextD_X_p, Vector_p->CipherTextD_XLen);
        memcpy(EncryptedPoint2.y.Data_p, Vector_p->CipherTextD_Y_p, Vector_p->CipherTextD_YLen);

        MessagePoint.x.Data_p = (uint8_t *)SFZUTF_MALLOC(Vector_p->Curve_p->OrderLen);
        MessagePoint.x.ByteDataSize = Vector_p->MessageXLen;
        MessagePoint.y.Data_p = (uint8_t *)SFZUTF_MALLOC(Vector_p->Curve_p->OrderLen);
        MessagePoint.y.ByteDataSize = Vector_p->MessageYLen;
        fail_if((MessagePoint.x.Data_p == NULL) ||
                (MessagePoint.y.Data_p == NULL),
                "Allocation ", (int)Vector_p->Curve_p->OrderLen);
        memset(MessagePoint.x.Data_p, 0, MessagePoint.x.ByteDataSize);
        memset(MessagePoint.y.Data_p, 0, MessagePoint.y.ByteDataSize);

        Status = val_AsymEccElGamalDecrypt(&PrivateKey,
                                           &EncryptedPoint1, &EncryptedPoint2,
                                           &MessagePoint);
        fail_if(Status != ExpectedStatus, "val_AsymEccElGamalDecrypt()=", Status);
        fail_if(MessagePoint.x.ByteDataSize != Vector_p->MessageXLen,
                "Size mismatch ", VectorIndex);
        fail_if(MessagePoint.y.ByteDataSize != Vector_p->MessageYLen,
                "Size mismatch ", VectorIndex);
        fail_if(memcmp(MessagePoint.x.Data_p, Vector_p->MessageX_p, Vector_p->MessageXLen),
                "Data mismatch ", VectorIndex);

        SFZUTF_FREE(MessagePoint.x.Data_p);
        SFZUTF_FREE(MessagePoint.y.Data_p);
        SFZUTF_FREE(EncryptedPoint2.x.Data_p);
        SFZUTF_FREE(EncryptedPoint2.y.Data_p);
        SFZUTF_FREE(EncryptedPoint1.x.Data_p);
        SFZUTF_FREE(EncryptedPoint1.y.Data_p);
    }

    /* Clean-up*/
    Status = val_AssetFree(PrivateKeyAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree()=", Status);

    Status = val_AssetFree(PublicKeyAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree()=", Status);

    Status = val_AssetFree(DomainAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree()=", Status);

    return END_TEST_SUCCES;
}


START_TEST (test_AsymEccElgamalVectorsParamsCheck)
{
    int ndx;
    int Failed = 0;

    unsupported_unless(valtest_IsTrngActive(true), "TRNG not activated");

    for (ndx = 0; ; ndx++)
    {
        TestVector_ECC_ElGamal_t vector_p;

        vector_p = test_vectors_ecc_elgamal_get(ndx);
        if (vector_p == NULL)
        {
            break;
        }

        if (DoEccElGmalParamsTest(ndx, vector_p) != END_TEST_SUCCES)
        {
            LOG_CRIT("Process vector %d\n", ndx);
            Failed++;
        }

        test_vectors_ecc_elgamal_release(vector_p);
    }

    fail_if(Failed, "#wrong tests", Failed);
}
END_TEST


START_TEST (test_AsymEccElgamalVectorsCheck)
{
    int ndx;
    int Failed = 0;

    unsupported_unless(valtest_IsTrngActive(true), "TRNG not activated");

    for (ndx = 0; ; ndx++)
    {
        TestVector_ECC_ElGamal_t vector_p;

        vector_p = test_vectors_ecc_elgamal_get(ndx);
        if (vector_p == NULL)
        {
            break;
        }

        if (DoEccElGmalTest(ndx, vector_p, false, false, false, VAL_SUCCESS) != END_TEST_SUCCES)
        {
            LOG_CRIT("Process vector %d\n", ndx);
            Failed++;
        }

        test_vectors_ecc_elgamal_release(vector_p);
    }

    fail_if(Failed, "#wrong tests", Failed);
}
END_TEST


START_TEST (test_AsymEccElgamalVectorsKeyPairCheck)
{
    int ndx;
    int Failed = 0;

    unsupported_unless(valtest_IsTrngActive(true), "TRNG not activated");

    for (ndx = 0; ; ndx++)
    {
        TestVector_ECC_ElGamal_t vector_p;

        vector_p = test_vectors_ecc_elgamal_get(ndx);
        if (vector_p == NULL)
        {
            break;
        }

        if (DoEccElGmalTest(ndx, vector_p, true, false, false, VAL_SUCCESS) != END_TEST_SUCCES)
        {
            LOG_CRIT("Process vector %d\n", ndx);
            Failed++;
        }

        test_vectors_ecc_elgamal_release(vector_p);
    }

    fail_if(Failed, "#wrong tests", Failed);
}
END_TEST


START_TEST (test_AsymEccElgamalVectorsKeyPairGenCheck)
{
    int ndx;
    int Failed = 0;

    unsupported_unless(valtest_IsTrngActive(true), "TRNG not activated");

    for (ndx = 0; ; ndx++)
    {
        TestVector_ECC_ElGamal_t vector_p;

        vector_p = test_vectors_ecc_elgamal_get(ndx);
        if (vector_p == NULL)
        {
            break;
        }

        if (DoEccElGmalTest(ndx, vector_p, true, true, false, VAL_SUCCESS) != END_TEST_SUCCES)
        {
            LOG_CRIT("Process vector %d\n", ndx);
            Failed++;
        }

        test_vectors_ecc_elgamal_release(vector_p);
    }

    fail_if(Failed, "#wrong tests", Failed);
}
END_TEST


START_TEST (test_AsymEccElgamalVectorsPublicKeyGenCheck)
{
    int ndx;
    int Failed = 0;

    unsupported_unless(valtest_IsTrngActive(true), "TRNG not activated");

    for (ndx = 0; ; ndx++)
    {
        TestVector_ECC_ElGamal_t vector_p;

        vector_p = test_vectors_ecc_elgamal_get(ndx);
        if (vector_p == NULL)
        {
            break;
        }

        if (DoEccElGmalTest(ndx, vector_p, true, false, true, VAL_SUCCESS) != END_TEST_SUCCES)
        {
            LOG_CRIT("Process vector %d\n", ndx);
            Failed++;
        }

        test_vectors_ecc_elgamal_release(vector_p);
    }

    fail_if(Failed, "#wrong tests", Failed);
}
END_TEST


START_TEST(test_AsymEccElgamalArguments)
{
    uint8_t Buffer[4];
    ValAssetId_t AssetId = 0x5001;
    ValAsymKey_t Key;
    ValAsymECCPoint_t Point;
    ValAsymECCPoint_t ErrorPoint;
    ValStatus_t Status;

    // ECC part
    if (test_AsymEccArguments() != END_TEST_SUCCES)
    {
        return END_TEST_FAIL;
    }

    // ECC El-Gamal part
    // - val_AsymEccElGamalAllocPrivateKeyAsset
    Status = val_AsymEccElGamalAllocPrivateKeyAsset(VAL_ASYM_ECP_MIN_BITS-1, false, false, &AssetId);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalAllocPrivateKeyAsset(BadArgument1L)=", Status);

    Status = val_AsymEccElGamalAllocPrivateKeyAsset(VAL_ASYM_ECP_MAX_BITS+1, false, false, &AssetId);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalAllocPrivateKeyAsset(BadArgument1H)=", Status);

    Status = val_AsymEccElGamalAllocPrivateKeyAsset(VAL_ASYM_ECP_MIN_BITS, false, false, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalAllocPrivateKeyAsset(BadArgument2)=", Status);

    // - val_AsymEccElGamalAllocPublicKeyAsset
    Status = val_AsymEccElGamalAllocPublicKeyAsset(VAL_ASYM_ECP_MIN_BITS-1, false, false, &AssetId);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalAllocPublicKeyAsset(BadArgument1L)=", Status);

    Status = val_AsymEccElGamalAllocPublicKeyAsset(VAL_ASYM_ECP_MAX_BITS+1, false, false, &AssetId);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalAllocPublicKeyAsset(BadArgument1H)=", Status);

    Status = val_AsymEccElGamalAllocPublicKeyAsset(VAL_ASYM_ECP_MIN_BITS, false, false, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalAllocPublicKeyAsset(BadArgument2)=", Status);

    // - val_AsymEccElGamalEncrypt/val_AsymEccElGamalDecrypt
    Point.x.Data_p = Buffer;
    Point.x.ByteDataSize = VAL_ASYM_DATA_SIZE_B2B(VAL_ASYM_ECP_MIN_BITS);
    Point.y.Data_p = Buffer;
    Point.y.ByteDataSize = VAL_ASYM_DATA_SIZE_B2B(VAL_ASYM_ECP_MIN_BITS);

    Status = val_AsymEccElGamalEncrypt(NULL, &Point, &Point, &Point);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalEncrypt(BadArgument1)=", Status);
    Status = val_AsymEccElGamalDecrypt(NULL, &Point, &Point, &Point);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalDecrypt(BadArgument1)=", Status);

    Key.KeyAssetId = VAL_ASSETID_INVALID;
    Key.DomainAssetId = AssetId;
    Key.ModulusSizeBits = VAL_ASYM_ECP_MIN_BITS;
    Key.HashAlgorithm = VAL_SYM_ALGO_NONE;

    Status = val_AsymEccElGamalEncrypt(&Key, &Point, &Point, &Point);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalEncrypt(BadArgument1I)=", Status);
    Status = val_AsymEccElGamalDecrypt(&Key, &Point, &Point, &Point);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalDecrypt(BadArgument1I)=", Status);
    Key.KeyAssetId = AssetId;

    Key.DomainAssetId = VAL_ASSETID_INVALID;
    Status = val_AsymEccElGamalEncrypt(&Key, &Point, &Point, &Point);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalEncrypt(BadArgument1I)=", Status);
    Status = val_AsymEccElGamalDecrypt(&Key, &Point, &Point, &Point);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalDecrypt(BadArgument1I)=", Status);
    Key.DomainAssetId = AssetId;

    Key.ModulusSizeBits = VAL_ASYM_ECP_MIN_BITS-1;
    Status = val_AsymEccElGamalEncrypt(&Key, &Point, &Point, &Point);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalEncrypt(BadArgument1I)=", Status);
    Status = val_AsymEccElGamalDecrypt(&Key, &Point, &Point, &Point);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalDecrypt(BadArgument1I)=", Status);

    Key.ModulusSizeBits = VAL_ASYM_ECP_MAX_BITS+1;
    Status = val_AsymEccElGamalEncrypt(&Key, &Point, &Point, &Point);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalEncrypt(BadArgument1I)=", Status);
    Status = val_AsymEccElGamalDecrypt(&Key, &Point, &Point, &Point);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalDecrypt(BadArgument1I)=", Status);
    Key.ModulusSizeBits = VAL_ASYM_ECP_MIN_BITS;

    Key.HashAlgorithm = VAL_SYM_ALGO_HASH_SHA1;
    Status = val_AsymEccElGamalEncrypt(&Key, &Point, &Point, &Point);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalEncrypt(BadArgument1I)=", Status);
    Status = val_AsymEccElGamalDecrypt(&Key, &Point, &Point, &Point);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalDecrypt(BadArgument1I)=", Status);
    Key.HashAlgorithm = VAL_SYM_ALGO_NONE;

    Status = val_AsymEccElGamalEncrypt(&Key, NULL, &Point, &Point);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalEncrypt(BadArgument2)=", Status);
    Status = val_AsymEccElGamalDecrypt(&Key, NULL, &Point, &Point);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalDecrypt(BadArgument2)=", Status);
    Status = val_AsymEccElGamalEncrypt(&Key, &Point, NULL, &Point);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalEncrypt(BadArgument3)=", Status);
    Status = val_AsymEccElGamalDecrypt(&Key, &Point, NULL, &Point);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalDecrypt(BadArgument3)=", Status);
    Status = val_AsymEccElGamalEncrypt(&Key, &Point, &Point, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalEncrypt(BadArgument4)=", Status);
    Status = val_AsymEccElGamalDecrypt(&Key, &Point, &Point, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalDecrypt(BadArgument4)=", Status);

    ErrorPoint = Point;
    ErrorPoint.x.Data_p = NULL;
    Status = val_AsymEccElGamalEncrypt(&Key, &ErrorPoint, &Point, &Point);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalEncrypt(BadArgument2I)=", Status);
    Status = val_AsymEccElGamalDecrypt(&Key, &ErrorPoint, &Point, &Point);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalDecrypt(BadArgument2I)=", Status);
    Status = val_AsymEccElGamalEncrypt(&Key, &Point, &ErrorPoint, &Point);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalEncrypt(BadArgument3I)=", Status);
    Status = val_AsymEccElGamalDecrypt(&Key, &Point, &ErrorPoint, &Point);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalDecrypt(BadArgument3I)=", Status);
    Status = val_AsymEccElGamalEncrypt(&Key, &Point, &Point, &ErrorPoint);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalEncrypt(BadArgument4I)=", Status);
    Status = val_AsymEccElGamalDecrypt(&Key, &Point, &Point, &ErrorPoint);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalDecrypt(BadArgument4I)=", Status);
    ErrorPoint.x.Data_p = Buffer;

    ErrorPoint.x.ByteDataSize--;
    Status = val_AsymEccElGamalEncrypt(&Key, &ErrorPoint, &Point, &Point);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalEncrypt(BadArgument2I)=", Status);
    Status = val_AsymEccElGamalDecrypt(&Key, &ErrorPoint, &Point, &Point);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalDecrypt(BadArgument2I)=", Status);
    Status = val_AsymEccElGamalEncrypt(&Key, &Point, &ErrorPoint, &Point);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalEncrypt(BadArgument3I)=", Status);
    Status = val_AsymEccElGamalDecrypt(&Key, &Point, &ErrorPoint, &Point);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalDecrypt(BadArgument3I)=", Status);
    Status = val_AsymEccElGamalEncrypt(&Key, &Point, &Point, &ErrorPoint);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalEncrypt(BadArgument4I)=", Status);
    Status = val_AsymEccElGamalDecrypt(&Key, &Point, &Point, &ErrorPoint);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalDecrypt(BadArgument4I)=", Status);
    ErrorPoint.x.ByteDataSize++;

    ErrorPoint.y.Data_p = NULL;
    Status = val_AsymEccElGamalEncrypt(&Key, &ErrorPoint, &Point, &Point);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalEncrypt(BadArgument2I)=", Status);
    Status = val_AsymEccElGamalDecrypt(&Key, &ErrorPoint, &Point, &Point);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalDecrypt(BadArgument2I)=", Status);
    Status = val_AsymEccElGamalEncrypt(&Key, &Point, &ErrorPoint, &Point);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalEncrypt(BadArgument3I)=", Status);
    Status = val_AsymEccElGamalDecrypt(&Key, &Point, &ErrorPoint, &Point);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalDecrypt(BadArgument3I)=", Status);
    Status = val_AsymEccElGamalEncrypt(&Key, &Point, &Point, &ErrorPoint);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalEncrypt(BadArgument4I)=", Status);
    Status = val_AsymEccElGamalDecrypt(&Key, &Point, &Point, &ErrorPoint);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalDecrypt(BadArgument4I)=", Status);
    ErrorPoint.y.Data_p = Buffer;

    ErrorPoint.y.ByteDataSize--;
    Status = val_AsymEccElGamalEncrypt(&Key, &ErrorPoint, &Point, &Point);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalEncrypt(BadArgument2I)=", Status);
    Status = val_AsymEccElGamalDecrypt(&Key, &ErrorPoint, &Point, &Point);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalDecrypt(BadArgument2I)=", Status);
    Status = val_AsymEccElGamalEncrypt(&Key, &Point, &ErrorPoint, &Point);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalEncrypt(BadArgument3I)=", Status);
    Status = val_AsymEccElGamalDecrypt(&Key, &Point, &ErrorPoint, &Point);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalDecrypt(BadArgument3I)=", Status);
    Status = val_AsymEccElGamalEncrypt(&Key, &Point, &Point, &ErrorPoint);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalEncrypt(BadArgument4I)=", Status);
    Status = val_AsymEccElGamalDecrypt(&Key, &Point, &Point, &ErrorPoint);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_AsymEccElGamalDecrypt(BadArgument4I)=", Status);
    ErrorPoint.y.ByteDataSize++;
}
END_TEST

START_TEST(test_AsymEccElgamalInitKey)
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
suite_add_test_AsymEccElGamal(
        struct TestSuite * TestSuite_p)
{
    struct TestCase * TestCase_p;

    TestCase_p = sfzutf_tcase_create(TestSuite_p, "AsymCrypto_ECC_ElGamal_Vectors");
    if (TestCase_p != NULL)
    {
        if (sfzutf_tcase_add_fixture(TestCase_p, valtest_initialize, valtest_terminate) != 0)
        {
             goto FuncErrorReturn;
        }

        if (sfzutf_test_add(TestCase_p, test_AsymEccElgamalVectorsParamsCheck) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add(TestCase_p, test_AsymEccElgamalVectorsCheck) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add(TestCase_p, test_AsymEccElgamalVectorsKeyPairCheck) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add(TestCase_p, test_AsymEccElgamalVectorsKeyPairGenCheck) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add(TestCase_p, test_AsymEccElgamalVectorsPublicKeyGenCheck) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add(TestCase_p, test_AsymEccElgamalInitKey) != 0) goto FuncErrorReturn;
        if (valtest_StrictArgsCheck())
        {
            if (sfzutf_test_add(TestCase_p, test_AsymEccElgamalArguments) != 0) goto FuncErrorReturn;
        }
        return 0;
    }

FuncErrorReturn:
    return -1;
}

/* end of file valtest_asym_ecc_elgamal.c */
