/* adapter_driver_init.c
 *
 * Adapter top level module, EIP-13x driver's entry point.
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

#include "api_driver_init.h"    // Driver Init API


/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */

// Default Adapter configuration
#include "cs_adapter.h"      // ADAPTER_DRIVER_NAME,
                             // ADAPTER_INIT_VEX_STUB_ENABLE,
                             // ADAPTER_INIT_VEX_PROXY_ENABLE

// Adapter Initialization API
//#include "adapter_init.h"   // Adapter_*
//#include "adapter_global_init.h"

// Logging API
#include "log.h"            // LOG_INFO


/*----------------------------------------------------------------------------
 * Local prototypes
 */

int
Driver_Custom_Init(void);

void
Driver_Custom_UnInit(void);


/*----------------------------------------------------------------------------
 * Driver130_Init
 */
int
Driver130_Init(void)
{
    LOG_INFO("\n\t Driver130_Init \n");

    LOG_INFO("%s driver: initializing\n", ADAPTER_DRIVER_NAME);

#if 0
    if (!Adapter_Global_Init())
    {
        Adapter_UnInit();
        return -1;
    }
#endif

    if (Driver_Custom_Init() < 0)
    {
        return -1;
    }

    LOG_INFO("\n\t Driver130_Init done \n");

    return 0;   // success
}


/*----------------------------------------------------------------------------
 * Driver130_Exit
 */
void
Driver130_Exit(void)
{
    LOG_INFO("\n\t Driver130_Exit \n");

    LOG_INFO("%s driver: exit\n", ADAPTER_DRIVER_NAME);

    Driver_Custom_UnInit();

    //Adapter_Global_UnInit();

    LOG_INFO("\n\t Driver130_Exit done \n");
}


#include "adapter_driver_init_ext.h"


/* end of file adapter_driver_init.c */
