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

#include "HAPIPAccessory.h"

#include "HAPAccessoryServer+Internal.h"
#include "HAPCharacteristic.h"
#include "HAPIPCharacteristic.h"
#include "HAPIPSession.h"
#include "HAPJSONUtils.h"
#include "HAPLog+Attributes.h"
#include "HAPLogSubsystem.h"
#include "HAPUUID.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "IPAccessory" };

/**
 * Default maximum number of bytes if the characteristic format is "string".
 *
 * @see See HomeKit Accessory Protocol Specification R17
 *      Table 6-3 Properties of Characteristic Objects in JSON
 */
#define kHAPIPAccessorySerialization_DefaultMaxStringBytes ((size_t) 64)

/**
 * Default maximum number of bytes if the characteristic format is "data".
 *
 * @see See HomeKit Accessory Protocol Specification R17
 *      Table 6-3 Properties of Characteristic Objects in JSON
 */
#define kHAPIPAccessorySerialization_DefaultMaxDataBytes ((size_t) 2097152)

/**
 * Accessory serialization state.
 */
HAP_ENUM_BEGIN(uint8_t, HAPIPAccessorySerializationState) {
    kHAPIPAccessorySerializationState_ResponseObject_Begin,
    kHAPIPAccessorySerializationState_ResponseObject_End,

    kHAPIPAccessorySerializationState_AccessoriesArray_Name,
    kHAPIPAccessorySerializationState_AccessoriesArray_NameSeparator,

    kHAPIPAccessorySerializationState_AccessoriesArray_Begin,
    kHAPIPAccessorySerializationState_AccessoriesArray_End,

    kHAPIPAccessorySerializationState_AccessoryObject_Begin,
    kHAPIPAccessorySerializationState_AccessoryObject_End,
    kHAPIPAccessorySerializationState_AccessoryObject_Separator,

    kHAPIPAccessorySerializationState_AccessoryID_Name,
    kHAPIPAccessorySerializationState_AccessoryID_NameSeparator,
    kHAPIPAccessorySerializationState_AccessoryID_Value,
    kHAPIPAccessorySerializationState_AccessoryID_ValueSeparator,

    kHAPIPAccessorySerializationState_ServicesArray_Name,
    kHAPIPAccessorySerializationState_ServicesArray_NameSeparator,

    kHAPIPAccessorySerializationState_ServicesArray_Begin,
    kHAPIPAccessorySerializationState_ServicesArray_End,

    kHAPIPAccessorySerializationState_ServiceObject_Begin,
    kHAPIPAccessorySerializationState_ServiceObject_End,
    kHAPIPAccessorySerializationState_ServiceObject_Separator,

    kHAPIPAccessorySerializationState_ServiceID_Name,
    kHAPIPAccessorySerializationState_ServiceID_NameSeparator,
    kHAPIPAccessorySerializationState_ServiceID_Value,
    kHAPIPAccessorySerializationState_ServiceID_ValueSeparator,

    kHAPIPAccessorySerializationState_ServiceType_Name,
    kHAPIPAccessorySerializationState_ServiceType_NameSeparator,
    kHAPIPAccessorySerializationState_ServiceType_Value,
    kHAPIPAccessorySerializationState_ServiceType_ValueSeparator,

    kHAPIPAccessorySerializationState_ServicePropertyPrimary_Name,
    kHAPIPAccessorySerializationState_ServicePropertyPrimary_NameSeparator,
    kHAPIPAccessorySerializationState_ServicePropertyPrimary_Value,
    kHAPIPAccessorySerializationState_ServicePropertyPrimary_ValueSeparator,

    kHAPIPAccessorySerializationState_ServicePropertyHidden_Name,
    kHAPIPAccessorySerializationState_ServicePropertyHidden_NameSeparator,
    kHAPIPAccessorySerializationState_ServicePropertyHidden_Value,
    kHAPIPAccessorySerializationState_ServicePropertyHidden_ValueSeparator,

    kHAPIPAccessorySerializationState_LinkedServicesArray_Name,
    kHAPIPAccessorySerializationState_LinkedServicesArray_NameSeparator,

    kHAPIPAccessorySerializationState_LinkedServicesArray_Begin,
    kHAPIPAccessorySerializationState_LinkedServicesArray_End,
    kHAPIPAccessorySerializationState_LinkedServicesArray_ValueSeparator,

    kHAPIPAccessorySerializationState_LinkedServiceID_Value,
    kHAPIPAccessorySerializationState_LinkedServiceID_Separator,

    kHAPIPAccessorySerializationState_CharacteristicsArray_Name,
    kHAPIPAccessorySerializationState_CharacteristicsArray_NameSeparator,

    kHAPIPAccessorySerializationState_CharacteristicsArray_Begin,
    kHAPIPAccessorySerializationState_CharacteristicsArray_End,

    kHAPIPAccessorySerializationState_CharacteristicsObject_Begin,
    kHAPIPAccessorySerializationState_CharacteristicObject_End,
    kHAPIPAccessorySerializationState_CharacteristicObject_Separator,

    kHAPIPAccessorySerializationState_CharacteristicID_Name,
    kHAPIPAccessorySerializationState_CharacteristicID_NameSeparator,
    kHAPIPAccessorySerializationState_CharacteristicID_Value,
    kHAPIPAccessorySerializationState_CharacteristicID_ValueSeparator,

    kHAPIPAccessorySerializationState_CharacteristicType_Name,
    kHAPIPAccessorySerializationState_CharacteristicType_NameSeparator,
    kHAPIPAccessorySerializationState_CharacteristicType_Value,
    kHAPIPAccessorySerializationState_CharacteristicType_ValueSeparator,

    kHAPIPAccessorySerializationState_CharacteristicFormat_Name,
    kHAPIPAccessorySerializationState_CharacteristicFormat_NameSeparator,
    kHAPIPAccessorySerializationState_CharacteristicFormat_Value,
    kHAPIPAccessorySerializationState_CharacteristicFormat_ValueSeparator,

    kHAPIPAccessorySerializationState_CharacteristicValue_Name,
    kHAPIPAccessorySerializationState_CharacteristicValue_NameSeparator,
    kHAPIPAccessorySerializationState_CharacteristicValue_Value,
    kHAPIPAccessorySerializationState_CharacteristicValue_ValueSeparator,

    kHAPIPAccessorySerializationState_CharacteristicPermissionsArray_Name,
    kHAPIPAccessorySerializationState_CharacteristicPermissionsArray_NameSeparator,

    kHAPIPAccessorySerializationState_CharacteristicPermissionsArray_Begin,
    kHAPIPAccessorySerializationState_CharacteristicPermissionsArray_End,
    kHAPIPAccessorySerializationState_CharacteristicPermissionsArray_ValueSeparator,

    kHAPIPAccessorySerializationState_CharacteristicPermission_Value,
    kHAPIPAccessorySerializationState_CharacteristicPermission_Separator,

    kHAPIPAccessorySerializationState_CharacteristicEventNotifications_Name,
    kHAPIPAccessorySerializationState_CharacteristicEventNotifications_NameSeparator,
    kHAPIPAccessorySerializationState_CharacteristicEventNotifications_Value,
    kHAPIPAccessorySerializationState_CharacteristicEventNotifications_ValueSeparator,
    kHAPIPAccessorySerializationState_CharacteristicEventNotificationContextData_Name,
    kHAPIPAccessorySerializationState_CharacteristicEventNotificationContextData_NameSeparator,
    kHAPIPAccessorySerializationState_CharacteristicEventNotificationContextData_Value,
    kHAPIPAccessorySerializationState_CharacteristicEventNotificationContextData_ValueSeparator,
    kHAPIPAccessorySerializationState_CharacteristicDescription_Name,
    kHAPIPAccessorySerializationState_CharacteristicDescription_NameSeparator,
    kHAPIPAccessorySerializationState_CharacteristicDescription_Value,
    kHAPIPAccessorySerializationState_CharacteristicDescription_ValueSeparator,

    kHAPIPAccessorySerializationState_CharacteristicUnit_Name,
    kHAPIPAccessorySerializationState_CharacteristicUnit_NameSeparator,
    kHAPIPAccessorySerializationState_CharacteristicUnit_Value,
    kHAPIPAccessorySerializationState_CharacteristicUnit_ValueSeparator,

    kHAPIPAccessorySerializationState_CharacteristicMinimumValue_Name,
    kHAPIPAccessorySerializationState_CharacteristicMinimumValue_NameSeparator,
    kHAPIPAccessorySerializationState_CharacteristicMinimumValue_Value,
    kHAPIPAccessorySerializationState_CharacteristicMinimumValue_ValueSeparator,

    kHAPIPAccessorySerializationState_CharacteristicMaximumValue_Name,
    kHAPIPAccessorySerializationState_CharacteristicMaximumValue_NameSeparator,
    kHAPIPAccessorySerializationState_CharacteristicMaximumValue_Value,
    kHAPIPAccessorySerializationState_CharacteristicMaximumValue_ValueSeparator,

    kHAPIPAccessorySerializationState_CharacteristicStepValue_Name,
    kHAPIPAccessorySerializationState_CharacteristicStepValue_NameSeparator,
    kHAPIPAccessorySerializationState_CharacteristicStepValue_Value,
    kHAPIPAccessorySerializationState_CharacteristicStepValue_ValueSeparator,

    kHAPIPAccessorySerializationState_CharacteristicMaxLength_Name,
    kHAPIPAccessorySerializationState_CharacteristicMaxLength_NameSeparator,
    kHAPIPAccessorySerializationState_CharacteristicMaxLength_Value,

    kHAPIPAccessorySerializationState_CharacteristicMaxDataLength_Name,
    kHAPIPAccessorySerializationState_CharacteristicMaxDataLength_NameSeparator,
    kHAPIPAccessorySerializationState_CharacteristicMaxDataLength_Value,

    kHAPIPAccessorySerializationState_CharacteristicValidValuesArray_Name,
    kHAPIPAccessorySerializationState_CharacteristicValidValuesArray_NameSeparator,

    kHAPIPAccessorySerializationState_CharacteristicValidValuesArray_Begin,
    kHAPIPAccessorySerializationState_CharacteristicValidValuesArray_End,
    kHAPIPAccessorySerializationState_CharacteristicValidValuesArray_ValueSeparator,

    kHAPIPAccessorySerializationState_CharacteristicValidValue_Value,
    kHAPIPAccessorySerializationState_CharacteristicValidValue_Separator,

    kHAPIPAccessorySerializationState_CharacteristicValidValuesRangesArray_Name,
    kHAPIPAccessorySerializationState_CharacteristicValidValuesRangesArray_NameSeparator,

    kHAPIPAccessorySerializationState_CharacteristicValidValuesRangesArray_Begin,
    kHAPIPAccessorySerializationState_CharacteristicValidValuesRangesArray_End,

    kHAPIPAccessorySerializationState_CharacteristicValidValuesRangeArray_Begin,
    kHAPIPAccessorySerializationState_CharacteristicValidValuesRangeArray_End,
    kHAPIPAccessorySerializationState_CharacteristicValidValuesRangeArray_Separator,

    kHAPIPAccessorySerializationState_CharacteristicValidValuesRangeStart_Value,
    kHAPIPAccessorySerializationState_CharacteristicValidValuesRangeEnd_Value,
    kHAPIPAccessorySerializationState_CharacteristicValidValuesRange_Separator,

    kHAPIPAccessorySerializationState_ResponseIsComplete
} HAP_ENUM_END(uint8_t, HAPIPAccessorySerializationState);

void HAPIPAccessoryCreateSerializationContext(HAPIPAccessorySerializationContext* context) {
    HAPPrecondition(context);

    HAPRawBufferZero(context, sizeof *context);
}

HAP_RESULT_USE_CHECK
bool HAPIPAccessorySerializationIsComplete(HAPIPAccessorySerializationContext* context) {
    HAPPrecondition(context);

    return context->state == kHAPIPAccessorySerializationState_ResponseIsComplete;
}

/**
 * Gets the current accessory in the given serialization context.
 *
 * @param      context              Serialization context.
 * @param      server              Accessory server.
 *
 * @return Current accessory or NULL if all accessories have been serialized.
 */
HAP_RESULT_USE_CHECK
static const HAPAccessory* _Nullable GetCurrentAccessory(
        HAPIPAccessorySerializationContext* context,
        HAPAccessoryServer* server) {
    HAPPrecondition(context);
    HAPPrecondition(server);
    HAPPrecondition(server->primaryAccessory);

    return context->accessoryIndex == 0 ?
                   server->primaryAccessory :
                   (server->ip.bridgedAccessories ? server->ip.bridgedAccessories[context->accessoryIndex - 1] : NULL);
}

/**
 * Gets the current service in the given serialization context.
 *
 * @param      context              Serialization context.
 * @param      server               Accessory server.
 *
 * @return Current service or NULL if all services of the current accessory have been serialized.
 */
HAP_RESULT_USE_CHECK
static const HAPService* _Nullable GetCurrentService(
        HAPIPAccessorySerializationContext* context,
        HAPAccessoryServer* server) {
    HAPPrecondition(context);
    HAPPrecondition(server);

    const HAPAccessory* accessory = GetCurrentAccessory(context, server);
    HAPAssert(accessory);

    return accessory->services[context->serviceIndex];
}

/**
 * Gets the current characteristic in the given serialization context.
 *
 * @param      context              Serialization context.
 * @param      server               Accessory server.
 *
 * @return Current characteristic or NULL if all characteristics of the current service have been serialized.
 */
HAP_RESULT_USE_CHECK
static const HAPCharacteristic* _Nullable GetCurrentCharacteristic(
        HAPIPAccessorySerializationContext* context,
        HAPAccessoryServer* server) {
    HAPPrecondition(context);
    HAPPrecondition(server);

    const HAPService* service = GetCurrentService(context, server);
    HAPAssert(service);

    return service->characteristics[context->characteristicIndex];
}

HAP_RESULT_USE_CHECK
HAPError HAPIPAccessorySerializeReadResponse( // NOLINT(readability-function-size)
        HAPIPAccessorySerializationContext* context,
        HAPAccessoryServer* server,
        HAPIPSessionDescriptor* session,
        char* bytes,
        size_t minBytes,
        size_t maxBytes,
        size_t* numBytes) {
    HAPPrecondition(context);
    HAPPrecondition(context->state != kHAPIPAccessorySerializationState_ResponseIsComplete);
    HAPPrecondition(server);
    HAPPrecondition(server->primaryAccessory);
    HAPPrecondition(session);
    HAPPrecondition(bytes);
    HAPPrecondition(minBytes >= 1);
    HAPPrecondition(maxBytes >= minBytes);
    HAPPrecondition(numBytes);

    HAPError err;

    // See HomeKit Accessory Protocol Specification R17
    // Section 6.3 HAP Objects

    // See HomeKit Accessory Protocol Specification R17
    // Section 6.6.4 Example Accessory Attribute Database in JSON

    // For the JSON Data Interchange Format, see RFC 7159.
    // http://www.rfc-editor.org/rfc/rfc7159.txt

    char scratchBytes[64];

#define GET_CURRENT_ACCESSORY() GetCurrentAccessory(context, server)

#define GET_CURRENT_SERVICE() GetCurrentService(context, server)

#define GET_CURRENT_CHARACTERISTIC() ((const HAPBaseCharacteristic*) GetCurrentCharacteristic(context, server))

#define APPEND_STRING_OR_RETURN_ERROR(string) \
    do { \
        HAPAssert(*numBytes <= maxBytes); \
        size_t numStringBytes = HAPStringGetNumBytes(string); \
        if (maxBytes - *numBytes < numStringBytes) { \
            HAPLogError(&logObject, "Not enough resources to serialize GET /accessories response."); \
            return kHAPError_OutOfResources; \
        } \
        HAPRawBufferCopyBytes(&bytes[*numBytes], (string), numStringBytes); \
        *numBytes += numStringBytes; \
        HAPAssert(*numBytes <= maxBytes); \
    } while (0)

#define APPEND_UUID_OR_RETURN_ERROR(uuid) \
    do { \
        HAPAssert(*numBytes <= maxBytes); \
        HAPAssert(sizeof scratchBytes >= 2); \
        err = HAPUUIDGetDescription((uuid), &scratchBytes[1], sizeof scratchBytes - 2); \
        HAPAssert(!err); \
        size_t numScratchBytes = HAPStringGetNumBytes(&scratchBytes[1]) + 2; \
        scratchBytes[0] = '"'; \
        scratchBytes[numScratchBytes - 1] = '"'; \
        if (maxBytes - *numBytes < numScratchBytes) { \
            HAPLogError(&logObject, "Not enough resources to serialize GET /accessories response."); \
            return kHAPError_OutOfResources; \
        } \
        HAPRawBufferCopyBytes(&bytes[*numBytes], scratchBytes, numScratchBytes); \
        *numBytes += numScratchBytes; \
        HAPAssert(*numBytes <= maxBytes); \
    } while (0)

#define APPEND_UINT64_OR_RETURN_ERROR(value) \
    do { \
        HAPAssert(*numBytes <= maxBytes); \
        err = HAPUInt64GetDescription((value), scratchBytes, sizeof scratchBytes); \
        HAPAssert(!err); \
        size_t numScratchBytes = HAPStringGetNumBytes(scratchBytes); \
        if (maxBytes - *numBytes < numScratchBytes) { \
            HAPLogError(&logObject, "Not enough resources to serialize GET /accessories response."); \
            return kHAPError_OutOfResources; \
        } \
        HAPRawBufferCopyBytes(&bytes[*numBytes], scratchBytes, numScratchBytes); \
        *numBytes += numScratchBytes; \
        HAPAssert(*numBytes <= maxBytes); \
    } while (0)

#define APPEND_INT32_OR_RETURN_ERROR(value) \
    do { \
        HAPAssert(*numBytes <= maxBytes); \
        err = HAPStringWithFormat(scratchBytes, sizeof scratchBytes, "%ld", (long) (value)); \
        HAPAssert(!err); \
        size_t numScratchBytes = HAPStringGetNumBytes(scratchBytes); \
        if (maxBytes - *numBytes < numScratchBytes) { \
            HAPLogError(&logObject, "Not enough resources to serialize GET /accessories response."); \
            return kHAPError_OutOfResources; \
        } \
        HAPRawBufferCopyBytes(&bytes[*numBytes], scratchBytes, numScratchBytes); \
        *numBytes += numScratchBytes; \
        HAPAssert(*numBytes <= maxBytes); \
    } while (0)

#define APPEND_FLOAT_OR_RETURN_ERROR(value) \
    do { \
        HAPAssert(*numBytes <= maxBytes); \
        err = HAPJSONUtilsGetFloatDescription((value), scratchBytes, sizeof scratchBytes); \
        HAPAssert(!err); \
        size_t numScratchBytes = HAPStringGetNumBytes(scratchBytes); \
        if (maxBytes - *numBytes < numScratchBytes) { \
            HAPLogError(&logObject, "Not enough resources to serialize GET /accessories response."); \
            return kHAPError_OutOfResources; \
        } \
        HAPRawBufferCopyBytes(&bytes[*numBytes], scratchBytes, numScratchBytes); \
        *numBytes += numScratchBytes; \
        HAPAssert(*numBytes <= maxBytes); \
    } while (0)

    *numBytes = 0;

    do {
        HAPAssert(sizeof context->state == sizeof(HAPIPAccessorySerializationState));
        switch ((HAPIPAccessorySerializationState) context->state) {
            case kHAPIPAccessorySerializationState_ResponseObject_Begin: {
                APPEND_STRING_OR_RETURN_ERROR("{");
                context->state = kHAPIPAccessorySerializationState_AccessoriesArray_Name;
            }
                continue;
            case kHAPIPAccessorySerializationState_ResponseObject_End: {
                APPEND_STRING_OR_RETURN_ERROR("}");
                context->state = kHAPIPAccessorySerializationState_ResponseIsComplete;
            }
                continue;
            case kHAPIPAccessorySerializationState_AccessoriesArray_Name: {
                APPEND_STRING_OR_RETURN_ERROR("\"accessories\"");
                context->state = kHAPIPAccessorySerializationState_AccessoriesArray_NameSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_AccessoriesArray_NameSeparator: {
                APPEND_STRING_OR_RETURN_ERROR(":");
                context->state = kHAPIPAccessorySerializationState_AccessoriesArray_Begin;
            }
                continue;
            case kHAPIPAccessorySerializationState_AccessoriesArray_Begin: {
                APPEND_STRING_OR_RETURN_ERROR("[");
                context->accessoryIndex = 0;
                context->state = kHAPIPAccessorySerializationState_AccessoryObject_Begin;
            }
                continue;
            case kHAPIPAccessorySerializationState_AccessoriesArray_End: {
                APPEND_STRING_OR_RETURN_ERROR("]");
                context->state = kHAPIPAccessorySerializationState_ResponseObject_End;
            }
                continue;
            case kHAPIPAccessorySerializationState_AccessoryObject_Begin: {
                APPEND_STRING_OR_RETURN_ERROR("{");
                context->state = kHAPIPAccessorySerializationState_AccessoryID_Name;
            }
                continue;
            case kHAPIPAccessorySerializationState_AccessoryObject_End: {
                APPEND_STRING_OR_RETURN_ERROR("}");
                HAPAssert(context->accessoryIndex < UINT8_MAX);
                context->accessoryIndex++;
                if (context->accessoryIndex == 1) {
                    if (server->ip.bridgedAccessories && server->ip.bridgedAccessories[0]) {
                        context->state = kHAPIPAccessorySerializationState_AccessoryObject_Separator;
                    } else {
                        context->state = kHAPIPAccessorySerializationState_AccessoriesArray_End;
                    }
                } else {
                    HAPAssert(server->ip.bridgedAccessories);
                    if (server->ip.bridgedAccessories[context->accessoryIndex - 1]) {
                        context->state = kHAPIPAccessorySerializationState_AccessoryObject_Separator;
                    } else {
                        context->state = kHAPIPAccessorySerializationState_AccessoriesArray_End;
                    }
                }
            }
                continue;
            case kHAPIPAccessorySerializationState_AccessoryObject_Separator: {
                APPEND_STRING_OR_RETURN_ERROR(",");
                context->state = kHAPIPAccessorySerializationState_AccessoryObject_Begin;
            }
                continue;
            case kHAPIPAccessorySerializationState_AccessoryID_Name: {
                APPEND_STRING_OR_RETURN_ERROR("\"aid\"");
                context->state = kHAPIPAccessorySerializationState_AccessoryID_NameSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_AccessoryID_NameSeparator: {
                APPEND_STRING_OR_RETURN_ERROR(":");
                context->state = kHAPIPAccessorySerializationState_AccessoryID_Value;
            }
                continue;
            case kHAPIPAccessorySerializationState_AccessoryID_Value: {
                const HAPAccessory* accessory = GET_CURRENT_ACCESSORY();
                HAPAssert(accessory);
                APPEND_UINT64_OR_RETURN_ERROR(accessory->aid);
                context->state = kHAPIPAccessorySerializationState_AccessoryID_ValueSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_AccessoryID_ValueSeparator: {
                APPEND_STRING_OR_RETURN_ERROR(",");
                context->state = kHAPIPAccessorySerializationState_ServicesArray_Name;
            }
                continue;
            case kHAPIPAccessorySerializationState_ServicesArray_Name: {
                APPEND_STRING_OR_RETURN_ERROR("\"services\"");
                context->state = kHAPIPAccessorySerializationState_ServicesArray_NameSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_ServicesArray_NameSeparator: {
                APPEND_STRING_OR_RETURN_ERROR(":");
                context->state = kHAPIPAccessorySerializationState_ServicesArray_Begin;
            }
                continue;
            case kHAPIPAccessorySerializationState_ServicesArray_Begin: {
                APPEND_STRING_OR_RETURN_ERROR("[");
                const HAPAccessory* accessory = GET_CURRENT_ACCESSORY();
                HAPAssert(accessory);
                const HAPService* const* services = accessory->services;
                HAPAssert(services);
                context->serviceIndex = 0;
                const HAPService* service = services[context->serviceIndex];
                while (service && !HAPAccessoryServerSupportsService(server, kHAPTransportType_IP, service)) {
                    HAPAssert(context->serviceIndex < UINT8_MAX);
                    context->serviceIndex++;
                    service = services[context->serviceIndex];
                }
                if (service) {
                    context->state = kHAPIPAccessorySerializationState_ServiceObject_Begin;
                } else {
                    context->state = kHAPIPAccessorySerializationState_ServicesArray_End;
                }
            }
                continue;
            case kHAPIPAccessorySerializationState_ServicesArray_End: {
                APPEND_STRING_OR_RETURN_ERROR("]");
                context->state = kHAPIPAccessorySerializationState_AccessoryObject_End;
            }
                continue;
            case kHAPIPAccessorySerializationState_ServiceObject_Begin: {
                APPEND_STRING_OR_RETURN_ERROR("{");
                context->state = kHAPIPAccessorySerializationState_ServiceID_Name;
            }
                continue;
            case kHAPIPAccessorySerializationState_ServiceObject_End: {
                APPEND_STRING_OR_RETURN_ERROR("}");
                const HAPAccessory* accessory = GET_CURRENT_ACCESSORY();
                HAPAssert(accessory);
                const HAPService* const* services = accessory->services;
                HAPAssert(services);
                const HAPService* service;
                do {
                    HAPAssert(context->serviceIndex < UINT8_MAX);
                    context->serviceIndex++;
                    service = services[context->serviceIndex];
                } while (service && !HAPAccessoryServerSupportsService(server, kHAPTransportType_IP, service));
                if (service) {
                    context->state = kHAPIPAccessorySerializationState_ServiceObject_Separator;
                } else {
                    context->state = kHAPIPAccessorySerializationState_ServicesArray_End;
                }
            }
                continue;
            case kHAPIPAccessorySerializationState_ServiceObject_Separator: {
                APPEND_STRING_OR_RETURN_ERROR(",");
                context->state = kHAPIPAccessorySerializationState_ServiceObject_Begin;
            }
                continue;
            case kHAPIPAccessorySerializationState_ServiceID_Name: {
                APPEND_STRING_OR_RETURN_ERROR("\"iid\"");
                context->state = kHAPIPAccessorySerializationState_ServiceID_NameSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_ServiceID_NameSeparator: {
                APPEND_STRING_OR_RETURN_ERROR(":");
                context->state = kHAPIPAccessorySerializationState_ServiceID_Value;
            }
                continue;
            case kHAPIPAccessorySerializationState_ServiceID_Value: {
                const HAPService* service = GET_CURRENT_SERVICE();
                HAPAssert(service);
                APPEND_UINT64_OR_RETURN_ERROR(service->iid);
                context->state = kHAPIPAccessorySerializationState_ServiceID_ValueSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_ServiceID_ValueSeparator: {
                APPEND_STRING_OR_RETURN_ERROR(",");
                context->state = kHAPIPAccessorySerializationState_ServiceType_Name;
            }
                continue;
            case kHAPIPAccessorySerializationState_ServiceType_Name: {
                APPEND_STRING_OR_RETURN_ERROR("\"type\"");
                context->state = kHAPIPAccessorySerializationState_ServiceType_NameSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_ServiceType_NameSeparator: {
                APPEND_STRING_OR_RETURN_ERROR(":");
                context->state = kHAPIPAccessorySerializationState_ServiceType_Value;
            }
                continue;
            case kHAPIPAccessorySerializationState_ServiceType_Value: {
                const HAPService* service = GET_CURRENT_SERVICE();
                HAPAssert(service);
                APPEND_UUID_OR_RETURN_ERROR(service->serviceType);
                context->state = kHAPIPAccessorySerializationState_ServiceType_ValueSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_ServiceType_ValueSeparator: {
                APPEND_STRING_OR_RETURN_ERROR(",");
                context->state = kHAPIPAccessorySerializationState_ServicePropertyPrimary_Name;
            }
                continue;
            case kHAPIPAccessorySerializationState_ServicePropertyPrimary_Name: {
                APPEND_STRING_OR_RETURN_ERROR("\"primary\"");
                context->state = kHAPIPAccessorySerializationState_ServicePropertyPrimary_NameSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_ServicePropertyPrimary_NameSeparator: {
                APPEND_STRING_OR_RETURN_ERROR(":");
                context->state = kHAPIPAccessorySerializationState_ServicePropertyPrimary_Value;
            }
                continue;
            case kHAPIPAccessorySerializationState_ServicePropertyPrimary_Value: {
                const HAPService* service = GET_CURRENT_SERVICE();
                HAPAssert(service);
                APPEND_STRING_OR_RETURN_ERROR(service->properties.primaryService ? "true" : "false");
                context->state = kHAPIPAccessorySerializationState_ServicePropertyPrimary_ValueSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_ServicePropertyPrimary_ValueSeparator: {
                APPEND_STRING_OR_RETURN_ERROR(",");
                context->state = kHAPIPAccessorySerializationState_ServicePropertyHidden_Name;
            }
                continue;
            case kHAPIPAccessorySerializationState_ServicePropertyHidden_Name: {
                APPEND_STRING_OR_RETURN_ERROR("\"hidden\"");
                context->state = kHAPIPAccessorySerializationState_ServicePropertyHidden_NameSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_ServicePropertyHidden_NameSeparator: {
                APPEND_STRING_OR_RETURN_ERROR(":");
                context->state = kHAPIPAccessorySerializationState_ServicePropertyHidden_Value;
            }
                continue;
            case kHAPIPAccessorySerializationState_ServicePropertyHidden_Value: {
                const HAPService* service = GET_CURRENT_SERVICE();
                HAPAssert(service);
                APPEND_STRING_OR_RETURN_ERROR(service->properties.hidden ? "true" : "false");
                context->state = kHAPIPAccessorySerializationState_ServicePropertyHidden_ValueSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_ServicePropertyHidden_ValueSeparator: {
                APPEND_STRING_OR_RETURN_ERROR(",");
                const HAPService* service = GET_CURRENT_SERVICE();
                HAPAssert(service);
                const uint16_t* linkedServices = service->linkedServices;
                if (linkedServices) {
                    context->index = 0;
                    if (linkedServices[context->index]) {
                        context->state = kHAPIPAccessorySerializationState_LinkedServicesArray_Name;
                    } else {
                        context->state = kHAPIPAccessorySerializationState_CharacteristicsArray_Name;
                    }
                } else {
                    context->state = kHAPIPAccessorySerializationState_CharacteristicsArray_Name;
                }
            }
                continue;
            case kHAPIPAccessorySerializationState_LinkedServicesArray_Name: {
                APPEND_STRING_OR_RETURN_ERROR("\"linked\"");
                context->state = kHAPIPAccessorySerializationState_LinkedServicesArray_NameSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_LinkedServicesArray_NameSeparator: {
                APPEND_STRING_OR_RETURN_ERROR(":");
                context->state = kHAPIPAccessorySerializationState_LinkedServicesArray_Begin;
            }
                continue;
            case kHAPIPAccessorySerializationState_LinkedServicesArray_Begin: {
                APPEND_STRING_OR_RETURN_ERROR("[");
                context->state = kHAPIPAccessorySerializationState_LinkedServiceID_Value;
            }
                continue;
            case kHAPIPAccessorySerializationState_LinkedServicesArray_End: {
                APPEND_STRING_OR_RETURN_ERROR("]");
                context->state = kHAPIPAccessorySerializationState_LinkedServicesArray_ValueSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_LinkedServicesArray_ValueSeparator: {
                APPEND_STRING_OR_RETURN_ERROR(",");
                context->state = kHAPIPAccessorySerializationState_CharacteristicsArray_Name;
            }
                continue;
            case kHAPIPAccessorySerializationState_LinkedServiceID_Value: {
                const HAPService* service = GET_CURRENT_SERVICE();
                HAPAssert(service);
                const uint16_t* linkedServices = service->linkedServices;
                HAPAssert(linkedServices);
                HAPAssert(linkedServices[context->index]);
                APPEND_UINT64_OR_RETURN_ERROR(linkedServices[context->index]);
                HAPAssert(context->index < UINT8_MAX);
                context->index++;
                if (linkedServices[context->index]) {
                    context->state = kHAPIPAccessorySerializationState_LinkedServiceID_Separator;
                } else {
                    context->state = kHAPIPAccessorySerializationState_LinkedServicesArray_End;
                }
            }
                continue;
            case kHAPIPAccessorySerializationState_LinkedServiceID_Separator: {
                APPEND_STRING_OR_RETURN_ERROR(",");
                context->state = kHAPIPAccessorySerializationState_LinkedServiceID_Value;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicsArray_Name: {
                APPEND_STRING_OR_RETURN_ERROR("\"characteristics\"");
                context->state = kHAPIPAccessorySerializationState_CharacteristicsArray_NameSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicsArray_NameSeparator: {
                APPEND_STRING_OR_RETURN_ERROR(":");
                context->state = kHAPIPAccessorySerializationState_CharacteristicsArray_Begin;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicsArray_Begin: {
                APPEND_STRING_OR_RETURN_ERROR("[");
                const HAPService* service = GET_CURRENT_SERVICE();
                HAPAssert(service);
                const HAPCharacteristic* const* characteristics = service->characteristics;
                HAPAssert(characteristics);
                context->characteristicIndex = 0;
                const HAPBaseCharacteristic* characteristic = characteristics[context->characteristicIndex];
                while (characteristic && !HAPIPCharacteristicIsSupported(characteristic)) {
                    HAPAssert(context->characteristicIndex < UINT8_MAX);
                    context->characteristicIndex++;
                    characteristic = characteristics[context->characteristicIndex];
                }
                if (characteristic) {
                    context->state = kHAPIPAccessorySerializationState_CharacteristicsObject_Begin;
                } else {
                    context->state = kHAPIPAccessorySerializationState_CharacteristicsArray_End;
                }
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicsArray_End: {
                APPEND_STRING_OR_RETURN_ERROR("]");
                context->state = kHAPIPAccessorySerializationState_ServiceObject_End;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicsObject_Begin: {
                APPEND_STRING_OR_RETURN_ERROR("{");
                context->state = kHAPIPAccessorySerializationState_CharacteristicID_Name;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicObject_End: {
                APPEND_STRING_OR_RETURN_ERROR("}");
                const HAPService* service = GET_CURRENT_SERVICE();
                HAPAssert(service);
                const HAPCharacteristic* const* characteristics = service->characteristics;
                HAPAssert(characteristics);
                const HAPBaseCharacteristic* characteristic;
                do {
                    HAPAssert(context->characteristicIndex < UINT8_MAX);
                    context->characteristicIndex++;
                    characteristic = characteristics[context->characteristicIndex];
                } while (characteristic && !HAPIPCharacteristicIsSupported(characteristic));
                if (characteristic) {
                    context->state = kHAPIPAccessorySerializationState_CharacteristicObject_Separator;
                } else {
                    context->state = kHAPIPAccessorySerializationState_CharacteristicsArray_End;
                }
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicObject_Separator: {
                APPEND_STRING_OR_RETURN_ERROR(",");
                context->state = kHAPIPAccessorySerializationState_CharacteristicsObject_Begin;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicID_Name: {
                APPEND_STRING_OR_RETURN_ERROR("\"iid\"");
                context->state = kHAPIPAccessorySerializationState_CharacteristicID_NameSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicID_NameSeparator: {
                APPEND_STRING_OR_RETURN_ERROR(":");
                context->state = kHAPIPAccessorySerializationState_CharacteristicID_Value;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicID_Value: {
                const HAPBaseCharacteristic* baseCharacteristic = GET_CURRENT_CHARACTERISTIC();
                HAPAssert(baseCharacteristic);
                APPEND_UINT64_OR_RETURN_ERROR(baseCharacteristic->iid);
                context->state = kHAPIPAccessorySerializationState_CharacteristicID_ValueSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicID_ValueSeparator: {
                APPEND_STRING_OR_RETURN_ERROR(",");
                context->state = kHAPIPAccessorySerializationState_CharacteristicType_Name;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicType_Name: {
                APPEND_STRING_OR_RETURN_ERROR("\"type\"");
                context->state = kHAPIPAccessorySerializationState_CharacteristicType_NameSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicType_NameSeparator: {
                APPEND_STRING_OR_RETURN_ERROR(":");
                context->state = kHAPIPAccessorySerializationState_CharacteristicType_Value;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicType_Value: {
                const HAPBaseCharacteristic* baseCharacteristic = GET_CURRENT_CHARACTERISTIC();
                HAPAssert(baseCharacteristic);
                APPEND_UUID_OR_RETURN_ERROR(baseCharacteristic->characteristicType);
                context->state = kHAPIPAccessorySerializationState_CharacteristicType_ValueSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicType_ValueSeparator: {
                APPEND_STRING_OR_RETURN_ERROR(",");
                context->state = kHAPIPAccessorySerializationState_CharacteristicFormat_Name;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicFormat_Name: {
                APPEND_STRING_OR_RETURN_ERROR("\"format\"");
                context->state = kHAPIPAccessorySerializationState_CharacteristicFormat_NameSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicFormat_NameSeparator: {
                APPEND_STRING_OR_RETURN_ERROR(":");
                context->state = kHAPIPAccessorySerializationState_CharacteristicFormat_Value;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicFormat_Value: {
                const HAPBaseCharacteristic* baseCharacteristic = GET_CURRENT_CHARACTERISTIC();
                HAPAssert(baseCharacteristic);
                switch (baseCharacteristic->format) {
                    case kHAPCharacteristicFormat_Bool: {
                        APPEND_STRING_OR_RETURN_ERROR("\"bool\"");
                        break;
                    }
                    case kHAPCharacteristicFormat_UInt8: {
                        APPEND_STRING_OR_RETURN_ERROR("\"uint8\"");
                        break;
                    }
                    case kHAPCharacteristicFormat_UInt16: {
                        APPEND_STRING_OR_RETURN_ERROR("\"uint16\"");
                        break;
                    }
                    case kHAPCharacteristicFormat_UInt32: {
                        APPEND_STRING_OR_RETURN_ERROR("\"uint32\"");
                        break;
                    }
                    case kHAPCharacteristicFormat_UInt64: {
                        APPEND_STRING_OR_RETURN_ERROR("\"uint64\"");
                        break;
                    }
                    case kHAPCharacteristicFormat_Int: {
                        APPEND_STRING_OR_RETURN_ERROR("\"int\"");
                        break;
                    }
                    case kHAPCharacteristicFormat_Float: {
                        APPEND_STRING_OR_RETURN_ERROR("\"float\"");
                        break;
                    }
                    case kHAPCharacteristicFormat_String: {
                        APPEND_STRING_OR_RETURN_ERROR("\"string\"");
                        break;
                    }
                    case kHAPCharacteristicFormat_TLV8: {
                        APPEND_STRING_OR_RETURN_ERROR("\"tlv8\"");
                        break;
                    }
                    case kHAPCharacteristicFormat_Data: {
                        APPEND_STRING_OR_RETURN_ERROR("\"data\"");
                        break;
                    }
                }
                context->state = kHAPIPAccessorySerializationState_CharacteristicFormat_ValueSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicFormat_ValueSeparator: {
                APPEND_STRING_OR_RETURN_ERROR(",");
                const HAPBaseCharacteristic* baseCharacteristic = GET_CURRENT_CHARACTERISTIC();
                HAPAssert(baseCharacteristic);
                if (baseCharacteristic->properties.readable) {
                    context->state = kHAPIPAccessorySerializationState_CharacteristicValue_Name;
                } else {
                    context->state = kHAPIPAccessorySerializationState_CharacteristicPermissionsArray_Name;
                }
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicValue_Name: {
                APPEND_STRING_OR_RETURN_ERROR("\"value\"");
                context->state = kHAPIPAccessorySerializationState_CharacteristicValue_NameSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicValue_NameSeparator: {
                APPEND_STRING_OR_RETURN_ERROR(":");
                context->state = kHAPIPAccessorySerializationState_CharacteristicValue_Value;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicValue_Value: {
                const HAPBaseCharacteristic* baseCharacteristic = GET_CURRENT_CHARACTERISTIC();
                HAPAssert(baseCharacteristic);
                HAPAssert(baseCharacteristic->properties.readable);
                HAPIPSessionReadResult readResult;

                HAPAssert(*numBytes <= maxBytes);
                if (maxBytes - *numBytes < 2) {
                    HAPLogError(&logObject, "Not enough resources to serialize GET /accessories response.");
                    return kHAPError_OutOfResources;
                }
                // Buffer 'bytes' has enough capacity to store at least an empty string including quotation marks.

                HAPIPByteBuffer dataBuffer;
                dataBuffer.data = &bytes[*numBytes + 1]; // Leave space for beginning quotation mark.
                dataBuffer.position = 0;
                dataBuffer.limit = maxBytes - *numBytes - 2; // Leave space for ending quotation mark.
                dataBuffer.capacity = dataBuffer.limit;
                HAPAssert(dataBuffer.data);
                HAPAssert(dataBuffer.position <= dataBuffer.limit);
                HAPAssert(dataBuffer.limit <= dataBuffer.capacity);

                const HAPAccessory* accessory = GET_CURRENT_ACCESSORY();
                HAPAssert(accessory);
                const HAPService* service = GET_CURRENT_SERVICE();
                HAPAssert(service);
                HAPIPSessionHandleReadRequest(
                        session,
                        kHAPIPSessionContext_GetAccessories,
                        baseCharacteristic,
                        service,
                        accessory,
                        &readResult,
                        &dataBuffer);
                if (HAPUUIDAreEqual(baseCharacteristic->characteristicType, &kHAPCharacteristicType_ButtonEvent)) {
                    // Workaround for 47267690: Error -70402 when reading from "Button Event" characteristic.
                    // The Button Event characteristic is event only.
                    HAPLogCharacteristicInfo(
                            &logObject,
                            baseCharacteristic,
                            service,
                            accessory,
                            "Sending null value (readHandler callback is only called for HAP events).");
                    APPEND_STRING_OR_RETURN_ERROR("null");
                } else if (HAPUUIDAreEqual(
                                   baseCharacteristic->characteristicType,
                                   &kHAPCharacteristicType_ProgrammableSwitchEvent)) {
                    // A read of this characteristic must always return a null value for IP accessories.
                    // See HomeKit Accessory Protocol Specification R17
                    // Section 11.47 Programmable Switch Event
                    HAPLogCharacteristicInfo(
                            &logObject,
                            baseCharacteristic,
                            service,
                            accessory,
                            "Sending null value (readHandler callback is only called for HAP events).");
                    APPEND_STRING_OR_RETURN_ERROR("null");
                } else if (
                        baseCharacteristic->properties.ip.controlPoint &&
                        (baseCharacteristic->format == kHAPCharacteristicFormat_TLV8)) {
                    APPEND_STRING_OR_RETURN_ERROR("\"\"");
                } else if (readResult.status != 0) {
                    if (baseCharacteristic->format == kHAPCharacteristicFormat_TLV8) {
                        HAPLogCharacteristicInfo(
                                &logObject,
                                baseCharacteristic,
                                service,
                                accessory,
                                "Read handler failed with error. Sending empty TLV value.");
                        APPEND_STRING_OR_RETURN_ERROR("\"\"");
                    } else {
                        APPEND_STRING_OR_RETURN_ERROR("null");
                    }
                } else {
                    switch (baseCharacteristic->format) {
                        case kHAPCharacteristicFormat_Bool: {
                            APPEND_STRING_OR_RETURN_ERROR(readResult.value.unsignedIntValue ? "1" : "0");
                            break;
                        }
                        case kHAPCharacteristicFormat_UInt8:
                        case kHAPCharacteristicFormat_UInt16:
                        case kHAPCharacteristicFormat_UInt32:
                        case kHAPCharacteristicFormat_UInt64: {
                            APPEND_UINT64_OR_RETURN_ERROR(readResult.value.unsignedIntValue);
                            break;
                        }
                        case kHAPCharacteristicFormat_Int: {
                            APPEND_INT32_OR_RETURN_ERROR(readResult.value.intValue);
                            break;
                        }
                        case kHAPCharacteristicFormat_Float: {
                            APPEND_FLOAT_OR_RETURN_ERROR(readResult.value.floatValue);
                            break;
                        }
                        case kHAPCharacteristicFormat_String:
                        case kHAPCharacteristicFormat_TLV8:
                        case kHAPCharacteristicFormat_Data: {
                            err = HAPJSONUtilsEscapeStringData(
                                    HAPNonnull(readResult.value.stringValue.bytes),
                                    dataBuffer.limit,
                                    &readResult.value.stringValue.numBytes);
                            if (err) {
                                HAPAssert(err == kHAPError_OutOfResources);
                                HAPLogError(&logObject, "Not enough resources to serialize GET /accessories response.");
                                return err;
                            }
                            bytes[*numBytes] = '"';
                            bytes[*numBytes + 1 + readResult.value.stringValue.numBytes] = '"';
                            *numBytes += 1 + readResult.value.stringValue.numBytes + 1;
                            break;
                        }
                    }
                }

                HAPAssert(*numBytes <= maxBytes);

                context->state = kHAPIPAccessorySerializationState_CharacteristicValue_ValueSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicValue_ValueSeparator: {
                APPEND_STRING_OR_RETURN_ERROR(",");
                context->state = kHAPIPAccessorySerializationState_CharacteristicPermissionsArray_Name;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicPermissionsArray_Name: {
                APPEND_STRING_OR_RETURN_ERROR("\"perms\"");
                context->state = kHAPIPAccessorySerializationState_CharacteristicPermissionsArray_NameSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicPermissionsArray_NameSeparator: {
                APPEND_STRING_OR_RETURN_ERROR(":");
                context->state = kHAPIPAccessorySerializationState_CharacteristicPermissionsArray_Begin;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicPermissionsArray_Begin: {
                const HAPBaseCharacteristic* baseCharacteristic = GET_CURRENT_CHARACTERISTIC();
                HAPAssert(baseCharacteristic);
                APPEND_STRING_OR_RETURN_ERROR("[");
                context->index = 0;
                if (context->index < HAPCharacteristicGetNumEnabledProperties(baseCharacteristic)) {
                    context->state = kHAPIPAccessorySerializationState_CharacteristicPermission_Value;
                } else {
                    context->state = kHAPIPAccessorySerializationState_CharacteristicPermissionsArray_End;
                }
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicPermissionsArray_End: {
                APPEND_STRING_OR_RETURN_ERROR("]");
                context->state = kHAPIPAccessorySerializationState_CharacteristicPermissionsArray_ValueSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicPermissionsArray_ValueSeparator: {
                const HAPBaseCharacteristic* baseCharacteristic = GET_CURRENT_CHARACTERISTIC();
                HAPAssert(baseCharacteristic);
                if (baseCharacteristic->properties.readable) {
                    APPEND_STRING_OR_RETURN_ERROR(",");
                    context->state = kHAPIPAccessorySerializationState_CharacteristicEventNotifications_Name;
                } else if (baseCharacteristic->manufacturerDescription) {
                    APPEND_STRING_OR_RETURN_ERROR(",");
                    context->state = kHAPIPAccessorySerializationState_CharacteristicDescription_Name;
                } else if (HAPCharacteristicGetUnit(baseCharacteristic) != kHAPCharacteristicUnits_None) {
                    APPEND_STRING_OR_RETURN_ERROR(",");
                    context->state = kHAPIPAccessorySerializationState_CharacteristicUnit_Name;
                } else {
                    context->state = kHAPIPAccessorySerializationState_CharacteristicUnit_ValueSeparator;
                }
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicPermission_Value: {
                const HAPBaseCharacteristic* baseCharacteristic = GET_CURRENT_CHARACTERISTIC();
                HAPAssert(baseCharacteristic);
                size_t numEnabledProperties = HAPCharacteristicGetNumEnabledProperties(baseCharacteristic);
                HAPAssert(context->index < numEnabledProperties);
                uint8_t i = 0;
                if (baseCharacteristic->properties.readable) {
                    if (i == context->index) {
                        APPEND_STRING_OR_RETURN_ERROR("\"pr\"");
                    }
                    i++;
                }
                if (baseCharacteristic->properties.writable) {
                    if (i == context->index) {
                        APPEND_STRING_OR_RETURN_ERROR("\"pw\"");
                    }
                    i++;
                }
                if (baseCharacteristic->properties.supportsEventNotification) {
                    if (i == context->index) {
                        APPEND_STRING_OR_RETURN_ERROR("\"ev\"");
                    }
                    i++;
                }
                if (baseCharacteristic->properties.supportsEventNotificationContextInformation) {
                    if (i == context->index) {
                        APPEND_STRING_OR_RETURN_ERROR("\"enc\"");
                    }
                    i++;
                }
                if (baseCharacteristic->properties.supportsAuthorizationData) {
                    if (i == context->index) {
                        APPEND_STRING_OR_RETURN_ERROR("\"aa\"");
                    }
                    i++;
                }
                if (baseCharacteristic->properties.requiresTimedWrite) {
                    if (i == context->index) {
                        APPEND_STRING_OR_RETURN_ERROR("\"tw\"");
                    }
                    i++;
                }
                if (baseCharacteristic->properties.ip.supportsWriteResponse) {
                    if (i == context->index) {
                        APPEND_STRING_OR_RETURN_ERROR("\"wr\"");
                    }
                    i++;
                }
                if (baseCharacteristic->properties.hidden) {
                    if (i == context->index) {
                        APPEND_STRING_OR_RETURN_ERROR("\"hd\"");
                    }
                    i++;
                }
                HAPAssert(i == numEnabledProperties);
                HAPAssert(context->index < UINT8_MAX);
                context->index++;
                if (context->index < numEnabledProperties) {
                    context->state = kHAPIPAccessorySerializationState_CharacteristicPermission_Separator;
                } else {
                    context->state = kHAPIPAccessorySerializationState_CharacteristicPermissionsArray_End;
                }
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicPermission_Separator: {
                APPEND_STRING_OR_RETURN_ERROR(",");
                context->state = kHAPIPAccessorySerializationState_CharacteristicPermission_Value;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicEventNotifications_Name: {
                APPEND_STRING_OR_RETURN_ERROR("\"ev\"");
                context->state = kHAPIPAccessorySerializationState_CharacteristicEventNotifications_NameSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicEventNotifications_NameSeparator: {
                APPEND_STRING_OR_RETURN_ERROR(":");
                context->state = kHAPIPAccessorySerializationState_CharacteristicEventNotifications_Value;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicEventNotifications_Value: {
                const HAPAccessory* accessory = GET_CURRENT_ACCESSORY();
                HAPAssert(accessory);
                const HAPService* service = GET_CURRENT_SERVICE();
                HAPAssert(service);
                const HAPBaseCharacteristic* baseCharacteristic = GET_CURRENT_CHARACTERISTIC();
                HAPAssert(baseCharacteristic);
                APPEND_STRING_OR_RETURN_ERROR(
                        HAPIPSessionAreEventNotificationsEnabled(session, baseCharacteristic, service, accessory) ?
                                "true" :
                                "false");
                context->state = kHAPIPAccessorySerializationState_CharacteristicEventNotifications_ValueSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicEventNotifications_ValueSeparator: {
                APPEND_STRING_OR_RETURN_ERROR(",");
                context->state = kHAPIPAccessorySerializationState_CharacteristicEventNotificationContextData_Name;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicEventNotificationContextData_Name: {
                APPEND_STRING_OR_RETURN_ERROR("\"enc\"");
                context->state =
                        kHAPIPAccessorySerializationState_CharacteristicEventNotificationContextData_NameSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicEventNotificationContextData_NameSeparator: {
                APPEND_STRING_OR_RETURN_ERROR(":");
                context->state = kHAPIPAccessorySerializationState_CharacteristicEventNotificationContextData_Value;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicEventNotificationContextData_Value: {
                const HAPAccessory* accessory = GET_CURRENT_ACCESSORY();
                HAPAssert(accessory);
                const HAPService* service = GET_CURRENT_SERVICE();
                HAPAssert(service);
                const HAPBaseCharacteristic* baseCharacteristic = GET_CURRENT_CHARACTERISTIC();
                HAPAssert(baseCharacteristic);
                APPEND_STRING_OR_RETURN_ERROR(
                        (baseCharacteristic->properties.supportsEventNotificationContextInformation == true) ? "true" :
                                                                                                               "false");
                context->state =
                        kHAPIPAccessorySerializationState_CharacteristicEventNotificationContextData_ValueSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicEventNotificationContextData_ValueSeparator: {
                const HAPBaseCharacteristic* baseCharacteristic = GET_CURRENT_CHARACTERISTIC();
                HAPAssert(baseCharacteristic);
                if (baseCharacteristic->manufacturerDescription) {
                    APPEND_STRING_OR_RETURN_ERROR(",");
                    context->state = kHAPIPAccessorySerializationState_CharacteristicDescription_Name;
                } else if (HAPCharacteristicGetUnit(baseCharacteristic) != kHAPCharacteristicUnits_None) {
                    APPEND_STRING_OR_RETURN_ERROR(",");
                    context->state = kHAPIPAccessorySerializationState_CharacteristicUnit_Name;
                } else {
                    context->state = kHAPIPAccessorySerializationState_CharacteristicUnit_ValueSeparator;
                }
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicDescription_Name: {
                APPEND_STRING_OR_RETURN_ERROR("\"description\"");
                context->state = kHAPIPAccessorySerializationState_CharacteristicDescription_NameSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicDescription_NameSeparator: {
                APPEND_STRING_OR_RETURN_ERROR(":");
                context->state = kHAPIPAccessorySerializationState_CharacteristicDescription_Value;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicDescription_Value: {
                const HAPBaseCharacteristic* baseCharacteristic = GET_CURRENT_CHARACTERISTIC();
                HAPAssert(baseCharacteristic);
                HAPAssert(baseCharacteristic->manufacturerDescription);

                HAPAssert(*numBytes <= maxBytes);
                if (maxBytes - *numBytes < 2) {
                    HAPLogError(&logObject, "Not enough resources to serialize GET /accessories response.");
                    return kHAPError_OutOfResources;
                }
                // Buffer 'bytes' has enough capacity to store at least an empty string including quotation marks.

                const char* manufacturerDescription = HAPNonnull(baseCharacteristic->manufacturerDescription);
                size_t numManufacturerDescriptionBytes = HAPStringGetNumBytes(manufacturerDescription);
                if (maxBytes - *numBytes - 2 < numManufacturerDescriptionBytes) {
                    HAPLogError(&logObject, "Not enough resources to serialize GET /accessories response.");
                    return kHAPError_OutOfResources;
                }
                HAPRawBufferCopyBytes(&bytes[*numBytes + 1], manufacturerDescription, numManufacturerDescriptionBytes);
                err = HAPJSONUtilsEscapeStringData(
                        &bytes[*numBytes + 1], maxBytes - *numBytes - 2, &numManufacturerDescriptionBytes);
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    HAPLogError(&logObject, "Not enough resources to serialize GET /accessories response.");
                    return err;
                }
                bytes[*numBytes] = '"';
                bytes[*numBytes + 1 + numManufacturerDescriptionBytes] = '"';
                *numBytes += 1 + numManufacturerDescriptionBytes + 1;

                HAPAssert(*numBytes <= maxBytes);

                context->state = kHAPIPAccessorySerializationState_CharacteristicDescription_ValueSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicDescription_ValueSeparator: {
                const HAPBaseCharacteristic* baseCharacteristic = GET_CURRENT_CHARACTERISTIC();
                HAPAssert(baseCharacteristic);
                if (HAPCharacteristicGetUnit(baseCharacteristic) != kHAPCharacteristicUnits_None) {
                    APPEND_STRING_OR_RETURN_ERROR(",");
                    context->state = kHAPIPAccessorySerializationState_CharacteristicUnit_Name;
                } else {
                    context->state = kHAPIPAccessorySerializationState_CharacteristicUnit_ValueSeparator;
                }
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicUnit_Name: {
                APPEND_STRING_OR_RETURN_ERROR("\"unit\"");
                context->state = kHAPIPAccessorySerializationState_CharacteristicUnit_NameSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicUnit_NameSeparator: {
                APPEND_STRING_OR_RETURN_ERROR(":");
                context->state = kHAPIPAccessorySerializationState_CharacteristicUnit_Value;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicUnit_Value: {
                const HAPBaseCharacteristic* baseCharacteristic = GET_CURRENT_CHARACTERISTIC();
                HAPAssert(baseCharacteristic);
                switch (HAPCharacteristicGetUnit(baseCharacteristic)) {
                    case kHAPCharacteristicUnits_None: {
                    }
                        HAPFatalError();
                    case kHAPCharacteristicUnits_Celsius: {
                        APPEND_STRING_OR_RETURN_ERROR("\"celsius\"");
                        break;
                    }
                    case kHAPCharacteristicUnits_ArcDegrees: {
                        APPEND_STRING_OR_RETURN_ERROR("\"arcdegrees\"");
                        break;
                    }
                    case kHAPCharacteristicUnits_Percentage: {
                        APPEND_STRING_OR_RETURN_ERROR("\"percentage\"");
                        break;
                    }
                    case kHAPCharacteristicUnits_Lux: {
                        APPEND_STRING_OR_RETURN_ERROR("\"lux\"");
                        break;
                    }
                    case kHAPCharacteristicUnits_Seconds: {
                        APPEND_STRING_OR_RETURN_ERROR("\"seconds\"");
                        break;
                    }
                }
                context->state = kHAPIPAccessorySerializationState_CharacteristicUnit_ValueSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicUnit_ValueSeparator: {
                const HAPBaseCharacteristic* baseCharacteristic = GET_CURRENT_CHARACTERISTIC();
                HAPAssert(baseCharacteristic);
                switch (baseCharacteristic->format) {
                    case kHAPCharacteristicFormat_Bool: {
                        context->state = kHAPIPAccessorySerializationState_CharacteristicObject_End;
                        break;
                    }
                    case kHAPCharacteristicFormat_UInt8: {
                        const HAPUInt8Characteristic* uint8Characteristic =
                                (const HAPUInt8Characteristic*) baseCharacteristic;
                        uint8_t minimumValue = uint8Characteristic->constraints.minimumValue;
                        uint8_t maximumValue = uint8Characteristic->constraints.maximumValue;
                        uint8_t stepValue = uint8Characteristic->constraints.stepValue;
                        HAPAssert(minimumValue <= maximumValue);
                        if (minimumValue || maximumValue != UINT8_MAX || stepValue > 1) {
                            APPEND_STRING_OR_RETURN_ERROR(",");
                            context->state = kHAPIPAccessorySerializationState_CharacteristicMinimumValue_Name;
                        } else {
                            context->state = kHAPIPAccessorySerializationState_CharacteristicStepValue_ValueSeparator;
                        }
                        break;
                    }
                    case kHAPCharacteristicFormat_UInt16: {
                        const HAPUInt16Characteristic* uint16Characteristic =
                                (const HAPUInt16Characteristic*) baseCharacteristic;
                        uint16_t minimumValue = uint16Characteristic->constraints.minimumValue;
                        uint16_t maximumValue = uint16Characteristic->constraints.maximumValue;
                        uint16_t stepValue = uint16Characteristic->constraints.stepValue;
                        HAPAssert(minimumValue <= maximumValue);
                        if (minimumValue || maximumValue != UINT16_MAX || stepValue > 1) {
                            APPEND_STRING_OR_RETURN_ERROR(",");
                            context->state = kHAPIPAccessorySerializationState_CharacteristicMinimumValue_Name;
                        } else {
                            context->state = kHAPIPAccessorySerializationState_CharacteristicObject_End;
                        }
                        break;
                    }
                    case kHAPCharacteristicFormat_UInt32: {
                        const HAPUInt32Characteristic* uint32Characteristic =
                                (const HAPUInt32Characteristic*) baseCharacteristic;
                        uint32_t minimumValue = uint32Characteristic->constraints.minimumValue;
                        uint32_t maximumValue = uint32Characteristic->constraints.maximumValue;
                        uint32_t stepValue = uint32Characteristic->constraints.stepValue;
                        HAPAssert(minimumValue <= maximumValue);
                        if (minimumValue || maximumValue != UINT32_MAX || stepValue > 1) {
                            APPEND_STRING_OR_RETURN_ERROR(",");
                            context->state = kHAPIPAccessorySerializationState_CharacteristicMinimumValue_Name;
                        } else {
                            context->state = kHAPIPAccessorySerializationState_CharacteristicObject_End;
                        }
                        break;
                    }
                    case kHAPCharacteristicFormat_UInt64: {
                        const HAPUInt64Characteristic* uint64Characteristic =
                                (const HAPUInt64Characteristic*) baseCharacteristic;
                        uint64_t minimumValue = uint64Characteristic->constraints.minimumValue;
                        uint64_t maximumValue = uint64Characteristic->constraints.maximumValue;
                        uint64_t stepValue = uint64Characteristic->constraints.stepValue;
                        HAPAssert(minimumValue <= maximumValue);
                        if (minimumValue || maximumValue != UINT64_MAX || stepValue > 1) {
                            APPEND_STRING_OR_RETURN_ERROR(",");
                            context->state = kHAPIPAccessorySerializationState_CharacteristicMinimumValue_Name;
                        } else {
                            context->state = kHAPIPAccessorySerializationState_CharacteristicObject_End;
                        }
                        break;
                    }
                    case kHAPCharacteristicFormat_Int: {
                        const HAPIntCharacteristic* intCharacteristic =
                                (const HAPIntCharacteristic*) baseCharacteristic;
                        int32_t minimumValue = intCharacteristic->constraints.minimumValue;
                        int32_t maximumValue = intCharacteristic->constraints.maximumValue;
                        int32_t stepValue = intCharacteristic->constraints.stepValue;
                        HAPAssert(minimumValue <= maximumValue);
                        HAPAssert(stepValue >= 0);
                        if (minimumValue != INT32_MIN || maximumValue != INT32_MAX || stepValue > 1) {
                            APPEND_STRING_OR_RETURN_ERROR(",");
                            context->state = kHAPIPAccessorySerializationState_CharacteristicMinimumValue_Name;
                        } else {
                            context->state = kHAPIPAccessorySerializationState_CharacteristicObject_End;
                        }
                        break;
                    }
                    case kHAPCharacteristicFormat_Float: {
                        const HAPFloatCharacteristic* floatCharacteristic =
                                (const HAPFloatCharacteristic*) baseCharacteristic;
                        float minimumValue = floatCharacteristic->constraints.minimumValue;
                        float maximumValue = floatCharacteristic->constraints.maximumValue;
                        float stepValue = floatCharacteristic->constraints.stepValue;
                        HAPAssert(HAPFloatIsFinite(minimumValue) || HAPFloatIsInfinite(minimumValue));
                        HAPAssert(HAPFloatIsFinite(maximumValue) || HAPFloatIsInfinite(maximumValue));
                        HAPAssert(minimumValue <= maximumValue);
                        HAPAssert(stepValue >= 0);
                        if (!(HAPFloatIsInfinite(minimumValue) && minimumValue < 0) ||
                            !(HAPFloatIsInfinite(maximumValue) && maximumValue > 0) || !HAPFloatIsZero(stepValue)) {
                            APPEND_STRING_OR_RETURN_ERROR(",");
                            context->state = kHAPIPAccessorySerializationState_CharacteristicMinimumValue_Name;
                        } else {
                            context->state = kHAPIPAccessorySerializationState_CharacteristicObject_End;
                        }
                        break;
                    }
                    case kHAPCharacteristicFormat_String: {
                        const HAPStringCharacteristic* stringCharacteristic =
                                (const HAPStringCharacteristic*) baseCharacteristic;
                        if (stringCharacteristic->constraints.maxLength !=
                            kHAPIPAccessorySerialization_DefaultMaxStringBytes) {
                            APPEND_STRING_OR_RETURN_ERROR(",");
                            context->state = kHAPIPAccessorySerializationState_CharacteristicMaxLength_Name;
                        } else {
                            context->state = kHAPIPAccessorySerializationState_CharacteristicObject_End;
                        }
                        break;
                    }
                    case kHAPCharacteristicFormat_TLV8: {
                        context->state = kHAPIPAccessorySerializationState_CharacteristicObject_End;
                        break;
                    }
                    case kHAPCharacteristicFormat_Data: {
                        const HAPDataCharacteristic* dataCharacteristic =
                                (const HAPDataCharacteristic*) baseCharacteristic;
                        if (dataCharacteristic->constraints.maxLength !=
                            kHAPIPAccessorySerialization_DefaultMaxDataBytes) {
                            APPEND_STRING_OR_RETURN_ERROR(",");
                            context->state = kHAPIPAccessorySerializationState_CharacteristicMaxDataLength_Name;
                        } else {
                            context->state = kHAPIPAccessorySerializationState_CharacteristicObject_End;
                        }
                        break;
                    }
                }
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicMinimumValue_Name: {
                APPEND_STRING_OR_RETURN_ERROR("\"minValue\"");
                context->state = kHAPIPAccessorySerializationState_CharacteristicMinimumValue_NameSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicMinimumValue_NameSeparator: {
                APPEND_STRING_OR_RETURN_ERROR(":");
                context->state = kHAPIPAccessorySerializationState_CharacteristicMinimumValue_Value;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicMinimumValue_Value: {
                const HAPBaseCharacteristic* baseCharacteristic = GET_CURRENT_CHARACTERISTIC();
                HAPAssert(baseCharacteristic);
                switch (baseCharacteristic->format) {
                    case kHAPCharacteristicFormat_UInt8: {
                        const HAPUInt8Characteristic* uint8Characteristic =
                                (const HAPUInt8Characteristic*) baseCharacteristic;
                        APPEND_UINT64_OR_RETURN_ERROR(uint8Characteristic->constraints.minimumValue);
                        context->state = kHAPIPAccessorySerializationState_CharacteristicMinimumValue_ValueSeparator;
                        break;
                    }
                    case kHAPCharacteristicFormat_UInt16: {
                        const HAPUInt16Characteristic* uint16Characteristic =
                                (const HAPUInt16Characteristic*) baseCharacteristic;
                        APPEND_UINT64_OR_RETURN_ERROR(uint16Characteristic->constraints.minimumValue);
                        context->state = kHAPIPAccessorySerializationState_CharacteristicMinimumValue_ValueSeparator;
                        break;
                    }
                    case kHAPCharacteristicFormat_UInt32: {
                        const HAPUInt32Characteristic* uint32Characteristic =
                                (const HAPUInt32Characteristic*) baseCharacteristic;
                        APPEND_UINT64_OR_RETURN_ERROR(uint32Characteristic->constraints.minimumValue);
                        context->state = kHAPIPAccessorySerializationState_CharacteristicMinimumValue_ValueSeparator;
                        break;
                    }
                    case kHAPCharacteristicFormat_UInt64: {
                        const HAPUInt64Characteristic* uint64Characteristic =
                                (const HAPUInt64Characteristic*) baseCharacteristic;
                        APPEND_UINT64_OR_RETURN_ERROR(uint64Characteristic->constraints.minimumValue);
                        context->state = kHAPIPAccessorySerializationState_CharacteristicMinimumValue_ValueSeparator;
                        break;
                    }
                    case kHAPCharacteristicFormat_Int: {
                        const HAPIntCharacteristic* intCharacteristic =
                                (const HAPIntCharacteristic*) baseCharacteristic;
                        APPEND_INT32_OR_RETURN_ERROR(intCharacteristic->constraints.minimumValue);
                        context->state = kHAPIPAccessorySerializationState_CharacteristicMinimumValue_ValueSeparator;
                        break;
                    }
                    case kHAPCharacteristicFormat_Float: {
                        const HAPFloatCharacteristic* floatCharacteristic =
                                (const HAPFloatCharacteristic*) baseCharacteristic;
                        APPEND_FLOAT_OR_RETURN_ERROR(floatCharacteristic->constraints.minimumValue);
                        context->state = kHAPIPAccessorySerializationState_CharacteristicMinimumValue_ValueSeparator;
                        break;
                    }
                    case kHAPCharacteristicFormat_Bool:
                    case kHAPCharacteristicFormat_String:
                    case kHAPCharacteristicFormat_TLV8:
                    case kHAPCharacteristicFormat_Data: {
                    }
                        HAPFatalError();
                }
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicMinimumValue_ValueSeparator: {
                APPEND_STRING_OR_RETURN_ERROR(",");
                context->state = kHAPIPAccessorySerializationState_CharacteristicMaximumValue_Name;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicMaximumValue_Name: {
                APPEND_STRING_OR_RETURN_ERROR("\"maxValue\"");
                context->state = kHAPIPAccessorySerializationState_CharacteristicMaximumValue_NameSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicMaximumValue_NameSeparator: {
                APPEND_STRING_OR_RETURN_ERROR(":");
                context->state = kHAPIPAccessorySerializationState_CharacteristicMaximumValue_Value;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicMaximumValue_Value: {
                const HAPBaseCharacteristic* baseCharacteristic = GET_CURRENT_CHARACTERISTIC();
                HAPAssert(baseCharacteristic);
                switch (baseCharacteristic->format) {
                    case kHAPCharacteristicFormat_UInt8: {
                        const HAPUInt8Characteristic* uint8Characteristic =
                                (const HAPUInt8Characteristic*) baseCharacteristic;
                        APPEND_UINT64_OR_RETURN_ERROR(uint8Characteristic->constraints.maximumValue);
                        context->state = kHAPIPAccessorySerializationState_CharacteristicMaximumValue_ValueSeparator;
                        break;
                    }
                    case kHAPCharacteristicFormat_UInt16: {
                        const HAPUInt16Characteristic* uint16Characteristic =
                                (const HAPUInt16Characteristic*) baseCharacteristic;
                        APPEND_UINT64_OR_RETURN_ERROR(uint16Characteristic->constraints.maximumValue);
                        context->state = kHAPIPAccessorySerializationState_CharacteristicMaximumValue_ValueSeparator;
                        break;
                    }
                    case kHAPCharacteristicFormat_UInt32: {
                        const HAPUInt32Characteristic* uint32Characteristic =
                                (const HAPUInt32Characteristic*) baseCharacteristic;
                        APPEND_UINT64_OR_RETURN_ERROR(uint32Characteristic->constraints.maximumValue);
                        context->state = kHAPIPAccessorySerializationState_CharacteristicMaximumValue_ValueSeparator;
                        break;
                    }
                    case kHAPCharacteristicFormat_UInt64: {
                        const HAPUInt64Characteristic* uint64Characteristic =
                                (const HAPUInt64Characteristic*) baseCharacteristic;
                        APPEND_UINT64_OR_RETURN_ERROR(uint64Characteristic->constraints.maximumValue);
                        context->state = kHAPIPAccessorySerializationState_CharacteristicMaximumValue_ValueSeparator;
                        break;
                    }
                    case kHAPCharacteristicFormat_Int: {
                        const HAPIntCharacteristic* intCharacteristic =
                                (const HAPIntCharacteristic*) baseCharacteristic;
                        APPEND_INT32_OR_RETURN_ERROR(intCharacteristic->constraints.maximumValue);
                        context->state = kHAPIPAccessorySerializationState_CharacteristicMaximumValue_ValueSeparator;
                        break;
                    }
                    case kHAPCharacteristicFormat_Float: {
                        const HAPFloatCharacteristic* floatCharacteristic =
                                (const HAPFloatCharacteristic*) baseCharacteristic;
                        APPEND_FLOAT_OR_RETURN_ERROR(floatCharacteristic->constraints.maximumValue);
                        context->state = kHAPIPAccessorySerializationState_CharacteristicMaximumValue_ValueSeparator;
                        break;
                    }
                    case kHAPCharacteristicFormat_Bool:
                    case kHAPCharacteristicFormat_String:
                    case kHAPCharacteristicFormat_TLV8:
                    case kHAPCharacteristicFormat_Data: {
                    }
                        HAPFatalError();
                }
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicMaximumValue_ValueSeparator: {
                APPEND_STRING_OR_RETURN_ERROR(",");
                context->state = kHAPIPAccessorySerializationState_CharacteristicStepValue_Name;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicStepValue_Name: {
                APPEND_STRING_OR_RETURN_ERROR("\"minStep\"");
                context->state = kHAPIPAccessorySerializationState_CharacteristicStepValue_NameSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicStepValue_NameSeparator: {
                APPEND_STRING_OR_RETURN_ERROR(":");
                context->state = kHAPIPAccessorySerializationState_CharacteristicStepValue_Value;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicStepValue_Value: {
                const HAPBaseCharacteristic* baseCharacteristic = GET_CURRENT_CHARACTERISTIC();
                HAPAssert(baseCharacteristic);
                switch (baseCharacteristic->format) {
                    case kHAPCharacteristicFormat_UInt8: {
                        const HAPUInt8Characteristic* uint8Characteristic =
                                (const HAPUInt8Characteristic*) baseCharacteristic;
                        APPEND_UINT64_OR_RETURN_ERROR(uint8Characteristic->constraints.stepValue);
                        context->state = kHAPIPAccessorySerializationState_CharacteristicStepValue_ValueSeparator;
                        break;
                    }
                    case kHAPCharacteristicFormat_UInt16: {
                        const HAPUInt16Characteristic* uint16Characteristic =
                                (const HAPUInt16Characteristic*) baseCharacteristic;
                        APPEND_UINT64_OR_RETURN_ERROR(uint16Characteristic->constraints.stepValue);
                        context->state = kHAPIPAccessorySerializationState_CharacteristicObject_End;
                        break;
                    }
                    case kHAPCharacteristicFormat_UInt32: {
                        const HAPUInt32Characteristic* uint32Characteristic =
                                (const HAPUInt32Characteristic*) baseCharacteristic;
                        APPEND_UINT64_OR_RETURN_ERROR(uint32Characteristic->constraints.stepValue);
                        context->state = kHAPIPAccessorySerializationState_CharacteristicObject_End;
                        break;
                    }
                    case kHAPCharacteristicFormat_UInt64: {
                        const HAPUInt64Characteristic* uint64Characteristic =
                                (const HAPUInt64Characteristic*) baseCharacteristic;
                        APPEND_UINT64_OR_RETURN_ERROR(uint64Characteristic->constraints.stepValue);
                        context->state = kHAPIPAccessorySerializationState_CharacteristicObject_End;
                        break;
                    }
                    case kHAPCharacteristicFormat_Int: {
                        const HAPIntCharacteristic* intCharacteristic =
                                (const HAPIntCharacteristic*) baseCharacteristic;
                        APPEND_INT32_OR_RETURN_ERROR(intCharacteristic->constraints.stepValue);
                        context->state = kHAPIPAccessorySerializationState_CharacteristicObject_End;
                        break;
                    }
                    case kHAPCharacteristicFormat_Float: {
                        const HAPFloatCharacteristic* floatCharacteristic =
                                (const HAPFloatCharacteristic*) baseCharacteristic;
                        APPEND_FLOAT_OR_RETURN_ERROR(floatCharacteristic->constraints.stepValue);
                        context->state = kHAPIPAccessorySerializationState_CharacteristicObject_End;
                        break;
                    }
                    case kHAPCharacteristicFormat_Bool:
                    case kHAPCharacteristicFormat_String:
                    case kHAPCharacteristicFormat_TLV8:
                    case kHAPCharacteristicFormat_Data: {
                    }
                        HAPFatalError();
                }
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicStepValue_ValueSeparator: {
                const HAPBaseCharacteristic* baseCharacteristic = GET_CURRENT_CHARACTERISTIC();
                HAPAssert(baseCharacteristic);
                HAPAssert(baseCharacteristic->format == kHAPCharacteristicFormat_UInt8);
                const HAPUInt8Characteristic* uint8Characteristic = (const HAPUInt8Characteristic*) baseCharacteristic;
                if (HAPUUIDIsAppleDefined(uint8Characteristic->characteristicType)) {
                    if (uint8Characteristic->constraints.validValues) {
                        APPEND_STRING_OR_RETURN_ERROR(",");
                        context->state = kHAPIPAccessorySerializationState_CharacteristicValidValuesArray_Name;
                    } else if (uint8Characteristic->constraints.validValuesRanges) {
                        APPEND_STRING_OR_RETURN_ERROR(",");
                        context->state = kHAPIPAccessorySerializationState_CharacteristicValidValuesRangesArray_Name;
                    } else {
                        context->state = kHAPIPAccessorySerializationState_CharacteristicObject_End;
                    }
                } else {
                    context->state = kHAPIPAccessorySerializationState_CharacteristicObject_End;
                }
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicMaxLength_Name: {
                APPEND_STRING_OR_RETURN_ERROR("\"maxLen\"");
                context->state = kHAPIPAccessorySerializationState_CharacteristicMaxLength_NameSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicMaxLength_NameSeparator: {
                APPEND_STRING_OR_RETURN_ERROR(":");
                context->state = kHAPIPAccessorySerializationState_CharacteristicMaxLength_Value;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicMaxLength_Value: {
                const HAPBaseCharacteristic* baseCharacteristic = GET_CURRENT_CHARACTERISTIC();
                HAPAssert(baseCharacteristic);
                HAPAssert(baseCharacteristic->format == kHAPCharacteristicFormat_String);
                const HAPStringCharacteristic* stringCharacteristic =
                        (const HAPStringCharacteristic*) baseCharacteristic;
                APPEND_UINT64_OR_RETURN_ERROR(stringCharacteristic->constraints.maxLength);
                context->state = kHAPIPAccessorySerializationState_CharacteristicObject_End;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicMaxDataLength_Name: {
                APPEND_STRING_OR_RETURN_ERROR("\"maxDataLen\"");
                context->state = kHAPIPAccessorySerializationState_CharacteristicMaxDataLength_NameSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicMaxDataLength_NameSeparator: {
                APPEND_STRING_OR_RETURN_ERROR(":");
                context->state = kHAPIPAccessorySerializationState_CharacteristicMaxDataLength_Value;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicMaxDataLength_Value: {
                const HAPBaseCharacteristic* baseCharacteristic = GET_CURRENT_CHARACTERISTIC();
                HAPAssert(baseCharacteristic);
                HAPAssert(baseCharacteristic->format == kHAPCharacteristicFormat_Data);
                const HAPDataCharacteristic* dataCharacteristic = (const HAPDataCharacteristic*) baseCharacteristic;
                APPEND_UINT64_OR_RETURN_ERROR(dataCharacteristic->constraints.maxLength);
                context->state = kHAPIPAccessorySerializationState_CharacteristicObject_End;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicValidValuesArray_Name: {
                APPEND_STRING_OR_RETURN_ERROR("\"valid-values\"");
                context->state = kHAPIPAccessorySerializationState_CharacteristicValidValuesArray_NameSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicValidValuesArray_NameSeparator: {
                APPEND_STRING_OR_RETURN_ERROR(":");
                context->state = kHAPIPAccessorySerializationState_CharacteristicValidValuesArray_Begin;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicValidValuesArray_Begin: {
                APPEND_STRING_OR_RETURN_ERROR("[");
                const HAPBaseCharacteristic* baseCharacteristic = GET_CURRENT_CHARACTERISTIC();
                HAPAssert(baseCharacteristic);
                HAPAssert(baseCharacteristic->format == kHAPCharacteristicFormat_UInt8);
                const HAPUInt8Characteristic* uint8Characteristic = (const HAPUInt8Characteristic*) baseCharacteristic;
                HAPAssert(HAPUUIDIsAppleDefined(uint8Characteristic->characteristicType));
                const uint8_t* const* validValues = uint8Characteristic->constraints.validValues;
                HAPAssert(validValues);
                context->index = 0;
                if (validValues[context->index]) {
                    context->state = kHAPIPAccessorySerializationState_CharacteristicValidValue_Value;
                } else {
                    context->state = kHAPIPAccessorySerializationState_CharacteristicValidValuesArray_End;
                }
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicValidValuesArray_End: {
                APPEND_STRING_OR_RETURN_ERROR("]");
                context->state = kHAPIPAccessorySerializationState_CharacteristicValidValuesArray_ValueSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicValidValuesArray_ValueSeparator: {
                const HAPBaseCharacteristic* baseCharacteristic = GET_CURRENT_CHARACTERISTIC();
                HAPAssert(baseCharacteristic);
                HAPAssert(baseCharacteristic->format == kHAPCharacteristicFormat_UInt8);
                const HAPUInt8Characteristic* uint8Characteristic = (const HAPUInt8Characteristic*) baseCharacteristic;
                HAPAssert(HAPUUIDIsAppleDefined(uint8Characteristic->characteristicType));
                if (uint8Characteristic->constraints.validValuesRanges) {
                    APPEND_STRING_OR_RETURN_ERROR(",");
                    context->state = kHAPIPAccessorySerializationState_CharacteristicValidValuesRangesArray_Name;
                } else {
                    context->state = kHAPIPAccessorySerializationState_CharacteristicObject_End;
                }
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicValidValue_Value: {
                const HAPBaseCharacteristic* baseCharacteristic = GET_CURRENT_CHARACTERISTIC();
                HAPAssert(baseCharacteristic);
                HAPAssert(baseCharacteristic->format == kHAPCharacteristicFormat_UInt8);
                const HAPUInt8Characteristic* uint8Characteristic = (const HAPUInt8Characteristic*) baseCharacteristic;
                HAPAssert(HAPUUIDIsAppleDefined(uint8Characteristic->characteristicType));
                const uint8_t* const* validValues = uint8Characteristic->constraints.validValues;
                HAPAssert(validValues);
                HAPAssert(validValues[context->index]);
                APPEND_UINT64_OR_RETURN_ERROR(*validValues[context->index]);
                HAPAssert(context->index < UINT8_MAX);
                context->index++;
                if (validValues[context->index]) {
                    context->state = kHAPIPAccessorySerializationState_CharacteristicValidValue_Separator;
                } else {
                    context->state = kHAPIPAccessorySerializationState_CharacteristicValidValuesArray_End;
                }
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicValidValue_Separator: {
                APPEND_STRING_OR_RETURN_ERROR(",");
                context->state = kHAPIPAccessorySerializationState_CharacteristicValidValue_Value;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicValidValuesRangesArray_Name: {
                APPEND_STRING_OR_RETURN_ERROR("\"valid-values-range\"");
                context->state = kHAPIPAccessorySerializationState_CharacteristicValidValuesRangesArray_NameSeparator;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicValidValuesRangesArray_NameSeparator: {
                APPEND_STRING_OR_RETURN_ERROR(":");
                context->state = kHAPIPAccessorySerializationState_CharacteristicValidValuesRangesArray_Begin;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicValidValuesRangesArray_Begin: {
                APPEND_STRING_OR_RETURN_ERROR("[");
                const HAPBaseCharacteristic* baseCharacteristic = GET_CURRENT_CHARACTERISTIC();
                HAPAssert(baseCharacteristic);
                HAPAssert(baseCharacteristic->format == kHAPCharacteristicFormat_UInt8);
                const HAPUInt8Characteristic* uint8Characteristic = (const HAPUInt8Characteristic*) baseCharacteristic;
                HAPAssert(HAPUUIDIsAppleDefined(uint8Characteristic->characteristicType));
                const HAPUInt8CharacteristicValidValuesRange* const* validValuesRanges =
                        uint8Characteristic->constraints.validValuesRanges;
                HAPAssert(validValuesRanges);
                context->index = 0;
                if (validValuesRanges[context->index]) {
                    context->state = kHAPIPAccessorySerializationState_CharacteristicValidValuesRangeArray_Begin;
                } else {
                    context->state = kHAPIPAccessorySerializationState_CharacteristicValidValuesRangesArray_End;
                }
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicValidValuesRangesArray_End: {
                APPEND_STRING_OR_RETURN_ERROR("]");
                context->state = kHAPIPAccessorySerializationState_CharacteristicObject_End;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicValidValuesRangeArray_Begin: {
                APPEND_STRING_OR_RETURN_ERROR("[");
                context->state = kHAPIPAccessorySerializationState_CharacteristicValidValuesRangeStart_Value;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicValidValuesRangeArray_End: {
                APPEND_STRING_OR_RETURN_ERROR("]");
                const HAPBaseCharacteristic* baseCharacteristic = GET_CURRENT_CHARACTERISTIC();
                HAPAssert(baseCharacteristic);
                HAPAssert(baseCharacteristic->format == kHAPCharacteristicFormat_UInt8);
                const HAPUInt8Characteristic* uint8Characteristic = (const HAPUInt8Characteristic*) baseCharacteristic;
                HAPAssert(HAPUUIDIsAppleDefined(uint8Characteristic->characteristicType));
                const HAPUInt8CharacteristicValidValuesRange* const* validValuesRanges =
                        uint8Characteristic->constraints.validValuesRanges;
                HAPAssert(validValuesRanges);
                HAPAssert(context->index < UINT8_MAX);
                context->index++;
                if (validValuesRanges[context->index]) {
                    context->state = kHAPIPAccessorySerializationState_CharacteristicValidValuesRangeArray_Separator;
                } else {
                    context->state = kHAPIPAccessorySerializationState_CharacteristicValidValuesRangesArray_End;
                }
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicValidValuesRangeArray_Separator: {
                APPEND_STRING_OR_RETURN_ERROR(",");
                context->state = kHAPIPAccessorySerializationState_CharacteristicValidValuesRangeArray_Begin;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicValidValuesRangeStart_Value: {
                const HAPBaseCharacteristic* baseCharacteristic = GET_CURRENT_CHARACTERISTIC();
                HAPAssert(baseCharacteristic);
                HAPAssert(baseCharacteristic->format == kHAPCharacteristicFormat_UInt8);
                const HAPUInt8Characteristic* uint8Characteristic = (const HAPUInt8Characteristic*) baseCharacteristic;
                HAPAssert(HAPUUIDIsAppleDefined(uint8Characteristic->characteristicType));
                const HAPUInt8CharacteristicValidValuesRange* const* validValuesRanges =
                        uint8Characteristic->constraints.validValuesRanges;
                HAPAssert(validValuesRanges);
                HAPAssert(validValuesRanges[context->index]);
                APPEND_UINT64_OR_RETURN_ERROR(validValuesRanges[context->index]->start);
                context->state = kHAPIPAccessorySerializationState_CharacteristicValidValuesRange_Separator;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicValidValuesRangeEnd_Value: {
                const HAPBaseCharacteristic* baseCharacteristic = GET_CURRENT_CHARACTERISTIC();
                HAPAssert(baseCharacteristic);
                HAPAssert(baseCharacteristic->format == kHAPCharacteristicFormat_UInt8);
                const HAPUInt8Characteristic* uint8Characteristic = (const HAPUInt8Characteristic*) baseCharacteristic;
                HAPAssert(HAPUUIDIsAppleDefined(uint8Characteristic->characteristicType));
                const HAPUInt8CharacteristicValidValuesRange* const* validValuesRanges =
                        uint8Characteristic->constraints.validValuesRanges;
                HAPAssert(validValuesRanges);
                HAPAssert(validValuesRanges[context->index]);
                APPEND_UINT64_OR_RETURN_ERROR(validValuesRanges[context->index]->end);
                context->state = kHAPIPAccessorySerializationState_CharacteristicValidValuesRangeArray_End;
            }
                continue;
            case kHAPIPAccessorySerializationState_CharacteristicValidValuesRange_Separator: {
                APPEND_STRING_OR_RETURN_ERROR(",");
                context->state = kHAPIPAccessorySerializationState_CharacteristicValidValuesRangeEnd_Value;
            }
                continue;
            case kHAPIPAccessorySerializationState_ResponseIsComplete: {
            }
                HAPFatalError();
            default:
                HAPFatalError();
        }
    } while ((*numBytes < minBytes) && (context->state != kHAPIPAccessorySerializationState_ResponseIsComplete));

#undef APPEND_FLOAT_OR_RETURN_ERROR
#undef APPEND_INT32_OR_RETURN_ERROR
#undef APPEND_UINT64_OR_RETURN_ERROR
#undef APPEND_UUID_OR_RETURN_ERROR
#undef APPEND_STRING_OR_RETURN_ERROR

#undef GET_CURRENT_CHARACTERISTIC
#undef GET_CURRENT_SERVICE
#undef GET_CURRENT_ACCESSORY

    return kHAPError_None;
}

#endif
