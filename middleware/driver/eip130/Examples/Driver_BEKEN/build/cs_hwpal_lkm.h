/** @file cs_hwpal_lkm.h
 *
 * @brief Configuration Settings for the hardware platform abstraction layer
 *        intended for the Linux Kernel Module.
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

#ifndef INCLUDE_GUARD_CS_HWPAL_LKM_H
#define INCLUDE_GUARD_CS_HWPAL_LKM_H

/*-----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */
#include "cs_hwpal.h"


/*----------------------------------------------------------------------------
 * Definitions and macros
 */
#ifdef ARCH_ARM64
// Enable cache-coherent DMA buffer allocation
#define HWPAL_DMARESOURCE_ALLOC_CACHE_COHERENT
#endif

#if defined(ARCH_ARM) || defined(ARCH_ARM64)
// Use non-cached DMA buffer allocation
//#define HWPAL_DMARESOURCE_ALLOC_CACHE_COHERENT

// Use minimum required cache-control functionality for DMA-safe buffers
//#define HWPAL_DMARESOURCE_MINIMUM_CACHE_CONTROL

//#define HWPAL_DMARESOURCE_DIRECT_DCHACHE_CONTROL
//#define HWPAL_DMARESOURCE_DSB_ENABLE
#endif // defined(ARCH_ARM) || defined(ARCH_ARM64)

#if defined(DRIVER_SWAPENDIAN) && defined(DRIVER_ENABLE_SWAP_SLAVE)
#define HWPAL_DEVICE_ENABLE_SWAP
#endif // DRIVER_SWAPENDIAN


#define HWPAL_DMARESOURCE_BANK_STATIC_PCI_OFFSET  0x200000


#endif // INCLUDE_GUARD_CS_HWPAL_LKM_H

/* end of file cs_hwpal_lkm.h */
