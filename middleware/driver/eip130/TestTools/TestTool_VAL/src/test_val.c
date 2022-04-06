/* test_val.c
 *
 * Test Tool for VAL API
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

/*----------------------------------------------------------------------------
 * This module implements (provides) the following interface(s):
 */

#ifdef TEST_VAL_USERMODE
#include "test_val.h"                   // test_val_main()
#endif


/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */

#include "c_test_val.h"                 // configuration

// Driver Framework C Run-time abstraction
#include "clib.h"

// Logging API
#include "log.h"

// Driver Initialization API
#include "api_driver_init.h"

#include "sfzutf_internal.h"            // test environment
#include "valtest_internal.h"           // test suite setup


/*----------------------------------------------------------------------------
 * Definitions and macros
 */
#ifndef TEST_VAL_USERMODE
MODULE_LICENSE(TEST_VAL_LICENSE);
MODULE_AUTHOR("Inside Secure");
MODULE_DESCRIPTION("Test program for the VAL API.");
#endif


/*----------------------------------------------------------------------------
 * Global variables
 */
int g_BeginVector = 0;
uint16_t g_TrngSampleCycles = 3072;
bool g_ResetAllowed = true;
bool g_RepeatVector = false;
bool g_CleanUp = true;


/*----------------------------------------------------------------------------
 * Local variables
 */
static char * gl_SuiteName_p = NULL;
static char * gl_TCaseName_p = NULL;
static char * gl_TestName_p = NULL;
static int * gl_IterValue_p = NULL;
#ifdef TEST_VAL_USERMODE
static int gl_IterValue = 0;
#endif
static SfzUtfEnable_t gl_EnableMode = SFZUTF_ENABLE_ALL;


/*----------------------------------------------------------------------------
 * test_val_module_init
 *
 * Initializes and runs the tests for the VAL API
 */
static int
test_val_module_init(void)
{
    LOG_CRIT("Starting Test Application\n");

    if (build_suite() == 0)
    {
        struct TestSuite * Suite_p = sfzutf_tsuite_get(NULL);
        if (Suite_p != NULL)
        {
            if (sfzutf_tsuite_enable(Suite_p,
                                     gl_SuiteName_p,
                                     gl_TCaseName_p,
                                     gl_TestName_p,
                                     gl_IterValue_p,
                                     gl_EnableMode))
            {
                int number_failed = sfzutf_tsuite_run(Suite_p);
                if (number_failed == 0)
                {
                    LOG_CRIT("SUCCESS: All tests executed\n");
                }
                else
                {
                    if (number_failed < 0)
                    {
                        LOG_CRIT("FAILED: Test execution aborted\n");
                    }
                    else
                    {
                        LOG_CRIT("FAILED: %d tests\n", number_failed);
                    }
                }
            }
            else
            {
                LOG_CRIT("FAILED: No tests enabled\n");
            }
        }
        else
        {
            LOG_CRIT("FAILED: No test suite to test\n");
        }
    }
    else
    {
        LOG_CRIT("FAILED: Test suite setup\n");
    }

    sfzutf_tsuite_release(NULL);

    return 0;
}


/*----------------------------------------------------------------------------
 * test_val_module_exit
 *
 * Stops the tests.
 */
static void
test_val_module_exit(void)
{
    LOG_CRIT("Stopping Test Application\n");
}


#ifdef TEST_VAL_USERMODE
/*----------------------------------------------------------------------------
 * ConvertToLower
 */
static void
ConvertToLower(
        char *p)
{
    while(*p != 0)
    {
        if ((*p >= 'A') && (*p <= 'Z'))
        {
            *p = *p | 0x60;
        }
        p++;
    }
}


/*----------------------------------------------------------------------------
 * GetDecimalValue
 */
static int
GetDecimalValue(
        char *p)
{
    int value = 0;

    while (*p != 0)
    {
        if ((*p >= '0') && (*p <= '9'))
        {
            value = (value * 10) + (int)(*p - '0');
        }
        else
        {
            break;
        }
        p++;
    }

    return value;
}


/*----------------------------------------------------------------------------
 * test_val_main
 */
int
test_val_main(
        int argc,
        char **argv)
{
    bool fRunTests = true;

    if (Driver130_Init() < 0)
    {
        LOG_CRIT("TestTool VAL: Driver130_Init() failed\n");
        return -1;
    }

    if (argc > 1)
    {
        int i;

        // Process options
        for (i = 1; i < argc; i++)
        {
            ConvertToLower(argv[i]);
            if (strcmp(argv[i], "-case") == 0)
            {
                i++; // Increment to jump on next argument
                if (i < argc)
                {
                    gl_TCaseName_p = argv[i];
                    LOG_CRIT("Start TestCase '%s'\n", gl_TCaseName_p);
                }
            }
            else if (strcmp(argv[i], "-test") == 0)
            {
                i++; // Increment to jump on next argument
                if (i < argc)
                {
                    gl_TestName_p = argv[i];
                    LOG_CRIT("Start Test '%s'\n", gl_TestName_p);
                }
            }
            else if (strcmp(argv[i], "-iter") == 0)
            {
                i++; // Increment to jump on next argument
                if (i < argc)
                {
                    if ((gl_TCaseName_p != NULL) &&
                        (gl_TestName_p != NULL))
                    {
                        gl_IterValue = GetDecimalValue(argv[i]);
                        gl_IterValue_p = &gl_IterValue;
                        LOG_CRIT("Start at Iterator %d\n", gl_IterValue);
                    }
                }
            }
            else if (strcmp(argv[i], "-mode") == 0)
            {
                i++; // Increment to jump on next argument
                if (i < argc)
                {
                    ConvertToLower(argv[i]);

                    if (strcmp(argv[i], "all") == 0)
                    {
                        gl_EnableMode = SFZUTF_ENABLE_ALL;
                        LOG_CRIT("EnableMode = SFZUTF_ENABLE_ALL\n");
                    }
                    else if (strcmp(argv[i], "single") == 0)
                    {
                        gl_EnableMode = SFZUTF_ENABLE_SINGLE;
                        LOG_CRIT("EnableMode = SFZUTF_ENABLE_SINGLE\n");
                    }
                    else if (strcmp(argv[i], "after") == 0)
                    {
                        gl_EnableMode = SFZUTF_ENABLE_AFTER;
                        LOG_CRIT("EnableMode = SFZUTF_ENABLE_AFTER\n");
                    }
                }
            }
            else if (strcmp(argv[i], "-noreset") == 0)
            {
                // Disable reset
                g_ResetAllowed = false;
            }
            else if (strcmp(argv[i], "-noclean") == 0)
            {
                // Disable clean up
                g_CleanUp = false;
            }
            else if (strncmp(argv[i], "-vector=", 8) == 0)
            {
                g_BeginVector = atoi(&argv[i][8]);
                if (g_BeginVector < 0)
                {
                    g_BeginVector = 0;
                }
            }
            else if (strcmp(argv[i], "-repeat") == 0)
            {
                // Enable repeat vector
                g_RepeatVector = true;
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
                ValStatus_t FuncRes;

                FuncRes = val_OTPDataWrite(0, 1, true, ADLabel, 44, AssetData, sizeof(AssetData));
                if (FuncRes != VAL_SUCCESS)
                {
                    LOG_CRIT("FAILED: OTP data write (%d).\n", (int)FuncRes);
                    fRunTests = false;
                }
                else
                {
                    LOG_CRIT("PASSED: OTP data written.\n");
                }
                if (g_ResetAllowed)
                {
                    FuncRes = val_SystemReset();
                    if (FuncRes != VAL_SUCCESS)
                    {
                        LOG_CRIT("FAILED: System reset (%d).\n", (int)FuncRes);
                        fRunTests = false;
                    }
                    else
                    {
                        LOG_CRIT("PASSED: System reset.\n");
                    }
                }
            }
            else if (strncmp(argv[i], "-trng", 5) == 0)
            {
                ValStatus_t FuncRes;

                if (argv[i][5] == '=')
                {
                    g_TrngSampleCycles = (uint16_t)atoi(&argv[i][6]);
                    if (g_TrngSampleCycles == 0)
                    {
                        g_TrngSampleCycles = 2;
                    }
                }

                LOG_CRIT("Configure TRNG: ");
                ////FuncRes = val_TrngConfig(0, g_TrngSampleCycles, 1, 2, 1);
				FuncRes = val_TrngConfig(0, g_TrngSampleCycles, 1, 2, 0, 1);	////V3.0.2
                if(FuncRes == VAL_SUCCESS)
                {
                    LOG_CRIT("PASSED\n");
                }
                else
                {
                    LOG_CRIT(" FAILED\n");
                    fRunTests = false;
                }
            }
            else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-help"))
            {
                LOG_CRIT("Syntax     : %s [<option> ...]\n", argv[0]);
                LOG_CRIT("Description: Execute the VAL API test application.\n");
                LOG_CRIT("             With the options you can specify the test application behavior.\n");
                LOG_CRIT("Options    : -help              Display this information\n");
                LOG_CRIT("             -noclean           No test cleanup\n");
                LOG_CRIT("             -noreset           No reset during tests is allowed\n");
                LOG_CRIT("             -initotp           Initialize the OTP with a default COID and HUK\n");
                LOG_CRIT("             -trng[=<Samples>]  Configure/Start the TRNG (default=3072)\n");
                LOG_CRIT("             -case <Test Case>  Define the test case to run\n");
                LOG_CRIT("             -test <Test>       Define the test to run\n");
                LOG_CRIT("             -iter <Iteration>  Define the iteration of the test/test case to run\n");
                LOG_CRIT("             -mode <mode>       Define the test enable mode:\n");
                LOG_CRIT("                                all     All tests including tests after the specified test\n");
                LOG_CRIT("                                single  Only the specified test\n");
                LOG_CRIT("                                after   Including all tests after the specified test\n");
                LOG_CRIT("             -vector=<Index>    Vector index number [0...]\n");
                LOG_CRIT("             -repeat            Repeat a vector till error or program is terminated\n");
                fRunTests = false;
            }
            else
            {
                LOG_CRIT("Illegal option: %s\n", argv[i]);
            }
        }
    }
#if 0	////POCHIN
	 static const uint8_t AssetData[] =
     {
         0x18, 0x6E, 0x2E, 0xA2, 0x32, 0x09, 0x5B, 0x4A, 0x17, 0xCD, 0xA0, 0xDA, 0x8C, 0xB8, 0x88, 0xED,
         0x2B, 0x33, 0x8A, 0x33, 0xE6, 0x35, 0xC9, 0x8B, 0x20, 0x24, 0x3B, 0x44, 0x5B, 0x39, 0x4B, 0xDD,
         0x98, 0x11, 0x37, 0xF0, 0x96, 0x20, 0xB3, 0x34, 0x6E, 0xC4, 0xDE, 0xCB, 0xC4, 0x34, 0x53, 0x63,
     };
     static const uint8_t ADLabel[] = "SomeAssociatedDataForProvisioningWithKeyBlob";
     ValStatus_t FuncRes;

     FuncRes = val_OTPDataWrite(0, 1, true, ADLabel, 44, AssetData, sizeof(AssetData));
     if (FuncRes != VAL_SUCCESS)
     {
         LOG_CRIT("FAILED: OTP data write (%d).\n", (int)FuncRes);
         fRunTests = false;
     }
     else
     {
         LOG_CRIT("PASSED: OTP data written.\n");
     }
     if (g_ResetAllowed)
     {
         FuncRes = val_SystemReset();
         if (FuncRes != VAL_SUCCESS)
         {
             LOG_CRIT("FAILED: System reset (%d).\n", (int)FuncRes);
             fRunTests = false;
         }
         else
         {
             LOG_CRIT("PASSED: System reset.\n");
         }
     }
	 
	 
	ValAssetId_t RootAssetId = VAL_ASSETID_INVALID;
	RootAssetId = val_AssetGetRootKey();
	if(RootAssetId == VAL_ASSETID_INVALID)
	{
		LOG_INFO("No Root key!! \r\n");
		LOG_INFO("Do not write Root key NOW!! \r\n");
////			FuncRes = WriteRandomHuk(0, false, true);
	}
	else
	{
		LOG_INFO("RootAssetId = 0x%X \r\n", RootAssetId);
	}
	 
	 
#endif
    if (fRunTests)
    {
        (void)test_val_module_init();
        test_val_module_exit();
    }

    Driver130_Exit();

    return 0;
}


#ifndef TEST_VAL_MAIN_REMOVE
/*----------------------------------------------------------------------------
 * main
 */
int
main(
        int argc,
        char **argv)
{
    return test_val_main(argc, argv);
}
#endif // !TEST_VAL_MAIN_REMOVE
#else // TEST_VAL_USERMODE
module_init(test_val_module_init);
module_exit(test_val_module_exit);
#endif // !TEST_VAL_USERMODE


/* end of file test_val.c */

