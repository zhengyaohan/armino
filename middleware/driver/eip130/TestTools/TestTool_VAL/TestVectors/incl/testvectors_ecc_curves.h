/* testvectors_ecc_curves.h
 *
 * Description: Structure for the ECC curves.
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

#ifndef INCLUDE_GUARD_TESTVECTORS_ECC_CURVES_H
#define INCLUDE_GUARD_TESTVECTORS_ECC_CURVES_H

#include "basic_defs.h"

/* Structures for the ECC curves. */
typedef struct
{
    uint32_t CurveBits;
    const uint8_t * P_p;
    uint32_t PLen;
    const uint8_t * A_p;
    uint32_t ALen;
    const uint8_t * B_p;
    uint32_t BLen;
    const uint8_t * ECPointX_p;
    uint32_t ECPointXLen;
    const uint8_t * ECPointY_p;
    uint32_t ECPointYLen;
    const uint8_t * Order_p;
    uint32_t OrderLen;
    const uint8_t Cofactor;
} TestVector_ECC_Curve_Rec_t;

typedef const TestVector_ECC_Curve_Rec_t * const TestVector_ECC_Curve_t;


extern const TestVector_ECC_Curve_Rec_t ECurve_SEC_P128r1;
extern const TestVector_ECC_Curve_Rec_t ECurve_SEC_P128r2;
extern const TestVector_ECC_Curve_Rec_t ECurve_SEC_P160r1;
extern const TestVector_ECC_Curve_Rec_t ECurve_NIST_P192;
extern const TestVector_ECC_Curve_Rec_t ECurve_NIST_P224;
extern const TestVector_ECC_Curve_Rec_t ECurve_NIST_P256;
extern const TestVector_ECC_Curve_Rec_t ECurve_NIST_P384;
extern const TestVector_ECC_Curve_Rec_t ECurve_NIST_P521;
extern const TestVector_ECC_Curve_Rec_t ECurve_25519;
extern const TestVector_ECC_Curve_Rec_t ECurve_Ed25519;

extern TestVector_ECC_Curve_t ECC_Curve_SEC_P128r1;
extern TestVector_ECC_Curve_t ECC_Curve_SEC_P128r2;
extern TestVector_ECC_Curve_t ECC_Curve_SEC_P160r1;
extern TestVector_ECC_Curve_t ECC_Curve_NIST_P192;
extern TestVector_ECC_Curve_t ECC_Curve_NIST_P224;
extern TestVector_ECC_Curve_t ECC_Curve_NIST_P256;
extern TestVector_ECC_Curve_t ECC_Curve_NIST_P384;
extern TestVector_ECC_Curve_t ECC_Curve_NIST_P521;
extern TestVector_ECC_Curve_t ECC_Curve_25519;
extern TestVector_ECC_Curve_t ECC_Curve_Ed25519;


#endif /* Include guard */

/* end of file testvectors_ecc_curves.h */
