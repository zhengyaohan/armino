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
// Copyright (C) 2020 Apple Inc. All Rights Reserved.

#include "HAPBitSet.h"

#include "HAPPlatform+Init.h"

#include "Harness/UnitTestFramework.h"

// Use TEST_ASSERT for mock assertions
#define MOCK_ASSERT TEST_ASSERT

#include "Harness/HAPPlatformServiceDiscoveryMock.h"

// Tests HAP Assert failure
TEST(TestHapAssert) {
    // Pass invalid argument on purpose to fail HAPPrecondition()
    uint8_t bits = 0xaa;
    bool result = HAPBitSetContains(&bits, sizeof bits, 8);

    // The following isn't going to be reached.
    TEST_ASSERT(result);
}

// Tests TEST_ASSERT() failure
TEST(TestAssert) {
    TEST_ASSERT(false);
}

// Tests TEST_ASSERT_EQUAL() failure
TEST(TestAssertEqualLong) {
    TEST_ASSERT_EQUAL(1, 2);
}

// Tests TEST_ASSERT_EQUAL() failure
TEST(TestAssertEqualLongLong) {
    uint64_t a = 0x100000000ull;
    TEST_ASSERT_EQUAL(a, 0u);
}

// Tests TEST_ASSERT_ENUM() failure
TEST(TestAssertEqualEnum) {
    TEST_ASSERT_EQUAL(kHAPError_Unknown, kHAPError_None);
}

// Tests TEST_ASSERT_EQUAL() failure
TEST(TestAssertEqualEnumInt) {
    TEST_ASSERT_EQUAL(kHAPError_None, 1);
}

// Tests TEST_ASSERT_EQUAL() failure
TEST(TestAssertEqualIntEnum) {
    TEST_ASSERT_EQUAL(1, kHAPError_None);
}

// Tests TEST_ASSERT_EQUAL() failure
TEST(TestAssertEqualDouble) {
    TEST_ASSERT_EQUAL(1.0, 2.0);
}

// Tests TEST_ASSERT_EQUAL() failure
TEST(TestAssertEqualString) {
    TEST_ASSERT_EQUAL("Foo", "Bar");
}

// Tests TEST_ASSERT_NE() failure
TEST(TestAssertNotEqualLong) {
    TEST_ASSERT_NE(1, 1);
}

// Tests TEST_ASSERT_NE() failure
TEST(TestAssertNotEqualLongLong) {
    TEST_ASSERT_NE(0x100000000ull, 0x100000000ull);
}

// Tests TEST_ASSERT_NE() failure
TEST(TestAssertNotEqualEnum) {
    TEST_ASSERT_NE(kHAPError_None, kHAPError_None);
}

// Tests TEST_ASSERT_NE() failure
TEST(TestAssertNotEqualEnumInt) {
    TEST_ASSERT_NE(kHAPError_None, 0);
}

// Tests TEST_ASSERT_NE() failure
TEST(TestAssertNotEqualIntEnum) {
    TEST_ASSERT_NE(0, kHAPError_None);
}

// Tests TEST_ASSERT_NE() failure
TEST(TestAssertNotEqualDouble) {
    TEST_ASSERT_NE(2.0, 2.0);
}

// Tests TEST_ASSERT_NE() failure
TEST(TestAssertNotEqualString) {
    TEST_ASSERT_NE("Foo", "Foo");
}

// Tests assert from within a mock function
TEST(TestAssertWithinMock) {
    HAP_PLATFORM_SERVICEDISCOVERY_MOCK(mock);

    bool failIt = true;
    EXPECT(mock, HAPPlatformServiceDiscoveryGetPort).Do([&](HAPPlatformServiceDiscoveryRef serviceDiscovery) {
        TEST_ASSERT(!failIt);
        return 3;
    });

    HAPPlatformServiceDiscovery sd;
    auto port = HAPPlatformServiceDiscoveryGetPort(&sd);

    // Test won't reach here anyways
    TEST_ASSERT_EQUAL(port, 3);
}

// Tests mock verification failure
TEST(TestMockVerification) {
    HAP_PLATFORM_SERVICEDISCOVERY_MOCK(mock);

    EXPECT(mock, HAPPlatformServiceDiscoveryGetPort).AtLeast(1);

    // No call, which should cause verification failure below.
    VERIFY_ALL(mock);
}

int main(int argc, char** argv) {
    HAPPlatformCreate();

    // All tests are expected to fail.
    return EXECUTE_TESTS_TO_FAIL(argc, (const char**) argv);
}
