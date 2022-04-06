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

#ifndef HAP_PLATFORM_WIFI_ROUTER_H
#define HAP_PLATFORM_WIFI_ROUTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPBase.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * @file
 *
 * This platform module allows Wi-Fi router accessories to support managed network configuration.
 *
 * For purposes of observable network behavior, deployments that contain one or more Wi-Fi satellites
 * or a mesh consisting of multiple routers must behave as if they were a single logical router that
 * all network clients are connected to directly.
 *
 *
 * IP requirements:
 *
 * The router must implement a DHCP server to allocate IPv4 addresses to network clients. The router should place
 * all network clients in a single subnet.
 *
 * The router must accept and respond to ICMP (v4 and v6) echo requests on all local network interfaces.
 *
 * The router should filter ARP and NDP traffic so that a network client can only discover the MAC address of
 * network clients with which it would be allowed to communicate based on the applicable LAN firewall rules.
 *
 * If the router places network clients in multiple subnets, it must implement bridging for broadcast and multicast
 * traffic as allowed by multicast LAN rules.
 *
 *
 * NTP requirements:
 *
 * The router must implement an NTP server. The router's DHCP offers must instruct network clients to use
 * this NTP server (DHCP option 42). The router's NTP server must be pre-configured with a suitable pool of
 * upstream NTP servers, unless another reliable reference time source is available (e.g., GPS).
 *
 *
 * DNS requirements:
 *
 * The router must implement a DNS server. The router's DHCP offers must instruct network clients to use
 * this DNS server (DHCP option 6).
 *
 * The router's DNS server must process DNS requests from a network client based on the WAN firewall rules applicable
 * to that network client:
 *
 * - If the network client has Full WAN Access the DNS request is allowed.
 *
 * - If the DNS name being queried matches the DNS name or pattern for any applicable DNS-based WAN firewall rule,
 *   the request is allowed.
 *
 * - Otherwise the request must be rejected with a response code of "Refused" (rcode 5) without forwarding it
 *   to an upstream DNS server.
 *
 * For the purpose of implementing DNS-based WAN firewall rules, the router must retain details of any A, AAAA
 * or CNAME records encountered when processing DNS requests on behalf of a network client, such that a network client
 * that has resolved a host name matching a WAN rule can subsequently communicate with that host as allowed by
 * the rule. A new conversation with an IP address must be allowed to be established in relation to a specific
 * WAN rule for as long as, and no longer than, a network client that has correctly resolved a name matching the rule
 * to that IP address can consider that address to be valid given the TTLs of the DNS records involved.
 * Once a conversation (e.g., a TCP connection) has been established, it must not be affected by the TTL of any of
 * the relevant DNS records expiring.
 *
 * One implementation approach is to "instantiate" IP-address based versions of each matching WAN rule when
 * a DNS response is forwarded to the network client. Note that in cases where a name resolves to an address via
 * one or more CNAMEs, the CNAME record(s) earlier in the chain can have a longer TTL than later records or the
 * final A/AAAA record. This means that a subsequent connection attempt by a network client can result in a DNS query
 * starting at a point somewhere along the CNAME chain, rather than at the original name matching the DNS-based rule.
 *
 * To simplify the implementation of DNS-based WAN rules, it is recommended that the router's DNS server modify
 * DNS responses to clients as follows:
 *
 * - Cap the TTL of any CNAME record in a DNS response to be less than or equal to the smallest TTL of any subsequent
 *   CNAME or A/AAAA record it points to.
 *
 * - Cap the TTL of any A, AAAA, and CNAME records to a small value, e.g., 10 or 15 seconds.
 *
 * This avoids the case of subsequent DNS requests starting part-way down a CNAME chain, as well as keeping the volume
 * and validity period of any instantiated rules small. If the router performs any TTL modifications of this kind
 * it must implement a DNS cache (based on the true TTL of records received from upstream) to mitigate the additional
 * latency and DNS request load that would otherwise be introduced.
 *
 *
 * Bonjour and SSDP requirements:
 *
 * If the router places network clients in different subnets, it must implement mDNS and SSDP proxying to enable
 * network clients to advertise and discover Bonjour/DNS-SD and SSDP services.
 *
 * The router must support dynamically instantiating discovery-based LAN firewall rules based on the Bonjour/DNS-SD
 * services advertised or discovered by a network client. New or updated Dynamic Port Rules must be given effect
 * without waiting for an unsolicited advertisement from the client. To this end, the router should itself perform
 * a DNS-SD query or SDP search towards the relevant clients when a Dynamic Port Rule is added or updated.
 *
 * The router should filter mDNS and SSDP traffic so that a network client can only discover network clients and
 * services with which it would be allowed to communicate based on the applicable LAN firewall rules.
 *
 *
 * Other requirements:
 *
 * - The router must support Wi-Fi networks using WPA2 Personal security with multiple per-client PSKs.
 *
 * - The router must support at least 1000 network client profiles with credentials based on a MAC address or
 *   per-client PSK.
 *
 * - The router must support at least 8192 WAN firewall rules comprising of a 253 byte DNS string.
 *
 * - The router must support at least 8192 LAN firewall rules comprising of a 32 byte service type string.
 *
 *
 * Limitations:
 *
 * There is currently no support for securely identifying (wired) Ethernet network clients. Router must support
 * enforcing WAN firewall rules for Ethernet network clients with a MAC-address-based network client profile,
 * and should support enforcing LAN firewall rules in the case where the network client is directly connected
 * to an Ethernet port on the router or one of its satellites.
 */

/**
 * Wi-Fi router.
 */
typedef struct HAPPlatformWiFiRouter HAPPlatformWiFiRouter;
typedef struct HAPPlatformWiFiRouter* HAPPlatformWiFiRouterRef;
HAP_NONNULL_SUPPORT(HAPPlatformWiFiRouter)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Identifier.
 */
typedef uint32_t HAPPlatformWiFiRouterIdentifier;

//----------------------------------------------------------------------------------------------------------------------

/**
 * WAN identifier.
 *
 * - 0: Invalid.
 * - 1 - 1023: Reserved for predefined purposes.
 * - 1024 - 2047: Can be used by vendors to represent non-standard WANs.
 */
typedef HAPPlatformWiFiRouterIdentifier HAPPlatformWiFiRouterWANIdentifier;

/** Main WAN. */
#define kHAPPlatformWiFiRouterWANIdentifier_Main ((HAPPlatformWiFiRouterWANIdentifier) 1)

//----------------------------------------------------------------------------------------------------------------------

/**
 * Network client group identifier.
 *
 * - 0: Invalid.
 */
typedef HAPPlatformWiFiRouterIdentifier HAPPlatformWiFiRouterGroupIdentifier;

/** Main network client group. */
#define kHAPPlatformWiFiRouterGroupIdentifier_Main ((HAPPlatformWiFiRouterGroupIdentifier) 1)

/** Restricted network client group. */
#define kHAPPlatformWiFiRouterGroupIdentifier_Restricted ((HAPPlatformWiFiRouterGroupIdentifier) 3)

//----------------------------------------------------------------------------------------------------------------------

/**
 * Network client profile identifier.
 *
 * - 0: Invalid.
 */
typedef HAPPlatformWiFiRouterIdentifier HAPPlatformWiFiRouterClientIdentifier;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @defgroup FirewallRule Firewall rules.
 *
 * The firewall configuration is based on a "default deny" policy: Any traffic not explicitly allowed by a
 * firewall rule must be blocked by the router. This applies equally to unicast, multicast, and broadcast traffic.
 * All firewall rules are "allow" rules. This means that the firewall rules can be treated as an unordered set:
 * traffic allowed by any applicable rule is allowed.
 *
 * When unicast traffic from a network client on the local network is blocked, the router must signal this
 * to the client: i.e., blocked traffic must be "rejected" rather than silently "dropped" where the
 * relevant protocol permits it. For IP protocols, the router should use an ICMP "Destination Unreachable" or
 * "Communication Administratively Prohibited" message (type 3 / code 13) for this purpose.
 *
 * Several different types of firewall rules exist, but in general every rule specifies a direction
 * (outbound or inbound), a set of endpoints, as well as a protocol and protocol specific details.
 * At a high level, rules can be categorized into those that apply to traffic between a network client and the Internet
 * ("WAN rules"), and those that apply to traffic between network clients on the local network ("LAN rules").
 *
 * Unless specified otherwise, rules for IP protocols are treated as covering all protocol versions,
 * i.e., TCP and UDP rules apply to both IPv4 and IPv6, and ICMP rules also include ICMP6.
 *
 * In general all firewall rules are stateful and apply to a conversation as a whole, rather than just
 * to individual packets. For example, a rule that allows outbound TCP connections to "api.example.com" port 443
 * must be interpreted as allowing not only outbound TCP packets to that endpoint and port, but also the associated
 * response packets going in the opposite direction. This means the direction of a rule specifies the direction
 * in which direction the conversation is initiated, not the direction of all individual packets.
 * In the absence of further traffic, the router must maintain the state related to an established conversation for
 * a minimum of 2 minutes but no longer than 10 minutes. A conversation is defined in protocol-specific ways as follows:
 *
 * - For TCP, a conversation is synonymous with a connection, and is identified by the (source IP, source port,
 *   destination IP, destination port) 4-tuple. It is established by a SYN packet from the source to the destination.
 *   The conversation covers both outbound and inbound TCP packets between those IP/port pairs as well as
 *   related ICMP messages (e.g., Destination Unreachable).
 *
 * - For unicast UDP, a conversation is identified by the (source IP, source port, destination IP, destination port)
 *   4-tuple and is established by any UDP packet from the source to the destination. It covers UDP packets between
 *   those IP/port pairs in either direction as well as related ICMP messages.
 *
 * - For unicast ICMP, only Echo Request messages are considered to establish a conversation, which is identified
 *   by the (source IP, destination IP, Echo Identifier) triplet.
 *
 * All other traffic (including broadcast, multicast, and non-IP protocols) is treated in a stateless manner.
 *
 *
 * Firewall rules on network client profiles may be queried using:
 * - HAPPlatformWiFiRouterClientGetWANFirewallType
 * - HAPPlatformWiFiRouterClientEnumerateWANFirewallRules (if WAN firewall type is Allowlist)
 * - HAPPlatformWiFiRouterClientGetLANFirewallType
 * - HAPPlatformWiFiRouterClientEnumerateLANFirewallRules (if LAN firewall type is Allowlist)
 *
 * Firewall rules on network client profiles may be configured using:
 * - HAPPlatformWiFiRouterClientResetWANFirewall
 * - HAPPlatformWiFiRouterClientAddWANFirewallRule
 * - HAPPlatformWiFiRouterClientResetLANFirewall
 * - HAPPlatformWiFiRouterClientAddLANFirewallRule
 */
/**@{*/

/**
 * Firewall type.
 */
HAP_ENUM_BEGIN(uint8_t, HAPPlatformWiFiRouterFirewallType) {
    /** Full Access. No access restrictions on WAN/within same LAN. */
    kHAPPlatformWiFiRouterFirewallType_FullAccess = 1,

    /** Allowlist. No access except those specified in the rule list. */
    kHAPPlatformWiFiRouterFirewallType_Allowlist
} HAP_ENUM_END(uint8_t, HAPPlatformWiFiRouterFirewallType);

/**
 * Firewall rule direction.
 */
HAP_ENUM_BEGIN(uint8_t, HAPPlatformWiFiRouterFirewallRuleDirection) {
    /** Firewall rule affects traffic from conversations initiated locally. */
    kHAPPlatformWiFiRouterFirewallRuleDirection_Outbound = 1,

    /** Firewall rule affects traffic from conversations initiated by peer. */
    kHAPPlatformWiFiRouterFirewallRuleDirection_Inbound
} HAP_ENUM_END(uint8_t, HAPPlatformWiFiRouterFirewallRuleDirection);

/**
 * Transport protocol.
 */
HAP_ENUM_BEGIN(uint8_t, HAPPlatformWiFiRouterTransportProtocol) { /** TCP. */
                                                                  kHAPPlatformWiFiRouterTransportProtocol_TCP = 1,

                                                                  /** UDP. */
                                                                  kHAPPlatformWiFiRouterTransportProtocol_UDP
} HAP_ENUM_END(uint8_t, HAPPlatformWiFiRouterTransportProtocol);

/**
 * ICMP protocol.
 */
HAP_ENUM_BEGIN(uint8_t, HAPPlatformWiFiRouterICMPProtocol) { /** ICMPv4. */
                                                             kHAPPlatformWiFiRouterICMPProtocol_ICMPv4 = 1,

                                                             /** ICMPv6. */
                                                             kHAPPlatformWiFiRouterICMPProtocol_ICMPv6
} HAP_ENUM_END(uint8_t, HAPPlatformWiFiRouterICMPProtocol);

/**
 * ICMP type.
 */
typedef struct {
    /** ICMP protocol. */
    HAPPlatformWiFiRouterICMPProtocol icmpProtocol;

    /**
     * Optional. ICMP type value to match.
     * If absent, all types of ICMP messages for the specified protocol match this record.
     */
    uint8_t typeValue;
    bool typeValueIsSet;
} HAPPlatformWiFiRouterICMPType;

/**
 * Maximum number of ICMP types per firewall rule.
 */
#define kHAPPlatformWiFiRouterFirewallRule_MaxICMPTypes ((size_t) 16)

//----------------------------------------------------------------------------------------------------------------------

/**
 * WAN firewall rule type.
 */
HAP_ENUM_BEGIN(uint8_t, HAPPlatformWiFiRouterWANFirewallRuleType) { /** Port. */
                                                                    kHAPPlatformWiFiRouterWANFirewallRuleType_Port = 1,

                                                                    /** ICMP. */
                                                                    kHAPPlatformWiFiRouterWANFirewallRuleType_ICMP
} HAP_ENUM_END(uint8_t, HAPPlatformWiFiRouterWANFirewallRuleType);

/**
 * WAN firewall rule.
 *
 * The direction of WAN firewall rules is always "outbound", i.e., allowed connections must always be initiated
 * by the local network client. The Internet endpoint can be specified either as an IP address range (including
 * the special cases of a single address, and "any address"), or as a DNS name or wildcard pattern.
 *
 * A "Full WAN Access" rule allows traffic to any Internet endpoints. Note that "Full WAN Access" is represented
 * as the special firewall type kHAPPlatformWiFiRouterFirewallType_FullAccess, however it can conceptually be
 * thought of as a rule.
 *
 * If the router implements any NAT management services, including but not limited to Internet Gateway Device
 * Protocol (IGD), NAT Port-Mapping Protocol (NAT-PMP), and Port Control Protocol (PCP), use of these services
 * must be restricted to clients with Full WAN Access. A client must not be allowed to communicate with WAN hosts
 * in any way not explicitly allowed by its WAN Firewall Configuration, even if a NAT mapping was created
 * to target that client via a NAT management protocol, or a port forwarding configuration, or similar functionality
 * offered by the router.
 *
 * - May be safely cast to HAPPlatformWiFiRouterWANFirewallRuleType, and to the corresponding concrete type:
 *   - kHAPPlatformWiFiRouterWANFirewallRuleType_Port
 *     => May be safely cast to HAPPlatformWiFiRouterPortWANFirewallRule.
 *   - kHAPPlatformWiFiRouterWANFirewallRuleType_ICMP
 *     => May be safely cast to HAPPlatformWiFiRouterICMPWANFirewallRule.
 *
 * - WAN firewall rules apply to all available WAN links.
 */
typedef void HAPPlatformWiFiRouterWANFirewallRule;

/**
 * WAN host URI type.
 */
HAP_ENUM_BEGIN(uint8_t, HAPPlatformWiFiRouterWANHostURIType) { /** Any host with any protocol (IPv4 or IPv6). */
                                                               kHAPPlatformWiFiRouterWANHostURIType_Any = 1,

                                                               /** DNS name or pattern of the host(s). */
                                                               kHAPPlatformWiFiRouterWANHostURIType_DNSNamePattern,

                                                               /** IP address. */
                                                               kHAPPlatformWiFiRouterWANHostURIType_IPAddress,

                                                               /** IP address range. */
                                                               kHAPPlatformWiFiRouterWANHostURIType_IPAddressRange
} HAP_ENUM_END(uint8_t, HAPPlatformWiFiRouterWANHostURIType);

/**
 * WAN host URI.
 */
typedef struct {
    /** WAN host URI type. */
    HAPPlatformWiFiRouterWANHostURIType type;

    /** Type specific parameters. */
    union {
        /**
         * Fully-qualified DNS name or pattern of the host(s).
         *
         * - DNS names must adhere to RFC 1123: 1 to 253 characters in length, consisting of a sequence of labels
         *   delimited by dots ("."). Each label must be 1 to 63 characters in length, contain only
         *   ASCII letters ("a"-"Z"), digits ("0"-"9"), or hyphens ("-") and must not start or end with a hyphen.
         *
         * - Patterns follow the syntax of DNS names, but additionally allow the wildcard character "*" to be used up to
         *   twice per label to match 0 or more characters within that label. Note that the wildcard never matches a dot
         *   (e.g., "*.example.com" matches "api.example.com" but not "api.us.example.com").
         *
         * - A valid name or pattern must be fully qualified, i.e., consist of at least two labels.
         *   The final label must not be fully numeric, and must not be the "local" pseudo-TLD.
         *   A pattern must end with at least two literal (non-wildcard) labels.
         *
         * - Examples:
         *   Valid: "example.com", "*.example.com", "video*.example.com", "api*.*.example.com", "*-prod-*.example.com"
         *   Invalid: "ipcamera", "ipcamera.local", "*", "*.com", "8.8.8.8"
         */
        const char* dnsNamePattern;

        /** IP address. */
        HAPIPAddress ipAddress;

        /** IP address range. */
        HAPIPAddressRange ipAddressRange;
    } _;
} HAPPlatformWiFiRouterWANHostURI;

/**
 * Port WAN firewall rule.
 *
 * A single WAN firewall rule for TCP or UDP traffic.
 */
typedef struct {
    /**
     * Type. Must be kHAPPlatformWiFiRouterWANFirewallRuleType_Port.
     */
    HAPPlatformWiFiRouterWANFirewallRuleType type;

    /** Transport protocol. */
    HAPPlatformWiFiRouterTransportProtocol transportProtocol;

    /** WAN host URI. */
    HAPPlatformWiFiRouterWANHostURI host;

    /** WAN port range. */
    HAPNetworkPortRange hostPortRange;
} HAPPlatformWiFiRouterPortWANFirewallRule;
HAP_STATIC_ASSERT(
        HAP_OFFSETOF(HAPPlatformWiFiRouterPortWANFirewallRule, type) == 0,
        HAPPlatformWiFiRouterPortWANFirewallRule_type);

/**
 * ICMP WAN firewall rule.
 *
 * A WAN firewall rule for ICMP traffic.
 */
typedef struct {
    /**
     * Type. Must be kHAPPlatformWiFiRouterWANFirewallRuleType_ICMP.
     */
    HAPPlatformWiFiRouterWANFirewallRuleType type;

    /** WAN host URI. */
    HAPPlatformWiFiRouterWANHostURI host;

    /** ICMP types. */
    HAPPlatformWiFiRouterICMPType icmpTypes[kHAPPlatformWiFiRouterFirewallRule_MaxICMPTypes];

    /** Number of ICMP types. At least 1 is always defined. */
    size_t numICMPTypes;
} HAPPlatformWiFiRouterICMPWANFirewallRule;
HAP_STATIC_ASSERT(
        HAP_OFFSETOF(HAPPlatformWiFiRouterICMPWANFirewallRule, type) == 0,
        HAPPlatformWiFiRouterICMPWANFirewallRule_type);

//----------------------------------------------------------------------------------------------------------------------

/**
 * LAN firewall rule type.
 */
HAP_ENUM_BEGIN(uint8_t, HAPPlatformWiFiRouterLANFirewallRuleType) {
    /** Multicast bridging. */
    kHAPPlatformWiFiRouterLANFirewallRuleType_MulticastBridging = 1,

    /** Static port. */
    kHAPPlatformWiFiRouterLANFirewallRuleType_StaticPort,

    /** Dynamic port. */
    kHAPPlatformWiFiRouterLANFirewallRuleType_DynamicPort,

    /** Static ICMP. */
    kHAPPlatformWiFiRouterLANFirewallRuleType_StaticICMP
} HAP_ENUM_END(uint8_t, HAPPlatformWiFiRouterLANFirewallRuleType);

/**
 * LAN firewall rule.
 *
 * LAN firewall rules can be specified in both "outbound" and "inbound" direction. For traffic between network clients
 * on the local network to be allowed, both an "outbound" rule for the network client originating the conversation
 * and an "inbound" rule for the network client receiving it must be present. Endpoints for LAN rules are specified
 * as a list of network client group identifiers and can be further restricted to specific IP addresses.
 *
 * A "Full LAN Access" rule allows traffic (IP or otherwise) to or from any local network clients. Note that it does
 * not override the requirement for the other network client to also have a rule allowing the traffic. "Full LAN Access"
 * is represented as the special firewall type kHAPPlatformWiFiRouterFirewallType_FullAccess, however it can
 * conceptually be thought of as a rule.
 *
 * - May be safely cast to HAPPlatformWiFiRouterLANFirewallRuleType, and to the corresponding concrete type:
 *   - kHAPPlatformWiFiRouterLANFirewallRuleType_MulticastBridging
 *     => May be safely cast to HAPPlatformWiFiRouterMulticastBridgingLANFirewallRule.
 *   - kHAPPlatformWiFiRouterLANFirewallRuleType_StaticPort
 *     => May be safely cast to HAPPlatformWiFiRouterStaticPortLANFirewallRule.
 *   - kHAPPlatformWiFiRouterLANFirewallRuleType_DynamicPort
 *     => May be safely cast to HAPPlatformWiFiRouterDynamicPortLANFirewallRule.
 *   - kHAPPlatformWiFiRouterLANFirewallRuleType_StaticICMP
 *     => May be safely cast to HAPPlatformWiFiRouterStaticICMPLANFirewallRule.
 */
typedef void HAPPlatformWiFiRouterLANFirewallRule;

/**
 * Maximum number of peer network client group identifiers per LAN firewall rule.
 */
#define kHAPPlatformWiFiRouterLANFirewallRule_MaxPeerGroups ((size_t) 32)

/**
 * Multicast bridging LAN firewall rule.
 *
 * A LAN firewall rule that allows UDP broadcast or multicast traffic.
 */
typedef struct {
    /**
     * Type. Must be kHAPPlatformWiFiRouterLANFirewallRuleType_MulticastBridging.
     */
    HAPPlatformWiFiRouterLANFirewallRuleType type;

    /** Direction. */
    HAPPlatformWiFiRouterFirewallRuleDirection direction;

    /** Peer network client group identifiers. */
    HAPPlatformWiFiRouterGroupIdentifier peerGroupIdentifiers[kHAPPlatformWiFiRouterLANFirewallRule_MaxPeerGroups];

    /** Number of peer network client group identifiers. At least 1 is always defined. */
    size_t numPeerGroupIdentifiers;

    /** Destination multicast or broadcast IP address. */
    HAPIPAddress destinationIP;

    /** Destination port. */
    HAPNetworkPort destinationPort;
} HAPPlatformWiFiRouterMulticastBridgingLANFirewallRule;
HAP_STATIC_ASSERT(
        HAP_OFFSETOF(HAPPlatformWiFiRouterMulticastBridgingLANFirewallRule, type) == 0,
        HAPPlatformWiFiRouterMulticastBridgingLANFirewallRule_type);

/**
 * Static port LAN firewall rule.
 *
 * A LAN firewall rule that allows TCP or unicast UDP traffic between static ports.
 */
typedef struct {
    /**
     * Type. Must be kHAPPlatformWiFiRouterLANFirewallRuleType_StaticPort.
     */
    HAPPlatformWiFiRouterLANFirewallRuleType type;

    /** Direction. */
    HAPPlatformWiFiRouterFirewallRuleDirection direction;

    /** Transport protocol. */
    HAPPlatformWiFiRouterTransportProtocol transportProtocol;

    /** Peer network client group identifiers. */
    HAPPlatformWiFiRouterGroupIdentifier peerGroupIdentifiers[kHAPPlatformWiFiRouterLANFirewallRule_MaxPeerGroups];

    /** Number of peer network client group identifiers. At least 1 is always defined. */
    size_t numPeerGroupIdentifiers;

    /** Destination port range. */
    HAPNetworkPortRange destinationPortRange;
} HAPPlatformWiFiRouterStaticPortLANFirewallRule;
HAP_STATIC_ASSERT(
        HAP_OFFSETOF(HAPPlatformWiFiRouterStaticPortLANFirewallRule, type) == 0,
        HAPPlatformWiFiRouterStaticPortLANFirewallRule_type);

/**
 * Advertisement protocol to negotiate dynamic port information.
 */
HAP_ENUM_BEGIN(uint8_t, HAPPlatformWiFiRouterDynamicPortAdvertisementProtocol) {
    /** DNS-SD. */
    kHAPPlatformWiFiRouterDynamicPortAdvertisementProtocol_DNSSD = 1,

    /** SSDP. */
    kHAPPlatformWiFiRouterDynamicPortAdvertisementProtocol_SSDP
} HAP_ENUM_END(uint8_t, HAPPlatformWiFiRouterDynamicPortAdvertisementProtocol);

/**
 * Dynamic port service type.
 */
typedef struct {
    /** The protocol advertising the port to listen to. */
    HAPPlatformWiFiRouterDynamicPortAdvertisementProtocol advertisementProtocol;

    /** Protocol specific parameters. */
    union {
        /** DNS-SD. */
        struct {
            /**
             * DNS-SD service type.
             *
             * - If set, only ports associated with advertisements that match the specified service type
             *   will be advertised and optionally opened.
             *
             * - Note this is only the logical service name (e.g., "hap", "airplay", "raop");
             *   the full DNS-SD service label (e.g., "_hap._tcp" is derived from this value and the transport protocol.
             */
            const char* _Nullable serviceType;
        } dns_sd;

        /** SSDP. */
        struct {
            /**
             * SSDP service type URI.
             *
             * - If set, only ports associated with advertisements that match the specified service type
             *   will be advertised and optionally opened.
             */
            const char* _Nullable serviceTypeURI;
        } ssdp;
    } _;
} HAPPlatformWiFiRouterDynamicPortServiceType;

/**
 * Dynamic port LAN firewall rule.
 *
 * A LAN firewall rule that allows advertisement or discovery of a particular type of service, and optionally allows
 * TCP or unicast UDP traffic to or from the advertised service.
 */
typedef struct {
    /**
     * Type. Must be kHAPPlatformWiFiRouterLANFirewallRuleType_DynamicPort.
     */
    HAPPlatformWiFiRouterLANFirewallRuleType type;

    /**
     * Direction.
     *
     * - Outbound: The network client is allowed to listen for advertisements and discover services as specified
     *   by serviceType. Unless advertisementOnly is set, the network client is also allowed to initiate conversations
     *   with the discovered services.
     * - Inbound: The network client is allowed to advertise services as specified by serviceType.
     *   Unless advertisementOnly is set, the network client is also allowed to accept conversations on
     *   any advertised service ports.
     *
     * - All advertisement traffic required by the service discovery protocol is implicitly allowed by this rule.
     */
    HAPPlatformWiFiRouterFirewallRuleDirection direction;

    /** Transport protocol (excluding traffic related to the advertisement). */
    HAPPlatformWiFiRouterTransportProtocol transportProtocol;

    /** Peer network client group identifiers. */
    HAPPlatformWiFiRouterGroupIdentifier peerGroupIdentifiers[kHAPPlatformWiFiRouterLANFirewallRule_MaxPeerGroups];

    /** Number of peer network client group identifiers. At least 1 is always defined. */
    size_t numPeerGroupIdentifiers;

    /** Destination service type. */
    HAPPlatformWiFiRouterDynamicPortServiceType serviceType;

    /** If set to true, only bridge the advertisements, do not open any pinholes. */
    bool advertisementOnly : 1;
} HAPPlatformWiFiRouterDynamicPortLANFirewallRule;
HAP_STATIC_ASSERT(
        HAP_OFFSETOF(HAPPlatformWiFiRouterDynamicPortLANFirewallRule, type) == 0,
        HAPPlatformWiFiRouterDynamicPortLANFirewallRule_type);

/**
 * Static ICMP LAN firewall rule.
 *
 * A LAN firewall rule that allows ICMP traffic to static IP addresses and/or ports.
 */
typedef struct {
    /**
     * Type. Must be kHAPPlatformWiFiRouterLANFirewallRuleType_StaticICMP.
     */
    HAPPlatformWiFiRouterLANFirewallRuleType type;

    /** Direction. */
    HAPPlatformWiFiRouterFirewallRuleDirection direction;

    /** Peer network client group identifiers. */
    HAPPlatformWiFiRouterGroupIdentifier peerGroupIdentifiers[kHAPPlatformWiFiRouterLANFirewallRule_MaxPeerGroups];

    /** Number of peer network client group identifiers. At least 1 is always defined. */
    size_t numPeerGroupIdentifiers;

    /** ICMP types. */
    HAPPlatformWiFiRouterICMPType icmpTypes[kHAPPlatformWiFiRouterFirewallRule_MaxICMPTypes];

    /** Number of ICMP types. At least 1 is always defined. */
    size_t numICMPTypes;
} HAPPlatformWiFiRouterStaticICMPLANFirewallRule;
HAP_STATIC_ASSERT(
        HAP_OFFSETOF(HAPPlatformWiFiRouterStaticICMPLANFirewallRule, type) == 0,
        HAPPlatformWiFiRouterStaticICMPLANFirewallRule_type);

/**@}*/

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Delegate that is used to monitor the Wi-Fi router state.
 */
typedef struct {
    /**
     * Client context pointer. Will be passed to callbacks.
     */
    void* _Nullable context;

    /**
     * Invoked when the Wi-Fi router's ready state has changed.
     *
     * - Updated state may be retrieved through the HAPPlatformWiFiRouterIsReady method.
     *
     * @param      wiFiRouter           Wi-Fi router.
     * @param      context              The context pointer of the Wi-Fi router delegate structure.
     */
    void (*_Nullable handleReadyStateChanged)(HAPPlatformWiFiRouterRef wiFiRouter, void* _Nullable context);

    /**
     * Invoked when the Wi-Fi router's managed network state has changed.
     *
     * - Updated state may be retrieved through the HAPPlatformWiFiRouterIsManagedNetworkEnabled method.
     *
     * @param      wiFiRouter           Wi-Fi router.
     * @param      context              The context pointer of the Wi-Fi router delegate structure.
     */
    void (*_Nullable handleManagedNetworkStateChanged)(HAPPlatformWiFiRouterRef wiFiRouter, void* _Nullable context);

    /**
     * Invoked when the Wi-Fi router's WAN configuration has changed.
     *
     * - This callback shall only be invoked for out-of-band changes to the network configuration.
     *
     * - Updated state may be retrieved through the HAPPlatformWiFiRouterEnumerateWANs method.
     *
     * @param      wiFiRouter           Wi-Fi router.
     * @param      context              The context pointer of the Wi-Fi router delegate structure.
     */
    void (*_Nullable handleWANConfigurationChanged)(HAPPlatformWiFiRouterRef wiFiRouter, void* _Nullable context);

    /**
     * Invoked when the Wi-Fi router's WAN status has changed.
     *
     * - Updated state may be retrieved through the HAPPlatformWiFiRouterWANGetStatus method.
     *
     * @param      wiFiRouter           Wi-Fi router.
     * @param      context              The context pointer of the Wi-Fi router delegate structure.
     */
    void (*_Nullable handleWANStatusChanged)(HAPPlatformWiFiRouterRef wiFiRouter, void* _Nullable context);

    /**
     * Invoked when a network access violation has been committed by a managed network client
     * for the first time after its matching network access profile has been created, or
     * for the first time after its network access violations have been reset.
     *
     * - Updated state may be retrieved through the HAPPlatformWiFiRouterClientGetAccessViolationMetadata method.
     *
     * @param      wiFiRouter           Wi-Fi router.
     * @param      clientIdentifier     Network client profile identifier.
     * @param      context              The context pointer of the Wi-Fi router delegate structure.
     */
    void (*_Nullable handleAccessViolationMetadataChanged)(
            HAPPlatformWiFiRouterRef wiFiRouter,
            HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
            void* _Nullable context);

    /**
     * Invoked when a Wi-Fi router's satellite accessory status has changed.
     *
     * - Updated status may be retrieved through the HAPPlatformWiFiRouterGetSatelliteStatus method.
     *
     * @param      wiFiRouter           Wi-Fi router.
     * @param      satelliteIndex       The index of the satellite accessory whose status changed.
     * @param      context              The context pointer of the Wi-Fi router delegate structure.
     */
    void (*_Nullable handleSatelliteStatusChanged)(
            HAPPlatformWiFiRouterRef wiFiRouter,
            size_t satelliteIndex,
            void* _Nullable context);
} HAPPlatformWiFiRouterDelegate;

/**
 * Specifies or clears the delegate for receiving Wi-Fi router events.
 *
 * - The delegate structure is copied and does not need to remain valid.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      delegate             The delegate to receive the Wi-Fi router events. NULL to clear.
 */
void HAPPlatformWiFiRouterSetDelegate(
        HAPPlatformWiFiRouterRef wiFiRouter,
        const HAPPlatformWiFiRouterDelegate* _Nullable delegate);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @defgroup ConfigurationAccess Network configuration access.
 *
 * Access to the network configuration is synchronized across different execution contexts using a set of functions.
 *
 * While holding shared access to the network configuration, the network configuration may be read.
 * No other client may modify it until all clients released their access.
 *
 * While holding exclusive access to the network configuration, modifications to the network configuration
 * are also permitted. No other client may acquire access to the network configuration while holding exclusive access.
 *
 * **Example**

   @code{.c}

   HAPPlatformWiFiRouterRef wiFiRouter;

   HAPError err;

   // Acquire shared (read-only) access to the network configuration.
   err = HAPPlatformWiFiRouterAcquireSharedConfigurationAccess(wiFiRouter);
   if (err) {
       // Handle error.
   }
   {
       // Shared (read-only) access to the network configuration has been acquired.
       bool isEnabled;
       err = HAPPlatformWiFiRouterIsManagedNetworkEnabled(wiFiRouter, &isEnabled);
       if (err) {
           // Handle error, ensuring that shared configuration access is released.
       }
       if (!isEnabled) {
           // Acquire exclusive (read-write) access to the network configuration.
           err = HAPPlatformWiFiRouterAcquireExclusiveConfigurationAccess(wiFiRouter);
           if (err) {
               // Handle error, ensuring that exclusive and shared configuration access is released.
           }
           {
               // Exclusive (read-write) access to the network configuration has been acquired.
               isEnabled = true;
               err = HAPPlatformWiFiRouterSetManagedNetworkEnabled(wiFiRouter, isEnabled);
               if (err) {
                   // Handle error, ensuring that exclusive and shared configuration access is released.
               }
           }
           // Release exclusive (read-write) access to the network configuration.
           HAPPlatformWiFiRouterReleaseExclusiveConfigurationAccess(wiFiRouter);
           // Shared (read-only access to the network configuration is still available.
       }

       // No modification to the network configuration by other clients is possible
       // until all clients release access to the network configuration.
   }
   HAPPlatformWiFiRouterReleaseSharedConfigurationAccess(wiFiRouter);

   @endcode
 */
/**@{*/

/**
 * Acquires shared (read-only) access to the network configuration.
 * Successful calls to this function must be balanced with HAPPlatformWiFiRouterReleaseSharedConfigurationAccess.
 *
 * - Shared access allows multiple clients to read the network configuration but does not allow modifications.
 *   While shared access is acquired the network configuration must not change.
 *
 * - Access may only be provided for a limited time. Other functions may fail with kHAPError_Unknown
 *   when access has been released due to a timeout.
 *
 * @param      wiFiRouter           Wi-Fi router.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Busy           If the network configuration is currently unavailable for shared access.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterAcquireSharedConfigurationAccess(HAPPlatformWiFiRouterRef wiFiRouter);

/**
 * Releases shared access to the network configuration.
 *
 * @param      wiFiRouter           Wi-Fi router.
 */
void HAPPlatformWiFiRouterReleaseSharedConfigurationAccess(HAPPlatformWiFiRouterRef wiFiRouter);

/**
 * Acquires exclusive (read-write) access to the network configuration.
 * Successful calls to this function must be balanced with HAPPlatformWiFiRouterReleaseExclusiveConfigurationAccess.
 *
 * - Exclusive access allows a single client to modify the network configuration.
 *   No shared access may be acquired by other clients while exclusive access is granted.
 *
 * - It is possible to acquire exclusive access while holding shared access to the network configuration.
 *
 * - Access may only be provided for a limited time. Other functions may fail with kHAPError_Unknown
 *   when access has been released due to a timeout.
 *
 * @param      wiFiRouter           Wi-Fi router.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Busy           If the network configuration is currently unavailable for exclusive access.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterAcquireExclusiveConfigurationAccess(HAPPlatformWiFiRouterRef wiFiRouter);

/**
 * Releases exclusive access to the network configuration.
 *
 * @param      wiFiRouter           Wi-Fi router.
 */
void HAPPlatformWiFiRouterReleaseExclusiveConfigurationAccess(HAPPlatformWiFiRouterRef wiFiRouter);

/**@}*/

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @defgroup ConfigurationChange Network configuration change.
 *
 * Network configuration changes are grouped in transactions that are committed atomically.
 * If any requested operation fails the whole transaction may be rolled back without altering the network configuration.
 *
 * **Example**

   @code{.c}

   HAPPlatformWiFiRouterRef wiFiRouter;

   HAPError err;

   // In this example an existing network client profile with a specific identifier is updated.
   HAPPlatformWiFiRouterClientIdentifier clientIdentifier = ...;
   HAPPlatformWiFiRouterGroupIdentifier groupIdentifier = ...;
   HAPPlatformWiFiRouterClientCredential credential = ...;

   // Acquire shared (read-only) access to the network configuration.
   err = HAPPlatformWiFiRouterAcquireSharedConfigurationAccess(wiFiRouter);
   if (err) {
       // Handle error.
   }
   {
       // Check whether the managed network is enabled.
       bool isManagedNetworkEnabled;
       err = HAPPlatformWiFiRouterIsManagedNetworkEnabled(wiFiRouter, &isManagedNetworkEnabled);
       if (err || !isManagedNetworkEnabled) {
           // Handle error, ensuring that shared configuration access is released.
       }

       // Acquire exclusive (read-write) access to the network configuration.
       err = HAPPlatformWiFiRouterAcquireExclusiveConfigurationAccess(wiFiRouter);
       if (err) {
           // Handle error, ensuring that shared configuration access is released.
       }
       {
           // Check if the network client profile has been registered.
           bool exists;
           err = HAPPlatformWiFiRouterClientExists(wiFiRouter, clientIdentifier, &exists);
           if (err) {
               // Handle error, ensuring that exclusive and shared configuration access is released.
           }
           if (!exists) {
               // Handle error, ensuring that exclusive and shared configuration access is released.
           }

           // Begin network configuration change.
           err = HAPPlatformWiFiRouterBeginConfigurationChange(wiFiRouter);
           if (err) {
               // Handle error, ensuring that exclusive and shared configuration access is released.
           }
           {
               err = HAPPlatformWiFiRouterClientSetGroupIdentifier(wiFiRouter, clientIdentifier,
                   groupIdentifier);
               if (err) {
                   // Handle error, ensuring that the configuration change is rolled back,
                   // and that exclusive and shared configuration access is released.
               }

               err = HAPPlatformWiFiRouterClientSetCredential(wiFiRouter, clientIdentifier,
                   &credential);
               if (err) {
                   // Handle error, ensuring that the configuration change is rolled back,
                   // and that exclusive and shared configuration access is released.
               }
           }
           err = HAPPlatformWiFiRouterCommitConfigurationChange(wiFiRouter);
           if (err) {
               // Handle error, ensuring that the configuration change is rolled back,
               // and that exclusive and shared configuration access is released.
           }
       }
       HAPPlatformWiFiRouterReleaseExclusiveConfigurationAccess(wiFiRouter);
   }
   HAPPlatformWiFiRouterReleaseSharedConfigurationAccess(wiFiRouter);

   @endcode
 */

/**
 * Begins a network configuration change.
 *
 * - Changes to the network configuration do not take effect until the HAPPlatformWiFiRouterCommitConfigurationChange
 *   function is called, but HAPPlatformWiFiRouter functions must behave as if not-yet committed changes were already
 *   applied. For example, when a network client profile is added but the network configuration changes have not yet
 *   been committed, HAPPlatformWiFiRouterClientExists shall already indicate that the network client profile exists.
 *   When the HAPPlatformWiFiRouterRollbackConfigurationChange function is called, not-yet committed changes
 *   must be discarded.
 *
 * - This function may only be called while holding exclusive access to the network configuration.
 *
 * - Exclusive access to the network configuration must be held until the
 *   pending network configuration changes are either committed or rolled back.
 *
 * - The following functions may only be called after HAPPlatformWiFiRouterBeginConfigurationChange has been called.
 *   Effects of those functions are only applied once HAPPlatformWiFiRouterCommitConfigurationChange is called
 *   and must be rolled back when HAPPlatformWiFiRouterRollbackConfigurationChange is called.
 *   - HAPPlatformWiFiRouterAddClient
 *   - HAPPlatformWiFiRouterRemoveClient
 *   - HAPPlatformWiFiRouterClientSetGroupIdentifier
 *   - HAPPlatformWiFiRouterClientSetCredential
 *   - HAPPlatformWiFiRouterClientResetWANFirewall
 *   - HAPPlatformWiFiRouterClientAddWANFirewallRule
 *   - HAPPlatformWiFiRouterClientResetLANFirewall
 *   - HAPPlatformWiFiRouterClientAddLANFirewallRule
 *   Note: The corresponding Enumerate and Get functions already have to return pending but not yet committed data
 *   before HAPPlatformWiFiRouterCommitConfigurationChange is called.
 *
 * - The following functions may not be called during a network configuration change.
 *   The existing network configuration changes must be committed or rolled back before calling these functions.
 *   - HAPPlatformWiFiRouterSetManagedNetworkEnabled
 *   - HAPPlatformWiFiRouterGetClientStatusForIdentifiers
 *   - HAPPlatformWiFiRouterClientGetAccessViolationMetadata
 *   - HAPPlatformWiFiRouterClientResetAccessViolations
 *
 * - Effects of functions that do not modify the network configuration such as HAPPlatformWiFiRouterSetDelegate
 *   do not need to be rolled back when HAPPlatformWiFiRouterRollbackConfigurationChange is called.
 *
 * @param      wiFiRouter           Wi-Fi router.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If the network configuration is currently unavailable for changing.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterBeginConfigurationChange(HAPPlatformWiFiRouterRef wiFiRouter);

/**
 * Commits all pending network configuration changes.
 *
 * - The updated configuration must be fully committed to persistent storage before returning from this function.
 *
 * - If the pending network configuration changes cannot be applied
 *   all pending network configuration changes are rolled back and the previous network configuration remains valid.
 *
 * - This function may only be called while holding exclusive access to the network configuration.
 *
 * @param      wiFiRouter           Wi-Fi router.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If the pending network configuration changes could not be applied.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterCommitConfigurationChange(HAPPlatformWiFiRouterRef wiFiRouter);

/**
 * Rolls back all pending network configuration changes.
 *
 * - This function may only be called while holding exclusive access to the network configuration.
 *
 * @param      wiFiRouter           Wi-Fi router.
 */
void HAPPlatformWiFiRouterRollbackConfigurationChange(HAPPlatformWiFiRouterRef wiFiRouter);

/**
 * Returns whether the current network configurations (WAN, LAN, network client profile configurations)
 * have been deployed and are fully operational. A non-ready status indicates that the router is in the process
 * of setting up the network.
 *
 * - Note, the router status does not represent the state of the WAN link (online, offline);
 *   see HAPPlatformWiFiRouterWANGetStatus for WAN link status.
 *
 * - When the status changes the delegate's handleReadyStateChanged method shall be called.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param[out] isReady              Whether the network configurations have been deployed and are fully operational.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If network configuration state is unknown.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterIsReady(HAPPlatformWiFiRouterRef wiFiRouter, bool* isReady);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Indicates whether the managed network is currently enabled.
 *
 * - This function may only be called while holding shared or exclusive access to the network configuration.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param[out] isEnabled            Whether the managed network is enabled or disabled.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If network configuration access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterIsManagedNetworkEnabled(HAPPlatformWiFiRouterRef wiFiRouter, bool* isEnabled);

/**
 * Enables or disables the managed network.
 *
 * The network is fully operational after HAPPlatformWiFiRouterIsReady indicates "ready".
 * When enabled, network client profile configurations can be created and managed and network policies are enforced.
 * When disabled, all managed network client profile configurations are deleted and any corresponding network clients
 * are disassociated from the network.
 *
 * - When factory reset occurs, when the last HomeKit pairing is removed, or when any of the WAN links are configured
 *   to bridge mode, the managed network must be disabled and all network client profile configurations deleted and
 *   all network clients disassociated as specified above.
 *   The delegate's handleManagedNetworkStateChanged method shall be called.
 *
 * - When the managed network state is modified out-of-band
 *   the delegate's handleManagedNetworkStateChanged method shall be called.
 *
 * - This function may only be called while holding exclusive access to the network configuration.
 *
 * - This function may not be called during a network configuration change.
 *   The existing network configuration changes must be committed or rolled back before calling this function.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      isEnabled            Whether the managed network should be enabled or disabled.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If network configuration access failed.
 * @return kHAPError_InvalidState   If managed network cannot be enabled because a WAN is configured for bridge mode.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterSetManagedNetworkEnabled(HAPPlatformWiFiRouterRef wiFiRouter, bool isEnabled);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @defgroup WAN WAN configuration.
 *
 * The WAN links are configured out-of-band.
 *
 *
 * The WAN identifiers may be queried using:
 * - HAPPlatformWiFiRouterEnumerateWANs
 * - HAPPlatformWiFiRouterWANExists
 *
 * Given an identifier, the corresponding WAN configuration may be queried using:
 * - HAPPlatformWiFiRouterWANGetType
 *
 * The WAN configuration is read-only.
 * When the WAN configuration is modified out-of-band
 * the delegate's handleWANConfigurationChanged method shall be called.
 *
 *
 * The current WAN statuses are provided by HAPPlatformWiFiRouterWANGetStatus.
 * When the WAN status of any WAN changes the delegate's handleWANStatusChanged method shall be called.
 */
/**@{*/

/**
 * Callback that should be invoked for each configured WAN.
 *
 * @param      context              Context.
 * @param      wiFiRouter           Wi-Fi router.
 * @param      wanIdentifier        WAN identifier.
 * @param[in,out] shouldContinue    True if enumeration shall continue, False otherwise. Is set to true on input.
 */
typedef void (*HAPPlatformWiFiRouterEnumerateWANsCallback)(
        void* _Nullable context,
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterWANIdentifier wanIdentifier,
        bool* shouldContinue);

/**
 * Enumerates the configured WANs.
 * Function returns after all items were reported.
 *
 * - The network configuration may not be mutated during enumeration.
 *
 * - This function may only be called while holding shared or exclusive access to the network configuration.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      callback             Function to call on each configured WAN.
 * @param      context              Context that is passed to the callback.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If network configuration access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterEnumerateWANs(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterEnumerateWANsCallback callback,
        void* _Nullable context);

/**
 * Determines whether a WAN with a given identifier exists.
 *
 * - This function may only be called while holding shared or exclusive access to the network configuration.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      wanIdentifier        WAN identifier.
 * @param[out] exists               True if a configuration exists for the given identifier, false otherwise.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If network configuration access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterWANExists(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterWANIdentifier wanIdentifier,
        bool* exists);

//----------------------------------------------------------------------------------------------------------------------

/**
 * WAN type.
 */
HAP_ENUM_BEGIN(uint8_t, HAPPlatformWiFiRouterWANType) {
    /** Unconfigured. */
    kHAPPlatformWiFiRouterWANType_Unconfigured,

    /** Other. */
    kHAPPlatformWiFiRouterWANType_Other,

    /** DHCP. */
    kHAPPlatformWiFiRouterWANType_DHCP,

    /** Bridge mode. */
    kHAPPlatformWiFiRouterWANType_BridgeMode,
} HAP_ENUM_END(uint8_t, HAPPlatformWiFiRouterWANType);

/**
 * Fetches the WAN type of a WAN.
 *
 * - When any WAN is configured for bridge mode, the managed network must be disabled and
 *   the delegate's handleManagedNetworkStateChanged method shall be called.
 *
 * - This function may only be called while holding shared or exclusive access to the network configuration.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      wanIdentifier        WAN identifier.
 * @param[out] wanType              WAN type.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If network configuration access failed.
 * @return kHAPError_InvalidState   If no WAN with the given identifier exists.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterWANGetType(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterWANIdentifier wanIdentifier,
        HAPPlatformWiFiRouterWANType* wanType);

//----------------------------------------------------------------------------------------------------------------------

/**
 * WAN status. No bits set indicates the WAN link is online.
 */
HAP_OPTIONS_BEGIN(uint16_t, HAPPlatformWiFiRouterWANStatus) {
    /** Unknown. */
    kHAPPlatformWiFiRouterWANStatus_Unknown = 1U << 0U,

    /** No cable connected. */
    kHAPPlatformWiFiRouterWANStatus_NoCableConnected = 1U << 1U,

    /** No IP address. */
    kHAPPlatformWiFiRouterWANStatus_NoIPAddress = 1U << 2U,

    /** No gateway specified. */
    kHAPPlatformWiFiRouterWANStatus_NoGatewaySpecified = 1U << 3U,

    /** Gateway unreachable. */
    kHAPPlatformWiFiRouterWANStatus_GatewayUnreachable = 1U << 4U,

    /** No DNS server(s) specified. */
    kHAPPlatformWiFiRouterWANStatus_NoDNSServerSpecified = 1U << 5U,

    /** DNS server(s) unreachable. */
    kHAPPlatformWiFiRouterWANStatus_DNSServerUnreachable = 1U << 6U,

    /** Authentication failed. */
    kHAPPlatformWiFiRouterWANStatus_AuthenticationFailed = 1U << 7U,

    /** Walled (WAN link is available, but crippled). */
    kHAPPlatformWiFiRouterWANStatus_Walled = 1U << 8U
} HAP_OPTIONS_END(uint16_t, HAPPlatformWiFiRouterWANStatus);

/**
 * Fetches the current WAN status of a WAN.
 *
 * - When the WAN status of any WAN changes the delegate's handleWANStatusChanged method shall be called.
 *
 * - This function may only be called while holding shared or exclusive access to the network configuration.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      wanIdentifier        WAN identifier.
 * @param[out] wanStatus            Current WAN status.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If network configuration access failed.
 * @return kHAPError_InvalidState   If no WAN with the given identifier exists.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterWANGetStatus(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterWANIdentifier wanIdentifier,
        HAPPlatformWiFiRouterWANStatus* wanStatus);

/**@}*/

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @defgroup Client Network client profile configuration.
 *
 * Every Wi-Fi or Ethernet network client connected to the local network managed by the router is associated with a
 * single network client profile. Network client profiles are managed via this set of functions, and are associated with
 * a network client via a client-specific credential (preferred), or via its MAC address (for backwards compatibility)
 * as follows:
 *
 * - A Wi-Fi network client that associates with the router using a client-specific credential is assigned
 *   the network client profile associated with that credential.
 *
 * - A Wi-Fi network client using the shared PSK (i.e., the Wi-Fi password manually configured by the owner) is assigned
 *   a network client profile matching it's MAC address, if any.
 *
 * - All other network clients are assigned the default network client profile.
 *
 *
 * A network client profile consists of:
 *
 * - The set of firewall rules that apply to network clients associated with the network client profile.
 *
 * - The network client group the network client belongs to.
 *
 * Network client groups simply provide a way of referring to a set of network clients as allowed endpoints
 * in a LAN firewall rule.
 *
 *
 * Default network client profile:
 *
 * - Firewall rules:
 *   - Full WAN Access (Allow outbound WAN traffic to any endpoint).
 *   - Full LAN Access (Allow outbound and inbound LAN traffic (IP or otherwise) to and from any local network client.
 *
 * - Network client groups:
 *   - Main (1).
 *
 * This default network client profile provides unmanaged network clients with essentially the same network access
 * they would have in the absence of a managed network.
 *
 *
 * The Wi-Fi router must complete all operations related to network client profile configurations, credentials,
 * and firewall modifications. Configuration changes must persist within 1 second of receiving the operation request.
 *
 *
 * The router must ensure that PSK credentials managed via this characteristic do not conflict with other PSKs
 * managed by the router, including the default PSK.
 *
 * When a network client profile is updated or removed, any established conversations that are no longer permitted
 * by the updated firewall rules must be closed. Established conversations that would still be allowed under the
 * updated rules should not be affected. If a router places clients in different subnets and a network client profile
 * change results in the need to move a network client to a different subnet must (1) immediately attempt to force
 * the network client to obtain a new IP address, and (2) not forward any packets to/from the network client
 * using the old IP address.
 *
 * Modifications to network client profile configurations managed by this platform module must only be done
 * through these functions. Additional network client configurations managed by the router (if any) must be kept
 * separate from the profiles configured through these functions.
 *
 * When factory reset occurs or when the last HomeKit pairing is removed, all data written through these functions
 * and associated data (e.g., installed firewall rules, installed PSKs) must be deleted. Further, any state related to
 * generating the next network client profile identifier, if any, must be reset.
 *
 *
 * The network client profile identifiers may be queried using:
 * - HAPPlatformWiFiRouterEnumerateClients
 * - HAPPlatformWiFiRouterClientExists
 *
 * A network client profile may be added or removed using:
 * - HAPPlatformWiFiRouterAddClient
 * - HAPPlatformWiFiRouterRemoveClient
 *
 * Given an identifier, the corresponding network client profile configuration may be queried using:
 * - HAPPlatformWiFiRouterClientGetGroupIdentifier
 * - HAPPlatformWiFiRouterClientGetCredentialType
 * - HAPPlatformWiFiRouterClientGetMACAddressCredential
 *
 * Network client profile configurations may be updated using:
 * - HAPPlatformWiFiRouterClientSetGroupIdentifier
 * - HAPPlatformWiFiRouterClientSetCredential (for security reasons network client credentials cannot be fetched)
 *
 * Firewall rules may be queried using:
 * - HAPPlatformWiFiRouterClientGetWANFirewallType
 * - HAPPlatformWiFiRouterClientEnumerateWANFirewallRules (if WAN firewall type is Allowlist)
 * - HAPPlatformWiFiRouterClientGetLANFirewallType
 * - HAPPlatformWiFiRouterClientEnumerateLANFirewallRules (if LAN firewall type is Allowlist)
 *
 * Firewall rules may be configured using:
 * - HAPPlatformWiFiRouterClientResetWANFirewall
 * - HAPPlatformWiFiRouterClientAddWANFirewallRule
 * - HAPPlatformWiFiRouterClientResetLANFirewall
 * - HAPPlatformWiFiRouterClientAddLANFirewallRule
 *
 * **Example**

   @code{.c}

   HAPPlatformWiFiRouterRef wiFiRouter;

   HAPError err;

   // In this example the firewall configuration of a network client profile is updated.
   HAPPlatformWiFiRouterClientIdentifier clientIdentifier = ...;

   // Acquire exclusive (read-write) access to the network configuration.
   err = HAPPlatformWiFiRouterAcquireExclusiveConfigurationAccess(wiFiRouter);
   if (err) {
       // Handle error.
   }
   {
       bool exists;
       err = HAPPlatformWiFiRouterClientExists(wiFiRouter, clientIdentifier, &exists);
       if (err) {
           // Handle error, ensuring that exclusive configuration access is released.
       }
       if (exists) {
           // Begin network configuration change.
           err = HAPPlatformWiFiRouterBeginConfigurationChange(wiFiRouter);
           if (err) {
               // Handle error, ensuring that exclusive configuration access is released.
           }
           {
               // Reset WAN firewall rules.
               err = HAPPlatformWiFiRouterClientResetWANFirewall(wiFiRouter, clientIdentifier,
                   kHAPPlatformWiFiRouterFirewallType_Allowlist);
               if (err) {
                   // Handle error, ensuring that the configuration change is rolled back,
                   // and that exclusive configuration access is released.
               }

               err = HAPPlatformWiFiRouterClientAddWANFirewallRule(wiFiRouter, clientIdentifier,
                   &(const HAPPlatformWiFiRouterPortWANFirewallRule) {
                       .type = kHAPPlatformWiFiRouterWANFirewallRuleType_Port,
                       .transportProtocol = kHAPPlatformWiFiRouterTransportProtocol_TCP,
                       .host = {
                           .type = kHAPPlatformWiFiRouterWANHostURIType_DNSNamePattern,
                           ._.dnsNamePattern = "*.apple.com"
                       }
                   });
               if (err) {
                   // Handle error, ensuring that the configuration change is rolled back,
                   // and that exclusive configuration access is released.
               }

               err = HAPPlatformWiFiRouterClientAddWANFirewallRule(wiFiRouter, clientIdentifier,
                   &(const HAPPlatformWiFiRouterPortWANFirewallRule) {
                       .type = kHAPPlatformWiFiRouterWANFirewallRuleType_Port,
                       .transportProtocol = kHAPPlatformWiFiRouterTransportProtocol_TCP,
                       .host = {
                           .type = kHAPPlatformWiFiRouterWANHostURIType_IPAddressRange,
                           ._.ipAddressRange = &(const HAPIPAddressRange) {
                               .version = kHAPIPAddressVersion_IPv4,
                               ._.ipv4 = {
                                   .startAddress.bytes = { 17, 0, 0, 0 },
                                   .endAddress.bytes = { 17, 255, 255, 255 }
                               }
                           }
                       }
                   });
               if (err) {
                   // Handle error, ensuring that the configuration change is rolled back,
                   // and that exclusive configuration access is released.
               }

               // Finalize adding WAN firewall rules.
               err = HAPPlatformWiFiRouterClientFinalizeWANFirewallRules(wiFiRouter, clientIdentifier);
               if (err) {
                   // Handle error, ensuring that the configuration change is rolled back,
                   // and that exclusive configuration access is released.
               }

               // Reset LAN firewall rules.
               err = HAPPlatformWiFiRouterClientResetLANFirewall(wiFiRouter, clientIdentifier,
                   kHAPPlatformWiFiRouterFirewallType_FullAccess);
               if (err) {
                   // Handle error, ensuring that the configuration change is rolled back,
                   // and that exclusive configuration access is released.
               }
           }
           err = HAPPlatformWiFiRouterCommitConfigurationChange(wiFiRouter);
           if (err) {
               // Handle error, ensuring that the configuration change is rolled back,
               // and that exclusive configuration access is released.
           }
       }
   }
   HAPPlatformWiFiRouterReleaseExclusiveConfigurationAccess(wiFiRouter);

   @endcode

 */
/**@{*/

/**
 * Callback that should be invoked for each configured network client profile.
 *
 * @param      context              Context.
 * @param      wiFiRouter           Wi-Fi router.
 * @param      clientIdentifier     Network client profile identifier.
 * @param[in,out] shouldContinue    True if enumeration shall continue, False otherwise. Is set to true on input.
 */
typedef void (*HAPPlatformWiFiRouterEnumerateClientsCallback)(
        void* _Nullable context,
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        bool* shouldContinue);

/**
 * Enumerates the configured network client profiles.
 * Function returns after all items were reported.
 *
 * - The network configuration may not be mutated during enumeration.
 *
 * - This function may only be called while holding shared or exclusive access to the network configuration.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      callback             Function to call on each configured network client profile.
 * @param      context              Context that is passed to the callback.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If network configuration access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterEnumerateClients(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterEnumerateClientsCallback callback,
        void* _Nullable context);

/**
 * Determines whether a network client profile with a given identifier exists.
 *
 * - This function may only be called while holding shared or exclusive access to the network configuration.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      clientIdentifier     Network client profile identifier.
 * @param[out] exists               True if a configuration exists for the given identifier, false otherwise.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If network configuration access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientExists(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        bool* exists);

/**
 * Credential type.
 */
HAP_ENUM_BEGIN(uint8_t, HAPPlatformWiFiRouterCredentialType) {
    /**
     * MAC address based credential.
     *
     * - This is used for Ethernet clients and for Wi-Fi clients where the default PSK is used.
     */
    kHAPPlatformWiFiRouterCredentialType_MACAddress = 1,

    /**
     * PSK based credential.
     *
     * - This is used for Wi-Fi clients where a unique PSK is used.
     */
    kHAPPlatformWiFiRouterCredentialType_PSK
} HAP_ENUM_END(uint8_t, HAPPlatformWiFiRouterCredentialType);

/**
 * Network client profile credential.
 */
typedef struct {
    /** Credential type. */
    HAPPlatformWiFiRouterCredentialType type;

    /** Type specific parameters. */
    union {
        /** MAC address based credential. */
        HAPMACAddress macAddress;

        /** PSK based credential. */
        HAPWiFiWPAPersonalCredential psk;
    } _;
} HAPPlatformWiFiRouterClientCredential;

/**
 * Registers a network client profile.
 *
 * - WAN and LAN firewalls are initialized as empty allowlists. To configure firewall rules,
 *   HAPPlatformWiFiRouterClientResetWANFirewall and HAPPlatformWiFiRouterClientResetLANFirewall are used.
 *
 * - For security reasons network client profile credentials cannot be fetched, i.e., may only be updated.
 *
 * - This function may only be called after HAPPlatformWiFiRouterBeginConfigurationChange has been called.
 *   Effects of this function are only applied once HAPPlatformWiFiRouterCommitConfigurationChange is called.
 *
 * - This function may only be called while holding exclusive access to the network configuration.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      groupIdentifier      Identifier of the network client group associated with this network client profile.
 * @param      credential           The credential used to authenticate the network client profile.
 * @param[out] clientIdentifier     Network client profile identifier. Must monotonically increase.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If network configuration access failed, or managed network is not enabled.
 * @return kHAPError_InvalidData    If the given parameters are not supported.
 * @return kHAPError_OutOfResources If out of resources, or if all network client profile identifiers are exhausted.
 * @return kHAPError_NotAuthorized  If the given network client profile credential is already in use.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterAddClient(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterGroupIdentifier groupIdentifier,
        const HAPPlatformWiFiRouterClientCredential* credential,
        HAPPlatformWiFiRouterClientIdentifier* clientIdentifier);

/**
 * Removes a network client profile.
 *
 * - Recorded network access violations by this network client profile are only deleted once
 *   HAPPlatformWiFiRouterCommitConfigurationChange is called.
 *
 * - This function may only be called after HAPPlatformWiFiRouterBeginConfigurationChange has been called.
 *   Effects of this function are only applied once HAPPlatformWiFiRouterCommitConfigurationChange is called.
 *
 * - This function may only be called while holding exclusive access to the network configuration.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      clientIdentifier     Network client profile identifier.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If network configuration access failed.
 * @return kHAPError_OutOfResources If out of resources.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterRemoveClient(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier);

//----------------------------------------------------------------------------------------------------------------------

/**
 * Fetches the network client group identifier that a network client profile will be joined into.
 *
 * - This function may only be called while holding shared or exclusive access to the network configuration.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      clientIdentifier     Network client profile identifier.
 * @param[out] groupIdentifier      Identifier of the network client group associated with this network client profile.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If network configuration access failed.
 * @return kHAPError_InvalidState   If no network client profile with the given identifier exists.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientGetGroupIdentifier(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterGroupIdentifier* groupIdentifier);

/**
 * Updates the network client group identifier that a network client profile will be joined into.
 *
 * - This function may only be called after HAPPlatformWiFiRouterBeginConfigurationChange has been called.
 *   Effects of this function are only applied once HAPPlatformWiFiRouterCommitConfigurationChange is called.
 *
 * - This function may only be called while holding exclusive access to the network configuration.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      clientIdentifier     Network client profile identifier.
 * @param      groupIdentifier      Identifier of the network client group associated with this network client profile.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If network configuration access failed.
 * @return kHAPError_InvalidState   If no network client profile with the given identifier exists.
 * @return kHAPError_InvalidData    If the given parameters are not supported.
 * @return kHAPError_OutOfResources If out of resources.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientSetGroupIdentifier(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterGroupIdentifier groupIdentifier);

/**
 * Fetches the credential type used to authenticate a network client profile.
 *
 * - This function may only be called while holding shared or exclusive access to the network configuration.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      clientIdentifier     Network client profile identifier.
 * @param[out] credentialType       The credential type used to authenticate the network client profile.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If network configuration access failed.
 * @return kHAPError_InvalidState   If no network client profile with the given identifier exists.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientGetCredentialType(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterCredentialType* credentialType);

/**
 * Fetches the credential of a MAC address based network client profile.
 *
 * - This function may only be called while holding shared or exclusive access to the network configuration.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      clientIdentifier     Network client profile identifier.
 * @param[out] credential           The credential used to authenticate the network client profile.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If network configuration access failed.
 * @return kHAPError_InvalidState   If no MAC address based network client profile with the given identifier exists.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientGetMACAddressCredential(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPMACAddress* credential);

/**
 * Updates the credential used to authenticate a network client profile.
 *
 * - For security reasons network client profile credentials cannot be fetched, i.e., may only be updated.
 *
 * - This function may only be called after HAPPlatformWiFiRouterBeginConfigurationChange has been called.
 *   Effects of this function are only applied once HAPPlatformWiFiRouterCommitConfigurationChange is called.
 *
 * - This function may only be called while holding exclusive access to the network configuration.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      clientIdentifier     Network client profile identifier.
 * @param      credential           The credential used to authenticate the network client profile.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If network configuration access failed.
 * @return kHAPError_InvalidState   If no network client profile with the given identifier exists.
 * @return kHAPError_InvalidData    If the given parameters are not supported.
 * @return kHAPError_OutOfResources If out of resources.
 * @return kHAPError_NotAuthorized  If the given network client profile credential is already in use.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientSetCredential(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        const HAPPlatformWiFiRouterClientCredential* credential);

//----------------------------------------------------------------------------------------------------------------------

/**
 * Fetches the WAN firewall type of a network client profile.
 *
 * - This function may only be called while holding shared or exclusive access to the network configuration.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      clientIdentifier     Network client profile identifier.
 * @param[out] firewallType         Firewall type.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If network configuration access failed.
 * @return kHAPError_InvalidState   If no network client profile with the given identifier exists.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientGetWANFirewallType(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterFirewallType* firewallType);

/**
 * Callback that should be invoked for each WAN firewall rule of a network client profile.
 *
 * @param      context              Context.
 * @param      wiFiRouter           Wi-Fi router.
 * @param      clientIdentifier     Network client profile identifier.
 * @param      firewallRule         Firewall rule.
 * @param[in,out] shouldContinue    True if enumeration shall continue, False otherwise. Is set to true on input.
 */
typedef void (*HAPPlatformWiFiRouterClientEnumerateWANFirewallRulesCallback)(
        void* _Nullable context,
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        const HAPPlatformWiFiRouterWANFirewallRule* firewallRule,
        bool* shouldContinue);

/**
 * Enumerates the WAN firewall rules of a network client profile.
 * Function returns after all items were reported.
 *
 * - Firewall rules must be reported in the same order as they were added.
 *
 * - The network configuration may not be mutated during enumeration.
 *
 * - This function may only be called while holding shared or exclusive access to the network configuration.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      clientIdentifier     Network client profile identifier.
 * @param      callback             Function to call on each firewall rule.
 * @param      context              Context that is passed to the callback.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If network configuration access failed.
 * @return kHAPError_InvalidState   If no network client profile with the identifier exists.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientEnumerateWANFirewallRules(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterClientEnumerateWANFirewallRulesCallback callback,
        void* _Nullable context);

/**
 * Resets the WAN firewall of a network client profile. All existing WAN firewall rules are removed.
 *
 * - For allowlist, firewall rules are set up using a series of HAPPlatformWiFiRouterClientAddWANFirewallRule calls
 *   followed by a HAPPlatformWiFiRouterClientFinalizeWANFirewallRules call (also called for empty allowlist).
 *   A new firewall may only be configured after HAPPlatformWiFiRouterClientFinalizeWANFirewallRules.
 *
 * - This function may only be called after HAPPlatformWiFiRouterBeginConfigurationChange has been called.
 *   Effects of this function are only applied once HAPPlatformWiFiRouterCommitConfigurationChange is called.
 *
 * - This function may only be called while holding exclusive access to the network configuration.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      clientIdentifier     Network client profile identifier.
 * @param      firewallType         Firewall type.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If network configuration access failed.
 * @return kHAPError_InvalidState   If no network client profile with the given identifier exists.
 * @return kHAPError_InvalidData    If the given parameters are not supported.
 * @return kHAPError_OutOfResources If out of resources.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientResetWANFirewall(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterFirewallType firewallType);

/**
 * Adds a WAN firewall rule to the most recently reset WAN firewall of a network client profile.
 *
 * - This function may only be called after HAPPlatformWiFiRouterClientResetWANFirewall with an allowlist firewall type.
 *   HAPPlatformWiFiRouterClientFinalizeWANFirewallRules is called once all WAN firewall rules have been added.
 *   The same network client profile identifier is passed to all functions.
 *   Only one firewall may be reconfigured at a time, and no other network client operations are allowed until complete.
 *
 * - This function may only be called after HAPPlatformWiFiRouterBeginConfigurationChange has been called.
 *   Effects of this function are only applied once HAPPlatformWiFiRouterCommitConfigurationChange is called.
 *
 * - This function may only be called while holding exclusive access to the network configuration.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      clientIdentifier     Network client profile identifier.
 * @param      firewallRule         Firewall rule.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If network configuration access failed.
 * @return kHAPError_InvalidData    If the given parameters are not supported.
 * @return kHAPError_OutOfResources If out of resources.
 *
 * **Example**

   @code{.c}

   HAP_RESULT_USE_CHECK
   HAPError HAPPlatformWiFiRouterClientAddWANFirewallRule(
       HAPPlatformWiFiRouterRef wiFiRouter,
       HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
       const HAPPlatformWiFiRouterWANFirewallRule *firewallRule_)
   {
       switch (*(const HAPPlatformWiFiRouterWANFirewallRuleType *) firewallRule_) {
           case kHAPPlatformWiFiRouterWANFirewallRuleType_Port: {
               const HAPPlatformWiFiRouterPortWANFirewallRule *firewallRule = firewallRule_;
               // Process port WAN firewall rule.
           } return kHAPError_None;
           case kHAPPlatformWiFiRouterWANFirewallRuleType_ICMP: {
               const HAPPlatformWiFiRouterICMPWANFirewallRule *firewallRule = firewallRule_;
               // Process ICMP WAN firewall rule.
           } return kHAPError_None;
           default: HAPFatalError();
       }
   }

   @endcode
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientAddWANFirewallRule(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        const HAPPlatformWiFiRouterWANFirewallRule* firewallRule);

/**
 * Finalizes the WAN firewall rule list once all WAN firewall rules are added to a network client profile.
 *
 * - This function may only be called after HAPPlatformWiFiRouterBeginConfigurationChange has been called.
 *   Effects of this function are only applied once HAPPlatformWiFiRouterCommitConfigurationChange is called.
 *
 * - This function may only be called while holding exclusive access to the network configuration.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      clientIdentifier     Network client profile identifier.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If network configuration access failed.
 * @return kHAPError_OutOfResources If out of resources.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientFinalizeWANFirewallRules(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier);

//----------------------------------------------------------------------------------------------------------------------

/**
 * Fetches the LAN firewall type of a network client profile.
 *
 * - This function may only be called while holding shared or exclusive access to the network configuration.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      clientIdentifier     Network client profile identifier.
 * @param[out] firewallType         Firewall type.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If network configuration access failed.
 * @return kHAPError_InvalidState   If no network client profile with the given identifier exists.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientGetLANFirewallType(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterFirewallType* firewallType);

/**
 * Callback that should be invoked for each LAN firewall rule of a network client profile.
 *
 * @param      context              Context.
 * @param      wiFiRouter           Wi-Fi router.
 * @param      clientIdentifier     Network client profile identifier.
 * @param      firewallRule         Firewall rule.
 * @param[in,out] shouldContinue    True if enumeration shall continue, False otherwise. Is set to true on input.
 */
typedef void (*HAPPlatformWiFiRouterClientEnumerateLANFirewallRulesCallback)(
        void* _Nullable context,
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        const HAPPlatformWiFiRouterLANFirewallRule* firewallRule,
        bool* shouldContinue);

/**
 * Enumerates the LAN firewall rules of a network client profile.
 * Function returns after all items were reported.
 *
 * - Firewall rules must be reported in the same order as they were added.
 *
 * - The network configuration may not be mutated during enumeration.
 *
 * - This function may only be called while holding shared or exclusive access to the network configuration.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      clientIdentifier     Network client profile identifier.
 * @param      callback             Function to call on each firewall rule.
 * @param      context              Context that is passed to the callback.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If network configuration access failed.
 * @return kHAPError_InvalidState   If no network client profile with the identifier exists.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientEnumerateLANFirewallRules(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterClientEnumerateLANFirewallRulesCallback callback,
        void* _Nullable context);

/**
 * Resets the LAN firewall of a network client profile. All existing LAN firewall rules are removed.
 *
 * - For allowlist, firewall rules are set up using a series of HAPPlatformWiFiRouterClientAddLANFirewallRule calls
 *   followed by a HAPPlatformWiFiRouterClientFinalizeLANFirewallRules call (also called for empty allowlist).
 *   A new firewall may only be configured after HAPPlatformWiFiRouterClientFinalizeLANFirewallRules.
 *
 * - This function may only be called after HAPPlatformWiFiRouterBeginConfigurationChange has been called.
 *   Effects of this function are only applied once HAPPlatformWiFiRouterCommitConfigurationChange is called.
 *
 * - This function may only be called while holding exclusive access to the network configuration.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      clientIdentifier     Network client profile identifier.
 * @param      firewallType         Firewall type.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If network configuration access failed.
 * @return kHAPError_InvalidState   If no network client profile with the given identifier exists.
 * @return kHAPError_InvalidData    If the given parameters are not supported.
 * @return kHAPError_OutOfResources If out of resources.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientResetLANFirewall(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterFirewallType firewallType);

/**
 * Adds a LAN firewall rule.
 *
 * - This function may only be called after HAPPlatformWiFiRouterClientResetLANFirewall with an allowlist firewall type.
 *   HAPPlatformWiFiRouterClientFinalizeLANFirewallRules is called once all LAN firewall rules have been added.
 *   The same network client profile identifier is passed to all functions.
 *   Only one firewall may be reconfigured at a time, and no other network client operations are allowed until complete.
 *
 * - This function may only be called after HAPPlatformWiFiRouterBeginConfigurationChange has been called.
 *   Effects of this function are only applied once HAPPlatformWiFiRouterCommitConfigurationChange is called.
 *
 * - This function may only be called while holding exclusive access to the network configuration.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      clientIdentifier     Network client profile identifier.
 * @param      firewallRule         Firewall rule.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If network configuration access failed.
 * @return kHAPError_InvalidData    If the given parameters are not supported.
 * @return kHAPError_OutOfResources If out of resources.
 *
 * **Example**

   @code{.c}

   HAP_RESULT_USE_CHECK
   HAPError HAPPlatformWiFiRouterClientAddLANFirewallRule(
       HAPPlatformWiFiRouterRef wiFiRouter,
       HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
       const HAPPlatformWiFiRouterLANFirewallRule *firewallRule_)
   {
       switch (*(const HAPPlatformWiFiRouterLANFirewallRuleType *) firewallRule_) {
           case kHAPPlatformWiFiRouterLANFirewallRuleType_MulticastBridging: {
               const HAPPlatformWiFiRouterMulticastBridgingLANFirewallRule *firewallRule = firewallRule_;
               // Process multicast bridging LAN firewall rule.
           } return kHAPError_None;
           case kHAPPlatformWiFiRouterLANFirewallRuleType_StaticPort: {
               const HAPPlatformWiFiRouterStaticPortLANFirewallRule *firewallRule = firewallRule_;
               // Process static port LAN firewall rule.
           } return kHAPError_None;
           case kHAPPlatformWiFiRouterLANFirewallRuleType_DynamicPort: {
               const HAPPlatformWiFiRouterDynamicPortLANFirewallRule *firewallRule = firewallRule_;
               // Process dynamic port LAN firewall rule.
           } return kHAPError_None;
           case kHAPPlatformWiFiRouterLANFirewallRuleType_StaticICMP: {
               const HAPPlatformWiFiRouterStaticICMPLANFirewallRule *firewallRule = firewallRule_;
               // Process static ICMP LAN firewall rule.
           } return kHAPError_None;
           default: HAPFatalError();
       }
   }

   @endcode
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientAddLANFirewallRule(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        const HAPPlatformWiFiRouterLANFirewallRule* firewallRule);

/**
 * Finalizes the LAN firewall rule list once all LAN firewall rules are added to a network client profile.
 *
 * - This function may only be called after HAPPlatformWiFiRouterBeginConfigurationChange has been called.
 *   Effects of this function are only applied once HAPPlatformWiFiRouterCommitConfigurationChange is called.
 *
 * - This function may only be called while holding exclusive access to the network configuration.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      clientIdentifier     Network client profile identifier.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If network configuration access failed.
 * @return kHAPError_OutOfResources If out of resources.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientFinalizeLANFirewallRules(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier);

/**@}*/

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Network Client Status Identifier format.
 */
HAP_ENUM_BEGIN(uint8_t, HAPPlatformWiFiRouterClientStatusIdentifierFormat) {
    /** Network client identifier. */
    kHAPPlatformWiFiRouterClientStatusIdentifierFormat_Client = 1,

    /** MAC address. */
    kHAPPlatformWiFiRouterClientStatusIdentifierFormat_MACAddress,

    /** IP address. */
    kHAPPlatformWiFiRouterClientStatusIdentifierFormat_IPAddress,
} HAP_ENUM_END(uint8_t, HAPPlatformWiFiRouterClientStatusIdentifierFormat);

/**
 * Network Client Status Identifier.
 *
 * This union has been optimized for memory to avoid unnecessary padding between members.
 * To determine which of the format variants is valid, access the `format` member first.
 *
 * **Example**

   @code{.c}

   const HAPPlatformWiFiRouterClientStatusIdentifier *statusIdentifier = ...;
   switch (statusIdentifier->format) {
       case kHAPPlatformWiFiRouterClientStatusIdentifierFormat_Client: {
           HAPPlatformWiFiRouterClientIdentifier clientIdentifier = statusIdentifier->clientIdentifier._;
       break;
}
       case kHAPPlatformWiFiRouterClientStatusIdentifierFormat_MACAddress: {
           const HACAddress *macAddress = &statusIdentifier->macAddress._;
       break;
}
       case kHAPPlatformWiFiRouterClientStatusIdentifierFormat_IPAddress: {
           const HAPIPAddress *ipAddress = &statusIdentifier->ipAddress._;
       break;
}
       default: HAPFatalError();
   }

   @endcode
 */
typedef union {
    /** Format. */
    HAPPlatformWiFiRouterClientStatusIdentifierFormat format;

    /** Network client identifier format specific parameters. */
    struct {
        /** Format. Must be kHAPPlatformWiFiRouterClientStatusIdentifierFormat_Client. */
        HAPPlatformWiFiRouterClientStatusIdentifierFormat format;

        /** Network client identifier. */
        HAPPlatformWiFiRouterClientIdentifier _;
    } clientIdentifier;

    /** MAC address format specific parameters. */
    struct {
        /** Format. Must be kHAPPlatformWiFiRouterClientStatusIdentifierFormat_MACAddress. */
        HAPPlatformWiFiRouterClientStatusIdentifierFormat format;

        /** MAC address. */
        HAPMACAddress _;
    } macAddress;

    /** IP address format specific parameters. */
    struct {
        /** Format. Must be kHAPPlatformWiFiRouterClientStatusIdentifierFormat_IPAddress. */
        HAPPlatformWiFiRouterClientStatusIdentifierFormat format;

        /** IP address. */
        HAPIPAddress _;
    } ipAddress;
} HAPPlatformWiFiRouterClientStatusIdentifier;

/**
 * Network client status.
 */
typedef struct {
    /** The identifier of the network client profile, if the client is managed. 0 otherwise. */
    HAPPlatformWiFiRouterClientIdentifier clientIdentifier;

    /** MAC address of the network client. */
    const HAPMACAddress* macAddress;

    /** Array of IP addresses of the network client, if any. */
    const HAPIPAddress* _Nullable ipAddresses;

    /** Number of IP addresses. */
    size_t numIPAddresses;

    /** The network client's advertised name, if any. */
    const char* _Nullable name;

    /** RSSI for the network client, if connected via Wi-Fi and the information is known. */
    struct {
        HAPRSSI value;      /**< RSSI measurement in dBm. */
        bool isDefined : 1; /**< Whether or not the RSSI measurement is defined. */
    } rssi;
} HAPPlatformWiFiRouterClientStatus;

/**
 * Callback that should be invoked for each connected network client.
 *
 * @param      context              Context.
 * @param      wiFiRouter           Wi-Fi router.
 * @param      clientStatus         Network client status.
 * @param[in,out] shouldContinue    True if enumeration shall continue, False otherwise. Is set to true on input.
 */
typedef void (*HAPPlatformWiFiRouterGetClientStatusForIdentifiersCallback)(
        void* _Nullable context,
        HAPPlatformWiFiRouterRef wiFiRouter,
        const HAPPlatformWiFiRouterClientStatus* clientStatus,
        bool* shouldContinue);

/**
 * Enumerates all connected network clients that match the given Network Client Status Identifiers.
 * Function returns after all items were reported.
 *
 * - The network configuration may not be mutated during enumeration.
 *
 * - This function may only be called while holding shared or exclusive access to the network configuration.
 *
 * - This function may not be called during a network configuration change.
 *   The existing network configuration changes must be committed or rolled back before calling this function.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      statusIdentifiers    Network Client Status Identifiers.
 * @param      numStatusIdentifiers Number of Network Client Status Identifiers.
 * @param      callback             Function to call on each connected network client.
 * @param      context              Context that is passed to the callback.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If network configuration access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterGetClientStatusForIdentifiers(
        HAPPlatformWiFiRouterRef wiFiRouter,
        const HAPPlatformWiFiRouterClientStatusIdentifier* statusIdentifiers,
        size_t numStatusIdentifiers,
        HAPPlatformWiFiRouterGetClientStatusForIdentifiersCallback callback,
        void* _Nullable context);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @defgroup AccessViolation Network access violations.
 *
 * When a network client attempts to access a host that it is not allowed to according to its firewall rules,
 * a network access violation is logged. The most recent network access violations as well as aggregated statistics
 * about the network access violations may be queried for each network client profile.
 */
/**@{*/

/**
 * UTC time in seconds since UNIX Epoch (00:00:00 Thursday, 1 January 1970).
 */
typedef uint64_t HAPPlatformWiFiRouterTimestamp;

/**
 * Network access violation metadata.
 */
typedef struct {
    /** Last time the Reset operation was performed for this network client profile. */
    HAPPlatformWiFiRouterTimestamp lastResetTimestamp;

    /** Last time a network access violation attempt was registered. Only valid if hasViolations is set. */
    HAPPlatformWiFiRouterTimestamp lastViolationTimestamp;

    /** Whether or not a Reset operation has been performed for this network client profile. */
    bool wasReset : 1;

    /** Whether or not a network access violation has occurred since it was last reset. */
    bool hasViolations : 1;
} HAPPlatformWiFiRouterAccessViolationMetadata;

/**
 * Fetches network access violation metadata for a network client profile.
 *
 * - This function may only be called while holding shared or exclusive access to the network configuration.
 *
 * - This function may not be called during a network configuration change.
 *   The existing network configuration changes must be committed or rolled back before calling this function.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      clientIdentifier     Network client profile identifier.
 * @param[out] violationMetadata    Network access violation metadata.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If network configuration access failed.
 * @return kHAPError_InvalidState   If no network client profile with the given identifier exists.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientGetAccessViolationMetadata(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterAccessViolationMetadata* violationMetadata);

/**
 * Resets all network access violation records for a network client profile.
 *
 * - This function may only be called while holding exclusive access to the network configuration.
 *
 * - This function may not be called during a network configuration change.
 *   The existing network configuration changes must be committed or rolled back before calling this function.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      clientIdentifier     Network client profile identifier.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If network configuration access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientResetAccessViolations(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier);

/**@}*/

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @defgroup SatelliteAccessory Satellite accessories management.
 *
 * Wi-Fi satellite accessories extend the Wi-Fi network coverage provided by a router.
 */
/**@{*/

/**
 * Satellite accessory status.
 */
HAP_ENUM_BEGIN(uint8_t, HAPPlatformWiFiRouterSatelliteStatus) { /** Wi-Fi Satellite status is unknown. */
                                                                kHAPPlatformWiFiRouterSatelliteStatus_Unknown,

                                                                /** Wi-Fi Satellite is connected. */
                                                                kHAPPlatformWiFiRouterSatelliteStatus_Connected,

                                                                /** Wi-Fi Satellite is not connected. */
                                                                kHAPPlatformWiFiRouterSatelliteStatus_NotConnected
} HAP_ENUM_END(uint8_t, HAPPlatformWiFiRouterSatelliteStatus);

/**
 * Returns a Wi-Fi router's satellite accessory status.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      satelliteIndex       Index of the satellite accessory.
 *
 * @return the Wi-Fi router's satellite accessory status.
 */
HAP_RESULT_USE_CHECK
HAPPlatformWiFiRouterSatelliteStatus
        HAPPlatformWiFiRouterGetSatelliteStatus(HAPPlatformWiFiRouterRef wiFiRouter, size_t satelliteIndex);

/**@}*/

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
