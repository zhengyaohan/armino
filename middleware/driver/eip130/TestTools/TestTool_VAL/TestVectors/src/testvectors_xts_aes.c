/* testvectors_xts_aes_basic.c
 *
 * Description: Test vectors for AES in XTS mode.
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

#include "basic_defs.h"

#include "testvectors_xts_aes.h"
#include "testvectors_xts_aes_data.h"

/* Macros used to build the test vectors. */
#define TEST_VECTOR_XTS_AES(key, ptx, ctx, tweak) \
    {                                             \
        key, ptx, ctx, tweak,                     \
        sizeof(key), sizeof(ptx)                  \
    }

static const TestVector_XTS_AES_Rec_t xts_aes_test_vectors [] =
{
    TEST_VECTOR_XTS_AES(V128_1_key, V128_1_ptx, V128_1_ctx, V128_1_tweak),
    TEST_VECTOR_XTS_AES(V128_2_key, V128_2_ptx, V128_2_ctx, V128_2_tweak),
    TEST_VECTOR_XTS_AES(V128_3_key, V128_3_ptx, V128_3_ctx, V128_3_tweak),
    TEST_VECTOR_XTS_AES(V256_1_key, V256_1_ptx, V256_1_ctx, V256_1_tweak),
    TEST_VECTOR_XTS_AES(V256_2_key, V256_2_ptx, V256_2_ctx, V256_2_tweak),
    TEST_VECTOR_XTS_AES(V256_3_key, V256_3_ptx, V256_3_ctx, V256_3_tweak),
};


/* The function API for accessing the vectors. */
int
test_vectors_xts_aes_num(void)
{
    return sizeof(xts_aes_test_vectors) / sizeof(xts_aes_test_vectors[0]);
}

TestVector_XTS_AES_t
test_vectors_xts_aes_get(int Index)
{
    if (Index >= test_vectors_xts_aes_num())
        return NULL;

    return &xts_aes_test_vectors[Index];
}

void
test_vectors_xts_aes_release(TestVector_XTS_AES_t Vector_p)
{
    /* Test vectors are statically defined => nothing to do. */
    IDENTIFIER_NOT_USED(Vector_p);
}


/* end of file testvectors_xts_aes.c */
