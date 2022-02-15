/*
*  File: testvectors_aes_siv_data.h
*
*  Description :  Test vectors for AES SIV, test vector contents.
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

/* Helper arrays, referenced from siv_test_vectors. */
static uint8_t test_vector_keys[64];
static uint8_t test_vector_msgs[47 + 104];
static uint8_t test_vector_outs[32 + 14];

/* Actual test vectors. */
static
const TestVector_AES_SIV_Rec_t siv_test_vectors[] = {
    /* First vector from RFC 5297, formed of two parts. */
    {
        &test_vector_keys[0],
        NULL,
        &test_vector_msgs[24],
        &test_vector_outs[0],
        &test_vector_msgs[0],
        32,
        0,
        14,
        16 + 14,
        24,
        NULL,
    }
    /* Second vector from RFC 5297, omitted as vector uses two AADs. */
};

/* The key material for the test vectors. */
static uint8_t test_vector_keys[] = {
    /* 256-bit key [two actual AES keys] from RFC 5297 */
    0xff, 0xfe, 0xfd, 0xfc, 0xfb, 0xfa, 0xf9, 0xf8,
    0xf7, 0xf6, 0xf5, 0xf4, 0xf3, 0xf2, 0xf1, 0xf0,
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
    0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,
    /* Another 256-bit key from RFC 5297 */
    0x7f, 0x7e, 0x7d, 0x7c, 0x7b, 0x7a, 0x79, 0x78,
    0x77, 0x76, 0x75, 0x74, 0x73, 0x72, 0x71, 0x70,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
    0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f
};

/* The msg material for the test vectors. */
static uint8_t test_vector_msgs[] = {
    /* From RFC 5297. */
    /* First plaintext */
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, /* 24 */
    /* Second plaintext */
    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
    0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee,             /* 14 + 24 */
    /* Plaintext (AD1) */
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
    0xde, 0xad, 0xda, 0xda, 0xde, 0xad, 0xda, 0xda,
    0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa, 0x99, 0x88,
    0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00, /* 40 + 28 */
    /* Plaintext (AD2) */
    0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80,
    0x90, 0xa0,                                     /* 10 + 68 */
    /* Plaintext (Nonce) */
    0x09, 0xf9, 0x11, 0x02, 0x9d, 0x74, 0xe3, 0x5b,
    0xd8, 0x41, 0x56, 0xc5, 0x63, 0x56, 0x88, 0xc0, /* 16 + 78 */
    /* Actual Plaintext */
    0x74, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20,
    0x73, 0x6f, 0x6d, 0x65, 0x20, 0x70, 0x6c, 0x61,
    0x69, 0x6e, 0x74, 0x65, 0x78, 0x74, 0x20, 0x74,
    0x6f, 0x20, 0x65, 0x6e, 0x63, 0x72, 0x79, 0x70,
    0x74, 0x20, 0x75, 0x73, 0x69, 0x6e, 0x67, 0x20,
    0x53, 0x49, 0x56, 0x2d, 0x41, 0x45, 0x53        /* 47 + 94 */
};

/* Expected macs for the test vectors. */
static uint8_t test_vector_outs[] = {
    /* Expected result of the first vector. */
    0x85, 0x63, 0x2d, 0x07, 0xc6, 0xe8, 0xf3, 0x7f,
    0x95, 0x0a, 0xcd, 0x32, 0x0a, 0x2e, 0xcc, 0x93, /* 16 */
    /* Expected result of the first vector, ciphertext. */
    0x40, 0xc0, 0x2b, 0x96, 0x90, 0xc4, 0xdc, 0x04,
    0xda, 0xef, 0x7f, 0x6a, 0xfe, 0x5c,             /* 14 */
    /* Expected result of the second vector. */
    0x7b, 0xdb, 0x6e, 0x3b, 0x43, 0x26, 0x67, 0xeb,
    0x06, 0xf4, 0xd1, 0x4b, 0xff, 0x2f, 0xbd, 0x0f, /* 16 */
};


/* end of file testvectors_aes_siv_data.h */
