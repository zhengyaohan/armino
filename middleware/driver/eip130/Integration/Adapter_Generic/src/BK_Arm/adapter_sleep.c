/* adapter_sleep.c
 *
 * Linux user-space specific Adapter module
 * responsible for adapter-wide time management.
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

// Adapter Sleep API
////#include "adapter_sleep.h"
#include "basic_defs.h"

#if 0	////DEL
// FreeRTOS interfaces
#include "FreeRTOS.h"   // pdMS_TO_TICKS
#include "task.h"       // vTaskDelay
#endif

#define DELAY_MS_COUNT		1000	////Need correct
#define DELAY_US_COUNT		10	////Need correct

void DelayMs(volatile uint16_t times)
{
	uint32_t delay = (times * DELAY_MS_COUNT);
    while(delay--) ;
}	


void DelayUs(volatile uint16_t times)
{
	uint32_t delay = (times * DELAY_US_COUNT);
    while(delay--) ;
}

/*----------------------------------------------------------------------------
 * Adapter_SleepMS
 */
void
Adapter_SleepMS(
        const unsigned int Duration_ms)
{
    DelayMs(Duration_ms);
}

/* end of file adapter_sleep.c */
