/* mbfreertos.c
 *
 * C runtime library and some POSIX functions implementation
 * for MicroBlaze and FreeRTOS
 *
 */

/*****************************************************************************
* Copyright (c) 2017-2018 INSIDE Secure B.V. All Rights Reserved.
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

#include <stdlib.h>   // malloc, free
#include <unistd.h>   // usleep
#include <pthread.h>  // pthread_mutex*
#include <string.h>   // memcpy, memset


/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */

// FreeRTOS API
#include "FreeRTOS.h"
#include "portable.h"   // pvPortMalloc, vPortFree
#include "task.h"       // vTaskDelay


/*----------------------------------------------------------------------------
 * Definitions and macros
 */

#ifndef pdUS_TO_TICKS
#define pdUS_TO_TICKS( xTimeInMs ) ( ( TickType_t ) ( ( ( TickType_t ) \
                        ( xTimeInMs ) * ( TickType_t ) configTICK_RATE_HZ ) / \
                            ( TickType_t ) 1000000 ) )
#endif


/*----------------------------------------------------------------------------
 * Local variables
 */


/*----------------------------------------------------------------------------
 * malloc
 */
void *
malloc(
        size_t byte_count)
{
    return pvPortMalloc(byte_count);
}


/*----------------------------------------------------------------------------
 * free
 */
void
free(
        void * p)
{
    vPortFree(p);
}


/*----------------------------------------------------------------------------
 * usleep
 */
int
usleep(
        useconds_t useconds)
{
    vTaskDelay(pdUS_TO_TICKS(useconds) ? pdUS_TO_TICKS(useconds) : 1);

    return 0;
}


/* end of file mbfreertos.c */
