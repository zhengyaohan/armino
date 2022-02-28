/* testvectors_aes_wrap.h
 *
 * Description: Test vectors for AES-WRAP.
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

#ifndef INCLUDE_GUARD_TESTVECTORS_AES_WRAP_H
#define INCLUDE_GUARD_TESTVECTORS_AES_WRAP_H

#include "basic_defs.h"

typedef struct
{
    const uint8_t * WrapKey_p;
    const uint8_t * PlainTxt_p;
    const uint8_t * WrappedTxt_p;
    uint32_t KeyLen;
    uint32_t PlainTxtLen;
} TestVector_AES_WRAP_Rec_t;

typedef const TestVector_AES_WRAP_Rec_t * TestVector_AES_WRAP_t;

/* The function API for accessing the vectors. */

int
test_vectors_aes_wrap_num(void);

TestVector_AES_WRAP_t
test_vectors_aes_wrap_get(int Index);

void
test_vectors_aes_wrap_release(TestVector_AES_WRAP_t Vector_p);

#endif /* Include guard */

/* end of file testvectors_aes_wrap.h */
