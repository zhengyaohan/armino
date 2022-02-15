/* valtest_hash.c
 *
 * Description: hash tests for MD5, SHA-1, SHA-2
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

/* Test vectors. */
#include "testvectors_hash.h"
#include "testvectors_hmac.h"


/* Foreach macro, for iterating through arrays.
   When reading the next/last element, reading (just) outside the array
   is prevented. */
#define FOREACH(var, index, array)                                      \
    for(((index) = 0),(var) = (array)[0];                               \
        ((index) < sizeof((array))/sizeof((array)[0]));                 \
        (var) = (++(index) < sizeof((array))/sizeof((array)[0]))?       \
            (array)[(index)]: (var))

/* Context for iterating over data in fragments.
   If frags_p == NULL, handle data unfragmented.
   If frags_p[i] == 0, re-use last fragment size.
   If frags_p[i] < 0, cycle back to first fragment size.
   If frags_p[i] > 0, use frags_p[i] as next fragment size. */
typedef struct
{
    const uint8_t * msg_p;
    size_t nbytes_done;
    size_t nbytes_left;
    const int * frags_p;
    size_t frags_ndx;
    size_t frag_len;
} MsgIter_t;


/*----------------------------------------------------------------------------
 * msgiter_init
 *
 * Initialize '*msgiter_p' for iterating over the message defined by
 * 'Msg_p' and 'MsgLen', using fragments as defined by '*Fragments_p'
 * if non-NULL.
 */
static void
msgiter_init(
         MsgIter_t * const msgiter_p,
         const uint8_t * Msg_p,
         size_t MsgLen,
         const int * Fragments_p)
{
    msgiter_p->msg_p = Msg_p;
    msgiter_p->nbytes_done = 0;
    msgiter_p->nbytes_left = MsgLen;
    msgiter_p->frags_p = Fragments_p;
    msgiter_p->frags_ndx = 0;

    if (Fragments_p == NULL)
    {
        msgiter_p->frag_len = MsgLen;
    }
    else
    {
        msgiter_p->frag_len = (size_t)Fragments_p[msgiter_p->frags_ndx++];
    }
}


/*----------------------------------------------------------------------------
 * msgiter_next
 *
 * Update '*msgiter_p' for the next message fragment.
 * Return false if the previous fragment was the last.
 */
static bool
msgiter_next(MsgIter_t * const msgiter_p)
{
    size_t fraglen = msgiter_p->frag_len;
    int next_fraglen;

    msgiter_p->nbytes_done += fraglen;
    msgiter_p->nbytes_left -= fraglen;
    if (msgiter_p->nbytes_left == 0)
    {
        msgiter_p->frag_len = 0;
        return false;
    }

    if (msgiter_p->frags_p == NULL)
    {
        msgiter_p->frag_len = 0;
        return false;
    }

    next_fraglen = msgiter_p->frags_p[msgiter_p->frags_ndx];
    if (next_fraglen < 0)
    {
        msgiter_p->frags_ndx = 0;
        next_fraglen = msgiter_p->frags_p[0];
    }

    if (next_fraglen == 0)
    {
        next_fraglen = (int)fraglen;
        msgiter_p->frags_ndx--;

        if (next_fraglen == 0)
        {
            msgiter_p->frag_len = 0;
            return false;
        }
    }

    msgiter_p->frag_len = MIN(msgiter_p->nbytes_left, (size_t)next_fraglen);
    msgiter_p->frags_ndx++;
    return true;
}


/*----------------------------------------------------------------------------
 * do_hash_test
 *
 * Helper function that runs a single hash test.
 */
static int
do_hash_test(
        int VectorIndex,
        const bool fReadWriteTemp,
        ValSymContextPtr_t SymContext_p,
        TestVector_HASH_t Vector_p,
        const int * const frags_p)
{
    ValStatus_t Status;
    ValOctetsOut_t Digest[VAL_SYM_ALGO_MAX_DIGEST_SIZE];
    ValSize_t DigestSize = 0;
    MsgIter_t msgit;

    memset(Digest, 0, sizeof(Digest));

    msgiter_init(&msgit, Vector_p->Msg_p, Vector_p->MsgLen, frags_p);
    do
    {
#ifdef SFZUTF_USERMODE
        const uint8_t * InCopy_p = (const uint8_t *)(msgit.msg_p + msgit.nbytes_done);
#else
        uint8_t * InCopy_p = (uint8_t *)SFZUTF_MALLOC(msgit.frag_len);
        fail_if(InCopy_p == NULL, "Allocation ", (int)msgit.frag_len);
        memcpy(InCopy_p, (const uint8_t *)(msgit.msg_p + msgit.nbytes_done), msgit.frag_len);
#endif

        if (msgit.nbytes_left == msgit.frag_len)
        {
            DigestSize = sizeof(Digest);
            Status = val_SymHashFinal(SymContext_p,
                                      InCopy_p, msgit.frag_len,
                                      Digest, &DigestSize);
            fail_if(Status != VAL_SUCCESS, "val_SymHashFinal()=", Status);
        }
        else
        {
            Status = val_SymHashUpdate(SymContext_p, InCopy_p, msgit.frag_len);
            fail_if(Status != VAL_SUCCESS, "val_SymHashUpdate()=", Status);

            if (fReadWriteTemp)
            {
                ValSize_t TotalMessageLength;

                DigestSize = sizeof(Digest);
                Status = val_SymReadTokenTemp(SymContext_p,
                                              Digest, &DigestSize,
                                              &TotalMessageLength);
                fail_if(Status != VAL_SUCCESS, "val_SymReadTokenTemp()=", Status);

                Status = val_SymWriteTokenTemp(SymContext_p,
                                               Digest, DigestSize,
                                               TotalMessageLength);
                fail_if(Status != VAL_SUCCESS, "val_SymWriteTokenTemp()=", Status);
            }
        }

#ifndef SFZUTF_USERMODE
        SFZUTF_FREE(InCopy_p);
#endif
    }
    while (msgiter_next(&msgit));

    fail_if(Vector_p->DigestLen != DigestSize,
            "Length mismatch ", (int)DigestSize);
    fail_if(memcmp(Digest, Vector_p->Digest_p, Vector_p->DigestLen) != 0,
            "", VectorIndex);

    return END_TEST_SUCCES;
}


/*----------------------------------------------------------------------------
 * do_hmac_key_hash
 *
 * Helper function that hashes the key if needed.
 */
static bool
do_hmac_key_hash_if_needed(
        ValSymAlgo_t Algorithm,
        const uint8_t * Key_p,
        uint32_t KeySize,
        uint8_t * HashedKey_p,
        ValSize_t * HashedKeySize)
{
    switch (Algorithm)
    {
    case VAL_SYM_ALGO_MAC_HMAC_SHA1:
    case VAL_SYM_ALGO_MAC_HMAC_SHA224:
    case VAL_SYM_ALGO_MAC_HMAC_SHA256:
        if (KeySize <= VAL_SYM_ALGO_MAX_SHA2_MAC_KEY_SIZE)
        {
            return false;
        }
        break;
#ifdef VALTEST_SYM_ALGO_SHA512
    case VAL_SYM_ALGO_MAC_HMAC_SHA384:
    case VAL_SYM_ALGO_MAC_HMAC_SHA512:
        if (KeySize <= VAL_SYM_ALGO_MAX_SHA512_MAC_KEY_SIZE)
        {
            return false;
        }
        break;
#endif
    default:
        return false;
    }
    {
        ValSymContextPtr_t SymContext_p = NULL;
        ValSymAlgo_t HashAlgorithm;
        ValStatus_t Status;

        switch (Algorithm)
        {
        default:
        case VAL_SYM_ALGO_MAC_HMAC_SHA1:
            HashAlgorithm = VAL_SYM_ALGO_HASH_SHA1;
            break;
        case VAL_SYM_ALGO_MAC_HMAC_SHA224:
            HashAlgorithm = VAL_SYM_ALGO_HASH_SHA224;
            break;
        case VAL_SYM_ALGO_MAC_HMAC_SHA256:
            HashAlgorithm = VAL_SYM_ALGO_HASH_SHA256;
            break;
#ifdef VALTEST_SYM_ALGO_SHA512
        case VAL_SYM_ALGO_MAC_HMAC_SHA384:
            HashAlgorithm = VAL_SYM_ALGO_HASH_SHA384;
            break;
        case VAL_SYM_ALGO_MAC_HMAC_SHA512:
            HashAlgorithm = VAL_SYM_ALGO_HASH_SHA512;
            break;
#endif
        }

        Status = val_SymAlloc(HashAlgorithm, VAL_SYM_MODE_NONE, false, &SymContext_p);
        if  (Status == VAL_SUCCESS)
        {
#ifdef SFZUTF_USERMODE
            const uint8_t * KeyCopy_p = Key_p;
#else
            uint8_t * KeyCopy_p = (uint8_t *)SFZUTF_MALLOC(KeySize);
            fail_if(KeyCopy_p == NULL, "Allocation ", (int)KeySize);
            memcpy(KeyCopy_p, Key_p, KeySize);
#endif

            Status = val_SymHashFinal(SymContext_p,
                                      KeyCopy_p,
                                      KeySize,
                                      HashedKey_p,
                                      HashedKeySize);
#ifndef SFZUTF_USERMODE
            SFZUTF_FREE(KeyCopy_p);
#endif
            if  (Status == VAL_SUCCESS)
            {
                return true;
            }

            (void)val_SymRelease(SymContext_p);
        }
    }
    return false;
}


/*----------------------------------------------------------------------------
 * do_hmac_test
 * Helper function that runs a single HMAC test.
 */
static int
do_hmac_test(
        int VectorIndex,
        const bool fReadWriteTemp,
        ValSymContextPtr_t SymContext_p,
        TestVector_HMAC_t Vector_p,
        const int * const frags_p,
        const bool fVerify,
        ValAssetId_t const MacAssetId)
{
    ValStatus_t Status;
    ValOctetsOut_t Mac[VAL_SYM_ALGO_MAX_DIGEST_SIZE];
    ValSize_t MacSize = 0;
    MsgIter_t msgit;

    memset(Mac, 0, sizeof(Mac));

    msgiter_init(&msgit, Vector_p->Msg_p, Vector_p->MsgLen, frags_p);
    do
    {
#ifdef SFZUTF_USERMODE
        const uint8_t * InCopy_p = (const uint8_t *)(msgit.msg_p + msgit.nbytes_done);
#else
        uint8_t * InCopy_p = (uint8_t *)SFZUTF_MALLOC(msgit.frag_len);
        fail_if(InCopy_p == NULL, "Allocation ", (int)msgit.frag_len);
        memcpy(InCopy_p, (const uint8_t *)(msgit.msg_p + msgit.nbytes_done), msgit.frag_len);
#endif

        if (msgit.nbytes_left == msgit.frag_len)
        {
            if (fVerify)
            {
                if (MacAssetId == VAL_ASSETID_INVALID)
                {
                    MacSize = Vector_p->MacLen;
                    memcpy(Mac, Vector_p->Mac_p, MacSize);
                    Status = val_SymMacVerify(SymContext_p,
                                              InCopy_p, msgit.frag_len,
                                              VAL_ASSETID_INVALID,
                                              Mac, MacSize);
                    fail_if(Status != VAL_SUCCESS, "val_SymMacVerify(1)=", Status);
                }
                else
                {
                    Status = val_SymMacVerify(SymContext_p,
                                              InCopy_p, msgit.frag_len,
                                              MacAssetId,
                                              NULL, 0);
                    fail_if(Status != VAL_SUCCESS, "val_SymMacVerify(2)=", Status);
                }
            }
            else
            {
                MacSize = sizeof(Mac);
                Status = val_SymMacGenerate(SymContext_p,
                                            InCopy_p, msgit.frag_len,
                                            Mac, &MacSize);
                fail_if(Status != VAL_SUCCESS, "val_SymMacGenerate()=", Status);
                //fail_if(Vector_p->MacLen != MacSize, "Result mismatch on length", VectorIndex);
                fail_if(memcmp(Mac, Vector_p->Mac_p, Vector_p->MacLen) != 0, "", VectorIndex);
            }
        }
        else
        {
            Status = val_SymMacUpdate(SymContext_p, InCopy_p, msgit.frag_len);
            fail_if(Status != VAL_SUCCESS, "val_SymMacUpdate()=", Status);

            if (fReadWriteTemp)
            {
                ValSize_t TotalMessageLength;

                MacSize = sizeof(Mac);
                Status = val_SymReadTokenTemp(SymContext_p,
                                              Mac, &MacSize,
                                              &TotalMessageLength);
                fail_if(Status != VAL_SUCCESS, "val_SymReadTokenTemp()=", Status);

                Status = val_SymWriteTokenTemp(SymContext_p,
                                               Mac, MacSize,
                                               TotalMessageLength);
                fail_if(Status != VAL_SUCCESS, "val_SymWriteTokenTemp()=", Status);
            }
        }

#ifndef SFZUTF_USERMODE
        SFZUTF_FREE(InCopy_p);
#endif
    }
    while (msgiter_next(&msgit));

    return END_TEST_SUCCES;
}

static int
do_test_hmac(
        const bool fVerify,
        const bool fKeyAsset,
        const bool fMacFinalAsset)
{
    int ndx = g_BeginVector;
    int Failed = 0;
    int SuccessFul = 0;

    while (1)
    {
        TestVector_HMAC_t tv_p;
        uint32_t MacSize = 0;
        ValSymContextPtr_t SymContext_p = NULL;
        ValSymAlgo_t Algorithm = VAL_SYM_ALGO_NONE;
        ValAssetId_t KeyAssetId = VAL_ASSETID_INVALID;
        ValAssetId_t MacAssetId = VAL_ASSETID_INVALID;
        ValPolicyMask_t AssetPolicy;
        ValStatus_t Status;

        tv_p = test_vectors_hmac_get(ndx);
        if (tv_p == NULL)
        {
            break;
        }

        if (fVerify)
        {
            AssetPolicy = VAL_POLICY_MAC_VERIFY;
        }
        else
        {
            AssetPolicy = VAL_POLICY_MAC_GENERATE;
        }
        switch (tv_p->Algorithm)
        {
        default:
            // Not (yet) supported
            test_vectors_hmac_release(tv_p);
            continue;
        case TESTVECTORS_HASH_SHA160:
            Algorithm = VAL_SYM_ALGO_MAC_HMAC_SHA1;
            AssetPolicy |= VAL_POLICY_SHA1;
            MacSize = (160/8);
            break;
        case TESTVECTORS_HASH_SHA224:
            Algorithm = VAL_SYM_ALGO_MAC_HMAC_SHA224;
            AssetPolicy |= VAL_POLICY_SHA224;
            MacSize = (224/8);
            break;
        case TESTVECTORS_HASH_SHA256:
            Algorithm = VAL_SYM_ALGO_MAC_HMAC_SHA256;
            AssetPolicy |= VAL_POLICY_SHA256;
            MacSize = (256/8);
            break;
        case TESTVECTORS_HASH_SHA384:
            Algorithm = VAL_SYM_ALGO_MAC_HMAC_SHA384;
            AssetPolicy |= VAL_POLICY_SHA384;
            MacSize = (384/8);
            break;
        case TESTVECTORS_HASH_SHA512:
            Algorithm = VAL_SYM_ALGO_MAC_HMAC_SHA512;
            AssetPolicy |= VAL_POLICY_SHA512;
            MacSize = (512/8);
            break;
        }

        Status = val_SymAlloc(Algorithm, VAL_SYM_MODE_NONE, false, &SymContext_p);
        fail_if(Status != VAL_SUCCESS, "val_SymAlloc()=", Status);

        if (tv_p->KeyLen != 0)
        {
            if (fKeyAsset)
            {
                uint8_t HashedKeyBuffer[VAL_SYM_ALGO_MAX_DIGEST_SIZE];
                ValOctetsIn_t * Key_p = NULL;
                ValSize_t KeySize = VAL_SYM_ALGO_MAX_DIGEST_SIZE;

                if (tv_p->KeyLen < (MacSize/2))
                {
                    // HMAC Asset require a minimum size
                    goto next_vector;
                }

                if (do_hmac_key_hash_if_needed(Algorithm, tv_p->Key_p, tv_p->KeyLen,
                                               HashedKeyBuffer, &KeySize))
                {
                    Key_p = HashedKeyBuffer;
                }
                else
                {
                    Key_p = tv_p->Key_p;
                    KeySize = tv_p->KeyLen;
                }

                if (!val_IsAccessSecure())
                {
                    AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
                }
                Status = val_AssetAlloc(AssetPolicy, KeySize,
                                        false, false,
                                        VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                                        &KeyAssetId);
                fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(Key)=", Status);

                {
#ifdef SFZUTF_USERMODE
                    const uint8_t * KeyCopy_p = Key_p;
#else
                    uint8_t * KeyCopy_p = (uint8_t *)SFZUTF_MALLOC(KeySize);
                    fail_if(KeyCopy_p == NULL, "Allocation ", (int)KeySize);
                    memcpy(KeyCopy_p, Key_p, KeySize);
#endif
                    Status = val_AssetLoadPlaintext(KeyAssetId, KeyCopy_p, KeySize);
#ifndef SFZUTF_USERMODE
                    SFZUTF_FREE(KeyCopy_p);
#endif
                }
                fail_if(Status != VAL_SUCCESS, "val_AssetLoadPlaintext(Key)=", Status);

                Status = val_SymInitKey(SymContext_p, KeyAssetId, NULL, KeySize);
            }
            else
            {
                Status = val_SymInitKey(SymContext_p, VAL_ASSETID_INVALID,
                                        (ValOctetsIn_Optional_t * const)tv_p->Key_p,
                                        tv_p->KeyLen);
            }
            fail_if(Status != VAL_SUCCESS, "val_SymInitKey()=", Status);
        }

        if (fVerify)
        {
            if (MacSize != tv_p->MacLen)
            {
                // HMAC does not handle partial MAC compare yet
                goto next_vector;
            }

            if (fMacFinalAsset)
            {
                AssetPolicy |= VAL_POLICY_TEMP_MAC;
                if (!val_IsAccessSecure())
                {
                    AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
                }
                Status = val_AssetAlloc(AssetPolicy, tv_p->MacLen,
                                        false, false,
                                        VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                                        &MacAssetId);
                fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(MAC)=", Status);

                {
#ifdef SFZUTF_USERMODE
                    const uint8_t * MacCopy_p = tv_p->Mac_p;
#else
                    uint8_t * MacCopy_p = (uint8_t *)SFZUTF_MALLOC(tv_p->MacLen);
                    fail_if(MacCopy_p == NULL, "Allocation ", (int)tv_p->MacLen);
                    memcpy(MacCopy_p, tv_p->Mac_p, tv_p->MacLen);
#endif
                    Status = val_AssetLoadPlaintext(MacAssetId,
                                                    MacCopy_p, tv_p->MacLen);
#ifndef SFZUTF_USERMODE
                    SFZUTF_FREE(MacCopy_p);
#endif
                }
                fail_if(Status != VAL_SUCCESS, "val_AssetLoadPlaintext(MAC)=", Status);
            }
        }

        if (do_hmac_test(ndx, false, SymContext_p, tv_p, NULL,
                         fVerify, MacAssetId) != END_TEST_SUCCES)
        {
            LOG_CRIT("Process vector %d\n", ndx);
            Failed++;

            if (!g_CleanUp)
            {
                LOG_CRIT("#sucessful tests %d\n", SuccessFul);
                break;
            }
        }
        else
        {
            SuccessFul++;
            SymContext_p = NULL;        // Internally released
        }

next_vector:
        if (SymContext_p != NULL)
        {
            Status = val_SymRelease(SymContext_p);
            fail_if(Status != VAL_SUCCESS, "val_SymRelease()=", Status);
        }
        if (KeyAssetId != VAL_ASSETID_INVALID)
        {
            Status = val_AssetFree(KeyAssetId);
            fail_if(Status != VAL_SUCCESS, "val_AssetFree(Key)=", Status);
        }
        if (MacAssetId != VAL_ASSETID_INVALID)
        {
            Status = val_AssetFree(MacAssetId);
            fail_if(Status != VAL_SUCCESS, "val_AssetFree(MAC)=", Status);
        }

        test_vectors_hmac_release(tv_p);

        if (!g_RepeatVector)
        {
            ndx++;
        }
    }

    if (Failed)
    {
        LOG_CRIT("#wrong tests %d\n", Failed);
        return END_TEST_FAIL;
    }
    return END_TEST_SUCCES;
}


static int
do_test_hmac_multipart(
        const bool fVerify,
        const bool fUseTokenTemp)
{
    static const int MULTIPART_HMAC_TEST64[] = {1*64, 0};
    static const int MULTIPART_HMAC_TEST128[] = {1*128, 0};
    int ndx;

    for (ndx = 0; ; ndx++)
    {
        TestVector_HMAC_t tv_p;
        const int * frags_p;
        ValSymAlgo_t Algorithm;

        tv_p = test_vectors_hmac_get(ndx);
        if (tv_p == NULL)
        {
            break;
        }

        Algorithm = VAL_SYM_ALGO_NONE;
        frags_p = NULL;
        switch (tv_p->Algorithm)
        {
        default:
            // Not (yet) supported
            break;
        case TESTVECTORS_HASH_SHA160:
            if (tv_p->MsgLen >= 64)
            {
                Algorithm = VAL_SYM_ALGO_MAC_HMAC_SHA1;
                frags_p = MULTIPART_HMAC_TEST64;
            }
            break;
        case TESTVECTORS_HASH_SHA224:
            if (tv_p->MsgLen >= 64)
            {
                Algorithm = VAL_SYM_ALGO_MAC_HMAC_SHA224;
                frags_p = MULTIPART_HMAC_TEST64;
            }
            break;
        case TESTVECTORS_HASH_SHA256:
            if (tv_p->MsgLen >= 64)
            {
                Algorithm = VAL_SYM_ALGO_MAC_HMAC_SHA256;
                frags_p = MULTIPART_HMAC_TEST64;
            }
            break;
        case TESTVECTORS_HASH_SHA384:
            if (tv_p->MsgLen >= 128)
            {
                Algorithm = VAL_SYM_ALGO_MAC_HMAC_SHA384;
                frags_p = MULTIPART_HMAC_TEST128;
            }
            break;
        case TESTVECTORS_HASH_SHA512:
            if (tv_p->MsgLen >= 128)
            {
                Algorithm = VAL_SYM_ALGO_MAC_HMAC_SHA512;
                frags_p = MULTIPART_HMAC_TEST128;
            }
            break;
        }

        if (frags_p != NULL)
        {
            ValSymContextPtr_t SymContext_p = NULL;
            ValStatus_t Status;

            LOG_INFO("Process vector %d\n", ndx);

            Status = val_SymAlloc(Algorithm, VAL_SYM_MODE_NONE, fUseTokenTemp, &SymContext_p);
            fail_if(Status != VAL_SUCCESS, "val_SymAlloc()=", Status);

            if (tv_p->KeyLen != 0)
            {
                Status = val_SymInitKey(SymContext_p, VAL_ASSETID_INVALID,
                                        (ValOctetsIn_Optional_t * const)tv_p->Key_p,
                                        tv_p->KeyLen);
                fail_if(Status != VAL_SUCCESS, "val_SymInitKey()=", Status);
            }

            if (do_hmac_test(ndx, fUseTokenTemp, SymContext_p,
                             tv_p, frags_p, fVerify, VAL_ASSETID_INVALID) != 0)
            {
                return -1;
            }
        }

        test_vectors_hmac_release(tv_p);
    }

    return END_TEST_SUCCES;
}


START_TEST(test_hash)
{
    int ndx;
    int failed;

    for (ndx = 0, failed = 0; ; ndx++)
    {
        ValSymContextPtr_t SymContext_p = NULL;
        ValSymAlgo_t Algorithm = VAL_SYM_ALGO_NONE;
        TestVector_HASH_t tv_p;
        ValStatus_t Status;

        tv_p = test_vectors_hash_get(ndx);
        if (tv_p == NULL)
        {
            break;
        }

        LOG_INFO("Process vector %d\n", ndx);

        switch (tv_p->Algorithm)
        {
        default:
            // Not (yet) supported
            test_vectors_hash_release(tv_p);
            continue;
        case TESTVECTORS_HASH_SHA160:
            Algorithm = VAL_SYM_ALGO_HASH_SHA1;
            break;
        case TESTVECTORS_HASH_SHA224:
            Algorithm = VAL_SYM_ALGO_HASH_SHA224;
            break;
        case TESTVECTORS_HASH_SHA256:
            Algorithm = VAL_SYM_ALGO_HASH_SHA256;
            break;
#ifdef VALTEST_SYM_ALGO_SHA512
        case TESTVECTORS_HASH_SHA384:
            Algorithm = VAL_SYM_ALGO_HASH_SHA384;
            break;
        case TESTVECTORS_HASH_SHA512:
            Algorithm = VAL_SYM_ALGO_HASH_SHA512;
            break;
#endif
        }

        Status = val_SymAlloc(Algorithm, VAL_SYM_MODE_NONE, false, &SymContext_p);
        fail_if(Status != VAL_SUCCESS, "val_SymAlloc()=", Status);

        if (do_hash_test(ndx, false, SymContext_p, tv_p, NULL) != END_TEST_SUCCES)
        {
            LOG_CRIT("Process vector %d\n", ndx);
            failed++;
        }
        test_vectors_hash_release(tv_p);
    }

    fail_if(failed, "#wrong tests", failed);
}
END_TEST

START_TEST(test_hash_multipart)
{
    static const int MULTIPART_HASH_TEST1[] = {128, 0};
    static const int MULTIPART_HASH_TEST2[] = {5*128, 11*128, 1*128, -1};
    static const int * MULTIPART_HASH_TESTS[] = {
        MULTIPART_HASH_TEST1,
        MULTIPART_HASH_TEST2
    };
    int ndx;

    for (ndx = 0; ; ndx++)
    {
        TestVector_HASH_t tv_p;

        tv_p = test_vectors_hash_get(ndx);
        if (tv_p == NULL)
        {
            break;
        }

        if (tv_p->MsgLen > 1000)
        {
            ValSymAlgo_t Algorithm = VAL_SYM_ALGO_NONE;
            const int * frags_p;
            unsigned int ndx2;

            LOG_INFO("Process vector %d\n", ndx);

            switch (tv_p->Algorithm)
            {
            default:
                // Not (yet) supported
                test_vectors_hash_release(tv_p);
                continue;
            case TESTVECTORS_HASH_SHA160:
                Algorithm = VAL_SYM_ALGO_HASH_SHA1;
                break;
            case TESTVECTORS_HASH_SHA224:
                Algorithm = VAL_SYM_ALGO_HASH_SHA224;
                break;
            case TESTVECTORS_HASH_SHA256:
                Algorithm = VAL_SYM_ALGO_HASH_SHA256;
                break;
#ifdef VALTEST_SYM_ALGO_SHA512
            case TESTVECTORS_HASH_SHA384:
                Algorithm = VAL_SYM_ALGO_HASH_SHA384;
                break;
            case TESTVECTORS_HASH_SHA512:
                Algorithm = VAL_SYM_ALGO_HASH_SHA512;
                break;
#endif
            }

            FOREACH(frags_p, ndx2, MULTIPART_HASH_TESTS)
            {
                ValSymContextPtr_t SymContext_p = NULL;
                ValStatus_t Status;

                Status = val_SymAlloc(Algorithm, VAL_SYM_MODE_NONE, ((_i & 1) != 0), &SymContext_p);
                fail_if(Status != VAL_SUCCESS, "val_SymAlloc()=", Status);

                if (do_hash_test(ndx, ((_i & 1) != 0), SymContext_p, tv_p, frags_p) != END_TEST_SUCCES)
                {
                    LOG_CRIT("Process vector %d\n", ndx);
                    return END_TEST_FAIL;
                }
            }
        }
        test_vectors_hash_release(tv_p);
    }
}
END_TEST

#ifdef SFZUTF_USERMODE
START_TEST(test_hash_unaligned_buffer)
{
    static uint8_t Buffer[512];
    int ndx;

    for (ndx = 0; ; ndx++)
    {
        TestVector_HASH_t tv_p;

        tv_p = test_vectors_hash_get(ndx);
        if (tv_p == NULL)
        {
            break;
        }

        if ((tv_p->MsgLen > 200) && (tv_p->MsgLen < 500))
        {
            ValSymAlgo_t Algorithm = VAL_SYM_ALGO_NONE;
            int disalign;

            LOG_INFO("Process vector %d\n", ndx);

            switch (tv_p->Algorithm)
            {
            default:
                // Not (yet) supported
                test_vectors_hash_release(tv_p);
                continue;
            case TESTVECTORS_HASH_SHA160:
                Algorithm = VAL_SYM_ALGO_HASH_SHA1;
                break;
            case TESTVECTORS_HASH_SHA224:
                Algorithm = VAL_SYM_ALGO_HASH_SHA224;
                break;
            case TESTVECTORS_HASH_SHA256:
                Algorithm = VAL_SYM_ALGO_HASH_SHA256;
                break;
#ifdef VALTEST_SYM_ALGO_SHA512
            case TESTVECTORS_HASH_SHA384:
                Algorithm = VAL_SYM_ALGO_HASH_SHA384;
                break;
            case TESTVECTORS_HASH_SHA512:
                Algorithm = VAL_SYM_ALGO_HASH_SHA512;
                break;
#endif
            }

            for (disalign = 1; disalign < 4; disalign++)
            {
                TestVector_HASH_Rec_t tvrec;
                ValSymContextPtr_t SymContext_p = NULL;
                ValStatus_t Status;

                Status = val_SymAlloc(Algorithm, VAL_SYM_MODE_NONE, false, &SymContext_p);
                fail_if(Status != VAL_SUCCESS, "val_SymAlloc()=", Status);

                tvrec = *tv_p; // copy structure
                tvrec.Msg_p = Buffer + disalign;
                memset(Buffer, 0xdc, sizeof(Buffer));
                memcpy(&Buffer[disalign], tv_p->Msg_p, tv_p->MsgLen);

                if (do_hash_test(ndx, false, SymContext_p, &tvrec, NULL) != END_TEST_SUCCES)
                {
                    LOG_CRIT("Process vector %d\n", ndx);
                    return END_TEST_FAIL;
                }
            }
        }

        test_vectors_hash_release(tv_p);
    }
}
END_TEST
#endif

START_TEST(test_hmac_generate)
{
    if (do_test_hmac(false, false, false) != END_TEST_SUCCES)
    {
        return END_TEST_FAIL;
    }
}
END_TEST

START_TEST(test_hmac_verify)
{
    if (do_test_hmac(true, false, false) != END_TEST_SUCCES)
    {
        return END_TEST_FAIL;
    }
}
END_TEST

START_TEST(test_hmac_generate_key_asset)
{
    if (do_test_hmac(false, true, false) != END_TEST_SUCCES)
    {
        return END_TEST_FAIL;
    }
}
END_TEST

START_TEST(test_hmac_verify_key_asset)
{
    if (do_test_hmac(true, true, false) != END_TEST_SUCCES)
    {
        return END_TEST_FAIL;
    }
}
END_TEST

START_TEST(test_hmac_verify_mac_asset)
{
    if (do_test_hmac(true, false, true) != END_TEST_SUCCES)
    {
        return END_TEST_FAIL;
    }
}
END_TEST

START_TEST(test_hmac_verify_key_mac_asset)
{
    if (do_test_hmac(true, true, true) != END_TEST_SUCCES)
    {
        return END_TEST_FAIL;
    }
}
END_TEST

START_TEST(test_hmac_multipart_generate)
{
    if (do_test_hmac_multipart(false, (_i & 1)) != END_TEST_SUCCES)
    {
        return END_TEST_FAIL;
    }
}
END_TEST

START_TEST(test_hmac_multipart_verify)
{
    if (do_test_hmac_multipart(true, (_i & 1)) != END_TEST_SUCCES)
    {
        return END_TEST_FAIL;
    }
}
END_TEST

#ifdef SFZUTF_USERMODE
START_TEST(test_hmac_unaligned_buffer)
{
    static uint8_t Buffer[512];
    int ndx;

    for (ndx = 0; ; ndx++)
    {
        TestVector_HMAC_t tv_p;

        tv_p = test_vectors_hmac_get(ndx);
        if (tv_p == NULL)
        {
            break;
        }

        if ((tv_p->MsgLen > 200) && (tv_p->MsgLen < 500))
        {
            ValSymAlgo_t Algorithm = VAL_SYM_ALGO_NONE;
            int disalign;

            LOG_INFO("Process vector %d\n", ndx);

            switch (tv_p->Algorithm)
            {
            default:
                // Not (yet) supported
                test_vectors_hmac_release(tv_p);
                continue;
            case TESTVECTORS_HASH_SHA160:
                Algorithm = VAL_SYM_ALGO_HASH_SHA1;
                break;
            case TESTVECTORS_HASH_SHA224:
                Algorithm = VAL_SYM_ALGO_HASH_SHA224;
                break;
            case TESTVECTORS_HASH_SHA256:
                Algorithm = VAL_SYM_ALGO_HASH_SHA256;
                break;
#ifdef VALTEST_SYM_ALGO_SHA512
            case TESTVECTORS_HASH_SHA384:
                Algorithm = VAL_SYM_ALGO_HASH_SHA384;
                break;
            case TESTVECTORS_HASH_SHA512:
                Algorithm = VAL_SYM_ALGO_HASH_SHA512;
                break;
#endif
            }

            for (disalign = 1; disalign < 4; disalign++)
            {
                TestVector_HMAC_Rec_t tvrec;
                ValSymContextPtr_t SymContext_p = NULL;
                ValStatus_t Status;
                size_t offs;

                Status = val_SymAlloc(Algorithm, VAL_SYM_MODE_NONE, false, &SymContext_p);
                fail_if(Status != VAL_SUCCESS, "val_SymAlloc()=", Status);

                offs = (tv_p->KeyLen + 3 + 3) & (size_t)~3;
                tvrec = *tv_p; // copy structure
                tvrec.Key_p = Buffer + disalign;
                tvrec.Msg_p = Buffer + offs + (4 - disalign);
                memset(Buffer, 0xdc, sizeof(Buffer));
                memcpy((Buffer + disalign), tv_p->Key_p, tv_p->KeyLen);
                memcpy((Buffer + offs + (4 - disalign)), tv_p->Msg_p, tv_p->MsgLen);

                do_hmac_test(ndx, false, SymContext_p, &tvrec, NULL,
                             false, VAL_ASSETID_INVALID);
            }
        }

        test_vectors_hmac_release(tv_p);
    }
}
END_TEST
#endif


/*----------------------------------------------------------------------------
 * Some negative Hash tests
 *--------------------------------------------------------------------------*/
START_TEST(test_hash_invalid_algorithm)
{
    ValSymContextPtr_t SymContext_p = NULL;
    ValOctetsOut_t Digest[VAL_SYM_ALGO_MAX_DIGEST_SIZE];
    ValSize_t DigestSize = sizeof(Digest);
    uint8_t hashData[128];
    ValStatus_t Status;

    memset(hashData, 'a', sizeof(hashData));

    Status = val_SymAlloc(VAL_SYM_ALGO_MAC_HMAC_SHA256,
                          VAL_SYM_MODE_NONE,
                          false,
                          &SymContext_p);
    fail_if(Status != VAL_SUCCESS, "val_SymAlloc()=", Status);

    Status = val_SymHashFinal(SymContext_p,
                              hashData,
                              sizeof(hashData),
                              Digest,
                              &DigestSize);
    fail_if(Status != VAL_INVALID_ALGORITHM, "val_SymHashFinal()=", Status);

    Status = val_SymHashUpdate(SymContext_p,
                               hashData,
                               sizeof(hashData));
    fail_if(Status != VAL_INVALID_ALGORITHM, "val_SymHashUpdate()=", Status);

    Status = val_SymRelease(SymContext_p);
    fail_if(Status != VAL_SUCCESS, "val_SymRelease()=", Status);
}
END_TEST

START_TEST(test_hash_buffer_too_small)
{
    ValSymContextPtr_t SymContext_p = NULL;
    ValOctetsOut_t Digest[VAL_SYM_ALGO_MAX_DIGEST_SIZE];
    ValSize_t DigestSize = sizeof(Digest);
    uint8_t hashData[128];
    ValStatus_t Status;

    memset(hashData, 'a', sizeof(hashData));

    Status = val_SymAlloc(VAL_SYM_ALGO_HASH_SHA256,
                          VAL_SYM_MODE_NONE,
                          false,
                          &SymContext_p);
    fail_if(Status != VAL_SUCCESS, "val_SymAlloc()=", Status);

    DigestSize = (224/8);
    Status = val_SymHashFinal(SymContext_p,
                              hashData,
                              sizeof(hashData),
                              Digest,
                              &DigestSize);
    fail_if(Status != VAL_BUFFER_TOO_SMALL, "val_SymHashFinal()=", Status);

    Status = val_SymRelease(SymContext_p);
    fail_if(Status != VAL_SUCCESS, "val_SymRelease()=", Status);
}
END_TEST


START_TEST(test_hash_arguments)
{
    ValSymContextPtr_t SymContext_p = NULL;
    uint8_t * DataBuffer_p;
    ValOctetsOut_t Digest[VAL_SYM_ALGO_MAX_DIGEST_SIZE];
    ValSize_t DigestSize = sizeof(Digest);
    ValStatus_t Status;

    DataBuffer_p = (uint8_t *)SFZUTF_MALLOC(100);
    fail_if(DataBuffer_p == NULL, "Allocation ", 100);

    Status = val_SymAlloc(VAL_SYM_ALGO_HASH_SHA256,
                          VAL_SYM_MODE_NONE,
                          false,
                          &SymContext_p);
    fail_if(Status != VAL_SUCCESS, "val_SymAlloc()=", Status);

    Status = val_SymHashFinal(NULL, DataBuffer_p, 64, Digest, &DigestSize);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_SymHashFinal(BadArgument1)=", Status);

    Status = val_SymHashFinal(SymContext_p, NULL, 64, Digest, &DigestSize);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_SymHashFinal(BadArgument2)=", Status);

    Status = val_SymHashFinal(SymContext_p, DataBuffer_p, 64, NULL, &DigestSize);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_SymHashFinal(BadArgument4)=", Status);

    Status = val_SymHashFinal(SymContext_p, DataBuffer_p, 64, Digest, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_SymHashFinal(BadArgument5)=", Status);

    Status = val_SymHashUpdate(NULL, DataBuffer_p, 64);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_SymHashUpdate(BadArgument1)=", Status);

    Status = val_SymHashUpdate(SymContext_p, NULL, 64);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_SymHashUpdate(BadArgument2)=", Status);

    Status = val_SymHashUpdate(SymContext_p, DataBuffer_p, 0);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_SymHashUpdate(BadArgument3L)=", Status);

    Status = val_SymHashUpdate(SymContext_p, DataBuffer_p, 32);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_SymHashUpdate(BadArgument3M)=", Status);

    Status = val_SymHashFinal(SymContext_p, NULL, 0, Digest, &DigestSize);
    fail_if(Status != VAL_SUCCESS, "val_SymHashFinal(NULL HASH)=", Status);

    SFZUTF_FREE(DataBuffer_p);
}
END_TEST


START_TEST(test_hmac_invalid_algorithm)
{
    ValSymContextPtr_t SymContext_p = NULL;
    ValOctetsOut_t Mac[VAL_SYM_ALGO_MAX_DIGEST_SIZE];
    ValSize_t MacSize = sizeof(Mac);
    uint8_t macData[128];
    ValStatus_t Status;

    memset(macData, 'a', sizeof(macData));

    Status = val_SymAlloc(VAL_SYM_ALGO_HASH_SHA256,
                          VAL_SYM_MODE_NONE,
                          false,
                          &SymContext_p);
    fail_if(Status != VAL_SUCCESS, "val_SymAlloc()=", Status);

    Status = val_SymMacGenerate(SymContext_p,
                                macData,
                                sizeof(macData),
                                Mac,
                                &MacSize);
    fail_if(Status != VAL_INVALID_ALGORITHM, "val_SymMacGenerate()=", Status);

    Status = val_SymMacVerify(SymContext_p,
                              macData,
                              sizeof(macData),
                              VAL_ASSETID_INVALID,
                              Mac,
                              MacSize);
    fail_if(Status != VAL_INVALID_ALGORITHM, "val_SymMacVerify()=", Status);

    Status = val_SymMacUpdate(SymContext_p,
                              macData,
                              sizeof(macData));
    fail_if(Status != VAL_INVALID_ALGORITHM, "val_SymMacUpdate()=", Status);

    Status = val_SymRelease(SymContext_p);
    fail_if(Status != VAL_SUCCESS, "val_SymRelease()=", Status);
}
END_TEST

START_TEST(test_hmac_buffer_too_small)
{
    ValSymContextPtr_t SymContext_p = NULL;
    ValOctetsOut_t Mac[VAL_SYM_ALGO_MAX_DIGEST_SIZE];
    ValSize_t MacSize = sizeof(Mac);
    uint8_t macData[128];
    ValStatus_t Status;

    memset(macData, 'a', sizeof(macData));

    Status = val_SymAlloc(VAL_SYM_ALGO_MAC_HMAC_SHA256,
                          VAL_SYM_MODE_NONE,
                          false,
                          &SymContext_p);
    fail_if(Status != VAL_SUCCESS, "val_SymAlloc()=", Status);

    MacSize = (224/8);
    Status = val_SymMacGenerate(SymContext_p,
                                macData,
                                sizeof(macData),
                                Mac,
                                &MacSize);
    fail_if(Status != VAL_BUFFER_TOO_SMALL, "val_SymMacGenerate()=", Status);

    Status = val_SymRelease(SymContext_p);
    fail_if(Status != VAL_SUCCESS, "val_SymRelease()=", Status);

}
END_TEST


START_TEST(test_hmac_arguments)
{
    ValSymContextPtr_t SymContext_p = NULL;
    uint8_t * DataBuffer_p;
    ValOctetsOut_t Mac[VAL_SYM_ALGO_MAX_DIGEST_SIZE];
    ValSize_t MacSize = sizeof(Mac);
    ValStatus_t Status;

    DataBuffer_p = (uint8_t *)SFZUTF_MALLOC(100);
    fail_if(DataBuffer_p == NULL, "Allocation ", 100);

    Status = val_SymAlloc(VAL_SYM_ALGO_MAC_HMAC_SHA256,
                          VAL_SYM_MODE_NONE,
                          false,
                          &SymContext_p);
    fail_if(Status != VAL_SUCCESS, "val_SymAlloc()=", Status);

    Status = val_SymMacGenerate(NULL, DataBuffer_p, 64, Mac, &MacSize);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_SymMacGenerate(BadArgument1)=", Status);

    Status = val_SymMacGenerate(SymContext_p, NULL, 64, Mac, &MacSize);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_SymMacGenerate(BadArgument2)=", Status);

    Status = val_SymMacGenerate(SymContext_p, DataBuffer_p, 64, NULL, &MacSize);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_SymMacGenerate(BadArgument4)=", Status);

    Status = val_SymMacGenerate(SymContext_p, DataBuffer_p, 64, Mac, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_SymMacGenerate(BadArgument5)=", Status);

    Status = val_SymMacVerify(NULL, DataBuffer_p, 64, VAL_ASSETID_INVALID, Mac, MacSize);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_SymMacVerify(BadArgument1)=", Status);

    Status = val_SymMacVerify(SymContext_p, NULL, 64, VAL_ASSETID_INVALID, Mac, MacSize);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_SymMacVerify(BadArgument2)=", Status);

    Status = val_SymMacVerify(SymContext_p, DataBuffer_p, 64, VAL_ASSETID_INVALID, NULL, MacSize);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_SymMacVerify(BadArgument5)=", Status);

    Status = val_SymMacUpdate(NULL, DataBuffer_p, 64);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_SymMacUpdate(BadArgument1)=", Status);

    Status = val_SymMacUpdate(SymContext_p, NULL, 64);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_SymMacUpdate(BadArgument2)=", Status);

    Status = val_SymMacUpdate(SymContext_p, DataBuffer_p, 0);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_SymMacUpdate(BadArgument3L)=", Status);

    Status = val_SymMacUpdate(SymContext_p, DataBuffer_p, 32);
    fail_if(Status != VAL_BAD_ARGUMENT, "val_SymMacUpdate(BadArgument3M)=", Status);

    Status = val_SymRelease(SymContext_p);
    fail_if(Status != VAL_SUCCESS, "val_SymAlloc()=", Status);

    SFZUTF_FREE(DataBuffer_p);
}
END_TEST


int
suite_add_test_SymHashHMac(
        struct TestSuite * TestSuite_p)
{
    struct TestCase * TestCase_p;

    TestCase_p = sfzutf_tcase_create(TestSuite_p, "SymCrypto_Hash_and_HMac_Tests");
    if (TestCase_p != NULL)
    {
        if (sfzutf_tcase_add_fixture(TestCase_p, valtest_initialize, valtest_terminate) != 0)
        {
             goto FuncErrorReturn;
        }

        if (sfzutf_test_add(TestCase_p, test_hash) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add_loop(TestCase_p, test_hash_multipart, 2) != 0) goto FuncErrorReturn;
#ifdef SFZUTF_USERMODE
        if (sfzutf_test_add(TestCase_p, test_hash_unaligned_buffer) != 0) goto FuncErrorReturn;
#endif

        if (sfzutf_test_add(TestCase_p, test_hmac_generate) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add(TestCase_p, test_hmac_verify) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add_loop(TestCase_p, test_hmac_multipart_generate, 2) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add_loop(TestCase_p, test_hmac_multipart_verify, 2) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add(TestCase_p, test_hmac_generate_key_asset) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add(TestCase_p, test_hmac_verify_key_asset) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add(TestCase_p, test_hmac_verify_mac_asset) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add(TestCase_p, test_hmac_verify_key_mac_asset) != 0) goto FuncErrorReturn;
#ifdef SFZUTF_USERMODE
        if (sfzutf_test_add(TestCase_p, test_hmac_unaligned_buffer) != 0) goto FuncErrorReturn;
#endif

        if (sfzutf_test_add(TestCase_p, test_hash_invalid_algorithm) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add(TestCase_p, test_hash_buffer_too_small) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add(TestCase_p, test_hmac_invalid_algorithm) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add(TestCase_p, test_hmac_buffer_too_small) != 0) goto FuncErrorReturn;

        if (valtest_StrictArgsCheck())
        {
            if (sfzutf_test_add(TestCase_p, test_hash_arguments) != 0) goto FuncErrorReturn;
            if (sfzutf_test_add(TestCase_p, test_hmac_arguments) != 0) goto FuncErrorReturn;
        }
        return 0;
    }

FuncErrorReturn:
    return -1;
}

/* end of file valtest_hash.c */
