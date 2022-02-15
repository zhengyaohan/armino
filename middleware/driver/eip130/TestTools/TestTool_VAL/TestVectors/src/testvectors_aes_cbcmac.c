/* testvectors_aes_cbcmac.c
 *
 * Description: Test vectors for AES CBC-MAC.
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

#include "testvectors_aes_cbcmac.h"
#include "testvectors_aes_cbcmac_data.h"

/* Macros used to build the test vectors. */
#define TEST_VECTOR_AES_CBCMAC(key, keylen, msg, msglen, mac, maclen) \
    {                                                      \
        test_vector_keys + (key),                          \
        test_vector_msgs + (msg),                          \
        test_vector_macs + (mac),                          \
        keylen,                                            \
        msglen,                                            \
        maclen,                                            \
    }

/* Actual test vectors. */
static const TestVector_AES_CBCMAC_Rec_t cbcmac_test_vectors[] =
{
    /* Miscellaneous AES CBC-MAC Test Vector #1 */
    TEST_VECTOR_AES_CBCMAC(0, 16, 0, 16, 0, 16),
    /* Miscellaneous AES CBC-MAC Test Vector #2 */
    TEST_VECTOR_AES_CBCMAC(16, 24, 16, 16, 16, 16),
    /* Miscellaneous AES CBC-MAC Test Vector #3 */
    TEST_VECTOR_AES_CBCMAC(40, 32, 32, 16, 32, 16),
    /* Miscellaneous AES CBC-MAC Test Vector #4 */
    TEST_VECTOR_AES_CBCMAC(72, 16, 48, 16, 48, 16),
    /* Miscellaneous AES CBC-MAC Test Vector #5 */
    TEST_VECTOR_AES_CBCMAC(88, 24, 64, 32, 64, 16),
    /* Miscellaneous AES CBC-MAC Test Vector #6 */
    TEST_VECTOR_AES_CBCMAC(112, 32, 96, 80, 80, 16),
    /* Miscellaneous AES CBC-MAC Test Vector #7 */
    TEST_VECTOR_AES_CBCMAC(144, 32, 176, 16384, 96, 16),
};


/* The function API for accessing the vectors. */
int
test_vectors_aes_cbcmac_num(void)
{
    return sizeof(cbcmac_test_vectors) / sizeof(cbcmac_test_vectors[0]);
}

TestVector_AES_CBCMAC_t
test_vectors_aes_cbcmac_get(int Index)
{
    if (Index >= test_vectors_aes_cbcmac_num()) return NULL;
    return &cbcmac_test_vectors[Index];
}

void
test_vectors_aes_cbcmac_release(TestVector_AES_CBCMAC_t Vector_p)
{
    /* Test vectors are statically defined => nothing to do. */
    IDENTIFIER_NOT_USED(Vector_p);
}


/* end of file testvectors_aes_cbcmac.c */
