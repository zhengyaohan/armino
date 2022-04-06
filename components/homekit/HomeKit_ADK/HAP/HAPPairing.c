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

#include "HAPPairing.h"

#include "HAPAccessoryServer+Internal.h"
#include "HAPLogSubsystem.h"
#include "HAPPairingKVS.h"
#include "HAPThreadAccessoryServer+NetworkJoin.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "Pairing" };

HAP_STATIC_ASSERT(
        sizeof(HAPPairingIndex) == sizeof(HAPPlatformKeyValueStoreKey),
        HAPPairingIndexMatchesKeyValueStoreKey);

HAP_RESULT_USE_CHECK
uint32_t HAPPairingReadFlags(const HAPTLV* tlv) {
    HAPPrecondition(tlv);

    uint32_t value = 0;
    for (size_t i = 0; i < tlv->value.numBytes; i++) {
        const uint8_t* b = tlv->value.bytes;

        if (i < sizeof value) {
            value |= (uint32_t)((uint32_t) b[i] << (i * 8)); // NOLINT(google-readability-casting)
        } else {
            if (b[i]) {
                HAPLogBuffer(
                        &logObject,
                        tlv->value.bytes,
                        tlv->value.numBytes,
                        "Ignoring flags after 32 bits in TLV type 0x%02X.",
                        tlv->type);
                break;
            }
        }
    }
    return value;
}

HAP_RESULT_USE_CHECK
size_t HAPPairingGetNumBytes(uint32_t value) {
    if (value > 0x00FFFFFF) {
        return 4;
    }
    if (value > 0x0000FFFF) {
        return 3;
    }
    if (value > 0x000000FF) {
        return 2;
    }
    if (value > 0x00000000) {
        return 1;
    }
    return 0;
}

HAP_RESULT_USE_CHECK
HAPError HAPPairingAdd(HAPAccessoryServer* server, HAPPairing* pairing, HAPPairingIndex* _Nullable pairingIndex) {
    HAPPrecondition(server);
    HAPPrecondition(pairing);
    HAPPrecondition(pairing->numIdentifierBytes <= sizeof pairing->identifier.bytes);

    HAPError err;
    bool wasPaired = HAPAccessoryServerIsPaired(server);

    {
        err = HAPPairingKVSAdd(server, pairing, pairingIndex);
        if (err) {
            HAPAssert(err == kHAPError_Unknown || err == kHAPError_OutOfResources);
            return err;
        }
    }

    HAPAccessoryServerUpdateUnpairedStateTimer(server);

    // Inform application of owner controller added
    if (!wasPaired && server->callbacks.handlePairingStateChange) {
        server->callbacks.handlePairingStateChange(server, kHAPPairingStateChange_Paired, server->context);
    }

    // Inform application of admin or additional controller added
    if (server->callbacks.handleControllerPairingStateChange) {
        // Copy controller pairing info
        HAPControllerPairingIdentifier pairingIdentifier;
        HAPRawBufferZero(&pairingIdentifier, sizeof pairingIdentifier);
        HAPAssert(sizeof pairingIdentifier.bytes >= pairing->numIdentifierBytes);
        HAPRawBufferCopyBytes(pairingIdentifier.bytes, pairing->identifier.bytes, pairing->numIdentifierBytes);
        pairingIdentifier.numBytes = pairing->numIdentifierBytes;

        HAPControllerPublicKey* publicKey;
        HAPAssert(sizeof publicKey->bytes == sizeof pairing->publicKey.value);
        publicKey = (HAPControllerPublicKey*) &pairing->publicKey.value;

        server->callbacks.handleControllerPairingStateChange(
                server, kHAPControllerPairingStateChange_Paired, &pairingIdentifier, publicKey, NULL);
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPairingUpdatePermissions(
        HAPAccessoryServer* server,
        HAPPairingIndex pairingIndex,
        uint8_t pairingPermissions) {
    HAPPrecondition(server);

    HAPError err;

    {
        err = HAPPairingKVSUpdatePermissions(server, pairingIndex, pairingPermissions);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            return err;
        }
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPairingGet(HAPAccessoryServer* server, HAPPairingIndex pairingIndex, HAPPairing* pairing, bool* exists) {
    HAPPrecondition(server);
    HAPPrecondition(pairing);
    HAPPrecondition(pairingIndex < server->maxPairings);
    HAPPrecondition(exists);

    HAPError err;

    HAPRawBufferZero(pairing, sizeof *pairing);

    {
        err = HAPPairingKVSRead(server, pairingIndex, pairing, exists);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            return err;
        }
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPairingRemove(HAPAccessoryServer* server, HAPPairing* _Nullable pairing, HAPPairingIndex pairingIndex) {
    HAPPrecondition(server);

    HAPError err;

    {
        err = HAPPairingKVSRemove(server, pairingIndex);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            return err;
        }
    }

    if (!HAPAccessoryServerIsPaired(server)) {
        HAPAccessoryServerUpdateUnpairedStateTimer(server);

        // Inform application of last admin controller removed
        if (server->callbacks.handlePairingStateChange) {
            server->callbacks.handlePairingStateChange(server, kHAPPairingStateChange_Unpaired, server->context);
        }
    }

    // Inform application of admin or additional controller removed
    if (server->callbacks.handleControllerPairingStateChange) {
        // Copy controller pairing info
        HAPControllerPairingIdentifier pairingIdentifier;
        HAPRawBufferZero(&pairingIdentifier, sizeof pairingIdentifier);
        HAPAssert(sizeof pairingIdentifier.bytes >= pairing->numIdentifierBytes);
        HAPRawBufferCopyBytes(pairingIdentifier.bytes, pairing->identifier.bytes, pairing->numIdentifierBytes);
        pairingIdentifier.numBytes = pairing->numIdentifierBytes;

        HAPControllerPublicKey* publicKey;
        HAPAssert(sizeof publicKey->bytes == sizeof pairing->publicKey.value);
        publicKey = (HAPControllerPublicKey*) &pairing->publicKey.value;
        server->callbacks.handleControllerPairingStateChange(
                server, kHAPControllerPairingStateChange_Unpaired, &pairingIdentifier, publicKey, NULL);
    }
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------
// HAPPairingFind

typedef struct {
    const HAPPairingID* identifier;
    size_t numIdentifierBytes;
    HAPPairing* _Nullable pairing;
    HAPPairingIndex* _Nullable pairingIndex;
    bool* found;
} FindPairingEnumerateContext;

HAP_RESULT_USE_CHECK
static HAPError FindPairingEnumerateCallback(
        void* _Nullable context,
        HAPAccessoryServer* server,
        HAPPairingIndex pairingIndex,
        bool* shouldContinue) {
    HAPPrecondition(context);
    FindPairingEnumerateContext* arguments = context;
    HAPPrecondition(server);
    HAPPrecondition(arguments->found);
    HAPPrecondition(!*arguments->found);
    HAPPrecondition(shouldContinue);

    HAPError err;

    bool exists;
    HAPPairing pairing;
    err = HAPPairingGet(server, pairingIndex, &pairing, &exists);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }
    if (!exists) {
        HAPLogError(&logObject, "%s: Pairing with ID %u not found.", __func__, pairingIndex);
        HAPFatalError();
    }

    // Check if pairing found.
    if (pairing.numIdentifierBytes != arguments->numIdentifierBytes) {
        return kHAPError_None;
    }
    if (!HAPRawBufferAreEqual(pairing.identifier.bytes, arguments->identifier->bytes, arguments->numIdentifierBytes)) {
        return kHAPError_None;
    }

    // Pairing found.
    *arguments->found = true;
    if (arguments->pairingIndex) {
        *arguments->pairingIndex = pairingIndex;
    }
    if (arguments->pairing) {
        HAPRawBufferCopyBytes(HAPNonnullVoid(arguments->pairing), &pairing, sizeof pairing);
    }
    *shouldContinue = false;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPairingFind(
        HAPAccessoryServer* server,
        const HAPPairingID* identifier,
        size_t numIdentifierBytes,
        HAPPairing* _Nullable pairing,
        HAPPairingIndex* _Nullable pairingIndex,
        bool* found) {
    HAPPrecondition(server);
    HAPPrecondition(identifier);
    HAPPrecondition(numIdentifierBytes <= sizeof identifier->bytes);
    HAPPrecondition(found);

    HAPError err;

    *found = false;
    FindPairingEnumerateContext context;
    context.identifier = identifier;
    context.numIdentifierBytes = numIdentifierBytes;
    context.pairing = pairing;
    context.pairingIndex = pairingIndex;
    context.found = found;

    err = HAPPairingEnumerate(server, FindPairingEnumerateCallback, &context);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError
        HAPPairingEnumerate(HAPAccessoryServer* server, HAPPairingEnumerateCallback callback, void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(callback);

    HAPError err;

    {
        err = HAPPairingKVSEnumerate(server, callback, context);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            return err;
        }
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPairingRemoveAll(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    HAPError err;

    err = HAPPairingKVSRemoveAll(server);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    // Update unpaired state timer if necessary
    HAPAccessoryServerUpdateUnpairedStateTimer(server);

    // Inform application of last admin controller removed
    if (server->callbacks.handlePairingStateChange) {
        server->callbacks.handlePairingStateChange(server, kHAPPairingStateChange_Unpaired, server->context);
    }

    return kHAPError_None;
}
