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

#include "HAP+API.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)

#include "HAPAccessory+Camera.h"
#include "HAPAccessoryServer+Internal.h"
#include "HAPLog+Attributes.h"
#include "HAPLogSubsystem.h"
#include "HAPServiceTypes.h"
#include "HAPTLV+Internal.h"
#include "HAPVideo.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "RequestHandlers" };

/**
 * Timeout value to avoid perpetual starvation in cases where the controlling HAP session fails to make progress in
 * starting a streaming session.
 */
#define kHAPCamera_HAPSessionStartTimeout ((HAPTime)(30 * HAPSecond))

/**
 * Timeout value to avoid perpetual starvation in cases where the controlling HAP session fails to make progress in
 * resuming a streaming session.
 */
#define kHAPCamera_HAPSessionResumeTimeout ((HAPTime)(300 * HAPSecond))

// Note: While the stream is running it is up to the platform to maintain a progression timer. We only maintain timers
// while the stream is not started or suspended.

HAP_RESULT_USE_CHECK
static size_t GetStreamIndex(const HAPService* service, const HAPAccessory* accessory) {
    HAPPrecondition(service);
    HAPPrecondition(HAPUUIDAreEqual(service->serviceType, &kHAPServiceType_CameraRTPStreamManagement));
    HAPPrecondition(accessory);

    size_t streamIndex = 0;
    HAPPrecondition(accessory->services);
    for (size_t i = 0; accessory->services[i]; i++) {
        const HAPService* s = accessory->services[i];
        if (s == service) {
            return streamIndex;
        }

        // Count number of streaming services before the addressed one while skipping non-streaming services.
        if (HAPUUIDAreEqual(s->serviceType, &kHAPServiceType_CameraRTPStreamManagement)) {
            streamIndex++;
        }
    }
    HAPFatalError();
}

HAP_RESULT_USE_CHECK
HAPPlatformCameraRef HAPAccessoryGetCamera(HAPAccessoryServer* server, const HAPAccessory* accessory) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);

    HAPPlatformCameraRef _Nullable camera = NULL;
    if (accessory == server->primaryAccessory) {
        camera = server->platform.ip.camera;
    } else {
        if (server->ip.bridgedAccessories && server->ip.bridgedCameras) {
            for (size_t i = 0; server->ip.bridgedAccessories[i]; i++) {
                if (accessory == server->ip.bridgedAccessories[i]) {
                    camera = server->ip.bridgedCameras[i];
                    break;
                }
            }
        }
    }

    if (!camera) {
        HAPLogAccessoryError(&logObject, accessory, "No camera registered.");
        HAPFatalError();
    }
    return HAPNonnull(camera);
}

static void GetCameraAndIndex(
        HAPAccessoryServer* server,
        const HAPAccessory* accessory,
        HAPPlatformCameraRef* camera,
        size_t* baseIndex) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);
    HAPPrecondition(camera);
    HAPPrecondition(baseIndex);

    size_t index = 0;
    if (server->platform.ip.camera) {
        if (accessory == server->primaryAccessory) {
            *baseIndex = 0;
            *camera = server->platform.ip.camera;
            return;
        }
        // Increase index by number of streams in accessory.
        for (size_t j = 0; server->primaryAccessory->cameraStreamConfigurations[j]; j++) {
            index++;
        }
    }
    // Bridge.
    HAPAssert(server->ip.bridgedAccessories);
    HAPAssert(server->ip.bridgedCameras);
    const HAPPlatformCameraRef _Nullable* _Nullable cameras = server->ip.bridgedCameras;
    for (size_t i = 0; server->ip.bridgedAccessories[i]; i++) {
        const HAPAccessory* acc = server->ip.bridgedAccessories[i];
        HAPAssert(acc->cameraStreamConfigurations);
        if (acc == accessory) {
            HAPAssert(cameras[i]);
            *baseIndex = index;
            *camera = cameras[i];
            return;
        }
        // Increase index by number of streams in accessory.
        for (size_t j = 0; acc->cameraStreamConfigurations[j]; j++) {
            index++;
        }
    }
    // Not found.
    HAPLogError(&logObject, "Camera not found in accessories.");
    HAPFatalError();
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleCameraRTPStreamManagementActiveRead(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(HAPUUIDAreEqual(request->characteristic->characteristicType, &kHAPCharacteristicType_Active));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_CameraRTPStreamManagement));
    HAPPrecondition(value);

    HAPError err;

    size_t streamIndex = GetStreamIndex(request->service, request->accessory);
    HAPPlatformCameraRef camera = HAPAccessoryGetCamera(server, request->accessory);

    bool isStreamingActive;
    err = HAPPlatformCameraIsStreamingActive(camera, streamIndex, &isStreamingActive);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "HAPPlatformCameraIsStreamingActive failed: %u.", err);
        return err;
    }
    if (isStreamingActive) {
        *value = kHAPCharacteristicValue_Active_Active;
    } else {
        *value = kHAPCharacteristicValue_Active_Inactive;
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_ActiveIsValid(uint8_t value) {
    HAPAssert(sizeof value == sizeof(HAPCharacteristicValue_Active));
    switch ((HAPCharacteristicValue_Active) value) {
        case kHAPCharacteristicValue_Active_Inactive:
        case kHAPCharacteristicValue_Active_Active: {
            return true;
        }
        default:
            return false;
    }
}

static void InvalidateCameraStream(HAPAccessoryServer* server, const HAPAccessory* accessory, size_t streamIndex);

HAP_RESULT_USE_CHECK
HAPError HAPHandleCameraRTPStreamManagementActiveWrite(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value_,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(HAPUUIDAreEqual(request->characteristic->characteristicType, &kHAPCharacteristicType_Active));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_CameraRTPStreamManagement));
    HAPPrecondition(HAPCharacteristicValue_ActiveIsValid(value_));
    HAPCharacteristicValue_Active value = (HAPCharacteristicValue_Active) value_;

    HAPError err;

    size_t streamIndex = GetStreamIndex(request->service, request->accessory);
    HAPPlatformCameraRef camera = HAPAccessoryGetCamera(server, request->accessory);

    bool wasStreamingActive;
    err = HAPPlatformCameraIsStreamingActive(camera, streamIndex, &wasStreamingActive);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "HAPPlatformCameraIsStreamingActive failed: %u.", err);
        return err;
    }
    bool isStreamingActive = value == kHAPCharacteristicValue_Active_Active;

    if (isStreamingActive != wasStreamingActive) {
        HAPLogInfo(
                &logObject,
                "[%p:%zu] %s Camera RTP Streaming.",
                (const void*) camera,
                streamIndex,
                isStreamingActive ? "Enabling" : "Disabling");
        err = HAPPlatformCameraSetStreamingActive(camera, streamIndex, isStreamingActive);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPLogError(&logObject, "HAPPlatformCameraSetStreamingActive failed: %u.", err);
            return err;
        }
        if (!isStreamingActive) {
            InvalidateCameraStream(server, request->accessory, streamIndex);

            // Invalidate incomplete camera streaming setups.
            size_t numServices =
                    HAPAccessoryServerGetNumServiceInstances(server, &kHAPServiceType_CameraRTPStreamManagement);
            HAPAssert(numServices);
            HAPServiceTypeIndex serviceIndex =
                    HAPAccessoryServerGetServiceTypeIndex(server, request->service, request->accessory);
            HAPAssert(serviceIndex < numServices);
            HAPAssert(server->ip.camera.streamingSessionStorage.setups);
            for (size_t setupIndex = (size_t) serviceIndex;
                 setupIndex < server->ip.camera.streamingSessionStorage.numSetups;
                 setupIndex += HAPMin(numServices, server->ip.camera.streamingSessionStorage.numSetups - setupIndex)) {
                HAPRawBufferZero(
                        &server->ip.camera.streamingSessionStorage.setups[setupIndex],
                        sizeof *server->ip.camera.streamingSessionStorage.setups);
            }
        }
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }
    return kHAPError_None;
}

/**
 * Invalidate camera stream.
 *
 * @param      server               Accessory server.
 * @param      streamIndex          Camera stream index.
 */
static void InvalidateCameraStream(HAPAccessoryServer* server, const HAPAccessory* accessory, size_t streamIndex) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);

    HAPError err;

    HAPPlatformCameraRef camera;
    size_t baseIndex;
    GetCameraAndIndex(server, accessory, &camera, &baseIndex);
    HAPCameraStreamingSession* streamingSession =
            &server->ip.camera.streamingSessionStorage.sessions[baseIndex + streamIndex];

    if (streamingSession->hapSessionTimer) {
        // Deregister HAP session timer.
        HAPPlatformTimerDeregister(streamingSession->hapSessionTimer);
        streamingSession->hapSessionTimer = 0;
    }

    if (HAPPlatformCameraGetStreamStatus(camera, streamIndex) == kHAPCameraStreamingStatus_InUse) {
        // End stream.
        HAPPlatformCameraEndStreamingSession(camera, streamIndex);

        // Reset status.
        err = HAPPlatformCameraTrySetStreamStatus(camera, streamIndex, kHAPCameraStreamingStatus_Available);
        HAPAssert(err == kHAPError_None);
    }

    // Reset streaming session.
    HAPRawBufferZero(streamingSession, sizeof *streamingSession);
}

HAP_RESULT_USE_CHECK
bool HAPSessionHasActiveCameraStream(HAPSession* session) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPAccessoryServer* server = session->server;

    for (size_t index = 0; index < server->ip.camera.streamingSessionStorage.numSessions; index++) {
        HAPCameraStreamingSession* streamingSession = &server->ip.camera.streamingSessionStorage.sessions[index];
        // Check primary accessory.
        if (server->platform.ip.camera && streamingSession->hapSession == session &&
            streamingSession->isSessionActive) {
            HAPAssert(server->primaryAccessory);
            return true;
        }
    }
    if (server->ip.bridgedAccessories) {
        // Check all sessions in all accessories.
        size_t sessionIndex = 0;
        for (size_t accessoryIndex = 0; server->ip.bridgedAccessories[accessoryIndex]; accessoryIndex++) {
            const HAPAccessory* accessory = server->ip.bridgedAccessories[accessoryIndex];
            if (accessory->cameraStreamConfigurations != NULL) {
                for (size_t streamIdx = 0; accessory->cameraStreamConfigurations[streamIdx]; streamIdx++) {
                    HAPCameraStreamingSession* streamingSession =
                            &server->ip.camera.streamingSessionStorage.sessions[sessionIndex];
                    if (streamingSession->hapSession == session && streamingSession->isSessionActive) {
                        return true;
                    }
                    sessionIndex++;
                }
            }
        }
    }

    return false;
}

void HAPInvalidateCameraStreamForSession(HAPAccessoryServer* server, HAPSession* session) {
    HAPPrecondition(server);

    for (size_t index = 0; index < server->ip.camera.streamingSessionStorage.numSessions; index++) {
        HAPCameraStreamingSession* streamingSession = &server->ip.camera.streamingSessionStorage.sessions[index];
        // Check primary accessory.
        if (server->platform.ip.camera && streamingSession->hapSession == session) {
            HAPAssert(server->primaryAccessory);
            InvalidateCameraStream(server, server->primaryAccessory, index);
        }
    }
    if (server->ip.bridgedAccessories) {
        // Check all sessions in all accessories.
        size_t sessionIndex = 0;
        for (size_t accessoryIndex = 0; server->ip.bridgedAccessories[accessoryIndex]; accessoryIndex++) {
            const HAPAccessory* accessory = server->ip.bridgedAccessories[accessoryIndex];
            if (accessory->cameraStreamConfigurations != NULL) {
                for (size_t streamIdx = 0; accessory->cameraStreamConfigurations[streamIdx]; streamIdx++) {
                    HAPCameraStreamingSession* streamingSession =
                            &server->ip.camera.streamingSessionStorage.sessions[sessionIndex];
                    if (streamingSession->hapSession == session) {
                        InvalidateCameraStream(server, accessory, streamIdx);
                    }
                    sessionIndex++;
                }
            }
        }
    }

    // Invalidate incomplete camera streaming setups.
    size_t ipIndex = HAPAccessoryServerGetIPSessionIndex(server, session);
    size_t numServices = HAPAccessoryServerGetNumServiceInstances(server, &kHAPServiceType_CameraRTPStreamManagement);
    if (numServices) {
        HAPAssert(server->ip.camera.streamingSessionStorage.setups);
        HAPRawBufferZero(
                &server->ip.camera.streamingSessionStorage.setups[ipIndex * numServices],
                numServices * sizeof *server->ip.camera.streamingSessionStorage.setups);
    }
}

/**
 * Ends streaming.
 *
 * @param      server              Accessory server.
 * @param      service              Service that stops the stream.
 * @param      accessory            Accessory corresponding to the stream.
 */
static void EndStreaming(HAPAccessoryServer* server, const HAPService* service, const HAPAccessory* accessory) {
    HAPPrecondition(server);
    HAPPrecondition(service);
    HAPPrecondition(accessory);

    size_t streamIndex = GetStreamIndex(service, accessory);
    InvalidateCameraStream(server, accessory, streamIndex);
}

/**
 * Aborts streaming after an error occurred.
 *
 * @param      server              Accessory server.
 * @param      service              Service that was accessed when the error occurred.
 * @param      accessory            Accessory that was accessed when the error occurred.
 */
static void AbortStreaming(HAPAccessoryServer* server, const HAPService* service, const HAPAccessory* accessory) {
    HAPLog(&logObject, "Aborting streaming session due to error.");
    EndStreaming(server, service, accessory);
}

static void HandleHAPSessionTimerExpired(HAPPlatformTimerRef timer, void* _Nullable context) {
    HAPPrecondition(timer);
    HAPPrecondition(context);
    HAPCameraStreamingSession* streamingSession = (HAPCameraStreamingSession*) context;
    HAPPrecondition(timer == streamingSession->hapSessionTimer);

    HAPLogInfo(&logObject, "HAP session timer expired.");
    streamingSession->hapSessionTimer = 0;

    HAPAssert(streamingSession->hapSession);
    HAPSession* session = streamingSession->hapSession;
    HAPInvalidateCameraStreamForSession(session->server, HAPNonnull(streamingSession->hapSession));
}

/**
 * Temporary log buffer so that a full message may be logged in one go.
 */
typedef struct {
    char* _Nullable bytes; /**< Buffer to store the log message in. */
    size_t maxBytes;       /**< Capacity of buffer. */
    size_t numBytes;       /**< Length of data in buffer. */
} Log;

/**
 * Appends a message to a temporary log buffer.
 *
 * @param[in,out] logBuffer         Temporary log buffer to append to.
 * @param      format               A format string that produces a log message.
 * @param      ...                  Format string arguments.
 */
HAP_PRINTFLIKE(2, 3)
static void LogAppend(Log* logBuffer, const char* format, ...) {
    HAPPrecondition(format);
    HAPPrecondition(!logBuffer->bytes || logBuffer->maxBytes);

    HAPError err;

    if (!logBuffer->bytes) {
        return;
    }

    va_list arg;
    va_start(arg, format);
    size_t c = logBuffer->maxBytes - logBuffer->numBytes;
    HAPAssert(c > 0);
    err = HAPStringWithFormatAndArguments(&logBuffer->bytes[logBuffer->numBytes], c, format, arg);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPLog(&logObject, "Log truncated (buffer not large enough).");
        HAPAssert(c);
        logBuffer->numBytes += c - 1;
    } else {
        logBuffer->numBytes += HAPStringGetNumBytes(&logBuffer->bytes[logBuffer->numBytes]);
    }
    va_end(arg);
}

/**
 * Appends a separator to the TLV writer payload.
 *
 * @param      responseWriter       Writer to append separator TLV to.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If writer does not have enough capacity.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 18.1.1 TLV Rules
 */
HAP_RESULT_USE_CHECK
static HAPError AppendSeparator(HAPTLVWriter* responseWriter) {
    HAPPrecondition(responseWriter);

    HAPError err;
    err = HAPTLVWriterAppend(
            responseWriter, &(const HAPTLV) { .type = 0x00, .value = { .bytes = NULL, .numBytes = 0 } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }
    return kHAPError_None;
}

/**
 * Serializes streaming status.
 *
 * @param      streamingStatus      Value.
 *
 * @return Serialized value.
 */
HAP_RESULT_USE_CHECK
static HAPCharacteristicValue_StreamingStatus_Status
        SerializeStreamingStatus(HAPCameraStreamingStatus streamingStatus) {
    switch (streamingStatus) {
        case kHAPCameraStreamingStatus_Available:
            return kHAPCharacteristicValue_StreamingStatus_Status_Available;
        case kHAPCameraStreamingStatus_InUse:
            return kHAPCharacteristicValue_StreamingStatus_Status_InUse;
        case kHAPCameraStreamingStatus_Unavailable:
            return kHAPCharacteristicValue_StreamingStatus_Status_Unavailable;
        default:
            HAPFatalError();
    }
}

/**
 * Returns description of streaming status.
 *
 * @param      streamingStatus      Value.
 *
 * @return Description of value.
 */
HAP_RESULT_USE_CHECK
static const char* GetStreamingStatusDescription(HAPCameraStreamingStatus streamingStatus) {
    switch (streamingStatus) {
        case kHAPCameraStreamingStatus_Available:
            return "Available";
        case kHAPCameraStreamingStatus_InUse:
            return "In Use";
        case kHAPCameraStreamingStatus_Unavailable:
            return "Unavailable";
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleCameraRTPStreamManagementStreamingStatusRead(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->service);
    HAPPrecondition(request->accessory);
    HAPPrecondition(responseWriter);

    HAPError err;

    size_t streamIndex = GetStreamIndex(request->service, request->accessory);
    HAPPlatformCameraRef camera = HAPAccessoryGetCamera(server, request->accessory);

    // Get streaming status.
    HAPCameraStreamingStatus status = HAPPlatformCameraGetStreamStatus(camera, streamIndex);

    // Log.
    HAPLogInfo(
            &logObject,
            "Streaming Status:\n"
            "    Status = %s",
            GetStreamingStatusDescription(status));

    // Status.
    uint8_t streamingStatusBytes[] = { SerializeStreamingStatus(status) };
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPCharacteristicValue_StreamingStatus_Status,
                              .value = { .bytes = streamingStatusBytes, .numBytes = sizeof streamingStatusBytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    return kHAPError_None;
}

/**
 * Serializes a video codec type that is supported for streaming.
 *
 * @param      videoCodecType       Value.
 *
 * @return Serialized value.
 */
HAP_RESULT_USE_CHECK
static HAPCharacteristicValue_CameraStream_VideoCodecType SerializeVideoCodecType(HAPVideoCodecType videoCodecType) {
    HAPPrecondition(HAPVideoCodecTypeIsSupportedForStreaming(videoCodecType));

    switch (videoCodecType) { // NOLINT(hicpp-multiway-paths-covered)
        case kHAPVideoCodecType_H264: {
            return kHAPCharacteristicValue_CameraStream_VideoCodecType_H264;
        }
        default:
            HAPFatalError();
    }
}

/**
 * Returns description of a video codec type that is supported for streaming.
 *
 * @param      videoCodecType       Value.
 *
 * @return Description of value.
 */
HAP_RESULT_USE_CHECK
static const char* GetVideoCodecTypeDescription(HAPVideoCodecType videoCodecType) {
    HAPPrecondition(HAPVideoCodecTypeIsSupportedForStreaming(videoCodecType));

    return HAPVideoGetVideoCodecTypeDescription(videoCodecType);
}

/**
 * Serializes a supported video stream configuration.
 *
 * @param      configuration        Video stream configuration.
 * @param      responseWriter       TLV writer for serializing the response.
 * @param      l                    Temporary log buffer to append to.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If out of resources to process request.
 */
static HAPError SerializeSupportedVideoCodecConfiguration(
        const HAPCameraSupportedVideoCodecConfiguration* configuration,
        HAPTLVWriter* responseWriter,
        Log* l) {
    HAPPrecondition(configuration);
    HAPPrecondition(HAPVideoCodecTypeIsSupportedForStreaming(configuration->codecType));
    if (configuration->attributes) {
        for (size_t j = 0; configuration->attributes[j]; j++) {
            const HAPVideoAttributes* attributes = configuration->attributes[j];
            HAPPrecondition(attributes->maxFrameRate >= kHAPCameraSupportedVideoCodecConfiguration_MinFrameRate);
        }
    }
    HAPPrecondition(responseWriter);
    HAPPrecondition(l);

    HAPError err;

    // Video Codec Type.
    LogAppend(l, "    Video Codec Type: %s\n", GetVideoCodecTypeDescription(configuration->codecType));
    uint8_t bytesVideoCodecType[] = { SerializeVideoCodecType(configuration->codecType) };
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) {
                    .type = kHAPCharacteristicValue_SupportedVideoStreamConfiguration_VideoConfiguration_CodecType,
                    .value = { .bytes = bytesVideoCodecType, .numBytes = sizeof bytesVideoCodecType } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // Video Codec Parameters.
    LogAppend(l, "    Video Codec Parameters:\n");
    {
        HAPTLVWriter subWriter;
        HAPTLVCreateSubWriter(&subWriter, responseWriter);

        switch (configuration->codecType) {
            case kHAPVideoCodecType_H264: {
                const HAPH264VideoCodecParameters* parameters = configuration->codecParameters;

                // ProfileID.
                // One instance of this TLV must be present for each supported profile.
                LogAppend(l, "        ProfileIDs: ");
                bool appended = false;
                HAPH264VideoCodecProfile profile = parameters->profile;
                for (size_t j = 0; profile; j++) {
                    HAPH264VideoCodecProfile profileID = (HAPH264VideoCodecProfile)(1U << j);

                    if (profile & profileID) {
                        profile &= (HAPH264VideoCodecProfile) ~profileID;

                        // Append separator if necessary.
                        if (appended) {
                            LogAppend(l, ", ");
                            err = AppendSeparator(&subWriter);
                            if (err) {
                                HAPAssert(err == kHAPError_OutOfResources);
                                return err;
                            }
                        } else {
                            appended = true;
                        }

                        // Serialize.
                        LogAppend(l, "%s", HAPVideoGetH264ProfileIDDescription(profileID));
                        uint8_t bytesH264ProfileID[] = { HAPVideoSerializeH264ProfileID(profileID) };
                        err = HAPTLVWriterAppend(
                                &subWriter,
                                &(const HAPTLV) {
                                        .type = kHAPCharacteristicValue_CameraStream_H264VideoCodecParameter_Profile,
                                        .value = { .bytes = bytesH264ProfileID,
                                                   .numBytes = sizeof bytesH264ProfileID } });
                        if (err) {
                            HAPAssert(err == kHAPError_OutOfResources);
                            return err;
                        }
                    }
                }
                HAPPrecondition(appended);
                LogAppend(l, "\n");

                // Level.
                LogAppend(l, "        Level: ");
                appended = false;
                HAPH264VideoCodecProfileLevel level = parameters->level;
                for (size_t j = 0; level; j++) {
                    HAPH264VideoCodecProfileLevel levelID = (HAPH264VideoCodecProfileLevel)(1U << j);

                    if (level & levelID) {
                        level &= (HAPH264VideoCodecProfileLevel) ~levelID;

                        // Append separator if necessary.
                        if (appended) {
                            LogAppend(l, ", ");
                            err = AppendSeparator(&subWriter);
                            if (err) {
                                HAPAssert(err == kHAPError_OutOfResources);
                                return err;
                            }
                        } else {
                            appended = true;
                        }

                        // Serialize.
                        LogAppend(l, "%s", HAPVideoGetH264ProfileLevelDescription(levelID));
                        uint8_t bytesH264Level[] = { HAPVideoSerializeH264ProfileLevel(levelID) };
                        err = HAPTLVWriterAppend(
                                &subWriter,
                                &(const HAPTLV) {
                                        .type = kHAPCharacteristicValue_CameraStream_H264VideoCodecParameter_Level,
                                        .value = { .bytes = bytesH264Level, .numBytes = sizeof bytesH264Level } });
                        if (err) {
                            HAPAssert(err == kHAPError_OutOfResources);
                            return err;
                        }
                    }
                }
                HAPPrecondition(appended);
                LogAppend(l, "\n");

                // Packetization mode.
                // One instance of this TLV must be present for each supported mode.
                LogAppend(l, "        Packetization modes: ");
                appended = false;
                HAPH264VideoCodecPacketizationMode packetizationMode = parameters->packetizationMode;
                for (size_t j = 0; packetizationMode; j++) {
                    HAPH264VideoCodecPacketizationMode mode = (HAPH264VideoCodecPacketizationMode)(1U << j);

                    if (packetizationMode & mode) {
                        packetizationMode &= (HAPH264VideoCodecPacketizationMode) ~mode;

                        // Append separator if necessary.
                        if (appended) {
                            LogAppend(l, ", ");
                            err = AppendSeparator(&subWriter);
                            if (err) {
                                HAPAssert(err == kHAPError_OutOfResources);
                                return err;
                            }
                        } else {
                            appended = true;
                        }

                        // Serialize.
                        LogAppend(l, "%s", HAPVideoGetH264PacketizationModeDescription(mode));
                        uint8_t bytesH264PacketizationMode[] = { HAPVideoSerializeH264PacketizationMode(mode) };
                        err = HAPTLVWriterAppend(
                                &subWriter,
                                &(const HAPTLV) {
                                        .type = kHAPCharacteristicValue_CameraStream_H264VideoCodecParameter_PacketizationMode,
                                        .value = { .bytes = bytesH264PacketizationMode,
                                                   .numBytes = sizeof bytesH264PacketizationMode } });
                        if (err) {
                            HAPAssert(err == kHAPError_OutOfResources);
                            return err;
                        }
                    }
                }
                HAPPrecondition(appended);
                LogAppend(l, "\n");

                // Bit rate.
                if (parameters->bitRate) {
                    LogAppend(
                            l,
                            "        Maximum supported bit rate: %lu kbit/s (not sent to controller).",
                            (unsigned long) parameters->bitRate);
                }

                // I-Frame interval.
                if (parameters->iFrameInterval) {
                    LogAppend(
                            l,
                            "        Maximum supported I-Frame interval: %lu ms (not sent to controller).",
                            (unsigned long) parameters->iFrameInterval);
                } else {
                    LogAppend(
                            l,
                            "        Maximum supported I-Frame interval: %lu ms (not sent to controller).",
                            (unsigned long) 5000);
                }
                break;
            }
        }

        // Finalize.
        err = HAPTLVFinalizeSubWriter(
                &subWriter,
                responseWriter,
                kHAPCharacteristicValue_SupportedVideoStreamConfiguration_VideoConfiguration_CodecParameters);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
    }

    // Video Attributes.
    LogAppend(l, "    Video Attributes:\n");
    if (configuration->attributes) {
        for (size_t j = 0; configuration->attributes[j]; j++) {
            const HAPVideoAttributes* attributes = configuration->attributes[j];

            // Append separator if necessary.
            if (j) {
                LogAppend(l, "\n");
                err = AppendSeparator(responseWriter);
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }
            }

            // Video Attributes.
            LogAppend(l, "        %4u x %4u @ %u fps", attributes->width, attributes->height, attributes->maxFrameRate);
            {
                HAPTLVWriter subWriter;
                {
                    void* bytes;
                    size_t maxBytes;
                    HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);
                    HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
                }

                // Image width.
                uint8_t widthBytes[] = { HAPExpandLittleUInt16(attributes->width) };
                err = HAPTLVWriterAppend(
                        &subWriter,
                        &(const HAPTLV) { .type = kHAPCharacteristicValue_VideoCodecAttribute_ImageWidth,
                                          .value = { .bytes = widthBytes, .numBytes = sizeof widthBytes } });
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }

                // Image height.
                uint8_t heightBytes[] = { HAPExpandLittleUInt16(attributes->height) };
                err = HAPTLVWriterAppend(
                        &subWriter,
                        &(const HAPTLV) { .type = kHAPCharacteristicValue_VideoCodecAttribute_ImageHeight,
                                          .value = { .bytes = heightBytes, .numBytes = sizeof heightBytes } });
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }

                // Frame rate.
                uint8_t maxFrameRateBytes[] = { attributes->maxFrameRate };
                err = HAPTLVWriterAppend(
                        &subWriter,
                        &(const HAPTLV) {
                                .type = kHAPCharacteristicValue_VideoCodecAttribute_FrameRate,
                                .value = { .bytes = maxFrameRateBytes, .numBytes = sizeof maxFrameRateBytes } });
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }

                // Finalize.
                void* bytes;
                size_t numBytes;
                HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
                err = HAPTLVWriterAppend(
                        responseWriter,
                        &(const HAPTLV) {
                                .type = kHAPCharacteristicValue_SupportedVideoStreamConfiguration_VideoConfiguration_Attributes,
                                .value = { .bytes = bytes, .numBytes = numBytes } });
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }
            }
        }
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleCameraRTPStreamManagementSupportedVideoStreamConfigurationRead(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->service);
    HAPPrecondition(request->accessory);
    HAPPrecondition(responseWriter);

    HAPError err;

    size_t streamIndex = GetStreamIndex(request->service, request->accessory);

    const HAPCameraStreamSupportedConfigurations* streamConfiguration =
            request->accessory->cameraStreamConfigurations[streamIndex];

    // Enumerate video configurations.
    char logBytes[2048];
    HAPRawBufferZero(logBytes, sizeof logBytes);
    Log l = { .bytes = logBytes, .maxBytes = sizeof logBytes, .numBytes = 0 };
    LogAppend(&l, "Supported Video Codec Configurations:\n");
    if (streamConfiguration->videoStream.configurations) {
        for (size_t i = 0; streamConfiguration->videoStream.configurations[i]; i++) {
            const HAPCameraSupportedVideoCodecConfiguration* configuration =
                    streamConfiguration->videoStream.configurations[i];

            // Append separator if necessary.
            if (i) {
                LogAppend(&l, "\n\n");
                err = AppendSeparator(responseWriter);
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }
            }

            // Video Codec Configuration.
            LogAppend(&l, "Video Codec Configuration (%lu):\n", (unsigned long) i);
            {
                HAPTLVWriter subWriter;
                {
                    void* bytes;
                    size_t maxBytes;
                    HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);
                    HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
                }

                err = SerializeSupportedVideoCodecConfiguration(configuration, &subWriter, &l);
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }

                void* bytes;
                size_t numBytes;
                HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
                err = HAPTLVWriterAppend(
                        responseWriter,
                        &(const HAPTLV) {
                                .type = kHAPCharacteristicValue_SupportedVideoStreamConfiguration_VideoConfiguration,
                                .value = { .bytes = bytes, .numBytes = numBytes } });
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }
            }
        }
    }

    HAPLogInfo(&logObject, "%s", l.bytes);
    return kHAPError_None;
}

/**
 * Serializes an audio codec type that is supported for streaming.
 *
 * @param      audioCodecType       Value.
 *
 * @return Serialized value.
 */
HAP_RESULT_USE_CHECK
static HAPCharacteristicValue_AudioCodecType SerializeAudioCodecType(HAPAudioCodecType audioCodecType) {
    HAPPrecondition(HAPAudioCodecTypeIsSupportedForStreaming(audioCodecType));

    switch (audioCodecType) {
        case kHAPAudioCodecType_AAC_ELD: {
            return kHAPCharacteristicValue_AudioCodecType_AAC_ELD;
        }
        case kHAPAudioCodecType_Opus: {
            return kHAPCharacteristicValue_AudioCodecType_Opus;
        }
        case kHAPAudioCodecType_AMR: {
            return kHAPCharacteristicValue_AudioCodecType_AMR;
        }
        case kHAPAudioCodecType_AMR_WB: {
            return kHAPCharacteristicValue_AudioCodecType_AMR_WB;
        }
        case kHAPAudioCodecType_PCMU:
        case kHAPAudioCodecType_PCMA:
        case kHAPAudioCodecType_AAC_LC:
        case kHAPAudioCodecType_MSBC: {
            HAPFatalError();
        }
        default:
            HAPFatalError();
    }
}

/**
 * Returns description of an audio codec type that is supported for streaming.
 *
 * @param      audioCodecType       Value.
 *
 * @return Description of value.
 */
HAP_RESULT_USE_CHECK
static const char* GetAudioCodecTypeDescription(HAPAudioCodecType audioCodecType) {
    HAPPrecondition(HAPAudioCodecTypeIsSupportedForStreaming(audioCodecType));

    switch (audioCodecType) {
        case kHAPAudioCodecType_AAC_ELD: {
            return "AAC-ELD";
        }
        case kHAPAudioCodecType_Opus: {
            return "Opus";
        }
        case kHAPAudioCodecType_AMR: {
            return "AMR";
        }
        case kHAPAudioCodecType_AMR_WB: {
            return "AMR-WB";
        }
        case kHAPAudioCodecType_PCMU:
        case kHAPAudioCodecType_PCMA:
        case kHAPAudioCodecType_AAC_LC:
        case kHAPAudioCodecType_MSBC: {
            HAPFatalError();
        }
        default:
            HAPFatalError();
    }
}

/**
 * Serializes audio bit-rate.
 *
 * @param      audioBitRate         Value.
 *
 * @return Serialized value.
 */
HAP_RESULT_USE_CHECK
static HAPCharacteristicValue_AudioCodecBitRateControlMode
        SerializeAudioBitRate(HAPAudioCodecBitRateControlMode audioBitRate) {
    switch (audioBitRate) {
        case kHAPAudioCodecBitRateControlMode_Variable:
            return kHAPCharacteristicValue_AudioCodecBitRateControlMode_Variable;
        case kHAPAudioCodecBitRateControlMode_Constant:
            return kHAPCharacteristicValue_AudioCodecBitRateControlMode_Constant;
        default:
            HAPFatalError();
    }
}

/**
 * Returns description of audio bit rate.
 *
 * @param      audioBitRate         Value.
 *
 * @return Description of value.
 */
HAP_RESULT_USE_CHECK
static const char* GetAudioBitRateDescription(HAPAudioCodecBitRateControlMode audioBitRate) {
    switch (audioBitRate) {
        case kHAPAudioCodecBitRateControlMode_Variable:
            return "Variable bit-rate";
        case kHAPAudioCodecBitRateControlMode_Constant:
            return "Constant bit-rate";
        default:
            HAPFatalError();
    }
}

/**
 * Indicates whether an audio sample rate is supported for streaming.
 *
 * @param      audioSampleRate      Audio sample rate.
 *
 * @return true                     If the audio sample rate is supported for streaming.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
static bool AudioSampleRateIsSupportedForStreaming(HAPAudioCodecSampleRate audioSampleRate) {
    switch (audioSampleRate) {
        case kHAPAudioCodecSampleRate_8KHz:
        case kHAPAudioCodecSampleRate_16KHz:
        case kHAPAudioCodecSampleRate_24KHz: {
            return true;
        }
        case kHAPAudioCodecSampleRate_32KHz:
        case kHAPAudioCodecSampleRate_44_1KHz:
        case kHAPAudioCodecSampleRate_48KHz: {
            return false;
        }
        default:
            HAPFatalError();
    }
}

/**
 * Serializes audio sample rate.
 *
 * @param      audioSampleRate      Value.
 *
 * @return Serialized value.
 */
HAP_RESULT_USE_CHECK
static HAPCharacteristicValue_AudioCodecSampleRate SerializeAudioSampleRate(HAPAudioCodecSampleRate audioSampleRate) {
    HAPPrecondition(AudioSampleRateIsSupportedForStreaming(audioSampleRate));

    switch (audioSampleRate) {
        case kHAPAudioCodecSampleRate_8KHz: {
            return kHAPCharacteristicValue_AudioCodecSampleRate_8KHz;
        }
        case kHAPAudioCodecSampleRate_16KHz: {
            return kHAPCharacteristicValue_AudioCodecSampleRate_16KHz;
        }
        case kHAPAudioCodecSampleRate_24KHz: {
            return kHAPCharacteristicValue_AudioCodecSampleRate_24KHz;
        }
        case kHAPAudioCodecSampleRate_32KHz:
        case kHAPAudioCodecSampleRate_44_1KHz:
        case kHAPAudioCodecSampleRate_48KHz: {
            HAPFatalError();
        }
        default:
            HAPFatalError();
    }
}

/**
 * Returns description of audio sample rate.
 *
 * @param      audioSampleRate      Value.
 *
 * @return Description of value.
 */
HAP_RESULT_USE_CHECK
static const char* GetAudioSampleRateDescription(HAPAudioCodecSampleRate audioSampleRate) {
    HAPPrecondition(AudioSampleRateIsSupportedForStreaming(audioSampleRate));

    switch (audioSampleRate) {
        case kHAPAudioCodecSampleRate_8KHz: {
            return "8 KHz";
        }
        case kHAPAudioCodecSampleRate_16KHz: {
            return "16 KHz";
        }
        case kHAPAudioCodecSampleRate_24KHz: {
            return "24 KHz";
        }
        case kHAPAudioCodecSampleRate_32KHz:
        case kHAPAudioCodecSampleRate_44_1KHz:
        case kHAPAudioCodecSampleRate_48KHz: {
            HAPFatalError();
        }
        default:
            HAPFatalError();
    }
}

/**
 * Serializes a supported audio stream configuration.
 *
 * @param      configuration        Audio stream configuration.
 * @param      responseWriter       TLV writer for serializing the response.
 * @param      l                    Temporary log buffer to append to.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If out of resources to process request.
 */
static HAPError SerializeSupportedAudioCodecConfiguration(
        const HAPCameraSupportedAudioCodecConfiguration* configuration,
        HAPTLVWriter* responseWriter,
        Log* l) {
    HAPPrecondition(configuration);
    HAPPrecondition(HAPAudioCodecTypeIsSupportedForStreaming(configuration->codecType));
    HAPPrecondition(responseWriter);
    HAPPrecondition(l);

    HAPError err;

    // Audio Codec Type.
    LogAppend(l, "    Audio Codec Type: %s\n", GetAudioCodecTypeDescription(configuration->codecType));
    uint8_t bytesAudioCodecType[] = { SerializeAudioCodecType(configuration->codecType) };
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) {
                    .type = kHAPCharacteristicValue_SupportedAudioStreamConfiguration_AudioConfiguration_CodecType,
                    .value = { .bytes = bytesAudioCodecType, .numBytes = sizeof bytesAudioCodecType } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // Audio Codec Parameters.
    LogAppend(l, "    Audio Codec Parameters:\n");
    {
        HAPTLVWriter subWriter;
        {
            void* bytes;
            size_t maxBytes;
            HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);
            HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
        }

        const HAPAudioCodecParameters* parameters = configuration->codecParameters;

        // Audio channels.
        LogAppend(l, "        Audio channels: %u\n", parameters->numberOfChannels);
        uint8_t bytesNumberOfChannels[] = { parameters->numberOfChannels };
        err = HAPTLVWriterAppend(
                &subWriter,
                &(const HAPTLV) {
                        .type = kHAPCharacteristicValue_AudioCodecStreamParameter_AudioChannels,
                        .value = { .bytes = bytesNumberOfChannels, .numBytes = sizeof bytesNumberOfChannels } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }

        // Bit-rate.
        LogAppend(l, "        Bit-rate: ");
        bool appended = false;
        HAPAudioCodecBitRateControlMode bitRateMode = parameters->bitRateMode;
        for (size_t j = 0; bitRateMode; j++) {
            HAPAudioCodecBitRateControlMode rate = (HAPAudioCodecBitRateControlMode)(1U << j);

            if (bitRateMode & rate) {
                bitRateMode &= (HAPAudioCodecBitRateControlMode) ~rate;

                // Append separator if necessary.
                if (appended) {
                    LogAppend(l, ", ");
                    err = AppendSeparator(&subWriter);
                    if (err) {
                        HAPAssert(err == kHAPError_OutOfResources);
                        return err;
                    }
                } else {
                    appended = true;
                }

                // Serialize.
                LogAppend(l, "%s", GetAudioBitRateDescription(rate));
                uint8_t bytesAudioBitRate[] = { SerializeAudioBitRate(rate) };
                err = HAPTLVWriterAppend(
                        &subWriter,
                        &(const HAPTLV) {
                                .type = kHAPCharacteristicValue_AudioCodecStreamParameter_BitRate,
                                .value = { .bytes = bytesAudioBitRate, .numBytes = sizeof bytesAudioBitRate } });
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }
            }
        }
        HAPPrecondition(appended);
        LogAppend(l, "\n");

        // Sample rate.
        LogAppend(l, "        Sample rate: ");
        appended = false;
        HAPAudioCodecSampleRate sampleRate = parameters->sampleRate;
        for (size_t j = 0; sampleRate; j++) {
            HAPAudioCodecSampleRate rate = (HAPAudioCodecSampleRate)(1U << j);

            if (sampleRate & rate) {
                HAPPrecondition(AudioSampleRateIsSupportedForStreaming(rate));
                sampleRate &= (HAPAudioCodecSampleRate) ~rate;

                // Append separator if necessary.
                if (appended) {
                    LogAppend(l, ", ");
                    err = AppendSeparator(&subWriter);
                    if (err) {
                        HAPAssert(err == kHAPError_OutOfResources);
                        return err;
                    }
                } else {
                    appended = true;
                }

                // Serialize.
                LogAppend(l, "%s", GetAudioSampleRateDescription(rate));
                uint8_t bytesAudioSampleRate[] = { SerializeAudioSampleRate(rate) };
                err = HAPTLVWriterAppend(
                        &subWriter,
                        &(const HAPTLV) {
                                .type = kHAPCharacteristicValue_AudioCodecStreamParameter_SampleRate,
                                .value = { .bytes = bytesAudioSampleRate, .numBytes = sizeof bytesAudioSampleRate } });
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }
            }
        }
        HAPPrecondition(appended);
        LogAppend(l, "\n");

        // Finalize.
        void* bytes;
        size_t numBytes;
        HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
        err = HAPTLVWriterAppend(
                responseWriter,
                &(const HAPTLV) {
                        .type = kHAPCharacteristicValue_SupportedAudioStreamConfiguration_AudioConfiguration_CodecParameters,
                        .value = { .bytes = bytes, .numBytes = numBytes } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleCameraRTPStreamManagementSupportedAudioStreamConfigurationRead(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->service);
    HAPPrecondition(request->accessory);
    HAPPrecondition(responseWriter);

    HAPError err;

    size_t streamIndex = GetStreamIndex(request->service, request->accessory);

    const HAPCameraStreamSupportedConfigurations* streamConfiguration =
            request->accessory->cameraStreamConfigurations[streamIndex];

    // Enumerate audio configurations.
    char logBytes[2048];
    HAPRawBufferZero(logBytes, sizeof logBytes);
    Log l = { .bytes = logBytes, .maxBytes = sizeof logBytes, .numBytes = 0 };
    LogAppend(&l, "Supported Audio Codec Configurations:\n");
    if (streamConfiguration->audioStream.configurations) {
        for (size_t i = 0; streamConfiguration->audioStream.configurations[i]; i++) {
            const HAPCameraSupportedAudioCodecConfiguration* configuration =
                    streamConfiguration->audioStream.configurations[i];

            // Append separator if necessary.
            if (i) {
                LogAppend(&l, "\n");
                err = AppendSeparator(responseWriter);
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }
            }

            // Audio Codec Configuration.
            LogAppend(&l, "Audio Codec Configuration (%lu):\n", (unsigned long) i);
            {
                HAPTLVWriter subWriter;
                HAPTLVCreateSubWriter(&subWriter, responseWriter);

                err = SerializeSupportedAudioCodecConfiguration(configuration, &subWriter, &l);
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }

                err = HAPTLVFinalizeSubWriter(
                        &subWriter,
                        responseWriter,
                        kHAPCharacteristicValue_SupportedAudioStreamConfiguration_AudioConfiguration);
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }
            }
        }
    }

    // Comfort Noise.
    bool comfortNoiseSupported = streamConfiguration->audioStream.comfortNoise.supported;
    LogAppend(&l, "\nComfort Noise: %s", comfortNoiseSupported ? "Supported" : "Not supported");
    uint8_t bytesComfortNoiseSupported[] = { (uint8_t)(comfortNoiseSupported ? 1 : 0) };
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) {
                    .type = kHAPCharacteristicValue_SupportedAudioStreamConfiguration_ComfortNoiseSupport,
                    .value = { .bytes = bytesComfortNoiseSupported, .numBytes = sizeof bytesComfortNoiseSupported } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    HAPLogInfo(&logObject, "%s", l.bytes);
    return kHAPError_None;
}

/**
 * Serializes SRTP crypto suite.
 *
 * @param      srtpCryptoSuite      Value.
 *
 * @return Serialized value.
 */
HAP_RESULT_USE_CHECK
static HAPCharacteristicValue_SRTPCryptoSuite SerializeSRTPCryptoSuite(HAPSRTPCryptoSuite srtpCryptoSuite) {
    switch (srtpCryptoSuite) {
        case kHAPSRTPCryptoSuite_AES_CM_128_HMAC_SHA1_80:
            return kHAPCharacteristicValue_SRTPCryptoSuite_AES_CM_128_HMAC_SHA1_80;
        case kHAPSRTPCryptoSuite_AES_256_CM_HMAC_SHA1_80:
            return kHAPCharacteristicValue_SRTPCryptoSuite_AES_256_CM_HMAC_SHA1_80;
        case kHAPSRTPCryptoSuite_Disabled:
            return kHAPCharacteristicValue_SRTPCryptoSuite_Disabled;
        default:
            HAPFatalError();
    }
}

/**
 * Returns description of SRTP crypto suite.
 *
 * @param      srtpCryptoSuite      Value.
 *
 * @return Description of value.
 */
HAP_RESULT_USE_CHECK
static const char* GetSRTPCryptoSuiteDescription(HAPSRTPCryptoSuite srtpCryptoSuite) {
    switch (srtpCryptoSuite) {
        case kHAPSRTPCryptoSuite_AES_CM_128_HMAC_SHA1_80:
            return "AES_CM_128_HMAC_SHA1_80";
        case kHAPSRTPCryptoSuite_AES_256_CM_HMAC_SHA1_80:
            return "AES_256_CM_HMAC_SHA1_80";
        case kHAPSRTPCryptoSuite_Disabled:
            return "Disabled";
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleCameraRTPStreamManagementSupportedRTPConfigurationRead(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->service);
    HAPPrecondition(request->accessory);
    HAPPrecondition(responseWriter);

    HAPError err;

    size_t streamIndex = GetStreamIndex(request->service, request->accessory);

    const HAPCameraStreamSupportedConfigurations* streamConfiguration =
            request->accessory->cameraStreamConfigurations[streamIndex];

    // Enumerate RTP configurations.
    char logBytes[2048];
    HAPRawBufferZero(logBytes, sizeof logBytes);
    Log l = { .bytes = logBytes, .maxBytes = sizeof logBytes, .numBytes = 0 };
    LogAppend(&l, "Supported RTP Configurations:\n");
    size_t i = 0;
    bool appended = false;
    HAPSRTPCryptoSuite cryptoSuite = streamConfiguration->rtp.srtpCryptoSuites;
    for (size_t j = 0; cryptoSuite; j++) {
        HAPSRTPCryptoSuite suite = (HAPSRTPCryptoSuite)(1U << j);

        if (cryptoSuite & suite) {
            cryptoSuite &= (HAPSRTPCryptoSuite) ~suite;

            if (appended) {
                LogAppend(&l, "\n\n");
                err = AppendSeparator(responseWriter);
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }
            } else {
                appended = true;
            }

            // Log.
            LogAppend(&l, "RTP Configuration (%lu):\n", (unsigned long) i);

            // SRTP Crypto Suite.
            LogAppend(&l, "    Crypto Suite: %s", GetSRTPCryptoSuiteDescription(suite));
            uint8_t bytesSRTPCryptoSuite[] = { SerializeSRTPCryptoSuite(suite) };
            err = HAPTLVWriterAppend(
                    responseWriter,
                    &(const HAPTLV) {
                            .type = kHAPCharacteristicValue_SupportedRTPConfiguration_SRTPCryptoSuite,
                            .value = { .bytes = bytesSRTPCryptoSuite, .numBytes = sizeof bytesSRTPCryptoSuite } });
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }

            i++;
        }
    }

    HAPLogInfo(&logObject, "%s", l.bytes);
    return kHAPError_None;
}

/**
 * Serializes IP address version.
 *
 * @param      ipAddressVersion     Value.
 *
 * @return Serialized value.
 */
HAP_RESULT_USE_CHECK
static HAPCharacteristicValue_SetupEndpoints_Address_IPAddressVersion
        SerializeIPAddressVersion(HAPIPAddressVersion ipAddressVersion) {
    switch (ipAddressVersion) {
        case kHAPIPAddressVersion_IPv4:
            return kHAPCharacteristicValue_SetupEndpoints_Address_IPAddressVersion_IPv4;
        case kHAPIPAddressVersion_IPv6:
            return kHAPCharacteristicValue_SetupEndpoints_Address_IPAddressVersion_IPv6;
        default:
            HAPFatalError();
    }
}

/**
 * Returns description of IP address version.
 *
 * @param      ipAddressVersion     Value.
 *
 * @return Description of value.
 */
HAP_RESULT_USE_CHECK
static const char* GetIPAddressVersionDescription(HAPIPAddressVersion ipAddressVersion) {
    switch (ipAddressVersion) {
        case kHAPIPAddressVersion_IPv4:
            return "IPv4";
        case kHAPIPAddressVersion_IPv6:
            return "IPv6";
        default:
            HAPFatalError();
    }
}

/**
 * Serializes accessory address.
 *
 * @param      ipAddress            IP.
 * @param      videoRTPPort         Video RTP port.
 * @param      audioRTPPort         Audio RTP port.
 * @param      responseWriter       TLV writer.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If writer does not have enough capacity.
 */
HAP_RESULT_USE_CHECK
static HAPError SerializeAccessoryAddress(
        const HAPPlatformCameraIPAddress* ipAddress,
        const HAPNetworkPort videoRTPPort,
        const HAPNetworkPort audioRTPPort,
        HAPTLVWriter* responseWriter) {
    HAPPrecondition(ipAddress);
    HAPPrecondition(responseWriter);

    HAPError err;

    // IP address version.
    uint8_t bytesIPAddressVersion[] = { SerializeIPAddressVersion(ipAddress->version) };
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPCharacteristicValue_SetupEndpoints_Address_IPAddressVersion,
                              .value = { .bytes = bytesIPAddressVersion, .numBytes = sizeof bytesIPAddressVersion } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // IP address.
    {
        size_t numIPStringBytes = HAPStringGetNumBytes(ipAddress->ipString);
        switch (ipAddress->version) {
            case kHAPIPAddressVersion_IPv4: {
                HAPAssert(numIPStringBytes < INET_ADDRSTRLEN);
                break;
            }
            case kHAPIPAddressVersion_IPv6: {
                HAPAssert(numIPStringBytes < INET6_ADDRSTRLEN);
                break;
            }
        }
        err = HAPTLVWriterAppend(
                responseWriter,
                &(const HAPTLV) { .type = kHAPCharacteristicValue_SetupEndpoints_Address_IPAddress,
                                  .value = { .bytes = ipAddress->ipString, .numBytes = numIPStringBytes } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
    }

    // Video RTP port.
    HAPAssert(sizeof videoRTPPort == sizeof(uint16_t));
    uint8_t videoRTPPortBytes[] = { HAPExpandLittleUInt16(videoRTPPort) };
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPCharacteristicValue_SetupEndpoints_Address_VideoRTPPort,
                              .value = { .bytes = videoRTPPortBytes, .numBytes = sizeof videoRTPPortBytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // Audio RTP port.
    HAPAssert(sizeof audioRTPPort == sizeof(uint16_t));
    uint8_t audioRTPPortBytes[] = { HAPExpandLittleUInt16(audioRTPPort) };
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPCharacteristicValue_SetupEndpoints_Address_AudioRTPPort,
                              .value = { .bytes = audioRTPPortBytes, .numBytes = sizeof audioRTPPortBytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    return kHAPError_None;
}

/**
 * Serializes SRTP parameters.
 *
 * @param      srtpParameters       SRTP parameters.
 * @param      responseWriter       TLV writer.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If writer does not have enough capacity.
 */
static HAPError
        SerializeSRTPParameters(const HAPPlatformCameraSRTPParameters* srtpParameters, HAPTLVWriter* responseWriter) {
    HAPPrecondition(srtpParameters);
    HAPPrecondition(responseWriter);

    HAPError err;

    // SRTP Crypto Suite.
    uint8_t bytesSRTPCryptoSuite[] = { SerializeSRTPCryptoSuite(srtpParameters->cryptoSuite) };
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPCharacteristicValue_SetupEndpoints_SRTPParameters_CryptoSuite,
                              .value = { .bytes = bytesSRTPCryptoSuite, .numBytes = sizeof bytesSRTPCryptoSuite } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // SRTP Master Key, SRTP Master Salt.
    switch (srtpParameters->cryptoSuite) {
        case kHAPSRTPCryptoSuite_AES_CM_128_HMAC_SHA1_80: {
            err = HAPTLVWriterAppend(
                    responseWriter,
                    &(const HAPTLV) { .type = kHAPCharacteristicValue_SetupEndpoints_SRTPParameters_MasterKey,
                                      .value = { .bytes = srtpParameters->_.AES_CM_128_HMAC_SHA1_80.key,
                                                 .numBytes = sizeof srtpParameters->_.AES_CM_128_HMAC_SHA1_80.key } });
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }
            err = HAPTLVWriterAppend(
                    responseWriter,
                    &(const HAPTLV) { .type = kHAPCharacteristicValue_SetupEndpoints_SRTPParameters_MasterSalt,
                                      .value = { .bytes = srtpParameters->_.AES_CM_128_HMAC_SHA1_80.salt,
                                                 .numBytes = sizeof srtpParameters->_.AES_CM_128_HMAC_SHA1_80.salt } });
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }
            break;
        }
        case kHAPSRTPCryptoSuite_AES_256_CM_HMAC_SHA1_80: {
            err = HAPTLVWriterAppend(
                    responseWriter,
                    &(const HAPTLV) { .type = kHAPCharacteristicValue_SetupEndpoints_SRTPParameters_MasterKey,
                                      .value = { .bytes = srtpParameters->_.AES_256_CM_HMAC_SHA1_80.key,
                                                 .numBytes = sizeof srtpParameters->_.AES_256_CM_HMAC_SHA1_80.key } });
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }
            err = HAPTLVWriterAppend(
                    responseWriter,
                    &(const HAPTLV) { .type = kHAPCharacteristicValue_SetupEndpoints_SRTPParameters_MasterSalt,
                                      .value = { .bytes = srtpParameters->_.AES_256_CM_HMAC_SHA1_80.salt,
                                                 .numBytes = sizeof srtpParameters->_.AES_256_CM_HMAC_SHA1_80.salt } });
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }
            break;
        }
        case kHAPSRTPCryptoSuite_Disabled: {
            err = HAPTLVWriterAppend(
                    responseWriter,
                    &(const HAPTLV) { .type = kHAPCharacteristicValue_SetupEndpoints_SRTPParameters_MasterKey,
                                      .value = { .bytes = NULL, .numBytes = 0 } });
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }
            err = HAPTLVWriterAppend(
                    responseWriter,
                    &(const HAPTLV) { .type = kHAPCharacteristicValue_SetupEndpoints_SRTPParameters_MasterSalt,
                                      .value = { .bytes = NULL, .numBytes = 0 } });
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }
            break;
        }
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static HAPCameraStreamingSessionSetup* GetCameraStreamingSetup(
        HAPAccessoryServer* server,
        HAPSession* session,
        const HAPService* service,
        const HAPAccessory* accessory) {
    HAPPrecondition(server);
    HAPPrecondition(server->ip.camera.streamingSessionStorage.setups);
    HAPPrecondition(session);
    HAPPrecondition(service);
    HAPPrecondition(HAPUUIDAreEqual(service->serviceType, &kHAPServiceType_CameraRTPStreamManagement));
    HAPPrecondition(accessory);

    // Get IP session index.
    size_t ipIndex = HAPAccessoryServerGetIPSessionIndex(server, session);

    // Get Camera RTP Stream Management service index within attribute database.
    size_t numServices = HAPAccessoryServerGetNumServiceInstances(server, &kHAPServiceType_CameraRTPStreamManagement);
    HAPAssert(numServices);
    HAPServiceTypeIndex serviceIndex = HAPAccessoryServerGetServiceTypeIndex(server, service, accessory);
    HAPAssert(serviceIndex < numServices);

    // Compute camera streaming session setup index.
    size_t setupIndex = ipIndex * numServices + serviceIndex;
    HAPAssert(setupIndex < server->ip.camera.streamingSessionStorage.numSetups);

    // Fetch streaming session setup.
    return &server->ip.camera.streamingSessionStorage.setups[setupIndex];
}

HAP_RESULT_USE_CHECK
bool HAPCameraAreSnapshotsEnabled(
        HAPPlatformCameraRef camera,
        const HAPAccessory* accessory,
        HAPIPCameraSnapshotReason snapshotReason) {
    HAPAssert(camera);
    HAPAssert(accessory);
    HAPError err;

    // The accessory must reject any event snapshot request or snapshot request without a (valid) reason field
    // when the Event Snapshots Active characteristic is set to Disabled with HTTP status code 207
    // Multi-Status indicating HAP status code -70412 (Not allowed in the current state).
    // See HomeKit Accessory Protocol Specification R17
    // Section 11.129 Event Snapshots Active
    bool isEventSnapshotsActive = true;
    if (snapshotReason == kHAPIPCameraSnapshotReason_EventSnapshot ||
        snapshotReason == kHAPIPCameraSnapshotReason_Undefined) {
        err = HAPPlatformCameraAreEventSnapshotsActive(camera, &isEventSnapshotsActive);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPLogError(&logObject, "HAPPlatformCameraAreEventSnapshotsActive failed: %u.", err);
            return false;
        }
    }

    // The accessory must reject any periodic snapshot request or snapshot request without a (valid) reason field
    // when the Periodic Snapshots Active characteristic is set to Disabled with HTTP status code 207
    // Multi-Status indicating HAP status code -70412 (Not allowed in the current state).
    // See HomeKit Accessory Protocol Specification R17
    // Section 11.130 Periodic Snapshots Active
    bool isPeriodicSnapshotsActive = true;
    if (snapshotReason == kHAPIPCameraSnapshotReason_PeriodicSnapshot ||
        snapshotReason == kHAPIPCameraSnapshotReason_Undefined) {
        err = HAPPlatformCameraArePeriodicSnapshotsActive(camera, &isPeriodicSnapshotsActive);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPLogError(&logObject, "HAPPlatformCameraArePeriodicSnapshotsActive failed: %u.", err);
            return false;
        }
    }

    // The accessory must reject any snapshot request when Active is set to False on all Camera RTP Stream
    // Management Services on the accessory with HTTP status code 207 Multi-Status indicating HAP status code
    // -70412 (Not allowed in the current state).
    // See HomeKit Accessory Protocol Specification R17
    // Section 10.26 Camera RTP Stream Management
    bool isStreamingActive = false;
    size_t streamIndex = 0;
    HAPPrecondition(accessory->services);
    for (size_t i = 0; accessory->services[i] && !isStreamingActive; i++) {
        // Count number of streaming services.
        const HAPService* s = accessory->services[i];
        if (HAPUUIDAreEqual(s->serviceType, &kHAPServiceType_CameraRTPStreamManagement)) {
            err = HAPPlatformCameraIsStreamingActive(camera, streamIndex, &isStreamingActive);
            if (err) {
                HAPAssert(err == kHAPError_Unknown);
                HAPLogError(&logObject, "HAPPlatformCameraIsStreamingActive failed: %u.", err);
                return false;
            }
            streamIndex++;
        }
    }

    // The accessory must reject any snapshot request when HomeKit Camera Active is set to Off with HTTP
    // status code is 207 Multi-Status indicating HAP status code -70412 (Not allowed in the current state).
    // See HomeKit Accessory Protocol Specification R17
    // Section 11.131 HomeKit Camera Active
    bool isHomeKitCameraActive;
    err = HAPPlatformCameraIsHomeKitCameraActive(camera, &isHomeKitCameraActive);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "HAPPlatformCameraIsHomeKitCameraActive failed: %u.", err);
        return false;
    }

    // If camera is turned off with a physical button, it will override both HK and third party operating modes.
    // See HomeKit Accessory Protocol Specification R17
    // Section 11.136 Manually Disabled
    bool isManuallyDisabled;
    err = HAPPlatformCameraIsManuallyDisabled(camera, &isManuallyDisabled);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "HAPPlatformCameraIsManuallyDisabled failed: %u.", err);
        return false;
    }

    if (!isEventSnapshotsActive || !isPeriodicSnapshotsActive || !isStreamingActive || !isHomeKitCameraActive ||
        isManuallyDisabled) {
        HAPLog(&logObject, "Camera snapshots are currently disabled.");
        return false;
    }
    return true;
}

HAP_RESULT_USE_CHECK
static HAPError HAPCameraCheckStreamingEnabled(HAPPlatformCameraRef camera, size_t streamIndex) {
    // The accessory must reject any request to start a stream when Active is set to False and respond to any
    // read/write to the Setup Endpoints characteristic and the Selected RTP Stream Configuration
    // characteristic with HTTP 207 Multi-Status response including HAP status code -70412 (Not allowed in
    // the current state).
    // See HomeKit Accessory Protocol Specification R17
    // Section 10.26 Camera RTP Stream Management
    bool isStreamingActive;
    HAPError err = HAPPlatformCameraIsStreamingActive(camera, streamIndex, &isStreamingActive);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "HAPPlatformCameraIsStreamingActive failed: %u.", err);
        return err;
    }

    // The accessory must reject any request to start a stream for the Camera RTP Stream Management service
    // when HomeKit Camera Active is set to Off and respond to any read/write to the Setup Endpoints
    // characteristic and the Selected RTP Stream Configuration characteristic with HTTP 207 Multi-Status
    // response including HAP status code -70412 (Not allowed in the current state).
    // See HomeKit Accessory Protocol Specification R17
    // Section 11.131 HomeKit Camera Active
    bool isHomeKitCameraActive;
    err = HAPPlatformCameraIsHomeKitCameraActive(camera, &isHomeKitCameraActive);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "HAPPlatformCameraIsHomeKitCameraActive failed: %u.", err);
        return err;
    }

    // If camera is turned off with a physical button, it will override both HK and third party operating modes.
    // See HomeKit Accessory Protocol Specification R17
    // Section 11.136 Manually Disabled
    bool isManuallyDisabled;
    err = HAPPlatformCameraIsManuallyDisabled(camera, &isManuallyDisabled);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "HAPPlatformCameraIsManuallyDisabled failed: %u.", err);
        return err;
    }

    if (!isStreamingActive || !isHomeKitCameraActive || isManuallyDisabled) {
        HAPLog(&logObject, "RTP streaming is currently disabled.");
        return kHAPError_InvalidState;
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleCameraRTPStreamManagementSetupEndpointsRead(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->session);
    HAPPrecondition(request->service);
    HAPPrecondition(request->accessory);
    HAPPrecondition(responseWriter);

    HAPError err;

    size_t streamIndex = GetStreamIndex(request->service, request->accessory);

    HAPPlatformCameraRef camera;
    size_t baseIndex;
    GetCameraAndIndex(server, request->accessory, &camera, &baseIndex);
    size_t streamingSessionIndex = baseIndex + streamIndex;

    err = HAPCameraCheckStreamingEnabled(camera, streamIndex);
    if (err) {
        HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidState);
        return err;
    }

    // Fetch setup.
    HAPCameraStreamingSessionSetup* setup =
            GetCameraStreamingSetup(server, request->session, request->service, request->accessory);
    if (!setup->isActive) {
        HAPLog(&logObject, "No prepared streaming session setup (read without prior write).");

        // Status.
        uint8_t statusBytes[] = { kHAPCharacteristicValue_SetupEndpoints_Status_Error };
        err = HAPTLVWriterAppend(
                responseWriter,
                &(const HAPTLV) { .type = kHAPCharacteristicValue_SetupEndpoints_Status,
                                  .value = { .bytes = statusBytes, .numBytes = sizeof statusBytes } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }

        return kHAPError_None;
    }

    // Session Identifier.
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) {
                    .type = kHAPCharacteristicValue_SetupEndpoints_SessionID,
                    .value = { .bytes = setup->sessionID.value, .numBytes = sizeof setup->sessionID.value } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // Get camera streaming status.
    HAPCameraStreamingSession* streamingSession =
            &server->ip.camera.streamingSessionStorage.sessions[streamingSessionIndex];
    HAPCameraStreamingStatus status = HAPPlatformCameraGetStreamStatus(HAPNonnull(camera), streamIndex);

    switch (status) {
        case kHAPCameraStreamingStatus_InUse:
        case kHAPCameraStreamingStatus_Unavailable: {
            HAPLog(&logObject, "Camera stream is in use / unavailable. Replying busy.");
            HAPRawBufferZero(setup, sizeof *setup);

            // Status.
            uint8_t statusBytes[] = { kHAPCharacteristicValue_SetupEndpoints_Status_Busy };
            err = HAPTLVWriterAppend(
                    responseWriter,
                    &(const HAPTLV) { .type = kHAPCharacteristicValue_SetupEndpoints_Status,
                                      .value = { .bytes = statusBytes, .numBytes = sizeof statusBytes } });
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }
        }
            return kHAPError_None;
        case kHAPCameraStreamingStatus_Available: {
            // Transfer setup into session.
            HAPLog(&logObject, "Completing Setup Endpoints transaction.");
            HAPRawBufferZero(streamingSession, sizeof *streamingSession);
            HAPAssert(sizeof streamingSession->sessionID == sizeof setup->sessionID);
            HAPRawBufferCopyBytes(&streamingSession->sessionID, &setup->sessionID, sizeof setup->sessionID);
            HAPAssert(sizeof streamingSession->controllerEndpoint == sizeof setup->controllerEndpoint);
            HAPRawBufferCopyBytes(
                    &streamingSession->controllerEndpoint,
                    &setup->controllerEndpoint,
                    sizeof setup->controllerEndpoint);
            HAPRawBufferZero(setup, sizeof *setup);

            // Bind streaming session to HAP session.
            streamingSession->hapSession = request->session;

            // Try to claim stream.
            err = HAPPlatformCameraTrySetStreamStatus(camera, streamIndex, kHAPCameraStreamingStatus_InUse);
            if (err) {
                HAPAssert(err == kHAPError_InvalidState);
                HAPLogError(&logObject, "HAPPlatformCameraTrySetStreamStatus failed: %u.", err);
                AbortStreaming(server, request->service, request->accessory);

                // Status.
                uint8_t statusBytes[] = { kHAPCharacteristicValue_SetupEndpoints_Status_Error };
                err = HAPTLVWriterAppend(
                        responseWriter,
                        &(const HAPTLV) { .type = kHAPCharacteristicValue_SetupEndpoints_Status,
                                          .value = { .bytes = statusBytes, .numBytes = sizeof statusBytes } });
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }

                return kHAPError_None;
            }

            // Register HAP session timer.
            HAPAssert(!streamingSession->hapSessionTimer);
            HAPTime now = HAPPlatformClockGetCurrent();
            HAPTime deadline = now + kHAPCamera_HAPSessionStartTimeout;
            HAPAssert(deadline >= now);
            err = HAPPlatformTimerRegister(
                    &streamingSession->hapSessionTimer, deadline, HandleHAPSessionTimerExpired, streamingSession);
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                HAPLogError(&logObject, "Not enough timers available (IP camera HAP session timer).");
                AbortStreaming(server, request->service, request->accessory);
                return err;
            }
            HAPLogInfo(
                    &logObject,
                    "Waiting for Start (timeout: %llu seconds).",
                    (unsigned long long) (kHAPCamera_HAPSessionStartTimeout / HAPSecond));

            // Get endpoint information.
            HAPPlatformCameraEndpointParameters* endpoint = &streamingSession->accessoryEndpoint;
            err = HAPPlatformCameraGetStreamingSessionEndpoint(
                    camera,
                    streamIndex,
                    &streamingSession->controllerEndpoint.ipAddress,
                    &endpoint->ipAddress,
                    &endpoint->video.port,
                    &endpoint->audio.port);
            if (err) {
                HAPAssert(err == kHAPError_Unknown);
                HAPLogError(&logObject, "HAPPlatformCameraGetStreamingSessionEndpoint failed: %u.", err);
                AbortStreaming(server, request->service, request->accessory);

                // Status.
                uint8_t statusBytes[] = { kHAPCharacteristicValue_SetupEndpoints_Status_Error };
                err = HAPTLVWriterAppend(
                        responseWriter,
                        &(const HAPTLV) { .type = kHAPCharacteristicValue_SetupEndpoints_Status,
                                          .value = { .bytes = statusBytes, .numBytes = sizeof statusBytes } });
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }

                return kHAPError_None;
            }
            // The RTP port number must be >= 1024.
            // See HomeKit Accessory Protocol Specification R17
            // Section 11.4.5 Media Transport.
            if (endpoint->video.port < 1024) {
                HAPLogError(
                        &logObject,
                        "The port number for RTP must be >= 1024, but the port number for video is %u.",
                        endpoint->video.port);
            }
            if (endpoint->audio.port < 1024) {
                HAPLogError(
                        &logObject,
                        "The port number for RTP must be >= 1024, but the port number for audio is %u.",
                        endpoint->audio.port);
            }

            // Status.
            uint8_t statusBytes[] = { kHAPCharacteristicValue_SetupEndpoints_Status_Success };
            err = HAPTLVWriterAppend(
                    responseWriter,
                    &(const HAPTLV) { .type = kHAPCharacteristicValue_SetupEndpoints_Status,
                                      .value = { .bytes = statusBytes, .numBytes = sizeof statusBytes } });
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                AbortStreaming(server, request->service, request->accessory);
                return err;
            }

            // SRTP settings.
            endpoint->video.srtpParameters.cryptoSuite =
                    streamingSession->controllerEndpoint.video.srtpParameters.cryptoSuite;
            switch (endpoint->video.srtpParameters.cryptoSuite) {
                case kHAPSRTPCryptoSuite_AES_CM_128_HMAC_SHA1_80: {
                    HAPPlatformRandomNumberFill(
                            endpoint->video.srtpParameters._.AES_CM_128_HMAC_SHA1_80.key,
                            sizeof endpoint->video.srtpParameters._.AES_CM_128_HMAC_SHA1_80.key);
                    HAPPlatformRandomNumberFill(
                            endpoint->video.srtpParameters._.AES_CM_128_HMAC_SHA1_80.salt,
                            sizeof endpoint->video.srtpParameters._.AES_CM_128_HMAC_SHA1_80.salt);
                    break;
                }
                case kHAPSRTPCryptoSuite_AES_256_CM_HMAC_SHA1_80: {
                    HAPPlatformRandomNumberFill(
                            endpoint->video.srtpParameters._.AES_256_CM_HMAC_SHA1_80.key,
                            sizeof endpoint->video.srtpParameters._.AES_256_CM_HMAC_SHA1_80.key);
                    HAPPlatformRandomNumberFill(
                            endpoint->video.srtpParameters._.AES_256_CM_HMAC_SHA1_80.salt,
                            sizeof endpoint->video.srtpParameters._.AES_256_CM_HMAC_SHA1_80.salt);
                    break;
                }
                case kHAPSRTPCryptoSuite_Disabled: {
                    break;
                }
            }
            endpoint->audio.srtpParameters.cryptoSuite =
                    streamingSession->controllerEndpoint.audio.srtpParameters.cryptoSuite;
            switch (endpoint->audio.srtpParameters.cryptoSuite) {
                case kHAPSRTPCryptoSuite_AES_CM_128_HMAC_SHA1_80: {
                    HAPPlatformRandomNumberFill(
                            endpoint->audio.srtpParameters._.AES_CM_128_HMAC_SHA1_80.key,
                            sizeof endpoint->audio.srtpParameters._.AES_CM_128_HMAC_SHA1_80.key);
                    HAPPlatformRandomNumberFill(
                            endpoint->audio.srtpParameters._.AES_CM_128_HMAC_SHA1_80.salt,
                            sizeof endpoint->audio.srtpParameters._.AES_CM_128_HMAC_SHA1_80.salt);
                    break;
                }
                case kHAPSRTPCryptoSuite_AES_256_CM_HMAC_SHA1_80: {
                    HAPPlatformRandomNumberFill(
                            endpoint->audio.srtpParameters._.AES_256_CM_HMAC_SHA1_80.key,
                            sizeof endpoint->audio.srtpParameters._.AES_256_CM_HMAC_SHA1_80.key);
                    HAPPlatformRandomNumberFill(
                            endpoint->audio.srtpParameters._.AES_256_CM_HMAC_SHA1_80.salt,
                            sizeof endpoint->audio.srtpParameters._.AES_256_CM_HMAC_SHA1_80.salt);
                    break;
                }
                case kHAPSRTPCryptoSuite_Disabled: {
                    break;
                }
            }

            // SynchronizationSource.
            HAPPlatformRandomNumberFill(&endpoint->video.ssrc, sizeof endpoint->video.ssrc);
            do {
                HAPPlatformRandomNumberFill(&endpoint->audio.ssrc, sizeof endpoint->audio.ssrc);
            } while (endpoint->audio.ssrc == endpoint->video.ssrc);

            // Accessory Address.
            {
                HAPTLVWriter subWriter;
                {
                    void* bytes;
                    size_t maxBytes;
                    HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);
                    HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
                }

                err = SerializeAccessoryAddress(
                        &endpoint->ipAddress, endpoint->video.port, endpoint->audio.port, &subWriter);
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    AbortStreaming(server, request->service, request->accessory);
                    return err;
                }

                // Finalize.
                void* bytes;
                size_t numBytes;
                HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
                err = HAPTLVWriterAppend(
                        responseWriter,
                        &(const HAPTLV) { .type = kHAPCharacteristicValue_SetupEndpoints_Address,
                                          .value = { .bytes = bytes, .numBytes = numBytes } });
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    AbortStreaming(server, request->service, request->accessory);
                    return err;
                }
            }

            // SRTP Parameters for Video.
            {
                HAPTLVWriter subWriter;
                {
                    void* bytes;
                    size_t maxBytes;
                    HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);
                    HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
                }

                err = SerializeSRTPParameters(&endpoint->video.srtpParameters, &subWriter);
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    AbortStreaming(server, request->service, request->accessory);
                    return err;
                }

                // Finalize.
                void* bytes;
                size_t numBytes;
                HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
                err = HAPTLVWriterAppend(
                        responseWriter,
                        &(const HAPTLV) { .type = kHAPCharacteristicValue_SetupEndpoints_SRTPParametersForVideo,
                                          .value = { .bytes = bytes, .numBytes = numBytes } });
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    AbortStreaming(server, request->service, request->accessory);
                    return err;
                }
            }

            // SRTP Parameters for Audio.
            {
                HAPTLVWriter subWriter;
                {
                    void* bytes;
                    size_t maxBytes;
                    HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);
                    HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
                }

                err = SerializeSRTPParameters(&endpoint->audio.srtpParameters, &subWriter);
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    AbortStreaming(server, request->service, request->accessory);
                    return err;
                }

                // Finalize.
                void* bytes;
                size_t numBytes;
                HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
                err = HAPTLVWriterAppend(
                        responseWriter,
                        &(const HAPTLV) { .type = kHAPCharacteristicValue_SetupEndpoints_SRTPParametersForAudio,
                                          .value = { .bytes = bytes, .numBytes = numBytes } });
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    AbortStreaming(server, request->service, request->accessory);
                    return err;
                }
            }

            // SynchronizationSource for Video.
            uint8_t videoSSRCBytes[] = { HAPExpandLittleUInt32(endpoint->video.ssrc) };
            err = HAPTLVWriterAppend(
                    responseWriter,
                    &(const HAPTLV) { .type = kHAPCharacteristicValue_SetupEndpoints_SynchronizationSourceForVideo,
                                      .value = { .bytes = videoSSRCBytes, .numBytes = sizeof videoSSRCBytes } });
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                AbortStreaming(server, request->service, request->accessory);
                return err;
            }

            // SynchronizationSource for Audio.
            uint8_t audioSSRCBytes[] = { HAPExpandLittleUInt32(endpoint->audio.ssrc) };
            err = HAPTLVWriterAppend(
                    responseWriter,
                    &(const HAPTLV) { .type = kHAPCharacteristicValue_SetupEndpoints_SynchronizationSourceForAudio,
                                      .value = { .bytes = audioSSRCBytes, .numBytes = sizeof audioSSRCBytes } });
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                AbortStreaming(server, request->service, request->accessory);
                return err;
            }

            // Log.
            HAPLogInfo(
                    &logObject,
                    "Setup Endpoints:\n"
                    "    Session ID: %02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X\n"
                    "    Accessory Address: %s: %s\n"
                    "    Video:\n"
                    "        Port: %u\n"
                    "        SRTP crypto suite: %s\n"
                    "        SynchronizationSource: 0x%08X\n"
                    "    Audio:\n"
                    "        Port: %u\n"
                    "        SRTP crypto suite: %s\n"
                    "        SynchronizationSource: 0x%08X",
                    streamingSession->sessionID.value[0],
                    streamingSession->sessionID.value[1],
                    streamingSession->sessionID.value[2],
                    streamingSession->sessionID.value[3],
                    streamingSession->sessionID.value[4],
                    streamingSession->sessionID.value[5],
                    streamingSession->sessionID.value[6],
                    streamingSession->sessionID.value[7],
                    streamingSession->sessionID.value[8],
                    streamingSession->sessionID.value[9],
                    streamingSession->sessionID.value[10],
                    streamingSession->sessionID.value[11],
                    streamingSession->sessionID.value[12],
                    streamingSession->sessionID.value[13],
                    streamingSession->sessionID.value[14],
                    streamingSession->sessionID.value[15],
                    GetIPAddressVersionDescription(endpoint->ipAddress.version),
                    endpoint->ipAddress.ipString,
                    endpoint->video.port,
                    GetSRTPCryptoSuiteDescription(endpoint->video.srtpParameters.cryptoSuite),
                    (unsigned int) endpoint->video.ssrc,
                    endpoint->audio.port,
                    GetSRTPCryptoSuiteDescription(endpoint->audio.srtpParameters.cryptoSuite),
                    (unsigned int) endpoint->audio.ssrc);
            switch (endpoint->video.srtpParameters.cryptoSuite) {
                case kHAPSRTPCryptoSuite_AES_CM_128_HMAC_SHA1_80: {
                    HAPLogSensitiveBufferInfo(
                            &logObject,
                            endpoint->video.srtpParameters._.AES_CM_128_HMAC_SHA1_80.key,
                            sizeof endpoint->video.srtpParameters._.AES_CM_128_HMAC_SHA1_80.key,
                            "Video: SRTP Master Key (Receive)");
                    HAPLogSensitiveBufferInfo(
                            &logObject,
                            endpoint->video.srtpParameters._.AES_CM_128_HMAC_SHA1_80.salt,
                            sizeof endpoint->video.srtpParameters._.AES_CM_128_HMAC_SHA1_80.salt,
                            "Video: SRTP Master Salt (Receive)");
                    break;
                }
                case kHAPSRTPCryptoSuite_AES_256_CM_HMAC_SHA1_80: {
                    HAPLogSensitiveBufferInfo(
                            &logObject,
                            endpoint->video.srtpParameters._.AES_256_CM_HMAC_SHA1_80.key,
                            sizeof endpoint->video.srtpParameters._.AES_256_CM_HMAC_SHA1_80.key,
                            "Video: SRTP Master Key (Receive)");
                    HAPLogSensitiveBufferInfo(
                            &logObject,
                            endpoint->video.srtpParameters._.AES_256_CM_HMAC_SHA1_80.salt,
                            sizeof endpoint->video.srtpParameters._.AES_256_CM_HMAC_SHA1_80.salt,
                            "Video: SRTP Master Salt (Receive)");
                    break;
                }
                case kHAPSRTPCryptoSuite_Disabled: {
                    break;
                }
            }
            switch (endpoint->audio.srtpParameters.cryptoSuite) {
                case kHAPSRTPCryptoSuite_AES_CM_128_HMAC_SHA1_80: {
                    HAPLogSensitiveBufferInfo(
                            &logObject,
                            endpoint->audio.srtpParameters._.AES_CM_128_HMAC_SHA1_80.key,
                            sizeof endpoint->audio.srtpParameters._.AES_CM_128_HMAC_SHA1_80.key,
                            "Audio: SRTP Master Key (Receive)");
                    HAPLogSensitiveBufferInfo(
                            &logObject,
                            endpoint->audio.srtpParameters._.AES_CM_128_HMAC_SHA1_80.salt,
                            sizeof endpoint->audio.srtpParameters._.AES_CM_128_HMAC_SHA1_80.salt,
                            "Audio: SRTP Master Salt (Receive)");
                    break;
                }
                case kHAPSRTPCryptoSuite_AES_256_CM_HMAC_SHA1_80: {
                    HAPLogSensitiveBufferInfo(
                            &logObject,
                            endpoint->audio.srtpParameters._.AES_256_CM_HMAC_SHA1_80.key,
                            sizeof endpoint->audio.srtpParameters._.AES_256_CM_HMAC_SHA1_80.key,
                            "Audio: SRTP Master Key (Receive)");
                    HAPLogSensitiveBufferInfo(
                            &logObject,
                            endpoint->audio.srtpParameters._.AES_256_CM_HMAC_SHA1_80.salt,
                            sizeof endpoint->audio.srtpParameters._.AES_256_CM_HMAC_SHA1_80.salt,
                            "Audio: SRTP Master Salt (Receive)");
                    break;
                }
                case kHAPSRTPCryptoSuite_Disabled: {
                    break;
                }
            }
        }
            return kHAPError_None;
        default:
            HAPFatalError();
    }
}

/**
 * Parses IP address version.
 *
 * @param[out] ipAddressVersion     Parsed value.
 * @param      rawValue             Raw value.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the value is invalid.
 */
HAP_RESULT_USE_CHECK
static HAPError TryParseIPAddressVersion(HAPIPAddressVersion* ipAddressVersion, uint8_t rawValue) {
    HAPPrecondition(ipAddressVersion);

    HAPAssert(sizeof rawValue == sizeof(HAPCharacteristicValue_SetupEndpoints_Address_IPAddressVersion));
    switch ((HAPCharacteristicValue_SetupEndpoints_Address_IPAddressVersion) rawValue) {
        case kHAPCharacteristicValue_SetupEndpoints_Address_IPAddressVersion_IPv4: {
            *ipAddressVersion = kHAPIPAddressVersion_IPv4;
        }
            return kHAPError_None;
        case kHAPCharacteristicValue_SetupEndpoints_Address_IPAddressVersion_IPv6: {
            *ipAddressVersion = kHAPIPAddressVersion_IPv6;
        }
            return kHAPError_None;
        default: {
            HAPLog(&logObject, "IP address version invalid: %u.", rawValue);
        }
            return kHAPError_InvalidData;
    }
}

/**
 * Parses controller address.
 *
 * @param[out] ipAddress            Parsed IP.
 * @param[out] videoRTPPort         Parsed video RTP port.
 * @param[out] audioRTPPort         Parsed audio RTP port.
 * @param      requestReader        TLV reader.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the request reader contains invalid data.
 */
HAP_RESULT_USE_CHECK
static HAPError TryParseControllerAddress(
        HAPPlatformCameraIPAddress* ipAddress,
        HAPNetworkPort* videoRTPPort,
        HAPNetworkPort* audioRTPPort,
        HAPTLVReader* requestReader) {
    HAPPrecondition(ipAddress);
    HAPPrecondition(videoRTPPort);
    HAPPrecondition(audioRTPPort);
    HAPPrecondition(requestReader);

    HAPError err;

    HAPTLV ipAddressVersionTLV, ipAddressTLV, videoRTPPortTLV, audioRTPPortTLV;
    ipAddressVersionTLV.type = kHAPCharacteristicValue_SetupEndpoints_Address_IPAddressVersion;
    ipAddressTLV.type = kHAPCharacteristicValue_SetupEndpoints_Address_IPAddress;
    videoRTPPortTLV.type = kHAPCharacteristicValue_SetupEndpoints_Address_VideoRTPPort;
    audioRTPPortTLV.type = kHAPCharacteristicValue_SetupEndpoints_Address_AudioRTPPort;
    err = HAPTLVReaderGetAll(
            requestReader,
            (HAPTLV* const[]) { &ipAddressVersionTLV, &ipAddressTLV, &videoRTPPortTLV, &audioRTPPortTLV, NULL });
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }

    // IP address version.
    if (!ipAddressVersionTLV.value.bytes) {
        HAPLog(&logObject, "IP address version missing.");
        return kHAPError_InvalidData;
    }
    if (ipAddressVersionTLV.value.numBytes != 1) {
        HAPLog(&logObject,
               "IP address version has invalid length (%lu).",
               (unsigned long) ipAddressVersionTLV.value.numBytes);
        return kHAPError_InvalidData;
    }
    err = TryParseIPAddressVersion(&ipAddress->version, ((const uint8_t*) ipAddressVersionTLV.value.bytes)[0]);
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }

    // IP Address.
    if (!ipAddressTLV.value.bytes) {
        HAPLog(&logObject, "IP Address missing.");
        return kHAPError_InvalidData;
    }
    switch (ipAddress->version) {
        case kHAPIPAddressVersion_IPv4: {
            if (ipAddressTLV.value.numBytes >= INET_ADDRSTRLEN) {
                HAPLog(&logObject, "IP Address has invalid length (%zu).", ipAddressTLV.value.numBytes);
                return kHAPError_InvalidData;
            }
            break;
        }
        case kHAPIPAddressVersion_IPv6: {
            if (ipAddressTLV.value.numBytes >= INET6_ADDRSTRLEN) {
                HAPLog(&logObject, "IP Address has invalid length (%zu).", ipAddressTLV.value.numBytes);
                return kHAPError_InvalidData;
            }
            break;
        }
    }
    HAPRawBufferCopyBytes(ipAddress->ipString, HAPNonnullVoid(ipAddressTLV.value.bytes), ipAddressTLV.value.numBytes);

    // Video RTP port.
    if (!videoRTPPortTLV.value.bytes) {
        HAPLog(&logObject, "Video RTP port missing.");
        return kHAPError_InvalidData;
    }
    if (videoRTPPortTLV.value.numBytes != sizeof *videoRTPPort) {
        HAPLog(&logObject, "Video RTP port has invalid length (%zu).", videoRTPPortTLV.value.numBytes);
        return kHAPError_InvalidData;
    }
    HAPAssert(sizeof *videoRTPPort == sizeof(uint16_t));
    *videoRTPPort = HAPReadLittleUInt16(videoRTPPortTLV.value.bytes);

    // Audio RTP port.
    if (!audioRTPPortTLV.value.bytes) {
        HAPLog(&logObject, "Audio RTP port missing.");
        return kHAPError_InvalidData;
    }
    if (audioRTPPortTLV.value.numBytes != sizeof *audioRTPPort) {
        HAPLog(&logObject, "Audio RTP port has invalid length (%zu).", audioRTPPortTLV.value.numBytes);
        return kHAPError_InvalidData;
    }
    HAPAssert(sizeof *audioRTPPort == sizeof(uint16_t));
    *audioRTPPort = HAPReadLittleUInt16(audioRTPPortTLV.value.bytes);

    return kHAPError_None;
}

/**
 * Parses SRTP crypto suite.
 *
 * @param[out] srtpCryptoSuite      Parsed value.
 * @param      rawValue             Raw value.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the value is invalid.
 */
HAP_RESULT_USE_CHECK
static HAPError TryParseSRTPCryptoSuite(HAPSRTPCryptoSuite* srtpCryptoSuite, uint8_t rawValue) {
    HAPPrecondition(srtpCryptoSuite);

    HAPAssert(sizeof rawValue == sizeof(HAPCharacteristicValue_SRTPCryptoSuite));
    switch ((HAPCharacteristicValue_SRTPCryptoSuite) rawValue) {
        case kHAPCharacteristicValue_SRTPCryptoSuite_AES_CM_128_HMAC_SHA1_80: {
            *srtpCryptoSuite = kHAPSRTPCryptoSuite_AES_CM_128_HMAC_SHA1_80;
        }
            return kHAPError_None;
        case kHAPCharacteristicValue_SRTPCryptoSuite_AES_256_CM_HMAC_SHA1_80: {
            *srtpCryptoSuite = kHAPSRTPCryptoSuite_AES_256_CM_HMAC_SHA1_80;
        }
            return kHAPError_None;
        case kHAPCharacteristicValue_SRTPCryptoSuite_Disabled: {
            *srtpCryptoSuite = kHAPSRTPCryptoSuite_Disabled;
        }
            return kHAPError_None;
        default: {
            HAPLog(&logObject, "SRTP crypto suite invalid: %u.", rawValue);
        }
            return kHAPError_InvalidData;
    }
}

/**
 * Parses SRTP parameters.
 *
 * @param[out] srtpParameters       Parsed SRTP parameters.
 * @param      requestReader        TLV reader.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the request reader contains invalid data.
 */
HAP_RESULT_USE_CHECK
static HAPError TryParseSRTPParameters(HAPPlatformCameraSRTPParameters* srtpParameters, HAPTLVReader* requestReader) {
    HAPPrecondition(srtpParameters);
    HAPPrecondition(requestReader);

    HAPError err;

    HAPTLV srtpCryptoSuiteTLV, srtpMasterKeyTLV, srtpMasterSaltTLV;
    srtpCryptoSuiteTLV.type = kHAPCharacteristicValue_SetupEndpoints_SRTPParameters_CryptoSuite;
    srtpMasterKeyTLV.type = kHAPCharacteristicValue_SetupEndpoints_SRTPParameters_MasterKey;
    srtpMasterSaltTLV.type = kHAPCharacteristicValue_SetupEndpoints_SRTPParameters_MasterSalt;
    err = HAPTLVReaderGetAll(
            requestReader, (HAPTLV* const[]) { &srtpCryptoSuiteTLV, &srtpMasterKeyTLV, &srtpMasterSaltTLV, NULL });
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }

    // SRTP Crypto Suite.
    if (!srtpCryptoSuiteTLV.value.bytes) {
        HAPLog(&logObject, "SRTP Crypto Suite missing.");
        return kHAPError_InvalidData;
    }
    if (srtpCryptoSuiteTLV.value.numBytes != 1) {
        HAPLog(&logObject,
               "SRTP Crypto Suite has invalid length (%lu).",
               (unsigned long) srtpCryptoSuiteTLV.value.numBytes);
        return kHAPError_InvalidData;
    }
    err = TryParseSRTPCryptoSuite(&srtpParameters->cryptoSuite, ((const uint8_t*) srtpCryptoSuiteTLV.value.bytes)[0]);
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }

    // SRTP Master Key, SRTP Master Salt.
    if (!srtpMasterKeyTLV.value.bytes) {
        HAPLog(&logObject, "SRTP Master Key missing.");
        return kHAPError_InvalidData;
    }
    if (!srtpMasterSaltTLV.value.bytes) {
        HAPLog(&logObject, "SRTP Master Salt missing.");
        return kHAPError_InvalidData;
    }
    switch (srtpParameters->cryptoSuite) {
        case kHAPSRTPCryptoSuite_AES_CM_128_HMAC_SHA1_80: {
            if (srtpMasterKeyTLV.value.numBytes != sizeof srtpParameters->_.AES_CM_128_HMAC_SHA1_80.key) {
                HAPLog(&logObject,
                       "SRTP Master Key has invalid length (%lu).",
                       (unsigned long) srtpMasterKeyTLV.value.numBytes);
                return kHAPError_InvalidData;
            }
            if (srtpMasterSaltTLV.value.numBytes != sizeof srtpParameters->_.AES_CM_128_HMAC_SHA1_80.salt) {
                HAPLog(&logObject,
                       "SRTP Master Salt has invalid length (%lu).",
                       (unsigned long) srtpMasterSaltTLV.value.numBytes);
                return kHAPError_InvalidData;
            }
            HAPRawBufferCopyBytes(
                    srtpParameters->_.AES_CM_128_HMAC_SHA1_80.key,
                    HAPNonnullVoid(srtpMasterKeyTLV.value.bytes),
                    srtpMasterKeyTLV.value.numBytes);
            HAPRawBufferCopyBytes(
                    srtpParameters->_.AES_CM_128_HMAC_SHA1_80.salt,
                    HAPNonnullVoid(srtpMasterSaltTLV.value.bytes),
                    srtpMasterSaltTLV.value.numBytes);
            break;
        }
        case kHAPSRTPCryptoSuite_AES_256_CM_HMAC_SHA1_80: {
            if (srtpMasterKeyTLV.value.numBytes != sizeof srtpParameters->_.AES_256_CM_HMAC_SHA1_80.key) {
                HAPLog(&logObject,
                       "SRTP Master Key has invalid length (%lu).",
                       (unsigned long) srtpMasterKeyTLV.value.numBytes);
                return kHAPError_InvalidData;
            }
            if (srtpMasterSaltTLV.value.numBytes != sizeof srtpParameters->_.AES_256_CM_HMAC_SHA1_80.salt) {
                HAPLog(&logObject,
                       "SRTP Master Salt has invalid length (%lu).",
                       (unsigned long) srtpMasterSaltTLV.value.numBytes);
                return kHAPError_InvalidData;
            }
            HAPRawBufferCopyBytes(
                    srtpParameters->_.AES_256_CM_HMAC_SHA1_80.key,
                    HAPNonnullVoid(srtpMasterKeyTLV.value.bytes),
                    srtpMasterKeyTLV.value.numBytes);
            HAPRawBufferCopyBytes(
                    srtpParameters->_.AES_256_CM_HMAC_SHA1_80.salt,
                    HAPNonnullVoid(srtpMasterSaltTLV.value.bytes),
                    srtpMasterSaltTLV.value.numBytes);
            break;
        }
        case kHAPSRTPCryptoSuite_Disabled: {
            if (srtpMasterKeyTLV.value.numBytes != 0) {
                HAPLog(&logObject,
                       "SRTP Master Key has invalid length (%lu).",
                       (unsigned long) srtpMasterKeyTLV.value.numBytes);
                return kHAPError_InvalidData;
            }
            if (srtpMasterSaltTLV.value.numBytes != 0) {
                HAPLog(&logObject,
                       "SRTP Master Salt has invalid length (%lu).",
                       (unsigned long) srtpMasterSaltTLV.value.numBytes);
                return kHAPError_InvalidData;
            }
            break;
        }
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleCameraRTPStreamManagementSetupEndpointsWrite(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicWriteRequest* request,
        HAPTLVReader* requestReader,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->session);
    HAPPrecondition(request->service);
    HAPPrecondition(request->accessory);
    HAPPrecondition(requestReader);

    HAPError err;

    size_t streamIndex = GetStreamIndex(request->service, request->accessory);

    HAPPlatformCameraRef camera = HAPAccessoryGetCamera(server, request->accessory);

    err = HAPCameraCheckStreamingEnabled(camera, streamIndex);
    if (err) {
        HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidState);
        return err;
    }

    HAPCameraStreamingSessionID sessionID;
    HAPPlatformCameraEndpointParameters endpoint;

    HAPRawBufferZero(&sessionID, sizeof sessionID);
    HAPRawBufferZero(&endpoint, sizeof endpoint);

    HAPTLV sessionIDTLV, addressTLV, srtpParametersForVideoTLV, srtpParametersForAudioTLV;
    sessionIDTLV.type = kHAPCharacteristicValue_SetupEndpoints_SessionID;
    addressTLV.type = kHAPCharacteristicValue_SetupEndpoints_Address;
    srtpParametersForVideoTLV.type = kHAPCharacteristicValue_SetupEndpoints_SRTPParametersForVideo;
    srtpParametersForAudioTLV.type = kHAPCharacteristicValue_SetupEndpoints_SRTPParametersForAudio;
    err = HAPTLVReaderGetAll(
            requestReader,
            (HAPTLV* const[]) {
                    &sessionIDTLV, &addressTLV, &srtpParametersForVideoTLV, &srtpParametersForAudioTLV, NULL });
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }

    // Session ID.
    if (!sessionIDTLV.value.bytes) {
        HAPLog(&logObject, "Session ID missing.");
        return kHAPError_InvalidData;
    }
    if (sessionIDTLV.value.numBytes != sizeof sessionID.value) {
        HAPLog(&logObject, "Session ID has invalid length (%lu).", (unsigned long) sessionIDTLV.value.numBytes);
        return kHAPError_InvalidData;
    }
    HAPRawBufferCopyBytes(sessionID.value, HAPNonnullVoid(sessionIDTLV.value.bytes), sessionIDTLV.value.numBytes);

    // Controller Address.
    if (!addressTLV.value.bytes) {
        HAPLog(&logObject, "Controller Address missing.");
        return kHAPError_InvalidData;
    }
    {
        HAPTLVReader subReader;
        HAPTLVReaderCreate(&subReader, (void*) (uintptr_t) addressTLV.value.bytes, addressTLV.value.numBytes);

        err = TryParseControllerAddress(&endpoint.ipAddress, &endpoint.video.port, &endpoint.audio.port, &subReader);
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            return err;
        }
    }

    // SRTP Parameters for Video.
    if (!srtpParametersForVideoTLV.value.bytes) {
        HAPLog(&logObject, "SRTP Parameters for Video missing.");
        return kHAPError_InvalidData;
    }
    {
        HAPTLVReader subReader;
        HAPTLVReaderCreate(
                &subReader,
                (void*) (uintptr_t) srtpParametersForVideoTLV.value.bytes,
                srtpParametersForVideoTLV.value.numBytes);
        err = TryParseSRTPParameters(&endpoint.video.srtpParameters, &subReader);
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            return err;
        }
    }

    // SRTP Parameters for Audio.
    if (!srtpParametersForAudioTLV.value.bytes) {
        HAPLog(&logObject, "SRTP Parameters for Audio missing.");
        return kHAPError_InvalidData;
    }
    {
        HAPTLVReader subReader;
        HAPTLVReaderCreate(
                &subReader,
                (void*) (uintptr_t) srtpParametersForAudioTLV.value.bytes,
                srtpParametersForAudioTLV.value.numBytes);

        err = TryParseSRTPParameters(&endpoint.audio.srtpParameters, &subReader);
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            return err;
        }
    }

    // Log.
    HAPLogInfo(
            &logObject,
            "Setup Endpoints:\n"
            "    Session ID: %02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X\n"
            "    Controller Address: %s: %s\n"
            "    Video:\n"
            "        Port: %u\n"
            "        SRTP crypto suite: %s\n"
            "    Audio:\n"
            "        Port: %u\n"
            "        SRTP crypto suite: %s",
            sessionID.value[0],
            sessionID.value[1],
            sessionID.value[2],
            sessionID.value[3],
            sessionID.value[4],
            sessionID.value[5],
            sessionID.value[6],
            sessionID.value[7],
            sessionID.value[8],
            sessionID.value[9],
            sessionID.value[10],
            sessionID.value[11],
            sessionID.value[12],
            sessionID.value[13],
            sessionID.value[14],
            sessionID.value[15],
            GetIPAddressVersionDescription(endpoint.ipAddress.version),
            endpoint.ipAddress.ipString,
            endpoint.video.port,
            GetSRTPCryptoSuiteDescription(endpoint.video.srtpParameters.cryptoSuite),
            endpoint.audio.port,
            GetSRTPCryptoSuiteDescription(endpoint.audio.srtpParameters.cryptoSuite));
    switch (endpoint.video.srtpParameters.cryptoSuite) {
        case kHAPSRTPCryptoSuite_AES_CM_128_HMAC_SHA1_80: {
            HAPLogSensitiveBufferInfo(
                    &logObject,
                    endpoint.video.srtpParameters._.AES_CM_128_HMAC_SHA1_80.key,
                    sizeof endpoint.video.srtpParameters._.AES_CM_128_HMAC_SHA1_80.key,
                    "Video: SRTP Master Key (Send)");
            HAPLogSensitiveBufferInfo(
                    &logObject,
                    endpoint.video.srtpParameters._.AES_CM_128_HMAC_SHA1_80.salt,
                    sizeof endpoint.video.srtpParameters._.AES_CM_128_HMAC_SHA1_80.salt,
                    "Video: SRTP Master Salt (Send)");
            break;
        }
        case kHAPSRTPCryptoSuite_AES_256_CM_HMAC_SHA1_80: {
            HAPLogSensitiveBufferInfo(
                    &logObject,
                    endpoint.video.srtpParameters._.AES_256_CM_HMAC_SHA1_80.key,
                    sizeof endpoint.video.srtpParameters._.AES_256_CM_HMAC_SHA1_80.key,
                    "Video: SRTP Master Key (Send)");
            HAPLogSensitiveBufferInfo(
                    &logObject,
                    endpoint.video.srtpParameters._.AES_256_CM_HMAC_SHA1_80.salt,
                    sizeof endpoint.video.srtpParameters._.AES_256_CM_HMAC_SHA1_80.salt,
                    "Video: SRTP Master Salt (Send)");
            break;
        }
        case kHAPSRTPCryptoSuite_Disabled: {
            break;
        }
    }
    switch (endpoint.audio.srtpParameters.cryptoSuite) {
        case kHAPSRTPCryptoSuite_AES_CM_128_HMAC_SHA1_80: {
            HAPLogSensitiveBufferInfo(
                    &logObject,
                    endpoint.audio.srtpParameters._.AES_CM_128_HMAC_SHA1_80.key,
                    sizeof endpoint.audio.srtpParameters._.AES_CM_128_HMAC_SHA1_80.key,
                    "Audio: SRTP Master Key (Send)");
            HAPLogSensitiveBufferInfo(
                    &logObject,
                    endpoint.audio.srtpParameters._.AES_CM_128_HMAC_SHA1_80.salt,
                    sizeof endpoint.audio.srtpParameters._.AES_CM_128_HMAC_SHA1_80.salt,
                    "Audio: SRTP Master Salt (Send)");
            break;
        }
        case kHAPSRTPCryptoSuite_AES_256_CM_HMAC_SHA1_80: {
            HAPLogSensitiveBufferInfo(
                    &logObject,
                    endpoint.audio.srtpParameters._.AES_256_CM_HMAC_SHA1_80.key,
                    sizeof endpoint.audio.srtpParameters._.AES_256_CM_HMAC_SHA1_80.key,
                    "Audio: SRTP Master Key (Send)");
            HAPLogSensitiveBufferInfo(
                    &logObject,
                    endpoint.audio.srtpParameters._.AES_256_CM_HMAC_SHA1_80.salt,
                    sizeof endpoint.audio.srtpParameters._.AES_256_CM_HMAC_SHA1_80.salt,
                    "Audio: SRTP Master Salt (Send)");
            break;
        }
        case kHAPSRTPCryptoSuite_Disabled: {
            break;
        }
    }

    const HAPCameraStreamSupportedConfigurations* streamConfiguration =
            request->accessory->cameraStreamConfigurations[streamIndex];

    // Verify crypto support.
    if (!(streamConfiguration->rtp.srtpCryptoSuites & endpoint.video.srtpParameters.cryptoSuite)) {
        HAPLog(&logObject, "Requested video crypto suite unsupported.");
        return kHAPError_InvalidData;
    }
    if (!(streamConfiguration->rtp.srtpCryptoSuites & endpoint.audio.srtpParameters.cryptoSuite)) {
        HAPLog(&logObject, "Requested audio crypto suite unsupported.");
        return kHAPError_InvalidData;
    }

    // Initialize setup.
    HAPCameraStreamingSessionSetup* setup =
            GetCameraStreamingSetup(server, request->session, request->service, request->accessory);
    if (setup->isActive) {
        HAPLog(&logObject, "Cancelling prepared streaming session setup (multiple writes without read).");
    }

    HAPRawBufferZero(setup, sizeof *setup);
    HAPAssert(sizeof setup->sessionID == sizeof sessionID);
    HAPRawBufferCopyBytes(&setup->sessionID, &sessionID, sizeof sessionID);
    HAPAssert(sizeof setup->controllerEndpoint == sizeof endpoint);
    HAPRawBufferCopyBytes(&setup->controllerEndpoint, &endpoint, sizeof endpoint);
    setup->isActive = true;

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleCameraRTPStreamManagementSelectedRTPStreamConfigurationRead(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter HAP_UNUSED,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->session);
    HAPPrecondition(request->service);
    HAPPrecondition(request->accessory);

    HAPError err;

    size_t streamIndex = GetStreamIndex(request->service, request->accessory);
    HAPPlatformCameraRef camera = HAPAccessoryGetCamera(server, request->accessory);

    err = HAPCameraCheckStreamingEnabled(camera, streamIndex);
    if (err) {
        HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidState);
        return err;
    }

    return kHAPError_None;
}

/**
 * Parses selected video attributes.
 *
 * @param[out] videoAttributes      Parsed video attributes.
 * @param      requestReader        TLV reader.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the request reader contains invalid data.
 */
HAP_RESULT_USE_CHECK
static HAPError TryParseVideoAttributes(HAPVideoAttributes* videoAttributes, HAPTLVReader* requestReader) {
    HAPPrecondition(videoAttributes);
    HAPPrecondition(requestReader);

    HAPError err;

    HAPRawBufferZero(videoAttributes, sizeof *videoAttributes);

    HAPTLV imageWidthTLV, imageHeightTLV, frameRateTLV;
    imageWidthTLV.type = kHAPCharacteristicValue_VideoCodecAttribute_ImageWidth;
    imageHeightTLV.type = kHAPCharacteristicValue_VideoCodecAttribute_ImageHeight;
    frameRateTLV.type = kHAPCharacteristicValue_VideoCodecAttribute_FrameRate;
    err = HAPTLVReaderGetAll(requestReader, (HAPTLV* const[]) { &imageWidthTLV, &imageHeightTLV, &frameRateTLV, NULL });
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }

    // Image width.
    if (!imageWidthTLV.value.bytes) {
        HAPLog(&logObject, "Image width missing.");
        return kHAPError_InvalidData;
    }
    if (imageWidthTLV.value.numBytes != 2) {
        HAPLog(&logObject, "Image width has invalid length (%lu).", (unsigned long) imageWidthTLV.value.numBytes);
        return kHAPError_InvalidData;
    }
    videoAttributes->width = HAPReadLittleUInt16(imageWidthTLV.value.bytes);

    // Image height.
    if (!imageHeightTLV.value.bytes) {
        HAPLog(&logObject, "Image height missing.");
        return kHAPError_InvalidData;
    }
    if (imageHeightTLV.value.numBytes != 2) {
        HAPLog(&logObject, "Image height has invalid length (%lu).", (unsigned long) imageHeightTLV.value.numBytes);
        return kHAPError_InvalidData;
    }
    videoAttributes->height = HAPReadLittleUInt16(imageHeightTLV.value.bytes);

    // Frame rate.
    if (!frameRateTLV.value.bytes) {
        HAPLog(&logObject, "Frame rate missing.");
        return kHAPError_InvalidData;
    }
    if (frameRateTLV.value.numBytes != 1) {
        HAPLog(&logObject, "Frame rate has invalid length (%lu).", (unsigned long) frameRateTLV.value.numBytes);
        return kHAPError_InvalidData;
    }
    videoAttributes->maxFrameRate = ((const uint8_t*) frameRateTLV.value.bytes)[0];
    if (videoAttributes->maxFrameRate == 0) {
        HAPLog(&logObject, "Frame rate is zero.");
        return kHAPError_InvalidData;
    }

    return kHAPError_None;
}

/**
 * Parses selected video RTP parameters of a Start streaming session command.
 *
 * @param[out] rtpParameters        Parsed RTP parameters.
 * @param      requestReader        TLV reader.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the request reader contains invalid data.
 */
HAP_RESULT_USE_CHECK
static HAPError TryParseStartStreamingSessionVideoRTPParameters(
        HAPPlatformCameraRTPParameters* rtpParameters,
        HAPTLVReader* requestReader) {
    HAPPrecondition(rtpParameters);
    HAPPrecondition(requestReader);

    HAPError err;

    HAPRawBufferZero(rtpParameters, sizeof *rtpParameters);

    HAPTLV payloadTypeTLV, ssrcTLV, maxBitrateTLV, minRTCPIntervalTLV, maxMTUTLV;
    payloadTypeTLV.type = kHAPCharacteristicValue_SelectedRTPStreamConfiguration_RTPParameters_PayloadType;
    ssrcTLV.type = kHAPCharacteristicValue_SelectedRTPStreamConfiguration_RTPParameters_SynchronizationSource;
    maxBitrateTLV.type = kHAPCharacteristicValue_SelectedRTPStreamConfiguration_RTPParameters_MaxBitrate;
    minRTCPIntervalTLV.type = kHAPCharacteristicValue_SelectedRTPStreamConfiguration_RTPParameters_MinRTCPInterval;
    maxMTUTLV.type = kHAPCharacteristicValue_SelectedRTPStreamConfiguration_RTPParameters_MaxMTU;
    err = HAPTLVReaderGetAll(
            requestReader,
            (HAPTLV* const[]) { &payloadTypeTLV, &ssrcTLV, &maxBitrateTLV, &minRTCPIntervalTLV, &maxMTUTLV, NULL });
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }

    // Payload type.
    if (!payloadTypeTLV.value.bytes) {
        HAPLog(&logObject, "Start: Video payload type missing.");
        return kHAPError_InvalidData;
    }
    if (payloadTypeTLV.value.numBytes != 1) {
        HAPLog(&logObject,
               "Start: Video payload type has invalid length (%lu).",
               (unsigned long) payloadTypeTLV.value.numBytes);
        return kHAPError_InvalidData;
    }
    rtpParameters->type = ((const uint8_t*) payloadTypeTLV.value.bytes)[0];

    // SynchronizationSource for Video.
    if (!ssrcTLV.value.bytes) {
        HAPLog(&logObject, "Start: Video SSRC missing.");
        return kHAPError_InvalidData;
    }
    if (ssrcTLV.value.numBytes != 4) {
        HAPLog(&logObject, "Start: Video SSRC has invalid length (%lu).", (unsigned long) ssrcTLV.value.numBytes);
        return kHAPError_InvalidData;
    }
    rtpParameters->ssrc = HAPReadLittleUInt32(ssrcTLV.value.bytes);

    // Maximum Bitrate.
    if (!maxBitrateTLV.value.bytes) {
        HAPLog(&logObject, "Start: Video maximum bitrate missing.");
        return kHAPError_InvalidData;
    }
    if (maxBitrateTLV.value.numBytes != 2) {
        HAPLog(&logObject,
               "Start: Video maximum bitrate has invalid length (%lu).",
               (unsigned long) maxBitrateTLV.value.numBytes);
        return kHAPError_InvalidData;
    }
    rtpParameters->maximumBitRate = HAPReadLittleUInt16(maxBitrateTLV.value.bytes);

    // Min RTCP Interval.
    if (!minRTCPIntervalTLV.value.bytes) {
        HAPLog(&logObject, "Start: Video min RTCP interval missing.");
        return kHAPError_InvalidData;
    }
    if (minRTCPIntervalTLV.value.numBytes != 4) {
        HAPLog(&logObject,
               "Start: Video min RTCP interval has invalid length (%lu).",
               (unsigned long) minRTCPIntervalTLV.value.numBytes);
        return kHAPError_InvalidData;
    }
    uint32_t bitPattern = HAPReadLittleUInt32(minRTCPIntervalTLV.value.bytes);
    rtpParameters->minimumRTCPInterval = HAPFloatFromBitPattern(bitPattern);
    if (!HAPFloatIsFinite(rtpParameters->minimumRTCPInterval) || rtpParameters->minimumRTCPInterval < 0) {
        HAPLog(&logObject, "Start: Video min RTCP interval invalid: %g.", (double) rtpParameters->minimumRTCPInterval);
        return kHAPError_InvalidData;
    }

    // Max MTU.
    if (maxMTUTLV.value.bytes) {
        if (maxMTUTLV.value.numBytes != 2) {
            HAPLog(&logObject,
                   "Start: Video max MTU has invalid length (%lu).",
                   (unsigned long) maxMTUTLV.value.numBytes);
            return kHAPError_InvalidData;
        }
        rtpParameters->maximumMTU = HAPReadLittleUInt16(maxMTUTLV.value.bytes);
    } else {
        rtpParameters->maximumMTU = 0;
    }

    return kHAPError_None;
}

/**
 * Parses video codec type.
 *
 * Converts from Characteristic to ADK representation of value.
 *
 * @param[out] videoCodecType       Parsed value.
 * @param      rawValue             Raw value.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the value is invalid.
 */
HAP_RESULT_USE_CHECK
static HAPError TryParseVideoCodecType(HAPVideoCodecType* videoCodecType, uint8_t rawValue) {
    HAPPrecondition(videoCodecType);

    HAPAssert(sizeof rawValue == sizeof(HAPCharacteristicValue_CameraStream_VideoCodecType));
    switch ((HAPCharacteristicValue_CameraStream_VideoCodecType) rawValue) {
        case kHAPCharacteristicValue_CameraStream_VideoCodecType_H264: {
            *videoCodecType = kHAPVideoCodecType_H264;
        }
            return kHAPError_None;
        default: {
            HAPLog(&logObject, "Video codec type invalid: %u.", rawValue);
        }
            return kHAPError_InvalidData;
    }
}

/**
 * Parses selected video parameters of a Start streaming session command.
 *
 * @param[out] videoCodecType       Parsed codec type.
 * @param[out] codecParameters      Parsed codec parameters.
 * @param[out] videoAttributes      Parsed video attributes.
 * @param[out] rtpParameters        Parsed RTP parameters.
 * @param      requestReader        TLV reader.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the requestReader contains invalid data.
 */
HAP_RESULT_USE_CHECK
static HAPError TryParseStartStreamingSessionVideoParameters(
        HAPVideoCodecType* videoCodecType,
        HAPH264VideoCodecParameters* codecParameters,
        HAPVideoAttributes* videoAttributes,
        HAPPlatformCameraRTPParameters* rtpParameters,
        HAPTLVReader* requestReader) {
    HAPPrecondition(videoCodecType);
    HAPPrecondition(codecParameters);
    HAPPrecondition(videoAttributes);
    HAPPrecondition(rtpParameters);
    HAPPrecondition(requestReader);

    HAPError err;

    HAPRawBufferZero(videoCodecType, sizeof *videoCodecType);
    HAPRawBufferZero(codecParameters, sizeof *codecParameters);
    HAPRawBufferZero(videoAttributes, sizeof *videoAttributes);
    HAPRawBufferZero(rtpParameters, sizeof *rtpParameters);

    HAPTLV codecTypeTLV, codecParametersTLV, attributesTLV, rtpParametersTLV;
    codecTypeTLV.type = kHAPCharacteristicValue_SelectedRTPStreamConfiguration_Video_CodecType;
    codecParametersTLV.type = kHAPCharacteristicValue_SelectedRTPStreamConfiguration_Video_CodecParameters;
    attributesTLV.type = kHAPCharacteristicValue_SelectedRTPStreamConfiguration_Video_Attributes;
    rtpParametersTLV.type = kHAPCharacteristicValue_SelectedRTPStreamConfiguration_Video_RTPParameters;
    err = HAPTLVReaderGetAll(
            requestReader,
            (HAPTLV* const[]) { &codecTypeTLV, &codecParametersTLV, &attributesTLV, &rtpParametersTLV, NULL });
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }

    // Selected Video Codec type.
    if (!codecTypeTLV.value.bytes) {
        HAPLog(&logObject, "Start: Selected Video Codec type missing.");
        return kHAPError_InvalidData;
    }
    if (codecTypeTLV.value.numBytes != 1) {
        HAPLog(&logObject,
               "Start: Selected Video Codec type has invalid length (%lu).",
               (unsigned long) codecTypeTLV.value.numBytes);
        return kHAPError_InvalidData;
    }
    err = TryParseVideoCodecType(videoCodecType, ((const uint8_t*) codecTypeTLV.value.bytes)[0]);
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }
    HAPAssert(HAPVideoCodecTypeIsSupportedForStreaming(*videoCodecType));

    // Selected Video RTP parameters.
    // Parse this before Video Codec parameters so that bit rate is known.
    if (!rtpParametersTLV.value.bytes) {
        HAPLog(&logObject, "Start: Selected Video RTP parameters missing.");
        return kHAPError_InvalidData;
    }
    {
        HAPTLVReader subReader;
        HAPTLVReaderCreate(
                &subReader, (void*) (uintptr_t) rtpParametersTLV.value.bytes, rtpParametersTLV.value.numBytes);

        err = TryParseStartStreamingSessionVideoRTPParameters(rtpParameters, &subReader);
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            return err;
        }
    }

    // Selected Video Codec parameters.
    if (!codecParametersTLV.value.bytes) {
        HAPLog(&logObject, "Start: Selected Video Codec parameters missing.");
        return kHAPError_InvalidData;
    }
    {
        HAPTLVReader subReader;
        HAPTLVReaderCreate(
                &subReader, (void*) (uintptr_t) codecParametersTLV.value.bytes, codecParametersTLV.value.numBytes);
        switch (*videoCodecType) {
            case kHAPVideoCodecType_H264: {
                HAPTLV profileIDTLV, levelTLV, packetizationModeTLV;
                profileIDTLV.type = kHAPCharacteristicValue_CameraStream_H264VideoCodecParameter_Profile;
                levelTLV.type = kHAPCharacteristicValue_CameraStream_H264VideoCodecParameter_Level;
                packetizationModeTLV.type =
                        kHAPCharacteristicValue_CameraStream_H264VideoCodecParameter_PacketizationMode;
                err = HAPTLVReaderGetAll(
                        &subReader, (HAPTLV* const[]) { &profileIDTLV, &levelTLV, &packetizationModeTLV, NULL });
                if (err) {
                    HAPAssert(err == kHAPError_InvalidData);
                    return err;
                }

                // ProfileID.
                if (!profileIDTLV.value.bytes) {
                    HAPLog(&logObject, "Start: H.264 ProfileID missing.");
                    return kHAPError_InvalidData;
                }
                if (profileIDTLV.value.numBytes != 1) {
                    HAPLog(&logObject,
                           "Start: H.264 ProfileID has invalid length (%lu).",
                           (unsigned long) profileIDTLV.value.numBytes);
                    return kHAPError_InvalidData;
                }
                err = HAPVideoTryParseH264ProfileID(
                        &codecParameters->profile, ((const uint8_t*) profileIDTLV.value.bytes)[0]);
                if (err) {
                    HAPAssert(err == kHAPError_InvalidData);
                    return err;
                }

                // Level.
                if (!levelTLV.value.bytes) {
                    HAPLog(&logObject, "Start: H.264 Level missing.");
                    return kHAPError_InvalidData;
                }
                if (levelTLV.value.numBytes != 1) {
                    HAPLog(&logObject,
                           "Start: H.264 Level has invalid length (%lu).",
                           (unsigned long) levelTLV.value.numBytes);
                    return kHAPError_InvalidData;
                }
                err = HAPVideoTryParseH264ProfileLevel(
                        &codecParameters->level, ((const uint8_t*) levelTLV.value.bytes)[0]);
                if (err) {
                    HAPAssert(err == kHAPError_InvalidData);
                    return err;
                }

                // Packetization mode.
                if (!packetizationModeTLV.value.bytes) {
                    HAPLog(&logObject, "Start: Packetization mode missing.");
                    return kHAPError_InvalidData;
                }
                if (packetizationModeTLV.value.numBytes != 1) {
                    HAPLog(&logObject,
                           "Start: Packetization mode has invalid length (%lu).",
                           (unsigned long) packetizationModeTLV.value.numBytes);
                    return kHAPError_InvalidData;
                }
                err = HAPVideoTryParseH264PacketizationMode(
                        &codecParameters->packetizationMode, ((const uint8_t*) packetizationModeTLV.value.bytes)[0]);
                if (err) {
                    HAPAssert(err == kHAPError_InvalidData);
                    return err;
                }

                // Bit rate.
                codecParameters->bitRate = rtpParameters->maximumBitRate;

                // I-Frame interval.
                // The minimum keyframe interval shall be 5 seconds.
                // See HomeKit Accessory Protocol Specification R17
                // Section 12.4.4.1 Mandatory Video RTP Service Settings
                codecParameters->iFrameInterval = 5000;

                break;
            }
        }
    }

    // Selected Video attributes.
    if (!attributesTLV.value.bytes) {
        HAPLog(&logObject, "Start: Selected Video attributes missing.");
        return kHAPError_InvalidData;
    }
    {
        HAPTLVReader subReader;
        HAPTLVReaderCreate(&subReader, (void*) (uintptr_t) attributesTLV.value.bytes, attributesTLV.value.numBytes);

        err = TryParseVideoAttributes(videoAttributes, &subReader);
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            return err;
        }
    }

    return kHAPError_None;
}

/**
 * Parses audio codec type.
 *
 * @param[out] audioCodecType       Parsed value.
 * @param      rawValue             Raw value.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the value is invalid.
 */
HAP_RESULT_USE_CHECK
static HAPError TryParseAudioCodecType(HAPAudioCodecType* audioCodecType, uint8_t rawValue) {
    HAPPrecondition(audioCodecType);

    HAPAssert(sizeof rawValue == sizeof(HAPCharacteristicValue_AudioCodecType));
    HAP_DIAGNOSTIC_PUSH
    HAP_DIAGNOSTIC_IGNORED_CLANG("-Wdeprecated-declarations")
    HAP_DIAGNOSTIC_IGNORED_GCC("-Wdeprecated-declarations")
    HAP_DIAGNOSTIC_IGNORED_ARMCC(2570)
    HAP_DIAGNOSTIC_IGNORED_ICCARM(Pe1444)
    switch ((HAPCharacteristicValue_AudioCodecType) rawValue) {
        case kHAPCharacteristicValue_AudioCodecType_AAC_ELD: {
            *audioCodecType = kHAPAudioCodecType_AAC_ELD;
            return kHAPError_None;
        }
        case kHAPCharacteristicValue_AudioCodecType_Opus: {
            *audioCodecType = kHAPAudioCodecType_Opus;
            return kHAPError_None;
        }
        case kHAPCharacteristicValue_AudioCodecType_AMR: {
            *audioCodecType = kHAPAudioCodecType_AMR;
            return kHAPError_None;
        }
        case kHAPCharacteristicValue_AudioCodecType_AMR_WB: {
            *audioCodecType = kHAPAudioCodecType_AMR_WB;
            return kHAPError_None;
        }
        default: {
            HAPLog(&logObject, "Audio codec type invalid: %u.", rawValue);
            return kHAPError_InvalidData;
        }
    }
    HAP_DIAGNOSTIC_RESTORE_ICCARM(Pe1444)
    HAP_DIAGNOSTIC_POP
}

/**
 * Parses audio bit-rate.
 *
 * @param[out] audioBitRate         Parsed value.
 * @param      rawValue             Raw value.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the value is invalid.
 */
HAP_RESULT_USE_CHECK
static HAPError TryParseAudioBitRate(HAPAudioCodecBitRateControlMode* audioBitRate, uint8_t rawValue) {
    HAPPrecondition(audioBitRate);

    HAPAssert(sizeof rawValue == sizeof(HAPCharacteristicValue_AudioCodecBitRateControlMode));
    switch ((HAPCharacteristicValue_AudioCodecBitRateControlMode) rawValue) {
        case kHAPCharacteristicValue_AudioCodecBitRateControlMode_Variable: {
            *audioBitRate = kHAPAudioCodecBitRateControlMode_Variable;
        }
            return kHAPError_None;
        case kHAPCharacteristicValue_AudioCodecBitRateControlMode_Constant: {
            *audioBitRate = kHAPAudioCodecBitRateControlMode_Constant;
        }
            return kHAPError_None;
        default: {
            HAPLog(&logObject, "Audio bit-rate invalid: %u.", rawValue);
        }
            return kHAPError_InvalidData;
    }
}

/**
 * Parses audio sample rate.
 *
 * @param[out] audioSampleRate      Parsed value.
 * @param      rawValue             Raw value.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the value is invalid.
 */
HAP_RESULT_USE_CHECK
static HAPError TryParseAudioSampleRate(HAPAudioCodecSampleRate* audioSampleRate, uint8_t rawValue) {
    HAPPrecondition(audioSampleRate);

    HAPAssert(sizeof rawValue == sizeof(HAPCharacteristicValue_AudioCodecSampleRate));
    switch ((HAPCharacteristicValue_AudioCodecSampleRate) rawValue) {
        case kHAPCharacteristicValue_AudioCodecSampleRate_8KHz: {
            *audioSampleRate = kHAPAudioCodecSampleRate_8KHz;
        }
            return kHAPError_None;
        case kHAPCharacteristicValue_AudioCodecSampleRate_16KHz: {
            *audioSampleRate = kHAPAudioCodecSampleRate_16KHz;
        }
            return kHAPError_None;
        case kHAPCharacteristicValue_AudioCodecSampleRate_24KHz: {
            *audioSampleRate = kHAPAudioCodecSampleRate_24KHz;
        }
            return kHAPError_None;
        default: {
            HAPLog(&logObject, "Audio sample rate invalid: %u.", rawValue);
        }
            return kHAPError_InvalidData;
    }
}

/**
 * Returns whether an RTP time is supported by a given audio codec and audio sample rate.
 *
 * @param      audioCodecType       Audio codec type.
 * @param      audioSampleRate      Audio sample rate.
 * @param      rtpTime              RTP time in ms.
 *
 * @return true                     If the given RTP time is supported by the audio codec and audio sample rate.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
static bool IsRTPTimeSupportedByAudioCodec(
        HAPAudioCodecType audioCodecType,
        HAPAudioCodecSampleRate audioSampleRate,
        uint8_t rtpTime) {
    HAPPrecondition(HAPAudioCodecTypeIsSupportedForStreaming(audioCodecType));
    HAPPrecondition(AudioSampleRateIsSupportedForStreaming(audioSampleRate));

    // General HAP limitations: Only 20, 30, 40, 60 ms.
    switch (audioCodecType) {
        case kHAPAudioCodecType_AAC_ELD: {
            // Multiple of 480 samples.
            switch (audioSampleRate) {
                case kHAPAudioCodecSampleRate_8KHz: {
                    return rtpTime == 60;
                }
                case kHAPAudioCodecSampleRate_16KHz: {
                    return rtpTime == 30 || rtpTime == 60;
                }
                case kHAPAudioCodecSampleRate_24KHz: {
                    return rtpTime == 20 || rtpTime == 40 || rtpTime == 60;
                }
                case kHAPAudioCodecSampleRate_32KHz:
                case kHAPAudioCodecSampleRate_44_1KHz:
                case kHAPAudioCodecSampleRate_48KHz:
                default:
                    HAPFatalError();
            }
        }
        case kHAPAudioCodecType_Opus: { // NOLINT(bugprone-branch-clone)
            // 2.5, 5, 10, 20, 40, 60 ms.
            return rtpTime == 20 || rtpTime == 40 || rtpTime == 60;
        }
        case kHAPAudioCodecType_AMR: {
            // Multiple of 20 ms.
            return rtpTime == 20 || rtpTime == 40 || rtpTime == 60;
        }
        case kHAPAudioCodecType_AMR_WB: {
            // Multiple of 20 ms.
            return rtpTime == 20 || rtpTime == 40 || rtpTime == 60;
        }
        case kHAPAudioCodecType_PCMU:
        case kHAPAudioCodecType_PCMA:
        case kHAPAudioCodecType_AAC_LC:
        case kHAPAudioCodecType_MSBC: {
            HAPFatalError();
        }
        default:
            HAPFatalError();
    }
}

/**
 * Parses selected audio parameters of a Start streaming session command.
 *
 * @param[out] audioCodecType       Parsed codec type.
 * @param[out] codecParameters      Parsed codec parameters.
 * @param[out] rtpTime              Parsed RTP time.
 * @param[out] rtpParameters        Parsed RTP parameters.
 * @param      requestReader        TLV reader.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the request reader contains invalid data.
 */
HAP_RESULT_USE_CHECK
static HAPError TryParseStartStreamingSessionAudioParameters(
        HAPAudioCodecType* audioCodecType,
        HAPAudioCodecParameters* codecParameters,
        uint8_t* rtpTime,
        HAPPlatformCameraRTPParameters* rtpParameters,
        HAPPlatformCameraComfortNoise* comfortNoise,
        HAPTLVReader* requestReader) {
    HAPPrecondition(audioCodecType);
    HAPPrecondition(codecParameters);
    HAPPrecondition(rtpTime);
    HAPPrecondition(rtpParameters);
    HAPPrecondition(requestReader);

    HAPError err;

    HAPRawBufferZero(audioCodecType, sizeof *audioCodecType);
    HAPRawBufferZero(codecParameters, sizeof *codecParameters);
    HAPRawBufferZero(rtpParameters, sizeof *rtpParameters);

    HAPTLV codecTypeTLV, codecParametersTLV, rtpParametersTLV, comfortNoiseTLV;
    codecTypeTLV.type = kHAPCharacteristicValue_SelectedRTPStreamConfiguration_Audio_CodecType;
    codecParametersTLV.type = kHAPCharacteristicValue_SelectedRTPStreamConfiguration_Audio_CodecParameters;
    rtpParametersTLV.type = kHAPCharacteristicValue_SelectedRTPStreamConfiguration_Audio_RTPParameters;
    comfortNoiseTLV.type = kHAPCharacteristicValue_SelectedRTPStreamConfiguration_Audio_ComfortNoise;
    err = HAPTLVReaderGetAll(
            requestReader,
            (HAPTLV* const[]) { &codecTypeTLV, &codecParametersTLV, &rtpParametersTLV, &comfortNoiseTLV, NULL });
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }

    // Selected Audio Codec type.
    if (!codecTypeTLV.value.bytes) {
        HAPLog(&logObject, "Start: Selected Audio Codec type missing.");
        return kHAPError_InvalidData;
    }
    if (codecTypeTLV.value.numBytes != 1) {
        HAPLog(&logObject,
               "Start: Selected Audio Codec type has invalid length (%lu).",
               (unsigned long) codecTypeTLV.value.numBytes);
        return kHAPError_InvalidData;
    }
    err = TryParseAudioCodecType(audioCodecType, ((const uint8_t*) codecTypeTLV.value.bytes)[0]);
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }
    HAPAssert(HAPAudioCodecTypeIsSupportedForStreaming(*audioCodecType));

    // Selected Audio Codec parameters.
    if (!codecParametersTLV.value.bytes) {
        HAPLog(&logObject, "Start: Selected Audio Codec parameters missing.");
        return kHAPError_InvalidData;
    }
    {
        HAPTLVReader subReader;
        HAPTLVReaderCreate(
                &subReader, (void*) (uintptr_t) codecParametersTLV.value.bytes, codecParametersTLV.value.numBytes);

        HAPTLV audioChannelsTLV, bitRateTLV, sampleRateTLV, rtpTimeTLV;
        audioChannelsTLV.type = kHAPCharacteristicValue_AudioCodecStreamParameter_AudioChannels;
        bitRateTLV.type = kHAPCharacteristicValue_AudioCodecStreamParameter_BitRate;
        sampleRateTLV.type = kHAPCharacteristicValue_AudioCodecStreamParameter_SampleRate;
        rtpTimeTLV.type = kHAPCharacteristicValue_AudioCodecStreamParameter_RTPTime;
        err = HAPTLVReaderGetAll(
                &subReader, (HAPTLV* const[]) { &audioChannelsTLV, &bitRateTLV, &sampleRateTLV, &rtpTimeTLV, NULL });
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            return err;
        }

        // Audio channels.
        if (!audioChannelsTLV.value.bytes) {
            HAPLog(&logObject, "Start: Audio channels missing.");
            return kHAPError_InvalidData;
        }
        if (audioChannelsTLV.value.numBytes != 1) {
            HAPLog(&logObject,
                   "Start: Audio channels has invalid length (%lu).",
                   (unsigned long) audioChannelsTLV.value.numBytes);
            return kHAPError_InvalidData;
        }
        codecParameters->numberOfChannels = ((const uint8_t*) audioChannelsTLV.value.bytes)[0];

        // Bit rate.
        if (!bitRateTLV.value.bytes) {
            HAPLog(&logObject, "Start: Audio bit rate missing.");
            return kHAPError_InvalidData;
        }
        if (bitRateTLV.value.numBytes != 1) {
            HAPLog(&logObject,
                   "Start: Audio bit rate has invalid length (%lu).",
                   (unsigned long) bitRateTLV.value.numBytes);
            return kHAPError_InvalidData;
        }
        err = TryParseAudioBitRate(&codecParameters->bitRateMode, ((const uint8_t*) bitRateTLV.value.bytes)[0]);
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            return err;
        }

        // Sample rate.
        if (!sampleRateTLV.value.bytes) {
            HAPLog(&logObject, "Start: Audio sample rate missing.");
            return kHAPError_InvalidData;
        }
        if (sampleRateTLV.value.numBytes != 1) {
            HAPLog(&logObject,
                   "Start: Audio sample rate has invalid length (%lu).",
                   (unsigned long) sampleRateTLV.value.numBytes);
            return kHAPError_InvalidData;
        }
        err = TryParseAudioSampleRate(&codecParameters->sampleRate, ((const uint8_t*) sampleRateTLV.value.bytes)[0]);
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            return err;
        }
        HAPAssert(AudioSampleRateIsSupportedForStreaming(codecParameters->sampleRate));

        // RTP time.
        if (!rtpTimeTLV.value.bytes) {
            HAPLog(&logObject, "Start: Audio RTP time missing.");
            return kHAPError_InvalidData;
        }
        if (rtpTimeTLV.value.numBytes != 1) {
            HAPLog(&logObject,
                   "Start: Audio RTP time has invalid length (%lu).",
                   (unsigned long) rtpTimeTLV.value.numBytes);
            return kHAPError_InvalidData;
        }
        *rtpTime = ((const uint8_t*) rtpTimeTLV.value.bytes)[0];
        if (*rtpTime != 20 && *rtpTime != 30 && *rtpTime != 40 && *rtpTime != 60) {
            HAPLog(&logObject, "Start: Audio RTP time invalid: %u.", *rtpTime);
            return kHAPError_InvalidData;
        }
        if (!IsRTPTimeSupportedByAudioCodec(*audioCodecType, codecParameters->sampleRate, *rtpTime)) {
            HAPLog(&logObject, "Start: Audio RTP time is not supported by codec: %u.", *rtpTime);
            if (*audioCodecType == kHAPAudioCodecType_AAC_ELD) {
                // Workaround for wrong AAC RTP time.
                if (codecParameters->sampleRate == kHAPAudioCodecSampleRate_8KHz) {
                    *rtpTime = 60;
                } else if (codecParameters->sampleRate == kHAPAudioCodecSampleRate_16KHz) {
                    *rtpTime = 30;
                } else {
                    *rtpTime = 20;
                }
            } else {
                return kHAPError_InvalidData;
            }
        }
    }

    // Comfort Noise.
    if (!comfortNoiseTLV.value.bytes) {
        HAPLog(&logObject, "Start: Audio Comfort Noise missing.");
        return kHAPError_InvalidData;
    }
    if (comfortNoiseTLV.value.numBytes != 1) {
        HAPLog(&logObject,
               "Start: Audio Comfort Noise has invalid length (%lu).",
               (unsigned long) comfortNoiseTLV.value.numBytes);
        return kHAPError_InvalidData;
    }
    comfortNoise->enabled = ((const uint8_t*) comfortNoiseTLV.value.bytes)[0];
    if (comfortNoise->enabled != 0 && comfortNoise->enabled != 1) {
        HAPLog(&logObject, "Start: Audio Comfort Noise invalid: %u.", comfortNoise->enabled);
    }

    // Selected Audio RTP parameters.
    if (!rtpParametersTLV.value.bytes) {
        HAPLog(&logObject, "Selected Audio RTP parameters missing.");
        return kHAPError_InvalidData;
    }
    {
        HAPTLVReader subReader;
        HAPTLVReaderCreate(
                &subReader, (void*) (uintptr_t) rtpParametersTLV.value.bytes, rtpParametersTLV.value.numBytes);

        HAPTLV payloadTypeTLV, ssrcTLV, maxBitrateTLV, minRTCPIntervalTLV, cnPayloadTypeTLV;
        payloadTypeTLV.type = kHAPCharacteristicValue_SelectedRTPStreamConfiguration_RTPParameters_PayloadType;
        ssrcTLV.type = kHAPCharacteristicValue_SelectedRTPStreamConfiguration_RTPParameters_SynchronizationSource;
        maxBitrateTLV.type = kHAPCharacteristicValue_SelectedRTPStreamConfiguration_RTPParameters_MaxBitrate;
        minRTCPIntervalTLV.type = kHAPCharacteristicValue_SelectedRTPStreamConfiguration_RTPParameters_MinRTCPInterval;
        cnPayloadTypeTLV.type =
                kHAPCharacteristicValue_SelectedRTPStreamConfiguration_RTPParameters_ComfortNoisePayloadType;
        err = HAPTLVReaderGetAll(
                &subReader,
                (HAPTLV* const[]) {
                        &payloadTypeTLV, &ssrcTLV, &maxBitrateTLV, &minRTCPIntervalTLV, &cnPayloadTypeTLV, NULL });
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            return err;
        }

        // Payload type.
        if (!payloadTypeTLV.value.bytes) {
            HAPLog(&logObject, "Start: Audio payload type missing.");
            return kHAPError_InvalidData;
        }
        if (payloadTypeTLV.value.numBytes != 1) {
            HAPLog(&logObject,
                   "Start: Audio payload type has invalid length (%lu).",
                   (unsigned long) payloadTypeTLV.value.numBytes);
            return kHAPError_InvalidData;
        }
        rtpParameters->type = ((const uint8_t*) payloadTypeTLV.value.bytes)[0];

        // SynchronizationSource for Audio.
        if (!ssrcTLV.value.bytes) {
            HAPLog(&logObject, "Start: Audio SSRC missing.");
            return kHAPError_InvalidData;
        }
        if (ssrcTLV.value.numBytes != 4) {
            HAPLog(&logObject, "Start: Audio SSRC has invalid length (%lu).", (unsigned long) ssrcTLV.value.numBytes);
            return kHAPError_InvalidData;
        }
        rtpParameters->ssrc = HAPReadLittleUInt32(ssrcTLV.value.bytes);

        // Maximum Bitrate.
        if (!maxBitrateTLV.value.bytes) {
            HAPLog(&logObject, "Start: Audio maximum bitrate missing.");
            return kHAPError_InvalidData;
        }
        if (maxBitrateTLV.value.numBytes != 2) {
            HAPLog(&logObject,
                   "Start: Audio maximum bitrate has invalid length (%lu).",
                   (unsigned long) maxBitrateTLV.value.numBytes);
            return kHAPError_InvalidData;
        }
        rtpParameters->maximumBitRate = HAPReadLittleUInt16(maxBitrateTLV.value.bytes);

        // Bit rate.
        codecParameters->bitRate = rtpParameters->maximumBitRate;

        // Min RTCP Interval.
        if (!minRTCPIntervalTLV.value.bytes) {
            HAPLog(&logObject, "Start: Audio min RTCP interval missing.");
            return kHAPError_InvalidData;
        }
        if (minRTCPIntervalTLV.value.numBytes != 4) {
            HAPLog(&logObject,
                   "Start: Audio min RTCP interval has invalid length (%lu).",
                   (unsigned long) minRTCPIntervalTLV.value.numBytes);
            return kHAPError_InvalidData;
        }
        uint32_t bitPattern = HAPReadLittleUInt32(minRTCPIntervalTLV.value.bytes);
        rtpParameters->minimumRTCPInterval = HAPFloatFromBitPattern(bitPattern);
        if (!HAPFloatIsFinite(rtpParameters->minimumRTCPInterval) || rtpParameters->minimumRTCPInterval < 0) {
            HAPLog(&logObject,
                   "Start: Audio min RTCP interval invalid: %g.",
                   (double) rtpParameters->minimumRTCPInterval);
            return kHAPError_InvalidData;
        }

        // Max MTU.
        rtpParameters->maximumMTU = 0; // default

        // Comfort Noise Payload Type.
        if (comfortNoise->enabled) {
            if (!cnPayloadTypeTLV.value.bytes) {
                HAPLog(&logObject, "Start: Audio Comfort Noise Payload Type missing.");
                return kHAPError_InvalidData;
            }
            if (cnPayloadTypeTLV.value.numBytes != 1) {
                HAPLog(&logObject,
                       "Start: Audio Comfort Noise Payload Type has invalid length (%lu).",
                       (unsigned long) cnPayloadTypeTLV.value.numBytes);
                return kHAPError_InvalidData;
            }
            comfortNoise->type = ((const uint8_t*) cnPayloadTypeTLV.value.bytes)[0];
        }
    }

    return kHAPError_None;
}

/**
 * Parses selected video RTP parameters of a Reconfigure streaming sessions command.
 *
 * @param[out] rtpParameters        Parsed RTP parameters.
 * @param      requestReader        TLV reader.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the request reader contains invalid data.
 */
HAP_RESULT_USE_CHECK
static HAPError TryParseReconfigureStreamingSessionsVideoRTPParameters(
        HAPPlatformCameraReconfigureStreamingSessionVideoRTPParameters* rtpParameters,
        HAPTLVReader* requestReader) {
    HAPPrecondition(rtpParameters);
    HAPPrecondition(requestReader);

    HAPError err;

    HAPRawBufferZero(rtpParameters, sizeof *rtpParameters);

    HAPTLV payloadTypeTLV, ssrcTLV, maxBitrateTLV, minRTCPIntervalTLV, maxMTUTLV;
    payloadTypeTLV.type = kHAPCharacteristicValue_SelectedRTPStreamConfiguration_RTPParameters_PayloadType;
    ssrcTLV.type = kHAPCharacteristicValue_SelectedRTPStreamConfiguration_RTPParameters_SynchronizationSource;
    maxBitrateTLV.type = kHAPCharacteristicValue_SelectedRTPStreamConfiguration_RTPParameters_MaxBitrate;
    minRTCPIntervalTLV.type = kHAPCharacteristicValue_SelectedRTPStreamConfiguration_RTPParameters_MinRTCPInterval;
    maxMTUTLV.type = kHAPCharacteristicValue_SelectedRTPStreamConfiguration_RTPParameters_MaxMTU;
    err = HAPTLVReaderGetAll(
            requestReader,
            (HAPTLV* const[]) { &payloadTypeTLV, &ssrcTLV, &maxBitrateTLV, &minRTCPIntervalTLV, &maxMTUTLV, NULL });
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }

    // Payload type.
    if (payloadTypeTLV.value.bytes) {
        HAPLog(&logObject, "Reconfigure: Video payload type specified, although not expected.");
        return kHAPError_InvalidData;
    }

    // SynchronizationSource for Video.
    if (ssrcTLV.value.bytes) {
        HAPLog(&logObject, "Reconfigure: Video SSRC specified, although not expected.");
        return kHAPError_InvalidData;
    }

    // Maximum Bitrate.
    if (!maxBitrateTLV.value.bytes) {
        HAPLog(&logObject, "Reconfigure: Video maximum bitrate missing.");
        return kHAPError_InvalidData;
    }
    if (maxBitrateTLV.value.numBytes != 2) {
        HAPLog(&logObject,
               "Reconfigure: Video maximum bitrate has invalid length (%lu).",
               (unsigned long) maxBitrateTLV.value.numBytes);
        return kHAPError_InvalidData;
    }
    rtpParameters->maximumBitRate = HAPReadLittleUInt16(maxBitrateTLV.value.bytes);

    // Min RTCP Interval.
    if (!minRTCPIntervalTLV.value.bytes) {
        HAPLog(&logObject, "Reconfigure: Video min RTCP interval missing.");
        return kHAPError_InvalidData;
    }
    if (minRTCPIntervalTLV.value.numBytes != 4) {
        HAPLog(&logObject,
               "Reconfigure: Video min RTCP interval has invalid length (%lu).",
               (unsigned long) minRTCPIntervalTLV.value.numBytes);
        return kHAPError_InvalidData;
    }
    uint32_t bitPattern = HAPReadLittleUInt32(minRTCPIntervalTLV.value.bytes);
    rtpParameters->minimumRTCPInterval = HAPFloatFromBitPattern(bitPattern);
    if (!HAPFloatIsFinite(rtpParameters->minimumRTCPInterval) || rtpParameters->minimumRTCPInterval < 0) {
        HAPLog(&logObject,
               "Reconfigure: Video min RTCP interval invalid: %g.",
               (double) rtpParameters->minimumRTCPInterval);
        return kHAPError_InvalidData;
    }

    // Max MTU.
    if (maxMTUTLV.value.bytes) {
        HAPLog(&logObject, "Reconfigure: Video max MTU specified, although not expected.");
        return kHAPError_InvalidData;
    }

    return kHAPError_None;
}

/**
 * Parses selected video parameters of a Reconfigure streaming sessions command.
 *
 * @param[out] videoAttributes      Parsed video attributes.
 * @param[out] rtpParameters        Parsed RTP parameters.
 * @param      requestReader        TLV reader.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the request reader contains invalid data.
 */
HAP_RESULT_USE_CHECK
static HAPError TryParseReconfigureStreamingSessionsVideoParameters(
        HAPVideoAttributes* videoAttributes,
        HAPPlatformCameraReconfigureStreamingSessionVideoRTPParameters* rtpParameters,
        HAPTLVReader* requestReader) {
    HAPPrecondition(videoAttributes);
    HAPPrecondition(rtpParameters);
    HAPPrecondition(requestReader);

    HAPError err;

    HAPTLV codecTypeTLV, codecParametersTLV, attributesTLV, rtpParametersTLV;
    codecTypeTLV.type = kHAPCharacteristicValue_SelectedRTPStreamConfiguration_Video_CodecType;
    codecParametersTLV.type = kHAPCharacteristicValue_SelectedRTPStreamConfiguration_Video_CodecParameters;
    attributesTLV.type = kHAPCharacteristicValue_SelectedRTPStreamConfiguration_Video_Attributes;
    rtpParametersTLV.type = kHAPCharacteristicValue_SelectedRTPStreamConfiguration_Video_RTPParameters;
    err = HAPTLVReaderGetAll(
            requestReader,
            (HAPTLV* const[]) { &codecTypeTLV, &codecParametersTLV, &attributesTLV, &rtpParametersTLV, NULL });
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }

    // Selected Video Codec type.
    if (codecTypeTLV.value.bytes) {
        HAPLog(&logObject, "Reconfigure: Selected Video Codec type specified, although not expected.");
        return kHAPError_InvalidData;
    }

    // Selected Video Codec parameters.
    if (codecParametersTLV.value.bytes) {
        HAPLog(&logObject, "Reconfigure: Selected Video Codec parameters specified, although not expected.");
        return kHAPError_InvalidData;
    }

    // Selected Video attributes.
    if (!attributesTLV.value.bytes) {
        HAPLog(&logObject, "Reconfigure: Selected Video attributes missing.");
        return kHAPError_InvalidData;
    }
    {
        HAPTLVReader subReader;
        HAPTLVReaderCreate(&subReader, (void*) (uintptr_t) attributesTLV.value.bytes, attributesTLV.value.numBytes);

        err = TryParseVideoAttributes(videoAttributes, &subReader);
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            return err;
        }
    }

    // Selected Video RTP parameters.
    if (!rtpParametersTLV.value.bytes) {
        HAPLog(&logObject, "Reconfigure: Selected Video RTP parameters missing.");
        return kHAPError_InvalidData;
    }
    {
        HAPTLVReader subReader;
        HAPTLVReaderCreate(
                &subReader, (void*) (uintptr_t) rtpParametersTLV.value.bytes, rtpParametersTLV.value.numBytes);

        err = TryParseReconfigureStreamingSessionsVideoRTPParameters(rtpParameters, &subReader);
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            return err;
        }
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static HAPError CheckSupportedVideoParameters(
        const HAPCameraStreamSupportedConfigurations* streamConfiguration,
        const HAPPlatformCameraStartStreamingSessionConfiguration* actual) {
    // General codec restrictions.
    if (actual->video.codecType == kHAPVideoCodecType_H264) {
        uint32_t pixelLimit = HAPVideoGetH264ProfileLevelPixelLimit(actual->video.codecParameters.level);
        if ((uint32_t) actual->video.attributes.width * (uint32_t) actual->video.attributes.height > pixelLimit) {
            HAPLogInfo(&logObject, "Warning: Requested resolution is not supported by requested H.264 profile level.");
            // ADK supports the 1536x1536 resolution, which requires H.264 profile level 5. However, the HAP R17
            // specification does not provide a way for a controller to specify profile level 5.
            // Therefore, we leave it up to the accessory to decide how it wants to handle mismatched
            // profile level and resolutions, in order to enable the controller requesting 1536x1536 resolution.
            // Note: Enable the "return kHAPError_InvalidData;" line below if an accessory wants resolutions to conform
            // to the controller specified profile level. Enabling this line will disable resolutions that require
            // profile level 5 (or any other profile level not specified in HAP R17).
            // @see  HomeKit Accessory Protocol Specification R17
            //       Table 10‐4: Video Codec Streaming Configuration TLV Types
            //       Table 10‐42: Video Codec Recording Parameters H.264 TLV Types
            // return kHAPError_InvalidData;
        }
    }

    if (streamConfiguration->videoStream.configurations) {
        for (size_t i = 0; streamConfiguration->videoStream.configurations[i]; i++) {
            const HAPCameraSupportedVideoCodecConfiguration* configuration =
                    streamConfiguration->videoStream.configurations[i];
            if (actual->video.codecType == kHAPVideoCodecType_H264 &&
                configuration->codecType == kHAPVideoCodecType_H264) {
                const HAPH264VideoCodecParameters* codec = configuration->codecParameters;
                if (codec && actual->video.codecParameters.profile & codec->profile &&
                    actual->video.codecParameters.level & codec->level &&
                    actual->video.codecParameters.packetizationMode & codec->packetizationMode &&
                    (!codec->bitRate || actual->video.codecParameters.bitRate <= codec->bitRate) &&
                    (!codec->iFrameInterval || actual->video.codecParameters.iFrameInterval <= codec->iFrameInterval) &&
                    configuration->attributes) {
                    for (size_t j = 0; configuration->attributes[j]; j++) {
                        const HAPVideoAttributes* attributes = configuration->attributes[j];
                        if (actual->video.attributes.width == attributes->width &&
                            actual->video.attributes.height == attributes->height &&
                            actual->video.attributes.maxFrameRate <= attributes->maxFrameRate) {
                            // we found a compatible configuration
                            return kHAPError_None;
                        }
                    }
                }
            }
        }
    }
    // no compatible configuration found
    return kHAPError_InvalidData;
}

HAP_RESULT_USE_CHECK
static HAPError CheckSupportedAudioParameters(
        const HAPCameraStreamSupportedConfigurations* streamConfiguration,
        const HAPPlatformCameraStartStreamingSessionConfiguration* actual) {
    if (streamConfiguration->audioStream.configurations) {
        for (size_t i = 0; streamConfiguration->audioStream.configurations[i]; i++) {
            const HAPCameraSupportedAudioCodecConfiguration* configuration =
                    streamConfiguration->audioStream.configurations[i];
            const HAPAudioCodecParameters* parameters = configuration->codecParameters;
            if (parameters && actual->audio.codecType == configuration->codecType &&
                actual->audio.codecParameters.numberOfChannels == parameters->numberOfChannels &&
                actual->audio.codecParameters.bitRateMode & parameters->bitRateMode &&
                actual->audio.codecParameters.sampleRate & parameters->sampleRate) {
                // we found a compatible configuration
                return kHAPError_None;
            }
        }
    }
    // no compatible configuration found
    return kHAPError_InvalidData;
}

HAP_RESULT_USE_CHECK
static HAPError CheckSupportedReconfigureParameters(
        const HAPCameraStreamSupportedConfigurations* streamConfiguration,
        const HAPCameraStreamingConfiguration* initial,
        const HAPPlatformCameraReconfigureStreamingSessionConfiguration* actual) {
    if (streamConfiguration->videoStream.configurations) {
        for (size_t i = 0; streamConfiguration->videoStream.configurations[i]; i++) {
            const HAPCameraSupportedVideoCodecConfiguration* configuration =
                    streamConfiguration->videoStream.configurations[i];
            if (initial->codecType == kHAPVideoCodecType_H264 && configuration->codecType == kHAPVideoCodecType_H264) {
                const HAPH264VideoCodecParameters* codec = configuration->codecParameters;
                if (codec && initial->codecParameters.profile & codec->profile &&
                    initial->codecParameters.level & codec->level &&
                    initial->codecParameters.packetizationMode & codec->packetizationMode &&
                    (!codec->bitRate || initial->codecParameters.bitRate <= codec->bitRate) &&
                    (!codec->iFrameInterval || initial->codecParameters.iFrameInterval <= codec->iFrameInterval) &&
                    configuration->attributes) {
                    for (size_t j = 0; configuration->attributes[j]; j++) {
                        const HAPVideoAttributes* attributes = configuration->attributes[j];
                        if (actual->video.attributes.width == attributes->width &&
                            actual->video.attributes.height == attributes->height &&
                            actual->video.attributes.maxFrameRate <= attributes->maxFrameRate) {
                            // we found a compatible configuration
                            return kHAPError_None;
                        }
                    }
                }
            }
        }
    }
    // no compatible configuration found
    return kHAPError_InvalidData;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleCameraRTPStreamManagementSelectedRTPStreamConfigurationWrite(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicWriteRequest* request,
        HAPTLVReader* requestReader,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->service);
    HAPPrecondition(requestReader);

    HAPError err;

    size_t streamIndex = GetStreamIndex(request->service, request->accessory);

    HAPPlatformCameraRef camera;
    size_t baseIndex;
    GetCameraAndIndex(server, request->accessory, &camera, &baseIndex);
    HAPCameraStreamingSession* streamingSession =
            &server->ip.camera.streamingSessionStorage.sessions[baseIndex + streamIndex];

    err = HAPCameraCheckStreamingEnabled(camera, streamIndex);
    if (err) {
        HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidState);
        return err;
    }

    HAPSession* session = request->session;

    // Get status.
    HAPCameraStreamingStatus status = HAPPlatformCameraGetStreamStatus(camera, streamIndex);

    switch (status) {
        case kHAPCameraStreamingStatus_Available: {
            HAPLog(&logObject, "Camera stream is not set up.");
        }
            return kHAPError_InvalidState;
        case kHAPCameraStreamingStatus_Unavailable: {
            HAPLog(&logObject, "Camera stream is unavailable.");
        }
            return kHAPError_InvalidState;
        case kHAPCameraStreamingStatus_InUse: {
            if (streamingSession->hapSession != session) {
                HAPLog(&logObject, "Camera stream is in use by other controller.");
                return kHAPError_InvalidState;
            }

            // Log.
            char logBytes[2048];
            HAPRawBufferZero(logBytes, sizeof logBytes);
            Log l = { .bytes = logBytes, .maxBytes = sizeof logBytes, .numBytes = 0 };
            LogAppend(&l, "Selected RTP Stream Configuration:\n");

            HAPTLV sessionControlTLV, videoParametersTLV, audioParametersTLV;
            sessionControlTLV.type = kHAPCharacteristicValue_SelectedRTPStreamConfiguration_SessionControl;
            videoParametersTLV.type = kHAPCharacteristicValue_SelectedRTPStreamConfiguration_Video;
            audioParametersTLV.type = kHAPCharacteristicValue_SelectedRTPStreamConfiguration_Audio;
            err = HAPTLVReaderGetAll(
                    requestReader,
                    (HAPTLV* const[]) { &sessionControlTLV, &videoParametersTLV, &audioParametersTLV, NULL });
            if (err) {
                HAPAssert(err == kHAPError_InvalidData);
                AbortStreaming(server, request->service, request->accessory);
                return err;
            }

            // Session Control.
            HAPCharacteristicValue_SelectedRTPStreamConfiguration_SessionControl_Command command;
            if (!sessionControlTLV.value.bytes) {
                HAPLog(&logObject, "Session Control missing.");
                AbortStreaming(server, request->service, request->accessory);
                return kHAPError_InvalidData;
            }
            {
                HAPTLVReader subReader;
                HAPTLVReaderCreate(
                        &subReader,
                        (void*) (uintptr_t) sessionControlTLV.value.bytes,
                        sessionControlTLV.value.numBytes);

                HAPTLV sessionIDTLV, commandTLV;
                sessionIDTLV.type =
                        kHAPCharacteristicValue_SelectedRTPStreamConfiguration_SessionControl_SessionIdentifier;
                commandTLV.type = kHAPCharacteristicValue_SelectedRTPStreamConfiguration_SessionControl_Command;
                err = HAPTLVReaderGetAll(&subReader, (HAPTLV* const[]) { &sessionIDTLV, &commandTLV, NULL });
                if (err) {
                    HAPAssert(err == kHAPError_InvalidData);
                    AbortStreaming(server, request->service, request->accessory);
                    return err;
                }

                // Session Identifier.
                if (!sessionIDTLV.value.bytes) {
                    HAPLog(&logObject, "Session Identifier missing.");
                    AbortStreaming(server, request->service, request->accessory);
                    return kHAPError_InvalidData;
                }
                if (sessionIDTLV.value.numBytes != sizeof streamingSession->sessionID) {
                    HAPLog(&logObject,
                           "Session Identifier has invalid length (%lu).",
                           (unsigned long) sessionIDTLV.value.numBytes);
                    AbortStreaming(server, request->service, request->accessory);
                    return kHAPError_InvalidData;
                }
                if (!HAPRawBufferAreEqual(
                            HAPNonnullVoid(sessionIDTLV.value.bytes),
                            streamingSession->sessionID.value,
                            sessionIDTLV.value.numBytes)) {
                    HAPLogBuffer(
                            &logObject,
                            sessionIDTLV.value.bytes,
                            sessionIDTLV.value.numBytes,
                            "Session Identifier does not match.");
                    AbortStreaming(server, request->service, request->accessory);
                    return kHAPError_InvalidState;
                }
                LogAppend(
                        &l,
                        "    Session ID: %02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X\n",
                        streamingSession->sessionID.value[0],
                        streamingSession->sessionID.value[1],
                        streamingSession->sessionID.value[2],
                        streamingSession->sessionID.value[3],
                        streamingSession->sessionID.value[4],
                        streamingSession->sessionID.value[5],
                        streamingSession->sessionID.value[6],
                        streamingSession->sessionID.value[7],
                        streamingSession->sessionID.value[8],
                        streamingSession->sessionID.value[9],
                        streamingSession->sessionID.value[10],
                        streamingSession->sessionID.value[11],
                        streamingSession->sessionID.value[12],
                        streamingSession->sessionID.value[13],
                        streamingSession->sessionID.value[14],
                        streamingSession->sessionID.value[15]);

                // Command.
                if (!commandTLV.value.bytes) {
                    HAPLog(&logObject, "Command missing.");
                    AbortStreaming(server, request->service, request->accessory);
                    return kHAPError_InvalidData;
                }
                if (commandTLV.value.numBytes != 1) {
                    HAPLog(&logObject, "Command has invalid length (%lu).", (unsigned long) commandTLV.value.numBytes);
                    AbortStreaming(server, request->service, request->accessory);
                    return kHAPError_InvalidData;
                }
                uint8_t cmd = ((const uint8_t*) commandTLV.value.bytes)[0];
                if (cmd != kHAPCharacteristicValue_SelectedRTPStreamConfiguration_SessionControl_Command_End &&
                    cmd != kHAPCharacteristicValue_SelectedRTPStreamConfiguration_SessionControl_Command_Start &&
                    cmd != kHAPCharacteristicValue_SelectedRTPStreamConfiguration_SessionControl_Command_Suspend &&
                    cmd != kHAPCharacteristicValue_SelectedRTPStreamConfiguration_SessionControl_Command_Resume &&
                    cmd != kHAPCharacteristicValue_SelectedRTPStreamConfiguration_SessionControl_Command_Reconfigure) {
                    HAPLog(&logObject, "Command invalid: %u.", cmd);
                    AbortStreaming(server, request->service, request->accessory);
                    return kHAPError_InvalidData;
                }

                command = (HAPCharacteristicValue_SelectedRTPStreamConfiguration_SessionControl_Command) cmd;
            }

            switch (command) {
                case kHAPCharacteristicValue_SelectedRTPStreamConfiguration_SessionControl_Command_End: {
                    LogAppend(&l, "    Command: End streaming session");

                    if (videoParametersTLV.value.bytes) {
                        HAPLog(&logObject, "End: Selected Video Parameters specified, although not expected.");
                        AbortStreaming(server, request->service, request->accessory);
                        return kHAPError_InvalidData;
                    }
                    if (audioParametersTLV.value.bytes) {
                        HAPLog(&logObject, "End: Selected Audio Parameters specified, although not expected.");
                        AbortStreaming(server, request->service, request->accessory);
                        return kHAPError_InvalidData;
                    }

                    // Log.
                    HAPLogInfo(&logObject, "%s", l.bytes);

                    // End streaming session.
                    EndStreaming(server, request->service, request->accessory);
                }
                    return kHAPError_None;
                case kHAPCharacteristicValue_SelectedRTPStreamConfiguration_SessionControl_Command_Start: {
                    LogAppend(&l, "    Command: Start streaming session\n");

                    HAPPlatformCameraStartStreamingSessionConfiguration cfg;
                    HAPRawBufferZero(&cfg, sizeof cfg);

                    // Selected Video Parameters.
                    if (!videoParametersTLV.value.bytes) {
                        HAPLog(&logObject, "Start: Selected Video Parameters missing.");
                        AbortStreaming(server, request->service, request->accessory);
                        return kHAPError_InvalidData;
                    }
                    {
                        HAPTLVReader subReader;
                        HAPTLVReaderCreate(
                                &subReader,
                                (void*) (uintptr_t) videoParametersTLV.value.bytes,
                                videoParametersTLV.value.numBytes);

                        err = TryParseStartStreamingSessionVideoParameters(
                                &cfg.video.codecType,
                                &cfg.video.codecParameters,
                                &cfg.video.attributes,
                                &cfg.video.rtpParameters,
                                &subReader);
                        if (err) {
                            HAPAssert(err == kHAPError_InvalidData);
                            AbortStreaming(server, request->service, request->accessory);
                            return err;
                        }
                    }
                    LogAppend(&l, "    Selected Video Parameters:\n");
                    LogAppend(
                            &l,
                            "        Selected Video Codec type: %s\n",
                            GetVideoCodecTypeDescription(cfg.video.codecType));
                    LogAppend(&l, "        Selected Video Codec parameters:\n");
                    switch (cfg.video.codecType) {
                        case kHAPVideoCodecType_H264: {
                            LogAppend(
                                    &l,
                                    "            ProfileID: %s\n"
                                    "            Level: %s\n"
                                    "            Packetization mode: %s\n",
                                    HAPVideoGetH264ProfileIDDescription(cfg.video.codecParameters.profile),
                                    HAPVideoGetH264ProfileLevelDescription(cfg.video.codecParameters.level),
                                    HAPVideoGetH264PacketizationModeDescription(
                                            cfg.video.codecParameters.packetizationMode));
                            LogAppend(
                                    &l,
                                    "            Target bit rate: %lu kbit/s\n"
                                    "            Requested I-Frame interval: %lu ms\n",
                                    (unsigned long) cfg.video.codecParameters.bitRate,
                                    (unsigned long) cfg.video.codecParameters.iFrameInterval);
                            break;
                        }
                    }
                    LogAppend(&l, "        Selected Video attributes:\n");
                    LogAppend(
                            &l,
                            "            %4u x %4u @ %u fps\n",
                            cfg.video.attributes.width,
                            cfg.video.attributes.height,
                            cfg.video.attributes.maxFrameRate);
                    LogAppend(&l, "        Selected Video RTP parameters:\n");
                    LogAppend(&l, "            Payload type: 0x%02X\n", cfg.video.rtpParameters.type);
                    LogAppend(
                            &l,
                            "            SynchronizationSource for Video: 0x%08lX\n",
                            (unsigned long) cfg.video.rtpParameters.ssrc);
                    LogAppend(&l, "            Maximum Bitrate: %d kbps\n", cfg.video.rtpParameters.maximumBitRate);
                    LogAppend(
                            &l,
                            "            Min RTCP interval: %g seconds\n",
                            (double) cfg.video.rtpParameters.minimumRTCPInterval);
                    LogAppend(&l, "            Max MTU: ");
                    if (cfg.video.rtpParameters.maximumMTU) {
                        LogAppend(&l, "%u bytes\n", cfg.video.rtpParameters.maximumMTU);
                    } else {
                        LogAppend(&l, "Default (IPv4: 1378 bytes, IPv6: 1228 bytes)\n");
                    }

                    // Selected Audio Parameters.
                    if (!audioParametersTLV.value.bytes) {
                        HAPLog(&logObject, "Start: Selected Audio Parameters missing.");
                        AbortStreaming(server, request->service, request->accessory);
                        return kHAPError_InvalidData;
                    }
                    {
                        HAPTLVReader subReader;
                        HAPTLVReaderCreate(
                                &subReader,
                                (void*) (uintptr_t) audioParametersTLV.value.bytes,
                                audioParametersTLV.value.numBytes);

                        err = TryParseStartStreamingSessionAudioParameters(
                                &cfg.audio.codecType,
                                &cfg.audio.codecParameters,
                                &cfg.audio.rtpTime,
                                &cfg.audio.rtpParameters,
                                &cfg.audio.comfortNoise,
                                &subReader);
                        if (err) {
                            HAPAssert(err == kHAPError_InvalidData);
                            AbortStreaming(server, request->service, request->accessory);
                            return err;
                        }
                    }
                    LogAppend(&l, "    Selected Audio Parameters:\n");
                    LogAppend(
                            &l,
                            "        Selected Audio Codec type: %s\n",
                            GetAudioCodecTypeDescription(cfg.audio.codecType));
                    LogAppend(&l, "        Selected Audio Codec parameters:\n");
                    LogAppend(&l, "            Audio channels: %u\n", cfg.audio.codecParameters.numberOfChannels);
                    LogAppend(
                            &l,
                            "            Bit-rate: %s\n",
                            GetAudioBitRateDescription(cfg.audio.codecParameters.bitRateMode));
                    LogAppend(
                            &l,
                            "            Sample rate: %s\n",
                            GetAudioSampleRateDescription(cfg.audio.codecParameters.sampleRate));
                    LogAppend(&l, "            RTP time: %u ms\n", cfg.audio.rtpTime);
                    LogAppend(&l, "        Selected Audio RTP parameters:\n");
                    LogAppend(&l, "            Payload type: 0x%02X\n", cfg.audio.rtpParameters.type);
                    LogAppend(
                            &l,
                            "            SynchronizationSource for Audio: 0x%08lX\n",
                            (unsigned long) cfg.audio.rtpParameters.ssrc);
                    LogAppend(&l, "            Maximum Bitrate: %d kbps\n", cfg.audio.rtpParameters.maximumBitRate);
                    LogAppend(
                            &l,
                            "            Min RTCP interval: %g seconds\n",
                            (double) cfg.audio.rtpParameters.minimumRTCPInterval);
                    if (cfg.audio.comfortNoise.enabled) {
                        LogAppend(&l, "            Comfort Noise Payload Type: 0x%02X\n", cfg.audio.comfortNoise.type);
                    }
                    LogAppend(&l, "        Comfort Noise: %s", cfg.audio.comfortNoise.enabled ? "Enabled" : "Disabled");

                    // Log.
                    HAPLogInfo(&logObject, "%s", l.bytes);

                    err = CheckSupportedVideoParameters(
                            request->accessory->cameraStreamConfigurations[streamIndex], &cfg);
                    if (err) {
                        HAPAssert(err == kHAPError_InvalidData);
                        HAPLogError(&logObject, "Unsupported video parameters requested");
                        AbortStreaming(server, request->service, request->accessory);
                        return err;
                    }
                    err = CheckSupportedAudioParameters(
                            request->accessory->cameraStreamConfigurations[streamIndex], &cfg);
                    if (err) {
                        HAPAssert(err == kHAPError_InvalidData);
                        HAPLogError(&logObject, "Unsupported audio parameters requested");
                        AbortStreaming(server, request->service, request->accessory);
                        return err;
                    }

                    // Save initial video configuration
                    streamingSession->configuration.codecType = cfg.video.codecType;
                    streamingSession->configuration.codecParameters.profile = cfg.video.codecParameters.profile;
                    streamingSession->configuration.codecParameters.level = cfg.video.codecParameters.level;
                    streamingSession->configuration.codecParameters.packetizationMode =
                            cfg.video.codecParameters.packetizationMode;

                    // Copy SynchronizationSource.
                    streamingSession->controllerEndpoint.video.ssrc = cfg.video.rtpParameters.ssrc;
                    streamingSession->controllerEndpoint.audio.ssrc = cfg.audio.rtpParameters.ssrc;

                    // Use default if MTU is undefined
                    if (cfg.video.rtpParameters.maximumMTU == 0) {
                        // use default
                        if (streamingSession->accessoryEndpoint.ipAddress.version == kHAPIPAddressVersion_IPv6) {
                            cfg.video.rtpParameters.maximumMTU = 1228; // IP V6 default
                        } else {
                            cfg.video.rtpParameters.maximumMTU = 1378; // IP V4 default
                        }
                    }

                    // Start streaming session.
                    err = HAPPlatformCameraStartStreamingSession(
                            camera,
                            streamIndex,
                            &streamingSession->controllerEndpoint,
                            &streamingSession->accessoryEndpoint,
                            &cfg);
                    if (err) {
                        HAPAssert(
                                err == kHAPError_Unknown || err == kHAPError_InvalidState ||
                                err == kHAPError_InvalidData || err == kHAPError_OutOfResources);
                        AbortStreaming(server, request->service, request->accessory);
                        return err;
                    }

                    if (streamingSession->hapSessionTimer) {
                        // Deregister HAP session timer.
                        HAPPlatformTimerDeregister(streamingSession->hapSessionTimer);
                        streamingSession->hapSessionTimer = 0;
                    }

                    // Update streaming session state.
                    streamingSession->isSessionActive = true;
                }
                    return kHAPError_None;
                case kHAPCharacteristicValue_SelectedRTPStreamConfiguration_SessionControl_Command_Suspend: {
                    LogAppend(&l, "    Command: Suspend streaming session");

                    if (videoParametersTLV.value.bytes) {
                        HAPLog(&logObject, "Suspend: Selected Video Parameters specified, although not expected.");
                        AbortStreaming(server, request->service, request->accessory);
                        return kHAPError_InvalidData;
                    }
                    if (audioParametersTLV.value.bytes) {
                        HAPLog(&logObject, "Suspend: Selected Audio Parameters specified, although not expected.");
                        AbortStreaming(server, request->service, request->accessory);
                        return kHAPError_InvalidData;
                    }

                    if (!streamingSession->isSessionActive) {
                        HAPLog(&logObject, "Suspend: Selected stream is not started yet.");
                        AbortStreaming(server, request->service, request->accessory);
                        return kHAPError_InvalidState;
                    }

                    // Log.
                    HAPLogInfo(&logObject, "%s", l.bytes);

                    // Suspend streaming session.
                    err = HAPPlatformCameraSuspendStreamingSession(camera, streamIndex);
                    if (err) {
                        HAPAssert(err == kHAPError_Unknown);
                        AbortStreaming(server, request->service, request->accessory);
                        return err;
                    }

                    if (streamingSession->hapSessionTimer) {
                        // Example scenario: Start > Suspend > Suspend.
                        // Deregister HAP session timer.
                        HAPPlatformTimerDeregister(streamingSession->hapSessionTimer);
                        streamingSession->hapSessionTimer = 0;
                    }

                    // Register HAP session timer.
                    HAPTime now = HAPPlatformClockGetCurrent();
                    HAPTime deadline = now + kHAPCamera_HAPSessionResumeTimeout;
                    HAPAssert(deadline >= now);
                    err = HAPPlatformTimerRegister(
                            &streamingSession->hapSessionTimer,
                            deadline,
                            HandleHAPSessionTimerExpired,
                            streamingSession);
                    if (err) {
                        HAPAssert(err == kHAPError_OutOfResources);
                        HAPLogError(&logObject, "Not enough timers available (IP camera HAP session timer).");
                        AbortStreaming(server, request->service, request->accessory);
                        return err;
                    }
                    HAPLogInfo(
                            &logObject,
                            "Waiting for Resume (timeout: %llu seconds).",
                            (unsigned long long) (kHAPCamera_HAPSessionResumeTimeout / HAPSecond));
                }
                    return kHAPError_None;
                case kHAPCharacteristicValue_SelectedRTPStreamConfiguration_SessionControl_Command_Resume: {
                    LogAppend(&l, "    Command: Resume streaming session");

                    if (videoParametersTLV.value.bytes) {
                        HAPLog(&logObject, "Resume: Selected Video Parameters specified, although not expected.");
                        AbortStreaming(server, request->service, request->accessory);
                        return kHAPError_InvalidData;
                    }
                    if (audioParametersTLV.value.bytes) {
                        HAPLog(&logObject, "Resume: Selected Audio Parameters specified, although not expected.");
                        AbortStreaming(server, request->service, request->accessory);
                        return kHAPError_InvalidData;
                    }

                    if (!streamingSession->isSessionActive) {
                        HAPLog(&logObject, "Resume: Selected stream is not started yet.");
                        AbortStreaming(server, request->service, request->accessory);
                        return kHAPError_InvalidState;
                    }

                    // Log.
                    HAPLogInfo(&logObject, "%s", l.bytes);

                    // Resume streaming session.
                    err = HAPPlatformCameraResumeStreamingSession(camera, streamIndex);
                    if (err) {
                        HAPAssert(err == kHAPError_Unknown);
                        AbortStreaming(server, request->service, request->accessory);
                        return err;
                    }

                    if (streamingSession->hapSessionTimer) {
                        // Deregister HAP session timer.
                        HAPPlatformTimerDeregister(streamingSession->hapSessionTimer);
                        streamingSession->hapSessionTimer = 0;
                    }
                }
                    return kHAPError_None;
                case kHAPCharacteristicValue_SelectedRTPStreamConfiguration_SessionControl_Command_Reconfigure: {
                    LogAppend(&l, "    Command: Reconfigure streaming sessions\n");

                    HAPPlatformCameraReconfigureStreamingSessionConfiguration cfg;
                    HAPRawBufferZero(&cfg, sizeof cfg);

                    // Selected Video Parameters.
                    if (!videoParametersTLV.value.bytes) {
                        HAPLog(&logObject, "Reconfigure: Selected Video Parameters missing.");
                        return kHAPError_InvalidData;
                    }
                    {
                        HAPTLVReader subReader;
                        HAPTLVReaderCreate(
                                &subReader,
                                (void*) (uintptr_t) videoParametersTLV.value.bytes,
                                videoParametersTLV.value.numBytes);

                        err = TryParseReconfigureStreamingSessionsVideoParameters(
                                &cfg.video.attributes, &cfg.video.rtpParameters, &subReader);
                        if (err) {
                            HAPAssert(err == kHAPError_InvalidData);
                            return err;
                        }
                    }
                    LogAppend(&l, "    Selected Video Parameters:\n");
                    LogAppend(&l, "        Selected Video attributes:\n");
                    LogAppend(
                            &l,
                            "            %4u x %4u @ %u fps\n",
                            cfg.video.attributes.width,
                            cfg.video.attributes.height,
                            cfg.video.attributes.maxFrameRate);
                    LogAppend(&l, "        Selected Video RTP parameters:\n");
                    LogAppend(&l, "            Maximum Bitrate: %d kbps\n", cfg.video.rtpParameters.maximumBitRate);
                    LogAppend(
                            &l,
                            "            Min RTCP interval: %g seconds",
                            (double) cfg.video.rtpParameters.minimumRTCPInterval);

                    // Selected Audio Parameters.
                    if (audioParametersTLV.value.bytes) {
                        HAPLog(&logObject, "Reconfigure: Selected Audio Parameters specified, although not expected.");
                        return kHAPError_InvalidData;
                    }

                    if (!streamingSession->isSessionActive) {
                        HAPLog(&logObject, "Reconfigure: Selected stream is not started yet.");
                        AbortStreaming(server, request->service, request->accessory);
                        return kHAPError_InvalidState;
                    }

                    // Log.
                    HAPLogInfo(&logObject, "%s", l.bytes);

                    err = CheckSupportedReconfigureParameters(
                            request->accessory->cameraStreamConfigurations[streamIndex],
                            &streamingSession->configuration,
                            &cfg);
                    if (err) {
                        HAPAssert(err == kHAPError_InvalidData);
                        HAPLogError(&logObject, "Unsupported video parameters requested");
                        return err;
                    }

                    // Reconfigure streaming session.
                    err = HAPPlatformCameraReconfigureStreamingSession(camera, streamIndex, &cfg);
                    if (err) {
                        HAPAssert(
                                err == kHAPError_Unknown || err == kHAPError_InvalidData ||
                                err == kHAPError_OutOfResources);
                        AbortStreaming(server, request->service, request->accessory);
                        return err;
                    }

                    if (streamingSession->hapSessionTimer) {
                        // Example scenario: Start > Suspend > Reconfigure.
                        // Renew HAP session timer.
                        HAPTime now = HAPPlatformClockGetCurrent();
                        HAPTime deadline = now + kHAPCamera_HAPSessionResumeTimeout;
                        HAPAssert(deadline >= now);
                        HAPPlatformTimerDeregister(streamingSession->hapSessionTimer);
                        err = HAPPlatformTimerRegister(
                                &streamingSession->hapSessionTimer,
                                deadline,
                                HandleHAPSessionTimerExpired,
                                streamingSession);
                        if (err) {
                            HAPAssert(err == kHAPError_OutOfResources);
                            HAPLogError(&logObject, "Not enough timers available (IP camera HAP session timer).");
                            AbortStreaming(server, request->service, request->accessory);
                            return err;
                        }
                        HAPLogInfo(
                                &logObject,
                                "Waiting for Resume (timeout: %llu seconds).",
                                (unsigned long long) (kHAPCamera_HAPSessionResumeTimeout / HAPSecond));
                    }
                }
                    return kHAPError_None;
                default:
                    HAPFatalError();
            }
        }
            HAPFatalError();
        default:
            HAPFatalError();
    }
}

#endif
