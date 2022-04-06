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

#include "HAPPlatformCameraRecorder+Init.h"

#if (HAP_FEATURE_BATTERY_POWERED_RECORDER == 0)

#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "Camera" };

#ifndef HAP_SAVE_MP4_FILE
#define HAP_SAVE_MP4_FILE 0
#endif

#ifdef DARWIN
#define kMaxBitrateForClips (5000) // 5Mbps
#define kMaxBitrateForAudio (512)  // 512Kbps
#endif
/**
 * Determines frequency for the video timestamps.
 * Video timestamp frequency = frameRate * kFrameRateFactor.
 */
#define kFrameRateFactor (100)

/** Standard AAC frame length in samples. */
#define kAACLCSamplesPerBlock  (1024)
#define kAACELDSamplesPerBlock (480)

#define kNALUSizePrefixSize (sizeof(uint32_t))

#define kMaxSamplesPerSecond (50)
#define kMaxSamplesPerRun    ((kMaxSamplesPerSecond) * (kHAPPlatformCameraRecorder_MaxRunDuration) / 1000)
#define kMaxSamples          (kHAPPlatformCameraRecorder_MaxRuns * kMaxSamplesPerRun)

/** Log2 of clock synchronization control loop time constant in frames. */
#define loopTimeConstLog (10) // 1024 frames ~= 30 sec (@30fps)

// NAL unit types.
// "ITU-T H.264", Table 7-1.
#define kNALUTypeMask (0x1F)
#define kNALUTypeIDR  (5)
#define kNALUTypeSPS  (7)
#define kNALUTypePPS  (8)
// Flag set on starting key frame of a fragment.
#define kNALUTypeFragmentStartFlag (0x40)
// Flag set on first frame after an out-of-memory condition.
#define kNALUTypeRestartFrameFlag (0x80)

// ------------------------ Fragment streaming upcall. --------------------------

static void HAPPlatformCameraSetState(
        HAPPlatformCameraRecorder* _Nullable recorder,
        HAPPlatformCameraRecordingState state) {
    HAPPrecondition(recorder);

    if (recorder->recordingState != state) {
        recorder->recordingState = state;
        char* str = NULL;
        switch (state) {
            case kHAPPlatformCameraRecordingState_Disabled:
                str = "disabled";
                break;
            case kHAPPlatformCameraRecordingState_Monitoring:
                str = "monitoring";
                break;
            case kHAPPlatformCameraRecordingState_Recording:
                str = "recording";
                break;
            case kHAPPlatformCameraRecordingState_Paused:
                str = "paused";
                break;
        }
        HAPLog(&logObject, "Camera recorder set to %s", str);
    }
}

static void HAPPlatformCameraRestartRecording(HAPPlatformCameraRecorder* _Nullable recorder);

static void CheckSendData(HAPPlatformCameraRecorder* _Nullable recorder);

static void SendFragmentContinuation(void* _Nullable context, size_t contextSize) {
    HAPPrecondition(context);
    HAPPrecondition(contextSize == sizeof(HAPPlatformCameraRecorder*));

    HAPPlatformCameraRecorder* recorder = *(HAPPlatformCameraRecorder**) context;
    HAPAssert(recorder->camera);

    if (recorder->dataStreamContext.server) {
        if (recorder->recordingState == kHAPPlatformCameraRecordingState_Recording) {
            // Recording & streaming -> send next packet if possible.
            CheckSendData(recorder);
        }
    } else {
        if (recorder->recordingState == kHAPPlatformCameraRecordingState_Paused) {
            // No streaming & out of memory -> restart recorder.
            HAPPlatformCameraRestartRecording(recorder);
        }
    }
}

static void TrySendFragment(HAPPlatformCameraRecorder* _Nullable recorder) {
    // Synchronize execution to main thread.
    HAPError err;
    err = HAPPlatformRunLoopScheduleCallback(SendFragmentContinuation, &recorder, sizeof recorder);
    if (err) {
        HAPLogError(&logObject, "Send fragment synchronization failed");
    }
}

// ------------------------ Recording queues. --------------------------

/*
 * Cyclic buffers.
 * <free bytes> <header1> <data1> <header2> <data2> ... <headerN> <dataN> <free bytes>
 *              ^tail                                                     ^head
 * tail == head => queue empty
 *
 * Recorder structure:
 * Audio thread -> audio synchronization queue \
 * Video thread -> video synchronization queue -> main queue -> data stream
 *
 * Audio synchronization queue: time ordered audio packets.
 * Video synchronization queue: time ordered video frames.
 * Main queue: time ordered, alternating video/audio runs.
 * Data stream: fragments containing a sequence of runs.
 */

/** Frame header used internally in queues. */
typedef struct {
    HAPTimeNS sampleTime;
    uint32_t size;
    uint32_t type; // video: NALU type, audio: 0
} QueueHeader;

/**
 * Adds raw data to a queue.
 * The caller has to ensure there is enough free space in the queue.
 * The queue must be locked by the caller.
 *
 * @param      queue                The queue.
 * @param      bytes                The bytes to be added to the queue.
 * @param      numBytes             Number of bytes.
 */
static void
        QueuePutBytes(HAPPlatformCameraRecorderQueue* _Nullable queue, const void* _Nullable bytes, size_t numBytes) {
    HAPPrecondition(queue);
    HAPPrecondition(bytes);

    HAPAssert(numBytes < queue->numBytes);

    size_t head = queue->headIndex;
    size_t tail = queue->tailIndex;
    size_t newHead;

    size_t rest = queue->numBytes - head;
    if (numBytes >= rest) {
        newHead = numBytes - rest;
        HAPAssert(tail <= head && tail > newHead);
        HAPRawBufferCopyBytes(&queue->bytes[head], bytes, rest);
        HAPRawBufferCopyBytes(queue->bytes, (const uint8_t*) bytes + rest, newHead);
    } else {
        newHead = head + numBytes;
        HAPAssert(tail <= head || tail > newHead);
        HAPRawBufferCopyBytes(&queue->bytes[head], bytes, numBytes);
    }
    HAPAssert(newHead < queue->numBytes);
    queue->headIndex = newHead;
}

/**
 * Reads raw data from a queue.
 * The caller has to ensure there is enough data in the queue.
 * The queue must be locked by the caller.
 * Used in combination with @fn QueueStartRead() and @fn QueueConfirmRead().
 *
 * @param      queue                The queue.
 * @param      bytes                The buffer to store the bytes.
 * @param      numBytes             Number of bytes.
 */
static void QueueGetBytes(HAPPlatformCameraRecorderQueue* _Nullable queue, void* _Nullable bytes, size_t numBytes) {
    HAPPrecondition(queue);

    size_t head = queue->headIndex;
    size_t tail = queue->tempIndex;
    size_t newTail;

    HAPAssert(numBytes < queue->numBytes);

    size_t rest = queue->numBytes - tail;
    if (numBytes >= rest) {
        newTail = numBytes - rest;
        HAPAssert(head < tail && head >= newTail);
        if (bytes) {
            HAPRawBufferCopyBytes(HAPNonnullVoid(bytes), &queue->bytes[tail], rest);
            HAPRawBufferCopyBytes((uint8_t*) bytes + rest, queue->bytes, newTail);
        }
    } else {
        newTail = tail + numBytes;
        HAPAssert(head < tail || head >= newTail);
        if (bytes) {
            HAPRawBufferCopyBytes(HAPNonnullVoid(bytes), &queue->bytes[tail], numBytes);
        }
    }
    HAPAssert(newTail < queue->numBytes);
    queue->tempIndex = newTail;
}

/**
 * Copies raw data from one queue to another.
 * The caller has to ensure there is enough data in the source queue
 * and enough free space in the destination queue.
 * The queues must be locked by the caller.
 * Used in combination with @fn QueueStartRead() and @fn QueueConfirmRead().
 *
 * @param      srcQueue             The source queue.
 * @param      dstQueue             The destination queue.
 * @param      numBytes             Number of bytes.
 */
static void QueueCopyBytes(
        HAPPlatformCameraRecorderQueue* _Nonnull srcQueue,
        HAPPlatformCameraRecorderQueue* _Nonnull dstQueue,
        size_t numBytes) {
    HAPPrecondition(srcQueue);
    HAPPrecondition(dstQueue);

    size_t head = srcQueue->headIndex;
    size_t tail = srcQueue->tempIndex;
    size_t newTail;

    HAPAssert(numBytes < srcQueue->numBytes);

    size_t rest = srcQueue->numBytes - tail;
    if (numBytes >= rest) {
        newTail = numBytes - rest;
        HAPAssert(head < tail && head >= newTail);
        QueuePutBytes(dstQueue, &srcQueue->bytes[tail], rest);
        QueuePutBytes(dstQueue, srcQueue->bytes, newTail);
    } else {
        newTail = tail + numBytes;
        HAPAssert(head < tail || head >= newTail);
        QueuePutBytes(dstQueue, &srcQueue->bytes[tail], numBytes);
    }
    HAPAssert(newTail < srcQueue->numBytes);
    srcQueue->tempIndex = newTail;
}

/**
 * Initializes a queue.
 *
 * @param      queue                The queue.
 * @param      bytes                The buffer used for the queue.
 * @param      numBytes             Number of bytes in the buffer.
 */
static void QueueInitialize(HAPPlatformCameraRecorderQueue* _Nonnull queue, uint8_t* _Nonnull bytes, size_t numBytes) {
    HAPPrecondition(queue);

    HAPRawBufferZero(queue, sizeof *queue);
    queue->bytes = bytes;
    queue->numBytes = numBytes;

    // Setup mutex.
    int res = pthread_mutex_init(&queue->mutex, NULL);
    if (res) {
        HAPLogError(&logObject, "pthread_mutex_init failed: %d.", res);
        HAPFatalError();
    }
}

/**
 * De-initializes a queue.
 *
 * @param      queue                The queue.
 */
static void QueueDeinitialize(HAPPlatformCameraRecorderQueue* _Nonnull queue) {
    HAPPrecondition(queue);

    // Destroy mutex.
    int res = pthread_mutex_destroy(&queue->mutex);
    HAPAssert(res == 0);
}

/**
 * Starts reading from a queue.
 * The data is not removed from the queue until @fn QueueConfirmRead() is called.
 *
 * @param      queue                The queue.
 */
static void QueueStartRead(HAPPlatformCameraRecorderQueue* _Nonnull queue) {
    HAPPrecondition(queue);

    queue->tempIndex = queue->tailIndex;
}

/**
 * Confirms data removal after reading from a queue.
 *
 * @param      queue                The queue.
 */
static void QueueConfirmRead(HAPPlatformCameraRecorderQueue* _Nonnull queue) {
    HAPPrecondition(queue);

    queue->tailIndex = queue->tempIndex;
}

/**
 * Checks for enough free space to store some data in a queue.
 * Handles needed state transitions if a recorder object is given.
 * The queue must be locked by the caller.
 *
 * @param      queue                The queue.
 * @param      recorder             The associated recorder if the queue is the main queue, NULL otherwise.
 * @param      header               The header describing the data.
 */
static bool QueueSpaceAvailable(
        HAPPlatformCameraRecorderQueue* _Nonnull queue,
        HAPPlatformCameraRecorder* _Nullable recorder,
        QueueHeader* _Nullable header) {
    HAPPrecondition(queue);
    HAPPrecondition(header);

    size_t queueLength = queue->headIndex >= queue->tailIndex ? queue->headIndex - queue->tailIndex :
                                                                queue->numBytes - queue->tailIndex + queue->headIndex;

    size_t size = header->size + sizeof *header;

    if (recorder == NULL) {
        if (queueLength + size >= queue->numBytes) {
            HAPLogError(
                    &logObject,
                    "Synchronization queue too small: size = %zu, used = %zu, new = %zu",
                    queue->numBytes,
                    queueLength,
                    size);
            HAPAssert(false);
            return false;
        }
        return true;
    }

    // Check fragment start.
    if (header->type & kNALUTypeFragmentStartFlag) {
        // Remember start index of current fragment.
        recorder->fragmentStartIndex = queue->headIndex;
        if (recorder->recordingState == kHAPPlatformCameraRecordingState_Paused) {
            // Try to continue recording with this fragment.
            HAPPlatformCameraSetState(recorder, kHAPPlatformCameraRecordingState_Recording);
            header->type |= kNALUTypeRestartFrameFlag;
        }
    }

    // Check out of memory.
    if (recorder->recordingState == kHAPPlatformCameraRecordingState_Monitoring) {
        // Discard outdated frames and ensure there is enough free space.
        if (header->sampleTime >= recorder->prebufferMargin) {
            HAPTimeNS timeLimit = header->sampleTime - recorder->prebufferMargin;
            while (queueLength != 0) { // Not empty.
                QueueHeader hdr;
                QueueStartRead(queue);
                QueueGetBytes(queue, &hdr, sizeof hdr);
                if (queueLength + size < queue->numBytes && // Enough space.
                    hdr.sampleTime >= timeLimit) {          // Frame valid.
                    // Done.
                    return true;
                }
                // Skip and discard the outdated frame.
                QueueGetBytes(queue, NULL, hdr.size);
                QueueConfirmRead(queue);
                queueLength -= sizeof hdr + hdr.size;
            }
        }
        return true;
    } else if (recorder->recordingState == kHAPPlatformCameraRecordingState_Recording) {
        if (queueLength + size >= queue->numBytes) {
            // Not enough space in queue.
            // Reset current fragment.
            queue->headIndex = recorder->fragmentStartIndex;
            // Switch to Paused.
            HAPPlatformCameraSetState(recorder, kHAPPlatformCameraRecordingState_Paused);
            return false;
        }
        return true;
    }
    // recordingState == Disabled or Paused.
    return false;
}

/**
 * Puts a frame into a queue.
 * Handles needed state transitions if a recorder object is given.
 *
 * @param      queue                The queue.
 * @param      recorder             The associated recorder if the queue is the main queue, NULL otherwise.
 * @param      bytes                The bytes to be put to the queue.
 * @param      numBytes             The number of bytes.
 * @param      sampleTime           The sample time to be stored with the bytes.
 * @param      type                 The type to be stored with the bytes.
 * @param      sizePrefix           The contents of a NAL size prefix, 0 if no prefix is needed.
 */
static void QueuePutFrame(
        HAPPlatformCameraRecorderQueue* _Nonnull queue,
        HAPPlatformCameraRecorder* _Nullable recorder,
        const void* _Nullable bytes,
        size_t numBytes,
        HAPTimeNS sampleTime,
        uint32_t type,
        uint32_t sizePrefix) {
    HAPPrecondition(queue);
    HAPPrecondition(bytes);
    QueueHeader header;

    int res = pthread_mutex_lock(&queue->mutex);
    HAPAssert(res == 0);

    if (sizePrefix) {
        // Add space for size prefix bytes.
        numBytes += kNALUSizePrefixSize;
    }

    HAPAssert(numBytes <= UINT32_MAX);
    header.size = (uint32_t) numBytes;
    header.sampleTime = sampleTime;
    header.type = type;
    if (QueueSpaceAvailable(queue, recorder, &header)) {
        // Put header and data to queue.
        QueuePutBytes(queue, &header, sizeof header);
        if (sizePrefix) {
            // Add size prefix bytes.
            uint8_t sizeBytes[kNALUSizePrefixSize];
            HAPWriteBigUInt32(sizeBytes, sizePrefix);
            QueuePutBytes(queue, sizeBytes, kNALUSizePrefixSize);
            numBytes -= kNALUSizePrefixSize;
        }
        QueuePutBytes(queue, bytes, numBytes);
    }

    res = pthread_mutex_unlock(&queue->mutex);
    HAPAssert(res == 0);
}

/**
 * Reads the header information from a frame in a queue.
 * Used in combination with @fn QueueStartRead() and @fn QueueConfirmRead().
 *
 * @param      queue                The queue.
 * @param[out] numBytes             The number of bytes in the frame.
 * @param[out] sampleTime           The sample time of the frame.
 * @param[out] type                 The type of the frame.
 */
static void QueueScanFrame(
        HAPPlatformCameraRecorderQueue* _Nonnull queue,
        size_t* _Nullable numBytes,
        HAPTimeNS* _Nullable sampleTime,
        uint32_t* _Nullable type) {
    HAPPrecondition(queue);
    HAPPrecondition(numBytes);
    HAPPrecondition(sampleTime);
    HAPPrecondition(type);
    *numBytes = 0;

    int res = pthread_mutex_lock(&queue->mutex);
    HAPAssert(res == 0);

    if (queue->tempIndex != queue->headIndex) {
        QueueHeader header;
        QueueGetBytes(queue, &header, sizeof header);
        QueueGetBytes(queue, NULL, header.size);
        *numBytes = header.size;
        *sampleTime = header.sampleTime;
        *type = header.type;
    }

    res = pthread_mutex_unlock(&queue->mutex);
    HAPAssert(res == 0);
}

/**
 * Reads a chunk of data from a frame in a queue.
 * Used in combination with @fn QueueStartRead() and @fn QueueConfirmRead().
 *
 * @param      queue                The queue.
 * @param[out] bytes                The buffer to store the chunk.
 * @param      maxBytes             The size of the buffer.
 * @param[out] numBytes             The size of the chunk.
 * @param      skipAudio            Whether to skip audio data.
 */
static void QueueGetChunk(
        HAPPlatformCameraRecorderQueue* _Nonnull queue,
        void* _Nullable bytes,
        size_t maxBytes,
        size_t* _Nonnull numBytes,
        bool skipAudio) {
    HAPPrecondition(queue);
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes);
    *numBytes = 0;

    int res = pthread_mutex_lock(&queue->mutex);
    HAPAssert(res == 0);

    if (queue->tempIndex != queue->headIndex) {
        size_t size = queue->remainingBytes;

        if (size == 0) {
            // Start new frame.
            QueueHeader header;
            QueueGetBytes(queue, &header, sizeof header);
            while (skipAudio && header.type == 0) {
                QueueGetBytes(queue, NULL, header.size);
                QueueGetBytes(queue, &header, sizeof header);
            }
            size = header.size;
        }

        if (maxBytes > size) {
            maxBytes = size;
        }
        QueueGetBytes(queue, bytes, maxBytes);
        *numBytes += maxBytes;
        queue->remainingBytes = size - maxBytes;
    }

    res = pthread_mutex_unlock(&queue->mutex);
    HAPAssert(res == 0);
}

/**
 * Removes the remaining bytes of a partially read frame from the queue.
 *
 * @param      queue                The queue.
 */
static void QueueSkipChunks(HAPPlatformCameraRecorderQueue* _Nonnull queue) {
    HAPPrecondition(queue);

    int res = pthread_mutex_lock(&queue->mutex);
    HAPAssert(res == 0);

    if (queue->remainingBytes) {
        // Read the remeining bytes of a partially read frame.
        QueueGetBytes(queue, NULL, queue->remainingBytes);
        QueueConfirmRead(queue);
        queue->remainingBytes = 0;
    }

    res = pthread_mutex_unlock(&queue->mutex);
    HAPAssert(res == 0);
}

/**
 * Copy contents of a synchronization queue to the main queue up to a time limit.
 *
 * @param      synchQueue           The synchronization queue to read from.
 * @param      mainQueue            The main queue to write to.
 * @param      recorder             The recorder object.
 * @param      timeLimit            Upper limit for the sample times of the copied samples.
 * @param[out] synchQueueEmpty      Whether the source queue is empty after the copy.
 */
static void QueueCopyFrames(
        HAPPlatformCameraRecorderQueue* _Nonnull synchQueue,
        HAPPlatformCameraRecorderQueue* _Nonnull mainQueue,
        HAPPlatformCameraRecorder* _Nonnull recorder,
        HAPTimeNS timeLimit,
        bool* _Nullable synchQueueEmpty) {
    HAPPrecondition(synchQueue);
    HAPPrecondition(mainQueue);
    HAPPrecondition(recorder);
    HAPPrecondition(synchQueueEmpty);

    for (;;) {
        QueueHeader header;
        QueueStartRead(synchQueue);

        int res = pthread_mutex_lock(&synchQueue->mutex);
        HAPAssert(res == 0);

        if (synchQueue->tempIndex == synchQueue->headIndex) {
            // synchQueue empty.
            res = pthread_mutex_unlock(&synchQueue->mutex);
            HAPAssert(res == 0);
            *synchQueueEmpty = true;
            return;
        }

        QueueGetBytes(synchQueue, &header, sizeof header);

        if (header.sampleTime >= timeLimit) {
            res = pthread_mutex_unlock(&synchQueue->mutex);
            HAPAssert(res == 0);
            *synchQueueEmpty = false;
            return;
        }

        res = pthread_mutex_lock(&mainQueue->mutex);
        HAPAssert(res == 0);

        if (QueueSpaceAvailable(mainQueue, recorder, &header)) {
            QueuePutBytes(mainQueue, &header, sizeof header);
            QueueCopyBytes(synchQueue, mainQueue, header.size);
        } else {
            // No space -> read and discard.
            QueueGetBytes(synchQueue, NULL, header.size);
        }

        res = pthread_mutex_unlock(&mainQueue->mutex);
        HAPAssert(res == 0);
        res = pthread_mutex_unlock(&synchQueue->mutex);
        HAPAssert(res == 0);

        QueueConfirmRead(synchQueue);
    }
}

// ------------------------ Video / Audio PAL interface. --------------------------

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
static void VideoCallback(
        void* _Nullable context,
        HAPPlatformCameraInputRef _Nullable cameraInput,
        HAPPlatformCameraInputStreamRef _Nullable cameraInputStream,
        const void* _Nullable bytes,
        size_t numBytes,
        HAPTimeNS sampleTime) {
    HAPPrecondition(context);
    HAPPrecondition(cameraInput);
    HAPPrecondition(cameraInputStream);
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes > 0);
    HAPPrecondition(numBytes <= UINT32_MAX);

    HAPPlatformCameraRecorder* recorder = (HAPPlatformCameraRecorder*) context;

    uint32_t type = ((const uint8_t*) bytes)[0] & kNALUTypeMask;
    if (type == kNALUTypeSPS) {
        // Save SPS bytes.
        HAPAssert(numBytes <= sizeof recorder->sps);
        HAPRawBufferCopyBytes(recorder->sps, bytes, numBytes);
        recorder->mp4Configuration.video.sps.numBytes = (uint16_t) numBytes;
    } else if (type == kNALUTypePPS) {
        // Save PPS bytes.
        HAPAssert(numBytes <= sizeof recorder->pps);
        HAPRawBufferCopyBytes(recorder->pps, bytes, numBytes);
        recorder->mp4Configuration.video.pps.numBytes = (uint16_t) numBytes;
    } else {
        HAPAssert(sampleTime != UINT64_MAX);

        // Synchronize video time to audio time.
        sampleTime += (uint64_t) recorder->videoTimeCorrection;
        int64_t timeDiff = (int64_t)(sampleTime - recorder->audioStreamTime);
        int64_t corrInc = (timeDiff + (1U << (loopTimeConstLog - 1))) >> loopTimeConstLog;
        recorder->videoTimeCorrection -= corrInc;

        bool runStart = false;
        if (type == kNALUTypeIDR) {
            // Check fragment start.
            if (sampleTime >= recorder->fragmentEnd) {
                type |= kNALUTypeFragmentStartFlag;
                recorder->fragmentEnd = sampleTime + recorder->fragmentDuration;

                if (recorder->keyFrameRequested) {
                    recorder->keyFrameRequested = false;
                }
            }
            runStart = true;
            recorder->videoRunStart = sampleTime;
        } else {
            // If IDR hasn't arrived yet, request one.
            // This will ensure we start a new fragment and the current fragment
            // doesn't extend its duration beyond maximum length configured by
            // the controller.
            if (sampleTime > recorder->fragmentEnd) {
                if (!recorder->keyFrameRequested) {
                    HAPPlatformCameraInputRequestKeyFrame(cameraInput, cameraInputStream);
                    recorder->keyFrameRequested = true;
                }
            }
            if (sampleTime >= recorder->videoRunStart + recorder->runDuration) {
                runStart = true;
                recorder->videoRunStart = sampleTime;
            }
        }

        if (runStart && sampleTime != 0) {
            // Before we start a new video run, we have to add all pending audio samples to the audio run.
            bool synchQueueEmpty;
            QueueCopyFrames(&recorder->audioSynchQueue, &recorder->mainQueue, recorder, sampleTime, &synchQueueEmpty);
            if (synchQueueEmpty) {
                // Not enough samples in audio queue -> add remaining audio samples directly.
                recorder->runIsVideo = false;
            }
        }

        if (recorder->runIsVideo) {
            // Assure all queued video frames are copied before we add a new one.
            bool synchQueueEmpty;
            QueueCopyFrames(&recorder->videoSynchQueue, &recorder->mainQueue, recorder, UINT64_MAX, &synchQueueEmpty);

            // Add video frame to video run.
            QueuePutFrame(&recorder->mainQueue, recorder, bytes, numBytes, sampleTime, type, (uint32_t) numBytes);
        } else {
            // Add video frame to synchronization queue.
            QueuePutFrame(&recorder->videoSynchQueue, NULL, bytes, numBytes, sampleTime, type, (uint32_t) numBytes);
        }

        // Check for streaming of a new fragment.
        if (type & kNALUTypeFragmentStartFlag &&
            (recorder->recordingState == kHAPPlatformCameraRecordingState_Recording ||
             recorder->recordingState == kHAPPlatformCameraRecordingState_Paused)) {
            TrySendFragment(recorder);
        }
    }
}

#ifdef DARWIN
/**
 * Video EOF Handler
 * Flags that the EOF has been reached, and stops the recorder.
 *
 * @param      context              Context.
 */
static void StopVideoCallback(void* _Nullable context) {
    HAPPrecondition(context);
    HAPPlatformCameraRecorder* recorder = (HAPPlatformCameraRecorder*) context;

    // Flag that we reached the end of the video file.
    recorder->videoEOF = true;
    HAPLogInfo(&logObject, "%s: Stopping recording", __func__);
    // Because stopping the camera changes the recorder's operating mode,
    // applications that have registered a callback for this event will be
    // notified that the recording status has changed.
    HAPPlatformCameraStopRecording(recorder);
}
#endif

/**
 * Audio data handler.
 *
 * @param      context              Context.
 * @param      microphone           Microphone.
 * @param      microphoneStream     Microphone stream.
 * @param      bytes                Audio data (AAC packets).
 * @param      numBytes             Length of video data.
 * @param      sampleTime           Sample time of first data entry [ns].
 * @param      rms                  RMS of raw audio data in range -1 .. +1.
 */
static void AudioCallback(
        void* _Nullable context,
        HAPPlatformMicrophoneRef _Nullable microphone,
        HAPPlatformMicrophoneStreamRef _Nullable microphoneStream,
        const void* _Nonnull bytes,
        size_t numBytes,
        HAPTimeNS sampleTime,
        float rms HAP_UNUSED) {
    HAPPrecondition(context);
    HAPPrecondition(microphone);
    HAPPrecondition(microphoneStream);
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes > 0);

    HAPPlatformCameraRecorder* recorder = (HAPPlatformCameraRecorder*) context;
    recorder->audioStreamTime = sampleTime;

    if (!recorder->runIsVideo) {
        // Assure all queued audio frames are copied before we add a new one.
        bool synchQueueEmpty;
        QueueCopyFrames(
                &recorder->audioSynchQueue, &recorder->mainQueue, recorder, recorder->videoRunStart, &synchQueueEmpty);
        if (!synchQueueEmpty) {
            // All audio samples copied.
            recorder->runIsVideo = true;
        }
    }

    if (!recorder->runIsVideo && sampleTime >= recorder->videoRunStart) {
        // Before we start a new audio run, we have to add all pending video frames to the video run.
        bool synchQueueEmpty;
        QueueCopyFrames(&recorder->videoSynchQueue, &recorder->mainQueue, recorder, UINT64_MAX, &synchQueueEmpty);
        // Add remaining video frames directly.
        recorder->runIsVideo = true;
    }

    if (!recorder->runIsVideo) {
        // Add audio sample to audio run.
        QueuePutFrame(&recorder->mainQueue, recorder, bytes, numBytes, sampleTime, 0, 0);
    } else {
        // Add audio sample to synchronization queue.
        QueuePutFrame(&recorder->audioSynchQueue, NULL, bytes, numBytes, sampleTime, 0, 0);
    }
}

/**
 * Starts the recorder and sets it to Monitoring mode.
 *
 * @param      recorder             Recorder.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If an I/O error occurred.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraStartRecording(HAPPlatformCameraRecorder* _Nullable recorder) {
    HAPPrecondition(recorder);
    HAPPrecondition(recorder->recordingState == kHAPPlatformCameraRecordingState_Disabled);

    HAPError err;

    // Load persistent recording configuration.
    bool found;
    HAPCameraRecordingConfiguration configuration;
    err = HAPPlatformCameraGetRecordingConfiguration(recorder->camera, &found, &configuration);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }
    if (!found) {
        HAPLog(&logObject, "%s: No recording configuration selected.", __func__);
        return kHAPError_None;
    }
    bool isAudioEnabled;
    err = HAPPlatformCameraIsRecordingAudioEnabled(recorder->camera, &isAudioEnabled);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    // Disable corresponding camera stream to avoid camera overload.
    // Only needed if the camera input is restricted to two streams.
    HAPCameraStreamingStatus high, low;
    high = HAPPlatformCameraGetStreamStatus(recorder->camera, 0);
    low = HAPPlatformCameraGetStreamStatus(recorder->camera, 1);
    if (HAPMin(configuration.video.attributes.width, configuration.video.attributes.height) > 720) {
        // High resolution recording.
        if (high == kHAPCameraStreamingStatus_InUse && low == kHAPCameraStreamingStatus_InUse) {
            // Both streams are running -> abort and disable high resolution stream.
            HAPPlatformCameraEndStreamingSession(recorder->camera, 0);
            err = HAPPlatformCameraTrySetStreamStatus(recorder->camera, 0, kHAPCameraStreamingStatus_Available);
            HAPAssert(err == kHAPError_None);
            err = HAPPlatformCameraTrySetStreamStatus(recorder->camera, 0, kHAPCameraStreamingStatus_Unavailable);
            HAPAssert(err == kHAPError_None);
        } else if (high == kHAPCameraStreamingStatus_InUse) {
            // Disable low resolution camera stream.
            err = HAPPlatformCameraTrySetStreamStatus(recorder->camera, 1, kHAPCameraStreamingStatus_Unavailable);
            HAPAssert(err == kHAPError_None);
        } else {
            // Disable high resolution camera stream.
            err = HAPPlatformCameraTrySetStreamStatus(recorder->camera, 0, kHAPCameraStreamingStatus_Unavailable);
            HAPAssert(err == kHAPError_None);
            if (low == kHAPCameraStreamingStatus_Unavailable) {
                // Enable low resolution camera stream.
                err = HAPPlatformCameraTrySetStreamStatus(recorder->camera, 1, kHAPCameraStreamingStatus_Available);
                HAPAssert(err == kHAPError_None);
            }
        }
    } else {
        // Low resolution recording.
        if (high == kHAPCameraStreamingStatus_InUse && low == kHAPCameraStreamingStatus_InUse) {
            // Both streams are running -> abort and disable low resolution stream.
            HAPPlatformCameraEndStreamingSession(recorder->camera, 1);
            err = HAPPlatformCameraTrySetStreamStatus(recorder->camera, 1, kHAPCameraStreamingStatus_Available);
            HAPAssert(err == kHAPError_None);
            err = HAPPlatformCameraTrySetStreamStatus(recorder->camera, 1, kHAPCameraStreamingStatus_Unavailable);
            HAPAssert(err == kHAPError_None);
        } else if (low == kHAPCameraStreamingStatus_InUse) {
            // Disable high resolution camera stream.
            err = HAPPlatformCameraTrySetStreamStatus(recorder->camera, 0, kHAPCameraStreamingStatus_Unavailable);
            HAPAssert(err == kHAPError_None);
        } else {
            // Disable low resolution camera stream.
            err = HAPPlatformCameraTrySetStreamStatus(recorder->camera, 1, kHAPCameraStreamingStatus_Unavailable);
            HAPAssert(err == kHAPError_None);
            if (high == kHAPCameraStreamingStatus_Unavailable) {
                // Enable high resolution camera stream.
                err = HAPPlatformCameraTrySetStreamStatus(recorder->camera, 0, kHAPCameraStreamingStatus_Available);
                HAPAssert(err == kHAPError_None);
            }
        }
    }

    // We use a 2 second video and audio synchronization queue.
    size_t videoSize = configuration.video.codecParameters.h264.bitRate * (2000 / 8);
    uint32_t audioBitrate = configuration.audio.codecParameters.bitRate;

#ifdef DARWIN
    // When feeding video samples from file, allocate large queues to allow high bitrate input bitstreams
    videoSize = kMaxBitrateForClips * (2000 / 8);
    audioBitrate = kMaxBitrateForAudio;
#endif

    // AAC encoder requires minimum bitrate of 32kbps for VBR
    if (configuration.audio.codecParameters.bitRateMode == kHAPAudioCodecBitRateControlMode_Variable) {
        audioBitrate = HAPMax(audioBitrate, 32);
    }

    size_t audioSize = audioBitrate * (2000 / 8);
    // The rest is used as the main recording queue.
    HAPAssert(recorder->recordingBuffer.numBytes > videoSize + audioSize);
    size_t mainSize = recorder->recordingBuffer.numBytes - videoSize - audioSize;

    // Setup queues.
    QueueInitialize(&recorder->mainQueue, recorder->recordingBuffer.bytes, mainSize);
    QueueInitialize(&recorder->videoSynchQueue, recorder->recordingBuffer.bytes + mainSize, videoSize);
    QueueInitialize(&recorder->audioSynchQueue, recorder->recordingBuffer.bytes + mainSize + videoSize, audioSize);
    HAPLogInfo(&logObject, "Recording queue sizes: video: %zu, audio: %zu, main: %zu", videoSize, audioSize, mainSize);

    recorder->extraKeyFrameMargin = 0;
    recorder->fragmentEnd = 0;
    recorder->fragmentStartIndex = 0;

    // Setup key frame configuration
    uint32_t fragmentDurationMS =
            configuration.recording.containerConfiguration.containerParameters.fragmentedMP4.fragmentDuration;
    uint32_t iFrameIntervalMS = configuration.video.codecParameters.h264.iFrameInterval;
    if (iFrameIntervalMS > fragmentDurationMS) {
        // One key frame per fragment.
        iFrameIntervalMS = fragmentDurationMS;
    }
    uint32_t frameTimeMS = 1000 / configuration.video.attributes.maxFrameRate;
    if ((fragmentDurationMS + frameTimeMS / 2) % iFrameIntervalMS > frameTimeMS) {
        // Fragment duration is not a multiple of the key frame interval.
        // We have to request an additional keyframe.
        // Threshold is in the middle of the second last frame time.
        recorder->extraKeyFrameMargin = frameTimeMS;
    }
    // Threshold is in the middle of the last frame time.
    recorder->fragmentDuration = (fragmentDurationMS - frameTimeMS / 2) * 1000000;
    recorder->runDuration = (kHAPPlatformCameraRecorder_MaxRunDuration - frameTimeMS / 2) * 1000000;

    // Reset the key frame request flag
    recorder->keyFrameRequested = false;

    // Setup prebuffer.
    recorder->prebufferMargin =
            (configuration.recording.prebufferDuration +
             configuration.recording.containerConfiguration.containerParameters.fragmentedMP4.fragmentDuration) *
            1000000ll;

    // Get timestamp parameters.
    recorder->videoFrequency = configuration.video.attributes.maxFrameRate * kFrameRateFactor;
    switch (configuration.audio.codecParameters.sampleRate) {
        case kHAPAudioCodecSampleRate_8KHz:
            recorder->audioFrequency = 8000;
            break;
        case kHAPAudioCodecSampleRate_16KHz:
            recorder->audioFrequency = 16000;
            break;
        case kHAPAudioCodecSampleRate_24KHz:
            recorder->audioFrequency = 24000;
            break;
        case kHAPAudioCodecSampleRate_32KHz:
            recorder->audioFrequency = 32000;
            break;
        case kHAPAudioCodecSampleRate_44_1KHz:
            recorder->audioFrequency = 44100;
            break;
        case kHAPAudioCodecSampleRate_48KHz:
            recorder->audioFrequency = 48000;
            break;
    }

    // Store MP4 configuration to assure it remains fixed while the recorder is running.
    recorder->mp4Configuration.video.type = kHAPFragmentedMP4VideoType_H264;
    recorder->mp4Configuration.video.width = configuration.video.attributes.width;
    recorder->mp4Configuration.video.height = configuration.video.attributes.height;
    HAPAssert(recorder->videoFrequency <= UINT16_MAX);
    recorder->mp4Configuration.video.timescale = (uint16_t) recorder->videoFrequency;
    recorder->mp4Configuration.video.sps.bytes = recorder->sps;
    recorder->mp4Configuration.video.sps.numBytes = 0;
    recorder->mp4Configuration.video.pps.bytes = recorder->pps;
    recorder->mp4Configuration.video.pps.numBytes = 0;
    recorder->mp4Configuration.video.naluSizeBytes = kNALUSizePrefixSize;
    if (isAudioEnabled) {
        switch (configuration.audio.codecType) {
            case kHAPAudioCodecType_AAC_LC: {
                recorder->mp4Configuration.audio.type = kHAPFragmentedMP4AudioType_AAC_LC;
                break;
            }
            case kHAPAudioCodecType_AAC_ELD: {
                recorder->mp4Configuration.audio.type = kHAPFragmentedMP4AudioType_AAC_ELD;
                break;
            }
            case kHAPAudioCodecType_PCMU:
            case kHAPAudioCodecType_PCMA:
            case kHAPAudioCodecType_Opus:
            case kHAPAudioCodecType_MSBC:
            case kHAPAudioCodecType_AMR:
            case kHAPAudioCodecType_AMR_WB: {
                HAPFatalError();
            }
        }
        recorder->mp4Configuration.audio.numberOfChannels = configuration.audio.codecParameters.numberOfChannels;
        HAPAssert(recorder->audioFrequency <= UINT16_MAX);
        recorder->mp4Configuration.audio.sampleRate = (uint16_t) recorder->audioFrequency;
        recorder->mp4Configuration.audio.bitRate = configuration.audio.codecParameters.bitRate * 1000;
    } else {
        recorder->mp4Configuration.audio.numberOfChannels = 0; // Disable audio in MP4.
    }

    recorder->configurationChanged = false;
    recorder->videoRunStart = 0;
    recorder->runIsVideo = true;
    recorder->audioStreamTime = 0;
    recorder->videoTimeCorrection = 0;
#ifdef DARWIN
    recorder->videoEOF = false;
#endif

    if (configuration.audio.codecType == kHAPAudioCodecType_AAC_ELD) {
        // Create AAC-ELD microphone stream.
        err = HAPPlatformMicrophoneStartAACELDStream(
                recorder->camera->microphone,
                &recorder->microphoneStream,
                configuration.audio.codecParameters.sampleRate,
                configuration.audio.codecParameters.bitRateMode,
                configuration.audio.codecParameters.bitRate * 1000,
                AudioCallback,
                (void*) recorder);
        if (err) {
            HAPLogError(&logObject, "Unable to start AAC-ELD audio recording");
            return err;
        }
    } else {
        // Create AAC-LC microphone stream.
        err = HAPPlatformMicrophoneStartAACLCStream(
                recorder->camera->microphone,
                &recorder->microphoneStream,
                configuration.audio.codecParameters.sampleRate,
                configuration.audio.codecParameters.bitRateMode,
                configuration.audio.codecParameters.bitRate * 1000,
                AudioCallback,
                (void*) recorder);
        if (err) {
            HAPLogError(&logObject, "Unable to start AAC-LC audio recording");
            return err;
        }
    }

    // Create camera input stream.
    err = HAPPlatformCameraInputStartStream(
            recorder->camera->cameraInput,
            &recorder->cameraInputStream,
            configuration.video.attributes.width,
            configuration.video.attributes.height,
            configuration.video.attributes.maxFrameRate,
            configuration.video.codecParameters.h264.bitRate * 1000,
            iFrameIntervalMS,
            configuration.video.codecParameters.h264.profile,
            configuration.video.codecParameters.h264.level,
            VideoCallback,
#ifdef DARWIN
            StopVideoCallback,
#endif
            (void*) recorder);
    if (err) {
        HAPLogError(&logObject, "Unable to start video recording");
        HAPPlatformMicrophoneStopStream(recorder->camera->microphone, recorder->microphoneStream);
        return err;
    }

    HAPPlatformCameraSetState(recorder, kHAPPlatformCameraRecordingState_Monitoring);

    return kHAPError_None;
}

/**
 * Stops the recorder and sets it to Disabled.
 *
 * @param      recorder             Recorder.
 */
void HAPPlatformCameraStopRecording(HAPPlatformCameraRecorder* _Nullable recorder) {
    HAPPrecondition(recorder);
    HAPPrecondition(recorder->recordingState != kHAPPlatformCameraRecordingState_Disabled);
    HAPError err;

    HAPPlatformMicrophoneStopStream(recorder->camera->microphone, recorder->microphoneStream);
    HAPPlatformCameraInputStopStream(recorder->camera->cameraInput, recorder->cameraInputStream);

    HAPPlatformCameraSetState(recorder, kHAPPlatformCameraRecordingState_Disabled);

    // Re-enable camera streams.
    // Only needed if the camera input is restricted to two streams.
    if (HAPPlatformCameraGetStreamStatus(recorder->camera, 0) == kHAPCameraStreamingStatus_Unavailable) {
        err = HAPPlatformCameraTrySetStreamStatus(recorder->camera, 0, kHAPCameraStreamingStatus_Available);
        HAPAssert(err == kHAPError_None);
    } else if (HAPPlatformCameraGetStreamStatus(recorder->camera, 1) == kHAPCameraStreamingStatus_Unavailable) {
        err = HAPPlatformCameraTrySetStreamStatus(recorder->camera, 1, kHAPCameraStreamingStatus_Available);
        HAPAssert(err == kHAPError_None);
    }

    QueueDeinitialize(&recorder->mainQueue);
    QueueDeinitialize(&recorder->videoSynchQueue);
    QueueDeinitialize(&recorder->audioSynchQueue);
}

/**
 * Sets the recorder to Monitoring mode and restarts it if the configuration has changed.
 *
 * @param      recorder             Recorder.
 */
static void HAPPlatformCameraRestartRecording(HAPPlatformCameraRecorder* _Nullable recorder) {
    HAPPrecondition(recorder);

    if (recorder->recordingState != kHAPPlatformCameraRecordingState_Disabled) {
        if (recorder->configurationChanged) {
            HAPPlatformCameraStopRecording(recorder);
            HAPError err = HAPPlatformCameraStartRecording(recorder);
            if (err) {
                HAPLogError(&logObject, "Restarting camera recording stream failed");
            }
        } else {
            QueueSkipChunks(&recorder->mainQueue);
            HAPPlatformCameraSetState(recorder, kHAPPlatformCameraRecordingState_Monitoring);
        }
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraConfigureRecording(HAPPlatformCamera* _Nonnull camera) {
    HAPPrecondition(camera);
    HAPPrecondition(camera->recorder);
    HAPPlatformCameraRecorder* recorder = camera->recorder;

    // Delay configuration until streaming stops.
    if (recorder->recordingState == kHAPPlatformCameraRecordingState_Recording ||
        recorder->recordingState == kHAPPlatformCameraRecordingState_Paused) {
        recorder->configurationChanged = true;
        return kHAPError_None;
    }

    // Stop recording to apply the new recording configuration.
    if (recorder->recordingState != kHAPPlatformCameraRecordingState_Disabled) {
        HAPPlatformCameraStopRecording(recorder);
    }

    return HAPPlatformCameraStartRecording(recorder);
}

void HAPPlatformCameraDisableRecording(HAPPlatformCamera* _Nonnull camera) {
    HAPPrecondition(camera);
    HAPPrecondition(camera->recorder);
    HAPPlatformCameraRecorder* recorder = camera->recorder;

    if (recorder->recordingState != kHAPPlatformCameraRecordingState_Disabled) {
        HAPPlatformCameraStopRecording(recorder);

        // Cancel data stream if needed.
        CheckSendData(recorder);
    }
}

HAP_RESULT_USE_CHECK
uint32_t HAPPlatformCameraGetRecordingResolution(HAPPlatformCameraRef _Nonnull camera) {
    HAPPrecondition(camera);
    HAPPrecondition(camera->recorder);
    HAPPlatformCameraRecorder* recorder = camera->recorder;

    if (recorder->recordingState == kHAPPlatformCameraRecordingState_Disabled) {
        return 0;
    } else {
        return HAPMin(recorder->mp4Configuration.video.width, recorder->mp4Configuration.video.height);
    }
}

// ------------------------ FMP4 encoding. --------------------------

/**
 * Starts MP4 encoding on the recorder.
 *
 * @param      recorder             Recorder.
 */
static void HAPPlatformCameraRecordingOpen(HAPPlatformCameraRecorder* _Nullable recorder) {
    HAPPrecondition(recorder);

    recorder->fragmentNumber = 0;
    recorder->chunkNumber = 1;
}

/**
 * Reads a MP4 movie header from the recorder.
 *
 * @param      recorder             Recorder.
 * @param[out] bytes                The buffer to store the header to.
 * @param      maxBytes             The size of the buffer.
 * @param[out] numBytes             The number of bytes written.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the header.
 * @return kHAPError_Busy           If not enough data is available in the recording.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPPlatformCameraRecordingGetMP4MovieHeader(
        HAPPlatformCameraRecorder* _Nullable recorder,
        void* _Nullable bytes,
        size_t maxBytes,
        size_t* _Nullable numBytes) {
    HAPPrecondition(recorder);
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes);

    *numBytes = 0;

    if (recorder->mp4Configuration.video.sps.numBytes == 0 || recorder->mp4Configuration.video.pps.numBytes == 0) {
        // No SPS/PPS available -> wait for more data.
        HAPLogError(&logObject, "Get movie header: no sps/pps");
        return kHAPError_Busy;
    }

    // Search for next fragment start.
    size_t sampleSize;
    HAPTimeNS sampleTime;
    uint32_t type;
    QueueStartRead(&recorder->mainQueue);
    for (;;) {
        QueueScanFrame(&recorder->mainQueue, &sampleSize, &sampleTime, &type);
        if (sampleSize == 0) {
            // Not enough data in queue -> wait for more data.
            HAPLogError(&logObject, "Get movie header: no fragment start in recording");
            return kHAPError_Busy;
        } else if (type & kNALUTypeFragmentStartFlag) {
            recorder->firstFragmentTime = sampleTime;
            break;
        }
        // Remove unused frames.
        QueueConfirmRead(&recorder->mainQueue);
    }

    recorder->fragmentNumber = 1;
    recorder->chunkNumber = 1;

    // Write FMP4 movie header.
    HAPError err = HAPFragmentedMP4WriteMovieHeader(&recorder->mp4Configuration, bytes, maxBytes, numBytes);
    if (err == kHAPError_None) {
        HAPLogInfo(&logObject, "movie header (%zu bytes)", *numBytes);
    }
    return err;
}

/**
 * Reads a MP4 fragment header from the recorder.
 *
 * @param      recorder             Recorder.
 * @param[out] bytes                The buffer to store the header to.
 * @param      maxBytes             The size of the buffer.
 * @param[out] numBytes             The number of bytes written.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the header.
 * @return kHAPError_Busy           If not enough data is available in the recording.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPPlatformCameraRecordingGetFragmentHeader(
        HAPPlatformCameraRecorder* _Nullable recorder,
        void* _Nullable bytes,
        size_t maxBytes,
        size_t* _Nullable numBytes) {
    HAPPrecondition(recorder);
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes);

    *numBytes = 0;

    // Get sample tables.
    HAPFragmentedMP4SampleDescriptor videoSampleTable[kMaxSamples];
    HAPFragmentedMP4SampleDescriptor audioSampleTable[kMaxSamples];
    size_t videoIndex = 0;
    size_t audioIndex = 0;
    uint32_t previousVideoTimestamp = 0;
    uint32_t previousAudioTimestamp = 0;
    HAPFragmentedMP4RunDescriptor runTable[kHAPPlatformCameraRecorder_MaxRuns];
    HAPFragmentedMP4RunDescriptor* run = NULL;
    size_t runIndex = 0;
    uint32_t previousType = 0;
    size_t fragmentSize = 0;

    QueueStartRead(&recorder->mainQueue);
    for (;;) {
        size_t sampleSize;
        HAPTimeNS sampleTime;
        uint32_t type;
        QueueScanFrame(&recorder->mainQueue, &sampleSize, &sampleTime, &type);
        if (sampleSize == 0) {
            // End of queue -> Wait for more data.
            return kHAPError_Busy;
        }
        // Get timestamp.
        uint32_t frequency = type ? recorder->videoFrequency : recorder->audioFrequency;
        uint32_t timestamp =
                (uint32_t)(((sampleTime - recorder->firstFragmentTime) * frequency + 500000000) / 1000000000);
        if (type) {
            // Check end of fragment.
            if ((type & kNALUTypeFragmentStartFlag) && videoIndex > 0) {
                // This video frame starts the next fragment.
                break;
            }
            // Video sample.
            if (videoIndex > 0) {
                videoSampleTable[videoIndex - 1].duration = timestamp - previousVideoTimestamp;
            }
            previousVideoTimestamp = timestamp;
            // Check recording restart after out-of-memory.
            if (type & kNALUTypeRestartFrameFlag) {
                // Signal missing fragments by a non-contiguous sequence number.
                recorder->fragmentNumber++;
            }
            if (!previousType) {
                // Start new video run.
                HAPAssert(runIndex < kHAPPlatformCameraRecorder_MaxRuns);
                run = &runTable[runIndex];
                runIndex++;
                run->sampleTable = &videoSampleTable[videoIndex];
                run->decodeTime = timestamp;
                run->trackType = kHAPFragmentedMP4TrackType_Video;
                run->numRunDataBytes = 0;
                run->numSamples = 0;
            }
            HAPAssert(videoIndex < kMaxSamples);
            HAPAssert(sampleSize <= UINT32_MAX);
            videoSampleTable[videoIndex].size = (uint32_t) sampleSize;
            videoSampleTable[videoIndex].iFrameFlag = (type & kNALUTypeMask) == kNALUTypeIDR;
            videoSampleTable[videoIndex].duration = kFrameRateFactor; // Default duration for last sample.
            videoIndex++;
            HAPAssert(run);
            run->numSamples++;
            run->numRunDataBytes += sampleSize;
            fragmentSize += sampleSize;
        } else if (recorder->mp4Configuration.audio.numberOfChannels) {
            // Audio sample.
            if (audioIndex > 0) {
                audioSampleTable[audioIndex - 1].duration = timestamp - previousAudioTimestamp;
            }
            previousAudioTimestamp = timestamp;
            if (previousType) {
                // Start new audio run.
                HAPAssert(runIndex < kHAPPlatformCameraRecorder_MaxRuns);
                run = &runTable[runIndex];
                runIndex++;
                run->sampleTable = &audioSampleTable[audioIndex];
                run->decodeTime = timestamp;
                run->trackType = kHAPFragmentedMP4TrackType_Audio;
                run->numRunDataBytes = 0;
                run->numSamples = 0;
            }
            HAPAssert(audioIndex < kMaxSamples);
            HAPAssert(sampleSize <= UINT32_MAX);
            audioSampleTable[audioIndex].size = (uint32_t) sampleSize;
            audioSampleTable[audioIndex].iFrameFlag = 0;
            audioSampleTable[audioIndex].duration = // Default duration for last sample.
                    recorder->mp4Configuration.audio.type == kHAPFragmentedMP4AudioType_AAC_ELD ?
                            kAACELDSamplesPerBlock :
                            kAACLCSamplesPerBlock;
            audioIndex++;
            HAPAssert(run);
            run->numSamples++;
            run->numRunDataBytes += sampleSize;
            fragmentSize += sampleSize;
        }
        previousType = type;
    }

    // Prepare data reading.
    recorder->fragmentSize = (uint32_t) fragmentSize;
    recorder->mainQueue.remainingBytes = 0;
    QueueStartRead(&recorder->mainQueue);

    HAPFragmentedMP4FragmentConfiguration fragmentConfig = {
        .sequenceNumber = recorder->fragmentNumber,
        .numRuns = (uint32_t) runIndex,
        .run = runTable,
    };
    HAPLogInfo(
            &logObject,
            "fragment %lu @ %lu (%zu H264, %zu AAC, %zu runs, %zu bytes)",
            (unsigned long) recorder->fragmentNumber,
            (unsigned long) runTable[0].decodeTime,
            videoIndex,
            audioIndex,
            runIndex,
            fragmentSize);

    // Write fragment header.
    return HAPFragmentedMP4WriteFragmentHeader(&fragmentConfig, bytes, maxBytes, numBytes);
}

/**
 * Reads a chunk of an MP4 fragment from the recorder.
 *
 * @param      recorder             Recorder.
 * @param[out] bytes                The buffer to store the header to.
 * @param      maxBytes             The size of the buffer.
 * @param[out] numBytes             The number of bytes written.
 * @param[out] fragmentNumber       The fragment number of the chunk.
 * @param[out] chunkNumber          The chunk number.
 * @param[out] isLastChunk          Whether this is the last chunk of the fragment.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If the recorder is disabled.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the header.
 * @return kHAPError_Busy           If not enough data is available in the recording.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPPlatformCameraRecordingGetMP4Chunk(
        HAPPlatformCameraRecorder* _Nullable recorder,
        void* _Nullable bytes,
        size_t maxBytes,
        size_t* _Nullable numBytes,
        uint32_t* _Nullable fragmentNumber,
        uint32_t* _Nullable chunkNumber,
        bool* _Nullable isLastChunk) {
    HAPPrecondition(recorder);
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes);
    HAPPrecondition(fragmentNumber);
    HAPPrecondition(chunkNumber);
    HAPPrecondition(isLastChunk);
    *numBytes = 0;
    *fragmentNumber = recorder->fragmentNumber;
    *chunkNumber = recorder->chunkNumber;
    *isLastChunk = false;

    if (recorder->recordingState == kHAPPlatformCameraRecordingState_Disabled) {
        HAPLogError(&logObject, "Get chunk: recorder disabled");
        return kHAPError_InvalidState;
    } else if (recorder->recordingState == kHAPPlatformCameraRecordingState_Monitoring) {
        HAPPlatformCameraSetState(recorder, kHAPPlatformCameraRecordingState_Recording);
    }

    if (recorder->fragmentNumber == 0) {
        // Get movie header.
        HAPError err;
        err = HAPPlatformCameraRecordingGetMP4MovieHeader(recorder, bytes, maxBytes, numBytes);
        if (err != kHAPError_None) {
            return err;
        }
        HAPAssert(*numBytes > 0);
        recorder->fragmentSize = 0;
        recorder->fragmentNumber = 1;
        *isLastChunk = true;
        return kHAPError_None;
    }

    if (recorder->fragmentSize == 0) {
        // Start a new fragment.
        HAPError err;
        size_t length;
        err = HAPPlatformCameraRecordingGetFragmentHeader(recorder, bytes, maxBytes, &length);
        if (err != kHAPError_None) {
            return err;
        }
        HAPAssert(length > 0);
        bytes = (uint8_t*) bytes + length;
        maxBytes -= length;
        *numBytes = length;
    }
    while (maxBytes > 0) {
        size_t length;
        QueueGetChunk(
                &recorder->mainQueue, bytes, maxBytes, &length, recorder->mp4Configuration.audio.numberOfChannels == 0);
        HAPAssert(length > 0);
        QueueConfirmRead(&recorder->mainQueue);
        bytes = (uint8_t*) bytes + length;
        maxBytes -= length;
        *numBytes += length;

        HAPAssert(recorder->fragmentSize >= length);
        recorder->fragmentSize -= length;
        if (recorder->fragmentSize == 0) {
            // End of fragment.
            *isLastChunk = true;
            recorder->fragmentNumber++;
            recorder->chunkNumber = 1;
            return kHAPError_None;
        }
    }
    recorder->chunkNumber++;

    return kHAPError_None;
}

// ------------------------ Data stream. --------------------------

#if HAP_SAVE_MP4_FILE
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static int streamFile = -1;
#endif

static void HandleSendDataComplete(
        HAPAccessoryServer* _Nullable server,
        HAPDataStreamDispatcher* _Nullable dispatcher,
        HAPDataSendDataStreamProtocol* _Nullable dataStreamProtocol,
        const HAPDataStreamRequest* _Nullable request,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolStream* _Nullable dataSendStream,
        HAPError error,
        void* _Nullable scratchBytes,
        size_t numScratchBytes,
        void* _Nullable context HAP_UNUSED);

static void ResetDataSend(HAPPlatformCameraRecorder* _Nullable recorder) {
    HAPPrecondition(recorder);
    HAPPlatformCameraRecorderDataStreamContext* context = &recorder->dataStreamContext;

    if (context->server) {
        HAPRawBufferZero(context, sizeof *context);
        HAPPlatformCameraRestartRecording(recorder);
    }
}

static void CheckSendData(HAPPlatformCameraRecorder* _Nullable recorder) {
    HAPPrecondition(recorder);
    HAPPlatformCameraRecorderDataStreamContext* context = &recorder->dataStreamContext;

    if (!context->server) {
        // No data stream set up.
        return;
    }

    if (!context->readyToSend) {
        // Data Send in progress.
        return;
    }

    uint8_t chunkBytes[kHAPPlatformCameraRecorder_NumChunkBytes];
    HAPError err;
    size_t numChunkBytes;
    uint32_t fragmentNumber;
    uint32_t chunkNumber;
    bool isLastChunk;
    err = HAPPlatformCameraRecordingGetMP4Chunk(
            recorder, chunkBytes, sizeof chunkBytes, &numChunkBytes, &fragmentNumber, &chunkNumber, &isLastChunk);
    if (err == kHAPError_InvalidState) {
        // Recording stopped.
        HAPDataSendDataStreamProtocolCancelWithReason(
                context->server,
                context->dispatcher,
                context->dataStream,
                context->dataSendStream,
                kHAPDataSendDataStreamProtocolCancellationReason_NotAllowed);
        ResetDataSend(recorder);
        return;
    } else if (err == kHAPError_Busy) {
        // Waiting for more data.
        return;
    } else if (err != kHAPError_None) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPLogError(&kHAPLog_Default, "Chunk buffer too small. chunkBytes array needs to be enlarged.");
        HAPFatalError();
    }

#if HAP_SAVE_MP4_FILE
    HAPAssert(streamFile >= 0);
    ssize_t w = write(streamFile, chunkBytes, numChunkBytes);
    HAPAssert(w == (ssize_t) numChunkBytes);
#endif

    size_t dataTotalSize = 0;
    if (chunkNumber == 1) {
        // Add total fragment size field to first chunk.
        dataTotalSize = numChunkBytes + recorder->fragmentSize;
    }

    HAPDataSendDataStreamProtocolPacket packets
            [] = { { .data = { .bytes = chunkBytes, .numBytes = numChunkBytes },
                     .metadata = {
                             .type = kHAPDataSendDataStreamProtocolType_IPCamera_Recording,
                             ._.ipCamera.recording = {
                                     .dataType =
                                             fragmentNumber == 0 ?
                                                     kHAPDataSendDataStreamProtocolIPCameraRecordingDataType_MediaInitialization :
                                                     kHAPDataSendDataStreamProtocolIPCameraRecordingDataType_MediaFragment,
                                     .dataSequenceNumber = fragmentNumber + 1,
                                     .isLastDataChunk = isLastChunk,
                                     .dataChunkSequenceNumber = chunkNumber,
                                     .dataTotalSize = (int64_t) dataTotalSize } } } };

    context->readyToSend = false;
    err = HAPDataSendDataStreamProtocolSendData(
            context->server,
            context->dispatcher,
            context->dataStream,
            context->dataSendStream,
            recorder->dataSendScratchBytes,
            sizeof recorder->dataSendScratchBytes,
            packets,
            HAPArrayCount(packets),
            false, // endOfStream
            HandleSendDataComplete);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPLogError(&kHAPLog_Default, "Scratch buffer too small. dataSendScratchBytes needs to be enlarged.");
        HAPFatalError();
    }

    // Both video and audio must be on. If either one is down while the other is on, it would cause sync queue full.
    const char* microphoneStreamStateString;
    bool isMicrophoneStreamRunning =
            HAPPlatformMicrophoneIsStreamRunning(recorder->microphoneStream, &microphoneStreamStateString);
    const char* cameraStreamStateString;
    bool isCameraStreamRunning =
            HAPPlatformCameraInputIsStreamRunning(recorder->cameraInputStream, &cameraStreamStateString);
    if ((!isMicrophoneStreamRunning && isCameraStreamRunning) ||
        (isMicrophoneStreamRunning && !isCameraStreamRunning)) {
        HAPLogError(
                &logObject,
                "Unexpected microphone stream state '%s' and camera stream state '%s'.",
                microphoneStreamStateString,
                cameraStreamStateString);
        HAPAssert(false);
    }
}

static void HandleSendDataComplete(
        HAPAccessoryServer* _Nullable server,
        HAPDataStreamDispatcher* _Nullable dispatcher,
        HAPDataSendDataStreamProtocol* _Nullable dataStreamProtocol,
        const HAPDataStreamRequest* _Nullable request,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolStream* _Nullable dataSendStream,
        HAPError error,
        void* _Nullable scratchBytes,
        size_t numScratchBytes,
        void* _Nullable _context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol);
    HAPPrecondition(request);
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPPrecondition(dataSendStream);
    HAPPrecondition(scratchBytes);
    HAPPrecondition(numScratchBytes);

    HAPPlatformCameraRef camera = HAPAccessoryGetCamera(server, request->accessory);
    HAPAssert(camera);
    HAPPlatformCameraRecorder* recorder = camera->recorder;
    HAPAssert(recorder);

    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        HAPLogInfo(
                &kHAPLog_Default,
                "%s: camera = %p, dataStream = %u, dataSendStream = %p, "
                "error = HomeKit Data Stream is being invalidated.",
                __func__,
                (const void*) camera,
                dataStream,
                (const void*) dataSendStream);
    }

    HAPPlatformCameraRecorderDataStreamContext* context = &recorder->dataStreamContext;
    if (error) {
        ResetDataSend(recorder);
    } else {
        context->readyToSend = true;
        CheckSendData(recorder);
    }
}

static void HandleDataSendStreamClose(
        HAPAccessoryServer* _Nullable server,
        HAPDataStreamDispatcher* _Nullable dispatcher,
        HAPDataSendDataStreamProtocol* _Nullable dataStreamProtocol,
        const HAPDataStreamRequest* _Nullable request,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolStream* _Nullable dataSendStream,
        HAPError error,
        HAPDataSendDataStreamProtocolCloseReason closeReason,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol);
    HAPPrecondition(request);
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPPrecondition(dataSendStream);

    HAPPlatformCameraRef camera = HAPAccessoryGetCamera(server, request->accessory);
    HAPAssert(camera);
    HAPPlatformCameraRecorder* recorder = camera->recorder;
    HAPAssert(recorder);

    const char* errorDescription = NULL;
    switch (error) {
        case kHAPError_None: {
            errorDescription = "No error occurred.";
            break;
        }
        case kHAPError_InvalidState: {
            errorDescription = "HomeKit Data Stream is being invalidated.";
            break;
        }
        case kHAPError_InvalidData: {
            errorDescription = "Unexpected message has been received.";
            break;
        }
        case kHAPError_OutOfResources: {
            errorDescription = "Out of resources to receive message.";
            break;
        }
        case kHAPError_Unknown:
        case kHAPError_NotAuthorized:
        case kHAPError_Busy: {
        }
            HAPFatalError();
    }
    HAPAssert(errorDescription);

    const char* closeReasonDescription = NULL;
    switch (closeReason) {
        case kHAPDataSendDataStreamProtocolCloseReason_Normal: {
            closeReasonDescription = "Normal close.";
            break;
        }
        case kHAPDataSendDataStreamProtocolCloseReason_NotAllowed: {
            closeReasonDescription = "Controller will not allow the Accessory to send this transfer.";
            break;
        }
        case kHAPDataSendDataStreamProtocolCloseReason_Busy: {
            closeReasonDescription = "Controller cannot accept this transfer right now.";
            break;
        }
        case kHAPDataSendDataStreamProtocolCloseReason_Canceled: {
            closeReasonDescription = "Accessory will not finish the transfer.";
            break;
        }
        case kHAPDataSendDataStreamProtocolCloseReason_Unsupported: {
            closeReasonDescription = "Controller does not support this stream type.";
            break;
        }
        case kHAPDataSendDataStreamProtocolCloseReason_UnexpectedFailure: {
            closeReasonDescription = "Protocol error occurred and the stream has failed.";
            break;
        }
        case kHAPDataSendDataStreamProtocolCloseReason_Timeout: {
            closeReasonDescription = "Accessory could not start the session.";
            break;
        }
        case kHAPDataSendDataStreamProtocolCloseReason_BadData: {
            closeReasonDescription = "Controller failed to parse the MP4 fragment.";
            break;
        }
        case kHAPDataSendDataStreamProtocolCloseReason_ProtocolError: {
            closeReasonDescription =
                    "A protocol error occurred: incorrect sequence number for chunks and/or MP4 fragment.";
            break;
        }
        case kHAPDataSendDataStreamProtocolCloseReason_InvalidConfiguration: {
            closeReasonDescription = "Accessory not configured to perform the request.";
            break;
        }
    }
    HAPAssert(closeReasonDescription);

    HAPLogInfo(
            &kHAPLog_Default,
            "%s: camera = %p, dataStream = %u, dataSendStream = %p, error = %s, closeReason = %s",
            __func__,
            (const void*) camera,
            dataStream,
            (const void*) dataSendStream,
            errorDescription,
            closeReasonDescription);

#if HAP_SAVE_MP4_FILE
    if (streamFile >= 0) {
        close(streamFile);
        streamFile = -1;
        HAPLogInfo(&kHAPLog_Default, "stream.MP4 file written");
    }
#endif

    ResetDataSend(recorder);
}

static void HandleDataSendStreamOpen(
        HAPAccessoryServer* _Nullable server,
        HAPDataStreamDispatcher* _Nullable dispatcher,
        HAPDataSendDataStreamProtocol* _Nullable dataStreamProtocol,
        const HAPDataStreamRequest* _Nullable request,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolStream* _Nullable dataSendStream,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol);
    HAPPrecondition(request);
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPPrecondition(dataSendStream);

    HAPPlatformCameraRef camera = HAPAccessoryGetCamera(server, request->accessory);
    HAPAssert(camera);
    HAPPlatformCameraRecorder* recorder = camera->recorder;
    HAPAssert(recorder);
    HAPPlatformCameraRecorderDataStreamContext* dsContext = &recorder->dataStreamContext;

    HAPLogInfo(
            &kHAPLog_Default,
            "%s: camera = %p, dataStream = %u, dataSendStream = %p",
            __func__,
            (const void*) camera,
            dataStream,
            (const void*) dataSendStream);

    HAPPlatformCameraRecordingOpen(recorder);

#if HAP_SAVE_MP4_FILE
    streamFile = open("stream.mp4", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    HAPAssert(streamFile >= 0);
#endif

    dsContext->readyToSend = true;
    CheckSendData(recorder);
}

static const HAPDataSendDataStreamProtocolStreamCallbacks dataSendStreamCallbacks = {
    .handleClose = HandleDataSendStreamClose,
    .handleOpen = HandleDataSendStreamOpen
};

void HAPPlatformCameraRecorderHandleDataSendStreamAvailable(
        HAPAccessoryServer* _Nonnull server,
        HAPDataStreamDispatcher* _Nonnull dispatcher,
        HAPDataSendDataStreamProtocol* _Nonnull dataStreamProtocol,
        const HAPServiceRequest* _Nonnull request,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolType type,
        HAPDataSendDataStreamProtocolOpenMetadata* _Nullable metadata HAP_UNUSED,
        void* _Nullable inDataSendStreamCallbacksPtr HAP_UNUSED,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol);
    HAPPrecondition(request);
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);

    HAPPlatformCameraRef camera = HAPAccessoryGetCamera(server, request->accessory);
    HAPAssert(camera);
    HAPPlatformCameraRecorder* recorder = camera->recorder;
    HAPAssert(recorder);

    const char* protocolTypeDescription = NULL;
    switch (type) {
        case kHAPDataSendDataStreamProtocolType_Audio_Siri: {
            protocolTypeDescription = "Siri.";
            break;
        }
        case kHAPDataSendDataStreamProtocolType_IPCamera_Recording: {
            protocolTypeDescription = "IP Camera recording.";
            break;
        }
#if HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)
        case kHAPDataSendDataStreamProtocolType_Diagnostics_Snapshot: {
            protocolTypeDescription = "Diagnostics snapshot.";
        } break;
#endif
#if HAP_FEATURE_ENABLED(HAP_FEATURE_ACCESSORY_METRICS)
        case kHAPDataSendDataStreamProtocolType_Accessory_Metrics: {
            protocolTypeDescription = "Accessory metrics.";
            break;
        }
#endif
    }
    HAPAssert(protocolTypeDescription);

    HAPLogInfo(
            &kHAPLog_Default,
            "%s: camera = %p, dataStream = %u, protocolType = %s",
            __func__,
            (const void*) camera,
            dataStream,
            protocolTypeDescription);

    if (!HAPPlatformCameraIsRecordingEnabled(camera)) {
        HAPLog(&kHAPLog_Default, "Camera event recording is currently disabled. Rejecting \"dataSend\" stream.");
        HAPDataSendDataStreamProtocolReject(
                server, dispatcher, dataStream, kHAPDataSendDataStreamProtocolRejectReason_NotAllowed);
        return;
    }

    if (type != kHAPDataSendDataStreamProtocolType_IPCamera_Recording) {
        HAPLog(&kHAPLog_Default, "Unsupported incoming \"dataSend\" stream type. Rejecting \"dataSend\" stream.");
        HAPDataSendDataStreamProtocolReject(
                server, dispatcher, dataStream, kHAPDataSendDataStreamProtocolRejectReason_Unsupported);
        return;
    }

    HAPPlatformCameraRecorderDataStreamContext* dsContext = &recorder->dataStreamContext;
    if (dsContext->server) {
        HAPLog(&kHAPLog_Default, "\"dataSend\" stream already open. Rejecting \"dataSend\" stream.");
        HAPDataSendDataStreamProtocolReject(
                server, dispatcher, dataStream, kHAPDataSendDataStreamProtocolRejectReason_Busy);
        return;
    }

    // Allocate "dataSend" stream.
    HAPDataSendDataStreamProtocolStream* dataSendStream = &recorder->dataSendStream;
    if (inDataSendStreamCallbacksPtr) {
        HAPDataSendDataStreamProtocolStreamCallbacks* inDataSendStreamCallbacks =
                (HAPDataSendDataStreamProtocolStreamCallbacks*) inDataSendStreamCallbacksPtr;
        HAPDataSendDataStreamProtocolAccept(server, dispatcher, dataStream, dataSendStream, inDataSendStreamCallbacks);
    } else {
        HAPDataSendDataStreamProtocolAccept(server, dispatcher, dataStream, dataSendStream, &dataSendStreamCallbacks);
    }

    // Setup Data Stream context.
    HAPRawBufferZero(dsContext, sizeof *dsContext);
    dsContext->server = server;
    dsContext->dispatcher = dispatcher;
    dsContext->dataStream = dataStream;
    dsContext->dataSendStream = dataSendStream;

    HAPLogInfo(&kHAPLog_Default, "Accepted \"dataSend\" stream: dataSendStream = %p.", (const void*) dataSendStream);
}

// ------------------------ Recording configuration. --------------------------

void HAPPlatformCameraTriggerRecording(HAPPlatformCamera* _Nonnull camera) {
    HAPPrecondition(camera);
    HAPPlatformCameraRecorder* recorder = camera->recorder;
    HAPPrecondition(recorder);

    if (recorder->recordingState == kHAPPlatformCameraRecordingState_Monitoring) {
        HAPPlatformCameraSetState(recorder, kHAPPlatformCameraRecordingState_Recording);
    }
}

void HAPPlatformCameraRecorderCreate(
        HAPPlatformCameraRef _Nonnull camera,
        HAPPlatformCameraRecorderRef _Nonnull recorder,
        const HAPPlatformCameraOptions* _Nonnull cameraOptions,
        const HAPPlatformCameraRecorderOptions* _Nonnull recorderOptions) {
    HAPPrecondition(camera);
    HAPPrecondition(recorder);
    HAPPrecondition(cameraOptions);
    HAPPrecondition(recorderOptions);

    HAPRawBufferZero(recorder, sizeof *recorder);

    HAPPlatformCameraCreate(camera, cameraOptions);

    camera->recorder = recorder;
    recorder->camera = camera;

    HAPLogInfo(&logObject, "%s", __func__);

    // Setup recorder memory.
    recorder->recordingBuffer = recorderOptions->recordingBuffer;
}

void HAPPlatformCameraRecorderRelease(HAPPlatformCameraRef _Nonnull camera) {
    HAPPrecondition(camera);

    // Cleanup recorder.
    HAPPlatformCameraDisableRecording(camera);

    HAPPlatformCameraRelease(camera);
}

#endif

#endif // #if (HAP_FEATURE_BATTERY_POWERED_RECORDER == 0)
