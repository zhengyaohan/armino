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

#include "HAP+API.h"
#include "HAPLogSubsystem.h"

void HAPPlatformThreadInitialize(
        void* server HAP_UNUSED,
        HAPPlatformThreadDeviceCapabilities deviceType HAP_UNUSED,
        uint32_t pollPeriod HAP_UNUSED,
        uint32_t childTimeout HAP_UNUSED,
        uint8_t txPower HAP_UNUSED) {
    HAPLog(&kHAPLog_Default, "HAPPlatformThreadInitialize() called");
}

void HAPPlatformThreadDeinitialize(void* server HAP_UNUSED) {
    HAPLog(&kHAPLog_Default, "HAPPlatformThreadDeinitialize() called");
}

bool HAPPlatformThreadIsCommissioned(void) {
    HAPLog(&kHAPLog_Default, "HAPPlatformThreadIsCommissioned() called");
    return false;
}

void HAPPlatformThreadTestNetworkReachability(
        const HAPPlatformThreadNetworkParameters* parameters HAP_UNUSED,
        HAPTime testDuration HAP_UNUSED,
        HAPPlatformRunLoopCallback callback HAP_UNUSED,
        void* server HAP_UNUSED) {
    HAPLog(&kHAPLog_Default, "HAPPlatformThreadTestNetworkReachability() called");
}

void HAPPlatformThreadRegisterBorderRouterStateCallback(
        HAPPlatformThreadBorderRouterStateCallback callback HAP_UNUSED,
        void* context HAP_UNUSED) {
    HAPLog(&kHAPLog_Default, "%s", __func__);
}

void HAPPlatformThreadDeregisterBorderRouterStateCallback(
        HAPPlatformThreadBorderRouterStateCallback callback HAP_UNUSED) {
    HAPLog(&kHAPLog_Default, "%s", __func__);
}

void HAPPlatformThreadSetNetworkParameters(const HAPPlatformThreadNetworkParameters* parameters HAP_UNUSED) {
    HAPLog(&kHAPLog_Default, "HAPPlatformThreadSetNetworkParameters() called");
}

HAPError HAPPlatformThreadStartJoiner(
        const char* passphrase HAP_UNUSED,
        HAPPlatformRunLoopCallback callback HAP_UNUSED,
        void* server HAP_UNUSED) {
    HAPLog(&kHAPLog_Default, "HAPPlatformThreadStartJoiner() called");
    return kHAPError_None;
}

void HAPPlatformThreadJoinCommissionedNetwork(void) {
    HAPLog(&kHAPLog_Default, "HAPPlatformThreadJoinCommissionedNetwork() called");
}

bool HAPPlatformThreadJoinStaticCommissioninedNetwork(void) {
    HAPLog(&kHAPLog_Default, "HAPPlatformThreadJoinStaticCommissioninedNetwork() called");
    return false;
}

HAPError HAPPlatformThreadGetNetworkParameters(HAPPlatformThreadNetworkParameters* parameters HAP_UNUSED) {
    HAPLog(&kHAPLog_Default, "HAPPlatformThreadGetNetworkParameters() called");
    return kHAPError_None;
}

HAPError HAPPlatformThreadClearParameters(void* server HAP_UNUSED) {
    HAPLog(&kHAPLog_Default, "HAPPlatformThreadClearParameters() called");
    return kHAPError_None;
}

void HAPPlatformThreadInitiateFactoryReset(void) {
    HAPLog(&kHAPLog_Default, "HAPPlatformThreadInitiateFactoryReset() called");
}

HAPError HAPPlatformThreadGetRole(HAPPlatformThreadDeviceRole* role) {
    HAPLog(&kHAPLog_Default, "HAPPlatformThreadGetRole() called");
    *role = kHAPPlatformThreadDeviceRole_Child;
    return kHAPError_None;
}

HAPError HAPPlatformThreadRefreshAdvertisement(void* server HAP_UNUSED, bool updateTxtRecord HAP_UNUSED) {
    HAPLog(&kHAPLog_Default, "HAPPlatformThreadRefreshAdvertisement() called");
    return kHAPError_None;
}

void HAPPlatformThreadRegisterNetworkLossCallback(HAPPlatformThreadNetworkLossCallback callback HAP_UNUSED) {
    HAPLog(&kHAPLog_Default, "HAPPlatformThreadNetworkLossCallback() called");
}

void HAPPlatformThreadDeregisterNetworkLossCallback(HAPPlatformThreadNetworkLossCallback callback HAP_UNUSED) {
    HAPLog(&kHAPLog_Default, "HAPPlatformThreadDeregisterNetworkLossCallback() called");
}

void HAPPlatformThreadAddWakeLock(HAPPlatformThreadWakeLock* lock HAP_UNUSED, HAPTime timeout HAP_UNUSED) {
    HAPLog(&kHAPLog_Default, "HAPPlatformThreadAddWakeLock() called");
}

void HAPPlatformThreadRemoveWakeLock(HAPPlatformThreadWakeLock* lock HAP_UNUSED) {
    HAPLog(&kHAPLog_Default, "HAPPlatformThreadRemoveWakeLock() called");
}

void HAPPlatformThreadPurgeWakeLocks() {
    HAPLog(&kHAPLog_Default, "HAPPlatformThreadPurgeWakeLocks() called");
}

// This struct should hold any data the Thread service needs.
struct {
    uint32_t id;
} threadHandle;

void* HAPPlatformThreadGetHandle(void) {
    HAPLog(&kHAPLog_Default, "HAPPlatformThreadGetHandle() called");
    return &threadHandle;
}

void HAPPlatformThreadRegisterRoleUpdateCallback(HAPPlatformRunLoopCallback callback HAP_UNUSED) {
    // Darwin thread doesn't need to deal with role changing since we are not actually running thread
}

void HAPPlatformThreadUnregisterRoleUpdateCallback(void) {
}