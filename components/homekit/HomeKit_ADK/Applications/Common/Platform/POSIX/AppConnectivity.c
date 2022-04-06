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

#include "AppConnectivity.h"
#include "ApplicationFeatures.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/socket.h>

// VENDOR-TODO: Update these network interface names to match accessory configuration
//              Can use prefix (ex: "wlan") instead of exact names (ex: "wlan0").
#ifndef WIFI_INTERFACE_NAME
#define WIFI_INTERFACE_NAME "wlan"
#endif
#ifndef ETH_INTERFACE_NAME
#define ETH_INTERFACE_NAME "eth"
#endif

static bool isInterfaceRunning(const char* _Nullable iface) {
    struct ifaddrs* ifaddr;
    bool found = false;

    // Search for any non-loopback interface with a valid IP address.
    if (getifaddrs(&ifaddr) == 0) {
        for (struct ifaddrs* ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
            if ((ifa->ifa_flags & IFF_LOOPBACK) != 0) {
                continue;
            }

            if ((iface != NULL) && !HAPStringHasPrefix(ifa->ifa_name, iface)) {
                continue;
            }

            // Check for up and running interfaces.
            if ((ifa->ifa_flags & (IFF_UP | IFF_RUNNING)) == (IFF_UP | IFF_RUNNING)) {
                // Check if any IPv4 or IPv6 address
                if (ifa->ifa_addr->sa_family == AF_INET || ifa->ifa_addr->sa_family == AF_INET6) {
                    HAPLogDebug(&kHAPLog_Default, "%s interface '%s' has IP connectivity", __func__, ifa->ifa_name);
                    found = true;
                    break;
                }
            }
        }
        freeifaddrs(ifaddr);
    }
    return found;
}

/**
 * Check if the WiFi interface is connected
 */
bool AppConnectivityIsWiFiConnected(HAPAccessoryServer* _Nullable server HAP_UNUSED) {
#if (HAVE_WAC == 1)
    // If WAC is enabled, use the wifi manager to check if the accessory
    // has IP connectivity.
    if (NULL != server && NULL != server->platform.ip.wiFi.wiFiManager) {
        bool isWiFiLinkEstablished = false;
        bool isWiFiNetworkConfigured = false;

        isWiFiLinkEstablished =
                HAPPlatformWiFiManagerIsWiFiLinkEstablished(HAPNonnull(server->platform.ip.wiFi.wiFiManager));
        isWiFiNetworkConfigured =
                HAPPlatformWiFiManagerIsWiFiNetworkConfigured(HAPNonnull(server->platform.ip.wiFi.wiFiManager));

        HAPLogDebug(
                &kHAPLog_Default,
                "%s: Wifi+WAC(link:%d, conf:%d)",
                __func__,
                isWiFiLinkEstablished,
                isWiFiNetworkConfigured);
        if (isWiFiLinkEstablished && isWiFiNetworkConfigured) {
            return true;
        }
    }
#endif

    // If WAC is not enabled, or if a wifi link was established outside of WAC,
    // check the network interface by name.
    return isInterfaceRunning(WIFI_INTERFACE_NAME);
}

/**
 * Check if the ethernet interface is connected
 */
bool AppConnectivityIsEthernetConnected() {
    return isInterfaceRunning(ETH_INTERFACE_NAME);
}

#if (HAP_TESTING == 1)
/**
 * Loop through all network interface to see if any has a valid IP address.
 */
bool AppConnectivityIsAnyIPConnected() {
    return isInterfaceRunning(NULL);
}
#endif
