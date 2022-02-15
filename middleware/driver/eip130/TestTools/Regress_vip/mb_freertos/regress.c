/* regress.c
 *
 * Regression system for MicroBlaze FreeRTOS
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

/*----------------------------------------------------------------------------
 * This module implements (provides) the following interface(s):
 */


/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */

// Demo application VAL entry point
#include "da_val.h"                 // da_val_main

// Demo application VAL specials entry point
#include "da_val_specials.h"        // da_val_specials_main

// Demo application Secure Debug entry point
#include "da_securedebug.h"         // da_securedebug_main()

// Demo application OTP Key Blob entry point
#include "da_otpkeyblob.h"          // da_otpkeyblob_main()

// Demo application Encrypt Vector entry point
#include "da_encryptvector.h"       // da_da_encryptvector_main

// Demo application OTP Program entry point
#include "da_otpprogram.h"          // da_otpprogram_main()

// Test Tool VAL entry point
#include "test_val.h"               // test_val_main()

// FreeRTOS API
#include "FreeRTOS.h"
#include "task.h"


/*----------------------------------------------------------------------------
 * Definitions and macros
 */

typedef int (*Test_Function_t)(int argc, char ** argv);

typedef struct
{
    Test_Function_t func;
    int argc;
    char * argv[5];
} Test_Func_Params_t;


/*----------------------------------------------------------------------------
 * Local variables
 */

static Test_Func_Params_t TestFuncs[] =
{
#if 1
    // func               argc  argv
    { da_encryptvector_main, 2, {"da_encryptvector_main","-help", 0, 0, 0  } },
    { da_otpkeyblob_main,    3, {"da_otpkeyblob_main",   "-help",
                                                         "-exit", 0, 0     } },
    { da_otpprogram_main,    3, {"da_otpprogram_main",   "-help",
                                                         "-exit", 0, 0     } },
    { da_securedebug_main,   3, {"da_securedebug_main",  "-help",
                                                         "-exit", 0        } },
    { da_val_specials_main,  5, {"da_val_specials_main", "-help",
                                                         "-hwversion",
                                                         "-hwoptions",
                                                         "-hwmodulestatus" } },
    { da_val_main,           2, {"da_val_main",          "-help", 0, 0, 0  } },
    { da_val_main,           2, {"da_val_main",          "-initotp",
                                                         0, 0, 0           } },
    { test_val_main,         1, {"test_val_main",        0, 0, 0, 0        } }
#else
    { da_val_main,           2, {"da_val_main",          0, 0, 0, 0        } },
#endif
};


/*----------------------------------------------------------------------------
 * TestTask
 *
 * Test task function.
 */
static void
TestTask(
        void * arg_p)
{
    unsigned int i, Count = sizeof(TestFuncs) / sizeof(TestFuncs[0]);

    for (i = 0; i < Count; i++)
        if (TestFuncs[i].func)
            TestFuncs[i].func(TestFuncs[i].argc, TestFuncs[i].argv);

    while(1);
}


/*----------------------------------------------------------------------------
 * main
 */
int
main(
        int argc,
        char **argv)
{
    TaskHandle_t TestTaskHandle;

    xTaskCreate(TestTask,
                (const char *)"Regression task",
                0x40000 / sizeof(uint32_t),
                NULL,
                tskIDLE_PRIORITY,
                &TestTaskHandle);

    // Start the tasks and timer running
    vTaskStartScheduler();

    return 0;
}


/* end of file regress.c */

