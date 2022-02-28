/* valtest_asym_ecdh.c
 *
 * Description: ECDH tests
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

#include "valtest_internal.h"

#include "testvectors_ecc_curves.h"

int
test_AsymEccLoadDomain(
        const TestVector_ECC_Curve_Rec_t * Curve_p,
        ValAssetId_t * DomainAssetId_p,
        bool ftestExport);


/*----------------------------------------------------------------------------
 * test_ecdh_GenerateSharedSecret
 *
 * Helper function that performs the ECDH operations.
 * If fDual is set, the dual keypair ECDH operation is used otherwise a single
 * key pair.
 * If fSaveSharedSecret is set, the shared secret is saved to an Asset, which
 * is then used to do an external/other key derivation otherwise the ECDH
 * operation key derivation is used.
 */
static int
test_ecdh_GenerateSharedSecret(
        TestVector_ECC_Curve_t Curve_p,
        bool fDual,
        bool fSaveSharedSecret)
{
    static const char * SomeOtherInfo_p = "Some data as OtherInfo";
    static const char * Message_p = "This is some text that is used to check the secrets";
    int rc;
    int ListAssets = 0;
    ValStatus_t Status;
    ValSize_t ModulusSizeBits;
    ValAssetId_t DomainAssetId = VAL_ASSETID_INVALID;
    ValAssetId_t SndPrivateKeyAssetId = VAL_ASSETID_INVALID;
    ValAssetId_t RcvPrivateKeyAssetId = VAL_ASSETID_INVALID;
    ValAssetId_t SndPublicKeyAssetId = VAL_ASSETID_INVALID;
    ValAssetId_t RcvPublicKeyAssetId = VAL_ASSETID_INVALID;
    ValAssetId_t SndPrivateKey2AssetId = VAL_ASSETID_INVALID;
    ValAssetId_t RcvPrivateKey2AssetId = VAL_ASSETID_INVALID;
    ValAssetId_t SndPublicKey2AssetId = VAL_ASSETID_INVALID;
    ValAssetId_t RcvPublicKey2AssetId = VAL_ASSETID_INVALID;
    ValAsymKey_t SndPrivateKey;
    ValAsymKey_t RcvPrivateKey;
    ValAsymKey_t SndPublicKey;
    ValAsymKey_t RcvPublicKey;
    ValAsymKey_t SndPrivateKey2;
    ValAsymKey_t RcvPrivateKey2;
    ValAsymKey_t SndPublicKey2;
    ValAsymKey_t RcvPublicKey2;
    ValAssetId_t SndAssetList[2];
    ValAssetId_t RcvAssetList[2];
    ValSymContextPtr_t SndSymContext_p = NULL;
    ValSymContextPtr_t RcvSymContext_p = NULL;
    ValPolicyMask_t AssetPolicy;
    ValOctetsOut_t SndMac[VAL_SYM_ALGO_MAX_DIGEST_SIZE];
    ValOctetsOut_t RcvMac[VAL_SYM_ALGO_MAX_DIGEST_SIZE];
    ValSize_t SndMacSize = 0;
    ValSize_t RcvMacSize = 0;
    uint8_t * Msg_p = NULL;
    uint8_t * Info_p = NULL;
    ValSize_t MsgSize = sizeof(Message_p);
    ValSize_t InfoSize = 0;

    /* Load domain parameters */
    rc = test_AsymEccLoadDomain(Curve_p, &DomainAssetId, false);
    if (rc != END_TEST_SUCCES)
    {
        return rc;
    }
    ModulusSizeBits = Curve_p->CurveBits;

    /* Allocate and generate the first key pair */
    /* Sender */
    /* - Allocate the private key asset and initialize key structure */
    Status = val_AsymEcdhAllocPrivateKeyAsset(ModulusSizeBits, false, false, &SndPrivateKeyAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AsymEcdhAllocPrivateKeyAsset(Sender)=", Status);

    Status = val_AsymInitKey(SndPrivateKeyAssetId, DomainAssetId,
                             ModulusSizeBits, VAL_SYM_ALGO_NONE,
                             &SndPrivateKey);
    fail_if(Status != VAL_SUCCESS, "val_AsymInitKey(Sender)=", Status);

    /* - Allocate the public key asset and initialize key structure */
    Status = val_AsymEcdhAllocPublicKeyAsset(ModulusSizeBits, false, false, &SndPublicKeyAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AsymEcdhAllocPublicKeyAsset(Sender)=", Status);

    Status = val_AsymInitKey(SndPublicKeyAssetId, DomainAssetId,
                             ModulusSizeBits, VAL_SYM_ALGO_NONE,
                             &SndPublicKey);
    fail_if(Status != VAL_SUCCESS, "val_AsymInitKey(Sender)=", Status);

    /* - Generate a key pair */
    Status = val_AsymEccGenKeyPair(&SndPublicKey, &SndPrivateKey,
                                   VAL_ASSETID_INVALID, NULL, 0, NULL, NULL, NULL);
    fail_if(Status != VAL_SUCCESS, "val_AsymEccGenKeyPair(Sender)=", Status);

    /* Receiver */
    /* - Allocate the private key asset and initialize key structure */
    Status = val_AsymEcdhAllocPrivateKeyAsset(ModulusSizeBits, false, false, &RcvPrivateKeyAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AsymEcdhAllocPrivateKeyAsset(Receiver)=", Status);

    Status = val_AsymInitKey(RcvPrivateKeyAssetId, DomainAssetId,
                             ModulusSizeBits, VAL_SYM_ALGO_NONE,
                             &RcvPrivateKey);
    fail_if(Status != VAL_SUCCESS, "val_AsymInitKey(Receiver)=", Status);

    /* - Allocate the public key asset and initialize key structure */
    Status = val_AsymEcdhAllocPublicKeyAsset(ModulusSizeBits, false, false, &RcvPublicKeyAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AsymEcdhAllocPublicKeyAsset(Receiver)=", Status);

    Status = val_AsymInitKey(RcvPublicKeyAssetId, DomainAssetId,
                             ModulusSizeBits, VAL_SYM_ALGO_NONE,
                             &RcvPublicKey);
    fail_if(Status != VAL_SUCCESS, "val_AsymInitKey(Receiver)=", Status);

    /* - Generate a key pair */
    Status = val_AsymEccGenKeyPair(&RcvPublicKey, &RcvPrivateKey,
                                   VAL_ASSETID_INVALID, NULL, 0, NULL, NULL, NULL);
    fail_if(Status != VAL_SUCCESS, "val_AsymEccGenKeyPair(Receiver)=", Status);

    if (fDual)
    {
        /* Allocate and generate the second key pair */
        /* Sender */
        /* - Allocate the private key asset and initialize key structure */
        Status = val_AsymEcdhAllocPrivateKeyAsset(ModulusSizeBits, false, false, &SndPrivateKey2AssetId);
        fail_if(Status != VAL_SUCCESS, "val_AsymEcdhAllocPrivateKeyAsset(Sender2)=", Status);

        Status = val_AsymInitKey(SndPrivateKey2AssetId, DomainAssetId,
                                 ModulusSizeBits, VAL_SYM_ALGO_NONE,
                                 &SndPrivateKey2);
        fail_if(Status != VAL_SUCCESS, "val_AsymInitKey(Sender2)=", Status);

        /* - Allocate the public key asset and initialize key structure */
        Status = val_AsymEcdhAllocPublicKeyAsset(ModulusSizeBits, false, false, &SndPublicKey2AssetId);
        fail_if(Status != VAL_SUCCESS, "val_AsymEcdhAllocPublicKeyAsset(Sender2)=", Status);

        Status = val_AsymInitKey(SndPublicKey2AssetId, DomainAssetId,
                                 ModulusSizeBits, VAL_SYM_ALGO_NONE,
                                 &SndPublicKey2);
        fail_if(Status != VAL_SUCCESS, "val_AsymInitKey(Sender2)=", Status);

        /* - Generate a key pair */
        Status = val_AsymEccGenKeyPair(&SndPublicKey2, &SndPrivateKey2,
                                       VAL_ASSETID_INVALID, NULL, 0, NULL, NULL, NULL);
        fail_if(Status != VAL_SUCCESS, "val_AsymEccGenKeyPair(Sender2)=", Status);

        /* Receiver */
        /* - Allocate the private key asset and initialize key structure */
        Status = val_AsymEcdhAllocPrivateKeyAsset(ModulusSizeBits, false, false, &RcvPrivateKey2AssetId);
        fail_if(Status != VAL_SUCCESS, "val_AsymEcdhAllocPrivateKeyAsset(Receiver2)=", Status);

        Status = val_AsymInitKey(RcvPrivateKey2AssetId, DomainAssetId,
                                 ModulusSizeBits, VAL_SYM_ALGO_NONE,
                                 &RcvPrivateKey2);
        fail_if(Status != VAL_SUCCESS, "val_AsymInitKey(Receiver2)=", Status);

        /* - Allocate the public key asset and initialize key structure */
        Status = val_AsymEcdhAllocPublicKeyAsset(ModulusSizeBits, false, false, &RcvPublicKey2AssetId);
        fail_if(Status != VAL_SUCCESS, "val_AsymEcdhAllocPublicKeyAsset(Receiver2)=", Status);

        Status = val_AsymInitKey(RcvPublicKey2AssetId, DomainAssetId,
                                 ModulusSizeBits, VAL_SYM_ALGO_NONE,
                                 &RcvPublicKey2);
        fail_if(Status != VAL_SUCCESS, "val_AsymInitKey(Receiver2)=", Status);

        /* - Generate a key pair */
        Status = val_AsymEccGenKeyPair(&RcvPublicKey2, &RcvPrivateKey2,
                                       VAL_ASSETID_INVALID, NULL, 0, NULL, NULL, NULL);
        fail_if(Status != VAL_SUCCESS, "val_AsymEccGenKeyPair(Receiver2)=", Status);
    }

    /* Allocate assets to be initialized via Shared Secret generation */
    if (fSaveSharedSecret)
    {
        ValSize_t DeriveKeySize = (((ModulusSizeBits + 31)/32) * 4);
        if (fDual)
        {
            /* The shared secret has a double size */
            DeriveKeySize *= 2;
        }

        AssetPolicy = VAL_POLICY_KEY_DERIVE | VAL_POLICY_SHA256;
        if (!val_IsAccessSecure())
        {
            AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
        }
        Status = val_AssetAlloc(AssetPolicy, DeriveKeySize,
                                false, false,
                                VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                                &SndAssetList[0]);
        fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(SenderDeriveKey)=", Status);
        Status = val_AssetAlloc(AssetPolicy, DeriveKeySize,
                                false, false,
                                VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                                &RcvAssetList[0]);
        fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(ReceiverDeriveKey)=", Status);

        ListAssets = 1;
    }
    else
    {
        AssetPolicy = VAL_POLICY_ALGO_CIPHER_AES|VAL_POLICY_MODE2|VAL_POLICY_ENCRYPT|VAL_POLICY_DECRYPT;
        if (!val_IsAccessSecure())
        {
            AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
        }
        Status = val_AssetAlloc(AssetPolicy, 32,
                                false, false,
                                VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                                &SndAssetList[0]);
        fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(SenderAESKey)=", Status);
        Status = val_AssetAlloc(AssetPolicy, 32,
                                false, false,
                                VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                                &RcvAssetList[0]);
        fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(ReceiverAESKey)=", Status);

        InfoSize = sizeof(SomeOtherInfo_p);
        Info_p = (uint8_t *)SFZUTF_MALLOC(InfoSize);
        fail_if(Info_p == NULL, "Allocation ", (int)InfoSize);
        memcpy(Info_p, SomeOtherInfo_p, InfoSize);

        ListAssets = 2;
    }

    AssetPolicy = VAL_POLICY_MAC_GENERATE|VAL_POLICY_SHA256;
    if (!val_IsAccessSecure())
    {
        AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
    }
    Status = val_AssetAlloc(AssetPolicy, 32,
                            false, false,
                            VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                            &SndAssetList[1]);
    fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(SenderHMACKey)=", Status);
    Status = val_AssetAlloc(AssetPolicy, 32,
                            false, false,
                            VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                            &RcvAssetList[1]);
    fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(ReceiverHMACKey)=", Status);

    /* Initialized the keys */
    if (fDual)
    {
        Status = val_AsymEcdhGenSharedSecretDual(&RcvPublicKey, &SndPrivateKey,
                                                 &RcvPublicKey2, &SndPrivateKey2,
                                                 fSaveSharedSecret,
                                                 Info_p, InfoSize,
                                                 SndAssetList, (ValSize_t)ListAssets);
        if (fSaveSharedSecret && (Status == VAL_INVALID_ASSET))
        {
            rc = END_TEST_UNSUPPORTED;
            goto NotSupportedCleanup;
        }
        fail_if(Status != VAL_SUCCESS, "val_AsymEcdhGenSharedSecretDual(Sender)=", Status);
        Status = val_AsymEcdhGenSharedSecretDual(&SndPublicKey, &RcvPrivateKey,
                                                 &SndPublicKey2, &RcvPrivateKey2,
                                                 fSaveSharedSecret,
                                                 Info_p, InfoSize,
                                                 RcvAssetList, (ValSize_t)ListAssets);
        fail_if(Status != VAL_SUCCESS, "val_AsymEcdhGenSharedSecretDual(Receiver)=", Status);
    }
    else
    {
        Status = val_AsymEcdhGenSharedSecretSingle(&RcvPublicKey, &SndPrivateKey,
                                                   fSaveSharedSecret,
                                                   Info_p, InfoSize,
                                                   SndAssetList, (ValSize_t)ListAssets);
        if (fSaveSharedSecret && (Status == VAL_INVALID_ASSET))
        {
            rc = END_TEST_UNSUPPORTED;
            goto NotSupportedCleanup;
        }
        fail_if(Status != VAL_SUCCESS, "val_AsymEcdhGenSharedSecretSingle(Sender)=", Status);
        Status = val_AsymEcdhGenSharedSecretSingle(&SndPublicKey, &RcvPrivateKey,
                                                   fSaveSharedSecret,
                                                   Info_p, InfoSize,
                                                   RcvAssetList, (ValSize_t)ListAssets);
        fail_if(Status != VAL_SUCCESS, "val_AsymEcdhGenSharedSecretSingle(Receiver)=", Status);
    }

    if (fSaveSharedSecret)
    {
        /* Initialize HMAC keys via a key derivation from the shared secret */
        InfoSize = strlen(g_ValTestAssociatedData_p);
        Info_p = (uint8_t *)SFZUTF_MALLOC(InfoSize);
        fail_if(Info_p == NULL, "Allocation ", (int)InfoSize);
        memcpy(Info_p, g_ValTestAssociatedData_p, InfoSize);

        Status = val_AssetLoadDerive(SndAssetList[1], SndAssetList[0],
                                     Info_p, InfoSize,
                                     false, false, NULL, 0, NULL, 0);
        fail_if(Status != VAL_SUCCESS, "val_AssetLoadDerive(SenderHMACKey)=", Status);

        Status = val_AssetLoadDerive(RcvAssetList[1], RcvAssetList[0],
                                     Info_p, InfoSize,
                                     false, false, NULL, 0, NULL, 0);
        fail_if(Status != VAL_SUCCESS, "val_AssetLoadDerive(SenderHMACKey)=", Status);
    }

    SFZUTF_FREE(Info_p);
    Info_p = NULL;

    /* Check the generate key(s) */
    Status = val_SymAlloc(VAL_SYM_ALGO_MAC_HMAC_SHA256, VAL_SYM_MODE_NONE, false, &SndSymContext_p);
    fail_if(Status != VAL_SUCCESS, "val_SymAlloc(Sender)=", Status);
    Status = val_SymInitKey(SndSymContext_p, SndAssetList[1], NULL, 22);
    fail_if(Status != VAL_SUCCESS, "val_SymInitKey(Sender)=", Status);

    Status = val_SymAlloc(VAL_SYM_ALGO_MAC_HMAC_SHA256, VAL_SYM_MODE_NONE, false, &RcvSymContext_p);
    fail_if(Status != VAL_SUCCESS, "val_SymAlloc(Receiver)=", Status);
    Status = val_SymInitKey(RcvSymContext_p, RcvAssetList[1], NULL, 22);
    fail_if(Status != VAL_SUCCESS, "val_SymInitKey(Receiver)=", Status);

    Msg_p = (uint8_t *)SFZUTF_MALLOC(MsgSize);
    fail_if(Msg_p == NULL, "Allocation ", (int)MsgSize);
    memcpy(Msg_p, Message_p, MsgSize);

    SndMacSize = sizeof(SndMac);
    Status = val_SymMacGenerate(SndSymContext_p, Msg_p, MsgSize, SndMac, &SndMacSize);
    fail_if(Status != VAL_SUCCESS, "val_SymMacGenerate()=", Status);

    RcvMacSize = sizeof(RcvMac);
    Status = val_SymMacGenerate(RcvSymContext_p, Msg_p, MsgSize, RcvMac, &RcvMacSize);
    fail_if(Status != VAL_SUCCESS, "val_SymMacGenerate()=", Status);

    fail_if(SndMacSize != RcvMacSize, "", 0);
    fail_if(memcmp(SndMac, RcvMac, SndMacSize) != 0, "", 0);

    SFZUTF_FREE(Msg_p);

NotSupportedCleanup:
    if (Info_p != NULL)
    {
        SFZUTF_FREE(Info_p);
    }

    /* Release used assets */
    Status = val_AssetFree(DomainAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree(Domain)=", Status);
    Status = val_AssetFree(SndPrivateKeyAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree(SndPrivateKey)=", Status);
    Status = val_AssetFree(RcvPrivateKeyAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree(RcvPrivateKey)=", Status);
    Status = val_AssetFree(SndPublicKeyAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree(SndPublicKey)=", Status);
    Status = val_AssetFree(RcvPublicKeyAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree(RcvPublicKey)=", Status);
    if (fDual)
    {
        Status = val_AssetFree(SndPrivateKey2AssetId);
        fail_if(Status != VAL_SUCCESS, "val_AssetFree(SndPrivateKey2)=", Status);
        Status = val_AssetFree(RcvPrivateKey2AssetId);
        fail_if(Status != VAL_SUCCESS, "val_AssetFree(RcvPrivateKey2)=", Status);
        Status = val_AssetFree(SndPublicKey2AssetId);
        fail_if(Status != VAL_SUCCESS, "val_AssetFree(SndPublicKey2)=", Status);
        Status = val_AssetFree(RcvPublicKey2AssetId);
        fail_if(Status != VAL_SUCCESS, "val_AssetFree(RcvPublicKey2)=", Status);
    }
    Status = val_AssetFree(SndAssetList[0]);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree(SndAssetList[0])=", Status);
    Status = val_AssetFree(SndAssetList[1]);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree(SndAssetList[1])=", Status);
    Status = val_AssetFree(RcvAssetList[0]);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree(RcvAssetList[0])=", Status);
    Status = val_AssetFree(RcvAssetList[1]);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree(RcvAssetList[1])=", Status);

    if (rc == END_TEST_UNSUPPORTED)
    {
        unsupported("Shared secret saving");
    }
    return END_TEST_SUCCES;
}


START_TEST(test_ecdh_GenerateSharedSecret_192)
{
    int rc;

    unsupported_unless(valtest_IsTrngActive(true), "TRNG not activated");

    rc = test_ecdh_GenerateSharedSecret(ECC_Curve_NIST_P192,
                                        ((_i & 1) != 0), ((_i & 2) != 0));
    if (rc != END_TEST_SUCCES)
    {
        return rc;
    }
}
END_TEST

START_TEST(test_ecdh_GenerateSharedSecret_224)
{
    int rc;

    unsupported_unless(valtest_IsTrngActive(true), "TRNG not activated");

    rc = test_ecdh_GenerateSharedSecret(ECC_Curve_NIST_P224,
                                        ((_i & 1) != 0), ((_i & 2) != 0));
    if (rc != END_TEST_SUCCES)
    {
        return rc;
    }
}
END_TEST

START_TEST(test_ecdh_GenerateSharedSecret_256)
{
    int rc;

    unsupported_unless(valtest_IsTrngActive(true), "TRNG not activated");

    rc = test_ecdh_GenerateSharedSecret(ECC_Curve_NIST_P256,
                                        ((_i & 1) != 0), ((_i & 2) != 0));
    if (rc != END_TEST_SUCCES)
    {
        return rc;
    }
}
END_TEST

START_TEST(test_ecdh_GenerateSharedSecret_384)
{
    int rc;

    unsupported_unless(valtest_IsTrngActive(true), "TRNG not activated");

    rc = test_ecdh_GenerateSharedSecret(ECC_Curve_NIST_P384,
                                        ((_i & 1) != 0), ((_i & 2) != 0));
    if (rc != END_TEST_SUCCES)
    {
        return rc;
    }
}
END_TEST

START_TEST(test_ecdh_GenerateSharedSecret_521)
{
    int rc;

    unsupported_unless(valtest_IsTrngActive(true), "TRNG not activated");

    rc = test_ecdh_GenerateSharedSecret(ECC_Curve_NIST_P521,
                                        ((_i & 1) != 0), ((_i & 2) != 0));
    if (rc != END_TEST_SUCCES)
    {
        return rc;
    }
}
END_TEST


int
suite_add_test_AsymEcdh(
    struct TestSuite * TestSuite_p)
{
    struct TestCase * TestCase_p;

    TestCase_p = sfzutf_tcase_create(TestSuite_p, "AsymCrypto_ECDH_Vectors");
    if (TestCase_p != NULL)
    {
        if (sfzutf_tcase_add_fixture(TestCase_p, valtest_initialize, valtest_terminate) != 0)
        {
            goto FuncErrorReturn;
        }

        if (sfzutf_test_add_loop(TestCase_p, test_ecdh_GenerateSharedSecret_192, 4) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add_loop(TestCase_p, test_ecdh_GenerateSharedSecret_224, 4) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add_loop(TestCase_p, test_ecdh_GenerateSharedSecret_256, 4) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add_loop(TestCase_p, test_ecdh_GenerateSharedSecret_384, 2) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add_loop(TestCase_p, test_ecdh_GenerateSharedSecret_521, 2) != 0) goto FuncErrorReturn;

        return 0;
    }

FuncErrorReturn:
    return -1;
}

/* end of file valtest_asym_ecdh.c */
