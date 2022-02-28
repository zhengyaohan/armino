/** @file cs_eip130.h
 *
 * @brief Configuration Settings for the EIP130 module.
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

#ifndef INCLUDE_GUARD_CS_EIP130_H
#define INCLUDE_GUARD_CS_EIP130_H

/*-----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */
#include "cs_driver.h"


/*----------------------------------------------------------------------------
 * Definitions and macros
 */
/** Strict argument checking use */
#ifndef DRIVER_PERFORMANCE
//#define EIP130_STRICT_ARGS
#endif

/** Footprint reduction switches */
//#define EIP130_REMOVE_MODULEGETOPTIONS
//#define EIP130_REMOVE_MODULEGETSTATUS
#ifndef DRIVER_ENABLE_FIRMWARE_SLEEP
#define EIP130_REMOVE_MODULEFIRMWAREWRITTEN
#endif

//#define EIP130_REMOVE_MAILBOXGETOPTIONS
#define EIP130_REMOVE_MAILBOXACCESSVERIFY
//#define EIP130_REMOVE_MAILBOXACCESSCONTROL
//#define EIP130_REMOVE_MAILBOXLINK
//#define EIP130_REMOVE_MAILBOXLINKRESET
//#define EIP130_REMOVE_MAILBOXUNLINK
//#define EIP130_REMOVE_MAILBOXCANWRITETOKEN
#define EIP130_REMOVE_MAILBOXRAWSTATUS
#define EIP130_REMOVE_MAILBOXRESET
#define EIP130_REMOVE_MAILBOXLINKID
#define EIP130_REMOVE_MAILBOXOUTID

#ifndef DRIVER_ENABLE_FIRMWARE_LOAD
#define EIP130_REMOVE_FIRMWAREDOWNLOAD
#endif

#endif /* INCLUDE_GUARD_CS_EIP130_H */

/* end of file cs_eip130.h */
