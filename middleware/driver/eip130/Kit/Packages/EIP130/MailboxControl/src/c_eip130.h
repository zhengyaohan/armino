/* c_eip130.h
 *
 * Configuration options for the EIP130 module.
 * The project-specific cs_eip130.h file is included,
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

#ifndef INCLUDE_GUARD_C_EIP130_H
#define INCLUDE_GUARD_C_EIP130_H

/*----------------------------------------------------------------
 * Defines that can be used in the cs_eip130.h file
 */
// Set this option to enable checking of all arguments to all EIP130 functions
// disable it to reduce code size and reduce overhead
// #define EIP130_STRICT_ARGS

// Footprint reduction switches
// #define EIP130_REMOVE_MAILBOXACCESSVERIFY
// #define EIP130_REMOVE_MAILBOXGETOPTIONS
// #define EIP130_REMOVE_MAILBOXACCESSCONTROL
// #define EIP130_REMOVE_MAILBOXLINK
// #define EIP130_REMOVE_MAILBOXUNLINK
// #define EIP130_REMOVE_MAILBOXCANWRITETOKEN
// #define EIP130_REMOVE_MAILBOXRAWSTATUS
// #define EIP130_REMOVE_MAILBOXRESET
// #define EIP130_REMOVE_MAILBOXLINKID
// #define EIP130_REMOVE_MAILBOXOUTID
// #define EIP130_REMOVE_FIRMWAREDOWNLOAD

/*----------------------------------------------------------------
 * inclusion of cs_eip130.h
 */
#include "cs_eip130.h"


/*----------------------------------------------------------------
 * provide backup values for all missing configuration parameters
 */


/*----------------------------------------------------------------
 * other configuration parameters that cannot be set in cs_xxx.h
 * but are considered product-configurable anyway
 */


#endif /* INCLUDE_GUARD_C_EIP130_H */

/* end of file c_eip130.h */
