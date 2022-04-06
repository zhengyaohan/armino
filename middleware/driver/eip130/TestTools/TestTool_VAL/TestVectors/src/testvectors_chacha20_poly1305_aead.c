/* testvectors_chacha20_poly1305_aead.c
 *
 * Description: Test vectors for ChaCha20-Poly1305-AEAD.
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

#include "testvectors_chacha20_poly1305_aead.h"
#include "testvectors_chacha20_poly1305_aead_data.h"

/* Macros used to build the test vectors. */
#define TEST_VECTOR_CHACHA20_POLY1305(key, aad, nonce, ptx, ctx, tag)   \
    {                                                                   \
        key, aad, nonce, ptx, ctx, tag,                                 \
        sizeof(key), sizeof(aad), sizeof(nonce), sizeof(ptx)            \
    }

static const TestVector_ChaCha20_Poly1305_Rec_t chacha20_poly1305_test_vectors [] =
{
    TEST_VECTOR_CHACHA20_POLY1305(
            ChaCha20_Poly1305_TV1_Key,
            ChaCha20_Poly1305_TV1_AAD,
            ChaCha20_Poly1305_TV1_Nonce,
            ChaCha20_Poly1305_TV1_PlainData,
            ChaCha20_Poly1305_TV1_CipherData,
            ChaCha20_Poly1305_TV1_Tag),

    TEST_VECTOR_CHACHA20_POLY1305(
            ChaCha20_Poly1305_TV2_Key,
            ChaCha20_Poly1305_TV2_AAD,
            ChaCha20_Poly1305_TV2_Nonce,
            ChaCha20_Poly1305_TV2_PlainData,
            ChaCha20_Poly1305_TV2_CipherData,
            ChaCha20_Poly1305_TV2_Tag),
};

/* The function API for accessing the vectors. */
int
test_vectors_chacha20_poly1305_num(void)
{
    return sizeof(chacha20_poly1305_test_vectors) / sizeof(chacha20_poly1305_test_vectors[0]);
}

TestVector_ChaCha20_Poly1305_t
test_vectors_chacha20_poly1305_get(int Index)
{
    if (Index >= test_vectors_chacha20_poly1305_num())
        return NULL;

    return &chacha20_poly1305_test_vectors[Index];
}

void
test_vectors_chacha20_poly1305_release(TestVector_ChaCha20_Poly1305_t Vector_p)
{
    /* Test vectors are statically defined => nothing to do. */
    IDENTIFIER_NOT_USED(Vector_p);
}


/* end of file testvectors_chacha20_poly1305_aead.c */
