/* adapter_vex_logicaltoken.c
 *
 * Implementation of the VEX API.
 *
 * This file contains the logical token exchange implementation between the
 * VAL API and VEX API.
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

#include "basic_defs.h"             // uint16_t
#include "log.h"

#include "adapter_vex.h"            // API to implement
#include "adapter_vex_internal.h"   // vex_*() and VexToken_Command/Result_t


/*----------------------------------------------------------------------------
 * vex_LogicalToken
 */
VexStatus_t
vex_LogicalToken(
        VexToken_Command_t * const CommandToken_p,
        VexToken_Result_t * const ResultToken_p)
{
    VexStatus_t funcres = VEX_UNSUPPORTED;

#ifdef VEX_STRICT_ARGS
    if ((CommandToken_p == NULL) ||
        (ResultToken_p == NULL))
    {
        return VEX_BAD_ARGUMENT;
    }
#endif

    if (vex_LockAcquire() < 0)
    {
        return VEX_LOCK_TIMEOUT;
    }

    if (vex_DeviceGetHandle() != NULL)
    {
        // Device connection is available
        switch (CommandToken_p->OpCode)
        {
        case VEXTOKEN_OPCODE_NOP:
            funcres = vex_Nop(CommandToken_p, ResultToken_p);
            break;

        case VEXTOKEN_OPCODE_ENCRYPTION:
            if (CommandToken_p->SubCode)
            {
                funcres = vex_SymCipherAE(CommandToken_p, ResultToken_p);
            }
            else
            {
                funcres = vex_SymCipher(CommandToken_p, ResultToken_p);
            }
            break;

        case VEXTOKEN_OPCODE_HASH:
            funcres = vex_SymHash(CommandToken_p, ResultToken_p);
            break;

        case VEXTOKEN_OPCODE_MAC:
            funcres = vex_SymMac(CommandToken_p, ResultToken_p);
            break;

        case VEXTOKEN_OPCODE_TRNG:
            funcres = vex_Trng(CommandToken_p, ResultToken_p);
            break;

        case VEXTOKEN_OPCODE_SPECIALFUNCTIONS:
            funcres = vex_SpecialFunctions(CommandToken_p, ResultToken_p);
            break;

        case VEXTOKEN_OPCODE_AESWRAP:
#ifdef VEX_ENABLE_ENCRYPTED_VECTOR
            if (CommandToken_p->SubCode)
            {
                funcres = vex_EncryptedVector(CommandToken_p, ResultToken_p);
            }
            else
#endif
            {
                funcres = vex_SymKeyWrap(CommandToken_p, ResultToken_p);
            }
            break;

        case VEXTOKEN_OPCODE_ASSETMANAGEMENT:
            funcres = vex_Asset(CommandToken_p, ResultToken_p);
            break;

        case VEXTOKEN_OPCODE_AUTH_UNLOCK:
            funcres = vex_AUnlock(CommandToken_p, ResultToken_p);
            break;

        case VEXTOKEN_OPCODE_PUBLIC_KEY:
            funcres = vex_Asym(CommandToken_p, ResultToken_p);
            break;

        case VEXTOKEN_OPCODE_EMMC:
            funcres = vex_eMMC(CommandToken_p, ResultToken_p);
            break;

        case VEXTOKEN_OPCODE_EXTSERVICE:
            funcres = vex_ExtService(CommandToken_p, ResultToken_p);
            break;

        case VEXTOKEN_OPCODE_SERVICE:
            funcres = vex_Service(CommandToken_p, ResultToken_p);
            break;

        case VEXTOKEN_OPCODE_SYSTEM:
            funcres = vex_System(CommandToken_p, ResultToken_p);
            break;

#ifdef VEX_ENABLE_HW_FUNCTIONS
        case VEXTOKEN_OPCODE_HARDWARE:
            funcres = vex_Hardware(CommandToken_p, ResultToken_p);
            break;
#endif

        case VEXTOKEN_OPCODE_CLAIMCONTROL:
            funcres = vex_Claim(CommandToken_p, ResultToken_p);
            break;

        default:
            funcres = VEX_INVALID_OPCODE;
            break;
        }
    }
    else
    {
        funcres = VEX_NOT_CONNECTED;
    }

    vex_LockRelease();

    return funcres;
}


/* end of file adapter_vex_logicaltoken.c */
