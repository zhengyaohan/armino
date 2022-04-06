/* adapter_lock.c
 *
 * Adapter concurrency (locking) management
 * MicroBlaze FreeRTOS implementation
 *
 */

/*****************************************************************************
* Copyright (c) 2017 INSIDE Secure B.V. All Rights Reserved.
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

// Adapter locking API
#include "adapter_lock.h"


/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */

// Driver Framework Basic Definitions API
#include "basic_defs.h"     // IDENTIFIER_NOT_USED

// Adapter Lock Internal API
#include "adapter_lock_internal.h"

// Adapter Memory Allocation API
#include "adapter_alloc.h"

// Logging API
#undef LOG_SEVERITY_MAX
#define LOG_SEVERITY_MAX    LOG_SEVERITY_WARN

// Logging API
#include "log.h"

#if 0	////DEL
// FreeRTOS API
#include "FreeRTOS.h"
#include "portable.h"   // pvPortMalloc, vPortFree
#include "task.h"       // vTaskDelay
#include "semphr.h"     // *Semaphore*
#endif

/*----------------------------------------------------------------------------
 * Definitions and macros
 */

typedef void* SemaphoreHandle_t;
typedef struct
{
    SemaphoreHandle_t SemHandle;
} Adapter_Lock_Internal_t;

Adapter_Lock_Internal_t Internal_lock;
uint32_t Internal_lock_Hndl = 0;

/*----------------------------------------------------------------------------
 * Adapter_Lock_Alloc
 */
Adapter_Lock_t
Adapter_Lock_Alloc(void)
{
    Adapter_Lock_Internal_t * Lock_p;
	Lock_p = &Internal_lock;
	Lock_p->SemHandle = (void *)&Internal_lock_Hndl;
    return Lock_p; // success
}


/*----------------------------------------------------------------------------
 * Adapter_Lock_Free
 */
void
Adapter_Lock_Free(
        Adapter_Lock_t Lock)
{
	IDENTIFIER_NOT_USED(Lock);
}


/*----------------------------------------------------------------------------
 * Adapter_Lock_Acquire
 */
void
Adapter_Lock_Acquire(
        Adapter_Lock_t Lock,
        unsigned long * Flags)
{
	IDENTIFIER_NOT_USED(Lock);
    IDENTIFIER_NOT_USED(Flags);
}


/*----------------------------------------------------------------------------
 * Adapter_Lock_Release
 */
void
Adapter_Lock_Release(
        Adapter_Lock_t Lock,
        unsigned long * Flags)
{
	IDENTIFIER_NOT_USED(Lock);
    IDENTIFIER_NOT_USED(Flags);
}


/* end of file adapter_lock.c */
