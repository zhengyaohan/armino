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

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "HAP.h"
#include "HAPPlatformCamera+Init.h"
#include "HAPPlatformLog+Init.h"
#include "HAPPlatformMicrophone+Init.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)

#import <CoreGraphics/CoreGraphics.h>
#import <CoreImage/CoreImage.h>
#import <CoreServices/CoreServices.h>
#import <Foundation/Foundation.h>
#import <Network/Network.h>

static const char* HAPCameraInterfaceName = NULL;

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "Camera" };

/**
 * Camera logging subsystem.
 *
 * It is possible to direct logging output to different log handlers. The log handlers need to be configured in
 * @fn HAPPlatformLogCapture.
 * Usage example:

    @code{.c}

    // Log an info-level buffer in the packet logger.
    HAPLogBufferInfo(&kHAPRTPController_PacketLog, bytes, numBytes, "(%p) Packet log", (const void *) stream);

    // Log an info-level video payload in the video logger
    HAPLogBufferInfo(&kHAPPlatformCamera_VideoLog, bytes, numBytes, "(%p) Video log", (const void *) stream);

    // Log an info-level audio payload in the audio logger
    HAPLogBufferInfo(&kHAPPlatformCamera_AudioLog, bytes, numBytes, "(%p) Audio log", (const void *) stream);

    // Log an info-level audio payload in the speaker logger
    HAPLogBufferInfo(&kHAPPlatformCamera_SpeakerLog, bytes, numBytes, "(%p) Speaker log", (const void *) stream);
    @endcode
 */
/**@{*/
const HAPLogObject kHAPPlatformCamera_VideoLog = { .subsystem = kHAPPlatform_LogSubsystem, .category = "Camera" };

const HAPLogObject kHAPPlatformCamera_AudioLog = { .subsystem = kHAPPlatform_LogSubsystem, .category = "Camera" };

const HAPLogObject kHAPPlatformCamera_SpeakerLog = { .subsystem = kHAPPlatform_LogSubsystem, .category = "Camera" };

/**@}*/

/**
 * Maximum RTP packet size
 */
#define kMaxPacketSize (4096)

/**
 * Video keyframe interval [s].
 */
#define kVideoKeyframeInterval (5)

/**
 * H264 Timestamp Frequency
 *
 * Given by RFC6184
 */
#define kH264TimestampFrequency (90000)

/**
 * Socket timeout [us] (100 ms)
 *
 * - Used as the polling interval for RTP stream feedback and requests to end a streaming session.
 */
#define kSocketTimeoutUs (100000)

/**
 * Stream timeout (30 s)
 */
#define kStreamTimeout (30)

/**
 * Accessory port video
 */
#define kHAPAccessoryPort_Video ((HAPNetworkPort) 5010)

/**
 * Accessory port audio
 */
#define kHAPAccessoryPort_Audio ((HAPNetworkPort) 5012)

#define MAX_SNAPSHOT 5

#define kMediaFilePath "PAL/Darwin/MediaFiles"

void SendRTP(nw_connection_t connection, const uint8_t* packet, size_t length);

@class Session;

static nw_connection_t CreateConnection(
        const HAPPlatformCameraIPAddress* localAddr,
        uint16_t localPort,
        const HAPPlatformCameraIPAddress* remoteAddr,
        uint16_t remotePort) {
    nw_endpoint_t endpoint =
            nw_endpoint_create_host(remoteAddr->ipString, [NSString stringWithFormat:@"%u", remotePort].UTF8String);
    nw_parameters_configure_protocol_block_t configure_tls = NW_PARAMETERS_DISABLE_PROTOCOL;
    nw_parameters_t parameters = nw_parameters_create_secure_udp(configure_tls, NW_PARAMETERS_DEFAULT_CONFIGURATION);
    nw_protocol_stack_t protocol_stack = nw_parameters_copy_default_protocol_stack(parameters);
    nw_protocol_options_t ip_options = nw_protocol_stack_copy_internet_protocol(protocol_stack);
    HAPAssert(ip_options);
    if (localAddr->version == kHAPIPAddressVersion_IPv4) {
        nw_ip_options_set_version(ip_options, nw_ip_version_4);
    } else {
        HAPAssert(localAddr->version == kHAPIPAddressVersion_IPv6);
        nw_ip_options_set_version(ip_options, nw_ip_version_6);
    }
    nw_endpoint_t local_endpoint =
            nw_endpoint_create_host(localAddr->ipString, [NSString stringWithFormat:@"%u", localPort].UTF8String);
    nw_parameters_set_local_endpoint(parameters, local_endpoint);
    nw_connection_t connection = nw_connection_create(endpoint, parameters);
    nw_connection_set_queue(connection, dispatch_get_main_queue());
    nw_connection_set_state_changed_handler(connection, ^(nw_connection_state_t state, nw_error_t error HAP_UNUSED) {
        if (state == nw_connection_state_ready) {
            NSLog(@"Connection started");
        }
        if (state == nw_connection_state_failed || state == nw_connection_state_cancelled) {
            NSLog(@"Connection failed/cancelled");
        }
    });
    nw_connection_start(connection);
    return connection;
}

@interface Session : NSObject {
@public
    nw_connection_t videoConnection;
    nw_connection_t audioConnection;
}
@end

static struct {
    NSMutableArray<Session*>* sessions;
} state = {
    .sessions = nil,
};

static Session* GetSession(size_t streamIndex) {
    HAPLogInfo(&logObject, "%s %zd %zd", __func__, streamIndex, state.sessions.count);
    HAPPrecondition(streamIndex < state.sessions.count);
    return state.sessions[streamIndex];
}

static HAPEpochTime ActualTime() {
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    uint32_t sec = (uint32_t) spec.tv_sec;
    uint32_t ns = (uint32_t) spec.tv_nsec;
    return (HAPEpochTime) sec * HAPSecondNS + ns;
}

void ReceiveRTP(nw_connection_t connection, HAPRTPStream* stream, bool cameraStream) {
    HAPLog(&logObject, "%s", __func__);
    nw_connection_receive(
            connection,
            1,
            UINT32_MAX,
            ^(dispatch_data_t content, nw_content_context_t context HAP_UNUSED, bool done, nw_error_t error) {
                HAPLog(&logObject, "%s done %d cameraStream %d", __func__, done, cameraStream);
                if (content) {

                    // A packet can be both a stream packet or a control packet. If the size is 0, then
                    // a control packet was read. Both data and control packets are sent over the same
                    // socket.
                    NSData* data = (NSData*) content;
                    size_t numPacketBytes = 0;

                    HAPLog(&logObject,
                           "%s have content cameraStream %d %zd bytes",
                           __func__,
                           cameraStream,
                           data.length);

                    HAPEpochTime time = ActualTime();
                    HAPRTPStreamPushPacket(stream, (void*) data.bytes, data.length, &numPacketBytes, time);
                    if (numPacketBytes) {
                        size_t numPayloadBytes;
                        HAPTimeNS sampleTime;
                        uint8_t data[numPacketBytes];
                        HAPRTPStreamPollPayload(stream, data, numPacketBytes, &numPayloadBytes, &sampleTime);
                        if (numPayloadBytes > 0) {
                            // Log payload.
                            HAPLogBufferInfo(
                                    &kHAPPlatformCamera_SpeakerLog,
                                    data,
                                    numPayloadBytes,
                                    "(%p) S",
                                    (const void*) stream);
                        }
                    }
                }
                if (done) {
                    uint8_t packet[kMaxPacketSize];
                    size_t numPacketBytes = 0;
                    HAPEpochTime time = ActualTime();
                    uint32_t bitRate = 0;
                    bool newKeyFrame = false;
                    uint32_t dropoutTime = 0;

                    HAPRTPStreamCheckFeedback(
                            stream,
                            time,
                            packet,
                            sizeof(packet),
                            &numPacketBytes,
                            &bitRate,
                            &newKeyFrame,
                            &dropoutTime);
                    HAPLog(&logObject, "%s StreamFeedback numBytes %zd", __func__, numPacketBytes);

                    if (numPacketBytes) {
                        SendRTP(connection, packet, numPacketBytes);
                    }

                    if (cameraStream) {
                        HAPLog(&logObject, "%s cameraInputStream", __func__);
                        if (bitRate) {
                            HAPLog(&logObject, "%s bitRate", __func__);
                        }
                        if (newKeyFrame) {
                            HAPLog(&logObject, "%s newKeyFrame", __func__);
                        }

                        if (dropoutTime > kStreamTimeout) {
                            HAPLog(&logObject, "%s dropoutTime (%d) > kStreamTimeout", __func__, dropoutTime);
                        }
                    }
                }
                if (!error) {
                    ReceiveRTP(connection, stream, cameraStream);
                } else {
                    HAPLog(&logObject,
                           "%s error! %d %s",
                           __func__,
                           nw_error_get_error_code(error),
                           strerror(nw_error_get_error_code(error)));
                }
            });
}

void SendRTP(nw_connection_t connection, const uint8_t* packet, size_t length) {
    dispatch_queue_t queue = dispatch_get_main_queue();
    dispatch_data_t data = dispatch_data_create(packet, length, queue, NULL);

    nw_connection_send(
            connection,
            (dispatch_data_t) data,
            NW_CONNECTION_DEFAULT_MESSAGE_CONTEXT,
            true,
            ^(nw_error_t _Nullable error) {
                if (error != nil) {
                    HAPLogInfo(
                            &logObject,
                            "%s failed %d %s",
                            __func__,
                            nw_error_get_error_code(error),
                            strerror(nw_error_get_error_code(error)));
                }
            });
}

void CheckRTPTimeouts(Session* session HAP_UNUSED, nw_connection_t connection, HAPRTPStream* stream) {
    HAPEpochTime time = ActualTime();

    uint32_t bitRate;
    bool newKeyFrame;
    uint32_t dropoutTime;
    uint8_t packet[kMaxPacketSize];
    size_t numPacketBytes;
    HAPRTPStreamCheckFeedback(
            stream, time, packet, sizeof packet, &numPacketBytes, &bitRate, &newKeyFrame, &dropoutTime);
    if (numPacketBytes) {
        // send feedback
        SendRTP(connection, packet, numPacketBytes);
    }
    if (bitRate) {
        HAPLogInfo(&logObject, "request to change bitrate");
    }
    if (newKeyFrame) {
        HAPLogInfo(&logObject, "request for keyframe");
    }
}

/**
 * Video data handler.
 *
 * @param      context              Context.
 * @param      cameraInput          Camera.
 * @param      cameraInputStream    Camera stream.
 * @param      bytes                Video data.
 * @param      numBytes             Length of video data.
 * @param      sampleTime           Sample time of first data entry [ns].
 */
static void VideoStreamCallback(
        void* _Nullable context,
        HAPPlatformCameraInputRef cameraInput,
        HAPPlatformCameraInputStreamRef cameraInputStream,
        const void* bytes,
        size_t numBytes,
        HAPTimeNS sampleTime) {
    HAPPrecondition(context);
    HAPPrecondition(cameraInput);
    HAPPrecondition(cameraInputStream);
    HAPPrecondition(bytes);

    HAPPlatformCameraStream* cameraStream = (HAPPlatformCameraStream*) context;
    HAPAssert(cameraStream->isInitialized);
    HAPRTPStream* rtpStream = &cameraStream->rtpStream;
    Session* currentSession = GetSession(cameraStream->streamIndex);

    __atomic_thread_fence(__ATOMIC_SEQ_CST);
    if (cameraStream->isAborted) {
        return;
    }

    // Log payload.
    HAPLogBufferInfo(&kHAPPlatformCamera_VideoLog, bytes, numBytes, "(%p) V", (const void*) cameraStream);

    // Push video data into RTP buffer.
    size_t numPayloadBytes;
    HAPRTPStreamPushPayload(rtpStream, bytes, numBytes, &numPayloadBytes, sampleTime, ActualTime());

    // Send data if available.
    if (numPayloadBytes) {
        for (;;) {
            uint8_t packet[kMaxPacketSize];
            size_t length;
            HAPRTPStreamPollPacket(rtpStream, packet, sizeof packet, &length);
            if (length == 0) {
                break;
            }

            SendRTP(currentSession->videoConnection, packet, length);
        }
    }
}

/**
 * Audio data handler.
 *
 * @param      context              Context.
 * @param      microphone           Microphone.
 * @param      microphoneStream     Microphone stream.
 * @param      bytes                Audio data.
 * @param      numBytes             Length of audio data.
 * @param      sampleTime           Sample time of first data entry [ns].
 * @param      rms                  RMS of raw audio data in range -1 .. +1.
 */
static void AudioStreamCallback(
        void* _Nullable context,
        HAPPlatformMicrophoneRef microphone,
        HAPPlatformMicrophoneStreamRef microphoneStream,
        const void* bytes,
        size_t numBytes,
        HAPTimeNS sampleTime,
        float rms HAP_UNUSED) {
    HAPPrecondition(context);
    HAPPrecondition(microphone);
    HAPPrecondition(microphoneStream);
    HAPPrecondition(bytes);

    HAPPlatformCameraStream* stream = (HAPPlatformCameraStream*) context;
    HAPAssert(stream->isInitialized);
    HAPRTPStream* rtpStream = &stream->rtpStream;
    Session* currentSession = GetSession(stream->streamIndex);

    __atomic_thread_fence(__ATOMIC_SEQ_CST);
    if (stream->isAborted) {
        return;
    }

    // Log payload.
    HAPLogBufferInfo(&kHAPPlatformCamera_AudioLog, bytes, numBytes, "(%p) A", (const void*) stream);

    // Push audio sample into RTP buffer.
    size_t numPayloadBytes;
    HAPRTPStreamPushPayload(rtpStream, bytes, numBytes, &numPayloadBytes, sampleTime, ActualTime());
    if (numPayloadBytes) {
        // Send data if available.
        uint8_t packet[kMaxPacketSize];
        for (;;) {
            size_t length;
            HAPRTPStreamPollPacket(rtpStream, packet, sizeof packet, &length);
            if (length == 0) {
                break;
            }

            SendRTP(currentSession->audioConnection, packet, length);
        }
    }
}

@implementation Session

- (instancetype)init {
    if (self = [super init]) {
        videoConnection = NULL;
        audioConnection = NULL;
    }
    return self;
}

- (void)stop {
    if (videoConnection) {
        nw_connection_cancel(videoConnection);
    }
    if (audioConnection) {
        nw_connection_cancel(audioConnection);
    }
}
@end

static NSData* Snapshot(size_t width, size_t height) {
    static int x = 0;
    HAPLogInfo(&logObject, "%s %d", __func__, x);

    NSString* jpg = [NSString stringWithFormat:@"%s/snapshot%d.jpg", kMediaFilePath, x];
    x = (x + 1) % MAX_SNAPSHOT;

    NSURL* url = [[NSURL alloc] initFileURLWithPath:jpg];
    CGImageSourceRef source = CGImageSourceCreateWithURL((__bridge CFURLRef) url, NULL);
    if (!source) {
        HAPLogError(&logObject, "%s: CGImageSourceCreateWithURL() failed. Could be missing file", __func__);
        return NULL;
    }
    CGImageRef image = CGImageSourceCreateImageAtIndex(source, 0, NULL);
    CFRelease(source);

    CGColorSpaceRef rgbColorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate(NULL, width, height, 8, 0, rgbColorSpace, kCGImageAlphaNoneSkipFirst);
    CGColorSpaceRelease(rgbColorSpace);
    CGContextDrawImage(context, CGRectMake(0, 0, width, height), image);
    CGImageRelease(image);
    CGImageRef output = CGBitmapContextCreateImage(context);
    NSMutableData* data = [[NSMutableData alloc] init];
    CGImageDestinationRef jpeg =
            CGImageDestinationCreateWithData((__bridge CFMutableDataRef) data, kUTTypeJPEG, 1, NULL);
    if (jpeg) {
        if (output) {
            CGImageDestinationAddImage(jpeg, output, NULL);
        }
        CGImageDestinationFinalize(jpeg);
    }
    CGImageRelease(output);
    CGContextRelease(context);
    if (jpeg) {
        CFRelease(jpeg);
    }

    return data;
}

void HAPPlatformCameraCreate(HAPPlatformCameraRef camera, const HAPPlatformCameraOptions* options) {
    HAPPrecondition(camera);
    HAPPrecondition(options);
    HAPPrecondition(options->streamingSessionStorage);
    HAPPrecondition(options->cameraInput);
    HAPPrecondition(options->microphone);
    HAPPrecondition(options->speaker);

    HAPLogInfo(&logObject, "%s", __func__);

    HAPRawBufferZero(camera, sizeof *camera);
    camera->identifier = options->identifier;
    if (!camera->identifier) {
        HAPLog(&logObject, "No identifier set in Camera initialization options. Treating as 1.");
        camera->identifier = 1;
    }

    camera->keyValueStore = options->keyValueStore;
    if (!camera->keyValueStore) {
        HAPLog(&logObject, "No key-value store supplied in Camera initialization options.");
    }

    if (options->interfaceName) {
        size_t numInterfaceNameBytes = HAPStringGetNumBytes(HAPNonnull(options->interfaceName));
        if ((numInterfaceNameBytes == 0) || (numInterfaceNameBytes >= sizeof camera->interfaceName)) {
            HAPLogError(&logObject, "Invalid local network interface name.");
            HAPFatalError();
        }
        HAPRawBufferCopyBytes(camera->interfaceName, HAPNonnull(options->interfaceName), numInterfaceNameBytes);
    }

    camera->streamingSessionStorage = *options->streamingSessionStorage;
    camera->cameraInput = options->cameraInput;
    camera->microphone = options->microphone;
    camera->speaker = options->speaker;

    state.sessions = [[NSMutableArray alloc] init];
    while (state.sessions.count < camera->streamingSessionStorage.numSessions) {
        [state.sessions addObject:[[Session alloc] init]];
    }
}

void HAPPlatformCameraRelease(HAPPlatformCameraRef camera) {
    HAPPrecondition(camera);

    HAPLogInfo(&logObject, "%s", __func__);

    // Assert correct cleanup of streaming sessions.
    for (size_t i = 0; i < camera->streamingSessionStorage.numSessions; i++) {
        HAPAssert(camera->streamingSessionStorage.sessions[i].status != kHAPCameraStreamingStatus_InUse);
    }
}

void HAPPlatformCameraSetDelegate(HAPPlatformCameraRef camera, const HAPPlatformCameraDelegate* _Nullable delegate) {
    HAPPrecondition(camera);

    if (delegate) {
        camera->delegate = *delegate;
    } else {
        HAPRawBufferZero(&camera->delegate, sizeof camera->delegate);
    }
}

HAP_RESULT_USE_CHECK
HAPCameraStreamingStatus HAPPlatformCameraGetStreamStatus(HAPPlatformCameraRef camera, size_t streamIndex) {
    HAPPrecondition(camera);
    HAPPrecondition(streamIndex < camera->streamingSessionStorage.numSessions);

    HAPPlatformCameraStreamingSession* session = &camera->streamingSessionStorage.sessions[streamIndex];
    return session->status;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraTrySetStreamStatus(
        HAPPlatformCameraRef camera,
        size_t streamIndex,
        HAPCameraStreamingStatus status) {
    HAPPrecondition(camera);
    HAPPrecondition(streamIndex < camera->streamingSessionStorage.numSessions);

    HAPPlatformCameraStreamingSession* session = &camera->streamingSessionStorage.sessions[streamIndex];

    if ((status == kHAPCameraStreamingStatus_Unavailable && session->status == kHAPCameraStreamingStatus_InUse) ||
        (status == kHAPCameraStreamingStatus_InUse && session->status == kHAPCameraStreamingStatus_Unavailable)) {
        // Illegal transition.
        HAPLogInfo(&logObject, "%s %d failed", __func__, (int) streamIndex);
        return kHAPError_InvalidState;
    } else if (status != session->status) {
        session->status = status;
        if (camera->delegate.handleStreamStateChanged) {
            camera->delegate.handleStreamStateChanged(camera, streamIndex, camera->delegate.context);
        }
    }
    HAPLogInfo(&logObject, "%s %d succeeded", __func__, (int) streamIndex);

    if (camera->recorderDelegate.handleOperatingModeChanged) {
        camera->recorderDelegate.handleOperatingModeChanged(camera, false);
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraGetStreamingSessionEndpoint(
        HAPPlatformCameraRef camera,
        size_t streamIndex,
        const HAPPlatformCameraIPAddress* controllerAddress,
        HAPPlatformCameraIPAddress* accessoryAddress,
        HAPNetworkPort* videoPort,
        HAPNetworkPort* audioPort) {
    HAPPrecondition(camera);
    HAPPrecondition(streamIndex < camera->streamingSessionStorage.numSessions);
    HAPPrecondition(controllerAddress);
    HAPPrecondition(accessoryAddress);
    HAPPrecondition(videoPort);
    HAPPrecondition(audioPort);

    HAPLogInfo(&logObject, "%s", __func__);

    // IP address.
    accessoryAddress->version = controllerAddress->version;

    // search for best matching address
    struct ifaddrs* ifaddr;
    int max = -1;
    if (getifaddrs(&ifaddr) == 0) {
        for (struct ifaddrs* ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
            char ipString[INET6_ADDRSTRLEN];

            if (ifa->ifa_addr == NULL || (ifa->ifa_name[0] == 'l' && ifa->ifa_name[1] == 'o')) {
                continue;
            }
            if (HAPCameraInterfaceName && !HAPStringAreEqual(HAPCameraInterfaceName, ifa->ifa_name)) {
                continue;
            }
            if (controllerAddress->version == kHAPIPAddressVersion_IPv4 && ifa->ifa_addr->sa_family == AF_INET) {
                getnameinfo(
                        ifa->ifa_addr, sizeof(struct sockaddr_in), ipString, INET_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST);
            } else if (
                    controllerAddress->version == kHAPIPAddressVersion_IPv6 && ifa->ifa_addr->sa_family == AF_INET6) {
                getnameinfo(
                        ifa->ifa_addr,
                        sizeof(struct sockaddr_in6),
                        ipString,
                        INET6_ADDRSTRLEN,
                        NULL,
                        0,
                        NI_NUMERICHOST);
            } else {
                continue;
            }
            // count matching characters
            int n = 0;
            while (ipString[n] == controllerAddress->ipString[n])
                n++;
            // take best match
            if (n > max) {
                HAPRawBufferCopyBytes(accessoryAddress->ipString, ipString, HAPStringGetNumBytes(ipString) + 1);
                max = n;
            }
        }
        freeifaddrs(ifaddr);
    }
    if (max < 0) {
        HAPLogError(&logObject, "No IP Address found, trying localhost");
        HAPRawBufferCopyBytes(accessoryAddress->ipString, "localhost", HAPStringGetNumBytes("localhost") + 1);
    }

    // Ports.
    *videoPort = (uint16_t)(kHAPAccessoryPort_Video + 4 * streamIndex);
    *audioPort = (uint16_t)(kHAPAccessoryPort_Audio + 4 * streamIndex);

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraStartStreamingSession(
        HAPPlatformCameraRef camera,
        size_t streamIndex,
        const HAPPlatformCameraEndpointParameters* controllerEndpoint,
        const HAPPlatformCameraEndpointParameters* accessoryEndpoint,
        const HAPPlatformCameraStartStreamingSessionConfiguration* streamConfiguration) {
    HAPPrecondition(camera);
    HAPPrecondition(streamIndex < camera->streamingSessionStorage.numSessions);
    HAPPrecondition(controllerEndpoint);
    HAPPrecondition(accessoryEndpoint);
    HAPPrecondition(streamConfiguration);

    HAPLogInfo(&logObject, "new %s", __func__);

    HAPPlatformCameraStream* videoStream = &camera->streamingSessionStorage.sessions[streamIndex].video;
    HAPPlatformCameraStream* audioStream = &camera->streamingSessionStorage.sessions[streamIndex].audio;

    HAPRawBufferZero(videoStream, sizeof *videoStream);
    HAPRawBufferZero(audioStream, sizeof *audioStream);
    videoStream->isInitialized = true;
    audioStream->isInitialized = true;
    videoStream->streamIndex = streamIndex;
    audioStream->streamIndex = streamIndex;

    Session* currentSession = GetSession(streamIndex);

    currentSession->videoConnection = CreateConnection(
            &accessoryEndpoint->ipAddress,
            accessoryEndpoint->video.port,
            &controllerEndpoint->ipAddress,
            controllerEndpoint->video.port);

    currentSession->audioConnection = CreateConnection(
            &accessoryEndpoint->ipAddress,
            accessoryEndpoint->audio.port,
            &controllerEndpoint->ipAddress,
            controllerEndpoint->audio.port);

    // Fill in stream CName.
    char cname[INET6_ADDRSTRLEN + 10];
    HAPError err = HAPStringWithFormat(cname, sizeof cname, "IPCamera@%s", accessoryEndpoint->ipAddress.ipString);
    HAPAssert(!err);
    if (err) {
        HAPLogError(&logObject, "Failed to set the name of the device");
        return err;
    }

    // Stream start time.
    HAPEpochTime startTime = ActualTime();

    // Start video RTP stream.
    err = HAPRTPStreamStart(
            &videoStream->rtpStream,
            &streamConfiguration->video.rtpParameters,
            kHAPRTPEncodeType_H264,
            kH264TimestampFrequency, // h264 RTP timestamp clock frequency.
            accessoryEndpoint->video.ssrc,
            startTime,
            cname,
            &controllerEndpoint->video.srtpParameters,
            &accessoryEndpoint->video.srtpParameters);
    if (err) {
        HAPLogError(&logObject, "Unable to start video RTP stream");
        return err;
    }

    // Start audio RTP stream.
    uint32_t timestampFrequency = 16000;
    switch (streamConfiguration->audio.codecParameters.sampleRate) {
        case kHAPAudioCodecSampleRate_8KHz:
            timestampFrequency = 8000;
            break;
        case kHAPAudioCodecSampleRate_16KHz:
            timestampFrequency = 16000;
            break;
        case kHAPAudioCodecSampleRate_24KHz:
            timestampFrequency = 24000;
            break;
    }
    err = HAPRTPStreamStart(
            &audioStream->rtpStream,
            &streamConfiguration->audio.rtpParameters,
            kHAPRTPEncodeType_Simple, // Opus does not need any special payloading.
            timestampFrequency,       // RTP timestamp clock frequency = sample rate.
            accessoryEndpoint->audio.ssrc,
            startTime,
            cname,
            &controllerEndpoint->audio.srtpParameters,
            &accessoryEndpoint->audio.srtpParameters);
    if (err) {
        HAPLogError(&logObject, "Unable to start audio RTP stream");
        return err;
    }

    // Create camera input stream.
    err = HAPPlatformCameraInputStartStream(
            camera->cameraInput,
            &videoStream->cameraInputStream,
            streamConfiguration->video.attributes.width,
            streamConfiguration->video.attributes.height,
            streamConfiguration->video.attributes.maxFrameRate,
            streamConfiguration->video.rtpParameters.maximumBitRate * 1000,
            kVideoKeyframeInterval * 1000,
            streamConfiguration->video.codecParameters.profile,
            streamConfiguration->video.codecParameters.level,
            VideoStreamCallback,
            NULL,
            (void*) videoStream);
    if (err) {
        HAPLogError(&logObject, "Unable to start video stream");
        return err;
    }

    // Create microphone stream.
    err = HAPPlatformMicrophoneStartOpusStream(
            camera->microphone,
            &audioStream->microphoneStream,
            streamConfiguration->audio.codecParameters.sampleRate,
            streamConfiguration->audio.codecParameters.bitRateMode,
            streamConfiguration->audio.rtpParameters.maximumBitRate * 1000,
            streamConfiguration->audio.rtpTime,
            AudioStreamCallback,
            (void*) audioStream);
    if (err) {
        HAPLogError(&logObject, "Unable to start microphone stream");
        return err;
    }

    ReceiveRTP(currentSession->videoConnection, &videoStream->rtpStream, true);
    ReceiveRTP(currentSession->audioConnection, &audioStream->rtpStream, false);

    HAPLogInfo(&logObject, "%s %d", __func__, (int) streamIndex);

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraReconfigureStreamingSession(
        HAPPlatformCameraRef camera,
        size_t streamIndex,
        const HAPPlatformCameraReconfigureStreamingSessionConfiguration* streamConfiguration) {
    // Video only.
    HAPPrecondition(camera);
    HAPPrecondition(streamIndex < camera->streamingSessionStorage.numSessions);
    HAPPrecondition(streamConfiguration);

    // We cannot reconfigure with static video, so just ignore
    HAPLogInfo(&logObject, "%s ignoring reconfigure request for stream: %d", __func__, (int) streamIndex);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraSuspendStreamingSession(HAPPlatformCameraRef camera, size_t streamIndex) {
    HAPPrecondition(camera);
    HAPPrecondition(streamIndex < camera->streamingSessionStorage.numSessions);

    HAPPlatformCameraStream* videoStream = &camera->streamingSessionStorage.sessions[streamIndex].video;
    HAPPlatformCameraStream* audioStream = &camera->streamingSessionStorage.sessions[streamIndex].audio;

    HAPLogInfo(&logObject, "%s %d", __func__, (int) streamIndex);

    if (videoStream->isInitialized) {
        videoStream->isSuspended = true;
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
        HAPPlatformCameraInputSuspendStream(camera->cameraInput, videoStream->cameraInputStream);
    }

    if (audioStream->isInitialized) {
        audioStream->isSuspended = true;
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
        HAPPlatformMicrophoneSuspendStream(camera->microphone, audioStream->microphoneStream);
        HAPPlatformSpeakerSuspendStream(camera->speaker, audioStream->speakerStream);
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraResumeStreamingSession(HAPPlatformCameraRef camera, size_t streamIndex) {
    HAPPrecondition(camera);
    HAPPrecondition(streamIndex < camera->streamingSessionStorage.numSessions);

    HAPLogInfo(&logObject, "%s %d", __func__, (int) streamIndex);

    HAPPlatformCameraStream* videoStream = &camera->streamingSessionStorage.sessions[streamIndex].video;
    HAPPlatformCameraStream* audioStream = &camera->streamingSessionStorage.sessions[streamIndex].audio;

    if (videoStream->isInitialized) {
        videoStream->isSuspended = false;
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
        HAPRTPStreamResetDropoutTimer(&videoStream->rtpStream, ActualTime());
        HAPPlatformCameraInputResumeStream(camera->cameraInput, videoStream->cameraInputStream);
    }

    if (audioStream->isInitialized) {
        audioStream->isSuspended = false;
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
        HAPPlatformMicrophoneResumeStream(camera->microphone, audioStream->microphoneStream);
        HAPPlatformSpeakerResumeStream(camera->speaker, audioStream->speakerStream);
    }

    return kHAPError_None;
}

void HAPPlatformCameraEndStreamingSession(HAPPlatformCameraRef camera, size_t streamIndex) {
    HAPPrecondition(camera);
    HAPPrecondition(streamIndex < camera->streamingSessionStorage.numSessions);

    HAPPlatformCameraStream* videoStream = &camera->streamingSessionStorage.sessions[streamIndex].video;
    HAPPlatformCameraStream* audioStream = &camera->streamingSessionStorage.sessions[streamIndex].audio;

    HAPLogInfo(&logObject, "%s %d", __func__, (int) streamIndex);

    if (videoStream->isInitialized) {
        if (videoStream->isActive) {
            videoStream->isAborted = true;
            videoStream->isSuspended = true;
        }

        if (videoStream->cameraInputStream) {
            HAPLogDebug(&logObject, "stop camera stream");
            HAPPlatformCameraInputStopStream(camera->cameraInput, videoStream->cameraInputStream);
        }

        HAPLogDebug(&logObject, "stop video RTP stream");
        HAPRTPStreamEnd(&videoStream->rtpStream);

        Session* session = GetSession(streamIndex);
        [session stop];

        videoStream->isActive = false;
        videoStream->isInitialized = false;
    }

    if (audioStream->isInitialized) {
        if (audioStream->isActive) {
            audioStream->isAborted = true;
            audioStream->isSuspended = true;
        }

        if (audioStream->microphoneStream) {
            HAPLogDebug(&logObject, "stop microphone stream");
            HAPPlatformMicrophoneStopStream(camera->microphone, audioStream->microphoneStream);
            audioStream->microphoneStream = NULL;
        }

        if (audioStream->speakerStream) {
            HAPLogDebug(&logObject, "stop speaker stream");
            HAPPlatformSpeakerStopStream(camera->speaker, audioStream->speakerStream);
            audioStream->speakerStream = NULL;
        }

        HAPLogDebug(&logObject, "stop audio RTP stream");
        HAPRTPStreamEnd(&audioStream->rtpStream);

        Session* session = GetSession(streamIndex);
        [session stop];

        audioStream->isActive = false;
        audioStream->isInitialized = false;
    }

    if (camera->recorderDelegate.handleResolutionChanged && streamIndex == 0) {
        // Allow adaption of recorder configuration to avoid camera overload.
        // Only needed if the camera input is restricted to two streams.
        HAPError err;
        err = camera->recorderDelegate.handleResolutionChanged(camera, NULL);
        HAPAssert(err == kHAPError_None);
    }
}

typedef struct {
    HAPPlatformCameraSnapshotReader reader;
    size_t offset;
} DarwinSnapshot;

static HAPError GetSize(const HAPPlatformCameraSnapshotReader* reader, size_t* numBytes) {
    const DarwinSnapshot* snapshot = (const DarwinSnapshot*) reader;
    NSData* data = (__bridge NSData*) snapshot->reader.context;
    *numBytes = data.length;
    return kHAPError_None;
}

static HAPError Read(const HAPPlatformCameraSnapshotReader* reader, void* bytes, size_t maxBytes, size_t* numBytes) {
    DarwinSnapshot* snapshot = (DarwinSnapshot*) reader;
    NSData* data = (__bridge NSData*) snapshot->reader.context;
    if (snapshot->offset >= data.length) {
        *numBytes = 0;
        return kHAPError_None;
    }
    size_t remaining = data.length - snapshot->offset;
    size_t n = maxBytes;
    if (n > remaining) {
        n = remaining;
    }
    HAPRawBufferCopyBytes(bytes, (void*) ((uint8_t*) data.bytes + snapshot->offset), n);
    snapshot->offset += n;
    *numBytes = n;
    return kHAPError_None;
}

static void Close(const HAPPlatformCameraSnapshotReader* reader) {
    DarwinSnapshot* snapshot = (DarwinSnapshot*) reader;
    NSData* data HAP_UNUSED = (__bridge_transfer NSData*) snapshot->reader.context;
    free(snapshot);
    data = nil;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraTakeSnapshot(
        HAPPlatformCameraRef camera,
        uint16_t imageWidth,
        uint16_t imageHeight,
        HAPPlatformCameraSnapshotReader** snapshotReader) {
    HAPPrecondition(camera);
    HAPPrecondition(snapshotReader);

    HAPLogInfo(&logObject, "%s: w = %d, h = %d", __func__, imageWidth, imageHeight);

    NSData* data = Snapshot(imageWidth, imageHeight);
    DarwinSnapshot* snapshot = (DarwinSnapshot*) calloc(1, sizeof(DarwinSnapshot));
    snapshot->reader.context = (__bridge_retained void*) data;
    snapshot->reader.getSize = GetSize;
    snapshot->reader.read = Read;
    snapshot->reader.close = Close;
    snapshot->offset = 0;
    *snapshotReader = &snapshot->reader;

    return kHAPError_None;
}

void HAPPlatformCameraSetRecorderDelegate(
        HAPPlatformCameraRef camera,
        const HAPPlatformCameraRecorderDelegate* _Nullable delegate) {
    HAPPrecondition(camera);

    if (delegate) {
        camera->recorderDelegate = *delegate;
    } else {
        HAPRawBufferZero(&camera->recorderDelegate, sizeof camera->recorderDelegate);
    }
}

#endif
