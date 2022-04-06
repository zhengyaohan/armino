/* aessiv.h
 *
 * AES-SIV implementation using the VAL API.
 */

/*****************************************************************************
* Copyright (c) 2014-2019 INSIDE Secure B.V. All Rights Reserved.
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

#ifndef INCLUDE_GUARD_AESSIV_H
#define INCLUDE_GUARD_AESSIV_H

/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */
#include "basic_defs.h"
#include "clib.h"


typedef struct
{
    uint8_t Key[2*(256/8)];             // AES_CMAC [256/8] + AES-CTR [256/8] key
    unsigned int KeySize;               // Key size in bytes
    uint8_t AD_Buffer[4096];            // Common buffer associated Data (labels)
    uint8_t * AD_List[9];               // Associated Data (label) array
    unsigned int AD_ListCount;          // Number of labels in the AD_List
    bool Verbose;                       // Verbose setting
} AESSIV_Context;


int
AESSIV_Init(
        AESSIV_Context * Context_p,
        const bool Verbose);

int
AESSIV_SetKey(
        AESSIV_Context * Context_p,
        const uint8_t * Key_p,
        const size_t KeySize);

int
AESSIV_SetAD(
        AESSIV_Context * Context_p,
        const uint8_t * AD_p,
        const size_t ADSize);

int
AESSIV_Encrypt(
        AESSIV_Context * Context_p,
        const uint8_t * InData_p,
        const size_t InDataSize,
        uint8_t * OutData_p,
        size_t * OutDataSize_p);


#endif /* INCLUDE_GUARD_AESSIV_H */

/* end of file aessiv.h */

