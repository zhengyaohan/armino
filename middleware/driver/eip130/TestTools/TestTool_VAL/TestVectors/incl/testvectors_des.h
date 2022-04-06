/* testvectors_des.h
 *
 * Description: Test vectors for DES.
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

#ifndef INCLUDE_GUARD_TESTVECTORS_DES_H
#define INCLUDE_GUARD_TESTVECTORS_DES_H

#include "basic_defs.h"
#include "testvectors_aes_basic.h"


/* Re-use AES basic test vector type for DES. */
typedef const TestVector_AES_BASIC_Rec_t * TestVector_DES_t;

/* The function API for accessing the vectors. */
int
test_vectors_des_num(void);

TestVector_AES_BASIC_t
test_vectors_des_get(int Index);

void
test_vectors_des_release(TestVector_AES_BASIC_t Vector_p);


#endif /* Include guard */

/* end of file testvectors_des.h */
