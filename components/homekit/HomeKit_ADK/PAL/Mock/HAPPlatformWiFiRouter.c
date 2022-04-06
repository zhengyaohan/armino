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

#include "HAPPlatformWiFiRouter+Init.h"

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "WiFiRouter(Mock)" };

void HAPPlatformWiFiRouterCreate(
        HAPPlatformWiFiRouterRef _Nonnull wiFiRouter,
        const HAPPlatformWiFiRouterOptions* _Nonnull options) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(options);

    HAPRawBufferZero(wiFiRouter, sizeof *wiFiRouter);

    // Main WAN.
    HAPAssert(wiFiRouter->numWANs < HAPArrayCount(wiFiRouter->wans));
    wiFiRouter->wans[wiFiRouter->numWANs].identifier = kHAPPlatformWiFiRouterWANIdentifier_Main;
    wiFiRouter->wans[wiFiRouter->numWANs].wanType = kHAPPlatformWiFiRouterWANType_DHCP;
    wiFiRouter->wans[wiFiRouter->numWANs].wanStatus = 0;
    wiFiRouter->numWANs++;

    // Main network client group.
    HAPAssert(wiFiRouter->numGroups < HAPArrayCount(wiFiRouter->groups));
    wiFiRouter->groups[wiFiRouter->numGroups].identifier = kHAPPlatformWiFiRouterGroupIdentifier_Main;
    wiFiRouter->numGroups++;

    wiFiRouter->isReady = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void HAPPlatformWiFiRouterSetDelegate(
        HAPPlatformWiFiRouterRef _Nonnull wiFiRouter,
        const HAPPlatformWiFiRouterDelegate* _Nullable delegate) {
    HAPPrecondition(wiFiRouter);

    if (delegate) {
        wiFiRouter->delegate = *delegate;
    } else {
        HAPRawBufferZero(&wiFiRouter->delegate, sizeof wiFiRouter->delegate);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterAcquireSharedConfigurationAccess(HAPPlatformWiFiRouterRef _Nonnull wiFiRouter) {
    HAPPrecondition(wiFiRouter);

    wiFiRouter->sharedAccessRefCount++;
    HAPAssert(wiFiRouter->sharedAccessRefCount);
    HAPLogDebug(&logObject, "Shared access reference count: %u.", wiFiRouter->sharedAccessRefCount);
    return kHAPError_None;
}

void HAPPlatformWiFiRouterReleaseSharedConfigurationAccess(HAPPlatformWiFiRouterRef _Nonnull wiFiRouter) {
    HAPPrecondition(wiFiRouter);

    HAPAssert(wiFiRouter->sharedAccessRefCount);
    wiFiRouter->sharedAccessRefCount--;
    HAPLogDebug(&logObject, "Shared access reference count: %u.", wiFiRouter->sharedAccessRefCount);
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterAcquireExclusiveConfigurationAccess(HAPPlatformWiFiRouterRef _Nonnull wiFiRouter) {
    HAPPrecondition(wiFiRouter);

    wiFiRouter->exclusiveAccessRefCount++;
    HAPAssert(wiFiRouter->exclusiveAccessRefCount);
    HAPLogDebug(&logObject, "Exclusive access reference count: %u.", wiFiRouter->exclusiveAccessRefCount);
    return kHAPError_None;
}

void HAPPlatformWiFiRouterReleaseExclusiveConfigurationAccess(HAPPlatformWiFiRouterRef _Nonnull wiFiRouter) {
    HAPPrecondition(wiFiRouter);

    HAPAssert(wiFiRouter->exclusiveAccessRefCount);
    wiFiRouter->exclusiveAccessRefCount--;
    HAPLogDebug(&logObject, "Exclusive access reference count: %u.", wiFiRouter->exclusiveAccessRefCount);
}

HAP_RESULT_USE_CHECK
bool HAPPlatformWiFiRouterIsConfigurationAccessAvailable(HAPPlatformWiFiRouterRef _Nonnull wiFiRouter) {
    HAPPrecondition(wiFiRouter);

    return wiFiRouter->sharedAccessRefCount == 0 && wiFiRouter->exclusiveAccessRefCount == 0 &&
           !wiFiRouter->isModifyingConfiguration;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HAP_RESULT_USE_CHECK
static bool HAPWiFiWPAPersonalCredentialAreEqual(
        const HAPWiFiWPAPersonalCredential* _Nonnull credential,
        const HAPWiFiWPAPersonalCredential* _Nonnull otherCredential) {
    HAPPrecondition(credential);
    HAPPrecondition(otherCredential);

    if (credential->type != otherCredential->type) {
        return false;
    }
    switch (credential->type) {
        case kHAPWiFiWPAPersonalCredentialType_Passphrase: {
            if (!HAPStringAreEqual(credential->_.passphrase.stringValue, otherCredential->_.passphrase.stringValue)) {
                return false;
            }
            break;
        }
        case kHAPWiFiWPAPersonalCredentialType_PSK: {
            if (!HAPRawBufferAreEqual(
                        credential->_.psk.bytes, otherCredential->_.psk.bytes, sizeof credential->_.psk.bytes)) {
                return false;
            }
            break;
        }
    }

    return true;
}

static bool HAPPlatformWiFiRouterClientCredentialAreEqual(
        const HAPPlatformWiFiRouterClientCredential* _Nonnull credential,
        const HAPPlatformWiFiRouterClientCredential* _Nonnull otherCredential) {
    HAPPrecondition(credential);
    HAPPrecondition(otherCredential);

    if (credential->type != otherCredential->type) {
        return false;
    }

    switch (credential->type) {
        case kHAPPlatformWiFiRouterCredentialType_MACAddress: {
            return HAPRawBufferAreEqual(
                    credential->_.macAddress.bytes,
                    otherCredential->_.macAddress.bytes,
                    sizeof credential->_.macAddress.bytes);
        }
        case kHAPPlatformWiFiRouterCredentialType_PSK: {
            return HAPWiFiWPAPersonalCredentialAreEqual(&credential->_.psk, &otherCredential->_.psk);
        }
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterBeginConfigurationChange(HAPPlatformWiFiRouterRef _Nonnull wiFiRouter) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wiFiRouter->exclusiveAccessRefCount);
    HAPPrecondition(!wiFiRouter->isModifyingConfiguration);
    HAPPrecondition(!wiFiRouter->enumerationRefCount);

    HAPRawBufferZero(wiFiRouter->edits, sizeof wiFiRouter->edits);
    wiFiRouter->numEdits = 0;
    wiFiRouter->isModifyingConfiguration = true;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterCommitConfigurationChange(HAPPlatformWiFiRouterRef _Nonnull wiFiRouter) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wiFiRouter->exclusiveAccessRefCount);
    HAPPrecondition(wiFiRouter->isModifyingConfiguration);
    HAPPrecondition(!wiFiRouter->enumerationRefCount);
    HAPPrecondition(!wiFiRouter->isEditingWANFirewall);
    HAPPrecondition(!wiFiRouter->isEditingLANFirewall);

    for (size_t i = 0; i < wiFiRouter->numEdits; i++) {
        size_t clientIndex;
        for (clientIndex = 0; clientIndex < wiFiRouter->numClients; clientIndex++) {
            if (wiFiRouter->clients[clientIndex].identifier == wiFiRouter->edits[i].identifier) {
                break;
            }
        }

        if (!wiFiRouter->edits[i].groupIdentifier) {
            // Remove.
            wiFiRouter->numClients--;
            HAPRawBufferCopyBytes(
                    &wiFiRouter->clients[clientIndex],
                    &wiFiRouter->clients[clientIndex + 1],
                    (wiFiRouter->numClients - clientIndex) * sizeof wiFiRouter->clients[0]);
            HAPRawBufferCopyBytes(
                    &wiFiRouter->accessViolations[clientIndex],
                    &wiFiRouter->accessViolations[clientIndex + 1],
                    (wiFiRouter->numClients - clientIndex) * sizeof wiFiRouter->accessViolations[0]);

            // Disconnect removed network clients if connected.
            for (size_t j = 0; j < HAPArrayCount(wiFiRouter->connections); j++) {
                if (!wiFiRouter->connections[j].isActive) {
                    continue;
                }
                if (wiFiRouter->connections[j].clientIdentifier == wiFiRouter->edits[i].identifier) {
                    HAPPlatformWiFiRouterDisconnectClient(wiFiRouter, &wiFiRouter->connections[j].macAddress);
                }
            }
        } else {
            bool reconnectRequired = false;
            if (wiFiRouter->edits[i].identifier != wiFiRouter->clients[clientIndex].identifier) {
                reconnectRequired = true;
            }
            if (wiFiRouter->edits[i].groupIdentifier != wiFiRouter->clients[clientIndex].groupIdentifier) {
                reconnectRequired = true;
            }
            if (!HAPPlatformWiFiRouterClientCredentialAreEqual(
                        &wiFiRouter->edits[i].credential, &wiFiRouter->clients[clientIndex].credential)) {
                reconnectRequired = true;
            }

            // Add.
            if (clientIndex == wiFiRouter->numClients) {
                wiFiRouter->numClients++;

                // Initialize network access violations.
                HAPRawBufferZero(
                        &wiFiRouter->accessViolations[clientIndex], sizeof wiFiRouter->accessViolations[clientIndex]);
            }

            // Update.
            HAPRawBufferCopyBytes(
                    &wiFiRouter->clients[clientIndex], &wiFiRouter->edits[i], sizeof wiFiRouter->edits[i]);
            HAPPlatformWiFiRouterClient* client = &wiFiRouter->clients[clientIndex];
            if (client->wanFirewall.type == kHAPPlatformWiFiRouterFirewallType_Allowlist) {
                for (size_t j = 0; j < client->wanFirewall.numRules; j++) {
                    HAPPlatformWiFiRouterWANFirewallRuleType ruleType =
                            *(const HAPPlatformWiFiRouterWANFirewallRuleType*) &client->wanFirewall.rules[j];
                    switch (ruleType) {
                        case kHAPPlatformWiFiRouterWANFirewallRuleType_Port: {
                            HAPPlatformWiFiRouterPortWANFirewallRule* firewallRule =
                                    &client->wanFirewall.rules[j].port._;

                            if (client->wanFirewall.rules[j].port._.host.type ==
                                kHAPPlatformWiFiRouterWANHostURIType_DNSNamePattern) {
                                firewallRule->host._.dnsNamePattern = client->wanFirewall.rules[j].port.bytes;
                            }
                            break;
                        }
                        case kHAPPlatformWiFiRouterWANFirewallRuleType_ICMP: {
                            HAPPlatformWiFiRouterICMPWANFirewallRule* firewallRule =
                                    &client->wanFirewall.rules[j].icmp._;

                            if (client->wanFirewall.rules[j].icmp._.host.type ==
                                kHAPPlatformWiFiRouterWANHostURIType_DNSNamePattern) {
                                firewallRule->host._.dnsNamePattern = client->wanFirewall.rules[j].icmp.bytes;
                            }
                            break;
                        }
                    }
                }
            }
            if (client->lanFirewall.type == kHAPPlatformWiFiRouterFirewallType_Allowlist) {
                for (size_t j = 0; j < client->lanFirewall.numRules; j++) {
                    HAPPlatformWiFiRouterLANFirewallRuleType ruleType =
                            *(const HAPPlatformWiFiRouterLANFirewallRuleType*) &client->lanFirewall.rules[j];
                    switch (ruleType) {
                        case kHAPPlatformWiFiRouterLANFirewallRuleType_MulticastBridging:
                        case kHAPPlatformWiFiRouterLANFirewallRuleType_StaticPort: {
                            break;
                        }
                        case kHAPPlatformWiFiRouterLANFirewallRuleType_DynamicPort: {
                            HAPPlatformWiFiRouterDynamicPortLANFirewallRule* firewallRule =
                                    &client->lanFirewall.rules[j].dynamicPort._;

                            if (client->lanFirewall.rules[j].dynamicPort.serviceTypeStorageInUse) {
                                switch (firewallRule->serviceType.advertisementProtocol) {
                                    case kHAPPlatformWiFiRouterDynamicPortAdvertisementProtocol_DNSSD: {
                                        firewallRule->serviceType._.dns_sd.serviceType =
                                                client->lanFirewall.rules[j].dynamicPort.serviceTypeStorage;
                                        break;
                                    }
                                    case kHAPPlatformWiFiRouterDynamicPortAdvertisementProtocol_SSDP: {
                                        firewallRule->serviceType._.ssdp.serviceTypeURI =
                                                client->lanFirewall.rules[j].dynamicPort.serviceTypeStorage;
                                        break;
                                    }
                                }
                            }
                            break;
                        }
                        case kHAPPlatformWiFiRouterLANFirewallRuleType_StaticICMP: {
                            break;
                        }
                    }
                }
            }

            // Reconnect modified network clients if necessary.
            if (reconnectRequired) {
                for (size_t j = 0; j < HAPArrayCount(wiFiRouter->connections); j++) {
                    if (!wiFiRouter->connections[j].isActive) {
                        continue;
                    }
                    if (wiFiRouter->connections[j].clientIdentifier == wiFiRouter->edits[i].identifier ||
                        (wiFiRouter->edits[i].credential.type == kHAPPlatformWiFiRouterCredentialType_MACAddress &&
                         HAPRawBufferAreEqual(
                                 wiFiRouter->connections[j].macAddress.bytes,
                                 wiFiRouter->edits[i].credential._.macAddress.bytes,
                                 sizeof wiFiRouter->edits[i].credential._.macAddress.bytes))) {
                        HAPPlatformWiFiRouterDisconnectClient(wiFiRouter, &wiFiRouter->connections[j].macAddress);
                    }
                }
            }
        }
    }

    HAPRawBufferZero(wiFiRouter->edits, sizeof wiFiRouter->edits);
    wiFiRouter->numEdits = 0;
    wiFiRouter->isModifyingConfiguration = false;
    return kHAPError_None;
}

void HAPPlatformWiFiRouterRollbackConfigurationChange(HAPPlatformWiFiRouterRef _Nonnull wiFiRouter) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wiFiRouter->exclusiveAccessRefCount);
    HAPPrecondition(wiFiRouter->isModifyingConfiguration);
    HAPPrecondition(!wiFiRouter->enumerationRefCount);

    HAPRawBufferZero(wiFiRouter->edits, sizeof wiFiRouter->edits);
    wiFiRouter->numEdits = 0;
    wiFiRouter->isModifyingConfiguration = false;
    wiFiRouter->isEditingWANFirewall = false;
    wiFiRouter->isEditingLANFirewall = false;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterIsReady(HAPPlatformWiFiRouterRef _Nonnull wiFiRouter, bool* _Nonnull isReady) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(isReady);

    *isReady = wiFiRouter->isReady;
    return kHAPError_None;
}

void HAPPlatformWiFiRouterSetReady(HAPPlatformWiFiRouterRef _Nonnull wiFiRouter, bool ready) {
    HAPPrecondition(wiFiRouter);

    if (ready == wiFiRouter->isReady) {
        return;
    }

    HAPLogInfo(&logObject, "Setting router status to %s.", ready ? "ready" : "not ready");
    wiFiRouter->isReady = ready;
    if (wiFiRouter->delegate.handleReadyStateChanged) {
        wiFiRouter->delegate.handleReadyStateChanged(wiFiRouter, wiFiRouter->delegate.context);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterIsManagedNetworkEnabled(
        HAPPlatformWiFiRouterRef _Nonnull wiFiRouter,
        bool* _Nonnull isEnabled) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wiFiRouter->sharedAccessRefCount || wiFiRouter->exclusiveAccessRefCount);
    HAPPrecondition(isEnabled);

    *isEnabled = wiFiRouter->isManagedNetworkEnabled;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterSetManagedNetworkEnabled(HAPPlatformWiFiRouterRef _Nonnull wiFiRouter, bool isEnabled) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wiFiRouter->exclusiveAccessRefCount);
    HAPPrecondition(!wiFiRouter->isModifyingConfiguration);
    HAPPrecondition(!wiFiRouter->enumerationRefCount);

    if (isEnabled == wiFiRouter->isManagedNetworkEnabled) {
        return kHAPError_None;
    }
    if (isEnabled) {
        for (size_t i = 0; i < wiFiRouter->numWANs; i++) {
            if (wiFiRouter->wans[i].wanType == kHAPPlatformWiFiRouterWANType_BridgeMode) {
                HAPLog(&logObject, "Managed Network cannot be enabled: A WAN is in bridge mode.");
                return kHAPError_InvalidState;
            }
        }
    }

    wiFiRouter->isManagedNetworkEnabled = isEnabled;

    // Delete all network clients.
    wiFiRouter->numClients = 0;
    HAPRawBufferZero(wiFiRouter->clients, sizeof wiFiRouter->clients);
    HAPRawBufferZero(wiFiRouter->accessViolations, sizeof wiFiRouter->accessViolations);

    // Reset network client identifier allocations.
    wiFiRouter->lastAddedClientIdentifier = 0;

    // Set up or delete Restricted network client group.
    HAPPlatformWiFiRouterGroupIdentifier groupIdentifier = kHAPPlatformWiFiRouterGroupIdentifier_Restricted;
    if (isEnabled) {
        for (size_t i = 0; i < wiFiRouter->numGroups; i++) {
            HAPAssert(wiFiRouter->groups[i].identifier != groupIdentifier);
        }
        HAPAssert(wiFiRouter->numGroups < HAPArrayCount(wiFiRouter->groups));
        HAPRawBufferZero(&wiFiRouter->groups[wiFiRouter->numGroups], sizeof wiFiRouter->groups[0]);
        wiFiRouter->groups[wiFiRouter->numGroups].identifier = groupIdentifier;
        wiFiRouter->numGroups++;
    } else {
        for (size_t i = 0; i < HAPArrayCount(wiFiRouter->connections); i++) {
            if (wiFiRouter->connections[i].isActive && wiFiRouter->connections[i].clientIdentifier) {
                HAPPlatformWiFiRouterDisconnectClient(wiFiRouter, &wiFiRouter->connections[i].macAddress);
            }
        }

        bool deleted = false;
        for (size_t i = 0; i < wiFiRouter->numGroups; i++) {
            if (wiFiRouter->groups[i].identifier == groupIdentifier) {
                wiFiRouter->numGroups--;
                HAPRawBufferCopyBytes(
                        &wiFiRouter->groups[i],
                        &wiFiRouter->groups[i + 1],
                        (wiFiRouter->numGroups - i) * sizeof wiFiRouter->groups[0]);
                deleted = true;
                break;
            }
        }
        HAPAssert(deleted);
    }
    return kHAPError_None;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterEnumerateWANs(
        HAPPlatformWiFiRouterRef _Nonnull wiFiRouter,
        HAPPlatformWiFiRouterEnumerateWANsCallback _Nonnull callback,
        void* _Nullable context) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wiFiRouter->sharedAccessRefCount || wiFiRouter->exclusiveAccessRefCount);
    HAPPrecondition(callback);

    wiFiRouter->enumerationRefCount++;
    HAPAssert(wiFiRouter->enumerationRefCount);

    bool shouldContinue = true;
    for (size_t i = 0; shouldContinue && i < wiFiRouter->numWANs; i++) {
        callback(context, wiFiRouter, wiFiRouter->wans[i].identifier, &shouldContinue);
    }

    HAPAssert(wiFiRouter->enumerationRefCount);
    wiFiRouter->enumerationRefCount--;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterWANExists(
        HAPPlatformWiFiRouterRef _Nonnull wiFiRouter,
        HAPPlatformWiFiRouterWANIdentifier wanIdentifier,
        bool* _Nonnull exists) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wiFiRouter->sharedAccessRefCount || wiFiRouter->exclusiveAccessRefCount);
    HAPPrecondition(wanIdentifier);
    HAPPrecondition(exists);

    for (size_t i = 0; i < wiFiRouter->numWANs; i++) {
        if (wiFiRouter->wans[i].identifier == wanIdentifier) {
            *exists = true;
            return kHAPError_None;
        }
    }
    *exists = false;
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterWANGetType(
        HAPPlatformWiFiRouterRef _Nonnull wiFiRouter,
        HAPPlatformWiFiRouterWANIdentifier wanIdentifier,
        HAPPlatformWiFiRouterWANType* _Nonnull wanType) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wiFiRouter->sharedAccessRefCount || wiFiRouter->exclusiveAccessRefCount);
    HAPPrecondition(wanIdentifier);
    HAPPrecondition(wanType);

    for (size_t i = 0; i < wiFiRouter->numWANs; i++) {
        if (wiFiRouter->wans[i].identifier == wanIdentifier) {
            *wanType = wiFiRouter->wans[i].wanType;
            return kHAPError_None;
        }
    }
    HAPLog(&logObject, "WAN %lu not found.", (unsigned long) wanIdentifier);
    return kHAPError_InvalidState;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterWANSetType(
        HAPPlatformWiFiRouterRef _Nonnull wiFiRouter,
        HAPPlatformWiFiRouterWANIdentifier wanIdentifier,
        HAPPlatformWiFiRouterWANType wanType) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wiFiRouter->exclusiveAccessRefCount);
    HAPPrecondition(!wiFiRouter->isModifyingConfiguration);
    HAPPrecondition(wanIdentifier);

    HAPError err;

    for (size_t i = 0; i < wiFiRouter->numWANs; i++) {
        if (wiFiRouter->wans[i].identifier == wanIdentifier) {
            if (wanType == wiFiRouter->wans[i].wanType) {
                return kHAPError_None;
            }
            if (wanType == kHAPPlatformWiFiRouterWANType_BridgeMode && wiFiRouter->isManagedNetworkEnabled) {
                HAPLog(&logObject, "Disabling managed network because WAN type is changed to bridge mode.");
                err = HAPPlatformWiFiRouterSetManagedNetworkEnabled(wiFiRouter, /* isEnabled: */ false);
                if (err) {
                    HAPAssert(err == kHAPError_Unknown);
                    return err;
                }
                if (wiFiRouter->delegate.handleManagedNetworkStateChanged) {
                    wiFiRouter->delegate.handleManagedNetworkStateChanged(wiFiRouter, wiFiRouter->delegate.context);
                }
            }
            HAPLogInfo(&logObject, "Setting WAN %lu type to %u.", (unsigned long) wanIdentifier, wanType);
            wiFiRouter->wans[i].wanType = wanType;
            if (wiFiRouter->delegate.handleWANConfigurationChanged) {
                wiFiRouter->delegate.handleWANConfigurationChanged(wiFiRouter, wiFiRouter->delegate.context);
            }
            return kHAPError_None;
        }
    }
    HAPLog(&logObject, "WAN %lu not found.", (unsigned long) wanIdentifier);
    return kHAPError_InvalidState;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterWANGetStatus(
        HAPPlatformWiFiRouterRef _Nonnull wiFiRouter,
        HAPPlatformWiFiRouterWANIdentifier wanIdentifier,
        HAPPlatformWiFiRouterWANStatus* _Nonnull wanStatus) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wiFiRouter->sharedAccessRefCount || wiFiRouter->exclusiveAccessRefCount);
    HAPPrecondition(wanIdentifier);
    HAPPrecondition(wanStatus);

    for (size_t i = 0; i < wiFiRouter->numWANs; i++) {
        if (wiFiRouter->wans[i].identifier == wanIdentifier) {
            *wanStatus = wiFiRouter->wans[i].wanStatus;
            return kHAPError_None;
        }
    }
    HAPLog(&logObject, "WAN %lu not found.", (unsigned long) wanIdentifier);
    return kHAPError_InvalidState;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterWANSetStatus(
        HAPPlatformWiFiRouterRef _Nonnull wiFiRouter,
        HAPPlatformWiFiRouterWANIdentifier wanIdentifier,
        HAPPlatformWiFiRouterWANStatus wanStatus) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wanIdentifier);
    HAPPrecondition(wanStatus);

    for (size_t i = 0; i < wiFiRouter->numWANs; i++) {
        if (wiFiRouter->wans[i].identifier == wanIdentifier) {
            if (wanStatus == wiFiRouter->wans[i].wanStatus) {
                return kHAPError_None;
            }

            HAPLogInfo(&logObject, "Setting WAN %lu status to %u.", (unsigned long) wanIdentifier, wanStatus);
            wiFiRouter->wans[i].wanStatus = wanStatus;
            if (wiFiRouter->delegate.handleWANStatusChanged) {
                wiFiRouter->delegate.handleWANStatusChanged(wiFiRouter, wiFiRouter->delegate.context);
            }
            return kHAPError_None;
        }
    }
    HAPLog(&logObject, "WAN %lu not found.", (unsigned long) wanIdentifier);
    return kHAPError_InvalidState;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterEnumerateClients(
        HAPPlatformWiFiRouterRef _Nonnull wiFiRouter,
        HAPPlatformWiFiRouterEnumerateClientsCallback _Nonnull callback,
        void* _Nullable context) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wiFiRouter->sharedAccessRefCount || wiFiRouter->exclusiveAccessRefCount);
    HAPPrecondition(callback);

    wiFiRouter->enumerationRefCount++;
    HAPAssert(wiFiRouter->enumerationRefCount);

    bool shouldContinue = true;
    for (size_t i = 0; shouldContinue && i < wiFiRouter->numClients; i++) {
        // Check if network client profile is being deleted.
        bool pendingDeletion = false;
        for (size_t j = 0; j < wiFiRouter->numEdits; j++) {
            if (wiFiRouter->edits[j].identifier != wiFiRouter->clients[i].identifier) {
                continue;
            }
            if (!wiFiRouter->edits[j].groupIdentifier) {
                pendingDeletion = true;
            }
            break;
        }
        if (pendingDeletion) {
            continue;
        }

        // Report network client.
        callback(context, wiFiRouter, wiFiRouter->clients[i].identifier, &shouldContinue);
    }
    for (size_t i = 0; shouldContinue && i < wiFiRouter->numEdits; i++) {
        if (!wiFiRouter->edits[i].groupIdentifier) {
            continue;
        }

        // Check if network client profile was already reported.
        bool alreadyReported = false;
        for (size_t j = 0; j < wiFiRouter->numClients; j++) {
            if (wiFiRouter->clients[j].identifier == wiFiRouter->edits[i].identifier) {
                alreadyReported = true;
                break;
            }
        }
        if (alreadyReported) {
            continue;
        }

        // Report network client.
        callback(context, wiFiRouter, wiFiRouter->edits[i].identifier, &shouldContinue);
    }

    HAPAssert(wiFiRouter->enumerationRefCount);
    wiFiRouter->enumerationRefCount--;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientExists(
        HAPPlatformWiFiRouterRef _Nonnull wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        bool* _Nonnull exists) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wiFiRouter->sharedAccessRefCount || wiFiRouter->exclusiveAccessRefCount);
    HAPPrecondition(clientIdentifier);
    HAPPrecondition(exists);

    for (size_t i = 0; i < wiFiRouter->numEdits; i++) {
        if (wiFiRouter->edits[i].identifier == clientIdentifier) {
            *exists = wiFiRouter->edits[i].groupIdentifier != 0;
            return kHAPError_None;
        }
    }
    for (size_t i = 0; i < wiFiRouter->numClients; i++) {
        if (wiFiRouter->clients[i].identifier == clientIdentifier) {
            *exists = true;
            return kHAPError_None;
        }
    }
    *exists = false;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterAddClient(
        HAPPlatformWiFiRouterRef _Nonnull wiFiRouter,
        HAPPlatformWiFiRouterGroupIdentifier groupIdentifier,
        const HAPPlatformWiFiRouterClientCredential* _Nonnull credential,
        HAPPlatformWiFiRouterClientIdentifier* _Nonnull clientIdentifier) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wiFiRouter->exclusiveAccessRefCount);
    HAPPrecondition(wiFiRouter->isModifyingConfiguration);
    HAPPrecondition(!wiFiRouter->enumerationRefCount);
    HAPPrecondition(groupIdentifier);
    HAPPrecondition(credential);
    HAPPrecondition(clientIdentifier);

    if (!wiFiRouter->isManagedNetworkEnabled) {
        HAPLog(&logObject, "Network client profile management is only available when managed network is enabled.");
        return kHAPError_Unknown;
    }

    // Validate network client group identifier.
    bool foundGroup = false;
    for (size_t i = 0; i < wiFiRouter->numGroups; i++) {
        if (wiFiRouter->groups[i].identifier == groupIdentifier) {
            foundGroup = true;
            break;
        }
    }
    if (!foundGroup) {
        HAPLog(&logObject, "Network client group %lu not found.", (unsigned long) groupIdentifier);
        return kHAPError_InvalidData;
    }

    // Validate credential.
    bool exists = false;
    for (size_t i = 0; i < wiFiRouter->numClients; i++) {
        if (!HAPPlatformWiFiRouterClientCredentialAreEqual(credential, &wiFiRouter->clients[i].credential)) {
            continue;
        }

        exists = true;

        // Check if this network client profile is being deleted.
        for (size_t j = 0; j < wiFiRouter->numEdits; j++) {
            if (wiFiRouter->edits[j].identifier != wiFiRouter->clients[i].identifier) {
                continue;
            }

            // If network client profile is being removed, the credential is available again.
            if (!wiFiRouter->edits[j].groupIdentifier) {
                exists = false;
                break;
            }
        }
        break;
    }
    if (!exists) {
        for (size_t i = 0; i < wiFiRouter->numEdits; i++) {
            if (!wiFiRouter->edits[i].groupIdentifier) {
                continue;
            }

            // If same credential is already used in a pending add / update, it is unavailable.
            if (HAPPlatformWiFiRouterClientCredentialAreEqual(credential, &wiFiRouter->clients[i].credential)) {
                exists = true;
                break;
            }
        }
    }
    if (exists) {
        HAPLog(&logObject, "Credential already exists.");
        return kHAPError_NotAuthorized;
    }

    // Assign network client profile identifier.
    if (wiFiRouter->lastAddedClientIdentifier == (HAPPlatformWiFiRouterClientIdentifier) -1) {
        HAPLog(&logObject, "Ran out of network client profile identifiers. Managed network must be reset.");
        return kHAPError_OutOfResources;
    }
    *clientIdentifier = ++wiFiRouter->lastAddedClientIdentifier;

    // Store pending network configuration change.
    size_t i = wiFiRouter->numEdits;
    if (i >= HAPArrayCount(wiFiRouter->edits)) {
        HAPLog(&logObject, "Maximum number of pending configuration changes reached.");
        return kHAPError_OutOfResources;
    }
    if (i == wiFiRouter->numEdits) {
        wiFiRouter->numEdits++;
    }
    wiFiRouter->edits[i].identifier = *clientIdentifier;
    wiFiRouter->edits[i].groupIdentifier = groupIdentifier;
    wiFiRouter->edits[i].credential = *credential;
    wiFiRouter->edits[i].wanFirewall.type = kHAPPlatformWiFiRouterFirewallType_Allowlist;
    wiFiRouter->edits[i].wanFirewall.numRules = 0;
    wiFiRouter->edits[i].lanFirewall.type = kHAPPlatformWiFiRouterFirewallType_Allowlist;
    wiFiRouter->edits[i].lanFirewall.numRules = 0;

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static HAPError HAPPlatformWiFiRouterClientFind(
        HAPPlatformWiFiRouterRef _Nonnull wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterClient* _Nonnull* _Nonnull client) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wiFiRouter->sharedAccessRefCount || wiFiRouter->exclusiveAccessRefCount);
    HAPPrecondition(clientIdentifier);
    HAPPrecondition(client);

    // Check if network client profile exists.
    for (size_t i = 0; i < wiFiRouter->numEdits; i++) {
        if (wiFiRouter->edits[i].identifier == clientIdentifier) {
            // Check if network client profile is being deleted.
            if (!wiFiRouter->edits[i].groupIdentifier) {
                HAPLog(&logObject, "Network client profile %lu not found.", (unsigned long) clientIdentifier);
                return kHAPError_InvalidState;
            }

            *client = &wiFiRouter->edits[i];
            return kHAPError_None;
        }
    }
    for (size_t i = 0; i < wiFiRouter->numClients; i++) {
        if (wiFiRouter->clients[i].identifier == clientIdentifier) {
            *client = &wiFiRouter->clients[i];
            return kHAPError_None;
        }
    }

    HAPLog(&logObject, "Network client profile %lu not found.", (unsigned long) clientIdentifier);
    return kHAPError_InvalidState;
}

HAP_RESULT_USE_CHECK
static HAPError HAPPlatformWiFiRouterClientEdit(
        HAPPlatformWiFiRouterRef _Nonnull wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        size_t* _Nonnull editIndex) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wiFiRouter->exclusiveAccessRefCount);
    HAPPrecondition(wiFiRouter->isModifyingConfiguration);
    HAPPrecondition(!wiFiRouter->enumerationRefCount);
    HAPPrecondition(clientIdentifier);
    HAPPrecondition(editIndex);

    // Check if network client profile exists.
    for (size_t i = 0; i < wiFiRouter->numEdits; i++) {
        if (wiFiRouter->edits[i].identifier == clientIdentifier) {
            // Check if network client profile is being deleted.
            if (!wiFiRouter->edits[i].groupIdentifier) {
                HAPLog(&logObject, "Network client profile %lu not found.", (unsigned long) clientIdentifier);
                return kHAPError_InvalidState;
            }

            *editIndex = i;
            return kHAPError_None;
        }
    }
    for (size_t i = 0; i < wiFiRouter->numClients; i++) {
        if (wiFiRouter->clients[i].identifier == clientIdentifier) {
            *editIndex = wiFiRouter->numEdits;
            if (*editIndex >= HAPArrayCount(wiFiRouter->edits)) {
                HAPLog(&logObject, "Maximum number of pending configuration changes reached.");
                return kHAPError_OutOfResources;
            }
            wiFiRouter->numEdits++;
            HAPRawBufferCopyBytes(
                    &wiFiRouter->edits[*editIndex], &wiFiRouter->clients[i], sizeof wiFiRouter->clients[0]);
            return kHAPError_None;
        }
    }

    HAPLog(&logObject, "Network client profile %lu not found.", (unsigned long) clientIdentifier);
    return kHAPError_InvalidState;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterRemoveClient(
        HAPPlatformWiFiRouterRef _Nonnull wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wiFiRouter->exclusiveAccessRefCount);
    HAPPrecondition(wiFiRouter->isModifyingConfiguration);
    HAPPrecondition(!wiFiRouter->enumerationRefCount);
    HAPPrecondition(clientIdentifier);

    HAPError err;

    size_t editIndex;
    err = HAPPlatformWiFiRouterClientEdit(wiFiRouter, clientIdentifier, &editIndex);
    if (err == kHAPError_InvalidState) {
        HAPLogInfo(&logObject, "Network client profile %lu already deleted.", (unsigned long) clientIdentifier);
        return kHAPError_None;
    }
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    HAPRawBufferZero(&wiFiRouter->edits[editIndex], sizeof wiFiRouter->edits[editIndex]);
    wiFiRouter->edits[editIndex].identifier = clientIdentifier;
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientGetGroupIdentifier(
        HAPPlatformWiFiRouterRef _Nonnull wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterGroupIdentifier* _Nonnull groupIdentifier) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wiFiRouter->sharedAccessRefCount || wiFiRouter->exclusiveAccessRefCount);
    HAPPrecondition(clientIdentifier);
    HAPPrecondition(groupIdentifier);

    HAPError err;

    HAPPlatformWiFiRouterClient* client;
    err = HAPPlatformWiFiRouterClientFind(wiFiRouter, clientIdentifier, &client);
    if (err) {
        HAPAssert(err == kHAPError_InvalidState);
        return err;
    }

    *groupIdentifier = client->groupIdentifier;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientSetGroupIdentifier(
        HAPPlatformWiFiRouterRef _Nonnull wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterGroupIdentifier groupIdentifier) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wiFiRouter->exclusiveAccessRefCount);
    HAPPrecondition(wiFiRouter->isModifyingConfiguration);
    HAPPrecondition(!wiFiRouter->enumerationRefCount);
    HAPPrecondition(clientIdentifier);
    HAPPrecondition(groupIdentifier);

    HAPError err;

    // Validate network client group identifier.
    bool foundGroup = false;
    for (size_t i = 0; i < wiFiRouter->numGroups; i++) {
        if (wiFiRouter->groups[i].identifier == groupIdentifier) {
            foundGroup = true;
            break;
        }
    }
    if (!foundGroup) {
        HAPLog(&logObject, "Network client group %lu not found.", (unsigned long) groupIdentifier);
        return kHAPError_InvalidData;
    }

    size_t editIndex;
    err = HAPPlatformWiFiRouterClientEdit(wiFiRouter, clientIdentifier, &editIndex);
    if (err) {
        HAPAssert(err == kHAPError_InvalidState || err == kHAPError_OutOfResources);
        return err;
    }

    wiFiRouter->edits[editIndex].groupIdentifier = groupIdentifier;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientGetCredentialType(
        HAPPlatformWiFiRouterRef _Nonnull wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterCredentialType* _Nonnull credentialType) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wiFiRouter->sharedAccessRefCount || wiFiRouter->exclusiveAccessRefCount);
    HAPPrecondition(clientIdentifier);
    HAPPrecondition(credentialType);

    HAPError err;

    HAPPlatformWiFiRouterClient* client;
    err = HAPPlatformWiFiRouterClientFind(wiFiRouter, clientIdentifier, &client);
    if (err) {
        HAPAssert(err == kHAPError_InvalidState);
        return err;
    }

    *credentialType = client->credential.type;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientGetMACAddressCredential(
        HAPPlatformWiFiRouterRef _Nonnull wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPMACAddress* _Nonnull credential) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wiFiRouter->sharedAccessRefCount || wiFiRouter->exclusiveAccessRefCount);
    HAPPrecondition(clientIdentifier);
    HAPPrecondition(credential);

    HAPError err;

    HAPPlatformWiFiRouterClient* client;
    err = HAPPlatformWiFiRouterClientFind(wiFiRouter, clientIdentifier, &client);
    if (err) {
        HAPAssert(err == kHAPError_InvalidState);
        return err;
    }
    if (client->credential.type != kHAPPlatformWiFiRouterCredentialType_MACAddress) {
        HAPLog(&logObject, "%lu is not a MAC address based network client profile.", (unsigned long) clientIdentifier);
        return kHAPError_InvalidState;
    }

    *credential = client->credential._.macAddress;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientSetCredential(
        HAPPlatformWiFiRouterRef _Nonnull wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        const HAPPlatformWiFiRouterClientCredential* _Nonnull credential) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wiFiRouter->exclusiveAccessRefCount);
    HAPPrecondition(wiFiRouter->isModifyingConfiguration);
    HAPPrecondition(!wiFiRouter->enumerationRefCount);
    HAPPrecondition(clientIdentifier);
    HAPPrecondition(credential);

    HAPError err;

    // Validate credential.
    bool exists = false;
    for (size_t i = 0; i < wiFiRouter->numClients; i++) {
        if (wiFiRouter->clients[i].identifier == clientIdentifier) {
            continue;
        }
        if (!HAPPlatformWiFiRouterClientCredentialAreEqual(credential, &wiFiRouter->clients[i].credential)) {
            continue;
        }

        exists = true;

        // Check if this network client profile is being deleted.
        for (size_t j = 0; j < wiFiRouter->numEdits; j++) {
            if (wiFiRouter->edits[j].identifier != wiFiRouter->clients[i].identifier) {
                continue;
            }

            // If network client profile is being removed, the credential is available again.
            if (!wiFiRouter->edits[j].groupIdentifier) {
                exists = false;
                break;
            }
        }
        break;
    }
    if (!exists) {
        for (size_t i = 0; i < wiFiRouter->numEdits; i++) {
            if (wiFiRouter->edits[i].identifier == clientIdentifier) {
                continue;
            }
            if (!wiFiRouter->edits[i].groupIdentifier) {
                continue;
            }

            // If same credential is already used in a pending add / update, it is unavailable.
            if (HAPPlatformWiFiRouterClientCredentialAreEqual(credential, &wiFiRouter->clients[i].credential)) {
                exists = true;
                break;
            }
        }
    }
    if (exists) {
        HAPLog(&logObject, "Credential already exists.");
        return kHAPError_NotAuthorized;
    }

    size_t editIndex;
    err = HAPPlatformWiFiRouterClientEdit(wiFiRouter, clientIdentifier, &editIndex);
    if (err) {
        HAPAssert(err == kHAPError_InvalidState || err == kHAPError_OutOfResources);
        return err;
    }

    wiFiRouter->edits[editIndex].credential = *credential;
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientGetWANFirewallType(
        HAPPlatformWiFiRouterRef _Nonnull wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterFirewallType* _Nonnull firewallType) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wiFiRouter->sharedAccessRefCount || wiFiRouter->exclusiveAccessRefCount);
    HAPPrecondition(clientIdentifier);
    HAPPrecondition(firewallType);

    HAPError err;

    HAPPlatformWiFiRouterClient* client;
    err = HAPPlatformWiFiRouterClientFind(wiFiRouter, clientIdentifier, &client);
    if (err) {
        HAPAssert(err == kHAPError_InvalidState);
        return err;
    }

    *firewallType = client->wanFirewall.type;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientEnumerateWANFirewallRules(
        HAPPlatformWiFiRouterRef _Nonnull wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterClientEnumerateWANFirewallRulesCallback _Nonnull callback,
        void* _Nullable context) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wiFiRouter->sharedAccessRefCount || wiFiRouter->exclusiveAccessRefCount);
    HAPPrecondition(clientIdentifier);
    HAPPrecondition(callback);

    HAPError err;

    wiFiRouter->enumerationRefCount++;
    HAPAssert(wiFiRouter->enumerationRefCount);

    HAPPlatformWiFiRouterClient* client;
    err = HAPPlatformWiFiRouterClientFind(wiFiRouter, clientIdentifier, &client);
    if (err) {
        HAPAssert(err == kHAPError_InvalidState);
        return err;
    }

    bool shouldContinue = true;
    for (size_t i = 0; shouldContinue && i < client->wanFirewall.numRules; i++) {
        callback(context, wiFiRouter, clientIdentifier, &client->wanFirewall.rules[i], &shouldContinue);
    }

    HAPAssert(wiFiRouter->enumerationRefCount);
    wiFiRouter->enumerationRefCount--;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientResetWANFirewall(
        HAPPlatformWiFiRouterRef _Nonnull wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterFirewallType firewallType) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wiFiRouter->exclusiveAccessRefCount);
    HAPPrecondition(wiFiRouter->isModifyingConfiguration);
    HAPPrecondition(!wiFiRouter->enumerationRefCount);
    HAPPrecondition(!wiFiRouter->isEditingWANFirewall);
    HAPPrecondition(clientIdentifier);
    HAPPrecondition(firewallType);

    HAPError err;

    size_t editIndex;
    err = HAPPlatformWiFiRouterClientEdit(wiFiRouter, clientIdentifier, &editIndex);
    if (err) {
        HAPAssert(err == kHAPError_InvalidState || err == kHAPError_OutOfResources);
        return err;
    }

    HAPRawBufferZero(&wiFiRouter->edits[editIndex].wanFirewall, sizeof wiFiRouter->edits[editIndex].wanFirewall);
    wiFiRouter->edits[editIndex].wanFirewall.type = firewallType;
    if (firewallType == kHAPPlatformWiFiRouterFirewallType_Allowlist) {
        wiFiRouter->isEditingWANFirewall = true;
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientAddWANFirewallRule(
        HAPPlatformWiFiRouterRef _Nonnull wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        const HAPPlatformWiFiRouterWANFirewallRule* _Nonnull firewallRule_) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wiFiRouter->exclusiveAccessRefCount);
    HAPPrecondition(wiFiRouter->isModifyingConfiguration);
    HAPPrecondition(!wiFiRouter->enumerationRefCount);
    HAPPrecondition(wiFiRouter->isEditingWANFirewall);
    HAPPrecondition(clientIdentifier);
    HAPPrecondition(firewallRule_);

    HAPError err;

    size_t editIndex;
    err = HAPPlatformWiFiRouterClientEdit(wiFiRouter, clientIdentifier, &editIndex);
    if (err) {
        HAPAssert(err == kHAPError_InvalidState || err == kHAPError_OutOfResources);
        return err;
    }

    HAPPlatformWiFiRouterClient* edit = &wiFiRouter->edits[editIndex];
    if (edit->wanFirewall.numRules >= HAPArrayCount(edit->wanFirewall.rules)) {
        HAPLog(&logObject, "Maximum number of WAN firewall rules per network client profile reached.");
        return kHAPError_OutOfResources;
    }
    switch (*(const HAPPlatformWiFiRouterWANFirewallRuleType*) firewallRule_) {
        case kHAPPlatformWiFiRouterWANFirewallRuleType_Port: {
            const HAPPlatformWiFiRouterPortWANFirewallRule* firewallRule = firewallRule_;
            edit->wanFirewall.rules[edit->wanFirewall.numRules].port._ = *firewallRule;

            // Copy variable-length Host DNS Name Pattern to separate storage.
            if (firewallRule->host.type == kHAPPlatformWiFiRouterWANHostURIType_DNSNamePattern) {
                size_t numDNSNamePatternBytes = HAPStringGetNumBytes(firewallRule->host._.dnsNamePattern);
                if (numDNSNamePatternBytes >= sizeof edit->wanFirewall.rules[edit->wanFirewall.numRules].port.bytes) {
                    HAPLog(&logObject, "Host DNS Name Pattern too long (%zu bytes).", numDNSNamePatternBytes);
                    return kHAPError_InvalidData;
                }
                HAPRawBufferCopyBytes(
                        edit->wanFirewall.rules[edit->wanFirewall.numRules].port.bytes,
                        firewallRule->host._.dnsNamePattern,
                        numDNSNamePatternBytes + 1);
                edit->wanFirewall.rules[edit->wanFirewall.numRules].port._.host._.dnsNamePattern =
                        edit->wanFirewall.rules[edit->wanFirewall.numRules].port.bytes;
            }
            break;
        }
        case kHAPPlatformWiFiRouterWANFirewallRuleType_ICMP: {
            const HAPPlatformWiFiRouterICMPWANFirewallRule* firewallRule = firewallRule_;
            edit->wanFirewall.rules[edit->wanFirewall.numRules].icmp._ = *firewallRule;

            // Copy variable-length Host DNS Name Pattern to separate storage.
            if (firewallRule->host.type == kHAPPlatformWiFiRouterWANHostURIType_DNSNamePattern) {
                size_t numDNSNamePatternBytes = HAPStringGetNumBytes(firewallRule->host._.dnsNamePattern);
                if (numDNSNamePatternBytes >= sizeof edit->wanFirewall.rules[edit->wanFirewall.numRules].icmp.bytes) {
                    HAPLog(&logObject, "Host DNS Name Pattern too long (%zu bytes).", numDNSNamePatternBytes);
                    return kHAPError_InvalidData;
                }
                HAPRawBufferCopyBytes(
                        edit->wanFirewall.rules[edit->wanFirewall.numRules].icmp.bytes,
                        firewallRule->host._.dnsNamePattern,
                        numDNSNamePatternBytes + 1);
                edit->wanFirewall.rules[edit->wanFirewall.numRules].icmp._.host._.dnsNamePattern =
                        edit->wanFirewall.rules[edit->wanFirewall.numRules].icmp.bytes;
            }
            break;
        }
    }

    edit->wanFirewall.numRules++;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientFinalizeWANFirewallRules(
        HAPPlatformWiFiRouterRef _Nonnull wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wiFiRouter->exclusiveAccessRefCount);
    HAPPrecondition(wiFiRouter->isModifyingConfiguration);
    HAPPrecondition(!wiFiRouter->enumerationRefCount);
    HAPPrecondition(wiFiRouter->isEditingWANFirewall);
    HAPPrecondition(clientIdentifier);

    wiFiRouter->isEditingWANFirewall = false;
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientGetLANFirewallType(
        HAPPlatformWiFiRouterRef _Nonnull wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterFirewallType* _Nonnull firewallType) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wiFiRouter->sharedAccessRefCount || wiFiRouter->exclusiveAccessRefCount);
    HAPPrecondition(clientIdentifier);
    HAPPrecondition(firewallType);

    HAPError err;

    HAPPlatformWiFiRouterClient* client;
    err = HAPPlatformWiFiRouterClientFind(wiFiRouter, clientIdentifier, &client);
    if (err) {
        HAPAssert(err == kHAPError_InvalidState);
        return err;
    }

    *firewallType = client->lanFirewall.type;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientEnumerateLANFirewallRules(
        HAPPlatformWiFiRouterRef _Nonnull wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterClientEnumerateLANFirewallRulesCallback _Nonnull callback,
        void* _Nullable context) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wiFiRouter->sharedAccessRefCount || wiFiRouter->exclusiveAccessRefCount);
    HAPPrecondition(clientIdentifier);
    HAPPrecondition(callback);

    HAPError err;

    wiFiRouter->enumerationRefCount++;
    HAPAssert(wiFiRouter->enumerationRefCount);

    HAPPlatformWiFiRouterClient* client;
    err = HAPPlatformWiFiRouterClientFind(wiFiRouter, clientIdentifier, &client);
    if (err) {
        HAPAssert(err == kHAPError_InvalidState);
        return err;
    }

    bool shouldContinue = true;
    for (size_t i = 0; shouldContinue && i < client->lanFirewall.numRules; i++) {
        callback(context, wiFiRouter, clientIdentifier, &client->lanFirewall.rules[i], &shouldContinue);
    }

    HAPAssert(wiFiRouter->enumerationRefCount);
    wiFiRouter->enumerationRefCount--;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientResetLANFirewall(
        HAPPlatformWiFiRouterRef _Nonnull wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterFirewallType firewallType) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wiFiRouter->exclusiveAccessRefCount);
    HAPPrecondition(wiFiRouter->isModifyingConfiguration);
    HAPPrecondition(!wiFiRouter->enumerationRefCount);
    HAPPrecondition(!wiFiRouter->isEditingLANFirewall);
    HAPPrecondition(clientIdentifier);
    HAPPrecondition(firewallType);

    HAPError err;

    size_t editIndex;
    err = HAPPlatformWiFiRouterClientEdit(wiFiRouter, clientIdentifier, &editIndex);
    if (err) {
        HAPAssert(err == kHAPError_InvalidState || err == kHAPError_OutOfResources);
        return err;
    }

    HAPRawBufferZero(&wiFiRouter->edits[editIndex].lanFirewall, sizeof wiFiRouter->edits[editIndex].lanFirewall);
    wiFiRouter->edits[editIndex].lanFirewall.type = firewallType;
    if (firewallType == kHAPPlatformWiFiRouterFirewallType_Allowlist) {
        wiFiRouter->isEditingLANFirewall = true;
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientAddLANFirewallRule(
        HAPPlatformWiFiRouterRef _Nonnull wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        const HAPPlatformWiFiRouterLANFirewallRule* _Nonnull firewallRule_) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wiFiRouter->exclusiveAccessRefCount);
    HAPPrecondition(wiFiRouter->isModifyingConfiguration);
    HAPPrecondition(!wiFiRouter->enumerationRefCount);
    HAPPrecondition(wiFiRouter->isEditingLANFirewall);
    HAPPrecondition(clientIdentifier);
    HAPPrecondition(firewallRule_);

    HAPError err;

    size_t editIndex;
    err = HAPPlatformWiFiRouterClientEdit(wiFiRouter, clientIdentifier, &editIndex);
    if (err) {
        HAPAssert(err == kHAPError_InvalidState || err == kHAPError_OutOfResources);
        return err;
    }

    HAPPlatformWiFiRouterClient* edit = &wiFiRouter->edits[editIndex];
    if (edit->lanFirewall.numRules >= HAPArrayCount(edit->lanFirewall.rules)) {
        HAPLog(&logObject, "Maximum number of LAN firewall rules per network client profile reached.");
        return kHAPError_OutOfResources;
    }
    switch (*(const HAPPlatformWiFiRouterLANFirewallRuleType*) firewallRule_) {
        case kHAPPlatformWiFiRouterLANFirewallRuleType_MulticastBridging: {
            const HAPPlatformWiFiRouterMulticastBridgingLANFirewallRule* firewallRule = firewallRule_;
            edit->lanFirewall.rules[edit->lanFirewall.numRules].multicastBridging = *firewallRule;
            break;
        }
        case kHAPPlatformWiFiRouterLANFirewallRuleType_StaticPort: {
            const HAPPlatformWiFiRouterStaticPortLANFirewallRule* firewallRule = firewallRule_;
            edit->lanFirewall.rules[edit->lanFirewall.numRules].staticPort = *firewallRule;
            break;
        }
        case kHAPPlatformWiFiRouterLANFirewallRuleType_DynamicPort: {
            const HAPPlatformWiFiRouterDynamicPortLANFirewallRule* firewallRule = firewallRule_;
            edit->lanFirewall.rules[edit->lanFirewall.numRules].dynamicPort._ = *firewallRule;

            // Copy variable-length service type to separate storage.
            char* serviceTypeStorage =
                    edit->lanFirewall.rules[edit->lanFirewall.numRules].dynamicPort.serviceTypeStorage;
            size_t maxServiceTypeBytes =
                    sizeof edit->lanFirewall.rules[edit->lanFirewall.numRules].dynamicPort.serviceTypeStorage - 1;
            switch (firewallRule->serviceType.advertisementProtocol) {
                case kHAPPlatformWiFiRouterDynamicPortAdvertisementProtocol_DNSSD: {
                    if (firewallRule->serviceType._.dns_sd.serviceType) {
                        const char* serviceType = HAPNonnull(firewallRule->serviceType._.dns_sd.serviceType);
                        size_t numServiceTypeBytes = HAPStringGetNumBytes(serviceType);
                        HAPAssert(numServiceTypeBytes <= maxServiceTypeBytes);
                        HAPRawBufferCopyBytes(serviceTypeStorage, serviceType, numServiceTypeBytes);
                        edit->lanFirewall.rules[edit->lanFirewall.numRules].dynamicPort.serviceTypeStorageInUse = true;
                        edit->lanFirewall.rules[edit->lanFirewall.numRules]
                                .dynamicPort._.serviceType._.dns_sd.serviceType = serviceTypeStorage;
                    }
                    break;
                }
                case kHAPPlatformWiFiRouterDynamicPortAdvertisementProtocol_SSDP: {
                    if (firewallRule->serviceType._.ssdp.serviceTypeURI) {
                        const char* serviceTypeURI = HAPNonnull(firewallRule->serviceType._.ssdp.serviceTypeURI);
                        size_t numServiceTypeURIBytes = HAPStringGetNumBytes(serviceTypeURI);
                        if (numServiceTypeURIBytes > maxServiceTypeBytes) {
                            HAPLog(&logObject, "SSDP service type URI too long.");
                            return kHAPError_OutOfResources;
                        }
                        HAPRawBufferCopyBytes(serviceTypeStorage, serviceTypeURI, numServiceTypeURIBytes);
                        edit->lanFirewall.rules[edit->lanFirewall.numRules].dynamicPort.serviceTypeStorageInUse = true;
                        edit->lanFirewall.rules[edit->lanFirewall.numRules]
                                .dynamicPort._.serviceType._.ssdp.serviceTypeURI = serviceTypeStorage;
                    }
                    break;
                }
            }
            break;
        }
        case kHAPPlatformWiFiRouterLANFirewallRuleType_StaticICMP: {
            const HAPPlatformWiFiRouterStaticICMPLANFirewallRule* firewallRule = firewallRule_;
            edit->lanFirewall.rules[edit->lanFirewall.numRules].staticICMP = *firewallRule;
            break;
        }
    }

    edit->lanFirewall.numRules++;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientFinalizeLANFirewallRules(
        HAPPlatformWiFiRouterRef _Nonnull wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wiFiRouter->exclusiveAccessRefCount);
    HAPPrecondition(wiFiRouter->isModifyingConfiguration);
    HAPPrecondition(!wiFiRouter->enumerationRefCount);
    HAPPrecondition(wiFiRouter->isEditingLANFirewall);
    HAPPrecondition(clientIdentifier);

    wiFiRouter->isEditingLANFirewall = false;
    return kHAPError_None;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HAP_RESULT_USE_CHECK
static bool ShouldIncludeClientStatusForIdentifiers(
        const HAPPlatformWiFiRouterClientStatus* _Nonnull clientStatus,
        const HAPPlatformWiFiRouterClientStatusIdentifier* _Nonnull statusIdentifiers,
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

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterGetClientStatusForIdentifiers(
        HAPPlatformWiFiRouterRef _Nonnull wiFiRouter,
        const HAPPlatformWiFiRouterClientStatusIdentifier* _Nonnull statusIdentifiers,
        size_t numStatusIdentifiers,
        HAPPlatformWiFiRouterGetClientStatusForIdentifiersCallback _Nonnull callback,
        void* _Nullable context) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wiFiRouter->sharedAccessRefCount || wiFiRouter->exclusiveAccessRefCount);
    HAPPrecondition(!wiFiRouter->isModifyingConfiguration);
    HAPPrecondition(statusIdentifiers);
    HAPPrecondition(callback);

    wiFiRouter->enumerationRefCount++;
    HAPAssert(wiFiRouter->enumerationRefCount);

    bool shouldContinue = true;
    for (size_t i = 0; shouldContinue && i < HAPArrayCount(wiFiRouter->connections); i++) {
        if (!wiFiRouter->connections[i].isActive) {
            continue;
        }

        HAPPlatformWiFiRouterClientStatus clientStatus = {
            .clientIdentifier = wiFiRouter->connections[i].clientIdentifier,
            .macAddress = &wiFiRouter->connections[i].macAddress,
            .ipAddresses = wiFiRouter->connections[i].ipAddresses,
            .numIPAddresses = wiFiRouter->connections[i].numIPAddresses,
            .name = HAPStringGetNumBytes(wiFiRouter->connections[i].name) ? wiFiRouter->connections[i].name : NULL,
            .rssi = { .isDefined = false }
        };

        // Generate RSSI.
        clientStatus.rssi.isDefined = true;
        do {
            HAPPlatformRandomNumberFill(&clientStatus.rssi.value, sizeof clientStatus.rssi.value);
        } while (clientStatus.rssi.value > 0);

        if (!ShouldIncludeClientStatusForIdentifiers(&clientStatus, statusIdentifiers, numStatusIdentifiers)) {
            continue;
        }

        callback(context, wiFiRouter, &clientStatus, &shouldContinue);
    }

    HAPAssert(wiFiRouter->enumerationRefCount);
    wiFiRouter->enumerationRefCount--;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterConnectClient(
        HAPPlatformWiFiRouterRef _Nonnull wiFiRouter,
        const HAPWiFiWPAPersonalCredential* _Nullable credential,
        const HAPMACAddress* _Nonnull macAddress,
        const char* _Nullable name,
        HAPIPAddress* _Nullable ipAddress) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterIsConfigurationAccessAvailable(wiFiRouter));
    HAPPrecondition(macAddress);

    HAPPlatformWiFiRouterGroupIdentifier groupIdentifier = 0;
    HAPPlatformWiFiRouterClientIdentifier clientIdentifier = 0;

    groupIdentifier = kHAPPlatformWiFiRouterGroupIdentifier_Main;
    if (credential) {
        // Check if a matching network client profile configuration exists.
        for (size_t j = 0; !clientIdentifier && j < wiFiRouter->numClients; j++) {
            if (wiFiRouter->clients[j].credential.type != kHAPPlatformWiFiRouterCredentialType_PSK) {
                continue;
            }

            if (HAPWiFiWPAPersonalCredentialAreEqual(&wiFiRouter->clients[j].credential._.psk, credential)) {
                clientIdentifier = wiFiRouter->clients[j].identifier;
                groupIdentifier = wiFiRouter->clients[j].groupIdentifier;
            }
        }
    }
    if (!clientIdentifier) {
        // Check if a network client profile configuration based on MAC address exists.
        for (size_t j = 0; !clientIdentifier && j < wiFiRouter->numClients; j++) {
            if (wiFiRouter->clients[j].credential.type != kHAPPlatformWiFiRouterCredentialType_MACAddress) {
                continue;
            }

            if (HAPRawBufferAreEqual(
                        wiFiRouter->clients[j].credential._.macAddress.bytes,
                        macAddress->bytes,
                        sizeof macAddress->bytes)) {
                clientIdentifier = wiFiRouter->clients[j].identifier;
                groupIdentifier = wiFiRouter->clients[j].groupIdentifier;
            }
        }
    }

    // Add connection.
    for (size_t i = 0; i < HAPArrayCount(wiFiRouter->connections); i++) {
        if (wiFiRouter->connections[i].isActive) {
            if (HAPRawBufferAreEqual(
                        wiFiRouter->connections[i].macAddress.bytes, macAddress->bytes, sizeof macAddress->bytes)) {
                HAPLog(&logObject, "Network client with the given MAC address is already connected.");
                return kHAPError_InvalidState;
            }

            continue;
        }

        // Basic network client information.
        wiFiRouter->connections[i].isActive = true;
        wiFiRouter->connections[i].clientIdentifier = clientIdentifier;
        wiFiRouter->connections[i].macAddress = *macAddress;

        // Assign IP addresses.
        size_t numIPAddresses = 0;
        HAPAssert(numIPAddresses < HAPArrayCount(wiFiRouter->connections[i].ipAddresses));
        wiFiRouter->connections[i].ipAddresses[numIPAddresses].version = kHAPIPAddressVersion_IPv4;
        HAPAssert(i <= UINT8_MAX - 2);
        if (groupIdentifier == kHAPPlatformWiFiRouterGroupIdentifier_Restricted) {
            wiFiRouter->connections[i].ipAddresses[numIPAddresses]._.ipv4.bytes[0] = 10;
            wiFiRouter->connections[i].ipAddresses[numIPAddresses]._.ipv4.bytes[1] = 0;
            wiFiRouter->connections[i].ipAddresses[numIPAddresses]._.ipv4.bytes[2] = 1;
            wiFiRouter->connections[i].ipAddresses[numIPAddresses]._.ipv4.bytes[3] = (uint8_t)(i + 3);
        } else {
            wiFiRouter->connections[i].ipAddresses[numIPAddresses]._.ipv4.bytes[0] = 192;
            wiFiRouter->connections[i].ipAddresses[numIPAddresses]._.ipv4.bytes[1] = 168;
            wiFiRouter->connections[i].ipAddresses[numIPAddresses]._.ipv4.bytes[2] = 1;
            wiFiRouter->connections[i].ipAddresses[numIPAddresses]._.ipv4.bytes[3] = (uint8_t)(i + 3);
        }
        numIPAddresses++;
        wiFiRouter->connections[i].numIPAddresses = numIPAddresses;
        if (ipAddress) {
            HAPRawBufferCopyBytes(HAPNonnull(ipAddress), &wiFiRouter->connections[i].ipAddresses[0], sizeof *ipAddress);
        }

        // Network client name.
        if (name) {
            size_t numNameBytes = HAPStringGetNumBytes(HAPNonnull(name));
            if (numNameBytes >= sizeof wiFiRouter->connections[i].name) {
                HAPLog(&logObject, "Network client has too long name. Not keeping track of name.");
            } else {
                HAPRawBufferCopyBytes(wiFiRouter->connections[i].name, HAPNonnull(name), numNameBytes + 1);
            }
        }

        HAPLogInfo(
                &logObject,
                "Network client %02X:%02X:%02X:%02X:%02X:%02X connected (network client identifier %lu).",
                macAddress->bytes[0],
                macAddress->bytes[1],
                macAddress->bytes[2],
                macAddress->bytes[3],
                macAddress->bytes[4],
                macAddress->bytes[5],
                (unsigned long) clientIdentifier);
        return kHAPError_None;
    }

    HAPLog(&logObject, "Reached limit of concurrently connected clients.");
    return kHAPError_OutOfResources;
}

void HAPPlatformWiFiRouterDisconnectClient(
        HAPPlatformWiFiRouterRef _Nonnull wiFiRouter,
        const HAPMACAddress* _Nonnull macAddress) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(macAddress);

    for (size_t i = 0; i < HAPArrayCount(wiFiRouter->connections); i++) {
        if (!wiFiRouter->connections[i].isActive) {
            continue;
        }
        if (!HAPRawBufferAreEqual(
                    wiFiRouter->connections[i].macAddress.bytes, macAddress->bytes, sizeof macAddress->bytes)) {
            continue;
        }

        HAPLogInfo(
                &logObject,
                "Network client %02X:%02X:%02X:%02X:%02X:%02X disconnected.",
                macAddress->bytes[0],
                macAddress->bytes[1],
                macAddress->bytes[2],
                macAddress->bytes[3],
                macAddress->bytes[4],
                macAddress->bytes[5]);
        HAPRawBufferZero(&wiFiRouter->connections[i], sizeof wiFiRouter->connections[i]);
        return;
    }

    HAPLog(&logObject,
           "Network client %02X:%02X:%02X:%02X:%02X:%02X was not found in connection list.",
           macAddress->bytes[0],
           macAddress->bytes[1],
           macAddress->bytes[2],
           macAddress->bytes[3],
           macAddress->bytes[4],
           macAddress->bytes[5]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientGetAccessViolationMetadata(
        HAPPlatformWiFiRouterRef _Nonnull wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterAccessViolationMetadata* _Nonnull violationMetadata) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wiFiRouter->sharedAccessRefCount || wiFiRouter->exclusiveAccessRefCount);
    HAPPrecondition(!wiFiRouter->isModifyingConfiguration);
    HAPPrecondition(clientIdentifier);
    HAPPrecondition(violationMetadata);

    for (size_t i = 0; i < wiFiRouter->numClients; i++) {
        if (wiFiRouter->clients[i].identifier == clientIdentifier) {
            *violationMetadata = wiFiRouter->accessViolations[i].violation;
            return kHAPError_None;
        }
    }

    HAPLog(&logObject, "Network client profile %lu not found.", (unsigned long) clientIdentifier);
    return kHAPError_InvalidState;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientResetAccessViolations(
        HAPPlatformWiFiRouterRef _Nonnull wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wiFiRouter->exclusiveAccessRefCount);
    HAPPrecondition(!wiFiRouter->isModifyingConfiguration);
    HAPPrecondition(!wiFiRouter->enumerationRefCount);
    HAPPrecondition(clientIdentifier);

    size_t i;
    for (i = 0; i < wiFiRouter->numClients; i++) {
        if (wiFiRouter->clients[i].identifier == clientIdentifier) {
            break;
        }
    }
    if (i == wiFiRouter->numClients) {
        HAPLog(&logObject, "Network client profile %lu not found.", (unsigned long) clientIdentifier);
        return kHAPError_None;
    }
    HAPAssert(i < HAPArrayCount(wiFiRouter->accessViolations));

    if (wiFiRouter->accessViolations[i].violation.hasViolations) {
        wiFiRouter->accessViolations[i].violation.lastResetTimestamp =
                wiFiRouter->accessViolations[i].violation.lastViolationTimestamp;
    } else {
        wiFiRouter->accessViolations[i].violation.lastResetTimestamp = 0;
    }
    wiFiRouter->accessViolations[i].violation.wasReset = true;
    wiFiRouter->accessViolations[i].violation.lastViolationTimestamp = 0;
    wiFiRouter->accessViolations[i].violation.hasViolations = false;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientRecordAccessViolation(
        HAPPlatformWiFiRouterRef _Nonnull wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterTimestamp timestamp) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(clientIdentifier);

    size_t i;
    for (i = 0; i < wiFiRouter->numClients; i++) {
        if (wiFiRouter->clients[i].identifier == clientIdentifier) {
            break;
        }
    }
    if (i == wiFiRouter->numClients) {
        HAPLog(&logObject,
               "Cannot record network access violation for unknown network client profile %lu.",
               (unsigned long) clientIdentifier);
        return kHAPError_InvalidState;
    }
    HAPAssert(i < HAPArrayCount(wiFiRouter->accessViolations));

    if (wiFiRouter->accessViolations[i].violation.wasReset &&
        timestamp <= wiFiRouter->accessViolations[i].violation.lastResetTimestamp) {
        HAPLog(&logObject, "Ignoring network access violation because its timestamp occurred before the last reset.");
        return kHAPError_None;
    }

    HAPLogInfo(
            &logObject,
            "Recording network access violation for network client profile %lu (timestamp %lld).",
            (unsigned long) clientIdentifier,
            (long long) timestamp);
    bool hadViolations = wiFiRouter->accessViolations[i].violation.hasViolations;
    wiFiRouter->accessViolations[i].violation.lastViolationTimestamp = timestamp;
    wiFiRouter->accessViolations[i].violation.hasViolations = true;
    if (!hadViolations && wiFiRouter->delegate.handleAccessViolationMetadataChanged) {
        wiFiRouter->delegate.handleAccessViolationMetadataChanged(
                wiFiRouter, clientIdentifier, wiFiRouter->delegate.context);
    }
    return kHAPError_None;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HAP_RESULT_USE_CHECK
HAPPlatformWiFiRouterSatelliteStatus
        HAPPlatformWiFiRouterGetSatelliteStatus(HAPPlatformWiFiRouterRef _Nonnull wiFiRouter, size_t satelliteIndex) {
    HAPPrecondition(wiFiRouter);

    (void) satelliteIndex;
    HAPLogError(&logObject, "[NYI] %s.", __func__);
    HAPFatalError();
}
