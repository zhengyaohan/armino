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

#ifndef HAP_PLATFORM_NFC_ACCESS_H
#define HAP_PLATFORM_NFC_ACCESS_H

#ifdef __cplusplus
extern "C" {
#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_NFC_ACCESS)

HAP_ENUM_BEGIN(uint8_t, NfcAccessStatusCode) {
    NFC_ACCESS_STATUS_CODE_SUCCESS = 0,
    NFC_ACCESS_STATUS_CODE_OUT_OF_RESOURCES,
    NFC_ACCESS_STATUS_CODE_DUPLICATE,
    NFC_ACCESS_STATUS_CODE_DOES_NOT_EXIST,
    NFC_ACCESS_STATUS_CODE_NOT_SUPPORTED
} HAP_ENUM_END(uint8_t, NfcAccessStatusCode);

/**
 * The possible actions that results in caching an issuer key
 */
HAP_ENUM_BEGIN(uint8_t, NfcAccessIssuerKeyCacheType) {
    // Update cache for HAP pairing LTPK issuer key from existing HAP pairing
    NFC_ACCESS_ISSUER_KEY_CACHE_TYPE_HAP_PAIRING_READ = 0,
    // Update cache for HAP pairing LTPK issuer key from adding a new HAP pairing
    NFC_ACCESS_ISSUER_KEY_CACHE_TYPE_HAP_PAIRING_ADD,
    // Update cache for HAP pairing LTPK issuer key from removing a HAP pairing
    NFC_ACCESS_ISSUER_KEY_CACHE_TYPE_HAP_PAIRING_REMOVE,
    // Update cache for the issuer key from an add write request (non-Home user)
    NFC_ACCESS_ISSUER_KEY_CACHE_TYPE_REQUEST_ADD,
    // Update cache for the issuer key from a remove write request (non-Home user)
    NFC_ACCESS_ISSUER_KEY_CACHE_TYPE_REQUEST_REMOVE
} HAP_ENUM_END(uint8_t, NfcAccessIssuerKeyCacheType);

typedef struct {
    /**
     * Type of the issuer key
     */
    uint8_t type;

    /**
     * Issuer key
     */
    uint8_t* key;

    /**
     * Issuer key size in bytes
     */
    size_t keyNumBytes;

    /**
     * Issuer key identifier
     */
    uint8_t* identifier;
} HAPPlatformNfcAccessIssuerKey;

typedef struct {
    /**
     * Type of the device credential key
     */
    uint8_t type;

    /**
     * Device credential key
     */
    uint8_t* key;

    /**
     * Device credential key size in bytes
     */
    size_t keyNumBytes;

    /**
     * Issuer key identifier
     */
    uint8_t* issuerKeyIdentifier;

    /**
     * Device credential key state
     */
    uint8_t state;

    /**
     * Device credential key identifier
     */
    uint8_t* identifier;
} HAPPlatformNfcAccessDeviceCredentialKey;

typedef struct {
    /**
     * Type of the reader key
     */
    uint8_t type;

    /**
     * Reader key
     */
    uint8_t* key;

    /**
     * Reader key size in bytes
     */
    size_t keyNumBytes;

    /**
     * Reader identifier
     */
    uint8_t* readerIdentifier;

    /**
     * Reader key identifier
     */
    uint8_t* identifier;
} HAPPlatformNfcAccessReaderKey;

/**
 * Number of bytes of an NFC Access Key Identifier
 */
#define NFC_ACCESS_KEY_IDENTIFIER_BYTES 8

/**
 * NFC lock state change info for the callback function
 */
typedef struct {
    uint8_t issuerKeyIdentifier[NFC_ACCESS_KEY_IDENTIFIER_BYTES];
    bool locked;
} NfcLockStateChangeInfo;

/**
 * Configuration state change callback function
 */
typedef void (*HAPConfigurationStateChangeCallback)(void);

/**
 * NFC transaction detected callback function
 */
typedef void (*HAPNfcTransactionDetectedCallback)(NfcLockStateChangeInfo lockStateChangeInfo);

/**
 * Generates an Identifier. The Identifier is the first 8 bytes of a SHA256 hash of a salt value + key.
 *
 * @param        key           Key used to generate the SHA256 hash
 * @param        keyNumBytes   Size of key in bytes
 * @param[out]   identifier    Buffer to store the resultant identifier
 */
void HAPPlatformNfcAccessGenerateIdentifier(const uint8_t* key, size_t keyNumBytes, uint8_t* identifier);

/**
 * Creates NFC Access service platform
 *
 * Note that the implementation of this function is optional.
 * That is, HAP does not call this function.
 *
 * @param    keyValueStore                      Key value store where the NFC access key list is stored
 * @param    storeDomain                        Key value store domain dedicated to the NFC access key list
 * @param    configurationStateChangeCallback   The callback function called when configuration state has changed
 * @param    nfcTransactionDetectedCallback     The callback function called when NFC transaction has been detected
 * (lock state change)
 *
 * @return kHAPError_None when successful.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformNfcAccessCreate(
        HAPPlatformKeyValueStoreRef _Nonnull keyValueStore,
        HAPPlatformKeyValueStoreDomain storeDomain,
        HAPConfigurationStateChangeCallback _Nullable configurationStateChangeCallback,
        HAPNfcTransactionDetectedCallback _Nullable nfcTransactionDetectedCallback);

/**
 * Purges NFC Access store domain, reloads all empty key lists, and clears keys on reader
 *
 * Note that the implementation of this function is optional.
 * That is, HAP does not call this function.
 *
 * @return kHAPError_None when successful.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformNfcAccessPurge(void);

/**
 * Lists all NFC Access Issuer Keys that have been added. Note that memory for the returned data is allocated by the
 * caller.
 *
 * @param[out] issuerKeyList   Issuer key list containing data to be returned
 * @param[out] numIssuerKeys   Number of issuer keys returned in issuerKeyList
 * @param[out] statusCode      Contains valid error codes for the operation
 *
 * @return @ref kHAPError_None           Successfully retrieved all keys
 *         @ref kHAPError_InvalidState   If platform is not initialized
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformNfcAccessIssuerKeyList(
        HAPPlatformNfcAccessIssuerKey* _Nonnull issuerKeyList,
        uint8_t* _Nonnull numIssuerKeys,
        NfcAccessStatusCode* _Nonnull statusCode);

/**
 * Adds an NFC Access Issuer Key
 *
 * @param      issuerKey   Issuer key to add
 * @param      cacheType   The type of caching for this issuer key
 * @param[out] statusCode  Contains valid error codes for the operation
 *
 * @return @ref kHAPError_None           Successfully added the new key unless statusCode contains an error
 *         @ref kHAPError_InvalidState   If platform is not initialized
 *         @ref Other errors             Did not successfully add new key
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformNfcAccessIssuerKeyAdd(
        const HAPPlatformNfcAccessIssuerKey* _Nonnull issuerKey,
        NfcAccessIssuerKeyCacheType cacheType,
        NfcAccessStatusCode* _Nonnull statusCode);

/**
 * Removes an NFC Access Issuer Key
 *
 * @param      issuerKey   Issuer key to remove
 * @param      cacheType   The type of caching for this issuer key
 * @param[out] statusCode  Contains valid error codes for the operation
 *
 * @return @ref kHAPError_None           Successfully removed the key unless statusCode contains an error
 *         @ref kHAPError_InvalidState   If platform is not initialized
 *         @ref Other errors             Did not successfully remove key
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformNfcAccessIssuerKeyRemove(
        const HAPPlatformNfcAccessIssuerKey* _Nonnull issuerKey,
        NfcAccessIssuerKeyCacheType cacheType,
        NfcAccessStatusCode* _Nonnull statusCode);

/**
 * Lists all NFC Access Device Credential Keys that have been added. Note that memory for the returned data is allocated
 * by the caller.
 *
 * @param[out] deviceCredentialKeyList   Device credential key list containing data to be returned
 * @param[out] numDeviceCredentailKeys   Number of device credential keys returned in deviceCredentialKeyList
 * @param[out] statusCode                Contains valid error codes for the operation
 *
 * @return @ref kHAPError_None           Successfully retrieved all keys
 *         @ref kHAPError_InvalidState   If platform is not initialized
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformNfcAccessDeviceCredentialKeyList(
        HAPPlatformNfcAccessDeviceCredentialKey* _Nonnull deviceCredentialKeyList,
        uint8_t* _Nonnull numDeviceCredentailKeys,
        NfcAccessStatusCode* _Nonnull statusCode);

/**
 * Adds an NFC Access Device Credential Key
 *
 * @param      deviceCredentialKey   Device credential key to add
 * @param[out] statusCode            Contains valid error codes for the operation
 *
 * @return @ref kHAPError_None           Successfully added the new key unless statusCode contains an error
 *         @ref kHAPError_InvalidState   If platform is not initialized
 *         @ref Other errors             Did not successfully add new key
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformNfcAccessDeviceCredentialKeyAdd(
        const HAPPlatformNfcAccessDeviceCredentialKey* _Nonnull deviceCredentialKey,
        NfcAccessStatusCode* _Nonnull statusCode);

/**
 * Removes an NFC Access Device Credential Key
 *
 * @param      deviceCredentialKey   Device credential key to remove
 * @param[out] statusCode            Contains valid error codes for the operation
 *
 * @return @ref kHAPError_None           Successfully removed the key unless statusCode contains an error
 *         @ref kHAPError_InvalidState   If platform is not initialized
 *         @ref Other errors             Did not successfully remove key
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformNfcAccessDeviceCredentialKeyRemove(
        const HAPPlatformNfcAccessDeviceCredentialKey* _Nonnull deviceCredentialKey,
        NfcAccessStatusCode* _Nonnull statusCode);

/**
 * Lists NFC Access Reader Key that has been added. Note that memory for the returned data is allocated by the caller.
 *
 * @param[out] readerKey        Reader key containing data to be returned
 * @param[out] readerKeyFound   Indicate whether reader key exists
 * @param[out] statusCode       Contains valid error codes for the operation
 *
 * @return @ref kHAPError_None           Successfully retrieved key
 *         @ref kHAPError_InvalidState   If platform is not initialized
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformNfcAccessReaderKeyList(
        HAPPlatformNfcAccessReaderKey* _Nonnull readerKey,
        bool* _Nonnull readerKeyFound,
        NfcAccessStatusCode* _Nonnull statusCode);

/**
 * Adds an NFC Access Reader Key
 *
 * @param      issuerKey   Reader key to add
 * @param[out] statusCode  Contains valid error codes for the operation
 *
 * @return @ref kHAPError_None           Successfully added the new key unless statusCode contains an error
 *         @ref kHAPError_InvalidState   If platform is not initialized
 *         @ref Other errors             Did not successfully add new key
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformNfcAccessReaderKeyAdd(
        const HAPPlatformNfcAccessReaderKey* _Nonnull readerKey,
        NfcAccessStatusCode* _Nonnull statusCode);

/**
 * Removes an NFC Access Reader Key
 *
 * @param      readerKey   Reader key to remove
 * @param[out] statusCode  Contains valid error codes for the operation
 *
 * @return @ref kHAPError_None           Successfully removed the key unless statusCode contains an error
 *         @ref kHAPError_InvalidState   If platform is not initialized
 *         @ref Other errors             Did not successfully remove key
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformNfcAccessReaderKeyRemove(
        const HAPPlatformNfcAccessReaderKey* _Nonnull readerKey,
        NfcAccessStatusCode* _Nonnull statusCode);

/**
 * Gets maximum NFC Issuer Keys supported
 *
 * @return Maximum NFC Issuer Keys supported
 */
HAP_RESULT_USE_CHECK
uint16_t HAPPlatformNfcAccessGetMaximumIssuerKeys(void);

/**
 * Gets maximum active NFC Device Credential Keys supported
 *
 * @return Maximum active NFC Device Credential Keys supported
 */
HAP_RESULT_USE_CHECK
uint16_t HAPPlatformNfcAccessGetMaximumActiveDeviceCredentialKeys(void);

/**
 * Gets maximum suspended NFC Device Credential Keys supported
 *
 * @return Maximum suspended NFC Device Credential Keys supported
 */
HAP_RESULT_USE_CHECK
uint16_t HAPPlatformNfcAccessGetMaximumSuspendedDeviceCredentialKeys(void);

/**
 * Gets the configuration state value
 *
 * @return Configuration state value
 */
HAP_RESULT_USE_CHECK
uint16_t HAPPlatformNfcAccessGetConfigurationState(void);

#if (HAP_TESTING == 1)
/**
 * Sets maximum NFC Issuer Keys supported
 *
 * @param   value   The new maximum NFC Issuer Keys supported
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformNfcAccessSetMaximumIssuerKeys(uint16_t value);

/**
 * Sets maximum active NFC Device Credential Keys supported
 *
 * @param   value   The new maximum active NFC Device Credential Keys supported
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformNfcAccessSetMaximumActiveDeviceCredentialKeys(uint16_t value);

/**
 * Sets maximum suspended NFC Device Credential Keys supported
 *
 * @param   value   The new maximum suspended NFC Device Credential Keys supported
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformNfcAccessSetMaximumSuspendedDeviceCredentialKeys(uint16_t value);

/**
 * Finds the index of a device credential key
 *
 * @param   key          The key to search
 * @param   index[out]   The index of the key if found
 */
HAP_RESULT_USE_CHECK
HAPError FindDeviceCredentialKeyIndex(uint8_t* _Nonnull key, uint16_t* _Nonnull index);

#endif

#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
