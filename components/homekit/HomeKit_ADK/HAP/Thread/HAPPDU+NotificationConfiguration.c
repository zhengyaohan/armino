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

#include "HAPAccessoryServer+Internal.h"
#include "HAPBitSet.h"
#include "HAPCharacteristic.h"
#include "HAPLog+Attributes.h"
#include "HAPLogSubsystem.h"
#include "HAPNotification+Delivery.h"
#include "HAPPDU+TLV.h"
#include "HAPSession.h"
#include "HAPThreadSessionStorage.h"

// IP and BLE not yet implemented.

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "Notification" };

// HAP Opcode.
// - 0x0A / HAP-Notification-Configuration-Read
// - 0x0B / HAP-Notification-Register
// - 0x0C / HAP-Notification-Deregister
//
// See HomeKit Accessory Protocol Specification R17
// Section 7.3.3.2 HAP Request Format

// Additional Parameter Types.
// - 0x17 / HAP-Param-Notifications-Enabled
//
// See HomeKit Accessory Protocol Specification R17
// Section 7.3.3.4 HAP PDU Body

// HAP-Notification-Configuration-Read-Request.
//
// +-------------------------------------------------------+
// |                      PDU Header                       |
// +===============+==============+==========+=============+
// | Control Field | HAP PDU Type |   TID    | Instance ID |
// |   (1 Byte)    |   (1 Byte)   | (1 Byte) |  (2 Bytes)  |
// +---------------+--------------+----------+-------------+
// | 0b 0000 0000  |     0x0A     |   0xXX   |   0xXXXX    |
// +---------------+--------------+----------+-------------+

// HAP-Notification-Configuration-Read-Response.
//
// +-------------------------------------+---------------------------------------+
// |             PDU Header              |               PDU Body                |
// +===============+==========+==========+=============+=========================+
// | Control Field |   TID    |  Status  | Body Length |           TLV           |
// |   (1 Byte)    | (1 Byte) | (1 Byte) |  (2 Bytes)  | (Notifications-Enabled) |
// +---------------+----------+----------+-------------+-------------------------+
// | 0b 0000 0010  |   0xXX   |   0xXX   |    0xXXXX   |   <0x17, 0x01, 0xXX>    |
// +---------------+----------+----------+-------------+-------------------------+
//
// The value of the Notifications-Enabled field is set to 0x01 when notifications for the characteristic
// are currently enabled for the controller who originated the request, and to 0x00 otherwise.

// HAP Notification Configuration Read Procedure.
// This procedure is used to read the current notification configuration for a characteristic.
// The procedure must not be supported when the session is not secured, or when using session security
// based on transient Pair Setup (Software Authentication).
//
// 1. HAP-PDU: HAP-Notification-Configuration-Read-Request
// 2. HAP-PDU: HAP-Notification-Configuration-Read-Response

// HAP-Notification-Register-Request.
//
// +-------------------------------------------------------+
// |                      PDU Header                       |
// +===============+==============+==========+=============+
// | Control Field | HAP PDU Type |   TID    | Instance ID |
// |   (1 Byte)    |   (1 Byte)   | (1 Byte) |  (2 Bytes)  |
// +---------------+--------------+----------+-------------+
// | 0b 0000 0000  |     0x0B     |   0xXX   |   0xXXXX    |
// +---------------+--------------+----------+-------------+

// HAP-Notification-Register-Response.
//
// +-------------------------------------+
// |             PDU Header              |
// +===============+==========+==========+
// | Control Field |   TID    |  Status  |
// |   (1 Byte)    | (1 Byte) | (1 Byte) |
// +---------------+----------+----------+
// | 0b 0000 0010  |   0xXX   |   0xXX   |
// +---------------+----------+----------+

// HAP Notification Register Procedure.
// This procedure is used to register for event notifications on a characteristic. When an update occurs
// on the characteristic, the accessory sends an event notification to the registered controllers.
// When the security session is terminated, registrations are cleared. The controller will have to re-register
// for event notifications after re-establishing session security. The procedure must not be supported when the
// session is not secured, or when using session security based on transient Pair Setup (Software Authentication).
//
// 1. HAP-PDU: HAP-Notification-Register-Request
// 2. HAP-PDU: HAP-Notification-Register-Response
// 3. HAP-PDU: HAP-Notification-Event (encrypted using EventAccessoryToControllerKey and separate nonce)

// HAP-Notification-Deregister-Request.
//
// +-------------------------------------------------------+
// |                      PDU Header                       |
// +===============+==============+==========+=============+
// | Control Field | HAP PDU Type |   TID    | Instance ID |
// |   (1 Byte)    |   (1 Byte)   | (1 Byte) |  (2 Bytes)  |
// +---------------+--------------+----------+-------------+
// | 0b 0000 0000  |     0x0C     |   0xXX   |   0xXXXX    |
// +---------------+--------------+----------+-------------+

// HAP-Notification-Deregister-Response.
//
// +-------------------------------------+
// |             PDU Header              |
// +===============+==========+==========+
// | Control Field |   TID    |  Status  |
// |   (1 Byte)    | (1 Byte) | (1 Byte) |
// +---------------+----------+----------+
// | 0b 0000 0010  |   0xXX   |   0xXX   |
// +---------------+----------+----------+

// HAP Notification Deregister Procedure.
// This procedure is used to deregister for event notifications on a characteristic. The controller no longer receives
// event notifications when an update occurs on the characteristic. The procedure must not be supported when the
// session is not secured, or when using session security based on transient Pair Setup (Software Authentication).
//
// 1. HAP-PDU: HAP-Notification-Deregister-Request
// 2. HAP-PDU: HAP-Notification-Deregister-Response

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HAP_RESULT_USE_CHECK
static bool AreNotificationsEnabled(
        HAPAccessoryServer* server,
        HAPSession* session,
        const HAPCharacteristic* characteristic_,
        const HAPService* service,
        const HAPAccessory* accessory) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    (void) session;
    HAPPrecondition(characteristic_);
    const HAPBaseCharacteristic* characteristic = characteristic_;
    (void) characteristic;
    HAPPrecondition(service);
    HAPPrecondition(accessory);

    switch (session->transportType) {
        case kHAPTransportType_IP: {
            HAPFatalError();
        }
        case kHAPTransportType_BLE: {
            HAPFatalError();
        }
        case kHAPTransportType_Thread: {
            size_t eventNotificationIndex;
            bool eventNotificationIndexFound;
            HAPAccessoryServerGetEventNotificationIndex(
                    server, accessory->aid, characteristic->iid, &eventNotificationIndex, &eventNotificationIndexFound);
            HAPAssert(eventNotificationIndexFound);

            uint8_t* bitSet;
            size_t numBytes;
            HAPThreadSessionStorageGetNotificationBitSet(
                    server, session, kHAPSessionStorage_NotificationBitSet_Status, &bitSet, &numBytes);

            return HAPBitSetContains(bitSet, numBytes, eventNotificationIndex);
        }
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPNotificationGetConfigurationReadResponse(
        HAPAccessoryServer* server,
        HAPSession* session,
        const HAPCharacteristic* characteristic_,
        const HAPService* service,
        const HAPAccessory* accessory,
        HAPTLVWriter* responseWriter) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(characteristic_);
    const HAPBaseCharacteristic* characteristic = characteristic_;
    HAPPrecondition(service);
    HAPPrecondition(accessory);
    HAPPrecondition(responseWriter);

    HAPError err;

    if (!characteristic->properties.supportsEventNotification) {
        HAPLogCharacteristic(
                &logObject, characteristic_, service, accessory, "Characteristic does not support notifications.");
        return kHAPError_InvalidState;
    }

    bool enabled = AreNotificationsEnabled(server, session, characteristic_, service, accessory);
    uint8_t enabledBytes[] = { enabled ? 0x01 : 0x00 };
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPDUTLVType_NotificationsEnabled,
                              .value = { .bytes = enabledBytes, .numBytes = sizeof enabledBytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    return kHAPError_None;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void SetNotificationsEnabled(
        HAPAccessoryServer* server,
        HAPSession* session,
        const HAPCharacteristic* characteristic_,
        const HAPService* service,
        const HAPAccessory* accessory,
        bool enable) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(characteristic_);
    const HAPBaseCharacteristic* characteristic = characteristic_;
    (void) characteristic;
    HAPPrecondition(service);
    HAPPrecondition(accessory);
    (void) enable;

    switch (session->transportType) {
        case kHAPTransportType_IP: {
            HAPFatalError();
        }
        case kHAPTransportType_BLE: {
            HAPFatalError();
        }
        case kHAPTransportType_Thread: {
            size_t eventNotificationIndex;
            bool eventNotificationIndexFound;
            HAPAccessoryServerGetEventNotificationIndex(
                    server, accessory->aid, characteristic->iid, &eventNotificationIndex, &eventNotificationIndexFound);
            HAPAssert(eventNotificationIndexFound);

            uint8_t* bitSet;
            size_t numBytes;
            HAPThreadSessionStorageGetNotificationBitSet(
                    server, session, kHAPSessionStorage_NotificationBitSet_Status, &bitSet, &numBytes);

            if (enable) {
                HAPBitSetInsert(bitSet, numBytes, eventNotificationIndex);
            } else {
                HAPBitSetRemove(bitSet, numBytes, eventNotificationIndex);
            }
            return;
        }
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPNotificationHandleRegisterRequest(
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

    if (!characteristic->properties.supportsEventNotification) {
        HAPLogCharacteristic(
                &logObject, characteristic_, service, accessory, "Characteristic does not support notifications.");
        return kHAPError_InvalidState;
    }

    if (AreNotificationsEnabled(server, session, characteristic_, service, accessory)) {
        return kHAPError_None;
    }

    SetNotificationsEnabled(server, session, characteristic, service, accessory, /* enable: */ true);

    if (HAPSessionIsSecured(session)) {
        HAPAccessoryServerHandleSubscribe(server, session, characteristic_, service, accessory);
        HAPNotificationContinueSending(server);
    } else {
        HAPLogCharacteristic(
                &logObject,
                characteristic,
                service,
                accessory,
                "Session is not secured. Delaying to inform application about %s of events.",
                "enabling");
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPNotificationHandleDeregisterRequest(
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

    if (!characteristic->properties.supportsEventNotification) {
        HAPLogCharacteristic(
                &logObject, characteristic_, service, accessory, "Characteristic does not support notifications.");
        return kHAPError_InvalidState;
    }

    if (!AreNotificationsEnabled(server, session, characteristic_, service, accessory)) {
        return kHAPError_None;
    }

    SetNotificationsEnabled(server, session, characteristic_, service, accessory, /* enable: */ false);

    if (HAPSessionIsSecured(session)) {
        HAPAccessoryServerHandleUnsubscribe(server, session, characteristic_, service, accessory);
        HAPNotificationContinueSending(server);
    } else {
        HAPLogCharacteristic(
                &logObject,
                characteristic,
                service,
                accessory,
                "Session is not secured. Delaying to inform application about %s of events.",
                "disabling");
    }

    return kHAPError_None;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void HAPNotificationDeregisterAll(HAPAccessoryServer* server, HAPSession* session) {
    HAPPrecondition(server);
    HAPPrecondition(session);

    bool isSecured = HAPSessionIsSecured(session);

    uint8_t* statusBitSet;
    size_t numStatusBytes;
    HAPThreadSessionStorageGetNotificationBitSet(
            server, session, kHAPSessionStorage_NotificationBitSet_Status, &statusBitSet, &numStatusBytes);

    uint8_t* pendingBitSet;
    size_t numPendingBytes;
    HAPThreadSessionStorageGetNotificationBitSet(
            server, session, kHAPSessionStorage_NotificationBitSet_Pending, &pendingBitSet, &numPendingBytes);

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
                            if (HAPBitSetContains(statusBitSet, numStatusBytes, eventNotificationIndex)) {
                                HAPBitSetRemove(pendingBitSet, numPendingBytes, eventNotificationIndex);
                                HAPBitSetRemove(statusBitSet, numStatusBytes, eventNotificationIndex);

                                if (isSecured) {
                                    HAPAccessoryServerHandleUnsubscribe(
                                            server, session, characteristic, service, accessory);
                                } else {
                                    HAPLogCharacteristic(
                                            &logObject,
                                            characteristic,
                                            service,
                                            accessory,
                                            "Session was not secured. Not informing application about %s of events.",
                                            "disabling");
                                }
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
}

#endif
