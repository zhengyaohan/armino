/* da_val.c
 *
 * Demo Application for the VAL API
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

#ifdef DA_VAL_USERMODE
#include "da_val.h"                     // da_val_main()
#endif


/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */

#include "c_da_val.h"                   // configuration

#include "log.h"
#include "api_driver_init.h"
#include "api_val.h"


/*----------------------------------------------------------------------------
 * Definitions and macros
 */

#ifdef DA_VAL_USERMODE
#include <stdlib.h>  // malloc, free, atoi
#define da_val_malloc(s) malloc(s)
#define da_val_free(s)   free(s)
#else // DA_VAL_USERMODE
#define EXPORT_SYMTAB

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>       // printk
#include <linux/slab.h>
#include <asm/delay.h>          // udelay

#define da_val_malloc(s) kmalloc(s, GFP_KERNEL)
#define da_val_free(s)   kfree(s)

MODULE_LICENSE(DA_VAL_LICENSE);
MODULE_AUTHOR("Inside Secure");
MODULE_DESCRIPTION("Demo program for the VAL API.");
#endif // !DA_VAL_USERMODE

#define DA_PREFIX   "DA_VAL: "


/*----------------------------------------------------------------------------
 * da_val_module_init
 */
static int
da_val_module_init(void)
{
    int rc = -1;
    uint8_t * DataIn_p = NULL;
    const unsigned int DataInByteCount = 40;
    ValOctetsOut_t * DataOut_p = NULL;
    ValSize_t DataOutByteCount = 100;
    ValStatus_t FuncRes;
    uint8_t i;

    LOG_CRIT(DA_PREFIX "Starting Demo Application\n");

    // Allocate DMA buffers
    DataOut_p = da_val_malloc(DataOutByteCount);
    if (DataOut_p == NULL)
    {
        LOG_CRIT(DA_PREFIX "Failed to allocate DMA output buffer\n");
        return -1;
    }
    memset(DataOut_p, 0, DataOutByteCount);

    DataIn_p = da_val_malloc(DataInByteCount);
    if (DataIn_p == NULL)
    {
        LOG_CRIT(DA_PREFIX "Failed to allocate DMA input buffer\n");
        goto exit;
    }

    // Get version information to check token functionality
    LOG_CRIT(DA_PREFIX "Start Get Version test\n");

    FuncRes = val_SystemGetVersion(DataOut_p,
                                   &DataOutByteCount);
    if(FuncRes == VAL_SUCCESS)
    {
        LOG_CRIT(DA_PREFIX "Version: %s\n", DataOut_p);
        LOG_CRIT(DA_PREFIX "Get Version test PASSED\n");
    }
    else
    {
        LOG_CRIT(DA_PREFIX "Version retrieving failed (%d)\n", FuncRes);
        LOG_CRIT(DA_PREFIX "Get Version test FAILED\n");
        goto exit;
    }

    // Get state information
    LOG_CRIT(DA_PREFIX "Start Get State test\n");
    {
        uint8_t OtpErrorCode = 0;
        uint16_t OtpErrorLocation = 0;
        uint8_t Mode = 0;
        uint8_t ErrorTest = 0;
        uint8_t CryptoOfficer = 0;
        uint8_t HostID = 0;
        uint8_t NonSecure = 0;
        uint32_t Identity = 0;

        FuncRes = val_SystemGetState(&OtpErrorCode, &OtpErrorLocation,
                                     &Mode, &ErrorTest, &CryptoOfficer,
                                     &HostID, &NonSecure, &Identity);
        if(FuncRes == VAL_SUCCESS)
        {
            LOG_CRIT(DA_PREFIX "  State:\n");
            LOG_CRIT(DA_PREFIX "    OTP State     : %u (%u)\n",
                     OtpErrorCode, OtpErrorLocation);
            LOG_CRIT(DA_PREFIX "    Mode          : %u (0x%02X)\n", Mode, ErrorTest);
            LOG_CRIT(DA_PREFIX "    CryptoOfficer : %sAvailable\n",
                      CryptoOfficer ? "" : "NOT ");
            LOG_CRIT(DA_PREFIX "    HostID        : %u\n", HostID);
            LOG_CRIT(DA_PREFIX "    Secure        : %s\n", NonSecure ? "No" : "Yes ");
            LOG_CRIT(DA_PREFIX "    Identity      : 0x%X\n", Identity);
            LOG_CRIT(DA_PREFIX "Get State test PASSED\n");
        }
        else
        {
            LOG_CRIT(DA_PREFIX "State retrieving failed (%d)\n", FuncRes);
            LOG_CRIT(DA_PREFIX "Get State test FAILED\n");
            goto exit;
        }
    }

    // Scan OTP objects
    LOG_CRIT(DA_PREFIX "OTP Scan: Start\n");
    for (i = 0; i < 64; i++)
    {
        ValAssetId_t AssetId;

        FuncRes = val_AssetSearch(i, &AssetId, &DataOutByteCount);
        if (FuncRes == VAL_SUCCESS)
        {
            const char * pType = "Private Static Asset";
            uint8_t * pData = NULL;

            FuncRes = val_PublicDataRead(AssetId, DataOut_p, DataOutByteCount);
            if ((FuncRes == VAL_SUCCESS) ||
                (FuncRes == VAL_BUFFER_TOO_SMALL))
            {
                pType = "Public Data object";
                if (FuncRes == VAL_SUCCESS)
                {
                    pData = DataOut_p;
                }
            }
            else
            {
                FuncRes = val_MonotonicCounterRead(AssetId, DataOut_p, DataOutByteCount);
                if ((FuncRes == VAL_SUCCESS) ||
                    (FuncRes == VAL_BUFFER_TOO_SMALL))
                {
                    pType = "Monotonic Counter";
                    if (FuncRes == VAL_SUCCESS)
                    {
                        pData = DataOut_p;
                    }
                }
            }
            LOG_CRIT(DA_PREFIX "  Found Asset number %d (%s), size %d bytes\n",
                     (int)i, pType, (int)DataOutByteCount);
            if (pData != NULL)
            {
                Log_HexDump(DA_PREFIX "    Data", 0, pData, (unsigned int)DataOutByteCount);
            }
        }
        else if (FuncRes == VAL_INTERNAL_ERROR)
        {
            LOG_CRIT(DA_PREFIX "OTP Scan: FAILED\n");
            break;
        }
    }
    if (i == 64)
    {
        LOG_CRIT(DA_PREFIX "OTP Scan: PASSED\n");
    }

    // Perform a NOP operation to check the DMA functionality
    LOG_CRIT(DA_PREFIX "Start NOP test\n");

    for(i = 0; i < DataInByteCount; i++)
    {
        DataIn_p[i] = i;
    }

    FuncRes = val_NOP(DataIn_p, DataOut_p, DataInByteCount);
    if(FuncRes == VAL_SUCCESS)
    {
        for(i = 0; i < DataInByteCount; i++)
        {
            if (DataOut_p[i] != i)
            {
                LOG_CRIT(DA_PREFIX "Data check failed\n");
                LOG_CRIT(DA_PREFIX "NOP test FAILED\n");
                goto exit;
            }
        }
        LOG_CRIT(DA_PREFIX "NOP test PASSED\n");
    }
    else
    {
        LOG_CRIT(DA_PREFIX "Operation failed (%d)\n", FuncRes);
        LOG_CRIT(DA_PREFIX "NOP test FAILED\n");
        goto exit;
    }

    rc = 0; // success

exit:
    if (DataIn_p != NULL)
    {
        da_val_free(DataIn_p);
    }
    if (DataOut_p != NULL)
    {
        da_val_free(DataOut_p);
    }

    return rc;
}


/*----------------------------------------------------------------------------
 * da_val_module_exit
 */
static void
da_val_module_exit(void)
{
    LOG_CRIT(DA_PREFIX "Stopping Demo Application\n");
}


#ifdef DA_VAL_USERMODE
/*----------------------------------------------------------------------------
 * da_val_main
 */
int
da_val_main(
        int argc,
        char **argv)
{
    int i;
    bool fHelp = false;

    if (Driver130_Init() < 0)
    {
        LOG_CRIT(DA_PREFIX "Demo Application: Driver130_Init() failed\n");
        return -1;
    }

    // Process option arguments
    for (i = 1; argc > i; i++)
    {
        ValStatus_t FuncRes;

        if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-help"))
        {
            LOG_CRIT(DA_PREFIX "Example application for functionality provided by the VAL API.\n\n");
            LOG_CRIT("Syntax     : %s [<option> ...]\n", argv[0]);
            LOG_CRIT("Description: Execute the Demo Application. Note that you can\n");
            LOG_CRIT("             use the options to perform an operation to set\n");
            LOG_CRIT("             the hardware in a certain state.\n");
            LOG_CRIT("Options    : -help              Display this information\n");
            LOG_CRIT("             -initotp           Initialize the OTP with a default COID and HUK\n");
            LOG_CRIT("             -reset             Reset firmware (hardware)\n");
            LOG_CRIT("             -trng[=<Samples>]  Configure/Start the TRNG (default=3072)\n");

            fHelp = true;
            continue;
        }

        if (strcmp(argv[i], "-reset") == 0)
        {
            // Reset system to have known starting state
            LOG_CRIT(DA_PREFIX "Reset system\n");
            FuncRes = val_SystemReset();
            if(FuncRes == VAL_SUCCESS)
            {
                LOG_CRIT(DA_PREFIX "Reset system PASSED\n");
            }
            else
            {
                LOG_CRIT(DA_PREFIX "Reset system FAILED\n");
            }
            fHelp = true;
        }
        else if (strncmp(argv[i], "-trng", 5) == 0)
        {
            uint16_t SampleCycles = 3072;

            if (argv[i][5] == '=')
            {
                SampleCycles = (uint16_t)atoi(&argv[i][6]);
                if (SampleCycles == 0)
                {
                    SampleCycles = 2;
                }
            }

            LOG_CRIT(DA_PREFIX "Configure TRNG (%u)\n", SampleCycles);
            FuncRes = val_TrngConfig(0, SampleCycles, 1, 2, 1);
            if(FuncRes == VAL_SUCCESS)
            {
                LOG_CRIT(DA_PREFIX "Configure TRNG PASSED\n");
            }
            else
            {
                LOG_CRIT(DA_PREFIX "Configure TRNG FAILED\n");
                fHelp = true;
            }
        }
        else if (strcmp(argv[i], "-initotp") == 0)
        {
            static const uint8_t AssetData[] =
            {
                0x18, 0x6E, 0x2E, 0xA2, 0x32, 0x09, 0x5B, 0x4A, 0x17, 0xCD, 0xA0, 0xDA, 0x8C, 0xB8, 0x88, 0xED,
                0x2B, 0x33, 0x8A, 0x33, 0xE6, 0x35, 0xC9, 0x8B, 0x20, 0x24, 0x3B, 0x44, 0x5B, 0x39, 0x4B, 0xDD,
                0x98, 0x11, 0x37, 0xF0, 0x96, 0x20, 0xB3, 0x34, 0x6E, 0xC4, 0xDE, 0xCB, 0xC4, 0x34, 0x53, 0x63,
            };
            static const uint8_t ADLabel[] = "SomeAssociatedDataForProvisioningWithKeyBlob";

            LOG_CRIT(DA_PREFIX "Program HUK and Crypto Officer ID\n");
            FuncRes = val_OTPDataWrite(0, 1, true, ADLabel, 44, AssetData, sizeof(AssetData));
            if (FuncRes != VAL_SUCCESS)
            {
                LOG_CRIT(DA_PREFIX "FAILED: OTP data write (%d).\n", (int)FuncRes);
                fHelp = true;
            }
            else
            {
                LOG_CRIT(DA_PREFIX "PASSED: OTP data written.\n");

                LOG_CRIT(DA_PREFIX "Reset system\n");
                FuncRes = val_SystemReset();
                if(FuncRes == VAL_SUCCESS)
                {
                    LOG_CRIT(DA_PREFIX "Reset system PASSED\n");
                }
                else
                {
                    LOG_CRIT(DA_PREFIX "Reset system FAILED\n");
                }
            }
        }
    }

    if (fHelp == false)
    {
        da_val_module_init();
        da_val_module_exit();
    }

    Driver130_Exit();

    return 0;
}


#ifndef DA_VAL_MAIN_REMOVE
/*----------------------------------------------------------------------------
 * main
 */
int
main(
        int argc,
        char ** argv)
{
    return da_val_main(argc, argv);
}
#endif // !DA_VAL_MAIN_REMOVE
#else // DA_VAL_USERMODE
module_init(da_val_module_init);
module_exit(da_val_module_exit);
#endif // !DA_VAL_USERMODE


/* end of file da_val.c */


