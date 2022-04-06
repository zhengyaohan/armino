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

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)

#include "util_base64.h"

#include "HAPIPAccessoryServer.h"

#include "HAPAccessory+Camera.h"
#include "HAPAccessory+Info.h"
#include "HAPAccessoryServer+Internal.h"
#include "HAPBitSet.h"
#include "HAPCharacteristic.h"
#include "HAPDataStream.h"
#include "HAPIP+ByteBuffer.h"
#include "HAPIPCharacteristic.h"
#include "HAPIPSecurityProtocol.h"
#include "HAPIPServiceDiscovery.h"
#include "HAPIPSession.h"
#include "HAPLog+Attributes.h"
#include "HAPLogSubsystem.h"
#include "HAPMFiTokenAuth.h"
#include "HAPPDU.h"
#include "HAPServiceTypes.h"
#include "HAPTLV+Internal.h"
#include "HAPWACAppleDeviceIE.h"
#include "HAPWACEngine.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "IPAccessoryServer" };

/** Build-time flag to disable session security. */
#define kHAPIPAccessoryServer_SessionSecurityDisabled ((bool) false)

#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 1)
/** Description of inbound buffer. */
#define kHAPIPAccessoryServer_InboundBufferDescription ("inbound buffer")

/** Description of outbound buffer. */
#define kHAPIPAccessoryServer_OutboundBufferDescription ("outbound buffer")

/** Maximum encrypted frame length in the IP security protocol. */
#define kHAPIPAccessoryServer_MaxEncryptedFrameBytes \
    (HAPIPSecurityProtocolGetNumEncryptedBytes(kHAPIPSecurityProtocol_MaxFrameBytes))

/** Maximum inbound buffer capacity that will be allocated on a unsecured session. */
#define kHAPIPAccessoryServer_MaxUnsecuredInboundBufferBytes ((size_t) 4 * kHAPIPAccessoryServer_MaxEncryptedFrameBytes)
#endif

/** Minimum outbound buffer capacity. */
#define kHAPIPAccessoryServer_MinOutboundBufferBytes ((size_t) 128)

/** US-ASCII horizontal-tab character. */
#define kHAPIPAccessoryServerCharacter_HorizontalTab ((char) 9)

/** US-ASCII space character. */
#define kHAPIPAccessoryServerCharacter_Space ((char) 32)

/**
 * HAP status codes.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Table 6-11 HAP Status Codes
 */
/**@{*/
/** This specifies a success for the request. */
#define kHAPIPAccessoryServerStatusCode_Success ((int32_t) 0)

/** Request denied due to insufficient privileges. */
#define kHAPIPAccessoryServerStatusCode_InsufficientPrivileges ((int32_t) -70401)

/** Unable to perform operation with requested characteristic. */
#define kHAPIPAccessoryServerStatusCode_UnableToPerformOperation ((int32_t) -70402)

/** Resource is busy, try again. */
#define kHAPIPAccessoryServerStatusCode_ResourceIsBusy ((int32_t) -70403)

/** Cannot write to read only characteristic. */
#define kHAPIPAccessoryServerStatusCode_WriteToReadOnlyCharacteristic ((int32_t) -70404)

/** Cannot read from a write only characteristic. */
#define kHAPIPAccessoryServerStatusCode_ReadFromWriteOnlyCharacteristic ((int32_t) -70405)

/** Notification is not supported for characteristic. */
#define kHAPIPAccessoryServerStatusCode_NotificationNotSupported ((int32_t) -70406)

/** Out of resources to process request. */
#define kHAPIPAccessoryServerStatusCode_OutOfResources ((int32_t) -70407)

/** Resource does not exist. */
#define kHAPIPAccessoryServerStatusCode_ResourceDoesNotExist ((int32_t) -70409)

/** Accessory received an invalid value in a write request. */
#define kHAPIPAccessoryServerStatusCode_InvalidValueInWrite ((int32_t) -70410)

/** Insufficient Authorization. */
#define kHAPIPAccessoryServerStatusCode_InsufficientAuthorization ((int32_t) -70411)

/** Not allowed in the current state. */
#define kHAPIPAccessoryServerStatusCode_NotAllowedInCurrentState ((int32_t) -70412)
/**@}*/

/**
 * Predefined HTTP/1.1 response indicating success of a WAC message.
 */
#define kHAPIPAccessoryServerResponse_WACSuccess \
    ("HTTP/1.1 200 OK\r\n" \
     "Content-Type: application/octet-stream\r\n" \
     "Content-Length: 0\r\n\r\n")

/**
 * Predefined HTTP/1.1 response indicating successful request completion with an empty response body.
 */
#define kHAPIPAccessoryServerResponse_NoContent ("HTTP/1.1 204 No Content\r\n\r\n")

/**
 * Predefined HTTP/1.1 response indicating that the client is not allowed
 * to request the corresponding operation in the current state.
 */
#define kHAPIPAccessoryServerResponse_NotAllowedInCurrentState \
    ("HTTP/1.1 207 Multi-Status\r\n" \
     "Content-Type: application/hap+json\r\n" \
     "Content-Length: 17\r\n\r\n" \
     "{\"status\":-70412}")

/**
 * Predefined HTTP/1.1 response indicating a malformed request.
 */
#define kHAPIPAccessoryServerResponse_BadRequest \
    ("HTTP/1.1 400 Bad Request\r\n" \
     "Content-Length: 0\r\n\r\n")

/**
 * Predefined HTTP/1.1 response indicating that the client has insufficient privileges to request the corresponding
 * operation.
 */
#define kHAPIPAccessoryServerResponse_InsufficientPrivileges \
    ("HTTP/1.1 400 Bad Request\r\n" \
     "Content-Type: application/hap+json\r\n" \
     "Content-Length: 17\r\n\r\n" \
     "{\"status\":-70401}")

/**
 * Predefined HTTP/1.1 response indicating that the requested resource is not available.
 */
#define kHAPIPAccessoryServerResponse_ResourceNotFound \
    ("HTTP/1.1 404 Not Found\r\n" \
     "Content-Length: 0\r\n\r\n")

/**
 * Predefined HTTP/1.1 response indicating that the requested operation is not supported for the requested resource.
 */
#define kHAPIPAccessoryServerResponse_MethodNotAllowed \
    ("HTTP/1.1 405 Method Not Allowed\r\n" \
     "Content-Length: 0\r\n\r\n")

/**
 * Predefined HTTP/1.1 response indicating that the connection is not authorized to request the corresponding operation.
 */
#define kHAPIPAccessoryServerResponse_ConnectionAuthorizationRequired \
    ("HTTP/1.1 470 Connection Authorization Required\r\n" \
     "Content-Length: 0\r\n\r\n")

/**
 * Predefined HTTP/1.1 response indicating that the connection is not authorized to request the corresponding operation,
 * including a HAP status code.
 */
#define kHAPIPAccessoryServerResponse_ConnectionAuthorizationRequiredWithStatus \
    ("HTTP/1.1 470 Connection Authorization Required\r\n" \
     "Content-Type: application/hap+json\r\n" \
     "Content-Length: 17\r\n\r\n" \
     "{\"status\":-70411}")

/**
 * Predefined HTTP/1.1 response indicating that the server encountered an unexpected condition which prevented it from
 * successfully processing the request.
 */
#define kHAPIPAccessoryServerResponse_InternalServerError \
    ("HTTP/1.1 500 Internal Server Error\r\n" \
     "Content-Length: 0\r\n\r\n")

/**
 * Predefined HTTP/1.1 response indicating that the server did not have enough resources to process request.
 */
#define kHAPIPAccessoryServerResponse_OutOfResources \
    ("HTTP/1.1 500 Internal Server Error\r\n" \
     "Content-Type: application/hap+json\r\n" \
     "Content-Length: 17\r\n\r\n" \
     "{\"status\":-70407}")

/**
 * Maximum amount of time in milliseconds that transmitted data may remain unacknowledged
 * before TCP will forcibly close the corresponding connection.
 *
 * @see RFC 5482 - https://tools.ietf.org/html/rfc5482
 */
#define kHAPIPAccessoryServer_TCPUserTimeout ((HAPTime)(10 * HAPSecond))

/**
 * Maximum time an IP session can stay idle before it will be closed by the accessory server.
 *
 * - Maximum idle time will be enforced during shutdown of the accessory server.
 */
#define kHAPIPSession_MaxIdleTime ((HAPTime)(60 * HAPSecond))

/**
 * Maximum delay during which event notifications will be coalesced into a single message.
 */
#define kHAPIPAccessoryServer_MaxEventNotificationDelay ((HAPTime)(1 * HAPSecond))

/**
 * Maximum time the accessory server will stay in WAC mode.
 */
#define kHAPIPAccessoryServer_MaxWACModeTime ((HAPTime)(15 * HAPMinute))

/**
 * Maximum time from activity until next WAC step must be complete.
 *
 * - 65 seconds = iOS 11.3 timeout on controller side to discover _hap._tcp service.
 * - CoreUtils: 20 seconds to FindPreConfig, 60 seconds to FindPostConfig, 20 second HTTP exchange, 120 second overall.
 *
 * - Values chosen arbitrarily, not backed by specification.
 */
/**@{*/
#define kHAPIPAccessoryServer_MaxWACModeStepTime                 ((HAPTime)(50 * HAPSecond))
#define kHAPIPAccessoryServer_MaxWACModeWaitingForFINTime        ((HAPTime)(15 * HAPSecond))
#define kHAPIPAccessoryServer_MaxWACModeWaitingForConfiguredTime ((HAPTime)(65 * HAPSecond))
/**@}*/

/**
 * Interval at which TCP progression is checked.
 * TCP streams are closed if there is no progress for a full interval.
 */
#define kHAPIPAccessoryServer_TCPProgressionCheckInterval ((HAPTime)(30 * HAPSecond))//By practise, No TCP progression for last 11.1s.

/**
 * Timeout to poll the WiFi status (WiFi connected, ip address received) after the WAC operation
 */
#define kHAPIPAccessoryServer_WifiStatusRetryTimeout (10 * HAPSecond)

/**
 * Maximum number of retries to poll the WiFi status after the WAC operation
 */
#define kHAPIPAccessoryServer_WifiStatusMaxRetries 3

static void log_result(HAPLogType type, char* msg, int result, const char* function, const char* file, int line) {
    HAPAssert(msg);
    HAPAssert(function);
    HAPAssert(file);

    HAPLogWithType(&logObject, type, "%s:%d - %s @ %s:%d", msg, result, function, file, line);
}

static void log_protocol_error(
        HAPLogType type,
        char* msg,
        HAPIPByteBuffer* b,
        const char* function,
        const char* file,
        int line) {
    HAPAssert(msg);
    HAPAssert(b);
    HAPAssert(function);
    HAPAssert(file);

    HAPLogBufferWithType(
            &logObject,
            b->data,
            b->position,
            type,
            "%s:%lu - %s @ %s:%d",
            msg,
            (unsigned long) b->position,
            function,
            file,
            line);
}

static void get_db_ctx(
        HAPAccessoryServer* server,
        uint64_t aid,
        uint64_t iid,
        const HAPCharacteristic** chr,
        const HAPService** svc,
        const HAPAccessory** acc) {
    HAPPrecondition(server);
    HAPPrecondition(chr);
    HAPPrecondition(svc);
    HAPPrecondition(acc);

    *chr = NULL;
    *svc = NULL;
    *acc = NULL;

    const HAPAccessory* accessory = NULL;

    if (server->primaryAccessory->aid == aid) {
        accessory = server->primaryAccessory;
    } else {
        if (server->ip.bridgedAccessories) {
            for (size_t i = 0; server->ip.bridgedAccessories[i]; i++) {
                if (server->ip.bridgedAccessories[i]->aid == aid) {
                    accessory = server->ip.bridgedAccessories[i];
                    break;
                }
            }
        }
    }

    if (accessory) {
        size_t i = 0;
        while (accessory->services[i] && !*chr) {
            const HAPService* service = accessory->services[i];
            if (HAPAccessoryServerSupportsService(server, kHAPTransportType_IP, service)) {
                size_t j = 0;
                while (service->characteristics[j] && !*chr) {
                    const HAPBaseCharacteristic* characteristic = service->characteristics[j];
                    if (HAPIPCharacteristicIsSupported(characteristic)) {
                        if (characteristic->iid == iid) {
                            *chr = characteristic;
                            *svc = service;
                            *acc = accessory;
                        } else {
                            j++;
                        }
                    } else {
                        j++;
                    }
                }
                if (!*chr) {
                    i++;
                }
            } else {
                i++;
            }
        }
    }
}

static void publish_homeKit_service(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    HAPAssert(!server->ip.isServiceDiscoverable);
    HAPAssert(HAPPlatformTCPStreamManagerIsListenerOpen(HAPNonnull(server->platform.ip.tcpStreamManager)));

    HAPIPServiceDiscoverySetHAPService(server);
    server->ip.isServiceDiscoverable = true;
}

static void HandlePendingTCPStream(HAPPlatformTCPStreamManagerRef tcpStreamManager, void* _Nullable context);

static void handle_wac_mode(HAPAccessoryServer* server);

/**
 * Called when we exit wac mode. Stop the service discovery and stop software access point. Re-enter WAC mode if
 * reEnterWACMode is set to true.
 *
 * @param server         HAP Accessory Server object
 * @param reEnterWACMode Flag to indicate if we will re-enter WAC mode. Set to true for WAC failures. Set to false
 *                       when WAC is successful or accessory server is stopped or wac mode timer expires.
 *
 */
static void exit_wac_mode(HAPAccessoryServer* server, bool reEnterWACMode) {
    HAPPrecondition(server);

    HAPAssert(
            !HAPPlatformTCPStreamManagerIsListenerOpen(HAPNonnull(server->platform.ip.tcpStreamManager)) &&
            (server->ip.numSessions == 0));

    HAPAssert(server->ip.isInWACMode);

    // Stop service discovery.
    // WAC mode uses server->ip.discoverableService flag. WAC2 uses server->ip.isServiceDiscoverable flag.
    if (server->ip.discoverableService || server->ip.isServiceDiscoverable) {
        HAPIPServiceDiscoveryStop(server);
    }
    server->ip.discoverableService = false;
    server->ip.isServiceDiscoverable = false;

    // Stop software access point.
    if (server->ip.wac.softwareAccessPointIsActive) {
        HAPPlatformSoftwareAccessPointStop(HAPNonnull(server->platform.ip.wiFi.softwareAccessPoint));
        server->ip.wac.softwareAccessPointIsActive = false;
    }

    // Delete Wi-Fi configuration.
    if (server->ip.wac.wiFiConfiguration.isSet && !server->ip.wac.wiFiConfiguration.isApplied) {
        HAPLog(&logObject, "Deleting cached Wi-Fi configuration as WAC mode is aborted.");
    }

    // Stop WAC mode timer.
    if (server->ip.wacModeTimer) {
        HAPLogInfo(&logObject, "Stopping Wi-Fi configuration timer as WAC mode is aborted.");
        HAPPlatformTimerDeregister(server->ip.wacModeTimer);
        server->ip.wacModeTimer = 0;
    }

    if (server->ip.checkWiFiStatusTimer) {
        HAPLogInfo(&logObject, "Stopping check wifi status timer as WAC mode is aborted.");
        HAPPlatformTimerDeregister(server->ip.checkWiFiStatusTimer);
        server->ip.checkWiFiStatusTimer = 0;
    }

    server->ip.isInWACMode = false;

    if (!reEnterWACMode) {
        // Reset WAC state.
        HAPRawBufferZero(&server->ip.wac, sizeof server->ip.wac);
        // Update WAC mode.
        HAPAccessoryServerDelegateScheduleHandleUpdatedState(server);
    } else {
        // Reset WAC state.
        HAPRawBufferZero(&server->ip.wac, sizeof server->ip.wac);
        // Re-enter WAC mode
        handle_wac_mode(server);
    }
}

static void schedule_max_idle_time_timer(HAPAccessoryServer* server);

static void handle_wac_mode_timer(HAPPlatformTimerRef timer, void* _Nullable context) {
    HAPPrecondition(context);
    HAPAccessoryServer* server = context;
    HAPPrecondition(timer == server->ip.wacModeTimer);
    server->ip.wacModeTimer = 0;

    HAPAssert(server->ip.isInWACMode);

    HAPLog(&logObject, "Wi-Fi configuration timer expired.");

    HAPAssert(server->ip.state == kHAPIPAccessoryServerState_Running);
    HAPAssert(server->ip.nextState == kHAPIPAccessoryServerState_Undefined);
    server->ip.state = kHAPIPAccessoryServerState_Stopping;
    server->ip.nextState = kHAPIPAccessoryServerState_Running;
    schedule_max_idle_time_timer(server);
}

static void handle_wac_mode(HAPAccessoryServer* server) {
    HAPPrecondition(server);
    HAPPrecondition(server->primaryAccessory);

    HAPError err;

    HAPLog(&logObject, "Entering WAC mode.");

    HAPAssert(!HAPPlatformTCPStreamManagerIsListenerOpen(HAPNonnull(server->platform.ip.tcpStreamManager)));

    // Enter WAC mode.
    HAPAssert(!server->ip.isInWACMode);
    server->ip.isInWACMode = true;

    // Reset number of retries to check wifi status
    server->ip.numCheckWiFiStatusRetries = 0;

    HAPAccessoryServerDelegateScheduleHandleUpdatedState(server);

    // Set up WAC mode timer.
    HAPLogInfo(
            &logObject,
            "Waiting for initial Wi-Fi configuration message (timeout: %llu seconds).",
            (unsigned long long) (kHAPIPAccessoryServer_MaxWACModeTime / HAPSecond));
    HAPAssert(!server->ip.wacModeTimer);
    err = HAPPlatformTimerRegister(
            &server->ip.wacModeTimer,
            HAPPlatformClockGetCurrent() + kHAPIPAccessoryServer_MaxWACModeTime,
            handle_wac_mode_timer,
            server);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPLog(&logObject, "Not enough resources to start Wi-Fi configuration timer!");
        HAPFatalError();
    }

    // Open listener.
    HAPPlatformTCPStreamManagerSetTCPUserTimeout(
            HAPNonnull(server->platform.ip.tcpStreamManager), kHAPIPAccessoryServer_TCPUserTimeout);
    HAPPlatformTCPStreamManagerOpenListener(
            HAPNonnull(server->platform.ip.tcpStreamManager), HandlePendingTCPStream, server);
    HAPAssert(HAPPlatformTCPStreamManagerIsListenerOpen(HAPNonnull(server->platform.ip.tcpStreamManager)));

    // Get vendor-specific IE to be broadcasted by the software access point.
    uint8_t appleDeviceIEBytes[kHAPWACAppleDeviceIE_MaxBytes];
    size_t numAppleDeviceIEBytes;
    err = HAPWACAppleDeviceIECreate(server, appleDeviceIEBytes, sizeof appleDeviceIEBytes, &numAppleDeviceIEBytes);

    uint8_t statusFlags = HAPAccessoryServerGetStatusFlags(server);
    server->ip.wac.isInReWACMode = (!((statusFlags & 0x01) == 0x01)) && ((statusFlags & 0x02) == 0x02);

    HAPAssert(!err);
    // Start software access point.
    HAPAssert(!server->ip.wac.softwareAccessPointIsActive);
    HAPPlatformSoftwareAccessPointStart(
            HAPNonnull(server->platform.ip.wiFi.softwareAccessPoint), appleDeviceIEBytes, numAppleDeviceIEBytes);
    server->ip.wac.softwareAccessPointIsActive = true;

    // Register Bonjour service.
    HAPIPServiceDiscoverySetHAPService(server);
}

HAP_RESULT_USE_CHECK
static char* GetHttpMethodBytes(HAPIPSessionDescriptor* session) {
    HAPPrecondition(session);
    HAPPrecondition(session->inboundBuffer.data);
    HAPPrecondition(session->httpMethodIsDefined);

    return &session->inboundBuffer.data[session->httpMethod.position];
}

HAP_RESULT_USE_CHECK
static char* GetHttpURIBytes(HAPIPSessionDescriptor* session) {
    HAPPrecondition(session);
    HAPPrecondition(session->inboundBuffer.data);
    HAPPrecondition(session->httpURIIsDefined);

    return &session->inboundBuffer.data[session->httpURI.position];
}

HAP_RESULT_USE_CHECK
static char* GetHttpHeaderFieldNameBytes(HAPIPSessionDescriptor* session) {
    HAPPrecondition(session);
    HAPPrecondition(session->inboundBuffer.data);
    HAPPrecondition(session->httpHeaderFieldNameIsDefined);

    return &session->inboundBuffer.data[session->httpHeaderFieldName.position];
}

HAP_RESULT_USE_CHECK
static char* GetHttpHeaderFieldValueBytes(HAPIPSessionDescriptor* session) {
    HAPPrecondition(session);
    HAPPrecondition(session->inboundBuffer.data);
    HAPPrecondition(session->httpHeaderFieldValueIsDefined);

    return &session->inboundBuffer.data[session->httpHeaderFieldValue.position];
}

HAP_RESULT_USE_CHECK
static bool SessionHasEnabledEventNotifications(HAPIPSessionDescriptor* session) {
    HAPPrecondition(session);

    return session->numEventNotifications != 0;
}

HAP_RESULT_USE_CHECK
static size_t GetNumSessionsWithEnabledEventNotifications(const HAPAccessoryServer* server) {
    HAPPrecondition(server);

    size_t numSessionsWithEnabledEventNotifications = 0;

    for (size_t i = 0; i < server->ip.storage->numSessions; i++) {
        HAPIPSession* ipSession = &server->ip.storage->sessions[i];
        HAPIPSessionDescriptor* session = &ipSession->descriptor;
        if (!session->server) {
            continue;
        }

        if (SessionHasEnabledEventNotifications(session)) {
            numSessionsWithEnabledEventNotifications++;
            HAPAssert(numSessionsWithEnabledEventNotifications);
        }
    }

    return numSessionsWithEnabledEventNotifications;
}

HAP_RESULT_USE_CHECK
static bool SessionIsLastSessionWithEnabledEventNotifications(HAPIPSessionDescriptor* session) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);

    return SessionHasEnabledEventNotifications(session) &&
           GetNumSessionsWithEnabledEventNotifications(HAPNonnull(session->server)) == 1;
}

HAP_RESULT_USE_CHECK
static bool SessionHasPendingEventNotifications(HAPIPSessionDescriptor* session) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);

#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 0)
    return session->numEventNotificationFlags;
#else
    HAPAccessoryServer* server = session->server;
    return session->numEventNotificationFlags ||
           (session->eventNotificationFlags &&
            !HAPBitSetIsEmpty(HAPNonnull(session->eventNotificationFlags), server->ip.numEventNotificationBitSetBytes));
#endif
}

static void CloseSession(HAPIPSessionDescriptor* session);

static void schedule_event_notifications(HAPAccessoryServer* server);

static void RepublishHAPService(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    if (server->ip.isServiceDiscoverable) {
        if (!HAPAccessoryServerIsPaired(server)) {
            HAPLog(&logObject, "Not re-publishing HomeKit service because accessory server is not paired.");
        } else {
            HAPLog(&logObject, "Re-publishing HomeKit service.");
            HAPIPServiceDiscoverySetHAPService(server);
        }
    }
}

static void HandleWACFailure(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    if (!server->ip.wac.isInReWACMode) {
        HAPLogDebug(&logObject, "Removing all pairing data since WAC failed");
        HAPError err = HAPPairingRemoveAll(server);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPLogError(&logObject, "Failed to remove pairing data. Aborting. Please reset the accessory");
            HAPFatalError();
        }
    }

    // Forget current Wi-Fi configuration.
    if (HAPPlatformWiFiManagerIsConfigured(HAPNonnull(server->platform.ip.wiFi.wiFiManager))) {
        HAPPlatformWiFiManagerClearConfiguration(HAPNonnull(server->platform.ip.wiFi.wiFiManager));
    }
    // Since there was a WAC failure, we will re-enter WAC mode.
    exit_wac_mode(server, true);
}

static void HandleTCPProgressionTimerExpired(HAPPlatformTimerRef timer, void* _Nullable context) {
    HAPPrecondition(context);
    HAPAccessoryServer* server = context;
    HAPPrecondition(timer == server->ip.tcpProgressionTimer);
    server->ip.tcpProgressionTimer = 0;

    HAPError err;

    // A global timer is running that periodically checks for TCP transmission
    // progress. As this timer may be scheduled to fire immediately after a write
    // occurred, a situation may arise where a socket is closed incorrectly as the
    // socket was not given any time to send the enqueued payload. Therefore, the
    // flag tcpDidProgress was introduced to protect a socket from being closed the
    // next time when the global timer is checking for progression.

    // If the tcpDidProgress flag would be re-set to true on every write, problems
    // arise when data is constantly streamed (e.g., button events of a remote). In
    // this case, the global timer may never detect that a connection is stale, as
    // the tcpDidProgressFlag that is constantly being re-set would prevent the
    // connection from being closed. As the number of non-acknowledged bytes
    // increases when additional data is enqueued, we need to keep track of the
    // additional data.

    // Ideally, we would increment the number of non-acknowledged bytes by the
    // amount that was added due to the HAPPlatformTCPStreamWrite call. However, the
    // HAPPlatformTCPStreamGetNumNonAcknowledgedBytes may further increment due to
    // additional non-payload data such as TCP headers.

    // One scenario where we cannot reliably detect progression remains:

    // 1. Other writes have already been enqueued, but are not yet fully
    //    transmitted.
    // 2. No progress occurs until HAPPlatformTCPStreamWrite is called.
    // 3. Only a little bit of progress occurs before numNonAcknowledgedBytesAfter
    //    is initialized (<= the amount of additional non-payload data, e.g., TCP
    //    headers).
    // 4. No progress occurs until the global timer checks for progression.

    // In this scenario, tcpDidProgress would not be set to true, and the timer may
    // close the session before the usual timeout.

    // For now, we assume that case 3 may not occur on popular platforms, as TCP
    // headers may not be partially acknowledged. However, we are not sure what
    // additional non-payload data may be reported by
    // HAPPlatformTCPStreamGetNumNonAcknowledgedBytes across different platforms.
    // For example, when using SIOCOUTQ on Linux or SO_NWRITE on macOS, the overhead
    // is sometimes only reported as 18 bytes although a TCP header is usually 20-60
    // bytes in size. Other platforms may have even different measurements.

    // We try to additionally mitigate this situation by initializing
    // numNonAcknowledgedBytesAfter as soon as possible after calling
    // HAPPlatformTCPStreamWrite.

    // Update TCP progression.
    for (size_t i = 0; i < server->ip.storage->numSessions; i++) {
        HAPIPSession* ipSession = &server->ip.storage->sessions[i];
        HAPIPSessionDescriptor* session = &ipSession->descriptor;
        if (!session->server) {
            continue;
        }
        if (session->state == kHAPIPSessionState_Idle) {
            continue;
        }

        if (session->tcpStreamIsOpen) {
            size_t numNonAcknowledgedBytes = HAPPlatformTCPStreamGetNumNonAcknowledgedBytes(
                    HAPNonnull(server->platform.ip.tcpStreamManager), session->tcpStream);
            if (numNonAcknowledgedBytes) {
                if (numNonAcknowledgedBytes == session->numNonAcknowledgedBytes) {
                    if (session->tcpDidProgress) {
                        session->tcpDidProgress = false;
                    } else {
                        HAPLog(&logObject,
                               "session:%p:No TCP progression for last %llu.%03llus (%zu bytes). Terminating session.",
                               (const void*) session,
                               (unsigned long long) (kHAPIPAccessoryServer_TCPProgressionCheckInterval / HAPSecond),
                               (unsigned long long) (kHAPIPAccessoryServer_TCPProgressionCheckInterval % HAPSecond),
                               numNonAcknowledgedBytes);
                        if (SessionIsLastSessionWithEnabledEventNotifications(session)) {
                            CloseSession(session);
                            RepublishHAPService(server);
                        } else {
                            CloseSession(session);
                        }
                    }
                } else {
                    HAPLogDebug(
                            &logObject,
                            "session:%p:<%zu bytes pending.",
                            (const void*) session,
                            numNonAcknowledgedBytes);
                    session->tcpDidProgress = true;
                }
            }
            session->numNonAcknowledgedBytes = numNonAcknowledgedBytes;
        }

        // Additional check for progression between TCP stream events.
        if (session->tcpStreamIsOpen &&
            (((session->state == kHAPIPSessionState_Reading) && (session->inboundBuffer.position != 0)) ||
             (session->state == kHAPIPSessionState_Writing))) {
            HAPTime now = HAPPlatformClockGetCurrent();
            HAPAssert(now >= session->stamp);
            HAPTime delta = now - session->stamp;
            if (delta > kHAPIPAccessoryServer_TCPProgressionCheckInterval) {
                HAPLog(&logObject,
                       "session:%p:No TCP progression for last %llu.%03llus. Terminating session.",
                       (const void*) session,
                       (unsigned long long) (delta / HAPSecond),
                       (unsigned long long) (delta % HAPSecond));
                if (SessionIsLastSessionWithEnabledEventNotifications(session)) {
                    CloseSession(session);
                    RepublishHAPService(server);
                } else {
                    CloseSession(session);
                }
            }
        }
    }

    err = HAPPlatformTimerRegister(
            &server->ip.tcpProgressionTimer,
            HAPPlatformClockGetCurrent() + kHAPIPAccessoryServer_TCPProgressionCheckInterval,
            HandleTCPProgressionTimerExpired,
            server);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPLogError(&logObject, "Not enough resources to start TCP progression timer!");
        HAPFatalError();
    }
}

static void HAPIPSessionDestroy(HAPIPSession* ipSession) {
    HAPPrecondition(ipSession);

    HAPIPSessionDescriptor* session = &ipSession->descriptor;
    if (!session->server) {
        return;
    }
    HAPAccessoryServer* server = session->server;

    HAPLogDebug(&logObject, "session:%p:releasing session", (const void*) session);

#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 0)
    HAPAssert(ipSession->inboundBuffer.bytes);
    HAPRawBufferZero(ipSession->inboundBuffer.bytes, ipSession->inboundBuffer.numBytes);
    HAPAssert(ipSession->outboundBuffer.bytes);
    HAPRawBufferZero(ipSession->outboundBuffer.bytes, ipSession->outboundBuffer.numBytes);
    HAPAssert(ipSession->eventNotifications);
    HAPRawBufferZero(
            ipSession->eventNotifications, ipSession->numEventNotifications * sizeof *ipSession->eventNotifications);
#else
    if (!server->ip.storage->dynamicMemoryAllocation.deallocateMemory) {
        HAPAssert(ipSession->inboundBuffer.bytes);
        HAPRawBufferZero(HAPNonnullVoid(ipSession->inboundBuffer.bytes), ipSession->inboundBuffer.numBytes);
        HAPAssert(ipSession->outboundBuffer.bytes);
        HAPRawBufferZero(HAPNonnullVoid(ipSession->outboundBuffer.bytes), ipSession->outboundBuffer.numBytes);
        HAPAssert(ipSession->eventNotifications);
        HAPRawBufferZero(
                HAPNonnullVoid(ipSession->eventNotifications),
                ipSession->numEventNotifications * sizeof *ipSession->eventNotifications);
    } else {
        HAPAssert(!ipSession->inboundBuffer.bytes);
        HAPAssert(!ipSession->inboundBuffer.numBytes);
        HAPAssert(!session->inboundBuffer.data);
        HAPAssert(!session->inboundBuffer.position);
        HAPAssert(!session->inboundBuffer.limit);
        HAPAssert(!session->inboundBuffer.capacity);

        HAPAssert(!ipSession->outboundBuffer.bytes);
        HAPAssert(!ipSession->outboundBuffer.numBytes);
        HAPAssert(!session->outboundBuffer.data);
        HAPAssert(!session->outboundBuffer.position);
        HAPAssert(!session->outboundBuffer.limit);
        HAPAssert(!session->outboundBuffer.capacity);

        HAPAssert(!ipSession->eventNotifications);
        HAPAssert(!ipSession->numEventNotifications);
        HAPAssert(!session->eventNotificationSubscriptions);
        HAPAssert(!session->eventNotificationFlags);
        HAPAssert(!session->eventNotifications);
        HAPAssert(!session->maxEventNotifications);
    }
#endif

    HAPRawBufferZero(session, sizeof *session);

    HAPLogDebug(&logObject, "session:%p: session released", (const void*) session);

    schedule_event_notifications(server);
}

static void handle_wac_configured_timer(HAPPlatformTimerRef timer, void* _Nullable context) {
    HAPPrecondition(context);
    HAPAccessoryServer* server = context;
    HAPPrecondition(timer == server->ip.wacConfiguredMessageTimer);
    HAPLog(&logObject, "Wi-Fi configuration /configured timer expired.");
    server->ip.wacConfiguredMessageTimer = 0;

    HAPAssert(server->ip.state == kHAPIPAccessoryServerState_Running);
    HAPAssert(server->ip.nextState == kHAPIPAccessoryServerState_Undefined);
}

/**
 * Callback that is called to poll the WiFi status after WiFi configuration is complete. There are maximum 3 retries
 * to get the WiFi status.
 *
 * @param timer          Timer that was registered and expired
 * @param context        Data passed into the callback
 *
 */
void OnWiFiStatusTimerExpired(HAPPlatformTimerRef timer, void* _Nullable context) {
    HAPPrecondition(context);
    HAPAccessoryServer* const server = context;
    HAPError err;

    HAPPrecondition(timer == server->ip.checkWiFiStatusTimer);
    HAPLogInfo(&logObject, "WiFi configuration - check WiFi status timer expired.");
    server->ip.checkWiFiStatusTimer = 0;
    server->ip.numCheckWiFiStatusRetries++;

    bool isWiFiLinkEstablished =
            HAPPlatformWiFiManagerIsWiFiLinkEstablished(HAPNonnull(server->platform.ip.wiFi.wiFiManager));
    ;
    bool isWiFiNetworkConfigured =
            HAPPlatformWiFiManagerIsWiFiNetworkConfigured(HAPNonnull(server->platform.ip.wiFi.wiFiManager));
    ;

    if (isWiFiLinkEstablished && isWiFiNetworkConfigured) {
        HAPLogInfo(&logObject, "WiFi configuration - connected to Wifi successfully.");
        server->ip.wac.wiFiConfiguration.isApplied = true;

        // Controllers determine the initial seed value according to the following priority list:
        // 1. "seed" value (Legacy WAC).
        // 2. "sd" value (AirPlay).
        // 3. "c#" value (HomeKit).
        // 4. 1 constant.
        // See -[EasyConfigDevice configureStart:]
        //
        // After the /config is sent, controllers wait for a Bonjour service with a following seed value
        // according to the following priority list:
        // 1. "seed" value (Legacy WAC).
        // 2. "sd" value (AirPlay).
        // 3. "c#" value (HomeKit).
        // 4. 0 constant.
        // See -[EasyConfigDevice findDevicePostConfigEvent:info:]
        // The new seed value must be different than the initial seed value for the /configured to be sent.
        //
        // Notably, seeds are processed as Int64 by the EasyConfig framework, but HomeKit uses different ranges.
        //
        // Recent controllers also accept any "_hap._tcp" (case-insensitive) Bonjour service
        // with ("sf" & 0x2) = 0 (Wi-Fi is configured) regardless of the resulting seed value.
        // Since we are moving to an implementation where we use _hap._tcp, no configuration
        // number update is needed.

        // Exit WAC mode.
        HAPLog(&logObject, "Wi-Fi configuration complete.");
        // Wifi configuration succeeded, we do not need to re-enter WAC mode.
        exit_wac_mode(server, false);
        HAPAssert(!server->ip.isInWACMode);

        // Open TCP listener again (on new network).
        HAPLogInfo(&logObject, "Opening TCP listener (on new network).");
        HAPPlatformTCPStreamManagerSetTCPUserTimeout(
                HAPNonnull(server->platform.ip.tcpStreamManager), kHAPIPAccessoryServer_TCPUserTimeout);
        HAPPlatformTCPStreamManagerOpenListener(
                HAPNonnull(server->platform.ip.tcpStreamManager), HandlePendingTCPStream, server);
        HAPAssert(HAPPlatformTCPStreamManagerIsListenerOpen(HAPNonnull(server->platform.ip.tcpStreamManager)));
        // Start _hap Bonjour service discovery.
        publish_homeKit_service(server);
        HAPAssert(server->ip.isServiceDiscoverable);

        // Start step timer for completion of WAC.
        HAPLogInfo(
                &logObject,
                "Start a timer for /configured (timeout: %llu seconds).",
                (unsigned long long) (kHAPIPAccessoryServer_MaxWACModeWaitingForConfiguredTime / HAPSecond));
        err = HAPPlatformTimerRegister(
                &server->ip.wacConfiguredMessageTimer,
                HAPPlatformClockGetCurrent() + kHAPIPAccessoryServer_MaxWACModeWaitingForConfiguredTime,
                handle_wac_configured_timer,
                server);
        if (err) {
            HAPLog(&logObject, "Not enough resources to start Wi-Fi configuration step timer!");
            HAPFatalError();
        }

        HAPAssert(!server->ip.wac.softwareAccessPointIsActive);
    } else {
        if (server->ip.numCheckWiFiStatusRetries >= kHAPIPAccessoryServer_WifiStatusMaxRetries) {
            server->ip.wac.wiFiConfiguration.isApplied = false;
            HAPLogError(&logObject, "Failed to connect to wifi successfully.");
            if (HAPPlatformTCPStreamManagerIsListenerOpen(HAPNonnull(server->platform.ip.tcpStreamManager))) {
                HAPPlatformTCPStreamManagerCloseListener(HAPNonnull(server->platform.ip.tcpStreamManager));
            }
            HandleWACFailure(server);
        } else {
            // Retry polling WiFi status until maximum number of retries is reached
            HAPError err = HAPPlatformTimerRegister(
                    &server->ip.checkWiFiStatusTimer,
                    HAPPlatformClockGetCurrent() + kHAPIPAccessoryServer_WifiStatusRetryTimeout,
                    OnWiFiStatusTimerExpired,
                    context);
            if (err) {
                server->ip.checkWiFiStatusTimer = 0;
                HAPLogError(&logObject, "Register timer for checking WiFi status failed");
            }
        }
    }
}

static void collect_garbage(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    HAPError err;

    if (server->ip.garbageCollectionTimer) {
        HAPPlatformTimerDeregister(server->ip.garbageCollectionTimer);
        server->ip.garbageCollectionTimer = 0;
    }

    size_t n = 0;
    for (size_t i = 0; i < server->ip.storage->numSessions; i++) {
        HAPIPSession* ipSession = &server->ip.storage->sessions[i];
        HAPIPSessionDescriptor* session = &ipSession->descriptor;
        if (!session->server) {
            continue;
        }

        if (session->state == kHAPIPSessionState_Idle) {
            HAPIPSessionDestroy(ipSession);
            HAPAssert(server->ip.numSessions > 0);
            server->ip.numSessions--;

            if (!server->ip.numSessions) {
                HAPLogDebug(&logObject, "Stopping TCP progression timer.");

                HAPAssert(server->ip.tcpProgressionTimer);
                HAPPlatformTimerDeregister(server->ip.tcpProgressionTimer);
                server->ip.tcpProgressionTimer = 0;
            }
        } else {
            n++;
        }
    }
    HAPAssert(n == server->ip.numSessions);

    // If there are open sessions, wait until they are closed before continuing.
    if (HAPPlatformTCPStreamManagerIsListenerOpen(HAPNonnull(server->platform.ip.tcpStreamManager)) ||
        (server->ip.numSessions != 0)) {
        return;
    }

    // Finalize server state transition after last session closed.
    HAPAssert(server->ip.state == kHAPIPAccessoryServerState_Stopping);
    if (server->ip.stateTransitionTimer) {
        HAPPlatformTimerDeregister(server->ip.stateTransitionTimer);
        server->ip.stateTransitionTimer = 0;
    }
    if (server->ip.maxIdleTimeTimer) {
        HAPPlatformTimerDeregister(server->ip.maxIdleTimeTimer);
        server->ip.maxIdleTimeTimer = 0;
    }
    HAPLogDebug(&logObject, "Completing accessory server state transition.");
    if (server->ip.nextState == kHAPIPAccessoryServerState_Running) {
        if (server->ip.isInWACModeTransition) {
            // Enter WAC mode.
            // WAC2: NotConfigured / _hap  ==>  SoftAP + _hap
            HAPAssert(!server->ip.isInWACMode);
            HAPAssert(server->ip.isServiceDiscoverable || !server->ip.discoverableService);
            HAPAssert(!server->ip.wac.softwareAccessPointIsActive);

            server->ip.isInWACModeTransition = false;

            HAPLog(&logObject, "Entering WAC mode.");

            // Stop _hap service discovery.
            if (server->ip.isServiceDiscoverable) {
                HAPIPServiceDiscoveryStop(server);
                server->ip.isServiceDiscoverable = false;
            }

            // Forget current Wi-Fi configuration.
            if (HAPPlatformWiFiManagerIsConfigured(HAPNonnull(server->platform.ip.wiFi.wiFiManager))) {
                HAPPlatformWiFiManagerRemoveConfiguration(HAPNonnull(server->platform.ip.wiFi.wiFiManager));
            }

            if (server->ip.wacConfiguredMessageTimer) {
                HAPPlatformTimerDeregister(server->ip.wacConfiguredMessageTimer);
                server->ip.wacConfiguredMessageTimer = 0;
            }

            // Enter WAC mode.
            handle_wac_mode(server);

            HAPAssert(server->ip.isInWACMode);
            HAPAssert(server->ip.discoverableService == kHAPIPServiceDiscoveryType_HAP);
            HAPAssert(server->ip.wac.softwareAccessPointIsActive);
        } else if (server->ip.wac.wiFiConfiguration.isSet && !server->ip.wac.wiFiConfiguration.isApplied) {
            // Apply new Wi-Fi configuration after /config.
            // WAC2:        SoftAP + _hap  ==>  _hap
            HAPAssert(!server->ip.wacModeTimer);

            HAPAssert(server->ip.isInWACMode);

            HAPLog(&logObject, "Received Wi-Fi configuration - Switching to new Wi-Fi network.");

            // Stop service discovery.
            HAPAssert(!server->ip.isInWACMode || !server->ip.isServiceDiscoverable);
            HAPAssert(server->ip.discoverableService == kHAPIPServiceDiscoveryType_HAP);
            HAPIPServiceDiscoveryStop(server);
            server->ip.isServiceDiscoverable = false;

            if (server->ip.wac.softwareAccessPointIsActive) {
                // Stop software access point.
                HAPLogInfo(
                        &logObject,
                        "Stopping software access point. All Service Discovery operations must be completed "
                        "by the platform before actually stopping the software access point.");
                HAPPlatformSoftwareAccessPointStop(HAPNonnull(server->platform.ip.wiFi.softwareAccessPoint));
                server->ip.wac.softwareAccessPointIsActive = false;
            }

            // Apply Wi-Fi configuration.
            HAPLogInfo(&logObject, "Applying Wi-Fi configuration.");
            bool isWiFiSecured = HAPStringGetNumBytes(server->ip.wac.wiFiConfiguration.passphrase) != 0;
            bool isRegulatoryDomainConfigured =
                    HAPStringGetNumBytes(server->ip.wac.wiFiConfiguration.regulatoryDomain) != 0;
            err = HAPPlatformWiFiManagerApplyConfiguration(
                    HAPNonnull(server->platform.ip.wiFi.wiFiManager),
                    server->ip.wac.wiFiConfiguration.ssid,
                    isWiFiSecured ? server->ip.wac.wiFiConfiguration.passphrase : NULL,
                    isRegulatoryDomainConfigured ? server->ip.wac.wiFiConfiguration.regulatoryDomain : NULL
#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_RECONFIGURATION)
                    ,
                    0,
                    0,
                    true
#endif
            );

            HAPRawBufferZero(&server->ip.wac.wiFiConfiguration.ssid, sizeof server->ip.wac.wiFiConfiguration.ssid);
            HAPRawBufferZero(
                    &server->ip.wac.wiFiConfiguration.passphrase, sizeof server->ip.wac.wiFiConfiguration.passphrase);
            HAPRawBufferZero(
                    &server->ip.wac.wiFiConfiguration.regulatoryDomain,
                    sizeof server->ip.wac.wiFiConfiguration.regulatoryDomain);
            if (err) {
                HAPAssert(err == kHAPError_Unknown);
                HAPLogError(&logObject, "Failed to apply wifi configuration.");
                HandleWACFailure(server);
            } else {
                err = HAPPlatformTimerRegister(
                        &server->ip.checkWiFiStatusTimer,
                        HAPPlatformClockGetCurrent() + kHAPIPAccessoryServer_WifiStatusRetryTimeout,
                        OnWiFiStatusTimerExpired,
                        server);
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    HAPLog(&logObject, "Not enough resources to start Wi-Fi status timer!");
                    HAPFatalError();
                }
            }
        } else if (
                server->ip.isInWACMode &&
                HAPPlatformWiFiManagerIsConfigured(HAPNonnull(server->platform.ip.wiFi.wiFiManager))) {
            // Switch to HAP mode after / WAC timeout after /config / External configuration => ExitWAC.
            // WAC2:        SoftAP + _hap  ==>  _hap
            HAPAssert(!server->ip.wacModeTimer);
            HAPAssert(!server->ip.isServiceDiscoverable);
            HAPAssert(server->ip.discoverableService == kHAPIPServiceDiscoveryType_HAP);

            // Exit WAC mode.
            HAPLog(&logObject, "Wi-Fi configuration complete.");
            // Wifi configuration succeeded, we do not need to re-enter WAC mode.
            exit_wac_mode(server, false);

            // Start _hap Bonjour service discovery.
            HAPPlatformTCPStreamManagerSetTCPUserTimeout(
                    HAPNonnull(server->platform.ip.tcpStreamManager), kHAPIPAccessoryServer_TCPUserTimeout);
            HAPPlatformTCPStreamManagerOpenListener(
                    HAPNonnull(server->platform.ip.tcpStreamManager), HandlePendingTCPStream, server);
            HAPAssert(HAPPlatformTCPStreamManagerIsListenerOpen(HAPNonnull(server->platform.ip.tcpStreamManager)));
            publish_homeKit_service(server);

            HAPAssert(!server->ip.isInWACMode);
            HAPAssert(server->ip.isServiceDiscoverable);
        } else if (server->ip.isInWACMode) {
            // Exit WAC mode / WAC timeout.
            // WAC2:        SoftAP + _hap  ==>  NotConfigured
            HAPAssert(!server->ip.wacModeTimer);
            HAPAssert(!server->ip.isServiceDiscoverable);
            HAPAssert(server->ip.discoverableService == kHAPIPServiceDiscoveryType_HAP);

            // Exit WAC mode.
            HAPLog(&logObject, "Wi-Fi configuration failed.");
            // WAC mode timer has expired or we exited WAC mode, we will not re-enter WAC mode.
            exit_wac_mode(server, false);

            HAPAssert(!server->ip.discoverableService);
            HAPAssert(!server->ip.wac.softwareAccessPointIsActive);
            HAPAssert(!server->ip.isInWACMode);
            HAPAssert(!server->ip.isServiceDiscoverable);
        } else {
            // No WAC mode transition.
        }
        server->ip.state = kHAPIPAccessoryServerState_Running;
        server->ip.nextState = kHAPIPAccessoryServerState_Undefined;
        HAPAccessoryServerDelegateScheduleHandleUpdatedState(server);
    } else {
        HAPAssert(server->ip.nextState == kHAPIPAccessoryServerState_Idle);
        // HAPAccessoryServerStop.

        // Stop service discovery.
        // WAC mode uses server->ip.discoverableService flag. WAC2 uses server->ip.isServiceDiscoverable flag.
        if (server->ip.discoverableService || server->ip.isServiceDiscoverable) {
            HAPIPServiceDiscoveryStop(server);
        }
        server->ip.discoverableService = false;
        server->ip.isServiceDiscoverable = false;

        if (server->ip.isInWACMode) {
            // When we are in WAC mode and accessory server stop is called, we will not re-enter WAC mode.
            exit_wac_mode(server, false);
        }

        if (server->ip.wacConfiguredMessageTimer) {
            HAPPlatformTimerDeregister(server->ip.wacConfiguredMessageTimer);
            server->ip.wacConfiguredMessageTimer = 0;
        }
        HAPAssert(!server->ip.wac.wiFiConfiguration.isSet);

        HAPAssert(!server->ip.discoverableService);
        HAPAssert(!server->ip.wac.softwareAccessPointIsActive);
        HAPAssert(!server->ip.isInWACMode);
        HAPAssert(!server->ip.isServiceDiscoverable);

        server->ip.state = kHAPIPAccessoryServerState_Idle;
        server->ip.nextState = kHAPIPAccessoryServerState_Undefined;
        HAPAccessoryServerDelegateScheduleHandleUpdatedState(server);
    }
}

static void handle_garbage_collection_timer(HAPPlatformTimerRef timer, void* _Nullable context) {
    HAPPrecondition(context);
    HAPAccessoryServer* server = context;
    HAPPrecondition(timer == server->ip.garbageCollectionTimer);
    server->ip.garbageCollectionTimer = 0;

    collect_garbage(server);
}

static void handle_max_idle_time_timer(HAPPlatformTimerRef timer, void* _Nullable context) {
    HAPPrecondition(context);
    HAPAccessoryServer* server = context;
    HAPPrecondition(timer == server->ip.maxIdleTimeTimer);
    server->ip.maxIdleTimeTimer = 0;

    HAPLogDebug(&logObject, "Session idle timer expired.");
    schedule_max_idle_time_timer(server);
}

static void schedule_max_idle_time_timer(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    if (server->ip.maxIdleTimeTimer) {
        HAPPlatformTimerDeregister(server->ip.maxIdleTimeTimer);
        server->ip.maxIdleTimeTimer = 0;
    }

    HAPError err;

    HAPTime clock_now_ms = HAPPlatformClockGetCurrent();

    int64_t timeout_ms = -1;

    if ((server->ip.state == kHAPIPAccessoryServerState_Stopping) &&
        HAPPlatformTCPStreamManagerIsListenerOpen(HAPNonnull(server->platform.ip.tcpStreamManager))) {
        HAPPlatformTCPStreamManagerCloseListener(HAPNonnull(server->platform.ip.tcpStreamManager));
    }

    for (size_t i = 0; i < server->ip.storage->numSessions; i++) {
        HAPIPSession* ipSession = &server->ip.storage->sessions[i];
        HAPIPSessionDescriptor* session = &ipSession->descriptor;
        if (!session->server) {
            continue;
        }

        if ((session->state == kHAPIPSessionState_Reading) && (session->inboundBuffer.position == 0) &&
            !SessionHasPendingEventNotifications(session) &&
            (server->ip.state == kHAPIPAccessoryServerState_Stopping)) {
            CloseSession(session);
        } else if (
                ((session->state == kHAPIPSessionState_Reading) || (session->state == kHAPIPSessionState_Writing)) &&
                (server->ip.state == kHAPIPAccessoryServerState_Stopping)) {
            HAPAssert(clock_now_ms >= session->stamp);
            HAPTime dt_ms = clock_now_ms - session->stamp;
            if (dt_ms < kHAPIPSession_MaxIdleTime) {
                HAPAssert(kHAPIPSession_MaxIdleTime <= INT64_MAX);
                int64_t t_ms = (int64_t)(kHAPIPSession_MaxIdleTime - dt_ms);
                if ((timeout_ms == -1) || (t_ms < timeout_ms)) {
                    timeout_ms = t_ms;
                }
            } else {
                HAPLogInfo(&logObject, "Connection timeout.");
                CloseSession(session);
            }
        }
    }

    if (timeout_ms >= 0) {
        HAPTime deadline_ms;

        if (UINT64_MAX - clock_now_ms < (HAPTime) timeout_ms) {
            HAPLog(&logObject, "Clipping maximum idle time timer to avoid clock overflow.");
            deadline_ms = UINT64_MAX;
        } else {
            deadline_ms = clock_now_ms + (HAPTime) timeout_ms;
        }
        HAPAssert(deadline_ms >= clock_now_ms);

        err = HAPPlatformTimerRegister(&server->ip.maxIdleTimeTimer, deadline_ms, handle_max_idle_time_timer, server);
        if (err) {
            HAPLog(&logObject, "Not enough resources to schedule maximum idle time timer!");
            HAPFatalError();
        }
        HAPAssert(server->ip.maxIdleTimeTimer);
    }

    if (!server->ip.garbageCollectionTimer) {
        err = HAPPlatformTimerRegister(&server->ip.garbageCollectionTimer, 0, handle_garbage_collection_timer, server);
        if (err) {
            HAPLog(&logObject, "Not enough resources to schedule garbage collection!");
            HAPFatalError();
        }
        HAPAssert(server->ip.garbageCollectionTimer);
    }
}

#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 1)
HAP_RESULT_USE_CHECK
static HAPError AllocateBuffer(
        HAPIPSessionDescriptor* session,
        HAPIPByteBuffer* buffer,
        const char* bufferDescription,
        size_t numBytes) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPPrecondition((buffer == &session->inboundBuffer) || (buffer == &session->outboundBuffer));
    HAPPrecondition(bufferDescription);

    HAPPrecondition(!buffer->data);

    HAPAccessoryServer* server = session->server;
    HAPPrecondition(server->ip.storage->dynamicMemoryAllocation.allocateMemory);

    HAPPrecondition(numBytes);

    char* _Nullable bufferData = server->ip.storage->dynamicMemoryAllocation.allocateMemory(numBytes);
    if (!bufferData) {
        HAPLog(&logObject,
               "session:%p:failed to allocate %s (%zu)",
               (const void*) session,
               bufferDescription,
               numBytes);
        return kHAPError_OutOfResources;
    }

    HAPRawBufferZero(HAPNonnull(bufferData), numBytes);

    buffer->data = HAPNonnull(bufferData);
    buffer->capacity = numBytes;
    buffer->limit = buffer->capacity;
    buffer->position = 0;
    session->inboundBufferMark = 0;

    HAPLogDebug(
            &logObject, "session:%p:allocated %s (%zu)", (const void*) session, bufferDescription, buffer->capacity);

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static HAPError ReallocateBuffer(
        HAPIPSessionDescriptor* session,
        HAPIPByteBuffer* buffer,
        const char* bufferDescription,
        size_t numBytes) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPPrecondition((buffer == &session->inboundBuffer) || (buffer == &session->outboundBuffer));
    HAPPrecondition(bufferDescription);

    HAPPrecondition(buffer->data);
    HAPPrecondition(buffer->position <= buffer->limit);
    HAPPrecondition(buffer->limit <= buffer->capacity);

    HAPAccessoryServer* server = session->server;
    HAPPrecondition(server->ip.storage->dynamicMemoryAllocation.reallocateMemory);

    if (numBytes < buffer->capacity) {
        HAPRawBufferZero(&buffer->data[numBytes], buffer->capacity - numBytes);
    }

    // If reallocation requires movement of the buffer->data object, we are not able to zero the space for the previous
    // instantiation of the object. This is okay since zero-ing buffers is not assumed to be security critical at this
    // point.

    char* _Nullable bufferData = server->ip.storage->dynamicMemoryAllocation.reallocateMemory(buffer->data, numBytes);
    if (!bufferData) {
        HAPLog(&logObject,
               "session:%p:failed to reallocate %s (%zu)",
               (const void*) session,
               bufferDescription,
               numBytes);
        return kHAPError_OutOfResources;
    }

    if (numBytes > buffer->capacity) {
        HAPRawBufferZero(&bufferData[buffer->capacity], numBytes - buffer->capacity);
    }

    buffer->data = HAPNonnull(bufferData);
    buffer->capacity = numBytes;
    buffer->limit = HAPMin(buffer->limit, numBytes);
    buffer->position = HAPMin(buffer->position, numBytes);

    HAPLogDebug(
            &logObject, "session:%p:reallocated %s (%zu)", (const void*) session, bufferDescription, buffer->capacity);

    return kHAPError_None;
}

static void DeallocateBuffer(HAPIPSessionDescriptor* session, HAPIPByteBuffer* buffer, const char* bufferDescription) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPPrecondition((buffer == &session->inboundBuffer) || (buffer == &session->outboundBuffer));
    HAPPrecondition(bufferDescription);

    HAPPrecondition(buffer->data);
    HAPPrecondition(buffer->position <= buffer->limit);
    HAPPrecondition(buffer->limit <= buffer->capacity);

    HAPAccessoryServer* server = session->server;
    HAPPrecondition(server->ip.storage->dynamicMemoryAllocation.deallocateMemory);

    HAPRawBufferZero(buffer->data, buffer->capacity);

    server->ip.storage->dynamicMemoryAllocation.deallocateMemory(buffer->data);

    HAPRawBufferZero(buffer, sizeof *buffer);

    HAPLogDebug(&logObject, "session:%p:deallocated %s", (const void*) session, bufferDescription);

    HAPAssert(!buffer->data);
    HAPAssert(!buffer->capacity);
    HAPAssert(!buffer->limit);
    HAPAssert(!buffer->position);
}

HAP_RESULT_USE_CHECK
static HAPError AllocateInboundBuffer(HAPIPSessionDescriptor* session, size_t numBytes) {
    HAPPrecondition(session);

    return AllocateBuffer(session, &session->inboundBuffer, kHAPIPAccessoryServer_InboundBufferDescription, numBytes);
}

HAP_RESULT_USE_CHECK
static HAPError ReallocateInboundBuffer(HAPIPSessionDescriptor* session, size_t numBytes) {
    HAPPrecondition(session);

    return ReallocateBuffer(session, &session->inboundBuffer, kHAPIPAccessoryServer_InboundBufferDescription, numBytes);
}

static void DeallocateInboundBuffer(HAPIPSessionDescriptor* session) {
    HAPPrecondition(session);

    DeallocateBuffer(session, &session->inboundBuffer, kHAPIPAccessoryServer_InboundBufferDescription);
}

HAP_RESULT_USE_CHECK
static HAPError AllocateOutboundBuffer(HAPIPSessionDescriptor* session, size_t numBytes) {
    HAPPrecondition(session);

    return AllocateBuffer(session, &session->outboundBuffer, kHAPIPAccessoryServer_OutboundBufferDescription, numBytes);
}

HAP_RESULT_USE_CHECK
static HAPError ReallocateOutboundBuffer(HAPIPSessionDescriptor* session, size_t numBytes) {
    HAPPrecondition(session);

    return ReallocateBuffer(
            session, &session->outboundBuffer, kHAPIPAccessoryServer_OutboundBufferDescription, numBytes);
}

static void DeallocateOutboundBuffer(HAPIPSessionDescriptor* session) {
    HAPPrecondition(session);

    DeallocateBuffer(session, &session->outboundBuffer, kHAPIPAccessoryServer_OutboundBufferDescription);
}
#endif

static void RegisterSession(HAPIPSessionDescriptor* session) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPAccessoryServer* server = session->server;
    HAPPrecondition(server->ip.numSessions < server->ip.storage->numSessions);

    HAPError err;

    if (!server->ip.numSessions) {
        HAPLogDebug(&logObject, "Starting TCP progression timer.");

        HAPAssert(!server->ip.tcpProgressionTimer);
        err = HAPPlatformTimerRegister(
                &server->ip.tcpProgressionTimer,
                HAPPlatformClockGetCurrent() + kHAPIPAccessoryServer_TCPProgressionCheckInterval,
                HandleTCPProgressionTimerExpired,
                session->server);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            HAPLogError(&logObject, "Not enough resources to start TCP progression timer!");
            HAPFatalError();
        }
    }

    server->ip.numSessions++;
    if (server->ip.numSessions == server->ip.storage->numSessions) {
        schedule_max_idle_time_timer(session->server);
    }
}

static void handle_characteristic_unsubscribe_request(
        HAPIPSessionDescriptor* session,
        const HAPCharacteristic* chr,
        const HAPService* svc,
        const HAPAccessory* acc);

static void UnsubscribeEventNotifications(HAPIPSessionDescriptor* session) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);

    if (session->eventNotifications) {
        while (session->numEventNotifications) {
            HAPIPEventNotification* eventNotification =
                    &session->eventNotifications[session->numEventNotifications - 1];
            const HAPCharacteristic* characteristic;
            const HAPService* service;
            const HAPAccessory* accessory;
            get_db_ctx(
                    session->server,
                    eventNotification->aid,
                    eventNotification->iid,
                    &characteristic,
                    &service,
                    &accessory);
            if (eventNotification->flag) {
                HAPAssert(session->numEventNotificationFlags);
                session->numEventNotificationFlags--;
            }
            session->numEventNotifications--;
            handle_characteristic_unsubscribe_request(session, characteristic, service, accessory);
        }
        HAPAssert(!session->numEventNotifications);
        HAPAssert(!session->numEventNotificationFlags);
    } else {
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 0)
        HAPAssertionFailure();
#else
        if (session->eventNotificationSubscriptions) {
            HAPAssert(session->eventNotificationFlags);
            HAPAccessoryServer* server = session->server;
            if (!HAPBitSetIsEmpty(
                        HAPNonnull(session->eventNotificationSubscriptions),
                        server->ip.numEventNotificationBitSetBytes)) {
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
                                        if (HAPBitSetContains(
                                                    HAPNonnull(session->eventNotificationSubscriptions),
                                                    server->ip.numEventNotificationBitSetBytes,
                                                    eventNotificationIndex)) {
                                            HAPBitSetRemove(
                                                    HAPNonnull(session->eventNotificationSubscriptions),
                                                    server->ip.numEventNotificationBitSetBytes,
                                                    eventNotificationIndex);
                                            HAPBitSetRemove(
                                                    HAPNonnull(session->eventNotificationFlags),
                                                    server->ip.numEventNotificationBitSetBytes,
                                                    eventNotificationIndex);
                                            handle_characteristic_unsubscribe_request(
                                                    session, characteristic, service, accessory);
                                        }
                                        eventNotificationIndex++;
                                        HAPAssert(eventNotificationIndex);
                                    }
                                }
                            }
                        }
                    }
                    accessoryIndex++;
                    accessory =
                            server->ip.bridgedAccessories ? server->ip.bridgedAccessories[accessoryIndex - 1] : NULL;
                }
            }
            HAPAssert(HAPBitSetIsEmpty(
                    HAPNonnull(session->eventNotificationSubscriptions), server->ip.numEventNotificationBitSetBytes));
            HAPAssert(HAPBitSetIsEmpty(
                    HAPNonnull(session->eventNotificationFlags), server->ip.numEventNotificationBitSetBytes));
        } else {
            HAPAssert(!session->eventNotificationSubscriptions);
            HAPAssert(!session->eventNotificationFlags);
        }
#endif
    }
}

static void CloseSession(HAPIPSessionDescriptor* session) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPAccessoryServer* server = session->server;

    HAPAssert(session->state != kHAPIPSessionState_Idle);

    HAPError err;

    HAPLogDebug(&logObject, "session:%p:closing", (const void*) session);

    UnsubscribeEventNotifications(session);

#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 1)
    if (server->ip.storage->dynamicMemoryAllocation.deallocateMemory) {
        if (session->inboundBuffer.data) {
            DeallocateInboundBuffer(session);
        }
        HAPAssert(!session->inboundBuffer.data);
        HAPAssert(!session->inboundBuffer.position);
        HAPAssert(!session->inboundBuffer.limit);
        HAPAssert(!session->inboundBuffer.capacity);
        if (session->outboundBuffer.data) {
            DeallocateOutboundBuffer(session);
        }
        HAPAssert(!session->outboundBuffer.data);
        HAPAssert(!session->outboundBuffer.position);
        HAPAssert(!session->outboundBuffer.limit);
        HAPAssert(!session->outboundBuffer.capacity);
        bool deallocatedEventNotifications = false;
        if (session->eventNotificationSubscriptions) {
            server->ip.storage->dynamicMemoryAllocation.deallocateMemory(session->eventNotificationSubscriptions);
            session->eventNotificationSubscriptions = NULL;
            deallocatedEventNotifications = true;
        }
        if (session->eventNotificationFlags) {
            server->ip.storage->dynamicMemoryAllocation.deallocateMemory(session->eventNotificationFlags);
            session->eventNotificationFlags = NULL;
            deallocatedEventNotifications = true;
        }
        if (deallocatedEventNotifications) {
            HAPLogDebug(&logObject, "session:%p:deallocated event notifications", (const void*) session);
        }
        HAPAssert(!session->eventNotificationSubscriptions);
        HAPAssert(!session->eventNotificationFlags);
    }
#else
    // Clear inbound and outbound buffers for session
    HAPIPByteBufferClear(&session->outboundBuffer);
    session->outboundBufferMark = 0;
    HAPIPByteBufferClear(&session->inboundBuffer);
    session->inboundBufferMark = 0;
#endif
    if (session->securitySession.isOpen) {
        HAPLogDebug(&logObject, "session:%p:closing security context", (const void*) session);
        HAPLogDebug(&logObject, "Closing HAP session.");
        HAPSessionRelease(HAPNonnull(session->server), &session->securitySession._.hap);
        HAPRawBufferZero(&session->securitySession, sizeof session->securitySession);
        server->ip.gsnDidIncrement = false;
        HAPAssert(!session->securitySession.type);
        HAPAssert(!session->securitySession.isSecured);
        HAPAssert(!session->securitySession.isOpen);
    }
    if (session->tcpStreamIsOpen) {
        HAPLogDebug(&logObject, "session:%p:closing TCP stream", (const void*) session);
        HAPPlatformTCPStreamClose(HAPNonnull(server->platform.ip.tcpStreamManager), session->tcpStream);
        session->tcpStreamIsOpen = false;
    }
    if (session->cameraSnapshotReader) {
        HAPAssert(session->cameraSnapshotReader->close);
        HAPLogDebug(&logObject, "session:%p:closing camera snapshot reader.", (const void*) session);
        session->cameraSnapshotReader->close(HAPNonnull(session->cameraSnapshotReader));
        session->cameraSnapshotReader = NULL;
    }
    session->state = kHAPIPSessionState_Idle;

    if (!server->ip.garbageCollectionTimer) {
        err = HAPPlatformTimerRegister(
                &server->ip.garbageCollectionTimer, 0, handle_garbage_collection_timer, session->server);
        if (err) {
            HAPLog(&logObject, "Not enough resources to schedule garbage collection!");
            HAPFatalError();
        }
        HAPAssert(server->ip.garbageCollectionTimer);
    }

    HAPLogDebug(&logObject, "session:%p:closed", (const void*) session);
}

static void OpenSecuritySession(HAPIPSessionDescriptor* session) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPPrecondition(!session->securitySession.isOpen);
    HAPPrecondition(!session->securitySession.isSecured);

    HAPLogDebug(&logObject, "Opening HAP session.");
    session->securitySession.type = kHAPIPSecuritySessionType_HAP;
    HAPSessionCreate(HAPNonnull(session->server), &session->securitySession._.hap, kHAPTransportType_IP);
    session->securitySession.isOpen = true;
}

static void PrepareReadingRequest(HAPIPSessionDescriptor* session) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);

#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 1)
    HAPAccessoryServer* server = session->server;
    if (server->ip.storage->dynamicMemoryAllocation.deallocateMemory) {
        if (session->outboundBuffer.data) {
            DeallocateOutboundBuffer(session);
        }
    }
#endif

    util_http_reader_init(&session->httpReader, util_HTTP_READER_TYPE_REQUEST);
    session->httpReaderPosition = 0;
    session->httpParserError = false;
    session->httpMethodIsDefined = false;
    session->httpURIIsDefined = false;
    session->httpHeaderFieldNameIsDefined = false;
    session->httpHeaderFieldValueIsDefined = false;
    session->httpContentLengthIsDefined = false;
    session->httpContentType = kHAPIPAccessoryServerContentType_Unknown;
}

HAP_RESULT_USE_CHECK
static HAPError CompleteReadingRequest(HAPIPSessionDescriptor* session) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);

    HAPAssert(session->inboundBuffer.data);
    HAPAssert(session->inboundBuffer.position <= session->inboundBuffer.limit);
    HAPAssert(session->inboundBuffer.limit <= session->inboundBuffer.capacity);
    HAPAssert(session->httpReaderPosition <= session->inboundBuffer.position);
    HAPAssert(session->httpReader.state == util_HTTP_READER_STATE_DONE);
    HAPAssert(!session->httpParserError);

    size_t content_length;
    if (session->httpContentLengthIsDefined) {
        content_length = session->httpContentLength;
    } else {
        content_length = 0;
    }

    HAPIPByteBufferShiftLeft(&session->inboundBuffer, session->httpReaderPosition + content_length);

    session->inboundBufferMark = session->inboundBuffer.position;
    session->inboundBuffer.position = session->inboundBuffer.limit;
    session->inboundBuffer.limit = session->inboundBuffer.capacity;

#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 1)
    HAPAccessoryServer* server = session->server;
    if (session->inboundBuffer.position == 0) {
        if (server->ip.storage->dynamicMemoryAllocation.deallocateMemory) {
            DeallocateInboundBuffer(session);
        }
    } else {
        HAPAssert(session->inboundBufferMark <= session->inboundBuffer.position);
        HAPAssert(session->inboundBuffer.position < session->inboundBuffer.limit);
        HAPAssert(session->inboundBuffer.limit == session->inboundBuffer.capacity);
        if (server->ip.storage->dynamicMemoryAllocation.reallocateMemory) {
            size_t maxEncryptedFrameBytes = kHAPIPAccessoryServer_MaxEncryptedFrameBytes;
            HAPAssert(session->inboundBuffer.capacity >= maxEncryptedFrameBytes);
            if (session->inboundBufferMark < session->inboundBuffer.capacity - maxEncryptedFrameBytes) {
                HAPError err = ReallocateInboundBuffer(session, session->inboundBufferMark + maxEncryptedFrameBytes);
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }
            }
            HAPAssert(session->inboundBufferMark <= session->inboundBuffer.position);
            HAPAssert(session->inboundBuffer.position < session->inboundBuffer.limit);
            HAPAssert(session->inboundBuffer.limit == session->inboundBuffer.capacity);
        }
    }
#endif

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static HAPError PrepareWritingResponse(HAPIPSessionDescriptor* session, size_t maxResponseBytes) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPPrecondition(maxResponseBytes >= kHAPIPAccessoryServer_MinOutboundBufferBytes);

#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 0)
    HAPAssert(session->outboundBuffer.data);
    HAPAssert(session->outboundBuffer.position <= session->outboundBuffer.limit);
    HAPAssert(session->outboundBuffer.limit <= session->outboundBuffer.capacity);
#else
    HAPAccessoryServer* server = session->server;
    if (!server->ip.storage->dynamicMemoryAllocation.allocateMemory) {
        HAPAssert(session->outboundBuffer.data);
        HAPAssert(session->outboundBuffer.position <= session->outboundBuffer.limit);
        HAPAssert(session->outboundBuffer.limit <= session->outboundBuffer.capacity);
    } else {
        HAPAssert(!session->outboundBuffer.data);
        HAPAssert(!session->outboundBuffer.position);
        HAPAssert(!session->outboundBuffer.limit);
        HAPAssert(!session->outboundBuffer.capacity);
        HAPError err = AllocateOutboundBuffer(session, HAPIPSecurityProtocolGetNumEncryptedBytes(maxResponseBytes));
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
        HAPAssert(session->outboundBuffer.data);
        HAPAssert(session->outboundBuffer.position <= session->outboundBuffer.limit);
        HAPAssert(session->outboundBuffer.limit <= session->outboundBuffer.capacity);
    }
#endif

    return kHAPError_None;
}

static void WriteResponse(HAPIPSessionDescriptor* session, const char* response) {
    HAPPrecondition(session);
    HAPPrecondition(response);

    HAPError err;

    err = CompleteReadingRequest(session);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        CloseSession(session);
        return;
    }
    err = PrepareWritingResponse(session, kHAPIPAccessoryServer_MinOutboundBufferBytes);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        CloseSession(session);
        return;
    }
    err = HAPIPByteBufferAppendStringWithFormat(&session->outboundBuffer, "%s", response);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPAssertionFailure();
    }
}

static void handle_input(HAPIPSessionDescriptor* session);

static void handle_ip_camera_snapshot(HAPIPSessionDescriptor* session HAP_UNUSED) {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPPrecondition(session->securitySession.type == kHAPIPSecuritySessionType_HAP);
    HAPPrecondition(session->securitySession.isOpen);

    HAPError err;

    size_t n;
    size_t outbound_chunk_size, encrypted_length;

    HAPAssert(session->cameraSnapshotReader);
    HAPAssert(session->cameraSnapshotReader->read);
    HAPAssert(session->cameraSnapshotReader->close);
    HAPPrecondition(session->outboundBuffer.data);
    HAPPrecondition(session->outboundBuffer.position <= session->outboundBuffer.limit);
    HAPPrecondition(session->outboundBuffer.limit <= session->outboundBuffer.capacity);
    outbound_chunk_size = (session->outboundBuffer.limit - session->outboundBuffer.position) / 2;
    if (outbound_chunk_size == 0) {
        HAPLogError(&logObject, "Invalid configuration (outbound buffer too small).");
        HAPFatalError();
    }
    err = session->cameraSnapshotReader->read(
            HAPNonnull(session->cameraSnapshotReader),
            &session->outboundBuffer.data[session->outboundBuffer.position],
            outbound_chunk_size,
            &n);
    if (!err && n > 0) {
        if (session->numCameraSnapshotBytesToSerialize < n) {
            HAPLogError(
                    &logObject,
                    "session:%p:inconsistency while writing camera snapshot. "
                    "Number of bytes to write: %zu, number of bytes read from snapshot reader: %zu.",
                    (const void*) session,
                    session->numCameraSnapshotBytesToSerialize,
                    n);
            HAPFatalError();
        }
        session->numCameraSnapshotBytesToSerialize -= n;
        HAPAssert(n <= outbound_chunk_size);
        session->outboundBuffer.position += n;
        HAPIPByteBufferFlip(&session->outboundBuffer);
        HAPLogDebug(
                &logObject,
                "session:%p:writing camera snapshot chunk: %zu.",
                (const void*) session,
                session->outboundBuffer.limit);
        if (session->securitySession.isSecured) {
            encrypted_length = HAPIPSecurityProtocolGetNumEncryptedBytes(
                    session->outboundBuffer.limit - session->outboundBuffer.position);
            if (encrypted_length > session->outboundBuffer.capacity - session->outboundBuffer.position) {
                HAPLogError(&logObject, "Invalid configuration (outbound buffer too small).");
                HAPFatalError();
            }
            HAPIPSecurityProtocolEncryptData(
                    HAPNonnull(session->server), &session->securitySession._.hap, &session->outboundBuffer);
            HAPAssert(encrypted_length == session->outboundBuffer.limit - session->outboundBuffer.position);
        }
        HAPAssert(session->state == kHAPIPSessionState_Writing);
    } else {
        if (err) {
            HAPLogError(&logObject, "session:%p:reading camera snapshot failed.", (const void*) session);
        } else {
            if (session->numCameraSnapshotBytesToSerialize > 0) {
                HAPLogError(
                        &logObject,
                        "session:%p:inconsistency while writing camera snapshot. "
                        "Number of bytes to write: %zu, number of bytes read from snapshot reader: %zu.",
                        (const void*) session,
                        session->numCameraSnapshotBytesToSerialize,
                        n);
                HAPFatalError();
            }
        }
        HAPLogDebug(&logObject, "session:%p:closing camera snapshot reader.", (const void*) session);
        session->cameraSnapshotReader->close(HAPNonnull(session->cameraSnapshotReader));
        session->cameraSnapshotReader = NULL;
        if (err) {
            CloseSession(session);
        } else {
            HAPAssert(n == 0);
            session->state = kHAPIPSessionState_Reading;
            PrepareReadingRequest(session);
            if (session->inboundBuffer.position != 0) {
                handle_input(session);
            }
        }
    }
#endif
}

static void post_resource(HAPIPSessionDescriptor* session HAP_UNUSED) {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPAccessoryServer* server = session->server;
    HAPPrecondition(session->securitySession.type == kHAPIPSecuritySessionType_HAP);
    HAPPrecondition(session->securitySession.isOpen);
    HAPPrecondition(session->securitySession.isSecured || kHAPIPAccessoryServer_SessionSecurityDisabled);
    HAPPrecondition(!HAPSessionIsTransient(&session->securitySession._.hap));

    HAPError err;

    HAPAssert(session->inboundBuffer.data);
    HAPAssert(session->inboundBuffer.position <= session->inboundBuffer.limit);
    HAPAssert(session->inboundBuffer.limit <= session->inboundBuffer.capacity);
    HAPAssert(session->httpReaderPosition <= session->inboundBuffer.position);

    if (!session->httpContentLengthIsDefined) {
        WriteResponse(session, kHAPIPAccessoryServerResponse_BadRequest);
        return;
    }
    HAPAssert(session->httpContentLength <= session->inboundBuffer.position - session->httpReaderPosition);

    uint64_t aid;
    HAPIPCameraSnapshotReason snapshotReason;
    const HAPAccessory* _Nullable accessory = NULL;
    uint64_t image_width, image_height;
    err = HAPIPAccessoryProtocolGetCameraSnapshotRequest(
            &session->inboundBuffer.data[session->httpReaderPosition],
            session->httpContentLength,
            &aid,
            &snapshotReason,
            &image_width,
            &image_height);
    if (err || (image_width > UINT16_MAX) || (image_height > UINT16_MAX)) {
        WriteResponse(session, kHAPIPAccessoryServerResponse_BadRequest);
        return;
    }

    HAPPlatformCameraRef camera;
    camera = NULL;
    if (aid == kHAPIPAccessoryProtocolAID_PrimaryAccessory) {
        camera = server->platform.ip.camera;
        accessory = server->primaryAccessory;
    } else {
        if (server->ip.bridgedCameras && server->ip.bridgedAccessories) {
            const HAPPlatformCameraRef _Nullable* cameras = server->ip.bridgedCameras;
            for (size_t i = 0; server->ip.bridgedAccessories[i]; i++) {
                if (server->ip.bridgedAccessories[i]->aid == aid) {
                    camera = cameras[i];
                    accessory = server->ip.bridgedAccessories[i];
                    break;
                }
            }
        }
    }
    if (!camera) {
        HAPLog(&logObject, "Camera not set in platform.");
        WriteResponse(session, kHAPIPAccessoryServerResponse_ResourceNotFound);
        return;
    }

    if (!HAPCameraAreSnapshotsEnabled(camera, HAPNonnull(accessory), snapshotReason)) {
        WriteResponse(session, kHAPIPAccessoryServerResponse_NotAllowedInCurrentState);
        return;
    }

    HAPLogDebug(&logObject, "Camera snapshot request - aid = %llu.", (unsigned long long) aid);
    HAPAssert(!session->cameraSnapshotReader);
    err = HAPPlatformCameraTakeSnapshot(
            HAPNonnull(camera), (uint16_t) image_width, (uint16_t) image_height, &session->cameraSnapshotReader);
    if (err) {
        HAPLogError(&logObject, "session:%p:taking camera snapshot failed.", (const void*) session);
        session->cameraSnapshotReader = NULL;
        WriteResponse(session, kHAPIPAccessoryServerResponse_ResourceNotFound);
        return;
    }
    HAPAssert(session->cameraSnapshotReader);
    HAPAssert(session->cameraSnapshotReader->getSize);
    HAPAssert(session->cameraSnapshotReader->close);

    // Check for invalid camera snapshot reader reuse.
    for (size_t i = 0; i < server->ip.storage->numSessions; i++) {
        HAPIPSession* otherIPSession = &server->ip.storage->sessions[i];
        HAPIPSessionDescriptor* otherSession = &otherIPSession->descriptor;
        if (otherSession->server && otherSession != session &&
            otherSession->cameraSnapshotReader == session->cameraSnapshotReader) {
            HAPLogError(
                    &logObject,
                    "session:%p:camera snapshot reader is already in use on session %p.",
                    (const void*) session,
                    (const void*) otherSession);
            HAPFatalError();
        }
    }

    err = session->cameraSnapshotReader->getSize(
            HAPNonnull(session->cameraSnapshotReader), &session->numCameraSnapshotBytesToSerialize);
    if (err) {
        HAPLogError(&logObject, "session:%p:getting camera snapshot size failed.", (const void*) session);
        HAPLogDebug(&logObject, "session:%p:closing camera snapshot reader.", (const void*) session);
        session->cameraSnapshotReader->close(HAPNonnull(session->cameraSnapshotReader));
        session->cameraSnapshotReader = NULL;
        WriteResponse(session, kHAPIPAccessoryServerResponse_ResourceNotFound);
        return;
    }

    err = CompleteReadingRequest(session);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        CloseSession(session);
        return;
    }
    err = PrepareWritingResponse(session, kHAPIPSecurityProtocol_MaxFrameBytes);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPLogDebug(&logObject, "session:%p:closing camera snapshot reader.", (const void*) session);
        session->cameraSnapshotReader->close(HAPNonnull(session->cameraSnapshotReader));
        session->cameraSnapshotReader = NULL;
        err = PrepareWritingResponse(session, kHAPIPAccessoryServer_MinOutboundBufferBytes);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            CloseSession(session);
            return;
        }
        err = HAPIPByteBufferAppendStringWithFormat(
                &session->outboundBuffer, "%s", kHAPIPAccessoryServerResponse_OutOfResources);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            HAPAssertionFailure();
        }
        return;
    }
    err = HAPIPByteBufferAppendStringWithFormat(
            &session->outboundBuffer,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: image/jpeg\r\n"
            "Content-Length: %zu\r\n\r\n",
            session->numCameraSnapshotBytesToSerialize);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPAssertionFailure();
    }
#endif
}

static void put_prepare(HAPIPSessionDescriptor* session) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPPrecondition(session->securitySession.type == kHAPIPSecuritySessionType_HAP);
    HAPPrecondition(session->securitySession.isOpen);
    HAPPrecondition(session->securitySession.isSecured || kHAPIPAccessoryServer_SessionSecurityDisabled);
    HAPPrecondition(!HAPSessionIsTransient(&session->securitySession._.hap));

    HAPError err;
    uint64_t ttl, pid;

    HAPAssert(session->inboundBuffer.data);
    HAPAssert(session->inboundBuffer.position <= session->inboundBuffer.limit);
    HAPAssert(session->inboundBuffer.limit <= session->inboundBuffer.capacity);
    HAPAssert(session->httpReaderPosition <= session->inboundBuffer.position);
    if (session->httpContentLengthIsDefined) {
        HAPAssert(session->httpContentLength <= session->inboundBuffer.position - session->httpReaderPosition);
        err = HAPIPAccessoryProtocolGetCharacteristicWritePreparation(
                &session->inboundBuffer.data[session->httpReaderPosition], session->httpContentLength, &ttl, &pid);
        if (!err) {
            HAPLogDebug(&logObject, "Prepare Write Request - TTL = %lu ms.", (unsigned long) ttl);

            // If the accessory receives consecutive Prepare Write Requests in the same session, the accessory must
            // reset the timed write transaction with the TTL specified by the latest request.
            // See HomeKit Accessory Protocol Specification R17
            // Section 6.7.2.4 Timed Write Procedures
            // Assumption: Same behavior for PID.

            // TTL.
            HAPTime clock_now_ms = HAPPlatformClockGetCurrent();
            if (UINT64_MAX - clock_now_ms < ttl) {
                HAPLog(&logObject, "Clipping TTL to avoid clock overflow.");
                session->timedWriteExpirationTime = UINT64_MAX;
            } else {
                session->timedWriteExpirationTime = clock_now_ms + ttl;
            }
            HAPAssert(session->timedWriteExpirationTime >= clock_now_ms);

            // PID.
            session->timedWritePID = pid;

            // The accessory must respond with a 200 OK HTTP status code and include a HAP status code indicating if
            // timed write procedure can be executed or not.
            // See HomeKit Accessory Protocol Specification R17
            // Section 6.7.2.4 Timed Write Procedures
            // It is not documented under what conditions this should fail.
            err = CompleteReadingRequest(session);
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                CloseSession(session);
                return;
            }
            err = PrepareWritingResponse(session, kHAPIPAccessoryServer_MinOutboundBufferBytes);
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                CloseSession(session);
                return;
            }
            err = HAPIPByteBufferAppendStringWithFormat(
                    &session->outboundBuffer,
                    "%s",
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: application/hap+json\r\n"
                    "Content-Length: 12\r\n\r\n"
                    "{\"status\":0}");
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                HAPAssertionFailure();
            }
        } else {
            WriteResponse(session, kHAPIPAccessoryServerResponse_BadRequest);
        }
    } else {
        WriteResponse(session, kHAPIPAccessoryServerResponse_BadRequest);
    }
}

static void write_characteristic_write_response(
        HAPIPSessionDescriptor* session,
        HAPIPWriteContext* contexts,
        size_t contexts_count) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPPrecondition(session->securitySession.type == kHAPIPSecuritySessionType_HAP);
    HAPPrecondition(session->securitySession.isOpen);
    HAPPrecondition(session->securitySession.isSecured || kHAPIPAccessoryServer_SessionSecurityDisabled);
    HAPPrecondition(!HAPSessionIsTransient(&session->securitySession._.hap));

    HAPError err;
    size_t content_length, mark;

    HAPAssert(contexts);

    content_length = HAPIPAccessoryProtocolGetNumCharacteristicWriteResponseBytes(
            HAPNonnull(session->server), contexts, contexts_count);

    HAP_DIAGNOSTIC_IGNORED_ICCARM(Pa084)
    if (content_length <= UINT32_MAX) {
        err = CompleteReadingRequest(session);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            CloseSession(session);
            return;
        }
        err = PrepareWritingResponse(session, kHAPIPAccessoryServer_MinOutboundBufferBytes + content_length);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            err = PrepareWritingResponse(session, kHAPIPAccessoryServer_MinOutboundBufferBytes);
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                CloseSession(session);
                return;
            }
            err = HAPIPByteBufferAppendStringWithFormat(
                    &session->outboundBuffer, "%s", kHAPIPAccessoryServerResponse_OutOfResources);
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                HAPAssertionFailure();
            }
            return;
        }
        mark = session->outboundBuffer.position;
        err = HAPIPByteBufferAppendStringWithFormat(
                &session->outboundBuffer,
                "HTTP/1.1 207 Multi-Status\r\n"
                "Content-Type: application/hap+json\r\n"
                "Content-Length: %lu\r\n\r\n",
                (unsigned long) content_length);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            HAPAssertionFailure();
        }
        if (content_length <= session->outboundBuffer.limit - session->outboundBuffer.position) {
            mark = session->outboundBuffer.position;
            err = HAPIPAccessoryProtocolGetCharacteristicWriteResponseBytes(
                    HAPNonnull(session->server), contexts, contexts_count, &session->outboundBuffer);
            HAPAssert(!err && (session->outboundBuffer.position - mark == content_length));
        } else {
            HAPLog(&logObject, "Out of resources (outbound buffer too small).");
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 1)
            HAPAccessoryServer* server = session->server;
            if (server->ip.storage->dynamicMemoryAllocation.allocateMemory) {
                HAPFatalError();
            }
#endif
            session->outboundBuffer.position = mark;
            err = HAPIPByteBufferAppendStringWithFormat(
                    &session->outboundBuffer, "%s", kHAPIPAccessoryServerResponse_OutOfResources);
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                HAPAssertionFailure();
            }
        }
    } else {
        HAPLog(&logObject, "Content length exceeding UINT32_MAX.");
        WriteResponse(session, kHAPIPAccessoryServerResponse_OutOfResources);
    }
    HAP_DIAGNOSTIC_RESTORE_ICCARM(Pa084)
}

static void handle_event_notification_timer(HAPPlatformTimerRef timer, void* _Nullable context) {
    HAPPrecondition(context);
    HAPAccessoryServer* server = context;
    if (server->ip.eventNotificationTimer == 0 || timer != server->ip.eventNotificationTimer) {
        // Note that this could legitimately happen due to race condition between
        // schedule_event_notifications() called by handle_io_progression()
        // and the timer triggering.
        // That is, even if HAPPlatformTimerDeregister() is called,
        // the timer event might have been already queued and the timer handler
        // is called regardless,
        // and call to schedule_event_notifications() by handle_io_progression()
        // creates such a race condition.
        return;
    }
    server->ip.eventNotificationTimer = 0;

    schedule_event_notifications(server);
}

static void write_event_notifications(HAPIPSessionDescriptor* session);

static void schedule_event_notifications(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    HAPError err;

    HAPLogDebug(&logObject, "Attempting to send event notifications.");

    if (server->ip.eventNotificationTimer) {
        HAPPlatformTimerDeregister(server->ip.eventNotificationTimer);
        server->ip.eventNotificationTimer = 0;
    }

    for (size_t i = 0; i < server->ip.storage->numSessions; i++) {
        HAPIPSession* ipSession = &server->ip.storage->sessions[i];
        HAPIPSessionDescriptor* session = &ipSession->descriptor;
        if (!session->server) {
            continue;
        }

        if (session->securitySession.isSecured && HAPSessionKeyExpired(&session->securitySession._.hap)) {
            // Close secure session if its key has expired
            CloseSession(session);
            continue;
        }

        if ((session->state == kHAPIPSessionState_Reading) && (session->inboundBuffer.position == 0) &&
            SessionHasPendingEventNotifications(session)) {
            write_event_notifications(session);
        }
    }

    HAPTime clock_now_ms = HAPPlatformClockGetCurrent();
    int64_t timeout_ms = -1;

    for (size_t i = 0; i < server->ip.storage->numSessions; i++) {
        HAPIPSession* ipSession = &server->ip.storage->sessions[i];
        HAPIPSessionDescriptor* session = &ipSession->descriptor;
        if (!session->server) {
            continue;
        }

        if ((session->state == kHAPIPSessionState_Reading) && (session->inboundBuffer.position == 0) &&
            SessionHasPendingEventNotifications(session)) {
            HAPAssert(clock_now_ms >= session->eventNotificationStamp);
            HAPTime dt_ms = clock_now_ms - session->eventNotificationStamp;
            HAP_DIAGNOSTIC_PUSH
            HAP_DIAGNOSTIC_IGNORED_ARMCC(186)
            HAP_DIAGNOSTIC_IGNORED_GCC("-Wtype-limits")
            if (dt_ms < kHAPIPAccessoryServer_MaxEventNotificationDelay) {
                HAPAssert(kHAPIPAccessoryServer_MaxEventNotificationDelay <= INT64_MAX);
                int64_t t_ms = (int64_t)(kHAPIPAccessoryServer_MaxEventNotificationDelay - dt_ms);
                if ((timeout_ms == -1) || (t_ms < timeout_ms)) {
                    timeout_ms = t_ms;
                }
            } else {
                timeout_ms = 0;
            }
            HAP_DIAGNOSTIC_POP
        }
    }

    // Network-based notifications must be coalesced by the accessory using a delay of no less than 1 second.
    // See HomeKit Accessory Protocol Specification R17
    // Section 6.8 Notifications
    if (timeout_ms >= 0) {
        HAPTime deadline_ms;

        if (UINT64_MAX - clock_now_ms < (HAPTime) timeout_ms) {
            HAPLog(&logObject, "Clipping event notification timer to avoid clock overflow.");
            deadline_ms = UINT64_MAX;
        } else {
            deadline_ms = clock_now_ms + (HAPTime) timeout_ms;
        }
        HAPAssert(deadline_ms >= clock_now_ms);

        HAPLogDebug(
                &logObject,
                "Delaying sending of further events by %llu.%03llus (event notification coalescing).",
                (unsigned long long) ((HAPTime) timeout_ms / HAPSecond),
                (unsigned long long) ((HAPTime) timeout_ms % HAPSecond));

        err = HAPPlatformTimerRegister(
                &server->ip.eventNotificationTimer, deadline_ms, handle_event_notification_timer, server);
        if (err) {
            HAPLog(&logObject, "Not enough resources to schedule event notification timer!");
            HAPFatalError();
        }
        HAPAssert(server->ip.eventNotificationTimer);
    }
}

static void handle_characteristic_subscribe_request(
        HAPIPSessionDescriptor* session,
        const HAPCharacteristic* chr,
        const HAPService* svc,
        const HAPAccessory* acc) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPPrecondition(session->securitySession.type == kHAPIPSecuritySessionType_HAP);
    HAPPrecondition(session->securitySession.isOpen);
    HAPPrecondition(session->securitySession.isSecured || kHAPIPAccessoryServer_SessionSecurityDisabled);
    HAPPrecondition(!HAPSessionIsTransient(&session->securitySession._.hap));
    HAPPrecondition(chr);
    HAPPrecondition(svc);
    HAPPrecondition(acc);

    HAPAccessoryServerHandleSubscribe(HAPNonnull(session->server), &session->securitySession._.hap, chr, svc, acc);
}

static void handle_characteristic_unsubscribe_request(
        HAPIPSessionDescriptor* session,
        const HAPCharacteristic* chr,
        const HAPService* svc,
        const HAPAccessory* acc) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPPrecondition(session->securitySession.type == kHAPIPSecuritySessionType_HAP);
    HAPPrecondition(session->securitySession.isOpen);
    HAPPrecondition(session->securitySession.isSecured || kHAPIPAccessoryServer_SessionSecurityDisabled);
    HAPPrecondition(!HAPSessionIsTransient(&session->securitySession._.hap));
    HAPPrecondition(chr);
    HAPPrecondition(svc);
    HAPPrecondition(acc);

    HAPAccessoryServerHandleUnsubscribe(HAPNonnull(session->server), &session->securitySession._.hap, chr, svc, acc);
}

static void handle_characteristic_read_request(
        HAPIPSessionDescriptor* session,
        const HAPCharacteristic* chr,
        const HAPService* svc,
        const HAPAccessory* acc,
        HAPIPReadContext* ctx,
        HAPIPByteBuffer* data_buffer);

/**
 * Converts a characteristic write request error to the corresponding HAP status code.
 *
 * @param      error                Write request error.
 *
 * @return HAP write request status code.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Table 6-11 HAP Status Codes
 */
HAP_RESULT_USE_CHECK
static int32_t ConvertCharacteristicWriteErrorToStatusCode(HAPError error) {
    switch (error) {
        case kHAPError_None:
            return kHAPIPAccessoryServerStatusCode_Success;
        case kHAPError_Unknown:
            return kHAPIPAccessoryServerStatusCode_UnableToPerformOperation;
        case kHAPError_InvalidState:
            return kHAPIPAccessoryServerStatusCode_NotAllowedInCurrentState;
        case kHAPError_InvalidData:
            return kHAPIPAccessoryServerStatusCode_InvalidValueInWrite;
        case kHAPError_OutOfResources:
            return kHAPIPAccessoryServerStatusCode_OutOfResources;
        case kHAPError_NotAuthorized:
            return kHAPIPAccessoryServerStatusCode_InsufficientAuthorization;
        case kHAPError_Busy:
            return kHAPIPAccessoryServerStatusCode_ResourceIsBusy;
        default:
            HAPFatalError();
    }
}

/**
 * Parses and processes a subscription state change request from a write request.
 *
 * 1. Checks if controller has needed permissions (if any are needed)
 * 2. Checks if characteristic supports event notifications
 * 3. Will subscribe/unsubscribe the session to/from event notifications generated from the
 * accessory/characteristic. Different code paths are use depending on whether or not memory is already
 * allocated for session event notifications.
 *
 * @param session           The session which may be requesting a subscription state change.
 * @param characteristic    The characteristic which generates the event notifications to
 *                          subscribe/unsubscribe to.
 * @param service           The service that contains the characteristic.
 * @param accessory         he accessory that provides the service.
 * @param writeContext      The context of the write request.
 */
static void handle_characteristic_write_request_process_subscription_change(
        HAPIPSessionDescriptor* session,
        const HAPCharacteristic* characteristic,
        const HAPService* service,
        const HAPAccessory* accessory,
        HAPIPWriteContext* writeContext) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPPrecondition(session->securitySession.type == kHAPIPSecuritySessionType_HAP);
    HAPPrecondition(session->securitySession.isOpen);
    HAPPrecondition(session->securitySession.isSecured || kHAPIPAccessoryServer_SessionSecurityDisabled);
    HAPPrecondition(!HAPSessionIsTransient(&session->securitySession._.hap));
    HAPPrecondition(characteristic);
    HAPPrecondition(service);
    HAPPrecondition(accessory);
    HAPPrecondition(writeContext);

    if (writeContext->ev == kHAPIPEventNotificationState_Undefined) {
        // If the context is not requesting an event notification state change, return.
        return;
    }

    const HAPBaseCharacteristic* baseCharacteristic = characteristic;

    if (HAPCharacteristicReadRequiresAdminPermissions(baseCharacteristic) &&
        !HAPSessionControllerIsAdmin(&session->securitySession._.hap)) {
        writeContext->status = kHAPIPAccessoryServerStatusCode_InsufficientPrivileges;
        return;
    } else if (!baseCharacteristic->properties.supportsEventNotification) {
        writeContext->status = kHAPIPAccessoryServerStatusCode_NotificationNotSupported;
        return;
    }

    writeContext->status = kHAPIPAccessoryServerStatusCode_Success;
    if (session->eventNotifications) {
        // The session has memory allocated for registered event notifications.

        // Sanity check. Verify current session has a valid number of registered event notifications before proceeding.
        HAPAssert(session->numEventNotifications <= session->maxEventNotifications);

        // Is the current session already subscribed for this event notification? If so, find it's index in the array.
        size_t eventNotificationIndex = 0;
        while ((eventNotificationIndex < session->numEventNotifications) &&
               ((session->eventNotifications[eventNotificationIndex].aid != writeContext->aid) ||
                (session->eventNotifications[eventNotificationIndex].iid != writeContext->iid))) {
            eventNotificationIndex++;
        }

        // Sanity check. One of these two things must be true:
        // 1) The session is not subscribed for this event notification (the index is equal to the current number of
        // session event notifications) or 2) The session is subscribed for this event notification, and the event
        // notification found matches the write context's accessory/characteristic ids.
        HAPAssert(
                (eventNotificationIndex == session->numEventNotifications) ||
                ((eventNotificationIndex < session->numEventNotifications) &&
                 (session->eventNotifications[eventNotificationIndex].aid == writeContext->aid) &&
                 (session->eventNotifications[eventNotificationIndex].iid == writeContext->iid)));

        if (eventNotificationIndex == session->numEventNotifications) {
            // The session is not subscribed for this event notification.
            if (writeContext->ev == kHAPIPEventNotificationState_Enabled) {
                // Request is to subscribe this session to event notifications for the accessory/characteristic.
                if (eventNotificationIndex == session->maxEventNotifications) {
                    writeContext->status = kHAPIPAccessoryServerStatusCode_OutOfResources;
                } else {
                    session->eventNotifications[eventNotificationIndex].aid = writeContext->aid;
                    session->eventNotifications[eventNotificationIndex].iid = writeContext->iid;
                    session->eventNotifications[eventNotificationIndex].flag = false;
                    session->numEventNotifications++;
                    handle_characteristic_subscribe_request(session, characteristic, service, accessory);
                }
            }
        } else {
            // The session is already subscribed for this event notification.
            if (writeContext->ev == kHAPIPEventNotificationState_Disabled) {
                // Request is to unsubscribe this session from event notifications for the accessory/characteristic.
                session->numEventNotifications--;
                if (session->eventNotifications[eventNotificationIndex].flag) {
                    HAPAssert(session->numEventNotificationFlags > 0);
                    session->numEventNotificationFlags--;
                }
                // Move the array to the left, to overwrite the event notification we are unsubscribing from,
                // and keep the array contiguous.
                while (eventNotificationIndex < session->numEventNotifications) {
                    HAPRawBufferCopyBytes(
                            &session->eventNotifications[eventNotificationIndex],
                            &session->eventNotifications[eventNotificationIndex + 1],
                            sizeof session->eventNotifications[eventNotificationIndex]);
                    eventNotificationIndex++;
                }
                HAPAssert(eventNotificationIndex == session->numEventNotifications);
                handle_characteristic_unsubscribe_request(session, characteristic, service, accessory);
            }
        }
    } else {
        // The session does not have memory allocated for event notifications.
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 0)
        HAPAssertionFailure();
#else
        HAPAssert(session->eventNotificationSubscriptions);
        HAPAssert(session->eventNotificationFlags);
        HAPAccessoryServer* server = session->server;
        size_t eventNotificationIndex;
        bool eventNotificationIndexFound;
        HAPAccessoryServerGetEventNotificationIndex(
                HAPNonnull(session->server),
                writeContext->aid,
                writeContext->iid,
                &eventNotificationIndex,
                &eventNotificationIndexFound);
        HAPAssert(eventNotificationIndexFound);
        if (writeContext->ev == kHAPIPEventNotificationState_Enabled &&
            !HAPBitSetContains(
                    HAPNonnull(session->eventNotificationSubscriptions),
                    server->ip.numEventNotificationBitSetBytes,
                    eventNotificationIndex)) {
            HAPBitSetInsert(
                    HAPNonnull(session->eventNotificationSubscriptions),
                    server->ip.numEventNotificationBitSetBytes,
                    eventNotificationIndex);
            HAPAssert(!HAPBitSetContains(
                    HAPNonnull(session->eventNotificationFlags),
                    server->ip.numEventNotificationBitSetBytes,
                    eventNotificationIndex));
            handle_characteristic_subscribe_request(session, characteristic, service, accessory);
        } else if (
                writeContext->ev == kHAPIPEventNotificationState_Disabled &&
                HAPBitSetContains(
                        HAPNonnull(session->eventNotificationSubscriptions),
                        server->ip.numEventNotificationBitSetBytes,
                        eventNotificationIndex)) {
            HAPBitSetRemove(
                    HAPNonnull(session->eventNotificationSubscriptions),
                    server->ip.numEventNotificationBitSetBytes,
                    eventNotificationIndex);
            HAPBitSetRemove(
                    HAPNonnull(session->eventNotificationFlags),
                    server->ip.numEventNotificationBitSetBytes,
                    eventNotificationIndex);
            handle_characteristic_unsubscribe_request(session, characteristic, service, accessory);
        } else {
            // Characteristic subscription status did not change.
        }
#endif
    }
}

/**
 * Parses and processes a request to write a value to a characteristic.
 *
 * 1. Verify that both the controller and characteristic have the correct permissions.
 * 2. Parse Authorization data (if present)
 * 3. Write the new value to the characteristic.
 * 4. Exit if there was an error.
 * 5. If a write response is expected, read updated value into the writeContext.
 *
 * @param session           The session which may be requesting a value be written to a characteritic.
 * @param characteristic    The characteristic which will hold the new value.
 * @param service           The service that contains the characteristic.
 * @param accessory         he accessory that provides the service.
 * @param writeContext      The context of the write request.
 * @param dataBuffer        Buffer for values of type data, string or TLV8.
 */
static void handle_characteristic_write_request_process_write(
        HAPIPSessionDescriptor* session,
        const HAPCharacteristic* characteristic,
        const HAPService* service,
        const HAPAccessory* accessory,
        HAPIPWriteContext* writeContext,
        HAPIPByteBuffer* dataBuffer) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPPrecondition(session->securitySession.type == kHAPIPSecuritySessionType_HAP);
    HAPPrecondition(session->securitySession.isOpen);
    HAPPrecondition(session->securitySession.isSecured || kHAPIPAccessoryServer_SessionSecurityDisabled);
    HAPPrecondition(!HAPSessionIsTransient(&session->securitySession._.hap));
    HAPPrecondition(characteristic);
    HAPPrecondition(service);
    HAPPrecondition(accessory);
    HAPPrecondition(writeContext);
    HAPPrecondition(dataBuffer);

    const HAPBaseCharacteristic* baseCharacteristic = characteristic;

    if (writeContext->type == kHAPIPWriteValueType_None) {
        // If the context is not requesting anything to be written, return.
        return;
    }
    if (HAPCharacteristicWriteRequiresAdminPermissions(baseCharacteristic) &&
        !HAPSessionControllerIsAdmin(&session->securitySession._.hap)) {
        // The characteristic requires a controller to have admin permissions to write,
        // and this controller is not admin.
        writeContext->status = kHAPIPAccessoryServerStatusCode_InsufficientPrivileges;
        return;
    }
    if ((baseCharacteristic->properties.ip.supportsWriteResponse || writeContext->response) &&
        HAPCharacteristicReadRequiresAdminPermissions(baseCharacteristic) &&
        !HAPSessionControllerIsAdmin(&session->securitySession._.hap)) {
        // The characteristic requires a controller to have admin permissions to read,
        // and this controller is not admin. Read is needed when sending a write response,
        // since we read from the characteristic to get the data.
        writeContext->status = kHAPIPAccessoryServerStatusCode_InsufficientPrivileges;
        return;
    }
    if (!baseCharacteristic->properties.writable) {
        // Characteristic is not writeable.
        writeContext->status = kHAPIPAccessoryServerStatusCode_WriteToReadOnlyCharacteristic;
        return;
    }

    HAPError err;
    // Assume success. We will update this as we run into errors, and will occasionally be checking it
    // to see if we can continue to the next step.
    writeContext->status = kHAPIPAccessoryServerStatusCode_Success;

    const void* authorizationDataBytes = NULL;
    size_t numAuthorizationDataBytes = 0;
    if (writeContext->authorizationData.bytes) {
        err = util_base64_decode(
                writeContext->authorizationData.bytes,
                writeContext->authorizationData.numBytes,
                writeContext->authorizationData.bytes,
                writeContext->authorizationData.numBytes,
                &writeContext->authorizationData.numBytes);
        if (err == kHAPError_None) {
            authorizationDataBytes = writeContext->authorizationData.bytes;
            numAuthorizationDataBytes = writeContext->authorizationData.numBytes;
        } else {
            writeContext->status = kHAPIPAccessoryServerStatusCode_InvalidValueInWrite;
            return;
        }
    }
    if (writeContext->contextData.numBytes) {
        err = util_base64_decode(
                writeContext->contextData.bytes,
                writeContext->contextData.numBytes,
                writeContext->contextData.bytes,
                writeContext->contextData.numBytes,
                &writeContext->contextData.numBytes);
        if (err != kHAPError_None) {
            writeContext->status = kHAPIPAccessoryServerStatusCode_InvalidValueInWrite;
            return;
        }
    }

    switch (baseCharacteristic->format) {
        // Write the new value to the characteristic.
        case kHAPCharacteristicFormat_Data: {
            if (writeContext->type == kHAPIPWriteValueType_String) {
                HAPAssert(writeContext->value.stringValue.bytes);
                err = util_base64_decode(
                        writeContext->value.stringValue.bytes,
                        writeContext->value.stringValue.numBytes,
                        writeContext->value.stringValue.bytes,
                        writeContext->value.stringValue.numBytes,
                        &writeContext->value.stringValue.numBytes);
                if (err == kHAPError_None) {
                    HAPAssert(writeContext->value.stringValue.bytes);
                    err = HAPDataCharacteristicHandleWrite(
                            HAPNonnull(session->server),
                            &(const HAPDataCharacteristicWriteRequest) {
                                    .transportType = kHAPTransportType_IP,
                                    .session = &session->securitySession._.hap,
                                    .characteristic = (const HAPDataCharacteristic*) baseCharacteristic,
                                    .service = service,
                                    .accessory = accessory,
                                    .remote = writeContext->remote,
                                    .authorizationData = { .bytes = authorizationDataBytes,
                                                           .numBytes = numAuthorizationDataBytes } },
                            HAPNonnull(writeContext->value.stringValue.bytes),
                            writeContext->value.stringValue.numBytes,
                            HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
                    writeContext->status = ConvertCharacteristicWriteErrorToStatusCode(err);
                } else {
                    writeContext->status = kHAPIPAccessoryServerStatusCode_InvalidValueInWrite;
                }
            } else {
                // The string value type is the only valid format for data based Characteristics.
                writeContext->status = kHAPIPAccessoryServerStatusCode_InvalidValueInWrite;
            }
            break;
        }
        case kHAPCharacteristicFormat_Bool: {
            if ((writeContext->type == kHAPIPWriteValueType_UInt) && (writeContext->value.unsignedIntValue <= 1)) {
                err = HAPBoolCharacteristicHandleWrite(
                        HAPNonnull(session->server),
                        &(const HAPBoolCharacteristicWriteRequest) {
                                .transportType = kHAPTransportType_IP,
                                .session = &session->securitySession._.hap,
                                .characteristic = (const HAPBoolCharacteristic*) baseCharacteristic,
                                .service = service,
                                .accessory = accessory,
                                .remote = writeContext->remote,
                                .authorizationData = { .bytes = authorizationDataBytes,
                                                       .numBytes = numAuthorizationDataBytes } },
                        (bool) writeContext->value.unsignedIntValue,
                        HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
                writeContext->status = ConvertCharacteristicWriteErrorToStatusCode(err);
            } else {
                writeContext->status = kHAPIPAccessoryServerStatusCode_InvalidValueInWrite;
            }
            break;
        }
        case kHAPCharacteristicFormat_UInt8: {
            if ((writeContext->type == kHAPIPWriteValueType_UInt) &&
                (writeContext->value.unsignedIntValue <= UINT8_MAX)) {
                err = HAPUInt8CharacteristicHandleWrite(
                        HAPNonnull(session->server),
                        &(const HAPUInt8CharacteristicWriteRequest) {
                                .transportType = kHAPTransportType_IP,
                                .session = &session->securitySession._.hap,
                                .characteristic = (const HAPUInt8Characteristic*) baseCharacteristic,
                                .service = service,
                                .accessory = accessory,
                                .remote = writeContext->remote,

                                .authorizationData = { .bytes = authorizationDataBytes,
                                                       .numBytes = numAuthorizationDataBytes },
                                .contextData = { .bytes = writeContext->contextData.bytes,
                                                 .numBytes = writeContext->contextData.numBytes },
                        },
                        (uint8_t) writeContext->value.unsignedIntValue,
                        HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
                writeContext->status = ConvertCharacteristicWriteErrorToStatusCode(err);
            } else {
                writeContext->status = kHAPIPAccessoryServerStatusCode_InvalidValueInWrite;
            }
            break;
        }
        case kHAPCharacteristicFormat_UInt16: {
            if ((writeContext->type == kHAPIPWriteValueType_UInt) &&
                (writeContext->value.unsignedIntValue <= UINT16_MAX)) {
                err = HAPUInt16CharacteristicHandleWrite(
                        HAPNonnull(session->server),
                        &(const HAPUInt16CharacteristicWriteRequest) {
                                .transportType = kHAPTransportType_IP,
                                .session = &session->securitySession._.hap,
                                .characteristic = (const HAPUInt16Characteristic*) baseCharacteristic,
                                .service = service,
                                .accessory = accessory,
                                .remote = writeContext->remote,
                                .authorizationData = { .bytes = authorizationDataBytes,
                                                       .numBytes = numAuthorizationDataBytes } },
                        (uint16_t) writeContext->value.unsignedIntValue,
                        HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
                writeContext->status = ConvertCharacteristicWriteErrorToStatusCode(err);
            } else {
                writeContext->status = kHAPIPAccessoryServerStatusCode_InvalidValueInWrite;
            }
            break;
        }
        case kHAPCharacteristicFormat_UInt32: {
            if ((writeContext->type == kHAPIPWriteValueType_UInt) &&
                (writeContext->value.unsignedIntValue <= UINT32_MAX)) {
                err = HAPUInt32CharacteristicHandleWrite(
                        HAPNonnull(session->server),
                        &(const HAPUInt32CharacteristicWriteRequest) {
                                .transportType = kHAPTransportType_IP,
                                .session = &session->securitySession._.hap,
                                .characteristic = (const HAPUInt32Characteristic*) baseCharacteristic,
                                .service = service,
                                .accessory = accessory,
                                .remote = writeContext->remote,
                                .authorizationData = { .bytes = authorizationDataBytes,
                                                       .numBytes = numAuthorizationDataBytes } },
                        (uint32_t) writeContext->value.unsignedIntValue,
                        HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
                writeContext->status = ConvertCharacteristicWriteErrorToStatusCode(err);
            } else {
                writeContext->status = kHAPIPAccessoryServerStatusCode_InvalidValueInWrite;
            }
            break;
        }
        case kHAPCharacteristicFormat_UInt64: {
            if (writeContext->type == kHAPIPWriteValueType_UInt) {
                err = HAPUInt64CharacteristicHandleWrite(
                        HAPNonnull(session->server),
                        &(const HAPUInt64CharacteristicWriteRequest) {
                                .transportType = kHAPTransportType_IP,
                                .session = &session->securitySession._.hap,
                                .characteristic = (const HAPUInt64Characteristic*) baseCharacteristic,
                                .service = service,
                                .accessory = accessory,
                                .remote = writeContext->remote,
                                .authorizationData = { .bytes = authorizationDataBytes,
                                                       .numBytes = numAuthorizationDataBytes } },
                        writeContext->value.unsignedIntValue,
                        HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
                writeContext->status = ConvertCharacteristicWriteErrorToStatusCode(err);
            } else {
                writeContext->status = kHAPIPAccessoryServerStatusCode_InvalidValueInWrite;
            }
            break;
        }
        case kHAPCharacteristicFormat_Int: {
            if ((writeContext->type == kHAPIPWriteValueType_UInt) &&
                (writeContext->value.unsignedIntValue <= INT32_MAX)) {
                writeContext->value.intValue = (int32_t) writeContext->value.unsignedIntValue;
                writeContext->type = kHAPIPWriteValueType_Int;
            }
            if (writeContext->type == kHAPIPWriteValueType_Int) {
                err = HAPIntCharacteristicHandleWrite(
                        HAPNonnull(session->server),
                        &(const HAPIntCharacteristicWriteRequest) {
                                .transportType = kHAPTransportType_IP,
                                .session = &session->securitySession._.hap,
                                .characteristic = (const HAPIntCharacteristic*) baseCharacteristic,
                                .service = service,
                                .accessory = accessory,
                                .remote = writeContext->remote,
                                .authorizationData = { .bytes = authorizationDataBytes,
                                                       .numBytes = numAuthorizationDataBytes } },
                        writeContext->value.intValue,
                        HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
                writeContext->status = ConvertCharacteristicWriteErrorToStatusCode(err);
            } else {
                writeContext->status = kHAPIPAccessoryServerStatusCode_InvalidValueInWrite;
            }
            break;
        }
        case kHAPCharacteristicFormat_Float: {
            if ((writeContext->type == kHAPIPWriteValueType_Int) && (writeContext->value.intValue >= -FLT_MAX) &&
                (writeContext->value.intValue <= FLT_MAX)) {
                writeContext->value.floatValue = (float) writeContext->value.intValue;
                writeContext->type = kHAPIPWriteValueType_Float;
            }
            if ((writeContext->type == kHAPIPWriteValueType_UInt) &&
                (writeContext->value.unsignedIntValue <= FLT_MAX)) {
                writeContext->value.floatValue = (float) writeContext->value.unsignedIntValue;
                writeContext->type = kHAPIPWriteValueType_Float;
            }
            if (writeContext->type == kHAPIPWriteValueType_Float) {
                err = HAPFloatCharacteristicHandleWrite(
                        HAPNonnull(session->server),
                        &(const HAPFloatCharacteristicWriteRequest) {
                                .transportType = kHAPTransportType_IP,
                                .session = &session->securitySession._.hap,
                                .characteristic = (const HAPFloatCharacteristic*) baseCharacteristic,
                                .service = service,
                                .accessory = accessory,
                                .remote = writeContext->remote,
                                .authorizationData = { .bytes = authorizationDataBytes,
                                                       .numBytes = numAuthorizationDataBytes } },
                        writeContext->value.floatValue,
                        HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
                writeContext->status = ConvertCharacteristicWriteErrorToStatusCode(err);
            } else {
                writeContext->status = kHAPIPAccessoryServerStatusCode_InvalidValueInWrite;
            }
            break;
        }
        case kHAPCharacteristicFormat_String: {
            if ((writeContext->type == kHAPIPWriteValueType_String) &&
                (writeContext->value.stringValue.numBytes <= 256)) {
                HAPAssert(writeContext->value.stringValue.bytes);
                HAPAssert(dataBuffer->data);
                HAPAssert(dataBuffer->position <= dataBuffer->limit);
                HAPAssert(dataBuffer->limit <= dataBuffer->capacity);
                if (writeContext->value.stringValue.numBytes >= dataBuffer->limit - dataBuffer->position) {
                    writeContext->status = kHAPIPAccessoryServerStatusCode_OutOfResources;
                } else {
                    HAPRawBufferCopyBytes(
                            &dataBuffer->data[dataBuffer->position],
                            HAPNonnull(writeContext->value.stringValue.bytes),
                            writeContext->value.stringValue.numBytes);
                    dataBuffer->data[dataBuffer->position + writeContext->value.stringValue.numBytes] = '\0';
                    err = HAPStringCharacteristicHandleWrite(
                            HAPNonnull(session->server),
                            &(const HAPStringCharacteristicWriteRequest) {
                                    .transportType = kHAPTransportType_IP,
                                    .session = &session->securitySession._.hap,
                                    .characteristic = (const HAPStringCharacteristic*) baseCharacteristic,
                                    .service = service,
                                    .accessory = accessory,
                                    .remote = writeContext->remote,
                                    .authorizationData = { .bytes = authorizationDataBytes,
                                                           .numBytes = numAuthorizationDataBytes } },
                            &dataBuffer->data[dataBuffer->position],
                            HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
                    writeContext->status = ConvertCharacteristicWriteErrorToStatusCode(err);
                }
            } else {
                writeContext->status = kHAPIPAccessoryServerStatusCode_InvalidValueInWrite;
            }
            break;
        }
        case kHAPCharacteristicFormat_TLV8: {
            if (writeContext->type == kHAPIPWriteValueType_String) {
                HAPAssert(writeContext->value.stringValue.bytes);
                err = util_base64_decode(
                        writeContext->value.stringValue.bytes,
                        writeContext->value.stringValue.numBytes,
                        writeContext->value.stringValue.bytes,
                        writeContext->value.stringValue.numBytes,
                        &writeContext->value.stringValue.numBytes);
                if (err == kHAPError_None) {
                    HAPTLVReader tlvReader;
                    HAPTLVReaderCreate(
                            &tlvReader,
                            writeContext->value.stringValue.bytes,
                            writeContext->value.stringValue.numBytes);
                    err = HAPTLV8CharacteristicHandleWrite(
                            HAPNonnull(session->server),
                            &(const HAPTLV8CharacteristicWriteRequest) {
                                    .transportType = kHAPTransportType_IP,
                                    .session = &session->securitySession._.hap,
                                    .characteristic = (const HAPTLV8Characteristic*) baseCharacteristic,
                                    .service = service,
                                    .accessory = accessory,
                                    .remote = writeContext->remote,
                                    .authorizationData = { .bytes = authorizationDataBytes,
                                                           .numBytes = numAuthorizationDataBytes } },
                            &tlvReader,
                            HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
                    writeContext->status = ConvertCharacteristicWriteErrorToStatusCode(err);
                } else {
                    writeContext->status = kHAPIPAccessoryServerStatusCode_InvalidValueInWrite;
                }
            }
            break;
        }
    }

    if (writeContext->status != kHAPIPAccessoryServerStatusCode_Success) {
        // There was an error writing the new value to the characteristic. Return.
        return;
    }

    if (baseCharacteristic->properties.ip.supportsWriteResponse) {
        // The characteristic supports write responses.
        HAPIPByteBuffer dataBufferSnapshot;
        HAPRawBufferCopyBytes(&dataBufferSnapshot, dataBuffer, sizeof dataBufferSnapshot);
        HAPIPReadContext readContext;
        HAPRawBufferZero(&readContext, sizeof readContext);
        readContext.aid = writeContext->aid;
        readContext.iid = writeContext->iid;
        // Read the current value of the characteristic.
        handle_characteristic_read_request(session, characteristic, service, accessory, &readContext, dataBuffer);
        writeContext->status = readContext.status;
        if (readContext.status == kHAPIPAccessoryServerStatusCode_Success) {
            // If the value was read from the characteristic, we can continue generating the write response.
            if (writeContext->response) {
                // The write request is expecting a response.
                switch (baseCharacteristic->format) {
                    case kHAPCharacteristicFormat_Bool:
                    case kHAPCharacteristicFormat_UInt8:
                    case kHAPCharacteristicFormat_UInt16:
                    case kHAPCharacteristicFormat_UInt32:
                    case kHAPCharacteristicFormat_UInt64: {
                        writeContext->value.unsignedIntValue = readContext.value.unsignedIntValue;
                        break;
                    }
                    case kHAPCharacteristicFormat_Int: {
                        writeContext->value.intValue = readContext.value.intValue;
                        break;
                    }
                    case kHAPCharacteristicFormat_Float: {
                        writeContext->value.floatValue = readContext.value.floatValue;
                        break;
                    }
                    case kHAPCharacteristicFormat_Data:
                    case kHAPCharacteristicFormat_String:
                    case kHAPCharacteristicFormat_TLV8: {
                        writeContext->value.stringValue.bytes = readContext.value.stringValue.bytes;
                        writeContext->value.stringValue.numBytes = readContext.value.stringValue.numBytes;
                        break;
                    }
                }
            } else {
                // The write request is not expecting a response.
                // Ignore value of read operation and revert possible changes to data buffer.
                HAPRawBufferCopyBytes(dataBuffer, &dataBufferSnapshot, sizeof *dataBuffer);
            }
        }
    } else {
        // The characteristic does not support write responses.
        if (writeContext->response) {
            // The write request was expecting a response.
            writeContext->status = kHAPIPAccessoryServerStatusCode_ReadFromWriteOnlyCharacteristic;
        }
    }
}

/**
 * Handles a characterstic write request.
 *
 * Handling a characteristic write request must do at least one of two things:
 * 1. If requested, change the session subscription status for characteristic event notifications.
 * 2. If requested, write a new value to the characteristic.
 *
 * @param session           The session who is performing the request.
 * @param characteristic    The characteristic who is the target of the request.
 * @param service           The service that contains the characteristic.
 * @param accessory         The accessory that provides the service.
 * @param context           The context of the write request.
 * @param dataBuffer        Buffer for values of type data, string or TLV8.
 */
static void handle_characteristic_write_request(
        HAPIPSessionDescriptor* session,
        const HAPCharacteristic* characteristic,
        const HAPService* service,
        const HAPAccessory* accessory,
        HAPIPWriteContext* context,
        HAPIPByteBuffer* dataBuffer) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPPrecondition(session->securitySession.type == kHAPIPSecuritySessionType_HAP);
    HAPPrecondition(session->securitySession.isOpen);
    HAPPrecondition(session->securitySession.isSecured || kHAPIPAccessoryServer_SessionSecurityDisabled);
    HAPPrecondition(!HAPSessionIsTransient(&session->securitySession._.hap));
    HAPPrecondition(characteristic);
    HAPPrecondition(service);
    HAPPrecondition(accessory);
    HAPPrecondition(context);
    HAPPrecondition(dataBuffer);

    const HAPBaseCharacteristic* baseCharacteristic = characteristic;
    HAPAssert(baseCharacteristic->iid == context->iid);

    if ((context->type == kHAPIPWriteValueType_None) && (context->ev == kHAPIPEventNotificationState_Undefined)) {
        // There is neither an event notification or a write to process.
        context->status = kHAPIPAccessoryServerStatusCode_InvalidValueInWrite;
        return;
    }

    // Will handle a session subscription change request for events from this characteristic.
    handle_characteristic_write_request_process_subscription_change(
            session, characteristic, service, accessory, context);
    // Will handle a request to write a new value to this characteristic.
    handle_characteristic_write_request_process_write(session, characteristic, service, accessory, context, dataBuffer);
}

/**
 * Handles a set of characteristic write requests.
 *
 * @param      session              IP session descriptor.
 * @param      contexts             Request contexts.
 * @param      numContexts          Length of @p contexts.
 * @param      dataBuffer           Buffer for values of type data, string or TLV8.
 * @param      timedWrite           Whether the request was a valid Execute Write Request or a regular Write Request.
 *
 * @return 0                        If all writes could be handled successfully.
 * @return -1                       Otherwise (Multi-Status).
 */
HAP_RESULT_USE_CHECK
static int handle_characteristic_write_requests(
        HAPIPSessionDescriptor* session,
        HAPIPWriteContext* contexts,
        size_t numContexts,
        HAPIPByteBuffer* dataBuffer,
        bool timedWrite) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPPrecondition(session->securitySession.type == kHAPIPSecuritySessionType_HAP);
    HAPPrecondition(session->securitySession.isOpen);
    HAPPrecondition(session->securitySession.isSecured || kHAPIPAccessoryServer_SessionSecurityDisabled);
    HAPPrecondition(!HAPSessionIsTransient(&session->securitySession._.hap));
    HAPPrecondition(contexts);
    HAPPrecondition(dataBuffer);

    int r = 0;

    for (size_t i = 0; i < numContexts; i++) {
        HAPIPWriteContext* writeContext = &contexts[i];
        const HAPCharacteristic* characteristic;
        const HAPService* service;
        const HAPAccessory* accessory;
        get_db_ctx(session->server, writeContext->aid, writeContext->iid, &characteristic, &service, &accessory);
        if (characteristic) {
            HAPAssert(service);
            HAPAssert(accessory);
            const HAPBaseCharacteristic* baseCharacteristic = characteristic;
            if ((writeContext->type != kHAPIPWriteValueType_None) &&
                baseCharacteristic->properties.requiresTimedWrite && !timedWrite) {
                // If the accessory receives a standard write request on a characteristic which requires timed write,
                // the accessory must respond with HAP status error code -70410 (HAPIPStatusErrorCodeInvalidWrite).
                // See HomeKit Accessory Protocol Specification R17
                // Section 6.7.2.4 Timed Write Procedures
                HAPLogCharacteristic(
                        &logObject,
                        characteristic,
                        service,
                        accessory,
                        "Rejected write: Only timed writes are supported.");
                writeContext->status = kHAPIPAccessoryServerStatusCode_InvalidValueInWrite;
            } else {
                handle_characteristic_write_request(
                        session, characteristic, service, accessory, &contexts[i], dataBuffer);
            }
        } else {
            writeContext->status = kHAPIPAccessoryServerStatusCode_ResourceDoesNotExist;
        }
        if ((r == 0) && (writeContext->status != kHAPIPAccessoryServerStatusCode_Success)) {
            r = -1;
        }
        if ((r == 0) && writeContext->response) {
            r = -1;
        }
    }

    return r;
}

static void put_characteristics(HAPIPSessionDescriptor* session) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPAccessoryServer* server = session->server;
    HAPPrecondition(session->securitySession.type == kHAPIPSecuritySessionType_HAP);
    HAPPrecondition(session->securitySession.isOpen);
    HAPPrecondition(session->securitySession.isSecured || kHAPIPAccessoryServer_SessionSecurityDisabled);
    HAPPrecondition(!HAPSessionIsTransient(&session->securitySession._.hap));

    HAPError err;
    int r;
    size_t i, contexts_count;
    bool pid_valid;
    uint64_t pid;
    HAPIPByteBuffer data_buffer;

    HAPAssert(session->inboundBuffer.data);
    HAPAssert(session->inboundBuffer.position <= session->inboundBuffer.limit);
    HAPAssert(session->inboundBuffer.limit <= session->inboundBuffer.capacity);
    HAPAssert(session->httpReaderPosition <= session->inboundBuffer.position);
    if (session->httpContentLengthIsDefined) {
        HAPAssert(session->httpContentLength <= session->inboundBuffer.position - session->httpReaderPosition);
        err = HAPIPAccessoryProtocolGetCharacteristicWriteRequests(
                &session->inboundBuffer.data[session->httpReaderPosition],
                session->httpContentLength,
                server->ip.storage->writeContexts,
                server->ip.storage->numWriteContexts,
                &contexts_count,
                &pid_valid,
                &pid);
        if (!err) {
            if ((session->timedWriteExpirationTime && pid_valid &&
                 session->timedWriteExpirationTime < HAPPlatformClockGetCurrent()) ||
                (session->timedWriteExpirationTime && pid_valid && session->timedWritePID != pid) ||
                (!session->timedWriteExpirationTime && pid_valid)) {
                // If the accessory receives an Execute Write Request after the TTL has expired it must ignore the
                // request and respond with HAP status error code -70410 (HAPIPStatusErrorCodeInvalidWrite).
                // See HomeKit Accessory Protocol Specification R17
                // Section 6.7.2.4 Timed Write Procedures
                HAPLog(&logObject, "Rejecting expired Execute Write Request.");
                for (i = 0; i < contexts_count; i++) {
                    server->ip.storage->writeContexts[i].status = kHAPIPAccessoryServerStatusCode_InvalidValueInWrite;
                }
                HAPAssert(i == contexts_count);
                write_characteristic_write_response(session, server->ip.storage->writeContexts, contexts_count);
            } else if (contexts_count == 0) {
                WriteResponse(session, kHAPIPAccessoryServerResponse_NoContent);
            } else {
                data_buffer.data = server->ip.storage->scratchBuffer.bytes;
                data_buffer.capacity = server->ip.storage->scratchBuffer.numBytes;
                data_buffer.limit = server->ip.storage->scratchBuffer.numBytes;
                data_buffer.position = 0;
                HAPAssert(data_buffer.data);
                HAPAssert(data_buffer.position <= data_buffer.limit);
                HAPAssert(data_buffer.limit <= data_buffer.capacity);
                r = handle_characteristic_write_requests(
                        session, server->ip.storage->writeContexts, contexts_count, &data_buffer, pid_valid);
                if (r == 0) {
                    WriteResponse(session, kHAPIPAccessoryServerResponse_NoContent);
                } else {
                    write_characteristic_write_response(session, server->ip.storage->writeContexts, contexts_count);
                }
            }
            // Reset timed write transaction.
            if (session->timedWriteExpirationTime && pid_valid) {
                session->timedWriteExpirationTime = 0;
                session->timedWritePID = 0;
            }
        } else if (err == kHAPError_OutOfResources) {
            WriteResponse(session, kHAPIPAccessoryServerResponse_OutOfResources);
        } else {
            WriteResponse(session, kHAPIPAccessoryServerResponse_BadRequest);
        }
    } else {
        WriteResponse(session, kHAPIPAccessoryServerResponse_BadRequest);
    }
}

/**
 * Converts a characteristic read request error to the corresponding HAP status code.
 *
 * @param      error                Read request error.
 *
 * @return HAP read request status code.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Table 6-11 HAP Status Codes
 */
HAP_RESULT_USE_CHECK
static int32_t ConvertCharacteristicReadErrorToStatusCode(HAPError error) {
    switch (error) {
        case kHAPError_None: {
            return kHAPIPAccessoryServerStatusCode_Success;
        }
        case kHAPError_Unknown: {
            return kHAPIPAccessoryServerStatusCode_UnableToPerformOperation;
        }
        case kHAPError_InvalidState: {
            return kHAPIPAccessoryServerStatusCode_NotAllowedInCurrentState;
        }
        case kHAPError_InvalidData: {
            HAPFatalError();
        }
        case kHAPError_OutOfResources: {
            return kHAPIPAccessoryServerStatusCode_OutOfResources;
        }
        case kHAPError_NotAuthorized: {
            return kHAPIPAccessoryServerStatusCode_InsufficientAuthorization;
        }
        case kHAPError_Busy: {
            return kHAPIPAccessoryServerStatusCode_ResourceIsBusy;
        }
        default:
            HAPFatalError();
    }
}

static void handle_characteristic_read_request(
        HAPIPSessionDescriptor* session,
        const HAPCharacteristic* chr_,
        const HAPService* svc,
        const HAPAccessory* acc,
        HAPIPReadContext* ctx,
        HAPIPByteBuffer* data_buffer) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPPrecondition(session->securitySession.type == kHAPIPSecuritySessionType_HAP);
    HAPPrecondition(session->securitySession.isOpen);
    HAPPrecondition(session->securitySession.isSecured || kHAPIPAccessoryServer_SessionSecurityDisabled);
    HAPPrecondition(!HAPSessionIsTransient(&session->securitySession._.hap));

    HAPError err = kHAPError_None;

    size_t n, sval_length;
    bool bool_val;
    int32_t int_val;
    uint8_t uint8_val;
    uint16_t uint16_val;
    uint32_t uint32_val;
    uint64_t uint64_val;
    float float_val;
    HAPCharacteristicContextCallbacks* contextCallbacks = NULL;
    HAPTLVWriter tlv8_writer;
    HAPAssert(chr_);
    const HAPBaseCharacteristic* chr = chr_;
    HAPAssert(svc);
    HAPAssert(acc);
    HAPAssert(ctx);
    HAPAssert(data_buffer);
    HAPAssert(data_buffer->data);
    HAPAssert(data_buffer->position <= data_buffer->limit);
    HAPAssert(data_buffer->limit <= data_buffer->capacity);
    HAPIPReadContext* readContext = ctx;
    readContext->status = kHAPIPAccessoryServerStatusCode_Success;
    switch (chr->format) {
        case kHAPCharacteristicFormat_Data: {
            err = HAPDataCharacteristicHandleRead(
                    HAPNonnull(session->server),
                    &(const HAPDataCharacteristicReadRequest) { .transportType = kHAPTransportType_IP,
                                                                .session = &session->securitySession._.hap,
                                                                .characteristic = (const HAPDataCharacteristic*) chr,
                                                                .service = svc,
                                                                .accessory = acc },
                    &data_buffer->data[data_buffer->position],
                    data_buffer->limit - data_buffer->position,
                    &sval_length,
                    HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
            readContext->status = ConvertCharacteristicReadErrorToStatusCode(err);
            if (readContext->status == kHAPIPAccessoryServerStatusCode_Success) {
                if (sval_length <= data_buffer->limit - data_buffer->position) {
                    util_base64_encode(
                            &data_buffer->data[data_buffer->position],
                            sval_length,
                            &data_buffer->data[data_buffer->position],
                            data_buffer->limit - data_buffer->position,
                            &sval_length);
                    if (sval_length < data_buffer->limit - data_buffer->position) {
                        data_buffer->data[data_buffer->position + sval_length] = 0;
                        readContext->value.stringValue.bytes = &data_buffer->data[data_buffer->position];
                        readContext->value.stringValue.numBytes = sval_length;
                        data_buffer->position += sval_length + 1;
                        HAPAssert(data_buffer->position <= data_buffer->limit);
                        HAPAssert(data_buffer->limit <= data_buffer->capacity);
                    } else {
                        readContext->status = kHAPIPAccessoryServerStatusCode_OutOfResources;
                    }
                } else {
                    readContext->status = kHAPIPAccessoryServerStatusCode_OutOfResources;
                }
            }
            break;
        }
        case kHAPCharacteristicFormat_Bool: {
            err = HAPBoolCharacteristicHandleRead(
                    HAPNonnull(session->server),
                    &(const HAPBoolCharacteristicReadRequest) { .transportType = kHAPTransportType_IP,
                                                                .session = &session->securitySession._.hap,
                                                                .characteristic = (const HAPBoolCharacteristic*) chr,
                                                                .service = svc,
                                                                .accessory = acc },
                    &bool_val,
                    HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
            readContext->status = ConvertCharacteristicReadErrorToStatusCode(err);
            if (readContext->status == kHAPIPAccessoryServerStatusCode_Success) {
                readContext->value.unsignedIntValue = bool_val ? 1 : 0;
            }
            break;
        }
        case kHAPCharacteristicFormat_UInt8: {
            err = HAPUInt8CharacteristicHandleRead(
                    HAPNonnull(session->server),
                    &(const HAPUInt8CharacteristicReadRequest) { .transportType = kHAPTransportType_IP,
                                                                 .session = &session->securitySession._.hap,
                                                                 .characteristic = (const HAPUInt8Characteristic*) chr,
                                                                 .service = svc,
                                                                 .accessory = acc },
                    &uint8_val,
                    HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
            readContext->status = ConvertCharacteristicReadErrorToStatusCode(err);
            if (readContext->status == kHAPIPAccessoryServerStatusCode_Success) {
                readContext->value.unsignedIntValue = uint8_val;
            }
            if (chr->properties.supportsEventNotificationContextInformation) {
                contextCallbacks = &((HAPUInt8Characteristic*) chr)->contextCallbacks;
            }
            break;
        }
        case kHAPCharacteristicFormat_UInt16: {
            err = HAPUInt16CharacteristicHandleRead(
                    HAPNonnull(session->server),
                    &(const HAPUInt16CharacteristicReadRequest) { .transportType = kHAPTransportType_IP,
                                                                  .session = &session->securitySession._.hap,
                                                                  .characteristic =
                                                                          (const HAPUInt16Characteristic*) chr,
                                                                  .service = svc,
                                                                  .accessory = acc },
                    &uint16_val,
                    HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
            readContext->status = ConvertCharacteristicReadErrorToStatusCode(err);
            if (readContext->status == kHAPIPAccessoryServerStatusCode_Success) {
                readContext->value.unsignedIntValue = uint16_val;
            }
            break;
        }
        case kHAPCharacteristicFormat_UInt32: {
            err = HAPUInt32CharacteristicHandleRead(
                    HAPNonnull(session->server),
                    &(const HAPUInt32CharacteristicReadRequest) { .transportType = kHAPTransportType_IP,
                                                                  .session = &session->securitySession._.hap,
                                                                  .characteristic =
                                                                          (const HAPUInt32Characteristic*) chr,
                                                                  .service = svc,
                                                                  .accessory = acc },
                    &uint32_val,
                    HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
            readContext->status = ConvertCharacteristicReadErrorToStatusCode(err);
            if (readContext->status == kHAPIPAccessoryServerStatusCode_Success) {
                readContext->value.unsignedIntValue = uint32_val;
            }
            break;
        }
        case kHAPCharacteristicFormat_UInt64: {
            err = HAPUInt64CharacteristicHandleRead(
                    HAPNonnull(session->server),
                    &(const HAPUInt64CharacteristicReadRequest) { .transportType = kHAPTransportType_IP,
                                                                  .session = &session->securitySession._.hap,
                                                                  .characteristic =
                                                                          (const HAPUInt64Characteristic*) chr,
                                                                  .service = svc,
                                                                  .accessory = acc },
                    &uint64_val,
                    HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
            readContext->status = ConvertCharacteristicReadErrorToStatusCode(err);
            if (readContext->status == kHAPIPAccessoryServerStatusCode_Success) {
                readContext->value.unsignedIntValue = uint64_val;
            }
            break;
        }
        case kHAPCharacteristicFormat_Int: {
            err = HAPIntCharacteristicHandleRead(
                    HAPNonnull(session->server),
                    &(const HAPIntCharacteristicReadRequest) { .transportType = kHAPTransportType_IP,
                                                               .session = &session->securitySession._.hap,
                                                               .characteristic = (const HAPIntCharacteristic*) chr,
                                                               .service = svc,
                                                               .accessory = acc },
                    &int_val,
                    HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
            readContext->status = ConvertCharacteristicReadErrorToStatusCode(err);
            if (readContext->status == kHAPIPAccessoryServerStatusCode_Success) {
                readContext->value.intValue = int_val;
            }
            break;
        }
        case kHAPCharacteristicFormat_Float: {
            err = HAPFloatCharacteristicHandleRead(
                    HAPNonnull(session->server),
                    &(const HAPFloatCharacteristicReadRequest) { .transportType = kHAPTransportType_IP,
                                                                 .session = &session->securitySession._.hap,
                                                                 .characteristic = (const HAPFloatCharacteristic*) chr,
                                                                 .service = svc,
                                                                 .accessory = acc },
                    &float_val,
                    HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
            readContext->status = ConvertCharacteristicReadErrorToStatusCode(err);
            if (readContext->status == kHAPIPAccessoryServerStatusCode_Success) {
                readContext->value.floatValue = float_val;
            }
            break;
        }
        case kHAPCharacteristicFormat_String: {
            err = HAPStringCharacteristicHandleRead(
                    HAPNonnull(session->server),
                    &(const HAPStringCharacteristicReadRequest) { .transportType = kHAPTransportType_IP,
                                                                  .session = &session->securitySession._.hap,
                                                                  .characteristic =
                                                                          (const HAPStringCharacteristic*) chr,
                                                                  .service = svc,
                                                                  .accessory = acc },
                    &data_buffer->data[data_buffer->position],
                    data_buffer->limit - data_buffer->position,
                    HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
            readContext->status = ConvertCharacteristicReadErrorToStatusCode(err);
            if (readContext->status == kHAPIPAccessoryServerStatusCode_Success) {
                sval_length = HAPStringGetNumBytes(&data_buffer->data[data_buffer->position]);
                if (sval_length < data_buffer->limit - data_buffer->position) {
                    data_buffer->data[data_buffer->position + sval_length] = 0;
                    readContext->value.stringValue.bytes = &data_buffer->data[data_buffer->position];
                    readContext->value.stringValue.numBytes = sval_length;
                    data_buffer->position += sval_length + 1;
                    HAPAssert(data_buffer->position <= data_buffer->limit);
                    HAPAssert(data_buffer->limit <= data_buffer->capacity);
                } else {
                    readContext->status = kHAPIPAccessoryServerStatusCode_OutOfResources;
                }
            }
            break;
        }
        case kHAPCharacteristicFormat_TLV8: {
            n = data_buffer->limit - data_buffer->position;
            HAPTLVWriterCreate(&tlv8_writer, &data_buffer->data[data_buffer->position], n);
            err = HAPTLV8CharacteristicHandleRead(
                    HAPNonnull(session->server),
                    &(const HAPTLV8CharacteristicReadRequest) { .transportType = kHAPTransportType_IP,
                                                                .session = &session->securitySession._.hap,
                                                                .characteristic = (const HAPTLV8Characteristic*) chr,
                                                                .service = svc,
                                                                .accessory = acc },
                    &tlv8_writer,
                    HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
            readContext->status = ConvertCharacteristicReadErrorToStatusCode(err);
            if (readContext->status == kHAPIPAccessoryServerStatusCode_Success) {
                if (tlv8_writer.numBytes <= data_buffer->limit - data_buffer->position) {
                    util_base64_encode(
                            &data_buffer->data[data_buffer->position],
                            tlv8_writer.numBytes,
                            &data_buffer->data[data_buffer->position],
                            data_buffer->limit - data_buffer->position,
                            &sval_length);
                    if (sval_length < data_buffer->limit - data_buffer->position) {
                        data_buffer->data[data_buffer->position + sval_length] = 0;
                        readContext->value.stringValue.bytes = &data_buffer->data[data_buffer->position];
                        readContext->value.stringValue.numBytes = sval_length;
                        data_buffer->position += sval_length + 1;
                        HAPAssert(data_buffer->position <= data_buffer->limit);
                        HAPAssert(data_buffer->limit <= data_buffer->capacity);
                    } else {
                        readContext->status = kHAPIPAccessoryServerStatusCode_OutOfResources;
                    }
                } else {
                    readContext->status = kHAPIPAccessoryServerStatusCode_OutOfResources;
                }
            }
            break;
        }
    }
    if (err == kHAPError_None && readContext->status == kHAPIPAccessoryServerStatusCode_Success) {
        if (contextCallbacks != NULL) {
            HAPTLVWriter* writer;
            err = contextCallbacks->handleReadContextData(
                    HAPNonnull(session->server),
                    chr,
                    &writer,
                    HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
            if (err) {
                HAPLogError(&logObject, "Error during read context data");
                return;
            }

            if (writer->numBytes != 0) {
                size_t contextLength = util_base64_encoded_len(writer->numBytes);
                if ((contextLength + 1) <= data_buffer->limit - data_buffer->position) {
                    char* contextDataBuffer = data_buffer->data + data_buffer->position;
                    size_t contextDataBufferLength = 0;
                    util_base64_encode(
                            writer->bytes,
                            writer->numBytes,
                            contextDataBuffer,
                            data_buffer->limit - data_buffer->position,
                            &contextDataBufferLength);

                    HAPAssert(contextDataBufferLength == contextLength);
                    data_buffer->position += contextDataBufferLength;
                    data_buffer->data[data_buffer->position] = 0;
                    data_buffer->position += 1;
                    readContext->contextData.numBytes = contextDataBufferLength;
                    readContext->contextData.bytes = contextDataBuffer;
                } else {
                    readContext->status = kHAPIPAccessoryServerStatusCode_OutOfResources;
                }
            } else {
                readContext->contextData.numBytes = 0;
                readContext->contextData.bytes = NULL;
            }
        } else {
            readContext->contextData.numBytes = 0;
            readContext->contextData.bytes = NULL;
        }
    }
}

HAP_RESULT_USE_CHECK
static int handle_characteristic_read_requests(
        HAPIPSessionDescriptor* session,
        HAPIPSessionContext session_context,
        HAPIPReadContext* contexts,
        size_t contexts_count,
        HAPIPByteBuffer* data_buffer) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPPrecondition(session->securitySession.type == kHAPIPSecuritySessionType_HAP);
    HAPPrecondition(session->securitySession.isOpen);
    HAPPrecondition(session->securitySession.isSecured || kHAPIPAccessoryServer_SessionSecurityDisabled);
    HAPPrecondition(!HAPSessionIsTransient(&session->securitySession._.hap));

    int r;
    size_t i, j;
    const HAPCharacteristic* c;
    const HAPService* svc;
    const HAPAccessory* acc;
    HAPAssert(contexts);
    r = 0;
    for (i = 0; i < contexts_count; i++) {
        HAPIPReadContext* readContext = &contexts[i];

        get_db_ctx(session->server, readContext->aid, readContext->iid, &c, &svc, &acc);
        if (c) {
            const HAPBaseCharacteristic* chr = c;
            HAPAssert(chr->iid == readContext->iid);
            if (session->eventNotifications) {
                HAPAssert(session->numEventNotifications <= session->maxEventNotifications);
                j = 0;
                while ((j < session->numEventNotifications) &&
                       ((session->eventNotifications[j].aid != readContext->aid) ||
                        (session->eventNotifications[j].iid != readContext->iid))) {
                    j++;
                }
                HAPAssert(
                        (j == session->numEventNotifications) ||
                        ((j < session->numEventNotifications) &&
                         (session->eventNotifications[j].aid == readContext->aid) &&
                         (session->eventNotifications[j].iid == readContext->iid)));
                readContext->ev = j < session->numEventNotifications;
            } else {
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 0)
                HAPAssertionFailure();
#else
                if (session->eventNotificationSubscriptions) {
                    HAPAssert(session->eventNotificationFlags);
                    HAPAccessoryServer* server = session->server;
                    size_t eventNotificationIndex;
                    bool eventNotificationIndexFound;
                    HAPAccessoryServerGetEventNotificationIndex(
                            HAPNonnull(session->server),
                            readContext->aid,
                            readContext->iid,
                            &eventNotificationIndex,
                            &eventNotificationIndexFound);
                    readContext->ev =
                            eventNotificationIndexFound && HAPBitSetContains(
                                                                   HAPNonnull(session->eventNotificationSubscriptions),
                                                                   server->ip.numEventNotificationBitSetBytes,
                                                                   eventNotificationIndex);
                } else {
                    HAPAssert(!session->eventNotificationSubscriptions);
                    HAPAssert(!session->eventNotificationFlags);
                    readContext->ev = false;
                }
#endif
            }
            if (!HAPCharacteristicReadRequiresAdminPermissions(chr) ||
                HAPSessionControllerIsAdmin(&session->securitySession._.hap)) {
                if (chr->properties.readable) {
                    if ((session_context != kHAPIPSessionContext_EventNotification) &&
                        HAPUUIDAreEqual(chr->characteristicType, &kHAPCharacteristicType_ButtonEvent)) {
                        // Workaround for 47267690: Error -70402 when reading from "Button Event" characteristic.
                        // The Button Event characteristic is event only.
                        readContext->status = kHAPIPAccessoryServerStatusCode_Success;
                        readContext->value.stringValue.bytes = "";
                        readContext->value.stringValue.numBytes = 0;
                    } else if (
                            (session_context != kHAPIPSessionContext_EventNotification) &&
                            HAPUUIDAreEqual(chr->characteristicType, &kHAPCharacteristicType_ProgrammableSwitchEvent)) {
                        // A read of this characteristic must always return a null value for IP accessories.
                        // See HomeKit Accessory Protocol Specification R17
                        // Section 11.47 Programmable Switch Event
                        readContext->status = kHAPIPAccessoryServerStatusCode_Success;
                        readContext->value.unsignedIntValue = 0;
                    } else if (
                            (session_context == kHAPIPSessionContext_GetAccessories) &&
                            chr->properties.ip.controlPoint) {
                        readContext->status = kHAPIPAccessoryServerStatusCode_UnableToPerformOperation;
                    } else {
                        handle_characteristic_read_request(session, chr, svc, acc, &contexts[i], data_buffer);
                    }
                } else {
                    readContext->status = kHAPIPAccessoryServerStatusCode_ReadFromWriteOnlyCharacteristic;
                }
            } else {
                readContext->status = kHAPIPAccessoryServerStatusCode_InsufficientPrivileges;
            }
        } else {
            readContext->status = kHAPIPAccessoryServerStatusCode_ResourceDoesNotExist;
        }
        if ((r == 0) && (readContext->status != kHAPIPAccessoryServerStatusCode_Success)) {
            r = -1;
        }
    }
    HAPAssert(i == contexts_count);
    return r;
}

static void get_characteristics(HAPIPSessionDescriptor* session) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPAccessoryServer* server = session->server;
    HAPPrecondition(session->securitySession.type == kHAPIPSecuritySessionType_HAP);
    HAPPrecondition(session->securitySession.isOpen);
    HAPPrecondition(session->securitySession.isSecured || kHAPIPAccessoryServer_SessionSecurityDisabled);
    HAPPrecondition(!HAPSessionIsTransient(&session->securitySession._.hap));

    HAPError err;

    int r;
    size_t contexts_count, content_length, mark;
    HAPIPReadRequestParameters parameters;
    HAPIPByteBuffer data_buffer;

    HAPAssert(session->httpURIIsDefined);
    HAPAssert(
            (session->httpURI.numBytes >= 16) &&
            HAPRawBufferAreEqual(GetHttpURIBytes(session), "/characteristics", 16));
    if ((session->httpURI.numBytes >= 17) && (GetHttpURIBytes(session)[16] == '?')) {
        err = HAPIPAccessoryProtocolGetCharacteristicReadRequests(
                &GetHttpURIBytes(session)[17],
                session->httpURI.numBytes - 17,
                server->ip.storage->readContexts,
                server->ip.storage->numReadContexts,
                &contexts_count,
                &parameters);
        if (!err) {
            if (contexts_count == 0) {
                WriteResponse(session, kHAPIPAccessoryServerResponse_NoContent);
            } else {
                data_buffer.data = server->ip.storage->scratchBuffer.bytes;
                data_buffer.capacity = server->ip.storage->scratchBuffer.numBytes;
                data_buffer.limit = server->ip.storage->scratchBuffer.numBytes;
                data_buffer.position = 0;
                HAPAssert(data_buffer.data);
                HAPAssert(data_buffer.position <= data_buffer.limit);
                HAPAssert(data_buffer.limit <= data_buffer.capacity);
                r = handle_characteristic_read_requests(
                        session,
                        kHAPIPSessionContext_GetCharacteristics,
                        server->ip.storage->readContexts,
                        contexts_count,
                        &data_buffer);

                content_length = HAPIPAccessoryProtocolGetNumCharacteristicReadResponseBytes(
                        HAPNonnull(session->server), server->ip.storage->readContexts, contexts_count, &parameters);

                HAP_DIAGNOSTIC_IGNORED_ICCARM(Pa084)
                if (content_length <= UINT32_MAX) {
                    err = CompleteReadingRequest(session);
                    if (err) {
                        HAPAssert(err == kHAPError_OutOfResources);
                        CloseSession(session);
                        return;
                    }
                    err = PrepareWritingResponse(
                            session, kHAPIPAccessoryServer_MinOutboundBufferBytes + content_length);
                    if (err) {
                        HAPAssert(err == kHAPError_OutOfResources);
                        err = PrepareWritingResponse(session, kHAPIPAccessoryServer_MinOutboundBufferBytes);
                        if (err) {
                            HAPAssert(err == kHAPError_OutOfResources);
                            CloseSession(session);
                            return;
                        }
                        err = HAPIPByteBufferAppendStringWithFormat(
                                &session->outboundBuffer, "%s", kHAPIPAccessoryServerResponse_OutOfResources);
                        if (err) {
                            HAPAssert(err == kHAPError_OutOfResources);
                            HAPAssertionFailure();
                        }
                        return;
                    }
                    mark = session->outboundBuffer.position;
                    if (r == 0) {
                        err = HAPIPByteBufferAppendStringWithFormat(&session->outboundBuffer, "HTTP/1.1 200 OK\r\n");
                    } else {
                        err = HAPIPByteBufferAppendStringWithFormat(
                                &session->outboundBuffer, "HTTP/1.1 207 Multi-Status\r\n");
                    }
                    if (err) {
                        HAPAssert(err == kHAPError_OutOfResources);
                        HAPAssertionFailure();
                    }
                    err = HAPIPByteBufferAppendStringWithFormat(
                            &session->outboundBuffer,
                            "Content-Type: application/hap+json\r\n"
                            "Content-Length: %lu\r\n\r\n",
                            (unsigned long) content_length);
                    if (err) {
                        HAPAssert(err == kHAPError_OutOfResources);
                        HAPAssertionFailure();
                    }
                    if (content_length <= session->outboundBuffer.limit - session->outboundBuffer.position) {
                        mark = session->outboundBuffer.position;
                        err = HAPIPAccessoryProtocolGetCharacteristicReadResponseBytes(
                                HAPNonnull(session->server),
                                server->ip.storage->readContexts,
                                contexts_count,
                                &parameters,
                                &session->outboundBuffer);
                        HAPAssert(!err && (session->outboundBuffer.position - mark == content_length));
                    } else {
                        HAPLog(&logObject, "Out of resources (outbound buffer too small).");
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 1)
                        if (server->ip.storage->dynamicMemoryAllocation.allocateMemory) {
                            HAPFatalError();
                        }
#endif
                        session->outboundBuffer.position = mark;
                        err = HAPIPByteBufferAppendStringWithFormat(
                                &session->outboundBuffer, "%s", kHAPIPAccessoryServerResponse_OutOfResources);
                        if (err) {
                            HAPAssert(err == kHAPError_OutOfResources);
                            HAPAssertionFailure();
                        }
                    }
                } else {
                    HAPLog(&logObject, "Content length exceeding UINT32_MAX.");
                    WriteResponse(session, kHAPIPAccessoryServerResponse_OutOfResources);
                }
                HAP_DIAGNOSTIC_RESTORE_ICCARM(Pa084)
            }
        } else if (err == kHAPError_OutOfResources) {
            WriteResponse(session, kHAPIPAccessoryServerResponse_OutOfResources);
        } else {
            WriteResponse(session, kHAPIPAccessoryServerResponse_BadRequest);
        }
    } else {
        WriteResponse(session, kHAPIPAccessoryServerResponse_BadRequest);
    }
}

static void handle_accessory_serialization(HAPIPSessionDescriptor* session) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPPrecondition(session->securitySession.type == kHAPIPSecuritySessionType_HAP);
    HAPPrecondition(session->securitySession.isOpen);
    HAPPrecondition(session->securitySession.isSecured || kHAPIPAccessoryServer_SessionSecurityDisabled);
    HAPPrecondition(!HAPSessionIsTransient(&session->securitySession._.hap));

    HAPError err;

    HAPAssert(session->outboundBuffer.data);
    HAPAssert(session->outboundBuffer.capacity);

    if (session->accessorySerializationIsInProgress) {
        HAPAssert(session->outboundBuffer.position == session->outboundBuffer.limit);
        if (session->securitySession.isSecured) {
            HAPAssert(session->outboundBuffer.limit <= session->outboundBufferMark);
            HAPAssert(session->outboundBufferMark <= session->outboundBuffer.capacity);
            HAPRawBufferCopyBytes(
                    &session->outboundBuffer.data[0],
                    &session->outboundBuffer.data[session->outboundBuffer.limit],
                    session->outboundBufferMark - session->outboundBuffer.limit);
            session->outboundBuffer.position = session->outboundBufferMark - session->outboundBuffer.limit;
            session->outboundBuffer.limit = session->outboundBuffer.capacity;
            session->outboundBufferMark = 0;
        } else {
            HAPAssert(session->outboundBuffer.limit <= session->outboundBuffer.capacity);
            session->outboundBuffer.position = 0;
            session->outboundBuffer.limit = session->outboundBuffer.capacity;
        }
    }

    HAPAssert(session->outboundBuffer.position <= session->outboundBuffer.limit);
    HAPAssert(session->outboundBuffer.limit <= session->outboundBuffer.capacity);

    if ((session->outboundBuffer.position < session->outboundBuffer.limit) &&
        (session->outboundBuffer.position < kHAPIPSecurityProtocol_MaxFrameBytes) &&
        !HAPIPAccessorySerializationIsComplete(&session->accessorySerializationContext)) {
        size_t minBytes, maxBytes, numBytes, numBytesSerialized;
        numBytesSerialized = 0;
        do {
            HAPAssert(session->outboundBuffer.position < session->outboundBuffer.limit);
            HAPAssert(session->outboundBuffer.position < kHAPIPSecurityProtocol_MaxFrameBytes);
            HAPAssert(!HAPIPAccessorySerializationIsComplete(&session->accessorySerializationContext));

            HAPAssert(numBytesSerialized < session->outboundBuffer.limit - session->outboundBuffer.position);
            HAPAssert(numBytesSerialized < kHAPIPSecurityProtocol_MaxFrameBytes - session->outboundBuffer.position);

            maxBytes = session->outboundBuffer.limit - session->outboundBuffer.position - numBytesSerialized;
            minBytes =
                    HAPMin(kHAPIPSecurityProtocol_MaxFrameBytes - session->outboundBuffer.position - numBytesSerialized,
                           maxBytes);
            err = HAPIPAccessorySerializeReadResponse(
                    &session->accessorySerializationContext,
                    HAPNonnull(session->server),
                    session,
                    &session->outboundBuffer.data[session->outboundBuffer.position],
                    minBytes,
                    maxBytes,
                    &numBytes);
            HAPAssert(numBytes <= maxBytes);
            HAPAssert(
                    ((err == kHAPError_None) &&
                     ((numBytes >= minBytes) ||
                      HAPIPAccessorySerializationIsComplete(&session->accessorySerializationContext))) ||
                    ((err == kHAPError_OutOfResources) &&
                     !((numBytes >= minBytes) ||
                       HAPIPAccessorySerializationIsComplete(&session->accessorySerializationContext))));
            numBytesSerialized += numBytes;
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 0)
                HAPLogError(&logObject, "Invalid configuration (outbound buffer too small).");
                HAPFatalError();
#else
                HAPAccessoryServer* server = session->server;
                if (!server->ip.storage->dynamicMemoryAllocation.reallocateMemory) {
                    HAPLogError(&logObject, "Invalid configuration (outbound buffer too small).");
                    HAPFatalError();
                } else {
                    HAPLogError(&logObject, "Invalid configuration (outbound buffer too small).");
                    HAPFatalError();
                    // In the future, the outbound buffer should be reallocated and the function
                    // HAPIPAccessorySerializeReadResponse should be modified to be safe for retries.
                    // Function HAPIPAccessorySerializeReadResponse would also require a data buffer (backed
                    // by server->ip.storage->scratchBuffer.bytes) as an additional argument to guarantee that every
                    // characteristic value can be read without out of memory errors, which would be serialized in
                    // JSON as "value":null or "value":"".
                    // err = ReallocateOutboundBuffer(session,
                    //     session->outboundBuffer.capacity + kHAPIPAccessoryServer_MaxEncryptedFrameBytes);
                    // if (err) {
                    //     HAPAssert(err == kHAPError_OutOfResources);
                    //     HAPFatalError();
                    // }
                    // session->outboundBuffer.limit = session->outboundBuffer.capacity;
                }
#endif
            }
        } while (err == kHAPError_OutOfResources);

        HAPAssert(session->outboundBuffer.position < session->outboundBuffer.limit);
        HAPAssert(session->outboundBuffer.position < kHAPIPSecurityProtocol_MaxFrameBytes);

        HAPAssert(numBytesSerialized <= session->outboundBuffer.limit - session->outboundBuffer.position);
        HAPAssert(
                (numBytesSerialized >= kHAPIPSecurityProtocol_MaxFrameBytes - session->outboundBuffer.position) ||
                HAPIPAccessorySerializationIsComplete(&session->accessorySerializationContext));

        // maxProtocolBytes = max(8, size_t represented in HEX + '\r' + '\n' + '\0')
        char protocolBytes[HAPMax(8, sizeof(size_t) * 2 + 2 + 1)];

        err = HAPStringWithFormat(protocolBytes, sizeof protocolBytes, "%zX\r\n", numBytesSerialized);
        HAPAssert(!err);
        size_t numProtocolBytes = HAPStringGetNumBytes(protocolBytes);

        while (numProtocolBytes > session->outboundBuffer.limit - session->outboundBuffer.position) {
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 0)
            HAPLogError(&logObject, "Invalid configuration (outbound buffer too small).");
            HAPFatalError();
#else
            HAPAccessoryServer* server = session->server;
            if (!server->ip.storage->dynamicMemoryAllocation.reallocateMemory) {
                HAPLogError(&logObject, "Invalid configuration (outbound buffer too small).");
                HAPFatalError();
            } else {
                err = ReallocateOutboundBuffer(
                        session, session->outboundBuffer.capacity + kHAPIPAccessoryServer_MaxEncryptedFrameBytes);
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    HAPFatalError();
                }
                session->outboundBuffer.limit = session->outboundBuffer.capacity;
            }
#endif
        }
        while (numBytesSerialized >
               session->outboundBuffer.limit - session->outboundBuffer.position - numProtocolBytes) {
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 0)
            HAPLogError(&logObject, "Invalid configuration (outbound buffer too small).");
            HAPFatalError();
#else
            HAPAccessoryServer* server = session->server;
            if (!server->ip.storage->dynamicMemoryAllocation.reallocateMemory) {
                HAPLogError(&logObject, "Invalid configuration (outbound buffer too small).");
                HAPFatalError();
            } else {
                err = ReallocateOutboundBuffer(
                        session, session->outboundBuffer.capacity + kHAPIPAccessoryServer_MaxEncryptedFrameBytes);
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    HAPFatalError();
                }
                session->outboundBuffer.limit = session->outboundBuffer.capacity;
            }
#endif
        }

        HAPRawBufferCopyBytes(
                &session->outboundBuffer.data[session->outboundBuffer.position + numProtocolBytes],
                &session->outboundBuffer.data[session->outboundBuffer.position],
                numBytesSerialized);
        HAPRawBufferCopyBytes(
                &session->outboundBuffer.data[session->outboundBuffer.position], protocolBytes, numProtocolBytes);
        session->outboundBuffer.position += numProtocolBytes + numBytesSerialized;

        if (HAPIPAccessorySerializationIsComplete(&session->accessorySerializationContext)) {
            err = HAPStringWithFormat(protocolBytes, sizeof protocolBytes, "\r\n0\r\n\r\n");
        } else {
            err = HAPStringWithFormat(protocolBytes, sizeof protocolBytes, "\r\n");
        }
        HAPAssert(!err);
        numProtocolBytes = HAPStringGetNumBytes(protocolBytes);

        while (numProtocolBytes > session->outboundBuffer.limit - session->outboundBuffer.position) {
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 0)
            HAPLogError(&logObject, "Invalid configuration (outbound buffer too small).");
            HAPFatalError();
#else
            HAPAccessoryServer* server = session->server;
            if (!server->ip.storage->dynamicMemoryAllocation.reallocateMemory) {
                HAPLogError(&logObject, "Invalid configuration (outbound buffer too small).");
                HAPFatalError();
            } else {
                err = ReallocateOutboundBuffer(
                        session, session->outboundBuffer.capacity + kHAPIPAccessoryServer_MaxEncryptedFrameBytes);
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    HAPFatalError();
                }
                session->outboundBuffer.limit = session->outboundBuffer.capacity;
            }
#endif
        }

        HAPRawBufferCopyBytes(
                &session->outboundBuffer.data[session->outboundBuffer.position], protocolBytes, numProtocolBytes);
        session->outboundBuffer.position += numProtocolBytes;
    }

    if (session->outboundBuffer.position > 0) {
        HAPIPByteBufferFlip(&session->outboundBuffer);

        if (session->securitySession.isSecured) {
            size_t numFrameBytes = kHAPIPSecurityProtocol_MaxFrameBytes <
                                                   session->outboundBuffer.limit - session->outboundBuffer.position ?
                                           kHAPIPSecurityProtocol_MaxFrameBytes :
                                           session->outboundBuffer.limit - session->outboundBuffer.position;

            HAPLogBufferDebug(
                    &logObject,
                    &session->outboundBuffer.data[session->outboundBuffer.position],
                    numFrameBytes,
                    "session:%p:<",
                    (const void*) session);

            size_t numUnencryptedBytes =
                    session->outboundBuffer.limit - session->outboundBuffer.position - numFrameBytes;

            size_t numEncryptedBytes = HAPIPSecurityProtocolGetNumEncryptedBytes(numFrameBytes);

            while (numEncryptedBytes >
                   session->outboundBuffer.capacity - session->outboundBuffer.position - numUnencryptedBytes) {
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 0)
                HAPLogError(&logObject, "Invalid configuration (outbound buffer too small).");
                HAPFatalError();
#else
                HAPAccessoryServer* server = session->server;
                if (!server->ip.storage->dynamicMemoryAllocation.reallocateMemory) {
                    HAPLogError(&logObject, "Invalid configuration (outbound buffer too small).");
                    HAPFatalError();
                } else {
                    err = ReallocateOutboundBuffer(
                            session, session->outboundBuffer.capacity + kHAPIPAccessoryServer_MaxEncryptedFrameBytes);
                    if (err) {
                        HAPAssert(err == kHAPError_OutOfResources);
                        HAPFatalError();
                    }
                }
#endif
            }

            HAPRawBufferCopyBytes(
                    &session->outboundBuffer.data[session->outboundBuffer.position + numEncryptedBytes],
                    &session->outboundBuffer.data[session->outboundBuffer.position + numFrameBytes],
                    numUnencryptedBytes);

            session->outboundBuffer.limit = session->outboundBuffer.position + numFrameBytes;

            HAPIPSecurityProtocolEncryptData(
                    HAPNonnull(session->server), &session->securitySession._.hap, &session->outboundBuffer);
            HAPAssert(numEncryptedBytes == session->outboundBuffer.limit - session->outboundBuffer.position);

            session->outboundBufferMark = session->outboundBuffer.limit + numUnencryptedBytes;
        } else {
            HAPLogBufferDebug(
                    &logObject,
                    &session->outboundBuffer.data[session->outboundBuffer.position],
                    session->outboundBuffer.limit - session->outboundBuffer.position,
                    "session:%p:<",
                    (const void*) session);
        }

        session->state = kHAPIPSessionState_Writing;

        session->accessorySerializationIsInProgress = true;
    } else {
        session->accessorySerializationIsInProgress = false;

        session->state = kHAPIPSessionState_Reading;
        PrepareReadingRequest(session);
        if (session->inboundBuffer.position != 0) {
            handle_input(session);
        }
    }
}

static void get_accessories(HAPIPSessionDescriptor* session) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPPrecondition(session->securitySession.type == kHAPIPSecuritySessionType_HAP);
    HAPPrecondition(session->securitySession.isOpen);
    HAPPrecondition(session->securitySession.isSecured || kHAPIPAccessoryServer_SessionSecurityDisabled);
    HAPPrecondition(!HAPSessionIsTransient(&session->securitySession._.hap));
    HAPPrecondition(!session->accessorySerializationIsInProgress);

    HAPError err;

    err = CompleteReadingRequest(session);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        CloseSession(session);
        return;
    }

    // IP accessory serialization requires an outbound buffer length of at least
    //
    //     kHAPIPSecurityProtocol_MaxFrameBytes + maximum length of all attribute DB values when represented in JSON
    //
    // As a heuristic, we reserve here a buffer length of at least 2 * kHAPIPSecurityProtocol_MaxFrameBytes which should
    // be sufficient for many accessories. If the given session is pre-configured with a larger outbound buffer, the
    // entire preallocated space will be used, otherwise function handle_accessory_serialization will attempt to
    // reallocate the available outbound buffer space if needed.

    err = PrepareWritingResponse(session, 2 * kHAPIPSecurityProtocol_MaxFrameBytes);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        err = PrepareWritingResponse(session, kHAPIPAccessoryServer_MinOutboundBufferBytes);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            CloseSession(session);
            return;
        }
        err = HAPIPByteBufferAppendStringWithFormat(
                &session->outboundBuffer, "%s", kHAPIPAccessoryServerResponse_OutOfResources);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            HAPAssertionFailure();
        }
        return;
    }
    err = HAPIPByteBufferAppendStringWithFormat(
            &session->outboundBuffer,
            "HTTP/1.1 200 OK\r\n"
            "Transfer-Encoding: chunked\r\n"
            "Content-Type: application/hap+json\r\n\r\n");
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPAssertionFailure();
    }

    HAPIPAccessoryCreateSerializationContext(&session->accessorySerializationContext);
    handle_accessory_serialization(session);
}

static void handle_pairing_data(
        HAPIPSessionDescriptor* session,
        HAPError (*write_hap_pairing_data)(HAPAccessoryServer* p_acc, HAPSession* p_sess, HAPTLVReader* p_reader),
        HAPError (*read_hap_pairing_data)(HAPAccessoryServer* p_acc, HAPSession* p_sess, HAPTLVWriter* p_writer)) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPAccessoryServer* server = session->server;
    HAPPrecondition(session->securitySession.type == kHAPIPSecuritySessionType_HAP);
    HAPPrecondition(session->securitySession.isOpen);

    HAPError err;

    int r;
    bool pairing_status;
    uint8_t* p_tlv8_buffer;
    size_t tlv8_length, mark;
    HAPTLVReaderOptions tlv8_reader_init;
    HAPTLVReader tlv8_reader;
    HAPTLVWriter tlv8_writer;

    char* scratchBuffer = server->ip.storage->scratchBuffer.bytes;
    size_t maxScratchBufferBytes = server->ip.storage->scratchBuffer.numBytes;

    HAPAssert(session->inboundBuffer.data);
    HAPAssert(session->inboundBuffer.position <= session->inboundBuffer.limit);
    HAPAssert(session->inboundBuffer.limit <= session->inboundBuffer.capacity);
    HAPAssert(session->httpReaderPosition <= session->inboundBuffer.position);
    HAPAssert(write_hap_pairing_data);
    HAPAssert(read_hap_pairing_data);
    pairing_status = HAPAccessoryServerIsPaired(HAPNonnull(session->server));
    if (session->httpContentLengthIsDefined) {
        HAPAssert(session->httpContentLength <= session->inboundBuffer.position - session->httpReaderPosition);
        if (session->httpContentLength <= maxScratchBufferBytes) {
            HAPRawBufferCopyBytes(
                    scratchBuffer,
                    &session->inboundBuffer.data[session->httpReaderPosition],
                    session->httpContentLength);
            tlv8_reader_init.bytes = scratchBuffer;
            tlv8_reader_init.numBytes = session->httpContentLength;
            tlv8_reader_init.maxBytes = maxScratchBufferBytes;
            HAPTLVReaderCreateWithOptions(&tlv8_reader, &tlv8_reader_init);
            r = write_hap_pairing_data(HAPNonnull(session->server), &session->securitySession._.hap, &tlv8_reader);
            if (r == 0) {
                HAPTLVWriterCreate(&tlv8_writer, scratchBuffer, maxScratchBufferBytes);
                r = read_hap_pairing_data(HAPNonnull(session->server), &session->securitySession._.hap, &tlv8_writer);
                if (r == 0) {
                    HAPTLVWriterGetBuffer(&tlv8_writer, (void*) &p_tlv8_buffer, &tlv8_length);
                    if (HAPAccessoryServerIsPaired(HAPNonnull(session->server)) != pairing_status) {
                        HAPIPServiceDiscoverySetHAPService(HAPNonnull(session->server));
                    }
                    HAP_DIAGNOSTIC_IGNORED_ICCARM(Pa084)
                    if (tlv8_length <= UINT32_MAX) {
                        err = CompleteReadingRequest(session);
                        if (err) {
                            HAPAssert(err == kHAPError_OutOfResources);
                            CloseSession(session);
                            return;
                        }
                        err = PrepareWritingResponse(
                                session, kHAPIPAccessoryServer_MinOutboundBufferBytes + tlv8_length);
                        if (err) {
                            HAPAssert(err == kHAPError_OutOfResources);
                            err = PrepareWritingResponse(session, kHAPIPAccessoryServer_MinOutboundBufferBytes);
                            if (err) {
                                HAPAssert(err == kHAPError_OutOfResources);
                                CloseSession(session);
                                return;
                            }
                            err = HAPIPByteBufferAppendStringWithFormat(
                                    &session->outboundBuffer, "%s", kHAPIPAccessoryServerResponse_InternalServerError);
                            if (err) {
                                HAPAssert(err == kHAPError_OutOfResources);
                                HAPAssertionFailure();
                            }
                            return;
                        }
                        mark = session->outboundBuffer.position;
                        err = HAPIPByteBufferAppendStringWithFormat(
                                &session->outboundBuffer,
                                "HTTP/1.1 200 OK\r\n"
                                "Content-Type: application/pairing+tlv8\r\n"
                                "Content-Length: %lu\r\n\r\n",
                                (unsigned long) tlv8_length);
                        if (err) {
                            HAPAssert(err == kHAPError_OutOfResources);
                            HAPAssertionFailure();
                        }
                        if (tlv8_length <= session->outboundBuffer.limit - session->outboundBuffer.position) {
                            HAPRawBufferCopyBytes(
                                    &session->outboundBuffer.data[session->outboundBuffer.position],
                                    p_tlv8_buffer,
                                    tlv8_length);
                            session->outboundBuffer.position += tlv8_length;
                            for (size_t i = 0; i < server->ip.storage->numSessions; i++) {
                                HAPIPSession* ipSession = &server->ip.storage->sessions[i];
                                HAPIPSessionDescriptor* t = &ipSession->descriptor;
                                if (!t->server) {
                                    continue;
                                }

                                // Other sessions whose pairing has been removed during the pairing session
                                // need to be closed as soon as possible.
                                if (t != session && t->state == kHAPIPSessionState_Reading &&
                                    t->securitySession.type == kHAPIPSecuritySessionType_HAP &&
                                    t->securitySession.isSecured && !HAPSessionIsSecured(&t->securitySession._.hap)) {
                                    HAPLogInfo(&logObject, "Closing other session whose pairing has been removed.");
                                    CloseSession(t);
                                }
                            }
                        } else {
                            HAPLog(&logObject, "Invalid configuration (outbound buffer too small).");
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 1)
                            if (server->ip.storage->dynamicMemoryAllocation.allocateMemory) {
                                HAPFatalError();
                            }
#endif
                            session->outboundBuffer.position = mark;
                            err = HAPIPByteBufferAppendStringWithFormat(
                                    &session->outboundBuffer, "%s", kHAPIPAccessoryServerResponse_InternalServerError);
                            if (err) {
                                HAPAssert(err == kHAPError_OutOfResources);
                                HAPAssertionFailure();
                            }
                        }
                        HAP_DIAGNOSTIC_RESTORE_ICCARM(Pa084)
                    } else {
                        HAPLog(&logObject, "Content length exceeding UINT32_MAX.");
                        WriteResponse(session, kHAPIPAccessoryServerResponse_InternalServerError);
                    }
                } else {
                    log_result(
                            kHAPLogType_Error,
                            "error:Function 'read_hap_pairing_data' failed.",
                            r,
                            __func__,
                            HAP_FILE,
                            __LINE__);
                    WriteResponse(session, kHAPIPAccessoryServerResponse_InternalServerError);
                }
            } else {
                WriteResponse(session, kHAPIPAccessoryServerResponse_BadRequest);
            }
        } else {
            HAPLog(&logObject, "Invalid configuration (inbound buffer too small).");
            WriteResponse(session, kHAPIPAccessoryServerResponse_InternalServerError);
        }
    } else {
        WriteResponse(session, kHAPIPAccessoryServerResponse_BadRequest);
    }
}

/**
 * Handles a POST request on the /secure-message endpoint.
 *
 * - Session has already been validated to be secured.
 *
 * @param      session              IP session descriptor.
 */
static void handle_secure_message(HAPIPSessionDescriptor* session) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPAccessoryServer* server = session->server;
    HAPPrecondition(server->primaryAccessory);
    HAPPrecondition(session->securitySession.type == kHAPIPSecuritySessionType_HAP);
    HAPPrecondition(session->securitySession.isOpen);
    HAPSession* hapSession = &session->securitySession._.hap;
    HAPPrecondition(session->securitySession.isSecured || kHAPIPAccessoryServer_SessionSecurityDisabled);
    HAPPrecondition(session->inboundBuffer.data);
    HAPPrecondition(session->inboundBuffer.position <= session->inboundBuffer.limit);
    HAPPrecondition(session->inboundBuffer.limit <= session->inboundBuffer.capacity);
    HAPPrecondition(session->httpReaderPosition <= session->inboundBuffer.position);

    HAPError err;

    // Validate request.
    // Requests use the HAP PDU format.
    // See HomeKit Accessory Protocol Specification R17
    // Section 7.3.3 HAP PDU Format
    if (session->httpContentType != kHAPIPAccessoryServerContentType_Application_OctetStream) {
        HAPLog(&logObject, "Received unexpected Content-Type in /secure-message request.");
        WriteResponse(session, kHAPIPAccessoryServerResponse_BadRequest);
        return;
    }
    if (!session->httpContentLengthIsDefined) {
        HAPLog(&logObject, "Received malformed /secure-message request (no content length).");
        WriteResponse(session, kHAPIPAccessoryServerResponse_BadRequest);
        return;
    }
    HAPAssert(session->httpContentLength <= session->inboundBuffer.position - session->httpReaderPosition);
    uint8_t* requestBytes = (uint8_t*) &session->inboundBuffer.data[session->httpReaderPosition];
    size_t numRequestBytes = session->httpContentLength;
    if (numRequestBytes < 5) {
        HAPLog(&logObject, "Received too short /secure-message request.");
        WriteResponse(session, kHAPIPAccessoryServerResponse_BadRequest);
        return;
    }
    if (requestBytes[0] != ((0U << 7U) | (0U << 4U) | (0U << 3U) | (0U << 2U) | (0U << 1U) | (0U << 0U))) {
        HAPLog(&logObject, "Received malformed /secure-message request (control field: 0x%02x).", requestBytes[0]);
        WriteResponse(session, kHAPIPAccessoryServerResponse_BadRequest);
        return;
    }
    uint8_t opcode = requestBytes[1];
    uint8_t tid = requestBytes[2];
    uint16_t iid = HAPReadLittleUInt16(&requestBytes[3]);
    HAPTLVReader requestBodyReader;
    if (numRequestBytes <= 5) {
        HAPAssert(numRequestBytes == 5);
        HAPTLVReaderCreate(&requestBodyReader, NULL, 0);
    } else {
        if (numRequestBytes < 7) {
            HAPLog(&logObject, "Received malformed /secure-message request (malformed body length).");
            WriteResponse(session, kHAPIPAccessoryServerResponse_BadRequest);
            return;
        }
        uint16_t numRequestBodyBytes = HAPReadLittleUInt16(&requestBytes[5]);
        if (numRequestBytes - 7 != numRequestBodyBytes) {
            HAPLog(&logObject, "Received malformed /secure-message request (incorrect body length).");
            WriteResponse(session, kHAPIPAccessoryServerResponse_BadRequest);
            return;
        }
        HAPTLVReaderCreate(&requestBodyReader, &requestBytes[7], numRequestBodyBytes);
    }

    // Response variables.
    HAPPDUStatus status;
    void* _Nullable responseBodyBytes = NULL;
    size_t numResponseBodyBytes = 0;

    // Validate opcode.
    if (!HAPPDUOpcodeIsValid(opcode)) {
        // If an accessory receives a HAP PDU with an opcode that it does not support it shall reject the PDU and
        // respond with a status code Unsupported PDU in its HAP response.
        // See HomeKit Accessory Protocol Specification R17
        // Section 7.3.3.2 HAP Request Format
        HAPLogAccessory(
                &logObject,
                server->primaryAccessory,
                "Rejected /secure-message request with unsupported opcode: 0x%02x.",
                opcode);
        status = kHAPPDUStatus_UnsupportedPDU;
        goto SendResponse;
    }
    if (!HAPPDUOpcodeIsSupportedForTransport(opcode, hapSession->transportType)) {
        HAPLogAccessory(
                &logObject,
                server->primaryAccessory,
                "Rejected /secure-message request with opcode that is not supported by IP: 0x%02x.",
                opcode);
        status = kHAPPDUStatus_UnsupportedPDU;
        goto SendResponse;
    }

    // Validate iid.
    // For IP accessories instance ID in the request shall be set to 0.
    // See HomeKit Accessory Protocol Specification R17
    // Section 5.17 Software Authentication Procedure
    if (iid) {
        HAPLogAccessory(
                &logObject,
                server->primaryAccessory,
                "Request's IID [00000000%08X] does not match the addressed IID.",
                iid);
        status = kHAPPDUStatus_InvalidInstanceID;
        goto SendResponse;
    }

#define DestroyRequestBodyAndCreateResponseBodyWriter(responseWriter) \
    do { \
        size_t numBytes = server->ip.storage->scratchBuffer.numBytes; \
        if (numBytes > UINT16_MAX) { \
            /* Maximum for HAP-BLE PDU. */ \
            numBytes = UINT16_MAX; \
        } \
        HAPTLVWriterCreate(responseWriter, server->ip.storage->scratchBuffer.bytes, numBytes); \
    } while (0)

    // Handle request.
    HAPAssert(sizeof opcode == sizeof(HAPPDUOpcode));
    switch ((HAPPDUOpcode) opcode) {
        case kHAPPDUOpcode_ServiceSignatureRead:
        case kHAPPDUOpcode_CharacteristicSignatureRead:
        case kHAPPDUOpcode_CharacteristicConfiguration:
        case kHAPPDUOpcode_ProtocolConfiguration:
        case kHAPPDUOpcode_CharacteristicTimedWrite:
        case kHAPPDUOpcode_CharacteristicExecuteWrite:
        case kHAPPDUOpcode_CharacteristicWrite:
        case kHAPPDUOpcode_CharacteristicRead:
        case kHAPPDUOpcode_AccessorySignatureRead:
        case kHAPPDUOpcode_NotificationConfigurationRead:
        case kHAPPDUOpcode_NotificationRegister:
        case kHAPPDUOpcode_NotificationDeregister: {
            HAPLogAccessory(
                    &logObject,
                    server->primaryAccessory,
                    "Rejected /secure-message request with opcode that is not supported by IP: 0x%02x.",
                    opcode);
            status = kHAPPDUStatus_UnsupportedPDU;
        }
            goto SendResponse;
        case kHAPPDUOpcode_Token: {
            // See HomeKit Accessory Protocol Specification R17
            // Section 5.16.1 HAP-Token-Request
            HAPAssert(!iid);
            HAPAssert(session->securitySession.isSecured || kHAPIPAccessoryServer_SessionSecurityDisabled);

            // HAP-Token-Request ok.
            HAPTLVWriter writer;
            DestroyRequestBodyAndCreateResponseBodyWriter(&writer);

#if HAP_FEATURE_ENABLED(HAP_FEATURE_MFI_TOKEN_AUTH)
            // Serialize HAP-Token-Response.
            err = HAPMFiTokenAuthGetTokenResponse(
                    HAPNonnull(session->server), hapSession, HAPNonnull(server->primaryAccessory), &writer);
            if (err) {
                HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources);
                HAPLogAccessory(
                        &logObject,
                        server->primaryAccessory,
                        "Rejected token request: Request handling failed with error %u.",
                        err);
                status = kHAPPDUStatus_InvalidRequest;
                goto SendResponse;
            }
            HAPTLVWriterGetBuffer(&writer, &responseBodyBytes, &numResponseBodyBytes);
            status = kHAPPDUStatus_Success;
            goto SendResponse;
#else
            HAPLogAccessory(&logObject, server->primaryAccessory, "Rejected token request: mfi token auth unsupported");
            status = kHAPPDUStatus_InvalidRequest;
            goto SendResponse;
#endif
        }
        case kHAPPDUOpcode_TokenUpdate: {
            // See HomeKit Accessory Protocol Specification R17
            // Section 5.16.3 HAP-Token-Update-Request
            HAPAssert(!iid);
            HAPAssert(session->securitySession.isSecured || kHAPIPAccessoryServer_SessionSecurityDisabled);

#if HAP_FEATURE_ENABLED(HAP_FEATURE_MFI_TOKEN_AUTH)
            // Handle HAP-Token-Update-Request.
            err = HAPMFiTokenAuthHandleTokenUpdateRequest(
                    HAPNonnull(session->server), hapSession, HAPNonnull(server->primaryAccessory), &requestBodyReader);
            if (err) {
                HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidData);
                HAPLogAccessory(
                        &logObject,
                        server->primaryAccessory,
                        "Rejected token update request: Request handling failed with error %u.",
                        err);
                status = kHAPPDUStatus_InvalidRequest;
                goto SendResponse;
            }

            // Send HAP-Token-Update-Response.
            status = kHAPPDUStatus_Success;
            goto SendResponse;
#else
            HAPLogAccessory(
                    &logObject, server->primaryAccessory, "Rejected token update request: mfi token auth unsupported");
            status = kHAPPDUStatus_InvalidRequest;
            goto SendResponse;
#endif
        }
        case kHAPPDUOpcode_Info: {
            // See HomeKit Accessory Protocol Specification R17
            // Section 5.16.5 HAP-Info-Request
            HAPAssert(!iid);
            HAPAssert(session->securitySession.isSecured || kHAPIPAccessoryServer_SessionSecurityDisabled);

            // HAP-Info-Request ok.
            HAPTLVWriter writer;
            DestroyRequestBodyAndCreateResponseBodyWriter(&writer);

            // Serialize HAP-Info-Response.
            err = HAPAccessoryGetInfoResponse(
                    HAPNonnull(session->server), hapSession, HAPNonnull(server->primaryAccessory), &writer);
            if (err) {
                HAPAssert(err == kHAPError_Unknown || err == kHAPError_OutOfResources);
                HAPLogAccessory(
                        &logObject,
                        server->primaryAccessory,
                        "Rejected info request: Request handling failed with error %u.",
                        err);
                status = kHAPPDUStatus_InvalidRequest;
                goto SendResponse;
            }
            HAPTLVWriterGetBuffer(&writer, &responseBodyBytes, &numResponseBodyBytes);
            status = kHAPPDUStatus_Success;
        }
            goto SendResponse;
        default:
            HAPFatalError();
    }

#undef DestroyRequestBodyAndCreateResponseBodyWriter

SendResponse : {
    // Serialize response.
    // Responses use the HAP PDU format.
    // See HomeKit Accessory Protocol Specification R17
    // Section 7.3.3 HAP PDU Format
    size_t numResponseBytes = 3;
    if (responseBodyBytes) {
        numResponseBytes += 2;
        numResponseBytes += numResponseBodyBytes;
    }
    HAP_DIAGNOSTIC_IGNORED_ICCARM(Pa084)
    if (numResponseBytes > UINT32_MAX) {
        HAPLog(&logObject, "/secure-message response: Content length exceeds UINT32_MAX.");
        WriteResponse(session, kHAPIPAccessoryServerResponse_OutOfResources);
        return;
    }
    HAP_DIAGNOSTIC_RESTORE_ICCARM(Pa084)
    err = CompleteReadingRequest(session);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        CloseSession(session);
        return;
    }
    err = PrepareWritingResponse(session, kHAPIPAccessoryServer_MinOutboundBufferBytes + numResponseBytes);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        err = PrepareWritingResponse(session, kHAPIPAccessoryServer_MinOutboundBufferBytes);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            CloseSession(session);
            return;
        }
        err = HAPIPByteBufferAppendStringWithFormat(
                &session->outboundBuffer, "%s", kHAPIPAccessoryServerResponse_OutOfResources);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            HAPAssertionFailure();
        }
        return;
    }
    size_t mark = session->outboundBuffer.position;
    const char* contentType = "application/octet-stream";
    err = HAPIPByteBufferAppendStringWithFormat(
            &session->outboundBuffer,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %lu\r\n\r\n",
            contentType,
            (unsigned long) numResponseBytes);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPLog(&logObject, "/secure-message response: Invalid configuration (outbound buffer too small for headers).");
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 1)
        if (server->ip.storage->dynamicMemoryAllocation.allocateMemory) {
            HAPFatalError();
        }
#endif
        session->outboundBuffer.position = mark;
        err = HAPIPByteBufferAppendStringWithFormat(
                &session->outboundBuffer, "%s", kHAPIPAccessoryServerResponse_InternalServerError);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            HAPAssertionFailure();
        }
        return;
    }
    if (numResponseBytes > session->outboundBuffer.limit - session->outboundBuffer.position) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPLog(&logObject, "/secure-message response: Invalid configuration (outbound buffer too small for body).");
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 1)
        if (server->ip.storage->dynamicMemoryAllocation.allocateMemory) {
            HAPFatalError();
        }
#endif
        session->outboundBuffer.position = mark;
        err = HAPIPByteBufferAppendStringWithFormat(
                &session->outboundBuffer, "%s", kHAPIPAccessoryServerResponse_InternalServerError);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            HAPAssertionFailure();
        }
        return;
    }
    session->outboundBuffer.data[session->outboundBuffer.position++] =
            (0U << 7U) | (0U << 4U) | (0U << 3U) | (0U << 2U) | (1U << 1U) | (0U << 0U);
    session->outboundBuffer.data[session->outboundBuffer.position++] = (char) tid;
    session->outboundBuffer.data[session->outboundBuffer.position++] = (char) status;
    if (responseBodyBytes) {
        HAPWriteLittleUInt16(&session->outboundBuffer.data[session->outboundBuffer.position], numResponseBodyBytes);
        session->outboundBuffer.position += 2;

        HAPRawBufferCopyBytes(
                &session->outboundBuffer.data[session->outboundBuffer.position],
                HAPNonnullVoid(responseBodyBytes),
                numResponseBodyBytes);
        session->outboundBuffer.position += numResponseBodyBytes;
    }
    HAPAssert(session->outboundBuffer.limit >= session->outboundBuffer.position);
}
}

static void identify_primary_accessory(HAPIPSessionDescriptor* session) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPAccessoryServer* server = session->server;
    HAPPrecondition(server->primaryAccessory);
    HAPPrecondition(server->primaryAccessory->aid == kHAPIPAccessoryProtocolAID_PrimaryAccessory);
    HAPPrecondition(session->securitySession.type == kHAPIPSecuritySessionType_HAP);
    HAPPrecondition(session->securitySession.isOpen);
    HAPPrecondition(!session->securitySession.isSecured);

    HAPError err;

    const HAPService* service = NULL;
    for (size_t i = 0; server->primaryAccessory->services[i]; i++) {
        const HAPService* s = server->primaryAccessory->services[i];
        if ((s->iid == kHAPIPAccessoryProtocolIID_AccessoryInformation) &&
            HAPUUIDAreEqual(s->serviceType, &kHAPServiceType_AccessoryInformation)) {
            service = s;
            break;
        }
    }
    if (service) {
        const HAPBaseCharacteristic* characteristic = NULL;
        for (size_t i = 0; service->characteristics[i]; i++) {
            const HAPBaseCharacteristic* c = service->characteristics[i];
            if (HAPUUIDAreEqual(c->characteristicType, &kHAPCharacteristicType_Identify) &&
                (c->format == kHAPCharacteristicFormat_Bool) && c->properties.writable) {
                characteristic = c;
                break;
            }
        }
        if (characteristic) {
            err = HAPBoolCharacteristicHandleWrite(
                    HAPNonnull(session->server),
                    &(const HAPBoolCharacteristicWriteRequest) {
                            .transportType = kHAPTransportType_IP,
                            .session = &session->securitySession._.hap,
                            .characteristic = (const HAPBoolCharacteristic*) characteristic,
                            .service = service,
                            .accessory = HAPNonnull(server->primaryAccessory),
                            .remote = false,
                            .authorizationData = { .bytes = NULL, .numBytes = 0 } },
                    true,
                    HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
            if (err) {
                HAPAssert(
                        err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                        err == kHAPError_OutOfResources || err == kHAPError_NotAuthorized || err == kHAPError_Busy);
                HAPLog(&logObject, "Identify failed: %u.", err);
            }
        }
    }

    WriteResponse(session, kHAPIPAccessoryServerResponse_NoContent);
}

static void handle_wac_msg(HAPIPSessionDescriptor* session, HAPWACEngineHandler _handle_wac_msg) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPAccessoryServer* server = session->server;
    HAPPrecondition(session->securitySession.type == kHAPIPSecuritySessionType_HAP);
    HAPPrecondition(session->securitySession.isOpen);

    HAPError err;

    size_t data_length, mark;

    HAPAssert(server->ip.isInWACMode);
    HAPAssert(session->inboundBuffer.data);
    HAPAssert(session->inboundBuffer.position <= session->inboundBuffer.limit);
    HAPAssert(session->inboundBuffer.limit <= session->inboundBuffer.capacity);
    HAPAssert(session->httpReaderPosition <= session->inboundBuffer.position);

    if (server->ip.wacModeTimer) {
        if (session->httpContentLengthIsDefined) {
            HAPAssert(session->httpContentLength <= session->inboundBuffer.position - session->httpReaderPosition);
            session->outboundBuffer.position = 0;
            err = _handle_wac_msg(
                    HAPNonnull(session->server),
                    &session->securitySession,
                    &session->inboundBuffer.data[session->httpReaderPosition],
                    session->httpContentLength,
                    server->ip.storage->scratchBuffer.bytes,
                    server->ip.storage->scratchBuffer.numBytes,
                    &data_length);
            if (err) {
                WriteResponse(session, kHAPIPAccessoryServerResponse_BadRequest);
            } else {
                HAPPlatformTimerDeregister(server->ip.wacModeTimer);
                if (session->securitySession.receivedConfig) {
                    HAPLogInfo(
                            &logObject,
                            "Waiting for sending of Wi-Fi configuration response and FIN (timeout: %llu seconds).",
                            (unsigned long long) (kHAPIPAccessoryServer_MaxWACModeWaitingForFINTime / HAPSecond));
                    err = HAPPlatformTimerRegister(
                            &server->ip.wacModeTimer,
                            HAPPlatformClockGetCurrent() + kHAPIPAccessoryServer_MaxWACModeWaitingForFINTime,
                            handle_wac_mode_timer,
                            session->server);
                    if (err) {
                        HAPLog(&logObject, "Not enough resources to start Wi-Fi configuration step timer!");
                        HAPFatalError();
                    }
                } else {
                    HAPLogInfo(
                            &logObject,
                            "Waiting for next Wi-Fi configuration step (timeout: %llu seconds).",
                            (unsigned long long) (kHAPIPAccessoryServer_MaxWACModeStepTime / HAPSecond));
                    err = HAPPlatformTimerRegister(
                            &server->ip.wacModeTimer,
                            HAPPlatformClockGetCurrent() + kHAPIPAccessoryServer_MaxWACModeStepTime,
                            handle_wac_mode_timer,
                            session->server);
                    if (err) {
                        HAPLog(&logObject, "Not enough resources to start Wi-Fi configuration step timer!");
                        HAPFatalError();
                    }
                }

                HAP_DIAGNOSTIC_IGNORED_ICCARM(Pa084)
                if (data_length <= UINT32_MAX) {
                    err = CompleteReadingRequest(session);
                    if (err) {
                        HAPAssert(err == kHAPError_OutOfResources);
                        CloseSession(session);
                        return;
                    }
                    err = PrepareWritingResponse(session, kHAPIPAccessoryServer_MinOutboundBufferBytes + data_length);
                    if (err) {
                        HAPAssert(err == kHAPError_OutOfResources);
                        err = PrepareWritingResponse(session, kHAPIPAccessoryServer_MinOutboundBufferBytes);
                        if (err) {
                            HAPAssert(err == kHAPError_OutOfResources);
                            CloseSession(session);
                            return;
                        }
                        err = HAPIPByteBufferAppendStringWithFormat(
                                &session->outboundBuffer, "%s", kHAPIPAccessoryServerResponse_InternalServerError);
                        if (err) {
                            HAPAssert(err == kHAPError_OutOfResources);
                            HAPAssertionFailure();
                        }
                        return;
                    }
                    mark = session->outboundBuffer.position;
                    err = HAPIPByteBufferAppendStringWithFormat(
                            &session->outboundBuffer,
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Type: application/octet-stream\r\n"
                            "Content-Length: %lu\r\n\r\n",
                            (unsigned long) data_length);
                    if (err) {
                        HAPAssert(err == kHAPError_OutOfResources);
                        HAPAssertionFailure();
                    }
                    if (data_length <= session->outboundBuffer.limit - session->outboundBuffer.position) {
                        HAPRawBufferCopyBytes(
                                &session->outboundBuffer.data[session->outboundBuffer.position],
                                server->ip.storage->scratchBuffer.bytes,
                                data_length);
                        session->outboundBuffer.position += data_length;
                    } else {
                        HAPLog(&logObject, "Invalid configuration (outbound buffer too small).");
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 1)
                        if (server->ip.storage->dynamicMemoryAllocation.allocateMemory) {
                            HAPFatalError();
                        }
#endif
                        session->outboundBuffer.position = mark;
                        err = HAPIPByteBufferAppendStringWithFormat(
                                &session->outboundBuffer, "%s", kHAPIPAccessoryServerResponse_InternalServerError);
                        if (err) {
                            HAPAssert(err == kHAPError_OutOfResources);
                            HAPAssertionFailure();
                        }
                    }
                } else {
                    HAPLog(&logObject, "WAC Content length exceeding UINT32_MAX.");
                    WriteResponse(session, kHAPIPAccessoryServerResponse_InternalServerError);
                }
                HAP_DIAGNOSTIC_RESTORE_ICCARM(Pa084)
            }
        } else {
            WriteResponse(session, kHAPIPAccessoryServerResponse_BadRequest);
        }
    } else {
        HAPLog(&logObject, "Ignoring received WAC request after WAC timeout.");
        WriteResponse(session, kHAPIPAccessoryServerResponse_ResourceNotFound);
    }
}

static void handle_http_request(HAPIPSessionDescriptor* session) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPAccessoryServer* server = session->server;
    HAPPrecondition(session->securitySession.isOpen);

    HAPAssert(session->httpReader.state == util_HTTP_READER_STATE_DONE);
    HAPAssert(!session->httpParserError);

    // /configured comes when we are not in WAC mode and on the _hap._tcp connection
    if ((session->httpURI.numBytes == 11) && HAPRawBufferAreEqual(GetHttpURIBytes(session), "/configured", 11)) {
        HAPPrecondition(session->securitySession.type == kHAPIPSecuritySessionType_HAP);
        if ((session->httpMethod.numBytes == 4) && HAPRawBufferAreEqual(GetHttpMethodBytes(session), "POST", 4)) {
            HAPLogDebug(&logObject, "/configured.");

            if (server->ip.wac.softwareAccessPointIsActive) {
                HAPLog(&logObject,
                       "Rejecting /configured message because it is not supported on the Software Access Point.");
                WriteResponse(session, kHAPIPAccessoryServerResponse_NotAllowedInCurrentState);
                return;
            }

            session->securitySession.receivedConfigured = true;
            WriteResponse(session, kHAPIPAccessoryServerResponse_WACSuccess);
            return;
        }
    } else if ((session->httpURI.numBytes == 11) && HAPRawBufferAreEqual(GetHttpURIBytes(session), "/auth-setup", 11)) {
        if ((session->httpMethod.numBytes == 4) && HAPRawBufferAreEqual(GetHttpMethodBytes(session), "POST", 4)) {
            HAPLog(&logObject, "Rejected POST /auth-setup: Legacy WAC mode is not active.");
            return;
        } else {
            WriteResponse(session, kHAPIPAccessoryServerResponse_MethodNotAllowed);
            return;
        }
    } else {
        if ((session->httpURI.numBytes == 9) && HAPRawBufferAreEqual(GetHttpURIBytes(session), "/identify", 9)) {
            if ((session->httpMethod.numBytes == 4) && HAPRawBufferAreEqual(GetHttpMethodBytes(session), "POST", 4)) {
                if (!HAPAccessoryServerIsPaired(HAPNonnull(session->server))) {
                    identify_primary_accessory(session);
                } else {
                    WriteResponse(session, kHAPIPAccessoryServerResponse_InsufficientPrivileges);
                }
            } else {
                WriteResponse(session, kHAPIPAccessoryServerResponse_MethodNotAllowed);
            }
        } else if (
                (session->httpURI.numBytes == 11) &&
                HAPRawBufferAreEqual(GetHttpURIBytes(session), "/pair-setup", 11)) {
            if ((session->httpMethod.numBytes == 4) && HAPRawBufferAreEqual(GetHttpMethodBytes(session), "POST", 4)) {
                if (!session->securitySession.isSecured) {
                    // Close existing transient session.
                    for (size_t i = 0; i < server->ip.storage->numSessions; i++) {
                        HAPIPSession* ipSession = &server->ip.storage->sessions[i];
                        HAPIPSessionDescriptor* t = &ipSession->descriptor;
                        if (!t->server) {
                            continue;
                        }
                        // We immediately close the existing transient session.
                        // However, it might be better to finish writing ongoing responses, similar to Remove Pairing.
                        if (t != session && t->securitySession.type == kHAPIPSecuritySessionType_HAP &&
                            HAPSessionIsTransient(&t->securitySession._.hap)) {
                            HAPLog(&logObject,
                                   "Closing transient session "
                                   "due to /pair-setup while transient session is active.");
                            CloseSession(t);
                        }
                    }

                    // Handle message.
                    handle_pairing_data(session, HAPSessionHandlePairSetupWrite, HAPSessionHandlePairSetupRead);
                } else {
                    HAPLog(&logObject, "Rejected POST /pair-setup: Only non-secure access is supported.");
                    WriteResponse(session, kHAPIPAccessoryServerResponse_BadRequest);
                }
            } else {
                WriteResponse(session, kHAPIPAccessoryServerResponse_MethodNotAllowed);
            }
        } else if (
                (session->httpURI.numBytes == 12) &&
                HAPRawBufferAreEqual(GetHttpURIBytes(session), "/pair-verify", 12)) {
            if ((session->httpMethod.numBytes == 4) && HAPRawBufferAreEqual(GetHttpMethodBytes(session), "POST", 4)) {
                if (!session->securitySession.isSecured) {
                    handle_pairing_data(session, HAPSessionHandlePairVerifyWrite, HAPSessionHandlePairVerifyRead);
                } else {
                    HAPLog(&logObject, "Rejected POST /pair-verify: Only non-secure access is supported.");
                    WriteResponse(session, kHAPIPAccessoryServerResponse_BadRequest);
                }
            } else {
                WriteResponse(session, kHAPIPAccessoryServerResponse_MethodNotAllowed);
            }
        } else if ((session->httpURI.numBytes == 9) && HAPRawBufferAreEqual(GetHttpURIBytes(session), "/pairings", 9)) {
            if ((session->httpMethod.numBytes == 4) && HAPRawBufferAreEqual(GetHttpMethodBytes(session), "POST", 4)) {
                if (session->securitySession.isSecured || kHAPIPAccessoryServer_SessionSecurityDisabled) {
                    if (!HAPSessionIsTransient(&session->securitySession._.hap)) {
                        handle_pairing_data(session, HAPSessionHandlePairingsWrite, HAPSessionHandlePairingsRead);
                    } else {
                        HAPLog(&logObject, "Rejected POST /pairings: Session is transient.");
                        WriteResponse(session, kHAPIPAccessoryServerResponse_BadRequest);
                    }
                } else {
                    WriteResponse(session, kHAPIPAccessoryServerResponse_ConnectionAuthorizationRequired);
                }
            } else {
                if (session->securitySession.isSecured || kHAPIPAccessoryServer_SessionSecurityDisabled) {
                    if (!HAPSessionIsTransient(&session->securitySession._.hap)) {
                        WriteResponse(session, kHAPIPAccessoryServerResponse_MethodNotAllowed);
                    } else {
                        HAPLog(&logObject, "Rejected request for /pairings: Session is transient.");
                        WriteResponse(session, kHAPIPAccessoryServerResponse_BadRequest);
                    }
                } else {
                    WriteResponse(session, kHAPIPAccessoryServerResponse_ConnectionAuthorizationRequired);
                }
            }
        } else if (
                (session->httpURI.numBytes == 15) &&
                HAPRawBufferAreEqual(GetHttpURIBytes(session), "/secure-message", 15)) {
            if ((session->httpMethod.numBytes == 4) && HAPRawBufferAreEqual(GetHttpMethodBytes(session), "POST", 4)) {
                if (session->securitySession.isSecured || kHAPIPAccessoryServer_SessionSecurityDisabled) {
                    handle_secure_message(session);
                } else {
                    WriteResponse(session, kHAPIPAccessoryServerResponse_ConnectionAuthorizationRequired);
                }
            } else {
                if (session->securitySession.isSecured || kHAPIPAccessoryServer_SessionSecurityDisabled) {
                    WriteResponse(session, kHAPIPAccessoryServerResponse_MethodNotAllowed);
                } else {
                    WriteResponse(session, kHAPIPAccessoryServerResponse_ConnectionAuthorizationRequired);
                }
            }
        } else if ((session->httpURI.numBytes == 7) && HAPRawBufferAreEqual(GetHttpURIBytes(session), "/config", 7)) {
            if ((session->httpMethod.numBytes == 4) && HAPRawBufferAreEqual(GetHttpMethodBytes(session), "POST", 4)) {
                if (session->securitySession.isSecured || kHAPIPAccessoryServer_SessionSecurityDisabled) {
                    if (!HAPSessionIsTransient(&session->securitySession._.hap)) {
                        if (HAPAccessoryServerIsInWACMode(HAPNonnull(session->server))) {
                            handle_wac_msg(session, HAPWACEngineHandleConfig);
                        } else {
                            HAPLog(&logObject, "Rejected POST /config: WAC mode is not active.");
                            WriteResponse(session, kHAPIPAccessoryServerResponse_ResourceNotFound);
                        }
                    } else {
                        HAPLog(&logObject, "Rejected POST /config: Session is transient.");
                        WriteResponse(session, kHAPIPAccessoryServerResponse_BadRequest);
                    }
                } else {
                    WriteResponse(session, kHAPIPAccessoryServerResponse_ConnectionAuthorizationRequired);
                }
            } else {
                if (session->securitySession.isSecured || kHAPIPAccessoryServer_SessionSecurityDisabled) {
                    if (!HAPSessionIsTransient(&session->securitySession._.hap)) {
                        WriteResponse(session, kHAPIPAccessoryServerResponse_MethodNotAllowed);
                    } else {
                        HAPLog(&logObject, "Rejected request for /config: Session is transient.");
                        WriteResponse(session, kHAPIPAccessoryServerResponse_BadRequest);
                    }
                } else {
                    WriteResponse(session, kHAPIPAccessoryServerResponse_ConnectionAuthorizationRequired);
                }
            }
        } else if (
                (session->httpURI.numBytes == 12) &&
                HAPRawBufferAreEqual(GetHttpURIBytes(session), "/accessories", 12)) {
            if ((session->httpMethod.numBytes == 3) && HAPRawBufferAreEqual(GetHttpMethodBytes(session), "GET", 3)) {
                if (session->securitySession.isSecured || kHAPIPAccessoryServer_SessionSecurityDisabled) {
                    if (!HAPSessionIsTransient(&session->securitySession._.hap)) {
                        get_accessories(session);
                    } else {
                        HAPLog(&logObject, "Rejected GET /accessories: Session is transient.");
                        WriteResponse(session, kHAPIPAccessoryServerResponse_BadRequest);
                    }
                } else {
                    WriteResponse(session, kHAPIPAccessoryServerResponse_ConnectionAuthorizationRequiredWithStatus);
                }
            } else {
                if (session->securitySession.isSecured || kHAPIPAccessoryServer_SessionSecurityDisabled) {
                    if (!HAPSessionIsTransient(&session->securitySession._.hap)) {
                        WriteResponse(session, kHAPIPAccessoryServerResponse_MethodNotAllowed);
                    } else {
                        HAPLog(&logObject, "Rejected request for /accessories: Session is transient.");
                        WriteResponse(session, kHAPIPAccessoryServerResponse_BadRequest);
                    }
                } else {
                    WriteResponse(session, kHAPIPAccessoryServerResponse_ConnectionAuthorizationRequired);
                }
            }
        } else if (
                (session->httpURI.numBytes >= 16) &&
                HAPRawBufferAreEqual(GetHttpURIBytes(session), "/characteristics", 16)) {
            if ((session->httpMethod.numBytes == 3) && HAPRawBufferAreEqual(GetHttpMethodBytes(session), "GET", 3)) {
                if (session->securitySession.isSecured || kHAPIPAccessoryServer_SessionSecurityDisabled) {
                    if (!HAPSessionIsTransient(&session->securitySession._.hap)) {
                        get_characteristics(session);
                    } else {
                        HAPLog(&logObject, "Rejected GET /characteristics: Session is transient.");
                        WriteResponse(session, kHAPIPAccessoryServerResponse_BadRequest);
                    }
                } else {
                    WriteResponse(session, kHAPIPAccessoryServerResponse_ConnectionAuthorizationRequiredWithStatus);
                }
            } else if (
                    (session->httpMethod.numBytes == 3) &&
                    HAPRawBufferAreEqual(GetHttpMethodBytes(session), "PUT", 3)) {
                if (session->securitySession.isSecured || kHAPIPAccessoryServer_SessionSecurityDisabled) {
                    if (!HAPSessionIsTransient(&session->securitySession._.hap)) {
                        put_characteristics(session);
                    } else {
                        HAPLog(&logObject, "Rejected PUT /characteristics: Session is transient.");
                        WriteResponse(session, kHAPIPAccessoryServerResponse_BadRequest);
                    }
                } else {
                    WriteResponse(session, kHAPIPAccessoryServerResponse_ConnectionAuthorizationRequiredWithStatus);
                }
            } else {
                if (session->securitySession.isSecured || kHAPIPAccessoryServer_SessionSecurityDisabled) {
                    if (!HAPSessionIsTransient(&session->securitySession._.hap)) {
                        WriteResponse(session, kHAPIPAccessoryServerResponse_MethodNotAllowed);
                    } else {
                        HAPLog(&logObject, "Rejected request for /characteristics: Session is transient.");
                        WriteResponse(session, kHAPIPAccessoryServerResponse_BadRequest);
                    }
                } else {
                    WriteResponse(session, kHAPIPAccessoryServerResponse_ConnectionAuthorizationRequired);
                }
            }
        } else if ((session->httpURI.numBytes == 8) && HAPRawBufferAreEqual(GetHttpURIBytes(session), "/prepare", 8)) {
            if ((session->httpMethod.numBytes == 3) && HAPRawBufferAreEqual(GetHttpMethodBytes(session), "PUT", 3)) {
                if (session->securitySession.isSecured || kHAPIPAccessoryServer_SessionSecurityDisabled) {
                    if (!HAPSessionIsTransient(&session->securitySession._.hap)) {
                        put_prepare(session);
                    } else {
                        HAPLog(&logObject, "Rejected PUT /prepare: Session is transient.");
                        WriteResponse(session, kHAPIPAccessoryServerResponse_BadRequest);
                    }
                } else {
                    WriteResponse(session, kHAPIPAccessoryServerResponse_ConnectionAuthorizationRequiredWithStatus);
                }
            } else {
                if (session->securitySession.isSecured || kHAPIPAccessoryServer_SessionSecurityDisabled) {
                    if (!HAPSessionIsTransient(&session->securitySession._.hap)) {
                        WriteResponse(session, kHAPIPAccessoryServerResponse_MethodNotAllowed);
                    } else {
                        HAPLog(&logObject, "Rejected request for /prepare: Session is transient.");
                        WriteResponse(session, kHAPIPAccessoryServerResponse_BadRequest);
                    }
                } else {
                    WriteResponse(session, kHAPIPAccessoryServerResponse_ConnectionAuthorizationRequired);
                }
            }
        } else if ((session->httpURI.numBytes == 9) && HAPRawBufferAreEqual(GetHttpURIBytes(session), "/resource", 9)) {
            if ((session->httpMethod.numBytes == 4) && HAPRawBufferAreEqual(GetHttpMethodBytes(session), "POST", 4)) {
                if (session->securitySession.isSecured || kHAPIPAccessoryServer_SessionSecurityDisabled) {
                    if (!HAPSessionIsTransient(&session->securitySession._.hap)) {
                        post_resource(session);
                    } else {
                        HAPLog(&logObject, "Rejected POST /resource: Session is transient.");
                        WriteResponse(session, kHAPIPAccessoryServerResponse_BadRequest);
                    }
                } else {
                    WriteResponse(session, kHAPIPAccessoryServerResponse_ConnectionAuthorizationRequiredWithStatus);
                }
            } else {
                if (session->securitySession.isSecured || kHAPIPAccessoryServer_SessionSecurityDisabled) {
                    if (!HAPSessionIsTransient(&session->securitySession._.hap)) {
                        WriteResponse(session, kHAPIPAccessoryServerResponse_MethodNotAllowed);
                    } else {
                        HAPLog(&logObject, "Rejected request for /resource: Session is transient.");
                        WriteResponse(session, kHAPIPAccessoryServerResponse_BadRequest);
                    }
                } else {
                    WriteResponse(session, kHAPIPAccessoryServerResponse_ConnectionAuthorizationRequired);
                }
            }
        } else {
            HAPLogBuffer(&logObject, GetHttpURIBytes(session), session->httpURI.numBytes, "Unknown endpoint accessed.");
            if (session->securitySession.isSecured || kHAPIPAccessoryServer_SessionSecurityDisabled) {
                if (!HAPSessionIsTransient(&session->securitySession._.hap)) {
                    WriteResponse(session, kHAPIPAccessoryServerResponse_ResourceNotFound);
                } else {
                    HAPLog(&logObject, "Rejected request for unknown endpoint: Session is transient.");
                    WriteResponse(session, kHAPIPAccessoryServerResponse_BadRequest);
                }
            } else {
                WriteResponse(session, kHAPIPAccessoryServerResponse_ConnectionAuthorizationRequired);
            }
        }
    }
}

static void handle_http(HAPIPSessionDescriptor* session) {
    HAPPrecondition(session);
    if (!session->securitySession.isOpen) {
        HAPLogDebug(&logObject, "%s: session is not open, exiting function.", __func__);
        return;
    }
    HAPPrecondition(session->server);
    HAPPrecondition(session->securitySession.isOpen);

    HAPError err;

    size_t content_length, encrypted_length;
    HAPAssert(session->inboundBuffer.data);
    HAPAssert(session->inboundBuffer.position <= session->inboundBuffer.limit);
    HAPAssert(session->inboundBuffer.limit <= session->inboundBuffer.capacity);
    HAPAssert(session->httpReaderPosition <= session->inboundBuffer.position);
    HAPAssert(session->httpReader.state == util_HTTP_READER_STATE_DONE);
    HAPAssert(!session->httpParserError);
    if (session->httpContentLengthIsDefined) {
        content_length = session->httpContentLength;
    } else {
        content_length = 0;
    }
    if ((content_length <= session->inboundBuffer.position) &&
        (session->httpReaderPosition <= session->inboundBuffer.position - content_length)) {
        HAPLogBufferDebug(
                &logObject,
                session->inboundBuffer.data,
                session->httpReaderPosition + content_length,
                "session:%p:>",
                (const void*) session);
        handle_http_request(session);
        if ((session->state == kHAPIPSessionState_Reading) || (session->state == kHAPIPSessionState_Writing)) {
            if (session->accessorySerializationIsInProgress) {
                // Session is already prepared for writing
                HAPAssert(session->outboundBuffer.data);
                HAPAssert(session->outboundBuffer.position <= session->outboundBuffer.limit);
                HAPAssert(session->outboundBuffer.limit <= session->outboundBuffer.capacity);
                HAPAssert(session->state == kHAPIPSessionState_Writing);
            } else {
                HAPAssert(session->outboundBuffer.data);
                HAPAssert(session->outboundBuffer.position <= session->outboundBuffer.limit);
                HAPAssert(session->outboundBuffer.limit <= session->outboundBuffer.capacity);
                HAPIPByteBufferFlip(&session->outboundBuffer);
                HAPLogBufferDebug(
                        &logObject,
                        session->outboundBuffer.data,
                        session->outboundBuffer.limit,
                        "session:%p:<",
                        (const void*) session);

                if (session->securitySession.type == kHAPIPSecuritySessionType_HAP &&
                    session->securitySession.isSecured) {
                    encrypted_length = HAPIPSecurityProtocolGetNumEncryptedBytes(
                            session->outboundBuffer.limit - session->outboundBuffer.position);
                    if (encrypted_length > session->outboundBuffer.capacity - session->outboundBuffer.position) {
                        HAPLog(&logObject, "Out of resources (outbound buffer too small).");
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 1)
                        HAPAccessoryServer* server = session->server;
                        if (server->ip.storage->dynamicMemoryAllocation.allocateMemory) {
                            HAPFatalError();
                        }
#endif
                        session->outboundBuffer.limit = session->outboundBuffer.capacity;
                        err = HAPIPByteBufferAppendStringWithFormat(
                                &session->outboundBuffer, "%s", kHAPIPAccessoryServerResponse_OutOfResources);
                        if (err) {
                            HAPAssert(err == kHAPError_OutOfResources);
                            HAPAssertionFailure();
                        }
                        HAPIPByteBufferFlip(&session->outboundBuffer);
                        encrypted_length = HAPIPSecurityProtocolGetNumEncryptedBytes(
                                session->outboundBuffer.limit - session->outboundBuffer.position);
                        HAPAssert(
                                encrypted_length <=
                                session->outboundBuffer.capacity - session->outboundBuffer.position);
                    }
                    HAPIPSecurityProtocolEncryptData(
                            HAPNonnull(session->server), &session->securitySession._.hap, &session->outboundBuffer);
                    HAPAssert(encrypted_length == session->outboundBuffer.limit - session->outboundBuffer.position);
                }
                session->state = kHAPIPSessionState_Writing;
            }
        }
    }
}

static void read_http_content_length(HAPIPSessionDescriptor* session) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);

    size_t i;
    int overflow;
    unsigned int v;

    HAPAssert(session->inboundBuffer.data);
    HAPAssert(session->inboundBuffer.position <= session->inboundBuffer.limit);
    HAPAssert(session->inboundBuffer.limit <= session->inboundBuffer.capacity);
    HAPAssert(session->httpReaderPosition <= session->inboundBuffer.position);
    HAPAssert(session->httpReader.state == util_HTTP_READER_STATE_COMPLETED_HEADER_VALUE);
    HAPAssert(!session->httpParserError);
    char* httpHeaderFieldValueBytes = GetHttpHeaderFieldValueBytes(session);
    i = 0;
    while ((i < session->httpHeaderFieldValue.numBytes) &&
           ((httpHeaderFieldValueBytes[i] == kHAPIPAccessoryServerCharacter_Space) ||
            (httpHeaderFieldValueBytes[i] == kHAPIPAccessoryServerCharacter_HorizontalTab))) {
        // Skip whitespace.
        i++;
    }
    HAPAssert(
            (i == session->httpHeaderFieldValue.numBytes) ||
            ((i < session->httpHeaderFieldValue.numBytes) &&
             (httpHeaderFieldValueBytes[i] != kHAPIPAccessoryServerCharacter_Space) &&
             (httpHeaderFieldValueBytes[i] != kHAPIPAccessoryServerCharacter_HorizontalTab)));
    if ((i < session->httpHeaderFieldValue.numBytes) && HAPASCIICharacterIsNumber(httpHeaderFieldValueBytes[i]) &&
        !session->httpContentLengthIsDefined) {
        overflow = 0;
        session->httpContentLength = 0;
        do {
            v = (unsigned int) (httpHeaderFieldValueBytes[i] - '0');
            if (session->httpContentLength <= (SIZE_MAX - v) / 10) {
                session->httpContentLength = session->httpContentLength * 10 + v;
                i++;
            } else {
                overflow = 1;
            }
        } while (!overflow && (i < session->httpHeaderFieldValue.numBytes) &&
                 HAPASCIICharacterIsNumber(httpHeaderFieldValueBytes[i]));
        HAPAssert(
                overflow || (i == session->httpHeaderFieldValue.numBytes) ||
                ((i < session->httpHeaderFieldValue.numBytes) &&
                 (!HAPASCIICharacterIsNumber(httpHeaderFieldValueBytes[i]))));
        if (!overflow) {
            while ((i < session->httpHeaderFieldValue.numBytes) &&
                   ((httpHeaderFieldValueBytes[i] == kHAPIPAccessoryServerCharacter_Space) ||
                    (httpHeaderFieldValueBytes[i] == kHAPIPAccessoryServerCharacter_HorizontalTab))) {
                i++;
            }
            HAPAssert(
                    (i == session->httpHeaderFieldValue.numBytes) ||
                    ((i < session->httpHeaderFieldValue.numBytes) &&
                     (httpHeaderFieldValueBytes[i] != kHAPIPAccessoryServerCharacter_Space) &&
                     (httpHeaderFieldValueBytes[i] != kHAPIPAccessoryServerCharacter_HorizontalTab)));
            if (i == session->httpHeaderFieldValue.numBytes) {
                session->httpContentLengthIsDefined = true;
            } else {
                session->httpParserError = true;
            }
        } else {
            session->httpParserError = true;
        }
    } else {
        session->httpParserError = true;
    }
}

static void read_http_content_type(HAPIPSessionDescriptor* session) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);

    HAPAssert(session->inboundBuffer.data);
    HAPAssert(session->inboundBuffer.position <= session->inboundBuffer.limit);
    HAPAssert(session->inboundBuffer.limit <= session->inboundBuffer.capacity);
    HAPAssert(session->httpReaderPosition <= session->inboundBuffer.position);
    HAPAssert(session->httpReader.state == util_HTTP_READER_STATE_COMPLETED_HEADER_VALUE);
    HAPAssert(!session->httpParserError);

    char* httpHeaderFieldValueBytes = GetHttpHeaderFieldValueBytes(session);
    size_t i = 0;
    while ((i < session->httpHeaderFieldValue.numBytes) &&
           ((httpHeaderFieldValueBytes[i] == kHAPIPAccessoryServerCharacter_Space) ||
            (httpHeaderFieldValueBytes[i] == kHAPIPAccessoryServerCharacter_HorizontalTab))) {
        // Skip whitespace.
        i++;
    }
    HAPAssert(
            (i == session->httpHeaderFieldValue.numBytes) ||
            ((i < session->httpHeaderFieldValue.numBytes) &&
             (httpHeaderFieldValueBytes[i] != kHAPIPAccessoryServerCharacter_Space) &&
             (httpHeaderFieldValueBytes[i] != kHAPIPAccessoryServerCharacter_HorizontalTab)));
    if ((i < session->httpHeaderFieldValue.numBytes)) {
        session->httpContentType = kHAPIPAccessoryServerContentType_Unknown;

#define TryAssignContentType(contentType, contentTypeString) \
    do { \
        size_t numContentTypeStringBytes = sizeof(contentTypeString) - 1; \
        if (session->httpHeaderFieldValue.numBytes - i >= numContentTypeStringBytes && \
            HAPRawBufferAreEqual( \
                    &GetHttpHeaderFieldValueBytes(session)[i], (contentTypeString), numContentTypeStringBytes)) { \
            session->httpContentType = (contentType); \
            i += numContentTypeStringBytes; \
        } \
    } while (0)

        // Check longer header values first if multiple have the same prefix.
        TryAssignContentType(kHAPIPAccessoryServerContentType_Application_HAPJSON, "application/hap+json");
        TryAssignContentType(kHAPIPAccessoryServerContentType_Application_OctetStream, "application/octet-stream");
        TryAssignContentType(kHAPIPAccessoryServerContentType_Application_PairingTLV8, "application/pairing+tlv8");

#undef TryAssignContentType

        while ((i < session->httpHeaderFieldValue.numBytes) &&
               ((httpHeaderFieldValueBytes[i] == kHAPIPAccessoryServerCharacter_Space) ||
                (httpHeaderFieldValueBytes[i] == kHAPIPAccessoryServerCharacter_HorizontalTab))) {
            i++;
        }
        HAPAssert(
                (i == session->httpHeaderFieldValue.numBytes) ||
                ((i < session->httpHeaderFieldValue.numBytes) &&
                 (httpHeaderFieldValueBytes[i] != kHAPIPAccessoryServerCharacter_Space) &&
                 (httpHeaderFieldValueBytes[i] != kHAPIPAccessoryServerCharacter_HorizontalTab)));
        if (i != session->httpHeaderFieldValue.numBytes) {
            HAPLogBuffer(
                    &logObject,
                    httpHeaderFieldValueBytes,
                    session->httpHeaderFieldValue.numBytes,
                    "Unknown Content-Type.");
            session->httpContentType = kHAPIPAccessoryServerContentType_Unknown;
        }
    } else {
        session->httpParserError = true;
    }
}

static void read_http(HAPIPSessionDescriptor* session) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);

    struct util_http_reader* r;

    HAPAssert(session->inboundBuffer.data);
    HAPAssert(session->inboundBuffer.position <= session->inboundBuffer.limit);
    HAPAssert(session->inboundBuffer.limit <= session->inboundBuffer.capacity);
    HAPAssert(session->httpReaderPosition <= session->inboundBuffer.position);
    HAPAssert(!session->httpParserError);
    r = &session->httpReader;
    bool hasContentLength = false;
    bool hasContentType = false;
    if (session->httpReaderPosition < session->inboundBuffer.position) {
        do {
            size_t n = util_http_reader_read(
                    r,
                    &session->inboundBuffer.data[session->httpReaderPosition],
                    session->inboundBuffer.position - session->httpReaderPosition);
            switch (r->state) {
                case util_HTTP_READER_STATE_READING_METHOD:
                case util_HTTP_READER_STATE_COMPLETED_METHOD: {
                    if (!session->httpMethodIsDefined) {
                        session->httpMethod.position =
                                session->httpReaderPosition +
                                (size_t)(r->result_token - &session->inboundBuffer.data[session->httpReaderPosition]);
                        session->httpMethod.numBytes = r->result_length;
                        session->httpMethodIsDefined = true;
                    } else {
                        HAPAssert(
                                &session->inboundBuffer
                                         .data[session->httpMethod.position + session->httpMethod.numBytes] ==
                                r->result_token);
                        session->httpMethod.numBytes += r->result_length;
                    }
                    break;
                }
                case util_HTTP_READER_STATE_READING_URI:
                case util_HTTP_READER_STATE_COMPLETED_URI: {
                    if (!session->httpURIIsDefined) {
                        session->httpURI.position =
                                session->httpReaderPosition +
                                (size_t)(r->result_token - &session->inboundBuffer.data[session->httpReaderPosition]);
                        session->httpURI.numBytes = r->result_length;
                        session->httpURIIsDefined = true;
                    } else {
                        HAPAssert(
                                &session->inboundBuffer.data[session->httpURI.position + session->httpURI.numBytes] ==
                                r->result_token);
                        session->httpURI.numBytes += r->result_length;
                    }
                    break;
                }
                case util_HTTP_READER_STATE_READING_HEADER_NAME:
                case util_HTTP_READER_STATE_COMPLETED_HEADER_NAME: {
                    if (!session->httpHeaderFieldNameIsDefined) {
                        session->httpHeaderFieldName.position =
                                session->httpReaderPosition +
                                (size_t)(r->result_token - &session->inboundBuffer.data[session->httpReaderPosition]);
                        session->httpHeaderFieldName.numBytes = r->result_length;
                        session->httpHeaderFieldNameIsDefined = true;
                    } else {
                        HAPAssert(
                                &session->inboundBuffer
                                         .data[session->httpHeaderFieldName.position +
                                               session->httpHeaderFieldName.numBytes] == r->result_token);
                        session->httpHeaderFieldName.numBytes += r->result_length;
                    }
                    break;
                }
                case util_HTTP_READER_STATE_READING_HEADER_VALUE: {
                    if (!session->httpHeaderFieldValueIsDefined) {
                        session->httpHeaderFieldValue.position =
                                session->httpReaderPosition +
                                (size_t)(r->result_token - &session->inboundBuffer.data[session->httpReaderPosition]);
                        session->httpHeaderFieldValue.numBytes = r->result_length;
                        session->httpHeaderFieldValueIsDefined = true;
                    } else {
                        HAPAssert(
                                &session->inboundBuffer
                                         .data[session->httpHeaderFieldValue.position +
                                               session->httpHeaderFieldValue.numBytes] == r->result_token);
                        session->httpHeaderFieldValue.numBytes += r->result_length;
                    }
                    break;
                }
                case util_HTTP_READER_STATE_COMPLETED_HEADER_VALUE: {
                    if (!session->httpHeaderFieldValueIsDefined) {
                        session->httpHeaderFieldValue.position =
                                session->httpReaderPosition +
                                (size_t)(r->result_token - &session->inboundBuffer.data[session->httpReaderPosition]);
                        session->httpHeaderFieldValue.numBytes = r->result_length;
                        session->httpHeaderFieldValueIsDefined = true;
                    } else {
                        HAPAssert(
                                &session->inboundBuffer
                                         .data[session->httpHeaderFieldValue.position +
                                               session->httpHeaderFieldValue.numBytes] == r->result_token);
                        session->httpHeaderFieldValue.numBytes += r->result_length;
                    }
                    char* httpHeaderFieldNameBytes = GetHttpHeaderFieldNameBytes(session);
                    if ((session->httpHeaderFieldName.numBytes == 14) &&
                        (httpHeaderFieldNameBytes[0] == 'C' || httpHeaderFieldNameBytes[0] == 'c') &&
                        (httpHeaderFieldNameBytes[1] == 'O' || httpHeaderFieldNameBytes[1] == 'o') &&
                        (httpHeaderFieldNameBytes[2] == 'N' || httpHeaderFieldNameBytes[2] == 'n') &&
                        (httpHeaderFieldNameBytes[3] == 'T' || httpHeaderFieldNameBytes[3] == 't') &&
                        (httpHeaderFieldNameBytes[4] == 'E' || httpHeaderFieldNameBytes[4] == 'e') &&
                        (httpHeaderFieldNameBytes[5] == 'N' || httpHeaderFieldNameBytes[5] == 'n') &&
                        (httpHeaderFieldNameBytes[6] == 'T' || httpHeaderFieldNameBytes[6] == 't') &&
                        (httpHeaderFieldNameBytes[7] == '-') &&
                        (httpHeaderFieldNameBytes[8] == 'L' || httpHeaderFieldNameBytes[8] == 'l') &&
                        (httpHeaderFieldNameBytes[9] == 'E' || httpHeaderFieldNameBytes[9] == 'e') &&
                        (httpHeaderFieldNameBytes[10] == 'N' || httpHeaderFieldNameBytes[10] == 'n') &&
                        (httpHeaderFieldNameBytes[11] == 'G' || httpHeaderFieldNameBytes[11] == 'g') &&
                        (httpHeaderFieldNameBytes[12] == 'T' || httpHeaderFieldNameBytes[12] == 't') &&
                        (httpHeaderFieldNameBytes[13] == 'H' || httpHeaderFieldNameBytes[13] == 'h')) {
                        if (hasContentLength) {
                            HAPLog(&logObject, "Request has multiple Content-Length headers.");
                            session->httpParserError = true;
                        } else {
                            hasContentLength = true;
                            read_http_content_length(session);
                        }
                    } else if (
                            (session->httpHeaderFieldName.numBytes == 12) &&
                            (httpHeaderFieldNameBytes[0] == 'C' || httpHeaderFieldNameBytes[0] == 'c') &&
                            (httpHeaderFieldNameBytes[1] == 'O' || httpHeaderFieldNameBytes[1] == 'o') &&
                            (httpHeaderFieldNameBytes[2] == 'N' || httpHeaderFieldNameBytes[2] == 'n') &&
                            (httpHeaderFieldNameBytes[3] == 'T' || httpHeaderFieldNameBytes[3] == 't') &&
                            (httpHeaderFieldNameBytes[4] == 'E' || httpHeaderFieldNameBytes[4] == 'e') &&
                            (httpHeaderFieldNameBytes[5] == 'N' || httpHeaderFieldNameBytes[5] == 'n') &&
                            (httpHeaderFieldNameBytes[6] == 'T' || httpHeaderFieldNameBytes[6] == 't') &&
                            (httpHeaderFieldNameBytes[7] == '-') &&
                            (httpHeaderFieldNameBytes[8] == 'T' || httpHeaderFieldNameBytes[8] == 't') &&
                            (httpHeaderFieldNameBytes[9] == 'Y' || httpHeaderFieldNameBytes[9] == 'y') &&
                            (httpHeaderFieldNameBytes[10] == 'P' || httpHeaderFieldNameBytes[10] == 'p') &&
                            (httpHeaderFieldNameBytes[11] == 'E' || httpHeaderFieldNameBytes[11] == 'e')) {
                        if (hasContentType) {
                            HAPLog(&logObject, "Request has multiple Content-Type headers.");
                            session->httpParserError = true;
                        } else {
                            hasContentType = true;
                            read_http_content_type(session);
                        }
                    } else {
                        // HTTP header that we are not interested in.
                    }
                    session->httpHeaderFieldNameIsDefined = false;
                    session->httpHeaderFieldValueIsDefined = false;
                    break;
                }
                default:
                    break;
            }
            session->httpReaderPosition += n;
        } while ((session->httpReaderPosition < session->inboundBuffer.position) &&
                 (r->state != util_HTTP_READER_STATE_DONE) && (r->state != util_HTTP_READER_STATE_ERROR) &&
                 !session->httpParserError);
    }
    HAPAssert(
            (session->httpReaderPosition == session->inboundBuffer.position) ||
            ((session->httpReaderPosition < session->inboundBuffer.position) &&
             ((r->state == util_HTTP_READER_STATE_DONE) || (r->state == util_HTTP_READER_STATE_ERROR) ||
              session->httpParserError)));
}

static void handle_input(HAPIPSessionDescriptor* session) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPPrecondition(session->securitySession.isOpen);

    HAPError err;

    HAPAssert(session->inboundBuffer.data);
    HAPAssert(session->inboundBuffer.position <= session->inboundBuffer.limit);
    HAPAssert(session->inboundBuffer.limit <= session->inboundBuffer.capacity);
    HAPAssert(session->inboundBufferMark <= session->inboundBuffer.position);
    session->inboundBuffer.limit = session->inboundBuffer.position;
    if (session->securitySession.type == kHAPIPSecuritySessionType_HAP &&
        HAPSessionIsSecured(&session->securitySession._.hap)) {
        if (!session->securitySession.isSecured) {
            HAPLogDebug(&logObject, "Established HAP security session.");
            session->securitySession.isSecured = true;
        }
        session->inboundBuffer.position = session->inboundBufferMark;
        err = HAPIPSecurityProtocolDecryptData(
                HAPNonnull(session->server), &session->securitySession._.hap, &session->inboundBuffer);
    } else {
        HAPAssert(
                session->securitySession.type != kHAPIPSecuritySessionType_HAP || !session->securitySession.isSecured);
        err = kHAPError_None;
    }
    if (!err) {
        read_http(session);
        if ((session->httpReader.state == util_HTTP_READER_STATE_ERROR) || session->httpParserError) {
            log_protocol_error(
                    kHAPLogType_Info, "Unexpected request.", &session->inboundBuffer, __func__, HAP_FILE, __LINE__);
            CloseSession(session);
        } else {
            if (session->httpReader.state == util_HTTP_READER_STATE_DONE) {
                handle_http(session);
            }
            if (session->state == kHAPIPSessionState_Reading) {
                session->inboundBufferMark = session->inboundBuffer.position;
                session->inboundBuffer.position = session->inboundBuffer.limit;
                session->inboundBuffer.limit = session->inboundBuffer.capacity;
                if (session->inboundBuffer.position == session->inboundBuffer.limit) {
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 0)
                    log_protocol_error(
                            kHAPLogType_Info,
                            "Unexpected request. Closing connection (inbound buffer too small).",
                            &session->inboundBuffer,
                            __func__,
                            HAP_FILE,
                            __LINE__);
                    CloseSession(session);
#else
                    HAPAccessoryServer* server = session->server;
                    if (!server->ip.storage->dynamicMemoryAllocation.reallocateMemory) {
                        log_protocol_error(
                                kHAPLogType_Info,
                                "Unexpected request. Closing connection (inbound buffer too small).",
                                &session->inboundBuffer,
                                __func__,
                                HAP_FILE,
                                __LINE__);
                        CloseSession(session);
                    } else {
                        size_t maxEncryptedFrameBytes = kHAPIPAccessoryServer_MaxEncryptedFrameBytes;
                        HAPAssert(session->inboundBuffer.limit == session->inboundBuffer.capacity);
                        HAPAssert(
                                session->inboundBufferMark + maxEncryptedFrameBytes > session->inboundBuffer.capacity);
                        if (!session->securitySession.isSecured && !kHAPIPAccessoryServer_SessionSecurityDisabled &&
                            ((session->inboundBufferMark + maxEncryptedFrameBytes) >
                             kHAPIPAccessoryServer_MaxUnsecuredInboundBufferBytes)) {
                            log_protocol_error(
                                    kHAPLogType_Info,
                                    "Unexpected unpaired request. Closing connection (request to long).",
                                    &session->inboundBuffer,
                                    __func__,
                                    HAP_FILE,
                                    __LINE__);
                            CloseSession(session);
                        } else {
                            err = ReallocateInboundBuffer(session, session->inboundBufferMark + maxEncryptedFrameBytes);
                            if (err) {
                                HAPAssert(err == kHAPError_OutOfResources);
                                CloseSession(session);
                            } else {
                                session->inboundBuffer.limit = session->inboundBuffer.capacity;
                                HAPAssert(session->inboundBuffer.position < session->inboundBuffer.limit);
                            }
                        }
                    }
#endif
                }
            }
        }
    } else {
        HAPAssert(err == kHAPError_InvalidData);
        HAPLog(&logObject, "Decryption error.");
        CloseSession(session);
    }
}

static void HandleTCPStreamEvent(
        HAPPlatformTCPStreamManagerRef tcpStreamManager,
        HAPPlatformTCPStreamRef tcpStream,
        HAPPlatformTCPStreamEvent event,
        void* _Nullable context);

static void write_event_notifications(HAPIPSessionDescriptor* session) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPAccessoryServer* server = session->server;
    HAPPrecondition(session->securitySession.type == kHAPIPSecuritySessionType_HAP);
    HAPPrecondition(session->securitySession.isOpen);
    HAPPrecondition(!HAPSessionIsTransient(&session->securitySession._.hap));
    HAPPrecondition(session->state == kHAPIPSessionState_Reading);
    HAPPrecondition(session->inboundBuffer.position == 0);

    HAPError err;

    if (session->securitySession.isSecured || kHAPIPAccessoryServer_SessionSecurityDisabled) {
        HAPTime clock_now_ms = HAPPlatformClockGetCurrent();
        HAPAssert(clock_now_ms >= session->eventNotificationStamp);
        HAPTime dt_ms = clock_now_ms - session->eventNotificationStamp;

        size_t numReadContexts = 0;

        if (session->eventNotifications) {
            HAPPrecondition(session->numEventNotificationFlags > 0);
            HAPPrecondition(session->numEventNotificationFlags <= session->numEventNotifications);
            HAPPrecondition(session->numEventNotifications <= session->maxEventNotifications);
            for (size_t i = 0; i < session->numEventNotifications; i++) {
                HAPIPEventNotification* eventNotification = &session->eventNotifications[i];
                if (eventNotification->flag) {
                    bool notifyNow;
                    if (dt_ms >= kHAPIPAccessoryServer_MaxEventNotificationDelay) {
                        notifyNow = true;
                        session->eventNotificationStamp = clock_now_ms;
                    } else {
                        // Network-based notifications must be coalesced by the accessory using a delay of no less than
                        // 1 second. The exception to this rule includes notifications for the following characteristics
                        // which must be delivered immediately.
                        // See HomeKit Accessory Protocol Specification R17
                        // Section 6.8 Notifications
                        const HAPCharacteristic* characteristic_;
                        const HAPService* service;
                        const HAPAccessory* accessory;
                        get_db_ctx(
                                session->server,
                                eventNotification->aid,
                                eventNotification->iid,
                                &characteristic_,
                                &service,
                                &accessory);
                        HAPAssert(accessory);
                        HAPAssert(service);
                        HAPAssert(characteristic_);
                        const HAPBaseCharacteristic* characteristic = characteristic_;
                        notifyNow = HAPUUIDAreEqual(
                                            characteristic->characteristicType, &kHAPCharacteristicType_ButtonEvent) ||
                                    HAPUUIDAreEqual(
                                            characteristic->characteristicType,
                                            &kHAPCharacteristicType_ProgrammableSwitchEvent);
                        if (notifyNow) {
                            HAPLogCharacteristicDebug(
                                    &logObject,
                                    characteristic_,
                                    service,
                                    accessory,
                                    "Characteristic allowed to bypass notification coalescing requirement.");
                        }
                    }
                    if (notifyNow) {
                        HAPAssert(numReadContexts < server->ip.storage->numReadContexts);
                        HAPIPReadContext* readContext = &server->ip.storage->readContexts[numReadContexts];
                        HAPRawBufferZero(readContext, sizeof *readContext);
                        readContext->aid = eventNotification->aid;
                        readContext->iid = eventNotification->iid;
                        numReadContexts++;
                        eventNotification->flag = false;
                        HAPAssert(session->numEventNotificationFlags > 0);
                        session->numEventNotificationFlags--;
                    }
                }
            }
        } else {
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 0)
            HAPAssertionFailure();
#else
            if (session->eventNotificationSubscriptions) {
                HAPAssert(session->eventNotificationFlags);
                if (!HAPBitSetIsEmpty(
                            HAPNonnull(session->eventNotificationFlags), server->ip.numEventNotificationBitSetBytes)) {
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
                                            if (HAPBitSetContains(
                                                        HAPNonnull(session->eventNotificationFlags),
                                                        server->ip.numEventNotificationBitSetBytes,
                                                        eventNotificationIndex)) {
                                                bool notifyNow;
                                                if (dt_ms >= kHAPIPAccessoryServer_MaxEventNotificationDelay) {
                                                    notifyNow = true;
                                                    session->eventNotificationStamp = clock_now_ms;
                                                } else {
                                                    // Network-based notifications must be coalesced by the
                                                    // accessory using a delay of no less than 1 second. The
                                                    // exception to this rule includes notifications for the
                                                    // following characteristics which must be delivered
                                                    // immediately.
                                                    // See HomeKit Accessory Protocol Specification R17
                                                    // Section 6.8 Notifications
                                                    notifyNow =
                                                            HAPUUIDAreEqual(
                                                                    characteristic->characteristicType,
                                                                    &kHAPCharacteristicType_ButtonEvent) ||
                                                            HAPUUIDAreEqual(
                                                                    characteristic->characteristicType,
                                                                    &kHAPCharacteristicType_ProgrammableSwitchEvent);
                                                    if (notifyNow) {
                                                        HAPLogCharacteristicDebug(
                                                                &logObject,
                                                                characteristic,
                                                                service,
                                                                accessory,
                                                                "Characteristic allowed to bypass notification "
                                                                "coalescing requirement.");
                                                    }
                                                }
                                                if (notifyNow) {
                                                    HAPAssert(numReadContexts < server->ip.storage->numReadContexts);
                                                    HAPIPReadContext* readContext =
                                                            &server->ip.storage->readContexts[numReadContexts];
                                                    HAPRawBufferZero(readContext, sizeof *readContext);
                                                    readContext->aid = accessory->aid;
                                                    readContext->iid = characteristic->iid;
                                                    numReadContexts++;
                                                    HAPBitSetRemove(
                                                            HAPNonnull(session->eventNotificationFlags),
                                                            server->ip.numEventNotificationBitSetBytes,
                                                            eventNotificationIndex);
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
                        accessory = server->ip.bridgedAccessories ? server->ip.bridgedAccessories[accessoryIndex - 1] :
                                                                    NULL;
                    }
                }
            } else {
                HAPAssert(!session->eventNotificationSubscriptions);
                HAPAssert(!session->eventNotificationFlags);
            }
#endif
        }

        if (numReadContexts > 0) {
            HAPIPByteBuffer data_buffer;
            data_buffer.data = server->ip.storage->scratchBuffer.bytes;
            data_buffer.capacity = server->ip.storage->scratchBuffer.numBytes;
            data_buffer.limit = server->ip.storage->scratchBuffer.numBytes;
            data_buffer.position = 0;
            HAPAssert(data_buffer.data);
            HAPAssert(data_buffer.position <= data_buffer.limit);
            HAPAssert(data_buffer.limit <= data_buffer.capacity);
            int r = handle_characteristic_read_requests(
                    session,
                    kHAPIPSessionContext_EventNotification,
                    server->ip.storage->readContexts,
                    numReadContexts,
                    &data_buffer);
            (void) r;

            size_t content_length = HAPIPAccessoryProtocolGetNumEventNotificationBytes(
                    HAPNonnull(session->server), server->ip.storage->readContexts, numReadContexts);

            err = PrepareWritingResponse(session, kHAPIPAccessoryServer_MinOutboundBufferBytes + content_length);
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                CloseSession(session);
                return;
            }
            size_t mark = session->outboundBuffer.position;
            err = HAPIPByteBufferAppendStringWithFormat(
                    &session->outboundBuffer,
                    "EVENT/1.0 200 OK\r\n"
                    "Content-Type: application/hap+json\r\n"
                    "Content-Length: %zu\r\n\r\n",
                    content_length);
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                HAPLog(&logObject, "Invalid configuration (outbound buffer too small).");
                HAPFatalError();
            }
            if (content_length <= session->outboundBuffer.limit - session->outboundBuffer.position) {
                mark = session->outboundBuffer.position;
                err = HAPIPAccessoryProtocolGetEventNotificationBytes(
                        HAPNonnull(session->server),
                        server->ip.storage->readContexts,
                        numReadContexts,
                        &session->outboundBuffer);
                HAPAssert(!err && (session->outboundBuffer.position - mark == content_length));
                HAPIPByteBufferFlip(&session->outboundBuffer);
                HAPLogBufferDebug(
                        &logObject,
                        session->outboundBuffer.data,
                        session->outboundBuffer.limit,
                        "session:%p:<",
                        (const void*) session);
                if (session->securitySession.isSecured) {
                    size_t encrypted_length = HAPIPSecurityProtocolGetNumEncryptedBytes(
                            session->outboundBuffer.limit - session->outboundBuffer.position);
                    if (encrypted_length <= session->outboundBuffer.capacity - session->outboundBuffer.position) {
                        HAPIPSecurityProtocolEncryptData(
                                HAPNonnull(session->server), &session->securitySession._.hap, &session->outboundBuffer);
                        HAPAssert(encrypted_length == session->outboundBuffer.limit - session->outboundBuffer.position);
                        session->state = kHAPIPSessionState_Writing;
                    } else {
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 1)
                        if (server->ip.storage->dynamicMemoryAllocation.allocateMemory) {
                            HAPLog(&logObject, "Invalid configuration (outbound buffer too small).");
                            HAPFatalError();
                        }
#endif
                        HAPLog(&logObject, "Skipping event notifications (outbound buffer too small).");
                        HAPIPByteBufferClear(&session->outboundBuffer);
                    }
                } else {
                    HAPAssert(kHAPIPAccessoryServer_SessionSecurityDisabled);
                    HAP_DIAGNOSTIC_IGNORED_ICCARM(Pe111)
                    session->state = kHAPIPSessionState_Writing;
                    HAP_DIAGNOSTIC_RESTORE_ICCARM(Pe111)
                }
                if (session->state == kHAPIPSessionState_Writing) {
                    HAPPlatformTCPStreamEvent interests = { .hasBytesAvailable = false, .hasSpaceAvailable = true };
                    session->stamp = clock_now_ms;
                    HAPPlatformTCPStreamUpdateInterests(
                            HAPNonnull(server->platform.ip.tcpStreamManager),
                            session->tcpStream,
                            interests,
                            HandleTCPStreamEvent,
                            session);
                }
            } else {
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 1)
                if (server->ip.storage->dynamicMemoryAllocation.allocateMemory) {
                    HAPLog(&logObject, "Invalid configuration (outbound buffer too small).");
                    HAPFatalError();
                }
#endif
                HAPLog(&logObject, "Skipping event notifications (outbound buffer too small).");
                session->outboundBuffer.position = mark;
            }
        }
    } else {
        if (session->eventNotifications) {
            HAPPrecondition(session->numEventNotificationFlags > 0);
            HAPPrecondition(session->numEventNotificationFlags <= session->numEventNotifications);
            HAPPrecondition(session->numEventNotifications <= session->maxEventNotifications);
            for (size_t i = 0; i < session->numEventNotifications; i++) {
                HAPIPEventNotification* eventNotification = &session->eventNotifications[i];
                if (eventNotification->flag) {
                    eventNotification->flag = false;
                    HAPAssert(session->numEventNotificationFlags > 0);
                    session->numEventNotificationFlags--;
                }
            }
            HAPAssert(!session->numEventNotificationFlags);
        } else {
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 0)
            HAPAssertionFailure();
#else
            if (session->eventNotificationSubscriptions) {
                HAPAssert(session->eventNotificationFlags);
                HAPRawBufferZero(
                        HAPNonnullVoid(session->eventNotificationFlags), server->ip.numEventNotificationBitSetBytes);
                HAPAssert(HAPBitSetIsEmpty(
                        HAPNonnull(session->eventNotificationFlags), server->ip.numEventNotificationBitSetBytes));
            } else {
                HAPAssert(!session->eventNotificationSubscriptions);
                HAPAssert(!session->eventNotificationFlags);
            }
#endif
        }
        session->eventNotificationStamp = HAPPlatformClockGetCurrent();
    }
}

static void handle_io_progression(HAPIPSessionDescriptor* session) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPAccessoryServer* server = session->server;

    if ((session->state == kHAPIPSessionState_Reading) && (session->inboundBuffer.position == 0)) {
        if (server->ip.state == kHAPIPAccessoryServerState_Running) {
            if (SessionHasPendingEventNotifications(session)) {
                HAPAssert(session->securitySession.type == kHAPIPSecuritySessionType_HAP);
                schedule_event_notifications(session->server);
            }
        } else {
            HAPAssert(server->ip.state == kHAPIPAccessoryServerState_Stopping);
            if (SessionHasPendingEventNotifications(session)) {
                HAPAssert(session->securitySession.type == kHAPIPSecuritySessionType_HAP);
                schedule_event_notifications(session->server);
            } else {
                CloseSession(session);
            }
        }
    }
    if (session->tcpStreamIsOpen) {
        HAPPlatformTCPStreamEvent interests = { .hasBytesAvailable = (session->state == kHAPIPSessionState_Reading),
                                                .hasSpaceAvailable = (session->state == kHAPIPSessionState_Writing) };
        if ((session->state == kHAPIPSessionState_Reading) || (session->state == kHAPIPSessionState_Writing)) {
            HAPPlatformTCPStreamUpdateInterests(
                    HAPNonnull(server->platform.ip.tcpStreamManager),
                    session->tcpStream,
                    interests,
                    HandleTCPStreamEvent,
                    session);
        } else {
            HAPPlatformTCPStreamUpdateInterests(
                    HAPNonnull(server->platform.ip.tcpStreamManager), session->tcpStream, interests, NULL, session);
        }
    } else {
        HAPAssert(server->ip.garbageCollectionTimer);
    }
}

static void handle_output_completion(HAPIPSessionDescriptor* session) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPAccessoryServer* server = session->server;

    HAPAssert(session->state == kHAPIPSessionState_Writing);

    if (session->securitySession.isOpen) {
        if (session->securitySession.receivedConfig) {

            HAPLogDebug(&logObject, "Completed sending of Wi-Fi configuration response.");

            HAPAssert(session->tcpStreamIsOpen);
            HAPPlatformTCPStreamCloseOutput(HAPNonnull(server->platform.ip.tcpStreamManager), session->tcpStream);
        } else {
            if (session->securitySession.type == kHAPIPSecuritySessionType_HAP &&
                session->securitySession.receivedConfigured) {
                HAPLogDebug(&logObject, "Completed sending of /configured response.");
                session->securitySession.receivedConfigured = false;
                if (server->ip.wacConfiguredMessageTimer) {
                    HAPPlatformTimerDeregister(server->ip.wacConfiguredMessageTimer);
                    server->ip.wacConfiguredMessageTimer = 0;
                }
                HAPAssert(server->ip.state == kHAPIPAccessoryServerState_Running);
                HAPAssert(server->ip.nextState == kHAPIPAccessoryServerState_Undefined);
            }
        }
    }
    session->state = kHAPIPSessionState_Reading;
    PrepareReadingRequest(session);
    if (session->inboundBuffer.position != 0) {
        handle_input(session);
    }
}

static void WriteOutboundData(HAPIPSessionDescriptor* session) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPAccessoryServer* server = session->server;
    HAPPrecondition(session->tcpStreamIsOpen);

    HAPError err;

    HAPIPByteBuffer* b;
    b = &session->outboundBuffer;
    HAPAssert(b->data);
    HAPAssert(b->position <= b->limit);
    HAPAssert(b->limit <= b->capacity);

    size_t numNonAcknowledgedBytesBefore = HAPPlatformTCPStreamGetNumNonAcknowledgedBytes(
            HAPNonnull(server->platform.ip.tcpStreamManager), session->tcpStream);

    size_t numBytes;
    err = HAPPlatformTCPStreamWrite(
            HAPNonnull(server->platform.ip.tcpStreamManager),
            session->tcpStream,
            /* bytes: */ &b->data[b->position],
            /* maxBytes: */ b->limit - b->position,
            &numBytes);

    size_t numNonAcknowledgedBytesAfter = HAPPlatformTCPStreamGetNumNonAcknowledgedBytes(
            HAPNonnull(server->platform.ip.tcpStreamManager), session->tcpStream);

    if (err == kHAPError_Unknown) {
        log_result(
                kHAPLogType_Error,
                "error:Function 'HAPPlatformTCPStreamWrite' failed.",
                err,
                __func__,
                HAP_FILE,
                __LINE__);
        if (SessionIsLastSessionWithEnabledEventNotifications(session)) {
            CloseSession(session);
            RepublishHAPService(session->server);
        } else {
            CloseSession(session);
        }
        return;
    }
    if (err == kHAPError_Busy) {
        return;
    }

    HAPAssert(!err);
    if (numBytes == 0) {
        HAPLogDebug(&logObject, "error:Function 'HAPPlatformTCPStreamWrite' failed: 0 bytes written.");
        if (SessionIsLastSessionWithEnabledEventNotifications(session)) {
            CloseSession(session);
            RepublishHAPService(session->server);
        } else {
            CloseSession(session);
        }
        return;
    }

    // Assumption: HAPPlatformTCPStreamGetNumNonAcknowledgedBytes may include TCP protocol bytes.
    if (!numNonAcknowledgedBytesBefore                                       /* No history. */
        || numNonAcknowledgedBytesBefore != session->numNonAcknowledgedBytes /* Ack'ed before write. */
        || numNonAcknowledgedBytesAfter < numBytes                           /* Ack'ed part of payload. */
        || numNonAcknowledgedBytesAfter - numBytes < numNonAcknowledgedBytesBefore /* Ack'ed part of payload. */) {
        session->tcpDidProgress = true;
    }
    session->numNonAcknowledgedBytes = numNonAcknowledgedBytesAfter;

    HAPAssert(numBytes <= b->limit - b->position);
    b->position += numBytes;
    if (b->position == b->limit) {
        if (session->securitySession.type == kHAPIPSecuritySessionType_HAP && session->securitySession.isSecured &&
            (!HAPSessionIsSecured(&session->securitySession._.hap) ||
             HAPSessionKeyExpired(&session->securitySession._.hap))) {
            HAPLogDebug(&logObject, "Pairing removed, closing session.");
            CloseSession(session);
        } else if (session->accessorySerializationIsInProgress) {
            handle_accessory_serialization(session);
        } else if (session->cameraSnapshotReader) {
            HAPIPByteBufferClear(b);
            handle_ip_camera_snapshot(session);
        } else {
            HAPIPByteBufferClear(b);
            handle_output_completion(session);
        }
    }
}

static void handle_input_closed(HAPIPSessionDescriptor* session) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPAccessoryServer* server = session->server;

    HAPLogDebug(&logObject, "session:%p:input closed", (const void*) session);

    if (session->securitySession.isOpen && session->securitySession.receivedConfig) {
        HAPLogDebug(
                &logObject,
                "Session closed by controller after sending of Wi-Fi configuration response (FIN received).");

        session->securitySession.receivedConfig = false;
        CloseSession(session);

        if (server->ip.wacModeTimer) {
            HAPLogInfo(
                    &logObject,
                    "Stopping Wi-Fi configuration timer while accessory server is restarted after configuration.");

            HAPPlatformTimerDeregister(server->ip.wacModeTimer);
            server->ip.wacModeTimer = 0;

            HAPAssert(server->ip.state == kHAPIPAccessoryServerState_Running);
            HAPAssert(server->ip.nextState == kHAPIPAccessoryServerState_Undefined);
            server->ip.state = kHAPIPAccessoryServerState_Stopping;
            server->ip.nextState = kHAPIPAccessoryServerState_Running;
            schedule_max_idle_time_timer(session->server);
        }
        HAPAssert(server->ip.state == kHAPIPAccessoryServerState_Stopping);
    } else {
        CloseSession(session);
    }
}

static void ReadInboundData(HAPIPSessionDescriptor* session) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPAccessoryServer* server = session->server;
    HAPAssert(session->tcpStreamIsOpen);

    HAPError err;

#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 1)
    if (server->ip.storage->dynamicMemoryAllocation.allocateMemory) {
        if (!session->inboundBuffer.data) {
            err = AllocateInboundBuffer(session, kHAPIPAccessoryServer_MaxEncryptedFrameBytes);
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                CloseSession(session);
                return;
            }
        }
    }
#endif

    HAPIPByteBuffer* b;
    b = &session->inboundBuffer;
    HAPAssert(b->data);
    HAPAssert(b->position <= b->limit);
    HAPAssert(b->limit <= b->capacity);

    size_t numBytes;
    err = HAPPlatformTCPStreamRead(
            HAPNonnull(server->platform.ip.tcpStreamManager),
            session->tcpStream,
            /* bytes: */ &b->data[b->position],
            /* maxBytes: */ b->limit - b->position,
            &numBytes);

    if (err == kHAPError_Unknown) {
        log_result(
                kHAPLogType_Error,
                "error:Function 'HAPPlatformTCPStreamRead' failed.",
                err,
                __func__,
                HAP_FILE,
                __LINE__);
        if (SessionIsLastSessionWithEnabledEventNotifications(session)) {
            CloseSession(session);
            RepublishHAPService(session->server);
        } else {
            CloseSession(session);
        }
        return;
    }
    if (err == kHAPError_Busy) {
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 1)
        if (server->ip.storage->dynamicMemoryAllocation.deallocateMemory) {
            if (b->position == 0) {
                DeallocateInboundBuffer(session);
            }
        }
#endif
        return;
    }

    HAPAssert(!err);
    if (numBytes == 0) {
        handle_input_closed(session);
    } else {
        HAPAssert(numBytes <= b->limit - b->position);
        b->position += numBytes;
        handle_input(session);
    }
}

static void HandleTCPStreamEvent(
        HAPPlatformTCPStreamManagerRef tcpStreamManager_,
        HAPPlatformTCPStreamRef tcpStream,
        HAPPlatformTCPStreamEvent event,
        void* _Nullable context) {
    HAPAssert(context);
    HAPIPSessionDescriptor* session = context;
    HAPAssert(session->server);
    HAPAccessoryServer* server = session->server;
    HAPAssert(tcpStreamManager_ == server->platform.ip.tcpStreamManager);
    HAPAssert(session->tcpStream == tcpStream);
    HAPAssert(session->tcpStreamIsOpen);

    HAPTime clock_now_ms = HAPPlatformClockGetCurrent();

    if (event.hasBytesAvailable) {
        HAPAssert(!event.hasSpaceAvailable);
        HAPAssert(session->state == kHAPIPSessionState_Reading);
        session->stamp = clock_now_ms;
        ReadInboundData(session);
        handle_io_progression(session);
    }

    if (event.hasSpaceAvailable) {
        HAPAssert(!event.hasBytesAvailable);
        HAPAssert(session->state == kHAPIPSessionState_Writing);
        session->stamp = clock_now_ms;
        WriteOutboundData(session);
        handle_io_progression(session);
    }
}

// Close the least recently used HAP session to free up space for a new inbound connection
static void CloseLeastRecentlyUsedHAPSession(HAPAccessoryServer* _Nonnull server) {
    HAPPrecondition(server);

    HAPTime now = HAPPlatformClockGetCurrent();
    HAPIPSessionDescriptor* candidate = NULL;
    HAPTime candidateDelta = 0;

    for (size_t i = 0; i < server->ip.storage->numSessions; i++) {
        HAPIPSession* ipSession = &server->ip.storage->sessions[i];
        HAPIPSessionDescriptor* session = &ipSession->descriptor;
        if (!session->server) {
            continue;
        }

        if ((session->state == kHAPIPSessionState_Reading) && (session->inboundBuffer.position == 0) &&
            !SessionHasPendingEventNotifications(session) &&
            (server->ip.state == kHAPIPAccessoryServerState_Stopping)) {
            // There is a stopping session.
            // Close this one. It's not necessary to search for the least recently used.
            candidate = session;
            break;
        }

        HAPAssert(now >= session->stamp);
        HAPTime dt_ms = now - session->stamp;
#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)
        HAPSession* hapSession = NULL;
        if (session->securitySession.isOpen && (session->securitySession.type == kHAPIPSecuritySessionType_HAP)) {
            hapSession = &session->securitySession._.hap;
        }

        if (hapSession != NULL) {
            if (HAPSessionHasActiveCameraStream(hapSession)) {
                // Active camera stream is considered to be using the session as of now.
                dt_ms = 0;
            }
        }
#endif
        HAPLogDebug(
                &logObject, "%s: session %p delta %llu ms", __func__, (void*) ipSession, (unsigned long long) dt_ms);

        if (!candidate) {
            candidate = session;
            candidateDelta = dt_ms;
            continue;
        }

        if (dt_ms > candidateDelta) {
            candidate = session;
            candidateDelta = dt_ms;
        }
    }
    CloseSession(candidate);
}

static void HandlePendingTCPStream(HAPPlatformTCPStreamManagerRef tcpStreamManager, void* _Nullable context) {
    HAPPrecondition(context);
    HAPAccessoryServer* server = context;
    HAPAssert(tcpStreamManager == server->platform.ip.tcpStreamManager);

    HAPError err;

    HAPPlatformTCPStreamRef tcpStream;
    err = HAPPlatformTCPStreamManagerAcceptTCPStream(HAPNonnull(server->platform.ip.tcpStreamManager), &tcpStream);
    if (err) {
        log_result(
                kHAPLogType_Error,
                "error:Function 'HAPPlatformTCPStreamManagerAcceptTCPStream' failed.",
                err,
                __func__,
                HAP_FILE,
                __LINE__);
        return;
    }

    if (server->ip.numSessions == server->ip.storage->numSessions - 1) {
        // Kick out least recently used session,
        // when number of session reached the max.
        // Note that an extra one in the storage is reserved to use as a free slot to fill in
        // before the kicked out session is completely destroyed.
        CloseLeastRecentlyUsedHAPSession(server);
    }

    // Find free IP session.
    HAPIPSession* ipSession = NULL;
    for (size_t i = 0; i < server->ip.storage->numSessions; i++) {
        HAPIPSessionDescriptor* descriptor = &server->ip.storage->sessions[i].descriptor;
        if (!descriptor->server) {
            ipSession = &server->ip.storage->sessions[i];
            break;
        }
    }
    if (!ipSession) {
        // This is only reachable if there are concurrently inbound new connections
        // when max number of sessions are full.
        // Because such a case is rare, doubling the memory resources to store
        // all pending new connections isn't worth and hence the back to back connections
        // are dropped instead.
        HAPLogError(&logObject, "%s: Failed to allocate session.", __func__);
        HAPPlatformTCPStreamClose(HAPNonnull(server->platform.ip.tcpStreamManager), tcpStream);
        return;
    }

    HAPIPSessionDescriptor* t = &ipSession->descriptor;
    HAPRawBufferZero(t, sizeof *t);
    t->server = server;
    t->tcpStream = tcpStream;
    t->tcpStreamIsOpen = true;
    t->state = kHAPIPSessionState_Idle;
    t->stamp = HAPPlatformClockGetCurrent();
    t->securitySession.isOpen = false;
    t->securitySession.isSecured = false;

#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 0)
    HAPAssert(ipSession->inboundBuffer.bytes);
    t->inboundBuffer.data = ipSession->inboundBuffer.bytes;
    t->inboundBuffer.capacity = ipSession->inboundBuffer.numBytes;
    t->inboundBuffer.limit = ipSession->inboundBuffer.numBytes;
#else
    if (!server->ip.storage->dynamicMemoryAllocation.allocateMemory) {
        HAPAssert(ipSession->inboundBuffer.bytes);
        t->inboundBuffer.data = HAPNonnullVoid(ipSession->inboundBuffer.bytes);
        t->inboundBuffer.capacity = ipSession->inboundBuffer.numBytes;
        t->inboundBuffer.limit = ipSession->inboundBuffer.numBytes;
    } else {
        HAPAssert(!ipSession->inboundBuffer.bytes);
        HAPAssert(!ipSession->inboundBuffer.numBytes);
    }
#endif
    t->inboundBuffer.position = 0;
    t->inboundBufferMark = 0;

#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 0)
    HAPAssert(ipSession->outboundBuffer.bytes);
    t->outboundBuffer.data = ipSession->outboundBuffer.bytes;
    t->outboundBuffer.capacity = ipSession->outboundBuffer.numBytes;
    t->outboundBuffer.limit = ipSession->outboundBuffer.numBytes;
#else
    if (!server->ip.storage->dynamicMemoryAllocation.allocateMemory) {
        HAPAssert(ipSession->outboundBuffer.bytes);
        t->outboundBuffer.data = HAPNonnullVoid(ipSession->outboundBuffer.bytes);
        t->outboundBuffer.capacity = ipSession->outboundBuffer.numBytes;
        t->outboundBuffer.limit = ipSession->outboundBuffer.numBytes;
    } else {
        HAPAssert(!ipSession->outboundBuffer.bytes);
        HAPAssert(!ipSession->outboundBuffer.numBytes);
    }
#endif
    t->outboundBuffer.position = 0;
    t->outboundBufferMark = 0;

#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 0)
    HAPAssert(ipSession->eventNotifications);
    t->eventNotifications = ipSession->eventNotifications;
    t->maxEventNotifications = ipSession->numEventNotifications;
#else
    if (!server->ip.storage->dynamicMemoryAllocation.allocateMemory) {
        HAPAssert(ipSession->eventNotifications);
        t->eventNotifications = HAPNonnullVoid(ipSession->eventNotifications);
        t->maxEventNotifications = ipSession->numEventNotifications;
    } else {
        HAPAssert(!ipSession->eventNotifications);
        HAPAssert(!ipSession->numEventNotifications);
        if (server->ip.numEventNotificationBitSetBytes) {
            t->eventNotificationSubscriptions = server->ip.storage->dynamicMemoryAllocation.allocateMemory(
                    server->ip.numEventNotificationBitSetBytes);
            if (!t->eventNotificationSubscriptions) {
                HAPLog(&logObject,
                       "session:%p:failed to allocate event notification storage (%zu)",
                       (const void*) t,
                       2 * server->ip.numEventNotificationBitSetBytes);
                HAPRawBufferZero(t, sizeof *t);
                HAPPlatformTCPStreamClose(HAPNonnull(server->platform.ip.tcpStreamManager), tcpStream);
                HAPLogDebug(&logObject, "session:%p: session released", (const void*) t);
                return;
            }
            HAPRawBufferZero(
                    HAPNonnullVoid(t->eventNotificationSubscriptions), server->ip.numEventNotificationBitSetBytes);
            t->eventNotificationFlags = server->ip.storage->dynamicMemoryAllocation.allocateMemory(
                    server->ip.numEventNotificationBitSetBytes);
            if (!t->eventNotificationFlags) {
                HAPLog(&logObject,
                       "session:%p:failed to allocate event notification storage (%zu)",
                       (const void*) t,
                       2 * server->ip.numEventNotificationBitSetBytes);
                HAPAssert(server->ip.storage->dynamicMemoryAllocation.deallocateMemory);
                server->ip.storage->dynamicMemoryAllocation.deallocateMemory(t->eventNotificationSubscriptions);
                HAPRawBufferZero(t, sizeof *t);
                HAPPlatformTCPStreamClose(HAPNonnull(server->platform.ip.tcpStreamManager), tcpStream);
                HAPLogDebug(&logObject, "session:%p: session released", (const void*) t);
                return;
            }
            HAPRawBufferZero(HAPNonnullVoid(t->eventNotificationFlags), server->ip.numEventNotificationBitSetBytes);
            HAPLogDebug(
                    &logObject,
                    "session:%p:allocated event notification storage (%zu)",
                    (const void*) t,
                    2 * server->ip.numEventNotificationBitSetBytes);
        }
    }
#endif
    t->numEventNotifications = 0;
    t->numEventNotificationFlags = 0;
    t->eventNotificationStamp = 0;

    t->timedWriteExpirationTime = 0;
    t->timedWritePID = 0;
    t->cameraSnapshotReader = NULL;

    OpenSecuritySession(t);
    t->state = kHAPIPSessionState_Reading;
    PrepareReadingRequest(t);
    HAPAssert(t->tcpStreamIsOpen);
    HAPPlatformTCPStreamEvent interests = { .hasBytesAvailable = true, .hasSpaceAvailable = false };
    HAPPlatformTCPStreamUpdateInterests(
            HAPNonnull(server->platform.ip.tcpStreamManager), t->tcpStream, interests, HandleTCPStreamEvent, t);

    RegisterSession(t);

    HAPLogDebug(&logObject, "session:%p:accepted", (const void*) t);
}

static void engine_init(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    HAPLogDebug(
            &logObject,
            "Storage configuration: ipAccessoryServerStorage = %lu",
            (unsigned long) sizeof *server->ip.storage);
    HAPLogDebug(
            &logObject, "Storage configuration: numSessions = %lu", (unsigned long) server->ip.storage->numSessions);
    HAPLogDebug(
            &logObject,
            "Storage configuration: sessions = %lu",
            (unsigned long) (server->ip.storage->numSessions * sizeof(HAPIPSession)));
    for (size_t i = 0; i < server->ip.storage->numSessions;) {
        size_t j;
        for (j = i + 1; j < server->ip.storage->numSessions; j++) {
            if (server->ip.storage->sessions[j].inboundBuffer.numBytes !=
                        server->ip.storage->sessions[i].inboundBuffer.numBytes ||
                server->ip.storage->sessions[j].outboundBuffer.numBytes !=
                        server->ip.storage->sessions[i].outboundBuffer.numBytes ||
                server->ip.storage->sessions[j].numEventNotifications !=
                        server->ip.storage->sessions[i].numEventNotifications) {
                break;
            }
        }
        if (i == j - 1) {
            HAPLogDebug(
                    &logObject,
                    "Storage configuration: sessions[%lu].inboundBuffer.numBytes = %lu",
                    (unsigned long) i,
                    (unsigned long) server->ip.storage->sessions[i].inboundBuffer.numBytes);
            HAPLogDebug(
                    &logObject,
                    "Storage configuration: sessions[%lu].outboundBuffer.numBytes = %lu",
                    (unsigned long) i,
                    (unsigned long) server->ip.storage->sessions[i].outboundBuffer.numBytes);
            HAPLogDebug(
                    &logObject,
                    "Storage configuration: sessions[%lu].numEventNotifications = %lu",
                    (unsigned long) i,
                    (unsigned long) server->ip.storage->sessions[i].numEventNotifications);
            HAPLogDebug(
                    &logObject,
                    "Storage configuration: sessions[%lu].eventNotifications = %lu",
                    (unsigned long) i,
                    (unsigned long) (server->ip.storage->sessions[i].numEventNotifications * sizeof(HAPIPEventNotification)));
        } else {
            HAPLogDebug(
                    &logObject,
                    "Storage configuration: sessions[%lu...%lu].inboundBuffer.numBytes = %lu",
                    (unsigned long) i,
                    (unsigned long) j - 1,
                    (unsigned long) server->ip.storage->sessions[i].inboundBuffer.numBytes);
            HAPLogDebug(
                    &logObject,
                    "Storage configuration: sessions[%lu...%lu].outboundBuffer.numBytes = %lu",
                    (unsigned long) i,
                    (unsigned long) j - 1,
                    (unsigned long) server->ip.storage->sessions[i].outboundBuffer.numBytes);
            HAPLogDebug(
                    &logObject,
                    "Storage configuration: sessions[%lu...%lu].numEventNotifications = %lu",
                    (unsigned long) i,
                    (unsigned long) j - 1,
                    (unsigned long) server->ip.storage->sessions[i].numEventNotifications);
            HAPLogDebug(
                    &logObject,
                    "Storage configuration: sessions[%lu...%lu].eventNotifications = %lu",
                    (unsigned long) i,
                    (unsigned long) j - 1,
                    (unsigned long) (server->ip.storage->sessions[i].numEventNotifications * sizeof(HAPIPEventNotification)));
        }
        i = j;
    }
    HAPLogDebug(
            &logObject,
            "Storage configuration: numReadContexts = %lu",
            (unsigned long) server->ip.storage->numReadContexts);
    HAPLogDebug(
            &logObject,
            "Storage configuration: readContexts = %lu",
            (unsigned long) (server->ip.storage->numReadContexts * sizeof(HAPIPReadContext)));
    HAPLogDebug(
            &logObject,
            "Storage configuration: numWriteContexts = %lu",
            (unsigned long) server->ip.storage->numWriteContexts);
    HAPLogDebug(
            &logObject,
            "Storage configuration: writeContexts = %lu",
            (unsigned long) (server->ip.storage->numWriteContexts * sizeof(HAPIPWriteContext)));
    HAPLogDebug(
            &logObject,
            "Storage configuration: scratchBuffer.numBytes = %lu",
            (unsigned long) server->ip.storage->scratchBuffer.numBytes);

    HAPAssert(server->ip.state == kHAPIPAccessoryServerState_Undefined);

    server->ip.state = kHAPIPAccessoryServerState_Idle;
    server->ip.nextState = kHAPIPAccessoryServerState_Undefined;
}

HAP_RESULT_USE_CHECK
static HAPError engine_deinit(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    HAPAssert(server->ip.state == kHAPIPAccessoryServerState_Idle);

    HAPIPAccessoryServerStorage* storage = HAPNonnull(server->ip.storage);

    HAPAssert(storage->readContexts);
    HAPRawBufferZero(storage->readContexts, storage->numReadContexts * sizeof *storage->readContexts);

    HAPAssert(storage->writeContexts);
    HAPRawBufferZero(storage->writeContexts, storage->numWriteContexts * sizeof *storage->writeContexts);

    HAPAssert(storage->scratchBuffer.bytes);
    HAPRawBufferZero(storage->scratchBuffer.bytes, storage->scratchBuffer.numBytes);

    server->ip.state = kHAPIPAccessoryServerState_Undefined;

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static HAPAccessoryServerState engine_get_state(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    switch (server->ip.state) {
        case kHAPIPAccessoryServerState_Idle: {
            return kHAPAccessoryServerState_Idle;
        }
        case kHAPIPAccessoryServerState_Running: {
            return kHAPAccessoryServerState_Running;
        }
        case kHAPIPAccessoryServerState_Stopping: {
            if (server->ip.nextState == kHAPIPAccessoryServerState_Running) {
                return kHAPAccessoryServerState_Running;
            }
            HAPAssert(server->ip.nextState == kHAPIPAccessoryServerState_Idle);
            return kHAPAccessoryServerState_Stopping;
        }
        case kHAPIPAccessoryServerState_Undefined:
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
bool HAPAccessoryServerIsInWACMode(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    if (!server->caps.wac) {
        return false;
    }

    HAPPrecondition(server->ip.state != kHAPIPAccessoryServerState_Undefined);

    return server->ip.isInWACMode || server->ip.isInWACModeTransition;
}

static void handle_server_state_transition_timer(HAPPlatformTimerRef timer, void* _Nullable context) {
    HAPPrecondition(context);
    HAPAccessoryServer* server = context;
    HAPPrecondition(timer == server->ip.stateTransitionTimer);
    server->ip.stateTransitionTimer = 0;

    HAPAssert(server->ip.state == kHAPIPAccessoryServerState_Stopping);
    schedule_max_idle_time_timer(server);
}

static void schedule_server_state_transition(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    HAPAssert(server->ip.state == kHAPIPAccessoryServerState_Stopping);

    HAPError err;

    if (!server->ip.stateTransitionTimer) {
        err = HAPPlatformTimerRegister(
                &server->ip.stateTransitionTimer, 0, handle_server_state_transition_timer, server);
        if (err) {
            HAPLog(&logObject, "Not enough resources to schedule accessory server state transition!");
            HAPFatalError();
        }
        HAPAssert(server->ip.stateTransitionTimer);
    }
}

/**
 * Closes all the HAP IP sessions
 *
 * @param      server          Accessory server ref
 *
 */
void HAPAccessoryServerCloseTCPSessions(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    // Closes all the HAP IP sessions
    for (size_t i = 0; i < server->ip.storage->numSessions; i++) {
        HAPIPSession* ipSession = &server->ip.storage->sessions[i];
        HAPIPSessionDescriptor* session = &ipSession->descriptor;
        if (!session->server) {
            continue;
        }
        if (session->tcpStreamIsOpen) {
            if (SessionIsLastSessionWithEnabledEventNotifications(session)) {
                CloseSession(session);
                RepublishHAPService(server);
            } else {
                CloseSession(session);
            }
        }
    }
}

/**
 * Closes all the HAP IP sessions and the TCP listener.
 *
 * @param      server          Accessory server ref
 *
 */
void HAPAccessoryServerCloseTCPSessionsAndListener(HAPAccessoryServer* server) {
    HAPPrecondition(server);
    if (HAPPlatformTCPStreamManagerIsListenerOpen(HAPNonnull(server->platform.ip.tcpStreamManager))) {
        HAPPlatformTCPStreamManagerCloseListener(HAPNonnull(server->platform.ip.tcpStreamManager));
    }
    HAPAccessoryServerCloseTCPSessions(server);
}

/**
 * Opens the listener for TCP and subsequently HAP IP sessions
 *
 * @param      server          Accessory server ref
 *
 */
void HAPAccessoryServerOpenTCPListener(HAPAccessoryServer* server) {
    HAPPrecondition(server);
    if (!HAPPlatformTCPStreamManagerIsListenerOpen(HAPNonnull(server->platform.ip.tcpStreamManager))) {
        HAPPlatformTCPStreamManagerOpenListener(
                HAPNonnull(server->platform.ip.tcpStreamManager), HandlePendingTCPStream, server);
    }
}

#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_RECONFIGURATION)
/**
 * Increments the configuration number, opens up HAP IP sessions and starts HAP Bonjour service discovery
 *
 * @param      server          Accessory server ref
 *
 */
void HAPAccessoryServerHandleWiFiReconfigurationDone(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    HAPError err = HAPAccessoryServerIncrementCN(server->platform.keyValueStore);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Failed to increment Configuration Number.");
    }
    // Update BLE advertising state to reflect the new configuration number.
    HAPAccessoryServerUpdateAdvertisingData(server);

    uint16_t configurationNumber;
    err = HAPAccessoryServerGetCN(server->platform.keyValueStore, &configurationNumber);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Failed to fetch Configuration Number.");
    }
    HAPLogInfo(&logObject, "HAP Configuration Number updated: %u.", configurationNumber);

    // Start _hap Bonjour service discovery.
    HAPPlatformTCPStreamManagerSetTCPUserTimeout(
            HAPNonnull(server->platform.ip.tcpStreamManager), kHAPIPAccessoryServer_TCPUserTimeout);
    HAPPlatformTCPStreamManagerOpenListener(
            HAPNonnull(server->platform.ip.tcpStreamManager), HandlePendingTCPStream, server);
    HAPAssert(HAPPlatformTCPStreamManagerIsListenerOpen(HAPNonnull(server->platform.ip.tcpStreamManager)));
    publish_homeKit_service(server);
}
#endif

void HAPAccessoryServerEnterWACMode(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    if (!server->caps.wac) {
        HAPLog(&logObject, "%s: WAC is not available.", __func__);
        return;
    }

    HAPLogDebug(&logObject, "%s", __func__);
    if (server->ip.state == kHAPIPAccessoryServerState_Running) {
        if (!server->ip.isInWACMode) {
            server->ip.isInWACModeTransition = true;
            HAPAccessoryServerDelegateScheduleHandleUpdatedState(server);

            HAPAssert(server->ip.nextState == kHAPIPAccessoryServerState_Undefined);
            server->ip.state = kHAPIPAccessoryServerState_Stopping;
            server->ip.nextState = kHAPIPAccessoryServerState_Running;
            schedule_server_state_transition(server);
        }
    } else if (server->ip.state == kHAPIPAccessoryServerState_Stopping) {
        if (server->ip.nextState == kHAPIPAccessoryServerState_Running) {
            if (!server->ip.isInWACModeTransition) {
                server->ip.isInWACModeTransition = true;
                HAPAccessoryServerDelegateScheduleHandleUpdatedState(server);
            }
        } else {
            HAPAssert(server->ip.nextState == kHAPIPAccessoryServerState_Idle);
        }
    } else {
        HAPPrecondition(server->ip.state == kHAPIPAccessoryServerState_Idle);
    }
}

void HAPAccessoryServerExitWACMode(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    if (!server->caps.wac) {
        HAPLog(&logObject, "%s: WAC is not available.", __func__);
        return;
    }

    HAPLogDebug(&logObject, "%s", __func__);
    if (server->ip.state == kHAPIPAccessoryServerState_Running) {
        if (server->ip.isInWACMode) {
            if (server->ip.wacModeTimer) {
                HAPPlatformTimerDeregister(server->ip.wacModeTimer);
                server->ip.wacModeTimer = 0;
            }

            if (server->ip.wacConfiguredMessageTimer) {
                HAPPlatformTimerDeregister(server->ip.wacConfiguredMessageTimer);
                server->ip.wacConfiguredMessageTimer = 0;
            }
            HAPAssert(!server->ip.isInWACModeTransition);
            if (server->ip.checkWiFiStatusTimer) {
                HAPPlatformTimerDeregister(server->ip.checkWiFiStatusTimer);
                server->ip.checkWiFiStatusTimer = 0;
            }

            HAPAssert(server->ip.nextState == kHAPIPAccessoryServerState_Undefined);
            server->ip.state = kHAPIPAccessoryServerState_Stopping;
            server->ip.nextState = kHAPIPAccessoryServerState_Running;
            schedule_server_state_transition(server);
        }
    } else if (server->ip.state == kHAPIPAccessoryServerState_Stopping) {
        if (server->ip.nextState == kHAPIPAccessoryServerState_Running) {
            if (server->ip.isInWACModeTransition) {
                server->ip.isInWACModeTransition = false;
                HAPAccessoryServerDelegateScheduleHandleUpdatedState(server);
            }
        } else {
            HAPAssert(server->ip.nextState == kHAPIPAccessoryServerState_Idle);
        }
    } else {
        HAPPrecondition(server->ip.state == kHAPIPAccessoryServerState_Idle);
    }
}

static void engine_start(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    HAPAssert(server->ip.state == kHAPIPAccessoryServerState_Idle);

    HAPLogDebug(&logObject, "Starting server engine.");

#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 1)
    if (!server->ip.storage->dynamicMemoryAllocation.allocateMemory) {
        server->ip.numEventNotificationBitSetBytes = 0;
    } else {
        size_t numCharacteristicsSupportingEventNotification =
                HAPAccessoryServerGetNumCharacteristicsSupportingEventNotification(server);
        server->ip.numEventNotificationBitSetBytes = numCharacteristicsSupportingEventNotification / CHAR_BIT;
        if (numCharacteristicsSupportingEventNotification % CHAR_BIT) {
            server->ip.numEventNotificationBitSetBytes++;
        }
    }
#endif

    server->ip.state = kHAPIPAccessoryServerState_Running;
    HAPAccessoryServerDelegateScheduleHandleUpdatedState(server);

    HAPAssert(!HAPPlatformTCPStreamManagerIsListenerOpen(HAPNonnull(server->platform.ip.tcpStreamManager)));

    HAPAssert(!server->ip.isInWACMode);

    if (server->caps.wac && !HAPPlatformWiFiManagerIsConfigured(HAPNonnull(server->platform.ip.wiFi.wiFiManager))) {
        handle_wac_mode(server);
        HAPAssert(server->ip.isInWACMode);
    } else {
        HAPPlatformTCPStreamManagerSetTCPUserTimeout(
                HAPNonnull(server->platform.ip.tcpStreamManager), kHAPIPAccessoryServer_TCPUserTimeout);
        HAPPlatformTCPStreamManagerOpenListener(
                HAPNonnull(server->platform.ip.tcpStreamManager), HandlePendingTCPStream, server);
        HAPAssert(HAPPlatformTCPStreamManagerIsListenerOpen(HAPNonnull(server->platform.ip.tcpStreamManager)));
        publish_homeKit_service(server);
    }
}

HAP_RESULT_USE_CHECK
static HAPError engine_stop(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    HAPLogDebug(&logObject, "Stopping server engine.");

    switch (server->ip.state) {
        case kHAPIPAccessoryServerState_Undefined:
        case kHAPIPAccessoryServerState_Idle: {
            break;
        }
        case kHAPIPAccessoryServerState_Running: {
            if (server->ip.wacModeTimer) {
                HAPPlatformTimerDeregister(server->ip.wacModeTimer);
                server->ip.wacModeTimer = 0;
            }

            if (server->ip.wacConfiguredMessageTimer) {
                HAPPlatformTimerDeregister(server->ip.wacConfiguredMessageTimer);
                server->ip.wacConfiguredMessageTimer = 0;
            }
            HAPAssert(!server->ip.isInWACModeTransition);
            if (server->ip.checkWiFiStatusTimer) {
                HAPPlatformTimerDeregister(server->ip.checkWiFiStatusTimer);
                server->ip.checkWiFiStatusTimer = 0;
            }

#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_RECONFIGURATION)
            // Deregister any timers that were registered for Wifi Reconfiguration
            if (server->ip.updateWiFiConfigurationTimer) {
                HAPPlatformTimerDeregister(server->ip.updateWiFiConfigurationTimer);
                server->ip.updateWiFiConfigurationTimer = 0;
            }
            if (server->ip.failSafeUpdateTimeoutTimer) {
                HAPPlatformTimerDeregister(server->ip.failSafeUpdateTimeoutTimer);
                server->ip.failSafeUpdateTimeoutTimer = 0;
            }
#endif
            if (server->ip.checkWiFiStatusTimer) {
                HAPPlatformTimerDeregister(server->ip.checkWiFiStatusTimer);
                server->ip.checkWiFiStatusTimer = 0;
            }
            HAPAssert(server->ip.nextState == kHAPIPAccessoryServerState_Undefined);
            server->ip.state = kHAPIPAccessoryServerState_Stopping;
            server->ip.nextState = kHAPIPAccessoryServerState_Idle;
            HAPAccessoryServerDelegateScheduleHandleUpdatedState(server);
            schedule_server_state_transition(server);
            break;
        }
        case kHAPIPAccessoryServerState_Stopping: {
            if (server->ip.nextState == kHAPIPAccessoryServerState_Running) {
                if (server->ip.isInWACModeTransition) {
                    server->ip.isInWACModeTransition = false;
                    HAPAccessoryServerDelegateScheduleHandleUpdatedState(server);
                }
                server->ip.nextState = kHAPIPAccessoryServerState_Idle;
            } else {
                HAPAssert(server->ip.nextState == kHAPIPAccessoryServerState_Idle);
            }
            break;
        }
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static HAPError engine_raise_event_on_session_(
        HAPAccessoryServer* server,
        const HAPCharacteristic* characteristic_,
        const HAPService* service_,
        const HAPAccessory* accessory_,
        const HAPSession* _Nullable securitySession_) {
    HAPPrecondition(server);

    HAPPrecondition(characteristic_);
    HAPPrecondition(service_);
    HAPPrecondition(accessory_);

    HAPError err;

    bool hasHAPSessions = false;
    size_t events_raised = 0;

    uint64_t aid = accessory_->aid;
    uint64_t iid = ((const HAPBaseCharacteristic*) characteristic_)->iid;

    if (server->ip.state == kHAPIPAccessoryServerState_Idle) {
        HAPLog(&logObject, "Ignoring event notification (accessory server is idle).");
        return kHAPError_None;
    }
    if (server->ip.state == kHAPIPAccessoryServerState_Stopping) {
        if (server->ip.nextState == kHAPIPAccessoryServerState_Running) {
            HAPLog(&logObject, "Ignoring event notification (accessory server is restarting).");
        } else {
            HAPAssert(server->ip.nextState == kHAPIPAccessoryServerState_Idle);
            HAPLog(&logObject, "Ignoring event notification (accessory server is stopping).");
        }
        return kHAPError_None;
    }
    HAPPrecondition(server->ip.state == kHAPIPAccessoryServerState_Running);

    for (size_t i = 0; i < server->ip.storage->numSessions; i++) {
        HAPIPSession* ipSession = &server->ip.storage->sessions[i];
        HAPIPSessionDescriptor* session = &ipSession->descriptor;
        if (!session->server) {
            continue;
        }
        if (session->securitySession.type != kHAPIPSecuritySessionType_HAP) {
            if (!securitySession_) {
                HAPLogDebug(&logObject, "Not flagging event pending on non-HAP session.");
            }
            continue;
        }

        hasHAPSessions = true;

        if (securitySession_ && (securitySession_ != &session->securitySession._.hap)) {
            continue;
        }
        if (HAPSessionIsTransient(&session->securitySession._.hap)) {
            HAPLogDebug(&logObject, "Not flagging event pending on transient session.");
            continue;
        }

        if (HAPCharacteristicIsHandlingWrite(
                    server, &session->securitySession._.hap, characteristic_, service_, accessory_)) {
            HAPLogCharacteristicInfo(
                    &logObject,
                    characteristic_,
                    service_,
                    accessory_,
                    "Not sending event notification to the controller that performed the write.");
        } else {
            if (session->eventNotifications) {
                HAPAssert(session->numEventNotifications <= session->maxEventNotifications);
                size_t j = 0;
                while ((j < session->numEventNotifications) &&
                       ((session->eventNotifications[j].aid != aid) || (session->eventNotifications[j].iid != iid))) {
                    j++;
                }
                HAPAssert(
                        (j == session->numEventNotifications) ||
                        ((j < session->numEventNotifications) && (session->eventNotifications[j].aid == aid) &&
                         (session->eventNotifications[j].iid == iid)));
                if ((j < session->numEventNotifications) && !session->eventNotifications[j].flag) {
                    session->eventNotifications[j].flag = true;
                    session->numEventNotificationFlags++;
                    events_raised++;
                }
            } else {
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 0)
                HAPAssertionFailure();
#else
                if (session->eventNotificationSubscriptions) {
                    HAPAssert(session->eventNotificationFlags);
                    size_t eventNotificationIndex;
                    bool eventNotificationIndexFound;
                    HAPAccessoryServerGetEventNotificationIndex(
                            HAPNonnull(session->server),
                            aid,
                            iid,
                            &eventNotificationIndex,
                            &eventNotificationIndexFound);
                    if (eventNotificationIndexFound &&
                        HAPBitSetContains(
                                HAPNonnull(session->eventNotificationSubscriptions),
                                server->ip.numEventNotificationBitSetBytes,
                                eventNotificationIndex) &&
                        !HAPBitSetContains(
                                HAPNonnull(session->eventNotificationFlags),
                                server->ip.numEventNotificationBitSetBytes,
                                eventNotificationIndex)) {
                        HAPBitSetInsert(
                                HAPNonnull(session->eventNotificationFlags),
                                server->ip.numEventNotificationBitSetBytes,
                                eventNotificationIndex);
                        events_raised++;
                    }
                } else {
                    HAPAssert(!session->eventNotificationSubscriptions);
                    HAPAssert(!session->eventNotificationFlags);
                }
#endif
            }
        }
    }

    if (!hasHAPSessions && !server->ip.gsnDidIncrement && server->ip.isServiceDiscoverable) {
        HAPLogInfo(&logObject, "Raising an event while in disconnected state.");
        if (!HAPAccessoryServerIsPaired(server)) {
            HAPLog(&logObject, "Not re-publishing HomeKit service because accessory server is not paired.");
        } else {
            HAPLog(&logObject, "Re-publishing HomeKit service.");
            HAPIPServiceDiscoverySetHAPService(server);
            server->ip.gsnDidIncrement = true;
        }
    }

    if (events_raised) {
        if (server->ip.eventNotificationTimer) {
            HAPPlatformTimerDeregister(server->ip.eventNotificationTimer);
            server->ip.eventNotificationTimer = 0;
        }
        err = HAPPlatformTimerRegister(&server->ip.eventNotificationTimer, 0, handle_event_notification_timer, server);
        if (err) {
            HAPLog(&logObject, "Not enough resources to schedule event notification timer!");
            HAPFatalError();
        }
        HAPAssert(server->ip.eventNotificationTimer);
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static HAPError engine_raise_event(
        HAPAccessoryServer* server,
        const HAPCharacteristic* characteristic,
        const HAPService* service,
        const HAPAccessory* accessory) {
    HAPPrecondition(server);
    HAPPrecondition(characteristic);
    HAPPrecondition(service);
    HAPPrecondition(accessory);

    return engine_raise_event_on_session_(server, characteristic, service, accessory, /* session: */ NULL);
}

HAP_RESULT_USE_CHECK
static HAPError engine_raise_event_on_session(
        HAPAccessoryServer* server,
        const HAPCharacteristic* characteristic,
        const HAPService* service,
        const HAPAccessory* accessory,
        const HAPSession* session) {
    HAPPrecondition(server);
    HAPPrecondition(characteristic);
    HAPPrecondition(service);
    HAPPrecondition(accessory);
    HAPPrecondition(session);

    return engine_raise_event_on_session_(server, characteristic, service, accessory, session);
}

static void Create(HAPAccessoryServer* server, const HAPAccessoryServerOptions* options) {
    HAPPrecondition(server);

    HAPPrecondition(server->platform.ip.tcpStreamManager);
    HAPPrecondition(server->platform.ip.serviceDiscovery);

    // Initialize IP storage.
    HAPPrecondition(options->ip.accessoryServerStorage);
    HAPIPAccessoryServerStorage* storage = options->ip.accessoryServerStorage;

#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 1)
    HAPPrecondition(
            (!storage->dynamicMemoryAllocation.allocateMemory && !storage->dynamicMemoryAllocation.reallocateMemory &&
             !storage->dynamicMemoryAllocation.deallocateMemory) ||
            (storage->dynamicMemoryAllocation.allocateMemory && storage->dynamicMemoryAllocation.reallocateMemory &&
             storage->dynamicMemoryAllocation.deallocateMemory && options->ip.transport));
#endif
    HAPPrecondition(storage->readContexts);
    HAPPrecondition(storage->writeContexts);
    HAPPrecondition(storage->scratchBuffer.bytes);
    HAPPrecondition(storage->sessions);
    HAPPrecondition(storage->numSessions);
    for (size_t i = 0; i < storage->numSessions; i++) {
        HAPIPSession* session = &storage->sessions[i];
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 0)
        HAPPrecondition(session->inboundBuffer.bytes);
        HAPPrecondition(session->outboundBuffer.bytes);
        HAPPrecondition(session->eventNotifications);
#else
        if (!storage->dynamicMemoryAllocation.allocateMemory) {
            HAPPrecondition(session->inboundBuffer.bytes);
            HAPPrecondition(session->outboundBuffer.bytes);
            HAPPrecondition(session->eventNotifications);
        } else {
            HAPPrecondition(!session->inboundBuffer.bytes);
            HAPPrecondition(!session->inboundBuffer.numBytes);
            HAPPrecondition(!session->outboundBuffer.bytes);
            HAPPrecondition(!session->outboundBuffer.numBytes);
            HAPPrecondition(!session->eventNotifications);
            HAPPrecondition(!session->numEventNotifications);
        }
#endif
    }
    HAPRawBufferZero(storage->readContexts, storage->numReadContexts * sizeof *storage->readContexts);
    HAPRawBufferZero(storage->writeContexts, storage->numWriteContexts * sizeof *storage->writeContexts);
    HAPRawBufferZero(storage->scratchBuffer.bytes, storage->scratchBuffer.numBytes);
    for (size_t i = 0; i < storage->numSessions; i++) {
        HAPIPSession* ipSession = &storage->sessions[i];
        HAPRawBufferZero(&ipSession->descriptor, sizeof ipSession->descriptor);
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 0)
        HAPRawBufferZero(ipSession->inboundBuffer.bytes, ipSession->inboundBuffer.numBytes);
        HAPRawBufferZero(ipSession->outboundBuffer.bytes, ipSession->outboundBuffer.numBytes);
        HAPRawBufferZero(
                ipSession->eventNotifications,
                ipSession->numEventNotifications * sizeof *ipSession->eventNotifications);
#else
        if (ipSession->inboundBuffer.bytes) {
            HAPRawBufferZero(HAPNonnullVoid(ipSession->inboundBuffer.bytes), ipSession->inboundBuffer.numBytes);
        }
        if (ipSession->outboundBuffer.bytes) {
            HAPRawBufferZero(HAPNonnullVoid(ipSession->outboundBuffer.bytes), ipSession->outboundBuffer.numBytes);
        }
        if (ipSession->eventNotifications) {
            HAPRawBufferZero(
                    HAPNonnullVoid(ipSession->eventNotifications),
                    ipSession->numEventNotifications * sizeof *ipSession->eventNotifications);
        }
#endif
    }
    server->ip.storage = options->ip.accessoryServerStorage;

    // Install server engine.
    HAPNonnull(server->transports.ip)->serverEngine.install(server);

    // Copy WAC parameters.
    server->caps.wac = options->ip.wac.available;
    if (options->ip.wac.available) {
        HAPPrecondition(server->platform.ip.wiFi.wiFiManager);
        HAPPrecondition(server->platform.ip.wiFi.softwareAccessPoint);
        HAP_DIAGNOSTIC_PUSH
        HAP_DIAGNOSTIC_IGNORED_CLANG("-Wdeprecated-declarations")
        HAP_DIAGNOSTIC_IGNORED_GCC("-Wdeprecated-declarations")
        HAP_DIAGNOSTIC_IGNORED_ARMCC(2570)
        HAP_DIAGNOSTIC_IGNORED_ICCARM(Pe1444)

        bool supportsAuthentication = false;
        if (HAPAccessoryServerSupportsMFiHWAuth(server)) {
            supportsAuthentication = true;
        }
        if (server->platform.authentication.mfiTokenAuth) {
            supportsAuthentication = true;
        }
        if (!supportsAuthentication) {
            HAPLogError(
                    &logObject,
                    "An MFi authentication strategy must be supported for Wi-Fi Accessory Configuration (WAC).");
            HAPFatalError();
        }
        HAP_DIAGNOSTIC_RESTORE_ICCARM(Pe1444)
        HAP_DIAGNOSTIC_POP
    } else {
        if ((server->platform.authentication.mfiHWAuth) && !HAPAccessoryServerSupportsMFiHWAuth(server)) {
            HAPLogError(&logObject, "An MFi authentication strategy must be supported for HW Auth");
            HAPFatalError();
        }
    }

#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)
    // Copy IP Camera parameters.
    if (options->ip.camera.streamingSessionStorage) {
        size_t numSessions = options->ip.camera.streamingSessionStorage->numSessions;
        HAPRawBufferZero(
                options->ip.camera.streamingSessionStorage->sessions,
                numSessions * sizeof *options->ip.camera.streamingSessionStorage->sessions);

        size_t numSetups = options->ip.camera.streamingSessionStorage->numSetups;
        HAPRawBufferZero(
                options->ip.camera.streamingSessionStorage->setups,
                numSetups * sizeof *options->ip.camera.streamingSessionStorage->setups);

        HAPAssert(
                sizeof server->ip.camera.streamingSessionStorage == sizeof *options->ip.camera.streamingSessionStorage);
        HAPRawBufferCopyBytes(
                &server->ip.camera.streamingSessionStorage,
                HAPNonnull(options->ip.camera.streamingSessionStorage),
                sizeof server->ip.camera.streamingSessionStorage);
    }
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_ACCESS_CODE)
    // Copy Access Code service parameters
    if (options->accessCode.responseStorage) {
        HAPAssert(options->accessCode.handleOperation);

        HAPRawBufferCopyBytes(
                &server->accessCode.responseStorage,
                HAPNonnull(options->accessCode.responseStorage),
                sizeof server->accessCode.responseStorage);

        server->accessCode.handleOperation = options->accessCode.handleOperation;
        server->accessCode.operationCtx = options->accessCode.operationCtx;
    }
    server->accessCode.numResponseBytes = 0;
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_NFC_ACCESS)
    // Copy NFC Access service parameters
    if (options->nfcAccess.responseStorage) {
        HAPRawBufferCopyBytes(
                &server->nfcAccess.responseStorage,
                HAPNonnull(options->nfcAccess.responseStorage),
                sizeof server->nfcAccess.responseStorage);
    }
    server->nfcAccess.numResponseBytes = 0;
#endif
}

static void PrepareStart(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    HAPIPAccessoryServerStorage* storage = HAPNonnull(server->ip.storage);
    HAPRawBufferZero(storage->readContexts, storage->numReadContexts * sizeof *storage->readContexts);
    HAPRawBufferZero(storage->writeContexts, storage->numWriteContexts * sizeof *storage->writeContexts);
    HAPRawBufferZero(storage->scratchBuffer.bytes, storage->scratchBuffer.numBytes);
    for (size_t i = 0; i < storage->numSessions; i++) {
        HAPIPSession* ipSession = &storage->sessions[i];
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 0)
        HAPRawBufferZero(ipSession->inboundBuffer.bytes, ipSession->inboundBuffer.numBytes);
        HAPRawBufferZero(ipSession->outboundBuffer.bytes, ipSession->outboundBuffer.numBytes);
        HAPRawBufferZero(
                ipSession->eventNotifications,
                ipSession->numEventNotifications * sizeof *ipSession->eventNotifications);
#else
        if (ipSession->inboundBuffer.bytes) {
            HAPRawBufferZero(HAPNonnullVoid(ipSession->inboundBuffer.bytes), ipSession->inboundBuffer.numBytes);
        }
        if (ipSession->outboundBuffer.bytes) {
            HAPRawBufferZero(HAPNonnullVoid(ipSession->outboundBuffer.bytes), ipSession->outboundBuffer.numBytes);
        }
        if (ipSession->eventNotifications) {
            HAPRawBufferZero(
                    HAPNonnullVoid(ipSession->eventNotifications),
                    ipSession->numEventNotifications * sizeof *ipSession->eventNotifications);
        }
#endif
    }
}

static void WillStart(HAPAccessoryServer* server HAP_UNUSED) {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)
    HAPPrecondition(server);

    HAPIPAccessoryServerStorage* storage = HAPNonnull(server->ip.storage);
    size_t numCameraStreamServices =
            HAPAccessoryServerGetNumServiceInstances(server, &kHAPServiceType_CameraRTPStreamManagement);
    if (numCameraStreamServices) {
        if (server->ip.camera.streamingSessionStorage.numSessions < numCameraStreamServices) {
            HAPLogError(
                    &logObject,
                    "One HAPCameraStreamingSession is required per "
                    "Camera RTP Stream Management Service (%zu required).",
                    numCameraStreamServices);
            HAPPreconditionFailure();
        }
        HAPPrecondition(server->ip.camera.streamingSessionStorage.sessions);

        if (server->ip.camera.streamingSessionStorage.numSetups < storage->numSessions * numCameraStreamServices) {
            HAPLogError(
                    &logObject,
                    "One HAPCameraStreamingSessionSetup is required per "
                    "concurrently supported IP connection and per "
                    "Camera RTP Stream Management Service (%zu required).",
                    storage->numSessions * numCameraStreamServices);
            HAPPreconditionFailure();
        }
        HAPPrecondition(server->ip.camera.streamingSessionStorage.setups);
    }
#endif
}

static void Start(HAPAccessoryServer* server HAP_UNUSED) {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)
    HAPPrecondition(server);

    // Set up camera event recordings.
    if (server->primaryAccessory->services) {
        for (size_t i = 0; server->primaryAccessory->services[i]; i++) {
            const HAPService* service = server->primaryAccessory->services[i];
            if (HAPUUIDAreEqual(service->serviceType, &kHAPServiceType_CameraEventRecordingManagement)) {
                HAPCameraAccessoryHandleSupportedRecordingConfigurationChange(
                        server, service, HAPNonnull(server->primaryAccessory), /* changes: */ 0);
            }
        }
    }
    if (server->ip.bridgedAccessories) {
        for (size_t i = 0; server->ip.bridgedAccessories[i]; i++) {
            const HAPAccessory* accessory = server->ip.bridgedAccessories[i];
            if (accessory->services) {
                for (size_t j = 0; accessory->services[j]; j++) {
                    const HAPService* service = accessory->services[j];
                    if (HAPUUIDAreEqual(service->serviceType, &kHAPServiceType_CameraEventRecordingManagement)) {
                        HAPCameraAccessoryHandleSupportedRecordingConfigurationChange(
                                server, service, accessory, /* changes: */ 0);
                    }
                }
            }
        }
    }
#endif
}

static void PrepareStop(HAPAccessoryServer* server HAP_UNUSED) {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)
    HAPPrecondition(server);

    // Remove streaming events delegate.
    HAPPlatformCameraRef camera = server->platform.ip.camera;
    if (camera) {
        HAPPlatformCameraSetDelegate(camera, NULL);
    }
    if (server->ip.bridgedAccessories && server->ip.bridgedCameras) {
        const HAPPlatformCameraRef _Nullable* cameras = server->ip.bridgedCameras;
        for (size_t i = 0; server->ip.bridgedAccessories[i]; i++) {
            if (cameras[i]) {
                HAPPlatformCameraSetDelegate(HAPNonnull(cameras[i]), NULL);
            }
        }
    }
#endif
}

static void HAPSessionInvalidateDependentIPState(HAPAccessoryServer* server, HAPSession* session) {
    HAPPrecondition(server);
    HAPPrecondition(session);

#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)
    // Invalidate attached camera streams.
    HAPInvalidateCameraStreamForSession(server, session);
#endif

    // Invalidate attached HomeKit Data Streams.
    HAPDataStreamInvalidateAllForHAPSession(server, session);
}

static void HAPAccessoryServerInstallServerEngine(HAPAccessoryServer* server) {
    HAPPrecondition(server);
    HAPPrecondition(!server->_serverEngine);

    server->_serverEngine = &HAPIPAccessoryServerServerEngine;
}

static void HAPAccessoryServerUninstallServerEngine(HAPAccessoryServer* server) {
    HAPPrecondition(server);
    server->_serverEngine = NULL;
}

static const HAPAccessoryServerServerEngine* _Nullable HAPAccessoryServerGetServerEngine(HAPAccessoryServer* server) {
    return server->_serverEngine;
}

const HAPIPAccessoryServerTransport kHAPAccessoryServerTransport_IP = {
    .create = Create,
    .prepareStart = PrepareStart,
    .willStart = WillStart,
    .start = Start,
    .prepareStop = PrepareStop,
    .session = { .invalidateDependentIPState = HAPSessionInvalidateDependentIPState },
    .serverEngine = { .install = HAPAccessoryServerInstallServerEngine,
                      .uninstall = HAPAccessoryServerUninstallServerEngine,
                      .get = HAPAccessoryServerGetServerEngine }
};

const HAPAccessoryServerServerEngine HAPIPAccessoryServerServerEngine = { .init = engine_init,
                                                                          .deinit = engine_deinit,
                                                                          .get_state = engine_get_state,
                                                                          .start = engine_start,
                                                                          .stop = engine_stop,
                                                                          .raise_event = engine_raise_event,
                                                                          .raise_event_on_session =
                                                                                  engine_raise_event_on_session };

HAP_RESULT_USE_CHECK
size_t HAPAccessoryServerGetIPSessionIndex(const HAPAccessoryServer* server, const HAPSession* session) {
    HAPPrecondition(server);
    HAPPrecondition(session);

    const HAPIPAccessoryServerStorage* storage = HAPNonnull(server->ip.storage);

    for (size_t i = 0; i < storage->numSessions; i++) {
        HAPIPSessionDescriptor* t = &storage->sessions[i].descriptor;
        if (!t->server) {
            continue;
        }
        if (t->securitySession.type != kHAPIPSecuritySessionType_HAP) {
            continue;
        }
        if (&t->securitySession._.hap == session) {
            return i;
        }
    }
    HAPFatalError();
}

HAP_RESULT_USE_CHECK
bool HAPIPSessionAreEventNotificationsEnabled(
        HAPIPSessionDescriptor* session,
        const HAPCharacteristic* characteristic,
        const HAPService* service,
        const HAPAccessory* accessory) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPPrecondition(session->securitySession.type == kHAPIPSecuritySessionType_HAP);
    HAPPrecondition(session->securitySession.isOpen);
    HAPPrecondition(session->securitySession.isSecured || kHAPIPAccessoryServer_SessionSecurityDisabled);
    HAPPrecondition(!HAPSessionIsTransient(&session->securitySession._.hap));
    HAPPrecondition(characteristic);
    HAPPrecondition(service);
    HAPPrecondition(accessory);

    uint64_t aid = accessory->aid;
    uint64_t iid = ((const HAPBaseCharacteristic*) characteristic)->iid;

    if (session->eventNotifications) {
        size_t i = 0;
        while ((i < session->numEventNotifications) &&
               ((session->eventNotifications[i].aid != aid) || (session->eventNotifications[i].iid != iid))) {
            i++;
        }
        HAPAssert(
                (i == session->numEventNotifications) ||
                ((i < session->numEventNotifications) && (session->eventNotifications[i].aid == aid) &&
                 (session->eventNotifications[i].iid == iid)));

        return i < session->numEventNotifications;
    }
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 0)
    HAPAssertionFailure();
    return false;
#else
    if (session->eventNotificationSubscriptions) {
        HAPAssert(session->eventNotificationFlags);
        HAPAccessoryServer* server = session->server;
        size_t eventNotificationIndex;
        bool eventNotificationIndexFound;
        HAPAccessoryServerGetEventNotificationIndex(
                HAPNonnull(session->server), aid, iid, &eventNotificationIndex, &eventNotificationIndexFound);

        return eventNotificationIndexFound && HAPBitSetContains(
                                                      HAPNonnull(session->eventNotificationSubscriptions),
                                                      server->ip.numEventNotificationBitSetBytes,
                                                      eventNotificationIndex);
    }

    HAPAssert(!session->eventNotificationSubscriptions);
    HAPAssert(!session->eventNotificationFlags);
    return false;
#endif
}

void HAPIPSessionHandleReadRequest(
        HAPIPSessionDescriptor* session,
        HAPIPSessionContext sessionContext,
        const HAPCharacteristic* characteristic,
        const HAPService* service,
        const HAPAccessory* accessory,
        HAPIPSessionReadResult* readResult,
        HAPIPByteBuffer* dataBuffer) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPPrecondition(session->securitySession.type == kHAPIPSecuritySessionType_HAP);
    HAPPrecondition(session->securitySession.isOpen);
    HAPPrecondition(session->securitySession.isSecured || kHAPIPAccessoryServer_SessionSecurityDisabled);
    HAPPrecondition(!HAPSessionIsTransient(&session->securitySession._.hap));
    HAPPrecondition(readResult);
    HAPPrecondition(dataBuffer);

    HAPRawBufferZero(readResult, sizeof *readResult);

    const HAPBaseCharacteristic* baseCharacteristic = (const HAPBaseCharacteristic*) characteristic;

    HAPIPReadContext readContext;
    HAPRawBufferZero(&readContext, sizeof readContext);

    readContext.aid = accessory->aid;
    readContext.iid = baseCharacteristic->iid;

    if (!HAPCharacteristicReadRequiresAdminPermissions(baseCharacteristic) ||
        HAPSessionControllerIsAdmin(&session->securitySession._.hap)) {
        if (baseCharacteristic->properties.readable) {
            if ((sessionContext != kHAPIPSessionContext_EventNotification) &&
                HAPUUIDAreEqual(baseCharacteristic->characteristicType, &kHAPCharacteristicType_ButtonEvent)) {
                // Workaround for 47267690: Error -70402 when reading from "Button Event" characteristic.
                // The Button Event characteristic is event only.
                readResult->status = kHAPIPAccessoryServerStatusCode_Success;
                readResult->value.stringValue.bytes = "";
                readResult->value.stringValue.numBytes = 0;
            } else if (
                    (sessionContext != kHAPIPSessionContext_EventNotification) &&
                    HAPUUIDAreEqual(
                            baseCharacteristic->characteristicType, &kHAPCharacteristicType_ProgrammableSwitchEvent)) {
                // A read of this characteristic must always return a null value for IP accessories.
                // See HomeKit Accessory Protocol Specification R17
                // Section 11.47 Programmable Switch Event
                readResult->status = kHAPIPAccessoryServerStatusCode_Success;
                readResult->value.unsignedIntValue = 0;
            } else if (
                    (sessionContext == kHAPIPSessionContext_GetAccessories) &&
                    baseCharacteristic->properties.ip.controlPoint) {
                readResult->status = kHAPIPAccessoryServerStatusCode_UnableToPerformOperation;
            } else {
                handle_characteristic_read_request(
                        session, characteristic, service, accessory, &readContext, dataBuffer);
                readResult->status = readContext.status;
                switch (baseCharacteristic->format) {
                    case kHAPCharacteristicFormat_Bool:
                    case kHAPCharacteristicFormat_UInt8:
                    case kHAPCharacteristicFormat_UInt16:
                    case kHAPCharacteristicFormat_UInt32:
                    case kHAPCharacteristicFormat_UInt64: {
                        readResult->value.unsignedIntValue = readContext.value.unsignedIntValue;
                        break;
                    }
                    case kHAPCharacteristicFormat_Int: {
                        readResult->value.intValue = readContext.value.intValue;
                        break;
                    }
                    case kHAPCharacteristicFormat_Float: {
                        readResult->value.floatValue = readContext.value.floatValue;
                        break;
                    }
                    case kHAPCharacteristicFormat_Data:
                    case kHAPCharacteristicFormat_String:
                    case kHAPCharacteristicFormat_TLV8: {
                        readResult->value.stringValue.bytes = readContext.value.stringValue.bytes;
                        readResult->value.stringValue.numBytes = readContext.value.stringValue.numBytes;
                        break;
                    }
                }
            }
        } else {
            readResult->status = kHAPIPAccessoryServerStatusCode_ReadFromWriteOnlyCharacteristic;
        }
    } else {
        readResult->status = kHAPIPAccessoryServerStatusCode_InsufficientPrivileges;
    }
}

void HAPAccessoryServerUpdateIPSessionLastUsedTimestamp(HAPAccessoryServer* server, HAPSession* session) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(session->transportType == kHAPTransportType_IP);

    for (size_t i = 0; i < server->ip.storage->numSessions; i++) {
        HAPIPSession* ipSession = &server->ip.storage->sessions[i];
        HAPIPSessionDescriptor* sessionDescriptor = &ipSession->descriptor;
        if (!sessionDescriptor->server) {
            continue;
        }
        if (sessionDescriptor->securitySession.type != kHAPIPSecuritySessionType_HAP) {
            continue;
        }
        if (&sessionDescriptor->securitySession._.hap == session) {
            sessionDescriptor->stamp = HAPPlatformClockGetCurrent();
            break;
        }
    }
}

#endif
