/* testvectors_aes_siv.h
 *
 * Description: Test vectors for AES SIV.
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

#ifndef INCLUDE_GUARD_TESTVECTORS_AES_SIV_H
#define INCLUDE_GUARD_TESTVECTORS_AES_SIV_H

#include "basic_defs.h"

/* Structure for additional AES-SIV Aad.
   AES-SIV supports variable number of AAD arguments, unlike most other AEAD.
   The additional AAD are in a linked list.
   Current sfzcrypto_auth_crypt does not support additional AAD arguments and
   therefore, there are currently no test vectors using
   TestVector_AES_SIV_NextAad_t.
*/
typedef struct TestVector_AES_SIV_NextAad_Struct
{
    const uint8_t *Aad;
    uint32_t AadLen;
    const struct TestVector_AES_SIV_NextAad_Struct * NextAad;
} TestVector_AES_SIV_NextAad_t;

/* Structure for AES-SIV test vectors. */
typedef struct
{
    const uint8_t *Key;
    const uint8_t *Nonce;
    const uint8_t *Msg;
    const uint8_t *Result;
    const uint8_t *Aad;
    uint32_t KeyLen;
    uint32_t NonceLen;
    uint32_t MsgLen;
    uint32_t ResultLen;
    uint32_t AadLen;
    const TestVector_AES_SIV_NextAad_t *NextAad;
} TestVector_AES_SIV_Rec_t;

typedef const TestVector_AES_SIV_Rec_t * TestVector_AES_SIV_t;

/* API for using AES SIV test vectors. */

/* Request number of AES SIV test vectors available. */
int test_vectors_aes_siv_num(void);

/* Request test vector by index.
   If Index >= test_vectors_aes_siv_num(), the function returns NULL.

   Note: The function returns zero only when invalid vector has been requested.
   Note: The received vector is constant and must not be altered by caller.
   The test vector is described by structure TestVector_AES_SIV_t and
   the structure shall be accessed directly by the user of the test vector.
*/
TestVector_AES_SIV_t test_vectors_aes_siv_get(int Index);

/* Release a vector.
   It is mandatory to release a vector once user of test vector is finished
   with the vector. */
void test_vectors_aes_siv_release(TestVector_AES_SIV_t Vector_p);

#endif /* Include guard */

/* end of file testvectors_aes_siv.h */
