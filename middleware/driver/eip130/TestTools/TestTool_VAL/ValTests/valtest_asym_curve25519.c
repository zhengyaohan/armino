/* valtest_asym_curve25519.c
 *
 * Description: Curve25519 tests
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
 * test_curve25519_GenerateSharedSecrets
 *
 * Helper function
 * If fSaveSharedSecret is set, the shared secret is saved to an Asset, which
 * is then used to do an external/other key derivation otherwise the operation
 * key derivation is used.
 */
#ifdef VALTEST_ASYM_CURVE25519
static int
test_curve25519_GenerateSharedSecrets(
        TestVector_ECC_Curve_t Curve_p,
        bool fSaveSharedSecret)
{
    static const char * SomeOtherInfo_p = "Some data as OtherInfo";
    static const char * Message_p = "This is some text that is used to check the secrets";
    int rc;
    int ListAssets = 0;
    ValStatus_t Status;
    ValSize_t ModulusSizeBits;
    ValAssetId_t DomainAssetId = VAL_ASSETID_INVALID;
    ValAssetId_t AlicePrivateKeyAssetId = VAL_ASSETID_INVALID;
    ValAssetId_t AlicePublicKeyAssetId = VAL_ASSETID_INVALID;
    ValAssetId_t AlicePublicKey2AssetId = VAL_ASSETID_INVALID;
    ValAssetId_t BobPrivateKeyAssetId = VAL_ASSETID_INVALID;
    ValAssetId_t BobPublicKeyAssetId = VAL_ASSETID_INVALID;
    ValAsymKey_t AlicePrivateKey;
    ValAsymKey_t AlicePublicKey;
    ValAsymKey_t AlicePublicKey2;
    ValAsymKey_t BobPrivateKey;
    ValAsymKey_t BobPublicKey;
    ValAssetId_t AliceAssetList[2];
    ValAssetId_t BobAssetList[2];
    ValSymContextPtr_t AliceSymContext_p = NULL;
    ValSymContextPtr_t BobSymContext_p = NULL;
    ValPolicyMask_t AssetPolicy;
    ValAsymBigInt_t PrivateKeyData;
    ValOctetsOut_t AliceMac[VAL_SYM_ALGO_MAX_DIGEST_SIZE];
    ValOctetsOut_t BobMac[VAL_SYM_ALGO_MAX_DIGEST_SIZE];
    ValSize_t AliceMacSize = 0;
    ValSize_t BobMacSize = 0;
    uint8_t * Msg_p;
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

    /* Allocate and load the key for function coverage */
    /* - Allocate the private key asset */
    Status = val_AsymCurve25519AllocPrivateKeyAsset(false, false, &AlicePrivateKeyAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AsymCurve25519AllocPrivateKeyAsset()=", Status);

    /* - Initialize key structure */
    Status = val_AsymInitKey(AlicePrivateKeyAssetId, DomainAssetId,
                             ModulusSizeBits, VAL_SYM_ALGO_NONE,
                             &AlicePrivateKey);
    fail_if(Status != VAL_SUCCESS, "val_AsymInitKey()=", Status);

    /* - Load the private key asset with some data */
    PrivateKeyData.ByteDataSize = (uint32_t)(ModulusSizeBits + 7) / 8;
    PrivateKeyData.Data_p = (uint8_t *)SFZUTF_MALLOC(PrivateKeyData.ByteDataSize);
    fail_if(PrivateKeyData.Data_p == NULL, "Allocation ", (int)PrivateKeyData.ByteDataSize);
    memset(PrivateKeyData.Data_p, 0x55, PrivateKeyData.ByteDataSize);
    Status = val_AsymCurve25519LoadKeyAssetPlaintext(&PrivateKeyData, AlicePrivateKeyAssetId,
                                                     VAL_ASSETID_INVALID, NULL, 0, NULL, 0);
    SFZUTF_FREE(PrivateKeyData.Data_p);
    fail_if(Status != VAL_SUCCESS, "val_AsymCurve25519LoadKeyAssetPlaintext()=", Status);

    /* - Delete the private key asset again */
    Status = val_AssetFree(AlicePrivateKeyAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree()=", Status);
    AlicePrivateKeyAssetId = VAL_ASSETID_INVALID;

    /* Allocate and generate the key pair for Alice */
    /* - Allocate the private key asset and initialize key structure */
    Status = val_AsymCurve25519AllocPrivateKeyAsset(false, false, &AlicePrivateKeyAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AsymCurve25519AllocPrivateKeyAsset()=", Status);

    Status = val_AsymInitKey(AlicePrivateKeyAssetId, DomainAssetId,
                             ModulusSizeBits, VAL_SYM_ALGO_NONE,
                             &AlicePrivateKey);
    fail_if(Status != VAL_SUCCESS, "val_AsymInitKey()=", Status);

    /* - Allocate the public key asset and initialize key structure */
    Status = val_AsymCurve25519AllocPublicKeyAsset(false, false, &AlicePublicKeyAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AsymCurve25519AllocPublicKeyAsset()=", Status);

    Status = val_AsymInitKey(AlicePublicKeyAssetId, DomainAssetId,
                             ModulusSizeBits, VAL_SYM_ALGO_NONE,
                             &AlicePublicKey);
    fail_if(Status != VAL_SUCCESS, "val_AsymInitKey()=", Status);

    /* - Generate a key pair */
    Status = val_AsymCurve25519GenKeyPair(&AlicePublicKey, &AlicePrivateKey,
                                          VAL_ASSETID_INVALID, NULL, 0, NULL, NULL, NULL);
    fail_if(Status != VAL_SUCCESS, "val_AsymCurve25519GenKeyPair()=", Status);

    /* Check public key generation for Alice */
    /* - Allocate the public key asset and initialize key structure */
    Status = val_AsymCurve25519AllocPublicKeyAsset(false, false, &AlicePublicKey2AssetId);
    fail_if(Status != VAL_SUCCESS, "val_AsymCurve25519AllocPublicKeyAsset()=", Status);

    Status = val_AsymInitKey(AlicePublicKey2AssetId, DomainAssetId,
                             ModulusSizeBits, VAL_SYM_ALGO_NONE,
                             &AlicePublicKey2);
    fail_if(Status != VAL_SUCCESS, "val_AsymInitKey()=", Status);

    /* - Generate the public key */
    Status = val_AsymCurve25519GenPublicKey(&AlicePublicKey2, &AlicePrivateKey, NULL);
    fail_if(Status != VAL_SUCCESS, "val_AsymCurve25519GenPublicKey()=", Status);

    /* Allocate and generate the key pair for Bob */
    /* - Allocate the private key asset and initialize key structure */
    Status = val_AsymCurve25519AllocPrivateKeyAsset(false, false, &BobPrivateKeyAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AsymCurve25519AllocPrivateKeyAsset()=", Status);

    Status = val_AsymInitKey(BobPrivateKeyAssetId, DomainAssetId,
                             ModulusSizeBits, VAL_SYM_ALGO_NONE,
                             &BobPrivateKey);
    fail_if(Status != VAL_SUCCESS, "val_AsymInitKey()=", Status);

    /* - Allocate the public key asset and initialize key structure */
    Status = val_AsymCurve25519AllocPublicKeyAsset(false, false, &BobPublicKeyAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AsymCurve25519AllocPublicKeyAsset()=", Status);

    Status = val_AsymInitKey(BobPublicKeyAssetId, DomainAssetId,
                             ModulusSizeBits, VAL_SYM_ALGO_NONE,
                             &BobPublicKey);
    fail_if(Status != VAL_SUCCESS, "val_AsymInitKey()=", Status);

    /* - Generate a key pair */
    Status = val_AsymCurve25519GenKeyPair(&BobPublicKey, &BobPrivateKey,
                                          VAL_ASSETID_INVALID, NULL, 0, NULL, NULL, NULL);
    fail_if(Status != VAL_SUCCESS, "val_AsymCurve25519GenKeyPair()=", Status);

    /* Allocate assets to be initialized via Shared Secret generation */
    if (fSaveSharedSecret)
    {
        ValSize_t DeriveKeySize = (((ModulusSizeBits + 31)/32) * 4);
        AssetPolicy = VAL_POLICY_KEY_DERIVE|VAL_POLICY_SHA256;
        if (!val_IsAccessSecure())
        {
            AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
        }
        Status = val_AssetAlloc(AssetPolicy, DeriveKeySize,
                                false, false,
                                VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                                &AliceAssetList[0]);
        fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(AliceDeriveKey)=", Status);
        Status = val_AssetAlloc(AssetPolicy, DeriveKeySize,
                                false, false,
                                VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                                &BobAssetList[0]);
        fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(BobDeriveKey)=", Status);

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
                                &AliceAssetList[0]);
        fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(AliceAESKey)=", Status);
        Status = val_AssetAlloc(AssetPolicy, 32,
                                false, false,
                                VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                                &BobAssetList[0]);
        fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(BobAESKey)=", Status);

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
                            &AliceAssetList[1]);
    fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(AliceAESKey)=", Status);
    Status = val_AssetAlloc(AssetPolicy, 32,
                            false, false,
                            VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                            &BobAssetList[1]);
    fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(BobAESKey)=", Status);

    /* Initialized the keys */
    Status = val_AsymCurve25519GenSharedSecret(&BobPublicKey, &AlicePrivateKey,
                                               fSaveSharedSecret,
                                               Info_p, InfoSize,
                                               AliceAssetList, (ValSize_t)ListAssets);

    if (fSaveSharedSecret && (Status == VAL_INVALID_ASSET))
    {
        rc = END_TEST_UNSUPPORTED;
        goto NotSupportedCleanup;
    }
    fail_if(Status != VAL_SUCCESS, "val_AsymCurve25519GenSharedSecret(Alice)=", Status);
    Status = val_AsymCurve25519GenSharedSecret(&AlicePublicKey2, &BobPrivateKey,
                                               fSaveSharedSecret,
                                               Info_p, InfoSize,
                                               BobAssetList, (ValSize_t)ListAssets);
    fail_if(Status != VAL_SUCCESS, "val_AsymCurve25519GenSharedSecret(Bob)=", Status);

    if (fSaveSharedSecret)
    {
        /* Initialize HMAC keys via a key derivation from the shared secret */
        InfoSize = strlen(g_ValTestAssociatedData_p);
        Info_p = (uint8_t *)SFZUTF_MALLOC(InfoSize);
        fail_if(Info_p == NULL, "Allocation ", (int)InfoSize);
        memcpy(Info_p, g_ValTestAssociatedData_p, InfoSize);

        Status = val_AssetLoadDerive(AliceAssetList[1], AliceAssetList[0],
                                     Info_p, InfoSize,
                                     false, false, NULL, 0, NULL, 0);
        fail_if(Status != VAL_SUCCESS, "val_AssetLoadDerive(SenderHMACKey)=", Status);

        Status = val_AssetLoadDerive(BobAssetList[1], BobAssetList[0],
                                     Info_p, InfoSize,
                                     false, false, NULL, 0, NULL, 0);
        fail_if(Status != VAL_SUCCESS, "val_AssetLoadDerive(SenderHMACKey)=", Status);
    }

    SFZUTF_FREE(Info_p);
    Info_p = NULL;

    /* Check the generate key(s) */
    Status = val_SymAlloc(VAL_SYM_ALGO_MAC_HMAC_SHA256, VAL_SYM_MODE_NONE, false, &AliceSymContext_p);
    fail_if(Status != VAL_SUCCESS, "val_SymAlloc(Alice)=", Status);
    Status = val_SymInitKey(AliceSymContext_p, AliceAssetList[1], NULL, 22);
    fail_if(Status != VAL_SUCCESS, "val_SymInitKey(Alice)=", Status);

    Status = val_SymAlloc(VAL_SYM_ALGO_MAC_HMAC_SHA256, VAL_SYM_MODE_NONE, false, &BobSymContext_p);
    fail_if(Status != VAL_SUCCESS, "val_SymAlloc(Bob)=", Status);
    Status = val_SymInitKey(BobSymContext_p, BobAssetList[1], NULL, 22);
    fail_if(Status != VAL_SUCCESS, "val_SymInitKey(Bob)=", Status);

    Msg_p = (uint8_t *)SFZUTF_MALLOC(MsgSize);
    fail_if(Msg_p == NULL, "Allocation ", (int)MsgSize);
    memcpy(Msg_p, Message_p, MsgSize);

    AliceMacSize = sizeof(AliceMac);
    Status = val_SymMacGenerate(AliceSymContext_p, Msg_p, MsgSize, AliceMac, &AliceMacSize);
    fail_if(Status != VAL_SUCCESS, "val_SymMacGenerate()=", Status);

    BobMacSize = sizeof(BobMac);
    Status = val_SymMacGenerate(BobSymContext_p, Msg_p, MsgSize, BobMac, &BobMacSize);
    fail_if(Status != VAL_SUCCESS, "val_SymMacGenerate()=", Status);

    fail_if(AliceMacSize != BobMacSize, "", 0);
    fail_if(memcmp(AliceMac, BobMac, AliceMacSize) != 0, "", 0);

    SFZUTF_FREE(Msg_p);

NotSupportedCleanup:
    if (Info_p != NULL)
    {
        SFZUTF_FREE(Info_p);
    }

    /* Release used assets */
    Status = val_AssetFree(DomainAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree(Domain)=", Status);
    Status = val_AssetFree(AlicePrivateKeyAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree(AlicePrivateKey)=", Status);
    Status = val_AssetFree(BobPrivateKeyAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree(BobPrivateKey)=", Status);
    Status = val_AssetFree(AlicePublicKeyAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree(AlicePublicKey)=", Status);
    Status = val_AssetFree(AlicePublicKey2AssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree(AlicePublicKey2)=", Status);
    Status = val_AssetFree(BobPublicKeyAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree(BobPublicKey)=", Status);
    Status = val_AssetFree(AliceAssetList[0]);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree(AliceAssetList[0])=", Status);
    Status = val_AssetFree(AliceAssetList[1]);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree(AliceAssetList[1])=", Status);
    Status = val_AssetFree(BobAssetList[0]);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree(BobAssetList[0])=", Status);
    Status = val_AssetFree(BobAssetList[1]);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree(BobAssetList[1])=", Status);

    if (rc == END_TEST_UNSUPPORTED)
    {
        unsupported("Shared secret saving");
    }
    return END_TEST_SUCCES;
}
#endif

START_TEST(test_curve25519)
{
#ifdef VALTEST_ASYM_CURVE25519
    int rc;

    unsupported_unless(valtest_IsTrngActive(true), "TRNG not activated");

    rc = test_curve25519_GenerateSharedSecrets(ECC_Curve_25519,
                                               ((_i & 1) != 0));
    if (rc != END_TEST_SUCCES)
    {
        return rc;
    }
#else
    return END_TEST_UNSUPPORTED;
#endif
}
END_TEST


int
suite_add_test_AsymCurve25519(
    struct TestSuite * TestSuite_p)
{
    struct TestCase * TestCase_p;

    TestCase_p = sfzutf_tcase_create(TestSuite_p, "AsymCrypto_Curve25519_Vectors");
    if (TestCase_p != NULL)
    {
        if (sfzutf_tcase_add_fixture(TestCase_p,
                                     valtest_initialize,
                                     valtest_terminate) != 0)
        {
            goto FuncErrorReturn;
        }

        if (sfzutf_test_add_loop(TestCase_p, test_curve25519, 2) != 0) goto FuncErrorReturn;

        return 0;
    }

FuncErrorReturn:
    return -1;
}

/* end of file valtest_asym_ecdh.c */
