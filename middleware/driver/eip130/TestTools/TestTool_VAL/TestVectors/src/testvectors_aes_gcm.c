/* testvectors_aes_gcm.c
 *
 * Description: Test vectors for AES GCM.
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

#include "testvectors_aes_gcm.h"
#include "testvectors_aes_gcm_data.h"

/* Macros used to build the test vectors. */
#define TEST_VECTOR_AES_GCM(key, pt, ct, aad, iv, h, y0, ghash, tag) \
  {                                                                  \
      key, pt, ct, aad, iv, h, y0, ghash, tag,                       \
      sizeof(key), sizeof(pt), sizeof(ct), sizeof(aad), sizeof(iv),  \
      sizeof(h), sizeof(y0), sizeof(ghash), sizeof(tag),             \
  }

/* Actual test vectors. */
static const TestVector_AES_GCM_Rec_t gcm_test_vectors[] =
{
    TEST_VECTOR_AES_GCM(AESGCM_128_tv0_Key, AESGCM_128_tv0_P, AESGCM_128_tv0_C, AESGCM_128_tv0_A, AESGCM_128_tv0_IV, AESGCM_128_tv0_H, AESGCM_128_tv0_Y0, AESGCM_128_tv0_GHASH, AESGCM_128_tv0_T),
    TEST_VECTOR_AES_GCM(AESGCM_128_tv1_Key, AESGCM_128_tv1_P, AESGCM_128_tv1_C, AESGCM_128_tv1_A, AESGCM_128_tv1_IV, AESGCM_128_tv1_H, AESGCM_128_tv1_Y0, AESGCM_128_tv1_GHASH, AESGCM_128_tv1_T),
    TEST_VECTOR_AES_GCM(AESGCM_128_tv2_Key, AESGCM_128_tv2_P, AESGCM_128_tv2_C, AESGCM_128_tv2_A, AESGCM_128_tv2_IV, AESGCM_128_tv2_H, AESGCM_128_tv2_Y0, AESGCM_128_tv2_GHASH, AESGCM_128_tv2_T),
    TEST_VECTOR_AES_GCM(AESGCM_128_tv3_Key, AESGCM_128_tv3_P, AESGCM_128_tv3_C, AESGCM_128_tv3_A, AESGCM_128_tv3_IV, AESGCM_128_tv3_H, AESGCM_128_tv3_Y0, AESGCM_128_tv3_GHASH, AESGCM_128_tv3_T),
    TEST_VECTOR_AES_GCM(AESGCM_128_tv4_Key, AESGCM_128_tv4_P, AESGCM_128_tv4_C, AESGCM_128_tv4_A, AESGCM_128_tv4_IV, AESGCM_128_tv4_H, AESGCM_128_tv4_Y0, AESGCM_128_tv4_GHASH, AESGCM_128_tv4_T),
    TEST_VECTOR_AES_GCM(AESGCM_192_tv0_Key, AESGCM_192_tv0_P, AESGCM_192_tv0_C, AESGCM_192_tv0_A, AESGCM_192_tv0_IV, AESGCM_192_tv0_H, AESGCM_192_tv0_Y0, AESGCM_192_tv0_GHASH, AESGCM_192_tv0_T),
    TEST_VECTOR_AES_GCM(AESGCM_192_tv1_Key, AESGCM_192_tv1_P, AESGCM_192_tv1_C, AESGCM_192_tv1_A, AESGCM_192_tv1_IV, AESGCM_192_tv1_H, AESGCM_192_tv1_Y0, AESGCM_192_tv1_GHASH, AESGCM_192_tv1_T),
    TEST_VECTOR_AES_GCM(AESGCM_192_tv2_Key, AESGCM_192_tv2_P, AESGCM_192_tv2_C, AESGCM_192_tv2_A, AESGCM_192_tv2_IV, AESGCM_192_tv2_H, AESGCM_192_tv2_Y0, AESGCM_192_tv2_GHASH, AESGCM_192_tv2_T),
    TEST_VECTOR_AES_GCM(AESGCM_192_tv3_Key, AESGCM_192_tv3_P, AESGCM_192_tv3_C, AESGCM_192_tv3_A, AESGCM_192_tv3_IV, AESGCM_192_tv3_H, AESGCM_192_tv3_Y0, AESGCM_192_tv3_GHASH, AESGCM_192_tv3_T),
    TEST_VECTOR_AES_GCM(AESGCM_192_tv4_Key, AESGCM_192_tv4_P, AESGCM_192_tv4_C, AESGCM_192_tv4_A, AESGCM_192_tv4_IV, AESGCM_192_tv4_H, AESGCM_192_tv4_Y0, AESGCM_192_tv4_GHASH, AESGCM_192_tv4_T),
    TEST_VECTOR_AES_GCM(AESGCM_256_tv0_Key, AESGCM_256_tv0_P, AESGCM_256_tv0_C, AESGCM_256_tv0_A, AESGCM_256_tv0_IV, AESGCM_256_tv0_H, AESGCM_256_tv0_Y0, AESGCM_256_tv0_GHASH, AESGCM_256_tv0_T),
    TEST_VECTOR_AES_GCM(AESGCM_256_tv1_Key, AESGCM_256_tv1_P, AESGCM_256_tv1_C, AESGCM_256_tv1_A, AESGCM_256_tv1_IV, AESGCM_256_tv1_H, AESGCM_256_tv1_Y0, AESGCM_256_tv1_GHASH, AESGCM_256_tv1_T),
    TEST_VECTOR_AES_GCM(AESGCM_256_tv2_Key, AESGCM_256_tv2_P, AESGCM_256_tv2_C, AESGCM_256_tv2_A, AESGCM_256_tv2_IV, AESGCM_256_tv2_H, AESGCM_256_tv2_Y0, AESGCM_256_tv2_GHASH, AESGCM_256_tv2_T),
    TEST_VECTOR_AES_GCM(AESGCM_256_tv3_Key, AESGCM_256_tv3_P, AESGCM_256_tv3_C, AESGCM_256_tv3_A, AESGCM_256_tv3_IV, AESGCM_256_tv3_H, AESGCM_256_tv3_Y0, AESGCM_256_tv3_GHASH, AESGCM_256_tv3_T),
    TEST_VECTOR_AES_GCM(AESGCM_256_tv4_Key, AESGCM_256_tv4_P, AESGCM_256_tv4_C, AESGCM_256_tv4_A, AESGCM_256_tv4_IV, AESGCM_256_tv4_H, AESGCM_256_tv4_Y0, AESGCM_256_tv4_GHASH, AESGCM_256_tv4_T),
};


/* The function API for accessing the vectors. */
int
test_vectors_aes_gcm_num(void)
{
    return sizeof(gcm_test_vectors) / sizeof(gcm_test_vectors[0]);
}

TestVector_AES_GCM_t
test_vectors_aes_gcm_get(int Index)
{
    if (Index >= test_vectors_aes_gcm_num())
    {
         return NULL;
    }
    return &gcm_test_vectors[Index];
}

void
test_vectors_aes_gcm_release(TestVector_AES_GCM_t Vector_p)
{
    /* Test vectors are statically defined => nothing to do. */
    IDENTIFIER_NOT_USED(Vector_p);
}


/* end of file testvectors_aes_gcm.c */
