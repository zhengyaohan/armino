/* testvectors_aria_basic.h
 *
 * Description: Test vectors for ARIA for modes ECB, CBC and CTR.
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

#ifndef INCLUDE_GUARD_TESTVECTORS_ARIA_BASIC_H
#define INCLUDE_GUARD_TESTVECTORS_ARIA_BASIC_H

#include "basic_defs.h"
#include "testvectors_aes_basic.h"


typedef struct
{
    TestVectors_Mode_t Mode;
    const uint8_t * Key_p;
    const uint8_t * Ptx_p;
    const uint8_t * Ctx_p;
    const uint8_t * Iv_p;
    uint32_t KeyLen;
    uint32_t PtxLen;
} TestVector_ARIA_BASIC_Rec_t;

typedef const TestVector_ARIA_BASIC_Rec_t * TestVector_ARIA_BASIC_t;


/* The function API for accessing the vectors. */
int
test_vectors_aria_basic_num(void);

TestVector_ARIA_BASIC_t
test_vectors_aria_basic_get(int Index);

void
test_vectors_aria_basic_release(TestVector_ARIA_BASIC_t Vector_p);


#endif /* Include guard */

/* end of file testvectors_aria_basic.h */
