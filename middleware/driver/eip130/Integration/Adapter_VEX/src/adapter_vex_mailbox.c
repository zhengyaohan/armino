/* adapter_vex_mailbox.c
 *
 * Implementation of the VEX API.
 *
 * This file implements the mailbox related functionality.
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

#include "adapter_vex_internal.h"   // API implementation


/*----------------------------------------------------------------------------
 * vex_MailboxGet
 */
uint8_t
vex_MailboxGet(
        uint32_t Identity)
{
    IDENTIFIER_NOT_USED(Identity);

    // Future extension: Link mailbox based on identity.

    return VEX_MAILBOX_NR;
}


/* end of file adapter_vex_mailbox.c */
