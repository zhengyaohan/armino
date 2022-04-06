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
// Copyright (C) 2021 Apple Inc. All Rights Reserved.

#include "HAPPlatformCameraRecorder+Init.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)

#include "HAPCircularQueue.h"

#include "../POSIX/HAPPlatformCameraRecorder.c"

#define SCRATCH_BUFFER_SIZE (1024 * 1024 * 2)

static uint8_t* mediaBuffer;
static size_t mediaBufferSize;
static HAPCircularQueue sideloadQueue;

static HAPPlatformCameraRecorder* recorder;
static HAPPlatformCameraRecorderDataStreamContext context;
static HAPPlatformCameraRecorderDataStreamContext* contextRef;

static uint8_t scratchBuffer[SCRATCH_BUFFER_SIZE];
static uint8_t chunkBuffer[kHAPPlatformCameraRecorder_NumChunkBytes];

static void (*isFragmentSendInProgressObserver)(bool) = NULL;
static void (*isFragmentInQueueObserver)(bool) = NULL;
static void (*currentFragmentNumberObserver)(int64_t) = NULL;

// Current state
static HAPPlatformCameraSideloadedMP4FragmentHeader currentFragmentHeader;
static size_t fragmentProgress;
static int64_t chunkNumber;
static bool needsNewFragment;
static bool isFragmentSendInProgress;
static bool lastChunkOfCurrentFragment;

static void SendSideloadedChunk(void* _Nullable ctx, size_t contextSize);

static inline bool isFragmentInQueue() {
    return sideloadQueue.usedSpace != 0;
}

static inline void notifyFragmentInQueueObserver() {
    if (isFragmentInQueueObserver) {
        isFragmentInQueueObserver(isFragmentInQueue());
    }
}

int64_t HAPPlatformGetCurrentFragmentNumber(void) {
    if (isFragmentSendInProgress == false) {
        return -1;
    }
    return currentFragmentHeader.fragmentNumber;
}

static void notifyCurrentFragmentNumberObserver() {
    if (currentFragmentNumberObserver) {
        currentFragmentNumberObserver(HAPPlatformGetCurrentFragmentNumber());
    }
}

void HAPPlatformSetCurrentFragmentNumberObserver(void (*observer)(int64_t)) {
    currentFragmentNumberObserver = observer;
}

void HAPPlatformSetIsFragmentInQueueObserver(void (*observer)(bool)) {
    isFragmentInQueueObserver = observer;
}

void HAPPlatformSetIsFragmentSendInProgressObserver(void (*observer)(bool)) {
    isFragmentSendInProgressObserver = observer;
}

static inline void setIsFragmentSendInProgress(bool value) {
    isFragmentSendInProgress = value;
    if (isFragmentSendInProgressObserver) {
        isFragmentSendInProgressObserver(value);
    }
}

bool HAPPlatformIsFragmentSendInProgress(void) {
    return isFragmentSendInProgress;
}

bool HAPPlatformIsFragmentInQueue(void) {
    return isFragmentInQueue();
}

static void resetQueue() {
    HAPLogDebug(&kHAPLog_Default, "Clearing queue and resetting state");
    HAPCircularQueueCreate(&sideloadQueue, mediaBuffer, mediaBufferSize);
}

void HAPPlatformInitializeCameraSideload(uint8_t* inMediaBuffer, size_t inMediaBufferSize) {
    HAPLogDebug(&kHAPLog_Default, "HAPPlatformInitializeCameraSideload");
    mediaBuffer = inMediaBuffer;
    mediaBufferSize = inMediaBufferSize;
    chunkNumber = 1;
    fragmentProgress = 0;
    needsNewFragment = true;
    setIsFragmentSendInProgress(false);
    lastChunkOfCurrentFragment = false;
    resetQueue();
}

bool HAPPlatformAddSideloadedMP4Fragment(
        uint8_t* fragmentBytes,
        size_t fragmentSize,
        int64_t fragmentNumber,
        bool endOfStream) {
    HAPLogDebug(&kHAPLog_Default, "Adding MP4 Fragment. Fragment Number: %lld", fragmentNumber);

    HAPPlatformCameraSideloadedMP4FragmentHeader header;
    header.fragmentSize = fragmentSize;
    header.fragmentNumber = fragmentNumber;
    header.endOfStream = endOfStream;

    size_t totalRequiredSpace = sizeof(HAPPlatformCameraSideloadedMP4FragmentHeader) + fragmentSize;
    if (totalRequiredSpace > sideloadQueue.remainingSpace) {
        return false;
    }

    HAPError err = HAPCircularQueueEnqueue(
            &sideloadQueue, (uint8_t*) (&header), sizeof(HAPPlatformCameraSideloadedMP4FragmentHeader));
    if (err) {
        return false;
    }

    err = HAPCircularQueueEnqueue(&sideloadQueue, fragmentBytes, fragmentSize);
    if (err) {
        HAPLogError(&kHAPLog_Default, "Fatal error: Enqueue failed, sideload queue corrupted.");
        HAPFatalError();
    }
    notifyFragmentInQueueObserver();
    return true;
}

static void startFragment() {
    HAPCircularQueueDequeue(
            &sideloadQueue, (uint8_t*) &currentFragmentHeader, sizeof(HAPPlatformCameraSideloadedMP4FragmentHeader));
    fragmentProgress = 0;
    chunkNumber = 0;
    needsNewFragment = false;

    HAPLogInfo(&kHAPLog_Default, "New Fragment, Fragment Number: %lld", currentFragmentHeader.fragmentNumber);
    notifyFragmentInQueueObserver();
    notifyCurrentFragmentNumberObserver();
}

static size_t getSideloadedMP4Chunk(uint8_t* buffer) {
    size_t dataSize = kHAPPlatformCameraRecorder_NumChunkBytes;
    if (currentFragmentHeader.fragmentSize - fragmentProgress < kHAPPlatformCameraRecorder_NumChunkBytes) {
        dataSize = currentFragmentHeader.fragmentSize - fragmentProgress;
        HAPRawBufferZero(buffer, kHAPPlatformCameraRecorder_NumChunkBytes);
    }
    HAPCircularQueueDequeue(&sideloadQueue, buffer, dataSize);
    notifyFragmentInQueueObserver();
    return dataSize;
}

static void sideloadDataSendComplete(
        HAPAccessoryServer* _Nullable server HAP_UNUSED,
        HAPDataStreamDispatcher* _Nullable dispatcher HAP_UNUSED,
        HAPDataSendDataStreamProtocol* _Nullable dataStreamProtocol HAP_UNUSED,
        const HAPDataStreamRequest* _Nullable request HAP_UNUSED,
        HAPDataStreamHandle dataStream HAP_UNUSED,
        HAPDataSendDataStreamProtocolStream* _Nullable dataSendStream,
        HAPError error HAP_UNUSED,
        void* _Nullable scratchBytes HAP_UNUSED,
        size_t numScratchBytes HAP_UNUSED,
        void* _Nullable _context HAP_UNUSED) {

    dataSendStream->state = kHAPDataSendDataStreamProtocolStreamState_Idle;

    if (lastChunkOfCurrentFragment) {
        lastChunkOfCurrentFragment = false;
        setIsFragmentSendInProgress(false);
        notifyCurrentFragmentNumberObserver();
    } else {
        if (fragmentProgress < currentFragmentHeader.fragmentSize) {
            HAPError err = HAPPlatformRunLoopScheduleCallback(SendSideloadedChunk, NULL, 0);
            HAPAssert(!err);
        }
    }
}

bool HAPPlatformSendSideloadedFragment(void) {
    HAPLogDebug(&kHAPLog_Default, "HAPPlatformSendSideloadedFragment");
    if (isFragmentInQueue() == false) {
        HAPLogError(&kHAPLog_Default, "No fragment in queue, exiting");
        return false;
    }
    if (isFragmentSendInProgress) {
        HAPLogError(&kHAPLog_Default, "Another fragment is currently being sent, exiting");
        return false;
    }
    setIsFragmentSendInProgress(true);
    notifyCurrentFragmentNumberObserver();
    HAPError err = HAPPlatformRunLoopScheduleCallback(SendSideloadedChunk, NULL, 0);
    HAPAssert(!err);
    return true;
}

void HAPPlatformStopSendingFragment(void) {
    if (isFragmentSendInProgress) {
        HAPLogDebug(&kHAPLog_Default, "Stopping current fragment send");
        resetQueue();
        notifyFragmentInQueueObserver();
        setIsFragmentSendInProgress(false);
        notifyCurrentFragmentNumberObserver();
        needsNewFragment = true;
    }
}

void HAPPlatformCloseDataStream(void) {
    HAPPlatformCameraStopRecording((void*) recorder);
}

void HAPPlatformSideloadHandleDataSendStreamOpen(
        HAPAccessoryServer* _Nullable server HAP_UNUSED,
        HAPDataStreamDispatcher* _Nullable dispatcher HAP_UNUSED,
        HAPDataSendDataStreamProtocol* _Nullable dataStreamProtocol HAP_UNUSED,
        const HAPDataStreamRequest* _Nullable request HAP_UNUSED,
        HAPDataStreamHandle dataStream HAP_UNUSED,
        HAPDataSendDataStreamProtocolStream* _Nullable dataSendStream HAP_UNUSED,
        void* _Nullable context HAP_UNUSED) {

    // Do nothing
    HAPLogDebug(&kHAPLog_Default, "HAPPlatformSideloadHandleDataSendStreamOpen");
}

void HAPPlatformSideloadHandleDataSendStreamClose(
        HAPAccessoryServer* _Nullable server HAP_UNUSED,
        HAPDataStreamDispatcher* _Nullable dispatcher HAP_UNUSED,
        HAPDataSendDataStreamProtocol* _Nullable dataStreamProtocol HAP_UNUSED,
        const HAPDataStreamRequest* _Nullable request HAP_UNUSED,
        HAPDataStreamHandle dataStream HAP_UNUSED,
        HAPDataSendDataStreamProtocolStream* _Nullable dataSendStream HAP_UNUSED,
        HAPError error HAP_UNUSED,
        HAPDataSendDataStreamProtocolCloseReason closeReason HAP_UNUSED,
        void* _Nullable callbackContext HAP_UNUSED) {

    HAPLogDebug(&kHAPLog_Default, "HAPPlatformSideloadHandleDataSendStreamClose");
    HAPPlatformStopSendingFragment();
    resetQueue();
    HAPRawBufferZero(contextRef, sizeof context);
}

void HAPPlatformSetCameraSideloadDataStreamContext(
        HAPPlatformCameraRecorder* inRecorder,
        HAPPlatformCameraRecorderDataStreamContext* inContext) {
    context.server = inContext->server;
    context.dispatcher = inContext->dispatcher;
    context.dataStream = inContext->dataStream;
    context.dataSendStream = inContext->dataSendStream;
    contextRef = inContext;
    recorder = inRecorder;
}

static void SendSideloadedChunk(void* _Nullable ctx HAP_UNUSED, size_t contextSize HAP_UNUSED) {
    HAPLogDebug(&kHAPLog_Default, "SendSideloadedChunk");

    if (!isFragmentSendInProgress) {
        HAPLogError(&kHAPLog_Default, "Fragment send has aborted: Returning.");
        return;
    }

    if (!context.dataSendStream->isOpen) {
        HAPLogError(&kHAPLog_Default, "DataSendStream has closed unexpectedly. Aborting fragment transmission.");
        HAPPlatformStopSendingFragment();
        return;
    }

    if (needsNewFragment) {
        startFragment();
    }

    size_t numChunkBytes = getSideloadedMP4Chunk(chunkBuffer);
    fragmentProgress += numChunkBytes;
    if (fragmentProgress >= currentFragmentHeader.fragmentSize) {
        lastChunkOfCurrentFragment = true;
        needsNewFragment = true;
    }

    size_t dataTotalSize = 0;
    chunkNumber += 1;
    if (chunkNumber == 1) {
        dataTotalSize = currentFragmentHeader.fragmentSize;
    }

    HAPLogDebug(&kHAPLog_Default, "Sending Chunk");
    HAPLogDebug(&kHAPLog_Default, "Fragment number: %lld", currentFragmentHeader.fragmentNumber);
    HAPLogDebug(&kHAPLog_Default, "Chunk number: %lld", chunkNumber);
    HAPLogDebug(&kHAPLog_Default, "IsLastChunk: %d", lastChunkOfCurrentFragment);

    HAPDataSendDataStreamProtocolPacket packets[] = {
        {
            .data = {
                .bytes = chunkBuffer,
                .numBytes = numChunkBytes,
            },
            .metadata = {
                .type = kHAPDataSendDataStreamProtocolType_IPCamera_Recording,
                ._.ipCamera.recording = {
                    .dataType = currentFragmentHeader.fragmentNumber == 1 ?
                                kHAPDataSendDataStreamProtocolIPCameraRecordingDataType_MediaInitialization :
                                kHAPDataSendDataStreamProtocolIPCameraRecordingDataType_MediaFragment,
                    .dataSequenceNumber = currentFragmentHeader.fragmentNumber,
                    .isLastDataChunk = lastChunkOfCurrentFragment,
                    .dataChunkSequenceNumber = chunkNumber,
                    .dataTotalSize = (int64_t)dataTotalSize,
                },
            },
        }
    };

    bool endOfStream = false;

    if (lastChunkOfCurrentFragment && currentFragmentHeader.endOfStream) {
        HAPLogDebug(&kHAPLog_Default, "End of stream reached (Finished fragment marked end of stream)");
        endOfStream = true;
    }

    HAPError err = HAPDataSendDataStreamProtocolSendData(
            context.server,
            context.dispatcher,
            context.dataStream,
            context.dataSendStream,
            scratchBuffer,
            SCRATCH_BUFFER_SIZE,
            packets,
            HAPArrayCount(packets),
            endOfStream,
            sideloadDataSendComplete);

    HAPAssert(!err);
}

#undef SCRATCH_BUFFER_SIZE

#endif // HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)
