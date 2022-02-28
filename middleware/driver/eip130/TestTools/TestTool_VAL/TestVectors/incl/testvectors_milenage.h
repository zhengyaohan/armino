/* testvectors_milenage.h
 *
 * Description: Test vectors for Milenage.
 */

/*****************************************************************************
* Copyright (c) 2017-2018 INSIDE Secure B.V. All Rights Reserved.
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

#ifndef INCLUDE_GUARD_TESTVECTORS_POLY1305_H
#define INCLUDE_GUARD_TESTVECTORS_POLY1305_H

#include "basic_defs.h"

typedef struct
{
    const uint8_t * K_p;
    const uint8_t * RAND_p;
    const uint8_t * SQN_p;
    const uint8_t * AMF_p;
    const uint8_t * OP_p;
    const uint8_t * OPc_p;
    const uint8_t * f1_p;
    const uint8_t * f1star_p;
    const uint8_t * f2_p;
    const uint8_t * f3_p;
    const uint8_t * f4_p;
    const uint8_t * f5_p;
    const uint8_t * f5star_p;
    uint32_t K_Len;
    uint32_t RAND_Len;
    uint32_t SQN_Len;
    uint32_t AMF_Len;
    uint32_t OP_Len;
    uint32_t OPc_Len;
    uint32_t f1_Len;
    uint32_t f1star_Len;
    uint32_t f2_Len;
    uint32_t f3_Len;
    uint32_t f4_Len;
    uint32_t f5_Len;
    uint32_t f5star_Len;
} TestVector_Milenage_Rec_t;

typedef const TestVector_Milenage_Rec_t * TestVector_Milenage_t;

/* The function API for accessing the vectors. */

int
test_vectors_milenage_num(void);

TestVector_Milenage_t
test_vectors_milenage_get(int Index);

void
test_vectors_milenage_release(TestVector_Milenage_t Vector_p);

#endif /* Include guard */

/* end of file testvectors_milenage.h */
