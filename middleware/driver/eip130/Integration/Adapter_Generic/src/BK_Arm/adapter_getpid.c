/* adapter_getpid.c
 *
 * Linux user-space specific Adapter module
 * responsible for adapter-wide pid management.
 */

/*****************************************************************************
* Copyright (c) 2015-2016 INSIDE Secure B.V. All Rights Reserved.
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

#include "adapter_getpid.h"

#if 0	////DEL
// C Library API
#include <sys/types.h>
#include <unistd.h>
#endif

/*----------------------------------------------------------------------------
 * Adapter_GetPid
 */
int
Adapter_GetPid(void)
{
    return 0;
}


/* end of file adapter_getpid.c */
