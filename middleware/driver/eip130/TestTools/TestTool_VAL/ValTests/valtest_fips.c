/* valtest_fips.c
 *
 * Description: FIPS related tests
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

#include "valtest_internal.h"

static int
do_fips_system_define_users(void)
{
    ValStatus_t Status;
    uint8_t OtpErrorCode;
    uint16_t OtpErrorLocation;
    uint8_t Mode;
    uint8_t ErrorTest;
    uint8_t CryptoOfficer;
    uint8_t HostID;
    uint8_t NonSecure;
    uint32_t Identity;

    Status = val_SystemGetState(&OtpErrorCode, &OtpErrorLocation,
                                &Mode, &ErrorTest, &CryptoOfficer,
                                &HostID, &NonSecure, &Identity);
    fail_if(Status != VAL_SUCCESS, "val_SystemGetState(INFO)=", Status);

    Status = val_SystemDefineUsers(Identity, 0x609, 0x86090001, 0xA);
    fail_if(Status != VAL_SUCCESS, "val_SystemDefineUsers()=", Status);

    Status = val_SystemGetState(&OtpErrorCode, &OtpErrorLocation,
                                &Mode, &ErrorTest, &CryptoOfficer,
                                &HostID, &NonSecure, &Identity);
    fail_if(Status != VAL_SUCCESS, "val_SystemGetState(INFO)=", Status);

    return END_TEST_SUCCES;
}

START_TEST(test_fips_system_define_users1)
{
    unsupported_unless(valtest_IsCOIDAvailable(), "COID not available");

    if (do_fips_system_define_users() != END_TEST_SUCCES)
    {
        return END_TEST_FAIL;
    }
}
END_TEST

START_TEST(test_fips_system_define_users2)
{
    unsupported_unless(valtest_IsCOIDAvailable(), "COID not available");

    if (do_fips_system_define_users() != END_TEST_SUCCES)
    {
        return END_TEST_FAIL;
    }
}
END_TEST

START_TEST(test_fips_system_selftest)
{
    ValStatus_t Status;
    uint8_t Mode;

    unsupported_unless(valtest_IsCOIDAvailable(), "COID not available");

    Status = val_SystemGetState(NULL, NULL, &Mode, NULL, NULL, NULL, NULL, NULL);
    fail_if(Status != VAL_SUCCESS, "val_SystemGetState(INFO)=", Status);
    fail_if(Mode != 0, "Not non-FIPS - Mode=", Mode);

    Status = val_SystemSelfTest();
    fail_if(Status != VAL_SUCCESS, "val_SystemSelfTest()=", Status);

    Status = val_SystemGetState(NULL, NULL, &Mode, NULL, NULL, NULL, NULL, NULL);
    fail_if(Status != VAL_SUCCESS, "val_SystemGetState(INFO)=", Status);
    fail_if(Mode != 15, "Not FIPS - Mode=", Mode);
}
END_TEST

START_TEST(test_fips_system_reset)
{
    char versionString[VAL_TEST_MAX_VERSION_LENGTH + 1];
    ValSize_t versionStringLen;
    ValStatus_t Status;

    Status = val_SystemReset();
    fail_if(Status != VAL_SUCCESS, "val_SystemReset()=", Status);

    versionStringLen = VAL_TEST_MAX_VERSION_LENGTH;
    Status = val_SystemGetVersion((ValOctetsOut_t *)versionString,
                                  &versionStringLen);
    fail_if(Status != VAL_SUCCESS, "val_SystemGetVersion()=", Status);
}
END_TEST

int
suite_add_test_FIPS(
        struct TestSuite * TestSuite_p)
{
    struct TestCase * TestCase_p;

    TestCase_p = sfzutf_tcase_create(TestSuite_p, "FIPS_Tests");
    if (TestCase_p != NULL)
    {
        if (sfzutf_tcase_add_fixture(TestCase_p, valtest_initialize, valtest_terminate) != 0)
        {
             goto FuncErrorReturn;
        }

        if (sfzutf_test_add(TestCase_p, test_fips_system_define_users1) != 0) goto FuncErrorReturn;
        if (g_ResetAllowed)
        {
            if (sfzutf_test_add(TestCase_p, test_fips_system_selftest) != 0) goto FuncErrorReturn;
            if (sfzutf_test_add(TestCase_p, test_fips_system_define_users2) != 0) goto FuncErrorReturn;

            // OPTIONAL: Added some FIPS specific tests, but does not add coverage.

            if (sfzutf_test_add(TestCase_p, test_fips_system_reset) != 0) goto FuncErrorReturn;
        }

        return 0;
    }

FuncErrorReturn:
    return -1;
}

/* end of file valtest_fips.c */
