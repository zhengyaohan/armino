/** @file c_adapter_val.h
 *
 * @brief Configuration options for the VAL API driver
 *
 * The project-specific cs_adapter_val.h file is included,
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

#ifndef INCLUDE_GUARD_C_ADAPTER_VAL_H
#define INCLUDE_GUARD_C_ADAPTER_VAL_H

/*-----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */
#include "cs_adapter_val.h"

/** Maximum log severity */
#ifndef LOG_SEVERITY_MAX
#define LOG_SEVERITY_MAX  LOG_SEVERITY_WARN
#endif

/** Authenticated Unlock avialability */
#ifndef VAL_REMOVE_SECURE_DEBUG
#ifdef VAL_REMOVE_AUTH_UNLOCK
#undef VAL_REMOVE_AUTH_UNLOCK
#endif
#endif


#endif /* INCLUDE_GUARD_C_ADAPTER_VAL_H */

/* end of file c_adapter_val.h */
