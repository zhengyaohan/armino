/* c_vex_proxy.h
 *
 * Configuration options for the VEX API Proxy
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

#ifndef INCLUDE_GUARD_C_VEX_PROXY_H
#define INCLUDE_GUARD_C_VEX_PROXY_H

/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */

// Top-level Adapter configuration
#include "cs_vex_proxy.h"

#ifndef LOG_SEVERITY_MAX
#define LOG_SEVERITY_MAX  LOG_SEVERITY_WARN
#endif

#ifndef VEX_PROXY_NODE_NAME
#define VEX_PROXY_NODE_NAME     "/dev/vexp_c"
#endif


#endif  /* Include Guard */


/* end of file c_vex_proxy.h */
