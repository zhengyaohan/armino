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

#include "HAPPDU+TLV.h"

#include "HAPCharacteristic.h"
#include "HAPLogSubsystem.h"
#include "HAPTLV+Internal.h"
#include "HAPUUID.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "BLEPDU" };

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPPDUValue_CharacteristicType_Encode(const HAPCharacteristic* characteristic_, HAPTLVWriter* responseWriter) {
    HAPPrecondition(characteristic_);
    const HAPBaseCharacteristic* characteristic = characteristic_;
    HAPPrecondition(responseWriter);

    HAPError err;

    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPDUTLVType_CharacteristicType,
                              .value = { .bytes = characteristic->characteristicType->bytes,
                                         .numBytes = sizeof characteristic->characteristicType->bytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------
HAP_RESULT_USE_CHECK
HAPError HAPPDUValue_CharacteristicType_EncodeShortForm(
        const HAPCharacteristic* characteristic_,
        HAPTLVWriter* responseWriter) {
    HAPPrecondition(characteristic_);
    const HAPBaseCharacteristic* characteristic = characteristic_;
    HAPPrecondition(responseWriter);

    HAPError err;

    uint8_t shortUUIDBytes[sizeof characteristic->characteristicType->bytes];
    size_t numShortUUIDBytes;
    err = HAPUUIDGetShortFormBytes(
            characteristic->characteristicType, shortUUIDBytes, sizeof shortUUIDBytes, &numShortUUIDBytes);
    HAPAssert(!err);

    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPDUTLVType_CharacteristicType,
                              .value = { .bytes = shortUUIDBytes, .numBytes = numShortUUIDBytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------
HAP_RESULT_USE_CHECK
HAPError HAPPDUValue_CharacteristicInstanceID_Encode(
        const HAPCharacteristic* characteristic_,
        HAPTLVWriter* responseWriter) {
    HAPPrecondition(characteristic_);
    const HAPBaseCharacteristic* characteristic = characteristic_;
    HAPPrecondition(responseWriter);

    HAPError err;

    HAPAssert(characteristic->iid <= UINT16_MAX);
    uint8_t cidBytes[] = { HAPExpandLittleUInt16(characteristic->iid) };
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPDUTLVType_CharacteristicInstanceID,
                              .value = { .bytes = cidBytes, .numBytes = sizeof cidBytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPPDUValue_ServiceType_EncodeShortForm(const HAPService* service, HAPTLVWriter* responseWriter) {
    HAPPrecondition(service);
    HAPPrecondition(responseWriter);

    HAPError err;

    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPDUTLVType_ServiceType,
                              .value = { .bytes = service->serviceType->bytes,
                                         .numBytes = sizeof service->serviceType->bytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------
HAP_RESULT_USE_CHECK
HAPError HAPBLEPDUTLVSerializeServiceTypeShortForm(const HAPService* service, HAPTLVWriter* responseWriter) {
    HAPPrecondition(service);
    HAPPrecondition(responseWriter);

    HAPError err;

    uint8_t shortUUIDBytes[sizeof service->serviceType->bytes];
    size_t numShortUUIDBytes;
    err = HAPUUIDGetShortFormBytes(service->serviceType, shortUUIDBytes, sizeof shortUUIDBytes, &numShortUUIDBytes);
    HAPAssert(!err);

    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPDUTLVType_ServiceType,
                              .value = { .bytes = shortUUIDBytes, .numBytes = numShortUUIDBytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPPDUValue_ServiceInstanceID_Encode(const HAPService* service, HAPTLVWriter* responseWriter) {
    HAPPrecondition(service);
    HAPPrecondition(responseWriter);

    HAPError err;

    HAPAssert(service->iid <= UINT16_MAX);
    uint8_t sidBytes[] = { HAPExpandLittleUInt16(service->iid) };
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPDUTLVType_ServiceInstanceID,
                              .value = { .bytes = sidBytes, .numBytes = sizeof sidBytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPPDUValue_HAPCharacteristicProperties_Encode(
        const HAPCharacteristic* characteristic_,
        HAPTLVWriter* responseWriter) {
    HAPPrecondition(characteristic_);
    const HAPBaseCharacteristic* characteristic = characteristic_;
    HAPPrecondition(responseWriter);

    HAPError err;

    uint16_t propsShort = (characteristic->properties.ble.readableWithoutSecurity ? 0x0001U : 0U) |
                          (characteristic->properties.ble.writableWithoutSecurity ? 0x0002U : 0U) |
                          (characteristic->properties.supportsAuthorizationData ? 0x0004U : 0U) |
                          (characteristic->properties.requiresTimedWrite ? 0x0008U : 0U) |
                          (characteristic->properties.readable ? 0x0010U : 0U) |
                          (characteristic->properties.writable ? 0x0020U : 0U) |
                          (characteristic->properties.hidden ? 0x0040U : 0U) |
                          (characteristic->properties.supportsEventNotification ? 0x0080U : 0U) |
                          (characteristic->properties.ble.supportsDisconnectedNotification ? 0x0100U : 0U) |
                          (characteristic->properties.ble.supportsBroadcastNotification ? 0x0200U : 0U) |
                          (characteristic->properties.supportsEventNotificationContextInformation ? 0x0400U : 0U);
    uint8_t propertiesBytes[] = { HAPExpandLittleUInt16(propsShort) };
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPDUTLVType_HAPCharacteristicPropertiesDescriptor,
                              .value = { .bytes = propertiesBytes, .numBytes = sizeof propertiesBytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError
        HAPPDUValue_GATTUserDescription_Encode(const HAPCharacteristic* characteristic_, HAPTLVWriter* responseWriter) {
    HAPPrecondition(characteristic_);
    const HAPBaseCharacteristic* characteristic = characteristic_;
    HAPPrecondition(responseWriter);

    HAPError err;

    if (!characteristic->manufacturerDescription) {
        return kHAPError_None;
    }

    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPDUTLVType_GATTUserDescriptionDescriptor,
                              .value = { .bytes = characteristic->manufacturerDescription,
                                         .numBytes = HAPStringGetNumBytes(
                                                 HAPNonnull(characteristic->manufacturerDescription)) } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Converts a HAP format to the corresponding BT SIG format.
 *
 * @param      hapFormat            HAP format to convert.
 *
 * @return BT SIG format.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Table 7-51 HAP Format to BT SIG Format mapping
 */
HAP_RESULT_USE_CHECK
static uint8_t ConvertHAPFormatToBTSIGFormat(HAPCharacteristicFormat hapFormat) {
    switch (hapFormat) {
        case kHAPCharacteristicFormat_Data:
            return 0x1B;
        case kHAPCharacteristicFormat_Bool:
            return 0x01;
        case kHAPCharacteristicFormat_UInt8:
            return 0x04;
        case kHAPCharacteristicFormat_UInt16:
            return 0x06;
        case kHAPCharacteristicFormat_UInt32:
            return 0x08;
        case kHAPCharacteristicFormat_UInt64:
            return 0x0A;
        case kHAPCharacteristicFormat_Int:
            return 0x10;
        case kHAPCharacteristicFormat_Float:
            return 0x14;
        case kHAPCharacteristicFormat_String:
            return 0x19;
        case kHAPCharacteristicFormat_TLV8:
            return 0x1B;
        default:
            HAPFatalError();
    }
}

/**
 * Converts a HAP unit to the corresponding BT SIG unit.
 *
 * @param      hapUnit              HAP unit to convert.
 *
 * @return BT SIG unit.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Table 7-52 HAP Unit to BT SIG Unit mapping
 */
HAP_RESULT_USE_CHECK
static uint16_t ConvertHAPUnitToBTSIGUnit(HAPCharacteristicUnits hapUnit) {
    switch (hapUnit) {
        case kHAPCharacteristicUnits_Celsius:
            return 0x272F;
        case kHAPCharacteristicUnits_ArcDegrees:
            return 0x2763;
        case kHAPCharacteristicUnits_Percentage:
            return 0x27AD;
        case kHAPCharacteristicUnits_None:
            return 0x2700;
        case kHAPCharacteristicUnits_Lux:
            return 0x2731;
        case kHAPCharacteristicUnits_Seconds:
            return 0x2703;
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPPDUValue_GATTPresentationFormatDescriptor_Encode(
        const HAPCharacteristic* characteristic_,
        HAPTLVWriter* responseWriter) {
    HAPPrecondition(characteristic_);
    const HAPBaseCharacteristic* characteristic = characteristic_;
    HAPPrecondition(responseWriter);

    HAPError err;

    uint8_t btSIGFormat = ConvertHAPFormatToBTSIGFormat(characteristic->format);

    uint16_t btSIGUnit = 0;
    switch (characteristic->format) {
        case kHAPCharacteristicFormat_UInt8: {
            btSIGUnit = ConvertHAPUnitToBTSIGUnit(((const HAPUInt8Characteristic*) characteristic)->units);
            break;
        }
        case kHAPCharacteristicFormat_UInt16: {
            btSIGUnit = ConvertHAPUnitToBTSIGUnit(((const HAPUInt16Characteristic*) characteristic)->units);
            break;
        }
        case kHAPCharacteristicFormat_UInt32: {
            btSIGUnit = ConvertHAPUnitToBTSIGUnit(((const HAPUInt32Characteristic*) characteristic)->units);
            break;
        }
        case kHAPCharacteristicFormat_UInt64: {
            btSIGUnit = ConvertHAPUnitToBTSIGUnit(((const HAPUInt64Characteristic*) characteristic)->units);
            break;
        }
        case kHAPCharacteristicFormat_Int: {
            btSIGUnit = ConvertHAPUnitToBTSIGUnit(((const HAPIntCharacteristic*) characteristic)->units);
            break;
        }
        case kHAPCharacteristicFormat_Float: {
            btSIGUnit = ConvertHAPUnitToBTSIGUnit(((const HAPFloatCharacteristic*) characteristic)->units);
            break;
        }
        case kHAPCharacteristicFormat_Bool:
        case kHAPCharacteristicFormat_String:
        case kHAPCharacteristicFormat_TLV8:
        case kHAPCharacteristicFormat_Data: {
            btSIGUnit = ConvertHAPUnitToBTSIGUnit(kHAPCharacteristicUnits_None);
            break;
        }
        default:
            HAPFatalError();
    }

    uint8_t formatBytes[] = { /* Format (8bit): */ btSIGFormat,
                              /* Exponent (sint8): */ 0,
                              /* Unit (uint16): */ HAPExpandLittleUInt16(btSIGUnit),
                              /* Namespace (8bit): */ 1,
                              /* Description (16bit): */ HAPExpandLittleUInt16(0U) };
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPDUTLVType_GATTPresentationFormatDescriptor,
                              .value = { .bytes = formatBytes, .numBytes = sizeof formatBytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPPDUValue_GATTValidRange_Encode(const HAPCharacteristic* characteristic_, HAPTLVWriter* responseWriter) {
    HAPPrecondition(characteristic_);
    HAPPrecondition(responseWriter);

    HAPError err;

    switch (*((const HAPCharacteristicFormat*) characteristic_)) {
        case kHAPCharacteristicFormat_Data: {
            const HAPDataCharacteristic* characteristic = characteristic_;
            uint32_t maxLength HAP_UNUSED = characteristic->constraints.maxLength;

            // Although HAP over IP defines a "maxDataLen" key as a characteristic property,
            // no such descriptor is defined for HAP over Bluetooth LE.
            // See HomeKit Accessory Protocol Specification R17
            // Table 6-3 Properties of Characteristic Objects in JSON
            //
            // HAP over Bluetooth LE used to define a similar descriptor for String characteristics.
            // However, using that descriptor for Data characteristics leads to an error during pairing as of iOS 13:
            // "Characteristics with format type 'data' do not support valid ranges"
            // Therefore, we don't attempt to transfer maximum length information for Data characteristics
            // when using HAP over Bluetooth LE.
            // See HomeKit Accessory Protocol Specification - R17
            // Section 7.4.4.6.4 Minimum and Maximum Length Descriptor
            //
            // Note also that HAP over Bluetooth LE has a global limit for characteristic value lengths
            // independent of their characteristic format. This implicitly limits Data characteristic values.
            // See HomeKit Accessory Protocol Specification R17
            // Section 7.4.1.7 Maximum Payload Size
        }
            return kHAPError_None;
        case kHAPCharacteristicFormat_Bool: {
        }
            return kHAPError_None;
        case kHAPCharacteristicFormat_UInt8: {
            const HAPUInt8Characteristic* characteristic = characteristic_;
            uint8_t minimumValue = characteristic->constraints.minimumValue;
            uint8_t maximumValue = characteristic->constraints.maximumValue;
            HAPPrecondition(minimumValue <= maximumValue);

            if (!minimumValue && maximumValue == UINT8_MAX) {
                return kHAPError_None;
            }
            uint8_t rangeBytes[] = { minimumValue, maximumValue };
            err = HAPTLVWriterAppend(
                    responseWriter,
                    &(const HAPTLV) { .type = kHAPPDUTLVType_GATTValidRange,
                                      .value = { .bytes = rangeBytes, .numBytes = sizeof rangeBytes } });
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }
        }
            return kHAPError_None;
        case kHAPCharacteristicFormat_UInt16: {
            const HAPUInt16Characteristic* characteristic = characteristic_;
            uint16_t minimumValue = characteristic->constraints.minimumValue;
            uint16_t maximumValue = characteristic->constraints.maximumValue;
            HAPPrecondition(minimumValue <= maximumValue);

            if (!minimumValue && maximumValue == UINT16_MAX) {
                return kHAPError_None;
            }

            uint8_t rangeBytes[] = { HAPExpandLittleUInt16(minimumValue), HAPExpandLittleUInt16(maximumValue) };
            err = HAPTLVWriterAppend(
                    responseWriter,
                    &(const HAPTLV) { .type = kHAPPDUTLVType_GATTValidRange,
                                      .value = { .bytes = rangeBytes, .numBytes = sizeof rangeBytes } });
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }
        }
            return kHAPError_None;
        case kHAPCharacteristicFormat_UInt32: {
            const HAPUInt32Characteristic* characteristic = characteristic_;
            uint32_t minimumValue = characteristic->constraints.minimumValue;
            uint32_t maximumValue = characteristic->constraints.maximumValue;
            HAPPrecondition(minimumValue <= maximumValue);

            if (!minimumValue && maximumValue == UINT32_MAX) {
                return kHAPError_None;
            }

            uint8_t rangeBytes[] = { HAPExpandLittleUInt32(minimumValue), HAPExpandLittleUInt32(maximumValue) };
            err = HAPTLVWriterAppend(
                    responseWriter,
                    &(const HAPTLV) { .type = kHAPPDUTLVType_GATTValidRange,
                                      .value = { .bytes = rangeBytes, .numBytes = sizeof rangeBytes } });
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }
        }
            return kHAPError_None;
        case kHAPCharacteristicFormat_UInt64: {
            const HAPUInt64Characteristic* characteristic = characteristic_;
            uint64_t minimumValue = characteristic->constraints.minimumValue;
            uint64_t maximumValue = characteristic->constraints.maximumValue;
            HAPPrecondition(minimumValue <= maximumValue);

            if (!minimumValue && maximumValue == UINT64_MAX) {
                return kHAPError_None;
            }

            uint8_t rangeBytes[] = { HAPExpandLittleUInt64(minimumValue), HAPExpandLittleUInt64(maximumValue) };
            err = HAPTLVWriterAppend(
                    responseWriter,
                    &(const HAPTLV) { .type = kHAPPDUTLVType_GATTValidRange,
                                      .value = { .bytes = rangeBytes, .numBytes = sizeof rangeBytes } });
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }
        }
            return kHAPError_None;
        case kHAPCharacteristicFormat_Int: {
            const HAPIntCharacteristic* characteristic = characteristic_;
            int32_t minimumValue = characteristic->constraints.minimumValue;
            int32_t maximumValue = characteristic->constraints.maximumValue;
            HAPPrecondition(minimumValue <= maximumValue);

            if (minimumValue == INT32_MIN && maximumValue == INT32_MAX) {
                return kHAPError_None;
            }

            uint8_t rangeBytes[] = { HAPExpandLittleInt32(minimumValue), HAPExpandLittleInt32(maximumValue) };
            err = HAPTLVWriterAppend(
                    responseWriter,
                    &(const HAPTLV) { .type = kHAPPDUTLVType_GATTValidRange,
                                      .value = { .bytes = rangeBytes, .numBytes = sizeof rangeBytes } });
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }
        }
            return kHAPError_None;
        case kHAPCharacteristicFormat_Float: {
            const HAPFloatCharacteristic* characteristic = characteristic_;
            float minimumValue = characteristic->constraints.minimumValue;
            float maximumValue = characteristic->constraints.maximumValue;
            HAPPrecondition(HAPFloatIsFinite(minimumValue) || HAPFloatIsInfinite(minimumValue));
            HAPPrecondition(HAPFloatIsFinite(maximumValue) || HAPFloatIsInfinite(maximumValue));
            HAPPrecondition(minimumValue <= maximumValue);

            if (HAPFloatIsInfinite(minimumValue) && minimumValue < 0 && HAPFloatIsInfinite(maximumValue) &&
                maximumValue > 0) {
                return kHAPError_None;
            }

            uint32_t minimumBitPattern = HAPFloatGetBitPattern(minimumValue);
            uint32_t maximumBitPattern = HAPFloatGetBitPattern(maximumValue);
            uint8_t rangeBytes[] = { HAPExpandLittleUInt32(minimumBitPattern),
                                     HAPExpandLittleUInt32(maximumBitPattern) };
            err = HAPTLVWriterAppend(
                    responseWriter,
                    &(const HAPTLV) { .type = kHAPPDUTLVType_GATTValidRange,
                                      .value = { .bytes = rangeBytes, .numBytes = sizeof rangeBytes } });
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }
        }
            return kHAPError_None;
        case kHAPCharacteristicFormat_String: {
            // See HomeKit Accessory Protocol Specification R17. Section 7.4.4.6.4.
            // The descriptor must only be used for formats of integer types or float.
            return kHAPError_None;
        }
        case kHAPCharacteristicFormat_TLV8: {
        }
            return kHAPError_None;
        default:
            HAPFatalError();
    }
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPPDUValue_HAPStepValueDescriptor_Encode(
        const HAPCharacteristic* characteristic_,
        HAPTLVWriter* responseWriter) {
    HAPPrecondition(characteristic_);
    HAPPrecondition(responseWriter);

    HAPError err;

    switch (*((const HAPCharacteristicFormat*) characteristic_)) {
        case kHAPCharacteristicFormat_Data:
        case kHAPCharacteristicFormat_Bool:
        case kHAPCharacteristicFormat_String:
        case kHAPCharacteristicFormat_TLV8: {
        }
            return kHAPError_None;
        case kHAPCharacteristicFormat_UInt8: {
            const HAPUInt8Characteristic* characteristic = characteristic_;
            uint8_t stepValue = characteristic->constraints.stepValue;

            if (stepValue <= 1) {
                return kHAPError_None;
            }

            uint8_t stepBytes[] = { stepValue };
            err = HAPTLVWriterAppend(
                    responseWriter,
                    &(const HAPTLV) { .type = kHAPPDUTLVType_HAPStepValueDescriptor,
                                      .value = { .bytes = stepBytes, .numBytes = sizeof stepBytes } });
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }
        }
            return kHAPError_None;
        case kHAPCharacteristicFormat_UInt16: {
            const HAPUInt16Characteristic* characteristic = characteristic_;
            uint16_t stepValue = characteristic->constraints.stepValue;

            if (stepValue <= 1) {
                return kHAPError_None;
            }

            uint8_t stepBytes[] = { HAPExpandLittleUInt16(stepValue) };
            err = HAPTLVWriterAppend(
                    responseWriter,
                    &(const HAPTLV) { .type = kHAPPDUTLVType_HAPStepValueDescriptor,
                                      .value = { .bytes = stepBytes, .numBytes = sizeof stepBytes } });
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }
        }
            return kHAPError_None;
        case kHAPCharacteristicFormat_UInt32: {
            const HAPUInt32Characteristic* characteristic = characteristic_;
            uint32_t stepValue = characteristic->constraints.stepValue;

            if (stepValue <= 1) {
                return kHAPError_None;
            }

            uint8_t stepBytes[] = { HAPExpandLittleUInt32(stepValue) };
            err = HAPTLVWriterAppend(
                    responseWriter,
                    &(const HAPTLV) { .type = kHAPPDUTLVType_HAPStepValueDescriptor,
                                      .value = { .bytes = stepBytes, .numBytes = sizeof stepBytes } });
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }
        }
            return kHAPError_None;
        case kHAPCharacteristicFormat_UInt64: {
            const HAPUInt64Characteristic* characteristic = characteristic_;
            uint64_t stepValue = characteristic->constraints.stepValue;

            if (stepValue <= 1) {
                return kHAPError_None;
            }

            uint8_t stepBytes[] = { HAPExpandLittleUInt64(stepValue) };
            err = HAPTLVWriterAppend(
                    responseWriter,
                    &(const HAPTLV) { .type = kHAPPDUTLVType_HAPStepValueDescriptor,
                                      .value = { .bytes = stepBytes, .numBytes = sizeof stepBytes } });
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }
        }
            return kHAPError_None;
        case kHAPCharacteristicFormat_Int: {
            const HAPIntCharacteristic* characteristic = characteristic_;
            int32_t stepValue = characteristic->constraints.stepValue;
            HAPPrecondition(stepValue >= 0);

            if (stepValue <= 1) {
                return kHAPError_None;
            }

            uint8_t stepBytes[] = { HAPExpandLittleInt32(stepValue) };
            err = HAPTLVWriterAppend(
                    responseWriter,
                    &(const HAPTLV) { .type = kHAPPDUTLVType_HAPStepValueDescriptor,
                                      .value = { .bytes = stepBytes, .numBytes = sizeof stepBytes } });
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }
        }
            return kHAPError_None;
        case kHAPCharacteristicFormat_Float: {
            const HAPFloatCharacteristic* characteristic = characteristic_;
            float stepValue = characteristic->constraints.stepValue;
            HAPPrecondition(HAPFloatIsFinite(stepValue));
            HAPPrecondition(stepValue >= 0);

            if (HAPFloatIsZero(stepValue)) {
                return kHAPError_None;
            }

            uint32_t bitPattern = HAPFloatGetBitPattern(stepValue);
            uint8_t stepBytes[] = { HAPExpandLittleUInt32(bitPattern) };
            err = HAPTLVWriterAppend(
                    responseWriter,
                    &(const HAPTLV) { .type = kHAPPDUTLVType_HAPStepValueDescriptor,
                                      .value = { .bytes = stepBytes, .numBytes = sizeof stepBytes } });
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }
        }
            return kHAPError_None;
        default:
            HAPFatalError();
    }
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPPDUValue_HAPServiceProperties_Encode(const HAPService* _Nullable service, HAPTLVWriter* responseWriter) {
    HAPPrecondition(responseWriter);

    HAPError err;

    // See HomeKit Accessory Protocol Specification R17
    // Table 7-49 HAP Service Properties
    uint16_t properties = 0;
    if (service) {
        properties = (uint16_t)(
                (service->properties.primaryService ? 0x0001U : 0U) | (service->properties.hidden ? 0x0002U : 0U) |
                (service->properties.ble.supportsConfiguration ? 0x0004U : 0U));
    }

    // Accessories must include the "HAP Service Properties" characteristic only if it supports non-default properties
    // or has linked services. Other services must not include this characteristic.
    // See HomeKit Accessory Protocol Specification R17
    // Section 7.4.4.4 HAP Service Properties
    if (!properties && (!service || !service->linkedServices || !service->linkedServices[0])) {
        return kHAPError_None;
    }

    uint8_t propertiesBytes[] = { HAPExpandLittleUInt16(properties) };
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPDUTLVType_HAPServiceProperties,
                              .value = { .bytes = propertiesBytes, .numBytes = sizeof propertiesBytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPPDUValue_HAPLinkedServices_Encode(const HAPService* _Nullable service, HAPTLVWriter* responseWriter) {
    HAPPrecondition(responseWriter);

    HAPError err;

    // See HomeKit Accessory Protocol Specification R17
    // Section 7.4.4.4.1 HAP Linked Services
    size_t linkedServicesCount = 0;
    if (service && service->linkedServices) {
        while (service->linkedServices[linkedServicesCount]) {
            linkedServicesCount++;
        }
    }

    void* bytes;
    size_t maxBytes;
    HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);

    uint8_t* linkedServicesBytes = HAPTLVScratchBufferAllocUnaligned(&bytes, &maxBytes, 2 * linkedServicesCount);
    if (!linkedServicesBytes) {
        HAPLog(&logObject, "Not enough memory to allocate HAP-Param-HAP-Linked-Services.");
        return kHAPError_OutOfResources;
    }

    for (size_t i = 0; i < linkedServicesCount; i++) {
        HAPWriteLittleUInt16(&linkedServicesBytes[2 * i], service->linkedServices[i]);
    }

    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPDUTLVType_HAPLinkedServices,
                              .value = { .bytes = linkedServicesBytes, .numBytes = 2 * linkedServicesCount } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPPDUValue_HAPValidValuesDescriptor_Encode(
        const HAPCharacteristic* characteristic_,
        HAPTLVWriter* responseWriter) {
    HAPPrecondition(characteristic_);
    const HAPBaseCharacteristic* characteristic = characteristic_;
    HAPPrecondition(responseWriter);

    HAPError err;

    // See HomeKit Accessory Protocol Specification R17
    // Section 7.4.5.2 Valid Values Descriptor
    if (characteristic->format != kHAPCharacteristicFormat_UInt8) {
        return kHAPError_None;
    }

    const uint8_t* const* validValues = ((const HAPUInt8Characteristic*) characteristic)->constraints.validValues;
    size_t validValuesCount = 0;
    if (validValues) {
        while (validValues[validValuesCount]) {
            validValuesCount++;
        }
    }
    if (!validValuesCount) {
        return kHAPError_None;
    }

    // See HomeKit Accessory Protocol Specification R17
    // Section 2.3.3.1 Valid Characteristic Values
    HAPPrecondition(HAPUUIDIsAppleDefined(characteristic->characteristicType));

    void* bytes;
    size_t maxBytes;
    HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);

    uint8_t* validValuesBytes = HAPTLVScratchBufferAllocUnaligned(&bytes, &maxBytes, validValuesCount);
    if (!validValuesBytes) {
        HAPLog(&logObject, "Not enough memory to allocate HAP-Param-HAP-Valid-Values-Descriptor.");
        return kHAPError_OutOfResources;
    }

    for (size_t i = 0; i < validValuesCount; i++) {
        validValuesBytes[i] = *validValues[i];

        // See HomeKit Accessory Protocol Specification R17
        // Section 7.4.5.2 Valid Values Descriptor
        if (i) {
            HAPPrecondition(validValuesBytes[i] > validValuesBytes[i - 1]);
        }
    }

    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPDUTLVType_HAPValidValuesDescriptor,
                              .value = { .bytes = validValuesBytes, .numBytes = validValuesCount } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPPDUValue_HAPValidValuesRangeDescriptor_Encode(
        const HAPCharacteristic* characteristic_,
        HAPTLVWriter* responseWriter) {
    HAPPrecondition(characteristic_);
    const HAPBaseCharacteristic* characteristic = characteristic_;
    HAPPrecondition(responseWriter);

    HAPError err;

    // See HomeKit Accessory Protocol Specification R17
    // Section 7.4.5.3 Valid Values Range Descriptor
    if (characteristic->format != kHAPCharacteristicFormat_UInt8) {
        return kHAPError_None;
    }

    const HAPUInt8CharacteristicValidValuesRange* const* validValuesRanges =
            ((const HAPUInt8Characteristic*) characteristic)->constraints.validValuesRanges;
    size_t validValuesRangesCount = 0;
    if (validValuesRanges) {
        while (validValuesRanges[validValuesRangesCount]) {
            validValuesRangesCount++;
        }
    }
    if (!validValuesRangesCount) {
        return kHAPError_None;
    }

    // See HomeKit Accessory Protocol Specification R17
    // Section 2.3.3.1 Valid Characteristic Values
    HAPPrecondition(HAPUUIDIsAppleDefined(characteristic->characteristicType));

    void* bytes;
    size_t maxBytes;
    HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);

    uint8_t* validValuesRangesBytes = HAPTLVScratchBufferAllocUnaligned(&bytes, &maxBytes, 2 * validValuesRangesCount);
    if (!validValuesRangesBytes) {
        HAPLog(&logObject, "Not enough memory to allocate HAP-Param-HAP-Valid-Values-Range-Descriptor.");
        return kHAPError_OutOfResources;
    }

    for (size_t i = 0; i < validValuesRangesCount; i++) {
        HAPPrecondition(validValuesRanges[i]->start <= validValuesRanges[i]->end);

        validValuesRangesBytes[2 * i + 0] = validValuesRanges[i]->start;
        validValuesRangesBytes[2 * i + 1] = validValuesRanges[i]->end;

        // See HomeKit Accessory Protocol Specification R17
        // Section 7.4.5.3 Valid Values Range Descriptor
        if (i) {
            HAPPrecondition(validValuesRangesBytes[2 * i] > validValuesRangesBytes[2 * i - 1]);
        }
    }

    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPDUTLVType_HAPValidValuesRangeDescriptor,
                              .value = { .bytes = validValuesRangesBytes, .numBytes = 2 * validValuesRangesCount } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    return kHAPError_None;
}

#endif
