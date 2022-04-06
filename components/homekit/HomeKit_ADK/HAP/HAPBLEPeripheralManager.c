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

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)

#include "HAPAccessoryServer+Internal.h"
#include "HAPBLEAccessoryServer+Advertising.h"
#include "HAPBLECharacteristic.h"
#include "HAPBLEPeripheralManager.h"
#include "HAPBLEProcedure.h"
#include "HAPBLESession.h"
#include "HAPCharacteristic.h"
#include "HAPLog+Attributes.h"
#include "HAPLogSubsystem.h"
#include "HAPPDU+TLV.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "BLEPeripheralManager" };

// Set this flag to disable all BLE peripheral manager timeouts.
#define DEBUG_DISABLE_TIMEOUTS (false)

static void RetryPendingEventNotifications(HAPPlatformTimerRef timer, void* _Nullable context);

/**
 * Resets the state of HAP Events.
 *
 * @param      server              Accessory server.
 */
static void ResetEventState(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    HAPLogDebug(&logObject, "%s", __func__);

    for (size_t i = 0; i < server->ble.storage->numGATTTableElements; i++) {
        HAPBLEGATTTableElement* gattAttribute = &server->ble.storage->gattTableElements[i];

        if (!gattAttribute->accessory) {
            break;
        }

        gattAttribute->connectionState.centralSubscribed = false;
        gattAttribute->connectionState.pendingEvent = false;
    }
}

/**
 * Aborts all fallback HAP-BLE procedures.
 *
 * @param      server              Accessory server.
 */
static void AbortAllFallbackProcedures(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    HAPLogDebug(&logObject, "%s", __func__);

    for (size_t i = 0; i < server->ble.storage->numGATTTableElements; i++) {
        HAPBLEGATTTableElement* gattAttribute = &server->ble.storage->gattTableElements[i];

        if (!gattAttribute->accessory) {
            break;
        }

        if (gattAttribute->connectionState.fallbackProcedure.timer) {
            const HAPAccessory* accessory = gattAttribute->accessory;
            HAPAssert(gattAttribute->service);
            HAPAssert(gattAttribute->characteristic);
            const HAPBaseCharacteristic* characteristic = gattAttribute->characteristic;

            HAPLogCharacteristicInfo(&logObject, characteristic, service, accessory, "Aborting fallback procedure.");

#if !DEBUG_DISABLE_TIMEOUTS
            HAPPlatformTimerDeregister(gattAttribute->connectionState.fallbackProcedure.timer);
#endif

            HAPRawBufferZero(
                    &gattAttribute->connectionState.fallbackProcedure,
                    sizeof gattAttribute->connectionState.fallbackProcedure);
        }
    }
}

void HAPBLEPeripheralManagerRelease(HAPAccessoryServer* server) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.ble.blePeripheralManager);
    HAPPlatformBLEPeripheralManager* blePeripheralManager = server->platform.ble.blePeripheralManager;

    // Abort procedures.
    AbortAllFallbackProcedures(server);
    if (server->ble.connection.procedureAttached) {
        HAPBLEProcedureDestroy(&server->ble.storage->procedures[0]);
        server->ble.connection.procedureAttached = false;
    }

    // Abort connections.
    if (server->ble.connection.connected) {
        HAPAssert(server->ble.storage->session);
        HAPSessionRelease(server, server->ble.storage->session);
        server->ble.connection.connected = false;
    }

    // Deregister platform callbacks.
    HAPPlatformBLEPeripheralManagerRemoveAllServices(blePeripheralManager);
    HAPPlatformBLEPeripheralManagerSetDelegate(blePeripheralManager, NULL);
}

static void HandleConnectedCentral(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
        HAPPlatformBLEPeripheralManagerConnectionHandle connectionHandle,
        void* _Nullable context) {
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(context);
    HAPAccessoryServer* server = context;
    HAPPrecondition(server->ble.storage->session);
    HAPSession* session = server->ble.storage->session;

    HAPError err;

    HAPLogInfo(&logObject, "%s(0x%04x)", __func__, connectionHandle);
    HAPPrecondition(!server->ble.connection.connected);

    AbortAllFallbackProcedures(server);
    ResetEventState(server);
    server->ble.connection.connectionHandle = connectionHandle;
    server->ble.connection.connected = true;

    err = HAPBLEAccessoryServerDidConnect(server);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPFatalError();
    }

    HAPSessionCreate(server, session, kHAPTransportType_BLE);
}

static void HandleDisconnectedCentral(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
        HAPPlatformBLEPeripheralManagerConnectionHandle connectionHandle,
        void* _Nullable context) {
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(context);
    HAPAccessoryServer* server = context;
    HAPPrecondition(server->ble.storage->session);
    HAPSession* session = server->ble.storage->session;

    HAPError err;

    HAPLogInfo(&logObject, "%s(0x%04x)", __func__, connectionHandle);
    HAPPrecondition(server->ble.connection.connected);
    HAPPrecondition(connectionHandle == server->ble.connection.connectionHandle);

    server->ble.connection.connected = false;
    if (server->ble.connection.procedureAttached) {
        HAPAssert(server->ble.storage->numProcedures >= 1);
        HAPBLEProcedureDestroy(&server->ble.storage->procedures[0]);
    }
    AbortAllFallbackProcedures(server);
    HAPSessionRelease(server, session);
    ResetEventState(server);
    HAPRawBufferZero(&server->ble.connection, sizeof server->ble.connection);

    err = HAPBLEAccessoryServerDidDisconnect(server);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPFatalError();
    }
}

/**
 * Continues sending of pending HAP event notifications.
 *
 * @param      server              Accessory server.
 */
static void SendPendingEventNotifications(HAPAccessoryServer* server) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.ble.blePeripheralManager);
    HAPPlatformBLEPeripheralManagerRef blePeripheralManager = server->platform.ble.blePeripheralManager;
    HAPPrecondition(server->ble.connection.connected);
    HAPPrecondition(server->ble.storage->session);
    HAPSession* session = server->ble.storage->session;

    HAPError err;

    for (size_t i = 0; i < server->ble.storage->numGATTTableElements; i++) {
        HAPBLEGATTTableElement* gattAttribute = &server->ble.storage->gattTableElements[i];
        const HAPBaseCharacteristic* characteristic = gattAttribute->characteristic;
        const HAPService* service = gattAttribute->service;
        const HAPAccessory* accessory = gattAttribute->accessory;
        if (!accessory) {
            break;
        }
        HAPAssert(service);
        if (!characteristic) {
            continue;
        }
        if (!characteristic->properties.supportsEventNotification) {
            HAPAssert(!gattAttribute->connectionState.centralSubscribed);
            HAPAssert(!gattAttribute->connectionState.pendingEvent);
            continue;
        }
        if (characteristic->iid > UINT16_MAX) {
            HAPLogCharacteristicError(
                    &logObject,
                    characteristic,
                    service,
                    accessory,
                    "Not sending Handle Value Indication because characteristic instance ID is not supported.");
            continue;
        }
        HAPAssert(gattAttribute->valueHandle);
        HAPAssert(gattAttribute->cccDescriptorHandle);
        HAPAssert(gattAttribute->iidHandle);

        if (!gattAttribute->connectionState.centralSubscribed) {
            continue;
        }
        if (!gattAttribute->connectionState.pendingEvent) {
            continue;
        }
        if (!HAPSessionIsSecured(session)) {
            HAPLogCharacteristicInfo(
                    &logObject,
                    characteristic,
                    service,
                    accessory,
                    "Not sending Handle Value Indication because the session is not secured.");
            return;
        }
        if (HAPSessionIsTransient(session)) {
            HAPLogCharacteristicInfo(
                    &logObject,
                    characteristic,
                    service,
                    accessory,
                    "Not sending Handle Value Indication because the session is transient.");
            return;
        }
        if (HAPCharacteristicReadRequiresAdminPermissions(characteristic) && !HAPSessionControllerIsAdmin(session)) {
            HAPLogCharacteristicInfo(
                    &logObject,
                    characteristic,
                    service,
                    accessory,
                    "Not sending Handle Value Indication because event notification values will only be delivered to "
                    "controllers with admin permissions.");
            continue;
        }

        err = HAPPlatformBLEPeripheralManagerSendHandleValueIndication(
                blePeripheralManager,
                server->ble.connection.connectionHandle,
                gattAttribute->valueHandle,
                /* bytes: */ NULL,
                /* numBytes: */ 0);
        if (err) {
            if (err == kHAPError_InvalidState) {
                HAPLogCharacteristicInfo(
                        &logObject,
                        characteristic,
                        service,
                        accessory,
                        "Delayed event sending until ready to update subscribers.");
                HAPPlatformTimerRef timerId;
                err = HAPPlatformTimerRegister(
                        &timerId, HAPPlatformClockGetCurrent(), RetryPendingEventNotifications, server);
                if (err) {
                    HAPFatalError();
                }
                return;
            }

            HAPAssert(err == kHAPError_OutOfResources);
            HAPFatalError();
        }
        gattAttribute->connectionState.pendingEvent = false;
        HAPLogCharacteristicInfo(&logObject, characteristic, service, accessory, "Sent event.");

        err = HAPBLEAccessoryServerDidSendEventNotification(server, characteristic, service, accessory);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }
    }
}

static void RetryPendingEventNotifications(HAPPlatformTimerRef timer HAP_UNUSED, void* _Nullable context) {
    HAPPrecondition(context);
    HAPAccessoryServer* server = context;

    if (server->ble.connection.connected) {
        SendPendingEventNotifications(server);
    }
}

/**
 * Gets the GATT attribute structure associated with an attribute handle.
 *
 * @param      server              Accessory server.
 * @param      attributeHandle      GATT attribute handle.
 *
 * @return GATT attribute structure If found.
 * @return NULL                     Otherwise.
 */
static HAPBLEGATTTableElement* _Nullable GetGATTAttribute(
        HAPAccessoryServer* server,
        HAPPlatformBLEPeripheralManagerAttributeHandle attributeHandle) {
    HAPPrecondition(server);
    HAPPrecondition(attributeHandle);

    for (size_t i = 0; i < server->ble.storage->numGATTTableElements; i++) {
        HAPBLEGATTTableElement* gattAttribute = &server->ble.storage->gattTableElements[i];
        if (!gattAttribute->accessory) {
            break;
        }

        // Validate GATT attribute.
        HAPAssert(gattAttribute->service);
        if (!gattAttribute->characteristic) {
            HAPAssert(!gattAttribute->valueHandle);
            HAPAssert(!gattAttribute->cccDescriptorHandle);
        } else {
            const HAPBaseCharacteristic* characteristic = gattAttribute->characteristic;
            HAPAssert(gattAttribute->valueHandle);
            if (!characteristic->properties.supportsEventNotification) {
                HAPAssert(!gattAttribute->cccDescriptorHandle);
            }
        }
        HAPAssert(gattAttribute->iidHandle);

        // Check for match.
        if (attributeHandle == gattAttribute->valueHandle || attributeHandle == gattAttribute->cccDescriptorHandle ||
            attributeHandle == gattAttribute->iidHandle) {
            return gattAttribute;
        }
    }
    HAPLog(&logObject, "GATT attribute structure not found for handle 0x%04x", (unsigned int) attributeHandle);
    return NULL;
}

HAP_RESULT_USE_CHECK
static bool AreNotificationsEnabled(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPBLEGATTTableElement* gattAttribute) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(gattAttribute);
    const HAPCharacteristic* characteristic = HAPNonnullVoid(gattAttribute->characteristic);
    const HAPService* service HAP_UNUSED = HAPNonnull(gattAttribute->service);
    const HAPAccessory* accessory = HAPNonnull(gattAttribute->accessory);

    HAPLogCharacteristicInfo(
            &logObject,
            characteristic,
            service,
            accessory,
            "Events are %s.",
            gattAttribute->connectionState.centralSubscribed ? "enabled" : "disabled");
    return gattAttribute->connectionState.centralSubscribed;
}

static void SetNotificationsEnabled(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPBLEGATTTableElement* gattAttribute,
        bool enable) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(gattAttribute);
    const HAPCharacteristic* characteristic = gattAttribute->characteristic;
    const HAPService* service = gattAttribute->service;
    const HAPAccessory* accessory = gattAttribute->accessory;

    HAPLogCharacteristicInfo(
            &logObject, characteristic, service, accessory, "%s events.", enable ? "Enabling" : "Disabling");
    if (gattAttribute->connectionState.centralSubscribed == enable) {
        return;
    }
    gattAttribute->connectionState.centralSubscribed = enable;

    // Inform application.
    if (HAPSessionIsSecured(session)) {
        if (enable) {
            HAPAccessoryServerHandleSubscribe(server, session, characteristic, service, accessory);
        } else {
            HAPAccessoryServerHandleUnsubscribe(server, session, characteristic, service, accessory);
        }
    } else {
        HAPLogCharacteristic(
                &logObject,
                characteristic,
                service,
                accessory,
                "Session is not secured. Delaying to inform application about %s of events.",
                enable ? "enabling" : "disabling");
    }

    // Subscription state changed. Continue sending events.
    SendPendingEventNotifications(server);
}

#if !DEBUG_DISABLE_TIMEOUTS
static void FallbackProcedureTimerExpired(HAPPlatformTimerRef timer, void* _Nullable context) {
    HAPPrecondition(context);
    HAPAccessoryServer* server = context;

    HAPLogDebug(&logObject, "%s", __func__);

    // 39. Accessories must implement a 10 second HAP procedure timeout, all HAP procedures [...] must complete within
    // 10 seconds, if a procedure fails to complete within the procedure timeout the accessory must drop the security
    // session and also drop the Bluetooth link.
    // See HomeKit Accessory Protocol Specification R17
    // Section 7.5 Testing Bluetooth LE Accessories

    for (size_t i = 0; i < server->ble.storage->numGATTTableElements; i++) {
        HAPBLEGATTTableElement* gattAttribute = &server->ble.storage->gattTableElements[i];

        if (!gattAttribute->accessory) {
            break;
        }
        if (gattAttribute->connectionState.fallbackProcedure.timer != timer) {
            continue;
        }

        const HAPAccessory* accessory = gattAttribute->accessory;
        HAPAssert(gattAttribute->service);
        HAPAssert(gattAttribute->characteristic);
        const HAPBaseCharacteristic* characteristic = gattAttribute->characteristic;

        HAPLogCharacteristicInfo(&logObject, characteristic, service, accessory, "Fallback procedure expired.");

#if !DEBUG_DISABLE_TIMEOUTS // NOLINT(readability-redundant-preprocessor)
        HAPPlatformTimerDeregister(gattAttribute->connectionState.fallbackProcedure.timer);
#endif
        HAPRawBufferZero(
                &gattAttribute->connectionState.fallbackProcedure,
                sizeof gattAttribute->connectionState.fallbackProcedure);
    }

    HAPAssert(server->ble.connection.connected);
    HAPAssert(server->ble.storage->session);
    HAPSession* session = server->ble.storage->session;
    HAPSessionInvalidate(server, session, /* terminateLink: */ true);
}
#endif

/**
 * HAP-BLE procedure type.
 */
HAP_ENUM_BEGIN(uint8_t, HAPBLEProcedureType) { /**
                                                * Full-featured procedure.
                                                *
                                                * - Associated type: HAPBLEProcedure
                                                */
                                               kHAPBLEProcedureType_Full = 1,

                                               /**
                                                * Fallback procedure.
                                                *
                                                * - Associated type: HAPBLEFallbackProcedure
                                                */
                                               kHAPBLEProcedureType_Fallback
} HAP_ENUM_END(uint8_t, HAPBLEProcedureType);

/**
 * Checks that a value matches the claimed HAPBLEProcedureType.
 *
 * - Argument numbers start at 1.
 *
 * @param      valueArg             Argument number of the value.
 * @param      typeArg              Argument number of the value type.
 */
#if __has_attribute(pointer_with_type_tag) && __has_attribute(type_tag_for_datatype)
/**@cond */
__attribute__((type_tag_for_datatype(HAPBLEProcedureType, HAPBLEProcedure*))) static const HAPBLEProcedureType
        _kHAPBLEProcedureType_Full HAP_UNUSED = kHAPBLEProcedureType_Full;

__attribute__((type_tag_for_datatype(HAPBLEProcedureType, HAPBLEFallbackProcedure*))) static const HAPBLEProcedureType
        _kHAPBLEProcedureType_Fallback HAP_UNUSED = kHAPBLEProcedureType_Fallback;
/**@endcond */

#define HAP_PWT_HAPBLEProcedureType(valueArg, typeArg) \
    __attribute__((pointer_with_type_tag(HAPBLEProcedureType, valueArg, typeArg)))
#else
#define HAP_PWT_HAPBLEProcedureType(valueArg, typeArg)
#endif

/**
 * Gets the HAP-BLE procedure for a GATT attribute.
 *
 * @param      server              Accessory server.
 * @param      session             The session over which the request has been received.
 * @param      gattAttribute        The GATT attribute that is accessed.
 * @param[out] procedureType        Type of the attached procedure.
 * @param[out] procedure            HAP-BLE procedure.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If no procedure can be fetched at this time.
 */
HAP_PWT_HAPBLEProcedureType(5, 4) HAP_RESULT_USE_CHECK static HAPError GetProcedure(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPBLEGATTTableElement* gattAttribute,
        HAPBLEProcedureType* procedureType,
        void* _Nonnull* _Nonnull procedure) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.ble.blePeripheralManager);
    HAPPlatformBLEPeripheralManagerRef blePeripheralManager = server->platform.ble.blePeripheralManager;
    HAPPrecondition(session);
    HAPPrecondition(gattAttribute);
    HAPPrecondition(gattAttribute->characteristic);
    const HAPBaseCharacteristic* characteristic = gattAttribute->characteristic;
    HAPPrecondition(gattAttribute->service);
    HAPPrecondition(gattAttribute->accessory);
    const HAPAccessory* accessory = gattAttribute->accessory;
    HAPPrecondition(procedureType);
    HAPPrecondition(procedure);

    // For now, we only support 1 concurrent full-featured procedure.
    HAPPrecondition(server->ble.storage->procedures);
    HAPPrecondition(server->ble.storage->procedures);
    HAPPrecondition(server->ble.storage->numProcedures >= 1);
    HAPBLEProcedure* fullProcedure = &server->ble.storage->procedures[0];

    // Every characteristic supports a fallback procedure.
    HAPBLEFallbackProcedure* fallbackProcedure = &gattAttribute->connectionState.fallbackProcedure;

    // If session is terminal, no more requests may be accepted.
    if (HAPBLESessionIsTerminal(&session->_.ble)) {
        HAPLogCharacteristic(&logObject, characteristic, service, accessory, "Rejecting request: Session is terminal.");
        HAPPlatformBLEPeripheralManagerCancelCentralConnection(
                blePeripheralManager, server->ble.connection.connectionHandle);
        return kHAPError_InvalidState;
    }

    // An accessory must cancel any pending procedures when a new HAP secure session starts getting established.
    // See HomeKit Accessory Protocol Specification R17
    // Section 7.3.1 HAP Transactions and Procedures
    if (HAPBLECharacteristicDropsSecuritySession(characteristic)) {
        HAPLogCharacteristicDebug(
                &logObject,
                characteristic,
                service,
                accessory,
                "Aborting fallback procedure (%s).",
                "Characteristic drops security session");
        AbortAllFallbackProcedures(server);
    }

    // Check if already attached to the same characteristic (fallback procedure).
    if (gattAttribute->connectionState.fallbackProcedure.timer) {
        *procedureType = kHAPBLEProcedureType_Fallback;
        *procedure = fallbackProcedure;
        return kHAPError_None;
    }

    // Check if already attached to the same characteristic (full procedure).
    if (server->ble.connection.procedureAttached) {
        const HAPBaseCharacteristic* attachedCharacteristic = HAPBLEProcedureGetAttachedCharacteristic(fullProcedure);
        HAPAssert(attachedCharacteristic);

        if (attachedCharacteristic == characteristic) {
            *procedureType = kHAPBLEProcedureType_Full;
            *procedure = fullProcedure;
            return kHAPError_None;
        }
    }

    // Unsolicited read request.
    // 12. Accessory must reject GATT Read Requests on a HAP characteristic if it was not preceded by an
    // GATT Write Request with the same transaction ID at most 10 seconds prior.
    // See HomeKit Accessory Protocol Specification R17
    // Section 7.5 Testing Bluetooth LE Accessories
    return kHAPError_InvalidState;
}

HAP_RESULT_USE_CHECK
static HAPError HandleReadRequest(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
        HAPPlatformBLEPeripheralManagerConnectionHandle connectionHandle,
        HAPPlatformBLEPeripheralManagerAttributeHandle attributeHandle,
        void* bytes,
        size_t maxBytes,
        size_t* numBytes,
        void* _Nullable context) {
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(attributeHandle);
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes);
    HAPPrecondition(context);
    HAPAccessoryServer* server = context;
    HAPPrecondition(server->ble.storage->session);
    HAPSession* session = server->ble.storage->session;

    HAPError err;

    HAPLogDebug(&logObject, "%s(0x%04x, 0x%04x)", __func__, connectionHandle, attributeHandle);
    HAPPrecondition(server->ble.connection.connected);
    HAPPrecondition(connectionHandle == server->ble.connection.connectionHandle);
    HAPBLEGATTTableElement* _Nullable gattAttribute = GetGATTAttribute(server, attributeHandle);
    HAPPrecondition(gattAttribute);
    const HAPBaseCharacteristic* _Nullable characteristic = gattAttribute->characteristic;
    const HAPService* _Nullable service = gattAttribute->service;
    const HAPAccessory* _Nullable accessory = gattAttribute->accessory;

    if (attributeHandle == gattAttribute->valueHandle) {
        HAPAssert(characteristic);
        HAPAssert(service);
        HAPAssert(accessory);
        HAPLogCharacteristicDebug(&logObject, characteristic, service, accessory, "GATT Read value.");

        // Get HAP-BLE procedure.
        HAPBLEProcedureType procedureType;
        void* procedure;
        err = GetProcedure(server, session, gattAttribute, &procedureType, &procedure);
        if (err) {
            HAPAssert(err == kHAPError_InvalidState);
            HAPSessionInvalidate(server, session, /* terminateLink: */ true);
            return err;
        }

        // Process request.
        switch (procedureType) {
            case kHAPBLEProcedureType_Full: {
                HAPBLEProcedure* fullProcedure = procedure;

                // Process request.
                err = HAPBLEProcedureHandleGATTRead(fullProcedure, bytes, maxBytes, numBytes);
                if (err) {
                    HAPAssert(err == kHAPError_InvalidState || err == kHAPError_OutOfResources);
                    HAPSessionInvalidate(server, session, /* terminateLink: */ true);
                    return err;
                }
                break;
            }
            case kHAPBLEProcedureType_Fallback: {
                HAPBLEFallbackProcedure* fallbackProcedure = procedure;

                HAPLogCharacteristicInfo(
                        &logObject, characteristic, service, accessory, "Processing response of fallback procedure.");

                if (fallbackProcedure->remainingBodyBytes) {
                    HAPLogCharacteristic(
                            &logObject,
                            characteristic,
                            service,
                            accessory,
                            "Response of fallback procedure expected before request was fully sent.");
                    HAPSessionInvalidate(server, session, /* terminateLink: */ true);
                    return kHAPError_InvalidState;
                }

                // Compute response length.
                *numBytes = 3;
                switch (fallbackProcedure->status) {
                    case kHAPBLEFallbackProcedureStatus_MaxProcedures:
                    case kHAPBLEFallbackProcedureStatus_InvalidInstanceID: {
                        *numBytes += 0;
                        break;
                    }
                    case kHAPBLEFallbackProcedureStatus_ZeroInstanceIDServiceSignatureRead: {
                        *numBytes += 2; // Body length.
                        *numBytes += 2;
                        break;
                    }
                }

                // When Pair Verify is accessed, all fallback procedures are canceled.
                // Therefore, we do not need to remember whether or not the procedure has been secured at start.
                bool isSecured = HAPSessionIsSecured(session);
                if (isSecured) {
                    *numBytes += CHACHA20_POLY1305_TAG_BYTES;
                }
                if (maxBytes < *numBytes) {
                    HAPLogCharacteristic(
                            &logObject,
                            characteristic,
                            service,
                            accessory,
                            "Response of fallback procedure on too long for available space.");
                    HAPSessionInvalidate(server, session, /* terminateLink: */ true);
                    return kHAPError_OutOfResources;
                }

                // Serialize response.
                uint8_t* data = bytes;
                data[0] = (0U << 7U) | (0U << 3U) | (0U << 2U) | (1U << 1U) | (0U << 0U);
                data[1] = fallbackProcedure->transactionID;
                switch (fallbackProcedure->status) {
                    case kHAPBLEFallbackProcedureStatus_MaxProcedures: {
                        HAPLogCharacteristic(
                                &logObject, characteristic, service, accessory, "Sending Max-Procedures error.");
                        data[2] = kHAPPDUStatus_MaxProcedures;
                        break;
                    }
                    case kHAPBLEFallbackProcedureStatus_InvalidInstanceID: {
                        HAPLogCharacteristic(
                                &logObject, characteristic, service, accessory, "Sending Invalid Instance ID error.");
                        data[2] = kHAPPDUStatus_InvalidInstanceID;
                        break;
                    }
                    case kHAPBLEFallbackProcedureStatus_ZeroInstanceIDServiceSignatureRead: {
                        HAPLogCharacteristic(
                                &logObject,
                                characteristic,
                                service,
                                accessory,
                                "Sending default service signature response (iid 0).");
                        data[2] = kHAPPDUStatus_Success;
                        HAPWriteLittleUInt16(&data[3], 2U);
                        data[5] = kHAPPDUTLVType_HAPLinkedServices;
                        data[6] = 0;
                        break;
                    }
                }

                // Encrypt response if necessary.
                if (isSecured) {
                    err = HAPSessionEncryptControlMessage(
                            server, session, bytes, bytes, *numBytes - CHACHA20_POLY1305_TAG_BYTES);
                    if (err) {
                        HAPLogCharacteristic(
                                &logObject,
                                characteristic,
                                service,
                                accessory,
                                "Response of fallback procedure could not be encrypted.");
                        HAPAssert(err == kHAPError_InvalidState);
                        HAPSessionInvalidate(server, session, /* terminateLink: */ true);
                        return err;
                    }
                }

                // Reset procedure.
                HAPAssert(fallbackProcedure->timer);
#if !DEBUG_DISABLE_TIMEOUTS
                HAPPlatformTimerDeregister(fallbackProcedure->timer);
#endif
                HAPRawBufferZero(fallbackProcedure, sizeof *fallbackProcedure);

                // Report response being sent.
                HAPBLESessionDidSendGATTResponse(server, session);
                break;
            }
            default:
                HAPFatalError();
        }

        // Remove queued broadcast event corresponding to the read characteristic if any
        HAPBLEAccessoryServerRemoveQueuedBroadcastEvent(
                server, (HAPCharacteristic*) characteristic, service, accessory);

        // Continue sending events (if security state changed).
        SendPendingEventNotifications(server);
    } else if (attributeHandle == gattAttribute->cccDescriptorHandle) {
        HAPAssert(characteristic);
        HAPAssert(service);
        HAPAssert(accessory);
        HAPLogCharacteristicDebug(
                &logObject,
                characteristic,
                service,
                accessory,
                "GATT Read Client Characteristic Configuration descriptor value.");

        // This descriptor value must support always being read in the clear, i.e., with or without a security session.
        // See HomeKit Accessory Protocol Specification R17
        // Section 7.4.4.5.3 Client Characteristic Configuration

        // Process request.
        if (maxBytes < 2) {
            HAPLogCharacteristic(
                    &logObject,
                    characteristic,
                    service,
                    accessory,
                    "Not enough space available to write Client Characteristic Configuration descriptor value.");
            return kHAPError_OutOfResources;
        }
        bool isEnabled = AreNotificationsEnabled(server, session, gattAttribute);
        HAPWriteLittleUInt16(bytes, isEnabled ? 0x0002U : 0x0000U);
        *numBytes = sizeof(uint16_t);
    } else {
        HAPAssert(attributeHandle == gattAttribute->iidHandle);
        HAPAssert(service);
        HAPAssert(accessory);
        if (characteristic) {
            HAPLogCharacteristicDebug(
                    &logObject,
                    characteristic,
                    service,
                    accessory,
                    "GATT Read Characteristic Instance ID descriptor value.");

            // Process request.
            if (maxBytes < 2) {
                HAPLogCharacteristic(
                        &logObject,
                        characteristic,
                        service,
                        accessory,
                        "Not enough space available to write Characteristic Instance ID descriptor value.");
                return kHAPError_OutOfResources;
            }
            HAPAssert(characteristic->iid <= UINT16_MAX);
            HAPWriteLittleUInt16(bytes, characteristic->iid);
            *numBytes = sizeof(uint16_t);
        } else {
            HAPLogServiceDebug(&logObject, service, accessory, "GATT Read Service Instance ID descriptor value.");

            // Process request.
            if (maxBytes < 2) {
                HAPLogService(
                        &logObject,
                        service,
                        accessory,
                        "Not enough space available to write Service Instance ID descriptor value.");
                return kHAPError_OutOfResources;
            }
            HAPAssert(service->iid <= UINT16_MAX);
            HAPWriteLittleUInt16(bytes, service->iid);
            *numBytes = sizeof(uint16_t);
        }
    }
    return kHAPError_None;
}

/**
 * Attaches a HAP-BLE procedure.
 *
 * @param      server              Accessory server.
 * @param      session             The session over which the request has been received.
 * @param      gattAttribute        The GATT attribute that is accessed.
 * @param[out] procedureType        Type of the attached procedure.
 * @param[out] procedure            HAP-BLE procedure.
 * @param[out] isNewProcedure       Whether a new or existing procedure was attached. true = new.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If no procedure can be fetched at this time.
 * @return kHAPError_OutOfResources If no procedure is available.
 */
HAP_PWT_HAPBLEProcedureType(5, 4) HAP_RESULT_USE_CHECK static HAPError AttachProcedure(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPBLEGATTTableElement* gattAttribute,
        HAPBLEProcedureType* procedureType,
        void* _Nonnull* _Nonnull procedure,
        bool* isNewProcedure) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.ble.blePeripheralManager);
    HAPPlatformBLEPeripheralManagerRef blePeripheralManager = server->platform.ble.blePeripheralManager;
    HAPPrecondition(session);
    HAPPrecondition(gattAttribute);
    HAPPrecondition(gattAttribute->characteristic);
    const HAPBaseCharacteristic* characteristic = gattAttribute->characteristic;
    HAPPrecondition(gattAttribute->service);
    const HAPService* service = gattAttribute->service;
    HAPPrecondition(gattAttribute->accessory);
    const HAPAccessory* accessory = gattAttribute->accessory;
    HAPPrecondition(procedureType);
    HAPPrecondition(procedure);
    HAPPrecondition(isNewProcedure);

#if !DEBUG_DISABLE_TIMEOUTS
    HAPError err;
#endif

    // For now, we only support 1 concurrent full-featured procedure.
    HAPPrecondition(server->ble.storage->procedures);
    HAPPrecondition(server->ble.storage->procedures);
    HAPPrecondition(server->ble.storage->numProcedures >= 1);
    HAPBLEProcedure* fullProcedure = &server->ble.storage->procedures[0];

    // Every characteristic supports a fallback procedure.
    HAPBLEFallbackProcedure* fallbackProcedure = &gattAttribute->connectionState.fallbackProcedure;

    // If session is terminal, no more requests may be accepted.
    if (HAPBLESessionIsTerminal(&session->_.ble)) {
        HAPLogCharacteristic(&logObject, characteristic, service, accessory, "Rejecting request: Session is terminal.");
        HAPPlatformBLEPeripheralManagerCancelCentralConnection(
                blePeripheralManager, server->ble.connection.connectionHandle);
        return kHAPError_InvalidState;
    }

    // Handle shut down.
    if (server->state != kHAPAccessoryServerState_Running) {
        if (server->ble.connection.procedureAttached && HAPBLEProcedureIsInProgress(fullProcedure)) {
            // Allow finishing procedure to avoid dealing with bugs from halfway completed procedures.
            // Fallback procedures do not modify any state, so it's okay to abort them while they are ongoing.
            // Procedures have a timeout so this cannot delay forever.
            HAPLogCharacteristicInfo(
                    &logObject,
                    characteristic,
                    service,
                    accessory,
                    "Shutdown has been requested. Allowing current HAP-BLE procedure to finish.");
        } else {
            // Do not start new procedures and abort pending fallback procedures.
            HAPLogCharacteristic(
                    &logObject, characteristic, service, accessory, "Rejecting request: Shutdown requested.");
            HAPPlatformBLEPeripheralManagerCancelCentralConnection(
                    blePeripheralManager, server->ble.connection.connectionHandle);
            return kHAPError_InvalidState;
        }
    }

    // An accessory must cancel any pending procedures when a new HAP secure session starts getting established.
    // See HomeKit Accessory Protocol Specification R17
    // Section 7.3.1 HAP Transactions and Procedures
    if (HAPBLECharacteristicDropsSecuritySession(characteristic)) {
        HAPLogCharacteristicDebug(
                &logObject,
                characteristic,
                service,
                accessory,
                "Aborting fallback procedure (%s).",
                "Characteristic drops security session");
        AbortAllFallbackProcedures(server);
    }

    // Check if already attached to the same characteristic (fallback procedure).
    if (gattAttribute->connectionState.fallbackProcedure.timer) {
        *procedureType = kHAPBLEProcedureType_Fallback;
        *procedure = fallbackProcedure;
        *isNewProcedure = false;
        return kHAPError_None;
    }

    // Detach full-featured procedure from previous characteristic if necessary.
    if (server->ble.connection.procedureAttached) {
        const HAPBaseCharacteristic* attachedCharacteristic = HAPBLEProcedureGetAttachedCharacteristic(fullProcedure);
        HAPAssert(attachedCharacteristic);

        // Check if already attached to the same characteristic.
        if (attachedCharacteristic == characteristic) {
            *procedureType = kHAPBLEProcedureType_Full;
            *procedure = fullProcedure;
            *isNewProcedure = false;
            return kHAPError_None;
        }

        // Check if previous procedure is detachable.
        if (HAPBLEProcedureIsInProgress(fullProcedure)) {
            // An accessory must cancel any pending procedures when a new HAP secure session starts getting established.
            // See HomeKit Accessory Protocol Specification R17
            // Section 7.3.1 HAP Transactions and Procedures
            if (HAPBLECharacteristicDropsSecuritySession(characteristic)) {
                HAPLogCharacteristicInfo(
                        &logObject,
                        characteristic,
                        service,
                        accessory,
                        "Aborting existing procedure on [%016llX %s] (%s).",
                        (unsigned long long) attachedCharacteristic->iid,
                        attachedCharacteristic->debugDescription,
                        "Characteristic drops security session");

                AbortAllFallbackProcedures(server);
            } else {
                HAPLogCharacteristic(
                        &logObject,
                        characteristic,
                        service,
                        accessory,
                        "HAP-BLE procedure on [%016llX %s] is in progress. Attaching fallback procedure.",
                        (unsigned long long) attachedCharacteristic->iid,
                        attachedCharacteristic->debugDescription);

#if !DEBUG_DISABLE_TIMEOUTS
                err = HAPPlatformTimerRegister(
                        &fallbackProcedure->timer,
                        HAPPlatformClockGetCurrent() + 10 * HAPSecond,
                        FallbackProcedureTimerExpired,
                        server);
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    HAPLogCharacteristicError(
                            &logObject,
                            characteristic,
                            service,
                            accessory,
                            "Not enough resources to start timer. Disconnecting immediately!");
                    return err;
                }
#else
                fallbackProcedure->timer = 1;
#endif
                HAPBLESessionDidStartBLEProcedure(server, session);

                *procedureType = kHAPBLEProcedureType_Fallback;
                *procedure = fallbackProcedure;
                *isNewProcedure = true;
                return kHAPError_None;
            }
        }

        // Detach from previous procedure.
        HAPLogCharacteristicDebug(
                &logObject,
                characteristic,
                service,
                accessory,
                "Detaching procedure from [%016llX %s] to start procedure.",
                (unsigned long long) attachedCharacteristic->iid,
                attachedCharacteristic->debugDescription);
        HAPBLEProcedureDestroy(fullProcedure);
        server->ble.connection.procedureAttached = false;
    }

    // Attach to new characteristic.
    HAPLogCharacteristicDebug(&logObject, characteristic, service, accessory, "Attaching procedure.");
    HAPBLEProcedureAttach(
            fullProcedure,
            server->ble.storage->procedureBuffer.bytes,
            server->ble.storage->procedureBuffer.numBytes,
            server,
            session,
            characteristic,
            service,
            accessory);
    server->ble.connection.procedureAttached = true;

    *procedureType = kHAPBLEProcedureType_Full;
    *procedure = fullProcedure;
    *isNewProcedure = true;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static HAPError HandleWriteRequest(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
        HAPPlatformBLEPeripheralManagerConnectionHandle connectionHandle,
        HAPPlatformBLEPeripheralManagerAttributeHandle attributeHandle,
        void* bytes,
        size_t numBytes,
        void* _Nullable context) {
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(attributeHandle);
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes);
    HAPPrecondition(context);
    HAPAccessoryServer* server = context;
    HAPPrecondition(server->ble.storage->session);
    HAPSession* session = server->ble.storage->session;

    HAPError err;

    HAPLogDebug(&logObject, "%s(0x%04x, 0x%04x)", __func__, connectionHandle, attributeHandle);
    HAPPrecondition(server->ble.connection.connected);
    HAPPrecondition(connectionHandle == server->ble.connection.connectionHandle);
    HAPBLEGATTTableElement* _Nullable gattAttribute = GetGATTAttribute(server, attributeHandle);
    HAPPrecondition(gattAttribute);
    const HAPBaseCharacteristic* _Nullable characteristic = gattAttribute->characteristic;
    const HAPService* _Nullable service = gattAttribute->service;
    const HAPAccessory* _Nullable accessory = gattAttribute->accessory;

    if (attributeHandle == gattAttribute->valueHandle) {
        HAPAssert(characteristic);
        HAPAssert(service);
        HAPAssert(accessory);
        HAPLogCharacteristicDebug(&logObject, characteristic, service, accessory, "GATT Write value.");

        // Get HAP-BLE procedure.
        HAPBLEProcedureType procedureType;
        void* procedure;
        bool isNewProcedure;
        err = AttachProcedure(server, session, gattAttribute, &procedureType, &procedure, &isNewProcedure);
        if (err) {
            HAPAssert(err == kHAPError_InvalidState || err == kHAPError_OutOfResources);
            HAPSessionInvalidate(server, session, /* terminateLink: */ true);
            return err;
        }

        // Process request.
        switch (procedureType) {
            case kHAPBLEProcedureType_Full: {
                HAPBLEProcedure* fullProcedure = procedure;

                // Process request.
                err = HAPBLEProcedureHandleGATTWrite(fullProcedure, bytes, numBytes);
                if (err) {
                    HAPAssert(
                            err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                            err == kHAPError_OutOfResources);
                    HAPSessionInvalidate(server, session, /* terminateLink: */ true);
                    return err;
                }
                break;
            }
            case kHAPBLEProcedureType_Fallback: {
                HAPBLEFallbackProcedure* fallbackProcedure = procedure;

                // When Pair Verify is accessed, all fallback procedures are canceled.
                // Therefore, we do not need to remember whether or not the procedure has been secured at start.
                if (HAPSessionIsSecured(session)) {
                    if (numBytes < CHACHA20_POLY1305_TAG_BYTES) {
                        HAPLogCharacteristic(
                                &logObject,
                                characteristic,
                                service,
                                accessory,
                                "Write to fallback procedure malformed (too short for auth tag).");
                        HAPSessionInvalidate(server, session, /* terminateLink: */ true);
                        return kHAPError_InvalidData;
                    }
                    err = HAPSessionDecryptControlMessage(server, session, bytes, bytes, numBytes);
                    if (err) {
                        HAPLogCharacteristic(
                                &logObject,
                                characteristic,
                                service,
                                accessory,
                                "First fragment of fallback procedure malformed (decryption failed).");
                        HAPAssert(err == kHAPError_InvalidState || err == kHAPError_InvalidData);
                        HAPSessionInvalidate(server, session, /* terminateLink: */ true);
                        return err;
                    }
                    numBytes -= CHACHA20_POLY1305_TAG_BYTES;
                }

                if (isNewProcedure) {
                    HAPLogCharacteristicInfo(
                            &logObject,
                            characteristic,
                            service,
                            accessory,
                            "Processing first fragment of fallback procedure.");

                    uint8_t* data = bytes;
                    if (numBytes < 5) {
                        HAPLogCharacteristic(
                                &logObject,
                                characteristic,
                                service,
                                accessory,
                                "First fragment of fallback procedure malformed (too short).");
                        HAPSessionInvalidate(server, session, /* terminateLink: */ true);
                        return kHAPError_InvalidData;
                    }
                    if (data[0] != ((0U << 7U) | (0U << 3U) | (0U << 2U) | (0U << 1U) | (0U << 0U))) {
                        HAPLogCharacteristic(
                                &logObject,
                                characteristic,
                                service,
                                accessory,
                                "First fragment of fallback procedure malformed (control field).");
                        HAPSessionInvalidate(server, session, /* terminateLink: */ true);
                        return kHAPError_InvalidData;
                    }

                    // Store minimal information to be able to throw error.
                    fallbackProcedure->transactionID = data[2];
                    fallbackProcedure->status = kHAPBLEFallbackProcedureStatus_MaxProcedures;

                    // Handle simple errors.
                    uint8_t operation = data[1];
                    uint16_t iid = HAPReadLittleUInt16(&data[3]);
                    if (HAPPDUOpcodeIsValid(operation)) {
                        uint16_t expectedIID;
                        if (HAPPDUOpcodeGetOperationType((HAPPDUOpcode) operation) == kHAPPDUOperationType_Service) {
                            HAPAssert(service->iid <= UINT16_MAX);
                            expectedIID = (uint16_t) service->iid;
                        } else {
                            HAPAssert(characteristic->iid <= UINT16_MAX);
                            expectedIID = (uint16_t) characteristic->iid;
                        }

                        if (iid != expectedIID) {
                            HAPLogCharacteristic(
                                    &logObject,
                                    characteristic,
                                    service,
                                    accessory,
                                    "Invalid IID %u in fallback procedure.",
                                    iid);

                            fallbackProcedure->status = kHAPBLEFallbackProcedureStatus_InvalidInstanceID;

                            // If the accessory receives an invalid (e.g., 0) Service instance ID in the
                            // HAP-Service-Signature-Read-Request, it must respond with a valid
                            // HAP-Service-Signature-Read-Response with Svc Properties set to 0 and Linked Svc
                            // (if applicable) set to 0 length.
                            // See HomeKit Accessory Protocol Specification R17
                            // Section 7.3.4.13 HAP-Service-Signature-Read-Response
                            if (operation == kHAPPDUOpcode_ServiceSignatureRead && !iid) {
                                fallbackProcedure->status =
                                        kHAPBLEFallbackProcedureStatus_ZeroInstanceIDServiceSignatureRead;
                            }
                        }
                    }

                    // Skip body.
                    if (numBytes > 5) {
                        if (numBytes < 7) {
                            HAPLogCharacteristic(
                                    &logObject,
                                    characteristic,
                                    service,
                                    accessory,
                                    "First fragment of fallback procedure on malformed (body length).");
                            HAPSessionInvalidate(server, session, /* terminateLink: */ true);
                            return kHAPError_InvalidData;
                        }

                        fallbackProcedure->remainingBodyBytes = HAPReadLittleUInt16(&data[5]);

                        // Skip body.
                        if (fallbackProcedure->remainingBodyBytes < numBytes - 7) {
                            HAPLogCharacteristic(
                                    &logObject,
                                    characteristic,
                                    service,
                                    accessory,
                                    "First fragment of fallback procedure on malformed (body too long).");
                            HAPSessionInvalidate(server, session, /* terminateLink: */ true);
                            return kHAPError_InvalidData;
                        }
                        fallbackProcedure->remainingBodyBytes -= (uint16_t)(numBytes - 7);
                    } else {
                        fallbackProcedure->remainingBodyBytes = 0;
                    }
                } else {
                    HAPLogCharacteristicInfo(
                            &logObject,
                            characteristic,
                            service,
                            accessory,
                            "Processing continuation of fallback procedure.");

                    uint8_t* data = bytes;
                    if (numBytes < 2) {
                        HAPLogCharacteristic(
                                &logObject,
                                characteristic,
                                service,
                                accessory,
                                "Continuation of fallback procedure malformed (too short).");
                        HAPSessionInvalidate(server, session, /* terminateLink: */ true);
                        return kHAPError_InvalidData;
                    }
                    if (data[0] != ((1U << 7U) | (0U << 3U) | (0U << 2U) | (0U << 1U) | (0U << 0U))) {
                        HAPLogCharacteristic(
                                &logObject,
                                characteristic,
                                service,
                                accessory,
                                "Continuation of fallback procedure malformed (control field).");
                        HAPSessionInvalidate(server, session, /* terminateLink: */ true);
                        return kHAPError_InvalidData;
                    }
                    if (data[1] != fallbackProcedure->transactionID) {
                        HAPLogCharacteristic(
                                &logObject,
                                characteristic,
                                service,
                                accessory,
                                "Continuation of fallback procedure malformed (invalid TID).");
                        HAPSessionInvalidate(server, session, /* terminateLink: */ true);
                        return kHAPError_InvalidData;
                    }

                    // Skip body.
                    if (fallbackProcedure->remainingBodyBytes < numBytes - 2) {
                        HAPLogCharacteristic(
                                &logObject,
                                characteristic,
                                service,
                                accessory,
                                "Continuation of fallback procedure malformed (body too long).");
                        HAPSessionInvalidate(server, session, /* terminateLink: */ true);
                        return kHAPError_InvalidData;
                    }
                    fallbackProcedure->remainingBodyBytes -= (uint16_t)(numBytes - 2);
                }

                // Report response being sent.
                HAPBLESessionDidSendGATTResponse(server, session);
                break;
            }
            default:
                HAPFatalError();
        }

        // Continue sending events (if security state changed).
        SendPendingEventNotifications(server);
    } else if (attributeHandle == gattAttribute->cccDescriptorHandle) {
        HAPAssert(characteristic);
        HAPAssert(service);
        HAPAssert(accessory);
        HAPLogCharacteristicDebug(
                &logObject,
                characteristic,
                service,
                accessory,
                "GATT Write Client Characteristic Configuration descriptor value.");

        // Process request.
        if (numBytes != 2) {
            HAPLogCharacteristic(
                    &logObject,
                    characteristic,
                    service,
                    accessory,
                    "Unexpected Client Characteristic Configuration descriptor length: %lu.",
                    (unsigned long) numBytes);
            return kHAPError_InvalidData;
        }
        uint16_t v = HAPReadLittleUInt16(bytes);
        if (v & (uint16_t) ~0x0002U) {
            HAPLogCharacteristic(
                    &logObject,
                    characteristic,
                    service,
                    accessory,
                    "Unexpected Client Characteristic Configuration descriptor value: 0x%04x.",
                    (unsigned int) v);
            return kHAPError_InvalidData;
        }
        bool eventsEnabled = (v & 0x0002U) != 0;
        SetNotificationsEnabled(server, session, gattAttribute, eventsEnabled);
    } else {
        HAPAssert(attributeHandle == gattAttribute->iidHandle);
        HAPAssert(service);
        HAPAssert(accessory);
        if (characteristic) {
            HAPLogCharacteristicDebug(
                    &logObject,
                    characteristic,
                    service,
                    accessory,
                    "GATT Write Characteristic Instance ID descriptor value.");

            // Process request.
            HAPLogCharacteristic(
                    &logObject,
                    characteristic,
                    service,
                    accessory,
                    "Rejecting write to Characteristic Instance ID descriptor value.");
        } else {
            HAPLogServiceDebug(&logObject, service, accessory, "GATT Write Service Instance ID descriptor value.");

            // Process request.
            HAPLogService(&logObject, service, accessory, "Rejecting write to Service Instance ID descriptor value.");
        }
        return kHAPError_InvalidState;
    }
    return kHAPError_None;
}

static void HandleReadyToUpdateSubscribers(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
        HAPPlatformBLEPeripheralManagerConnectionHandle connectionHandle,
        void* _Nullable context) {
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(context);
    HAPAccessoryServer* server = context;
    HAPPrecondition(server->ble.connection.connected);

    HAPLogDebug(&logObject, "%s(0x%04x)", __func__, connectionHandle);
    HAPPrecondition(server->ble.connection.connected);
    HAPPrecondition(connectionHandle == server->ble.connection.connectionHandle);

    SendPendingEventNotifications(server);
}

void HAPBLEPeripheralManagerRegister(HAPAccessoryServer* server) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.ble.blePeripheralManager);
    HAPPlatformBLEPeripheralManagerRef blePeripheralManager = server->platform.ble.blePeripheralManager;
    HAPPrecondition(server->primaryAccessory);
    const HAPAccessory* accessory = server->primaryAccessory;

    HAPError err;

    // Set delegate.
    HAPPlatformBLEPeripheralManagerSetDelegate(
            blePeripheralManager,
            &(const HAPPlatformBLEPeripheralManagerDelegate) { .context = server,
                                                               .handleConnectedCentral = HandleConnectedCentral,
                                                               .handleDisconnectedCentral = HandleDisconnectedCentral,
                                                               .handleReadRequest = HandleReadRequest,
                                                               .handleWriteRequest = HandleWriteRequest,
                                                               .handleReadyToUpdateSubscribers =
                                                                       HandleReadyToUpdateSubscribers });

    if (!HAPPlatformBLEPeripheralManagerAllowsServiceRefresh(blePeripheralManager)) {
        // If service refreshing is not allowed on this platform in the current state,
        // publish the service and quit.
        // Note that server->ble.storage->gattTableElements must be retaining the previously added
        // set of services and characteristics.
        HAPLogDebug(&logObject, "%s: BLE services not re-registered", __func__);
        HAPPlatformBLEPeripheralManagerPublishServices(blePeripheralManager);
        return;
    }

    // Reset table.
    HAPRawBufferZero(
            server->ble.storage->gattTableElements,
            server->ble.storage->numGATTTableElements * sizeof *server->ble.storage->gattTableElements);
    HAPPlatformBLEPeripheralManagerRemoveAllServices(blePeripheralManager);

    // Register DB.
    size_t o = 0;
    if (accessory->services) {
        for (size_t i = 0; accessory->services[i]; i++) {
            const HAPService* service = accessory->services[i];

            if (!HAPAccessoryServerSupportsService(server, kHAPTransportType_BLE, service)) {
                continue;
            }

            // Map GATT attribute for service.
            if (o >= server->ble.storage->numGATTTableElements) {
                HAPLogServiceError(
                        &logObject, service, accessory, "GATT table capacity not large enough to store service.");
                HAPFatalError();
            }
            HAPBLEGATTTableElement* gattAttribute = &server->ble.storage->gattTableElements[o];
            gattAttribute->accessory = accessory;
            gattAttribute->service = service;

            // Register service.
            HAPAssert(sizeof *service->serviceType == sizeof(HAPPlatformBLEPeripheralManagerUUID));
            err = HAPPlatformBLEPeripheralManagerAddService(
                    blePeripheralManager,
                    (const HAPPlatformBLEPeripheralManagerUUID*) service->serviceType,
                    /* isPrimary: */ true);
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                HAPFatalError();
            }

            // Register Service Instance ID characteristic.
            // This characteristic contains a static value and does not use HAP-BLE procedures.
            static const HAPPlatformBLEPeripheralManagerUUID kBLECharacteristicUUID_ServiceInstanceID = {
                { 0xD1, 0xA0, 0x83, 0x50, 0x00, 0xAA, 0xD3, 0x87, 0x17, 0x48, 0x59, 0xA7, 0x5D, 0xE9, 0x04, 0xE6 }
            };
            uint8_t iid[2];
            HAPWriteLittleUInt16(iid, service->iid);
            err = HAPPlatformBLEPeripheralManagerAddCharacteristic(
                    blePeripheralManager,
                    &kBLECharacteristicUUID_ServiceInstanceID,
                    (HAPPlatformBLEPeripheralManagerCharacteristicProperties) { .read = true,
                                                                                .writeWithoutResponse = false,
                                                                                .write = false,
                                                                                .notify = false,
                                                                                .indicate = false },
                    iid,
                    sizeof iid,
                    &gattAttribute->iidHandle,
                    /* cccDescriptorHandle: */ NULL);
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                HAPFatalError();
            }

            // Finalize GATT attribute.
            HAPLogServiceInfo(&logObject, service, accessory, "(service)");
            o++;

            // Register characteristics.
            if (service->characteristics) {
                for (size_t j = 0; service->characteristics[j]; j++) {
                    const HAPBaseCharacteristic* characteristic = service->characteristics[j];

                    // Map GATT attribute for characteristic.
                    if (o >= server->ble.storage->numGATTTableElements) {
                        HAPLogCharacteristicError(
                                &logObject,
                                characteristic,
                                service,
                                accessory,
                                "GATT table capacity not large enough to store characteristic.");
                        HAPFatalError();
                    }
                    gattAttribute = &server->ble.storage->gattTableElements[o];
                    gattAttribute->accessory = accessory;
                    gattAttribute->service = service;
                    gattAttribute->characteristic = characteristic;

                    // Register characteristic.
                    HAPAssert(
                            sizeof *characteristic->characteristicType == sizeof(HAPPlatformBLEPeripheralManagerUUID));
                    err = HAPPlatformBLEPeripheralManagerAddCharacteristic(
                            blePeripheralManager,
                            (const HAPPlatformBLEPeripheralManagerUUID*) characteristic->characteristicType,
                            (HAPPlatformBLEPeripheralManagerCharacteristicProperties) {
                                    .read = true,
                                    .writeWithoutResponse = false,
                                    .write = true,
                                    .notify = false,
                                    .indicate = characteristic->properties.supportsEventNotification },
                            NULL,
                            0,
                            &gattAttribute->valueHandle,
                            characteristic->properties.supportsEventNotification ? &gattAttribute->cccDescriptorHandle :
                                                                                   NULL);
                    if (err) {
                        HAPAssert(err == kHAPError_OutOfResources);
                        HAPFatalError();
                    }

                    // Register Characteristic Instance ID descriptor.
                    // This descriptor contains a static value and does not use HAP-BLE procedures.
                    static const HAPPlatformBLEPeripheralManagerUUID kBLEDescriptorUUID_CharacteristicInstanceID = {
                        { 0x9A,
                          0x93,
                          0x96,
                          0xD7,
                          0xBD,
                          0x6A,
                          0xD9,
                          0xB5,
                          0x16,
                          0x46,
                          0xD2,
                          0x81,
                          0xFE,
                          0xF0,
                          0x46,
                          0xDC }
                    };
                    HAPWriteLittleUInt16(iid, characteristic->iid);
                    err = HAPPlatformBLEPeripheralManagerAddDescriptor(
                            blePeripheralManager,
                            &kBLEDescriptorUUID_CharacteristicInstanceID,
                            (HAPPlatformBLEPeripheralManagerDescriptorProperties) { .read = true, .write = false },
                            iid,
                            sizeof iid,
                            &gattAttribute->iidHandle);
                    if (err) {
                        HAPAssert(err == kHAPError_OutOfResources);
                        HAPFatalError();
                    }

                    // Finalize GATT attribute.
                    HAPLogCharacteristicInfo(
                            &logObject,
                            characteristic,
                            service,
                            accessory,
                            "val %04x / iid %04x",
                            gattAttribute->valueHandle,
                            gattAttribute->iidHandle);
                    o++;
                }
            }
        }
    }

    // Finalize GATT database.
    HAPPlatformBLEPeripheralManagerPublishServices(blePeripheralManager);
}

void HAPBLEPeripheralManagerRaiseEvent(
        HAPAccessoryServer* server,
        const HAPCharacteristic* characteristic_,
        const HAPService* service,
        const HAPAccessory* accessory) {
    HAPPrecondition(server);
    HAPPrecondition(characteristic_);
    const HAPBaseCharacteristic* characteristic = characteristic_;
    HAPPrecondition(service);
    HAPPrecondition(accessory);

    for (size_t i = 0; i < server->ble.storage->numGATTTableElements; i++) {
        HAPBLEGATTTableElement* gattAttribute = &server->ble.storage->gattTableElements[i];
        if (!gattAttribute->accessory) {
            break;
        }

        if (gattAttribute->characteristic == characteristic && gattAttribute->service == service &&
            gattAttribute->accessory == accessory) {
            HAPLogCharacteristicInfo(&logObject, characteristic, service, accessory, "Scheduling event.");
            gattAttribute->connectionState.pendingEvent = true;
            SendPendingEventNotifications(server);
            return;
        }
    }
    HAPLogCharacteristic(&logObject, characteristic, service, accessory, "GATT attribute structure not found.");
}

void HAPBLEPeripheralManagerHandleSessionAccept(HAPAccessoryServer* server, HAPSession* session) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(HAPSessionIsSecured(session));
    if (!server->transports.ble) {
        return;
    }
    if (session != server->ble.storage->session) {
        return;
    }

    // On BLE event subscriptions may be enabled before the HomeKit session is secured.
    // If this happens we have delayed informing the application about the updated subscription state
    // and need to inform it now that it has been informed.
    for (size_t i = 0; i < server->ble.storage->numGATTTableElements; i++) {
        HAPBLEGATTTableElement* gattAttribute = &server->ble.storage->gattTableElements[i];
        if (!gattAttribute->accessory) {
            break;
        }

        if (!gattAttribute->characteristic) {
            continue;
        }
        const HAPBaseCharacteristic* characteristic = HAPNonnullVoid(gattAttribute->characteristic);
        const HAPService* service = HAPNonnull(gattAttribute->service);
        const HAPAccessory* accessory = HAPNonnull(gattAttribute->accessory);
        if (!characteristic->properties.supportsEventNotification) {
            continue;
        }

        // Inform application.
        if (gattAttribute->connectionState.centralSubscribed) {
            HAPLogCharacteristic(
                    &logObject,
                    characteristic,
                    service,
                    accessory,
                    "Informing application about enabling of events that were enabled before session was accepted.");
            HAPAccessoryServerHandleSubscribe(server, session, characteristic, service, accessory);
        }
    }

    // Continue sending events.
    SendPendingEventNotifications(server);
}

void HAPBLEPeripheralManagerHandleSessionInvalidate(HAPAccessoryServer* server, HAPSession* session) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    if (!server->transports.ble) {
        return;
    }
    if (session != server->ble.storage->session) {
        return;
    }

    // Inform application that controller has unsubscribed from all characteristics.
    // Note that on BLE the actual subscription state persists across sequential sessions until there is a disconnect.
    for (size_t i = 0; i < server->ble.storage->numGATTTableElements; i++) {
        HAPBLEGATTTableElement* gattAttribute = &server->ble.storage->gattTableElements[i];
        if (!gattAttribute->accessory) {
            break;
        }

        if (!gattAttribute->characteristic) {
            continue;
        }
        const HAPBaseCharacteristic* characteristic = HAPNonnullVoid(gattAttribute->characteristic);
        const HAPService* service = HAPNonnull(gattAttribute->service);
        const HAPAccessory* accessory = HAPNonnull(gattAttribute->accessory);
        if (!characteristic->properties.supportsEventNotification) {
            continue;
        }

        // Inform application.
        if (gattAttribute->connectionState.centralSubscribed) {
            HAPLogCharacteristicDebug(
                    &logObject, characteristic, service, accessory, "Informing application about disabling of events.");
            HAPAccessoryServerHandleUnsubscribe(server, session, characteristic, service, accessory);
        }
    }
}
#endif
