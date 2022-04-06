/* valtest_system.c
 *
 * Description: System related tests
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

#include "valtest_internal.h"


/* Maximum expected version string length.
   If this length is exceeded, the test shall fail.
   You may increase the size should this happen. */
START_TEST(test_system_version)
{
    ValStatus_t Status;

    /* Added +1 to make sure there is zero in the buffer even if
       sfzcrypto_read_version is broken and forgets insert it. */
    char versionString[VAL_TEST_MAX_VERSION_LENGTH + 1];
    ValSize_t versionStringLen;
    size_t StringLen;

    versionStringLen = 0;
    Status = val_SystemGetVersion(NULL, &versionStringLen);
    fail_if(Status != VAL_BUFFER_TOO_SMALL,
            "val_SystemGetVersion(length query)=", Status);
    fail_if((versionStringLen < 5) ||
            (versionStringLen > VAL_TEST_MAX_VERSION_LENGTH),
            "val_SystemGetVersion() returned unexpected length ",
            (int)versionStringLen);

    LOG_INFO("Firmware version string length is: %d\n", (int)versionStringLen);

    memset(versionString, 0, sizeof(versionString));
    versionStringLen = VAL_TEST_MAX_VERSION_LENGTH;
    Status = val_SystemGetVersion((ValOctetsOut_t *)versionString,
                                  &versionStringLen);
    fail_if(Status != VAL_SUCCESS, "val_SystemGetVersion()=", Status);
    fail_if((versionStringLen <= 1) ||
            (versionStringLen > VAL_TEST_MAX_VERSION_LENGTH),
            "val_SystemGetVersion() returned unexpected length ",
            (int)versionStringLen);
    StringLen = (strlen(versionString) + 1);
    fail_if(StringLen != versionStringLen,
            "val_SystemGetVersion(): Not a string or wrong length reported",
            (int)StringLen);

    LOG_INFO("Firmware version detected: %s\n", versionString);

    // VAL_BAD_ARGUMENT tests
    Status = val_SystemGetVersion(NULL, NULL);
    fail_if(Status != VAL_BAD_ARGUMENT,
            "val_SystemGetVersion(NULL, NULL)=", Status);

    versionStringLen = VAL_TEST_MAX_VERSION_LENGTH;
    Status = val_SystemGetVersion(NULL, &versionStringLen);
    fail_if(Status != VAL_BAD_ARGUMENT,
            "val_SystemGetVersion(NULL, &Length(!=0))=", Status);
}
END_TEST

START_TEST(test_system_state)
{
    ValStatus_t Status;
    uint8_t OtpErrorCode = 0;
    uint16_t OtpErrorLocation = 0;
    uint8_t Mode = 0;
    uint8_t ErrorTest = 0;
    uint8_t CryptoOfficer = 0;
    uint8_t HostID = 0;
    uint8_t NonSecure = 0;
    uint32_t Identity = 0;

    Status = val_SystemGetState(&OtpErrorCode, &OtpErrorLocation,
                                &Mode, &ErrorTest, &CryptoOfficer,
                                &HostID, &NonSecure, &Identity);
    fail_if(Status != VAL_SUCCESS, "val_SystemGetState(INFO)=", Status);

    LOG_INFO("OTP State     : %u (%u)\n", OtpErrorCode, OtpErrorLocation);
    LOG_INFO("Mode          : %u (0x%02X)\n", Mode, ErrorTest);
    LOG_INFO("CryptoOfficer : %sAvailable\n", CryptoOfficer ? "" : "NOT ");
    LOG_INFO("HostID        : %u\n", HostID);
    LOG_INFO("Secure        : %s\n", NonSecure ? "No" : "Yes ");
    LOG_INFO("Identity      : 0x%X\n", Identity);

    Status = val_SystemGetState(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    fail_if(Status != VAL_SUCCESS, "val_SystemGetState(NULL)=", Status);
}
END_TEST

START_TEST(test_system_reset)
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

#ifdef API_VAL_ASSET_LIFETIME_MANAGEMENT
START_TEST(test_system_set_time)
{
    uint8_t * AssociatedData_p;
    ValSize_t AssociatedDataSize = strlen(g_ValTestAssociatedData_p);
    ValAssetId_t RootAssetId = VAL_ASSETID_INVALID;
    ValAssetId_t KEKAssetId1 = VAL_ASSETID_INVALID;
    ValAssetId_t KEKAssetId2 = VAL_ASSETID_INVALID;
    ValPolicyMask_t AssetPolicy;
    ValStatus_t Status;

    uint32_t Time1 = 0x5A210C00;    /* December 1st 2017 08:00 */
    uint32_t Time2 = 0x5A211A10;    /* December 1st 2017 09:00 */
    uint32_t Time3 = 0x58B50000;    /* Time before March 1st 2017 - not valid*/

    //  Get root key
    RootAssetId = val_AssetGetRootKey();
    unsupported_if((RootAssetId == VAL_ASSETID_INVALID), "No Root key");

    AssociatedData_p = (uint8_t *)SFZUTF_MALLOC(AssociatedDataSize);
    fail_if(AssociatedData_p == NULL, "Allocation ", (int)AssociatedDataSize);
    memcpy(AssociatedData_p, g_ValTestAssociatedData_p, AssociatedDataSize);

    // Run tests
    if (RootAssetId != VAL_ASSETID_INVALID)
    {
        // Create and derive KEK Asset
        AssetPolicy = VAL_POLICY_TRUSTED_WRAP | VAL_POLICY_ENCRYPT | VAL_POLICY_DECRYPT;
        if (!val_IsAccessSecure())
        {
            AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
        }

        // Set System Time to initial time (08:00)
        Status = val_SystemSetTime(Time1);
        fail_if(Status != VAL_SUCCESS, "val_SystemSetTime(Time1)=", Status);

        // Allocate 2 KEK assets with lifetimes 10s less than second time (08:59:50)
        Status = val_AssetAlloc(AssetPolicy, 64,
                                false, false,
                                VAL_ASSET_LIFETIME_SECONDS, false, false,
                                Time2 - 10,
                                &KEKAssetId1);
        fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(KEK1)=", Status);
        Status = val_AssetAlloc(AssetPolicy, 64,
                                false, false,
                                VAL_ASSET_LIFETIME_SECONDS, false, false,
                                Time2 - 10,
                                &KEKAssetId2);
        fail_if(Status != VAL_SUCCESS, "val_AssetAlloc(KEK2)=", Status);

        // Use AssetLoadDerive, result should be OK as the asset is not expired
        Status = val_AssetLoadDerive(KEKAssetId1, RootAssetId,
                                     AssociatedData_p, AssociatedDataSize,
                                     false, false, NULL, 0, NULL, 0);
        fail_if(Status != VAL_SUCCESS, "val_AssetLoadDerive(KEK1)=", Status);

        // Set System Time to the second time (09:00)
        Status = val_SystemSetTime(Time2);
        fail_if(Status != VAL_SUCCESS, "val_SystemSetTime(Time2)=", Status);

        // Use AssetLoadDerive again, Asset Expired expected as the returned error code
        Status = val_AssetLoadDerive(KEKAssetId2, RootAssetId,
                                     AssociatedData_p, AssociatedDataSize,
                                     false, false, NULL, 0, NULL, 0);
        fail_if(Status != VAL_ASSET_EXPIRED, "val_AssetLoadDerive(KEK2)=", Status);

        // Try to set system time to time before March 1st 2017
        Status = val_SystemSetTime(Time3);
        fail_if((Status != VAL_INVALID_PARAMETER) && (Status != VAL_BAD_ARGUMENT),
                "val_SystemSetTime(Time3)=", Status);

        // Free allocated KEK assets
        Status = val_AssetFree(KEKAssetId1);
        fail_if(Status != VAL_SUCCESS, "val_AssetFree()=", Status);
        Status = val_AssetFree(KEKAssetId2);
        fail_if(Status != VAL_SUCCESS, "val_AssetFree()=", Status);
    }
    SFZUTF_FREE(AssociatedData_p);
}
END_TEST
#endif

int
suite_add_test_System(
        struct TestSuite * TestSuite_p)
{
    struct TestCase * TestCase_p;

    TestCase_p = sfzutf_tcase_create(TestSuite_p, "System_Tests");
    if (TestCase_p != NULL)
    {
        if (sfzutf_tcase_add_fixture(TestCase_p, valtest_initialize, valtest_terminate) != 0)
        {
             goto FuncErrorReturn;
        }

        if (sfzutf_test_add(TestCase_p, test_system_version) != 0) goto FuncErrorReturn;
        if (sfzutf_test_add(TestCase_p, test_system_state) != 0) goto FuncErrorReturn;
        if (g_ResetAllowed)
        {
            if (sfzutf_test_add(TestCase_p, test_system_reset) != 0) goto FuncErrorReturn;
        }
#ifdef API_VAL_ASSET_LIFETIME_MANAGEMENT
        if (sfzutf_test_add(TestCase_p, test_system_set_time) != 0) goto FuncErrorReturn;
#endif

        return 0;
    }

FuncErrorReturn:
    return -1;
}

/* end of file valtest_system.c */
