/* testvectors_aria_gcm.c
 *
 * Description: Test vectors for ARIA GCM.
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

#include "testvectors_aria_gcm.h"
#include "testvectors_aria_gcm_data.h"

/* Use macro to build gcm_test_vectors. */
#define TEST_VECTOR_ARIA_GCM(key, pt, ct, aad, iv, h, y0, ghash, tag) \
  {                                                                  \
      key, pt, ct, aad, iv, h, y0, ghash, tag,                       \
      sizeof(key), sizeof(pt), sizeof(ct), sizeof(aad), sizeof(iv),  \
      sizeof(h), sizeof(y0), sizeof(ghash), sizeof(tag),             \
  }

/* Actual test vectors. */
static const TestVector_ARIA_GCM_Rec_t gcm_test_vectors[] =
{
    TEST_VECTOR_ARIA_GCM(ARIA_GCM_128_0_K, ARIA_GCM_128_0_P, ARIA_GCM_128_0_C, ARIA_GCM_128_0_A, ARIA_GCM_128_0_IV, ARIA_GCM_128_0_H, ARIA_GCM_128_0_Y0, ARIA_GCM_128_0_GH, ARIA_GCM_128_0_T),
    TEST_VECTOR_ARIA_GCM(ARIA_GCM_128_1_K, ARIA_GCM_128_1_P, ARIA_GCM_128_1_C, ARIA_GCM_128_1_A, ARIA_GCM_128_1_IV, ARIA_GCM_128_1_H, ARIA_GCM_128_1_Y0, ARIA_GCM_128_1_GH, ARIA_GCM_128_1_T),
    TEST_VECTOR_ARIA_GCM(ARIA_GCM_128_2_K, ARIA_GCM_128_2_P, ARIA_GCM_128_2_C, ARIA_GCM_128_2_A, ARIA_GCM_128_2_IV, ARIA_GCM_128_2_H, ARIA_GCM_128_2_Y0, ARIA_GCM_128_2_GH, ARIA_GCM_128_2_T),
    TEST_VECTOR_ARIA_GCM(ARIA_GCM_192_0_K, ARIA_GCM_192_0_P, ARIA_GCM_192_0_C, ARIA_GCM_192_0_A, ARIA_GCM_192_0_IV, ARIA_GCM_192_0_H, ARIA_GCM_192_0_Y0, ARIA_GCM_192_0_GH, ARIA_GCM_192_0_T),
    TEST_VECTOR_ARIA_GCM(ARIA_GCM_192_1_K, ARIA_GCM_192_1_P, ARIA_GCM_192_1_C, ARIA_GCM_192_1_A, ARIA_GCM_192_1_IV, ARIA_GCM_192_1_H, ARIA_GCM_192_1_Y0, ARIA_GCM_192_1_GH, ARIA_GCM_192_1_T),
    TEST_VECTOR_ARIA_GCM(ARIA_GCM_192_2_K, ARIA_GCM_192_2_P, ARIA_GCM_192_2_C, ARIA_GCM_192_2_A, ARIA_GCM_192_2_IV, ARIA_GCM_192_2_H, ARIA_GCM_192_2_Y0, ARIA_GCM_192_2_GH, ARIA_GCM_192_2_T),
    TEST_VECTOR_ARIA_GCM(ARIA_GCM_256_0_K, ARIA_GCM_256_0_P, ARIA_GCM_256_0_C, ARIA_GCM_256_0_A, ARIA_GCM_256_0_IV, ARIA_GCM_256_0_H, ARIA_GCM_256_0_Y0, ARIA_GCM_256_0_GH, ARIA_GCM_256_0_T),
    TEST_VECTOR_ARIA_GCM(ARIA_GCM_256_1_K, ARIA_GCM_256_1_P, ARIA_GCM_256_1_C, ARIA_GCM_256_1_A, ARIA_GCM_256_1_IV, ARIA_GCM_256_1_H, ARIA_GCM_256_1_Y0, ARIA_GCM_256_1_GH, ARIA_GCM_256_1_T),
    TEST_VECTOR_ARIA_GCM(ARIA_GCM_256_2_K, ARIA_GCM_256_2_P, ARIA_GCM_256_2_C, ARIA_GCM_256_2_A, ARIA_GCM_256_2_IV, ARIA_GCM_256_2_H, ARIA_GCM_256_2_Y0, ARIA_GCM_256_2_GH, ARIA_GCM_256_2_T),
};


/* The function API for accessing the vectors. */
int
test_vectors_aria_gcm_num(void)
{
    return sizeof(gcm_test_vectors) / sizeof(gcm_test_vectors[0]);
}

TestVector_ARIA_GCM_t
test_vectors_aria_gcm_get(int Index)
{
    if (Index >= test_vectors_aria_gcm_num())
    {
         return NULL;
    }
    return &gcm_test_vectors[Index];
}

void
test_vectors_aria_gcm_release(TestVector_ARIA_GCM_t Vector_p)
{
    /* Test vectors are statically defined => nothing to do. */
    IDENTIFIER_NOT_USED(Vector_p);
}


/* end of file testvectors_aria_gcm.c */
