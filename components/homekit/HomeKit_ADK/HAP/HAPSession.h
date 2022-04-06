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

#ifndef HAP_SESSION_H
#define HAP_SESSION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP+API.h"
#include "HAPCrypto.h"
#include "HAPPDUProcedure.h"
#include "HAPPairing.h"
#include "HAPThreadSessionStorage.h"
#include "HAPWiFiRouter.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * Session key.
 */
typedef struct {
    /** Value. */
    uint8_t bytes[CHACHA20_POLY1305_KEY_BYTES];
} HAPSessionKey;
HAP_STATIC_ASSERT(sizeof(HAPSessionKey) == CHACHA20_POLY1305_KEY_BYTES, HAPSessionKey);

/**
 * HAP session channel state.
 */
typedef struct {
    HAPSessionKey key; /**< Encryption key. */
    uint64_t nonce;    /**< Nonce. */
} HAPSessionChannelState;

/**
 * Bluetooth LE specific parameters. Part of #HAPSession structure.
 */
typedef struct {
    /**@cond */
    HAPAccessoryServer* server; /**< Accessory server. */
    HAPSession* session;        /**< Session. */

    bool isTerminal;               /**< True if LE link must be disconnected. No more requests are accepted. */
    HAPPlatformTimerRef linkTimer; /**< On expiry, the LE link is disconnected. */
    HAPTime linkTimerDeadline;     /**< Timeout of link timer, if timer is active. */

    /**
     * On expiry, the current Pairing procedure times out.
     */
    HAPPlatformTimerRef pairingProcedureTimer;

    /**
     * Whether or not it is safe to disconnect.
     *
     * - After a BLE response packet has been sent, it may take a certain time until the packet is fully transmitted.
     *   If a disconnect is requested before that happens, certain BLE stacks may drop the packet.
     *   Therefore, a timer is used to delay pending disconnects until we assume that the packet has been sent.
     */
    bool isSafeToDisconnect;

    /**
     * On expiry, it is safe to disconnect.
     */
    HAPPlatformTimerRef safeToDisconnectTimer;
    /**@endcond */
} HAPBLESession;

/**
 * HAP session.
 */
typedef struct _HAPSession {
    /**@cond */
    HAPAccessoryServer* server;

#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    /** Session specific data. */
    HAPThreadSessionStorageState storageState;

    /** Procedure specific data. */
    HAPPDUProcedureSessionState procedure;
#endif

    /**
     * HAP session state.
     */
    struct {
        /** Whether the security session is active. */
        bool active : 1;

        /** Whether the security session originated from a transient Pair Setup procedure (Software Authentication). */
        bool isTransient : 1;

        /**
         * Key-value store key of the pairing, if applicable.
         *
         * - For sessions from a transient Pair Setup procedure (Software Authentication), this is a value < 0.
         */
        int pairingID;

        /**
         * Shared secret, if applicable.
         *
         * - This is used to derive the BLE Broadcast Encryption Key and HomeKit Data Stream keys.
         *
         * - For sessions from a transient Pair Setup procedure (Software Authentication), this is uninitialized.
         */
        uint8_t cv_KEY[X25519_BYTES];

        /** Accessory to controller state. */
        struct {
            /** Control channel encryption. */
            HAPSessionChannelState controlChannel;

            /**
             * Event channel encryption, if applicable.
             *
             * - For sessions from a transient Pair Setup procedure (Software Authentication), this is uninitialized.
             */
            HAPSessionChannelState eventChannel;
        } accessoryToController;

        /** Controller to accessory state. */
        struct {
            /** Control channel encryption. */
            HAPSessionChannelState controlChannel;
        } controllerToAccessory;

        /** timestamp when the session becomes active */
        HAPTime timestamp;
    } hap;

    struct {
        /**
         * Pair Setup procedure state.
         */
        struct {
            uint8_t state;  /**< State. */
            uint8_t method; /**< Method. */
            uint8_t error;  /**< Error code. */
        } pairSetup;

        /**
         * Pair Verify procedure state.
         */
        struct {
            uint8_t state;  /**< State. */
            uint8_t method; /**< Method. */
            uint8_t error;  /**< Error code. */

            uint8_t SessionKey[CHACHA20_POLY1305_KEY_BYTES]; // Session Key for the Pair Verify procedure.
            uint8_t cv_PK[X25519_BYTES];                     // PK
            uint8_t cv_SK[X25519_SCALAR_BYTES];              // SK
            uint8_t cv_KEY[X25519_BYTES];                    // Key (SK, CTRL PK)
            int pairingID;
            uint8_t Controller_cv_PK[X25519_BYTES]; // CTRL PK
        } pairVerify;

        /**
         * Pairings state.
         */
        struct {
            uint8_t state;  /**< State. */
            uint8_t method; /**< Method. */
            uint8_t error;  /**< Error code. */

            // Remove pairing.
            HAPPairingID removedPairingID;
            size_t removedPairingIDLength;
        } pairings;
    } state;

    /**
     * Type of the underlying transport.
     */
    HAPTransportType transportType;

    /**
     * Transport-specific parameters, depending on #transport_type.
     */
    union {
        HAPBLESession ble; /**< Bluetooth LE specific parameters. */
    } _;

#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_ROUTER)
    /** Wi-Fi router specific HAP event state. */
    struct {
        /** Network Client Profile Control characteristic specific state. */
        HAPWiFiRouterSessionState networkClientProfileControl;

        /** Network Access Violation Control characteristic specific state. */
        HAPWiFiRouterSessionState networkAccessViolationControl;
    } wiFiRouterEventState;
    /**@endcond */
#endif
} HAPSession;

/**
 * Pairing procedure.
 */
HAP_ENUM_BEGIN(uint8_t, HAPPairingProcedureType) { /**
                                                    * Pair Verify.
                                                    */
                                                   kHAPPairingProcedureType_PairVerify,

                                                   /**
                                                    * Pairing Pairings.
                                                    */
                                                   kHAPPairingProcedureType_PairingPairings
} HAP_ENUM_END(uint8_t, HAPPairingProcedureType);

/**
 * Checks whether a value represents a valid transport type.
 *
 * @param      value                Value to check.
 *
 * @return     true                 If the value is valid.
 * @return     false                Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPTransportTypeIsValid(HAPTransportType value);

/**
 * Initializes a session.
 *
 * - The session must be destroyed using #hm_session_deinit once it is no
 *   longer needed to ensure that the accessory state is cleaned up.
 *
 * - While the session is in use, it must be retained in the same memory location.
 *
 * @param      server               Accessory server that the session belongs to.
 * @param[out] session              Session to initialize.
 * @param      transportType        Transport type.
 */
void HAPSessionCreate(HAPAccessoryServer* server, HAPSession* session, HAPTransportType transportType);

/**
 * Destroys a session, cleaning up state in the accessory server.
 *
 * @param      server               Accessory server that the session belongs to.
 * @param      session              Session to destroy.
 */
void HAPSessionRelease(HAPAccessoryServer* server, HAPSession* session);

/**
 * Invalidates a session so that all future requests are rejected until the session is destroyed.
 *
 * - Multiple invocations are okay and do nothing.
 *
 * @param      server               Accessory server that the session belongs to.
 * @param      session              Session to destroy.
 * @param      terminateLink        Whether or not the underlying connection should also be terminated.
 */
void HAPSessionInvalidate(HAPAccessoryServer* server, HAPSession* session, bool terminateLink);

/**
 * Returns whether a secured HAP session has been established.
 *
 * @param      session              Session to query.
 *
 * @return true                     If a secured HAP session has been established.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPSessionIsSecured(const HAPSession* session);

/**
 * Returns whether a HAP session key has expired
 *
 * @param      session              Session to query.
 *
 * @return true                     If the HAP session key has expired.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPSessionKeyExpired(const HAPSession* session);

/**
 * Returns whether a secured HAP session is transient (Software Authentication).
 *
 * @param      session              Session to query.
 *
 * @return true                     If a secured HAP session is transient.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPSessionIsTransient(const HAPSession* session);

/**
 * Returns whether the controller of a HAP session has administrator privileges.
 *
 * @param      session              Session to query.
 *
 * @return true                     If the controller of the HAP session has administrator privileges.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPSessionControllerIsAdmin(const HAPSession* session);

/**
 * Processes a Pair Setup write request.
 *
 * @param      server               Accessory server.
 * @param      session              The session over which the request has been received.
 * @param      requestReader        TLV reader for parsing the value.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If the request cannot be processed in the current state.
 * @return kHAPError_InvalidData    If the controller sent a malformed request.
 * @return kHAPError_OutOfResources If request reader does not have enough free memory.
 */
HAP_RESULT_USE_CHECK
HAPError HAPSessionHandlePairSetupWrite(HAPAccessoryServer* server, HAPSession* session, HAPTLVReader* requestReader);

/**
 * Processes a Pair Setup read request.
 *
 * @param      server               Accessory server.
 * @param      session              The session over which the response will be sent.
 * @param      responseWriter       TLV writer for serializing the response.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If communication with Apple Authentication Coprocessor failed.
 * @return kHAPError_InvalidState   If the request cannot be processed in the current state.
 * @return kHAPError_OutOfResources If response writer does not have enough capacity.
 */
HAP_RESULT_USE_CHECK
HAPError HAPSessionHandlePairSetupRead(HAPAccessoryServer* server, HAPSession* session, HAPTLVWriter* responseWriter);

/**
 * Processes a Pair Verify write request.
 *
 * @param      server               Accessory server.
 * @param      session              The session over which the request has been received.
 * @param      requestReader        TLV reader for parsing the value.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If the request cannot be processed in the current state.
 * @return kHAPError_InvalidData    If the controller sent a malformed request.
 * @return kHAPError_OutOfResources If request reader does not have enough free memory.
 */
HAP_RESULT_USE_CHECK
HAPError HAPSessionHandlePairVerifyWrite(HAPAccessoryServer* server, HAPSession* session, HAPTLVReader* requestReader);

/**
 * Processes a Pair Verify read request.
 *
 * @param      server               Accessory server.
 * @param      session              The session over which the response will be sent.
 * @param      responseWriter       TLV writer for serializing the response.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If the request cannot be processed in the current state.
 * @return kHAPError_OutOfResources If response writer does not have enough capacity.
 */
HAP_RESULT_USE_CHECK
HAPError HAPSessionHandlePairVerifyRead(HAPAccessoryServer* server, HAPSession* session, HAPTLVWriter* responseWriter);

/**
 * Processes a Pairings write request.
 *
 * @param      server               Accessory server.
 * @param      session              The session over which the request has been received.
 * @param      requestReader        TLV reader for parsing the value.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If the request cannot be processed in the current state.
 */
HAP_RESULT_USE_CHECK
HAPError HAPSessionHandlePairingsWrite(HAPAccessoryServer* server, HAPSession* session, HAPTLVReader* requestReader);

/**
 * Processes a Pairings read request.
 *
 * @param      server               Accessory server.
 * @param      session              The session over which the response will be sent.
 * @param      responseWriter       TLV writer for serializing the response.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If the request cannot be processed in the current state.
 * @return kHAPError_OutOfResources If response writer does not have enough capacity.
 */
HAP_RESULT_USE_CHECK
HAPError HAPSessionHandlePairingsRead(HAPAccessoryServer* server, HAPSession* session, HAPTLVWriter* responseWriter);

/**
 * Encrypt a control message to be sent over a HomeKit session.
 *
 * The length of the encrypted message is `<plaintext message length> + CHACHA20_POLY1305_TAG_BYTES` bytes.
 *
 * @param      server               Accessory server.
 * @param      session              The session over which the message will be sent.
 * @param[out] encryptedBytes       Encrypted message.
 * @param      plaintextBytes       Plaintext message.
 * @param      numPlaintextBytes    Plaintext message length.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If the session is not encrypted.
 */
HAP_RESULT_USE_CHECK
HAPError HAPSessionEncryptControlMessage(
        const HAPAccessoryServer* server,
        HAPSession* session,
        void* encryptedBytes,
        const void* plaintextBytes,
        size_t numPlaintextBytes);

/**
 * Encrypt a control message with additional authenticated data to be sent over a HomeKit session.
 *
 * The length of the encrypted message is `<plaintext message length> + CHACHA20_POLY1305_TAG_BYTES` bytes.
 *
 * @param      server               Accessory server.
 * @param      session              The session over which the message will be sent.
 * @param[out] encryptedBytes       Encrypted message.
 * @param      plaintextBytes       Plaintext message.
 * @param      numPlaintextBytes    Plaintext message length.
 * @param      aadBytes             Additional authenticated data.
 * @param      numAADBytes          Additional authenticated data length.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If the session is not encrypted.
 */
HAP_RESULT_USE_CHECK
HAPError HAPSessionEncryptControlMessageWithAAD(
        const HAPAccessoryServer* server,
        HAPSession* session,
        void* encryptedBytes,
        const void* plaintextBytes,
        size_t numPlaintextBytes,
        const void* aadBytes,
        size_t numAADBytes);

/**
 * Encrypt an event message to be sent over a HomeKit session.
 *
 * The length of the encrypted message is `<plaintext message length> + CHACHA20_POLY1305_TAG_BYTES` bytes.
 *
 * @param      server               Accessory server.
 * @param      session              The session over which the message will be sent.
 * @param[out] encryptedBytes       Encrypted message.
 * @param      plaintextBytes       Plaintext message.
 * @param      numPlaintextBytes    Plaintext message length.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If the session is not encrypted.
 */
HAP_RESULT_USE_CHECK
HAPError HAPSessionEncryptEventMessage(
        const HAPAccessoryServer* server,
        HAPSession* session,
        void* encryptedBytes,
        const void* plaintextBytes,
        size_t numPlaintextBytes);

/**
 * Clears the control keys without invalidating the session.
 *
 * @param      session              The session with keys to be
 *                                  purged
 */
void HAPSessionClearControlKeys(HAPSession* session);

/**
 * Decrypts a control message received over a HomeKit session.
 *
 * The length of the decrypted message is `<encrypted message length> - CHACHA20_POLY1305_TAG_BYTES` bytes.
 *
 * @param      server               Accessory server.
 * @param      session              The session over which the message has been received.
 * @param[out] plaintextBytes       Plaintext message.
 * @param      encryptedBytes       Encrypted message.
 * @param      numEncryptedBytes    Encrypted message length.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If the session is not encrypted.
 * @return kHAPError_InvalidData    If the controller sent a malformed request, or decryption failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPSessionDecryptControlMessage(
        HAPAccessoryServer* server,
        HAPSession* session,
        void* plaintextBytes,
        const void* encryptedBytes,
        size_t numEncryptedBytes);

/**
 * Decrypts a control message with additional authenticated data received over a HomeKit session.
 *
 * The length of the decrypted message is `<encrypted message length> - CHACHA20_POLY1305_TAG_BYTES` bytes.
 *
 * @param      server               Accessory server.
 * @param      session              The session over which the message has been received.
 * @param[out] plaintextBytes       Plaintext message.
 * @param      encryptedBytes       Encrypted message.
 * @param      numEncryptedBytes    Encrypted message length.
 * @param      aadBytes             Additional authenticated data.
 * @param      numAADBytes          Additional authenticated data length.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If the session is not encrypted.
 * @return kHAPError_InvalidData    If the controller sent a malformed request, or decryption failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPSessionDecryptControlMessageWithAAD(
        HAPAccessoryServer* server,
        HAPSession* session,
        void* plaintextBytes,
        const void* encryptedBytes,
        size_t numEncryptedBytes,
        const void* aadBytes,
        size_t numAADBytes);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
