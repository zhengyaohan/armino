/* testvectors_ecdsa.c
 *
 * Description: Test vectors for ECDSA algorithm.
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

#include "c_test_val.h"                 // configuration

#include "basic_defs.h"

#include "testvectors_ecdsa.h"
#include "testvectors_ecdsa_data.h"

/* The function API for accessing the vectors. */

int
test_vectors_ecdsa_num(void)
{
    return sizeof(test_vectors_ecdsa) / sizeof(test_vectors_ecdsa[0]);
}

TestVector_ECDSA_t
test_vectors_ecdsa_get(int Index)
{
    if (Index >= test_vectors_ecdsa_num())
    {
        return NULL;
    }
    return &test_vectors_ecdsa[Index];
}

void
test_vectors_ecdsa_release(TestVector_ECDSA_t Vector_p)
{
    /* Test vectors are statically defined => nothing to do. */
    IDENTIFIER_NOT_USED(Vector_p);
}

/* end of file testvectors_ecdsa.c */
