/** @file cs_hwpal_ext2.h
 *
 * @brief Configuration Settings for the hardware platform abstraction layer
 *        intended for the EIP-13x (FPGA) PCI chip DMA specifics.
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

#ifndef INCLUDE_GUARD_CS_HWPAL_EXT2_H
#define INCLUDE_GUARD_CS_HWPAL_EXT2_H

/*----------------------------------------------------------------------------
 * Definitions and macros
 */

// Enables DMA resources banks so that different memory regions can be used
// for DMA buffer allocation
#ifdef DRIVER_DMARESOURCE_BANKS_ENABLE
#define HWPAL_DMARESOURCE_BANKS_ENABLE
#endif // DRIVER_DMARESOURCE_BANKS_ENABLE

#ifdef HWPAL_DMARESOURCE_BANKS_ENABLE
// Definition of DMA banks, one dynamic and 1 static
//                                  Bank   Static   Blocks   Block size
#define HWPAL_DMARESOURCE_BANKS                                          \
        HWPAL_DMARESOURCE_BANK_ADD (0,       0,       0,          0),    \
        HWPAL_DMARESOURCE_BANK_ADD (1,       1,                          \
                                    DRIVER_TRANSFORM_RECORD_COUNT,       \
                                    DRIVER_TRANSFORM_RECORD_BYTE_COUNT)
#endif // HWPAL_DMARESOURCE_BANKS_ENABLE


#endif /* INCLUDE_GUARD_CS_HWPAL_EXT2_H */

/* end of file cs_hwpal_ext2.h */
