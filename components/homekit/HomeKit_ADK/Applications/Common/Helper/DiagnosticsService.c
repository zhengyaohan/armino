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

#include "DiagnosticsService.h"
#include "ApplicationFeatures.h"

#if (HAVE_DIAGNOSTICS_SERVICE == 1)

#include "HAPDiagnostics.h"
#include "HAPPlatformDiagnostics.h"

static HAPDiagnosticsContext diagnosticsContext;
static bool isDiagnosticsInitialized = false;
#if !HAP_FEATURE_ENABLED(HAP_FEATURE_FILE_STORAGE)
static uint8_t diagnosticsLogBuffer[kAppDiagnosticsLogBufferSizeBytes];
#endif // HAP_FEATURE_ENABLED(HAP_FEATURE_FILE_STORAGE)

static HAPAccessoryDiagnosticsConfig accessoryDiagnosticsConfig = {
#if (HAVE_DIAGNOSTICS_TEXT_FORMAT == 1)
    .diagnosticsSnapshotFormat = kHAPDiagnosticsSnapshotFormat_Text,
#else
    .diagnosticsSnapshotFormat = kHAPDiagnosticsSnapshotFormat_Zip,
#endif
#if (HAVE_DIAGNOSTICS_MANUFACTURER == 1)
    .diagnosticsSnapshotType = kHAPDiagnosticsSnapshotType_Manufacturer,
#else
    .diagnosticsSnapshotType = kHAPDiagnosticsSnapshotType_ADK,
#endif

    .diagnosticsSnapshotOptions = kHAPDiagnosticsSnapshotOptions_ConfigurableMaxLogSize,
    .diagnosticsSupportedMode = kHAPCharacteristicValue_SupportedDiagnosticsModes_VerboseLogging,
    .diagnosticsContext = &diagnosticsContext
};

HAP_RESULT_USE_CHECK
HAPError GetAccessoryDiagnosticsConfig(
        HAPAccessoryServer* _Nullable server HAP_UNUSED,
        const HAPAccessory* _Nullable accessory HAP_UNUSED,
        HAPAccessoryDiagnosticsConfig* diagnosticsConfigStruct,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    HAPPrecondition(diagnosticsConfigStruct);

    HAPRawBufferCopyBytes(diagnosticsConfigStruct, &accessoryDiagnosticsConfig, sizeof accessoryDiagnosticsConfig);
    diagnosticsConfigStruct->diagnosticsContext = &diagnosticsContext;
    return kHAPError_None;
}

void InitializeDiagnostics(uint32_t* selectedModeState, const HAPAccessory* accessory, const HAPPlatform* hapPlatform) {
    HAPPrecondition(accessory);
    HAPPrecondition(hapPlatform);
    diagnosticsContext.accessory = accessory;
    diagnosticsContext.hapPlatform = hapPlatform;
    diagnosticsContext.diagnosticsSelectedMode = selectedModeState;
#if !HAP_FEATURE_ENABLED(HAP_FEATURE_FILE_STORAGE)
    diagnosticsContext.logBuffer = diagnosticsLogBuffer;
    diagnosticsContext.logBufferSizeBytes = sizeof diagnosticsLogBuffer;
#endif // !HAP_FEATURE_ENABLED(HAP_FEATURE_FILE_STORAGE)
    HAPPlatformDiagnosticsInitialize(&accessoryDiagnosticsConfig);
    isDiagnosticsInitialized = true;

#if HAP_FEATURE_ENABLED(HAP_FEATURE_FILE_STORAGE)
    // Using HAPDiagnosticsSetupLogCaptureToFile is optional.
    // Only needed if HAP logs need to be captured
    if (kHAPError_None !=
        HAPDiagnosticsSetupLogCaptureToFile(
                kAppDiagnosticsFolderName, kAppDiagnosticsLogFileName, kAppDiagnosticsMaxLogFileSizeMB, accessory)) {
        HAPLogError(&kHAPLog_Default, "Failed to setup diagnostics logging");
        HAPFatalError();
    }
#else
    HAPRawBufferZero(diagnosticsLogBuffer, sizeof diagnosticsLogBuffer);
#endif // HAP_FEATURE_ENABLED(HAP_FEATURE_FILE_STORAGE)
}
void DeinitializeDiagnostics(const HAPAccessory* accessory) {
    HAPPrecondition(accessory);
#if HAP_FEATURE_ENABLED(HAP_FEATURE_FILE_STORAGE)
    HAPDiagnosticsStopLogCaptureToFile(accessory);
#else
    HAPRawBufferZero(diagnosticsLogBuffer, sizeof diagnosticsLogBuffer);
#endif // #if HAP_FEATURE_ENABLED(HAP_FEATURE_FILE_STORAGE)
    HAPPlatformDiagnosticsDeinitialize();
    isDiagnosticsInitialized = false;
}

void DiagnosticsHandlePairingStateChange(
        HAPPairingStateChange state,
        HAPCharacteristicValue_SelectedDiagnosticsModes* selectedModeState,
        SaveAccessoryStateFunc saveAccessoryStateFunc) {
    HAPPrecondition(selectedModeState);
    HAPPrecondition(saveAccessoryStateFunc);
    switch (state) {
        case kHAPPairingStateChange_Paired:
            break;
        case kHAPPairingStateChange_Unpaired: {
            *selectedModeState = kHAPCharacteristicValue_SelectedDiagnosticsModes_None;
            // Save accessory state in order to save the updated setting into persistent memory.
            saveAccessoryStateFunc();
            break;
        }
    }
}

/**
 * Handle write request to the 'Selected Diagnostics Mode' characteristic of the Diagnostics service.
 *
 * @param      server                  Accessory server.
 * @param      request                 Characteristic write request.
 * @param      value                   Value for the request.
 * @param      context                 Context pointer.
 * @param      selectedModeState       State of selected mode
 * @param      saveAccessoryStateFunc  Function to call to save the accessory state in persistent memory.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If out of resources to process request.
 */
HAP_RESULT_USE_CHECK
HAPError HandleSelectedDiagnosticsModesWriteHelper(
        HAPAccessoryServer* server,
        const HAPUInt32CharacteristicWriteRequest* request,
        uint32_t value,
        void* _Nullable context HAP_UNUSED,
        HAPCharacteristicValue_SelectedDiagnosticsModes* selectedModeState,
        SaveAccessoryStateFunc saveAccessoryStateFunc) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->service);
    HAPPrecondition(request->accessory);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_SelectedDiagnosticsModes));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_Diagnostics));
    HAPPrecondition(selectedModeState);
    HAPPrecondition(diagnosticsContext.diagnosticsSelectedMode);
    HAPPrecondition(saveAccessoryStateFunc);

    if (value != *selectedModeState) {
        *selectedModeState = value;
        if (isDiagnosticsInitialized == true) {
            // Re-initialize diagnostics with updated settings
            HAPPlatformDiagnosticsInitialize(&accessoryDiagnosticsConfig);
        }
        // Save accessory state in order to save the updated setting into persistent memory.
        saveAccessoryStateFunc();
    }
    return kHAPError_None;
}

#endif // HAVE_DIAGNOSTICS_SERVICE
