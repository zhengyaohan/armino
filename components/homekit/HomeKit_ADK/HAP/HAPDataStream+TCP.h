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

#ifndef HAP_DATA_STREAM_TCP_H
#define HAP_DATA_STREAM_TCP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP+API.h"
#include "HAPCrypto.h"
#include "HAPDataStream+Internal.h"
#include "HAPSession.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)

/**
 * Timeout after which an unmatched TCP stream is closed.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.121 Setup Data Stream Transport
 */
#define kHAPDataStream_TCPStreamTimeout ((HAPTime)(10 * HAPSecond))

/**
 * Minimum TCP port value that is allowed to be used for HDS.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 9.1 HomeKit Data Stream
 */
#define kHAPDataStream_TCPMinimumPort (32768)

typedef struct _HAPDataStreamTCPTransportState {
    HAPPlatformTCPStreamManagerRef _Nullable tcpStreamManager;

    /** Timer to enforce timeouts during matchmaking. */
    HAPPlatformTimerRef matchmakingTimer;

    /** Reference time for TTL values of HAPDataStreams. */
    HAPTime referenceTime;
} HAPDataStreamTCPTransportState;

/**
 * State of one way of HomeKit Data Stream communication.
 */
typedef struct {
    HAPDataStreamTransmission tx; /**< Transmission state. */
    union {
        HAPDataStreamSalt salt; /**< Encryption key salt. Only valid before session is connected. */
        HAPSessionKey key;      /**< Encryption key. Only valid once session is connected. */
    } _;
    uint64_t nonce; /**< Nonce. */
} HAPDataStreamTCPUnidirectionalState;
HAP_STATIC_ASSERT(
        sizeof(HAPDataStreamTCPUnidirectionalState) <= 240,
        HAPDataStreamTCPUnidirectionalState); // Must be small.

/**
 * HomeKit TCP Data Stream.
 *
 * - When a HomeKit Data Stream setup is requested through a HomeKit Data Stream Transport Management service:
 *   - The setupTTL is set to a time kHAPDataStream_SetupTimeout in the future.
 *   - The session and serviceTypeIndex as well as the encryption salts are stored in this structure.
 *   - The HomeKit Data Stream TCP stream manager is instructed to listen for incoming connections.
 *   - After HAPDataStreamSetupComplete has been called, the following steps are performed:
 *     - The setupRequested flag is set.
 *     - Encryption keys are derived from the encryption salts.
 *
 * - When a TCP stream is accepted through the HomeKit Data Stream TCP stream manager:
 *   - The tcpStreamTTL is set to a time kHAPDataStream_TCPStreamTimeout in the future.
 *   - The tcpStream is stored in this structure.
 *   - The tcpStreamIsConnected flag is set (0 value is a legal value for HAPPlatformTCPStreamRef).
 *   - The very first frame's payload is received into unused fields of the structure without incremental decryption.
 *     It is limited to have a maximum Data length of kHAPDataStream_MaxInitialDataBytes.
 *
 * - TTLs are in ms relative to server->dataStream.transportState.referenceTime.
 * - When a TTL is reached, the corresponding state (setup request, or TCP stream) is cleared and associated resources
 *   are released. The HomeKit Data Stream TCP stream manager listener is stopped if no more setups are pending.
 *
 * - Matchmaking happens when the first frame has been fully received on a TCP stream (nonce == 0):
 *   - A decryption attempt is made for each eligible setup request.
 *     For decryption a separate crypto context is used as the payload is in accessoryToController.tx.crypto.
 *     A HomeKit setup request is eligible for matchmaking when: !isMatchmakingComplete && setupRequested && session.
 *   - If no decryption attempt succeeded, the TCP stream is closed.
 *   - If the decryption attempt succeeded, the setup request information and encryption keys are swapped
 *     between the HAPDataStream structure holding the TCP stream and the one holding the succeeding encryption keys.
 *   - TTL values are cleared, and the isMatchmakingComplete flag is set.
 *   - The delegate is informed that the HomeKit Data Stream has been accepted.
 *     It may opt to immediately start sending packets.
 *   - The delegate is informed that data has been received, and it may receive or skip the payload as usual.
 *   - Further packets are received normally.
 */
typedef struct {
    /** Accessory to controller state. */
    HAPDataStreamTCPUnidirectionalState accessoryToController;

    /** Controller to accessory state. */
    HAPDataStreamTCPUnidirectionalState controllerToAccessory;

    /** The HomeKit session that originated this HomeKit Data Stream. */
    HAPSession* _Nullable session;

    /** TCP stream. */
    HAPPlatformTCPStreamRef tcpStream;

    /** Data Stream Transport Management service index. */
    HAPServiceTypeIndex serviceTypeIndex;

    uint16_t setupTTL;     /**< Time when matchmaking for the setup request must be complete. */
    uint16_t tcpStreamTTL; /**< Time when matchmaking for the TCP stream must be complete. */

    /** current stream priority */
    HAPStreamPriority streamPriority;

    bool setupRequested : 1;        /**< Whether or not HomeKit Data Stream setup has been requested. */
    bool tcpStreamIsConnected : 1;  /**< Whether or not a TCP stream is connected. */
    bool isMatchmakingComplete : 1; /**< Whether or not matchmaking of TCP stream and setup request has completed. */
    bool pendingDestroy : 1;        /**< Whether or not the HomeKit Data Stream is being invalidated. */
    bool isHandlingEvent : 1;       /**< Whether or not the HomeKit Data Stream is handling a TCP stream event. */
} HAPDataStreamTCP;

/**
 * Prepares setup of a HomeKit Data Stream over TCP.
 *
 * See `HAPDataStreamSetupBegin` for usage (which delegates here).
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If out of resources.
 * @return kHAPError_InvalidData    If setup parameters are not correct.
 */
HAP_RESULT_USE_CHECK
HAPError HAPDataStreamTCPSetupBegin(
        HAPAccessoryServer* server,
        const HAPService* service,
        const HAPAccessory* accessory,
        HAPSession* session,
        const HAPDataStreamControllerSetupParams* setupParams);

/**
 * Cancels a prepared HomeKit Data Stream setup if one is pending.
 *
 * See `HAPDataStreamSetupCancel` for usage (which delegates here).
 *
 * @param      server               Accessory server.
 */
void HAPDataStreamTCPSetupCancel(HAPAccessoryServer* server);

/**
 * Completes setup of a HomeKit Data Stream.
 *
 * See `HAPDataStreamSetupComplete` for usage (which delegates here).
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If HomeKit Data Stream setup has not been prepared or has timed out.
 */
HAP_RESULT_USE_CHECK
HAPError HAPDataStreamTCPSetupComplete(HAPAccessoryServer* server, HAPDataStreamAccessorySetupParams* setupParams);

/**
 * Invalidates a HomeKit TCP Data Stream.
 *
 * See `HAPDataStreamInvalidate` for usage (which delegates here).
 */
void HAPDataStreamTCPInvalidate(HAPAccessoryServer* server, HAPDataStreamRef* dataStream);

/**
 * Invalidates all HomeKit Data Streams that originated from a given HAP Pairing.
 *
 * See `HAPDataStreamInvalidateAllForHAPPairingID` for usage (which delegates here).
 */
void HAPDataStreamTCPInvalidateAllForHAPPairingID(
        HAPAccessoryServer* server,
        int pairingID,
        HAPDataStreamRef* dataStream);

/**
 * Invalidates all HomeKit Data Streams that originated from a given HAP session.
 *
 * See `HAPDataStreamInvalidateAllForHAPSession` for usage (which delegates here).
 * If session == NULL, then the dataStream will be unconditionally invalidated.
 * If session != NULL, then the dataStream will only be invalidated if it is associated with the session provided.
 *
 */
void HAPDataStreamTCPInvalidateAllForHAPSession(
        HAPAccessoryServer* server,
        HAPSession* _Nullable session,
        HAPDataStreamRef* dataStream);

/**
 * Updates TCP stream to schedule a receive for a HomeKit Data Stream.
 *
 * - HandleTCPStreamEvent will be called once data or space is available.
 *
 * @param      server              Accessory server.
 * @param      dataStream_          HomeKit Data Stream.
 */
void HAPDataStreamTCPDoReceive(HAPAccessoryServer* server, HAPDataStreamRef* dataStream);

/**
 * Updates TCP stream to schedule a send for a HomeKit Data Stream.
 *
 * - HandleTCPStreamEvent will be called once data or space is available.
 *
 * @param      server              Accessory server.
 * @param      dataStream_          HomeKit Data Stream.
 */
void HAPDataStreamTCPDoSend(HAPAccessoryServer* server, HAPDataStreamRef* dataStream);

/**
 * Gets the HomeKit request context that originated a HomeKit Data Stream.
 *
 * See `HAPDataStreamGetRequestContext` for usage (which delegates here).
 */
void HAPDataStreamTCPGetRequestContext(
        HAPAccessoryServer* server,
        HAPDataStreamRef* dataStream,
        HAPDataStreamRequest* request);

/**
 * Gets the transmission state for the controller-to-accessory direction.
 */
HAP_RESULT_USE_CHECK
HAPDataStreamTransmission* HAPDataStreamTCPGetReceiveTransmission(HAPDataStreamRef* dataStream);

/**
 * Gets the transmission state for the accessory-to-controller direction.
 */
HAP_RESULT_USE_CHECK
HAPDataStreamTransmission* HAPDataStreamTCPGetSendTransmission(HAPDataStreamRef* dataStream);

/**
 * Set minimum priority of TCP stream
 *
 * @param      server              Accessory server.
 * @param      dataStream          HomeKit Data Stream.
 * @param      priority            minimum priority.
 */
void HAPDataStreamTCPSetMinimumPriority(
        HAPAccessoryServer* server,
        HAPDataStreamRef* dataStream,
        HAPStreamPriority priority);

#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif // HAP_DATA_STREAM_TCP_H
