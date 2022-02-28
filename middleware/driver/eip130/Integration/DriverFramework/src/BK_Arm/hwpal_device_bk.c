/* hwpal_device_bk.c
 *
 * This is the BK72XX Driver Framework v4 Device API
 * implementation for BEKEN BK72XX. The implementation is device-agnostic and
 * receives configuration details from the cs_hwpal_umdevxs.h file.
 */

/*****************************************************************************
* Copyright (c) 2017 INSIDE Secure B.V. All Rights Reserved.
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

// Driver Framework Device API
#include "device_mgmt.h"            // API to implement
#include "device_rw.h"              // API to implement
#include "device_swap.h"

// Driver Framework Device Internal interface
#include "device_internal.h"        // Device_Internal_*


/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */

// Default configuration
#include "c_hwpal_device_bk.h" // get the configuration options

// Driver Framework Basic Defs API
#include "basic_defs.h"             // uint32_t, NULL, inline, etc.

// Standard functions API
#include "clib.h"                   // memcmp
#include "stdlib.h"                 // malloc, free

// Driver Framework Device Platform interface
#include "device_platform.h"        // Device_Platform_*

#undef LOG_SEVERITY_MAX
#define LOG_SEVERITY_MAX  HWPAL_LOG_SEVERITY

// Logging API
#include "log.h"

#if 0	////DEL
// Xilinx sleep interface
////#include "sleep.h"   // usleep

// Xilinx IO interface
////#include "xil_io.h"
#endif

/*----------------------------------------------------------------------------
 * Definitions and macros
 */

// definition of the Flags
#define HWPAL_FLAGS_READ        BIT_0
#define HWPAL_FLAGS_WRITE       BIT_1
#define HWPAL_FLAGS_SWAP        BIT_2

#define HWPAL_PAGE_SIZE         4096


/*----------------------------------------------------------------------------
 * Local variables
 */

static Device_Admin_Static_t HWPALLib_Devices_Static[] =
{
    HWPAL_DEVICES
};

#define HWPAL_DEVICE_STATIC_COUNT  \
            (sizeof(HWPALLib_Devices_Static) / sizeof(Device_Admin_Static_t))

// All supported devices
static Device_Admin_t * HWPALLib_Devices_p [HWPAL_DEVICE_COUNT];

// Global administration data
static Device_Global_Admin_t HWPALLib_Device_Global;


/*----------------------------------------------------------------------------
 * write32_volatile
 */
static inline void
write32_volatile(
        uint32_t b, volatile void *addr)
{
	 *((volatile unsigned long *) (addr)) = b;
}


/*----------------------------------------------------------------------------
 * read32_volatile
 */
static inline uint32_t
read32_volatile(
        const volatile void *addr)
{
    return *((volatile unsigned long *) (addr));
}


/*----------------------------------------------------------------------------
 * Device_RemapDeviceAddress
 *
 * This function remaps certain device addresses (relative within the whole
 * device address map) to other addresses. This is needed when the integration
 * has remapped some EIP device registers to other addresses. The EIP Driver
 * Libraries assume the devices always have the same internal layout.
 */

// the cs_hwpal_device_lkm_pci.h file defines a HWPAL_REMAP_ADDRESSES that
// depends on the following HWPAL_REMAP_ONE

#define HWPAL_REMAP_ONE(_old, _new) \
    case _old: \
        DeviceByteOffset = _new; \
        break;

static inline unsigned int
Device_RemapDeviceAddress(
        unsigned int DeviceByteOffset)
{
    switch(DeviceByteOffset)
    {
        // include the remap statements
        HWPAL_REMAP_ADDRESSES

        default:
            break;
    }

    return DeviceByteOffset;
}


/*----------------------------------------------------------------------------
 * HWPALLib_Device2RecPtr
 *
 * This function converts an Device_Handle_t received via one of the
 * Device API functions into a Device_Admin_t record pointer, if it is
 * valid.
 *
 * Return Value
 *     NULL    Provided Device Handle was not valid
 *     other   Pointer to a Device_Admin_t record
 */
static inline Device_Admin_t *
HWPALLib_Device2RecordPtr(
        Device_Handle_t Device)
{
    // since we have so few records, we simply enumerate them
    Device_Admin_t * p = (void *)Device;

    if (p == NULL)
        return NULL;

#ifdef HWPAL_DEVICE_MAGIC
    if (p->Magic != HWPAL_DEVICE_MAGIC)
        return NULL;
#endif

    return p;
}


/*----------------------------------------------------------------------------
 * HWPALLib_IsValid
 *
 * This function checks that the parameters are valid to make the access.
 *
 * Device_p is valid
 * ByteOffset is inside device memory range
 * ByteOffset is 32-bit aligned
 */
static inline bool
HWPALLib_IsValid(
        const Device_Admin_t * const Device_p,
        const unsigned int ByteOffset)
{
    if (Device_p == NULL)
        return false;

    if (ByteOffset & 3)
        return false;

    if (Device_p->FirstOfs + ByteOffset > Device_p->LastOfs)
        return false;

    return true;
}


/*-----------------------------------------------------------------------------
 * device_internal interface
 *
 */

/*----------------------------------------------------------------------------
 * Device_Internal_Static_Count_Get
 */
unsigned int
Device_Internal_Static_Count_Get(void)
{
    return HWPAL_DEVICE_STATIC_COUNT;
}


/*----------------------------------------------------------------------------
 * Device_Internal_Count_Get
 */
unsigned int
Device_Internal_Count_Get(void)
{
    return HWPAL_DEVICE_COUNT;
}


/*----------------------------------------------------------------------------
 * Device_Internal_Admin_Static_Get
 */
const Device_Admin_Static_t *
Device_Internal_Admin_Static_Get(void)
{
    return HWPALLib_Devices_Static;
}


/*----------------------------------------------------------------------------
 * Device_Internal_Admin_Get
 */
Device_Admin_t **
Device_Internal_Admin_Get(void)
{
    return HWPALLib_Devices_p;
}


/*----------------------------------------------------------------------------
 * Device_Internal_Admin_Global_Get
 */
Device_Global_Admin_t *
Device_Internal_Admin_Global_Get(void)
{
    return &HWPALLib_Device_Global;
}


/*----------------------------------------------------------------------------
 * Device_Internal_Alloc
 */
void *
Device_Internal_Alloc(
        unsigned int ByteCount)
{
    return malloc(ByteCount);
}


/*----------------------------------------------------------------------------
 * Device_Internal_Free
 */
void
Device_Internal_Free(
        void * Ptr)
{
    free(Ptr);
}


/*-----------------------------------------------------------------------------
 * Device_Internal_Initialize
 */
int
Device_Internal_Initialize(
        void * CustomInitData_p)
{
    unsigned int i;
    IDENTIFIER_NOT_USED(CustomInitData_p);

    for (i = 0; i < HWPAL_DEVICE_COUNT; i++)
        if (HWPALLib_Devices_p[i])
{			
            HWPALLib_Devices_p[i]->Platform.Mem32_p =
                                    (uint32_t*)HWPAL_DEVICE_MEM_ADDR;
HWPALLib_Devices_p[i]->Platform.FirstOfs = 	HWPALLib_Devices_p[i]->FirstOfs;	////WHY???
HWPALLib_Devices_p[i]->Platform.LastOfs = 	HWPALLib_Devices_p[i]->LastOfs;		////WHY???
}
    return 0; // success
}


/*-----------------------------------------------------------------------------
 * Device_Internal_Initialize
 */
void
Device_Internal_UnInitialize(void)
{
    unsigned int i;

    for (i = 0; i < HWPAL_DEVICE_COUNT; i++)
        if (HWPALLib_Devices_p[i])
            HWPALLib_Devices_p[i]->Platform.Mem32_p = NULL;
}


/*-----------------------------------------------------------------------------
 * Device_Internal_Find
 */
Device_Handle_t
Device_Internal_Find(
        const char * DeviceName_p,
        const unsigned int Index)
{
    IDENTIFIER_NOT_USED(DeviceName_p);

#ifdef HWPAL_STRICT_ARGS_CHECK
    if (Index >= HWPAL_DEVICE_COUNT)
    {
        LOG_CRIT("%s: Failed to find device '%s'\n", __func__, DeviceName_p);
        return NULL; // error, out of device list boundaries
    }
#endif

#ifdef HWPAL_TRACE_DEVICE_FIND
    Log_FormattedMessage("%s: found device name %s, index %d\n",
                         __func__,
                         DeviceName_p,
                         Index);
#endif

    // Return the device handle
    return (Device_Handle_t)HWPALLib_Devices_p[Index];
}


/*-----------------------------------------------------------------------------
 * Device_Internal_GetIndex
 */
int
Device_Internal_GetIndex(
        const Device_Handle_t Device)
{
    Device_Admin_t * Device_p;

#ifdef HWPAL_STRICT_ARGS_CHECK
    Device_p = HWPALLib_Device2RecordPtr(Device);
#else
    Device_p = Device;
#endif

#ifdef HWPAL_STRICT_ARGS_CHECK
    if (!HWPALLib_IsValid(Device_p, 0))
    {
        LOG_CRIT("%s: invalid device (%p) or ByteOffset (%u)\n",
                 __func__,
                 Device,
                 0);
        return -1;
    }
#endif

    return Device_p->DeviceId;
}


/*-----------------------------------------------------------------------------
 * device_rw API
 *
 * These functions can be used to transfer a single 32bit word or an array of
 * 32bit words to or from a device.
 * Endianness swapping is performed on the fly based on the configuration for
 * this device.
 *
 */

/*-----------------------------------------------------------------------------
 * Device_Read32
 */
uint32_t
Device_Read32(
        const Device_Handle_t Device,
        const unsigned int ByteOffset)
{
    Device_Admin_t * Device_p;
    uint32_t WordRead;
    unsigned int Idx;

    if (!HWPALLib_Device_Global.fInitialized)
    {
        LOG_CRIT("%s: failed, not initialized\n", __func__);
        return 0xEEEEEEEE;
    }

#ifdef HWPAL_STRICT_ARGS_CHECK
    Device_p = HWPALLib_Device2RecordPtr(Device);
#else
    Device_p = Device;
#endif

#ifdef HWPAL_STRICT_ARGS_CHECK
    if (!HWPALLib_IsValid(Device_p, ByteOffset))
    {
        LOG_CRIT("%s: Invalid Device (%p) or ByteOffset (%u)\n",
                 __func__,
                 Device,
                 ByteOffset);
        return 0xEEEEEEEE;
    }
#endif

    {
        unsigned int DeviceByteOffset = Device_p->Platform.FirstOfs + ByteOffset;

        DeviceByteOffset = Device_RemapDeviceAddress(DeviceByteOffset);

        Idx = DeviceByteOffset >> 2;

  #ifdef HWPAL_DEVICE_READ_DELAY_US
        LOG_INFO("%s: delay %u us before read\n",
                 __func__,
                 HWPAL_DEVICE_READ_DELAY_US);
        usleep(HWPAL_DEVICE_READ_DELAY_US);
  #endif

        WordRead = read32_volatile(Device_p->Platform.Mem32_p + Idx);

#ifdef HWPAL_DEVICE_ENABLE_SWAP
        if (Device_p->Flags & HWPAL_FLAGS_SWAP)
            WordRead = Device_SwapEndian32(WordRead);
#endif
    }

#ifdef HWPAL_TRACE_DEVICE_READ
    if (Device_p->Flags & HWPAL_FLAGS_READ)
    {
        unsigned int DeviceByteOffset = Device_p->Platform.FirstOfs + ByteOffset;
        unsigned int DeviceByteOffset2 =
                Device_RemapDeviceAddress(DeviceByteOffset);

        if (DeviceByteOffset2 != DeviceByteOffset)
        {
            DeviceByteOffset2 -= Device_p->Platform.FirstOfs;
            Log_FormattedMessage("%s: 0x%x(was 0x%x) = 0x%08x (%s)\n",
                                 __func__,
                                 DeviceByteOffset2,
                                 ByteOffset,
                                 WordRead,
                                 Device_p->DevName);
        }
        else
        {
            Log_FormattedMessage("%s: %s@0x%08x => 0x%08x, dev nr = %d, "
                                 "addr = %p, offset = 0x%08x\n",
                                 __func__,
                                 Device_p->DevName,
                                 ByteOffset,
                                 WordRead,
                                 Device_p->DeviceNr,
                                 (void *)Device_p->Platform.Mem32_p,
                                 Device_p->Platform.FirstOfs + ByteOffset);
        }
    }
#endif

    return WordRead;
}


/*-----------------------------------------------------------------------------
 * Device_Write32
 */
int
Device_Write32(
        const Device_Handle_t Device,
        const unsigned int ByteOffset,
        const uint32_t Value)
{
    Device_Admin_t * Device_p;
    uint32_t WordWrite = Value;
    unsigned int Idx;

    if (!HWPALLib_Device_Global.fInitialized)
    {
        LOG_CRIT("%s: failed, not initialized\n", __func__);
        return -1;
    }

#ifdef HWPAL_STRICT_ARGS_CHECK
    Device_p = HWPALLib_Device2RecordPtr(Device);
#else
    Device_p = Device;
#endif

#ifdef HWPAL_STRICT_ARGS_CHECK
    if (!HWPALLib_IsValid(Device_p, ByteOffset))
    {
        LOG_CRIT("%s: Invalid Device (%p) or ByteOffset (%u)\n",
                 __func__,
                 Device,
                 ByteOffset);
        return -2;
    }
#endif

#ifdef HWPAL_TRACE_DEVICE_WRITE
    if (Device_p->Flags & HWPAL_FLAGS_WRITE)
    {
        unsigned int DeviceByteOffset = Device_p->Platform.FirstOfs + ByteOffset;
        unsigned int DeviceByteOffset2 =
                Device_RemapDeviceAddress(DeviceByteOffset);

        if (DeviceByteOffset2 != DeviceByteOffset)
        {
            DeviceByteOffset2 -= Device_p->Platform.FirstOfs;
            Log_FormattedMessage("%s: 0x%x(was 0x%x) = 0x%08x (%s)\n",
                                 __func__,
                                 DeviceByteOffset2,
                                 ByteOffset,
                                 Value,
                                 Device_p->DevName);
        }
        else
        {
            Log_FormattedMessage("%s: %s@0x%08x = 0x%08x, dev nr = %d, "
                                 "addr = %p, offset = 0x%08x\n",
                                 __func__,
                                 Device_p->DevName,
                                 ByteOffset,
                                 Value,
                                 Device_p->DeviceNr,
                                 (void *)Device_p->Platform.Mem32_p,
                                 Device_p->Platform.FirstOfs + ByteOffset);
        }
    }
#endif

    {
        uint32_t DeviceByteOffset = Device_p->Platform.FirstOfs + ByteOffset;

        DeviceByteOffset = Device_RemapDeviceAddress(DeviceByteOffset);

#ifdef HWPAL_DEVICE_ENABLE_SWAP
        if (Device_p->Flags & HWPAL_FLAGS_SWAP)
            WordWrite = Device_SwapEndian32(WordWrite);
#endif

        Idx = DeviceByteOffset >> 2;
        write32_volatile(WordWrite, Device_p->Platform.Mem32_p + Idx);
    }

    return 0;
}


/*-----------------------------------------------------------------------------
 * Device_Read32Array
 */
int
Device_Read32Array(
        const Device_Handle_t Device,
        const unsigned int StartByteOffset,
        uint32_t * MemoryDst_p,
        const int Count)
{
    Device_Admin_t * Device_p;
    uint32_t WordRead;
    unsigned int Idx;
    int Nwords;

    if (!HWPALLib_Device_Global.fInitialized)
    {
        LOG_CRIT("%s: failed, not initialized\n", __func__);
        return -1;
    }

#ifdef HWPAL_STRICT_ARGS_CHECK
    Device_p = HWPALLib_Device2RecordPtr(Device);
#else
    Device_p = Device;
#endif

    if (Count == 0)
    {
        // avoid that `Count-1' goes negative in test below
        return -3;
    }

#ifdef HWPAL_STRICT_ARGS_CHECK
    if ((Count < 0) ||
        !HWPALLib_IsValid(Device_p, StartByteOffset) ||
        !HWPALLib_IsValid(Device_p, StartByteOffset + (Count - 1) * 4))
    {
        LOG_CRIT("%s: Invalid Device (%p) or read area (%u-%u)\n",
                 __func__,
                 Device,
                 StartByteOffset,
                 (unsigned int)(StartByteOffset + (Count - 1) *
                 sizeof(uint32_t)));
        return -1;
    }
#endif

    Idx = (Device_p->FirstOfs + StartByteOffset) >> 2;
    for (Nwords = 0; Nwords < Count; ++Nwords, ++Idx)
    {
        WordRead = read32_volatile(Device_p->Platform.Mem32_p + Idx);

#ifdef HWPAL_DEVICE_ENABLE_SWAP
        if (Device_p->Flags & HWPAL_FLAGS_SWAP)
            WordRead = Device_SwapEndian32(WordRead);
#endif

        MemoryDst_p[Nwords] = WordRead;

#ifdef HWPAL_TRACE_DEVICE_READ
        if (Device_p->Flags & HWPAL_FLAGS_READ)
        {
            Log_FormattedMessage("%s: rd %s@0x%08x => 0x%08x\n",
                                 __func__,
                                 Device_p->DevName,
                                 (Nwords << 2) + Device_p->Platform.FirstOfs +
                                                             StartByteOffset,
                                 WordRead);
        }
#endif
    }
	return 0;
}


/*-----------------------------------------------------------------------------
 * Device_Read32Array
 */
int
Device_Write32Array(
        const Device_Handle_t Device,
        const unsigned int StartByteOffset,
        const uint32_t * MemorySrc_p,
        const int Count)
{
    Device_Admin_t * Device_p;
    uint32_t WordWrite;
    unsigned int Idx;
    int Nwords;

    if (!HWPALLib_Device_Global.fInitialized)
    {
        LOG_CRIT("%s: failed, not initialized\n", __func__);
        return -1;
    }

#ifdef HWPAL_STRICT_ARGS_CHECK
    Device_p = HWPALLib_Device2RecordPtr(Device);
#else
    Device_p = Device;
#endif

    if (Count == 0)
        return -3; // avoid that `Count-1' goes negative in test below

#ifdef HWPAL_STRICT_ARGS_CHECK
    if ((Count < 0) ||
        !HWPALLib_IsValid(Device_p, StartByteOffset) ||
        !HWPALLib_IsValid(Device_p, StartByteOffset + (Count - 1) * 4))
    {
        LOG_CRIT("%s: Invalid Device (%p) or write area (%u-%u)\n",
                 __func__,
                 Device,
                 StartByteOffset,
                 (unsigned int)(StartByteOffset + (Count - 1) *
                 sizeof(uint32_t)));
        return -1;
    }
#endif

    Idx = (Device_p->Platform.FirstOfs + StartByteOffset) >> 2;
    for (Nwords = 0; Nwords < Count; ++Nwords, ++Idx)
    {
        WordWrite = MemorySrc_p[Nwords];

#ifdef HWPAL_DEVICE_ENABLE_SWAP
        if (Device_p->Flags & HWPAL_FLAGS_SWAP)
            WordWrite = Device_SwapEndian32(WordWrite);
#endif

        write32_volatile(WordWrite, Device_p->Platform.Mem32_p + Idx);

#ifdef HWPAL_TRACE_DEVICE_WRITE
        if (Device_p->Flags & HWPAL_FLAGS_WRITE)
        {
            Log_FormattedMessage("%s: wr %s@0x%08x = 0x%08x\n",
                                 __func__,
                                 Device_p->DevName,
                                 (Nwords << 2) + Device_p->Platform.FirstOfs +
                                                             StartByteOffset,
                                 WordWrite);
        }
#endif
    }
	return 0;
}


/* end of file hwpal_device_bk.c */
