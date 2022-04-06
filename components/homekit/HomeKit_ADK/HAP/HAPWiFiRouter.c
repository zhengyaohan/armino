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
// Copyright (C) 2015-2020 Apple Inc. All Rights Reserved.

#include "HAP+API.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_ROUTER)

#include "HAPWiFiRouter.h"

#include "HAPAccessoryServer+Internal.h"
#include "HAPBitSet.h"
#include "HAPCharacteristic.h"
#include "HAPLog+Attributes.h"
#include "HAPLogSubsystem.h"
#include "HAPServiceTypes.h"
#include "HAPSession.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "WiFiRouter" };

//----------------------------------------------------------------------------------------------------------------------

void HAPWiFiRouterSubscribeForEvents(HAPWiFiRouterEventState* eventState, HAPWiFiRouterSessionState* sessionState) {
    HAPPrecondition(eventState);
    HAPPrecondition(sessionState);

    HAPAssert(HAPRawBufferIsZero(sessionState, sizeof *sessionState));
    if (eventState->timestamp) {
        sessionState->timestamp = eventState->timestamp;
    } else {
        HAPAssert(sizeof sessionState->timestamp == sizeof(uint8_t));
        sessionState->timestamp = UINT8_MAX;
    }
}

void HAPWiFiRouterUnsubscribeFromEvents(HAPWiFiRouterEventState* eventState, HAPWiFiRouterSessionState* sessionState) {
    HAPPrecondition(eventState);
    HAPPrecondition(sessionState);

    HAPAssert(sessionState->timestamp);
    HAPRawBufferZero(sessionState, sizeof *sessionState);
}

typedef struct {
    const HAPCharacteristic* characteristic;
} PrepareNextEventEnumerateSessionsContext;

static void PrepareNextEventEnumerateSessionsCallback(
        void* _Nullable context_,
        HAPAccessoryServer* server,
        HAPSession* session,
        bool* shouldContinue) // NOLINT(readability-non-const-parameter)
{
    HAPPrecondition(context_);
    PrepareNextEventEnumerateSessionsContext* context = context_;
    HAPPrecondition(context->characteristic);
    const HAPBaseCharacteristic* characteristic = context->characteristic;
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(shouldContinue);
    HAPPrecondition(*shouldContinue);

    HAPWiFiRouterEventState* eventState;
    HAPWiFiRouterSessionState* sessionState;
    if (HAPUUIDAreEqual(characteristic->characteristicType, &kHAPCharacteristicType_NetworkClientProfileControl)) {
        eventState = &server->ip.wiFiRouterEventState.networkClientProfileControl;
        sessionState = &session->wiFiRouterEventState.networkClientProfileControl;
    } else {
        HAPAssert(HAPUUIDAreEqual(
                characteristic->characteristicType, &kHAPCharacteristicType_NetworkAccessViolationControl));
        eventState = &server->ip.wiFiRouterEventState.networkAccessViolationControl;
        sessionState = &session->wiFiRouterEventState.networkAccessViolationControl;
    }

    if (!sessionState->timestamp) {
        // Not subscribed.
        return;
    }

    // Clear isOriginator state.
    HAPBitSetRemove(sessionState->isOriginator, sizeof sessionState->isOriginator, eventState->nextIndex);

    // Advance timestamp if we are in conflict with the upcoming timestamp.
    // This prevents reporting "all events sent" (empty list in event TLV)
    // when in fact we should report "too many pending changes" (empty event TLV without list).
    uint8_t nextTimestamp = eventState->timestamp + 1;
    if (!nextTimestamp) {
        nextTimestamp++;
    }
    if (sessionState->timestamp == nextTimestamp) {
        sessionState->timestamp++;
        if (!sessionState->timestamp) {
            sessionState->timestamp++;
        }
    }
}

void HAPWiFiRouterRaiseClientEvent(
        HAPAccessoryServer* server,
        const HAPCharacteristic* characteristic_,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPSession* _Nullable session) {
    HAPPrecondition(server);
    HAPPrecondition(characteristic_);
    const HAPBaseCharacteristic* characteristic = characteristic_;
    HAPPrecondition(clientIdentifier);

    HAPLogInfo(
            &logObject,
            "[%016llX %s] Raising event for network client profile identifier %lu.",
            (unsigned long long) characteristic->iid,
            characteristic->debugDescription,
            (unsigned long) clientIdentifier);

    HAPWiFiRouterEventState* eventState;
    HAPWiFiRouterSessionState* sessionState = NULL;
    if (HAPUUIDAreEqual(characteristic->characteristicType, &kHAPCharacteristicType_NetworkClientProfileControl)) {
        eventState = &server->ip.wiFiRouterEventState.networkClientProfileControl;
        if (session) {
            sessionState = &session->wiFiRouterEventState.networkClientProfileControl;
        }
    } else {
        HAPAssert(HAPUUIDAreEqual(
                characteristic->characteristicType, &kHAPCharacteristicType_NetworkAccessViolationControl));
        eventState = &server->ip.wiFiRouterEventState.networkAccessViolationControl;
        if (session) {
            sessionState = &session->wiFiRouterEventState.networkAccessViolationControl;
        }
    }

    // Clear out next event slot.
    HAPAssert(eventState->nextIndex < HAPArrayCount(eventState->identifiers));
    if (eventState->identifiers[eventState->nextIndex]) {
        eventState->identifiers[eventState->nextIndex] = 0;

        PrepareNextEventEnumerateSessionsContext enumerateContext;
        HAPRawBufferZero(&enumerateContext, sizeof enumerateContext);
        enumerateContext.characteristic = characteristic_;
        HAPAccessoryServerEnumerateConnectedSessions(
                server, PrepareNextEventEnumerateSessionsCallback, &enumerateContext);
    }

    // Store new event.
    eventState->identifiers[eventState->nextIndex] = clientIdentifier;
    if (sessionState && sessionState->timestamp) {
        HAPBitSetInsert(sessionState->isOriginator, sizeof sessionState->isOriginator, eventState->nextIndex);
    }

    // Advance event slots and timestamp.
    eventState->nextIndex++;
    if (eventState->nextIndex == HAPArrayCount(eventState->identifiers)) {
        eventState->nextIndex = 0;
    }
    eventState->timestamp++;
    if (!eventState->timestamp) {
        eventState->timestamp++;
    }
}

void HAPWiFiRouterHandleClientEventSent(HAPAccessoryServer* server, const HAPTLV8CharacteristicReadRequest* request) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->session);
    HAPSession* session = HAPNonnull(request->session);

    HAPWiFiRouterEventState* eventState;
    HAPWiFiRouterSessionState* sessionState;
    if (HAPUUIDAreEqual(
                request->characteristic->characteristicType, &kHAPCharacteristicType_NetworkClientProfileControl)) {
        eventState = &server->ip.wiFiRouterEventState.networkClientProfileControl;
        sessionState = &session->wiFiRouterEventState.networkClientProfileControl;
    } else {
        HAPAssert(HAPUUIDAreEqual(
                request->characteristic->characteristicType, &kHAPCharacteristicType_NetworkAccessViolationControl));
        eventState = &server->ip.wiFiRouterEventState.networkAccessViolationControl;
        sessionState = &session->wiFiRouterEventState.networkAccessViolationControl;
    }

    if (sessionState->timestamp && eventState->timestamp) {
        sessionState->timestamp = eventState->timestamp;
    }
}

typedef struct {
    HAPAccessoryServer* server;
    const HAPTLV8CharacteristicReadRequest* request;
} SequenceDataSource;
HAP_STATIC_ASSERT(sizeof(SequenceDataSource) <= sizeof(HAPSequenceTLVDataSource), SequenceDataSource);

HAP_RESULT_USE_CHECK
static HAPError EnumerateClientEvents(
        HAPSequenceTLVDataSource* dataSource_,
        HAPCharacteristicValueCallback_WiFiRouter_ClientList callback,
        void* _Nullable context) {
    HAPPrecondition(dataSource_);
    SequenceDataSource* dataSource = (SequenceDataSource*) dataSource_;
    HAPPrecondition(dataSource->server);
    HAPAccessoryServer* server = dataSource->server;
    HAPPrecondition(server->platform.ip.wiFiRouter);
    HAPPlatformWiFiRouterRef wiFiRouter HAP_UNUSED = server->platform.ip.wiFiRouter;
    HAPPrecondition(dataSource->request);
    const HAPTLV8CharacteristicReadRequest* request = dataSource->request;
    HAPPrecondition(request->session);
    HAPSession* session = HAPNonnull(request->session);
    HAPPrecondition(callback);

    HAPWiFiRouterEventState* eventState;
    HAPWiFiRouterSessionState* sessionState;
    if (HAPUUIDAreEqual(
                request->characteristic->characteristicType, &kHAPCharacteristicType_NetworkClientProfileControl)) {
        eventState = &server->ip.wiFiRouterEventState.networkClientProfileControl;
        sessionState = &session->wiFiRouterEventState.networkClientProfileControl;
    } else {
        HAPAssert(HAPUUIDAreEqual(
                request->characteristic->characteristicType, &kHAPCharacteristicType_NetworkAccessViolationControl));
        eventState = &server->ip.wiFiRouterEventState.networkAccessViolationControl;
        sessionState = &session->wiFiRouterEventState.networkAccessViolationControl;
    }

    HAPPrecondition(sessionState->timestamp);

    HAPAssert(sizeof eventState->timestamp == sizeof(uint8_t));
    HAPAssert(sizeof sessionState->timestamp == sizeof(uint8_t));
    uint8_t diff = 0;
    if (eventState->timestamp) {
        if (eventState->timestamp >= sessionState->timestamp) {
            diff = eventState->timestamp - sessionState->timestamp;
        } else {
            diff = eventState->timestamp + UINT8_MAX - sessionState->timestamp;
        }
    }
    if (!diff) {
        return kHAPError_None;
    }

    // If there were more changes than the memory allows, this function would not be called
    // as the whole list TLV would be omitted to indicate that resynchronization is required.
    HAPAssert(diff <= HAPArrayCount(eventState->identifiers));

    uint8_t firstIndex;
    if (eventState->nextIndex >= diff) {
        firstIndex = eventState->nextIndex - diff;
    } else {
        firstIndex = eventState->nextIndex + HAPArrayCount(eventState->identifiers) - diff;
    }

    bool shouldContinue = true;
    for (uint8_t i = 0, identifierIndex = firstIndex; shouldContinue && i < diff; i++) {
        HAPAssert(identifierIndex < HAPArrayCount(eventState->identifiers));
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier = eventState->identifiers[identifierIndex];

        // HAP event should not be sent back to the controller that originated it with a write request.
        if (!HAPBitSetContains(sessionState->isOriginator, sizeof sessionState->isOriginator, identifierIndex)) {
            // When there were multiple changes, only report them once.
            bool isDuplicate = false;
            for (uint8_t j = 0, otherIndex = firstIndex; j < i; j++) {
                HAPAssert(otherIndex < HAPArrayCount(eventState->identifiers));
                HAPPlatformWiFiRouterClientIdentifier otherClientIdentifier = eventState->identifiers[otherIndex];

                if (!HAPBitSetContains(sessionState->isOriginator, sizeof sessionState->isOriginator, otherIndex)) {
                    if (clientIdentifier == otherClientIdentifier) {
                        isDuplicate = true;
                        break;
                    }
                }

                otherIndex++;
                if (otherIndex == HAPArrayCount(eventState->identifiers)) {
                    otherIndex = 0;
                }
            }

            // Report network client profile identifier.
            if (!isDuplicate) {
                HAPAssert(clientIdentifier);
                callback(context, &clientIdentifier, &shouldContinue);
            }
        }

        identifierIndex++;
        if (identifierIndex == HAPArrayCount(eventState->identifiers)) {
            identifierIndex = 0;
        }
    }

    return kHAPError_None;
}

void HAPWiFiRouterGetEventClientList(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPCharacteristicValue_WiFiRouter_ClientList* clientList,
        bool* clientListIsSet) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPSession* session = request->session;
    HAPPrecondition(clientList);
    HAPPrecondition(clientListIsSet);

    *clientListIsSet = false;

    if (!request->session) {
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Sending empty TLV: Request not associated with session.");
        return;
    }

    HAPWiFiRouterEventState* eventState;
    HAPWiFiRouterSessionState* sessionState;
    if (HAPUUIDAreEqual(
                request->characteristic->characteristicType, &kHAPCharacteristicType_NetworkClientProfileControl)) {
        eventState = &server->ip.wiFiRouterEventState.networkClientProfileControl;
        sessionState = &session->wiFiRouterEventState.networkClientProfileControl;
    } else {
        HAPAssert(HAPUUIDAreEqual(
                request->characteristic->characteristicType, &kHAPCharacteristicType_NetworkAccessViolationControl));
        eventState = &server->ip.wiFiRouterEventState.networkAccessViolationControl;
        sessionState = &session->wiFiRouterEventState.networkAccessViolationControl;
    }

    if (!sessionState->timestamp) {
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Sending empty TLV: Not subscribed for events.");
        return;
    }

    HAPAssert(sizeof eventState->timestamp == sizeof(uint8_t));
    HAPAssert(sizeof sessionState->timestamp == sizeof(uint8_t));
    uint8_t diff = 0;
    if (eventState->timestamp) {
        if (eventState->timestamp >= sessionState->timestamp) {
            diff = eventState->timestamp - sessionState->timestamp;
        } else {
            diff = eventState->timestamp + UINT8_MAX - sessionState->timestamp;
        }
    }
    if (diff > HAPArrayCount(eventState->identifiers)) {
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Sending empty TLV: Too many pending changes. Resynchronization required.");
        return;
    }

    SequenceDataSource* dataSource = (SequenceDataSource*) &clientList->dataSource;
    dataSource->server = server;
    dataSource->request = request;
    clientList->enumerate = EnumerateClientEvents;
    *clientListIsSet = true;
}

//----------------------------------------------------------------------------------------------------------------------

static void HAPWiFiRouterRaiseEvent(
        HAPAccessoryServer* server,
        const HAPUUID* characteristicType,
        const char* characteristicDebugDescription,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.ip.wiFiRouter);
    HAPPlatformWiFiRouterRef wiFiRouter = server->platform.ip.wiFiRouter;
    const HAPUUID* serviceType = &kHAPServiceType_WiFiRouter;
    const char* serviceDebugDescription = kHAPServiceDebugDescription_WiFiRouter;
    HAPPrecondition(characteristicType);
    HAPPrecondition(characteristicDebugDescription);

    HAPPrecondition(server->primaryAccessory);
    const HAPAccessory* accessory = server->primaryAccessory;
    if (accessory->services) {
        for (size_t i = 0; accessory->services[i]; i++) {
            const HAPService* service = accessory->services[i];
            if (HAPUUIDAreEqual(service->serviceType, serviceType)) {
                if (service->characteristics) {
                    for (size_t j = 0; service->characteristics[j]; j++) {
                        const HAPBaseCharacteristic* characteristic = service->characteristics[j];
                        if (HAPUUIDAreEqual(characteristic->characteristicType, characteristicType)) {
                            HAPLogCharacteristicInfo(
                                    &logObject,
                                    characteristic,
                                    service,
                                    accessory,
                                    "[%p] Raising Wi-Fi router event.",
                                    (const void*) wiFiRouter);
                            if (clientIdentifier) {
                                HAPWiFiRouterRaiseClientEvent(
                                        server, characteristic, clientIdentifier, /* session: */ NULL);
                            }
                            HAPAccessoryServerRaiseEvent(server, characteristic, service, accessory);
                            return;
                        }
                    }
                }
                HAPLogService(
                        &logObject,
                        service,
                        accessory,
                        "[%p] Wi-Fi router state changed but %s characteristic not found.",
                        (const void*) wiFiRouter,
                        characteristicDebugDescription);
                return;
            }
        }
    }
    HAPLogAccessory(
            &logObject,
            accessory,
            "[%p] Wi-Fi router state changed but %s service not found.",
            (const void*) wiFiRouter,
            serviceDebugDescription);
}

void HAPWiFiRouterHandleReadyStateChanged(HAPPlatformWiFiRouterRef wiFiRouter, void* _Nullable context) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(context);
    HAPAccessoryServer* server = context;
    HAPPrecondition(wiFiRouter == server->platform.ip.wiFiRouter);

    HAPWiFiRouterRaiseEvent(
            server,
            &kHAPCharacteristicType_RouterStatus,
            kHAPCharacteristicDebugDescription_RouterStatus,
            /* clientIdentifier: */ 0);
}

void HAPWiFiRouterHandleManagedNetworkStateChanged(HAPPlatformWiFiRouterRef wiFiRouter, void* _Nullable context) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(context);
    HAPAccessoryServer* server = context;
    HAPPrecondition(wiFiRouter == server->platform.ip.wiFiRouter);

    HAPWiFiRouterRaiseEvent(
            server,
            &kHAPCharacteristicType_ManagedNetworkEnable,
            kHAPCharacteristicDebugDescription_ManagedNetworkEnable,
            /* clientIdentifier: */ 0);
}

void HAPWiFiRouterHandleWANConfigurationChanged(HAPPlatformWiFiRouterRef wiFiRouter, void* _Nullable context) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(context);
    HAPAccessoryServer* server = context;
    HAPPrecondition(wiFiRouter == server->platform.ip.wiFiRouter);

    HAPWiFiRouterRaiseEvent(
            server,
            &kHAPCharacteristicType_WANConfigurationList,
            kHAPCharacteristicDebugDescription_WANConfigurationList,
            /* clientIdentifier: */ 0);
}

void HAPWiFiRouterHandleWANStatusChanged(HAPPlatformWiFiRouterRef wiFiRouter, void* _Nullable context) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(context);
    HAPAccessoryServer* server = context;
    HAPPrecondition(wiFiRouter == server->platform.ip.wiFiRouter);

    HAPWiFiRouterRaiseEvent(
            server,
            &kHAPCharacteristicType_WANStatusList,
            kHAPCharacteristicDebugDescription_WANStatusList,
            /* clientIdentifier: */ 0);
}

void HAPWiFiRouterHandleAccessViolationMetadataChanged(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        void* _Nullable context) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(clientIdentifier);
    HAPPrecondition(context);
    HAPAccessoryServer* server = context;
    HAPPrecondition(wiFiRouter == server->platform.ip.wiFiRouter);

    HAPWiFiRouterRaiseEvent(
            server,
            &kHAPCharacteristicType_NetworkAccessViolationControl,
            kHAPCharacteristicDebugDescription_NetworkAccessViolationControl,
            clientIdentifier);
}

void HAPWiFiRouterHandleSatelliteStatusChanged(
        HAPPlatformWiFiRouterRef wiFiRouter,
        size_t satelliteIndex,
        void* _Nullable context) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(context);
    HAPAccessoryServer* server = context;
    HAPPrecondition(server->primaryAccessory);
    HAPPrecondition(server->platform.ip.wiFiRouter == wiFiRouter);

    size_t currentSatelliteIndex = 0;

    const HAPAccessory* accessory = server->primaryAccessory;
    if (accessory->services) {
        for (size_t i = 0; accessory->services[i]; i++) {
            const HAPService* service = accessory->services[i];
            if (HAPUUIDAreEqual(service->serviceType, &kHAPServiceType_WiFiSatellite)) {
                HAPLogServiceError(
                        &logObject,
                        service,
                        accessory,
                        "Satellite accessories must be published as bridged accessories.");
                HAPFatalError();
            }
        }
    }
    if (server->ip.bridgedAccessories) {
        for (size_t i = 0; server->ip.bridgedAccessories[i]; i++) {
            accessory = server->ip.bridgedAccessories[i];
            if (accessory->services) {
                for (size_t j = 0; accessory->services[j]; j++) {
                    const HAPService* service = accessory->services[j];
                    if (HAPUUIDAreEqual(service->serviceType, &kHAPServiceType_WiFiSatellite)) {
                        if (currentSatelliteIndex < satelliteIndex) {
                            currentSatelliteIndex++;
                        } else {
                            HAPAssert(currentSatelliteIndex == satelliteIndex);
                            if (service->characteristics) {
                                for (size_t k = 0; service->characteristics[k]; k++) {
                                    const HAPBaseCharacteristic* characteristic = service->characteristics[k];
                                    if (HAPUUIDAreEqual(
                                                characteristic->characteristicType,
                                                &kHAPCharacteristicType_WiFiSatelliteStatus)) {
                                        HAPAccessoryServerRaiseEvent(server, characteristic, service, accessory);
                                        return;
                                    }
                                }
                            }
                            HAPLogServiceError(
                                    &logObject,
                                    service,
                                    accessory,
                                    "No characteristic of type %s found.",
                                    kHAPCharacteristicDebugDescription_WiFiSatelliteStatus);
                            HAPFatalError();
                        }
                    }
                }
            }
        }
    }
    HAPLog(&logObject,
           "[%p] Satellite accessory status changed but satellite accessory with index %zu not found.",
           (const void*) wiFiRouter,
           satelliteIndex);
}

HAP_RESULT_USE_CHECK
bool HAPWiFiRouterHostDNSNameIsValid(const char* value) {
    HAPPrecondition(value);

    size_t numBytes = 0;
    size_t numLabels = 0;
    const char* label = NULL;
    bool isLabelNumeric = true;
    for (const char* c = value;; c++) {
        if (*c && *c != '-' && *c != '.' && !HAPASCIICharacterIsAlphanumeric(*c)) {
            HAPLog(&logObject, "DNS name %s invalid: Invalid character %c.", value, *c);
            return false;
        }
        if (*c) {
            numBytes++;
        }
        if (!label) {
            if (!*c || *c == '.') {
                HAPLog(&logObject, "DNS name %s invalid: Empty label.", value);
                return false;
            }
            if (*c == '-') {
                HAPLog(&logObject, "DNS name %s invalid: Label starts with hyphen.", value);
                return false;
            }
            label = c;
        }
        if (*c && *c != '.') {
            if (!HAPASCIICharacterIsNumber(*c)) {
                isLabelNumeric = false;
            }
        } else {
            if (c[-1] == '-') {
                HAPLog(&logObject, "DNS name %s invalid: Label ends with hyphen.", value);
                return false;
            }
            size_t numLabelBytes = (size_t)(c - label);
            if (numLabelBytes > 63) {
                HAPLog(&logObject, "DNS name %s invalid: Label is too long (%zu).", value, numLabelBytes);
                return false;
            }
            numLabels++;
            if (!*c) {
                if (numLabels < 2) {
                    HAPLog(&logObject, "DNS name %s invalid: Not enough labels (%zu).", value, numLabels);
                    return false;
                }
                if (isLabelNumeric) {
                    HAPLog(&logObject, "DNS name %s invalid: Final label is fully numeric.", value);
                    return false;
                }
                if (numLabelBytes == 5 && (label[0] == 'l' || label[0] == 'L') &&
                    (label[1] == 'o' || label[1] == 'O') && (label[2] == 'c' || label[2] == 'C') &&
                    (label[3] == 'a' || label[3] == 'A') && (label[4] == 'l' || label[4] == 'L')) {
                    HAPLog(&logObject, "DNS name %s invalid: \"local\" pseudo-TLD.", value);
                    return false;
                }
                if (numBytes < 1 || numBytes > 253) {
                    HAPLog(&logObject, "DNS name %s has invalid length (%zu).", value, numBytes);
                    return false;
                }
                return true;
            }
            label = NULL;
            isLabelNumeric = true;
        }
    }
}

HAP_RESULT_USE_CHECK
bool HAPWiFiRouterHostDNSNamePatternIsValid(const char* value) {
    HAPPrecondition(value);

    size_t numBytes = 0;
    size_t numLabels = 0;
    const char* label = NULL;
    bool isLabelNumeric = true;
    size_t numWildcards = 0;
    bool previousLabelHasWildcard = true;
    for (const char* c = value;; c++) {
        if (*c && *c != '*' && // Wildcard.
            *c != '-' && *c != '.' && !HAPASCIICharacterIsAlphanumeric(*c)) {
            HAPLog(&logObject, "DNS name pattern %s invalid: Invalid character %c.", value, *c);
            return false;
        }
        if (*c && *c != '*') {
            numBytes++;
        }
        if (!label) {
            if (!*c || *c == '.') {
                HAPLog(&logObject, "DNS name pattern %s invalid: Empty label.", value);
                return false;
            }
            if (*c == '-') {
                HAPLog(&logObject, "DNS name pattern %s invalid: Label starts with hyphen.", value);
                return false;
            }
            label = c;
        }
        if (*c && *c != '.') {
            if (!HAPASCIICharacterIsNumber(*c)) {
                isLabelNumeric = false;
            }
            if (*c == '*') {
                if (numWildcards >= 2) {
                    HAPLog(&logObject,
                           "DNS name pattern %s invalid: Wildcard character used more than twice per label.",
                           value);
                    return false;
                }
                numWildcards++;
            }
        } else {
            if (c[-1] == '-') {
                HAPLog(&logObject, "DNS name pattern %s invalid: Label ends with hyphen.", value);
                return false;
            }
            size_t numLabelBytes = (size_t)(c - label) - numWildcards;
            if (numLabelBytes > 63) {
                HAPLog(&logObject, "DNS name pattern %s invalid: Label is too long (%zu).", value, numLabelBytes);
                return false;
            }
            numLabels++;
            if (!*c) {
                if (numLabels < 2) {
                    HAPLog(&logObject, "DNS name pattern %s invalid: Not enough labels (%zu).", value, numLabels);
                    return false;
                }
                if (numWildcards != 0 || previousLabelHasWildcard) {
                    HAPLog(&logObject, "DNS name pattern %s invalid: Wildcard within final two labels.", value);
                    return false;
                }
                if (isLabelNumeric) {
                    HAPLog(&logObject, "DNS name pattern %s invalid: Final label is fully numeric.", value);
                    return false;
                }
                if (numLabelBytes == 5 && (label[0] == 'l' || label[0] == 'L') &&
                    (label[1] == 'o' || label[1] == 'O') && (label[2] == 'c' || label[2] == 'C') &&
                    (label[3] == 'a' || label[3] == 'A') && (label[4] == 'l' || label[4] == 'L')) {
                    HAPLog(&logObject, "DNS name pattern %s invalid: \"local\" pseudo-TLD.", value);
                    return false;
                }
                if (numBytes < 1 || numBytes > 253) {
                    HAPLog(&logObject,
                           "DNS name pattern %s has invalid length after removing wildcards (%zu).",
                           value,
                           numBytes);
                    return false;
                }
                return true;
            }
            label = NULL;
            isLabelNumeric = true;
            previousLabelHasWildcard = numWildcards != 0;
            numWildcards = 0;
        }
    }
}

#endif
