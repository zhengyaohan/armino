/* testvectors_aes_cmac.h
 *
 * Description: Test vectors for AES CMAC.
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

#ifndef INCLUDE_GUARD_TESTVECTORS_AES_CMAC_H
#define INCLUDE_GUARD_TESTVECTORS_AES_CMAC_H

#include "basic_defs.h"

/* Structure for Test Vectors. */
typedef struct
{
    const uint8_t *Key;
    const uint8_t *Msg;
    const uint8_t *Mac;
    uint32_t KeyLen;
    uint32_t MsgLen;
    uint32_t MacLen;
} TestVector_AES_CMAC_Rec_t;

typedef const TestVector_AES_CMAC_Rec_t * TestVector_AES_CMAC_t;

/* API for using AES CMAC test vectors. */

/* Request number of AES CMAC test vectors available. */
int test_vectors_aes_cmac_num(void);

/* Request test vector by index.
   If Index >= test_vectors_aes_cmac_num(), the function returns NULL.

   Note: The function returns zero only when invalid vector has been requested.
   Note: The received vector is constant and must not be altered by caller.
   The test vector is described by structure TestVector_AES_CMAC_t and
   the structure shall be accessed directly by the user of the test vector.
*/
TestVector_AES_CMAC_t test_vectors_aes_cmac_get(int Index);

/* Release a vector.
   It is mandatory to release a vector once user of test vector is finished
   with the vector. */
void test_vectors_aes_cmac_release(TestVector_AES_CMAC_t Vector_p);

/* Search test vector with specific properties.
   The (inline) helper function provides an test vector with
   KeyMin <= KeyLen <= KeyMax
   and
   MsgMin <= MsgLen <= MsgMax.

   Returns NULL if no suitable vector was found.
 */
static inline
TestVector_AES_CMAC_t test_vectors_aes_cmac_search(
        uint32_t KeyMin,
        uint32_t KeyMax,
        uint32_t MsgMin,
        uint32_t MsgMax)
{
    TestVector_AES_CMAC_t vector_p;
    int i;

    for (i = 0; ; i++)
    {
        vector_p = test_vectors_aes_cmac_get(i);

        if (!vector_p)
            break;

        if (vector_p->KeyLen >= KeyMin &&
            vector_p->KeyLen <= KeyMax &&
            vector_p->MsgLen >= MsgMin &&
            vector_p->MsgLen <= MsgMax)
        {
            break;
        }
    }
    return vector_p;
}

#endif /* Include guard */

/* end of file testvectors_aes_cmac.h */
