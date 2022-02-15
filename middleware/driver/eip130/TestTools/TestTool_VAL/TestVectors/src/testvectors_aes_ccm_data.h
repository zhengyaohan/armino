/* testvectors_aes_ccm_data.h
 *
 * Description: Test vectors for AES CCM, test vector contents.
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

/*
#  CAVS 16.3
#  "CCM-VADT" information for "532-04-InsideSecure"
#  AES Keylen: 256
# Generated on Fri Jan  2 02:41:28 1970

Plen = 32
Nlen = 13
Tlen = 16
*/

/*
[Alen = 0]
Key = b7c7ecb134e516e8b342b52ebcd53158a23419ca0908c68da6be3ba246805096
Nonce = 0535d8852ed3f4951bfdee85ed
Count = 0
Adata = 00
Payload = 03ef7e463f524e2ec54a0c43d95942b96df69a649833f07b439dab9d26644e11
CT = 775e4e1373fe0644c1348bb40b19dff283e4a805df82970cb5c79d478e9c4b76a7c1153193f856cd7b29c1ea2796dc7a
*/
static const uint8_t V256_1_Key[] =
{
    0xB7, 0xC7, 0xEC, 0xB1, 0x34, 0xE5, 0x16, 0xE8, 0xB3, 0x42, 0xB5, 0x2E, 0xBC, 0xD5, 0x31, 0x58,
    0xA2, 0x34, 0x19, 0xCA, 0x09, 0x08, 0xC6, 0x8D, 0xA6, 0xBE, 0x3B, 0xA2, 0x46, 0x80, 0x50, 0x96,
};
static const uint8_t V256_1_Nonce[] =
{
    0x05, 0x35, 0xD8, 0x85, 0x2E, 0xD3, 0xF4, 0x95, 0x1B, 0xFD, 0xEE, 0x85, 0xED,
};
static const uint8_t V256_1_Adata[] = {};
static const uint8_t V256_1_Payload[] =
{
    0x03, 0xEF, 0x7E, 0x46, 0x3F, 0x52, 0x4E, 0x2E, 0xC5, 0x4A, 0x0C, 0x43, 0xD9, 0x59, 0x42, 0xB9,
    0x6D, 0xF6, 0x9A, 0x64, 0x98, 0x33, 0xF0, 0x7B, 0x43, 0x9D, 0xAB, 0x9D, 0x26, 0x64, 0x4E, 0x11,
};
static const uint8_t V256_1_CT[] =
{
    0x77, 0x5E, 0x4E, 0x13, 0x73, 0xFE, 0x06, 0x44, 0xC1, 0x34, 0x8B, 0xB4, 0x0B, 0x19, 0xDF, 0xF2,
    0x83, 0xE4, 0xA8, 0x05, 0xDF, 0x82, 0x97, 0x0C, 0xB5, 0xC7, 0x9D, 0x47, 0x8E, 0x9C, 0x4B, 0x76,
    0xA7, 0xC1, 0x15, 0x31, 0x93, 0xF8, 0x56, 0xCD, 0x7B, 0x29, 0xC1, 0xEA, 0x27, 0x96, 0xDC, 0x7A,
};

/*
[Alen = 6]
Key = 434f617a771622349039b5c923c4844b0cf7609abe4a7285d7ae7432f81621c0
Nonce = 38f2d3718a55e1e58e62ade98c
Count = 60
Adata = e9c0f7cad84b
Payload = 9246650a009acbeb66939d7bdcf952ea288d7621de9cf1f5ccd38ca19455b173
CT = 150dfe59db06ae79c10bc8053806d60b7095ac3a4ac18bd4f3769e198a8c95bdfc717fdc3f6bdb6d13f30cb768e648de
*/
static const uint8_t V256_2_Key[] =
{
    0x43, 0x4F, 0x61, 0x7A, 0x77, 0x16, 0x22, 0x34, 0x90, 0x39, 0xB5, 0xC9, 0x23, 0xC4, 0x84, 0x4B,
    0x0C, 0xF7, 0x60, 0x9A, 0xBE, 0x4A, 0x72, 0x85, 0xD7, 0xAE, 0x74, 0x32, 0xF8, 0x16, 0x21, 0xC0,
};
static const uint8_t V256_2_Nonce[] =
{
    0x38, 0xF2, 0xD3, 0x71, 0x8A, 0x55, 0xE1, 0xE5, 0x8E, 0x62, 0xAD, 0xE9, 0x8C,
};
static const uint8_t V256_2_Adata[] =
{
    0xE9, 0xC0, 0xF7, 0xCA, 0xD8, 0x4B,
};
static const uint8_t V256_2_Payload[] =
{
    0x92, 0x46, 0x65, 0x0A, 0x00, 0x9A, 0xCB, 0xEB, 0x66, 0x93, 0x9D, 0x7B, 0xDC, 0xF9, 0x52, 0xEA,
    0x28, 0x8D, 0x76, 0x21, 0xDE, 0x9C, 0xF1, 0xF5, 0xCC, 0xD3, 0x8C, 0xA1, 0x94, 0x55, 0xB1, 0x73,
};
static const uint8_t V256_2_CT[] =
{
    0x15, 0x0D, 0xFE, 0x59, 0xDB, 0x06, 0xAE, 0x79, 0xC1, 0x0B, 0xC8, 0x05, 0x38, 0x06, 0xD6, 0x0B,
    0x70, 0x95, 0xAC, 0x3A, 0x4A, 0xC1, 0x8B, 0xD4, 0xF3, 0x76, 0x9E, 0x19, 0x8A, 0x8C, 0x95, 0xBD,
    0xFC, 0x71, 0x7F, 0xDC, 0x3F, 0x6B, 0xDB, 0x6D, 0x13, 0xF3, 0x0C, 0xB7, 0x68, 0xE6, 0x48, 0xDE,
};

/*
[Alen = 17]
Key = 60a2666e890fa94954e0e5c87fa7f5a9e0b86c89b12f85052c345920587d55e4
Nonce = 611082dc33aea9a461cd8d72c6
Count = 170
Adata = 344e11033090e9ddb40aa66428d5100cbc
Payload = 1c8d60221d994c7428b7779bf3d0fdd90d4b77f07b23c1daa43c01b96af7e411
CT = a7befb376115bc1408dde12155f4559537a02162a25ea3cb99b64c59af618dab753a8f0ecee69af71635d91f1aa147bd
*/
static const uint8_t V256_3_Key[] =
{
    0x60, 0xA2, 0x66, 0x6E, 0x89, 0x0F, 0xA9, 0x49, 0x54, 0xE0, 0xE5, 0xC8, 0x7F, 0xA7, 0xF5, 0xA9,
    0xE0, 0xB8, 0x6C, 0x89, 0xB1, 0x2F, 0x85, 0x05, 0x2C, 0x34, 0x59, 0x20, 0x58, 0x7D, 0x55, 0xE4,
};
static const uint8_t V256_3_Nonce[] =
{
    0x61, 0x10, 0x82, 0xDC, 0x33, 0xAE, 0xA9, 0xA4, 0x61, 0xCD, 0x8D, 0x72, 0xC6,
};
static const uint8_t V256_3_Adata[] =
{
    0x34, 0x4E, 0x11, 0x03, 0x30, 0x90, 0xE9, 0xDD, 0xB4, 0x0A, 0xA6, 0x64, 0x28, 0xD5, 0x10, 0x0C,
    0xBC,
};
static const uint8_t V256_3_Payload[] =
{
    0x1C, 0x8D, 0x60, 0x22, 0x1D, 0x99, 0x4C, 0x74, 0x28, 0xB7, 0x77, 0x9B, 0xF3, 0xD0, 0xFD, 0xD9,
    0x0D, 0x4B, 0x77, 0xF0, 0x7B, 0x23, 0xC1, 0xDA, 0xA4, 0x3C, 0x01, 0xB9, 0x6A, 0xF7, 0xE4, 0x11,
};
static const uint8_t V256_3_CT[] =
{
    0xA7, 0xBE, 0xFB, 0x37, 0x61, 0x15, 0xBC, 0x14, 0x08, 0xDD, 0xE1, 0x21, 0x55, 0xF4, 0x55, 0x95,
    0x37, 0xA0, 0x21, 0x62, 0xA2, 0x5E, 0xA3, 0xCB, 0x99, 0xB6, 0x4C, 0x59, 0xAF, 0x61, 0x8D, 0xAB,
    0x75, 0x3A, 0x8F, 0x0E, 0xCE, 0xE6, 0x9A, 0xF7, 0x16, 0x35, 0xD9, 0x1F, 0x1A, 0xA1, 0x47, 0xBD,
};

/*
[Alen = 31]
Key = 4c5b076555d9064565dab0823af4d10b1b27006e681bce4075a944bf989d8db6
Nonce = ba07666834c5e20a48e0d1c8b3
Count = 310
Adata = ad75e6a6e0b2ec96432ec46de8091de8238f215d875f04fc10ccdc55a283b3
Payload = bf5af50c7dbb411cb4260fc3cbf5a53ee358bd731592e2c9fa651d3d71cad1b3
CT = c3c8a8d77756d543c91b8775d080aefd143ce18a6b376ac1dda3357c4b388dec74c9f15cf9d7539d2f9459e3b5c5afe1
*/
static const uint8_t V256_4_Key[] =
{
    0x4C, 0x5B, 0x07, 0x65, 0x55, 0xD9, 0x06, 0x45, 0x65, 0xDA, 0xB0, 0x82, 0x3A, 0xF4, 0xD1, 0x0B,
    0x1B, 0x27, 0x00, 0x6E, 0x68, 0x1B, 0xCE, 0x40, 0x75, 0xA9, 0x44, 0xBF, 0x98, 0x9D, 0x8D, 0xB6,
};
static const uint8_t V256_4_Nonce[] =
{
    0xBA, 0x07, 0x66, 0x68, 0x34, 0xC5, 0xE2, 0x0A, 0x48, 0xE0, 0xD1, 0xC8, 0xB3,
};
static const uint8_t V256_4_Adata[] =
{
    0xAD, 0x75, 0xE6, 0xA6, 0xE0, 0xB2, 0xEC, 0x96, 0x43, 0x2E, 0xC4, 0x6D, 0xE8, 0x09, 0x1D, 0xE8,
    0x23, 0x8F, 0x21, 0x5D, 0x87, 0x5F, 0x04, 0xFC, 0x10, 0xCC, 0xDC, 0x55, 0xA2, 0x83, 0xB3,
};
static const uint8_t V256_4_Payload[] =
{
    0xBF, 0x5A, 0xF5, 0x0C, 0x7D, 0xBB, 0x41, 0x1C, 0xB4, 0x26, 0x0F, 0xC3, 0xCB, 0xF5, 0xA5, 0x3E,
    0xE3, 0x58, 0xBD, 0x73, 0x15, 0x92, 0xE2, 0xC9, 0xFA, 0x65, 0x1D, 0x3D, 0x71, 0xCA, 0xD1, 0xB3,
};
static const uint8_t V256_4_CT[] =
{
    0xC3, 0xC8, 0xA8, 0xD7, 0x77, 0x56, 0xD5, 0x43, 0xC9, 0x1B, 0x87, 0x75, 0xD0, 0x80, 0xAE, 0xFD,
    0x14, 0x3C, 0xE1, 0x8A, 0x6B, 0x37, 0x6A, 0xC1, 0xDD, 0xA3, 0x35, 0x7C, 0x4B, 0x38, 0x8D, 0xEC,
    0x74, 0xC9, 0xF1, 0x5C, 0xF9, 0xD7, 0x53, 0x9D, 0x2F, 0x94, 0x59, 0xE3, 0xB5, 0xC5, 0xAF, 0xE1,
};

/*
[Alen = 32]
Key = 21adac74c023980f14f5f7b6184338ab50949db9ad233e26b17a52e4d342aa07
Nonce = 0fcb90425ee2801926e7999698
Count = 320
Adata = e97b52d90d4d6a2a91983fc8a0f1e30f73ba018bbbf366683f53c02ac697a69f
Payload = fb1eb07c40709960f858f072bb6020416e2c561ab71590ceb313f7b5ece06ef3
CT = 655ee526617e5d5a2a8f16a3c5b517775b2131cb9f725b6fc0e68a0252086dfb692c6f7227239a4b9a9ad36759cbf37f
*/
static const uint8_t V256_5_Key[] =
{
    0x21, 0xAD, 0xAC, 0x74, 0xC0, 0x23, 0x98, 0x0F, 0x14, 0xF5, 0xF7, 0xB6, 0x18, 0x43, 0x38, 0xAB,
    0x50, 0x94, 0x9D, 0xB9, 0xAD, 0x23, 0x3E, 0x26, 0xB1, 0x7A, 0x52, 0xE4, 0xD3, 0x42, 0xAA, 0x07,
};
static const uint8_t V256_5_Nonce[] =
{
    0x0F, 0xCB, 0x90, 0x42, 0x5E, 0xE2, 0x80, 0x19, 0x26, 0xE7, 0x99, 0x96, 0x98,
};
static const uint8_t V256_5_Adata[] =
{
    0xE9, 0x7B, 0x52, 0xD9, 0x0D, 0x4D, 0x6A, 0x2A, 0x91, 0x98, 0x3F, 0xC8, 0xA0, 0xF1, 0xE3, 0x0F,
    0x73, 0xBA, 0x01, 0x8B, 0xBB, 0xF3, 0x66, 0x68, 0x3F, 0x53, 0xC0, 0x2A, 0xC6, 0x97, 0xA6, 0x9F,
};
static const uint8_t V256_5_Payload[] =
{
    0xFB, 0x1E, 0xB0, 0x7C, 0x40, 0x70, 0x99, 0x60, 0xF8, 0x58, 0xF0, 0x72, 0xBB, 0x60, 0x20, 0x41,
    0x6E, 0x2C, 0x56, 0x1A, 0xB7, 0x15, 0x90, 0xCE, 0xB3, 0x13, 0xF7, 0xB5, 0xEC, 0xE0, 0x6E, 0xF3,
};
static const uint8_t V256_5_CT[] =
{
    0x65, 0x5E, 0xE5, 0x26, 0x61, 0x7E, 0x5D, 0x5A, 0x2A, 0x8F, 0x16, 0xA3, 0xC5, 0xB5, 0x17, 0x77,
    0x5B, 0x21, 0x31, 0xCB, 0x9F, 0x72, 0x5B, 0x6F, 0xC0, 0xE6, 0x8A, 0x02, 0x52, 0x08, 0x6D, 0xFB,
    0x69, 0x2C, 0x6F, 0x72, 0x27, 0x23, 0x9A, 0x4B, 0x9A, 0x9A, 0xD3, 0x67, 0x59, 0xCB, 0xF3, 0x7F,
};


/* end of file testvectors_aes_ccm_data.h */
