/* testvectors_rsa.h
 *
 * Description: Test vectors for RSA.
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

#ifndef INCLUDE_GUARD_TESTVECTORS_RSA_H
#define INCLUDE_GUARD_TESTVECTORS_RSA_H

#include "basic_defs.h"

/* Structures for Test Vectors. */
typedef struct
{
    const uint8_t * VersionId_p;
    uint32_t VersionIdBytes;
    const uint8_t * Modulus_p;
    uint32_t ModulusBytes;
    const uint8_t * PublicExponent_p;
    uint32_t PublicExponentBytes;
    const uint8_t * PrivateExponent_p;
    uint32_t PrivateExponentBytes;
    const uint8_t * Prime1_p;
    uint32_t Prime1Bytes;
    const uint8_t * Prime2_p;
    uint32_t Prime2Bytes;
    const uint8_t * Exponent1_p;
    uint32_t Exponent1Bytes;
    const uint8_t * Exponent2_p;
    uint32_t Exponent2Bytes;
    const uint8_t * Coefficient_p;
    uint32_t CoefficientBytes;
} TestVector_RSA_PKCS1v15_KeyPair_Rec_t;

typedef struct
{
    const char * Name_p;
    uint32_t ModulusBits;
    uint32_t HashBits;
    TestVector_RSA_PKCS1v15_KeyPair_Rec_t Key;
    const uint8_t * Msg_p;
    uint32_t MsgBytes;
    const uint8_t * Signature_p;
    uint32_t SignatureBytes;
} TestVector_RSA_PKCS1v15_Rec_t;

typedef const TestVector_RSA_PKCS1v15_Rec_t * TestVector_RSA_PKCS1v15_t;

typedef struct
{
    const uint8_t * Modulus_p;
    uint32_t ModulusBytes;
    const uint8_t * PublicExponent_p;
    uint32_t PublicExponentBytes;
    const uint8_t * PrivateExponent_p;
    uint32_t PrivateExponentBytes;
} TestVector_RSA_PSS_KeyPair_Rec_t;

typedef struct
{
    const char * Name_p;
    uint32_t ModulusBits;
    uint32_t HashBits;
    TestVector_RSA_PSS_KeyPair_Rec_t Key;
    const uint8_t * Msg_p;
    uint32_t MsgBytes;
    const uint8_t * Signature_p;
    uint32_t SignatureBytes;
} TestVector_RSA_PSS_Rec_t;

typedef const TestVector_RSA_PSS_Rec_t * TestVector_RSA_PSS_t;


typedef struct
{
    const uint8_t * Modulus_p;
    uint32_t ModulusBytes;
    const uint8_t * PublicExponent_p;
    uint32_t PublicExponentBytes;
    const uint8_t * PrivateExponent_p;
    uint32_t PrivateExponentBytes;
} TestVector_RSA_OAEP_KeyPair_Rec_t;

typedef struct
{
    const char * Name_p;
    uint32_t ModulusBits;
    uint32_t HashBits;
    TestVector_RSA_OAEP_KeyPair_Rec_t Key;
    const uint8_t * AdditionalInput_p;
    uint32_t AdditionalInputBytes;
    const uint8_t * AdditionalInputHash_p;
    uint32_t AdditionalInputHashBytes;
    const uint8_t * PlainData_p;
    uint32_t PlainDataBytes;
    const uint8_t * WrappedData_p;
    uint32_t WrappedDataBytes;
} TestVector_RSA_OAEP_Rec_t;

typedef const TestVector_RSA_OAEP_Rec_t * TestVector_RSA_OAEP_t;


typedef struct
{
    const uint8_t * Modulus_p;
    uint32_t ModulusBytes;
    const uint8_t * PublicExponent_p;
    uint32_t PublicExponentBytes;
    const uint8_t * PrivateExponent_p;
    uint32_t PrivateExponentBytes;
} TestVector_RSA_PKCS1V15WRAP_KeyPair_Rec_t;

typedef struct
{
    const char * Name_p;
    uint32_t ModulusBits;
    uint32_t HashBits;
    TestVector_RSA_PKCS1V15WRAP_KeyPair_Rec_t Key;
    const uint8_t * PlainData_p;
    uint32_t PlainDataBytes;
    const uint8_t * WrappedData_p;
    uint32_t WrappedDataBytes;
} TestVector_RSA_PKCS1V15WRAP_Rec_t;

typedef const TestVector_RSA_PKCS1V15WRAP_Rec_t * TestVector_PKCS1V15WRAP_t;

/* API for using RSA based test vectors. */

/* Request number of RSA PKCS#1 v1.5 test vectors available. */
int
test_vectors_rsa_pkcs1v15_num(void);

/* Request test vector by index.
   If Index >= test_vectors_rsa_pkcs1v15_num(), the function returns NULL.

   Note: The function returns zero only when invalid vector has been requested.
   Note: The received vector is constant and must not be altered by caller.
   The test vector is described by structure TestVector_RSA_PKCS1v15_t and
   the structure shall be accessed directly by the user of the test vector.
*/
TestVector_RSA_PKCS1v15_t
test_vectors_rsa_pkcs1v15_get(
        int Index);

/* Release a vector.
   It is mandatory to release a vector once user of test vector is finished
   with the vector. */
void
test_vectors_rsa_pkcs1v15_release(
        TestVector_RSA_PKCS1v15_t Vector_p);


/* Request number of RSA PSS test vectors available. */
int
test_vectors_rsa_pss_num(void);

/* Request test vector by index.
   If Index >= test_vectors_rsa_pss_num(), the function returns NULL.

   Note: The function returns zero only when invalid vector has been requested.
   Note: The received vector is constant and must not be altered by caller.
   The test vector is described by structure TestVector_RSA_PSS_t and
   the structure shall be accessed directly by the user of the test vector.
*/
TestVector_RSA_PSS_t
test_vectors_rsa_pss_get(
        int Index);

/* Release a vector.
   It is mandatory to release a vector once user of test vector is finished
   with the vector. */
void
test_vectors_rsa_pss_release(
        TestVector_RSA_PSS_t Vector_p);


/* Request number of RSA OAEP test vectors available. */
int
test_vectors_rsa_oaep_num(void);

/* Request test vector by index.
   If Index >= test_vectors_rsa_oaep_num(), the function returns NULL.

   Note: The function returns zero only when invalid vector has been requested.
   Note: The received vector is constant and must not be altered by caller.
   The test vector is described by structure TestVector_RSA_OAEP_t and
   the structure shall be accessed directly by the user of the test vector.
*/
TestVector_RSA_OAEP_t
test_vectors_rsa_oaep_get(
        int Index);

/* Release a vector.
   It is mandatory to release a vector once user of test vector is finished
   with the vector. */
void
test_vectors_rsa_oaep_release(
        TestVector_RSA_OAEP_t Vector_p);


/* Request number of RSA PKCS#1v1.5 test vectors available. */
int
test_vectors_rsa_pkcs1v15wrap_num(void);

/* Request test vector by index.
   If Index >= test_vectors_rsa_pkcs1v15wrap_num(), the function returns NULL.

   Note: The function returns zero only when invalid vector has been requested.
   Note: The received vector is constant and must not be altered by caller.
   The test vector is described by structure TestVector_PKCS1V15WRAP_t and
   the structure shall be accessed directly by the user of the test vector.
*/
TestVector_PKCS1V15WRAP_t
test_vectors_rsa_pkcs1v15wrap_get(
        int Index);

/* Release a vector.
   It is mandatory to release a vector once user of test vector is finished
   with the vector. */
void
test_vectors_rsa_pkcs1v15wrap_release(
        TestVector_PKCS1V15WRAP_t Vector_p);


#endif /* Include guard */

/* end of file testvectors_rsa.h */
