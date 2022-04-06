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

#include "ble_hci.h"

#include "HAPPlatformBLE+Init.h"
#include "HAPPlatformBLEPeripheralManager+Init.h"
#include "HAPPlatformKeyValueStore+Init.h"

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "BLEPeripheralManager" };

/** Delay till next advertisement stop attempt */
#define kStopAdvertisingRetryDelay (HAPMillisecond * 20)

// forward reference
static void RestartAdvertising(HAPPlatformBLEPeripheralManagerRef blePeripheralManager);
static void StopAdvertising(HAPPlatformBLEPeripheralManagerRef blePeripheralManager);

void HAPPlatformBLEPeripheralManagerCreate(
        HAPPlatformBLEPeripheralManager* blePeripheralManager,
        const HAPPlatformBLEPeripheralManagerOptions* options) {
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(options);
    HAPPrecondition(options->keyValueStore);
    HAPPrecondition(options->uuidBases);

    HAPRawBufferZero(blePeripheralManager, sizeof *blePeripheralManager);
    blePeripheralManager->keyValueStore = options->keyValueStore;
    blePeripheralManager->uuidBases = options->uuidBases;
    blePeripheralManager->numUUIDBases = options->numUUIDBases;
    blePeripheralManager->numUsedUUIDBases = 0;
    blePeripheralManager->advertisingHandle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;
    blePeripheralManager->connectionHandle = BLE_CONN_HANDLE_INVALID;
}

void HAPPlatformBLEPeripheralManagerClear(HAPPlatformBLEPeripheralManager* blePeripheralManager) {
    blePeripheralManager->numUsedUUIDBases = 0;
}

void HAPPlatformBLEPeripheralManagerSetDelegate(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
        const HAPPlatformBLEPeripheralManagerDelegate* _Nullable delegate) {
    HAPPrecondition(blePeripheralManager);

    if (delegate) {
        blePeripheralManager->delegate = *delegate;
    } else {
        HAPRawBufferZero(&blePeripheralManager->delegate, sizeof blePeripheralManager->delegate);
    }
}

void HAPPlatformBLEPeripheralManagerSetDeviceAddress(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
        const HAPPlatformBLEPeripheralManagerDeviceAddress* deviceAddress) {
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(deviceAddress);

    HAPAssert(sizeof deviceAddress->bytes == 6);
    HAPLogInfo(
            &logObject,
            "Setting Bluetooth address to %02X:%02X:%02X:%02X:%02X:%02X",
            deviceAddress->bytes[0],
            deviceAddress->bytes[1],
            deviceAddress->bytes[2],
            deviceAddress->bytes[3],
            deviceAddress->bytes[4],
            deviceAddress->bytes[5]);
    uint32_t e = sd_ble_gap_addr_set(&(const ble_gap_addr_t) { .addr_type = BLE_GAP_ADDR_TYPE_RANDOM_STATIC,
                                                               .addr[0] = deviceAddress->bytes[0],
                                                               .addr[1] = deviceAddress->bytes[1],
                                                               .addr[2] = deviceAddress->bytes[2],
                                                               .addr[3] = deviceAddress->bytes[3],
                                                               .addr[4] = deviceAddress->bytes[4],
                                                               .addr[5] = deviceAddress->bytes[5] });
    if (e) {
        HAPLogError(&logObject, "sd_ble_gap_addr_set failed: 0x%02x.", (unsigned int) e);
        HAPFatalError();
    }
}

void HAPPlatformBLEPeripheralManagerSetDeviceName(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
        const char* deviceName) {
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(deviceName);

    // Set Bluetooth device name, truncating it to the available maximum length.
    ble_gap_conn_sec_mode_t secMode;
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&secMode);
    uint32_t e = sd_ble_gap_device_name_set(
            &secMode,
            (const uint8_t*) deviceName,
            (uint16_t) HAPMin(HAPStringGetNumBytes(deviceName), BLE_GAP_DEVNAME_DEFAULT_LEN));
    if (e) {
        HAPLogError(&logObject, "sd_ble_gap_device_name_set %s failed: 0x%02x.", deviceName, (unsigned int) e);
        HAPFatalError();
    }
}

HAPError HAPPlatformBLEPeripheralManagerGetAdvertisementAddress(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
        HAPPlatformBLEPeripheralManagerDeviceAddress* deviceAddress) {
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(deviceAddress);

    // Set Bluetooth device name, truncating it to the available maximum length.
    ble_gap_conn_sec_mode_t secMode;
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&secMode);
    ble_gap_addr_t gapAddress;
    uint32_t e = sd_ble_gap_addr_get(&gapAddress);
    if (e) {
        HAPLogError(&logObject, "sd_ble_gap_addr_get failed: 0x%02x.", (unsigned int) e);
        HAPFatalError();
    }
    HAPRawBufferCopyBytes(deviceAddress->bytes, gapAddress.addr, sizeof deviceAddress->bytes);
    return kHAPError_None;
}

static bool servicesPublished;
/**
 * Do not add services for services were already added.
 *
 * Note that there is no way to remove added services with Nordic stack.
 * Hence when using multiprotocol libraries, we have to assume that services never change and
 * once added, services should never be added back again till device reboots.
 */
static bool doNotAddServices = false;

bool HAPPlatformBLEPeripheralManagerAllowsServiceRefresh(HAPPlatformBLEPeripheralManagerRef blePeripheralManager) {
    HAPPrecondition(blePeripheralManager);

    // If using nRF52 multiprotocol library, it's not possible to refresh services.
    // Hence, if services were added and done once, do not allow the services.
    return !doNotAddServices;
}

void HAPPlatformBLEPeripheralManagerRemoveAllServices(HAPPlatformBLEPeripheralManagerRef blePeripheralManager) {
    HAPPrecondition(blePeripheralManager);

    if (servicesPublished) {
        HAPLog(&logObject, "Removing services not supported. Re-initialize soft device to set new database.");
        doNotAddServices = true;
        servicesPublished = false;
    }
}

// Bytes 12 and 13 of the provided UUID will not be used internally,
// as those are replaced by the 16-bit uuid field in ble_uuid_t.
// See https://developer.nordicsemi.com/nRF51_SDK/nRF51_SDK_v8.x.x/doc/8.1.0/s110/html/a01284.html
#define kShortUUID_Offset ((size_t) 12)

/**
 * Compares the bases of two 128-bit UUIDs.
 *
 * @param      uuid                 UUID to compare.
 * @param      otherUUID            UUID to compare with.
 *
 * @return true                     If the bases of both UUIDs are the same.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
static bool CompareBases(const uint8_t* uuid, const uint8_t* otherUUID) {
    HAPPrecondition(uuid);
    HAPPrecondition(otherUUID);

    return (uuid[0] == otherUUID[0]) && (uuid[1] == otherUUID[1]) && (uuid[2] == otherUUID[2]) &&
           (uuid[3] == otherUUID[3]) && (uuid[4] == otherUUID[4]) && (uuid[5] == otherUUID[5]) &&
           (uuid[6] == otherUUID[6]) && (uuid[7] == otherUUID[7]) && (uuid[8] == otherUUID[8]) &&
           (uuid[9] == otherUUID[9]) && (uuid[10] == otherUUID[10]) && (uuid[11] == otherUUID[11]) &&
           (uuid[14] == otherUUID[14]) && (uuid[15] == otherUUID[15]);
}

/**
 * Gets a Nordic ble_uuid_t from a UUID. UUID types are automatically registered.
 *
 * @param      blePeripheralManager BLE peripheral manager.
 * @param      uuid                 128-bit UUID to convert.
 * @param[out] bleUUID              Resulting ble_uuid_t.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If no more bases may be registered.
 */
HAP_RESULT_USE_CHECK
static HAPError GetBLEUUID(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
        const HAPPlatformBLEPeripheralManagerUUID* uuid,
        ble_uuid_t* bleUUID) {
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(uuid);
    HAPPrecondition(bleUUID);

    bleUUID->uuid = HAPReadLittleUInt16(&uuid->bytes[kShortUUID_Offset]);

    // Check for Bluetooth Base UUID (0000xxxx-0000-1000-8000-00805F9B34FB).
    // See Bluetooth Core Specification Version 5
    // Vol 3 Part B Section 2.5.1 UUID
    uint8_t bluetoothBaseUUID[] = { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80,
                                    0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    if (CompareBases(uuid->bytes, bluetoothBaseUUID)) {
        bleUUID->type = BLE_UUID_TYPE_BLE;
        return kHAPError_None;
    }

    // Find matching UUID base.
    for (size_t i = 0; i < blePeripheralManager->numUsedUUIDBases; i++) {
        const ble_uuid128_t* base = &blePeripheralManager->uuidBases[i];

        if (CompareBases(uuid->bytes, base->uuid128)) {
            bleUUID->type = base->uuid128[kShortUUID_Offset];
            return kHAPError_None;
        }
    }

    // UUID base not found. Check for space to allocate a new one.
    if (blePeripheralManager->numUsedUUIDBases >= blePeripheralManager->numUUIDBases) {
        HAPLog(&logObject, "Not enough space to allocate UUID base. Please increase the number of UUID bases.");
        return kHAPError_OutOfResources;
    }

    // Set up new UUID base record (little-endian).
    // UUIDs are represented as a base index plus the value of bytes 12 and 13 to conserve memory.
    // The bases must be registered with the SoftDevice to get a valid index.
    ble_uuid128_t* base = &blePeripheralManager->uuidBases[blePeripheralManager->numUsedUUIDBases];
    base->uuid128[0] = uuid->bytes[0];
    base->uuid128[1] = uuid->bytes[1];
    base->uuid128[2] = uuid->bytes[2];
    base->uuid128[3] = uuid->bytes[3];
    base->uuid128[4] = uuid->bytes[4];
    base->uuid128[5] = uuid->bytes[5];
    base->uuid128[6] = uuid->bytes[6];
    base->uuid128[7] = uuid->bytes[7];
    base->uuid128[8] = uuid->bytes[8];
    base->uuid128[9] = uuid->bytes[9];
    base->uuid128[10] = uuid->bytes[10];
    base->uuid128[11] = uuid->bytes[11];
    base->uuid128[12] = 0; // Not used in UUID base.
    base->uuid128[13] = 0; // Not used in UUID base.
    base->uuid128[14] = uuid->bytes[14];
    base->uuid128[15] = uuid->bytes[15];

    // Register new UUID base.
    uint32_t e = sd_ble_uuid_vs_add(base, &base->uuid128[kShortUUID_Offset]);
    if (e) {
        HAPLogError(&logObject, "sd_ble_uuid_vs_add failed: 0x%02x.", (unsigned int) e);
        HAPFatalError();
    }

    // Registration succeeded.
    blePeripheralManager->numUsedUUIDBases++;

    bleUUID->type = base->uuid128[kShortUUID_Offset];
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformBLEPeripheralManagerAddService(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
        const HAPPlatformBLEPeripheralManagerUUID* type,
        bool isPrimary) {
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(type);
    HAPPrecondition(!servicesPublished);
    HAPPrecondition(!doNotAddServices);

    HAPError err;

    // Convert type to native UUID.
    ble_uuid_t uuid;
    err = GetBLEUUID(blePeripheralManager, type, &uuid);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // Register service.
    HAPLogDebug(&logObject, "service_add %u %u %u\n", (unsigned) isPrimary, uuid.uuid, uuid.type);
    uint32_t e = sd_ble_gatts_service_add(
            (uint8_t)(isPrimary ? BLE_GATTS_SRVC_TYPE_PRIMARY : BLE_GATTS_SRVC_TYPE_SECONDARY),
            &uuid,
            /* p_handle: */ NULL);
    if (e) {
        HAPLogDebug(&logObject, "service_add returns %u\n", (unsigned) e);
        HAPAssert(e == NRF_ERROR_NO_MEM);
        HAPLog(&logObject, "Ran out of memory while adding service.");
        return kHAPError_OutOfResources;
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformBLEPeripheralManagerAddCharacteristic(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
        const HAPPlatformBLEPeripheralManagerUUID* type,
        HAPPlatformBLEPeripheralManagerCharacteristicProperties properties,
        const void* _Nullable constBytes,
        size_t constNumBytes,
        HAPPlatformBLEPeripheralManagerAttributeHandle* valueHandle,
        HAPPlatformBLEPeripheralManagerAttributeHandle* _Nullable cccDescriptorHandle) {
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(type);
    HAPPrecondition(valueHandle);
    if (properties.notify || properties.indicate) {
        HAPPrecondition(cccDescriptorHandle);
    } else {
        HAPPrecondition(!cccDescriptorHandle);
    }
    HAPPrecondition(!servicesPublished);
    HAPPrecondition(!doNotAddServices);

    HAPError err;

    // Convert type to native UUID.
    ble_uuid_t uuid;
    err = GetBLEUUID(blePeripheralManager, type, &uuid);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // Register characteristic.
    ble_gatts_char_handles_t handles;
    uint32_t e = sd_ble_gatts_characteristic_add(
            BLE_GATT_HANDLE_INVALID,
            &(ble_gatts_char_md_t) {
                    .char_props = { .broadcast = 0,
                                    .read = (uint8_t)(properties.read ? 1 : 0),
                                    .write_wo_resp = (uint8_t)(properties.writeWithoutResponse ? 1 : 0),
                                    .write = (uint8_t)(properties.write ? 1 : 0),
                                    .notify = (uint8_t)(properties.notify ? 1 : 0),
                                    .indicate = (uint8_t)(properties.indicate ? 1 : 0),
                                    .auth_signed_wr = 0 },
                    .char_ext_props = { .reliable_wr = 0, .wr_aux = 0 },
                    .p_char_user_desc = NULL,
                    .char_user_desc_max_size = 0,
                    .char_user_desc_size = 0,
                    .p_char_pf = NULL,
                    .p_user_desc_md = NULL,

                    .p_cccd_md = (properties.notify || properties.indicate) ?
                                         &(ble_gatts_attr_md_t) { .read_perm = { .sm = 1, .lv = 1 },
                                                                  .write_perm = { .sm = 1, .lv = 1 },
                                                                  .vlen = 0,
                                                                  .vloc = BLE_GATTS_VLOC_STACK,
                                                                  .rd_auth = 0,
                                                                  .wr_auth = 1 } :
                                         NULL,
                    .p_sccd_md = NULL },
            &(ble_gatts_attr_t) {
                    .p_uuid = &uuid,
                    .p_attr_md = &(ble_gatts_attr_md_t) { .read_perm = { .sm = (uint8_t)(properties.read ? 1 : 0),
                                                                         .lv = (uint8_t)(properties.read ? 1 : 0) },
                                                          .write_perm = { .sm = (uint8_t)(
                                                                                  (properties.writeWithoutResponse ||
                                                                                   properties.write) ?
                                                                                          1 :
                                                                                          0),
                                                                          .lv = (uint8_t)(
                                                                                  (properties.writeWithoutResponse ||
                                                                                   properties.write) ?
                                                                                          1 :
                                                                                          0) },
                                                          .vlen = 1,
                                                          .vloc = BLE_GATTS_VLOC_USER,
                                                          .rd_auth = 1,
                                                          .wr_auth = 1 },
                    .init_len = sizeof blePeripheralManager->bytes,
                    .max_len = sizeof blePeripheralManager->bytes,
                    .p_value = blePeripheralManager->bytes },
            &handles);
    if (e) {
        HAPAssert(e == NRF_ERROR_NO_MEM);
        HAPLog(&logObject, "Ran out of memory while adding characteristic.");
        return kHAPError_OutOfResources;
    }
    *valueHandle = handles.value_handle;
    if (properties.notify || properties.indicate) {
        *cccDescriptorHandle = handles.cccd_handle;
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformBLEPeripheralManagerAddDescriptor(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
        const HAPPlatformBLEPeripheralManagerUUID* type,
        HAPPlatformBLEPeripheralManagerDescriptorProperties properties,
        const void* _Nullable constBytes,
        size_t constNumBytes,
        HAPPlatformBLEPeripheralManagerAttributeHandle* descriptorHandle) {
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(type);
    HAPPrecondition(descriptorHandle);
    HAPPrecondition(!servicesPublished);

    HAPError err;

    // Convert type to native UUID.
    ble_uuid_t uuid;
    err = GetBLEUUID(blePeripheralManager, type, &uuid);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // Register descriptor.
    uint32_t e = sd_ble_gatts_descriptor_add(
            BLE_GATT_HANDLE_INVALID,
            &(ble_gatts_attr_t) {
                    .p_uuid = &uuid,
                    .p_attr_md = &(ble_gatts_attr_md_t) { .read_perm = { .sm = (uint8_t)(properties.read ? 1 : 0),
                                                                         .lv = (uint8_t)(properties.read ? 1 : 0) },
                                                          .write_perm = { .sm = (uint8_t)(properties.write ? 1 : 0),
                                                                          .lv = (uint8_t)(properties.write ? 1 : 0) },
                                                          .vlen = 1,
                                                          .vloc = BLE_GATTS_VLOC_USER,
                                                          .rd_auth = 1,
                                                          .wr_auth = 1 },
                    .init_len = sizeof blePeripheralManager->bytes,
                    .max_len = sizeof blePeripheralManager->bytes,
                    .p_value = blePeripheralManager->bytes },
            descriptorHandle);
    if (e) {
        HAPAssert(e == NRF_ERROR_NO_MEM);
        HAPLog(&logObject, "Ran out of memory while adding descriptor.");
        return kHAPError_OutOfResources;
    }

    return kHAPError_None;
}

void HAPPlatformBLEPeripheralManagerPublishServices(HAPPlatformBLEPeripheralManagerRef blePeripheralManager) {
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(!servicesPublished);

    servicesPublished = true;
}

/**
 * Timer handler function to try to stop advertising again.
 */
static void HandleStopAdvertisingTimer(HAPPlatformTimerRef timer, void* _Nullable context) {
    HAPPrecondition(context);
    HAPPlatformBLEPeripheralManagerRef blePeripheralManager = context;

    HAPLogDebug(&logObject, "%s", __func__);

    if (blePeripheralManager->stopAdvertisingRetryTimer != timer) {
        // Timer must have expired in a race condition. Ignore this.
        return;
    }
    blePeripheralManager->stopAdvertisingRetryTimer = 0;

    if (blePeripheralManager->isAdvertising) {
        // Note that advertising might have stopped when a connection was made.
        // If not, try to stop advertising.
        StopAdvertising(blePeripheralManager);
    }

    if (!blePeripheralManager->stopAdvertisingRetryTimer && blePeripheralManager->restartAdvertisingAfterStopTimer) {
        // Restarting the advertisement was due upon this timer expiry.
        blePeripheralManager->restartAdvertisingAfterStopTimer = false;
        if (blePeripheralManager->connectionHandle == BLE_CONN_HANDLE_INVALID) {
            // No connection. Can restart advertisement immediately
            HAPLogDebug(&logObject, "Execute pending restart-advertisement.");
            RestartAdvertising(blePeripheralManager);
        } else {
            // Advertisement must be started only after connection is closed
            HAPLogDebug(&logObject, "Pending restart-advertisement is pushed further till connection closes.");
            blePeripheralManager->restartAdvertisingAfterDisconnection = true;
        }
    }
}

/**
 * Starts BLE advertisement immediately from the update parameters stored in the peripheral manager.
 *
 * @param blePeripheralManager   BLE peripheral manager
 */
static void RestartAdvertising(HAPPlatformBLEPeripheralManagerRef blePeripheralManager) {
    HAPPrecondition(blePeripheralManager);

    HAPLogDebug(&logObject, "%s: %d", __func__, (int) blePeripheralManager->isAdvertising);

    if (blePeripheralManager->restartAdvertisingAfterStopTimer) {
        HAPLogDebug(&logObject, "Restarting was already scheduled.");
        return;
    }

    // Stop advertisement if any.
    if (blePeripheralManager->isAdvertising) {
        StopAdvertising(blePeripheralManager);

        if (blePeripheralManager->isAdvertising) {
            // Stopping failed. Flag that restart is pending and quit.
            blePeripheralManager->restartAdvertisingAfterStopTimer = true;
            return;
        }
    }

    // Copy advertising data.
    HAPRawBufferZero(blePeripheralManager->advData, sizeof blePeripheralManager->advData);
    HAPAssert(blePeripheralManager->numUpdatedAdvertisingBytes <= sizeof blePeripheralManager->advData);
    HAPRawBufferCopyBytes(
            blePeripheralManager->advData,
            blePeripheralManager->advDataUpdate,
            blePeripheralManager->numUpdatedAdvertisingBytes);

    // Copy scan response data.
    HAPRawBufferZero(blePeripheralManager->scanRspData, sizeof blePeripheralManager->scanRspData);
    if (blePeripheralManager->numUpdatedScanResponseBytes) {
        HAPAssert(blePeripheralManager->numUpdatedScanResponseBytes <= sizeof blePeripheralManager->scanRspData);
        HAPRawBufferCopyBytes(
                blePeripheralManager->scanRspData,
                blePeripheralManager->scanRspDataUpdate,
                blePeripheralManager->numUpdatedScanResponseBytes);
    }

    // Set advertising parameters.
    uint32_t e = sd_ble_gap_adv_set_configure(
            &blePeripheralManager->advertisingHandle,
            &(const ble_gap_adv_data_t) {
                    .adv_data = { .p_data = blePeripheralManager->advData,
                                  .len = (uint16_t) blePeripheralManager->numUpdatedAdvertisingBytes },
                    .scan_rsp_data = { .p_data = blePeripheralManager->numUpdatedScanResponseBytes > 0 ?
                                                         blePeripheralManager->scanRspData :
                                                         NULL,
                                       .len = (uint16_t) blePeripheralManager->numUpdatedScanResponseBytes } },
            &(const ble_gap_adv_params_t) { .properties = { .type = BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED,
                                                            .anonymous = 0,
                                                            .include_tx_power = 0 },
                                            .p_peer_addr = NULL,
                                            .interval = blePeripheralManager->updatedAdvertisingInterval,
                                            .duration = BLE_GAP_ADV_TIMEOUT_GENERAL_UNLIMITED,
                                            .max_adv_evts = 0,
                                            .channel_mask = { 0x00, 0x00, 0x00, 0x00, 0x00 },
                                            .filter_policy = BLE_GAP_ADV_FP_ANY,
                                            .primary_phy = BLE_GAP_PHY_AUTO,
                                            .secondary_phy = BLE_GAP_PHY_AUTO,
                                            .set_id = 0,
                                            .scan_req_notification = 0 });

    blePeripheralManager->numUpdatedAdvertisingBytes = 0;
    blePeripheralManager->numUpdatedScanResponseBytes = 0;

    if (e) {
        HAPLogError(&logObject, "sd_ble_gap_adv_set_configure failed: 0x%02x.", (unsigned int) e);
        HAPFatalError();
    }

    // Start advertising.
    e = sd_ble_gap_adv_start(blePeripheralManager->advertisingHandle, APP_BLE_CONN_CFG_TAG);
    if (e) {
        HAPLogError(&logObject, "sd_ble_gap_adv_start failed: 0x%02x.", (unsigned int) e);
        HAPFatalError();
    }
    blePeripheralManager->isAdvertising = true;
}

void HAPPlatformBLEPeripheralManagerStartAdvertising(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
        HAPBLEAdvertisingInterval advertisingInterval,
        const void* advertisingBytes,
        size_t numAdvertisingBytes,
        const void* _Nullable scanResponseBytes,
        size_t numScanResponseBytes) {
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(advertisingInterval);
    HAPPrecondition(advertisingBytes);
    HAPPrecondition(numAdvertisingBytes <= UINT16_MAX);
    HAPPrecondition(!numScanResponseBytes || scanResponseBytes);
    HAPPrecondition(numScanResponseBytes <= UINT16_MAX);

    // Copy advertising data.
    HAPRawBufferZero(blePeripheralManager->advDataUpdate, sizeof blePeripheralManager->advDataUpdate);
    HAPAssert(numAdvertisingBytes <= sizeof blePeripheralManager->advDataUpdate);
    HAPRawBufferCopyBytes(blePeripheralManager->advDataUpdate, advertisingBytes, numAdvertisingBytes);
    blePeripheralManager->numUpdatedAdvertisingBytes = numAdvertisingBytes;

    // Copy scan response data.
    HAPRawBufferZero(blePeripheralManager->scanRspDataUpdate, sizeof blePeripheralManager->scanRspDataUpdate);
    if (scanResponseBytes) {
        HAPAssert(numScanResponseBytes > 0);
        HAPAssert(numScanResponseBytes <= sizeof blePeripheralManager->scanRspDataUpdate);
        HAPRawBufferCopyBytes(
                blePeripheralManager->scanRspDataUpdate, HAPNonnullVoid(scanResponseBytes), numScanResponseBytes);
    } else {
        HAPAssert(numScanResponseBytes == 0);
    }
    blePeripheralManager->numUpdatedScanResponseBytes = numScanResponseBytes;

    // Copy the advertising interval.
    blePeripheralManager->updatedAdvertisingInterval = advertisingInterval;

    if (blePeripheralManager->connectionHandle == BLE_CONN_HANDLE_INVALID) {
        // No connection. Can restart advertisement immediately
        RestartAdvertising(blePeripheralManager);
    } else {
        // Advertisement must be started only after connection is closed
        blePeripheralManager->restartAdvertisingAfterDisconnection = true;
    }
}

/**
 * Stops advertising
 */
static void StopAdvertising(HAPPlatformBLEPeripheralManagerRef blePeripheralManager) {
    HAPPrecondition(blePeripheralManager);

    HAPLogDebug(&logObject, "%s", __func__);

    if (blePeripheralManager->stopAdvertisingRetryTimer) {
        HAPLogDebug(&logObject, "Stopping advertisement was already pending.");
    }

    // Note that this function does not release the blePeripheralManager->advertisingHandle.
    uint32_t e = sd_ble_gap_adv_stop(blePeripheralManager->advertisingHandle);
    if (e) {
        HAPLogError(&logObject, "sd_ble_gap_adv_stop failed: 0x%lx", (unsigned long) e);
        // sd_ble_gap_adv_stop() could still be called due to a race condition
        // with a connection is being setup. In such a case, the function returns
        // an invalid-state error.
        // We also see other cases where the function fails with an invalid state error.
        // One cause might be that sd_ble_gap_adv_stop() is called too soon before the initial
        // advertisement is scheduled but it is not clear that is the only case.
        HAPAssert(e == NRF_ERROR_INVALID_STATE);

        // Schedule retry
        HAPError err = HAPPlatformTimerRegister(
                &blePeripheralManager->stopAdvertisingRetryTimer,
                HAPPlatformClockGetCurrent() + kStopAdvertisingRetryDelay,
                HandleStopAdvertisingTimer,
                blePeripheralManager);
        HAPAssert(!err);
        return;
    }
    // sd_ble_gap_adv_stop() doesn't release the advertising handle.
    // That is, the advertising handle still hangs on to the advertising resource.
    // Creating a new handle with sd_ble_gap_adv_set_configure() will leak resources
    // and eventually, as early as in the next call, will cause an error.
    // Hence, instead of clearing the advertising handle, `isAdvertising` flag is
    // used to indicate that the advertising handle is not in use.
    blePeripheralManager->isAdvertising = false;

    // Clear advertising and scan response data.
    HAPRawBufferZero(blePeripheralManager->advData, sizeof blePeripheralManager->advData);
    HAPRawBufferZero(blePeripheralManager->scanRspData, sizeof blePeripheralManager->scanRspData);
}

void HAPPlatformBLEPeripheralManagerStopAdvertising(HAPPlatformBLEPeripheralManagerRef blePeripheralManager) {
    HAPPrecondition(blePeripheralManager);

    // Clear pending restart request
    blePeripheralManager->restartAdvertisingAfterDisconnection = false;
    blePeripheralManager->restartAdvertisingAfterStopTimer = false;

    // Stop advertising.
    if (blePeripheralManager->isAdvertising) {
        StopAdvertising(blePeripheralManager);
    }
}

static void ResetGATTState(HAPPlatformBLEPeripheralManagerRef blePeripheralManager) {
    HAPPrecondition(blePeripheralManager);

    if (blePeripheralManager->gatt.retryResponseTimer) {
        HAPLogDebug(&logObject, "Aborting retry response timer.");
        HAPPlatformTimerDeregister(blePeripheralManager->gatt.retryResponseTimer);
        blePeripheralManager->gatt.retryResponseTimer = 0;
    }
    HAPRawBufferZero(&blePeripheralManager->gatt, sizeof blePeripheralManager->gatt);
}

void HAPPlatformBLEPeripheralManagerCancelCentralConnection(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
        HAPPlatformBLEPeripheralManagerConnectionHandle connectionHandle) {
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(blePeripheralManager->connectionHandle == connectionHandle);

    HAPLogInfo(&logObject, "%s", __func__);
    if (blePeripheralManager->gatt.operation == kHAPPlatformBLEPeripheralManagerGATTOperation_Disconnecting) {
        return;
    }

    // Disconnect from central.
    ResetGATTState(blePeripheralManager);
    blePeripheralManager->gatt.operation = kHAPPlatformBLEPeripheralManagerGATTOperation_Disconnecting;
    uint32_t e = sd_ble_gap_disconnect(connectionHandle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
    if (e == NRF_ERROR_INVALID_STATE) {
        // The connection might have already been closed remotely.
        HAPLog(&logObject, "Ignoring NRF_ERROR_INVALID_STATE in sd_ble_gap_disconnect.");
    } else if (e) {
        HAPLogError(
                &logObject,
                "sd_ble_gap_disconnect(0x%04x) failed: 0x%02x.",
                (unsigned int) connectionHandle,
                (unsigned int) e);
        HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformBLEPeripheralManagerSendHandleValueIndication(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
        HAPPlatformBLEPeripheralManagerConnectionHandle connectionHandle,
        HAPPlatformBLEPeripheralManagerAttributeHandle valueHandle,
        const void* _Nullable bytes,
        size_t numBytes) {
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(blePeripheralManager->connectionHandle == connectionHandle);
    HAPPrecondition(valueHandle);
    HAPPrecondition(!numBytes || bytes);

    HAPLogDebug(&logObject, "%s", __func__);

    if (blePeripheralManager->indicationInProgress) {
        HAPLog(&logObject, "An indication is already in progress.");
        return kHAPError_InvalidState;
    }
    if (blePeripheralManager->pendingATTExchangeMTURequest) {
        // "Once the MTU Exchange Request has been sent, the initiating device shall not send an Attribute Protocol
        // Indication or Notification until after the MTU Exchange Response has been received."
        // See Bluetooth Core Specification Version 5
        // Vol 3 Part F Section 3.4.2.2 Exchange MTU Response
        HAPLog(&logObject, "ATT Exchange MTU Request is pending. Not sending indication.");
        return kHAPError_InvalidState;
    }
    if (blePeripheralManager->gatt.operation == kHAPPlatformBLEPeripheralManagerGATTOperation_Disconnecting) {
        HAPLog(&logObject, "Not sending indication as the connection is being terminated.");
        return kHAPError_InvalidState;
    }

    // Handle Value Indication.
    // See Bluetooth Core Specification Version 5
    // Vol 3 Part F Section 3.4.7.2 Handle Value Indication
    uint16_t n;
    HAPAssert(blePeripheralManager->mtu >= 3);
    if (numBytes > UINT16_MAX || numBytes > (size_t) blePeripheralManager->mtu - 3) {
        HAPLog(&logObject, "Event value is too large.");
        return kHAPError_OutOfResources;
    }
    n = (uint16_t) numBytes;
    if (n) {
        // Sending an indication with a value overrides the GATT buffer.
        // Indications can occur concurrently with a central-initiated GATT transaction.
        HAPLog(&logObject, "Indications with a non-empty value are not supported at this time.");
        return kHAPError_OutOfResources;
    }
    uint32_t e = sd_ble_gatts_hvx(
            connectionHandle,
            &(const ble_gatts_hvx_params_t) { .handle = valueHandle,
                                              .type = BLE_GATT_HVX_INDICATION,
                                              .p_len = &n,
                                              .p_data = bytes ? bytes : &(const uint8_t) { 0 } });
    if (e == NRF_ERROR_INVALID_STATE) {
        HAPLog(&logObject, "Indications are not enabled.");
        return kHAPError_InvalidState;
    } else if (e) {
        HAPLogError(
                &logObject,
                "sd_ble_gatts_hvx(0x%04x) failed: 0x%02x.",
                (unsigned int) connectionHandle,
                (unsigned int) e);
        HAPFatalError();
    }
    blePeripheralManager->indicationInProgress = true;
    return kHAPError_None;
}

static void TrySendingReadResponse(HAPPlatformBLEPeripheralManagerRef blePeripheralManager);

static void RetryResponseTimerExpired(HAPPlatformTimerRef timer, void* _Nullable context) {
    HAPPrecondition(timer);
    HAPPrecondition(context);
    HAPPlatformBLEPeripheralManagerRef blePeripheralManager = context;
    HAPPrecondition(timer == blePeripheralManager->gatt.retryResponseTimer);
    blePeripheralManager->gatt.retryResponseTimer = 0;

    TrySendingReadResponse(blePeripheralManager);
}

static void TrySendingReadResponse(HAPPlatformBLEPeripheralManagerRef blePeripheralManager) {
    HAPPrecondition(blePeripheralManager);

    HAPError err;

    uint16_t connectionHandle = blePeripheralManager->connectionHandle;

    if (HAPPlatformKeyValueStoreNeedsTimeToPersist(blePeripheralManager->keyValueStore)) {
        HAPLog(&logObject, "Key-value store needs time to persist pending data. Delaying BLE response.");
        if (!blePeripheralManager->gatt.retryResponseTimer) {
            err = HAPPlatformTimerRegister(
                    &blePeripheralManager->gatt.retryResponseTimer,
                    HAPPlatformClockGetCurrent() + 50 * HAPMillisecond,
                    RetryResponseTimerExpired,
                    blePeripheralManager);
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                HAPLog(&logObject, "Not enough memory to allocate retry response timer. Requesting disconnect.");
                HAPPlatformBLEPeripheralManagerCancelCentralConnection(blePeripheralManager, connectionHandle);
            }
        }
        return;
    }
    HAPAssert(!blePeripheralManager->gatt.retryResponseTimer);

    HAPAssert(blePeripheralManager->mtu >= 1);
    uint16_t numSent = (uint16_t)(blePeripheralManager->mtu - 1);
    if (blePeripheralManager->gatt.numBytes - blePeripheralManager->gatt.offset < numSent) {
        numSent = blePeripheralManager->gatt.numBytes - blePeripheralManager->gatt.offset;
    }

    HAPLogDebug(
            &logObject,
            "Sending Read Response [%u / %u].",
            blePeripheralManager->gatt.offset + numSent,
            blePeripheralManager->gatt.numBytes);
    HAPAssert(blePeripheralManager->gatt.numBytes >= blePeripheralManager->gatt.offset);
    uint32_t e = sd_ble_gatts_rw_authorize_reply(
            connectionHandle,
            &(const ble_gatts_rw_authorize_reply_params_t) {
                    .type = BLE_GATTS_AUTHORIZE_TYPE_READ,
                    .params.read = { .gatt_status = BLE_GATT_STATUS_SUCCESS,
                                     .update = 1,
                                     .offset = blePeripheralManager->gatt.offset,
                                     .len = blePeripheralManager->gatt.numBytes - blePeripheralManager->gatt.offset,
                                     .p_data = &blePeripheralManager->bytes[blePeripheralManager->gatt.offset] } });
    if (e) {
        if (e == NRF_ERROR_INVALID_STATE) {
            // Note that this error could happen if the BLE central is disconnected
            // before making the reply call.
            // Hence, log the failure and continue operation.
            HAPLogInfo(
                    &logObject,
                    "sd_ble_gatts_rw_authorize_reply(0x%04x) failed with invalid state. Connection must be "
                    "down.",
                    (unsigned int) connectionHandle);
        } else {
            HAPLogError(
                    &logObject,
                    "sd_ble_gatts_rw_authorize_reply(0x%04x) failed: 0x%02x.",
                    (unsigned int) connectionHandle,
                    (unsigned int) e);
            HAPFatalError();
        }
    }

    // Update state.
    blePeripheralManager->gatt.offset += numSent;
    if (blePeripheralManager->gatt.offset == blePeripheralManager->gatt.numBytes) {
        if (numSent == blePeripheralManager->mtu - 1) {
            HAPLogDebug(&logObject, "Expecting additional ATT Read Blob Request to resolve length ambiguity.");
        } else if (blePeripheralManager->secondClientMTU && numSent >= blePeripheralManager->secondClientMTU - 1) {
            // Controller requests additional MTU update with different value while using aptX Bluetooth headphones.
            // However, the previous MTU is still valid, except for ATT Read Blob Requests where the new MTU is used
            // to determine whether an additional ATT Read Blob Request should be sent.
            // Observed on macOS 10.13.2 on MacBook Pro (15-inch, 2016).
            HAPLogError(
                    &logObject,
                    "[macOS 10.13.2 Bluetooth Bugs] aptX Bluetooth headphones detected. "
                    "Expecting spurious ATT Read Blob Request.");
        } else {
            ResetGATTState(blePeripheralManager);
        }
    }
}

static void HandleRWAuthorizeRequestBLEEvent(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
        uint16_t connectionHandle,
        const ble_gatts_evt_rw_authorize_request_t* event) {
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(blePeripheralManager->connectionHandle == connectionHandle);
    HAPPrecondition(event);

    HAPError err;

    // Handle macOS Bluetooth bugs.
    if ((event->type == BLE_GATTS_AUTHORIZE_TYPE_READ && !event->request.read.offset) ||
        event->type != BLE_GATTS_AUTHORIZE_TYPE_READ) {
        if (blePeripheralManager->secondClientMTU &&
            blePeripheralManager->gatt.operation == kHAPPlatformBLEPeripheralManagerGATTOperation_Read &&
            blePeripheralManager->gatt.offset == blePeripheralManager->gatt.numBytes) {
            // Controller requests additional MTU update with different value while using aptX Bluetooth headphones.
            // However, the previous MTU is still valid, except for ATT Read Blob Requests where the new MTU is used
            // to determine whether an additional ATT Read Blob Request should be sent.
            // Observed on macOS 10.13.2 on MacBook Pro (15-inch, 2016).
            HAPLogError(
                    &logObject,
                    "[macOS 10.13.2 Bluetooth Bugs] aptX Bluetooth headphones detected. "
                    "Expected spurious ATT Read Blob Request not received.");
            ResetGATTState(blePeripheralManager);
        }
    }

    // Process prepared writes and blob reads.
    switch (event->type) {
        case BLE_GATTS_AUTHORIZE_TYPE_READ: {
            // Process request.
            bool ok = true;
            if (!event->request.read.offset) {
                // Read Request.
                // See Bluetooth Core Specification Version 5
                // Vol 3 Part F Section 3.4.4.3 Read Request
                HAPLogDebug(&logObject, "(0x%04x) ATT Read Request.", (unsigned int) event->request.read.handle);

                if (blePeripheralManager->gatt.operation) {
                    HAPLog(&logObject, "Received Read Request while other operation is in progress.");
                    ok = false;
                } else {
                    // Start new read transaction.
                    if (blePeripheralManager->delegate.handleReadRequest) {
                        size_t len;
                        err = blePeripheralManager->delegate.handleReadRequest(
                                blePeripheralManager,
                                connectionHandle,
                                event->request.read.handle,
                                blePeripheralManager->bytes,
                                sizeof blePeripheralManager->bytes,
                                &len,
                                blePeripheralManager->delegate.context);
                        if (err) {
                            HAPAssert(err == kHAPError_InvalidState || err == kHAPError_OutOfResources);
                            ok = false;
                        } else if (blePeripheralManager->gatt.operation) {
                            HAPAssert(
                                    blePeripheralManager->gatt.operation ==
                                    kHAPPlatformBLEPeripheralManagerGATTOperation_Disconnecting);
                            HAPLogInfo(&logObject, "Requested disconnect while handling read request.");
                            ok = false;
                        } else {
                            blePeripheralManager->gatt.operation = kHAPPlatformBLEPeripheralManagerGATTOperation_Read;
                            blePeripheralManager->gatt.handle = event->request.read.handle;
                            blePeripheralManager->gatt.offset = 0;
                            HAPAssert(len <= sizeof blePeripheralManager->bytes);
                            blePeripheralManager->gatt.numBytes = (uint16_t) len;
                        }
                    } else {
                        HAPLog(&logObject, "No read request handler plugged in. Sending error response.");
                        ok = false;
                    }
                }
            } else {
                // Read Blob Request.
                // See Bluetooth Core Specification Version 5
                // Vol 3 Part F Section 3.4.4.5 Read Blob Request
                HAPLogDebug(&logObject, "(0x%04x) ATT Read Blob Request.", (unsigned int) event->request.read.handle);
                if (blePeripheralManager->gatt.operation != kHAPPlatformBLEPeripheralManagerGATTOperation_Read) {
                    HAPLog(&logObject, "Received Read Blob Request without prior Read Request.");
                    ok = false;
                } else if (blePeripheralManager->gatt.handle != event->request.read.handle) {
                    HAPLog(&logObject,
                           "Received Read Blob Request for a different characteristic than prior Read Request.");
                    ok = false;
                } else if (blePeripheralManager->gatt.offset != event->request.read.offset) {
                    HAPLog(&logObject,
                           "Received Read Blob Request with non-sequential offset (expected: %u, actual: %u).",
                           blePeripheralManager->gatt.offset,
                           event->request.read.offset);
                    ok = false;
                }
            }

            // Send response.
            if (!ok) {
                if (blePeripheralManager->gatt.operation ==
                    kHAPPlatformBLEPeripheralManagerGATTOperation_Disconnecting) {
                    HAPLogInfo(&logObject, "Not sending any response as disconnect has been requested.");
                } else {
                    // Certain HAT / iOS versions do not cope well with GATT errors and get stuck.
                    HAPLog(&logObject, "Error occurred - requesting disconnect.");
                    HAPPlatformBLEPeripheralManagerCancelCentralConnection(blePeripheralManager, connectionHandle);
                }
            } else {
                TrySendingReadResponse(blePeripheralManager);
            }
            break;
        }
        case BLE_GATTS_AUTHORIZE_TYPE_WRITE: {
            // Process request.
            bool ok = true;
            uint16_t attributeHandle = event->request.write.handle;
            switch (event->request.write.op) {
                case BLE_GATTS_OP_WRITE_REQ:
                case BLE_GATTS_OP_WRITE_CMD:
                case BLE_GATTS_OP_SIGN_WRITE_CMD: {
                    if (event->request.write.op == BLE_GATTS_OP_WRITE_REQ) {
                        // Write Request.
                        // See Bluetooth Core Specification Version 5
                        // Vol 3 Part F Section 3.4.5.1 Write Request
                        HAPLogDebug(&logObject, "(0x%04x) ATT Write Request.", (unsigned int) attributeHandle);
                    } else if (event->request.write.op == BLE_GATTS_OP_WRITE_CMD) {
                        // Write Command.
                        // See Bluetooth Core Specification Version 5
                        // Vol 3 Part F Section 3.4.5.3 Write Command
                        HAPLogDebug(&logObject, "(0x%04x) ATT Write Command.", (unsigned int) attributeHandle);
                    } else {
                        HAPAssert(event->request.write.op == BLE_GATTS_OP_SIGN_WRITE_CMD);
                        // Signed Write Command.
                        // See Bluetooth Core Specification Version 5
                        // Vol 3 Part F Section 3.4.5.4 Signed Write Command
                        HAPLogDebug(&logObject, "(0x%04x) ATT Signed Write Command.", (unsigned int) attributeHandle);
                    }
                    if (blePeripheralManager->gatt.operation) {
                        HAPLog(&logObject, "Received Write Request while other operation is in progress.");
                        ok = false;
                    } else {
                        // Start new write transaction.
                        HAPAssert(event->request.write.offset == 0);
                        HAPAssert(event->request.write.len <= sizeof blePeripheralManager->bytes);
                        if (blePeripheralManager->delegate.handleWriteRequest) {
                            blePeripheralManager->gatt.operation = kHAPPlatformBLEPeripheralManagerGATTOperation_Write;
                            blePeripheralManager->gatt.handle = attributeHandle;
                            HAPRawBufferCopyBytes(
                                    blePeripheralManager->bytes, event->request.write.data, event->request.write.len);
                            blePeripheralManager->gatt.offset = 0;
                            blePeripheralManager->gatt.numBytes = event->request.write.len;
                            err = blePeripheralManager->delegate.handleWriteRequest(
                                    blePeripheralManager,
                                    connectionHandle,
                                    blePeripheralManager->gatt.handle,
                                    blePeripheralManager->bytes,
                                    blePeripheralManager->gatt.numBytes,
                                    blePeripheralManager->delegate.context);
                            if (err) {
                                HAPAssert(
                                        err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                                        err == kHAPError_OutOfResources);
                                ok = false;
                            } else if (
                                    blePeripheralManager->gatt.operation !=
                                    kHAPPlatformBLEPeripheralManagerGATTOperation_Write) {
                                HAPAssert(
                                        blePeripheralManager->gatt.operation ==
                                        kHAPPlatformBLEPeripheralManagerGATTOperation_Disconnecting);
                                HAPLogInfo(&logObject, "Requested disconnect while handling write request.");
                                ok = false;
                            } else {
                                ResetGATTState(blePeripheralManager);
                            }
                        } else {
                            HAPLog(&logObject, "No write request handler plugged in. Sending error response.");
                            ok = false;
                        }
                    }
                    break;
                }
                case BLE_GATTS_OP_PREP_WRITE_REQ: {
                    // Prepare Write Request.
                    // See Bluetooth Core Specification Version 5
                    // Vol 3 Part F Section 3.4.6.1 Prepare Write Request
                    HAPLogDebug(&logObject, "(0x%04x) ATT Prepare Write Request.", (unsigned int) attributeHandle);
                    if (!event->request.write.offset) {
                        if (blePeripheralManager->gatt.operation) {
                            HAPLog(&logObject, "Received Prepare Write Request while other operation is in progress.");
                            ok = false;
                        } else if (sizeof blePeripheralManager->bytes < event->request.write.len) {
                            HAPLog(&logObject, "Prepare Write Request data does not fit into buffer.");
                            ok = false;
                        } else {
                            // Start new write transaction.
                            blePeripheralManager->gatt.operation = kHAPPlatformBLEPeripheralManagerGATTOperation_Write;
                            blePeripheralManager->gatt.handle = event->request.read.handle;
                            blePeripheralManager->gatt.offset = 0;
                            blePeripheralManager->gatt.numBytes = 0;
                        }
                    } else {
                        if (blePeripheralManager->gatt.operation !=
                            kHAPPlatformBLEPeripheralManagerGATTOperation_Write) {
                            HAPLog(&logObject, "Received Prepare Write Request without prior request at offset 0.");
                            ok = false;
                        } else if (blePeripheralManager->gatt.handle != attributeHandle) {
                            HAPLog(&logObject,
                                   "Received Prepare Write Request for a different characteristic than before.");
                            ok = false;
                        } else if (blePeripheralManager->gatt.offset != event->request.write.offset) {
                            HAPLog(&logObject, "Received Prepare Write Request with non-sequential offset.");
                            ok = false;
                        } else if (
                                sizeof blePeripheralManager->bytes - blePeripheralManager->gatt.offset <
                                event->request.write.len) {
                            HAPLog(&logObject, "Prepare Write Request data does not fit into buffer.");
                            ok = false;
                        }
                    }

                    // Update state.
                    if (ok) {
                        HAPRawBufferCopyBytes(
                                &blePeripheralManager->bytes[event->request.write.offset],
                                event->request.write.data,
                                event->request.write.len);
                        blePeripheralManager->gatt.offset += event->request.write.len;
                        blePeripheralManager->gatt.numBytes += event->request.write.len;
                    }
                    break;
                }
                case BLE_GATTS_OP_EXEC_WRITE_REQ_CANCEL:
                case BLE_GATTS_OP_EXEC_WRITE_REQ_NOW: {
                    // Execute Write Request.
                    // See Bluetooth Core Specification Version 5
                    // Vol 3 Part F Section 3.4.6.3 Execute Write Request
                    HAPLogDebug(&logObject, "(0x%04x) ATT Execute Write Request.", (unsigned int) attributeHandle);
                    if (blePeripheralManager->gatt.operation != kHAPPlatformBLEPeripheralManagerGATTOperation_None &&
                        blePeripheralManager->gatt.operation != kHAPPlatformBLEPeripheralManagerGATTOperation_Write) {
                        HAPLog(&logObject, "Received Execute Write Request while other operation is in progress.");
                        ok = false;
                    }
                    if (blePeripheralManager->gatt.operation == kHAPPlatformBLEPeripheralManagerGATTOperation_Write &&
                        event->request.write.op == BLE_GATTS_OP_EXEC_WRITE_REQ_NOW) {
                        if (blePeripheralManager->delegate.handleWriteRequest) {
                            err = blePeripheralManager->delegate.handleWriteRequest(
                                    blePeripheralManager,
                                    connectionHandle,
                                    blePeripheralManager->gatt.handle,
                                    blePeripheralManager->bytes,
                                    blePeripheralManager->gatt.numBytes,
                                    blePeripheralManager->delegate.context);
                            if (err) {
                                HAPAssert(
                                        err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                                        err == kHAPError_OutOfResources);
                                ok = false;
                            } else if (
                                    blePeripheralManager->gatt.operation ==
                                    kHAPPlatformBLEPeripheralManagerGATTOperation_Disconnecting) {
                                HAPLogInfo(&logObject, "Requested disconnect while handling write request.");
                                ok = false;
                            }
                        } else {
                            HAPLog(&logObject, "No write request handler plugged in. Sending error response.");
                            ok = false;
                        }
                    }

                    // Update state.
                    if (blePeripheralManager->gatt.operation == kHAPPlatformBLEPeripheralManagerGATTOperation_Write) {
                        // Queue must be cleared also in error case.
                        ResetGATTState(blePeripheralManager);
                    }
                    break;
                }
                case BLE_GATTS_OP_INVALID: {
                }
                    HAPFatalError();
                default: {
                    HAPLogError(
                            &logObject,
                            "Unexpected BLE_GATTS_AUTHORIZE_TYPE_WRITE op: 0x%02x.",
                            event->request.write.op);
                }
                    HAPFatalError();
            }

            // Send response.
            if (!ok) {
                if (blePeripheralManager->gatt.operation ==
                    kHAPPlatformBLEPeripheralManagerGATTOperation_Disconnecting) {
                    HAPLogInfo(&logObject, "Not sending any response as disconnect has been requested.");
                } else {
                    // Certain HAT / iOS versions do not cope well with GATT errors and get stuck.
                    HAPLog(&logObject, "Error occurred - requesting disconnect.");
                    HAPPlatformBLEPeripheralManagerCancelCentralConnection(blePeripheralManager, connectionHandle);
                }
            } else {
                HAPLogDebug(&logObject, "Sending Write Response.");
                uint32_t e = sd_ble_gatts_rw_authorize_reply(
                        connectionHandle,
                        &(const ble_gatts_rw_authorize_reply_params_t) {
                                .type = BLE_GATTS_AUTHORIZE_TYPE_WRITE,
                                .params.write = { .gatt_status = BLE_GATT_STATUS_SUCCESS,
                                                  .update = 1,
                                                  .offset = event->request.write.offset,
                                                  .len = event->request.write.len,
                                                  .p_data = event->request.write.data } });
                if (e) {
                    if (e == NRF_ERROR_INVALID_STATE) {
                        // Note that this error could happen if the BLE central is disconnected
                        // before making the reply call.
                        // Hence, log the failure and continue operation.
                        HAPLogInfo(
                                &logObject,
                                "sd_ble_gatts_rw_authorize_reply(0x%04x) failed with invalid state. Connection must be "
                                "down.",
                                (unsigned int) connectionHandle);
                    } else {
                        HAPLogError(
                                &logObject,
                                "sd_ble_gatts_rw_authorize_reply(0x%04x) failed: 0x%02x.",
                                (unsigned int) connectionHandle,
                                (unsigned int) e);
                        HAPFatalError();
                    }
                }
            }
            break;
        }
        default: {
            HAPLogError(&logObject, "Unexpected BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST type: 0x%02x.", event->type);
        }
            HAPFatalError();
    }
}

#define kHAPPlatformBLEPeripheralManager_ATTExchangeMTURequestDelay ((HAPTime)(1 * HAPSecond))

static void SendATTExchangeMTURequest(HAPPlatformBLEPeripheralManagerRef blePeripheralManager) {
    HAPPrecondition(blePeripheralManager);

    HAPLog(&logObject, "Sending ATT Exchange MTU Request (%u).", NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
    uint32_t e =
            sd_ble_gattc_exchange_mtu_request(blePeripheralManager->connectionHandle, NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
    if (e == NRF_ERROR_BUSY) {
        HAPLog(&logObject, "sd_ble_gattc_exchange_mtu_request failed (NRF_ERROR_BUSY). Retrying after next event.");
        blePeripheralManager->pendingATTExchangeMTURequest = true;
    }
    if (e) {
        HAPLogError(&logObject, "sd_ble_gattc_exchange_mtu_request failed (%lu).", (unsigned long) e);
        return;
    }

    HAPAssert(blePeripheralManager->pendingATTExchangeMTURequest);
    blePeripheralManager->pendingATTExchangeMTURequest = false;
    HAPAssert(!blePeripheralManager->pendingATTExchangeMTUResponse);
    blePeripheralManager->pendingATTExchangeMTUResponse = true;
}

static void RequestMTUUpdateTimerExpired(HAPPlatformTimerRef timer, void* _Nullable context) {
    HAPPrecondition(context);
    HAPPlatformBLEPeripheralManagerRef blePeripheralManager = context;
    HAPPrecondition(timer == blePeripheralManager->requestMTUUpdateTimer);
    blePeripheralManager->requestMTUUpdateTimer = 0;

    HAPLog(&logObject, "Central did not request MTU update. Requesting from accessory side.");
    HAPAssert(!blePeripheralManager->pendingATTExchangeMTURequest);
    blePeripheralManager->pendingATTExchangeMTURequest = true;
    SendATTExchangeMTURequest(blePeripheralManager);
}

static void HandleClientMTU(HAPPlatformBLEPeripheralManagerRef blePeripheralManager, uint16_t clientMTU) {
    HAPPrecondition(blePeripheralManager);

    if (!blePeripheralManager->clientMTU) {
        blePeripheralManager->clientMTU = clientMTU;

        // ATT Exchange MTU procedure is now complete.
        HAPAssert(NRF_SDH_BLE_GATT_MAX_MTU_SIZE <= sizeof blePeripheralManager->bytes);
        uint16_t mtu = HAPMin((uint16_t) NRF_SDH_BLE_GATT_MAX_MTU_SIZE, blePeripheralManager->clientMTU);
        mtu = HAPMax(mtu, BLE_L2CAP_MTU_MIN);
        HAPAssert(sizeof blePeripheralManager->bytes >= mtu);
        blePeripheralManager->mtu = mtu;
        HAPLog(&logObject, "New MTU: %u.", blePeripheralManager->mtu);
    } else if (blePeripheralManager->clientMTU != clientMTU) {
        if (blePeripheralManager->secondClientMTU && blePeripheralManager->secondClientMTU != clientMTU) {
            HAPLogError(&logObject, "Received too many conflicting ATT Exchange MTU Requests.");
            HAPPlatformBLEPeripheralManagerCancelCentralConnection(
                    blePeripheralManager, blePeripheralManager->connectionHandle);
            return;
        }

        // Compute client MTU.
        HAPAssert(NRF_SDH_BLE_GATT_MAX_MTU_SIZE <= sizeof blePeripheralManager->bytes);
        uint16_t mtu = HAPMin((uint16_t) NRF_SDH_BLE_GATT_MAX_MTU_SIZE, clientMTU);
        mtu = HAPMax(mtu, BLE_L2CAP_MTU_MIN);
        HAPAssert(sizeof blePeripheralManager->bytes >= mtu);

        // Controller requests additional MTU update with different value while using aptX Bluetooth headphones.
        // However, the previous MTU is still valid, except for ATT Read Blob Requests where the new MTU is used
        // to determine whether an additional ATT Read Blob Request should be sent.
        // Observed on macOS 10.13.2 on MacBook Pro (15-inch, 2016).
        HAPLogError(
                &logObject,
                "[macOS 10.13.2 Bluetooth Bugs] aptX Bluetooth headphones detected. "
                "Staying on MTU %u instead of new MTU %u.",
                blePeripheralManager->mtu,
                mtu);
        blePeripheralManager->secondClientMTU = mtu;
    }
}

void HAPPlatformBLEPeripheralManagerHandleBLEEvent(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
        const ble_evt_t* event) {
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(event);

    HAPError err;

    /* Log event: */ {
#define AssignEventName(id) \
    case id: \
        eventName = #id; \
        break
        const char* eventName;
        switch (event->header.evt_id) {
            AssignEventName(BLE_EVT_USER_MEM_REQUEST);
            AssignEventName(BLE_EVT_USER_MEM_RELEASE);
            AssignEventName(BLE_GAP_EVT_CONNECTED);
            AssignEventName(BLE_GAP_EVT_DISCONNECTED);
            AssignEventName(BLE_GAP_EVT_CONN_PARAM_UPDATE);
            AssignEventName(BLE_GAP_EVT_SEC_PARAMS_REQUEST);
            AssignEventName(BLE_GAP_EVT_SEC_INFO_REQUEST);
            AssignEventName(BLE_GAP_EVT_PASSKEY_DISPLAY);
            AssignEventName(BLE_GAP_EVT_KEY_PRESSED);
            AssignEventName(BLE_GAP_EVT_AUTH_KEY_REQUEST);
            AssignEventName(BLE_GAP_EVT_LESC_DHKEY_REQUEST);
            AssignEventName(BLE_GAP_EVT_AUTH_STATUS);
            AssignEventName(BLE_GAP_EVT_CONN_SEC_UPDATE);
            AssignEventName(BLE_GAP_EVT_TIMEOUT);
            AssignEventName(BLE_GAP_EVT_RSSI_CHANGED);
            AssignEventName(BLE_GAP_EVT_ADV_REPORT);
            AssignEventName(BLE_GAP_EVT_SEC_REQUEST);
            AssignEventName(BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST);
            AssignEventName(BLE_GAP_EVT_SCAN_REQ_REPORT);
            AssignEventName(BLE_GAP_EVT_PHY_UPDATE_REQUEST);
            AssignEventName(BLE_GAP_EVT_PHY_UPDATE);
            AssignEventName(BLE_GAP_EVT_DATA_LENGTH_UPDATE_REQUEST);
            AssignEventName(BLE_GAP_EVT_DATA_LENGTH_UPDATE);
            AssignEventName(BLE_GAP_EVT_QOS_CHANNEL_SURVEY_REPORT);
            AssignEventName(BLE_GAP_EVT_ADV_SET_TERMINATED);
            AssignEventName(BLE_GATTC_EVT_PRIM_SRVC_DISC_RSP);
            AssignEventName(BLE_GATTC_EVT_REL_DISC_RSP);
            AssignEventName(BLE_GATTC_EVT_CHAR_DISC_RSP);
            AssignEventName(BLE_GATTC_EVT_DESC_DISC_RSP);
            AssignEventName(BLE_GATTC_EVT_ATTR_INFO_DISC_RSP);
            AssignEventName(BLE_GATTC_EVT_CHAR_VAL_BY_UUID_READ_RSP);
            AssignEventName(BLE_GATTC_EVT_READ_RSP);
            AssignEventName(BLE_GATTC_EVT_CHAR_VALS_READ_RSP);
            AssignEventName(BLE_GATTC_EVT_WRITE_RSP);
            AssignEventName(BLE_GATTC_EVT_HVX);
            AssignEventName(BLE_GATTC_EVT_EXCHANGE_MTU_RSP);
            AssignEventName(BLE_GATTC_EVT_TIMEOUT);
            AssignEventName(BLE_GATTC_EVT_WRITE_CMD_TX_COMPLETE);
            AssignEventName(BLE_GATTS_EVT_WRITE);
            AssignEventName(BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST);
            AssignEventName(BLE_GATTS_EVT_SYS_ATTR_MISSING);
            AssignEventName(BLE_GATTS_EVT_HVC);
            AssignEventName(BLE_GATTS_EVT_SC_CONFIRM);
            AssignEventName(BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST);
            AssignEventName(BLE_GATTS_EVT_TIMEOUT);
            AssignEventName(BLE_GATTS_EVT_HVN_TX_COMPLETE);
            default:
                eventName = "Unknown";
                break;
        }
#undef AssignEventName
        HAPLogDebug(&logObject, "BLE Event: %s (0x%04X).", eventName, event->header.evt_id);
    }

    if (!HAPPlatformBLEIsInitialized()) {
        if (event->header.evt_id == BLE_GAP_EVT_CONNECTED) {
            // Controller was connected in a race condition while stopping BLE transport.
            // Disconnect immediately.
            HAPLogInfo(&logObject, "BLE is connected when BLE transport is not active. Will disconnect.");
            uint32_t e =
                    sd_ble_gap_disconnect(event->evt.gap_evt.conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if (e) {
                HAPAssert(e == NRF_ERROR_INVALID_STATE);
            }
            // Update advertisement state.
            blePeripheralManager->isAdvertising = false;
        }
        // All other events that must have occurred from a connection through a race condition
        // should be ignored.
        return;
    }

    switch (event->header.evt_id) {
        case BLE_EVT_USER_MEM_REQUEST: {
            const ble_evt_user_mem_request_t* ev = &event->evt.common_evt.params.user_mem_request;
            HAPAssert(blePeripheralManager->connectionHandle == event->evt.common_evt.conn_handle);

            switch (ev->type) {
                case BLE_USER_MEM_TYPE_GATTS_QUEUED_WRITES: {
                    uint32_t e = sd_ble_user_mem_reply(event->evt.common_evt.conn_handle, NULL);
                    if (e) {
                        HAPLogError(
                                &logObject,
                                "sd_ble_user_mem_reply(0x%04x) failed: 0x%02x.",
                                (unsigned int) event->evt.common_evt.conn_handle,
                                (unsigned int) e);
                        HAPFatalError();
                    }
                    break;
                }
                case BLE_USER_MEM_TYPE_INVALID:
                default: {
                    HAPLogError(&logObject, "Unexpected BLE_EVT_USER_MEM_REQUEST type: 0x%02x.", ev->type);
                }
                    HAPFatalError();
            }
            break;
        }
        case BLE_EVT_USER_MEM_RELEASE: {
            break;
        }
        case BLE_GAP_EVT_CONNECTED: {
            HAPAssert(blePeripheralManager->connectionHandle == BLE_CONN_HANDLE_INVALID);
            blePeripheralManager->connectionHandle = event->evt.gap_evt.conn_handle;
            blePeripheralManager->isAdvertising = false; // advertising stops when connected

            // Update MTU state.
            //
            // Send ATT MTU Request only after we think that controller does not initiate an ATT Exchange MTU procedure.
            // This helps working around unexpected behaviour of certain controllers.
            HAPAssert(!blePeripheralManager->requestMTUUpdateTimer);
            err = HAPPlatformTimerRegister(
                    &blePeripheralManager->requestMTUUpdateTimer,
                    HAPPlatformClockGetCurrent() + kHAPPlatformBLEPeripheralManager_ATTExchangeMTURequestDelay,
                    RequestMTUUpdateTimerExpired,
                    blePeripheralManager);
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                HAPLogError(&logObject, "Not enough resources to allocate MTU update timer.");
                HAPFatalError();
            }
            blePeripheralManager->mtu = BLE_L2CAP_MTU_MIN;
            blePeripheralManager->clientMTU = 0;
            blePeripheralManager->secondClientMTU = 0;
            blePeripheralManager->pendingATTExchangeMTURequest = false;
            blePeripheralManager->pendingATTExchangeMTUResponse = false;

            uint32_t e = sd_ble_gatts_sys_attr_set(
                    event->evt.gap_evt.conn_handle,
                    /* p_sys_attr_data: */ NULL,
                    /* len: */ 0,
                    /* flags: */ 0);
            if (e) {
                HAPLogError(
                        &logObject,
                        "sd_ble_gatts_sys_attr_set(0x%04x) failed: 0x%02x.",
                        (unsigned int) event->evt.gap_evt.conn_handle,
                        (unsigned int) e);
                HAPFatalError();
            }

            if (blePeripheralManager->delegate.handleConnectedCentral) {
                blePeripheralManager->delegate.handleConnectedCentral(
                        blePeripheralManager, event->evt.gap_evt.conn_handle, blePeripheralManager->delegate.context);
            } else {
                HAPLogError(&logObject, "No connected handler plugged in.");
                HAPFatalError();
            }
            break;
        }
        case BLE_GAP_EVT_DISCONNECTED: {
            HAPAssert(blePeripheralManager->connectionHandle == event->evt.gap_evt.conn_handle);
            blePeripheralManager->connectionHandle = BLE_CONN_HANDLE_INVALID;

            if (blePeripheralManager->restartAdvertisingAfterDisconnection) {
                RestartAdvertising(blePeripheralManager);
                blePeripheralManager->restartAdvertisingAfterDisconnection = false;
            }

            // Reset MTU state.
            if (blePeripheralManager->requestMTUUpdateTimer) {
                HAPPlatformTimerDeregister(blePeripheralManager->requestMTUUpdateTimer);
                blePeripheralManager->requestMTUUpdateTimer = 0;
            }
            blePeripheralManager->mtu = 0;
            blePeripheralManager->clientMTU = 0;
            blePeripheralManager->secondClientMTU = 0;
            blePeripheralManager->pendingATTExchangeMTURequest = false;
            blePeripheralManager->pendingATTExchangeMTUResponse = false;

            HAPRawBufferZero(blePeripheralManager->bytes, sizeof blePeripheralManager->bytes);
            ResetGATTState(blePeripheralManager);
            blePeripheralManager->indicationInProgress = false;

            if (blePeripheralManager->delegate.handleDisconnectedCentral) {
                blePeripheralManager->delegate.handleDisconnectedCentral(
                        blePeripheralManager, event->evt.gap_evt.conn_handle, blePeripheralManager->delegate.context);
            } else {
                HAPLog(&logObject, "No disconnected handler plugged in. Ignoring.");
            }
            break;
        }
        case BLE_GAP_EVT_CONN_PARAM_UPDATE: {
            HAPLogDebug(
                    &logObject,
                    "Received BLE_GAP_EVT_CONN_PARAM_UPDATE with params: %u, %u, %u, %u",
                    event->evt.gap_evt.params.conn_param_update.conn_params.min_conn_interval,
                    event->evt.gap_evt.params.conn_param_update.conn_params.max_conn_interval,
                    event->evt.gap_evt.params.conn_param_update.conn_params.slave_latency,
                    event->evt.gap_evt.params.conn_param_update.conn_params.conn_sup_timeout);
            break;
        }
        case BLE_GAP_EVT_PHY_UPDATE_REQUEST: {
            HAPAssert(blePeripheralManager->connectionHandle == event->evt.gap_evt.conn_handle);

            HAPLogDebug(
                    &logObject,
                    "Received BLE_GAP_EVT_PHY_UPDATE_REQUEST with params: tx_phy: 0x%02x, rx_phy: 0x%02x",
                    event->evt.gap_evt.params.phy_update.tx_phy,
                    event->evt.gap_evt.params.phy_update.rx_phy);
            uint32_t e = sd_ble_gap_phy_update(
                    event->evt.gap_evt.conn_handle,
                    &(ble_gap_phys_t) { .rx_phys = BLE_GAP_PHY_AUTO, .tx_phys = BLE_GAP_PHY_AUTO });
            if (e) {
                HAPLogError(
                        &logObject,
                        "sd_ble_gap_phy_update(0x%04x) failed: 0x%02x.",
                        (unsigned int) event->evt.gap_evt.conn_handle,
                        (unsigned int) e);
                HAPFatalError();
            }
            break;
        }
        case BLE_GAP_EVT_PHY_UPDATE: {
            HAPLogDebug(&logObject, "Received BLE_GAP_EVT_PHY_UPDATE, phy update completed.");
            break;
        }
        case BLE_GAP_EVT_DATA_LENGTH_UPDATE_REQUEST: {
            const ble_gap_evt_data_length_update_request_t* ev = &event->evt.gap_evt.params.data_length_update_request;
            HAPAssert(blePeripheralManager->connectionHandle == event->evt.gap_evt.conn_handle);

            HAPLogDebug(
                    &logObject,
                    "Data length update request: peer_params = "
                    "<max_tx_octets = %u, max_rx_octets = %u, max_tx_time_us = %u, max_rx_time_us = %u>.",
                    ev->peer_params.max_tx_octets,
                    ev->peer_params.max_rx_octets,
                    ev->peer_params.max_tx_time_us,
                    ev->peer_params.max_rx_time_us);

            uint32_t e = sd_ble_gap_data_length_update(event->evt.gap_evt.conn_handle, NULL, NULL);
            if (e) {
                HAPLogError(
                        &logObject,
                        "sd_ble_gap_data_length_update(0x%04x) failed: 0x%02x.",
                        (unsigned int) event->evt.gap_evt.conn_handle,
                        (unsigned int) e);
                break;
            }
            break;
        }
        case BLE_GAP_EVT_DATA_LENGTH_UPDATE: {
            const ble_gap_evt_data_length_update_t* ev = &event->evt.gap_evt.params.data_length_update;
            HAPAssert(blePeripheralManager->connectionHandle == event->evt.gap_evt.conn_handle);

            HAPLogDebug(
                    &logObject,
                    "Data length update: effective_params = "
                    "<max_tx_octets = %u, max_rx_octets = %u, max_tx_time_us = %u, max_rx_time_us = %u>.",
                    ev->effective_params.max_tx_octets,
                    ev->effective_params.max_rx_octets,
                    ev->effective_params.max_tx_time_us,
                    ev->effective_params.max_rx_time_us);
            break;
        }
        case BLE_GATTC_EVT_EXCHANGE_MTU_RSP: {
            const ble_gattc_evt_exchange_mtu_rsp_t* ev = &event->evt.gattc_evt.params.exchange_mtu_rsp;
            HAPAssert(blePeripheralManager->connectionHandle == event->evt.gattc_evt.conn_handle);
            if (!blePeripheralManager->pendingATTExchangeMTUResponse) {
                HAPLogError(&logObject, "Received unexpected ATT Exchange MTU Response (%u).", ev->server_rx_mtu);
                break;
            }

            // ATT Exchange MTU Response.
            HAPLog(&logObject, "Received ATT Exchange MTU Response (%u).", ev->server_rx_mtu);
            blePeripheralManager->pendingATTExchangeMTUResponse = false;
            HandleClientMTU(blePeripheralManager, ev->server_rx_mtu);
            if (blePeripheralManager->gatt.operation == kHAPPlatformBLEPeripheralManagerGATTOperation_Disconnecting) {
                break;
            }

            // Indications were not delivered while the ATT Exchange MTU Procedure was in progress.
            if (!blePeripheralManager->indicationInProgress) {
                if (blePeripheralManager->delegate.handleReadyToUpdateSubscribers) {
                    blePeripheralManager->delegate.handleReadyToUpdateSubscribers(
                            blePeripheralManager,
                            event->evt.gatts_evt.conn_handle,
                            blePeripheralManager->delegate.context);
                }
            }
            break;
        }
        case BLE_GATTC_EVT_TIMEOUT: {
            HAPAssert(blePeripheralManager->connectionHandle == event->evt.gattc_evt.conn_handle);

            // ATT Exchange MTU Request was not acknowledged.
            HAPLogError(&logObject, "GATT Client Timeout.");
            HAPPlatformBLEPeripheralManagerCancelCentralConnection(
                    blePeripheralManager, event->evt.gattc_evt.conn_handle);
            break;
        }
        case BLE_GATTS_EVT_WRITE: {
            // All attributes that are registered through HAPPlatformBLEPeripheralManagerAddXxx APIs
            // are tagged to require authorization for writing.
            // Therefore, this event should not fire for those attributes.
            // However, other modules may define additional attributes that use this event.
            // Therefore, the event is ignored here, as it is expected by those modules to handle it.
            // To get rid of this log message, only forward BLE events related to connections and attributes
            // to this event handler which originate from a HAPPlatformBLEPeripheralManagerAddXxx API.
            HAPLog(&logObject,
                   "BLE_GATTS_EVT_WRITE (attribute handle 0x%04x). Expecting different module to handle it.",
                   event->evt.gatts_evt.params.write.handle);
            break;
        }
        case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST: {
            HAPAssert(blePeripheralManager->connectionHandle == event->evt.gatts_evt.conn_handle);

            // Work around macOS bugs.
            //
            // Problematic sequence:
            // 1. Accessory sends ATT Exchange MTU Request (NRF_SDH_BLE_GATT_MAX_MTU_SIZE).
            // 2. Controller sends ATT Exchange MTU Request (smaller value).
            // 3. Accessory sends ATT Exchange MTU Response (NRF_SDH_BLE_GATT_MAX_MTU_SIZE).
            // 4. Controller forgets to send ATT Exchange MTU Response.
            // 5. NRF_BLE_GATT_EVT_ATT_MTU_UPDATED is not reported by nrf_ble_gatt.
            // 6. After 30 seconds, nRF5 SDK reports a client timeout and terminates the link.
            // Observed on macOS 10.13.6 on Mac mini Server (Mid 2011).
            //
            // We assume that the controller only sends a Read / Write request
            // once its ATT Exchange MTU Request is assumed to have completed.
            //
            // The ATT Exchange MTU procedure is assumed to have completed to avoid having to drop the connection here.
            // Note: 30 seconds after this, BLE_GATTC_EVT_TIMEOUT will require the BLE link to be reestablished.
            if (blePeripheralManager->pendingATTExchangeMTUResponse) {
                // Assume that ATT Exchange MTU procedure is now complete.
                HAPAssert(NRF_SDH_BLE_GATT_MAX_MTU_SIZE <= sizeof blePeripheralManager->bytes);
                uint16_t mtu = HAPMin((uint16_t) NRF_SDH_BLE_GATT_MAX_MTU_SIZE, blePeripheralManager->clientMTU);
                mtu = HAPMax(mtu, BLE_L2CAP_MTU_MIN);
                HAPAssert(sizeof blePeripheralManager->bytes >= mtu);

                if (mtu != blePeripheralManager->mtu) {
                    HAPLogError(
                            &logObject,
                            "[macOS 10.13.6 Bluetooth Bugs] ATT Exchange MTU procedure incomplete. "
                            "Assuming MTU %u instead of %u (desired: %u). BLE_GATTC_EVT_TIMEOUT in ~30 seconds "
                            "imminent.",
                            mtu,
                            blePeripheralManager->mtu,
                            (uint16_t) NRF_SDH_BLE_GATT_MAX_MTU_SIZE);

                    blePeripheralManager->mtu = mtu;
                    HAPLog(&logObject, "New MTU: %u.", blePeripheralManager->mtu);
                }
            }

            HandleRWAuthorizeRequestBLEEvent(
                    blePeripheralManager,
                    event->evt.gatts_evt.conn_handle,
                    &event->evt.gatts_evt.params.authorize_request);
            break;
        }
        case BLE_GATTS_EVT_HVC: {
            HAPAssert(blePeripheralManager->connectionHandle == event->evt.gatts_evt.conn_handle);

            blePeripheralManager->indicationInProgress = false;
            if (blePeripheralManager->delegate.handleReadyToUpdateSubscribers) {
                if (!blePeripheralManager->pendingATTExchangeMTURequest &&
                    !blePeripheralManager->pendingATTExchangeMTUResponse) {
                    blePeripheralManager->delegate.handleReadyToUpdateSubscribers(
                            blePeripheralManager,
                            event->evt.gatts_evt.conn_handle,
                            blePeripheralManager->delegate.context);
                } else {
                    HAPLog(&logObject,
                           "Waiting for ATT Exchange MTU completion. Not sending ready to update subscribers.");
                }
            } else {
                HAPLog(&logObject, "No ready to update subscribers handler plugged in. Ignoring.");
            }
            break;
        }
        case BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST: {
            const ble_gatts_evt_exchange_mtu_request_t* ev = &event->evt.gatts_evt.params.exchange_mtu_request;
            HAPAssert(blePeripheralManager->connectionHandle == event->evt.gatts_evt.conn_handle);

            // ATT Exchange MTU procedure initiated. Do not send ATT Exchange MTU Request in the other direction.
            if (blePeripheralManager->requestMTUUpdateTimer) {
                HAPPlatformTimerDeregister(blePeripheralManager->requestMTUUpdateTimer);
                blePeripheralManager->requestMTUUpdateTimer = 0;
            } else {
                blePeripheralManager->pendingATTExchangeMTURequest = false;
            }

            // ATT Exchange MTU Request.
            HAPLog(&logObject, "Received ATT Exchange MTU Request (%u).", ev->client_rx_mtu);
            HandleClientMTU(blePeripheralManager, ev->client_rx_mtu);
            if (blePeripheralManager->gatt.operation == kHAPPlatformBLEPeripheralManagerGATTOperation_Disconnecting) {
                break;
            }

            HAPLog(&logObject, "Sending ATT Exchange MTU Response (%u).", NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
            uint32_t e = sd_ble_gatts_exchange_mtu_reply(
                    blePeripheralManager->connectionHandle, NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
            if (e) {
                HAPLogError(&logObject, "sd_ble_gatts_exchange_mtu_reply failed (%lu).", (unsigned long) e);
                HAPPlatformBLEPeripheralManagerCancelCentralConnection(
                        blePeripheralManager, blePeripheralManager->connectionHandle);
                break;
            }
            break;
        }
        case BLE_GATTS_EVT_TIMEOUT: {
            HAPAssert(blePeripheralManager->connectionHandle == event->evt.gatts_evt.conn_handle);

            // Handle Value Indication was not acknowledged.
            // See https://infocenter.nordicsemi.com/topic/
            //         com.nordic.infocenter.s132.api.v4.0.2/group___b_l_e___g_a_t_t_s___h_v_i___m_s_c.html
            HAPLog(&logObject, "GATT Server Timeout.");
            HAPPlatformBLEPeripheralManagerCancelCentralConnection(
                    blePeripheralManager, event->evt.gatts_evt.conn_handle);
            break;
        }
        default: {
            HAPLog(&logObject, "Did not handle BLE event!");
            break;
        }
    }

    if (blePeripheralManager->gatt.operation != kHAPPlatformBLEPeripheralManagerGATTOperation_Disconnecting &&
        blePeripheralManager->pendingATTExchangeMTURequest) {
        SendATTExchangeMTURequest(blePeripheralManager);
    }
}
