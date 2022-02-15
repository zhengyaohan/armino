/* adapter_driver_init_ext.h
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

#ifndef INCLUDE_GUARD_ADAPTER_DRIVER_INIT_EXT_H
#define INCLUDE_GUARD_ADAPTER_DRIVER_INIT_EXT_H

#ifndef ADAPTER_INIT_VEX_PROXY_ENABLE
  #include "adapter_init.h"       // Adapter_*
  #include "adapter_vex.h"        // vex_Init/UnInit()
#endif


/*----------------------------------------------------------------------------
 * Driver_Custom_Init
 */
int
Driver_Custom_Init(void)
{
    Adapter_Report_Build_Params();

    if (!Adapter_Init())
    {
        return -1;
    }

    vex_Init();

    return 0; // success
}


/*----------------------------------------------------------------------------
 * Driver_Custom_UnInit
 */
void
Driver_Custom_UnInit(void)
{
#ifndef ADAPTER_INIT_VEX_PROXY_ENABLE
    vex_UnInit();

    Adapter_UnInit();
#endif
}


#endif /* Include guard */


/* end of file adapter_driver_init_ext.h */
