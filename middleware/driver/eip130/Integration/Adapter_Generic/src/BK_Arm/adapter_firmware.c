/* adapter_firmware.c
 *
 * User-space implementation of Interface for obtaining the firmware image.
 * Read from file data segment.
 */

/*****************************************************************************
* Copyright (c) 2016-2017 INSIDE Secure B.V. All Rights Reserved.
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

#include "adapter_firmware.h"

/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */

// Default configuration
#include "c_adapter_firmware.h"

// Driver Framework Basic Defs API
#include "basic_defs.h"

// Logging API
#include "log.h"

#include "adapter_alloc.h"

#include "clib.h"


/*----------------------------------------------------------------------------
 * Definitions and macros
 */

/*----------------------------------------------------------------------------
 * Adapter_Firmware_NULL
 *
 */
const Adapter_Firmware_t Adapter_Firmware_NULL = NULL;

extern uint8_t _binary__lib_firmware_firmware_eip130ram_sbi_start;
extern uint8_t _binary__lib_firmware_firmware_eip130ram_sbi_end;
extern unsigned int _binary__lib_firmware_firmware_eip130ram_sbi_size;


/*----------------------------------------------------------------------------
 * Adapter_Firmware_Acquire
 */
Adapter_Firmware_t
Adapter_Firmware_Acquire(
        const char * Firmware_Name_p,
        const uint32_t ** Firmware_p,
        unsigned int * Firmware_Word32Count)
{
    unsigned int Firmware_ByteCount, i;
    uint32_t * Firmware_Data_p;

    uint8_t * p;
    uint8_t * fw_start = (uint8_t*)&_binary__lib_firmware_firmware_eip130ram_sbi_start;
    uint8_t * fw_end = (uint8_t*)&_binary__lib_firmware_firmware_eip130ram_sbi_end;
    uint32_t * fw_size = (uint32_t*)&_binary__lib_firmware_firmware_eip130ram_sbi_size;

    Firmware_ByteCount = (unsigned int)fw_size;

    LOG_INFO("%s for %s\n", __func__, Firmware_Name_p);

    // Initialize output parameters.
    *Firmware_p = NULL;
    *Firmware_Word32Count = 0;

    LOG_INFO("%s: firmware file size %d\n", __func__, Firmware_ByteCount);

    if (Firmware_ByteCount == 0 ||
        Firmware_ByteCount >= 256*1024 ||
        Firmware_ByteCount % sizeof(uint32_t) != 0)
    {
        LOG_CRIT("%s: Invalid file size %d\n", __func__, Firmware_ByteCount);
        return NULL;
    }

    // Allocate buffer for data
    p = Adapter_Alloc(Firmware_ByteCount);
    if (p == NULL)
    {
        LOG_CRIT("%s: Failed to allocate\n", __func__);
        return NULL;
    }

    Firmware_Data_p = (uint32_t*)p;

    // Read from firmware blob
    while(fw_start < fw_end)
    {
        *p = *fw_start;
        fw_start++;
        p++;
    }

    // Convert bytes from file to array of 32-bit words (in-place).
    {
        p = (uint8_t *)Firmware_Data_p;

        for (i = 0; i < Firmware_ByteCount / sizeof(uint32_t); i++)
        {
            Firmware_Data_p[i] = p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24);

            p += sizeof(uint32_t);
        }
    }

    // Pass results to caller
    *Firmware_p             = Firmware_Data_p;
    *Firmware_Word32Count   = Firmware_ByteCount / sizeof(uint32_t);

    IDENTIFIER_NOT_USED(Firmware_Name_p);

    return (Adapter_Firmware_t) Firmware_Data_p;
}


/*----------------------------------------------------------------------------
 * Adapter_Firmware_Release
 */
void
Adapter_Firmware_Release(
        Adapter_Firmware_t FirmwareHandle)
{
    LOG_INFO("%s\n", __func__);

    Adapter_Free((void*)FirmwareHandle);
}


/* end of file adapter_firmware.c */
