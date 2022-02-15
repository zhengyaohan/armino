/** @file cs_da_otpkeyblob.h
 *
 * @brief OTP KeyBlob generation example configuration settings
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

#ifndef INCLUDE_GUARD_CS_DA_OTPKEYBLOB_H
#define INCLUDE_GUARD_CS_DA_OTPKEYBLOB_H

/*----------------------------------------------------------------------------
 * Definitions and macros
 */
/** Maximum log severity */
#define LOG_SEVERITY_MAX  LOG_SEVERITY_CRIT
//#define LOG_SEVERITY_MAX  LOG_SEVERITY_WARN
//#define LOG_SEVERITY_MAX  LOG_SEVERITY_INFO

#ifdef ARCH_MB
// MicroBlaze deployment does not have a file system
// Remove key blob read operations using file system,
// a statically linked key blob will be used instead
#define DA_OTPKEYBLOB_FS_REMOVE
#endif


#endif /* INCLUDE_GUARD_CS_DA_OTPKEYBLOB_H */


/* end of file cs_da_otpkeyblob.h */
