/* testvectors_aes_wrap.c
 *
 * Description: Test vectors for AES-WRAP.
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

#include "testvectors_aes_wrap.h"
#include "testvectors_aes_wrap_data.h"

/* Macros used to build the test vectors. */
#define TEST_VECTOR_AES_WRAP(key, ptx, ctx) \
    {                                       \
        key, ptx, ctx,                      \
        sizeof(key), sizeof(ptx)            \
    }

static const TestVector_AES_WRAP_Rec_t aes_wrap_test_vectors [] =
{
    TEST_VECTOR_AES_WRAP(TC1_key, TC1_ptx, TC1_ctx),
    TEST_VECTOR_AES_WRAP(TC2_key, TC2_ptx, TC2_ctx),
    TEST_VECTOR_AES_WRAP(TC3_key, TC3_ptx, TC3_ctx),
    TEST_VECTOR_AES_WRAP(TC4_key, TC4_ptx, TC4_ctx),
    TEST_VECTOR_AES_WRAP(TC5_key, TC5_ptx, TC5_ctx),
    TEST_VECTOR_AES_WRAP(TC6_key, TC6_ptx, TC6_ctx)
};


/* The function API for accessing the vectors. */
int
test_vectors_aes_wrap_num(void)
{
    return sizeof(aes_wrap_test_vectors) / sizeof(aes_wrap_test_vectors[0]);
}

TestVector_AES_WRAP_t
test_vectors_aes_wrap_get(int Index)
{
    if (Index >= test_vectors_aes_wrap_num())
        return NULL;

    return &aes_wrap_test_vectors[Index];
}

void
test_vectors_aes_wrap_release(TestVector_AES_WRAP_t Vector_p)
{
    /* Test vectors are statically defined => nothing to do. */
    IDENTIFIER_NOT_USED(Vector_p);
}


/* end of file testvectors_aes_wrap.c */
