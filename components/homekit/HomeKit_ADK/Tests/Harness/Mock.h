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

#ifndef __MOCK_H_
#define __MOCK_H_

#include <stdlib.h>
#include <functional>
#include <vector>

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

extern "C" {

// Use HAPAssert by default for assertions
#ifndef MOCK_ASSERT
#define MOCK_ASSERT HAPAssert
#endif

static const HAPLogObject mockLogObject = { .subsystem = "com.apple.mfi.HomeKit.Core.Test", .category = "UnitTest" };
}

namespace adk_unittest {
/** Base class for mock function match entry which is the single unit of expected function call */
template <typename R, typename... Args> class MockMatchEntryBase {
protected:
    std::function<R(Args...)> mockFunction;
    std::vector<std::function<bool(Args...)>> argumentChecks;
    bool repeatCountIsSet;
    size_t repeatCount;
    bool atLeastIsSet;
    size_t atLeast;
    bool atMostIsSet;
    size_t atMost;
    size_t callCount;

    const char* fname;
    size_t linenum;

public:
    /**
     * Checks wether the argument values match the expectation set in this object.
     */
    bool Match(Args... args) {
        if (repeatCountIsSet && callCount >= repeatCount) {
            return false;
        }

        bool matched = (argumentChecks.size() == 0);
        for (auto& check : argumentChecks) {
            if (check(args...)) {
                matched = true;
                break;
            }
        }
        return matched;
    }

    /**
     * Retrieves mock function of action if any
     */
    std::function<R(Args...)> GetMockFunction() {
        callCount++;
        return mockFunction;
    }

    /**
     * Verifies that expectation set in this object is satisfied.
     */
    bool Verify() {
        if (atLeastIsSet && callCount < atLeast) {
            HAPLogError(
                    &mockLogObject,
                    "%s:%zu expected at least %zu calls but called only %zu times.",
                    fname,
                    linenum,
                    atLeast,
                    callCount);
            return false;
        }
        if (atMostIsSet && callCount > atMost) {
            HAPLogError(
                    &mockLogObject,
                    "%s:%zu expected at most %zu calls but called %zu times.",
                    fname,
                    linenum,
                    atMost,
                    callCount);
            return false;
        }
        return true;
    }

    /**
     * Adds an argument match function.
     *
     * Note that any of the argument match function added through this method returns true,
     * this object is supposed to match the argument.
     */
    MockMatchEntryBase& If(std::function<bool(Args...)> argumentCheck) {
        argumentChecks.push_back(argumentCheck);
        return *this;
    }

    /**
     * Sets a mock function.
     *
     * Note that this method would replace previously set mock function, if any.
     */
    MockMatchEntryBase& Do(std::function<R(Args...)> doFunction) {
        if (mockFunction) {
            HAPLogError(&mockLogObject, "%s:%zu redundant actions", fname, linenum);
        }
        mockFunction = doFunction;
        return *this;
    }

    /**
     * Sets the object to repeats matches of this object for the specified times.
     */
    MockMatchEntryBase& Repeats(size_t times) {
        if (repeatCountIsSet) {
            HAPLogError(&mockLogObject, "%s:%zu redundant Repeats()", fname, linenum);
        }
        repeatCount = times;
        repeatCountIsSet = true;
        return *this;
    }

    /**
     * Sets the expectation that the matches would occur at least the designated times.
     */
    MockMatchEntryBase& AtLeast(size_t times) {
        if (atLeastIsSet) {
            HAPLogError(&mockLogObject, "%s:%zu redundant AtLeast()", fname, linenum);
        }
        atLeast = times;
        atLeastIsSet = true;
        return *this;
    }

    /**
     * Sets the expectation that the matches would occur at most the designated times.
     */
    MockMatchEntryBase& AtMost(size_t times) {
        if (atMostIsSet) {
            HAPLogError(&mockLogObject, "%s:%zu redundant AtMost()", fname, linenum);
        }
        atMost = times;
        atMostIsSet = true;
        return *this;
    }

    /** constructor */
    MockMatchEntryBase(const char* fname, size_t linenum)
        : fname(fname)
        , linenum(linenum) {
        repeatCountIsSet = false;
        atLeastIsSet = false;
        atMostIsSet = false;
        callCount = 0;
    }
};

template <typename R, typename... Args> class MockMatchEntry;

/** Match entry for void returning mock functions */
template <typename... Args> class MockMatchEntry<void, Args...> : public MockMatchEntryBase<void, Args...> {
public:
    MockMatchEntry& If(std::function<bool(Args...)> argumentCheck) {
        return (MockMatchEntry&) MockMatchEntryBase<void, Args...>::If(argumentCheck);
    }

    MockMatchEntry& Do(std::function<void(Args...)> doFunction) {
        return (MockMatchEntry&) MockMatchEntryBase<void, Args...>::Do(doFunction);
    }

    MockMatchEntry& Repeats(size_t times) {
        return (MockMatchEntry&) MockMatchEntryBase<void, Args...>::Repeats(times);
    }

    MockMatchEntry& AtLeast(size_t times) {
        return (MockMatchEntry&) MockMatchEntryBase<void, Args...>::AtLeast(times);
    }

    MockMatchEntry& AtMost(size_t times) {
        return (MockMatchEntry&) MockMatchEntryBase<void, Args...>::AtMost(times);
    }

    MockMatchEntry(const char* fname, size_t linenum)
        : MockMatchEntryBase<void, Args...>(fname, linenum) {
    }
};

/** Match entry for non-void returning mock functions */
template <typename R, typename... Args> class MockMatchEntry : public MockMatchEntryBase<R, Args...> {
public:
    MockMatchEntry& If(std::function<bool(Args...)> argumentCheck) {
        return (MockMatchEntry&) MockMatchEntryBase<R, Args...>::If(argumentCheck);
    }

    MockMatchEntry& Do(std::function<R(Args...)> doFunction) {
        return (MockMatchEntry&) MockMatchEntryBase<R, Args...>::Do(doFunction);
    }

    MockMatchEntry& Repeats(size_t times) {
        return (MockMatchEntry&) MockMatchEntryBase<R, Args...>::Repeats(times);
    }

    MockMatchEntry& AtLeast(size_t times) {
        return (MockMatchEntry&) MockMatchEntryBase<R, Args...>::AtLeast(times);
    }

    MockMatchEntry& AtMost(size_t times) {
        return (MockMatchEntry&) MockMatchEntryBase<R, Args...>::AtMost(times);
    }

    MockMatchEntry(const char* fname, size_t linenum)
        : MockMatchEntryBase<R, Args...>(fname, linenum) {
    }

    MockMatchEntry& Return(R value) {
        if (MockMatchEntryBase<R, Args...>::mockFunction) {
            HAPLogError(
                    &mockLogObject,
                    "%s:%zu redundant actions",
                    MockMatchEntryBase<R, Args...>::fname,
                    MockMatchEntryBase<R, Args...>::linenum);
        }
        MockMatchEntryBase<R, Args...>::mockFunction = [=](Args...) { return value; };
        return *this;
    }

    MockMatchEntry& Return(std::function<R()> valueFunction) {
        if (MockMatchEntryBase<R, Args...>::mockFunction) {
            HAPLogError(
                    &mockLogObject,
                    "%s:%zu redundant actions",
                    MockMatchEntryBase<R, Args...>::fname,
                    MockMatchEntryBase<R, Args...>::linenum);
        }
        MockMatchEntryBase<R, Args...>::mockFunction = [=](Args...) { return valueFunction(); };
        return *this;
    }
};

/** Base class for storage of match entries for a specific function to be mocked */
class MockMatchEntryStorageBase {
public:
    /** Verifies the expectations of all match entries for the specific function to be mocked. */
    virtual bool Verify() = 0;
    /** Resets all expectations including always-expectations */
    virtual void Reset() = 0;
};

/** Class of storage object of match entries for a specific function to be mocked */
template <typename R, typename... Args> class MockMatchEntryStorage : public MockMatchEntryStorageBase {
private:
    std::vector<MockMatchEntry<R, Args...>*> storage;
    std::vector<MockMatchEntry<R, Args...>*> alwaysStorage;
    const char* name;

    /** Clears the match entry storage but not always-match entry storage */
    void Clear() {
        for (auto entry : storage) {
            delete entry;
        }
        storage.clear();
    }

public:
    /** Creates a new match entry that will last forever through the lifetime of this object */
    MockMatchEntry<R, Args...>& NewAlwaysEntry(const char* fname, size_t linenum) {
        auto entry = new MockMatchEntry<R, Args...>(fname, linenum);
        alwaysStorage.push_back(entry);
        return *entry;
    }

    /** Creates a new match entry that last till verified */
    MockMatchEntry<R, Args...>& NewEntry(const char* fname, size_t linenum) {
        auto entry = new MockMatchEntry<R, Args...>(fname, linenum);
        storage.push_back(entry);
        return *entry;
    }

    /**
     * Constructor
     *
     * @param name  function name to be used in error logs if any.
     */
    MockMatchEntryStorage(const char* name)
        : name(name) {
    }

    /** Destructor */
    ~MockMatchEntryStorage() {
        Clear();
        for (auto entry : alwaysStorage) {
            delete entry;
        }
        alwaysStorage.clear();
    }

    bool Verify() override {
        bool success = true;
        for (auto entry : storage) {
            if (!entry->Verify()) {
                HAPLogError(&mockLogObject, "<== for %s()", name);
                success = false;
            }
        }
        Clear();
        for (auto entry : alwaysStorage) {
            if (!entry->Verify()) {
                HAPLogError(&mockLogObject, "<== for %s()", name);
                success = false;
            }
        }
        return success;
    }

    /**
     * Reset all expectations including always-expectations.
     */
    void Reset() override {
        Clear();
        for (auto entry : alwaysStorage) {
            delete entry;
        }
        alwaysStorage.clear();
    }

    /**
     * Retrieve a argument-matching mock function among the stored match entries
     */
    std::function<R(Args...)> GetMatchingFunction(Args... args) {
        std::function<R(Args...)> mockFunction;
        for (auto entry : storage) {
            if (entry->Match(args...)) {
                auto mockFunction = entry->GetMockFunction();
                if (mockFunction) {
                    return mockFunction;
                }
                break;
            }
        }
        for (auto entry : alwaysStorage) {
            if (entry->Match(args...)) {
                auto mockFunction = entry->GetMockFunction();
                if (mockFunction) {
                    return mockFunction;
                }
                break;
            }
        }
        return NULL;
    }
};

#define MOCK_MATCH_ENTRY_STORAGE(_mock, _func) ((_mock).matchEntryStorages._func)

/** Mock object class */
template <typename MatchEntryStorages> class Mock {
public:
    /** Match entry storages */
    MatchEntryStorages matchEntryStorages;

private:
    /** Retrieves the nth match entry storage */
    MockMatchEntryStorageBase* GetMatchEntryStorage(size_t index) {
        MockMatchEntryStorage<void>* ptr = (MockMatchEntryStorage<void>*) &matchEntryStorages;
        return (MockMatchEntryStorageBase*) (ptr + index);
    }

public:
    /** Verifies all expectations */
    bool VerifyAll() {
        bool success = true;

        // Check all matches
        for (size_t i = 0; i < matchEntryStorageCount; i++) {
            if (!GetMatchEntryStorage(i)->Verify()) {
                success = false;
                // Note that we must not break here because all non-always match entries have to be cleared with verify
                // call.
            }
        }
        return success;
    }

    /** Resets all expectations including those set by ALWAYS() */
    void Reset() {
        for (size_t i = 0; i < matchEntryStorageCount; i++) {
            GetMatchEntryStorage(i)->Reset();
        }
    }

    /**
     * Constructor
     *
     * @param globalMockPtr   a global pointer to a mock object to be used in stub C functions.
     *                        The constructor updates the pointer value to dereference this object.
     */
    Mock(Mock* _Nullable* _Nonnull globalMockPtr)
        : globalMockPtr(globalMockPtr) {
        originalMock = *globalMockPtr;
        *globalMockPtr = this;

        static_assert(
                sizeof matchEntryStorages % sizeof(MockMatchEntryStorage<void>) == 0,
                "Assumption about the size of matchEntryStorages was wrong");

        matchEntryStorageCount = sizeof matchEntryStorages / sizeof(MockMatchEntryStorage<void>);
    }

    /**
     * Destructor
     *
     * Note that destructor restores the original value of the global mock object pointer.
     */
    ~Mock() {
        MOCK_ASSERT(VerifyAll());
        *globalMockPtr = originalMock;
    }

private:
    Mock* _Nullable* _Nonnull globalMockPtr;
    Mock* _Nullable originalMock;
    size_t matchEntryStorageCount;
};

// Macros to use with test

/** Create a mock function to stay as far as the mock object is alive */
#define ALWAYS(_mock, _func) MOCK_MATCH_ENTRY_STORAGE(_mock, _func).NewAlwaysEntry(HAP_FILE, __LINE__)

/** Create a mock function that will stay till verified */
#define EXPECT(_mock, _func) MOCK_MATCH_ENTRY_STORAGE(_mock, _func).NewEntry(HAP_FILE, __LINE__)

/**
 * Verify mock expected behavior on specific function.
 * This will end that mock function except those created with ALWAYS().
 */
#define VERIFY(_mock, _func) MOCK_ASSERT(MOCK_MATCH_ENTRY_STORAGE(_mock, _func).Verify())

/**
 * Verify mock expected behavior on specific function without assertion.
 * Return value will be true or false.
 *
 * This will end that mock function except those created with ALWAYS().
 */
#define VERIFY_NO_ASSERT(_mock, _func) MOCK_MATCH_ENTRY_STORAGE(_mock, _func).Verify()

/**
 * Verify all mock behaviors.
 * This will end all mock behaviors except those created with ALWAYS().
 */
#define VERIFY_ALL(_mock) MOCK_ASSERT((_mock).VerifyAll())

} // adk_unittest

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#endif // __MOCK_H_
