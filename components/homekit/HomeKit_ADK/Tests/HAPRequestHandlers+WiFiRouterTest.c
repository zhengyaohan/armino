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
// Copyright (C) 2015-2021 Apple Inc. All Rights Reserved.

#include "HAPCharacteristic.h"
#include "HAPLog+Attributes.h"

#include "HAPPlatform+Init.h"
#include "HAPPlatformWiFiRouter+Init.h"
#include "HAPWiFiRouter.h"

#include "Harness/HAPAccessoryServerHelper.h"
#include "Harness/HAPPlatformClockHelper.h"
#include "Harness/HAPTestController.c"
#include "Harness/IPTestHelpers.h"
#include "Harness/SecurityTestHelpers.h"
#include "Harness/TemplateDB.c"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_ROUTER)

const HAPIPAddress kHAPIPAddress_IPVersion = { .version = kHAPIPAddressVersion_IPv4 };

static const HAPAccessory accessory = { .aid = 1,
                                        .category = kHAPAccessoryCategory_WiFiRouters,
                                        .name = "Acme Test",
                                        .productData = "03d8a775e3644573",
                                        .manufacturer = "Acme",
                                        .model = "Test1,1",
                                        .serialNumber = "099DB48E9E28",
                                        .firmwareVersion = "1",
                                        .hardwareVersion = "1",
                                        .services = (const HAPService* const[]) { &accessoryInformationService,
                                                                                  &hapProtocolInformationService,
                                                                                  &pairingService,
                                                                                  &wiFiRouterService,
                                                                                  NULL },
                                        .callbacks = { .identify = IdentifyAccessoryHelper } };

static HAPPlatformWiFiRouterClientIdentifier _nextExpectedClientIdentifier = 1;

typedef struct {
    HAPPlatformWiFiRouterWANIdentifier identifier;
    HAPPlatformWiFiRouterWANType wanType;
} WANConfiguration;

typedef struct {
    HAPPlatformWiFiRouterWANIdentifier wanIdentifier;
    HAPPlatformWiFiRouterWANStatus linkStatus;
} WANStatus;

typedef union {
    struct {
        HAPPlatformWiFiRouterPortWANFirewallRule _;
        char bytes[512];
    } port;
    struct {
        HAPPlatformWiFiRouterICMPWANFirewallRule _;
        char bytes[512];
    } icmp;
} WANFirewallRule;

typedef union {
    HAPPlatformWiFiRouterMulticastBridgingLANFirewallRule multicastBridging;
    HAPPlatformWiFiRouterStaticPortLANFirewallRule staticPort;
    struct {
        HAPPlatformWiFiRouterDynamicPortLANFirewallRule _;
        char serviceTypeStorage[256];
    } dynamicPort;
    HAPPlatformWiFiRouterStaticICMPLANFirewallRule staticICMP;
} LANFirewallRule;

typedef struct {
    HAPPlatformWiFiRouterClientIdentifier clientIdentifier;
    HAPMACAddress macAddress;
    HAPIPAddress ipAddresses[4];
    size_t numIPAddresses;
    char name[255 + 1];
    struct {
        HAPRSSI value;
        bool isDefined : 1;
    } rssi;
} ClientStatus;

typedef struct {
    HAPPlatformWiFiRouterClientIdentifier identifier;
    HAPPlatformWiFiRouterGroupIdentifier groupIdentifier;
    HAPPlatformWiFiRouterClientCredential credential;
    struct {
        HAPPlatformWiFiRouterFirewallType type;
        WANFirewallRule rules[32];
        size_t numRules;
    } wanFirewall;
    struct {
        HAPPlatformWiFiRouterFirewallType type;
        LANFirewallRule rules[32];
        size_t numRules;
    } lanFirewall;
} ClientConfiguration;

typedef struct {
    WANFirewallRule* rules;
    size_t maxRules;
    size_t* numRules;
} WANFirewallRuleListEnumerateContext;

static void EnumerateWANFirewallRuleTLVCallback(
        void* _Nullable context_,
        HAPCharacteristicValue_WiFiRouter_WANFirewall_Rule* value_,
        bool* shouldContinue) {
    HAPPrecondition(context_);
    WANFirewallRuleListEnumerateContext* context = context_;
    HAPPrecondition(context->rules);
    WANFirewallRule* rules = context->rules;
    size_t maxRules = context->maxRules;
    HAPPrecondition(context->numRules);
    size_t* numRules = context->numRules;
    HAPPrecondition(value_);
    HAPPrecondition(shouldContinue);
    HAPPrecondition(*shouldContinue);

    HAPAssert(*numRules < maxRules);
    WANFirewallRule* firewallRule_ = &rules[*numRules];

    switch (value_->type) {
        case kHAPCharacteristicValueType_WiFiRouter_WANFirewall_Rule_Port: {
            const HAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule* value = &value_->_.port;
            HAPPlatformWiFiRouterPortWANFirewallRule* firewallRule = &firewallRule_->port._;

            firewallRule->type = kHAPPlatformWiFiRouterWANFirewallRuleType_Port;
            switch (value->transportProtocol) {
                case kHAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule_Protocol_TCP: {
                    firewallRule->transportProtocol = kHAPPlatformWiFiRouterTransportProtocol_TCP;
                    break;
                }
                case kHAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule_Protocol_UDP: {
                    firewallRule->transportProtocol = kHAPPlatformWiFiRouterTransportProtocol_UDP;
                    break;
                }
            }
            if (value->hostDNSNameIsSet) {
                HAPAssert(!value->hostIPStartIsSet);
                HAPAssert(!value->hostIPEndIsSet);
                firewallRule->host.type = kHAPPlatformWiFiRouterWANHostURIType_DNSNamePattern;
                firewallRule->host._.dnsNamePattern = firewallRule_->port.bytes;
                size_t numHostDNSNameBytes = HAPStringGetNumBytes(value->hostDNSName);
                HAPAssert(numHostDNSNameBytes < sizeof firewallRule_->port.bytes);
                HAPRawBufferCopyBytes(firewallRule_->port.bytes, value->hostDNSName, numHostDNSNameBytes + 1);
            } else {
                HAPAssert(value->hostIPStartIsSet);
                if (value->hostIPStart.ipv4AddressIsSet && value->hostIPStart.ipv6AddressIsSet) {
                    HAPAssert(HAPIPv4AddressAreEqual(&value->hostIPStart.ipv4Address, &kHAPIPAddress_IPv4Any._.ipv4));
                    HAPAssert(HAPIPv6AddressAreEqual(&value->hostIPStart.ipv6Address, &kHAPIPAddress_IPv6Any._.ipv6));
                    HAPAssert(!value->hostIPEndIsSet);
                    firewallRule->host.type = kHAPPlatformWiFiRouterWANHostURIType_Any;
                } else if (!value->hostIPEndIsSet) {
                    firewallRule->host.type = kHAPPlatformWiFiRouterWANHostURIType_IPAddress;
                    if (value->hostIPStart.ipv4AddressIsSet) {
                        firewallRule->host._.ipAddress.version = kHAPIPAddressVersion_IPv4;
                        HAPRawBufferCopyBytes(
                                &firewallRule->host._.ipAddress._.ipv4,
                                &value->hostIPStart.ipv4Address,
                                sizeof firewallRule->host._.ipAddress._.ipv4);
                    } else {
                        HAPAssert(value->hostIPStart.ipv6AddressIsSet);
                        firewallRule->host._.ipAddress.version = kHAPIPAddressVersion_IPv6;
                        HAPRawBufferCopyBytes(
                                &firewallRule->host._.ipAddress._.ipv6,
                                &value->hostIPStart.ipv6Address,
                                sizeof firewallRule->host._.ipAddress._.ipv6);
                    }
                } else {
                    firewallRule->host.type = kHAPPlatformWiFiRouterWANHostURIType_IPAddressRange;
                    HAPAssert(value->hostIPStart.ipv4AddressIsSet == value->hostIPEnd.ipv4AddressIsSet);
                    HAPAssert(value->hostIPStart.ipv6AddressIsSet == value->hostIPEnd.ipv6AddressIsSet);
                    if (value->hostIPStart.ipv4AddressIsSet) {
                        firewallRule->host._.ipAddressRange.version = kHAPIPAddressVersion_IPv4;
                        HAPRawBufferCopyBytes(
                                &firewallRule->host._.ipAddressRange._.ipv4.startAddress,
                                &value->hostIPStart.ipv4Address,
                                sizeof firewallRule->host._.ipAddressRange._.ipv4.startAddress);
                        HAPRawBufferCopyBytes(
                                &firewallRule->host._.ipAddressRange._.ipv4.endAddress,
                                &value->hostIPEnd.ipv4Address,
                                sizeof firewallRule->host._.ipAddressRange._.ipv4.endAddress);
                    } else {
                        HAPAssert(value->hostIPStart.ipv6AddressIsSet);
                        HAPRawBufferCopyBytes(
                                &firewallRule->host._.ipAddressRange._.ipv6.startAddress,
                                &value->hostIPStart.ipv6Address,
                                sizeof firewallRule->host._.ipAddressRange._.ipv6.startAddress);
                        HAPRawBufferCopyBytes(
                                &firewallRule->host._.ipAddressRange._.ipv6.endAddress,
                                &value->hostIPEnd.ipv6Address,
                                sizeof firewallRule->host._.ipAddressRange._.ipv6.endAddress);
                    }
                }
            }
            firewallRule->hostPortRange.startPort = value->hostPortStart;
            if (!value->hostPortEndIsSet) {
                firewallRule->hostPortRange.endPort = value->hostPortStart;
            } else {
                HAPAssert(value->hostPortStart != kHAPNetworkPort_Any);
                HAPAssert(value->hostPortEnd > value->hostPortStart);

                firewallRule->hostPortRange.endPort = value->hostPortEnd;
            }
            break;
        }
        case kHAPCharacteristicValueType_WiFiRouter_WANFirewall_Rule_ICMP: {
            const HAPCharacteristicValue_WiFiRouter_WANFirewall_ICMPRule* value = &value_->_.icmp;
            HAPPlatformWiFiRouterICMPWANFirewallRule* firewallRule = &firewallRule_->icmp._;

            firewallRule->type = kHAPPlatformWiFiRouterWANFirewallRuleType_ICMP;
            if (value->hostDNSNameIsSet) {
                HAPAssert(!value->hostIPStartIsSet);
                HAPAssert(!value->hostIPEndIsSet);
                firewallRule->host.type = kHAPPlatformWiFiRouterWANHostURIType_DNSNamePattern;
                firewallRule->host._.dnsNamePattern = firewallRule_->icmp.bytes;
                size_t numHostDNSNameBytes = HAPStringGetNumBytes(value->hostDNSName);
                HAPAssert(numHostDNSNameBytes < sizeof firewallRule_->icmp.bytes);
                HAPRawBufferCopyBytes(firewallRule_->icmp.bytes, value->hostDNSName, numHostDNSNameBytes + 1);
            } else {
                HAPAssert(value->hostIPStartIsSet);
                if (value->hostIPStart.ipv4AddressIsSet && value->hostIPStart.ipv6AddressIsSet) {
                    HAPAssert(HAPIPv4AddressAreEqual(&value->hostIPStart.ipv4Address, &kHAPIPAddress_IPv4Any._.ipv4));
                    HAPAssert(HAPIPv6AddressAreEqual(&value->hostIPStart.ipv6Address, &kHAPIPAddress_IPv6Any._.ipv6));
                    HAPAssert(!value->hostIPEndIsSet);
                    firewallRule->host.type = kHAPPlatformWiFiRouterWANHostURIType_Any;
                } else if (!value->hostIPEndIsSet) {
                    firewallRule->host.type = kHAPPlatformWiFiRouterWANHostURIType_IPAddress;
                    if (value->hostIPStart.ipv4AddressIsSet) {
                        firewallRule->host._.ipAddress.version = kHAPIPAddressVersion_IPv4;
                        HAPRawBufferCopyBytes(
                                &firewallRule->host._.ipAddress._.ipv4,
                                &value->hostIPStart.ipv4Address,
                                sizeof firewallRule->host._.ipAddress._.ipv4);
                    } else {
                        HAPAssert(value->hostIPStart.ipv6AddressIsSet);
                        firewallRule->host._.ipAddress.version = kHAPIPAddressVersion_IPv6;
                        HAPRawBufferCopyBytes(
                                &firewallRule->host._.ipAddress._.ipv6,
                                &value->hostIPStart.ipv6Address,
                                sizeof firewallRule->host._.ipAddress._.ipv6);
                    }
                } else {
                    firewallRule->host.type = kHAPPlatformWiFiRouterWANHostURIType_IPAddressRange;
                    HAPAssert(value->hostIPStart.ipv4AddressIsSet == value->hostIPEnd.ipv4AddressIsSet);
                    HAPAssert(value->hostIPStart.ipv6AddressIsSet == value->hostIPEnd.ipv6AddressIsSet);
                    if (value->hostIPStart.ipv4AddressIsSet) {
                        firewallRule->host._.ipAddressRange.version = kHAPIPAddressVersion_IPv4;
                        HAPRawBufferCopyBytes(
                                &firewallRule->host._.ipAddressRange._.ipv4.startAddress,
                                &value->hostIPStart.ipv4Address,
                                sizeof firewallRule->host._.ipAddressRange._.ipv4.startAddress);
                        HAPRawBufferCopyBytes(
                                &firewallRule->host._.ipAddressRange._.ipv4.endAddress,
                                &value->hostIPEnd.ipv4Address,
                                sizeof firewallRule->host._.ipAddressRange._.ipv4.endAddress);
                    } else {
                        HAPAssert(value->hostIPStart.ipv6AddressIsSet);
                        HAPRawBufferCopyBytes(
                                &firewallRule->host._.ipAddressRange._.ipv6.startAddress,
                                &value->hostIPStart.ipv6Address,
                                sizeof firewallRule->host._.ipAddressRange._.ipv6.startAddress);
                        HAPRawBufferCopyBytes(
                                &firewallRule->host._.ipAddressRange._.ipv6.endAddress,
                                &value->hostIPEnd.ipv6Address,
                                sizeof firewallRule->host._.ipAddressRange._.ipv6.endAddress);
                    }
                }
            }
            HAPAssert(value->icmpList.numICMPTypes <= HAPArrayCount(firewallRule->icmpTypes));
            for (size_t i = 0; i < value->icmpList.numICMPTypes; i++) {
                switch (value->icmpList.icmpTypes[i].icmpProtocol) {
                    case kHAPCharacteristicValue_WiFiRouter_ICMPProtocol_ICMPv4: {
                        firewallRule->icmpTypes[i].icmpProtocol = kHAPPlatformWiFiRouterICMPProtocol_ICMPv4;
                        break;
                    }
                    case kHAPCharacteristicValue_WiFiRouter_ICMPProtocol_ICMPv6: {
                        firewallRule->icmpTypes[i].icmpProtocol = kHAPPlatformWiFiRouterICMPProtocol_ICMPv6;
                        break;
                    }
                }
                if (value->icmpList.icmpTypes[i].typeValueIsSet) {
                    firewallRule->icmpTypes[i].typeValue = value->icmpList.icmpTypes[i].typeValue;
                    firewallRule->icmpTypes[i].typeValueIsSet = true;
                }
            }
            firewallRule->numICMPTypes = value->icmpList.numICMPTypes;
            break;
        }
    }

    (*numRules)++;
}

static void ParseWANFirewallConfiguration(
        HAPCharacteristicValue_WiFiRouter_WANFirewall* wanFirewall,
        HAPPlatformWiFiRouterFirewallType* type,
        WANFirewallRule* rules,
        size_t maxRules,
        size_t* numRules) {
    HAPPrecondition(wanFirewall);
    HAPPrecondition(type);
    HAPPrecondition(rules);
    HAPPrecondition(numRules);

    HAPError err;

    *numRules = 0;

    switch (wanFirewall->type) {
        case kHAPCharacteristicValue_WiFiRouter_FirewallType_FullAccess: {
            *type = kHAPPlatformWiFiRouterFirewallType_FullAccess;
            break;
        }
        case kHAPCharacteristicValue_WiFiRouter_FirewallType_Allowlist: {
            *type = kHAPPlatformWiFiRouterFirewallType_Allowlist;

            HAPAssert(wanFirewall->ruleListIsSet);

            WANFirewallRuleListEnumerateContext enumerateContext;
            HAPRawBufferZero(&enumerateContext, sizeof enumerateContext);
            enumerateContext.rules = rules;
            enumerateContext.maxRules = maxRules;
            enumerateContext.numRules = numRules;

            err = wanFirewall->ruleList.enumerate(
                    &wanFirewall->ruleList.dataSource, EnumerateWANFirewallRuleTLVCallback, &enumerateContext);
            HAPAssert(!err);
            break;
        }
    }
}

typedef struct {
    LANFirewallRule* rules;
    size_t maxRules;
    size_t* numRules;
} LANFirewallRuleListEnumerateContext;

static void EnumerateLANFirewallRuleTLVCallback(
        void* _Nullable context_,
        HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule* value_,
        bool* shouldContinue) {
    HAPPrecondition(context_);
    LANFirewallRuleListEnumerateContext* context = context_;
    HAPPrecondition(context->rules);
    LANFirewallRule* rules = context->rules;
    size_t maxRules = context->maxRules;
    HAPPrecondition(context->numRules);
    size_t* numRules = context->numRules;
    HAPPrecondition(value_);
    HAPPrecondition(shouldContinue);
    HAPPrecondition(*shouldContinue);

    HAPAssert(*numRules < maxRules);
    LANFirewallRule* firewallRule_ = &rules[*numRules];

    switch (value_->type) {
        case kHAPCharacteristicValueType_WiFiRouter_LANFirewall_Rule_MulticastBridging: {
            const HAPCharacteristicValue_WiFiRouter_LANFirewall_MulticastBridgingRule* value =
                    &value_->_.multicastBridging;
            HAPPlatformWiFiRouterMulticastBridgingLANFirewallRule* firewallRule = &firewallRule_->multicastBridging;

            firewallRule->type = kHAPPlatformWiFiRouterLANFirewallRuleType_MulticastBridging;
            switch (value->direction) {
                case kHAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction_Outbound: {
                    firewallRule->direction = kHAPPlatformWiFiRouterFirewallRuleDirection_Outbound;
                    break;
                }
                case kHAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction_Inbound: {
                    firewallRule->direction = kHAPPlatformWiFiRouterFirewallRuleDirection_Inbound;
                    break;
                }
            }
            HAPAssert(firewallRule->direction);
            HAPAssert(value->endpointList.numGroupIdentifiers <= HAPArrayCount(firewallRule->peerGroupIdentifiers));
            HAPRawBufferCopyBytes(
                    firewallRule->peerGroupIdentifiers,
                    value->endpointList.groupIdentifiers,
                    value->endpointList.numGroupIdentifiers * sizeof value->endpointList.groupIdentifiers[0]);
            firewallRule->numPeerGroupIdentifiers = value->endpointList.numGroupIdentifiers;
            if (value->ipAddress.ipv4AddressIsSet) {
                HAPAssert(!value->ipAddress.ipv6AddressIsSet);
                firewallRule->destinationIP.version = kHAPIPAddressVersion_IPv4;
                HAPRawBufferCopyBytes(
                        &firewallRule->destinationIP._.ipv4,
                        &value->ipAddress.ipv4Address,
                        sizeof firewallRule->destinationIP._.ipv4);
            } else {
                HAPAssert(value->ipAddress.ipv6AddressIsSet);
                firewallRule->destinationIP.version = kHAPIPAddressVersion_IPv6;
                HAPRawBufferCopyBytes(
                        &firewallRule->destinationIP._.ipv6,
                        &value->ipAddress.ipv4Address,
                        sizeof firewallRule->destinationIP._.ipv6);
            }
            firewallRule->destinationPort = value->port;
            break;
        }
        case kHAPCharacteristicValueType_WiFiRouter_LANFirewall_Rule_StaticPort: {
            const HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule* value = &value_->_.staticPort;
            HAPPlatformWiFiRouterStaticPortLANFirewallRule* firewallRule = &firewallRule_->staticPort;

            firewallRule->type = kHAPPlatformWiFiRouterLANFirewallRuleType_StaticPort;
            switch (value->direction) {
                case kHAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction_Outbound: {
                    firewallRule->direction = kHAPPlatformWiFiRouterFirewallRuleDirection_Outbound;
                    break;
                }
                case kHAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction_Inbound: {
                    firewallRule->direction = kHAPPlatformWiFiRouterFirewallRuleDirection_Inbound;
                    break;
                }
            }
            HAPAssert(firewallRule->direction);
            HAPAssert(value->endpointList.numGroupIdentifiers <= HAPArrayCount(firewallRule->peerGroupIdentifiers));
            HAPRawBufferCopyBytes(
                    firewallRule->peerGroupIdentifiers,
                    value->endpointList.groupIdentifiers,
                    value->endpointList.numGroupIdentifiers * sizeof value->endpointList.groupIdentifiers[0]);
            firewallRule->numPeerGroupIdentifiers = value->endpointList.numGroupIdentifiers;
            switch (value->transportProtocol) {
                case kHAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule_Protocol_TCP: {
                    firewallRule->transportProtocol = kHAPPlatformWiFiRouterTransportProtocol_TCP;
                    break;
                }
                case kHAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule_Protocol_UDP: {
                    firewallRule->transportProtocol = kHAPPlatformWiFiRouterTransportProtocol_UDP;
                    break;
                }
            }
            HAPAssert(firewallRule->transportProtocol);
            firewallRule->destinationPortRange.startPort = value->portStart;
            firewallRule->destinationPortRange.endPort = value->portStart;
            if (value->portEndIsSet) {
                firewallRule->destinationPortRange.endPort = value->portEnd;
            }
            break;
        }
        case kHAPCharacteristicValueType_WiFiRouter_LANFirewall_Rule_DynamicPort: {
            const HAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule* value = &value_->_.dynamicPort;
            HAPPlatformWiFiRouterDynamicPortLANFirewallRule* firewallRule = &firewallRule_->dynamicPort._;

            firewallRule->type = kHAPPlatformWiFiRouterLANFirewallRuleType_DynamicPort;
            switch (value->direction) {
                case kHAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction_Outbound: {
                    firewallRule->direction = kHAPPlatformWiFiRouterFirewallRuleDirection_Outbound;
                    break;
                }
                case kHAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction_Inbound: {
                    firewallRule->direction = kHAPPlatformWiFiRouterFirewallRuleDirection_Inbound;
                    break;
                }
            }
            HAPAssert(firewallRule->direction);
            HAPAssert(value->endpointList.numGroupIdentifiers <= HAPArrayCount(firewallRule->peerGroupIdentifiers));
            HAPRawBufferCopyBytes(
                    firewallRule->peerGroupIdentifiers,
                    value->endpointList.groupIdentifiers,
                    value->endpointList.numGroupIdentifiers * sizeof value->endpointList.groupIdentifiers[0]);
            firewallRule->numPeerGroupIdentifiers = value->endpointList.numGroupIdentifiers;
            switch (value->transportProtocol) {
                case kHAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule_Protocol_TCP: {
                    firewallRule->transportProtocol = kHAPPlatformWiFiRouterTransportProtocol_TCP;
                    break;
                }
                case kHAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule_Protocol_UDP: {
                    firewallRule->transportProtocol = kHAPPlatformWiFiRouterTransportProtocol_UDP;
                    break;
                }
            }
            HAPAssert(firewallRule->transportProtocol);
            switch (value->advertProtocol) {
                case kHAPCharacteristicValue_WiFiRouter_AdvertisementProtocol_DNSSD: {
                    firewallRule->serviceType.advertisementProtocol =
                            kHAPPlatformWiFiRouterDynamicPortAdvertisementProtocol_DNSSD;
                    break;
                }
                case kHAPCharacteristicValue_WiFiRouter_AdvertisementProtocol_SSDP: {
                    firewallRule->serviceType.advertisementProtocol =
                            kHAPPlatformWiFiRouterDynamicPortAdvertisementProtocol_SSDP;
                    break;
                }
            }
            HAPAssert(firewallRule->serviceType.advertisementProtocol);
            if (value->flags & kHAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule_Flags_AdvertisementOnly) {
                firewallRule->advertisementOnly = true;
            }
            if (value->serviceIsSet) {
                char* serviceTypeStorage = firewallRule_->dynamicPort.serviceTypeStorage;
                size_t maxServiceTypeBytes = sizeof firewallRule_->dynamicPort.serviceTypeStorage - 1;
                switch (value->advertProtocol) {
                    case kHAPCharacteristicValue_WiFiRouter_AdvertisementProtocol_DNSSD: {
                        const char* serviceType = value->service.name;
                        size_t numServiceTypeBytes = HAPStringGetNumBytes(serviceType);
                        HAPAssert(numServiceTypeBytes <= maxServiceTypeBytes);
                        HAPRawBufferCopyBytes(serviceTypeStorage, serviceType, numServiceTypeBytes);
                        firewallRule->serviceType._.dns_sd.serviceType = serviceTypeStorage;
                        break;
                    }
                    case kHAPCharacteristicValue_WiFiRouter_AdvertisementProtocol_SSDP: {
                        const char* serviceTypeURI = value->service.name;
                        size_t numServiceTypeURIBytes = HAPStringGetNumBytes(serviceTypeURI);
                        HAPAssert(numServiceTypeURIBytes <= maxServiceTypeBytes);
                        HAPRawBufferCopyBytes(serviceTypeStorage, serviceTypeURI, numServiceTypeURIBytes);
                        firewallRule->serviceType._.ssdp.serviceTypeURI = serviceTypeStorage;
                        break;
                    }
                }
            }
            break;
        }
        case kHAPCharacteristicValueType_WiFiRouter_LANFirewall_Rule_StaticICMP: {
            const HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticICMPRule* value = &value_->_.staticICMP;
            HAPPlatformWiFiRouterStaticICMPLANFirewallRule* firewallRule = &firewallRule_->staticICMP;

            firewallRule->type = kHAPPlatformWiFiRouterLANFirewallRuleType_StaticICMP;
            switch (value->direction) {
                case kHAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction_Outbound: {
                    firewallRule->direction = kHAPPlatformWiFiRouterFirewallRuleDirection_Outbound;
                    break;
                }
                case kHAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction_Inbound: {
                    firewallRule->direction = kHAPPlatformWiFiRouterFirewallRuleDirection_Inbound;
                    break;
                }
            }
            HAPAssert(firewallRule->direction);
            HAPAssert(value->endpointList.numGroupIdentifiers <= HAPArrayCount(firewallRule->peerGroupIdentifiers));
            HAPRawBufferCopyBytes(
                    firewallRule->peerGroupIdentifiers,
                    value->endpointList.groupIdentifiers,
                    value->endpointList.numGroupIdentifiers * sizeof value->endpointList.groupIdentifiers[0]);
            firewallRule->numPeerGroupIdentifiers = value->endpointList.numGroupIdentifiers;
            HAPAssert(value->icmpList.numICMPTypes <= HAPArrayCount(firewallRule->icmpTypes));
            for (size_t i = 0; i < value->icmpList.numICMPTypes; i++) {
                switch (value->icmpList.icmpTypes[i].icmpProtocol) {
                    case kHAPCharacteristicValue_WiFiRouter_ICMPProtocol_ICMPv4: {
                        firewallRule->icmpTypes[i].icmpProtocol = kHAPPlatformWiFiRouterICMPProtocol_ICMPv4;
                        break;
                    }
                    case kHAPCharacteristicValue_WiFiRouter_ICMPProtocol_ICMPv6: {
                        firewallRule->icmpTypes[i].icmpProtocol = kHAPPlatformWiFiRouterICMPProtocol_ICMPv6;
                        break;
                    }
                }
                if (value->icmpList.icmpTypes[i].typeValueIsSet) {
                    firewallRule->icmpTypes[i].typeValue = value->icmpList.icmpTypes[i].typeValue;
                    firewallRule->icmpTypes[i].typeValueIsSet = true;
                }
            }
            firewallRule->numICMPTypes = value->icmpList.numICMPTypes;
            break;
        }
    }

    (*numRules)++;
}

static void ParseLANFirewallConfiguration(
        HAPCharacteristicValue_WiFiRouter_LANFirewall* lanFirewall,
        HAPPlatformWiFiRouterFirewallType* type,
        LANFirewallRule* rules,
        size_t maxRules,
        size_t* numRules) {
    HAPPrecondition(lanFirewall);
    HAPPrecondition(type);
    HAPPrecondition(rules);
    HAPPrecondition(numRules);

    HAPError err;

    *numRules = 0;

    switch (lanFirewall->type) {
        case kHAPCharacteristicValue_WiFiRouter_FirewallType_FullAccess: {
            *type = kHAPPlatformWiFiRouterFirewallType_FullAccess;
            break;
        }
        case kHAPCharacteristicValue_WiFiRouter_FirewallType_Allowlist: {
            *type = kHAPPlatformWiFiRouterFirewallType_Allowlist;

            HAPAssert(lanFirewall->ruleListIsSet);

            LANFirewallRuleListEnumerateContext enumerateContext;
            HAPRawBufferZero(&enumerateContext, sizeof enumerateContext);
            enumerateContext.rules = rules;
            enumerateContext.maxRules = maxRules;
            enumerateContext.numRules = numRules;

            err = lanFirewall->ruleList.enumerate(
                    &lanFirewall->ruleList.dataSource, EnumerateLANFirewallRuleTLVCallback, &enumerateContext);
            HAPAssert(!err);
            break;
        }
    }
}

typedef struct {
    WANConfiguration* configs;
    size_t maxConfigs;
    size_t numConfigs;
} EnumerateWANConfigurationsContext;

static void EnumerateWANConfigurationsCallback(
        void* _Nullable context_,
        HAPCharacteristicValue_WANConfigurationList_Config* value,
        bool* shouldContinue) {
    HAPPrecondition(context_);
    EnumerateWANConfigurationsContext* context = context_;
    HAPPrecondition(context->configs);
    HAPPrecondition(context->numConfigs < context->maxConfigs);
    WANConfiguration* config = &context->configs[context->numConfigs++];
    HAPRawBufferZero(config, sizeof *config);
    HAPPrecondition(value);
    HAPPrecondition(shouldContinue);
    HAPPrecondition(*shouldContinue);

    config->identifier = value->wanIdentifier;

    switch (value->wanType) {
        case kHAPCharacteristicValue_WANConfigurationList_Config_WANType_Unconfigured: {
            config->wanType = kHAPPlatformWiFiRouterWANType_Unconfigured;
            break;
        }
        case kHAPCharacteristicValue_WANConfigurationList_Config_WANType_Other: {
            config->wanType = kHAPPlatformWiFiRouterWANType_Other;
            break;
        }
        case kHAPCharacteristicValue_WANConfigurationList_Config_WANType_DHCP: {
            config->wanType = kHAPPlatformWiFiRouterWANType_DHCP;
            break;
        }
        case kHAPCharacteristicValue_WANConfigurationList_Config_WANType_BridgeMode: {
            config->wanType = kHAPPlatformWiFiRouterWANType_BridgeMode;
            break;
        }
    }
}

typedef struct {
    WANStatus* statuses;
    size_t maxStatuses;
    size_t numStatuses;
} EnumerateWANStatusesContext;

static void EnumerateWANStatusesCallback(
        void* _Nullable context_,
        HAPCharacteristicValue_WANStatusList_Status* value,
        bool* shouldContinue) {
    HAPPrecondition(context_);
    EnumerateWANStatusesContext* context = context_;
    HAPPrecondition(context->statuses);
    HAPPrecondition(context->numStatuses < context->maxStatuses);
    WANStatus* status = &context->statuses[context->numStatuses++];
    HAPRawBufferZero(status, sizeof *status);
    HAPPrecondition(value);
    HAPPrecondition(shouldContinue);
    HAPPrecondition(*shouldContinue);

    status->wanIdentifier = value->wanIdentifier;

    if (value->linkStatus & kHAPCharacteristicValue_WANStatusList_Status_LinkStatus_Unknown) {
        status->linkStatus |= kHAPPlatformWiFiRouterWANStatus_Unknown;
    }
    if (value->linkStatus & kHAPCharacteristicValue_WANStatusList_Status_LinkStatus_NoCableConnected) {
        status->linkStatus |= kHAPPlatformWiFiRouterWANStatus_NoCableConnected;
    }
    if (value->linkStatus & kHAPCharacteristicValue_WANStatusList_Status_LinkStatus_NoIPAddress) {
        status->linkStatus |= kHAPPlatformWiFiRouterWANStatus_NoIPAddress;
    }
    if (value->linkStatus & kHAPCharacteristicValue_WANStatusList_Status_LinkStatus_NoGatewaySpecified) {
        status->linkStatus |= kHAPPlatformWiFiRouterWANStatus_NoGatewaySpecified;
    }
    if (value->linkStatus & kHAPCharacteristicValue_WANStatusList_Status_LinkStatus_GatewayUnreachable) {
        status->linkStatus |= kHAPPlatformWiFiRouterWANStatus_GatewayUnreachable;
    }
    if (value->linkStatus & kHAPCharacteristicValue_WANStatusList_Status_LinkStatus_NoDNSServerSpecified) {
        status->linkStatus |= kHAPPlatformWiFiRouterWANStatus_NoDNSServerSpecified;
    }
    if (value->linkStatus & kHAPCharacteristicValue_WANStatusList_Status_LinkStatus_DNSServerUnreachable) {
        status->linkStatus |= kHAPPlatformWiFiRouterWANStatus_DNSServerUnreachable;
    }
    if (value->linkStatus & kHAPCharacteristicValue_WANStatusList_Status_LinkStatus_AuthenticationFailed) {
        status->linkStatus |= kHAPPlatformWiFiRouterWANStatus_AuthenticationFailed;
    }
    if (value->linkStatus & kHAPCharacteristicValue_WANStatusList_Status_LinkStatus_Walled) {
        status->linkStatus |= kHAPPlatformWiFiRouterWANStatus_Walled;
    }
}

static void addFirewalWANConfigurationRule() {
    HAPError err;

    // In this example the firewall configuration of a network client profile is updated.
    HAPPlatformWiFiRouterClientIdentifier clientIdentifier = 1;
    HAPPrecondition(platform.ip.wiFiRouter);

    // Acquire exclusive (read-write) access to the network configuration.
    err = HAPPlatformWiFiRouterAcquireExclusiveConfigurationAccess(HAPNonnull(platform.ip.wiFiRouter));
    if (err) {
        HAPLogError(&kHAPLog_Default, "HAPPlatformWiFiRouterAcquireExclusiveConfigurationAccess failed - err %d", err);
        HAPAssert(err == kHAPError_Busy);
        // Handle error.
    }
    bool exists;
    err = HAPPlatformWiFiRouterClientExists(HAPNonnull(platform.ip.wiFiRouter), clientIdentifier, &exists);
    if (err) {
        HAPLogError(&kHAPLog_Default, "HAPPlatformWiFiRouterClientExists failed - err %d", err);
        HAPAssert(err == kHAPError_Unknown);
        // Handle error, ensuring that exclusive configuration access is released.
    }
    if (exists) {
        // Begin network configuration change.
        err = HAPPlatformWiFiRouterBeginConfigurationChange(HAPNonnull(platform.ip.wiFiRouter));
        if (err) {
            HAPLogError(&kHAPLog_Default, "HAPPlatformWiFiRouterBeginConfigurationChange failed - err %d", err);
            HAPAssert(err == kHAPError_Unknown);
            // Handle error, ensuring that exclusive configuration access is released.
        }
        // Reset WAN firewall rules.
        err = HAPPlatformWiFiRouterClientResetWANFirewall(
                HAPNonnull(platform.ip.wiFiRouter), clientIdentifier, kHAPPlatformWiFiRouterFirewallType_Allowlist);
        if (err) {
            HAPLogError(&kHAPLog_Default, "HAPPlatformWiFiRouterClientResetWANFirewall failed - err %d", err);
            HAPAssert(
                    err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                    err == kHAPError_OutOfResources);
            // Handle error, ensuring that the configuration change is rolled back,
            // and that exclusive configuration access is released.
        }

        err = HAPPlatformWiFiRouterClientAddWANFirewallRule(
                HAPNonnull(platform.ip.wiFiRouter),
                clientIdentifier,
                &(const HAPPlatformWiFiRouterPortWANFirewallRule) {
                        .type = kHAPPlatformWiFiRouterWANFirewallRuleType_Port,
                        .transportProtocol = kHAPPlatformWiFiRouterTransportProtocol_TCP,
                        .host = { .type = kHAPPlatformWiFiRouterWANHostURIType_DNSNamePattern,
                                  ._.dnsNamePattern = "amazonaws.com" },
                        .hostPortRange = { .startPort = 443, .endPort = 443 } });
        if (err) {
            HAPLogError(&kHAPLog_Default, "HAPPlatformWiFiRouterClientAddWANFirewallRule failed - err %d", err);
            HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidData || err == kHAPError_OutOfResources);
            // Handle error, ensuring that the configuration change is rolled back,
            // and that exclusive configuration access is released.
        }

        err = HAPPlatformWiFiRouterClientAddWANFirewallRule(
                HAPNonnull(platform.ip.wiFiRouter),
                clientIdentifier,
                &(const HAPPlatformWiFiRouterPortWANFirewallRule) {
                        .type = kHAPPlatformWiFiRouterWANFirewallRuleType_Port,
                        .transportProtocol = kHAPPlatformWiFiRouterTransportProtocol_TCP,
                        .host = { .type = kHAPPlatformWiFiRouterWANHostURIType_IPAddress,
                                  ._.ipAddress = kHAPIPAddress_IPv4Any } });
        if (err) {
            HAPLogError(&kHAPLog_Default, "HAPPlatformWiFiRouterClientAddWANFirewallRule failed - err %d", err);
            HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidData || err == kHAPError_OutOfResources);
            // Handle error, ensuring that the configuration change is rolled back,
            // and that exclusive configuration access is released.
        }

        // Finalize adding WAN firewall rules.
        err = HAPPlatformWiFiRouterClientFinalizeWANFirewallRules(HAPNonnull(platform.ip.wiFiRouter), clientIdentifier);
        if (err) {
            HAPLogError(&kHAPLog_Default, "HAPPlatformWiFiRouterClientFinalizeWANFirewallRules failed - err %d", err);
            HAPAssert(err == kHAPError_Unknown || err == kHAPError_OutOfResources);
            // Handle error, ensuring that the configuration change is rolled back,
            // and that exclusive configuration access is released.
        }
    }

    err = HAPPlatformWiFiRouterCommitConfigurationChange(HAPNonnull(platform.ip.wiFiRouter));
    if (err) {
        HAPLogError(&kHAPLog_Default, "HAPPlatformWiFiRouterCommitConfigurationChange failed - err %d", err);
        HAPAssert(err == kHAPError_Unknown);
        // Handle error, ensuring that the configuration change is rolled back,
        // and that exclusive configuration access is released.
    }
    HAPPlatformWiFiRouterReleaseExclusiveConfigurationAccess(HAPNonnull(platform.ip.wiFiRouter));
}

/**
 * Run scenario 1: Add router.
 */
static void Scenario1(HAPAccessoryServer* server, HAPSession* session) {
    HAPPrecondition(server);
    HAPPrecondition(session);

    HAPError err;

    // Read Supported Router Configuration.
    bool supportsManagedNetwork = false;
    bool supportsMACAddressCredentials = false;
    bool supportsPSKCredentials = false;
    {
        const HAPTLV8CharacteristicReadRequest request = {
            .transportType = kHAPTransportType_IP,
            .session = session,
            .characteristic = &wiFiRouterSupportedRouterConfigurationCharacteristic,
            .service = &wiFiRouterService,
            .accessory = &accessory
        };
        uint8_t bytes[1024];
        HAPTLVWriter responseWriter;
        HAPTLVWriterCreate(&responseWriter, bytes, sizeof bytes);
        err = request.characteristic->callbacks.handleRead(server, &request, &responseWriter, NULL);
        HAPAssert(!err);
        HAPAssert(HAPPlatformWiFiRouterIsConfigurationAccessAvailable(HAPNonnull(platform.ip.wiFiRouter)));
        void* responseBytes;
        size_t numResponseBytes;
        HAPTLVWriterGetBuffer(&responseWriter, &responseBytes, &numResponseBytes);
        HAPLogCharacteristicBufferDebug(
                &kHAPLog_Default,
                request.characteristic,
                request.service,
                request.accessory,
                responseBytes,
                numResponseBytes,
                "<");
        HAPTLVReader responseReader;
        HAPTLVReaderCreate(&responseReader, responseBytes, numResponseBytes);

        HAPCharacteristicValue_SupportedRouterConfiguration value;
        err = HAPTLVReaderDecode(&responseReader, &kHAPCharacteristicTLVFormat_SupportedRouterConfiguration, &value);
        HAPAssert(!err);

        if (value.flags & kHAPCharacteristicValue_SupportedRouterConfiguration_Flags_SupportsManagedNetwork) {
            supportsManagedNetwork = true;
        }
        if (value.flags & kHAPCharacteristicValue_SupportedRouterConfiguration_Flags_SupportsMACAddressCredentials) {
            supportsMACAddressCredentials = true;
        }
        if (value.flags & kHAPCharacteristicValue_SupportedRouterConfiguration_Flags_SupportsPSKCredentials) {
            supportsPSKCredentials = true;
        }

        HAPLogCharacteristicInfo(
                &kHAPLog_Default,
                request.characteristic,
                request.service,
                request.accessory,
                "Supported Router Configuration:"
                "\n- Supports Managed Network: %s"
                "\n- Supports MAC address based credentials: %s"
                "\n- Supports PSK based credentials: %s",
                supportsManagedNetwork ? "Yes" : "No",
                supportsMACAddressCredentials ? "Yes" : "No",
                supportsPSKCredentials ? "Yes" : "No");
    }
    HAPAssert(supportsManagedNetwork);
    HAPAssert(supportsMACAddressCredentials);
    HAPAssert(supportsPSKCredentials);

    // Read WAN configuration.
    size_t numWANs = 0;
    WANConfiguration wans[32];
    HAPRawBufferZero(wans, sizeof wans);
    {
        const HAPTLV8CharacteristicReadRequest request = { .transportType = kHAPTransportType_IP,
                                                           .session = session,
                                                           .characteristic =
                                                                   &wiFiRouterWANConfigurationListCharacteristic,
                                                           .service = &wiFiRouterService,
                                                           .accessory = &accessory };
        uint8_t bytes[1024];
        HAPTLVWriter responseWriter;
        HAPTLVWriterCreate(&responseWriter, bytes, sizeof bytes);
        err = request.characteristic->callbacks.handleRead(server, &request, &responseWriter, NULL);
        HAPAssert(!err);
        HAPAssert(HAPPlatformWiFiRouterIsConfigurationAccessAvailable(HAPNonnull(platform.ip.wiFiRouter)));
        void* responseBytes;
        size_t numResponseBytes;
        HAPTLVWriterGetBuffer(&responseWriter, &responseBytes, &numResponseBytes);
        HAPLogCharacteristicBufferDebug(
                &kHAPLog_Default,
                request.characteristic,
                request.service,
                request.accessory,
                responseBytes,
                numResponseBytes,
                "<");
        HAPTLVReader responseReader;
        HAPTLVReaderCreate(&responseReader, responseBytes, numResponseBytes);

        HAPCharacteristicValue_WANConfigurationList value;
        err = HAPTLVReaderDecode(&responseReader, &kHAPCharacteristicTLVFormat_WANConfigurationList, &value);
        HAPAssert(!err);

        EnumerateWANConfigurationsContext enumerateContext;
        HAPRawBufferZero(&enumerateContext, sizeof enumerateContext);
        enumerateContext.configs = wans;
        enumerateContext.maxConfigs = HAPArrayCount(wans);
        err = value.enumerate(&value.dataSource, EnumerateWANConfigurationsCallback, &enumerateContext);
        HAPAssert(!err);
        numWANs = enumerateContext.numConfigs;

        {
            char logBytes[2 * 1024];
            HAPStringBuilder stringBuilder;
            HAPStringBuilderCreate(&stringBuilder, logBytes, sizeof logBytes);
            HAPStringBuilderAppend(&stringBuilder, "WAN Configuration List:");
            for (size_t i = 0; i < numWANs; i++) {
                HAPStringBuilderAppend(&stringBuilder, "\n- WAN Configuration (%zu):", i);
                HAPStringBuilderAppend(&stringBuilder, "\n  - WAN Identifier: %lu", (unsigned long) wans[i].identifier);
                HAPStringBuilderAppend(&stringBuilder, "\n  - WAN Type: ");
                switch (wans[i].wanType) {
                    case kHAPPlatformWiFiRouterWANType_Unconfigured: {
                        HAPStringBuilderAppend(&stringBuilder, "Unconfigured");
                        break;
                    }
                    case kHAPPlatformWiFiRouterWANType_Other: {
                        HAPStringBuilderAppend(&stringBuilder, "Other");
                        break;
                    }
                    case kHAPPlatformWiFiRouterWANType_DHCP: {
                        HAPStringBuilderAppend(&stringBuilder, "DHCP");
                        break;
                    }
                    case kHAPPlatformWiFiRouterWANType_BridgeMode: {
                        HAPStringBuilderAppend(&stringBuilder, "Bridge Mode");
                        break;
                    }
                }
            }
            HAPAssert(!HAPStringBuilderDidOverflow(&stringBuilder));
            HAPLogCharacteristicInfo(
                    &kHAPLog_Default,
                    request.characteristic,
                    request.service,
                    request.accessory,
                    "%s",
                    HAPStringBuilderGetString(&stringBuilder));
        }
    }
    bool hasMainWAN = false;
    for (size_t i = 0; i < numWANs; i++) {
        for (size_t j = 0; j < i; j++) {
            HAPAssert(wans[i].identifier != wans[j].identifier);
        }
        switch (wans[i].identifier) {
            case kHAPCharacteristicValue_WiFiRouter_WANIdentifier_Main: {
                HAPAssert(!hasMainWAN);
                hasMainWAN = true;
                break;
            }
            default: {
                HAPAssert(wans[i].identifier >= 1024);
                break;
            }
        }
        HAPAssert(wans[i].wanType != kHAPPlatformWiFiRouterWANType_Unconfigured);
        HAPAssert(wans[i].wanType != kHAPPlatformWiFiRouterWANType_BridgeMode);
    }

    // Read WAN Status.
    size_t numWANStatuses = 0;
    WANStatus wanStatuses[32];
    HAPRawBufferZero(wanStatuses, sizeof wanStatuses);
    {
        const HAPTLV8CharacteristicReadRequest request = { .transportType = kHAPTransportType_IP,
                                                           .session = session,
                                                           .characteristic = &wiFiRouterWANStatusListCharacteristic,
                                                           .service = &wiFiRouterService,
                                                           .accessory = &accessory };
        uint8_t bytes[1024];
        HAPTLVWriter responseWriter;
        HAPTLVWriterCreate(&responseWriter, bytes, sizeof bytes);
        err = request.characteristic->callbacks.handleRead(server, &request, &responseWriter, NULL);
        HAPAssert(!err);
        HAPAssert(HAPPlatformWiFiRouterIsConfigurationAccessAvailable(HAPNonnull(platform.ip.wiFiRouter)));
        void* responseBytes;
        size_t numResponseBytes;
        HAPTLVWriterGetBuffer(&responseWriter, &responseBytes, &numResponseBytes);
        HAPLogCharacteristicBufferDebug(
                &kHAPLog_Default,
                request.characteristic,
                request.service,
                request.accessory,
                responseBytes,
                numResponseBytes,
                "<");
        HAPTLVReader responseReader;
        HAPTLVReaderCreate(&responseReader, responseBytes, numResponseBytes);

        HAPCharacteristicValue_WANStatusList value;
        err = HAPTLVReaderDecode(&responseReader, &kHAPCharacteristicTLVFormat_WANStatusList, &value);
        HAPAssert(!err);

        EnumerateWANStatusesContext enumerateContext;
        HAPRawBufferZero(&enumerateContext, sizeof enumerateContext);
        enumerateContext.statuses = wanStatuses;
        enumerateContext.maxStatuses = HAPArrayCount(wanStatuses);
        err = value.enumerate(&value.dataSource, EnumerateWANStatusesCallback, &enumerateContext);
        HAPAssert(!err);
        numWANStatuses = numWANs;

        {
            char logBytes[2 * 1024];
            HAPStringBuilder stringBuilder;
            HAPStringBuilderCreate(&stringBuilder, logBytes, sizeof logBytes);
            HAPStringBuilderAppend(&stringBuilder, "WAN Status List:");
            for (size_t i = 0; i < numWANStatuses; i++) {
                HAPStringBuilderAppend(&stringBuilder, "\n- WAN Status (%zu):", i);
                HAPStringBuilderAppend(
                        &stringBuilder, "\n  - WAN Identifier: %lu", (unsigned long) wanStatuses[i].wanIdentifier);
                HAPStringBuilderAppend(&stringBuilder, "\n  - WAN Status: ");
                if (!wanStatuses[i].linkStatus) {
                    HAPStringBuilderAppend(&stringBuilder, "Online");
                } else {
                    HAPStringBuilderAppend(&stringBuilder, "[");
                    bool needsSeparator = false;
                    if (wanStatuses[i].linkStatus & kHAPPlatformWiFiRouterWANStatus_Unknown) {
                        HAPStringBuilderAppend(&stringBuilder, "Unknown");
                        needsSeparator = true;
                    }
                    if (wanStatuses[i].linkStatus & kHAPPlatformWiFiRouterWANStatus_NoCableConnected) {
                        if (needsSeparator) {
                            HAPStringBuilderAppend(&stringBuilder, ", ");
                        }
                        HAPStringBuilderAppend(&stringBuilder, "No cable connected");
                        needsSeparator = true;
                    }
                    if (wanStatuses[i].linkStatus & kHAPPlatformWiFiRouterWANStatus_NoCableConnected) {
                        if (needsSeparator) {
                            HAPStringBuilderAppend(&stringBuilder, ", ");
                        }
                        HAPStringBuilderAppend(&stringBuilder, "No cable connected");
                        needsSeparator = true;
                    }
                    if (wanStatuses[i].linkStatus & kHAPPlatformWiFiRouterWANStatus_NoIPAddress) {
                        if (needsSeparator) {
                            HAPStringBuilderAppend(&stringBuilder, ", ");
                        }
                        HAPStringBuilderAppend(&stringBuilder, "No IP address");
                        needsSeparator = true;
                    }
                    if (wanStatuses[i].linkStatus & kHAPPlatformWiFiRouterWANStatus_NoGatewaySpecified) {
                        if (needsSeparator) {
                            HAPStringBuilderAppend(&stringBuilder, ", ");
                        }
                        HAPStringBuilderAppend(&stringBuilder, "No gateway specified");
                        needsSeparator = true;
                    }
                    if (wanStatuses[i].linkStatus & kHAPPlatformWiFiRouterWANStatus_GatewayUnreachable) {
                        if (needsSeparator) {
                            HAPStringBuilderAppend(&stringBuilder, ", ");
                        }
                        HAPStringBuilderAppend(&stringBuilder, "Gateway unreachable");
                        needsSeparator = true;
                    }
                    if (wanStatuses[i].linkStatus & kHAPPlatformWiFiRouterWANStatus_NoDNSServerSpecified) {
                        if (needsSeparator) {
                            HAPStringBuilderAppend(&stringBuilder, ", ");
                        }
                        HAPStringBuilderAppend(&stringBuilder, "No DNS server(s) specified");
                        needsSeparator = true;
                    }
                    if (wanStatuses[i].linkStatus & kHAPPlatformWiFiRouterWANStatus_DNSServerUnreachable) {
                        if (needsSeparator) {
                            HAPStringBuilderAppend(&stringBuilder, ", ");
                        }
                        HAPStringBuilderAppend(&stringBuilder, "DNS server(s) unreachable");
                        needsSeparator = true;
                    }
                    if (wanStatuses[i].linkStatus & kHAPPlatformWiFiRouterWANStatus_AuthenticationFailed) {
                        if (needsSeparator) {
                            HAPStringBuilderAppend(&stringBuilder, ", ");
                        }
                        HAPStringBuilderAppend(&stringBuilder, "Authentication failed");
                        needsSeparator = true;
                    }
                    if (wanStatuses[i].linkStatus & kHAPPlatformWiFiRouterWANStatus_Walled) {
                        if (needsSeparator) {
                            HAPStringBuilderAppend(&stringBuilder, ", ");
                        }
                        HAPStringBuilderAppend(&stringBuilder, "Walled");
                    }
                    HAPStringBuilderAppend(&stringBuilder, "]");
                }
            }
            HAPAssert(!HAPStringBuilderDidOverflow(&stringBuilder));
            HAPLogCharacteristicInfo(
                    &kHAPLog_Default,
                    request.characteristic,
                    request.service,
                    request.accessory,
                    "%s",
                    HAPStringBuilderGetString(&stringBuilder));
        }
    }
    HAPAssert(numWANStatuses == numWANs);
    for (size_t i = 0; i < numWANStatuses; i++) {
        for (size_t j = 0; j < i; j++) {
            HAPAssert(wanStatuses[i].wanIdentifier != wanStatuses[j].wanIdentifier);
        }
        bool found = false;
        for (size_t j = 0; j < numWANStatuses; j++) {
            if (wanStatuses[j].wanIdentifier == wans[i].identifier) {
                found = true;
                break;
            }
        }
        HAPAssert(found);
    }
    for (size_t i = 0; i < numWANStatuses; i++) {
        HAPAssert(!wanStatuses[i].linkStatus);
    }

    // Enable HomeKit managed network.
    bool isManagedNetworkEnabled = false;
    {
        const HAPUInt8CharacteristicWriteRequest request = { .transportType = kHAPTransportType_IP,
                                                             .session = session,
                                                             .characteristic =
                                                                     &wiFiRouterManagedNetworkEnableCharacteristic,
                                                             .service = &wiFiRouterService,
                                                             .accessory = &accessory,
                                                             .remote = false,
                                                             .authorizationData = { .bytes = NULL, .numBytes = 0 } };
        uint8_t value = kHAPCharacteristicValue_ManagedNetworkEnable_Enabled;
        HAPLogCharacteristicDebug(
                &kHAPLog_Default, request.characteristic, request.service, request.accessory, "> %u", value);
        err = request.characteristic->callbacks.handleWrite(server, &request, value, NULL);
        HAPAssert(!err);
        HAPAssert(HAPPlatformWiFiRouterIsConfigurationAccessAvailable(HAPNonnull(platform.ip.wiFiRouter)));
    }
    {
        const HAPUInt8CharacteristicReadRequest request = { .transportType = kHAPTransportType_IP,
                                                            .session = session,
                                                            .characteristic =
                                                                    &wiFiRouterManagedNetworkEnableCharacteristic,
                                                            .service = &wiFiRouterService,
                                                            .accessory = &accessory };
        uint8_t value;
        err = request.characteristic->callbacks.handleRead(server, &request, &value, NULL);
        HAPAssert(!err);
        HAPAssert(HAPPlatformWiFiRouterIsConfigurationAccessAvailable(HAPNonnull(platform.ip.wiFiRouter)));
        HAPLogCharacteristicDebug(
                &kHAPLog_Default, request.characteristic, request.service, request.accessory, "< %u", value);

        switch (value) {
            case kHAPCharacteristicValue_ManagedNetworkEnable_Disabled: {
                isManagedNetworkEnabled = false;
                break;
            }
            case kHAPCharacteristicValue_ManagedNetworkEnable_Enabled: {
                isManagedNetworkEnabled = true;
                break;
            }
            default:
                HAPAssertionFailure();
        }

        HAPLogCharacteristicInfo(
                &kHAPLog_Default,
                request.characteristic,
                request.service,
                request.accessory,
                "Managed Network Enable: %s",
                isManagedNetworkEnabled ? "Enabled" : "Disabled");
    }
    HAPAssert(isManagedNetworkEnabled);

    // Read Router Status.
    bool isReady = false;
    {
        const HAPUInt8CharacteristicReadRequest request = { .transportType = kHAPTransportType_IP,
                                                            .session = session,
                                                            .characteristic = &wiFiRouterRouterStatusCharacteristic,
                                                            .service = &wiFiRouterService,
                                                            .accessory = &accessory };
        uint8_t value;
        err = request.characteristic->callbacks.handleRead(server, &request, &value, NULL);
        HAPAssert(!err);
        HAPAssert(HAPPlatformWiFiRouterIsConfigurationAccessAvailable(HAPNonnull(platform.ip.wiFiRouter)));
        HAPLogCharacteristicDebug(
                &kHAPLog_Default, request.characteristic, request.service, request.accessory, "< %u", value);

        switch (value) {
            case kHAPCharacteristicValue_RouterStatus_Ready: {
                isReady = true;
                break;
            }
            case kHAPCharacteristicValue_RouterStatus_NotReady: {
                isReady = false;
                break;
            }
            default:
                HAPAssertionFailure();
        }

        HAPLogCharacteristicInfo(
                &kHAPLog_Default,
                request.characteristic,
                request.service,
                request.accessory,
                "Router Status: %s",
                isReady ? "Ready" : "Not Ready");
    }
    HAPAssert(isReady);
}

//----------------------------------------------------------------------------------------------------------------------

typedef struct {
    HAPIPAddress* ipAddresses;
    size_t maxIPAddresses;
    size_t* numIPAddresses;
} EnumerateIPAddressesContext;

static void EnumerateIPAddressesCallback(
        void* _Nullable context_,
        HAPCharacteristicValue_WiFiRouter_IPAddress* value,
        bool* shouldContinue) {
    HAPPrecondition(context_);
    EnumerateIPAddressesContext* context = context_;
    HAPPrecondition(context->ipAddresses);
    HAPIPAddress* ipAddresses = context->ipAddresses;
    size_t maxIPAddresses = context->maxIPAddresses;
    HAPPrecondition(context->numIPAddresses);
    size_t* numIPAddresses = context->numIPAddresses;
    HAPPrecondition(value);
    HAPPrecondition(shouldContinue);
    HAPPrecondition(*shouldContinue);

    *numIPAddresses = 0;

    HAPAssert(*numIPAddresses < maxIPAddresses);
    HAPAssert(!(value->ipv4AddressIsSet && value->ipv6AddressIsSet));
    if (value->ipv4AddressIsSet) {
        ipAddresses[*numIPAddresses].version = kHAPIPAddressVersion_IPv4;
        HAPRawBufferCopyBytes(
                &ipAddresses[*numIPAddresses]._.ipv4, &value->ipv4Address, sizeof ipAddresses[*numIPAddresses]._.ipv4);
    } else {
        HAPAssert(value->ipv6AddressIsSet);
        ipAddresses[*numIPAddresses].version = kHAPIPAddressVersion_IPv6;
        HAPRawBufferCopyBytes(
                &ipAddresses[*numIPAddresses]._.ipv6, &value->ipv4Address, sizeof ipAddresses[*numIPAddresses]._.ipv6);
    }
    HAPRawBufferCopyBytes(&ipAddresses[*numIPAddresses], value, sizeof ipAddresses[*numIPAddresses]);
    (*numIPAddresses)++;
}

typedef struct {
    ClientStatus* statuses;
    size_t maxStatuses;
    size_t numStatuses;
} EnumerateClientStatusesContext;

static void EnumerateClientStatusesCallback(
        void* _Nullable context_,
        HAPCharacteristicValue_NetworkClientStatusControl_Status* value,
        bool* shouldContinue) {
    HAPPrecondition(context_);
    EnumerateClientStatusesContext* context = context_;
    HAPPrecondition(context->statuses);
    HAPPrecondition(context->numStatuses < context->maxStatuses);
    ClientStatus* status = &context->statuses[context->numStatuses++];
    HAPRawBufferZero(status, sizeof *status);
    HAPPrecondition(value);
    HAPPrecondition(shouldContinue);
    HAPPrecondition(*shouldContinue);

    HAPError err;

    if (value->clientIsSet) {
        status->clientIdentifier = value->clientIdentifier;
    }

    HAPRawBufferCopyBytes(&status->macAddress, &value->macAddress, sizeof status->macAddress);

    if (value->ipAddressListIsSet) {
        EnumerateIPAddressesContext enumerateContext;
        HAPRawBufferZero(&enumerateContext, sizeof enumerateContext);
        enumerateContext.ipAddresses = status->ipAddresses;
        enumerateContext.maxIPAddresses = HAPArrayCount(status->ipAddresses);
        enumerateContext.numIPAddresses = &status->numIPAddresses;
        err = value->ipAddressList.enumerate(
                &value->ipAddressList.dataSource, EnumerateIPAddressesCallback, &enumerateContext);
        HAPAssert(!err);
    }

    if (value->nameIsSet) {
        size_t numValueBytes = HAPStringGetNumBytes(value->name);
        HAPAssert(numValueBytes < sizeof status->name);
        HAPRawBufferCopyBytes(status->name, value->name, numValueBytes + 1);
    }

    status->rssi.isDefined = value->rssiIsSet;
    if (value->rssiIsSet) {
        status->rssi.value = (HAPRSSI) value->rssi;
    }
}

static void ReadNetworkClientStatus(
        HAPAccessoryServer* server,
        HAPSession* session,
        ClientStatus* statuses,
        size_t maxStatuses,
        size_t* numStatuses) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(statuses);
    HAPPrecondition(numStatuses);

    HAPError err;

    *numStatuses = 0;

    {
        const HAPTLV8CharacteristicReadRequest request = { .transportType = kHAPTransportType_IP,
                                                           .session = session,
                                                           .characteristic =
                                                                   &wiFiRouterNetworkClientStatusControlCharacteristic,
                                                           .service = &wiFiRouterService,
                                                           .accessory = &accessory };
        uint8_t bytes[1024];
        HAPTLVWriter responseWriter;
        HAPTLVWriterCreate(&responseWriter, bytes, sizeof bytes);
        err = request.characteristic->callbacks.handleRead(server, &request, &responseWriter, NULL);
        HAPAssert(!err);
        HAPAssert(HAPPlatformWiFiRouterIsConfigurationAccessAvailable(HAPNonnull(platform.ip.wiFiRouter)));
        void* responseBytes;
        size_t numResponseBytes;
        HAPTLVWriterGetBuffer(&responseWriter, &responseBytes, &numResponseBytes);
        HAPLogCharacteristicBufferDebug(
                &kHAPLog_Default,
                request.characteristic,
                request.service,
                request.accessory,
                responseBytes,
                numResponseBytes,
                "<");
        HAPTLVReader responseReader;
        HAPTLVReaderCreate(&responseReader, responseBytes, numResponseBytes);

        HAPCharacteristicValue_NetworkClientStatusControl_Response value;
        err = HAPTLVReaderDecode(
                &responseReader, &kHAPCharacteristicTLVFormat_NetworkClientStatusControl_Response, &value);
        HAPAssert(!err);

        EnumerateClientStatusesContext enumerateContext;
        HAPRawBufferZero(&enumerateContext, sizeof enumerateContext);
        enumerateContext.statuses = statuses;
        enumerateContext.maxStatuses = maxStatuses;
        err = value.enumerate(&value.dataSource, EnumerateClientStatusesCallback, &enumerateContext);
        HAPAssert(!err);
        *numStatuses = enumerateContext.numStatuses;

        {
            char logBytes[2 * 1024];
            HAPStringBuilder stringBuilder;
            HAPStringBuilderCreate(&stringBuilder, logBytes, sizeof logBytes);
            HAPStringBuilderAppend(&stringBuilder, "Network Client Status List:");
            for (size_t i = 0; i < *numStatuses; i++) {
                HAPStringBuilderAppend(&stringBuilder, "\n- Network Client Status (%zu):", i);
                if (statuses[i].clientIdentifier) {
                    HAPStringBuilderAppend(
                            &stringBuilder,
                            "\n  - Network Client Profile Identifier: %lu",
                            (unsigned long) statuses[i].clientIdentifier);
                }
                HAPStringBuilderAppend(
                        &stringBuilder,
                        "\n  - MAC Address: %02X:%02X:%02X:%02X:%02X:%02X",
                        statuses[i].macAddress.bytes[0],
                        statuses[i].macAddress.bytes[1],
                        statuses[i].macAddress.bytes[2],
                        statuses[i].macAddress.bytes[3],
                        statuses[i].macAddress.bytes[4],
                        statuses[i].macAddress.bytes[5]);
                HAPStringBuilderAppend(&stringBuilder, "\n  - IP Address List:");
                for (size_t j = 0; j < statuses[i].numIPAddresses; j++) {
                    HAPStringBuilderAppend(&stringBuilder, "\n    - IP Address (%zu):", j);
                    switch (statuses[i].ipAddresses[j].version) {
                        case kHAPIPAddressVersion_IPv4: {
                            HAPStringBuilderAppend(
                                    &stringBuilder,
                                    "\n      - IPv4 Address: %u.%u.%u.%u",
                                    statuses[i].ipAddresses[j]._.ipv4.bytes[0],
                                    statuses[i].ipAddresses[j]._.ipv4.bytes[1],
                                    statuses[i].ipAddresses[j]._.ipv4.bytes[2],
                                    statuses[i].ipAddresses[j]._.ipv4.bytes[3]);
                            break;
                        }
                        case kHAPIPAddressVersion_IPv6: {
                            HAPStringBuilderAppend(
                                    &stringBuilder,
                                    "\n      - IPv6 Address: %04X:%04X:%04X:%04X:%04X:%04X:%04X:%04X",
                                    HAPReadBigUInt16(&statuses[i].ipAddresses[j]._.ipv6.bytes[0]),
                                    HAPReadBigUInt16(&statuses[i].ipAddresses[j]._.ipv6.bytes[1]),
                                    HAPReadBigUInt16(&statuses[i].ipAddresses[j]._.ipv6.bytes[2]),
                                    HAPReadBigUInt16(&statuses[i].ipAddresses[j]._.ipv6.bytes[3]),
                                    HAPReadBigUInt16(&statuses[i].ipAddresses[j]._.ipv6.bytes[4]),
                                    HAPReadBigUInt16(&statuses[i].ipAddresses[j]._.ipv6.bytes[5]),
                                    HAPReadBigUInt16(&statuses[i].ipAddresses[j]._.ipv6.bytes[6]),
                                    HAPReadBigUInt16(&statuses[i].ipAddresses[j]._.ipv6.bytes[7]));
                            break;
                        }
                    }
                }
                if (HAPStringGetNumBytes(statuses[i].name)) {
                    HAPStringBuilderAppend(&stringBuilder, "\n  - Name: %s", statuses[i].name);
                }
                if (statuses[i].rssi.isDefined) {
                    HAPStringBuilderAppend(&stringBuilder, "\n  - RSSI: %d", statuses[i].rssi.value);
                }
            }
            HAPAssert(!HAPStringBuilderDidOverflow(&stringBuilder));
            HAPLogCharacteristicInfo(
                    &kHAPLog_Default,
                    request.characteristic,
                    request.service,
                    request.accessory,
                    "%s",
                    HAPStringBuilderGetString(&stringBuilder));
        }
    }
}

static void GetClientStatusForIPAddress(
        HAPAccessoryServer* server,
        HAPSession* session,
        const HAPIPAddress* ipAddress,
        ClientStatus* status) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(ipAddress);
    HAPPrecondition(status);

    HAPError err;

    HAPRawBufferZero(status, sizeof *status);

    {
        uint8_t requestBytes[1024];
        HAPTLVWriter requestWriter;
        HAPTLVWriterCreate(&requestWriter, requestBytes, sizeof requestBytes);

        // Operation Type.
        uint8_t operationTypeBytes[] = { kHAPCharacteristicValue_NetworkClientStatusControl_OperationType_Read };
        err = HAPTLVWriterAppend(
                &requestWriter,
                &(const HAPTLV) { .type = kHAPCharacteristicTLVType_NetworkClientStatusControl_OperationType,
                                  .value = { .bytes = operationTypeBytes, .numBytes = sizeof operationTypeBytes } });
        HAPAssert(!err);

        // Network Client Status Identifier List.
        {
            HAPTLVWriter subWriter;
            {
                void* bytes;
                size_t maxBytes;
                HAPTLVWriterGetScratchBytes(&requestWriter, &bytes, &maxBytes);
                HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
            }

            // Network Client Status Identifier.
            {
                HAPTLVWriter sub2Writer;
                {
                    void* bytes;
                    size_t maxBytes;
                    HAPTLVWriterGetScratchBytes(&subWriter, &bytes, &maxBytes);
                    HAPTLVWriterCreate(&sub2Writer, bytes, maxBytes);
                }

                // IP Address.
                {
                    HAPTLVWriter sub3Writer;
                    {
                        void* bytes;
                        size_t maxBytes;
                        HAPTLVWriterGetScratchBytes(&sub2Writer, &bytes, &maxBytes);
                        HAPTLVWriterCreate(&sub3Writer, bytes, maxBytes);
                    }

                    switch (ipAddress->version) {
                        case kHAPIPAddressVersion_IPv4: {
                            err = HAPTLVWriterAppend(
                                    &sub3Writer,
                                    &(const HAPTLV) {
                                            .type = kHAPCharacteristicTLVType_WiFiRouter_IPAddress_IPv4Address,
                                            .value = { .bytes = ipAddress->_.ipv4.bytes,
                                                       .numBytes = sizeof ipAddress->_.ipv4.bytes } });
                            HAPAssert(!err);
                            break;
                        }
                        case kHAPIPAddressVersion_IPv6: {
                            err = HAPTLVWriterAppend(
                                    &sub3Writer,
                                    &(const HAPTLV) {
                                            .type = kHAPCharacteristicTLVType_WiFiRouter_IPAddress_IPv6Address,
                                            .value = { .bytes = ipAddress->_.ipv6.bytes,
                                                       .numBytes = sizeof ipAddress->_.ipv6.bytes } });
                            HAPAssert(!err);
                            break;
                        }
                    }

                    // Finalize.
                    void* bytes;
                    size_t numBytes;
                    HAPTLVWriterGetBuffer(&sub3Writer, &bytes, &numBytes);
                    err = HAPTLVWriterAppend(
                            &sub2Writer,
                            &(const HAPTLV) {
                                    .type = kHAPCharacteristicValueType_NetworkClientStatusControl_Identifier_IPAddress,
                                    .value = { .bytes = bytes, .numBytes = numBytes } });
                    HAPAssert(!err);
                }

                // Finalize.
                void* bytes;
                size_t numBytes;
                HAPTLVWriterGetBuffer(&sub2Writer, &bytes, &numBytes);
                err = HAPTLVWriterAppend(
                        &subWriter,
                        &(const HAPTLV) {
                                .type = kHAPCharacteristicTLVType_NetworkClientStatusControl_IdentifierList_Identifier,
                                .value = { .bytes = bytes, .numBytes = numBytes } });
                HAPAssert(!err);
            }

            // Finalize.
            void* bytes;
            size_t numBytes;
            HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
            err = HAPTLVWriterAppend(
                    &requestWriter,
                    &(const HAPTLV) { .type = kHAPCharacteristicTLVType_NetworkClientStatusControl_IdentifierList,
                                      .value = { .bytes = bytes, .numBytes = numBytes } });
            HAPAssert(!err);
        }

        const HAPTLV8CharacteristicWriteRequest request = { .transportType = kHAPTransportType_IP,
                                                            .session = session,
                                                            .characteristic =
                                                                    &wiFiRouterNetworkClientStatusControlCharacteristic,
                                                            .service = &wiFiRouterService,
                                                            .accessory = &accessory,
                                                            .remote = false,
                                                            .authorizationData = { .bytes = NULL, .numBytes = 0 } };
        void* bytes;
        size_t numBytes;
        HAPTLVWriterGetBuffer(&requestWriter, &bytes, &numBytes);
        HAPLogCharacteristicBufferDebug(
                &kHAPLog_Default, request.characteristic, request.service, request.accessory, bytes, numBytes, ">");
        HAPTLVReader requestReader;
        HAPTLVReaderCreate(&requestReader, bytes, numBytes);
        err = request.characteristic->callbacks.handleWrite(server, &request, &requestReader, NULL);
        HAPAssert(!err);
    }
    size_t numStatuses;
    ReadNetworkClientStatus(server, session, status, 1, &numStatuses);
    HAPAssert(numStatuses == 1);
}

HAP_RESULT_USE_CHECK
static bool IsClientConnected(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        const HAPMACAddress* macAddress) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(clientIdentifier);
    HAPPrecondition(macAddress);

    HAPError err;

    size_t numStatuses = 0;
    ClientStatus statuses[64];
    HAPRawBufferZero(statuses, sizeof statuses);
    {
        uint8_t requestBytes[1024];
        HAPTLVWriter requestWriter;
        HAPTLVWriterCreate(&requestWriter, requestBytes, sizeof requestBytes);

        // Operation Type.
        uint8_t operationTypeBytes[] = { kHAPCharacteristicValue_NetworkClientStatusControl_OperationType_Read };
        err = HAPTLVWriterAppend(
                &requestWriter,
                &(const HAPTLV) { .type = kHAPCharacteristicTLVType_NetworkClientStatusControl_OperationType,
                                  .value = { .bytes = operationTypeBytes, .numBytes = sizeof operationTypeBytes } });
        HAPAssert(!err);

        // Network Client Status Identifier List.
        {
            HAPTLVWriter subWriter;
            {
                void* bytes;
                size_t maxBytes;
                HAPTLVWriterGetScratchBytes(&requestWriter, &bytes, &maxBytes);
                HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
            }

            // Network Client Status Identifier.
            {
                HAPTLVWriter sub2Writer;
                {
                    void* bytes;
                    size_t maxBytes;
                    HAPTLVWriterGetScratchBytes(&subWriter, &bytes, &maxBytes);
                    HAPTLVWriterCreate(&sub2Writer, bytes, maxBytes);
                }

                // Network Client Profile Identifier.
                uint8_t networkClientIdentifierBytes[] = { HAPExpandLittleUInt32(clientIdentifier) };
                err = HAPTLVWriterAppend(
                        &sub2Writer,
                        &(const HAPTLV) {
                                .type = kHAPCharacteristicValueType_NetworkClientStatusControl_Identifier_Client,
                                .value = { .bytes = networkClientIdentifierBytes,
                                           .numBytes = sizeof networkClientIdentifierBytes } });
                HAPAssert(!err);

                // Finalize.
                void* bytes;
                size_t numBytes;
                HAPTLVWriterGetBuffer(&sub2Writer, &bytes, &numBytes);
                err = HAPTLVWriterAppend(
                        &subWriter,
                        &(const HAPTLV) {
                                .type = kHAPCharacteristicTLVType_NetworkClientStatusControl_IdentifierList_Identifier,
                                .value = { .bytes = bytes, .numBytes = numBytes } });
                HAPAssert(!err);
            }

            // Finalize.
            void* bytes;
            size_t numBytes;
            HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
            err = HAPTLVWriterAppend(
                    &requestWriter,
                    &(const HAPTLV) { .type = kHAPCharacteristicTLVType_NetworkClientStatusControl_IdentifierList,
                                      .value = { .bytes = bytes, .numBytes = numBytes } });
            HAPAssert(!err);
        }

        const HAPTLV8CharacteristicWriteRequest request = { .transportType = kHAPTransportType_IP,
                                                            .session = session,
                                                            .characteristic =
                                                                    &wiFiRouterNetworkClientStatusControlCharacteristic,
                                                            .service = &wiFiRouterService,
                                                            .accessory = &accessory,
                                                            .remote = false,
                                                            .authorizationData = { .bytes = NULL, .numBytes = 0 } };
        void* bytes;
        size_t numBytes;
        HAPTLVWriterGetBuffer(&requestWriter, &bytes, &numBytes);
        HAPLogCharacteristicBufferDebug(
                &kHAPLog_Default, request.characteristic, request.service, request.accessory, bytes, numBytes, ">");
        HAPTLVReader requestReader;
        HAPTLVReaderCreate(&requestReader, bytes, numBytes);
        err = request.characteristic->callbacks.handleWrite(server, &request, &requestReader, NULL);
        HAPAssert(!err);
    }
    ReadNetworkClientStatus(server, session, statuses, HAPArrayCount(statuses), &numStatuses);
    bool found = false;
    for (size_t i = 0; i < numStatuses; i++) {
        HAPAssert(statuses[i].clientIdentifier == clientIdentifier);
        HAPAssert(!found);
        found = true;
        HAPAssert(HAPRawBufferAreEqual(statuses[i].macAddress.bytes, macAddress->bytes, sizeof macAddress->bytes));
        HAPAssert(statuses[i].numIPAddresses);
    }
    return found;
}

typedef struct {
    ClientConfiguration* configs;
    size_t maxConfigs;
    size_t numConfigs;
} EnumerateClientOperationResponsesContext;

static void EnumerateListClientOperationResponsesCallback(
        void* _Nullable context_,
        HAPCharacteristicValue_NetworkClientProfileControl_OperationRsp* value,
        bool* shouldContinue) {
    HAPPrecondition(context_);
    EnumerateClientOperationResponsesContext* context = context_;
    HAPPrecondition(context->configs);
    HAPPrecondition(context->numConfigs < context->maxConfigs);
    ClientConfiguration* config = &context->configs[context->numConfigs++];
    HAPRawBufferZero(config, sizeof *config);
    HAPPrecondition(value);
    HAPPrecondition(shouldContinue);
    HAPPrecondition(*shouldContinue);

    HAPAssert(!value->status);
    HAPAssert(value->configIsSet);

    HAPAssert(value->config.clientIsSet);
    config->identifier = value->config.clientIdentifier;

    HAPAssert(!value->config.groupIsSet);
    HAPAssert(!value->config.credentialIsSet);
    HAPAssert(!value->config.wanFirewallIsSet);
    HAPAssert(!value->config.lanFirewallIsSet);
}

static void EnumerateReadClientOperationResponsesCallback(
        void* _Nullable context_,
        HAPCharacteristicValue_NetworkClientProfileControl_OperationRsp* value,
        bool* shouldContinue) {
    HAPPrecondition(context_);
    EnumerateClientOperationResponsesContext* context = context_;
    HAPPrecondition(context->configs);
    HAPPrecondition(context->numConfigs < context->maxConfigs);
    ClientConfiguration* config = &context->configs[context->numConfigs++];
    HAPRawBufferZero(config, sizeof *config);
    HAPPrecondition(value);
    HAPPrecondition(shouldContinue);
    HAPPrecondition(*shouldContinue);

    HAPAssert(!value->status);
    HAPAssert(value->configIsSet);

    HAPAssert(value->config.clientIsSet);
    config->identifier = value->config.clientIdentifier;

    HAPAssert(value->config.groupIsSet);
    config->groupIdentifier = value->config.groupIdentifier;

    HAPAssert(value->config.credentialIsSet);
    switch (value->config.credential.type) {
        case kHAPCharacteristicValueType_WiFiRouter_Credential_MACAddress: {
            config->credential.type = kHAPPlatformWiFiRouterCredentialType_MACAddress;
            HAPRawBufferCopyBytes(
                    &config->credential._.macAddress,
                    &value->config.credential._.macAddress,
                    sizeof config->credential._.macAddress);
            break;
        }
        case kHAPCharacteristicValueType_WiFiRouter_Credential_PSK: {
            HAPAssert(HAPStringAreEqual(value->config.credential._.psk, ""));
            config->credential.type = kHAPPlatformWiFiRouterCredentialType_PSK;
            break;
        }
    }

    HAPAssert(value->config.wanFirewallIsSet);
    ParseWANFirewallConfiguration(
            &value->config.wanFirewall,
            &config->wanFirewall.type,
            config->wanFirewall.rules,
            HAPArrayCount(config->wanFirewall.rules),
            &config->wanFirewall.numRules);

    HAPAssert(value->config.lanFirewallIsSet);
    ParseLANFirewallConfiguration(
            &value->config.lanFirewall,
            &config->lanFirewall.type,
            config->lanFirewall.rules,
            HAPArrayCount(config->lanFirewall.rules),
            &config->lanFirewall.numRules);
}

static void EnumerateAddClientOperationResponsesCallback(
        void* _Nullable context_,
        HAPCharacteristicValue_NetworkClientProfileControl_OperationRsp* value,
        bool* shouldContinue) {
    HAPPrecondition(context_);
    EnumerateClientOperationResponsesContext* context = context_;
    HAPPrecondition(context->configs);
    HAPPrecondition(context->numConfigs < context->maxConfigs);
    ClientConfiguration* config = &context->configs[context->numConfigs++];
    HAPRawBufferZero(config, sizeof *config);
    HAPPrecondition(value);
    HAPPrecondition(shouldContinue);
    HAPPrecondition(*shouldContinue);

    HAPAssert(!value->status);
    HAPAssert(value->configIsSet);

    HAPAssert(value->config.clientIsSet);
    config->identifier = value->config.clientIdentifier;

    HAPAssert(!value->config.groupIsSet);
    HAPAssert(!value->config.credentialIsSet);
    HAPAssert(!value->config.wanFirewallIsSet);
    HAPAssert(!value->config.lanFirewallIsSet);
}

static void EnumerateUpdateAndRemoveClientOperationResponsesCallback(
        void* _Nullable context_,
        HAPCharacteristicValue_NetworkClientProfileControl_OperationRsp* value,
        bool* shouldContinue) {
    HAPPrecondition(context_);
    EnumerateClientOperationResponsesContext* context = context_;
    HAPPrecondition(context->configs);
    HAPPrecondition(context->numConfigs < context->maxConfigs);
    ClientConfiguration* config = &context->configs[context->numConfigs++];
    HAPRawBufferZero(config, sizeof *config);
    HAPPrecondition(value);
    HAPPrecondition(shouldContinue);
    HAPPrecondition(*shouldContinue);

    HAPAssert(!value->status);
    HAPAssert(!value->configIsSet);
}

/**
 * Run scenario 2: Add Network Client Profile with MAC credentials.
 */
static void Scenario2(HAPAccessoryServer* server, HAPSession* session) {
    HAPPrecondition(server);
    HAPPrecondition(session);

    HAPError err;

    // Network client information.
    HAPPlatformWiFiRouterGroupIdentifier groupIdentifier =
            kHAPCharacteristicValue_WiFiRouter_GroupIdentifier_Restricted;
    HAPMACAddress macAddress;
    HAPPlatformRandomNumberFill(macAddress.bytes, sizeof macAddress.bytes);
    char name[64];
    {
        uint8_t nameIdentifier[sizeof(uint64_t)];
        HAPPlatformRandomNumberFill(nameIdentifier, sizeof nameIdentifier);
        err = HAPStringWithFormat(
                name, sizeof name, "Network Client %016llX.", (unsigned long long) HAPReadLittleUInt64(nameIdentifier));
        HAPAssert(!err);
    }
    HAPPlatformWiFiRouterClientIdentifier clientIDA;
    HAPIPAddress clientIPAddressA;

    // Connect network client.
    err = HAPPlatformWiFiRouterConnectClient(
            HAPNonnull(platform.ip.wiFiRouter), NULL, &macAddress, name, &clientIPAddressA);
    HAPAssert(!err);

    // Read Network Client Status.
    ClientStatus clientStatusA;
    GetClientStatusForIPAddress(server, session, &clientIPAddressA, &clientStatusA);
    HAPAssert(HAPMACAddressAreEqual(&clientStatusA.macAddress, &macAddress));

    // Add Network Client Profile (MAC Address).
    size_t numOperations = 0;
    ClientConfiguration operations[1];
    HAPRawBufferZero(operations, sizeof operations);
    {
        uint8_t requestBytes[1024];
        HAPTLVWriter requestWriter;
        HAPTLVWriterCreate(&requestWriter, requestBytes, sizeof requestBytes);

        // Network Client Profile Control Operation.
        {
            HAPTLVWriter subWriter;
            {
                void* bytes;
                size_t maxBytes;
                HAPTLVWriterGetScratchBytes(&requestWriter, &bytes, &maxBytes);
                HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
            }

            // Operation Type.
            uint8_t operationTypeBytes[] = { kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_Add };
            err = HAPTLVWriterAppend(
                    &subWriter,
                    &(const HAPTLV) {
                            .type = kHAPCharacteristicTLVType_NetworkClientProfileControl_Operation_Type,
                            .value = { .bytes = operationTypeBytes, .numBytes = sizeof operationTypeBytes } });
            HAPAssert(!err);

            // Network Client Profile Configuration.
            {
                HAPTLVWriter sub2Writer;
                {
                    void* bytes;
                    size_t maxBytes;
                    HAPTLVWriterGetScratchBytes(&subWriter, &bytes, &maxBytes);
                    HAPTLVWriterCreate(&sub2Writer, bytes, maxBytes);
                }

                // Client Group Identifier.
                uint8_t groupIdentifierBytes[] = { HAPExpandLittleUInt32(groupIdentifier) };
                err = HAPTLVWriterAppend(
                        &sub2Writer,
                        &(const HAPTLV) {
                                .type = kHAPCharacteristicTLVType_NetworkClientProfileControl_Config_Group,
                                .value = { .bytes = groupIdentifierBytes, .numBytes = sizeof groupIdentifierBytes } });
                HAPAssert(!err);

                // Credential Data.
                {
                    HAPTLVWriter sub3Writer;
                    {
                        void* bytes;
                        size_t maxBytes;
                        HAPTLVWriterGetScratchBytes(&sub2Writer, &bytes, &maxBytes);
                        HAPTLVWriterCreate(&sub3Writer, bytes, maxBytes);
                    }

                    // MAC Address Credential.
                    err = HAPTLVWriterAppend(
                            &sub3Writer,
                            &(const HAPTLV) { .type = kHAPCharacteristicValueType_WiFiRouter_Credential_MACAddress,
                                              .value = { .bytes = clientStatusA.macAddress.bytes,
                                                         .numBytes = sizeof clientStatusA.macAddress.bytes } });
                    HAPAssert(!err);

                    // Finalize.
                    void* bytes;
                    size_t numBytes;
                    HAPTLVWriterGetBuffer(&sub3Writer, &bytes, &numBytes);
                    err = HAPTLVWriterAppend(
                            &sub2Writer,
                            &(const HAPTLV) {
                                    .type = kHAPCharacteristicTLVType_NetworkClientProfileControl_Config_Credential,
                                    .value = { .bytes = bytes, .numBytes = numBytes } });
                    HAPAssert(!err);
                }

                // WAN Firewall Configuration.
                {
                    HAPTLVWriter sub3Writer;
                    {
                        void* bytes;
                        size_t maxBytes;
                        HAPTLVWriterGetScratchBytes(&sub2Writer, &bytes, &maxBytes);
                        HAPTLVWriterCreate(&sub3Writer, bytes, maxBytes);
                    }

                    // WAN Firewall Type.
                    uint8_t wanFirewallTypeBytes[] = { kHAPCharacteristicValue_WiFiRouter_FirewallType_Allowlist };
                    err = HAPTLVWriterAppend(
                            &sub3Writer,
                            &(const HAPTLV) { .type = kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_Type,
                                              .value = { .bytes = wanFirewallTypeBytes,
                                                         .numBytes = sizeof wanFirewallTypeBytes } });
                    HAPAssert(!err);

                    // WAN Firewall Rule List.
                    {
                        HAPTLVWriter sub4Writer;
                        {
                            void* bytes;
                            size_t maxBytes;
                            HAPTLVWriterGetScratchBytes(&sub3Writer, &bytes, &maxBytes);
                            HAPTLVWriterCreate(&sub4Writer, bytes, maxBytes);
                        }

                        // Finalize.
                        void* bytes;
                        size_t numBytes;
                        HAPTLVWriterGetBuffer(&sub4Writer, &bytes, &numBytes);
                        err = HAPTLVWriterAppend(
                                &sub3Writer,
                                &(const HAPTLV) { .type = kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_RuleList,
                                                  .value = { .bytes = bytes, .numBytes = numBytes } });
                        HAPAssert(!err);
                    }

                    // Finalize.
                    void* bytes;
                    size_t numBytes;
                    HAPTLVWriterGetBuffer(&sub3Writer, &bytes, &numBytes);
                    err = HAPTLVWriterAppend(
                            &sub2Writer,
                            &(const HAPTLV) {
                                    .type = kHAPCharacteristicTLVType_NetworkClientProfileControl_Config_WANFirewall,
                                    .value = { .bytes = bytes, .numBytes = numBytes } });
                    HAPAssert(!err);
                }

                // LAN Firewall Configuration.
                {
                    HAPTLVWriter sub3Writer;
                    {
                        void* bytes;
                        size_t maxBytes;
                        HAPTLVWriterGetScratchBytes(&sub2Writer, &bytes, &maxBytes);
                        HAPTLVWriterCreate(&sub3Writer, bytes, maxBytes);
                    }

                    // LAN Firewall Type.
                    uint8_t wanFirewallTypeBytes[] = { kHAPCharacteristicValue_WiFiRouter_FirewallType_Allowlist };
                    err = HAPTLVWriterAppend(
                            &sub3Writer,
                            &(const HAPTLV) { .type = kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_Type,
                                              .value = { .bytes = wanFirewallTypeBytes,
                                                         .numBytes = sizeof wanFirewallTypeBytes } });
                    HAPAssert(!err);

                    // LAN Firewall Rule List.
                    {
                        HAPTLVWriter sub4Writer;
                        {
                            void* bytes;
                            size_t maxBytes;
                            HAPTLVWriterGetScratchBytes(&sub3Writer, &bytes, &maxBytes);
                            HAPTLVWriterCreate(&sub4Writer, bytes, maxBytes);
                        }

                        // Finalize.
                        void* bytes;
                        size_t numBytes;
                        HAPTLVWriterGetBuffer(&sub4Writer, &bytes, &numBytes);
                        err = HAPTLVWriterAppend(
                                &sub3Writer,
                                &(const HAPTLV) { .type = kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_RuleList,
                                                  .value = { .bytes = bytes, .numBytes = numBytes } });
                        HAPAssert(!err);
                    }

                    // Finalize.
                    void* bytes;
                    size_t numBytes;
                    HAPTLVWriterGetBuffer(&sub3Writer, &bytes, &numBytes);
                    err = HAPTLVWriterAppend(
                            &sub2Writer,
                            &(const HAPTLV) {
                                    .type = kHAPCharacteristicTLVType_NetworkClientProfileControl_Config_LANFirewall,
                                    .value = { .bytes = bytes, .numBytes = numBytes } });
                    HAPAssert(!err);
                }

                // Finalize.
                void* bytes;
                size_t numBytes;
                HAPTLVWriterGetBuffer(&sub2Writer, &bytes, &numBytes);
                err = HAPTLVWriterAppend(
                        &subWriter,
                        &(const HAPTLV) {
                                .type = kHAPCharacteristicTLVType_NetworkClientProfileControl_Operation_Config,
                                .value = { .bytes = bytes, .numBytes = numBytes } });
                HAPAssert(!err);
            }

            // Finalize.
            void* bytes;
            size_t numBytes;
            HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
            err = HAPTLVWriterAppend(
                    &requestWriter,
                    &(const HAPTLV) { .type = kHAPCharacteristicTLVType_NetworkClientProfileControl_Operation,
                                      .value = { .bytes = bytes, .numBytes = numBytes } });
            HAPAssert(!err);
        }

        const HAPTLV8CharacteristicWriteRequest request = {
            .transportType = kHAPTransportType_IP,
            .session = session,
            .characteristic = &wiFiRouterNetworkClientProfileControlCharacteristic,
            .service = &wiFiRouterService,
            .accessory = &accessory,
            .remote = false,
            .authorizationData = { .bytes = NULL, .numBytes = 0 }
        };
        void* bytes;
        size_t numBytes;
        HAPTLVWriterGetBuffer(&requestWriter, &bytes, &numBytes);
        HAPLogCharacteristicBufferDebug(
                &kHAPLog_Default, request.characteristic, request.service, request.accessory, bytes, numBytes, ">");
        HAPTLVReader requestReader;
        HAPTLVReaderCreate(&requestReader, bytes, numBytes);
        err = request.characteristic->callbacks.handleWrite(server, &request, &requestReader, NULL);
        HAPAssert(!err);
    }
    {
        const HAPTLV8CharacteristicReadRequest request = { .transportType = kHAPTransportType_IP,
                                                           .session = session,
                                                           .characteristic =
                                                                   &wiFiRouterNetworkClientProfileControlCharacteristic,
                                                           .service = &wiFiRouterService,
                                                           .accessory = &accessory };
        uint8_t bytes[1024];
        HAPTLVWriter responseWriter;
        HAPTLVWriterCreate(&responseWriter, bytes, sizeof bytes);
        err = request.characteristic->callbacks.handleRead(server, &request, &responseWriter, NULL);
        HAPAssert(!err);
        HAPAssert(HAPPlatformWiFiRouterIsConfigurationAccessAvailable(HAPNonnull(platform.ip.wiFiRouter)));
        void* responseBytes;
        size_t numResponseBytes;
        HAPTLVWriterGetBuffer(&responseWriter, &responseBytes, &numResponseBytes);
        HAPLogCharacteristicBufferDebug(
                &kHAPLog_Default,
                request.characteristic,
                request.service,
                request.accessory,
                responseBytes,
                numResponseBytes,
                "<");
        HAPTLVReader responseReader;
        HAPTLVReaderCreate(&responseReader, responseBytes, numResponseBytes);

        HAPCharacteristicValue_NetworkClientProfileControl_Response value;
        err = HAPTLVReaderDecode(
                &responseReader, &kHAPCharacteristicTLVFormat_NetworkClientProfileControl_Response, &value);
        HAPAssert(!err);

        EnumerateClientOperationResponsesContext enumerateContext;
        HAPRawBufferZero(&enumerateContext, sizeof enumerateContext);
        enumerateContext.configs = operations;
        enumerateContext.maxConfigs = HAPArrayCount(operations);
        err = value.enumerate(&value.dataSource, EnumerateAddClientOperationResponsesCallback, &enumerateContext);
        HAPAssert(!err);
        numOperations = enumerateContext.numConfigs;

        {
            char logBytes[2 * 1024];
            HAPStringBuilder stringBuilder;
            HAPStringBuilderCreate(&stringBuilder, logBytes, sizeof logBytes);
            HAPStringBuilderAppend(&stringBuilder, "Network Client Profile Control Response");
            for (size_t i = 0; i < numOperations; i++) {
                HAPStringBuilderAppend(
                        &stringBuilder, "\n- Network Client Profile Control Operation Response (%zu):", i);
                HAPStringBuilderAppend(
                        &stringBuilder, "\n  - Status: %u", kHAPCharacteristicValue_WiFiRouter_OperationStatus_Success);
                HAPStringBuilderAppend(&stringBuilder, "\n  - Network Client Profile Configuration:");
                HAPStringBuilderAppend(
                        &stringBuilder,
                        "\n    - Network Client Profile Identifier: %lu",
                        (unsigned long) operations[i].identifier);
            }
            HAPAssert(!HAPStringBuilderDidOverflow(&stringBuilder));
            HAPLogCharacteristicInfo(
                    &kHAPLog_Default,
                    request.characteristic,
                    request.service,
                    request.accessory,
                    "%s",
                    HAPStringBuilderGetString(&stringBuilder));
        }
    }
    HAPAssert(numOperations == 1);
    HAPAssert(operations[0].identifier == _nextExpectedClientIdentifier);
    _nextExpectedClientIdentifier++;
    clientIDA = operations[0].identifier;

    // Join Wi-Fi with default-psk.
    err = HAPPlatformWiFiRouterConnectClient(
            HAPNonnull(platform.ip.wiFiRouter),
            NULL,
            &macAddress,
            name,
            /* ipAddress: */ NULL);
    HAPAssert(!err);

    // Poll Network Client Status.
    HAPAssert(IsClientConnected(server, session, clientIDA, &macAddress));
}

//----------------------------------------------------------------------------------------------------------------------

typedef struct {
    char stringValue[64];
} ClientName;

/**
 * Run scenario 3: Add Network Client Profile with PSK credentials.
 */
static void Scenario3(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPPlatformWiFiRouterClientIdentifier* clientIDC,
        HAPWiFiWPAPersonalCredential* uniquePSKC,
        HAPMACAddress* clientMACAddressC,
        ClientName* clientNameC,
        HAPIPAddress* clientIPAddressC) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(clientIDC);
    HAPPrecondition(uniquePSKC);
    HAPPrecondition(clientMACAddressC);
    HAPPrecondition(clientNameC);
    HAPPrecondition(clientIPAddressC);

    HAPError err;

    // Network client information.
    HAPPlatformWiFiRouterGroupIdentifier groupIdentifier = kHAPCharacteristicValue_WiFiRouter_GroupIdentifier_Main;
    HAPPlatformRandomNumberFill(clientMACAddressC->bytes, sizeof clientMACAddressC->bytes);
    uniquePSKC->type = kHAPWiFiWPAPersonalCredentialType_PSK;
    HAPPlatformRandomNumberFill(uniquePSKC->_.psk.bytes, sizeof uniquePSKC->_.psk.bytes);
    {
        uint8_t nameIdentifier[sizeof(uint64_t)];
        HAPPlatformRandomNumberFill(nameIdentifier, sizeof nameIdentifier);
        err = HAPStringWithFormat(
                clientNameC->stringValue,
                sizeof clientNameC->stringValue,
                "Network Client %016llX.",
                (unsigned long long) HAPReadLittleUInt64(nameIdentifier));
        HAPAssert(!err);
    }

    // Add Network Client Profile (Main network client group and no firewall rules).
    size_t numOperations = 0;
    ClientConfiguration operations[1];
    HAPRawBufferZero(operations, sizeof operations);
    {
        uint8_t requestBytes[1024];
        HAPTLVWriter requestWriter;
        HAPTLVWriterCreate(&requestWriter, requestBytes, sizeof requestBytes);

        // Network Client Profile Control Operation.
        {
            HAPTLVWriter subWriter;
            {
                void* bytes;
                size_t maxBytes;
                HAPTLVWriterGetScratchBytes(&requestWriter, &bytes, &maxBytes);
                HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
            }

            // Operation Type.
            uint8_t operationTypeBytes[] = { kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_Add };
            err = HAPTLVWriterAppend(
                    &subWriter,
                    &(const HAPTLV) {
                            .type = kHAPCharacteristicTLVType_NetworkClientProfileControl_Operation_Type,
                            .value = { .bytes = operationTypeBytes, .numBytes = sizeof operationTypeBytes } });
            HAPAssert(!err);

            // Network Client Profile Configuration.
            {
                HAPTLVWriter sub2Writer;
                {
                    void* bytes;
                    size_t maxBytes;
                    HAPTLVWriterGetScratchBytes(&subWriter, &bytes, &maxBytes);
                    HAPTLVWriterCreate(&sub2Writer, bytes, maxBytes);
                }

                // Client Group Identifier.
                uint8_t groupIdentifierBytes[] = { HAPExpandLittleUInt32(groupIdentifier) };
                err = HAPTLVWriterAppend(
                        &sub2Writer,
                        &(const HAPTLV) {
                                .type = kHAPCharacteristicTLVType_NetworkClientProfileControl_Config_Group,
                                .value = { .bytes = groupIdentifierBytes, .numBytes = sizeof groupIdentifierBytes } });
                HAPAssert(!err);

                // Credential Data.
                {
                    HAPTLVWriter sub3Writer;
                    {
                        void* bytes;
                        size_t maxBytes;
                        HAPTLVWriterGetScratchBytes(&sub2Writer, &bytes, &maxBytes);
                        HAPTLVWriterCreate(&sub3Writer, bytes, maxBytes);
                    }

                    // PSK Credential Credential.
                    HAPAssert(uniquePSKC->type == kHAPWiFiWPAPersonalCredentialType_PSK);
                    char pskCredentialBytes[2 * kHAPWiFiWPAPSK_NumBytes + 1] = { 0 };
                    for (size_t i = 0; i < kHAPWiFiWPAPSK_NumBytes; i++) {
                        err = HAPStringWithFormat(
                                &pskCredentialBytes[2 * i],
                                sizeof pskCredentialBytes - 2 * i,
                                "%02X",
                                uniquePSKC->_.psk.bytes[i]);
                        HAPAssert(!err);
                    }
                    err = HAPTLVWriterAppend(
                            &sub3Writer,
                            &(const HAPTLV) { .type = kHAPCharacteristicValueType_WiFiRouter_Credential_PSK,
                                              .value = { .bytes = pskCredentialBytes,
                                                         .numBytes = HAPStringGetNumBytes(pskCredentialBytes) } });
                    HAPAssert(!err);

                    // Finalize.
                    void* bytes;
                    size_t numBytes;
                    HAPTLVWriterGetBuffer(&sub3Writer, &bytes, &numBytes);
                    err = HAPTLVWriterAppend(
                            &sub2Writer,
                            &(const HAPTLV) {
                                    .type = kHAPCharacteristicTLVType_NetworkClientProfileControl_Config_Credential,
                                    .value = { .bytes = bytes, .numBytes = numBytes } });
                    HAPAssert(!err);
                }

                // WAN Firewall Configuration.
                {
                    HAPTLVWriter sub3Writer;
                    {
                        void* bytes;
                        size_t maxBytes;
                        HAPTLVWriterGetScratchBytes(&sub2Writer, &bytes, &maxBytes);
                        HAPTLVWriterCreate(&sub3Writer, bytes, maxBytes);
                    }

                    // WAN Firewall Type.
                    uint8_t wanFirewallTypeBytes[] = { kHAPCharacteristicValue_WiFiRouter_FirewallType_Allowlist };
                    err = HAPTLVWriterAppend(
                            &sub3Writer,
                            &(const HAPTLV) { .type = kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_Type,
                                              .value = { .bytes = wanFirewallTypeBytes,
                                                         .numBytes = sizeof wanFirewallTypeBytes } });
                    HAPAssert(!err);

                    // WAN Firewall Rule List.
                    {
                        HAPTLVWriter sub4Writer;
                        {
                            void* bytes;
                            size_t maxBytes;
                            HAPTLVWriterGetScratchBytes(&sub3Writer, &bytes, &maxBytes);
                            HAPTLVWriterCreate(&sub4Writer, bytes, maxBytes);
                        }

                        // Finalize.
                        void* bytes;
                        size_t numBytes;
                        HAPTLVWriterGetBuffer(&sub4Writer, &bytes, &numBytes);
                        err = HAPTLVWriterAppend(
                                &sub3Writer,
                                &(const HAPTLV) { .type = kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_RuleList,
                                                  .value = { .bytes = bytes, .numBytes = numBytes } });
                        HAPAssert(!err);
                    }

                    // Finalize.
                    void* bytes;
                    size_t numBytes;
                    HAPTLVWriterGetBuffer(&sub3Writer, &bytes, &numBytes);
                    err = HAPTLVWriterAppend(
                            &sub2Writer,
                            &(const HAPTLV) {
                                    .type = kHAPCharacteristicTLVType_NetworkClientProfileControl_Config_WANFirewall,
                                    .value = { .bytes = bytes, .numBytes = numBytes } });
                    HAPAssert(!err);
                }

                // LAN Firewall Configuration.
                {
                    HAPTLVWriter sub3Writer;
                    {
                        void* bytes;
                        size_t maxBytes;
                        HAPTLVWriterGetScratchBytes(&sub2Writer, &bytes, &maxBytes);
                        HAPTLVWriterCreate(&sub3Writer, bytes, maxBytes);
                    }

                    // LAN Firewall Type.
                    uint8_t wanFirewallTypeBytes[] = { kHAPCharacteristicValue_WiFiRouter_FirewallType_Allowlist };
                    err = HAPTLVWriterAppend(
                            &sub3Writer,
                            &(const HAPTLV) { .type = kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_Type,
                                              .value = { .bytes = wanFirewallTypeBytes,
                                                         .numBytes = sizeof wanFirewallTypeBytes } });
                    HAPAssert(!err);

                    // LAN Firewall Rule List.
                    {
                        HAPTLVWriter sub4Writer;
                        {
                            void* bytes;
                            size_t maxBytes;
                            HAPTLVWriterGetScratchBytes(&sub3Writer, &bytes, &maxBytes);
                            HAPTLVWriterCreate(&sub4Writer, bytes, maxBytes);
                        }

                        // Finalize.
                        void* bytes;
                        size_t numBytes;
                        HAPTLVWriterGetBuffer(&sub4Writer, &bytes, &numBytes);
                        err = HAPTLVWriterAppend(
                                &sub3Writer,
                                &(const HAPTLV) { .type = kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_RuleList,
                                                  .value = { .bytes = bytes, .numBytes = numBytes } });
                        HAPAssert(!err);
                    }

                    // Finalize.
                    void* bytes;
                    size_t numBytes;
                    HAPTLVWriterGetBuffer(&sub3Writer, &bytes, &numBytes);
                    err = HAPTLVWriterAppend(
                            &sub2Writer,
                            &(const HAPTLV) {
                                    .type = kHAPCharacteristicTLVType_NetworkClientProfileControl_Config_LANFirewall,
                                    .value = { .bytes = bytes, .numBytes = numBytes } });
                    HAPAssert(!err);
                }

                // Finalize.
                void* bytes;
                size_t numBytes;
                HAPTLVWriterGetBuffer(&sub2Writer, &bytes, &numBytes);
                err = HAPTLVWriterAppend(
                        &subWriter,
                        &(const HAPTLV) {
                                .type = kHAPCharacteristicTLVType_NetworkClientProfileControl_Operation_Config,
                                .value = { .bytes = bytes, .numBytes = numBytes } });
                HAPAssert(!err);
            }

            // Finalize.
            void* bytes;
            size_t numBytes;
            HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
            err = HAPTLVWriterAppend(
                    &requestWriter,
                    &(const HAPTLV) { .type = kHAPCharacteristicTLVType_NetworkClientProfileControl_Operation,
                                      .value = { .bytes = bytes, .numBytes = numBytes } });
            HAPAssert(!err);
        }

        const HAPTLV8CharacteristicWriteRequest request = {
            .transportType = kHAPTransportType_IP,
            .session = session,
            .characteristic = &wiFiRouterNetworkClientProfileControlCharacteristic,
            .service = &wiFiRouterService,
            .accessory = &accessory,
            .remote = false,
            .authorizationData = { .bytes = NULL, .numBytes = 0 }
        };
        void* bytes;
        size_t numBytes;
        HAPTLVWriterGetBuffer(&requestWriter, &bytes, &numBytes);
        HAPLogCharacteristicBufferDebug(
                &kHAPLog_Default, request.characteristic, request.service, request.accessory, bytes, numBytes, ">");
        HAPTLVReader requestReader;
        HAPTLVReaderCreate(&requestReader, bytes, numBytes);
        err = request.characteristic->callbacks.handleWrite(server, &request, &requestReader, NULL);
        HAPAssert(!err);
    }
    {
        const HAPTLV8CharacteristicReadRequest request = { .transportType = kHAPTransportType_IP,
                                                           .session = session,
                                                           .characteristic =
                                                                   &wiFiRouterNetworkClientProfileControlCharacteristic,
                                                           .service = &wiFiRouterService,
                                                           .accessory = &accessory };
        uint8_t bytes[1024];
        HAPTLVWriter responseWriter;
        HAPTLVWriterCreate(&responseWriter, bytes, sizeof bytes);
        err = request.characteristic->callbacks.handleRead(server, &request, &responseWriter, NULL);
        HAPAssert(!err);
        HAPAssert(HAPPlatformWiFiRouterIsConfigurationAccessAvailable(HAPNonnull(platform.ip.wiFiRouter)));
        void* responseBytes;
        size_t numResponseBytes;
        HAPTLVWriterGetBuffer(&responseWriter, &responseBytes, &numResponseBytes);
        HAPLogCharacteristicBufferDebug(
                &kHAPLog_Default,
                request.characteristic,
                request.service,
                request.accessory,
                responseBytes,
                numResponseBytes,
                "<");
        HAPTLVReader responseReader;
        HAPTLVReaderCreate(&responseReader, responseBytes, numResponseBytes);

        HAPCharacteristicValue_NetworkClientProfileControl_Response value;
        err = HAPTLVReaderDecode(
                &responseReader, &kHAPCharacteristicTLVFormat_NetworkClientProfileControl_Response, &value);
        HAPAssert(!err);

        EnumerateClientOperationResponsesContext enumerateContext;
        HAPRawBufferZero(&enumerateContext, sizeof enumerateContext);
        enumerateContext.configs = operations;
        enumerateContext.maxConfigs = HAPArrayCount(operations);
        err = value.enumerate(&value.dataSource, EnumerateAddClientOperationResponsesCallback, &enumerateContext);
        HAPAssert(!err);
        numOperations = enumerateContext.numConfigs;

        {
            char logBytes[2 * 1024];
            HAPStringBuilder stringBuilder;
            HAPStringBuilderCreate(&stringBuilder, logBytes, sizeof logBytes);
            HAPStringBuilderAppend(&stringBuilder, "Network Client Profile Control Response");
            for (size_t i = 0; i < numOperations; i++) {
                HAPStringBuilderAppend(
                        &stringBuilder, "\n- Network Client Profile Control Operation Response (%zu):", i);
                HAPStringBuilderAppend(
                        &stringBuilder, "\n  - Status: %u", kHAPCharacteristicValue_WiFiRouter_OperationStatus_Success);
                HAPStringBuilderAppend(&stringBuilder, "\n  - Network Client Profile Configuration:");
                HAPStringBuilderAppend(
                        &stringBuilder,
                        "\n    - Network Client Profile Identifier: %lu",
                        (unsigned long) operations[i].identifier);
            }
            HAPAssert(!HAPStringBuilderDidOverflow(&stringBuilder));
            HAPLogCharacteristicInfo(
                    &kHAPLog_Default,
                    request.characteristic,
                    request.service,
                    request.accessory,
                    "%s",
                    HAPStringBuilderGetString(&stringBuilder));
        }
    }
    HAPAssert(numOperations == 1);
    HAPAssert(operations[0].identifier == _nextExpectedClientIdentifier);
    _nextExpectedClientIdentifier++;
    *clientIDC = operations[0].identifier;

    // Join Wi-Fi with unique-psk-c.
    err = HAPPlatformWiFiRouterConnectClient(
            HAPNonnull(platform.ip.wiFiRouter),
            uniquePSKC,
            clientMACAddressC,
            clientNameC->stringValue,
            clientIPAddressC);
    HAPAssert(!err);

    // Poll Network Client Status.
    HAPAssert(IsClientConnected(server, session, *clientIDC, clientMACAddressC));
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Run scenario 4: Update Network Client Profile.
 */
static void Scenario4(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPPlatformWiFiRouterClientIdentifier clientIDC,
        const HAPWiFiWPAPersonalCredential* uniquePSKC,
        const HAPMACAddress* clientMACAddressC,
        const ClientName* clientNameC,
        const HAPIPAddress* clientIPAddressC) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(clientIDC);
    HAPPrecondition(uniquePSKC);
    HAPPrecondition(clientMACAddressC);
    HAPPrecondition(clientNameC);
    HAPPrecondition(clientIPAddressC);

    HAPError err;

    // Network client information.
    HAPPlatformWiFiRouterGroupIdentifier groupIdentifier =
            kHAPCharacteristicValue_WiFiRouter_GroupIdentifier_Restricted;

    // Read Network Client Status.
    ClientStatus clientStatusC;
    GetClientStatusForIPAddress(server, session, clientIPAddressC, &clientStatusC);
    HAPAssert(clientStatusC.clientIdentifier == clientIDC);
    HAPAssert(HAPMACAddressAreEqual(&clientStatusC.macAddress, clientMACAddressC));

    // Update Network Client Profile (Restricted network client group and firewall rules).
    size_t numOperations = 0;
    ClientConfiguration operations[1];
    HAPRawBufferZero(operations, sizeof operations);
    {
        uint8_t requestBytes[1024];
        HAPTLVWriter requestWriter;
        HAPTLVWriterCreate(&requestWriter, requestBytes, sizeof requestBytes);

        // Network Client Profile Control Operation.
        {
            HAPTLVWriter subWriter;
            {
                void* bytes;
                size_t maxBytes;
                HAPTLVWriterGetScratchBytes(&requestWriter, &bytes, &maxBytes);
                HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
            }

            // Operation Type.
            uint8_t operationTypeBytes[] = {
                kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_Update
            };
            err = HAPTLVWriterAppend(
                    &subWriter,
                    &(const HAPTLV) {
                            .type = kHAPCharacteristicTLVType_NetworkClientProfileControl_Operation_Type,
                            .value = { .bytes = operationTypeBytes, .numBytes = sizeof operationTypeBytes } });
            HAPAssert(!err);

            // Network Client Profile Configuration.
            {
                HAPTLVWriter sub2Writer;
                {
                    void* bytes;
                    size_t maxBytes;
                    HAPTLVWriterGetScratchBytes(&subWriter, &bytes, &maxBytes);
                    HAPTLVWriterCreate(&sub2Writer, bytes, maxBytes);
                }

                // Network Client Profile Identifier.
                uint8_t networkClientIdentifierBytes[] = { HAPExpandLittleUInt32(clientIDC) };
                err = HAPTLVWriterAppend(
                        &sub2Writer,
                        &(const HAPTLV) {
                                .type = kHAPCharacteristicTLVType_NetworkClientProfileControl_Config_Identifier,
                                .value = { .bytes = networkClientIdentifierBytes,
                                           .numBytes = sizeof networkClientIdentifierBytes } });
                HAPAssert(!err);

                // Client Group Identifier.
                uint8_t groupIdentifierBytes[] = { HAPExpandLittleUInt32(groupIdentifier) };
                err = HAPTLVWriterAppend(
                        &sub2Writer,
                        &(const HAPTLV) {
                                .type = kHAPCharacteristicTLVType_NetworkClientProfileControl_Config_Group,
                                .value = { .bytes = groupIdentifierBytes, .numBytes = sizeof groupIdentifierBytes } });
                HAPAssert(!err);

                // WAN Firewall Configuration.
                {
                    HAPTLVWriter sub3Writer;
                    {
                        void* bytes;
                        size_t maxBytes;
                        HAPTLVWriterGetScratchBytes(&sub2Writer, &bytes, &maxBytes);
                        HAPTLVWriterCreate(&sub3Writer, bytes, maxBytes);
                    }

                    // WAN Firewall Type.
                    uint8_t wanFirewallTypeBytes[] = { kHAPCharacteristicValue_WiFiRouter_FirewallType_Allowlist };
                    err = HAPTLVWriterAppend(
                            &sub3Writer,
                            &(const HAPTLV) { .type = kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_Type,
                                              .value = { .bytes = wanFirewallTypeBytes,
                                                         .numBytes = sizeof wanFirewallTypeBytes } });
                    HAPAssert(!err);

                    // WAN Firewall Rule List.
                    {
                        HAPTLVWriter sub4Writer;
                        {
                            void* bytes;
                            size_t maxBytes;
                            HAPTLVWriterGetScratchBytes(&sub3Writer, &bytes, &maxBytes);
                            HAPTLVWriterCreate(&sub4Writer, bytes, maxBytes);
                        }

                        // WAN Port Rule.
                        {
                            HAPTLVWriter sub5Writer;
                            {
                                void* bytes;
                                size_t maxBytes;
                                HAPTLVWriterGetScratchBytes(&sub4Writer, &bytes, &maxBytes);
                                HAPTLVWriterCreate(&sub5Writer, bytes, maxBytes);
                            }

                            // Protocol.
                            uint8_t protocolBytes[] = {
                                kHAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule_Protocol_TCP
                            };
                            err = HAPTLVWriterAppend(
                                    &sub5Writer,
                                    &(const HAPTLV) {
                                            .type = kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_PortRule_Protocol,
                                            .value = { .bytes = protocolBytes, .numBytes = sizeof protocolBytes } });
                            HAPAssert(!err);

                            // Host DNS Name.
                            const char* hostDNSName = "*.apple.com";
                            err = HAPTLVWriterAppend(
                                    &sub5Writer,
                                    &(const HAPTLV) {
                                            .type = kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_PortRule_HostDNSName,
                                            .value = { .bytes = hostDNSName,
                                                       .numBytes = HAPStringGetNumBytes(hostDNSName) } });
                            HAPAssert(!err);

                            // Host Port Start.
                            uint8_t hostPortStartBytes[] = { HAPExpandLittleUInt16(443) };
                            err = HAPTLVWriterAppend(
                                    &sub5Writer,
                                    &(const HAPTLV) {
                                            .type = kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_PortRule_HostPortStart,
                                            .value = { .bytes = hostPortStartBytes,
                                                       .numBytes = sizeof hostPortStartBytes } });
                            HAPAssert(!err);

                            // Finalize.
                            void* bytes;
                            size_t numBytes;
                            HAPTLVWriterGetBuffer(&sub5Writer, &bytes, &numBytes);
                            err = HAPTLVWriterAppend(
                                    &sub4Writer,
                                    &(const HAPTLV) {
                                            .type = kHAPCharacteristicValueType_WiFiRouter_WANFirewall_Rule_Port,
                                            .value = { .bytes = bytes, .numBytes = numBytes } });
                            HAPAssert(!err);
                        }

                        // Separator.
                        err = HAPTLVWriterAppend(
                                &sub4Writer, &(const HAPTLV) { .type = 0, .value = { .bytes = NULL, .numBytes = 0 } });
                        HAPAssert(!err);

                        // WAN Port Rule.
                        {
                            HAPTLVWriter sub5Writer;
                            {
                                void* bytes;
                                size_t maxBytes;
                                HAPTLVWriterGetScratchBytes(&sub4Writer, &bytes, &maxBytes);
                                HAPTLVWriterCreate(&sub5Writer, bytes, maxBytes);
                            }

                            // Protocol.
                            uint8_t protocolBytes[] = {
                                kHAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule_Protocol_UDP
                            };
                            err = HAPTLVWriterAppend(
                                    &sub5Writer,
                                    &(const HAPTLV) {
                                            .type = kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_PortRule_Protocol,
                                            .value = { .bytes = protocolBytes, .numBytes = sizeof protocolBytes } });
                            HAPAssert(!err);

                            // Host IP Start.
                            {
                                HAPTLVWriter sub6Writer;
                                {
                                    void* bytes;
                                    size_t maxBytes;
                                    HAPTLVWriterGetScratchBytes(&sub5Writer, &bytes, &maxBytes);
                                    HAPTLVWriterCreate(&sub6Writer, bytes, maxBytes);
                                }

                                uint8_t ipv4AddressBytes[] = { 17, 0, 0, 0 };
                                err = HAPTLVWriterAppend(
                                        &sub6Writer,
                                        &(const HAPTLV) {
                                                .type = kHAPCharacteristicTLVType_WiFiRouter_IPAddress_IPv4Address,
                                                .value = { .bytes = ipv4AddressBytes,
                                                           .numBytes = sizeof ipv4AddressBytes } });
                                HAPAssert(!err);

                                // Finalize.
                                void* bytes;
                                size_t numBytes;
                                HAPTLVWriterGetBuffer(&sub6Writer, &bytes, &numBytes);
                                err = HAPTLVWriterAppend(
                                        &sub5Writer,
                                        &(const HAPTLV) {
                                                .type = kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_PortRule_HostIPStart,
                                                .value = { .bytes = bytes, .numBytes = numBytes } });
                                HAPAssert(!err);
                            }

                            // Host IP End.
                            {
                                HAPTLVWriter sub6Writer;
                                {
                                    void* bytes;
                                    size_t maxBytes;
                                    HAPTLVWriterGetScratchBytes(&sub5Writer, &bytes, &maxBytes);
                                    HAPTLVWriterCreate(&sub6Writer, bytes, maxBytes);
                                }

                                uint8_t ipv4AddressBytes[] = { 17, 255, 255, 255 };
                                err = HAPTLVWriterAppend(
                                        &sub6Writer,
                                        &(const HAPTLV) {
                                                .type = kHAPCharacteristicTLVType_WiFiRouter_IPAddress_IPv4Address,
                                                .value = { .bytes = ipv4AddressBytes,
                                                           .numBytes = sizeof ipv4AddressBytes } });
                                HAPAssert(!err);

                                // Finalize.
                                void* bytes;
                                size_t numBytes;
                                HAPTLVWriterGetBuffer(&sub6Writer, &bytes, &numBytes);
                                err = HAPTLVWriterAppend(
                                        &sub5Writer,
                                        &(const HAPTLV) {
                                                .type = kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_PortRule_HostIPEnd,
                                                .value = { .bytes = bytes, .numBytes = numBytes } });
                                HAPAssert(!err);
                            }

                            // Host Port Start.
                            uint8_t hostPortStartBytes[] = { HAPExpandLittleUInt16(49152) };
                            err = HAPTLVWriterAppend(
                                    &sub5Writer,
                                    &(const HAPTLV) {
                                            .type = kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_PortRule_HostPortStart,
                                            .value = { .bytes = hostPortStartBytes,
                                                       .numBytes = sizeof hostPortStartBytes } });
                            HAPAssert(!err);

                            // Host Port End.
                            uint8_t hostPortEndBytes[] = { HAPExpandLittleUInt16(65535) };
                            err = HAPTLVWriterAppend(
                                    &sub5Writer,
                                    &(const HAPTLV) {
                                            .type = kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_PortRule_HostPortEnd,
                                            .value = { .bytes = hostPortEndBytes,
                                                       .numBytes = sizeof hostPortEndBytes } });
                            HAPAssert(!err);

                            // Finalize.
                            void* bytes;
                            size_t numBytes;
                            HAPTLVWriterGetBuffer(&sub5Writer, &bytes, &numBytes);
                            err = HAPTLVWriterAppend(
                                    &sub4Writer,
                                    &(const HAPTLV) {
                                            .type = kHAPCharacteristicValueType_WiFiRouter_WANFirewall_Rule_Port,
                                            .value = { .bytes = bytes, .numBytes = numBytes } });
                            HAPAssert(!err);
                        }

                        // Separator.
                        err = HAPTLVWriterAppend(
                                &sub4Writer, &(const HAPTLV) { .type = 0, .value = { .bytes = NULL, .numBytes = 0 } });
                        HAPAssert(!err);

                        // WAN ICMP Rule.
                        {
                            HAPTLVWriter sub5Writer;
                            {
                                void* bytes;
                                size_t maxBytes;
                                HAPTLVWriterGetScratchBytes(&sub4Writer, &bytes, &maxBytes);
                                HAPTLVWriterCreate(&sub5Writer, bytes, maxBytes);
                            }

                            // Host IP Start.
                            {
                                HAPTLVWriter sub6Writer;
                                {
                                    void* bytes;
                                    size_t maxBytes;
                                    HAPTLVWriterGetScratchBytes(&sub5Writer, &bytes, &maxBytes);
                                    HAPTLVWriterCreate(&sub6Writer, bytes, maxBytes);
                                }

                                uint8_t ipv4AddressBytes[] = { 0, 0, 0, 0 };
                                err = HAPTLVWriterAppend(
                                        &sub6Writer,
                                        &(const HAPTLV) {
                                                .type = kHAPCharacteristicTLVType_WiFiRouter_IPAddress_IPv4Address,
                                                .value = { .bytes = ipv4AddressBytes,
                                                           .numBytes = sizeof ipv4AddressBytes } });
                                HAPAssert(!err);

                                uint8_t ipv6AddressBytes[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
                                err = HAPTLVWriterAppend(
                                        &sub6Writer,
                                        &(const HAPTLV) {
                                                .type = kHAPCharacteristicTLVType_WiFiRouter_IPAddress_IPv6Address,
                                                .value = { .bytes = ipv6AddressBytes,
                                                           .numBytes = sizeof ipv6AddressBytes } });
                                HAPAssert(!err);

                                // Finalize.
                                void* bytes;
                                size_t numBytes;
                                HAPTLVWriterGetBuffer(&sub6Writer, &bytes, &numBytes);
                                err = HAPTLVWriterAppend(
                                        &sub5Writer,
                                        &(const HAPTLV) {
                                                .type = kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_ICMPRule_HostIPStart,
                                                .value = { .bytes = bytes, .numBytes = numBytes } });
                                HAPAssert(!err);
                            }

                            // ICMP Type List.
                            {
                                HAPTLVWriter sub6Writer;
                                {
                                    void* bytes;
                                    size_t maxBytes;
                                    HAPTLVWriterGetScratchBytes(&sub5Writer, &bytes, &maxBytes);
                                    HAPTLVWriterCreate(&sub6Writer, bytes, maxBytes);
                                }

                                // ICMP Type.
                                {
                                    HAPTLVWriter sub7Writer;
                                    {
                                        void* bytes;
                                        size_t maxBytes;
                                        HAPTLVWriterGetScratchBytes(&sub6Writer, &bytes, &maxBytes);
                                        HAPTLVWriterCreate(&sub7Writer, bytes, maxBytes);
                                    }

                                    // ICMP Protocol.
                                    uint8_t icmpProtocolBytes[] = { 0 };
                                    err = HAPTLVWriterAppend(
                                            &sub7Writer,
                                            &(const HAPTLV) {
                                                    .type = kHAPCharacteristicTLVType_WiFiRouter_ICMPType_Protocol,
                                                    .value = { .bytes = icmpProtocolBytes,
                                                               .numBytes = sizeof icmpProtocolBytes } });
                                    HAPAssert(!err);

                                    // ICMP Type.
                                    uint8_t icmpTypeBytes[] = { 8 };
                                    err = HAPTLVWriterAppend(
                                            &sub7Writer,
                                            &(const HAPTLV) {
                                                    .type = kHAPCharacteristicTLVType_WiFiRouter_ICMPType_TypeValue,
                                                    .value = { .bytes = icmpTypeBytes,
                                                               .numBytes = sizeof icmpTypeBytes } });
                                    HAPAssert(!err);

                                    // Finalize.
                                    void* bytes;
                                    size_t numBytes;
                                    HAPTLVWriterGetBuffer(&sub7Writer, &bytes, &numBytes);
                                    err = HAPTLVWriterAppend(
                                            &sub6Writer,
                                            &(const HAPTLV) {
                                                    .type = kHAPCharacteristicRawTLVType_WiFiRouter_ICMPList_ICMPType,
                                                    .value = { .bytes = bytes, .numBytes = numBytes } });
                                    HAPAssert(!err);
                                }

                                // Separator.
                                err = HAPTLVWriterAppend(
                                        &sub6Writer,
                                        &(const HAPTLV) { .type = 0, .value = { .bytes = NULL, .numBytes = 0 } });
                                HAPAssert(!err);

                                // ICMP Type.
                                {
                                    HAPTLVWriter sub7Writer;
                                    {
                                        void* bytes;
                                        size_t maxBytes;
                                        HAPTLVWriterGetScratchBytes(&sub6Writer, &bytes, &maxBytes);
                                        HAPTLVWriterCreate(&sub7Writer, bytes, maxBytes);
                                    }

                                    // ICMP Protocol.
                                    uint8_t icmpProtocolBytes[] = { 1 };
                                    err = HAPTLVWriterAppend(
                                            &sub7Writer,
                                            &(const HAPTLV) {
                                                    .type = kHAPCharacteristicTLVType_WiFiRouter_ICMPType_Protocol,
                                                    .value = { .bytes = icmpProtocolBytes,
                                                               .numBytes = sizeof icmpProtocolBytes } });
                                    HAPAssert(!err);

                                    // ICMP Type Value.
                                    uint8_t icmpTypeBytes[] = { 128 };
                                    err = HAPTLVWriterAppend(
                                            &sub7Writer,
                                            &(const HAPTLV) {
                                                    .type = kHAPCharacteristicTLVType_WiFiRouter_ICMPType_TypeValue,
                                                    .value = { .bytes = icmpTypeBytes,
                                                               .numBytes = sizeof icmpTypeBytes } });
                                    HAPAssert(!err);

                                    // Finalize.
                                    void* bytes;
                                    size_t numBytes;
                                    HAPTLVWriterGetBuffer(&sub7Writer, &bytes, &numBytes);
                                    err = HAPTLVWriterAppend(
                                            &sub6Writer,
                                            &(const HAPTLV) {
                                                    .type = kHAPCharacteristicRawTLVType_WiFiRouter_ICMPList_ICMPType,
                                                    .value = { .bytes = bytes, .numBytes = numBytes } });
                                    HAPAssert(!err);
                                }

                                // Separator.
                                err = HAPTLVWriterAppend(
                                        &sub6Writer,
                                        &(const HAPTLV) { .type = 0, .value = { .bytes = NULL, .numBytes = 0 } });
                                HAPAssert(!err);

                                // ICMP Type.
                                {
                                    HAPTLVWriter sub7Writer;
                                    {
                                        void* bytes;
                                        size_t maxBytes;
                                        HAPTLVWriterGetScratchBytes(&sub6Writer, &bytes, &maxBytes);
                                        HAPTLVWriterCreate(&sub7Writer, bytes, maxBytes);
                                    }

                                    // ICMP Protocol.
                                    uint8_t icmpProtocolBytes[] = { 1 };
                                    err = HAPTLVWriterAppend(
                                            &sub7Writer,
                                            &(const HAPTLV) {
                                                    .type = kHAPCharacteristicTLVType_WiFiRouter_ICMPType_Protocol,
                                                    .value = { .bytes = icmpProtocolBytes,
                                                               .numBytes = sizeof icmpProtocolBytes } });
                                    HAPAssert(!err);

                                    // Finalize.
                                    void* bytes;
                                    size_t numBytes;
                                    HAPTLVWriterGetBuffer(&sub7Writer, &bytes, &numBytes);
                                    err = HAPTLVWriterAppend(
                                            &sub6Writer,
                                            &(const HAPTLV) {
                                                    .type = kHAPCharacteristicRawTLVType_WiFiRouter_ICMPList_ICMPType,
                                                    .value = { .bytes = bytes, .numBytes = numBytes } });
                                    HAPAssert(!err);
                                }

                                // Finalize.
                                void* bytes;
                                size_t numBytes;
                                HAPTLVWriterGetBuffer(&sub6Writer, &bytes, &numBytes);
                                err = HAPTLVWriterAppend(
                                        &sub5Writer,
                                        &(const HAPTLV) {
                                                .type = kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_ICMPRule_ICMPList,
                                                .value = { .bytes = bytes, .numBytes = numBytes } });
                                HAPAssert(!err);
                            }

                            // Finalize.
                            void* bytes;
                            size_t numBytes;
                            HAPTLVWriterGetBuffer(&sub5Writer, &bytes, &numBytes);
                            err = HAPTLVWriterAppend(
                                    &sub4Writer,
                                    &(const HAPTLV) {
                                            .type = kHAPCharacteristicValueType_WiFiRouter_WANFirewall_Rule_ICMP,
                                            .value = { .bytes = bytes, .numBytes = numBytes } });
                            HAPAssert(!err);
                        }

                        // Finalize.
                        void* bytes;
                        size_t numBytes;
                        HAPTLVWriterGetBuffer(&sub4Writer, &bytes, &numBytes);
                        err = HAPTLVWriterAppend(
                                &sub3Writer,
                                &(const HAPTLV) { .type = kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_RuleList,
                                                  .value = { .bytes = bytes, .numBytes = numBytes } });
                        HAPAssert(!err);
                    }

                    // Finalize.
                    void* bytes;
                    size_t numBytes;
                    HAPTLVWriterGetBuffer(&sub3Writer, &bytes, &numBytes);
                    err = HAPTLVWriterAppend(
                            &sub2Writer,
                            &(const HAPTLV) {
                                    .type = kHAPCharacteristicTLVType_NetworkClientProfileControl_Config_WANFirewall,
                                    .value = { .bytes = bytes, .numBytes = numBytes } });
                    HAPAssert(!err);
                }

                // LAN Firewall Configuration.
                {
                    HAPTLVWriter sub3Writer;
                    {
                        void* bytes;
                        size_t maxBytes;
                        HAPTLVWriterGetScratchBytes(&sub2Writer, &bytes, &maxBytes);
                        HAPTLVWriterCreate(&sub3Writer, bytes, maxBytes);
                    }

                    // LAN Firewall Type.
                    uint8_t wanFirewallTypeBytes[] = { kHAPCharacteristicValue_WiFiRouter_FirewallType_Allowlist };
                    err = HAPTLVWriterAppend(
                            &sub3Writer,
                            &(const HAPTLV) { .type = kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_Type,
                                              .value = { .bytes = wanFirewallTypeBytes,
                                                         .numBytes = sizeof wanFirewallTypeBytes } });
                    HAPAssert(!err);

                    // LAN Firewall Rule List.
                    {
                        HAPTLVWriter sub4Writer;
                        {
                            void* bytes;
                            size_t maxBytes;
                            HAPTLVWriterGetScratchBytes(&sub3Writer, &bytes, &maxBytes);
                            HAPTLVWriterCreate(&sub4Writer, bytes, maxBytes);
                        }

                        // Static ICMP Rule.
                        {
                            HAPTLVWriter sub5Writer;
                            {
                                void* bytes;
                                size_t maxBytes;
                                HAPTLVWriterGetScratchBytes(&sub4Writer, &bytes, &maxBytes);
                                HAPTLVWriterCreate(&sub5Writer, bytes, maxBytes);
                            }

                            // Direction.
                            uint8_t directionBytes[] = {
                                kHAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction_Outbound
                            };
                            err = HAPTLVWriterAppend(
                                    &sub5Writer,
                                    &(const HAPTLV) {
                                            .type = kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_StaticICMPRule_Direction,
                                            .value = { .bytes = directionBytes, .numBytes = sizeof directionBytes } });
                            HAPAssert(!err);

                            // Endpoint List.
                            {
                                HAPTLVWriter sub6Writer;
                                {
                                    void* bytes;
                                    size_t maxBytes;
                                    HAPTLVWriterGetScratchBytes(&sub5Writer, &bytes, &maxBytes);
                                    HAPTLVWriterCreate(&sub6Writer, bytes, maxBytes);
                                }

                                // Client Group Identifier.
                                uint8_t identifier2Bytes[] = { HAPExpandLittleUInt32(
                                        kHAPCharacteristicValue_WiFiRouter_GroupIdentifier_Main) };
                                err = HAPTLVWriterAppend(
                                        &sub6Writer,
                                        &(const HAPTLV) {
                                                .type = kHAPCharacteristicRawTLVType_WiFiRouter_LANFirewall_Rule_EndpointList_Group,
                                                .value = { .bytes = identifier2Bytes,
                                                           .numBytes = sizeof identifier2Bytes } });
                                HAPAssert(!err);

                                // Finalize.
                                void* bytes;
                                size_t numBytes;
                                HAPTLVWriterGetBuffer(&sub6Writer, &bytes, &numBytes);
                                err = HAPTLVWriterAppend(
                                        &sub5Writer,
                                        &(const HAPTLV) {
                                                .type = kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_StaticICMPRule_EndpointList,
                                                .value = { .bytes = bytes, .numBytes = numBytes } });
                                HAPAssert(!err);
                            }

                            // ICMP Type List.
                            {
                                HAPTLVWriter sub6Writer;
                                {
                                    void* bytes;
                                    size_t maxBytes;
                                    HAPTLVWriterGetScratchBytes(&sub5Writer, &bytes, &maxBytes);
                                    HAPTLVWriterCreate(&sub6Writer, bytes, maxBytes);
                                }

                                // ICMP Type.
                                {
                                    HAPTLVWriter sub7Writer;
                                    {
                                        void* bytes;
                                        size_t maxBytes;
                                        HAPTLVWriterGetScratchBytes(&sub6Writer, &bytes, &maxBytes);
                                        HAPTLVWriterCreate(&sub7Writer, bytes, maxBytes);
                                    }

                                    // ICMP Protocol.
                                    uint8_t icmpProtocolBytes[] = { 0 };
                                    err = HAPTLVWriterAppend(
                                            &sub7Writer,
                                            &(const HAPTLV) {
                                                    .type = kHAPCharacteristicTLVType_WiFiRouter_ICMPType_Protocol,
                                                    .value = { .bytes = icmpProtocolBytes,
                                                               .numBytes = sizeof icmpProtocolBytes } });
                                    HAPAssert(!err);

                                    // ICMP Type.
                                    uint8_t icmpTypeBytes[] = { 8 };
                                    err = HAPTLVWriterAppend(
                                            &sub7Writer,
                                            &(const HAPTLV) {
                                                    .type = kHAPCharacteristicTLVType_WiFiRouter_ICMPType_TypeValue,
                                                    .value = { .bytes = icmpTypeBytes,
                                                               .numBytes = sizeof icmpTypeBytes } });
                                    HAPAssert(!err);

                                    // Finalize.
                                    void* bytes;
                                    size_t numBytes;
                                    HAPTLVWriterGetBuffer(&sub7Writer, &bytes, &numBytes);
                                    err = HAPTLVWriterAppend(
                                            &sub6Writer,
                                            &(const HAPTLV) {
                                                    .type = kHAPCharacteristicRawTLVType_WiFiRouter_ICMPList_ICMPType,
                                                    .value = { .bytes = bytes, .numBytes = numBytes } });
                                    HAPAssert(!err);
                                }

                                // Separator.
                                err = HAPTLVWriterAppend(
                                        &sub6Writer,
                                        &(const HAPTLV) { .type = 0, .value = { .bytes = NULL, .numBytes = 0 } });
                                HAPAssert(!err);

                                // ICMP Type.
                                {
                                    HAPTLVWriter sub7Writer;
                                    {
                                        void* bytes;
                                        size_t maxBytes;
                                        HAPTLVWriterGetScratchBytes(&sub6Writer, &bytes, &maxBytes);
                                        HAPTLVWriterCreate(&sub7Writer, bytes, maxBytes);
                                    }

                                    // ICMP Protocol.
                                    uint8_t icmpProtocolBytes[] = { 1 };
                                    err = HAPTLVWriterAppend(
                                            &sub7Writer,
                                            &(const HAPTLV) {
                                                    .type = kHAPCharacteristicTLVType_WiFiRouter_ICMPType_Protocol,
                                                    .value = { .bytes = icmpProtocolBytes,
                                                               .numBytes = sizeof icmpProtocolBytes } });
                                    HAPAssert(!err);

                                    // ICMP Type Value.
                                    uint8_t icmpTypeBytes[] = { 133 };
                                    err = HAPTLVWriterAppend(
                                            &sub7Writer,
                                            &(const HAPTLV) {
                                                    .type = kHAPCharacteristicTLVType_WiFiRouter_ICMPType_TypeValue,
                                                    .value = { .bytes = icmpTypeBytes,
                                                               .numBytes = sizeof icmpTypeBytes } });
                                    HAPAssert(!err);

                                    // Finalize.
                                    void* bytes;
                                    size_t numBytes;
                                    HAPTLVWriterGetBuffer(&sub7Writer, &bytes, &numBytes);
                                    err = HAPTLVWriterAppend(
                                            &sub6Writer,
                                            &(const HAPTLV) {
                                                    .type = kHAPCharacteristicRawTLVType_WiFiRouter_ICMPList_ICMPType,
                                                    .value = { .bytes = bytes, .numBytes = numBytes } });
                                    HAPAssert(!err);
                                }

                                // Separator.
                                err = HAPTLVWriterAppend(
                                        &sub6Writer,
                                        &(const HAPTLV) { .type = 0, .value = { .bytes = NULL, .numBytes = 0 } });
                                HAPAssert(!err);

                                // ICMP Type.
                                {
                                    HAPTLVWriter sub7Writer;
                                    {
                                        void* bytes;
                                        size_t maxBytes;
                                        HAPTLVWriterGetScratchBytes(&sub6Writer, &bytes, &maxBytes);
                                        HAPTLVWriterCreate(&sub7Writer, bytes, maxBytes);
                                    }

                                    // ICMP Protocol.
                                    uint8_t icmpProtocolBytes[] = { 1 };
                                    err = HAPTLVWriterAppend(
                                            &sub7Writer,
                                            &(const HAPTLV) {
                                                    .type = kHAPCharacteristicTLVType_WiFiRouter_ICMPType_Protocol,
                                                    .value = { .bytes = icmpProtocolBytes,
                                                               .numBytes = sizeof icmpProtocolBytes } });
                                    HAPAssert(!err);

                                    // Finalize.
                                    void* bytes;
                                    size_t numBytes;
                                    HAPTLVWriterGetBuffer(&sub7Writer, &bytes, &numBytes);
                                    err = HAPTLVWriterAppend(
                                            &sub6Writer,
                                            &(const HAPTLV) {
                                                    .type = kHAPCharacteristicRawTLVType_WiFiRouter_ICMPList_ICMPType,
                                                    .value = { .bytes = bytes, .numBytes = numBytes } });
                                    HAPAssert(!err);
                                }

                                // Finalize.
                                void* bytes;
                                size_t numBytes;
                                HAPTLVWriterGetBuffer(&sub6Writer, &bytes, &numBytes);
                                err = HAPTLVWriterAppend(
                                        &sub5Writer,
                                        &(const HAPTLV) {
                                                .type = kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_StaticICMPRule_ICMPList,
                                                .value = { .bytes = bytes, .numBytes = numBytes } });
                                HAPAssert(!err);
                            }

                            // Finalize.
                            void* bytes;
                            size_t numBytes;
                            HAPTLVWriterGetBuffer(&sub5Writer, &bytes, &numBytes);
                            err = HAPTLVWriterAppend(
                                    &sub4Writer,
                                    &(const HAPTLV) {
                                            .type = kHAPCharacteristicValueType_WiFiRouter_LANFirewall_Rule_StaticICMP,
                                            .value = { .bytes = bytes, .numBytes = numBytes } });
                            HAPAssert(!err);
                        }

                        // Finalize.
                        void* bytes;
                        size_t numBytes;
                        HAPTLVWriterGetBuffer(&sub4Writer, &bytes, &numBytes);
                        err = HAPTLVWriterAppend(
                                &sub3Writer,
                                &(const HAPTLV) { .type = kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_RuleList,
                                                  .value = { .bytes = bytes, .numBytes = numBytes } });
                        HAPAssert(!err);
                    }

                    // Finalize.
                    void* bytes;
                    size_t numBytes;
                    HAPTLVWriterGetBuffer(&sub3Writer, &bytes, &numBytes);
                    err = HAPTLVWriterAppend(
                            &sub2Writer,
                            &(const HAPTLV) {
                                    .type = kHAPCharacteristicTLVType_NetworkClientProfileControl_Config_LANFirewall,
                                    .value = { .bytes = bytes, .numBytes = numBytes } });
                    HAPAssert(!err);
                }

                // Finalize.
                void* bytes;
                size_t numBytes;
                HAPTLVWriterGetBuffer(&sub2Writer, &bytes, &numBytes);
                err = HAPTLVWriterAppend(
                        &subWriter,
                        &(const HAPTLV) {
                                .type = kHAPCharacteristicTLVType_NetworkClientProfileControl_Operation_Config,
                                .value = { .bytes = bytes, .numBytes = numBytes } });
                HAPAssert(!err);
            }

            // Finalize.
            void* bytes;
            size_t numBytes;
            HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
            err = HAPTLVWriterAppend(
                    &requestWriter,
                    &(const HAPTLV) { .type = kHAPCharacteristicTLVType_NetworkClientProfileControl_Operation,
                                      .value = { .bytes = bytes, .numBytes = numBytes } });
            HAPAssert(!err);
        }

        const HAPTLV8CharacteristicWriteRequest request = {
            .transportType = kHAPTransportType_IP,
            .session = session,
            .characteristic = &wiFiRouterNetworkClientProfileControlCharacteristic,
            .service = &wiFiRouterService,
            .accessory = &accessory,
            .remote = false,
            .authorizationData = { .bytes = NULL, .numBytes = 0 }
        };
        void* bytes;
        size_t numBytes;
        HAPTLVWriterGetBuffer(&requestWriter, &bytes, &numBytes);
        HAPLogCharacteristicBufferDebug(
                &kHAPLog_Default, request.characteristic, request.service, request.accessory, bytes, numBytes, ">");
        HAPTLVReader requestReader;
        HAPTLVReaderCreate(&requestReader, bytes, numBytes);
        err = request.characteristic->callbacks.handleWrite(server, &request, &requestReader, NULL);
        HAPAssert(!err);
    }
    {
        const HAPTLV8CharacteristicReadRequest request = { .transportType = kHAPTransportType_IP,
                                                           .session = session,
                                                           .characteristic =
                                                                   &wiFiRouterNetworkClientProfileControlCharacteristic,
                                                           .service = &wiFiRouterService,
                                                           .accessory = &accessory };
        uint8_t bytes[1024];
        HAPTLVWriter responseWriter;
        HAPTLVWriterCreate(&responseWriter, bytes, sizeof bytes);
        err = request.characteristic->callbacks.handleRead(server, &request, &responseWriter, NULL);
        HAPAssert(!err);
        HAPAssert(HAPPlatformWiFiRouterIsConfigurationAccessAvailable(HAPNonnull(platform.ip.wiFiRouter)));
        void* responseBytes;
        size_t numResponseBytes;
        HAPTLVWriterGetBuffer(&responseWriter, &responseBytes, &numResponseBytes);
        HAPLogCharacteristicBufferDebug(
                &kHAPLog_Default,
                request.characteristic,
                request.service,
                request.accessory,
                responseBytes,
                numResponseBytes,
                "<");
        HAPTLVReader responseReader;
        HAPTLVReaderCreate(&responseReader, responseBytes, numResponseBytes);

        HAPCharacteristicValue_NetworkClientProfileControl_Response value;
        err = HAPTLVReaderDecode(
                &responseReader, &kHAPCharacteristicTLVFormat_NetworkClientProfileControl_Response, &value);
        HAPAssert(!err);

        EnumerateClientOperationResponsesContext enumerateContext;
        HAPRawBufferZero(&enumerateContext, sizeof enumerateContext);
        enumerateContext.configs = operations;
        enumerateContext.maxConfigs = HAPArrayCount(operations);
        err = value.enumerate(
                &value.dataSource, EnumerateUpdateAndRemoveClientOperationResponsesCallback, &enumerateContext);
        HAPAssert(!err);
        numOperations = enumerateContext.numConfigs;

        {
            char logBytes[2 * 1024];
            HAPStringBuilder stringBuilder;
            HAPStringBuilderCreate(&stringBuilder, logBytes, sizeof logBytes);
            HAPStringBuilderAppend(&stringBuilder, "Network Client Profile Control Response");
            for (size_t i = 0; i < numOperations; i++) {
                HAPStringBuilderAppend(
                        &stringBuilder, "\n- Network Client Profile Control Operation Response (%zu):", i);
                HAPStringBuilderAppend(
                        &stringBuilder, "\n  - Status: %u", kHAPCharacteristicValue_WiFiRouter_OperationStatus_Success);
            }
            HAPAssert(!HAPStringBuilderDidOverflow(&stringBuilder));
            HAPLogCharacteristicInfo(
                    &kHAPLog_Default,
                    request.characteristic,
                    request.service,
                    request.accessory,
                    "%s",
                    HAPStringBuilderGetString(&stringBuilder));
        }
    }
    HAPAssert(numOperations == 1);

    // Check that Wi-Fi has been disassociated.
    HAPAssert(!IsClientConnected(server, session, clientIDC, &clientStatusC.macAddress));

    // Join Wi-Fi with unique-psk-c.
    err = HAPPlatformWiFiRouterConnectClient(
            HAPNonnull(platform.ip.wiFiRouter),
            uniquePSKC,
            clientMACAddressC,
            clientNameC->stringValue,
            /* ipAddress: */ NULL);
    HAPAssert(!err);

    // Poll Network Client Status.
    HAPAssert(IsClientConnected(server, session, clientIDC, clientMACAddressC));
}

//----------------------------------------------------------------------------------------------------------------------

typedef struct {
    HAPPlatformWiFiRouterClientIdentifier clientIdentifier;
    HAPPlatformWiFiRouterAccessViolationMetadata violation;
} NetworkAccessViolation;

typedef struct {
    const uint32_t* _Nullable identifiers;
    size_t numIdentifiers;
} ClientIdentifierDataSource;

HAP_RESULT_USE_CHECK
static HAPError EnumerateClientIdentifiers(
        HAPSequenceTLVDataSource* dataSource_,
        HAPCharacteristicValueCallback_WiFiRouter_ClientList callback,
        void* _Nullable context) {
    HAPPrecondition(dataSource_);
    ClientIdentifierDataSource* dataSource = (ClientIdentifierDataSource*) dataSource_;
    HAPPrecondition(dataSource->identifiers);
    const uint32_t* identifiers = dataSource->identifiers;
    size_t numIdentifiers = dataSource->numIdentifiers;
    HAPPrecondition(callback);

    bool shouldContinue = true;
    for (size_t i = 0; shouldContinue && i < numIdentifiers; i++) {
        uint32_t value = identifiers[i];
        callback(context, &value, &shouldContinue);
    }

    return kHAPError_None;
}

typedef struct {
    NetworkAccessViolation* violations;
    size_t maxViolations;
    size_t numViolations;
} EnumerateAccessViolationsContext;

static void EnumerateAccessViolationsCallback(
        void* _Nullable context_,
        HAPCharacteristicValue_NetworkAccessViolationControl_Violation* value,
        bool* shouldContinue) {
    HAPPrecondition(context_);
    EnumerateAccessViolationsContext* context = context_;
    HAPPrecondition(context->violations);
    HAPPrecondition(context->numViolations < context->maxViolations);
    NetworkAccessViolation* violation = &context->violations[context->numViolations++];
    HAPRawBufferZero(violation, sizeof *violation);
    HAPPrecondition(value);
    HAPPrecondition(shouldContinue);
    HAPPrecondition(*shouldContinue);

    HAPAssert(value->clientIdentifier);
    violation->clientIdentifier = value->clientIdentifier;

    violation->violation.lastResetTimestamp = value->resetTimestamp;

    if (value->resetIsSet) {
        violation->violation.lastResetTimestamp = value->resetTimestamp;
        violation->violation.wasReset = true;
    }
    if (value->timestampIsSet) {
        if (value->resetIsSet) {
            HAPAssert(value->resetTimestamp < value->lastTimestamp);
        }
        violation->violation.lastViolationTimestamp = value->lastTimestamp;
        violation->violation.hasViolations = true;
    }
}

static void ListNetworkAccessViolations(
        HAPAccessoryServer* server,
        HAPSession* session,
        NetworkAccessViolation* violations,
        size_t maxViolations,
        size_t* numViolations) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(violations);
    HAPPrecondition(numViolations);

    HAPError err;

    *numViolations = 0;
    {
        uint8_t requestBytes[1024];
        HAPTLVWriter requestWriter;
        HAPTLVWriterCreate(&requestWriter, requestBytes, sizeof requestBytes);

        HAPCharacteristicValue_NetworkAccessViolationControl value;
        HAPRawBufferZero(&value, sizeof value);
        value.operationType = kHAPCharacteristicValue_NetworkAccessViolationControl_OperationType_List;
        err = HAPTLVWriterEncode(&requestWriter, &kHAPCharacteristicTLVFormat_NetworkAccessViolationControl, &value);
        HAPAssert(!err);

        const HAPTLV8CharacteristicWriteRequest request = {
            .transportType = kHAPTransportType_IP,
            .session = session,
            .characteristic = &wiFiRouterNetworkAccessViolationControlCharacteristic,
            .service = &wiFiRouterService,
            .accessory = &accessory,
            .remote = false,
            .authorizationData = { .bytes = NULL, .numBytes = 0 }
        };
        void* bytes;
        size_t numBytes;
        HAPTLVWriterGetBuffer(&requestWriter, &bytes, &numBytes);
        HAPLogCharacteristicBufferDebug(
                &kHAPLog_Default, request.characteristic, request.service, request.accessory, bytes, numBytes, ">");
        HAPTLVReader requestReader;
        HAPTLVReaderCreate(&requestReader, bytes, numBytes);
        err = request.characteristic->callbacks.handleWrite(server, &request, &requestReader, NULL);
        HAPAssert(!err);
    }
    {
        const HAPTLV8CharacteristicReadRequest request = {
            .transportType = kHAPTransportType_IP,
            .session = session,
            .characteristic = &wiFiRouterNetworkAccessViolationControlCharacteristic,
            .service = &wiFiRouterService,
            .accessory = &accessory
        };
        uint8_t bytes[1024];
        HAPTLVWriter responseWriter;
        HAPTLVWriterCreate(&responseWriter, bytes, sizeof bytes);
        err = request.characteristic->callbacks.handleRead(server, &request, &responseWriter, NULL);
        HAPAssert(!err);
        HAPAssert(HAPPlatformWiFiRouterIsConfigurationAccessAvailable(HAPNonnull(platform.ip.wiFiRouter)));
        void* responseBytes;
        size_t numResponseBytes;
        HAPTLVWriterGetBuffer(&responseWriter, &responseBytes, &numResponseBytes);
        HAPLogCharacteristicBufferDebug(
                &kHAPLog_Default,
                request.characteristic,
                request.service,
                request.accessory,
                responseBytes,
                numResponseBytes,
                "<");
        HAPTLVReader responseReader;
        HAPTLVReaderCreate(&responseReader, responseBytes, numResponseBytes);

        HAPCharacteristicValue_NetworkAccessViolationControl_Response value;
        err = HAPTLVReaderDecode(
                &responseReader, &kHAPCharacteristicTLVFormat_NetworkAccessViolationControl_Response, &value);
        HAPAssert(!err);

        EnumerateAccessViolationsContext enumerateContext;
        HAPRawBufferZero(&enumerateContext, sizeof enumerateContext);
        enumerateContext.violations = violations;
        enumerateContext.maxViolations = maxViolations;
        err = value.enumerate(&value.dataSource, EnumerateAccessViolationsCallback, &enumerateContext);
        HAPAssert(!err);
        *numViolations = enumerateContext.numViolations;
    }
}

static void ResetNetworkAccessViolations(
        HAPAccessoryServer* server,
        HAPSession* session,
        const HAPPlatformWiFiRouterClientIdentifier* clientIdentifiers,
        size_t numClientIdentifiers) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(clientIdentifiers);

    HAPError err;

    size_t numViolations = 0;
    NetworkAccessViolation violations[32];
    {
        uint8_t requestBytes[1024];
        HAPTLVWriter requestWriter;
        HAPTLVWriterCreate(&requestWriter, requestBytes, sizeof requestBytes);

        HAPCharacteristicValue_NetworkAccessViolationControl value;
        HAPRawBufferZero(&value, sizeof value);

        value.operationType = kHAPCharacteristicValue_NetworkAccessViolationControl_OperationType_Reset;

        ClientIdentifierDataSource* dataSource = (ClientIdentifierDataSource*) &value.clientList.dataSource;
        dataSource->identifiers = clientIdentifiers;
        dataSource->numIdentifiers = numClientIdentifiers;
        value.clientList.enumerate = EnumerateClientIdentifiers;
        value.clientListIsSet = true;

        err = HAPTLVWriterEncode(&requestWriter, &kHAPCharacteristicTLVFormat_NetworkAccessViolationControl, &value);
        HAPAssert(!err);

        const HAPTLV8CharacteristicWriteRequest request = {
            .transportType = kHAPTransportType_IP,
            .session = session,
            .characteristic = &wiFiRouterNetworkAccessViolationControlCharacteristic,
            .service = &wiFiRouterService,
            .accessory = &accessory,
            .remote = false,
            .authorizationData = { .bytes = NULL, .numBytes = 0 }
        };
        void* bytes;
        size_t numBytes;
        HAPTLVWriterGetBuffer(&requestWriter, &bytes, &numBytes);
        HAPLogCharacteristicBufferDebug(
                &kHAPLog_Default, request.characteristic, request.service, request.accessory, bytes, numBytes, ">");
        HAPTLVReader requestReader;
        HAPTLVReaderCreate(&requestReader, bytes, numBytes);
        err = request.characteristic->callbacks.handleWrite(server, &request, &requestReader, NULL);
        HAPAssert(!err);
    }
    {
        const HAPTLV8CharacteristicReadRequest request = {
            .transportType = kHAPTransportType_IP,
            .session = session,
            .characteristic = &wiFiRouterNetworkAccessViolationControlCharacteristic,
            .service = &wiFiRouterService,
            .accessory = &accessory
        };
        uint8_t bytes[1024];
        HAPTLVWriter responseWriter;
        HAPTLVWriterCreate(&responseWriter, bytes, sizeof bytes);
        err = request.characteristic->callbacks.handleRead(server, &request, &responseWriter, NULL);
        HAPAssert(!err);
        HAPAssert(HAPPlatformWiFiRouterIsConfigurationAccessAvailable(HAPNonnull(platform.ip.wiFiRouter)));
        void* responseBytes;
        size_t numResponseBytes;
        HAPTLVWriterGetBuffer(&responseWriter, &responseBytes, &numResponseBytes);
        HAPLogCharacteristicBufferDebug(
                &kHAPLog_Default,
                request.characteristic,
                request.service,
                request.accessory,
                responseBytes,
                numResponseBytes,
                "<");
        HAPTLVReader responseReader;
        HAPTLVReaderCreate(&responseReader, responseBytes, numResponseBytes);

        HAPCharacteristicValue_NetworkAccessViolationControl_Response value;
        err = HAPTLVReaderDecode(
                &responseReader, &kHAPCharacteristicTLVFormat_NetworkAccessViolationControl_Response, &value);
        HAPAssert(!err);

        EnumerateAccessViolationsContext enumerateContext;
        HAPRawBufferZero(&enumerateContext, sizeof enumerateContext);
        enumerateContext.violations = violations;
        enumerateContext.maxViolations = HAPArrayCount(violations);
        err = value.enumerate(&value.dataSource, EnumerateAccessViolationsCallback, &enumerateContext);
        HAPAssert(!err);
        numViolations = enumerateContext.numViolations;
    }
    HAPAssert(!numViolations);
}

static void SubscribeToNetworkAccessViolations(HAPAccessoryServer* server, HAPSession* session) {
    HAPPrecondition(server);
    HAPPrecondition(session);

    {
        const HAPTLV8CharacteristicSubscriptionRequest request = {
            .transportType = kHAPTransportType_IP,
            .session = session,
            .characteristic = &wiFiRouterNetworkAccessViolationControlCharacteristic,
            .service = &wiFiRouterService,
            .accessory = &accessory
        };
        request.characteristic->callbacks.handleSubscribe(server, &request, NULL);
    }
}

typedef struct {
    HAPPlatformWiFiRouterClientIdentifier* clients;
    size_t maxClients;
    size_t numClients;
} EnumerateClientListContext;

static void EnumerateClientListCallback(void* _Nullable context_, uint32_t* value, bool* shouldContinue) {
    HAPPrecondition(context_);
    EnumerateClientListContext* context = context_;
    HAPPrecondition(context->clients);
    HAPPrecondition(context->numClients < context->maxClients);
    HAPPlatformWiFiRouterClientIdentifier* client = &context->clients[context->numClients++];
    HAPRawBufferZero(client, sizeof *client);
    HAPPrecondition(value);
    HAPPrecondition(shouldContinue);
    HAPPrecondition(*shouldContinue);

    HAPAssert(*value);
    *client = *value;
}

static void GetNetworkAccessViolationEvent(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPPlatformWiFiRouterClientIdentifier* clients,
        size_t maxClients,
        size_t* numClients) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(clients);
    HAPPrecondition(numClients);

    HAPError err;

    *numClients = 0;
    {
        const HAPTLV8CharacteristicReadRequest request = {
            .transportType = kHAPTransportType_IP,
            .session = session,
            .characteristic = &wiFiRouterNetworkAccessViolationControlCharacteristic,
            .service = &wiFiRouterService,
            .accessory = &accessory
        };
        uint8_t bytes[1024];
        HAPTLVWriter responseWriter;
        HAPTLVWriterCreate(&responseWriter, bytes, sizeof bytes);
        err = request.characteristic->callbacks.handleRead(server, &request, &responseWriter, NULL);
        HAPAssert(!err);
        HAPAssert(HAPPlatformWiFiRouterIsConfigurationAccessAvailable(HAPNonnull(platform.ip.wiFiRouter)));
        void* responseBytes;
        size_t numResponseBytes;
        HAPTLVWriterGetBuffer(&responseWriter, &responseBytes, &numResponseBytes);
        HAPLogCharacteristicBufferDebug(
                &kHAPLog_Default,
                request.characteristic,
                request.service,
                request.accessory,
                responseBytes,
                numResponseBytes,
                "<");
        HAPTLVReader responseReader;
        HAPTLVReaderCreate(&responseReader, responseBytes, numResponseBytes);

        HAPCharacteristicValue_NetworkAccessViolationControl_Event value;
        err = HAPTLVReaderDecode(
                &responseReader, &kHAPCharacteristicTLVFormat_NetworkAccessViolationControl_Event, &value);
        HAPAssert(!err);

        EnumerateClientListContext enumerateContext;
        HAPRawBufferZero(&enumerateContext, sizeof enumerateContext);
        enumerateContext.clients = clients;
        enumerateContext.maxClients = maxClients;
        HAPAssert(value.clientListIsSet);
        err = value.clientList.enumerate(&value.clientList.dataSource, EnumerateClientListCallback, &enumerateContext);
        HAPAssert(!err);
        *numClients = enumerateContext.numClients;
    }
}

/**
 * Run scenario 5: Network Access Violations.
 */
static void Scenario5(HAPAccessoryServer* server, HAPSession* session) {
    HAPPrecondition(server);
    HAPPrecondition(session);

    HAPError err;

    SubscribeToNetworkAccessViolations(server, session);

    // List Network Access Violations.
    HAPLog(&kHAPLog_Default, "Listing Network Access Violations.");
    {
        size_t numViolations = 0;
        NetworkAccessViolation violations[32];
        ListNetworkAccessViolations(server, session, violations, HAPArrayCount(violations), &numViolations);
        HAPAssert(numViolations == 2);
        bool foundViolations[2] = { 0 };
        for (size_t i = 0; i < numViolations; i++) {
            if (violations[i].clientIdentifier == 1) {
                HAPAssert(!violations[i].violation.wasReset);
                HAPAssert(!violations[i].violation.hasViolations);
                HAPAssert(!foundViolations[0]);
                foundViolations[0] = true;
            } else if (violations[i].clientIdentifier == 2) {
                HAPAssert(!violations[i].violation.wasReset);
                HAPAssert(!violations[i].violation.hasViolations);
                HAPAssert(!foundViolations[1]);
                foundViolations[1] = true;
            } else {
                HAPAssertionFailure();
            }
        }
        HAPAssert(foundViolations[0]);
        HAPAssert(foundViolations[1]);
    }

    // Create Network Access Violations.
    HAPLog(&kHAPLog_Default, "Creating Network Access Violations.");
    {
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier = 1;
        HAPPlatformWiFiRouterTimestamp timestamp = 100;
        err = HAPPlatformWiFiRouterClientRecordAccessViolation(
                HAPNonnull(platform.ip.wiFiRouter), clientIdentifier, timestamp);
        HAPAssert(!err);
    }
    {
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier = 2;
        HAPPlatformWiFiRouterTimestamp timestamp = 200;
        err = HAPPlatformWiFiRouterClientRecordAccessViolation(
                HAPNonnull(platform.ip.wiFiRouter), clientIdentifier, timestamp);
        HAPAssert(!err);
    }

    // Poll Network Access Violation Events.
    {
        HAPPlatformWiFiRouterClientIdentifier clients[32];
        size_t numClients = 0;
        GetNetworkAccessViolationEvent(server, session, clients, HAPArrayCount(clients), &numClients);
        HAPAssert(numClients == 2);
        HAPAssert(clients[0] == 1);
        HAPAssert(clients[1] == 2);
    }

    // List Network Access Violations.
    HAPLog(&kHAPLog_Default, "Listing Network Access Violations.");
    {
        size_t numViolations = 0;
        NetworkAccessViolation violations[32];
        ListNetworkAccessViolations(server, session, violations, HAPArrayCount(violations), &numViolations);
        bool foundViolations[2] = { 0 };
        for (size_t i = 0; i < numViolations; i++) {
            if (violations[i].clientIdentifier == 1) {
                HAPAssert(!violations[i].violation.wasReset);
                HAPAssert(violations[i].violation.hasViolations);
                HAPAssert(violations[i].violation.lastViolationTimestamp == 100);
                HAPAssert(!foundViolations[0]);
                foundViolations[0] = true;
            } else if (violations[i].clientIdentifier == 2) {
                HAPAssert(!violations[i].violation.wasReset);
                HAPAssert(violations[i].violation.hasViolations);
                HAPAssert(violations[i].violation.lastViolationTimestamp == 200);
                HAPAssert(!foundViolations[1]);
                foundViolations[1] = true;
            } else {
                HAPAssertionFailure();
            }
        }
        HAPAssert(foundViolations[0]);
        HAPAssert(foundViolations[1]);
    }

    // Reset Network Access Violations.
    HAPLog(&kHAPLog_Default, "Resetting Network Access Violations.");
    {
        HAPPlatformWiFiRouterClientIdentifier clientIdentifiers[] = { 1, 2 };
        ResetNetworkAccessViolations(server, session, clientIdentifiers, HAPArrayCount(clientIdentifiers));
    }

    // Poll Network Access Violation Events.
    {
        HAPPlatformWiFiRouterClientIdentifier clients[32];
        size_t numClients = 0;
        GetNetworkAccessViolationEvent(server, session, clients, HAPArrayCount(clients), &numClients);
        HAPAssert(numClients == 0);
    }

    // List Network Access Violations.
    HAPLog(&kHAPLog_Default, "Listing Network Access Violations.");
    {
        size_t numViolations = 0;
        NetworkAccessViolation violations[32];
        ListNetworkAccessViolations(server, session, violations, HAPArrayCount(violations), &numViolations);
        bool foundViolations[2] = { 0 };
        for (size_t i = 0; i < numViolations; i++) {
            if (violations[i].clientIdentifier == 1) {
                HAPAssert(violations[i].violation.wasReset);
                HAPAssert(!violations[i].violation.hasViolations);
                HAPAssert(!foundViolations[0]);
                foundViolations[0] = true;
            } else if (violations[i].clientIdentifier == 2) {
                HAPAssert(violations[i].violation.wasReset);
                HAPAssert(!violations[i].violation.hasViolations);
                HAPAssert(!foundViolations[1]);
                foundViolations[1] = true;
            } else {
                HAPAssertionFailure();
            }
        }
        HAPAssert(foundViolations[0]);
        HAPAssert(foundViolations[1]);
    }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Run scenario 6: Remove Network Client Profile.
 */
static void Scenario6(
        HAPAccessoryServer* server,
        HAPSession* session,
        const HAPMACAddress* macAddress,
        HAPPlatformWiFiRouterClientIdentifier clientIDC) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(macAddress);
    HAPPrecondition(clientIDC);

    HAPError err;

    // Network client information.

    size_t numOperations = 0;
    ClientConfiguration operations[1];
    HAPRawBufferZero(operations, sizeof operations);
    {
        uint8_t requestBytes[1024];
        HAPTLVWriter requestWriter;
        HAPTLVWriterCreate(&requestWriter, requestBytes, sizeof requestBytes);

        // Network Client Profile Control Operation.
        {
            HAPTLVWriter subWriter;
            {
                void* bytes;
                size_t maxBytes;
                HAPTLVWriterGetScratchBytes(&requestWriter, &bytes, &maxBytes);
                HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
            }

            // Operation Type.
            uint8_t operationTypeBytes[] = {
                kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_Remove
            };
            err = HAPTLVWriterAppend(
                    &subWriter,
                    &(const HAPTLV) {
                            .type = kHAPCharacteristicTLVType_NetworkClientProfileControl_Operation_Type,
                            .value = { .bytes = operationTypeBytes, .numBytes = sizeof operationTypeBytes } });
            HAPAssert(!err);

            // Network Client Profile Configuration.
            {
                HAPTLVWriter sub2Writer;
                {
                    void* bytes;
                    size_t maxBytes;
                    HAPTLVWriterGetScratchBytes(&subWriter, &bytes, &maxBytes);
                    HAPTLVWriterCreate(&sub2Writer, bytes, maxBytes);
                }

                // Network Client Profile Identifier.
                uint8_t networkClientIdentifierBytes[] = { HAPExpandLittleUInt32(clientIDC) };
                err = HAPTLVWriterAppend(
                        &sub2Writer,
                        &(const HAPTLV) {
                                .type = kHAPCharacteristicTLVType_NetworkClientProfileControl_Config_Identifier,
                                .value = { .bytes = networkClientIdentifierBytes,
                                           .numBytes = sizeof networkClientIdentifierBytes } });
                HAPAssert(!err);

                // Finalize.
                void* bytes;
                size_t numBytes;
                HAPTLVWriterGetBuffer(&sub2Writer, &bytes, &numBytes);
                err = HAPTLVWriterAppend(
                        &subWriter,
                        &(const HAPTLV) {
                                .type = kHAPCharacteristicTLVType_NetworkClientProfileControl_Operation_Config,
                                .value = { .bytes = bytes, .numBytes = numBytes } });
                HAPAssert(!err);
            }

            // Finalize.
            void* bytes;
            size_t numBytes;
            HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
            err = HAPTLVWriterAppend(
                    &requestWriter,
                    &(const HAPTLV) { .type = kHAPCharacteristicTLVType_NetworkClientProfileControl_Operation,
                                      .value = { .bytes = bytes, .numBytes = numBytes } });
            HAPAssert(!err);
        }

        const HAPTLV8CharacteristicWriteRequest request = {
            .transportType = kHAPTransportType_IP,
            .session = session,
            .characteristic = &wiFiRouterNetworkClientProfileControlCharacteristic,
            .service = &wiFiRouterService,
            .accessory = &accessory,
            .remote = false,
            .authorizationData = { .bytes = NULL, .numBytes = 0 }
        };
        void* bytes;
        size_t numBytes;
        HAPTLVWriterGetBuffer(&requestWriter, &bytes, &numBytes);
        HAPLogCharacteristicBufferDebug(
                &kHAPLog_Default, request.characteristic, request.service, request.accessory, bytes, numBytes, ">");
        HAPTLVReader requestReader;
        HAPTLVReaderCreate(&requestReader, bytes, numBytes);
        err = request.characteristic->callbacks.handleWrite(server, &request, &requestReader, NULL);
        HAPAssert(!err);
    }
    {
        const HAPTLV8CharacteristicReadRequest request = { .transportType = kHAPTransportType_IP,
                                                           .session = session,
                                                           .characteristic =
                                                                   &wiFiRouterNetworkClientProfileControlCharacteristic,
                                                           .service = &wiFiRouterService,
                                                           .accessory = &accessory };
        uint8_t bytes[1024];
        HAPTLVWriter responseWriter;
        HAPTLVWriterCreate(&responseWriter, bytes, sizeof bytes);
        err = request.characteristic->callbacks.handleRead(server, &request, &responseWriter, NULL);
        HAPAssert(!err);
        HAPAssert(HAPPlatformWiFiRouterIsConfigurationAccessAvailable(HAPNonnull(platform.ip.wiFiRouter)));
        void* responseBytes;
        size_t numResponseBytes;
        HAPTLVWriterGetBuffer(&responseWriter, &responseBytes, &numResponseBytes);
        HAPLogCharacteristicBufferDebug(
                &kHAPLog_Default,
                request.characteristic,
                request.service,
                request.accessory,
                responseBytes,
                numResponseBytes,
                "<");
        HAPTLVReader responseReader;
        HAPTLVReaderCreate(&responseReader, responseBytes, numResponseBytes);

        HAPCharacteristicValue_NetworkClientProfileControl_Response value;
        err = HAPTLVReaderDecode(
                &responseReader, &kHAPCharacteristicTLVFormat_NetworkClientProfileControl_Response, &value);
        HAPAssert(!err);

        EnumerateClientOperationResponsesContext enumerateContext;
        HAPRawBufferZero(&enumerateContext, sizeof enumerateContext);
        enumerateContext.configs = operations;
        enumerateContext.maxConfigs = HAPArrayCount(operations);
        err = value.enumerate(
                &value.dataSource, EnumerateUpdateAndRemoveClientOperationResponsesCallback, &enumerateContext);
        HAPAssert(!err);
        numOperations = enumerateContext.numConfigs;

        {
            char logBytes[2 * 1024];
            HAPStringBuilder stringBuilder;
            HAPStringBuilderCreate(&stringBuilder, logBytes, sizeof logBytes);
            HAPStringBuilderAppend(&stringBuilder, "Network Client Profile Control Response");
            for (size_t i = 0; i < numOperations; i++) {
                HAPStringBuilderAppend(
                        &stringBuilder, "\n- Network Client Profile Control Operation Response (%zu):", i);
                HAPStringBuilderAppend(
                        &stringBuilder, "\n  - Status: %u", kHAPCharacteristicValue_WiFiRouter_OperationStatus_Success);
            }
            HAPAssert(!HAPStringBuilderDidOverflow(&stringBuilder));
            HAPLogCharacteristicInfo(
                    &kHAPLog_Default,
                    request.characteristic,
                    request.service,
                    request.accessory,
                    "%s",
                    HAPStringBuilderGetString(&stringBuilder));
        }
    }
    HAPAssert(numOperations == 1);

    // Check that client has been disassociated.
    HAPAssert(!IsClientConnected(server, session, clientIDC, macAddress));
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Run scenario 7: Remove Router from Home.
 */
static void Scenario7(HAPAccessoryServer* server, HAPSession* session) {
    HAPPrecondition(server);
    HAPPrecondition(session);

    HAPError err;

    // Disable HomeKit managed network.
    bool isManagedNetworkEnabled = false;
    {
        const HAPUInt8CharacteristicWriteRequest request = { .transportType = kHAPTransportType_IP,
                                                             .session = session,
                                                             .characteristic =
                                                                     &wiFiRouterManagedNetworkEnableCharacteristic,
                                                             .service = &wiFiRouterService,
                                                             .accessory = &accessory,
                                                             .remote = false,
                                                             .authorizationData = { .bytes = NULL, .numBytes = 0 } };
        uint8_t value = kHAPCharacteristicValue_ManagedNetworkEnable_Disabled;
        HAPLogCharacteristicDebug(
                &kHAPLog_Default, request.characteristic, request.service, request.accessory, "> %u", value);
        err = request.characteristic->callbacks.handleWrite(server, &request, value, NULL);
        HAPAssert(!err);
        HAPAssert(HAPPlatformWiFiRouterIsConfigurationAccessAvailable(HAPNonnull(platform.ip.wiFiRouter)));
    }
    {
        const HAPUInt8CharacteristicReadRequest request = { .transportType = kHAPTransportType_IP,
                                                            .session = session,
                                                            .characteristic =
                                                                    &wiFiRouterManagedNetworkEnableCharacteristic,
                                                            .service = &wiFiRouterService,
                                                            .accessory = &accessory };
        uint8_t value;
        err = request.characteristic->callbacks.handleRead(server, &request, &value, NULL);
        HAPAssert(!err);
        HAPAssert(HAPPlatformWiFiRouterIsConfigurationAccessAvailable(HAPNonnull(platform.ip.wiFiRouter)));
        HAPLogCharacteristicDebug(
                &kHAPLog_Default, request.characteristic, request.service, request.accessory, "< %u", value);

        switch (value) {
            case kHAPCharacteristicValue_ManagedNetworkEnable_Disabled: {
                isManagedNetworkEnabled = false;
                break;
            }
            case kHAPCharacteristicValue_ManagedNetworkEnable_Enabled: {
                isManagedNetworkEnabled = true;
                break;
            }
            default:
                HAPAssertionFailure();
        }

        HAPLogCharacteristicInfo(
                &kHAPLog_Default,
                request.characteristic,
                request.service,
                request.accessory,
                "Managed Network Enable: %s",
                isManagedNetworkEnabled ? "Enabled" : "Disabled");
    }
    HAPAssert(!isManagedNetworkEnabled);
}

//----------------------------------------------------------------------------------------------------------------------

static void ListAllClients(HAPAccessoryServer* server, HAPSession* session) {
    HAPPrecondition(server);
    HAPPrecondition(session);

    HAPError err;

    // List Network Client Profile Configurations.
    HAPLog(&kHAPLog_Default, "Listing Network Client Profile Configurations.");
    size_t numClients = 0;
    ClientConfiguration clients[32];
    {
        uint8_t requestBytes[1024];
        HAPTLVWriter requestWriter;
        HAPTLVWriterCreate(&requestWriter, requestBytes, sizeof requestBytes);

        // Network Client Profile Control Operation.
        {
            HAPTLVWriter subWriter;
            {
                void* bytes;
                size_t maxBytes;
                HAPTLVWriterGetScratchBytes(&requestWriter, &bytes, &maxBytes);
                HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
            }

            // Operation Type.
            uint8_t operationTypeBytes[] = { kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_List };
            err = HAPTLVWriterAppend(
                    &subWriter,
                    &(const HAPTLV) {
                            .type = kHAPCharacteristicTLVType_NetworkClientProfileControl_Operation_Type,
                            .value = { .bytes = operationTypeBytes, .numBytes = sizeof operationTypeBytes } });
            HAPAssert(!err);

            // Finalize.
            void* bytes;
            size_t numBytes;
            HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
            err = HAPTLVWriterAppend(
                    &requestWriter,
                    &(const HAPTLV) { .type = kHAPCharacteristicTLVType_NetworkClientProfileControl_Operation,
                                      .value = { .bytes = bytes, .numBytes = numBytes } });
            HAPAssert(!err);
        }

        const HAPTLV8CharacteristicWriteRequest request = {
            .transportType = kHAPTransportType_IP,
            .session = session,
            .characteristic = &wiFiRouterNetworkClientProfileControlCharacteristic,
            .service = &wiFiRouterService,
            .accessory = &accessory,
            .remote = false,
            .authorizationData = { .bytes = NULL, .numBytes = 0 }
        };
        void* bytes;
        size_t numBytes;
        HAPTLVWriterGetBuffer(&requestWriter, &bytes, &numBytes);
        HAPLogCharacteristicBufferDebug(
                &kHAPLog_Default, request.characteristic, request.service, request.accessory, bytes, numBytes, ">");
        HAPTLVReader requestReader;
        HAPTLVReaderCreate(&requestReader, bytes, numBytes);
        err = request.characteristic->callbacks.handleWrite(server, &request, &requestReader, NULL);
        HAPAssert(!err);
    }
    {
        const HAPTLV8CharacteristicReadRequest request = { .transportType = kHAPTransportType_IP,
                                                           .session = session,
                                                           .characteristic =
                                                                   &wiFiRouterNetworkClientProfileControlCharacteristic,
                                                           .service = &wiFiRouterService,
                                                           .accessory = &accessory };
        uint8_t bytes[1024];
        HAPTLVWriter responseWriter;
        HAPTLVWriterCreate(&responseWriter, bytes, sizeof bytes);
        err = request.characteristic->callbacks.handleRead(server, &request, &responseWriter, NULL);
        HAPAssert(!err);
        HAPAssert(HAPPlatformWiFiRouterIsConfigurationAccessAvailable(HAPNonnull(platform.ip.wiFiRouter)));
        void* responseBytes;
        size_t numResponseBytes;
        HAPTLVWriterGetBuffer(&responseWriter, &responseBytes, &numResponseBytes);
        HAPLogCharacteristicBufferDebug(
                &kHAPLog_Default,
                request.characteristic,
                request.service,
                request.accessory,
                responseBytes,
                numResponseBytes,
                "<");
        HAPTLVReader responseReader;
        HAPTLVReaderCreate(&responseReader, responseBytes, numResponseBytes);

        HAPCharacteristicValue_NetworkClientProfileControl_Response value;
        err = HAPTLVReaderDecode(
                &responseReader, &kHAPCharacteristicTLVFormat_NetworkClientProfileControl_Response, &value);
        HAPAssert(!err);

        EnumerateClientOperationResponsesContext enumerateContext;
        HAPRawBufferZero(&enumerateContext, sizeof enumerateContext);
        enumerateContext.configs = clients;
        enumerateContext.maxConfigs = HAPArrayCount(clients);
        err = value.enumerate(&value.dataSource, EnumerateListClientOperationResponsesCallback, &enumerateContext);
        HAPAssert(!err);
        numClients = enumerateContext.numConfigs;

        {
            char logBytes[2 * 1024];
            HAPStringBuilder stringBuilder;
            HAPStringBuilderCreate(&stringBuilder, logBytes, sizeof logBytes);
            HAPAssert(!err);
            HAPStringBuilderAppend(&stringBuilder, "Network Client Profile Control Response:");
            for (size_t i = 0; i < numClients; i++) {
                HAPStringBuilderAppend(&stringBuilder, "\n- Network Client Profile Control Operation Response:");
                HAPStringBuilderAppend(&stringBuilder, "\n  - Status: 0");
                HAPStringBuilderAppend(
                        &stringBuilder,
                        "\n  - Network Client Profile Identifier: %lu",
                        (unsigned long) clients[i].identifier);
            }
            HAPAssert(!HAPStringBuilderDidOverflow(&stringBuilder));
            HAPLogCharacteristicInfo(
                    &kHAPLog_Default,
                    request.characteristic,
                    request.service,
                    request.accessory,
                    "%s",
                    HAPStringBuilderGetString(&stringBuilder));
        }
    }
    for (size_t i = 0; i < numClients; i++) {
        for (size_t j = 0; j < i; j++) {
            HAPAssert(clients[i].identifier != clients[j].identifier);
        }
    }
    {
        uint8_t requestBytes[1024];
        HAPTLVWriter requestWriter;
        HAPTLVWriterCreate(&requestWriter, requestBytes, sizeof requestBytes);

        for (size_t i = 0; i < numClients; i++) {
            // Append separator if necessary.
            if (i) {
                err = HAPTLVWriterAppend(
                        &requestWriter, &(const HAPTLV) { .type = 0, .value = { .bytes = NULL, .numBytes = 0 } });
                HAPAssert(!err);
            }

            // Network Client Profile Control Operation.
            {
                HAPTLVWriter subWriter;
                {
                    void* bytes;
                    size_t maxBytes;
                    HAPTLVWriterGetScratchBytes(&requestWriter, &bytes, &maxBytes);
                    HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
                }

                // Operation Type.
                uint8_t operationTypeBytes[] = {
                    kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_Read
                };
                err = HAPTLVWriterAppend(
                        &subWriter,
                        &(const HAPTLV) {
                                .type = kHAPCharacteristicTLVType_NetworkClientProfileControl_Operation_Type,
                                .value = { .bytes = operationTypeBytes, .numBytes = sizeof operationTypeBytes } });
                HAPAssert(!err);

                // Network Client Profile Configuration.
                {
                    HAPTLVWriter sub2Writer;
                    {
                        void* bytes;
                        size_t maxBytes;
                        HAPTLVWriterGetScratchBytes(&subWriter, &bytes, &maxBytes);
                        HAPTLVWriterCreate(&sub2Writer, bytes, maxBytes);
                    }

                    // Network Client Profile Identifier.
                    uint8_t networkClientIdentifierBytes[] = { HAPExpandLittleUInt32(clients[i].identifier) };
                    err = HAPTLVWriterAppend(
                            &sub2Writer,
                            &(const HAPTLV) {
                                    .type = kHAPCharacteristicTLVType_NetworkClientProfileControl_Config_Identifier,
                                    .value = { .bytes = networkClientIdentifierBytes,
                                               .numBytes = sizeof networkClientIdentifierBytes } });
                    HAPAssert(!err);

                    // Finalize.
                    void* bytes;
                    size_t numBytes;
                    HAPTLVWriterGetBuffer(&sub2Writer, &bytes, &numBytes);
                    err = HAPTLVWriterAppend(
                            &subWriter,
                            &(const HAPTLV) {
                                    .type = kHAPCharacteristicTLVType_NetworkClientProfileControl_Operation_Config,
                                    .value = { .bytes = bytes, .numBytes = numBytes } });
                    HAPAssert(!err);
                }

                // Finalize.
                void* bytes;
                size_t numBytes;
                HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
                err = HAPTLVWriterAppend(
                        &requestWriter,
                        &(const HAPTLV) { .type = kHAPCharacteristicTLVType_NetworkClientProfileControl_Operation,
                                          .value = { .bytes = bytes, .numBytes = numBytes } });
                HAPAssert(!err);
            }
        }

        const HAPTLV8CharacteristicWriteRequest request = {
            .transportType = kHAPTransportType_IP,
            .session = session,
            .characteristic = &wiFiRouterNetworkClientProfileControlCharacteristic,
            .service = &wiFiRouterService,
            .accessory = &accessory,
            .remote = false,
            .authorizationData = { .bytes = NULL, .numBytes = 0 }
        };
        void* bytes;
        size_t numBytes;
        HAPTLVWriterGetBuffer(&requestWriter, &bytes, &numBytes);
        HAPLogCharacteristicBufferDebug(
                &kHAPLog_Default, request.characteristic, request.service, request.accessory, bytes, numBytes, ">");
        HAPTLVReader requestReader;
        HAPTLVReaderCreate(&requestReader, bytes, numBytes);
        err = request.characteristic->callbacks.handleWrite(server, &request, &requestReader, NULL);
        HAPAssert(!err);
    }
    {
        const HAPTLV8CharacteristicReadRequest request = { .transportType = kHAPTransportType_IP,
                                                           .session = session,
                                                           .characteristic =
                                                                   &wiFiRouterNetworkClientProfileControlCharacteristic,
                                                           .service = &wiFiRouterService,
                                                           .accessory = &accessory };
        uint8_t bytes[1024];
        HAPTLVWriter responseWriter;
        HAPTLVWriterCreate(&responseWriter, bytes, sizeof bytes);
        err = request.characteristic->callbacks.handleRead(server, &request, &responseWriter, NULL);
        HAPAssert(!err);
        HAPAssert(HAPPlatformWiFiRouterIsConfigurationAccessAvailable(HAPNonnull(platform.ip.wiFiRouter)));
        void* responseBytes;
        size_t numResponseBytes;
        HAPTLVWriterGetBuffer(&responseWriter, &responseBytes, &numResponseBytes);
        HAPLogCharacteristicBufferDebug(
                &kHAPLog_Default,
                request.characteristic,
                request.service,
                request.accessory,
                responseBytes,
                numResponseBytes,
                "<");
        HAPTLVReader responseReader;
        HAPTLVReaderCreate(&responseReader, responseBytes, numResponseBytes);

        HAPCharacteristicValue_NetworkClientProfileControl_Response value;
        err = HAPTLVReaderDecode(
                &responseReader, &kHAPCharacteristicTLVFormat_NetworkClientProfileControl_Response, &value);
        HAPAssert(!err);

        EnumerateClientOperationResponsesContext enumerateContext;
        HAPRawBufferZero(&enumerateContext, sizeof enumerateContext);
        enumerateContext.configs = clients;
        enumerateContext.maxConfigs = numClients;
        err = value.enumerate(&value.dataSource, EnumerateReadClientOperationResponsesCallback, &enumerateContext);
        HAPAssert(!err);
        HAPAssert(enumerateContext.numConfigs == numClients);

        {
            HAPLogCharacteristicInfo(
                    &kHAPLog_Default,
                    request.characteristic,
                    request.service,
                    request.accessory,
                    "\nNetwork Client Profile Control Response:");
            for (size_t i = 0; i < numClients; i++) {
                char logBytes[2 * 1024];
                HAPStringBuilder stringBuilder;
                HAPStringBuilderCreate(&stringBuilder, logBytes, sizeof logBytes);

                HAPStringBuilderAppend(
                        &stringBuilder, "\n- Network Client Profile Control Operation Response (%zu):", i);
                HAPStringBuilderAppend(&stringBuilder, "\n  - Status: 0");
                HAPStringBuilderAppend(&stringBuilder, "\n  - Network Client Profile Configuration:");
                HAPStringBuilderAppend(
                        &stringBuilder,
                        "\n    - Network Client Profile Identifier: %lu",
                        (unsigned long) clients[i].identifier);
                HAPStringBuilderAppend(
                        &stringBuilder,
                        "\n    - Client Group Identifier: %lu",
                        (unsigned long) clients[i].groupIdentifier);
                HAPStringBuilderAppend(&stringBuilder, "\n    - Credential Data:");
                switch (clients[i].credential.type) {
                    case kHAPPlatformWiFiRouterCredentialType_MACAddress: {
                        HAPStringBuilderAppend(
                                &stringBuilder,
                                "\n      - MAC Address Credential: %02X:%02X:%02X:%02X:%02X:%02X",
                                clients[i].credential._.macAddress.bytes[0],
                                clients[i].credential._.macAddress.bytes[1],
                                clients[i].credential._.macAddress.bytes[2],
                                clients[i].credential._.macAddress.bytes[3],
                                clients[i].credential._.macAddress.bytes[4],
                                clients[i].credential._.macAddress.bytes[5]);
                        break;
                    }
                    case kHAPCharacteristicValueType_WiFiRouter_Credential_PSK: {
                        HAPStringBuilderAppend(&stringBuilder, "\n      - PSK Credential: ");
                        switch (clients[i].credential._.psk.type) {
                            case kHAPWiFiWPAPersonalCredentialType_Passphrase: {
                                HAPStringBuilderAppend(
                                        &stringBuilder,
                                        "Passphrase: %s",
                                        clients[i].credential._.psk._.passphrase.stringValue);
                                break;
                            }
                            case kHAPWiFiWPAPersonalCredentialType_PSK: {
                                HAPStringBuilderAppend(&stringBuilder, "PSK: <");
                                for (size_t k = 0; k < sizeof clients[i].credential._.psk._.psk.bytes; k++) {
                                    if (k && !(k % 4)) {
                                        HAPStringBuilderAppend(&stringBuilder, " ");
                                    }
                                    HAPStringBuilderAppend(
                                            &stringBuilder, "%02X", clients[i].credential._.psk._.psk.bytes[k]);
                                }
                                HAPStringBuilderAppend(&stringBuilder, ">");
                                break;
                            }
                        }
                        break;
                    }
                }
                HAPStringBuilderAppend(&stringBuilder, "\n    - WAN Firewall Configuration:");
                switch ((HAPPlatformWiFiRouterFirewallType) clients[i].wanFirewall.type) {
                    case kHAPPlatformWiFiRouterFirewallType_FullAccess: {
                        HAPStringBuilderAppend(&stringBuilder, "\n      - WAN Firewall Type: Full Access");
                        break;
                    }
                    case kHAPPlatformWiFiRouterFirewallType_Allowlist: {
                        HAPStringBuilderAppend(&stringBuilder, "\n      - WAN Firewall Type: Allowlist");
                        HAPStringBuilderAppend(&stringBuilder, "\n      - WAN Firewall Rule List:");
                        for (size_t j = 0; j < clients[i].wanFirewall.numRules; j++) {
                            const WANFirewallRule* firewallRule_ = &clients[i].wanFirewall.rules[j];
                            switch (*(const HAPPlatformWiFiRouterWANFirewallRuleType*) firewallRule_) {
                                case kHAPPlatformWiFiRouterWANFirewallRuleType_Port: {
                                    const HAPPlatformWiFiRouterPortWANFirewallRule* firewallRule =
                                            &firewallRule_->port._;

                                    HAPStringBuilderAppend(&stringBuilder, "\n        - WAN Firewall Rule (%zu):", j);
                                    HAPStringBuilderAppend(&stringBuilder, "\n          - WAN Firewall Rule Type: ");
                                    switch (firewallRule->transportProtocol) {
                                        case kHAPPlatformWiFiRouterTransportProtocol_TCP: {
                                            HAPStringBuilderAppend(&stringBuilder, "TCP");
                                            break;
                                        }
                                        case kHAPPlatformWiFiRouterTransportProtocol_UDP: {
                                            HAPStringBuilderAppend(&stringBuilder, "UDP");
                                            break;
                                        }
                                    }
                                    switch (firewallRule->host.type) {
                                        case kHAPPlatformWiFiRouterWANHostURIType_Any: {
                                            HAPStringBuilderAppend(&stringBuilder, "\n          - Host IP Start:");
                                            HAPStringBuilderAppend(
                                                    &stringBuilder,
                                                    "\n            - IPv4 Address: %u.%u.%u.%u",
                                                    0,
                                                    0,
                                                    0,
                                                    0);
                                            HAPStringBuilderAppend(
                                                    &stringBuilder,
                                                    "\n            - IPv6 Address: "
                                                    "%04X:%04X:%04X:%04X:%04X:%04X:%04X:%04X",
                                                    0,
                                                    0,
                                                    0,
                                                    0,
                                                    0,
                                                    0,
                                                    0,
                                                    0);
                                            break;
                                        }
                                        case kHAPPlatformWiFiRouterWANHostURIType_DNSNamePattern: {
                                            HAPStringBuilderAppend(
                                                    &stringBuilder,
                                                    "\n          - Host DNS Name: %s",
                                                    firewallRule->host._.dnsNamePattern);
                                            break;
                                        }
                                        case kHAPPlatformWiFiRouterWANHostURIType_IPAddress: {
                                            HAPStringBuilderAppend(&stringBuilder, "\n          - Host IP Start:");
                                            switch (firewallRule->host._.ipAddress.version) {
                                                case kHAPIPAddressVersion_IPv4: {
                                                    HAPStringBuilderAppend(
                                                            &stringBuilder,
                                                            "\n            - IPv4 Address: %u.%u.%u.%u",
                                                            firewallRule->host._.ipAddress._.ipv4.bytes[0],
                                                            firewallRule->host._.ipAddress._.ipv4.bytes[1],
                                                            firewallRule->host._.ipAddress._.ipv4.bytes[2],
                                                            firewallRule->host._.ipAddress._.ipv4.bytes[3]);
                                                    break;
                                                }
                                                case kHAPIPAddressVersion_IPv6: {
                                                    HAPStringBuilderAppend(
                                                            &stringBuilder,
                                                            "\n            - IPv6 Address: "
                                                            "%04X:%04X:%04X:%04X:%04X:%04X:%04X:%04X",
                                                            HAPReadBigUInt16(
                                                                    &firewallRule->host._.ipAddress._.ipv6.bytes[0]),
                                                            HAPReadBigUInt16(
                                                                    &firewallRule->host._.ipAddress._.ipv6.bytes[2]),
                                                            HAPReadBigUInt16(
                                                                    &firewallRule->host._.ipAddress._.ipv6.bytes[4]),
                                                            HAPReadBigUInt16(
                                                                    &firewallRule->host._.ipAddress._.ipv6.bytes[6]),
                                                            HAPReadBigUInt16(
                                                                    &firewallRule->host._.ipAddress._.ipv6.bytes[8]),
                                                            HAPReadBigUInt16(
                                                                    &firewallRule->host._.ipAddress._.ipv6.bytes[10]),
                                                            HAPReadBigUInt16(
                                                                    &firewallRule->host._.ipAddress._.ipv6.bytes[12]),
                                                            HAPReadBigUInt16(
                                                                    &firewallRule->host._.ipAddress._.ipv6.bytes[14]));
                                                    break;
                                                }
                                            }
                                            break;
                                        }
                                        case kHAPPlatformWiFiRouterWANHostURIType_IPAddressRange: {
                                            HAPStringBuilderAppend(&stringBuilder, "\n          - Host IP Start:");
                                            switch (firewallRule->host._.ipAddressRange.version) {
                                                case kHAPIPAddressVersion_IPv4: {
                                                    HAPStringBuilderAppend(
                                                            &stringBuilder,
                                                            "\n            - IPv4 Address: %u.%u.%u.%u",
                                                            firewallRule->host._.ipAddressRange._.ipv4.startAddress
                                                                    .bytes[0],
                                                            firewallRule->host._.ipAddressRange._.ipv4.startAddress
                                                                    .bytes[1],
                                                            firewallRule->host._.ipAddressRange._.ipv4.startAddress
                                                                    .bytes[2],
                                                            firewallRule->host._.ipAddressRange._.ipv4.startAddress
                                                                    .bytes[3]);
                                                    break;
                                                }
                                                case kHAPIPAddressVersion_IPv6: {
                                                    HAPStringBuilderAppend(
                                                            &stringBuilder,
                                                            "\n            - IPv6 Address: "
                                                            "%04X:%04X:%04X:%04X:%04X:%04X:%04X:%04X",
                                                            HAPReadBigUInt16(&firewallRule->host._.ipAddressRange._.ipv6
                                                                                      .startAddress.bytes[0]),
                                                            HAPReadBigUInt16(&firewallRule->host._.ipAddressRange._.ipv6
                                                                                      .startAddress.bytes[2]),
                                                            HAPReadBigUInt16(&firewallRule->host._.ipAddressRange._.ipv6
                                                                                      .startAddress.bytes[4]),
                                                            HAPReadBigUInt16(&firewallRule->host._.ipAddressRange._.ipv6
                                                                                      .startAddress.bytes[6]),
                                                            HAPReadBigUInt16(&firewallRule->host._.ipAddressRange._.ipv6
                                                                                      .startAddress.bytes[8]),
                                                            HAPReadBigUInt16(&firewallRule->host._.ipAddressRange._.ipv6
                                                                                      .startAddress.bytes[10]),
                                                            HAPReadBigUInt16(&firewallRule->host._.ipAddressRange._.ipv6
                                                                                      .startAddress.bytes[12]),
                                                            HAPReadBigUInt16(&firewallRule->host._.ipAddressRange._.ipv6
                                                                                      .startAddress.bytes[14]));
                                                    break;
                                                }
                                            }
                                            HAPStringBuilderAppend(&stringBuilder, "\n          - Host IP End:");
                                            switch (firewallRule->host._.ipAddressRange.version) {
                                                case kHAPIPAddressVersion_IPv4: {
                                                    HAPStringBuilderAppend(
                                                            &stringBuilder,
                                                            "\n            - IPv4 Address: %u.%u.%u.%u",
                                                            firewallRule->host._.ipAddressRange._.ipv4.endAddress
                                                                    .bytes[0],
                                                            firewallRule->host._.ipAddressRange._.ipv4.endAddress
                                                                    .bytes[1],
                                                            firewallRule->host._.ipAddressRange._.ipv4.endAddress
                                                                    .bytes[2],
                                                            firewallRule->host._.ipAddressRange._.ipv4.endAddress
                                                                    .bytes[3]);
                                                    break;
                                                }
                                                case kHAPIPAddressVersion_IPv6: {
                                                    HAPStringBuilderAppend(
                                                            &stringBuilder,
                                                            "\n            - IPv6 Address: "
                                                            "%04X:%04X:%04X:%04X:%04X:%04X:%04X:%04X",
                                                            HAPReadBigUInt16(&firewallRule->host._.ipAddressRange._.ipv6
                                                                                      .endAddress.bytes[0]),
                                                            HAPReadBigUInt16(&firewallRule->host._.ipAddressRange._.ipv6
                                                                                      .endAddress.bytes[2]),
                                                            HAPReadBigUInt16(&firewallRule->host._.ipAddressRange._.ipv6
                                                                                      .endAddress.bytes[4]),
                                                            HAPReadBigUInt16(&firewallRule->host._.ipAddressRange._.ipv6
                                                                                      .endAddress.bytes[6]),
                                                            HAPReadBigUInt16(&firewallRule->host._.ipAddressRange._.ipv6
                                                                                      .endAddress.bytes[8]),
                                                            HAPReadBigUInt16(&firewallRule->host._.ipAddressRange._.ipv6
                                                                                      .endAddress.bytes[10]),
                                                            HAPReadBigUInt16(&firewallRule->host._.ipAddressRange._.ipv6
                                                                                      .endAddress.bytes[12]),
                                                            HAPReadBigUInt16(&firewallRule->host._.ipAddressRange._.ipv6
                                                                                      .endAddress.bytes[14]));
                                                    break;
                                                }
                                            }
                                            break;
                                        }
                                    }
                                    HAPStringBuilderAppend(
                                            &stringBuilder,
                                            "\n          - Host Port Start: %u",
                                            firewallRule->hostPortRange.startPort);
                                    if (firewallRule->hostPortRange.endPort != firewallRule->hostPortRange.startPort) {
                                        HAPStringBuilderAppend(
                                                &stringBuilder,
                                                "\n          - Host Port End: %u",
                                                firewallRule->hostPortRange.endPort);
                                    }
                                    break;
                                }
                                case kHAPPlatformWiFiRouterWANFirewallRuleType_ICMP: {
                                    const HAPPlatformWiFiRouterICMPWANFirewallRule* firewallRule =
                                            &firewallRule_->icmp._;

                                    HAPStringBuilderAppend(&stringBuilder, "\n        - WAN Firewall Rule (%zu):", j);
                                    HAPStringBuilderAppend(&stringBuilder, "\n          - WAN Firewall Rule Type: ");
                                    HAPStringBuilderAppend(&stringBuilder, "ICMP");
                                    switch (firewallRule->host.type) {
                                        case kHAPPlatformWiFiRouterWANHostURIType_Any: {
                                            HAPStringBuilderAppend(&stringBuilder, "\n          - Host IP Start:");
                                            HAPStringBuilderAppend(
                                                    &stringBuilder,
                                                    "\n            - IPv4 Address: %u.%u.%u.%u",
                                                    0,
                                                    0,
                                                    0,
                                                    0);
                                            HAPStringBuilderAppend(
                                                    &stringBuilder,
                                                    "\n            - IPv6 Address: "
                                                    "%04X:%04X:%04X:%04X:%04X:%04X:%04X:%04X",
                                                    0,
                                                    0,
                                                    0,
                                                    0,
                                                    0,
                                                    0,
                                                    0,
                                                    0);
                                            break;
                                        }
                                        case kHAPPlatformWiFiRouterWANHostURIType_DNSNamePattern: {
                                            HAPStringBuilderAppend(
                                                    &stringBuilder,
                                                    "\n          - Host DNS Name: %s",
                                                    firewallRule->host._.dnsNamePattern);
                                            break;
                                        }
                                        case kHAPPlatformWiFiRouterWANHostURIType_IPAddress: {
                                            HAPStringBuilderAppend(&stringBuilder, "\n          - Host IP Start:");
                                            switch (firewallRule->host._.ipAddress.version) {
                                                case kHAPIPAddressVersion_IPv4: {
                                                    HAPStringBuilderAppend(
                                                            &stringBuilder,
                                                            "\n            - IPv4 Address: %u.%u.%u.%u",
                                                            firewallRule->host._.ipAddress._.ipv4.bytes[0],
                                                            firewallRule->host._.ipAddress._.ipv4.bytes[1],
                                                            firewallRule->host._.ipAddress._.ipv4.bytes[2],
                                                            firewallRule->host._.ipAddress._.ipv4.bytes[3]);
                                                    break;
                                                }
                                                case kHAPIPAddressVersion_IPv6: {
                                                    HAPStringBuilderAppend(
                                                            &stringBuilder,
                                                            "\n            - IPv6 Address: "
                                                            "%04X:%04X:%04X:%04X:%04X:%04X:%04X:%04X",
                                                            HAPReadBigUInt16(
                                                                    &firewallRule->host._.ipAddress._.ipv6.bytes[0]),
                                                            HAPReadBigUInt16(
                                                                    &firewallRule->host._.ipAddress._.ipv6.bytes[2]),
                                                            HAPReadBigUInt16(
                                                                    &firewallRule->host._.ipAddress._.ipv6.bytes[4]),
                                                            HAPReadBigUInt16(
                                                                    &firewallRule->host._.ipAddress._.ipv6.bytes[6]),
                                                            HAPReadBigUInt16(
                                                                    &firewallRule->host._.ipAddress._.ipv6.bytes[8]),
                                                            HAPReadBigUInt16(
                                                                    &firewallRule->host._.ipAddress._.ipv6.bytes[10]),
                                                            HAPReadBigUInt16(
                                                                    &firewallRule->host._.ipAddress._.ipv6.bytes[12]),
                                                            HAPReadBigUInt16(
                                                                    &firewallRule->host._.ipAddress._.ipv6.bytes[14]));
                                                    break;
                                                }
                                            }
                                            break;
                                        }
                                        case kHAPPlatformWiFiRouterWANHostURIType_IPAddressRange: {
                                            HAPStringBuilderAppend(&stringBuilder, "\n          - Host IP Start:");
                                            switch (firewallRule->host._.ipAddressRange.version) {
                                                case kHAPIPAddressVersion_IPv4: {
                                                    HAPStringBuilderAppend(
                                                            &stringBuilder,
                                                            "\n            - IPv4 Address: %u.%u.%u.%u",
                                                            firewallRule->host._.ipAddressRange._.ipv4.startAddress
                                                                    .bytes[0],
                                                            firewallRule->host._.ipAddressRange._.ipv4.startAddress
                                                                    .bytes[1],
                                                            firewallRule->host._.ipAddressRange._.ipv4.startAddress
                                                                    .bytes[2],
                                                            firewallRule->host._.ipAddressRange._.ipv4.startAddress
                                                                    .bytes[3]);
                                                    break;
                                                }
                                                case kHAPIPAddressVersion_IPv6: {
                                                    HAPStringBuilderAppend(
                                                            &stringBuilder,
                                                            "\n            - IPv6 Address: "
                                                            "%04X:%04X:%04X:%04X:%04X:%04X:%04X:%04X",
                                                            HAPReadBigUInt16(&firewallRule->host._.ipAddressRange._.ipv6
                                                                                      .startAddress.bytes[0]),
                                                            HAPReadBigUInt16(&firewallRule->host._.ipAddressRange._.ipv6
                                                                                      .startAddress.bytes[2]),
                                                            HAPReadBigUInt16(&firewallRule->host._.ipAddressRange._.ipv6
                                                                                      .startAddress.bytes[4]),
                                                            HAPReadBigUInt16(&firewallRule->host._.ipAddressRange._.ipv6
                                                                                      .startAddress.bytes[6]),
                                                            HAPReadBigUInt16(&firewallRule->host._.ipAddressRange._.ipv6
                                                                                      .startAddress.bytes[8]),
                                                            HAPReadBigUInt16(&firewallRule->host._.ipAddressRange._.ipv6
                                                                                      .startAddress.bytes[10]),
                                                            HAPReadBigUInt16(&firewallRule->host._.ipAddressRange._.ipv6
                                                                                      .startAddress.bytes[12]),
                                                            HAPReadBigUInt16(&firewallRule->host._.ipAddressRange._.ipv6
                                                                                      .startAddress.bytes[14]));
                                                    break;
                                                }
                                            }
                                            HAPStringBuilderAppend(&stringBuilder, "\n          - Host IP End:");
                                            switch (firewallRule->host._.ipAddressRange.version) {
                                                case kHAPIPAddressVersion_IPv4: {
                                                    HAPStringBuilderAppend(
                                                            &stringBuilder,
                                                            "\n            - IPv4 Address: %u.%u.%u.%u",
                                                            firewallRule->host._.ipAddressRange._.ipv4.endAddress
                                                                    .bytes[0],
                                                            firewallRule->host._.ipAddressRange._.ipv4.endAddress
                                                                    .bytes[1],
                                                            firewallRule->host._.ipAddressRange._.ipv4.endAddress
                                                                    .bytes[2],
                                                            firewallRule->host._.ipAddressRange._.ipv4.endAddress
                                                                    .bytes[3]);
                                                    break;
                                                }
                                                case kHAPIPAddressVersion_IPv6: {
                                                    HAPStringBuilderAppend(
                                                            &stringBuilder,
                                                            "\n            - IPv6 Address: "
                                                            "%04X:%04X:%04X:%04X:%04X:%04X:%04X:%04X",
                                                            HAPReadBigUInt16(&firewallRule->host._.ipAddressRange._.ipv6
                                                                                      .endAddress.bytes[0]),
                                                            HAPReadBigUInt16(&firewallRule->host._.ipAddressRange._.ipv6
                                                                                      .endAddress.bytes[2]),
                                                            HAPReadBigUInt16(&firewallRule->host._.ipAddressRange._.ipv6
                                                                                      .endAddress.bytes[4]),
                                                            HAPReadBigUInt16(&firewallRule->host._.ipAddressRange._.ipv6
                                                                                      .endAddress.bytes[6]),
                                                            HAPReadBigUInt16(&firewallRule->host._.ipAddressRange._.ipv6
                                                                                      .endAddress.bytes[8]),
                                                            HAPReadBigUInt16(&firewallRule->host._.ipAddressRange._.ipv6
                                                                                      .endAddress.bytes[10]),
                                                            HAPReadBigUInt16(&firewallRule->host._.ipAddressRange._.ipv6
                                                                                      .endAddress.bytes[12]),
                                                            HAPReadBigUInt16(&firewallRule->host._.ipAddressRange._.ipv6
                                                                                      .endAddress.bytes[14]));
                                                    break;
                                                }
                                            }
                                            break;
                                        }
                                    }
                                    HAPStringBuilderAppend(&stringBuilder, "\n          - ICMP Type List:");
                                    for (size_t k = 0; k < firewallRule->numICMPTypes; k++) {
                                        HAPStringBuilderAppend(&stringBuilder, "\n            - ICMP Type (%zu):", k);
                                        HAPStringBuilderAppend(&stringBuilder, "\n              - ICMP Protocol: ");
                                        switch (firewallRule->icmpTypes[k].icmpProtocol) {
                                            case kHAPPlatformWiFiRouterICMPProtocol_ICMPv4: {
                                                HAPStringBuilderAppend(&stringBuilder, "ICMPv4");
                                                break;
                                            }
                                            case kHAPPlatformWiFiRouterICMPProtocol_ICMPv6: {
                                                HAPStringBuilderAppend(&stringBuilder, "ICMPv6");
                                                break;
                                            }
                                        }
                                        if (firewallRule->icmpTypes[k].typeValueIsSet) {
                                            HAPStringBuilderAppend(
                                                    &stringBuilder,
                                                    "\n              - ICMP Type Value: %u",
                                                    firewallRule->icmpTypes[k].typeValue);
                                        }
                                    }
                                    break;
                                }
                            }
                        }
                        break;
                    }
                }
                HAPStringBuilderAppend(&stringBuilder, "\n    - LAN Firewall Configuration:");
                switch ((HAPPlatformWiFiRouterFirewallType) clients[i].lanFirewall.type) {
                    case kHAPPlatformWiFiRouterFirewallType_FullAccess: {
                        HAPStringBuilderAppend(&stringBuilder, "\n      - LAN Firewall Type: Full Access");
                        break;
                    }
                    case kHAPPlatformWiFiRouterFirewallType_Allowlist: {
                        HAPStringBuilderAppend(&stringBuilder, "\n      - LAN Firewall Type: Allowlist");
                        HAPStringBuilderAppend(&stringBuilder, "\n      - LAN Firewall Rule List:");
                        for (size_t j = 0; j < clients[i].lanFirewall.numRules; j++) {
                            const LANFirewallRule* firewallRule_ = &clients[i].lanFirewall.rules[j];
                            switch (*(const HAPPlatformWiFiRouterLANFirewallRuleType*) firewallRule_) {
                                case kHAPPlatformWiFiRouterLANFirewallRuleType_MulticastBridging: {
                                    const HAPPlatformWiFiRouterMulticastBridgingLANFirewallRule* firewallRule =
                                            &firewallRule_->multicastBridging;

                                    HAPStringBuilderAppend(
                                            &stringBuilder, "\n        - Multicast Bridging Rule (%zu):", j);
                                    HAPStringBuilderAppend(&stringBuilder, "\n          - Direction: ");
                                    switch (firewallRule->direction) {
                                        case kHAPPlatformWiFiRouterFirewallRuleDirection_Outbound: {
                                            HAPStringBuilderAppend(&stringBuilder, "0");
                                            break;
                                        }
                                        case kHAPPlatformWiFiRouterFirewallRuleDirection_Inbound: {
                                            HAPStringBuilderAppend(&stringBuilder, "1");
                                            break;
                                        }
                                    }
                                    HAPStringBuilderAppend(&stringBuilder, "\n          - Endpoint List: ");
                                    for (size_t k = 0; k < firewallRule->numPeerGroupIdentifiers; k++) {
                                        HAPStringBuilderAppend(
                                                &stringBuilder,
                                                "\n            - Client Group Identifier (%zu): %lu",
                                                k,
                                                (unsigned long) firewallRule->peerGroupIdentifiers[k]);
                                    }
                                    HAPStringBuilderAppend(&stringBuilder, "\n          - Destination IP Address:");
                                    switch (firewallRule->destinationIP.version) {
                                        case kHAPIPAddressVersion_IPv4: {
                                            HAPStringBuilderAppend(
                                                    &stringBuilder,
                                                    "\n            - IPv4 Address: %u.%u.%u.%u",
                                                    firewallRule->destinationIP._.ipv4.bytes[0],
                                                    firewallRule->destinationIP._.ipv4.bytes[1],
                                                    firewallRule->destinationIP._.ipv4.bytes[2],
                                                    firewallRule->destinationIP._.ipv4.bytes[3]);
                                            break;
                                        }
                                        case kHAPIPAddressVersion_IPv6: {
                                            HAPStringBuilderAppend(
                                                    &stringBuilder,
                                                    "\n            - IPv6 Address: "
                                                    "%04X:%04X:%04X:%04X:%04X:%04X:%04X:%04X",
                                                    HAPReadBigUInt16(&firewallRule->destinationIP._.ipv6.bytes[0]),
                                                    HAPReadBigUInt16(&firewallRule->destinationIP._.ipv6.bytes[2]),
                                                    HAPReadBigUInt16(&firewallRule->destinationIP._.ipv6.bytes[4]),
                                                    HAPReadBigUInt16(&firewallRule->destinationIP._.ipv6.bytes[6]),
                                                    HAPReadBigUInt16(&firewallRule->destinationIP._.ipv6.bytes[8]),
                                                    HAPReadBigUInt16(&firewallRule->destinationIP._.ipv6.bytes[10]),
                                                    HAPReadBigUInt16(&firewallRule->destinationIP._.ipv6.bytes[12]),
                                                    HAPReadBigUInt16(&firewallRule->destinationIP._.ipv6.bytes[14]));
                                            break;
                                        }
                                    }
                                    HAPStringBuilderAppend(
                                            &stringBuilder,
                                            "\n          - Destination Port: %u",
                                            firewallRule->destinationPort);
                                    break;
                                }
                                case kHAPPlatformWiFiRouterLANFirewallRuleType_StaticPort: {
                                    const HAPPlatformWiFiRouterStaticPortLANFirewallRule* firewallRule =
                                            &firewallRule_->staticPort;

                                    HAPStringBuilderAppend(&stringBuilder, "\n        - Static Port Rule (%zu):", j);
                                    HAPStringBuilderAppend(&stringBuilder, "\n          - Direction: ");
                                    switch (firewallRule->direction) {
                                        case kHAPPlatformWiFiRouterFirewallRuleDirection_Outbound: {
                                            HAPStringBuilderAppend(&stringBuilder, "0");
                                            break;
                                        }
                                        case kHAPPlatformWiFiRouterFirewallRuleDirection_Inbound: {
                                            HAPStringBuilderAppend(&stringBuilder, "1");
                                            break;
                                        }
                                    }
                                    HAPStringBuilderAppend(&stringBuilder, "\n          - Endpoint List: ");
                                    for (size_t k = 0; k < firewallRule->numPeerGroupIdentifiers; k++) {
                                        if (!err) {
                                            HAPStringBuilderAppend(
                                                    &stringBuilder,
                                                    "\n            - Client Group Identifier (%zu): %lu",
                                                    k,
                                                    (unsigned long) firewallRule->peerGroupIdentifiers[k]);
                                        }
                                    }
                                    HAPStringBuilderAppend(&stringBuilder, "\n          - Protocol: ");
                                    switch (firewallRule->transportProtocol) {
                                        case kHAPPlatformWiFiRouterTransportProtocol_TCP: {
                                            HAPStringBuilderAppend(&stringBuilder, "TCP");
                                            break;
                                        }
                                        case kHAPPlatformWiFiRouterTransportProtocol_UDP: {
                                            HAPStringBuilderAppend(&stringBuilder, "UDP");
                                            break;
                                        }
                                    }
                                    HAPStringBuilderAppend(
                                            &stringBuilder,
                                            "\n          - Destination Port Start: %u",
                                            firewallRule->destinationPortRange.startPort);
                                    if (firewallRule->destinationPortRange.endPort !=
                                        firewallRule->destinationPortRange.startPort) {
                                        HAPStringBuilderAppend(
                                                &stringBuilder,
                                                "\n          - Destination Port End: %u",
                                                firewallRule->destinationPortRange.endPort);
                                    }
                                    break;
                                }
                                case kHAPPlatformWiFiRouterLANFirewallRuleType_DynamicPort: {
                                    const HAPPlatformWiFiRouterDynamicPortLANFirewallRule* firewallRule =
                                            &firewallRule_->dynamicPort._;

                                    HAPStringBuilderAppend(&stringBuilder, "\n        - Dynamic Port Rule (%zu):", j);
                                    HAPStringBuilderAppend(&stringBuilder, "\n          - Direction: ");
                                    switch (firewallRule->direction) {
                                        case kHAPPlatformWiFiRouterFirewallRuleDirection_Outbound: {
                                            HAPStringBuilderAppend(&stringBuilder, "0");
                                            break;
                                        }
                                        case kHAPPlatformWiFiRouterFirewallRuleDirection_Inbound: {
                                            HAPStringBuilderAppend(&stringBuilder, "1");
                                            break;
                                        }
                                    }
                                    HAPStringBuilderAppend(&stringBuilder, "\n          - Endpoint List:");
                                    for (size_t k = 0; k < firewallRule->numPeerGroupIdentifiers; k++) {
                                        HAPStringBuilderAppend(
                                                &stringBuilder,
                                                "\n            - Client Group Identifier (%zu): %lu",
                                                k,
                                                (unsigned long) firewallRule->peerGroupIdentifiers[k]);
                                    }
                                    HAPStringBuilderAppend(&stringBuilder, "\n          - Protocol: ");
                                    switch (firewallRule->transportProtocol) {
                                        case kHAPPlatformWiFiRouterTransportProtocol_TCP: {
                                            HAPStringBuilderAppend(&stringBuilder, "TCP");
                                            break;
                                        }
                                        case kHAPPlatformWiFiRouterTransportProtocol_UDP: {
                                            HAPStringBuilderAppend(&stringBuilder, "UDP");
                                            break;
                                        }
                                    }
                                    HAPStringBuilderAppend(&stringBuilder, "\n          - Advertisement Protocol: ");
                                    switch (firewallRule->serviceType.advertisementProtocol) {
                                        case kHAPPlatformWiFiRouterDynamicPortAdvertisementProtocol_DNSSD: {
                                            HAPStringBuilderAppend(&stringBuilder, "DNS-SD");
                                            break;
                                        }
                                        case kHAPPlatformWiFiRouterDynamicPortAdvertisementProtocol_SSDP: {
                                            HAPStringBuilderAppend(&stringBuilder, "SSDP");
                                            break;
                                        }
                                    }
                                    HAPStringBuilderAppend(&stringBuilder, "\n          - Flags: [");
                                    if (firewallRule->advertisementOnly) {
                                        HAPStringBuilderAppend(&stringBuilder, "advertisement only");
                                    }
                                    HAPStringBuilderAppend(&stringBuilder, "]");
                                    switch (firewallRule->serviceType.advertisementProtocol) {
                                        case kHAPPlatformWiFiRouterDynamicPortAdvertisementProtocol_DNSSD: {
                                            if (firewallRule->serviceType._.dns_sd.serviceType) {
                                                HAPStringBuilderAppend(&stringBuilder, "\n          - Service Type:");
                                                HAPStringBuilderAppend(
                                                        &stringBuilder,
                                                        "\n            - Name: %s",
                                                        firewallRule->serviceType._.dns_sd.serviceType);
                                            }
                                            break;
                                        }
                                        case kHAPPlatformWiFiRouterDynamicPortAdvertisementProtocol_SSDP: {
                                            if (firewallRule->serviceType._.ssdp.serviceTypeURI) {
                                                HAPStringBuilderAppend(&stringBuilder, "\n          - Service Type:");
                                                HAPStringBuilderAppend(
                                                        &stringBuilder,
                                                        "\n            - Name: %s",
                                                        firewallRule->serviceType._.ssdp.serviceTypeURI);
                                            }
                                            break;
                                        }
                                    }
                                    break;
                                }
                                case kHAPPlatformWiFiRouterLANFirewallRuleType_StaticICMP: {
                                    const HAPPlatformWiFiRouterStaticICMPLANFirewallRule* firewallRule =
                                            &firewallRule_->staticICMP;

                                    HAPStringBuilderAppend(&stringBuilder, "\n        - Static ICMP Rule (%zu):", j);
                                    HAPStringBuilderAppend(&stringBuilder, "\n          - Direction: ");
                                    switch (firewallRule->direction) {
                                        case kHAPPlatformWiFiRouterFirewallRuleDirection_Outbound: {
                                            HAPStringBuilderAppend(&stringBuilder, "0");
                                            break;
                                        }
                                        case kHAPPlatformWiFiRouterFirewallRuleDirection_Inbound: {
                                            HAPStringBuilderAppend(&stringBuilder, "1");
                                            break;
                                        }
                                    }
                                    HAPStringBuilderAppend(&stringBuilder, "\n          - Endpoint List: ");
                                    for (size_t k = 0; k < firewallRule->numPeerGroupIdentifiers; k++) {
                                        if (!err) {
                                            HAPStringBuilderAppend(
                                                    &stringBuilder,
                                                    "\n            - Client Group Identifier (%zu): %lu",
                                                    k,
                                                    (unsigned long) firewallRule->peerGroupIdentifiers[k]);
                                        }
                                    }
                                    HAPStringBuilderAppend(&stringBuilder, "\n          - ICMP Type List:");
                                    for (size_t k = 0; k < firewallRule->numICMPTypes; k++) {
                                        HAPStringBuilderAppend(&stringBuilder, "\n            - ICMP Type (%zu):", k);
                                        HAPStringBuilderAppend(&stringBuilder, "\n              - ICMP Protocol: ");
                                        switch (firewallRule->icmpTypes[k].icmpProtocol) {
                                            case kHAPPlatformWiFiRouterICMPProtocol_ICMPv4: {
                                                HAPStringBuilderAppend(&stringBuilder, "ICMPv4");
                                                break;
                                            }
                                            case kHAPPlatformWiFiRouterICMPProtocol_ICMPv6: {
                                                HAPStringBuilderAppend(&stringBuilder, "ICMPv6");
                                                break;
                                            }
                                        }
                                        if (firewallRule->icmpTypes[k].typeValueIsSet) {
                                            HAPStringBuilderAppend(
                                                    &stringBuilder,
                                                    "\n              - ICMP Type Value: %u",
                                                    firewallRule->icmpTypes[k].typeValue);
                                        }
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }
                HAPAssert(!HAPStringBuilderDidOverflow(&stringBuilder));
                HAPLogCharacteristicInfo(
                        &kHAPLog_Default,
                        request.characteristic,
                        request.service,
                        request.accessory,
                        "%s",
                        HAPStringBuilderGetString(&stringBuilder));
            }
        }
    }
}

int main() {
    HAPError err;
    HAPPlatformCreate();

    // Prepare key-value store.
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    HAPControllerPublicKey controllerPublicKey;
    TestPrepareKeyValueStore(controllerPairingID, &controllerPublicKey);

    // Prepare accessory server storage.
    HAPIPSession ipSessions[kHAPIPSessionStorage_DefaultNumElements];
    IPSessionState ipSessionStates[HAPArrayCount(ipSessions)];
    HAPIPReadContext ipReadContexts[kAttributeCount];
    HAPIPWriteContext ipWriteContexts[kAttributeCount];
    uint8_t ipScratchBuffer[kHAPIPSession_DefaultScratchBufferSize];
    InitializeIPSessions(ipSessions, ipSessionStates, HAPArrayCount(ipSessions));
    HAPIPAccessoryServerStorage ipAccessoryServerStorage = { .sessions = ipSessions,
                                                             .numSessions = HAPArrayCount(ipSessions),
                                                             .readContexts = ipReadContexts,
                                                             .numReadContexts = HAPArrayCount(ipReadContexts),
                                                             .writeContexts = ipWriteContexts,
                                                             .numWriteContexts = HAPArrayCount(ipWriteContexts),
                                                             .scratchBuffer = { .bytes = ipScratchBuffer,
                                                                                .numBytes = sizeof ipScratchBuffer } };

    // Initialize accessory server.
    HAPAssert(HAPGetCompatibilityVersion() == HAP_COMPATIBILITY_VERSION);
    HAPAccessoryServer accessoryServer;
    HAPAccessoryServerCreate(
            &accessoryServer,
            &(const HAPAccessoryServerOptions) { .maxPairings = kHAPPairingStorage_MinElements,
                                                 .ip = { .transport = &kHAPAccessoryServerTransport_IP,
                                                         .accessoryServerStorage = &ipAccessoryServerStorage } },
            &platform,
            &(const HAPAccessoryServerCallbacks) { .handleUpdatedState =
                                                           HAPAccessoryServerHelperHandleUpdatedAccessoryServerState },
            NULL);

    // Start accessory server.
    HAPAccessoryServerStart(&accessoryServer, &accessory);
    HAPPlatformClockAdvance(0);
    HAPAssert(HAPAccessoryServerGetState(&accessoryServer) == kHAPAccessoryServerState_Running);

    // Discover IP accessory server.
    HAPAccessoryServerInfo serverInfo;
    HAPNetworkPort serverPort;
    err = HAPDiscoverIPAccessoryServer(HAPNonnull(platform.ip.serviceDiscovery), &serverInfo, &serverPort);
    HAPAssert(!err);
    HAPAssert(!serverInfo.statusFlags.isNotPaired);

    // Create fake security session.
    HAPSession session;
    TestCreateFakeSecuritySession(&session, &accessoryServer, controllerPairingID);

    // Validate version.
    // See HomeKit Accessory Protocol Specification R17
    // Section 9.48 Wi-Fi Router
    {
        const HAPStringCharacteristicReadRequest request = { .transportType = kHAPTransportType_IP,
                                                             .session = &session,
                                                             .characteristic = &wiFiRouterVersionCharacteristic,
                                                             .service = &wiFiRouterService,
                                                             .accessory = &accessory };
        char bytes[1024];
        err = request.characteristic->callbacks.handleRead(&accessoryServer, &request, bytes, sizeof bytes, NULL);
        HAPAssert(!err);
        HAPAssert(HAPStringAreEqual(bytes, kHAPWiFiRouter_Version));
    }

    // Run scenarios.

    HAPLog(&kHAPLog_Default, "SCENARIO 1: Add router.");
    Scenario1(&accessoryServer, &session);
    ListAllClients(&accessoryServer, &session);

    HAPLog(&kHAPLog_Default, "SCENARIO 2: Add Network Client Profile with MAC credentials.");
    Scenario2(&accessoryServer, &session);
    ListAllClients(&accessoryServer, &session);

    HAPLog(&kHAPLog_Default, " Add firewall WLAN configuration rule.");
    addFirewalWANConfigurationRule();
    ListAllClients(&accessoryServer, &session);

    HAPLog(&kHAPLog_Default, "SCENARIO 3: Add Network Client Profile with PSK credentials.");
    HAPPlatformWiFiRouterClientIdentifier clientIDC;
    HAPWiFiWPAPersonalCredential uniquePSKC;
    HAPMACAddress clientMACAddressC;
    ClientName clientNameC;
    HAPIPAddress clientIPAddressC;
    Scenario3(&accessoryServer, &session, &clientIDC, &uniquePSKC, &clientMACAddressC, &clientNameC, &clientIPAddressC);
    ListAllClients(&accessoryServer, &session);

    HAPLog(&kHAPLog_Default, "SCENARIO 4: Update Network Client Profile.");
    Scenario4(&accessoryServer, &session, clientIDC, &uniquePSKC, &clientMACAddressC, &clientNameC, &clientIPAddressC);
    ListAllClients(&accessoryServer, &session);

    HAPLog(&kHAPLog_Default, "SCENARIO 5: Network Access Violations.");
    Scenario5(&accessoryServer, &session);

    HAPLog(&kHAPLog_Default, "SCENARIO 6: Remove Network Client Profile.");
    Scenario6(&accessoryServer, &session, &clientMACAddressC, clientIDC);
    ListAllClients(&accessoryServer, &session);

    HAPLog(&kHAPLog_Default, "SCENARIO 7: Remove Router from Home.");
    Scenario7(&accessoryServer, &session);
}

#else

int main() {
    HAPLog(&kHAPLog_Default, "This test is not enabled. Please enable the WiFi Router feature to run this test.");
    return 0;
}

#endif
