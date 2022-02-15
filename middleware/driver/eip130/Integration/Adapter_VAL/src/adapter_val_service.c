/* adapter_val_service.c
 *
 * Implementation of the VAL API.
 *
 * This file implements the service services.
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

#include "api_val_service.h"            // the API to implement
#include "adapter_val_internal.h"       // val_ExchangeToken()
#include "adapter_vex.h"                // VexToken_Command_t, VexToken_Result_t


/*----------------------------------------------------------------------------
 * val_ServiceRegisterRead
 */
#ifndef VAL_REMOVE_SERVICE_REGISTERREAD
ValStatus_t
val_ServiceRegisterRead(
        uint32_t         Address,
        uint32_t * const Value_p)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;

    if (Value_p == NULL)
    {
        return VAL_BAD_ARGUMENT;
    }

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_SERVICE;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_REGISTERREAD;
    t_cmd.Service.Register.Address = Address;
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;
    t_res.Service.Register.Value = 0;

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if (funcres == VAL_SUCCESS)
    {
        if (t_res.Result == VEXTOKEN_RESULT_NO_ERROR)
        {
            *Value_p = t_res.Service.Register.Value;
        }
        else
        {
            funcres = (ValStatus_t)t_res.Result;
            LOG_WARN("Abort - %s()=%d\n", __func__, t_res.Result);
        }
    }

    return funcres;
}
#endif /* !VAL_REMOVE_SERVICE_REGISTERREAD */


/*----------------------------------------------------------------------------
 * val_ServiceRegisterWrite
 */
#ifndef VAL_REMOVE_SERVICE_REGISTERWRITE
ValStatus_t
val_ServiceRegisterWrite(
        uint32_t Address,
        uint32_t Value)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;

    // Format service request
    t_cmd.OpCode  = VEXTOKEN_OPCODE_SERVICE;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_REGISTERWRITE;
    t_cmd.Service.Register.Address = Address;
    t_cmd.Service.Register.Value = Value;
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if ((funcres == VAL_SUCCESS) &&
        (t_res.Result != VEXTOKEN_RESULT_NO_ERROR))
    {
        // Error
        funcres = (ValStatus_t)t_res.Result;
        LOG_WARN("Abort - %s()=%d\n", __func__, t_res.Result);
    }

    return funcres;
}
#endif /* !VAL_REMOVE_SERVICE_REGISTERWRITE */


/*----------------------------------------------------------------------------
 * val_ServiceZeroizeMailbox
 */
#ifndef VAL_REMOVE_SERVICE_ZEROIZEMAILBOX
ValStatus_t
val_ServiceZeroizeMailbox(void)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_SERVICE;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_ZEROOUTMAILBOX;
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if ((funcres == VAL_SUCCESS) &&
        (t_res.Result != VEXTOKEN_RESULT_NO_ERROR))
    {
        // Error
        funcres = (ValStatus_t)t_res.Result;
        LOG_WARN("Abort - %s()=%d\n", __func__, t_res.Result);
    }

    return funcres;
}
#endif /* !VAL_REMOVE_SERVICE_ZEROIZEMAILBOX */


/*----------------------------------------------------------------------------
 * val_ServiceSelectOTPZeroize
 */
#ifndef VAL_REMOVE_SERVICE_ZEROIZEOTP
ValStatus_t
val_ServiceSelectOTPZeroize(void)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_SERVICE;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_SELECTOTPZERO;
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if ((funcres == VAL_SUCCESS) &&
        (t_res.Result != VEXTOKEN_RESULT_NO_ERROR))
    {
        // Error
        funcres = (ValStatus_t)t_res.Result;
        LOG_WARN("Abort - %s()=%d\n", __func__, t_res.Result);
    }

    return funcres;
}
#endif /* !VAL_REMOVE_SERVICE_ZEROIZEOTP */


/*----------------------------------------------------------------------------
 * val_ServiceZeroizeOTP
 */
#ifndef VAL_REMOVE_SERVICE_ZEROIZEOTP
ValStatus_t
val_ServiceZeroizeOTP(void)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_SERVICE;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_ZEROIZEOTP;
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if ((funcres == VAL_SUCCESS) &&
        (t_res.Result != VEXTOKEN_RESULT_NO_ERROR))
    {
        // Error
        funcres = (ValStatus_t)t_res.Result;
        LOG_WARN("Abort - %s()=%d\n", __func__, t_res.Result);
    }

    return funcres;
}
#endif /* !VAL_REMOVE_SERVICE_ZEROIZEOTP */


/*----------------------------------------------------------------------------
 * val_ServiceClockSwitch
 */
#ifndef VAL_REMOVE_CLOCKSWITCH
ValStatus_t
val_ServiceClockSwitch(
        uint16_t ClocksForcedOn,
        uint16_t ClocksForcedOff)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_SERVICE;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_CLOCKSWITCH;
    t_cmd.Service.ClockSwitch.On = ClocksForcedOn;
    t_cmd.Service.ClockSwitch.Off = ClocksForcedOff;
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if ((funcres == VAL_SUCCESS) &&
        (t_res.Result != VEXTOKEN_RESULT_NO_ERROR))
    {
        // Error
        funcres = (ValStatus_t)t_res.Result;
        LOG_WARN("Abort - %s()=%d\n", __func__, t_res.Result);
    }

    return funcres;
}
#endif /* !VAL_REMOVE_CLOCKSWITCH */


/* end of file adapter_val_service.c */
