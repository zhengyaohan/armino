/* testvectors_dsa.h
 *
 * Description: Test vectors for DSA.
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

#ifndef INCLUDE_GUARD_TESTVECTORS_DSA_H
#define INCLUDE_GUARD_TESTVECTORS_DSA_H

#include "basic_defs.h"

/* Structures for Test Vectors. */
typedef struct
{
    uint32_t L_Bits;
    uint32_t N_Bits;
    uint32_t HashBits;
    const uint8_t * DomainParamsP_p;
    uint32_t DomainParamsPSize;
    const uint8_t * DomainParamsQ_p;
    uint32_t DomainParamsQSize;
    const uint8_t * DomainParamsG_p;
    uint32_t DomainParamsGSize;
    const uint8_t * Msg_p;
    uint32_t MsgSize;
    const uint8_t * PublicKeyY_p;
    uint32_t PublicKeyYSize;
    const uint8_t * SignatureR_p;
    uint32_t SignatureRSize;
    const uint8_t * SignatureS_p;
    uint32_t SignatureSSize;
} TestVector_DSA_Rec_t;

typedef const TestVector_DSA_Rec_t * TestVector_DSA_t;

/* API for using DSA test vectors. */

/* Request number of DSA test vectors available. */
int test_vectors_dsa_num(void);

/* Request test vector by index.
   If Index >= test_vectors_dsa_num(), the function returns NULL.

   Note: The function returns zero only when invalid vector has been requested.
   Note: The received vector is constant and must not be altered by caller.
   The test vector is described by structure TestVector_DSA_t and
   the structure shall be accessed directly by the user of the test vector.
*/
TestVector_DSA_t test_vectors_dsa_get(int Index);

/* Release a vector.
   It is mandatory to release a vector once user of test vector is finished
   with the vector. */
void test_vectors_dsa_release(TestVector_DSA_t Vector_p);

#endif /* Include guard */

/* end of file testvectors_dsa.h */
