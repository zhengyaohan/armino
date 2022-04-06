/* testvectors_rsa.c
 *
 * Description: Test vectors for RSA algorithm.
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

#include "c_test_val.h"                 // configuration

#include "basic_defs.h"

#include "testvectors_rsa.h"
#include "testvectors_rsa_data.h"


/* The function API for accessing the vectors. */
int
test_vectors_rsa_pkcs1v15_num(void)
{
    return (sizeof(test_vectors_rsa_pkcs1v15) /
            sizeof(test_vectors_rsa_pkcs1v15[0]));
}

TestVector_RSA_PKCS1v15_t
test_vectors_rsa_pkcs1v15_get(int Index)
{
    if (Index >= test_vectors_rsa_pkcs1v15_num())
    {
        return NULL;
    }
    return &test_vectors_rsa_pkcs1v15[Index];
}

void
test_vectors_rsa_pkcs1v15_release(TestVector_RSA_PKCS1v15_t Vector_p)
{
    /* Test vectors are statically defined => nothing to do. */
    IDENTIFIER_NOT_USED(Vector_p);
}


int
test_vectors_rsa_pss_num(void)
{
    return (sizeof(test_vectors_rsa_pss) / sizeof(test_vectors_rsa_pss[0]));
}

TestVector_RSA_PSS_t
test_vectors_rsa_pss_get(int Index)
{
    if (Index >= test_vectors_rsa_pss_num())
    {
        return NULL;
    }
    return &test_vectors_rsa_pss[Index];
}

void
test_vectors_rsa_pss_release(TestVector_RSA_PSS_t Vector_p)
{
    /* Test vectors are statically defined => nothing to do. */
    IDENTIFIER_NOT_USED(Vector_p);
}


int
test_vectors_rsa_oaep_num(void)
{
    return (sizeof(test_vectors_rsa_oaep) / sizeof(test_vectors_rsa_oaep[0]));
}

TestVector_RSA_OAEP_t
test_vectors_rsa_oaep_get(int Index)
{
    if (Index >= test_vectors_rsa_oaep_num())
    {
        return NULL;
    }
    return &test_vectors_rsa_oaep[Index];
}

void
test_vectors_rsa_oaep_release(TestVector_RSA_OAEP_t Vector_p)
{
    /* Test vectors are statically defined => nothing to do. */
    IDENTIFIER_NOT_USED(Vector_p);
}


int
test_vectors_rsa_pkcs1v15wrap_num(void)
{
    return (sizeof(test_vectors_rsa_pkcs1v15wrap) /
            sizeof(test_vectors_rsa_pkcs1v15wrap[0]));
}

TestVector_PKCS1V15WRAP_t
test_vectors_rsa_pkcs1v15wrap_get(int Index)
{
    if (Index >= test_vectors_rsa_pkcs1v15wrap_num())
    {
        return NULL;
    }
    return &test_vectors_rsa_pkcs1v15wrap[Index];
}

void
test_vectors_rsa_pkcs1v15wrap_release(TestVector_PKCS1V15WRAP_t Vector_p)
{
    /* Test vectors are statically defined => nothing to do. */
    IDENTIFIER_NOT_USED(Vector_p);
}

/* end of file testvectors_rsa.c */
