/* testvectors_hmac.c
 *
 * Description: Test vectors for HMAC algorithms
 *              HMAC-MD5, HMAC-SHA1, HMAC-SHA224 and HMAC-SHA256.
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

#include "testvectors_hmac.h"
#include "testvectors_hmac_data.h"

/* Macros used to build the test vectors. */
#define TEST_VECTOR_HMAC(alg, key, msg, result) \
    {                                           \
        TESTVECTORS_HASH_##alg,                 \
        key, sizeof(key),                       \
        msg, sizeof(msg),                       \
        result, sizeof(result)                  \
    }

static const TestVector_HMAC_Rec_t hmac_test_vectors [] =
{
    TEST_VECTOR_HMAC(SHA160, RFC2202_3_1_key, RFC2202_3_1_msg, RFC2202_3_1_mac),
    TEST_VECTOR_HMAC(SHA160, RFC2202_3_2_key, RFC2202_3_2_msg, RFC2202_3_2_mac),
    TEST_VECTOR_HMAC(SHA160, RFC2202_3_3_key, RFC2202_3_3_msg, RFC2202_3_3_mac),
    TEST_VECTOR_HMAC(SHA160, RFC2202_3_4_key, RFC2202_3_4_msg, RFC2202_3_4_mac),
    TEST_VECTOR_HMAC(SHA160, RFC2202_3_5_key, RFC2202_3_5_msg, RFC2202_3_5_mac),
    TEST_VECTOR_HMAC(SHA160, RFC2202_3_6_key, RFC2202_3_6_msg, RFC2202_3_6_mac),
    TEST_VECTOR_HMAC(SHA160, RFC2202_3_7_key, RFC2202_3_7_msg, RFC2202_3_7_mac),
    TEST_VECTOR_HMAC(SHA160, FIPS_198_A_1_key, FIPS_198_A_1_msg, FIPS_198_A_1_mac),
    TEST_VECTOR_HMAC(SHA160, FIPS_198_A_2_key, FIPS_198_A_2_msg, FIPS_198_A_2_mac),
    TEST_VECTOR_HMAC(SHA160, FIPS_198_A_3_key, FIPS_198_A_3_msg, FIPS_198_A_3_mac),
    TEST_VECTOR_HMAC(SHA160, FIPS_198_A_4_key, FIPS_198_A_4_msg, FIPS_198_A_4_mac),
    TEST_VECTOR_HMAC(SHA224, RFC4231_4_2a_key, RFC4231_4_2a_msg, RFC4231_4_2a_mac),
    TEST_VECTOR_HMAC(SHA224, RFC4231_4_3a_key, RFC4231_4_3a_msg, RFC4231_4_3a_mac),
    TEST_VECTOR_HMAC(SHA224, RFC4231_4_4a_key, RFC4231_4_4a_msg, RFC4231_4_4a_mac),
    TEST_VECTOR_HMAC(SHA224, RFC4231_4_5a_key, RFC4231_4_5a_msg, RFC4231_4_5a_mac),
    TEST_VECTOR_HMAC(SHA224, RFC4231_4_6a_key, RFC4231_4_6a_msg, RFC4231_4_6a_mac),
    TEST_VECTOR_HMAC(SHA224, RFC4231_4_7a_key, RFC4231_4_7a_msg, RFC4231_4_7a_mac),
    TEST_VECTOR_HMAC(SHA224, RFC4231_4_8a_key, RFC4231_4_8a_msg, RFC4231_4_8a_mac),
    TEST_VECTOR_HMAC(SHA256, ietf_TC_1_key, ietf_TC_1_msg, ietf_TC_1_mac),
    TEST_VECTOR_HMAC(SHA256, ietf_TC_2_key, ietf_TC_2_msg, ietf_TC_2_mac),
    TEST_VECTOR_HMAC(SHA256, ietf_TC_3_key, ietf_TC_3_msg, ietf_TC_3_mac),
    TEST_VECTOR_HMAC(SHA256, ietf_TC_4_key, ietf_TC_4_msg, ietf_TC_4_mac),
    TEST_VECTOR_HMAC(SHA256, ietf_TC_6_key, ietf_TC_6_msg, ietf_TC_6_mac),
    TEST_VECTOR_HMAC(SHA256, ietf_TC_7_key, ietf_TC_7_msg, ietf_TC_7_mac),
    TEST_VECTOR_HMAC(SHA256, ietf_TC_8_key, ietf_TC_8_msg, ietf_TC_8_mac),
    TEST_VECTOR_HMAC(SHA256, ietf_TC_9_key, ietf_TC_9_msg, ietf_TC_9_mac),
    TEST_VECTOR_HMAC(SHA256, ietf_TC_10_key, ietf_TC_10_msg, ietf_TC_10_mac),
    TEST_VECTOR_HMAC(SHA256, RFC4231_4_2b_key, RFC4231_4_2b_msg, RFC4231_4_2b_mac),
    TEST_VECTOR_HMAC(SHA256, RFC4231_4_3b_key, RFC4231_4_3b_msg, RFC4231_4_3b_mac),
    TEST_VECTOR_HMAC(SHA256, RFC4231_4_4b_key, RFC4231_4_4b_msg, RFC4231_4_4b_mac),
    TEST_VECTOR_HMAC(SHA256, RFC4231_4_5b_key, RFC4231_4_5b_msg, RFC4231_4_5b_mac),
    TEST_VECTOR_HMAC(SHA256, RFC4231_4_6b_key, RFC4231_4_6b_msg, RFC4231_4_6b_mac),
    TEST_VECTOR_HMAC(SHA256, RFC4231_4_7b_key, RFC4231_4_7b_msg, RFC4231_4_7b_mac),
    TEST_VECTOR_HMAC(SHA256, RFC4231_4_8b_key, RFC4231_4_8b_msg, RFC4231_4_8b_mac),
    TEST_VECTOR_HMAC(SHA384, RFC4231_SHA384_512_1_key,   RFC4231_SHA384_512_1_msg, RFC4231_SHA384_1_mac),
    TEST_VECTOR_HMAC(SHA384, RFC4231_SHA384_512_2_key,   RFC4231_SHA384_512_2_msg, RFC4231_SHA384_2_mac),
    TEST_VECTOR_HMAC(SHA384, RFC4231_SHA384_512_3_key,   RFC4231_SHA384_512_3_msg, RFC4231_SHA384_3_mac),
    TEST_VECTOR_HMAC(SHA384, RFC4231_SHA384_512_4_key,   RFC4231_SHA384_512_4_msg, RFC4231_SHA384_4_mac),
    TEST_VECTOR_HMAC(SHA384, RFC4231_SHA384_512_5_6_key, RFC4231_SHA384_512_5_msg, RFC4231_SHA384_5_mac),
    TEST_VECTOR_HMAC(SHA384, RFC4231_SHA384_512_5_6_key, RFC4231_SHA384_512_6_msg, RFC4231_SHA384_6_mac),
    TEST_VECTOR_HMAC(SHA512, RFC4231_SHA384_512_1_key,   RFC4231_SHA384_512_1_msg, RFC4231_SHA512_1_mac),
    TEST_VECTOR_HMAC(SHA512, RFC4231_SHA384_512_2_key,   RFC4231_SHA384_512_2_msg, RFC4231_SHA512_2_mac),
    TEST_VECTOR_HMAC(SHA512, RFC4231_SHA384_512_3_key,   RFC4231_SHA384_512_3_msg, RFC4231_SHA512_3_mac),
    TEST_VECTOR_HMAC(SHA512, RFC4231_SHA384_512_4_key,   RFC4231_SHA384_512_4_msg, RFC4231_SHA512_4_mac),
    TEST_VECTOR_HMAC(SHA512, RFC4231_SHA384_512_5_6_key, RFC4231_SHA384_512_5_msg, RFC4231_SHA512_5_mac),
    TEST_VECTOR_HMAC(SHA512, RFC4231_SHA384_512_5_6_key, RFC4231_SHA384_512_6_msg, RFC4231_SHA512_6_mac),
};


/* The function API for accessing the vectors. */
int
test_vectors_hmac_num(void)
{
    return sizeof(hmac_test_vectors) / sizeof(hmac_test_vectors[0]);
}

TestVector_HMAC_t
test_vectors_hmac_get(int Index)
{
    if (Index >= test_vectors_hmac_num())
        return NULL;
    return &hmac_test_vectors[Index];
}

void
test_vectors_hmac_release(TestVector_HMAC_t Vector_p)
{
    /* Test vectors are statically defined => nothing to do. */
    IDENTIFIER_NOT_USED(Vector_p);
}


/* end of file testvectors_hmac.c */
