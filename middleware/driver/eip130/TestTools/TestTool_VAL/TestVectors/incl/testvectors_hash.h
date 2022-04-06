/* testvectors_hash.h
 *
 * Description: Test vectors for various hash test vectors.
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

#ifndef INCLUDE_GUARD_TESTVECTORS_HASH_H
#define INCLUDE_GUARD_TESTVECTORS_HASH_H

#include "basic_defs.h"

typedef enum
{
    TESTVECTORS_HASH_SHA160 = 1,
    TESTVECTORS_HASH_SHA224,
    TESTVECTORS_HASH_SHA256,
    TESTVECTORS_HASH_SHA384,
    TESTVECTORS_HASH_SHA512
} TestVectors_HashAlgo_t;

typedef struct
{
    TestVectors_HashAlgo_t Algorithm;
    const uint8_t * Msg_p;
    const uint8_t * Digest_p;
    uint32_t MsgLen;
    uint32_t DigestLen;
} TestVector_HASH_Rec_t;

typedef const TestVector_HASH_Rec_t * TestVector_HASH_t;

/* The function API for accessing the vectors. */

int
test_vectors_hash_num(void);

TestVector_HASH_t
test_vectors_hash_get(
        int Index);

void
test_vectors_hash_release(
        TestVector_HASH_t Vector_p);

#endif /* Include guard */

/* end of file testvectors_hash.h */
