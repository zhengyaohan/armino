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

#include "HAPPlatformFeatures.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)

#include "HAP+KeyValueStoreDomains.h"
#include "HAPAccessoryServer+Internal.h"
#include "HAPAccessorySetup.h"
#include "HAPLogSubsystem.h"
#include "HAPThreadAccessoryServer+NetworkJoin.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "ThreadAccessoryServer" };

/** Each reachability test maximum duration */
#define REACHABILITY_TEST_DURATION ((HAPTime)(3 * HAPSecond))

/** BLE capable device joiner delay */
#define JOINER_DELAY ((HAPTime)(5 * HAPSecond))

// forward reference
static void HandleJoinerTimer(HAPPlatformTimerRef timer, void* context);

/**
 * Timer handler for border router detection timer
 */
static void HandleBorderRouterDetectionTimeout(HAPPlatformTimerRef timer HAP_UNUSED, void* context) {
    HAPPrecondition(context);
    HAPAccessoryServer* server = context;

    if (server->thread.storage) {
        server->thread.storage->networkJoinState.borderRouterDetectTimer = 0;
    }

    if (!server->transports.thread) {
        // Server must have stopped.
        return;
    }

    if (!server->thread.isTransportRunning || server->thread.isTransportStopping) {
        // Transport has stopped or is stopping. Do not proceed.
        return;
    }

    HAPLogInfo(&logObject, "Border router presence wasn't detected.");
    HAPThreadAccessoryServerHandleThreadConnectivityLoss(server);
}

/**
 * Enables Thread and kicks off border router presence detection timeout
 *
 * @param server    Accessory server
 */
static void EnableThread(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    HAPPlatformThreadJoinCommissionedNetwork();
    if (server->thread.storage->networkJoinState.borderRouterDetectTimer != 0) {
        HAPPlatformTimerDeregister(server->thread.storage->networkJoinState.borderRouterDetectTimer);
    }
    HAPError err = HAPPlatformTimerRegister(
            &server->thread.storage->networkJoinState.borderRouterDetectTimer,
            HAPPlatformClockGetCurrent() + kHAPThreadAccessoryServerBorderRouterDetectionTimeout,
            HandleBorderRouterDetectionTimeout,
            server);
    HAPAssert(!err);
}

/**
 * Handles result of StartThreadStack
 *
 * @param context  should contain the pointer to the server
 */
static void HandleNetworkJoinResult(void* _Nullable context, size_t len) {
    HAPPrecondition(context);
    HAPPrecondition(len == sizeof(HAPAccessoryServer*));
    HAPAccessoryServer* server = *(HAPAccessoryServer**) context;
    HAPAssert(server);
    HAPAssert(server->thread.storage);

    server->thread.storage->networkJoinState.inProgress = false;

    if (server->thread.storage->networkJoinState.joinSuccessful) {
        HAPLogInfo(&logObject, "Thread network join successful");
    } else {
        HAPLogInfo(&logObject, "Thread network join failed");
        // Join failed. Stop Thread session. Upon stopping Thread, start BLE.
        if (!server->thread.shouldStartTransport && server->transports.ble) {
            // Unless thread restart is pending, start BLE transport after Thread transport stops.
            server->ble.shouldStartTransport = true;
        }
        HAPAccessoryServerStopThreadTransport(server);
    }
}

/**
 * Handles network reachability test result in Platform RunLoop context
 */
static void HandleNetworkReachabilityTestResult(void* _Nullable context, size_t len) {
    HAPPrecondition(context);
    HAPPrecondition(len == sizeof(HAPAccessoryServer*));
    HAPAccessoryServer* server = *(HAPAccessoryServer**) context;
    HAPPrecondition(server);

    if (!server->transports.thread) {
        // Server must have been shutdown before reachability test completes.
        if (server->thread.storage) {
            server->thread.storage->networkJoinState.inProgress = false;
        }
        return;
    }

    if (!server->thread.isTransportRunning || server->thread.isTransportStopping) {
        // Transport has stopped or is stopping. Do not proceed.
        server->thread.storage->networkJoinState.inProgress = false;
        return;
    }

    HAPAssert(server->thread.storage);

    if (server->thread.storage->networkJoinState.reachable) {
        // Network reachable
        HAPLogInfo(&logObject, "Thread network was reachable");
        HAPPlatformThreadSetNetworkParameters(&server->thread.storage->networkJoinState.parameters);
        EnableThread(server);
        server->thread.storage->networkJoinState.joinSuccessful = true;
    } else {
        // Network unreachable
        if (HAPPlatformClockGetCurrent() + REACHABILITY_TEST_DURATION <=
            server->thread.storage->networkJoinState.attachExpireTime) {

            // Retry
            HAPPlatformThreadTestNetworkReachability(
                    &server->thread.storage->networkJoinState.parameters,
                    REACHABILITY_TEST_DURATION,
                    HandleNetworkReachabilityTestResult,
                    server);
            return;
        }
        // Time out
        HAPLogError(&logObject, "Thread network reachability test timed out");
        server->thread.storage->networkJoinState.joinSuccessful = false;
    }

    HandleNetworkJoinResult(context, len);
}

/**
 * Retrieves the number of times authorization has failed
 *
 * @param authAttempts (Out) the number of times authorization
 *                     has failed
 *
 * @return HAPError kHAPError_None if successful
 */
static HAPError GetNumAuthFailures(HAPAccessoryServer* server, uint8_t* authAttempts) {
    HAPPrecondition(server);
    HAPPrecondition(authAttempts);
    HAPError err;
    // Retrieve the number of authorization attempts
    bool found;
    size_t numBytes;
    err = HAPPlatformKeyValueStoreGet(
            server->platform.keyValueStore,
            kHAPKeyValueStoreDomain_Configuration,
            kHAPKeyValueStoreKey_Configuration_NumUnsuccessfulAuthAttempts,
            authAttempts,
            sizeof(*authAttempts),
            &numBytes,
            &found);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Unable to retrieve number of auth attempts");
        return err;
    } else if (!found) {
        *authAttempts = 0;
    } else if (numBytes != sizeof(*authAttempts)) {
        HAPLogError(&logObject, "Invalid authentication attempts counter length: %zu.", numBytes);
        return kHAPError_Unknown;
    }

    HAPAssert(*authAttempts < UINT8_MAX);
    return kHAPError_None;
}

/**
 * Joiner callback in Platform RunLoop (thread-safe) context
 *
 * @param context  accessory server pointer
 * @param len      length of the pointer
 */
static void HandleJoinerResult(void* _Nullable context, size_t len) {
    HAPPrecondition(context);
    HAPPrecondition(len == sizeof(HAPAccessoryServer*));

    HAPAccessoryServer* server = *(HAPAccessoryServer**) context;
    HAPPrecondition(server);

    if (!server->transports.thread) {
        // Transport was shutdown. Do not proceed.
        if (server->thread.storage) {
            server->thread.storage->networkJoinState.inProgress = false;
        }
        return;
    }

    if (!server->thread.isTransportRunning || server->thread.isTransportStopping) {
        // Transport has stopped or is stopping. Do not proceed.
        HAPAssert(server->thread.storage);
        server->thread.storage->networkJoinState.inProgress = false;
        return;
    }

    HAPAssert(server->thread.storage);

    HAPTime joinerDelay = JOINER_DELAY;

    if (server->thread.storage->networkJoinState.reachable) {
        EnableThread(server);
        server->thread.storage->networkJoinState.joinSuccessful = true;
    } else {
        bool retry = true;

        // Retrieve the number of authorization attempts
        uint8_t numAuthAttempts = 0;
        HAPError err = GetNumAuthFailures(server, &numAuthAttempts);
        if (err) {
            HAPLogError(&logObject, "Unable to retrieve number of auth attempts");
            retry = false;
        }

        // A security error means authentication failed.
        if (server->thread.storage->networkJoinState.securityError) {
            // Increment num authorization attempts.
            numAuthAttempts++;
            err = HAPPlatformKeyValueStoreSet(
                    server->platform.keyValueStore,
                    kHAPKeyValueStoreDomain_Configuration,
                    kHAPKeyValueStoreKey_Configuration_NumUnsuccessfulAuthAttempts,
                    &numAuthAttempts,
                    sizeof numAuthAttempts);
            if (err) {
                HAPAssert(err == kHAPError_Unknown);
                retry = false;
            }
            HAPLogError(
                    &logObject,
                    "Invalid authentication.  Attempts remaining: %d/%d.",
                    numAuthAttempts,
                    HAPAuthAttempts_Max);
        }

        if (retry &&
            (server->thread.storage->networkJoinState.attachExpireTime == 0 ||
             HAPPlatformClockGetCurrent() + joinerDelay < server->thread.storage->networkJoinState.attachExpireTime)) {
            if (server->thread.storage->networkJoinState.attachExpireTime == 0) {
                HAPLog(&logObject, "Trying to join again");
            } else {
                HAPLog(&logObject,
                       "Trying to join again: %lu secs left",
                       (unsigned long) ((server->thread.storage->networkJoinState.attachExpireTime - HAPPlatformClockGetCurrent()) / 1000));
            }

            HAPAssert(!server->thread.storage->networkJoinState.joinerTimer);
            HAPError hapErr = HAPPlatformTimerRegister(
                    &server->thread.storage->networkJoinState.joinerTimer,
                    HAPPlatformClockGetCurrent() + joinerDelay,
                    HandleJoinerTimer,
                    server);
            HAPAssert(!hapErr);
            // No completion callback. Retry joiner.
            return;
        } else {
            HAPLog(&logObject, "Giving up on joining");
            server->thread.storage->networkJoinState.joinSuccessful = false;
        }
    }

    HandleNetworkJoinResult(context, len);
}

/**
 * Handle Thread joiner timer
 *
 * @param timer     timer
 * @param context   accessory server
 */
static void HandleJoinerTimer(HAPPlatformTimerRef timer HAP_UNUSED, void* context) {
    HAPPrecondition(context);
    HAPAccessoryServer* server = context;

    HAPLog(&logObject, "%s", __func__);

    if (server->thread.storage) {
        server->thread.storage->networkJoinState.joinerTimer = 0;
    }

    if (!server->transports.thread) {
        // Transport was shutdown. Do not proceed.
        if (server->thread.storage) {
            server->thread.storage->networkJoinState.inProgress = false;
        }
        return;
    }

    if (!server->thread.isTransportRunning || server->thread.isTransportStopping) {
        // Transport has stopped or is stopping. Do not proceed.
        HAPAssert(server->thread.storage);
        server->thread.storage->networkJoinState.inProgress = false;
        return;
    }

    HAPError err;
    // Retrieve the Joiner Passphrase.
    HAPJoinerPassphrase joinerPassphrase;
    bool found = false;

    if (!server->platform.setupDisplay && server->platform.setupNFC) {
        // Generating passphrase from stored setup code.
        HAPSetupCode setupCode;
        HAPPlatformAccessorySetupLoadSetupCode(server->platform.accessorySetup, &setupCode);
        HAPAccessorySetupGenerateJoinerPassphrase(&joinerPassphrase, &setupCode);
        found = true;
    }

    err = HAPPlatformThreadStartJoiner(found ? joinerPassphrase.stringValue : NULL, HandleJoinerResult, server);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Unable to find/generate joiner passphrase");

        // Finish joining with failure
        server->thread.storage->networkJoinState.reachable = false;
        HandleNetworkJoinResult(&server, sizeof server);
    }
}

void HAPThreadAccessoryServerJoinNetwork(HAPAccessoryServer* server) {
    HAPPrecondition(server);
    HAPPrecondition(server->thread.storage);

    HAPLogDebug(&logObject, "%s", __func__);

    HAPError err;

    HAPAssert(!server->thread.storage->networkJoinState.inProgress);
    server->thread.storage->networkJoinState.inProgress = true;
    server->thread.storage->networkJoinState.joinSuccessful = false;
    server->thread.storage->networkJoinState.staticCommissioning = false;

    // By default, attempt to reattach if joining fails
    server->thread.storage->networkJoinState.shouldReattach = true;

    // Retrieve the number of authorization attempts

    uint8_t numAuthAttempts = 0;
    err = GetNumAuthFailures(server, &numAuthAttempts);

    if (err) {
        HAPLogError(&logObject, "Unable to retrieve number of auth attempts");
        // Fail the joining
        err = HAPPlatformRunLoopScheduleCallback(HandleNetworkJoinResult, &server, sizeof server);
        HAPAssert(!err);
        return;
    }

    // Make sure there haven't been too many attempts to join the network
    if (numAuthAttempts >= HAPAuthAttempts_Max) {
        HAPLogError(
                &logObject,
                "Accessory has received more than %d unsuccessful authentication attempts",
                HAPAuthAttempts_Max);
        // Fail the joining
        err = HAPPlatformRunLoopScheduleCallback(HandleNetworkJoinResult, &server, sizeof server);
        HAPAssert(!err);

        // Do not reattach autonomously
        server->thread.storage->networkJoinState.shouldReattach = false;
        return;
    }

    // If we are commissioned with designated parameters and the instance hasn't been commissioned yet
    // set commissioning data
    if (!HAPPlatformThreadIsCommissioned() && server->thread.storage->networkJoinState.parametersAreSet) {
        if (server->thread.storage->networkJoinState.formingAllowed) {
            HAPLogDebug(&logObject, "Forming allowed with new network parameters");
            HAPPlatformThreadSetNetworkParameters(&server->thread.storage->networkJoinState.parameters);
        } else {
            // If forming is not allowed, run Active Scan to check if Thread network
            // with the commissioned parameters is reachable so that the device
            // won't form its own network.
            HAPLogDebug(&logObject, "Testing reachability with new network parameters");
            HAPLogInfo(&logObject, "Running reachability test over Thread channel");
            server->thread.storage->networkJoinState.attachExpireTime =
                    HAPPlatformClockGetCurrent() + server->thread.storage->networkJoinState.attachTimeout;
            HAPPlatformThreadTestNetworkReachability(
                    &server->thread.storage->networkJoinState.parameters,
                    HAPMin(server->thread.storage->networkJoinState.attachTimeout, REACHABILITY_TEST_DURATION),
                    HandleNetworkReachabilityTestResult,
                    server);
            return;
        }
    }

    if (HAPPlatformThreadIsCommissioned() || server->thread.storage->networkJoinState.parametersAreSet) {
        HAPLogDebug(&logObject, "Enabling Thread");
        EnableThread(server);
        server->thread.storage->networkJoinState.joinSuccessful = true;
    } else if (server->thread.storage->networkJoinState.joinerRequested) {
        HAPLogDebug(&logObject, "Running Joiner");
        server->thread.storage->networkJoinState.attachExpireTime =
                HAPPlatformClockGetCurrent() + server->thread.storage->networkJoinState.attachTimeout;
        HAPPlatformTimerRef joinerTimer;
        err = HAPPlatformTimerRegister(&joinerTimer, 0, HandleJoinerTimer, server);
        HAPAssert(!err);

        // Do not reattach when joiner fails
        server->thread.storage->networkJoinState.shouldReattach = false;
        return;
    } else {
        // Not commissioned and not requested of joiner. Try static commissioning.
        server->thread.storage->networkJoinState.joinSuccessful = HAPPlatformThreadJoinStaticCommissioninedNetwork();
        if (server->thread.storage->networkJoinState.joinSuccessful) {
            HAPLogDebug(&logObject, "Thread enabled with static commissioning");
            server->thread.storage->networkJoinState.staticCommissioning = true;
        }

        if (!server->thread.storage->networkJoinState.joinSuccessful) {
            // Use Joiner if allowed.
            bool useJoiner = true;
            if (server->transports.ble) {
                // When BLE is supported, do not run joiner
                useJoiner = false;
            }
            if (useJoiner) {
                HAPLogDebug(&logObject, "Running default joiner forever");
                server->thread.storage->networkJoinState.attachExpireTime = 0;
                HAPAssert(!server->thread.storage->networkJoinState.joinerTimer);
                err = HAPPlatformTimerRegister(
                        &server->thread.storage->networkJoinState.joinerTimer,
                        HAPPlatformClockGetCurrent() + JOINER_DELAY,
                        HandleJoinerTimer,
                        server);
                HAPAssert(!err);
                return;
            }
        }

        if (!server->thread.storage->networkJoinState.joinSuccessful) {
            HAPLogDebug(&logObject, "No Thread credentials");
            server->thread.storage->networkJoinState.shouldReattach = false;
        }
    }

    // Schedule callback so that failure would potentially be handled after app creation is complete
    err = HAPPlatformRunLoopScheduleCallback(HandleNetworkJoinResult, &server, sizeof server);
    HAPAssert(!err);
}

/**
 * Handles loss of Thread connectivity
 *
 * @param server   accessory server
 */
void HAPThreadAccessoryServerHandleThreadConnectivityLoss(HAPAccessoryServer* server) {
    HAPPrecondition(server);
    if (server->transports.ble) {
#if (HAP_FEATURE_THREAD_CERTIFICATION_OVERRIDES)
        // If thread network viability checking is disabled, then just keep thread on, and do not start BLE.
#else
        if ((server->thread.deviceParameters.deviceType & kHAPPlatformThreadDeviceCapabilities_SED) != 0) {
            // Start BLE when Thread stops
            // even if concurrent BLE and Thread is supported by the platform,
            // in order to prevent potentially overlapping window of BLE and Thread connectivity
            // despite the Thread connectivity loss detection at the moment.
            server->ble.shouldStartTransport = true;

            HAPAccessoryServerStopThreadTransport(server);
        } else {
            HAPAccessoryServerStartBLETransport(server);
        }
#endif
    }
}

void HAPThreadAccessoryServerStartBorderRouterDetectionTimer(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    HAPLogError(&logObject, "Thread connectivity was lost. Border router detection timer starts.");
    HAPError err = HAPPlatformTimerRegister(
            &server->thread.storage->networkJoinState.borderRouterDetectTimer,
            HAPPlatformClockGetCurrent() + kHAPThreadAccessoryServerBorderRouterDetectionTimeout,
            HandleBorderRouterDetectionTimeout,
            server);
    HAPAssert(!err);
}

void HAPAccessoryServerSetThreadNetworkReachable(HAPAccessoryServer* server, bool reachable, bool securityError) {
    HAPPrecondition(server);

    // Note that server might have shutdown Thread transport already, in which case the call can be ignored.
    if (server->thread.storage) {
        server->thread.storage->networkJoinState.reachable = reachable;
        server->thread.storage->networkJoinState.securityError = securityError;
    }
}

#endif
