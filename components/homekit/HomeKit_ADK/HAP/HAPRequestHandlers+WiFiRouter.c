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

#include "HAP+API.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_ROUTER)

#include "HAPAccessoryServer+Internal.h"
#include "HAPBitSet.h"
#include "HAPCharacteristic.h"
#include "HAPCharacteristicTypes+TLV.h"
#include "HAPCharacteristicTypes.h"
#include "HAPCheckedPlatformWiFiRouter.h"
#include "HAPLog+Attributes.h"
#include "HAPLogSubsystem.h"
#include "HAPServiceTypes.h"
#include "HAPWiFiRouter.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "RequestHandlers" };

typedef struct {
    HAPAccessoryServer* server;
    const HAPTLV8CharacteristicReadRequest* request;
    HAPPlatformWiFiRouterIdentifier identifier;
} SequenceDataSource;
HAP_STATIC_ASSERT(sizeof(SequenceDataSource) <= sizeof(HAPSequenceTLVDataSource), SequenceDataSource);

typedef struct {
    HAPAccessoryServer* server;
    const HAPTLV8CharacteristicWriteRequest* _Nullable request;
    HAPPlatformWiFiRouterIdentifier identifier;
    HAPError err;
} SequenceEnumerateContext;

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
static HAPCharacteristicValue_WiFiRouter_OperationStatus OperationStatusForError(HAPError error) {
    switch (error) {
        case kHAPError_None:
            return kHAPCharacteristicValue_WiFiRouter_OperationStatus_Success;
        case kHAPError_Unknown:
            return kHAPCharacteristicValue_WiFiRouter_OperationStatus_Unknown;
        case kHAPError_InvalidState:
            return kHAPCharacteristicValue_WiFiRouter_OperationStatus_InvalidIdentifier;
        case kHAPError_InvalidData:
            return kHAPCharacteristicValue_WiFiRouter_OperationStatus_InvalidParameters;
        case kHAPError_OutOfResources:
            return kHAPCharacteristicValue_WiFiRouter_OperationStatus_OutOfResources;
        case kHAPError_NotAuthorized:
        case kHAPError_Busy:
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
static HAPCharacteristicValue_WiFiRouter_OperationStatus OperationStatusForCredentialError(HAPError error) {
    switch (error) {
        case kHAPError_None:
            return kHAPCharacteristicValue_WiFiRouter_OperationStatus_Success;
        case kHAPError_Unknown:
            return kHAPCharacteristicValue_WiFiRouter_OperationStatus_Unknown;
        case kHAPError_InvalidState:
            return kHAPCharacteristicValue_WiFiRouter_OperationStatus_InvalidIdentifier;
        case kHAPError_InvalidData:
            return kHAPCharacteristicValue_WiFiRouter_OperationStatus_InvalidParameters;
        case kHAPError_OutOfResources:
            return kHAPCharacteristicValue_WiFiRouter_OperationStatus_OutOfResources;
        case kHAPError_NotAuthorized:
            return kHAPCharacteristicValue_WiFiRouter_OperationStatus_Duplicate;
        case kHAPError_Busy:
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
static HAPCharacteristicValue_WiFiRouter_OperationStatus OperationStatusForFirewallError(HAPError error) {
    switch (error) {
        case kHAPError_None:
            return kHAPCharacteristicValue_WiFiRouter_OperationStatus_Success;
        case kHAPError_Unknown:
            return kHAPCharacteristicValue_WiFiRouter_OperationStatus_Unknown;
        case kHAPError_InvalidState:
            return kHAPCharacteristicValue_WiFiRouter_OperationStatus_InvalidIdentifier;
        case kHAPError_InvalidData:
            return kHAPCharacteristicValue_WiFiRouter_OperationStatus_InvalidFirewallRule;
        case kHAPError_OutOfResources:
            return kHAPCharacteristicValue_WiFiRouter_OperationStatus_OutOfResources;
        case kHAPError_NotAuthorized:
        case kHAPError_Busy:
        default:
            HAPFatalError();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HAP_RESULT_USE_CHECK
HAPError HAPHandleWiFiRouterVersionRead(
        HAPAccessoryServer* server,
        const HAPStringCharacteristicReadRequest* request,
        char* value,
        size_t maxValueBytes,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.ip.wiFiRouter);
    HAPPlatformWiFiRouterRef wiFiRouter HAP_UNUSED = HAPNonnull(server->platform.ip.wiFiRouter);
    HAPPrecondition(request);
    HAPPrecondition(HAPUUIDAreEqual(request->characteristic->characteristicType, &kHAPCharacteristicType_Version));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_WiFiRouter));
    HAPPrecondition(request->accessory->aid == 1);
    HAPPrecondition(value);

    const char* stringToCopy = kHAPWiFiRouter_Version;
    size_t numBytes = HAPStringGetNumBytes(stringToCopy);
    if (numBytes >= maxValueBytes) {
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Not enough space (needed: %zu, available: %zu).",
                numBytes + 1,
                maxValueBytes);
        return kHAPError_OutOfResources;
    }
    HAPRawBufferCopyBytes(value, stringToCopy, numBytes);
    value[numBytes] = '\0';
    return kHAPError_None;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct {
    HAPCharacteristicValueCallback_WiFiRouter_WANFirewall_RuleList callback;
    void* _Nullable callbackContext;
} EnumerateWANFirewallRulesContext;

static void EnumerateWANFirewallRulesCallback(
        void* _Nullable context_,
        const HAPPlatformWiFiRouterWANFirewallRule* firewallRule_,
        bool* shouldContinue) {
    HAPPrecondition(context_);
    EnumerateWANFirewallRulesContext* context = context_;
    HAPPrecondition(context->callback);
    HAPPrecondition(firewallRule_);
    HAPPrecondition(shouldContinue);
    HAPPrecondition(*shouldContinue);

    HAPCharacteristicValue_WiFiRouter_WANFirewall_Rule value;
    HAPRawBufferZero(&value, sizeof value);
    switch (*(const HAPPlatformWiFiRouterWANFirewallRuleType*) firewallRule_) {
        case kHAPPlatformWiFiRouterWANFirewallRuleType_Port: {
            const HAPPlatformWiFiRouterPortWANFirewallRule* firewallRule = firewallRule_;
            value.type = kHAPCharacteristicValueType_WiFiRouter_WANFirewall_Rule_Port;
            HAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule* val = &value._.port;
            switch (firewallRule->transportProtocol) {
                case kHAPPlatformWiFiRouterTransportProtocol_TCP: {
                    val->transportProtocol = kHAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule_Protocol_TCP;
                    break;
                }
                case kHAPPlatformWiFiRouterTransportProtocol_UDP: {
                    val->transportProtocol = kHAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule_Protocol_UDP;
                    break;
                }
                default:
                    HAPFatalError();
            }
            switch (firewallRule->host.type) {
                case kHAPPlatformWiFiRouterWANHostURIType_Any: {
                    HAPRawBufferCopyBytes(
                            &val->hostIPStart.ipv4Address,
                            &kHAPIPAddress_IPv4Any._.ipv4,
                            sizeof val->hostIPStart.ipv4Address);
                    val->hostIPStart.ipv4AddressIsSet = true;
                    HAPRawBufferCopyBytes(
                            &val->hostIPStart.ipv6Address,
                            &kHAPIPAddress_IPv6Any._.ipv6,
                            sizeof val->hostIPStart.ipv6Address);
                    val->hostIPStart.ipv6AddressIsSet = true;
                    val->hostIPStartIsSet = true;
                    break;
                }
                case kHAPPlatformWiFiRouterWANHostURIType_DNSNamePattern: {
                    val->hostDNSName = firewallRule->host._.dnsNamePattern;
                    val->hostDNSNameIsSet = true;
                    break;
                }
                case kHAPPlatformWiFiRouterWANHostURIType_IPAddress: {
                    switch (firewallRule->host._.ipAddress.version) {
                        case kHAPIPAddressVersion_IPv4: {
                            HAPRawBufferCopyBytes(
                                    &val->hostIPStart.ipv4Address,
                                    &firewallRule->host._.ipAddress._.ipv4,
                                    sizeof val->hostIPStart.ipv4Address);
                            val->hostIPStart.ipv4AddressIsSet = true;
                            val->hostIPStartIsSet = true;
                            break;
                        }
                        case kHAPIPAddressVersion_IPv6: {
                            HAPRawBufferCopyBytes(
                                    &val->hostIPStart.ipv6Address,
                                    &firewallRule->host._.ipAddress._.ipv6,
                                    sizeof val->hostIPStart.ipv6Address);
                            val->hostIPStart.ipv6AddressIsSet = true;
                            val->hostIPStartIsSet = true;
                            break;
                        }
                    }
                    break;
                }
                case kHAPPlatformWiFiRouterWANHostURIType_IPAddressRange: {
                    switch (firewallRule->host._.ipAddressRange.version) {
                        case kHAPIPAddressVersion_IPv4: {
                            HAPRawBufferCopyBytes(
                                    &val->hostIPStart.ipv4Address,
                                    &firewallRule->host._.ipAddressRange._.ipv4.startAddress,
                                    sizeof val->hostIPStart.ipv4Address);
                            val->hostIPStart.ipv4AddressIsSet = true;
                            val->hostIPStartIsSet = true;

                            if (!HAPIPv4AddressAreEqual(
                                        &firewallRule->host._.ipAddressRange._.ipv4.endAddress,
                                        &firewallRule->host._.ipAddressRange._.ipv4.startAddress)) {
                                HAPRawBufferCopyBytes(
                                        &val->hostIPEnd.ipv4Address,
                                        &firewallRule->host._.ipAddressRange._.ipv4.endAddress,
                                        sizeof val->hostIPEnd.ipv4Address);
                                val->hostIPEnd.ipv4AddressIsSet = true;
                                val->hostIPEndIsSet = true;
                            }
                            break;
                        }
                        case kHAPIPAddressVersion_IPv6: {
                            HAPRawBufferCopyBytes(
                                    &val->hostIPStart.ipv6Address,
                                    &firewallRule->host._.ipAddressRange._.ipv6.startAddress,
                                    sizeof val->hostIPStart.ipv6Address);
                            val->hostIPStart.ipv6AddressIsSet = true;
                            val->hostIPStartIsSet = true;

                            if (!HAPIPv6AddressAreEqual(
                                        &firewallRule->host._.ipAddressRange._.ipv6.endAddress,
                                        &firewallRule->host._.ipAddressRange._.ipv6.startAddress)) {
                                HAPRawBufferCopyBytes(
                                        &val->hostIPEnd.ipv6Address,
                                        &firewallRule->host._.ipAddressRange._.ipv6.endAddress,
                                        sizeof val->hostIPEnd.ipv6Address);
                                val->hostIPEnd.ipv6AddressIsSet = true;
                                val->hostIPEndIsSet = true;
                            }
                            break;
                        }
                        default:
                            HAPFatalError();
                    }
                }
            }
            val->hostPortStart = firewallRule->hostPortRange.startPort;
            if (firewallRule->hostPortRange.endPort != firewallRule->hostPortRange.startPort) {
                val->hostPortEnd = firewallRule->hostPortRange.endPort;
                val->hostPortEndIsSet = true;
            }
            break;
        }
        case kHAPPlatformWiFiRouterWANFirewallRuleType_ICMP: {
            const HAPPlatformWiFiRouterICMPWANFirewallRule* firewallRule = firewallRule_;
            value.type = kHAPCharacteristicValueType_WiFiRouter_WANFirewall_Rule_ICMP;
            HAPCharacteristicValue_WiFiRouter_WANFirewall_ICMPRule* val = &value._.icmp;
            switch (firewallRule->host.type) {
                case kHAPPlatformWiFiRouterWANHostURIType_Any: {
                    HAPRawBufferCopyBytes(
                            &val->hostIPStart.ipv4Address,
                            &kHAPIPAddress_IPv4Any._.ipv4,
                            sizeof val->hostIPStart.ipv4Address);
                    val->hostIPStart.ipv4AddressIsSet = true;
                    HAPRawBufferCopyBytes(
                            &val->hostIPStart.ipv6Address,
                            &kHAPIPAddress_IPv6Any._.ipv6,
                            sizeof val->hostIPStart.ipv6Address);
                    val->hostIPStart.ipv6AddressIsSet = true;
                    val->hostIPStartIsSet = true;
                    break;
                }
                case kHAPPlatformWiFiRouterWANHostURIType_DNSNamePattern: {
                    val->hostDNSName = firewallRule->host._.dnsNamePattern;
                    val->hostDNSNameIsSet = true;
                    break;
                }
                case kHAPPlatformWiFiRouterWANHostURIType_IPAddress: {
                    HAPRawBufferCopyBytes(&val->hostIPStart, &firewallRule->host._.ipAddress, sizeof val->hostIPStart);
                    val->hostIPStartIsSet = true;
                    break;
                }
                case kHAPPlatformWiFiRouterWANHostURIType_IPAddressRange: {
                    switch (firewallRule->host._.ipAddressRange.version) {
                        case kHAPIPAddressVersion_IPv4: {
                            HAPRawBufferCopyBytes(
                                    &val->hostIPStart.ipv4Address,
                                    &firewallRule->host._.ipAddressRange._.ipv4.startAddress,
                                    sizeof val->hostIPStart.ipv4Address);
                            val->hostIPStart.ipv4AddressIsSet = true;
                            val->hostIPStartIsSet = true;

                            if (!HAPIPv4AddressAreEqual(
                                        &firewallRule->host._.ipAddressRange._.ipv4.endAddress,
                                        &firewallRule->host._.ipAddressRange._.ipv4.startAddress)) {
                                HAPRawBufferCopyBytes(
                                        &val->hostIPEnd.ipv4Address,
                                        &firewallRule->host._.ipAddressRange._.ipv4.endAddress,
                                        sizeof val->hostIPEnd.ipv4Address);
                                val->hostIPEnd.ipv4AddressIsSet = true;
                                val->hostIPEndIsSet = true;
                            }
                            break;
                        }
                        case kHAPIPAddressVersion_IPv6: {
                            HAPRawBufferCopyBytes(
                                    &val->hostIPStart.ipv6Address,
                                    &firewallRule->host._.ipAddressRange._.ipv6.startAddress,
                                    sizeof val->hostIPStart.ipv6Address);
                            val->hostIPStart.ipv6AddressIsSet = true;
                            val->hostIPStartIsSet = true;

                            if (!HAPIPv6AddressAreEqual(
                                        &firewallRule->host._.ipAddressRange._.ipv6.endAddress,
                                        &firewallRule->host._.ipAddressRange._.ipv6.startAddress)) {
                                HAPRawBufferCopyBytes(
                                        &val->hostIPEnd.ipv6Address,
                                        &firewallRule->host._.ipAddressRange._.ipv6.endAddress,
                                        sizeof val->hostIPEnd.ipv6Address);
                                val->hostIPEnd.ipv6AddressIsSet = true;
                                val->hostIPEndIsSet = true;
                            }
                            break;
                        }
                        default:
                            HAPFatalError();
                    }
                }
            }
            HAPAssert(firewallRule->numICMPTypes <= HAPArrayCount(val->icmpList.icmpTypes));
            for (size_t i = 0; i < firewallRule->numICMPTypes; i++) {
                switch (firewallRule->icmpTypes[i].icmpProtocol) {
                    case kHAPPlatformWiFiRouterICMPProtocol_ICMPv4: {
                        val->icmpList.icmpTypes[i].icmpProtocol =
                                kHAPCharacteristicValue_WiFiRouter_ICMPProtocol_ICMPv4;
                        break;
                    }
                    case kHAPPlatformWiFiRouterICMPProtocol_ICMPv6: {
                        val->icmpList.icmpTypes[i].icmpProtocol =
                                kHAPCharacteristicValue_WiFiRouter_ICMPProtocol_ICMPv6;
                        break;
                    }
                    default:
                        HAPFatalError();
                }
                if (firewallRule->icmpTypes[i].typeValueIsSet) {
                    val->icmpList.icmpTypes[i].typeValue = firewallRule->icmpTypes[i].typeValue;
                    val->icmpList.icmpTypes[i].typeValueIsSet = true;
                }
            }
            val->icmpList.numICMPTypes = firewallRule->numICMPTypes;
            break;
        }
        default:
            HAPFatalError();
    }

    context->callback(context->callbackContext, &value, shouldContinue);
}

static void ClientEnumerateWANFirewallRulesCallback(
        void* _Nullable context,
        HAPPlatformWiFiRouterRef wiFiRouter HAP_UNUSED,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier HAP_UNUSED,
        const HAPPlatformWiFiRouterWANFirewallRule* firewallRule,
        bool* shouldContinue) {
    EnumerateWANFirewallRulesCallback(context, firewallRule, shouldContinue);
}

HAP_RESULT_USE_CHECK
static HAPError ClientEnumerateWANFirewallRules(
        HAPSequenceTLVDataSource* dataSource_,
        HAPCharacteristicValueCallback_WiFiRouter_WANFirewall_RuleList callback,
        void* _Nullable context) {
    HAPPrecondition(dataSource_);
    SequenceDataSource* dataSource = (SequenceDataSource*) dataSource_;
    HAPPrecondition(dataSource->server);
    HAPAccessoryServer* server = dataSource->server;
    HAPPrecondition(server->platform.ip.wiFiRouter);
    HAPPlatformWiFiRouterRef wiFiRouter = server->platform.ip.wiFiRouter;
    HAPPrecondition(dataSource->request);
    const HAPTLV8CharacteristicReadRequest* request HAP_UNUSED = dataSource->request;
    HAPPrecondition(dataSource->identifier);
    HAPPlatformWiFiRouterClientIdentifier clientIdentifier = dataSource->identifier;
    HAPPrecondition(callback);

    HAPError err;

    EnumerateWANFirewallRulesContext enumerateContext;
    HAPRawBufferZero(&enumerateContext, sizeof enumerateContext);
    enumerateContext.callback = callback;
    enumerateContext.callbackContext = context;
    err = HAPCheckedPlatformWiFiRouterClientEnumerateWANFirewallRules(
            wiFiRouter, clientIdentifier, ClientEnumerateWANFirewallRulesCallback, &enumerateContext);
    if (err) {
        HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidState);
        return err;
    }

    return kHAPError_None;
}

static void ClientConfigureWANFirewallCallback(
        void* _Nullable context_,
        HAPCharacteristicValue_WiFiRouter_WANFirewall_Rule* value_,
        bool* shouldContinue) {
    HAPPrecondition(context_);
    SequenceEnumerateContext* context = context_;
    HAPPrecondition(context->server);
    HAPAccessoryServer* server = context->server;
    HAPPrecondition(server->platform.ip.wiFiRouter);
    HAPPlatformWiFiRouterRef wiFiRouter = HAPNonnull(server->platform.ip.wiFiRouter);
    HAPPrecondition(!server->ip.wiFiRouter.status);
    HAPPrecondition(context->identifier);
    HAPPlatformWiFiRouterClientIdentifier clientIdentifier = context->identifier;
    HAPPrecondition(!context->err);
    HAPPrecondition(value_);
    HAPPrecondition(shouldContinue);
    HAPPrecondition(*shouldContinue);

    HAPError err;

    switch (value_->type) {
        case kHAPCharacteristicValueType_WiFiRouter_WANFirewall_Rule_Port: {
            const HAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule* value = &value_->_.port;

            HAPPlatformWiFiRouterPortWANFirewallRule firewallRule;
            HAPRawBufferZero(&firewallRule, sizeof firewallRule);
            firewallRule.type = kHAPPlatformWiFiRouterWANFirewallRuleType_Port;
            switch (value->transportProtocol) {
                case kHAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule_Protocol_TCP: {
                    firewallRule.transportProtocol = kHAPPlatformWiFiRouterTransportProtocol_TCP;
                    break;
                }
                case kHAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule_Protocol_UDP: {
                    firewallRule.transportProtocol = kHAPPlatformWiFiRouterTransportProtocol_UDP;
                    break;
                }
                default:
                    HAPFatalError();
            }
            if (value->hostDNSNameIsSet) {
                HAPAssert(!value->hostIPStartIsSet);
                firewallRule.host.type = kHAPPlatformWiFiRouterWANHostURIType_DNSNamePattern;
                firewallRule.host._.dnsNamePattern = value->hostDNSName;
            } else {
                HAPAssert(value->hostIPStartIsSet);
                if (value->hostIPStart.ipv4AddressIsSet && value->hostIPStart.ipv6AddressIsSet) {
                    HAPAssert(HAPIPv4AddressAreEqual(&value->hostIPStart.ipv4Address, &kHAPIPAddress_IPv4Any._.ipv4));
                    HAPAssert(HAPIPv6AddressAreEqual(&value->hostIPStart.ipv6Address, &kHAPIPAddress_IPv6Any._.ipv6));
                    HAPAssert(!value->hostIPEndIsSet);
                    firewallRule.host.type = kHAPPlatformWiFiRouterWANHostURIType_Any;
                } else if (!value->hostIPEndIsSet) {
                    firewallRule.host.type = kHAPPlatformWiFiRouterWANHostURIType_IPAddress;
                    if (value->hostIPStart.ipv4AddressIsSet) {
                        firewallRule.host._.ipAddress.version = kHAPIPAddressVersion_IPv4;
                        HAPRawBufferCopyBytes(
                                &firewallRule.host._.ipAddress._.ipv4,
                                &value->hostIPStart.ipv4Address,
                                sizeof firewallRule.host._.ipAddress._.ipv4);
                    } else {
                        HAPAssert(value->hostIPStart.ipv6AddressIsSet);
                        firewallRule.host._.ipAddress.version = kHAPIPAddressVersion_IPv6;
                        HAPRawBufferCopyBytes(
                                &firewallRule.host._.ipAddress._.ipv6,
                                &value->hostIPStart.ipv6Address,
                                sizeof firewallRule.host._.ipAddress._.ipv6);
                    }
                } else {
                    firewallRule.host.type = kHAPPlatformWiFiRouterWANHostURIType_IPAddressRange;
                    HAPAssert(value->hostIPStart.ipv4AddressIsSet == value->hostIPEnd.ipv4AddressIsSet);
                    HAPAssert(value->hostIPStart.ipv6AddressIsSet == value->hostIPEnd.ipv6AddressIsSet);
                    if (value->hostIPStart.ipv4AddressIsSet) {
                        firewallRule.host._.ipAddressRange.version = kHAPIPAddressVersion_IPv4;
                        HAPRawBufferCopyBytes(
                                &firewallRule.host._.ipAddressRange._.ipv4.startAddress,
                                &value->hostIPStart.ipv4Address,
                                sizeof firewallRule.host._.ipAddressRange._.ipv4.startAddress);
                        HAPRawBufferCopyBytes(
                                &firewallRule.host._.ipAddressRange._.ipv4.endAddress,
                                &value->hostIPEnd.ipv4Address,
                                sizeof firewallRule.host._.ipAddressRange._.ipv4.endAddress);
                    } else {
                        HAPAssert(value->hostIPStart.ipv6AddressIsSet);
                        HAPRawBufferCopyBytes(
                                &firewallRule.host._.ipAddressRange._.ipv6.startAddress,
                                &value->hostIPStart.ipv6Address,
                                sizeof firewallRule.host._.ipAddressRange._.ipv6.startAddress);
                        HAPRawBufferCopyBytes(
                                &firewallRule.host._.ipAddressRange._.ipv6.endAddress,
                                &value->hostIPEnd.ipv6Address,
                                sizeof firewallRule.host._.ipAddressRange._.ipv6.endAddress);
                    }
                }
            }
            firewallRule.hostPortRange.startPort = value->hostPortStart;
            if (!value->hostPortEndIsSet) {
                firewallRule.hostPortRange.endPort = value->hostPortStart;
            } else {
                firewallRule.hostPortRange.endPort = value->hostPortEnd;
            }

            err = HAPCheckedPlatformWiFiRouterClientAddWANFirewallRule(wiFiRouter, clientIdentifier, &firewallRule);
            if (err) {
                HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidData || err == kHAPError_OutOfResources);
                context->err = err;
                *shouldContinue = false;
                return;
            }
            return;
        }
        case kHAPCharacteristicValueType_WiFiRouter_WANFirewall_Rule_ICMP: {
            const HAPCharacteristicValue_WiFiRouter_WANFirewall_ICMPRule* value = &value_->_.icmp;

            HAPPlatformWiFiRouterICMPWANFirewallRule firewallRule;
            HAPRawBufferZero(&firewallRule, sizeof firewallRule);
            firewallRule.type = kHAPPlatformWiFiRouterWANFirewallRuleType_ICMP;
            if (value->hostDNSNameIsSet) {
                HAPAssert(!value->hostIPStartIsSet);
                firewallRule.host.type = kHAPPlatformWiFiRouterWANHostURIType_DNSNamePattern;
                firewallRule.host._.dnsNamePattern = value->hostDNSName;
            } else {
                HAPAssert(value->hostIPStartIsSet);
                if (value->hostIPStart.ipv4AddressIsSet && value->hostIPStart.ipv6AddressIsSet) {
                    HAPAssert(HAPIPv4AddressAreEqual(&value->hostIPStart.ipv4Address, &kHAPIPAddress_IPv4Any._.ipv4));
                    HAPAssert(HAPIPv6AddressAreEqual(&value->hostIPStart.ipv6Address, &kHAPIPAddress_IPv6Any._.ipv6));
                    HAPAssert(!value->hostIPEndIsSet);
                    firewallRule.host.type = kHAPPlatformWiFiRouterWANHostURIType_Any;
                } else if (!value->hostIPEndIsSet) {
                    firewallRule.host.type = kHAPPlatformWiFiRouterWANHostURIType_IPAddress;
                    if (value->hostIPStart.ipv4AddressIsSet) {
                        firewallRule.host._.ipAddress.version = kHAPIPAddressVersion_IPv4;
                        HAPRawBufferCopyBytes(
                                &firewallRule.host._.ipAddress._.ipv4,
                                &value->hostIPStart.ipv4Address,
                                sizeof firewallRule.host._.ipAddress._.ipv4);
                    } else {
                        HAPAssert(value->hostIPStart.ipv6AddressIsSet);
                        firewallRule.host._.ipAddress.version = kHAPIPAddressVersion_IPv6;
                        HAPRawBufferCopyBytes(
                                &firewallRule.host._.ipAddress._.ipv6,
                                &value->hostIPStart.ipv6Address,
                                sizeof firewallRule.host._.ipAddress._.ipv6);
                    }
                } else {
                    firewallRule.host.type = kHAPPlatformWiFiRouterWANHostURIType_IPAddressRange;
                    HAPAssert(value->hostIPStart.ipv4AddressIsSet == value->hostIPEnd.ipv4AddressIsSet);
                    HAPAssert(value->hostIPStart.ipv6AddressIsSet == value->hostIPEnd.ipv6AddressIsSet);
                    if (value->hostIPStart.ipv4AddressIsSet) {
                        firewallRule.host._.ipAddressRange.version = kHAPIPAddressVersion_IPv4;
                        HAPRawBufferCopyBytes(
                                &firewallRule.host._.ipAddressRange._.ipv4.startAddress,
                                &value->hostIPStart.ipv4Address,
                                sizeof firewallRule.host._.ipAddressRange._.ipv4.startAddress);
                        HAPRawBufferCopyBytes(
                                &firewallRule.host._.ipAddressRange._.ipv4.endAddress,
                                &value->hostIPEnd.ipv4Address,
                                sizeof firewallRule.host._.ipAddressRange._.ipv4.endAddress);
                    } else {
                        HAPAssert(value->hostIPStart.ipv6AddressIsSet);
                        HAPRawBufferCopyBytes(
                                &firewallRule.host._.ipAddressRange._.ipv6.startAddress,
                                &value->hostIPStart.ipv6Address,
                                sizeof firewallRule.host._.ipAddressRange._.ipv6.startAddress);
                        HAPRawBufferCopyBytes(
                                &firewallRule.host._.ipAddressRange._.ipv6.endAddress,
                                &value->hostIPEnd.ipv6Address,
                                sizeof firewallRule.host._.ipAddressRange._.ipv6.endAddress);
                    }
                }
            }
            HAPAssert(value->icmpList.numICMPTypes <= HAPArrayCount(firewallRule.icmpTypes));
            for (size_t i = 0; i < value->icmpList.numICMPTypes; i++) {
                switch (value->icmpList.icmpTypes[i].icmpProtocol) {
                    case kHAPCharacteristicValue_WiFiRouter_ICMPProtocol_ICMPv4: {
                        firewallRule.icmpTypes[i].icmpProtocol = kHAPPlatformWiFiRouterICMPProtocol_ICMPv4;
                        break;
                    }
                    case kHAPCharacteristicValue_WiFiRouter_ICMPProtocol_ICMPv6: {
                        firewallRule.icmpTypes[i].icmpProtocol = kHAPPlatformWiFiRouterICMPProtocol_ICMPv6;
                        break;
                    }
                    default:
                        HAPFatalError();
                }
                if (value->icmpList.icmpTypes[i].typeValueIsSet) {
                    firewallRule.icmpTypes[i].typeValue = value->icmpList.icmpTypes[i].typeValue;
                    firewallRule.icmpTypes[i].typeValueIsSet = true;
                }
            }
            firewallRule.numICMPTypes = value->icmpList.numICMPTypes;

            err = HAPCheckedPlatformWiFiRouterClientAddWANFirewallRule(wiFiRouter, clientIdentifier, &firewallRule);
            if (err) {
                HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidData || err == kHAPError_OutOfResources);
                context->err = err;
                *shouldContinue = false;
                return;
            }
            return;
        }
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
static HAPError ClientConfigureWANFirewall(
        HAPAccessoryServer* server,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPCharacteristicValue_WiFiRouter_WANFirewall* wanFirewall) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.ip.wiFiRouter);
    HAPPlatformWiFiRouterRef wiFiRouter = HAPNonnull(server->platform.ip.wiFiRouter);
    HAPPrecondition(!server->ip.wiFiRouter.status);
    HAPPrecondition(clientIdentifier);
    HAPPrecondition(wanFirewall);

    HAPError err;

    switch (wanFirewall->type) {
        case kHAPCharacteristicValue_WiFiRouter_FirewallType_FullAccess: {
            err = HAPCheckedPlatformWiFiRouterClientResetWANFirewall(
                    wiFiRouter, clientIdentifier, kHAPPlatformWiFiRouterFirewallType_FullAccess);
            if (err) {
                HAPAssert(
                        err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                        err == kHAPError_OutOfResources);
                return err;
            }
        }
            return kHAPError_None;
        case kHAPCharacteristicValue_WiFiRouter_FirewallType_Allowlist: {
            HAPAssert(wanFirewall->ruleListIsSet);

            err = HAPCheckedPlatformWiFiRouterClientResetWANFirewall(
                    wiFiRouter, clientIdentifier, kHAPPlatformWiFiRouterFirewallType_Allowlist);
            if (err) {
                HAPAssert(
                        err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                        err == kHAPError_OutOfResources);
                return err;
            }

            SequenceEnumerateContext enumerateContext;
            HAPRawBufferZero(&enumerateContext, sizeof enumerateContext);
            enumerateContext.server = server;
            enumerateContext.identifier = clientIdentifier;
            err = wanFirewall->ruleList.enumerate(
                    &wanFirewall->ruleList.dataSource, ClientConfigureWANFirewallCallback, &enumerateContext);
            if (err) {
                HAPAssert(err == kHAPError_InvalidData);
                return err;
            }
            err = enumerateContext.err;
            if (err) {
                HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidData || err == kHAPError_OutOfResources);
                return err;
            }

            err = HAPCheckedPlatformWiFiRouterClientFinalizeWANFirewallRules(wiFiRouter, clientIdentifier);
            if (err) {
                HAPAssert(err == kHAPError_Unknown || err == kHAPError_OutOfResources);
                return err;
            }
        }
            return kHAPError_None;
        default:
            HAPFatalError();
    }
}

//----------------------------------------------------------------------------------------------------------------------

typedef struct {
    HAPCharacteristicValueCallback_WiFiRouter_LANFirewall_RuleList callback;
    void* _Nullable callbackContext;
} EnumerateLANFirewallRulesContext;

static void EnumerateLANFirewallRulesCallback(
        void* _Nullable context_,
        const HAPPlatformWiFiRouterLANFirewallRule* firewallRule_,
        bool* shouldContinue) {
    HAPPrecondition(context_);
    EnumerateLANFirewallRulesContext* context = context_;
    HAPPrecondition(context->callback);
    HAPPrecondition(firewallRule_);
    HAPPrecondition(shouldContinue);
    HAPPrecondition(*shouldContinue);

    HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule value;
    HAPRawBufferZero(&value, sizeof value);
    switch (*(const HAPPlatformWiFiRouterLANFirewallRuleType*) firewallRule_) {
        case kHAPPlatformWiFiRouterLANFirewallRuleType_MulticastBridging: {
            const HAPPlatformWiFiRouterMulticastBridgingLANFirewallRule* firewallRule = firewallRule_;
            value.type = kHAPCharacteristicValueType_WiFiRouter_LANFirewall_Rule_MulticastBridging;
            HAPCharacteristicValue_WiFiRouter_LANFirewall_MulticastBridgingRule* val = &value._.multicastBridging;
            switch (firewallRule->direction) {
                case kHAPPlatformWiFiRouterFirewallRuleDirection_Outbound: {
                    val->direction = kHAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction_Outbound;
                    break;
                }
                case kHAPPlatformWiFiRouterFirewallRuleDirection_Inbound: {
                    val->direction = kHAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction_Inbound;
                    break;
                }
                default:
                    HAPFatalError();
            }
            HAPAssert(firewallRule->numPeerGroupIdentifiers <= HAPArrayCount(val->endpointList.groupIdentifiers));
            for (size_t i = 0; i < firewallRule->numPeerGroupIdentifiers; i++) {
                val->endpointList.groupIdentifiers[i] = firewallRule->peerGroupIdentifiers[i];
            }
            val->endpointList.numGroupIdentifiers = firewallRule->numPeerGroupIdentifiers;
            switch (firewallRule->destinationIP.version) {
                case kHAPIPAddressVersion_IPv4: {
                    HAPRawBufferCopyBytes(
                            &val->ipAddress.ipv4Address,
                            &firewallRule->destinationIP._.ipv4,
                            sizeof val->ipAddress.ipv4Address);
                    val->ipAddress.ipv4AddressIsSet = true;
                    break;
                }
                case kHAPIPAddressVersion_IPv6: {
                    HAPRawBufferCopyBytes(
                            &val->ipAddress.ipv6Address,
                            &firewallRule->destinationIP._.ipv6,
                            sizeof val->ipAddress.ipv6Address);
                    val->ipAddress.ipv6AddressIsSet = true;
                    break;
                }
                default:
                    HAPFatalError();
            }
            val->port = firewallRule->destinationPort;
            break;
        }
        case kHAPPlatformWiFiRouterLANFirewallRuleType_StaticPort: {
            const HAPPlatformWiFiRouterStaticPortLANFirewallRule* firewallRule = firewallRule_;
            value.type = kHAPCharacteristicValueType_WiFiRouter_LANFirewall_Rule_StaticPort;
            HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule* val = &value._.staticPort;
            switch (firewallRule->direction) {
                case kHAPPlatformWiFiRouterFirewallRuleDirection_Outbound: {
                    val->direction = kHAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction_Outbound;
                    break;
                }
                case kHAPPlatformWiFiRouterFirewallRuleDirection_Inbound: {
                    val->direction = kHAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction_Inbound;
                    break;
                }
                default:
                    HAPFatalError();
            }
            switch (firewallRule->transportProtocol) {
                case kHAPPlatformWiFiRouterTransportProtocol_TCP: {
                    val->transportProtocol = kHAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule_Protocol_TCP;
                    break;
                }
                case kHAPPlatformWiFiRouterTransportProtocol_UDP: {
                    val->transportProtocol = kHAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule_Protocol_UDP;
                    break;
                }
                default:
                    HAPFatalError();
            }
            HAPAssert(firewallRule->numPeerGroupIdentifiers <= HAPArrayCount(val->endpointList.groupIdentifiers));
            for (size_t i = 0; i < firewallRule->numPeerGroupIdentifiers; i++) {
                val->endpointList.groupIdentifiers[i] = firewallRule->peerGroupIdentifiers[i];
            }
            val->endpointList.numGroupIdentifiers = firewallRule->numPeerGroupIdentifiers;
            val->portStart = firewallRule->destinationPortRange.startPort;
            if (firewallRule->destinationPortRange.endPort != firewallRule->destinationPortRange.startPort) {
                val->portEnd = firewallRule->destinationPortRange.endPort;
                val->portEndIsSet = true;
            }
            break;
        }
        case kHAPPlatformWiFiRouterLANFirewallRuleType_DynamicPort: {
            const HAPPlatformWiFiRouterDynamicPortLANFirewallRule* firewallRule = firewallRule_;
            value.type = kHAPCharacteristicValueType_WiFiRouter_LANFirewall_Rule_DynamicPort;
            HAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule* val = &value._.dynamicPort;
            switch (firewallRule->direction) {
                case kHAPPlatformWiFiRouterFirewallRuleDirection_Outbound: {
                    val->direction = kHAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction_Outbound;
                    break;
                }
                case kHAPPlatformWiFiRouterFirewallRuleDirection_Inbound: {
                    val->direction = kHAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction_Inbound;
                    break;
                }
                default:
                    HAPFatalError();
            }
            switch (firewallRule->transportProtocol) {
                case kHAPPlatformWiFiRouterTransportProtocol_TCP: {
                    val->transportProtocol =
                            kHAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule_Protocol_TCP;
                    break;
                }
                case kHAPPlatformWiFiRouterTransportProtocol_UDP: {
                    val->transportProtocol =
                            kHAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule_Protocol_UDP;
                    break;
                }
                default:
                    HAPFatalError();
            }
            HAPAssert(firewallRule->numPeerGroupIdentifiers <= HAPArrayCount(val->endpointList.groupIdentifiers));
            for (size_t i = 0; i < firewallRule->numPeerGroupIdentifiers; i++) {
                val->endpointList.groupIdentifiers[i] = firewallRule->peerGroupIdentifiers[i];
            }
            val->endpointList.numGroupIdentifiers = firewallRule->numPeerGroupIdentifiers;
            switch (firewallRule->serviceType.advertisementProtocol) {
                case kHAPPlatformWiFiRouterDynamicPortAdvertisementProtocol_DNSSD: {
                    val->advertProtocol = kHAPCharacteristicValue_WiFiRouter_AdvertisementProtocol_DNSSD;
                    if (firewallRule->serviceType._.dns_sd.serviceType) {
                        val->service.name = HAPNonnull(firewallRule->serviceType._.dns_sd.serviceType);
                        val->serviceIsSet = true;
                    }
                    break;
                }
                case kHAPPlatformWiFiRouterDynamicPortAdvertisementProtocol_SSDP: {
                    val->advertProtocol = kHAPCharacteristicValue_WiFiRouter_AdvertisementProtocol_SSDP;
                    if (firewallRule->serviceType._.ssdp.serviceTypeURI) {
                        val->service.name = HAPNonnull(firewallRule->serviceType._.ssdp.serviceTypeURI);
                        val->serviceIsSet = true;
                    }
                    break;
                }
                default:
                    HAPFatalError();
            }
            if (firewallRule->advertisementOnly) {
                HAPAssert(sizeof val->flags == sizeof(uint32_t));
                val->flags |= (uint32_t)
                        kHAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule_Flags_AdvertisementOnly;
            }
            break;
        }
        case kHAPPlatformWiFiRouterLANFirewallRuleType_StaticICMP: {
            const HAPPlatformWiFiRouterStaticICMPLANFirewallRule* firewallRule = firewallRule_;
            value.type = kHAPCharacteristicValueType_WiFiRouter_LANFirewall_Rule_StaticICMP;
            HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticICMPRule* val = &value._.staticICMP;
            switch (firewallRule->direction) {
                case kHAPPlatformWiFiRouterFirewallRuleDirection_Outbound: {
                    val->direction = kHAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction_Outbound;
                    break;
                }
                case kHAPPlatformWiFiRouterFirewallRuleDirection_Inbound: {
                    val->direction = kHAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction_Inbound;
                    break;
                }
                default:
                    HAPFatalError();
            }
            HAPAssert(firewallRule->numPeerGroupIdentifiers <= HAPArrayCount(val->endpointList.groupIdentifiers));
            for (size_t i = 0; i < firewallRule->numPeerGroupIdentifiers; i++) {
                val->endpointList.groupIdentifiers[i] = firewallRule->peerGroupIdentifiers[i];
            }
            val->endpointList.numGroupIdentifiers = firewallRule->numPeerGroupIdentifiers;
            HAPAssert(firewallRule->numICMPTypes <= HAPArrayCount(val->icmpList.icmpTypes));
            for (size_t i = 0; i < firewallRule->numICMPTypes; i++) {
                switch (firewallRule->icmpTypes[i].icmpProtocol) {
                    case kHAPPlatformWiFiRouterICMPProtocol_ICMPv4: {
                        val->icmpList.icmpTypes[i].icmpProtocol =
                                kHAPCharacteristicValue_WiFiRouter_ICMPProtocol_ICMPv4;
                        break;
                    }
                    case kHAPPlatformWiFiRouterICMPProtocol_ICMPv6: {
                        val->icmpList.icmpTypes[i].icmpProtocol =
                                kHAPCharacteristicValue_WiFiRouter_ICMPProtocol_ICMPv6;
                        break;
                    }
                    default:
                        HAPFatalError();
                }
                if (firewallRule->icmpTypes[i].typeValueIsSet) {
                    val->icmpList.icmpTypes[i].typeValue = firewallRule->icmpTypes[i].typeValue;
                    val->icmpList.icmpTypes[i].typeValueIsSet = true;
                }
            }
            val->icmpList.numICMPTypes = firewallRule->numICMPTypes;
            break;
        }
        default:
            HAPFatalError();
    }

    context->callback(context->callbackContext, &value, shouldContinue);
}

static void ClientEnumerateLANFirewallRulesCallback(
        void* _Nullable context,
        HAPPlatformWiFiRouterRef wiFiRouter HAP_UNUSED,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier HAP_UNUSED,
        const HAPPlatformWiFiRouterLANFirewallRule* firewallRule,
        bool* shouldContinue) {
    EnumerateLANFirewallRulesCallback(context, firewallRule, shouldContinue);
}

HAP_RESULT_USE_CHECK
static HAPError ClientEnumerateLANFirewallRules(
        HAPSequenceTLVDataSource* dataSource_,
        HAPCharacteristicValueCallback_WiFiRouter_LANFirewall_RuleList callback,
        void* _Nullable context) {
    HAPPrecondition(dataSource_);
    SequenceDataSource* dataSource = (SequenceDataSource*) dataSource_;
    HAPPrecondition(dataSource->server);
    HAPAccessoryServer* server = dataSource->server;
    HAPPrecondition(server->platform.ip.wiFiRouter);
    HAPPlatformWiFiRouterRef wiFiRouter = server->platform.ip.wiFiRouter;
    HAPPrecondition(dataSource->request);
    const HAPTLV8CharacteristicReadRequest* request HAP_UNUSED = dataSource->request;
    HAPPrecondition(dataSource->identifier);
    HAPPlatformWiFiRouterClientIdentifier clientIdentifier = dataSource->identifier;
    HAPPrecondition(callback);

    HAPError err;

    EnumerateLANFirewallRulesContext enumerateContext;
    HAPRawBufferZero(&enumerateContext, sizeof enumerateContext);
    enumerateContext.callback = callback;
    enumerateContext.callbackContext = context;

    err = HAPCheckedPlatformWiFiRouterClientEnumerateLANFirewallRules(
            wiFiRouter, clientIdentifier, ClientEnumerateLANFirewallRulesCallback, &enumerateContext);
    if (err) {
        HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidState);
        return err;
    }

    return kHAPError_None;
}

static void ClientConfigureLANFirewallCallback(
        void* _Nullable context_,
        HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule* value_,
        bool* shouldContinue) {
    HAPPrecondition(context_);
    SequenceEnumerateContext* context = context_;
    HAPPrecondition(context->server);
    HAPAccessoryServer* server = context->server;
    HAPPrecondition(server->platform.ip.wiFiRouter);
    HAPPlatformWiFiRouterRef wiFiRouter = HAPNonnull(server->platform.ip.wiFiRouter);
    HAPPrecondition(!server->ip.wiFiRouter.status);
    HAPPrecondition(context->identifier);
    HAPPlatformWiFiRouterClientIdentifier clientIdentifier = context->identifier;
    HAPPrecondition(!context->err);
    HAPPrecondition(value_);
    HAPPrecondition(shouldContinue);
    HAPPrecondition(*shouldContinue);

    HAPError err;

    switch (value_->type) {
        case kHAPCharacteristicValueType_WiFiRouter_LANFirewall_Rule_MulticastBridging: {
            const HAPCharacteristicValue_WiFiRouter_LANFirewall_MulticastBridgingRule* value =
                    &value_->_.multicastBridging;

            HAPPlatformWiFiRouterMulticastBridgingLANFirewallRule firewallRule;
            HAPRawBufferZero(&firewallRule, sizeof firewallRule);
            firewallRule.type = kHAPPlatformWiFiRouterLANFirewallRuleType_MulticastBridging;
            switch (value->direction) {
                case kHAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction_Outbound: {
                    firewallRule.direction = kHAPPlatformWiFiRouterFirewallRuleDirection_Outbound;
                    break;
                }
                case kHAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction_Inbound: {
                    firewallRule.direction = kHAPPlatformWiFiRouterFirewallRuleDirection_Inbound;
                    break;
                }
                default:
                    HAPFatalError();
            }
            HAPAssert(value->endpointList.numGroupIdentifiers <= HAPArrayCount(firewallRule.peerGroupIdentifiers));
            for (size_t i = 0; i < value->endpointList.numGroupIdentifiers; i++) {
                firewallRule.peerGroupIdentifiers[i] = value->endpointList.groupIdentifiers[i];
            }
            firewallRule.numPeerGroupIdentifiers = value->endpointList.numGroupIdentifiers;
            if (value->ipAddress.ipv4AddressIsSet) {
                HAPAssert(!value->ipAddress.ipv6AddressIsSet);
                firewallRule.destinationIP.version = kHAPIPAddressVersion_IPv4;
                HAPRawBufferCopyBytes(
                        &firewallRule.destinationIP._.ipv4,
                        &value->ipAddress.ipv4Address,
                        sizeof firewallRule.destinationIP._.ipv4);
            } else {
                HAPAssert(value->ipAddress.ipv6AddressIsSet);
                firewallRule.destinationIP.version = kHAPIPAddressVersion_IPv6;
                HAPRawBufferCopyBytes(
                        &firewallRule.destinationIP._.ipv6,
                        &value->ipAddress.ipv4Address,
                        sizeof firewallRule.destinationIP._.ipv6);
            }
            firewallRule.destinationPort = value->port;

            err = HAPCheckedPlatformWiFiRouterClientAddLANFirewallRule(wiFiRouter, clientIdentifier, &firewallRule);
            if (err) {
                HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidData || err == kHAPError_OutOfResources);
                context->err = err;
                *shouldContinue = false;
                return;
            }
            return;
        }
        case kHAPCharacteristicValueType_WiFiRouter_LANFirewall_Rule_StaticPort: {
            const HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule* value = &value_->_.staticPort;

            HAPPlatformWiFiRouterStaticPortLANFirewallRule firewallRule;
            HAPRawBufferZero(&firewallRule, sizeof firewallRule);
            firewallRule.type = kHAPPlatformWiFiRouterLANFirewallRuleType_StaticPort;
            switch (value->direction) {
                case kHAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction_Outbound: {
                    firewallRule.direction = kHAPPlatformWiFiRouterFirewallRuleDirection_Outbound;
                    break;
                }
                case kHAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction_Inbound: {
                    firewallRule.direction = kHAPPlatformWiFiRouterFirewallRuleDirection_Inbound;
                    break;
                }
                default:
                    HAPFatalError();
            }
            HAPAssert(value->endpointList.numGroupIdentifiers <= HAPArrayCount(firewallRule.peerGroupIdentifiers));
            for (size_t i = 0; i < value->endpointList.numGroupIdentifiers; i++) {
                firewallRule.peerGroupIdentifiers[i] = value->endpointList.groupIdentifiers[i];
            }
            firewallRule.numPeerGroupIdentifiers = value->endpointList.numGroupIdentifiers;
            switch (value->transportProtocol) {
                case kHAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule_Protocol_TCP: {
                    firewallRule.transportProtocol = kHAPPlatformWiFiRouterTransportProtocol_TCP;
                    break;
                }
                case kHAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule_Protocol_UDP: {
                    firewallRule.transportProtocol = kHAPPlatformWiFiRouterTransportProtocol_UDP;
                    break;
                }
                default:
                    HAPFatalError();
            }
            firewallRule.destinationPortRange.startPort = value->portStart;
            firewallRule.destinationPortRange.endPort = value->portStart;
            if (value->portEndIsSet) {
                firewallRule.destinationPortRange.endPort = value->portEnd;
            }

            err = HAPCheckedPlatformWiFiRouterClientAddLANFirewallRule(wiFiRouter, clientIdentifier, &firewallRule);
            if (err) {
                HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidData || err == kHAPError_OutOfResources);
                context->err = err;
                *shouldContinue = false;
                return;
            }
            return;
        }
        case kHAPCharacteristicValueType_WiFiRouter_LANFirewall_Rule_DynamicPort: {
            const HAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule* value = &value_->_.dynamicPort;

            HAPPlatformWiFiRouterDynamicPortLANFirewallRule firewallRule;
            HAPRawBufferZero(&firewallRule, sizeof firewallRule);
            firewallRule.type = kHAPPlatformWiFiRouterLANFirewallRuleType_DynamicPort;
            switch (value->direction) {
                case kHAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction_Outbound: {
                    firewallRule.direction = kHAPPlatformWiFiRouterFirewallRuleDirection_Outbound;
                    break;
                }
                case kHAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction_Inbound: {
                    firewallRule.direction = kHAPPlatformWiFiRouterFirewallRuleDirection_Inbound;
                    break;
                }
                default:
                    HAPFatalError();
            }
            HAPAssert(value->endpointList.numGroupIdentifiers <= HAPArrayCount(firewallRule.peerGroupIdentifiers));
            for (size_t i = 0; i < value->endpointList.numGroupIdentifiers; i++) {
                firewallRule.peerGroupIdentifiers[i] = value->endpointList.groupIdentifiers[i];
            }
            firewallRule.numPeerGroupIdentifiers = value->endpointList.numGroupIdentifiers;
            switch (value->transportProtocol) {
                case kHAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule_Protocol_TCP: {
                    firewallRule.transportProtocol = kHAPPlatformWiFiRouterTransportProtocol_TCP;
                    break;
                }
                case kHAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule_Protocol_UDP: {
                    firewallRule.transportProtocol = kHAPPlatformWiFiRouterTransportProtocol_UDP;
                    break;
                }
                default:
                    HAPFatalError();
            }
            switch (value->advertProtocol) {
                case kHAPCharacteristicValue_WiFiRouter_AdvertisementProtocol_DNSSD: {
                    firewallRule.serviceType.advertisementProtocol =
                            kHAPPlatformWiFiRouterDynamicPortAdvertisementProtocol_DNSSD;
                    break;
                }
                case kHAPCharacteristicValue_WiFiRouter_AdvertisementProtocol_SSDP: {
                    firewallRule.serviceType.advertisementProtocol =
                            kHAPPlatformWiFiRouterDynamicPortAdvertisementProtocol_SSDP;
                    break;
                }
                default:
                    HAPFatalError();
            }
            HAPAssert(sizeof value->flags == sizeof(uint32_t));
            if (value->flags &
                (uint32_t) kHAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule_Flags_AdvertisementOnly) {
                firewallRule.advertisementOnly = true;
            }
            if (value->serviceIsSet) {
                switch (value->advertProtocol) {
                    case kHAPCharacteristicValue_WiFiRouter_AdvertisementProtocol_DNSSD: {
                        firewallRule.serviceType._.dns_sd.serviceType = value->service.name;
                        break;
                    }
                    case kHAPCharacteristicValue_WiFiRouter_AdvertisementProtocol_SSDP: {
                        firewallRule.serviceType._.ssdp.serviceTypeURI = value->service.name;
                        break;
                    }
                    default:
                        HAPFatalError();
                }
            }

            err = HAPCheckedPlatformWiFiRouterClientAddLANFirewallRule(wiFiRouter, clientIdentifier, &firewallRule);
            if (err) {
                HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidData || err == kHAPError_OutOfResources);
                context->err = err;
                *shouldContinue = false;
                return;
            }
            return;
        }
        case kHAPCharacteristicValueType_WiFiRouter_LANFirewall_Rule_StaticICMP: {
            const HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticICMPRule* value = &value_->_.staticICMP;

            HAPPlatformWiFiRouterStaticICMPLANFirewallRule firewallRule;
            HAPRawBufferZero(&firewallRule, sizeof firewallRule);
            firewallRule.type = kHAPPlatformWiFiRouterLANFirewallRuleType_StaticICMP;
            switch (value->direction) {
                case kHAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction_Outbound: {
                    firewallRule.direction = kHAPPlatformWiFiRouterFirewallRuleDirection_Outbound;
                    break;
                }
                case kHAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction_Inbound: {
                    firewallRule.direction = kHAPPlatformWiFiRouterFirewallRuleDirection_Inbound;
                    break;
                }
                default:
                    HAPFatalError();
            }
            HAPAssert(value->endpointList.numGroupIdentifiers <= HAPArrayCount(firewallRule.peerGroupIdentifiers));
            for (size_t i = 0; i < value->endpointList.numGroupIdentifiers; i++) {
                firewallRule.peerGroupIdentifiers[i] = value->endpointList.groupIdentifiers[i];
            }
            firewallRule.numPeerGroupIdentifiers = value->endpointList.numGroupIdentifiers;
            HAPAssert(value->icmpList.numICMPTypes <= HAPArrayCount(firewallRule.icmpTypes));
            for (size_t i = 0; i < value->icmpList.numICMPTypes; i++) {
                switch (value->icmpList.icmpTypes[i].icmpProtocol) {
                    case kHAPCharacteristicValue_WiFiRouter_ICMPProtocol_ICMPv4: {
                        firewallRule.icmpTypes[i].icmpProtocol = kHAPPlatformWiFiRouterICMPProtocol_ICMPv4;
                        break;
                    }
                    case kHAPCharacteristicValue_WiFiRouter_ICMPProtocol_ICMPv6: {
                        firewallRule.icmpTypes[i].icmpProtocol = kHAPPlatformWiFiRouterICMPProtocol_ICMPv6;
                        break;
                    }
                    default:
                        HAPFatalError();
                }
                if (value->icmpList.icmpTypes[i].typeValueIsSet) {
                    firewallRule.icmpTypes[i].typeValue = value->icmpList.icmpTypes[i].typeValue;
                    firewallRule.icmpTypes[i].typeValueIsSet = true;
                }
            }
            firewallRule.numICMPTypes = value->icmpList.numICMPTypes;

            err = HAPCheckedPlatformWiFiRouterClientAddLANFirewallRule(wiFiRouter, clientIdentifier, &firewallRule);
            if (err) {
                HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidData || err == kHAPError_OutOfResources);
                context->err = err;
                *shouldContinue = false;
                return;
            }
            return;
        }
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
static HAPError ClientConfigureLANFirewall(
        HAPAccessoryServer* server,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPCharacteristicValue_WiFiRouter_LANFirewall* lanFirewall) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.ip.wiFiRouter);
    HAPPlatformWiFiRouterRef wiFiRouter = HAPNonnull(server->platform.ip.wiFiRouter);
    HAPPrecondition(!server->ip.wiFiRouter.status);
    HAPPrecondition(clientIdentifier);
    HAPPrecondition(lanFirewall);

    HAPError err;

    switch (lanFirewall->type) {
        case kHAPCharacteristicValue_WiFiRouter_FirewallType_FullAccess: {
            err = HAPCheckedPlatformWiFiRouterClientResetLANFirewall(
                    wiFiRouter, clientIdentifier, kHAPPlatformWiFiRouterFirewallType_FullAccess);
            if (err) {
                HAPAssert(
                        err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                        err == kHAPError_OutOfResources);
                return err;
            }
        }
            return kHAPError_None;
        case kHAPCharacteristicValue_WiFiRouter_FirewallType_Allowlist: {
            HAPAssert(lanFirewall->ruleListIsSet);

            err = HAPCheckedPlatformWiFiRouterClientResetLANFirewall(
                    wiFiRouter, clientIdentifier, kHAPPlatformWiFiRouterFirewallType_Allowlist);
            if (err) {
                HAPAssert(
                        err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                        err == kHAPError_OutOfResources);
                return err;
            }

            SequenceEnumerateContext enumerateContext;
            HAPRawBufferZero(&enumerateContext, sizeof enumerateContext);
            enumerateContext.server = server;
            enumerateContext.identifier = clientIdentifier;
            err = lanFirewall->ruleList.enumerate(
                    &lanFirewall->ruleList.dataSource, ClientConfigureLANFirewallCallback, &enumerateContext);
            if (err) {
                HAPAssert(err == kHAPError_InvalidData);
                return err;
            }
            err = enumerateContext.err;
            if (err) {
                HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidData || err == kHAPError_OutOfResources);
                return err;
            }

            err = HAPCheckedPlatformWiFiRouterClientFinalizeLANFirewallRules(wiFiRouter, clientIdentifier);
            if (err) {
                HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidData || err == kHAPError_OutOfResources);
                return err;
            }
        }
            return kHAPError_None;
        default:
            HAPFatalError();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct {
    HAPCharacteristicValueCallback_NetworkClientProfileControl_Response callback;
    void* _Nullable callbackContext;
    HAPError err;
} EnumerateClientOperationResponsesContext;

static void EnumerateClientsCallback(
        void* _Nullable context_,
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        bool* shouldContinue) {
    HAPPrecondition(context_);
    EnumerateClientOperationResponsesContext* context = context_;
    HAPPrecondition(context->callback);
    HAPPrecondition(!context->err);
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(clientIdentifier);
    HAPPrecondition(shouldContinue);
    HAPPrecondition(*shouldContinue);

    HAPCharacteristicValue_NetworkClientProfileControl_OperationRsp value;
    HAPRawBufferZero(&value, sizeof value);

    value.status = kHAPCharacteristicValue_WiFiRouter_OperationStatus_Success;
    value.config.clientIdentifier = clientIdentifier;
    value.config.clientIsSet = true;
    value.configIsSet = true;

    context->callback(context->callbackContext, &value, shouldContinue);
}

HAP_RESULT_USE_CHECK
static HAPError EnumerateClientOperationResponses(
        HAPSequenceTLVDataSource* dataSource_,
        HAPCharacteristicValueCallback_NetworkClientProfileControl_Response callback,
        void* _Nullable context) {
    HAPPrecondition(dataSource_);
    SequenceDataSource* dataSource = (SequenceDataSource*) dataSource_;
    HAPPrecondition(dataSource->server);
    HAPAccessoryServer* server = dataSource->server;
    HAPPrecondition(server->platform.ip.wiFiRouter);
    HAPPlatformWiFiRouterRef wiFiRouter = server->platform.ip.wiFiRouter;
    HAPPrecondition(dataSource->request);
    const HAPTLV8CharacteristicReadRequest* request = dataSource->request;
    HAPPrecondition(callback);

    HAPError err;

    switch (server->ip.wiFiRouter.operation) {
        case kHAPWiFiRouterOperation_Undefined: {
        }
            return kHAPError_None;
        case kHAPWiFiRouterOperation_Enumerate: {
            EnumerateClientOperationResponsesContext enumerateContext;
            HAPRawBufferZero(&enumerateContext, sizeof enumerateContext);
            enumerateContext.callback = callback;
            enumerateContext.callbackContext = context;
            err = HAPCheckedPlatformWiFiRouterEnumerateClients(wiFiRouter, EnumerateClientsCallback, &enumerateContext);
            if (!err) {
                err = enumerateContext.err;
            }
            if (err) {
                HAPAssert(err == kHAPError_Unknown);
                return err;
            }
        }
            return kHAPError_None;
        case kHAPWiFiRouterOperation_Query: {
            bool shouldContinue = true;
            for (size_t i = 0; shouldContinue && i < server->ip.wiFiRouter.numIdentifiers; i++) {
                HAPPlatformWiFiRouterClientIdentifier clientIdentifier = server->ip.wiFiRouter._.identifiers[i];
                HAPAssert(clientIdentifier);

                HAPCharacteristicValue_NetworkClientProfileControl_OperationRsp value;
                HAPRawBufferZero(&value, sizeof value);

                bool exists;
                err = HAPCheckedPlatformWiFiRouterClientExists(wiFiRouter, clientIdentifier, &exists);
                if (err) {
                    HAPAssert(err == kHAPError_Unknown);
                    value.status = OperationStatusForError(err);
                    callback(context, &value, &shouldContinue);
                    continue;
                }
                if (!exists) {
                    HAPLogCharacteristic(
                            &logObject,
                            request->characteristic,
                            request->service,
                            request->accessory,
                            "Network Client Profile Identifier %lu not found.",
                            (unsigned long) clientIdentifier);
                    value.status = OperationStatusForError(kHAPError_InvalidState);
                    callback(context, &value, &shouldContinue);
                    continue;
                }
                value.config.clientIdentifier = clientIdentifier;
                value.config.clientIsSet = true;

                HAPPlatformWiFiRouterGroupIdentifier groupIdentifier;
                err = HAPCheckedPlatformWiFiRouterClientGetGroupIdentifier(
                        wiFiRouter, clientIdentifier, &groupIdentifier);
                if (err) {
                    HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidState);
                    value.status = OperationStatusForError(err);
                    callback(context, &value, &shouldContinue);
                    continue;
                }
                value.config.groupIdentifier = groupIdentifier;
                value.config.groupIsSet = true;

                HAPPlatformWiFiRouterCredentialType credentialType;
                err = HAPCheckedPlatformWiFiRouterClientGetCredentialType(
                        wiFiRouter, clientIdentifier, &credentialType);
                if (err) {
                    HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidState);
                    value.status = OperationStatusForError(err);
                    callback(context, &value, &shouldContinue);
                    continue;
                }
                switch (credentialType) {
                    case kHAPPlatformWiFiRouterCredentialType_MACAddress: {
                        value.config.credential.type = kHAPCharacteristicValueType_WiFiRouter_Credential_MACAddress;

                        HAPMACAddress credential;
                        err = HAPCheckedPlatformWiFiRouterClientGetMACAddressCredential(
                                wiFiRouter, clientIdentifier, &credential);
                        if (err) {
                            HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidState);
                            value.status = OperationStatusForError(err);
                            callback(context, &value, &shouldContinue);
                            continue;
                        }
                        HAPRawBufferCopyBytes(&value.config.credential._.macAddress, &credential, sizeof credential);
                        break;
                    }
                    case kHAPPlatformWiFiRouterCredentialType_PSK: {
                        value.config.credential.type = kHAPCharacteristicValueType_WiFiRouter_Credential_PSK;
                        value.config.credential._.psk = "";
                        break;
                    }
                    default:
                        HAPFatalError();
                }
                value.config.credentialIsSet = true;

                HAPPlatformWiFiRouterFirewallType wanFirewallType;
                err = HAPCheckedPlatformWiFiRouterClientGetWANFirewallType(
                        wiFiRouter, clientIdentifier, &wanFirewallType);
                if (err) {
                    HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidState);
                    value.status = OperationStatusForError(err);
                    callback(context, &value, &shouldContinue);
                    continue;
                }
                switch (wanFirewallType) {
                    case kHAPPlatformWiFiRouterFirewallType_FullAccess: {
                        value.config.wanFirewall.type = kHAPCharacteristicValue_WiFiRouter_FirewallType_FullAccess;
                        break;
                    }
                    case kHAPPlatformWiFiRouterFirewallType_Allowlist: {
                        value.config.wanFirewall.type = kHAPCharacteristicValue_WiFiRouter_FirewallType_Allowlist;
                        SequenceDataSource* source =
                                (SequenceDataSource*) &value.config.wanFirewall.ruleList.dataSource;
                        source->server = server;
                        source->request = request;
                        source->identifier = clientIdentifier;
                        value.config.wanFirewall.ruleList.enumerate = ClientEnumerateWANFirewallRules;
                        value.config.wanFirewall.ruleListIsSet = true;
                        break;
                    }
                    default:
                        HAPFatalError();
                }
                value.config.wanFirewallIsSet = true;

                HAPPlatformWiFiRouterFirewallType lanFirewallType;
                err = HAPCheckedPlatformWiFiRouterClientGetLANFirewallType(
                        wiFiRouter, clientIdentifier, &lanFirewallType);
                if (err) {
                    HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidState);
                    value.status = OperationStatusForError(err);
                    callback(context, &value, &shouldContinue);
                    continue;
                }
                switch (lanFirewallType) {
                    case kHAPPlatformWiFiRouterFirewallType_FullAccess: {
                        value.config.lanFirewall.type = kHAPCharacteristicValue_WiFiRouter_FirewallType_FullAccess;
                        break;
                    }
                    case kHAPPlatformWiFiRouterFirewallType_Allowlist: {
                        value.config.lanFirewall.type = kHAPCharacteristicValue_WiFiRouter_FirewallType_Allowlist;
                        SequenceDataSource* source =
                                (SequenceDataSource*) &value.config.lanFirewall.ruleList.dataSource;
                        source->server = server;
                        source->request = request;
                        source->identifier = clientIdentifier;
                        value.config.lanFirewall.ruleList.enumerate = ClientEnumerateLANFirewallRules;
                        value.config.lanFirewall.ruleListIsSet = true;
                        break;
                    }
                    default:
                        HAPFatalError();
                }
                value.config.lanFirewallIsSet = true;

                value.status = kHAPCharacteristicValue_WiFiRouter_OperationStatus_Success;
                value.configIsSet = true;

                callback(context, &value, &shouldContinue);
            }
        }
            return kHAPError_None;
        case kHAPWiFiRouterOperation_Modify: {
            bool shouldContinue = true;
            for (uint8_t i = 0; shouldContinue && i < server->ip.wiFiRouter.numIdentifiers; i++) {
                HAPPlatformWiFiRouterClientIdentifier clientIdentifier = server->ip.wiFiRouter._.identifiers[i];

                HAPCharacteristicValue_NetworkClientProfileControl_OperationRsp value;
                HAPRawBufferZero(&value, sizeof value);

                if (server->ip.wiFiRouter.status) {
                    if (i == server->ip.wiFiRouter.failedOperationIndex) {
                        value.status = server->ip.wiFiRouter.status;
                    } else {
                        value.status = kHAPCharacteristicValue_WiFiRouter_OperationStatus_BulkOperationFailed;
                    }
                } else {
                    HAPAssert(clientIdentifier);
                    value.status = kHAPCharacteristicValue_WiFiRouter_OperationStatus_Success;

                    // Add:    The Network Client Profile Configuration in the response contains just
                    //         the Network Client Profile Identifier for the newly added configuration.
                    // Remove: The response does not include the Network Client Profile Configuration.
                    // Update: The response does not include the Network Client Profile Configuration.
                    // See HomeKit Accessory Protocol Specification R17
                    // Section 11.142 Network Client Profile Control
                    if (HAPBitSetContains(
                                server->ip.wiFiRouter.isAddOperation, sizeof server->ip.wiFiRouter.isAddOperation, i)) {
                        value.config.clientIdentifier = clientIdentifier;
                        value.config.clientIsSet = true;
                        value.configIsSet = true;
                    }
                }

                callback(context, &value, &shouldContinue);
            }
        }
            return kHAPError_None;
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleWiFiRouterNetworkClientProfileControlRead(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.ip.wiFiRouter);
    HAPPlatformWiFiRouterRef wiFiRouter = HAPNonnull(server->platform.ip.wiFiRouter);
    HAPPrecondition(request);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_NetworkClientProfileControl));
    HAPPrecondition(request->characteristic->properties.ip.supportsWriteResponse);
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_WiFiRouter));
    HAPPrecondition(request->accessory->aid == 1);
    HAPPrecondition(responseWriter);

    HAPError err;

    // Operation is always set when a write request was received.
    // Since the characteristic has supportsWriteResponse set, the read handler is always invoked after a write request.
    // Therefore, when operation is not set we can assume that read handler was called due to a RaiseEvent.
    if (!server->ip.wiFiRouter.operation) {
        HAPCharacteristicValue_NetworkClientProfileControl_Event value;
        HAPRawBufferZero(&value, sizeof value);

        HAPWiFiRouterGetEventClientList(server, request, &value.clientList, &value.clientListIsSet);
        err = HAPTLVWriterEncode(
                responseWriter, &kHAPCharacteristicTLVFormat_NetworkClientProfileControl_Event, &value);
        if (err) {
            HAPAssert(
                    err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                    err == kHAPError_Busy);

            // Retry sending HAP event.
            HAPAccessoryServerRaiseEventOnSession(
                    server,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    HAPNonnull(request->session));
            return err;
        }
        HAPWiFiRouterHandleClientEventSent(server, request);
        return kHAPError_None;
    }

    HAPCharacteristicValue_NetworkClientProfileControl_Response value;
    HAPRawBufferZero(&value, sizeof value);
    SequenceDataSource* dataSource = (SequenceDataSource*) &value.dataSource;

    dataSource->server = server;
    dataSource->request = request;
    value.enumerate = EnumerateClientOperationResponses;
    err = HAPTLVWriterEncode(responseWriter, &kHAPCharacteristicTLVFormat_NetworkClientProfileControl_Response, &value);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                err == kHAPError_Busy);
        goto cleanup;
    }
    if (server->ip.wiFiRouter.status) {
        goto cleanup;
    }
    if (server->ip.wiFiRouter.operation == kHAPWiFiRouterOperation_Modify) {
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Committing network configuration change.");
        err = HAPCheckedPlatformWiFiRouterCommitConfigurationChange(wiFiRouter);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            goto cleanup;
        }

        // Record recent network client profile identifier changes for HAP events.
        for (uint8_t operationIndex = 0; operationIndex < server->ip.wiFiRouter.numIdentifiers; operationIndex++) {
            HAPPlatformWiFiRouterClientIdentifier clientIdentifier =
                    server->ip.wiFiRouter._.identifiers[operationIndex];
            HAPAssert(clientIdentifier);
            HAPWiFiRouterRaiseClientEvent(
                    server, request->characteristic, clientIdentifier, HAPNonnull(request->session));
        }
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }

cleanup : {
    if (err || server->ip.wiFiRouter.status) {
        if (server->ip.wiFiRouter.isChangingConfiguration) {
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "Rolling back network configuration change.");
            HAPCheckedPlatformWiFiRouterRollbackConfigurationChange(wiFiRouter);
            server->ip.wiFiRouter.isChangingConfiguration = false;
        }
    }
    if (server->ip.wiFiRouter.hasExclusiveConfigurationAccess) {
        HAPCheckedPlatformWiFiRouterReleaseExclusiveConfigurationAccess(wiFiRouter);
        server->ip.wiFiRouter.hasExclusiveConfigurationAccess = false;
    }
    if (server->ip.wiFiRouter.hasSharedConfigurationAccess) {
        HAPCheckedPlatformWiFiRouterReleaseSharedConfigurationAccess(wiFiRouter);
        server->ip.wiFiRouter.hasSharedConfigurationAccess = false;
    }
    HAPRawBufferZero(&server->ip.wiFiRouter, sizeof server->ip.wiFiRouter);
}
    return err;
}

//----------------------------------------------------------------------------------------------------------------------

static void EnumerateClientOperationsCallback(
        void* _Nullable context_,
        HAPCharacteristicValue_NetworkClientProfileControl_Operation* value,
        bool* shouldContinue) {
    HAPPrecondition(context_);
    SequenceEnumerateContext* context = context_;
    HAPPrecondition(context->server);
    HAPAccessoryServer* server = context->server;
    HAPPrecondition(server->platform.ip.wiFiRouter);
    HAPPlatformWiFiRouterRef wiFiRouter = HAPNonnull(server->platform.ip.wiFiRouter);
    HAPPrecondition(context->request);
    const HAPTLV8CharacteristicWriteRequest* request = context->request;
    HAPPrecondition(!context->err);
    HAPPrecondition(value);
    HAPPrecondition(shouldContinue);
    HAPPrecondition(*shouldContinue);

    HAPError err;

    switch (value->operationType) {
        case kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_List: {
            // For the list operation, no other operations can be included in the bulk operation.
            // See HomeKit Accessory Protocol Specification R17
            // Section 11.142 Network Client Profile Control
            if (server->ip.wiFiRouter.operation) {
                HAPLogCharacteristic(
                        &logObject,
                        request->characteristic,
                        request->service,
                        request->accessory,
                        "Cannot mix List operation with other operations.");
                context->err = kHAPError_InvalidData;
                *shouldContinue = false;
                return;
            }
            server->ip.wiFiRouter.operation = kHAPWiFiRouterOperation_Enumerate;
            return;
        }
        case kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_Read: {
            // For read operations, only read operations can be included in the bulk operation.
            // See HomeKit Accessory Protocol Specification R17
            // Section 11.142 Network Client Profile Control
            if (!server->ip.wiFiRouter.operation) {
                server->ip.wiFiRouter.operation = kHAPWiFiRouterOperation_Query;
            }
            if (server->ip.wiFiRouter.operation != kHAPWiFiRouterOperation_Query) {
                HAPLogCharacteristic(
                        &logObject,
                        request->characteristic,
                        request->service,
                        request->accessory,
                        "Cannot mix Read operation with other operations.");
                context->err = kHAPError_InvalidData;
                *shouldContinue = false;
                return;
            }
            if (server->ip.wiFiRouter.numIdentifiers >= HAPArrayCount(server->ip.wiFiRouter._.identifiers)) {
                HAPLogCharacteristic(
                        &logObject,
                        request->characteristic,
                        request->service,
                        request->accessory,
                        "Cannot process more than %zu bulk operations.",
                        HAPArrayCount(server->ip.wiFiRouter._.identifiers));
                context->err = kHAPError_OutOfResources;
                *shouldContinue = false;
                return;
            }
            uint8_t operationIndex = server->ip.wiFiRouter.numIdentifiers++;
            server->ip.wiFiRouter._.identifiers[operationIndex] = value->config.clientIdentifier;
            return;
        }
        case kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_Add:
        case kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_Remove:
        case kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_Update: {
            // Add, update, and remove operations can be combined into a single bulk operation provided the
            // operations do not modify the same network client profile configuration (e.g., same Network Client
            // Profile Identifier).
            // See HomeKit Accessory Protocol Specification R17
            // Section 11.142 Network Client Profile Control
            if (!server->ip.wiFiRouter.operation) {
                HAPLogCharacteristic(
                        &logObject,
                        request->characteristic,
                        request->service,
                        request->accessory,
                        "Beginning network configuration change.");

                HAPAssert(!server->ip.wiFiRouter.hasExclusiveConfigurationAccess);
                err = HAPCheckedPlatformWiFiRouterAcquireExclusiveConfigurationAccess(wiFiRouter);
                if (err) {
                    HAPAssert(err == kHAPError_Busy);
                    context->err = err;
                    *shouldContinue = false;
                    return;
                }
                server->ip.wiFiRouter.hasExclusiveConfigurationAccess = true;

                HAPAssert(!server->ip.wiFiRouter.isChangingConfiguration);
                err = HAPCheckedPlatformWiFiRouterBeginConfigurationChange(wiFiRouter);
                if (err) {
                    HAPAssert(err == kHAPError_Unknown);
                    context->err = err;
                    *shouldContinue = false;
                    return;
                }
                server->ip.wiFiRouter.isChangingConfiguration = true;

                server->ip.wiFiRouter.operation = kHAPWiFiRouterOperation_Modify;
            }
            if (server->ip.wiFiRouter.operation != kHAPWiFiRouterOperation_Modify) {
                HAPLogCharacteristic(
                        &logObject,
                        request->characteristic,
                        request->service,
                        request->accessory,
                        "Cannot mix Add/Update/Remove operation with other operations.");
                context->err = kHAPError_InvalidData;
                *shouldContinue = false;
                return;
            }
            if (server->ip.wiFiRouter.numIdentifiers >= HAPArrayCount(server->ip.wiFiRouter._.identifiers)) {
                HAPLogCharacteristic(
                        &logObject,
                        request->characteristic,
                        request->service,
                        request->accessory,
                        "Cannot process more than %zu bulk operations.",
                        HAPArrayCount(server->ip.wiFiRouter._.identifiers));
                context->err = kHAPError_OutOfResources;
                *shouldContinue = false;
                return;
            }
            uint8_t operationIndex = server->ip.wiFiRouter.numIdentifiers++;
            server->ip.wiFiRouter._.identifiers[operationIndex] = 0;

            if (server->ip.wiFiRouter.status) {
                return;
            }

            // Transform credential into platform type.
            HAPPlatformWiFiRouterClientCredential credential;
            HAPRawBufferZero(&credential, sizeof credential);
            if (value->config.credentialIsSet) {
                switch (value->config.credential.type) {
                    case kHAPCharacteristicValueType_WiFiRouter_Credential_MACAddress: {
                        credential.type = kHAPPlatformWiFiRouterCredentialType_MACAddress;
                        HAPRawBufferCopyBytes(
                                &credential._.macAddress,
                                &value->config.credential._.macAddress,
                                sizeof credential._.macAddress);
                        break;
                    }
                    case kHAPCharacteristicValueType_WiFiRouter_Credential_PSK: {
                        credential.type = kHAPPlatformWiFiRouterCredentialType_PSK;
                        size_t numPSKCredentialBytes = HAPStringGetNumBytes(value->config.credential._.psk);
                        if (numPSKCredentialBytes == 2 * kHAPWiFiWPAPSK_NumBytes) {
                            credential._.psk.type = kHAPWiFiWPAPersonalCredentialType_PSK;
                            for (size_t j = 0; j < kHAPWiFiWPAPSK_NumBytes; j++) {
                                uint8_t configCredentialOffset0;
                                uint8_t configCredentialOffset1;
                                err = HAPUInt8FromHexDigit(
                                        value->config.credential._.psk[2 * j + 0], &configCredentialOffset0);
                                if (err != kHAPError_None) {
                                    HAPFatalError();
                                }
                                err = HAPUInt8FromHexDigit(
                                        value->config.credential._.psk[2 * j + 1], &configCredentialOffset1);
                                if (err != kHAPError_None) {
                                    HAPFatalError();
                                }

                                credential._.psk._.psk.bytes[j] = (uint8_t)(configCredentialOffset0 << 4U) |
                                                                  (uint8_t)(configCredentialOffset1 << 0U);
                            }
                        } else {
                            credential._.psk.type = kHAPWiFiWPAPersonalCredentialType_Passphrase;
                            HAPAssert(numPSKCredentialBytes < sizeof credential._.psk._.passphrase.stringValue);
                            HAPRawBufferCopyBytes(
                                    credential._.psk._.passphrase.stringValue,
                                    value->config.credential._.psk,
                                    numPSKCredentialBytes + 1);
                        }
                        break;
                    }
                    default:
                        HAPFatalError();
                }
                HAPAssert(credential.type);
            }

            switch (value->operationType) {
                case kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_Add: {
                    HAPAssert(!value->config.clientIsSet);

                    HAPPlatformWiFiRouterClientIdentifier clientIdentifier;

                    HAPAssert(value->config.groupIsSet);
                    HAPAssert(value->config.credentialIsSet);
                    err = HAPCheckedPlatformWiFiRouterAddClient(
                            wiFiRouter, value->config.groupIdentifier, &credential, &clientIdentifier);
                    if (err) {
                        HAPAssert(
                                err == kHAPError_Unknown || err == kHAPError_InvalidData ||
                                err == kHAPError_OutOfResources || err == kHAPError_NotAuthorized);
                        server->ip.wiFiRouter.failedOperationIndex = operationIndex;
                        server->ip.wiFiRouter.status = OperationStatusForCredentialError(err);
                        clientIdentifier = 0;
                        return;
                    }
                    server->ip.wiFiRouter._.identifiers[operationIndex] = clientIdentifier;
                    HAPAssert(!HAPBitSetContains(
                            server->ip.wiFiRouter.isAddOperation,
                            sizeof server->ip.wiFiRouter.isAddOperation,
                            operationIndex));
                    HAPBitSetInsert(
                            server->ip.wiFiRouter.isAddOperation,
                            sizeof server->ip.wiFiRouter.isAddOperation,
                            operationIndex);

                    HAPAssert(value->config.wanFirewallIsSet);
                    err = ClientConfigureWANFirewall(server, clientIdentifier, &value->config.wanFirewall);
                    if (err) {
                        HAPAssert(
                                err == kHAPError_Unknown || err == kHAPError_InvalidState ||
                                err == kHAPError_InvalidData || err == kHAPError_OutOfResources);
                        server->ip.wiFiRouter.failedOperationIndex = operationIndex;
                        server->ip.wiFiRouter.status = OperationStatusForError(err);
                        return;
                    }

                    HAPAssert(value->config.lanFirewallIsSet);
                    err = ClientConfigureLANFirewall(server, clientIdentifier, &value->config.lanFirewall);
                    if (err) {
                        HAPAssert(
                                err == kHAPError_Unknown || err == kHAPError_InvalidState ||
                                err == kHAPError_InvalidData || err == kHAPError_OutOfResources);
                        server->ip.wiFiRouter.failedOperationIndex = operationIndex;
                        server->ip.wiFiRouter.status = OperationStatusForError(err);
                        return;
                    }
                    break;
                }
                case kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_Remove: {
                    HAPAssert(value->config.clientIsSet);
                    HAPPlatformWiFiRouterClientIdentifier clientIdentifier = value->config.clientIdentifier;
                    server->ip.wiFiRouter._.identifiers[operationIndex] = clientIdentifier;

                    err = HAPCheckedPlatformWiFiRouterRemoveClient(wiFiRouter, clientIdentifier);
                    if (err) {
                        HAPAssert(err == kHAPError_Unknown || err == kHAPError_OutOfResources);
                        server->ip.wiFiRouter.failedOperationIndex = operationIndex;
                        server->ip.wiFiRouter.status = OperationStatusForError(err);
                        return;
                    }
                    break;
                }
                case kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_Update: {
                    HAPAssert(value->config.clientIsSet);
                    HAPPlatformWiFiRouterClientIdentifier clientIdentifier = value->config.clientIdentifier;
                    server->ip.wiFiRouter._.identifiers[operationIndex] = clientIdentifier;

                    bool exists;
                    err = HAPCheckedPlatformWiFiRouterClientExists(wiFiRouter, clientIdentifier, &exists);
                    if (err) {
                        HAPAssert(err == kHAPError_Unknown);
                        server->ip.wiFiRouter.failedOperationIndex = operationIndex;
                        server->ip.wiFiRouter.status = OperationStatusForError(err);
                        return;
                    }
                    if (!exists) {
                        HAPLogCharacteristic(
                                &logObject,
                                request->characteristic,
                                request->service,
                                request->accessory,
                                "Network Client Profile Identifier %lu not found.",
                                (unsigned long) clientIdentifier);
                        server->ip.wiFiRouter.failedOperationIndex = operationIndex;
                        server->ip.wiFiRouter.status = OperationStatusForError(kHAPError_InvalidState);
                        return;
                    }

                    if (value->config.groupIsSet) {
                        err = HAPCheckedPlatformWiFiRouterClientSetGroupIdentifier(
                                wiFiRouter, clientIdentifier, value->config.groupIdentifier);
                        if (err) {
                            HAPAssert(
                                    err == kHAPError_Unknown || err == kHAPError_InvalidState ||
                                    err == kHAPError_InvalidData || err == kHAPError_OutOfResources);
                            server->ip.wiFiRouter.failedOperationIndex = operationIndex;
                            server->ip.wiFiRouter.status = OperationStatusForError(err);
                            return;
                        }
                    }

                    if (value->config.credentialIsSet) {
                        err = HAPCheckedPlatformWiFiRouterClientSetCredential(
                                wiFiRouter, clientIdentifier, &credential);
                        if (err) {
                            HAPAssert(
                                    err == kHAPError_Unknown || err == kHAPError_InvalidState ||
                                    err == kHAPError_InvalidData || err == kHAPError_OutOfResources ||
                                    err == kHAPError_NotAuthorized);
                            server->ip.wiFiRouter.failedOperationIndex = operationIndex;
                            server->ip.wiFiRouter.status = OperationStatusForCredentialError(err);
                            return;
                        }
                    }

                    if (value->config.wanFirewallIsSet) {
                        err = ClientConfigureWANFirewall(server, clientIdentifier, &value->config.wanFirewall);
                        if (err) {
                            HAPAssert(
                                    err == kHAPError_Unknown || err == kHAPError_InvalidState ||
                                    err == kHAPError_InvalidData || err == kHAPError_OutOfResources);
                            server->ip.wiFiRouter.failedOperationIndex = operationIndex;
                            server->ip.wiFiRouter.status = OperationStatusForFirewallError(err);
                            return;
                        }
                    }

                    if (value->config.lanFirewallIsSet) {
                        err = ClientConfigureLANFirewall(server, clientIdentifier, &value->config.lanFirewall);
                        if (err) {
                            HAPAssert(
                                    err == kHAPError_Unknown || err == kHAPError_InvalidState ||
                                    err == kHAPError_InvalidData || err == kHAPError_OutOfResources);
                            server->ip.wiFiRouter.failedOperationIndex = operationIndex;
                            server->ip.wiFiRouter.status = OperationStatusForFirewallError(err);
                            return;
                        }
                    }
                    break;
                }
                case kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_List:
                case kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_Read: {
                    HAPFatalError();
                }
                default:
                    HAPFatalError();
            }
            HAPAssert(server->ip.wiFiRouter._.identifiers[operationIndex]);

            for (size_t i = 0; i < operationIndex; i++) {
                if (server->ip.wiFiRouter._.identifiers[i] == server->ip.wiFiRouter._.identifiers[operationIndex]) {
                    HAPLogCharacteristic(
                            &logObject,
                            request->characteristic,
                            request->service,
                            request->accessory,
                            "Only one bulk modification allowed per network client profile configuration.");
                    server->ip.wiFiRouter.failedOperationIndex = operationIndex;
                    server->ip.wiFiRouter.status = kHAPCharacteristicValue_WiFiRouter_OperationStatus_InvalidIdentifier;
                    return;
                }
            }
            return;
        }
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleWiFiRouterNetworkClientProfileControlWrite(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicWriteRequest* request,
        HAPTLVReader* requestReader,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.ip.wiFiRouter);
    HAPPlatformWiFiRouterRef wiFiRouter = HAPNonnull(server->platform.ip.wiFiRouter);
    HAPPrecondition(request);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_NetworkClientProfileControl));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_WiFiRouter));
    HAPPrecondition(request->accessory->aid == 1);
    HAPPrecondition(requestReader);

    HAPError err;

    HAPAssert(!server->ip.wiFiRouter.hasSharedConfigurationAccess);
    err = HAPCheckedPlatformWiFiRouterAcquireSharedConfigurationAccess(wiFiRouter);
    if (err) {
        HAPAssert(err == kHAPError_Busy);
        goto cleanup;
    }
    server->ip.wiFiRouter.hasSharedConfigurationAccess = true;

    bool isEnabled;
    err = HAPCheckedPlatformWiFiRouterIsManagedNetworkEnabled(wiFiRouter, &isEnabled);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        goto cleanup;
    }
    if (!isEnabled) {
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Characteristic not accessible because managed network is disabled.");
        err = kHAPError_InvalidState;
        goto cleanup;
    }

    HAPCharacteristicValue_NetworkClientProfileControl value;
    err = HAPTLVReaderDecode(requestReader, &kHAPCharacteristicTLVFormat_NetworkClientProfileControl, &value);
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        goto cleanup;
    }

    HAPAssert(!server->ip.wiFiRouter.operation);
    SequenceEnumerateContext enumerateContext;
    HAPRawBufferZero(&enumerateContext, sizeof enumerateContext);
    enumerateContext.server = server;
    enumerateContext.request = request;
    err = value.enumerate(&value.dataSource, EnumerateClientOperationsCallback, &enumerateContext);
    if (!err) {
        err = enumerateContext.err;
    }
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                err == kHAPError_OutOfResources || err == kHAPError_Busy);
        goto cleanup;
    }
    if (!server->ip.wiFiRouter.operation) {
        HAPLogCharacteristic(
                &logObject, request->characteristic, request->service, request->accessory, "No operations in request.");
        server->ip.wiFiRouter.operation = kHAPWiFiRouterOperation_Query;
    }

cleanup : {
    if (err) {
        if (server->ip.wiFiRouter.isChangingConfiguration) {
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "Rolling back network configuration change.");
            HAPCheckedPlatformWiFiRouterRollbackConfigurationChange(wiFiRouter);
            server->ip.wiFiRouter.isChangingConfiguration = false;
        }
        if (server->ip.wiFiRouter.hasExclusiveConfigurationAccess) {
            HAPCheckedPlatformWiFiRouterReleaseExclusiveConfigurationAccess(wiFiRouter);
            server->ip.wiFiRouter.hasExclusiveConfigurationAccess = false;
        }
        if (server->ip.wiFiRouter.hasSharedConfigurationAccess) {
            HAPCheckedPlatformWiFiRouterReleaseSharedConfigurationAccess(wiFiRouter);
            server->ip.wiFiRouter.hasSharedConfigurationAccess = false;
        }
        HAPRawBufferZero(&server->ip.wiFiRouter, sizeof server->ip.wiFiRouter);
    }
}
    return err;
}

//----------------------------------------------------------------------------------------------------------------------

void HAPHandleWiFiRouterNetworkClientProfileControlSubscribe(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicSubscriptionRequest* request,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.ip.wiFiRouter);
    HAPPlatformWiFiRouterRef wiFiRouter HAP_UNUSED = HAPNonnull(server->platform.ip.wiFiRouter);
    HAPPrecondition(request);
    HAPPrecondition(request->session);
    HAPSession* session = request->session;
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_NetworkClientProfileControl));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_WiFiRouter));
    HAPPrecondition(request->accessory->aid == 1);

    HAPWiFiRouterEventState* eventState = &server->ip.wiFiRouterEventState.networkClientProfileControl;
    HAPWiFiRouterSessionState* sessionState = &session->wiFiRouterEventState.networkClientProfileControl;

    HAPWiFiRouterSubscribeForEvents(eventState, sessionState);
}

//----------------------------------------------------------------------------------------------------------------------

void HAPHandleWiFiRouterNetworkClientProfileControlUnsubscribe(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicSubscriptionRequest* request,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.ip.wiFiRouter);
    HAPPlatformWiFiRouterRef wiFiRouter HAP_UNUSED = HAPNonnull(server->platform.ip.wiFiRouter);
    HAPPrecondition(request);
    HAPPrecondition(request->session);
    HAPSession* session = request->session;
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_NetworkClientProfileControl));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_WiFiRouter));
    HAPPrecondition(request->accessory->aid == 1);

    HAPWiFiRouterEventState* eventState = &server->ip.wiFiRouterEventState.networkClientProfileControl;
    HAPWiFiRouterSessionState* sessionState = &session->wiFiRouterEventState.networkClientProfileControl;

    HAPWiFiRouterUnsubscribeFromEvents(eventState, sessionState);
}

//----------------------------------------------------------------------------------------------------------------------

typedef struct {
    const HAPIPAddress* _Nullable ipAddresses;
    size_t numIPAddresses;
} IPAddressListDataSource;
HAP_STATIC_ASSERT(sizeof(IPAddressListDataSource) <= sizeof(HAPSequenceTLVDataSource), IPAddressListDataSource);

typedef struct {
    HAPCharacteristicValueCallback_NetworkClientStatusControl_IPAddressList callback;
    void* _Nullable callbackContext;
} EnumerateIPAddressesContext;

HAP_RESULT_USE_CHECK
static HAPError EnumerateIPAddresses(
        HAPSequenceTLVDataSource* dataSource_,
        HAPCharacteristicValueCallback_NetworkClientStatusControl_IPAddressList callback,
        void* _Nullable context) {
    HAPPrecondition(dataSource_);
    IPAddressListDataSource* dataSource = (IPAddressListDataSource*) dataSource_;
    const HAPIPAddress* _Nullable ipAddresses = dataSource->ipAddresses;
    size_t numIPAddresses = dataSource->numIPAddresses;
    HAPPrecondition(callback);

    if (!ipAddresses) {
        HAPLogError(&logObject, "IP address enumeration is no longer possible.");
        return kHAPError_Unknown;
    }

    bool shouldContinue = true;
    for (size_t i = 0; shouldContinue && i < numIPAddresses; i++) {
        HAPCharacteristicValue_WiFiRouter_IPAddress value;
        HAPRawBufferZero(&value, sizeof value);
        switch (ipAddresses[i].version) {
            case kHAPIPAddressVersion_IPv4: {
                HAPRawBufferCopyBytes(&value.ipv4Address, &ipAddresses[i]._.ipv4, sizeof value.ipv4Address);
                value.ipv4AddressIsSet = true;
                break;
            }
            case kHAPIPAddressVersion_IPv6: {
                HAPRawBufferCopyBytes(&value.ipv6Address, &ipAddresses[i]._.ipv6, sizeof value.ipv6Address);
                value.ipv6AddressIsSet = true;
                break;
            }
            default:
                HAPFatalError();
        }
        callback(context, &value, &shouldContinue);
    }
    return kHAPError_None;
}

typedef struct {
    HAPCharacteristicValueCallback_NetworkClientStatusControl_Response callback;
    void* _Nullable callbackContext;
} GetClientStatusForIdentifiersContext;

static void GetClientStatusForIdentifiersCallback(
        void* _Nullable context_,
        HAPPlatformWiFiRouterRef wiFiRouter,
        const HAPPlatformWiFiRouterClientStatus* clientStatus,
        bool* shouldContinue) {
    HAPPrecondition(context_);
    GetClientStatusForIdentifiersContext* context = context_;
    HAPPrecondition(context->callback);
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(clientStatus);
    HAPPrecondition(shouldContinue);
    HAPPrecondition(*shouldContinue);

    HAPCharacteristicValue_NetworkClientStatusControl_Status value;
    HAPRawBufferZero(&value, sizeof value);

    if (clientStatus->clientIdentifier) {
        value.clientIdentifier = clientStatus->clientIdentifier;
        value.clientIsSet = true;
    }
    HAPRawBufferCopyBytes(&value.macAddress, clientStatus->macAddress, sizeof value.macAddress);
    if (clientStatus->numIPAddresses) {
        IPAddressListDataSource* dataSource = (IPAddressListDataSource*) &value.ipAddressList.dataSource;
        dataSource->ipAddresses = clientStatus->ipAddresses;
        dataSource->numIPAddresses = clientStatus->numIPAddresses;
        value.ipAddressList.enumerate = EnumerateIPAddresses;
        value.ipAddressListIsSet = true;
    }
    if (clientStatus->name) {
        value.name = HAPNonnull(clientStatus->name);
        value.nameIsSet = true;
    }
    if (clientStatus->rssi.isDefined) {
        value.rssi = clientStatus->rssi.value;
        value.rssiIsSet = true;
    }

    context->callback(context->callbackContext, &value, shouldContinue);

    // Prevent enumeration of IP address list after callback as it is no longer valid once we return.
    if (clientStatus->numIPAddresses) {
        IPAddressListDataSource* dataSource = (IPAddressListDataSource*) &value.ipAddressList.dataSource;
        HAPRawBufferZero(dataSource, sizeof *dataSource);
    }
}

HAP_RESULT_USE_CHECK
static HAPError EnumerateClientStatuses(
        HAPSequenceTLVDataSource* dataSource_,
        HAPCharacteristicValueCallback_NetworkClientStatusControl_Response callback,
        void* _Nullable context) {
    HAPPrecondition(dataSource_);
    SequenceDataSource* dataSource = (SequenceDataSource*) dataSource_;
    HAPPrecondition(dataSource->server);
    HAPAccessoryServer* server = dataSource->server;
    HAPPrecondition(server->platform.ip.wiFiRouter);
    HAPPlatformWiFiRouterRef wiFiRouter = server->platform.ip.wiFiRouter;
    HAPPrecondition(dataSource->request);
    const HAPTLV8CharacteristicReadRequest* request HAP_UNUSED = dataSource->request;
    HAPPrecondition(callback);

    HAPError err;

    switch (server->ip.wiFiRouter.operation) {
        case kHAPWiFiRouterOperation_Undefined: {
        }
            return kHAPError_None;
        case kHAPWiFiRouterOperation_Enumerate: {
        }
            HAPFatalError();
        case kHAPWiFiRouterOperation_Query: {
            GetClientStatusForIdentifiersContext enumerateContext;
            HAPRawBufferZero(&enumerateContext, sizeof enumerateContext);
            enumerateContext.callback = callback;
            enumerateContext.callbackContext = context;
            err = HAPCheckedPlatformWiFiRouterGetClientStatusForIdentifiers(
                    wiFiRouter,
                    server->ip.wiFiRouter._.statusIdentifiers,
                    server->ip.wiFiRouter.numIdentifiers,
                    GetClientStatusForIdentifiersCallback,
                    &enumerateContext);
            if (err) {
                HAPAssert(err == kHAPError_Unknown);
                return err;
            }
        }
            return kHAPError_None;
        case kHAPWiFiRouterOperation_Modify: {
        }
            HAPFatalError();
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleWiFiRouterNetworkClientStatusControlRead(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.ip.wiFiRouter);
    HAPPlatformWiFiRouterRef wiFiRouter = HAPNonnull(server->platform.ip.wiFiRouter);
    HAPPrecondition(request);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_NetworkClientStatusControl));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_WiFiRouter));
    HAPPrecondition(request->accessory->aid == 1);
    HAPPrecondition(responseWriter);

    HAPError err;

    HAPCharacteristicValue_NetworkClientStatusControl_Response value;
    HAPRawBufferZero(&value, sizeof value);

    SequenceDataSource* dataSource = (SequenceDataSource*) &value.dataSource;
    dataSource->server = server;
    dataSource->request = request;
    value.enumerate = EnumerateClientStatuses;
    err = HAPTLVWriterEncode(responseWriter, &kHAPCharacteristicTLVFormat_NetworkClientStatusControl_Response, &value);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                err == kHAPError_Busy);
        goto cleanup;
    }

cleanup : {
    HAPAssert(!server->ip.wiFiRouter.isChangingConfiguration);
    HAPAssert(!server->ip.wiFiRouter.hasExclusiveConfigurationAccess);
    if (server->ip.wiFiRouter.hasSharedConfigurationAccess) {
        HAPCheckedPlatformWiFiRouterReleaseSharedConfigurationAccess(wiFiRouter);
        server->ip.wiFiRouter.hasSharedConfigurationAccess = false;
    }
    HAPRawBufferZero(&server->ip.wiFiRouter, sizeof server->ip.wiFiRouter);
}
    return err;
}

//----------------------------------------------------------------------------------------------------------------------

static void EnumerateClientStatusIdentifiersCallback(
        void* _Nullable context_,
        HAPCharacteristicValue_NetworkClientStatusControl_Identifier* value_,
        bool* shouldContinue) {
    HAPPrecondition(context_);
    SequenceEnumerateContext* context = context_;
    HAPPrecondition(context->server);
    HAPAccessoryServer* server = context->server;
    HAPPrecondition(!server->ip.wiFiRouter.status);
    HAPPrecondition(context->request);
    const HAPTLV8CharacteristicWriteRequest* request = context->request;
    HAPPrecondition(!context->err);
    HAPPrecondition(value_);
    HAPPrecondition(shouldContinue);
    HAPPrecondition(*shouldContinue);

    if (server->ip.wiFiRouter.numIdentifiers >= HAPArrayCount(server->ip.wiFiRouter._.statusIdentifiers)) {
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Cannot process more than %zu Network Client Status Identifiers.",
                HAPArrayCount(server->ip.wiFiRouter._.statusIdentifiers));
        context->err = kHAPError_OutOfResources;
        *shouldContinue = false;
        return;
    }

    HAPPlatformWiFiRouterClientStatusIdentifier* statusIdentifier =
            &server->ip.wiFiRouter._.statusIdentifiers[server->ip.wiFiRouter.numIdentifiers];

    switch (value_->type) {
        case kHAPCharacteristicValueType_NetworkClientStatusControl_Identifier_Client: {
            const uint32_t* value = &value_->_.clientIdentifier;

            statusIdentifier->format = kHAPPlatformWiFiRouterClientStatusIdentifierFormat_Client;
            statusIdentifier->clientIdentifier._ = *value;
            break;
        }
        case kHAPCharacteristicValueType_NetworkClientStatusControl_Identifier_MACAddress: {
            const HAPMACAddress* value = &value_->_.macAddress;

            statusIdentifier->format = kHAPPlatformWiFiRouterClientStatusIdentifierFormat_MACAddress;
            HAPRawBufferCopyBytes(&statusIdentifier->macAddress._, value, sizeof statusIdentifier->macAddress._);
            break;
        }
        case kHAPCharacteristicValueType_NetworkClientStatusControl_Identifier_IPAddress: {
            const HAPCharacteristicValue_WiFiRouter_IPAddress* value = &value_->_.ipAddress;
            if (value->ipv4AddressIsSet && value->ipv6AddressIsSet) {
                HAPLog(&logObject, "IPv4/IPv6 wildcard address only allowed in context of WAN firewall rules.");
                context->err = kHAPError_InvalidData;
                *shouldContinue = false;
                return;
            }

            statusIdentifier->format = kHAPPlatformWiFiRouterClientStatusIdentifierFormat_IPAddress;
            if (value->ipv4AddressIsSet) {
                statusIdentifier->ipAddress._.version = kHAPIPAddressVersion_IPv4;
                HAPRawBufferCopyBytes(
                        &statusIdentifier->ipAddress._._.ipv4,
                        &value->ipv4Address,
                        sizeof statusIdentifier->ipAddress._._.ipv4);
            } else {
                HAPAssert(value->ipv6AddressIsSet);
                statusIdentifier->ipAddress._.version = kHAPIPAddressVersion_IPv6;
                HAPRawBufferCopyBytes(
                        &statusIdentifier->ipAddress._._.ipv6,
                        &value->ipv6Address,
                        sizeof statusIdentifier->ipAddress._._.ipv6);
            }
            break;
        }
        default:
            HAPFatalError();
    }

    server->ip.wiFiRouter.numIdentifiers++;
}

HAP_RESULT_USE_CHECK
static HAPError ProcessWiFiRouterNetworkClientStatusControlWrite(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicWriteRequest* request,
        HAPCharacteristicValue_NetworkClientStatusControl* value) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.ip.wiFiRouter);
    HAPPrecondition(request);
    HAPPrecondition(value);

    HAPError err;

    switch (value->operationType) {
        case kHAPCharacteristicValue_NetworkClientStatusControl_OperationType_Read: {
            HAPAssert(!server->ip.wiFiRouter.operation);
            server->ip.wiFiRouter.operation = kHAPWiFiRouterOperation_Query;

            SequenceEnumerateContext enumerateContext;
            HAPRawBufferZero(&enumerateContext, sizeof enumerateContext);
            enumerateContext.server = server;
            enumerateContext.request = request;
            err = value->identifierList.enumerate(
                    &value->identifierList.dataSource, EnumerateClientStatusIdentifiersCallback, &enumerateContext);
            if (!err) {
                err = enumerateContext.err;
            }
            if (err) {
                HAPAssert(err == kHAPError_InvalidData || err == kHAPError_OutOfResources);
                return err;
            }
        }
            return kHAPError_None;
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleWiFiRouterNetworkClientStatusControlWrite(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicWriteRequest* request,
        HAPTLVReader* requestReader,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.ip.wiFiRouter);
    HAPPlatformWiFiRouterRef wiFiRouter = HAPNonnull(server->platform.ip.wiFiRouter);
    HAPPrecondition(request);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_NetworkClientStatusControl));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_WiFiRouter));
    HAPPrecondition(request->accessory->aid == 1);
    HAPPrecondition(requestReader);

    HAPError err;

    HAPAssert(!server->ip.wiFiRouter.hasSharedConfigurationAccess);
    err = HAPCheckedPlatformWiFiRouterAcquireSharedConfigurationAccess(wiFiRouter);
    if (err) {
        HAPAssert(err == kHAPError_Busy);
        goto cleanup;
    }
    server->ip.wiFiRouter.hasSharedConfigurationAccess = true;

    bool isEnabled;
    err = HAPCheckedPlatformWiFiRouterIsManagedNetworkEnabled(wiFiRouter, &isEnabled);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }
    if (!isEnabled) {
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Characteristic not accessible because managed network is disabled.");
        err = kHAPError_InvalidState;
        goto cleanup;
    }

    HAPCharacteristicValue_NetworkClientStatusControl value;
    err = HAPTLVReaderDecode(requestReader, &kHAPCharacteristicTLVFormat_NetworkClientStatusControl, &value);
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        goto cleanup;
    }

    err = ProcessWiFiRouterNetworkClientStatusControlWrite(server, request, &value);
    if (err) {
        HAPAssert(err == kHAPError_InvalidData || err == kHAPError_OutOfResources);
        goto cleanup;
    }

cleanup : {
    if (err) {
        HAPAssert(!server->ip.wiFiRouter.isChangingConfiguration);
        HAPAssert(!server->ip.wiFiRouter.hasExclusiveConfigurationAccess);
        if (server->ip.wiFiRouter.hasSharedConfigurationAccess) {
            HAPCheckedPlatformWiFiRouterReleaseSharedConfigurationAccess(wiFiRouter);
            server->ip.wiFiRouter.hasSharedConfigurationAccess = false;
        }
        HAPRawBufferZero(&server->ip.wiFiRouter, sizeof server->ip.wiFiRouter);
    }
}
    return err;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPHandleWiFiRouterRouterStatusRead(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.ip.wiFiRouter);
    HAPPlatformWiFiRouterRef wiFiRouter = HAPNonnull(server->platform.ip.wiFiRouter);
    HAPPrecondition(request);
    HAPPrecondition(HAPUUIDAreEqual(request->characteristic->characteristicType, &kHAPCharacteristicType_RouterStatus));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_WiFiRouter));
    HAPPrecondition(request->accessory->aid == 1);
    HAPPrecondition(value);

    HAPError err;

    bool isReady;
    err = HAPCheckedPlatformWiFiRouterIsReady(wiFiRouter, &isReady);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogCharacteristicInfo(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "[%p] Wi-Fi router ready state could not be received.",
                (const void*) wiFiRouter);
        *value = kHAPCharacteristicValue_RouterStatus_NotReady;
    } else if (isReady) {
        HAPLogCharacteristicInfo(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "[%p] Network configuration fully deployed and operational.",
                (const void*) wiFiRouter);
        *value = kHAPCharacteristicValue_RouterStatus_Ready;
    } else {
        HAPLogCharacteristicInfo(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "[%p] Wi-Fi router is in the process of setting up the network.",
                (const void*) wiFiRouter);
        *value = kHAPCharacteristicValue_RouterStatus_NotReady;
    }

    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPHandleWiFiRouterSupportedRouterConfigurationRead(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.ip.wiFiRouter);
    HAPPlatformWiFiRouterRef wiFiRouter HAP_UNUSED = HAPNonnull(server->platform.ip.wiFiRouter);
    HAPPrecondition(request);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_SupportedRouterConfiguration));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_WiFiRouter));
    HAPPrecondition(request->accessory->aid == 1);
    HAPPrecondition(responseWriter);

    HAPError err;

    HAPCharacteristicValue_SupportedRouterConfiguration value;
    HAPRawBufferZero(&value, sizeof value);

    value.flags = kHAPCharacteristicValue_SupportedRouterConfiguration_Flags_SupportsManagedNetwork +
                  kHAPCharacteristicValue_SupportedRouterConfiguration_Flags_SupportsMACAddressCredentials +
                  kHAPCharacteristicValue_SupportedRouterConfiguration_Flags_SupportsPSKCredentials;

    err = HAPTLVWriterEncode(responseWriter, &kHAPCharacteristicTLVFormat_SupportedRouterConfiguration, &value);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                err == kHAPError_Busy);
        return err;
    }

    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

typedef struct {
    HAPCharacteristicValueCallback_WANConfigurationList callback;
    void* _Nullable callbackContext;
    HAPError err;
} EnumerateWANConfigurationsContext;

static void EnumerateWANConfigurationsCallback(
        void* _Nullable context_,
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterWANIdentifier wanIdentifier,
        bool* shouldContinue) {
    HAPPrecondition(context_);
    EnumerateWANConfigurationsContext* context = context_;
    HAPPrecondition(context->callback);
    HAPPrecondition(!context->err);
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wanIdentifier);
    HAPPrecondition(shouldContinue);
    HAPPrecondition(*shouldContinue);

    HAPError err;

    HAPCharacteristicValue_WANConfigurationList_Config value;
    HAPRawBufferZero(&value, sizeof value);

    value.wanIdentifier = wanIdentifier;

    HAPPlatformWiFiRouterWANType wanType;
    err = HAPCheckedPlatformWiFiRouterWANGetType(wiFiRouter, wanIdentifier, &wanType);
    if (err) {
        HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidState);
        context->err = err;
        *shouldContinue = false;
        return;
    }
    switch (wanType) {
        case kHAPPlatformWiFiRouterWANType_Unconfigured: {
            value.wanType = kHAPCharacteristicValue_WANConfigurationList_Config_WANType_Unconfigured;
            break;
        }
        case kHAPPlatformWiFiRouterWANType_Other: {
            value.wanType = kHAPCharacteristicValue_WANConfigurationList_Config_WANType_Other;
            break;
        }
        case kHAPPlatformWiFiRouterWANType_DHCP: {
            value.wanType = kHAPCharacteristicValue_WANConfigurationList_Config_WANType_DHCP;
            break;
        }
        case kHAPPlatformWiFiRouterWANType_BridgeMode: {
            value.wanType = kHAPCharacteristicValue_WANConfigurationList_Config_WANType_BridgeMode;
            break;
        }
        default:
            HAPFatalError();
    }

    context->callback(context->callbackContext, &value, shouldContinue);
}

HAP_RESULT_USE_CHECK
static HAPError EnumerateWANConfigurations(
        HAPSequenceTLVDataSource* dataSource_,
        HAPCharacteristicValueCallback_WANConfigurationList callback,
        void* _Nullable context) {
    HAPPrecondition(dataSource_);
    SequenceDataSource* dataSource = (SequenceDataSource*) dataSource_;
    HAPPrecondition(dataSource->server);
    HAPAccessoryServer* server = dataSource->server;
    HAPPrecondition(server->platform.ip.wiFiRouter);
    HAPPlatformWiFiRouterRef wiFiRouter = server->platform.ip.wiFiRouter;
    HAPPrecondition(dataSource->request);
    const HAPTLV8CharacteristicReadRequest* request HAP_UNUSED = dataSource->request;
    HAPPrecondition(callback);

    HAPError err;

    EnumerateWANConfigurationsContext enumerateContext;
    HAPRawBufferZero(&enumerateContext, sizeof enumerateContext);
    enumerateContext.callback = callback;
    enumerateContext.callbackContext = context;
    err = HAPCheckedPlatformWiFiRouterEnumerateWANs(wiFiRouter, EnumerateWANConfigurationsCallback, &enumerateContext);
    if (!err) {
        err = enumerateContext.err;
    }
    if (err) {
        HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidState);
        return err;
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleWiFiRouterWANConfigurationListRead(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.ip.wiFiRouter);
    HAPPlatformWiFiRouterRef wiFiRouter = HAPNonnull(server->platform.ip.wiFiRouter);
    HAPPrecondition(request);
    HAPPrecondition(
            HAPUUIDAreEqual(request->characteristic->characteristicType, &kHAPCharacteristicType_WANConfigurationList));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_WiFiRouter));
    HAPPrecondition(request->accessory->aid == 1);
    HAPPrecondition(responseWriter);

    HAPError err;

    HAPCharacteristicValue_WANConfigurationList value;
    HAPRawBufferZero(&value, sizeof value);
    SequenceDataSource* dataSource = (SequenceDataSource*) &value.dataSource;

    HAPAssert(!server->ip.wiFiRouter.hasSharedConfigurationAccess);
    err = HAPCheckedPlatformWiFiRouterAcquireSharedConfigurationAccess(wiFiRouter);
    if (err) {
        HAPAssert(err == kHAPError_Busy);
        goto cleanup;
    }
    server->ip.wiFiRouter.hasSharedConfigurationAccess = true;

    dataSource->server = server;
    dataSource->request = request;
    value.enumerate = EnumerateWANConfigurations;
    err = HAPTLVWriterEncode(responseWriter, &kHAPCharacteristicTLVFormat_WANConfigurationList, &value);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                err == kHAPError_Busy);
        goto cleanup;
    }

cleanup : {
    HAPAssert(!server->ip.wiFiRouter.isChangingConfiguration);
    HAPAssert(!server->ip.wiFiRouter.hasExclusiveConfigurationAccess);
    if (server->ip.wiFiRouter.hasSharedConfigurationAccess) {
        HAPCheckedPlatformWiFiRouterReleaseSharedConfigurationAccess(wiFiRouter);
        server->ip.wiFiRouter.hasSharedConfigurationAccess = false;
    }
    HAPRawBufferZero(&server->ip.wiFiRouter, sizeof server->ip.wiFiRouter);
}
    return err;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct {
    HAPCharacteristicValueCallback_WANStatusList callback;
    void* _Nullable callbackContext;
    HAPError err;
} EnumerateWANStatusesContext;

HAP_STATIC_ASSERT(
        (uint64_t) kHAPPlatformWiFiRouterWANStatus_Unknown ==
                (uint64_t) kHAPCharacteristicValue_WANStatusList_Status_LinkStatus_Unknown,
        kHAPPlatformWiFiRouterWANStatus_Unknown);
HAP_STATIC_ASSERT(
        (uint64_t) kHAPPlatformWiFiRouterWANStatus_NoCableConnected ==
                (uint64_t) kHAPCharacteristicValue_WANStatusList_Status_LinkStatus_NoCableConnected,
        kHAPPlatformWiFiRouterWANStatus_NoCableConnected);
HAP_STATIC_ASSERT(
        (uint64_t) kHAPPlatformWiFiRouterWANStatus_NoIPAddress ==
                (uint64_t) kHAPCharacteristicValue_WANStatusList_Status_LinkStatus_NoIPAddress,
        kHAPPlatformWiFiRouterWANStatus_NoIPAddress);
HAP_STATIC_ASSERT(
        (uint64_t) kHAPPlatformWiFiRouterWANStatus_NoGatewaySpecified ==
                (uint64_t) kHAPCharacteristicValue_WANStatusList_Status_LinkStatus_NoGatewaySpecified,
        kHAPPlatformWiFiRouterWANStatus_NoGatewaySpecified);
HAP_STATIC_ASSERT(
        (uint64_t) kHAPPlatformWiFiRouterWANStatus_GatewayUnreachable ==
                (uint64_t) kHAPCharacteristicValue_WANStatusList_Status_LinkStatus_GatewayUnreachable,
        kHAPPlatformWiFiRouterWANStatus_GatewayUnreachable);
HAP_STATIC_ASSERT(
        (uint64_t) kHAPPlatformWiFiRouterWANStatus_NoDNSServerSpecified ==
                (uint64_t) kHAPCharacteristicValue_WANStatusList_Status_LinkStatus_NoDNSServerSpecified,
        kHAPPlatformWiFiRouterWANStatus_NoDNSServerSpecified);
HAP_STATIC_ASSERT(
        (uint64_t) kHAPPlatformWiFiRouterWANStatus_DNSServerUnreachable ==
                (uint64_t) kHAPCharacteristicValue_WANStatusList_Status_LinkStatus_DNSServerUnreachable,
        kHAPPlatformWiFiRouterWANStatus_DNSServerUnreachable);
HAP_STATIC_ASSERT(
        (uint64_t) kHAPPlatformWiFiRouterWANStatus_AuthenticationFailed ==
                (uint64_t) kHAPCharacteristicValue_WANStatusList_Status_LinkStatus_AuthenticationFailed,
        kHAPPlatformWiFiRouterWANStatus_AuthenticationFailed);
HAP_STATIC_ASSERT(
        (uint64_t) kHAPPlatformWiFiRouterWANStatus_Walled ==
                (uint64_t) kHAPCharacteristicValue_WANStatusList_Status_LinkStatus_Walled,
        kHAPPlatformWiFiRouterWANStatus_Walled);

static void EnumerateWANStatusesCallback(
        void* _Nullable context_,
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterWANIdentifier wanIdentifier,
        bool* shouldContinue) {
    HAPPrecondition(context_);
    EnumerateWANStatusesContext* context = context_;
    HAPPrecondition(context->callback);
    HAPPrecondition(!context->err);
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wanIdentifier);
    HAPPrecondition(shouldContinue);
    HAPPrecondition(*shouldContinue);

    HAPError err;

    HAPCharacteristicValue_WANStatusList_Status value;
    HAPRawBufferZero(&value, sizeof value);

    value.wanIdentifier = wanIdentifier;

    HAPPlatformWiFiRouterWANStatus wanStatus;
    err = HAPCheckedPlatformWiFiRouterWANGetStatus(wiFiRouter, wanIdentifier, &wanStatus);
    if (err) {
        HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidState);
        context->err = err;
        *shouldContinue = false;
        return;
    }
    value.linkStatus = wanStatus;

    context->callback(context->callbackContext, &value, shouldContinue);
}

HAP_RESULT_USE_CHECK
static HAPError EnumerateWANStatuses(
        HAPSequenceTLVDataSource* dataSource_,
        HAPCharacteristicValueCallback_WANStatusList callback,
        void* _Nullable context) {
    HAPPrecondition(dataSource_);
    SequenceDataSource* dataSource = (SequenceDataSource*) dataSource_;
    HAPPrecondition(dataSource->server);
    HAPAccessoryServer* server = dataSource->server;
    HAPPrecondition(server->platform.ip.wiFiRouter);
    HAPPlatformWiFiRouterRef wiFiRouter = server->platform.ip.wiFiRouter;
    HAPPrecondition(dataSource->request);
    const HAPTLV8CharacteristicReadRequest* request HAP_UNUSED = dataSource->request;
    HAPPrecondition(callback);

    HAPError err;

    EnumerateWANStatusesContext enumerateContext;
    HAPRawBufferZero(&enumerateContext, sizeof enumerateContext);
    enumerateContext.callback = callback;
    enumerateContext.callbackContext = context;
    err = HAPCheckedPlatformWiFiRouterEnumerateWANs(wiFiRouter, EnumerateWANStatusesCallback, &enumerateContext);
    if (!err) {
        err = enumerateContext.err;
    }
    if (err) {
        HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidState);
        return err;
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleWiFiRouterWANStatusListRead(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.ip.wiFiRouter);
    HAPPlatformWiFiRouterRef wiFiRouter = HAPNonnull(server->platform.ip.wiFiRouter);
    HAPPrecondition(request);
    HAPPrecondition(
            HAPUUIDAreEqual(request->characteristic->characteristicType, &kHAPCharacteristicType_WANStatusList));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_WiFiRouter));
    HAPPrecondition(request->accessory->aid == 1);
    HAPPrecondition(responseWriter);

    HAPError err;

    HAPCharacteristicValue_WANStatusList value;
    HAPRawBufferZero(&value, sizeof value);
    SequenceDataSource* dataSource = (SequenceDataSource*) &value.dataSource;

    HAPAssert(!server->ip.wiFiRouter.hasSharedConfigurationAccess);
    err = HAPCheckedPlatformWiFiRouterAcquireSharedConfigurationAccess(wiFiRouter);
    if (err) {
        HAPAssert(err == kHAPError_Busy);
        goto cleanup;
    }
    server->ip.wiFiRouter.hasSharedConfigurationAccess = true;

    dataSource->server = server;
    dataSource->request = request;
    value.enumerate = EnumerateWANStatuses;
    err = HAPTLVWriterEncode(responseWriter, &kHAPCharacteristicTLVFormat_WANStatusList, &value);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                err == kHAPError_Busy);
        goto cleanup;
    }

cleanup : {
    HAPAssert(!server->ip.wiFiRouter.isChangingConfiguration);
    HAPAssert(!server->ip.wiFiRouter.hasExclusiveConfigurationAccess);
    if (server->ip.wiFiRouter.hasSharedConfigurationAccess) {
        HAPCheckedPlatformWiFiRouterReleaseSharedConfigurationAccess(wiFiRouter);
        server->ip.wiFiRouter.hasSharedConfigurationAccess = false;
    }
    HAPRawBufferZero(&server->ip.wiFiRouter, sizeof server->ip.wiFiRouter);
}
    return err;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HAP_RESULT_USE_CHECK
static HAPError ProcessWiFiRouterManagedNetworkEnableRead(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.ip.wiFiRouter);
    HAPPlatformWiFiRouterRef wiFiRouter = HAPNonnull(server->platform.ip.wiFiRouter);
    HAPPrecondition(request);
    HAPPrecondition(value);

    HAPError err;

    bool isEnabled;
    err = HAPCheckedPlatformWiFiRouterIsManagedNetworkEnabled(wiFiRouter, &isEnabled);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        *value = kHAPCharacteristicValue_ManagedNetworkEnable_Unknown;
        return kHAPError_None;
    }

    HAPLogCharacteristicInfo(
            &logObject,
            request->characteristic,
            request->service,
            request->accessory,
            "Managed Network Enable: %s",
            isEnabled ? "Enabled" : "Disabled");

    if (isEnabled) {
        *value = kHAPCharacteristicValue_ManagedNetworkEnable_Enabled;
    } else {
        *value = kHAPCharacteristicValue_ManagedNetworkEnable_Disabled;
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleWiFiRouterManagedNetworkEnableRead(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.ip.wiFiRouter);
    HAPPlatformWiFiRouterRef wiFiRouter = HAPNonnull(server->platform.ip.wiFiRouter);
    HAPPrecondition(request);
    HAPPrecondition(
            HAPUUIDAreEqual(request->characteristic->characteristicType, &kHAPCharacteristicType_ManagedNetworkEnable));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_WiFiRouter));
    HAPPrecondition(request->accessory->aid == 1);
    HAPPrecondition(value);

    HAPError err;

    HAPAssert(!server->ip.wiFiRouter.hasSharedConfigurationAccess);
    err = HAPCheckedPlatformWiFiRouterAcquireSharedConfigurationAccess(wiFiRouter);
    if (err) {
        HAPAssert(err == kHAPError_Busy);
        goto cleanup;
    }
    server->ip.wiFiRouter.hasSharedConfigurationAccess = true;

    err = ProcessWiFiRouterManagedNetworkEnableRead(server, request, value);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        goto cleanup;
    }

cleanup : {
    HAPAssert(!server->ip.wiFiRouter.isChangingConfiguration);
    HAPAssert(!server->ip.wiFiRouter.hasExclusiveConfigurationAccess);
    if (server->ip.wiFiRouter.hasSharedConfigurationAccess) {
        HAPCheckedPlatformWiFiRouterReleaseSharedConfigurationAccess(wiFiRouter);
        server->ip.wiFiRouter.hasSharedConfigurationAccess = false;
    }
    HAPRawBufferZero(&server->ip.wiFiRouter, sizeof server->ip.wiFiRouter);
}
    return err;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
static HAPError ProcessWiFiRouterManagedNetworkEnableWrite(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.ip.wiFiRouter);
    HAPPlatformWiFiRouterRef wiFiRouter = HAPNonnull(server->platform.ip.wiFiRouter);
    HAPPrecondition(request);

    HAPError err;

    bool isEnabled;
    err = HAPCheckedPlatformWiFiRouterIsManagedNetworkEnabled(wiFiRouter, &isEnabled);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    bool enable = false;
    switch ((HAPCharacteristicValue_ManagedNetworkEnable) value) {
        case kHAPCharacteristicValue_ManagedNetworkEnable_Disabled: {
            enable = false;
            break;
        }
        case kHAPCharacteristicValue_ManagedNetworkEnable_Enabled: {
            enable = true;
            break;
        }
        case kHAPCharacteristicValue_ManagedNetworkEnable_Unknown: {
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "Unknown value not allowed in writes.");
        }
            return kHAPError_InvalidData;
        default:
            HAPFatalError();
    }

    HAPLogCharacteristicInfo(
            &logObject,
            request->characteristic,
            request->service,
            request->accessory,
            "Managed Network Enable: %s",
            enable ? "Enabled" : "Disabled");

    if (enable != isEnabled) {
        err = HAPCheckedPlatformWiFiRouterSetManagedNetworkEnabled(wiFiRouter, enable);
        if (err) {
            HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidState);
            return err;
        }
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleWiFiRouterManagedNetworkEnableWrite(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.ip.wiFiRouter);
    HAPPlatformWiFiRouterRef wiFiRouter = HAPNonnull(server->platform.ip.wiFiRouter);
    HAPPrecondition(request);
    HAPPrecondition(
            HAPUUIDAreEqual(request->characteristic->characteristicType, &kHAPCharacteristicType_ManagedNetworkEnable));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_WiFiRouter));
    HAPPrecondition(request->accessory->aid == 1);

    HAPError err;

    HAPAssert(!server->ip.wiFiRouter.hasExclusiveConfigurationAccess);
    err = HAPCheckedPlatformWiFiRouterAcquireExclusiveConfigurationAccess(wiFiRouter);
    if (err) {
        HAPAssert(err == kHAPError_Busy);
        goto cleanup;
    }
    server->ip.wiFiRouter.hasExclusiveConfigurationAccess = true;

    err = ProcessWiFiRouterManagedNetworkEnableWrite(server, request, value);
    if (err) {
        HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidState);
        goto cleanup;
    }

cleanup : {
    HAPAssert(!server->ip.wiFiRouter.isChangingConfiguration);
    if (server->ip.wiFiRouter.hasExclusiveConfigurationAccess) {
        HAPCheckedPlatformWiFiRouterReleaseExclusiveConfigurationAccess(wiFiRouter);
        server->ip.wiFiRouter.hasExclusiveConfigurationAccess = false;
    }
    HAPAssert(!server->ip.wiFiRouter.hasSharedConfigurationAccess);
    HAPRawBufferZero(&server->ip.wiFiRouter, sizeof server->ip.wiFiRouter);
}
    return err;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HAP_RESULT_USE_CHECK
static size_t GetSatelliteIndex(HAPAccessoryServer* server, const HAPService* service, const HAPAccessory* accessory) {
    HAPPrecondition(server);
    HAPPrecondition(server->primaryAccessory);
    HAPPrecondition(server->ip.bridgedAccessories);
    HAPPrecondition(service);
    HAPPrecondition(HAPUUIDAreEqual(service->serviceType, &kHAPServiceType_WiFiSatellite));
    HAPPrecondition(accessory);

    bool satelliteIndexFound = false;
    size_t satelliteIndex = 0;

    const HAPAccessory* currentAccessory = server->primaryAccessory;
    if (currentAccessory->services) {
        for (size_t i = 0; currentAccessory->services[i]; i++) {
            const HAPService* currentService = currentAccessory->services[i];
            if (HAPUUIDAreEqual(currentService->serviceType, &kHAPServiceType_WiFiSatellite)) {
                HAPLogServiceError(
                        &logObject,
                        currentService,
                        currentAccessory,
                        "Satellite accessories must be published as bridged accessories.");
                HAPFatalError();
            }
        }
    }
    for (size_t i = 0; server->ip.bridgedAccessories[i]; i++) {
        currentAccessory = server->ip.bridgedAccessories[i];
        if (currentAccessory->services) {
            for (size_t j = 0; currentAccessory->services[j]; j++) {
                const HAPService* currentService = currentAccessory->services[j];
                if (HAPUUIDAreEqual(currentService->serviceType, &kHAPServiceType_WiFiSatellite)) {
                    if (currentAccessory == accessory && currentService == service) {
                        if (satelliteIndexFound) {
                            HAPLogServiceError(
                                    &logObject,
                                    service,
                                    accessory,
                                    "Multiple matching services of type %s found.",
                                    kHAPServiceDebugDescription_WiFiSatellite);
                            HAPFatalError();
                        }
                        HAPPrecondition(!satelliteIndexFound);
                        satelliteIndexFound = true;
                    } else {
                        if (!satelliteIndexFound) {
                            satelliteIndex++;
                            HAPAssert(satelliteIndex);
                        }
                    }
                }
            }
        }
    }

    if (!satelliteIndexFound) {
        HAPLogServiceError(
                &logObject,
                service,
                accessory,
                "No matching service of type %s found.",
                kHAPServiceDebugDescription_WiFiSatellite);
        HAPFatalError();
    }
    HAPPrecondition(satelliteIndexFound);
    return satelliteIndex;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleWiFiRouterWiFiSatelliteStatusRead(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.ip.wiFiRouter);
    HAPPlatformWiFiRouterRef wiFiRouter = HAPNonnull(server->platform.ip.wiFiRouter);
    HAPPrecondition(request);
    HAPPrecondition(
            HAPUUIDAreEqual(request->characteristic->characteristicType, &kHAPCharacteristicType_WiFiSatelliteStatus));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_WiFiSatellite));
    HAPPrecondition(value);

    size_t satelliteIndex = GetSatelliteIndex(server, request->service, request->accessory);

    HAPPlatformWiFiRouterSatelliteStatus status =
            HAPCheckedPlatformWiFiRouterGetSatelliteStatus(wiFiRouter, satelliteIndex);
    switch (status) {
        case kHAPPlatformWiFiRouterSatelliteStatus_Unknown: {
            HAPLogCharacteristicInfo(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "[%p] Status of Wi-Fi satellite accessory %zu: unknown.",
                    (const void*) wiFiRouter,
                    satelliteIndex);
            *value = kHAPCharacteristicValue_WiFiSatelliteStatus_Unknown;
            break;
        }
        case kHAPPlatformWiFiRouterSatelliteStatus_Connected: {
            HAPLogCharacteristicInfo(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "[%p] Status of Wi-Fi satellite accessory %zu: connected.",
                    (const void*) wiFiRouter,
                    satelliteIndex);
            *value = kHAPCharacteristicValue_WiFiSatelliteStatus_Connected;
            break;
        }
        case kHAPPlatformWiFiRouterSatelliteStatus_NotConnected: {
            HAPLogCharacteristicInfo(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "[%p] Status of Wi-Fi satellite accessory %zu: not connected.",
                    (const void*) wiFiRouter,
                    satelliteIndex);
            *value = kHAPCharacteristicValue_WiFiSatelliteStatus_NotConnected;
            break;
        }
        default:
            HAPFatalError();
    }

    return kHAPError_None;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct {
    HAPCharacteristicValueCallback_NetworkAccessViolationControl_Response callback;
    void* _Nullable callbackContext;
    HAPError err;
} EnumerateAccessViolationsContext;

static void EnumerateAccessViolationsCallback(
        void* _Nullable context_,
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        bool* shouldContinue) {
    HAPPrecondition(context_);
    EnumerateAccessViolationsContext* context = context_;
    HAPPrecondition(context->callback);
    HAPPrecondition(!context->err);
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(clientIdentifier);
    HAPPrecondition(shouldContinue);
    HAPPrecondition(*shouldContinue);

    HAPError err;

    HAPCharacteristicValue_NetworkAccessViolationControl_Violation value;
    HAPRawBufferZero(&value, sizeof value);

    HAPPlatformWiFiRouterAccessViolationMetadata violationMetadata;
    err = HAPCheckedPlatformWiFiRouterClientGetAccessViolationMetadata(
            wiFiRouter, clientIdentifier, &violationMetadata);
    if (err) {
        HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidState);
        context->err = kHAPError_Unknown;
        *shouldContinue = false;
        return;
    }

    value.clientIdentifier = clientIdentifier;
    if (violationMetadata.wasReset) {
        value.resetTimestamp = violationMetadata.lastResetTimestamp;
        value.resetIsSet = true;
    }
    if (violationMetadata.hasViolations) {
        value.lastTimestamp = violationMetadata.lastViolationTimestamp;
        value.timestampIsSet = true;
    }

    context->callback(context->callbackContext, &value, shouldContinue);
}

HAP_RESULT_USE_CHECK
static HAPError EnumerateAccessViolations(
        HAPSequenceTLVDataSource* dataSource_,
        HAPCharacteristicValueCallback_NetworkAccessViolationControl_Response callback,
        void* _Nullable context) {
    HAPPrecondition(dataSource_);
    SequenceDataSource* dataSource = (SequenceDataSource*) dataSource_;
    HAPPrecondition(dataSource->server);
    HAPAccessoryServer* server = dataSource->server;
    HAPPrecondition(server->platform.ip.wiFiRouter);
    HAPPlatformWiFiRouterRef wiFiRouter = server->platform.ip.wiFiRouter;
    HAPPrecondition(dataSource->request);
    const HAPTLV8CharacteristicReadRequest* request HAP_UNUSED = dataSource->request;
    HAPPrecondition(callback);

    HAPError err;

    switch (server->ip.wiFiRouter.operation) {
        case kHAPWiFiRouterOperation_Undefined: {
        }
            return kHAPError_None;
        case kHAPWiFiRouterOperation_Enumerate: {
            EnumerateAccessViolationsContext enumerateContext;
            HAPRawBufferZero(&enumerateContext, sizeof enumerateContext);
            enumerateContext.callback = callback;
            enumerateContext.callbackContext = context;
            err = HAPCheckedPlatformWiFiRouterEnumerateClients(
                    wiFiRouter, EnumerateAccessViolationsCallback, &enumerateContext);
            if (!err) {
                err = enumerateContext.err;
            }
            if (err) {
                HAPAssert(err == kHAPError_Unknown);
                return err;
            }
        }
            return kHAPError_None;
        case kHAPWiFiRouterOperation_Query: {
        }
            HAPFatalError();
        case kHAPWiFiRouterOperation_Modify: {
        }
            return kHAPError_None;
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleWiFiRouterNetworkAccessViolationControlRead(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.ip.wiFiRouter);
    HAPPlatformWiFiRouterRef wiFiRouter = HAPNonnull(server->platform.ip.wiFiRouter);
    HAPPrecondition(request);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_NetworkAccessViolationControl));
    HAPPrecondition(request->characteristic->properties.ip.supportsWriteResponse);
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_WiFiRouter));
    HAPPrecondition(request->accessory->aid == 1);
    HAPPrecondition(responseWriter);

    HAPError err;

    // Operation is always set when a write request was received.
    // Since the characteristic has supportsWriteResponse set, the read handler is always invoked after a write request.
    // Therefore, when operation is not set we can assume that read handler was called due to a RaiseEvent.
    if (!server->ip.wiFiRouter.operation) {
        HAPCharacteristicValue_NetworkAccessViolationControl_Event value;
        HAPRawBufferZero(&value, sizeof value);

        HAPWiFiRouterGetEventClientList(server, request, &value.clientList, &value.clientListIsSet);
        err = HAPTLVWriterEncode(
                responseWriter, &kHAPCharacteristicTLVFormat_NetworkAccessViolationControl_Event, &value);
        if (err) {
            HAPAssert(
                    err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                    err == kHAPError_Busy);

            // Retry sending HAP event.
            HAPAccessoryServerRaiseEventOnSession(
                    server,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    HAPNonnull(request->session));
            return err;
        }
        HAPWiFiRouterHandleClientEventSent(server, request);
        return kHAPError_None;
    }

    HAPCharacteristicValue_NetworkAccessViolationControl_Response value;
    HAPRawBufferZero(&value, sizeof value);
    SequenceDataSource* dataSource = (SequenceDataSource*) &value.dataSource;

    dataSource->server = server;
    dataSource->request = request;
    value.enumerate = EnumerateAccessViolations;
    err = HAPTLVWriterEncode(
            responseWriter, &kHAPCharacteristicTLVFormat_NetworkAccessViolationControl_Response, &value);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                err == kHAPError_Busy);
        goto cleanup;
    }

cleanup : {
    HAPAssert(!server->ip.wiFiRouter.isChangingConfiguration);
    if (server->ip.wiFiRouter.hasExclusiveConfigurationAccess) {
        HAPCheckedPlatformWiFiRouterReleaseExclusiveConfigurationAccess(wiFiRouter);
        server->ip.wiFiRouter.hasExclusiveConfigurationAccess = false;
    }
    if (server->ip.wiFiRouter.hasSharedConfigurationAccess) {
        HAPCheckedPlatformWiFiRouterReleaseSharedConfigurationAccess(wiFiRouter);
        server->ip.wiFiRouter.hasSharedConfigurationAccess = false;
    }
    HAPRawBufferZero(&server->ip.wiFiRouter, sizeof server->ip.wiFiRouter);
}
    return err;
}

//----------------------------------------------------------------------------------------------------------------------

typedef struct {
    HAPAccessoryServer* server;
    const HAPTLV8CharacteristicWriteRequest* _Nullable request;
    HAPError err;
} EnumerateAccessViolationTLVContext;

static void EnumerateAccessViolationClientsCallback(
        void* _Nullable context_,
        uint32_t* value, // NOLINT(readability-non-const-parameter)
        bool* shouldContinue) {
    HAPPrecondition(context_);
    EnumerateAccessViolationTLVContext* context = context_;
    HAPPrecondition(context->server);
    HAPAccessoryServer* server = context->server;
    HAPPrecondition(server->platform.ip.wiFiRouter);
    HAPPlatformWiFiRouterRef wiFiRouter = HAPNonnull(server->platform.ip.wiFiRouter);
    HAPPrecondition(context->request);
    const HAPTLV8CharacteristicWriteRequest* request HAP_UNUSED = context->request;
    HAPPrecondition(!context->err);
    HAPPrecondition(value);
    HAPPrecondition(shouldContinue);
    HAPPrecondition(*shouldContinue);

    HAPError err;

    HAPAssert(server->ip.wiFiRouter.operation == kHAPWiFiRouterOperation_Modify);
    { err = HAPCheckedPlatformWiFiRouterClientResetAccessViolations(wiFiRouter, *value); }
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        context->err = err;
        *shouldContinue = false;
        return;
    }
}

HAP_RESULT_USE_CHECK
static HAPError ProcessWiFiRouterNetworkAccessViolationControlWrite(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicWriteRequest* request,
        HAPCharacteristicValue_NetworkAccessViolationControl* value) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.ip.wiFiRouter);
    HAPPlatformWiFiRouterRef wiFiRouter = HAPNonnull(server->platform.ip.wiFiRouter);
    HAPPrecondition(request);
    HAPPrecondition(value);

    HAPError err;

    HAPAssert(!server->ip.wiFiRouter.operation);
    switch (value->operationType) {
        case kHAPCharacteristicValue_NetworkAccessViolationControl_OperationType_List: {
            server->ip.wiFiRouter.operation = kHAPWiFiRouterOperation_Enumerate;
        }
            return kHAPError_None;
        case kHAPCharacteristicValue_NetworkAccessViolationControl_OperationType_Reset: {
            HAPAssert(!server->ip.wiFiRouter.hasExclusiveConfigurationAccess);
            err = HAPCheckedPlatformWiFiRouterAcquireExclusiveConfigurationAccess(wiFiRouter);
            if (err) {
                HAPAssert(err == kHAPError_Busy);
                return err;
            }
            server->ip.wiFiRouter.hasExclusiveConfigurationAccess = true;

            server->ip.wiFiRouter.operation = kHAPWiFiRouterOperation_Modify;

            EnumerateAccessViolationTLVContext enumerateContext;
            HAPRawBufferZero(&enumerateContext, sizeof enumerateContext);
            enumerateContext.server = server;
            enumerateContext.request = request;
            err = value->clientList.enumerate(
                    &value->clientList.dataSource, EnumerateAccessViolationClientsCallback, &enumerateContext);
            if (!err) {
                err = enumerateContext.err;
            }
            if (err) {
                HAPAssert(
                        err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                        err == kHAPError_OutOfResources || err == kHAPError_Busy);
                return err;
            }
        }
            return kHAPError_None;
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleWiFiRouterNetworkAccessViolationControlWrite(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicWriteRequest* request,
        HAPTLVReader* requestReader,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.ip.wiFiRouter);
    HAPPlatformWiFiRouterRef wiFiRouter = HAPNonnull(server->platform.ip.wiFiRouter);
    HAPPrecondition(request);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_NetworkAccessViolationControl));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_WiFiRouter));
    HAPPrecondition(request->accessory->aid == 1);
    HAPPrecondition(requestReader);

    HAPError err;

    HAPAssert(!server->ip.wiFiRouter.hasSharedConfigurationAccess);
    err = HAPCheckedPlatformWiFiRouterAcquireSharedConfigurationAccess(wiFiRouter);
    if (err) {
        HAPAssert(err == kHAPError_Busy);
        goto cleanup;
    }
    server->ip.wiFiRouter.hasSharedConfigurationAccess = true;

    bool isEnabled;
    err = HAPCheckedPlatformWiFiRouterIsManagedNetworkEnabled(wiFiRouter, &isEnabled);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        goto cleanup;
    }
    if (!isEnabled) {
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Characteristic not accessible because managed network is disabled.");
        err = kHAPError_InvalidState;
        goto cleanup;
    }

    HAPCharacteristicValue_NetworkAccessViolationControl value;
    err = HAPTLVReaderDecode(requestReader, &kHAPCharacteristicTLVFormat_NetworkAccessViolationControl, &value);
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        goto cleanup;
    }

    err = ProcessWiFiRouterNetworkAccessViolationControlWrite(server, request, &value);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                err == kHAPError_OutOfResources || err == kHAPError_Busy);
        goto cleanup;
    }

cleanup : {
    if (err) {
        HAPAssert(!server->ip.wiFiRouter.isChangingConfiguration);
        if (server->ip.wiFiRouter.hasExclusiveConfigurationAccess) {
            HAPCheckedPlatformWiFiRouterReleaseExclusiveConfigurationAccess(wiFiRouter);
            server->ip.wiFiRouter.hasExclusiveConfigurationAccess = false;
        }
        if (server->ip.wiFiRouter.hasSharedConfigurationAccess) {
            HAPCheckedPlatformWiFiRouterReleaseSharedConfigurationAccess(wiFiRouter);
            server->ip.wiFiRouter.hasSharedConfigurationAccess = false;
        }
        HAPRawBufferZero(&server->ip.wiFiRouter, sizeof server->ip.wiFiRouter);
    }
}
    return err;
}

//----------------------------------------------------------------------------------------------------------------------

void HAPHandleWiFiRouterNetworkAccessViolationControlSubscribe(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicSubscriptionRequest* request,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.ip.wiFiRouter);
    HAPPlatformWiFiRouterRef wiFiRouter HAP_UNUSED = HAPNonnull(server->platform.ip.wiFiRouter);
    HAPPrecondition(request);
    HAPPrecondition(request->session);
    HAPSession* session = request->session;
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_NetworkAccessViolationControl));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_WiFiRouter));
    HAPPrecondition(request->accessory->aid == 1);

    HAPWiFiRouterEventState* eventState = &server->ip.wiFiRouterEventState.networkAccessViolationControl;
    HAPWiFiRouterSessionState* sessionState = &session->wiFiRouterEventState.networkAccessViolationControl;

    HAPWiFiRouterSubscribeForEvents(eventState, sessionState);
}

//----------------------------------------------------------------------------------------------------------------------

void HAPHandleWiFiRouterNetworkAccessViolationControlUnsubscribe(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicSubscriptionRequest* request,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.ip.wiFiRouter);
    HAPPlatformWiFiRouterRef wiFiRouter HAP_UNUSED = HAPNonnull(server->platform.ip.wiFiRouter);
    HAPPrecondition(request);
    HAPPrecondition(request->session);
    HAPSession* session = request->session;
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_NetworkAccessViolationControl));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_WiFiRouter));
    HAPPrecondition(request->accessory->aid == 1);

    HAPWiFiRouterEventState* eventState = &server->ip.wiFiRouterEventState.networkAccessViolationControl;
    HAPWiFiRouterSessionState* sessionState = &session->wiFiRouterEventState.networkAccessViolationControl;

    HAPWiFiRouterUnsubscribeFromEvents(eventState, sessionState);
}

#endif
