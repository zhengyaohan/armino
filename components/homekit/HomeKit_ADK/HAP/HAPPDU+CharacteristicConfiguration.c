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

#include "HAPBLECharacteristic+Broadcast.h"
#include "HAPCharacteristic.h"
#include "HAPLog+Attributes.h"
#include "HAPLogSubsystem.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "PDU" };

/**
 * Characteristic configuration parameter types.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Table 7-28 Characteristic configuration parameter types
 */
HAP_ENUM_BEGIN(uint8_t, HAPPDUTLVType_CharacteristicConfiguration) {
    /** HAP-Characteristic-Configuration-Param-Properties. */
    kHAPPDUTLVType_CharacteristicConfiguration_Properties = 0x01,

    /** HAP-Characteristic-Configuration-Param-Broadcast-Interval. */
    kHAPPDUTLVType_CharacteristicConfiguration_BroadcastInterval = 0x02
} HAP_ENUM_END(uint8_t, HAPPDUTLVType_CharacteristicConfiguration);

/**
 * Characteristic configuration properties.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Table 7-29 Characteristic configuration properties
 */
HAP_OPTIONS_BEGIN(uint8_t, HAPPDUValue_CharacteristicConfiguration_Property) {
    /** Enable/Disable Broadcast Notification. */
    kHAPPDUValue_CharacteristicConfiguration_Property_EnableBroadcasts = 1U << 0U
} HAP_OPTIONS_END(uint8_t, HAPPDUValue_CharacteristicConfiguration_Property);

HAP_RESULT_USE_CHECK
HAPError HAPPDUHandleCharacteristicConfigurationRequest(
        const HAPCharacteristic* characteristic_,
        const HAPService* service,
        const HAPAccessory* accessory,
        HAPTLVReader* requestReader,
        HAPPlatformKeyValueStoreRef keyValueStore) {
    HAPPrecondition(characteristic_);
    const HAPBaseCharacteristic* characteristic = characteristic_;
    HAPPrecondition(service);
    HAPPrecondition(accessory);
    HAPPrecondition(requestReader);
    HAPPrecondition(keyValueStore);

    HAPError err;

    HAPTLV propertiesTLV, broadcastIntervalTLV;
    propertiesTLV.type = kHAPPDUTLVType_CharacteristicConfiguration_Properties;
    broadcastIntervalTLV.type = kHAPPDUTLVType_CharacteristicConfiguration_BroadcastInterval;
    err = HAPTLVReaderGetAll(requestReader, (HAPTLV* const[]) { &propertiesTLV, &broadcastIntervalTLV, NULL });
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }

    // HAP-Characteristic-Configuration-Param-Properties.
    if (propertiesTLV.value.bytes) {
        if (propertiesTLV.value.numBytes != 2) {
            HAPLogCharacteristic(
                    &logObject,
                    characteristic,
                    service,
                    accessory,
                    "HAP-Characteristic-Configuration-Param-Properties has invalid length (%lu).",
                    (unsigned long) propertiesTLV.value.numBytes);
            return kHAPError_InvalidData;
        }
        uint16_t properties = HAPReadLittleUInt16(propertiesTLV.value.bytes);
        uint16_t allProperties = kHAPPDUValue_CharacteristicConfiguration_Property_EnableBroadcasts;
        if (properties & (uint16_t) ~allProperties) {
            HAPLogCharacteristic(
                    &logObject,
                    characteristic,
                    service,
                    accessory,
                    "HAP-Characteristic-Configuration-Param-Properties invalid: %u.",
                    properties);
            return kHAPError_InvalidData;
        }

        // Broadcast notifications.
        if (properties & (uint16_t) kHAPPDUValue_CharacteristicConfiguration_Property_EnableBroadcasts) {
            // HAP-Characteristic-Configuration-Param-Broadcast-Interval.
            HAPBLECharacteristicBroadcastInterval broadcastInterval = kHAPBLECharacteristicBroadcastInterval_20Ms;
            if (broadcastIntervalTLV.value.bytes) {
                if (broadcastIntervalTLV.value.numBytes != 1) {
                    HAPLogCharacteristic(
                            &logObject,
                            characteristic,
                            service,
                            accessory,
                            "HAP-Characteristic-Configuration-Param-Broadcast-Interval has invalid length (%lu).",
                            (unsigned long) broadcastIntervalTLV.value.numBytes);
                    return kHAPError_InvalidData;
                }
                uint8_t value = ((const uint8_t*) broadcastIntervalTLV.value.bytes)[0];
                if (!HAPBLECharacteristicIsValidBroadcastInterval(value)) {
                    HAPLogCharacteristic(
                            &logObject,
                            characteristic,
                            service,
                            accessory,
                            "HAP-Characteristic-Configuration-Param-Broadcast-Interval invalid: %u.",
                            broadcastInterval);
                    return kHAPError_InvalidData;
                }
                broadcastInterval = (HAPBLECharacteristicBroadcastInterval) value;
            }

            // Check that characteristic supports broadcasts.
            if (!characteristic->properties.ble.supportsBroadcastNotification) {
                HAPLogCharacteristic(
                        &logObject,
                        characteristic,
                        service,
                        accessory,
                        "Controller requested enabling broadcasts on characteristic that does not support it.");
                return kHAPError_InvalidData;
            }

            // Enable broadcasts.
            err = HAPBLECharacteristicEnableBroadcastNotifications(
                    characteristic, service, accessory, broadcastInterval, keyValueStore);
            if (err) {
                HAPAssert(err == kHAPError_Unknown);
                return err;
            }
        } else {
            // HAP-Characteristic-Configuration-Param-Broadcast-Interval.
            if (broadcastIntervalTLV.value.bytes) {
                HAPLogCharacteristic(
                        &logObject,
                        characteristic,
                        service,
                        accessory,
                        "Excess HAP-Characteristic-Configuration-Param-Broadcast-Interval (disabling broadcasts).");
                return kHAPError_InvalidData;
            }

            // Disable broadcasts if characteristic supports broadcasts.
            if (characteristic->properties.ble.supportsBroadcastNotification) {
                err = HAPBLECharacteristicDisableBroadcastNotifications(
                        characteristic, service, accessory, keyValueStore);
                if (err) {
                    HAPAssert(err == kHAPError_Unknown);
                    return err;
                }
            }
        }
    } else {
        // HAP-Characteristic-Configuration-Param-Broadcast-Interval.
        if (broadcastIntervalTLV.value.bytes) {
            HAPLogCharacteristic(
                    &logObject,
                    characteristic,
                    service,
                    accessory,
                    "Excess HAP-Characteristic-Configuration-Param-Broadcast-Interval (no properties present).");
            return kHAPError_InvalidData;
        }
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPDUGetCharacteristicConfigurationResponse(
        const HAPCharacteristic* characteristic_,
        const HAPService* service,
        const HAPAccessory* accessory,
        HAPTLVWriter* responseWriter,
        HAPPlatformKeyValueStoreRef keyValueStore) {
    HAPPrecondition(characteristic_);
    const HAPBaseCharacteristic* characteristic = characteristic_;
    HAPPrecondition(service);
    HAPPrecondition(accessory);
    HAPPrecondition(responseWriter);
    HAPPrecondition(keyValueStore);

    HAPError err;
    uint16_t properties = 0;
    if (characteristic->properties.ble.supportsBroadcastNotification) {
        HAPBLECharacteristicBroadcastInterval broadcastInterval;
        bool broadcastsEnabled;
        err = HAPBLECharacteristicGetBroadcastConfiguration(
                characteristic, service, accessory, &broadcastsEnabled, &broadcastInterval, keyValueStore);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            return err;
        }

        if (broadcastsEnabled) {
            properties |= (uint16_t) kHAPPDUValue_CharacteristicConfiguration_Property_EnableBroadcasts;

            // HAP-Characteristic-Configuration-Param-Broadcast-Interval.
            // The accessory must include all parameters in the response even if the default Broadcast Interval is used.
            // See HomeKit Accessory Protocol Specification R17
            // Section 7.3.4.15 HAP-Characteristic-Configuration-Response
            uint8_t broadcastIntervalBytes[] = { broadcastInterval };
            err = HAPTLVWriterAppend(
                    responseWriter,
                    &(const HAPTLV) {
                            .type = kHAPPDUTLVType_CharacteristicConfiguration_BroadcastInterval,
                            .value = { .bytes = broadcastIntervalBytes, .numBytes = sizeof broadcastIntervalBytes } });
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }
        }
    }

    // HAP-Characteristic-Configuration-Param-Properties.
    uint8_t propertiesBytes[] = { HAPExpandLittleUInt16(properties) };
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPDUTLVType_CharacteristicConfiguration_Properties,
                              .value = { .bytes = propertiesBytes, .numBytes = sizeof propertiesBytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    return kHAPError_None;
}

#endif
