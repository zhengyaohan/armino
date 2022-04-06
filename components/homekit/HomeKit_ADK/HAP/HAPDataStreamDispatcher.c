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

#include "HAPDataStreamDispatcher.h"
#include "HAPAccessoryServer+Internal.h"
#include "HAPDataStream.h"
#include "HAPDataStreamProtocols+Control.h"
#include "HAPLogSubsystem.h"
#include "HAPOPACK.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP) || HAP_FEATURE_ENABLED(HAP_FEATURE_HDS_TRANSPORT_OVER_HAP)

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "DataStreamDispatcher" };

/**
 * Maximum HomeKit Data Stream handle value.
 */
#define kHAPDataStreamDispatcher_MaxDataStreamHandle ((HAPDataStreamHandle) -1)

/**
 * Maximum number of concurrently supported HomeKit Data Streams.
 */
#define kHAPDataStreamDispatcher_MaxDataStreams \
    (HAPMax(SIZE_MAX, (size_t) kHAPDataStreamDispatcher_MaxDataStreamHandle + (size_t) 1))

/**
 * Event timeout.
 */
#define kHAPDataStreamDispatcher_EventTimeout ((HAPTime)(10 * HAPSecond))

/**
 * Request timeout.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.121 Setup Data Stream Transport
 */
#define kHAPDataStreamDispatcher_RequestTimeout ((HAPTime)(10 * HAPSecond))

/**
 * Checks whether a value represents a valid HomeKit Data Stream response status code.
 *
 * @param      value                Value to check.
 *
 * @return true                     If the value is valid.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
static bool HAPDataStreamResponseStatusIsValid(int64_t value) {
    if ((uint64_t) value > (HAPDataStreamResponseStatus) -1) {
        return false;
    }
    switch ((HAPDataStreamResponseStatus) value) {
        case kHAPDataStreamResponseStatus_Success:
        case kHAPDataStreamResponseStatus_OutOfMemory:
        case kHAPDataStreamResponseStatus_Timeout:
        case kHAPDataStreamResponseStatus_HeaderError:
        case kHAPDataStreamResponseStatus_PayloadError:
        case kHAPDataStreamResponseStatus_MissingProtocol:
        case kHAPDataStreamResponseStatus_ProtocolSpecificError: {
            // See computation in HAPDataStreamDescriptor._.request.nameAndTopic.
            HAPAssert(value <= 39);
            return true;
        }
        default:
            return false;
    }
}

/**
 * Checks whether protocol name and topic are valid (i.e., whether a valid response header may be formed).
 *
 * @param      protocolName         Name of the protocol.
 * @param      topic                Topic of the message.
 *
 * @return true                     If a valid response message may be formed with the given protocol name and topic.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
static bool HAPDataStreamRequestNameAndTopicAreValid(const char* protocolName, const char* topic) {
    HAPPrecondition(protocolName);
    HAPPrecondition(topic);

    size_t numNameBytes = HAPStringGetNumBytes(protocolName);
    size_t numTopicBytes = HAPStringGetNumBytes(topic);

    // See computation in HAPDataStreamDescriptor._.request.nameAndTopic.
    size_t numLongStrings = (numNameBytes > 32 ? 1 : 0) + (numTopicBytes > 32 ? 1 : 0);
    return numNameBytes + numTopicBytes <= UINT8_MAX - 41 - numLongStrings;
}

/**
 * Checks whether a value represents a valid HomeKit Data Stream transaction context type.
 *
 * @param      value                Value to check.
 *
 * @return true                     If the value is valid.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
static bool HAPDataStreamTransactionTypeIsValid(uint8_t value) {
    HAPAssert(sizeof value == sizeof(HAPDataStreamTransactionType));
    switch ((HAPDataStreamTransactionType) value) {
        case kHAPDataStreamTransactionType_SendEvent:
        case kHAPDataStreamTransactionType_SendRequest:
        case kHAPDataStreamTransactionType_ReceiveRequest: {
            return true;
        }
        default:
            return false;
    }
}

/**
 * Returns the HomeKit Data Stream handle for a given HomeKit Data Stream.
 *
 * @param      server               Accessory server.
 * @param      dispatcher          HomeKit Data Stream dispatcher.
 * @param      dataStream           HomeKit Data Stream.
 *
 * @return HomeKit Data Stream handle.
 */
HAP_RESULT_USE_CHECK
static HAPDataStreamHandle HAPDataStreamGetHandleForDataStream(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamRef* dataStream) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStream);

    for (size_t i = 0; i < dispatcher->storage->numDataStreams; i++) {
        HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[i];

        if (dataStreamDescriptor->dataStream == dataStream) {
            HAPAssert(i <= kHAPDataStreamDispatcher_MaxDataStreamHandle);
            return (HAPDataStreamHandle) i;
        }
    }

    HAPLogError(
            &logObject,
            "[%p] HomeKit Data Stream [%p] not found in HomeKit Data Stream dispatcher storage.",
            (const void*) dispatcher,
            (const void*) dataStream);
    HAPFatalError();
}

void HAPDataStreamDispatcherCreate(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        const HAPDataStreamDispatcherOptions* options) {
    HAPPrecondition(dispatcher);
    HAPPrecondition(options);
    HAPPrecondition(options->storage);
    HAPPrecondition(options->storage->numDataStreams <= kHAPDataStreamDispatcher_MaxDataStreams);
    HAPPrecondition(options->storage->dataStreamDescriptors);
    HAPPrecondition(options->storage->dataStreamProtocols);
    for (size_t i = 0; options->storage->dataStreamProtocols[i]; i++) {
        HAPBaseDataStreamProtocol* dataStreamProtocol = options->storage->dataStreamProtocols[i];
        HAPPrecondition(dataStreamProtocol->base);
        HAPPrecondition(dataStreamProtocol->base->name);
        HAPPrecondition(!HAPStringAreEqual(dataStreamProtocol->base->name, kHAPControlDataStreamProtocol_Name));
        HAPPrecondition(dataStreamProtocol->base->callbacks.handleEventAvailable);
        HAPPrecondition(dataStreamProtocol->base->callbacks.handleRequestAvailable);

        for (size_t j = 0; j < i; j++) {
            HAPBaseDataStreamProtocol* otherDataStreamProtocol = options->storage->dataStreamProtocols[j];
            HAPPrecondition(!HAPStringAreEqual(dataStreamProtocol->base->name, otherDataStreamProtocol->base->name));
        }
    }

    // Initialize HomeKit Data Stream descriptors.
    HAPRawBufferZero(
            options->storage->dataStreamDescriptors,
            options->storage->numDataStreams * sizeof *options->storage->dataStreamDescriptors);

    // Retain storage.
    HAPRawBufferZero(dispatcher, sizeof *dispatcher);
    dispatcher->server = server;
    dispatcher->storage = options->storage;
}

HAP_RESULT_USE_CHECK
HAPDataStreamProtocol* _Nullable HAPDataStreamDispatcherFindHandlerForProtocolName(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        const char* protocolName) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(protocolName);

    // "control" HomeKit Data Stream protocol is implicitly supported.
    if (HAPStringAreEqual(protocolName, kHAPControlDataStreamProtocol_Name)) {
        static HAPControlDataStreamProtocol controlDataStreamProtocol = { .base = &kHAPControlDataStreamProtocol_Base };
        return &controlDataStreamProtocol;
    }

    // Search supported HomeKit Data Stream protocols.
    for (size_t i = 0; dispatcher->storage->dataStreamProtocols[i]; i++) {
        HAPBaseDataStreamProtocol* dataStreamProtocol = dispatcher->storage->dataStreamProtocols[i];

        if (HAPStringAreEqual(dataStreamProtocol->base->name, protocolName)) {
            return dataStreamProtocol;
        }
    }

    return NULL;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Updates state of transactions.
 *
 * - This checks for expired transactions and updates the reference time for the TTL fields.
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      asyncInvalidate      Whether HomeKit Data Stream invalidation should be performed asynchronously.
 */
static void
        UpdateTransactionState(HAPAccessoryServer* server, HAPDataStreamDispatcher* dispatcher, bool asyncInvalidate);

static void HandleTransactionTimerExpired(HAPPlatformTimerRef timer, void* _Nullable context) {
    HAPPrecondition(context);
    HAPDataStreamDispatcher* dispatcher = context;
    HAPPrecondition(timer == dispatcher->transactionTimer);
    HAPLogDebug(&logObject, "[%p] Transaction timer expired.", (const void*) dispatcher);
    dispatcher->transactionTimer = 0;

    UpdateTransactionState(dispatcher->server, dispatcher, /* asyncInvalidate: */ false);
}

static void
        UpdateTransactionState(HAPAccessoryServer* server, HAPDataStreamDispatcher* dispatcher, bool asyncInvalidate) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);

    HAPError err;

    // Abort ongoing timer.
    if (dispatcher->transactionTimer) {
        HAPPlatformTimerDeregister(dispatcher->transactionTimer);
        dispatcher->transactionTimer = 0;
    }

    // Advance time.
    HAPTime now = HAPPlatformClockGetCurrent();
    HAPTime delta = now - dispatcher->referenceTime;
    dispatcher->referenceTime = now;

    HAPTime minTTL = HAPTime_Max;

    // Enumerate connected HomeKit Data Streams.
    for (size_t i = 0; i < dispatcher->storage->numDataStreams; i++) {
        HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[i];
        if (!dataStreamDescriptor->dataStream) {
            continue;
        }
        HAPAssert(i <= kHAPDataStreamDispatcher_MaxDataStreamHandle);
        HAPDataStreamHandle dataStreamHandle = (HAPDataStreamHandle) i;

        // Enforce transaction timeouts.
        HAPDataStreamTransactionRef* _Nullable baseTransaction_ = dataStreamDescriptor->firstTransaction;
        while (baseTransaction_) {
            HAPDataStreamBaseTransaction* baseTransaction = baseTransaction_;
            HAPAssert(HAPDataStreamTransactionTypeIsValid(baseTransaction->_.type));

            switch ((HAPDataStreamTransactionType) baseTransaction->_.type) {
                case kHAPDataStreamTransactionType_SendEvent: {
                    HAPDataStreamSendEventTransaction* transaction_ = baseTransaction_;
                    HAPDataStreamSendEventTransaction* transaction = (HAPDataStreamSendEventTransaction*) transaction_;

                    HAPBaseDataStreamProtocol* dataStreamProtocol = transaction->dataStreamProtocol;

                    if (transaction->ttl <= delta) {
                        transaction->ttl = 0;
                        if (!asyncInvalidate) {
                            HAPLog(&logObject,
                                   "[%p.%u] %s.%s: event timed out. Invalidating HomeKit Data Stream.",
                                   (const void*) dispatcher,
                                   dataStreamHandle,
                                   dataStreamProtocol->base->name,
                                   transaction->topic);
                            HAPDataStreamInvalidate(server, HAPNonnull(dataStreamDescriptor->dataStream));
                            return;
                        }
                    } else {
                        HAPAssert(delta <= UINT16_MAX);
                        transaction->ttl -= (uint16_t) delta;
                    }
                    minTTL = HAPMin(transaction->ttl, minTTL);
                    HAPAssert(minTTL <= kHAPDataStreamDispatcher_EventTimeout);
                    break;
                }
                case kHAPDataStreamTransactionType_SendRequest: {
                    HAPDataStreamSendRequestTransaction* transaction_ = baseTransaction_;
                    HAPDataStreamSendRequestTransaction* transaction =
                            (HAPDataStreamSendRequestTransaction*) transaction_;

                    HAPBaseDataStreamProtocol* dataStreamProtocol = transaction->dataStreamProtocol;

                    if (transaction->ttl <= delta) {
                        transaction->ttl = 0;
                        if (!asyncInvalidate) {
                            HAPLog(&logObject,
                                   "[%p.%u] %s.%s:%lld request timed out. Invalidating HomeKit Data Stream.",
                                   (const void*) dispatcher,
                                   dataStreamHandle,
                                   dataStreamProtocol->base->name,
                                   transaction->topic,
                                   (long long) transaction->requestID);
                            HAPDataStreamInvalidate(server, HAPNonnull(dataStreamDescriptor->dataStream));
                            return;
                        }
                    } else {
                        HAPAssert(delta <= UINT16_MAX);
                        transaction->ttl -= (uint16_t) delta;
                    }
                    minTTL = HAPMin(transaction->ttl, minTTL);
                    HAPAssert(minTTL <= kHAPDataStreamDispatcher_RequestTimeout);
                    break;
                }
                case kHAPDataStreamTransactionType_ReceiveRequest: {
                    // The timeout for received requests is enforced by the sender.
                    break;
                }
            }

            baseTransaction_ = baseTransaction->nextTransaction;
        }
    }

    // Schedule timer.
    if (minTTL != HAPTime_Max) {
        // NOLINTNEXTLINE(bugprone-branch-clone)
        HAPAssert(minTTL <= HAPMax(kHAPDataStreamDispatcher_EventTimeout, kHAPDataStreamDispatcher_RequestTimeout));
        err = HAPPlatformTimerRegister(
                &dispatcher->transactionTimer, now + minTTL, HandleTransactionTimerExpired, dispatcher);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            HAPLogError(&logObject, "Not enough timers available (HomeKit Data Stream dispatcher request timer).");
            HAPFatalError();
        }
    }
}

/**
 * Indicates whether a HomeKit Data Stream transaction context is registered.
 *
 * Registers a HomeKit Data Stream transaction context.
 *
 * @param      server               Accessory server.
 * @param      dispatcher          HomeKit Data Stream dispatcher.
 * @param      dataStream           HomeKit Data Stream.
 * @param      transaction_         HomeKit Data Stream transaction context.
 */
HAP_RESULT_USE_CHECK
static bool IsTransactionRegistered(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        const HAPDataStreamTransactionRef* transaction_) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream < dispatcher->storage->numDataStreams);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStream];
    HAPPrecondition(transaction_);

    HAPDataStreamTransactionRef* _Nullable otherTransaction_ = dataStreamDescriptor->firstTransaction;
    while (otherTransaction_) {
        HAPDataStreamBaseTransaction* otherTransaction = otherTransaction_;
        if (otherTransaction_ == transaction_) {
            return true;
        }
        otherTransaction_ = otherTransaction->nextTransaction;
    }
    return false;
}

/**
 * Registers a HomeKit Data Stream transaction context.
 *
 * @param      server               Accessory server.
 * @param      dispatcher          HomeKit Data Stream dispatcher.
 * @param      dataStream           HomeKit Data Stream.
 * @param      transaction_         HomeKit Data Stream transaction context.
 */
static void RegisterTransaction(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataStreamTransactionRef* transaction_) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream < dispatcher->storage->numDataStreams);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStream];
    HAPPrecondition(transaction_);
    HAPDataStreamBaseTransaction* transaction = transaction_;
    HAPPrecondition(HAPDataStreamTransactionTypeIsValid(transaction->_.type));

    // Set transaction timeout.
    UpdateTransactionState(server, dispatcher, /* asyncInvalidate: */ true);
    switch ((HAPDataStreamTransactionType) transaction->_.type) {
        case kHAPDataStreamTransactionType_SendEvent: {
            ((HAPDataStreamSendEventTransaction*) transaction_)->ttl = kHAPDataStreamDispatcher_EventTimeout;
            break;
        }
        case kHAPDataStreamTransactionType_SendRequest: {
            ((HAPDataStreamSendRequestTransaction*) transaction_)->ttl = kHAPDataStreamDispatcher_RequestTimeout;
            break;
        }
        case kHAPDataStreamTransactionType_ReceiveRequest: {
            // The timeout for received requests is enforced by the sender.
            break;
        }
    }

    // Add transaction to linked list.
    HAPPrecondition(!transaction->nextTransaction);
    if (!dataStreamDescriptor->firstTransaction) {
        dataStreamDescriptor->firstTransaction = transaction_;
    } else {
        HAPDataStreamTransactionRef* _Nullable previousTransaction_ = dataStreamDescriptor->firstTransaction;
        HAPDataStreamBaseTransaction* _Nullable previousTransaction = previousTransaction_;
        while (previousTransaction->nextTransaction) {
            previousTransaction_ = previousTransaction->nextTransaction;
            previousTransaction = previousTransaction_;
        }
        previousTransaction->nextTransaction = transaction_;
    }
    UpdateTransactionState(server, dispatcher, /* asyncInvalidate: */ true);
}

/**
 * Deregisters a HomeKit Data Stream transaction context.
 *
 * @param      server               Accessory server.
 * @param      dispatcher          HomeKit Data Stream dispatcher.
 * @param      dataStream           HomeKit Data Stream.
 * @param      transaction_         HomeKit Data Stream transaction context.
 */
static void DeregisterTransaction(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataStreamTransactionRef* transaction_) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream < dispatcher->storage->numDataStreams);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStream];
    HAPPrecondition(transaction_);
    HAPDataStreamBaseTransaction* transaction = transaction_;
    HAPPrecondition(HAPDataStreamTransactionTypeIsValid(transaction->_.type));

    // Remove transaction from linked list.
    HAPAssert(dataStreamDescriptor->firstTransaction);
    if (dataStreamDescriptor->firstTransaction == transaction_) {
        dataStreamDescriptor->firstTransaction = transaction->nextTransaction;
        transaction->nextTransaction = NULL;
        UpdateTransactionState(server, dispatcher, /* asyncInvalidate: */ true);
    } else {
        HAPDataStreamTransactionRef* _Nullable previousTransaction_ = dataStreamDescriptor->firstTransaction;
        while (previousTransaction_) {
            HAPDataStreamBaseTransaction* previousTransaction = previousTransaction_;

            if (previousTransaction->nextTransaction == transaction_) {
                previousTransaction->nextTransaction = transaction->nextTransaction;
                transaction->nextTransaction = NULL;
                UpdateTransactionState(server, dispatcher, /* asyncInvalidate: */ true);
                return;
            }

            previousTransaction_ = previousTransaction->nextTransaction;
        }
        HAPLogError(&logObject, "[%p.%u] Transaction to deregister not found.", (const void*) dispatcher, dataStream);
        HAPFatalError();
    }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Gets the HomeKit Data Stream transaction context for which data is being received.
 *
 * @param      server               Accessory server.
 * @param      dispatcher          HomeKit Data Stream dispatcher.
 * @param      dataStream           HomeKit Data Stream.
 *
 * @return HomeKit Data Stream transaction context for which data is being received.
 */
HAP_RESULT_USE_CHECK
static HAPDataStreamTransactionRef* GetReceivingTransaction(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStream];

    HAPAssert(dataStreamDescriptor->firstTransaction);
    HAPDataStreamTransactionRef* _Nullable baseTransaction_ = dataStreamDescriptor->firstTransaction;
    while (baseTransaction_) {
        HAPDataStreamBaseTransaction* baseTransaction = baseTransaction_;
        HAPAssert(HAPDataStreamTransactionTypeIsValid(baseTransaction->_.type));

        switch ((HAPDataStreamTransactionType) baseTransaction->_.type) {
            case kHAPDataStreamTransactionType_SendEvent: {
                // Sending an event does not involve receiving data.
                break;
            }
            case kHAPDataStreamTransactionType_SendRequest: {
                HAPDataStreamSendRequestTransaction* transaction_ = baseTransaction_;
                HAPDataStreamSendRequestTransaction* transaction = (HAPDataStreamSendRequestTransaction*) transaction_;

                if (transaction->state == kHAPDataStreamSendRequestTransactionState_ReceivingResponse) {
                    return baseTransaction_;
                }
                break;
            }
            case kHAPDataStreamTransactionType_ReceiveRequest: {
                HAPDataStreamReceiveRequestTransaction* transaction_ = baseTransaction_;
                HAPDataStreamReceiveRequestTransaction* transaction =
                        (HAPDataStreamReceiveRequestTransaction*) transaction_;

                if (transaction->state == kHAPDataStreamReceiveRequestTransactionState_ReceivingRequest) {
                    return baseTransaction_;
                }
                break;
            }
        }

        baseTransaction_ = baseTransaction->nextTransaction;
    }
    HAPLogError(
            &logObject,
            "[%p.%u] No transaction found for which data is being received.",
            (const void*) dispatcher,
            dataStream);
    HAPFatalError();
}

/**
 * Gets the HomeKit Data Stream transaction context for which data is being sent.
 *
 * @param      server               Accessory server.
 * @param      dispatcher          HomeKit Data Stream dispatcher.
 * @param      dataStream           HomeKit Data Stream.
 *
 * @return HomeKit Data Stream transaction context for which data is being sent.
 */
HAP_RESULT_USE_CHECK
static HAPDataStreamTransactionRef* GetSendingTransaction(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStream];

    HAPAssert(dataStreamDescriptor->firstTransaction);
    HAPDataStreamTransactionRef* _Nullable baseTransaction_ = dataStreamDescriptor->firstTransaction;
    while (baseTransaction_) {
        HAPDataStreamBaseTransaction* baseTransaction = baseTransaction_;
        HAPAssert(HAPDataStreamTransactionTypeIsValid(baseTransaction->_.type));

        switch ((HAPDataStreamTransactionType) baseTransaction->_.type) {
            case kHAPDataStreamTransactionType_SendEvent: {
                HAPDataStreamSendEventTransaction* transaction_ = baseTransaction_;
                HAPDataStreamSendEventTransaction* transaction = (HAPDataStreamSendEventTransaction*) transaction_;

                if (transaction->state == kHAPDataStreamSendEventTransactionState_SendingEvent) {
                    return baseTransaction_;
                }
                break;
            }
            case kHAPDataStreamTransactionType_SendRequest: {
                HAPDataStreamSendRequestTransaction* transaction_ = baseTransaction_;
                HAPDataStreamSendRequestTransaction* transaction = (HAPDataStreamSendRequestTransaction*) transaction_;

                if (transaction->state == kHAPDataStreamSendRequestTransactionState_SendingRequest) {
                    return baseTransaction_;
                }
                break;
            }
            case kHAPDataStreamTransactionType_ReceiveRequest: {
                HAPDataStreamReceiveRequestTransaction* transaction_ = baseTransaction_;
                HAPDataStreamReceiveRequestTransaction* transaction =
                        (HAPDataStreamReceiveRequestTransaction*) transaction_;

                if (transaction->state == kHAPDataStreamReceiveRequestTransactionState_SendingResponse) {
                    return baseTransaction_;
                }
                break;
            }
        }

        baseTransaction_ = baseTransaction->nextTransaction;
    }
    HAPLogError(
            &logObject,
            "[%p.%u] No transaction found for which data is being sent.",
            (const void*) dispatcher,
            dataStream);
    HAPFatalError();
}

static void HandleAccept(
        HAPAccessoryServer* server,
        const HAPDataStreamRequest* request,
        HAPDataStreamRef* dataStream,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->accessory);
    HAPPrecondition(request->accessory->dataStream.delegate.callbacks == &kHAPDataStreamDispatcher_DataStreamCallbacks);
    HAPDataStreamDispatcher* dispatcher = request->accessory->dataStream.delegate.context;
    HAPPrecondition(dataStream);

    for (size_t i = 0; i < dispatcher->storage->numDataStreams; i++) {
        HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[i];
        if (dataStreamDescriptor->dataStream) {
            continue;
        }

        HAPRawBufferZero(dataStreamDescriptor, sizeof *dataStreamDescriptor);
        dataStreamDescriptor->dataStream = dataStream;

        HAPAssert(i <= kHAPDataStreamDispatcher_MaxDataStreamHandle);
        HAPDataStreamHandle dataStreamHandle = (HAPDataStreamHandle) i;

        // Inform "control" HomeKit Data Stream protocol handler.
        HAPBaseDataStreamProtocol* _Nullable controlDataStreamProtocol =
                HAPDataStreamDispatcherFindHandlerForProtocolName(
                        server, dispatcher, kHAPControlDataStreamProtocol_Name);
        HAPAssert(controlDataStreamProtocol);
        HAPLogInfo(
                &logObject,
                "[%p.%u] HomeKit Data Stream [%p] accepted. Informing %s protocol handler.",
                (const void*) dispatcher,
                dataStreamHandle,
                (const void*) dataStream,
                controlDataStreamProtocol->base->name);
        HAPAssert(controlDataStreamProtocol->base->callbacks.handleAccept);
        controlDataStreamProtocol->base->callbacks.handleAccept(
                server, dispatcher, HAPNonnull(controlDataStreamProtocol), request, dataStreamHandle, context);
        return;
    }

    HAPLogError(
            &logObject,
            "[%p] Cannot accept HomeKit Data Stream [%p] (HAPDataStreamDispatcherStorage.numDataStreams = %zu).",
            (const void*) dispatcher,
            (const void*) dataStream,
            dispatcher->storage->numDataStreams);
    HAPDataStreamInvalidate(server, dataStream);
}

static void HandleInvalidate(
        HAPAccessoryServer* server,
        const HAPDataStreamRequest* request,
        HAPDataStreamRef* dataStream,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->accessory);
    HAPPrecondition(request->accessory->dataStream.delegate.callbacks == &kHAPDataStreamDispatcher_DataStreamCallbacks);
    HAPDataStreamDispatcher* dispatcher = request->accessory->dataStream.delegate.context;
    HAPPrecondition(dataStream);

    for (size_t i = 0; i < dispatcher->storage->numDataStreams; i++) {
        HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[i];
        if (dataStreamDescriptor->dataStream != dataStream) {
            continue;
        }

        HAPAssert(i <= kHAPDataStreamDispatcher_MaxDataStreamHandle);
        HAPDataStreamHandle dataStreamHandle = (HAPDataStreamHandle) i;

        // Abort pending skip and reply to request transaction.
        if (dataStreamDescriptor->receiveState == kHAPDataStreamDescriptorReceiveState_SkippingRequestMessage ||
            dataStreamDescriptor->receiveState == kHAPDataStreamDescriptorReceiveState_WaitingForResponse ||
            dataStreamDescriptor->receiveState == kHAPDataStreamDescriptorReceiveState_SendingResponse) {
            HAPDataStreamSkipAndReplyToRequestCompletionHandler _Nullable completionHandler =
                    dataStreamDescriptor->_.request.skipAndReplyToRequestCompletionHandler;
            if (completionHandler) {
                HAPLogInfo(
                        &logObject,
                        "[%p.%u] HomeKit Data Stream invalidated. Invalidating skip and reply to request transaction.",
                        (const void*) dispatcher,
                        dataStreamHandle);
                const char* protocolName = &dataStreamDescriptor->_.request.nameAndTopic[0];
                HAPDataStreamProtocol* _Nullable dataStreamProtocol =
                        HAPDataStreamDispatcherFindHandlerForProtocolName(server, dispatcher, protocolName);
                completionHandler(
                        server,
                        dispatcher,
                        dataStreamProtocol,
                        request,
                        dataStreamHandle,
                        kHAPError_InvalidState,
                        context);
            }
        }

        // Abort pending transactions.
        HAP_DIAGNOSTIC_PUSH
        HAP_DIAGNOSTIC_IGNORED_ARMCC(1293) // #1293-D: assignment in condition
        HAPDataStreamTransactionRef* _Nullable baseTransaction_;
        while ((baseTransaction_ = dataStreamDescriptor->firstTransaction)) {
            HAPDataStreamBaseTransaction* baseTransaction = baseTransaction_;
            HAPAssert(HAPDataStreamTransactionTypeIsValid(baseTransaction->_.type));

            switch ((HAPDataStreamTransactionType) baseTransaction->_.type) {
                case kHAPDataStreamTransactionType_SendEvent: {
                    HAPDataStreamSendEventTransaction* transaction_ = baseTransaction_;
                    HAPDataStreamSendEventTransaction* transaction = (HAPDataStreamSendEventTransaction*) transaction_;

                    HAPBaseDataStreamProtocol* dataStreamProtocol = transaction->dataStreamProtocol;

                    HAPLogInfo(
                            &logObject,
                            "[%p.%u] HomeKit Data Stream invalidated. Invalidating %s.%s send event transaction.",
                            (const void*) dispatcher,
                            dataStreamHandle,
                            dataStreamProtocol->base->name,
                            transaction->topic);

                    // Abort ongoing message operation.
                    HAPAssert(transaction->messageCompletionHandler);
                    HAPDataStreamSendEventCompletionHandler completionHandler = transaction->messageCompletionHandler;
                    void* _Nullable messageBytes = transaction->message.mutableBytes;
                    size_t numMessageBytes = transaction->numMessageBytes;
                    DeregisterTransaction(server, dispatcher, dataStreamHandle, transaction_);
                    HAPRawBufferZero(HAPNonnull(transaction_), sizeof *transaction_);
                    completionHandler(
                            server,
                            dispatcher,
                            dataStreamProtocol,
                            request,
                            dataStreamHandle,
                            HAPNonnull(transaction_),
                            kHAPError_InvalidState,
                            messageBytes,
                            numMessageBytes,
                            context);
                    break;
                }
                case kHAPDataStreamTransactionType_SendRequest: {
                    HAPDataStreamSendRequestTransaction* transaction_ = baseTransaction_;
                    HAPDataStreamSendRequestTransaction* transaction =
                            (HAPDataStreamSendRequestTransaction*) transaction_;

                    HAPBaseDataStreamProtocol* dataStreamProtocol = transaction->dataStreamProtocol;

                    HAPLogInfo(
                            &logObject,
                            "[%p.%u] HomeKit Data Stream invalidated. Invalidating %s.%s:%lld send request "
                            "transaction.",
                            (const void*) dispatcher,
                            dataStreamHandle,
                            dataStreamProtocol->base->name,
                            transaction->topic,
                            (long long) transaction->requestID);

                    // Abort ongoing message operation.
                    if (transaction->messageCompletionHandler) {
                        HAPDataStreamSendRequestMessageCompletionHandler completionHandler =
                                transaction->messageCompletionHandler;
                        void* _Nullable messageBytes = transaction->message.mutableBytes;
                        size_t numMessageBytes = transaction->numMessageBytes;
                        transaction->messageCompletionHandler = NULL;
                        transaction->_.messageIsMutable = false;
                        transaction->message.mutableBytes = NULL;
                        transaction->numMessageBytes = 0;
                        completionHandler(
                                server,
                                dispatcher,
                                dataStreamProtocol,
                                request,
                                dataStreamHandle,
                                HAPNonnull(transaction_),
                                kHAPError_InvalidState,
                                messageBytes,
                                numMessageBytes,
                                false,
                                context);
                    }
                    DeregisterTransaction(server, dispatcher, dataStreamHandle, transaction_);
                    HAPRawBufferZero(HAPNonnull(transaction_), sizeof *transaction_);
                    // Informing "control" HomeKit Data Stream protocol handler below
                    // will notify all delegates of the stream invalidation in a safe state,
                    // where the transaction linked list is empty.
                    // Hence, delegate handleInvalidate must not be called here.
                    break;
                }
                case kHAPDataStreamTransactionType_ReceiveRequest: {
                    HAPDataStreamReceiveRequestTransaction* transaction_ = baseTransaction_;
                    HAPDataStreamReceiveRequestTransaction* transaction =
                            (HAPDataStreamReceiveRequestTransaction*) transaction_;

                    HAPBaseDataStreamProtocol* dataStreamProtocol = transaction->dataStreamProtocol;

                    HAPLogInfo(
                            &logObject,
                            "[%p.%u] HomeKit Data Stream invalidated. Invalidating %s.%s:%lld receive request "
                            "transaction.",
                            (const void*) dispatcher,
                            dataStreamHandle,
                            dataStreamProtocol->base->name,
                            transaction->topic,
                            (long long) transaction->requestID);

                    // Abort ongoing message operation.
                    if (transaction->messageCompletionHandler) {
                        HAPDataStreamReceiveRequestMessageCompletionHandler completionHandler =
                                transaction->messageCompletionHandler;
                        void* _Nullable messageBytes = transaction->message.mutableBytes;
                        size_t numMessageBytes = transaction->numMessageBytes;
                        transaction->messageCompletionHandler = NULL;
                        transaction->_.messageIsMutable = false;
                        transaction->message.mutableBytes = NULL;
                        transaction->numMessageBytes = 0;
                        completionHandler(
                                server,
                                dispatcher,
                                dataStreamProtocol,
                                request,
                                dataStreamHandle,
                                HAPNonnull(transaction_),
                                kHAPError_InvalidState,
                                messageBytes,
                                numMessageBytes,
                                false,
                                context);
                    }
                    DeregisterTransaction(server, dispatcher, dataStreamHandle, transaction_);
                    HAPRawBufferZero(HAPNonnull(transaction_), sizeof *transaction_);
                    // Informing "control" HomeKit Data Stream protocol handler below
                    // will notify all delegates of the stream invalidation in a safe state,
                    // where the transaction linked list is empty.
                    // Hence, delegate handleInvalidate must not be called here.
                    break;
                }
            }
        }
        HAP_DIAGNOSTIC_POP

        // Inform "control" HomeKit Data Stream protocol handler.
        HAPBaseDataStreamProtocol* _Nullable controlDataStreamProtocol =
                HAPDataStreamDispatcherFindHandlerForProtocolName(
                        server, dispatcher, kHAPControlDataStreamProtocol_Name);
        HAPAssert(controlDataStreamProtocol);
        HAPLogInfo(
                &logObject,
                "[%p.%u] HomeKit Data Stream invalidated. Informing %s protocol handler.",
                (const void*) dispatcher,
                dataStreamHandle,
                controlDataStreamProtocol->base->name);
        HAPAssert(controlDataStreamProtocol->base->callbacks.handleInvalidate);
        controlDataStreamProtocol->base->callbacks.handleInvalidate(
                server, dispatcher, HAPNonnull(controlDataStreamProtocol), request, dataStreamHandle, context);

        // Reset HomeKit Data Stream.
        HAPRawBufferZero(dataStreamDescriptor, sizeof *dataStreamDescriptor);
        return;
    }

    HAPLog(&logObject,
           "[%p] HomeKit Data Stream [%p] invalidated (not accepted).",
           (const void*) dispatcher,
           (const void*) dataStream);
}

void HAPDataStreamDispatcherClearTransaction(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataStreamTransactionRef* transaction_) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream < dispatcher->storage->numDataStreams);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStream];
    HAPPrecondition(transaction_);
    HAPDataStreamBaseTransaction* transaction = transaction_;

    if (!dataStreamDescriptor->firstTransaction) {
        return;
    }

    // Remove transaction from linked list.
    if (dataStreamDescriptor->firstTransaction == transaction_) {
        dataStreamDescriptor->firstTransaction = transaction->nextTransaction;
        transaction->nextTransaction = NULL;
    } else {
        HAPDataStreamTransactionRef* _Nullable previousTransaction_ = dataStreamDescriptor->firstTransaction;
        while (previousTransaction_) {
            HAPDataStreamBaseTransaction* previousTransaction = previousTransaction_;

            if (previousTransaction->nextTransaction == transaction_) {
                previousTransaction->nextTransaction = transaction->nextTransaction;
                transaction->nextTransaction = NULL;
                return;
            }

            previousTransaction_ = previousTransaction->nextTransaction;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Begins receiving a new packet asynchronously.
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      dataStreamHandle     HomeKit Data Stream handle.
 */
static void BeginReceivingPacket(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStreamHandle);

/**
 * Begins sending a new packet asynchronously.
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      dataStreamHandle     HomeKit Data Stream handle.
 */
static void BeginSendingPacket(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStreamHandle);

//----------------------------------------------------------------------------------------------------------------------

static void HandleReceiveEventMessageComplete(
        HAPAccessoryServer* server,
        const HAPDataStreamRequest* request,
        HAPDataStreamRef* dataStream,
        HAPError error,
        void* _Nullable dataBytes,
        size_t numDataBytes,
        bool isComplete,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->accessory);
    HAPPrecondition(request->accessory->dataStream.delegate.callbacks == &kHAPDataStreamDispatcher_DataStreamCallbacks);
    HAPDataStreamDispatcher* dispatcher = request->accessory->dataStream.delegate.context;
    HAPPrecondition(dataStream);
    HAPDataStreamHandle dataStreamHandle = HAPDataStreamGetHandleForDataStream(server, dispatcher, dataStream);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStreamHandle];
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        HAPDataStreamInvalidate(server, dataStream);
        return;
    }
    HAPAssert(dataStreamDescriptor->numDataBytes >= numDataBytes);
    dataStreamDescriptor->numDataBytes -= (uint32_t) numDataBytes;
    HAPAssert(isComplete == !dataStreamDescriptor->numDataBytes);

    HAPDataStreamProtocol* _Nullable dataStreamProtocol = dataStreamDescriptor->_.event.dataStreamProtocol;

    // Complete receiving Message.
    HAPAssert(dataStreamDescriptor->receiveState == kHAPDataStreamDescriptorReceiveState_ReceivingMessage);
    if (!dataBytes) {
        HAPLogDebug(&logObject, "[%p.%u] Skipped event Message.", (const void*) dispatcher, dataStreamHandle);
    } else if (dataBytes == dataStreamDescriptor->_.event.messageBytes) {
        HAPLogDebug(&logObject, "[%p.%u] Received event Message (short).", (const void*) dispatcher, dataStreamHandle);
    } else {
        HAPLogDebug(&logObject, "[%p.%u] Received event Message.", (const void*) dispatcher, dataStreamHandle);
    }

    // Inform completion handler.
    HAPAssert(isComplete);
    HAPDataStreamReceiveEventMessageCompletionHandler _Nullable completionHandler =
            dataStreamDescriptor->_.event.completionHandler;
    HAPAssert(completionHandler);
    uint8_t messageBytes[kHAPDataStreamDispatcher_NumShortMessageBytes];
    if (dataBytes == dataStreamDescriptor->_.event.messageBytes) {
        HAPAssert(numDataBytes <= sizeof messageBytes);
        HAPRawBufferCopyBytes(messageBytes, HAPNonnullVoid(dataBytes), numDataBytes);
        dataBytes = messageBytes;
    }
    dataStreamDescriptor->receiveState = kHAPDataStreamDescriptorReceiveState_Idle;
    HAPRawBufferZero(&dataStreamDescriptor->_, sizeof dataStreamDescriptor->_);
    completionHandler(
            server, dispatcher, dataStreamProtocol, request, dataStreamHandle, error, dataBytes, numDataBytes, context);
    if (dataBytes == messageBytes) {
        HAPAssert(numDataBytes <= sizeof messageBytes);
        HAPRawBufferZero(HAPNonnullVoid(dataBytes), numDataBytes);
    }

    // Continue receiving.
    if (dataStreamDescriptor->receiveState == kHAPDataStreamDescriptorReceiveState_Idle) {
        BeginReceivingPacket(server, dispatcher, dataStreamHandle);
    }
}

static void ReceiveEventMessage(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        void* _Nullable dataBytes,
        size_t numDataBytes,
        HAPDataStreamReceiveEventMessageCompletionHandler completionHandler) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream < dispatcher->storage->numDataStreams);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStream];
    HAPPrecondition(dataStreamDescriptor->receiveState == kHAPDataStreamDescriptorReceiveState_EventAvailable);
    HAPPrecondition(numDataBytes == dataStreamDescriptor->numDataBytes);
    HAPPrecondition(completionHandler);
    HAPDataStreamRequest request;
    HAPDataStreamGetRequestContext(server, HAPNonnull(dataStreamDescriptor->dataStream), &request);
    HAPError error = kHAPError_None;
    bool isComplete = !dataStreamDescriptor->numDataBytes;
    void* _Nullable context = server->context;

    // Receive Message.
    dataStreamDescriptor->receiveState = kHAPDataStreamDescriptorReceiveState_ReceivingMessage;
    dataStreamDescriptor->_.event.completionHandler = completionHandler;
    if (!dataBytes) {
        HAPLogDebug(&logObject, "[%p.%u] Skipping event Message.", (const void*) dispatcher, dataStream);
    } else if (dataBytes == dataStreamDescriptor->_.event.messageBytes) {
        HAPAssert(numDataBytes <= sizeof dataStreamDescriptor->_.event.messageBytes);
        HAPLogDebug(&logObject, "[%p.%u] Receiving event Message (short).", (const void*) dispatcher, dataStream);
    } else {
        HAPLogDebug(&logObject, "[%p.%u] Receiving event Message.", (const void*) dispatcher, dataStream);
    }
    if (!isComplete) {
        if (dataBytes) {
            HAPDataStreamReceiveData(
                    server,
                    HAPNonnull(dataStreamDescriptor->dataStream),
                    HAPNonnullVoid(dataBytes),
                    numDataBytes,
                    HandleReceiveEventMessageComplete);
        } else {
            HAPDataStreamSkipData(
                    server,
                    HAPNonnull(dataStreamDescriptor->dataStream),
                    numDataBytes,
                    HandleReceiveEventMessageComplete);
        }
    } else {
        HandleReceiveEventMessageComplete(
                server,
                &request,
                dataStreamDescriptor->dataStream,
                error,
                dataBytes,
                numDataBytes,
                isComplete,
                context);
    }
}

void HAPDataStreamDispatcherReceiveEvent(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        void* messageBytes,
        size_t numMessageBytes,
        HAPDataStreamReceiveEventMessageCompletionHandler completionHandler) {
    ReceiveEventMessage(server, dispatcher, dataStream, messageBytes, numMessageBytes, completionHandler);
}

void HAPDataStreamDispatcherSkipEvent(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataStreamReceiveEventMessageCompletionHandler completionHandler) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream < dispatcher->storage->numDataStreams);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStream];

    void* _Nullable dataBytes = NULL;
    size_t numDataBytes = dataStreamDescriptor->numDataBytes;
    ReceiveEventMessage(server, dispatcher, dataStream, dataBytes, numDataBytes, completionHandler);
}

void HAPDataStreamDispatcherReceiveShortEvent(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataStreamReceiveEventMessageCompletionHandler completionHandler) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream < dispatcher->storage->numDataStreams);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStream];

    if ((size_t) dataStreamDescriptor->numDataBytes <= sizeof dataStreamDescriptor->_.event.messageBytes) {
        void* dataBytes = dataStreamDescriptor->_.event.messageBytes;
        size_t numDataBytes = dataStreamDescriptor->numDataBytes;
        ReceiveEventMessage(server, dispatcher, dataStream, dataBytes, numDataBytes, completionHandler);
    } else {
        HAPLogDebug(
                &logObject,
                "[%p.%u] Skipping event Message (too long - %zu bytes).",
                (const void*) dispatcher,
                dataStream,
                (size_t) dataStreamDescriptor->numDataBytes);
        HAPDataStreamDispatcherSkipEvent(server, dispatcher, dataStream, completionHandler);
    }
}

//----------------------------------------------------------------------------------------------------------------------

static void HandleReceiveRequestMessageComplete(
        HAPAccessoryServer* server,
        const HAPDataStreamRequest* request,
        HAPDataStreamRef* dataStream,
        HAPError error,
        void* _Nullable dataBytes,
        size_t numDataBytes,
        bool isComplete,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->accessory);
    HAPPrecondition(request->accessory->dataStream.delegate.callbacks == &kHAPDataStreamDispatcher_DataStreamCallbacks);
    HAPDataStreamDispatcher* dispatcher = request->accessory->dataStream.delegate.context;
    HAPPrecondition(dataStream);
    HAPDataStreamHandle dataStreamHandle = HAPDataStreamGetHandleForDataStream(server, dispatcher, dataStream);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStreamHandle];
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        HAPDataStreamInvalidate(server, dataStream);
        return;
    }
    HAPAssert(dataStreamDescriptor->numDataBytes >= numDataBytes);
    dataStreamDescriptor->numDataBytes -= (uint32_t) numDataBytes;
    HAPAssert(isComplete == !dataStreamDescriptor->numDataBytes);

    HAPDataStreamReceiveRequestTransaction* transaction_ =
            GetReceivingTransaction(server, dispatcher, dataStreamHandle);
    HAPDataStreamReceiveRequestTransaction* transaction = (HAPDataStreamReceiveRequestTransaction*) transaction_;
    HAPAssert(transaction->_.type == kHAPDataStreamTransactionType_ReceiveRequest);

    HAPDataStreamProtocol* dataStreamProtocol = transaction->dataStreamProtocol;

    // Complete receiving Message.
    HAPAssert(dataStreamDescriptor->receiveState == kHAPDataStreamDescriptorReceiveState_ReceivingMessage);
    HAPAssert(transaction->state == kHAPDataStreamReceiveRequestTransactionState_ReceivingRequest);
    HAPAssert(dataBytes == transaction->message.mutableBytes);
    HAPAssert(numDataBytes == transaction->numMessageBytes);
    if (!dataBytes) {
        HAPLogDebug(&logObject, "[%p.%u] Skipped request Message.", (const void*) dispatcher, dataStreamHandle);
    } else if (dataBytes == dataStreamDescriptor->_.messageBytes) {
        HAPLogDebug(
                &logObject, "[%p.%u] Received request Message (short).", (const void*) dispatcher, dataStreamHandle);
    } else {
        HAPLogDebug(&logObject, "[%p.%u] Received request Message.", (const void*) dispatcher, dataStreamHandle);
    }

    // Inform completion handler.
    HAPAssert(isComplete);
    HAPDataStreamReceiveRequestMessageCompletionHandler _Nullable completionHandler =
            transaction->messageCompletionHandler;
    HAPAssert(completionHandler);
    uint8_t messageBytes[kHAPDataStreamDispatcher_NumShortMessageBytes];
    if (dataBytes == dataStreamDescriptor->_.messageBytes) {
        HAPAssert(numDataBytes <= sizeof messageBytes);
        HAPRawBufferCopyBytes(messageBytes, HAPNonnullVoid(dataBytes), numDataBytes);
        dataBytes = messageBytes;
    }
    dataStreamDescriptor->receiveState = kHAPDataStreamDescriptorReceiveState_Idle;
    transaction->state = kHAPDataStreamReceiveRequestTransactionState_WaitingForResponse;
    transaction->messageCompletionHandler = NULL;
    transaction->_.messageIsMutable = false;
    transaction->message.mutableBytes = NULL;
    transaction->numMessageBytes = 0;
    HAPRawBufferZero(&dataStreamDescriptor->_, sizeof dataStreamDescriptor->_);
    completionHandler(
            server,
            dispatcher,
            dataStreamProtocol,
            request,
            dataStreamHandle,
            transaction_,
            error,
            dataBytes,
            numDataBytes,
            false,
            context);
    if (dataBytes == messageBytes) {
        HAPAssert(numDataBytes <= sizeof messageBytes);
        HAPRawBufferZero(HAPNonnullVoid(dataBytes), numDataBytes);
    }

    // Continue receiving.
    if (dataStreamDescriptor->receiveState == kHAPDataStreamDescriptorReceiveState_Idle) {
        BeginReceivingPacket(server, dispatcher, dataStreamHandle);
    }
}

static void ReceiveRequestMessage(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataStreamReceiveRequestTransaction* transaction_,
        const HAPDataStreamReceiveRequestTransactionCallbacks* callbacks,
        const char* topic,
        void* _Nullable dataBytes,
        size_t numDataBytes,
        HAPDataStreamReceiveRequestMessageCompletionHandler completionHandler) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream < dispatcher->storage->numDataStreams);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStream];
    HAPPrecondition(dataStreamDescriptor->receiveState == kHAPDataStreamDescriptorReceiveState_RequestAvailable);
    HAPPrecondition(transaction_);
    HAPPrecondition(!IsTransactionRegistered(server, dispatcher, dataStream, transaction_));
    HAPDataStreamReceiveRequestTransaction* transaction = (HAPDataStreamReceiveRequestTransaction*) transaction_;
    HAPPrecondition(callbacks);
    HAPPrecondition(callbacks->handleInvalidate);
    const char* protocolName = &dataStreamDescriptor->_.request.nameAndTopic[0];
    size_t numNameBytes = HAPStringGetNumBytes(protocolName);
    const char* cachedTopic = &dataStreamDescriptor->_.request.nameAndTopic[numNameBytes + 1];
    size_t numTopicBytes = HAPStringGetNumBytes(cachedTopic);
    HAPAssert(sizeof dataStreamDescriptor->_.request.nameAndTopic >= numNameBytes + 1 + numTopicBytes + 1);
    HAPPrecondition(topic);
    HAPPrecondition(topic != cachedTopic);
    HAPPrecondition(HAPStringAreEqual(topic, cachedTopic));
    HAPDataStreamProtocol* _Nullable dataStreamProtocol_ =
            HAPDataStreamDispatcherFindHandlerForProtocolName(server, dispatcher, protocolName);
    HAPPrecondition(dataStreamProtocol_);
    HAPBaseDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(HAPDataStreamRequestNameAndTopicAreValid(dataStreamProtocol->base->name, topic));
    HAPPrecondition(numDataBytes == dataStreamDescriptor->numDataBytes);
    HAPPrecondition(completionHandler);
    HAPDataStreamRequest request;
    HAPDataStreamGetRequestContext(server, HAPNonnull(dataStreamDescriptor->dataStream), &request);
    HAPError error = kHAPError_None;
    bool isComplete = !dataStreamDescriptor->numDataBytes;
    void* _Nullable context = server->context;

    // Initialize transaction.
    HAPRawBufferZero(transaction_, sizeof *transaction_);
    transaction->_.type = kHAPDataStreamTransactionType_ReceiveRequest;
    transaction->dataStreamProtocol = dataStreamProtocol;
    transaction->topic = topic;
    transaction->requestID = dataStreamDescriptor->_.request.requestID;
    transaction->callbacks = callbacks;
    RegisterTransaction(server, dispatcher, dataStream, transaction_);

    // Receive Message.
    dataStreamDescriptor->receiveState = kHAPDataStreamDescriptorReceiveState_ReceivingMessage;
    transaction->messageCompletionHandler = completionHandler;
    transaction->_.messageIsMutable = true;
    transaction->message.mutableBytes = dataBytes;
    transaction->numMessageBytes = (uint32_t) numDataBytes;
    transaction->state = kHAPDataStreamReceiveRequestTransactionState_ReceivingRequest;
    if (!dataBytes) {
        HAPLogDebug(&logObject, "[%p.%u] Skipping request Message.", (const void*) dispatcher, dataStream);
    } else if (dataBytes == dataStreamDescriptor->_.messageBytes) {
        HAPAssert(numDataBytes <= sizeof dataStreamDescriptor->_.messageBytes);
        HAPLogDebug(&logObject, "[%p.%u] Receiving request Message (short).", (const void*) dispatcher, dataStream);
    } else {
        HAPLogDebug(&logObject, "[%p.%u] Receiving request Message.", (const void*) dispatcher, dataStream);
    }
    if (!isComplete) {
        if (dataBytes) {
            HAPDataStreamReceiveData(
                    server,
                    HAPNonnull(dataStreamDescriptor->dataStream),
                    HAPNonnullVoid(dataBytes),
                    numDataBytes,
                    HandleReceiveRequestMessageComplete);
        } else {
            HAPDataStreamSkipData(
                    server,
                    HAPNonnull(dataStreamDescriptor->dataStream),
                    numDataBytes,
                    HandleReceiveRequestMessageComplete);
        }
    } else {
        HandleReceiveRequestMessageComplete(
                server,
                &request,
                dataStreamDescriptor->dataStream,
                error,
                dataBytes,
                numDataBytes,
                isComplete,
                context);
    }
}

void HAPDataStreamDispatcherReceiveRequest(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataStreamReceiveRequestTransaction* transaction,
        const HAPDataStreamReceiveRequestTransactionCallbacks* callbacks,
        const char* topic,
        void* messageBytes,
        size_t numMessageBytes,
        HAPDataStreamReceiveRequestMessageCompletionHandler completionHandler) {
    ReceiveRequestMessage(
            server,
            dispatcher,
            dataStream,
            transaction,
            callbacks,
            topic,
            messageBytes,
            numMessageBytes,
            completionHandler);
}

void HAPDataStreamDispatcherSkipRequest(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataStreamReceiveRequestTransaction* transaction,
        const HAPDataStreamReceiveRequestTransactionCallbacks* callbacks,
        const char* topic,
        HAPDataStreamReceiveRequestMessageCompletionHandler completionHandler) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream < dispatcher->storage->numDataStreams);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStream];

    void* _Nullable dataBytes = NULL;
    size_t numDataBytes = dataStreamDescriptor->numDataBytes;
    ReceiveRequestMessage(
            server, dispatcher, dataStream, transaction, callbacks, topic, dataBytes, numDataBytes, completionHandler);
}

void HAPDataStreamDispatcherReceiveShortRequest(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataStreamReceiveRequestTransaction* transaction,
        const HAPDataStreamReceiveRequestTransactionCallbacks* callbacks,
        const char* topic,
        HAPDataStreamReceiveRequestMessageCompletionHandler completionHandler) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream < dispatcher->storage->numDataStreams);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStream];

    if ((size_t) dataStreamDescriptor->numDataBytes <= sizeof dataStreamDescriptor->_.messageBytes) {
        void* dataBytes = dataStreamDescriptor->_.messageBytes;
        size_t numDataBytes = dataStreamDescriptor->numDataBytes;
        ReceiveRequestMessage(
                server,
                dispatcher,
                dataStream,
                transaction,
                callbacks,
                topic,
                dataBytes,
                numDataBytes,
                completionHandler);
    } else {
        HAPLogDebug(
                &logObject,
                "[%p.%u] Skipping request Message (too long - %zu bytes).",
                (const void*) dispatcher,
                dataStream,
                (size_t) dataStreamDescriptor->numDataBytes);
        HAPDataStreamDispatcherSkipRequest(
                server, dispatcher, dataStream, transaction, callbacks, topic, completionHandler);
    }
}

//----------------------------------------------------------------------------------------------------------------------

static void HandleSkipRequestMessageComplete(
        HAPAccessoryServer* server,
        const HAPDataStreamRequest* request,
        HAPDataStreamRef* dataStream,
        HAPError error,
        void* _Nullable dataBytes,
        size_t numDataBytes,
        bool isComplete,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->accessory);
    HAPPrecondition(request->accessory->dataStream.delegate.callbacks == &kHAPDataStreamDispatcher_DataStreamCallbacks);
    HAPDataStreamDispatcher* dispatcher = request->accessory->dataStream.delegate.context;
    HAPPrecondition(dataStream);
    HAPDataStreamHandle dataStreamHandle = HAPDataStreamGetHandleForDataStream(server, dispatcher, dataStream);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStreamHandle];
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        HAPDataStreamInvalidate(server, dataStream);
        return;
    }
    HAPAssert(dataStreamDescriptor->numDataBytes >= numDataBytes);
    dataStreamDescriptor->numDataBytes -= (uint32_t) numDataBytes;
    HAPAssert(isComplete == !dataStreamDescriptor->numDataBytes);

    // Complete skipping Message.
    HAPAssert(dataStreamDescriptor->receiveState == kHAPDataStreamDescriptorReceiveState_SkippingRequestMessage);
    HAPAssert(dataBytes == NULL);
    HAPAssert(isComplete);
    HAPLogDebug(&logObject, "[%p.%u] Skipped request Message.", (const void*) dispatcher, dataStreamHandle);

    // Send response.
    dataStreamDescriptor->receiveState = kHAPDataStreamDescriptorReceiveState_WaitingForResponse;
    if (dataStreamDescriptor->sendState == kHAPDataStreamDescriptorSendState_Idle) {
        BeginSendingPacket(server, dispatcher, dataStreamHandle);
    } else {
        HAPLogInfo(
                &logObject,
                "[%p.%u] Delaying to send packet until previous packet is sent.",
                (const void*) dispatcher,
                dataStreamHandle);
    }
}

void HAPDataStreamDispatcherSkipAndReplyToRequest(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataStreamResponseStatus status,
        HAPDataStreamSkipAndReplyToRequestCompletionHandler completionHandler) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream < dispatcher->storage->numDataStreams);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStream];
    HAPPrecondition(dataStreamDescriptor->receiveState == kHAPDataStreamDescriptorReceiveState_RequestAvailable);
    HAPPrecondition(HAPDataStreamResponseStatusIsValid(status));
    HAPPrecondition(completionHandler);
    HAPDataStreamRequest request;
    HAPDataStreamGetRequestContext(server, HAPNonnull(dataStreamDescriptor->dataStream), &request);
    HAPError error = kHAPError_None;
    bool isComplete = !dataStreamDescriptor->numDataBytes;
    void* _Nullable context = server->context;

    // Skip Message.
    dataStreamDescriptor->receiveState = kHAPDataStreamDescriptorReceiveState_SkippingRequestMessage;
    dataStreamDescriptor->_.request.skipAndReplyToRequestCompletionHandler = completionHandler;
    dataStreamDescriptor->_.request.status = status;
    HAPLogDebug(&logObject, "[%p.%u] Skipping request Message.", (const void*) dispatcher, dataStream);
    if (!isComplete) {
        HAPDataStreamSkipData(
                server,
                HAPNonnull(dataStreamDescriptor->dataStream),
                dataStreamDescriptor->numDataBytes,
                HandleSkipRequestMessageComplete);
    } else {
        HandleSkipRequestMessageComplete(
                server,
                &request,
                dataStreamDescriptor->dataStream,
                error,
                NULL,
                dataStreamDescriptor->numDataBytes,
                isComplete,
                context);
    }
}

//----------------------------------------------------------------------------------------------------------------------

static void HandleReceiveResponseMessageComplete(
        HAPAccessoryServer* server,
        const HAPDataStreamRequest* request,
        HAPDataStreamRef* dataStream,
        HAPError error,
        void* _Nullable dataBytes,
        size_t numDataBytes,
        bool isComplete,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->accessory);
    HAPPrecondition(request->accessory->dataStream.delegate.callbacks == &kHAPDataStreamDispatcher_DataStreamCallbacks);
    HAPDataStreamDispatcher* dispatcher = request->accessory->dataStream.delegate.context;
    HAPPrecondition(dataStream);
    HAPDataStreamHandle dataStreamHandle = HAPDataStreamGetHandleForDataStream(server, dispatcher, dataStream);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStreamHandle];
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        HAPDataStreamInvalidate(server, dataStream);
        return;
    }
    HAPAssert(dataStreamDescriptor->numDataBytes >= numDataBytes);
    dataStreamDescriptor->numDataBytes -= (uint32_t) numDataBytes;
    HAPAssert(isComplete == !dataStreamDescriptor->numDataBytes);

    HAPDataStreamSendRequestTransaction* transaction_ = GetReceivingTransaction(server, dispatcher, dataStreamHandle);
    HAPDataStreamSendRequestTransaction* transaction = (HAPDataStreamSendRequestTransaction*) transaction_;
    HAPAssert(transaction->_.type == kHAPDataStreamTransactionType_SendRequest);

    HAPBaseDataStreamProtocol* dataStreamProtocol = transaction->dataStreamProtocol;

    // Complete receiving Message.
    HAPAssert(dataStreamDescriptor->receiveState == kHAPDataStreamDescriptorReceiveState_ReceivingMessage);
    HAPAssert(transaction->state == kHAPDataStreamSendRequestTransactionState_ReceivingResponse);
    HAPAssert(dataBytes == transaction->message.mutableBytes);
    HAPAssert(numDataBytes == transaction->numMessageBytes);
    if (!dataBytes) {
        HAPLogDebug(&logObject, "[%p.%u] Skipped response Message.", (const void*) dispatcher, dataStreamHandle);
    } else if (dataBytes == dataStreamDescriptor->_.messageBytes) {
        HAPLogDebug(
                &logObject, "[%p.%u] Received response Message (short).", (const void*) dispatcher, dataStreamHandle);
    } else {
        HAPLogDebug(&logObject, "[%p.%u] Received response Message.", (const void*) dispatcher, dataStreamHandle);
    }

    // Inform completion handler.
    HAPAssert(isComplete);
    HAPDataStreamSendRequestMessageCompletionHandler _Nullable completionHandler =
            transaction->messageCompletionHandler;
    HAPAssert(completionHandler);
    uint8_t messageBytes[kHAPDataStreamDispatcher_NumShortMessageBytes];
    if (dataBytes == dataStreamDescriptor->_.messageBytes) {
        HAPAssert(numDataBytes <= sizeof messageBytes);
        HAPRawBufferCopyBytes(messageBytes, HAPNonnullVoid(dataBytes), numDataBytes);
        dataBytes = messageBytes;
    }
    dataStreamDescriptor->receiveState = kHAPDataStreamDescriptorReceiveState_Idle;
    DeregisterTransaction(server, dispatcher, dataStreamHandle, transaction_);
    HAPRawBufferZero(transaction_, sizeof *transaction_);
    completionHandler(
            server,
            dispatcher,
            dataStreamProtocol,
            request,
            dataStreamHandle,
            transaction_,
            error,
            dataBytes,
            numDataBytes,
            true,
            context);
    if (dataBytes == messageBytes) {
        HAPAssert(numDataBytes <= sizeof messageBytes);
        HAPRawBufferZero(HAPNonnullVoid(dataBytes), numDataBytes);
    }

    // Continue receiving.
    if (dataStreamDescriptor->receiveState == kHAPDataStreamDescriptorReceiveState_Idle) {
        BeginReceivingPacket(server, dispatcher, dataStreamHandle);
    }
}

static void ReceiveResponseMessage(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataStreamSendRequestTransaction* transaction_,
        void* _Nullable dataBytes,
        size_t numDataBytes,
        HAPDataStreamSendRequestMessageCompletionHandler completionHandler) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream < dispatcher->storage->numDataStreams);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStream];
    HAPPrecondition(dataStreamDescriptor->receiveState == kHAPDataStreamDescriptorReceiveState_ResponseAvailable);
    HAPPrecondition(transaction_);
    HAPDataStreamSendRequestTransaction* transaction = (HAPDataStreamSendRequestTransaction*) transaction_;
    HAPPrecondition(transaction->state == kHAPDataStreamSendRequestTransactionState_ResponseAvailable);
    HAPPrecondition(numDataBytes == dataStreamDescriptor->numDataBytes);
    HAPPrecondition(completionHandler);
    HAPDataStreamRequest request;
    HAPDataStreamGetRequestContext(server, HAPNonnull(dataStreamDescriptor->dataStream), &request);
    HAPError error = kHAPError_None;
    bool isComplete = !dataStreamDescriptor->numDataBytes;
    void* _Nullable context = server->context;

    // Receive Message.
    dataStreamDescriptor->receiveState = kHAPDataStreamDescriptorReceiveState_ReceivingMessage;
    transaction->messageCompletionHandler = completionHandler;
    transaction->_.messageIsMutable = true;
    transaction->message.mutableBytes = dataBytes;
    transaction->numMessageBytes = (uint32_t) numDataBytes;
    transaction->state = kHAPDataStreamSendRequestTransactionState_ReceivingResponse;
    if (!dataBytes) {
        HAPLogDebug(&logObject, "[%p.%u] Skipping response Message.", (const void*) dispatcher, dataStream);
    } else if (dataBytes == dataStreamDescriptor->_.messageBytes) {
        HAPAssert(numDataBytes <= sizeof dataStreamDescriptor->_.messageBytes);
        HAPLogDebug(&logObject, "[%p.%u] Receiving response Message (short).", (const void*) dispatcher, dataStream);
    } else {
        HAPLogDebug(&logObject, "[%p.%u] Receiving response Message.", (const void*) dispatcher, dataStream);
    }
    if (!isComplete) {
        if (dataBytes) {
            HAPDataStreamReceiveData(
                    server,
                    HAPNonnull(dataStreamDescriptor->dataStream),
                    HAPNonnullVoid(dataBytes),
                    numDataBytes,
                    HandleReceiveResponseMessageComplete);
        } else {
            HAPDataStreamSkipData(
                    server,
                    HAPNonnull(dataStreamDescriptor->dataStream),
                    numDataBytes,
                    HandleReceiveResponseMessageComplete);
        }
    } else {
        HandleReceiveResponseMessageComplete(
                server,
                &request,
                dataStreamDescriptor->dataStream,
                error,
                dataBytes,
                numDataBytes,
                isComplete,
                context);
    }
}

void HAPDataStreamDispatcherReceiveResponse(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataStreamSendRequestTransaction* transaction,
        void* messageBytes,
        size_t numMessageBytes,
        HAPDataStreamSendRequestMessageCompletionHandler completionHandler) {
    ReceiveResponseMessage(
            server, dispatcher, dataStream, transaction, messageBytes, numMessageBytes, completionHandler);
}

void HAPDataStreamDispatcherSkipResponse(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataStreamSendRequestTransaction* transaction,
        HAPDataStreamSendRequestMessageCompletionHandler completionHandler) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream < dispatcher->storage->numDataStreams);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStream];

    void* _Nullable dataBytes = NULL;
    size_t numDataBytes = dataStreamDescriptor->numDataBytes;
    ReceiveResponseMessage(server, dispatcher, dataStream, transaction, dataBytes, numDataBytes, completionHandler);
}

void HAPDataStreamDispatcherReceiveShortResponse(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataStreamSendRequestTransaction* transaction,
        HAPDataStreamSendRequestMessageCompletionHandler completionHandler) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream < dispatcher->storage->numDataStreams);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStream];

    if ((size_t) dataStreamDescriptor->numDataBytes <= sizeof dataStreamDescriptor->_.messageBytes) {
        void* dataBytes = dataStreamDescriptor->_.messageBytes;
        size_t numDataBytes = dataStreamDescriptor->numDataBytes;
        ReceiveResponseMessage(server, dispatcher, dataStream, transaction, dataBytes, numDataBytes, completionHandler);
    } else {
        HAPLogDebug(
                &logObject,
                "[%p.%u] Skipping response Message (too long - %zu bytes).",
                (const void*) dispatcher,
                dataStream,
                (size_t) dataStreamDescriptor->numDataBytes);
        HAPDataStreamDispatcherSkipResponse(server, dispatcher, dataStream, transaction, completionHandler);
    }
}

void HAPDataStreamDispatcherSetMinimumPriority(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPStreamPriority priority) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream < dispatcher->storage->numDataStreams);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStream];
    HAPDataStreamSetMinimumPriority(server, HAPNonnull(dataStreamDescriptor->dataStream), priority);
}

//----------------------------------------------------------------------------------------------------------------------

static void HandleSkipEventComplete(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* _Nullable dataStreamProtocol HAP_UNUSED,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPError error,
        void* _Nullable messageBytes,
        size_t numMessageBytes HAP_UNUSED,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->accessory);
    HAPPrecondition(request->accessory->dataStream.delegate.callbacks == &kHAPDataStreamDispatcher_DataStreamCallbacks);
    HAPPrecondition(dataStream < dispatcher->storage->numDataStreams);
    HAPPrecondition(!messageBytes);
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        return;
    }

    HAPLog(&logObject, "[%p.%u] Skipped event.", (const void*) dispatcher, dataStream);
}

static void HandleRejectRequestComplete(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* _Nullable dataStreamProtocol HAP_UNUSED,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPError error,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->accessory);
    HAPPrecondition(request->accessory->dataStream.delegate.callbacks == &kHAPDataStreamDispatcher_DataStreamCallbacks);
    HAPPrecondition(dataStream < dispatcher->storage->numDataStreams);
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        return;
    }

    HAPLog(&logObject, "[%p.%u] Rejected request.", (const void*) dispatcher, dataStream);
}

static void HandleReceiveHeaderComplete(
        HAPAccessoryServer* server,
        const HAPDataStreamRequest* request,
        HAPDataStreamRef* dataStream,
        HAPError error,
        void* _Nullable dataBytes,
        size_t numDataBytes,
        bool isComplete,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->accessory);
    HAPPrecondition(request->accessory->dataStream.delegate.callbacks == &kHAPDataStreamDispatcher_DataStreamCallbacks);
    HAPDataStreamDispatcher* dispatcher = request->accessory->dataStream.delegate.context;
    HAPPrecondition(dataStream);
    HAPDataStreamHandle dataStreamHandle = HAPDataStreamGetHandleForDataStream(server, dispatcher, dataStream);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStreamHandle];
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        HAPDataStreamInvalidate(server, dataStream);
        return;
    }
    HAPAssert(dataStreamDescriptor->numDataBytes >= numDataBytes);
    dataStreamDescriptor->numDataBytes -= (uint32_t) numDataBytes;
    HAPAssert(isComplete == !dataStreamDescriptor->numDataBytes);

    HAPError err;

    // Complete receiving Header.
    HAPAssert(dataStreamDescriptor->receiveState == kHAPDataStreamDescriptorReceiveState_ReceivingHeader);
    HAPAssert(dataBytes == dataStreamDescriptor->_.header.bytes);
    HAPAssert(numDataBytes == dataStreamDescriptor->_.header.numBytes);
    HAPLogDebug(&logObject, "[%p.%u] Received Header.", (const void*) dispatcher, dataStreamHandle);

    // Parse Header.
    HAPOPACKReader headerReader;
    HAPOPACKReaderCreate(&headerReader, dataStreamDescriptor->_.header.bytes, dataStreamDescriptor->_.header.numBytes);
    HAPOPACKStringDictionaryElement protocolElement, eventElement, requestElement;
    HAPOPACKStringDictionaryElement responseElement, idElement, statusElement;
    protocolElement.key = "protocol";
    eventElement.key = "event";
    requestElement.key = "request";
    responseElement.key = "response";
    idElement.key = "id";
    statusElement.key = "status";
    err = HAPOPACKStringDictionaryReaderGetAll(
            &headerReader,
            (HAPOPACKStringDictionaryElement* const[]) { &protocolElement,
                                                         &eventElement,
                                                         &requestElement,
                                                         &responseElement,
                                                         &idElement,
                                                         &statusElement,
                                                         NULL });
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        HAPLog(&logObject, "[%p.%u] Received malformed Header.", (const void*) dispatcher, dataStreamHandle);
        HAPDataStreamInvalidate(server, dataStream);
        return;
    }

    // Parse protocol.
    if (!protocolElement.value.exists) {
        HAPLog(&logObject,
               "[%p.%u] Received Header does not contain 'protocol' key.",
               (const void*) dispatcher,
               dataStreamHandle);
        HAPDataStreamInvalidate(server, dataStream);
        return;
    }
    char* protocolName;
    err = HAPOPACKReaderGetNextString(&protocolElement.value.reader, &protocolName);
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        HAPLog(&logObject,
               "[%p.%u] Received Header with malformed 'protocol'.",
               (const void*) dispatcher,
               dataStreamHandle);
        HAPDataStreamInvalidate(server, dataStream);
        return;
    }
    if (dataStreamDescriptor->controlState != kHAPControlDataStreamProtocolState_Connected &&
        !HAPStringAreEqual(protocolName, kHAPControlDataStreamProtocol_Name)) {
        HAPLog(&logObject,
               "[%p.%u] Received Header for %s protocol when only %s protocol is accessible.",
               (const void*) dispatcher,
               dataStreamHandle,
               protocolName,
               kHAPControlDataStreamProtocol_Name);
        HAPDataStreamInvalidate(server, dataStream);
        return;
    }
    HAPBaseDataStreamProtocol* _Nullable dataStreamProtocol =
            HAPDataStreamDispatcherFindHandlerForProtocolName(server, dispatcher, protocolName);

    // Verify message type.
    size_t numTopics = (eventElement.value.exists ? 1 : 0) + (requestElement.value.exists ? 1 : 0) +
                       (responseElement.value.exists ? 1 : 0);
    if (!numTopics) {
        HAPLog(&logObject,
               "[%p.%u] Received Header does not contain 'event' / 'request' / 'response' key.",
               (const void*) dispatcher,
               dataStreamHandle);
        HAPDataStreamInvalidate(server, dataStream);
        return;
    }
    if (numTopics != 1) {
        HAPLog(&logObject,
               "[%p.%u] Received Header contains multiple 'event' / 'request' / 'response' key.",
               (const void*) dispatcher,
               dataStreamHandle);
        HAPDataStreamInvalidate(server, dataStream);
        return;
    }

    // Handle message.
    if (eventElement.value.exists) {
        // Parse topic.
        char* topic;
        err = HAPOPACKReaderGetNextString(&eventElement.value.reader, &topic);
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            HAPLog(&logObject,
                   "[%p.%u] Received event Header with malformed 'event'.",
                   (const void*) dispatcher,
                   dataStreamHandle);
            HAPDataStreamInvalidate(server, dataStream);
            return;
        }

        // Check for spurious keys.
        if (idElement.value.exists) {
            HAPLog(&logObject,
                   "[%p.%u] Ignoring key 'id' in received event Header.",
                   (const void*) dispatcher,
                   dataStreamHandle);
        }
        if (statusElement.value.exists) {
            HAPLog(&logObject,
                   "[%p.%u] Ignoring key 'status' in received event Header.",
                   (const void*) dispatcher,
                   dataStreamHandle);
        }

        // Event message is well-formed.
        HAPLogInfo(
                &logObject,
                "[%p.%u] Received %s.%s event Header.",
                (const void*) dispatcher,
                dataStreamHandle,
                protocolName,
                topic);
        dataStreamDescriptor->receiveState = kHAPDataStreamDescriptorReceiveState_EventAvailable;
        dataStreamDescriptor->_.event.dataStreamProtocol = dataStreamProtocol;

        // Inform HomeKit Data Stream protocol handler.
        if (!dataStreamProtocol) {
            HAPLog(&logObject,
                   "[%p.%u] Skipping %s.%s event (protocol not supported).",
                   (const void*) dispatcher,
                   dataStreamHandle,
                   protocolName,
                   topic);
            HAPDataStreamDispatcherSkipEvent(server, dispatcher, dataStreamHandle, HandleSkipEventComplete);
            return;
        }
        dataStreamProtocol->base->callbacks.handleEventAvailable(
                server,
                dispatcher,
                HAPNonnull(dataStreamProtocol),
                request,
                dataStreamHandle,
                topic,
                dataStreamDescriptor->numDataBytes,
                context);
    } else if (requestElement.value.exists) {
        // Parse topic.
        char* topic;
        err = HAPOPACKReaderGetNextString(&requestElement.value.reader, &topic);
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            HAPLog(&logObject,
                   "[%p.%u] Received request Header with malformed 'request'.",
                   (const void*) dispatcher,
                   dataStreamHandle);
            HAPDataStreamInvalidate(server, dataStream);
            return;
        }
        if (!HAPDataStreamRequestNameAndTopicAreValid(protocolName, topic)) {
            HAPLog(&logObject,
                   "[%p.%u] Received request Header with protocol name and topic combination %s.%s "
                   "that does not allow forming a response.",
                   (const void*) dispatcher,
                   dataStreamHandle,
                   protocolName,
                   topic);
            HAPDataStreamInvalidate(server, dataStream);
            return;
        }
        size_t numNameBytes = HAPStringGetNumBytes(protocolName);
        size_t numTopicBytes = HAPStringGetNumBytes(topic);
        char nameAndTopic[sizeof dataStreamDescriptor->_.request.nameAndTopic];
        HAPAssert(sizeof nameAndTopic >= numNameBytes + 1 + numTopicBytes + 1);
        HAPRawBufferZero(nameAndTopic, sizeof nameAndTopic);
        HAPRawBufferCopyBytes(&nameAndTopic[0], protocolName, numNameBytes + 1);
        HAPRawBufferCopyBytes(&nameAndTopic[numNameBytes + 1], topic, numTopicBytes + 1);
        protocolName = &nameAndTopic[0];
        topic = &nameAndTopic[numNameBytes + 1];

        // Parse id.
        if (!idElement.value.exists) {
            HAPLog(&logObject,
                   "[%p.%u] Received request Header does not contain 'id' key.",
                   (const void*) dispatcher,
                   dataStreamHandle);
            HAPDataStreamInvalidate(server, dataStream);
            return;
        }
        int64_t requestID;
        err = HAPOPACKReaderGetNextInt(&idElement.value.reader, &requestID);
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            HAPLog(&logObject,
                   "[%p.%u] Received request Header with malformed 'id'.",
                   (const void*) dispatcher,
                   dataStreamHandle);
            HAPDataStreamInvalidate(server, dataStream);
            return;
        }

        // Check for spurious keys.
        if (statusElement.value.exists) {
            HAPLog(&logObject,
                   "[%p.%u] Ignoring key 'status' in received request Header.",
                   (const void*) dispatcher,
                   dataStreamHandle);
        }

        // Request message is well-formed.
        HAPLogInfo(
                &logObject,
                "[%p.%u] Received %s.%s:%lld request Header.",
                (const void*) dispatcher,
                dataStreamHandle,
                protocolName,
                topic,
                (long long) requestID);
        dataStreamDescriptor->receiveState = kHAPDataStreamDescriptorReceiveState_RequestAvailable;
        dataStreamDescriptor->_.request.requestID = requestID;
        HAPAssert(sizeof dataStreamDescriptor->_.request.nameAndTopic == sizeof nameAndTopic);
        HAPRawBufferCopyBytes(dataStreamDescriptor->_.request.nameAndTopic, nameAndTopic, sizeof nameAndTopic);

        // Inform HomeKit Data Stream protocol handler.
        if (!dataStreamProtocol) {
            HAPLog(&logObject,
                   "[%p.%u] Rejecting %s.%s request (protocol not supported).",
                   (const void*) dispatcher,
                   dataStreamHandle,
                   protocolName,
                   topic);
            HAPDataStreamDispatcherSkipAndReplyToRequest(
                    server,
                    dispatcher,
                    dataStreamHandle,
                    kHAPDataStreamResponseStatus_MissingProtocol,
                    HandleRejectRequestComplete);
            return;
        }
        dataStreamProtocol->base->callbacks.handleRequestAvailable(
                server,
                dispatcher,
                HAPNonnull(dataStreamProtocol),
                request,
                dataStreamHandle,
                topic,
                dataStreamDescriptor->numDataBytes,
                context);
    } else {
        HAPAssert(responseElement.value.exists);

        // Parse topic.
        char* topic;
        err = HAPOPACKReaderGetNextString(&responseElement.value.reader, &topic);
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            HAPLog(&logObject,
                   "[%p.%u] Received response Header with malformed 'response'.",
                   (const void*) dispatcher,
                   dataStreamHandle);
            HAPDataStreamInvalidate(server, dataStream);
            return;
        }

        // Parse id.
        if (!idElement.value.exists) {
            HAPLog(&logObject,
                   "[%p.%u] Received response Header does not contain 'id' key.",
                   (const void*) dispatcher,
                   dataStreamHandle);
            HAPDataStreamInvalidate(server, dataStream);
            return;
        }
        int64_t requestID;
        err = HAPOPACKReaderGetNextInt(&idElement.value.reader, &requestID);
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            HAPLog(&logObject,
                   "[%p.%u] Received response Header with malformed 'id'.",
                   (const void*) dispatcher,
                   dataStreamHandle);
            HAPDataStreamInvalidate(server, dataStream);
            return;
        }

        // Parse status.
        if (!statusElement.value.exists) {
            HAPLog(&logObject,
                   "[%p.%u] Received response Header does not contain 'status' key.",
                   (const void*) dispatcher,
                   dataStreamHandle);
            HAPDataStreamInvalidate(server, dataStream);
            return;
        }
        int64_t rawStatus;
        err = HAPOPACKReaderGetNextInt(&statusElement.value.reader, &rawStatus);
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            HAPLog(&logObject,
                   "[%p.%u] Received response Header with malformed 'status'.",
                   (const void*) dispatcher,
                   dataStreamHandle);
            HAPDataStreamInvalidate(server, dataStream);
            return;
        }
        if (!HAPDataStreamResponseStatusIsValid(rawStatus)) {
            HAPLog(&logObject,
                   "[%p.%u] Received response Header with out-of-range 'status' (%lld).",
                   (const void*) dispatcher,
                   dataStreamHandle,
                   (long long) rawStatus);
            HAPDataStreamInvalidate(server, dataStream);
            return;
        }
        HAPDataStreamResponseStatus status = (HAPDataStreamResponseStatus) rawStatus;

        // Response message is well-formed.
        HAPLogInfo(
                &logObject,
                "[%p.%u] Received %s.%s:%lld(0x%02llX) response Header.",
                (const void*) dispatcher,
                dataStreamHandle,
                protocolName,
                topic,
                (long long) requestID,
                (long long) status);
        HAPDataStreamTransactionRef* _Nullable baseTransaction_ = dataStreamDescriptor->firstTransaction;
        while (baseTransaction_) {
            HAPDataStreamBaseTransaction* baseTransaction = baseTransaction_;
            HAPAssert(HAPDataStreamTransactionTypeIsValid(baseTransaction->_.type));

            if (baseTransaction->_.type == kHAPDataStreamTransactionType_SendRequest) {
                HAPDataStreamSendRequestTransaction* transaction_ = baseTransaction_;
                HAPDataStreamSendRequestTransaction* transaction = (HAPDataStreamSendRequestTransaction*) transaction_;

                if (transaction->state == kHAPDataStreamSendRequestTransactionState_WaitingForResponse &&
                    transaction->requestID == requestID) {
                    break;
                }
            }

            baseTransaction_ = baseTransaction->nextTransaction;
        }
        if (!baseTransaction_) {
            HAPLog(&logObject,
                   "[%p.%u] Received %s.%s response Header with unexpected 'id'.",
                   (const void*) dispatcher,
                   dataStreamHandle,
                   protocolName,
                   topic);
            HAPDataStreamInvalidate(server, dataStream);
            return;
        }
        HAPDataStreamBaseTransaction* baseTransaction = baseTransaction_;
        HAPAssert(baseTransaction->_.type == kHAPDataStreamTransactionType_SendRequest);
        HAPDataStreamSendRequestTransaction* transaction_ = baseTransaction_;
        HAPDataStreamSendRequestTransaction* transaction = (HAPDataStreamSendRequestTransaction*) transaction_;

        if (dataStreamProtocol != transaction->dataStreamProtocol) {
            HAPBaseDataStreamProtocol* requestDataStreamProtocol = transaction->dataStreamProtocol;
            HAPLog(&logObject,
                   "[%p.%u] Received %s.%s response Header with different 'protocol' than %s.%s request.",
                   (const void*) dispatcher,
                   dataStreamHandle,
                   protocolName,
                   topic,
                   requestDataStreamProtocol->base->name,
                   transaction->topic);
            HAPDataStreamInvalidate(server, dataStream);
            return;
        }
        if (!HAPStringAreEqual(topic, transaction->topic)) {
            HAPLog(&logObject,
                   "[%p.%u] Received %s.%s response Header with different 'protocol' than %s.%s request.",
                   (const void*) dispatcher,
                   dataStreamHandle,
                   protocolName,
                   topic,
                   protocolName,
                   transaction->topic);
            HAPDataStreamInvalidate(server, dataStream);
            return;
        }
        dataStreamDescriptor->receiveState = kHAPDataStreamDescriptorReceiveState_ResponseAvailable;
        transaction->state = kHAPDataStreamSendRequestTransactionState_ResponseAvailable;

        // Inform HomeKit Data Stream protocol handler.
        transaction->callbacks->handleResponseAvailable(
                server,
                dispatcher,
                HAPNonnull(dataStreamProtocol),
                request,
                dataStreamHandle,
                HAPNonnull(transaction_),
                transaction->topic,
                status,
                dataStreamDescriptor->numDataBytes,
                context);
    }
}

static void HandleReceiveHeaderLenComplete(
        HAPAccessoryServer* server,
        const HAPDataStreamRequest* request,
        HAPDataStreamRef* dataStream,
        HAPError error,
        void* _Nullable dataBytes,
        size_t numDataBytes,
        bool isComplete,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->accessory);
    HAPPrecondition(request->accessory->dataStream.delegate.callbacks == &kHAPDataStreamDispatcher_DataStreamCallbacks);
    HAPDataStreamDispatcher* dispatcher = request->accessory->dataStream.delegate.context;
    HAPPrecondition(dataStream);
    HAPDataStreamHandle dataStreamHandle = HAPDataStreamGetHandleForDataStream(server, dispatcher, dataStream);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStreamHandle];
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        HAPDataStreamInvalidate(server, dataStream);
        return;
    }
    HAPAssert(dataStreamDescriptor->numDataBytes >= numDataBytes);
    dataStreamDescriptor->numDataBytes -= (uint32_t) numDataBytes;
    HAPAssert(isComplete == !dataStreamDescriptor->numDataBytes);

    // Complete receiving HeaderLen.
    HAPAssert(dataStreamDescriptor->receiveState == kHAPDataStreamDescriptorReceiveState_ReceivingHeader);
    HAPAssert(dataBytes == &dataStreamDescriptor->_.header.numBytes);
    HAPAssert(numDataBytes == sizeof dataStreamDescriptor->_.header.numBytes);
    HAPLogDebug(
            &logObject,
            "[%p.%u] Received HeaderLen: %u.",
            (const void*) dispatcher,
            dataStreamHandle,
            dataStreamDescriptor->_.header.numBytes);

    // Receive Header.
    if (dataStreamDescriptor->numDataBytes < dataStreamDescriptor->_.header.numBytes) {
        HAPLog(&logObject,
               "[%p.%u] Received packet is too short for Header (remaining packet length: %zu bytes).",
               (const void*) dispatcher,
               dataStreamHandle,
               (size_t) dataStreamDescriptor->numDataBytes);
        HAPDataStreamInvalidate(server, dataStream);
        return;
    }
    HAPLogDebug(&logObject, "[%p.%u] Receiving Header.", (const void*) dispatcher, dataStreamHandle);
    size_t maxBytes = sizeof dataStreamDescriptor->_.header.bytes;
    HAPAssert(dataStreamDescriptor->_.header.numBytes <= maxBytes);
    if (!isComplete) {
        HAPDataStreamReceiveData(
                server,
                dataStream,
                &dataStreamDescriptor->_.header.bytes,
                dataStreamDescriptor->_.header.numBytes,
                HandleReceiveHeaderComplete);
    } else {
        HandleReceiveHeaderComplete(
                server,
                request,
                dataStream,
                error,
                dataStreamDescriptor->_.header.bytes,
                dataStreamDescriptor->_.header.numBytes,
                isComplete,
                context);
    }
}

static void BeginReceivingPacket(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStreamHandle) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStreamHandle];
    HAPDataStreamRef* dataStream = HAPNonnull(dataStreamDescriptor->dataStream);
    HAPPrecondition(dataStreamDescriptor->receiveState == kHAPDataStreamDescriptorReceiveState_Idle);

    if (!dataStreamDescriptor->hasPendingPacket) {
        return;
    }

    // Receive HeaderLen.
    dataStreamDescriptor->hasPendingPacket = false;
    dataStreamDescriptor->receiveState = kHAPDataStreamDescriptorReceiveState_ReceivingHeader;
    if (dataStreamDescriptor->numDataBytes < sizeof dataStreamDescriptor->_.header.numBytes) {
        HAPLog(&logObject,
               "[%p.%u] Received packet is too short for HeaderLen (remaining packet length: %zu bytes).",
               (const void*) dispatcher,
               dataStreamHandle,
               (size_t) dataStreamDescriptor->numDataBytes);
        HAPDataStreamInvalidate(server, dataStream);
        return;
    }
    HAPLogDebug(&logObject, "[%p.%u] Receiving HeaderLen.", (const void*) dispatcher, dataStreamHandle);
    HAPDataStreamReceiveData(
            server,
            dataStream,
            &dataStreamDescriptor->_.header.numBytes,
            sizeof dataStreamDescriptor->_.header.numBytes,
            HandleReceiveHeaderLenComplete);
}

static void HandleData(
        HAPAccessoryServer* server,
        const HAPDataStreamRequest* request,
        HAPDataStreamRef* dataStream,
        size_t totalDataBytes,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->accessory);
    HAPPrecondition(request->accessory->dataStream.delegate.callbacks == &kHAPDataStreamDispatcher_DataStreamCallbacks);
    HAPDataStreamDispatcher* dispatcher = request->accessory->dataStream.delegate.context;
    HAPPrecondition(dataStream);
    HAPDataStreamHandle dataStreamHandle = HAPDataStreamGetHandleForDataStream(server, dispatcher, dataStream);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStreamHandle];
    HAPPrecondition(totalDataBytes <= kHAPDataStream_MaxPayloadBytes);

    // Register incoming data.
    HAPAssert(!dataStreamDescriptor->hasPendingPacket);
    HAPAssert(!dataStreamDescriptor->numDataBytes);
    dataStreamDescriptor->hasPendingPacket = true;
    dataStreamDescriptor->numDataBytes = (uint32_t) totalDataBytes;

    // Accept data.
    if (dataStreamDescriptor->receiveState == kHAPDataStreamDescriptorReceiveState_Idle) {
        BeginReceivingPacket(server, dispatcher, dataStreamHandle);
    } else {
        HAPLogInfo(
                &logObject,
                "[%p.%u] Delaying to receive packet until previous packet is handled.",
                (const void*) dispatcher,
                dataStreamHandle);
    }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * @defgroup HeaderParts Parts to incrementally send a header.
 *
 * - OPACK encoding for a complete Header:
 *
 *   Dictionary with N entries: 0xE2                                 (event)
 *                              0xE3                                 (request)
 *                              0xE4                                 (response)
 *
 *                    "status": 0x46 's' 't' 'a' 't' 'u' 's'         (response)
 *        Response status code: 1 byte (up to 39)                    (response)
 *
 *                        "id": 0x42 'i' 'd'                         (request / response)
 *                  Request ID: 0x33 8 bytes little-endian           (request / response)
 *
 *                     "event": 0x45 'e' 'v' 'e' 'n' 't'             (event)
 *                   "request": 0x47 'r' 'e' 'q' 'u' 'e' 's' 't'     (request)
 *                  "response": 0x47 'r' 'e' 's' 'p' 'o' 'n' 's' 'e' (response)
 *
 *                       Topic: 1+ bytes + topic
 *                  "protocol": 0x48 'p' 'r' 'o' 't' 'o' 'c' 'o' 'l'
 *               Protocol name: 1+ bytes + protocol name
 *
 * - OPACK string encoding:
 *       Up to 32 bytes length: 1 byte + string
 *        Longer than 32 bytes: 0x6F + string + '\0'
 * @{
 */
static const uint8_t kHAPDataStreamDispatcher_EventHeaderDictionaryTag = kHAPOPACKTag_Dictionary2;
static const uint8_t kHAPDataStreamDispatcher_RequestHeaderDictionaryTag = kHAPOPACKTag_Dictionary3;
static const uint8_t kHAPDataStreamDispatcher_ResponseHeaderDictionaryTag = kHAPOPACKTag_Dictionary4;

static const uint8_t kHAPDataStreamDispatcher_HeaderStatusKey[] = {
    kHAPOPACKTag_String6, 's', 't', 'a', 't', 'u', 's'
};

static const uint8_t kHAPDataStreamDispatcher_HeaderIDKeyAndTag[] = { kHAPOPACKTag_String2,
                                                                      'i',
                                                                      'd',
                                                                      kHAPOPACKTag_Int64 };

static const uint8_t kHAPDataStreamDispatcher_EventHeaderTopicKey[] = { kHAPOPACKTag_String5, 'e', 'v', 'e', 'n', 't' };
static const uint8_t kHAPDataStreamDispatcher_RequestHeaderTopicKey[] = {
    kHAPOPACKTag_String7, 'r', 'e', 'q', 'u', 'e', 's', 't'
};
static const uint8_t kHAPDataStreamDispatcher_ResponseHeaderTopicKey[] = {
    kHAPOPACKTag_String8, 'r', 'e', 's', 'p', 'o', 'n', 's', 'e'
};

static const uint8_t kHAPDataStreamDispatcher_HeaderProtocolKey[] = {
    kHAPOPACKTag_String8, 'p', 'r', 'o', 't', 'o', 'c', 'o', 'l'
};
/**@}*/

/**
 * Computes the HeaderLen for an event Header.
 *
 * @param      protocolName         HomeKit Data Stream protocol name.
 * @param      topic                Topic.
 *
 * @return HeaderLen value.
 */
HAP_RESULT_USE_CHECK
static uint8_t GetEventHeaderLen(const char* protocolName, const char* topic) {
    HAPPrecondition(protocolName);
    HAPPrecondition(topic);

    size_t numNameBytes = HAPStringGetNumBytes(protocolName);
    size_t numTopicBytes = HAPStringGetNumBytes(topic);

    // See computation in HAPDataStreamDescriptor._.request.nameAndTopic.
    size_t headerLen = 0;
    headerLen += sizeof kHAPDataStreamDispatcher_EventHeaderDictionaryTag;
    headerLen += sizeof kHAPDataStreamDispatcher_EventHeaderTopicKey;
    headerLen += 1;
    headerLen += numTopicBytes + (numTopicBytes > 32 ? 1 : 0);
    headerLen += sizeof kHAPDataStreamDispatcher_HeaderProtocolKey;
    headerLen += 1;
    headerLen += numNameBytes + (numNameBytes > 32 ? 1 : 0);

    HAPAssert(headerLen <= UINT8_MAX);
    return (uint8_t) headerLen;
}

/**
 * Computes the HeaderLen for a request Header.
 *
 * @param      protocolName         HomeKit Data Stream protocol name.
 * @param      topic                Topic.
 *
 * @return HeaderLen value.
 */
HAP_RESULT_USE_CHECK
static uint8_t GetRequestHeaderLen(const char* protocolName, const char* topic) {
    HAPPrecondition(protocolName);
    HAPPrecondition(topic);
    HAPPrecondition(HAPDataStreamRequestNameAndTopicAreValid(protocolName, topic));

    size_t numNameBytes = HAPStringGetNumBytes(protocolName);
    size_t numTopicBytes = HAPStringGetNumBytes(topic);

    // See computation in HAPDataStreamDescriptor._.request.nameAndTopic.
    size_t headerLen = 0;
    headerLen += sizeof kHAPDataStreamDispatcher_RequestHeaderDictionaryTag;
    headerLen += sizeof kHAPDataStreamDispatcher_HeaderIDKeyAndTag;
    headerLen += sizeof(uint64_t) / 2;
    headerLen += sizeof(uint64_t) / 2;
    headerLen += sizeof kHAPDataStreamDispatcher_RequestHeaderTopicKey;
    headerLen += 1;
    headerLen += numTopicBytes + (numTopicBytes > 32 ? 1 : 0);
    headerLen += sizeof kHAPDataStreamDispatcher_HeaderProtocolKey;
    headerLen += 1;
    headerLen += numNameBytes + (numNameBytes > 32 ? 1 : 0);

    HAPAssert(headerLen <= UINT8_MAX);
    return (uint8_t) headerLen;
}

/**
 * Computes the HeaderLen for a response Header.
 *
 * @param      protocolName         HomeKit Data Stream protocol name.
 * @param      topic                Topic.
 *
 * @return HeaderLen value.
 */
HAP_RESULT_USE_CHECK
static uint8_t GetResponseHeaderLen(const char* protocolName, const char* topic) {
    HAPPrecondition(protocolName);
    HAPPrecondition(topic);
    HAPPrecondition(HAPDataStreamRequestNameAndTopicAreValid(protocolName, topic));

    size_t numNameBytes = HAPStringGetNumBytes(protocolName);
    size_t numTopicBytes = HAPStringGetNumBytes(topic);

    // See computation in HAPDataStreamDescriptor._.request.nameAndTopic.
    size_t headerLen = 0;
    headerLen += sizeof kHAPDataStreamDispatcher_ResponseHeaderDictionaryTag;
    headerLen += sizeof kHAPDataStreamDispatcher_HeaderStatusKey;
    headerLen += 1;
    headerLen += sizeof kHAPDataStreamDispatcher_HeaderIDKeyAndTag;
    headerLen += sizeof(uint64_t) / 2;
    headerLen += sizeof(uint64_t) / 2;
    headerLen += sizeof kHAPDataStreamDispatcher_ResponseHeaderTopicKey;
    headerLen += 1;
    headerLen += numTopicBytes + (numTopicBytes > 32 ? 1 : 0);
    headerLen += sizeof kHAPDataStreamDispatcher_HeaderProtocolKey;
    headerLen += 1;
    headerLen += numNameBytes + (numNameBytes > 32 ? 1 : 0);

    HAPAssert(headerLen <= UINT8_MAX);
    return (uint8_t) headerLen;
}

/**
 * Payload of an empty message.
 */
static const uint8_t kHAPDataStreamDispatcher_EmptyMessage[] = { kHAPOPACKTag_Dictionary0 };

/**
 * Payload of a capability message.
 */
static const uint8_t kHAPDataStreamDispatcher_HDSCapabilityMessage[] = { kHAPOPACKTag_Dictionary1,
                                                                         kHAPOPACKTag_String18,
                                                                         'c',
                                                                         'a',
                                                                         'p',
                                                                         'a',
                                                                         'b',
                                                                         'i',
                                                                         'l',
                                                                         'i',
                                                                         't',
                                                                         'y',
                                                                         '-',
                                                                         'v',
                                                                         'e',
                                                                         'r',
                                                                         's',
                                                                         'i',
                                                                         'o',
                                                                         'n',
                                                                         kHAPOPACKTag_True };

//----------------------------------------------------------------------------------------------------------------------

static void HandleSendMessageComplete(
        HAPAccessoryServer* server,
        const HAPDataStreamRequest* request,
        HAPDataStreamRef* dataStream,
        HAPError error,
        void* _Nullable dataBytes,
        size_t numDataBytes,
        bool isComplete,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->accessory);
    HAPPrecondition(request->accessory->dataStream.delegate.callbacks == &kHAPDataStreamDispatcher_DataStreamCallbacks);
    HAPDataStreamDispatcher* dispatcher = request->accessory->dataStream.delegate.context;
    HAPPrecondition(dataStream);
    HAPDataStreamHandle dataStreamHandle = HAPDataStreamGetHandleForDataStream(server, dispatcher, dataStream);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStreamHandle];
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        HAPDataStreamInvalidate(server, dataStream);
        return;
    }

    switch ((HAPDataStreamDescriptorSendState) dataStreamDescriptor->sendState) {
        case kHAPDataStreamDescriptorSendState_Idle: {
        }
            HAPFatalError();
        case kHAPDataStreamDescriptorSendState_SendingEvent: {
            HAPDataStreamSendEventTransaction* transaction_ =
                    GetSendingTransaction(server, dispatcher, dataStreamHandle);
            HAPDataStreamSendEventTransaction* transaction = (HAPDataStreamSendEventTransaction*) transaction_;
            HAPAssert(transaction->_.type == kHAPDataStreamTransactionType_SendEvent);

            HAPBaseDataStreamProtocol* dataStreamProtocol = transaction->dataStreamProtocol;

            // Complete sending Message.
            HAPAssert(dataStreamDescriptor->sendState == kHAPDataStreamDescriptorSendState_SendingEvent);
            HAPAssert(transaction->state == kHAPDataStreamSendEventTransactionState_SendingEvent);
            HAPAssert(dataBytes == transaction->message.mutableBytes);
            HAPAssert(numDataBytes == transaction->numMessageBytes);

            // Inform completion handler.
            HAPAssert(isComplete);
            HAPDataStreamSendEventCompletionHandler _Nullable completionHandler = transaction->messageCompletionHandler;
            HAPAssert(completionHandler);
            dataStreamDescriptor->sendState = kHAPDataStreamDescriptorSendState_Idle;
            DeregisterTransaction(server, dispatcher, dataStreamHandle, transaction_);
            HAPRawBufferZero(transaction_, sizeof *transaction_);
            completionHandler(
                    server,
                    dispatcher,
                    dataStreamProtocol,
                    request,
                    dataStreamHandle,
                    transaction_,
                    kHAPError_None,
                    dataBytes,
                    numDataBytes,
                    context);

            // Continue sending.
            if (dataStreamDescriptor->sendState == kHAPDataStreamDescriptorSendState_Idle) {
                BeginSendingPacket(server, dispatcher, dataStreamHandle);
            }
            return;
        }
        case kHAPDataStreamDescriptorSendState_SendingRequest: {
            HAPDataStreamSendRequestTransaction* transaction_ =
                    GetSendingTransaction(server, dispatcher, dataStreamHandle);
            HAPDataStreamSendRequestTransaction* transaction = (HAPDataStreamSendRequestTransaction*) transaction_;
            HAPAssert(transaction->_.type == kHAPDataStreamTransactionType_SendRequest);

            HAPBaseDataStreamProtocol* dataStreamProtocol = transaction->dataStreamProtocol;

            // Complete sending Message.
            HAPAssert(dataStreamDescriptor->sendState == kHAPDataStreamDescriptorSendState_SendingRequest);
            HAPAssert(transaction->state == kHAPDataStreamSendRequestTransactionState_SendingRequest);
            HAPAssert(dataBytes == transaction->message.mutableBytes);
            HAPAssert(numDataBytes == transaction->numMessageBytes);

            // Inform completion handler.
            HAPAssert(isComplete);
            HAPDataStreamSendRequestMessageCompletionHandler _Nullable completionHandler =
                    transaction->messageCompletionHandler;
            HAPAssert(completionHandler);
            dataStreamDescriptor->sendState = kHAPDataStreamDescriptorSendState_Idle;
            transaction->state = kHAPDataStreamSendRequestTransactionState_WaitingForResponse;
            transaction->messageCompletionHandler = NULL;
            transaction->_.messageIsMutable = false;
            transaction->message.mutableBytes = NULL;
            transaction->numMessageBytes = 0;
            completionHandler(
                    server,
                    dispatcher,
                    dataStreamProtocol,
                    request,
                    dataStreamHandle,
                    transaction_,
                    kHAPError_None,
                    dataBytes,
                    numDataBytes,
                    false,
                    context);

            // Continue sending.
            if (dataStreamDescriptor->sendState == kHAPDataStreamDescriptorSendState_Idle) {
                BeginSendingPacket(server, dispatcher, dataStreamHandle);
            }
            return;
        }
        case kHAPDataStreamDescriptorSendState_SendingResponse: {
            HAPDataStreamReceiveRequestTransaction* transaction_ =
                    GetSendingTransaction(server, dispatcher, dataStreamHandle);
            HAPDataStreamReceiveRequestTransaction* transaction =
                    (HAPDataStreamReceiveRequestTransaction*) transaction_;
            HAPAssert(transaction->_.type == kHAPDataStreamTransactionType_ReceiveRequest);

            HAPBaseDataStreamProtocol* dataStreamProtocol = transaction->dataStreamProtocol;

            // Complete sending Message.
            HAPAssert(dataStreamDescriptor->sendState == kHAPDataStreamDescriptorSendState_SendingResponse);
            HAPAssert(transaction->state == kHAPDataStreamReceiveRequestTransactionState_SendingResponse);
            HAPAssert(dataBytes == transaction->message.mutableBytes);
            HAPAssert(numDataBytes == transaction->numMessageBytes);

            // Inform completion handler.
            HAPAssert(isComplete);
            HAPDataStreamReceiveRequestMessageCompletionHandler _Nullable completionHandler =
                    transaction->messageCompletionHandler;
            HAPAssert(completionHandler);
            dataStreamDescriptor->sendState = kHAPDataStreamDescriptorSendState_Idle;
            DeregisterTransaction(server, dispatcher, dataStreamHandle, transaction_);
            HAPRawBufferZero(transaction_, sizeof *transaction_);
            completionHandler(
                    server,
                    dispatcher,
                    dataStreamProtocol,
                    request,
                    dataStreamHandle,
                    transaction_,
                    kHAPError_None,
                    dataBytes,
                    numDataBytes,
                    true,
                    context);

            // Continue sending.
            if (dataStreamDescriptor->sendState == kHAPDataStreamDescriptorSendState_Idle) {
                BeginSendingPacket(server, dispatcher, dataStreamHandle);
            }
            return;
        }
        case kHAPDataStreamDescriptorSendState_SendingStatusOnlyResponse: {
            const char* protocolName = &dataStreamDescriptor->_.request.nameAndTopic[0];
            size_t numNameBytes = HAPStringGetNumBytes(protocolName);
            HAPAssert(sizeof dataStreamDescriptor->_.request.nameAndTopic >= numNameBytes + 1);
            HAPDataStreamProtocol* _Nullable dataStreamProtocol =
                    HAPDataStreamDispatcherFindHandlerForProtocolName(server, dispatcher, protocolName);
            const char* topic = &dataStreamDescriptor->_.request.nameAndTopic[numNameBytes + 1];

            // Complete sending Message.
            HAPAssert(dataStreamDescriptor->sendState == kHAPDataStreamDescriptorSendState_SendingStatusOnlyResponse);
            HAPAssert(dataStreamDescriptor->receiveState == kHAPDataStreamDescriptorReceiveState_SendingResponse);
            if ((HAPStringAreEqual(protocolName, kHAPControlDataStreamProtocol_Name) == true) &&
                (HAPStringAreEqual(topic, kHAPControlDataStreamProtocol_HelloTopic) == true)) {
                HAPAssert(dataBytes == kHAPDataStreamDispatcher_HDSCapabilityMessage);
                HAPAssert(numDataBytes == sizeof kHAPDataStreamDispatcher_HDSCapabilityMessage);
            } else {
                HAPAssert(dataBytes == kHAPDataStreamDispatcher_EmptyMessage);
                HAPAssert(numDataBytes == sizeof kHAPDataStreamDispatcher_EmptyMessage);
            }

            // Inform completion handler.
            HAPAssert(isComplete);
            HAPDataStreamSkipAndReplyToRequestCompletionHandler _Nullable completionHandler =
                    dataStreamDescriptor->_.request.skipAndReplyToRequestCompletionHandler;
            HAPAssert(completionHandler);
            dataStreamDescriptor->sendState = kHAPDataStreamDescriptorSendState_Idle;
            dataStreamDescriptor->receiveState = kHAPDataStreamDescriptorReceiveState_Idle;
            HAPRawBufferZero(&dataStreamDescriptor->_, sizeof dataStreamDescriptor->_);
            completionHandler(server, dispatcher, dataStreamProtocol, request, dataStreamHandle, error, context);

            // Continue receiving.
            if (dataStreamDescriptor->receiveState == kHAPDataStreamDescriptorReceiveState_Idle) {
                BeginReceivingPacket(server, dispatcher, dataStreamHandle);
            }

            // Continue sending.
            if (dataStreamDescriptor->sendState == kHAPDataStreamDescriptorSendState_Idle) {
                BeginSendingPacket(server, dispatcher, dataStreamHandle);
            }
            return;
        }
        default:
            HAPFatalError();
    }
}

static void HandleSendHeaderProtocolNameComplete(
        HAPAccessoryServer* server,
        const HAPDataStreamRequest* request,
        HAPDataStreamRef* dataStream,
        HAPError error,
        void* _Nullable dataBytes,
        size_t numDataBytes,
        bool isComplete,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->accessory);
    HAPPrecondition(request->accessory->dataStream.delegate.callbacks == &kHAPDataStreamDispatcher_DataStreamCallbacks);
    HAPDataStreamDispatcher* dispatcher = request->accessory->dataStream.delegate.context;
    HAPPrecondition(dataStream);
    HAPDataStreamHandle dataStreamHandle = HAPDataStreamGetHandleForDataStream(server, dispatcher, dataStream);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStreamHandle];
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        HAPDataStreamInvalidate(server, dataStream);
        return;
    }

    if (dataStreamDescriptor->sendState == kHAPDataStreamDescriptorSendState_SendingStatusOnlyResponse) {
        const char* protocolName = &dataStreamDescriptor->_.request.nameAndTopic[0];
        size_t numNameBytes = HAPStringGetNumBytes(protocolName);
        HAPAssert(sizeof dataStreamDescriptor->_.request.nameAndTopic >= numNameBytes + 1);

        // Complete sending Header (protocol name).
        // See computation in HAPDataStreamDescriptor._.request.nameAndTopic.
        HAPAssert(dataStreamDescriptor->sendState == kHAPDataStreamDescriptorSendState_SendingStatusOnlyResponse);
        HAPAssert(dataStreamDescriptor->receiveState == kHAPDataStreamDescriptorReceiveState_SendingResponse);
        HAPAssert(dataBytes == protocolName);
        HAPAssert(numDataBytes == numNameBytes + (numNameBytes > 32 ? 1 : 0));

        // Send Message.
        HAPAssert(!isComplete);
        const char* topic = &dataStreamDescriptor->_.request.nameAndTopic[numNameBytes + 1];
        if ((HAPStringAreEqual(protocolName, kHAPControlDataStreamProtocol_Name) == true) &&
            (HAPStringAreEqual(topic, kHAPControlDataStreamProtocol_HelloTopic) == true)) {
            HAPDataStreamSendData(
                    server,
                    dataStream,
                    kHAPDataStreamDispatcher_HDSCapabilityMessage,
                    sizeof kHAPDataStreamDispatcher_HDSCapabilityMessage,
                    HandleSendMessageComplete);
        } else {
            HAPDataStreamSendData(
                    server,
                    dataStream,
                    kHAPDataStreamDispatcher_EmptyMessage,
                    sizeof kHAPDataStreamDispatcher_EmptyMessage,
                    HandleSendMessageComplete);
        }
        return;
    }

    HAPDataStreamTransactionRef* transaction_ = GetSendingTransaction(server, dispatcher, dataStreamHandle);
    HAPDataStreamBaseTransaction* transaction = transaction_;

    HAPBaseDataStreamProtocol* dataStreamProtocol = transaction->dataStreamProtocol;
    const char* protocolName = dataStreamProtocol->base->name;
    size_t numNameBytes = HAPStringGetNumBytes(protocolName);

    // Complete sending Header (protocol name).
    // See computation in HAPDataStreamDescriptor._.request.nameAndTopic.
    HAPAssert(dataBytes == protocolName);
    HAPAssert(numDataBytes == numNameBytes + (numNameBytes > 32 ? 1 : 0));

    // Send Message.
    HAPAssert(isComplete == (transaction->numMessageBytes == 0));
    if (!isComplete) {
        if (transaction->_.messageIsMutable) {
            HAPAssert(transaction->message.mutableBytes);
            HAPDataStreamSendMutableData(
                    server,
                    dataStream,
                    HAPNonnullVoid(transaction->message.mutableBytes),
                    transaction->numMessageBytes,
                    HandleSendMessageComplete);
        } else {
            HAPAssert(transaction->message.bytes);
            HAPDataStreamSendData(
                    server,
                    dataStream,
                    HAPNonnullVoid(transaction->message.bytes),
                    transaction->numMessageBytes,
                    HandleSendMessageComplete);
        }
    } else {
        HandleSendMessageComplete(
                server,
                request,
                dataStream,
                error,
                transaction->message.mutableBytes,
                transaction->numMessageBytes,
                isComplete,
                context);
    }
}

static void HandleSendHeaderProtocolNameTagComplete(
        HAPAccessoryServer* server,
        const HAPDataStreamRequest* request,
        HAPDataStreamRef* dataStream,
        HAPError error,
        void* _Nullable dataBytes,
        size_t numDataBytes,
        bool isComplete,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->accessory);
    HAPPrecondition(request->accessory->dataStream.delegate.callbacks == &kHAPDataStreamDispatcher_DataStreamCallbacks);
    HAPDataStreamDispatcher* dispatcher = request->accessory->dataStream.delegate.context;
    HAPPrecondition(dataStream);
    HAPDataStreamHandle dataStreamHandle = HAPDataStreamGetHandleForDataStream(server, dispatcher, dataStream);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStreamHandle];
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        HAPDataStreamInvalidate(server, dataStream);
        return;
    }

    HAPDataStreamTransactionRef* _Nullable transaction_ = NULL;
    const char* protocolName;
    size_t numNameBytes;
    if (dataStreamDescriptor->sendState == kHAPDataStreamDescriptorSendState_SendingStatusOnlyResponse) {
        protocolName = &dataStreamDescriptor->_.request.nameAndTopic[0];
        numNameBytes = HAPStringGetNumBytes(protocolName);
        HAPAssert(sizeof dataStreamDescriptor->_.request.nameAndTopic >= numNameBytes + 1);
    } else {
        transaction_ = GetSendingTransaction(server, dispatcher, dataStreamHandle);
        HAPDataStreamBaseTransaction* transaction = transaction_;

        HAPBaseDataStreamProtocol* dataStreamProtocol = transaction->dataStreamProtocol;
        protocolName = dataStreamProtocol->base->name;
        numNameBytes = HAPStringGetNumBytes(protocolName);
    }

    // Complete sending Header (protocol name tag).
    HAPAssert(dataBytes == dataStreamDescriptor->outBytes);
    HAPAssert(numDataBytes == 1);

    // Send Header (protocol name).
    // See computation in HAPDataStreamDescriptor._.request.nameAndTopic.
    if (!isComplete) {
        HAPAssert(!isComplete);
        HAPDataStreamSendData(
                server,
                dataStream,
                protocolName,
                numNameBytes + (numNameBytes > 32 ? 1 : 0),
                HandleSendHeaderProtocolNameComplete);
    } else {
        HAPAssert(isComplete);
        HandleSendHeaderProtocolNameComplete(
                server,
                request,
                dataStream,
                error,
                (char*) (uintptr_t) protocolName,
                numNameBytes + (numNameBytes > 32 ? 1 : 0),
                isComplete,
                context);
    }
}

static void HandleSendHeaderProtocolKeyComplete(
        HAPAccessoryServer* server,
        const HAPDataStreamRequest* request,
        HAPDataStreamRef* dataStream,
        HAPError error,
        void* _Nullable dataBytes,
        size_t numDataBytes,
        bool isComplete,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->accessory);
    HAPPrecondition(request->accessory->dataStream.delegate.callbacks == &kHAPDataStreamDispatcher_DataStreamCallbacks);
    HAPDataStreamDispatcher* dispatcher = request->accessory->dataStream.delegate.context;
    HAPPrecondition(dataStream);
    HAPDataStreamHandle dataStreamHandle = HAPDataStreamGetHandleForDataStream(server, dispatcher, dataStream);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStreamHandle];
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        HAPDataStreamInvalidate(server, dataStream);
        return;
    }

    HAPDataStreamTransactionRef* _Nullable transaction_ = NULL;
    const char* protocolName;
    size_t numNameBytes;
    if (dataStreamDescriptor->sendState == kHAPDataStreamDescriptorSendState_SendingStatusOnlyResponse) {
        protocolName = &dataStreamDescriptor->_.request.nameAndTopic[0];
        numNameBytes = HAPStringGetNumBytes(protocolName);
        HAPAssert(sizeof dataStreamDescriptor->_.request.nameAndTopic >= numNameBytes + 1);
    } else {
        transaction_ = GetSendingTransaction(server, dispatcher, dataStreamHandle);
        HAPDataStreamBaseTransaction* transaction = transaction_;

        HAPBaseDataStreamProtocol* dataStreamProtocol = transaction->dataStreamProtocol;
        protocolName = dataStreamProtocol->base->name;
        numNameBytes = HAPStringGetNumBytes(protocolName);
    }

    // Complete sending Header (protocol key).
    HAPAssert(dataBytes == kHAPDataStreamDispatcher_HeaderProtocolKey);
    HAPAssert(numDataBytes == sizeof kHAPDataStreamDispatcher_HeaderProtocolKey);

    // Send Header (protocol name tag).
    // See computation in HAPDataStreamDescriptor._.request.nameAndTopic.
    HAPAssert(!isComplete);
    HAPAssert(sizeof dataStreamDescriptor->outBytes >= 1);
    if (numNameBytes <= 32) {
        dataStreamDescriptor->outBytes[0] = (uint8_t)(kHAPOPACKTag_String0 + numNameBytes);
    } else {
        dataStreamDescriptor->outBytes[0] = kHAPOPACKTag_String;
    }
    HAPDataStreamSendMutableData(
            server, dataStream, dataStreamDescriptor->outBytes, 1, HandleSendHeaderProtocolNameTagComplete);
}

static void HandleSendHeaderTopicComplete(
        HAPAccessoryServer* server,
        const HAPDataStreamRequest* request,
        HAPDataStreamRef* dataStream,
        HAPError error,
        void* _Nullable dataBytes,
        size_t numDataBytes,
        bool isComplete,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->accessory);
    HAPPrecondition(request->accessory->dataStream.delegate.callbacks == &kHAPDataStreamDispatcher_DataStreamCallbacks);
    HAPDataStreamDispatcher* dispatcher = request->accessory->dataStream.delegate.context;
    HAPPrecondition(dataStream);
    HAPDataStreamHandle dataStreamHandle = HAPDataStreamGetHandleForDataStream(server, dispatcher, dataStream);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStreamHandle];
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        HAPDataStreamInvalidate(server, dataStream);
        return;
    }

    HAPDataStreamTransactionRef* _Nullable transaction_ = NULL;
    const char* topic;
    size_t numTopicBytes;
    if (dataStreamDescriptor->sendState == kHAPDataStreamDescriptorSendState_SendingStatusOnlyResponse) {
        const char* protocolName = &dataStreamDescriptor->_.request.nameAndTopic[0];
        size_t numNameBytes = HAPStringGetNumBytes(protocolName);
        topic = &dataStreamDescriptor->_.request.nameAndTopic[numNameBytes + 1];
        numTopicBytes = HAPStringGetNumBytes(topic);
        HAPAssert(sizeof dataStreamDescriptor->_.request.nameAndTopic >= numNameBytes + 1 + numTopicBytes + 1);
    } else {
        transaction_ = GetSendingTransaction(server, dispatcher, dataStreamHandle);
        HAPDataStreamBaseTransaction* transaction = transaction_;

        topic = transaction->topic;
        numTopicBytes = HAPStringGetNumBytes(topic);
    }

    // Complete sending Header (topic).
    // See computation in HAPDataStreamDescriptor._.request.nameAndTopic.
    HAPAssert(dataBytes == topic);
    HAPAssert(numDataBytes == numTopicBytes + (numTopicBytes > 32 ? 1 : 0));

    // Send Header (protocol key).
    HAPAssert(!isComplete);
    HAPDataStreamSendData(
            server,
            dataStream,
            kHAPDataStreamDispatcher_HeaderProtocolKey,
            sizeof kHAPDataStreamDispatcher_HeaderProtocolKey,
            HandleSendHeaderProtocolKeyComplete);
}

static void HandleSendHeaderTopicTagComplete(
        HAPAccessoryServer* server,
        const HAPDataStreamRequest* request,
        HAPDataStreamRef* dataStream,
        HAPError error,
        void* _Nullable dataBytes,
        size_t numDataBytes,
        bool isComplete,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->accessory);
    HAPPrecondition(request->accessory->dataStream.delegate.callbacks == &kHAPDataStreamDispatcher_DataStreamCallbacks);
    HAPDataStreamDispatcher* dispatcher = request->accessory->dataStream.delegate.context;
    HAPPrecondition(dataStream);
    HAPDataStreamHandle dataStreamHandle = HAPDataStreamGetHandleForDataStream(server, dispatcher, dataStream);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStreamHandle];
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        HAPDataStreamInvalidate(server, dataStream);
        return;
    }

    HAPDataStreamTransactionRef* _Nullable transaction_ = NULL;
    const char* topic;
    size_t numTopicBytes;
    if (dataStreamDescriptor->sendState == kHAPDataStreamDescriptorSendState_SendingStatusOnlyResponse) {
        const char* protocolName = &dataStreamDescriptor->_.request.nameAndTopic[0];
        size_t numNameBytes = HAPStringGetNumBytes(protocolName);
        topic = &dataStreamDescriptor->_.request.nameAndTopic[numNameBytes + 1];
        numTopicBytes = HAPStringGetNumBytes(topic);
        HAPAssert(sizeof dataStreamDescriptor->_.request.nameAndTopic >= numNameBytes + 1 + numTopicBytes + 1);
    } else {
        transaction_ = GetSendingTransaction(server, dispatcher, dataStreamHandle);
        HAPDataStreamBaseTransaction* transaction = transaction_;

        topic = transaction->topic;
        numTopicBytes = HAPStringGetNumBytes(topic);
    }

    // Complete sending Header (topic tag).
    HAPAssert(dataBytes == dataStreamDescriptor->outBytes);
    HAPAssert(numDataBytes == 1);

    // Send Header (topic).
    // See computation in HAPDataStreamDescriptor._.request.nameAndTopic.
    HAPAssert(!isComplete);
    HAPDataStreamSendData(
            server, dataStream, topic, numTopicBytes + (numTopicBytes > 32 ? 1 : 0), HandleSendHeaderTopicComplete);
}

static void HandleSendHeaderTopicKeyComplete(
        HAPAccessoryServer* server,
        const HAPDataStreamRequest* request,
        HAPDataStreamRef* dataStream,
        HAPError error,
        void* _Nullable dataBytes,
        size_t numDataBytes,
        bool isComplete,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->accessory);
    HAPPrecondition(request->accessory->dataStream.delegate.callbacks == &kHAPDataStreamDispatcher_DataStreamCallbacks);
    HAPDataStreamDispatcher* dispatcher = request->accessory->dataStream.delegate.context;
    HAPPrecondition(dataStream);
    HAPDataStreamHandle dataStreamHandle = HAPDataStreamGetHandleForDataStream(server, dispatcher, dataStream);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStreamHandle];
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        HAPDataStreamInvalidate(server, dataStream);
        return;
    }

    HAPDataStreamTransactionRef* _Nullable transaction_ = NULL;
    const char* topic;
    size_t numTopicBytes;
    if (dataStreamDescriptor->sendState == kHAPDataStreamDescriptorSendState_SendingStatusOnlyResponse) {
        const char* protocolName = &dataStreamDescriptor->_.request.nameAndTopic[0];
        size_t numNameBytes = HAPStringGetNumBytes(protocolName);
        topic = &dataStreamDescriptor->_.request.nameAndTopic[numNameBytes + 1];
        numTopicBytes = HAPStringGetNumBytes(topic);
        HAPAssert(sizeof dataStreamDescriptor->_.request.nameAndTopic >= numNameBytes + 1 + numTopicBytes + 1);
    } else {
        transaction_ = GetSendingTransaction(server, dispatcher, dataStreamHandle);
        HAPDataStreamBaseTransaction* transaction = transaction_;

        topic = transaction->topic;
        numTopicBytes = HAPStringGetNumBytes(topic);
    }

    // Complete sending Header (topic key).
    if (dataStreamDescriptor->sendState == kHAPDataStreamDescriptorSendState_SendingEvent) {
        HAPAssert(dataBytes == kHAPDataStreamDispatcher_EventHeaderTopicKey);
        HAPAssert(numDataBytes == sizeof kHAPDataStreamDispatcher_EventHeaderTopicKey);
    } else if (dataStreamDescriptor->sendState == kHAPDataStreamDescriptorSendState_SendingRequest) {
        HAPAssert(dataBytes == kHAPDataStreamDispatcher_RequestHeaderTopicKey);
        HAPAssert(numDataBytes == sizeof kHAPDataStreamDispatcher_RequestHeaderTopicKey);
    } else {
        HAPAssert(
                dataStreamDescriptor->sendState == kHAPDataStreamDescriptorSendState_SendingResponse ||
                dataStreamDescriptor->sendState == kHAPDataStreamDescriptorSendState_SendingStatusOnlyResponse);
        HAPAssert(dataBytes == kHAPDataStreamDispatcher_ResponseHeaderTopicKey);
        HAPAssert(numDataBytes == sizeof kHAPDataStreamDispatcher_ResponseHeaderTopicKey);
    }

    // Send Header (topic tag).
    // See computation in HAPDataStreamDescriptor._.request.nameAndTopic.
    HAPAssert(!isComplete);
    HAPAssert(sizeof dataStreamDescriptor->outBytes >= 1);
    if (numTopicBytes <= 32) {
        dataStreamDescriptor->outBytes[0] = (uint8_t)(kHAPOPACKTag_String0 + numTopicBytes);
    } else {
        dataStreamDescriptor->outBytes[0] = kHAPOPACKTag_String;
    }
    HAPDataStreamSendMutableData(
            server, dataStream, dataStreamDescriptor->outBytes, 1, HandleSendHeaderTopicTagComplete);
}

static void HandleSendHeaderRequestIDPart2Complete(
        HAPAccessoryServer* server,
        const HAPDataStreamRequest* request,
        HAPDataStreamRef* dataStream,
        HAPError error,
        void* _Nullable dataBytes,
        size_t numDataBytes,
        bool isComplete,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->accessory);
    HAPPrecondition(request->accessory->dataStream.delegate.callbacks == &kHAPDataStreamDispatcher_DataStreamCallbacks);
    HAPDataStreamDispatcher* dispatcher = request->accessory->dataStream.delegate.context;
    HAPPrecondition(dataStream);
    HAPDataStreamHandle dataStreamHandle = HAPDataStreamGetHandleForDataStream(server, dispatcher, dataStream);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStreamHandle];
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        HAPDataStreamInvalidate(server, dataStream);
        return;
    }

    // Complete sending Header (request ID part 2).
    HAPAssert(dataBytes == dataStreamDescriptor->outBytes);
    HAPAssert(numDataBytes == 4);

    // Send Header (topic key).
    HAPAssert(!isComplete);
    if (dataStreamDescriptor->sendState == kHAPDataStreamDescriptorSendState_SendingRequest) {
        HAPDataStreamSendData(
                server,
                dataStream,
                kHAPDataStreamDispatcher_RequestHeaderTopicKey,
                sizeof kHAPDataStreamDispatcher_RequestHeaderTopicKey,
                HandleSendHeaderTopicKeyComplete);
    } else {
        HAPAssert(
                dataStreamDescriptor->sendState == kHAPDataStreamDescriptorSendState_SendingResponse ||
                dataStreamDescriptor->sendState == kHAPDataStreamDescriptorSendState_SendingStatusOnlyResponse);
        HAPDataStreamSendData(
                server,
                dataStream,
                kHAPDataStreamDispatcher_ResponseHeaderTopicKey,
                sizeof kHAPDataStreamDispatcher_ResponseHeaderTopicKey,
                HandleSendHeaderTopicKeyComplete);
    }
}

static void HandleSendHeaderRequestIDPart1Complete(
        HAPAccessoryServer* server,
        const HAPDataStreamRequest* request,
        HAPDataStreamRef* dataStream,
        HAPError error,
        void* _Nullable dataBytes,
        size_t numDataBytes,
        bool isComplete,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->accessory);
    HAPPrecondition(request->accessory->dataStream.delegate.callbacks == &kHAPDataStreamDispatcher_DataStreamCallbacks);
    HAPDataStreamDispatcher* dispatcher = request->accessory->dataStream.delegate.context;
    HAPPrecondition(dataStream);
    HAPDataStreamHandle dataStreamHandle = HAPDataStreamGetHandleForDataStream(server, dispatcher, dataStream);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStreamHandle];
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        HAPDataStreamInvalidate(server, dataStream);
        return;
    }

    HAPDataStreamTransactionRef* _Nullable transaction_ = NULL;
    int64_t requestID;
    if (dataStreamDescriptor->sendState == kHAPDataStreamDescriptorSendState_SendingStatusOnlyResponse) {
        requestID = dataStreamDescriptor->_.request.requestID;
    } else {
        transaction_ = GetSendingTransaction(server, dispatcher, dataStreamHandle);
        HAPDataStreamBaseTransaction* transaction = transaction_;

        if (transaction->_.type == kHAPDataStreamTransactionType_SendRequest) {
            requestID = ((HAPDataStreamSendRequestTransaction*) transaction_)->requestID;
        } else {
            HAPAssert(transaction->_.type == kHAPDataStreamTransactionType_ReceiveRequest);
            requestID = ((HAPDataStreamReceiveRequestTransaction*) transaction_)->requestID;
        }
    }

    // Complete sending Header (request ID part 1).
    HAPAssert(dataBytes == dataStreamDescriptor->outBytes);
    HAPAssert(numDataBytes == 4);

    // Send Header (request ID part 2).
    HAPAssert(!isComplete);
    HAPAssert(sizeof dataStreamDescriptor->outBytes >= 4);
    dataStreamDescriptor->outBytes[0] = (uint8_t)(((uint64_t) requestID) >> 0x20U & 0xFFU);
    dataStreamDescriptor->outBytes[1] = (uint8_t)(((uint64_t) requestID) >> 0x28U & 0xFFU);
    dataStreamDescriptor->outBytes[2] = (uint8_t)(((uint64_t) requestID) >> 0x30U & 0xFFU);
    dataStreamDescriptor->outBytes[3] = (uint8_t)(((uint64_t) requestID) >> 0x38U & 0xFFU);
    HAPDataStreamSendMutableData(
            server, dataStream, dataStreamDescriptor->outBytes, 4, HandleSendHeaderRequestIDPart2Complete);
}

static void HandleSendHeaderIDKeyAndTagComplete(
        HAPAccessoryServer* server,
        const HAPDataStreamRequest* request,
        HAPDataStreamRef* dataStream,
        HAPError error,
        void* _Nullable dataBytes,
        size_t numDataBytes,
        bool isComplete,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->accessory);
    HAPPrecondition(request->accessory->dataStream.delegate.callbacks == &kHAPDataStreamDispatcher_DataStreamCallbacks);
    HAPDataStreamDispatcher* dispatcher = request->accessory->dataStream.delegate.context;
    HAPPrecondition(dataStream);
    HAPDataStreamHandle dataStreamHandle = HAPDataStreamGetHandleForDataStream(server, dispatcher, dataStream);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStreamHandle];
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        HAPDataStreamInvalidate(server, dataStream);
        return;
    }

    HAPDataStreamTransactionRef* _Nullable transaction_ = NULL;
    int64_t requestID;
    if (dataStreamDescriptor->sendState == kHAPDataStreamDescriptorSendState_SendingStatusOnlyResponse) {
        requestID = dataStreamDescriptor->_.request.requestID;
    } else {
        transaction_ = GetSendingTransaction(server, dispatcher, dataStreamHandle);
        HAPDataStreamBaseTransaction* transaction = transaction_;

        if (transaction->_.type == kHAPDataStreamTransactionType_SendRequest) {
            requestID = ((HAPDataStreamSendRequestTransaction*) transaction_)->requestID;
        } else {
            HAPAssert(transaction->_.type == kHAPDataStreamTransactionType_ReceiveRequest);
            requestID = ((HAPDataStreamReceiveRequestTransaction*) transaction_)->requestID;
        }
    }

    // Complete sending Header (id key and tag).
    HAPAssert(dataBytes == kHAPDataStreamDispatcher_HeaderIDKeyAndTag);
    HAPAssert(numDataBytes == sizeof kHAPDataStreamDispatcher_HeaderIDKeyAndTag);

    // Send Header (request ID part 1).
    HAPAssert(!isComplete);
    HAPAssert(sizeof dataStreamDescriptor->outBytes >= 4);
    dataStreamDescriptor->outBytes[0] = (uint8_t)(((uint64_t) requestID) >> 0x00U & 0xFFU);
    dataStreamDescriptor->outBytes[1] = (uint8_t)(((uint64_t) requestID) >> 0x08U & 0xFFU);
    dataStreamDescriptor->outBytes[2] = (uint8_t)(((uint64_t) requestID) >> 0x10U & 0xFFU);
    dataStreamDescriptor->outBytes[3] = (uint8_t)(((uint64_t) requestID) >> 0x18U & 0xFFU);
    HAPDataStreamSendMutableData(
            server, dataStream, dataStreamDescriptor->outBytes, 4, HandleSendHeaderRequestIDPart1Complete);
}

static void HandleSendHeaderStatusComplete(
        HAPAccessoryServer* server,
        const HAPDataStreamRequest* request,
        HAPDataStreamRef* dataStream,
        HAPError error,
        void* _Nullable dataBytes,
        size_t numDataBytes,
        bool isComplete,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->accessory);
    HAPPrecondition(request->accessory->dataStream.delegate.callbacks == &kHAPDataStreamDispatcher_DataStreamCallbacks);
    HAPDataStreamDispatcher* dispatcher = request->accessory->dataStream.delegate.context;
    HAPPrecondition(dataStream);
    HAPDataStreamHandle dataStreamHandle = HAPDataStreamGetHandleForDataStream(server, dispatcher, dataStream);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStreamHandle];
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        HAPDataStreamInvalidate(server, dataStream);
        return;
    }

    // Complete sending Header (status).
    HAPAssert(dataBytes == dataStreamDescriptor->outBytes);
    HAPAssert(numDataBytes == 1);

    // Send Header (id key and tag).
    HAPAssert(!isComplete);
    HAPDataStreamSendData(
            server,
            dataStream,
            kHAPDataStreamDispatcher_HeaderIDKeyAndTag,
            sizeof kHAPDataStreamDispatcher_HeaderIDKeyAndTag,
            HandleSendHeaderIDKeyAndTagComplete);
}

static void HandleSendHeaderStatusKeyComplete(
        HAPAccessoryServer* server,
        const HAPDataStreamRequest* request,
        HAPDataStreamRef* dataStream,
        HAPError error,
        void* _Nullable dataBytes,
        size_t numDataBytes,
        bool isComplete,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->accessory);
    HAPPrecondition(request->accessory->dataStream.delegate.callbacks == &kHAPDataStreamDispatcher_DataStreamCallbacks);
    HAPDataStreamDispatcher* dispatcher = request->accessory->dataStream.delegate.context;
    HAPPrecondition(dataStream);
    HAPDataStreamHandle dataStreamHandle = HAPDataStreamGetHandleForDataStream(server, dispatcher, dataStream);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStreamHandle];
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        HAPDataStreamInvalidate(server, dataStream);
        return;
    }

    HAPDataStreamTransactionRef* _Nullable transaction_ = NULL;
    HAPDataStreamResponseStatus status;
    if (dataStreamDescriptor->sendState == kHAPDataStreamDescriptorSendState_SendingStatusOnlyResponse) {
        status = dataStreamDescriptor->_.request.status;
    } else {
        transaction_ = GetSendingTransaction(server, dispatcher, dataStreamHandle);
        HAPDataStreamBaseTransaction* transaction = transaction_;

        HAPAssert(transaction->_.type == kHAPDataStreamTransactionType_ReceiveRequest);
        status = ((HAPDataStreamReceiveRequestTransaction*) transaction_)->status;
    }

    // Complete sending Header (status key).
    HAPAssert(dataBytes == kHAPDataStreamDispatcher_HeaderStatusKey);
    HAPAssert(numDataBytes == sizeof kHAPDataStreamDispatcher_HeaderStatusKey);

    // Send Header (status).
    HAPAssert(!isComplete);
    HAPAssert(sizeof dataStreamDescriptor->outBytes >= 1);
    HAPAssert((uint8_t) status <= 39); // See computation in HAPDataStreamDescriptor._.request.nameAndTopic.
    dataStreamDescriptor->outBytes[0] = (uint8_t)(kHAPOPACKTag_0 + status);
    HAPDataStreamSendMutableData(server, dataStream, dataStreamDescriptor->outBytes, 1, HandleSendHeaderStatusComplete);
}

static void HandleSendHeaderLenAndDictionaryTagComplete(
        HAPAccessoryServer* server,
        const HAPDataStreamRequest* request,
        HAPDataStreamRef* dataStream,
        HAPError error,
        void* _Nullable dataBytes,
        size_t numDataBytes,
        bool isComplete,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->accessory);
    HAPPrecondition(request->accessory->dataStream.delegate.callbacks == &kHAPDataStreamDispatcher_DataStreamCallbacks);
    HAPDataStreamDispatcher* dispatcher = request->accessory->dataStream.delegate.context;
    HAPPrecondition(dataStream);
    HAPDataStreamHandle dataStreamHandle = HAPDataStreamGetHandleForDataStream(server, dispatcher, dataStream);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStreamHandle];
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        HAPDataStreamInvalidate(server, dataStream);
        return;
    }

    // Complete sending HeaderLen and Header (dictionary tag).
    HAPAssert(dataBytes == dataStreamDescriptor->outBytes);
    HAPAssert(numDataBytes == 2);

    switch ((HAPDataStreamDescriptorSendState) dataStreamDescriptor->sendState) {
        case kHAPDataStreamDescriptorSendState_Idle: {
        }
            HAPFatalError();
        case kHAPDataStreamDescriptorSendState_SendingEvent: {
            // Send Header (topic key).
            HAPAssert(!isComplete);
            HAPDataStreamSendData(
                    server,
                    dataStream,
                    kHAPDataStreamDispatcher_EventHeaderTopicKey,
                    sizeof kHAPDataStreamDispatcher_EventHeaderTopicKey,
                    HandleSendHeaderTopicKeyComplete);
            return;
        }
        case kHAPDataStreamDescriptorSendState_SendingRequest: {
            // Send Header (id key).
            HAPAssert(!isComplete);
            HAPDataStreamSendData(
                    server,
                    dataStream,
                    kHAPDataStreamDispatcher_HeaderIDKeyAndTag,
                    sizeof kHAPDataStreamDispatcher_HeaderIDKeyAndTag,
                    HandleSendHeaderIDKeyAndTagComplete);
            return;
        }
        case kHAPDataStreamDescriptorSendState_SendingResponse:
        case kHAPDataStreamDescriptorSendState_SendingStatusOnlyResponse: {
            // Send Header (status key).
            HAPAssert(!isComplete);
            HAPDataStreamSendData(
                    server,
                    dataStream,
                    kHAPDataStreamDispatcher_HeaderStatusKey,
                    sizeof kHAPDataStreamDispatcher_HeaderStatusKey,
                    HandleSendHeaderStatusKeyComplete);
            return;
        }
        default:
            HAPFatalError();
    }
}

static void HandlePrepareComplete(
        HAPAccessoryServer* server,
        const HAPDataStreamRequest* request,
        HAPDataStreamRef* dataStream,
        HAPError error,
        void* _Nullable dataBytes,
        size_t numDataBytes,
        bool isComplete,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->accessory);
    HAPPrecondition(request->accessory->dataStream.delegate.callbacks == &kHAPDataStreamDispatcher_DataStreamCallbacks);
    HAPDataStreamDispatcher* dispatcher = request->accessory->dataStream.delegate.context;
    HAPPrecondition(dataStream);
    HAPDataStreamHandle dataStreamHandle = HAPDataStreamGetHandleForDataStream(server, dispatcher, dataStream);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStreamHandle];
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        HAPDataStreamInvalidate(server, dataStream);
        return;
    }

    HAPDataStreamTransactionRef* _Nullable transaction_ = NULL;
    const char* protocolName;
    const char* topic;
    if (dataStreamDescriptor->sendState == kHAPDataStreamDescriptorSendState_SendingStatusOnlyResponse) {
        protocolName = &dataStreamDescriptor->_.request.nameAndTopic[0];
        size_t numNameBytes = HAPStringGetNumBytes(protocolName);
        topic = &dataStreamDescriptor->_.request.nameAndTopic[numNameBytes + 1];
        size_t numTopicBytes = HAPStringGetNumBytes(topic);
        HAPAssert(sizeof dataStreamDescriptor->_.request.nameAndTopic >= numNameBytes + 1 + numTopicBytes + 1);
    } else {
        transaction_ = GetSendingTransaction(server, dispatcher, dataStreamHandle);
        HAPDataStreamBaseTransaction* transaction = transaction_;

        HAPBaseDataStreamProtocol* dataStreamProtocol = transaction->dataStreamProtocol;
        protocolName = dataStreamProtocol->base->name;
        topic = transaction->topic;
    }

    // Complete preparing.
    HAPAssert(!dataBytes);
    HAPAssert(!numDataBytes);

    switch ((HAPDataStreamDescriptorSendState) dataStreamDescriptor->sendState) {
        case kHAPDataStreamDescriptorSendState_Idle: {
        }
            HAPFatalError();
        case kHAPDataStreamDescriptorSendState_SendingEvent: {
            // Send HeaderLen and Header (dictionary tag).
            HAPAssert(!isComplete);
            HAPAssert(sizeof dataStreamDescriptor->outBytes >= 2);
            dataStreamDescriptor->outBytes[0] = GetEventHeaderLen(protocolName, topic);
            dataStreamDescriptor->outBytes[1] = kHAPDataStreamDispatcher_EventHeaderDictionaryTag;
            HAPDataStreamSendMutableData(
                    server, dataStream, dataStreamDescriptor->outBytes, 2, HandleSendHeaderLenAndDictionaryTagComplete);
            return;
        }
        case kHAPDataStreamDescriptorSendState_SendingRequest: {
            // Send HeaderLen and Header (dictionary tag).
            HAPAssert(!isComplete);
            HAPAssert(sizeof dataStreamDescriptor->outBytes >= 2);
            dataStreamDescriptor->outBytes[0] = GetRequestHeaderLen(protocolName, topic);
            dataStreamDescriptor->outBytes[1] = kHAPDataStreamDispatcher_RequestHeaderDictionaryTag;
            HAPDataStreamSendMutableData(
                    server, dataStream, dataStreamDescriptor->outBytes, 2, HandleSendHeaderLenAndDictionaryTagComplete);
            return;
        }
        case kHAPDataStreamDescriptorSendState_SendingResponse:
        case kHAPDataStreamDescriptorSendState_SendingStatusOnlyResponse: {
            // Send HeaderLen and Header (dictionary tag).
            HAPAssert(!isComplete);
            HAPAssert(sizeof dataStreamDescriptor->outBytes >= 2);
            dataStreamDescriptor->outBytes[0] = GetResponseHeaderLen(protocolName, topic);
            dataStreamDescriptor->outBytes[1] = kHAPDataStreamDispatcher_ResponseHeaderDictionaryTag;
            HAPDataStreamSendMutableData(
                    server, dataStream, dataStreamDescriptor->outBytes, 2, HandleSendHeaderLenAndDictionaryTagComplete);
            return;
        }
        default:
            HAPFatalError();
    }
}

static void BeginSendingPacket(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStreamHandle) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStreamHandle];
    HAPDataStreamRef* dataStream = HAPNonnull(dataStreamDescriptor->dataStream);
    HAPPrecondition(dataStreamDescriptor->sendState == kHAPDataStreamDescriptorSendState_Idle);

    // Prioritize response of skipped request because HomeKit Data Stream is stalled until complete.
    HAPAssert(dataStreamDescriptor->receiveState != kHAPDataStreamDescriptorReceiveState_SendingResponse);
    if (dataStreamDescriptor->receiveState == kHAPDataStreamDescriptorReceiveState_WaitingForResponse) {
        dataStreamDescriptor->receiveState = kHAPDataStreamDescriptorReceiveState_SendingResponse;

        const char* protocolName = &dataStreamDescriptor->_.request.nameAndTopic[0];
        size_t numNameBytes = HAPStringGetNumBytes(protocolName);
        const char* topic = &dataStreamDescriptor->_.request.nameAndTopic[numNameBytes + 1];
        size_t numTopicBytes = HAPStringGetNumBytes(topic);
        HAPAssert(sizeof dataStreamDescriptor->_.request.nameAndTopic >= numNameBytes + 1 + numTopicBytes + 1);
        size_t numMessageBytes = sizeof kHAPDataStreamDispatcher_EmptyMessage;
        if ((HAPStringAreEqual(protocolName, kHAPControlDataStreamProtocol_Name) == true) &&
            (HAPStringAreEqual(topic, kHAPControlDataStreamProtocol_HelloTopic) == true)) {
            numMessageBytes = sizeof kHAPDataStreamDispatcher_HDSCapabilityMessage;
        }

        // Prepare sending.
        dataStreamDescriptor->sendState = kHAPDataStreamDescriptorSendState_SendingStatusOnlyResponse;
        HAPDataStreamPrepareData(
                server,
                dataStream,
                1 + GetResponseHeaderLen(protocolName, topic) + numMessageBytes,
                HandlePrepareComplete);
        return;
    }

    // Send request for next waiting transaction.
    HAPDataStreamTransactionRef* _Nullable baseTransaction_ = dataStreamDescriptor->firstTransaction;
    while (baseTransaction_) {
        HAPDataStreamBaseTransaction* baseTransaction = baseTransaction_;
        HAPAssert(HAPDataStreamTransactionTypeIsValid(baseTransaction->_.type));

        switch ((HAPDataStreamTransactionType) baseTransaction->_.type) {
            case kHAPDataStreamTransactionType_SendEvent: {
                HAPDataStreamSendEventTransaction* transaction_ = baseTransaction_;
                HAPDataStreamSendEventTransaction* transaction = (HAPDataStreamSendEventTransaction*) transaction_;

                if (transaction->state == kHAPDataStreamSendEventTransactionState_WaitingForSend) {
                    HAPBaseDataStreamProtocol* dataStreamProtocol = transaction->dataStreamProtocol;
                    const char* protocolName = dataStreamProtocol->base->name;
                    const char* topic = transaction->topic;
                    size_t numMessageBytes = transaction->numMessageBytes;

                    // Prepare sending.
                    dataStreamDescriptor->sendState = kHAPDataStreamDescriptorSendState_SendingEvent;
                    transaction->state = kHAPDataStreamSendEventTransactionState_SendingEvent;
                    HAPDataStreamPrepareData(
                            server,
                            dataStream,
                            1 + GetEventHeaderLen(protocolName, topic) + numMessageBytes,
                            HandlePrepareComplete);
                    return;
                }
                break;
            }
            case kHAPDataStreamTransactionType_SendRequest: {
                HAPDataStreamSendRequestTransaction* transaction_ = baseTransaction_;
                HAPDataStreamSendRequestTransaction* transaction = (HAPDataStreamSendRequestTransaction*) transaction_;

                if (transaction->state == kHAPDataStreamSendRequestTransactionState_WaitingForSend) {
                    HAPBaseDataStreamProtocol* dataStreamProtocol = transaction->dataStreamProtocol;
                    const char* protocolName = dataStreamProtocol->base->name;
                    const char* topic = transaction->topic;
                    size_t numMessageBytes = transaction->numMessageBytes;

                    // Prepare sending.
                    dataStreamDescriptor->sendState = kHAPDataStreamDescriptorSendState_SendingRequest;
                    transaction->state = kHAPDataStreamSendRequestTransactionState_SendingRequest;
                    HAPDataStreamPrepareData(
                            server,
                            dataStream,
                            1 + GetRequestHeaderLen(protocolName, topic) + numMessageBytes,
                            HandlePrepareComplete);
                    return;
                }
                break;
            }
            case kHAPDataStreamTransactionType_ReceiveRequest: {
                HAPDataStreamReceiveRequestTransaction* transaction_ = baseTransaction_;
                HAPDataStreamReceiveRequestTransaction* transaction =
                        (HAPDataStreamReceiveRequestTransaction*) transaction_;

                if (transaction->state == kHAPDataStreamReceiveRequestTransactionState_WaitingForSend) {
                    HAPBaseDataStreamProtocol* dataStreamProtocol = transaction->dataStreamProtocol;
                    const char* protocolName = dataStreamProtocol->base->name;
                    const char* topic = transaction->topic;
                    size_t numMessageBytes = transaction->numMessageBytes;

                    // Prepare sending.
                    dataStreamDescriptor->sendState = kHAPDataStreamDescriptorSendState_SendingResponse;
                    transaction->state = kHAPDataStreamReceiveRequestTransactionState_SendingResponse;
                    HAPDataStreamPrepareData(
                            server,
                            dataStream,
                            1 + GetResponseHeaderLen(protocolName, topic) + numMessageBytes,
                            HandlePrepareComplete);
                    return;
                }
                break;
            }
        }

        baseTransaction_ = baseTransaction->nextTransaction;
    }
}

//----------------------------------------------------------------------------------------------------------------------

static void SendEvent(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* dataStreamProtocol_,
        HAPDataStreamHandle dataStream,
        HAPDataStreamSendEventTransaction* transaction_,
        const char* topic,
        bool messageIsMutable,
        void* _Nullable messageBytes,
        size_t numMessageBytes,
        HAPDataStreamSendEventCompletionHandler completionHandler) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream < dispatcher->storage->numDataStreams);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStream];
    HAPPrecondition(dataStreamProtocol_);
    HAPBaseDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    const char* protocolName = dataStreamProtocol->base->name;
    HAPPrecondition(transaction_);
    HAPPrecondition(!IsTransactionRegistered(server, dispatcher, dataStream, transaction_));
    HAPDataStreamSendEventTransaction* transaction = (HAPDataStreamSendEventTransaction*) transaction_;
    HAPPrecondition(topic);
    HAPPrecondition(!numMessageBytes || messageBytes);
    HAPPrecondition(numMessageBytes <= kHAPDataStream_MaxPayloadBytes - 1 - GetEventHeaderLen(protocolName, topic));
    HAPPrecondition(completionHandler);

    // Initialize transaction.
    HAPRawBufferZero(transaction_, sizeof *transaction_);
    transaction->_.type = kHAPDataStreamTransactionType_SendEvent;
    transaction->dataStreamProtocol = dataStreamProtocol;
    transaction->topic = topic;
    RegisterTransaction(server, dispatcher, dataStream, transaction_);

    // Prepare sending.
    transaction->state = kHAPDataStreamSendEventTransactionState_WaitingForSend;
    transaction->messageCompletionHandler = completionHandler;
    transaction->_.messageIsMutable = messageIsMutable;
    if (messageIsMutable) {
        transaction->message.mutableBytes = messageBytes;
    } else {
        transaction->message.bytes = messageBytes;
    }
    transaction->numMessageBytes = (uint32_t) numMessageBytes;

    // Continue sending.
    if (dataStreamDescriptor->sendState == kHAPDataStreamDescriptorSendState_Idle) {
        BeginSendingPacket(server, dispatcher, dataStream);
    }
}

void HAPDataStreamDispatcherSendEvent(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* dataStreamProtocol,
        HAPDataStreamHandle dataStream,
        HAPDataStreamSendEventTransaction* transaction,
        const char* topic,
        const void* _Nullable messageBytes,
        size_t numMessageBytes,
        HAPDataStreamSendEventCompletionHandler completionHandler) {
    SendEvent(
            server,
            dispatcher,
            dataStreamProtocol,
            dataStream,
            transaction,
            topic,
            false,
            (void*) (uintptr_t) messageBytes,
            numMessageBytes,
            completionHandler);
}

void HAPDataStreamDispatcherSendMutableEvent(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* dataStreamProtocol,
        HAPDataStreamHandle dataStream,
        HAPDataStreamSendEventTransaction* transaction,
        const char* topic,
        void* _Nullable messageBytes,
        size_t numMessageBytes,
        HAPDataStreamSendEventCompletionHandler completionHandler) {
    SendEvent(
            server,
            dispatcher,
            dataStreamProtocol,
            dataStream,
            transaction,
            topic,
            true,
            messageBytes,
            numMessageBytes,
            completionHandler);
}

//----------------------------------------------------------------------------------------------------------------------

static void SendRequest(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* dataStreamProtocol_,
        HAPDataStreamHandle dataStream,
        HAPDataStreamSendRequestTransaction* transaction_,
        const HAPDataStreamSendRequestTransactionCallbacks* callbacks,
        const char* topic,
        bool messageIsMutable,
        void* _Nullable messageBytes,
        size_t numMessageBytes,
        HAPDataStreamSendRequestMessageCompletionHandler completionHandler) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream < dispatcher->storage->numDataStreams);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStream];
    HAPPrecondition(dataStreamProtocol_);
    HAPBaseDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    const char* protocolName = dataStreamProtocol->base->name;
    HAPPrecondition(transaction_);
    HAPPrecondition(!IsTransactionRegistered(server, dispatcher, dataStream, transaction_));
    HAPDataStreamSendRequestTransaction* transaction = (HAPDataStreamSendRequestTransaction*) transaction_;
    HAPPrecondition(callbacks);
    HAPPrecondition(callbacks->handleInvalidate);
    HAPPrecondition(callbacks->handleResponseAvailable);
    HAPPrecondition(topic);
    HAPPrecondition(HAPDataStreamRequestNameAndTopicAreValid(protocolName, topic));
    HAPPrecondition(!numMessageBytes || messageBytes);
    HAPPrecondition(numMessageBytes <= kHAPDataStream_MaxPayloadBytes - 1 - GetRequestHeaderLen(protocolName, topic));
    HAPPrecondition(completionHandler);

    // Initialize send transaction.
    HAPRawBufferZero(transaction_, sizeof *transaction_);
    transaction->_.type = kHAPDataStreamTransactionType_SendRequest;
    transaction->dataStreamProtocol = dataStreamProtocol;
    transaction->topic = topic;
    transaction->callbacks = callbacks;
    HAPPlatformRandomNumberFill(&transaction->requestID, sizeof transaction->requestID);
    RegisterTransaction(server, dispatcher, dataStream, transaction_);

    // Prepare sending.
    transaction->state = kHAPDataStreamSendRequestTransactionState_WaitingForSend;
    transaction->messageCompletionHandler = completionHandler;
    transaction->_.messageIsMutable = messageIsMutable;
    if (messageIsMutable) {
        transaction->message.mutableBytes = messageBytes;
    } else {
        transaction->message.bytes = messageBytes;
    }
    transaction->numMessageBytes = (uint32_t) numMessageBytes;

    // Continue sending.
    if (dataStreamDescriptor->sendState == kHAPDataStreamDescriptorSendState_Idle) {
        BeginSendingPacket(server, dispatcher, dataStream);
    }
}

void HAPDataStreamDispatcherSendRequest(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* dataStreamProtocol,
        HAPDataStreamHandle dataStream,
        HAPDataStreamSendRequestTransaction* transaction,
        const HAPDataStreamSendRequestTransactionCallbacks* callbacks,
        const char* topic,
        const void* _Nullable messageBytes,
        size_t numMessageBytes,
        HAPDataStreamSendRequestMessageCompletionHandler completionHandler) {
    SendRequest(
            server,
            dispatcher,
            dataStreamProtocol,
            dataStream,
            transaction,
            callbacks,
            topic,
            false,
            (void*) (uintptr_t) messageBytes,
            numMessageBytes,
            completionHandler);
}

void HAPDataStreamDispatcherSendMutableRequest(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* dataStreamProtocol,
        HAPDataStreamHandle dataStream,
        HAPDataStreamSendRequestTransaction* transaction,
        const HAPDataStreamSendRequestTransactionCallbacks* callbacks,
        const char* topic,
        void* _Nullable messageBytes,
        size_t numMessageBytes,
        HAPDataStreamSendRequestMessageCompletionHandler completionHandler) {
    SendRequest(
            server,
            dispatcher,
            dataStreamProtocol,
            dataStream,
            transaction,
            callbacks,
            topic,
            true,
            messageBytes,
            numMessageBytes,
            completionHandler);
}

//----------------------------------------------------------------------------------------------------------------------

static void SendResponse(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataStreamReceiveRequestTransaction* transaction_,
        HAPDataStreamResponseStatus status,
        bool messageIsMutable,
        void* messageBytes,
        size_t numMessageBytes,
        HAPDataStreamReceiveRequestMessageCompletionHandler completionHandler) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream < dispatcher->storage->numDataStreams);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStream];
    HAPPrecondition(transaction_);
    HAPDataStreamReceiveRequestTransaction* transaction = (HAPDataStreamReceiveRequestTransaction*) transaction_;
    HAPAssert(transaction->state == kHAPDataStreamReceiveRequestTransactionState_WaitingForResponse);
    HAPBaseDataStreamProtocol* dataStreamProtocol = transaction->dataStreamProtocol;
    const char* protocolName = dataStreamProtocol->base->name;
    const char* topic = transaction->topic;
    HAPPrecondition(HAPDataStreamResponseStatusIsValid(status));
    HAPPrecondition(!numMessageBytes || messageBytes);
    HAPPrecondition(numMessageBytes <= kHAPDataStream_MaxPayloadBytes - 1 - GetResponseHeaderLen(protocolName, topic));
    HAPPrecondition(completionHandler);

    // Prepare sending.
    transaction->state = kHAPDataStreamReceiveRequestTransactionState_WaitingForSend;
    transaction->status = status;
    transaction->messageCompletionHandler = completionHandler;
    transaction->_.messageIsMutable = messageIsMutable;
    if (messageIsMutable) {
        transaction->message.mutableBytes = messageBytes;
    } else {
        transaction->message.bytes = messageBytes;
    }
    transaction->numMessageBytes = (uint32_t) numMessageBytes;

    // Continue sending.
    if (dataStreamDescriptor->sendState == kHAPDataStreamDescriptorSendState_Idle) {
        BeginSendingPacket(server, dispatcher, dataStream);
    }
}

void HAPDataStreamDispatcherSendResponse(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataStreamReceiveRequestTransaction* transaction,
        HAPDataStreamResponseStatus status,
        const void* messageBytes,
        size_t numMessageBytes,
        HAPDataStreamReceiveRequestMessageCompletionHandler completionHandler) {
    SendResponse(
            server,
            dispatcher,
            dataStream,
            transaction,
            status,
            false,
            (void*) (uintptr_t) messageBytes,
            numMessageBytes,
            completionHandler);
}

void HAPDataStreamDispatcherSendMutableResponse(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataStreamReceiveRequestTransaction* transaction,
        HAPDataStreamResponseStatus status,
        void* messageBytes,
        size_t numMessageBytes,
        HAPDataStreamReceiveRequestMessageCompletionHandler completionHandler) {
    SendResponse(
            server,
            dispatcher,
            dataStream,
            transaction,
            status,
            true,
            messageBytes,
            numMessageBytes,
            completionHandler);
}

//----------------------------------------------------------------------------------------------------------------------

const HAPDataStreamCallbacks kHAPDataStreamDispatcher_DataStreamCallbacks = { .handleAccept = HandleAccept,
                                                                              .handleInvalidate = HandleInvalidate,
                                                                              .handleData = HandleData };

#endif
