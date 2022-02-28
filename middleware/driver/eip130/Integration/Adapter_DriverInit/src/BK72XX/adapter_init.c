/* adapter_init.c
 *
 * Adapter module responsible for adapter initialization tasks.
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
 * This module implements (provides) the following interface(s):
 */

#include "adapter_init.h"


/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */

// Top-level Adapter configuration
#include "cs_adapter.h"

#ifdef ADAPTER_EIP130_INTERRUPTS_ENABLE
#include "adapter_interrupts.h"         // Adapter_Interrupts_Init/UnInit
#endif

// Logging API
#include "log.h"                        // LOG_*

// Driver Framework Device API
#include "device_mgmt.h"                // Device_Initialize/UnInitialize
#include "device_rw.h"                  // Device_Read32, Device_Write32

// Driver Framework DMAResource API
#include "dmares_mgmt.h"                // DMAResource_Init, DMAResource_UnInit

// Driver Framework Basic Definitions API
#include "basic_defs.h"                 // bool, true, false
// Driver hal
#include "vault_hal.h"
/*----------------------------------------------------------------------------
 * Local variables
 */
typedef struct {
	vault_hal_t hal;
	bool is_initied;
} vault_driver_t;

static vault_driver_t s_vault = {0};
static int Device_IRQ;

void secure_vault_init(void)
{
	LOG_CRIT("secure vault Build @ %s %s \r\n", __TIME__, __DATE__);
	*((volatile unsigned long *) (0x44010000+0xc*4))  |= 0x00008000;// bit[15] is vault clk gate 
	//*((volatile unsigned long *) (0x44010000+0x10*4)) |= 0x00000008;// bit[3] is vault power enable;
	vault_hal_start_common(&s_vault.hal);
}


/*----------------------------------------------------------------------------
 * Adapter_Init
 *
 * Return Value
 *     true   Success
 *     false  Failure (fatal!)
 */
bool
Adapter_Init(void)
{
    Device_IRQ = -1;
	vault_hal_init(&s_vault.hal);

    if (s_vault.is_initied != false)
    {
        LOG_WARN("Adapter_Init: Already initialized\n");
        return true;
    }

	secure_vault_init();
	
    // trigger first-time initialization of the adapter
    if (Device_Initialize(&Device_IRQ) < 0)
        return false;

    if (!DMAResource_Init())
    {
        Device_UnInitialize();
        return false;
    }



    s_vault.is_initied = true;

    return true;    // success
}


/*----------------------------------------------------------------------------
 * Adapter_UnInit
 */
void
Adapter_UnInit(void)
{
    if (!s_vault.is_initied)
    {
        LOG_WARN("Adapter_UnInit: Adapter is not initialized\n");
        return;
    }

    s_vault.is_initied = false;

    DMAResource_UnInit();


    Device_UnInitialize();
}


/*----------------------------------------------------------------------------
 * Adapter_Report_Build_Params
 */
void
Adapter_Report_Build_Params(void)
{
#ifdef LOG_INFO_ENABLED
    int dummy;

    // This function is dependent on config file cs_adapter.h.
    // Please update this when Config file for Adapter is changed.
    Log_FormattedMessage("Adapter build configuration:\n");

#define REPORT_SET(_X) \
    Log_FormattedMessage("\t" #_X "\n")

#define REPORT_STR(_X) \
    Log_FormattedMessage("\t" #_X ": %s\n", _X)

#define REPORT_INT(_X) \
    dummy = _X; Log_FormattedMessage("\t" #_X ": %d\n", _X)

#define REPORT_HEX32(_X) \
    dummy = _X; Log_FormattedMessage("\t" #_X ": 0x%08X\n", _X)

#define REPORT_EQ(_X, _Y) \
    dummy = (_X + _Y); Log_FormattedMessage("\t" #_X " == " #_Y "\n")

#define REPORT_EXPL(_X, _Y) \
    Log_FormattedMessage("\t" #_X _Y "\n")

    // Adapter VAL
#ifdef ADAPTER_STRICT_ARGS
    REPORT_SET(ADAPTER_STRICT_ARGS);
#endif

#ifdef ADAPTER_REMOVE_BOUNCEBUFFERS
    REPORT_EXPL(ADAPTER_REMOVE_BOUNCEBUFFERS, " is SET => Bounce DISABLED");
#else
    REPORT_EXPL(ADAPTER_REMOVE_BOUNCEBUFFERS, " is NOT set => Bounce ENABLED");
#endif

#ifdef ADAPTER_ENABLE_SWAP
    REPORT_EXPL(ADAPTER_ENABLE_SWAP, " is SET => Byte swap in words ENABLED");
#else
    REPORT_EXPL(ADAPTER_ENABLE_SWAP, " is NOT set => Byte swap in words DISABLED");
#endif

#ifdef ADAPTER_EIP130_INTERRUPTS_ENABLE
    REPORT_EXPL(ADAPTER_EIP130_INTERRUPTS_ENABLE,
            " is SET => Interrupts ENABLED");
#else
    REPORT_EXPL(ADAPTER_EIP130_INTERRUPTS_ENABLE,
            " is NOT set => Interrupts DISABLED");
#endif

#ifdef ADAPTER_64BIT_HOST
    REPORT_EXPL(ADAPTER_64BIT_HOST,
                " is SET => addresses are 64-bit");
#else
    REPORT_EXPL(ADAPTER_64BIT_HOST,
                " is NOT set => addresses are 32-bit");
#endif

#ifdef ADAPTER_64BIT_DEVICE
    REPORT_EXPL(ADAPTER_64BIT_DEVICE,
                " is SET => full 64-bit DMA addresses usable");
#else
    REPORT_EXPL(ADAPTER_64BIT_DEVICE,
                " is NOT set => DMA addresses must be below 4GB");
#endif

#ifdef ADAPTER_GLOBAL_PCI_SUPPORT_ENABLE
    REPORT_SET(ADAPTER_GLOBAL_PCI_SUPPORT_ENABLE);
#endif

#ifdef ADAPTER_GLOBAL_BOARDCTRL_SUPPORT_ENABLE
    REPORT_SET(ADAPTER_GLOBAL_BOARDCTRL_SUPPORT_ENABLE);
#endif

    // Log
    Log_FormattedMessage("Logging:\n");

#if (LOG_SEVERITY_MAX == LOG_SEVERITY_INFO)
    REPORT_EQ(LOG_SEVERITY_MAX, LOG_SEVERITY_INFO);
#elif (LOG_SEVERITY_MAX == LOG_SEVERITY_WARNING)
    REPORT_EQ(LOG_SEVERITY_MAX, LOG_SEVERITY_WARNING);
#elif (LOG_SEVERITY_MAX == LOG_SEVERITY_CRITICAL)
    REPORT_EQ(LOG_SEVERITY_MAX, LOG_SEVERITY_CRITICAL);
#else
    REPORT_EXPL(LOG_SEVERITY_MAX, " - Unknown (not info/warn/crit)");
#endif

    // Adapter other
    Log_FormattedMessage("Other:\n");
    REPORT_STR(ADAPTER_DRIVER_NAME);

    IDENTIFIER_NOT_USED(dummy);

#endif //LOG_INFO_ENABLED
}


/* end of file adapter_init.c */