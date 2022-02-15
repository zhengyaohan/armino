/* testvectors_aes_siv.c
 *
 * Description: Test vectors for AES SIV.
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

#include "testvectors_aes_siv.h"
#include "testvectors_aes_siv_data.h"


/* The function API for accessing the vectors. */
int
test_vectors_aes_siv_num(void)
{
    return sizeof(siv_test_vectors) / sizeof(siv_test_vectors[0]);
}

TestVector_AES_SIV_t
test_vectors_aes_siv_get(int Index)
{
    if (Index >= test_vectors_aes_siv_num()) return NULL;
    return &siv_test_vectors[Index];
}

void
test_vectors_aes_siv_release(TestVector_AES_SIV_t Vector_p)
{
    /* Test vectors are statically defined => nothing to do. */
    IDENTIFIER_NOT_USED(Vector_p);
}


/* end of file testvectors_aes_siv.c */
