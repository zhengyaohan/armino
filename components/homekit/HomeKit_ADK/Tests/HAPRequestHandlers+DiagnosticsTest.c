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

#include "HAPPlatform+Init.h"
#include "HAPPlatformFeatures.h"

#include "Harness/HAPAccessoryServerHelper.h"
#include "Harness/HAPPlatformClockHelper.h"
#include "Harness/HAPTestController.c"
#include "Harness/IPTestHelpers.h"
#include "Harness/SecurityTestHelpers.h"
#include "Harness/TemplateDB.c"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)

static HAPAccessoryDiagnosticsConfig accessoryDiagnosticsConfig = {
    .diagnosticsSnapshotFormat = kHAPDiagnosticsSnapshotFormat_Zip,
    .diagnosticsSnapshotType = kHAPDiagnosticsSnapshotType_Manufacturer,
    .diagnosticsSnapshotOptions = kHAPDiagnosticsSnapshotOptions_ConfigurableMaxLogSize,

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
    return kHAPError_None;
}

static const HAPAccessory accessory = {
    .aid = 1,
    .category = kHAPAccessoryCategory_Lighting,
    .name = "Acme Test",
    .productData = "03d8a775e3644573",
    .manufacturer = "Acme",
    .model = "Test1,1",
    .serialNumber = "099DB48E9E28",
    .firmwareVersion = "1",
    .hardwareVersion = "1",
    .services = (const HAPService* const[]) { &accessoryInformationService,
                                              &hapProtocolInformationService,
                                              &pairingService,
                                              &diagnosticsService,
                                              NULL },
    .callbacks = { .identify = IdentifyAccessoryHelper,
                   .diagnosticsConfig = { .getDiagnosticsConfig = GetAccessoryDiagnosticsConfig } }
};

static void TestSupportedDiagnosticsSnapshot(HAPSession* session, HAPAccessoryServer* server) {

    HAPLog(&kHAPLog_Default, "Test SupportedDiagnosticsSnapshot read handler");

    uint8_t bytes[1024];
    HAPError err;
    HAPTLVWriter responseWriter;
    HAPTLVWriterCreate(&responseWriter, bytes, sizeof bytes);
    const HAPTLV8CharacteristicReadRequest request = { .transportType = kHAPTransportType_IP,
                                                       .session = session,
                                                       .characteristic = &diagnosticsSupportedDiagnosticsSnapshot,
                                                       .service = &diagnosticsService,
                                                       .accessory = &accessory };
    err = request.characteristic->callbacks.handleRead(server, &request, &responseWriter, NULL);
    HAPAssert(!err);
    void* responseBytes;
    size_t numResponseBytes;
    HAPTLVWriterGetBuffer(&responseWriter, &responseBytes, &numResponseBytes);
    HAPTLVReader responseReader;
    HAPTLVReaderCreate(&responseReader, responseBytes, numResponseBytes);

    bool tlvRecv[4] = { false };

    for (;;) {
        HAPTLV tlv;
        bool valid;
        err = HAPTLVReaderGetNext(&responseReader, &valid, &tlv);
        HAPAssert(!err);
        if (!valid) {
            break;
        }

        HAPAssert(tlv.type != 0 && tlv.type <= sizeof tlvRecv / sizeof tlvRecv[0]);
        HAPAssert(!tlvRecv[tlv.type - 1]);
        tlvRecv[tlv.type - 1] = true;

        // Validate TLV data
        switch (tlv.type) {
            case 1: {
                // kHAPCharacteristicValue_SupportedDiagnosticsSnapshot_Format
                HAPAssert(HAPReadUInt8(tlv.value.bytes) == accessoryDiagnosticsConfig.diagnosticsSnapshotFormat);
            } break;
            case 2: {
                // kHAPCharacteristicValue_SupportedDiagnosticsSnapshot_Type
                HAPAssert(HAPReadUInt8(tlv.value.bytes) == accessoryDiagnosticsConfig.diagnosticsSnapshotType);
            } break;
            case 4: {
                // kHAPCharacteristicValue_SupportedDiagnosticsSnapshot_Options
                HAPAssert(HAPReadUInt8(tlv.value.bytes) == accessoryDiagnosticsConfig.diagnosticsSnapshotOptions);
            } break;
        }
    }

    // Validate expected TLV(s) were received
    HAPAssert(tlvRecv[0]);
    HAPAssert(tlvRecv[1]);
    HAPAssert(tlvRecv[3]);
}

int main() {
    HAPError err;
    HAPPlatformCreate();

    // Prepare key-value store.
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    HAPControllerPublicKey controllerPublicKey;
    {
        // Import Device ID.
        HAPAccessoryServerDeviceID expectedDeviceID;
        HAPPlatformRandomNumberFill(expectedDeviceID.bytes, sizeof expectedDeviceID.bytes);
        err = HAPLegacyImportDeviceID(platform.keyValueStore, &expectedDeviceID);
        HAPAssert(!err);

        // Import long-term secret key.
        HAPAccessoryServerLongTermSecretKey expectedLongTermSecretKey;
        HAPPlatformRandomNumberFill(expectedLongTermSecretKey.bytes, sizeof expectedLongTermSecretKey.bytes);
        err = HAPLegacyImportLongTermSecretKey(platform.keyValueStore, &expectedLongTermSecretKey);
        HAPAssert(!err);

        // Import controller pairing.
        HAPControllerPairingIdentifier controllerPairingIdentifier;
        HAPRawBufferZero(&controllerPairingIdentifier, sizeof controllerPairingIdentifier);
        controllerPairingIdentifier.numBytes = sizeof controllerPairingIdentifier.bytes;
        HAPPlatformRandomNumberFill(controllerPairingIdentifier.bytes, sizeof controllerPairingIdentifier.bytes);

        HAPPlatformRandomNumberFill(controllerPublicKey.bytes, sizeof controllerPublicKey.bytes);

        err = HAPLegacyImportControllerPairing(
                platform.keyValueStore,
                controllerPairingID,
                &controllerPairingIdentifier,
                &controllerPublicKey,
                /* isAdmin: */ true);
        HAPAssert(!err);
    }

    // Prepare accessory server storage.
    HAPIPSession ipSessions[kHAPIPSessionStorage_DefaultNumElements];
    uint8_t ipInboundBuffers[HAPArrayCount(ipSessions)][kHAPIPSession_DefaultInboundBufferSize];
    uint8_t ipOutboundBuffers[HAPArrayCount(ipSessions)][kHAPIPSession_DefaultOutboundBufferSize];
    HAPIPEventNotification ipEventNotifications[HAPArrayCount(ipSessions)][kAttributeCount];
    for (size_t i = 0; i < HAPArrayCount(ipSessions); i++) {
        ipSessions[i].inboundBuffer.bytes = ipInboundBuffers[i];
        ipSessions[i].inboundBuffer.numBytes = sizeof ipInboundBuffers[i];
        ipSessions[i].outboundBuffer.bytes = ipOutboundBuffers[i];
        ipSessions[i].outboundBuffer.numBytes = sizeof ipOutboundBuffers[i];
        ipSessions[i].eventNotifications = ipEventNotifications[i];
        ipSessions[i].numEventNotifications = HAPArrayCount(ipEventNotifications[i]);
    }
    HAPIPReadContext ipReadContexts[kAttributeCount];
    HAPIPWriteContext ipWriteContexts[kAttributeCount];
    uint8_t ipScratchBuffer[kHAPIPSession_DefaultScratchBufferSize];
    HAPIPAccessoryServerStorage ipAccessoryServerStorage = { .sessions = ipSessions,
                                                             .numSessions = HAPArrayCount(ipSessions),
                                                             .readContexts = ipReadContexts,
                                                             .numReadContexts = HAPArrayCount(ipReadContexts),
                                                             .writeContexts = ipWriteContexts,
                                                             .numWriteContexts = HAPArrayCount(ipWriteContexts),
                                                             .scratchBuffer = { .bytes = ipScratchBuffer,
                                                                                .numBytes = sizeof ipScratchBuffer } };

    // Initialize accessory server.
    HAPAssert(HAPGetCompatibilityVersion() == HAP_COMPATIBILITY_VERSION);
    HAPAccessoryServer accessoryServer;
    HAPAccessoryServerCreate(&accessoryServer,
        &(const HAPAccessoryServerOptions) {
            .maxPairings = kHAPPairingStorage_MinElements,
            .ip = {
                .transport = &kHAPAccessoryServerTransport_IP,
                .accessoryServerStorage = &ipAccessoryServerStorage,
            },
        },
        &platform,
        &(const HAPAccessoryServerCallbacks) {
            .handleUpdatedState = HAPAccessoryServerHelperHandleUpdatedAccessoryServerState
        },
        NULL);

    // Start accessory server.
    HAPAccessoryServerStart(&accessoryServer, &accessory);
    HAPPlatformClockAdvance(0);
    HAPAssert(HAPAccessoryServerGetState(&accessoryServer) == kHAPAccessoryServerState_Running);

    // Discover IP accessory server.
    HAPAccessoryServerInfo serverInfo;
    HAPNetworkPort serverPort;
    err = HAPDiscoverIPAccessoryServer(HAPNonnull(platform.ip.serviceDiscovery), &serverInfo, &serverPort);
    HAPAssert(!err);
    HAPAssert(!serverInfo.statusFlags.isNotPaired);

    // Create fake security session.
    HAPSession session;
    TestCreateFakeSecuritySession(&session, &accessoryServer, controllerPairingID);

    // Test SupportedDiagnosticsSnapshot characteristic read handler
    TestSupportedDiagnosticsSnapshot(&session, &accessoryServer);

    // Stop accessory server.
    HAPLog(&kHAPLog_Default, "Stopping accessory server.");
    HAPAccessoryServerForceStop(&accessoryServer);
    HAPPlatformClockAdvance(0);
    HAPPlatformClockAdvance(0);
    HAPAssert(HAPAccessoryServerGetState(&accessoryServer) == kHAPAccessoryServerState_Idle);
}
#else
int main() {
    HAPLogInfo(&kHAPLog_Default, "This test is not supported");
    return 0;
}
#endif // HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)
