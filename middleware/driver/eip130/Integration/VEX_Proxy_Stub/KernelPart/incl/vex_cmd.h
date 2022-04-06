/* vex_cmd.h
 *
 * Driver Command/Response data structure.
 * Used on character device interface between Vex Proxy and Stub.
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

#ifndef INCLUDE_GUARD_VEX_CMD_H
#define INCLUDE_GUARD_VEX_CMD_H

/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */

// UMDevXS Command API
#include "umdevxs_cmd.h"        // UMDevXS_CmdRsp_t


/*----------------------------------------------------------------------------
 * Definitions and macros
 */

enum
{
    VEX_OPCODE_TOKEN = UMDEVXS_OPCODE_LAST + 1,

    VEX_OPCODE_LAST,
};


typedef struct
{
    // Pointer to input data
    void * InData_p;

    // Input Data byte count
    unsigned int InDataByteCount;

    // Pointer to input data
    void * OutData_p;

    // Output Data byte count
    unsigned int OutDataByteCount;

} vexStub_DataIO_t;


/*----------------------------------------------------------------------------
 * Local vexStub_HandleCmd
 */
int
vexStub_HandleCmd(
        void * AppID,
        UMDevXS_CmdRsp_t * const CmdRsp_p);


#endif /* INCLUDE_GUARD_VEX_CMD_H */


/* vex_cmd.h */
