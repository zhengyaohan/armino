/* valtest_aes_keywrap.c
 *
 * Description: VAL Test Suite, AES-WRAP
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
#include "testvectors_aes_wrap.h"

/*----------------------------------------------------------------------------
 * do_AesKeyWrapUnwrap
 */
static int
do_AesKeyWrapUnwrap(
        TestVector_AES_WRAP_t tv_p,
        bool fWrap,
        bool fKeyAsset)
{
    ValStatus_t Status;
    ValAssetId_t KeyAssetId = VAL_ASSETID_INVALID;
    const uint8_t * TxtOut_p;
    uint8_t * TxtIn_p;
    uint8_t * Result_p;
    ValSize_t TxtInLen;
    ValSize_t ExpectedLen;
    ValSize_t ResultLen = (ValSize_t)(tv_p->PlainTxtLen + 8);

    Result_p = (uint8_t *)SFZUTF_MALLOC(ResultLen);
    fail_if(Result_p == NULL, "Allocation ", (int)ResultLen);
    memset(Result_p, 0, ResultLen);

    if (fKeyAsset)
    {
        ValPolicyMask_t AssetPolicy = VAL_POLICY_AES_WRAP|VAL_POLICY_ENCRYPT|VAL_POLICY_DECRYPT;
        if (!val_IsAccessSecure())
        {
            AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
        }
        Status = val_AssetAlloc(AssetPolicy, (ValSize_t)tv_p->KeyLen,
                                false, false,
                                VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                                &KeyAssetId);
        fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(Key)=", Status);

#ifdef SFZUTF_USERMODE
        Status = val_AssetLoadPlaintext(KeyAssetId,
                                        tv_p->WrapKey_p, (ValSize_t)tv_p->KeyLen);

#else
        {
            uint8_t * KeyCopy_p = (uint8_t *)SFZUTF_MALLOC(tv_p->KeyLen);
            fail_if(KeyCopy_p == NULL, "Allocation ", (int)tv_p->KeyLen);
            memcpy(KeyCopy_p, tv_p->WrapKey_p, tv_p->KeyLen);

            Status = val_AssetLoadPlaintext(KeyAssetId,
                                            KeyCopy_p, (ValSize_t)tv_p->KeyLen);

            SFZUTF_FREE(KeyCopy_p);
        }
#endif
        fail_if(Status != VAL_SUCCESS, "val_AssetLoadPlaintext(Key)=", Status);
    }

    if (fWrap)
    {
        TxtInLen = (ValSize_t)tv_p->PlainTxtLen;
        TxtOut_p = tv_p->WrappedTxt_p;
        ExpectedLen = (ValSize_t)(tv_p->PlainTxtLen + 8);

#ifdef SFZUTF_USERMODE
        TxtIn_p = (uint8_t *)sfzutf_discard_const(tv_p->PlainTxt_p);
#else
        TxtIn_p = (uint8_t *)SFZUTF_MALLOC(TxtInLen);
        fail_if(TxtIn_p == NULL, "Allocation ", (int)TxtInLen);
        memcpy(TxtIn_p, tv_p->PlainTxt_p, TxtInLen);
#endif

        if (fKeyAsset)
        {
            Status = val_SymAesKeyWrap(KeyAssetId,
                                       NULL,
                                       (ValSize_t)tv_p->KeyLen,
                                       (ValOctetsIn_t *)TxtIn_p,
                                       TxtInLen,
                                       (ValOctetsOut_t *)Result_p,
                                       &ResultLen);
        }
        else
        {
            Status = val_SymAesKeyWrap(VAL_ASSETID_INVALID,
                                       (ValOctetsIn_Optional_t *)tv_p->WrapKey_p,
                                       (ValSize_t)tv_p->KeyLen,
                                       (ValOctetsIn_t *)TxtIn_p,
                                       TxtInLen,
                                       (ValOctetsOut_t *)Result_p,
                                       &ResultLen);
        }
        fail_if(Status != VAL_SUCCESS, "val_SymAesKeyWrap()=", Status);
    }
    else
    {
        TxtInLen = (ValSize_t)(tv_p->PlainTxtLen + 8);
        TxtOut_p = tv_p->PlainTxt_p;
        ExpectedLen = (ValSize_t)tv_p->PlainTxtLen;

#ifdef SFZUTF_USERMODE
        TxtIn_p = (uint8_t *)sfzutf_discard_const(tv_p->WrappedTxt_p);
#else
        TxtIn_p = (uint8_t *)SFZUTF_MALLOC(TxtInLen);
        fail_if(TxtIn_p == NULL, "Allocation ", (int)TxtInLen);
        memcpy(TxtIn_p, tv_p->WrappedTxt_p, TxtInLen);
#endif

        if (fKeyAsset)
        {
            Status = val_SymAesKeyUnwrap(KeyAssetId,
                                         NULL,
                                         (ValSize_t)tv_p->KeyLen,
                                         (ValOctetsIn_t *)TxtIn_p,
                                         TxtInLen,
                                         (ValOctetsOut_t *)Result_p,
                                         &ResultLen);
            fail_if(Status != VAL_ACCESS_ERROR, "val_SymAesKeyUnwrap()=", Status);
        }
        else
        {
            Status = val_SymAesKeyUnwrap(VAL_ASSETID_INVALID,
                                         (ValOctetsIn_Optional_t *)tv_p->WrapKey_p,
                                         (ValSize_t)tv_p->KeyLen,
                                         (ValOctetsIn_t *)TxtIn_p,
                                         TxtInLen,
                                         (ValOctetsOut_t *)Result_p,
                                         &ResultLen);
            fail_if(Status != VAL_SUCCESS, "val_SymAesKeyUnwrap()=", Status);
        }
    }

#ifndef SFZUTF_USERMODE
    SFZUTF_FREE(TxtIn_p);
#endif

    if (Status == VAL_SUCCESS)
    {
        fail_if(ResultLen != ExpectedLen, "Mismatch", (int)ResultLen);
        fail_if(memcmp(TxtOut_p, Result_p, ResultLen) != 0,  "Mismatch", -1);
    }

    if (KeyAssetId != VAL_ASSETID_INVALID)
    {
        Status = val_AssetFree(KeyAssetId);
        fail_if(Status != VAL_SUCCESS, "val_AssetFree(Key)=", Status);
    }

    SFZUTF_FREE(Result_p);

    return END_TEST_SUCCES;
}

/*----------------------------------------------------------------------------
 * test_aes_wrap_unwrap
 *
 * Test with the AES-WRAP test vectors. Intended to be run twice, to cover
 * encryption/wrap (_1 = 0) and decryption/unwrap (_i = 1).
 */
START_TEST(test_aes_wrap_unwrap)
{
    int Index;
    int Success = 0;
    int Failed = 0;

    for (Index = 0; ; Index++)
    {
        TestVector_AES_WRAP_t tv_p;

        tv_p = test_vectors_aes_wrap_get(Index);
        if (tv_p == NULL)
        {
            break;
        }

        if (do_AesKeyWrapUnwrap(tv_p, ((_i & 1) == 0), ((_i & 2) != 0)) == END_TEST_SUCCES)
        {
            Success++;
        }
        else
        {
            LOG_CRIT("Process vector %d\n", Index);
            Failed++;
        }

        test_vectors_aes_wrap_release(tv_p);
    }

    fail_if(Failed, "#wrong tests", Failed);
}
END_TEST


int
suite_add_test_SymKeyWrap(
        struct TestSuite * TestSuite_p)
{
    struct TestCase * TestCase_p;

    TestCase_p = sfzutf_tcase_create(TestSuite_p, "SymCrypto_KeyWrapUnwrap_Tests");
    if (TestCase_p != NULL)
    {
        if (sfzutf_tcase_add_fixture(TestCase_p, valtest_initialize, valtest_terminate) != 0)
        {
             goto FuncErrorReturn;
        }

        if (sfzutf_test_add_loop(TestCase_p, test_aes_wrap_unwrap, 4) != 0) goto FuncErrorReturn;

        return 0;
    }

FuncErrorReturn:
    return -1;
}

/* end of file valtest_aes_keywrap.c */
