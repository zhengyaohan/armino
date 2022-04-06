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
// Copyright (C) 2020-2021 Apple Inc. All Rights Reserved.

#ifndef __UNIT_TEST_FRAMEWORK_H_
#define __UNIT_TEST_FRAMEWORK_H_

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __EXCEPTIONS
#include <exception>
#else
#include <setjmp.h>
#endif

#include <vector>

#include "HAPBase.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

extern "C" {
static const HAPLogObject testFrameworkLogObject = { .subsystem = "com.apple.mfi.HomeKit.Core.Test",
                                                     .category = "UnitTest" };

static const HAPLogObject testReportLogObject = { .subsystem = "com.apple.mfi.HomeKit.Core.Test",
                                                  .category = "Report" };
}

namespace adk_unittest {

/** Test function type */
typedef void (*TestFunction)();

/** Setup function type */
typedef void (*TestSetupFunction)();

/** Teardown function type */
typedef void (*TestTeardownFunction)();

class Test;

#if defined(__APPLE__) && defined(__MACH__)
#define TEST_ASSERT_MESSAGE_MAX_BYTES ((size_t)(256))
#else
#define TEST_ASSERT_MESSAGE_MAX_BYTES ((size_t)(100))
#endif

/**
 * Function to register test failure function for external harness modules
 *
 * This function is used internally within unit test framework modules. Do not use this function from tests.
 */
extern void RegisterUnitTestFailureFunction(void (*fn)(const char* msg));

/**
 * Failure reporting function to be used by test harness modules (external to the tests themselves)
 *
 * This function is referenced internally within unit test framework modules. Do not use this function from tests
 * directly.
 */
static void TestHarnessFailureFunction(const char* msg);

/**
 * Test registry
 */
class TestRegistry {
private:
    std::vector<Test*> testList;
    TestSetupFunction setupRoutine;
    TestTeardownFunction teardownRoutine;
#ifndef __EXCEPTIONS
    jmp_buf jumpBuffer;
    char* jumpMessage;
#endif

public:
    /** constructor */
    TestRegistry()
        : setupRoutine(NULL)
        , teardownRoutine(NULL) {
#ifndef __EXCEPTIONS
        jumpMessage = NULL;
#endif
    }

    /** destructor */
    ~TestRegistry() {
#ifndef __EXCEPTIONS
        if (jumpMessage) {
            delete[] jumpMessage;
        }
#endif
    }

    /** Registers a test */
    void Register(Test* test) {
        testList.push_back(test);
    }

    /**
     * Executes tests
     *
     * @param suiteName      Name of the test suite to be printed with the result report
     * @param argc           Argument count
     * @param argv           Arguments.<br>The 2nd argument and on will be the name of tests to run.
     *                       If no arguments are specified, all registered tests will be executed.
     * @param expectFailures This parameter is to test the test framework itself.
     *                       If set to true, all tests must fail in order to pass the test suite.
     *                       If set to false, all tests must succeed in order to pass the test suite.
     *
     * @return zero if all tests either passed or failed depending on the @ref expectFailures value.
     *         1, otherwise.
     */
    int Execute(const char* suiteName, int argc, const char* _Nonnull* _Nonnull argv, bool expectFailures);

    /**
     * Executes tests
     *
     * @param suiteName      Name of the test suite to be printed with the result report
     * @param argc           Argument count
     * @param argv           Arguments.<br>The 2nd argument and on will be the name of tests to run.
     *                       If no arguments are specified, all registered tests will be executed.
     *
     * @return zero if all tests passed. 1, otherwise.
     */
    int Execute(const char* suiteName, int argc, const char* _Nonnull* _Nonnull argv) {
        return Execute(suiteName, argc, argv, false);
    }

    /**
     * Executes setup routine before each test case execution
     */
    void Setup() {
        if (setupRoutine) {
            setupRoutine();
        }
    }

    /**
     * Executes teardown routine after each test case execution
     */
    void Teardown() {
        if (teardownRoutine) {
            teardownRoutine();
        }
    }

    /**
     * Registers a setup routine
     */
    void RegisterSetupRoutine(TestSetupFunction setupRoutine) {
        this->setupRoutine = setupRoutine;
    }

    /**
     * Registers a teardown routine
     */
    void RegisterTeardownRoutine(TestTeardownFunction teardownRoutine) {
        this->teardownRoutine = teardownRoutine;
    }

#ifndef __EXCEPTIONS
    /**
     * Fetches and returns the jump message.
     * The caller will be responsible to delete the returned char array.
     */
    const char* GetJumpMessage() {
        const char* tmp = (const char*) jumpMessage;
        jumpMessage = NULL;
        return tmp;
    }

    /**
     * Long-jumps upon an error with a formatted message.
     */
    HAP_NORETURN
    void LongJump(const char* format, ...) {
        if (jumpMessage) {
            delete[] jumpMessage;
            jumpMessage = NULL;
        }
        va_list argsPostScan;
        va_start(argsPostScan, format);

        jumpMessage = new char[TEST_ASSERT_MESSAGE_MAX_BYTES];
        if (jumpMessage) {
            HAPError err HAP_UNUSED =
                    HAPStringWithFormatAndArguments(jumpMessage, TEST_ASSERT_MESSAGE_MAX_BYTES, format, argsPostScan);
        }
        va_end(argsPostScan);

        longjmp(jumpBuffer, 1);
    }

    /**
     * Gets jump environment buffer
     */
    jmp_buf& GetJumpBuffer() {
        return jumpBuffer;
    }
#endif
};

/** A test registry */
static TestRegistry testRegistry;

#ifdef __EXCEPTIONS
/** Test Failure exception */
class TestFailure : public std::exception {
private:
    const char* failureMessage;

public:
    TestFailure(const char* format, ...) {
        va_list argsPostScan;
        va_start(argsPostScan, format);
        char* message = new char[TEST_ASSERT_MESSAGE_MAX_BYTES];
        if (message) {
            HAPError err =
                    HAPStringWithFormatAndArguments(message, TEST_ASSERT_MESSAGE_MAX_BYTES, format, argsPostScan);
            if (err) {
                delete[] message;
                failureMessage = NULL;
                va_end(argsPostScan);
                return;
            }
        }
        failureMessage = message;
        va_end(argsPostScan);
    }

    ~TestFailure() {
        if (failureMessage) {
            delete[] failureMessage;
        }
    }

    const char* what() const throw() {
        if (!failureMessage) {
            return "Bad failure message format";
        }
        return failureMessage;
    }
};
#endif

/** Test */
class Test {
private:
    /** test name */
    const char* name;

    /** test function */
    TestFunction function;

public:
    /** constructor */
    Test(const char* name, TestFunction function)
        : name(name)
        , function(function) {
        testRegistry.Register(this);
    }

    /** Gets the name of the test */
    const char* GetName() {
        return name;
    }

    /**
     * Executes the test.
     *
     * @param[out] failureMessage   Pointer to set to failure message string.
     *             If this is set, the string must be deleted by the caller.
     * @return true if test succeeded. false, if test failed.
     */
    bool Execute(const char* _Nullable& failureMessage) {
        bool succeeded = true;
        failureMessage = NULL;
        bool runTeardown = false;
#ifdef __EXCEPTIONS
        try {
            testRegistry.Setup();
            runTeardown = true;
            function();
        } catch (TestFailure& failure) {
            succeeded = false;
            failureMessage = GetFailureMessage(failure);
        }
        if (runTeardown) {
            try {
                testRegistry.Teardown();
            } catch (TestFailure& failure) {
                if (succeeded) {
                    succeeded = false;
                    if (failureMessage) {
                        delete[] failureMessage;
                    }
                    failureMessage = GetFailureMessage(failure);
                }
            }
        }
#else
        int jumpValue = setjmp(testRegistry.GetJumpBuffer());
        if (jumpValue == 0) {
            testRegistry.Setup();
            runTeardown = true;
            function();
        } else {
            // Error occurred
            succeeded = false;
            failureMessage = GetFailureMessage();
        }

        if (runTeardown) {
            jumpValue = setjmp(testRegistry.GetJumpBuffer());

            if (jumpValue == 0) {
                testRegistry.Teardown();
            } else {
                // error occurred during teardown
                if (succeeded) {
                    succeeded = false;
                    if (failureMessage) {
                        delete[] failureMessage;
                    }
                    failureMessage = GetFailureMessage();
                }
            }
        }
#endif
        return succeeded;
    }

private:
#if __EXCEPTIONS
    /** Creates a copy of failure message string from an exception */
    const char* GetFailureMessage(TestFailure& failure) {
        const char* msg = failure.what();
        char* buf = NULL;
        if (msg) {
            size_t len = HAPStringGetNumBytes(msg) + 1;
            buf = new char[len];
            if (buf) {
                HAPRawBufferCopyBytes((void*) buf, (void*) msg, len);
            }
        }
        return (const char*) buf;
    }
#else
    /** Creates a copy of failure message string from the last error */
    const char* GetFailureMessage() {
        return testRegistry.GetJumpMessage();
    }
#endif
};

struct TestSetup {
public:
    TestSetup(TestSetupFunction function) {
        testRegistry.RegisterSetupRoutine(function);
    }
};

struct TestTeardown {
public:
    TestTeardown(TestTeardownFunction function) {
        testRegistry.RegisterTeardownRoutine(function);
    }
};

/** Failed test result data structure per test case */
struct FailedTestResult {
    const char* testCaseName;   /**< test case name */
    const char* failureMessage; /**< failure message */
};

int TestRegistry::Execute(const char* suiteName, int argc, const char* _Nonnull* _Nonnull argv, bool expectFailures) {
    size_t passedTestCount = 0;
    std::vector<FailedTestResult> failedTestResults;
    std::vector<const char*> missingTestNames;

    RegisterUnitTestFailureFunction(TestHarnessFailureFunction);

    if (expectFailures) {
        HAPLog(&testFrameworkLogObject,
               "*******************************************************************************");
        HAPLog(&testFrameworkLogObject, " Note that failures are expected for test suite: %s", suiteName);
        HAPLog(&testFrameworkLogObject, " Ignore the error messages.");
        HAPLog(&testFrameworkLogObject,
               "*******************************************************************************");
    }

    bool badArgument = false;
    bool nextArgumentIsRepeatCount = false;
    uint32_t repeatCount = 1;

    // Run test one by one
    for (int i = 1; i < HAPMax(argc, 2); i++) {
        if (nextArgumentIsRepeatCount) {
            HAPError err = HAPUInt32FromString(argv[i], &repeatCount);
            if (err) {
                badArgument = true;
                break;
            }

            // Next argument is the test name to repeat
            nextArgumentIsRepeatCount = false;
            continue;
        }
        if (i < argc && HAPStringAreEqual(argv[i], "--repeat")) {
            // Next argument will be the repeat count.
            nextArgumentIsRepeatCount = true;
            continue;
        }
        if (i < argc && HAPRawBufferAreEqual(argv[i], "--repeat=", 9)) {
            HAPError err = HAPUInt32FromString(&argv[i][9], &repeatCount);
            if (err) {
                badArgument = true;
                break;
            }

            // Next argument is the test name to repeat
            continue;
        }

        bool testFound = false;
        for (auto testCase : testList) {
            if (argc < 2 || HAPStringAreEqual(testCase->GetName(), argv[i])) {
                testFound = true;
                for (uint32_t j = 0; j < repeatCount; j++) {
                    FailedTestResult result;
                    result.testCaseName = testCase->GetName();
                    result.failureMessage = NULL;
                    HAPLog(&testFrameworkLogObject, "Test Starts - %s", result.testCaseName);
                    bool succeeded = testCase->Execute(result.failureMessage);
                    HAPLog(&testFrameworkLogObject, "Test Ends - %s", result.testCaseName);
                    if (succeeded) {
                        passedTestCount++;
                        if (result.failureMessage) {
                            delete[] result.failureMessage;
                        }
                    } else {
                        failedTestResults.push_back(result);
                    }
                }
            }
        }
        repeatCount = 1;

        if (!testFound) {
            missingTestNames.push_back(argv[i]);
        }
    }

    if (badArgument) {
        HAPLog(&kHAPLog_Default,
               "Test inconclusive due to bad parameters\n"
               "Usage: <exe> [ <test_spec> ... ]\n"
               "   where <exe> is the test executable and\n"
               "         <test_spec> is [<repeat_option>] <test_name>.\n"
               "         <test_name> is the name of the test.\n"
               "         <repeat_option> is '--repeat <num>' or '--repeat=<num> to repeat the test <num> times.");
        return 1;
    }

    // Report final result
    HAPLog(&testReportLogObject, "=================================================================");
    HAPLog(&testReportLogObject, " Test Results : %s", suiteName);
    if (expectFailures) {
        HAPLog(&testReportLogObject, " Note that this test suite expected all tests to fail.");
    }
    HAPLog(&testReportLogObject, "-----------------------------------------------------------------");
    HAPLog(&testReportLogObject, "  Passed tests: %zu", expectFailures ? failedTestResults.size() : passedTestCount);
    HAPLog(&testReportLogObject, "  Failed tests: %zu", expectFailures ? passedTestCount : failedTestResults.size());
    HAPLog(&testReportLogObject, "  Missing tests: %zu", missingTestNames.size());

    // Print failed test list
    if (failedTestResults.size() > 0) {
        if (expectFailures) {
            // In case failures are expected, do not print failed tests
            // and just deallocate the failure messages.
            for (auto testResult : failedTestResults) {
                delete[] testResult.failureMessage;
                testResult.failureMessage = NULL;
            }
        } else {
            HAPLog(&testReportLogObject, "-----------------------------------------------------------------");
            HAPLog(&testReportLogObject, " Failed Tests");
            HAPLog(&testReportLogObject, "-----------------------------------------------------------------");
            for (auto testResult : failedTestResults) {
                HAPLog(&testReportLogObject, "    %s : %s", testResult.testCaseName, testResult.failureMessage);
                delete[] testResult.failureMessage;
                testResult.failureMessage = NULL;
            }
        }
    }

    // Print missing test list
    if (missingTestNames.size() > 0) {
        HAPLog(&testReportLogObject, "-----------------------------------------------------------------");
        HAPLog(&testReportLogObject, " Missing Tests");
        HAPLog(&testReportLogObject, "-----------------------------------------------------------------");
        for (auto testName : missingTestNames) {
            HAPLog(&testReportLogObject, "    %s", testName);
        }
    }

    HAPLog(&testReportLogObject, "-----------------------------------------------------------------");

    if (expectFailures) {
        return (passedTestCount > 0 || missingTestNames.size() > 0) ? 1 : 0;
    }
    return (failedTestResults.size() > 0 || missingTestNames.size() > 0) ? 1 : 0;
}

/** Internal macro to generate an error */
#ifdef __EXCEPTIONS
#define TEST_FAIL throw adk_unittest::TestFailure
#else
#define TEST_FAIL adk_unittest::testRegistry.LongJump
#endif

/** This is a function used by external test harness modules to report failures */
static void TestHarnessFailureFunction(const char* msg) {
    HAPLogError(&testFrameworkLogObject, "TestHarness failed: %s", msg);
    TEST_FAIL(msg);
}

/** asserts the predicate */
void TestAssert(bool p, const char* expr, const char* file, int line) {
    if (!p) {
        HAPLogError(&testFrameworkLogObject, "TEST_ASSERT(%s) failed @ %s:%d", expr, file, line);
        TEST_FAIL("TEST_ASSERT(%s) failed @ %s:%d", expr, file, line);
    }
}

/** Common body of TestAssertEqual for integral type or enum type */
template <typename T, typename U>
void _TestAssertEqualIntegralOrEnum(T a, U b, const char* aExpr, const char* bExpr, const char* file, int line) {
    if (a != b) {
        if (HAPMax(sizeof a, sizeof b) > sizeof(long)) {
            if (std::is_signed<T>::value) {
                HAPLogError(
                        &testFrameworkLogObject,
                        "TEST_ASSERT_EQUAL(%s, %s) failed: %lld != %lld @ %s:%d",
                        aExpr,
                        bExpr,
                        (long long) a,
                        (long long) b,
                        file,
                        line);
                TEST_FAIL(
                        "TEST_ASSERT_EQUAL(%s, %s) failed: %lld != %lld @ %s:%d",
                        aExpr,
                        bExpr,
                        (long long) a,
                        (long long) b,
                        file,
                        line);
            } else {
                // unsigned
                HAPLogError(
                        &testFrameworkLogObject,
                        "TEST_ASSERT_EQUAL(%s, %s) failed: %llu != %llu @ %s:%d",
                        aExpr,
                        bExpr,
                        (unsigned long long) a,
                        (unsigned long long) b,
                        file,
                        line);
                TEST_FAIL(
                        "TEST_ASSERT_EQUAL(%s, %s) failed: %llu != %llu @ %s:%d",
                        aExpr,
                        bExpr,
                        (unsigned long long) a,
                        (unsigned long long) b,
                        file,
                        line);
            }
        } else {
            if (std::is_signed<T>::value) {
                HAPLogError(
                        &testFrameworkLogObject,
                        "TEST_ASSERT_EQUAL(%s, %s) failed: %ld != %ld @ %s:%d",
                        aExpr,
                        bExpr,
                        (long) a,
                        (long) b,
                        file,
                        line);
                TEST_FAIL(
                        "TEST_ASSERT_EQUAL(%s, %s) failed: %ld != %ld @ %s:%d",
                        aExpr,
                        bExpr,
                        (long) a,
                        (long) b,
                        file,
                        line);
            } else {
                // unsigned
                HAPLogError(
                        &testFrameworkLogObject,
                        "TEST_ASSERT_EQUAL(%s, %s) failed: %lu != %lu @ %s:%d",
                        aExpr,
                        bExpr,
                        (unsigned long) a,
                        (unsigned long) b,
                        file,
                        line);
                TEST_FAIL(
                        "TEST_ASSERT_EQUAL(%s, %s) failed: %lu != %lu @ %s:%d",
                        aExpr,
                        bExpr,
                        (unsigned long) a,
                        (unsigned long) b,
                        file,
                        line);
            }
        }
    }
}

/** TestAssertEqual for integral types */
template <typename T, typename U>
void TestAssertEqual(
        T a,
        U b,
        const char* aExpr,
        const char* bExpr,
        const char* file,
        int line,
        typename std::enable_if<std::is_integral<T>::value>::type* _Nullable tDummy = 0,
        typename std::enable_if<std::is_integral<U>::value>::type* _Nullable uDummy = 0) {
    _TestAssertEqualIntegralOrEnum(a, b, aExpr, bExpr, file, line);
}

/** TestAssertEqual for enum types */
template <typename T, typename U>
void TestAssertEqual(
        T a,
        U b,
        const char* aExpr,
        const char* bExpr,
        const char* file,
        int line,
        typename std::enable_if<std::is_enum<T>::value>::type* _Nullable tDummy = 0,
        typename std::enable_if<std::is_enum<U>::value>::type* _Nullable uDummy = 0) {
    _TestAssertEqualIntegralOrEnum(a, b, aExpr, bExpr, file, line);
}

/** TestAssertEqual for integral type and enum type */
template <typename T, typename U>
void TestAssertEqual(
        T a,
        U b,
        const char* aExpr,
        const char* bExpr,
        const char* file,
        int line,
        typename std::enable_if<std::is_integral<T>::value>::type* _Nullable tDummy = 0,
        typename std::enable_if<std::is_enum<U>::value>::type* _Nullable uDummy = 0) {
    _TestAssertEqualIntegralOrEnum(a, b, aExpr, bExpr, file, line);
}

/** TestAssertEqual for enum type and integral type */
template <typename T, typename U>
void TestAssertEqual(
        T a,
        U b,
        const char* aExpr,
        const char* bExpr,
        const char* file,
        int line,
        typename std::enable_if<std::is_enum<T>::value>::type* _Nullable tDummy = 0,
        typename std::enable_if<std::is_integral<U>::value>::type* _Nullable uDummy = 0) {
    _TestAssertEqualIntegralOrEnum(a, b, aExpr, bExpr, file, line);
}

/** TestAssertEqual for floating point types */
template <typename T, typename U>
void TestAssertEqual(
        T a,
        U b,
        const char* aExpr,
        const char* bExpr,
        const char* file,
        int line,
        typename std::enable_if<std::is_floating_point<T>::value>::type* _Nullable tDummy = 0,
        typename std::enable_if<std::is_floating_point<U>::value>::type* _Nullable uDummy = 0) {
    if (a != b) {
        HAPLogError(
                &testFrameworkLogObject,
                "TEST_ASSERT_EQUAL(%s, %s) failed: %g != %g @ %s:%d",
                aExpr,
                bExpr,
                (double) a,
                (double) b,
                file,
                line);
        TEST_FAIL(
                "TEST_ASSERT_EQUAL(%s, %s) failed: %g != %g @ %s:%d", aExpr, bExpr, (double) a, (double) b, file, line);
    }
}

/** TestAssertEqual for pointer types, considered as strings */
template <typename T, typename U>
void TestAssertEqual(
        T a,
        U b,
        const char* aExpr,
        const char* bExpr,
        const char* file,
        int line,
        typename std::enable_if<std::is_pointer<T>::value>::type* _Nullable tDummy = 0,
        typename std::enable_if<std::is_pointer<U>::value>::type* _Nullable uDummy = 0) {
    if (!HAPStringAreEqual((const char*) a, (const char*) b)) {
        HAPLogError(
                &testFrameworkLogObject,
                "TEST_ASSERT_EQUAL(%s, %s) failed: \"%s\" <> \"%s\" @ %s:%d",
                aExpr,
                bExpr,
                (char*) a,
                (char*) b,
                file,
                line);
        TEST_FAIL(
                "TEST_ASSERT_EQUAL(%s, %s) failed: \"%s\" <> \"%s\" @ %s:%d",
                aExpr,
                bExpr,
                (char*) a,
                (char*) b,
                file,
                line);
    }
}

/** Common body of TestAssertNotEqual for integral type or enum type */
template <typename T, typename U>
void _TestAssertNotEqualIntegralOrEnum(T a, U b, const char* aExpr, const char* bExpr, const char* file, int line) {
    if (a == b) {
        if (HAPMax(sizeof a, sizeof b) > sizeof(long)) {
            HAPLogError(
                    &testFrameworkLogObject,
                    "TEST_ASSERT_NE(%s, %s) failed: %lld both @ %s:%d",
                    aExpr,
                    bExpr,
                    (long long) a,
                    file,
                    line);
            TEST_FAIL("TEST_ASSERT_NE(%s, %s) failed: %lld both @ %s:%d", aExpr, bExpr, (long long) a, file, line);
        } else {
            HAPLogError(
                    &testFrameworkLogObject,
                    "TEST_ASSERT_NE(%s, %s) failed: %ld both @ %s:%d",
                    aExpr,
                    bExpr,
                    (long) a,
                    file,
                    line);
            TEST_FAIL("TEST_ASSERT_NE(%s, %s) failed: %ld both @ %s:%d", aExpr, bExpr, (long) a, file, line);
        }
    }
}

/** TestAssertNotEqual for integral types */
template <typename T, typename U>
void TestAssertNotEqual(
        T a,
        U b,
        const char* aExpr,
        const char* bExpr,
        const char* file,
        int line,
        typename std::enable_if<std::is_integral<T>::value>::type* _Nullable tDummy = 0,
        typename std::enable_if<std::is_integral<U>::value>::type* _Nullable uDummy = 0) {
    _TestAssertNotEqualIntegralOrEnum(a, b, aExpr, bExpr, file, line);
}

/** TestAssertNotEqual for enum types */
template <typename T, typename U>
void TestAssertNotEqual(
        T a,
        U b,
        const char* aExpr,
        const char* bExpr,
        const char* file,
        int line,
        typename std::enable_if<std::is_enum<T>::value>::type* _Nullable tDummy = 0,
        typename std::enable_if<std::is_enum<U>::value>::type* _Nullable uDummy = 0) {
    _TestAssertNotEqualIntegralOrEnum(a, b, aExpr, bExpr, file, line);
}

/** TestAssertNotEqual for integral type and enum type */
template <typename T, typename U>
void TestAssertNotEqual(
        T a,
        U b,
        const char* aExpr,
        const char* bExpr,
        const char* file,
        int line,
        typename std::enable_if<std::is_integral<T>::value>::type* _Nullable tDummy = 0,
        typename std::enable_if<std::is_enum<U>::value>::type* _Nullable uDummy = 0) {
    _TestAssertNotEqualIntegralOrEnum(a, b, aExpr, bExpr, file, line);
}

/** TestAssertNotEqual for enum type and integral type */
template <typename T, typename U>
void TestAssertNotEqual(
        T a,
        U b,
        const char* aExpr,
        const char* bExpr,
        const char* file,
        int line,
        typename std::enable_if<std::is_enum<T>::value>::type* _Nullable tDummy = 0,
        typename std::enable_if<std::is_integral<U>::value>::type* _Nullable uDummy = 0) {
    _TestAssertNotEqualIntegralOrEnum(a, b, aExpr, bExpr, file, line);
}

/** TestAssertNotEqual for floating point types */
template <typename T, typename U>
void TestAssertNotEqual(
        T a,
        U b,
        const char* aExpr,
        const char* bExpr,
        const char* file,
        int line,
        typename std::enable_if<std::is_floating_point<T>::value>::type* _Nullable tDummy = 0,
        typename std::enable_if<std::is_floating_point<U>::value>::type* _Nullable uDummy = 0) {
    if (a == b) {
        HAPLogError(
                &testFrameworkLogObject,
                "TEST_ASSERT_NE(%s, %s) failed: %g both @ %s:%d",
                aExpr,
                bExpr,
                (double) a,
                file,
                line);
        TEST_FAIL("TEST_ASSERT_NE(%s, %s) failed: %g both @ %s:%d", aExpr, bExpr, (double) a, file, line);
    }
}

/** TestAssertNotEqual for pointer types, considered as strings */
template <typename T, typename U>
void TestAssertNotEqual(
        T a,
        U b,
        const char* aExpr,
        const char* bExpr,
        const char* file,
        int line,
        typename std::enable_if<std::is_pointer<T>::value>::type* _Nullable tDummy = 0,
        typename std::enable_if<std::is_pointer<U>::value>::type* _Nullable uDummy = 0) {
    if (HAPStringAreEqual((const char*) a, (const char*) b)) {
        HAPLogError(
                &testFrameworkLogObject,
                "TEST_ASSERT_NE(%s, %s) failed: \"%s\" both @ %s:%d",
                aExpr,
                bExpr,
                (char*) a,
                file,
                line);
        TEST_FAIL("TEST_ASSERT_NE(%s, %s) failed: \"%s\" both @ %s:%d", aExpr, bExpr, (char*) a, file, line);
    }
}

/** A single test setup method must be declared using the following macro */
#define TEST_SETUP() \
    static void TestSetupFunction(); \
    adk_unittest::TestSetup testSetup(TestSetupFunction); \
    static void TestSetupFunction()

/** A single test teardown method must be declared using the following macro */
#define TEST_TEARDOWN() \
    static void TestTeardownFunction(); \
    adk_unittest::TestTeardown testTeardown(TestTeardownFunction); \
    static void TestTeardownFunction()

/** Each test case function must be declared using the following macro */
#define TEST(_testName) \
    static void _testName(); \
    adk_unittest::Test testCase##_testName(#_testName, _testName); \
    static void _testName()

/** Execute all tests and generates report. This must be inserted into main(). */
#define EXECUTE_TESTS(_argc, _argv) adk_unittest::testRegistry.Execute(HAP_FILE, _argc, _argv)

/** Execute all tests, all of which are expected to fail. */
#define EXECUTE_TESTS_TO_FAIL(_argc, _argv) adk_unittest::testRegistry.Execute(HAP_FILE, _argc, _argv, true)

/** Assert equality to be used within a test */
#define TEST_ASSERT_EQUAL(a, b) adk_unittest::TestAssertEqual(a, b, #a, #b, HAP_FILE, __LINE__)

/** Assert inequality to be used within a test */
#define TEST_ASSERT_NE(a, b) adk_unittest::TestAssertNotEqual(a, b, #a, #b, HAP_FILE, __LINE__)

/** Assert true to be used within a test */
#define TEST_ASSERT(p) adk_unittest::TestAssert((bool) (p), #p, HAP_FILE, __LINE__)

} // adk_unittest

extern "C" {

#ifndef TEST_SUPPRESS_HAP_ASSERT_REMAP
// Note that it is dangerous to remap abort function to long jumps
// when HAP assertions can be called outside the thread where test framework runs.
// It could cause unexpected behaviors.
// In comparison when exceptions are supported, it would just cause the program to abort
// upon an unhandled exception.
// Hence, in such a platform where the HAP assertions could be called in a separate
// thread, define TEST_SUPPRESS_HAP_ASSERT_REMAP not to remap HAPPlatformAbort() function.
// It would sacrifice the ability to generate test reports when an HAP assertion fails
// in platforms that do not support exceptions, usually a platform where the exceptions are
// too costly.
void HAPPlatformAbort(void) {
    TEST_FAIL("Platform Aborted");
}
#endif

// Due to poisoning we cannot remap HAPAssert... functions in the header file.
// If we can organize the libraries that we can link with per test, the HAPAssert... functions
// might be remapped to generate test failures.

} // extern "C"

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#endif
