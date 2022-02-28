/* umdevxsproxy_shmem_obtainer.h
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

#ifndef INCLUDE_GUARD_UMDEVXSPROXY_SHMEM_OBTAINER_H
#define INCLUDE_GUARD_UMDEVXSPROXY_SHMEM_OBTAINER_H

#include "umdevxsproxy_shmem_types.h"


/*----------------------------------------------------------------------------
 * UMDevXSProxy_SHMem_Attach
 *
 * This routine allocates a block of memory that can be used for device DMA.
 * The memory is allocated in kernel space and mapped into the user's address
 * space. A handle and a pointer to the start of the memory block are
 * returned.
 *
 * Return Value
 *     0  Success
 *    -1  Failed to ...
 */
int
UMDevXSProxy_SHMem_Attach(
        const UMDevXSProxy_SHMem_GBI_t * const GBI_p,
        UMDevXSProxy_SHMem_Handle_t * const Handle_p,
        UMDevXSProxy_SHMem_BufPtr_t * const BufPtr_p,
        unsigned int * const Size_p);


/*----------------------------------------------------------------------------
 * UMDevXSProxy_SHMem_Detach
 *
 * This function detaches from a block of shared memory.
 * The memory block is removed from the caller's memory map.
 *
 * Return Value
 *     0  Success
 *    -1  Failed to ...
 */
int
UMDevXSProxy_SHMem_Detach(
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


#endif /* INCLUDE_GUARD_UMDEVXSPROXY_SHMEM_OBTAINER_H */

/* umdevxsproxy_shmem_obtainer.h */
