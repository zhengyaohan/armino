/* testvectors_chacha20.c
 *
 * Description: Test vectors for ChaCha20 encrypt.
 */

/*****************************************************************************
* Copyright (c) 2016-2018 INSIDE Secure B.V. All Rights Reserved.
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

#include "basic_defs.h"

#include "testvectors_chacha20.h"
#include "testvectors_chacha20_data.h"

/* Macros used to build the test vectors. */
#define TEST_VECTOR_CHACHA20(key, nonce, cnt, ptx, ctx)      \
    {                                                        \
        key, nonce, cnt, ptx, ctx,                           \
        sizeof(key), sizeof(nonce), sizeof(cnt), sizeof(ptx) \
    }

static const TestVector_ChaCha20_Rec_t chacha20_test_vectors [] =
{
    TEST_VECTOR_CHACHA20(ChaCha20_KAT1_Key,
                         ChaCha20_KAT1_Nonce, ChaCha20_KAT1_Counter,
                         ChaCha20_KAT1_PlainData, ChaCha20_KAT1_CipherData),
    TEST_VECTOR_CHACHA20(ChaCha20_KAT2_Key,
                         ChaCha20_KAT2_Nonce, ChaCha20_KAT2_Counter,
                         ChaCha20_KAT2_PlainData, ChaCha20_KAT2_CipherData),
    TEST_VECTOR_CHACHA20(ChaCha20_KAT3_Key,
                         ChaCha20_KAT3_Nonce, ChaCha20_KAT3_Counter,
                         ChaCha20_KAT3_PlainData, ChaCha20_KAT3_CipherData),
    TEST_VECTOR_CHACHA20(ChaCha20_KAT4_Key,
                         ChaCha20_KAT4_Nonce, ChaCha20_KAT4_Counter,
                         ChaCha20_KAT4_PlainData, ChaCha20_KAT4_CipherData),
    TEST_VECTOR_CHACHA20(ChaCha20_KAT5_Key,
                         ChaCha20_KAT5_Nonce, ChaCha20_KAT5_Counter,
                         ChaCha20_KAT5_PlainData, ChaCha20_KAT5_CipherData),
    TEST_VECTOR_CHACHA20(ChaCha20_KAT6_Key,
                         ChaCha20_KAT6_Nonce, ChaCha20_KAT6_Counter,
                         ChaCha20_KAT6_PlainData, ChaCha20_KAT6_CipherData),
    TEST_VECTOR_CHACHA20(ChaCha20_TV1_Key,
                         ChaCha20_TV1_Nonce, ChaCha20_TV1_Counter,
                         ChaCha20_TV1_PlainData, ChaCha20_TV1_CipherData),
    TEST_VECTOR_CHACHA20(ChaCha20_TV2_Key,
                         ChaCha20_TV2_Nonce, ChaCha20_TV2_Counter,
                         ChaCha20_TV2_PlainData, ChaCha20_TV2_CipherData),
    TEST_VECTOR_CHACHA20(ChaCha20_TV3_Key,
                         ChaCha20_TV3_Nonce, ChaCha20_TV3_Counter,
                         ChaCha20_TV3_PlainData, ChaCha20_TV3_CipherData),
};


/* The function API for accessing the vectors. */
int
test_vectors_chacha20_num(void)
{
    return sizeof(chacha20_test_vectors) / sizeof(chacha20_test_vectors[0]);
}

TestVector_ChaCha20_t
test_vectors_chacha20_get(int Index)
{
    if (Index >= test_vectors_chacha20_num())
        return NULL;

    return &chacha20_test_vectors[Index];
}

void
test_vectors_chacha20_release(TestVector_ChaCha20_t Vector_p)
{
    /* Test vectors are statically defined => nothing to do. */
    IDENTIFIER_NOT_USED(Vector_p);
}


/* end of file testvectors_chacha20.c */
