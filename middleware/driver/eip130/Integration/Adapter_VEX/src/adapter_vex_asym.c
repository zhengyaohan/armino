/* adapter_vex_asym.c
 *
 * Implementation of the VEX API.
 *
 * This file implements the main entry for asymmetric crypto services.
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

#include "c_adapter_vex.h"          // configuration

#include "basic_defs.h"
#include "clib.h"
#include "log.h"

#include "adapter_vex_intern_asym.h"   // API implementation


/*----------------------------------------------------------------------------
 * vex_Asym
 */
VexStatus_t
vex_Asym(
        VexToken_Command_t * const CommandToken_p,
        VexToken_Result_t * const ResultToken_p)
{
    VexStatus_t funcres = VEX_UNSUPPORTED;

    switch (CommandToken_p->SubCode)
    {
    case VEXTOKEN_SUBCODE_PK_NOASSETS:
        switch (CommandToken_p->Service.PkOperation.Operation)
        {
        case VEXTOKEN_PK_OP_NUMLOAD:
            funcres = vex_Asym_PkaNumLoad(CommandToken_p, ResultToken_p);
            break;

        case VEXTOKEN_PK_OP_NUMSETN:
            funcres = vex_Asym_PkaNumSet(CommandToken_p, ResultToken_p);
            break;

        case VEXTOKEN_PK_OP_MODEXPE:
        case VEXTOKEN_PK_OP_MODEXPD:
        case VEXTOKEN_PK_OP_MODEXPCRT:
        case VEXTOKEN_PK_OP_ECMONTMUL:
        case VEXTOKEN_PK_OP_ECCMUL:
        case VEXTOKEN_PK_OP_ECCADD:
        case VEXTOKEN_PK_OP_DSASIGN:
        case VEXTOKEN_PK_OP_DSAVERIFY:
        case VEXTOKEN_PK_OP_ECDSASIGN:
        case VEXTOKEN_PK_OP_ECDSAVERIFY:
            funcres = vex_Asym_PkaOperation(CommandToken_p, ResultToken_p);
            break;

        default:
            break;
        }
        break;

    case VEXTOKEN_SUBCODE_PK_WITHASSETS:
        switch (CommandToken_p->Service.PkAssetSignVerify.Method)
        {
        case VEXTOKEN_PKASSET_ECDH_ECDSA_KEYCHK:
        case VEXTOKEN_PKASSET_DH_DSA_KEYCHK:
            funcres = vex_Asym_AssetKeyCheck(CommandToken_p, ResultToken_p);
            break;

        case VEXTOKEN_PKASSET_DSA_SIGN:
        case VEXTOKEN_PKASSET_ECDSA_SIGN:
        case VEXTOKEN_PKASSET_RSA_PKCS1V1_5_SIGN:
        case VEXTOKEN_PKASSET_RSA_PSS_SIGN:
        case VEXTOKEN_PKASSET_EDDSA_SIGN_INITIAL:
        case VEXTOKEN_PKASSET_EDDSA_SIGN_UPDATE:
        case VEXTOKEN_PKASSET_EDDSA_SIGN_FINAL:
            funcres = vex_Asym_AssetSign(CommandToken_p, ResultToken_p);
            break;

        case VEXTOKEN_PKASSET_DSA_VERIFY:
        case VEXTOKEN_PKASSET_ECDSA_VERIFY:
        case VEXTOKEN_PKASSET_RSA_PKCS1V1_5_VERIFY:
        case VEXTOKEN_PKASSET_RSA_PSS_VERIFY:
        case VEXTOKEN_PKASSET_EDDSA_VERIFY_INITIAL:
        case VEXTOKEN_PKASSET_EDDSA_VERIFY_FINAL:
            funcres = vex_Asym_AssetVerify(CommandToken_p, ResultToken_p);
            break;

        case VEXTOKEN_PKASSET_DH_GEN_PUBKEY:
        case VEXTOKEN_PKASSET_ECDH_ECDSA_GEN_PUBKEY:
        case VEXTOKEN_PKASSET_CURVE25519_GEN_PUBKEY:
        case VEXTOKEN_PKASSET_EDDSA_GEN_PUBKEY:
            funcres = vex_Asym_AssetGenKeyPublic(CommandToken_p, ResultToken_p);
            break;

        case VEXTOKEN_PKASSET_DH_GEN_KEYPAIR:
        case VEXTOKEN_PKASSET_ECDH_ECDSA_GEN_KEYPAIR:
        case VEXTOKEN_PKASSET_CURVE25519_GEN_KEYPAIR:
        case VEXTOKEN_PKASSET_EDDSA_GEN_KEYPAIR:
            funcres = vex_Asym_AssetGenKeyPair(CommandToken_p, ResultToken_p);
            break;

        case VEXTOKEN_PKASSET_ECC_ELGAMAL_ENCRYPT:
        case VEXTOKEN_PKASSET_ECC_ELGAMAL_DECRYPT:
            funcres = vex_Asym_AssetEncrypt(CommandToken_p, ResultToken_p);
            break;

        case VEXTOKEN_PKASSET_DH_GEN_SHARED_SECRET_SKEYPAIR:
        case VEXTOKEN_PKASSET_DH_GEN_SHARED_SECRET_DKEYPAIR:
        case VEXTOKEN_PKASSET_ECDH_GEN_SHARED_SECRET_SKEYPAIR:
        case VEXTOKEN_PKASSET_ECDH_GEN_SHARED_SECRET_DKEYPAIR:
        case VEXTOKEN_PKASSET_CURVE25519_GEN_SHARED_SECRET:
            funcres = vex_Asym_AssetGenSharedSecret(CommandToken_p, ResultToken_p);
            break;

        case VEXTOKEN_PKASSET_RSA_OAEP_WRAP_STRING:
        case VEXTOKEN_PKASSET_RSA_OAEP_WRAP_HASH:
        case VEXTOKEN_PKASSET_RSA_OAEP_UNWRAP_STRING:
        case VEXTOKEN_PKASSET_RSA_OAEP_UNWRAP_HASH:
        case VEXTOKEN_PKASSET_RSA_PKCS1V15_WRAP:
        case VEXTOKEN_PKASSET_RSA_PKCS1V15_UNWRAP:
            funcres = vex_Asym_AssetKeyWrap(CommandToken_p, ResultToken_p);
            break;

        case VEXTOKEN_PKASSET_RSA_PKCS1V1_5_SIGN_CRT:
        case VEXTOKEN_PKASSET_RSA_PSS_SIGN_CRT:
        default:
            break;
        }
        break;

    default:
        break;
    }

    return funcres;
}


/* end of file adapter_vex_asym.c */
