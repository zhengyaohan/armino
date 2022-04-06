/* testvectors_eddsa.h
 *
 * Description: Test vectors for EdDSA.
 */

/*****************************************************************************
* Copyright (c) 2016-2018 INSIDE Secure B.V. All Rights Reserved.
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

#ifndef INCLUDE_GUARD_TESTVECTORS_EDDSA_H
#define INCLUDE_GUARD_TESTVECTORS_EDDSA_H

#include "basic_defs.h"
#include "testvectors_ecc_curves.h"

/* Structures for Test Vectors. */
typedef struct
{
    const TestVector_ECC_Curve_Rec_t * Curve_p;
    const uint8_t * PrivateKey_p;
    uint32_t PrivateKeyLen;
    const uint8_t * PublicKey_p;
    uint32_t PublicKeyLen;
    const uint8_t * Message_p;
    uint32_t MessageLen;
    const uint8_t * SignatureR_p;
    uint32_t SignatureRLen;
    const uint8_t * SignatureS_p;
    uint32_t SignatureSLen;
} TestVector_EdDSA_Rec_t;

typedef const TestVector_EdDSA_Rec_t * TestVector_EdDSA_t;

/* API for using EdDSA test vectors. */

/* Request number of EdDSA test vectors available. */
int test_vectors_eddsa_num(void);

/* Request test vector by index.
   If Index >= test_vectors_eddsa_num(), the function returns NULL.

   Notes:
   - The function returns zero only when invalid vector has been requested.
   - The received vector is constant and must not be altered by caller.
     The test vector is described by structure TestVector_EdDSA_t and
     the structure shall be accessed directly by the user of the test vector.
*/
TestVector_EdDSA_t test_vectors_eddsa_get(int Index);

/* Release a vector.
   It is mandatory to release a vector once user of test vector is finished
   with the vector. */
void test_vectors_eddsa_release(TestVector_EdDSA_t Vector_p);


#endif /* Include guard */

/* end of file testvectors_eddsa.h */
