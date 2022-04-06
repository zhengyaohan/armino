// Disclaimer: IMPORTANT: This Apple software is supplied to you, by Apple Inc. ("Apple"), in your
// capacity as a current, and in good standing, Licensee in the MFi Licensing Program. Use of this
// Apple software is governed by and subject to the terms and conditions of your MFi License,
// including, but not limited to, the restrictions specified in the provision entitled "Public
// Software", and is further subject to your agreement to the following additional terms, and your
// agreement that the use, installation, modification or redistribution of this Apple software
// constitutes acceptance of these additional terms. If you do not agree with these additional terms,
// you may not use, install, modify or redistribute this Apple software.
//
// Subject to all of these terms and in consideration of your agreement to abide by them, Apple grants
// you, for as long as you are a current and in good-standing MFi Licensee, a personal, non-exclusive
// license, under Apple's copyrights in this Apple software (the "Apple Software"), to use,
// reproduce, and modify the Apple Software in source form, and to use, reproduce, modify, and
// redistribute the Apple Software, with or without modifications, in binary form, in each of the
// foregoing cases to the extent necessary to develop and/or manufacture "Proposed Products" and
// "Licensed Products" in accordance with the terms of your MFi License. While you may not
// redistribute the Apple Software in source form, should you redistribute the Apple Software in binary
// form, you must retain this notice and the following text and disclaimers in all such redistributions
// of the Apple Software. Neither the name, trademarks, service marks, or logos of Apple Inc. may be
// used to endorse or promote products derived from the Apple Software without specific prior written
// permission from Apple. Except as expressly stated in this notice, no other rights or licenses,
// express or implied, are granted by Apple herein, including but not limited to any patent rights that
// may be infringed by your derivative works or by other works in which the Apple Software may be
// incorporated. Apple may terminate this license to the Apple Software by removing it from the list
// of Licensed Technology in the MFi License, or otherwise in accordance with the terms of such MFi License.
//
// Unless you explicitly state otherwise, if you provide any ideas, suggestions, recommendations, bug
// fixes or enhancements to Apple in connection with this software ("Feedback"), you hereby grant to
// Apple a non-exclusive, fully paid-up, perpetual, irrevocable, worldwide license to make, use,
// reproduce, incorporate, modify, display, perform, sell, make or have made derivative works of,
// distribute (directly or indirectly) and sublicense, such Feedback in connection with Apple products
// and services. Providing this Feedback is voluntary, but if you do provide Feedback to Apple, you
// acknowledge and agree that Apple may exercise the license granted above without the payment of
// royalties or further consideration to Participant.

// The Apple Software is provided by Apple on an "AS IS" basis. APPLE MAKES NO WARRANTIES, EXPRESS OR
// IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY
// AND FITNESS FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR
// IN COMBINATION WITH YOUR PRODUCTS.
//
// IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION
// AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
// (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (C) 2015-2020 Apple Inc. All Rights Reserved.

#include "HAPPlatform.h"

#define TEST_FROM_STRING(description, expectedValue) \
    do { \
        HAPPrecondition((description) != NULL); \
\
        HAPError err; \
\
        HAPLogInfo(&kHAPLog_Default, "Testing %s", description); \
        if ((expectedValue) >= 0) { \
            if ((uint64_t)(expectedValue) <= UINT64_MAX) { \
                HAPLogInfo(&kHAPLog_Default, "- Testing UInt64..."); \
                uint64_t value; \
                err = HAPUInt64FromString(description, &value); \
                HAPAssert(!err); \
                HAPAssert(value == (uint64_t)(expectedValue)); \
            } \
            if ((uint64_t)(expectedValue) <= UINT32_MAX) { \
                HAPLogInfo(&kHAPLog_Default, "- Testing UInt32..."); \
                uint32_t value; \
                err = HAPUInt32FromString(description, &value); \
                HAPAssert(!err); \
                HAPAssert(value == (uint32_t)(expectedValue)); \
            } \
            if ((uint64_t)(expectedValue) <= UINT16_MAX) { \
                HAPLogInfo(&kHAPLog_Default, "- Testing UInt16..."); \
                uint16_t value; \
                err = HAPUInt16FromString(description, &value); \
                HAPAssert(!err); \
                HAPAssert(value == (uint16_t)(expectedValue)); \
            } \
            if ((uint64_t)(expectedValue) <= UINT8_MAX) { \
                HAPLogInfo(&kHAPLog_Default, "- Testing UInt8..."); \
                uint8_t value; \
                err = HAPUInt8FromString(description, &value); \
                HAPAssert(!err); \
                HAPAssert(value == (uint8_t)(expectedValue)); \
            } \
            if ((uint64_t)(expectedValue) <= INT64_MAX) { \
                HAPLogInfo(&kHAPLog_Default, "- Testing Int64..."); \
                int64_t value; \
                err = HAPInt64FromString(description, &value); \
                HAPAssert(!err); \
                HAPAssert(value == (int64_t)(expectedValue)); \
            } \
        } \
        if ((expectedValue) < 0) { \
            if ((int64_t)(expectedValue) >= INT64_MIN) { \
                HAPLogInfo(&kHAPLog_Default, "- Testing Int64..."); \
                int64_t value; \
                err = HAPInt64FromString(description, &value); \
                HAPAssert(!err); \
                HAPAssert(value == (int64_t)(expectedValue)); \
            } \
        } \
    } while (0)

#define TEST_BORDER_CASE(description, limitThatIsExceeded) \
    do { \
        HAPPrecondition((description) != NULL); \
\
        HAPError err; \
\
        HAPLogInfo(&kHAPLog_Default, "Testing %s (expected fail)", description); \
        if ((limitThatIsExceeded) >= 0) { \
            if ((uint64_t)(limitThatIsExceeded) >= UINT64_MAX) { \
                HAPLogInfo(&kHAPLog_Default, "- Testing UInt64..."); \
                uint64_t value; \
                err = HAPUInt64FromString(description, &value); \
                HAPAssert(err); \
            } \
            if ((uint64_t)(limitThatIsExceeded) >= UINT32_MAX) { \
                HAPLogInfo(&kHAPLog_Default, "- Testing UInt32..."); \
                uint32_t value; \
                err = HAPUInt32FromString(description, &value); \
                HAPAssert(err); \
            } \
            if ((uint64_t)(limitThatIsExceeded) >= UINT16_MAX) { \
                HAPLogInfo(&kHAPLog_Default, "- Testing UInt16..."); \
                uint16_t value; \
                err = HAPUInt16FromString(description, &value); \
                HAPAssert(err); \
            } \
            if ((uint64_t)(limitThatIsExceeded) >= UINT8_MAX) { \
                HAPLogInfo(&kHAPLog_Default, "- Testing UInt8..."); \
                uint8_t value; \
                err = HAPUInt8FromString(description, &value); \
                HAPAssert(err); \
            } \
            if ((uint64_t)(limitThatIsExceeded) >= INT64_MAX) { \
                HAPLogInfo(&kHAPLog_Default, "- Testing Int64..."); \
                int64_t value; \
                err = HAPInt64FromString(description, &value); \
                HAPAssert(err); \
            } \
        } \
        if ((limitThatIsExceeded) <= 0) { \
            if ((int64_t)(limitThatIsExceeded) <= 0) { \
                HAPLogInfo(&kHAPLog_Default, "- Testing UInt64..."); \
                uint64_t value; \
                err = HAPUInt64FromString(description, &value); \
                HAPAssert(err); \
            } \
            if ((int64_t)(limitThatIsExceeded) <= 0) { \
                HAPLogInfo(&kHAPLog_Default, "- Testing UInt32..."); \
                uint32_t value; \
                err = HAPUInt32FromString(description, &value); \
                HAPAssert(err); \
            } \
            if ((int64_t)(limitThatIsExceeded) <= 0) { \
                HAPLogInfo(&kHAPLog_Default, "- Testing UInt16..."); \
                uint16_t value; \
                err = HAPUInt16FromString(description, &value); \
                HAPAssert(err); \
            } \
            if ((int64_t)(limitThatIsExceeded) <= 0) { \
                HAPLogInfo(&kHAPLog_Default, "- Testing UInt8..."); \
                uint8_t value; \
                err = HAPUInt8FromString(description, &value); \
                HAPAssert(err); \
            } \
            if ((int64_t)(limitThatIsExceeded) <= INT64_MIN) { \
                HAPLogInfo(&kHAPLog_Default, "- Testing Int64..."); \
                int64_t value; \
                err = HAPInt64FromString(description, &value); \
                HAPAssert(err); \
            } \
        } \
    } while (0)

#define TEST_FAIL(description) \
    do { \
        HAPPrecondition((description) != NULL); \
\
        HAPError err; \
\
        HAPLogInfo(&kHAPLog_Default, "Testing %s (expected fail)", description); \
        { \
            HAPLogInfo(&kHAPLog_Default, "- Testing UInt64..."); \
            uint64_t value; \
            err = HAPUInt64FromString(description, &value); \
            HAPAssert(err); \
        } \
        { \
            HAPLogInfo(&kHAPLog_Default, "- Testing UInt32..."); \
            uint32_t value; \
            err = HAPUInt32FromString(description, &value); \
            HAPAssert(err); \
        } \
        { \
            HAPLogInfo(&kHAPLog_Default, "- Testing UInt16..."); \
            uint16_t value; \
            err = HAPUInt16FromString(description, &value); \
            HAPAssert(err); \
        } \
        { \
            HAPLogInfo(&kHAPLog_Default, "- Testing UInt8..."); \
            uint8_t value; \
            err = HAPUInt8FromString(description, &value); \
            HAPAssert(err); \
        } \
        { \
            HAPLogInfo(&kHAPLog_Default, "- Testing Int64..."); \
            int64_t value; \
            err = HAPInt64FromString(description, &value); \
            HAPAssert(err); \
        } \
    } while (0)

#define TEST_GET_DESCRIPTION(value, expectedDescription) \
    do { \
        HAPError err; \
\
        HAPLogInfo(&kHAPLog_Default, "Testing %s (get description)", #value); \
        if ((value) >= 0) { \
            if ((uint64_t)(value) <= UINT64_MAX) { \
                HAPLogInfo(&kHAPLog_Default, "- Testing UInt64..."); \
                size_t actualNumBytes = HAPUInt64GetNumDescriptionBytes((uint64_t)(value)); \
                HAPAssert(actualNumBytes == sizeof(expectedDescription) - 1); \
                char description[sizeof(expectedDescription) + 1]; \
                err = HAPUInt64GetDescription((uint64_t)(value), description, sizeof(expectedDescription) + 1); \
                HAPAssert(!err); \
                HAPAssert(HAPStringAreEqual(description, (expectedDescription))); \
                err = HAPUInt64GetDescription((uint64_t)(value), description, sizeof(expectedDescription)); \
                HAPAssert(!err); \
                HAPAssert(HAPStringAreEqual(description, (expectedDescription))); \
                err = HAPUInt64GetDescription((uint64_t)(value), description, sizeof(expectedDescription) - 1); \
                HAPAssert(err == kHAPError_OutOfResources); \
            } \
            if ((uint64_t)(value) <= INT32_MAX) { \
                HAPLogInfo(&kHAPLog_Default, "- Testing Int32..."); \
                size_t actualNumBytes = HAPInt32GetNumDescriptionBytes((int32_t)(value)); \
                HAPAssert(actualNumBytes == sizeof(expectedDescription) - 1); \
            } \
        } \
        if ((value) < 0) { \
            if ((int64_t)(value) >= INT32_MIN) { \
                HAPLogInfo(&kHAPLog_Default, "- Testing Int32..."); \
                size_t actualNumBytes = HAPInt32GetNumDescriptionBytes((int32_t)(value)); \
                HAPAssert(actualNumBytes == sizeof(expectedDescription) - 1); \
            } \
        } \
    } while (0)

#define TEST_UINT8_FROM_HEX(input, expectedValue, expectedResult) \
    do { \
\
        HAPLogInfo(&kHAPLog_Default, "Testing  %c", input); \
        uint8_t actualValue = 0; \
        HAPError actualResult = HAPUInt8FromHexDigit(input, &actualValue); \
        HAPAssert(actualResult == expectedResult); \
        if (actualResult == kHAPError_None) { \
            HAPAssert(actualValue == expectedValue); \
        } \
    } while (0)

int main() {
    // Zero.
    TEST_FROM_STRING("0", 0);
    TEST_FROM_STRING("+0", 0);
    TEST_FROM_STRING("-0", 0);
    TEST_FROM_STRING("00", 0);
    TEST_GET_DESCRIPTION(0, "0");

    // Some random numbers.
    TEST_FROM_STRING("1", 1);
    TEST_FROM_STRING("2", 2);
    TEST_FROM_STRING("123", 123);
    TEST_FROM_STRING("-1", -1);
    TEST_FROM_STRING("-2", -2);
    TEST_FROM_STRING("-123", -123);
    TEST_GET_DESCRIPTION(1, "1");
    TEST_GET_DESCRIPTION(2, "2");
    TEST_GET_DESCRIPTION(123, "123");
    TEST_GET_DESCRIPTION(-1, "-1");
    TEST_GET_DESCRIPTION(-2, "-2");
    TEST_GET_DESCRIPTION(2, "2");

    // Border cases (UInt64).
    TEST_BORDER_CASE("-10000000000000000000000000", 0);
    TEST_BORDER_CASE("-1", 0);
    TEST_FROM_STRING("-0", 0);
    TEST_FROM_STRING("+18446744073709551615", UINT64_MAX);
    TEST_GET_DESCRIPTION(UINT64_MAX, "18446744073709551615");
    TEST_BORDER_CASE("+18446744073709551616", UINT64_MAX);
    TEST_BORDER_CASE("+10000000000000000000000000", UINT64_MAX);

    // Border cases (UInt32).
    TEST_BORDER_CASE("4294967296", UINT32_MAX);
    TEST_BORDER_CASE("+4294967296", UINT32_MAX);

    // Border cases (UInt16).
    TEST_BORDER_CASE("65536", UINT16_MAX);
    TEST_BORDER_CASE("+65536", UINT16_MAX);

    // Border cases (UInt8).
    TEST_BORDER_CASE("256", UINT8_MAX);
    TEST_BORDER_CASE("+256", UINT8_MAX);

    // Border cases (Int64).
    TEST_BORDER_CASE("-10000000000000000000000000", INT64_MIN);
    TEST_BORDER_CASE("-9223372036854775809", INT64_MIN);
    TEST_FROM_STRING("-9223372036854775808", INT64_MIN);
    TEST_FROM_STRING("+9223372036854775807", INT64_MAX);
    TEST_GET_DESCRIPTION(INT64_MIN, "-9223372036854775808");
    TEST_GET_DESCRIPTION(INT64_MAX, "9223372036854775807");
    TEST_BORDER_CASE("+9223372036854775808", INT64_MAX);
    TEST_BORDER_CASE("+10000000000000000000000000", INT64_MAX);

    // Empty string.
    TEST_FAIL("");
    TEST_FAIL("+");
    TEST_FAIL("-");

    // Whitespace.
    TEST_FAIL(" 100");
    TEST_FAIL("1 00");
    TEST_FAIL("100 ");
    TEST_FAIL("+ 100");
    TEST_FAIL("+1 00");
    TEST_FAIL("+100 ");
    TEST_FAIL("- 100");
    TEST_FAIL("-1 00");
    TEST_FAIL("-100 ");

    // Invalid format.
    TEST_FAIL("21-50");
    TEST_FAIL("ff6600");

    HAPAssert(HAPGetVariableIntEncodingLength(0) == 1);
    HAPAssert(HAPGetVariableIntEncodingLength(0x10) == 1);
    HAPAssert(HAPGetVariableIntEncodingLength(0x100) == 2);
    HAPAssert(HAPGetVariableIntEncodingLength(0x1000) == 2);
    HAPAssert(HAPGetVariableIntEncodingLength(0x10000) == 4);
    HAPAssert(HAPGetVariableIntEncodingLength(0x100000) == 4);
    HAPAssert(HAPGetVariableIntEncodingLength(0x100000000ull) == 8);
    HAPAssert(HAPGetVariableIntEncodingLength(0x1000000000000ull) == 8);

    // Test all valid input
    TEST_UINT8_FROM_HEX('0', 0, kHAPError_None);
    TEST_UINT8_FROM_HEX('1', 1, kHAPError_None);
    TEST_UINT8_FROM_HEX('2', 2, kHAPError_None);
    TEST_UINT8_FROM_HEX('3', 3, kHAPError_None);
    TEST_UINT8_FROM_HEX('4', 4, kHAPError_None);
    TEST_UINT8_FROM_HEX('5', 5, kHAPError_None);
    TEST_UINT8_FROM_HEX('6', 6, kHAPError_None);
    TEST_UINT8_FROM_HEX('7', 7, kHAPError_None);
    TEST_UINT8_FROM_HEX('8', 8, kHAPError_None);
    TEST_UINT8_FROM_HEX('9', 9, kHAPError_None);
    TEST_UINT8_FROM_HEX('A', 10, kHAPError_None);
    TEST_UINT8_FROM_HEX('B', 11, kHAPError_None);
    TEST_UINT8_FROM_HEX('C', 12, kHAPError_None);
    TEST_UINT8_FROM_HEX('D', 13, kHAPError_None);
    TEST_UINT8_FROM_HEX('E', 14, kHAPError_None);
    TEST_UINT8_FROM_HEX('F', 15, kHAPError_None);
    TEST_UINT8_FROM_HEX('a', 10, kHAPError_None);
    TEST_UINT8_FROM_HEX('b', 11, kHAPError_None);
    TEST_UINT8_FROM_HEX('c', 12, kHAPError_None);
    TEST_UINT8_FROM_HEX('d', 13, kHAPError_None);
    TEST_UINT8_FROM_HEX('e', 14, kHAPError_None);
    TEST_UINT8_FROM_HEX('f', 15, kHAPError_None);

    // Test some invalid input
    TEST_UINT8_FROM_HEX('h', 12, kHAPError_InvalidData);
    TEST_UINT8_FROM_HEX('.', 0, kHAPError_InvalidData);
    TEST_UINT8_FROM_HEX('z', 0, kHAPError_InvalidData);
    TEST_UINT8_FROM_HEX('-', 0, kHAPError_InvalidData);
}
