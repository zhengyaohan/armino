/* adapter_vex_init.c
 *
 * Adapter VEX initialization module.
 *
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

// VEX Adapter API
#include "adapter_vex.h"


/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */

// Default configuration
#include "c_adapter_vex.h"
#include "adapter_vex_internal.h"   // vex_DeviceInit/Exit, vex_InitBufManager
#include "log.h"


/*----------------------------------------------------------------------------
 * Definitions and macros
 */


/*----------------------------------------------------------------------------
 * Global constants
 */


/*----------------------------------------------------------------------------
 * Local variables
 */


/*----------------------------------------------------------------------------
 * Local prototypes
 */



/*----------------------------------------------------------------------------
 * vex_Init
 */
int
vex_Init(void)
{
    int rc;

    rc = vex_LockInit();
    if (rc < 0)
    {
        LOG_CRIT(VEX_LOG_PREFIX "FAILED to initialize lock (%d)\n", rc);
        return -1;
    }

    rc = vex_DeviceInit();
    if (rc < 0)
    {
        LOG_CRIT(VEX_LOG_PREFIX "FAILED to initialize device (%d)\n", rc);
        if (rc == -1)
        {
            vex_LockExit();
            return -2;
        }
    }

    vex_InitBufManager();

    return 0;
}


/*----------------------------------------------------------------------------
 * vex_UnInit
 */
void
vex_UnInit(void)
{
    vex_DeviceExit();
    vex_LockExit();

    return;
}


/* end of file adapter_vex_init.c */

