/* testvectors_aes_basic.c
 *
 * Description: Test vectors for AES, for key sizes 128, 192 and 256 bits
 *              and ECB, CBC and CTR modes.
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

#include "testvectors_aes_basic.h"
#include "testvectors_aes_basic_data.h"

/* Macros used to build the test vectors. */
#define TEST_VECTOR_AES(mode, key, ptx, ctx, iv) \
    {                                            \
        TESTVECTORS_MODE_##mode,                 \
        key, ptx, ctx, iv,                       \
        sizeof(key), sizeof(ptx)                 \
    }

static const TestVector_AES_BASIC_Rec_t aes_basic_test_vectors [] =
{
    TEST_VECTOR_AES(ECB, F_1_1_key, F_1_1_ptx, F_1_1_ctx, NULL),
    TEST_VECTOR_AES(ECB, F_1_3_key, F_1_3_ptx, F_1_3_ctx, NULL),
    TEST_VECTOR_AES(ECB, F_1_5_key, F_1_5_ptx, F_1_5_ctx, NULL),
    TEST_VECTOR_AES(CBC, F_2_1_key, F_2_1_ptx, F_2_1_ctx, F_2_1_iv),
    TEST_VECTOR_AES(CBC, F_2_3_key, F_2_3_ptx, F_2_3_ctx, F_2_3_iv),
    TEST_VECTOR_AES(CBC, F_2_5_key, F_2_5_ptx, F_2_5_ctx, F_2_5_iv),
    TEST_VECTOR_AES(CTR, F_5_1_key, F_5_1_ptx, F_5_1_ctx, F_5_1_iv),
    TEST_VECTOR_AES(CTR, F_5_3_key, F_5_3_ptx, F_5_3_ctx, F_5_3_iv),
    TEST_VECTOR_AES(CTR, F_5_5_key, F_5_5_ptx, F_5_5_ctx, F_5_5_iv),
    TEST_VECTOR_AES(ICM, F_4_1_key, F_4_1_ptx, F_4_1_ctx, F_4_1_iv),
    TEST_VECTOR_AES(ICM, F_4_2_key, F_4_2_ptx, F_4_2_ctx, F_4_2_iv)
};


/* The function API for accessing the vectors. */
int
test_vectors_aes_basic_num(void)
{
    return sizeof(aes_basic_test_vectors) / sizeof(aes_basic_test_vectors[0]);
}

TestVector_AES_BASIC_t
test_vectors_aes_basic_get(int Index)
{
    if (Index >= test_vectors_aes_basic_num())
        return NULL;

    return &aes_basic_test_vectors[Index];
}

void
test_vectors_aes_basic_release(TestVector_AES_BASIC_t Vector_p)
{
    /* Test vectors are statically defined => nothing to do. */
    IDENTIFIER_NOT_USED(Vector_p);
}


/* end of file testvectors_aes_basic.c */
