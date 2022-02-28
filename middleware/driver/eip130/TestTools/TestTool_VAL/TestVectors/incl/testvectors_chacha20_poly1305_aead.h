/* testvectors_chacha20_poly1305_aead.h
 *
 * Description: Test vectors for ChaCha20-Poly1305-AEAD.
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

#ifndef INCLUDE_GUARD_TESTVECTORS_CHACHA20_POLY1305_AEAD_H
#define INCLUDE_GUARD_TESTVECTORS_CHACHA20_POLY1305_AEAD_H

typedef struct
{
    const uint8_t * Key_p;
    const uint8_t * Aad_p;
    const uint8_t * Nonce_p;
    const uint8_t * PlainData_p;
    const uint8_t * CipherData_p;
    const uint8_t * Tag_p;
    uint32_t KeyLen;
    uint32_t AadLen;
    uint32_t NonceLen;
    uint32_t DataLen;
} TestVector_ChaCha20_Poly1305_Rec_t;

typedef const TestVector_ChaCha20_Poly1305_Rec_t * TestVector_ChaCha20_Poly1305_t;


/* The function API for accessing the vectors. */
int
test_vectors_chacha20_poly1305_num(void);

TestVector_ChaCha20_Poly1305_t
test_vectors_chacha20_poly1305_get(int Index);

void
test_vectors_chacha20_poly1305_release(TestVector_ChaCha20_Poly1305_t Vector_p);

#endif /* Include guard */

/* end of file testvectors_chacha20_poly1305_aead.h */
