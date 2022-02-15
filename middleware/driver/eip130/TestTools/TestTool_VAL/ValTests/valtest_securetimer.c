/* valtest_securetimer.c
 *
 * Description: Secure timer related tests
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


START_TEST(test_secure_timer)
{
    ValStatus_t Status;
    ValAssetId_t TimerAssetId_100us = VAL_ASSETID_INVALID;
    ValAssetId_t TimerAssetId_sec = VAL_ASSETID_INVALID;
    uint32_t ElapsedTime;

    Status = val_SecureTimerStart(false, &TimerAssetId_100us);
    fail_if(Status != VAL_SUCCESS, "val_SecureTimerStart()=", Status);

    Status = val_SecureTimerStart(true, &TimerAssetId_sec);
    fail_if(Status != VAL_SUCCESS, "val_SecureTimerStart()=", Status);

    Status = val_SecureTimerRead(TimerAssetId_sec, &ElapsedTime);
    fail_if(Status != VAL_SUCCESS, "val_SecureTimerRead()=", Status);
    // The expected ElapsedTime is zero. However, it can also be that the
    // execution moment of the timer start and read lay in the next consecutive
    // seconds. In that case, the ElapsedTime will be at the most one.
    fail_if(ElapsedTime > 1, "Expected to be zero or at the most one", ElapsedTime);

    Status = val_SecureTimerRead(TimerAssetId_100us, &ElapsedTime);
    fail_if(Status != VAL_SUCCESS, "val_SecureTimerRead()=", Status);
    fail_if(ElapsedTime == 0, "Expected to be not zero ", ElapsedTime);

    Status = val_SecureTimerRelease(TimerAssetId_100us);
    fail_if(Status != VAL_SUCCESS, "val_SecureTimerRelease()=", Status);

    while (1)
    {
        Status = val_SecureTimerRead(TimerAssetId_sec, &ElapsedTime);
        fail_if(Status != VAL_SUCCESS, "val_SecureTimerRead()=", Status);
        if (ElapsedTime != 0)
        {
            break;
        }
        SFZUTF_USLEEP(1000);
    }

    Status = val_SecureTimerStop(TimerAssetId_sec, &ElapsedTime);
    fail_if(Status != VAL_SUCCESS, "val_SecureTimerStop()=", Status);
    fail_if(ElapsedTime != 1, "Expected to be one ", ElapsedTime);
}
END_TEST


START_TEST(test_secure_timer_restart)
{
    ValStatus_t Status;
    ValAssetId_t TimerAssetId_100us1 = VAL_ASSETID_INVALID;
    ValAssetId_t TimerAssetId_100us2;
    uint32_t ElapsedTime1;
    uint32_t ElapsedTime2;

    Status = val_SecureTimerStart(false, &TimerAssetId_100us1);
    fail_if(Status != VAL_SUCCESS, "val_SecureTimerStart()=", Status);

    SFZUTF_USLEEP(500);

    Status = val_SecureTimerRead(TimerAssetId_100us1, &ElapsedTime1);
    fail_if(Status != VAL_SUCCESS, "val_SecureTimerRead()=", Status);

    TimerAssetId_100us2 = TimerAssetId_100us1;
    Status = val_SecureTimerStart(false, &TimerAssetId_100us2);
    fail_if(Status != VAL_SUCCESS, "val_SecureTimerStart()=", Status);
    fail_if(TimerAssetId_100us2 != TimerAssetId_100us1, "Not Asset change expected", Status);

    SFZUTF_USLEEP(300);

    Status = val_SecureTimerRead(TimerAssetId_100us1, &ElapsedTime2);
    fail_if(Status != VAL_SUCCESS, "val_SecureTimerRead()=", Status);
    fail_if(ElapsedTime1 < ElapsedTime2, "ElapsedTime1 should be bigger than ElapsedTime2 ", (ElapsedTime1-ElapsedTime2));

    Status = val_SecureTimerRelease(TimerAssetId_100us1);
    fail_if(Status != VAL_SUCCESS, "val_SecureTimerRelease()=", Status);
}
END_TEST


int
suite_add_test_SecureTimer(
        struct TestSuite * TestSuite_p)
{
    struct TestCase * TestCase_p;

    TestCase_p = sfzutf_tcase_create(TestSuite_p, "SecureTimer_Tests");
    if (TestCase_p != NULL)
    {
        if (sfzutf_tcase_add_fixture(TestCase_p, valtest_initialize, valtest_terminate) != 0)
        {
             goto FuncErrorReturn;
        }

        if (sfzutf_test_add(TestCase_p, test_secure_timer) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add(TestCase_p, test_secure_timer_restart) != 0) goto FuncErrorReturn;

        return 0;
    }

FuncErrorReturn:
    return -1;
}

/* end of file valtest_securetimer.c */
