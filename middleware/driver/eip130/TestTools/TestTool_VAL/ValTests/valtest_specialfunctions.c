/* valtest_specialfunctions.c
 *
 * Description: Special Functions functionality related tests
 */

/*****************************************************************************
* Copyright (c) 2017-2018 INSIDE Secure B.V. All Rights Reserved.
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

/* Test vectors. */
#include "testvectors_milenage.h"

static int
do_conformance_test(
        TestVector_Milenage_t Vector_p)
{
    ValStatus_t Status;
    uint8_t RES[8];
    uint8_t CK[16];
    uint8_t IK[16];
    uint8_t MACA[8];
    uint8_t MACS[8];
    uint8_t AK[6];
    uint8_t AKstar[6];
    uint8_t OPc[16];

    Status = val_SFMilenageConformance(Vector_p->RAND_p,
                                       Vector_p->SQN_p, Vector_p->AMF_p,
                                       Vector_p->K_p, Vector_p->OP_p,
                                       RES, CK, IK,
                                       MACA, MACS, AK, AKstar, OPc);

    unsupported_unless(Status != VAL_INVALID_TOKEN, "Special Functions not activated");
    unsupported_unless(Status != VAL_INVALID_PARAMETER, "Milenage (conformance) not activated");

    fail_if(Status != VAL_SUCCESS, "val_SFMilenageConformance()=", Status);
    fail_if(memcmp(MACA, Vector_p->f1_p, sizeof(MACA)), "MACA != f1", -1);
    fail_if(memcmp(MACS, Vector_p->f1star_p, sizeof(MACS)), "MACS != f1star", -1);
    fail_if(memcmp(RES, Vector_p->f2_p, sizeof(RES)), "RES != f2", -1);
    fail_if(memcmp(CK, Vector_p->f3_p, sizeof(CK)), "CK != f3", -1);
    fail_if(memcmp(IK, Vector_p->f4_p, sizeof(IK)), "IK != f4", -1);
    fail_if(memcmp(AK, Vector_p->f5_p, sizeof(AK)), "AK != f5", -1);
    fail_if(memcmp(AKstar, Vector_p->f5star_p, sizeof(AKstar)), "AKstar != f5star", -1);
    fail_if(memcmp(OPc, Vector_p->OPc_p, sizeof(OPc)), "OPc != OPc", -1);

    return END_TEST_SUCCES;
}

START_TEST(test_SF_MilenageConformance)
{
    int ndx;
    int rc;
    int failed;

    for (ndx = 0, failed = 0; ; ndx++)
    {
        TestVector_Milenage_t tv_p = test_vectors_milenage_get(ndx);
        if (tv_p == NULL)
        {
            break;
        }

        rc = do_conformance_test(tv_p);
        if (rc == END_TEST_UNSUPPORTED)
        {
            return rc;
        }
        if (rc != END_TEST_SUCCES)
        {
            LOG_CRIT("Process vector %d\n", ndx);
            failed++;
        }
        test_vectors_milenage_release(tv_p);
    }

    fail_if(failed, "#wrong tests", failed);
}
END_TEST


int
suite_add_test_SpecialFunctions(
        struct TestSuite * TestSuite_p)
{
    struct TestCase * TestCase_p;

    TestCase_p = sfzutf_tcase_create(TestSuite_p, "SpecialFunctions_Tests");
    if (TestCase_p != NULL)
    {
        if (sfzutf_tcase_add_fixture(TestCase_p, valtest_initialize, valtest_terminate) != 0)
        {
             goto FuncErrorReturn;
        }

        if (sfzutf_test_add(TestCase_p, test_SF_MilenageConformance) != 0) goto FuncErrorReturn;

        return 0;
    }

FuncErrorReturn:
    return -1;
}

/* end of file valtest_specialfunctions.c */
