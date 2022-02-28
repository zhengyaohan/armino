/* testvectors_ecc_elgamal.c
 *
 * Description: Test vectors for ECC ElGamal algorithm.
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

#include "testvectors_ecc_elgamal.h"

/* Actual vectors, from automatically generated file. */
#include "testvectors_ecc_elgamal_data.h"


/* The function API for accessing the vectors. */

int
test_vectors_ecc_elgamal_num(void)
{
    return sizeof(test_vectors_ecc_elgamal) / sizeof(test_vectors_ecc_elgamal[0]);
}

TestVector_ECC_ElGamal_t
test_vectors_ecc_elgamal_get(int Index)
{
    if (Index >= test_vectors_ecc_elgamal_num())
    {
        return NULL;
    }

    return &test_vectors_ecc_elgamal[Index];
}

void
test_vectors_ecc_elgamal_release(TestVector_ECC_ElGamal_t Vector_p)
{
    /* Test vectors are statically defined => nothing to do. */
    IDENTIFIER_NOT_USED(Vector_p);
}

/* end of file testvectors_ecc_elgamal.c */
