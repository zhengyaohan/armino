/* adapter_vex_bufmanager.c
 *
 * Implementation of the VEX API.
 *
 * This file implements the buffer manager related functionality.
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

#include "c_adapter_vex.h"          // configuration

#include "basic_defs.h"
#include "clib.h"
#include "log.h"

#include "adapter_vex_internal.h"   // API implementation
#include "adapter_bufmanager.h"     // BufManager_*()
#include "adapter_sleep.h"          // Adapter_SleepMS()

#define VEX_DMA_TOKEN_ID_SIZE   sizeof(uint32_t)


/*----------------------------------------------------------------------------
 * vexLocal_CB_SizeAlignment
 */
static size_t
vexLocal_CB_SizeAlignment(
        const size_t Size)
{
#ifdef VEX_CHECK_DMA_WITH_TOKEN_ID
    // 32-bit alignment + extra size for the TokenID
    return (((Size + 3) & (size_t)~3) + VEX_DMA_TOKEN_ID_SIZE);
#else
    // 32-bit alignment
    return ((Size + 3) & (size_t)~3);
#endif
}


/*----------------------------------------------------------------------------
 * vexLocal_CB_CheckClear
 */
#ifdef VEX_CHECK_DMA_WITH_TOKEN_ID
static int
vexLocal_CB_CheckClear(
        void * Buffer_p,
        const size_t Size,
        void * UserData_p)
{
    if (UserData_p != NULL)
    {
        // Zeroize TokenID area
        uint8_t * Ptr_p = (uint8_t *)Buffer_p;
        Ptr_p += (Size - VEX_DMA_TOKEN_ID_SIZE);
        memset(Ptr_p, 0, sizeof(uint32_t));
    }
    return 0;
}
#endif


/*----------------------------------------------------------------------------
 * vexLocal_CB_CheckClear
 */
#ifdef VEX_CHECK_DMA_WITH_TOKEN_ID
static int
vexLocal_CB_CheckReady(
        void * Buffer_p,
        const size_t Size,
        void * UserData_p)
{
    if (UserData_p != NULL)
    {
        uint32_t TokenID = (uint32_t)*(uint16_t *)UserData_p;
        uint8_t * Ptr_p = (uint8_t *)Buffer_p;
        uint8_t TokenBuf[VEX_DMA_TOKEN_ID_SIZE];

        // The HW writes 32-bit words in Little Endian format, so convert
        // token ID to a byte array in Little Endian format
        memset(TokenBuf, 0, sizeof(TokenBuf));
        TokenBuf[0] = (uint8_t)(TokenID & 0x000000FFU); // LSB first
        TokenBuf[1] = (uint8_t)((TokenID & 0x0000FF00U) >> 8);

        // Initialize pointer for check
        Ptr_p += (Size - VEX_DMA_TOKEN_ID_SIZE);

        if (memcmp(Ptr_p, TokenBuf, sizeof(uint32_t)) != 0)
            return -1;

        LOG_INFO(VEX_LOG_PREFIX "%s: Data ready\n", __func__);
    }
    else
    {
        // Force some delay to increase the chance that the data is available
        Adapter_SleepMS(VEX_POLLING_DELAY_MS);
        Adapter_SleepMS(VEX_POLLING_DELAY_MS);
    }
    return 0;
}
#endif


void
vex_InitBufManager(void)
{
#ifdef VEX_CHECK_DMA_WITH_TOKEN_ID
    (void)BufManager_Register(vexLocal_CB_SizeAlignment,
                              vexLocal_CB_CheckClear,
                              vexLocal_CB_CheckReady);
#else
    (void)BufManager_Register(vexLocal_CB_SizeAlignment,
                              NULL,
                              NULL);
#endif
}


/* end of file adapter_vex_bufmanager.c */
