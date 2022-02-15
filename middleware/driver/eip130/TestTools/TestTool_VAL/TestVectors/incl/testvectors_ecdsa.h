/* testvectors_ecdsa.h
 *
 * Description: Test vectors for ECDSA.
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

#ifndef INCLUDE_GUARD_TESTVECTORS_ECDSA_H
#define INCLUDE_GUARD_TESTVECTORS_ECDSA_H

#include "basic_defs.h"
#include "testvectors_ecc_curves.h"

/* Structures for Test Vectors. */
typedef struct
{
    const TestVector_ECC_Curve_Rec_t * Curve_p;
    const uint8_t * PrivateKey_p;
    uint32_t PrivateKeyLen;
    const uint8_t * PublicKeyX_p;
    uint32_t PublicKeyXLen;
    const uint8_t * PublicKeyY_p;
    uint32_t PublicKeyYLen;
    const uint8_t * Message_p;
    uint32_t MessageLen;
    uint32_t HashBits;
    const uint8_t * SignatureR_p;
    uint32_t SignatureRLen;
    const uint8_t * SignatureS_p;
    uint32_t SignatureSLen;
} TestVector_ECDSA_Rec_t;

typedef const TestVector_ECDSA_Rec_t * TestVector_ECDSA_t;

/* API for using ECDSA test vectors. */

/* Request number of ECDSA test vectors available. */
int test_vectors_ecdsa_num(void);

/* Request test vector by index.
   If Index >= test_vectors_ecdsa_num(), the function returns NULL.

   Note: The function returns zero only when invalid vector has been requested.
   Note: The received vector is constant and must not be altered by caller.
   The test vector is described by structure TestVector_ECDSA_t and
   the structure shall be accessed directly by the user of the test vector.
*/
TestVector_ECDSA_t test_vectors_ecdsa_get(int Index);

/* Release a vector.
   It is mandatory to release a vector once user of test vector is finished
   with the vector. */
void test_vectors_ecdsa_release(TestVector_ECDSA_t Vector_p);

/* Search test vector with specific properties.
   The (inline) helper function provides an test vector with
   CurveBitsMin <= CurveBits <= CurveBitsMax
   and
   HashBitsMin <= HashBits <= HashBitsMax.

   Returns NULL if no suitable vector was found.
 */
static inline
TestVector_ECDSA_t test_vectors_ecdsa_search(
        uint32_t CurveBitsMin,
        uint32_t CurveBitsMax,
        uint32_t HashBitsMin,
        uint32_t HashBitsMax)
{
    TestVector_ECDSA_t vector_p;
    int i;

    for (i = 0; ; i++)
    {
        vector_p = test_vectors_ecdsa_get(i);
        if (vector_p == NULL)
        {
            break;
        }

        if (vector_p->Curve_p->CurveBits >= CurveBitsMin &&
            vector_p->Curve_p->CurveBits <= CurveBitsMax &&
            vector_p->HashBits >= HashBitsMin &&
            vector_p->HashBits <= HashBitsMax)
        {
            break;
        }

        test_vectors_ecdsa_release(vector_p);
    }

    return vector_p;
}

#endif /* Include guard */

/* end of file testvectors_ecdsa.h */
