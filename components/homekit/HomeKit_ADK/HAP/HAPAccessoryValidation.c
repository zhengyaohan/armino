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

#include "HAPAccessoryValidation.h"

#include "HAPAccessoryServer+Internal.h"
#include "HAPCharacteristic.h"
#include "HAPLog+Attributes.h"
#include "HAPLogSubsystem.h"
#include "HAPServiceTypes.h"
#include "HAPUUID.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "AccessoryValidation" };

/**
 * Length of an accessory's product data string.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.146 Product Data
 */
#define kHAPAccessory_NumProductDataStringBytes ((size_t)(2 * 8))

/**
 * Maximum length of an accessory's manufacturer.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.15 Manufacturer
 */
#define kHAPAccessory_MaxManufacturerBytes ((size_t) 64)

/**
 * Minimum length of an accessory's model name.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.16 Model
 */
#define kHAPAccessory_MinModelBytes ((size_t) 1)

/**
 * Maximum length of an accessory's model name.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.16 Model
 */
#define kHAPAccessory_MaxModelBytes ((size_t) 64)

/**
 * Minimum length of an accessory's serial number.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.25 Serial Number
 */
#define kHAPAccessory_MinSerialNumberBytes ((size_t) 2)

/**
 * Maximum length of an accessory's serial number.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.25 Serial Number
 */
#define kHAPAccessory_MaxSerialNumberBytes ((size_t) 64)

/**
 * Validates generic rules of an accessory definition.
 *
 * @param      accessory            Accessory to validate.
 *
 * @return true                     If the accessory definition is valid.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
static bool AccessoryIsValid( // NOLINT(readability-function-size)
        const HAPAccessory* accessory) {
    HAPPrecondition(accessory);

    // Validate accessory information.
    if (!accessory->name) {
        HAPLogError(&logObject, "Accessory 0x%016llX %s is not set.", (unsigned long long) accessory->aid, "name");
        return false;
    }
    size_t numNameBytes = HAPStringGetNumBytes(accessory->name);
    if (numNameBytes > kHAPAccessory_MaxNameBytes) {
        HAPLogAccessoryError(
                &logObject,
                accessory,
                "Accessory %s %s has invalid length (%zu) - expected: max %zu.",
                "name",
                accessory->name,
                numNameBytes,
                kHAPAccessory_MaxNameBytes);
        return false;
    }
    if (!HAPUTF8IsValidData(accessory->name, numNameBytes)) {
        HAPLogAccessoryError(
                &logObject, accessory, "Accessory %s %s is not a valid UTF-8 string.", "name", accessory->name);
        return false;
    }

    if (accessory->productData) {
        size_t numProductDataBytes = HAPStringGetNumBytes(HAPNonnull(accessory->productData));
        if (numProductDataBytes != kHAPAccessory_NumProductDataStringBytes) {
            HAPLogAccessoryError(
                    &logObject,
                    accessory,
                    "Accessory %s %s has invalid length (%zu) - expected: %zu.",
                    "productData",
                    accessory->productData,
                    numProductDataBytes,
                    kHAPAccessory_NumProductDataStringBytes);
            return false;
        }
        if (!HAPUTF8IsValidData(HAPNonnull(accessory->productData), numProductDataBytes)) {
            HAPLogAccessoryError(
                    &logObject,
                    accessory,
                    "Accessory %s %s is not a valid UTF-8 string.",
                    "productData",
                    HAPNonnull(accessory->productData));
            return false;
        }
        for (size_t i = 0; i < numProductDataBytes; i++) {
            char c = accessory->productData[i];
            if (!HAPASCIICharacterIsLowercaseHexDigit(c)) {
                HAPLogAccessoryError(
                        &logObject,
                        accessory,
                        "Accessory %s %s is not a valid lowercase hex string.",
                        "productData",
                        HAPNonnull(accessory->productData));
                return false;
            }
        }
    }

    if (!accessory->manufacturer) {
        HAPLogError(
                &logObject, "Accessory 0x%016llX %s is not set.", (unsigned long long) accessory->aid, "manufacturer");
        return false;
    }
    size_t numManufacturerBytes = HAPStringGetNumBytes(accessory->manufacturer);
    if (numManufacturerBytes > kHAPAccessory_MaxManufacturerBytes) {
        HAPLogAccessoryError(
                &logObject,
                accessory,
                "Accessory %s %s has invalid length (%zu) - expected: max %zu.",
                "manufacturer",
                accessory->manufacturer,
                numManufacturerBytes,
                kHAPAccessory_MaxManufacturerBytes);
        return false;
    }
    if (!HAPUTF8IsValidData(accessory->manufacturer, numManufacturerBytes)) {
        HAPLogAccessoryError(
                &logObject,
                accessory,
                "Accessory %s %s is not a valid UTF-8 string.",
                "manufacturer",
                accessory->manufacturer);
        return false;
    }

    if (!accessory->model) {
        HAPLogError(&logObject, "Accessory 0x%016llX %s is not set.", (unsigned long long) accessory->aid, "model");
        return false;
    }
    size_t numModelBytes = HAPStringGetNumBytes(accessory->model);
    if (numModelBytes < kHAPAccessory_MinModelBytes || numModelBytes > kHAPAccessory_MaxModelBytes) {
        HAPLogAccessoryError(
                &logObject,
                accessory,
                "Accessory %s %s has invalid length (%zu) - expected: min %zu, max %zu.",
                "model",
                accessory->model,
                numModelBytes,
                kHAPAccessory_MinModelBytes,
                kHAPAccessory_MaxModelBytes);
        return false;
    }
    if (!HAPUTF8IsValidData(accessory->model, numModelBytes)) {
        HAPLogAccessoryError(
                &logObject, accessory, "Accessory %s %s is not a valid UTF-8 string.", "model", accessory->model);
        return false;
    }

    if (!accessory->serialNumber) {
        HAPLogError(
                &logObject, "Accessory 0x%016llX %s is not set.", (unsigned long long) accessory->aid, "serialNumber");
        return false;
    }
    size_t numSerialNumberBytes = HAPStringGetNumBytes(accessory->serialNumber);
    if (numSerialNumberBytes < kHAPAccessory_MinSerialNumberBytes ||
        numSerialNumberBytes > kHAPAccessory_MaxSerialNumberBytes) {
        HAPLogAccessoryError(
                &logObject,
                accessory,
                "Accessory %s %s has invalid length (%zu) - expected: min %zu, max %zu.",
                "serial number",
                accessory->serialNumber,
                numSerialNumberBytes,
                kHAPAccessory_MinSerialNumberBytes,
                kHAPAccessory_MaxSerialNumberBytes);
        return false;
    }
    if (!HAPUTF8IsValidData(accessory->serialNumber, numSerialNumberBytes)) {
        HAPLogAccessoryError(
                &logObject,
                accessory,
                "Accessory %s %s is not a valid UTF-8 string.",
                "serialNumber",
                accessory->serialNumber);
        return false;
    }

    if (!accessory->firmwareVersion) {
        HAPLogError(
                &logObject,
                "Accessory 0x%016llX %s is not set.",
                (unsigned long long) accessory->aid,
                "firmwareVersion");
        return false;
    }
    size_t numFirmwareVersionBytes = HAPStringGetNumBytes(accessory->firmwareVersion);
    if (!HAPUTF8IsValidData(accessory->firmwareVersion, numFirmwareVersionBytes)) {
        HAPLogAccessoryError(
                &logObject,
                accessory,
                "Accessory %s %s is not a valid UTF-8 string.",
                "firmwareVersion",
                accessory->firmwareVersion);
        return false;
    }
    if (accessory->hardwareVersion) {
        size_t numHardwareVersionBytes = HAPStringGetNumBytes(HAPNonnull(accessory->hardwareVersion));
        if (!HAPUTF8IsValidData(HAPNonnull(accessory->hardwareVersion), numHardwareVersionBytes)) {
            HAPLogAccessoryError(
                    &logObject,
                    accessory,
                    "Accessory %s %s is not a valid UTF-8 string.",
                    "hardwareVersion",
                    HAPNonnull(accessory->hardwareVersion));
            return false;
        }
    }

    bool hasPrimaryService = false;
    if (!accessory->services) {
        HAPLogAccessoryError(
                &logObject, accessory, "Accessory must at least contain the Accessory Information Service.");
        return false;
    }
    for (size_t i = 0; accessory->services[i]; i++) {
        const HAPService* service = accessory->services[i];

        if (!service->debugDescription) {
            HAPLogAccessoryError(
                    &logObject,
                    accessory,
                    "Service 0x%016llX debugDescription is not set.",
                    (unsigned long long) service->iid);
            return false;
        }
        size_t numServiceDebugDescriptionBytes = HAPStringGetNumBytes(service->debugDescription);
        if (!HAPUTF8IsValidData(service->debugDescription, numServiceDebugDescriptionBytes)) {
            HAPLogServiceError(
                    &logObject,
                    service,
                    accessory,
                    "Service debugDescription %s is not a valid UTF-8 string.",
                    service->debugDescription);
            return false;
        }

        if (service->name) {
            size_t numServiceNameBytes = HAPStringGetNumBytes(HAPNonnull(service->name));
            if (!HAPUTF8IsValidData(HAPNonnull(service->name), numServiceNameBytes)) {
                HAPLogServiceError(
                        &logObject, service, accessory, "Service name %s is not a valid UTF-8 string.", service->name);
                return false;
            }
        }

        if (service->properties.primaryService) {
            if (hasPrimaryService) {
                // See HomeKit Accessory Protocol Specification R17
                // Section 2.3.2.3 Primary Service
                HAPLogServiceError(
                        &logObject,
                        service,
                        accessory,
                        "An accessory must advertise only one service as its primary service.");
                return false;
            }
            if (!HAPUUIDIsAppleDefined(service->serviceType)) {
                // See HomeKit Accessory Protocol Specification R17
                // Section 2.3.2.3 Primary Service
                HAPLogServiceError(
                        &logObject, service, accessory, "The primary service must be an Apple-defined service.");
                return false;
            }
            hasPrimaryService = true;
        }

        if (service->linkedServices) {
            for (size_t j = 0; service->linkedServices[j]; j++) {
                uint16_t linkedService = service->linkedServices[j];

                for (size_t k = 0; k < j; k++) {
                    if (linkedService == service->linkedServices[k]) {
                        HAPLogServiceError(
                                &logObject,
                                service,
                                accessory,
                                "linkedServices entry 0x%016llX specified multiple times.",
                                (unsigned long long) service->linkedServices[k]);
                        return false;
                    }
                }

                bool found = false;
                for (size_t k = 0; accessory->services[k]; k++) {
                    const HAPService* otherService = accessory->services[k];

                    if (otherService->iid == linkedService) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    HAPLogServiceError(
                            &logObject,
                            service,
                            accessory,
                            "linkedServices entry 0x%016llX does not correspond to a specified service.",
                            (unsigned long long) linkedService);
                    return false;
                }
            }
        }
#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)
        if (HAPUUIDAreEqual(service->serviceType, &kHAPServiceType_CameraEventRecordingManagement)) {
            if (!accessory->callbacks.camera.getSupportedRecordingConfiguration) {
                HAPLogServiceError(
                        &logObject,
                        service,
                        accessory,
                        "No camera.getSupportedRecordingConfiguration callback specified.");
                return false;
            }
        }
#endif

        bool allCharacteristicsHidden = true;
        if (service->characteristics) {
            for (size_t j = 0; service->characteristics[j]; j++) {
                const HAPBaseCharacteristic* characteristic = service->characteristics[j];

                // Validate characteristic.
                if (!characteristic->debugDescription) {
                    HAPLogServiceError(
                            &logObject,
                            service,
                            accessory,
                            "Characteristic 0x%016llX debugDescription is not set.",
                            (unsigned long long) characteristic->iid);
                    return false;
                }
                size_t numCharacteristicDebugDescriptionBytes = HAPStringGetNumBytes(characteristic->debugDescription);
                if (!HAPUTF8IsValidData(characteristic->debugDescription, numCharacteristicDebugDescriptionBytes)) {
                    HAPLogCharacteristicError(
                            &logObject,
                            characteristic,
                            service,
                            accessory,
                            "Characteristic debugDescription %s is not a valid UTF-8 string.",
                            characteristic->debugDescription);
                    return false;
                }

                if (characteristic->manufacturerDescription) {
                    size_t numManufacturerDescriptionBytes =
                            HAPStringGetNumBytes(HAPNonnull(characteristic->manufacturerDescription));
                    if (!HAPUTF8IsValidData(
                                HAPNonnull(characteristic->manufacturerDescription), numManufacturerDescriptionBytes)) {
                        HAPLogCharacteristicError(
                                &logObject,
                                characteristic,
                                service,
                                accessory,
                                "Characteristic manufacturerDescription %s is not a valid UTF-8 string.",
                                characteristic->manufacturerDescription);
                        return false;
                    }
                }

                allCharacteristicsHidden &= characteristic->properties.hidden;

#define BASE_CHARACTERISTIC_CHECKS(chr) \
    do { \
        /* readable. */ \
        if ((chr)->properties.readable && !(chr)->callbacks.handleRead) { \
            HAPLogCharacteristicError( \
                    &logObject, \
                    characteristic, \
                    service, \
                    accessory, \
                    "Characteristic marked as readable but no handleRead callback set."); \
            return false; \
        } \
\
        /* writable. */ \
        if ((chr)->properties.writable && !(chr)->callbacks.handleWrite) { \
            HAPLogCharacteristicError( \
                    &logObject, \
                    characteristic, \
                    service, \
                    accessory, \
                    "Characteristic marked as writable but no handleWrite callback set."); \
            return false; \
        } \
\
        /* supportsEventNotification. */ \
        if ((chr)->properties.supportsEventNotification && !(chr)->callbacks.handleRead) { \
            HAPLogCharacteristicError( \
                    &logObject, \
                    characteristic, \
                    service, \
                    accessory, \
                    "Characteristic marked as supportsEventNotification but no handleRead callback set."); \
            return false; \
        } \
\
        /* readRequiresAdminPermissions. */ \
        if ((chr)->properties.readRequiresAdminPermissions && !(chr)->properties.readable) { \
            HAPLogCharacteristicError( \
                    &logObject, \
                    characteristic, \
                    service, \
                    accessory, \
                    "Characteristic marked as readRequiresAdminPermissions but not as readable."); \
            return false; \
        } \
\
        /* writeRequiresAdminPermissions. */ \
        if ((chr)->properties.writeRequiresAdminPermissions && !(chr)->properties.writable) { \
            HAPLogCharacteristicError( \
                    &logObject, \
                    characteristic, \
                    service, \
                    accessory, \
                    "Characteristic marked as writeRequiresAdminPermissions but not as writable."); \
            return false; \
        } \
\
        /* readRequiresAdminPermissions, writeRequiresAdminPermissions. */ \
        if (HAPCharacteristicReadRequiresAdminPermissions(chr) && (chr)->properties.writable && \
            !HAPCharacteristicWriteRequiresAdminPermissions(chr)) { \
            HAPLogCharacteristicError( \
                    &logObject, \
                    characteristic, \
                    service, \
                    accessory, \
                    "Characteristic marked as readRequiresAdminPermissions and writable " \
                    "but not as writeRequiresAdminPermissions."); \
            return false; \
        } \
\
        /* requiresTimedWrite. */ \
        if ((chr)->properties.requiresTimedWrite && !(chr)->properties.writable) { \
            HAPLogCharacteristicError( \
                    &logObject, \
                    characteristic, \
                    service, \
                    accessory, \
                    "Characteristic marked as requiresTimedWrite but not as writable."); \
            return false; \
        } \
\
        /* supportsAuthorizationData. */ \
        if ((chr)->properties.supportsAuthorizationData && !(chr)->properties.writable) { \
            HAPLogCharacteristicError( \
                    &logObject, \
                    characteristic, \
                    service, \
                    accessory, \
                    "Characteristic marked as supportsAuthorizationData but not as writable."); \
            return false; \
        } \
\
        /* ip.supportsWriteResponse. */ \
        if ((chr)->properties.ip.supportsWriteResponse && !(chr)->properties.writable) { \
            HAPLogCharacteristicError( \
                    &logObject, \
                    characteristic, \
                    service, \
                    accessory, \
                    "Characteristic marked as ip.supportsWriteResponse but not as writable."); \
            return false; \
        } \
        if ((chr)->properties.ip.supportsWriteResponse && !(chr)->callbacks.handleRead) { \
            HAPLogCharacteristicError( \
                    &logObject, \
                    characteristic, \
                    service, \
                    accessory, \
                    "Characteristic marked as ip.supportsWriteResponse but no handleRead callback set."); \
            return false; \
        } \
        if ((chr)->properties.ip.supportsWriteResponse && !(chr)->callbacks.handleWrite) { \
            HAPLogCharacteristicError( \
                    &logObject, \
                    characteristic, \
                    service, \
                    accessory, \
                    "Characteristic marked as ip.supportsWriteResponse but no handleWrite callback set."); \
            return false; \
        } \
\
        /* ble.supportsBroadcastNotification */ \
        if ((chr)->properties.ble.supportsBroadcastNotification && !(chr)->callbacks.handleRead) { \
            HAPLogCharacteristicError( \
                    &logObject, \
                    characteristic, \
                    service, \
                    accessory, \
                    "Characteristic marked as ble.supportsBroadcastNotification " \
                    "but no handleRead callback set."); \
            return false; \
        } \
        if ((chr)->properties.ble.supportsBroadcastNotification && \
            !(chr)->properties.ble.supportsDisconnectedNotification) { \
            HAPLogCharacteristicError( \
                    &logObject, \
                    characteristic, \
                    service, \
                    accessory, \
                    "Characteristic marked as ble.supportsBroadcastNotification " \
                    "but not as ble.supportsDisconnectedNotification."); \
            return false; \
        } \
\
        /* ble.supportsDisconnectedNotification */ \
        if ((chr)->properties.ble.supportsDisconnectedNotification && !(chr)->properties.readable) { \
            HAPLogCharacteristicError( \
                    &logObject, \
                    characteristic, \
                    service, \
                    accessory, \
                    "Characteristic marked as ble.supportsDisconnectedNotification " \
                    "but not as readable."); \
            return false; \
        } \
        if ((chr)->properties.ble.supportsDisconnectedNotification && !(chr)->properties.supportsEventNotification) { \
            HAPLogCharacteristicError( \
                    &logObject, \
                    characteristic, \
                    service, \
                    accessory, \
                    "Characteristic marked as ble.supportsDisconnectedNotification " \
                    "but not as supportsEventNotification."); \
            return false; \
        } \
        if ((chr)->properties.ble.supportsDisconnectedNotification && !(chr)->callbacks.handleRead) { \
            HAPLogCharacteristicError( \
                    &logObject, \
                    characteristic, \
                    service, \
                    accessory, \
                    "Characteristic marked as ble.supportsDisconnectedNotification " \
                    "but no handleRead callback set."); \
            return false; \
        } \
\
        /* ble.readableWithoutSecurity. */ \
        if ((chr)->properties.ble.readableWithoutSecurity && !(chr)->callbacks.handleRead) { \
            HAPLogCharacteristicError( \
                    &logObject, \
                    characteristic, \
                    service, \
                    accessory, \
                    "Characteristic marked as ble.readableWithoutSecurity " \
                    "but no handleRead callback set."); \
            return false; \
        } \
\
        /* ble.writableWithoutSecurity. */ \
        if ((chr)->properties.ble.writableWithoutSecurity && !(chr)->callbacks.handleWrite) { \
            HAPLogCharacteristicError( \
                    &logObject, \
                    characteristic, \
                    service, \
                    accessory, \
                    "Characteristic marked as ble.writableWithoutSecurity " \
                    "but no handleWrite callback set."); \
            return false; \
        } \
    } while (0)
                switch (characteristic->format) {
                    case kHAPCharacteristicFormat_Data: {
                        const HAPDataCharacteristic* chr = service->characteristics[j];
                        BASE_CHARACTERISTIC_CHECKS(chr);
                        break;
                    }
                    case kHAPCharacteristicFormat_Bool: {
                        const HAPBoolCharacteristic* chr = service->characteristics[j];
                        BASE_CHARACTERISTIC_CHECKS(chr);
                        break;
                    }
                    case kHAPCharacteristicFormat_UInt8: {
                        const HAPUInt8Characteristic* chr = service->characteristics[j];
                        BASE_CHARACTERISTIC_CHECKS(chr);

                        if (chr->constraints.minimumValue > chr->constraints.maximumValue) {
                            HAPLogCharacteristicError(
                                    &logObject,
                                    characteristic,
                                    service,
                                    accessory,
                                    "Characteristic constraints invalid "
                                    "(constraints: minimumValue = %u / maximumValue = %u / stepValue = %u).",
                                    chr->constraints.minimumValue,
                                    chr->constraints.maximumValue,
                                    chr->constraints.stepValue);
                            return false;
                        }

                        const uint8_t* _Nullable const* _Nullable validValues = chr->constraints.validValues;
                        const HAPUInt8CharacteristicValidValuesRange* _Nullable const* _Nullable validValuesRanges =
                                chr->constraints.validValuesRanges;
                        if (validValues || validValuesRanges) {
                            if (!HAPUUIDIsAppleDefined(chr->characteristicType)) {
                                HAPLogCharacteristicError(
                                        &logObject,
                                        characteristic,
                                        service,
                                        accessory,
                                        "Only Apple-defined characteristics can specify "
                                        "validValues and validValuesRanges constraints.");
                                return false;
                            }
                            if (validValues) {
                                for (size_t k = 0; validValues[k]; k++) {
                                    if (k) {
                                        if (*validValues[k] <= *validValues[k - 1]) {
                                            HAPLogCharacteristicError(
                                                    &logObject,
                                                    characteristic,
                                                    service,
                                                    accessory,
                                                    "Characteristic validValues must be sorted in ascending order "
                                                    "(%u is listed before %u).",
                                                    *validValues[k - 1],
                                                    *validValues[k]);
                                            return false;
                                        }
                                    }
                                }
                            }
                            if (validValuesRanges) {
                                for (size_t k = 0; validValuesRanges[k]; k++) {
                                    if (validValuesRanges[k]->start > validValuesRanges[k]->end) {
                                        HAPLogCharacteristicError(
                                                &logObject,
                                                characteristic,
                                                service,
                                                accessory,
                                                "Characteristic validValuesRanges invalid ([%u ... %u]).",
                                                validValuesRanges[k]->start,
                                                validValuesRanges[k]->end);
                                        return false;
                                    }
                                    if (k) {
                                        if (validValuesRanges[k]->start < validValuesRanges[k - 1]->end) {
                                            HAPLogCharacteristicError(
                                                    &logObject,
                                                    characteristic,
                                                    service,
                                                    accessory,
                                                    "Characteristic validValuesRanges must be sorted in ascending "
                                                    "order "
                                                    "([%u ... %u] is listed before [%u ... %u]).",
                                                    validValuesRanges[k - 1]->start,
                                                    validValuesRanges[k - 1]->end,
                                                    validValuesRanges[k]->start,
                                                    validValuesRanges[k]->end);
                                            return false;
                                        }
                                    }
                                }
                            }
                        }
                        break;
                    }
                    case kHAPCharacteristicFormat_UInt16: {
                        const HAPUInt16Characteristic* chr = service->characteristics[j];
                        BASE_CHARACTERISTIC_CHECKS(chr);

                        if (chr->constraints.minimumValue > chr->constraints.maximumValue) {
                            HAPLogCharacteristicError(
                                    &logObject,
                                    characteristic,
                                    service,
                                    accessory,
                                    "Characteristic constraints invalid "
                                    "(constraints: minimumValue = %u / maximumValue = %u / stepValue = %u).",
                                    chr->constraints.minimumValue,
                                    chr->constraints.maximumValue,
                                    chr->constraints.stepValue);
                            return false;
                        }
                        break;
                    }
                    case kHAPCharacteristicFormat_UInt32: {
                        const HAPUInt32Characteristic* chr = service->characteristics[j];
                        BASE_CHARACTERISTIC_CHECKS(chr);

                        if (chr->constraints.minimumValue > chr->constraints.maximumValue) {
                            HAPLogCharacteristicError(
                                    &logObject,
                                    characteristic,
                                    service,
                                    accessory,
                                    "Characteristic constraints invalid "
                                    "(constraints: minimumValue = %lu / maximumValue = %lu / stepValue = %lu).",
                                    (unsigned long) chr->constraints.minimumValue,
                                    (unsigned long) chr->constraints.maximumValue,
                                    (unsigned long) chr->constraints.stepValue);
                            return false;
                        }
                        break;
                    }
                    case kHAPCharacteristicFormat_UInt64: {
                        const HAPUInt64Characteristic* chr = service->characteristics[j];
                        BASE_CHARACTERISTIC_CHECKS(chr);

                        if (chr->constraints.minimumValue > chr->constraints.maximumValue) {
                            HAPLogCharacteristicError(
                                    &logObject,
                                    characteristic,
                                    service,
                                    accessory,
                                    "Characteristic constraints invalid "
                                    "(constraints: minimumValue = %llu / maximumValue = %llu / stepValue = %llu).",
                                    (unsigned long long) chr->constraints.minimumValue,
                                    (unsigned long long) chr->constraints.maximumValue,
                                    (unsigned long long) chr->constraints.stepValue);
                            return false;
                        }
                        break;
                    }
                    case kHAPCharacteristicFormat_Int: {
                        const HAPIntCharacteristic* chr = service->characteristics[j];
                        BASE_CHARACTERISTIC_CHECKS(chr);

                        if (chr->constraints.minimumValue > chr->constraints.maximumValue ||
                            chr->constraints.stepValue < 0) {
                            HAPLogCharacteristicError(
                                    &logObject,
                                    characteristic,
                                    service,
                                    accessory,
                                    "Characteristic constraints invalid "
                                    "(constraints: minimumValue = %ld / maximumValue = %ld / stepValue = %ld).",
                                    (long) chr->constraints.minimumValue,
                                    (long) chr->constraints.maximumValue,
                                    (long) chr->constraints.stepValue);
                            return false;
                        }
                        break;
                    }
                    case kHAPCharacteristicFormat_Float: {
                        const HAPFloatCharacteristic* chr = service->characteristics[j];
                        BASE_CHARACTERISTIC_CHECKS(chr);

                        float minimumValue = chr->constraints.minimumValue;
                        float maximumValue = chr->constraints.maximumValue;
                        float stepValue = chr->constraints.stepValue;
                        if ((!HAPFloatIsFinite(minimumValue) && !HAPFloatIsInfinite(minimumValue)) ||
                            (!HAPFloatIsFinite(maximumValue) && !HAPFloatIsInfinite(maximumValue)) ||
                            chr->constraints.minimumValue > chr->constraints.maximumValue ||
                            !HAPFloatIsFinite(stepValue) || chr->constraints.stepValue < 0) {
                            HAPLogCharacteristicError(
                                    &logObject,
                                    characteristic,
                                    service,
                                    accessory,
                                    "Characteristic constraints invalid "
                                    "(constraints: minimumValue = %g / maximumValue = %g / stepValue = %g).",
                                    (double) chr->constraints.minimumValue,
                                    (double) chr->constraints.maximumValue,
                                    (double) chr->constraints.stepValue);
                            return false;
                        }
                        break;
                    }
                    case kHAPCharacteristicFormat_String: {
                        const HAPStringCharacteristic* chr = service->characteristics[j];
                        BASE_CHARACTERISTIC_CHECKS(chr);
                        break;
                    }
                    case kHAPCharacteristicFormat_TLV8: {
                        const HAPTLV8Characteristic* chr = service->characteristics[j];
                        BASE_CHARACTERISTIC_CHECKS(chr);
                        break;
                    }
                }
#undef BASE_CHARACTERISTIC_CHECKS
            }
        }

        // When all characteristics in a services are marked hidden then the service must also be marked as hidden.
        // See HomeKit Accessory Protocol Specification R17
        // Section 2.3.2.4 Hidden Service
        if (allCharacteristicsHidden && !service->properties.hidden) {
            HAPLogServiceError(
                    &logObject,
                    service,
                    accessory,
                    "Service must be marked hidden if all of its characteristics are marked hidden.");
            return false;
        }

        // iOS 11: The configuration attribute is only working on HAP Protocol Information service.
        if (service->properties.ble.supportsConfiguration &&
            !HAPUUIDAreEqual(service->serviceType, &kHAPServiceType_HAPProtocolInformation)) {
            HAPLogServiceError(
                    &logObject,
                    service,
                    accessory,
                    "Only the HAP Protocol Information service may support configuration.");
            return false;
        }
    }
    if (!hasPrimaryService) {
        // See HomeKit Accessory Protocol Specification R17
        // Section 2.3.2.3 Primary Service
        HAPLogAccessory(
                &logObject, accessory, "An accessory should advertise one of its services as its primary service.");
    }

    // Validate accessory callbacks.
    {
        if (!accessory->callbacks.identify) {
            // The accessory must implement an identify routine.
            // See HomeKit Accessory Protocol Specification R17
            // Section 6.7.6 Identify Routine
            HAPLogAccessory(
                    &logObject,
                    accessory,
                    "The accessory must implement an identify routine. "
                    "In the HAPAccessory structure, callbacks.identify should be defined.");
        }
    }

    return true;
}

HAP_RESULT_USE_CHECK
bool HAPRegularAccessoryIsValid(HAPAccessoryServer* server, const HAPAccessory* accessory) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);
    HAPPrecondition(accessory->name);

    if (accessory->aid != 1) {
        HAPLogAccessoryError(&logObject, accessory, "Primary accessory must have aid 1.");
        return false;
    }

    // Validate category.
    if (!accessory->category) {
        HAPLogAccessoryError(
                &logObject, accessory, "Invalid accessory category has been selected (%u).", accessory->category);
        return false;
    }

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    // Validate BLE specific requirements.
    if (server->transports.ble) {
        // Work around iOS Bluetooth limitations.
        // The Local Name should match the accessory's markings and packaging and not contain ':' or ';'.
        // See Accessory Design Guidelines for Apple Devices R7
        // Section 11.4 Advertising Data
        for (const char* c = accessory->name; *c; c++) {
            if (*c == ':' || *c == ';') {
                HAPLogAccessoryError(&logObject, accessory, "The accessory name should not contain ':' or ';'.");
            }
        }
    }
#endif

    if (!AccessoryIsValid(accessory)) {
        return false;
    }

    return true;
}

HAP_RESULT_USE_CHECK
bool HAPBridgedAccessoryIsValid(const HAPAccessory* bridgedAccessory) {
    HAPPrecondition(bridgedAccessory);

    if (bridgedAccessory->aid == 1) {
        HAPLogAccessoryError(&logObject, bridgedAccessory, "Bridged accessory must have aid other than 1.");
        return false;
    }
    if (bridgedAccessory->category != kHAPAccessoryCategory_BridgedAccessory) {
        HAPLogAccessoryError(
                &logObject,
                bridgedAccessory,
                "Bridged accessory must have category kHAPAccessoryCategory_BridgedAccessory.");
        return false;
    }

    if (!AccessoryIsValid(bridgedAccessory)) {
        return false;
    }

    return true;
}
