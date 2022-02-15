/* cs_hwpal_ext.h
 *
 * EIP-13x chip specific configuration parameters
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

#ifndef CS_HWPAL_EXT_H_
#define CS_HWPAL_EXT_H_
#include "vault_reg.h"

/*----------------------------------------------------------------------------
 * Definitions and macros
 */

// For obtaining the IRQ number
#ifdef DRIVER_INTERRUPTS
#define HWPAL_INTERRUPTS
#endif

// Disable PCI Configuration Space support support
#define HWPAL_REMOVE_DEVICE_PCICONFIGSPACE

#define HWPAL_DMARESOURCE_REMOVE_ATTACH

// Device name in the Device Tree Structure
#define HWPAL_DEVICE_NAME           DRIVER_DEVICE_NAME
#define HWPAL_PLATFORM_DEVICE_NAME  DRIVER_DEVICE_NAME

// Max number of IRQ's supported by device
#define HWPAL_PLATFORM_IRQ_COUNT    1

// Index of the IRQ in the "interrupts" property of the Open Firmware device
// tree entry. 0 is the first IRQ listed, 1 is the second IRQ listed, etc.
#define HWPAL_PLATFORM_IRQ_IDX      0

#define HWPAL_REMAP_ADDRESSES       ;

// Definition of static resources inside the device
// Refer to the data sheet of device for the correct values
//                   Name           DevNr  Start       Last        Flags (see below)
#define HWPAL_DEVICES \
    HWPAL_DEVICE_ADD(HWPAL_DEVICE_NAME,       0, EIP_FIRST_OFS, EIP_LAST_OFS, 7)
// Flags:
//   bit0 = Trace reads (requires HWPAL_TRACE_DEVICE_READ)
//   bit1 = Trace writes (requires HWPAL_TRACE_DEVICE_WRITE)
//   bit2 = Swap word endianness (requires HWPAL_DEVICE_ENABLE_SWAP)

// Maximum number of devices in the device list,
// must be equal to or greater than the number of devices in HWPAL_DEVICES
#define HWPAL_DEVICE_COUNT          1

// Address where the device list is mapped in the MMIO
#define HWPAL_DEVICE_MEM_ADDR   	0		////0x00C00000U		////????


// Enables DMA resources banks so that different memory regions can be used
// for DMA buffer allocation
#ifdef DRIVER_DMARESOURCE_BANKS_ENABLE
#define HWPAL_DMARESOURCE_BANKS_ENABLE
#endif // DRIVER_DMARESOURCE_BANKS_ENABLE

#ifdef HWPAL_DMARESOURCE_BANKS_ENABLE
// Definition of DMA banks, one dynamic and 1 static
//                                 Bank    Type   Shared  Cached  Addr  Blocks   Block Size
#define HWPAL_DMARESOURCE_BANKS                                                              \
        HWPAL_DMARESOURCE_BANK_ADD (0,       0,     0,      1,      0,    0,         0),     \
        HWPAL_DMARESOURCE_BANK_ADD (1,       1,     1,      1,      0,                       \
                                    DRIVER_DMA_BANK_ELEMENT_COUNT,                           \
                                    DRIVER_DMA_BANK_ELEMENT_BYTE_COUNT)
#endif // HWPAL_DMARESOURCE_BANKS_ENABLE

#define HWPAL_DMARESOURCE_ADDR_MASK               0xffffffffULL


#endif /* CS_HWPAL_EXT_H_ */


/* end of file cs_hwpal_ext.h */
