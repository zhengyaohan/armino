/* testvectors_dhdsa_domain.h
 *
 * Description: Structure for the DH and DSA domain parameters.
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

#ifndef INCLUDE_GUARD_TESTVECTORS_DHDSA_DOMAIN_H
#define INCLUDE_GUARD_TESTVECTORS_DHDSA_DOMAIN_H

#include "basic_defs.h"

/* Structures for the ECC curves. */
typedef struct
{
    uint32_t LBits;
    uint32_t NBits;
    const uint8_t * P_p;
    uint32_t PLen;
    const uint8_t * Q_p;
    uint32_t QLen;
    const uint8_t * G_p;
    uint32_t GLen;
} TestVector_DhDsaDomain_Rec_t;

typedef const TestVector_DhDsaDomain_Rec_t * const TestVector_DhDsaDomain_t;

extern TestVector_DhDsaDomain_t DhDsaDomain_L1024N160_p;
extern TestVector_DhDsaDomain_t DhDsaDomain_L2048N224_p;
extern TestVector_DhDsaDomain_t DhDsaDomain_L2048N256_p;
extern TestVector_DhDsaDomain_t DhDsaDomain_L3072N256_p;


#endif /* Include guard */

/* end of file testvectors_dhdsa_domain.h */
