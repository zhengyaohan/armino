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
// Copyright (C) 2015-2020 Apple Inc. All Rights Reserved.

// Implementation of the Siri profile.
//
// /!\ It is strongly recommended not to modify Remote.c or Siri.c, to simplify the integration of revised
// versions of these files.

#include "Siri.h"

#define kSiri_StreamingTimeout ((HAPTime)(5 * HAPSecond))

static const HAPSiriDelegate* delegate;

//----------------------------------------------------------------------------------------------------------------------

// Siri Audio handling.

static const HAPDataSendDataStreamProtocolStreamCallbacks dataSendStreamCallbacks;

#define kOpusBitrate   (24000) // [bit/s]
#define kPacketTime    (20)    // [ms]
#define kMaxNumPackets (16)    // max number of audio packets sent in one data stream packet

static void HandleSendDataComplete(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataSendDataStreamProtocol* dataStreamProtocol,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolStream* dataSendStream,
        HAPError error,
        void* scratchBytes,
        size_t numScratchBytes,
        void* _Nullable context);

static void SendNextSiriPacket(HAPSiriAudioContext* audioContext, HAPSiriDataStreamContext* dataStreamContext) {
    HAPPrecondition(audioContext);
    HAPPrecondition(dataStreamContext);

    if (audioContext->endOfStream) {
        // All packets sent.
        return;
    }

    HAPDataSendDataStreamProtocolPacket packets[kMaxNumPackets];
    HAPRawBufferZero(packets, sizeof(packets));

    size_t packetIndex = 0;
    size_t head = audioContext->head;
    size_t tail = audioContext->tail;

    while (head != tail && packetIndex < kMaxNumPackets) {
        // Get next buffer from queue.
        packets[packetIndex].data.bytes = audioContext->circularBuffer[tail].bytes;
        packets[packetIndex].data.numBytes = audioContext->circularBuffer[tail].numBytes;
        packets[packetIndex].metadata.type = kHAPDataSendDataStreamProtocolType_Audio_Siri;
        packets[packetIndex].metadata._.audio.siri.rms = audioContext->circularBuffer[tail].rms;
        packets[packetIndex].metadata._.audio.siri.sequenceNumber = audioContext->sequenceNumber;
        audioContext->sequenceNumber++;
        tail++;
        if (tail == HAPArrayCount(audioContext->circularBuffer)) {
            tail = 0;
        }
        packetIndex++;
    }

    if (head == tail && !audioContext->isSampling) {
        // Last packet.
        audioContext->endOfStream = true;
    }

    if (packetIndex || audioContext->endOfStream) {
        dataStreamContext->busy = true;
        HAPError err;
        err = HAPDataSendDataStreamProtocolSendData(
                dataStreamContext->server,
                dataStreamContext->dispatcher,
                dataStreamContext->dataStream,
                &dataStreamContext->dataSendStream,
                audioContext->scratchBytes,
                sizeof audioContext->scratchBytes,
                packets,
                packetIndex,
                audioContext->endOfStream,
                HandleSendDataComplete);
        if (err) {
            dataStreamContext->busy = false;
            HAPLogError(&kHAPLog_Default, "Siri audio data send failed");
        }
    }

    // Update queue and release data.
    audioContext->tail = tail;
}

static void SiriAudioContinuation(void* _Nullable context, size_t contextSize) {
    HAPPrecondition(context);
    HAPPrecondition(contextSize == sizeof(HAPSiriAudioContext*));

    HAPSiriAudioContext* audioContext = *(HAPSiriAudioContext**) context;
    HAPSiriDataStreamContext* dataStreamContext = audioContext->activeStream;

    if (dataStreamContext && !dataStreamContext->busy) {
        // Send available data.
        SendNextSiriPacket(audioContext, dataStreamContext);
    }
}

static void SiriAudioCallback(
        void* _Nullable context,
        HAPPlatformMicrophoneRef microphone,
        HAPPlatformMicrophoneStreamRef microphoneStream,
        const void* bytes,
        size_t numBytes,
        HAPTimeNS sampleTime HAP_UNUSED,
        float rms) {
    HAPPrecondition(context);
    HAPSiriAudioContext* audioContext = (HAPSiriAudioContext*) context;
    HAPPrecondition(microphone);
    HAPPrecondition(microphoneStream);
    HAPPrecondition(bytes);

    if (!audioContext->isSampling) {
        // Outdated data;
        return;
    }

    // Store data.
    if (numBytes > sizeof audioContext->circularBuffer[0].bytes) {
        // Too much data, ignore.
        HAPLogError(&kHAPLog_Default, "Siri audio data buffer overflow");
        return;
    }
    size_t head = audioContext->head + 1;
    if (head == HAPArrayCount(audioContext->circularBuffer)) {
        head = 0;
    }
    if (head == audioContext->tail) {
        // Overflow, ignore data.
        HAPLogError(&kHAPLog_Default, "Siri audio data queue overflow");
        return;
    }
    HAPRawBufferCopyBytes(audioContext->circularBuffer[audioContext->head].bytes, bytes, numBytes);
    audioContext->circularBuffer[audioContext->head].numBytes = numBytes;
    audioContext->circularBuffer[audioContext->head].rms = rms;
    audioContext->head = head;

    // Synchronize execution to main thread.
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
    HAPError err;
    err = HAPPlatformRunLoopScheduleCallback(SiriAudioContinuation, &audioContext, sizeof audioContext);
    if (err) {
        HAPLogError(&kHAPLog_Default, "Siri audio data synchronization failed");
    }
}

static const HAPDataSendDataStreamProtocolStreamCallbacks dataSendStreamCallbacks;

static void SiriAudioStart(HAPSiriAudioContext* audioContext, HAPSiriDataStreamContext* dataStreamContext) {
    HAPPrecondition(audioContext);
    HAPPrecondition(dataStreamContext);

    dataStreamContext->busy = true;
    HAPDataSendDataStreamProtocolOpen(
            dataStreamContext->server,
            dataStreamContext->dispatcher,
            dataStreamContext->dataStream,
            &dataStreamContext->dataSendStream,
            kHAPDataSendDataStreamProtocolType_Audio_Siri,
            &dataSendStreamCallbacks);

    HAPError err;

    // Start audio.
    err = HAPPlatformMicrophoneStartOpusStream(
            audioContext->microphone,
            &audioContext->microphoneStream,
            audioContext->codecParameters.sampleRate,
            audioContext->codecParameters.bitRateMode,
            kOpusBitrate,
            kPacketTime,
            SiriAudioCallback,
            audioContext);
    if (err) {
        HAPAssert(err == kHAPError_Unknown || err == kHAPError_OutOfResources);
        HAPFatalError();
    }
    HAPPlatformMicrophoneSetVolume(audioContext->microphone, 100);
    HAPPlatformMicrophoneSetMuted(audioContext->microphone, false);

    audioContext->endOfStream = false;
    audioContext->newStream = NULL;
    audioContext->isSampling = true;
    audioContext->activeStream = dataStreamContext;
}

static void SiriAudioStop(HAPSiriAudioContext* audioContext) {
    HAPPrecondition(audioContext);

    if (audioContext->isSampling) {
        HAPPlatformMicrophoneStopStream(audioContext->microphone, HAPNonnull(audioContext->microphoneStream));
        audioContext->microphoneStream = NULL;
        audioContext->isSampling = false;
    }
}

void SiriInitializeAudioConfiguration(HAPSiriAudioContext* audioContext, HAPPlatformMicrophoneRef microphone) {
    HAPPrecondition(audioContext);
    HAPPrecondition(microphone);

    HAPRawBufferZero(audioContext, sizeof *audioContext);
    audioContext->microphone = microphone;
    audioContext->codecType = kHAPAudioCodecType_Opus;
    audioContext->codecParameters.numberOfChannels = 1;
    audioContext->codecParameters.bitRateMode = kHAPAudioCodecBitRateControlMode_Variable;
    audioContext->codecParameters.sampleRate = kHAPAudioCodecSampleRate_16KHz;
}

static void HandleStreamingTimeout(HAPPlatformTimerRef timer, void* _Nullable context) {
    HAPPrecondition(context);
    HAPSiriDataStreamContext* dataStreamContext = (HAPSiriDataStreamContext*) context;
    HAPPrecondition(timer == dataStreamContext->timeoutTimer);

    dataStreamContext->timeoutTimer = 0;

    // Cancel data stream.
    HAPDataSendDataStreamProtocolCancel(
            dataStreamContext->server,
            dataStreamContext->dispatcher,
            dataStreamContext->dataStream,
            &dataStreamContext->dataSendStream);
}

void SiriInputStart(HAPSiriAudioContext* audioContext, HAPSiriDataStreamContext* dataStreamContext) {
    HAPPrecondition(audioContext);
    HAPPrecondition(dataStreamContext);

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    HAPAssert(!audioContext->isSampling);

    if (audioContext->activeStream) {
        // Data stream is still active, delay start of new stream.
        audioContext->newStream = dataStreamContext;
        HAPLogInfo(&kHAPLog_Default, "Audio stream start delayed");
        return;
    }

    SiriAudioStart(audioContext, dataStreamContext);
}

void SiriInputStop(HAPSiriAudioContext* audioContext, HAPSiriDataStreamContext* dataStreamContext) {
    HAPPrecondition(audioContext);
    HAPPrecondition(dataStreamContext);

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    if (audioContext->newStream) {
        // Session not yet started.
        audioContext->newStream = NULL;
        return;
    }

    if (audioContext->isSampling) {
        // Stop audio.
        SiriAudioStop(audioContext);

        if (!dataStreamContext->busy) {
            // Send remaining data.
            SendNextSiriPacket(audioContext, dataStreamContext);
        }

        // Start streaming timeout.
        HAPError err;
        err = HAPPlatformTimerRegister(
                &dataStreamContext->timeoutTimer,
                HAPPlatformClockGetCurrent() + kSiri_StreamingTimeout,
                HandleStreamingTimeout,
                dataStreamContext);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            HAPLog(&kHAPLog_Default, "Not enough resources to start Siri timeout timer");
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------

void SiriHandleDataSendAccept(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataSendDataStreamProtocol* dataStreamProtocol,
        const HAPServiceRequest* request,
        HAPDataStreamHandle dataStream,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol);
    HAPPrecondition(request);
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    delegate->handleSiriAccept(server, request);

    // Setup siri data stream context.
    HAPSiriDataStreamContext* dataStreamContext;
    delegate->getSiriContexts(server, request, NULL, &dataStreamContext);
    HAPAssert(dataStreamContext);

    HAPRawBufferZero(dataStreamContext, sizeof *dataStreamContext);
    dataStreamContext->server = server;
    dataStreamContext->dispatcher = dispatcher;
    dataStreamContext->dataStream = dataStream;
}

void SiriHandleDataSendInvalidate(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataSendDataStreamProtocol* dataStreamProtocol,
        const HAPServiceRequest* request,
        HAPDataStreamHandle dataStream,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol);
    HAPPrecondition(request);
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    HAPSiriAudioContext* audioContext;
    HAPSiriDataStreamContext* dataStreamContext;
    delegate->getSiriContexts(server, request, &audioContext, &dataStreamContext);
    HAPAssert(audioContext);
    HAPAssert(dataStreamContext);

    dataStreamContext->busy = false;

    // Abort audio.
    SiriAudioStop(audioContext);
    audioContext->activeStream = NULL;

    if (dataStreamContext->timeoutTimer) {
        HAPPlatformTimerDeregister(dataStreamContext->timeoutTimer);
        dataStreamContext->timeoutTimer = 0;
    }

    delegate->handleSiriInvalidate(server, request);
}

//----------------------------------------------------------------------------------------------------------------------

static void HandleDataSendStreamClose(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataSendDataStreamProtocol* dataStreamProtocol,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolStream* dataSendStream,
        HAPError error,
        HAPDataSendDataStreamProtocolCloseReason closeReason,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol);
    HAPPrecondition(request);
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPPrecondition(dataSendStream);

    HAPLogInfo(&kHAPLog_Default, "%s (error %u / reason %u)", __func__, error, closeReason);

    HAPSiriAudioContext* audioContext;
    HAPSiriDataStreamContext* dataStreamContext;
    delegate->getSiriContexts(server, request, &audioContext, &dataStreamContext);
    HAPAssert(audioContext);
    HAPAssert(dataStreamContext);

    dataStreamContext->busy = false;

    // Abort audio.
    SiriAudioStop(audioContext);
    audioContext->activeStream = NULL;

    if (dataStreamContext->timeoutTimer) {
        HAPPlatformTimerDeregister(dataStreamContext->timeoutTimer);
        dataStreamContext->timeoutTimer = 0;
    }

    if (audioContext->newStream) {
        SiriAudioStart(audioContext, audioContext->newStream);
    }
}

static void HandleDataSendStreamOpen(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataSendDataStreamProtocol* dataStreamProtocol,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolStream* dataSendStream,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol);
    HAPPrecondition(request);
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPPrecondition(dataSendStream);

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    HAPSiriAudioContext* audioContext;
    HAPSiriDataStreamContext* dataStreamContext;
    delegate->getSiriContexts(server, request, &audioContext, &dataStreamContext);
    HAPAssert(audioContext);
    HAPAssert(dataStreamContext);

    dataStreamContext->busy = false;

    // Send available data.
    SendNextSiriPacket(audioContext, dataStreamContext);
}

static void HandleSendDataComplete(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataSendDataStreamProtocol* dataStreamProtocol,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolStream* dataSendStream,
        HAPError error,
        void* scratchBytes HAP_UNUSED,
        size_t numScratchBytes HAP_UNUSED,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol);
    HAPPrecondition(request);
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPPrecondition(dataSendStream);
    HAPPrecondition(!error);

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    HAPSiriAudioContext* audioContext;
    HAPSiriDataStreamContext* dataStreamContext;
    delegate->getSiriContexts(server, request, &audioContext, &dataStreamContext);
    HAPAssert(audioContext);
    HAPAssert(dataStreamContext);

    dataStreamContext->busy = false;

    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        HAPLogError(&kHAPLog_Default, "HAPDataSendSendData failed");
        return;
    }

    // Send remaining data.
    SendNextSiriPacket(audioContext, dataStreamContext);
}

static const HAPDataSendDataStreamProtocolStreamCallbacks dataSendStreamCallbacks = {
    .handleClose = HandleDataSendStreamClose,
    .handleOpen = HandleDataSendStreamOpen
};

void SiriSetDelegate(const HAPSiriDelegate* siriDelegate) {
    delegate = siriDelegate;
}
