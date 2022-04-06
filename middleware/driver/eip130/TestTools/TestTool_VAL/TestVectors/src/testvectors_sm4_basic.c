/* testvectors_sm4_basic.c
 *
 * Description: Test vectors for SM4 for modes ECB, CBC and CTR.
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

#include "testvectors_sm4_basic.h"
#include "testvectors_sm4_basic_data.h"

/* Macros used to build the test vectors. */
#define TEST_VECTOR_SM4(mode, key, ptx, ctx, iv, resiv) \
    {                                                   \
        TESTVECTORS_MODE_##mode,                        \
        key, ptx, ctx, iv, resiv,                       \
        sizeof(key), sizeof(ptx)                        \
    }

static const TestVector_SM4_BASIC_Rec_t sm4_basic_test_vectors [] =
{
    TEST_VECTOR_SM4(ECB, ECB_A_2_1_1_key, ECB_A_2_1_1_ptx, ECB_A_2_1_1_ctx, NULL, NULL),
    TEST_VECTOR_SM4(ECB, ECB_A_2_1_2_key, ECB_A_2_1_2_ptx, ECB_A_2_1_2_ctx, NULL, NULL),
    TEST_VECTOR_SM4(CBC, CBC_A_2_2_1_key, CBC_A_2_2_1_ptx, CBC_A_2_2_1_ctx, CBC_A_2_2_1_iv, CBC_A_2_2_1_res_iv),
    TEST_VECTOR_SM4(CBC, CBC_A_2_2_2_key, CBC_A_2_2_2_ptx, CBC_A_2_2_2_ctx, CBC_A_2_2_2_iv, CBC_A_2_2_2_res_iv),
    TEST_VECTOR_SM4(CTR, CTR_A_2_5_1_key, CTR_A_2_5_1_ptx, CTR_A_2_5_1_ctx, CTR_A_2_5_1_iv, CTR_A_2_5_1_res_iv),
    TEST_VECTOR_SM4(CTR, CTR_A_2_5_2_key, CTR_A_2_5_2_ptx, CTR_A_2_5_2_ctx, CTR_A_2_5_2_iv, CTR_A_2_5_2_res_iv),
};


/* The function API for accessing the vectors. */
int
test_vectors_sm4_basic_num(void)
{
    return sizeof(sm4_basic_test_vectors) / sizeof(sm4_basic_test_vectors[0]);
}

TestVector_SM4_BASIC_t
test_vectors_sm4_basic_get(int Index)
{
    if (Index >= test_vectors_sm4_basic_num())
        return NULL;

    return &sm4_basic_test_vectors[Index];
}

void
test_vectors_sm4_basic_release(TestVector_SM4_BASIC_t Vector_p)
{
    /* Test vectors are statically defined => nothing to do. */
    IDENTIFIER_NOT_USED(Vector_p);
}


/* end of file testvectors_sm4_basic.c */
