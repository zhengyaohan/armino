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

#include "HAP.h"

#include "ApplicationFeatures.h"

#if (HAVE_ACCESS_CODE == 1)

#include "AccessCodeHelper.h"

/**
 * Access Code write response TLV storage buffer size in bytes
 *   - 3 bytes for Operation Type type/length/value
 *   - The following multiplied by double the max number of access codes supported:
 *     - Number of bytes for Access Code Identifier field
 *     - Number of bytes for maximum Access Code length
 *     - Number of bytes for Flags field
 *     - Number of bytes for Status Code field
 *     - 2 bytes for type/length of Access Code Control Response
 *     - 8 bytes for type/length of each field in Access Code Control Response
 *     - 2 bytes for separators between each Access Code Control Response
 * Note that allowing double the max number of access codes supported is arbitrary. This supports bulk operations which
 * request to add more than max number of access codes, some of which may be invalid requests. In these cases, the
 * response should try to contain status on as many access codes as possible.
 */
#define kAccessCodeResponseBufferSize \
    (3 + ((kAccessCodeListSize * 2) * \
          (sizeof(uint32_t) + kAccessCodeMaximumLength + sizeof(uint32_t) + sizeof(uint8_t) + 2 + 8 + 2)))

/**
 * Access Code platform configuration
 */
static struct {
    /**
     * Allowed characters for the access code
     */
    uint8_t characterSet;

    /**
     * Minimum access code length
     */
    uint8_t accessCodeMinimumLength;

    /**
     * Maximum access code length
     */
    uint8_t accessCodeMaximumLength;

    /**
     * Maximum number of access codes supported
     */
    uint16_t maximumAccessCodes;

    /**
     * Counter keeping track of every change (add/update/remove) to an access code
     */
    uint16_t configurationState;

    /**
     * Key value store to store access code list
     */
    HAPPlatformKeyValueStoreRef _Nonnull keyValueStore;

    /**
     * Key value store domain dedicated for access code list
     */
    HAPPlatformKeyValueStoreDomain storeDomain;

    /**
     * Configuration state change callback function
     */
    HAPConfigurationStateChangeCallback _Nullable configurationStateChangeCallback;

    /**
     * At least one entry has been modified (add/update/remove)
     */
    bool modified;

    /**
     * Bulk operation is in progress
     */
    bool bulkOperationIsActive;

    /**
     * Whether the platform was setup.
     */
    bool isSetup;
} accessCodePlatform = { .characterSet = kHAPCharacteristicValue_AccessCode_CharacterSet_ArabicNumerals,
                         .accessCodeMinimumLength = kAccessCodeMinimumLength,
                         .accessCodeMaximumLength = kAccessCodeMaximumLength,
                         .maximumAccessCodes = kAccessCodeListSize,
                         .isSetup = false };

/**
 * Data structure of an access code stored in the key value store
 */
typedef struct {
    /**
     * Identifier of the access code
     */
    uint32_t id;

    /**
     * Access code as a null-terminated string
     */
    char accessCode[kAccessCodeMaximumLength + 1];

    /**
     * Bitmask indicating the restrictions on the access code
     */
    uint32_t flags;
} AccessCodeListEntry;

/**
 * Entire access code list value stored in the key value storage
 */
typedef struct {
    /**
     * Number of access code list entries
     */
    uint8_t numEntries;

    /**
     * Access code list
     */
    AccessCodeListEntry entries[kAccessCodeListSize];
} AccessCodeList;

/**
 * Access code list
 */
static AccessCodeList accessCodeList = { .numEntries = 0 };

/**
 * log object
 */
static const HAPLogObject accessCodeLogObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "AccessCode" };

/**
 * Key value storage key for access code list
 */
#define kAccessCodeListKey ((HAPPlatformKeyValueStoreKey) 0)

/**
 * Key for storing configuration state
 */
#define kAccessCodeConfigurationStateKey ((HAPPlatformKeyValueStoreKey) 1)

/**
 * Gets a new identifier value
 *
 * Note that bulk operation must have started before using this function.
 *
 * @return new identifier value
 */
HAP_RESULT_USE_CHECK
static uint32_t GetNewIdentifier(void) {
    uint32_t higherThanUsed = 0;
    uint32_t smallestUsed = UINT32_MAX;

    for (size_t i = 0; i < accessCodeList.numEntries; i++) {
        if (higherThanUsed <= accessCodeList.entries[i].id) {
            HAPAssert(accessCodeList.entries[i].id != UINT32_MAX);
            higherThanUsed = accessCodeList.entries[i].id + 1;
        }
        if (smallestUsed > accessCodeList.entries[i].id) {
            smallestUsed = accessCodeList.entries[i].id;
        }
    }
    // Prefer to use a higher number till rollover in order to minimize the race condition of controllers access
    // assuming it's unlikely to reach the highest (UINT32_MAX) in the life time of the accessory.
    if (higherThanUsed != UINT32_MAX) {
        return higherThanUsed;
    }

    // Once the highest number is reached. Pick an unused one.
    if (smallestUsed != 0) {
        return smallestUsed - 1;
    }

    // Even id == 0 is used
    uint32_t id;
    for (id = 1; id < UINT32_MAX; id++) {
        size_t j;
        for (j = 0; j < accessCodeList.numEntries; j++) {
            if (id == accessCodeList.entries[j].id) {
                break;
            }
        }
        if (j == accessCodeList.numEntries) {
            // i is unused
            break;
        }
    }

    // Access Code List size is less than or equal to 256 and hence, there has to be an unused identifier.
    HAPAssert(id != UINT32_MAX);

    return id;
}

HAPError AccessCodeCreate(
        HAPPlatformKeyValueStoreRef _Nonnull keyValueStore,
        HAPPlatformKeyValueStoreDomain storeDomain,
        HAPConfigurationStateChangeCallback _Nullable configurationStateChangeCallback,
        HAPAccessoryServerOptions* _Nonnull hapAccessoryServerOptions) {
    accessCodePlatform.keyValueStore = keyValueStore;
    accessCodePlatform.storeDomain = storeDomain;
    accessCodePlatform.configurationStateChangeCallback = configurationStateChangeCallback;
    accessCodePlatform.configurationState = 0;
    accessCodePlatform.modified = false;
    accessCodePlatform.bulkOperationIsActive = false;
    accessCodePlatform.isSetup = true;

    static uint8_t accessCodeResponseBuffer[kAccessCodeResponseBufferSize];
    static HAPAccessCodeResponseStorage accessCodeResponseStorage = { .bytes = accessCodeResponseBuffer,
                                                                      .maxBytes = sizeof accessCodeResponseBuffer };
    hapAccessoryServerOptions->accessCode.responseStorage = &accessCodeResponseStorage;
    hapAccessoryServerOptions->accessCode.handleOperation = AccessCodeHandleOperation;

    // Read access code list and configuration state in key value store into cache
    return AccessCodeRestart();
}

HAPError AccessCodeRestart(void) {
    if (!accessCodePlatform.isSetup) {
        HAPLogError(&accessCodeLogObject, "Platform wasn't setup yet");
        return kHAPError_InvalidState;
    }

    accessCodePlatform.bulkOperationIsActive = false;
    accessCodePlatform.isSetup = false;

    size_t numBytes;
    bool found;

    // TODO: If the length of the value of key value store must be restricted as is the case for the mock implementation
    //       (128 bytes), the following have to be rewritten to split the list into multiple values.

    // Caches the access code list
    HAPError err = HAPPlatformKeyValueStoreGet(
            accessCodePlatform.keyValueStore,
            accessCodePlatform.storeDomain,
            kAccessCodeListKey,
            &accessCodeList,
            sizeof accessCodeList,
            &numBytes,
            &found);

    if (err) {
        return err;
    }

    if (!found) {
        accessCodeList.numEntries = 0;
        accessCodePlatform.isSetup = true;
    } else {
        if (numBytes !=
            HAP_OFFSETOF(AccessCodeList, entries) + accessCodeList.numEntries * sizeof(AccessCodeListEntry)) {
            HAPLogError(
                    &accessCodeLogObject,
                    "Stored access code list size %zu mismatches expected %zu",
                    numBytes,
                    HAP_OFFSETOF(AccessCodeList, entries) + accessCodeList.numEntries * sizeof(AccessCodeListEntry));
            // VENDOR-TODO: Vendor must implement migrating key value store from an old version of firmware to a new
            // version so that this would never occur.
            return kHAPError_Unknown;
        }
    }

    // Cache configuration state value
    err = HAPPlatformKeyValueStoreGet(
            accessCodePlatform.keyValueStore,
            accessCodePlatform.storeDomain,
            kAccessCodeConfigurationStateKey,
            &accessCodePlatform.configurationState,
            sizeof accessCodePlatform.configurationState,
            &numBytes,
            &found);
    if (err) {
        return err;
    }

    if (!found) {
        accessCodePlatform.configurationState = 0;
        return kHAPError_None;
    }

    if (numBytes != sizeof accessCodePlatform.configurationState) {
        HAPLogError(
                &accessCodeLogObject,
                "Size mismatch for configuration state: actual=%zu, expected=%zu",
                numBytes,
                sizeof accessCodePlatform.configurationState);
        return kHAPError_Unknown;
    }

    accessCodePlatform.isSetup = true;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError AccessCodeClear(void) {
    if (!accessCodePlatform.isSetup) {
        HAPLogError(&accessCodeLogObject, "Platform wasn't setup yet");
        return kHAPError_InvalidState;
    }

    if (accessCodePlatform.bulkOperationIsActive) {
        accessCodePlatform.bulkOperationIsActive = false;
    }

    accessCodeList.numEntries = 0;

    HAPError err = HAPPlatformKeyValueStoreSet(
            accessCodePlatform.keyValueStore,
            accessCodePlatform.storeDomain,
            kAccessCodeListKey,
            &accessCodeList,
            HAP_OFFSETOF(AccessCodeList, entries));

    return err;
}

/**
 * Enumerates access codes
 *
 * @param      enumerateCallback  callback function to call per each access code identifier.
 *
 * @return @ref kHAPError_None when successfully.
 *         @ref kHAPError_Unknown for failure for an unknown reason
 */
static HAP_RESULT_USE_CHECK HAPError
        AccessCodeEnumerate(HAPAccessCodeIdentifierCallback _Nonnull enumerateCallback, void* _Nullable arg) {
    HAPPrecondition(enumerateCallback);

    if (!accessCodePlatform.isSetup) {
        HAPLogError(&accessCodeLogObject, "Platform wasn't setup yet");
        return kHAPError_InvalidState;
    }

    for (size_t i = 0; i < accessCodeList.numEntries; i++) {
        enumerateCallback(accessCodeList.entries[i].id, arg);
    }

    return kHAPError_None;
}

/**
 * Reads an access code.
 *
 * @param[inout] op               operation input and output
 *
 * @return @ref kHAPError_None when successfully read or the access code does not exist<br>
 *         @ref kHAPError_Unknown for failure for other reasons
 */
static HAP_RESULT_USE_CHECK HAPError AccessCodeRead(HAPAccessCodeOperation* _Nonnull op) {
    HAPPrecondition(op);

    if (!accessCodePlatform.isSetup) {
        HAPLogError(&accessCodeLogObject, "Platform wasn't setup yet");
        return kHAPError_InvalidState;
    }

    if (!accessCodePlatform.bulkOperationIsActive) {
        HAPLogError(&accessCodeLogObject, "Bulk operation not in progress");
        return kHAPError_InvalidState;
    }

    for (size_t i = 0; i < accessCodeList.numEntries; i++) {
        if (accessCodeList.entries[i].id == op->identifier) {
            // match found
            op->found = true;
            op->accessCode = accessCodeList.entries[i].accessCode;
            op->flags = accessCodeList.entries[i].flags;
            return kHAPError_None;
        }
    }

    op->found = false;
    return kHAPError_None;
}

/**
 * Starts a bulk operation on access codes.
 *
 * Platform should queue all the operations and execute them together at the @ref
 * AccessCodeBulkOperationCommit() call.
 *
 * @return kHAPError_None when successful.
 */
static HAP_RESULT_USE_CHECK HAPError AccessCodeBulkOperationStart(void) {
    if (!accessCodePlatform.isSetup) {
        HAPLogError(&accessCodeLogObject, "Platform wasn't setup yet");
        return kHAPError_InvalidState;
    }

    if (accessCodePlatform.bulkOperationIsActive) {
        HAPLogError(&accessCodeLogObject, "Bulk operation already in progress");
        return kHAPError_Busy;
    }

    accessCodePlatform.bulkOperationIsActive = true;

    return kHAPError_None;
}

/**
 * Commits the queued bulk operation.
 *
 * @return kHAPError_Unknown if commit fails, this should be an erroneous case. kHAPError_None if commit was successful.
 */
static HAP_RESULT_USE_CHECK HAPError AccessCodeBulkOperationCommit(void) {
    if (!accessCodePlatform.isSetup) {
        HAPLogError(&accessCodeLogObject, "Platform wasn't setup yet");
        return kHAPError_InvalidState;
    }

    if (!accessCodePlatform.bulkOperationIsActive) {
        HAPLogError(&accessCodeLogObject, "Bulk operation not in progress");
        return kHAPError_InvalidState;
    }

    HAPError err = kHAPError_None;

    // Only commit if entries have been modified
    if (accessCodePlatform.modified) {
        err = HAPPlatformKeyValueStoreSet(
                accessCodePlatform.keyValueStore,
                accessCodePlatform.storeDomain,
                kAccessCodeListKey,
                &accessCodeList,
                HAP_OFFSETOF(AccessCodeList, entries) + accessCodeList.numEntries * sizeof(AccessCodeListEntry));
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }

        // Update configuration state
        accessCodePlatform.configurationState++;
        err = HAPPlatformKeyValueStoreSet(
                accessCodePlatform.keyValueStore,
                accessCodePlatform.storeDomain,
                kAccessCodeConfigurationStateKey,
                &accessCodePlatform.configurationState,
                sizeof accessCodePlatform.configurationState);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }
        if (accessCodePlatform.configurationStateChangeCallback) {
            accessCodePlatform.configurationStateChangeCallback();
        }

        accessCodePlatform.modified = false;
    }

    accessCodePlatform.bulkOperationIsActive = false;

    return err;
}

/**
 * Aborts the queued bulk operation.
 */
static HAP_RESULT_USE_CHECK HAPError AccessCodeBulkOperationAbort(void) {
    if (!accessCodePlatform.isSetup) {
        HAPLogError(&accessCodeLogObject, "Platform wasn't setup yet");
        return kHAPError_InvalidState;
    }

    if (!accessCodePlatform.bulkOperationIsActive) {
        HAPLogError(&accessCodeLogObject, "Bulk operation not in progress");
        return kHAPError_InvalidState;
    }

    accessCodePlatform.modified = false;
    accessCodePlatform.bulkOperationIsActive = false;
    return kHAPError_None;
}

/**
 * Checks whether access code is OK based on supported character set.
 *
 * @param      accessCode    null termninated access code
 * @param[out] op            HAPAccessCodeOperation data structure to set failure flags if any.
 *
 * @return true when the access code is acceptable. false, otherwise.
 */
static HAP_RESULT_USE_CHECK bool
        AccessCodeChecksOk(const char* _Nonnull accessCode, HAPAccessCodeOperation* _Nonnull op) {
    HAPPrecondition(accessCode);
    HAPPrecondition(op);

    bool result = true;
    // Currently only supports arabic numerals
    size_t length = HAPStringGetNumBytes(accessCode);
    if (length < accessCodePlatform.accessCodeMinimumLength) {
        op->parameterTooShort = true;
        result = false;
    } else if (length > accessCodePlatform.accessCodeMaximumLength) {
        op->parameterTooLong = true;
        result = false;
    }
    for (size_t i = 0; i < length; i++) {
        if (!HAPASCIICharacterIsNumber(accessCode[i])) {
            op->invalidCharacterSet = true;
            result = false;
            break;
        }
    }
    return result;
}

/**
 * Adds an access code.
 *
 * @param[inout] op               operation input and output
 *
 * @return @ref kHAPError_None when successful including the case where the access code is duplicate<br>
 *         @ref kHAPError_Unknown for failure for other reasons
 */
static HAP_RESULT_USE_CHECK HAPError AccessCodeAdd(HAPAccessCodeOperation* _Nonnull op) {
    HAPPrecondition(op);
    HAPPrecondition(op->accessCode);

    if (!accessCodePlatform.isSetup) {
        HAPLogError(&accessCodeLogObject, "Platform wasn't setup yet");
        return kHAPError_InvalidState;
    }

    if (!accessCodePlatform.bulkOperationIsActive) {
        HAPLogError(&accessCodeLogObject, "Bulk operation not in progress");
        return kHAPError_InvalidState;
    }

    if (!AccessCodeChecksOk(op->accessCode, op)) {
        HAPLogError(&accessCodeLogObject, "Bad access code");
        return kHAPError_None;
    }

    // Check duplicate access code
    for (size_t i = 0; i < accessCodeList.numEntries; i++) {
        if (HAPStringAreEqual(accessCodeList.entries[i].accessCode, op->accessCode)) {
            // Duplicate access code found
            op->duplicate = true;

            // Set output values for duplicate entry
            op->identifier = accessCodeList.entries[i].id;
            op->flags = accessCodeList.entries[i].flags;
            return kHAPError_None;
        }
    }

    if (accessCodeList.numEntries >= accessCodePlatform.maximumAccessCodes) {
        op->outOfResources = true;
        return kHAPError_None;
    }

    AccessCodeListEntry* entry = &accessCodeList.entries[accessCodeList.numEntries];
    entry->id = GetNewIdentifier();

    size_t length = HAPStringGetNumBytes(op->accessCode) + 1;
    HAPAssert(length <= sizeof entry->accessCode);
    HAPRawBufferCopyBytes(entry->accessCode, op->accessCode, length);

    // Flags always 0 as there are no restrictions
    entry->flags = 0;

    accessCodeList.numEntries++;

    // Set output values for new entry
    op->identifier = entry->id;
    op->flags = entry->flags;

    accessCodePlatform.modified = true;
    return kHAPError_None;
}

/**
 * Updates an access code.
 *
 * @param[inout] op               operation input and output
 *
 * @return @ref kHAPError_None when successfully updated or
 *              when the access code corresponding to the identifier does not exist, or
 *              when the new access code is duplicate to another access code<br>
 *         @ref kHAPError_Unknown for failure for other reasons
 */
static HAP_RESULT_USE_CHECK HAPError AccessCodeUpdate(HAPAccessCodeOperation* _Nonnull op) {
    HAPPrecondition(op);

    if (!accessCodePlatform.isSetup) {
        HAPLogError(&accessCodeLogObject, "Platform wasn't setup yet");
        return kHAPError_InvalidState;
    }

    if (!accessCodePlatform.bulkOperationIsActive) {
        HAPLogError(&accessCodeLogObject, "Bulk operation not in progress");
        return kHAPError_InvalidState;
    }

    if (op->accessCode && !AccessCodeChecksOk(op->accessCode, op)) {
        HAPLogError(&accessCodeLogObject, "Bad access code");
        return kHAPError_None;
    }

    for (size_t i = 0; i < accessCodeList.numEntries; i++) {
        if (accessCodeList.entries[i].id == op->identifier) {
            // match found
            op->found = true;

            if (op->accessCode) {
                // Check for duplicate access code
                for (size_t j = 0; j < accessCodeList.numEntries; j++) {
                    if (HAPStringAreEqual(accessCodeList.entries[j].accessCode, op->accessCode)) {
                        // Duplicate access code found
                        op->duplicate = true;

                        // Set output values for duplicate entry
                        op->identifier = accessCodeList.entries[j].id;
                        op->flags = accessCodeList.entries[j].flags;
                        return kHAPError_None;
                    }
                }
            }

            AccessCodeListEntry* entry = &accessCodeList.entries[i];

            // Update the access code value only
            if (op->accessCode) {
                size_t length = HAPStringGetNumBytes(op->accessCode) + 1;
                HAPAssert(length <= sizeof entry->accessCode);
                HAPRawBufferCopyBytes(entry->accessCode, op->accessCode, length);
            }

            // Set output values for updated entry
            op->flags = entry->flags;

            accessCodePlatform.modified = true;
            break;
        }
    }

    return kHAPError_None;
}

/**
 * Removes an access code.
 *
 * @param[inout] op      Operation input and output
 *
 * @return @ref kHAPError_None when successfully including when the access code wasn't present.
 *         @ref kHAPError_Unknown for failure for an unknown reason
 */
static HAP_RESULT_USE_CHECK HAPError AccessCodeRemove(HAPAccessCodeOperation* _Nonnull op) {
    HAPPrecondition(op);

    if (!accessCodePlatform.isSetup) {
        HAPLogError(&accessCodeLogObject, "Platform wasn't setup yet");
        return kHAPError_InvalidState;
    }

    if (!accessCodePlatform.bulkOperationIsActive) {
        HAPLogError(&accessCodeLogObject, "Bulk operation not in progress");
        return kHAPError_InvalidState;
    }

    for (size_t i = 0; i < accessCodeList.numEntries; i++) {
        if (accessCodeList.entries[i].id == op->identifier) {
            // match found
            op->found = true;

            // Set output values for removed entry
            HAPRawBufferCopyBytes(
                    op->accessCode, accessCodeList.entries[i].accessCode, sizeof accessCodeList.entries[i].accessCode);
            op->flags = accessCodeList.entries[i].flags;

            // Delete the entry by copying over the rest
            size_t remainderCount = accessCodeList.numEntries - i - 1;
            if (remainderCount > 0) {
                HAPRawBufferCopyBytes(
                        &accessCodeList.entries[i],
                        &accessCodeList.entries[i + 1],
                        remainderCount * sizeof(AccessCodeListEntry));
            }
            accessCodeList.numEntries--;

            accessCodePlatform.modified = true;
            break;
        }
    }

    return kHAPError_None;
}

/**
 * Access Code operation callback function
 *
 * @param[inout] op     operation and result
 * @param        ctx    context. Unused in this implementation
 *
 * @return kHAPError_None when operation could result into a response whether successful or not.<br>
 *         Other error code when an unexpected error occurred and a normal response cannot be generated.
 */
HAPError AccessCodeHandleOperation(HAPAccessCodeOperation* _Nonnull op, void* _Nullable ctx HAP_UNUSED) {
    HAPPrecondition(op);

    op->found = false;
    op->duplicate = false;
    op->outOfResources = false;
    op->parameterTooShort = false;
    op->parameterTooLong = false;
    op->invalidCharacterSet = false;

    switch (op->type) {
        case kHAPAccessCodeOperationType_EnumerateIdentifiers: {
            return AccessCodeEnumerate(op->enumerateCallback, op->enumerateContext);
        }
        case kHAPAccessCodeOperationType_Read: {
            return AccessCodeRead(op);
        }
        case kHAPAccessCodeOperationType_BulkOperationStart: {
            return AccessCodeBulkOperationStart();
        }
        case kHAPAccessCodeOperationType_BulkOperationCommit: {
            return AccessCodeBulkOperationCommit();
        }
        case kHAPAccessCodeOperationType_BulkOperationAbort: {
            return AccessCodeBulkOperationAbort();
        }
        case kHAPAccessCodeOperationType_QueueAdd: {
            return AccessCodeAdd(op);
        }
        case kHAPAccessCodeOperationType_QueueUpdate: {
            return AccessCodeUpdate(op);
        }
        case kHAPAccessCodeOperationType_QueueRemove: {
            return AccessCodeRemove(op);
        }
    }

    HAPLogError(&accessCodeLogObject, "Unknown Access Code operation %u", op->type);
    return kHAPError_Unknown;
}

HAP_RESULT_USE_CHECK
HAPError HandleAccessCodeSupportedConfigurationRead(
        HAPAccessoryServer* _Nonnull server,
        const HAPTLV8CharacteristicReadRequest* _Nonnull request,
        HAPTLVWriter* _Nonnull responseWriter,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_AccessCodeSupportedConfiguration));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_AccessCode));
    HAPPrecondition(request->accessory->aid == 1);
    HAPPrecondition(responseWriter);

    // The parameters are specific to this application helper implementation.
    return HAPBuildAccessCodeSupportedConfigurationResponse(
            responseWriter,
            accessCodePlatform.characterSet,
            accessCodePlatform.accessCodeMinimumLength,
            accessCodePlatform.accessCodeMaximumLength,
            accessCodePlatform.maximumAccessCodes);
}

HAP_RESULT_USE_CHECK
HAPError HandleAccessCodeConfigurationStateRead(
        HAPAccessoryServer* _Nonnull server,
        const HAPUInt16CharacteristicReadRequest* _Nonnull request HAP_UNUSED,
        uint16_t* _Nonnull value,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(value);

    *value = accessCodePlatform.configurationState;
    HAPLogDebug(&accessCodeLogObject, "Access Code Configuration State read: %u", (unsigned) *value);

    return kHAPError_None;
}

#if (HAP_TESTING == 1)
HAP_RESULT_USE_CHECK
bool AccessCodeLookUp(const char* _Nonnull accessCode, uint32_t* _Nonnull accessCodeIdentifier) {
    HAPPrecondition(accessCode);
    HAPPrecondition(accessCodeIdentifier);

    for (size_t i = 0; i < accessCodeList.numEntries; i++) {
        if (HAPStringAreEqual(accessCodeList.entries[i].accessCode, accessCode)) {
            // Access code found
            *accessCodeIdentifier = accessCodeList.entries[i].id;
            return true;
        }
    }

    return false;
}

HAP_RESULT_USE_CHECK
HAPError AccessCodeSetCharacterSet(uint8_t value) {
    if (value != kHAPCharacteristicValue_AccessCode_CharacterSet_ArabicNumerals) {
        HAPLog(&accessCodeLogObject, "Access code character set must be Arabic numerals");
        return kHAPError_InvalidData;
    }

    accessCodePlatform.characterSet = value;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError AccessCodeSetMinimumLength(uint8_t value) {
    if (value == 0) {
        HAPLog(&accessCodeLogObject, "Access code minimum length cannot be 0");
        return kHAPError_InvalidData;
    }

    if (value > accessCodePlatform.accessCodeMaximumLength) {
        HAPLog(&accessCodeLogObject,
               "Access code minimum length cannot be greater than maximum length %d",
               accessCodePlatform.accessCodeMaximumLength);
        return kHAPError_InvalidData;
    }

    accessCodePlatform.accessCodeMinimumLength = value;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError AccessCodeSetMaximumLength(uint8_t value) {
    if (value > kAccessCodeMaximumLength) {
        HAPLog(&accessCodeLogObject, "Access code maximum length cannot be greater than %d", kAccessCodeMaximumLength);
        return kHAPError_InvalidData;
    }

    if (value < accessCodePlatform.accessCodeMinimumLength) {
        HAPLog(&accessCodeLogObject,
               "Access code maximum length cannot be less than minimum length %d",
               accessCodePlatform.accessCodeMinimumLength);
        return kHAPError_InvalidData;
    }

    accessCodePlatform.accessCodeMaximumLength = value;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError AccessCodeSetMaximumAccessCodes(uint16_t value) {
    if (value > kAccessCodeListSize) {
        HAPLog(&accessCodeLogObject, "Maximum number of access codes cannot exceed %d", kAccessCodeListSize);
        return kHAPError_InvalidData;
    }

    accessCodePlatform.maximumAccessCodes = value;
    return kHAPError_None;
}
#endif // (HAP_TESTING == 1)
#endif
