/* testvectors_eddsa.c
 *
 * Description: Test vectors for EdDSA algorithm.
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

#include "testvectors_eddsa.h"

/* Actual vectors, from automatically generated file. */
#include "testvectors_eddsa_data.h"

/* The function API for accessing the vectors. */

int
test_vectors_eddsa_num(void)
{
    return sizeof(test_vectors_eddsa) / sizeof(test_vectors_eddsa[0]);
}

TestVector_EdDSA_t
test_vectors_eddsa_get(int Index)
{
    if (Index >= test_vectors_eddsa_num())
    {
        return NULL;
    }
    return &test_vectors_eddsa[Index];
}

void
test_vectors_eddsa_release(TestVector_EdDSA_t Vector_p)
{
    /* Test vectors are statically defined => nothing to do. */
    IDENTIFIER_NOT_USED(Vector_p);
}

/* end of file testvectors_eddsa.c */
