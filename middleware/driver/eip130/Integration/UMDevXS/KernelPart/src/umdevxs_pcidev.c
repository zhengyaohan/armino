/* umdevxs_pcidev.c
 *
 * PCI device support for the Linux UMDevXS driver.
 */

/*****************************************************************************
* Copyright (c) 2009-2016 INSIDE Secure B.V. All Rights Reserved.
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

#ifndef UMDEVXS_REMOVE_PCI

/*----------------------------------------------------------------------------
 * This module implements (provides) the following interface(s):
 */
#include "umdevxs_internal.h"

#ifdef UMDEVXS_ENABLE_KERNEL_SUPPORT
#include "umdevxs_pcidev.h"     // UMDevXS_PCIDev_Get
#endif


/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */

// Default configuration
#include "c_umdevxs.h"          // config options

// Linux Kernel Module interface
#include "lkm.h"                // LKM_*

// Linux Kernel API
#include <linux/pci.h>          // pci_*, DEVICE_COUNT_RESOURCE
#include <linux/mm.h>           // remap_pfn_range

// Driver Framework C Run-Time Library API
#include "clib.h"               // ZEROINIT

// Driver Framework Basic Defs API
#include "basic_defs.h"         // uint8_t, IDENTIFIER_NOT_USED, bool

#ifdef UMDEVXS_USE_RPM
// Runtime Power Management Kernel Macros API
#include "rpm_kernel_macros.h"  // RPM_*
#endif


/*----------------------------------------------------------------------------
 * Definitions and macros
 */


/*----------------------------------------------------------------------------
 * Local variables
 */


/*----------------------------------------------------------------------------
 * Declarations related to the PCI driver
 */


/*----------------------------------------------------------------------------
 * UMDevXS_PCIDev_Map
 *
 * This function is called to request a PCI memory resource to be mapped into
 * the memory space of an application. Configured start-offset and maximum
 * size and provided, together with the size requested by the application.
 * These parameters can select a subset of the PCI memory resource, but must
 * always provide a resource as large as requested by the application.
 */
int
UMDevXS_PCIDev_Map(
        unsigned int BAR,
        unsigned int SubsetStart,       // defined
        unsigned int SubsetSize,        // defined
        unsigned int Length,            // requested
        struct vm_area_struct * vma_p)
{
    struct pci_dev * PCIDev_p = LKM_DeviceSpecific_Get();

    LOG_INFO(UMDEVXS_LOG_PREFIX
             "%s: PCI device %s, bar %u, "
             "subset start/size 0x%08x/0x%x, "
             "length 0x%x\n",
             __func__,
             pci_name(LKM_DeviceSpecific_Get()),
             BAR,
             SubsetStart,
             SubsetSize,
             Length);

    // honor application limiter
    if (SubsetSize > Length)
        SubsetSize = Length;

    // was the PCI side of the driver enabled by the OS
    // this only happens when a compatible PCI device is inserted
    if (PCIDev_p == NULL)
        return -1;

    // is the BAR valid?
    if (BAR >= DEVICE_COUNT_RESOURCE)
        return -2;

    // is this BAR a memory mapped resource?
    if ((pci_resource_flags(PCIDev_p, BAR) & IORESOURCE_MEM) == 0)
        return -3;

    // limit size
    {
        unsigned int Len;

        Len = (unsigned int)pci_resource_len(PCIDev_p, BAR);

#ifndef UMDEVXS_REMOVE_SMALL_PCIWINDOW_SUPPORT
        // special handling of Versatile Board with 1MB memory windows
        if ((SubsetStart > Len) && (Len == 1 * 1024 * 1024))
        {
            LOG_CRIT(
                UMDEVXS_LOG_PREFIX
                "Redirecting map for BAR=%u, SubsetStart=0x%08x,"
                " Len=0x%x, to StartOffset=0\n",
                BAR,
                SubsetStart,
                Len);

            SubsetStart = 0;
        }
#endif /* UMDEVXS_REMOVE_SMALL_PCIWINDOW_SUPPORT */

        // start beyond end of resource?
        if (SubsetStart > Len)
            return -4;

        // limit inside resource
        if (SubsetStart + SubsetSize > Len)
            SubsetSize = Len - SubsetStart;
    }

    // now map the region into the application memory space
    {
        unsigned long StartOfs;
        resource_size_t ResStart;
        int ret;

        ResStart = pci_resource_start(PCIDev_p, BAR);

        ResStart += SubsetStart;
        StartOfs = (unsigned long)(ResStart >> PAGE_SHIFT);

        LOG_INFO(UMDEVXS_LOG_PREFIX
                 "%s: "
                 "Start=0x%lx, Size=0x%x, Addr=0x%lx\n",
                 __func__,
                 StartOfs << PAGE_SHIFT,
                 SubsetSize,
                 vma_p->vm_start);

#if defined(UMDEVXS_PCI_UNCACHED_MAPPING) && !defined(ARCH_X86)
        LOG_INFO(UMDEVXS_LOG_PREFIX
                 "%s: request non-cached mapping\n", __func__);

        // On some host HW non-cached memory mapping appears to be unavailable.
        // but hardware cache coherency works, so the code runs fine
        // with cached memory.
        vma_p->vm_flags |= VM_IO ;
        vma_p->vm_page_prot = pgprot_noncached(vma_p->vm_page_prot);
#endif // UMDEVXS_PCI_UNCACHED_MAPPING && !ARCH_X86

        // map the whole physically contiguous area in one piece
        ret = io_remap_pfn_range(vma_p,
                                 vma_p->vm_start,
                                 StartOfs,
                                 SubsetSize,
                                 vma_p->vm_page_prot);
        if (ret < 0)
        {
            LOG_CRIT(UMDEVXS_LOG_PREFIX
                     "%s: failed, remap result: %d\n",
                     __func__,
                     ret);

            return -5;
        }
    }

    LOG_INFO(UMDEVXS_LOG_PREFIX
             "%s: successfully mapped PCI device %s\n",
             __func__,
             pci_name(PCIDev_p));

    return 0;       // 0 = success
}


/*----------------------------------------------------------------------------
 * UMDevXS_PCIDev_HandleCmd_Find
 *
 * This function is called when the Device_Find function matched a PCI
 * placeholder device. This function is called with the related BAR and must
 * check it is valid and fill in a few CmdRsp fields.
 * The BAR must be supported by this PCI device.
 * The Size and Handle must be returned.
 * CmdRsp_p->uint1 = configured size of device to map
 */
void
UMDevXS_PCIDev_HandleCmd_Find(
        UMDevXS_CmdRsp_t * const CmdRsp_p,
        unsigned int BAR,
        unsigned int SubsetStart)
{
    struct pci_dev * PCIDev_p = LKM_DeviceSpecific_Get();

    if (CmdRsp_p == NULL)
        return;

    LOG_INFO(UMDEVXS_LOG_PREFIX
             "%s: bar %u, PCI device %s\n",
             __func__,
             BAR,
             pci_name(PCIDev_p));

    // was the PCI side of the driver enabled by the OS
    // this only happens when a compatible PCI device is inserted
    if (PCIDev_p == NULL)
    {
        CmdRsp_p->Error = 2;
        return;
    }

    // is the BAR valid?
    if (BAR >= DEVICE_COUNT_RESOURCE)
    {
        CmdRsp_p->Error = 3;
        return;
    }

    // is this BAR a memory mapped resource?
    if ((pci_resource_flags(PCIDev_p, BAR) & IORESOURCE_MEM) == 0)
    {
        CmdRsp_p->Error = 4;
        return;
    }

    // limit size
    {
        unsigned int Len;

        Len = (unsigned int)pci_resource_len(PCIDev_p, BAR);

#ifndef UMDEVXS_REMOVE_SMALL_PCIWINDOW_SUPPORT
        // special handling of Versatile Board with 1MB memory windows
        if ((SubsetStart > Len) &&
            (Len == 1 * 1024 * 1024))
        {
            unsigned int PCIAddr;

            // address bits 31:28 are remapped by Versatile board
            // address bits 27:20 are taken from PCI address (assigned by BIOS)
            // address bits 19:0 are taken from PCI
            // thus: bits 27:20 must equal the wanted start offset

            // example: 256MB window, SMAP=3, StartOffset=0x08000000, size=2MB
            //          ==> offset 0..0x1FFFFF maps to 0x38000000..0x03801FFFF
            //          1MB window, PCI-BIOS assigned address = 0xD8000000
            //          ==> offset 0..0x1FFFFF maps to 0x38000000..0x03801FFFF

            PCIAddr = pci_resource_start(PCIDev_p, BAR);

            if ((PCIAddr & MASK_28_BITS) == SubsetStart)
            {
                // possible to support!

                // do not map more than requested
                if (Len > CmdRsp_p->uint1)
                    Len = CmdRsp_p->uint1;

                LOG_CRIT(
                    UMDEVXS_LOG_PREFIX
                    "Redirecting request for BAR=%u, StartOffset=0x%08x,"
                    " Len=0x%x, PCIAddr=0x%08x to StartOffset=0, Len=0x%x\n",
                    BAR,
                    SubsetStart,
                    CmdRsp_p->uint1,
                    PCIAddr,
                    Len);

                SubsetStart = 0;
                CmdRsp_p->uint1 = Len;
            }
        }
#endif /* UMDEVXS_REMOVE_SMALL_PCIWINDOW_SUPPORT */

        // start beyond end of resource?
        if (SubsetStart > Len)
        {
            CmdRsp_p->Error = 5;
            return;
        }

        // limit inside resource
        if (SubsetStart + CmdRsp_p->uint1 > Len)
            CmdRsp_p->uint1 = Len - SubsetStart;
    }

    LOG_INFO(UMDEVXS_LOG_PREFIX
             "%s: successfully found PCI device %s\n",
             __func__,
             pci_name(PCIDev_p));
}


/*----------------------------------------------------------------------------
 * UMDevXS_PCIDev_Init
 *
 * Returns <0 on error.
 * Returns >=0 on success. The number is the interrupt number associated
 * with this PCI device.
 */
int
UMDevXS_PCIDev_Init(void)
{
    int IrqNr;
    LKM_Init_t LKMInit;

    LOG_INFO(UMDEVXS_LOG_PREFIX "%s: entered\n", __func__);

    ZEROINIT(LKMInit);
    IrqNr = 0;

    LKMInit.DriverName_p        = UMDEVXS_MODULENAME;
    LKMInit.ResByteCount        = UMDEVXS_PCI_DEVICE_RESOURCE_BYTE_COUNT;
    LKMInit.CustomInitData_p    = &IrqNr;

#ifdef UMDEVXS_ENABLE_KERNEL_SUPPORT
    LKMInit.fRetainMap          = true;
#endif

#ifdef UMDEVXS_USE_RPM
    LKMInit.PM_p                = RPM_OPS_PM;
#endif

#ifdef UMDEVXS_USE_MSI
    LKMInit.fUseMSI             = true;
#endif

    if (LKM_Init(&LKMInit) < 0)
    {
        LOG_CRIT(UMDEVXS_LOG_PREFIX
                 "%s: Failed to register the PCI device\n",
                 __func__);
        return -1;
    }

#ifdef UMDEVXS_USE_RPM
    if (RPM_INIT_MACRO(LKM_DeviceGeneric_Get()) != RPM_SUCCESS)
    {
        LOG_CRIT(UMDEVXS_LOG_PREFIX "%s: RPM_Init() failed\n", __func__);
        LKM_Uninit();
        return -2; // error
    }
#endif

    LOG_INFO(UMDEVXS_LOG_PREFIX
             "%s: left, initialization successful\n",
             __func__);

    return IrqNr; // success
}


/*----------------------------------------------------------------------------
 * UMDevXS_PCIDev_UnInit
 */
void
UMDevXS_PCIDev_UnInit(void)
{
    LOG_INFO(UMDEVXS_LOG_PREFIX "%s: entered\n", __func__);

#ifdef UMDEVXS_USE_RPM
    // Check if a race condition is possible here with auto-suspend timer
    (void)RPM_UNINIT_MACRO();
#endif

    LKM_Uninit();

    LOG_INFO(UMDEVXS_LOG_PREFIX "%s: left\n", __func__);
}


/*----------------------------------------------------------------------------
 * UMDevXS_PCIDev_GetReference
 */
void *
UMDevXS_PCIDev_GetReference(
        UMDevXS_PCIDev_Data_t * const Data_p)
{
    if (Data_p != NULL)
        Data_p->PhysAddr = LKM_PhysBaseAddr_Get();

    return LKM_DeviceGeneric_Get();
}


/*----------------------------------------------------------------------------
 * UMDevXS_Device_HandleCmd_Read32
 */
#ifndef UMDEVXS_REMOVE_DEVICE_PCICFG
void
UMDevXS_PCIDev_HandleCmd_Read32(
        UMDevXS_CmdRsp_t * const CmdRsp_p)
{
    struct pci_dev * PCIDev_p = LKM_DeviceSpecific_Get();

    CmdRsp_p->Error = -1;

    if (CmdRsp_p == NULL)
        return;

    // was the PCI side of the driver enabled by the OS
    // this only happens when a compatible PCI device is inserted
    if (PCIDev_p == NULL)
        return;

    LOG_INFO(UMDEVXS_LOG_PREFIX
             "%s: read offset (32-bit integers)=%u\n",
             __func__,
             CmdRsp_p->uint1);

    pci_read_config_dword(PCIDev_p,
                          CmdRsp_p->uint1,
                          &CmdRsp_p->uint2);

    CmdRsp_p->Error = 0; // Success
}
#endif // UMDEVXS_REMOVE_DEVICE_PCICFG


/*----------------------------------------------------------------------------
 * UMDevXS_Device_HandleCmd_Write32
 */
#ifndef UMDEVXS_REMOVE_DEVICE_PCICFG
void
UMDevXS_PCIDev_HandleCmd_Write32(
        UMDevXS_CmdRsp_t * const CmdRsp_p)
{
    struct pci_dev * PCIDev_p = LKM_DeviceSpecific_Get();

    CmdRsp_p->Error = -1;

    if (CmdRsp_p == NULL)
        return;

    // was the PCI side of the driver enabled by the OS
    // this only happens when a compatible PCI device is inserted
    if (PCIDev_p == NULL)
        return;

    LOG_INFO(UMDEVXS_LOG_PREFIX
             "%s: write offset (32-bit integers)=%u, value=%u\n",
             __func__,
             CmdRsp_p->uint1,
             CmdRsp_p->uint2);

    pci_write_config_dword(PCIDev_p,
                           CmdRsp_p->uint1,
                           CmdRsp_p->uint2);

    CmdRsp_p->Error = 0; // Success
}
#endif // UMDEVXS_REMOVE_DEVICE_PCICFG


#ifdef UMDEVXS_ENABLE_KERNEL_SUPPORT
/*----------------------------------------------------------------------------
 * UMDevXS_PCIDev_Get
 */
int
UMDevXS_PCIDev_Get(
              unsigned int DeviceID,
              struct pci_dev ** Device_pp,
              void __iomem ** MappedBaseAddr_pp)
{
    *Device_pp         = LKM_DeviceSpecific_Get();
    *MappedBaseAddr_pp = LKM_MappedBaseAddr_Get();
    return 0;
}
#endif // UMDEVXS_ENABLE_KERNEL_SUPPORT


#endif /* UMDEVXS_REMOVE_PCI */


/* end of file umdevxs_pcidev.c */
