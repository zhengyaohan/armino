/** @file cs_driver.h
 *
 * @brief Configuration Settings for the EIP-13x Combined (VIP) Driver.
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

#ifndef INCLUDE_GUARD_CS_DRIVER_H
#define INCLUDE_GUARD_CS_DRIVER_H

/*-----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */
#include "cs_systemtestconfig.h"


/*----------------------------------------------------------------------------
 * Definitions and macros
 */
/** System behaviour specifics:\n */
/** Big-endian CPU use */
#ifdef ARCH_POWERPC
// Enable byte swap for 32-bit words in registers and internal memory,
// e.g. for device slave interface
#define DRIVER_SWAPENDIAN
#endif

#ifdef ARCH_MB_BE // MicroBlaze in Big Endian configuration
// Enable byte swap for words in DMA buffers, e.g. for device master interface
#define DRIVER_BUFMAN_SWAP_ENABLE
#endif

/** General driver/device name */
#define DRIVER_NAME             "EIP130"
#define DRIVER_DEVICE_NAME      DRIVER_NAME

/** Driver license */
#define DRIVER_LICENSE          "GPL"

/** Firmware file name */
#define DRIVER_FIRMWARE_FILE    "firmware_eip130ram.sbi"

/** Extend the VAL API with functions that can only be used in a Secure
 *  environment and by the Crypto Officer. By default, the functions that can be
 *  used in non-secure and secure environment are enabled */
#define DRIVER_VALAPI_CRYPTO_OFFICER

/** DMA operation finished checking based on TokenID use */
#define DRIVER_DMAREADYCHECK

/** Hardware and firmware specifics related to the used configuration:\n */
#define DRIVER_ENABLE_SHA512            /** SHA512 use */
#define DRIVER_ENABLE_AES_F8            /** AES_f8 use ; Note: AES is enabled by default */
#define DRIVER_ENABLE_AES_CCM           /** AES-CCM use ; Note: AES is enabled by default */
#define DRIVER_ENABLE_AES_GCM           /** AES_GCM use ; Note: AES is enabled by default */
#define DRIVER_ENABLE_AES_XTS           /** XTS-AES use ; Note: AES is enabled by default */
#define DRIVER_ENABLE_AES_KEYWRAP       /** AES Keywrap use */
#define DRIVER_ENABLE_DES               /** DES use */
#define DRIVER_ENABLE_3DES              /** Triple DES use */
#define DRIVER_ENABLE_CHACHA20          /** ChaCha20 use */
#define DRIVER_ENABLE_POLY1305          /** Poly1305 use */
#define DRIVER_ENABLE_SM4               /** SM4 use */
#define DRIVER_ENABLE_ARIA              /** ARIA use */
#define DRIVER_ENABLE_ARIA_CCM          /** ARIA-CCM use ; Note: DRIVER_ENABLE_ARIA must be enabled as well */
#define DRIVER_ENABLE_ARIA_GCM          /** ARIA_GCM use ; Note: DRIVER_ENABLE_ARIA must be enabled as well */
#define DRIVER_ENABLE_SF_MILENAGE       /** Special Functions for Milenage */
//#define DRIVER_ENABLE_SF_BLUETOOTH      /** Special Functions for Bluetooth */
//#define DRIVER_ENABLE_ENCRYPTED_VECTOR  /** Encrypted vector for PKI use */

#define DRIVER_ENABLE_FIRMWARE_LOAD         /** enable firmware load */
#define DRIVER_ENABLE_FIRMWARE_SLEEP        /** enable firmware sleep functionality */
#define DRIVER_ENABLE_FIRMWARE_HIBERATION   /** enable firmware hibernation functionality */


/** System Configuration */
// Driver Conf  Poll/Int  BounceBuff
// C0             Poll       Off
#ifdef SYSTEMTEST_CONFIGURATION_C0
//#define DRIVER_INTERRUPT
//#define DRIVER_BOUNCEBUFFERS
#define DRIVER_PERFORMANCE
#endif // SYSTEMTEST_CONFIGURATION_C0

// Driver Conf  Poll/Int  BounceBuff
// C1             Int        Off
#ifdef SYSTEMTEST_CONFIGURATION_C1
#define DRIVER_INTERRUPT
//#define DRIVER_BOUNCEBUFFERS
#define DRIVER_PERFORMANCE
#endif // SYSTEMTEST_CONFIGURATION_C1

// Driver Conf  Poll/Int  BounceBuff
// C2             Poll       Off
#ifdef SYSTEMTEST_CONFIGURATION_C2
//#define DRIVER_INTERRUPT
//#define DRIVER_BOUNCEBUFFERS
//#define DRIVER_PERFORMANCE
#endif // SYSTEMTEST_CONFIGURATION_C2

// Driver Conf  Poll/Int  BounceBuff
// C3             Poll       On
#ifdef SYSTEMTEST_CONFIGURATION_C3
//#define DRIVER_INTERRUPT
#define DRIVER_BOUNCEBUFFERS
//#define DRIVER_PERFORMANCE
#endif // SYSTEMTEST_CONFIGURATION_C3

// Driver Conf  Poll/Int  BounceBuff
// C4             Int        On
#ifdef SYSTEMTEST_CONFIGURATION_C4
#define DRIVER_INTERRUPT
#define DRIVER_BOUNCEBUFFERS
#define DRIVER_PERFORMANCE
#endif // SYSTEMTEST_CONFIGURATION_C4

// Driver Conf  Poll/Int  BounceBuff
// C5             Int        On
#ifdef SYSTEMTEST_CONFIGURATION_C5
#define DRIVER_INTERRUPT
#define DRIVER_BOUNCEBUFFERS
//#define DRIVER_PERFORMANCE
#endif // SYSTEMTEST_CONFIGURATION_C5


/*-----------------------------------------------------------------------------
 * Include platform specific driver configuration
 */
#include "cs_driver_ext.h"


#endif // INCLUDE_GUARD_CS_DRIVER_H

/* end of file cs_driver.h */
