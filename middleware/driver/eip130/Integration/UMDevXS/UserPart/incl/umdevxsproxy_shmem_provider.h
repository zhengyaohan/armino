/* umdevxsproxy_shmem_provider.h
 *
 * This user-mode library handles the communication with the
 * Linux User Mode Device Access driver. Using the API it is possible to
 * access memory mapped devices and to enable access to foreign-allocated
 * shared memory buffers.
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

#ifndef INCLUDE_GUARD_UMDEVXSPROXY_SHMEM_PROVIDER_H
#define INCLUDE_GUARD_UMDEVXSPROXY_SHMEM_PROVIDER_H

#include "umdevxsproxy_shmem_types.h"


/*----------------------------------------------------------------------------
 * UMDevXSProxy_SHMem_Alloc
 *
 * This routine allocates a block of memory that can be shared with another
 * host. The memory is allocated in kernel space and mapped into the user's
 * address space. A handle and a pointer to the start of the memory block are
 * returned. The GBI returned can be sent to the other host.
 * The size of the block allocated is rounded up to the required alignment.
 * The aligned size is returned.
 *
 * Return Value
 *     0  Success
 *    -1  Failed to ...
 */
int
UMDevXSProxy_SHMem_Alloc(
        const unsigned int Size,
        UMDevXSProxy_SHMem_Handle_t * const Handle_p,
        UMDevXSProxy_SHMem_BufPtr_t * const BufPtr_p,
        UMDevXSProxy_SHMem_GBI_t * const GBI_p,
        unsigned int * const ActualSize_p);


/*----------------------------------------------------------------------------
 * UMDevXSProxy_SHMem_Free
 *
 * This function frees a memory block previously allocated using
 * UMDevXSProxy_SHMem_Alloc. The memory block is removed from the caller's
 * memory map and then freed. The caller must make sure the buffer is not
 * accessed anymore (also by the remote host!) before calling this function.
 *
 * Return Value
 *     0  Success
 *    -1  Failed to ...
 */
int
UMDevXSProxy_SHMem_Free(
        const UMDevXSProxy_SHMem_Handle_t Handle);


/*----------------------------------------------------------------------------
 * UMDevXSProxy_SHMem_Commit
 *
 * This function ensures the [subset of the] buffer is committed from a cache
 * (if any) to system memory, to ensure the other host can read it.
 */
void
UMDevXSProxy_SHMem_Commit(
        const UMDevXSProxy_SHMem_Handle_t Handle,
        const unsigned int SubsetStart,
        const unsigned int SubsetLength);


/*----------------------------------------------------------------------------
 * UMDevXSProxy_SHMem_Refresh
 *
 * This function ensures the [subset of the] buffer is refreshed from system
 * memory into the cache (if any), to ensure we are not looking at old data
 * that has been replaced by the other host.
 */
void
UMDevXSProxy_SHMem_Refresh(
        const UMDevXSProxy_SHMem_Handle_t Handle,
        const unsigned int SubsetStart,
        const unsigned int SubsetLength);


#endif /* INCLUDE_GUARD_UMDEVXSPROXY_SHMEM_PROVIDER_H */

/* umdevxsproxy_shmem_provider.h */
