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

// This PAL module is making use of the MMAL library to obtain video data.
// http://www.jvcref.com/files/PI/documentation/html/index.html
// MMAL (Multimedia Abstraction Layer) is a C library designed by Broadcom
// for use with the Videocore IV GPU found on the Raspberry Pi.

/*
 * Pipeline Structure:
 *                       /--------------------------> [H264 encoder] -> callback
 * [camera] -> [splitter] -> [resizer] -- resample -> [H264 encoder] -> callback
 *                       \-> [resizer] -------------> [Jpeg encoder] -> callback
 */

#include <semaphore.h>
#include <sys/time.h>

#include "HAP.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)

#include "HAPPlatformCameraInput+Init.h"

#ifdef RPI
// MMAL Dependency.
// It is not possible to import MMAL without causing warnings. For this reason, they are switched off locally:
HAP_DIAGNOSTIC_PUSH
HAP_DIAGNOSTIC_IGNORED_CLANG("-Wpedantic")
HAP_DIAGNOSTIC_IGNORED_GCC("-Wpedantic")
#include "bcm_host.h"
#include "interface/mmal/mmal_buffer.h"
#include "interface/mmal/mmal_logging.h"
#include "interface/mmal/util/mmal_connection.h"
#include "interface/mmal/util/mmal_default_components.h"
#include "interface/mmal/util/mmal_util.h"
#include "interface/mmal/util/mmal_util_params.h"
HAP_DIAGNOSTIC_POP
#endif

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "CameraInput" };

#define kSnapshotTimeoutSeconds (10)

#define kNativeCameraWidth     (1280)
#define kNativeCameraHeight    (720)
#define kNativeCameraFramerate (30)

#define kH264BufferSize      (1000000)
#define kSnapshotBufferSize  (400000)
#define kNumSnapshotBuffers  (2)
#define kSnapshotJpegQuality (30) // [%]
#define kMaxSnapshotWidth    (1280)
#define kMaxSnapshotHeight   (720)

#define kCameraVideoPort (1)

#define AssertMMALSuccess(r) \
    if (r != MMAL_SUCCESS) { \
        HAPLogError( \
                &logObject, \
                "MMAL operation failed: %s! [%s @ %s:%d]", \
                mmal_status_to_string(r), \
                __func__, \
                HAP_FILE, \
                __LINE__); \
        HAPFatalError(); \
    }

#ifdef RPI

/** Snapshot synchronization. */
static sem_t snapshotSemaphore;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** FreeSafe wrapper for mmal_component_destroy. */
#define MMALComponentFreeSafe(ptr) \
    do { \
        HAPAssert(ptr); \
        mmal_component_destroy(ptr); \
        ptr = NULL; \
    } while (0)

/** FreeSafe wrapper for mmal_connection_destroy. */
#define MMALConnectionFreeSafe(ptr) \
    do { \
        HAPAssert(ptr); \
        mmal_connection_destroy(ptr); \
        ptr = NULL; \
    } while (0)

/** FreeSafe wrapper for mmal_pool_destroy. */
#define MMALPoolFreeSafe(ptr) \
    do { \
        HAPAssert(ptr); \
        mmal_pool_destroy(ptr); \
        ptr = NULL; \
    } while (0)

/** FreeSafe wrapper for mmal_buffer_header_release. */
#define MMALBufferFreeSafe(ptr) \
    do { \
        HAPAssert(ptr); \
        mmal_buffer_header_release(ptr); \
        ptr = NULL; \
    } while (0)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static size_t SkipNALUnitPrefix(const uint8_t* bytes, size_t numBytes) {
    // To allow separation of multiple NAL units, the NAL units are prefixed
    // by a Start Code Prefix consisting of the 4 bytes 0x00, 0x00, 0x00, 0x01.
    // See "ITU-T H.264", Annex B.
    size_t index = 0, numZero = 0;
    while (numBytes) {
        uint8_t byte = bytes[index];
        index++;
        numBytes--;
        if (byte == 0) {
            numZero++;
        } else {
            if (byte == 1 && numZero == 3) {
                return index;
            }
            numZero = 0;
        }
    }
    return index;
}

static void VideoCallback(MMAL_PORT_T* port, MMAL_BUFFER_HEADER_T* buffer) {
    HAPPrecondition(port);
    HAPPrecondition(buffer);
    HAPPrecondition(port->userdata);

    HAPPlatformCameraInputStream* stream = (HAPPlatformCameraInputStream*) port->userdata;

    if (buffer->cmd == 0) {
        size_t numBytes = buffer->length;
        if (numBytes) {
            (void) mmal_buffer_header_mem_lock(buffer);
            uint8_t* bytes = buffer->data;

            uint64_t ts = (uint64_t) buffer->pts; // Stream time in us.
            if ((int64_t) ts != MMAL_TIME_UNKNOWN && ts != 0) {
                if (stream->startTime == 0) {
                    stream->startTime = ts;
                }
                ts -= stream->startTime;
                ts *= 1000; // us -> ns.
            } else {
                ts = UINT64_MAX;
            }

            size_t startIndex = SkipNALUnitPrefix(bytes, numBytes);
            bytes += startIndex;
            numBytes -= startIndex;

            if (buffer->flags & MMAL_BUFFER_HEADER_FLAG_CONFIG) {
                // Contains sps and pps.
                // Search for second nal unit.
                startIndex = SkipNALUnitPrefix(bytes, numBytes);
                // Push sps.
                stream->callback(stream->context, HAPNonnull(stream->cameraInput), stream, bytes, startIndex - 4, ts);
                // Push pps.
                stream->callback(
                        stream->context,
                        HAPNonnull(stream->cameraInput),
                        stream,
                        bytes + startIndex,
                        numBytes - startIndex,
                        ts);
            } else {
                // Frame must not be fragmented.
                HAPAssert(buffer->flags & MMAL_BUFFER_HEADER_FLAG_FRAME_END);
                // Push video frame.
                stream->callback(stream->context, HAPNonnull(stream->cameraInput), stream, bytes, numBytes, ts);
            }

            mmal_buffer_header_mem_unlock(buffer);
        }
    }

    // Cleanup and recycle buffer.
    mmal_buffer_header_release(buffer);
    if (stream->pool && port->is_enabled) {
        MMAL_BUFFER_HEADER_T* _Nullable buf = mmal_queue_get(stream->pool->queue);
        if (buf) {
            // mmal_port_send_buffer may fail if port is closed simultaneously.
            (void) mmal_port_send_buffer(port, buf);
        }
    }
}

static void SnapshotCallback(MMAL_PORT_T* port, MMAL_BUFFER_HEADER_T* buffer) {
    HAPPrecondition(port);
    HAPPrecondition(buffer);
    HAPPrecondition(port->userdata);

    HAPPlatformCameraSnapshot* snapshot = (HAPPlatformCameraSnapshot*) port->userdata;

    if (buffer->cmd == 0 && buffer->length) {
        HAPAssert(buffer->flags & MMAL_BUFFER_HEADER_FLAG_FRAME_END);

        // The encoder output port must not be disabled inside its callback.
        // Disable the encoder input connection instead.
        MMAL_PORT_T* inPort = port->component->input[0];
        MMAL_CONNECTION_T* _Nullable connection = (MMAL_CONNECTION_T*) inPort->userdata;
        if (connection && connection->is_enabled) {
            (void) mmal_connection_disable(connection);
        }

        if (snapshot->buffer == NULL) {
            snapshot->buffer = buffer;
            int res = sem_post(&snapshotSemaphore);
            HAPAssert(res == 0);

            // Buffer is released in SnapshotReader.
            return;
        }
    }

    // Cleanup and recycle buffer.
    mmal_buffer_header_release(buffer);
    if (snapshot->pool && port->is_enabled) {
        MMAL_BUFFER_HEADER_T* _Nullable buf = mmal_queue_get(snapshot->pool->queue);
        if (buf) {
            // mmal_port_send_buffer may fail if port is closed simultaneously.
            (void) mmal_port_send_buffer(port, buf);
        }
    }
}

// MMAL connection callback for frame rate downsampling.
static void ResamplingCallback(MMAL_CONNECTION_T* connection) {
    HAPPrecondition(connection);
    HAPPrecondition(connection->queue);

    // Get source buffer.
    MMAL_BUFFER_HEADER_T* _Nullable buffer = mmal_queue_get(connection->queue);
    if (buffer) {
        HAPPrecondition(connection->user_data);
        HAPPlatformCameraInputStream* stream = (HAPPlatformCameraInputStream*) connection->user_data;
        // Discard selected buffers to get desired frame rate.
        if (stream->balance >= 0) {
            // Pass buffer to destination.
            if (connection->in->is_enabled) {
                // mmal_port_send_buffer may fail if port is closed simultaneously.
                (void) mmal_port_send_buffer(connection->in, buffer);
            }
            stream->balance -= kNativeCameraFramerate;
        } else {
            // Discard buffer.
            mmal_buffer_header_release(buffer);
        }
        stream->balance += stream->framerate;
    }

    // Get free buffer from destination.
    HAPPrecondition(connection->pool);
    buffer = mmal_queue_get(connection->pool->queue);
    if (buffer && connection->out->is_enabled) {
        // Send buffer back to source.
        // mmal_port_send_buffer may fail if port is closed simultaneously.
        (void) mmal_port_send_buffer(connection->out, buffer);
    }
}

static void ControlCallback(MMAL_PORT_T* port, MMAL_BUFFER_HEADER_T* buffer) {
    HAPPrecondition(port);
    HAPPrecondition(buffer);

    mmal_buffer_header_release(buffer);
}

static void SetPortFormat(MMAL_PORT_T* port, int width, int height, int framerate, MMAL_FOURCC_T encoding) {
    HAPPrecondition(port);

    MMAL_ES_FORMAT_T* format = port->format;
    format->encoding = encoding;
    format->encoding_variant = MMAL_ENCODING_I420;

    // Align width and height to video block size (32 x 16).
    int alignedW = (width + 31) & ~31;
    int alignedH = (height + 15) & ~15;

    HAPAssert(format->type == MMAL_ES_TYPE_VIDEO);

    MMAL_ES_SPECIFIC_FORMAT_T* es = format->es;
    es->video.width = alignedW;
    es->video.height = alignedH;
    es->video.crop.width = width;
    es->video.crop.height = height;

    if (framerate) {
        es->video.frame_rate.num = framerate;
        es->video.frame_rate.den = 1;
    }

    MMAL_STATUS_T res;
    res = mmal_port_format_commit(port);
    AssertMMALSuccess(res);
}

static void ConnectPorts(MMAL_PORT_T* sourcePort, MMAL_PORT_T* destinationPort) {
    MMAL_CONNECTION_T* connection;
    MMAL_STATUS_T res;
    res = mmal_connection_create(
            &connection,
            sourcePort,
            destinationPort,
            MMAL_CONNECTION_FLAG_TUNNELLING | MMAL_CONNECTION_FLAG_ALLOCATION_ON_INPUT);
    AssertMMALSuccess(res);

    res = mmal_connection_enable(connection);
    AssertMMALSuccess(res);
}

static void ConnectPortsWithResampling(
        MMAL_PORT_T* sourcePort,
        MMAL_PORT_T* destinationPort,
        int framerate,
        HAPPlatformCameraInputStream* stream) {
    MMAL_CONNECTION_T* connection;
    MMAL_STATUS_T res;
    res = mmal_connection_create(&connection, sourcePort, destinationPort, MMAL_CONNECTION_FLAG_ALLOCATION_ON_INPUT);
    AssertMMALSuccess(res);

    stream->framerate = framerate;
    stream->balance = 0;
    connection->user_data = stream;
    connection->callback = ResamplingCallback;
    res = mmal_port_parameter_set_boolean(sourcePort, MMAL_PARAMETER_ZERO_COPY, 1);
    AssertMMALSuccess(res);
    res = mmal_port_parameter_set_boolean(destinationPort, MMAL_PARAMETER_ZERO_COPY, 1);
    AssertMMALSuccess(res);

    res = mmal_connection_enable(connection);
    AssertMMALSuccess(res);

    // Initialize buffers.
    HAPAssert(connection->pool);
    HAPAssert(connection->pool->queue);
    MMAL_BUFFER_HEADER_T* _Nullable buffer;
    while ((buffer = mmal_queue_get(connection->pool->queue))) {
        res = mmal_port_send_buffer(connection->out, buffer);
        AssertMMALSuccess(res);
    }
}

static void SetupCamera(HAPPlatformCameraInput* state) {
    HAPPrecondition(state);

    state->cameraHW = NULL;
    state->splitter = NULL;

    /* ********* Create camera ********* */

    MMAL_STATUS_T res;
    MMAL_COMPONENT_T* camera;
    res = mmal_component_create("vc.ril.camera", &camera);
    if (res) {
        HAPLogError(&logObject, "Camera access failed (%s)", mmal_status_to_string(res));
        return;
    }

    state->cameraHW = camera;

    HAPAssert(camera->output_num >= 3);

    res = mmal_port_enable(camera->control, ControlCallback);
    AssertMMALSuccess(res);

    // Configure camera.
    MMAL_PARAMETER_CAMERA_CONFIG_T cam_config = {
        { MMAL_PARAMETER_CAMERA_CONFIG, sizeof(cam_config) },
        .max_stills_w = state->width,
        .max_stills_h = state->height,
        .stills_yuv422 = 0,
        .one_shot_stills = 0,
        .max_preview_video_w = state->width,
        .max_preview_video_h = state->height,
        .num_preview_video_frames = kNativeCameraFramerate > 30 ? kNativeCameraFramerate / 10 : 3,
        .stills_capture_circular_buffer_height = 0,
        .fast_preview_resume = 0,
        .use_stc_timestamp = MMAL_PARAM_TIMESTAMP_MODE_RAW_STC,
    };
    res = mmal_port_parameter_set(camera->control, &cam_config.hdr);
    AssertMMALSuccess(res);

    // Setup camera output port.
    SetPortFormat(
            camera->output[kCameraVideoPort],
            state->width,
            state->height,
            kNativeCameraFramerate,
            MMAL_ENCODING_OPAQUE);

    res = mmal_component_enable(camera);
    AssertMMALSuccess(res);

    /* ********* Create splitter ********* */

    MMAL_COMPONENT_T* splitter;
    res = mmal_component_create("vc.ril.video_splitter", &splitter);
    AssertMMALSuccess(res);

    state->splitter = splitter;

    HAPAssert(splitter->input_num == 1);
    HAPAssert(splitter->output_num >= 4);

    res = mmal_component_enable(splitter);
    AssertMMALSuccess(res);

    ConnectPorts(camera->output[kCameraVideoPort], splitter->input[0]);

    // Enable camera port.
    res = mmal_port_parameter_set_boolean(camera->output[kCameraVideoPort], MMAL_PARAMETER_CAPTURE, 1);
    AssertMMALSuccess(res);
}

static void RemoveCamera(HAPPlatformCameraInput* state) {
    HAPPrecondition(state);

    if (state->cameraHW) {
        (void) mmal_port_parameter_set_boolean(state->cameraHW->output[kCameraVideoPort], MMAL_PARAMETER_CAPTURE, 0);
    }

    if (state->splitter) {
        MMAL_CONNECTION_T* connection;
        connection = (MMAL_CONNECTION_T*) state->splitter->input[0]->userdata;
        if (connection) {
            MMALConnectionFreeSafe(connection);
        }
        MMALComponentFreeSafe(state->splitter);
    }

    if (state->cameraHW) {
        MMALComponentFreeSafe(state->cameraHW);
    }
}

HAP_RESULT_USE_CHECK
static HAPError GetFreeSplitterPort(const HAPPlatformCameraInput* cameraInput, MMAL_PORT_T** port_) {
    HAPPrecondition(cameraInput);

    *port_ = NULL;
    MMAL_COMPONENT_T* splitter = cameraInput->splitter;

    // Splitter port 0 is reserved for opaque connections.
    uint32_t i;
    for (i = 1; i < splitter->output_num; i++) {
        MMAL_PORT_T* port = splitter->output[i];
        if (!port->is_enabled) {
            *port_ = port;
            return kHAPError_None;
        }
    }

    HAPLogInfo(&logObject, "No free splitter port found");
    return kHAPError_OutOfResources;
}

static void RemoveVideoPipeline(HAPPlatformCameraInputStream* stream);

HAP_RESULT_USE_CHECK
static HAPError AddVideoPipeline(
        HAPPlatformCameraInputStream* stream,
        const HAPPlatformCameraInput* cameraInput,
        uint32_t width,
        uint32_t height,
        int framerate,
        int bitrate,
        int keyFrameInterval, // [frames]
        MMAL_VIDEO_PROFILE_T profile,
        MMAL_VIDEO_LEVEL_T level) {
    HAPPrecondition(stream);
    HAPPrecondition(cameraInput);

    stream->encoder = NULL;
    stream->resizer = NULL;
    stream->pool = NULL;

    if (cameraInput->splitter == NULL) {
        return kHAPError_OutOfResources;
    }

    /* ********* Create video encoder ********* */

    MMAL_STATUS_T res;
    MMAL_COMPONENT_T* encoder;
    res = mmal_component_create("vc.ril.video_encode", &encoder);
    AssertMMALSuccess(res);

    stream->encoder = encoder;

    HAPAssert(encoder->input_num == 1);
    HAPAssert(encoder->output_num >= 1);

    MMAL_PORT_T* inPort = encoder->input[0];
    MMAL_PORT_T* outPort = encoder->output[0];

    outPort->format->bitrate = bitrate;
    SetPortFormat(outPort, width, height, framerate, MMAL_ENCODING_H264);

    MMAL_PARAMETER_VIDEO_PROFILE_T h264_profile = { { MMAL_PARAMETER_PROFILE, sizeof h264_profile },
                                                    { { profile, level } } };
    res = mmal_port_parameter_set(outPort, &h264_profile.hdr);
    AssertMMALSuccess(res);

    // Generate sps/pps for each keyframe.
    res = mmal_port_parameter_set_boolean(outPort, MMAL_PARAMETER_VIDEO_ENCODE_INLINE_HEADER, 1);
    AssertMMALSuccess(res);

    // Keyframe interval.
    res = mmal_port_parameter_set_uint32(outPort, MMAL_PARAMETER_INTRAPERIOD, keyFrameInterval);
    AssertMMALSuccess(res);

    // Set variable rate control.
    MMAL_PARAMETER_VIDEO_RATECONTROL_T rate_control = { { MMAL_PARAMETER_RATECONTROL, sizeof rate_control },
                                                        MMAL_VIDEO_RATECONTROL_VARIABLE };
    res = mmal_port_parameter_set(outPort, &rate_control.hdr);
    AssertMMALSuccess(res);

    res = mmal_component_enable(encoder);
    AssertMMALSuccess(res);

    MMAL_PORT_T* sourcePort;
    if (width != cameraInput->width || height != cameraInput->height) {
        /* ********* Create video resizer ********* */

        HAPError err;
        err = GetFreeSplitterPort(cameraInput, &sourcePort);
        if (err) {
            RemoveVideoPipeline(stream);
            return err;
        }

        MMAL_COMPONENT_T* resizer;
        res = mmal_component_create("vc.ril.isp", &resizer);
        AssertMMALSuccess(res);
        stream->resizer = resizer;

        HAPAssert(resizer->input_num == 1);
        HAPAssert(resizer->output_num >= 1);

        res = mmal_component_enable(resizer);
        AssertMMALSuccess(res);

        SetPortFormat(resizer->output[0], width, height, framerate, MMAL_ENCODING_I420);

        ConnectPorts(resizer->output[0], inPort);
        SetPortFormat(sourcePort, cameraInput->width, cameraInput->height, kNativeCameraFramerate, MMAL_ENCODING_I420);

        inPort = resizer->input[0];
    } else {
        // Direct connection from splitter to h264 allows opaque buffers.
        // Opaque and non-opaque ports cannot be mixed up.
        // Use port 0 for opaque connections if possible.
        sourcePort = cameraInput->splitter->output[0];
        if (!sourcePort->is_enabled) {
            SetPortFormat(sourcePort, width, height, kNativeCameraFramerate, MMAL_ENCODING_OPAQUE);
        } else {
            // Fallback: use non-opaque.
            HAPError err;
            err = GetFreeSplitterPort(cameraInput, &sourcePort);
            if (err) {
                RemoveVideoPipeline(stream);
                return err;
            }
            SetPortFormat(sourcePort, width, height, kNativeCameraFramerate, MMAL_ENCODING_I420);
        }
    }

    if (framerate < kNativeCameraFramerate) {
        // Use frame rate resampling.
        ConnectPortsWithResampling(sourcePort, inPort, framerate, stream);
    } else {
        ConnectPorts(sourcePort, inPort);
    }

    /* ********* Create buffer pool ********* */

    if (outPort->buffer_size < kH264BufferSize) {
        outPort->buffer_size = kH264BufferSize;
    }
    MMAL_POOL_T* pool = mmal_port_pool_create(outPort, outPort->buffer_num, outPort->buffer_size);
    HAPAssert(pool);

    stream->pool = pool;
    outPort->userdata = (void*) stream;

    return kHAPError_None;
}

static void RemoveVideoPipeline(HAPPlatformCameraInputStream* stream) {
    HAPPrecondition(stream);

    if (stream->encoder) {
        MMAL_PORT_T* port = stream->encoder->output[0];
        if (port->is_enabled) {
            (void) mmal_port_disable(port);
        }

        if (stream->pool) {
            MMALPoolFreeSafe(stream->pool);
        }

        MMAL_CONNECTION_T* connection = (MMAL_CONNECTION_T*) stream->encoder->input[0]->userdata;
        if (connection) {
            MMALConnectionFreeSafe(connection);
        }
        MMALComponentFreeSafe(stream->encoder);
    }

    if (stream->resizer) {
        MMAL_CONNECTION_T* connection = (MMAL_CONNECTION_T*) stream->resizer->input[0]->userdata;
        if (connection) {
            MMALConnectionFreeSafe(connection);
        }
        MMALComponentFreeSafe(stream->resizer);
    }
}

HAP_RESULT_USE_CHECK
static HAPError AddSnapshotPipeline(
        HAPPlatformCameraSnapshot* snapshot,
        const HAPPlatformCameraInput* cameraInput,
        int width,
        int height) {
    HAPPrecondition(snapshot);
    HAPPrecondition(cameraInput);

    snapshot->encoder = NULL;
    snapshot->resizer = NULL;
    snapshot->pool = NULL;
    snapshot->buffer = NULL;

    if (cameraInput->splitter == NULL) {
        return kHAPError_OutOfResources;
    }

    MMAL_PORT_T* sourcePort;
    HAPError err;
    err = GetFreeSplitterPort(cameraInput, &sourcePort);
    if (err) {
        return err;
    }

    /* ********* Create image encoder ********* */

    MMAL_STATUS_T res;
    MMAL_COMPONENT_T* encoder;
    res = mmal_component_create("vc.ril.image_encode", &encoder);
    AssertMMALSuccess(res);

    snapshot->encoder = encoder;

    HAPAssert(encoder->input_num == 1);
    HAPAssert(encoder->output_num >= 1);
    MMAL_PORT_T* inPort = encoder->input[0];
    MMAL_PORT_T* outPort = encoder->output[0];

    SetPortFormat(outPort, width, height, 0, MMAL_ENCODING_JPEG);
    if (outPort->buffer_num < kNumSnapshotBuffers) {
        outPort->buffer_num = kNumSnapshotBuffers;
    }

    // Jpeg quality.
    res = mmal_port_parameter_set_uint32(outPort, MMAL_PARAMETER_JPEG_Q_FACTOR, kSnapshotJpegQuality);
    AssertMMALSuccess(res);

    res = mmal_component_enable(encoder);
    AssertMMALSuccess(res);

    /* ********* Create video resizer ********* */

    MMAL_COMPONENT_T* resizer;
    res = mmal_component_create("vc.ril.isp", &resizer);
    AssertMMALSuccess(res);
    snapshot->resizer = resizer;

    HAPAssert(resizer->input_num == 1);
    HAPAssert(resizer->output_num >= 1);

    res = mmal_component_enable(resizer);
    AssertMMALSuccess(res);

    SetPortFormat(sourcePort, cameraInput->width, cameraInput->height, 0, MMAL_ENCODING_I420);

    ConnectPorts(sourcePort, resizer->input[0]);

    // If camera is configured in portrait mode and snapshot request is in landscape mode or vice versa,
    // flip the resizer output port resolution to avoid stretching of image
    bool isCameraConfiguredPortrait = cameraInput->width < cameraInput->height;
    bool isSnapshotRequestPortrait = width < height;
    if (isCameraConfiguredPortrait == isSnapshotRequestPortrait) {
        SetPortFormat(resizer->output[0], width, height, 0, MMAL_ENCODING_I420);
    } else {
        SetPortFormat(resizer->output[0], height, width, 0, MMAL_ENCODING_I420);
    }

    ConnectPorts(resizer->output[0], inPort);

    /* ********* Create buffer pool and enable ********* */

    if (outPort->buffer_size < kSnapshotBufferSize) {
        outPort->buffer_size = kSnapshotBufferSize;
    }

    MMAL_POOL_T* pool = mmal_port_pool_create(outPort, outPort->buffer_num, outPort->buffer_size);
    HAPAssert(pool);

    snapshot->pool = pool;
    outPort->userdata = (void*) snapshot;

    res = mmal_port_enable(outPort, SnapshotCallback);
    AssertMMALSuccess(res);

    MMAL_BUFFER_HEADER_T* _Nullable buffer;
    while ((buffer = mmal_queue_get(pool->queue))) {
        res = mmal_port_send_buffer(outPort, buffer);
        AssertMMALSuccess(res);
    }

    return kHAPError_None;
}

static void RemoveSnapshotPipeline(HAPPlatformCameraSnapshot* snapshot) {
    HAPPrecondition(snapshot);

    if (snapshot->encoder) {
        MMAL_PORT_T* port = snapshot->encoder->output[0];
        if (port->is_enabled) {
            (void) mmal_port_disable(port);
        }

        MMAL_CONNECTION_T* connection = (MMAL_CONNECTION_T*) snapshot->encoder->input[0]->userdata;
        if (connection) {
            MMALConnectionFreeSafe(connection);
        }
        MMALComponentFreeSafe(snapshot->encoder);
    }

    if (snapshot->resizer) {
        MMAL_CONNECTION_T* connection = (MMAL_CONNECTION_T*) snapshot->resizer->input[0]->userdata;
        if (connection) {
            MMALConnectionFreeSafe(connection);
        }
        MMALComponentFreeSafe(snapshot->resizer);
    }
}

static void RemoveSnapshotPool(HAPPlatformCameraSnapshot* snapshot) {
    HAPPrecondition(snapshot);

    if (snapshot->buffer) {
        // Cleanup and recycle buffer.
        MMALBufferFreeSafe(snapshot->buffer);
    }

    if (snapshot->pool) {
        MMALPoolFreeSafe(snapshot->pool);
    }
}

#endif

void HAPPlatformCameraInputCreate(HAPPlatformCameraInputRef cameraInput, const HAPPlatformCameraInputOptions* options) {
    HAPPrecondition(cameraInput);
    HAPPrecondition(options);
    HAPPrecondition(options->cameraInputStreams);
    HAPPrecondition(options->cameraSnapshots);

    for (size_t i = 0; i < options->numCameraInputStreams; i++) {
        HAPPlatformCameraInputStreamRef cameraInputStream = &options->cameraInputStreams[i];
        HAPRawBufferZero(cameraInputStream, sizeof *cameraInputStream);
    }

    for (size_t i = 0; i < options->numCameraSnapshots; i++) {
        HAPPlatformCameraSnapshotRef cameraSnapshot = &options->cameraSnapshots[i];
        HAPRawBufferZero(cameraSnapshot, sizeof *cameraSnapshot);
    }

#ifdef RPI
    // Initialize MMAL.
    bcm_host_init();

    HAPRawBufferZero(cameraInput, sizeof *cameraInput);
    cameraInput->cameraInputStreams = options->cameraInputStreams;
    cameraInput->numCameraInputStreams = options->numCameraInputStreams;
    cameraInput->cameraSnapshots = options->cameraSnapshots;
    cameraInput->numCameraSnapshots = options->numCameraSnapshots;
    cameraInput->width = kNativeCameraWidth;
    cameraInput->height = kNativeCameraHeight;

    if (options->isPortraitMode) {
        cameraInput->width = kNativeCameraHeight;
        cameraInput->height = kNativeCameraWidth;
    }

    // Create camera components.
    SetupCamera(cameraInput);

    int res = sem_init(&snapshotSemaphore, 0, 0);
    HAPAssert(res == 0);
#endif
}

void HAPPlatformCameraInputRelease(HAPPlatformCameraInputRef cameraInput) {
    HAPPrecondition(cameraInput);

    for (size_t i = 0; i < cameraInput->numCameraInputStreams; i++) {
        HAPPlatformCameraInputStreamRef cameraInputStream = &cameraInput->cameraInputStreams[i];
        if (cameraInputStream->cameraInput) {
            HAPLogError(
                    &logObject,
                    "Camera input stream %p not stopped before releasing camera.",
                    (const void*) cameraInputStream);
            HAPFatalError();
        }
        HAPRawBufferZero(cameraInputStream, sizeof *cameraInputStream);
    }

#ifdef RPI
    int res = sem_destroy(&snapshotSemaphore);
    HAPAssert(res == 0);

    RemoveCamera(cameraInput);

    HAPRawBufferZero(cameraInput, sizeof *cameraInput);
#endif
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraInputStartStream(
        HAPPlatformCameraInputRef cameraInput,
        HAPPlatformCameraInputStreamRef _Nullable* _Nonnull cameraInputStream,
        uint32_t width HAP_UNUSED,
        uint32_t height HAP_UNUSED,
        uint32_t framerate HAP_UNUSED,
        uint32_t bitrate HAP_UNUSED,
        uint32_t keyFrameInterval HAP_UNUSED,
        HAPH264VideoCodecProfile profile HAP_UNUSED,
        HAPH264VideoCodecProfileLevel level HAP_UNUSED,
        HAPPlatformCameraInputDataCallback callback,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(cameraInput);
    HAPPrecondition(cameraInputStream);
    HAPPrecondition(callback);
    HAPPrecondition(cameraInput->cameraInputStreams);

    // Find free camera input stream.
    *cameraInputStream = NULL;
    for (size_t i = 0; i < cameraInput->numCameraInputStreams; i++) {
        HAPPlatformCameraInputStreamRef stream = &cameraInput->cameraInputStreams[i];
        if (!stream->cameraInput) {
            HAPRawBufferZero(stream, sizeof *stream);
            stream->cameraInput = cameraInput;
            *cameraInputStream = stream;
            break;
        }
    }
    if (!*cameraInputStream) {
        HAPLogError(
                &logObject,
                "[%p] No additional concurrent camera input streams may be started.",
                (const void*) cameraInput);
        return kHAPError_OutOfResources;
    }

#ifdef RPI
    MMAL_VIDEO_PROFILE_T h264Profile = MMAL_VIDEO_PROFILE_H264_MAIN;
    switch (profile) {
        case kHAPH264VideoCodecProfile_ConstrainedBaseline: {
            h264Profile = MMAL_VIDEO_PROFILE_H264_CONSTRAINED_BASELINE;
            break;
        }
        case kHAPH264VideoCodecProfile_Main: {
            h264Profile = MMAL_VIDEO_PROFILE_H264_MAIN;
            break;
        }
        case kHAPH264VideoCodecProfile_High: {
            h264Profile = MMAL_VIDEO_PROFILE_H264_HIGH;
            break;
        }
    }

    MMAL_VIDEO_LEVEL_T h264Level = MMAL_VIDEO_LEVEL_H264_4;
    switch (level) {
        case kHAPH264VideoCodecProfileLevel_3_1: {
            h264Level = MMAL_VIDEO_LEVEL_H264_31;
            break;
        }
        case kHAPH264VideoCodecProfileLevel_3_2: {
            h264Level = MMAL_VIDEO_LEVEL_H264_32;
            break;
        }
        case kHAPH264VideoCodecProfileLevel_4: {
            h264Level = MMAL_VIDEO_LEVEL_H264_4;
            break;
        }
    }

    // Create video stream.
    HAPError err;
    err = AddVideoPipeline(
            *cameraInputStream,
            cameraInput,
            width,
            height,
            framerate,
            bitrate,
            keyFrameInterval * framerate / 1000, // ms -> frames
            h264Profile,
            h264Level);
    if (err) {
        return err;
    }

    // Actual start time is set in callback.
    (*cameraInputStream)->startTime = 0;
    (*cameraInputStream)->callback = callback;
    (*cameraInputStream)->context = context;

    HAPPlatformCameraInputResumeStream(cameraInput, *cameraInputStream);
#endif

    return kHAPError_None;
}

void HAPPlatformCameraInputStopStream(
        HAPPlatformCameraInputRef cameraInput,
        HAPPlatformCameraInputStreamRef cameraInputStream) {
    HAPPrecondition(cameraInput);
    HAPPrecondition(cameraInputStream);

#ifdef RPI
    RemoveVideoPipeline(cameraInputStream);

    cameraInputStream->cameraInput = NULL;
#endif
}

void HAPPlatformCameraInputSuspendStream(
        HAPPlatformCameraInputRef cameraInput,
        HAPPlatformCameraInputStreamRef cameraInputStream) {
    HAPPrecondition(cameraInput);
    HAPPrecondition(cameraInputStream);

#ifdef RPI
    if (cameraInputStream->encoder) {
        MMAL_PORT_T* port = cameraInputStream->encoder->output[0];
        if (port->is_enabled) {
            MMAL_STATUS_T res;
            res = mmal_port_disable(port);
            AssertMMALSuccess(res);
        }
    }
#endif
}

void HAPPlatformCameraInputResumeStream(
        HAPPlatformCameraInputRef cameraInput HAP_UNUSED,
        HAPPlatformCameraInputStreamRef cameraInputStream HAP_UNUSED) {
#ifdef RPI
    HAPPrecondition(cameraInput);
    HAPPrecondition(cameraInputStream);
    HAPPrecondition(cameraInputStream->encoder);
    HAPPrecondition(cameraInputStream->pool);

    MMAL_PORT_T* outPort = cameraInputStream->encoder->output[0];

    MMAL_STATUS_T res;
    res = mmal_port_enable(outPort, VideoCallback);
    AssertMMALSuccess(res);

    MMAL_POOL_T* pool = cameraInputStream->pool;
    MMAL_BUFFER_HEADER_T* _Nullable buffer;
    while ((buffer = mmal_queue_get(pool->queue))) {
        res = mmal_port_send_buffer(outPort, buffer);
        AssertMMALSuccess(res);
    }
#endif
}

HAP_RESULT_USE_CHECK
bool HAPPlatformCameraInputIsStreamRunning(
        HAPPlatformCameraInputStreamRef cameraInputStream HAP_UNUSED,
        const char* _Nullable* _Nonnull stateString) {
    *stateString = "Unknown";
#ifdef RPI
    HAPPrecondition(cameraInputStream);

    if (cameraInputStream->encoder && cameraInputStream->encoder->output[0]) {
        MMAL_PORT_T* outPort = cameraInputStream->encoder->output[0];
        *stateString = outPort->is_enabled ? "Enabled" : "Disabled";
        return (outPort->is_enabled != 0);
    }
#endif
    return false;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraInputReconfigureStream(
        HAPPlatformCameraInputRef cameraInput HAP_UNUSED,
        HAPPlatformCameraInputStreamRef cameraInputStream HAP_UNUSED,
        uint32_t width HAP_UNUSED,
        uint32_t height HAP_UNUSED,
        uint32_t framerate HAP_UNUSED,
        uint32_t bitrate HAP_UNUSED) {
#ifdef RPI
    HAPPrecondition(cameraInput);
    HAPPrecondition(cameraInputStream);
    HAPPrecondition(cameraInputStream->encoder);

    // Get current profile/level.
    MMAL_PARAMETER_VIDEO_PROFILE_T parameter = { { MMAL_PARAMETER_PROFILE, sizeof(parameter) }, { { 0, 0 } } };
    MMAL_STATUS_T res;
    res = mmal_port_parameter_get(cameraInputStream->encoder->output[0], &parameter.hdr);
    AssertMMALSuccess(res);

    // Get current key frame interval.
    uint32_t keyFrameInterval;
    res = mmal_port_parameter_get_uint32(
            cameraInputStream->encoder->output[0], MMAL_PARAMETER_INTRAPERIOD, &keyFrameInterval);
    AssertMMALSuccess(res);

    RemoveVideoPipeline(cameraInputStream);

    HAPError err;
    err = AddVideoPipeline(
            cameraInputStream,
            cameraInput,
            width,
            height,
            framerate,
            bitrate,
            keyFrameInterval,
            parameter.profile[0].profile,
            parameter.profile[0].level);
    if (err) {
        return err;
    }

    // Restart stream.
    HAPPlatformCameraInputResumeStream(cameraInput, cameraInputStream);
#endif

    return kHAPError_None;
}

void HAPPlatformCameraInputSetBitrate(
        HAPPlatformCameraInputRef cameraInput,
        HAPPlatformCameraInputStreamRef cameraInputStream,
        uint32_t bitrate HAP_UNUSED) {
    HAPPrecondition(cameraInput);
    HAPPrecondition(cameraInputStream);

#ifdef RPI
    if (cameraInputStream->encoder) {
        MMAL_STATUS_T res;
        res = mmal_port_parameter_set_uint32(
                cameraInputStream->encoder->output[0], MMAL_PARAMETER_VIDEO_BIT_RATE, bitrate);
        AssertMMALSuccess(res);
    } else {
        HAPLogError(&logObject, "Set Bitrate on a closed stream");
    }
#endif
}

void HAPPlatformCameraInputRequestKeyFrame(
        HAPPlatformCameraInputRef cameraInput,
        HAPPlatformCameraInputStreamRef cameraInputStream) {
    HAPPrecondition(cameraInput);
    HAPPrecondition(cameraInputStream);

#ifdef RPI
    if (cameraInputStream->encoder) {
        MMAL_STATUS_T res;
        res = mmal_port_parameter_set_boolean(
                cameraInputStream->encoder->output[0], MMAL_PARAMETER_VIDEO_REQUEST_I_FRAME, 1);
        AssertMMALSuccess(res);
    } else {
        HAPLogError(&logObject, "Keyframe request on a closed stream");
    }
#endif
}

#ifdef RPI
HAP_RESULT_USE_CHECK
static HAPError SnapshotReaderGetSize(const HAPPlatformCameraSnapshotReader* snapshotReader, size_t* numBytes) {
    HAPPrecondition(snapshotReader && snapshotReader->context);
    HAPPrecondition(numBytes);

    HAPPlatformCameraSnapshot* snapshot = snapshotReader->context;
    *numBytes = snapshot->size;

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static HAPError SnapshotReaderRead(
        const HAPPlatformCameraSnapshotReader* snapshotReader,
        void* bytes,
        size_t maxBytes,
        size_t* numBytes) {
    HAPPrecondition(snapshotReader && snapshotReader->context);
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes);

    HAPPlatformCameraSnapshot* snapshot = snapshotReader->context;

    MMAL_BUFFER_HEADER_T* buffer = snapshot->buffer;
    size_t length = buffer->length - snapshot->offset;
    if (length > maxBytes) {
        length = maxBytes;
    }

    (void) mmal_buffer_header_mem_lock(buffer);
    HAPRawBufferCopyBytes(bytes, buffer->data + snapshot->offset, length);
    mmal_buffer_header_mem_unlock(buffer);
    snapshot->offset += length;

    *numBytes = length;

    return kHAPError_None;
}

static void SnapshotReaderClose(const HAPPlatformCameraSnapshotReader* snapshotReader) {
    HAPPrecondition(snapshotReader && snapshotReader->context);

    HAPPlatformCameraSnapshot* snapshot = snapshotReader->context;
    RemoveSnapshotPool(snapshot);
    snapshot->size = 0;
    snapshot->cameraInput = NULL;
}
#endif

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraInputInitializeSnapshot(
        HAPPlatformCameraInputRef cameraInput,
        HAPPlatformCameraSnapshotRef _Nonnull* _Nonnull snapshot) {
    HAPPrecondition(cameraInput);
    HAPPrecondition(snapshot);
    HAPPrecondition(cameraInput->cameraSnapshots);

#ifdef RPI
    // Find free snapshot stream.
    *snapshot = NULL;
    for (size_t i = 0; i < cameraInput->numCameraSnapshots; i++) {
        HAPPlatformCameraSnapshotRef stream = &cameraInput->cameraSnapshots[i];
        if (!stream->cameraInput) {
            HAPRawBufferZero(stream, sizeof *stream);
            stream->cameraInput = cameraInput;
            stream->reader.context = stream;
            stream->reader.getSize = SnapshotReaderGetSize;
            stream->reader.read = SnapshotReaderRead;
            stream->reader.close = SnapshotReaderClose;
            *snapshot = stream;
            break;
        }
    }
    if (!*snapshot) {
        HAPLogError(
                &logObject,
                "[%p] No additional concurrent snapshot streams may be started.",
                (const void*) cameraInput);
        return kHAPError_OutOfResources;
    }
#endif

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraInputTakeSnapshot(
        HAPPlatformCameraInputRef cameraInput,
        HAPPlatformCameraSnapshotRef snapshot,
        uint16_t width HAP_UNUSED,
        uint16_t height HAP_UNUSED,
        HAPPlatformCameraSnapshotReader* _Nonnull* _Nonnull snapshotReader) {
    HAPPrecondition(cameraInput);
    HAPPrecondition(snapshot);
    HAPPrecondition(snapshotReader);

#ifdef RPI
    // Assure semaphore is 0.
    int e;
    do {
        e = sem_trywait(&snapshotSemaphore);
    } while (e == -1 && errno == EINTR);

    HAPAssert(snapshot->buffer == NULL);

    if (width > height) {
        if ((width > kMaxSnapshotWidth) || (height > kMaxSnapshotHeight)) {
            HAPLogError(&logObject, "Invalid snapshot request. Max supported snapshot size is 1280x720p");
            return kHAPError_InvalidData;
        }
    } else {
        if ((height > kMaxSnapshotWidth) || (width > kMaxSnapshotHeight)) {
            HAPLogError(&logObject, "Invalid snapshot request. Max supported snapshot size is 1280x720p");
            return kHAPError_InvalidData;
        }
    }

    HAPError err;
    err = AddSnapshotPipeline(snapshot, cameraInput, width, height);
    if (err) {
        return err;
    }

    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += kSnapshotTimeoutSeconds;
    do {
        e = sem_timedwait(&snapshotSemaphore, &timeout);
    } while (e == -1 && errno == EINTR);

    RemoveSnapshotPipeline(snapshot);

    if (!snapshot->buffer) {
        HAPLogError(&logObject, "snapshot generation failed");
        RemoveSnapshotPool(snapshot);
        return kHAPError_Unknown;
    }

    snapshot->size = snapshot->buffer->length;
    snapshot->offset = 0;
    *snapshotReader = &snapshot->reader;
#endif

    return kHAPError_None;
}

#endif
