/* valtest_asset.c
 *
 * Description: Asset store tests
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
//#include "valtest_vectors_crypto.h"
//#include "valtest_tvstore.h"

/* Test vectors. */
#include "testvectors_aes_wrap.h"


/* Foreach macro, for iterating through arrays. */
#define FOREACH(var, index, array)                                      \
    for(((index) = 0),(var) = (array)[0];                               \
        ((index) < sizeof((array))/sizeof((array)[0]));                 \
        (var) = (++(index) < sizeof((array))/sizeof((array)[0]))?       \
            (array)[(index)]: (var))


typedef struct
{
    ValPolicyMask_t Policy;
    ValSize_t       Size;
    ValStatus_t     Result;
} TestAssetAllocateData_t;

const char * g_ValTestAssociatedData_p = "Some Associated Data to satisfy the various AssetLoad operations, which is also used to initialize the Asset data";

START_TEST(test_AssetAllocFree)
{
    ValAssetId_t AssetId;
    ValStatus_t Status;
    TestAssetAllocateData_t TestData;
    unsigned int foreach_helper;

    static const TestAssetAllocateData_t TestData_Table[] =
    {
        /* AES allowed key sizes. */
        { VAL_POLICY_ALGO_CIPHER_AES | VAL_POLICY_AES_MODE_CBC | VAL_POLICY_ENCRYPT | VAL_POLICY_DECRYPT, 16, VAL_SUCCESS },
        { VAL_POLICY_ALGO_CIPHER_AES | VAL_POLICY_AES_MODE_CBC | VAL_POLICY_ENCRYPT | VAL_POLICY_DECRYPT, 24, VAL_SUCCESS },
        { VAL_POLICY_ALGO_CIPHER_AES | VAL_POLICY_AES_MODE_CBC | VAL_POLICY_ENCRYPT | VAL_POLICY_DECRYPT, 32, VAL_SUCCESS },
        /* AES forbidden key sizes. */
        { VAL_POLICY_ALGO_CIPHER_AES | VAL_POLICY_AES_MODE_CBC | VAL_POLICY_ENCRYPT | VAL_POLICY_DECRYPT,  0, VAL_INVALID_LENGTH },
        { VAL_POLICY_ALGO_CIPHER_AES | VAL_POLICY_AES_MODE_CBC | VAL_POLICY_ENCRYPT | VAL_POLICY_DECRYPT,  8, VAL_INVALID_LENGTH },
        { VAL_POLICY_ALGO_CIPHER_AES | VAL_POLICY_AES_MODE_CBC | VAL_POLICY_ENCRYPT | VAL_POLICY_DECRYPT, 30, VAL_INVALID_LENGTH },
        { VAL_POLICY_ALGO_CIPHER_AES | VAL_POLICY_AES_MODE_CBC | VAL_POLICY_ENCRYPT | VAL_POLICY_DECRYPT, 33, VAL_INVALID_LENGTH },
        { VAL_POLICY_ALGO_CIPHER_AES | VAL_POLICY_AES_MODE_CBC | VAL_POLICY_ENCRYPT | VAL_POLICY_DECRYPT, 48, VAL_INVALID_LENGTH },
        /* AES no mode. */
        { VAL_POLICY_ALGO_CIPHER_AES | VAL_POLICY_ENCRYPT | VAL_POLICY_DECRYPT, 32, VAL_ACCESS_ERROR },
        /* Derive. */
        { VAL_POLICY_TRUSTED_KEY_DERIVE, 16, VAL_SUCCESS },
        { VAL_POLICY_TRUSTED_KEY_DERIVE, 32, VAL_SUCCESS },
        { VAL_POLICY_KEY_DERIVE | VAL_POLICY_SHA256, 16, VAL_SUCCESS },
        { VAL_POLICY_KEY_DERIVE | VAL_POLICY_SHA256, 32, VAL_SUCCESS },
        { VAL_POLICY_KEY_DERIVE | VAL_POLICY_SHA256, 48, VAL_SUCCESS },
        { VAL_POLICY_KEY_DERIVE | VAL_POLICY_SHA256, 56, VAL_SUCCESS },
        { VAL_POLICY_KEY_DERIVE | VAL_POLICY_ALGO_CIPHER_AES | VAL_POLICY_CMAC, 16, VAL_SUCCESS },
        { VAL_POLICY_KEY_DERIVE | VAL_POLICY_ALGO_CIPHER_AES | VAL_POLICY_CMAC, 24, VAL_SUCCESS },
        { VAL_POLICY_KEY_DERIVE | VAL_POLICY_ALGO_CIPHER_AES | VAL_POLICY_CMAC, 32, VAL_SUCCESS },
        /* Derive invalid length. */
        { VAL_POLICY_TRUSTED_KEY_DERIVE, 8, VAL_INVALID_LENGTH },
        { VAL_POLICY_TRUSTED_KEY_DERIVE, 24, VAL_INVALID_LENGTH },
        { VAL_POLICY_TRUSTED_KEY_DERIVE, 48, VAL_INVALID_LENGTH },
        { VAL_POLICY_TRUSTED_KEY_DERIVE, 64, VAL_INVALID_LENGTH },
    };

    FOREACH(TestData, foreach_helper, TestData_Table)
    {
        ValPolicyMask_t AssetPolicy = TestData.Policy;

        LOG_WARN("Process vector %d\n", foreach_helper);

        if (!val_IsAccessSecure())
        {
            AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
        }
        Status = val_AssetAlloc(AssetPolicy, TestData.Size,
                                false, false,
                                VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                                &AssetId);
        fail_if(Status != TestData.Result, "val_AssetAlloc()=", Status);

        if (Status == VAL_SUCCESS)
        {
            // Free allocated asset to keep list of allocated assets clean.
            Status = val_AssetFree(AssetId);
            fail_if(Status != VAL_SUCCESS, "val_AssetFree()=", Status);
        }
    }

    if (valtest_StrictArgsCheck())
    {
        Status = val_AssetAlloc(VAL_POLICY_ALGO_CIPHER_AES | VAL_POLICY_ENCRYPT | VAL_POLICY_DECRYPT,
                                16,
                                false, false,
                                VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                                NULL);
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetAlloc()=", Status);

        Status = val_AssetAlloc(0, 0,
                                false, false,
                                VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                                &AssetId);
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetAlloc()=", Status);

        Status = val_AssetAlloc(0, 16,
                                false, false,
                                VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                                &AssetId);
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetAlloc()=", Status);

        Status = val_AssetAlloc(0, 32,
                                false, false,
                                VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                                &AssetId);
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetAlloc()=", Status);

        Status = val_AssetAlloc(VAL_POLICY_PK_RSA_PSS_SIGN | VAL_POLICY_SHA1,
                                VAL_ASSET_SIZE_MAX+1,
                                false, false,
                                VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                                &AssetId);
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetAlloc()=", Status);

        Status = val_AssetFree(VAL_ASSETID_INVALID);
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetFree(BadArgument1)=", Status);
    }
}
END_TEST


START_TEST(test_AssetSearch)
{
    ValAssetId_t AssetId;
    ValSize_t AssetSize;
    ValStatus_t Status;

    // Next two (monotonic counter) assets are always available
    Status = val_AssetSearch(32, &AssetId, &AssetSize);
    fail_if(Status != VAL_SUCCESS, "val_AssetSearch(32)=", Status);
    fail_if(AssetSize != (32/8), "val_AssetSearch(32)=", Status);

    Status = val_AssetFree(AssetId);
    fail_if(Status != VAL_INVALID_ASSET, "val_AssetFree()=", Status);

    Status = val_AssetSearch(33, &AssetId, NULL);
    fail_if(Status != VAL_SUCCESS, "val_AssetSearch(33)=", Status);

    // Test unlikely asset number
    Status = val_AssetSearch(53, &AssetId, &AssetSize);
    fail_if(((Status != VAL_INVALID_ASSET) && (Status != VAL_BAD_ARGUMENT)),
            "val_AssetSearch(53)=", Status);

    if (valtest_StrictArgsCheck())
    {
        Status = val_AssetSearch(32, NULL, NULL);
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetSearch(NULL)=", Status);
    }
}
END_TEST


START_TEST(test_AssetGetRootKey)
{
    ValAssetId_t AssetId;

    // Expect to find a root key
    AssetId = val_AssetGetRootKey();
    unsupported_if((AssetId == VAL_ASSETID_INVALID), "No Root key");
}
END_TEST


START_TEST(test_AssetLoadImport)
{
    uint8_t * AssociatedData_p;
    ValSize_t AssociatedDataSize = strlen(g_ValTestAssociatedData_p);
    uint8_t * KeyBlob_p;
    ValSize_t KeyBlobSize = 128;
    ValAssetId_t RootAssetId = VAL_ASSETID_INVALID;
    ValAssetId_t KEKAssetId = VAL_ASSETID_INVALID;
    ValAssetId_t AssetId;
    ValPolicyMask_t AssetPolicy;
    ValStatus_t Status;
    ValSize_t Size = 48;

    //  Get root key
    RootAssetId = val_AssetGetRootKey();
    unsupported_if((RootAssetId == VAL_ASSETID_INVALID), "No Root key");

    KeyBlob_p = (uint8_t *)SFZUTF_MALLOC(KeyBlobSize);
    fail_if(KeyBlob_p == NULL, "Allocation ", (int)KeyBlobSize);

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

    // Create an Asset for random initialization to generate a KeyBlob
    AssetPolicy = VAL_POLICY_SHA256 | VAL_POLICY_MAC_GENERATE | VAL_POLICY_MAC_VERIFY;
    if (!val_IsAccessSecure())
    {
        AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
    }
    Status = val_AssetAlloc(AssetPolicy, Size,
                            false, false,
                            VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                            &AssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(Key)=", Status);

    Status = val_AssetLoadRandomExport(AssetId, KEKAssetId,
                                       AssociatedData_p, AssociatedDataSize,
                                       KeyBlob_p, &KeyBlobSize);
    fail_if(Status != VAL_SUCCESS, "val_AssetLoadRandomExport(Key)=", Status);
    fail_if(KeyBlobSize != VAL_KEYBLOB_SIZE(Size), "KeyBlob size ", (int)KeyBlobSize);

    Status = val_AssetFree(AssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree(Asset)=", Status);

    // Create an Asset to import the KeyBlob again
    AssetPolicy = VAL_POLICY_SHA256 | VAL_POLICY_MAC_GENERATE | VAL_POLICY_MAC_VERIFY;
    if (!val_IsAccessSecure())
    {
        AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
    }
    Status = val_AssetAlloc(AssetPolicy, Size,
                            false, false,
                            VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                            &AssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(Asset)=", Status);

    KeyBlob_p[0] = (uint8_t)(KeyBlob_p[0] + 1);
    Status = val_AssetLoadImport(AssetId, KEKAssetId,
                                 AssociatedData_p, AssociatedDataSize,
                                 KeyBlob_p, KeyBlobSize);
    fail_if(Status != VAL_UNWRAP_ERROR, "val_AssetLoadImport(Key)=", Status);
    KeyBlob_p[0] = (uint8_t)(KeyBlob_p[0] - 1);

    if (!valtest_StrictArgsCheck())
    {
        Status = val_AssetLoadImport(AssetId, KEKAssetId,
                                     AssociatedData_p, (VAL_KDF_LABEL_MIN_SIZE - 1),
                                     KeyBlob_p, KeyBlobSize);
        fail_if(Status != VAL_UNWRAP_ERROR, "val_AssetLoadImport(Key)=", Status);
    }

    Status = val_AssetLoadImport(AssetId, KEKAssetId,
                                 AssociatedData_p, AssociatedDataSize,
                                 KeyBlob_p, KeyBlobSize);
    fail_if(Status != VAL_SUCCESS, "val_AssetLoadImport(Key)=", Status);

    // Load the already loaded Asset
    Status = val_AssetLoadImport(AssetId, KEKAssetId,
                                 AssociatedData_p, AssociatedDataSize,
                                 KeyBlob_p, KeyBlobSize);
    fail_if(Status != VAL_INVALID_LOCATION, "val_AssetLoadImport(Key)=", Status);

    Status = val_AssetFree(AssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree(Asset)=", Status);

    if (valtest_StrictArgsCheck())
    {
        AssetPolicy = VAL_POLICY_SHA256 | VAL_POLICY_MAC_GENERATE | VAL_POLICY_MAC_VERIFY;
        if (!val_IsAccessSecure())
        {
            AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
        }
        Status = val_AssetAlloc(AssetPolicy, Size,
                                false, false,
                                VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                                &AssetId);
        fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(Asset)=", Status);

        Status = val_AssetLoadImport(VAL_ASSETID_INVALID, KEKAssetId,
                                     AssociatedData_p, AssociatedDataSize,
                                     KeyBlob_p, KeyBlobSize);
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadImport(BadArgument1)=", Status);

        Status = val_AssetLoadImport(AssetId, VAL_ASSETID_INVALID,
                                     AssociatedData_p, AssociatedDataSize,
                                     KeyBlob_p, KeyBlobSize);
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadImport(BadArgument2)=", Status);

        Status = val_AssetLoadImport(AssetId, KEKAssetId,
                                     NULL, AssociatedDataSize,
                                     KeyBlob_p, KeyBlobSize);
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadImport(BadArgument3)=", Status);

        Status = val_AssetLoadImport(AssetId, KEKAssetId,
                                     AssociatedData_p, (VAL_KEYBLOB_AAD_MIN_SIZE - 1),
                                     KeyBlob_p, KeyBlobSize);
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadImport(BadArgument4L)=", Status);

        Status = val_AssetLoadImport(AssetId, KEKAssetId,
                                     AssociatedData_p, (VAL_KEYBLOB_AAD_MAX_SIZE + 1),
                                     KeyBlob_p, KeyBlobSize);
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadImport(BadArgument4H)=", Status);

        Status = val_AssetLoadImport(AssetId, KEKAssetId,
                                     AssociatedData_p, AssociatedDataSize,
                                     NULL, KeyBlobSize);
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadImport(BadArgument5)=", Status);

        Status = val_AssetLoadImport(AssetId, KEKAssetId,
                                     AssociatedData_p, AssociatedDataSize,
                                     KeyBlob_p, VAL_KEYBLOB_SIZE(VAL_ASSET_SIZE_MAX) + 1);
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadImport(BadArgument6)=", Status);

        Status = val_AssetFree(AssetId);
        fail_if(Status != VAL_SUCCESS, "val_AssetFree(Asset)=", Status);
    }

    // Release the involved Assets
    Status = val_AssetFree(KEKAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree(KEK)=", Status);

    SFZUTF_FREE(AssociatedData_p);
    SFZUTF_FREE(KeyBlob_p);
}
END_TEST


START_TEST(test_AssetLoadDerive)
{
    static const char * gl_SaltData_p = "Some Salt Data";
    static const char * gl_IVData_p = "Some IV Some IV Some IV Some IV ";
    uint8_t * AssociatedData_p;
    ValSize_t AssociatedDataSize = strlen(g_ValTestAssociatedData_p);
    uint8_t * SaltData_p;
    ValSize_t SaltDataSize = strlen(gl_SaltData_p);
    uint8_t * IVData_p;
    ValSize_t IVDataSize = strlen(gl_IVData_p);
    ValAssetId_t RootAssetId = VAL_ASSETID_INVALID;
    ValAssetId_t TrustedKDKAssetId = VAL_ASSETID_INVALID;
    ValAssetId_t KDKAssetId = VAL_ASSETID_INVALID;
    ValAssetId_t AssetId = VAL_ASSETID_INVALID;
    ValPolicyMask_t AssetPolicy;
    ValStatus_t Status;

    //  Get root key
    RootAssetId = val_AssetGetRootKey();
    unsupported_if((RootAssetId == VAL_ASSETID_INVALID), "No Root key");

    AssociatedData_p = (uint8_t *)SFZUTF_MALLOC(AssociatedDataSize);
    fail_if(AssociatedData_p == NULL, "Allocation ", (int)AssociatedDataSize);
    memcpy(AssociatedData_p, g_ValTestAssociatedData_p, AssociatedDataSize);

    SaltData_p = (uint8_t *)SFZUTF_MALLOC(SaltDataSize);
    fail_if(SaltData_p == NULL, "Allocation ", (int)SaltDataSize);
    memcpy(SaltData_p, gl_SaltData_p, SaltDataSize);

    IVData_p = (uint8_t *)SFZUTF_MALLOC(IVDataSize);
    fail_if(IVData_p == NULL, "Allocation ", (int)IVDataSize);
    memcpy(IVData_p, gl_IVData_p, IVDataSize);

    // Create and derive a Trusted KDK Asset (SP800-108 feedback mode)
    AssetPolicy = VAL_POLICY_TRUSTED_KEY_DERIVE;
    if (!val_IsAccessSecure())
    {
        AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
    }
    Status = val_AssetAlloc(AssetPolicy, 32,
                            false, false,
                            VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                            &TrustedKDKAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(Trusted KDK)=", Status);

    Status = val_AssetLoadDerive(TrustedKDKAssetId, RootAssetId,
                                 AssociatedData_p, AssociatedDataSize,
                                 false, false, NULL, 0, NULL, 0);
    fail_if(Status != VAL_SUCCESS, "val_AssetLoadDerive(Trusted KDK)=", Status);

    // ------ TrustedKDK1
    // Create and derive a Trusted KDK Asset (1 - SP800-108 counter mode)
    AssetPolicy = VAL_POLICY_TRUSTED_KEY_DERIVE;
    if (!val_IsAccessSecure())
    {
        AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
    }
    Status = val_AssetAlloc(AssetPolicy, 32,
                            false, false,
                            VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                            &AssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(TrustedKDK1)=", Status);

    // Invalid associated data size
    if (!valtest_StrictArgsCheck())
    {
        Status = val_AssetLoadDerive(AssetId, RootAssetId,
                                     AssociatedData_p, (VAL_KDF_LABEL_MIN_SIZE - 1),
                                     true, false, NULL, 0, NULL, 0);
        fail_if(Status != VAL_INVALID_LENGTH, "val_AssetLoadDerive(TrustedKDK1)=", Status);
    }

    Status = val_AssetLoadDerive(AssetId, RootAssetId,
                                 AssociatedData_p, AssociatedDataSize,
                                 true, false, NULL, 0, NULL, 0);
    fail_if(Status != VAL_SUCCESS, "val_AssetLoadDerive(TrustedKDK1)=", Status);

    Status = val_AssetFree(AssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree(TrustedKDK1)=", Status);

    // ------
    // Create and derive KDK Asset (SP800-108 feedback mode)
    AssetPolicy = VAL_POLICY_KEY_DERIVE | VAL_POLICY_SHA256;
    if (!val_IsAccessSecure())
    {
        AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
    }
    Status = val_AssetAlloc(AssetPolicy, 64,
                            false, false,
                            VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                            &KDKAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(KDK)=", Status);

    Status = val_AssetLoadDerive(KDKAssetId, TrustedKDKAssetId,
                                 AssociatedData_p, AssociatedDataSize,
                                 false, false, NULL, 0, NULL, 0);
    fail_if(Status != VAL_SUCCESS, "val_AssetLoadDerive(KDK)=", Status);

    // ------ Asset1
    // Create and derive KDK Asset (SP800-108 counter mode)
    AssetPolicy = VAL_POLICY_KEY_DERIVE | VAL_POLICY_SHA256;
    if (!val_IsAccessSecure())
    {
        AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
    }
    Status = val_AssetAlloc(AssetPolicy, 32,
                            false, false,
                            VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                            &AssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(Asset1)=", Status);

    Status = val_AssetLoadDerive(AssetId, KDKAssetId,
                                 AssociatedData_p, AssociatedDataSize,
                                 true, false, NULL, 0, NULL, 0);
    fail_if(Status != VAL_SUCCESS, "val_AssetLoadDerive(Asset1)=", Status);

    Status = val_AssetFree(AssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree(Asset1)=", Status);

    // ------ Asset2
    // Create and derive KDK Asset (RFC5869)
    AssetPolicy = VAL_POLICY_KEY_DERIVE | VAL_POLICY_SHA256;
    if (!val_IsAccessSecure())
    {
        AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
    }
    Status = val_AssetAlloc(AssetPolicy, 32,
                            false, false,
                            VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                            &AssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(Asset2)=", Status);

    Status = val_AssetLoadDerive(AssetId, KDKAssetId,
                                 AssociatedData_p, AssociatedDataSize,
                                 false, true, NULL, 0, NULL, 0);
    fail_if(Status != VAL_SUCCESS, "val_AssetLoadDerive(Asset2)=", Status);

    Status = val_AssetFree(AssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree(Asset2)=", Status);

    // ------ Asset3
    // Create and derive KDK Asset
    // (SP800-108 feedmode mode with Randomness Extraction (salt))
    AssetPolicy = VAL_POLICY_KEY_DERIVE | VAL_POLICY_SHA256;
    if (!val_IsAccessSecure())
    {
        AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
    }
    Status = val_AssetAlloc(AssetPolicy, 32,
                            false, false,
                            VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                            &AssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(Asset3)=", Status);

    Status = val_AssetLoadDerive(AssetId, KDKAssetId,
                                 AssociatedData_p, AssociatedDataSize,
                                 false, false,
                                 SaltData_p, SaltDataSize,
                                 NULL, 0);
    fail_if(Status != VAL_SUCCESS, "val_AssetLoadDerive(Asset3)=", Status);

    Status = val_AssetFree(AssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree(Asset3)=", Status);

    // ------ Asset4
    // Create and derive KDK Asset
    // (SP800-108 feedmode mode with Randomness Extraction (salt) and
    //  IV for Key Expansion)
    AssetPolicy = VAL_POLICY_KEY_DERIVE | VAL_POLICY_SHA256;
    if (!val_IsAccessSecure())
    {
        AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
    }
    Status = val_AssetAlloc(AssetPolicy, 32,
                            false, false,
                            VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                            &AssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(Asset4)=", Status);

    Status = val_AssetLoadDerive(AssetId, KDKAssetId,
                                 AssociatedData_p, AssociatedDataSize,
                                 false, false,
                                 SaltData_p, SaltDataSize,
                                 IVData_p, IVDataSize);
    fail_if(Status != VAL_SUCCESS, "val_AssetLoadDerive(Asset4)=", Status);

    Status = val_AssetFree(AssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree(Asset4)=", Status);

    if (valtest_StrictArgsCheck())
    {
        AssetPolicy = VAL_POLICY_KEY_DERIVE | VAL_POLICY_SHA256;
        if (!val_IsAccessSecure())
        {
            AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
        }
        Status = val_AssetAlloc(AssetPolicy, 32,
                                false, false,
                                VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                                &AssetId);
        fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(Asset)=", Status);

        Status = val_AssetLoadDerive(VAL_ASSETID_INVALID, KDKAssetId,
                                     AssociatedData_p, AssociatedDataSize,
                                     false, false,
                                     SaltData_p, SaltDataSize,
                                     IVData_p, IVDataSize);
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadDerive(BadArgument1)=", Status);

        Status = val_AssetLoadDerive(AssetId, VAL_ASSETID_INVALID,
                                     AssociatedData_p, AssociatedDataSize,
                                     false, false,
                                     SaltData_p, SaltDataSize,
                                     IVData_p, IVDataSize);
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadDerive(BadArgument2)=", Status);

        Status = val_AssetLoadDerive(AssetId, KDKAssetId,
                                     NULL, AssociatedDataSize,
                                     false, false,
                                     SaltData_p, SaltDataSize,
                                     IVData_p, IVDataSize);
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadDerive(BadArgument3)=", Status);

        Status = val_AssetLoadDerive(AssetId, KDKAssetId,
                                     AssociatedData_p, (VAL_KDF_LABEL_MIN_SIZE - 1),
                                     false, false,
                                     SaltData_p, SaltDataSize,
                                     IVData_p, IVDataSize);
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadDerive(BadArgument4L)=", Status);

        Status = val_AssetLoadDerive(AssetId, KDKAssetId,
                                     AssociatedData_p, (VAL_KDF_LABEL_MAX_SIZE + 1),
                                     false, false,
                                     SaltData_p, SaltDataSize,
                                     IVData_p, IVDataSize);
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadDerive(BadArgument4H)=", Status);

        Status = val_AssetLoadDerive(AssetId, KDKAssetId,
                                     AssociatedData_p, AssociatedDataSize,
                                     true, true,
                                     SaltData_p, SaltDataSize,
                                     IVData_p, IVDataSize);
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadDerive(BadArgument5&6)=", Status);

        Status = val_AssetFree(AssetId);
        fail_if(Status != VAL_SUCCESS, "val_AssetFree(Asset)=", Status);
    }

    // ======
    // Release the KDK Asset
    Status = val_AssetFree(KDKAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree(KDK)=", Status);

    // Release the Trusted KDK Asset
    Status = val_AssetFree(TrustedKDKAssetId);
    fail_if(Status != VAL_SUCCESS, "val_AssetFree(Trusted KDK)=", Status);

    SFZUTF_FREE(AssociatedData_p);
    SFZUTF_FREE(SaltData_p);
    SFZUTF_FREE(IVData_p);
}
END_TEST


START_TEST(test_AssetLoadPlaintext)
{
    static const ValSize_t Sizes[] = { 16, 24, 32, 48, 64, 0};
    uint8_t * AssociatedData_p;
    ValSize_t AssociatedDataSize = strlen(g_ValTestAssociatedData_p);
    uint8_t * KeyData_p;
    uint8_t * KeyBlob_p;
    ValSize_t KeyBlobSize = 500;
    ValSize_t Size;
    ValSize_t MaxSize = 0;
    ValAssetId_t AssetId = VAL_ASSETID_INVALID;
    ValAssetId_t RootAssetId = VAL_ASSETID_INVALID;
    ValAssetId_t KEKAssetId = VAL_ASSETID_INVALID;
    ValPolicyMask_t AssetPolicy;
    ValStatus_t Status;
    unsigned int ndx;

    if (_i & 1)
    {
        //  Get root key
        RootAssetId = val_AssetGetRootKey();
        unsupported_if((RootAssetId == VAL_ASSETID_INVALID), "No Root key");
    }

    // Prepare key data
    for (ndx = 0; ; ndx++)
    {
        Size = Sizes[ndx];
        if (Size == 0)
        {
            break;
        }
        if (MaxSize < Size)
        {
            MaxSize = Size;
        }
    }
    KeyData_p = (uint8_t *)SFZUTF_MALLOC(MaxSize);
    fail_if(KeyData_p == NULL, "Allocation ", (int)MaxSize);
    for (ndx = 0; ndx < MaxSize; ndx++)
    {
        KeyData_p[ndx] = (uint8_t)(ndx +1);
    }

    AssociatedData_p = (uint8_t *)SFZUTF_MALLOC(AssociatedDataSize);
    fail_if(AssociatedData_p == NULL, "Allocation ", (int)AssociatedDataSize);
    memcpy(AssociatedData_p, g_ValTestAssociatedData_p, AssociatedDataSize);

    // Run tests
    if (RootAssetId != VAL_ASSETID_INVALID)
    {
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

    KeyBlob_p = (uint8_t *)SFZUTF_MALLOC(KeyBlobSize);
    fail_if(KeyBlob_p == NULL, "Allocation ", (int)KeyBlobSize);

    for (ndx = 0; ; ndx++)
    {
        Size = Sizes[ndx];
        if (Size == 0)
        {
            break;
        }

        LOG_INFO("Process vector %d\n", ndx);

        // Create an Asset for random initialization
        if ((Size == 16) || (Size == 24) || (Size == 32))
        {
            AssetPolicy = VAL_POLICY_ALGO_CIPHER_AES | VAL_POLICY_AES_MODE_CBC | VAL_POLICY_ENCRYPT | VAL_POLICY_DECRYPT;
        }
        else
        {
            AssetPolicy = VAL_POLICY_SHA256 | VAL_POLICY_MAC_GENERATE | VAL_POLICY_MAC_VERIFY;
        }
        if (!val_IsAccessSecure())
        {
            AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
        }
        Status = val_AssetAlloc(AssetPolicy, Size,
                                false, false,
                                VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                                &AssetId);
        fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(Key)=", Status);

        if (KEKAssetId == VAL_ASSETID_INVALID)
        {
            Status = val_AssetLoadPlaintext(AssetId, KeyData_p, Size);
            fail_if(Status != VAL_SUCCESS, "val_AssetLoadPlaintext(Key)=", Status);

            // Try to load a second time
            Status = val_AssetLoadPlaintext(AssetId, KeyData_p, Size);
            fail_if(Status != VAL_INVALID_LOCATION, "val_AssetLoadPlaintext(Key)=", Status);
        }
        else
        {
            KeyBlobSize = VAL_KEYBLOB_SIZE(Size) + 4;
            Status = val_AssetLoadPlaintextExport(AssetId,
                                                  KeyData_p, Size,
                                                  KEKAssetId,
                                                  AssociatedData_p, AssociatedDataSize,
                                                  KeyBlob_p, &KeyBlobSize);
            fail_if(Status != VAL_SUCCESS, "val_AssetLoadPlaintextExport(Key)=", Status);
            fail_if(KeyBlobSize != VAL_KEYBLOB_SIZE(Size), "KeyBlob size ", (int)KeyBlobSize);

            // Try to load a second time
            Status = val_AssetLoadPlaintextExport(AssetId,
                                                  KeyData_p, Size,
                                                  KEKAssetId,
                                                  AssociatedData_p, AssociatedDataSize,
                                                  KeyBlob_p, &KeyBlobSize);
            fail_if(Status != VAL_INVALID_LOCATION, "val_AssetLoadPlaintextExport(Key)=", Status);
        }

        // Release the involved Asset
        Status = val_AssetFree(AssetId);
        fail_if(Status != VAL_SUCCESS, "val_AssetFree(Asset)=", Status);
    }

    if (valtest_StrictArgsCheck())
    {
        Size = 32;
        AssetPolicy = VAL_POLICY_ALGO_CIPHER_AES | VAL_POLICY_AES_MODE_CBC | VAL_POLICY_ENCRYPT | VAL_POLICY_DECRYPT;
        if (!val_IsAccessSecure())
        {
            AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
        }
        Status = val_AssetAlloc(AssetPolicy, Size,
                                false, false,
                                VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                                &AssetId);
        fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(Asset)=", Status);

        Status = val_AssetLoadPlaintext(VAL_ASSETID_INVALID, KeyData_p, 32);
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadPlaintext(BadArgument1)=", Status);

        Status = val_AssetLoadPlaintext(AssetId, NULL, 32);
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadPlaintext(BadArgument2)=", Status);

        Status = val_AssetLoadPlaintext(AssetId, KeyData_p, 0);
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadPlaintext(BadArgument3L)=", Status);

        Status = val_AssetLoadPlaintext(AssetId, KeyData_p, (VAL_ASSET_SIZE_MAX + 1));
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadPlaintext(BadArgument3H)=", Status);

        if (KEKAssetId != VAL_ASSETID_INVALID)
        {
            Status = val_AssetLoadPlaintextExport(VAL_ASSETID_INVALID,
                                                  KeyData_p, Size,
                                                  KEKAssetId,
                                                  AssociatedData_p, AssociatedDataSize,
                                                  KeyBlob_p, &KeyBlobSize);
            fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadPlaintextExport(BadArgument1)=", Status);

            Status = val_AssetLoadPlaintextExport(AssetId,
                                                  NULL, Size,
                                                  KEKAssetId,
                                                  AssociatedData_p, AssociatedDataSize,
                                                  KeyBlob_p, &KeyBlobSize);
            fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadPlaintextExport(BadArgument2)=", Status);

            Status = val_AssetLoadPlaintextExport(AssetId,
                                                  KeyData_p, 0,
                                                  KEKAssetId,
                                                  AssociatedData_p, AssociatedDataSize,
                                                  KeyBlob_p, &KeyBlobSize);
            fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadPlaintextExport(BadArgument3L)=", Status);

            Status = val_AssetLoadPlaintextExport(AssetId,
                                                  KeyData_p, (VAL_ASSET_SIZE_MAX + 1),
                                                  KEKAssetId,
                                                  AssociatedData_p, AssociatedDataSize,
                                                  KeyBlob_p, &KeyBlobSize);
            fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadPlaintextExport(BadArgument3H)=", Status);

            Status = val_AssetLoadPlaintextExport(AssetId,
                                                  KeyData_p, Size,
                                                  VAL_ASSETID_INVALID,
                                                  AssociatedData_p, AssociatedDataSize,
                                                  KeyBlob_p, &KeyBlobSize);
            fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadPlaintextExport(BadArgument4)=", Status);

            Status = val_AssetLoadPlaintextExport(AssetId,
                                                  KeyData_p, Size,
                                                  KEKAssetId,
                                                  NULL, AssociatedDataSize,
                                                  KeyBlob_p, &KeyBlobSize);
            fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadPlaintextExport(BadArgument5)=", Status);

            Status = val_AssetLoadPlaintextExport(AssetId,
                                                  KeyData_p, Size,
                                                  KEKAssetId,
                                                  AssociatedData_p, (VAL_KEYBLOB_AAD_MIN_SIZE - 1),
                                                  KeyBlob_p, &KeyBlobSize);
            fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadPlaintextExport(BadArgument6L)=", Status);

            Status = val_AssetLoadPlaintextExport(AssetId,
                                                  KeyData_p, Size,
                                                  KEKAssetId,
                                                  AssociatedData_p, (VAL_KEYBLOB_AAD_MAX_SIZE + 1),
                                                  KeyBlob_p, &KeyBlobSize);
            fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadPlaintextExport(BadArgument6H)=", Status);

            Status = val_AssetLoadPlaintextExport(AssetId,
                                                  KeyData_p, Size,
                                                  KEKAssetId,
                                                  AssociatedData_p, AssociatedDataSize,
                                                  NULL, &KeyBlobSize);
            fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadPlaintextExport(BadArgument7)=", Status);

            Status = val_AssetLoadPlaintextExport(AssetId,
                                                  KeyData_p, Size,
                                                  KEKAssetId,
                                                  AssociatedData_p, AssociatedDataSize,
                                                  KeyBlob_p, NULL);
            fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadPlaintextExport(BadArgument8)=", Status);

            KeyBlobSize = (Size + (128 / 8)) - 1;
            Status = val_AssetLoadPlaintextExport(AssetId,
                                                  KeyData_p, Size,
                                                  KEKAssetId,
                                                  AssociatedData_p, AssociatedDataSize,
                                                  KeyBlob_p, &KeyBlobSize);
            fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadPlaintextExport(BadArgument8H)=", Status);
        }

        Status = val_AssetFree(AssetId);
        fail_if(Status != VAL_SUCCESS, "val_AssetFree(Asset)=", Status);
    }

    SFZUTF_FREE(KeyBlob_p);
    SFZUTF_FREE(KeyData_p);
    SFZUTF_FREE(AssociatedData_p);

    // Release the involved Assets
    if (KEKAssetId != VAL_ASSETID_INVALID)
    {
        Status = val_AssetFree(KEKAssetId);
        fail_if(Status != VAL_SUCCESS, "val_AssetFree(KEK)=", Status);
    }
}
END_TEST


START_TEST(test_AssetLoadRandom)
{
    static const ValSize_t Sizes[] = { 16, 24, 32, 48, 64, 0};
    ValSize_t Size;
    uint8_t * AssociatedData_p;
    ValSize_t AssociatedDataSize = strlen(g_ValTestAssociatedData_p);
    uint8_t * KeyBlob_p;
    ValSize_t KeyBlobSize = 100;
    ValAssetId_t AssetId = VAL_ASSETID_INVALID;
    ValAssetId_t RootAssetId = VAL_ASSETID_INVALID;
    ValAssetId_t KEKAssetId = VAL_ASSETID_INVALID;
    ValPolicyMask_t AssetPolicy;
    ValStatus_t Status;
    int ndx;

    unsupported_unless(valtest_IsTrngActive(true), "TRNG not activated");

    if (_i & 1)
    {
        //  Get root key
        RootAssetId = val_AssetGetRootKey();
        unsupported_if((RootAssetId == VAL_ASSETID_INVALID), "No Root key");
    }

    AssociatedData_p = (uint8_t *)SFZUTF_MALLOC(AssociatedDataSize);
    fail_if(AssociatedData_p == NULL, "Allocation ", (int)AssociatedDataSize);
    memcpy(AssociatedData_p, g_ValTestAssociatedData_p, AssociatedDataSize);

    if (RootAssetId != VAL_ASSETID_INVALID)
    {
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

    KeyBlob_p = (uint8_t *)SFZUTF_MALLOC(KeyBlobSize);
    fail_if(KeyBlob_p == NULL, "Allocation ", (int)KeyBlobSize);

    for (ndx = 0; ; ndx++)
    {
        Size = Sizes[ndx];
        if (Size == 0)
        {
            break;
        }

        LOG_INFO("Process vector %d\n", ndx);

        // Create an Asset for random initialization
        if ((Size == 16) || (Size == 24) || (Size == 32))
        {
            AssetPolicy = VAL_POLICY_ALGO_CIPHER_AES | VAL_POLICY_AES_MODE_CBC | VAL_POLICY_ENCRYPT | VAL_POLICY_DECRYPT;
        }
        else
        {
            AssetPolicy = VAL_POLICY_SHA256 | VAL_POLICY_MAC_GENERATE | VAL_POLICY_MAC_VERIFY;
        }
        if (!val_IsAccessSecure())
        {
            AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
        }
        Status = val_AssetAlloc(AssetPolicy, Size,
                                false, false,
                                VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                                &AssetId);
        fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(Key)=", Status);

        if (KEKAssetId == VAL_ASSETID_INVALID)
        {
            Status = val_AssetLoadRandom(AssetId);
            fail_if(Status != VAL_SUCCESS, "val_AssetLoadRandom(Key)=", Status);

            // Try to load a second time
            Status = val_AssetLoadRandom(AssetId);
            fail_if(Status != VAL_INVALID_LOCATION, "val_AssetLoadRandomExport(Key)=", Status);
        }
        else
        {
            KeyBlobSize = VAL_KEYBLOB_SIZE(Size) + 4;
            Status = val_AssetLoadRandomExport(AssetId, KEKAssetId,
                                               AssociatedData_p, AssociatedDataSize,
                                               KeyBlob_p, &KeyBlobSize);
            fail_if(Status != VAL_SUCCESS, "val_AssetLoadRandomExport(Key)=", Status);
            fail_if(KeyBlobSize != VAL_KEYBLOB_SIZE(Size), "KeyBlob size ", (int)KeyBlobSize);

            // Try to load a second time
            Status = val_AssetLoadRandomExport(AssetId, KEKAssetId,
                                               AssociatedData_p, AssociatedDataSize,
                                               KeyBlob_p, &KeyBlobSize);
            fail_if(Status != VAL_INVALID_LOCATION, "val_AssetLoadRandomExport(Key)=", Status);
        }

        // Release the involved Asset
        Status = val_AssetFree(AssetId);
        fail_if(Status != VAL_SUCCESS, "val_AssetFree(Asset)=", Status);
    }

    if (valtest_StrictArgsCheck())
    {
        Size = 32;
        AssetPolicy = VAL_POLICY_ALGO_CIPHER_AES | VAL_POLICY_AES_MODE_CBC | VAL_POLICY_ENCRYPT | VAL_POLICY_DECRYPT;
        if (!val_IsAccessSecure())
        {
            AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
        }
        Status = val_AssetAlloc(AssetPolicy, Size,
                                false, false,
                                VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                                &AssetId);
        fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(Asset)=", Status);

        Status = val_AssetLoadRandom(VAL_ASSETID_INVALID);
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadRandom(BadArgument1)=", Status);

        if (KEKAssetId != VAL_ASSETID_INVALID)
        {
            Status = val_AssetLoadRandomExport(VAL_ASSETID_INVALID, KEKAssetId,
                                               AssociatedData_p, AssociatedDataSize,
                                               KeyBlob_p, &KeyBlobSize);
            fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadRandomExport(BadArgument1)=", Status);

            Status = val_AssetLoadRandomExport(AssetId, VAL_ASSETID_INVALID,
                                               AssociatedData_p, AssociatedDataSize,
                                               KeyBlob_p, &KeyBlobSize);
            fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadRandomExport(BadArgument2)=", Status);

            Status = val_AssetLoadRandomExport(AssetId, KEKAssetId,
                                               NULL, AssociatedDataSize,
                                               KeyBlob_p, &KeyBlobSize);
            fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadRandomExport(BadArgument3)=", Status);

            Status = val_AssetLoadRandomExport(AssetId, KEKAssetId,
                                               AssociatedData_p, (VAL_KEYBLOB_AAD_MIN_SIZE - 1),
                                               KeyBlob_p, &KeyBlobSize);
            fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadRandomExport(BadArgument4L)=", Status);

            Status = val_AssetLoadRandomExport(AssetId, KEKAssetId,
                                               AssociatedData_p, (VAL_KEYBLOB_AAD_MAX_SIZE + 1),
                                               KeyBlob_p, &KeyBlobSize);
            fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadRandomExport(BadArgument4H)=", Status);

            Status = val_AssetLoadRandomExport(AssetId, KEKAssetId,
                                               AssociatedData_p, AssociatedDataSize,
                                               NULL, &KeyBlobSize);
            fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadRandomExport(BadArgument5)=", Status);

            Status = val_AssetLoadRandomExport(AssetId, KEKAssetId,
                                               AssociatedData_p, AssociatedDataSize,
                                               KeyBlob_p, NULL);
            fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadRandomExport(BadArgument6)=", Status);

            KeyBlobSize = (128 / 8) - 1;
            Status = val_AssetLoadRandomExport(AssetId, KEKAssetId,
                                               AssociatedData_p, AssociatedDataSize,
                                               KeyBlob_p, &KeyBlobSize);
            fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadRandomExport(BadArgument6H)=", Status);
        }

        Status = val_AssetFree(AssetId);
        fail_if(Status != VAL_SUCCESS, "val_AssetFree(Asset)=", Status);
    }

    SFZUTF_FREE(KeyBlob_p);
    SFZUTF_FREE(AssociatedData_p);

    // Release the involved Assets
    if (KEKAssetId != VAL_ASSETID_INVALID)
    {
        Status = val_AssetFree(KEKAssetId);
        fail_if(Status != VAL_SUCCESS, "val_AssetFree(KEK)=", Status);
    }
}
END_TEST

START_TEST(test_AssetLoadAesunwrap)
{
    uint8_t * KekData_p;
    uint8_t * KeyData_p = NULL;
    ValAssetId_t KekAssetId = VAL_ASSETID_INVALID;
    ValAssetId_t KeyAssetId = VAL_ASSETID_INVALID;
    ValSize_t Size;
    ValPolicyMask_t AssetPolicy;
    ValStatus_t Status;

    int ndx;

    for (ndx = 0; ; ndx++)
    {
        TestVector_AES_WRAP_t tv_p;

        tv_p = test_vectors_aes_wrap_get(ndx);
        if (tv_p == NULL)
        {
            break;
        }

        LOG_INFO("Process vector %d\n", ndx);

        // Create and initialize KEK Asset
        AssetPolicy = VAL_POLICY_AES_WRAP | VAL_POLICY_DECRYPT;
        if (!val_IsAccessSecure())
        {
            AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
        }
        Status = val_AssetAlloc(AssetPolicy, tv_p->KeyLen,
                                false, false,
                                VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                                &KekAssetId);
        fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(KEK)=", Status);

        KekData_p = (uint8_t *)SFZUTF_MALLOC(tv_p->KeyLen);
        fail_if(KekData_p == NULL, "Allocation ", (int)tv_p->KeyLen);
        memcpy(KekData_p, tv_p->WrapKey_p, tv_p->KeyLen);

        Status = val_AssetLoadPlaintext(KekAssetId, KekData_p, tv_p->KeyLen);
        fail_if(Status != VAL_SUCCESS, "val_AssetLoadPlaintext(KEK)=", Status);

        SFZUTF_FREE(KekData_p);

        // Create an Asset for the AES wrapped data
        if ((tv_p->PlainTxtLen == 16) || (tv_p->PlainTxtLen == 24) || (tv_p->PlainTxtLen == 32))
        {
            AssetPolicy = VAL_POLICY_ALGO_CIPHER_AES | VAL_POLICY_AES_MODE_CBC | VAL_POLICY_ENCRYPT | VAL_POLICY_DECRYPT;
        }
        else if ((tv_p->PlainTxtLen >= (160/8)) && (tv_p->PlainTxtLen <= 64))
        {
            AssetPolicy = VAL_POLICY_SHA1 | VAL_POLICY_MAC_GENERATE | VAL_POLICY_MAC_VERIFY;
        }
        else
        {
            AssetPolicy = VAL_POLICY_PK_ECC_ECDSA_SIGN;
        }
        if (!val_IsAccessSecure())
        {
            AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
        }
        Status = val_AssetAlloc(AssetPolicy, tv_p->PlainTxtLen,
                                false, false,
                                VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                                &KeyAssetId);
        fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(Key)=", Status);

        KeyData_p = (uint8_t *)SFZUTF_MALLOC((tv_p->PlainTxtLen + (64/8)));
        fail_if(KeyData_p == NULL, "Allocation ", (int)(tv_p->PlainTxtLen + (64/8)));
        memcpy(KeyData_p, tv_p->WrappedTxt_p, (tv_p->PlainTxtLen + (64/8)));

        // Initialize the Asset with a corrupt keyblob through an AES unwrap operation
        KeyData_p[4] = (uint8_t)(KeyData_p[4] + 1);
        Status = val_AssetLoadAesunwrap(KeyAssetId, KekAssetId, KeyData_p, (tv_p->PlainTxtLen + (64/8)));
        fail_if(Status != VAL_UNWRAP_ERROR, "val_AssetLoadAesunwrap(Key)=", Status);
        KeyData_p[4] = (uint8_t)(KeyData_p[4] - 1);

        // Keyblob too small
        Status = val_AssetLoadAesunwrap(KeyAssetId, KekAssetId, KeyData_p, (tv_p->PlainTxtLen + (32/8)));
        fail_if(Status != VAL_INVALID_ASSET, "val_AssetLoadAesunwrap(Key)=", Status);

        // Initialize the Asset through an AES unwrap operation
        Status = val_AssetLoadAesunwrap(KeyAssetId, KekAssetId, KeyData_p, (tv_p->PlainTxtLen + (64/8)));
        fail_if(Status != VAL_SUCCESS, "val_AssetLoadAesunwrap(Key)=", Status);

        SFZUTF_FREE(KeyData_p);

        // Release the involved Assets
        Status = val_AssetFree(KeyAssetId);
        fail_if(Status != VAL_SUCCESS, "val_AssetFree(Key)=", Status);

        Status = val_AssetFree(KekAssetId);
        fail_if(Status != VAL_SUCCESS, "val_AssetFree(KEK)=", Status);

        test_vectors_aes_wrap_release(tv_p);
    }

    if (valtest_StrictArgsCheck())
    {
        Size = 32;
        Status = val_AssetLoadAesunwrap(VAL_ASSETID_INVALID, KekAssetId, KeyData_p, (Size + (64/8)));
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadRandom(BadArgument1)=", Status);

        Status = val_AssetLoadAesunwrap(KeyAssetId, VAL_ASSETID_INVALID, KeyData_p, (Size + (64/8)));
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadRandomExport(BadArgument2)=", Status);

        Status = val_AssetLoadAesunwrap(KeyAssetId, KekAssetId, NULL, (Size + (64/8)));
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadRandomExport(BadArgument3)=", Status);

        Status = val_AssetLoadAesunwrap(KeyAssetId, KekAssetId, KeyData_p, (VAL_ASSET_SIZE_MAX + (64/8) +1));
        fail_if(Status != VAL_BAD_ARGUMENT, "val_AssetLoadRandomExport(BadArgument4H)=", Status);
    }
}
END_TEST


int
suite_add_test_AssetManagement(
        struct TestSuite * TestSuite_p)
{
    struct TestCase * TestCase_p;

    TestCase_p = sfzutf_tcase_create(TestSuite_p, "AssetManagement_Tests");
    if (TestCase_p != NULL)
    {
        if (sfzutf_tcase_add_fixture(TestCase_p, valtest_initialize, valtest_terminate) != 0)
        {
             goto FuncErrorReturn;
        }

        if (sfzutf_test_add(TestCase_p, test_AssetGetRootKey) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add(TestCase_p, test_AssetSearch) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add(TestCase_p, test_AssetAllocFree) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add(TestCase_p, test_AssetLoadDerive) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add_loop(TestCase_p, test_AssetLoadPlaintext, 2) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add_loop(TestCase_p, test_AssetLoadRandom, 2) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add(TestCase_p, test_AssetLoadImport) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add(TestCase_p, test_AssetLoadAesunwrap) != 0) goto FuncErrorReturn;

        return 0;
    }

FuncErrorReturn:
    return -1;
}

/* end of file valtest_asset.c */
