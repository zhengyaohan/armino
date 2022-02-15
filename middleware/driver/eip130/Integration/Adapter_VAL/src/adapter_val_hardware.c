/* adapter_val_hardware.c
 *
 * Implementation of the VAL API.
 *
 * This file implements the hardware registers related services.
 */

/*****************************************************************************
* Copyright (c) 2017-2018 INSIDE Secure B.V. All Rights Reserved.
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

#include "api_val.h"                    // the API to implement
#include "adapter_val_internal.h"       // val_ExchangeToken()
#include "adapter_vex.h"                // VexToken_Command_t, VexToken_Result_t


/*----------------------------------------------------------------------------
 * val_HW_ModuleStatus
 */
#ifndef VAL_REMOVE_HARDWARE_MODULESTATUS
ValStatus_t
val_HW_ModuleStatus(
        uint8_t * const FIPSmode_p,
        uint8_t * const NonFIPSmode_p,
        uint8_t * const FatalError_p,
        uint8_t * const CRC24Ok_p,
        uint8_t * const CRC24Busy_p,
        uint8_t * const CRC24Error_p,
        uint8_t * const FwImageWritten_p,
        uint8_t * const FwImageCheckDone_p,
        uint8_t * const FwImageAccepted_p)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_HARDWARE;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_MODULESTATUS;
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;
    ZEROINIT(t_res.Service.ModuleStatus);

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if (funcres == VAL_SUCCESS)
    {
        if (t_res.Result == VEXTOKEN_RESULT_NO_ERROR)
        {
            // return requested information
            if (FIPSmode_p != NULL)
            {
                *FIPSmode_p = t_res.Service.ModuleStatus.FIPSmode;
            }
            if (NonFIPSmode_p != NULL)
            {
                *NonFIPSmode_p = t_res.Service.ModuleStatus.NonFIPSmode;
            }
            if (CRC24Ok_p != NULL)
            {
                *CRC24Ok_p = t_res.Service.ModuleStatus.CRC24Ok;
            }
            if (CRC24Busy_p != NULL)
            {
                *CRC24Busy_p = t_res.Service.ModuleStatus.CRC24Busy;
            }
            if (CRC24Error_p != NULL)
            {
                *CRC24Error_p = t_res.Service.ModuleStatus.CRC24Error;
            }
            if (FwImageWritten_p != NULL)
            {
                *FwImageWritten_p = t_res.Service.ModuleStatus.FwImageWritten;
            }
            if (FwImageCheckDone_p != NULL)
            {
                *FwImageCheckDone_p = t_res.Service.ModuleStatus.FwImageCheckDone;
            }
            if (FwImageAccepted_p != NULL)
            {
                *FwImageAccepted_p = t_res.Service.ModuleStatus.FwImageAccepted;
            }
            if (FatalError_p != NULL)
            {
                *FatalError_p = t_res.Service.ModuleStatus.FatalError;
            }
         }
        else
        {
            funcres = (ValStatus_t)t_res.Result;
            LOG_WARN("Abort - %s()=%d\n", __func__, t_res.Result);
        }
    }

    return funcres;
}
#endif /* !VAL_REMOVE_HARDWARE_MODULESTATUS */


/*----------------------------------------------------------------------------
 * val_HW_EIP_Options
 */
#ifndef VAL_REMOVE_HARDWARE_EIP_OPTIONS
ValStatus_t
val_HW_EIP_Options(
        uint8_t * const nMailboxes_p,
        uint16_t * const MailboxSize_p,
        uint8_t * const HostId_p,
        uint8_t * const SecureHostId_p,
        uint8_t * const MasterId_p,
        uint8_t * const MyHostId_p,
        uint8_t * const ProtectionAvailable_p,
        uint8_t * const Protection_p,
        uint16_t * const StandardEngines_p,
        uint16_t * const CustomEngines_p)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_HARDWARE;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_EIP_OPTIONS;
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;
    ZEROINIT(t_res.Service.EipOptions);

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if (funcres == VAL_SUCCESS)
    {
        if (t_res.Result == VEXTOKEN_RESULT_NO_ERROR)
        {
            // return requested information
            if (nMailboxes_p != NULL)
            {
                *nMailboxes_p = t_res.Service.EipOptions.nMailboxes;
            }
            if (MailboxSize_p != NULL)
            {
                *MailboxSize_p = t_res.Service.EipOptions.MailboxSize;
            }
            if (HostId_p != NULL)
            {
                *HostId_p = t_res.Service.EipOptions.HostId;
            }
            if (SecureHostId_p != NULL)
            {
                *SecureHostId_p = t_res.Service.EipOptions.SecureHostId;
            }
            if (MasterId_p != NULL)
            {
                *MasterId_p = t_res.Service.EipOptions.MasterId;
            }
            if (MyHostId_p != NULL)
            {
                *MyHostId_p = t_res.Service.EipOptions.MyHostId;
            }
            if (ProtectionAvailable_p != NULL)
            {
                *ProtectionAvailable_p = t_res.Service.EipOptions.ProtectionAvailable;
            }
            if (Protection_p != NULL)
            {
                *Protection_p = t_res.Service.EipOptions.Protection;
            }
            if (StandardEngines_p != NULL)
            {
                *StandardEngines_p = t_res.Service.EipOptions.StandardEngines;
            }
            if (CustomEngines_p != NULL)
            {
                *CustomEngines_p = t_res.Service.EipOptions.CustomEngines;
            }
         }
        else
        {
            funcres = (ValStatus_t)t_res.Result;
            LOG_WARN("Abort - %s()=%d\n", __func__, t_res.Result);
        }
    }

    return funcres;
}
#endif /* !VAL_REMOVE_HARDWARE_EIP_OPTIONS */


/*----------------------------------------------------------------------------
 * val_HW_EIP_Version
 */
#ifndef VAL_REMOVE_HARDWARE_EIP_VERSION
ValStatus_t
val_HW_EIP_Version(
        uint8_t * const MajorVersion_p,
        uint8_t * const MinorVersion_p,
        uint8_t * const PatchLevel_p,
        uint8_t * const EipNumber_p)
{
    VexToken_Command_t t_cmd;
    VexToken_Result_t t_res;
    ValStatus_t funcres;

    // Format service request
    t_cmd.OpCode = VEXTOKEN_OPCODE_HARDWARE;
    t_cmd.SubCode = VEXTOKEN_SUBCODE_EIP_VERSION;
    t_res.Result = VEXTOKEN_RESULT_NO_ERROR;
    ZEROINIT(t_res.Service.EipVersion);

    // Exchange service request with the next driver level
    funcres = val_ExchangeToken(&t_cmd, &t_res);
    if (funcres == VAL_SUCCESS)
    {
        if (t_res.Result == VEXTOKEN_RESULT_NO_ERROR)
        {
            // return requested information
            if (MajorVersion_p != NULL)
            {
                *MajorVersion_p = t_res.Service.EipVersion.MajorVersion;
            }
            if (MinorVersion_p != NULL)
            {
                *MinorVersion_p = t_res.Service.EipVersion.MinorVersion;
            }
            if (PatchLevel_p != NULL)
            {
                *PatchLevel_p = t_res.Service.EipVersion.PatchLevel;
            }
            if (EipNumber_p != NULL)
            {
                *EipNumber_p = t_res.Service.EipVersion.EipNumber;
            }
        }
        else
        {
            funcres = (ValStatus_t)t_res.Result;
            LOG_WARN("Abort - %s()=%d\n", __func__, t_res.Result);
        }
    }

    return funcres;
}
#endif /* !VAL_REMOVE_HARDWARE_EIP_VERSION */


/* end of file adapter_val_hardware.c */
