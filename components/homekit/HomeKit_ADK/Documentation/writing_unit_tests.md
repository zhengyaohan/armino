Writing Unit Tests
==================

## Overview

*Tests/Harness* includes C++ header files as unit test framework, such as follows:

* *UnitTestFramework.h* provides macros to organize tests in a test file to generate a test report in a common format.
* *HAPPlatform...Mock.h* files provide mock utilities to use mock functions and to modify the mock function behaviors.

This document explains how to write ADK unit tests using those header files.

### Header files

An ADK unit test must be created as a C++ source file with `.cpp` extension in the `Tests` directory.
Include `Harness/UnitTestFramework.h` file after including relevant HAP and HAP Platform includes,
such as `HAPPlatform+Init.h` and `HAP+API.h`.

In addition, if a mock framework is used, define its assertion macro `MOCK_ASSERT` to use test assertion macro
`TEST_ASSERT` provided by the unit test framework, as in the following example:

```c
#include "HAP+API.h"

#include "Harness/UnitTestFramework.h"

// Use test framework assert macro for mock framework asserts
#define MOCK_ASSERT TEST_ASSERT

#include "Harness/HAPPlatformBLEMock.h"
```

### Test case declaration

A test file, which can be called a test suite, may contain multiple test cases.
Each of your test case function must be declared as follows:

```c
TEST(MyTestCaseName) {
    ....
}
```

### Main function

Your main function must include *EXECUTE_TESTS(argc, (const char**) argv)* call.
And the return value from the macro should be used to return from *main()*, for example, as follows:

```c
int main(int argc, char** argv) {
    HAPPlatformCreate();

    return EXECUTE_TESTS(argc, (const char**) argv);
}
```

*Note*: Typecast of `(const char**)` is required because C++ is strict on the type qualifier *const* as well.

### Test setup and teardown

If you have either a common setup procedure or a common teardown procedure or both for all test cases
in your test file, you can declare `TEST_SETUP() { ... }` or `TEST_TEARDOWN() { ... }` or both.

For example,

```c
TEST_SETUP() {
    // Clear all pairings
    HAPError err = HAPPlatformKeyValueStorePurgeDomain(platform.keyValueStore, kHAPKeyValueStoreDomain_Pairings);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
}

TEST_TEARDOWN() {
    // Clears all timers, hopefully, by advancing an hour.
    HAPPlatformClockAdvance(HAPMinute * 60);
}
```

### Test assertion macros

The test framework provides a few assert macros such as:
* `TEST_ASSERT(expression)` asserts that the *expression* is true.
* `TEST_ASSERT_EQUAL(expression1, expression2)` asserts that *expression1* is equal to *expression2*.
* `TEST_ASSERT_NE(expression1, expression2)` asserts that *expression1* does not equal *expression2*.
* `TEST_ASSERT_EQUAL()` and `TEST_ASSERT_NE()` macros support integer types such as `int64_t`, `float` and `double`
and null-terminated strings. All pointers will be considered as null-terminated strings.

*NOTE:* `HAPAssert()` macro can still be used but in some condition it may fail the entire test suite without generating
report. Hence, it is recommended to use above macros in your test functions.

## Running tests

### Running a subset of tests

You can pass each test names within `TEST()` parentheses as arguments to the test executable
in order to test a subset of test, e.g.,

```sh
Output/Darwin-x86_64-apple-darwin19.3.0/Test/Tests/UnitTestAssertMacrosFailureTest.OpenSSL TestHapAssert TestAssertEqualDouble
```

The above will only execute the two tests: `TestHapAssert` and `TestAssertEqualDouble`.

### Test report

At the end of a test, you will get a report like the following:

```sh
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report] =======================================================
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report]  Test Results : Tests/MockTest.cpp
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report] -------------------------------------------------------
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report]   Passed tests: 2
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report]   Failed tests: 0
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report]   Missing tests: 0
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report] -------------------------------------------------------

```

It reports number of passed tests, number of failed tests and number of missing tests. Missing tests are tests which
cannot be found and they would occur only when test names are passed when executing the test executable, e.g.,

```sh
$ Output/Darwin-x86_64-apple-darwin19.3.0/Test/Tests/MockTest.OpenSSL BadTestName
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report] =======================================================
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report]  Test Results : Tests/MockTest.cpp
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report] -------------------------------------------------------
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report]   Passed tests: 0
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report]   Failed tests: 0
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report]   Missing tests: 1
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report] -------------------------------------------------------
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report]  Missing Tests
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report] -------------------------------------------------------
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report]     BadTestName
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report] -------------------------------------------------------
```

When test fails, the failure messages are printed within the report, as follows:

```sh
$ Output/Darwin-x86_64-apple-darwin19.3.0/Test/Tests/UnitTestAssertMacrosFailureTest.OpenSSL
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:UnitTest] *******************************************************************************
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:UnitTest]  Note that failures are expected for test suite: Tests/UnitTestAssertMacrosFailureTest.cpp
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:UnitTest]  Ignore the error messages.
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:UnitTest] *******************************************************************************
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:UnitTest] Test Starts - TestHapAssert
--------------------------------------------------------------------------------
0.000	Fault	precondition failed: byteIndex < numBytes - HAPBitSetContains
--------------------------------------------------------------------------------
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:UnitTest] Test Ends - TestHapAssert
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:UnitTest] Test Starts - TestAssert
0.000	Error	[com.apple.mfi.HomeKit.Core.Test:UnitTest] TEST_ASSERT(false) failed @ Tests/UnitTestAssertMacrosFailureTest.cpp:72
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:UnitTest] Test Ends - TestAssert
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:UnitTest] Test Starts - TestAssertEqualLong
0.000	Error	[com.apple.mfi.HomeKit.Core.Test:UnitTest] TEST_ASSERT_EQUAL(1, 2) failed: 1 != 2 @ Tests/UnitTestAssertMacrosFailureTest.cpp:77
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:UnitTest] Test Ends - TestAssertEqualLong
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:UnitTest] Test Starts - TestAssertEqualDouble
0.000	Error	[com.apple.mfi.HomeKit.Core.Test:UnitTest] TEST_ASSERT_EQUAL(1.0, 2.0) failed: 1 != 2 @ Tests/UnitTestAssertMacrosFailureTest.cpp:82
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:UnitTest] Test Ends - TestAssertEqualDouble
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:UnitTest] Test Starts - TestAssertEqualString
0.000	Error	[com.apple.mfi.HomeKit.Core.Test:UnitTest] TEST_ASSERT_EQUAL("Foo", "Bar") failed: "Foo" <> "Bar" @ Tests/UnitTestAssertMacrosFailureTest.cpp:87
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:UnitTest] Test Ends - TestAssertEqualString
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:UnitTest] Test Starts - TestAssertNotEqualLong
0.000	Error	[com.apple.mfi.HomeKit.Core.Test:UnitTest] TEST_ASSERT_NE(1, 1) failed: 1 both @ Tests/UnitTestAssertMacrosFailureTest.cpp:92
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:UnitTest] Test Ends - TestAssertNotEqualLong
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:UnitTest] Test Starts - TestAssertNotEqualDouble
0.000	Error	[com.apple.mfi.HomeKit.Core.Test:UnitTest] TEST_ASSERT_NE(2.0, 2.0) failed: 2 both @ Tests/UnitTestAssertMacrosFailureTest.cpp:97
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:UnitTest] Test Ends - TestAssertNotEqualDouble
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:UnitTest] Test Starts - TestAssertNotEqualString
0.000	Error	[com.apple.mfi.HomeKit.Core.Test:UnitTest] TEST_ASSERT_NE("Foo", "Foo") failed: "Foo" both @ Tests/UnitTestAssertMacrosFailureTest.cpp:102
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:UnitTest] Test Ends - TestAssertNotEqualString
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:UnitTest] Test Starts - TestAssertWithinMock
0.000	Error	[com.apple.mfi.HomeKit.Core.Test:UnitTest] TEST_ASSERT(!failIt) failed @ Tests/UnitTestAssertMacrosFailureTest.cpp:111
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:UnitTest] Test Ends - TestAssertWithinMock
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:UnitTest] Test Starts - TestMockVerification
0.000	Error	[com.apple.mfi.HomeKit.Core.Test:UnitTest] Tests/UnitTestAssertMacrosFailureTest.cpp:126 expected at least 1 calls but called only 0 times.
0.000	Error	[com.apple.mfi.HomeKit.Core.Test:UnitTest] <== for HAPPlatformServiceDiscoveryGetPort()
0.000	Error	[com.apple.mfi.HomeKit.Core.Test:UnitTest] TEST_ASSERT((mock).VerifyAll()) failed @ Tests/UnitTestAssertMacrosFailureTest.cpp:129
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:UnitTest] Test Ends - TestMockVerification
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report] =================================================================
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report]  Test Results : Tests/UnitTestAssertMacrosFailureTest.cpp
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report]  Note that this test suite expected all tests to fail.
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report] -----------------------------------------------------------------
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report]   Passed tests: 0
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report]   Failed tests: 10
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report]   Missing tests: 0
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report] -----------------------------------------------------------------
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report]  Failed Tests
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report] -----------------------------------------------------------------
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report]     TestHapAssert : Platform Aborted
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report]     TestAssert : TEST_ASSERT(false) failed @ Tests/UnitTestAssertMacrosFailureTest.cpp:72
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report]     TestAssertEqualLong : TEST_ASSERT_EQUAL(1, 2) failed: 1 != 2 @ Tests/UnitTestAssertMacrosFailureTest.cpp:77
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report]     TestAssertEqualDouble : TEST_ASSERT_EQUAL(1.0, 2.0) failed: 1 != 2 @ Tests/UnitTestAssertMacrosFailureTest.cpp:82
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report]     TestAssertEqualString : TEST_ASSERT_EQUAL("Foo", "Bar") failed: "Foo" <> "Bar" @ Tests/UnitTestAssertMacrosFailureTest.cpp:87
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report]     TestAssertNotEqualLong : TEST_ASSERT_NE(1, 1) failed: 1 both @ Tests/UnitTestAssertMacrosFailureTest.cpp:92
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report]     TestAssertNotEqualDouble : TEST_ASSERT_NE(2.0, 2.0) failed: 2 both @ Tests/UnitTestAssertMacrosFailureTest.cpp:97
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report]     TestAssertNotEqualString : TEST_ASSERT_NE("Foo", "Foo") failed: "Foo" both @ Tests/UnitTestAssertMacrosFailureTest.cpp:102
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report]     TestAssertWithinMock : TEST_ASSERT(!failIt) failed @ Tests/UnitTestAssertMacrosFailureTest.cpp:111
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report]     TestMockVerification : TEST_ASSERT((mock).VerifyAll()) failed @ Tests/UnitTestAssertMacrosFailureTest.cpp:129
0.000	Default	[com.apple.mfi.HomeKit.Core.Test:Report] -----------------------------------------------------------------
```

Each failed test start with the test case name and the actual failure message separated by a colon.
When you get a *Platform Aborted* message, you will have to look at the test log to find out where the problem occurred.
You'll have to look at the test logs for mock verification failure as well as it provides the line where the expectation was set.

### Suppressing HAPPlatformAbort() remapping

Test framework re-implements `HAPPlatformAbort()` function to be able to generate a test report at the end.
The function is implemented using C++ exceptions for platforms that support C++ exceptions. However, there are platforms
that do not support C++ exceptions, most likely because of the cost of exceptions. In such a case, the function is
implemented with long jumps. Long jump behavior is undefined if the long jump occurs across threads.
Hence, it is unsafe to keep the `HAPPlatformAbort()` implementation if you expect that some assertions could be called
from within functions which are not running on the same thread as the test suite itself.

Note that in general, the test framework doesn't support thread-safety, which means that tests and test assertion macros
must be used on the same thread. That said, the intention wasn't to prevent `HAPAssert()` from being called by another
thread.

## Using mocks

### Preparing a mock framework

`Tests/Harness/HAPPlatform...Mock.h` files are already configured for certain PAL functions
but not all functions yet. In order to mock a new set of functions, use one of those header files as a template
and create a new header file if necessary.

Mock function must be declared as part of `MOCK_FUNCS` macro in this file with spaces
between them. Note that function declarations must not be separated with a comma or any
other symbols.
Each mock function is declared with either of the following formats in `MOCK_FUNCS`:

* `MOCK_FUNC_DEF(return_type, func_name, arguments, default_return_value)`
  * `return_type` shall be the type of the return value of the function
  * `func_name` is the name of the function
  * `default_return_value` is the default return value the mock function should return when the return value is not set during testing.
  * `arguments` must be in one of the following formats:
    * `MOCK_VOID_FUNC_DEF(func_name, arguments)`
    * `MOCK_ARGS(MOCK_ARG(argument_type, argument_name), ...)`
      * `argument_type` is the type of the argument
      * `argument_name` is the name of the argument
    * `MOCK_VOID_ARG` is used if the argument type is void
    * `MOCK_NO_ARGS` is used when there are not arguments

Note that since mock function mock a C function there is actually no difference between `MOCK_VOID_ARG` and `MOCK_NO_ARGS`.
Now, let's look at a hypothetical example function you want to mock:

```c
HAPError HAPPlatformSetGPIOMode(HAPPlatformGPIOPin* pin, HAPPlatformGPIOMode mode);
```

The mock declaration should look as follows:

```c
MOCK_FUNC_DEF(HAPError, HAPPlatformSetGPIOMode,
    MOCK_ARGS(
        MOCK_ARG(HAPPlatformGPIOPin*, pin),
        MOCK_ARG(HAPPlatformGPIOMode, mode)),
    kHAPError_None)
```

Let's look at another example:

```c
void HAPPlatformSetGPIOValue(HAPPlatformGPIOPin* pin, uint8_t value);
```

Its mock declaration should look as follows:

```c
MOCK_VOID_FUNC_DEF(HAPPlatformSetGPIOValue,
    MOCK_ARGS(
        MOCK_ARG(HAPPlatformGPIOPin* pin),
        MOCK_ARG(uint8_t, value)))
```

When adding a function to mock, the function could be added to the existing `MOCK_FUNCS` definition in the `Tests/Harness/HAPPlatformBLEMock.h` or a new header file will be created based on the `Tests/Harness/HAPPlatformBLEMock.h` template.
In case of the latter, make sure that the following names must be renamed to your unique name.

* `HAPPlatformBLEMockMatchEntryStorages`
* `HAPPlatformBLEMock`
* `globalHAPPlatformBLEMock`
* `HAP_PLATFORM_BLE_MOCK`

### Using mocks in the tests

This section describes how to use mocks in each test.

- Test file must be C++ file with an extension of `.cpp`.
- The mock header file must be included in the test after including all relevant HAP and HAP Platform headers.

If `Tests/Harness/HAPPlatformBLEMock.h` is used, it would look like the following:

```c
#include "HAPPlatform+Init.h"

#include "Harness/HAPPlatformBLEMock.h`
```

#### Declaring mock instance

Mock instance will be created with a macro provided in your mock header file.
For example, in case of `Tests/Harness/HAPPlatformBLEMock.h`, the following macro will declare
a mock instance with the name `mock`.

```c
HAP_PLATFORM_BLE_MOCK(mock);
```

Normally you will use one mock instance of a mock type for your test.
However, if you create another instance of the same mock type, it will overwrite
all the previous mock behaviors until the newly created instance is destroyed.

Now you can pass the `mock` variable as a mock instance to one of the following macros
to modify the mock function behavior and to verify the mock function calls.

* `ALWAYS(mock_instance, function_name)`- Sets up an expectation that remains valid as far as mock instance is alive.
* `EXPECT(mock_instance, function_name)`- Sets up an expectation that remains valid until the expectation is verified.
* `VERIFY(mock_instance, function_name)`- Verifies all expectations associated with the function
* `VERIFY_ALL(mock_instance)`- Verifies all expectations associated with the mock instance.

Each macro usage is described in subsequent sections.

#### Setting up expectations

Either `ALWAYS()` macro or `EXPECT()` macro is used to set up expectation of a mock function call. Use `EXPECT()`
when trying to verify calls to the mock functions up to a certain point in the test. Use `ALWAYS()` in case you are
defining a mock function behavior throughout your test. The `EXPECT()` or `ALWAYS()` macros alone don't do anything.
Either macro should be followed by method calls, i.e., *.MethodName()*, to define mock behavior or the mock function
call expectations.

The following methods are provided:
* `Return(T value)`: This method changes mock function behavior to return the designated value instead of
the default value setup in your mock header file. Note that this method is absent from void returning functions.
* `Repeats(size_t n)`: This method constrains the mock function behaviors or the expectations to be valid
up to n calls to the mock function.
* `AtMost(size_t n)`: This method sets expectation that the mock function is called at most n times till the mock
function is verified. Only the calls within the constraint set by `Repeats()` or `If()` or both are counted.
* `AtLeast(size_t n)`: This method sets expectation that the mock function is called at least n times till the mock
function is verified. Only the calls within the constraint set by `Repeats()` or `If()` or both are counted.
* `If(lambda_function)`: This method constrains the scope of expectation to the mock function calls with arguments that
will result in return value of true of the `lambda_function`. `lambda_function` takes the mock function call arguments
as its argument and returns either true or false.
* `Do(lambda_function)`: This method changes mock function behavior to call the `lambda_function` with the mock function
arguments and to return the exact value `lambda_function` returns. Practically, the `lambda_function` replaces the mock
function. However, note that the `lambda_function` is triggered only within the constraints that are specified by either
`Repeats()` or `If()` or both method calls. That is, if expectation is configured to repeat only 1 time, the
`lambda_function` will be called only one time.

The following sections describe in more detail how to use the macro and its methods.

#### Return value overriding

Let's use the example of the hypothetical GPIO PAL functions above. If you want to override the return value of a
function, you can do the following:

```c
HAP_PLATFORM_GPIO_MOCK(mock);

EXPECT(mock, HAPPlatformSetGPIOMode)
    .Return(kHAPError_Unknown);
```

The above will make `HAPPlatformSetupGPIOMode()` function to return `kHAPPError_Unknown` until a verification
is done against that function. That is, until one of the following events:
* `VERIFY(mock, HAPPlatformSetGPIOMode)` is called.
* `VERIFY_ALL(mock)` is called.
* `mock` goes out of scope.

#### Limiting the behavior changes to a certain number of calls.

```c
HAP_PLATFORM_GPIO_MOCK(mock);

EXPECT(mock, HAPPlatformSetGPIOMode)
    .Return(kHAPError_Unknown);
    .Repeats(4);
EXPECT(mock, HAPPlatformSetGPIOMode)
    .Return(kHAPError_InvalidData);
    .Repeats(2);
```

The above configuration will make `HAPPlatformSetGPIOMode()` to return `kHAPError_Unknown` four times
and then return `kHAPError_InvalidData` two times. Afterwards, the function will return default return value.

Note that if the mock function is verified before the designated number of calls were made, the configured number of
calls do not affect behaviors afterwards. That is, after verification, verified expectations are removed and hence they
are no longer effective.

#### Verifying number of calls to the mock function.
If you want to verify that a mock function was called certain number of times, you can use `AtMost()` and `AtLeast()` methods.

```c
HAP_PLATFORM_GPIO_MOCK(mock);

EXPECT(mock, HAPPlatformSetGPIOMode)
    .AtMost(1)
    .AtLeast(1);

...

VERIFY_ALL(mock);
```

The above verifies that there is exactly one call to `HAPPlatformSetGPIOMode()` in the *...* region.

#### Argument match

Each expectation can be narrowly limited to certain argument values of mock function call.

```c
HAP_PLATFORM_GPIO_MOCK(mock);

EXPECT(mock, HAPPlatformSetGPIOMode)
    .If([](HAPPlatformGPIOPin* pin, HAPPlatformGPIOMode mode) { return pin->index == 4; })
    .Return(kHAPError_None)
    .AtLeast(1);
EXPECT(mock, HAPPlatformSetGPIOMode)
    .Return(kHAPError_InvalidData)
    .AtMost(0);
...
VERIFY_ALL(mock);
```

The above makes `HAPPlatformSetGPIOMOde()` to return `kHAPError_None` when pin index is 4 and otherwise,
`kHAPError_InvalidData`. It also sets up expectation that there has to be at least one call to `HAPPlatformSetGPIOMode()`
with the pin index 4 while no calls with other values are expected. Argument matching is evaluated in the order of macro calls.

Note that the `If` conditions also constrain the number of calls set up with `Repeats()`. That is, call to a mock
function with argument values that doesn't match the `If` condition, will not be counted against the number of times
set up via `Repeats()` call.

#### ALWAYS()
Once the expectations are verified, the expectations and the mock function behaviors set up with `EXPECT()` are cleared.
Hence, it is important to set up expectations after verification.

However, you may want the same behavior of a mock function across the test and may not want to rewrite the expected
behavior of the mock function repeatedly after multiple verification points. In that case, you can use `ALWAYS()` macro
instead which remains valid till the mock instance itself is destroyed.

```c
HAP_PLATFORM_GPIO_MOCK(mock);

// Sets up the return values of HAPPlatformSetGPIOMode() which are applicable throughout the test
ALWAYS(mock, HAPPlatformSetGPIOMode)
    .If([](HAPPlatformGPIOPin* pin, HAPPlatformGPIOMode mode) { return pin->index == 4; })
    .Return(kHAPError_None);
ALWAYS(mock, HAPPlatformSetGPIOMode)
    .Return(kHAPError_InvalidData).
    .AtMost(0); // No pin-index != 4 call is expected throughout the test

// Until next verification, the following call is expected
EXPECT(mock, HAPPlatformSetGPIOMode)
    .If([](HAPPlatformGPIOPin* pin, HAPPlatformGPIOMode mode) { return mode == kHAPPlatformGPIOMode_Output; })
    .AtLeast(1);
...
VERIFY_ALL(mock);

// Until next verification, the following call is not expected
EXPECT(mock, HAPPlatformSetGPIOMode)
    .AtMost(0);
...
VERIFY_ALL(mock);

// Until next verification, the following call is expected
EXPECT(mock, HAPPlatformSetGPIOMode)
    .If([](HAPPlatformGPIOPin* pin, HAPPlatformGPIOMode mode) { return mode == kHAPPlatformGPIOMode_Input; })
    .AtLeast(1);
...
VERIFY_ALL(mock);
```

#### Global mock objects

If you want to set up mocks in the same way for all your tests,
you should declare mock objects as global variables outside `TEST()` function and
configure such global mock objects in `TEST_SETUP()` function.

In such a case, make sure you call `Reset()` method for each global mock objects
in `TEST_TEARDOWN()`, in order to reset the internal states of all such mock objects.
Otherwise, one test would be affected by expectations set by another test.

The following is an example:

```c
HAP_PLATFORM_GPIO_MOCK(mock);

TEST_SETUP() {
    ALWAYS(mock, HAPPlatformSetGPIOMode)
        .If([](HAPPlatformGPIOPin* pin, HAPPlatformGPIOMode mode) { return pin->index == 4; })
        .Return(kHAPError_None);
}

TEST_TEARDOWN() {
    // The following resets all expectations.
    mock.Reset();
}
```
