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

#ifndef HAP_IP_ACCESSORY_SERVER_H
#define HAP_IP_ACCESSORY_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP+API.h"
#include "HAPCrypto.h"
#include "HAPIP+ByteBuffer.h"
#include "HAPIPAccessory.h"
#include "HAPSession.h"
#include "util_http_reader.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)

typedef struct {
    void (*init)(HAPAccessoryServer* p_srv);
    HAP_RESULT_USE_CHECK
    HAPError (*deinit)(HAPAccessoryServer* p_srv);
    HAP_RESULT_USE_CHECK
    HAPAccessoryServerState (*get_state)(HAPAccessoryServer* p_srv);
    void (*start)(HAPAccessoryServer* p_srv);
    HAP_RESULT_USE_CHECK
    HAPError (*stop)(HAPAccessoryServer* p_srv);
    HAP_RESULT_USE_CHECK
    HAPError (*raise_event)(
            HAPAccessoryServer* server,
            const HAPCharacteristic* characteristic,
            const HAPService* service,
            const HAPAccessory* accessory);
    HAP_RESULT_USE_CHECK
    HAPError (*raise_event_on_session)(
            HAPAccessoryServer* server,
            const HAPCharacteristic* characteristic,
            const HAPService* service,
            const HAPAccessory* accessory,
            const HAPSession* session);
} HAPAccessoryServerServerEngine;

extern const HAPAccessoryServerServerEngine HAPIPAccessoryServerServerEngine;

struct HAPIPAccessoryServerTransport {
    void (*create)(HAPAccessoryServer* server, const HAPAccessoryServerOptions* options);

    void (*prepareStart)(HAPAccessoryServer* server);

    void (*willStart)(HAPAccessoryServer* server);

    void (*start)(HAPAccessoryServer* server);

    void (*prepareStop)(HAPAccessoryServer* server);

    struct {
        void (*invalidateDependentIPState)(HAPAccessoryServer* server, HAPSession* session);
    } session;

    struct {
        void (*install)(HAPAccessoryServer* server);

        void (*uninstall)(HAPAccessoryServer* server);

        const HAPAccessoryServerServerEngine* _Nullable (*_Nonnull get)(HAPAccessoryServer* server);
    } serverEngine;
};

/**
 * Session type.
 */
HAP_ENUM_BEGIN(uint8_t, HAPIPSecuritySessionType) { /** HAP session. */
                                                    kHAPIPSecuritySessionType_HAP = 1,

                                                    /** MFiSAP session. */
                                                    kHAPIPSecuritySessionType_MFiSAP
} HAP_ENUM_END(uint8_t, HAPIPSecuritySessionType);

/**
 * Session.
 */
typedef struct {
    /** Session type. */
    HAPIPSecuritySessionType type;

    /** Whether or not the session is open. */
    bool isOpen : 1;

    /** Whether or not a security session has been established. */
    bool isSecured : 1;

    /**
     * Whether or not the /config message has been received.
     *
     * - This sends FIN after the next response, and restarts the IP server after receiving FIN from controller.
     */
    bool receivedConfig : 1;

    /** Whether or not the /configured message has been received. */
    bool receivedConfigured : 1;

    /** Session. */
    union {
        /** HAP session. */
        HAPSession hap;

        /** MFi SAP session. */
        struct {
            /** AES master context. */
            HAP_aes_ctr_ctx aesMasterContext;

        } mfiSAP;
    } _;
} HAPIPSecuritySession;

//----------------------------------------------------------------------------------------------------------------------

/**
 * Accessory server session state.
 */
HAP_ENUM_BEGIN(uint8_t, HAPIPSessionState) { /** Accessory server session is idle. */
                                             kHAPIPSessionState_Idle,

                                             /** Accessory server session is reading. */
                                             kHAPIPSessionState_Reading,

                                             /** Accessory server session is writing. */
                                             kHAPIPSessionState_Writing
} HAP_ENUM_END(uint8_t, HAPIPSessionState);

/**
 * HTTP/1.1 Content Type.
 */
HAP_ENUM_BEGIN(uint8_t, HAPIPAccessoryServerContentType) { /** Unknown HTTP/1.1 content type. */
                                                           kHAPIPAccessoryServerContentType_Unknown,

                                                           /** application/hap+json. */
                                                           kHAPIPAccessoryServerContentType_Application_HAPJSON,

                                                           /** application/octet-stream. */
                                                           kHAPIPAccessoryServerContentType_Application_OctetStream,

                                                           /** application/pairing+tlv8. */
                                                           kHAPIPAccessoryServerContentType_Application_PairingTLV8
} HAP_ENUM_END(uint8_t, HAPIPAccessoryServerContentType);

/**
 * IP specific event notification state.
 */
typedef struct _HAPIPEventNotification {
    /** Accessory instance ID. */
    uint64_t aid;

    /** Characteristic instance ID. */
    uint64_t iid;

    /** Flag indicating whether an event has been raised for the given characteristic in the given accessory. */
    bool flag;
} HAPIPEventNotification;

/**
 * IP specific accessory server session descriptor.
 */
typedef struct _HAPIPSessionDescriptor {
    /** Accessory server serving this session. */
    HAPAccessoryServer* _Nullable server;

    /** TCP stream. */
    HAPPlatformTCPStreamRef tcpStream;

    /** Number of sent bytes that have not yet been acknowledged. */
    size_t numNonAcknowledgedBytes;

    /** Whether TCP write progression has occurred since last time TCP progression was checked. */
    bool tcpDidProgress : 1;

    /** Flag indicating whether the TCP stream is open. */
    bool tcpStreamIsOpen : 1;

    /** IP session state. */
    HAPIPSessionState state;

    /** Time stamp of last activity on this session. */
    HAPTime stamp;

    /** Security session. */
    HAPIPSecuritySession securitySession;

    /** Inbound buffer. */
    HAPIPByteBuffer inboundBuffer;

    /** Marked inbound buffer position indicating the position until which the buffer has been decrypted. */
    size_t inboundBufferMark;

    /** Outbound buffer. */
    HAPIPByteBuffer outboundBuffer;

    /**
     * Marked outbound buffer position indicating the position until which the buffer has not yet been encrypted
     * (starting from outboundBuffer->limit).
     */
    size_t outboundBufferMark;

    /** HTTP reader. */
    struct util_http_reader httpReader;

    /** Current position of the HTTP reader in the inbound buffer. */
    size_t httpReaderPosition;

    /** Flag indication whether an error has been encountered while parsing a HTTP message. */
    bool httpParserError : 1;

    /** Flag indicating whether a HTTP/1.1 method is defined. */
    bool httpMethodIsDefined : 1;

    /** Flag indicating whether a HTTP/1.1 URI is defined. */
    bool httpURIIsDefined : 1;

    /** Flag indicating whether a HTTP/1.1 header field name is defined. */
    bool httpHeaderFieldNameIsDefined : 1;

    /** Flag indicating whether a HTTP/1.1 header field value is defined. */
    bool httpHeaderFieldValueIsDefined : 1;

    /** Flag indicating whether a HTTP/1.1 content length is defined. */
    bool httpContentLengthIsDefined : 1;

    /** HTTP/1.1 Method. */
    struct {
        /** Position of the HTTP/1.1 method in the inbound buffer. */
        size_t position;

        /** Length of the HTTP/1.1 method in the inbound buffer. */
        size_t numBytes;
    } httpMethod;

    /** HTTP/1.1 URI. */
    struct {
        /** Position of the HTTP/1.1 URI in the inbound buffer. */
        size_t position;

        /** Length of the HTTP/1.1 URI in the inbound buffer. */
        size_t numBytes;
    } httpURI;

    /** HTTP/1.1 Header Field Name. */
    struct {
        /** Position of the current HTTP/1.1 header field name in the inbound buffer. */
        size_t position;

        /** Length of the current HTTP/1.1 header field name in the inbound buffer. */
        size_t numBytes;
    } httpHeaderFieldName;

    /** HTTP/1.1 Header Field Value. */
    struct {
        /** Position of the current HTTP/1.1 header field value in the inbound buffer. */
        size_t position;

        /** Length of the current HTTP/1.1 header field value in the inbound buffer. */
        size_t numBytes;
    } httpHeaderFieldValue;

    /** HTTP/1.1 Content Length. */
    size_t httpContentLength;

    /** HTTP/1.1 Content Type. */
    HAPIPAccessoryServerContentType httpContentType;

#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 1)
    /** Bit set representing the event notification subscriptions on this session. */
    uint8_t* _Nullable eventNotificationSubscriptions;

    /** Bit set representing the event notification flags on this session. */
    uint8_t* _Nullable eventNotificationFlags;
#endif

    /** Array of event notification contexts on this session. */
    HAPIPEventNotification* _Nullable eventNotifications;

    /** The maximum number of events this session can handle. */
    size_t maxEventNotifications;

    /** The number of subscribed events on this session. */
    size_t numEventNotifications;

    /** The number of raised events on this session. */
    size_t numEventNotificationFlags;

    /** Time stamp of last event notification on this session. */
    HAPTime eventNotificationStamp;

    /** Time when the request expires. 0 if no timed write in progress. */
    HAPTime timedWriteExpirationTime;

    /** PID of timed write. Must match "pid" of next PUT /characteristics. */
    uint64_t timedWritePID;

    /** Camera snapshot reader. */
    HAPPlatformCameraSnapshotReader* _Nullable cameraSnapshotReader;

    /** The number of camera snapshot bytes to serialize. */
    size_t numCameraSnapshotBytesToSerialize;

    /** Serialization context for incremental accessory attribute database serialization. */
    HAPIPAccessorySerializationContext accessorySerializationContext;

    /** Flag indicating whether incremental serialization of accessory attribute database is in progress. */
    bool accessorySerializationIsInProgress;
} HAPIPSessionDescriptor;

typedef struct _HAPIPSession {
    /**
     * IP session descriptor.
     */
    HAPIPSessionDescriptor descriptor;

    /**
     #if (HAP_DYNAMIC_MEMORY_ALLOCATION == 1)
     * Buffer to store received data (optional).
     #else
     * Buffer to store received data.
     #endif
     */
    struct {
/**
 #if (HAP_DYNAMIC_MEMORY_ALLOCATION == 1)
 * Inbound buffer. Unless dynamic memory management functions are provided as part of the accessory server
 * storage configuration, memory must be allocated and must remain valid while the accessory server is
 * initialized.
 #else
 * Inbound buffer. Memory must be allocated and must remain valid while the accessory server is initialized.
 #endif
 *
 * - It is recommended to allocate at least kHAPIPSession_DefaultInboundBufferSize bytes,
 *   but the optimal size may vary depending on the accessory's attribute database.
 */
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 1)
        void* _Nullable bytes;
#else
        void* bytes;
#endif

        /**
         * Size of inbound buffer.
         */
        size_t numBytes;
    } inboundBuffer;

    /**
     #if (HAP_DYNAMIC_MEMORY_ALLOCATION == 1)
     * Buffer to store pending data to be sent (optional).
     #else
     * Buffer to store pending data to be sent.
     #endif
     */
    struct {
/**
 #if (HAP_DYNAMIC_MEMORY_ALLOCATION == 1)
 * Outbound buffer. Unless dynamic memory management functions are provided as part of the accessory server
 * storage configuration, memory must be allocated and must remain valid while the accessory server is
 * initialized.
 #else
 * Outbound buffer. Memory must be allocated and must remain valid while the accessory server is initialized.
 #endif
 *
 * - It is recommended to allocate at least kHAPIPSession_DefaultOutboundBufferSize bytes,
 *   but the optimal size may vary depending on the accessory's attribute database.
 */
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 1)
        void* _Nullable bytes;
#else
        void* bytes;
#endif

        /**
         * Size of outbound buffer.
         */
        size_t numBytes;
    } outboundBuffer;

/**
 #if (HAP_DYNAMIC_MEMORY_ALLOCATION == 1)
 * Event notifications (optional).
 *
 * - Unless dynamic memory management functions are provided as part of the accessory server storage configuration,
 *   at least one of these structures must be allocated per HomeKit characteristic and service and must remain
 *   valid while the accessory server is initialized.
 #else
 * Event notifications.
 *
 * - At least one of these structures must be allocated per HomeKit characteristic and service and must remain
 *   valid while the accessory server is initialized.
 #endif
 */
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 1)
    HAPIPEventNotification* _Nullable eventNotifications;
#else
    HAPIPEventNotification* eventNotifications;
#endif

    /**
     * Number of event notification structures.
     */
    size_t numEventNotifications;
} HAPIPSession;

/**
 * Closes all the HAP IP sessions and the TCP listener.
 *
 * @param      server          Accessory server ref
 *
 */
void HAPAccessoryServerCloseTCPSessionsAndListener(HAPAccessoryServer* server);

/**
 * Closes all the HAP IP sessions
 *
 * @param      server          Accessory server ref
 *
 */
void HAPAccessoryServerCloseTCPSessions(HAPAccessoryServer* server);

/**
 * Opens the listener for TCP and subsequently HAP IP sessions
 *
 * @param      server          Accessory server ref
 *
 */
void HAPAccessoryServerOpenTCPListener(HAPAccessoryServer* server);

#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_RECONFIGURATION)
/**
 * Increments the configuration number, opens up HAP IP sessions and starts HAP Bonjour service discovery
 *
 * @param      server          Accessory server ref
 *
 */
void HAPAccessoryServerHandleWiFiReconfigurationDone(HAPAccessoryServer* server);
#endif

#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
