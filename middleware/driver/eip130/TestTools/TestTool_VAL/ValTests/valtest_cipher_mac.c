/* valtest_cipher_mac.c
 *
 * Description: VAL Test Suite; Symmetric Crypto functionality
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

/* Test vectors. */
#include "testvectors_aes_cmac.h"
#include "testvectors_aes_cbcmac.h"
#ifdef VALTEST_SYM_ALGO_ARIA
#include "testvectors_aria_cmac.h"
#include "testvectors_aria_cbcmac.h"
#endif
#ifdef VALTEST_SYM_ALGO_POLY1305
#include "testvectors_poly1305.h"
#endif


typedef struct
{
    int Index;
    ValSymContextPtr_t SymContext_p;
    ValAssetId_t KeyAssetId;
    uint8_t * Msg_p;
    uint32_t MsgLen;
    uint8_t * Mac_p;
    uint32_t MacLen;
    uint32_t BlockSize;
    bool fPadding;
    uint32_t options;
} SymCryptTestCtx_t;


/*----------------------------------------------------------------------------
 * SetupTestContextAndEnvironment
 */
static int
SetupTestContextAndEnvironment(
        SymCryptTestCtx_t * TestCntxt_p,
        int Index,
        ValSymAlgo_t Algorithm,
        bool fVerify,
        bool fKeyAsset,
        bool fUseTokenTemp,
        const uint8_t * const Key_p,
        uint32_t KeySize,
        const uint8_t * const Iv_p,
        const uint8_t * const Msg_p,
        const uint32_t MsgLen,
        const uint8_t * const Mac_p,
        const uint32_t MacLen)
{
    ValPolicyMask_t KeyAssetPolicy = 0;
    ValStatus_t Status;
    uint32_t IvSize = 0;

    memset(TestCntxt_p, 0, sizeof(SymCryptTestCtx_t));

    switch (Algorithm)
    {
    case VAL_SYM_ALGO_MAC_AES_CMAC:
        KeyAssetPolicy = VAL_POLICY_ALGO_CIPHER_AES|VAL_POLICY_CMAC;
        TestCntxt_p->BlockSize = VAL_SYM_ALGO_AES_BLOCK_SIZE;
        break;

    case VAL_SYM_ALGO_MAC_AES_CBC_MAC:
        KeyAssetPolicy = VAL_POLICY_ALGO_CIPHER_AES|VAL_POLICY_AES_MODE_CBC|VAL_POLICY_ENCRYPT;
        TestCntxt_p->BlockSize = VAL_SYM_ALGO_AES_BLOCK_SIZE;
        IvSize = VAL_SYM_ALGO_AES_IV_SIZE;
        break;

#ifdef VALTEST_SYM_ALGO_ARIA
    case VAL_SYM_ALGO_MAC_ARIA_CMAC:
        KeyAssetPolicy = VAL_POLICY_ALGO_CIPHER_ARIA|VAL_POLICY_CMAC;
        TestCntxt_p->BlockSize = VAL_SYM_ALGO_ARIA_BLOCK_SIZE;
        break;

    case VAL_SYM_ALGO_MAC_ARIA_CBC_MAC:
        KeyAssetPolicy = VAL_POLICY_ALGO_CIPHER_ARIA|VAL_POLICY_ARIA_MODE_CBC|VAL_POLICY_ENCRYPT;
        TestCntxt_p->BlockSize = VAL_SYM_ALGO_ARIA_BLOCK_SIZE;
        IvSize = VAL_SYM_ALGO_ARIA_IV_SIZE;
        break;
#endif

#ifdef VALTEST_SYM_ALGO_POLY1305
    case VAL_SYM_ALGO_MAC_POLY1305:
        KeyAssetPolicy = VAL_POLICY_POLY1305;
        TestCntxt_p->BlockSize = VAL_SYM_ALGO_POLY1305_BLOCK_SIZE;
        break;
#endif

    default:
        break;
    }

    Status = val_SymAlloc(Algorithm, VAL_SYM_MODE_NONE, fUseTokenTemp, &TestCntxt_p->SymContext_p);
    unsupported_if((Status == VAL_INVALID_ALGORITHM), "");
    fail_if(Status != VAL_SUCCESS, "val_SymAlloc()=", Status);

    if (fKeyAsset && KeyAssetPolicy)
    {
        if (fVerify)
        {
            KeyAssetPolicy |= VAL_POLICY_MAC_VERIFY;
        }
        else
        {
            KeyAssetPolicy |= VAL_POLICY_MAC_GENERATE;
        }
        if (!val_IsAccessSecure())
        {
            KeyAssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
        }
        Status = val_AssetAlloc(KeyAssetPolicy, KeySize,
                                false, false,
                                VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                                &TestCntxt_p->KeyAssetId);
        fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(Key)=", Status);

#ifdef SFZUTF_USERMODE
        Status = val_AssetLoadPlaintext(TestCntxt_p->KeyAssetId,
                                        Key_p, KeySize);
#else
        {
            uint8_t * KeyCopy_p = (uint8_t *)SFZUTF_MALLOC(KeySize);
            fail_if(KeyCopy_p == NULL, "Allocation ", (int)KeySize);
            memcpy(KeyCopy_p, Key_p, KeySize);

            Status = val_AssetLoadPlaintext(TestCntxt_p->KeyAssetId,
                                            KeyCopy_p, KeySize);

            SFZUTF_FREE(KeyCopy_p);
        }
#endif
        fail_if(Status != VAL_SUCCESS, "val_AssetLoadPlaintext(Key)=", Status);
    }

    Status = val_SymInitKey(TestCntxt_p->SymContext_p,
                            TestCntxt_p->KeyAssetId, Key_p, KeySize);
    fail_if(Status != VAL_SUCCESS, "val_SymInitKey()=", Status);

    if ((Iv_p != NULL) && (IvSize > 0))
    {
        Status = val_SymInitIV(TestCntxt_p->SymContext_p, Iv_p, IvSize);
        fail_if(Status != VAL_SUCCESS, "val_SymInitIV()=", Status);
    }

    TestCntxt_p->Index = Index;
    TestCntxt_p->Msg_p = sfzutf_discard_const(Msg_p);
    TestCntxt_p->MsgLen = MsgLen;
    TestCntxt_p->Mac_p = sfzutf_discard_const(Mac_p);
    TestCntxt_p->MacLen = MacLen;

    return END_TEST_SUCCES;
}


/*----------------------------------------------------------------------------
 * test_cipher_mac
 */
static int
test_cipher_mac(
        SymCryptTestCtx_t * TestCntxt_p,
        bool fVerify,
        bool fMacFinalAsset,
        bool fMultipart,
        bool fReadWriteTemp)
{
    ValStatus_t Status;
    ValPolicyMask_t AssetPolicy = 0;
    ValAssetId_t MacAssetId = VAL_ASSETID_INVALID;
    ValSize_t MacSize = 0;
    uint8_t * Msg_p = TestCntxt_p->Msg_p;
    ValSize_t MsgSize = TestCntxt_p->MsgLen;

    if (fVerify && fMacFinalAsset)
    {
        AssetPolicy = VAL_POLICY_TEMP_MAC;
        if (!val_IsAccessSecure())
        {
            AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
        }
        Status = val_AssetAlloc(AssetPolicy, TestCntxt_p->MacLen,
                                false, false,
                                VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                                &MacAssetId);
        fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(MAC)=", Status);

        {
#ifdef SFZUTF_USERMODE
            uint8_t * MacCopy_p = TestCntxt_p->Mac_p;
#else
            uint8_t * MacCopy_p = (uint8_t *)SFZUTF_MALLOC(TestCntxt_p->MacLen);
            fail_if(MacCopy_p == NULL, "Allocation ", (int)TestCntxt_p->MacLen);
            memcpy(MacCopy_p, TestCntxt_p->Mac_p, TestCntxt_p->MacLen);
#endif

            Status = val_AssetLoadPlaintext(MacAssetId,
                                            MacCopy_p, TestCntxt_p->MacLen);
            fail_if(Status != VAL_SUCCESS, "val_AssetLoadPlaintext(MAC)=", Status);

#ifndef SFZUTF_USERMODE
            SFZUTF_FREE(MacCopy_p);
#endif
        }
    }

    if (fMultipart && (MsgSize > TestCntxt_p->BlockSize))
    {
#ifdef SFZUTF_USERMODE
        uint8_t * InCopy_p = Msg_p;
#else
        uint8_t * InCopy_p = (uint8_t *)SFZUTF_MALLOC(TestCntxt_p->BlockSize);
        fail_if(InCopy_p == NULL, "Allocation ", (int)TestCntxt_p->BlockSize);
        memcpy(InCopy_p, Msg_p, TestCntxt_p->BlockSize);
#endif

        Status = val_SymMacUpdate(TestCntxt_p->SymContext_p,
                                  InCopy_p, TestCntxt_p->BlockSize);
        fail_if(Status != VAL_SUCCESS, "val_SymMacUpdate()=", Status);

#ifndef SFZUTF_USERMODE
        SFZUTF_FREE(InCopy_p);
#endif

        if (fReadWriteTemp)
        {
            ValOctetsOut_t Mac[VAL_SYM_ALGO_MAX_DIGEST_SIZE];

            MacSize = sizeof(Mac);
            Status = val_SymReadTokenTemp(TestCntxt_p->SymContext_p,
                                          Mac, &MacSize, NULL);
            fail_if(Status != VAL_SUCCESS, "val_SymReadTokenTemp()=", Status);

            Status = val_SymWriteTokenTemp(TestCntxt_p->SymContext_p,
                                           Mac, MacSize, 0);
            fail_if(Status != VAL_SUCCESS, "val_SymWriteTokenTemp()=", Status);
        }

        Msg_p += TestCntxt_p->BlockSize;
        MsgSize -= TestCntxt_p->BlockSize;
    }

    {
#ifdef SFZUTF_USERMODE
        uint8_t * InCopy_p = Msg_p;
#else
        uint8_t * InCopy_p = (uint8_t *)SFZUTF_MALLOC(MsgSize);
        fail_if(InCopy_p == NULL, "Allocation ", (int)MsgSize);
        memcpy(InCopy_p, Msg_p, MsgSize);
#endif
        if (fVerify)
        {
            MacSize = TestCntxt_p->MacLen;
            if (MacAssetId == VAL_ASSETID_INVALID)
            {
                Status = val_SymMacVerify(TestCntxt_p->SymContext_p,
                                          InCopy_p, MsgSize,
                                          VAL_ASSETID_INVALID,
                                          TestCntxt_p->Mac_p, MacSize);
                fail_if(Status != VAL_SUCCESS, "val_SymMacVerify()=", Status);
            }
            else
            {
                Status = val_SymMacVerify(TestCntxt_p->SymContext_p,
                                          InCopy_p, MsgSize,
                                          MacAssetId,
                                          NULL, MacSize);
                fail_if(Status != VAL_SUCCESS, "val_SymMacVerify()=", Status);
            }
        }
        else
        {
            ValOctetsOut_t Mac[VAL_SYM_ALGO_MAX_DIGEST_SIZE];

            MacSize = sizeof(Mac);
            Status = val_SymMacGenerate(TestCntxt_p->SymContext_p,
                                        InCopy_p, MsgSize,
                                        Mac, &MacSize);
            fail_if(Status != VAL_SUCCESS, "val_SymMacGenerate()=", Status);
            //fail_if(Vector_p->MacLen != MacSize, "Result mismatch on length %d", VectorIndex);
            fail_if(memcmp(Mac, TestCntxt_p->Mac_p, TestCntxt_p->MacLen) != 0, "", TestCntxt_p->Index);
        }

#ifndef SFZUTF_USERMODE
        SFZUTF_FREE(InCopy_p);
#endif
    }

    if (MacAssetId != VAL_ASSETID_INVALID)
    {
        Status = val_AssetFree(MacAssetId);
        fail_if(Status != VAL_SUCCESS, "val_AssetFree(VerifyMAC)=", Status);
    }
    if (TestCntxt_p->KeyAssetId != VAL_ASSETID_INVALID)
    {
        Status = val_AssetFree(TestCntxt_p->KeyAssetId);
        fail_if(Status != VAL_SUCCESS, "val_AssetFree(Key)=", Status);
    }

    return END_TEST_SUCCES;
}


/*----------------------------------------------------------------------------
 * test_aes_cmac
 * _i value: bit 0 -> 0 = MacGenerate
 *                 -> 1 = MacVerify
 *           bit 1 -> 0 = Key in token
 *                 -> 1 = Key as Asset
 *           bit 2 -> 0 = Final MAC in token
 *                 -> 1 = Final MAC Asset
 *           bit 3 -> 0 = Single shot
 *                 -> 1 = Multi-part (one block + remaining part)
 *           bit 4 -> 0 = Use Asset for the intermedaite MAC (only applicable when bit 3 is set)
 *                 -> 1 = Use Token for the intermedaite MAC (only applicable when bit 3 is set)
 */
START_TEST(test_aes_cmac)
{
    SymCryptTestCtx_t TestCntxt;
    int Index;
    int Success = 0;
    int Failed = 0;

    // Only perform a certain test once, so filter duplicate operations
    // for MACGenerate and fUseTokenTemp
    if (((_i & 0xF) == 1) || ((_i & 0xF) == 5) ||
        ((_i & 0xF) == 9) || ((_i & 0xF) == 13) ||
        ((_i & 0x10) && ((_i & 0xA) != 0x8)))
    {
        LOG_CRIT("SKIPPED: Duplicate/N.A. AES-CMAC test (L%d)\n", _i);
    }
    else
    {
        for (Index = 0; ; Index++)
        {
            TestVector_AES_CMAC_t tv_p;

            tv_p = test_vectors_aes_cmac_get(Index);
            if (tv_p == NULL)
            {
                break;
            }

            // NOTE: MAC does not handle a partial MAC value compare yet,
            //       so allow only complete MAC values
            if (/*(tv_p->MsgLen > 0) &&*/
                (tv_p->MacLen == VAL_SYM_ALGO_AES_BLOCK_SIZE))
            {
                if (SetupTestContextAndEnvironment(&TestCntxt, Index,
                                                   VAL_SYM_ALGO_MAC_AES_CMAC,
                                                   ((_i & 1) != 0),
                                                   ((_i & 2) != 0),
                                                   ((_i & 16) != 0),
                                                   tv_p->Key, tv_p->KeyLen,
                                                   NULL,
                                                   tv_p->Msg, tv_p->MsgLen,
                                                   tv_p->Mac, tv_p->MacLen) == END_TEST_SUCCES)
                {
                    if (test_cipher_mac(&TestCntxt, (_i & 1), (_i & 4), (_i & 8), ((_i & 16) != 0)) == END_TEST_SUCCES)
                    {
                        Success++;
                    }
                    else
                    {
                        LOG_CRIT("Process vector %d\n", Index);
                        Failed++;
                    }
                }
                else
                {
                    LOG_CRIT("Process vector %d\n", Index);
                    Failed++;
                }
            }
            test_vectors_aes_cmac_release(tv_p);
        }

        fail_if(Failed, "#wrong tests", Failed);
    }
}
END_TEST


/*----------------------------------------------------------------------------
 * test_aes_cbcmac
 * _i value: bit 0 -> 0 = MacGenerate
 *                 -> 1 = MacVerify
 *           bit 1 -> 0 = Key in token
 *                 -> 1 = Key as Asset
 *           bit 2 -> 0 = Final MAC in token
 *                 -> 1 = Final MAC Asset
 *           bit 3 -> 0 = Single shot
 *                 -> 1 = Multi-part (one block + remaining part)
 *           bit 4 -> 0 = Use Asset for the intermedaite MAC (only applicable when bit 3 is set)
 *                 -> 1 = Use Token for the intermedaite MAC (only applicable when bit 3 is set)
 */
START_TEST(test_aes_cbcmac)
{
    SymCryptTestCtx_t TestCntxt;
    int Index;
    int Success = 0;
    int Failed = 0;

    // Only perform a certain test once, so filter duplicate operations
    // for MACGenerate and fUseTokenTemp
    if (((_i & 0xF) == 1) || ((_i & 0xF) == 5) ||
        ((_i & 0xF) == 9) || ((_i & 0xF) == 13) ||
        ((_i & 0x10) && ((_i & 0xA) != 0x8)))
    {
        LOG_CRIT("SKIPPED: Duplicate/N.A. AES-CBCMAC test (L%d)\n", _i);
    }
    else
    {
        for (Index = 0; ; Index++)
        {
            TestVector_AES_CBCMAC_t tv_p;

            tv_p = test_vectors_aes_cbcmac_get(Index);
            if (tv_p == NULL)
            {
                break;
            }

            // NOTE: MAC does not handle a partial MAC value compare yet,
            //       so allow only complete MAC values
            if ((tv_p->MsgLen > 0) &&
                (tv_p->MacLen == VAL_SYM_ALGO_AES_BLOCK_SIZE))
            {
                if (SetupTestContextAndEnvironment(&TestCntxt, Index,
                                                   VAL_SYM_ALGO_MAC_AES_CBC_MAC,
                                                   ((_i & 1) != 0),
                                                   ((_i & 2) != 0),
                                                   ((_i & 16) != 0),
                                                   tv_p->Key, tv_p->KeyLen,
                                                   NULL,
                                                   tv_p->Msg, tv_p->MsgLen,
                                                   tv_p->Mac, tv_p->MacLen) == END_TEST_SUCCES)
                {
                    if (test_cipher_mac(&TestCntxt, (_i & 1), (_i & 4), (_i & 8), ((_i & 16) != 0)) == END_TEST_SUCCES)
                    {
                        Success++;
                    }
                    else
                    {
                        LOG_CRIT("Process vector %d\n", Index);
                        Failed++;
                    }
                }
                else
                {
                    LOG_CRIT("Process vector %d\n", Index);
                    Failed++;
                }
            }
            test_vectors_aes_cbcmac_release(tv_p);
        }

        fail_if(Failed, "#wrong tests", Failed);
    }
}
END_TEST


#ifdef VALTEST_SYM_ALGO_ARIA
/*----------------------------------------------------------------------------
 * test_aria_cmac
 * _i value: bit 0 -> 0 = MacGenerate
 *                 -> 1 = MacVerify
 *           bit 1 -> 0 = Key in token
 *                 -> 1 = Key as Asset
 *           bit 2 -> 0 = Final MAC in token
 *                 -> 1 = Final MAC Asset
 *           bit 3 -> 0 = Single shot
 *                 -> 1 = Multi-part (one block + remaining part)
 *           bit 4 -> 0 = Use Asset for the intermedaite MAC (only applicable when bit 3 is set)
 *                 -> 1 = Use Token for the intermedaite MAC (only applicable when bit 3 is set)
 */
START_TEST(test_aria_cmac)
{
    SymCryptTestCtx_t TestCntxt;
    int Index;
    int Success = 0;
    int Failed = 0;

    unsupported_if(!valtest_IsARIASupported(), "");

    // Only perform a certain test once, so filter duplicate operations
    // for MACGenerate and fUseTokenTemp
    if (((_i & 0xF) == 1) || ((_i & 0xF) == 5) ||
        ((_i & 0xF) == 9) || ((_i & 0xF) == 13) ||
        ((_i & 0x10) && ((_i & 0xA) != 0x8)))
    {
        LOG_CRIT("SKIPPED: Duplicate/N.A. ARIA-CMAC test (L%d)\n", _i);
    }
    else
    {
        for (Index = 0; ; Index++)
        {
            TestVector_ARIA_CMAC_t tv_p;

            tv_p = test_vectors_aria_cmac_get(Index);
            if (tv_p == NULL)
            {
                break;
            }

            // NOTE: MAC does not handle a partial MAC value compare yet,
            //       so allow only complete MAC values
            if (/*(tv_p->MsgLen > 0) &&*/
                (tv_p->MacLen == VAL_SYM_ALGO_ARIA_BLOCK_SIZE))
            {
                if (SetupTestContextAndEnvironment(&TestCntxt, Index,
                                                   VAL_SYM_ALGO_MAC_ARIA_CMAC,
                                                   ((_i & 1) != 0),
                                                   ((_i & 2) != 0),
                                                   ((_i & 16) != 0),
                                                   tv_p->Key, tv_p->KeyLen,
                                                   NULL,
                                                   tv_p->Msg, tv_p->MsgLen,
                                                   tv_p->Mac, tv_p->MacLen) == END_TEST_SUCCES)
                {
                    if (test_cipher_mac(&TestCntxt, (_i & 1), (_i & 4), (_i & 8), ((_i & 16) != 0)) == END_TEST_SUCCES)
                    {
                        Success++;
                    }
                    else
                    {
                        LOG_CRIT("Process vector %d\n", Index);
                        Failed++;
                    }
                }
                else
                {
                    LOG_CRIT("Process vector %d\n", Index);
                    Failed++;
                }
            }
            test_vectors_aria_cmac_release(tv_p);
        }

        fail_if(Failed, "#wrong tests", Failed);
    }
}
END_TEST


/*----------------------------------------------------------------------------
 * test_aria_cbcmac
 * _i value: bit 0 -> 0 = MacGenerate
 *                 -> 1 = MacVerify
 *           bit 1 -> 0 = Key in token
 *                 -> 1 = Key as Asset
 *           bit 2 -> 0 = Final MAC in token
 *                 -> 1 = Final MAC Asset
 *           bit 3 -> 0 = Single shot
 *                 -> 1 = Multi-part (one block + remaining part)
 *           bit 4 -> 0 = Use Asset for the intermedaite MAC (only applicable when bit 3 is set)
 *                 -> 1 = Use Token for the intermedaite MAC (only applicable when bit 3 is set)
 */
START_TEST(test_aria_cbcmac)
{
    SymCryptTestCtx_t TestCntxt;
    int Index;
    int Success = 0;
    int Failed = 0;

    unsupported_if(!valtest_IsARIASupported(), "");

    // Only perform a certain test once, so filter duplicate operations
    // for MACGenerate and fUseTokenTemp
    if (((_i & 0xF) == 1) || ((_i & 0xF) == 5) ||
        ((_i & 0xF) == 9) || ((_i & 0xF) == 13) ||
        ((_i & 0x10) && ((_i & 0xA) != 0x8)))
    {
        LOG_CRIT("SKIPPED: Duplicate/N.A. ARIA-CBCMAC test (L%d)\n", _i);
    }
    else
    {
        for (Index = 0; ; Index++)
        {
            TestVector_ARIA_CBCMAC_t tv_p;

            tv_p = test_vectors_aria_cbcmac_get(Index);
            if (tv_p == NULL)
            {
                break;
            }

            // NOTE: MAC does not handle a partial MAC value compare yet,
            //       so allow only complete MAC values
            if ((tv_p->MsgLen > 0) &&
                (tv_p->MacLen == VAL_SYM_ALGO_ARIA_BLOCK_SIZE))
            {
                if (SetupTestContextAndEnvironment(&TestCntxt, Index,
                                                   VAL_SYM_ALGO_MAC_ARIA_CBC_MAC,
                                                   ((_i & 1) != 0),
                                                   ((_i & 2) != 0),
                                                   ((_i & 16) != 0),
                                                   tv_p->Key, tv_p->KeyLen,
                                                   NULL,
                                                   tv_p->Msg, tv_p->MsgLen,
                                                   tv_p->Mac, tv_p->MacLen) == END_TEST_SUCCES)
                {
                    if (test_cipher_mac(&TestCntxt, (_i & 1), (_i & 4), (_i & 8), ((_i & 16) != 0)) == END_TEST_SUCCES)
                    {
                        Success++;
                    }
                    else
                    {
                        LOG_CRIT("Process vector %d\n", Index);
                        Failed++;
                    }
                }
                else
                {
                    LOG_CRIT("Process vector %d\n", Index);
                    Failed++;
                }
            }
            test_vectors_aria_cbcmac_release(tv_p);
        }

        fail_if(Failed, "#wrong tests", Failed);
    }
}
END_TEST
#endif


#ifdef VALTEST_SYM_ALGO_POLY1305
/*----------------------------------------------------------------------------
 * valtest_IsPoly1305Supported
 *
 * This function checks if the Poly1305 is supported or not.
 */
bool
valtest_IsPoly1305Supported(void)
{
    static uint8_t gl_Poly1305Supported = 0;
    ValSymContextPtr_t SymContext_p;
    TestVector_Poly1305_t tv_p;
    uint8_t * Message_p;
    ValStatus_t Status;
    ValSize_t MacSize = 0;
    ValOctetsOut_t Mac[VAL_SYM_ALGO_MAX_DIGEST_SIZE];

    if (gl_Poly1305Supported != 0)
    {
        return (gl_Poly1305Supported == 0xA5);
    }

    tv_p = test_vectors_poly1305_get(0);
    if (tv_p == NULL)
    {
        gl_Poly1305Supported = 0x5A;
        return false;
    }

    Status = val_SymAlloc(VAL_SYM_ALGO_MAC_POLY1305, VAL_SYM_MODE_NONE,
                          false, &SymContext_p);
    if (Status != VAL_SUCCESS)
    {
        gl_Poly1305Supported = 0x5A;
        return false;
    }

    Status = val_SymInitKey(SymContext_p, VAL_ASSETID_INVALID,
                            tv_p->Key_p, tv_p->KeyLen);
    if (Status != VAL_SUCCESS)
    {
        val_SymRelease(SymContext_p);
        gl_Poly1305Supported = 0x5A;
        return false;
    }

    Message_p = (uint8_t *)SFZUTF_MALLOC(tv_p->MsgLen);
    if (Message_p == NULL)
    {
        val_SymRelease(SymContext_p);
        gl_Poly1305Supported = 0x5A;
        return false;
    }
    memcpy(Message_p, tv_p->Message_p, tv_p->MsgLen);

    MacSize = sizeof(Mac);
    Status = val_SymMacGenerate(SymContext_p,
                                Message_p, tv_p->MsgLen,
                                Mac, &MacSize);
    SFZUTF_FREE(Message_p);
    if (Status != VAL_SUCCESS)
    {
        val_SymRelease(SymContext_p);
        gl_Poly1305Supported = 0x5A;
    }
    else
    {
        gl_Poly1305Supported = 0xA5;
    }
    return (gl_Poly1305Supported == 0xA5);
}


/*----------------------------------------------------------------------------
 * test_poly1305
 * _i value: bit 0 -> 0 = MacGenerate
 *                 -> 1 = MacVerify
 *           bit 1 -> 0 = Key in token
 *                 -> 1 = Key as Asset
 *           bit 2 -> 0 = Final MAC in token
 *                 -> 1 = Final MAC Asset
 *           bit 3 -> 0 = Single shot
 *                 -> 1 = Multi-part (one block + remaining part)
 *           bit 4 -> 0 = Use Asset for the intermedaite MAC (only applicable when bit 3 is set)
 *                 -> 1 = Use Token for the intermedaite MAC (only applicable when bit 3 is set)
 */
START_TEST(test_poly1305)
{
    SymCryptTestCtx_t TestCntxt;
    int Index;
    int Success = 0;
    int Failed = 0;

    unsupported_if(!valtest_IsPoly1305Supported(), "");

    // Only perform a certain test once, so filter duplicate operations
    // for MACGenerate and fUseTokenTemp
    if (((_i & 0xF) == 1) || ((_i & 0xF) == 5) ||
        ((_i & 0xF) == 9) || ((_i & 0xF) == 13) ||
        ((_i & 0x10) && ((_i & 0xA) != 0x8)))
    {
        LOG_CRIT("SKIPPED: Duplicate/N.A. Poly1305 test (L%d)\n", _i);
    }
    else
    {
        for (Index = 0; ; Index++)
        {
            TestVector_Poly1305_t tv_p;

            tv_p = test_vectors_poly1305_get(Index);
            if (tv_p == NULL)
            {
                break;
            }

            // NOTE: MAC does not handle a partial MAC value compare yet,
            //       so allow only complete MAC values
            if ((tv_p->MsgLen > 0) &&
                (tv_p->TagLen == VAL_SYM_ALGO_POLY1305_BLOCK_SIZE))
            {
                if (SetupTestContextAndEnvironment(&TestCntxt, Index,
                                                   VAL_SYM_ALGO_MAC_POLY1305,
                                                   ((_i & 1) != 0),
                                                   ((_i & 2) != 0),
                                                   ((_i & 16) != 0),
                                                   tv_p->Key_p, tv_p->KeyLen,
                                                   NULL,
                                                   tv_p->Message_p, tv_p->MsgLen,
                                                   tv_p->Tag_p, tv_p->TagLen) == END_TEST_SUCCES)
                {
                    if (test_cipher_mac(&TestCntxt, (_i & 1), (_i & 4), (_i & 8), ((_i & 16) != 0)) == END_TEST_SUCCES)
                    {
                        Success++;
                    }
                    else
                    {
                        LOG_CRIT("Process vector %d\n", Index);
                        Failed++;
                    }
                }
                else
                {
                    LOG_CRIT("Process vector %d\n", Index);
                    Failed++;
                }
            }
            test_vectors_poly1305_release(tv_p);
        }

        fail_if(Failed, "#wrong tests", Failed);
    }
}
END_TEST
#endif


/*----------------------------------------------------------------------------
 * suite_add_test_SymCipherMac
 */
int
suite_add_test_SymCipherMac(
        struct TestSuite * TestSuite_p)
{
    struct TestCase * TestCase_p;

    TestCase_p = sfzutf_tcase_create(TestSuite_p, "SymCrypto_CipherMac_Tests");
    if (TestCase_p != NULL)
    {
        if (sfzutf_tcase_add_fixture(TestCase_p, valtest_initialize, valtest_terminate) != 0)
        {
             goto FuncErrorReturn;
        }

        if (sfzutf_test_add_loop(TestCase_p, test_aes_cmac, 32) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add_loop(TestCase_p, test_aes_cbcmac, 32) != 0) goto FuncErrorReturn;
#ifdef VALTEST_SYM_ALGO_ARIA
        if (sfzutf_test_add_loop(TestCase_p, test_aria_cmac, 32) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add_loop(TestCase_p, test_aria_cbcmac, 32) != 0) goto FuncErrorReturn;
#endif
#ifdef VALTEST_SYM_ALGO_POLY1305
        if (sfzutf_test_add_loop(TestCase_p, test_poly1305, 32) != 0) goto FuncErrorReturn;
#endif

        return 0;
    }

FuncErrorReturn:
    return -1;
}

/* end of file sfzcrypto_cipher_mac.c */
