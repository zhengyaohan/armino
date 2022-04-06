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

#ifndef APP_UARP_H
#define APP_UARP_H

#include "ApplicationFeatures.h"

#include "HAP.h"

#if (HAVE_UARP_SUPPORT == 1)

#ifdef __cplusplus
extern "C" {
#endif

#include "CoreUARPPlatformAccessory.h"
#include "CoreUARPProtocolDefines.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * UARP data chunk size.
 * This corresponds to the size used for requesting payload chunks during staging as well as corresponding buffer
 * sizing. It should be tuned based on an accessory's available RAM, any platform-specific transport settings such as
 * MTU, and staging memory interface. A larger chunk size typically corresponds to decreased staging time as there is
 * less messaging overhead per payload data byte, although the benefit may vary by transport and platform. Note this
 * data size must be consumed by the accessory on each payload data callback.
 */
#if (HAVE_IP == 1)
#define kUARPDataChunkSize 8192
#else
#define kUARPDataChunkSize 512
#endif

/**
 * UARP "stream" HomeKit Data Stream protocol name.
 */
#define kHAPStreamAppProtocolName_UARP "UARP"

/**
 * UARP maximum number of controllers.
 * Equivalent to the number of supported data streams.
 */
#define kUARPNumMaxControllers (kApp_NumDataStreams)

/**
 * UARP max receive message size.
 */
#define kUARPMaxRxMessageSize (kUARPDataChunkSize + sizeof(struct UARPMsgAssetDataResponse))

/**
 * UARP receive buffer size.
 */
#define kUARPRxBufferSize (kUARPMaxRxMessageSize + kHAPStreamDataStreamProtocol_RxMsgHeaderBytes)

/**
 * UARP transmit buffer size.
 * Largest possible transmit message is an Information Response message in which the manufacturer, model, serial
 * number, firmware version, or hardware version string is the maximum HomeKit-defined length of 64 characters.
 */
#define kUARPTxBufferSize (sizeof(union UARPMessages) + sizeof(struct UARPTLVHeader) + 64)

/**
 * UARP send scratch buffer size.
 */
#define kUARPTxScratchBufferSize (kUARPTxBufferSize + kHAPStreamDataStreamProtocol_TxMsgHeaderBytes)

/**
 * UARP max tx buffers (send queue depth) per controller.
 */
#define kUARPMaxTxBuffersPerController 8

/**
 * UARP number of tx buffers.
 * Limits the number of UARP sessions which can simultaneously send messages to controllers.
 * Based on controller usage of the HAP profile to establish a UARP session as well as random delays for asset offering,
 * it is expected that an accessory will be talking UARP to no more than 2 controllers concurrently. Extend buffers by
 * an additional controller to account for race conditions in environments with large controller populations.
 */
#define kUARPNumTxBuffers (3 * kUARPMaxTxBuffersPerController)

/**
 * UARP number of rx buffers.
 * Limits the number of UARP sessions which can simultaneously pull assets from controllers.
 * Non-data request protocol messages are received into the data stream short buffer.
 * Only a single firmware asset can be staged at a given time.
 */
#define kUARPNumRxBuffers (1)

/**
 * The number of asset buffers required.
 * A single firmware asset may be staged at a time.
 * A second asset buffer is necessary to handle staging resume (asset merge) and competing offers.
 */
#define kUARPNumAssetBuffers (2)

/**
 * UARP number of data window buffers required.
 * A single firmware asset may be staged at a time, which requires a single data window buffer.
 */
#define kUARPNumDataWindowBuffers (1)

/**
 * UARP asset state change (initiated by controller or UARP stack).
 */
HAP_ENUM_BEGIN(uint8_t, UARPAssetStateChange) {
    kUARPAssetStateChange_StagingPaused = 0x00,
    kUARPAssetStateChange_StagingResumed = 0x01,
    kUARPAssetStateChange_AssetRescinded = 0x02,
    kUARPAssetStateChange_AssetCorrupt = 0x03,
} HAP_ENUM_END(uint8_t, UARPAssetStateChange);

/**
 * UARP asset staging state change request (accessory-initiated).
 */
HAP_ENUM_BEGIN(uint8_t, UARPAssetStagingStateChangeRequest) {
    kUARPAssetStagingStateChangeRequest_Pause = 0x00,
    kUARPAssetStagingStateChangeRequest_Resume = 0x01,
    kUARPAssetStagingStateChangeRequest_Abandon = 0x02,
} HAP_ENUM_END(uint8_t, UARPAssetStagingStateChangeRequest);

typedef struct UARPAccessory UARPAccessory;
typedef struct UARPVersion UARPVersion;
typedef struct UARPLastErrorAction UARPLastErrorAction;

/**
 * UARP callbacks for application handling of firmware assets.
 */
typedef struct {
    /**
     * The callback used to notify the accessory of an asset offering.
     *
     * @param      server              Accessory server.
     * @param      accessory           The accessory that provides the service.
     * @param      assetVersion        Asset version.
     * @param      assetTag            Asset tag identifier.
     * @param      assetLength         Total asset length.
     * @param      assetNumPayloads    Number of payloads in the asset.
     * @param[out] shouldAccept        Accessory indicator to accept or deny the asset.
     */
    void (*assetOffered)(
            HAPAccessoryServer* server,
            const HAPAccessory* accessory,
            UARPVersion* assetVersion,
            uint32_t assetTag,
            uint32_t assetLength,
            uint16_t assetNumPayloads,
            bool* shouldAccept);

    /**
     * The callback used to notify the accessory of an asset metadata TLV for processing.
     *
     * @param      server          Accessory server.
     * @param      accessory       The accessory that provides the service.
     * @param      tlvType         Type field of TLV.
     * @param      tlvLength       Length field of TLV.
     * @param      tlvValue        Buffer holding TLV value field.
     */
    void (*assetMetadataTLV)(
            HAPAccessoryServer* server,
            const HAPAccessory* accessory,
            uint32_t tlvType,
            uint32_t tlvLength,
            uint8_t* tlvValue);

    /**
     * The callback used to notify the accessory of asset metadata processing completion.
     *
     * @param      server          Accessory server.
     * @param      accessory       The accessory that provides the service.
     */
    void (*assetMetadataComplete)(HAPAccessoryServer* server, const HAPAccessory* accessory);

    /**
     * The callback used to provide the accessory with payload header information.
     *
     * @param      server               Accessory server.
     * @param      accessory            The accessory that provides the service.
     * @param      payloadVersion       Payload version.
     * @param      payloadTag           Payload tag identifier.
     * @param      payloadLength        Payload length.
     */
    void (*payloadReady)(
            HAPAccessoryServer* server,
            const HAPAccessory* accessory,
            UARPVersion* payloadVersion,
            uint32_t payloadTag,
            uint32_t payloadLength);

    /**
     * The callback used to notify the accessory of a payload metadata TLV for processing.
     *
     * @param      server          Accessory server.
     * @param      accessory       The accessory that provides the service.
     * @param      tlvType         Type field of TLV.
     * @param      tlvLength       Length field of TLV.
     * @param      tlvValue        Buffer holding TLV value field.
     */
    void (*payloadMetadataTLV)(
            HAPAccessoryServer* server,
            const HAPAccessory* accessory,
            uint32_t tlvType,
            uint32_t tlvLength,
            uint8_t* tlvValue);

    /**
     * The callback used to notify the accessory of payload metadata processing completion.
     *
     * @param      server          Accessory server.
     * @param      accessory       The accessory that provides the service.
     */
    void (*payloadMetadataComplete)(HAPAccessoryServer* server, const HAPAccessory* accessory);

    /**
     * The callback used to notify the accessory of payload data.
     *
     * @param      server          Accessory server.
     * @param      accessory       The accessory that provides the service.
     * @param      payloadTag      Payload tag identifier.
     * @param      offset          Offset for this data chunk.
     * @param      buffer          Buffer holding the data.
     * @param      bufferLength    Length of data buffer.
     */
    void (*payloadData)(
            HAPAccessoryServer* server,
            const HAPAccessory* accessory,
            uint32_t payloadTag,
            uint32_t offset,
            uint8_t* buffer,
            uint32_t bufferLength);

    /**
     * The callback used to notify the accessory of payload processing completion.
     *
     * @param      server          Accessory server.
     * @param      accessory       The accessory that provides the service.
     */
    void (*payloadDataComplete)(HAPAccessoryServer* server, const HAPAccessory* accessory);

    /**
     * The callback used to notify the accessory of a change in asset state.
     *
     * @param      server    Accessory server.
     * @param      accessory The accessory that provides the service.
     * @param      state     Asset state change.
     */
    void (*assetStateChange)(HAPAccessoryServer* server, const HAPAccessory* accessory, UARPAssetStateChange state);

    /**
     * The callback used to inform the accessory of an apply staged assets request.
     * It is expected that accepting the apply request will result in the accessory being reachable on the new firmware
     * version within the update duration time provided by the firmware update HAP profile.
     *
     * @param      server            Accessory server.
     * @param      accessory         The accessory that provides the service.
     * @param[out] requestRefused    Accessory indicator for refusing the apply request.
     */
    void (*applyStagedAssets)(HAPAccessoryServer* server, const HAPAccessory* accessory, bool* requestRefused);

    /**
     * The callback used to retrieve the last accessory error.
     *
     * @param      server             Accessory server.
     * @param      accessory          The accessory that provides the service.
     * @param      lastErrorAction    Accessory last error and associated action.
     */
    void (*retrieveLastError)(
            HAPAccessoryServer* server,
            const HAPAccessory* accessory,
            UARPLastErrorAction* lastErrorAction);
} UARPAccessoryFirmwareAssetCallbacks;

/**
 * Internal storage for UARP buffer requests.
 */
typedef struct {
    struct uarpPlatformController uarpPlatformController[kUARPNumMaxControllers];
    struct uarpPlatformAsset uarpPlatformAsset[kUARPNumAssetBuffers];
    uint8_t uarpDataWindow[kUARPNumDataWindowBuffers][kUARPDataChunkSize];
} UARPInternalStorage;

typedef struct {
    /** Accessory server associated with the data stream. */
    HAPAccessoryServer* server;
    /** Dispatcher associated with the data stream. */
    HAPDataStreamDispatcher* dispatcher;
    /** Stream application protocol. */
    HAPStreamApplicationProtocol* streamAppProtocol;
    /** Handle for the data stream. */
    HAPDataStreamHandle dataStream;
} UARPDataStreamContext;

typedef struct {
    /** UARP transmit buffers associated with UARP controller. */
    uint8_t* _Nullable uarpTxBuffer[kUARPMaxTxBuffersPerController];
    /** Stream scratch buffers associated with UARP controller. */
    uint8_t* _Nullable streamTxScratchBuffer[kUARPMaxTxBuffersPerController];
    /** Data Stream context associated with UARP controller. */
    UARPDataStreamContext dataStreamContext;
    /** UARP Platform controller associated with UARP controller. */
    struct uarpPlatformController* uarpPlatformController;
    /** Indicator for UARP controller registration. */
    bool isAllocated;
} UARPController;

typedef struct {
    /** Associated controller. */
    UARPController* _Nullable controller;
    /** UARP platform asset. */
    struct uarpPlatformAsset* _Nullable uarpPlatformAsset;
    /** Response timeout timer. */
    HAPPlatformTimerRef timeoutTimer;
    /** Controller failed to service data request. */
    bool dataResponseTimedOut;
    /** Controller has paused the transfer. */
    bool isPausedByController;
    /** Asset has been abandoned and its release is pending. */
    bool isPendingRelease;
} UARPFirmwareAsset;

typedef uint32_t UARPAccessoryBufferTracker;
HAP_STATIC_ASSERT(kApp_NumDataStreams <= (sizeof(UARPAccessoryBufferTracker) * CHAR_BIT), UARPAccessoryBufferTracker);

struct UARPAccessory {
    /** UARP configuration. */
    struct {
        size_t numTxBuffers;
        size_t txBufferSize;
        size_t txScratchBufferSize;
    } config;

    /** HAP accessory and server objects. */
    const HAPAccessory* hapAccessory;
    HAPAccessoryServer* hapAccessoryServer;

    /** UARP platform accessory storage. */
    struct uarpPlatformAccessory uarpPlatformAccessory;
    struct uarpPlatformAccessoryCallbacks const* uarpCallbacks;
    struct uarpPlatformOptionsObj uarpOptions;

    /** UARP controller objects. */
    UARPController controllers[kApp_NumDataStreams];

    /** Storage for UARP buffer requests. */
    UARPInternalStorage* uarpBuffers;
    /** Bitmask indicating buffer allocation for UARP platform controller buffers. */
    UARPAccessoryBufferTracker uarpPlatformControllerBufferIsAllocated;
    /** Bitmask indicating buffer allocation for UARP platform asset buffers. */
    UARPAccessoryBufferTracker uarpPlatformAssetBufferIsAllocated;
    /** Bitmask indicating buffer allocation for UARP data window buffers. */
    UARPAccessoryBufferTracker uarpDataWindowBufferIsAllocated;

    /** Storage for UARP transmit buffer requests. */
    uint8_t* uarpTxBuffers;
    uint8_t* streamTxScratchBuffers;
    /** Bitmask indicating buffer allocation for UARP transmit buffers. */
    UARPAccessoryBufferTracker uarpTxBufferIsAllocated;
    /** Bitmask indicating buffer allocation for stream transmit scratch buffers. */
    UARPAccessoryBufferTracker streamTxScratchBufferIsAllocated;

    /** Active firmware asset. */
    UARPFirmwareAsset activeFirmwareAsset;

    /**
     * Firmware asset pending acceptance.
     * Valid during the abandon/accept window associated with competing offers.
     */
    struct {
        UARPController* _Nullable controller;
        struct uarpPlatformAsset* _Nullable uarpPlatformAsset;
    } pendingFirmwareAsset;

    /** Staged firmware version. Initialized by accessory. */
    UARPVersion stagedFirmwareVersion;

    /** Accessory callbacks for firmware asset events. */
    const UARPAccessoryFirmwareAssetCallbacks* accessoryFirmwareAssetCallbacks;

    /** Init indicator. */
    bool isInitialized : 1;
    /** Message processing state. */
    bool isProcessingRxMsg : 1;
};

/**
 * UARP HDS Stream Application Protocol
 */
extern HAPStreamApplicationProtocol streamProtocolUARP;

/**
 * Initialize UARP.
 *
 * @param      server               HAP accessory server.
 * @param      accessory            HAP accessory containing accessory information.
 */
void UARPInitialize(HAPAccessoryServer* server, const HAPAccessory* accessory);

//----------------------------------------------------------------------------------------------------------------------
// UARP Firmware Update Accessory Interface
//----------------------------------------------------------------------------------------------------------------------

/**
 * Register for firmware assets.
 *
 * @param      callbacks                Callbacks for firmware asset events.
 * @param      stagedFirmwareVersion    Version of currently staged firmware.
 */
HAP_RESULT_USE_CHECK
HAPError UARPRegisterFirmwareAsset(
        const UARPAccessoryFirmwareAssetCallbacks* callbacks,
        UARPVersion* stagedFirmwareVersion);

/**
 * Request a payload from the SuperBinary by payload index.
 * Payloads are indexed starting from 0.
 * It is expected the payload processing order will be built into the accessory's update scheme or will be derived
 * from the SuperBinary metadata.
 *
 * @param      payloadIndex    Payload index to begin pulling.
 *
 * @return kHAPError_None            If successful.
 * @return kHAPError_InvalidState    If there is no active asset.
 * @return kHAPError_InvalidData     If the payload index is invalid.
 * @return kHAPError_Unknown         If the UARP library is unable to process the request.
 */
HAP_RESULT_USE_CHECK
HAPError UARPRequestFirmwareAssetPayloadByIndex(uint32_t payloadIndex);

/**
 * Changes the offset into the active payload.
 *
 * @param      payloadOffset   Relative offset into the payload.
 *
 * @return kHAPError_None            If successful.
 * @return kHAPError_InvalidState    If there is no active asset.
 * @return kHAPError_InvalidData     If the payload or payload offset is invalid.
 * @return kHAPError_Unknown         If the UARP library is unable to process the request.
 */
HAP_RESULT_USE_CHECK
HAPError UARPSetFirmwareAssetPayloadDataOffset(uint32_t payloadOffset);

/**
 * Indicate the asset staging has been completed.
 *
 * @param      stagedVersion   Version of the update staged by the accessory.
 *
 * @return kHAPError_None            If successful.
 * @return kHAPError_InvalidState    If there is no active asset.
 * @return kHAPError_Unknown         If the UARP library was unable to process the request.
 */
HAP_RESULT_USE_CHECK
HAPError UARPFirmwareAssetFullyStaged(UARPVersion* stagedVersion);

/**
 * Requests a change to the asset staging state.
 *
 * @param      state    Requested asset staging state change
 *
 * @return kHAPError_None            If successful.
 * @return kHAPError_InvalidState    If there is no active asset.
 * @return kHAPError_Unknown         If the UARP library is unable to process the request.
 */
HAP_RESULT_USE_CHECK
HAPError UARPRequestFirmwareAssetStagingStateChange(UARPAssetStagingStateChangeRequest state);

/**
 * Checks if the UARP version is zero.
 *
 * @param      version    UARP Version.
 *
 * @return true     If all fields of the version are zero.
 * @return false    If at least one version field is non-zero.
 */
bool UARPIsVersionZero(const UARPVersion* version);

/**
 * Checks if the UARP versions are equal.
 *
 * @param      version1    The 1st UARP Version for comparison.
 * @param      version2    The 2nd UARP Version for comparison.
 *
 * @return true     If all fields of the version are equal.
 * @return false    If at least one version field is not equal.
 */
bool UARPAreVersionsEqual(const UARPVersion* version1, const UARPVersion* version2);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif // (HAVE_UARP_SUPPORT == 1)

#endif
