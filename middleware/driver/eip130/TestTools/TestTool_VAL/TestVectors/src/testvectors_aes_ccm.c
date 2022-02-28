/* testvectors_aes_ccm.c
 *
 * Description: Test vectors for AES CCM.
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

#include "testvectors_aes_ccm.h"
#include "testvectors_aes_ccm_data.h"

/* Macros used to build the test vectors. */
#define TEST_VECTOR_AES_CCM(key, nonce, aad, payload, ct)                          \
  {                                                                        \
      key, nonce, aad, payload, ct,                                        \
      sizeof(key), sizeof(nonce), sizeof(aad), sizeof(payload), sizeof(ct) \
  }

static const TestVector_AES_CCM_Rec_t ccm_test_vectors[] =
{
    TEST_VECTOR_AES_CCM(V256_1_Key, V256_1_Nonce, V256_1_Adata, V256_1_Payload, V256_1_CT),
    TEST_VECTOR_AES_CCM(V256_2_Key, V256_2_Nonce, V256_2_Adata, V256_2_Payload, V256_2_CT),
    TEST_VECTOR_AES_CCM(V256_3_Key, V256_3_Nonce, V256_3_Adata, V256_3_Payload, V256_3_CT),
    TEST_VECTOR_AES_CCM(V256_4_Key, V256_4_Nonce, V256_4_Adata, V256_4_Payload, V256_4_CT),
    TEST_VECTOR_AES_CCM(V256_5_Key, V256_5_Nonce, V256_5_Adata, V256_5_Payload, V256_5_CT)
};


/* The function API for accessing the vectors. */
int
test_vectors_aes_ccm_num(void)
{
    return sizeof(ccm_test_vectors) / sizeof(ccm_test_vectors[0]);
}

TestVector_AES_CCM_t
test_vectors_aes_ccm_get(int Index)
{
    if (Index >= test_vectors_aes_ccm_num())
    {
         return NULL;
    }
    return &ccm_test_vectors[Index];
}

void
test_vectors_aes_ccm_release(TestVector_AES_CCM_t Vector_p)
{
    /* Test vectors are statically defined => nothing to do. */
    IDENTIFIER_NOT_USED(Vector_p);
}


/* end of file testvectors_aes_ccm.c */
