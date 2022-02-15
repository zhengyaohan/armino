/* da_otpkeyblob.c
 *
 * Demo Application intended as an OTP key blob generation example.
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

#include "da_otpkeyblob.h"              // da_otpkeyblob_main()


/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */

#include "c_da_otpkeyblob.h"            // configuration

#include "aessiv.h"
#include "log.h"
#include "api_driver_init.h"
#include "api_val.h"

#ifndef DA_OTPKEYBLOB_FS_REMOVE
#include <stdio.h>      // fopen, fclose, fread
#endif


/*----------------------------------------------------------------------------
 * Definitions and macros
 */
#include <stdlib.h>  // malloc, free
#include <unistd.h>  // usleep

#define da_malloc(s)    malloc(s)
#define da_free(s)      free(s)
#define da_usleep(s)    usleep(s)

#define DA_ARG_SEEN_KEY     0x01
#define DA_ARG_SEEN_LABEL   0x02
#define DA_ARG_SEEN_POLICY  0x04
#define DA_ARG_SEEN_DATA    0x08
#define DA_ARG_SEEN_OUTPUT  0x10

#ifndef DA_OTPKEYBLOB_FS_REMOVE
#define DA_ARG_SEEN_OK      (DA_ARG_SEEN_KEY    | DA_ARG_SEEN_LABEL |  \
                             DA_ARG_SEEN_POLICY | DA_ARG_SEEN_DATA  |  \
                             DA_ARG_SEEN_OUTPUT)
#else
#define DA_ARG_SEEN_OK      (DA_ARG_SEEN_KEY    | DA_ARG_SEEN_LABEL |  \
                             DA_ARG_SEEN_POLICY | DA_ARG_SEEN_DATA)
#endif

#define DA_PREFIX       "DA_OTPKB: "


/*----------------------------------------------------------------------------
 * Local variables
 */
static char * gl_Program = NULL;
static bool gl_Verbose = false;

static uint8_t gl_AES_SIV_Key[2*(256/8)];
static uint8_t gl_KeyBlob_Label[256];
static uint8_t * gl_AD_Label = NULL;
static size_t gl_AD_LabelSize = 0;
static uint8_t gl_AssetPolicy[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static uint8_t gl_AssetData[1024];
static size_t gl_AssetDataSize = 0;
static uint8_t gl_Output[1024+(128/8)];
static char * gl_OutputFile = NULL;
static AESSIV_Context gl_AESSIV_Context;

static int gl_ArgumentSeen = 0;


/*----------------------------------------------------------------------------
 * HandleArguments
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
        LOG_CRIT(DA_PREFIX "Example application for OTP Key Blob generation.\n\n"
                 "Usage: %s [-help] [-verbose] key [file] label <data> policy <number> data [file] output [file]\n\n"
                 "Arguments:\n"
                 " -help            Display this information.\n"
                 " -verbose         Enable process logging.\n"
                 " -exit            Exit without performing OTP Key Blob generation.\n"
                 " key [file]       Specifies the file name of the binary file in which the AES-SIV key is given,\n"
                 "                  file name is optional for hosts without file-systems.\n"
                 " label <data>     Specifies the Associated Data that must be used.\n"
                 " policy <number>  Specifies the policy number of the asset:\n"
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
                 " data [file]      Specifies the file name of the binary file in which the Asset data is given,\n"
                 "                  file name is optional for hosts without file-systems.\n"
                 " output [file]    Specifies the file name of the file to which the key blob data must be written,\n"
                 "                  file name is optional for hosts without file-systems.\n",
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
        else if (!strcmp(argv[i], "-exit"))
        {
            return 1;
        }

        if ((i + 1) >= argc)
        {
            return DisplayUsage("Wrong number of arguments");
        }

        if (!strcmp(argv[i], "key"))
        {
            size_t rv;
#ifdef DA_OTPKEYBLOB_FS_REMOVE
            extern uint8_t _binary_da_otpkeyblob_keyblob_bin_start;
            extern uint8_t _binary_da_otpkeyblob_keyblob_bin_end;
            extern unsigned int _binary_da_otpkeyblob_keyblob_bin_size;

            uint8_t * start = (uint8_t*)&_binary_da_otpkeyblob_keyblob_bin_start;
            uint8_t * end = (uint8_t*)&_binary_da_otpkeyblob_keyblob_bin_end;
            uint32_t * size = (uint32_t*)&_binary_da_otpkeyblob_keyblob_bin_size;
            uint8_t * p = gl_AES_SIV_Key;

            rv = (size_t)size;

            // Read key blob
            while(start < end)
            {
                *p = *start;
                start++;
                p++;
            }
#else
            FILE * fp;

            // Read the AES-SIV key from the specified file
            fp = fopen(argv[i + 1], "rb");
            if (fp == NULL)
            {
                return DisplayUsage("Cannot open key file");
            }

            rv = fread(gl_AES_SIV_Key, 1, sizeof(gl_AES_SIV_Key), fp);
            fclose(fp);
#endif
            if (rv != sizeof(gl_AES_SIV_Key))
            {
                return DisplayUsage("Cannot read key file or key has not the expected size");
            }

            gl_ArgumentSeen |= DA_ARG_SEEN_KEY;
        }
        else if (!strcmp(argv[i], "label"))
        {
            // Assign Associated Data (label)
            gl_AD_Label = (uint8_t *)argv[i + 1];
            gl_AD_LabelSize = strlen(argv[i + 1]);

            gl_ArgumentSeen |= DA_ARG_SEEN_LABEL;
        }
        else if (!strcmp(argv[i], "policy"))
        {
            // Read and validate policy number
            switch (atoi(argv[i + 1]))
            {
            default:
                return DisplayUsage("Wrong policy number");

            case 1:
                gl_AssetPolicy[4] = 0x02;
                break;

            case 8:
            case 2:
                gl_AssetPolicy[3] = 0x20;
                gl_AssetPolicy[4] = 0x20;
                break;

            case 3:
                gl_AssetPolicy[4] = 0x04;
                break;

            case 4:
                gl_AssetPolicy[7] = 0x10;
                break;

            case 6:
                gl_AssetPolicy[0] = 0x04;
                gl_AssetPolicy[3] = 0x0C;
                gl_AssetPolicy[7] = 0x04;
                break;

            case 7:
            case 24:
            case 25:
            case 26:
            case 28:
                gl_AssetPolicy[7] = 0x08;
                break;
            }

            gl_ArgumentSeen |= DA_ARG_SEEN_POLICY;
        }
        else if (!strcmp(argv[i], "data"))
        {
#ifdef DA_OTPKEYBLOB_FS_REMOVE
            extern uint8_t _binary_da_otpkeyblob_asset_bin_start;
            extern uint8_t _binary_da_otpkeyblob_asset_bin_end;
            extern unsigned int _binary_da_otpkeyblob_asset_bin_size;

            uint8_t * start = (uint8_t*)&_binary_da_otpkeyblob_asset_bin_start;
            uint8_t * end = (uint8_t*)&_binary_da_otpkeyblob_asset_bin_end;
            uint32_t * size = (uint32_t*)&_binary_da_otpkeyblob_asset_bin_size;
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
            FILE * fp;

            // Read the Asset data from the specified file
            fp = fopen(argv[i + 1], "rb");
            if (fp == NULL)
            {
                return DisplayUsage("Cannot open data file");
            }

            gl_AssetDataSize = fread(gl_AssetData, 1, sizeof(gl_AssetData), fp);
            fclose(fp);
#endif

            if (gl_AssetDataSize > 0)
            {
                gl_ArgumentSeen |= DA_ARG_SEEN_DATA;
            }
            else
            {
                return DisplayUsage("Cannot read data file");
            }

        }
        else if (!strcmp(argv[i], "output"))
        {
            // Assign output file name
            gl_OutputFile = argv[i + 1];

            gl_ArgumentSeen |= DA_ARG_SEEN_OUTPUT;
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
 * GenerateKeyBlob
 */
static int
GenerateKeyBlob(void)
{
    int FuncReturnCode = -1;
    size_t OutputSize = sizeof(gl_Output);

    FuncReturnCode = AESSIV_Init(&gl_AESSIV_Context, gl_Verbose);
    if (FuncReturnCode != 0)
    {
        return DisplayUsage("KeyBlob setup failed (Init)");
    }

    FuncReturnCode = AESSIV_SetKey(&gl_AESSIV_Context,
                                   gl_AES_SIV_Key, sizeof(gl_AES_SIV_Key));
    if (FuncReturnCode != 0)
    {
        return DisplayUsage("KeyBlob setup failed (Key)");
    }

    memcpy(gl_KeyBlob_Label, gl_AD_Label, gl_AD_LabelSize);
    memcpy(&gl_KeyBlob_Label[gl_AD_LabelSize], gl_AssetPolicy, 8);
    FuncReturnCode = AESSIV_SetAD(&gl_AESSIV_Context,
                                  gl_KeyBlob_Label, (gl_AD_LabelSize+8));
    if (FuncReturnCode != 0)
    {
        return DisplayUsage("KeyBlob setup failed (AD)");
    }

    FuncReturnCode = AESSIV_Encrypt(&gl_AESSIV_Context,
                                    gl_AssetData, gl_AssetDataSize,
                                    gl_Output, &OutputSize);
    if (FuncReturnCode != 0)
    {
        return DisplayUsage("KeyBlob generation failed");
    }

    // If file system is not supported then the output data can be stored
    // anywhere in memory
#ifndef DA_OTPKEYBLOB_FS_REMOVE
    {
        FILE *fp;

        fp = fopen(gl_OutputFile, "wb");
        if (fp == NULL)
        {
            return DisplayUsage("Cannot open output file");
        }
        FuncReturnCode = (int)fwrite(gl_Output, OutputSize, 1, fp);
        fclose(fp);
        if (FuncReturnCode != 1)
        {
            return DisplayUsage("Cannot write output file");
        }
    }
#endif

    return 0;
}


/*----------------------------------------------------------------------------
 * da_otpkeyblob_main
 */
int
da_otpkeyblob_main(
        int argc,
        char * argv[])
{
    int res, MainReturnCode;

    // Parse command line arguments.
    res = HandleArguments(argc, argv);
    if (res < 0)
    {
        return -1;                      // some error
    }
    else if (res > 0)
    {
        return 0;                       // exit normally
    }

    if (gl_ArgumentSeen != DA_ARG_SEEN_OK)
    {
        return DisplayUsage("Not all required arguments specified");
    }

    if (Driver130_Init() < 0)
    {
        LOG_CRIT(DA_PREFIX "FAILED: Driver130 initialization\n");
        return -1;
    }

    MainReturnCode = GenerateKeyBlob();

    Driver130_Exit();

    return MainReturnCode;
}


#ifndef DA_OTPKEYBLOB_MAIN_REMOVE
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
    return da_otpkeyblob_main(argc, argv);
}
#endif


/* end of file da_otpkeyblob.c */
