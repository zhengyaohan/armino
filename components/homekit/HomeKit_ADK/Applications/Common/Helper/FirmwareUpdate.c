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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// NOTE: This is a sample accessory implementation that an accessory vendor is expected to modify to integrate firmware
// update support. Please reference Documentation/firmware_update.md for more information regarding HomeKit firmware
// updates.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "FirmwareUpdate.h"
#include "ApplicationFeatures.h"

#if (HAVE_FIRMWARE_UPDATE == 1)

#include <sys/stat.h>

#if HAP_FEATURE_ENABLED(HAP_FEATURE_FILE_STORAGE)
#include "HAPPlatformFileManager.h"
#endif

#include "AccessoryInformationServiceDB.h"
#include "AppBase.h"
#include "FirmwareUpdateServiceDB.h"
#include "UARP.h"

//----------------------------------------------------------------------------------------------------------------------
// Pre-processor definitions
//----------------------------------------------------------------------------------------------------------------------

/**
 * File that holds the collated payload data of a firmware update asset.
 */
#define kFirmwareAssetFile "tmp/FirmwareUploadAsset.txt"

/**
 *  Key used in the key value store to store the firmware staging state for the current asset.
 */
#define kAppKeyValueStoreKey_FirmwareUpdate_FirmwareStagingState ((HAPPlatformKeyValueStoreKey) 0x00)

/**
 * Key used in the key value store to store the payload data state for the current asset.
 */
#define kAppKeyValueStoreKey_FirmwareUpdate_PayloadDataState ((HAPPlatformKeyValueStoreKey) 0x01)

/**
 * Key used in the key value store to store the firmware update state.
 */
#define kAppKeyValueStoreKey_FirmwareUpdate_State ((HAPPlatformKeyValueStoreKey) 0x02)

/**
 * Expected time (in seconds) the accessory will be unresponsive while a firmware update is being "applied".
 * This sample implementation simulates an apply by updating the firmware version while the accessory server is
 * still running.
 *
 * VENDOR-TODO: This should reflect the time required to boot into the new image and bring up the accessory server
 * to make the accessory reachable by HomeKit controllers. It may be a static value or set after staging is complete
 * if the duration is dependent upon properties of the staged update.
 */
#define kFirmwareUpdateDuration 60

/**
 * Sample SuperBinary payload index to stage.
 * This sample implementation stages a single payload.
 *
 * VENDOR-TODO: Usage of a single or multiple payloads and any ordering logic is up to the accessory.
 */
#define kSampleSuperBinaryPayloadIndex 0

//----------------------------------------------------------------------------------------------------------------------
// Private data types
//----------------------------------------------------------------------------------------------------------------------

/**
 * Apply context for simulating an update in the sample applications.
 */
typedef struct {
    uint16_t delay;
    HAPPlatformTimerRef timer;
    HAPAccessoryServer* server;
    HAPAccessory* accessory;
} FirmwareUpdateApplyContext;

/**
 * SuperBinary asset information.
 */
typedef struct {
    UARPVersion assetVersion;
    uint32_t assetTag;
    uint32_t assetLength;
    uint16_t assetNumPayload;
    uint32_t assetSelectedPayloadIndex;
} FirmwareUpdateStagingAssetInfo;

/**
 * Context for tracking firmware update process.
 */
typedef struct {
    FirmwareUpdateStagingAssetInfo stagingAssetInfo;
    char stagedFirmwareVersion[kHAPFirmwareVersion_MaxLength + 1];
    char updatedFirmwareVersion[kHAPFirmwareVersion_MaxLength + 1];
    HAPAccessoryFirmwareUpdateState HAPProfile;
    HAPPlatformKeyValueStoreRef keyValueStore;
    FirmwareUpdateOptions options;
    FirmwareUpdateApplyContext applyContext;

    struct {
        bool wasInterrupted;
        FirmwareUpdateStagingAssetInfo assetInfo;
        uint32_t payloadDataOffset;
    } prevStaging;
} FirmwareUpdateContext;

/**
 * State for firmware update characteristics.
 */
typedef struct {
    /**
     * The current state of the firmware update process.
     *
     * - For state definitions, reference HAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState.
     */
    uint8_t updateState;

    /**
     * Staging not ready reason.
     *
     * - If cleared, the accessory is ready to stage a firmware update. For bit definitions, reference
     *   HAPCharacteristicValue_FirmwareUpdateReadiness_StagingNotReadyReason.
     */
    uint32_t stagingNotReadyReason;

    /**
     * Update not ready reason.
     *
     * - If cleared, the accessory is ready to apply a firmware update. For bit definitions, reference
     *   HAPCharacteristicValue_FirmwareUpdateReadiness_UpdateNotReadyReason.
     */
    uint32_t updateNotReadyReason;
} FirmwareUpdateState;

/**
 * Staging state for simulating staging to non-volatile memory.
 */
typedef struct {
    UARPVersion stagedFirmwareVersion;
    FirmwareUpdateStagingAssetInfo stagingInProgressInfo;
} FirmwareUpdateStagingState;

/**
 * Persistent payload state for simulating accessory-driven staging resume.
 */
typedef struct {
    uint32_t payloadDataOffset;
} FirmwareUpdatePayloadDataState;

//----------------------------------------------------------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------------------------------------------------------

static void FwUpAssetOffered(
        HAPAccessoryServer* server,
        const HAPAccessory* accessory,
        UARPVersion* assetVersion,
        uint32_t assetTag,
        uint32_t assetLength,
        uint16_t assetNumPayloads,
        bool* shouldAccept);
static void FwUpAssetMetadataTLV(
        HAPAccessoryServer* server,
        const HAPAccessory* accessory,
        uint32_t tlvType,
        uint32_t tlvLength,
        uint8_t* _Nullable tlvValue);
static void FwUpAssetMetadataComplete(HAPAccessoryServer* server, const HAPAccessory* accessory);
static void FwUpPayloadReady(
        HAPAccessoryServer* server,
        const HAPAccessory* accessory,
        UARPVersion* payloadVersion,
        uint32_t payloadTag,
        uint32_t payloadLength);
static void FwUpPayloadMetadataTLV(
        HAPAccessoryServer* server,
        const HAPAccessory* accessory,
        uint32_t tlvType,
        uint32_t tlvLength,
        uint8_t* _Nullable tlvValue);
static void FwUpPayloadMetadataComplete(HAPAccessoryServer* server, const HAPAccessory* accessory);
static void FwUpPayloadData(
        HAPAccessoryServer* server,
        const HAPAccessory* accessory,
        uint32_t payloadTag,
        uint32_t offset,
        uint8_t* buffer,
        uint32_t bufferLength);
static void FwUpPayloadDataComplete(HAPAccessoryServer* server, const HAPAccessory* accessory);
static void FwUpAssetStateChange(HAPAccessoryServer* server, const HAPAccessory* accessory, UARPAssetStateChange state);
static void FwUpApplyStagedAssets(HAPAccessoryServer* server, const HAPAccessory* accessory, bool* requestRefused);
static void FwUpRetrieveLastError(
        HAPAccessoryServer* server,
        const HAPAccessory* accessory,
        UARPLastErrorAction* lastErrorAction);

//----------------------------------------------------------------------------------------------------------------------
// Global variables
//----------------------------------------------------------------------------------------------------------------------

static FirmwareUpdateContext firmwareUpdate;
static const UARPAccessoryFirmwareAssetCallbacks uarpFirmwareAssetCallbacks = {
    .assetOffered = FwUpAssetOffered,
    .assetMetadataTLV = FwUpAssetMetadataTLV,
    .assetMetadataComplete = FwUpAssetMetadataComplete,
    .payloadReady = FwUpPayloadReady,
    .payloadMetadataTLV = FwUpPayloadMetadataTLV,
    .payloadMetadataComplete = FwUpPayloadMetadataComplete,
    .payloadData = FwUpPayloadData,
    .payloadDataComplete = FwUpPayloadDataComplete,
    .assetStateChange = FwUpAssetStateChange,
    .applyStagedAssets = FwUpApplyStagedAssets,
    .retrieveLastError = FwUpRetrieveLastError
};

//----------------------------------------------------------------------------------------------------------------------
// Static functions
// These helpers can be used/leveraged as needed based on the accessory implementation.
//----------------------------------------------------------------------------------------------------------------------

/**
 * Retrieve a firmware version string from a UARP version.
 *
 * @param      version                The UARP version.
 * @param[out] firmwareString         The firmware version string corresponding to the UARP version.
 * @param      maxLength              The length of the firmwareString buffer, including the `\0` terminator.
 *
 * @return kHAPError_None            If successful.
 * @return kHAPError_InvalidData     If the conversion was not completed successfully.
 */
HAP_RESULT_USE_CHECK
static HAPError UARPVersionToAccessoryFirmwareVersionString(
        const UARPVersion* version,
        char* firmwareString,
        size_t maxLength) {
    HAPPrecondition(version);
    HAPPrecondition(firmwareString);
    HAPPrecondition(maxLength > 0);
    HAPError err = kHAPError_Unknown;

    if (version->release > 0) {
        err = HAPStringWithFormat(
                firmwareString,
                maxLength,
                "%lu.%lu.%lu",
                (unsigned long) version->major,
                (unsigned long) version->minor,
                (unsigned long) version->release);
    } else if (version->minor > 0) {
        err = HAPStringWithFormat(
                firmwareString, maxLength, "%lu.%lu", (unsigned long) version->major, (unsigned long) version->minor);
    } else if (version->major > 0) {
        err = HAPStringWithFormat(firmwareString, maxLength, "%lu", (unsigned long) version->major);
    }

    if (err) {
        firmwareString[0] = '\0';
        return kHAPError_InvalidData;
    }

    return kHAPError_None;
}

/**
 * Retrieve the payload data state from the Key Value Store.
 *
 * @param      payloadDataState      The payload data state.
 * @param      found                 True if state is present, false if missing.
 *
 * @return kHAPError_None            If successful.
 * @return kHAPError_InvalidData     If there was invalid data for the payload data state.
 * @return kHAPError_Unknown         If there was an issue reading from the KVS.
 */
HAP_RESULT_USE_CHECK
static HAPError FwUpGetKeyValueStorePayloadDataState(FirmwareUpdatePayloadDataState* payloadDataState, bool* found) {
    HAPAssert(payloadDataState);
    HAPAssert(found);

    size_t numBytes;

    HAPError err = HAPPlatformKeyValueStoreGet(
            firmwareUpdate.keyValueStore,
            kAppKeyValueStoreDomain_FirmwareUpdate,
            kAppKeyValueStoreKey_FirmwareUpdate_PayloadDataState,
            payloadDataState,
            sizeof(FirmwareUpdatePayloadDataState),
            &numBytes,
            found);
    if (err) {
        HAPLogError(&kHAPLog_Default, "%s: Could not get the payload data state in KVS.", __func__);
        HAPAssert(err == kHAPError_Unknown);
        HAPFatalError();
    }

    if (*found && numBytes != sizeof(FirmwareUpdatePayloadDataState)) {
        HAPLogError(
                &kHAPLog_Default,
                "%s: Found payload data state, but size mismatch. Expected size: %lu, Found size: %zu.",
                __func__,
                (unsigned long) sizeof(FirmwareUpdatePayloadDataState),
                numBytes);
        return kHAPError_InvalidData;
    }

    return kHAPError_None;
}

/**
 * Set the payload data state in the Key Value Store.
 *
 * @param      payloadDataState       The payload data state.
 *
 * @return kHAPError_None            If successful.
 * @return kHAPError_Unknown         If there was an issue writing to the KVS.
 */
HAP_RESULT_USE_CHECK
static HAPError FwUpSetKeyValueStorePayloadDataState(FirmwareUpdatePayloadDataState payloadDataState) {

    HAPError err = HAPPlatformKeyValueStoreSet(
            firmwareUpdate.keyValueStore,
            kAppKeyValueStoreDomain_FirmwareUpdate,
            kAppKeyValueStoreKey_FirmwareUpdate_PayloadDataState,
            &payloadDataState,
            sizeof payloadDataState);
    if (err) {
        HAPLogError(
                &kHAPLog_Default,
                "%s: Could not write offset %lu to KVS.",
                __func__,
                (unsigned long) payloadDataState.payloadDataOffset);
        HAPAssert(err == kHAPError_Unknown);
        HAPFatalError();
    }
    return kHAPError_None;
}

/**
 * Clear the payload data state in the Key Value Store.
 *
 * @return kHAPError_None            If successful.
 * @return kHAPError_Unknown         If there was an issue writing to the KVS.
 */
HAP_RESULT_USE_CHECK
static HAPError FwUpClearKeyValueStorePayloadDataState(void) {
    FirmwareUpdatePayloadDataState payloadDataState = { 0 };
    HAPError err = HAPPlatformKeyValueStoreSet(
            firmwareUpdate.keyValueStore,
            kAppKeyValueStoreDomain_FirmwareUpdate,
            kAppKeyValueStoreKey_FirmwareUpdate_PayloadDataState,
            &payloadDataState,
            sizeof payloadDataState);
    if (err) {
        HAPLogError(&kHAPLog_Default, "%s: Could not clear the payload data state in KVS.", __func__);
        HAPAssert(err == kHAPError_Unknown);
        HAPFatalError();
    }
    return kHAPError_None;
}

/**
 * Retrieve the firmware staging state from the Key Value Store.
 *
 * @param      state                 The firmware staging state.
 * @param      found                 True if state is present, false if missing.
 *
 * @return kHAPError_None            If successful.
 * @return kHAPError_InvalidData     If there was invalid data for the firmware staging state.
 * @return kHAPError_Unknown         If there was an issue reading from the KVS.
 */
HAP_RESULT_USE_CHECK
static HAPError FwUpGetKeyValueStoreFirmwareStagingState(FirmwareUpdateStagingState* state, bool* found) {
    HAPPrecondition(state);
    HAPPrecondition(found);

    size_t numBytes;

    // Read up the KVS current state, so we don't accidentally overwrite other data in the state.
    HAPError err = HAPPlatformKeyValueStoreGet(
            firmwareUpdate.keyValueStore,
            kAppKeyValueStoreDomain_FirmwareUpdate,
            kAppKeyValueStoreKey_FirmwareUpdate_FirmwareStagingState,
            state,
            sizeof(FirmwareUpdateStagingState),
            &numBytes,
            found);
    if (err) {
        HAPLogError(&kHAPLog_Default, "%s: Could not get the firmware update state from KVS.", __func__);
        HAPAssert(err == kHAPError_Unknown);
        HAPFatalError();
    }
    if (*found && numBytes != sizeof(FirmwareUpdateStagingState)) {
        HAPLogError(
                &kHAPLog_Default,
                "%s: Found firmware update state, but size mismatch. Expected size: %lu, Found size: %zu.",
                __func__,
                (unsigned long) sizeof(FirmwareUpdateStagingState),
                numBytes);

        return kHAPError_InvalidData;
    }

    return kHAPError_None;
}

/**
 * Set the firnware staging state in the Key Value Store.
 *
 * @param      state                 The firmware staging state.
 *
 * @return kHAPError_None            If successful.
 * @return kHAPError_Unknown         If there was an issue writing to the KVS.
 */
HAP_RESULT_USE_CHECK
static HAPError FwUpSetKeyValueStoreFirmwareStagingState(FirmwareUpdateStagingState state) {

    HAPError err = HAPPlatformKeyValueStoreSet(
            firmwareUpdate.keyValueStore,
            kAppKeyValueStoreDomain_FirmwareUpdate,
            kAppKeyValueStoreKey_FirmwareUpdate_FirmwareStagingState,
            &state,
            sizeof state);
    if (err) {
        HAPLogError(&kHAPLog_Default, "%s: Could not set the firmware staging state in KVS.", __func__);
        HAPAssert(err == kHAPError_Unknown);
        HAPFatalError();
    }

    return kHAPError_None;
}

/**
 * Retrieve the firmware update state from the Key Value Store.
 *
 * @param      state                 The firmware update state.
 * @param      found                 True if state is present, false if missing.
 *
 * @return kHAPError_None            If successful.
 * @return kHAPError_InvalidData     If there was invalid data for the firmware update state.
 * @return kHAPError_Unknown         If there was an issue reading from the KVS.
 */
HAP_RESULT_USE_CHECK
static HAPError FwUpGetKeyValueStoreFirmwareUpdateState(FirmwareUpdateState* state, bool* found) {
    HAPPrecondition(state);
    HAPPrecondition(found);

    size_t numBytes;

    // Read up the KVS current state, so we don't accidentally overwrite other data in the state.
    HAPError err = HAPPlatformKeyValueStoreGet(
            firmwareUpdate.keyValueStore,
            kAppKeyValueStoreDomain_FirmwareUpdate,
            kAppKeyValueStoreKey_FirmwareUpdate_State,
            state,
            sizeof(FirmwareUpdateState),
            &numBytes,
            found);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPFatalError();
    }
    if (*found && numBytes != sizeof(FirmwareUpdateState)) {
        HAPLogError(&kHAPLog_Default, "Unexpected firmware update state.");
        return kHAPError_InvalidData;
    }

    return kHAPError_None;
}

/**
 * Set the firmware update state in the Key Value Store.
 * This only includes the updateState, updateNotReadyReason, and stagingNotReadyReason.
 *
 * @param      hapProfile                 The payload data state.
 *
 * @return kHAPError_None            If successful.
 * @return kHAPError_Unknown         If there was an issue writing to the KVS.
 */
HAP_RESULT_USE_CHECK
static HAPError FwUpSetKeyValueStoreFirmwareUpdateState(HAPAccessoryFirmwareUpdateState hapProfile) {

    FirmwareUpdateState fwUpdateState;
    fwUpdateState.stagingNotReadyReason = hapProfile.stagingNotReadyReason;
    fwUpdateState.updateNotReadyReason = hapProfile.updateNotReadyReason;
    fwUpdateState.updateState = hapProfile.updateState;

    HAPError err = HAPPlatformKeyValueStoreSet(
            firmwareUpdate.keyValueStore,
            kAppKeyValueStoreDomain_FirmwareUpdate,
            kAppKeyValueStoreKey_FirmwareUpdate_State,
            &fwUpdateState,
            sizeof fwUpdateState);
    if (err) {
        HAPLogError(&kHAPLog_Default, "%s: Could not write firmware update state to KVS.", __func__);
        HAPAssert(err == kHAPError_Unknown);
        HAPFatalError();
    }
    return kHAPError_None;
}
/**
 * Retreive the payload index and the payload data offset for the previously interrupted staging.
 *
 * @param[out] payloadIndex           The payload index of the previous staging.
 * @param[out] payloadDataOffset      The payload data offset of the previous staging.
 *
 * @return kHAPError_None            If successful.
 * @return kHAPError_Unknown         If peristent staging is not enabled.
 * @return kHAPError_InvalidState    If previous staging was not interrupted.
 * @return kHAPError_InvalidData     If the data for the previous staging is invalid.
 */
HAP_RESULT_USE_CHECK
static HAPError FwUpGetInterruptedPayloadDataInfo(uint32_t* payloadIndex, uint32_t* payloadDataOffset) {
    HAPPrecondition(payloadIndex);
    HAPPrecondition(payloadDataOffset);

    // No data to return
    if (!firmwareUpdate.options.persistStaging) {
        return kHAPError_Unknown;
    }

    // No previous staging to restore.
    if (!firmwareUpdate.prevStaging.wasInterrupted) {
        return kHAPError_InvalidState;
    }

    HAPError err;
    FirmwareUpdateStagingAssetInfo* prevInfo = &(firmwareUpdate.prevStaging.assetInfo);
    FirmwareUpdateStagingAssetInfo* currentInfo = &(firmwareUpdate.stagingAssetInfo);

    // Compare the interrupted staging information with the current staging information, to determine
    // if we can use the previously saved payload data information.
    if (!UARPAreVersionsEqual(&(prevInfo->assetVersion), &(currentInfo->assetVersion)) ||
        (prevInfo->assetLength != currentInfo->assetLength) ||
        (prevInfo->assetNumPayload != currentInfo->assetNumPayload) || (prevInfo->assetTag != currentInfo->assetTag)) {
        // The saved payload data information for the interrupted firmware asset is no longer valid.
        return kHAPError_InvalidData;
    }

    HAPLogDebug(
            &kHAPLog_Default,
            "%s: This asset was previously interrupted during staging. Looking up payload data offset.",
            __func__);

    FirmwareUpdatePayloadDataState payloadDataState = { 0 };
    bool found = false;
    err = FwUpGetKeyValueStorePayloadDataState(&payloadDataState, &found);
    if (err) {
        return kHAPError_Unknown;
    }

    if (!found) {
        HAPLogError(
                &kHAPLog_Default,
                "%s: This asset was previously interrupted during staging, but could not find saved payload data "
                "offset.",
                __func__);
        return kHAPError_InvalidData;
    }

#if HAP_FEATURE_ENABLED(HAP_FEATURE_FILE_STORAGE)
    int fileSize;
    // Retrieve the size of the file we are using to store the asset payload data.
    err = HAPPlatformFileManagerGetFileSize(kFirmwareAssetFile, &fileSize);
    HAPAssert(err == kHAPError_None);
    if (err) {
        // The saved payload data information for the interrupted firmware asset is no longer needed.
        return kHAPError_Unknown;
    }

    // Check if the payload data offset matches the file size.
    if (fileSize != (off_t) payloadDataState.payloadDataOffset) {
        HAPLogDebug(
                &kHAPLog_Default,
                "%s: File size %lld does not match payload data starting offset %lu. Unable to use previously "
                "collected data.",
                __func__,
                (long long int) fileSize,
                (unsigned long) payloadDataState.payloadDataOffset);

        // The saved payload data information for the interrupted firmware asset is not valid.
        return kHAPError_InvalidData;
    }
#endif

    // Save the payload data offset. This will need to be used in FwUpAssetMetadataComplete()
    // to set the payload data offset.
    *payloadIndex = prevInfo->assetSelectedPayloadIndex;
    *payloadDataOffset = payloadDataState.payloadDataOffset;
    return kHAPError_None;
}

/**
 * Clears out volatile and non-volatile memory pertaining to the previously interrupted asset staging. Also
 * removes the old Firmware Asset File (used for debug purposes).
 */
static void FwUpRemoveInterruptedPayloadDataInfo() {
    if (firmwareUpdate.options.persistStaging) {
        HAPError err;
        bool found;

        FirmwareUpdateStagingState stagingState;

        // Clear out the accessory information pertaining to previously interrupted staging.
        firmwareUpdate.prevStaging.wasInterrupted = false;
        HAPRawBufferZero(&(firmwareUpdate.prevStaging.assetInfo), sizeof firmwareUpdate.prevStaging.assetInfo);
        firmwareUpdate.prevStaging.payloadDataOffset = 0;

#if HAP_FEATURE_ENABLED(HAP_FEATURE_FILE_STORAGE)
        // Remove the old Firmware Asset File. This file is used for debug purposes.
        err = HAPPlatformFileManagerRemoveFile(kFirmwareAssetFile);
        if (err != kHAPError_None) {
            // If it is unable to be removed, that means the new Asset's payload information will be appended to the
            // existing file. This file will no longer be valid for comparison to the original payload data provided as
            // input to the SuperBinary Tool. Returns on this error.
            HAPLogError(&kHAPLog_Default, "Unable to remove previous firmware asset file %s", kFirmwareAssetFile);
        }
#endif

        // Read up the KVS current staging state, so we don't accidentally overwrite other data in the state.
        found = false;
        HAPRawBufferZero(&(stagingState), sizeof stagingState);
        err = FwUpGetKeyValueStoreFirmwareStagingState(&stagingState, &found);
        if (err == kHAPError_InvalidData) {
            HAPLogError(
                    &kHAPLog_Default,
                    "%s: Could not find valid data for firmware staging state. Values reset to default.",
                    __func__);
            HAPRawBufferZero(&(stagingState), sizeof stagingState);
        } else if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }

        // Clear the offending info from the state.
        HAPRawBufferZero(&(stagingState.stagingInProgressInfo), sizeof stagingState.stagingInProgressInfo);

        // Update the KVS with the new state information.
        err = FwUpSetKeyValueStoreFirmwareStagingState(stagingState);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }

        // Clear the saved payload data state.
        err = FwUpClearKeyValueStorePayloadDataState();
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }
    }
}

/**
 * Raise an event for a characteristic associated with the firmware update service.
 *
 * @param      server          Accessory server.
 * @param      accessory       The accessory that provides the service.
 * @param      characteristic  The firmware update service characteristic for the event.
 */
static void FwUpRaiseCharacteristicEvent(
        HAPAccessoryServer* server,
        const HAPAccessory* accessory,
        const HAPCharacteristic* characteristic) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);
    HAPPrecondition(characteristic);

    HAPAccessoryServerRaiseEvent(server, characteristic, &firmwareUpdateService, accessory);
}

/**
 * Set the HAP profile update state.
 * Raises an event notification if the update state has changed.
 *
 * @param      server          Accessory server.
 * @param      accessory       The accessory that provides the service.
 * @param      newState        The firmware update state.
 */
static void FwUpSetHAPProfileUpdateState(
        HAPAccessoryServer* server,
        const HAPAccessory* accessory,
        HAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState newState) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);

    if (firmwareUpdate.HAPProfile.updateState != newState) {
        HAPLogDebug(
                &kHAPLog_Default,
                "%s: update state changed from %d to %d",
                __func__,
                (int) firmwareUpdate.HAPProfile.updateState,
                (int) newState);
        firmwareUpdate.HAPProfile.updateState = newState;
        // Save firmware update characteristic values to KVS.
        HAPError err = FwUpSetKeyValueStoreFirmwareUpdateState(firmwareUpdate.HAPProfile);
        if (err) {
            HAPLogError(&kHAPLog_Default, "Unable to save firmware update state.");
        }

        // Generate notification for changed characteristic.
        FwUpRaiseCharacteristicEvent(server, accessory, &firmwareUpdateStatusCharacteristic);
    }
}

/**
 * Set the HAP profile update duration.
 * Raises an event notification if the update duration has changed.
 *
 * @param      server          Accessory server.
 * @param      accessory       The accessory that provides the service.
 * @param      duration        The update duration.
 */
HAP_UNUSED
static void
        FwUpSetHAPProfileUpdateDuration(HAPAccessoryServer* server, const HAPAccessory* accessory, uint16_t duration) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);

    if (firmwareUpdate.HAPProfile.updateDuration != duration) {
        HAPLogDebug(
                &kHAPLog_Default,
                "%s: update duration changed from %d to %d",
                __func__,
                (int) firmwareUpdate.HAPProfile.updateDuration,
                (int) duration);
        firmwareUpdate.HAPProfile.updateDuration = duration;
        FwUpRaiseCharacteristicEvent(server, accessory, &firmwareUpdateStatusCharacteristic);
    }
}

/**
 * Set the HAP profile staging readiness.
 * Raises an event notification if the staging readiness has changed.
 *
 * @param      server          Accessory server.
 * @param      accessory       The accessory that provides the service.
 * @param      readiness       The staging readiness.
 */
HAP_UNUSED
static void FwUpSetHAPProfileStagingReadiness(
        HAPAccessoryServer* server,
        const HAPAccessory* accessory,
        uint32_t readiness) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);

    if (firmwareUpdate.HAPProfile.stagingNotReadyReason != readiness) {
        HAPLogDebug(
                &kHAPLog_Default,
                "%s: staging readiness changed from %d to %d",
                __func__,
                (int) firmwareUpdate.HAPProfile.stagingNotReadyReason,
                (int) readiness);
        firmwareUpdate.HAPProfile.stagingNotReadyReason = readiness;
        // Save firmware update characteristic values to KVS.
        HAPError err = FwUpSetKeyValueStoreFirmwareUpdateState(firmwareUpdate.HAPProfile);
        if (err) {
            HAPLogError(&kHAPLog_Default, "Unable to save firmware update state.");
        }
        FwUpRaiseCharacteristicEvent(server, accessory, &firmwareUpdateReadinessCharacteristic);
    }
}

/**
 * Set a single staging not ready reason.
 * Raises an event notification if the staging readiness has changed.
 *
 * @param      server          Accessory server.
 * @param      accessory       The accessory that provides the service.
 * @param      reason          The staging not ready reason to set.
 */
HAP_UNUSED
static void FwUpSetHAPProfileStagingNotReadyReason(
        HAPAccessoryServer* server,
        const HAPAccessory* accessory,
        HAPCharacteristicValue_FirmwareUpdateReadiness_StagingNotReadyReason reason) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);

    uint32_t prevReadiness = firmwareUpdate.HAPProfile.stagingNotReadyReason;
    firmwareUpdate.HAPProfile.stagingNotReadyReason |= reason;

    if (firmwareUpdate.HAPProfile.stagingNotReadyReason != prevReadiness) {
        HAPLogDebug(
                &kHAPLog_Default,
                "%s: staging readiness changed from %d to %d",
                __func__,
                (int) prevReadiness,
                (int) firmwareUpdate.HAPProfile.stagingNotReadyReason);
        // Save firmware update characteristic values to KVS.
        HAPError err = FwUpSetKeyValueStoreFirmwareUpdateState(firmwareUpdate.HAPProfile);
        if (err) {
            HAPLogError(&kHAPLog_Default, "Unable to save firmware update state.");
        }
        FwUpRaiseCharacteristicEvent(server, accessory, &firmwareUpdateReadinessCharacteristic);
    }
}

/**
 * Clear a single staging not ready reason.
 * Raises an event notification if the staging readiness has changed.
 *
 * @param      server          Accessory server.
 * @param      accessory       The accessory that provides the service.
 * @param      reason          The staging not ready reason to clear.
 */
HAP_UNUSED
static void FwUpClearHAPProfileStagingNotReadyReason(
        HAPAccessoryServer* server,
        const HAPAccessory* accessory,
        HAPCharacteristicValue_FirmwareUpdateReadiness_StagingNotReadyReason reason) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);

    uint32_t prevReadiness = firmwareUpdate.HAPProfile.stagingNotReadyReason;
    firmwareUpdate.HAPProfile.stagingNotReadyReason &= ~reason;

    if (firmwareUpdate.HAPProfile.stagingNotReadyReason != prevReadiness) {
        HAPLogDebug(
                &kHAPLog_Default,
                "%s: staging readiness changed from %d to %d",
                __func__,
                (int) prevReadiness,
                (int) firmwareUpdate.HAPProfile.stagingNotReadyReason);
        // Save firmware update characteristic values to KVS.
        HAPError err = FwUpSetKeyValueStoreFirmwareUpdateState(firmwareUpdate.HAPProfile);
        if (err) {
            HAPLogError(&kHAPLog_Default, "Unable to save firmware update state.");
        }
        FwUpRaiseCharacteristicEvent(server, accessory, &firmwareUpdateReadinessCharacteristic);
    }
}

/**
 * Set the HAP profile update readiness.
 * Raises an event notification if the update readiness has changed.
 *
 * @param      server          Accessory server.
 * @param      accessory       The accessory that provides the service.
 * @param      readiness       The update readiness.
 */
HAP_UNUSED
static void FwUpSetHAPProfileUpdateReadiness(
        HAPAccessoryServer* server,
        const HAPAccessory* accessory,
        uint32_t readiness) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);

    if (firmwareUpdate.HAPProfile.updateNotReadyReason != readiness) {
        HAPLogDebug(
                &kHAPLog_Default,
                "%s: update readiness changed from %d to %d",
                __func__,
                (int) firmwareUpdate.HAPProfile.updateNotReadyReason,
                (int) readiness);
        firmwareUpdate.HAPProfile.updateNotReadyReason = readiness;
        // Save firmware update characteristic values to KVS.
        HAPError err = FwUpSetKeyValueStoreFirmwareUpdateState(firmwareUpdate.HAPProfile);
        if (err) {
            HAPLogError(&kHAPLog_Default, "Unable to save firmware update state.");
        }
        FwUpRaiseCharacteristicEvent(server, accessory, &firmwareUpdateReadinessCharacteristic);
    }
}

/**
 * Set a single update not ready reason.
 * Raises an event notification if the update readiness has changed.
 *
 * @param      server          Accessory server.
 * @param      accessory       The accessory that provides the service.
 * @param      reason          The update not ready reason to set.
 */
static void FwUpSetHAPProfileUpdateNotReadyReason(
        HAPAccessoryServer* server,
        const HAPAccessory* accessory,
        HAPCharacteristicValue_FirmwareUpdateReadiness_UpdateNotReadyReason reason) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);

    uint32_t prevReadiness = firmwareUpdate.HAPProfile.updateNotReadyReason;
    firmwareUpdate.HAPProfile.updateNotReadyReason |= reason;

    if (firmwareUpdate.HAPProfile.updateNotReadyReason != prevReadiness) {
        HAPLogDebug(
                &kHAPLog_Default,
                "%s: update readiness changed from %d to %d",
                __func__,
                (int) prevReadiness,
                (int) firmwareUpdate.HAPProfile.updateNotReadyReason);
        // Save firmware update characteristic values to KVS.
        HAPError err = FwUpSetKeyValueStoreFirmwareUpdateState(firmwareUpdate.HAPProfile);
        if (err) {
            HAPLogError(&kHAPLog_Default, "Unable to save firmware update state.");
        }
        FwUpRaiseCharacteristicEvent(server, accessory, &firmwareUpdateReadinessCharacteristic);
    }
}

/**
 * Clear a single update not ready reason.
 * Raises an event notification if the update readiness has changed.
 *
 * @param      server          Accessory server.
 * @param      accessory       The accessory that provides the service.
 * @param      reason          The update not ready reason to clear.
 */
static void FwUpClearHAPProfileUpdateNotReadyReason(
        HAPAccessoryServer* server,
        const HAPAccessory* accessory,
        HAPCharacteristicValue_FirmwareUpdateReadiness_UpdateNotReadyReason reason) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);

    uint32_t prevReadiness = firmwareUpdate.HAPProfile.updateNotReadyReason;
    firmwareUpdate.HAPProfile.updateNotReadyReason &= ~reason;

    if (firmwareUpdate.HAPProfile.updateNotReadyReason != prevReadiness) {
        HAPLogDebug(
                &kHAPLog_Default,
                "%s: update readiness changed from %d to %d",
                __func__,
                (int) prevReadiness,
                (int) firmwareUpdate.HAPProfile.updateNotReadyReason);
        // Save firmware update characteristic values to KVS.
        HAPError err = FwUpSetKeyValueStoreFirmwareUpdateState(firmwareUpdate.HAPProfile);
        if (err) {
            HAPLogError(&kHAPLog_Default, "Unable to save firmware update state.");
        }
        FwUpRaiseCharacteristicEvent(server, accessory, &firmwareUpdateReadinessCharacteristic);
    }
}

/**
 * Set the HAP profile staged firmware version.
 * Raises an event notification if the staged firmware version has changed (added or cleared).
 *
 * @param      server          Accessory server.
 * @param      accessory       The accessory that provides the service.
 * @param      version         The staged firmware version.
 *                             NULL if there is no staged version.
 */
static void FwUpSetHAPProfileStagedFirmwareVersion(
        HAPAccessoryServer* server,
        const HAPAccessory* accessory,
        const char* _Nullable version) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);

    if (version) {
        HAPLogDebug(&kHAPLog_Default, "%s: staged firmware version set to %s", __func__, version);
    } else {
        HAPLogDebug(&kHAPLog_Default, "%s: staged firmware version cleared", __func__);
    }

    firmwareUpdate.HAPProfile.stagedFirmwareVersion = version;
    FwUpRaiseCharacteristicEvent(server, accessory, &firmwareUpdateStatusCharacteristic);
}

/**
 * Adjust the HAP profile for clearing of any partially or fully staged asset and remove any persistent state
 * related to the staged version, if applicable.
 *
 * @param      server          Accessory server.
 * @param      accessory       The accessory that provides the service.
 * @param      shouldIdle      The HAP profile should transition to the idle state.
 */
static void FwUpClearStaging(HAPAccessoryServer* server, const HAPAccessory* accessory, bool shouldIdle) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);

    // Clear local state associated with the asset.
    HAPRawBufferZero(
            &(firmwareUpdate.stagingAssetInfo.assetVersion), sizeof firmwareUpdate.stagingAssetInfo.assetVersion);

    // Clear staged firmware version from the HAP profile, if needed.
    if (HAPStringGetNumBytes(firmwareUpdate.stagedFirmwareVersion) > 0) {
        HAPRawBufferZero(&firmwareUpdate.stagedFirmwareVersion, sizeof firmwareUpdate.stagedFirmwareVersion);
        FwUpSetHAPProfileStagedFirmwareVersion(server, accessory, NULL);
    }

    // Ensure the HAP profile update not ready reasons indicate there is no staged firmware available.
    FwUpSetHAPProfileUpdateNotReadyReason(
            server, accessory, kHAPCharacteristicValue_FirmwareUpdateReadiness_UpdateNotReady_StagedUnavailable);

    if (shouldIdle) {
        // Return the HAP profile update state to idle.
        FwUpSetHAPProfileUpdateState(
                server, accessory, kHAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState_Idle);
    }

    // Clear the staged version from the KVS.
    if (firmwareUpdate.options.persistStaging) {
        // Retrieve the current persistent state.
        FirmwareUpdateStagingState stagingState;
        bool found = false;

        // Read up the KVS current staging state, so we don't accidentally overwrite other data in the state.
        HAPRawBufferZero(&(stagingState), sizeof stagingState);
        HAPError err = FwUpGetKeyValueStoreFirmwareStagingState(&stagingState, &found);
        if (err == kHAPError_InvalidData) {
            HAPLogError(
                    &kHAPLog_Default,
                    "%s: Could not find valid data for firmware staging state. Values reset to default.",
                    __func__);
            HAPRawBufferZero(&(stagingState), sizeof stagingState);
        } else if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }

        // Clear the version member.
        HAPRawBufferZero(&(stagingState.stagedFirmwareVersion), sizeof stagingState.stagedFirmwareVersion);

        // Update the KVS with the new state information.
        err = FwUpSetKeyValueStoreFirmwareStagingState(stagingState);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }
    }
}

/**
 * Complete the simulated apply routine.
 * Updates the firmware revision characteristic to that of the "applied" firmware version.
 *
 * @param      timer           Timer which triggered this handler.
 * @param      context         Context associated with the timer handler.
 */
static void FwUpCompleteApplyStagedAssets(HAPPlatformTimerRef timer HAP_UNUSED, void* _Nullable context) {
    HAPPrecondition(context);

    FirmwareUpdateApplyContext* applyContext = (FirmwareUpdateApplyContext*) context;
    HAPAccessoryServer* server = applyContext->server;
    HAPAccessory* accessory = applyContext->accessory;
    size_t stagedVersionLen = HAPStringGetNumBytes(firmwareUpdate.stagedFirmwareVersion);
    HAPAssert(stagedVersionLen > 0);

    // "Apply" has completed, update the firmware version.
    HAPRawBufferZero(&firmwareUpdate.updatedFirmwareVersion, sizeof firmwareUpdate.updatedFirmwareVersion);
    HAPRawBufferCopyBytes(
            firmwareUpdate.updatedFirmwareVersion, firmwareUpdate.stagedFirmwareVersion, stagedVersionLen);
    ((HAPAccessory*) accessory)->firmwareVersion = firmwareUpdate.updatedFirmwareVersion;
    HAPLogDebug(&kHAPLog_Default, "%s: firmware version set to %s", __func__, accessory->firmwareVersion);
    HAPAccessoryServerRaiseEvent(
            server, &accessoryInformationFirmwareRevisionCharacteristic, &accessoryInformationService, accessory);

    // Reset state associated with the applied asset.
    FwUpClearStaging(server, accessory, true);
}

static void FwUpHandleApplyStagedAssetsOnRunLoop(void* _Nullable context HAP_UNUSED, size_t contextSize HAP_UNUSED) {

    // VENDOR-TODO: This sample implementation updates the firmware revision after a simulated apply window.
    // For accessories, it is expected a reboot will be required to apply/boot into the new firmware.
    // HAPAccessoryServerStop() can be called to gracefully shut down the accessory server.

    HAPLogDebug(
            &kHAPLog_Default,
            "%s: Simulating update with %d sec delay before updating firmware version",
            __func__,
            (int) firmwareUpdate.applyContext.delay);

    HAPError err = HAPPlatformTimerRegister(
            &firmwareUpdate.applyContext.timer,
            HAPPlatformClockGetCurrent() + (HAPTime)(firmwareUpdate.applyContext.delay * HAPSecond),
            FwUpCompleteApplyStagedAssets,
            &firmwareUpdate.applyContext);
    HAPAssert(err == kHAPError_None);
}

//----------------------------------------------------------------------------------------------------------------------
// HAP accessory state callback
//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError FirmwareUpdateGetAccessoryState(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPAccessory* accessory HAP_UNUSED,
        HAPAccessoryFirmwareUpdateState* accessoryState,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);
    HAPPrecondition(accessoryState);

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    HAPRawBufferCopyBytes(accessoryState, &firmwareUpdate.HAPProfile, sizeof firmwareUpdate.HAPProfile);
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------
// UARP FW asset accessory callbacks
// These callbacks are triggered by UARP events and must be handled by the accessory. The below implementations may be
// used as a starting point.
//----------------------------------------------------------------------------------------------------------------------

static void FwUpAssetOffered(
        HAPAccessoryServer* server,
        const HAPAccessory* accessory,
        UARPVersion* assetVersion,
        uint32_t assetTag,
        uint32_t assetLength,
        uint16_t assetNumPayloads,
        bool* shouldAccept) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);
    HAPPrecondition(assetVersion);
    HAPPrecondition(shouldAccept);

    HAPError err;
    HAPLogInfo(
            &kHAPLog_Default,
            "%s: Asset offered, version = %d.%d.%d.%d",
            __func__,
            (int) assetVersion->major,
            (int) assetVersion->minor,
            (int) assetVersion->release,
            (int) assetVersion->build);

    // If an asset offer is received with a staging not ready reason set, deny the offer.
    // When the staging not ready reason is cleared, the controller will be notified and the asset may be re-offered.
    if (firmwareUpdate.HAPProfile.stagingNotReadyReason != 0) {
        *shouldAccept = false;
        return;
    }

    // Since the "apply" routine sets up a timer callback for updating the firmware version, refuse any offers during
    // the simulated update window.
    if (firmwareUpdate.HAPProfile.updateState ==
        kHAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState_UpdateInProgress) {
        *shouldAccept = false;
        return;
    }

    // An asset version of zero corresponds to no partially or fully staged asset.
    if (UARPIsVersionZero(&(firmwareUpdate.stagingAssetInfo.assetVersion)) == false) {
        // If we are already staging an asset, or have finished staging but haven't applied,
        // abandon the active asset.
        err = UARPRequestFirmwareAssetStagingStateChange(kUARPAssetStagingStateChangeRequest_Abandon);
        HAPAssert(err == kHAPError_None);
        if (err != kHAPError_None) {
            *shouldAccept = false;
            return;
        }
    }

    if (HAPStringGetNumBytes(firmwareUpdate.stagedFirmwareVersion) > 0) {
        // Reset state associated with the staged asset.
        FwUpClearStaging(server, accessory, false);
    }

    // Store the accepted version.
    HAPRawBufferCopyBytes(
            &(firmwareUpdate.stagingAssetInfo.assetVersion),
            assetVersion,
            sizeof firmwareUpdate.stagingAssetInfo.assetVersion);

    // If staging is persistent, the staging asset information will be needed later to determine
    // whether an asset staging was previously interrupted.
    if (firmwareUpdate.options.persistStaging) {
        firmwareUpdate.stagingAssetInfo.assetLength = assetLength;
        firmwareUpdate.stagingAssetInfo.assetNumPayload = assetNumPayloads;
        firmwareUpdate.stagingAssetInfo.assetTag = assetTag;
        // This information is not known yet.
        firmwareUpdate.stagingAssetInfo.assetSelectedPayloadIndex = 0;
    }

    // Indicate staging is in-progress.
    FwUpSetHAPProfileUpdateState(
            server, accessory, kHAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState_StagingInProgress);
    *shouldAccept = true;
}

static void FwUpAssetMetadataTLV(
        HAPAccessoryServer* server,
        const HAPAccessory* accessory,
        uint32_t tlvType,
        uint32_t tlvLength,
        uint8_t* _Nullable tlvValue HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);

    // VENDOR-TODO: Asset metadata TLVs are vendor-specific. Processing for any asset TLVs should be added here.
    HAPLogInfo(
            &kHAPLog_Default,
            "%s: Received SuperBinary metadata with type %u, length %u",
            __func__,
            (unsigned int) tlvType,
            (unsigned int) tlvLength);
}

static void FwUpAssetMetadataComplete(HAPAccessoryServer* server, const HAPAccessory* accessory) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    HAPError err;

    // Initialize to our (only) payload index.
    // VENDOR-TODO: This can be a hard-coded value or derived from asset metadata.
    uint32_t payloadIndex = kSampleSuperBinaryPayloadIndex;

    // Perform persistent state book-keeping for accessory-driven resume.
    if (firmwareUpdate.options.persistStaging) {
        uint32_t prevPayloadIndex;
        uint32_t prevPayloadDataOffset;

        if (firmwareUpdate.prevStaging.wasInterrupted) {
            err = FwUpGetInterruptedPayloadDataInfo(&prevPayloadIndex, &prevPayloadDataOffset);
            if (err) {
                HAPAssert(err == kHAPError_InvalidData);
                // The saved payload data information for the interrupted firmware asset is no longer valid.
                FwUpRemoveInterruptedPayloadDataInfo();
            } else {
                payloadIndex = prevPayloadIndex;
                firmwareUpdate.prevStaging.payloadDataOffset = prevPayloadDataOffset;
            }
        } else {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_FILE_STORAGE)
            // Since we are not resuming an interrupted asset staging, remove the old Firmware Asset File. This file is
            // used for debug purposes.
            err = HAPPlatformFileManagerRemoveFile(kFirmwareAssetFile);
            HAPAssert(err == kHAPError_None);
            if (err) {
                // If it is unable to be removed, that means the new Asset's payload information will be appended to the
                // existing file. This file will no longer be valid for comparison to the original payload data provided
                // as input to the SuperBinary Tool. Returns on this error.
                HAPLogError(&kHAPLog_Default, "Unable to remove previous firmware asset file %s", kFirmwareAssetFile);
                return;
            }
#endif
        }
        // In case the staging is interrupted (on the accessory side), we need to remember this
        // this payload index, so we can reference it later. It is saved to NV in FwUpPayloadData()
        firmwareUpdate.stagingAssetInfo.assetSelectedPayloadIndex = payloadIndex;
    }

    // Request the desired SuperBinary payload.
    err = UARPRequestFirmwareAssetPayloadByIndex(payloadIndex);
    HAPAssert(err == kHAPError_None);
}

static void FwUpPayloadReady(
        HAPAccessoryServer* server,
        const HAPAccessory* accessory,
        UARPVersion* payloadVersion,
        uint32_t payloadTag HAP_UNUSED,
        uint32_t payloadLength HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);
    HAPPrecondition(payloadVersion);

    // VENDOR-TODO: Perform any accessory-specific logic (e.g. verify tag/version, etc) now that the payload header
    // information is available. Payload headers may be iterated using UARPRequestPayloadByIndex(). If a new payload
    // is not requested, the currently-selected payload will be pulled upon completion of this callback.
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
}

static void FwUpPayloadMetadataTLV(
        HAPAccessoryServer* server,
        const HAPAccessory* accessory,
        uint32_t tlvType,
        uint32_t tlvLength,
        uint8_t* _Nullable tlvValue HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);

    // VENDOR-TODO: Payload metadata TLVs are vendor-specific. Processing for any payload TLVs should be added here.
    HAPLogInfo(
            &kHAPLog_Default,
            "%s: Received Payload metadata with type %u, length %u",
            __func__,
            (unsigned int) tlvType,
            (unsigned int) tlvLength);
}

static void FwUpPayloadMetadataComplete(HAPAccessoryServer* server, const HAPAccessory* accessory) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    // Accessory-driven resume.
    if (firmwareUpdate.options.persistStaging && firmwareUpdate.prevStaging.wasInterrupted) {
        HAPLogInfo(
                &kHAPLog_Default,
                "%s: Setting payload data offset to %lu.",
                __func__,
                (unsigned long) firmwareUpdate.prevStaging.payloadDataOffset);
        // Set the payload data offset to the next offset it would have processed (had it not been interrupted).
        HAPError err = UARPSetFirmwareAssetPayloadDataOffset(firmwareUpdate.prevStaging.payloadDataOffset);
        HAPAssert(err == kHAPError_None);
    }
}

static void FwUpPayloadData(
        HAPAccessoryServer* server,
        const HAPAccessory* accessory,
        uint32_t payloadTag,
        uint32_t offset,
        uint8_t* buffer,
        uint32_t bufferLength) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);
    HAPPrecondition(buffer);
    HAPError err;

    HAPLogInfo(
            &kHAPLog_Default,
            "%s: Received %u bytes of data for payload (0x%08X) at offset %u",
            __func__,
            (unsigned int) bufferLength,
            (unsigned int) payloadTag,
            (unsigned int) offset);

    // VENDOR-TODO: Write the payload chunk to the staging area. This sample uses the PAL file manager API, which
    // can be used by platforms which support a filesystem. Alternatively, platform-specific calls can be made if
    // staging to raw flash.
    if (firmwareUpdate.options.persistStaging) {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_FILE_STORAGE)
        // Writes Asset's payload information to a file. The completed file can be used for
        // comparison to the original payload data provided as input to the SuperBinary Tool.
        err = HAPPlatformFileManagerAppendFile(kFirmwareAssetFile, buffer, bufferLength);
        HAPAssert(err == kHAPError_None);
        if (err != kHAPError_None) {
            HAPLogError(&kHAPLog_Default, "%s: Could not write to file %s", __func__, kFirmwareAssetFile);
            return;
        }
#endif
        // If this is the first chunk of data, record the firmware asset information needed for the accessory
        // to resume, in case of interrupt or power loss.
        if (offset == 0) {
            bool found = false;
            FirmwareUpdateStagingState stagingState;
            HAPRawBufferZero(&(stagingState), sizeof stagingState);

            // Read up the KVS current state, so we don't accidentally overwrite other data in the state.
            err = FwUpGetKeyValueStoreFirmwareStagingState(&stagingState, &found);
            if (err == kHAPError_InvalidData) {
                HAPLogError(
                        &kHAPLog_Default,
                        "%s: Could not find valid data for firmware staging state. Values reset to default.",
                        __func__);
                HAPRawBufferZero(&(stagingState), sizeof stagingState);
            } else if (err) {
                HAPAssert(err == kHAPError_Unknown);
                HAPFatalError();
            }

            // Copy the Firmware Update staging information into the state to be written to KVS.
            HAPRawBufferCopyBytes(
                    &(stagingState.stagingInProgressInfo),
                    &(firmwareUpdate.stagingAssetInfo),
                    sizeof(stagingState.stagingInProgressInfo));

            // Update the firmware update state in the KVS. Now it will contain the information regarding this in
            // progress firmware asset.
            err = FwUpSetKeyValueStoreFirmwareStagingState(stagingState);
            if (err) {
                HAPAssert(err == kHAPError_Unknown);
                HAPFatalError();
            }
        }

        // Update the payload data offset in the KVS.
        FirmwareUpdatePayloadDataState payloadDataState = { 0 };
        payloadDataState.payloadDataOffset = offset + bufferLength;
        err = FwUpSetKeyValueStorePayloadDataState(payloadDataState);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }
    }
}

static void FwUpPayloadDataComplete(HAPAccessoryServer* server, const HAPAccessory* accessory) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);

    HAPLogInfo(&kHAPLog_Default, "%s: Asset staging completed", __func__);

    // VENDOR-TODO: If utilizing multiple payloads, another payload may be selected using UARPRequestPayloadByIndex().
    // UARPAssetFullyStaged() should be used when the accessory has determined it has completed staging.

    // This sample routine implements processing of a single payload asset.
    // Indicate the asset has been fully processed with a staged version equivalent to the asset version.
    // For assets with multiple payloads, the staged version may differ from the asset version (e.g. subset of
    // payloads processed to stage the minimum required firmware version for the asset version).
    HAPError err = UARPFirmwareAssetFullyStaged(&(firmwareUpdate.stagingAssetInfo.assetVersion));
    HAPAssert(err == kHAPError_None);

    if (firmwareUpdate.options.persistStaging) {
        // Store the staged version to KVS.
        FirmwareUpdateStagingState stagingState;
        HAPRawBufferCopyBytes(
                &(stagingState.stagedFirmwareVersion),
                &(firmwareUpdate.stagingAssetInfo.assetVersion),
                sizeof stagingState.stagedFirmwareVersion);
        HAPRawBufferZero(&(stagingState.stagingInProgressInfo), sizeof stagingState.stagingInProgressInfo);

        // Update the KVS with the new staging state information.
        err = FwUpSetKeyValueStoreFirmwareStagingState(stagingState);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }

        // Clear the saved payload data state.
        err = FwUpClearKeyValueStorePayloadDataState();
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }
    }

    // Update the HAP profile.
    // If a minimum firmware version has been staged, FwUpSetHAPProfileStagingNotReadyReason should be called with
    // reason kHAPCharacteristicValue_FirmwareUpdateReadiness_StagingNotReady_ApplyNeeded to indicate to the HomeKit
    // controller that the staged version must be applied before being able to update to a later version.
    // VENDOR-TODO: Per UARP system expectations, it is expected the accessory will validate the staged image with
    // corresponding verification information provided as part of asset/payload metadata.
    // VENDOR-TODO: The HAP profile update duration can optionally be set here as well if the duration is dependent
    // upon the staged asset.
    FwUpSetHAPProfileUpdateState(
            server, accessory, kHAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState_StagingSucceeded);
    FwUpClearHAPProfileUpdateNotReadyReason(
            server, accessory, kHAPCharacteristicValue_FirmwareUpdateReadiness_UpdateNotReady_StagedUnavailable);
    err = UARPVersionToAccessoryFirmwareVersionString(
            (const UARPVersion*) &(firmwareUpdate.stagingAssetInfo.assetVersion),
            firmwareUpdate.stagedFirmwareVersion,
            kHAPFirmwareVersion_MaxLength + 1);
    HAPAssert(err == kHAPError_None);
    if (err == kHAPError_None) {
        FwUpSetHAPProfileStagedFirmwareVersion(server, accessory, firmwareUpdate.stagedFirmwareVersion);
    }
}

static void
        FwUpAssetStateChange(HAPAccessoryServer* server, const HAPAccessory* accessory, UARPAssetStateChange state) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);

    switch (state) {
        case kUARPAssetStateChange_StagingPaused: {
            HAPLogInfo(&kHAPLog_Default, "%s: Asset staging paused", __func__);
            if (firmwareUpdate.HAPProfile.updateState ==
                kHAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState_StagingInProgress) {
                FwUpSetHAPProfileUpdateState(
                        server,
                        accessory,
                        kHAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState_StagingPaused);
            }
            break;
        }
        case kUARPAssetStateChange_StagingResumed: {
            HAPLogInfo(&kHAPLog_Default, "%s: Asset staging resumed", __func__);
            if (firmwareUpdate.HAPProfile.updateState ==
                kHAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState_StagingPaused) {
                FwUpSetHAPProfileUpdateState(
                        server,
                        accessory,
                        kHAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState_StagingInProgress);
            }
            break;
        }
        case kUARPAssetStateChange_AssetRescinded: {
            HAPLogInfo(&kHAPLog_Default, "%s: Asset rescinded", __func__);
            // Reset state associated with any partially or fully staged asset.
            FwUpClearStaging(server, accessory, true);
            // The saved payload data information for the interrupted firmware asset is no longer needed.
            FwUpRemoveInterruptedPayloadDataInfo();
            break;
        }
        case kUARPAssetStateChange_AssetCorrupt: {
            HAPLogInfo(&kHAPLog_Default, "%s: Asset detected as corrupt", __func__);
            // Reset state associated with the partially staged asset.
            FwUpClearStaging(server, accessory, true);
            // The saved payload data information for the interrupted firmware asset is now invalid.
            FwUpRemoveInterruptedPayloadDataInfo();
            break;
        }
        default:
            HAPFatalError();
    }
}

static void FwUpApplyStagedAssets(HAPAccessoryServer* server, const HAPAccessory* accessory, bool* requestRefused) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);
    HAPPrecondition(requestRefused);

    HAPLogInfo(
            &kHAPLog_Default, "%s: Apply staged asset, version = %s", __func__, firmwareUpdate.stagedFirmwareVersion);

    size_t stagedVersionLen = HAPStringGetNumBytes(firmwareUpdate.stagedFirmwareVersion);
    HAPAssert(stagedVersionLen > 0);

    // VENDOR-TODO: Accept/reject apply request based upon any accessory-specific criteria.
    // In accordance with Section 2.1.2 (Applying Update) of the HAP spec, the accessory should refuse an apply
    // request if doing so could knowingly lead to negative impact to the user. The status should be reflected
    // through the HAP profile, but any race condition due to readiness changing while the apply request is in flight
    // should be handled here.

    if (stagedVersionLen > 0) {
        *requestRefused = false;
    } else {
        *requestRefused = true;
        return;
    }

    // VENDOR-TODO: When issuing the apply command, HomeKit expects the accessory to be reachable on the new firmware
    // version within the HAP profile update duration. It does not require any further transitions to the HAP profile
    // update state. Accessory/platform-specific logic to boot into the new firmware should be initiated at this time.
    // Applies are simulated in the ADK sample apps by updating the firmware version after a delay. The additional
    // logic here will not be applicable to most accessories.

    // Since the "apply" routine sets up a timer callback for updating the firmware version, refuse any additional
    // apply requests during the simulated update window.
    if (firmwareUpdate.HAPProfile.updateState ==
        kHAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState_UpdateInProgress) {
        *requestRefused = true;
        return;
    }
    FwUpSetHAPProfileUpdateState(
            server, accessory, kHAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState_UpdateInProgress);

    // Schedule handling of the apply request on the run loop, allowing HDS processing of the message which triggered
    // this callback to complete. This allows shutting down the accessory server as part of the apply process.
    HAPError err = HAPPlatformRunLoopScheduleCallback(FwUpHandleApplyStagedAssetsOnRunLoop, NULL, 0);
    HAPAssert(err == kHAPError_None);
}

static void FwUpRetrieveLastError(
        HAPAccessoryServer* server,
        const HAPAccessory* accessory,
        UARPLastErrorAction* lastErrorAction) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);
    HAPPrecondition(lastErrorAction);

    HAPLogInfo(&kHAPLog_Default, "%s: Request received for last accessory error/action", __func__);

    // VENDOR-TODO: Errors associated with the UARP LastError accessory information request are vendor-specific and
    // opaque to HomeKit.
    lastErrorAction->lastAction = kUARPLastActionApplyFirmwareUpdate;
    lastErrorAction->lastError = 0;
}

//----------------------------------------------------------------------------------------------------------------------
// Firmware update initialization
// This must be called during application initialization to prepare the firmware update module, which at a minimum
// requires initializing UARP and the firmware update HAP profile. Any other accessory-specific initialization related
// to firmware updates can be performed here as well.
//----------------------------------------------------------------------------------------------------------------------

void FirmwareUpdateInitialize(
        HAPAccessoryServer* server,
        const HAPAccessory* accessory,
        HAPPlatformKeyValueStoreRef keyValueStore,
        FirmwareUpdateOptions* fwupOptions) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);
    HAPPrecondition(keyValueStore);
    HAPPrecondition(fwupOptions);

    HAPError err;
    bool found;
    FirmwareUpdateStagingState stagingState;
    HAPRawBufferZero(&(stagingState), sizeof stagingState);

    // Copy over the initialization options.
    HAPRawBufferZero(&firmwareUpdate, sizeof firmwareUpdate);
    HAPRawBufferCopyBytes(&firmwareUpdate.options, fwupOptions, sizeof firmwareUpdate.options);
    HAPLogDebug(&kHAPLog_Default, "%s: persistStaging = %d", __func__, (int) firmwareUpdate.options.persistStaging);

    firmwareUpdate.keyValueStore = keyValueStore;

    if (firmwareUpdate.options.persistStaging) {
        firmwareUpdate.prevStaging.wasInterrupted = false;
        HAPRawBufferZero(&(firmwareUpdate.prevStaging.assetInfo), sizeof firmwareUpdate.prevStaging.assetInfo);

        found = false;
        // Load persistent state from KVS, if available.
        HAPError err = FwUpGetKeyValueStoreFirmwareStagingState(&stagingState, &found);
        if (err == kHAPError_InvalidData) {
            HAPLogError(
                    &kHAPLog_Default,
                    "%s: Could not find valid data for firmware staging state. Values reset to default.",
                    __func__);
            HAPRawBufferZero(&(stagingState), sizeof stagingState);
        } else if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }

        if (found && !err) {
            HAPRawBufferCopyBytes(
                    &(firmwareUpdate.prevStaging.assetInfo.assetVersion),
                    &(stagingState.stagingInProgressInfo.assetVersion),
                    sizeof firmwareUpdate.prevStaging.assetInfo.assetVersion);
            // If a previous staging attempt was unexpectedly interrupted, the asset version will be non-zero
            if (!UARPIsVersionZero(&(firmwareUpdate.prevStaging.assetInfo.assetVersion))) {
                firmwareUpdate.prevStaging.wasInterrupted = true;
                firmwareUpdate.prevStaging.assetInfo.assetLength = stagingState.stagingInProgressInfo.assetLength;
                firmwareUpdate.prevStaging.assetInfo.assetNumPayload =
                        stagingState.stagingInProgressInfo.assetNumPayload;
                firmwareUpdate.prevStaging.assetInfo.assetSelectedPayloadIndex =
                        stagingState.stagingInProgressInfo.assetSelectedPayloadIndex;
                firmwareUpdate.prevStaging.assetInfo.assetTag = stagingState.stagingInProgressInfo.assetTag;
            }
        }
    } // if (firmwareUpdate.options.persistStaging)

    // Read previous values of Firmware Update Status aned Readiness Characteristics
    FirmwareUpdateState fwUpdateState;
    HAPRawBufferZero(&(fwUpdateState), sizeof fwUpdateState);
    found = false;
    err = FwUpGetKeyValueStoreFirmwareUpdateState(&fwUpdateState, &found);
    if (err == kHAPError_InvalidData) {
        HAPLogError(&kHAPLog_Default, "Unable to read previous firmware update state, starting with blank state.");
        HAPRawBufferZero(&(fwUpdateState), sizeof fwUpdateState);
    } else if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPFatalError();
    }

    // Restore previous values for firmware update characteristics
    if (found && !err) {
        firmwareUpdate.HAPProfile.updateState = fwUpdateState.updateState;
        firmwareUpdate.HAPProfile.updateNotReadyReason = fwUpdateState.updateNotReadyReason;
        firmwareUpdate.HAPProfile.stagingNotReadyReason = fwUpdateState.stagingNotReadyReason;
    } else {
        // Not found in KVS
        firmwareUpdate.HAPProfile.updateState = kHAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState_Idle;
        firmwareUpdate.HAPProfile.updateNotReadyReason = 0;
        firmwareUpdate.HAPProfile.stagingNotReadyReason = 0;
    }

    // Initialize HAP profile.
    firmwareUpdate.HAPProfile.updateDuration = kFirmwareUpdateDuration;
    if (!UARPIsVersionZero(&stagingState.stagedFirmwareVersion)) {
        // Asset staged.
        err = UARPVersionToAccessoryFirmwareVersionString(
                (const UARPVersion*) &stagingState.stagedFirmwareVersion,
                firmwareUpdate.stagedFirmwareVersion,
                kHAPFirmwareVersion_MaxLength + 1);
        HAPAssert(err == kHAPError_None);
        if (err == kHAPError_None) {
            FwUpSetHAPProfileStagedFirmwareVersion(server, accessory, firmwareUpdate.stagedFirmwareVersion);
        }
    }

    // Register with UARP for firmware asset processing.
    err = UARPRegisterFirmwareAsset(&uarpFirmwareAssetCallbacks, &(stagingState.stagedFirmwareVersion));
    HAPAssert(!err);

    // Initialize the apply context used when simulating an update.
    firmwareUpdate.applyContext.delay = kFirmwareUpdateDuration - 1;
    firmwareUpdate.applyContext.server = server;
    firmwareUpdate.applyContext.accessory = (HAPAccessory*) accessory;
}

//----------------------------------------------------------------------------------------------------------------------
// Firmware start
// This must be called after the application starts the accessory server. This updates any firmare update
// characterstic values which change as a result of the accessory rebooting.
//----------------------------------------------------------------------------------------------------------------------
void FirmwareUpdateStart(HAPAccessoryServer* server, const HAPAccessory* accessory) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);

    HAPLogDebug(&kHAPLog_Default, "%s", __func__);

    FirmwareUpdateStagingState stagingState;
    HAPRawBufferZero(&(stagingState), sizeof stagingState);

    if (firmwareUpdate.options.persistStaging) {

        bool found = false;
        HAPError err = FwUpGetKeyValueStoreFirmwareStagingState(&stagingState, &found);
        if (err == kHAPError_InvalidData) {
            HAPLogError(
                    &kHAPLog_Default,
                    "%s: Could not find valid data for firmware staging state. Values reset to default.",
                    __func__);
            HAPRawBufferZero(&(stagingState), sizeof stagingState);
        } else if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }
    }
    if (UARPIsVersionZero(&stagingState.stagedFirmwareVersion)) {
        // No staged asset.
        // In case the accessory was rebooted while staging, connected controllers will need to know the following
        // characteristic values have changed. The following functipns are called to update the state and readiness such
        // that a s#/GSN update occurs if the values have changed.
        FwUpSetHAPProfileUpdateState(
                server, accessory, kHAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState_Idle);
        FwUpSetHAPProfileUpdateReadiness(
                server, accessory, kHAPCharacteristicValue_FirmwareUpdateReadiness_UpdateNotReady_StagedUnavailable);

    } else {
        // Asset staged.
        // In case the accessory was rebooted while staging, connected controllers will need to know the following
        // characteristic value has changed. The following function is called to update such that a s#/GSN update occurs
        // if the value has changed.
        FwUpSetHAPProfileUpdateState(
                server, accessory, kHAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState_StagingSucceeded);
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Test support for debug service
//----------------------------------------------------------------------------------------------------------------------

#if (HAP_TESTING == 1)
void FirmwareUpdateSetAccessoryState(
        HAPAccessoryServer* server,
        const HAPAccessory* accessory,
        HAPAccessoryFirmwareUpdateState newState) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    FwUpSetHAPProfileUpdateDuration(server, accessory, newState.updateDuration);
    FwUpSetHAPProfileStagingReadiness(server, accessory, newState.stagingNotReadyReason);
    FwUpSetHAPProfileUpdateReadiness(server, accessory, newState.updateNotReadyReason);
    FwUpSetHAPProfileUpdateState(server, accessory, newState.updateState);
}

void FirmwareUpdateSetStagingNotReadyReason(HAPAccessoryServer* server, const HAPAccessory* accessory, uint8_t bit) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    FwUpSetHAPProfileStagingNotReadyReason(server, accessory, 1 << bit);
}

void FirmwareUpdateClearStagingNotReadyReason(HAPAccessoryServer* server, const HAPAccessory* accessory, uint8_t bit) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    FwUpClearHAPProfileStagingNotReadyReason(server, accessory, 1 << bit);
}

void FirmwareUpdateSetUpdateNotReadyReason(HAPAccessoryServer* server, const HAPAccessory* accessory, uint8_t bit) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    FwUpSetHAPProfileUpdateNotReadyReason(server, accessory, 1 << bit);
}

void FirmwareUpdateClearUpdateNotReadyReason(HAPAccessoryServer* server, const HAPAccessory* accessory, uint8_t bit) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    FwUpClearHAPProfileUpdateNotReadyReason(server, accessory, 1 << bit);
}

void FirmwareUpdateSetApplyDelay(HAPAccessoryServer* server, const HAPAccessory* accessory, uint16_t applyDelay) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    firmwareUpdate.applyContext.delay = applyDelay;
}
#endif // HAP_TESTING

#endif // HAVE_FIRMWARE_UPDATE
