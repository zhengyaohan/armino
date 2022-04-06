/* valtest_service.c
 *
 * Description: service functionality related tests
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


START_TEST(test_service_ReadWriteRegister)
{
    ValStatus_t Status;
    uint32_t RegAddress = 0xF878;
    uint32_t RegValue = 0;

    Status = val_ServiceRegisterRead(RegAddress, &RegValue);
    if (Status == VAL_INVALID_ADDRESS)
    {
        RegAddress = 0xF8E4;
        Status = val_ServiceRegisterRead(RegAddress, &RegValue);
    }
    fail_if(Status != VAL_SUCCESS, "val_ServiceRegisterRead()=", Status);

    if (val_IsAccessSecure() && valtest_IsCOIDAvailable())
    {
        Status = val_ServiceRegisterWrite(RegAddress, RegValue);
        fail_if(Status != VAL_SUCCESS, "val_ServiceRegisterWrite()=", Status);
    }
    else
    {
        Status = val_ServiceRegisterWrite(RegAddress, RegValue);
        fail_if(Status != VAL_ACCESS_ERROR, "val_ServiceRegisterWrite()=", Status);
    }
}
END_TEST


START_TEST(test_service_ZeroizeMailbox)
{
    ValStatus_t Status;

    Status = val_ServiceZeroizeMailbox();
    fail_if(Status != VAL_SUCCESS, "val_ServiceZeroizeMailbox()=", Status);
}
END_TEST

START_TEST(test_service_ClockSwitch)
{
    ValOctetsInOut_t DigestBuffer[VAL_SYM_ALGO_MAX_DIGEST_SIZE];
    ValSize_t DigestBufferSize = VAL_SYM_ALGO_MAX_DIGEST_SIZE;
    ValStatus_t Status;
    ValSymContextPtr_t SymContext_p = NULL;
    uint8_t * Msg_p = NULL;

    unsupported_unless(valtest_IsCOIDAvailable(), "COID not available");

    // Force hash clock OFF
    Status = val_ServiceClockSwitch(0, 0x02);
    if (valtest_IsCOIDAvailable())
    {
        fail_if(Status != VAL_SUCCESS, "val_ServiceClockSwitch()=", Status);

        Status = val_SymAlloc(VAL_SYM_ALGO_HASH_SHA256, VAL_SYM_MODE_NONE, false, &SymContext_p);
        fail_if(Status != VAL_SUCCESS, "val_SymAlloc()=", Status);

        Msg_p = (uint8_t *)SFZUTF_MALLOC(4);
        fail_if(Msg_p == NULL, "Allocation ", 4);
        memcpy(Msg_p, "Test", 4);

        Status = val_SymHashFinal(SymContext_p,
                                  Msg_p, 4,
                                  (ValOctetsInOut_t * const)&DigestBuffer, &DigestBufferSize);
        fail_if(Status != VAL_CLOCK_ERROR, "val_SymHashFinal()=", Status);

        // Set all clocks in automatic mode
        Status = val_ServiceClockSwitch(0, 0);
        fail_if(Status != VAL_SUCCESS, "val_ServiceClockSwitch()=", Status);

        Status = val_SymHashFinal(SymContext_p,
                                  Msg_p, 4,
                                  (ValOctetsInOut_t * const)&DigestBuffer, &DigestBufferSize);
        fail_if(Status != VAL_SUCCESS, "val_SymHashFinal()=", Status);

        SFZUTF_FREE(Msg_p);
    }
    else
    {
        fail_if(Status != VAL_ACCESS_ERROR, "val_ServiceClockSwitch()=", Status);
    }
}
END_TEST


int
suite_add_test_Service(
        struct TestSuite * TestSuite_p)
{
    struct TestCase * TestCase_p;

    TestCase_p = sfzutf_tcase_create(TestSuite_p, "Service_Tests");
    if (TestCase_p != NULL)
    {
        if (sfzutf_tcase_add_fixture(TestCase_p, valtest_initialize, valtest_terminate) != 0)
        {
             goto FuncErrorReturn;
        }

        if (sfzutf_test_add(TestCase_p, test_service_ReadWriteRegister) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add(TestCase_p, test_service_ZeroizeMailbox) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add(TestCase_p, test_service_ClockSwitch) != 0) goto FuncErrorReturn;

        return 0;
    }

FuncErrorReturn:
    return -1;
}

/* end of file valtest_service.c */
