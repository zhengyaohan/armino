/* c_test_val.h
 *
 * Module description
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

#ifndef INCLUDE_GUARD_C_TEST_VAL_H
#define INCLUDE_GUARD_C_TEST_VAL_H

/*----------------------------------------------------------------------------
 * This module implements (provides) the following interface(s):
 */
#include "cs_test_val.h"                // Configuration settings


/*----------------------------------------------------------------------------
 * Definitions and macros
 */
// Enable debug logging
#ifndef LOG_SEVERITY_MAX
#define LOG_SEVERITY_MAX    LOG_SEVERITY_CRIT
#endif

#ifndef TEST_VAL_LICENSE
#define TEST_VAL_LICENSE    "GPL"
#endif

// Remove main() from the application so that it can be linked as a library
#define TEST_VAL_MAIN_REMOVE


/* Define to block code inclusion */
#define TEST_VAL_NOT_USED    0


#endif /* INCLUDE_GUARD_C_TEST_VAL_H */

/* end of file c_test_val.h */

