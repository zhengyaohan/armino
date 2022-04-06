/* testvectors_xts_aes.h
 *
 * Description: Test vectors for XTS-AES-128 and XTS-AES-256.
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

#ifndef INCLUDE_GUARD_TESTVECTORS_XTS_AES_H
#define INCLUDE_GUARD_TESTVECTORS_XTS_AES_H

#include "basic_defs.h"

typedef struct
{
    const uint8_t * Key_p;              // AES-128 or AES-256 keys.
    const uint8_t * Ptx_p;
    const uint8_t * Ctx_p;
    const uint8_t * Tweak_p;            // always 128-bit
    uint32_t KeyLen;                    // 256-bit or 512-bit
    uint32_t PtxLen;
} TestVector_XTS_AES_Rec_t;

typedef const TestVector_XTS_AES_Rec_t * TestVector_XTS_AES_t;


/* The function API for accessing the vectors. */

int
test_vectors_xts_aes_num(void);

TestVector_XTS_AES_t
test_vectors_xts_aes_get(
    int Index);

void
test_vectors_xts_aes_release(
    TestVector_XTS_AES_t Vector_p);

#endif /* Include guard */

/* end of file testvectors_xts_aes.h */
