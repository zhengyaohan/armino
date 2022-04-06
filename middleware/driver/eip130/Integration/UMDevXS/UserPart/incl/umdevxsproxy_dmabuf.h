/* umdevxsproxy_dmabuf.h
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

#ifndef INCLUDE_GUARD_UMDEVXSPROXY_DMABUF_H
#define INCLUDE_GUARD_UMDEVXSPROXY_DMABUF_H

typedef struct
{
    void * p;
} UMDevXSProxy_DMABuf_Handle_t;


typedef struct
{
    void * p;
} UMDevXSProxy_DMABuf_BufPtr_t;


/*----------------------------------------------------------------------------
 * UMDevXSProxy_DMABuf_Alloc
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
UMDevXSProxy_DMABuf_Alloc(
        const unsigned int Size,
        UMDevXSProxy_DMABuf_BufPtr_t * const BufPtr_p,
        UMDevXSProxy_DMABuf_Handle_t * const Handle_p);


/*----------------------------------------------------------------------------
 * UMDevXSProxy_DMABuf_Free
 *
 * Return Value
 *     0  Success
 *    -1  Failed to ...
 */
int
UMDevXSProxy_DMABuf_Free(
        const UMDevXSProxy_DMABuf_Handle_t Handle_p);


/*----------------------------------------------------------------------------
 * UMDevXSProxy_DMABuf_Commit
 *
 */
void
UMDevXSProxy_DMABuf_Commit(
        const UMDevXSProxy_DMABuf_Handle_t Handle_p,
        const unsigned int SubsetStart,
        const unsigned int SubsetLength);


/*----------------------------------------------------------------------------
 * UMDevXSProxy_DMABuf_Refresh
 */
void
UMDevXSProxy_DMABuf_Refresh(
        const UMDevXSProxy_DMABuf_Handle_t Handle_p,
        const unsigned int SubsetStart,
        const unsigned int SubsetLength);


#endif /* INCLUDE_GUARD_UMDEVXSPROXY_DMABUF_H */

/* umdevxsproxy_dmabuf.h */
