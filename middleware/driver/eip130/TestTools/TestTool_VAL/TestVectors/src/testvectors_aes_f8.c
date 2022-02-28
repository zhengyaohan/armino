/* testvectors_aes_f8.c
 *
 * Description: Test vectors for AES-f8.
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

#include "testvectors_aes_f8.h"
#include "testvectors_aes_f8_data.h"

/* Macros used to build the test vectors. */
#define TEST_VECTOR_AES_F8(key, ptx, ctx, iv, saltkey) \
    {                                                  \
        key, ptx, ctx, iv, saltkey,                    \
        sizeof(key), sizeof(ptx), sizeof(saltkey)      \
    }

static const TestVector_AES_f8_Rec_t aes_f8_test_vectors [] =
{
    TEST_VECTOR_AES_F8(RFC3711_B_1_key, RFC3711_B_1_ptx, RFC3711_B_1_ctx, RFC3711_B_1_iv, RFC3711_B_1_saltkey),
    TEST_VECTOR_AES_F8(TC2_key, TC2_ptx, TC2_ctx, TC2_iv, TC2_saltkey),
};


/* The function API for accessing the vectors. */
int
test_vectors_aes_f8_num(void)
{
    return sizeof(aes_f8_test_vectors) / sizeof(aes_f8_test_vectors[0]);
}

TestVector_AES_f8_t
test_vectors_aes_f8_get(int Index)
{
    if (Index >= test_vectors_aes_f8_num())
        return NULL;

    return &aes_f8_test_vectors[Index];
}

void
test_vectors_aes_f8_release(TestVector_AES_f8_t Vector_p)
{
    /* Test vectors are statically defined => nothing to do. */
    IDENTIFIER_NOT_USED(Vector_p);
}


/* end of file testvectors_aes_f8.c */
