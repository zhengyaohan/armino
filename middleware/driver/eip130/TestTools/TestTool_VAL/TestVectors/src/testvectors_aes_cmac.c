/* testvectors_aes_cmac.c
 *
 * Description: Test vectors for AES CMAC.
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

#include "testvectors_aes_cmac.h"
#include "testvectors_aes_cmac_data.h"

/* Macros used to build the test vectors. */
#define TEST_VECTOR_AES_CMAC(key, keylen, msg, msglen, mac, maclen) \
    {                                                      \
        test_vector_keys + (key),                          \
        test_vector_msgs + (msg),                          \
        test_vector_macs + (mac),                          \
        keylen,                                            \
        msglen,                                            \
        maclen,                                            \
    }

/* Actual test vectors. */
static const TestVector_AES_CMAC_Rec_t cmac_test_vectors[] =
{
    /* NIST AES CMAC Vector from [NIST-SP-800-38B] #1 */
    TEST_VECTOR_AES_CMAC(0, 16, 0, 0, 0, 16),
    /* NIST AES CMAC Vector from [NIST-SP-800-38B] #2 */
    TEST_VECTOR_AES_CMAC(16, 24, 0, 0, 16, 16),
    /* NIST AES CMAC Vector from [NIST-SP-800-38B] #3 */
    TEST_VECTOR_AES_CMAC(40, 32, 0, 0, 32, 16),
    /* NIST AES CMAC Vector from [NIST-SP-800-38B] #4 */
    TEST_VECTOR_AES_CMAC(72, 16, 0, 16, 48, 16),
    /* NIST AES CMAC Vector from [NIST-SP-800-38B] #5 */
    TEST_VECTOR_AES_CMAC(88, 24, 16, 16, 64, 16),
    /* NIST AES CMAC Vector from [NIST-SP-800-38B] #6 */
    TEST_VECTOR_AES_CMAC(112, 32, 32, 16, 80, 16),
    /* NIST AES CMAC Vector from [NIST-SP-800-38B] #7 */
    TEST_VECTOR_AES_CMAC(144, 16, 48, 40, 96, 16),
    /* NIST AES CMAC Vector from [NIST-SP-800-38B] #8 */
    TEST_VECTOR_AES_CMAC(160, 24, 88, 40, 112, 16),
    /* NIST AES CMAC Vector from [NIST-SP-800-38B] #9 */
    TEST_VECTOR_AES_CMAC(184, 32, 128, 40, 128, 16),
    /* NIST AES CMAC Vector from [NIST-SP-800-38B] #10 */
    TEST_VECTOR_AES_CMAC(216, 16, 168, 64, 144, 16),
    /* NIST AES CMAC Vector from [NIST-SP-800-38B] #11 */
    TEST_VECTOR_AES_CMAC(232, 24, 232, 64, 160, 16),
    /* NIST AES CMAC Vector from [NIST-SP-800-38B] #12 */
    TEST_VECTOR_AES_CMAC(256, 32, 296, 64, 176, 16),
    /* Miscellaneous AES CMAC Test Vector #1 */
    TEST_VECTOR_AES_CMAC(288, 16, 360, 16, 192, 16),
    /* Miscellaneous AES CMAC Test Vector #2 */
    TEST_VECTOR_AES_CMAC(304, 24, 376, 17, 208, 16),
    /* Miscellaneous AES CMAC Test Vector #3 */
    TEST_VECTOR_AES_CMAC(328, 32, 393, 18, 224, 16),
    /* Miscellaneous AES CMAC Test Vector #4 */
    TEST_VECTOR_AES_CMAC(360, 16, 411, 35, 240, 16),
    /* Miscellaneous AES CMAC Test Vector #5 */
    TEST_VECTOR_AES_CMAC(376, 24, 446, 36, 256, 16),
    /* Miscellaneous AES CMAC Test Vector #6 */
    TEST_VECTOR_AES_CMAC(400, 32, 482, 37, 272, 16),
    /* Miscellaneous AES CMAC Test Vector #7 */
    TEST_VECTOR_AES_CMAC(432, 16, 519, 86, 288, 16),
    /* Miscellaneous AES CMAC Test Vector #8 */
    TEST_VECTOR_AES_CMAC(448, 24, 605, 87, 304, 16),
    /* Miscellaneous AES CMAC Test Vector #9 */
    TEST_VECTOR_AES_CMAC(472, 32, 692, 88, 320, 16),
    /* Miscellaneous AES CMAC Test Vector #10 */
    TEST_VECTOR_AES_CMAC(504, 16, 780, 329, 336, 16),
    /* Miscellaneous AES CMAC Test Vector #11 */
    TEST_VECTOR_AES_CMAC(520, 24, 1109, 330, 352, 16),
    /* Miscellaneous AES CMAC Test Vector #12 */
    TEST_VECTOR_AES_CMAC(544, 32, 1439, 331, 368, 16),
    /* Miscellaneous AES CMAC Test Vector #13 */
    TEST_VECTOR_AES_CMAC(576, 16, 1770, 812, 384, 16),
    /* Miscellaneous AES CMAC Test Vector #14 */
    TEST_VECTOR_AES_CMAC(592, 24, 2582, 813, 400, 16),
    /* Miscellaneous AES CMAC Test Vector #15 */
    TEST_VECTOR_AES_CMAC(616, 32, 3395, 814, 416, 16),
    /* Miscellaneous AES CMAC Test Vector #16 */
    TEST_VECTOR_AES_CMAC(648, 32, 4209, 16015, 432, 16),
};


/* The function API for accessing the vectors. */
int
test_vectors_aes_cmac_num(void)
{
    return sizeof(cmac_test_vectors) / sizeof(cmac_test_vectors[0]);
}

TestVector_AES_CMAC_t
test_vectors_aes_cmac_get(int Index)
{
    if (Index >= test_vectors_aes_cmac_num())
    {
         return NULL;
    }
    return &cmac_test_vectors[Index];
}

void
test_vectors_aes_cmac_release(TestVector_AES_CMAC_t Vector_p)
{
    /* Test vectors are statically defined => nothing to do. */
    IDENTIFIER_NOT_USED(Vector_p);
}


/* end of file testvectors_aes_cmac.c */
