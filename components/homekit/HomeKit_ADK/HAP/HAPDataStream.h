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

#ifndef HAP_DATA_STREAM_H
#define HAP_DATA_STREAM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP+API.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP) || HAP_FEATURE_ENABLED(HAP_FEATURE_HDS_TRANSPORT_OVER_HAP)

/**
 * HomeKit Data Stream version.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.43 Data Stream Transport Management
 */
#define kHAPDataStream_Version "1.0"

/**
 * Timeout after which a HomeKit Data Stream setup request times out.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.121 Setup Data Stream Transport
 */
#define kHAPDataStream_SetupTimeout ((HAPTime)(10 * HAPSecond))

/**
 * Maximum size of Data within the initial frame sent from controller to accessory.
 */
#define kHAPDataStream_MaxInitialDataBytes ((size_t) 128)

/**
 * Info to use when deriving the accessory to controller encryption key.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 9.1.1 Data Stream Transport Security
 */
#define kHAPDataStream_AccessoryToControllerKeyInfo "HDS-Read-Encryption-Key"

/**
 * Info to use when deriving the controller to accessory encryption key.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 9.1.1 Data Stream Transport Security
 */
#define kHAPDataStream_ControllerToAccessoryKeyInfo "HDS-Write-Encryption-Key"

/**
 * Maximum receive buffer log size.
 */
#ifndef HAP_HDS_RX_BUFFER_LOG_MAX_BYTES
#define HAP_HDS_RX_BUFFER_LOG_MAX_BYTES UINT32_MAX
#endif

/**
 * Sentinel value to signal "none set" for Max Controller Transport MTU setting.
 */
#define kHAPDataStream_MaxControllerTransportMTU_NotSet (0)

/**
 * Sessions over HAP have an accessory-assigned session identifier.
 */
typedef uint8_t HAPDataStreamHAPSessionIdentifier;

/**
 * Sentinel value to signal "no session identifier".
 *
 * Accessory gets to choose the session identifier. This implementation reserves this value
 * as a sentinel value for the software to use and will never pick this value to use with the
 * controller.
 */
#define kHAPDataStreamHAPSessionIdentifierNone ((HAPDataStreamHAPSessionIdentifier) 0)

/**
 * State of a HomeKit Data Stream transmission.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 9.1.2 Frame Format
 */
HAP_ENUM_BEGIN(uint8_t, HAPDataStreamTransport) {
    /** None. Data Stream is not supported or enabled. */
    kHAPDataStreamTransport_None = 0,

    /** TCP. Data Stream is enabled and operating over TCP. */
    kHAPDataStreamTransport_TCP = 1,

    /** HAP. Data Stream is enabled and operating over HAP. */
    kHAPDataStreamTransport_HAP = 2,
} HAP_ENUM_END(uint8_t, HAPDataStreamTransport);

/**
 * Salt of a HomeKit Data Stream key.
 */
typedef struct {
    uint8_t bytes[32]; /**< Value. */
} HAPDataStreamSalt;
HAP_STATIC_ASSERT(sizeof(HAPDataStreamSalt) == 32, HAPDataStreamSalt);

/**
 * State of a HomeKit Data Stream transmission.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 9.1.2 Frame Format
 */
HAP_ENUM_BEGIN(uint8_t, HAPDataStreamTransmissionState) {
    /** Idle. */
    kHAPDataStreamTransmissionState_Idle,

    /** Start New Frame. */
    kHAPDataStreamTransmissionState_PrepareFrame,

    /** Frame Header (Type and Length). */
    kHAPDataStreamTransmissionState_FrameHeader,

    /** Data. Sub-state while receiving that indicates that Data is ready to be accepted. */
    kHAPDataStreamTransmissionState_DataReadyToAccept,

    /** Data. */
    kHAPDataStreamTransmissionState_Data,

    /** Data. Sub-state while sending that indicates that the current data chunk has already been encrypted. */
    kHAPDataStreamTransmissionState_DataEncrypted,

    /** Auth Tag. */
    kHAPDataStreamTransmissionState_AuthTag
} HAP_ENUM_END(uint8_t, HAPDataStreamTransmissionState);

/**
 * Parameters needed to set up a new Data Stream (the parameters received from Controller in the Write-Request).
 *
 * For TCP, the controllerKeySalt is required.
 */
typedef struct {
    const HAPDataStreamSalt* _Nullable controllerKeySalt;
} HAPDataStreamControllerSetupParams;

/**
 * Parameters needed to set up a new Data Stream (the parameters sent to Controller in the Write-Response).
 */
typedef struct {
    bool hasListenerPort;
    bool hasAccessoryKeySalt;

    HAPDataStreamHAPSessionIdentifier sessionIdentifier; // set to kHAPDataStreamHAPSessionIdentifierNone if omitted.
    HAPNetworkPort listenerPort;
    HAPDataStreamSalt accessoryKeySalt;
} HAPDataStreamAccessorySetupParams;

/**
 * Returns the transport that HomeKit Data Stream is supported on.
 *
 * @param      server               Accessory server.
 * @param      transportType        Transport type of the originating request.
 *
 * @return which transport (if any) allows Data Stream operations.
 */
HAP_RESULT_USE_CHECK
HAPDataStreamTransport HAPDataStreamGetActiveTransport(HAPAccessoryServer* server, HAPTransportType transportType);

/**
 * Returns the transport that HomeKit Data Stream can use for the given HAP session.
 *
 * @param      server               Accessory server.
 * @param      session              The HAP session for the originating request.
 *
 * @return which transport (if any) allows Data Stream operations.
 */
HAP_RESULT_USE_CHECK
HAPDataStreamTransport HAPDataStreamGetActiveTransportForSession(HAPAccessoryServer* server, HAPSession* session);

/**
 * Get the maximum controller transport MTU supported by the Data Stream transport (used for HAP transport only).
 *
 * @param      server               Accessory server.
 *
 * @return true                     If HomeKit Data Stream over HAP is supported.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
unsigned HAPDataStreamGetMaxControllerTransportMTU(HAP_UNUSED HAPAccessoryServer* server);

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
/**
 * Initialize the DataStream subsystem to operate on TCP.
 *
 * @param      server               Accessory server.
 * @param      tcpStreamManager     TCP stream manager for the DataStream server.
 * @param      dataStreams          Array pointer to hold DataStream state.
 * @param      numDataStreams       Number of DataStream's that can be managed.
 */
void HAPDataStreamCreateWithTCPManager(
        HAPAccessoryServer* server,
        HAPPlatformTCPStreamManagerRef tcpStreamManager,
        HAPDataStreamRef* dataStreams,
        size_t numDataStreams);
#endif

/**
 * Initialize the DataStream subsystem to operate on HAP.
 *
 * @param      server               Accessory server.
 * @param      dataStreams          Array pointer to hold DataStream state.
 * @param      numDataStreams       Number of DataStream's that can be managed.
 */
void HAPDataStreamCreateWithHAP(HAPAccessoryServer* server, HAPDataStreamRef* dataStreams, size_t numDataStreams);

/**
 * Prepare the DataStream subsystem to start. Has no effect if not enabled.
 *
 * @param      server               Accessory server.
 */
void HAPDataStreamPrepareStart(HAPAccessoryServer* server);

/**
 * Prepare the DataStream subsystem to stop running. Has no effect if not running.
 *
 * @param      server               Accessory server.
 */
void HAPDataStreamPrepareStop(HAPAccessoryServer* server);

/**
 * Stop the DataStream subsystem. Has no effect if not running.
 *
 * @param      server               Accessory server.
 */
void HAPDataStreamStop(HAPAccessoryServer* server);

/**
 * Prepares setup of a HomeKit Data Stream.
 *
 * - If HAPDataStreamSetupComplete is not called before kHAPDataStream_SetupTimeout setup is canceled automatically.
 *
 * - /!\ Only one HomeKit Data Stream may be set up at a time. HAPDataStreamSetupCancel to cancel ongoing setup.
 *
 * Example flow:
 * 1. Setup Data Stream Transport write request.
 *    - HAPDataStreamSetupBegin
 * 2. Setup Data Stream Transport read request.
 *    - HAPDataStreamSetupComplete
 *
 * @param      server               Accessory server.
 * @param      service              Data Stream Transport Management service that originated this HomeKit Data Stream.
 * @param      accessory            The accessory that provides the service.
 * @param      session              The HomeKit session that originated this HomeKit Data Stream.
 * @param      setupParams          The setup parameters provided by the controller.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If out of resources.
 * @return kHAPError_InvalidData    If setup parameters are not correct.
 * @return kHAPError_InvalidState   If Data Stream is not set up.
 */
HAP_RESULT_USE_CHECK
HAPError HAPDataStreamSetupBegin(
        HAPAccessoryServer* server,
        const HAPService* service,
        const HAPAccessory* accessory,
        HAPSession* session,
        const HAPDataStreamControllerSetupParams* setupParams);

/**
 * Cancels a prepared HomeKit Data Stream setup if one is pending.
 *
 * @param      server               Accessory server.
 */
void HAPDataStreamSetupCancel(HAPAccessoryServer* server);

/**
 * Completes setup of a HomeKit Data Stream.
 *
 * The parameters that should be provided to the Controller will be filled into the output parameter 'setupParams'.
 *
 * @param      server               Accessory server.
 * @param      setupParams          The setup parameters to send to controller (filled in by this function)
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If HomeKit Data Stream setup has not been prepared or has timed out.
 */
HAP_RESULT_USE_CHECK
HAPError HAPDataStreamSetupComplete(HAPAccessoryServer* server, HAPDataStreamAccessorySetupParams* setupParams);

/**
 * Invalidates all HomeKit Data Streams that originated from a given Pairing.
 *
 * - /!\ This must not be called from a delegate callback.
 *
 * @param      server               Accessory server.
 * @param      pairingID            Pairing ID.
 */
void HAPDataStreamInvalidateAllForHAPPairingID(HAPAccessoryServer* server, int pairingID);

/**
 * Invalidates all HomeKit Data Streams that originated from a given HAP session.
 *
 * - /!\ This must not be called from a delegate callback.
 *
 * @param      server               Accessory server.
 * @param      session              Session. If NULL is specified, all HomeKit Data Streams are invalidated.
 */
void HAPDataStreamInvalidateAllForHAPSession(HAPAccessoryServer* server, HAPSession* _Nullable session);

/**
 * Gets the HomeKit request context that originated a HomeKit Data Stream.
 *
 * @param      server               Accessory server.
 * @param      dataStream           HomeKit Data Stream.
 * @param[out] request              Request.
 */
void HAPDataStreamGetRequestContext(
        HAPAccessoryServer* server,
        HAPDataStreamRef* dataStream,
        HAPDataStreamRequest* request);

/**
 * Prepare the DataStream subsystem to resume a session.
 *
 * @param      server                  Accessory server.
 * @param      resumedSessionSecret    The shared secret of the HomeKit session being resumed.
 * @param      session                 The HomeKit session for the resumed session.
 */
void HAPDataStreamPrepareSessionResume(HAPAccessoryServer* server, uint8_t* resumedSessionSecret, HAPSession* session);

/**
 * Set minimum priority
 *
 * @param server      Accessory server.
 * @param dataStream  data stream.
 * @param priority    minimum priority.
 */
void HAPDataStreamSetMinimumPriority(
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

#endif
