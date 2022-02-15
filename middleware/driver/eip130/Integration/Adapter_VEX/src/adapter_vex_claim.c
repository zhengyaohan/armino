/* adapter_vex_claim.c
 *
 * Implementation of the VEX API.
 *
 * This file implements the services with which the exclusive mailbox locking
 * can be controlled.
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

#include "adapter_vex_internal.h"   // API implementation


/*----------------------------------------------------------------------------
 * vex_Claim
 */
VexStatus_t
vex_Claim(
        VexToken_Command_t * const CommandToken_p,
        VexToken_Result_t * const ResultToken_p)
{
    uint32_t Identity;
    uint8_t MailboxNumber;
    int funcres;

    // Get and set identity of calling process
    Identity = vex_IdentityGet();
    if (Identity == 0)
    {
        return VEX_NO_IDENTITY;
    }

    // Get mailbox number to use
    MailboxNumber = vex_MailboxGet(Identity);
    if (MailboxNumber == 0)
    {
        return VEX_NO_MAILBOX;
    }

    switch (CommandToken_p->SubCode)
    {
    case VEXTOKEN_SUBCODE_CLAIM_EXCLUSIVE_USE:
        funcres = vex_DeviceLinkMailbox(MailboxNumber, Identity);
        if (funcres != 0)
        {
            if (funcres == -5)
            {
                return VEX_MAILBOX_IN_USE;
            }
            return VEX_INTERNAL_ERROR;
        }
        break;

    case VEXTOKEN_SUBCODE_CLAIM_EXCLUSIVE_USE_OVERRULE:
        funcres = vex_DeviceLinkMailboxOverrule(MailboxNumber, Identity);
        if (funcres != 0)
        {
            return VEX_INTERNAL_ERROR;
        }
        break;

    case VEXTOKEN_SUBCODE_CLAIM_RELEASE:
        funcres = vex_DeviceUnlinkMailbox(MailboxNumber, Identity);
        if (funcres != 0)
        {
            return VEX_INTERNAL_ERROR;
        }
        break;

    default:
        return VEX_UNSUPPORTED;
    }

    ResultToken_p->Result = 0;
    return VEX_SUCCESS;
}

/* end of file adapter_vex_claim.c */
