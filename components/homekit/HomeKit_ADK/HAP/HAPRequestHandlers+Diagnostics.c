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

#include "HAP+API.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)

#include "HAPCharacteristicTypes.h"
#include "HAPLogSubsystem.h"
#include "HAPPlatformDiagnostics.h"
#include "HAPServiceTypes.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "RequestHandlers" };
/**
 * Handle read request to the 'Supported Diagnostics Snapshot' characteristic of the Diagnostics service.
 *
 * @param      server          Accessory server.
 * @param      request         Characteristic read request.
 * @param      responseWriter  TLV writer for serializing the response.
 * @param      context         Context pointer.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If out of resources to process request.
 */
HAP_RESULT_USE_CHECK
HAPError HAPHandleSupportedDiagnosticsSnapshotRead(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->service);
    HAPPrecondition(request->accessory);
    HAPPrecondition(responseWriter);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_SupportedDiagnosticsSnapshot));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_Diagnostics));

    HAPLogInfo(&logObject, "%s", __func__);

    HAPError err;

    HAPAccessoryDiagnosticsConfig accessoryDiagnosticsConfig;
    HAPRawBufferZero(&accessoryDiagnosticsConfig, sizeof accessoryDiagnosticsConfig);

    HAPAssert(request->accessory->callbacks.diagnosticsConfig.getDiagnosticsConfig);

    err = request->accessory->callbacks.diagnosticsConfig.getDiagnosticsConfig(
            server, request->accessory, &accessoryDiagnosticsConfig, context);
    if (err) {
        return err;
    }

    // Validate diagnostics snapshot format
    switch (accessoryDiagnosticsConfig.diagnosticsSnapshotFormat) {
        case kHAPDiagnosticsSnapshotFormat_Zip: {
            HAPLogInfo(&logObject, "Diagnostics Snapshot Format : [zip]");
            break;
        }
        case kHAPDiagnosticsSnapshotFormat_Text: {
            HAPLogInfo(&logObject, "Diagnostics Snapshot Format : [text]");
            break;
        }
        default: {
            HAPLogError(
                    &kHAPLog_Default,
                    "Unknown Diagnostics Snapshot Format : [%u]",
                    accessoryDiagnosticsConfig.diagnosticsSnapshotFormat);
            HAPFatalError();
        }
    }

    uint8_t formatBytes[] = { (uint8_t) accessoryDiagnosticsConfig.diagnosticsSnapshotFormat };
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPCharacteristicValue_SupportedDiagnosticsSnapshot_Format,
                              .value = { .bytes = formatBytes, .numBytes = sizeof(formatBytes) } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    bool isValidValue = false;

    // Validate diagnostics snapshot type
    if ((accessoryDiagnosticsConfig.diagnosticsSnapshotType & kHAPDiagnosticsSnapshotType_Manufacturer) != 0) {
        HAPLogInfo(&logObject, "Supported Diagnostics Snapshot Type : [Manufacturer snapshot]");
        isValidValue = true;
    }
    if ((accessoryDiagnosticsConfig.diagnosticsSnapshotType & kHAPDiagnosticsSnapshotType_ADK) != 0) {
        HAPLogInfo(&logObject, "Supported Diagnostics Snapshot Type : [ADK snapshot]");
        isValidValue = true;
    }
    if (isValidValue == false) {
        HAPLogError(
                &kHAPLog_Default,
                "Unknown Diagnostics Snapshot Type : [%u]",
                accessoryDiagnosticsConfig.diagnosticsSnapshotType);
        HAPFatalError();
    }

    uint8_t typeBytes[] = { (uint8_t) accessoryDiagnosticsConfig.diagnosticsSnapshotType };
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPCharacteristicValue_SupportedDiagnosticsSnapshot_Type,
                              .value = { .bytes = typeBytes, .numBytes = sizeof(typeBytes) } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // Validate diagnostics snapshot options
    isValidValue = false;
    if (accessoryDiagnosticsConfig.diagnosticsSnapshotOptions == kHAPDiagnosticsSnapshotOptions_None) {
        HAPLogInfo(&logObject, "Supported Diagnostics Snapshot Options : [None]");
        isValidValue = true;
    }
    if ((accessoryDiagnosticsConfig.diagnosticsSnapshotOptions &
         kHAPDiagnosticsSnapshotOptions_ConfigurableMaxLogSize) != 0) {
        HAPLogInfo(&logObject, "Supported Diagnostics Snapshot Options : [Configurable max log size]");
        isValidValue = true;
    }
    if (isValidValue == false) {
        HAPLogError(
                &kHAPLog_Default,
                "Unknown Diagnostics Snapshot Options : [%u]",
                accessoryDiagnosticsConfig.diagnosticsSnapshotOptions);
        HAPFatalError();
    }

    uint16_t optionsBytes[] = { (uint16_t) accessoryDiagnosticsConfig.diagnosticsSnapshotOptions };
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPCharacteristicValue_SupportedDiagnosticsSnapshot_Options,
                              .value = { .bytes = optionsBytes, .numBytes = sizeof(optionsBytes) } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    return kHAPError_None;
}

/**
 * Handle read request to the 'Supported Diagnostics Mode' characteristic of the Diagnostics service.
 *
 * @param      server          Accessory server.
 * @param      request         Characteristic read request.
 * @param      value           Value for the response.
 * @param      context         Context pointer.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If out of resources to process request.
 */
HAP_RESULT_USE_CHECK
HAPError HAPHandleSupportedDiagnosticsModesRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt32CharacteristicReadRequest* request HAP_UNUSED,
        uint32_t* value,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->service);
    HAPPrecondition(request->accessory);
    HAPPrecondition(value);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_SupportedDiagnosticsModes));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_Diagnostics));
    HAPLogInfo(&logObject, "%s", __func__);

    HAPError err;
    HAPAccessoryDiagnosticsConfig accessoryDiagnosticsConfig;
    HAPRawBufferZero(&accessoryDiagnosticsConfig, sizeof accessoryDiagnosticsConfig);

    HAPAssert(request->accessory->callbacks.diagnosticsConfig.getDiagnosticsConfig);

    err = request->accessory->callbacks.diagnosticsConfig.getDiagnosticsConfig(
            server, request->accessory, &accessoryDiagnosticsConfig, context);
    if (err) {
        return err;
    }
    *value = accessoryDiagnosticsConfig.diagnosticsSupportedMode;

    return kHAPError_None;
}

/**
 * Handle read request to the 'Selected Diagnostics Mode' characteristic of the Diagnostics service.
 *
 * @param      server          Accessory server.
 * @param      request         Characteristic read request.
 * @param      value           Value for the response.
 * @param      context         Context pointer.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If out of resources to process request.
 */
HAP_RESULT_USE_CHECK
HAPError HAPHandleSelectedDiagnosticsModesRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt32CharacteristicReadRequest* request HAP_UNUSED,
        uint32_t* value,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->service);
    HAPPrecondition(request->accessory);
    HAPPrecondition(value);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_SelectedDiagnosticsModes));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_Diagnostics));

    HAPLogInfo(&logObject, "%s", __func__);

    HAPError err;
    HAPAccessoryDiagnosticsConfig accessoryDiagnosticsConfig;
    HAPRawBufferZero(&accessoryDiagnosticsConfig, sizeof accessoryDiagnosticsConfig);

    HAPAssert(request->accessory->callbacks.diagnosticsConfig.getDiagnosticsConfig);

    err = request->accessory->callbacks.diagnosticsConfig.getDiagnosticsConfig(
            server, request->accessory, &accessoryDiagnosticsConfig, context);
    if (err) {
        return err;
    }
    HAPDiagnosticsContext* dgContext = (HAPDiagnosticsContext*) (accessoryDiagnosticsConfig.diagnosticsContext);
    HAPPrecondition(dgContext);
    HAPPrecondition(dgContext->diagnosticsSelectedMode);

    *value = *(dgContext->diagnosticsSelectedMode);

    return kHAPError_None;
}

#endif // HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)
