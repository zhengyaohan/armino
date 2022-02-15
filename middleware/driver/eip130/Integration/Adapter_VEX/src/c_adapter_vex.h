/* c_adapter_vex.h
 *
 * Configuration options for the VEX API driver
 * The project-specific cs_adapter_vex.h file is included,
 * whereafter defaults are provided for missing parameters.
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

#ifndef INCLUDE_GUARD_C_ADAPTER_VEX_H
#define INCLUDE_GUARD_C_ADAPTER_VEX_H

/*----------------------------------------------------------------
 * Defines that can be used in the cs_xxx.h file
 */

/* currently none */


/*----------------------------------------------------------------
 *  cs_adapter_vex.h inclusion
 */
#include "cs_adapter_vex.h"

#ifndef LOG_SEVERITY_MAX
#define LOG_SEVERITY_MAX  LOG_SEVERITY_WARN
#endif

#ifndef VEX_DEVICE_NAME
// Set the default device name to use
#define VEX_DEVICE_NAME "EIP130"
#endif

#ifndef VEX_LOG_PREFIX
#define VEX_LOG_PREFIX  "VEX: "
#endif

#ifndef VEX_FIRMWARE_FILE
// Set the default firmware image filename
#define VEX_FIRMWARE_FILE  "firmware_eip130ram.sbi"
#endif

#ifndef VEX_MAILBOX_NR
// Set the default mailbox to use
#define VEX_MAILBOX_NR  1
#endif

#ifndef VEX_CRYPTO_OFFICER_ID
// Set the default Crypto Officer ID to use
#define VEX_CRYPTO_OFFICER_ID  0x4F5A3647
#endif

#ifndef VEX_POLLING_SKIP_FIRST_DELAYS
// Set the default
#define VEX_POLLING_SKIP_FIRST_DELAYS  50
#endif

#ifndef VEX_POLLING_DELAY_MS
// Set the default
#define VEX_POLLING_DELAY_MS  1
#endif

#ifndef VEX_POLLING_MAXLOOPS
// Set the default
#define VEX_POLLING_MAXLOOPS  5000
#endif


#endif /* INCLUDE_GUARD_C_ADAPTER_VEX_H */


/* end of file c_adapter_vex.h */
