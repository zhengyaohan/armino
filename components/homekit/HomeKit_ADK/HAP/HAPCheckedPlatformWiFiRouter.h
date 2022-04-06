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

#ifndef HAP_CHECKED_PLATFORM_WIFI_ROUTER_H
#define HAP_CHECKED_PLATFORM_WIFI_ROUTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPPlatformWiFiRouter.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_ROUTER)

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

void HAPCheckedPlatformWiFiRouterSetDelegate(
        HAPPlatformWiFiRouterRef wiFiRouter,
        const HAPPlatformWiFiRouterDelegate* _Nullable delegate);
HAP_DISALLOW_USE(HAPPlatformWiFiRouterSetDelegate)

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterAcquireSharedConfigurationAccess(HAPPlatformWiFiRouterRef wiFiRouter);
HAP_DISALLOW_USE(HAPPlatformWiFiRouterAcquireSharedConfigurationAccess)

void HAPCheckedPlatformWiFiRouterReleaseSharedConfigurationAccess(HAPPlatformWiFiRouterRef wiFiRouter);
HAP_DISALLOW_USE(HAPPlatformWiFiRouterReleaseSharedConfigurationAccess)

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterAcquireExclusiveConfigurationAccess(HAPPlatformWiFiRouterRef wiFiRouter);
HAP_DISALLOW_USE(HAPPlatformWiFiRouterAcquireExclusiveConfigurationAccess)

void HAPCheckedPlatformWiFiRouterReleaseExclusiveConfigurationAccess(HAPPlatformWiFiRouterRef wiFiRouter);
HAP_DISALLOW_USE(HAPPlatformWiFiRouterReleaseExclusiveConfigurationAccess)

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterBeginConfigurationChange(HAPPlatformWiFiRouterRef wiFiRouter);
HAP_DISALLOW_USE(HAPPlatformWiFiRouterBeginConfigurationChange)

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterCommitConfigurationChange(HAPPlatformWiFiRouterRef wiFiRouter);
HAP_DISALLOW_USE(HAPPlatformWiFiRouterCommitConfigurationChange)

void HAPCheckedPlatformWiFiRouterRollbackConfigurationChange(HAPPlatformWiFiRouterRef wiFiRouter);
HAP_DISALLOW_USE(HAPPlatformWiFiRouterRollbackConfigurationChange)

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterIsReady(HAPPlatformWiFiRouterRef wiFiRouter, bool* isReady);
HAP_DISALLOW_USE(HAPPlatformWiFiRouterIsReady)

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterIsManagedNetworkEnabled(HAPPlatformWiFiRouterRef wiFiRouter, bool* isEnabled);
HAP_DISALLOW_USE(HAPPlatformWiFiRouterIsManagedNetworkEnabled)

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterSetManagedNetworkEnabled(HAPPlatformWiFiRouterRef wiFiRouter, bool isEnabled);
HAP_DISALLOW_USE(HAPPlatformWiFiRouterSetManagedNetworkEnabled)

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterEnumerateWANs(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterEnumerateWANsCallback callback,
        void* _Nullable context);
HAP_DISALLOW_USE(HAPPlatformWiFiRouterEnumerateWANs)

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterWANExists(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterWANIdentifier wanIdentifier,
        bool* exists);
HAP_DISALLOW_USE(HAPPlatformWiFiRouterWANExists)

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterWANGetType(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterWANIdentifier wanIdentifier,
        HAPPlatformWiFiRouterWANType* wanType);
HAP_DISALLOW_USE(HAPPlatformWiFiRouterWANGetType)

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterWANGetStatus(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterWANIdentifier wanIdentifier,
        HAPPlatformWiFiRouterWANStatus* wanStatus);
HAP_DISALLOW_USE(HAPPlatformWiFiRouterWANGetStatus)

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterEnumerateClients(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterEnumerateClientsCallback callback,
        void* _Nullable context);
HAP_DISALLOW_USE(HAPPlatformWiFiRouterEnumerateClients)

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterClientExists(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        bool* exists);
HAP_DISALLOW_USE(HAPPlatformWiFiRouterClientExists)

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterAddClient(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterGroupIdentifier groupIdentifier,
        const HAPPlatformWiFiRouterClientCredential* credential,
        HAPPlatformWiFiRouterClientIdentifier* clientIdentifier);
HAP_DISALLOW_USE(HAPPlatformWiFiRouterAddClient)

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterRemoveClient(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier);
HAP_DISALLOW_USE(HAPPlatformWiFiRouterRemoveClient)

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterClientGetGroupIdentifier(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterGroupIdentifier* groupIdentifier);
HAP_DISALLOW_USE(HAPPlatformWiFiRouterClientGetGroupIdentifier)

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterClientSetGroupIdentifier(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterGroupIdentifier groupIdentifier);
HAP_DISALLOW_USE(HAPPlatformWiFiRouterClientSetGroupIdentifier)

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterClientGetCredentialType(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterCredentialType* credentialType);
HAP_DISALLOW_USE(HAPPlatformWiFiRouterClientGetCredentialType)

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterClientGetMACAddressCredential(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPMACAddress* credential);
HAP_DISALLOW_USE(HAPPlatformWiFiRouterClientGetMACAddressCredential)

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterClientSetCredential(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        const HAPPlatformWiFiRouterClientCredential* credential);
HAP_DISALLOW_USE(HAPPlatformWiFiRouterClientSetCredential)

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterClientGetWANFirewallType(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterFirewallType* firewallType);
HAP_DISALLOW_USE(HAPPlatformWiFiRouterClientGetWANFirewallType)

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterClientEnumerateWANFirewallRules(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterClientEnumerateWANFirewallRulesCallback callback,
        void* _Nullable context);
HAP_DISALLOW_USE(HAPPlatformWiFiRouterClientEnumerateWANFirewallRules)

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterClientResetWANFirewall(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterFirewallType firewallType);
HAP_DISALLOW_USE(HAPPlatformWiFiRouterClientResetWANFirewall)

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterClientAddWANFirewallRule(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        const HAPPlatformWiFiRouterWANFirewallRule* firewallRule);
HAP_DISALLOW_USE(HAPPlatformWiFiRouterClientAddWANFirewallRule)

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterClientFinalizeWANFirewallRules(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier);
HAP_DISALLOW_USE(HAPPlatformWiFiRouterClientFinalizeWANFirewallRules)

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterClientGetLANFirewallType(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterFirewallType* firewallType);
HAP_DISALLOW_USE(HAPPlatformWiFiRouterClientGetLANFirewallType)

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterClientEnumerateLANFirewallRules(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterClientEnumerateLANFirewallRulesCallback callback,
        void* _Nullable context);
HAP_DISALLOW_USE(HAPPlatformWiFiRouterClientEnumerateLANFirewallRules)

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterClientResetLANFirewall(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterFirewallType firewallType);
HAP_DISALLOW_USE(HAPPlatformWiFiRouterClientResetLANFirewall)

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterClientAddLANFirewallRule(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        const HAPPlatformWiFiRouterLANFirewallRule* firewallRule);
HAP_DISALLOW_USE(HAPPlatformWiFiRouterClientAddLANFirewallRule)

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterClientFinalizeLANFirewallRules(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier);
HAP_DISALLOW_USE(HAPPlatformWiFiRouterClientFinalizeLANFirewallRules)

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterGetClientStatusForIdentifiers(
        HAPPlatformWiFiRouterRef wiFiRouter,
        const HAPPlatformWiFiRouterClientStatusIdentifier* statusIdentifiers,
        size_t numStatusIdentifiers,
        HAPPlatformWiFiRouterGetClientStatusForIdentifiersCallback callback,
        void* _Nullable context);
HAP_DISALLOW_USE(HAPPlatformWiFiRouterGetClientStatusForIdentifiers)

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterClientGetAccessViolationMetadata(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterAccessViolationMetadata* violationMetadata);
HAP_DISALLOW_USE(HAPPlatformWiFiRouterClientGetAccessViolationMetadata)

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterClientResetAccessViolations(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier);
HAP_DISALLOW_USE(HAPPlatformWiFiRouterClientResetAccessViolations)

HAP_RESULT_USE_CHECK
HAPPlatformWiFiRouterSatelliteStatus
        HAPCheckedPlatformWiFiRouterGetSatelliteStatus(HAPPlatformWiFiRouterRef wiFiRouter, size_t satelliteIndex);
HAP_DISALLOW_USE(HAPPlatformWiFiRouterGetSatelliteStatus)

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif
