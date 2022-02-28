/* testvectors_aria_cbcmac.c
 *
 * Description: Test vectors for ARIA CBC-MAC.
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

#include "testvectors_aria_cbcmac.h"
#include "testvectors_aria_cbcmac_data.h"

/* Macros used to build the test vectors. */
#define TEST_VECTOR_ARIA_CBCMAC(key, keylen, msg, msglen, mac, maclen) \
    {                                                      \
        test_vector_keys + (key),                          \
        test_vector_msgs + (msg),                          \
        test_vector_macs + (mac),                          \
        keylen,                                            \
        msglen,                                            \
        maclen,                                            \
    }

static const TestVector_ARIA_CBCMAC_Rec_t cbcmac_test_vectors[] =
{
    /* Miscellaneous ARIA CBC-MAC Test Vector #1 */
    TEST_VECTOR_ARIA_CBCMAC(0, 16, 0, 262, 0, 16),
    /* Miscellaneous ARIA CBC-MAC Test Vector #2 */
    TEST_VECTOR_ARIA_CBCMAC(0, 24, 0, 262, 16, 16),
    /* Miscellaneous ARIA CBC-MAC Test Vector #3 */
    TEST_VECTOR_ARIA_CBCMAC(0, 32, 0, 262, 32, 16),
};


/* The function API for accessing the vectors. */
int
test_vectors_aria_cbcmac_num(void)
{
    return sizeof(cbcmac_test_vectors) / sizeof(cbcmac_test_vectors[0]);
}

TestVector_ARIA_CBCMAC_t
test_vectors_aria_cbcmac_get(int Index)
{
    if (Index >= test_vectors_aria_cbcmac_num()) return NULL;
    return &cbcmac_test_vectors[Index];
}

void
test_vectors_aria_cbcmac_release(TestVector_ARIA_CBCMAC_t Vector_p)
{
    /* Test vectors are statically defined => nothing to do. */
    IDENTIFIER_NOT_USED(Vector_p);
}


/* end of file testvectors_aria_cbcmac.c */
