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

#include "HAPIPGlobalStateNumber.h"

#include "HAP+KeyValueStoreDomains.h"
#include "HAPAccessoryServer+Internal.h"
#include "HAPLogSubsystem.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "IPGlobalStateNumber" };

// s# specifies the current state number.
// - Must have an initial value of 1.
// - The value must persist across reboots, power cycles, etc.
// - Must have a range of 1-65535 and wrap to 1 when it overflows.
// - Increments each time a paired accessory's state changes. Accessory state change for IP accessories
//   are defined for the following cases:
//   - Accessory reboot or restart due to any reason.
//   - Accessory re-publishes HomeKit service (i.e., changes in HomeKit-specific Bonjour text records).
//   - A characteristic that supports notification changes while a paired accessory is in disconnected state
//     (i.e., when not connected with a secure session to at least one HomeKit paired controller).
//     The state number should increment only once for multiple characteristic value changes
//     while in disconnected state until the accessory state changes from disconnected to connected state.
//   - Last HAP connection with enabled event notifications is closed due to an I/O error.
// See HomeKit Accessory Protocol Specification R17
// Table 6-7 _hap._tcp Bonjour TXT Record Keys

HAP_RESULT_USE_CHECK
HAPError HAPIPGlobalStateNumberGet(HAPAccessoryServer* server, HAPIPGlobalStateNumber* ipGSN) {
    HAPPrecondition(server);
    HAPPlatformKeyValueStoreRef keyValueStore = server->platform.keyValueStore;
    HAPPrecondition(ipGSN);

    HAPError err;

    bool found;
    size_t numBytes;
    uint8_t ipGSNBytes[sizeof *ipGSN];
    err = HAPPlatformKeyValueStoreGet(
            keyValueStore,
            kHAPKeyValueStoreDomain_Configuration,
            kHAPKeyValueStoreKey_Configuration_IPGSN,
            ipGSNBytes,
            sizeof ipGSNBytes,
            &numBytes,
            &found);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "HAPPlatformKeyValueStoreGet failed: %d.", err);
        return err;
    }
    if (!found) {
        *ipGSN = 1;
        return kHAPError_None;
    }
    if (numBytes != sizeof ipGSNBytes) {
        HAPLog(&logObject, "Invalid IP GSN length %zu.", numBytes);
        return kHAPError_Unknown;
    }

    HAPAssert(sizeof ipGSNBytes == sizeof(uint16_t));
    *ipGSN = HAPReadLittleUInt16(&ipGSNBytes[0]);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPIPGlobalStateNumberIncrement(HAPAccessoryServer* server) {
    HAPPrecondition(server);
    HAPPlatformKeyValueStoreRef keyValueStore = server->platform.keyValueStore;

    HAPError err;

    HAPIPGlobalStateNumber ipGSN;
    err = HAPIPGlobalStateNumberGet(server, &ipGSN);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    ipGSN++;
    if (!ipGSN) {
        HAPLogInfo(&logObject, "s# wrapped to 1.");
        ipGSN = 1;
    }

    HAPAssert(sizeof ipGSN == sizeof(uint16_t));
    uint8_t ipGSNBytes[] = { HAPExpandLittleUInt16(ipGSN) };
    err = HAPPlatformKeyValueStoreSet(
            keyValueStore,
            kHAPKeyValueStoreDomain_Configuration,
            kHAPKeyValueStoreKey_Configuration_IPGSN,
            ipGSNBytes,
            sizeof ipGSNBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "HAPPlatformKeyValueStoreSet failed: %d.", err);
        return err;
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPIPGlobalStateNumberReset(HAPAccessoryServer* server) {
    HAPPrecondition(server);
    HAPPlatformKeyValueStoreRef keyValueStore = server->platform.keyValueStore;

    HAPError err;

    err = HAPPlatformKeyValueStoreRemove(
            keyValueStore, kHAPKeyValueStoreDomain_Configuration, kHAPKeyValueStoreKey_Configuration_IPGSN);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "HAPPlatformKeyValueStoreRemove failed: %d.", err);
        return err;
    }

    return kHAPError_None;
}

#endif
