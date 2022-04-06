/* testvectors_poly1305.c
 *
 * Description: Test vectors for Poly1305
 */

/*****************************************************************************
* Copyright (c) 2016-2018 INSIDE Secure B.V. All Rights Reserved.
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

#include "testvectors_poly1305.h"
#include "testvectors_poly1305_data.h"

/* Macros used to build the test vectors. */
#define TEST_VECTOR_POLY1305(key, msg, tag)   \
    {                                         \
        key, msg, tag,                        \
        sizeof(key), sizeof(msg), sizeof(tag) \
    }

static const TestVector_Poly1305_Rec_t Poly1305_test_vectors [] =
{
    TEST_VECTOR_POLY1305(Poly1305_KAT1_Key, Poly1305_KAT1_Message, Poly1305_KAT1_Tag),
    TEST_VECTOR_POLY1305(Poly1305_KAT2_Key, Poly1305_KAT2_Message, Poly1305_KAT2_Tag),
    TEST_VECTOR_POLY1305(Poly1305_KAT3_Key, Poly1305_KAT3_Message, Poly1305_KAT3_Tag),
    TEST_VECTOR_POLY1305(Poly1305_KAT4_Key, Poly1305_KAT4_Message, Poly1305_KAT4_Tag),
    TEST_VECTOR_POLY1305(Poly1305_KAT5_Key, Poly1305_KAT5_Message, Poly1305_KAT5_Tag),
};


/* The function API for accessing the vectors. */
int
test_vectors_poly1305_num(void)
{
    return sizeof(Poly1305_test_vectors) / sizeof(Poly1305_test_vectors[0]);
}

TestVector_Poly1305_t
test_vectors_poly1305_get(int Index)
{
    if (Index >= test_vectors_poly1305_num())
        return NULL;

    return &Poly1305_test_vectors[Index];
}

void
test_vectors_poly1305_release(TestVector_Poly1305_t Vector_p)
{
    /* Test vectors are statically defined => nothing to do. */
    IDENTIFIER_NOT_USED(Vector_p);
}


/* end of file testvectors_poly1305.c */
