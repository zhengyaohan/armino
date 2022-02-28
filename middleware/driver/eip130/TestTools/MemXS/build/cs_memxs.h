/* cs_memxs.h
 *
 * Top-level configuration parameters for the MemXS module.
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

#ifndef INCLUDE_GUARD_CS_MEMXS_H
#define INCLUDE_GUARD_CS_MEMXS_H

/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */

// activate in case of endianness difference between CPU and EIP
// to ask driver to swap byte order of all control words
// we assume that if ARCH is not x86, then CPU is big endian
#ifndef ARCH_X86
#define DRIVER_SWAPENDIAN
#endif //ARCH_X86

// Top-level Driver Framework configuration
#include "cs_hwpal.h"


/*----------------------------------------------------------------------------
 * Definitions and macros
 */

// See description of these configuration parameters in c_memxs.h file
#undef LOG_SEVERITY_MAX
#define MEMXS_LOG_SEVERITY                      LOG_SEVERITY_WARN

#define MEMXS_ENABLE_PARAMETER_CHECK            1

#define MEMXS_PRINT_DEBUG_MSG                   1


#endif /* Include Guard */

/* end of file cs_memxs.h */
