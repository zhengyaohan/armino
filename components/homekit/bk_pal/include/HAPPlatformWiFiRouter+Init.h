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

#ifndef HAP_PLATFORM_WIFI_ROUTER_INIT_H
#define HAP_PLATFORM_WIFI_ROUTER_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPPlatform.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/** Maximum number of WAN configurations. */
#define kHAPPlatformWiFiRouter_MaxWANs ((size_t) 1)

/** Maximum number of network client group configurations. */
#define kHAPPlatformWiFiRouter_MaxGroups ((size_t) 3)

/** Maximum number of network client profile configurations. */
#define kHAPPlatformWiFiRouter_MaxClients ((size_t) 1024)

/** Maximum number of WAN / LAN firewall rules per network client profile configuration. */
#define kHAPPlatformWiFiRouter_MaxFirewallRules ((size_t) 32)

/** Maximum number of network client profile configurations that may be edited before committing the changes. */
#define kHAPPlatformWiFiRouter_MaxEdits ((size_t) 32)

/** Maximum number of concurrently connected network clients. */
#define kHAPPlatformWiFiRouter_MaxConnections ((size_t) 64)

/** Maximum number of IP addresses per connected network client. */
#define kHAPPlatformWiFiRouterConnection_MaxIPAddresses ((size_t) 4)

/** Maximum length of a network client name. */
#define kHAPPlatformWiFiRouterConnection_MaxNameBytes ((size_t) 255)

/**
 * Network client profile configuration.
 */
typedef struct {
    HAPPlatformWiFiRouterClientIdentifier identifier;
    HAPPlatformWiFiRouterGroupIdentifier groupIdentifier; // If set to 0, network client profile is deleted.
    HAPPlatformWiFiRouterClientCredential credential;
    struct {
        HAPPlatformWiFiRouterFirewallType type;
        union {
            struct {
                HAPPlatformWiFiRouterPortWANFirewallRule _;
                char bytes[512];
            } port;
            struct {
                HAPPlatformWiFiRouterICMPWANFirewallRule _;
                char bytes[512];
            } icmp;
        } rules[kHAPPlatformWiFiRouter_MaxFirewallRules];
        size_t numRules;
    } wanFirewall;
    struct {
        HAPPlatformWiFiRouterFirewallType type;
        union {
            HAPPlatformWiFiRouterMulticastBridgingLANFirewallRule multicastBridging;
            HAPPlatformWiFiRouterStaticPortLANFirewallRule staticPort;
            struct {
                HAPPlatformWiFiRouterDynamicPortLANFirewallRule _;
                char serviceTypeStorage[256];
                bool serviceTypeStorageInUse : 1;
            } dynamicPort;
            HAPPlatformWiFiRouterStaticICMPLANFirewallRule staticICMP;
        } rules[kHAPPlatformWiFiRouter_MaxFirewallRules];
        size_t numRules;
    } lanFirewall;
} HAPPlatformWiFiRouterClient;

/**
 * Wi-Fi router.
 */
struct HAPPlatformWiFiRouter {
    // Do not access the instance fields directly.
    /**@cond */
    HAPPlatformWiFiRouterDelegate delegate;

    uint8_t sharedAccessRefCount : 4;
    uint8_t exclusiveAccessRefCount : 4;
    uint8_t enumerationRefCount;

    struct {
        HAPPlatformWiFiRouterWANIdentifier identifier;
        HAPPlatformWiFiRouterWANType wanType;

        HAPPlatformWiFiRouterWANStatus wanStatus;
    } wans[kHAPPlatformWiFiRouter_MaxWANs];
    size_t numWANs;

    struct {
        HAPPlatformWiFiRouterGroupIdentifier identifier;
    } groups[kHAPPlatformWiFiRouter_MaxGroups];
    size_t numGroups;

    HAPPlatformWiFiRouterClientIdentifier lastAddedClientIdentifier;
    HAPPlatformWiFiRouterClient clients[kHAPPlatformWiFiRouter_MaxClients];
    struct {
        HAPPlatformWiFiRouterAccessViolationMetadata violation;
    } accessViolations[kHAPPlatformWiFiRouter_MaxClients];
    size_t numClients;

    HAPPlatformWiFiRouterClient edits[kHAPPlatformWiFiRouter_MaxEdits];
    size_t numEdits;
    bool isEditingWANFirewall : 1;
    bool isEditingLANFirewall : 1;

    struct {
        bool isActive : 1;
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier;
        HAPMACAddress macAddress;
        HAPIPAddress ipAddresses[kHAPPlatformWiFiRouterConnection_MaxIPAddresses];
        size_t numIPAddresses;
        char name[kHAPPlatformWiFiRouterConnection_MaxNameBytes + 1];
    } connections[kHAPPlatformWiFiRouter_MaxConnections];

    bool isManagedNetworkEnabled : 1;
    bool isModifyingConfiguration : 1;

    bool isReady : 1;
    /**@endcond */
};

/**
 * Wi-Fi router initialization options.
 */
typedef struct {
    char _;
} HAPPlatformWiFiRouterOptions;

/**
 * Initializes the Wi-Fi router.
 *
 * @param[out] wiFiRouter           Pointer to an allocated but uninitialized HAPPlatformWiFiRouter structure.
 * @param      options              Initialization options.
 */
void HAPPlatformWiFiRouterCreate(HAPPlatformWiFiRouterRef wiFiRouter, const HAPPlatformWiFiRouterOptions* options);

/**
 * Returns whether network configuration access (shared or exclusive) is available.
 *
 * @param      wiFiRouter           Wi-Fi router.
 *
 * @return true                     If network configuration access may be requested.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPPlatformWiFiRouterIsConfigurationAccessAvailable(HAPPlatformWiFiRouterRef wiFiRouter);

/**
 * Connects a network client to a Wi-Fi router.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      credential           Credential used by the network client if a Wi-Fi network interface is specified.
 * @param      macAddress           MAC address of the network client.
 * @param      name                 Name of the network client, if available.
 * @param[out] ipAddress            IP address of the network client. Optional.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If a network client with the given MAC address is already connected.
 * @return kHAPError_InvalidData    If the given parameters are not supported.
 * @return kHAPError_OufOfResources If no more network clients may join the network interface.
 * @return kHAPError_NotAuthorized  If the specified credential is not valid for the network interface.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterConnectClient(
        HAPPlatformWiFiRouterRef wiFiRouter,
        const HAPWiFiWPAPersonalCredential* _Nullable credential,
        const HAPMACAddress* macAddress,
        const char* _Nullable name,
        HAPIPAddress* _Nullable ipAddress);

/**
 * Disconnects a network client from a Wi-Fi router.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      macAddress           MAC address of the network client.
 */
void HAPPlatformWiFiRouterDisconnectClient(HAPPlatformWiFiRouterRef wiFiRouter, const HAPMACAddress* macAddress);

/**
 * Sets the ready state of a Wi-Fi router.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      ready                Whether the Wi-Fi router should indicate that it is ready.
 */
void HAPPlatformWiFiRouterSetReady(HAPPlatformWiFiRouterRef wiFiRouter, bool ready);

/**
 * Sets the WAN type of a WAN.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      wanIdentifier        WAN identifier.
 * @param      wanType              WAN type.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If network configuration access failed.
 * @return kHAPError_InvalidState   If no WAN with the given identifier exists.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterWANSetType(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterWANIdentifier wanIdentifier,
        HAPPlatformWiFiRouterWANType wanType);

/**
 * Sets the WAN status of a WAN.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      wanIdentifier        WAN identifier.
 * @param      wanStatus            WAN status.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If network configuration access failed.
 * @return kHAPError_InvalidState   If no WAN with the given identifier exists.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterWANSetStatus(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterWANIdentifier wanIdentifier,
        HAPPlatformWiFiRouterWANStatus wanStatus);

/**
 * Records a network access violation for a network client.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      clientIdentifier     Network client profile identifier to record network access violation for.
 * @param      timestamp            Network access violation timestamp.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If no network client profile with the given identifier exists.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientRecordAccessViolation(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterTimestamp timestamp);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
