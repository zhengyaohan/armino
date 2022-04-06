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

#include "HAPPlatformTimer.h"
#include "HAPPlatformClock.h"

#include "HAP+API.h"

#include "Harness/HAPPlatformClockHelper.h"
#include "Harness/HAPPlatformTimerHelper.h"
#include "Harness/UnitTestFramework.h"

// Use TEST_ASSERT for mock assertions
#define MOCK_ASSERT TEST_ASSERT

// Utility timer callback function mock just for this test suite
#include "Harness/Mock.h"
#include "Harness/MockMacros.h"
#define MOCK_FUNCS \
    MOCK_VOID_FUNC_DEF( \
            TimerCallback, MOCK_ARGS(MOCK_ARG(HAPPlatformTimerRef, timer), MOCK_ARG(void* _Nullable, context)))
struct TestCallbackMockMatchEntryStorages {
#include "Harness/MockExpandCppMatchEntryStorages.hpp"
};
typedef adk_unittest::Mock<TestCallbackMockMatchEntryStorages> TestCallbackMock;
TestCallbackMock* _Nullable globalTestCallbackMock;
#define CPP_MOCK_POINTER globalTestCallbackMock
#include "Harness/MockExpandFuncsCallingCppMock.h"
#undef CPP_MOCK_POINTER
#undef MOCK_FUNCS
#define TEST_CALLBACK_MOCK(_mock) TestCallbackMock _mock(&globalTestCallbackMock)

/**
 * Teardown step after a test so that next test can run in a clean state.
 */
TEST_TEARDOWN() {
    // Clears all timers
    HAPPlatformTimerDeregisterAll();
}

// Test HAPPlatformTimerDeregisterAll() function which is provide only for unit testing
TEST(TestDeregisterAll) {
    TEST_CALLBACK_MOCK(mock);

    HAPPlatformTimerRef timer;
    int myContext = 1;
    HAPError err = HAPPlatformTimerRegister(
            &timer, HAPPlatformClockGetCurrent() + HAPSecond, TimerCallback, (void*) &myContext);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    // Timer should be called
    EXPECT(mock, TimerCallback)
            .If([&](HAPPlatformTimerRef timerid, void* _Nullable context) {
                return (timerid == timer && context == (void*) &myContext);
            })
            .AtLeast(1)
            .AtMost(1);

    HAPPlatformClockAdvance(HAPSecond);

    VERIFY_ALL(mock);

    // Schedule another two
    err = HAPPlatformTimerRegister(&timer, HAPPlatformClockGetCurrent() + HAPSecond, TimerCallback, (void*) &myContext);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    HAPPlatformTimerRef secondTimer;
    err = HAPPlatformTimerRegister(
            &secondTimer, HAPPlatformClockGetCurrent() + HAPSecond * 2, TimerCallback, (void*) &myContext);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    // We'll deregister all timers. Hence we are not expecting any timer expiry.
    EXPECT(mock, TimerCallback).AtMost(0);

    // Deregister all timers before any timer expires.
    HAPPlatformClockAdvance(HAPMillisecond * 500);
    HAPPlatformTimerDeregisterAll();

    // Advance clock to have expired all timers if they were not deregistered.
    HAPPlatformClockAdvance(HAPMinute);

    VERIFY_ALL(mock);

    // Make sure timer still works fine
    err = HAPPlatformTimerRegister(&timer, HAPPlatformClockGetCurrent() + HAPSecond, TimerCallback, (void*) &myContext);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    EXPECT(mock, TimerCallback)
            .If([&](HAPPlatformTimerRef timerid, void* _Nullable context) {
                return (timerid == timer && context == (void*) &myContext);
            })
            .AtLeast(1)
            .AtMost(1);

    HAPPlatformClockAdvance(HAPSecond);

    VERIFY_ALL(mock);
}

// Test HAPPlatformTimerDeregisterAll() working properly when it's called from within a callback.
TEST(TestDeregisterAllFromWithinCallback) {
    TEST_CALLBACK_MOCK(mock);

    // Set up two timers
    HAPPlatformTimerRef timer;
    int myContext = 1;
    HAPError err = HAPPlatformTimerRegister(
            &timer, HAPPlatformClockGetCurrent() + HAPSecond, TimerCallback, (void*) &myContext);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    HAPPlatformTimerRef secondTimer;
    err = HAPPlatformTimerRegister(
            &secondTimer, HAPPlatformClockGetCurrent() + HAPSecond * 2, TimerCallback, (void*) &myContext);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    // We'll call deregister all from within the first callback.
    EXPECT(mock, TimerCallback)
            .If([&](HAPPlatformTimerRef timerid, void* _Nullable context) { return (timerid == timer); })
            .Do([](HAPPlatformTimerRef timerid, void* _Nullable context) { HAPPlatformTimerDeregisterAll(); })
            .AtLeast(1)
            .AtMost(1);

    // Other timer shouldn't expire.
    EXPECT(mock, TimerCallback).AtMost(0);

    // First timer should expire.
    HAPPlatformClockAdvance(HAPSecond);

    // It should have cancelled out the 2nd timer
    HAPPlatformClockAdvance(HAPMinute);

    VERIFY_ALL(mock);

    // Make sure timer still works fine
    err = HAPPlatformTimerRegister(&timer, HAPPlatformClockGetCurrent() + HAPSecond, TimerCallback, (void*) &myContext);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    EXPECT(mock, TimerCallback)
            .If([&](HAPPlatformTimerRef timerid, void* _Nullable context) {
                return (timerid == timer && context == (void*) &myContext);
            })
            .AtLeast(1)
            .AtMost(1);

    HAPPlatformClockAdvance(HAPSecond);

    VERIFY_ALL(mock);
}

// Test HAPPlatformTimerEmulateClockAdvances() function
TEST(TestEmulateClockAdvances) {
    TEST_CALLBACK_MOCK(mock);

    // Set up two timers
    HAPPlatformTimerRef firstTimer;
    int myContext = 1;
    HAPError err = HAPPlatformTimerRegister(
            &firstTimer, HAPPlatformClockGetCurrent() + HAPSecond, TimerCallback, (void*) &myContext);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    HAPPlatformTimerRef secondTimer;
    err = HAPPlatformTimerRegister(
            &secondTimer, HAPPlatformClockGetCurrent() + HAPSecond * 3, TimerCallback, (void*) &myContext);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    // Set up a third timer upon first timer expiry
    HAPPlatformTimerRef thirdTimer;
    EXPECT(mock, TimerCallback)
            .If([&](HAPPlatformTimerRef timerid, void* _Nullable context) { return (timerid == firstTimer); })
            .Do([&](HAPPlatformTimerRef timerid, void* _Nullable context) {
                HAPError err = HAPPlatformTimerRegister(
                        &thirdTimer, HAPPlatformClockGetCurrent() + HAPSecond, TimerCallback, context);
                TEST_ASSERT_EQUAL(err, kHAPError_None);
            })
            .AtLeast(1)
            .AtMost(1);
    // 2nd timer callback is expected
    EXPECT(mock, TimerCallback)
            .If([&](HAPPlatformTimerRef timerid, void* _Nullable context) { return (timerid == secondTimer); })
            .AtLeast(1)
            .AtMost(1);
    // 3rd timer callback is not expected when we use HAPPlatformClockAdvance().
    EXPECT(mock, TimerCallback)
            .If([&](HAPPlatformTimerRef timerid, void* _Nullable context) { return (timerid == thirdTimer); })
            .AtMost(0);

    // Advance timer.
    HAPPlatformClockAdvance(HAPSecond * 3);
    VERIFY_ALL(mock);

    // Note that 3rd timer will expire in one second from the timer tick where the first timer callback is called,
    // that is, from now.
    EXPECT(mock, TimerCallback)
            .If([&](HAPPlatformTimerRef timerid, void* _Nullable context) { return (timerid == thirdTimer); })
            .AtMost(0);
    HAPPlatformClockAdvance(HAPMillisecond * 500);
    VERIFY_ALL(mock);

    EXPECT(mock, TimerCallback)
            .If([&](HAPPlatformTimerRef timerid, void* _Nullable context) { return (timerid == thirdTimer); })
            .AtLeast(1)
            .AtMost(1);
    HAPPlatformClockAdvance(HAPMillisecond * 500);
    VERIFY_ALL(mock);

    // Set up two timer again.
    err = HAPPlatformTimerRegister(
            &firstTimer, HAPPlatformClockGetCurrent() + HAPSecond, TimerCallback, (void*) &myContext);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    err = HAPPlatformTimerRegister(
            &secondTimer, HAPPlatformClockGetCurrent() + HAPSecond * 3, TimerCallback, (void*) &myContext);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    // Set up a third timer upon first timer expiry like before.
    EXPECT(mock, TimerCallback)
            .If([&](HAPPlatformTimerRef timerid, void* _Nullable context) { return (timerid == firstTimer); })
            .Do([&](HAPPlatformTimerRef timerid, void* _Nullable context) {
                HAPError err = HAPPlatformTimerRegister(
                        &thirdTimer, HAPPlatformClockGetCurrent() + HAPSecond, TimerCallback, context);
                TEST_ASSERT_EQUAL(err, kHAPError_None);
            })
            .AtLeast(1)
            .AtMost(1);
    bool thirdTimerExpired = false;
    // 3rd timer callback is expected this time when we use HAPPlatformTimerEmulateClockAdvances().
    EXPECT(mock, TimerCallback)
            .If([&](HAPPlatformTimerRef timerid, void* _Nullable context) { return (timerid == thirdTimer); })
            .Do([&](HAPPlatformTimerRef timerid, void* _Nullable context) { thirdTimerExpired = true; })
            .AtLeast(1)
            .AtMost(1);
    // 2nd timer callback is expected after the third timer callback
    EXPECT(mock, TimerCallback)
            .If([&](HAPPlatformTimerRef timerid, void* _Nullable context) { return (timerid == secondTimer); })
            .Do([&](HAPPlatformTimerRef timerid, void* _Nullable context) { TEST_ASSERT(thirdTimerExpired); })
            .AtLeast(1)
            .AtMost(1);

    HAPTime now = HAPPlatformClockGetCurrent();
    HAPPlatformTimerEmulateClockAdvances(HAPSecond * 4);
    TEST_ASSERT_EQUAL(HAPPlatformClockGetCurrent(), now + HAPSecond * 4);
    VERIFY_ALL(mock);
}

int main(int argc, char** argv) {
    TEST_ASSERT(HAPGetCompatibilityVersion() == HAP_COMPATIBILITY_VERSION);

    return EXECUTE_TESTS(argc, (const char**) argv);
}
