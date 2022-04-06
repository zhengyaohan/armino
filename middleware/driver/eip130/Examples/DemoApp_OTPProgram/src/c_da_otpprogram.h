/** @file c_da_otpprogram.h
 *
 * @brief OTP programming example configuration options
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

#ifndef INCLUDE_GUARD_C_DA_OTPPROGRAM_H
#define INCLUDE_GUARD_C_DA_OTPPROGRAM_H

/*-----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */
#include "cs_da_otpprogram.h"          // Configuration settings


/*----------------------------------------------------------------------------
 * Definitions and macros
 */
/** Maximum log severity */
#ifndef LOG_SEVERITY_MAX
#define LOG_SEVERITY_MAX  LOG_SEVERITY_CRIT
#endif

// Remove key blob read operations using file system,
// a statically linked key blob will be used instead
//#define DA_OTPPROGRAM_KEYBLOB_FS_REMOVE

// Remove main() from the application so that it can be linked as a library
//#define DA_OTPPROGRAM_MAIN_REMOVE


#endif /* INCLUDE_GUARD_C_DA_OTPPROGRAM_H */


/* end of file c_da_otpprogram.h */
