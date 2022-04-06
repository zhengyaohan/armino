/* valtest_sym_auth_crypto.c
 *
 * Description: Authenticated crypto tests
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
#ifdef VALTEST_SYM_ALGO_AES_CCM
#include "testvectors_aes_ccm.h"
#endif
#ifdef VALTEST_SYM_ALGO_AES_GCM
#include "testvectors_aes_gcm.h"
#endif
#ifdef VALTEST_SYM_ALGO_ARIA_CCM
#include "testvectors_aria_ccm.h"
#endif
#ifdef VALTEST_SYM_ALGO_ARIA_GCM
#include "testvectors_aria_gcm.h"
#endif
#if (defined(VALTEST_SYM_ALGO_CHACHA20) && defined(VALTEST_SYM_ALGO_POLY1305))
#include "testvectors_chacha20_poly1305_aead.h"
#endif

#if defined(VALTEST_SYM_ALGO_AES_CCM) || \
    defined(VALTEST_SYM_ALGO_AES_GCM) || \
    (defined(VALTEST_SYM_ALGO_CHACHA20) && defined(VALTEST_SYM_ALGO_POLY1305)) || \
    defined(VALTEST_SYM_ALGO_ARIA_CCM) || \
    defined(VALTEST_SYM_ALGO_ARIA_GCM)
/*----------------------------------------------------------------------------
 * SetupTestContextAndEnvironment
 *
 * Setup for the "do_SymAuthCryptTest" test.
 */
static int
SetupTestContextAndEnvironment(
        const ValSymAlgo_t Algorithm,
        const ValSymMode_t Mode,
        const uint8_t * KeyData_p,
        const uint32_t KeyDataLen,
        bool fKeyAsset,
        ValSymContextPtr_t * SymContext_pp,
        ValAssetId_t * KeyAssetId_p)
{
    ValStatus_t Status = VAL_INVALID_ALGORITHM;

    switch (Mode)
    {
#if defined(VALTEST_SYM_ALGO_AES_CCM) || defined(VALTEST_SYM_ALGO_ARIA_CCM)
    case VAL_SYM_MODE_CIPHER_CCM:
#endif
#if defined(VALTEST_SYM_ALGO_AES_GCM) || defined(VALTEST_SYM_ALGO_ARIA_GCM)
    case VAL_SYM_MODE_CIPHER_GCM:
#endif
#if defined(VALTEST_SYM_ALGO_CHACHA20) && defined(VALTEST_SYM_ALGO_POLY1305)
    case VAL_SYM_MODE_CIPHER_CHACHA20_AEAD:
#endif
        Status = val_SymAlloc(Algorithm, Mode, false, SymContext_pp);
        break;

    default:
        break;
    }
    unsupported_if((Status == VAL_INVALID_ALGORITHM), "");
    fail_if(Status != VAL_SUCCESS, "val_SymAlloc()=", Status);

    if (fKeyAsset)
    {
        uint8_t * Key_p;
        ValPolicyMask_t KeyAssetPolicy = 0;

        switch (Mode)
        {
#if defined(VALTEST_SYM_ALGO_AES_CCM) || defined(VALTEST_SYM_ALGO_ARIA_CCM)
        case VAL_SYM_MODE_CIPHER_CCM:
            if (Algorithm == VAL_SYM_ALGO_CIPHER_ARIA)
            {
                KeyAssetPolicy = VAL_POLICY_ARIA_MODE_CCM | VAL_POLICY_ENCRYPT | VAL_POLICY_DECRYPT;
            }
            else
            {
                KeyAssetPolicy = VAL_POLICY_AES_MODE_CCM | VAL_POLICY_ENCRYPT | VAL_POLICY_DECRYPT;
            }
            break;
#endif
#if defined(VALTEST_SYM_ALGO_AES_GCM) || defined(VALTEST_SYM_ALGO_ARIA_GCM)
        case VAL_SYM_MODE_CIPHER_GCM:
            if (Algorithm == VAL_SYM_ALGO_CIPHER_ARIA)
            {
                KeyAssetPolicy = VAL_POLICY_ARIA_MODE_GCM | VAL_POLICY_ENCRYPT | VAL_POLICY_DECRYPT;
            }
            else
            {
                KeyAssetPolicy = VAL_POLICY_AES_MODE_GCM | VAL_POLICY_ENCRYPT | VAL_POLICY_DECRYPT;
            }
            break;
#endif
#if defined(VALTEST_SYM_ALGO_CHACHA20) && defined(VALTEST_SYM_ALGO_POLY1305)
        case VAL_SYM_MODE_CIPHER_CHACHA20_AEAD:
            KeyAssetPolicy = VAL_POLICY_CHACHA20_AEAD | VAL_POLICY_ENCRYPT | VAL_POLICY_DECRYPT;
            break;
#endif
        default:
            break;
        }
        if (!val_IsAccessSecure())
        {
            KeyAssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
        }
        Status = val_AssetAlloc(KeyAssetPolicy, KeyDataLen,
                                false, false,
                                VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                                KeyAssetId_p);
        fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(Key)=", Status);

#ifdef SFZUTF_USERMODE
        Key_p = sfzutf_discard_const(KeyData_p);
#else
        Key_p = (uint8_t *)SFZUTF_MALLOC(KeyDataLen);
        fail_if(Key_p == NULL, "Allocation ", (int)KeyDataLen);
        memcpy(Key_p, KeyData_p, KeyDataLen);
#endif

        Status = val_AssetLoadPlaintext(*KeyAssetId_p, Key_p, KeyDataLen);

#ifndef SFZUTF_USERMODE
        SFZUTF_FREE(Key_p);
#endif
        fail_if(Status != VAL_SUCCESS, "val_AssetLoadPlaintext(Key)=", Status);

        Status = val_SymInitKey(*SymContext_pp, *KeyAssetId_p,
                                NULL, KeyDataLen);
    }
    else
    {
        Status = val_SymInitKey(*SymContext_pp, VAL_ASSETID_INVALID,
                                KeyData_p, KeyDataLen);
    }
    fail_if(Status != VAL_SUCCESS, "val_SymInitKey()=", Status);

    return END_TEST_SUCCES;
}


/*----------------------------------------------------------------------------
 * do_SymAuthCryptTest
 *
 * Common helper function for authtenticated cipher testing.
 */
static int
do_SymAuthCryptTest(
        ValSymContextPtr_t SymContext_p,
        const ValSymMode_t Mode,
        const uint8_t * NonceHashKey_p,
        const uint32_t NonceHashKeyLen,
        const ValSymGCMMode_t GCMMode,
        const uint8_t * tvPt_p,
        const uint32_t tvPtLen,
        const uint8_t * tvCt_p,
        const uint32_t tvCtLen,
        const uint8_t * tvAad_p,
        const uint32_t tvAadLen,
        const uint8_t * tvTag_p,
        const uint32_t tvTagLen,
        bool fDecrypt,
        bool fInPlace)
{
    static uint8_t ResultBuffer[VAL_TEST_MAX_BUFLEN];
    static uint8_t InputBuffer[VAL_TEST_MAX_BUFLEN];
    uint8_t TagBuffer[VAL_SYM_ALGO_MAX_TAG_SIZE];
    ValStatus_t Status;
    uint8_t * Aad_p = NULL;
    uint8_t * TxtIn_p;
    uint8_t * TxtOut_p;
    uint8_t * Result_p = ResultBuffer;
#ifndef SFZUTF_USERMODE
    uint8_t * TempBuffer_p;
#endif
    ValSize_t TxtInLen;
    ValSize_t TxtOutLen;
    ValSize_t Padding = 0;
    ValSize_t TagLen = tvTagLen;
    ValSize_t ResultLen = sizeof(ResultBuffer);

    IDENTIFIER_NOT_USED(tvCtLen);

    Status = val_SymCipherAEInit(SymContext_p,
                                 NonceHashKey_p, NonceHashKeyLen,
                                 TagLen, GCMMode);
    fail_if(Status != VAL_SUCCESS, "val_SymCipherAEInit()=", Status);

    TxtInLen  = tvPtLen;
    TxtOutLen = TxtInLen;
    if (fDecrypt)
    {
        TxtIn_p = sfzutf_discard_const(tvCt_p);
        if (GCMMode == VAL_SYM_MODE_GCM_GHASH)
        {
            TxtOut_p = NULL;
        }
        else
        {
            TxtOut_p = sfzutf_discard_const(tvPt_p);
        }
        memcpy(TagBuffer, tvTag_p, TagLen);
    }
    else
    {
        if (GCMMode == VAL_SYM_MODE_GCM_GHASH)
        {
            /* Note always use the CipherText for a GHASH calculation */
            TxtIn_p = sfzutf_discard_const(tvCt_p);
            TxtOut_p = NULL;
        }
        else
        {
            TxtIn_p = sfzutf_discard_const(tvPt_p);
            TxtOut_p = sfzutf_discard_const(tvCt_p);
        }
        memset(TagBuffer, 0, sizeof(TagBuffer));
        TagLen = sizeof(TagBuffer);

        Status = val_SymCipherInitEncrypt(SymContext_p);
        fail_if(Status != VAL_SUCCESS, "val_SymCipherInitEncrypt()=", Status);
    }

#if defined(VALTEST_SYM_ALGO_CHACHA20) && defined(VALTEST_SYM_ALGO_POLY1305)
    if (Mode == VAL_SYM_MODE_CIPHER_CHACHA20_AEAD)
    {
        if (TxtInLen & (VAL_SYM_ALGO_CHACHA20_BLOCK_SIZE - 1))
        {
            Padding = (0 - TxtInLen) & (VAL_SYM_ALGO_CHACHA20_BLOCK_SIZE - 1);
            memcpy(InputBuffer, TxtIn_p, TxtInLen);
            memset(&InputBuffer[TxtInLen], 0x00, Padding);
            TxtIn_p = InputBuffer;
        }
    }
    else
#endif
    if (TxtInLen & (VAL_SYM_ALGO_AES_BLOCK_SIZE - 1))
    {
        Padding = (0 - TxtInLen) & (VAL_SYM_ALGO_AES_BLOCK_SIZE - 1);
        memcpy(InputBuffer, TxtIn_p, TxtInLen);
        memset(&InputBuffer[TxtInLen], 0x00, Padding);
        TxtIn_p = InputBuffer;
    }

    if (fInPlace)
    {
#ifndef SFZUTF_USERMODE
        TempBuffer_p = (uint8_t *)SFZUTF_MALLOC(TxtInLen + Padding);
        fail_if(TempBuffer_p == NULL, "Allocation ", (int)(TxtInLen + Padding));
        Result_p = TempBuffer_p;
#endif
        memcpy(Result_p, TxtIn_p, (TxtInLen + Padding));
        TxtIn_p = Result_p;
    }
    else
    {
#ifndef SFZUTF_USERMODE
        TempBuffer_p = (uint8_t *)SFZUTF_MALLOC(TxtInLen + Padding);
        fail_if(TempBuffer_p == NULL, "Allocation ", (int)(TxtInLen + Padding));
        memcpy(TempBuffer_p, TxtIn_p, (TxtInLen + Padding));
        TxtIn_p = TempBuffer_p;
#endif

        if (GCMMode != VAL_SYM_MODE_GCM_GHASH)
        {
#ifndef SFZUTF_USERMODE
            Result_p = (uint8_t *)SFZUTF_MALLOC(TxtInLen + Padding);
            fail_if(Result_p == NULL, "Allocation ", (int)(TxtInLen + Padding));
            ResultLen = TxtInLen + Padding;
#endif
            memset(Result_p, 0xDC, ResultLen);
        }
        else
        {
            Result_p = NULL;
            ResultLen = 0;
        }
    }

    if (tvAadLen > 0)
    {
#ifdef SFZUTF_USERMODE
        Aad_p = sfzutf_discard_const(tvAad_p);
#else
        Aad_p = (uint8_t *)SFZUTF_MALLOC(tvAadLen);
        fail_if(Aad_p == NULL, "Allocation ", (int)tvAadLen);
        memcpy(Aad_p, tvAad_p, tvAadLen);
#endif
    }

    Status = val_SymCipherAEFinal(SymContext_p,
                                  Aad_p, tvAadLen,
                                  TxtIn_p, TxtInLen,
                                  Result_p, &ResultLen,
                                  TagBuffer, &TagLen);
    fail_if(Status != VAL_SUCCESS, "val_SymCipherAEFinal()=", Status);
    if (TxtOut_p != NULL)
    {
        fail_if(ResultLen != TxtOutLen, " ", (int)ResultLen);
        fail_if(memcmp(Result_p, TxtOut_p, TxtOutLen) != 0, " ", -1);
    }
    if (!fDecrypt)
    {
        fail_if(TagLen != TagLen, " ", (int)TagLen);
        fail_if(memcmp(TagBuffer, tvTag_p, TagLen) != 0, " ", -1);
    }

#ifndef SFZUTF_USERMODE
    if (Aad_p != NULL)
    {
        SFZUTF_FREE(Aad_p);
    }
    if (!fInPlace)
    {
        SFZUTF_FREE(TxtIn_p);
    }
    if (Result_p != NULL)
    {
        SFZUTF_FREE(Result_p);
    }
#endif

    return END_TEST_SUCCES;
}
#endif


#ifdef VALTEST_SYM_ALGO_AES_CCM
START_TEST(test_authenticated_crypto_aes_ccm)
{
    int Index;
    int Success = 0;
    int Failed = 0;

    for (Index = 0; ; Index++)
    {
        TestVector_AES_CCM_t tv_p;
        ValSymContextPtr_t SymContext_p;
        ValAssetId_t KeyAssetId = VAL_ASSETID_INVALID;

        tv_p = test_vectors_aes_ccm_get(Index);
        if (tv_p == NULL)
        {
            break;
        }

        if (SetupTestContextAndEnvironment(VAL_SYM_ALGO_CIPHER_AES,
                                           VAL_SYM_MODE_CIPHER_CCM,
                                           tv_p->Key_p, tv_p->KeyLen,
                                           ((_i & 4) != 0),
                                           &SymContext_p,
                                           &KeyAssetId) == END_TEST_SUCCES)
        {
            if (do_SymAuthCryptTest(SymContext_p,
                                    VAL_SYM_MODE_CIPHER_CCM,
                                    tv_p->Nonce_p, tv_p->NonceLen,
                                    VAL_SYM_MODE_GCM_NONE,
                                    tv_p->Pt_p, tv_p->PtLen,
                                    tv_p->Ct_p, tv_p->CtLen,
                                    tv_p->Aad_p, tv_p->AadLen,
                                    (tv_p->Ct_p+tv_p->PtLen), (tv_p->CtLen - tv_p->PtLen),
                                    ((_i & 1) != 0),
                                    ((_i == 2) || (_i == 3))) == END_TEST_SUCCES)
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

        if (KeyAssetId != VAL_ASSETID_INVALID)
        {
            (void)val_AssetFree(KeyAssetId);
            KeyAssetId = VAL_ASSETID_INVALID;
        }

        test_vectors_aes_ccm_release(tv_p);
    }

    fail_if(Failed, "#wrong tests", Failed);
}
END_TEST
#endif


#ifdef VALTEST_SYM_ALGO_AES_GCM
static int
test_authenticated_crypto_aes_gcm(
        const ValSymGCMMode_t Mode,
        const int _i)
{
    int Index;
    int Success = 0;
    int Failed = 0;

    for (Index = 0; ; Index++)
    {
        TestVector_AES_GCM_t tv_p;
        ValSymContextPtr_t SymContext_p;
        ValAssetId_t KeyAssetId = VAL_ASSETID_INVALID;
        const uint8_t * HashKey_p = NULL;
        uint32_t HashKeyLen = 0;
        const uint8_t * Tag_p = NULL;
        uint32_t TagLen = 0;

        tv_p = test_vectors_aes_gcm_get(Index);
        if (tv_p == NULL)
        {
            break;
        }

        if (SetupTestContextAndEnvironment(VAL_SYM_ALGO_CIPHER_AES,
                                           VAL_SYM_MODE_CIPHER_GCM,
                                           tv_p->Key_p, tv_p->KeyLen,
                                           ((_i & 4) != 0),
                                           &SymContext_p,
                                           &KeyAssetId) == END_TEST_SUCCES)
        {
            {
                ValStatus_t Status;

                if (tv_p->IVLen == 12)
                {
                    uint8_t IV[VAL_SYM_ALGO_AES_IV_SIZE];

                    memcpy(IV, tv_p->IV_p, tv_p->IVLen);
                    memcpy(&IV[12], "\x00\x00\x00\x01", 4);
                    Status = val_SymInitIV(SymContext_p, IV, VAL_SYM_ALGO_AES_IV_SIZE);
                }
                else
                {
                    Status = val_SymInitIV(SymContext_p, tv_p->Y0_p, VAL_SYM_ALGO_AES_IV_SIZE);
                }
                fail_if(Status != VAL_SUCCESS, "val_SymInitIV()=", Status);
            }

            if (Mode != VAL_SYM_MODE_GCM_AUTO)
            {
                HashKey_p = tv_p->H_p;
                HashKeyLen = (uint32_t)tv_p->HLen;
            }

            if ((Mode == VAL_SYM_MODE_GCM_AUTO) ||
                (Mode == VAL_SYM_MODE_GCM_H_PRE_Y0_CALC))
            {
                Tag_p = tv_p->Tag_p;
                TagLen = (uint32_t)tv_p->TagLen;
            }
            else
            {
                Tag_p = tv_p->Ghash_p;
                TagLen = (uint32_t)tv_p->GhashLen;
            }

            if (do_SymAuthCryptTest(SymContext_p,
                                    VAL_SYM_MODE_CIPHER_GCM,
                                    HashKey_p, HashKeyLen,
                                    Mode,
                                    tv_p->Pt_p, tv_p->PtLen,
                                    tv_p->Ct_p, tv_p->CtLen,
                                    tv_p->Aad_p, tv_p->AadLen,
                                    Tag_p, TagLen,
                                    ((_i & 1) != 0),
                                    ((_i == 2) || (_i == 3))) == END_TEST_SUCCES)
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

        if (KeyAssetId != VAL_ASSETID_INVALID)
        {
            (void)val_AssetFree(KeyAssetId);
            KeyAssetId = VAL_ASSETID_INVALID;
        }

        test_vectors_aes_gcm_release(tv_p);
    }

    fail_if(Failed, "#wrong tests", Failed);

    return END_TEST_SUCCES;
}

START_TEST(test_authenticated_crypto_aes_gcm_auto)
{
    if (test_authenticated_crypto_aes_gcm(VAL_SYM_MODE_GCM_AUTO, _i) != END_TEST_SUCCES)
    {
        return END_TEST_FAIL;
    }
}
END_TEST

START_TEST(test_authenticated_crypto_aes_gcm_H_pre_Y0_calc)
{
    if (test_authenticated_crypto_aes_gcm(VAL_SYM_MODE_GCM_H_PRE_Y0_CALC, _i) != END_TEST_SUCCES)
    {
        return END_TEST_FAIL;
    }
}
END_TEST

START_TEST(test_authenticated_crypto_aes_gcm_H_pre_Y0_0)
{
    if (test_authenticated_crypto_aes_gcm(VAL_SYM_MODE_GCM_H_PRE_Y0_0, _i) != END_TEST_SUCCES)
    {
        return END_TEST_FAIL;
    }
}
END_TEST

START_TEST(test_authenticated_crypto_aes_gcm_ghash)
{
    if (test_authenticated_crypto_aes_gcm(VAL_SYM_MODE_GCM_GHASH, _i) != END_TEST_SUCCES)
    {
        return END_TEST_FAIL;
    }
}
END_TEST
#endif


#if defined(VALTEST_SYM_ALGO_CHACHA20) && defined(VALTEST_SYM_ALGO_POLY1305)
START_TEST(test_authenticated_crypto_chacha20_aead)
{
    int Index;

    unsupported_if(!valtest_IsChaCha20Supported(), "");
    unsupported_if(!valtest_IsPoly1305Supported(), "");

    for (Index = 0; ; Index++)
    {
        TestVector_ChaCha20_Poly1305_t tv_p;
        ValSymContextPtr_t SymContext_p;
        ValAssetId_t KeyAssetId = VAL_ASSETID_INVALID;
        int funcres;

        tv_p = test_vectors_chacha20_poly1305_get(Index);
        if (tv_p == NULL)
        {
            break;
        }

        funcres = SetupTestContextAndEnvironment(VAL_SYM_ALGO_CIPHER_CHACHA20,
                                                 VAL_SYM_MODE_CIPHER_CHACHA20_AEAD,
                                                 tv_p->Key_p, tv_p->KeyLen,
                                                 ((_i & 4) != 0),
                                                 &SymContext_p,
                                                 &KeyAssetId);
        fail_if(funcres != END_TEST_SUCCES, "SetupTestContextAndEnvironment", funcres);

        funcres = do_SymAuthCryptTest(SymContext_p,
                                      VAL_SYM_MODE_CIPHER_CHACHA20_AEAD,
                                      tv_p->Nonce_p, tv_p->NonceLen,
                                      VAL_SYM_MODE_GCM_NONE,
                                      tv_p->PlainData_p, tv_p->DataLen,
                                      tv_p->CipherData_p, tv_p->DataLen,
                                      tv_p->Aad_p, tv_p->AadLen,
                                      tv_p->Tag_p, 16,
                                      ((_i & 1) != 0),
                                      ((_i == 2) || (_i == 3)));
        fail_if(funcres != END_TEST_SUCCES, "do_SymAuthCryptTest", funcres);

        if (KeyAssetId != VAL_ASSETID_INVALID)
        {
            (void)val_AssetFree(KeyAssetId);
        }

        test_vectors_chacha20_poly1305_release(tv_p);
    }
}
END_TEST
#endif

#ifdef VALTEST_SYM_ALGO_ARIA_CCM
START_TEST(test_authenticated_crypto_aria_ccm)
{
    int Index;
    int Success = 0;
    int Failed = 0;

    unsupported_if(!valtest_IsARIASupported(), "");

    for (Index = 0; ; Index++)
    {
        TestVector_ARIA_CCM_t tv_p;
        ValSymContextPtr_t SymContext_p;
        ValAssetId_t KeyAssetId = VAL_ASSETID_INVALID;

        tv_p = test_vectors_aria_ccm_get(Index);
        if (tv_p == NULL)
        {
            break;
        }

        if (SetupTestContextAndEnvironment(VAL_SYM_ALGO_CIPHER_ARIA,
                                           VAL_SYM_MODE_CIPHER_CCM,
                                           tv_p->Key_p, tv_p->KeyLen,
                                           ((_i & 4) != 0),
                                           &SymContext_p,
                                           &KeyAssetId) == END_TEST_SUCCES)
        {
            if (do_SymAuthCryptTest(SymContext_p,
                                    VAL_SYM_MODE_CIPHER_CCM,
                                    tv_p->Nonce_p, tv_p->NonceLen,
                                    VAL_SYM_MODE_GCM_NONE,
                                    tv_p->Pt_p, tv_p->PtLen,
                                    tv_p->Ct_p, tv_p->CtLen,
                                    tv_p->Aad_p, tv_p->AadLen,
                                    (tv_p->Ct_p+tv_p->PtLen), (tv_p->CtLen - tv_p->PtLen),
                                    ((_i & 1) != 0),
                                    ((_i == 2) || (_i == 3))) == END_TEST_SUCCES)
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

        if (KeyAssetId != VAL_ASSETID_INVALID)
        {
            (void)val_AssetFree(KeyAssetId);
            KeyAssetId = VAL_ASSETID_INVALID;
        }

        test_vectors_aria_ccm_release(tv_p);
    }

    fail_if(Failed, "#wrong tests", Failed);
}
END_TEST
#endif


#ifdef VALTEST_SYM_ALGO_ARIA_GCM
static int
test_authenticated_crypto_aria_gcm(
        const ValSymGCMMode_t Mode,
        const int _i)
{
    int Index;
    int Success = 0;
    int Failed = 0;

    for (Index = 0; ; Index++)
    {
        TestVector_ARIA_GCM_t tv_p;
        ValSymContextPtr_t SymContext_p;
        ValAssetId_t KeyAssetId = VAL_ASSETID_INVALID;
        const uint8_t * HashKey_p = NULL;
        uint32_t HashKeyLen = 0;
        const uint8_t * Tag_p = NULL;
        uint32_t TagLen = 0;

        tv_p = test_vectors_aria_gcm_get(Index);
        if (tv_p == NULL)
        {
            break;
        }

        if (SetupTestContextAndEnvironment(VAL_SYM_ALGO_CIPHER_ARIA,
                                           VAL_SYM_MODE_CIPHER_GCM,
                                           tv_p->Key_p, tv_p->KeyLen,
                                           ((_i & 4) != 0),
                                           &SymContext_p,
                                           &KeyAssetId) == END_TEST_SUCCES)
        {
            {
                ValStatus_t Status;

                if (tv_p->IVLen == 12)
                {
                    uint8_t IV[VAL_SYM_ALGO_ARIA_IV_SIZE];

                    memcpy(IV, tv_p->IV_p, tv_p->IVLen);
                    memcpy(&IV[12], "\x00\x00\x00\x01", 4);
                    Status = val_SymInitIV(SymContext_p, IV, VAL_SYM_ALGO_ARIA_IV_SIZE);
                }
                else
                {
                    Status = val_SymInitIV(SymContext_p, tv_p->Y0_p, VAL_SYM_ALGO_ARIA_IV_SIZE);
                }
                fail_if(Status != VAL_SUCCESS, "val_SymInitIV()=", Status);
            }

            if (Mode != VAL_SYM_MODE_GCM_AUTO)
            {
                HashKey_p = tv_p->H_p;
                HashKeyLen = (uint32_t)tv_p->HLen;
            }

            if ((Mode == VAL_SYM_MODE_GCM_AUTO) ||
                (Mode == VAL_SYM_MODE_GCM_H_PRE_Y0_CALC))
            {
                Tag_p = tv_p->Tag_p;
                TagLen = (uint32_t)tv_p->TagLen;
            }
            else
            {
                Tag_p = tv_p->Ghash_p;
                TagLen = (uint32_t)tv_p->GhashLen;
            }

            if (do_SymAuthCryptTest(SymContext_p,
                                    VAL_SYM_MODE_CIPHER_GCM,
                                    HashKey_p, HashKeyLen,
                                    Mode,
                                    tv_p->Pt_p, tv_p->PtLen,
                                    tv_p->Ct_p, tv_p->CtLen,
                                    tv_p->Aad_p, tv_p->AadLen,
                                    Tag_p, TagLen,
                                    ((_i & 1) != 0),
                                    ((_i == 2) || (_i == 3))) == END_TEST_SUCCES)
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

        if (KeyAssetId != VAL_ASSETID_INVALID)
        {
            (void)val_AssetFree(KeyAssetId);
            KeyAssetId = VAL_ASSETID_INVALID;
        }

        test_vectors_aria_gcm_release(tv_p);
    }

    fail_if(Failed, "#wrong tests", Failed);

    return END_TEST_SUCCES;
}

START_TEST(test_authenticated_crypto_aria_gcm_auto)
{
    unsupported_if(!valtest_IsARIASupported(), "");

    if (test_authenticated_crypto_aria_gcm(VAL_SYM_MODE_GCM_AUTO, _i) != END_TEST_SUCCES)
    {
        return END_TEST_FAIL;
    }
}
END_TEST

START_TEST(test_authenticated_crypto_aria_gcm_H_pre_Y0_calc)
{
    unsupported_if(!valtest_IsARIASupported(), "");

    if (test_authenticated_crypto_aria_gcm(VAL_SYM_MODE_GCM_H_PRE_Y0_CALC, _i) != END_TEST_SUCCES)
    {
        return END_TEST_FAIL;
    }
}
END_TEST

START_TEST(test_authenticated_crypto_aria_gcm_H_pre_Y0_0)
{
    unsupported_if(!valtest_IsARIASupported(), "");

    if (test_authenticated_crypto_aria_gcm(VAL_SYM_MODE_GCM_H_PRE_Y0_0, _i) != END_TEST_SUCCES)
    {
        return END_TEST_FAIL;
    }
}
END_TEST

START_TEST(test_authenticated_crypto_aria_gcm_ghash)
{
    unsupported_if(!valtest_IsARIASupported(), "");

    if (test_authenticated_crypto_aria_gcm(VAL_SYM_MODE_GCM_GHASH, _i) != END_TEST_SUCCES)
    {
        return END_TEST_FAIL;
    }
}
END_TEST
#endif

int
suite_add_test_SymAuthCrypto(
        struct TestSuite * TestSuite_p)
{
    struct TestCase * TestCase_p;

    TestCase_p = sfzutf_tcase_create(TestSuite_p, "SymCrypto_ChiperAuth_Tests");
    if (TestCase_p != NULL)
    {
#if defined(VALTEST_SYM_ALGO_AES_CCM) || \
    defined(VALTEST_SYM_ALGO_AES_GCM) || \
    (defined(VALTEST_SYM_ALGO_CHACHA20) && defined(VALTEST_SYM_ALGO_POLY1305))
        if (sfzutf_tcase_add_fixture(TestCase_p, valtest_initialize, valtest_terminate) != 0)
        {
             goto FuncErrorReturn;
        }

#ifdef VALTEST_SYM_ALGO_AES_CCM
        if (sfzutf_test_add_loop(TestCase_p, test_authenticated_crypto_aes_ccm, 6) != 0) goto FuncErrorReturn;
#endif
#ifdef VALTEST_SYM_ALGO_AES_GCM
        if (sfzutf_test_add_loop(TestCase_p, test_authenticated_crypto_aes_gcm_auto, 6) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add_loop(TestCase_p, test_authenticated_crypto_aes_gcm_H_pre_Y0_calc, 6) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add_loop(TestCase_p, test_authenticated_crypto_aes_gcm_H_pre_Y0_0, 6) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add_loop(TestCase_p, test_authenticated_crypto_aes_gcm_ghash, 2) != 0) goto FuncErrorReturn;
#endif
#if defined(VALTEST_SYM_ALGO_CHACHA20) && defined(VALTEST_SYM_ALGO_POLY1305)
        if (sfzutf_test_add_loop(TestCase_p, test_authenticated_crypto_chacha20_aead, 6) != 0) goto FuncErrorReturn;
#endif
#ifdef VALTEST_SYM_ALGO_ARIA_CCM
        if (sfzutf_test_add_loop(TestCase_p, test_authenticated_crypto_aria_ccm, 6) != 0) goto FuncErrorReturn;
#endif
#ifdef VALTEST_SYM_ALGO_ARIA_GCM
        if (sfzutf_test_add_loop(TestCase_p, test_authenticated_crypto_aria_gcm_auto, 6) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add_loop(TestCase_p, test_authenticated_crypto_aria_gcm_H_pre_Y0_calc, 6) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add_loop(TestCase_p, test_authenticated_crypto_aria_gcm_H_pre_Y0_0, 6) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add_loop(TestCase_p, test_authenticated_crypto_aria_gcm_ghash, 2) != 0) goto FuncErrorReturn;
#endif

#endif
        return 0;
    }

FuncErrorReturn:
    return -1;
}

/* end of file valtest_sym_auth_crypto.c */
