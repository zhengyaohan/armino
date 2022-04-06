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

#include "HAPCharacteristic.h"

#include "HAPAccessoryServer+Internal.h"
#include "HAPLog+Attributes.h"
#include "HAPLogSubsystem.h"
#include "HAPUUID.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "Characteristic" };

// Read handlers may be called in response to:
// - Explicit read from controller
// - Implicit read to fetch notification value (unidirectional or BLE broadcast)
//
// Note that the mechanism by which notifications are delivered varies by transport.
//
// - When using HAP over IP (Ethernet / Wi-Fi), notification values within a 1 second window are coalesced.
//   If a notification happens on the same characteristic multiple times within that window,
//   earlier values get discarded and won't be sent. Only when the next notification windows is sent,
//   will the then-current characteristic value be fetched and be delivered to the controller.
//   As an exception to this rule, notifications for the `Button Event` and `Programmable Switch Event`
//   characteristics are delivered immediately.
//
// - When using HAP over Bluetooth LE, notification packets only indicate the changed characteristic.
//   The characteristic value is never delivered to the controller as part of the notification.
//   Instead, a read is triggered by the controller which is not distinguishable from a regular read
//   that is not triggered in response to a notification packet.
//   One of the reasons for this behaviour might be that Bluetooth LE only supports pushing
//   notification packets up to ~ATT_MTU which might be smaller than the characteristic value,
//   especially considering additional bytes necessary for encryption. Additionally, Bluetooth LE
//   sends GATT Indication and GATT Notification packets interleaved with response packets, so they
//   might get reordered, requiring keeping track of a separate encryption nonce for notifications.
//   Note that on HAP over Bluetooth LE, notification packets are sent without any authentication,
//   so could potentially be spoofed, dropped or modified by an attacker.
//
// As it is not possible to distinguish regular reads from implicit reads to fetch notification values
// on Bluetooth LE, we do not forward this information to the application implementing the read handler.
// This allows the application to be implemented in a transport agnostic manner.
//
// If a characteristic handler needs to send different values for notifications and regular reads
// there are a few available solutions:
//
// - If the characteristic is a control point supporting write response, regular reads are never used.
//   The supportsWriteResponse characteristic property guarantees that the read handler will be called
//   immediately after returning a success response from a write handler. This means, that if there
//   was a preceding successful write request, the next time the read handler is called is to fetch the
//   write response value. Otherwise, the read handler is only ever called to deliver notifications.
//   This scheme is used for Wi-Fi Router service control point characteristics that also support events.
//
// - There are a few characteristics which only support HAP events but which also have the readable property set
//   to improve compatibility with certain controller versions. For now, this is restricted to the
//   `Button Event` and `Programmable Switch Event` characteristics. As these characteristics are exceptions,
//   special behaviour for these characteristics is hardcoded (as it also varies across transports).
//   If more characteristics following that pattern are introduced in the future, it should be
//   considered to introduce a characteristic property to opt-in to this special behaviour.
//
// - For other characteristics, it should not be necessary to ever send different values as payloads
//   for regular reads compared to implicit reads due to a notification being processed.
//
// See HomeKit Accessory Protocol Specification R17
// Section 6.8 Notifications
// See HomeKit Accessory Protocol Specification R17
// Section 7.4.6.1 Connected Events

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HAP_RESULT_USE_CHECK
bool HAPCharacteristicReadRequiresAdminPermissions(const HAPCharacteristic* characteristic_) {
    HAPPrecondition(characteristic_);
    const HAPBaseCharacteristic* characteristic = characteristic_;

    HAP_DIAGNOSTIC_PUSH
    HAP_DIAGNOSTIC_IGNORED_CLANG("-Wdeprecated-declarations")
    HAP_DIAGNOSTIC_IGNORED_GCC("-Wdeprecated-declarations")
    HAP_DIAGNOSTIC_IGNORED_ARMCC(2570)
    HAP_DIAGNOSTIC_IGNORED_ICCARM(Pe1444)
    bool readRequiresAdminPermissions = characteristic->properties.readRequiresAdminPermissions;
    HAP_DIAGNOSTIC_RESTORE_ICCARM(Pe1444)
    HAP_DIAGNOSTIC_POP

    return readRequiresAdminPermissions;
}

HAP_RESULT_USE_CHECK
bool HAPCharacteristicWriteRequiresAdminPermissions(const HAPCharacteristic* characteristic_) {
    HAPPrecondition(characteristic_);
    const HAPBaseCharacteristic* characteristic = characteristic_;

    HAP_DIAGNOSTIC_PUSH
    HAP_DIAGNOSTIC_IGNORED_CLANG("-Wdeprecated-declarations")
    HAP_DIAGNOSTIC_IGNORED_GCC("-Wdeprecated-declarations")
    HAP_DIAGNOSTIC_IGNORED_ARMCC(2570)
    HAP_DIAGNOSTIC_IGNORED_ICCARM(Pe1444)
    bool writeRequiresAdminPermissions = characteristic->properties.writeRequiresAdminPermissions;
    HAP_DIAGNOSTIC_RESTORE_ICCARM(Pe1444)
    HAP_DIAGNOSTIC_POP

    return writeRequiresAdminPermissions;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void WillHandleWrite(
        HAPAccessoryServer* server,
        HAPSession* session,
        const HAPCharacteristic* characteristic_,
        const HAPService* service,
        const HAPAccessory* accessory) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(characteristic_);
    const HAPBaseCharacteristic* characteristic = characteristic_;
    HAPPrecondition(service);
    HAPPrecondition(accessory);

    HAPPrecondition(!server->writeRequest.session);
    server->writeRequest.session = session;
    server->writeRequest.aid = accessory->aid;
    server->writeRequest.iid = characteristic->iid;
}

static void DidHandleWrite(
        HAPAccessoryServer* server,
        HAPSession* session,
        const HAPCharacteristic* characteristic,
        const HAPService* service,
        const HAPAccessory* accessory) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(characteristic);
    HAPPrecondition(service);
    HAPPrecondition(accessory);

    HAPPrecondition(HAPCharacteristicIsHandlingWrite(server, session, characteristic, service, accessory));
    HAPRawBufferZero(&server->writeRequest, sizeof server->writeRequest);
}

HAP_RESULT_USE_CHECK
bool HAPCharacteristicIsHandlingWrite(
        HAPAccessoryServer* server,
        HAPSession* session,
        const HAPCharacteristic* characteristic_,
        const HAPService* service,
        const HAPAccessory* accessory) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(characteristic_);
    const HAPBaseCharacteristic* characteristic = characteristic_;
    HAPPrecondition(service);
    HAPPrecondition(accessory);

    return server->writeRequest.session == session && server->writeRequest.aid == accessory->aid &&
           server->writeRequest.iid == characteristic->iid;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define IS_VALUE_IN_RANGE(value, constraints) \
    ((value) >= (constraints).minimumValue && (value) <= (constraints).maximumValue && \
     (!(constraints).stepValue || ((value) - (constraints).minimumValue) % (constraints).stepValue == 0))

#define IS_VALUE_IN_RANGE_WITH_TOLERANCE(value, constraints, tolerance) \
    ((value) >= (constraints).minimumValue && (value) <= (constraints).maximumValue && \
     (HAPFloatIsZero((constraints).stepValue) || \
      HAPFloatGetAbsoluteValue( \
              HAPFloatGetFraction(((value) - (constraints).minimumValue) / (constraints).stepValue + 0.5F) - 0.5F) <= \
              (tolerance)))

#define IS_LENGTH_IN_RANGE(length, constraints) ((length) <= (constraints).maxLength)

#define ROUND_VALUE_TO_STEP(value, constraints) \
    ((value) - (HAPFloatGetFraction(((value) - (constraints).minimumValue) / (constraints).stepValue + 0.5F) - 0.5F) * \
                       (constraints).stepValue)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HAP_RESULT_USE_CHECK
static bool HAPDataCharacteristicIsValueFulfillingConstraints(
        const HAPDataCharacteristic* characteristic,
        const HAPService* service HAP_UNUSED,
        const HAPAccessory* accessory,
        size_t numValueBytes) {
    if (!IS_LENGTH_IN_RANGE(numValueBytes, characteristic->constraints)) {
        HAPLogCharacteristic(
                &logObject,
                characteristic,
                service,
                accessory,
                "Value too long: %zu bytes (constraints: maxLength = %lu bytes).",
                numValueBytes,
                (unsigned long) characteristic->constraints.maxLength);
        return false;
    }

    return true;
}

HAP_RESULT_USE_CHECK
HAPError HAPDataCharacteristicHandleRead(
        HAPAccessoryServer* server,
        const HAPDataCharacteristicReadRequest* request,
        void* valueBytes,
        size_t maxValueBytes,
        size_t* numValueBytes,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_Data);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->characteristic->callbacks.handleRead);
    HAPPrecondition(request->accessory);
    HAPPrecondition(valueBytes);
    HAPPrecondition(numValueBytes);

    HAPError err;

    // Call handler.
    HAPLogCharacteristicInfo(
            &logObject, request->characteristic, request->service, request->accessory, "Calling read handler.");
    err = request->characteristic->callbacks.handleRead(
            server, request, valueBytes, maxValueBytes, numValueBytes, context);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                err == kHAPError_Busy);
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Read handler failed with error %u.",
                err);
        return err;
    }
    if (*numValueBytes > maxValueBytes) {
        HAPLogCharacteristicError(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Read data value exceeds available buffer space (%zu bytes / available %zu bytes).",
                *numValueBytes,
                maxValueBytes);
        HAPFatalError();
    }

    // Validate constraints.
    HAPAssert(HAPDataCharacteristicIsValueFulfillingConstraints(
            request->characteristic, request->service, request->accessory, *numValueBytes));

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPDataCharacteristicHandleWrite(
        HAPAccessoryServer* server,
        const HAPDataCharacteristicWriteRequest* request,
        const void* valueBytes,
        size_t numValueBytes,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_Data);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->characteristic->callbacks.handleWrite);
    HAPPrecondition(request->accessory);
    HAPPrecondition(valueBytes);

    HAPError err;

    // Validate constraints.
    if (!HAPDataCharacteristicIsValueFulfillingConstraints(
                request->characteristic, request->service, request->accessory, numValueBytes)) {
        return kHAPError_InvalidData;
    }

    // Call handler.
    HAPLogCharacteristicInfo(
            &logObject, request->characteristic, request->service, request->accessory, "Calling write handler.");
    WillHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    err = request->characteristic->callbacks.handleWrite(server, request, valueBytes, numValueBytes, context);
    DidHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                err == kHAPError_OutOfResources || err == kHAPError_NotAuthorized || err == kHAPError_Busy);
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Write handler failed with error %u.",
                err);
        return err;
    }

    return kHAPError_None;
}

void HAPDataCharacteristicHandleSubscribe(
        HAPAccessoryServer* server,
        const HAPDataCharacteristicSubscriptionRequest* request,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_Data);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->accessory);

    if (request->characteristic->callbacks.handleSubscribe) {
        // Call handler.
        HAPLogCharacteristicInfo(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Calling subscribe handler.");
        WillHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
        request->characteristic->callbacks.handleSubscribe(server, request, context);
        DidHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    }
}

void HAPDataCharacteristicHandleUnsubscribe(
        HAPAccessoryServer* server,
        const HAPDataCharacteristicSubscriptionRequest* request,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_Data);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->accessory);

    if (request->characteristic->callbacks.handleUnsubscribe) {
        // Call handler.
        HAPLogCharacteristicInfo(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Calling unsubscribe handler.");
        WillHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
        request->characteristic->callbacks.handleUnsubscribe(server, request, context);
        DidHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HAP_RESULT_USE_CHECK
static bool HAPBoolCharacteristicIsValueFulfillingConstraints(
        const HAPBoolCharacteristic* characteristic,
        const HAPService* service HAP_UNUSED,
        const HAPAccessory* accessory,
        bool value) {
    if (value != false && value != true) {
        HAPLogCharacteristic(&logObject, characteristic, service, accessory, "Value invalid: %u.", value);
        return false;
    }

    return true;
}

HAP_RESULT_USE_CHECK
HAPError HAPBoolCharacteristicHandleRead(
        HAPAccessoryServer* server,
        const HAPBoolCharacteristicReadRequest* request,
        bool* value,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_Bool);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->characteristic->callbacks.handleRead);
    HAPPrecondition(request->accessory);
    HAPPrecondition(value);

    HAPError err;

    // Call handler.
    HAPLogCharacteristicInfo(
            &logObject, request->characteristic, request->service, request->accessory, "Calling read handler.");
    err = request->characteristic->callbacks.handleRead(server, request, value, context);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                err == kHAPError_Busy);
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Read handler failed with error %u.",
                err);
        return err;
    }

    // Validate constraints.
    HAPAssert(HAPBoolCharacteristicIsValueFulfillingConstraints(
            request->characteristic, request->service, request->accessory, *value));

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPBoolCharacteristicHandleWrite(
        HAPAccessoryServer* server,
        const HAPBoolCharacteristicWriteRequest* request,
        bool value,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_Bool);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->characteristic->callbacks.handleWrite);
    HAPPrecondition(request->accessory);

    HAPError err;

    // Validate constraints.
    if (!HAPBoolCharacteristicIsValueFulfillingConstraints(
                request->characteristic, request->service, request->accessory, value)) {
        return kHAPError_InvalidData;
    }

    // Call handler.
    HAPLogCharacteristicInfo(
            &logObject, request->characteristic, request->service, request->accessory, "Calling write handler.");
    WillHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    err = request->characteristic->callbacks.handleWrite(server, request, value, context);
    DidHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                err == kHAPError_OutOfResources || err == kHAPError_NotAuthorized || err == kHAPError_Busy);
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Write handler failed with error %u.",
                err);
        return err;
    }

    return kHAPError_None;
}

void HAPBoolCharacteristicHandleSubscribe(
        HAPAccessoryServer* server,
        const HAPBoolCharacteristicSubscriptionRequest* request,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_Bool);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->accessory);

    if (request->characteristic->callbacks.handleSubscribe) {
        // Call handler.
        HAPLogCharacteristicInfo(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Calling subscribe handler.");
        WillHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
        request->characteristic->callbacks.handleSubscribe(server, request, context);
        DidHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    }
}

void HAPBoolCharacteristicHandleUnsubscribe(
        HAPAccessoryServer* server,
        const HAPBoolCharacteristicSubscriptionRequest* request,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_Bool);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->accessory);

    if (request->characteristic->callbacks.handleUnsubscribe) {
        // Call handler.
        HAPLogCharacteristicInfo(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Calling unsubscribe handler.");
        WillHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
        request->characteristic->callbacks.handleUnsubscribe(server, request, context);
        DidHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HAP_RESULT_USE_CHECK
static bool IsInValidValues(uint8_t value, const uint8_t* const* validValues) {
    if (validValues != NULL) {
        size_t i = 0;
        while (validValues[i] && *validValues[i] != value) {
            i++;
        }
        if (validValues[i] == NULL) {
            return false;
        }
    }
    return true;
}

HAP_RESULT_USE_CHECK
static bool
        IsInValidValuesRanges(uint8_t value, const HAPUInt8CharacteristicValidValuesRange* const* validValuesRanges) {
    if (validValuesRanges != NULL) {
        size_t i = 0;
        while (validValuesRanges[i] && (value < validValuesRanges[i]->start || value > validValuesRanges[i]->end)) {
            i++;
        }
        if (validValuesRanges[i] == NULL) {
            return false;
        }
    }
    return true;
}

HAP_RESULT_USE_CHECK
static bool HAPUInt8CharacteristicIsValueFulfillingConstraints(
        const HAPUInt8Characteristic* characteristic,
        const HAPService* service HAP_UNUSED,
        const HAPAccessory* accessory,
        uint8_t value) {
    if (!IS_VALUE_IN_RANGE(value, characteristic->constraints)) {
        HAPLogCharacteristic(
                &logObject,
                characteristic,
                service,
                accessory,
                "Value out of range: %u (constraints: minimumValue = %u / maximumValue = %u / stepValue = %u).",
                value,
                characteristic->constraints.minimumValue,
                characteristic->constraints.maximumValue,
                characteristic->constraints.stepValue);
        return false;
    }
    if (HAPUUIDIsAppleDefined(characteristic->characteristicType)) {
        if (!characteristic->constraints.validValues && !characteristic->constraints.validValuesRanges) {
            return true;
        }
        if (characteristic->constraints.validValues &&
            IsInValidValues(value, characteristic->constraints.validValues)) {
            return true;
        }
        if (characteristic->constraints.validValuesRanges &&
            IsInValidValuesRanges(value, characteristic->constraints.validValuesRanges)) {
            return true;
        }
        HAPLogCharacteristic(
                &logObject,
                characteristic,
                request->service,
                accessory,
                "Value not supported: %u (constraints: validValues / validValuesRanges).",
                value);
        return false;
    }
    HAPAssert(!characteristic->constraints.validValues);
    HAPAssert(!characteristic->constraints.validValuesRanges);
    return true;
}

HAP_RESULT_USE_CHECK
HAPError HAPUInt8CharacteristicHandleRead(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_UInt8);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->characteristic->callbacks.handleRead);
    HAPPrecondition(request->accessory);
    HAPPrecondition(value);

    HAPError err;

    // Call handler.
    HAPLogCharacteristicInfo(
            &logObject, request->characteristic, request->service, request->accessory, "Calling read handler.");
    err = request->characteristic->callbacks.handleRead(server, request, value, context);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                err == kHAPError_Busy);
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Read handler failed with error %u.",
                err);
        return err;
    }

    // Validate constraints.
    HAPAssert(HAPUInt8CharacteristicIsValueFulfillingConstraints(
            request->characteristic, request->service, request->accessory, *value));

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPUInt8CharacteristicHandleWrite(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_UInt8);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->characteristic->callbacks.handleWrite);
    HAPPrecondition(request->accessory);

    HAPError err;

    // Validate constraints.
    if (!HAPUInt8CharacteristicIsValueFulfillingConstraints(
                request->characteristic, request->service, request->accessory, value)) {
        return kHAPError_InvalidData;
    }

    // Call handler.
    HAPLogCharacteristicInfo(
            &logObject, request->characteristic, request->service, request->accessory, "Calling write handler.");
    WillHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    err = request->characteristic->callbacks.handleWrite(server, request, value, context);
    DidHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                err == kHAPError_OutOfResources || err == kHAPError_NotAuthorized || err == kHAPError_Busy);
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Write handler failed with error %u.",
                err);
        return err;
    }

    return kHAPError_None;
}

void HAPUInt8CharacteristicHandleSubscribe(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicSubscriptionRequest* request,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_UInt8);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->accessory);

    if (request->characteristic->callbacks.handleSubscribe) {
        // Call handler.
        HAPLogCharacteristicInfo(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Calling subscribe handler.");
        WillHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
        request->characteristic->callbacks.handleSubscribe(server, request, context);
        DidHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    }
}

void HAPUInt8CharacteristicHandleUnsubscribe(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicSubscriptionRequest* request,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_UInt8);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->accessory);

    if (request->characteristic->callbacks.handleUnsubscribe) {
        // Call handler.
        HAPLogCharacteristicInfo(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Calling unsubscribe handler.");
        WillHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
        request->characteristic->callbacks.handleUnsubscribe(server, request, context);
        DidHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HAP_RESULT_USE_CHECK
static bool HAPUInt16CharacteristicIsValueFulfillingConstraints(
        const HAPUInt16Characteristic* characteristic,
        const HAPService* service HAP_UNUSED,
        const HAPAccessory* accessory,
        uint16_t value) {
    if (!IS_VALUE_IN_RANGE(value, characteristic->constraints)) {
        HAPLogCharacteristic(
                &logObject,
                characteristic,
                service,
                accessory,
                "Value out of range: %u (constraints: minimumValue = %u / maximumValue = %u / stepValue = %u).",
                value,
                characteristic->constraints.minimumValue,
                characteristic->constraints.maximumValue,
                characteristic->constraints.stepValue);
        return false;
    }

    return true;
}

HAP_RESULT_USE_CHECK
HAPError HAPUInt16CharacteristicHandleRead(
        HAPAccessoryServer* server,
        const HAPUInt16CharacteristicReadRequest* request,
        uint16_t* value,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_UInt16);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->characteristic->callbacks.handleRead);
    HAPPrecondition(request->accessory);
    HAPPrecondition(value);

    HAPError err;

    // Call handler.
    HAPLogCharacteristicInfo(
            &logObject, request->characteristic, request->service, request->accessory, "Calling read handler.");
    err = request->characteristic->callbacks.handleRead(server, request, value, context);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                err == kHAPError_Busy);
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Read handler failed with error %u.",
                err);
        return err;
    }

    // Validate constraints.
    HAPAssert(HAPUInt16CharacteristicIsValueFulfillingConstraints(
            request->characteristic, request->service, request->accessory, *value));

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPUInt16CharacteristicHandleWrite(
        HAPAccessoryServer* server,
        const HAPUInt16CharacteristicWriteRequest* request,
        uint16_t value,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_UInt16);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->characteristic->callbacks.handleWrite);
    HAPPrecondition(request->accessory);

    HAPError err;

    // Validate constraints.
    if (!HAPUInt16CharacteristicIsValueFulfillingConstraints(
                request->characteristic, request->service, request->accessory, value)) {
        return kHAPError_InvalidData;
    }

    // Call handler.
    HAPLogCharacteristicInfo(
            &logObject, request->characteristic, request->service, request->accessory, "Calling write handler.");
    WillHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    err = request->characteristic->callbacks.handleWrite(server, request, value, context);
    DidHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                err == kHAPError_OutOfResources || err == kHAPError_NotAuthorized || err == kHAPError_Busy);
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Write handler failed with error %u.",
                err);
        return err;
    }

    return kHAPError_None;
}

void HAPUInt16CharacteristicHandleSubscribe(
        HAPAccessoryServer* server,
        const HAPUInt16CharacteristicSubscriptionRequest* request,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_UInt16);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->accessory);

    if (request->characteristic->callbacks.handleSubscribe) {
        // Call handler.
        HAPLogCharacteristicInfo(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Calling subscribe handler.");
        WillHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
        request->characteristic->callbacks.handleSubscribe(server, request, context);
        DidHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    }
}

void HAPUInt16CharacteristicHandleUnsubscribe(
        HAPAccessoryServer* server,
        const HAPUInt16CharacteristicSubscriptionRequest* request,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_UInt16);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->accessory);

    if (request->characteristic->callbacks.handleUnsubscribe) {
        // Call handler.
        HAPLogCharacteristicInfo(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Calling unsubscribe handler.");
        WillHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
        request->characteristic->callbacks.handleUnsubscribe(server, request, context);
        DidHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HAP_RESULT_USE_CHECK
static bool HAPUInt32CharacteristicIsValueFulfillingConstraints(
        const HAPUInt32Characteristic* characteristic,
        const HAPService* service HAP_UNUSED,
        const HAPAccessory* accessory,
        uint32_t value) {
    if (!IS_VALUE_IN_RANGE(value, characteristic->constraints)) {
        HAPLogCharacteristic(
                &logObject,
                characteristic,
                service,
                accessory,
                "Value out of range: %lu (constraints: minimumValue = %lu / maximumValue = %lu / stepValue = %lu).",
                (unsigned long) value,
                (unsigned long) characteristic->constraints.minimumValue,
                (unsigned long) characteristic->constraints.maximumValue,
                (unsigned long) characteristic->constraints.stepValue);
        return false;
    }

    return true;
}

HAP_RESULT_USE_CHECK
HAPError HAPUInt32CharacteristicHandleRead(
        HAPAccessoryServer* server,
        const HAPUInt32CharacteristicReadRequest* request,
        uint32_t* value,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_UInt32);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->characteristic->callbacks.handleRead);
    HAPPrecondition(request->accessory);
    HAPPrecondition(value);

    HAPError err;

    // Call handler.
    HAPLogCharacteristicInfo(
            &logObject, request->characteristic, request->service, request->accessory, "Calling read handler.");
    err = request->characteristic->callbacks.handleRead(server, request, value, context);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                err == kHAPError_Busy);
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Read handler failed with error %u.",
                err);
        return err;
    }

    // Validate constraints.
    HAPAssert(HAPUInt32CharacteristicIsValueFulfillingConstraints(
            request->characteristic, request->service, request->accessory, *value));

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPUInt32CharacteristicHandleWrite(
        HAPAccessoryServer* server,
        const HAPUInt32CharacteristicWriteRequest* request,
        uint32_t value,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_UInt32);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->characteristic->callbacks.handleWrite);
    HAPPrecondition(request->accessory);

    HAPError err;

    // Validate constraints.
    if (!HAPUInt32CharacteristicIsValueFulfillingConstraints(
                request->characteristic, request->service, request->accessory, value)) {
        return kHAPError_InvalidData;
    }

    // Call handler.
    HAPLogCharacteristicInfo(
            &logObject, request->characteristic, request->service, request->accessory, "Calling write handler.");
    WillHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    err = request->characteristic->callbacks.handleWrite(server, request, value, context);
    DidHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                err == kHAPError_OutOfResources || err == kHAPError_NotAuthorized || err == kHAPError_Busy);
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Write handler failed with error %u.",
                err);
        return err;
    }

    return kHAPError_None;
}

void HAPUInt32CharacteristicHandleSubscribe(
        HAPAccessoryServer* server,
        const HAPUInt32CharacteristicSubscriptionRequest* request,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_UInt32);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->accessory);

    if (request->characteristic->callbacks.handleSubscribe) {
        // Call handler.
        HAPLogCharacteristicInfo(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Calling subscribe handler.");
        WillHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
        request->characteristic->callbacks.handleSubscribe(server, request, context);
        DidHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    }
}

void HAPUInt32CharacteristicHandleUnsubscribe(
        HAPAccessoryServer* server,
        const HAPUInt32CharacteristicSubscriptionRequest* request,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_UInt32);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->accessory);

    if (request->characteristic->callbacks.handleUnsubscribe) {
        // Call handler.
        HAPLogCharacteristicInfo(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Calling unsubscribe handler.");
        WillHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
        request->characteristic->callbacks.handleUnsubscribe(server, request, context);
        DidHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HAP_RESULT_USE_CHECK
static bool HAPUInt64CharacteristicIsValueFulfillingConstraints(
        const HAPUInt64Characteristic* characteristic,
        const HAPService* service HAP_UNUSED,
        const HAPAccessory* accessory,
        uint64_t value) {
    if (!IS_VALUE_IN_RANGE(value, characteristic->constraints)) {
        HAPLogCharacteristic(
                &logObject,
                characteristic,
                service,
                accessory,
                "Value out of range: %llu (constraints: minimumValue = %llu / maximumValue = %llu / stepValue = %llu).",
                (unsigned long long) value,
                (unsigned long long) characteristic->constraints.minimumValue,
                (unsigned long long) characteristic->constraints.maximumValue,
                (unsigned long long) characteristic->constraints.stepValue);
        return false;
    }

    return true;
}

HAP_RESULT_USE_CHECK
HAPError HAPUInt64CharacteristicHandleRead(
        HAPAccessoryServer* server,
        const HAPUInt64CharacteristicReadRequest* request,
        uint64_t* value,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_UInt64);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->characteristic->callbacks.handleRead);
    HAPPrecondition(request->accessory);
    HAPPrecondition(value);

    HAPError err;

    // Call handler.
    HAPLogCharacteristicInfo(
            &logObject, request->characteristic, request->service, request->accessory, "Calling read handler.");
    err = request->characteristic->callbacks.handleRead(server, request, value, context);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                err == kHAPError_Busy);
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Read handler failed with error %u.",
                err);
        return err;
    }

    // Validate constraints.
    HAPAssert(HAPUInt64CharacteristicIsValueFulfillingConstraints(
            request->characteristic, request->service, request->accessory, *value));

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPUInt64CharacteristicHandleWrite(
        HAPAccessoryServer* server,
        const HAPUInt64CharacteristicWriteRequest* request,
        uint64_t value,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_UInt64);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->characteristic->callbacks.handleWrite);
    HAPPrecondition(request->accessory);

    HAPError err;

    // Validate constraints.
    if (!HAPUInt64CharacteristicIsValueFulfillingConstraints(
                request->characteristic, request->service, request->accessory, value)) {
        return kHAPError_InvalidData;
    }

    // Call handler.
    HAPLogCharacteristicInfo(
            &logObject, request->characteristic, request->service, request->accessory, "Calling write handler.");
    WillHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    err = request->characteristic->callbacks.handleWrite(server, request, value, context);
    DidHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                err == kHAPError_OutOfResources || err == kHAPError_NotAuthorized || err == kHAPError_Busy);
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Write handler failed with error %u.",
                err);
        return err;
    }

    return kHAPError_None;
}

void HAPUInt64CharacteristicHandleSubscribe(
        HAPAccessoryServer* server,
        const HAPUInt64CharacteristicSubscriptionRequest* request,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_UInt64);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->accessory);

    if (request->characteristic->callbacks.handleSubscribe) {
        // Call handler.
        HAPLogCharacteristicInfo(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Calling subscribe handler.");
        WillHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
        request->characteristic->callbacks.handleSubscribe(server, request, context);
        DidHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    }
}

void HAPUInt64CharacteristicHandleUnsubscribe(
        HAPAccessoryServer* server,
        const HAPUInt64CharacteristicSubscriptionRequest* request,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_UInt64);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->accessory);

    if (request->characteristic->callbacks.handleUnsubscribe) {
        // Call handler.
        HAPLogCharacteristicInfo(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Calling unsubscribe handler.");
        WillHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
        request->characteristic->callbacks.handleUnsubscribe(server, request, context);
        DidHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HAP_RESULT_USE_CHECK
static bool HAPIntCharacteristicIsValueFulfillingConstraints(
        const HAPIntCharacteristic* characteristic,
        const HAPService* service HAP_UNUSED,
        const HAPAccessory* accessory,
        int32_t value) {
    if (!IS_VALUE_IN_RANGE(value, characteristic->constraints)) {
        HAPLogCharacteristic(
                &logObject,
                characteristic,
                service,
                accessory,
                "Value out of range: %ld (constraints: minimumValue = %ld / maximumValue = %ld / stepValue = %ld).",
                (long) value,
                (long) characteristic->constraints.minimumValue,
                (long) characteristic->constraints.maximumValue,
                (long) characteristic->constraints.stepValue);
        return false;
    }

    return true;
}

HAP_RESULT_USE_CHECK
HAPError HAPIntCharacteristicHandleRead(
        HAPAccessoryServer* server,
        const HAPIntCharacteristicReadRequest* request,
        int32_t* value,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_Int);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->characteristic->callbacks.handleRead);
    HAPPrecondition(request->accessory);
    HAPPrecondition(value);

    HAPError err;

    // Call handler.
    HAPLogCharacteristicInfo(
            &logObject, request->characteristic, request->service, request->accessory, "Calling read handler.");
    err = request->characteristic->callbacks.handleRead(server, request, value, context);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                err == kHAPError_Busy);
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Read handler failed with error %u.",
                err);
        return err;
    }

    // Validate constraints.
    HAPAssert(HAPIntCharacteristicIsValueFulfillingConstraints(
            request->characteristic, request->service, request->accessory, *value));

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPIntCharacteristicHandleWrite(
        HAPAccessoryServer* server,
        const HAPIntCharacteristicWriteRequest* request,
        int32_t value,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_Int);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->characteristic->callbacks.handleWrite);
    HAPPrecondition(request->accessory);

    HAPError err;

    // Validate constraints.
    if (!HAPIntCharacteristicIsValueFulfillingConstraints(
                request->characteristic, request->service, request->accessory, value)) {
        return kHAPError_InvalidData;
    }

    // Call handler.
    HAPLogCharacteristicInfo(
            &logObject, request->characteristic, request->service, request->accessory, "Calling write handler.");
    WillHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    err = request->characteristic->callbacks.handleWrite(server, request, value, context);
    DidHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                err == kHAPError_OutOfResources || err == kHAPError_NotAuthorized || err == kHAPError_Busy);
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Write handler failed with error %u.",
                err);
        return err;
    }

    return kHAPError_None;
}

void HAPIntCharacteristicHandleSubscribe(
        HAPAccessoryServer* server,
        const HAPIntCharacteristicSubscriptionRequest* request,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_Int);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->accessory);

    if (request->characteristic->callbacks.handleSubscribe) {
        // Call handler.
        HAPLogCharacteristicInfo(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Calling subscribe handler.");
        WillHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
        request->characteristic->callbacks.handleSubscribe(server, request, context);
        DidHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    }
}

void HAPIntCharacteristicHandleUnsubscribe(
        HAPAccessoryServer* server,
        const HAPIntCharacteristicSubscriptionRequest* request,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_Int);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->accessory);

    if (request->characteristic->callbacks.handleUnsubscribe) {
        // Call handler.
        HAPLogCharacteristicInfo(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Calling unsubscribe handler.");
        WillHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
        request->characteristic->callbacks.handleUnsubscribe(server, request, context);
        DidHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HAP_RESULT_USE_CHECK
static bool HAPFloatCharacteristicIsValueFulfillingConstraints(
        const HAPFloatCharacteristic* characteristic,
        const HAPService* service HAP_UNUSED,
        const HAPAccessory* accessory,
        float value) {
    if (HAPFloatIsInfinite(value)) {
        if (value > 0) {
            if (!HAPFloatIsInfinite(characteristic->constraints.maximumValue) ||
                characteristic->constraints.maximumValue < 0) {
                HAPLogCharacteristic(
                        &logObject,
                        characteristic,
                        service,
                        accessory,
                        "Value out of range: %g (constraints: minimumValue = %g / maximumValue = %g / stepValue = %g).",
                        (double) value,
                        (double) characteristic->constraints.minimumValue,
                        (double) characteristic->constraints.maximumValue,
                        (double) characteristic->constraints.stepValue);
                return false;
            }
        } else {
            if (!HAPFloatIsInfinite(characteristic->constraints.minimumValue) ||
                characteristic->constraints.minimumValue > 0) {
                HAPLogCharacteristic(
                        &logObject,
                        characteristic,
                        service,
                        accessory,
                        "Value out of range: %g (constraints: minimumValue = %g / maximumValue = %g / stepValue = %g).",
                        (double) value,
                        (double) characteristic->constraints.minimumValue,
                        (double) characteristic->constraints.maximumValue,
                        (double) characteristic->constraints.stepValue);
                return false;
            }
        }
    } else {
        if (!HAPFloatIsFinite(value)) {
            HAPLogCharacteristic(
                    &logObject,
                    characteristic,
                    service,
                    accessory,
                    "Value out of range: %g (constraints: minimumValue = %g / maximumValue = %g / stepValue = %g).",
                    (double) value,
                    (double) characteristic->constraints.minimumValue,
                    (double) characteristic->constraints.maximumValue,
                    (double) characteristic->constraints.stepValue);
            return false;
        }
        if (!IS_VALUE_IN_RANGE_WITH_TOLERANCE(value, characteristic->constraints, /* tolerance: */ 0.1F)) {
            HAPLogCharacteristic(
                    &logObject,
                    characteristic,
                    service,
                    accessory,
                    "Value out of range: %g (constraints: minimumValue = %g / maximumValue = %g / stepValue = %g).",
                    (double) value,
                    (double) characteristic->constraints.minimumValue,
                    (double) characteristic->constraints.maximumValue,
                    (double) characteristic->constraints.stepValue);
            return false;
        }
    }

    return true;
}

HAP_RESULT_USE_CHECK
static float HAPFloatCharacteristicRoundValueToStep(const HAPFloatCharacteristic* characteristic, float value) {
    if (!HAPFloatIsZero(characteristic->constraints.stepValue)) {
        value = ROUND_VALUE_TO_STEP(value, characteristic->constraints);
    }

    return value;
}

HAP_RESULT_USE_CHECK
HAPError HAPFloatCharacteristicHandleRead(
        HAPAccessoryServer* server,
        const HAPFloatCharacteristicReadRequest* request,
        float* value,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_Float);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->characteristic->callbacks.handleRead);
    HAPPrecondition(request->accessory);
    HAPPrecondition(value);

    HAPError err;

    // Call handler.
    HAPLogCharacteristicInfo(
            &logObject, request->characteristic, request->service, request->accessory, "Calling read handler.");
    err = request->characteristic->callbacks.handleRead(server, request, value, context);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                err == kHAPError_Busy);
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Read handler failed with error %u.",
                err);
        return err;
    }

    // Validate constraints.
    HAPAssert(HAPFloatCharacteristicIsValueFulfillingConstraints(
            request->characteristic, request->service, request->accessory, *value));

    // Round to step.
    *value = HAPFloatCharacteristicRoundValueToStep(request->characteristic, *value);

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPFloatCharacteristicHandleWrite(
        HAPAccessoryServer* server,
        const HAPFloatCharacteristicWriteRequest* request,
        float value,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_Float);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->characteristic->callbacks.handleWrite);
    HAPPrecondition(request->accessory);

    HAPError err;

    // Validate constraints.
    if (!HAPFloatCharacteristicIsValueFulfillingConstraints(
                request->characteristic, request->service, request->accessory, value)) {
        return kHAPError_InvalidData;
    }

    // Round to step.
    value = HAPFloatCharacteristicRoundValueToStep(request->characteristic, value);

    // Call handler.
    HAPLogCharacteristicInfo(
            &logObject, request->characteristic, request->service, request->accessory, "Calling write handler.");
    WillHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    err = request->characteristic->callbacks.handleWrite(server, request, value, context);
    DidHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                err == kHAPError_OutOfResources || err == kHAPError_NotAuthorized || err == kHAPError_Busy);
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Write handler failed with error %u.",
                err);
        return err;
    }

    return kHAPError_None;
}

void HAPFloatCharacteristicHandleSubscribe(
        HAPAccessoryServer* server,
        const HAPFloatCharacteristicSubscriptionRequest* request,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_Float);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->accessory);

    if (request->characteristic->callbacks.handleSubscribe) {
        // Call handler.
        HAPLogCharacteristicInfo(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Calling subscribe handler.");
        WillHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
        request->characteristic->callbacks.handleSubscribe(server, request, context);
        DidHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    }
}

void HAPFloatCharacteristicHandleUnsubscribe(
        HAPAccessoryServer* server,
        const HAPFloatCharacteristicSubscriptionRequest* request,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_Float);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->accessory);

    if (request->characteristic->callbacks.handleUnsubscribe) {
        // Call handler.
        HAPLogCharacteristicInfo(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Calling unsubscribe handler.");
        WillHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
        request->characteristic->callbacks.handleUnsubscribe(server, request, context);
        DidHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HAP_RESULT_USE_CHECK
static bool HAPStringCharacteristicIsValueFulfillingConstraints(
        const HAPStringCharacteristic* characteristic,
        const HAPService* service HAP_UNUSED,
        const HAPAccessory* accessory,
        const char* value) {
    size_t numValueBytes = HAPStringGetNumBytes(value);
    if (!IS_LENGTH_IN_RANGE(HAPStringGetNumBytes(value), characteristic->constraints)) {
        HAPLogCharacteristic(
                &logObject,
                characteristic,
                service,
                accessory,
                "Value too long: %zu bytes (constraints: maxLength = %u bytes).",
                numValueBytes,
                characteristic->constraints.maxLength);
        return false;
    }

    return true;
}

HAP_RESULT_USE_CHECK
HAPError HAPStringCharacteristicHandleRead(
        HAPAccessoryServer* server,
        const HAPStringCharacteristicReadRequest* request,
        char* value,
        size_t maxValueBytes,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_String);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->characteristic->callbacks.handleRead);
    HAPPrecondition(request->accessory);
    HAPPrecondition(value);

    HAPError err;

    // We require at least 1 byte for the NULL-terminator.
    if (!maxValueBytes) {
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Not enough space to store value: Need 1 byte for NULL-terminator.");
        return kHAPError_OutOfResources;
    }

    // Set NULL-terminator.
    value[maxValueBytes - 1] = '\0';

    // Call handler.
    HAPLogCharacteristicInfo(
            &logObject, request->characteristic, request->service, request->accessory, "Calling read handler.");
    err = request->characteristic->callbacks.handleRead(server, request, value, maxValueBytes, context);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                err == kHAPError_Busy);
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Read handler failed with error %u.",
                err);
        return err;
    }

    // Validate that NULL-terminator is still present.
    if (value[maxValueBytes - 1] != '\0') {
        HAPLogCharacteristicError(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Read string value exceeds available buffer space (available %zu bytes, including NULL-terminator).",
                maxValueBytes);
        HAPFatalError();
    }

    // Validate UTF-8 encoding.
    if (!HAPUTF8IsValidData(value, HAPStringGetNumBytes(value))) {
        HAPLogCharacteristicError(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Read string value is not valid UTF-8.");
        HAPFatalError();
    }

    // Validate constraints.
    HAPAssert(HAPStringCharacteristicIsValueFulfillingConstraints(
            request->characteristic, request->service, request->accessory, value));

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPStringCharacteristicHandleWrite(
        HAPAccessoryServer* server,
        const HAPStringCharacteristicWriteRequest* request,
        const char* value,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_String);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->characteristic->callbacks.handleWrite);
    HAPPrecondition(request->accessory);
    HAPPrecondition(value);

    HAPError err;

    // Validate constraints.
    if (!HAPStringCharacteristicIsValueFulfillingConstraints(
                request->characteristic, request->service, request->accessory, value)) {
        return kHAPError_InvalidData;
    }

    // Call handler.
    HAPLogCharacteristicInfo(
            &logObject, request->characteristic, request->service, request->accessory, "Calling write handler.");
    WillHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    err = request->characteristic->callbacks.handleWrite(server, request, value, context);
    DidHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                err == kHAPError_OutOfResources || err == kHAPError_NotAuthorized || err == kHAPError_Busy);
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Write handler failed with error %u.",
                err);
        return err;
    }

    return kHAPError_None;
}

void HAPStringCharacteristicHandleSubscribe(
        HAPAccessoryServer* server,
        const HAPStringCharacteristicSubscriptionRequest* request,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_String);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->accessory);

    if (request->characteristic->callbacks.handleSubscribe) {
        // Call handler.
        HAPLogCharacteristicInfo(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Calling subscribe handler.");
        WillHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
        request->characteristic->callbacks.handleSubscribe(server, request, context);
        DidHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    }
}

void HAPStringCharacteristicHandleUnsubscribe(
        HAPAccessoryServer* server,
        const HAPStringCharacteristicSubscriptionRequest* request,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_String);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->accessory);

    if (request->characteristic->callbacks.handleUnsubscribe) {
        // Call handler.
        HAPLogCharacteristicInfo(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Calling unsubscribe handler.");
        WillHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
        request->characteristic->callbacks.handleUnsubscribe(server, request, context);
        DidHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HAP_RESULT_USE_CHECK
HAPError HAPTLV8CharacteristicHandleRead(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_TLV8);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->characteristic->callbacks.handleRead);
    HAPPrecondition(request->accessory);
    HAPPrecondition(responseWriter);

    HAPError err;

    // Call handler.
    HAPLogCharacteristicInfo(
            &logObject, request->characteristic, request->service, request->accessory, "Calling read handler.");
    err = request->characteristic->callbacks.handleRead(server, request, responseWriter, context);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                err == kHAPError_Busy);
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Read handler failed with error %u.",
                err);
        return err;
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPTLV8CharacteristicHandleWrite(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicWriteRequest* request,
        HAPTLVReader* requestReader,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_TLV8);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->characteristic->callbacks.handleWrite);
    HAPPrecondition(request->accessory);
    HAPPrecondition(requestReader);

    HAPError err;

    // Call handler.
    HAPLogCharacteristicInfo(
            &logObject, request->characteristic, request->service, request->accessory, "Calling write handler.");
    WillHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    err = request->characteristic->callbacks.handleWrite(server, request, requestReader, context);
    DidHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                err == kHAPError_OutOfResources || err == kHAPError_NotAuthorized || err == kHAPError_Busy);
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Write handler failed with error %u.",
                err);
        return err;
    }

    return kHAPError_None;
}

void HAPTLV8CharacteristicHandleSubscribe(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicSubscriptionRequest* request,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_TLV8);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->accessory);

    if (request->characteristic->callbacks.handleSubscribe) {
        // Call handler.
        HAPLogCharacteristicInfo(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Calling subscribe handler.");
        WillHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
        request->characteristic->callbacks.handleSubscribe(server, request, context);
        DidHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    }
}

void HAPTLV8CharacteristicHandleUnsubscribe(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicSubscriptionRequest* request,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->characteristic);
    HAPPrecondition(request->characteristic->format == kHAPCharacteristicFormat_TLV8);
    HAPPrecondition(request->characteristic->debugDescription);
    HAPPrecondition(request->accessory);

    if (request->characteristic->callbacks.handleUnsubscribe) {
        // Call handler.
        HAPLogCharacteristicInfo(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Calling unsubscribe handler.");
        WillHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
        request->characteristic->callbacks.handleUnsubscribe(server, request, context);
        DidHandleWrite(server, request->session, request->characteristic, request->service, request->accessory);
    }
}
