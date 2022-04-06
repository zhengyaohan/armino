/* testvectors_aria_cmac_data.h
 *
 * Description: Test vectors for ARIA CMAC, test vector contents.
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
    /* CMACGenARIA128 COUNT =0 */
    0x42, 0x7B, 0xF4, 0xCD, 0x64, 0x6B, 0x5F, 0x18, 0x23, 0x93, 0xFD, 0x3D, 0xF4, 0xA0, 0x93, 0x80,
    /* CMACGenARIA128 COUNT =5 */
    0x9A, 0x1D, 0x11, 0x69, 0xB5, 0x07, 0xE4, 0x80, 0x05, 0x9D, 0xCA, 0x12, 0xC1, 0x4C, 0x50, 0x0A,
    /* CMACGenARIA128 COUNT =10 */
    0xF6, 0x4D, 0x14, 0x88, 0x42, 0x7E, 0x7F, 0x8D, 0xA2, 0x59, 0x3E, 0xA3, 0xA3, 0xC7, 0xE2, 0x28,
    /* CMACGenARIA128 COUNT =15 */
    0x3C, 0xDC, 0x61, 0x53, 0xEB, 0x07, 0x78, 0x8C, 0x09, 0x95, 0x5C, 0x71, 0xE6, 0xA0, 0x93, 0x03,
    /* CMACGenARIA192 COUNT =20 */
    0x6C, 0x99, 0xB3, 0x72, 0x3B, 0x3D, 0xD4, 0x2B, 0xFD, 0x72, 0x06, 0x63, 0x2F, 0x8F, 0xD6, 0xC1,
    0xEC, 0x14, 0x50, 0x72, 0xBE, 0x33, 0x72, 0xCA,
    /* CMACGenARIA192 COUNT =25 */
    0x8F, 0x62, 0x41, 0x8D, 0x1B, 0x8F, 0x8D, 0x73, 0x4C, 0x93, 0xF8, 0x0D, 0x2D, 0x22, 0xF7, 0x4D,
    0xFB, 0x53, 0xD5, 0xA0, 0x7C, 0xA4, 0xA8, 0x15,
    /* CMACGenARIA192 COUNT =30 */
    0x48, 0xFB, 0x6C, 0x6B, 0xA8, 0xB5, 0xF5, 0xEA, 0x1B, 0xCB, 0xA0, 0x67, 0x71, 0xA4, 0xC6, 0x20,
    0xFF, 0x0E, 0x8F, 0xF5, 0x37, 0x82, 0x1D, 0xCA,
    /* CMACGenARIA192 COUNT =40 */
    0x39, 0xAA, 0x2C, 0xD7, 0x32, 0x42, 0x1A, 0x23, 0x47, 0x51, 0x40, 0xFE, 0x13, 0x1B, 0x9C, 0xFA,
    0x2A, 0xEE, 0x15, 0xF6, 0x45, 0xB6, 0xBC, 0x4D,
    /* CMACGenARIA256 COUNT =47 */
    0x38, 0x2F, 0x4A, 0xE6, 0x78, 0x05, 0x45, 0xBE, 0xD3, 0x81, 0x63, 0xEF, 0xE4, 0xA7, 0x26, 0x3F,
    0x63, 0x1C, 0x35, 0x10, 0x3E, 0xE5, 0x09, 0x3F, 0xEB, 0x72, 0xEA, 0x4B, 0xFC, 0xDB, 0x1A, 0xBA,
    /* CMACGenARIA256 COUNT =50 */
    0x90, 0x30, 0x25, 0xBF, 0xB7, 0xEC, 0xA5, 0x5D, 0xBA, 0x88, 0x1A, 0x60, 0xCB, 0x47, 0x4C, 0xBE,
    0x17, 0x6B, 0x05, 0x66, 0xE4, 0xF8, 0xC7, 0x24, 0xDC, 0xBA, 0xBE, 0xC5, 0xA8, 0x89, 0xF4, 0x7B,
    /* CMACGenARIA256 COUNT =56 */
    0x0E, 0x7B, 0xD8, 0x42, 0x34, 0xB7, 0x56, 0x76, 0xCF, 0x8A, 0x3C, 0xEC, 0x36, 0x34, 0xD8, 0x87,
    0x55, 0x4B, 0x7B, 0x28, 0x15, 0x5D, 0x87, 0x75, 0x19, 0xD8, 0xCA, 0xD9, 0xAB, 0x27, 0x0A, 0xCB,
    /* CMACGenARIA256 COUNT =65 */
    0x7A, 0xB9, 0x6E, 0x25, 0xEB, 0x8D, 0xB7, 0x32, 0x48, 0x22, 0x34, 0x52, 0x68, 0x85, 0x02, 0x3A,
    0xF9, 0x38, 0xF6, 0x7E, 0x0D, 0x9A, 0x3B, 0xAA, 0xD5, 0xA3, 0x3D, 0x8E, 0xAB, 0xD4, 0xC6, 0xD9,
    /* CMACGenARIA128 COUNT =70 */
    0xAD, 0x57, 0x77, 0xAB, 0x87, 0xF8, 0x46, 0x67, 0x16, 0x42, 0xC0, 0x35, 0x3D, 0x61, 0xF1, 0x3D,
    /* CMACGenARIA128 COUNT =75 */
    0xEB, 0xBC, 0xE8, 0x79, 0xB6, 0xA0, 0xB4, 0xF5, 0xEB, 0x4E, 0x03, 0x37, 0xD7, 0xAD, 0xB7, 0xEA,
    /* CMACGenARIA128 COUNT =80 */
    0xF4, 0xE3, 0xAC, 0x7F, 0xB0, 0x95, 0x2E, 0x78, 0x89, 0xF4, 0x8B, 0x2F, 0x38, 0xAE, 0x4E, 0x56,
    /* CMACGenARIA192 COUNT =70 */
    0x36, 0xEE, 0x1E, 0x74, 0xEE, 0x82, 0x6F, 0x99, 0x7E, 0x14, 0x2D, 0x45, 0x05, 0x1D, 0x79, 0x69,
    0xC2, 0xAC, 0xB9, 0x21, 0x6F, 0x66, 0x8E, 0xDC,
    /* CMACGenARIA192 COUNT =75 */
    0x59, 0x05, 0xDF, 0xA3, 0x8B, 0xA5, 0x91, 0x11, 0xF6, 0x88, 0xC2, 0x34, 0xB0, 0x35, 0x69, 0x74,
    0x8E, 0x00, 0x1C, 0x60, 0xF4, 0x8A, 0x84, 0x51,
    /* CMACGenARIA192 COUNT =80 */
    0xFE, 0x08, 0x7A, 0x69, 0xFA, 0x97, 0x81, 0x71, 0xC9, 0xB7, 0xBC, 0x2D, 0x89, 0xDC, 0x5F, 0x56,
    0x1D, 0x2D, 0x80, 0xF4, 0x81, 0x8A, 0x12, 0x7F,
    /* CMACGenARIA256 COUNT =70 */
    0xC1, 0x7B, 0x82, 0x7E, 0x43, 0xF5, 0x4B, 0xB9, 0x0D, 0x67, 0xC6, 0x14, 0xD7, 0xB7, 0x11, 0xC3,
    0x56, 0xD2, 0x33, 0x44, 0x63, 0x43, 0x5D, 0x91, 0x59, 0x04, 0x70, 0x9A, 0xBF, 0x93, 0xBE, 0xF2,
    /* CMACGenARIA256 COUNT =75 */
    0x23, 0xD6, 0xE7, 0xE5, 0x4B, 0x28, 0xD2, 0xC1, 0x18, 0xD8, 0xBA, 0x5F, 0x85, 0x08, 0x0F, 0xFA,
    0x29, 0xFD, 0xC9, 0x3A, 0x5D, 0xBE, 0xAB, 0x66, 0xA1, 0xDE, 0xC9, 0xD5, 0x3C, 0x96, 0x48, 0xA5,
    /* CMACGenARIA256 COUNT =80 */
    0x71, 0x2F, 0x13, 0xBD, 0xAC, 0x4D, 0x8D, 0x13, 0x9F, 0xD0, 0x30, 0xE1, 0x4D, 0xBD, 0xC0, 0x70,
    0x29, 0xD1, 0xD4, 0x86, 0x33, 0x15, 0x22, 0x93, 0x64, 0xB2, 0xFC, 0x2F, 0x4C, 0x05, 0x1B, 0xC4
};

/* The msg material for the test vectors. */
static const uint8_t test_vector_msgs[] =
{
    /* CMACGenARIA128 COUNT =0 (length = 0, offset = 0) */
    /* CMACGenARIA128 COUNT =5 (length = 6, offset = 0) */
    0x7E, 0x68, 0x49, 0x7B, 0x51, 0xBF,
    /* CMACGenARIA128 COUNT =10 (length = 18, offset = 6) */
    0x04, 0x1A, 0xCA, 0x0D, 0xBC, 0xFC, 0x47, 0x72, 0x3D, 0xA5, 0xFD, 0x76, 0xFB, 0x52, 0x8D, 0x4D,
    0x28, 0xF2,
    /* CMACGenARIA128 COUNT =15 (length = 185, offset = 24) */
    0xCA, 0x62, 0x40, 0x68, 0x97, 0x47, 0xEA, 0x9B, 0xAB, 0x46, 0x7F, 0x62, 0x04, 0x72, 0x99, 0x80,
    0xA7, 0xCC, 0xFF, 0x2C, 0x42, 0x5C, 0x6F, 0xC3, 0x10, 0x80, 0x9B, 0xBA, 0xFD, 0x83, 0x71, 0xD6,
    0x0B, 0x71, 0x9F, 0x9F, 0xF4, 0x2A, 0x57, 0x12, 0x11, 0xB6, 0x4F, 0x96, 0x7A, 0xD8, 0x0D, 0x31,
    0x6F, 0xE6, 0x61, 0xFC, 0xF6, 0xD7, 0x33, 0xD4, 0xC6, 0x9E, 0x7E, 0x53, 0x64, 0xB7, 0x9D, 0xFE,
    0x8C, 0xFF, 0xC6, 0xC0, 0xD2, 0xC8, 0xD4, 0x98, 0x89, 0x2D, 0x49, 0x8E, 0xE4, 0xA3, 0x93, 0xEB,
    0x5B, 0xD1, 0x8E, 0xA9, 0x50, 0xA2, 0x4B, 0x29, 0xF3, 0x98, 0x10, 0x24, 0x63, 0x63, 0x9F, 0xE4,
    0x16, 0xB2, 0xBB, 0xB4, 0x7A, 0x49, 0xE9, 0x95, 0xDC, 0x53, 0x74, 0x32, 0x8A, 0xFA, 0xB3, 0x16,
    0x34, 0x36, 0x8E, 0x1D, 0x98, 0xE2, 0x3F, 0x28, 0x5F, 0x15, 0x56, 0x15, 0x41, 0xAF, 0x00, 0xEF,
    0x70, 0x32, 0x88, 0x61, 0x34, 0xD4, 0x1E, 0x71, 0xD2, 0xD2, 0xD8, 0x6B, 0xB3, 0x06, 0xF6, 0x1A,
    0xC2, 0xBC, 0x6A, 0x3E, 0x16, 0xC2, 0x98, 0x3B, 0xD1, 0xBE, 0x5A, 0x0F, 0x48, 0xC4, 0x47, 0x86,
    0x64, 0x28, 0x35, 0xB1, 0x48, 0x91, 0xFC, 0x94, 0x33, 0x4F, 0x7D, 0x1F, 0xA8, 0xEF, 0xE3, 0x60,
    0xCD, 0x0C, 0x29, 0xF6, 0x12, 0x68, 0xDD, 0xC9, 0x12,
    /* CMACGenARIA192 COUNT =20 (length = 0, offset = 209) */
    /* CMACGenARIA192 COUNT =25 (length = 28, offset = 209) */
    0x91, 0x3E, 0x83, 0xC2, 0x0F, 0x7B, 0xED, 0x64, 0xCB, 0xA6, 0x92, 0x45, 0xF4, 0x02, 0x39, 0x63,
    0xA6, 0x0D, 0xF6, 0xCE, 0xBB, 0x05, 0xA1, 0x52, 0xE5, 0xAC, 0x86, 0x56,
    /* CMACGenARIA192 COUNT =30 (length = 52, offset = 237)*/
    0x7C, 0xAA, 0x06, 0x2D, 0x34, 0x20, 0x08, 0x38, 0x1E, 0x79, 0xF8, 0xB9, 0xE5, 0x82, 0xFC, 0x91,
    0xF6, 0xC9, 0xFA, 0x5A, 0x8F, 0x5F, 0x50, 0x4D, 0x19, 0xDD, 0xC6, 0x62, 0xB8, 0xA5, 0xC0, 0xB8,
    0x20, 0x6A, 0xB9, 0x38, 0xAA, 0xF3, 0xEC, 0x76, 0xA7, 0x1B, 0x54, 0xD7, 0xF4, 0x1D, 0xD7, 0xF9,
    0xCF, 0x0E, 0xBF, 0x4F,
    /* CMACGenARIA192 COUNT =40 (length = 190, offset = 289)*/
    0x9F, 0xCB, 0x51, 0x4D, 0x9E, 0x0D, 0x09, 0xEB, 0xB6, 0x33, 0x8E, 0xB4, 0x8B, 0x60, 0x48, 0x07,
    0x55, 0xF7, 0xDB, 0x82, 0x89, 0xB1, 0xF1, 0xBA, 0xB6, 0xED, 0x39, 0x2F, 0xAC, 0x47, 0xB0, 0x7F,
    0xE4, 0x6B, 0x1F, 0xD9, 0x1C, 0x2C, 0xE4, 0xB5, 0x42, 0xD0, 0xF6, 0xC2, 0x93, 0xAE, 0x24, 0x20,
    0x04, 0xFC, 0x9D, 0xD0, 0xE1, 0xE2, 0xB5, 0x69, 0xB4, 0xD2, 0xE6, 0x0C, 0x68, 0x1C, 0x15, 0x99,
    0xAE, 0xBD, 0x17, 0x23, 0xA0, 0x78, 0x74, 0xA2, 0xA4, 0x29, 0x6B, 0xE8, 0x94, 0x55, 0x36, 0xD7,
    0x1C, 0x05, 0x8F, 0xD0, 0x63, 0xD5, 0x72, 0x6E, 0xED, 0x48, 0x25, 0x75, 0xC0, 0x5E, 0x76, 0x05,
    0xC5, 0x69, 0x44, 0x14, 0x72, 0x1C, 0x40, 0x1B, 0xA6, 0xE5, 0xF5, 0x0E, 0xD4, 0x7D, 0x06, 0x92,
    0x64, 0xBD, 0xB7, 0x6B, 0x56, 0xB3, 0xAF, 0x33, 0x29, 0xF5, 0xFD, 0x52, 0xFB, 0x35, 0x58, 0x2A,
    0xF1, 0x16, 0xAB, 0x92, 0xD9, 0x3E, 0xD1, 0x86, 0x0E, 0xAC, 0x9C, 0x1C, 0x9C, 0x4C, 0x1D, 0xBA,
    0xA5, 0xC9, 0x1F, 0x87, 0x04, 0xA3, 0xF5, 0x1F, 0x2F, 0x81, 0x76, 0x8B, 0x62, 0xC8, 0x45, 0x70,
    0xFA, 0x6C, 0x56, 0x86, 0x1F, 0x07, 0xAD, 0x4C, 0xA5, 0x28, 0x69, 0xFB, 0x34, 0xED, 0x02, 0xB8,
    0xA7, 0xD4, 0xCF, 0x0D, 0xB3, 0xCE, 0xCB, 0x9A, 0xC9, 0x95, 0x98, 0x08, 0x3D, 0x40,
    /* CMACGenARIA256 COUNT =47 (length = 0, offset = 479) */
    /* CMACGenARIA256 COUNT =50 (length = 46, offset = 479)*/
    0xC1, 0xCC, 0xC4, 0x83, 0x83, 0x3C, 0x91, 0x04, 0x84, 0xEB, 0xB8, 0x3B, 0x23, 0x84, 0xC2, 0xD5,
    0x64, 0xD4, 0xDF, 0x9C, 0xFB, 0x89, 0x90, 0x56, 0xA6, 0x3A, 0xA3, 0xEA, 0xC0, 0xA8, 0x64, 0xC5,
    0x14, 0x44, 0x12, 0x7C, 0xEF, 0xF1, 0x90, 0xB3, 0x79, 0x0A, 0x5D, 0x3D, 0x44, 0xA7,
    /* CMACGenARIA256 COUNT =56 (length = 88, offset = 525)*/
    0xF6, 0x1E, 0xE1, 0x54, 0x75, 0x98, 0xCC, 0xCB, 0xAB, 0x5E, 0x7E, 0xE4, 0x15, 0x4B, 0x7A, 0xE9,
    0x5D, 0xAE, 0xE0, 0x46, 0xD0, 0xF0, 0x87, 0x4B, 0x12, 0x77, 0x4D, 0x2E, 0x10, 0xC9, 0xAB, 0x4F,
    0x37, 0xF5, 0x8D, 0xC0, 0xE4, 0x2F, 0x60, 0x05, 0x1B, 0xBA, 0x6C, 0x18, 0x7B, 0x0B, 0x64, 0xB1,
    0x71, 0x2B, 0x3B, 0xC3, 0xAD, 0x5D, 0x3A, 0x4A, 0xD2, 0x01, 0x50, 0x44, 0x72, 0xB8, 0xA9, 0xFD,
    0x39, 0xC9, 0x81, 0x90, 0x68, 0xC3, 0x3B, 0xAB, 0x85, 0x64, 0xAD, 0x92, 0x51, 0xBA, 0xBF, 0x67,
    0xFB, 0x88, 0x34, 0xA8, 0x93, 0xEB, 0xC7, 0xF9,
    /* CMACGenARIA256 COUNT =65 (length = 162, offset = 613)*/
    0x56, 0x6E, 0x67, 0xEA, 0x8A, 0xE3, 0x1A, 0xE2, 0xAF, 0x8A, 0x25, 0xFE, 0xE3, 0x96, 0x42, 0x24,
    0x42, 0x49, 0xFC, 0xBB, 0x65, 0x65, 0x5C, 0xC0, 0x28, 0xE4, 0xC5, 0x16, 0xB1, 0xE8, 0x1D, 0x1E,
    0xAE, 0xF5, 0x2D, 0x88, 0xDD, 0x5E, 0x4B, 0x69, 0xD2, 0xFD, 0x36, 0x8D, 0xF5, 0x27, 0x41, 0x0E,
    0xCA, 0xDF, 0xB4, 0x25, 0x75, 0x4A, 0x6F, 0x41, 0x7D, 0x64, 0xD0, 0x56, 0xD0, 0xF0, 0xD7, 0x78,
    0x07, 0xB5, 0x88, 0xA8, 0xEC, 0xE7, 0x91, 0xEE, 0x39, 0xE4, 0x2D, 0xA8, 0xA3, 0x21, 0x48, 0x21,
    0x17, 0x63, 0xE4, 0x65, 0x44, 0x32, 0xBB, 0x54, 0x59, 0x8A, 0x26, 0xF6, 0x10, 0xD5, 0x3D, 0x0E,
    0xEB, 0x16, 0x40, 0xF2, 0xBD, 0x67, 0x36, 0x99, 0x6D, 0xA5, 0xD3, 0xF6, 0xF7, 0x6A, 0x9F, 0x84,
    0xB3, 0x3B, 0x55, 0x23, 0xDA, 0x03, 0x89, 0x22, 0x46, 0xC0, 0x8E, 0x9D, 0x79, 0x7D, 0x97, 0x09,
    0xE1, 0x80, 0x1C, 0x0D, 0x5B, 0xC4, 0x80, 0x93, 0xF5, 0xAA, 0xF0, 0x20, 0xF8, 0xEB, 0x8E, 0x60,
    0x26, 0xD1, 0xCF, 0x06, 0x40, 0xA7, 0x21, 0xD3, 0xCB, 0x6E, 0xD2, 0xF3, 0x14, 0xD1, 0x2D, 0x90,
    0x72, 0x5B,
    /* CMACGenARIA128 COUNT =70 (length = 0, offset = 775) */
    /* CMACGenARIA128 COUNT =75 (length = 112, offset = 775) */
    0x8F, 0x53, 0x99, 0x04, 0x3D, 0x90, 0x4B, 0xEF, 0xB9, 0xAF, 0x7F, 0xE7, 0x59, 0x78, 0x67, 0xA3,
    0x09, 0xFA, 0x6C, 0x37, 0x9E, 0x87, 0x23, 0x39, 0x67, 0xFF, 0x7D, 0x22, 0x05, 0x52, 0x3E, 0xF8,
    0x7D, 0x40, 0xB0, 0x2A, 0x8C, 0x63, 0x98, 0xBC, 0x3B, 0x70, 0x69, 0xA3, 0xB2, 0xBA, 0xB8, 0x73,
    0x51, 0xF7, 0xF0, 0x37, 0xFD, 0x47, 0x47, 0xA0, 0xB8, 0x72, 0xF0, 0x63, 0x75, 0x71, 0x92, 0xDC,
    0x2A, 0x31, 0xFA, 0xF7, 0x26, 0x93, 0x0D, 0x50, 0xA5, 0xB7, 0x00, 0x99, 0xA2, 0x7A, 0xCB, 0x3D,
    0xEC, 0x3D, 0xDA, 0x42, 0x7C, 0xE9, 0x06, 0x74, 0x06, 0x2F, 0xC5, 0xC0, 0xCE, 0x15, 0x9D, 0xDE,
    0xBD, 0xAE, 0xDF, 0x33, 0xB4, 0x28, 0x91, 0xF5, 0x20, 0x0D, 0xAC, 0x90, 0xCF, 0xC2, 0x88, 0x48,
    /* CMACGenARIA128 COUNT =80 (length = 66, offset = 887) */
    0xB6, 0xA2, 0x68, 0x6A, 0x97, 0xE7, 0x22, 0xDD, 0x99, 0x10, 0x56, 0x73, 0x7D, 0xAE, 0xDF, 0xED,
    0x15, 0x17, 0x4E, 0xE5, 0x9F, 0x66, 0xDE, 0xF9, 0x51, 0xC3, 0xAB, 0x2B, 0x35, 0xE5, 0x1E, 0x88,
    0x37, 0xD4, 0xAC, 0x08, 0x7D, 0xF4, 0xC0, 0xB6, 0xF6, 0x3F, 0xF5, 0x10, 0x36, 0xD2, 0x89, 0xB1,
    0x81, 0xAB, 0x0F, 0x2D, 0x25, 0xB1, 0x63, 0x3D, 0x0D, 0xF4, 0xE3, 0x1C, 0x94, 0x37, 0xDC, 0x30,
    0x97, 0xAC,
    /* CMACGenARIA192 COUNT =70 (length = 0, offset = 953) */
    /* CMACGenARIA192 COUNT =75 (length = 112, offset = 953) */
    0xEB, 0x8B, 0x0F, 0x4A, 0x74, 0xCF, 0xE4, 0x23, 0x1D, 0x7F, 0x1C, 0xE7, 0xD1, 0xA2, 0x2F, 0x7A,
    0xF4, 0xE3, 0xAC, 0x7F, 0xB0, 0x95, 0x2E, 0x78, 0x89, 0xF4, 0x8B, 0x2F, 0x38, 0xAE, 0x4E, 0x56,
    0xB6, 0xA2, 0x68, 0x6A, 0x97, 0xE7, 0x22, 0xDD, 0x99, 0x10, 0x56, 0x73, 0x7D, 0xAE, 0xDF, 0xED,
    0x15, 0x17, 0x4E, 0xE5, 0x9F, 0x66, 0xDE, 0xF9, 0x51, 0xC3, 0xAB, 0x2B, 0x35, 0xE5, 0x1E, 0x88,
    0x37, 0xD4, 0xAC, 0x08, 0x7D, 0xF4, 0xC0, 0xB6, 0xF6, 0x3F, 0xF5, 0x10, 0x36, 0xD2, 0x89, 0xB1,
    0x81, 0xAB, 0x0F, 0x2D, 0x25, 0xB1, 0x63, 0x3D, 0x0D, 0xF4, 0xE3, 0x1C, 0x94, 0x37, 0xDC, 0x30,
    0x97, 0xAC, 0x43, 0xED, 0xCE, 0xFE, 0xA6, 0xF7, 0x5C, 0x94, 0x61, 0x86, 0xA4, 0x15, 0x15, 0x0F,
    /* CMACGenARIA192 COUNT =80 (length = 66, offset = 1065) */
    0x5A, 0xCE, 0x16, 0x45, 0xD2, 0x51, 0x10, 0x0B, 0xA2, 0xB2, 0xB8, 0xF7, 0x70, 0xD3, 0xF3, 0xAE,
    0x5C, 0x24, 0x01, 0xB9, 0xD6, 0x25, 0xF8, 0x2D, 0xB2, 0x26, 0x71, 0x8B, 0x7E, 0x51, 0x81, 0xE5,
    0x13, 0xEB, 0xEB, 0x42, 0x6A, 0xD7, 0x93, 0x44, 0xB1, 0x5C, 0x36, 0x08, 0x9E, 0x28, 0x59, 0x4E,
    0x04, 0x94, 0x7F, 0xDA, 0xA4, 0x29, 0x9F, 0x1A, 0x45, 0x63, 0xD5, 0x08, 0x07, 0xB6, 0x58, 0x52,
    0xF4, 0xCF,
    /* CMACGenARIA256 COUNT =70 (length = 0, offset = 1131) */
    /* CMACGenARIA256 COUNT =75 (length = 112, offset = 1131) */
    0x23, 0x17, 0x65, 0xB0, 0x1C, 0x1D, 0xB8, 0xBC, 0x23, 0xC8, 0xBA, 0xF2, 0x0D, 0xA8, 0x86, 0x92,
    0x6B, 0x16, 0xDC, 0xE4, 0xB1, 0xCB, 0x6B, 0x72, 0x98, 0xAB, 0x50, 0x74, 0xC0, 0xE2, 0xDA, 0x8C,
    0x9A, 0x32, 0x90, 0xB3, 0x85, 0x8B, 0x75, 0x73, 0x37, 0xDD, 0x8A, 0x58, 0x5F, 0x2A, 0x95, 0xA1,
    0x88, 0xDF, 0x21, 0x3A, 0x41, 0x64, 0xC6, 0xEF, 0x7B, 0xF2, 0xAA, 0xDA, 0x32, 0xA5, 0x48, 0x1D,
    0x50, 0xD2, 0x70, 0xD7, 0xCE, 0x9A, 0x8F, 0x50, 0x1C, 0xBF, 0x31, 0x77, 0xC3, 0xB7, 0xC4, 0x8F,
    0x49, 0xFF, 0x9F, 0x25, 0x54, 0xB2, 0x43, 0x45, 0x13, 0x5A, 0xE0, 0xED, 0xDA, 0x06, 0x1B, 0xC2,
    0x0C, 0x9D, 0x0E, 0x02, 0x3E, 0x71, 0x90, 0xBA, 0x99, 0x17, 0xB7, 0x39, 0x80, 0x77, 0x9C, 0xC4,
    /* CMACGenARIA256 COUNT =80 (length = 66, offset = 1243) */
    0xFA, 0xF6, 0x9C, 0x88, 0x1C, 0x86, 0x48, 0xAF, 0x67, 0xA8, 0x18, 0xDE, 0x39, 0x52, 0xF8, 0xE2,
    0x7D, 0xD3, 0xCC, 0xA2, 0xD0, 0x67, 0xB1, 0x52, 0xE1, 0x09, 0x85, 0xEA, 0x1E, 0x88, 0xA9, 0xD7,
    0x8D, 0xDD, 0x05, 0xF0, 0xF7, 0xBC, 0x4D, 0xAB, 0x4C, 0x69, 0x83, 0x91, 0x43, 0xCD, 0xBE, 0xEF,
    0x41, 0xC9, 0x29, 0xCF, 0x7C, 0xCA, 0x4E, 0x26, 0x60, 0x9E, 0x95, 0x4F, 0x32, 0x86, 0x07, 0xB8,
    0xF3, 0x8C
};

/* Expected macs for the test vectors. */
static const uint8_t test_vector_macs[] =
{
    /* CMACGenARIA128 COUNT =0 */
    0x26, 0x9F, 0xDE, 0x39, 0xAA, 0x08, 0xF2, 0x28,
    /* CMACGenARIA128 COUNT =5 */
    0xAE, 0xDD, 0xF7, 0x41, 0x69, 0x60, 0x05, 0x1F,
    /* CMACGenARIA128 COUNT =10 */
    0x11, 0x4F, 0x97, 0xD2, 0x12, 0xB0, 0x25, 0x4F, 0x7E, 0x28,
    /* CMACGenARIA128 COUNT =15 */
    0xEA, 0x3C, 0xA6, 0x2E, 0x02, 0xA3, 0x4B, 0x33, 0x2F, 0xB3,
    /* CMACGenARIA192 COUNT =20 */
    0xF6, 0x5E, 0x4F, 0xD4, 0xCA, 0x06, 0x08, 0xA4, 0x36, 0x4D, 0x52,
    /* CMACGenARIA192 COUNT =25 */
    0x4A, 0x4F, 0xE1, 0x25, 0x17, 0x0B, 0x6D, 0x6C, 0x06, 0x77, 0x20,
    /* CMACGenARIA192 COUNT =30 */
    0xF2, 0x4B, 0x64, 0x59, 0x44, 0x7C, 0xE9, 0xE5, 0x89, 0x10, 0xFC, 0x55,
    /* CMACGenARIA192 COUNT =40 */
    0x4E, 0xCF, 0x4B, 0x59, 0x23, 0xBE, 0xD3, 0x61, 0xE1, 0x15, 0x68, 0x0E, 0x65,
    /* CMACGenARIA256 COUNT =47 */
    0xA4, 0x59, 0xD7, 0xDD, 0x67, 0x74, 0x57, 0x06, 0x95, 0x3D, 0x9A, 0xF0, 0x9F,
    /* CMACGenARIA256 COUNT =50 */
    0xAF, 0x5D, 0xA4, 0x56, 0x8D, 0x41, 0x0A, 0xB8, 0x27, 0xC4, 0x3C, 0xA0, 0x05, 0x60,
    /* CMACGenARIA256 COUNT =56 */
    0x55, 0xEB, 0x5E, 0x26, 0x19, 0xA1, 0x4A, 0xA9, 0xD3, 0x9D, 0x86, 0x82, 0x3E, 0x2F,
    /* CMACGenARIA256 COUNT =65 */
    0xF1, 0xB2, 0xEA, 0x32, 0x0C, 0x99, 0x43, 0xCA, 0xF7, 0xE5, 0x95, 0x0A, 0xBE, 0x05, 0xB8,
    /* CMACGenARIA128 COUNT =70 */
    0xA8, 0x1C, 0xF5, 0x26, 0x89, 0xCF, 0x6E, 0x95, 0xF4, 0x72, 0x6F, 0x62, 0x21, 0x66, 0xB0, 0xB7,
    /* CMACGenARIA128 COUNT =75 */
    0xF8, 0x3A, 0x93, 0xBA, 0xA2, 0xCB, 0xC0, 0x70, 0xBA, 0xDB, 0x8F, 0x4F, 0x05, 0x4F, 0x6E, 0x0E,
    /* CMACGenARIA128 COUNT =80 */
    0x08, 0x47, 0xBB, 0x3F, 0xBC, 0x50, 0x17, 0x46, 0xD6, 0xAC, 0x9C, 0x08, 0x17, 0x5A, 0x38, 0x09,
    /* CMACGenARIA192 COUNT =70 */
    0x52, 0x2D, 0x3A, 0xB8, 0x79, 0x93, 0x61, 0x49, 0x6C, 0x60, 0x65, 0x2B, 0xAC, 0x33, 0x23, 0x30,
    /* CMACGenARIA192 COUNT =75 */
    0xF9, 0x93, 0x70, 0x34, 0x3C, 0x03, 0x99, 0x15, 0x6F, 0x36, 0x2C, 0x0A, 0x2F, 0xF2, 0x62, 0xC7,
    /* CMACGenARIA192 COUNT =80 */
    0x27, 0x5B, 0x43, 0x0C, 0x82, 0xC6, 0x33, 0xDC, 0x95, 0x3B, 0x7B, 0xE9, 0xC3, 0xBD, 0x8A, 0x80,
    /* CMACGenARIA256 COUNT =70 */
    0x72, 0x63, 0xD2, 0x77, 0x53, 0xFC, 0xDD, 0xF4, 0x85, 0x13, 0xF2, 0x9E, 0x23, 0x8A, 0xBB, 0x8A,
    /* CMACGenARIA256 COUNT =75 */
    0x5B, 0x7C, 0x06, 0x21, 0xFF, 0x01, 0x70, 0x79, 0x11, 0x0D, 0xE6, 0x79, 0x40, 0x5B, 0xAD, 0x22,
    /* CMACGenARIA256 COUNT =80 */
    0x2B, 0x96, 0xA9, 0xF0, 0x58, 0x71, 0x10, 0xC8, 0x48, 0xD2, 0x4C, 0x17, 0xEC, 0x8B, 0xB1, 0x49
};


/* end of file testvectors_aria_cmac_data.h */
