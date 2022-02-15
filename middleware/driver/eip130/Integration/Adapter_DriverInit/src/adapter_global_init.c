/* adapter_global_init.c
 *
 * Global Control initialization module.
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
#include "adapter_global_init.h"


/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */

// Default Adapter configuration
#include "c_adapter_global.h"           // ADAPTER_GLOBAL_PRNG_SEED

// Global Control API: Packet I/O
#include "api_global_eip130.h"

// Driver Framework Basic Definitions API
#include "basic_defs.h"                 // uint8_t, uint32_t, bool

// Driver Framework C Library API
#include "clib.h"                       // memcpy

// EIP-130 Driver Library Global Control API
#include "eip130_global_event.h"        // Event Management
#include "eip130_global_init.h"         // Init/Uninit

#include "device_types.h"               // Device_Handle_t
#include "device_mgmt.h"                // Device_find
#include "log.h"                        // Log API


/*----------------------------------------------------------------------------
 * Definitions and macros
 */


/*----------------------------------------------------------------------------
 * Local variables
 */


/*----------------------------------------------------------------------------
 * Adapter_Global_StatusReport()
 *
 * Obtain all available global status information from the EIP-130 driver
 * and report it.
 */
static void
Adapter_Global_StatusReport(void)
{
    LOG_INFO("\n\t\tAdapter_Global_StatusReport\n");

    LOG_CRIT("Global Status of the EIP-130\n");
}


/*----------------------------------------------------------------------------
 * Adapter_Global_Init()
 *
 */
bool
Adapter_Global_Init(void)
{
    LOG_INFO("\n\t\t Adapter_Global_Init \n");

    return true; // success
}


/*----------------------------------------------------------------------------
 * Adapter_Global_UnInit()
 *
 */
void
Adapter_Global_UnInit(void)
{
    LOG_INFO("\n\t\t Adapter_Global_UnInit \n");

    Adapter_Global_StatusReport();
}


/* end of file adapter_global_init.c */

