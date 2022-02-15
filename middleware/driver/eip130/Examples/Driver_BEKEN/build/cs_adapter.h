/** @file cs_adapter.h
 *
 * @brief Configuration Settings for the Driver Adapter module.
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

#ifndef INCLUDE_GUARD_CS_ADAPTER_H
#define INCLUDE_GUARD_CS_ADAPTER_H

/*-----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */
#include "cs_driver.h"


/*----------------------------------------------------------------------------
 * Definitions and macros
 */
/** Adapter/driver name */
#define ADAPTER_DRIVER_NAME "driver-eip130"

#ifdef DRIVER_LICENSE
#define ADAPTER_DRIVER_LICENSE  DRIVER_LICENSE
#endif

/** Maximum log severity - log level for the entire adapter (for now)
 *  choose from LOG_SEVERITY_INFO, LOG_SEVERITY_WARN, LOG_SEVERITY_CRIT */
#ifdef DRIVER_PERFORMANCE
#define LOG_SEVERITY_MAX    LOG_SEVERITY_CRITICAL
#else
#define LOG_SEVERITY_MAX    LOG_SEVERITY_WARN
#endif

/** Bouncebuffers use */
#ifndef DRIVER_BOUNCEBUFFERS
#define ADAPTER_REMOVE_BOUNCEBUFFERS
#endif

/** Endian swap use */
#ifdef DRIVER_SWAPENDIAN
#define ADAPTER_ENABLE_SWAP
#endif

// Enable byte swap for words in DMA buffers
#ifdef DRIVER_BUFMAN_SWAP_ENABLE
// Enable it for Xilinx Zynq 70x FPGA MicroBlaze configured in BigEndian mode
// because in this mode either the MicroBlaze or the AXI interconnect will
// swap bytes in 32-bit words before/after writing/reading them to/from memory.
#define ADAPTER_BUFMAN_SWAP_ENABLE
#endif

/** Adapter/driver host and device type */
// Is host platform 64-bit?
#ifdef DRIVER_64BIT_HOST
#define ADAPTER_64BIT_HOST
// Is device 64-bit? Only makes sense on 64-bit host.
#ifdef DRIVER_64BIT_DEVICE
#define ADAPTER_64BIT_DEVICE
#endif  // DRIVER_64BIT_DEVICE
#endif  // DRIVER_64BIT_HOST

/** Strict argument checking use */
#ifndef DRIVER_PERFORMANCE
#define ADAPTER_STRICT_ARGS
#endif

// Misc options
/** Maximum DMA resource handles */
#define ADAPTER_MAX_DMARESOURCE_HANDLES  8		////128==>Org

/** Interrupt use */
#ifdef DRIVER_INTERRUPTS
#define ADAPTER_EIP130_INTERRUPTS_ENABLE
#endif

/** EIP130 interrupt signals\n
 *  Assigned values represent interrupt source bit numbers */
enum
{
    IRQ_MBX1_IN_FREE_IRQ  = 0,
    IRQ_MBX1_OUT_FULL_IRQ = 1,
    IRQ_MBX2_IN_FREE_IRQ  = 2,
    IRQ_MBX2_OUT_FULL_IRQ = 3,
    IRQ_MBX3_IN_FREE_IRQ  = 4,
    IRQ_MBX3_OUT_FULL_IRQ = 5,
    IRQ_MBX4_IN_FREE_IRQ  = 6,
    IRQ_MBX4_OUT_FULL_IRQ = 7,
    IRQ_MBX_LINKABLE_IRQ  = 8,
};

#define ADAPTER_IRQ_MBX1_IN_FREE_IRQ_NAME     "IRQ_MBX1_FREE"
#define ADAPTER_IRQ_MBX1_OUT_FULL_IRQ_NAME    "IRQ_MBX1_FULL"
#define ADAPTER_IRQ_MBX2_IN_FREE_IRQ_NAME     "IRQ_MBX2_FREE"
#define ADAPTER_IRQ_MBX2_OUT_FULL_IRQ_NAME    "IRQ_MBX2_FULL"
#define ADAPTER_IRQ_MBX3_IN_FREE_IRQ_NAME     "IRQ_MBX3_FREE"
#define ADAPTER_IRQ_MBX3_OUT_FULL_IRQ_NAME    "IRQ_MBX3_FULL"
#define ADAPTER_IRQ_MBX4_IN_FREE_IRQ_NAME     "IRQ_MBX4_FREE"
#define ADAPTER_IRQ_MBX4_OUT_FULL_IRQ_NAME    "IRQ_MBX4_FULL"
#define ADAPTER_IRQ_MBX_LINKABLE_IRQ_NAME     "IRQ_MBX_LINKABLE"

/** Cache line size */
#define ADAPTER_PCICONFIG_CACHELINESIZE 1

/** VEX stub use */
#ifdef DRIVER_VEX_STUB_ENABLE
#define ADAPTER_INIT_VEX_STUB_ENABLE
#endif

/** VEX proxy use */
#ifdef DRIVER_VEX_PROXY_ENABLE
#define ADAPTER_INIT_VEX_PROXY_ENABLE
#endif

/** Token/Data poll timeout settings, delay wait time in milliseconds */
#ifdef DRIVER_POLLING_DELAY_MS
#define ADAPTER_BUFMAN_POLLING_DELAY_MS     DRIVER_POLLING_DELAY_MS
#endif

/** Token/Data poll timeout settings, maximum number of polling attempts */
#ifdef DRIVER_POLLING_MAXLOOPS
#define ADAPTER_BUFMAN_POLLING_MAXLOOPS     DRIVER_POLLING_MAXLOOPS
#endif


#endif /* INCLUDE_GUARD_CS_ADAPTER_H */

/* end of file cs_adapter.h */
