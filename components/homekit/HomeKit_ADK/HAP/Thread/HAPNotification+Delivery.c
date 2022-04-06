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

#include "HAPPlatformFeatures.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)

#include "HAPAccessoryServer+Info.h"
#include "HAPAccessoryServer+Internal.h"
#include "HAPBitSet.h"
#include "HAPCharacteristic.h"
#include "HAPCrypto.h"
#include "HAPLog+Attributes.h"
#include "HAPLogSubsystem.h"
#include "HAPPDU+CharacteristicValue.h"
#include "HAPSession.h"
#include "HAPTLV+Internal.h"
#include "HAPThreadAccessoryServer+Reachability.h"
#include "HAPThreadSessionStorage.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "Notification" };

// Control Field Bit 1-3 Values.
// - 0 / 1 / 0 / Event
//
// See HomeKit Accessory Protocol Specification R17
// Section 7.3.3.1 HAP PDU Header - Control Field

// HAP Event Format.
// The HAP Event Format is shown in this table.
//
// +---------------------------+-----------------------------------------------------+
// |         PDU Header        |                      PDU Body                       |
// +===============+===========+=============+=======================================+
// | Control Field |  CharID   | Body Length | Additional Params and Values in TLV8s |
// |   (1 Byte)    | (2 Bytes) |  (2 Bytes)  |              (1-n Bytes)              |
// +---------------+-----------+-------------+---------------------------------------+
//
// - CharID: Characteristic Instance Identifier is the instance ID of the characteristic for a particular event.
//
// See HomeKit Accessory Protocol Specification R17
// Section 7.3.3.2 HAP Request Format
//
// See HomeKit Accessory Protocol Specification R17
// Section 7.3.3.3 HAP Response Format

// HAP-Notification-Event.
//
// +---------------------------+-----------------------------------+
// |        PDU Header         |             PDU Body              |
// +===============+===========+=============+=====================+
// | Control Field |  CharID   | Body Length |         TLV         |
// |   (1 Byte)    | (2 Bytes) |  (2 Bytes)  |    (Char Value)     |
// +---------------+-----------+-------------+---------------------+
// | 0b 0000 0100  |  0xXXXX   |    0xXXXX   | <0x01, 0xXX, value> |
// +---------------+-----------+-------------+---------------------+
//
// See HomeKit Accessory Protocol Specification R17
// Section 7.3.4.7 HAP-Characteristic-Read-Response

// HAP Characteristic Notification Procedure.
// This procedure is used to send HAP event notifications.
//
// When a controller is connected to an accessory the controller may choose to register for event notifications for
// some characteristics. When an update occurs on those characteristics, the accessory must send an event notification.
// Event notifications shall not be sent to the controller that changed the value of a characteristic if the change was
// due to a write from the controller. Event notifications must not be delivered when the session is not secured,
// or when using session security based on transient Pair Setup (Software Authentication).
//
// Event notification PDUs must be encrypted using the EventAccessoryToControllerKey and use a separate nonce than
// the one used for control messages (requests and responses).
//
// 1. HAP-PDU: HAP-Notification-Event
//
// See HomeKit Accessory Protocol Specification R17
// Section 7.3.5.3 HAP Characteristic Read Procedure
//
// See HomeKit Accessory Protocol Specification R17
// Section 7.4.6.1 Connected Events

HAP_RESULT_USE_CHECK
static HAPError GetNotificationData(
        HAPAccessoryServer* server,
        HAPSession* session,
        const HAPCharacteristic* characteristic_,
        const HAPService* service,
        const HAPAccessory* accessory,
        void* bytes_,
        size_t maxBytes,
        size_t* numBytes) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(characteristic_);
    const HAPBaseCharacteristic* characteristic = characteristic_;
    HAPPrecondition(service);
    HAPPrecondition(accessory);
    HAPPrecondition(bytes_);
    uint8_t* bytes = bytes_;
    HAPPrecondition(numBytes);

    HAPError err;

    size_t numHeaderBytes = 5;

    if (maxBytes < numHeaderBytes + CHACHA20_POLY1305_TAG_BYTES) {
        HAPLogError(
                &logObject,
                "Session storage buffer is not large enough (need %zu bytes more).",
                numHeaderBytes + CHACHA20_POLY1305_TAG_BYTES - maxBytes);
        return kHAPError_OutOfResources;
    }

    // Control Field.
    bytes[0] = (uint8_t)(
            (uint8_t)(0U << 7U) |                                             // First Fragment (or no fragmentation).
            (uint8_t)(0U << 4U) |                                             // 16 bit IIDs (or IID = 0).
            (uint8_t)(0U << 3U) | (uint8_t)(1U << 2U) | (uint8_t)(0U << 1U) | // Event.
            (uint8_t)(0U << 0U));                                             // 1 Byte Control Field.

    // CharID.
    HAPWriteLittleUInt16(&bytes[1], (uint16_t) characteristic->iid);

    // Body.
    HAPTLVWriter bodyWriter;
    HAPTLVWriterCreate(
            &bodyWriter,
            &bytes[numHeaderBytes],
            HAPMin(maxBytes - numHeaderBytes - CHACHA20_POLY1305_TAG_BYTES, UINT16_MAX));

    // HAP-Param-Value.
    err = HAPPDUReadAndSerializeCharacteristicValue(server, session, characteristic, service, accessory, &bodyWriter);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                err == kHAPError_Busy);
        HAPLogCharacteristicError(&logObject, characteristic, service, accessory, "Failed to handle read: %u.", err);
        return err;
    }

    // Finalize body.
    void* bodyBytes;
    size_t numBodyBytes;
    HAPTLVWriterGetBuffer(&bodyWriter, &bodyBytes, &numBodyBytes);
    HAPAssert(bodyBytes == &bytes[numHeaderBytes]);
    HAPAssert(numBodyBytes <= maxBytes - numHeaderBytes - CHACHA20_POLY1305_TAG_BYTES);

    // Body Length.
    HAPAssert(numBodyBytes <= UINT16_MAX);
    HAPWriteLittleUInt16(&bytes[3], (uint16_t) numBodyBytes);

    *numBytes = numHeaderBytes + numBodyBytes;
    HAPAssert(*numBytes <= maxBytes - CHACHA20_POLY1305_TAG_BYTES);

    // Encrypt.
    err = HAPSessionEncryptEventMessage(server, session, bytes, bytes, *numBytes);
    if (err) {
        HAPAssert(err == kHAPError_InvalidState);
        HAPLogCharacteristicError(
                &logObject, characteristic, service, accessory, "Failed to encrypt event notification: %u.", err);
        return err;
    }
    *numBytes += CHACHA20_POLY1305_TAG_BYTES;
    HAPAssert(*numBytes <= maxBytes);

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static HAPError NotificationDataSource(
        void* _Nullable context,
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPThreadSessionStorage_DataBuffer dataBufferType,
        void* bytes,
        size_t maxBytes,
        size_t* numBytes) {
    HAPPrecondition(!context);
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(dataBufferType == kHAPSessionStorage_DataBuffer_Notification);
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes);

    HAPError err;

    // Get pending notification information.
    uint8_t* bitSet;
    size_t numBitSetBytes;
    HAPThreadSessionStorageGetNotificationBitSet(
            server, session, kHAPSessionStorage_NotificationBitSet_Pending, &bitSet, &numBitSetBytes);

    // Check for characteristics with pending event notifications.
    bool sentEvents;
    *numBytes = 0;
    do {
        sentEvents = false;
        size_t eventNotificationIndex = 0;
        size_t accessoryIndex = 0;
        const HAPAccessory* accessory = server->primaryAccessory;
        while (accessory) {
            if (accessory->services) {
                for (size_t i = 0; accessory->services[i]; i++) {
                    const HAPService* service = accessory->services[i];
                    if (service->characteristics) {
                        for (size_t j = 0; service->characteristics[j]; j++) {
                            const HAPBaseCharacteristic* characteristic = service->characteristics[j];
                            if (characteristic->properties.supportsEventNotification) {
                                if (HAPBitSetContains(bitSet, numBitSetBytes, eventNotificationIndex)) {
                                    HAPBitSetRemove(bitSet, numBitSetBytes, eventNotificationIndex);
                                    sentEvents = true;

                                    // Get event notification data.
                                    err = GetNotificationData(
                                            server,
                                            session,
                                            characteristic,
                                            service,
                                            accessory,
                                            bytes,
                                            maxBytes,
                                            numBytes);
                                    if (err) {
                                        if (err == kHAPError_OutOfResources) {
                                            HAPLogError(
                                                    &logObject, "Session storage buffer might not be large enough.");
                                        } else {
                                            HAPAssert(
                                                    err == kHAPError_Unknown || err == kHAPError_InvalidState ||
                                                    err == kHAPError_Busy);
                                        }
                                        HAPLogCharacteristicError(
                                                &logObject,
                                                characteristic,
                                                service,
                                                accessory,
                                                "Failed to get notification data: %u. Dropping.",
                                                err);
                                    } else {
                                        // Send event notification.
                                        HAPLogCharacteristicDebug(
                                                &logObject, characteristic, service, accessory, "Sending event.");
                                        err = HAPNonnull(server->transports.thread)
                                                      ->session.sendEvent(server, session, bytes, *numBytes);
                                        if (err) {
                                            if (err == kHAPError_Busy) {
                                                HAPLogCharacteristic(
                                                        &logObject,
                                                        characteristic,
                                                        service,
                                                        accessory,
                                                        "Cannot send event right now. Retrying later.");
                                                return kHAPError_None;
                                            }
                                            HAPAssert(err == kHAPError_OutOfResources);
                                            HAPLogCharacteristicError(
                                                    &logObject,
                                                    characteristic,
                                                    service,
                                                    accessory,
                                                    "Failed to send event: %d. Dropping.",
                                                    err);
                                        } else {
                                            HAPLogDebug(&logObject, "Sent event.");
                                        }
                                    }
                                    *numBytes = 0;
                                }
                                eventNotificationIndex++;
                                HAPAssert(eventNotificationIndex);
                            }
                        }
                    }
                }
            }
            accessoryIndex++;
            accessory = NULL;
        }
    } while (sentEvents);

    HAPAssert(!*numBytes);
    return kHAPError_None;
}

void HAPNotificationContinueSending(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    HAPError err;

    if (HAPAccessoryServerGetState(server) != kHAPAccessoryServerState_Running) {
        HAPLog(&logObject, "Not sending event notifications because accessory server is not running.");
        return;
    }

    size_t numSessions = HAPAccessoryServerGetNumSessions(server);
    for (size_t sessionIndex = 0; sessionIndex < numSessions; sessionIndex++) {
        HAPSession* session = HAPAccessoryServerGetSessionWithIndex(server, sessionIndex);
        if (!session->server) {
            continue;
        }

        if (HAPSessionIsSecured(session) && HAPSessionKeyExpired(session)) {
            // Purge keys if session is expired
            HAPSessionClearControlKeys(session);
        }

        if (HAPThreadAccessoryServerReleaseHapSessionIfExpired(server, session)) {
            // Thread session expired. Note that the session is released from the function above.
            continue;
        }

        // Event notifications are not delivered when the session is session is not secured,
        // or when using session security based on transient Pair Setup (Software Authentication).
        if (!HAPSessionIsSecured(session) || HAPSessionIsTransient(session)) {
            continue;
        }

        if (server->transports.thread && session->transportType == kHAPTransportType_Thread) {
            // If an event notification is already being sent, retry sending it.
            void* bytes;
            size_t numBytes;
            HAPThreadSessionStorageGetData(
                    server, session, kHAPSessionStorage_DataBuffer_Notification, &bytes, &numBytes);
            if (numBytes) {
                HAPLogDebug(&logObject, "Retrying to send event.");
                err = HAPNonnull(server->transports.thread)->session.sendEvent(server, session, bytes, numBytes);
                if (err) {
                    if (err == kHAPError_Busy) {
                        HAPLog(&logObject, "Cannot send event right now. Retrying later again.");
                        continue;
                    }
                    HAPAssert(err == kHAPError_OutOfResources);
                    HAPLogError(&logObject, "Failed to send event: %d. Dropping.", err);
                } else {
                    HAPLogDebug(&logObject, "Sent event after retrying.");
                }
                HAPThreadSessionStorageClearData(server, session, kHAPSessionStorage_DataBuffer_Notification);
            }

            // Send as many event notifications as possible, and cache the final one if it couldn't be sent for retry.
            err = HAPThreadSessionStorageSetDynamicData(
                    server,
                    session,
                    kHAPSessionStorage_DataBuffer_Notification,
                    NotificationDataSource,
                    /* context: */ NULL);
            HAPAssert(!err);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HAP_RESULT_USE_CHECK
static bool IsEventOriginator(
        HAPAccessoryServer* server,
        const HAPCharacteristic* characteristic_,
        const HAPService* service,
        const HAPAccessory* accessory,
        HAPSession* session) {
    HAPPrecondition(server);
    HAPPrecondition(characteristic_);
    const HAPBaseCharacteristic* characteristic = characteristic_;
    HAPPrecondition(service);
    HAPPrecondition(accessory);

    return session == server->writeRequest.session && accessory->aid == server->writeRequest.aid &&
           characteristic->iid == server->writeRequest.iid;
}

static void HandleRaiseEvent(
        HAPAccessoryServer* server,
        size_t eventNotificationIndex,
        HAPSession* session,
        bool* needsSend) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(needsSend);

    if (session->transportType != kHAPTransportType_Thread) {
        return;
    }

    uint8_t* statusBitSet;
    size_t numStatusBytes;
    HAPThreadSessionStorageGetNotificationBitSet(
            server, session, kHAPSessionStorage_NotificationBitSet_Status, &statusBitSet, &numStatusBytes);

    uint8_t* pendingBitSet;
    size_t numPendingBytes;
    HAPThreadSessionStorageGetNotificationBitSet(
            server, session, kHAPSessionStorage_NotificationBitSet_Pending, &pendingBitSet, &numPendingBytes);

    if (HAPBitSetContains(statusBitSet, numStatusBytes, eventNotificationIndex) &&
        !HAPBitSetContains(pendingBitSet, numPendingBytes, eventNotificationIndex)) {
        HAPBitSetInsert(pendingBitSet, numPendingBytes, eventNotificationIndex);
        *needsSend = true;
    }
}

void HAPNotificationHandleRaiseEvent(
        HAPAccessoryServer* server,
        const HAPCharacteristic* characteristic_,
        const HAPService* service,
        const HAPAccessory* accessory) {
    HAPPrecondition(server);
    HAPPrecondition(characteristic_);
    const HAPBaseCharacteristic* characteristic = characteristic_;
    HAPPrecondition(service);
    HAPPrecondition(accessory);

    if (characteristic->properties.supportsEventNotification) {
        size_t eventNotificationIndex;
        bool eventNotificationIndexFound;
        HAPAccessoryServerGetEventNotificationIndex(
                server, accessory->aid, characteristic->iid, &eventNotificationIndex, &eventNotificationIndexFound);
        HAPAssert(eventNotificationIndexFound);

        bool needsSend = false;
        size_t numSessions = HAPAccessoryServerGetNumSessions(server);
        for (size_t sessionIndex = 0; sessionIndex < numSessions; sessionIndex++) {
            HAPSession* session = HAPAccessoryServerGetSessionWithIndex(server, sessionIndex);
            if (!session->server) {
                continue;
            }

            if (!IsEventOriginator(server, characteristic_, service, accessory, session)) {
                HandleRaiseEvent(server, eventNotificationIndex, session, &needsSend);
            }
        }
        if (needsSend) {
            HAPNotificationContinueSending(server);
        }
    }
}

void HAPNotificationHandleRaiseEventOnSession(
        HAPAccessoryServer* server,
        const HAPCharacteristic* characteristic_,
        const HAPService* service,
        const HAPAccessory* accessory,
        HAPSession* session) {
    HAPPrecondition(server);
    HAPPrecondition(characteristic_);
    const HAPBaseCharacteristic* characteristic = characteristic_;
    HAPPrecondition(service);
    HAPPrecondition(accessory);

    if (characteristic->properties.supportsEventNotification) {
        size_t eventNotificationIndex;
        bool eventNotificationIndexFound;
        HAPAccessoryServerGetEventNotificationIndex(
                server, accessory->aid, characteristic->iid, &eventNotificationIndex, &eventNotificationIndexFound);
        HAPAssert(eventNotificationIndexFound);

        bool needsSend = false;
        if (!IsEventOriginator(server, characteristic_, service, accessory, session)) {
            HandleRaiseEvent(server, eventNotificationIndex, session, &needsSend);
        }
        if (needsSend) {
            HAPNotificationContinueSending(server);
        }
    }
}

#endif
