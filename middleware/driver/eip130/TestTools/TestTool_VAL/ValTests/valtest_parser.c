/* valtest_parser.c
 *
 * Description: Calls other test functions
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


void
valtest_initialize(void)
{
}

void
valtest_terminate(void)
{
}

int
build_suite(void)
{
    struct TestSuite * TestSuite_p = sfzutf_tsuite_create("VAL_Tests");
    if (TestSuite_p != NULL)
    {
#if 0
		if (suite_add_test_SymAuthCrypto(TestSuite_p) != 0) goto FuncErrorReturn;
#else		
        if (suite_add_test_System(TestSuite_p) != 0) goto FuncErrorReturn;
        if (suite_add_test_Nop(TestSuite_p) != 0) goto FuncErrorReturn;
        if (suite_add_test_Random(TestSuite_p) != 0) goto FuncErrorReturn;
        if (suite_add_test_AssetManagement(TestSuite_p) != 0) goto FuncErrorReturn;
        if (suite_add_test_SymHashHMac(TestSuite_p) != 0) goto FuncErrorReturn;
        if (suite_add_test_SymCrypto(TestSuite_p) != 0) goto FuncErrorReturn;
        if (suite_add_test_SymAuthCrypto(TestSuite_p) != 0) goto FuncErrorReturn;
        if (suite_add_test_SymCipherMac(TestSuite_p) != 0) goto FuncErrorReturn;
        if (suite_add_test_SymKeyWrap(TestSuite_p) != 0) goto FuncErrorReturn;
        if (suite_add_test_AsymEcdh(TestSuite_p) != 0) goto FuncErrorReturn;
        if (suite_add_test_AsymEcdsa(TestSuite_p) != 0) goto FuncErrorReturn;
        if (suite_add_test_AsymEccElGamal(TestSuite_p) != 0) goto FuncErrorReturn;
        if (suite_add_test_AsymCurve25519(TestSuite_p) != 0) goto FuncErrorReturn;
        if (suite_add_test_AsymEddsa(TestSuite_p) != 0) goto FuncErrorReturn;
        if (suite_add_test_AsymDh(TestSuite_p) != 0) goto FuncErrorReturn;
        if (suite_add_test_AsymRsa(TestSuite_p) != 0) goto FuncErrorReturn;
        if (suite_add_test_AsymPk(TestSuite_p) != 0) goto FuncErrorReturn;
        if (suite_add_test_SecureTimer(TestSuite_p) != 0) goto FuncErrorReturn;
        if (suite_add_test_Service(TestSuite_p) != 0) goto FuncErrorReturn;
        if (suite_add_test_SpecialFunctions(TestSuite_p) != 0) goto FuncErrorReturn;
        if (suite_add_test_FIPS(TestSuite_p) != 0) goto FuncErrorReturn;
#endif
        return 0;
    }

FuncErrorReturn:
    return -1;
}

/* end of file valtest_parser.c */
