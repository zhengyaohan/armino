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

#include "HAPPlatform+Init.h"

#include "Harness/UnitTestFramework.h"

// Use TEST_ASSERT for mock assertions
#define MOCK_ASSERT TEST_ASSERT

#include "Harness/HAPPlatformServiceDiscoveryMock.h"

extern "C" {
static const HAPLogObject logObject = { .subsystem = "com.apple.mfi.HomeKit.Core.Test", .category = "TestController" };
}

TEST(TestIf) {
    HAP_PLATFORM_SERVICEDISCOVERY_MOCK(mock);

    HAPPlatformServiceDiscovery sd1, sd2, sd3;

    // Test that if conditions are picked up in the order of configuration.
    EXPECT(mock, HAPPlatformServiceDiscoveryGetProtocol)
            .If([&](HAPPlatformServiceDiscoveryRef serviceDiscovery) { return serviceDiscovery == &sd1; })
            .Return("My Protocol 1");
    EXPECT(mock, HAPPlatformServiceDiscoveryGetProtocol)
            .If([&](HAPPlatformServiceDiscoveryRef serviceDiscovery) { return serviceDiscovery == &sd2; })
            .Return("My Protocol 2");
    EXPECT(mock, HAPPlatformServiceDiscoveryGetProtocol).Return("Bad Protocol");

    const char* rv = HAPPlatformServiceDiscoveryGetProtocol(&sd3);
    TEST_ASSERT_EQUAL(rv, "Bad Protocol");
    rv = HAPPlatformServiceDiscoveryGetProtocol(&sd1);
    TEST_ASSERT_EQUAL(rv, "My Protocol 1");
    rv = HAPPlatformServiceDiscoveryGetProtocol(&sd2);
    TEST_ASSERT_EQUAL(rv, "My Protocol 2");
    VERIFY_ALL(mock);

    EXPECT(mock, HAPPlatformServiceDiscoveryGetProtocol)
            .If([&](HAPPlatformServiceDiscoveryRef serviceDiscovery) { return serviceDiscovery == &sd1; })
            .Return("My Protocol 1");
    EXPECT(mock, HAPPlatformServiceDiscoveryGetProtocol)
            .If([&](HAPPlatformServiceDiscoveryRef serviceDiscovery) { return serviceDiscovery == &sd2; })
            .Return("My Protocol 2");
    EXPECT(mock, HAPPlatformServiceDiscoveryGetProtocol).Return("Bad Protocol").AtMost(0);

    // The following call violates the expectations set above that no call should be made with
    // serviceDiscovery argument other than either &sd1 or &sd2.
    // It will result into an error during verification.
    rv = HAPPlatformServiceDiscoveryGetProtocol(&sd3);

    // Since we are expecting a verification failure, the following uses VERIFY_NO_ASSERT() macro to check
    // that the verification actually failed.
    // Note that VERIFY_NO_ASSERT() macro isn't intended for general use and it is intended only to test
    // the Mock framework itself.
    HAPLog(&logObject, "Note that error logs are expected below.");
    TEST_ASSERT(!VERIFY_NO_ASSERT(mock, HAPPlatformServiceDiscoveryGetProtocol));

    // Note that if VERIFY_NO_ASSERT() is not called above, the following VERIFY_ALL would have failed.
    VERIFY_ALL(mock);

    // Test multiple If conditions (OR'd)
    EXPECT(mock, HAPPlatformServiceDiscoveryGetProtocol)
            .If([&](HAPPlatformServiceDiscoveryRef serviceDiscovery) { return serviceDiscovery == &sd1; })
            .If([&](HAPPlatformServiceDiscoveryRef serviceDiscovery) { return serviceDiscovery == &sd2; })
            .Return("My Protocol");
    EXPECT(mock, HAPPlatformServiceDiscoveryGetProtocol).Return("Bad Protocol");

    rv = HAPPlatformServiceDiscoveryGetProtocol(&sd1);
    TEST_ASSERT_EQUAL(rv, "My Protocol");
    rv = HAPPlatformServiceDiscoveryGetProtocol(&sd2);
    TEST_ASSERT_EQUAL(rv, "My Protocol");
    rv = HAPPlatformServiceDiscoveryGetProtocol(&sd3);
    TEST_ASSERT_EQUAL(rv, "Bad Protocol");

    VERIFY_ALL(mock);
}

TEST(TestRepeats) {
    HAP_PLATFORM_SERVICEDISCOVERY_MOCK(mock);

    HAPPlatformServiceDiscovery sd1, sd2, sd3;

    // Test that if conditions are picked up in the order of configuration.
    EXPECT(mock, HAPPlatformServiceDiscoveryGetProtocol)
            .If([&](HAPPlatformServiceDiscoveryRef serviceDiscovery) { return serviceDiscovery == &sd2; })
            .Return("My Protocol 2");
    EXPECT(mock, HAPPlatformServiceDiscoveryGetProtocol)
            .If([&](HAPPlatformServiceDiscoveryRef serviceDiscovery) { return serviceDiscovery == &sd1; })
            .Return("My Protocol 1-1")
            .Repeats(2);
    EXPECT(mock, HAPPlatformServiceDiscoveryGetProtocol)
            .If([&](HAPPlatformServiceDiscoveryRef serviceDiscovery) { return serviceDiscovery == &sd1; })
            .Return("My Protocol 1-2")
            .Repeats(1);
    EXPECT(mock, HAPPlatformServiceDiscoveryGetProtocol)
            .If([&](HAPPlatformServiceDiscoveryRef serviceDiscovery) { return serviceDiscovery == &sd1; })
            .Return("My Protocol 1-3");
    EXPECT(mock, HAPPlatformServiceDiscoveryGetProtocol).Return("Bad");

    const char* rv = HAPPlatformServiceDiscoveryGetProtocol(&sd2);
    TEST_ASSERT_EQUAL(rv, "My Protocol 2");
    rv = HAPPlatformServiceDiscoveryGetProtocol(&sd1);
    TEST_ASSERT_EQUAL(rv, "My Protocol 1-1");

    rv = HAPPlatformServiceDiscoveryGetProtocol(&sd2);
    TEST_ASSERT_EQUAL(rv, "My Protocol 2");
    rv = HAPPlatformServiceDiscoveryGetProtocol(&sd1);
    TEST_ASSERT_EQUAL(rv, "My Protocol 1-1");

    rv = HAPPlatformServiceDiscoveryGetProtocol(&sd3);
    TEST_ASSERT_EQUAL(rv, "Bad");
    rv = HAPPlatformServiceDiscoveryGetProtocol(&sd1);
    TEST_ASSERT_EQUAL(rv, "My Protocol 1-2");

    rv = HAPPlatformServiceDiscoveryGetProtocol(&sd3);
    TEST_ASSERT_EQUAL(rv, "Bad");
    rv = HAPPlatformServiceDiscoveryGetProtocol(&sd1);
    TEST_ASSERT_EQUAL(rv, "My Protocol 1-3");

    rv = HAPPlatformServiceDiscoveryGetProtocol(&sd2);
    TEST_ASSERT_EQUAL(rv, "My Protocol 2");
    rv = HAPPlatformServiceDiscoveryGetProtocol(&sd1);
    TEST_ASSERT_EQUAL(rv, "My Protocol 1-3");

    VERIFY_ALL(mock);
}

int main(int argc, char** argv) {
    return EXECUTE_TESTS(argc, (const char**) argv);
}
