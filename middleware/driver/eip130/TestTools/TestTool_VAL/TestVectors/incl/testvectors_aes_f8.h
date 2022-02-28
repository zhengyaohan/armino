/* testvectors_aes_f8.h
 *
 * Description: Test vectors for AES-f8.
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

#ifndef INCLUDE_GUARD_TESTVECTORS_AES_F8_H
#define INCLUDE_GUARD_TESTVECTORS_AES_F8_H

#include "basic_defs.h"

typedef struct
{
    const uint8_t * Key_p;
    const uint8_t * Ptx_p;
    const uint8_t * Ctx_p;
    const uint8_t * Iv_p;
    const uint8_t * SaltKey_p;
    uint32_t KeyLen;
    uint32_t PtxLen;
    uint32_t SaltKeyLen;
} TestVector_AES_f8_Rec_t;

typedef const TestVector_AES_f8_Rec_t * TestVector_AES_f8_t;

/* The function API for accessing the vectors. */

int
test_vectors_aes_f8_num(void);

TestVector_AES_f8_t
test_vectors_aes_f8_get(int Index);

void
test_vectors_aes_f8_release(TestVector_AES_f8_t Vector_p);

#endif /* Include guard */

/* end of file testvectors_aes_f8.h */
