/* testvectors_des.c
 *
 * Description: Test vectors for DES in modes ECB and CBC
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

#include "testvectors_des.h"
#include "testvectors_des_data.h"

/* Macros used to build the test vectors. */
#define TEST_VECTOR_DES(key, ptx, ctx, mode, iv) \
    {                                            \
        TESTVECTORS_MODE_##mode,                 \
        key, ptx, ctx, iv,                       \
        sizeof(key), sizeof(ptx)                 \
    }

static const TestVector_AES_BASIC_Rec_t des_test_vectors [] =
{
    TEST_VECTOR_DES(App_A_key, App_A_ptx, App_A_ctx, ECB, NULL),
    TEST_VECTOR_DES(IP_and_E_Test_key, IP_and_E_Test_ptx, IP_and_E_Test_ctx, ECB, NULL),
    TEST_VECTOR_DES(FIPS81_B1_key, FIPS81_B1_ptx, FIPS81_B1_ctx, ECB, NULL),
    TEST_VECTOR_DES(FIPS81_C1_key, FIPS81_C1_ptx, FIPS81_C1_ctx, CBC, FIPS81_C1_iv),
    TEST_VECTOR_DES(B_1_3DES_ECB_key, B_1_3DES_ECB_ptx, B_1_3DES_ECB_ctx, ECB, NULL),
    TEST_VECTOR_DES(TDES_ECB_Ex1_key, TDES_ECB_Ex1_ptx, TDES_ECB_Ex1_ctx, ECB, NULL),
    TEST_VECTOR_DES(TDES_ECB_Ex2_key, TDES_ECB_Ex2_ptx, TDES_ECB_Ex2_ctx, ECB, NULL),
    TEST_VECTOR_DES(TDES_CBC_Ex1_key, TDES_CBC_Ex1_ptx, TDES_CBC_Ex1_ctx, CBC, TDES_CBC_Ex1_iv),
    TEST_VECTOR_DES(TDES_CBC_Ex2_key, TDES_CBC_Ex2_ptx, TDES_CBC_Ex2_ctx, CBC, TDES_CBC_Ex2_iv)
};


/* The function API for accessing the vectors. */
int
test_vectors_des_num(void)
{
    return sizeof(des_test_vectors) / sizeof(des_test_vectors[0]);
}

TestVector_DES_t
test_vectors_des_get(int Index)
{
    if (Index >= test_vectors_des_num())
        return NULL;

    return &des_test_vectors[Index];
}

void
test_vectors_des_release(TestVector_DES_t Vector_p)
{
    /* Test vectors are statically defined => nothing to do. */
    IDENTIFIER_NOT_USED(Vector_p);
}


/* end of file testvectors_des.c */
