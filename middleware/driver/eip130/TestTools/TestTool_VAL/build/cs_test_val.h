/* cs_test_val.h
 *
 * Configuration Settings for the VAL API test tool.
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

#ifndef INCLUDE_GUARD_CS_TEST_VAL_TEST_H
#define INCLUDE_GUARD_CS_TEST_VAL_TEST_H

/*----------------------------------------------------------------------------
 * Definitions and macros
 */
// Enable debug logging
#define LOG_SEVERITY_MAX  LOG_SEVERITY_CRIT
//#define LOG_SEVERITY_MAX  LOG_SEVERITY_WARN
//#define LOG_SEVERITY_MAX  LOG_SEVERITY_INFO

#define TEST_VAL_USERMODE		////

/*----------------------------------------------------------------------------
 * Hardware and firmware configuration specifics
 */
#define VALTEST_SYM_ALGO_SHA512

#define VALTEST_SYM_ALGO_DES
#define VALTEST_SYM_ALGO_3DES
#define VALTEST_SYM_ALGO_CHACHA20
#define VALTEST_SYM_ALGO_POLY1305
#define VALTEST_SYM_ALGO_SM4
#define VALTEST_SYM_ALGO_ARIA

#define VALTEST_SYM_ALGO_AES_CCM
#define VALTEST_SYM_ALGO_AES_GCM
#define VALTEST_SYM_ALGO_AES_F8
#define VALTEST_SYM_ALGO_AES_XTS

#ifdef VALTEST_SYM_ALGO_ARIA
#define VALTEST_SYM_ALGO_ARIA_CCM
#define VALTEST_SYM_ALGO_ARIA_GCM
#endif

#define VALTEST_ASYM_CURVE25519
#define VALTEST_ASYM_EDDSA

/*----------------------------------------------------------------------------
 * VALTEST_RANDOM_VERYLONG
 * Defines that very long random number generation is allowed
 */
//#define VALTEST_RANDOM_VERYLONG

/*----------------------------------------------------------------------------
 * VALTEST_TRACE_NOP_DATA
 * Defines that the NOP result must be trace
 */
//#define VALTEST_TRACE_NOP_DATA


#endif /* INCLUDE_GUARD_CS_TEST_VAL_TEST_H */

/* end of file cs_test_val.h */
