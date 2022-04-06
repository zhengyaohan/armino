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

#include "HAP+KeyValueStoreDomains.h"
#include "HAPAccessoryServer+Internal.h"
#include "HAPLogSubsystem.h"
#include "HAPPairing.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "Pairing" };

HAP_STATIC_ASSERT(
        sizeof(HAPPairingIndex) == sizeof(HAPPlatformKeyValueStoreKey),
        HAPPairingIndexMatchesKeyValueStoreKey);

HAP_RESULT_USE_CHECK
HAPError
        HAPPairingKVSRead(HAPAccessoryServer* server, HAPPairingIndex pairingIndex, HAPPairing* pairing, bool* exists) {
    HAPPrecondition(server);
    HAPPrecondition(pairingIndex < server->maxPairings);
    HAPPrecondition(pairing);
    HAPPrecondition(exists);

    HAPError err;

    // Load pairing.
    size_t numBytes;
    uint8_t pairingBytes[sizeof(HAPPairingID) + sizeof(uint8_t) + sizeof(HAPPairingPublicKey) + sizeof(uint8_t)];
    err = HAPPlatformKeyValueStoreGet(
            server->platform.keyValueStore,
            kHAPKeyValueStoreDomain_Pairings,
            pairingIndex,
            pairingBytes,
            sizeof pairingBytes,
            &numBytes,
            exists);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }
    if (!*exists) {
        return kHAPError_None;
    }
    if (numBytes != sizeof pairingBytes) {
        HAPLog(&logObject, "Invalid pairing 0x%02X size %lu.", pairingIndex, (unsigned long) numBytes);
        return kHAPError_Unknown;
    }

    HAPRawBufferZero(pairing, sizeof *pairing);
    HAPAssert(sizeof pairing->identifier.bytes == 36);
    HAPRawBufferCopyBytes(pairing->identifier.bytes, &pairingBytes[0], 36);
    pairing->numIdentifierBytes = pairingBytes[36];
    HAPAssert(sizeof pairing->publicKey.value == 32);
    HAPRawBufferCopyBytes(pairing->publicKey.value, &pairingBytes[37], 32);
    pairing->permissions = pairingBytes[69];

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPairingKVSWrite(HAPAccessoryServer* server, HAPPairingIndex pairingIndex, const HAPPairing* pairing) {
    HAPPrecondition(server);
    HAPPrecondition(pairing);
    HAPPrecondition(pairingIndex < server->maxPairings);

    HAPError err;

    uint8_t pairingBytes[sizeof(HAPPairingID) + sizeof(uint8_t) + sizeof(HAPPairingPublicKey) + sizeof(uint8_t)];
    HAPRawBufferZero(pairingBytes, sizeof pairingBytes);
    HAPAssert(sizeof pairing->identifier.bytes == 36);
    HAPAssert(pairing->numIdentifierBytes <= sizeof pairing->identifier.bytes);
    HAPRawBufferCopyBytes(&pairingBytes[0], pairing->identifier.bytes, pairing->numIdentifierBytes);
    pairingBytes[36] = pairing->numIdentifierBytes;
    HAPAssert(sizeof pairing->publicKey.value == 32);
    HAPRawBufferCopyBytes(&pairingBytes[37], pairing->publicKey.value, 32);
    pairingBytes[69] = pairing->permissions;
    err = HAPPlatformKeyValueStoreSet(
            server->platform.keyValueStore,
            kHAPKeyValueStoreDomain_Pairings,
            pairingIndex,
            pairingBytes,
            sizeof pairingBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPairingKVSAdd(HAPAccessoryServer* server, HAPPairing* pairing, HAPPairingIndex* _Nullable pairingIndex) {
    HAPPrecondition(server);
    HAPPrecondition(pairing);

    HAPError err;

    bool found;
    HAPPlatformKeyValueStoreKey key;
    // Look for free pairing slot.
    for (key = 0; key < server->maxPairings; key++) {
        HAPPairing pairingSlot;
        err = HAPPairingKVSRead(server, key, &pairingSlot, &found);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            return err;
        }
        if (!found) {
            // free space found;
            break;
        }
    }
    if (key == server->maxPairings) {
        HAPLog(&logObject, "No space for additional pairings.");
        return kHAPError_OutOfResources;
    }
    err = HAPPairingKVSWrite(server, key, pairing);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "%s: Unable to write pairing.", __func__);
        return err;
    }
    if (pairingIndex) {
        *pairingIndex = (HAPPairingIndex) key;
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPairingKVSUpdatePermissions(
        HAPAccessoryServer* server,
        HAPPairingIndex pairingIndex,
        uint8_t pairingPermissions) {
    HAPPrecondition(server);

    HAPError err;

    bool exists;
    HAPPairing pairing;
    err = HAPPairingKVSRead(server, pairingIndex, &pairing, &exists);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }
    if (!exists) {
        HAPLogError(&logObject, "%s: Pairing at index %u does not exist.", __func__, pairingIndex);
        HAPFatalError();
    }
    pairing.permissions = pairingPermissions;
    err = HAPPairingKVSWrite(server, pairingIndex, &pairing);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPairingKVSRemove(HAPAccessoryServer* server, HAPPairingIndex pairingIndex) {
    HAPPrecondition(server);

    HAPError err;

    err = HAPPlatformKeyValueStoreRemove(
            server->platform.keyValueStore, kHAPKeyValueStoreDomain_Pairings, pairingIndex);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    return kHAPError_None;
}

typedef struct {
    HAPAccessoryServer* server;
    void* _Nullable context;
    HAPPairingEnumerateCallback callback;
} PairingsEnumerateContext;

HAP_RESULT_USE_CHECK
static HAPError PairingsEnumerateCallback(
        void* _Nullable context,
        HAPPlatformKeyValueStoreRef keyValueStore,
        HAPPlatformKeyValueStoreDomain domain,
        HAPPlatformKeyValueStoreKey key,
        bool* shouldContinue) {
    HAPPrecondition(context);
    PairingsEnumerateContext* arguments = context;
    HAPPrecondition(arguments->server);
    HAPPrecondition(arguments->callback);
    HAPPrecondition(keyValueStore);
    HAPPrecondition(domain == kHAPKeyValueStoreDomain_Pairings);
    HAPPrecondition(shouldContinue);

    HAPError err;

    err = arguments->callback(arguments->context, arguments->server, key, shouldContinue);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPairingKVSEnumerate(
        HAPAccessoryServer* server,
        HAPPairingEnumerateCallback callback,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(callback);

    HAPError err;

    PairingsEnumerateContext kvsContext;
    HAPRawBufferZero(&kvsContext, sizeof kvsContext);
    kvsContext.server = server;
    kvsContext.context = context;
    kvsContext.callback = callback;

    err = HAPPlatformKeyValueStoreEnumerate(
            server->platform.keyValueStore, kHAPKeyValueStoreDomain_Pairings, PairingsEnumerateCallback, &kvsContext);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
    }
    return err;
}

HAP_RESULT_USE_CHECK
HAPError HAPPairingKVSRemoveAll(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    HAPError err;

    HAPPlatformKeyValueStoreRef keyValueStore = server->platform.keyValueStore;
    // Reset pairings.
    err = HAPPlatformKeyValueStorePurgeDomain(keyValueStore, kHAPKeyValueStoreDomain_Pairings);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
    }
    return err;
}
