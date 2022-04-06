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

#ifndef HAP_PLATFORM_CAMERA_H
#define HAP_PLATFORM_CAMERA_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPBase.h"
#include "HAPPlatformFeatures.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * @file
 *
 * \rst
 *
 * Cameras (with or without camera event recording) and video doorbells need to implement the HAPPlatformCamera.h API.
 * See :doc:`IP Camera  </ip_camera>` for more information.
 *
 * **Expected behavior:**
 *
 * Default implementation for Raspberry Pi that uses the ADK’s own implementation of SRTP etc. (HAP-RTP). It requires
 * implementation, and possibly adaptation, of the following helper APIs:
 *
 * - `HAPPlatformCameraInput+Init.h` / `HAPPlatformCameraInput.h`
 * - `HAPPlatformMicrophone+Init.h` / `HAPPlatformMicrophone.h`
 * - `HAPPlatformSpeaker+Init.h` / `HAPPlatformSpeaker.h`
 *
 * \endrst
 */

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN ((size_t) 16)
#endif
#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN ((size_t) 46)
#endif

/**
 * Camera provider.
 */
typedef struct HAPPlatformCamera HAPPlatformCamera;
typedef struct HAPPlatformCamera* HAPPlatformCameraRef;
HAP_NONNULL_SUPPORT(HAPPlatformCamera)

/**
 * Streaming status.
 */
HAP_ENUM_BEGIN(uint8_t, HAPCameraStreamingStatus) {
    /**
     * Available.
     *
     * - The session is uninitialized.
     */
    kHAPCameraStreamingStatus_Available,

    /**
     * In Use.
     *
     * - First HomeKit setup request marks a session as in use until end of streaming.
     */
    kHAPCameraStreamingStatus_InUse,

    /**
     * Unavailable.
     *
     * - Non-HomeKit reasons are making the stream unavailable.
     */
    kHAPCameraStreamingStatus_Unavailable
} HAP_ENUM_END(uint8_t, HAPCameraStreamingStatus);

/**
 * IP address.
 */
typedef struct {
    /**
     * IP address version.
     */
    HAPIPAddressVersion version;

    /**
     * IP address.
     */
    char ipString[INET6_ADDRSTRLEN];
} HAPPlatformCameraIPAddress;

HAP_STATIC_ASSERT(INET6_ADDRSTRLEN >= INET_ADDRSTRLEN, AllIpAddressesCanBeRepresented);

/**
 * Delegate that is used to monitor the camera stream status.
 */
typedef struct {
    /**
     * Client context pointer. Will be passed to callbacks.
     */
    void* _Nullable context;

    /**
     * Invoked when a stream status changes.
     *
     * @param      camera               IP Camera provider.
     * @param      streamIndex          The index of the stream whose status changed.
     * @param      context              The context pointer of the IP Camera provider delegate structure.
     */
    void (*_Nullable handleStreamStateChanged)(
            HAPPlatformCameraRef camera,
            size_t streamIndex,
            void* _Nullable context);
} HAPPlatformCameraDelegate;

/**
 * Specifies or clears the delegate for receiving camera events.
 *
 * - The delegate structure is copied and does not need to remain valid.
 *
 * @param      camera               IP Camera provider.
 * @param      delegate             The delegate to receive the camera events. NULL to clear.
 */
void HAPPlatformCameraSetDelegate(HAPPlatformCameraRef camera, const HAPPlatformCameraDelegate* _Nullable delegate);

/**
 * Reads the usage status of a camera stream.
 *
 * @param      camera               IP Camera provider.
 * @param      streamIndex          IP Camera stream index.
 *
 * @return                          The current stream status.
 */
HAP_RESULT_USE_CHECK
HAPCameraStreamingStatus HAPPlatformCameraGetStreamStatus(HAPPlatformCameraRef camera, size_t streamIndex);

/**
 * Tries to change the status of a camera stream.
 *
 * The status cannot be changed from Unavailable to InUse or from InUse to Unavailable.
 *
 * @param      camera               IP Camera provider.
 * @param      streamIndex          IP Camera stream index.
 * @param      status               The new stream status.

 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If an illegal state change was requested.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraTrySetStreamStatus(
        HAPPlatformCameraRef camera,
        size_t streamIndex,
        HAPCameraStreamingStatus status);

/**
 * Gets endpoint information of the IP Camera accessory.
 *
 * - The RTP port number must be >= 1024.
 *
 * @param      camera               IP Camera provider.
 * @param      streamIndex          IP Camera stream index.
 * @param      controllerAddress    Controller IP address.
 * @param[out] accessoryAddress     Accessory IP address.
 * @param[out] videoPort            Accessory port for the video stream of the RTP session.
 * @param[out] audioPort            Accessory port for the audio stream of the RTP session.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If an I/O error occurred.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraGetStreamingSessionEndpoint(
        HAPPlatformCameraRef camera,
        size_t streamIndex,
        const HAPPlatformCameraIPAddress* controllerAddress,
        HAPPlatformCameraIPAddress* accessoryAddress,
        HAPNetworkPort* videoPort,
        HAPNetworkPort* audioPort);

/**
 * SRTP parameters.
 */
typedef struct {
    /**
     * Crypto suite.
     */
    HAPSRTPCryptoSuite cryptoSuite;

    union {
        struct {
            uint8_t key[16];  /**< Master key. */
            uint8_t salt[14]; /**< Master salt. */
        } AES_CM_128_HMAC_SHA1_80;

        struct {
            uint8_t key[32];  /**< Master key. */
            uint8_t salt[14]; /**< Master salt. */
        } AES_256_CM_HMAC_SHA1_80;
    } _;
} HAPPlatformCameraSRTPParameters;

/**
 * Endpoint parameters.
 */
typedef struct {
    /** Local IP address for the streaming session. */
    HAPPlatformCameraIPAddress ipAddress;

    /** Video. */
    struct {
        /** Port for the video stream of the RTP session. */
        HAPNetworkPort port;

        /** RTP Parameters selected for the video streaming session. */
        HAPPlatformCameraSRTPParameters srtpParameters;

        /** Synchronization source for video RTP stream. */
        uint32_t ssrc;
    } video;

    /** Audio. */
    struct {
        /** Port for the audio stream of the RTP session. */
        HAPNetworkPort port;

        /** RTP Parameters selected for the audio streaming session. */
        HAPPlatformCameraSRTPParameters srtpParameters;

        /** Synchronization source for audio RTP stream. */
        uint32_t ssrc;
    } audio;
} HAPPlatformCameraEndpointParameters;

/**
 * RTP parameters.
 */
typedef struct {
    uint8_t type;              /**< Payload type. */
    uint32_t ssrc;             /**< Synchronization source. */
    uint16_t maximumBitRate;   /**< Maximum bit rate generated by the codec in kbps and averaged over 1 second. */
    float minimumRTCPInterval; /**< Minimum RTCP interval in seconds. */
    uint16_t maximumMTU;       /**< Video RTP packets MTU. If 0, use default (IPv4: 1378, IPv6: 1228). */
} HAPPlatformCameraRTPParameters;

/**
 * Comfort Noise.
 */
typedef struct {
    bool enabled; /**< Whether Comfort Noise has been selected. */
    uint8_t type; /**< Payload type, if Comfort Noise has been selected. */
} HAPPlatformCameraComfortNoise;

/**
 * Streaming session configuration (New session).
 */
typedef struct {
    /** Video. */
    struct {
        /** Video codec type. */
        HAPVideoCodecType codecType;

        /** Video codec parameters. */
        HAPH264VideoCodecParameters codecParameters;

        /** Video attributes. */
        HAPVideoAttributes attributes;

        /** Video RTP parameters. */
        HAPPlatformCameraRTPParameters rtpParameters;
    } video;

    /** Audio. */
    struct {
        /** Audio codec type. */
        HAPAudioCodecType codecType;

        /** Audio codec parameters. */
        HAPAudioCodecParameters codecParameters;

        /** Length of time represented by the media in a packet (ms). */
        uint8_t rtpTime;

        /** Audio RTP parameters. */
        HAPPlatformCameraRTPParameters rtpParameters;

        /** Comfort Noise. */
        HAPPlatformCameraComfortNoise comfortNoise;
    } audio;
} HAPPlatformCameraStartStreamingSessionConfiguration;

/**
 * Starts a streaming session.
 *
 * @param      camera               IP Camera provider.
 * @param      streamIndex          IP Camera stream index.
 * @param      controllerEndpoint   Controller endpoint information.
 * @param      accessoryEndpoint    Accessory endpoint information.
 * @param      streamConfiguration  Stream configuration.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If an I/O error occurred.
 * @return kHAPError_InvalidState   If the session is not in use, or has already been started.
 * @return kHAPError_InvalidData    If the stream configuration is not supported.
 * @return kHAPError_OutOfResources If not enough resources available.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraStartStreamingSession(
        HAPPlatformCameraRef camera,
        size_t streamIndex,
        const HAPPlatformCameraEndpointParameters* controllerEndpoint,
        const HAPPlatformCameraEndpointParameters* accessoryEndpoint,
        const HAPPlatformCameraStartStreamingSessionConfiguration* streamConfiguration);

/**
 * Suspends a streaming session.
 *
 * @param      camera               IP Camera provider.
 * @param      streamIndex          IP Camera stream index.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If an I/O error occurred.
 * @return kHAPError_InvalidState   If the session is not in use, or has not been started.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraSuspendStreamingSession(HAPPlatformCameraRef camera, size_t streamIndex);

/**
 * Resumes a streaming session.
 *
 * @param      camera               IP Camera provider.
 * @param      streamIndex          IP Camera stream index.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If an I/O error occurred.
 * @return kHAPError_InvalidState   If the session is not in use, or has not been started.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraResumeStreamingSession(HAPPlatformCameraRef camera, size_t streamIndex);

/**
 * Video RTP parameters (Update session).
 */
typedef struct {
    uint16_t maximumBitRate;   /**< Maximum bit rate generated by the codec in kbps and averaged over 1 second. */
    float minimumRTCPInterval; /**< Minimum RTCP interval in seconds. */
} HAPPlatformCameraReconfigureStreamingSessionVideoRTPParameters;

/**
 * Streaming session configuration (Update session).
 */
typedef struct {
    /** Video. */
    struct {
        /** Video attributes. */
        HAPVideoAttributes attributes;

        /** Video RTP parameters. */
        HAPPlatformCameraReconfigureStreamingSessionVideoRTPParameters rtpParameters;
    } video;
} HAPPlatformCameraReconfigureStreamingSessionConfiguration;

/**
 * Reconfigures a streaming session.
 *
 * @param      camera               IP Camera provider.
 * @param      streamIndex          IP Camera stream index.
 * @param      streamConfiguration  Stream configuration.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If an I/O error occurred.
 * @return kHAPError_InvalidState   If the session is not in use, or has not been started.
 * @return kHAPError_InvalidData    If the stream configuration is not supported.
 * @return kHAPError_OutOfResources If not enough resources available.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraReconfigureStreamingSession(
        HAPPlatformCameraRef camera,
        size_t streamIndex,
        const HAPPlatformCameraReconfigureStreamingSessionConfiguration* streamConfiguration);

/**
 * Ends a streaming session.
 *
 * @param      camera               IP Camera provider.
 * @param      streamIndex          IP Camera stream index.
 */
void HAPPlatformCameraEndStreamingSession(HAPPlatformCameraRef camera, size_t streamIndex);

//----------------------------------------------------------------------------------------------------------------------

typedef struct HAPPlatformCameraSnapshotReader HAPPlatformCameraSnapshotReader;

/**
 * IP Camera: Callback that is invoked to get the snapshot size.
 *
 * @param      snapshotReader       Snapshot reader.
 * @param[out] numBytes             The size of the snapshot.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If an I/O error occurred.
 */
HAP_RESULT_USE_CHECK
typedef HAPError (*HAPPlatformCameraSnapshotReaderGetSize)(
        const HAPPlatformCameraSnapshotReader* snapshotReader,
        size_t* numBytes);

/**
 * IP Camera: Callback that is invoked to read from snapshot.
 *
 * @param      snapshotReader       Snapshot reader.
 * @param[out] bytes                Buffer to load data into.
 * @param      maxBytes             Number of bytes to be read.
 * @param[out] numBytes             The number of bytes read.
 *
 * @return kHAPError_None           If successful.
 *                                  If (and only if) the end of the snapshot has been reached, numBytes is 0.
 * @return kHAPError_Unknown        If an I/O error occurred.
 */
HAP_RESULT_USE_CHECK
typedef HAPError (*HAPPlatformCameraSnapshotReaderRead)(
        const HAPPlatformCameraSnapshotReader* snapshotReader,
        void* bytes,
        size_t maxBytes,
        size_t* numBytes);

/**
 * IP Camera: Callback that is invoked to close a snapshot reader.
 *
 * @param      snapshotReader       Snapshot reader to be closed.
 */
typedef void (*HAPPlatformCameraSnapshotReaderClose)(const HAPPlatformCameraSnapshotReader* snapshotReader);

/**
 * IP Camera: snapshot reader.
 */
struct HAPPlatformCameraSnapshotReader {
    /**
     * Client context pointer.
     */
    void* _Nullable context;

    /**
     * Get snapshot size.
     */
    HAPPlatformCameraSnapshotReaderGetSize getSize;

    /**
     * Read from snapshot.
     */
    HAPPlatformCameraSnapshotReaderRead read;

    /**
     * Close snapshot reader.
     */
    HAPPlatformCameraSnapshotReaderClose close;
};
HAP_NONNULL_SUPPORT(HAPPlatformCameraSnapshotReader)

/**
 * Takes a Camera Snapshot.
 *
 * @param      camera               IP Camera provider.
 * @param      imageWidth           Width of the requested image.
 * @param      imageHeight          Height of the requested image.
 * @param[out] snapshotReader       Snapshot reader.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If an I/O error occurred.
 * @return kHAPError_OutOfResources If out of resources.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraTakeSnapshot(
        HAPPlatformCameraRef camera,
        uint16_t imageWidth,
        uint16_t imageHeight,
        HAPPlatformCameraSnapshotReader* _Nonnull* _Nonnull snapshotReader);

/**
 * Returns whether the RTP streaming feature is currently enabled.
 *
 * @param      camera               IP Camera provider.
 * @param      streamIndex          IP Camera stream index.
 * @param[out] isStreamingActive    Whether the RTP streaming feature is enabled or disabled.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If IP Camera configuration access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraIsStreamingActive(HAPPlatformCameraRef camera, size_t streamIndex, bool* isStreamingActive);

/**
 * Enables or disables the RTP streaming feature.
 *
 * @param      camera               IP Camera provider.
 * @param      streamIndex          IP Camera stream index.
 * @param      streamingActive      Whether RTP streaming should be enabled or disabled.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If IP Camera configuration access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraSetStreamingActive(HAPPlatformCameraRef camera, size_t streamIndex, bool streamingActive);

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns whether the recording feature is currently enabled.
 *
 * @param      camera               IP Camera provider.
 * @param[out] isRecordingActive    Whether the recording feature is enabled or disabled.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If IP Camera configuration access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraIsRecordingActive(HAPPlatformCameraRef camera, bool* isRecordingActive);

/**
 * Enables or disables the recording feature.
 *
 * @param      camera               IP Camera provider.
 * @param      recordingActive      Whether recordings should be enabled or disabled.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If IP Camera configuration access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraSetRecordingActive(HAPPlatformCameraRef camera, bool recordingActive);

/**
 * Gets the current recording configuration.
 *
 * @param      camera               IP Camera provider.
 * @param[out] found                True if a recording configuration has been applied.
 * @param[out] configuration        Current recording configuration, if available. Optional.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If IP Camera configuration access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraGetRecordingConfiguration(
        HAPPlatformCameraRef camera,
        bool* found,
        HAPCameraRecordingConfiguration* _Nullable configuration);

/**
 * Updates the recording configuration.
 *
 * @param      camera               IP Camera provider.
 * @param      configuration        New recording configuration.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If IP Camera configuration access failed.
 * @return kHAPError_InvalidData    If the recording configuration is not supported.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraSetRecordingConfiguration(
        HAPPlatformCameraRef camera,
        const HAPCameraRecordingConfiguration* configuration);

/**
 * Invalidates the recording configuration.
 *
 * @param      camera               IP Camera provider.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If IP Camera configuration access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraInvalidateRecordingConfiguration(HAPPlatformCameraRef camera);

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns whether the event snapshots are currently active.
 *
 * @param      camera               IP Camera provider.
 * @param[out] areSnapshotsActive   Whether the snapshots are enabled or disabled.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If IP Camera configuration access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraAreEventSnapshotsActive(HAPPlatformCameraRef camera, bool* areSnapshotsActive);

/**
 * Enables or disables the event snapshots feature.
 *
 * @param      camera               IP Camera provider.
 * @param      snapshotsActive      Whether snapshots should be enabled or disabled.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If IP Camera configuration access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraSetEventSnapshotsActive(HAPPlatformCameraRef camera, bool snapshotsActive);

/**
 * Returns whether the periodic snapshots are currently active.
 *
 * @param      camera               IP Camera provider.
 * @param[out] areSnapshotsActive   Whether the snapshots are enabled or disabled.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If IP Camera configuration access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraArePeriodicSnapshotsActive(HAPPlatformCameraRef camera, bool* areSnapshotsActive);

/**
 * Enables or disables the periodic snapshots feature.
 *
 * @param      camera               IP Camera provider.
 * @param      snapshotsActive      Whether snapshots should be enabled or disabled.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If IP Camera configuration access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraSetPeriodicSnapshotsActive(HAPPlatformCameraRef camera, bool snapshotsActive);

/**
 * Returns whether the HomeKit camera access is active.
 *
 * @param      camera               IP Camera provider.
 * @param[out] isHomeKitActive      Whether the HomeKit camera access is active.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If IP Camera configuration access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraIsHomeKitCameraActive(HAPPlatformCameraRef camera, bool* isHomeKitActive);

/**
 * Enables or disables the HomeKit camera access feature.
 *
 * @param      camera               IP Camera provider.
 * @param      homeKitActive        Whether HomeKit camera access should be active.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If IP Camera configuration access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraSetHomeKitCameraActive(HAPPlatformCameraRef camera, bool homeKitActive);

/**
 * Returns whether the third party camera access is active.
 *
 * @param      camera               IP Camera provider.
 * @param[out] isThirdPartyActive   Whether the third party camera access is active.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If IP Camera configuration access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraIsThirdPartyCameraActive(HAPPlatformCameraRef camera, bool* isThirdPartyActive);

/**
 * Returns whether the operating mode indicator is enabled.
 *
 * @param      camera               IP Camera provider.
 * @param[out] isIndicatorEnabled   Whether the operating mode indicator is enabled.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If IP Camera configuration access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraIsOperatingModeIndicatorEnabled(HAPPlatformCameraRef camera, bool* isIndicatorEnabled);

/**
 * Enables or disables the operating mode indicator.
 *
 * @param      camera               IP Camera provider.
 * @param      indicatorEnabled     Whether the operating mode indicator should be enabled.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If IP Camera configuration access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraSetOperatingModeIndicatorEnabled(HAPPlatformCameraRef camera, bool indicatorEnabled);

/**
 * Returns whether the recording audio is enabled.
 *
 * @param      camera               IP Camera provider.
 * @param[out] isAudioEnabled       Whether the recording audio is enabled.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If IP Camera configuration access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraIsRecordingAudioEnabled(HAPPlatformCameraRef camera, bool* isAudioEnabled);

/**
 * Enables or disables the recording audio.
 *
 * @param      camera               IP Camera provider.
 * @param      audioEnabled         Whether the recording audio should be enabled.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If IP Camera configuration access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraSetRecordingAudioEnabled(HAPPlatformCameraRef camera, bool audioEnabled);

/**
 * Returns whether the camera is manually disabled.
 *
 * @param      camera               IP Camera provider.
 * @param[out] isManuallyDisabled   Whether the camera is manually disabled.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If IP Camera configuration access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraIsManuallyDisabled(HAPPlatformCameraRef camera, bool* isManuallyDisabled);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
