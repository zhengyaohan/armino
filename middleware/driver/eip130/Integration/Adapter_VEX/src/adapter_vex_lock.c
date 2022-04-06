/* adapter_vex_lock.c
 *
 * Implementation of the VEX API.
 *
 * This file implements the locking specific functions.
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

/*----------------------------------------------------------------------------
 * This module implements (provides) the following interface(s):
 */

#include "adapter_vex_internal.h"   // API implementation


/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */

#include "c_adapter_vex.h"          // configuration

#include "adapter_lock.h"           // Adapter_Lock_t, Adapter_Lock*()
#include "adapter_sleep.h"          // Adapter_SleepMS()


/*----------------------------------------------------------------------------
 * Definitions and macros
 */

#define ADAPTER_VEX_LOCK_100_MS         100 // 100 milliseconds


/*----------------------------------------------------------------------------
 * Local variables
 */

// Lock and critical section for vex_Init/Uninit()
static Adapter_Lock_t gl_VexInitLock;
static Adapter_Lock_CS_t gl_VexInitCS;


/*----------------------------------------------------------------------------
 * vex_LockInit
 */
int
vex_LockInit(void)
{
    // Initialize lock mechanism
    gl_VexInitLock = Adapter_Lock_Alloc();

    if (gl_VexInitLock == Adapter_Lock_NULL)
        return -1;

    Adapter_Lock_CS_Set(&gl_VexInitCS, gl_VexInitLock);

    return 0;
}


/*----------------------------------------------------------------------------
 * vex_LockExit
 */
void
vex_LockExit(void)
{
    Adapter_Lock_Free(gl_VexInitLock);

    Adapter_Lock_CS_Set(&gl_VexInitCS, Adapter_Lock_NULL);
}


/*----------------------------------------------------------------------------
 * vex_LockAcquire
 */
int
vex_LockAcquire(void)
{
    while (!Adapter_Lock_CS_Enter(&gl_VexInitCS))
        Adapter_SleepMS(ADAPTER_VEX_LOCK_100_MS);

    return 0;
}


/*----------------------------------------------------------------------------
 * vex_LockRelease
 */
void
vex_LockRelease(void)
{
    Adapter_Lock_CS_Leave(&gl_VexInitCS);
}


/* end of file adapter_vex_lock.c */

