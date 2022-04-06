/* device_platform.h
 *
 * Driver Framework platform-specific interface,
 * MicroBlaze FreeRTOS.
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

#ifndef DEVICE_PLATFORM_H_
#define DEVICE_PLATFORM_H_

/*----------------------------------------------------------------------------
 * This module implements (provides) the following interface(s):
 */



/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */


/*----------------------------------------------------------------------------
 * Definitions and macros
 */

// Internal global device administration data
typedef struct
{
    unsigned int Reserverd; // not used

} Device_Platform_Global_t;

// Internal device administration data
typedef struct
{
    // Pointer to mapped area
    uint32_t * Mem32_p;

    // Offsets w.r.t. mapped area
    unsigned int FirstOfs;
    unsigned int LastOfs;

} Device_Platform_t;


/*----------------------------------------------------------------------------
 * Local variables
 */


#endif // DEVICE_PLATFORM_H_


/* end of file device_platform.h */

