/* testvectors_aes_basic.h
 *
 * Description: Test vectors for AES for modes ECB, CBC and CTR.
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

#ifndef INCLUDE_GUARD_TESTVECTORS_AES_BASIC_H
#define INCLUDE_GUARD_TESTVECTORS_AES_BASIC_H

#include "basic_defs.h"


typedef enum
{
    TESTVECTORS_MODE_ECB = 0,
    TESTVECTORS_MODE_CBC,
    TESTVECTORS_MODE_CTR,
    TESTVECTORS_MODE_ICM,
    TESTVECTORS_MODE_F8,
    TESTVECTORS_MODE_XTS,
    TESTVECTORS_MODE_NUM        // must be last
} TestVectors_Mode_t;

typedef struct
{
    TestVectors_Mode_t Mode;
    const uint8_t * Key_p;
    const uint8_t * Ptx_p;
    const uint8_t * Ctx_p;
    const uint8_t * Iv_p;
    uint32_t KeyLen;
    uint32_t PtxLen;
} TestVector_AES_BASIC_Rec_t;

typedef const TestVector_AES_BASIC_Rec_t * TestVector_AES_BASIC_t;


/* The function API for accessing the vectors. */
int
test_vectors_aes_basic_num(void);

TestVector_AES_BASIC_t
test_vectors_aes_basic_get(int Index);

void
test_vectors_aes_basic_release(TestVector_AES_BASIC_t Vector_p);


#endif /* Include guard */

/* end of file testvectors_aes_basic.h */
