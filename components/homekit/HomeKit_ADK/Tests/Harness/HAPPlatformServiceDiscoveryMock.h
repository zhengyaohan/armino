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

#ifndef _HAP_PLATFORM_SERVICEDISCOVERY_MOCK_H_
#define _HAP_PLATFORM_SERVICEDISCOVERY_MOCK_H_

#include "Mock.h"
#include "MockMacros.h"

#include "HAPPlatformServiceDiscoveryHelper.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

extern "C" {

struct HAPPlatformServiceDiscovery {
    // Empty
    uint8_t _;
};
}

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

// All mock functions must be defined here with list of MOCK_FUNC_DEF(return-type, func-name, arguments,
// default-return-value) where arguments should be either MOCK_VOID_ARG, MOCK_NO_ARGS or MOCK_ARGS(MOCK_ARG(type, name),
// ...).
//
// For void returning functions, MOCK_VOID_FUNC_DEF(func-name, arguments) must be used
#define MOCK_FUNCS \
    MOCK_VOID_FUNC_DEF( \
            HAPPlatformServiceDiscoveryRegister, \
            MOCK_ARGS( \
                    MOCK_ARG(HAPPlatformServiceDiscoveryRef, serviceDiscovery), \
                    MOCK_ARG(const char*, name), \
                    MOCK_ARG(const char*, protocol), \
                    MOCK_ARG(HAPNetworkPort, port), \
                    MOCK_ARG(HAPPlatformServiceDiscoveryTXTRecord*, txtRecords), \
                    MOCK_ARG(size_t, numTXTRecords))) \
    MOCK_VOID_FUNC_DEF( \
            HAPPlatformServiceDiscoveryUpdateTXTRecords, \
            MOCK_ARGS( \
                    MOCK_ARG(HAPPlatformServiceDiscoveryRef, serviceDiscovery), \
                    MOCK_ARG(HAPPlatformServiceDiscoveryTXTRecord*, txtRecords), \
                    MOCK_ARG(size_t, numTXTRecords))) \
    MOCK_VOID_FUNC_DEF( \
            HAPPlatformServiceDiscoveryRegisterReplyCallback, \
            MOCK_ARGS(MOCK_ARG(ServiceDiscoveryCallback, callback), MOCK_ARG(void*, context))) \
    MOCK_VOID_FUNC_DEF( \
            HAPPlatformServiceDiscoveryStop, MOCK_ARGS(MOCK_ARG(HAPPlatformServiceDiscoveryRef, serviceDiscovery))) \
    MOCK_FUNC_DEF( \
            bool, \
            HAPPlatformServiceDiscoveryIsAdvertising, \
            MOCK_ARGS(MOCK_ARG(HAPPlatformServiceDiscoveryRef, serviceDiscovery)), \
            false) \
    MOCK_FUNC_DEF( \
            const char*, \
            HAPPlatformServiceDiscoveryGetName, \
            MOCK_ARGS(MOCK_ARG(HAPPlatformServiceDiscoveryRef, serviceDiscovery)), \
            "") \
    MOCK_FUNC_DEF( \
            const char*, \
            HAPPlatformServiceDiscoveryGetProtocol, \
            MOCK_ARGS(MOCK_ARG(HAPPlatformServiceDiscoveryRef, serviceDiscovery)), \
            "") \
    MOCK_FUNC_DEF( \
            HAPNetworkPort, \
            HAPPlatformServiceDiscoveryGetPort, \
            MOCK_ARGS(MOCK_ARG(HAPPlatformServiceDiscoveryRef, serviceDiscovery)), \
            0) \
    MOCK_VOID_FUNC_DEF( \
            HAPPlatformServiceDiscoveryEnumerateTXTRecords, \
            MOCK_ARGS( \
                    MOCK_ARG(HAPPlatformServiceDiscoveryRef, serviceDiscovery), \
                    MOCK_ARG(HAPPlatformServiceDiscoveryEnumerateTXTRecordsCallback, callback), \
                    MOCK_ARG(void* _Nullable, context))) \
    MOCK_VOID_FUNC_DEF( \
            HAPPlatformServiceDiscoveryCreate, MOCK_ARGS(MOCK_ARG(HAPPlatformServiceDiscoveryRef, serviceDiscovery)))

/**
 * Data structure to keep all match entries of all mock function
 *
 * When you create a new mock type, a structure like the following has to be written with a new name.
 */
struct PalServiceDiscoveryMockMatchEntryStorages {
    // Declare match entry storages
    //
    // Including the following header file will expand MOCK_FUNCS macro into
    // match entry storage fields to be included in this structure.
#include "MockExpandCppMatchEntryStorages.hpp"
};

/**
 * Mock type.
 *
 * If you want to create your own mock type, use a unique typename.
 * The template must be specialized with your own struct type which includes all
 * match entries of the mock functions as declared above.
 */
typedef adk_unittest::Mock<PalServiceDiscoveryMockMatchEntryStorages> PalServiceDiscoveryMock;

/**
 * Global mock pointer to be used by mocked C functions.
 *
 * The name has to be unique per mock type.
 */
PalServiceDiscoveryMock* _Nullable globalPalServiceDiscoveryMock;

// Expand mock function definitions into C functions
//
// Including MockExpandFuncsCallingCppMock.h header file
// would automatically expand MOCK_FUNCS macro into C function
// declarations that uses CPP_MOCK_POINTER as pointer to the
// mock instance.
// The expanded C functions route their calls to the mock object
// dereferenced by the CPP_MOCK_POINTER.
#define CPP_MOCK_POINTER globalPalServiceDiscoveryMock
#include "MockExpandFuncsCallingCppMock.h"
#undef CPP_MOCK_POINTER

// Undefine MOCK_FUNCS so that other modules can be mocked.
#undef MOCK_FUNCS

// Macro to declare a PAL mock instance
#define HAP_PLATFORM_SERVICEDISCOVERY_MOCK(_mock) PalServiceDiscoveryMock _mock(&globalPalServiceDiscoveryMock)

#endif // _HAP_PLATFORM_SERVICEDISCOVERY_MOCK_H_
