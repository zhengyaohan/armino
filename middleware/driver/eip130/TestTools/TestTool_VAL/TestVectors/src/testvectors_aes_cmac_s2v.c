/* testvectors_aes_cmac_s2v.c
 *
 * Description: Test vectors for AES CMAC_S2V.
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

#include "testvectors_aes_cmac_s2v.h"
#include "testvectors_aes_cmac_s2v_data.h"

/* Macros used to build the test vectors. */
#define TEST_VECTOR(key, keylen, msg, msglen, mac, maclen) \
    {                                                      \
        test_vector_keys + (key),                          \
        test_vector_msgs + (msg),                          \
        test_vector_macs + (mac),                          \
        keylen,                                            \
        msglen,                                            \
        maclen,                                            \
    }

/* Actual test vectors. */
static const TestVector_AES_CMAC_S2V_Rec_t cmac_s2v_test_vectors[] =
{
    /* First vector from RFC 5297, formed of two parts. */
    {
        true, 2,
        {
            TEST_VECTOR(0, 16, 0, 24, 0, 0),
            TEST_VECTOR(0, 16, 24, 14, 0, 16),
        }
    },
    /* Second vector from RFC 5297, formed of four parts. */
    {
        false, 4,
        {
            TEST_VECTOR(32, 16, 38, 40, 0, 0),
            TEST_VECTOR(32, 16, 78, 10, 0, 0),
            TEST_VECTOR(32, 16, 88, 16, 0, 0),
            TEST_VECTOR(32, 16, 104, 47, 30, 16)
        }
    }
};


/* The function API for accessing the vectors. */
int
test_vectors_aes_cmac_s2v_num(void)
{
    return sizeof(cmac_s2v_test_vectors) / sizeof(cmac_s2v_test_vectors[0]);
}

TestVector_AES_CMAC_S2V_t
test_vectors_aes_cmac_s2v_get(int Index)
{
    if (Index >= test_vectors_aes_cmac_s2v_num()) return NULL;
    return &cmac_s2v_test_vectors[Index];
}

void
test_vectors_aes_cmac_s2v_release(TestVector_AES_CMAC_S2V_t Vector_p)
{
    /* Test vectors are statically defined => nothing to do. */
    IDENTIFIER_NOT_USED(Vector_p);
}


/* end of file testvectors_aes_cmac_s2v.c */
