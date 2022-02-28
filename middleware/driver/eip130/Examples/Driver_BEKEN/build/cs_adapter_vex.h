/** @file cs_adapter_vex.h
 *
 * @brief Configuration Settings for the VEX API driver.
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

#ifndef INCLUDE_GUARD_CS_ADAPTER_VEX_H
#define INCLUDE_GUARD_CS_ADAPTER_VEX_H

/*-----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */
#include "cs_driver.h"
#include "cs_adapter.h"


/*----------------------------------------------------------------------------
 * Definitions and macros
 */
/** Strict argument checking use */
#ifdef ADAPTER_STRICT_ARGS
//#define VEX_STRICT_ARGS
#endif

/** Token tracing use */
//#define VEX_TRACE_TOKENS		////

/** Defines the device name to use
 *  Please, look at the cs_hwpal_ext.h for the usable device names */
#define VEX_DEVICE_NAME                 DRIVER_DEVICE_NAME

/** SHA512 related functionality use */
#ifdef DRIVER_ENABLE_SHA512
#define VEX_ENABLE_SYM_ALGO_SHA512
#endif

/** DES related functionality use */
#ifdef DRIVER_ENABLE_DES
#define VEX_ENABLE_SYM_ALGO_DES
#endif

/** Triple-DES (3DES) related functionality use */
#ifdef DRIVER_ENABLE_3DES
#define VEX_ENABLE_SYM_ALGO_3DES
#endif

/** ChaCha20 related functionality use */
#ifdef DRIVER_ENABLE_CHACHA20
#define VEX_ENABLE_SYM_ALGO_CHACHA20
#endif

/** Poly1305 related functionality use */
#ifdef DRIVER_ENABLE_POLY1305
#define VEX_ENABLE_SYM_ALGO_POLY1305
#endif

/** SM4 related functionality use */
#ifdef DRIVER_ENABLE_SM4
#define VEX_ENABLE_SYM_ALGO_SM4
#endif

/** ARIA related functionality use */
#ifdef DRIVER_ENABLE_ARIA
#define VEX_ENABLE_SYM_ALGO_ARIA
#ifdef DRIVER_ENABLE_ARIA_CCM
#define VEX_ENABLE_SYM_ALGO_ARIA_CCM
#endif
#ifdef DRIVER_ENABLE_ARIA_GCM
#define VEX_ENABLE_SYM_ALGO_ARIA_GCM
#endif
#endif

/** AES-GCM related functionality use */
#ifdef DRIVER_ENABLE_AES_GCM
#define VEX_ENABLE_SYM_ALGO_AES_GCM
#endif

/** Special Functions Milenage related functionality use */
#ifdef DRIVER_ENABLE_SF_MILENAGE
#define VEX_ENABLE_SF_MILENAGE
#endif
/** Special Functions Bluetooth related functionality use */
#ifdef DRIVER_ENABLE_SF_BLUETOOTH
#define VEX_ENABLE_SF_BLUETOOTH
#endif
/** Special Functions Coverage related functionality use */
#ifdef DRIVER_ENABLE_SF_COVERAGE
#define VEX_ENABLE_SF_COVERAGE
#endif
/** Special Functions Protected App related functionality use */
#ifdef DRIVER_ENABLE_SF_PROTECTED_APP
#define VEX_ENABLE_SF_PROTECTED_APP
#endif

/** Encrypted Vector for PKI related functionality use */
#ifdef DRIVER_ENABLE_ENCRYPTED_VECTOR
#define VEX_ENABLE_ENCRYPTED_VECTOR
#endif

/** Firmware load related functionality use */
#ifdef DRIVER_ENABLE_FIRMWARE_LOAD

/** Defines that the firmware load must be enabled */
////#define VEX_ENABLE_FIRMWARE_LOAD		////ROM Version

/** Defines the firmware filename */
#define VEX_FIRMWARE_FILE               DRIVER_FIRMWARE_FILE

/** Defines the firmware sleep is enabled */
#ifdef DRIVER_ENABLE_FIRMWARE_SLEEP
#define VEX_ENABLE_FIRMWARE_SLEEP
#endif

/** Defines the firmware hibernation is enabled */
#ifdef DRIVER_ENABLE_FIRMWARE_HIBERATION
#define VEX_ENABLE_FIRMWARE_HIBERATION
#endif

#endif

/** Defines that HW Module status, options and version function must be
 *  enabled */
#define VEX_ENABLE_HW_FUNCTIONS

/** Defines the mailbox to use */
#define VEX_MAILBOX_NR 1

/** Defines if the mailbox must be link only once, otherwise the mailbox will
 *  be linked for every submitted token and released (unlinked) when the
 *  result token is read. */
#define VEX_MAILBOX_LINK_ONLY_ONCE

/** DMA operation finished checking based on TokenID use */
#ifdef DRIVER_DMAREADYCHECK
#define VEX_CHECK_DMA_WITH_TOKEN_ID
#endif

/** Token/Data poll timeout settings */
#ifdef DRIVER_POLLING_DELAY_MS
#define VEX_POLLING_DELAY_MS            DRIVER_POLLING_DELAY_MS
#endif
#ifdef DRIVER_POLLING_MAXLOOPS
#define VEX_POLLING_MAXLOOPS            DRIVER_POLLING_MAXLOOPS
#endif


#endif /* INCLUDE_GUARD_CS_ADAPTER_VEX_H */


/* end of file cs_adapter_vex.h */
