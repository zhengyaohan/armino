/* adapter_val_claim.c
 *
 * Implementation of the VAL API.
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

#include "c_adapter_val.h"              // configuration

#include "basic_defs.h"
#include "clib.h"
#include "log.h"

#include "api_val_claim.h"              // the API to implement
#include "adapter_val_internal.h"       // val_ExchangeToken()
#include "adapter_vex.h"                // VexToken_Command_t, VexToken_Result_t


/*----------------------------------------------------------------------------
 * val_Claim
 */
#ifndef VAL_REMOVE_CLAIM
ValStatus_t
val_Claim(void)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;

    // Format service request
    t_cmd.OpCode  = VEXTOKEN_OPCODE_CLAIMCONTROL;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_CLAIM_EXCLUSIVE_USE;

    // Exchange service request with the next driver level
    return val_ExchangeToken(&t_cmd, &t_res);
}
#endif /* !VAL_REMOVE_CLAIM */


/*----------------------------------------------------------------------------
 * val_ClaimOverrule
 */
#ifndef VAL_REMOVE_CLAIM
ValStatus_t
val_ClaimOverrule(void)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;

    // Format service request
    t_cmd.OpCode  = VEXTOKEN_OPCODE_CLAIMCONTROL;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_CLAIM_EXCLUSIVE_USE_OVERRULE;

    // Exchange service request with the next driver level
    return val_ExchangeToken(&t_cmd, &t_res);
}
#endif /* !VAL_REMOVE_CLAIM */


/*----------------------------------------------------------------------------
 * val_ClaimRelease
 */
#ifndef VAL_REMOVE_CLAIM
ValStatus_t
val_ClaimRelease(void)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;

    // Format service request
    t_cmd.OpCode  = VEXTOKEN_OPCODE_CLAIMCONTROL;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_CLAIM_RELEASE;

    // Exchange service request with the next driver level
    return val_ExchangeToken(&t_cmd, &t_res);
}
#endif /* !VAL_REMOVE_CLAIM */


/* end of file adapter_val_claim.c */
