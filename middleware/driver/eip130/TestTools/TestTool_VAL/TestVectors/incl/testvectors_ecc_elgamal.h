/* testvectors_ecc_elgamal.h
 *
 * Description: Test vectors for ECC ElGamal.
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

#ifndef INCLUDE_GUARD_TESTVECTORS_ECC_ELGAMAL_H
#define INCLUDE_GUARD_TESTVECTORS_ECC_ELGAMAL_H

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
    const uint8_t * MessageX_p;
    uint32_t MessageXLen;
    const uint8_t * MessageY_p;
    uint32_t MessageYLen;
    const uint8_t * CipherTextC_X_p;
    uint32_t CipherTextC_XLen;
    const uint8_t * CipherTextC_Y_p;
    uint32_t CipherTextC_YLen;
    const uint8_t * CipherTextD_X_p;
    uint32_t CipherTextD_XLen;
    const uint8_t * CipherTextD_Y_p;
    uint32_t CipherTextD_YLen;
} TestVector_ECC_ElGamal_Rec_t;

typedef const TestVector_ECC_ElGamal_Rec_t * TestVector_ECC_ElGamal_t;

/* API for using ECC ElGamal test vectors. */

/* Request number of ECC ElGamal test vectors available. */
int test_vectors_ecc_elgamal_num(void);

/* Request test vector by index.
   If Index >= test_vectors_ecdsa_num(), the function returns NULL.

   Note: The function returns zero only when invalid vector has been requested.
   Note: The received vector is constant and must not be altered by caller.
   The test vector is described by structure TestVector_ECC_ElGamal_t and
   the structure shall be accessed directly by the user of the test vector.
*/
TestVector_ECC_ElGamal_t test_vectors_ecc_elgamal_get(int Index);

/* Release a vector.
   It is mandatory to release a vector once user of test vector is finished
   with the vector. */
void test_vectors_ecc_elgamal_release(TestVector_ECC_ElGamal_t Vector_p);

#endif /* Include guard */

/* end of file testvectors_ecc_elgamal.h */
