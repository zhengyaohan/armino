/* c_memxs.h
 *
 * Default MemXS Module Configuration
 */

/*****************************************************************************
* Copyright (c) 2010-2016 INSIDE Secure B.V. All Rights Reserved.
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

#ifndef C_MEMXS_H_
#define C_MEMXS_H_

/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */

// Top-level product configuration
#include "cs_memxs.h"

/*----------------------------------------------------------------------------
 * Definitions and macros
 */

// Enable input parameter check for MemXS_xxx functions.
//  Set 1 to enable
//  Set 0 to disable
//#define MEMXS_ENABLE_PARAMETER_CHECK            1

// Enable debug message print
//  Set 1 to enable
//  Set 0 to disable
//#define MEMXS_PRINT_DEBUG_MSG                   1

// Here is the dependency on the Driver Framework configuration
// via the MemXS configuration
#ifndef HWPAL_DEVICES
#error "Expected HWPAL_DEVICES defined by cs_memxs.h"
#endif

// log level for the MemXS
// choose from LOG_SEVERITY_INFO, LOG_SEVERITY_WARN, LOG_SEVERITY_CRIT
#ifndef MEMXS_LOG_SEVERITY
#define MEMXS_LOG_SEVERITY  LOG_SEVERITY_WARN
#endif

#endif /* C_MEMXS_H_ */

/* end of file c_memxs.h */

