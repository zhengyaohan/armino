/* c_eip130_token.h
 *
 * Configuration options for the Security Module Token helper functions.
 * The project-specific cs_eip130_token.h file is included, whereafter
 * defaults are provided for missing parameters and checks for illegal
 * and conflicting settings can be performed.
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

#ifndef INCLUDE_GUARD_C_EIP130_TOKEN_H
#define INCLUDE_GUARD_C_EIP130_TOKEN_H

/*----------------------------------------------------------------
 * Defines that can be used in the cs_eip130_token.h file
 */

// This option enables function call parameter checking
// disable it to reduce code size and reduce overhead
// make sure upper layer does not rely on these checks!
//#define EIP130TOKEN_STRICT_ARGS

// Use the options below to selectively enable specific functions
//#define EIP130TOKEN_ENABLE_SYM_ALGO_AES_F8
//#define EIP130TOKEN_ENABLE_SYM_ALGO_AES_CCM
//#define EIP130TOKEN_ENABLE_SYM_ALGO_AES_GCM
//#define EIP130TOKEN_ENABLE_SYM_ALGO_ARIA_CCM
//#define EIP130TOKEN_ENABLE_SYM_ALGO_ARIA_CCM
//#define EIP130TOKEN_ENABLE_SYM_ALGO_CHACHA20
//#define EIP130TOKEN_ENABLE_SYM_ALGO_SM4
//#define EIP130TOKEN_ENABLE_ENCRYPTED_VECTOR


/*----------------------------------------------------------------
 * inclusion of cs_eip130_token.h
 */
#include "cs_eip130_token.h"


/*----------------------------------------------------------------
 * provide backup values for all missing configuration parameters
 */


/*----------------------------------------------------------------
 * check for conflicting settings and illegal values
 */


/*----------------------------------------------------------------
 * other configuration parameters that cannot be set in cs_xxx.h
 * but are considered product-configurable anyway
 */


#endif /* INCLUDE_GUARD_C_EIP130_TOKEN_H */

/* end of file c_eip130_token.h */
