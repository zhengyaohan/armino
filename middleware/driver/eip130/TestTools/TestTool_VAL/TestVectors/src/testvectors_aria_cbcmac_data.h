/* testvectors_aria_cbcmac_data.h
 *
 * Description: Test vectors for ARIA CBC-MAC, test vector contents.
 */

/*****************************************************************************
* Copyright (c) 2018 INSIDE Secure B.V. All Rights Reserved.
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

/* The key material for the test vectors. */
static const uint8_t test_vector_keys[] =
{
    /* Miscellaneous ARIA CBC-MAC Test Vector #1 */
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
    /* Miscellaneous ARIA CBC-MAC Test Vector #2 - extension to 192 */
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
    /* Miscellaneous ARIA CBC-MAC Test Vector #3 - extension to 256 */
    0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20,
};

/* The msg material for the test vectors. */
static const uint8_t test_vector_msgs[] =
{
    /* Miscellaneous ARIA CBC-MAC Test Vector #1/#2/#3 */
    0x6B, 0x91, 0x8F, 0xB1, 0xA5, 0xAD, 0x1F, 0x9C, 0x5E, 0x5D, 0xBD, 0xF1, 0x0A, 0x93, 0xA9, 0xC8,
    0xF6, 0xBC, 0xA8, 0x9F, 0x37, 0xE7, 0x9C, 0x9F, 0xE1, 0x2A, 0x57, 0x22, 0x79, 0x41, 0xB1, 0x73,
    0xAC, 0x79, 0xD8, 0xD4, 0x40, 0xCD, 0xE8, 0xC6, 0x4C, 0x4E, 0xBC, 0x84, 0xA4, 0xC8, 0x03, 0xD1,
    0x98, 0xA2, 0x96, 0xF3, 0xDE, 0x06, 0x09, 0x00, 0xCC, 0x42, 0x7F, 0x58, 0xCA, 0x6E, 0xC3, 0x73,
    0x08, 0x4F, 0x95, 0xDD, 0x6C, 0x7C, 0x42, 0x7E, 0xCF, 0xBF, 0x78, 0x1F, 0x68, 0xBE, 0x57, 0x2A,
    0x88, 0xDB, 0xCB, 0xB1, 0x88, 0x58, 0x1A, 0xB2, 0x00, 0xBF, 0xB9, 0x9A, 0x3A, 0x81, 0x64, 0x07,
    0xE7, 0xDD, 0x6D, 0xD2, 0x10, 0x03, 0x55, 0x4D, 0x4F, 0x7A, 0x99, 0xC9, 0x3E, 0xBF, 0xCE, 0x5C,
    0x30, 0x2F, 0xF0, 0xE1, 0x1F, 0x26, 0xF8, 0x3F, 0xE6, 0x69, 0xAC, 0xEF, 0xB0, 0xC1, 0xBB, 0xB8,
    0xB1, 0xE9, 0x09, 0xBD, 0x14, 0xAA, 0x48, 0xBA, 0x34, 0x45, 0xC8, 0x8B, 0x0E, 0x11, 0x90, 0xEE,
    0xF7, 0x65, 0xAD, 0x89, 0x8A, 0xB8, 0xCA, 0x2F, 0xE5, 0x07, 0x01, 0x5F, 0x15, 0x78, 0xF1, 0x0D,
    0xCE, 0x3C, 0x11, 0xA5, 0x5F, 0xB9, 0x43, 0x4E, 0xE6, 0xE9, 0xAD, 0x6C, 0xC0, 0xFD, 0xC4, 0x68,
    0x44, 0x47, 0xA9, 0xB3, 0xB1, 0x56, 0xB9, 0x08, 0x64, 0x63, 0x60, 0xF2, 0x4F, 0xEC, 0x2D, 0x8F,
    0xA6, 0x9E, 0x2C, 0x93, 0xDB, 0x78, 0x70, 0x8F, 0xCD, 0x2E, 0xEF, 0x74, 0x3D, 0xCB, 0x93, 0x53,
    0x81, 0x9B, 0x8D, 0x66, 0x7C, 0x48, 0xED, 0x54, 0xCD, 0x43, 0x6F, 0xB1, 0x47, 0x65, 0x98, 0xC4,
    0xA1, 0xD7, 0x02, 0x8E, 0x6F, 0x2F, 0xF5, 0x07, 0x51, 0xDB, 0x36, 0xAB, 0x6B, 0xC3, 0x24, 0x35,
    0x15, 0x2A, 0x00, 0xAB, 0xD3, 0xD5, 0x8D, 0x9A, 0x87, 0x70, 0xD9, 0xA3, 0xE5, 0x2D, 0x5A, 0x36,
    0x28, 0xAE, 0x3C, 0x9E, 0x03, 0x25
};

/* Expected macs for the test vectors. */
static const uint8_t test_vector_macs[] =
{
    /* Miscellaneous ARIA CBC-MAC Test Vector #1 */
    0x2E, 0x22, 0x7E, 0x9F, 0x13, 0xDD, 0x3C, 0x4A, 0x77, 0xE8, 0x13, 0xB4, 0x33, 0x91, 0x40, 0xF3,
    /* Miscellaneous ARIA CBC-MAC Test Vector #2 */
    0x8D, 0xDB, 0x50, 0x1A, 0x04, 0xCA, 0x0A, 0xA4, 0x1F, 0xBA, 0x4F, 0xC7, 0x10, 0x2C, 0xFE, 0xF9,
    /* Miscellaneous ARIA CBC-MAC Test Vector #3 */
    0x2F, 0xE4, 0x07, 0x08, 0x3B, 0xFA, 0x8C, 0x98, 0x30, 0xE2, 0x67, 0x7E, 0xA9, 0x18, 0x40, 0x95
};


/* end of file testvectors_aria_cbcmac_data.h */
