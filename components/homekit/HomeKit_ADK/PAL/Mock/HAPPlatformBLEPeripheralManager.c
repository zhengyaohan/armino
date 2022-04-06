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

#include "HAPPlatformBLEPeripheralManager+Init.h"

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem,
                                        .category = "BLEPeripheralManager(Mock)" };

void HAPPlatformBLEPeripheralManagerCreate(
        HAPPlatformBLEPeripheralManagerRef _Nonnull blePeripheralManager,
        const HAPPlatformBLEPeripheralManagerOptions* _Nonnull options) {
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(options);
    HAPPrecondition(options->attributes);
    HAPPrecondition(options->numAttributes <= SIZE_MAX / sizeof options->attributes[0]);

    HAPRawBufferZero(options->attributes, options->numAttributes * sizeof options->attributes[0]);

    HAPRawBufferZero(blePeripheralManager, sizeof *blePeripheralManager);
    blePeripheralManager->attributes = options->attributes;
    blePeripheralManager->numAttributes = options->numAttributes;
}

void HAPPlatformBLEPeripheralManagerSetDelegate(
        HAPPlatformBLEPeripheralManagerRef _Nonnull blePeripheralManager,
        const HAPPlatformBLEPeripheralManagerDelegate* _Nullable delegate) {
    HAPPrecondition(blePeripheralManager);

    if (delegate) {
        blePeripheralManager->delegate = *delegate;
    } else {
        HAPRawBufferZero(&blePeripheralManager->delegate, sizeof blePeripheralManager->delegate);
    }
}

void HAPPlatformBLEPeripheralManagerSetDeviceAddress(
        HAPPlatformBLEPeripheralManagerRef _Nonnull blePeripheralManager,
        const HAPPlatformBLEPeripheralManagerDeviceAddress* _Nonnull deviceAddress) {
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(!blePeripheralManager->isConnected);
    HAPPrecondition(deviceAddress);

    blePeripheralManager->deviceAddress = *deviceAddress;
    blePeripheralManager->isDeviceAddressSet = true;
}

void HAPPlatformBLEPeripheralManagerSetDeviceName(
        HAPPlatformBLEPeripheralManagerRef _Nonnull blePeripheralManager,
        const char* _Nonnull deviceName) {
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(deviceName);

    size_t numDeviceNameBytes = HAPStringGetNumBytes(deviceName);
    HAPPrecondition(numDeviceNameBytes < sizeof blePeripheralManager->deviceName);

    HAPRawBufferZero(blePeripheralManager->deviceName, sizeof blePeripheralManager->deviceName);
    HAPRawBufferCopyBytes(blePeripheralManager->deviceName, deviceName, numDeviceNameBytes);
}

bool HAPPlatformBLEPeripheralManagerAllowsServiceRefresh(
        HAPPlatformBLEPeripheralManagerRef _Nonnull blePeripheralManager) {
    HAPPrecondition(blePeripheralManager);
    return true;
}

void HAPPlatformBLEPeripheralManagerRemoveAllServices(
        HAPPlatformBLEPeripheralManagerRef _Nonnull blePeripheralManager) {
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(!blePeripheralManager->isConnected);

    HAPAssert(blePeripheralManager->numAttributes <= SIZE_MAX / sizeof blePeripheralManager->attributes[0]);
    HAPRawBufferZero(
            blePeripheralManager->attributes,
            blePeripheralManager->numAttributes * sizeof blePeripheralManager->attributes[0]);
    blePeripheralManager->didPublishAttributes = false;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformBLEPeripheralManagerAddService(
        HAPPlatformBLEPeripheralManagerRef _Nonnull blePeripheralManager,
        const HAPPlatformBLEPeripheralManagerUUID* _Nonnull type,
        bool isPrimary) {
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(!blePeripheralManager->didPublishAttributes);
    HAPPrecondition(type);

    bool inService = false;
    bool inCharacteristic = false;
    HAPPlatformBLEPeripheralManagerAttributeHandle handle = 0;
    for (size_t i = 0; i < blePeripheralManager->numAttributes; i++) {
        HAPPlatformBLEPeripheralManagerAttribute* attribute = &blePeripheralManager->attributes[i];

        switch (attribute->type) {
            case kHAPPlatformBLEPeripheralManagerAttributeType_None: {
                HAPPlatformBLEPeripheralManagerAttributeHandle numNeededHandles = 1;
                if (handle >= (UINT16_MAX - numNeededHandles)) {
                    HAPLog(&logObject, "Not enough resources to add GATT service (GATT database is full).");
                    return kHAPError_OutOfResources;
                }

                HAPRawBufferZero(attribute, sizeof *attribute);
                attribute->type = kHAPPlatformBLEPeripheralManagerAttributeType_Service;
                attribute->_.service.type = *type;
                attribute->_.service.isPrimary = isPrimary;
                attribute->_.service.handle = ++handle;
            }
                return kHAPError_None;
            case kHAPPlatformBLEPeripheralManagerAttributeType_Service: {
                inService = true;
                inCharacteristic = false;

                HAPAssert(attribute->_.service.handle == handle + 1);
                handle = attribute->_.service.handle;
                break;
            }
            case kHAPPlatformBLEPeripheralManagerAttributeType_Characteristic: {
                HAPAssert(inService);
                inCharacteristic = true;

                HAPAssert(attribute->_.characteristic.handle == handle + 1);
                handle = attribute->_.characteristic.handle;
                HAPAssert(attribute->_.characteristic.valueHandle == handle + 1);
                handle = attribute->_.characteristic.valueHandle;

                if (attribute->_.characteristic.cccDescriptorHandle) {
                    HAPAssert(attribute->_.characteristic.cccDescriptorHandle == handle + 1);
                    handle = attribute->_.characteristic.cccDescriptorHandle;
                }
                break;
            }
            case kHAPPlatformBLEPeripheralManagerAttributeType_Descriptor: {
                HAPAssert(inCharacteristic);

                HAPAssert(attribute->_.descriptor.handle == handle + 1);
                handle = attribute->_.descriptor.handle;
                break;
            }
        }
    }
    HAPLog(&logObject,
           "Not enough resources to add GATT service (have space for %zu GATT attributes).",
           blePeripheralManager->numAttributes);
    return kHAPError_OutOfResources;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformBLEPeripheralManagerAddCharacteristic(
        HAPPlatformBLEPeripheralManagerRef _Nonnull blePeripheralManager,
        const HAPPlatformBLEPeripheralManagerUUID* _Nonnull type,
        HAPPlatformBLEPeripheralManagerCharacteristicProperties properties,
        const void* _Nullable constBytes HAP_UNUSED,
        size_t constNumBytes HAP_UNUSED,
        HAPPlatformBLEPeripheralManagerAttributeHandle* _Nonnull valueHandle,
        HAPPlatformBLEPeripheralManagerAttributeHandle* _Nullable cccDescriptorHandle) {
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(!blePeripheralManager->didPublishAttributes);
    HAPPrecondition(type);
    HAPPrecondition(valueHandle);
    if (properties.notify || properties.indicate) {
        HAPPrecondition(cccDescriptorHandle);
    } else {
        HAPPrecondition(!cccDescriptorHandle);
    }

    bool inService = false;
    bool inCharacteristic = false;
    HAPPlatformBLEPeripheralManagerAttributeHandle handle = 0;
    for (size_t i = 0; i < blePeripheralManager->numAttributes; i++) {
        HAPPlatformBLEPeripheralManagerAttribute* attribute = &blePeripheralManager->attributes[i];

        switch (attribute->type) {
            case kHAPPlatformBLEPeripheralManagerAttributeType_None: {
                HAPPrecondition(inService);

                HAPPlatformBLEPeripheralManagerAttributeHandle numNeededHandles = 2;
                if (properties.indicate || properties.notify) {
                    numNeededHandles++;
                }
                if (handle >= (UINT16_MAX - numNeededHandles)) {
                    HAPLog(&logObject, "Not enough resources to add GATT characteristic (GATT database is full).");
                    return kHAPError_OutOfResources;
                }

                HAPRawBufferZero(attribute, sizeof *attribute);
                attribute->type = kHAPPlatformBLEPeripheralManagerAttributeType_Characteristic;
                attribute->_.characteristic.type = *type;
                attribute->_.characteristic.properties = properties;
                attribute->_.characteristic.handle = ++handle;
                attribute->_.characteristic.valueHandle = ++handle;
                if (properties.indicate || properties.notify) {
                    attribute->_.characteristic.cccDescriptorHandle = ++handle;
                }

                *valueHandle = attribute->_.characteristic.valueHandle;
                if (cccDescriptorHandle) {
                    *cccDescriptorHandle = attribute->_.characteristic.cccDescriptorHandle;
                }
            }
                return kHAPError_None;
            case kHAPPlatformBLEPeripheralManagerAttributeType_Service: {
                inService = true;
                inCharacteristic = false;

                HAPAssert(attribute->_.service.handle == handle + 1);
                handle = attribute->_.service.handle;
                break;
            }
            case kHAPPlatformBLEPeripheralManagerAttributeType_Characteristic: {
                HAPAssert(inService);
                inCharacteristic = true;

                HAPAssert(attribute->_.characteristic.handle == handle + 1);
                handle = attribute->_.characteristic.handle;
                HAPAssert(attribute->_.characteristic.valueHandle == handle + 1);
                handle = attribute->_.characteristic.valueHandle;

                if (attribute->_.characteristic.cccDescriptorHandle) {
                    HAPAssert(attribute->_.characteristic.cccDescriptorHandle == handle + 1);
                    handle = attribute->_.characteristic.cccDescriptorHandle;
                }
                break;
            }
            case kHAPPlatformBLEPeripheralManagerAttributeType_Descriptor: {
                HAPAssert(inCharacteristic);

                HAPAssert(attribute->_.descriptor.handle == handle + 1);
                handle = attribute->_.descriptor.handle;
                break;
            }
        }
    }
    HAPLog(&logObject,
           "Not enough resources to add GATT characteristic (have space for %zu GATT attributes).",
           blePeripheralManager->numAttributes);
    return kHAPError_OutOfResources;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformBLEPeripheralManagerAddDescriptor(
        HAPPlatformBLEPeripheralManagerRef _Nonnull blePeripheralManager,
        const HAPPlatformBLEPeripheralManagerUUID* _Nonnull type,
        HAPPlatformBLEPeripheralManagerDescriptorProperties properties,
        const void* _Nullable constBytes HAP_UNUSED,
        size_t constNumBytes HAP_UNUSED,
        HAPPlatformBLEPeripheralManagerAttributeHandle* _Nonnull descriptorHandle) {
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(!blePeripheralManager->didPublishAttributes);
    HAPPrecondition(type);
    HAPPrecondition(descriptorHandle);

    bool inService = false;
    bool inCharacteristic = false;
    HAPPlatformBLEPeripheralManagerAttributeHandle handle = 0;
    for (size_t i = 0; i < blePeripheralManager->numAttributes; i++) {
        HAPPlatformBLEPeripheralManagerAttribute* attribute = &blePeripheralManager->attributes[i];

        switch (attribute->type) {
            case kHAPPlatformBLEPeripheralManagerAttributeType_None: {
                HAPPrecondition(inCharacteristic);

                HAPPlatformBLEPeripheralManagerAttributeHandle numNeededHandles = 1;
                if (handle >= (UINT16_MAX - numNeededHandles)) {
                    HAPLog(&logObject, "Not enough resources to add GATT descriptor (GATT database is full).");
                    return kHAPError_OutOfResources;
                }

                HAPRawBufferZero(attribute, sizeof *attribute);
                attribute->type = kHAPPlatformBLEPeripheralManagerAttributeType_Descriptor;
                attribute->_.descriptor.type = *type;
                attribute->_.descriptor.properties = properties;
                attribute->_.descriptor.handle = ++handle;

                *descriptorHandle = attribute->_.descriptor.handle;
            }
                return kHAPError_None;
            case kHAPPlatformBLEPeripheralManagerAttributeType_Service: {
                inService = true;
                inCharacteristic = false;

                HAPAssert(attribute->_.service.handle == handle + 1);
                handle = attribute->_.service.handle;
                break;
            }
            case kHAPPlatformBLEPeripheralManagerAttributeType_Characteristic: {
                HAPAssert(inService);
                inCharacteristic = true;

                HAPAssert(attribute->_.characteristic.handle == handle + 1);
                handle = attribute->_.characteristic.handle;
                HAPAssert(attribute->_.characteristic.valueHandle == handle + 1);
                handle = attribute->_.characteristic.valueHandle;

                if (attribute->_.characteristic.cccDescriptorHandle) {
                    HAPAssert(attribute->_.characteristic.cccDescriptorHandle == handle + 1);
                    handle = attribute->_.characteristic.cccDescriptorHandle;
                }
                break;
            }
            case kHAPPlatformBLEPeripheralManagerAttributeType_Descriptor: {
                HAPAssert(inCharacteristic);

                HAPAssert(attribute->_.descriptor.handle == handle + 1);
                handle = attribute->_.descriptor.handle;
                break;
            }
        }
    }
    HAPLog(&logObject,
           "Not enough resources to add GATT descriptor (have space for %zu GATT attributes).",
           blePeripheralManager->numAttributes);
    return kHAPError_OutOfResources;
}

void HAPPlatformBLEPeripheralManagerPublishServices(HAPPlatformBLEPeripheralManagerRef _Nonnull blePeripheralManager) {
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(blePeripheralManager->isDeviceAddressSet);
    HAPPrecondition(!blePeripheralManager->didPublishAttributes);

    blePeripheralManager->didPublishAttributes = true;
}

void HAPPlatformBLEPeripheralManagerStartAdvertising(
        HAPPlatformBLEPeripheralManagerRef _Nonnull blePeripheralManager,
        HAPBLEAdvertisingInterval advertisingInterval,
        const void* _Nonnull advertisingBytes,
        size_t numAdvertisingBytes,
        const void* _Nullable scanResponseBytes,
        size_t numScanResponseBytes) {
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(blePeripheralManager->isDeviceAddressSet);
    HAPPrecondition(blePeripheralManager->didPublishAttributes);
    HAPPrecondition(advertisingInterval);
    HAPPrecondition(advertisingBytes);
    HAPPrecondition(numAdvertisingBytes);
    HAPPrecondition(numAdvertisingBytes <= sizeof blePeripheralManager->advertisingBytes);
    HAPPrecondition(!numScanResponseBytes || scanResponseBytes);
    HAPPrecondition(numScanResponseBytes <= sizeof blePeripheralManager->scanResponseBytes);

    HAPPlatformBLEPeripheralManagerStopAdvertising(blePeripheralManager);

    HAPRawBufferCopyBytes(blePeripheralManager->advertisingBytes, advertisingBytes, numAdvertisingBytes);
    blePeripheralManager->numAdvertisingBytes = (uint8_t) numAdvertisingBytes;
    if (scanResponseBytes) {
        HAPRawBufferCopyBytes(
                blePeripheralManager->scanResponseBytes, HAPNonnullVoid(scanResponseBytes), numScanResponseBytes);
    }
    blePeripheralManager->numScanResponseBytes = (uint8_t) numScanResponseBytes;
    blePeripheralManager->advertisingInterval = advertisingInterval;
}

void HAPPlatformBLEPeripheralManagerStopAdvertising(HAPPlatformBLEPeripheralManagerRef _Nonnull blePeripheralManager) {
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(blePeripheralManager->isDeviceAddressSet);
    HAPPrecondition(blePeripheralManager->didPublishAttributes);

    HAPRawBufferZero(blePeripheralManager->advertisingBytes, sizeof blePeripheralManager->advertisingBytes);
    blePeripheralManager->numAdvertisingBytes = 0;
    HAPRawBufferZero(blePeripheralManager->scanResponseBytes, sizeof blePeripheralManager->scanResponseBytes);
    blePeripheralManager->numScanResponseBytes = 0;
    blePeripheralManager->advertisingInterval = 0;
}

void HAPPlatformBLEPeripheralManagerCancelCentralConnection(
        HAPPlatformBLEPeripheralManagerRef _Nonnull blePeripheralManager,
        HAPPlatformBLEPeripheralManagerConnectionHandle connectionHandle) {
    HAPPrecondition(blePeripheralManager);

    (void) connectionHandle;
    HAPLogError(&logObject, "[NYI] %s.", __func__);
    HAPFatalError();
}

HAPError HAPPlatformBLEPeripheralManagerSendHandleValueIndication(
        HAPPlatformBLEPeripheralManagerRef _Nonnull blePeripheralManager,
        HAPPlatformBLEPeripheralManagerConnectionHandle connectionHandle,
        HAPPlatformBLEPeripheralManagerAttributeHandle valueHandle,
        const void* _Nullable bytes,
        size_t numBytes) HAP_DIAGNOSE_ERROR(!bytes && numBytes, "empty buffer cannot have a length") {
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(valueHandle);
    HAPPrecondition(!numBytes || bytes);

    (void) connectionHandle;
    HAPLogError(&logObject, "[NYI] %s.", __func__);
    HAPFatalError();
}
