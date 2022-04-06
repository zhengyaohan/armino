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

#ifndef HAP_PLATFORM_KEY_VALUE_STORE_H
#define HAP_PLATFORM_KEY_VALUE_STORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPBase.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * @file
 *
 * HAP expects to have a few KB of mutable persistent memory for its keys and other information. The data is stored as
 * a set of key-value pairs.
 *
 * Expected behavior:
 * Several instances of a key-value store must be supported (domains), so that information owned by the accessory, by
 * the SDK provider and by the HAP implementation can be separately managed. The following domains are defined:
 * - `0x00–0x3F` is reserved for accessory developers
 * - `0x40-0x7F` is reserved for platform developers
 * - `0x80-0xFF` is reserved for the HAP Library
 *
 * In every domain, keys in the range 0x00 to 0xFF can be used.
 *
 * Note that the function `HAPKeystoreRestoreFactorySettings` only touches domains 0x80 through 0xFF. This means that
 * provisioning information that is stored in the key-value store (domain 0x40 in the POSIX and BLE sample PALs)
 * survives such a factory reset as required by the HAP specification. However, it is up to the platform developer
 * where the provisioning information is stored, it does not have to be in the key-value store. Adjust
 * `HAPPlatformAccessorySetup` if you want to use another backing store for provisioning.
 *
 * Storage of key-value pairs is possible in files or directly on a micro-controller on-chip flash or EEPROM memory.
 * A flash-based implementation of the key-value store may require up to twice as much flash memory compared to what
 * HAP may require. The additional flash memory may be required e.g. because a copy of a flash sector has to be written
 * before deleting the old sector.
 *
 * The maximum theoretical possible capacity of the key-value store is `256 * 256` key-value pairs. The HAP Library owns
 * `128 * 256` of these pairs, the PAL owns `64 * 256` pairs, and the accessory logic also owns `64 * 256` pairs. Note
 * that PAL and accessory logic are not required to use the parts of the key-value store that are defined for them.
 * These parts are merely defined for convenience, and also used in this way in the ADK reference implementations.
 *
 * In practice, only few pairs are used (sparse structure). Actual use depends on parameters like the number of
 * pairings, the given accessory attribute database, IP vs BLE capabilities, etc. It is expected that a total size of
 * `4 KB` is sufficient if the accessory logic does not use the key-value store.
 *
 * Values have different sizes depending on the key; the ones used by HAP Library stay within reasonable bounds
 * (up to `128 bytes` for the longest). In the PAL, there are larger values, for example to store SRP salt and verifier.
 * But PAL code is provided in source form, so these values can be split across multiple keys if necessary.
 */

/**
 * Key-value store.
 */
typedef struct HAPPlatformKeyValueStore HAPPlatformKeyValueStore;
typedef struct HAPPlatformKeyValueStore* HAPPlatformKeyValueStoreRef;
HAP_NONNULL_SUPPORT(HAPPlatformKeyValueStore)

/**
 * Domain.
 *
 * Domain ownership:
 * - 0x00-0x3F - Accessory manufacturer.
 * - 0x40-0x7F - SDK developer.
 * - 0x80-0xFF - Reserved for core implementation.
 */
typedef uint8_t HAPPlatformKeyValueStoreDomain;

/**
 * Key. Semantics depend on domain.
 */
typedef uint8_t HAPPlatformKeyValueStoreKey;

/**
 * Fetches the value of a key in a domain.
 *
 * @param      keyValueStore        Key-value store.
 * @param      domain               Domain to search.
 * @param      key                  Key to fetch value of.
 * @param[out] bytes                On output, value of key, if found, truncated up to maxBytes bytes.
 * @param      maxBytes             Capacity of bytes buffer.
 * @param[out] numBytes             Effective length of bytes buffer, if found.
 * @param[out] found                Whether or not a key with a value has been found.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If persistent store access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformKeyValueStoreGet(
        HAPPlatformKeyValueStoreRef keyValueStore,
        HAPPlatformKeyValueStoreDomain domain,
        HAPPlatformKeyValueStoreKey key,
        void* _Nullable bytes,
        size_t maxBytes,
        size_t* _Nullable numBytes,
        bool* found);

/**
 * Sets the value of a key in a domain to the contents of a buffer.
 *
 * @param      keyValueStore        Key-value store.
 * @param      domain               Domain to modify.
 * @param      key                  Key to modify.
 * @param      bytes                Buffer that contains the value to set.
 * @param      numBytes             Length of bytes buffer.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If persistent store access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformKeyValueStoreSet(
        HAPPlatformKeyValueStoreRef keyValueStore,
        HAPPlatformKeyValueStoreDomain domain,
        HAPPlatformKeyValueStoreKey key,
        const void* bytes,
        size_t numBytes);

/**
 * Sets the value of a key in a domain to the contents of a buffer.
 *
 * While doing so, this function removes all pending (unexecuted) previous operations to set the value
 * of the same key in the same domain.
 * This function is useful for optimized use for key value which changes frequently and which does not
 * affect integrity of other key values.
 *
 * @param      keyValueStore        Key-value store.
 * @param      domain               Domain to modify.
 * @param      key                  Key to modify.
 * @param      bytes                Buffer that contains the value to set.
 * @param      numBytes             Length of bytes buffer.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If persistent store access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformKeyValueStoreOverrideAndSet(
        HAPPlatformKeyValueStoreRef keyValueStore,
        HAPPlatformKeyValueStoreDomain domain,
        HAPPlatformKeyValueStoreKey key,
        const void* bytes,
        size_t numBytes);

/**
 * Removes the value of a key in a domain.
 *
 * @param      keyValueStore        Key-value store.
 * @param      domain               Domain to modify.
 * @param      key                  Key to remove.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If persistent store access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformKeyValueStoreRemove(
        HAPPlatformKeyValueStoreRef keyValueStore,
        HAPPlatformKeyValueStoreDomain domain,
        HAPPlatformKeyValueStoreKey key);

/**
 * Callback that should be invoked for each key-value store entry.
 *
 * @param      context              Context.
 * @param      keyValueStore        Key-value store.
 * @param      domain               Domain of key-value store entry.
 * @param      key                  Key of key-value store entry.
 * @param[in,out] shouldContinue    True if enumeration shall continue, False otherwise. Is set to true on input.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If persistent store access failed.
 */
HAP_RESULT_USE_CHECK
typedef HAPError (*HAPPlatformKeyValueStoreEnumerateCallback)(
        void* _Nullable context,
        HAPPlatformKeyValueStoreRef keyValueStore,
        HAPPlatformKeyValueStoreDomain domain,
        HAPPlatformKeyValueStoreKey key,
        bool* shouldContinue);

/**
 * Enumerates keys in a domain.
 *
 * @param      keyValueStore        Key-value store.
 * @param      domain               Domain to enumerate.
 * @param      callback             Function to call on each key-value store entry.
 * @param      context              Context that is passed to the callback.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If persistent store access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformKeyValueStoreEnumerate(
        HAPPlatformKeyValueStoreRef keyValueStore,
        HAPPlatformKeyValueStoreDomain domain,
        HAPPlatformKeyValueStoreEnumerateCallback callback,
        void* _Nullable context);

/**
 * Removes values of all keys in a domain.
 *
 * @param      keyValueStore        Key-value store.
 * @param      domain               Domain to purge.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If persistent store access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformKeyValueStorePurgeDomain(
        HAPPlatformKeyValueStoreRef keyValueStore,
        HAPPlatformKeyValueStoreDomain domain);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
