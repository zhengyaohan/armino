/* adapter_val_internal.c
 *
 * Implementation of the VAL API.
 *
 * This file implements interval functions.
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

#include "c_adapter_val.h"              // configuration

#include "basic_defs.h"
#include "clib.h"
#include "log.h"

#include "adapter_val_internal.h"       // val_ExchangeToken()


/*----------------------------------------------------------------------------
 * valInternal_ReverseMemCpy
 */
void *
valInternal_ReverseMemCpy(
        void * Dest_p,
        const void * Src_p,
        size_t Size)
{
    uint8_t * dp = Dest_p;
    const uint8_t * sp = Src_p;

#ifdef VAL_STRICT_ARGS
    if (Size == 0)
    {
        LOG_WARN("Warning: size == 0\n");
        return Dest_p;
    }
#endif

    sp += (Size - 1);
    while (Size--)
    {
        *dp++ = *sp--;
    }
    return Dest_p;
}


/* end of file adapter_val_internal.c */
