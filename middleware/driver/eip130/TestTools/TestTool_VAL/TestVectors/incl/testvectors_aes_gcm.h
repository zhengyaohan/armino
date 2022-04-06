/* testvectors_aes_gcm.h
 *
 * Description: Test vectors for AES GCM.
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

#ifndef INCLUDE_GUARD_TESTVECTORS_AES_GCM_H
#define INCLUDE_GUARD_TESTVECTORS_AES_GCM_H

#include "basic_defs.h"

typedef struct
{
    const uint8_t * Key_p;
    const uint8_t * Pt_p;
    const uint8_t * Ct_p;
    const uint8_t * Aad_p;
    const uint8_t * IV_p;
    const uint8_t * H_p;
    const uint8_t * Y0_p;
    const uint8_t * Ghash_p;
    const uint8_t * Tag_p;
    uint32_t KeyLen;
    uint32_t PtLen;
    uint32_t CtLen;
    uint32_t AadLen;
    uint32_t IVLen;
    uint32_t HLen;
    uint32_t Y0Len;
    uint32_t GhashLen;
    uint32_t TagLen;
} TestVector_AES_GCM_Rec_t;

typedef const TestVector_AES_GCM_Rec_t * TestVector_AES_GCM_t;

/* API for using AES GCM test vectors. */

/* Request number of AES GCM test vectors available. */
int test_vectors_aes_gcm_num(void);

/* Request test vector by index.
   If Index >= test_vectors_aes_gcm_num(), the function returns NULL.

   Note: The function returns zero only when invalid vector has been requested.
   Note: The received vector is constant and must not be altered by caller.
   The test vector is described by structure TestVector_AES_GCM_t and
   the structure shall be accessed directly by the user of the test vector.
*/
TestVector_AES_GCM_t test_vectors_aes_gcm_get(int Index);

/* Release a vector.
   It is mandatory to release a vector once user of test vector is finished
   with the vector. */
void test_vectors_aes_gcm_release(TestVector_AES_GCM_t Vector_p);

#endif /* Include guard */

/* end of file testvectors_aes_gcm.h */
