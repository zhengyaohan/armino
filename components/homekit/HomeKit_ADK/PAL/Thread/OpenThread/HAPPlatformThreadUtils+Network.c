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

#include "HAPPlatformThreadUtils+Network.h"

#include "HAP+API.h"
#include "HAPLogSubsystem.h"

#include <openthread/dataset.h>
#include <openthread/netdata.h>
#include <openthread/platform/openthread-system.h>
#include <openthread/tasklet.h>
#include <openthread/thread.h>

#include "HAP.h"
#include "HAPDeviceID.h"

/**
 * Macro to compute the maximum poll period from child timeout.
 *
 * 90% of child timeout is the maximum poll period. The macro rounds the result.
 */
#define MAX_POLL_PERIOD_FROM_CHILD_TIMEOUT(childTimeout) ((((childTimeout) *9) + 5) / 10)

#define kNetDataLen 255

/**
 * Thread network scan state
 */
struct {
    /** Matching network was found */
    bool networkFound : 1;
    /** channel */
    uint32_t channel;
    /** PAN ID */
    uint32_t panId;
    /** Ext PAN ID */
    otExtendedPanId extPanId;
    /** network name */
    otNetworkName networkName;
    /** callback to make when scan is complete */
    HAPPlatformRunLoopCallback callback;
    /** Accessory server */
    HAPAccessoryServer* server;
} networkScanState;

/**
 * Data to pass to the RunLoopHandleScanResult()
 */
typedef struct {
    /** Call was made because active scan completed */
    bool completed : 1;
    /** Call was made because a matching network was found */
    bool found : 1;
} ActiveScanResultData;

/**
 * Border router callback state.
 */
struct {
    /** border router is present */
    bool isPresent : 1;
    /** border router state callback is reported at least once since being registered */
    bool reported : 1;
} borderRouterCallbackState;
/** Border router presence callback function */
static HAPPlatformThreadBorderRouterStateCallback borderRouterCallback = NULL;
/** context to pass to the borderRouterCallback */
void* borderRouterCallbackContext = NULL;

HAPPlatformThreadDeviceRole currentRole = kHAPPlatformThreadDeviceRole_Disabled;
HAPPlatformRunLoopCallback roleUpdateCallback = NULL;

//*****************************************************************************
// Network Scanning - These functions handle scanning for thread networks that
//                    match our configuration
//*****************************************************************************

/**
 * Thread active scan callback in Platform RunLoop context
 */
static void RunLoopHandleScanResult(void* _Nullable context, size_t len) {
    HAPPrecondition(context);
    HAPPrecondition(len == sizeof(ActiveScanResultData));

    ActiveScanResultData* result = context;
    if (result->found) {
        networkScanState.networkFound = true;
    }
    if (result->completed) {
        if (networkScanState.networkFound) {
            // Network found
            HAPLogInfo(&kHAPLog_Default, "Active scan found Thread network");
        } else {
            // Network not found.
            HAPLogInfo(&kHAPLog_Default, "Active scan fails to find Thread network");
        }

        HAPAccessoryServerSetThreadNetworkReachable(networkScanState.server, networkScanState.networkFound, false);
        HAPPlatformRunLoopCallback callback = networkScanState.callback;
        networkScanState.callback = NULL;

        callback(&networkScanState.server, sizeof networkScanState.server);
    }
}

/**
 * Open Thread active scan callback
 */
static void HandleActiveScanResult(otActiveScanResult* scanResult, void* context) {
    if (!scanResult) {
        // Scan complete
        ActiveScanResultData result = { .completed = true };
        HAPError err = HAPPlatformRunLoopScheduleCallback(RunLoopHandleScanResult, &result, sizeof result);
        HAPAssert(!err);
    } else {
        // A network is found. Check if it's matching.
        if (scanResult->mPanId == networkScanState.panId &&
            HAPRawBufferAreEqual(
                    scanResult->mExtendedPanId.m8, networkScanState.extPanId.m8, sizeof networkScanState.extPanId.m8) &&
            HAPStringAreEqual(scanResult->mNetworkName.m8, networkScanState.networkName.m8)) {
            // match

            // Channel must be the correct channel because only the designated channel was scanned
            HAPAssert(scanResult->mChannel == networkScanState.channel);

            ActiveScanResultData result = { .found = true };
            HAPError err = HAPPlatformRunLoopScheduleCallback(RunLoopHandleScanResult, &result, sizeof result);
            HAPAssert(!err);
        }
    }
}

/**
 * Converts channel number to open thread channel mask
 *
 * @param channel channel number
 * @return channel mask
 */
static uint32_t ChannelToMask(uint32_t channel) {
    return (1ul << channel);
}

void HAPPlatformThreadTestNetworkReachability(
        const HAPPlatformThreadNetworkParameters* parameters,
        HAPTime testDuration,
        HAPPlatformRunLoopCallback callback,
        void* server) {
    HAPAssert(HAPPlatformThreadIsInitialized());

    // Test reachability through active scan

    networkScanState.networkFound = false;
    networkScanState.callback = callback;
    networkScanState.channel = parameters->channel;
    networkScanState.panId = parameters->panId;
    HAPRawBufferCopyBytes(networkScanState.extPanId.m8, parameters->extPanId, sizeof networkScanState.extPanId.m8);
    HAPRawBufferCopyBytes(
            networkScanState.networkName.m8, parameters->networkName, sizeof networkScanState.networkName.m8);
    networkScanState.server = server;

    HAPPlatformThreadUpdateDeviceRole(kHAPPlatformThreadDeviceRole_Joining);

    HAPLogDebug(&kHAPLog_Default, "%s: starting active scan", __func__);

    otError e = otLinkActiveScan(
            HAPPlatformThreadGetHandle(),
            ChannelToMask(parameters->channel),
            testDuration,
            HandleActiveScanResult,
            NULL);
    HAPAssert(e == OT_ERROR_NONE);
}

//*****************************************************************************
// Border Router Presence - These functions detect a border router on the
// network
//*****************************************************************************
void HAPPlatformThreadRegisterBorderRouterStateCallback(
        HAPPlatformThreadBorderRouterStateCallback callback,
        void* context) {
    HAPPrecondition(callback);
    HAPPrecondition(context);
    HAPPrecondition(!borderRouterCallback);

    borderRouterCallback = callback;
    borderRouterCallbackContext = context;
    borderRouterCallbackState.reported = false;
}

void HAPPlatformThreadDeregisterBorderRouterStateCallback(HAPPlatformThreadBorderRouterStateCallback callback) {
    HAPPrecondition(callback);
    HAPAssert(borderRouterCallback == callback);

    borderRouterCallback = NULL;
    borderRouterCallbackContext = NULL;
    borderRouterCallbackState.reported = false;
}

static void CheckBorderRouterPresence(void) {
    if (!HAPPlatformThreadInstanceIsInitialized()) {
        return;
    }

    bool previousState = borderRouterCallbackState.isPresent;
    borderRouterCallbackState.isPresent = HAPPlatformThreadNetworkIsViable();
    if (!borderRouterCallbackState.reported || borderRouterCallbackState.isPresent != previousState) {
        borderRouterCallbackState.reported = true;
        if (borderRouterCallback) {
            borderRouterCallback(borderRouterCallbackState.isPresent, borderRouterCallbackContext);
        }
    }
}

//*****************************************************************************
// Network State - These functions handle Thread network state updates and
//                 Our accessory's role in the network
//*****************************************************************************
void HAPPlatformThreadReportStateChanged(uint32_t flags, void* context) {
    char* roleStr = "unknown";
    static otDeviceRole curRole = OT_DEVICE_ROLE_DISABLED;
    otDeviceRole prevRole = curRole;
    // Check border router presence
    CheckBorderRouterPresence();
    otInstance* threadInstance = HAPPlatformThreadGetHandle();
    uint32_t configuredPollPeriod = HAPPlatformThreadGetConfiguredPollPeriod();
    curRole = otThreadGetDeviceRole(threadInstance);

    HAPLog(&kHAPLog_Default, "Thread state changed! Flags: 0x%08lx", flags);

    // Handle a role change.
    if (flags & OT_CHANGED_THREAD_ROLE) {
        switch (curRole) {
            case OT_DEVICE_ROLE_DISABLED: {
                roleStr = "disabled";
                HAPPlatformThreadUpdateDeviceRole(kHAPPlatformThreadDeviceRole_Disabled);
                break;
            }
            case OT_DEVICE_ROLE_DETACHED: {
                roleStr = "detached";
                HAPPlatformThreadUpdateDeviceRole(kHAPPlatformThreadDeviceRole_Detached);
                break;
            }
            case OT_DEVICE_ROLE_CHILD: {
                roleStr = "child";
                HAPPlatformThreadUpdateDeviceRole(kHAPPlatformThreadDeviceRole_Child);
                // Child device might have received an updated child timeout
                if (configuredPollPeriod > 0) {
                    // Device is polling.
                    // Compute poll period that is shorter than child timeout.
                    uint32_t childTimeout = otThreadGetChildTimeout(threadInstance);
                    // Convert to milliseconds and compute maximum poll period allowed by the child timeout.
                    uint64_t maxPollPeriod = MAX_POLL_PERIOD_FROM_CHILD_TIMEOUT((uint64_t) childTimeout * 1000);

                    if (configuredPollPeriod > maxPollPeriod) {
                        configuredPollPeriod = (uint32_t) maxPollPeriod;
                        HAPLogDebug(
                                &kHAPLog_Default,
                                "Poll period updated to %lu msecs to meet child timeout of %lu secs",
                                (unsigned long) configuredPollPeriod,
                                (unsigned long) childTimeout);
                        if (!HAPPlatformThreadIsWakeLocked()) {
                            // Wake lock isn't present. Hence, update the poll period immediately.
                            otLinkSetPollPeriod(threadInstance, configuredPollPeriod);
                        }
                    }
                }
                break;
            }
            case OT_DEVICE_ROLE_ROUTER: {
                roleStr = "router";
                // Note that platform that supports Border Router may update role into Border Router
                HAPPlatformThreadUpdateDeviceRole(kHAPPlatformThreadDeviceRole_Router);
                break;
            }
            case OT_DEVICE_ROLE_LEADER: {
                roleStr = "leader";
                HAPPlatformThreadUpdateDeviceRole(kHAPPlatformThreadDeviceRole_Leader);
                break;
            }
        }

        HAPLog(&kHAPLog_Default, "Thread role changed! Current role: %s.", roleStr);
    }

    // Print Accessory Network Info
    // Device ID.
    HAPDeviceIDString deviceIDString;
    HAPError err = HAPDeviceIDGetAsString(context, &deviceIDString);
    HAPAssert(!err);

    // EUI
    HAPEui64 eui;
    HAPPlatformReadEui(&eui);
    char strbuf[24];
    err = HAPEui64GetDescription(&eui, strbuf, sizeof(strbuf));
    HAPAssert(!err);
    HAPLogDebug(&kHAPLog_Default, "EUI: %s", strbuf);
    HAPLog(&kHAPLog_Default, "Device ID: %s", deviceIDString.stringValue);

    // IP addresses
    for (const otNetifAddress* addr = otIp6GetUnicastAddresses(threadInstance); addr; addr = addr->mNext) {
        HAPLogDebug(
                &kHAPLog_Default,
                "%04X:%04X:%04X:%04X:%04X:%04X:%04X:%04X",
                HAPReadBigUInt16(&addr->mAddress.mFields.m8[0]),
                HAPReadBigUInt16(&addr->mAddress.mFields.m8[2]),
                HAPReadBigUInt16(&addr->mAddress.mFields.m8[4]),
                HAPReadBigUInt16(&addr->mAddress.mFields.m8[6]),
                HAPReadBigUInt16(&addr->mAddress.mFields.m8[8]),
                HAPReadBigUInt16(&addr->mAddress.mFields.m8[10]),
                HAPReadBigUInt16(&addr->mAddress.mFields.m8[12]),
                HAPReadBigUInt16(&addr->mAddress.mFields.m8[14]));
    }

    // When the thread state changes, ensure we advertise our service.  Force an update if we went
    // from disassociated to associated.
    bool forceUpdate = (prevRole == OT_DEVICE_ROLE_DISABLED || prevRole == OT_DEVICE_ROLE_DETACHED) &&
                       (curRole != OT_DEVICE_ROLE_DISABLED && curRole != OT_DEVICE_ROLE_DETACHED);
    if (forceUpdate ||
        (flags & (OT_CHANGED_THREAD_NETDATA | OT_CHANGED_IP6_ADDRESS_ADDED | OT_CHANGED_IP6_ADDRESS_REMOVED |
                  OT_CHANGED_THREAD_LL_ADDR | OT_CHANGED_THREAD_ML_ADDR))) {
        err = HAPPlatformThreadRefreshAdvertisement(context, forceUpdate);
        HAPAssert(!err);
    }
}

void HAPPlatformThreadUpdateDeviceRole(HAPPlatformThreadDeviceRole newRole) {
    if (currentRole != newRole) {
        currentRole = newRole;
        if (roleUpdateCallback) {
            HAPError err = HAPPlatformRunLoopScheduleCallback(roleUpdateCallback, &currentRole, sizeof currentRole);
            HAPAssert(!err);
        }
    }
}

void HAPPlatformThreadUnregisterRoleUpdateCallback(void) {
    roleUpdateCallback = NULL;
}

HAPError HAPPlatformThreadGetRole(HAPPlatformThreadDeviceRole* role) {
    HAPPrecondition(role);
    *role = currentRole;
    return kHAPError_None;
}

void HAPPlatformThreadRegisterRoleUpdateCallback(HAPPlatformRunLoopCallback callback) {
    roleUpdateCallback = callback;
}

//*****************************************************************************
// Network Viability - These functions determine if the thread network is
//   'Viable'.  A network is 'Viable' if
//   * The accessory can attach to it
//   * The network has a border router
//   * The network is serving srp-mdns-proxy
//*****************************************************************************
bool HAPPlatformThreadNetworkIsViable(void) {
    bool isDetached = false;
    bool brFound = false;
    bool proxyFound = false;

    if (!HAPPlatformThreadInstanceIsInitialized()) {
        return false;
    }

    otInstance* threadInstance = HAPPlatformThreadGetHandle();

    // Make sure we are part of a thread network
    otDeviceRole deviceRole = otThreadGetDeviceRole(threadInstance);
    isDetached = (deviceRole == OT_DEVICE_ROLE_DISABLED || deviceRole == OT_DEVICE_ROLE_DETACHED);

    if (!isDetached) {
        // Find the border router
        otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
        otBorderRouterConfig config;
        otError err = otNetDataGetNextOnMeshPrefix(threadInstance, &iterator, &config);
        brFound = err == OT_ERROR_NONE;

        if (!brFound) {
            HAPLogDebug(&kHAPLog_Default, "Unable to find On Mesh Prefix.  errno %d", err);

            // Retrieving net data for debugging.
            uint8_t data[kNetDataLen];
            uint8_t dataLen = kNetDataLen;
            err = otNetDataGet(threadInstance, false, data, &dataLen);

            if (err == OT_ERROR_NONE) {
                HAPLogBufferDebug(&kHAPLog_Default, data, dataLen, "Thread Network Data");
            } else {
                HAPLogError(&kHAPLog_Default, "Failed to retrieve net data.  errno %d", err);
            }
        }

        // find SRP MDNS Proxy
        proxyFound = HAPPlatformThreadMDNSProxyFound();
    }
    HAPLogDebug(
            &kHAPLog_Default,
            "Thread network viability: detached=%d, br=%d, proxy=%d",
            isDetached,
            brFound,
            proxyFound);
    return !isDetached && brFound && proxyFound;
}

//*****************************************************************************
// Network Upkeep - These functions must be called to allow the thread network
//                  to perform its processing tasks
//*****************************************************************************
void HAPPlatformThreadTick(void) {
    otInstance* threadInstance = HAPPlatformThreadGetHandle();
    if (HAPPlatformThreadIsInitialized()) {
        otTaskletsProcess(threadInstance);
        otSysProcessDrivers(threadInstance);
    }
}
