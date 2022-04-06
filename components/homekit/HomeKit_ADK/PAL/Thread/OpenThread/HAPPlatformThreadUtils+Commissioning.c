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

#include "HAP+API.h"

#include "HAPAccessoryServer+Internal.h"

#include "HAPPlatform.h"
#include "HAPPlatformKeyValueStore+SDKDomains.h"
#include "HAPPlatformThreadUtils+Commissioning.h"

#include <openthread/dataset.h>
#include <openthread/joiner.h>
#include <openthread/link.h>
#include <openthread/netdata.h>
#include <openthread/thread.h>

HAPPlatformRunLoopCallback joinerCallback;
HAPAccessoryServer* joinerServer;

//*****************************************************************************
// Static Commissioning Parameters (For Test/Debug purposes only)
//   - The Thread network parameters that will be used when built for static
//     commissioning.
//*****************************************************************************

#ifndef THREAD_PANID
#define THREAD_PANID 43981
#endif
#ifndef THREAD_EXTPANID
#define THREAD_EXTPANID 0xDEAD00BEEF00CAFEull
#endif
#ifndef THREAD_CHANNEL
#define THREAD_CHANNEL 11
#endif
#ifndef THREAD_MASTERKEY_UPPER64
#define THREAD_MASTERKEY_UPPER64 0x0011223344556677ull
#endif
#ifndef THREAD_MASTERKEY_LOWER64
#define THREAD_MASTERKEY_LOWER64 0x8899AABBCCDDEEFFull
#endif

bool HAPPlatformThreadIsCommissioned(void) {
    HAPAssert(HAPPlatformThreadInstanceIsInitialized());
    return otDatasetIsCommissioned(HAPPlatformThreadGetHandle());
}

void HAPPlatformThreadSetNetworkParameters(const HAPPlatformThreadNetworkParameters* parameters) {
    HAPAssert(HAPPlatformThreadIsInitialized());

    otInstance* threadInstance = HAPPlatformThreadGetHandle();

    otError e = otThreadSetNetworkName(threadInstance, parameters->networkName);
    HAPAssert(e == OT_ERROR_NONE);
    e = otLinkSetChannel(threadInstance, parameters->channel);
    HAPAssert(e == OT_ERROR_NONE);
    e = otLinkSetPanId(threadInstance, parameters->panId);
    HAPAssert(e == OT_ERROR_NONE);
    otExtendedPanId extPanId;
    HAPRawBufferCopyBytes(&extPanId.m8, parameters->extPanId, sizeof extPanId.m8);
    e = otThreadSetExtendedPanId(threadInstance, &extPanId);
    HAPAssert(e == OT_ERROR_NONE);
    otMasterKey masterKey;
    HAPRawBufferCopyBytes(&masterKey.m8, parameters->masterKey, sizeof masterKey.m8);
    e = otThreadSetMasterKey(threadInstance, &masterKey);
    HAPAssert(e == OT_ERROR_NONE);
}

void HAPPlatformThreadJoinCommissionedNetwork(void) {
    HAPAssert(HAPPlatformThreadIsInitialized());

    HAPPlatformThreadUpdateDeviceRole(kHAPPlatformThreadDeviceRole_Joining);
    otInstance* threadInstance = HAPPlatformThreadGetHandle();

    otError e = otThreadSetEnabled(threadInstance, true);
    HAPAssert(e == OT_ERROR_NONE);
}

bool HAPPlatformThreadJoinStaticCommissioninedNetwork(void) {
    HAPAssert(HAPPlatformThreadIsInitialized());

#if (HAP_FEATURE_THREAD_CERTIFICATION_OVERRIDES)
    return true;
#endif

#if (THREAD_STATIC_COMMISSIONING == 1)

    HAPPlatformThreadNetworkParameters params;
    params.channel = THREAD_CHANNEL;
    params.panId = THREAD_PANID;

    params.extPanId[0] = (THREAD_EXTPANID >> 56) & 0xFF;
    params.extPanId[1] = (THREAD_EXTPANID >> 48) & 0xFF;
    params.extPanId[2] = (THREAD_EXTPANID >> 40) & 0xFF;
    params.extPanId[3] = (THREAD_EXTPANID >> 32) & 0xFF;
    params.extPanId[4] = (THREAD_EXTPANID >> 24) & 0xFF;
    params.extPanId[5] = (THREAD_EXTPANID >> 16) & 0xFF;
    params.extPanId[6] = (THREAD_EXTPANID >> 8) & 0xFF;
    params.extPanId[7] = THREAD_EXTPANID & 0xFF;

    params.masterKey[0] = (THREAD_MASTERKEY_UPPER64 >> 56) & 0xff;
    params.masterKey[1] = (THREAD_MASTERKEY_UPPER64 >> 48) & 0xff;
    params.masterKey[2] = (THREAD_MASTERKEY_UPPER64 >> 40) & 0xff;
    params.masterKey[3] = (THREAD_MASTERKEY_UPPER64 >> 32) & 0xff;
    params.masterKey[4] = (THREAD_MASTERKEY_UPPER64 >> 24) & 0xff;
    params.masterKey[5] = (THREAD_MASTERKEY_UPPER64 >> 16) & 0xff;
    params.masterKey[6] = (THREAD_MASTERKEY_UPPER64 >> 8) & 0xff;
    params.masterKey[7] = THREAD_MASTERKEY_UPPER64 & 0xff;

    params.masterKey[8] = (THREAD_MASTERKEY_LOWER64 >> 56) & 0xff;
    params.masterKey[9] = (THREAD_MASTERKEY_LOWER64 >> 48) & 0xff;
    params.masterKey[10] = (THREAD_MASTERKEY_LOWER64 >> 40) & 0xff;
    params.masterKey[11] = (THREAD_MASTERKEY_LOWER64 >> 32) & 0xff;
    params.masterKey[12] = (THREAD_MASTERKEY_LOWER64 >> 24) & 0xff;
    params.masterKey[13] = (THREAD_MASTERKEY_LOWER64 >> 16) & 0xff;
    params.masterKey[14] = (THREAD_MASTERKEY_LOWER64 >> 8) & 0xff;
    params.masterKey[15] = THREAD_MASTERKEY_LOWER64 & 0xff;

    HAPPlatformThreadSetNetworkParameters(&params);
    HAPPlatformThreadJoinCommissionedNetwork();

    return true;
#else
    return false;
#endif
}

HAPError HAPPlatformThreadGetNetworkParameters(HAPPlatformThreadNetworkParameters* parameters) {
    HAPPrecondition(parameters);

    otInstance* threadInstance = HAPPlatformThreadGetHandle();
    if (!HAPPlatformThreadInstanceIsInitialized()) {
        return kHAPError_InvalidState;
    }

    const char* networkNameSrc = otThreadGetNetworkName(threadInstance);
    HAPAssert(networkNameSrc);
    size_t copyLength = HAPMin(sizeof parameters->networkName - 1, HAPStringGetNumBytes(networkNameSrc));
    HAPRawBufferCopyBytes(parameters->networkName, networkNameSrc, copyLength);
    parameters->networkName[copyLength] = '\0';

    parameters->channel = (uint16_t) otLinkGetChannel(threadInstance);
    parameters->panId = (uint16_t) otLinkGetPanId(threadInstance);

    const otExtendedPanId* extPanIdSrc = otThreadGetExtendedPanId(threadInstance);
    HAPRawBufferCopyBytes(parameters->extPanId, extPanIdSrc->m8, sizeof parameters->extPanId);
    return kHAPError_None;
}

// Thread native commissioning
typedef struct {
    otError err;
} JoinerResultData;

/**
 * Joiner callback in Platform RunLoop (thread-safe) context
 */
static void RunLoopHandleThreadJoinerResult(void* _Nullable context, size_t len) {
    HAPPrecondition(context);
    HAPPrecondition(len == sizeof(JoinerResultData));

    JoinerResultData* result = context;

    if (result->err == OT_ERROR_NONE) {
        HAPLog(&kHAPLog_Default, "Join success");
        HAPAccessoryServerSetThreadNetworkReachable(joinerServer, true, false);
    } else {
        HAPLog(&kHAPLog_Default, "Join failed: %s", otThreadErrorToString(result->err));
        HAPAccessoryServerSetThreadNetworkReachable(joinerServer, false, (result->err == OT_ERROR_SECURITY));
    }

    HAPPlatformRunLoopCallback callback = joinerCallback;
    joinerCallback = NULL;

    if (callback) {
        callback(&joinerServer, sizeof joinerServer);
    }
}

static void threadJoinerCallback(otError aError, void* aContext) {
    JoinerResultData result = { .err = aError };
    HAPError err = HAPPlatformRunLoopScheduleCallback(RunLoopHandleThreadJoinerResult, &result, sizeof result);
    HAPAssert(!err);
}

HAPError HAPPlatformThreadStartJoiner(const char* passphrase, HAPPlatformRunLoopCallback callback, void* server_) {
    HAPPrecondition(server_);
    joinerServer = server_;
    HAPAssert(HAPPlatformThreadIsInitialized());

    HAPJoinerPassphrase joinerPassphrase;

    if (!passphrase) {
        size_t numBytes;
        bool found;

        // If our accessory doesn't have access to the setup code then the passphrase
        // must have been provided during commissioning.
        HAPError err = HAPPlatformKeyValueStoreGet(
                joinerServer->platform.keyValueStore,
                kSDKKeyValueStoreDomain_Provisioning,
                kSDKKeyValueStoreKey_Provisioning_JoinerPassphrase,
                joinerPassphrase.stringValue,
                sizeof joinerPassphrase.stringValue,
                &numBytes,
                &found);

        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            return err;
        } else if (found) {
            passphrase = joinerPassphrase.stringValue;
        } else {
            // Not found
            return kHAPError_Unknown;
        }
    }

    joinerCallback = callback;

    HAPPlatformThreadUpdateDeviceRole(kHAPPlatformThreadDeviceRole_Joining);
    otInstance* threadInstance = HAPPlatformThreadGetHandle();
    otError error = otJoinerStart(
            threadInstance, passphrase, NULL, "Nordic", NULL, NULL, NULL, threadJoinerCallback, threadInstance);

    HAPAssert(error == OT_ERROR_NONE);

    HAPEui64 eui;
    HAPPlatformReadEui(&eui);
    char strbuf[24];
    HAPError err = HAPEui64GetDescription(&eui, strbuf, sizeof(strbuf));
    HAPAssert(!err);
    HAPLogDebug(&kHAPLog_Default, "EUI: %s", strbuf);

    HAPLogDebug(&kHAPLog_Default, "Joiner Passphrase: %s", passphrase);

    return kHAPError_None;
}
