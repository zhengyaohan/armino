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

#include "HAPPlatformThreadUtils+MDNS.h"

#include "HAP+API.h"
#include "HAPLogSubsystem.h"

#include <dns_sd.h>

#include <openthread/coap.h>
#include <openthread/dataset.h>
#include <openthread/ip6.h>
#include <openthread/server.h>
#include <openthread/thread.h>

#include "dns-msg.h"
#include "srp-adk.h"
#include "srp-api.h"
#include "srp-crypto.h"
#include "srp.h"

#include "HAP.h"
#include "HAPPlatformThreadUtils+Init.h"
#include "HAPServiceDiscoveryUtils.h"
#include "HAPThreadAccessoryServer+NetworkJoin.h"

#define TO_STRING(x)      (#x)
#define FLAG_TO_STRING(x) TO_STRING(x)

#define SERVICE_DATA_LEN_BYTES 1
#define SRP_PROXY_SERVICE_ID   0x5D

#define kServiceDiscoveryProtocol_HAP_Thread "_hap._udp"

#define kHostNameMaxLen 64

/** SRP Lease time while unpaired */
#define SRP_UNPAIRED_LEASE_TIME 30
/** Minimum SRP lease time and default lease time used for a non-sleepy device, in seconds */
#define SRP_MINIMUM_LEASE_TIME 3600
/** SRP key lease time in seconds. Set to one week. */
#define SRP_KEY_LEASE_TIME (60ul * 60 * 24 * 7)

/**
 * Macro to derive SRP lease time in seconds from a millisecond poll period.
 *
 * It is used to align the lease update frequency to the wakeup frequency in case the device
 * has a very long sleep interval beyond the minimum lease time.
 *
 * Note that this macro must not add any margin because SRP client will compute the time to
 * wakeup before the lease expiry to update the lease with its margin.
 */
#define LeaseTimeFromPollPeriod(pollPeriod) (((pollPeriod) + 999) / 1000)

static bool currentlyAdvertising = false;

static HAPPlatformTimerRef srpTimer;

#ifndef SRP_REGISTER_TIMEOUT
#define SRP_REGISTER_TIMEOUT (200 * HAPSecond)
#endif

HAPPlatformThreadNetworkLossCallback networkLossCallback = NULL;

/**
 * Called when the accessory cannot form a connection with the
 * SRP Proxy
 *
 * @param timer that expired
 * @param context context to pass to network loss callback
 */
static void SrpTimeoutHandler(HAPPlatformTimerRef timer, void* context) {
    // Clear the timer
    srpTimer = 0;
    // Give up on thread network - Report non-viability
    HAPLogError(&kHAPLog_Default, "Unable to register with SRP proxy, network is not viable");
    if (networkLossCallback) {
        networkLossCallback(context);
    }
}

/**
 * Called when SRP cannot use the hostname given due to
 * conflicts.  Not strictly necessary as srp is configured to
 * resolve conflicts itself.
 *
 * @param hostname The conflicting hostname.
 */
static void conflict_callback(const char* hostname) {
    HAPLogError(&kHAPLog_Default, "Host name conflict: %s", hostname);
}

/**
 * Determines whether an address is mesh local or not.
 */
static bool isLocalAddress(const otNetifAddress* address) {
    // If it's an RLOC it's definitely mesh local.  Can short circuit other checks quickly here
    if (address->mRloc) {
        return true;
    }

    // Check for link local addresses.
    if (address->mAddress.mFields.m8[0] == 0xFE && address->mAddress.mFields.m8[1] == 0x80) {
        return true;
    }

    // Check to see if we match the mesh prefix
    const otMeshLocalPrefix* prefix = otThreadGetMeshLocalPrefix(HAPPlatformThreadGetHandle());

    bool match = true;
    for (int i = 0; i < OT_MESH_LOCAL_PREFIX_SIZE && match; i++) {
        match = prefix->m8[i] == address->mAddress.mFields.m8[i];
    }

    return match;
}

/**
 * Function called back by the srp client once srp negotiation
 * is complete.
 */
static void serviceRegisterReply(bool success, void* context) {
    if (!success) {
        HAPLogError(&kHAPLog_Default, "%s: Service discovery registration failed", __func__);
        networkLossCallback(context);
    } else if (srpTimer) {
        HAPPlatformTimerDeregister(srpTimer);
        srpTimer = 0;
    }
}

void HAPPlatformThreadInitializeMDNS(void* server) {
    HAPPrecondition(server);
    currentlyAdvertising = false;
    HAPAccessoryServer* accServer = (HAPAccessoryServer*) server;
    srp_thread_init(HAPPlatformThreadGetHandle(), accServer->platform.keyValueStore);
    HAPPlatformServiceDiscoveryRegisterReplyCallback(serviceRegisterReply, server);
}

void HAPPlatformThreadDeinitializeMDNS(void) {
    srp_thread_shutdown(HAPPlatformThreadGetHandle());
}

void HAPPlatformThreadClearMDNSMem(void) {
    srp_thread_reset_key();
}

bool HAPPlatformThreadMDNSProxyFound(void) {
    otInstance* threadInstance = HAPPlatformThreadGetHandle();
    bool proxyFound = false;
    otServiceConfig serviceConfig;
    otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;

    while (!proxyFound && (otNetDataGetNextService(threadInstance, &iterator, &serviceConfig) == OT_ERROR_NONE)) {
        if (serviceConfig.mServiceDataLength == SERVICE_DATA_LEN_BYTES) {
            // Note: Future releases will need to check service ver against a set of supported versions
            //       This release only supports one.
            proxyFound = serviceConfig.mEnterpriseNumber == THREAD_ENTERPRISE_NUMBER &&
                         serviceConfig.mServiceData[0] == SRP_PROXY_SERVICE_ID;
        }
    }
    return proxyFound;
}

HAPError HAPPlatformThreadRefreshAdvertisement(void* server_, bool updateTxtRecord) {
    HAPPrecondition(server_);
    int srp_err;
    int serviceAddresses = 0;
    int serverAddresses = 0;

    HAPAccessoryServer* server = (HAPAccessoryServer*) server_;
    if (!currentlyAdvertising && !HAPPlatformThreadNetworkIsViable()) {
        // It isn't an error to request an advertisement refresh on a non-viable network.
        // It's just a no-op
        return kHAPError_None;
    }

    if (!server->thread.isServiceDiscoveryActive) {
        // Service discovery inactive
        HAPLogInfo(&kHAPLog_Default, "Transport service discovery function is inactive - no adv refresh.");
        return kHAPError_None;
    }

    if (server->thread.isTransportStopping) {
        // Thread transport is currently in the process of stopping
        HAPLogInfo(&kHAPLog_Default, "Transport is currently in shutdown process - no adv refresh.");
        return kHAPError_None;
    }

    if (server->thread.suppressUnpairedThreadAdvertising && !HAPAccessoryServerIsPaired(server)) {
        // Thread transport is unpaired and prevented from advertising as unpaired
        HAPLogInfo(&kHAPLog_Default, "Server is unpaired and unpaired advertisement is disabled - no adv refresh.");
        return kHAPError_None;
    }

    if (!server->thread.discoverableService) {
        // only need to do this the first time.
#if defined(THREAD_HOSTNAME) && (HAP_TESTING == 1)
        srp_err = srp_set_hostname(FLAG_TO_STRING(THREAD_HOSTNAME), conflict_callback);
        HAPAssert(srp_err == kDNSServiceErr_NoError);
#else
        char hostname[kHostNameMaxLen + 1];
        HAPRawBufferZero(hostname, sizeof(hostname));
        const char* name = server->primaryAccessory->name;

        // Replace invalid chars (such as spaces) in the host name with '-'
        for (int i = 0; i < kHostNameMaxLen && *name; i++) {
            // Hostnames are only allowed to have the following characters: 0-9 A-Z a-z . -

            if (!((*name == '.') || HAPASCIICharacterIsAlphanumeric(*name))) {
                hostname[i] = '-';
            } else {
                hostname[i] = *name;
            }
            name++;
        }
        srp_err = srp_set_hostname(hostname, conflict_callback);
        HAPAssert(srp_err == kDNSServiceErr_NoError);
#endif
    }
    // Set lease times
    uint32_t leaseTime =
            HAPMax(SRP_MINIMUM_LEASE_TIME, LeaseTimeFromPollPeriod(HAPPlatformThreadGetConfiguredPollPeriod()));
    if (!HAPAccessoryServerIsPaired(server)) {
        leaseTime = SRP_UNPAIRED_LEASE_TIME;
    }
    srp_err = srp_set_lease_times(leaseTime, SRP_KEY_LEASE_TIME);
    HAPAssert(srp_err == kDNSServiceErr_NoError);

    srp_err = srp_start_address_refresh();
    if (srp_err != kDNSServiceErr_NoError) {
        // We are already doing a refresh and cannot service this one.
        HAPLogError(&kHAPLog_Default, "Cannot refresh srp at this time");
        return kHAPError_InvalidState;
    }

    otInstance* threadInstance = HAPPlatformThreadGetHandle();

    // Walk through our addresses
    for (const otNetifAddress* addr = otIp6GetUnicastAddresses(threadInstance); addr; addr = addr->mNext) {
        if (!isLocalAddress(addr)) {
            // Add any addresses that extend beyond mesh-local scope to the interface
            // Print current address
            HAPLogDebug(
                    &kHAPLog_Default,
                    "srp_add_interface_address %04X:%04X:%04X:%04X:%04X:%04X:%04X:%04X",
                    HAPReadBigUInt16(&addr->mAddress.mFields.m8[0]),
                    HAPReadBigUInt16(&addr->mAddress.mFields.m8[2]),
                    HAPReadBigUInt16(&addr->mAddress.mFields.m8[4]),
                    HAPReadBigUInt16(&addr->mAddress.mFields.m8[6]),
                    HAPReadBigUInt16(&addr->mAddress.mFields.m8[8]),
                    HAPReadBigUInt16(&addr->mAddress.mFields.m8[10]),
                    HAPReadBigUInt16(&addr->mAddress.mFields.m8[12]),
                    HAPReadBigUInt16(&addr->mAddress.mFields.m8[14]));

            srp_err = srp_add_interface_address(dns_rrtype_aaaa, (uint8_t*) addr->mAddress.mFields.m8, 16);
            if (srp_err != kDNSServiceErr_NoError) {
                HAPLogError(&kHAPLog_Default, "ERROR srp_add_interface_address: %d", srp_err);
            } else {
                serviceAddresses++;
            }
        }
    }

    if (!currentlyAdvertising || updateTxtRecord) {
        HAPServiceDiscoveryUtilsSetHAPService(server, kServiceDiscoveryProtocol_HAP_Thread, OT_DEFAULT_COAP_PORT);
    }
    // Find the proxy
    otServiceConfig serviceConfig;
    otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;

    // Find the SRP Proxy Service
    otDeviceRole role = otThreadGetDeviceRole(threadInstance);
    while ((otNetDataGetNextService(threadInstance, &iterator, &serviceConfig) == OT_ERROR_NONE) &&
           (role != OT_DEVICE_ROLE_DETACHED) && (role != OT_DEVICE_ROLE_DISABLED)) {

        // TODO:  create a "Service Module" that can look for and parse the data of multiple different services.
        //        Currently we only support service ID 1 (srp-mdns-proxy) Ver 1 (initial protocol)
        if (serviceConfig.mServiceDataLength != SERVICE_DATA_LEN_BYTES ||
            serviceConfig.mEnterpriseNumber != THREAD_ENTERPRISE_NUMBER) {
            HAPLogInfo(
                    &kHAPLog_Default,
                    "Service Enterprise number (%lu) or length (%lu) do not match srp-mdns-proxy.  Skipping",
                    serviceConfig.mEnterpriseNumber,
                    (unsigned long) serviceConfig.mServiceDataLength);
        } else {

            HAPLog(&kHAPLog_Default,
                   "Checking service from %lu, id %d",
                   serviceConfig.mEnterpriseNumber,
                   serviceConfig.mServiceData[0]);

            if (serviceConfig.mEnterpriseNumber == THREAD_ENTERPRISE_NUMBER &&
                serviceConfig.mServiceData[0] == SRP_PROXY_SERVICE_ID) {

                // serviceConfig.mServerConfig.mServerData points to the Server data pertaining to the
                // SRP_PROXY_SERVICE_ID This data is 128 bits: Server's IPv6 address 16 bits:  Server's srp_mdns_proxy
                // port.
                if (serviceConfig.mServerConfig.mServerDataLength != OT_IP6_ADDRESS_SIZE + sizeof(uint16_t)) {
                    HAPLogError(
                            &kHAPLog_Default,
                            "Invalid srp_mdns_proxy server data length %d",
                            serviceConfig.mServerConfig.mServerDataLength);
                } else {

                    uint16_t portNum = HAPReadBigUInt16(&serviceConfig.mServerConfig.mServerData[OT_IP6_ADDRESS_SIZE]);
                    // Add the server address.
                    HAPLogInfo(
                            &kHAPLog_Default,
                            "srp_add_server_address %04X:%04X:%04X:%04X:%04X:%04X:%04X:%04X - PORT %04X",
                            HAPReadBigUInt16(&serviceConfig.mServerConfig.mServerData[0]),
                            HAPReadBigUInt16(&serviceConfig.mServerConfig.mServerData[2]),
                            HAPReadBigUInt16(&serviceConfig.mServerConfig.mServerData[4]),
                            HAPReadBigUInt16(&serviceConfig.mServerConfig.mServerData[6]),
                            HAPReadBigUInt16(&serviceConfig.mServerConfig.mServerData[8]),
                            HAPReadBigUInt16(&serviceConfig.mServerConfig.mServerData[10]),
                            HAPReadBigUInt16(&serviceConfig.mServerConfig.mServerData[12]),
                            HAPReadBigUInt16(&serviceConfig.mServerConfig.mServerData[14]),
                            portNum);

                    srp_err = srp_add_server_address(
                            (uint8_t*) &portNum,
                            dns_rrtype_aaaa,
                            serviceConfig.mServerConfig.mServerData,
                            OT_IP6_ADDRESS_SIZE);
                    if (srp_err != kDNSServiceErr_NoError) {
                        HAPLogError(&kHAPLog_Default, "ERROR srp_add_server_address: %d", srp_err);
                    }
                    serverAddresses++;
                }
            } else {
                HAPLog(&kHAPLog_Default,
                       "Not srp-mdns-proxy: Skipping service ent %ld  id %d",
                       serviceConfig.mEnterpriseNumber,
                       serviceConfig.mServiceData[0]);
            }
        }
    }

    currentlyAdvertising = (serverAddresses > 0) && (serviceAddresses > 0);
    bool srpWillUpdate = false;
    srp_err = srp_finish_address_refresh(&srpWillUpdate);

    if (srp_err != kDNSServiceErr_NoError) {
        HAPLogError(&kHAPLog_Default, "Unable to properly refresh srp address: %d", srp_err);
        networkLossCallback(server);
        return kHAPError_InvalidState;
    }
    HAPLogInfo(&kHAPLog_Default, "SRP will update = %s", srpWillUpdate ? "true" : "false");

    if (srpWillUpdate) {
        if (srpTimer) {
            // Ensure only one timer runs at a time
            HAPPlatformTimerDeregister(srpTimer);
            srpTimer = 0;
        }
        HAPError hapErr = HAPPlatformTimerRegister(
                &srpTimer, HAPPlatformClockGetCurrent() + SRP_REGISTER_TIMEOUT, SrpTimeoutHandler, server);
        HAPAssert(!hapErr);
    }

    return kHAPError_None;
}

void HAPPlatformThreadRegisterNetworkLossCallback(HAPPlatformThreadNetworkLossCallback callback) {
    networkLossCallback = callback;
}

void HAPPlatformThreadDeregisterNetworkLossCallback(HAPPlatformThreadNetworkLossCallback callback) {
    HAPPrecondition(callback);
    HAPAssert(networkLossCallback == callback);
    networkLossCallback = NULL;
}
