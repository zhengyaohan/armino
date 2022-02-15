/** @file cs_adapter_val.h
 *
 * @brief Configuration Settings for the VAL API driver.
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

#ifndef INCLUDE_GUARD_CS_ADAPTER_VAL_H
#define INCLUDE_GUARD_CS_ADAPTER_VAL_H

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
#define VAL_STRICT_ARGS
#endif

/*----------------------------------------------------------------
 * Define to set the AssetNumber used to identify the root key (HUK)
 * (typically in OTP)
 */
/** Root key (HUK) Asset Number (typically in OTP)  */
#define VAL_ROOT_KEY_INDEX    0


/*----------------------------------------------------------------
 * Defines (switches) that can be used to remove features (functionality)
 */
// - Asset Management related
//#define VAL_REMOVE_ASSET_ALLOC
//#define VAL_REMOVE_ASSET_FREE
//#define VAL_REMOVE_ASSET_SEARCH
//#define VAL_REMOVE_GET_ROOT_KEY
//#define VAL_REMOVE_ASSET_LOAD_IMPORT
//#define VAL_REMOVE_ASSET_LOAD_DERIVE
//#define VAL_REMOVE_ASSET_LOAD_PLAINTEXT
//#define VAL_REMOVE_ASSET_LOAD_PLAINTEXT_EXPORT
//#define VAL_REMOVE_ASSET_LOAD_RANDOM
//#define VAL_REMOVE_ASSET_LOAD_RANDOM_EXPORT
//#define VAL_REMOVE_ASSET_LOAD_AESWRAP
//#define VAL_REMOVE_SECURETIMER
//#define VAL_REMOVE_PUBLICDATAREAD
//#define VAL_REMOVE_MONOTONIC_COUNTER_READ
//#define VAL_REMOVE_MONOTONIC_COUNTER_INC
#ifndef DRIVER_VALAPI_CRYPTO_OFFICER
#define VAL_REMOVE_OTPDATAWRITE
#define VAL_REMOVE_PROVISIONRANDOMROOTKEY
#endif

// - Symmetric crypto related
//#define VAL_REMOVE_SYM_HASH
//#define VAL_REMOVE_SYM_MAC
#ifndef DRIVER_ENABLE_SHA512
#define VAL_REMOVE_SYM_ALGO_SHA512
#endif
//#define VAL_REMOVE_SYM_CIPHER
#ifndef DRIVER_ENABLE_DES
#define VAL_REMOVE_SYM_ALGO_DES
#endif
#ifndef DRIVER_ENABLE_3DES
#define VAL_REMOVE_SYM_ALGO_3DES
#endif
#ifndef DRIVER_ENABLE_CHACHA20
#define VAL_REMOVE_SYM_ALGO_CHACHA20
#endif
#ifndef DRIVER_ENABLE_POLY1305
#define VAL_REMOVE_SYM_ALGO_POLY1305
#endif
#ifndef DRIVER_ENABLE_SM4
#define VAL_REMOVE_SYM_ALGO_SM4
#endif
#ifndef DRIVER_ENABLE_ARIA
#define VAL_REMOVE_SYM_ALGO_ARIA
#define VAL_REMOVE_SYM_ALGO_ARIA_F8
#define VAL_REMOVE_SYM_ALGO_ARIA_XTS
#define VAL_REMOVE_SYM_ALGO_ARIA_CCM
#define VAL_REMOVE_SYM_ALGO_ARIA_GCM
#else
#define VAL_REMOVE_SYM_ALGO_ARIA_F8
#define VAL_REMOVE_SYM_ALGO_ARIA_XTS
#ifndef DRIVER_ENABLE_ARIA_CCM
#define VAL_REMOVE_SYM_ALGO_ARIA_CCM
#endif
#ifndef DRIVER_ENABLE_ARIA_GCM
#define VAL_REMOVE_SYM_ALGO_ARIA_GCM
#endif
#endif
#ifndef DRIVER_ENABLE_AES_F8
#define VAL_REMOVE_SYM_ALGO_AES_F8
#endif
#ifndef DRIVER_ENABLE_AES_XTS
#define VAL_REMOVE_SYM_ALGO_AES_XTS
#endif
#ifndef DRIVER_ENABLE_AES_CCM
#define VAL_REMOVE_SYM_ALGO_AES_CCM
#endif
#ifndef DRIVER_ENABLE_AES_GCM
#define VAL_REMOVE_SYM_ALGO_AES_GCM
#endif
#ifndef DRIVER_ENABLE_AES_KEYWRAP
#define VAL_REMOVE_SYM_AESKEYWRAP
#endif
#ifndef DRIVER_ENABLE_ENCRYPTED_VECTOR
#define VAL_REMOVE_ENCRYPTED_VECTOR
#endif

// - Asymmetric crypto related
//#define VAL_REMOVE_ASYM_ECC_GEN_KEYPAIR
//#define VAL_REMOVE_ASYM_ECC_GEN_PUBLICKEY
//#define VAL_REMOVE_ASYM_ECC_KEY_CHECK

//#define VAL_REMOVE_ASYM_ECC_ELGAMAL_DECRYPT
//#define VAL_REMOVE_ASYM_ECC_ELGAMAL_ENCRYPT

//#define VAL_REMOVE_ASYM_ECDH_GEN_SHAREDSECRET_SINGLE
//#define VAL_REMOVE_ASYM_ECDH_GEN_SHAREDSECRET_DUAL

//#define VAL_REMOVE_ASYM_ECDSA_SIGN
//#define VAL_REMOVE_ASYM_ECDSA_VERIFY

//#define VAL_REMOVE_ASYM_CURVE25519

//#define VAL_REMOVE_ASYM_EDDSA_SIGN
//#define VAL_REMOVE_ASYM_EDDSA_VERIFY

//#define VAL_REMOVE_ASYM_DH_GEN_KEYPAIR
//#define VAL_REMOVE_ASYM_DH_GEN_PUBLICKEY
//#define VAL_REMOVE_ASYM_DH_KEY_CHECK

//#define VAL_REMOVE_ASYM_DH_GEN_SHAREDSECRET_SINGLE
//#define VAL_REMOVE_ASYM_DH_GEN_SHAREDSECRET_DUAL

//#define VAL_REMOVE_ASYM_DSA_SIGN
//#define VAL_REMOVE_ASYM_DSA_VERIFY
//#define VAL_REMOVE_ASYM_DSA_KEY_CHECK

//#define VAL_REMOVE_ASYM_RSAPKCS1V15_SIGN
//#define VAL_REMOVE_ASYM_RSAPKCS1V15_VERIFY
//#define VAL_REMOVE_ASYM_RSAPSS_SIGN
//#define VAL_REMOVE_ASYM_RSAPSS_VERIFY

//#define VAL_REMOVE_ASYM_RSA_OAEP

//#define VAL_REMOVE_ASYM_PK_WITHOUT_ASSET

// - Authenticated Unlock related
//#define VAL_REMOVE_AUTH_UNLOCK
//#define VAL_REMOVE_SECURE_DEBUG

// - Random related
#ifndef DRIVER_VALAPI_CRYPTO_OFFICER
#define VAL_REMOVE_TRNG_CONFIG
#endif
//#define VAL_REMOVE_RANDOM_DATA
//#define VAL_REMOVE_RANDOM_RESEED
//#define VAL_REMOVE_VERIFY_DRBG_POSTPROCCESSING
//#define VAL_REMOVE_VERIFY_NRBG_SELFTEST

// - Miscellaneous
//#define VAL_REMOVE_NOP

// - Special Functions related
#ifndef DRIVER_ENABLE_SF_MILENAGE
#define VAL_REMOVE_SF_MILENAGE
#endif
#ifndef DRIVER_ENABLE_SF_BLUETOOTH
#define VAL_REMOVE_SF_BLUETOOTH
#endif
#ifndef DRIVER_ENABLE_SF_COVERAGE
#define VAL_REMOVE_SF_COVERAGE
#endif
#ifndef DRIVER_ENABLE_SF_PROTECTED_APP
#define VAL_REMOVE_SF_PROTECTED_APP
#endif

// - Service related
//#define VAL_REMOVE_SERVICE_REGISTERREAD
#ifndef DRIVER_VALAPI_CRYPTO_OFFICER
#define VAL_REMOVE_SERVICE_REGISTERWRITE
#endif
//#define VAL_REMOVE_SERVICE_ZEROIZEMAILBOX
//#define VAL_REMOVE_SERVICE_ZEROIZEOTP
#ifndef DRIVER_VALAPI_CRYPTO_OFFICER
#define VAL_REMOVE_CLOCKSWITCH
#endif

// - System related
//#define VAL_REMOVE_SYSTEM_GETVERSION
//#define VAL_REMOVE_SYSTEM_RESET
//#define VAL_REMOVE_SYSTEM_SELFTEST
#ifndef DRIVER_VALAPI_CRYPTO_OFFICER
#define VAL_REMOVE_SYSTEM_DEFINE_USERS
#define VAL_REMOVE_SYSTEM_SLEEP
#define VAL_REMOVE_SYSTEM_HIBERNATION
#define VAL_REMOVE_SYSTEM_SET_TIME
#endif

// - Hardware related
//#define VAL_REMOVE_HARDWARE_MODULESTATUS
//#define VAL_REMOVE_HARDWARE_EIP_OPTIONS
//#define VAL_REMOVE_HARDWARE_EIP_VERSION

// - Claim control related
//#define VAL_REMOVE_CLAIM


#endif /* INCLUDE_GUARD_CS_ADAPTER_VAL_H */

/* end of file cs_adapter_val.h */
