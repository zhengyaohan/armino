/* adapter_vex_hardware.c
 *
 * Implementation of the VEX API.
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

#include "c_adapter_vex.h"          // configuration

#ifdef VEX_ENABLE_HW_FUNCTIONS

#include "basic_defs.h"
#include "clib.h"
#include "log.h"

#include "adapter_vex_internal.h"   // API implementation
#define EIP130_REMOVE_READMAILBOXSTATUS     // Remove unused function warnings
#define EIP130_REMOVE_WRITEMAILBOXCONTROL
#define EIP130_REMOVE_MAILBOXRAWSTATUS
#define EIP130_REMOVE_MAILBOXRESET
#define EIP130_REMOVE_MAILBOXLINKRESET
#define EIP130_REMOVE_MAILBOXLINKID
#define EIP130_REMOVE_MAILBOXOUTID
#define EIP130_REMOVE_MAILBOXACCESSCONTROL
#define EIP130_REMOVE_MAILBOXACCESSVERIFY
#define EIP130_REMOVE_MODULEFIRMWAREWRITTEN
#define EIP130_REMOVE_FIRMWAREDOWNLOAD
#include "eip130_level0.h"


/*----------------------------------------------------------------------------
 * vex_Hardware
 */
VexStatus_t
vex_Hardware(
        VexToken_Command_t * const CommandToken_p,
        VexToken_Result_t * const ResultToken_p)
{
    Device_Handle_t Device;
    uint32_t Value;

    Device = vex_DeviceGetHandle();
    if (Device == NULL)
    {
        return VEX_NOT_CONNECTED;
    }

    // Format result token
    switch (CommandToken_p->SubCode)
    {
    case VEXTOKEN_SUBCODE_MODULESTATUS:
        Value = EIP130_RegisterReadModuleStatus(Device);
        ResultToken_p->Service.ModuleStatus.FIPSmode = (uint8_t)(MASK_1_BIT & (Value >> 0));
        ResultToken_p->Service.ModuleStatus.NonFIPSmode = (uint8_t)(MASK_1_BIT & (Value >> 1));
        ResultToken_p->Service.ModuleStatus.CRC24Busy = (uint8_t)(MASK_1_BIT & (Value >> 8));
        ResultToken_p->Service.ModuleStatus.CRC24Ok = (uint8_t)(MASK_1_BIT & (Value >> 9));
        ResultToken_p->Service.ModuleStatus.CRC24Error = (uint8_t)(MASK_1_BIT & (Value >> 10));
        ResultToken_p->Service.ModuleStatus.FwImageWritten = (uint8_t)(MASK_1_BIT & (Value >> 20));
        ResultToken_p->Service.ModuleStatus.FwImageCheckDone = (uint8_t)(MASK_1_BIT & (Value >> 22));
        ResultToken_p->Service.ModuleStatus.FwImageAccepted = (uint8_t)(MASK_1_BIT & (Value >> 23));
        ResultToken_p->Service.ModuleStatus.FatalError = (uint8_t)(MASK_1_BIT & (Value >> 31));
        break;

    case VEXTOKEN_SUBCODE_EIP_OPTIONS:
        Value = EIP130_RegisterReadOptions(Device);
        ResultToken_p->Service.EipOptions.nMailboxes = (uint8_t)(MASK_4_BITS & (Value >> 0));
        ResultToken_p->Service.EipOptions.MailboxSize = (uint16_t)(0x80 << (MASK_2_BITS & (Value >> 4)));
        ResultToken_p->Service.EipOptions.HostId = (uint8_t)(MASK_8_BITS & (Value >> 8));
        ResultToken_p->Service.EipOptions.MasterId = (uint8_t)(MASK_3_BITS & (Value >> 16));
        ResultToken_p->Service.EipOptions.ProtectionAvailable = (uint8_t)(MASK_1_BIT & (Value >> 19));
        ResultToken_p->Service.EipOptions.MyHostId = (uint8_t)(MASK_3_BITS & (Value >> 20));
        ResultToken_p->Service.EipOptions.Protection = (uint8_t)(MASK_1_BIT & (Value >> 23));
        ResultToken_p->Service.EipOptions.SecureHostId = (uint8_t)(MASK_8_BITS & (Value >> 24));

        Value = EIP130_RegisterReadOptions2(Device);
        ResultToken_p->Service.EipOptions.StandardEngines = (uint16_t)(MASK_16_BITS & (Value >> 0));
        ResultToken_p->Service.EipOptions.CustomEngines = (uint16_t)(MASK_16_BITS & (Value >> 16));
        break;

    case VEXTOKEN_SUBCODE_EIP_VERSION:
        Value = EIP130_RegisterReadVersion(Device);
        ResultToken_p->Service.EipVersion.EipNumber = (uint8_t)(MASK_8_BITS & (Value >> 0));
        ResultToken_p->Service.EipVersion.PatchLevel = (uint8_t)(MASK_4_BITS & (Value >> 16));
        ResultToken_p->Service.EipVersion.MinorVersion = (uint8_t)(MASK_4_BITS & (Value >> 20));
        ResultToken_p->Service.EipVersion.MajorVersion = (uint8_t)(MASK_4_BITS & (Value >> 24));
        break;

    default:
        return VEX_UNSUPPORTED;
    }

    ResultToken_p->Result = 0;
    return VEX_SUCCESS;
}

#endif /* VEX_ENABLE_HW_FUNCTIONS */

/* end of file adapter_vex_hardware.c */
