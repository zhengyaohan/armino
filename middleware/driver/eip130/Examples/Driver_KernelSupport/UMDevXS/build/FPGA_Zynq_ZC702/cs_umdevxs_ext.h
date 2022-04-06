/* cs_umdevxs_ext.h
 *
 * Configuration Switches for UMDevXS driver
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

#ifndef CS_UMDEVXS_EXT_H_
#define CS_UMDEVXS_EXT_H_


#define UMDEVXS_LICENSE    "GPL"

#define UMDEVXS_REMOVE_PCI
#define UMDEVXS_REMOVE_SMALL_PCIWINDOW_SUPPORT

// Device name in the Device Tree Structure
#define UMDEVXS_PLATFORM_DEVICE_NAME  "EIP130"

// Index of the IRQ in the "interrupts" property of the Open Firmware device
// tree entry. 0 is the first IRQ listed, 1 is the second IRQ listed, etc.
#define UMDEVXS_PLATFORM_IRQ_IDX 1

#define UMDEVXS_LOG_PREFIX "UMDevXS: "

#define UMDEVXS_MODULENAME "umdevxs"

// definition of device resources
//                           Name      Start    Last
#define UMDEVXS_DEVICES \
      UMDEVXS_DEVICE_ADD_OF("EIP130", 0x00000, 0x7FFFF)

// Filter for tracing interrupts: 0 - no traces, 0xFFFFFFFF - all interrupts
#define UMDEVXS_INTERRUPT_TRACE_FILTER  0x0


#endif /* CS_UMDEVXS_EXT_H_ */


/* end of file cs_umdevxs_ext.h */
