/* testvectors_aria_ccm.c
 *
 * Description: Test vectors for ARIA CCM.
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

#include "testvectors_aria_ccm.h"
#include "testvectors_aria_ccm_data.h"

/* Use macro to build ccm_test_vectors. */
#define TEST_VECTOR_ARIA_CCM(key, nonce, aad, payload, ct)                 \
  {                                                                        \
      key, nonce, aad, payload, ct,                                        \
      sizeof(key), sizeof(nonce), sizeof(aad), sizeof(payload), sizeof(ct) \
  }

/* Actual test vectors. */
static const TestVector_ARIA_CCM_Rec_t ccm_test_vectors[] =
{
    TEST_VECTOR_ARIA_CCM(ARIA_CCM_128_1_K,ARIA_CCM_128_1_N, ARIA_CCM_128_1_A, ARIA_CCM_128_1_P, ARIA_CCM_128_1_C),
    TEST_VECTOR_ARIA_CCM(ARIA_CCM_128_2_K,ARIA_CCM_128_2_N, ARIA_CCM_128_2_A, ARIA_CCM_128_2_P, ARIA_CCM_128_2_C),
    TEST_VECTOR_ARIA_CCM(ARIA_CCM_192_1_K,ARIA_CCM_192_1_N, ARIA_CCM_192_1_A, ARIA_CCM_192_1_P, ARIA_CCM_192_1_C),
    TEST_VECTOR_ARIA_CCM(ARIA_CCM_192_2_K,ARIA_CCM_192_2_N, ARIA_CCM_192_2_A, ARIA_CCM_192_2_P, ARIA_CCM_192_2_C),
    TEST_VECTOR_ARIA_CCM(ARIA_CCM_256_1_K,ARIA_CCM_256_1_N, ARIA_CCM_256_1_A, ARIA_CCM_256_1_P, ARIA_CCM_256_1_C),
    TEST_VECTOR_ARIA_CCM(ARIA_CCM_256_2_K,ARIA_CCM_256_2_N, ARIA_CCM_256_2_A, ARIA_CCM_256_2_P, ARIA_CCM_256_2_C)
};


/* The function API for accessing the vectors. */
int
test_vectors_aria_ccm_num(void)
{
    return sizeof(ccm_test_vectors) / sizeof(ccm_test_vectors[0]);
}

TestVector_ARIA_CCM_t
test_vectors_aria_ccm_get(int Index)
{
    if (Index >= test_vectors_aria_ccm_num())
    {
         return NULL;
    }
    return &ccm_test_vectors[Index];
}

void
test_vectors_aria_ccm_release(TestVector_ARIA_CCM_t Vector_p)
{
    /* Test vectors are statically defined => nothing to do. */
    IDENTIFIER_NOT_USED(Vector_p);
}


/* end of file testvectors_aria_ccm.c */
