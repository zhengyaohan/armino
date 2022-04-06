/* umdevxsproxy_shmem_types.h
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

#ifndef INCLUDE_GUARD_UMDEVXSPROXY_SHMEM_TYPES_H
#define INCLUDE_GUARD_UMDEVXSPROXY_SHMEM_TYPES_H

typedef struct
{
    void * p;
} UMDevXSProxy_SHMem_Handle_t;


typedef struct
{
    void * p;
} UMDevXSProxy_SHMem_BufPtr_t;


typedef struct
{
    unsigned char ID[8];
} UMDevXSProxy_SHMem_GBI_t;


#endif /* INCLUDE_GUARD_UMDEVXSPROXY_SHMEM_TYPES_H */

/* umdevxsproxy_shmem_types.h */
