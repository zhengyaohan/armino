/** @file cs_eip130_token.h
 *
 * @brief Configuration Settings for the EIP130 Token helper functions.
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

#ifndef INCLUDE_GUARD_CS_EIP130_TOKEN_H
#define INCLUDE_GUARD_CS_EIP130_TOKEN_H

/*-----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */
#include "cs_driver.h"


/*----------------------------------------------------------------------------
 * Definitions and macros
 */
/** Strict argument checking use */
#ifndef DRIVER_PERFORMANCE
//#define EIP130TOKEN_STRICT_ARGS
#endif

// Use the options below to selectively enable specific functions
#ifdef DRIVER_ENABLE_AES_CCM
#define EIP130TOKEN_ENABLE_SYM_ALGO_AES_CCM
#endif
#ifdef DRIVER_ENABLE_AES_GCM
#define EIP130TOKEN_ENABLE_SYM_ALGO_AES_GCM
#endif
#ifdef DRIVER_ENABLE_AES_F8
#define EIP130TOKEN_ENABLE_SYM_ALGO_AES_F8
#endif
#ifdef DRIVER_ENABLE_ARIA_CCM
#define EIP130TOKEN_ENABLE_SYM_ALGO_ARIA_CCM
#endif
#ifdef DRIVER_ENABLE_ARIA_GCM
#define EIP130TOKEN_ENABLE_SYM_ALGO_ARIA_GCM
#endif
#ifdef DRIVER_ENABLE_CHACHA20
#define EIP130TOKEN_ENABLE_SYM_ALGO_CHACHA20
#endif
#ifdef DRIVER_ENABLE_SM4
#define EIP130TOKEN_ENABLE_SYM_ALGO_SM4
#endif
#ifdef DRIVER_ENABLE_ENCRYPTED_VECTOR
#define EIP130TOKEN_ENABLE_ENCRYPTED_VECTOR
#endif


#endif /* INCLUDE_GUARD_CS_EIP130_TOKEN_H */

/* end of file cs_eip130_token.h */
