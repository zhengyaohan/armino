/* memxs_testapp.c
 *
 * Simple test program to read and write registers on a device
 * using the MemXS module.
 *
 * Commands:
 * -i Input file          --- Read configuration from the input file.
 *                            A sample input file "config" is available.
 * -a Address Space Id    --- Device name.
 * -w Address Data        --- write Data on Address.
 * -r Address Data        --- read value from Address.
 *                            If "Data" is also provided
 *                            then match the value obtained with the data.
 * -h Help                --- This Message
 *
 */

/*****************************************************************************
* Copyright (c) 2010-2016 INSIDE Secure B.V. All Rights Reserved.
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
 * This module uses (requires) the following interface(s):
 */

// Default configuration
#include "c_memxs.h"

// MemXS API
#include "api_memxs.h"

// Driver Framework Device API
#include "device_mgmt.h" // Device_Initialize()

// Logging API
#undef LOG_SEVERITY_MAX
#define LOG_SEVERITY_MAX    MEMXS_LOG_SEVERITY

// Logging API
#include "log.h"

// External interfaces
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>


/*----------------------------------------------------------------------------
 * Definitions and macros
 */

#define CMD_WRITE 1
#define CMD_READ 2
#define LINE_BUF_SIZE 80

#define check_ret(ret1, ret2) \
    if (ret1 != ret2) \
    { \
        usage (); \
        exit (1); \
    }

#define FIND_NEXT_SPACE(ptr, len) \
      while ((len) > 0 && !isspace(*(ptr))) \
        { \
          (ptr)++; \
          (len)--; \
        }

#define FIND_NEXT_NON_SPACE(ptr, len) \
      while ((len) > 0 && isspace(*(ptr))) \
        { \
          (ptr)++; \
          (len)--; \
        }

#define MOVE_AHEAD(ptr, len) \
  if (len > 0) \
    { \
      ptr++;\
      len--;\
    }

typedef struct
{
    int cmd;
    MemXS_Handle_t AddrSpaceId;
    int RegAddr;
    unsigned int Data;
    bool DataWasRead;
} CommandContext_t;


/*----------------------------------------------------------------------------
 * Local variables
 */


/*----------------------------------------------------------------------------
 * usage
 *
 */
static void
usage(
        void)
{
    unsigned int i, MemXS_Device_Count;
    MemXS_DevInfo_t MemXS_DeviceInfo;

    MemXS_Device_Count_Get(&MemXS_Device_Count);

    LOG_CRIT
    ("\nUsage: ./memxs [ADDRESS SPACE] [READ|WRITE] REGISTER_ADDRESS DATA \n "
      "Or:   ./memxs [FILE]\n\n"
      "Either -i must be used or complete command must be provided at the "
      "command line.\n"
      "Details of configuration file format are present in the "
      "README and sample\n"
      "file config\n\n"
      " Examples: \n"
      " # Write: ./memxs -a <DEVICE_NAME> -w REGISTER_ADDRESS DATA.\n"
      " # Read:  ./memxs -a <DEVICE_NAME> -r REGISTER_ADDRESS.\n"
      " # Use config file: ./memxs -i config\n\n"
      "ARGUMENTS:\n\n"
      "  -i FILE:                  Read command from the configuration file.\n"
      "  -a <DEVICE_NAME>:         Device name\n"
      "  -w REGISTER_ADDRESS DATA: Write data on the register address.\n"
      "  -r REGISTER_ADDRESS:      Read value from the register address.\n");

    LOG_CRIT ("\n\nNr\t Device Name \t First Offset \t Last Offset\n");
    for(i = 0; i < MemXS_Device_Count; i++)
    {
        MemXS_Device_Info_Get(i, &MemXS_DeviceInfo);
        LOG_CRIT ("%d\t%s\t0x%08x\t0x%08x\n",
                  i,
                  MemXS_DeviceInfo.Name_p,
                  MemXS_DeviceInfo.FirstOfs,
                  MemXS_DeviceInfo.LastOfs);
    }
}


/*----------------------------------------------------------------------------
 * check_command
 *
 */
static bool
check_command(
        CommandContext_t *ctx)
{
    bool ret = true;

    if (MemXS_Handle_IsSame(&ctx->AddrSpaceId, &MemXS_NULLHandle))
    {
        LOG_CRIT ("ERR: Invalid Address space ID provided.\n");
        ret = false;
        goto END;
    }

    if (ctx->cmd != CMD_READ && ctx->cmd != CMD_WRITE)
    {
        LOG_CRIT ("ERR: Invalid command provided.\n");
        ret = false;
        goto END;
    }

    if (ctx->RegAddr < 0)
    {
        LOG_CRIT ("ERR: Register address not provided.\n");
        ret = false;
        goto END;
    }

    // check whether data value was provided for write command
    if (ctx->cmd == CMD_WRITE && ctx->DataWasRead == false)
    {
        LOG_CRIT ("ERR: Data not provided.\n");
        ret = false;
        goto END;
    }

END:
    return ret;
}


/*----------------------------------------------------------------------------
 * FillAddressSpaceId
 *
 */
static bool
FillAddressSpaceId(
        char *tok,
        CommandContext_t *ctx)
{
    int i;
    bool ret = false;
    unsigned int MemXS_Device_Count;
    MemXS_DevInfo_t MemXS_DeviceInfo;
    MemXS_Device_Count_Get(&MemXS_Device_Count);

    ctx->AddrSpaceId = MemXS_NULLHandle;

    for (i = 0; i < (int)MemXS_Device_Count; i++)
    {
        MemXS_Device_Info_Get(i, &MemXS_DeviceInfo);
        if (0 == strcasecmp (tok, MemXS_DeviceInfo.Name_p))
        {
            ctx->AddrSpaceId = MemXS_DeviceInfo.Handle;
            ret = true;
        }
    }

    if (!ret)
        LOG_CRIT ("ERR: Invalid address space ID specified.\n");

    return ret;
}


/*----------------------------------------------------------------------------
 * FillOperation
 *
 */
static bool
FillOperation(
        char *tok,
        CommandContext_t *ctx)
{
    bool ret = false;

    if (0 == (strcasecmp (tok, "read")))
    {
        ctx->cmd = CMD_READ;
        ret = true;
    }
    else if (0 == (strcasecmp (tok, "write")))
    {
        ctx->cmd = CMD_WRITE;
        ret = true;
    }
    else
    {
        LOG_CRIT ("ERR: Invalid command specified. Correct options are "
                 "read|READ or write|WRITE.\n");
    }

    return ret;
}


/*----------------------------------------------------------------------------
 * FillRegisterOrData
 *
 */
static bool
FillRegisterOrData(
        char *tok,
        CommandContext_t *ctx,
        bool ByteOffset)
{
    bool ret = false;
    unsigned int val = 0;

    val = strtoul (tok, 0, 0);
    LOG_CRIT(">> 0x%x\n", val);

    if (val != ERANGE || val != EINVAL)
    {
        if (ByteOffset)
        {
            ctx->RegAddr = val;
        }
        else
        {
            ctx->Data = val;
            ctx->DataWasRead = true;
        }

        ret = true;
    }
    else
    {
        LOG_CRIT ("ERR: Invalid value specified for register/ data.\n");
    }

    return ret;
}


/*----------------------------------------------------------------------------
 * FillCommandContext
 *
 */
static bool
FillCommandContext(
        char *tok,
        int arg,
        CommandContext_t *ctx)
{
    bool ret = false;

    switch (arg)
    {
        case 1:
           ret = FillAddressSpaceId (tok, ctx);
           break;
        case 2:
           ret = FillOperation (tok, ctx);
           break;
        case 3:
           ret = FillRegisterOrData (tok, ctx, true);
           break;
        case 4:
           ret = FillRegisterOrData (tok, ctx, false);
           break;
        default:
           break;
    }

    return ret;
}


/*----------------------------------------------------------------------------
 * ExecuteCommand
 *
 */
static void
ExecuteCommand(
        CommandContext_t *ctx)
{
    unsigned int value = 0;

    if (ctx->cmd == CMD_WRITE)
    {
        MemXS_Write32 (ctx->AddrSpaceId,
                       ctx->RegAddr,
                       ctx->Data);
    }
    else
    {
        value = MemXS_Read32 (ctx->AddrSpaceId,
                              ctx->RegAddr);
        LOG_CRIT ("Value read from offset 0x%x is 0x%x\n", ctx->RegAddr,
                  value);
    }

}


/*----------------------------------------------------------------------------
 * Test_ConfigFile
 *
 */
static int
Test_ConfigFile(
        const char *FileName)
{
    FILE *f;
    char line[80];
    char *p, * curr;
    int length = 0;
    int arg = 0;
    bool ret = true;
    CommandContext_t ctx;

    // reset all context
    memset (&ctx, 0x0, sizeof (ctx));

    f = fopen (FileName, "r");
    if (f == NULL)
    {
        LOG_CRIT ("ERR: Error opening config file.\n");
        return 1;
    }

    while (f)
    {
        if (!fgets (line, sizeof (line), f))
        {
            fclose (f);
            return 0;
        }
        length = strlen (line);
        curr = p = line;

        while (*p != '\0' && isspace (*p))
        {
            p++;    /*Skip white spaces */
            length--;
        }

        /* Skip comments and empty lines */
        if (*p == '\0' || *p == '\n' || *p == '#')
        {
            continue;
        }

        while (length > 0)
        {
            arg++;
            FIND_NEXT_SPACE (p, length);
            *p = '\0';

            ret = FillCommandContext (curr, arg, &ctx);
            check_ret (ret, true);

            MOVE_AHEAD (p, length);
            FIND_NEXT_NON_SPACE (p, length);
            curr = p;
        }


        ret = check_command (&ctx);
        check_ret (ret, true);
        ExecuteCommand (&ctx);
        arg = length = 0;
    }

    fclose (f);
    return 0;
}


/*----------------------------------------------------------------------------
 * main
 *
 */
int
main(
        int argc,
        char *argv[])
{
    int opt_index = 1;
    const char *InputFileName = NULL;
    CommandContext_t CommandCtx;
    bool ret = false;
    int nIRQ = -1;

    // reset all context
    memset (&CommandCtx, 0x0, sizeof (CommandCtx));
    CommandCtx.RegAddr = -1;
    CommandCtx.AddrSpaceId = MemXS_NULLHandle;

    if (Device_Initialize(&nIRQ) != 0 )
    {
        LOG_CRIT ("MemXS_Init: Device_Initialize failed\n");
        exit(1);
    }

    if (!MemXS_Init ())
    {
        LOG_CRIT ("ERR: Module initialization failed. Exiting...\n");
        exit (1);
    }

    while (opt_index < argc)
    {
        char *opt_str = argv[opt_index++];
        char *optval;

        if ((opt_str[0] != '-') || (strlen (opt_str) != 2))
        {
            LOG_CRIT ("Err: Invalid option %s.\n", opt_str);
            exit (1);
        }

        optval = argv[opt_index++];
        if (opt_index > argc)
        {
            LOG_CRIT ("ERR: Value not provided for argument \" %s \"\n",
                    opt_str);
            usage ();
            exit (1);
        }

        switch (opt_str[1])
        {
            case 'i':
                InputFileName = optval;
                break;

            case 'a':
                ret = FillAddressSpaceId (optval, &CommandCtx);
                check_ret (ret, true);
                break;

            case 'w':
                ret = FillRegisterOrData (optval, &CommandCtx, true);
                check_ret (ret, true);
                if (opt_index < argc)
                {
                    optval = argv[opt_index++];
                    ret = FillRegisterOrData (optval, &CommandCtx, false);
                    check_ret (ret, true);
                }
            CommandCtx.cmd = CMD_WRITE;
            break;

            case 'r':
                ret = FillRegisterOrData (optval, &CommandCtx, true);
                check_ret (ret, true);
                CommandCtx.cmd = CMD_READ;
            break;

            case 'h':
                usage ();
                exit (0);

            default:
                LOG_CRIT ("ERR: Unrecognized option\n");
                usage ();
                exit (0);
        } // switch
    } // while


    if (InputFileName != NULL)
    {
        Test_ConfigFile (InputFileName);
    }
    else
    {
        ret = check_command (&CommandCtx);
        check_ret (ret, true);
        ExecuteCommand (&CommandCtx);
    }

    exit (0);
}


/* end of file memxs_testapp.c */
