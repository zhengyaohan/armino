/* api_driver_gc_init.h
 *
 * EIP-13x Global Control Driver Initialization Interface
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

#ifndef DRIVER_GC_INIT_H_
#define DRIVER_GC_INIT_H_


/*----------------------------------------------------------------------------
 * Driver130_Global_Init
 *
 * Initialize the driver. This function must be called before any other
 * driver API function can be called.
 *
 * Returns 0 for success and -1 for failure.
 */
int
Driver130_Global_Init(void);

/*----------------------------------------------------------------------------
 * Driver130_Global_Exit
 *
 * Initialize the driver. After this function is called no other driver API
 * function can be called except Driver130_Global_Init().
 */
void
Driver130_Global_Exit(void);


#endif /* DRIVER_GC_INIT_H_ */


/* end of file api_driver_gc_init.h */
