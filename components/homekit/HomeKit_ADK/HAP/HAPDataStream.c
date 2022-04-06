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

#include "HAPAccessoryServer+Internal.h"
#include "HAPDataStream+Internal.h"
#include "HAPDataStreamRef.h"
#include "HAPLogSubsystem.h"

#include "HAPDataStream+HAP.h"
#include "HAPDataStream+TCP.h"

#ifdef __cplusplus
extern "C" {
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP) || HAP_FEATURE_ENABLED(HAP_FEATURE_HDS_TRANSPORT_OVER_HAP)

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "DataStream" };

/**
 * A "transport" for Data Stream must implement the following set of functions.
 *
 * Each function's usage is defined in the transport header (see HAPDataStream+TCP.h for example).
 */
struct HAPDataStreamTransportStruct {
    HAPError (*const _Nonnull SetupBegin)(
            HAPAccessoryServer* server,
            const HAPService* service,
            const HAPAccessory* accessory,
            HAPSession* session,
            const HAPDataStreamControllerSetupParams* setupParams);

    void (*const _Nonnull SetupCancel)(HAPAccessoryServer* server);

    HAP_RESULT_USE_CHECK
    HAPError (*const _Nonnull SetupComplete)(
            HAPAccessoryServer* server,
            HAPDataStreamAccessorySetupParams* setupParams);

    void (*const _Nonnull Invalidate)(HAPAccessoryServer* server, HAPDataStreamRef* dataStream);

    void (*const _Nonnull InvalidateAllForHAPPairingID)(
            HAPAccessoryServer* server,
            int pairingID,
            HAPDataStreamRef* dataStream);

    void (*const _Nonnull InvalidateAllForHAPSession)(
            HAPAccessoryServer* server,
            HAPSession* _Nullable session,
            HAPDataStreamRef* dataStream);

    void (*const _Nonnull GetRequestContext)(
            HAPAccessoryServer* server,
            HAPDataStreamRef* dataStream,
            HAPDataStreamRequest* request);

    HAP_RESULT_USE_CHECK
    HAPDataStreamTransmission* (*const _Nonnull GetReceiveTransmission)(HAPDataStreamRef* dataStream);

    HAP_RESULT_USE_CHECK
    HAPDataStreamTransmission* (*const _Nonnull GetSendTransmission)(HAPDataStreamRef* dataStream);

    void (*const _Nonnull DoReceive)(HAPAccessoryServer* server, HAPDataStreamRef* dataStream);

    void (*const _Nonnull DoSend)(HAPAccessoryServer* server, HAPDataStreamRef* dataStream);
    void (*const _Nullable PrepareSessionResume)(
            HAPAccessoryServer* server,
            uint8_t* resumedSessionSecret,
            HAPSession* session);
    void (*const _Nullable SetMinimumPriority)(
            HAPAccessoryServer* server,
            HAPDataStreamRef* dataStream,
            HAPStreamPriority priority);
};

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
static const struct HAPDataStreamTransportStruct DataStreamTCPTransport = {
    .SetupBegin = HAPDataStreamTCPSetupBegin,
    .SetupCancel = HAPDataStreamTCPSetupCancel,
    .SetupComplete = HAPDataStreamTCPSetupComplete,
    .Invalidate = HAPDataStreamTCPInvalidate,
    .InvalidateAllForHAPPairingID = HAPDataStreamTCPInvalidateAllForHAPPairingID,
    .InvalidateAllForHAPSession = HAPDataStreamTCPInvalidateAllForHAPSession,
    .GetRequestContext = HAPDataStreamTCPGetRequestContext,
    .GetReceiveTransmission = HAPDataStreamTCPGetReceiveTransmission,
    .GetSendTransmission = HAPDataStreamTCPGetSendTransmission,
    .DoReceive = HAPDataStreamTCPDoReceive,
    .DoSend = HAPDataStreamTCPDoSend,
    .SetMinimumPriority = HAPDataStreamTCPSetMinimumPriority,
};
#endif // #if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)

static const struct HAPDataStreamTransportStruct DataStreamHAPTransport = {
    .SetupBegin = HAPDataStreamHAPSetupBegin,
    .SetupCancel = HAPDataStreamHAPSetupCancel,
    .SetupComplete = HAPDataStreamHAPSetupComplete,
    .Invalidate = HAPDataStreamHAPInvalidate,
    .InvalidateAllForHAPPairingID = HAPDataStreamHAPInvalidateAllForHAPPairingID,
    .InvalidateAllForHAPSession = HAPDataStreamHAPInvalidateAllForHAPSession,
    .GetRequestContext = HAPDataStreamHAPGetRequestContext,
    .GetReceiveTransmission = HAPDataStreamHAPGetReceiveTransmission,
    .GetSendTransmission = HAPDataStreamHAPGetSendTransmission,
    .DoReceive = HAPDataStreamHAPDoReceive,
    .DoSend = HAPDataStreamHAPDoSend,
    .PrepareSessionResume = HAPDataStreamHAPPrepareSessionResume,
};

#define GetTransport(server) (((HAPAccessoryServer* const)(server))->dataStream.transport)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void HAPDataStreamPrepareStart(HAPAccessoryServer* server) {
    HAPPrecondition(server);
    if (GetTransport(server)) {
        for (size_t i = 0; i < server->dataStream.numDataStreams; i++) {
            HAPDataStreamRef* dataStream = &server->dataStream.dataStreams[i];
            HAPRawBufferZero(dataStream, sizeof *dataStream);
        }
    }

    HAPRawBufferZero(&server->dataStream.setup, sizeof server->dataStream.setup);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void HAPDataStreamPrepareStop(HAPAccessoryServer* server) {
    HAPPrecondition(server);
    if (GetTransport(server)) {
        HAPDataStreamInvalidateAllForHAPSession(server, /* session: */ NULL);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void HAPDataStreamStop(HAPAccessoryServer* server) {
    HAPPrecondition(server);
    if (GetTransport(server)) {
        HAPDataStreamInvalidateAllForHAPSession(server, /* session: */ NULL);
    }
}

// All other functions are compiled out completely and not stubbed.

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void HAPDataStreamInitializeServer(HAPAccessoryServer* server, HAPDataStreamRef* dataStreams, size_t numDataStreams) {
    HAPPrecondition(server);
    HAPPrecondition(dataStreams);

    for (size_t i = 0; i < numDataStreams; i++) {
        HAPDataStreamRef* const dataStream = &dataStreams[i];
        HAPRawBufferZero(dataStream, sizeof *dataStream);
    }

    server->dataStream.dataStreams = dataStreams;
    server->dataStream.numDataStreams = numDataStreams;
}

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
void HAPDataStreamCreateWithTCPManager(
        HAPAccessoryServer* server,
        HAPPlatformTCPStreamManagerRef tcpStreamManager,
        HAPDataStreamRef* dataStreams,
        size_t numDataStreams) {
    HAPPrecondition(server);
    HAPPrecondition(tcpStreamManager);
    HAPPrecondition(dataStreams);
    HAPDataStreamTCPTransportState* const transportState =
            (HAPDataStreamTCPTransportState*) (&server->dataStream.transportState);

    // Set the active transport to TCP.
    HAPAssert(!GetTransport(server));
    server->dataStream.transport = &DataStreamTCPTransport;

    // Initialize the DataStream storage arrays.
    HAPDataStreamInitializeServer(server, dataStreams, numDataStreams);

    // Copy over the manager into the ServerRef.
    transportState->tcpStreamManager = tcpStreamManager;
}
#endif // HAP_FEATURE_ENABLED(HAP_FEATURE_IP)

void HAPDataStreamCreateWithHAP(HAPAccessoryServer* server, HAPDataStreamRef* dataStreams, size_t numDataStreams) {
    HAPPrecondition(server);
    HAPPrecondition(dataStreams);

    // Set the transport active.
    HAPAssert(!GetTransport(server)); // Should only be called once.
    server->dataStream.transport = &DataStreamHAPTransport;

    // Initialize the Data Stream storage.
    HAPDataStreamInitializeServer(server, dataStreams, numDataStreams);
}

HAPDataStreamTransport HAPDataStreamGetActiveTransport(HAPAccessoryServer* server, HAPTransportType transportType) {
    HAPPrecondition(server);
    struct HAPDataStreamTransportStruct const* const transport HAP_UNUSED = GetTransport(server);

    // HomeKit Data Stream transports.
    switch (transportType) {
        case kHAPTransportType_IP:
#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
            if (transport == &DataStreamTCPTransport) {
                return kHAPDataStreamTransport_TCP;
            }
#endif // HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
            if (transport == &DataStreamHAPTransport) {
                return kHAPDataStreamTransport_HAP;
            }
            return kHAPDataStreamTransport_None;

        case kHAPTransportType_BLE:
            if (transport == &DataStreamHAPTransport) {
                return kHAPDataStreamTransport_HAP;
            }
            return kHAPDataStreamTransport_None;

        case kHAPTransportType_Thread:
            if (transport == &DataStreamHAPTransport) {
                return kHAPDataStreamTransport_HAP;
            }
            return kHAPDataStreamTransport_None;
    }

    HAPLogError(&kHAPLog_Default, "Unknown transport type %d", transportType);
    HAPFatalError();
    return kHAPDataStreamTransport_None;
}

HAP_RESULT_USE_CHECK
HAPDataStreamTransport HAPDataStreamGetActiveTransportForSession(HAPAccessoryServer* server, HAPSession* session) {
    HAPPrecondition(session);
    return HAPDataStreamGetActiveTransport(server, session->transportType);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HAP_RESULT_USE_CHECK
HAPError HAPDataStreamSetupBegin(
        HAPAccessoryServer* server,
        const HAPService* service,
        const HAPAccessory* accessory,
        HAPSession* session,
        const HAPDataStreamControllerSetupParams* setupParams) {
    HAPPrecondition(server);
    HAPPrecondition(service);
    HAPPrecondition(accessory);
    HAPPrecondition(HAPSessionIsSecured(session) && !HAPSessionIsTransient(session));
    HAPPrecondition(setupParams);

    if (GetTransport(server)) {
        return GetTransport(server)->SetupBegin(server, service, accessory, session, setupParams);
    }

    HAPLogError(&logObject, "Illegal call to begin Data Stream (not active).");
    return kHAPError_InvalidState;
}

void HAPDataStreamSetupCancel(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    if (GetTransport(server)) {
        GetTransport(server)->SetupCancel(server);
        return;
    }

    // If not running, just go forward without error.
}

HAP_RESULT_USE_CHECK
HAPError HAPDataStreamSetupComplete(HAPAccessoryServer* server, HAPDataStreamAccessorySetupParams* setupParams) {
    HAPPrecondition(server);
    HAPPrecondition(setupParams);

    if (GetTransport(server)) {
        return GetTransport(server)->SetupComplete(server, setupParams);
    }

    HAPLogError(&logObject, "Illegal call to setup complete (not active).");
    return kHAPError_InvalidState;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void HAPDataStreamGetRequestContext(
        HAPAccessoryServer* server,
        HAPDataStreamRef* dataStream,
        HAPDataStreamRequest* request) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream);
    HAPPrecondition(request);
    HAPPrecondition(GetTransport(server));

    GetTransport(server)->GetRequestContext(server, dataStream, request);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void HAPDataStreamInvalidate(HAPAccessoryServer* server, HAPDataStreamRef* dataStream) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream);
    HAPPrecondition(GetTransport(server));

    GetTransport(server)->Invalidate(server, dataStream);
}

void HAPDataStreamInvalidateAllForHAPPairingID(HAPAccessoryServer* server, int pairingID) {
    HAPPrecondition(server);

    if (!GetTransport(server)) {
        return;
    }

    // Abort pending Setup Data Stream Transport transaction (HomeKit characteristic).
    if (server->dataStream.setup.session) {
        if (server->dataStream.setup.session->hap.pairingID == pairingID) {
            HAPLog(&logObject, "Aborting incomplete HomeKit Data Stream setup (Invalidate HAP Pairing ID).");
            HAPRawBufferZero(&server->dataStream.setup, sizeof server->dataStream.setup);
        }
    }

    // Invalidate HomeKit Data Streams that are relevant.
    for (size_t i = 0; i < server->dataStream.numDataStreams; i++) {
        HAPDataStreamRef* dataStream = &server->dataStream.dataStreams[i];
        GetTransport(server)->InvalidateAllForHAPPairingID(server, pairingID, dataStream);
    }
}

void HAPDataStreamInvalidateAllForHAPSession(HAPAccessoryServer* server, HAPSession* _Nullable session) {
    HAPPrecondition(server);

    if (!GetTransport(server)) {
        return;
    }

    // Abort pending Setup Data Stream Transport transaction (HomeKit characteristic).
    if (server->dataStream.setup.session) {
        if (session == NULL || server->dataStream.setup.session == session) {
            HAPLog(&logObject,
                   "Aborting incomplete HomeKit Data Stream setup (%s).",
                   session ? "Invalidate All" : "Invalidate HAP session");
            HAPRawBufferZero(&server->dataStream.setup, sizeof server->dataStream.setup);
        }
    }

    // Invalidate HomeKit Data Streams that are relevant.
    for (size_t i = 0; i < server->dataStream.numDataStreams; i++) {
        HAPDataStreamRef* dataStream = &server->dataStream.dataStreams[i];
        GetTransport(server)->InvalidateAllForHAPSession(server, session, dataStream);
    }

    // For "invalidate all" case, check that they were all invalidated properly.
    if (!session) {
        HAPAssert(HAPRawBufferIsZero(
                HAPNonnull(server->dataStream.dataStreams),
                server->dataStream.numDataStreams * sizeof *server->dataStream.dataStreams));
    }
}

void HAPDataStreamReceiveData(
        HAPAccessoryServer* server,
        HAPDataStreamRef* dataStream,
        void* dataBytes,
        size_t numDataBytes,
        HAPDataStreamDataCompletionHandler completionHandler) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream);
    HAPPrecondition(dataBytes);
    HAPPrecondition(completionHandler);
    HAPPrecondition(GetTransport(server));

    HAPDataStreamTransmission* const tx = GetTransport(server)->GetReceiveTransmission(dataStream);
    HAPPrecondition(tx->state == kHAPDataStreamTransmissionState_Data);
    HAPPrecondition(!tx->completionHandler);
    HAPPrecondition(numDataBytes <= tx->totalDataBytes);

    HAPLogDebug(&logObject, "[%p] Receiving data (%zu bytes).", (const void*) dataStream, numDataBytes);

    // Retain payload.
    tx->completionHandler = completionHandler;
    tx->dataIsMutable = true;
    tx->data.mutableBytes = dataBytes;
    tx->numDataBytes = numDataBytes;

    // Begin receiving.
    GetTransport(server)->DoReceive(server, dataStream);
}

void HAPDataStreamSkipData(
        HAPAccessoryServer* server,
        HAPDataStreamRef* dataStream,
        size_t numDataBytes,
        HAPDataStreamDataCompletionHandler completionHandler) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream);
    HAPPrecondition(completionHandler);
    HAPPrecondition(GetTransport(server));

    HAPDataStreamTransmission* const tx = GetTransport(server)->GetReceiveTransmission(dataStream);
    HAPPrecondition(tx->state == kHAPDataStreamTransmissionState_Data);
    HAPPrecondition(!tx->completionHandler);
    HAPPrecondition(numDataBytes <= tx->totalDataBytes);

    HAPLogDebug(&logObject, "[%p] Skipping data (%zu bytes).", (const void*) dataStream, numDataBytes);

    // Retain payload.
    tx->completionHandler = completionHandler;
    tx->dataIsMutable = false;
    tx->data.mutableBytes = NULL;
    tx->numDataBytes = numDataBytes;

    // Begin receiving.
    GetTransport(server)->DoReceive(server, dataStream);
}

void HAPDataStreamPrepareData(
        HAPAccessoryServer* server,
        HAPDataStreamRef* dataStream,
        size_t totalDataBytes,
        HAPDataStreamDataCompletionHandler completionHandler) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream);
    HAPPrecondition(totalDataBytes <= kHAPDataStream_MaxPayloadBytes);
    HAPPrecondition(completionHandler);
    HAPPrecondition(GetTransport(server));

    HAPDataStreamTransmission* const tx = GetTransport(server)->GetSendTransmission(dataStream);
    HAPPrecondition(tx->state == kHAPDataStreamTransmissionState_Idle);
    HAPPrecondition(!tx->completionHandler);

    // Retain payload.
    tx->completionHandler = completionHandler;
    tx->totalDataBytes = totalDataBytes;
    tx->state = kHAPDataStreamTransmissionState_PrepareFrame;

    // Begin sending.
    GetTransport(server)->DoSend(server, dataStream);
}

void HAPDataStreamSendData(
        HAPAccessoryServer* server,
        HAPDataStreamRef* dataStream,
        const void* dataBytes,
        size_t numDataBytes,
        HAPDataStreamDataCompletionHandler completionHandler) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream);
    HAPPrecondition(dataBytes);
    HAPPrecondition(completionHandler);
    HAPPrecondition(GetTransport(server));

    HAPDataStreamTransmission* const tx = GetTransport(server)->GetSendTransmission(dataStream);
    HAPPrecondition(tx->state == kHAPDataStreamTransmissionState_Data);
    HAPPrecondition(!tx->completionHandler);
    HAPPrecondition(numDataBytes <= tx->totalDataBytes);

    // Retain payload.
    tx->completionHandler = completionHandler;
    tx->dataIsMutable = false;
    tx->data.bytes = dataBytes;
    tx->numDataBytes = numDataBytes;

    // Begin sending.
    GetTransport(server)->DoSend(server, dataStream);
}

void HAPDataStreamSendMutableData(
        HAPAccessoryServer* server,
        HAPDataStreamRef* dataStream,
        void* dataBytes,
        size_t numDataBytes,
        HAPDataStreamDataCompletionHandler completionHandler) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream);
    HAPPrecondition(dataBytes);
    HAPPrecondition(completionHandler);
    HAPPrecondition(GetTransport(server));

    HAPDataStreamTransmission* const tx = GetTransport(server)->GetSendTransmission(dataStream);
    HAPPrecondition(tx->state == kHAPDataStreamTransmissionState_Data);
    HAPPrecondition(!tx->completionHandler);
    HAPPrecondition(numDataBytes <= tx->totalDataBytes);

    // Retain payload.
    tx->completionHandler = completionHandler;
    tx->dataIsMutable = true;
    tx->data.mutableBytes = dataBytes;
    tx->numDataBytes = numDataBytes;

    // Begin sending.
    GetTransport(server)->DoSend(server, dataStream);
}

void HAPDataStreamPrepareSessionResume(HAPAccessoryServer* server, uint8_t* resumedSessionSecret, HAPSession* session) {
    HAPPrecondition(server);
    HAPPrecondition(resumedSessionSecret);
    HAPPrecondition(session);

    if (!GetTransport(server)) {
        return;
    }

    if (GetTransport(server)->PrepareSessionResume) {
        GetTransport(server)->PrepareSessionResume(server, resumedSessionSecret, session);
    }
}

void HAPDataStreamSetMinimumPriority(
        HAPAccessoryServer* server,
        HAPDataStreamRef* dataStream,
        HAPStreamPriority priority) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream);

    if (!GetTransport(server)) {
        return;
    }

    if (GetTransport(server)->SetMinimumPriority) {
        GetTransport(server)->SetMinimumPriority(server, dataStream, priority);
    }
}

#endif

#ifdef __cplusplus
}
#endif
