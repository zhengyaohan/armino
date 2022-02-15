/* c_adapter_bufmanager.h
 *
 * Default Buffer Manager Adapter Module Configuration
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

#ifndef C_ADAPTER_BUFMANAGER_H_
#define C_ADAPTER_BUFMANAGER_H_

/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */

// Top-level Driver Framework configuration
#include "cs_adapter.h"


/*----------------------------------------------------------------------------
 * Definitions and macros
 */

// Enable swapping bytes in words written to or read from DMA buffers.
// Data is written to DMA buffer as bytes, not as words, so typically this
// byte swap is not needed, but on some systems it may be required to perform
// this byte swap in the DMA buffers, for example if the interconnect performs
// the byte swap in hardware for the DMA transfers.
//#define ADAPTER_BUFMAN_SWAP_ENABLE

// The number of polling attempts without delay in between
#ifndef ADAPTER_BUFMAN_POLLING_SKIP_FIRST_DELAYS
#define ADAPTER_BUFMAN_POLLING_SKIP_FIRST_DELAYS    50
#endif

// The delay (wait) time in milliseconds
// between the two consecutive polling attempts
#ifndef ADAPTER_BUFMAN_POLLING_DELAY_MS
#define ADAPTER_BUFMAN_POLLING_DELAY_MS             1
#endif

// Maximum number of polling attempts
#ifndef ADAPTER_BUFMAN_POLLING_MAXLOOPS
#define ADAPTER_BUFMAN_POLLING_MAXLOOPS             5000
#endif


#endif /* C_ADAPTER_BUFMANAGER_H_ */


/* end of file c_adapter_bufmanager.h */

