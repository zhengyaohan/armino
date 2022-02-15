/** @file cs_umdevxs.h
 *
 * @brief Configuration Settings for the UMDevXS driver (Linux user-space).
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

#ifndef INCLUDE_GUARD_CS_UMDEVXS_H
#define INCLUDE_GUARD_CS_UMDEVXS_H

/*----------------------------------------------------------------------------
 * Definitions and macros
 */

/** logging level */
#ifndef LOG_SEVERITY_MAX
#define LOG_SEVERITY_MAX LOG_SEVERITY_WARN
#endif

// uncomment to remove selected functionality
#define UMDEVXS_REMOVE_DEVICE
#define UMDEVXS_REMOVE_DMABUF
#define UMDEVXS_REMOVE_SMBUF
#define UMDEVXS_REMOVE_SMBUF_PROVIDER
#define UMDEVXS_REMOVE_SMBUF_OBTAINER
#define UMDEVXS_REMOVE_SIMULATION
#define UMDEVXS_REMOVE_INTERRUPT

#define UMDEVXS_MODULENAME "vexp"


#endif /* INCLUDE_GUARD_CS_UMDEVXS_H */

/* end of file cs_umdevxs.h */
