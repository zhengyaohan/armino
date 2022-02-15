/* testvectors_aes_cmac_s2v.h
 *
 * Description: Test vectors for AES CMAC_S2V.
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

#ifndef INCLUDE_GUARD_TESTVECTORS_AES_CMAC_S2V_H
#define INCLUDE_GUARD_TESTVECTORS_AES_CMAC_S2V_H

#include "basic_defs.h"

/* Structure for S2V Test Vectors builds upon CMAC test vectors. */
#include "testvectors_aes_cmac.h"

typedef struct
{
    bool Encrypt;   /* Support encryption as well. Can test SIV. */
    uint32_t Count; /* Number of valid vectors within the array */
    const TestVector_AES_CMAC_Rec_t Vector[10];
} TestVector_AES_CMAC_S2V_Rec_t;

typedef const TestVector_AES_CMAC_S2V_Rec_t * TestVector_AES_CMAC_S2V_t;

/* API for using AES CMAC_S2V test vectors. */

/* Request number of AES CMAC_S2V test vectors available. */
int test_vectors_aes_cmac_s2v_num(void);

/* Request test vector by index.
   If Index >= test_vectors_aes_cmac_s2v_num(), the function returns NULL.

   Note: The function returns zero only when invalid vector has been requested.
   Note: The received vector is constant and must not be altered by caller.
   The test vector is described by structure TestVector_AES_CMAC_S2V_t and
   the structure shall be accessed directly by the user of the test vector.
*/
TestVector_AES_CMAC_S2V_t test_vectors_aes_cmac_s2v_get(int Index);

/* Release a vector.
   It is mandatory to release a vector once user of test vector is finished
   with the vector. */
void test_vectors_aes_cmac_s2v_release(TestVector_AES_CMAC_S2V_t Vector_p);

#endif /* Include guard */

/* end of file testvectors_aes_cmac_s2v.h */
