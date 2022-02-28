/* adapter_val_exchangetoken.c
 *
 * Implementation of the VAL API.
 *
 * This file contains the token exchange with the next driver level.
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

#include "basic_defs.h"                 // uint16_t

#if 0    ////Debug
  #undef LOG_SEVERITY_MAX						////Debug only
  #define LOG_SEVERITY_MAX  LOG_SEVERITY_INFO	////Debug only
#endif
#include "log.h"

#include "adapter_val_internal.h"       // the API to implement
#include "adapter_val_internal_ext.h"   // User or Kernel mode indication
#include "adapter_vex.h"                // VexStatus_t, vex_ExchangeToken()


/*----------------------------------------------------------------------------
 * val_ExchangeToken
 *
 * Allocate an asset and set its policy. Its content is setup later.
 */
ValStatus_t
val_ExchangeToken(
        VexToken_Command_t * const CommandToken_p,
        VexToken_Result_t * const ResultToken_p)
{
    VexStatus_t funcres;

#ifdef VAL_STRICT_ARGS
    // In principle an internal function, so do not check arguments
    //if (CommandToken_p == NULL ||
    //    ResultToken_p == NULL)
    //{
    //    return VAL_INTERNAL_ERROR;
    //}
#endif

    CommandToken_p->fFromUserSpace = ADAPTER_VAL_INTERNAL_USER_SPACE_FLAG;
    funcres = vex_LogicalToken(CommandToken_p, ResultToken_p);
    if (funcres != VEX_SUCCESS)
    {
        LOG_WARN("%s::vex_LogicalToken()=%d\n", __func__, funcres);
        if ((funcres == VEX_REPONSE_TIMEOUT) ||
            (funcres == VEX_DATA_TIMEOUT))
        {
            return VAL_TIMEOUT_ERROR;
        }
        return VAL_INTERNAL_ERROR;
    }

    return VAL_SUCCESS;
}


/* end of file adapter_val_exchangetoken.c */
