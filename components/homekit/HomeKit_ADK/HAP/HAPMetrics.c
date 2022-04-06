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
// Copyright (C) 2020-2021 Apple Inc. All Rights Reserved.

#include "HAPBase.h"
#include "HAPPlatformFeatures.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_ACCESSORY_METRICS)

#include "HAPLogSubsystem.h"
#include "HAPMetrics.h"
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 1)
#include <stdlib.h>
#endif

/**
 * Metrics queue node
 */
struct MetricsQNode {
    void* buffer;
    size_t bufferSize;
    HAPMetricsID metricID;
    uint64_t nodeSequenceNumber;
    struct MetricsQNode* next;
    struct MetricsQNode* prev;
};

/**
 * Metrics queue
 */
struct MetricsQ {
    uint64_t sequenceNumber;
    int qSize;
    struct MetricsQNode* head;
    struct MetricsQNode* tail;
};

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "Metrics" };
static HAPMetricsContext* activeContext = NULL;
static HAPPlatformTimerRef sendDataTimer = 0;
static struct MetricsQ metricsQ;
static bool metricsBufferFullState = false;
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 0)
static struct MetricsQNode qnodes[kHAPMetrics_MaxStoredEvents];
#endif
/**
 * Initializes the queue
 */
static void MetricsQInit() {
    metricsQ.sequenceNumber = 0;
    metricsQ.qSize = 0;
    metricsQ.head = NULL;
    metricsQ.tail = NULL;
}

/**
 * Checks if queue is full
 */
static bool IsMetricsQFull() {
    if (metricsQ.qSize >= kHAPMetrics_MaxStoredEvents) {
        return true;
    }
    return false;
}

#if !(HAP_DYNAMIC_MEMORY_ALLOCATION == 1)
static struct MetricsQNode* GetFreeQNode() {
    for (int i = 0; i < kHAPMetrics_MaxStoredEvents; i++) {
        if (qnodes[i].buffer == NULL) {
            return &qnodes[i];
        }
    }
    return NULL;
}

static uint8_t* GetBufferUnused() {
    HAPPrecondition(activeContext);
    for (int i = 0; i < activeContext->numEventBuffers; i++) {
        if (activeContext->eventBuffers[i].isUsed == false) {
            activeContext->eventBuffers[i].isUsed = true;
            return activeContext->eventBuffers[i].buffer;
        }
    }
    return NULL;
}

static void MarkBufferUnused(uint8_t* buffer) {
    HAPPrecondition(activeContext);
    for (int i = 0; i < activeContext->numEventBuffers; i++) {
        if (buffer == activeContext->eventBuffers[i].buffer) {
            activeContext->eventBuffers[i].isUsed = false;
            return;
        }
    }
    HAPFatalError();
}
#endif

/**
 * Insert metric event at end of queue
 */
static void MetricsQInsertNodeAtEnd(struct MetricsQNode* node) {
    if (IsMetricsQFull() == true) {
        return;
    }

#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 1)
    struct MetricsQNode* temp = (struct MetricsQNode*) malloc(sizeof(struct MetricsQNode));
#else
    struct MetricsQNode* temp = GetFreeQNode();
#endif
    if (temp == NULL) {
        HAPLogError(&logObject, "%s: metrics node creation failed", __func__);
        HAPFatalError();
    }
    temp->buffer = node->buffer;
    temp->bufferSize = node->bufferSize;
    temp->metricID = node->metricID;
    metricsQ.sequenceNumber++;
    if (metricsQ.sequenceNumber == 0) {
        metricsQ.sequenceNumber++;
    }
    temp->nodeSequenceNumber = metricsQ.sequenceNumber;
    temp->next = NULL;
    temp->prev = NULL;

    if (metricsQ.head == NULL) {
        metricsQ.head = metricsQ.tail = temp;
    } else {
        temp->prev = metricsQ.tail;
        metricsQ.tail->next = temp;
        metricsQ.tail = temp;
    }
    metricsQ.qSize++;
}

/**
 * Checks if queue is empty
 */
static bool IsMetricsQEmpty() {
    if (metricsQ.qSize == 0) {
        return true;
    }
    return false;
}

/**
 * Gets metric node at front of queue
 */
static struct MetricsQNode* MetricsQGetNodeAtFront() {
    if (IsMetricsQEmpty() == true) {
        return NULL;
    }
    return metricsQ.head;
}

/**
 * Deletes metric node at front of queue
 */
static void MetricsQDeleteNodeAtFront() {
    if (IsMetricsQEmpty() == true) {
        return;
    }

    struct MetricsQNode* temp = metricsQ.head;
    metricsQ.head = metricsQ.head->next;
    if (metricsQ.head == NULL) {
        metricsQ.tail = NULL;
    } else {
        metricsQ.head->prev = NULL;
    }
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 1)
    free(temp->buffer);
    free(temp);
#else
    MarkBufferUnused(temp->buffer);
    temp->buffer = NULL;
#endif
    metricsQ.qSize--;

    return;
}

/**
 * Finds metric node in queue
 */
static struct MetricsQNode* MetricsQFindNode(HAPMetricsID metricID) {
    struct MetricsQNode* temp = metricsQ.head;
    while (temp != NULL) {
        if (temp->metricID == metricID) {
            return temp;
        }
        temp = temp->next;
    }
    return NULL;
}

/**
 * Deletes a specific metric node
 */
static void MetricsQDeleteNode(struct MetricsQNode* node) {
    if (node == NULL || metricsQ.head == NULL) {
        return;
    }
    if (metricsQ.head == node) {
        metricsQ.head = metricsQ.head->next;
    }

    if (node->next != NULL) {
        node->next->prev = node->prev;
    }
    if (node->prev != NULL) {
        node->prev->next = node->next;
    }
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 1)
    free(node->buffer);
    free(node);
#else
    MarkBufferUnused(node->buffer);
    node->buffer = NULL;
#endif
    metricsQ.qSize--;
}

/**
 * Deletes all metric nodes of a specific metric ID
 */
static void MetricsQDeleteSpecificNodes(HAPMetricsID metricID) {
    while (true) {
        struct MetricsQNode* temp = MetricsQFindNode(metricID);
        if (temp == NULL) {
            return;
        }
        MetricsQDeleteNode(temp);
    }
}

/**
 * Deletes all queue contents
 */
static void MetricsQDeleteAllNodes() {
    metricsQ.tail = NULL;
    while (metricsQ.head != NULL) {
        struct MetricsQNode* temp = metricsQ.head;
        metricsQ.head = metricsQ.head->next;
#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 1)
        free(temp->buffer);
        free(temp);
#else
        MarkBufferUnused(temp->buffer);
        temp->buffer = NULL;
#endif
    }
    metricsQ.qSize = 0;
    metricsQ.sequenceNumber = 0;
    return;
}

/**
 * Initialize accessory metrics and start metrics capture.
 *
 * @param  metricsContext           Metrics context.
 */
void HAPMetricsInitialize(HAPMetricsContext* metricsContext) {
    HAPPrecondition(metricsContext);
    HAPPrecondition(metricsContext->supportedMetricsStorage);
    HAPPrecondition(metricsContext->metricsActiveValueStorage);

    HAPLogInfo(&logObject, "%s", __func__);

    if (activeContext != NULL) {
        HAPLogInfo(&kHAPLog_Default, "An active metrics context already exists");
        return;
    }
    activeContext = metricsContext;
    activeContext->numDroppedEventsBufferFull = 0;

#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 0)
    HAPPrecondition(activeContext->numEventBuffers >= kHAPMetrics_MaxStoredEvents);
    for (int i = 0; i < activeContext->numEventBuffers; i++) {
        activeContext->eventBuffers[i].isUsed = false;
    }
    for (int i = 0; i < kHAPMetrics_MaxStoredEvents; i++) {
        qnodes[i].buffer = NULL;
    }
#endif

    // Initialize the queue
    MetricsQInit();
}

/**
 * Deinitialize accessory metrics and stop metrics capture.
 *
 * @param  metricsContext           Metrics context.
 */
void HAPMetricsDeinitialize(HAPMetricsContext* metricsContext) {
    HAPPrecondition(metricsContext);

    HAPLogInfo(&logObject, "%s", __func__);

    if (activeContext != metricsContext) {
        HAPLogError(&kHAPLog_Default, "Unknown metrics context");
    }
    MetricsQDeleteAllNodes();
    activeContext = NULL;
}

/**
 * Deletes all previously queued metric events
 */
void HAPMetricsDeleteAllMetricEvents(void) {
    HAPLogInfo(&logObject, "%s, deleting all stored metric events", __func__);
    MetricsQDeleteAllNodes();
}

/**
 * Callback from HDS dataSend when the stream is closed
 */
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

    HAPLogInfo(&logObject, "%s", __func__);

    HAPPrecondition(activeContext != NULL);

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
        case kHAPError_Unknown: {
            errorDescription = "Unknown error.";
            break;
        }
        case kHAPError_NotAuthorized: {
            errorDescription = "Not authorized to perform the operation.";
            break;
        }
        case kHAPError_Busy: {
            errorDescription = "Temporarily busy.";
            break;
        }
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
            closeReasonDescription = "Controller failed to parse the data.";
            break;
        }
        case kHAPDataSendDataStreamProtocolCloseReason_ProtocolError: {
            closeReasonDescription = "A protocol error occurred.";
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
            "%s: Metrics, dataStream = %u, dataSendStream = %p, error = %s, closeReason = %s",
            __func__,
            dataStream,
            (const void*) dataSendStream,
            errorDescription,
            closeReasonDescription);

    HAPMetricsDataStreamContext* dsContext = &(activeContext->dataStreamContext);
    HAPPrecondition(dsContext);
    HAPRawBufferZero(dsContext, sizeof *dsContext);

    if (sendDataTimer) {
        HAPPlatformTimerDeregister(sendDataTimer);
        sendDataTimer = 0;
    }
}

/**
 * Callback from HDS dataSend when the data packets are sent
 */
static void HandleSendMetricsDataComplete(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataSendDataStreamProtocol* dataStreamProtocol,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolStream* dataSendStream,
        HAPError error HAP_UNUSED,
        void* scratchBytes HAP_UNUSED,
        size_t numScratchBytes HAP_UNUSED,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol);
    HAPPrecondition(request);
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPPrecondition(dataSendStream);

    HAPLogInfo(&kHAPLog_Default, "%s: Sent all available metrics data", __func__);
}

/**
 * Sends metric data over HDS
 */
void HandleMetricsDataAvailable(int numEvents) {
    HAPLogInfo(&logObject, "%s: Number of events available to send: %d", __func__, numEvents);

    if (activeContext == NULL) {
        HAPLogError(&kHAPLog_Default, "No active metrics data stream found.");
        return;
    }
    if (numEvents == 0) {
        return;
    }
    HAPLogInfo(
            &logObject,
            "%s: Number of old events dropped due to a full buffer: %d",
            __func__,
            activeContext->numDroppedEventsBufferFull);

    HAPAssert(activeContext->metricsActiveValueStorage != NULL);
    if (*(activeContext->metricsActiveValueStorage) == kHAPCharacteristicValue_Active_Inactive) {
        // If the metrics active characteristic is inactive then expect the controller
        // to terminate the HDS channel.
        // However if the controller wants to keep the channel open in anticipation to
        // re-enable metrics soon then do not send events when inactive.
        HAPLogInfo(&logObject, "%s: Not sending stored events as metrics is set to inactive", __func__);
        return;
    }

    HAPMetricsDataStreamContext* dsContext = &(activeContext->dataStreamContext);
    HAPPrecondition(dsContext);
    HAPError err;
    uint8_t chunkBuf[numEvents][kHAPMetrics_NumScratchBytes];
    HAPDataSendDataStreamProtocolPacket packets[numEvents];

    for (int i = 0; i < numEvents; i++) {
        struct MetricsQNode* node = MetricsQGetNodeAtFront();
        if (node != NULL) {
            HAPPrecondition(kHAPMetrics_NumScratchBytes >= node->bufferSize);
            HAPRawBufferCopyBytes(chunkBuf[i], node->buffer, node->bufferSize);
            packets[i].data.bytes = chunkBuf[i];
            packets[i].data.numBytes = node->bufferSize;
            packets[i].metadata.type = kHAPDataSendDataStreamProtocolType_Accessory_Metrics;
            packets[i].metadata._.metrics.accessoryMetrics.metricSequenceNumber = node->nodeSequenceNumber;
            MetricsQDeleteNodeAtFront();
        } else {
            HAPDataSendDataStreamProtocolCancelWithReason(
                    dsContext->server,
                    dsContext->dispatcher,
                    dsContext->dataStream,
                    dsContext->dataSendStream,
                    kHAPDataSendDataStreamProtocolCancellationReason_UnexpectedFailure);
            return;
        }
    }

    err = HAPDataSendDataStreamProtocolSendData(
            dsContext->server,
            dsContext->dispatcher,
            dsContext->dataStream,
            dsContext->dataSendStream,
            activeContext->dataSendScratchBytes,
            sizeof activeContext->dataSendScratchBytes,
            packets,
            numEvents,
            false, // endOfStream
            HandleSendMetricsDataComplete);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPLogError(&kHAPLog_Default, "Scratch buffer too small. dataSendScratchBytes needs to be enlarged.");
        HAPFatalError();
    }

    bool isQFull = IsMetricsQFull();
    if (isQFull != metricsBufferFullState) {
        metricsBufferFullState = isQFull;
        // Inform the application the state of the metrics buffer
        activeContext->bufferFullStateCallback(metricsBufferFullState);
        if (isQFull == false) {
            activeContext->numDroppedEventsBufferFull = 0;
        }
    }
}

static void MetricsDataAvailableCallback(HAPPlatformTimerRef timer HAP_UNUSED, void* ptr HAP_UNUSED) {
    sendDataTimer = 0;

    HandleMetricsDataAvailable(metricsQ.qSize);

    HAPError err = HAPPlatformTimerRegister(
            &(sendDataTimer),
            HAPPlatformClockGetCurrent() + kHAPMetrics_SendInterval,
            &MetricsDataAvailableCallback,
            NULL);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPLogError(&logObject, "%s: Unable to register metrics send timer", __func__);
        HAPFatalError();
    }
    return;
}

/**
 * Callback from HDS dataSend when a stream is opened
 */
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
    HAPPrecondition(activeContext);

    HAPLogInfo(&logObject, "%s", __func__);

    HAPError err = HAPPlatformTimerRegister(
            &sendDataTimer,
            HAPPlatformClockGetCurrent() + kHAPMetrics_SendInterval,
            &MetricsDataAvailableCallback,
            NULL);
    if (err) {
        HAPLogError(&logObject, "%s: Unable to register metrics send timer", __func__);
        HAPFatalError();
    }
}

static const HAPDataSendDataStreamProtocolStreamCallbacks dataSendStreamCallbacks = {
    .handleClose = HandleDataSendStreamClose,
    .handleOpen = HandleDataSendStreamOpen
};

/**
 * Data stream available handler.
 */
void HAPMetricsHandleDataSendStreamAvailable(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataSendDataStreamProtocol* dataStreamProtocol,
        const HAPServiceRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolType type,
        HAPDataSendDataStreamProtocolOpenMetadata* _Nullable metadata,
        void* _Nullable inDataSendStreamCallbacks HAP_UNUSED,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol);
    HAPPrecondition(request);
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPPrecondition(metadata);

    HAPLogInfo(&logObject, "%s", __func__);

    if (type != kHAPDataSendDataStreamProtocolType_Accessory_Metrics) {
        HAPLogError(&kHAPLog_Default, "Unsupported incoming \"dataSend\" stream type. Rejecting \"dataSend\" stream.");
        HAPDataSendDataStreamProtocolReject(
                server, dispatcher, dataStream, kHAPDataSendDataStreamProtocolRejectReason_Unsupported);
        return;
    }

    HAPPrecondition(activeContext);
    HAPMetricsDataStreamContext* dsContext = &(activeContext->dataStreamContext);
    HAPPrecondition(dsContext);
    if (dsContext->server || dsContext->inProgress) {
        HAPLogError(&kHAPLog_Default, "\"dataSend\" stream already open. Rejecting \"dataSend\" stream.");
        HAPDataSendDataStreamProtocolReject(
                server, dispatcher, dataStream, kHAPDataSendDataStreamProtocolRejectReason_Busy);
        return;
    }

    // Mark dataStreamContext is in progress to prevent a concurrent metrics data stream request.
    HAPDataSendDataStreamProtocolStream* dataSendStream = &(activeContext->dataSendStream);
    activeContext->dataStreamContext.inProgress = true;

    HAPDataSendDataStreamProtocolAccept(server, dispatcher, dataStream, dataSendStream, &dataSendStreamCallbacks);

    // Setup Data Stream context.
    HAPRawBufferZero(dsContext, sizeof *dsContext);
    dsContext->server = server;
    dsContext->dispatcher = dispatcher;
    dsContext->dataStream = dataStream;
    dsContext->dataSendStream = dataSendStream;

    HAPLogInfo(&kHAPLog_Default, "Accepted \"dataSend\" stream: dataSendStream = %p.", (const void*) dataSendStream);
}

/**
 * Submit metric event to the metric queue waiting
 * to be sent to a controller.
 *
 * @param  metricID                 Metric ID.
 * @param  metricEventTLVBytes      TLV encoded metric event.
 * @param  numBytes                 Size of the TLV encoded metric event.
 */
void HAPMetricsSubmitMetricEvent(HAPMetricsID metricID, void* metricEventTLVBytes, size_t numBytes) {
    HAPPrecondition(metricEventTLVBytes);
    HAPPrecondition(numBytes > 0);
    HAPPrecondition(numBytes <= kHAPMetrics_MaxEventBytes);

    if (activeContext == NULL) {
        return;
    }

    // Check if metric event is enabled
    HAPPrecondition(activeContext->supportedMetricsStorage);
    bool foundMetricID = false;
    for (int i = 0; i < activeContext->supportedMetricsStorage->numMetrics; i++) {
        if (activeContext->supportedMetricsStorage->config[i].metricID == metricID) {
            if (activeContext->supportedMetricsStorage->config[i].status ==
                kHAPMetricsSupportedMetricsStatus_Disabled) {
                HAPLogDebug(
                        &logObject, "%s: This metric event %u is disabled. Dropping metric event.", __func__, metricID);
                return;
            }
            foundMetricID = true;
        }
    }
    if (foundMetricID == false) {
        HAPLogError(
                &logObject,
                "%s: This metric event is not found in supported metrics configuration. Dropping metric event.",
                __func__);
        return;
    }

    bool isQFull = IsMetricsQFull();
    if (isQFull != metricsBufferFullState) {
        metricsBufferFullState = isQFull;
        // Inform the application the state of the metrics buffer
        activeContext->bufferFullStateCallback(metricsBufferFullState);
    }
    if (isQFull == true) {
        // If the queue is full, drop the oldest metric event
        // and add the most recent one making the queue circular.
        activeContext->numDroppedEventsBufferFull++;
        MetricsQDeleteNodeAtFront();
    }

    struct MetricsQNode event;

#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 1)
    uint8_t* buffer = (uint8_t*) malloc(numBytes);
#else
    uint8_t* buffer = GetBufferUnused();
#endif
    if (buffer == NULL) {
        HAPLogError(&logObject, "%s: metrics event buffer creation failed", __func__);
        HAPFatalError();
    }

    HAPRawBufferCopyBytes(buffer, metricEventTLVBytes, numBytes);
    event.buffer = buffer;
    event.bufferSize = numBytes;
    event.metricID = metricID;

    MetricsQInsertNodeAtEnd(&event);
    isQFull = IsMetricsQFull();
    if (isQFull != metricsBufferFullState) {
        metricsBufferFullState = isQFull;
        // Inform the application the state of the metrics buffer
        activeContext->bufferFullStateCallback(metricsBufferFullState);
        if (isQFull == false) {
            activeContext->numDroppedEventsBufferFull = 0;
        }
    }
    return;
}

/**
 * Deletes all previously queued metric events of a specific metric ID
 *
 * @param  metricID                 Metric ID.
 */
void HAPMetricsDeleteMetricsEvents(HAPMetricsID metricID) {
    MetricsQDeleteSpecificNodes(metricID);
    return;
}

#endif
