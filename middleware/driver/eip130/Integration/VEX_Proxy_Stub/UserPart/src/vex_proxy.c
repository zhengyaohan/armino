/* vex_proxy.c
 *
 * Implementation of the VEX API Proxy.
 *
 */

/*****************************************************************************
* Copyright (c) 2014-2019 INSIDE Secure B.V. All Rights Reserved.
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
 * This module implements (provides) the following interface(s):
 */

// VEX Adapter API
#include "adapter_vex.h"


/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */

// Default configuration
#include "c_vex_proxy.h"

// UMDevXS Command API
#include "umdevxs_cmd.h"        // UMDevXS_CmdRsp_t

// VEX Command API
#include "vex_cmd.h"            // VEX_OPCODE_*, vexStub_*

#include <fcntl.h>              // open, O_RDWR
#include <unistd.h>             // close, write
#include <string.h>             // memset


/*----------------------------------------------------------------------------
 * Definitions and macros
 */

#define ZEROINIT(_x)  memset(&_x, 0, sizeof(_x))


/*----------------------------------------------------------------------------
 * Global constants
 */

static const char vexProxy_NodeName[] = VEX_PROXY_NODE_NAME;


/*----------------------------------------------------------------------------
 * Local variables
 */

// character device file descriptor
static int vexProxy_fd = -1;


/*----------------------------------------------------------------------------
 * Local prototypes
 */



/*----------------------------------------------------------------------------
 * vex_LogicalToken
 */
VexStatus_t
vex_LogicalToken(
        VexToken_Command_t * const CommandToken_p,
        VexToken_Result_t * const ResultToken_p)
{
    int res;
    UMDevXS_CmdRsp_t CmdRsp;
    vexStub_DataIO_t DataIO;

    if ((CommandToken_p == NULL) ||
        (ResultToken_p == NULL))
    {
        return VEX_BAD_ARGUMENT;
    }

    // Try to open the character device
    if (vexProxy_fd < 0)
    {
        vexProxy_fd = open(vexProxy_NodeName, O_RDWR);
        if (vexProxy_fd < 0)
        {
            return VEX_NOT_CONNECTED;
        }
    }

    // Indicate call is from User Space
    CommandToken_p->fFromUserSpace = true;

    // Prepare input data
    DataIO.InData_p         = CommandToken_p;
    DataIO.InDataByteCount  = sizeof(VexToken_Command_t);
    DataIO.OutData_p        = ResultToken_p;
    DataIO.OutDataByteCount = sizeof(VexToken_Result_t);

    // zero-init also protects against future extensions
    ZEROINIT(CmdRsp);

    CmdRsp.Magic  = UMDEVXS_CMDRSP_MAGIC;
    CmdRsp.Opcode = VEX_OPCODE_TOKEN;
    CmdRsp.ptr1   = &DataIO;
    CmdRsp.uint1  = sizeof(vexStub_DataIO_t);

    // Hand-over the call to Kernel Space with write()
    // - write() is a blocking call
    // - it takes the pointer to the CmdRsp structure
    // - processes the CmdRsp structure and fills it with the result
    res = (int)write(vexProxy_fd, &CmdRsp, sizeof(UMDevXS_CmdRsp_t));
    if (res != sizeof(UMDevXS_CmdRsp_t))
    {
        return VEX_INTERNAL_ERROR;
    }

    // Return the VEX result
    return CmdRsp.Error;
}


/* end of file vex_proxy.c */
