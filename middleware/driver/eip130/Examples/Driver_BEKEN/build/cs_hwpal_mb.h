/** cs_hwpal_mb.h
 *
 * Configuration Settings for the hardware platform abstraction layer
 * intended for the Linux user-space.
 */

/*****************************************************************************
* Copyright (c) 2017-2018 INSIDE Secure B.V. All Rights Reserved.
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

#ifndef INCLUDE_GUARD_CS_HWPAL_MB_H
#define INCLUDE_GUARD_CS_HWPAL_MB_H

/**----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */

#include "cs_hwpal_umdevxs.h"


/*----------------------------------------------------------------------------
 * Definitions and macros
 */

// DMA memory not used for D-cache
//#ifndef HWPAL_ARCH_COHERENT
//#define HWPAL_ARCH_COHERENT
//#endif

// Performance optimizations
//#define HWPAL_DMARES_UMDEVXS_OPT1
//#define HWPAL_DMARES_UMDEVXS_OPT2

// Log DMA buffer operations details
//#define HWPAL_TRACE_DMARESOURCE_BUF

// Detect DMA resource leaks
//#define HWPAL_TRACE_DMARESOURCE_LEAKS

// Lowest allowed DMA address
#define HWPAL_DMARESOURCE_DMA_ADDR_MIN      0x20000000U

// Highest allowed DMA address
#define HWPAL_DMARESOURCE_DMA_ADDR_MAX      0x3FFFFFFFU

// Logging level that DMAResource_CheckAndRegister() function will use
// for reporting errors that can be considered warnings otherwise
#define HWPAL_DMARESOURCE_LOG_LEVEL         LOG_INFO


#endif // INCLUDE_GUARD_CS_HWPAL_MB_H


/* end of file cs_hwpal_mb.h */
