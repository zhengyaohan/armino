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
#include "HAPCharacteristic.h"
#include "HAPCharacteristicTypes.h"
#include "HAPDataStream.h"
#include "HAPLog+Attributes.h"
#include "HAPLogSubsystem.h"
#include "HAPServiceTypes.h"
#include "HAPSession.h"
#include "HAPTLV+Internal.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "RequestHandlers" };

/**
 * Returns description of Transport Type.
 *
 * @param      transportType        Value.
 *
 * @return Description of value.
 */
HAP_RESULT_USE_CHECK
static const char* GetTransportTypeDescription(HAPCharacteristicValue_DataStreamTransport_TransportType transportType) {
    switch (transportType) { // NOLINT(hicpp-multiway-paths-covered)
        case kHAPCharacteristicValue_DataStreamTransport_TransportType_TCP:
            return "HomeKit Data Stream over TCP";
        case kHAPCharacteristicValue_DataStreamTransport_TransportType_HAP:
            return "HomeKit Data Stream over HAP";
        default:
            HAPFatalError();
    }
}

/**
 * Serializes a Transfer Transport Configuration.
 *
 * @param      transportType        Transfer Transport Configuration.
 * @param      maxControllerTransportMTU  The maximum controller MTU to use (or
 *                                        kHAPDataStream_MaxControllerTransportMTU_NotSet)
 * @param      responseWriter       TLV writer for serializing the response.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If out of resources to process request.
 */
HAP_RESULT_USE_CHECK
static HAPError SerializeTransferTransportConfiguration(
        HAPCharacteristicValue_DataStreamTransport_TransportType transportType,
        unsigned maxControllerTransportMTU,
        HAPTLVWriter* responseWriter) {
    HAPPrecondition(responseWriter);

    HAPError err;

    HAPLogDebug(
            &logObject,
            "Transfer Transport Configuration:\n"
            "    Transport Type: %s",
            GetTransportTypeDescription(transportType));

    // Transfer Transport Configuration.
    {
        HAPTLVWriter subWriter;
        HAPTLVCreateSubWriter(&subWriter, responseWriter);

        // Transport Type.
        uint8_t transportTypeBytes[] = { transportType };
        err = HAPTLVWriterAppend(
                &subWriter,
                &(const HAPTLV) {
                        .type = kHAPCharacteristicValue_SupportedDataStreamTransportConfiguration_Configuration_TransportType,
                        .value = { .bytes = transportTypeBytes, .numBytes = sizeof transportTypeBytes } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }

        if (maxControllerTransportMTU != kHAPDataStream_MaxControllerTransportMTU_NotSet) {
            uint8_t maxControllerTransportMTUBytes[] = { HAPExpandLittleUInt32(maxControllerTransportMTU) };
            err = HAPTLVWriterAppend(
                    &subWriter,
                    &(const HAPTLV) {
                            .type = kHAPCharacteristicValue_SupportedDataStreamTransportConfiguration_Configuration_MaxControllerTransportMTU,
                            .value = { .bytes = maxControllerTransportMTUBytes,
                                       .numBytes = sizeof maxControllerTransportMTUBytes } });
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }
        }

        // Finalize.
        err = HAPTLVFinalizeSubWriter(
                &subWriter,
                responseWriter,
                kHAPCharacteristicValue_SupportedDataStreamTransportConfiguration_Configuration);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
    }

    return kHAPError_None;
}

/**
 * Write the setup parameters for TCP in the "Setup Data Stream" response.
 *
 * @param      responseWriter       TLV writer for serializing the response.
 * @param      status               Status (success or error) of the setup procedure.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If out of resources to process request.
 */
HAP_RESULT_USE_CHECK
static HAPError SerializeDataStreamAccessorySetupParams(
        HAPTLVWriter* responseWriter,
        const HAPDataStreamAccessorySetupParams* setupParams) {
    HAPError err;

    // Transport Type Session Parameters.
    {
        HAPTLVWriter subWriter;
        HAPTLVCreateSubWriter(&subWriter, responseWriter);

        // TCP Listening Port.
        if (setupParams->hasListenerPort) {
            HAPAssert(sizeof setupParams->listenerPort == sizeof(uint16_t));
            uint8_t listeningPortBytes[] = { HAPExpandLittleUInt16(setupParams->listenerPort) };
            err = HAPTLVWriterAppend(
                    &subWriter,
                    &(const HAPTLV) {
                            .type = kHAPCharacteristicValue_DataStreamTransport_SessionParameter_TCP_ListeningPort,
                            .value = { .bytes = listeningPortBytes, .numBytes = sizeof listeningPortBytes } });
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }
        }

        // Session Identifier.
        if (setupParams->sessionIdentifier != kHAPDataStreamHAPSessionIdentifierNone) {
            HAPAssert(sizeof setupParams->sessionIdentifier == sizeof(uint8_t));
            uint8_t sessionIdentifierBytes[] = { setupParams->sessionIdentifier };
            err = HAPTLVWriterAppend(
                    &subWriter,
                    &(const HAPTLV) {
                            .type = kHAPCharacteristicValue_DataStreamTransport_SessionParameter_SessionIdentifier,
                            .value = { .bytes = sessionIdentifierBytes, .numBytes = sizeof sessionIdentifierBytes } });
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }
        }
        // Finalize.
        err = HAPTLVFinalizeSubWriter(
                &subWriter,
                responseWriter,
                kHAPCharacteristicValue_SetupDataStreamTransport_Response_SessionParameters);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
    }

    // Accessory Key Salt.
    if (setupParams->hasAccessoryKeySalt) {
        err = HAPTLVWriterAppend(
                responseWriter,
                &(const HAPTLV) { .type = kHAPCharacteristicValue_SetupDataStreamTransport_Response_AccessoryKeySalt,
                                  .value = { .bytes = setupParams->accessoryKeySalt.bytes,
                                             .numBytes = sizeof setupParams->accessoryKeySalt.bytes } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
    }

    return kHAPError_None;
}

typedef struct {
    HAPCharacteristicValue_SetupDataStreamTransport_SessionCommandType sessionCommandType;
    HAPCharacteristicValue_DataStreamTransport_TransportType transportType;
    HAPDataStreamControllerSetupParams setupParams;
} DataStreamSetupWriteRequestParams;

/**
 * Parse the transport parameters in the "Setup Data Stream" write request.
 *
 * @param      requestReader        TLV reader for the setup request.
 * @param      setupParams          Output parameter to hold the parsed fields.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the data types are incorrect (bad enum values, etc).
 */
HAP_RESULT_USE_CHECK
static HAPError ParseDataStreamSetupWriteRequest(
        const HAPTLV8CharacteristicWriteRequest* request,
        HAPTLVReader* requestReader,
        DataStreamSetupWriteRequestParams* params) {
    HAPError err;

    HAPTLV sessionCommandTypeTLV, transportTypeTLV, controllerKeySaltTLV;
    sessionCommandTypeTLV.type = kHAPCharacteristicValue_SetupDataStreamTransport_Request_SessionCommandType;
    transportTypeTLV.type = kHAPCharacteristicValue_SetupDataStreamTransport_Request_TransportType;
    controllerKeySaltTLV.type = kHAPCharacteristicValue_SetupDataStreamTransport_Request_ControllerKeySalt;
    err = HAPTLVReaderGetAll(
            requestReader,
            (HAPTLV* const[]) { &sessionCommandTypeTLV, &transportTypeTLV, &controllerKeySaltTLV, NULL });
    if (err) {
        HAPLogCharacteristic(
                &logObject, request->characteristic, request->service, request->accessory, "Parse Failure.");
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }

    // Session Command Type. (Required)
    if (!sessionCommandTypeTLV.value.bytes) {
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Session Command Type missing.");
        return kHAPError_InvalidData;
    }
    if (sessionCommandTypeTLV.value.numBytes > sizeof(uint8_t)) {
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Session Command Type has invalid length (%lu).",
                (unsigned long) sessionCommandTypeTLV.value.numBytes);
        return kHAPError_InvalidData;
    }
    params->sessionCommandType =
            HAPReadUIntMax8(sessionCommandTypeTLV.value.bytes, sessionCommandTypeTLV.value.numBytes);

    //  - validate the enum values allowed
    switch (params->sessionCommandType) {
        case kHAPCharacteristicValue_SetupDataStreamTransport_SessionCommandType_StartSession:
            break;
        default:
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "Session Command Type invalid: %u.",
                    params->sessionCommandType);
            return kHAPError_InvalidData;
    }

    // Transport Type. (Required)
    if (!transportTypeTLV.value.bytes) {
        HAPLogCharacteristic(
                &logObject, request->characteristic, request->service, request->accessory, "Transport Type missing.");
        return kHAPError_InvalidData;
    }
    if (transportTypeTLV.value.numBytes > sizeof(uint8_t)) {
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Transport Type has invalid length (%lu).",
                (unsigned long) transportTypeTLV.value.numBytes);
        return kHAPError_InvalidData;
    }
    params->transportType = HAPReadUIntMax8(transportTypeTLV.value.bytes, transportTypeTLV.value.numBytes);

    //  - validate the enum values allowed
    switch (params->transportType) {
        case kHAPCharacteristicValue_DataStreamTransport_TransportType_TCP:
        case kHAPCharacteristicValue_DataStreamTransport_TransportType_HAP:
            break;
        default:
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "Transport Type invalid: %u.",
                    params->transportType);
            return kHAPError_InvalidData;
    }

    // Controller Key Salt. (Optional)
    if (controllerKeySaltTLV.value.bytes) {
        if (controllerKeySaltTLV.value.numBytes != sizeof(HAPDataStreamSalt)) {
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "Controller Key Salt has invalid length (%lu).",
                    (unsigned long) transportTypeTLV.value.numBytes);
            return kHAPError_InvalidData;
        }
        params->setupParams.controllerKeySalt = controllerKeySaltTLV.value.bytes;
    } else {
        params->setupParams.controllerKeySalt = NULL;
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleDataStreamTransportManagementSupportedDataStreamTransportConfigurationRead(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType,
            &kHAPCharacteristicType_SupportedDataStreamTransportConfiguration));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_DataStreamTransportManagement));
    HAPPrecondition(responseWriter);

    HAPError err;

    switch (HAPDataStreamGetActiveTransport(server, request->transportType)) {
        case kHAPDataStreamTransport_None:
            // No support.
            break;
        case kHAPDataStreamTransport_TCP:
            // HomeKit Data Stream over TCP.
            err = SerializeTransferTransportConfiguration(
                    kHAPCharacteristicValue_DataStreamTransport_TransportType_TCP,
                    kHAPDataStream_MaxControllerTransportMTU_NotSet,
                    responseWriter);
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }
            break;

        case kHAPDataStreamTransport_HAP: {
            // HomeKit Data Stream over HAP.
            unsigned maxControllerTransportMTU = HAPDataStreamGetMaxControllerTransportMTU(server);
            err = SerializeTransferTransportConfiguration(
                    kHAPCharacteristicValue_DataStreamTransport_TransportType_HAP,
                    maxControllerTransportMTU,
                    responseWriter);
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }
            break;
        }
    }

    return kHAPError_None;
}

/**
 * Invalidates Setup Data Stream Transport state.
 *
 * @param      server              Accessory server.
 */
static void InvalidateDataStreamSetup(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    // Cancel HomeKit Data Stream setup request.
    HAPDataStreamSetupCancel(server);

    // Reset setup state.
    HAPRawBufferZero(&server->dataStream.setup, sizeof server->dataStream.setup);
}

/**
 * Returns description of Status.
 *
 * @param      status               Value.
 *
 * @return Description of value.
 */
HAP_RESULT_USE_CHECK
static const char* GetStatusDescription(HAPCharacteristicValue_SetupDataStreamTransport_Status status) {
    switch (status) {
        case kHAPCharacteristicValue_SetupDataStreamTransport_Status_Success:
            return "Success";
        case kHAPCharacteristicValue_SetupDataStreamTransport_Status_GenericError:
            return "Generic error";
        case kHAPCharacteristicValue_SetupDataStreamTransport_Status_Busy:
            return "Busy";
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleDataStreamTransportManagementSetupDataStreamTransportRead(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_SetupDataStreamTransport));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_DataStreamTransportManagement));
    HAPPrecondition(responseWriter);

    HAPError err;

    // State errors can only happen here if a regular write -> read pair has been used to access this characteristic
    // instead of an atomic write response transaction.

    if (!server->dataStream.setup.session) {
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "No prior write to Setup Data Stream Transport detected.");
        return kHAPError_InvalidState;
    }
    if (server->dataStream.setup.session != request->session) {
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Write to Setup Data Stream Transport came from different HAP session.");
        return kHAPError_InvalidState;
    }

    HAPCharacteristicValue_SetupDataStreamTransport_Status status = server->dataStream.setup.status;

    HAPDataStreamAccessorySetupParams setupParams = { 0 };

    // Complete HomeKit Data Stream setup and get the setup parameters needed.
    err = HAPDataStreamSetupComplete(server, &setupParams);
    if (err) {
        HAPAssert(err == kHAPError_InvalidState);
        // This is not really a problem because setup could also time out shortly after the response is sent.
        // It just happens that in this case it timed out before the response has even been sent.
        // This can only happen if a regular write -> read pair is used instead of an atomic write response transaction.
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "HomeKit Data Stream setup failed (bad state).");
        // Overwrite status only if an error status is not already set.
        if (status == kHAPCharacteristicValue_SetupDataStreamTransport_Status_Success) {
            status = kHAPCharacteristicValue_SetupDataStreamTransport_Status_GenericError;
        }
    }

    // Status.
    uint8_t statusBytes[] = { status };
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPCharacteristicValue_SetupDataStreamTransport_Response_Status,
                              .value = { .bytes = statusBytes, .numBytes = sizeof statusBytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        InvalidateDataStreamSetup(server);
        return err;
    }

    // All other transport parameters (only set if there was no error in the setup procedure so far)
    if (!status) {
        HAPLogCharacteristicDebug(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Status: %s - TCP Listening Port: %u",
                GetStatusDescription(status),
                setupParams.listenerPort);
        err = SerializeDataStreamAccessorySetupParams(responseWriter, &setupParams);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            InvalidateDataStreamSetup(server);
            return err;
        }
    }

    InvalidateDataStreamSetup(server);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleDataStreamTransportManagementSetupDataStreamTransportWrite(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicWriteRequest* request,
        HAPTLVReader* requestReader,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->session);
    HAPPrecondition(HAPSessionIsSecured(request->session));
    HAPPrecondition(!HAPSessionIsTransient(request->session));
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_SetupDataStreamTransport));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_DataStreamTransportManagement));
    HAPPrecondition(requestReader);

    HAPError err;

    DataStreamSetupWriteRequestParams params = { 0 };

    err = ParseDataStreamSetupWriteRequest(request, requestReader, &params);
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        // No log (the function logs on every path already)
        return err;
    }

    // Start Session.
    HAPAssert(
            params.sessionCommandType ==
            kHAPCharacteristicValue_SetupDataStreamTransport_SessionCommandType_StartSession);

    // Check for concurrent Setup.
    if (server->dataStream.setup.session) {
        // Example sequence:
        // 1. Controller A issues Write to Setup Data Stream.
        // 2. Controller B issues Write to Setup Data Stream.
        // 3. Controller A issues Read to Setup Data Stream.
        // This should only happen if "Write Response" is not used,
        // as "Write Response" makes steps 1 and 3 be executed atomically.
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Aborting incomplete HomeKit Data Stream setup (%s).",
                "Setup Data Stream write done, but read not done");
        InvalidateDataStreamSetup(server);
    }

    // Activate Setup Data Stream Transport session.
    HAPAssert(!server->dataStream.setup.session);
    server->dataStream.setup.session = request->session;

    // Check if HomeKit Data Stream is supported.
    switch (HAPDataStreamGetActiveTransport(server, request->transportType)) {
        case kHAPDataStreamTransport_None:
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "Aborting HomeKit Data Stream setup: transport %u not allowed (none supported).",
                    params.transportType);
            server->dataStream.setup.status = kHAPCharacteristicValue_SetupDataStreamTransport_Status_GenericError;
            return kHAPError_None;

        case kHAPDataStreamTransport_TCP:
            if (params.transportType != kHAPCharacteristicValue_DataStreamTransport_TransportType_TCP) {
                HAPLogCharacteristic(
                        &logObject,
                        request->characteristic,
                        request->service,
                        request->accessory,
                        "Aborting HomeKit Data Stream setup: transport %u not allowed (is not TCP).",
                        params.transportType);
                server->dataStream.setup.status = kHAPCharacteristicValue_SetupDataStreamTransport_Status_GenericError;
                return kHAPError_None;
            }
            break;

        case kHAPDataStreamTransport_HAP:
            if (params.transportType != kHAPCharacteristicValue_DataStreamTransport_TransportType_HAP) {
                HAPLogCharacteristic(
                        &logObject,
                        request->characteristic,
                        request->service,
                        request->accessory,
                        "Aborting HomeKit Data Stream setup: transport %u not allowed (is not HAP).",
                        params.transportType);
                server->dataStream.setup.status = kHAPCharacteristicValue_SetupDataStreamTransport_Status_GenericError;
                return kHAPError_None;
            }
            break;
    }

    // Begin HomeKit Data Stream setup.
    err = HAPDataStreamSetupBegin(server, request->service, request->accessory, request->session, &params.setupParams);
    if (err == kHAPError_InvalidData) {
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Aborting HomeKit Data Stream setup due to invalid setup parameters from controller.");
        return kHAPError_InvalidData;
    } else if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        server->dataStream.setup.status = kHAPCharacteristicValue_SetupDataStreamTransport_Status_Busy;
        return kHAPError_None;
    }

    // Prepare HomeKit Data Stream.
    server->dataStream.setup.status = kHAPCharacteristicValue_SetupDataStreamTransport_Status_Success;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleDataStreamTransportManagementVersionRead(
        HAPAccessoryServer* server,
        const HAPStringCharacteristicReadRequest* request,
        char* value,
        size_t maxValueBytes,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(HAPUUIDAreEqual(request->characteristic->characteristicType, &kHAPCharacteristicType_Version));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_DataStreamTransportManagement));
    HAPPrecondition(value);

    if (sizeof kHAPDataStream_Version > maxValueBytes) {
        HAPLog(&logObject,
               "Not enough space to store %s (needed: %zu, available: %zu).",
               "Version",
               sizeof kHAPDataStream_Version,
               maxValueBytes);
        return kHAPError_OutOfResources;
    }
    HAPRawBufferCopyBytes(value, kHAPDataStream_Version, sizeof kHAPDataStream_Version);
    return kHAPError_None;
}
