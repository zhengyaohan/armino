/* adapter_vex_sf.c
 *
 * Implementation of the VEX API.
 *
 * This file implements the main entry for the special functions services.
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

#include "basic_defs.h"
#include "clib.h"
#include "log.h"

#include "adapter_vex_intern_sf.h"   // API implementation


/*----------------------------------------------------------------------------
 * vex_SpecialFunctions
 */
VexStatus_t
vex_SpecialFunctions(
        VexToken_Command_t * const CommandToken_p,
        VexToken_Result_t * const ResultToken_p)
{
    VexStatus_t funcres = VEX_UNSUPPORTED;

    switch (CommandToken_p->SubCode)
    {
#ifdef VEX_ENABLE_SF_MILENAGE
    case VEXTOKEN_SUBCODE_SF_MILENAGE:
        funcres = vex_SF_Milenage(CommandToken_p, ResultToken_p);
        break;
#endif

#ifdef VEX_ENABLE_SF_BLUETOOTH
    case VEXTOKEN_SUBCODE_SF_BLUETOOTH:
        funcres = vex_SF_BluetoothLE_f5(CommandToken_p, ResultToken_p);
        break;
#endif

#ifdef VEX_ENABLE_SF_COVERAGE
    case VEXTOKEN_SUBCODE_SF_COVERAGE:
        funcres = vex_SF_Coverage(CommandToken_p, ResultToken_p);
        break;
#endif

    default:
#ifdef VEX_ENABLE_SF_PROTECTED_APP
        if ((CommandToken_p->SubCode >= VEXTOKEN_SUBCODE_SF_MIN_PROT_APP_ID) &&
            (CommandToken_p->SubCode <= VEXTOKEN_SUBCODE_SF_MAX_PROT_APP_ID))
        {
            funcres = vex_SF_ProtectedApp(CommandToken_p, ResultToken_p);
        }
#endif
        break;
    }

    return funcres;
}


/* end of file adapter_vex_sf.c */
