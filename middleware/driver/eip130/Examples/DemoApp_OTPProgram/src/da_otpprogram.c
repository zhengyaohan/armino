/* da_otpprogram.c
 *
 * Demo Application intended as an OTP programming use example.
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

#include "da_otpprogram.h"              // da_otpprogram_main()


/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */

#include "c_da_otpprogram.h"            // configuration

#include "log.h"
#include "api_driver_init.h"
#include "api_val.h"

#ifndef DA_OTPPROGRAM_KEYBLOB_FS_REMOVE
#include <stdio.h>      // fopen, fclose, fread
#endif


/*----------------------------------------------------------------------------
 * Definitions and macros
 */
#include <stdlib.h>  // malloc, sleep
#include <unistd.h>  // usleep

#define da_malloc(s)    malloc(s)
#define da_free(s)      free(s)
#define da_usleep(s)    usleep(s)

#define DA_ARG_SEEN_BLOB    0x01
#define DA_ARG_SEEN_ASSET   0x02
#define DA_ARG_SEEN_POLICY  0x04
#define DA_ARG_SEEN_LABEL   0x08
#define DA_ARG_SEEN_CRC     0x10
#define DA_ARG_SEEN_OK      (DA_ARG_SEEN_BLOB|DA_ARG_SEEN_ASSET|DA_ARG_SEEN_POLICY|DA_ARG_SEEN_LABEL)

#define DA_PREFIX       "DA_OTPPRG: "


/*----------------------------------------------------------------------------
 * Local variables
 */

static char * gl_Program = NULL;

static uint8_t * gl_AD_Label = NULL;
static size_t gl_AD_LabelSize = 0;
static uint8_t gl_AssetNumber = 0;
static uint8_t gl_AssetPolicy = 0;
static uint8_t gl_AssetData[2048+16];
static size_t gl_AssetDataSize = 0;

static bool gl_Verbose = false;
static int gl_ArgumentSeen = 0;


/*----------------------------------------------------------------------------
 * DisplayUsage
 *
 * Handle usage display & usage error situations.
 * This function never returns, instead calls exit with given code.
 */
static int
DisplayUsage(
        const char * error)
{
    if (error)
    {
        // On usage errors, give error message and point user to -help.
        LOG_CRIT(DA_PREFIX "FAILED: %s.\n"
                 DA_PREFIX "Try '%s -help' for usage.\n",
                 error, gl_Program);
    }
    else
    {
        // Otherwise, build usage instructions.
        LOG_CRIT(DA_PREFIX "Example application for OTP programming.\n\n"
                 "Usage: %s [-help] [-verbose] blob [file] asset <number> policy <number> label <data> [crc]\n\n"
                 "Arguments:\n"
                 " -help            Display this information.\n"
                 " -verbose         Enable logging.\n"
                 " -exit            Exit without performing OTP programming.\n"
                 " blob [file]      Specifies the file name of the key blob data file,\n"
                 "                  file name is optional for hosts without file-systems.\n"
                 " asset <number>   Specifies the asset number of the asset [0-30].\n"
                 " policy <number>  Specifies the policy number of the asset:\n"
                 "                  0  - Monotonic Counter\n"
                 "                  1  - TRUSTED-KDK-DERIVE (HUK)\n"
                 "                  2  - AES key unwrap key (SB)\n"
                 "                  3  - TRUSTED-KEY-DERIVE (SB)\n"
                 "                  4  - Public Data (SB) for Public key or SHA256 Public key digest\n"
                 "                  6  - Auth Key for eMMC (AK)\n"
                 "                  7  - Private Data for Milenage K and OPc\n"
                 "                  8  - AES Unwrap key, (Secure Debug)\n"
                 "                  24 - AES key unwrap key (FWSB)\n"
                 "                  25 - TRUSTED-KEY-DERIVE (FWSB)\n"
                 "                  26 - SHA256 Public key digest (FWSB)\n"
                 "                  28 - JTAG Secure Debug Public Key (Secure Debug)\n"
                 " label <data>     Specifies the Associated Data that must be used.\n"
                 " crc              Specifies that the asset must be protected with an CRC.\n",
                 gl_Program);
    }

    return -1;
}


/*----------------------------------------------------------------------------
 * HandleArguments
 *
 * Handles command line arguments.
 */
static int
HandleArguments(
        int argc,
        char * argv[])
{
    int i;

    gl_Program = argv[0];

    i = 1;
    while (i < argc)
    {
        if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-help"))
        {
            (void)DisplayUsage(NULL);
            return 1;
        }

        if (!strcmp(argv[i], "-verbose"))
        {
            gl_Verbose = true;
            i += 1;
            continue;
        }
        else if (!strcmp(argv[i], "crc"))
        {
            // CRC Asset
            gl_ArgumentSeen |= DA_ARG_SEEN_CRC;
            i += 1;
            continue;
        }
        else if (!strcmp(argv[i], "-exit"))
        {
            return 1;
        }

        if ((i + 1) >= argc)
        {
            return DisplayUsage("Wrong number of arguments");
        }

        if (!strcmp(argv[i], "blob"))
        {
#ifdef DA_OTPPROGRAM_KEYBLOB_FS_REMOVE
            extern uint8_t _binary_da_otpprogram_keyblob_bin_start;
            extern uint8_t _binary_da_otpprogram_keyblob_bin_end;
            extern unsigned int _binary_da_otpprogram_keyblob_bin_size;

            uint8_t * start = (uint8_t*)&_binary_da_otpprogram_keyblob_bin_start;
            uint8_t * end = (uint8_t*)&_binary_da_otpprogram_keyblob_bin_end;
            uint32_t * size = (uint32_t*)&_binary_da_otpprogram_keyblob_bin_size;
            uint8_t * p = gl_AssetData;

            gl_AssetDataSize = (size_t)size;

            // Read key blob
            while(start < end)
            {
                *p = *start;
                start++;
                p++;
            }
#else
            FILE *fp;

            // Read the AES-SIV key from the specified file
            fp = fopen(argv[i + 1], "rb");
            if (fp == NULL)
            {
                return DisplayUsage("Cannot open key blob file");
            }

            gl_AssetDataSize = fread(gl_AssetData, 1, sizeof(gl_AssetData), fp);
            fclose(fp);
#endif
            if (gl_AssetDataSize > 0)
            {
                gl_ArgumentSeen |= DA_ARG_SEEN_BLOB;
            }
            else
            {
                return DisplayUsage("Cannot read key blob file");
            }
        }
        else if (!strcmp(argv[i], "asset"))
        {
            // Read and validate asset number
            gl_AssetNumber = (uint8_t)atoi(argv[i + 1]);
            if (gl_AssetNumber > 30)
            {
                return DisplayUsage("Wrong asset number");
            }
            gl_ArgumentSeen |= DA_ARG_SEEN_ASSET;
        }
        else if (!strcmp(argv[i], "policy"))
        {
            // Read and validate policy number
            gl_AssetPolicy = (uint8_t)atoi(argv[i + 1]);
            switch (gl_AssetPolicy)
            {
            default:
                return DisplayUsage("Wrong policy number");

            case 0:     // Monotonic Counter
            case 1:     // TRUSTED-KDK-DERIVE (HUK)
            case 2:     // SBCR: AES key unwrap key (SB)
            case 3:     // PUK: TRUSTED-KEY-DERIVE (SB)
            case 4:     // Public Data, general use
            case 6:     // Auth Key for eMMC (AK)
            case 7:     // Private Data for Milenage K and OPc
            case 8:     // AES Unwrap key, Secure Debug usage restriction
            case 24:    // Private Data, AES key unwrap key (FWSB)
            case 25:    // Private Data, TRUSTED-KEY-DERIVE (FWSB)
            case 26:    // Private Data, SHA256 Public key digest (FWSB)
            case 28:    // Private Data, JTAG Secure Debug Public Key
                break;
            }

            gl_ArgumentSeen |= DA_ARG_SEEN_POLICY;
        }
        else if (!strcmp(argv[i], "label"))
        {
            // Assign Associated Data (label)
            gl_AD_Label = (uint8_t *)argv[i + 1];
            gl_AD_LabelSize = strlen(argv[i + 1]);

            gl_ArgumentSeen |= DA_ARG_SEEN_LABEL;
        }
        else
        {
            return DisplayUsage("Invalid option specified.");
        }
        i += 2;
    }
    return 0;
}


/*----------------------------------------------------------------------------
 * da_otpprogram_main
 */
int
da_otpprogram_main(
        int argc,
        char * argv[])
{
    int res, MainReturnCode = -1;
    ValStatus_t funcres;

    // Parse command line arguments.
    res = HandleArguments(argc, argv);
    if (res < 0)
    {
        return -1; // some error
    }
    else if (res > 0)
    {
        return 0; // exit normally
    }

    if ((gl_ArgumentSeen & DA_ARG_SEEN_OK) != DA_ARG_SEEN_OK)
    {
        return DisplayUsage("Not all required arguments specified");
    }

    if (Driver130_Init() < 0)
    {
        LOG_CRIT(DA_PREFIX "FAILED: Driver130 initialization\n");
    }
    else
    {
        if (val_IsAccessSecure())
        {
            if (gl_Verbose)
            {
                LOG_CRIT(DA_PREFIX "OTP Write information\n");
                LOG_CRIT("Asset number : %d\n", gl_AssetNumber);
                LOG_CRIT("Asset policy : %d\n", gl_AssetPolicy);
                LOG_CRIT("AD Label     : %s\n", gl_AD_Label);
                LOG_CRIT("CRC          : %s\n", (gl_ArgumentSeen & DA_ARG_SEEN_CRC) ? "Yes" : "No");
                Log_HexDump("OTP BlobData", 0, gl_AssetData, (unsigned int)gl_AssetDataSize);
            }

            funcres = val_OTPDataWrite(gl_AssetNumber, gl_AssetPolicy,
                                       ((gl_ArgumentSeen & DA_ARG_SEEN_CRC) != 0),
                                       gl_AD_Label, gl_AD_LabelSize,
                                       gl_AssetData, gl_AssetDataSize);
            if (funcres != VAL_SUCCESS)
            {
                LOG_CRIT(DA_PREFIX "FAILED: OTP data write (%d).\n", (int)funcres);
            }
            else
            {
                LOG_CRIT(DA_PREFIX "PASSED: OTP data written.\n");
                LOG_CRIT("NOTE: Hardware must be 'reset' to enable Asset for use.\n");
                MainReturnCode = 0;
            }
        }
        else
        {
            LOG_CRIT(DA_PREFIX "FAILED: No secure hardware connection.\n");
        }

        Driver130_Exit();
    }

    return MainReturnCode;
}


#ifndef DA_OTPPROGRAM_MAIN_REMOVE
/*----------------------------------------------------------------------------
 * main
 *
 * Program entry.
 */
int
main(
        int argc,
        char * argv[])
{
    return da_otpprogram_main(argc, argv);
}
#endif


/* end of file da_otpprogram.c */
