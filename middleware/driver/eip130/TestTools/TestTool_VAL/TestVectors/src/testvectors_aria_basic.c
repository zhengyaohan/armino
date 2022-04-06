/* testvectors_aria_basic.c
 *
 * Description: Test vectors for ARIA, for key sizes 128, 192 and 256 bits
 *              and ECB, CBC and CTR modes.
 */

/*****************************************************************************
* Copyright (c) 2018 INSIDE Secure B.V. All Rights Reserved.
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

#include "testvectors_aria_basic.h"
#include "testvectors_aria_basic_data.h"


/* Macros used to build test_vectors. */
#define TEST_VECTOR_ARIA(mode, key, ptx, ctx, iv) \
    {                                            \
        TESTVECTORS_MODE_##mode,                 \
        key, ptx, ctx, iv,                       \
        sizeof(key), sizeof(ptx)                 \
    }


static const TestVector_ARIA_BASIC_Rec_t aria_basic_test_vectors [] =
{
    TEST_VECTOR_ARIA(ECB, ARIA_ECB_128_1_key, ARIA_ECB_128_1_pt, ARIA_ECB_128_1_ct, NULL),
    TEST_VECTOR_ARIA(ECB, ARIA_ECB_128_2_key, ARIA_ECB_128_2_pt, ARIA_ECB_128_2_ct, NULL),
    TEST_VECTOR_ARIA(ECB, ARIA_ECB_192_1_key, ARIA_ECB_192_1_pt, ARIA_ECB_192_1_ct, NULL),
    TEST_VECTOR_ARIA(ECB, ARIA_ECB_192_2_key, ARIA_ECB_192_2_pt, ARIA_ECB_192_2_ct, NULL),
    TEST_VECTOR_ARIA(ECB, ARIA_ECB_256_1_key, ARIA_ECB_256_1_pt, ARIA_ECB_256_1_ct, NULL),
    TEST_VECTOR_ARIA(ECB, ARIA_ECB_256_2_key, ARIA_ECB_256_2_pt, ARIA_ECB_256_2_ct, NULL),

    TEST_VECTOR_ARIA(CBC, ARIA_CBC_128_1_key, ARIA_CBC_128_1_pt, ARIA_CBC_128_1_ct, ARIA_CBC_128_1_iv),
    TEST_VECTOR_ARIA(CBC, ARIA_CBC_128_2_key, ARIA_CBC_128_2_pt, ARIA_CBC_128_2_ct, ARIA_CBC_128_2_iv),
    TEST_VECTOR_ARIA(CBC, ARIA_CBC_192_1_key, ARIA_CBC_192_1_pt, ARIA_CBC_192_1_ct, ARIA_CBC_192_1_iv),
    TEST_VECTOR_ARIA(CBC, ARIA_CBC_192_2_key, ARIA_CBC_192_2_pt, ARIA_CBC_192_2_ct, ARIA_CBC_192_2_iv),
    TEST_VECTOR_ARIA(CBC, ARIA_CBC_256_1_key, ARIA_CBC_256_1_pt, ARIA_CBC_256_1_ct, ARIA_CBC_256_1_iv),
    TEST_VECTOR_ARIA(CBC, ARIA_CBC_256_2_key, ARIA_CBC_256_2_pt, ARIA_CBC_256_2_ct, ARIA_CBC_256_2_iv),

    TEST_VECTOR_ARIA(CTR, ARIA_CTR_128_1_key, ARIA_CTR_128_1_pt, ARIA_CTR_128_1_ct, ARIA_CTR_128_1_iv),
    TEST_VECTOR_ARIA(CTR, ARIA_CTR_128_2_key, ARIA_CTR_128_2_pt, ARIA_CTR_128_2_ct, ARIA_CTR_128_2_iv),
    TEST_VECTOR_ARIA(CTR, ARIA_CTR_192_1_key, ARIA_CTR_192_1_pt, ARIA_CTR_192_1_ct, ARIA_CTR_192_1_iv),
    TEST_VECTOR_ARIA(CTR, ARIA_CTR_192_2_key, ARIA_CTR_192_2_pt, ARIA_CTR_192_2_ct, ARIA_CTR_192_2_iv),
    TEST_VECTOR_ARIA(CTR, ARIA_CTR_256_1_key, ARIA_CTR_256_1_pt, ARIA_CTR_256_1_ct, ARIA_CTR_256_1_iv),
    TEST_VECTOR_ARIA(CTR, ARIA_CTR_256_2_key, ARIA_CTR_256_2_pt, ARIA_CTR_256_2_ct, ARIA_CTR_256_2_iv),

    TEST_VECTOR_ARIA(ICM, ARIA_ICM_128_1_key, ARIA_ICM_128_1_pt, ARIA_ICM_128_1_ct, ARIA_ICM_128_1_iv),
    TEST_VECTOR_ARIA(ICM, ARIA_ICM_192_1_key, ARIA_ICM_192_1_pt, ARIA_ICM_192_1_ct, ARIA_ICM_192_1_iv),
    TEST_VECTOR_ARIA(ICM, ARIA_ICM_256_1_key, ARIA_ICM_256_1_pt, ARIA_ICM_256_1_ct, ARIA_ICM_256_1_iv),
};


/* The function API for accessing the vectors. */
int
test_vectors_aria_basic_num(void)
{
    return sizeof(aria_basic_test_vectors) / sizeof(aria_basic_test_vectors[0]);
}

TestVector_ARIA_BASIC_t
test_vectors_aria_basic_get(int Index)
{
    if (Index >= test_vectors_aria_basic_num())
        return NULL;

    return &aria_basic_test_vectors[Index];
}

void
test_vectors_aria_basic_release(TestVector_ARIA_BASIC_t Vector_p)
{
    /* Test vectors are statically defined => nothing to do. */
    IDENTIFIER_NOT_USED(Vector_p);
}


/* end of file testvectors_aria_basic.c */
