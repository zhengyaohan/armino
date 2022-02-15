/* adapter_vex_intern_asym.h
 *
 * VEX API: Internal interfaces and definitions specific for Asymmetric Crypto.
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

#ifndef INCLUDE_GUARD_ADAPTER_VEX_INTERN_ASYM_H
#define INCLUDE_GUARD_ADAPTER_VEX_INTERN_ASYM_H

#include "adapter_vex_internal.h"   // Generic internal interfaces and definitions


/*----------------------------------------------------------------------------
 * vex_Asym_AssetSign
 *
 * This function handles the signature generate operation for ECDSA, DSA and
 * RSA based on a token using Assets.
 *
 * CommandToken_p
 *     Pointer to the buffer with the service request.
 *
 * ResultToken_p
 *     Pointer to the buffer in which the service result needs to be returned.
 *
 * Return Value:
 *     One of the VexStatus_t values.
 */
VexStatus_t
vex_Asym_AssetSign(
        VexToken_Command_t * const CommandToken_p,
        VexToken_Result_t * const ResultToken_p);

/*----------------------------------------------------------------------------
 * vex_Asym_AssetVerify
 *
 * This function handles the signature verify operation for ECDSA, DSA and
 * RSA based on a token using Assets.
 *
 * CommandToken_p
 *     Pointer to the buffer with the service request.
 *
 * ResultToken_p
 *     Pointer to the buffer in which the service result needs to be returned.
 *
 * Return Value:
 *     One of the VexStatus_t values.
 */
VexStatus_t
vex_Asym_AssetVerify(
        VexToken_Command_t * const CommandToken_p,
        VexToken_Result_t * const ResultToken_p);

/*----------------------------------------------------------------------------
 * vex_Asym_AssetEncrypt
 *
 * This function handles an asymmetric encrypt or decrypt operation based on
 * a token using Assets.
 *
 * CommandToken_p
 *     Pointer to the buffer with the service request.
 *
 * ResultToken_p
 *     Pointer to the buffer in which the service result needs to be returned.
 *
 * Return Value:
 *     One of the VexStatus_t values.
 */
VexStatus_t
vex_Asym_AssetEncrypt(
        VexToken_Command_t * const CommandToken_p,
        VexToken_Result_t * const ResultToken_p);

/*----------------------------------------------------------------------------
 * vex_Asym_AssetGenKeyPair
 *
 * This function handles the key pair generation operation based on a token
 * using Assets.
 *
 * CommandToken_p
 *     Pointer to the buffer with the service request.
 *
 * ResultToken_p
 *     Pointer to the buffer in which the service result needs to be returned.
 *
 * Return Value:
 *     One of the VexStatus_t values.
 */
VexStatus_t
vex_Asym_AssetGenKeyPair(
        VexToken_Command_t * const CommandToken_p,
        VexToken_Result_t * const ResultToken_p);

/*----------------------------------------------------------------------------
 * vex_Asym_AssetGenKeyPublic
 *
 * This function handles the public key generation operation based on a token
 * using Assets.
 *
 * CommandToken_p
 *     Pointer to the buffer with the service request.
 *
 * ResultToken_p
 *     Pointer to the buffer in which the service result needs to be returned.
 *
 * Return Value:
 *     One of the VexStatus_t values.
 */
VexStatus_t
vex_Asym_AssetGenKeyPublic(
        VexToken_Command_t * const CommandToken_p,
        VexToken_Result_t * const ResultToken_p);

/*----------------------------------------------------------------------------
 * vex_Asym_AssetKeyCheck
 *
 * This function handles the key check operation for ECDH/ECDSA based on
 * a token using Assets.
 *
 * CommandToken_p
 *     Pointer to the buffer with the service request.
 *
 * ResultToken_p
 *     Pointer to the buffer in which the service result needs to be returned.
 *
 * Return Value:
 *     One of the VexStatus_t values.
 */
VexStatus_t
vex_Asym_AssetKeyCheck(
        VexToken_Command_t * const CommandToken_p,
        VexToken_Result_t * const ResultToken_p);

/*----------------------------------------------------------------------------
 * vex_Asym_AssetGenSharedSecret
 *
 * This function handles the generation of shared secret(s) operation based on
 * a token using Assets.
 *
 * CommandToken_p
 *     Pointer to the buffer with the service request.
 *
 * ResultToken_p
 *     Pointer to the buffer in which the service result needs to be returned.
 *
 * Return Value:
 *     One of the VexStatus_t values.
 */
VexStatus_t
vex_Asym_AssetGenSharedSecret(
        VexToken_Command_t * const CommandToken_p,
        VexToken_Result_t * const ResultToken_p);

/*----------------------------------------------------------------------------
 * vex_Asym_AssetKeyWrap
 *
 * This function handles the key wrap and unwrap functionality for RSA-OAEP.
 *
 * CommandToken_p
 *     Pointer to the buffer with the service request.
 *
 * ResultToken_p
 *     Pointer to the buffer in which the service result needs to be returned.
 *
 * Return Value:
 *     One of the VexStatus_t values.
 */
VexStatus_t
vex_Asym_AssetKeyWrap(
        VexToken_Command_t * const CommandToken_p,
        VexToken_Result_t * const ResultToken_p);


/*----------------------------------------------------------------------------
 * vex_Asym_PkaNumSet
 *
 * This function handles the claim and release of PKA/PkCP for an operation.
 *
 * CommandToken_p
 *     Pointer to the buffer with the service request.
 *
 * ResultToken_p
 *     Pointer to the buffer in which the service result needs to be returned.
 *
 * Return Value:
 *     One of the VexStatus_t values.
 */
VexStatus_t
vex_Asym_PkaNumSet(
        VexToken_Command_t * const CommandToken_p,
        VexToken_Result_t * const ResultToken_p);

/*----------------------------------------------------------------------------
 * vex_Asym_PkaNumLoad
 *
 * This function handles loading of a PKA vector.
 *
 * CommandToken_p
 *     Pointer to the buffer with the service request.
 *
 * ResultToken_p
 *     Pointer to the buffer in which the service result needs to be returned.
 *
 * Return Value:
 *     One of the VexStatus_t values.
 */
VexStatus_t
vex_Asym_PkaNumLoad(
        VexToken_Command_t * const CommandToken_p,
        VexToken_Result_t * const ResultToken_p);

/*----------------------------------------------------------------------------
 * vex_Asym_PkaOperation
 *
 * This function handles the execution of an PKA/PKCP operation.
 *
 * CommandToken_p
 *     Pointer to the buffer with the service request.
 *
 * ResultToken_p
 *     Pointer to the buffer in which the service result needs to be returned.
 *
 * Return Value:
 *     One of the VexStatus_t values.
 */
VexStatus_t
vex_Asym_PkaOperation(
        VexToken_Command_t * const CommandToken_p,
        VexToken_Result_t * const ResultToken_p);


#endif /* Include Guard */

/* end of file adapter_vex_intern_asym.h */
