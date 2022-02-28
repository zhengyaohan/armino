/* umdevxsproxy.c
 *
 * This library integrates with an application and enables it to use the
 * services provided by the kernel driver.
 */

/*****************************************************************************
* Copyright (c) 2009-2019 INSIDE Secure B.V. All Rights Reserved.
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

// Default configuration
#include "c_umdevxsproxy.h"

/*----------------------------------------------------------------------------
 * This module implements (provides) the following interface(s):
 */

#ifndef UMDEVXSPROXY_REMOVE_DEVICE
#include "umdevxsproxy_device.h"            // API to provide
#endif

#ifndef UMDEVXSPROXY_REMOVE_SMBUF
#include "umdevxsproxy_shmem.h"             // API to provide
#endif

#ifndef UMDEVXSPROXY_REMOVE_INTERRUPT
#include "umdevxsproxy_interrupt.h"         // API to provide
#endif

#ifndef UMDEVXSPROXY_REMOVE_PCICFG
#include "umdevxsproxy_device_pcicfg.h"     // API to provide
#endif


/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */

#include "umdevxs_cmd.h"        // the cmd/rsp structure to the kernel
#include "umdevxsproxy.h"       // the APIs exported by this file

#include <fcntl.h>              // open, O_RDWR
#include <unistd.h>             // close, write, sysconf
#include <sys/mman.h>           // mmap
#include <stdio.h>              // NULL
#include <string.h>             // memset
#include <stdint.h>             // uintptr_t


/*----------------------------------------------------------------------------
 * Definitions and macros
 */

#ifndef ZEROINIT
#define ZEROINIT(_x)  memset(&_x, 0, sizeof(_x))
#endif

#ifndef IDENTIFIER_NOT_USED
#define IDENTIFIER_NOT_USED(_v) (void)(_v)
#endif


/*----------------------------------------------------------------------------
 * Local variables
 */

// character device file descriptor
static int UMDevXSProxy_fd = -1;

static const char UMDevXSProxy_NodeName[] = UMDEVXSPROXY_NODE_NAME;


/*----------------------------------------------------------------------------
 * UMDevXSProxy_DoCmdRsp
 *
 * Return Value
 *     0    Success
 *     <0   Error
 */
int
UMDevXSProxy_DoCmdRsp(
        UMDevXS_CmdRsp_t * const CmdRsp_p)
{
    int res;

    if (CmdRsp_p == NULL)
        return -1;

    if (UMDevXSProxy_fd < 0)
        return -2;

    CmdRsp_p->Magic = UMDEVXS_CMDRSP_MAGIC;

    // write() is a blocking call
    // it takes the pointer to the CmdRsp structure
    // process it and fills it with the results
    res = (int)write(UMDevXSProxy_fd, CmdRsp_p, sizeof(UMDevXS_CmdRsp_t));
    if (res != sizeof(UMDevXS_CmdRsp_t))
        return -3;

    return 0;       // 0 = success
}


/*----------------------------------------------------------------------------
 * UMDevXSProxy_Map
 */
void *
UMDevXSProxy_Map(
        const int Handle,
        const unsigned int MemorySize)
{
    void * p = NULL;

    // limit size to 1GB
    if (MemorySize > 1024*1024*1024)
        return NULL;        // ## RETURN ##

    // fail if not talking to character device
    if (UMDevXSProxy_fd >= 0)
    {
        off_t MapOffset;

        // encode the Handle into the MapOffset
        MapOffset = (off_t)Handle;
        MapOffset *= sysconf(_SC_PAGESIZE);     // mandatory

        // try to map the memory region
        // MAP_SHARED disable private buffering with manual sync
        p = mmap(
                NULL,
                (size_t)MemorySize,
                PROT_READ | PROT_WRITE,
                MAP_SHARED,
                UMDevXSProxy_fd,
                MapOffset);         // encodes the Handle

        // check for special error pointer
        if (p == MAP_FAILED)
            p = NULL;
    }

    return p;
}


/*----------------------------------------------------------------------------
 * UMDevXSProxy_Unmap
 */
int
UMDevXSProxy_Unmap(
        void * p,
        const unsigned int MemorySize)
{
    return munmap(p, (size_t)MemorySize);
}


/*----------------------------------------------------------------------------
 * UMDevXSProxy_Init
 *
 * Must be called once before any of the other functions.
 *
 * Return Value
 *     0  Success
 *    -1  Failed to contact kernel driver
 */
int
UMDevXSProxy_Init(void)
{
    // silently ignore bad use order
    if (UMDevXSProxy_fd >= 0)
        return 0;

    // try to open the character device
    {
        int fd = open(UMDevXSProxy_NodeName, O_RDWR);
        if (fd < 0)
            return -1;

        // connected successfully
        UMDevXSProxy_fd = fd;
    }

    return 0;       // 0 = success
}


/*----------------------------------------------------------------------------
 * UMDevXSProxy_Shutdown
 *
 * Must be called last, as clean-up step before stopping the application.
 */
void
UMDevXSProxy_Shutdown(void)
{
    if (UMDevXSProxy_fd >= 0)
    {
        close(UMDevXSProxy_fd);

        // mark as closed to avoid re-use
        UMDevXSProxy_fd = -1;
    }
}


/*----------------------------------------------------------------------------
 * UMDevXSProxy_Device_Find
 */
#ifndef UMDEVXSPROXY_REMOVE_DEVICE
#ifndef UMDEVXSPROXY_REMOVE_DEVICE_FIND
int
UMDevXSProxy_Device_Find(
        const char * Name_p,
        int * const DeviceID_p,
        unsigned int * const DeviceMemorySize_p)
{
    UMDevXS_CmdRsp_t CmdRsp;
    int res;

    if (Name_p == NULL || DeviceID_p == NULL || DeviceMemorySize_p == NULL)
        return -4; // bad parameters

    // initialize the output parameters
    *DeviceID_p = 0;
    *DeviceMemorySize_p = 0;

    // zero-init also protects against future extensions
    ZEROINIT(CmdRsp);

    CmdRsp.Opcode = UMDEVXS_OPCODE_DEVICE_FIND;

    // manual copy loop to avoid unnecessary dependency on system library
    {
        char * p = CmdRsp.szName;
        int i;
        for(i = 0; i < UMDEVXS_CMDRSP_MAXLEN_NAME; i++)
        {
            const char c = Name_p[i];
            if (c == 0)
                break;
            *p++ = c;
        }

        *p = 0;
    }

    // talk to the kernel
    res = UMDevXSProxy_DoCmdRsp(&CmdRsp);
    if (res < 0)
        return res;

    // found the device?
    if (CmdRsp.Error != 0)
        return -5;

    // populate the output parameters
    *DeviceID_p = CmdRsp.Handle;
    *DeviceMemorySize_p = CmdRsp.uint1;

    return 0;       // 0 = success
}
#endif /* UMDEVXSPROXY_REMOVE_DEVICE_FIND */


/*----------------------------------------------------------------------------
 * UMDevXSProxy_Device_FindRange
 */
int
UMDevXSProxy_Device_FindRange(
    const unsigned int bar,
    const unsigned int StartOffset,
    const unsigned int RangeSize,
    int * const DeviceID_p)
{
    UMDevXS_CmdRsp_t CmdRsp;
    int res;

    // initialize the output parameter
    *DeviceID_p = 0;

    // zero-init also protects against future extensions
    ZEROINIT(CmdRsp);

    CmdRsp.Opcode = UMDEVXS_OPCODE_DEVICE_FINDRANGE;
    CmdRsp.uint1 = bar;
    CmdRsp.uint2 = StartOffset;
    CmdRsp.uint3 = RangeSize;
    // talk to the kernel
    res = UMDevXSProxy_DoCmdRsp(&CmdRsp);
    if (res < 0)
        return res;      // ## RETURN ##

    // found the device?
    if (CmdRsp.Error != 0)
        return -4;      // ## RETURN ##

    // populate the output parameter
    *DeviceID_p = CmdRsp.Handle;

    return 0; // success
}
#endif /* UMDEVXSPROXY_REMOVE_DEVICE */


/*----------------------------------------------------------------------------
 * UMDevXSProxy_Device_Enum
 */
#ifndef UMDEVXSPROXY_REMOVE_DEVICE
#ifndef UMDEVXSPROXY_REMOVE_DEVICE_ENUM
int
UMDevXSProxy_Device_Enum(
        const unsigned int DeviceNr,
        const unsigned int Name_Size,
        char * Name_p)
{
    UMDevXS_CmdRsp_t CmdRsp;
    int res;

    if (Name_p == NULL || DeviceNr > 255)
        return -4; // bad parameters

    // zero-init also protects against future extensions
    ZEROINIT(CmdRsp);

    CmdRsp.Opcode = UMDEVXS_OPCODE_DEVICE_ENUM;
    CmdRsp.uint1 = DeviceNr;

    // talk to the kernel
    res = UMDevXSProxy_DoCmdRsp(&CmdRsp);
    if (res < 0)
        return res;

    // found the device?
    if (CmdRsp.Error != 0)
        return -5;

    // populate the output parameters
    {
        unsigned int Len = UMDEVXS_CMDRSP_MAXLEN_NAME;

        if (Len > Name_Size)
            Len = Name_Size;

        memcpy(Name_p, CmdRsp.szName, Len);
        Name_p[Name_Size - 1] = 0;
    }

    return 0; // success
}
#endif /* UMDEVXSPROXY_REMOVE_DEVICE_ENUM */
#endif /* UMDEVXSPROXY_REMOVE_DEVICE */


/*----------------------------------------------------------------------------
 * UMDevXSProxy_Device_Map
 */
#ifndef UMDEVXSPROXY_REMOVE_DEVICE
void *
UMDevXSProxy_Device_Map(
        const int DeviceID,
        const unsigned int DeviceMemorySize)
{
    return UMDevXSProxy_Map(DeviceID, DeviceMemorySize);
}
#endif /* UMDEVXSPROXY_REMOVE_DEVICE */


/*----------------------------------------------------------------------------
 * UMDevXSProxy_Device_Unmap
 */
#ifndef UMDEVXSPROXY_REMOVE_DEVICE
#ifndef UMDEVXSPROXY_REMOVE_DEVICE_UNMAP
int
UMDevXSProxy_Device_Unmap(
        const int DeviceID,
        void * DeviceMemory_p,
        const unsigned int DeviceMemorySize)
{
    IDENTIFIER_NOT_USED(DeviceID);

    return UMDevXSProxy_Unmap(DeviceMemory_p, DeviceMemorySize);
}
#endif /* UMDEVXSPROXY_REMOVE_DEVICE_UNMAP */
#endif /* UMDEVXSPROXY_REMOVE_DEVICE */


/*----------------------------------------------------------------------------
 * UMDevXSProxy_SHMem_Alloc
 */
#ifndef UMDEVXSPROXY_REMOVE_SMBUF
int
UMDevXSProxy_SHMem_Alloc(
        const unsigned int Size,
        const unsigned int Bank,
        const unsigned int Alignment,
        UMDevXSProxy_SHMem_Handle_t * const Handle_p,
        UMDevXSProxy_SHMem_BufPtr_t * const BufPtr_p,
        UMDevXSProxy_SHMem_DevAddr_t * const DevAddr_p,
        unsigned int * const ActualSize_p)
{
    UMDevXS_CmdRsp_t CmdRsp;
    void * p;
    int res;
    unsigned int page_size_1, PagedSize;

    if (Handle_p == NULL ||
        BufPtr_p == NULL ||
        DevAddr_p == NULL ||
        ActualSize_p == NULL ||
        Size == 0)
    {
        return -4; // bad parameter
    }

    // zero-init also protects against future extensions
    ZEROINIT(CmdRsp);

    // first use the Cmd/Rsp interface to create the buffer
    // and get the handle.

    // Adjust Size to a multiple of the page size
    page_size_1 = (unsigned int )(sysconf(_SC_PAGESIZE) - 1);
    PagedSize = (Size + page_size_1) & ~page_size_1;

    CmdRsp.Opcode = UMDEVXS_OPCODE_SMBUF_ALLOC;
    CmdRsp.uint1 = PagedSize;
    CmdRsp.uint2 = Bank;
    CmdRsp.uint3 = Alignment;

    // Store the device address, not always used
    CmdRsp.ptr1 = DevAddr_p->p;

    // populate the output parameters
    Handle_p->p   = NULL;
    BufPtr_p->p   = NULL;
    DevAddr_p->p  = NULL;
    *ActualSize_p = 0;

    // talk to the kernel
    res = UMDevXSProxy_DoCmdRsp(&CmdRsp);
    if (res < 0)
        return res;

    // allocated the buffer?
    if (CmdRsp.Error != 0)
        return -5;

    // next, map the buffer into the memory map of the caller,
    // using PagedSize, i.e. not the actual size in CmdRsp.uint.
    p = UMDevXSProxy_Map(CmdRsp.Handle, PagedSize);

    // managed to add to address map?
    if (p == NULL)
    {
        // no; free the allocated buffer immediately
        CmdRsp.Opcode = UMDEVXS_OPCODE_SMBUF_FREE;
        UMDevXSProxy_DoCmdRsp(&CmdRsp);
        return -6;
    }

    // populate the output parameters
    BufPtr_p->p   = p;
    DevAddr_p->p  = CmdRsp.ptr1;
    Handle_p->p   = (void *)(uintptr_t)CmdRsp.Handle;
    *ActualSize_p = PagedSize;

    // ask the kernel driver to remember the mapping info
    // as we need it in the free function
    CmdRsp.Opcode = UMDEVXS_OPCODE_SMBUF_SETBUFINFO;
    //CmdRsp.Handle (already set)
    //CmdRsp.uint1 (already set)
    CmdRsp.ptr1 = p;

    // talk to the kernel
    res = UMDevXSProxy_DoCmdRsp(&CmdRsp);
    if (res < 0)
        return res-6;

    if (CmdRsp.Error != 0)
        return -10;

    return 0;       // 0 = success
}
#endif /* UMDEVXSPROXY_REMOVE_SMBUF */


/*----------------------------------------------------------------------------
 * UMDevXSProxy_SHMem_Register
 */
#ifndef UMDEVXSPROXY_REMOVE_SMBUF
int
UMDevXSProxy_SHMem_Register(
        const unsigned int Size,
        const UMDevXSProxy_SHMem_BufPtr_t BufPtr,
        const UMDevXSProxy_SHMem_Handle_t Handle,
        UMDevXSProxy_SHMem_Handle_t * const RetHandle_p)
{
    UMDevXS_CmdRsp_t CmdRsp;
    int res;

    if (RetHandle_p == NULL || Handle.p == NULL ||
                            BufPtr.p == NULL || Size == 0)
        return -4; // bad parameter

    // populate the output parameter
    RetHandle_p->p = NULL;

    // zero-init also protects against future extensions
    ZEROINIT(CmdRsp);

    CmdRsp.Opcode = UMDEVXS_OPCODE_SMBUF_REGISTER;
    CmdRsp.uint1 = Size;
    CmdRsp.ptr1 = BufPtr.p;
    CmdRsp.Handle = (int)(uintptr_t)Handle.p;

    // talk to the kernel
    res = UMDevXSProxy_DoCmdRsp(&CmdRsp);
    if (res < 0)
        return res;

    // registered the buffer?
    if (CmdRsp.Error != 0)
        return -5;

    // populate the output parameter
    RetHandle_p->p = (void *)(uintptr_t)CmdRsp.Handle;

    return 0; // success
}
#endif /* UMDEVXSPROXY_REMOVE_SMBUF */


/*----------------------------------------------------------------------------
 * UMDevXSProxy_SHMem_Free
 */
#ifndef UMDEVXSPROXY_REMOVE_SMBUF
int
UMDevXSProxy_SHMem_Free(
        const UMDevXSProxy_SHMem_Handle_t Handle)
{
    UMDevXS_CmdRsp_t CmdRsp;
    int res;

    // zero-init also protects against future extensions
    ZEROINIT(CmdRsp);

    CmdRsp.Opcode = UMDEVXS_OPCODE_SMBUF_GETBUFINFO;
    CmdRsp.Handle = (int)(uintptr_t)Handle.p;

    // talk to the kernel
    res = UMDevXSProxy_DoCmdRsp(&CmdRsp);

    // found the buffer?
    if (res < 0)
        return res;

    if (CmdRsp.Error != 0)
        return -4;

    if (CmdRsp.ptr1 != NULL)
    {
        // first unmap the buffer from the application memory map
        res = UMDevXSProxy_Unmap(CmdRsp.ptr1, CmdRsp.uint1);
        if (res != 0)
            return -5;
    }

    // next, free the buffer (or just the AdminRecord for registered buffers)
    // using the cmd/rsp interface
    ZEROINIT(CmdRsp);

    CmdRsp.Opcode = UMDEVXS_OPCODE_SMBUF_FREE;
    CmdRsp.Handle = (int)(uintptr_t)Handle.p;

    // talk to the kernel
    res = UMDevXSProxy_DoCmdRsp(&CmdRsp);
    if (res < 0)
        return res-5;

    if (CmdRsp.Error != 0)
        return -9;

    return 0; // success
}
#endif /* UMDEVXSPROXY_REMOVE_SMBUF */


/*----------------------------------------------------------------------------
 * UMDevXSProxy_SHMem_Attach
 *
 * This implementation assumes that `DevAddr' holds the physical address
 * for the memory to be attached, valid for this host. This address typically
 * is the result from translating a (physical) address received from another
 * host.
 */
#ifndef UMDEVXSPROXY_REMOVE_SMBUF
#ifndef UMDEVXSPROXY_REMOVE_SMBUF_ATTACH
int
UMDevXSProxy_SHMem_Attach(
        const UMDevXSProxy_SHMem_DevAddr_t DevAddr,
        const unsigned int Size,
        const unsigned int Bank,
        UMDevXSProxy_SHMem_Handle_t * const Handle_p,
        UMDevXSProxy_SHMem_BufPtr_t * const BufPtr_p,
        unsigned int * const Size_p)
{
    UMDevXS_CmdRsp_t CmdRsp;
    void * p;
    int res;

    if (Size == 0 || Handle_p == NULL || BufPtr_p == NULL || Size_p == NULL)
        return -4; // bad parameter

    // populate the output parameters
    Handle_p->p = NULL;
    BufPtr_p->p = NULL;
    *Size_p = 0;

    // first use the Cmd/Rsp interface to create the buffer
    // and get the handle.

    // zero-init also protects against future extensions
    ZEROINIT(CmdRsp);

    CmdRsp.Opcode = UMDEVXS_OPCODE_SMBUF_ATTACH;
    CmdRsp.ptr1 = DevAddr.p;
    CmdRsp.uint1 = Size;
    CmdRsp.uint2 = Bank;

    // talk to the kernel
    res = UMDevXSProxy_DoCmdRsp(&CmdRsp);
    if (res < 0)
        return res;

    // registered the buffer?
    if (CmdRsp.Error != 0)
        return -5;

    // next, map the buffer into the memory map of the caller
    p = UMDevXSProxy_Map(CmdRsp.Handle, CmdRsp.uint1);

    // managed to add to address map?
    if (p == NULL)
    {
        // no; detach from the buffer immediately
        CmdRsp.Opcode = UMDEVXS_OPCODE_SMBUF_DETACH;
        UMDevXSProxy_DoCmdRsp(&CmdRsp);
        return -6;
    }

    // populate the output parameters
    BufPtr_p->p = p;
    Handle_p->p = (void *)(uintptr_t)CmdRsp.Handle;
    *Size_p = CmdRsp.uint1;

    // ask the kernel driver to remember the mapping info
    // as we need it in the free function
    CmdRsp.Opcode = UMDEVXS_OPCODE_SMBUF_SETBUFINFO;
    //CmdRsp.Handle (already set)
    //CmdRsp.uint1 (already set)
    CmdRsp.ptr1 = p;

    // talk to the kernel
    res = UMDevXSProxy_DoCmdRsp(&CmdRsp);
    if (res < 0)
        return res-6;

    if (CmdRsp.Error != 0)
        return -10;

    return 0; // success
}
#endif /* UMDEVXSPROXY_REMOVE_SMBUF_ATTACH */
#endif /* UMDEVXSPROXY_REMOVE_SMBUF */


/*----------------------------------------------------------------------------
 * UMDevXSProxy_SHMem_Detach
 *
 * This function unmaps a block of memory. The memory block is removed from
 * the caller's memory map.
 *
 * Return Value
 *     0  Success
 *    -1  Failed to ...
 */
#ifndef UMDEVXSPROXY_REMOVE_SMBUF
#ifndef UMDEVXSPROXY_REMOVE_SMBUF_ATTACH
int
UMDevXSProxy_SHMem_Detach(
        const UMDevXSProxy_SHMem_Handle_t Handle)
{
    UMDevXS_CmdRsp_t CmdRsp;
    int res;

    // zero-init also protects against future extensions
    ZEROINIT(CmdRsp);

    CmdRsp.Opcode = UMDEVXS_OPCODE_SMBUF_GETBUFINFO;
    CmdRsp.Handle = (int)(uintptr_t)Handle.p;

    // talk to the kernel
    res = UMDevXSProxy_DoCmdRsp(&CmdRsp);

    // found the buffer?
    if (res < 0)
        return res;

    if (CmdRsp.Error != 0)
        return -4;

    // first unmap the buffer from the application memory map
    res = UMDevXSProxy_Unmap(CmdRsp.ptr1, CmdRsp.uint1);
    if (res != 0)
        return -5;

    // next, forget about the buffer using the cmd/rsp interface
    ZEROINIT(CmdRsp);

    CmdRsp.Opcode = UMDEVXS_OPCODE_SMBUF_DETACH;
    CmdRsp.Handle = (int)(uintptr_t)Handle.p;

    // talk to the kernel
    res = UMDevXSProxy_DoCmdRsp(&CmdRsp);
    if (res < 0)
        return res-5;

    if (CmdRsp.Error != 0)
        return -9;

    return 0; // success
}
#endif /* UMDEVXSPROXY_REMOVE_SMBUF_ATTACH */
#endif /* UMDEVXSPROXY_REMOVE_SMBUF */


/*----------------------------------------------------------------------------
 * UMDevXSProxy_SHMem_Commit
 *
 * This function ensures the [subset of the] buffer is committed from a cache
 * (if any) to system memory, to ensure the other host can read it.
 */
#ifndef UMDEVXSPROXY_REMOVE_SMBUF
#ifndef UMDEVXSPROXY_REMOVE_SMBUF_COMMIT
void
UMDevXSProxy_SHMem_Commit(
        const UMDevXSProxy_SHMem_Handle_t Handle,
        const unsigned int SubsetStart,
        const unsigned int SubsetLength)
{
    UMDevXS_CmdRsp_t CmdRsp;

    // zero-init also protects against future extensions
    ZEROINIT(CmdRsp);

    CmdRsp.Opcode = UMDEVXS_OPCODE_SMBUF_COMMIT;
    CmdRsp.Handle = (int)(uintptr_t)Handle.p;
    CmdRsp.uint1 = SubsetStart;
    CmdRsp.uint2 = SubsetLength;

    // talk to the kernel
    // no error handling
    (void)UMDevXSProxy_DoCmdRsp(&CmdRsp);
}
#endif /* UMDEVXSPROXY_REMOVE_SMBUF_COMMIT */
#endif /* UMDEVXSPROXY_REMOVE_SMBUF */


/*----------------------------------------------------------------------------
 * UMDevXSProxy_SHMem_Refresh
 *
 * This function ensures the [subset of the] buffer is refreshed from system
 * memory into the cache (if any), to ensure we are not looking at old data
 * that has been replaced by the other host.
 */
#ifndef UMDEVXSPROXY_REMOVE_SMBUF
#ifndef UMDEVXSPROXY_REMOVE_SMBUF_COMMIT
void
UMDevXSProxy_SHMem_Refresh(
        const UMDevXSProxy_SHMem_Handle_t Handle,
        const unsigned int SubsetStart,
        const unsigned int SubsetLength)
{
    UMDevXS_CmdRsp_t CmdRsp;

    // zero-init also protects against future extensions
    ZEROINIT(CmdRsp);

    CmdRsp.Opcode = UMDEVXS_OPCODE_SMBUF_REFRESH;
    CmdRsp.Handle = (int)(uintptr_t)Handle.p;
    CmdRsp.uint1 = SubsetStart;
    CmdRsp.uint2 = SubsetLength;

    // talk to the kernel
    // no error handling
    (void)UMDevXSProxy_DoCmdRsp(&CmdRsp);
}
#endif /* UMDEVXSPROXY_REMOVE_SMBUF_COMMIT */
#endif /* UMDEVXSPROXY_REMOVE_SMBUF */


/*----------------------------------------------------------------------------
 * UMDevXSProxy_Interrupt_WaitWithTimeout
 *
 * Return Value
 *     0  Return due to interrupt
 *     1  Return due to timeout
 *    <0  Error code
 */
#ifndef UMDEVXSPROXY_REMOVE_INTERRUPT
int
UMDevXSProxy_Interrupt_WaitWithTimeout(
        const unsigned int Timeout_ms,
        const int IntId)
{
    char Fake;

    if (UMDevXSProxy_fd < 0)
        return -1;

    if (IntId < 0 || IntId > 0xFF)
        return -2;

    // read() is a blocking call,
    // the timeout and interrupt ID are sent as the length,
    // returns 0, 1 or -1
    return (int)read(UMDevXSProxy_fd,
                    &Fake,
                     (((unsigned int)IntId & UMDEVXS_INTERRUPT_INT_ID_MASK) <<
                          UMDEVXS_INTERRUPT_INT_ID_OFFSET) |
                      (Timeout_ms & UMDEVXS_INTERRUPT_TIMEOUT_MASK));
}
#endif /* UMDEVXSPROXY_REMOVE_INTERRUPT */


/*----------------------------------------------------------------------------
 * UMDevXSProxy_Device_PciCfg_Read32
 */
#ifndef UMDEVXSPROXY_REMOVE_PCICFG
int
UMDevXSProxy_Device_PciCfg_Read32(
        const unsigned int ByteOffset,
        unsigned int * const Int32_p)
{
    UMDevXS_CmdRsp_t CmdRsp;
    int res;

    if (Int32_p == NULL)
        return -4; // bad parameters

    // zero-init also protects against future extensions
    ZEROINIT(CmdRsp);

    CmdRsp.Opcode = UMDEVXS_OPCODE_DEVICE_PCICFG_READ32;
    CmdRsp.uint1 = ByteOffset;
    CmdRsp.Magic = UMDEVXS_CMDRSP_MAGIC;

    // talk to the kernel
    res = UMDevXSProxy_DoCmdRsp(&CmdRsp);
    if (res < 0)
        return res;

    if (CmdRsp.Error != 0)
        return -5;

    *Int32_p = CmdRsp.uint2;

    return 0; // success
}
#endif // UMDEVXSPROXY_REMOVE_PCICFG


/*----------------------------------------------------------------------------
 * UMDevXSProxy_Device_PciCfg_Write32
 */
#ifndef UMDEVXSPROXY_REMOVE_PCICFG
int
UMDevXSProxy_Device_PciCfg_Write32(
        const unsigned int ByteOffset,
        const unsigned int Int32)
{
    UMDevXS_CmdRsp_t CmdRsp;
    int res;

    // zero-init also protects against future extensions
    ZEROINIT(CmdRsp);

    CmdRsp.Opcode = UMDEVXS_OPCODE_DEVICE_PCICFG_WRITE32;
    CmdRsp.uint1 = ByteOffset;
    CmdRsp.uint2 = Int32;
    CmdRsp.Magic = UMDEVXS_CMDRSP_MAGIC;

    // talk to the kernel
    res = UMDevXSProxy_DoCmdRsp(&CmdRsp);
    if (res < 0)
        return res;

    if (CmdRsp.Error != 0)
        return -4;

    return 0; // success
}
#endif // UMDEVXSPROXY_REMOVE_PCICFG


/* end of file umdevxsproxy.c */
