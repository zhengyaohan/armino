/* adapter_bufmanager.c
 *
 * Buffer Manager intended for EIP-13x tokens.
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

#include "adapter_bufmanager.h"


/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */

// Default configuration
#include "c_adapter_bufmanager.h"   // ADAPTER_BUFMAN_*

#include "dmares_types.h"           // DMAResource_Handle_t
                                    // DMAResource_Properties_t
                                    // DMAResource_AddrPair_t
#include "dmares_buf.h"             // DMAResource_Alloc, DMAResource_Release
                                    // DMAResource_CheckAndRegister
#include "dmares_addr.h"            // DMAResource_Translate
#include "dmares_rw.h"              // DMAResource_PreDMA, DMAResource_PostDMA
#ifdef MODULE
#include <linux/uaccess.h>          // copy_to_user, copy_from_user
#endif

#include "clib.h"                   // ZEROINIT

#include "adapter_sleep.h"          // Adapter_SleepMS()


/*----------------------------------------------------------------------------
 * Definitions and macros
 */
#ifndef BUFMANAGER_ADMIN_ENTRIES
#define BUFMANAGER_ADMIN_ENTRIES    12
#endif


/*----------------------------------------------------------------------------
 * BufManager_AdminEntry_t
 * BufManager adminstration entry
 */
typedef struct
{
    DMAResource_Handle_t BufferHandle;  // Buffer handle
    uint64_t BufferAddress;             // Physic buffer address
    void * Buffer_p;                    // Host pointer to buffer
    size_t BufferSize;                  // Actual buffer size
    bool fBounced;                      // Bounce buffer indication
    bool fFromUserSpace;                // Data_p is from User Space indication
    BufManager_BufferType_t Type;       // Buffer type: NOT_USED,IN, OUT, INOUT
    union
    {
        const void * c_p;
        void * n_p;
    } Data;                             // Caller data buffer pointer
    size_t DataSize;                    // Caller data buffer size
    void * UserData_p;                  // Caller user data
} BufManager_AdminEntry_t;


/*----------------------------------------------------------------------------
 * Local variables
 */
static bool gl_BufManager_InitDone = false;
static BufManager_AdminEntry_t gl_BufManager_Admin[BUFMANAGER_ADMIN_ENTRIES];
static BufManager_CB_SizeAlignment_t gl_BufManager_SizeAlignment = NULL;
static BufManager_CB_CheckClear_t gl_BufManager_CheckClear = NULL;
static BufManager_CB_CheckReady_t gl_BufManager_CheckReady = NULL;


/*----------------------------------------------------------------------------
 * BufManagerLocal_GetEntry
 */
static BufManager_AdminEntry_t *
BufManagerLocal_GetEntry(
        uint64_t BufferAddress)
{
    int i;

    if (!gl_BufManager_InitDone)
    {
        // Initial the buffer manager administration
        memset(&gl_BufManager_Admin[0],
               0,
               (BUFMANAGER_ADMIN_ENTRIES * sizeof(BufManager_AdminEntry_t)));
        gl_BufManager_InitDone = true;
    }

    // Search for the requested buffer
    for (i = 0; i < BUFMANAGER_ADMIN_ENTRIES; i++)
    {
        if (gl_BufManager_Admin[i].BufferAddress == BufferAddress)
        {
            return &gl_BufManager_Admin[i];
        }
    }
    return (BufManager_AdminEntry_t *)NULL;
}


/*----------------------------------------------------------------------------
 * BufManager_Alloc
 */
static uint64_t
BufManagerLocal_AllocBuffer(
        bool fFromUserSpace,
        bool fBounce,
        BufManager_BufferType_t Type,
        size_t BufferSize,
        const void * const Data_p,
        size_t DataSize,
        void * UserData_p)
{
    uint64_t BufferAddress = 0;
    BufManager_AdminEntry_t *Entry_p = NULL;

    switch (Type)
    {
    case BUFMANAGER_BUFFERTYPE_IN:
    case BUFMANAGER_BUFFERTYPE_OUT:
    case BUFMANAGER_BUFFERTYPE_INOUT:
        Entry_p = BufManagerLocal_GetEntry(0);
        break;
    default:
        break;
    }
    if (Entry_p != NULL)
    {
        // Found a free entry
        DMAResource_Properties_t Props;
        DMAResource_AddrPair_t AddrPair;

        ZEROINIT(Props);

        Entry_p->Type = Type;
        Entry_p->fFromUserSpace = fFromUserSpace;
        Entry_p->Data.c_p = Data_p;
        Entry_p->DataSize = DataSize;
        Entry_p->UserData_p = UserData_p;

        // Determine buffer size
        Props.Alignment = 1;
        Props.Size = ((BufferSize + 3) & (unsigned int)~3);
        if (gl_BufManager_SizeAlignment &&
            ((Type == BUFMANAGER_BUFFERTYPE_OUT) ||
             (Type == BUFMANAGER_BUFFERTYPE_INOUT)))
        {
            Props.Size = (uint32_t)gl_BufManager_SizeAlignment(Props.Size);
        }
        Entry_p->BufferSize = Props.Size;

        AddrPair.Address_p = Entry_p->Data.n_p;
        AddrPair.Domain = DMARES_DOMAIN_HOST;

        if (!fFromUserSpace && !fBounce &&
            (Entry_p->BufferSize == ((DataSize + 3) & (unsigned int)~3)))
        {
            // Check if current buffer is usable
            if (DMAResource_CheckAndRegister(Props,
                                             AddrPair,
                                             'k',
                                             &Entry_p->BufferHandle) == 0)
            {
                Entry_p->Buffer_p = AddrPair.Address_p;

#ifdef ADAPTER_BUFMAN_SWAP_ENABLE
                DMAResource_SwapEndianness_Set(Entry_p->BufferHandle, true);
#endif
            }
        }

        if (Entry_p->Buffer_p == NULL)
        {
            Props.Alignment = 4;
            if (DMAResource_Alloc(Props, &AddrPair, &Entry_p->BufferHandle) < 0)
            {
                // Internal error
                goto error_func_exit;
            }
            Entry_p->Buffer_p = AddrPair.Address_p;
            Entry_p->fBounced = true;

#ifdef ADAPTER_BUFMAN_SWAP_ENABLE
            DMAResource_SwapEndianness_Set(Entry_p->BufferHandle, true);
#endif
        }

        // Translate address to get the physical address of the buffer
        if (DMAResource_Translate(Entry_p->BufferHandle,
                                  DMARES_DOMAIN_BUS,
                                  &AddrPair) != 0)
        {
            // Internal error
            goto error_func_exit;
        }
        Entry_p->BufferAddress = (uint64_t)(uintptr_t)AddrPair.Address_p;

        // If needed, clear/reset output ready check
        if (gl_BufManager_CheckClear &&
            ((Type == BUFMANAGER_BUFFERTYPE_OUT) ||
             (Type == BUFMANAGER_BUFFERTYPE_INOUT)))
        {
            if(gl_BufManager_CheckClear(Entry_p->Buffer_p,
                                        Entry_p->BufferSize,
                                        Entry_p->UserData_p) < 0)
            {
                // Internal error
                goto error_func_exit;
            }
        }

        // Copy the input data
        if (Entry_p->fBounced &&
            ((Type == BUFMANAGER_BUFFERTYPE_IN) ||
             (Type == BUFMANAGER_BUFFERTYPE_INOUT)))
        {
#ifdef MODULE
            if (fFromUserSpace)
            {
                if (copy_from_user(Entry_p->Buffer_p, Data_p, DataSize) != 0)
                {
                    // Internal error
                    goto error_func_exit;
                }
            }
            else
#endif
            {
                (void)memcpy(Entry_p->Buffer_p, Data_p, DataSize);
            }
        }
        BufferAddress = Entry_p->BufferAddress;

error_func_exit:
        if (BufferAddress == 0)
        {
            if (Entry_p->BufferHandle)
            {
                // Release the buffer
                DMAResource_Release(Entry_p->BufferHandle);
            }

            // Clear the administration entry
            memset(Entry_p, 0, sizeof(BufManager_AdminEntry_t));
        }
    }
    return BufferAddress;
}


/*----------------------------------------------------------------------------
 * BufManager_GetInAddress
 */
int
BufManager_Register(
        BufManager_CB_SizeAlignment_t SizeAlignment,
        BufManager_CB_CheckClear_t CheckClear,
        BufManager_CB_CheckReady_t CheckReady)
{
    gl_BufManager_SizeAlignment = SizeAlignment;
    gl_BufManager_CheckClear = CheckClear;
    gl_BufManager_CheckReady = CheckReady;
    return 0;
}


/*----------------------------------------------------------------------------
 * BufManager_GetInAddress
 */
uint64_t
BufManager_Map(
        bool fFromUserSpace,
        BufManager_BufferType_t Type,
        const void * const Data_p,
        size_t DataSize,
        void * UserData_p)
{
    uint64_t BufferAddress = 0;

    if (Data_p != NULL)
    {
        BufferAddress = BufManagerLocal_AllocBuffer(fFromUserSpace,
#ifdef ADAPTER_REMOVE_BOUNCEBUFFERS
                                                    false,
#else
                                                    true,
#endif
                                                    Type,
                                                    DataSize,
                                                    Data_p,
                                                    DataSize,
                                                    UserData_p);
        if (BufferAddress != 0)
        {
            BufManager_PreDmaAddress(BufferAddress);
        }
    }
    return BufferAddress;
}


/*----------------------------------------------------------------------------
 * BufManager_Alloc
 */
uint64_t
BufManager_Alloc(
        bool fFromUserSpace,
        BufManager_BufferType_t Type,
        size_t BufferSize,
        const void * const Data_p,
        size_t DataSize,
        void * UserData_p)
{
    return BufManagerLocal_AllocBuffer(fFromUserSpace,
                                       true,
                                       Type,
                                       BufferSize,
                                       Data_p,
                                       DataSize,
                                       UserData_p);
}


/*----------------------------------------------------------------------------
 * BufManager_GetInAddress
 */
int
BufManager_Unmap(
        uint64_t BufferAddress,
        bool fCopy)
{
    int rc = -1;

    if (BufferAddress != 0)
    {
        BufManager_AdminEntry_t * Entry_p;

        Entry_p = BufManagerLocal_GetEntry(BufferAddress);
        if (Entry_p != NULL)
        {
            rc = 0;                    // Looks OK

            if (fCopy  &&
                ((Entry_p->Type == BUFMANAGER_BUFFERTYPE_OUT) ||
                 (Entry_p->Type == BUFMANAGER_BUFFERTYPE_INOUT)))
            {
                DMAResource_PostDMA(Entry_p->BufferHandle,
                                    0,
                                    (unsigned int)Entry_p->BufferSize);

#ifdef ADAPTER_BUFMAN_SWAP_ENABLE
                if (DMAResource_SwapEndianness_Get(Entry_p->BufferHandle) == 1)
                {
                    uint8_t * p = Entry_p->Buffer_p;

                    p += Entry_p->BufferSize - sizeof(uint32_t);
                    DMAResource_Write32Array(
                                    Entry_p->BufferHandle,
                                    Entry_p->BufferSize / sizeof(uint32_t) - 1,
                                    1,
                                    (uint32_t*)p);
                }
#endif

                if (gl_BufManager_CheckReady != NULL)
                {
                    int SkipSleep = ADAPTER_BUFMAN_POLLING_SKIP_FIRST_DELAYS;
                    unsigned int LoopsLeft = ADAPTER_BUFMAN_POLLING_MAXLOOPS;

                    // Poll for TokenID available
                    while (rc == 0)
                    {
                        if (gl_BufManager_CheckReady(Entry_p->Buffer_p,
                                                     Entry_p->BufferSize,
                                                     Entry_p->UserData_p) == 0)
                            break; // buffer ready

                        if (SkipSleep > 0)
                        {
                            // First few rounds are without sleep
                            // this avoids sleeping unnecessarily for fast tokens
                            SkipSleep--;
                        }
                        else
                        {
                            // Sleep a bit
                            Adapter_SleepMS(ADAPTER_BUFMAN_POLLING_DELAY_MS);
                            LoopsLeft--;
                            if (LoopsLeft == 0)
                            {
                                // Report internal error
                                rc = -3; // buffer not ready, timeout
                            }
                        }

                        DMAResource_PostDMA(Entry_p->BufferHandle,
                                            0,
                                            (unsigned int)Entry_p->BufferSize);
#ifdef ADAPTER_BUFMAN_SWAP_ENABLE
                        if (DMAResource_SwapEndianness_Get(
                                         Entry_p->BufferHandle) == 1)
                        {
                            uint8_t * p = Entry_p->Buffer_p;

                            p += Entry_p->BufferSize - sizeof(uint32_t);
                            DMAResource_Write32Array(
                                            Entry_p->BufferHandle,
                                            Entry_p->BufferSize /
                                                        sizeof(uint32_t) - 1,
                                            1,
                                            (uint32_t*)p);
                        }
#endif
                    } // while
                }

                if (rc == 0)
                {
#ifdef ADAPTER_BUFMAN_SWAP_ENABLE
                    // Endianness conversion
                    if (DMAResource_SwapEndianness_Get(
                                        Entry_p->BufferHandle) == 1)
                        DMAResource_Write32Array(
                                         Entry_p->BufferHandle,
                                         0,
                                         Entry_p->BufferSize /
                                                     sizeof(uint32_t) - 1,
                                         (uint32_t*)Entry_p->Buffer_p);
#endif

                    // Copy output data
#ifdef MODULE
                    if (Entry_p->fFromUserSpace)
                    {
                        if (copy_to_user(Entry_p->Data.n_p,
                                         Entry_p->Buffer_p,
                                         Entry_p->DataSize) != 0)
                        {
                            // Report internal error
                            rc = -2;
                        }
                    }
                    else if (Entry_p->fBounced)
#else
                    if (Entry_p->fBounced)
#endif
                    {
                        (void)memcpy(Entry_p->Data.n_p,
                                     Entry_p->Buffer_p,
                                     Entry_p->DataSize);
                    } // buffer bounced
                } // buffer ready
            } // copy buffer

#ifdef ADAPTER_BUFMAN_SWAP_ENABLE
            // Unswap the bytes in words of the input buffer to restore data
            if (!Entry_p->fBounced && Entry_p->Type == BUFMANAGER_BUFFERTYPE_IN)
                DMAResource_Write32Array(
                                 Entry_p->BufferHandle,
                                 0,
                                 Entry_p->BufferSize / 4,
                                 (uint32_t*)Entry_p->Buffer_p);
#endif

            // Release buffer and clear administration entry
            DMAResource_Release(Entry_p->BufferHandle);
            memset(Entry_p, 0, sizeof(BufManager_AdminEntry_t));
        }
    }

    return rc;
}


/*----------------------------------------------------------------------------
 * BufManager_GetSize
 */
size_t
BufManager_GetSize(
        uint64_t BufferAddress)
{
    if (BufferAddress != 0)
    {
        BufManager_AdminEntry_t *Entry_p;

        Entry_p = BufManagerLocal_GetEntry(BufferAddress);
        if (Entry_p != NULL)
        {
            return Entry_p->BufferSize;
        }
    }
    return 0;
}



/*----------------------------------------------------------------------------
 * BufManager_GetHostAddress
 */
void *
BufManager_GetHostAddress(
        uint64_t BufferAddress)
{
    if (BufferAddress != 0)
    {
        BufManager_AdminEntry_t *Entry_p;

        Entry_p = BufManagerLocal_GetEntry(BufferAddress);
        if (Entry_p != NULL)
        {
            return Entry_p->Buffer_p;
        }
    }
    return NULL;
}


/*----------------------------------------------------------------------------
 * BufManager_PreDmaAddress
 */
int
BufManager_PreDmaAddress(
        uint64_t BufferAddress)
{
    if (BufferAddress != 0)
    {
        BufManager_AdminEntry_t *Entry_p;

        Entry_p = BufManagerLocal_GetEntry(BufferAddress);
        if (Entry_p != NULL)
        {
#ifdef ADAPTER_BUFMAN_SWAP_ENABLE
            if (DMAResource_SwapEndianness_Get(Entry_p->BufferHandle) == 1)
                DMAResource_Write32Array(Entry_p->BufferHandle,
                                         0,
                                         Entry_p->BufferSize / sizeof(uint32_t),
                                         (uint32_t*)Entry_p->Buffer_p);
#endif

            DMAResource_PreDMA(Entry_p->BufferHandle,
                               0,
                               (unsigned int)Entry_p->BufferSize);

            return 0;
        }
    }
    return -1;
}


/*----------------------------------------------------------------------------
 * BufManager_PostDmaAddress
 */
void
BufManager_PostDmaAddress(
        void * Buffer_p)
{
    if ((Buffer_p != NULL) && gl_BufManager_InitDone)
    {
        int i;

        // Search for the requested buffer
        for (i = 0; i < BUFMANAGER_ADMIN_ENTRIES; i++)
        {
            if (gl_BufManager_Admin[i].Buffer_p == Buffer_p)
            {
                DMAResource_PostDMA(gl_BufManager_Admin[i].BufferHandle,
                                    0,
                                    (unsigned int)gl_BufManager_Admin[i].BufferSize);
                break;
            }
        }
    }
}


/* end of file adapter_bufmanager.c */


