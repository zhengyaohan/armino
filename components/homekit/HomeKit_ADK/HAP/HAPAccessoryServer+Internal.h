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

#ifndef HAP_ACCESSORY_SERVER_INTERNAL_H
#define HAP_ACCESSORY_SERVER_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP+API.h"

#include "HAPAccessorySetupOwnership.h"
#include "HAPBLECharacteristic+Broadcast.h"
#include "HAPCrypto.h"
#include "HAPDataStream+HAP.h"
#include "HAPDataStream+TCP.h"
#include "HAPIPAccessoryProtocol.h"
#include "HAPIPAccessoryServer.h"
#include "HAPIPServiceDiscovery.h"
#include "HAPMFiHWAuth.h"
#include "HAPThreadAccessoryServer.h"
#include "HAPThreadSessionStorage.h"
#include "HAPWiFiRouter.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * Maximum number of queued BLE broadcast events
 */
#define kMaxNumQueuedBLEBroadcastEvents 3

/**
 * IP specific accessory server state.
 */
HAP_ENUM_BEGIN(uint8_t, HAPIPAccessoryServerState) { /** Server state is undefined. */
                                                     kHAPIPAccessoryServerState_Undefined,

                                                     /** Server is initialized but not running. */
                                                     kHAPIPAccessoryServerState_Idle,

                                                     /** Server is running. */
                                                     kHAPIPAccessoryServerState_Running,

                                                     /** Server is shutting down. */
                                                     kHAPIPAccessoryServerState_Stopping
} HAP_ENUM_END(uint8_t, HAPIPAccessoryServerState);

typedef void (*HAPCameraAccessoryGetSupportedRecordingConfigurationCallback)(
        void* _Nullable context,
        HAPAccessoryServer* server,
        const HAPService* service,
        const HAPAccessory* accessory,
        const HAPCameraRecordingSupportedConfiguration* supportedConfig);

struct HAPDataStreamTransportStruct;

/** Buffer to store a variant of a HomeKit data stream transport state */
#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP) || HAP_FEATURE_ENABLED(HAP_FEATURE_HDS_TRANSPORT_OVER_HAP)
typedef union {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
    HAPDataStreamTCPTransportState tcp;
#endif
#if HAP_FEATURE_ENABLED(HAP_FEATURE_HDS_TRANSPORT_OVER_HAP)
    HAPDataStreamHAPTransportState hap;
#endif
} HAPDataStreamTransportState;
#endif

typedef struct _HAPAccessoryServer {
    /** Transports. */
    struct {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
        /** HAP over IP. */
        const HAPIPAccessoryServerTransport* _Nullable ip;
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
        /** HAP over Bluetooth LE. */
        const HAPBLEAccessoryServerTransport* _Nullable ble;
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
        /** HAP over Thread. */
        const HAPThreadAccessoryServerTransport* _Nullable thread;
#endif
    } transports;

    /**
     * Characteristic write request state.
     *
     * - HAP Events triggered by a characteristic write request are not relayed back to the controller.
     *
     * @see HomeKit Accessory Protocol Specification R17
     *      Section 6.8.2 Accessory Sends Event to Controller
     *
     * @see HomeKit Accessory Protocol Specification R17
     *      Section 7.4.6.1 Connected Events
     */
    struct {
        /** The session over which the write request has been received. NULL if no write is active. */
        HAPSession* _Nullable session;

        /** Addressed accessory instance ID. */
        uint64_t aid;

        /** Addressed characteristic instance ID. */
        uint64_t iid;
    } writeRequest;

    /** Accessory server state. */
    HAPAccessoryServerState state;

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
    const HAPAccessoryServerServerEngine* _Nullable _serverEngine;
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
    /** Capabilities. */
    struct {
        /** Accessory supports Wi-Fi Accessory Configuration (WAC) for configuring Wi-Fi credentials. */
        bool wac : 1;
    } caps;
#endif

    /** Platform. */
    HAPPlatform platform;

    /** Callbacks. */
    HAPAccessoryServerCallbacks callbacks;

    /** Timer that on expiry triggers pending callbacks. */
    HAPPlatformTimerRef callbackTimer;

    /** Maximum number of allowed pairings. */
    HAPPlatformKeyValueStoreKey maxPairings;

    /** Timer to expire unpaired state */
    HAPPlatformTimerRef unpairedStateTimer;

    /**
     * Session key valid duration in milliseconds, after which session key expires, or
     * zero if the session key should be valid forever.
     */
    HAPTime sessionKeyExpiry;

    /** Accessory to serve. */
    const HAPAccessory* _Nullable primaryAccessory;

    /** Apple Authentication Coprocessor manager. */
    HAPMFiHWAuth mfi;

    /** Pairing identity. */
    struct {
        /** Long-term public key. */
        uint8_t ed_LTPK[ED25519_PUBLIC_KEY_BYTES];

        /** Long-term secret key. */
        HAPAccessoryServerLongTermSecretKey ed_LTSK;
    } identity;

    /** Accessory setup ownership. */
    HAPAccessorySetupOwnership setupOwnership;

    /**
     * Accessory setup state.
     */
    struct {
        /** Timer until the dynamic setup info needs to be refreshed. */
        HAPPlatformTimerRef dynamicRefreshTimer;

        /** Timer until NFC pairing mode expires. 0 if NFC pairing mode is not active. */
        HAPPlatformTimerRef nfcPairingModeTimer;

        /**
         * Current setup info state.
         */
        struct {
            /**
             * Setup info.
             */
            HAPSetupInfo setupInfo;

            /** Setup code (display / programmable NFC). */
            HAPSetupCode setupCode;

            /** Whether Setup info has been loaded / generated. */
            bool setupInfoIsAvailable : 1;

            /** Whether Setup code has been loaded / generated. */
            bool setupCodeIsAvailable : 1;

            /** Whether setup info should be kept on expiration of timers. */
            bool lockSetupInfo : 1;

            /** Whether setup info should be kept across pairing attempts. */
            bool keepSetupInfo : 1;
        } state;
    } accessorySetup;

    /**
     * Pair Setup procedure state.
     */
    struct {
        /**
         * Session where the current pairing takes place.
         * NULL if no pairing in progress.
         */
        HAPSession* _Nullable sessionThatIsCurrentlyPairing;

        /**
         * Time at which the current pairing operation was started.
         * Undefined if no pairing in progress.
         */
        HAPTime operationStartTime;

        uint8_t A[SRP_PUBLIC_KEY_BYTES];                 // M2, M4
        uint8_t b[SRP_SECRET_KEY_BYTES];                 // M2, M4
        uint8_t B[SRP_PUBLIC_KEY_BYTES];                 // M2, M4
        uint8_t K[SRP_SESSION_KEY_BYTES];                // SRP Session Key
        uint8_t SessionKey[CHACHA20_POLY1305_KEY_BYTES]; // SessionKey for the Pair Setup procedure

        uint8_t M1[SRP_PROOF_BYTES];
        uint8_t M2[SRP_PROOF_BYTES];

        /** Pairing Type flags. */
        uint32_t flags;

        bool flagsPresent : 1;  /**< Whether Pairing Type flags were present in Pair Setup M1. */
        bool keepSetupInfo : 1; /**< Whether setup info should be kept on disconnect. */
    } pairSetup;

#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    HAPThreadSessionStorage sessionStorage;
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
    /**
     * IP specific attributes.
     */
    struct {
        /** Storage. */
        HAPIPAccessoryServerStorage* _Nullable storage;

        /** NULL-terminated array of bridged accessories for a bridge accessory. NULL otherwise. */
        const HAPAccessory* _Nullable const* _Nullable bridgedAccessories;

        /**
         * Array of bridged cameras for a camera bridge. NULL otherwise.
         * Must have same length as bridgedAccessories array.
         * Elements are NULL for entries corresponding to non camera accessories.
         */
        const HAPPlatformCameraRef _Nullable* _Nullable bridgedCameras;

        /** IP specific accessory server state. */
        HAPIPAccessoryServerState state;

        /** Next IP specific accessory server state after state transition is completed. */
        HAPIPAccessoryServerState nextState;

        /** Flag indicating whether the HAP service is currently discoverable. */
        bool isServiceDiscoverable : 1;

        /** Flag indicating whether the accessory server is in WAC mode. */
        bool isInWACMode : 1;

        /** Flag indicating whether the accessory server is in WAC mode transition. */
        bool isInWACModeTransition : 1;

        /**
         * Flag indicating whether GSN has been incremented while raising an event
         * during the current connect / disconnect cycle.
         */
        bool gsnDidIncrement : 1;

        /** The number of active sessions served by the accessory server. */
        size_t numSessions;

#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 1)
        /**
         * The size of the bit sets representing the event notification state on active sessions served by the accessory
         * server.
         */
        size_t numEventNotificationBitSetBytes;
#endif

        /** Timer that on expiry triggers a server state transition. */
        HAPPlatformTimerRef stateTransitionTimer;

        /** Timer that on expiry schedules pending event notifications. */
        HAPPlatformTimerRef eventNotificationTimer;

        /** Timer that on expiry runs the garbage task. */
        HAPPlatformTimerRef garbageCollectionTimer;

        /** Timer that on expiry schedules a maximum idle time check. */
        HAPPlatformTimerRef maxIdleTimeTimer;

        /** Timer that on expiry enforces WAC mode timeouts. */
        HAPPlatformTimerRef wacModeTimer;

        /** Timer that on expiry checks that data has been sent. */
        HAPPlatformTimerRef tcpProgressionTimer;

        /** Timer that on expiry handles configured message for WAC */
        HAPPlatformTimerRef wacConfiguredMessageTimer;

        /** Currently registered Bonjour service. */
        HAPIPServiceDiscoveryType discoverableService;

        /**
         * Wi-Fi Accessory Configuration (WAC) specific attributes.
         */
        struct {
            /**
             * Wi-Fi configuration.
             */
            struct {
                /** Whether the stored Wi-Fi configuration has been set. */
                bool isSet : 1;

                /** Whether the stored Wi-Fi configuration has been applied. */
                bool isApplied : 1;

                /** SSID to join. NULL-terminated. */
                char ssid[HAPPlatformSSID_MaxBytes + 1];

                /**
                 * Passphrase of Wi-Fi to join. NULL-terminated.
                 *
                 * - String has length 0 if no passphrase is required.
                 * - String has length 8-63 if a passphrase is stored.
                 * - String has length 64 if a PSK hex string is stored.
                 */
                char passphrase[64 + 1];

                /**
                 *  * Regulatory domain to operate in. NULL-terminated.
                 *
                 * - String has length 0 if no regulatory domain is defined.
                 * - String is a ISO 3166-1 alpha-2 country code otherwise.
                 */
                char regulatoryDomain[HAPPlatformCountryCode_MaxBytes + 1];
            } wiFiConfiguration;

            /**
             * Whether software access point is active.
             */
            bool softwareAccessPointIsActive : 1;

            /**
             * Whether we are in re-WAC mode. This is to differentiate between WAC and re-WAC failure and handle them
             * differently
             */
            bool isInReWACMode : 1;
        } wac;

#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_ROUTER)
        /** Wi-Fi router specific attributes. */
        HAPWiFiRouter wiFiRouter;

        /** Wi-Fi router specific HAP event attributes. */
        struct {
            /** Network Client Profile Control characteristic specific state. */
            HAPWiFiRouterEventState networkClientProfileControl;

            /** Network Access Violation Control characteristic specific state. */
            HAPWiFiRouterEventState networkAccessViolationControl;
        } wiFiRouterEventState;
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)
        /** Camera specific attributes. */
        struct {
            /** IP Camera streaming sessions. */
            HAPCameraStreamingSessionStorage streamingSessionStorage;

            /** Delegate to use for handling completion of IP Camera getSupportedRecordingConfiguration request. */
            struct {
                HAPCameraAccessoryGetSupportedRecordingConfigurationCallback _Nullable callback;
                void* _Nullable context;
            } getSupportedRecordingConfigurationDelegate;
        } camera;
#endif

        /**
         * Timer to poll for the WiFi status after a certain timeout has elapsed
         */
        HAPPlatformTimerRef checkWiFiStatusTimer;

        /**
         * Number of retries to check for WiFi status updates
         */
        int numCheckWiFiStatusRetries;

#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_RECONFIGURATION)
        /**
         * Wi-Fi ReConfiguration specific attributes and timers
         */
        /**
         * Timer for simple/fail safe update callback to be called after UPDATE_WIFI_CONFIGURATION_TIMEOUT has elapsed
         */
        HAPPlatformTimerRef updateWiFiConfigurationTimer;

        /**
         * Timer for fail safe timeout callback to be called after FAILSAFE_UPDATE_TIMEOUT has elapsed
         */
        HAPPlatformTimerRef failSafeUpdateTimeoutTimer;

        struct {
            /** Operation type */
            HAPCharacteristicValue_WiFiConfigurationControl_OperationType operationType;

            /** Cookie sent from controller to identify diff versions of the WiFi configuration of an accessory */
            uint16_t cookie;

            /** Update status indicating whether update is pending, successful, failed. restart required, etc. */
            uint32_t updateStatus;

            /** ISO 3166-1 country code, e.g. "US". */
            char countryCode[HAPPlatformCountryCode_MaxBytes + 1];

            struct {
                /** SSID to join. NULL-terminated. */
                char ssid[HAPPlatformSSID_MaxBytes + 1];

                bool ssidIsSet;

                /** Security mode for the WiFi network */
                uint8_t securityMode;

                /**
                 * PSK Credential
                 *
                 * - String has length 0 if no passphrase is required.
                 * - String has length 8-63 if a passphrase is stored.
                 * - String has length 64 if a PSK hex string is stored.
                 */
                char psk[2 * kHAPWiFiWPAPSK_NumBytes + 1];

            } stationConfig;
        } wiFiReconfiguration;
#endif
    } ip;
#endif // HAP_FEATURE_ENABLED(HAP_FEATURE_IP)

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    /**
     * BLE specific attributes.
     */
    struct {
        /**
         * Storage.
         *
         * - Implementation note: For now, we use only one procedure.
         */
        HAPBLEAccessoryServerStorage* _Nullable storage;

        /**
         * Connection information.
         */
        struct {
            /** Connection handle of the connected controller, if applicable. */
            HAPPlatformBLEPeripheralManagerConnectionHandle connectionHandle;

            /** Whether a HomeKit controller is connected. */
            bool connected : 1;

            /** Whether the HAP-BLE procedure is attached. */
            bool procedureAttached : 1;
        } connection;

        /** Timestamp for Least Recently Used scheme in Pair Resume session cache. */
        uint32_t sessionCacheTimestamp;

        /**
         * Advertisement state.
         */
        struct {
            /** Preferred regular advertising interval. */
            HAPBLEAdvertisingInterval interval;

            /** Preferred duration of events in ms. */
            uint16_t ev_duration;

            HAPPlatformTimerRef timer; /**< Timer until advertisement parameters change. */
            HAPTime timerExpiryClock;  /**< Timer expiry clock value. */

            HAPPlatformTimerRef fast_timer; /**< Timer until fast initial advertising completes. */
            bool fast_started : 1;          /**< Whether the fast advertising has been started. */

            bool connected : 1; /**< Whether a controller is connected. */

            /*
             * Connected State Information
             */
            struct {
                bool didIncrementGSN : 1; /**< Whether GSN has been incremented in the current connect cycle. */
            } connectedState;

            /**
             * Broadcasted Events.
             *
             * @see HomeKit Accessory Protocol Specification R17
             *      Section 7.4.2.2.2 Manufacturer Data
             */
            struct {
                /** Broadcast interval. */
                HAPBLECharacteristicBroadcastInterval interval;

                /**
                 * Characteristic instance ID, if broadcasted event active. 0 otherwise.
                 *
                 * - For Bluetooth LE, instance IDs cannot exceed UINT16_MAX.
                 */
                uint16_t iid;

                /** Value. */
                uint8_t value[8];

            } broadcastedEvent;

            /** Queued broadcast events */
            struct {
                /**
                 * Queued characteristics for future broadcast events
                 */
                struct {
                    const HAPCharacteristic* characteristic;
                    const HAPService* service;
                    const HAPAccessory* accessory;
                } queue[kMaxNumQueuedBLEBroadcastEvents];

                /**
                 * Number of queued events
                 */
                size_t numQueuedEvents;
            } queuedBroadcastEvents;

        } adv;

        /** BLE transport is running */
        bool isTransportRunning : 1;
        /** BLE transport is stopping */
        bool isTransportStopping : 1;
        /** BLE transport must stop when disconnected */
        bool shouldStopTransportWhenDisconnected : 1;
        /** BLE transport must be started when possible */
        bool shouldStartTransport : 1;
        /** User requested keeping BLE transport on */
        bool didUserRequestToKeepTransportOn : 1;
    } ble;
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP) || HAP_FEATURE_ENABLED(HAP_FEATURE_HDS_TRANSPORT_OVER_HAP)
    /**
     * HomeKit Data Stream specific attributes.
     */
    struct {
        /** The active transport for Data Stream; or NULL if Data Stream is not available */
        struct HAPDataStreamTransportStruct const* _Nullable transport;

        HAPDataStreamTransportState transportState;

        /**
         * HomeKit Data Stream setup state.
         *
         * - Since Setup Data Stream Transport typically uses Write Response, one of these instances is sufficient.
         *   (When a regular Write -> Read pair is used, starting an additional setup request aborts the old one).
         */
        struct {
            /** Session that is accessing the Setup Data Stream Transport characteristic. */
            HAPSession* _Nullable session;

            /** Status code. */
            HAPCharacteristicValue_SetupDataStreamTransport_Status status;
        } setup;

        /** Storage for HomeKit Data Streams. */
        HAPDataStreamRef* _Nullable dataStreams;

        /** Number of HomeKit Data Streams. */
        size_t numDataStreams;
    } dataStream;
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    /**
     * Thread specific attributes.
     */
    struct {
        /** Storage. */
        HAPThreadAccessoryServerStorage* _Nullable storage;

        /** CoAP state. */
        struct {
            /** Identify resource. */
            HAPPlatformThreadCoAPManagerResourceRef identifyResource;

            /** Pair Setup resource. */
            HAPPlatformThreadCoAPManagerResourceRef pairSetupResource;

            /** Pair Verify resource. */
            HAPPlatformThreadCoAPManagerResourceRef pairVerifyResource;

            /** Secure message resource. */
            HAPPlatformThreadCoAPManagerResourceRef secureMessageResource;
        } coap;

        /** Thread device parameters */
        struct {
            /** Thread device type */
            HAPPlatformThreadDeviceCapabilities deviceType;

            /** Thread child timeout in seconds */
            uint32_t childTimeout;

            /**
             * Power leve in db
             */
            uint8_t txPowerdbm;
        } deviceParameters;

        /** Service discovery state. */
        bool isServiceDiscoveryActive : 1;

        /** Thread Management Service Read Thread Parameters requested */
        bool readParametersIsRequested : 1;

        /** Thread transport layer is running */
        bool isTransportRunning : 1;

        /** Thread transport layer is stopping */
        bool isTransportStopping : 1;

        /** Thread transport layer must be restarted after stopped */
        bool shouldStartTransport : 1;

        /** Currently registered Bonjour service. */
        HAPIPServiceDiscoveryType discoverableService;

        /** Thread transport layer should advertise if in unpaired state */
        bool suppressUnpairedThreadAdvertising : 1;
    } thread;
#endif

#if (HAP_TESTING == 1)
    /**
     * Firmware update specific attributes.
     */
    struct {
        bool persistStaging;
    } firmwareUpdate;
#endif
    /** Client context pointer. */
    void* _Nullable context;

    /**
     * Accessory Runtime Information service Heart Beat characteristic attributes
     */
    struct {
        uint32_t value;                               /**< current heart beat characteristic value */
        void* _Nullable data;                         /**< user data */
        HAPAccessoryServerHeartBeatCallback callback; /**< callback function */
        HAPPlatformTimerRef timer;                    /**< Heart beat update timer */
    } heartBeat;

#if HAP_FEATURE_ENABLED(HAP_FEATURE_ACCESS_CODE)
    /**
     * Access Code specific attributes
     */
    struct {
        /** Access Code response storage */
        HAPAccessCodeResponseStorage responseStorage;

        /** Number of bytes used in the access code response storage */
        size_t numResponseBytes;

        /** HAP session pending write response */
        HAPSession* _Nullable hapSession;

        /** Access Code operation callback function */
        HAPAccessCodeOperationCallback handleOperation;

        /**
         * Access Code operation callback context
         */
        void* operationCtx;
    } accessCode;
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_NFC_ACCESS)
    /**
     * NFC Access specific attributes
     */
    struct {
        /** NFC Access response storage */
        HAPNfcAccessResponseStorage responseStorage;

        /** Number of bytes used in the response storage */
        size_t numResponseBytes;

        /** Cached TLV type for pending write response */
        HAPTLVType responseTlvType;

        /** Cached HAP session for pending write response */
        HAPSession* _Nullable hapSession;
    } nfcAccess;
#endif
} HAPAccessoryServer;

/**
 * Gets client context for the accessory server.
 *
 * @param      server               Accessory server.
 *
 * @return Client context passed to HAPAccessoryServerCreate.
 */
HAP_RESULT_USE_CHECK
void* _Nullable HAPAccessoryServerGetClientContext(HAPAccessoryServer* server);

/**
 * Schedules invocation of the accessory server's handleUpdatedState callback.
 *
 * @param      server               Accessory server.
 */
void HAPAccessoryServerDelegateScheduleHandleUpdatedState(HAPAccessoryServer* server);

/**
 * Loads the accessory server LTSK. If none exists, it is generated.
 *
 * @param      server               Accessory server.
 * @param[out] ltsk                 Long-term secret key of the accessory server.
 */
void HAPAccessoryServerLoadLTSK(HAPAccessoryServer* server, HAPAccessoryServerLongTermSecretKey* ltsk);

/**
 * IP protocol version string.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 6.6.3 IP Protocol Version
 */
#define kHAPProtocolVersion_IP "1.1.0"

/**
 * IP protocol version string (short).
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 6.6.3 IP Protocol Version
 */
#define kHAPShortProtocolVersion_IP "1.1"

/**
 * BLE protocol version string.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 7.4.3.1 BLE Protocol Version Characteristic
 */
#define kHAPProtocolVersion_BLE "2.2.0"

/**
 * Thread protocol version string.
 */
#define kHAPProtocolVersion_Thread      "1.2.0"
#define kHAPShortProtocolVersion_Thread "1.2"

/**
 * Returns whether the accessory supports Apple Authentication Coprocessor.
 *
 * @param      server               Accessory server.
 *
 * @return true                     If the accessory supports Apple Authentication Coprocessor.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPAccessoryServerSupportsMFiHWAuth(HAPAccessoryServer* server);

/**
 * Returns whether the accessory supports Software Authentication.
 *
 * @param      server               Accessory server.
 *
 * @return true                     If the accessory supports Software Authentication.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPAccessoryServerSupportsMFiTokenAuth(HAPAccessoryServer* server);

/**
 * Returns the Pairing Feature flags.
 *
 * @param      server               Accessory server.
 *
 * @return Pairing feature flags
 */
HAP_RESULT_USE_CHECK
uint8_t HAPAccessoryServerGetPairingFeatureFlags(HAPAccessoryServer* server);

/**
 * Returns the Status flags.
 *
 * @param      server               Accessory server.
 *
 * @return Status flags
 */
HAP_RESULT_USE_CHECK
uint8_t HAPAccessoryServerGetStatusFlags(HAPAccessoryServer* server);

/**
 * Updates advertising data.
 *
 * @param      server               Accessory server.
 */
void HAPAccessoryServerUpdateAdvertisingData(HAPAccessoryServer* server);

/**
 * If the last remaining admin controller pairing is removed, all pairings on the accessory must be removed.
 *
 * This must be called when:
 * - the accessory server is started (to handle potential power failure during key-value store operations).
 * - a pairing is removed.
 *
 * @param      server               Accessory server.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If persistent store access failed.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 5.12 Remove Pairing
 */
HAP_RESULT_USE_CHECK
HAPError HAPAccessoryServerCleanupPairings(HAPAccessoryServer* server);

/**
 * Get configuration number.
 *
 * @param      keyValueStore        Key-value store.
 * @param[out] cn                   Configuration number.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If persistent store access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPAccessoryServerGetCN(HAPPlatformKeyValueStoreRef keyValueStore, uint16_t* cn);

/**
 * Increments configuration number.
 *
 * - IP: Must be called when an accessory, service or characteristic is added or removed from the accessory server.
 *
 * - WAC: Must be called whenever a Wi-Fi configuration is applied.
 *
 * @param      keyValueStore        Key-value store.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If persistent store access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPAccessoryServerIncrementCN(HAPPlatformKeyValueStoreRef keyValueStore);

/**
 * Resets HomeKit state after a firmware update has occurred.
 *
 * - Prior to calling, make sure that the accessory server is not running.
 *
 * @param      server               Accessory server.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If persistent store access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPHandleFirmwareUpdate(HAPAccessoryServer* server);

/**
 * Identifies the primary accessory hosted by an accessory server.
 *
 * @param      server               Accessory server.
 * @param      session              The session over which the identify request was received.
 */
void HAPAccessoryServerIdentify(HAPAccessoryServer* server, HAPSession* session);

/**
 * Informs the application that a controller subscribed to updates of characteristic value.
 *
 * @param      server               Accessory server.
 * @param      session              The session over which the subscription state was changed.
 * @param      characteristic       The characteristic whose value has changed.
 * @param      service              The service that contains the characteristic.
 * @param      accessory            The accessory that provides the service.
 */
void HAPAccessoryServerHandleSubscribe(
        HAPAccessoryServer* server,
        HAPSession* session,
        const HAPCharacteristic* characteristic,
        const HAPService* service,
        const HAPAccessory* accessory);

/**
 * Informs the application that a controller unsubscribed from updates of characteristic value.
 *
 * @param      server               Accessory server.
 * @param      session              The session over which the subscription state was changed.
 * @param      characteristic       The characteristic whose value has changed.
 * @param      service              The service that contains the characteristic.
 * @param      accessory            The accessory that provides the service.
 */
void HAPAccessoryServerHandleUnsubscribe(
        HAPAccessoryServer* server,
        HAPSession* session,
        const HAPCharacteristic* characteristic,
        const HAPService* service,
        const HAPAccessory* accessory);

/**
 * Returns whether a service is supported in the context of a given accessory server and over a
 * given transport type.
 *
 * - Certain services are only applicable to certain types of accessory server configurations or
 *   certain types of transports.
 *
 * @param      server               Accessory server.
 * @param      transportType        Transport type.
 * @param      service              Service.
 *
 * @return true                     If the service is supported.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPAccessoryServerSupportsService(
        HAPAccessoryServer* server,
        HAPTransportType transportType,
        const HAPService* service);

/**
 * Gets the number of service instances with a given type within an attribute database.
 *
 * @param      server               Accessory server.
 * @param      serviceType          The type of the service.
 *
 * @return Number of services with a given type within the accessory server's attribute database.
 */
HAP_RESULT_USE_CHECK
size_t HAPAccessoryServerGetNumServiceInstances(HAPAccessoryServer* server, const HAPUUID* serviceType);

/**
 * Gets the index of a service for later lookup.
 *
 * @param      server               Accessory server.
 * @param      service              Service.
 * @param      accessory            The accessory that provides the service.
 *
 * @return Index of service within the accessory server's attribute database.
 */
HAP_RESULT_USE_CHECK
HAPServiceTypeIndex HAPAccessoryServerGetServiceTypeIndex(
        HAPAccessoryServer* server,
        const HAPService* service,
        const HAPAccessory* accessory);

/**
 * Gets a service by type and index.
 *
 * @param      server               Accessory server.
 * @param      serviceType          The type of the service.
 * @param      serviceTypeIndex     Index of the service as received through HAPAccessoryServerGetServiceTypeIndex.
 * @param[out] service              Service.
 * @param[out] accessory            The accessory that provides the service.
 */
void HAPAccessoryServerGetServiceFromServiceTypeIndex(
        HAPAccessoryServer* server,
        const HAPUUID* serviceType,
        HAPServiceTypeIndex serviceTypeIndex,
        const HAPService* _Nonnull* _Nonnull service,
        const HAPAccessory* _Nonnull* _Nonnull accessory);

/**
 * Gets the number of characteristics that support event notification within the accessory server's attribute database.
 *
 * @param      server               Accessory server.
 *
 * @return Number of characteristics that support event notification.
 */
HAP_RESULT_USE_CHECK
size_t HAPAccessoryServerGetNumCharacteristicsSupportingEventNotification(HAPAccessoryServer* server);

/**
 * Gets the index of a characteristic within the set of characteristics that support event notification in the accessory
 * server's attribute database.
 *
 * @param      server               Accessory server.
 * @param      aid                  Accessory instance ID.
 * @param      iid                  Characteristic instance ID.
 * @param[out] index                The index of the characteristic within the set of characteristics that support
 *                                  event notification in the accessory server's attribute database.
 * @param[out] indexFound           Flag indicating whether the index of the characteristic has been found.
 */
void HAPAccessoryServerGetEventNotificationIndex(
        HAPAccessoryServer* server,
        uint64_t aid,
        uint64_t iid,
        size_t* index,
        bool* indexFound);

/**
 * Callback that should be invoked for each HAP session.
 *
 * @param      context              Context.
 * @param      server               Accessory server.
 * @param      session              Session.
 * @param[in,out] shouldContinue    True if enumeration shall continue, False otherwise. Is set to true on input.
 */
typedef void (*HAPAccessoryServerEnumerateSessionsCallback)(
        void* _Nullable context,
        HAPAccessoryServer* server,
        HAPSession* session,
        bool* shouldContinue);

/**
 * Enumerates all connected HAP sessions associated with an accessory server.
 *
 * @param      server               Accessory server.
 * @param      callback             Function to call on each configured HAP session.
 * @param      context              Context that is passed to the callback.
 */
void HAPAccessoryServerEnumerateConnectedSessions(
        HAPAccessoryServer* server,
        HAPAccessoryServerEnumerateSessionsCallback callback,
        void* _Nullable context);

/**
 * Searches for an IP session corresponding to a given HAP session and returns the session index.
 *
 * @param      server               Accessory server.
 * @param      session              The session to search for.
 *
 * @return The index of the IP session.
 */
HAP_RESULT_USE_CHECK
size_t HAPAccessoryServerGetIPSessionIndex(const HAPAccessoryServer* server, const HAPSession* session);

/**
 * Starts BLE transport.
 *
 * Note that accessory server must have been already started.
 *
 * @param      server               An initialized accessory server that is running.
 */
void HAPAccessoryServerStartBLETransport(HAPAccessoryServer* server);

/**
 * Starts Thread transport.
 *
 * Note that accessory server must have been already started.
 *
 * @param      server               An initialized accessory server that is running.
 */
void HAPAccessoryServerStartThreadTransport(HAPAccessoryServer* server);

/**
 * Stops the accessory server BLE transport.
 *
 * @param      server               An initialized accessory server.
 */
void HAPAccessoryServerStopBLETransport(HAPAccessoryServer* server);

/**
 * Stops the BLE transport when controller is disconnected.
 *
 * The difference from @ref HAPAccessoryServerStopBLETransport() is that
 * this function does not force disconnecting the BLE transport but rather waits
 * till either controller disconnects the connection or the link timer expires.
 *
 * @param      server               An initialized accessory server.
 */
void HAPAccessoryServerStopBLETransportWhenDisconnected(HAPAccessoryServer* server);

/**
 * Stops the accessory server Thread transport.
 *
 * @param      server               An initialized accessory server.
 */
void HAPAccessoryServerStopThreadTransport(HAPAccessoryServer* server);

#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)
/**
 * Returns whether a HAP session has one or more associated active camera streams.
 *
 * @param      session              HAP session.
 *
 * @return true                     If the given HAP session has one or more associated active camera streams.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPSessionHasActiveCameraStream(HAPSession* session);

/**
 * Invalidate camera stream belonging to a given HAP session.
 *
 * @param      server               Accessory server.
 * @param      session              HAP session corresponding to the camera streams.
 */
void HAPInvalidateCameraStreamForSession(HAPAccessoryServer* server, HAPSession* session);

/**
 * Returns whether the snapshots with the given reason are currently enabled.
 *
 * @param      camera               IP Camera provider.
 * @param      accessory            The accessory the snapshot request belongs to.
 * @param      snapshotReason       The reason of the snapshot request.
 *
 * @return true                     If recording is enabled.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPCameraAreSnapshotsEnabled(
        HAPPlatformCameraRef camera,
        const HAPAccessory* accessory,
        HAPIPCameraSnapshotReason snapshotReason);
#endif

/**
 * Monitor unpaired state
 *
 * This function must be called whenever pairing state changes,
 * in order to start or continue unpaired state monitoring.
 *
 * @param server  accessory server
 */
void HAPAccessoryServerUpdateUnpairedStateTimer(HAPAccessoryServer* server);

/**
 * Update last used timestamp of an IP HAP session
 *
 * @param server   Accessory server
 * @param session  HAP session
 */
void HAPAccessoryServerUpdateIPSessionLastUsedTimestamp(HAPAccessoryServer* server, HAPSession* session);

/**
 * Provides accessory configuration details as a null-terminated string
 *
 * @param      platform          Initialized HomeKit platform structure.
 * @param      primaryAccessory  Initialized accessory.
 * @param[out] configBuffer      Buffer that will be filled with configuration data. Must be at least 1KB.
 * @param      configBufferSize  Maximum number of bytes that may be written to the buffer
 */
void HAPAccessoryServerGetAccessoryConfigurationString(
        const HAPPlatform* platform,
        const HAPAccessory* primaryAccessory,
        char* configBuffer,
        size_t configBufferSize);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
