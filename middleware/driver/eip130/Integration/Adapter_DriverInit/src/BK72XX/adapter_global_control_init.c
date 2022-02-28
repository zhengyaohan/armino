/* adapter_global_control_init.c
 *
 * Adapter module responsible for adapter global control initialization tasks.
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

#include "adapter_global_control_init.h"


/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */

// Top-level Adapter configuration
#include "c_adapter_global.h"

#ifdef ADAPTER_GLOBAL_PCI_SUPPORT_ENABLE
#include "adapter_pciconfig.h"  // PCICONFIG_*
#endif

// Logging API
#include "log.h"            // LOG_*

// Driver Framework Device API
#include "device_mgmt.h"    // Device_Initialize, Device_UnInitialize
#include "device_rw.h"      // Device_Read32, Device_Write32

// Driver Framework Basic Definitions API
#include "basic_defs.h"     // bool, true, false


/*----------------------------------------------------------------------------
 * Local variables
 */

static bool Adapter_IsInitialized = false;

#ifdef ADAPTER_GLOBAL_BOARDCTRL_SUPPORT_ENABLE
static Device_Handle_t Adapter_Device_BOARDCTRL;
#endif

#ifdef ADAPTER_GLOBAL_PCI_SUPPORT_ENABLE
static Device_Handle_t Adapter_Device_PCIConfigSpace;
#endif


/*----------------------------------------------------------------------------
 * Adapter_Global_Control_Init
 *
 * Return Value
 *     true   Success
 *     false  Failure (fatal!)
 */
bool
Adapter_Global_Control_Init(void)
{
    int nIRQ = -1;

    if (Adapter_IsInitialized != false)
    {
        LOG_WARN("Adapter_Global_Control_Init: Already initialized\n");
        return true;
    }

    // trigger first-time initialization of the adapter
    if (Device_Initialize(&nIRQ) < 0)
        return false;

#ifdef ADAPTER_GLOBAL_PCI_SUPPORT_ENABLE
    Adapter_Device_PCIConfigSpace = Device_Find("PCI_CONFIG_SPACE");
    if (Adapter_Device_PCIConfigSpace == NULL)
    {
        LOG_CRIT("Adapter_Global_Control_Init: "
                 "Failed to locate PCI_CONFIG_SPACE\n");
        return false;
    }
#endif // ADAPTER_GLOBAL_PCI_SUPPORT_ENABLE

#ifdef ADAPTER_GLOBAL_BOARDCTRL_SUPPORT_ENABLE
    Adapter_Device_BOARDCTRL = Device_Find("BOARD_CTRL");
    if (Adapter_Device_BOARDCTRL == NULL)
    {
        LOG_CRIT("Adapter_Global_Control_Init: "
                 "Failed to locate BOARD_CTRL\n");
        return false;
    }
#endif // ADAPTER_GLOBAL_BOARDCTRL_SUPPORT_ENABLE

#ifdef ADAPTER_GLOBAL_PCI_SUPPORT_ENABLE
#ifdef LOG_INFO_ENABLED
    {
        uint32_t Value;
        int VendorID, DeviceID;

        Value = Device_Read32(
                    Adapter_Device_PCIConfigSpace,
                    PCICONFIG_REG_ID);

        VendorID = PCICONFIG_ID_EXTRACT_VENDOR(Value);
        DeviceID = PCICONFIG_ID_EXTRACT_DEVICE(Value);

        IDENTIFIER_NOT_USED(VendorID);
        IDENTIFIER_NOT_USED(DeviceID);

        LOG_INFO(
            "Adapter_Global_Control_Init: "
            "PCI device: "
            "Vendor=0x%X, "
            "Device=0x%X\n",
            VendorID,
            DeviceID);
    }
#endif // LOG_INFO_ENABLED

    // initialize the PCI device
    // command and status register - Writing value 0x146 to this register
    // is recommended before accessing the FPGA
    {
        uint32_t Value;

        Value = PCICONFIG_STATCMD_MEMORYACCESS_ENABLE +
                PCICONFIG_STATCMD_BUSMASTER_ENABLE +
                PCICONFIG_STATCMD_PARITYERR_ENABLE +
                PCICONFIG_STATCMD_SYSTEMERR_ENABLE;

        Device_Write32(
                Adapter_Device_PCIConfigSpace,
                PCICONFIG_REG_STATCMD,
                Value);
    }

    // Setting cache line size
    // maintain all other bits (set by BIOS or OS)
    {
#ifdef LOG_INFO_ENABLED
        uint32_t OldValue;
#endif
        uint32_t Value;

        Value = Device_Read32(
                    Adapter_Device_PCIConfigSpace,
                    PCICONFIG_REG_CONFIG);

#ifdef LOG_INFO_ENABLED
        OldValue = Value;
#endif

        Value = PCICONFIG_CONFIG_UPDATE_CACHELINESIZE(
                    Value,
                    ADAPTER_PCICONFIG_CACHELINESIZE);

#ifdef ADAPTER_PCICONFIG_MASTERLATENCYTIMER
        Value = PCICONFIG_CONFIG_UPDATE_MASTERLATENCYTIMER(
                    Value,
                    ADAPTER_PCICONFIG_MASTERLATENCYTIMER);
#endif

        Device_Write32(
                Adapter_Device_PCIConfigSpace,
                PCICONFIG_REG_CONFIG,
                Value);

#ifdef LOG_INFO_ENABLED
        LOG_INFO(
            "Adapter_Global_Control_Init: "
            "Changed PCI_Config[0x0c] "
            "from 0x%08x "
            "to 0x%08x\n",
            OldValue,
            Value);
#endif
    }
#endif // ADAPTER_GLOBAL_PCI_SUPPORT_ENABLE

    // FPGA board specific functionality
#ifdef ADAPTER_GLOBAL_BOARDCTRL_SUPPORT_ENABLE
    {
        // Enable PLB access on the board
        Device_Write32(Adapter_Device_BOARDCTRL, 0x8, 0x00400000);

#ifdef ADAPTER_GLOBAL_FPGA_HW_RESET_ENABLE
        // Perform HW Reset for the FPGA board
        Device_Write32(Adapter_Device_BOARDCTRL, 0x2000, 0);
        Device_Write32(Adapter_Device_BOARDCTRL, 0x2000, 0xFFFFFFFF);
        Device_Write32(Adapter_Device_BOARDCTRL, 0x2000, 0);
#endif // ADAPTER_GLOBAL_FPGA_HW_RESET_ENABLE
    }
#endif // ADAPTER_GLOBAL_BOARDCTRL_SUPPORT_ENABLE

    Adapter_IsInitialized = true;

    return true;    // success
}


/*----------------------------------------------------------------------------
 * Adapter_Global_Control_UnInit
 */
void
Adapter_Global_Control_UnInit(void)
{
    if (!Adapter_IsInitialized)
    {
        LOG_WARN("Adapter_Global_Control_UnInit: Adapter is uninitialized\n");
        return;
    }

    Adapter_IsInitialized = false;

    Device_UnInitialize();
}


/*----------------------------------------------------------------------------
 * Adapter_Global_Control_Report_Build_Params
 */
void
Adapter_Global_Control_Report_Build_Params(void)
{
#ifdef LOG_INFO_ENABLED
    int dummy;

    // This function is dependent on config file cs_adapter.h.
    // Please update this when Config file for Adapter is changed.
    Log_FormattedMessage("Adapter Global Control build configuration:\n");

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

#ifdef ADAPTER_64BIT_HOST
    REPORT_EXPL(ADAPTER_64BIT_HOST,
                " is SET => addresses are 64-bit");
#else
    REPORT_EXPL(ADAPTER_64BIT_HOST,
                " is NOT set => addresses are 32-bit");
#endif

    // PCI configuration
#ifdef ADAPTER_GLOBAL_PCI_SUPPORT_ENABLE
#ifdef ADAPTER_PCICONFIG_CACHELINESIZE
    REPORT_INT(ADAPTER_PCICONFIG_CACHELINESIZE);
#endif

#ifdef ADAPTER_PCICONFIG_MASTERLATENCYTIMER
    REPORT_INT(ADAPTER_PCICONFIG_MASTERLATENCYTIMER);
#endif
#endif // ADAPTER_GLOBAL_PCI_SUPPORT_ENABLE

    // Global interrupts
#ifdef ADAPTER_GLOBAL_INTERRUPTS_TRACEFILTER
    REPORT_INT(ADAPTER_GLOBAL_INTERRUPTS_TRACEFILTER);
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


    IDENTIFIER_NOT_USED(dummy);

    // Adapter other
    Log_FormattedMessage("Other:\n");
    REPORT_STR(ADAPTER_GLOBAL_DRIVER_NAME);
    REPORT_STR(ADAPTER_GLOBAL_LICENSE);

#endif //LOG_INFO_ENABLED
}


/* end of file adapter_global_control_init.c */
