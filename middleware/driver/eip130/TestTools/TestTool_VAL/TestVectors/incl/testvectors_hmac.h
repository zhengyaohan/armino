/* testvectors_hmac.h
 *
 * Description: Test vectors for various HMAC test vectors.
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

#ifndef INCLUDE_GUARD_TESTVECTORS_HMAC_H
#define INCLUDE_GUARD_TESTVECTORS_HMAC_H

#include "basic_defs.h"
#include "testvectors_hash.h"       // TestVectors_HashAlgo_t

typedef struct
{
    TestVectors_HashAlgo_t Algorithm;
    const uint8_t * Key_p;
    uint32_t KeyLen;
    const uint8_t * Msg_p;
    uint32_t MsgLen;
    const uint8_t * Mac_p;
    uint32_t MacLen;
} TestVector_HMAC_Rec_t;

typedef const TestVector_HMAC_Rec_t * TestVector_HMAC_t;

/* The function API for accessing the vectors. */

int
test_vectors_hmac_num(void);

TestVector_HMAC_t
test_vectors_hmac_get(int Index);

void
test_vectors_hmac_release(TestVector_HMAC_t Vector_p);

#endif /* Include guard */

/* end of file testvectors_hmac.h */
