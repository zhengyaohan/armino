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
// Copyright (C) 2020 Apple Inc. All Rights Reserved.
#include "HAPPlatformThreadUtils+Init.h"

#include "HAP+API.h"
#include "HAPAccessoryServer+Internal.h"
#include "HAPLogSubsystem.h"

#include <openthread/link.h>
#include <openthread/thread.h>

static otInstance* threadInstance;
static bool threadInstanceIsInitialized = false;
static bool threadIsInitialized = false;
static uint32_t configuredPollPeriod = 0;
static bool credentialsClearRequested = false;

#define DEFAULT_MAX_MAC_RETRIES 15

//*****************************************************************************
// Thread Instance Functions
//*****************************************************************************
void HAPPlatformThreadInitInstance(void) {
    threadInstance = otInstanceInitSingle();
    HAPAssert(threadInstance != NULL);
    threadInstanceIsInitialized = true;
}

bool HAPPlatformThreadInstanceIsInitialized(void) {
    return threadInstanceIsInitialized;
}

void* HAPPlatformThreadGetHandle(void) {
    return threadInstance;
}

//*****************************************************************************
// Thread Link Functions
//*****************************************************************************
/**
 * Clear Thread network credentials
 */
static void ClearThreadParameters(void) {
    // Clear stored active data
    otOperationalDataset dataset;
    otError otErr = otDatasetGetActive(threadInstance, &dataset);

    // Note that OpenThread API didn't document OT_ERROR_NOT_FOUND
    // but it is returned in case there is no active dataset.
    if (otErr != OT_ERROR_NOT_FOUND) {
        HAPAssert(otErr == OT_ERROR_NONE);
        HAPRawBufferZero(&dataset.mComponents, sizeof dataset.mComponents);
        otErr = otDatasetSetActive(threadInstance, &dataset);
    }

    // Erase persistent info.
    otInstanceErasePersistentInfo(threadInstance);

    // Finalize and recreate the instance
    otInstanceFinalize(threadInstance);
    threadInstance = otInstanceInitSingle();
    HAPAssert(threadInstance != NULL);
}

void HAPPlatformThreadInitialize(
        void* server,
        HAPPlatformThreadDeviceCapabilities deviceType,
        uint32_t pollPeriod,
        uint32_t childTimeout,
        uint8_t txPower) {
    HAPPrecondition(server);
    if (threadIsInitialized || !threadInstanceIsInitialized) {
        return;
    }

    if (credentialsClearRequested) {
        // Clearing credentials was requested
        ClearThreadParameters();
        credentialsClearRequested = false;
    }

    otError e;

    otLinkSetMaxFrameRetriesDirect(threadInstance, DEFAULT_MAX_MAC_RETRIES);
#if (OPENTHREAD_FTD == 1)
    otLinkSetMaxFrameRetriesIndirect(threadInstance, DEFAULT_MAX_MAC_RETRIES);
#endif
    otPlatRadioSetTransmitPower(threadInstance, txPower);

    otLinkSetEnabled(threadInstance, true);

    configuredPollPeriod = 0;
    otThreadSetLinkMode(
            threadInstance,
            (otLinkModeConfig) {
                    .mDeviceType = ((deviceType & kHAPPlatformThreadDeviceCapabilities_FTD) != 0), // Full Thread Device
                    .mNetworkData = ((deviceType & kHAPPlatformThreadDeviceCapabilities_FTD) != 0), // Full network data
                    .mRxOnWhenIdle =
                            ((deviceType & kHAPPlatformThreadDeviceCapabilities_SED) == 0), // Not a sleepy end device
                    .mSecureDataRequests = true,
            });
    if ((deviceType & kHAPPlatformThreadDeviceCapabilities_SED) != 0) {
        configuredPollPeriod = pollPeriod;
        e = otLinkSetPollPeriod(threadInstance, pollPeriod);
        HAPAssert(e == OT_ERROR_NONE);
    }

    if (childTimeout != 0) {
        otThreadSetChildTimeout(threadInstance, childTimeout);
    }

    e = otIp6SetEnabled(threadInstance, true);
    HAPAssert(e == OT_ERROR_NONE);

    e = otSetStateChangedCallback(threadInstance, HAPPlatformThreadReportStateChanged, server);
    HAPAssert(e == OT_ERROR_NONE || e == OT_ERROR_ALREADY);

    // Initialize MDNS module
    HAPPlatformThreadInitializeMDNS(server);

    threadIsInitialized = true;
}

void HAPPlatformThreadDeinitialize(void* server) {
    HAPPrecondition(server);
    if (threadIsInitialized) {
        HAPAccessoryServer* accServer = (HAPAccessoryServer*) server;

        // Deinitialize MDNS module
        HAPPlatformThreadDeinitializeMDNS();

        // Remove any wakelocks
        HAPPlatformThreadPurgeWakeLocks();

        // When using multiprotocol, otInstance cannot be finalized and reinitialized.
        // Hence, the instance is wound down here one by one.
        otThreadSetEnabled(threadInstance, false);

        otRemoveStateChangeCallback(threadInstance, HAPPlatformThreadReportStateChanged, accServer);
        otIp6SetEnabled(threadInstance, false);
        otLinkSetEnabled(threadInstance, false);

        if (credentialsClearRequested) {
            ClearThreadParameters();
            credentialsClearRequested = false;
        }
        threadIsInitialized = false;

        // Update the Thread device role, in case it's not updated through threadStateChangedCallback
        HAPPlatformThreadUpdateDeviceRole(kHAPPlatformThreadDeviceRole_Disabled);
    }
}

bool HAPPlatformThreadIsInitialized(void) {
    return threadIsInitialized;
}

void HAPPlatformThreadInitiateFactoryReset(void) {
    HAPPlatformThreadClearMDNSMem();
}

//*****************************************************************************
// Thread Parameters functions
//*****************************************************************************

HAPError HAPPlatformThreadClearParameters(void* server) {
    // If multiprotocol library is used, Thread could be shutdown here.
    // While multiprotocol library is not in use, platform for the other stack (BLE)
    // must be brought up.

    if (threadInstanceIsInitialized && !threadIsInitialized) {
        // Thread is not up. Credentials can be cleared immediately.
        ClearThreadParameters();
    } else {
        // Credentials must be cleared after Thread transport stops or when Thread instance is created.
        credentialsClearRequested = true;
    }

    HAPAccessoryServer* mServer = (HAPAccessoryServer*) server;
    mServer->thread.storage->networkJoinState.parametersAreSet = false;

    // Remove any wakelocks
    HAPPlatformThreadPurgeWakeLocks();

    return kHAPError_None;
}

uint32_t HAPPlatformThreadGetConfiguredPollPeriod(void) {
    return configuredPollPeriod;
}
