/* vex_stub.c
 *
 * Implementation of the VEX API Stub.
 *
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


/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */

// UMDevXS Command API
#include "umdevxs_cmd.h"        // UMDevXS_CmdRsp_t

// VEX Command API
#include "vex_cmd.h"            // VEX_OPCODE_*, vexStub_*

// VEX Adapter API
#include "adapter_vex.h"

// Default configuration
#include "c_vex_stub.h"

// Logging API
#include "log.h"

// Driver Framework C Run-Time Library API
//#include "clib.h"

// Linux kernel API
#include <linux/uaccess.h>          // copy_to/from_user, access_ok
#include <linux/errno.h>            // EIO


/*----------------------------------------------------------------------------
 * Local vexStub_HandleCmd
 */
int
vexStub_HandleCmd(
        void * AppID,
        UMDevXS_CmdRsp_t * const CmdRsp_p)
{
    VexToken_Command_t CommandToken;
    VexToken_Result_t ResultToken;
    vexStub_DataIO_t DataIO;

    if (CmdRsp_p->uint1 != sizeof(vexStub_DataIO_t))
    {
        LOG_CRIT(VEX_STUB_LOG_PREFIX
                 "%s(): failed, data size mismatch %d!=%d\n",
                 __func__,
                 CmdRsp_p->uint1,
                 (int)sizeof(vexStub_DataIO_t));
        return -EINVAL;     // ## RETURN ##
    }

    if (!access_ok(VERIFY_WRITE, CmdRsp_p->ptr1, sizeof(vexStub_DataIO_t)))
    {
        LOG_CRIT(VEX_STUB_LOG_PREFIX
                 "%s(): failed on access_ok\n",
                 __func__);
        return -EINVAL;     // ## RETURN ##
    }

    // copy the DataIO from application space into kernel space
    if (copy_from_user(&DataIO,
                       CmdRsp_p->ptr1,
                       sizeof(vexStub_DataIO_t)) != 0)
    {
        LOG_CRIT(VEX_STUB_LOG_PREFIX
                 "%s(): failed on copy_from_user\n",
                 __func__);
        return -EINVAL;     // ## RETURN ##
    }

    if (!access_ok(VERIFY_WRITE, DataIO.InData_p, DataIO.InDataByteCount) ||
        DataIO.InDataByteCount != sizeof(VexToken_Command_t))
    {
        LOG_CRIT(VEX_STUB_LOG_PREFIX
                 "%s(): failed on access_ok or data size mismatch %d/%d\n",
                 __func__,
                 DataIO.InDataByteCount,
                 (unsigned int)sizeof(VexToken_Command_t));
        return -EINVAL;     // ## RETURN ##
    }

    // copy the CommandToken from application space into kernel space
    if (copy_from_user(&CommandToken,
                       DataIO.InData_p,
                       sizeof(VexToken_Command_t)) != 0)
    {
        LOG_CRIT(VEX_STUB_LOG_PREFIX
                 "%s(): failed on copy_from_user\n",
                 __func__);
        return -EINVAL;     // ## RETURN ##
    }

    switch (CmdRsp_p->Opcode)
    {
    case VEX_OPCODE_TOKEN:
        CmdRsp_p->Error = vex_LogicalToken(&CommandToken, &ResultToken);
        if (CmdRsp_p->Error >= 0)
        {
            char * writebuf_p = (char *)&ResultToken;

            if (!access_ok(VERIFY_WRITE,
                           DataIO.OutData_p,
                           DataIO.OutDataByteCount) ||
                DataIO.OutDataByteCount != sizeof(VexToken_Result_t))
            {
                LOG_CRIT(VEX_STUB_LOG_PREFIX
                         "%s(): "
                         "failed on access_ok or data size mismatch %d/%d\n",
                         __func__,
                         DataIO.OutDataByteCount,
                         (unsigned int)sizeof(VexToken_Result_t));
                return -EINVAL;
            }

            if (copy_to_user(DataIO.OutData_p,
                             writebuf_p,
                             sizeof(VexToken_Result_t)) != 0)
            {
                LOG_CRIT(VEX_STUB_LOG_PREFIX
                         "%s: failed on copy_to_user\n",
                         __func__);
                return -EINVAL;     // ## RETURN ##
            }
        }
        break;

    default:
        LOG_CRIT(VEX_STUB_LOG_PREFIX
                 "%s(): unsupported opcode %d\n",
                __func__,
                 CmdRsp_p->Opcode);
        return -EINVAL;     // ## RETURN ##
    } // switch

    return 0;
}


/* end of file vex_stub.c */
