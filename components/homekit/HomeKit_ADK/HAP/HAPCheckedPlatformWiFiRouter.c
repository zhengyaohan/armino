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

#undef HAP_DISALLOW_USE_IGNORED
#define HAP_DISALLOW_USE_IGNORED 1

#include "HAPLogSubsystem.h"
#include "HAPWACEngine.h"
#include "HAPWiFiRouter.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_ROUTER)

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "WiFiRouter" };

HAP_RESULT_USE_CHECK
static bool HAPWiFiWPAPassphraseIsValid(const HAPWiFiWPAPassphrase* value) {
    HAPPrecondition(value);
    const char* name = "WPA/WPA2 Personal passphrase";

    if (value->stringValue[sizeof value->stringValue - 1]) {
        HAPLogError(&logObject, "%s is invalid: Not NULL-terminated.", name);
        return false;
    }
    size_t numBytes = HAPStringGetNumBytes(value->stringValue);
    if (numBytes < kHAPWiFiWPAPassphrase_MinBytes || numBytes > kHAPWiFiWPAPassphrase_MaxBytes) {
        HAPLogError(&logObject, "%s has invalid length: %zu.", name, numBytes);
        return false;
    }
    if (!HAPWACEngineIsValidWPAPassphrase(value->stringValue)) {
        HAPLogError(&logObject, "%s is invalid.", name);
        return false;
    }
    return true;
}

HAP_RESULT_USE_CHECK
static bool HAPWiFiWPAPersonalCredentialTypeIsValid(HAPWiFiWPAPersonalCredentialType value) {
    switch (value) {
        case kHAPWiFiWPAPersonalCredentialType_Passphrase:
        case kHAPWiFiWPAPersonalCredentialType_PSK: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
static bool HAPWiFiWPAPersonalCredentialIsValid(const HAPWiFiWPAPersonalCredential* value_) {
    HAPPrecondition(value_);

    if (!HAPWiFiWPAPersonalCredentialTypeIsValid(value_->type)) {
        HAPLogError(&logObject, "WPA/WPA2 Personal credential has invalid type: %u.", value_->type);
        return false;
    }
    switch (value_->type) {
        case kHAPWiFiWPAPersonalCredentialType_Passphrase: {
            const HAPWiFiWPAPassphrase* value = &value_->_.passphrase;
            const char* name = "WPA/WPA2 Personal passphrase";

            if (!HAPWiFiWPAPassphraseIsValid(value)) {
                HAPLogError(&logObject, "%s is invalid.", name);
                return false;
            }
        }
            return true;
        case kHAPWiFiWPAPersonalCredentialType_PSK: {
        }
            return true;
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
static bool HAPPlatformWiFiRouterWANIdentifierIsValid(HAPPlatformWiFiRouterWANIdentifier value) {
    return value != 0;
}

HAP_RESULT_USE_CHECK
static bool HAPPlatformWiFiRouterGroupIdentifierIsValid(HAPPlatformWiFiRouterGroupIdentifier value) {
    return value != 0;
}

HAP_RESULT_USE_CHECK
static bool HAPPlatformWiFiRouterClientIdentifierIsValid(HAPPlatformWiFiRouterClientIdentifier value) {
    return value != 0;
}

HAP_RESULT_USE_CHECK
static bool HAPPlatformWiFiRouterFirewallTypeIsValid(HAPPlatformWiFiRouterFirewallType value) {
    switch (value) {
        case kHAPPlatformWiFiRouterFirewallType_FullAccess:
        case kHAPPlatformWiFiRouterFirewallType_Allowlist: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
static bool HAPPlatformWiFiRouterFirewallRuleDirectionIsValid(HAPPlatformWiFiRouterFirewallRuleDirection value) {
    switch (value) {
        case kHAPPlatformWiFiRouterFirewallRuleDirection_Outbound:
        case kHAPPlatformWiFiRouterFirewallRuleDirection_Inbound: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
static bool HAPPlatformWiFiRouterTransportProtocolIsValid(HAPPlatformWiFiRouterTransportProtocol value) {
    switch (value) {
        case kHAPPlatformWiFiRouterTransportProtocol_TCP:
        case kHAPPlatformWiFiRouterTransportProtocol_UDP: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
static bool HAPPlatformWiFiRouterICMPProtocolIsValid(HAPPlatformWiFiRouterICMPProtocol value) {
    switch (value) {
        case kHAPPlatformWiFiRouterICMPProtocol_ICMPv4:
        case kHAPPlatformWiFiRouterICMPProtocol_ICMPv6: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
static bool HAPPlatformWiFiRouterICMPTypeIsValid(const HAPPlatformWiFiRouterICMPType* value) {
    HAPPrecondition(value);

    if (!HAPPlatformWiFiRouterICMPProtocolIsValid(value->icmpProtocol)) {
        HAPLogError(&logObject, "ICMP type has invalid icmpProtocol: %u.", value->icmpProtocol);
        return false;
    }
    return true;
}

HAP_RESULT_USE_CHECK
static bool HAPPlatformWiFiRouterWANFirewallRuleTypeIsValid(HAPPlatformWiFiRouterWANFirewallRuleType value) {
    switch (value) {
        case kHAPPlatformWiFiRouterWANFirewallRuleType_Port:
        case kHAPPlatformWiFiRouterWANFirewallRuleType_ICMP: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
static bool HAPPlatformWiFiRouterWANHostURITypeIsValid(HAPPlatformWiFiRouterWANHostURIType value) {
    switch (value) {
        case kHAPPlatformWiFiRouterWANHostURIType_Any:
        case kHAPPlatformWiFiRouterWANHostURIType_DNSNamePattern:
        case kHAPPlatformWiFiRouterWANHostURIType_IPAddress:
        case kHAPPlatformWiFiRouterWANHostURIType_IPAddressRange: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
static bool HAPPlatformWiFiRouterWANHostURIIsValid(const HAPPlatformWiFiRouterWANHostURI* value_) {
    HAPPrecondition(value_);

    if (!HAPPlatformWiFiRouterWANHostURITypeIsValid(value_->type)) {
        HAPLogError(&logObject, "WAN host URI has invalid type: %u.", value_->type);
        return false;
    }
    switch (value_->type) {
        case kHAPPlatformWiFiRouterWANHostURIType_Any: {
            break;
        }
        case kHAPPlatformWiFiRouterWANHostURIType_DNSNamePattern: {
            const char* value = value_->_.dnsNamePattern;
            const char* name = "DNS name WAN host URI";

            if (!HAPWiFiRouterHostDNSNamePatternIsValid(value)) {
                HAPLogError(&logObject, "%s is invalid.", name);
                return false;
            }
            break;
        }
        case kHAPPlatformWiFiRouterWANHostURIType_IPAddress: {
            const HAPIPAddress* value = &value_->_.ipAddress;
            const char* name = "IP address WAN host URI";

            if (!HAPIPAddressIsValid(value)) {
                HAPLogError(&logObject, "%s is invalid.", name);
                return false;
            }
            break;
        }
        case kHAPPlatformWiFiRouterWANHostURIType_IPAddressRange: {
            const HAPIPAddressRange* value = &value_->_.ipAddressRange;
            const char* name = "IP address range WAN host URI";

            if (!HAPIPAddressRangeIsValid(value)) {
                HAPLogError(&logObject, "%s is invalid.", name);
                return false;
            }
            break;
        }
    }
    return true;
}

HAP_RESULT_USE_CHECK
static bool HAPPlatformWiFiRouterWANFirewallRuleIsValid(const HAPPlatformWiFiRouterWANFirewallRule* value_) {
    HAPPrecondition(value_);
    const HAPPlatformWiFiRouterWANFirewallRuleType* type = value_;

    if (!HAPPlatformWiFiRouterWANFirewallRuleTypeIsValid(*type)) {
        HAPLogError(&logObject, "WAN firewall rule has invalid type: %u.", *type);
        return false;
    }
    switch (*type) {
        case kHAPPlatformWiFiRouterWANFirewallRuleType_Port: {
            const HAPPlatformWiFiRouterPortWANFirewallRule* value = value_;
            const char* name = "Port WAN firewall rule";

            if (!HAPPlatformWiFiRouterTransportProtocolIsValid(value->transportProtocol)) {
                HAPLogError(&logObject, "%s has invalid transportProtocol: %u.", name, value->transportProtocol);
                return false;
            }
            if (!HAPPlatformWiFiRouterWANHostURIIsValid(&value->host)) {
                HAPLogError(&logObject, "%s has invalid host.", name);
                return false;
            }
            if (!HAPNetworkPortRangeIsValid(&value->hostPortRange)) {
                HAPLogError(&logObject, "%s has invalid hostPortRange.", name);
                return false;
            }
        }
            return true;
        case kHAPPlatformWiFiRouterWANFirewallRuleType_ICMP: {
            const HAPPlatformWiFiRouterICMPWANFirewallRule* value = value_;
            const char* name = "ICMP WAN firewall rule";

            if (!HAPPlatformWiFiRouterWANHostURIIsValid(&value->host)) {
                HAPLogError(&logObject, "%s has invalid host.", name);
                return false;
            }
            if (!value->numICMPTypes || value->numICMPTypes > HAPArrayCount(value->icmpTypes)) {
                HAPLogError(&logObject, "%s has invalid numICMPTypes: %zu.", name, value->numICMPTypes);
                return false;
            }
            for (size_t i = 0; i < value->numICMPTypes; i++) {
                if (!HAPPlatformWiFiRouterICMPTypeIsValid(&value->icmpTypes[i])) {
                    HAPLogError(&logObject, "%s has invalid icmpTypes[%zu].", name, i);
                    return false;
                }
            }
        }
            return true;
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
static bool HAPPlatformWiFiRouterLANFirewallRuleTypeIsValid(HAPPlatformWiFiRouterLANFirewallRuleType value) {
    switch (value) {
        case kHAPPlatformWiFiRouterLANFirewallRuleType_MulticastBridging:
        case kHAPPlatformWiFiRouterLANFirewallRuleType_StaticPort:
        case kHAPPlatformWiFiRouterLANFirewallRuleType_DynamicPort:
        case kHAPPlatformWiFiRouterLANFirewallRuleType_StaticICMP: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
static bool HAPPlatformWiFiRouterDynamicPortAdvertisementProtocolIsValid(
        HAPPlatformWiFiRouterDynamicPortAdvertisementProtocol value) {
    switch (value) {
        case kHAPPlatformWiFiRouterDynamicPortAdvertisementProtocol_DNSSD:
        case kHAPPlatformWiFiRouterDynamicPortAdvertisementProtocol_SSDP: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
static bool
        HAPPlatformWiFiRouterDynamicPortServiceTypeIsValid(const HAPPlatformWiFiRouterDynamicPortServiceType* value) {
    HAPPrecondition(value);
    const char* name = "Dynamic port service type";

    if (!HAPPlatformWiFiRouterDynamicPortAdvertisementProtocolIsValid(value->advertisementProtocol)) {
        HAPLogError(&logObject, "%s has invalid advertisementProtocol: %u.", name, value->advertisementProtocol);
        return false;
    }
    switch (value->advertisementProtocol) {
        case kHAPPlatformWiFiRouterDynamicPortAdvertisementProtocol_DNSSD: {
            if (value->_.dns_sd.serviceType) {
                const char* serviceType = HAPNonnull(value->_.dns_sd.serviceType);
                size_t numServiceTypeBytes = HAPStringGetNumBytes(serviceType);
                if (!HAPUTF8IsValidData(HAPNonnull(serviceType), numServiceTypeBytes)) {
                    HAPLogError(&logObject, "%s has invalid DNS-SD serviceType: Not valid UTF-8.", name);
                    return false;
                }
            }
            break;
        }
        case kHAPPlatformWiFiRouterDynamicPortAdvertisementProtocol_SSDP: {
            if (value->_.ssdp.serviceTypeURI) {
                const char* serviceTypeURI = HAPNonnull(value->_.ssdp.serviceTypeURI);
                size_t numServiceTypeURIBytes = HAPStringGetNumBytes(serviceTypeURI);
                if (!HAPUTF8IsValidData(HAPNonnull(serviceTypeURI), numServiceTypeURIBytes)) {
                    HAPLogError(&logObject, "%s has invalid SSDP serviceTypeURI: Not valid UTF-8.", name);
                    return false;
                }
            }
            break;
        }
    }
    return true;
}

HAP_RESULT_USE_CHECK
static bool HAPPlatformWiFiRouterLANFirewallRuleIsValid(const HAPPlatformWiFiRouterLANFirewallRule* value_) {
    HAPPrecondition(value_);
    const HAPPlatformWiFiRouterLANFirewallRuleType* type = value_;

    if (!HAPPlatformWiFiRouterLANFirewallRuleTypeIsValid(*type)) {
        HAPLogError(&logObject, "LAN firewall rule has invalid type: %u.", *type);
        return false;
    }
    switch (*type) {
        case kHAPPlatformWiFiRouterLANFirewallRuleType_MulticastBridging: {
            const HAPPlatformWiFiRouterMulticastBridgingLANFirewallRule* value = value_;
            const char* name = "Multicast bridging LAN firewall rule";

            if (!HAPPlatformWiFiRouterFirewallRuleDirectionIsValid(value->direction)) {
                HAPLogError(&logObject, "%s has invalid direction: %u.", name, value->direction);
                return false;
            }
            if (!value->numPeerGroupIdentifiers ||
                value->numPeerGroupIdentifiers > HAPArrayCount(value->peerGroupIdentifiers)) {
                HAPLogError(
                        &logObject,
                        "%s has invalid numPeerGroupIdentifiers: %zu.",
                        name,
                        value->numPeerGroupIdentifiers);
                return false;
            }
            for (size_t i = 0; i < value->numPeerGroupIdentifiers; i++) {
                if (!HAPPlatformWiFiRouterGroupIdentifierIsValid(value->peerGroupIdentifiers[i])) {
                    HAPLogError(
                            &logObject,
                            "%s has invalid peerGroupIdentifiers[%zu]: %lu.",
                            name,
                            i,
                            (unsigned long) value->peerGroupIdentifiers[i]);
                    return false;
                }
            }
            if (!HAPIPAddressIsValid(&value->destinationIP)) {
                HAPLogError(&logObject, "%s has invalid destinationIP.", name);
                return false;
            }
        }
            return true;
        case kHAPPlatformWiFiRouterLANFirewallRuleType_StaticPort: {
            const HAPPlatformWiFiRouterStaticPortLANFirewallRule* value = value_;
            const char* name = "Static port LAN firewall rule";

            if (!HAPPlatformWiFiRouterFirewallRuleDirectionIsValid(value->direction)) {
                HAPLogError(&logObject, "%s has invalid direction: %u.", name, value->direction);
                return false;
            }
            if (!HAPPlatformWiFiRouterTransportProtocolIsValid(value->transportProtocol)) {
                HAPLogError(&logObject, "%s has invalid transportProtocol: %u.", name, value->transportProtocol);
                return false;
            }
            if (!value->numPeerGroupIdentifiers ||
                value->numPeerGroupIdentifiers > HAPArrayCount(value->peerGroupIdentifiers)) {
                HAPLogError(
                        &logObject,
                        "%s has invalid numPeerGroupIdentifiers: %zu.",
                        name,
                        value->numPeerGroupIdentifiers);
                return false;
            }
            for (size_t i = 0; i < value->numPeerGroupIdentifiers; i++) {
                if (!HAPPlatformWiFiRouterGroupIdentifierIsValid(value->peerGroupIdentifiers[i])) {
                    HAPLogError(
                            &logObject,
                            "%s has invalid peerGroupIdentifiers[%zu]: %lu.",
                            name,
                            i,
                            (unsigned long) value->peerGroupIdentifiers[i]);
                    return false;
                }
            }
            if (!HAPNetworkPortRangeIsValid(&value->destinationPortRange)) {
                HAPLogError(&logObject, "%s has invalid destinationPortRange.", name);
                return false;
            }
        }
            return true;
        case kHAPPlatformWiFiRouterLANFirewallRuleType_DynamicPort: {
            const HAPPlatformWiFiRouterDynamicPortLANFirewallRule* value = value_;
            const char* name = "Dynamic port LAN firewall rule";

            if (!HAPPlatformWiFiRouterFirewallRuleDirectionIsValid(value->direction)) {
                HAPLogError(&logObject, "%s has invalid direction: %u.", name, value->direction);
                return false;
            }
            if (!HAPPlatformWiFiRouterTransportProtocolIsValid(value->transportProtocol)) {
                HAPLogError(&logObject, "%s has invalid transportProtocol: %u.", name, value->transportProtocol);
                return false;
            }
            if (!value->numPeerGroupIdentifiers ||
                value->numPeerGroupIdentifiers > HAPArrayCount(value->peerGroupIdentifiers)) {
                HAPLogError(
                        &logObject,
                        "%s has invalid numPeerGroupIdentifiers: %zu.",
                        name,
                        value->numPeerGroupIdentifiers);
                return false;
            }
            for (size_t i = 0; i < value->numPeerGroupIdentifiers; i++) {
                if (!HAPPlatformWiFiRouterGroupIdentifierIsValid(value->peerGroupIdentifiers[i])) {
                    HAPLogError(
                            &logObject,
                            "%s has invalid peerGroupIdentifiers[%zu]: %lu.",
                            name,
                            i,
                            (unsigned long) value->peerGroupIdentifiers[i]);
                    return false;
                }
            }
            if (!HAPPlatformWiFiRouterDynamicPortServiceTypeIsValid(&value->serviceType)) {
                HAPLogError(&logObject, "%s has invalid serviceType.", name);
                return false;
            }
        }
            return true;
        case kHAPPlatformWiFiRouterLANFirewallRuleType_StaticICMP: {
            const HAPPlatformWiFiRouterStaticICMPLANFirewallRule* value = value_;
            const char* name = "Static ICMP LAN firewall rule";

            if (!HAPPlatformWiFiRouterFirewallRuleDirectionIsValid(value->direction)) {
                HAPLogError(&logObject, "%s has invalid direction: %u.", name, value->direction);
                return false;
            }
            if (!value->numPeerGroupIdentifiers ||
                value->numPeerGroupIdentifiers > HAPArrayCount(value->peerGroupIdentifiers)) {
                HAPLogError(
                        &logObject,
                        "%s has invalid numPeerGroupIdentifiers: %zu.",
                        name,
                        value->numPeerGroupIdentifiers);
                return false;
            }
            for (size_t i = 0; i < value->numPeerGroupIdentifiers; i++) {
                if (!HAPPlatformWiFiRouterGroupIdentifierIsValid(value->peerGroupIdentifiers[i])) {
                    HAPLogError(
                            &logObject,
                            "%s has invalid peerGroupIdentifiers[%zu]: %lu.",
                            name,
                            i,
                            (unsigned long) value->peerGroupIdentifiers[i]);
                    return false;
                }
            }
            if (!value->numICMPTypes || value->numICMPTypes > HAPArrayCount(value->icmpTypes)) {
                HAPLogError(&logObject, "%s has invalid numICMPTypes: %zu.", name, value->numICMPTypes);
                return false;
            }
            for (size_t i = 0; i < value->numICMPTypes; i++) {
                if (!HAPPlatformWiFiRouterICMPTypeIsValid(&value->icmpTypes[i])) {
                    HAPLogError(&logObject, "%s has invalid icmpTypes[%zu].", name, i);
                    return false;
                }
            }
        }
            return true;
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
static bool HAPPlatformWiFiRouterWANTypeIsValid(HAPPlatformWiFiRouterWANType value) {
    switch (value) {
        case kHAPPlatformWiFiRouterWANType_Unconfigured:
        case kHAPPlatformWiFiRouterWANType_Other:
        case kHAPPlatformWiFiRouterWANType_DHCP:
        case kHAPPlatformWiFiRouterWANType_BridgeMode: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
static bool HAPPlatformWiFiRouterWANStatusIsValid(HAPPlatformWiFiRouterWANStatus value) {
    HAPPlatformWiFiRouterWANStatus allOptions =
            kHAPPlatformWiFiRouterWANStatus_Unknown + kHAPPlatformWiFiRouterWANStatus_NoCableConnected +
            kHAPPlatformWiFiRouterWANStatus_NoIPAddress + kHAPPlatformWiFiRouterWANStatus_NoGatewaySpecified +
            kHAPPlatformWiFiRouterWANStatus_GatewayUnreachable + kHAPPlatformWiFiRouterWANStatus_NoDNSServerSpecified +
            kHAPPlatformWiFiRouterWANStatus_DNSServerUnreachable +
            kHAPPlatformWiFiRouterWANStatus_AuthenticationFailed + kHAPPlatformWiFiRouterWANStatus_Walled;
    if (value & (HAPPlatformWiFiRouterWANStatus) ~allOptions) {
        return false;
    }
    return true;
}

HAP_RESULT_USE_CHECK
static bool HAPPlatformWiFiRouterCredentialTypeIsValid(HAPPlatformWiFiRouterCredentialType value) {
    switch (value) {
        case kHAPPlatformWiFiRouterCredentialType_MACAddress:
        case kHAPPlatformWiFiRouterCredentialType_PSK: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
static bool HAPPlatformWiFiRouterClientCredentialIsValid(const HAPPlatformWiFiRouterClientCredential* value_) {
    HAPPrecondition(value_);

    if (!HAPPlatformWiFiRouterCredentialTypeIsValid(value_->type)) {
        HAPLogError(&logObject, "Network client profile credential has invalid type: %u.", value_->type);
        return false;
    }
    switch (value_->type) {
        case kHAPPlatformWiFiRouterCredentialType_MACAddress: {
        }
            return true;
        case kHAPPlatformWiFiRouterCredentialType_PSK: {
            const HAPWiFiWPAPersonalCredential* value = &value_->_.psk;
            const char* name = "WPA/WPA2 Personal credential";

            if (!HAPWiFiWPAPersonalCredentialIsValid(value)) {
                HAPLogError(&logObject, "%s is invalid.", name);
                return false;
            }
        }
            return true;
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
static bool HAPPlatformWiFiRouterClientStatusIsValid(const HAPPlatformWiFiRouterClientStatus* value) {
    HAPPrecondition(value);
    const char* name = "Network client status";

    if (!value->macAddress) {
        HAPLogError(&logObject, "%s has NULL macAddress.", name);
        return false;
    }
    if (value->numIPAddresses) {
        if (!value->ipAddresses) {
            HAPLogError(&logObject, "%s has NULL ipAddresses but %zu numIPAddresses.", name, value->numIPAddresses);
            return false;
        }
        for (size_t i = 0; i < value->numIPAddresses; i++) {
            if (!HAPIPAddressIsValid(&value->ipAddresses[i])) {
                HAPLogError(&logObject, "%s has invalid ipAddresses[%zu].", name, i);
                return false;
            }
        }
    }
    if (value->name) {
        if (!HAPUTF8IsValidData(HAPNonnull(value->name), HAPStringGetNumBytes(HAPNonnull(value->name)))) {
            HAPLogError(&logObject, "%s has invalid name: Not valid UTF-8.", name);
            return false;
        }
    }
    return true;
}

HAP_RESULT_USE_CHECK
static bool
        HAPPlatformWiFiRouterAccessViolationMetadataIsValid(const HAPPlatformWiFiRouterAccessViolationMetadata* value) {
    HAPPrecondition(value);
    const char* name = "Network access violation";

    if (value->hasViolations && value->wasReset) {
        if (value->lastViolationTimestamp < value->lastResetTimestamp) {
            HAPLogError(&logObject, "%s has lastViolationTimestamp before lastResetTimestamp.", name);
            return false;
        }
    }
    return true;
}

HAP_RESULT_USE_CHECK
static bool HAPPlatformWiFiRouterSatelliteStatusIsValid(HAPPlatformWiFiRouterSatelliteStatus value) {
    switch (value) {
        case kHAPPlatformWiFiRouterSatelliteStatus_Unknown:
        case kHAPPlatformWiFiRouterSatelliteStatus_Connected:
        case kHAPPlatformWiFiRouterSatelliteStatus_NotConnected: {
            return true;
        }
        default:
            return false;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void HAPCheckedPlatformWiFiRouterSetDelegate(
        HAPPlatformWiFiRouterRef wiFiRouter,
        const HAPPlatformWiFiRouterDelegate* _Nullable delegate) {
    HAPPrecondition(wiFiRouter);

    HAPPlatformWiFiRouterSetDelegate(wiFiRouter, delegate);
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterAcquireSharedConfigurationAccess(HAPPlatformWiFiRouterRef wiFiRouter) {
    HAPPrecondition(wiFiRouter);

    HAPError err;

    const char* func = "HAPPlatformWiFiRouterAcquireSharedConfigurationAccess";
    err = HAPPlatformWiFiRouterAcquireSharedConfigurationAccess(wiFiRouter);
    if (err) {
        if (err != kHAPError_Busy) {
            HAPLogError(&logObject, "%s failed with unexpected error: %u.", func, err);
            HAPFatalError();
        }
        HAPLogError(&logObject, "%s failed: %u.", func, err);
        return err;
    }
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

void HAPCheckedPlatformWiFiRouterReleaseSharedConfigurationAccess(HAPPlatformWiFiRouterRef wiFiRouter) {
    HAPPrecondition(wiFiRouter);

    HAPPlatformWiFiRouterReleaseSharedConfigurationAccess(wiFiRouter);
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterAcquireExclusiveConfigurationAccess(HAPPlatformWiFiRouterRef wiFiRouter) {
    HAPPrecondition(wiFiRouter);

    HAPError err;

    const char* func = "HAPPlatformWiFiRouterAcquireExclusiveConfigurationAccess";
    err = HAPPlatformWiFiRouterAcquireExclusiveConfigurationAccess(wiFiRouter);
    if (err) {
        if (err != kHAPError_Busy) {
            HAPLogError(&logObject, "%s failed with unexpected error: %u.", func, err);
            HAPFatalError();
        }
        HAPLogError(&logObject, "%s failed: %u.", func, err);
        return err;
    }
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

void HAPCheckedPlatformWiFiRouterReleaseExclusiveConfigurationAccess(HAPPlatformWiFiRouterRef wiFiRouter) {
    HAPPrecondition(wiFiRouter);

    HAPPlatformWiFiRouterReleaseExclusiveConfigurationAccess(wiFiRouter);
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterBeginConfigurationChange(HAPPlatformWiFiRouterRef wiFiRouter) {
    HAPPrecondition(wiFiRouter);

    HAPError err;

    const char* func = "HAPPlatformWiFiRouterBeginConfigurationChange";
    err = HAPPlatformWiFiRouterBeginConfigurationChange(wiFiRouter);
    if (err) {
        if (err != kHAPError_Unknown) {
            HAPLogError(&logObject, "%s failed with unexpected error: %u.", func, err);
            HAPFatalError();
        }
        HAPLogError(&logObject, "%s failed: %u.", func, err);
        return err;
    }
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterCommitConfigurationChange(HAPPlatformWiFiRouterRef wiFiRouter) {
    HAPPrecondition(wiFiRouter);

    HAPError err;

    const char* func = "HAPPlatformWiFiRouterCommitConfigurationChange";
    err = HAPPlatformWiFiRouterCommitConfigurationChange(wiFiRouter);
    if (err) {
        if (err != kHAPError_Unknown) {
            HAPLogError(&logObject, "%s failed with unexpected error: %u.", func, err);
            HAPFatalError();
        }
        HAPLogError(&logObject, "%s failed: %u.", func, err);
        return err;
    }
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

void HAPCheckedPlatformWiFiRouterRollbackConfigurationChange(HAPPlatformWiFiRouterRef wiFiRouter) {
    HAPPrecondition(wiFiRouter);

    HAPPlatformWiFiRouterRollbackConfigurationChange(wiFiRouter);
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterIsReady(HAPPlatformWiFiRouterRef wiFiRouter, bool* isReady) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(isReady);

    HAPError err;

    const char* func = "HAPPlatformWiFiRouterIsReady";
    err = HAPPlatformWiFiRouterIsReady(wiFiRouter, isReady);
    if (err) {
        if (err != kHAPError_Unknown) {
            HAPLogError(&logObject, "%s failed with unexpected error: %u.", func, err);
            HAPFatalError();
        }
        HAPLogError(&logObject, "%s failed: %u.", func, err);
        return err;
    }
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterIsManagedNetworkEnabled(HAPPlatformWiFiRouterRef wiFiRouter, bool* isEnabled) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(isEnabled);

    HAPError err;

    const char* func = "HAPPlatformWiFiRouterIsManagedNetworkEnabled";
    err = HAPPlatformWiFiRouterIsManagedNetworkEnabled(wiFiRouter, isEnabled);
    if (err) {
        if (err != kHAPError_Unknown) {
            HAPLogError(&logObject, "%s failed with unexpected error: %u.", func, err);
            HAPFatalError();
        }
        HAPLogError(&logObject, "%s failed: %u.", func, err);
        return err;
    }
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterSetManagedNetworkEnabled(HAPPlatformWiFiRouterRef wiFiRouter, bool isEnabled) {
    HAPPrecondition(wiFiRouter);

    HAPError err;

    const char* func = "HAPPlatformWiFiRouterSetManagedNetworkEnabled";
    err = HAPPlatformWiFiRouterSetManagedNetworkEnabled(wiFiRouter, isEnabled);
    if (err) {
        if (err != kHAPError_Unknown && err != kHAPError_InvalidState) {
            HAPLogError(&logObject, "%s failed with unexpected error: %u.", func, err);
            HAPFatalError();
        }
        if (!isEnabled && err == kHAPError_InvalidState) {
            HAPLogError(&logObject, "%s failed with error: %u which is only allowed if isEnabled is set.", func, err);
            HAPFatalError();
        }
        HAPLogError(&logObject, "%s failed: %u.", func, err);
        return err;
    }
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

typedef struct {
    HAPPlatformWiFiRouterEnumerateWANsCallback callback;
    void* _Nullable context;
    bool shouldContinue : 1;
} EnumerateWANsContext;

static void EnumerateWANsCallback(
        void* _Nullable context_,
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterWANIdentifier wanIdentifier,
        bool* shouldContinue) {
    const char* func = "HAPPlatformWiFiRouterEnumerateWANs";

    if (!context_) {
        HAPLogError(
                &logObject,
                "%s listed entry with NULL context. "
                "The context passed to %s should be provided instead.",
                func,
                func);
        HAPFatalError();
    }
    EnumerateWANsContext* enumerateContext = context_;
    if (!enumerateContext->callback) {
        HAPLogError(
                &logObject,
                "%s listed entry with invalid context. "
                "The context passed to %s should be provided instead.",
                func,
                func);
        HAPFatalError();
    }
    HAPPlatformWiFiRouterEnumerateWANsCallback callback = enumerateContext->callback;
    void* _Nullable context = enumerateContext->context;

    if (!wiFiRouter) {
        HAPLogError(&logObject, "%s listed entry with NULL wifiRouter.", func);
        HAPFatalError();
    }

    if (!HAPPlatformWiFiRouterWANIdentifierIsValid(wanIdentifier)) {
        HAPLogError(
                &logObject, "%s listed entry with invalid wanIdentifier: %lu.", func, (unsigned long) wanIdentifier);
        HAPFatalError();
    }

    if (!shouldContinue) {
        HAPLogError(&logObject, "%s listed entry with NULL shouldContinue.", func);
        HAPFatalError();
    }
    if (!enumerateContext->shouldContinue) {
        HAPLogError(&logObject, "%s listed entry although shouldContinue was cleared before.", func);
        HAPFatalError();
    }
    if (!*shouldContinue) {
        HAPLogError(&logObject, "%s listed entry without setting shouldContinue.", func);
        HAPFatalError();
    }

    callback(context, wiFiRouter, wanIdentifier, shouldContinue);
    enumerateContext->shouldContinue = *shouldContinue;
}

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterEnumerateWANs(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterEnumerateWANsCallback callback,
        void* _Nullable context) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(callback);

    HAPError err;

    const char* func = "HAPPlatformWiFiRouterEnumerateWANs";
    EnumerateWANsContext enumerateContext;
    HAPRawBufferZero(&enumerateContext, sizeof enumerateContext);
    enumerateContext.callback = callback;
    enumerateContext.context = context;
    enumerateContext.shouldContinue = true;
    err = HAPPlatformWiFiRouterEnumerateWANs(wiFiRouter, EnumerateWANsCallback, &enumerateContext);
    if (err) {
        if (err != kHAPError_Unknown) {
            HAPLogError(&logObject, "%s failed with unexpected error: %u.", func, err);
            HAPFatalError();
        }
        HAPLogError(&logObject, "%s failed: %u.", func, err);
        return err;
    }
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterWANExists(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterWANIdentifier wanIdentifier,
        bool* exists) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterWANIdentifierIsValid(wanIdentifier));
    HAPPrecondition(exists);

    HAPError err;

    const char* func = "HAPPlatformWiFiRouterWANExists";
    err = HAPPlatformWiFiRouterWANExists(wiFiRouter, wanIdentifier, exists);
    if (err) {
        if (err != kHAPError_Unknown) {
            HAPLogError(&logObject, "%s failed with unexpected error: %u.", func, err);
            HAPFatalError();
        }
        HAPLogError(&logObject, "%s failed: %u.", func, err);
        return err;
    }
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterWANGetType(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterWANIdentifier wanIdentifier,
        HAPPlatformWiFiRouterWANType* wanType) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterWANIdentifierIsValid(wanIdentifier));
    HAPPrecondition(wanType);

    HAPError err;

    const char* func = "HAPPlatformWiFiRouterWANGetType";
    err = HAPPlatformWiFiRouterWANGetType(wiFiRouter, wanIdentifier, wanType);
    if (err) {
        if (err != kHAPError_Unknown && err != kHAPError_InvalidState) {
            HAPLogError(&logObject, "%s failed with unexpected error: %u.", func, err);
            HAPFatalError();
        }
        HAPLogError(&logObject, "%s failed: %u.", func, err);
        return err;
    }
    if (!HAPPlatformWiFiRouterWANTypeIsValid(*wanType)) {
        HAPLogError(&logObject, "%s returned invalid wanType: %u.", func, *wanType);
        HAPFatalError();
    }
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterWANGetStatus(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterWANIdentifier wanIdentifier,
        HAPPlatformWiFiRouterWANStatus* wanStatus) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterWANIdentifierIsValid(wanIdentifier));
    HAPPrecondition(wanStatus);

    HAPError err;

    const char* func = "HAPPlatformWiFiRouterWANGetStatus";
    err = HAPPlatformWiFiRouterWANGetStatus(wiFiRouter, wanIdentifier, wanStatus);
    if (err) {
        if (err != kHAPError_Unknown && err != kHAPError_InvalidState) {
            HAPLogError(&logObject, "%s failed with unexpected error: %u.", func, err);
            HAPFatalError();
        }
        HAPLogError(&logObject, "%s failed: %u.", func, err);
        return err;
    }
    if (!HAPPlatformWiFiRouterWANStatusIsValid(*wanStatus)) {
        HAPLogError(&logObject, "%s returned invalid wanStatus: %u.", func, *wanStatus);
        HAPFatalError();
    }
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

typedef struct {
    HAPPlatformWiFiRouterEnumerateClientsCallback callback;
    void* _Nullable context;
    bool shouldContinue : 1;
} EnumerateClientsContext;

static void EnumerateClientsCallback(
        void* _Nullable context_,
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        bool* shouldContinue) {
    const char* func = "HAPPlatformWiFiRouterEnumerateClients";

    if (!context_) {
        HAPLogError(
                &logObject,
                "%s listed entry with NULL context. "
                "The context passed to %s should be provided instead.",
                func,
                func);
        HAPFatalError();
    }
    EnumerateClientsContext* enumerateContext = context_;
    if (!enumerateContext->callback) {
        HAPLogError(
                &logObject,
                "%s listed entry with invalid context. "
                "The context passed to %s should be provided instead.",
                func,
                func);
        HAPFatalError();
    }
    HAPPlatformWiFiRouterEnumerateClientsCallback callback = enumerateContext->callback;
    void* _Nullable context = enumerateContext->context;

    if (!wiFiRouter) {
        HAPLogError(&logObject, "%s listed entry with NULL wifiRouter.", func);
        HAPFatalError();
    }

    if (!HAPPlatformWiFiRouterClientIdentifierIsValid(clientIdentifier)) {
        HAPLogError(
                &logObject,
                "%s listed entry with invalid clientIdentifier: %lu.",
                func,
                (unsigned long) clientIdentifier);
        HAPFatalError();
    }

    if (!shouldContinue) {
        HAPLogError(&logObject, "%s listed entry with NULL shouldContinue.", func);
        HAPFatalError();
    }
    if (!enumerateContext->shouldContinue) {
        HAPLogError(&logObject, "%s listed entry although shouldContinue was cleared before.", func);
        HAPFatalError();
    }
    if (!*shouldContinue) {
        HAPLogError(&logObject, "%s listed entry without setting shouldContinue.", func);
        HAPFatalError();
    }

    callback(context, wiFiRouter, clientIdentifier, shouldContinue);
    enumerateContext->shouldContinue = *shouldContinue;
}

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterEnumerateClients(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterEnumerateClientsCallback callback,
        void* _Nullable context) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(callback);

    HAPError err;

    const char* func = "HAPPlatformWiFiRouterEnumerateClients";
    EnumerateClientsContext enumerateContext;
    HAPRawBufferZero(&enumerateContext, sizeof enumerateContext);
    enumerateContext.callback = callback;
    enumerateContext.context = context;
    enumerateContext.shouldContinue = true;
    err = HAPPlatformWiFiRouterEnumerateClients(wiFiRouter, EnumerateClientsCallback, &enumerateContext);
    if (err) {
        if (err != kHAPError_Unknown) {
            HAPLogError(&logObject, "%s failed with unexpected error: %u.", func, err);
            HAPFatalError();
        }
        HAPLogError(&logObject, "%s failed: %u.", func, err);
        return err;
    }
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterClientExists(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        bool* exists) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterClientIdentifierIsValid(clientIdentifier));
    HAPPrecondition(exists);

    HAPError err;

    const char* func = "HAPPlatformWiFiRouterClientExists";
    err = HAPPlatformWiFiRouterClientExists(wiFiRouter, clientIdentifier, exists);
    if (err) {
        if (err != kHAPError_Unknown) {
            HAPLogError(&logObject, "%s failed with unexpected error: %u.", func, err);
            HAPFatalError();
        }
        HAPLogError(&logObject, "%s failed: %u.", func, err);
        return err;
    }
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterAddClient(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterGroupIdentifier groupIdentifier,
        const HAPPlatformWiFiRouterClientCredential* credential,
        HAPPlatformWiFiRouterClientIdentifier* clientIdentifier) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterGroupIdentifierIsValid(groupIdentifier));
    HAPPrecondition(credential);
    HAPPrecondition(HAPPlatformWiFiRouterClientCredentialIsValid(credential));
    HAPPrecondition(clientIdentifier);

    HAPError err;

    const char* func = "HAPPlatformWiFiRouterAddClient";
    err = HAPPlatformWiFiRouterAddClient(wiFiRouter, groupIdentifier, credential, clientIdentifier);
    if (err) {
        if (err != kHAPError_Unknown && err != kHAPError_InvalidData && err != kHAPError_OutOfResources &&
            err != kHAPError_NotAuthorized) {
            HAPLogError(&logObject, "%s failed with unexpected error: %u.", func, err);
            HAPFatalError();
        }
        HAPLogError(&logObject, "%s failed: %u.", func, err);
        return err;
    }
    if (!HAPPlatformWiFiRouterClientIdentifierIsValid(*clientIdentifier)) {
        HAPLogError(&logObject, "%s returned invalid clientIdentifier: %lu.", func, (unsigned long) *clientIdentifier);
        HAPFatalError();
    }
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterRemoveClient(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterClientIdentifierIsValid(clientIdentifier));

    HAPError err;

    const char* func = "HAPPlatformWiFiRouterRemoveClient";
    err = HAPPlatformWiFiRouterRemoveClient(wiFiRouter, clientIdentifier);
    if (err) {
        if (err != kHAPError_Unknown && err != kHAPError_OutOfResources) {
            HAPLogError(&logObject, "%s failed with unexpected error: %u.", func, err);
            HAPFatalError();
        }
        HAPLogError(&logObject, "%s failed: %u.", func, err);
        return err;
    }
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterClientGetGroupIdentifier(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterGroupIdentifier* groupIdentifier) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterClientIdentifierIsValid(clientIdentifier));
    HAPPrecondition(groupIdentifier);

    HAPError err;

    const char* func = "HAPPlatformWiFiRouterClientGetGroupIdentifier";
    err = HAPPlatformWiFiRouterClientGetGroupIdentifier(wiFiRouter, clientIdentifier, groupIdentifier);
    if (err) {
        if (err != kHAPError_Unknown && err != kHAPError_InvalidState) {
            HAPLogError(&logObject, "%s failed with unexpected error: %u.", func, err);
            HAPFatalError();
        }
        HAPLogError(&logObject, "%s failed: %u.", func, err);
        return err;
    }
    if (!HAPPlatformWiFiRouterGroupIdentifierIsValid(*groupIdentifier)) {
        HAPLogError(&logObject, "%s returned invalid groupIdentifier: %lu.", func, (unsigned long) *groupIdentifier);
        HAPFatalError();
    }
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterClientSetGroupIdentifier(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterGroupIdentifier groupIdentifier) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterClientIdentifierIsValid(clientIdentifier));
    HAPPrecondition(HAPPlatformWiFiRouterGroupIdentifierIsValid(groupIdentifier));

    HAPError err;

    const char* func = "HAPPlatformWiFiRouterClientSetGroupIdentifier";
    err = HAPPlatformWiFiRouterClientSetGroupIdentifier(wiFiRouter, clientIdentifier, groupIdentifier);
    if (err) {
        if (err != kHAPError_Unknown && err != kHAPError_InvalidState && err != kHAPError_InvalidData &&
            err != kHAPError_OutOfResources) {
            HAPLogError(&logObject, "%s failed with unexpected error: %u.", func, err);
            HAPFatalError();
        }
        HAPLogError(&logObject, "%s failed: %u.", func, err);
        return err;
    }
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterClientGetCredentialType(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterCredentialType* credentialType) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterClientIdentifierIsValid(clientIdentifier));
    HAPPrecondition(credentialType);

    HAPError err;

    const char* func = "HAPPlatformWiFiRouterClientGetCredentialType";
    err = HAPPlatformWiFiRouterClientGetCredentialType(wiFiRouter, clientIdentifier, credentialType);
    if (err) {
        if (err != kHAPError_Unknown && err != kHAPError_InvalidState) {
            HAPLogError(&logObject, "%s failed with unexpected error: %u.", func, err);
            HAPFatalError();
        }
        HAPLogError(&logObject, "%s failed: %u.", func, err);
        return err;
    }
    if (!HAPPlatformWiFiRouterCredentialTypeIsValid(*credentialType)) {
        HAPLogError(&logObject, "%s returned invalid credentialType: %u.", func, *credentialType);
        HAPFatalError();
    }
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterClientGetMACAddressCredential(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPMACAddress* credential) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterClientIdentifierIsValid(clientIdentifier));
    HAPPrecondition(credential);

    HAPError err;

    const char* func = "HAPPlatformWiFiRouterClientGetMACAddressCredential";
    err = HAPPlatformWiFiRouterClientGetMACAddressCredential(wiFiRouter, clientIdentifier, credential);
    if (err) {
        if (err != kHAPError_Unknown && err != kHAPError_InvalidState) {
            HAPLogError(&logObject, "%s failed with unexpected error: %u.", func, err);
            HAPFatalError();
        }
        HAPLogError(&logObject, "%s failed: %u.", func, err);
        return err;
    }
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterClientSetCredential(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        const HAPPlatformWiFiRouterClientCredential* credential) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterClientIdentifierIsValid(clientIdentifier));
    HAPPrecondition(credential);
    HAPPrecondition(HAPPlatformWiFiRouterClientCredentialIsValid(credential));

    HAPError err;

    const char* func = "HAPPlatformWiFiRouterClientSetCredential";
    err = HAPPlatformWiFiRouterClientSetCredential(wiFiRouter, clientIdentifier, credential);
    if (err) {
        if (err != kHAPError_Unknown && err != kHAPError_InvalidState && err != kHAPError_InvalidData &&
            err != kHAPError_OutOfResources && err != kHAPError_NotAuthorized) {
            HAPLogError(&logObject, "%s failed with unexpected error: %u.", func, err);
            HAPFatalError();
        }
        HAPLogError(&logObject, "%s failed: %u.", func, err);
        return err;
    }
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterClientGetWANFirewallType(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterFirewallType* firewallType) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterClientIdentifierIsValid(clientIdentifier));
    HAPPrecondition(firewallType);

    HAPError err;

    const char* func = "HAPPlatformWiFiRouterClientGetWANFirewallType";
    err = HAPPlatformWiFiRouterClientGetWANFirewallType(wiFiRouter, clientIdentifier, firewallType);
    if (err) {
        if (err != kHAPError_Unknown && err != kHAPError_InvalidState) {
            HAPLogError(&logObject, "%s failed with unexpected error: %u.", func, err);
            HAPFatalError();
        }
        HAPLogError(&logObject, "%s failed: %u.", func, err);
        return err;
    }
    if (!HAPPlatformWiFiRouterFirewallTypeIsValid(*firewallType)) {
        HAPLogError(&logObject, "%s returned invalid firewallType: %u.", func, *firewallType);
        HAPFatalError();
    }
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

typedef struct {
    HAPPlatformWiFiRouterClientEnumerateWANFirewallRulesCallback callback;
    void* _Nullable context;
    bool shouldContinue : 1;
} ClientEnumerateWANFirewallRulesContext;

static void ClientEnumerateWANFirewallRulesCallback(
        void* _Nullable context_,
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        const HAPPlatformWiFiRouterWANFirewallRule* firewallRule,
        bool* shouldContinue) {
    const char* func = "HAPPlatformWiFiRouterClientEnumerateWANFirewallRules";

    if (!context_) {
        HAPLogError(
                &logObject,
                "%s listed entry with NULL context. "
                "The context passed to %s should be provided instead.",
                func,
                func);
        HAPFatalError();
    }
    ClientEnumerateWANFirewallRulesContext* enumerateContext = context_;
    if (!enumerateContext->callback) {
        HAPLogError(
                &logObject,
                "%s listed entry with invalid context. "
                "The context passed to %s should be provided instead.",
                func,
                func);
        HAPFatalError();
    }
    HAPPlatformWiFiRouterClientEnumerateWANFirewallRulesCallback callback = enumerateContext->callback;
    void* _Nullable context = enumerateContext->context;

    if (!wiFiRouter) {
        HAPLogError(&logObject, "%s listed entry with NULL wifiRouter.", func);
        HAPFatalError();
    }

    if (!HAPPlatformWiFiRouterClientIdentifierIsValid(clientIdentifier)) {
        HAPLogError(
                &logObject,
                "%s listed entry with invalid clientIdentifier: %lu.",
                func,
                (unsigned long) clientIdentifier);
        HAPFatalError();
    }

    if (!firewallRule) {
        HAPLogError(&logObject, "%s listed entry with NULL firewallRule.", func);
        HAPFatalError();
    }
    if (!HAPPlatformWiFiRouterWANFirewallRuleIsValid(firewallRule)) {
        HAPLogError(&logObject, "%s listed entry with invalid firewallRule.", func);
        HAPFatalError();
    }

    if (!shouldContinue) {
        HAPLogError(&logObject, "%s listed entry with NULL shouldContinue.", func);
        HAPFatalError();
    }
    if (!enumerateContext->shouldContinue) {
        HAPLogError(&logObject, "%s listed entry although shouldContinue was cleared before.", func);
        HAPFatalError();
    }
    if (!*shouldContinue) {
        HAPLogError(&logObject, "%s listed entry without setting shouldContinue.", func);
        HAPFatalError();
    }

    callback(context, wiFiRouter, clientIdentifier, firewallRule, shouldContinue);
    enumerateContext->shouldContinue = *shouldContinue;
}

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterClientEnumerateWANFirewallRules(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterClientEnumerateWANFirewallRulesCallback callback,
        void* _Nullable context) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterClientIdentifierIsValid(clientIdentifier));
    HAPPrecondition(callback);

    HAPError err;

    const char* func = "HAPPlatformWiFiRouterClientEnumerateWANFirewallRules";
    ClientEnumerateWANFirewallRulesContext enumerateContext;
    HAPRawBufferZero(&enumerateContext, sizeof enumerateContext);
    enumerateContext.callback = callback;
    enumerateContext.context = context;
    enumerateContext.shouldContinue = true;
    err = HAPPlatformWiFiRouterClientEnumerateWANFirewallRules(
            wiFiRouter, clientIdentifier, ClientEnumerateWANFirewallRulesCallback, &enumerateContext);
    if (err) {
        if (err != kHAPError_Unknown && err != kHAPError_InvalidState) {
            HAPLogError(&logObject, "%s failed with unexpected error: %u.", func, err);
            HAPFatalError();
        }
        HAPLogError(&logObject, "%s failed: %u.", func, err);
        return err;
    }
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAPError HAPCheckedPlatformWiFiRouterClientResetWANFirewall(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterFirewallType firewallType) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterClientIdentifierIsValid(clientIdentifier));
    HAPPrecondition(HAPPlatformWiFiRouterFirewallTypeIsValid(firewallType));

    HAPError err;

    const char* func = "HAPPlatformWiFiRouterClientResetWANFirewall";
    err = HAPPlatformWiFiRouterClientResetWANFirewall(wiFiRouter, clientIdentifier, firewallType);
    if (err) {
        if (err != kHAPError_Unknown && err != kHAPError_InvalidState && err != kHAPError_InvalidData &&
            err != kHAPError_OutOfResources) {
            HAPLogError(&logObject, "%s failed with unexpected error: %u.", func, err);
            HAPFatalError();
        }
        HAPLogError(&logObject, "%s failed: %u.", func, err);
        return err;
    }
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterClientAddWANFirewallRule(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        const HAPPlatformWiFiRouterWANFirewallRule* firewallRule) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterClientIdentifierIsValid(clientIdentifier));
    HAPPrecondition(firewallRule);
    HAPPrecondition(HAPPlatformWiFiRouterWANFirewallRuleIsValid(firewallRule));

    HAPError err;

    const char* func = "HAPPlatformWiFiRouterClientAddWANFirewallRule";
    err = HAPPlatformWiFiRouterClientAddWANFirewallRule(wiFiRouter, clientIdentifier, firewallRule);
    if (err) {
        if (err != kHAPError_Unknown && err != kHAPError_InvalidData && err != kHAPError_OutOfResources) {
            HAPLogError(&logObject, "%s failed with unexpected error: %u.", func, err);
            HAPFatalError();
        }
        HAPLogError(&logObject, "%s failed: %u.", func, err);
        return err;
    }
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterClientFinalizeWANFirewallRules(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterClientIdentifierIsValid(clientIdentifier));

    HAPError err;

    const char* func = "HAPPlatformWiFiRouterClientFinalizeWANFirewallRules";
    err = HAPPlatformWiFiRouterClientFinalizeWANFirewallRules(wiFiRouter, clientIdentifier);
    if (err) {
        if (err != kHAPError_Unknown && err != kHAPError_OutOfResources) {
            HAPLogError(&logObject, "%s failed with unexpected error: %u.", func, err);
            HAPFatalError();
        }
        HAPLogError(&logObject, "%s failed: %u.", func, err);
        return err;
    }
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterClientGetLANFirewallType(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterFirewallType* firewallType) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterClientIdentifierIsValid(clientIdentifier));
    HAPPrecondition(firewallType);

    HAPError err;

    const char* func = "HAPPlatformWiFiRouterClientGetLANFirewallType";
    err = HAPPlatformWiFiRouterClientGetLANFirewallType(wiFiRouter, clientIdentifier, firewallType);
    if (err) {
        if (err != kHAPError_Unknown && err != kHAPError_InvalidState) {
            HAPLogError(&logObject, "%s failed with unexpected error: %u.", func, err);
            HAPFatalError();
        }
        HAPLogError(&logObject, "%s failed: %u.", func, err);
        return err;
    }
    if (!HAPPlatformWiFiRouterFirewallTypeIsValid(*firewallType)) {
        HAPLogError(&logObject, "%s returned invalid firewallType: %u.", func, *firewallType);
        HAPFatalError();
    }
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

typedef struct {
    HAPPlatformWiFiRouterClientEnumerateLANFirewallRulesCallback callback;
    void* _Nullable context;
    bool shouldContinue : 1;
} ClientEnumerateLANFirewallRulesContext;

static void ClientEnumerateLANFirewallRulesCallback(
        void* _Nullable context_,
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        const HAPPlatformWiFiRouterLANFirewallRule* firewallRule,
        bool* shouldContinue) {
    const char* func = "HAPPlatformWiFiRouterClientEnumerateLANFirewallRules";

    if (!context_) {
        HAPLogError(
                &logObject,
                "%s listed entry with NULL context. "
                "The context passed to %s should be provided instead.",
                func,
                func);
        HAPFatalError();
    }
    ClientEnumerateLANFirewallRulesContext* enumerateContext = context_;
    if (!enumerateContext->callback) {
        HAPLogError(
                &logObject,
                "%s listed entry with invalid context. "
                "The context passed to %s should be provided instead.",
                func,
                func);
        HAPFatalError();
    }
    HAPPlatformWiFiRouterClientEnumerateLANFirewallRulesCallback callback = enumerateContext->callback;
    void* _Nullable context = enumerateContext->context;

    if (!wiFiRouter) {
        HAPLogError(&logObject, "%s listed entry with NULL wifiRouter.", func);
        HAPFatalError();
    }

    if (!HAPPlatformWiFiRouterClientIdentifierIsValid(clientIdentifier)) {
        HAPLogError(
                &logObject,
                "%s listed entry with invalid clientIdentifier: %lu.",
                func,
                (unsigned long) clientIdentifier);
        HAPFatalError();
    }

    if (!firewallRule) {
        HAPLogError(&logObject, "%s listed entry with NULL firewallRule.", func);
        HAPFatalError();
    }
    if (!HAPPlatformWiFiRouterLANFirewallRuleIsValid(firewallRule)) {
        HAPLogError(&logObject, "%s listed entry with invalid firewallRule.", func);
        HAPFatalError();
    }

    if (!shouldContinue) {
        HAPLogError(&logObject, "%s listed entry with NULL shouldContinue.", func);
        HAPFatalError();
    }
    if (!enumerateContext->shouldContinue) {
        HAPLogError(&logObject, "%s listed entry although shouldContinue was cleared before.", func);
        HAPFatalError();
    }
    if (!*shouldContinue) {
        HAPLogError(&logObject, "%s listed entry without setting shouldContinue.", func);
        HAPFatalError();
    }

    callback(context, wiFiRouter, clientIdentifier, firewallRule, shouldContinue);
    enumerateContext->shouldContinue = *shouldContinue;
}

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterClientEnumerateLANFirewallRules(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterClientEnumerateLANFirewallRulesCallback callback,
        void* _Nullable context) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterClientIdentifierIsValid(clientIdentifier));
    HAPPrecondition(callback);

    HAPError err;

    const char* func = "HAPPlatformWiFiRouterClientEnumerateLANFirewallRules";
    ClientEnumerateLANFirewallRulesContext enumerateContext;
    HAPRawBufferZero(&enumerateContext, sizeof enumerateContext);
    enumerateContext.callback = callback;
    enumerateContext.context = context;
    enumerateContext.shouldContinue = true;
    err = HAPPlatformWiFiRouterClientEnumerateLANFirewallRules(
            wiFiRouter, clientIdentifier, ClientEnumerateLANFirewallRulesCallback, &enumerateContext);
    if (err) {
        if (err != kHAPError_Unknown && err != kHAPError_InvalidState) {
            HAPLogError(&logObject, "%s failed with unexpected error: %u.", func, err);
            HAPFatalError();
        }
        HAPLogError(&logObject, "%s failed: %u.", func, err);
        return err;
    }
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterClientResetLANFirewall(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterFirewallType firewallType) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterClientIdentifierIsValid(clientIdentifier));
    HAPPrecondition(HAPPlatformWiFiRouterFirewallTypeIsValid(firewallType));

    HAPError err;

    const char* func = "HAPPlatformWiFiRouterClientResetLANFirewall";
    err = HAPPlatformWiFiRouterClientResetLANFirewall(wiFiRouter, clientIdentifier, firewallType);
    if (err) {
        if (err != kHAPError_Unknown && err != kHAPError_InvalidState && err != kHAPError_InvalidData &&
            err != kHAPError_OutOfResources) {
            HAPLogError(&logObject, "%s failed with unexpected error: %u.", func, err);
            HAPFatalError();
        }
        HAPLogError(&logObject, "%s failed: %u.", func, err);
        return err;
    }
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterClientAddLANFirewallRule(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        const HAPPlatformWiFiRouterLANFirewallRule* firewallRule) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterClientIdentifierIsValid(clientIdentifier));
    HAPPrecondition(firewallRule);
    HAPPrecondition(HAPPlatformWiFiRouterLANFirewallRuleIsValid(HAPNonnullVoid(firewallRule)));

    HAPError err;

    const char* func = "HAPPlatformWiFiRouterClientAddLANFirewallRule";
    err = HAPPlatformWiFiRouterClientAddLANFirewallRule(wiFiRouter, clientIdentifier, firewallRule);
    if (err) {
        if (err != kHAPError_Unknown && err != kHAPError_InvalidData && err != kHAPError_OutOfResources) {
            HAPLogError(&logObject, "%s failed with unexpected error: %u.", func, err);
            HAPFatalError();
        }
        HAPLogError(&logObject, "%s failed: %u.", func, err);
        return err;
    }
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterClientFinalizeLANFirewallRules(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterClientIdentifierIsValid(clientIdentifier));

    HAPError err;

    const char* func = "HAPPlatformWiFiRouterClientFinalizeLANFirewallRules";
    err = HAPPlatformWiFiRouterClientFinalizeLANFirewallRules(wiFiRouter, clientIdentifier);
    if (err) {
        if (err != kHAPError_Unknown && err != kHAPError_OutOfResources) {
            HAPLogError(&logObject, "%s failed with unexpected error: %u.", func, err);
            HAPFatalError();
        }
        HAPLogError(&logObject, "%s failed: %u.", func, err);
        return err;
    }
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
static bool ShouldIncludeClientStatusForIdentifiers(
        const HAPPlatformWiFiRouterClientStatus* clientStatus,
        const HAPPlatformWiFiRouterClientStatusIdentifier* statusIdentifiers,
        size_t numStatusIdentifiers) {
    HAPPrecondition(clientStatus);
    HAPPrecondition(statusIdentifiers);

    for (size_t i = 0; i < numStatusIdentifiers; i++) {
        const HAPPlatformWiFiRouterClientStatusIdentifier* statusIdentifier = &statusIdentifiers[i];
        switch (statusIdentifier->format) {
            case kHAPPlatformWiFiRouterClientStatusIdentifierFormat_Client: {
                if (clientStatus->clientIdentifier == statusIdentifier->clientIdentifier._) {
                    return true;
                }
                break;
            }
            case kHAPPlatformWiFiRouterClientStatusIdentifierFormat_MACAddress: {
                if (HAPMACAddressAreEqual(&statusIdentifier->macAddress._, clientStatus->macAddress)) {
                    return true;
                }
                break;
            }
            case kHAPPlatformWiFiRouterClientStatusIdentifierFormat_IPAddress: {
                for (size_t j = 0; j < clientStatus->numIPAddresses; j++) {
                    if (HAPIPAddressAreEqual(&statusIdentifier->ipAddress._, &clientStatus->ipAddresses[j])) {
                        return true;
                    }
                }
                break;
            }
        }
    }
    return false;
}

typedef struct {
    const HAPPlatformWiFiRouterClientStatusIdentifier* statusIdentifiers;
    size_t numStatusIdentifiers;
    HAPPlatformWiFiRouterGetClientStatusForIdentifiersCallback callback;
    void* _Nullable context;
    bool shouldContinue : 1;
} GetClientStatusForIdentifiersContext;

static void GetClientStatusForIdentifiersCallback(
        void* _Nullable context_,
        HAPPlatformWiFiRouterRef wiFiRouter,
        const HAPPlatformWiFiRouterClientStatus* clientStatus,
        bool* shouldContinue) {
    const char* func = "HAPPlatformWiFiRouterGetClientStatusForIdentifiers";

    if (!context_) {
        HAPLogError(
                &logObject,
                "%s listed entry with NULL context. "
                "The context passed to %s should be provided instead.",
                func,
                func);
        HAPFatalError();
    }
    GetClientStatusForIdentifiersContext* enumerateContext = context_;
    if (!enumerateContext->callback) {
        HAPLogError(
                &logObject,
                "%s listed entry with invalid context. "
                "The context passed to %s should be provided instead.",
                func,
                func);
        HAPFatalError();
    }
    const HAPPlatformWiFiRouterClientStatusIdentifier* statusIdentifiers = enumerateContext->statusIdentifiers;
    size_t numStatusIdentifiers = enumerateContext->numStatusIdentifiers;
    HAPPlatformWiFiRouterGetClientStatusForIdentifiersCallback callback = enumerateContext->callback;
    void* _Nullable context = enumerateContext->context;

    if (!wiFiRouter) {
        HAPLogError(&logObject, "%s listed entry with NULL wifiRouter.", func);
        HAPFatalError();
    }

    if (!clientStatus) {
        HAPLogError(&logObject, "%s listed entry with NULL clientStatus.", func);
        HAPFatalError();
    }
    if (!HAPPlatformWiFiRouterClientStatusIsValid(clientStatus)) {
        HAPLogError(&logObject, "%s listed entry with invalid clientStatus.", func);
        HAPFatalError();
    }

    if (!shouldContinue) {
        HAPLogError(&logObject, "%s listed entry with NULL shouldContinue.", func);
        HAPFatalError();
    }
    if (!enumerateContext->shouldContinue) {
        HAPLogError(&logObject, "%s listed entry although shouldContinue was cleared before.", func);
        HAPFatalError();
    }
    if (!*shouldContinue) {
        HAPLogError(&logObject, "%s listed entry without setting shouldContinue.", func);
        HAPFatalError();
    }

    if (!ShouldIncludeClientStatusForIdentifiers(clientStatus, statusIdentifiers, numStatusIdentifiers)) {
        HAPLogError(&logObject, "%s listed entry that does not match any requested statusIdentifiers.", func);
        HAPFatalError();
    }

    callback(context, wiFiRouter, clientStatus, shouldContinue);
    enumerateContext->shouldContinue = *shouldContinue;
}

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterGetClientStatusForIdentifiers(
        HAPPlatformWiFiRouterRef wiFiRouter,
        const HAPPlatformWiFiRouterClientStatusIdentifier* statusIdentifiers,
        size_t numStatusIdentifiers,
        HAPPlatformWiFiRouterGetClientStatusForIdentifiersCallback callback,
        void* _Nullable context) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(statusIdentifiers);
    HAPPrecondition(callback);

    HAPError err;

    const char* func = "HAPPlatformWiFiRouterGetClientStatusForIdentifiers";
    GetClientStatusForIdentifiersContext enumerateContext;
    HAPRawBufferZero(&enumerateContext, sizeof enumerateContext);
    enumerateContext.statusIdentifiers = statusIdentifiers;
    enumerateContext.numStatusIdentifiers = numStatusIdentifiers;
    enumerateContext.callback = callback;
    enumerateContext.context = context;
    enumerateContext.shouldContinue = true;
    err = HAPPlatformWiFiRouterGetClientStatusForIdentifiers(
            wiFiRouter,
            statusIdentifiers,
            numStatusIdentifiers,
            GetClientStatusForIdentifiersCallback,
            &enumerateContext);
    if (err) {
        if (err != kHAPError_Unknown) {
            HAPLogError(&logObject, "%s failed with unexpected error: %u.", func, err);
            HAPFatalError();
        }
        HAPLogError(&logObject, "%s failed: %u.", func, err);
        return err;
    }
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterClientGetAccessViolationMetadata(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterAccessViolationMetadata* violationMetadata) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterClientIdentifierIsValid(clientIdentifier));
    HAPPrecondition(violationMetadata);

    HAPError err;

    const char* func = "HAPPlatformWiFiRouterClientGetAccessViolationMetadata";
    err = HAPPlatformWiFiRouterClientGetAccessViolationMetadata(wiFiRouter, clientIdentifier, violationMetadata);
    if (err) {
        if (err != kHAPError_Unknown && err != kHAPError_InvalidState) {
            HAPLogError(&logObject, "%s failed with unexpected error: %u.", func, err);
            HAPFatalError();
        }
        HAPLogError(&logObject, "%s failed: %u.", func, err);
        return err;
    }
    if (!HAPPlatformWiFiRouterAccessViolationMetadataIsValid(violationMetadata)) {
        HAPLogError(&logObject, "%s returned invalid violationMetadata.", func);
        HAPFatalError();
    }
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPCheckedPlatformWiFiRouterClientResetAccessViolations(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterClientIdentifierIsValid(clientIdentifier));

    HAPError err;

    const char* func = "HAPPlatformWiFiRouterClientResetAccessViolations";
    err = HAPPlatformWiFiRouterClientResetAccessViolations(wiFiRouter, clientIdentifier);
    if (err) {
        if (err != kHAPError_Unknown) {
            HAPLogError(&logObject, "%s failed with unexpected error: %u.", func, err);
            HAPFatalError();
        }
        HAPLogError(&logObject, "%s failed: %u.", func, err);
        return err;
    }
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPPlatformWiFiRouterSatelliteStatus
        HAPCheckedPlatformWiFiRouterGetSatelliteStatus(HAPPlatformWiFiRouterRef wiFiRouter, size_t satelliteIndex) {
    HAPPrecondition(wiFiRouter);

    const char* func = "HAPPlatformWiFiRouterGetSatelliteStatus";
    HAPPlatformWiFiRouterSatelliteStatus value = HAPPlatformWiFiRouterGetSatelliteStatus(wiFiRouter, satelliteIndex);
    if (!HAPPlatformWiFiRouterSatelliteStatusIsValid(value)) {
        HAPLogError(&logObject, "%s returned invalid value: %u.", func, value);
        HAPFatalError();
    }
    return value;
}

#endif
