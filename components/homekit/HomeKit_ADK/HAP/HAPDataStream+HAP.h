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

#ifndef HAP_DATA_STREAM_HAP_H
#define HAP_DATA_STREAM_HAP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP+API.h"

#include "HAPCrypto.h"
#include "HAPDataStream+Internal.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP) || HAP_FEATURE_ENABLED(HAP_FEATURE_HDS_TRANSPORT_OVER_HAP)

/**
 * The default maximum MTU that the controller will use to send bytes to the accessory.
 * For now, this is hard-coded but could be made configurable.
 *
 * This is an arbitrary value and should be tuned for BLE later.
 */
#define kHAPDataStream_HAPTransport_DefaultMaxControllerTransportMTU (1024)

typedef uint8_t HAPDataStreamInterruptSequenceNumber;

typedef struct _HAPDataStreamHAPTransportState {
    /** Timer to enforce timeouts during setup. */
    HAPPlatformTimerRef setupTimer;

    /** Reference time for TTL values of HAPDataStreams. */
    HAPTime referenceTime;

    /** Track the last-assigned session identifier in order to pick a good next one. */
    HAPDataStreamHAPSessionIdentifier lastSessionIdentifier;

    /** Interrupt notification sequence number. */
    HAPDataStreamInterruptSequenceNumber interruptSequenceNumber;
} HAPDataStreamHAPTransportState;

HAP_STATIC_ASSERT(sizeof(HAPDataStreamHAPSessionIdentifier) == 1, HAPDataStreamHAPSessionIdentifier);

/**
 * State of one way of HomeKit Data Stream communication.
 */
typedef struct {
    HAPDataStreamTransmission tx; /**< Transmission state. */

    struct {
        union {
            uint8_t const* _Nullable buffer;  // controllerToAccessory
            uint8_t* _Nullable mutableBuffer; // accessoryToController
        };
        size_t size;
    } pendingData;
} HAPDataStreamHAPUnidirectionalState;
HAP_STATIC_ASSERT(
        sizeof(HAPDataStreamHAPUnidirectionalState) <= 240,
        HAPDataStreamHAPUnidirectionalState); // Must be small.

/**
 * State of a HomeKit Data Stream transmission.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 9.1.2 Frame Format
 */
HAP_ENUM_BEGIN(uint8_t, HAPDataStreamHAPState) {
    /** Uninitialized. */
    HAPDataStreamHAPState_Uninitialized,

    /** Set Up procedure has begun (got the "write" to Setup characteristic). */
    HAPDataStreamHAPState_SetupBegun,

    /** Set Up procedure has completed. Waiting for checkin frame. */
    HAPDataStreamHAPState_WaitingForFirstFrame,

    /** Active but idle. */
    HAPDataStreamHAPState_Active_Idle,

    /** Active: handling write. */
    HAPDataStreamHAPState_Active_HandlingWrite,

    /** Active: got write waiting for response. */
    HAPDataStreamHAPState_Active_WrittenPendingRead,

    /** Active: told controller Request To Send (no need to interrupt). */
    HAPDataStreamHAPState_Active_RequestedToSend,
} HAP_ENUM_END(uint8_t, HAPDataStreamHAPState);

/**
 * HomeKit HAP Data Stream.
 *
 * - When a HomeKit Data Stream setup is requested through a HomeKit Data Stream Transport Management service:
 *   - The setupTTL is set to a time kHAPDataStream_SetupTimeout in the future.
 *   - The session and serviceTypeIndex are stored in this structure.
 *   - After HAPDataStreamSetupComplete has been called, the following steps are performed:
 *     - The setupRequested flag is set.
 *     - Accessory picks a "session identifier" that controller will use to identify its transmissions.
 *     - Encryption keys are derived from the encryption salts.
 *
 * FIXME: THE BELOW TEXT IS NOT APPLICABLE TO TCP BUT LEFT HERE FOR MODEL FOR THE NEXT PATCH.
 * - When a TCP stream is accepted through the HomeKit Data Stream TCP stream manager:
 *   - The tcpStreamTTL is set to a time kHAPDataStream_TCPStreamTimeout in the future.
 *   - The tcpStream is stored in this structure.
 *   - The tcpStreamIsConnected flag is set (0 value is a legal value for HAPPlatformTCPStreamRef).
 *   - The very first frame's payload is received into unused fields of the structure without incremental decryption.
 *     It is limited to have a maximum Data length of kHAPDataStream_MaxInitialDataBytes.
 *
 * - TTLs are in ms relative to server->dataStream.transportState.referenceTime.
 * - When a TTL is reached, the corresponding state is cleared and associated resources are released.
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
    HAPDataStreamHAPUnidirectionalState accessoryToController;

    /** Controller to accessory state. */
    HAPDataStreamHAPUnidirectionalState controllerToAccessory;

    /** The HomeKit session that originated this HomeKit Data Stream. */
    HAPSession* _Nullable session;

    /** The shared secret of the HomeKit session that originated this HomeKit Data Stream. */
    uint8_t sessionSecret[X25519_BYTES];

    /** The session identifier chosen for this stream */
    HAPDataStreamHAPSessionIdentifier sessionIdentifier;

    /**
     * Key-value store key of the pairing that originated this HomeKit Data Stream, if applicable.
     *
     * - For sessions from a transient Pair Setup procedure (Software Authentication), this is a value < 0.
     */
    int pairingID;

    /** Data Stream Transport Management service index. */
    HAPServiceTypeIndex serviceTypeIndex;

    uint16_t setupTTL; /**< Time when matchmaking for the setup request must be complete. */

    HAPDataStreamHAPState state;
    bool pendingClose : 1;    /**< Whether or not the HomeKit Data Stream is being closed. */
    bool pendingDestroy : 1;  /**< Whether or not the HomeKit Data Stream is being invalidated. */
    bool isHandlingEvent : 1; /**< Whether or not the HomeKit Data Stream is handling a HAP read/write event. */
} HAPDataStreamHAP;

/**
 * Prepares setup of a HomeKit Data Stream over HAP.
 *
 * See `HAPDataStreamSetupBegin` for usage (which delegates here).
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If out of resources.
 * @return kHAPError_InvalidData    If setup parameters are not correct.
 */
HAP_RESULT_USE_CHECK
HAPError HAPDataStreamHAPSetupBegin(
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
void HAPDataStreamHAPSetupCancel(HAPAccessoryServer* server);

/**
 * Completes setup of a HomeKit Data Stream.
 *
 * See `HAPDataStreamSetupComplete` for usage (which delegates here).
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If HomeKit Data Stream setup has not been prepared or has timed out.
 */
HAP_RESULT_USE_CHECK
HAPError HAPDataStreamHAPSetupComplete(HAPAccessoryServer* server, HAPDataStreamAccessorySetupParams* setupParams);

/**
 * Controller pushes bytes to the accessory.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If out of resources.
 * @return kHAPError_InvalidData    If setup parameters are not correct.
 */
HAP_RESULT_USE_CHECK
HAPError HAPDataStreamHAPTransportHandleWrite(
        HAPAccessoryServer* _Nonnull server,
        HAPSession* _Nonnull session,
        HAPDataStreamHAPSessionIdentifier sessionIdentifier,
        const uint8_t* _Nullable payload,
        size_t payloadSize,
        bool forceClose);

/**
 * Controller reads bytes from the accessory (response to the write).
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If out of resources.
 * @return kHAPError_InvalidData    If setup parameters are not correct.
 */
HAP_RESULT_USE_CHECK
HAPError HAPDataStreamHAPTransportHandleRead(
        HAPAccessoryServer* _Nonnull server,
        HAPSession* _Nonnull session,
        uint8_t* _Nonnull payload,
        size_t payloadCapacity,
        size_t* _Nonnull payloadSize,
        bool* _Nonnull requestToSend,
        bool* _Nonnull sendEmptyResponse);

/**
 * Read current 'interrupt' state.
 *
 * Data Streams may enter a "request to send" interrupt state, which is flagging to the
 * controller(s) that some sessions require write-read operations (eg, after an accessory
 * has told a controller that it has no further data to send but now does).
 *
 * @param requestToSendSessionIdentifiers          Buffer to write into.
 * @param requestToSendSessionIdentifiersCapacity  Capacity of buffer.
 * @param[out] requestToSendSessionIdentifiersSize  Number of bytes actually written.
 * @param[out] interruptSequenceNumber             Interrupt sequence number.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If out of resources.
 * @return kHAPError_InvalidData    If setup parameters are not correct.
 */
HAP_RESULT_USE_CHECK
HAPError HAPDataStreamHAPTransportGetInterruptState(
        HAPAccessoryServer* server,
        uint8_t* requestToSendSessionIdentifiers,
        size_t requestToSendSessionIdentifiersCapacity,
        size_t* requestToSendSessionIdentifiersSize,
        HAPDataStreamInterruptSequenceNumber* interruptSequenceNumber);

/**
 * Invalidates a HomeKit HAP Data Stream.
 *
 * See `HAPDataStreamInvalidate` for usage (which delegates here).
 */
void HAPDataStreamHAPInvalidate(HAPAccessoryServer* server, HAPDataStreamRef* dataStream);

/**
 * Invalidates all HomeKit Data Streams that originated from a given HAP Pairing.
 *
 * See `HAPDataStreamInvalidateAllForHAPPairingID` for usage (which delegates here).
 */
void HAPDataStreamHAPInvalidateAllForHAPPairingID(
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
void HAPDataStreamHAPInvalidateAllForHAPSession(
        HAPAccessoryServer* server,
        HAPSession* _Nullable session,
        HAPDataStreamRef* dataStream);

/**
 * Gets the HomeKit request context that originated a HomeKit Data Stream.
 *
 * See `HAPDataStreamGetRequestContext` for usage (which delegates here).
 */
void HAPDataStreamHAPGetRequestContext(
        HAPAccessoryServer* server,
        HAPDataStreamRef* dataStream,
        HAPDataStreamRequest* request);

/**
 * Gets the transmission state for the controller-to-accessory direction.
 */
HAP_RESULT_USE_CHECK
HAPDataStreamTransmission* HAPDataStreamHAPGetReceiveTransmission(HAPDataStreamRef* dataStream);

/**
 * Gets the transmission state for the accessory-to-controller direction.
 */
HAP_RESULT_USE_CHECK
HAPDataStreamTransmission* HAPDataStreamHAPGetSendTransmission(HAPDataStreamRef* dataStream);

/**
 * Updates HAP stream to schedule a receive for a HomeKit Data Stream.
 */
void HAPDataStreamHAPDoReceive(HAPAccessoryServer* server, HAPDataStreamRef* dataStream);

/**
 * Updates HAP stream to schedule a send for a HomeKit Data Stream.
 */
void HAPDataStreamHAPDoSend(HAPAccessoryServer* server, HAPDataStreamRef* dataStream);

/**
 * Prepare the DataStream subsystem to resume a session.
 *
 * @param      server                  Accessory server.
 * @param      resumedSessionSecret    The shared secret of the HomeKit session being resumed.
 * @param      session                 The HomeKit session for the resumed session.
 */
void HAPDataStreamHAPPrepareSessionResume(
        HAPAccessoryServer* server,
        uint8_t* resumedSessionSecret,
        HAPSession* session);

#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif // HAP_DATA_STREAM_HAP_H
