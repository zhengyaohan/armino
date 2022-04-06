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

#include "HAP+API.h"

#include "HAPPlatform+Init.h"

#include "Harness/HAPAccessoryServerHelper.h"
#include "Harness/HAPPlatformClockHelper.h"
#include "Harness/HAPTestController.c"
#include "Harness/TemplateDB.c"

#if !HAP_FEATURE_ENABLED(HAP_FEATURE_UARP_SUPPORT)
int main() {
    return 0;
}
#else

#define kFirmwareUpdateDuration 60
static HAPAccessoryFirmwareUpdateState accessoryFirmwareUpdateState;

HAP_RESULT_USE_CHECK
HAPError FirmwareUpdateGetAccessoryState(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPAccessory* accessory HAP_UNUSED,
        HAPAccessoryFirmwareUpdateState* accessoryState,
        void* _Nullable context HAP_UNUSED) {
    HAPRawBufferCopyBytes(accessoryState, &accessoryFirmwareUpdateState, sizeof accessoryFirmwareUpdateState);
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
                                              &firmwareUpdateService,
                                              NULL },
    .callbacks = { .identify = IdentifyAccessoryHelper,
                   .firmwareUpdate = { .getAccessoryState = FirmwareUpdateGetAccessoryState } }
};

static void TestFirmwareUpdateReadinessHelper(HAPSession* session, HAPAccessoryServer* server) {

    uint8_t bytes[1024];
    HAPError err;
    HAPTLVWriter responseWriter;
    HAPTLVWriterCreate(&responseWriter, bytes, sizeof bytes);
    const HAPTLV8CharacteristicReadRequest request = { .transportType = kHAPTransportType_IP,
                                                       .session = session,
                                                       .characteristic = &firmwareUpdateReadinessCharacteristic,
                                                       .service = &firmwareUpdateService,
                                                       .accessory = &accessory };
    err = request.characteristic->callbacks.handleRead(server, &request, &responseWriter, NULL);
    HAPAssert(!err);
    void* responseBytes;
    size_t numResponseBytes;
    HAPTLVWriterGetBuffer(&responseWriter, &responseBytes, &numResponseBytes);
    HAPTLVReader responseReader;
    HAPTLVReaderCreate(&responseReader, responseBytes, numResponseBytes);

    bool tlvRecv[2] = { false };

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
            case kHAPCharacteristicTLVType_FirmwareUpdateReadiness_StagingNotReadyReason: {
                HAPAssert(HAPReadLittleUInt32(tlv.value.bytes) == accessoryFirmwareUpdateState.stagingNotReadyReason);
                break;
            }
            case kHAPCharacteristicTLVType_FirmwareUpdateReadiness_UpdateNotReadyReason: {
                HAPAssert(HAPReadLittleUInt32(tlv.value.bytes) == accessoryFirmwareUpdateState.updateNotReadyReason);
                break;
            }
        }
    }

    // Validate expected TLV(s) were received
    if (accessoryFirmwareUpdateState.stagingNotReadyReason) {
        HAPAssert(tlvRecv[0]);
    } else {
        HAPAssert(!tlvRecv[0]);
    }
    if (accessoryFirmwareUpdateState.updateNotReadyReason) {
        HAPAssert(tlvRecv[1]);
    } else {
        HAPAssert(!tlvRecv[1]);
    }
}

static void TestFirmwareUpdateReadiness(HAPSession* session, HAPAccessoryServer* server) {

    HAPLog(&kHAPLog_Default, "Test FirmwareUpdateReadiness read handler");

    // Staging cleared, update cleared
    HAPRawBufferZero(&accessoryFirmwareUpdateState, sizeof accessoryFirmwareUpdateState);
    TestFirmwareUpdateReadinessHelper(session, server);

    // Staging set, update cleared
    HAPRawBufferZero(&accessoryFirmwareUpdateState, sizeof accessoryFirmwareUpdateState);
    accessoryFirmwareUpdateState.stagingNotReadyReason =
            kHAPCharacteristicValue_FirmwareUpdateReadiness_StagingNotReady_LowBattery;
    TestFirmwareUpdateReadinessHelper(session, server);

    // Staging cleared, update set
    HAPRawBufferZero(&accessoryFirmwareUpdateState, sizeof accessoryFirmwareUpdateState);
    accessoryFirmwareUpdateState.updateNotReadyReason =
            kHAPCharacteristicValue_FirmwareUpdateReadiness_UpdateNotReady_StagedUnavailable;
    TestFirmwareUpdateReadinessHelper(session, server);

    // Staging set, update set
    HAPRawBufferZero(&accessoryFirmwareUpdateState, sizeof accessoryFirmwareUpdateState);
    accessoryFirmwareUpdateState.stagingNotReadyReason =
            kHAPCharacteristicValue_FirmwareUpdateReadiness_StagingNotReady_Connectivity;
    accessoryFirmwareUpdateState.updateNotReadyReason =
            kHAPCharacteristicValue_FirmwareUpdateReadiness_UpdateNotReady_CriticalOperation;
    TestFirmwareUpdateReadinessHelper(session, server);
}

static void TestFirmwareUpdateStatusHelper(HAPSession* session, HAPAccessoryServer* server) {

    accessoryFirmwareUpdateState.updateDuration = kFirmwareUpdateDuration;

    uint8_t bytes[1024];
    HAPError err;
    HAPTLVWriter responseWriter;
    HAPTLVWriterCreate(&responseWriter, bytes, sizeof bytes);
    const HAPTLV8CharacteristicReadRequest request = { .transportType = kHAPTransportType_IP,
                                                       .session = session,
                                                       .characteristic = &firmwareUpdateStatusCharacteristic,
                                                       .service = &firmwareUpdateService,
                                                       .accessory = &accessory };
    err = request.characteristic->callbacks.handleRead(server, &request, &responseWriter, NULL);
    HAPAssert(!err);
    void* responseBytes;
    size_t numResponseBytes;
    HAPTLVWriterGetBuffer(&responseWriter, &responseBytes, &numResponseBytes);
    HAPTLVReader responseReader;
    HAPTLVReaderCreate(&responseReader, responseBytes, numResponseBytes);

    bool tlvRecv[3] = { false };

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
            case kHAPCharacteristicTLVType_FirmwareUpdateStatus_FirmwareUpdateState: {
                HAPAssert(HAPReadLittleUInt16(tlv.value.bytes) == accessoryFirmwareUpdateState.updateState);
                break;
            }
            case kHAPCharacteristicTLVType_FirmwareUpdateStatus_UpdateDuration: {
                HAPAssert(HAPReadLittleUInt16(tlv.value.bytes) == accessoryFirmwareUpdateState.updateDuration);
                break;
            }
            case kHAPCharacteristicTLVType_FirmwareUpdateStatus_StagedFirmwareVersion: {
                HAPAssert(HAPStringAreEqual(tlv.value.bytes, accessoryFirmwareUpdateState.stagedFirmwareVersion));
                break;
            }
        }
    }

    // Validate expected TLV(s) were received
    HAPAssert(tlvRecv[0]);
    HAPAssert(tlvRecv[1]);
    if (accessoryFirmwareUpdateState.stagedFirmwareVersion) {
        HAPAssert(tlvRecv[2]);
    } else {
        HAPAssert(!tlvRecv[2]);
    }
}

static void TestFirmwareUpdateStatus(HAPSession* session, HAPAccessoryServer* server) {

    HAPLog(&kHAPLog_Default, "Test FirmwareUpdateStatus read handler");

    // Staged firmware version cleared
    HAPRawBufferZero(&accessoryFirmwareUpdateState, sizeof accessoryFirmwareUpdateState);
    accessoryFirmwareUpdateState.updateState = kHAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState_Idle;
    accessoryFirmwareUpdateState.stagedFirmwareVersion = NULL;
    TestFirmwareUpdateStatusHelper(session, server);

    // Staged firmware version set
    HAPRawBufferZero(&accessoryFirmwareUpdateState, sizeof accessoryFirmwareUpdateState);
    accessoryFirmwareUpdateState.updateState =
            kHAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState_StagingSucceeded;
    accessoryFirmwareUpdateState.stagedFirmwareVersion = "1.2.3";
    TestFirmwareUpdateStatusHelper(session, server);
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
    HAPSession session =
            (HAPSession) { .server = &accessoryServer, .hap = { .active = true, .pairingID = controllerPairingID } };
    HAPPlatformRandomNumberFill(session.state.pairVerify.cv_SK, sizeof(session.state.pairVerify.cv_SK));
    HAP_X25519_scalarmult_base(session.state.pairVerify.Controller_cv_PK, session.state.pairVerify.cv_SK);
    HAPPlatformRandomNumberFill(session.state.pairVerify.cv_SK, sizeof(session.state.pairVerify.cv_SK));
    HAP_X25519_scalarmult_base(session.state.pairVerify.cv_PK, session.state.pairVerify.cv_SK);
    HAP_X25519_scalarmult(
            session.state.pairVerify.cv_KEY, session.state.pairVerify.cv_SK, session.state.pairVerify.Controller_cv_PK);
    HAPRawBufferCopyBytes(session.hap.cv_KEY, session.state.pairVerify.cv_KEY, sizeof(session.hap.cv_KEY));
    HAPRawBufferZero(&session.state.pairVerify, sizeof(session.state.pairVerify));

    // Test FirmwareUpdateReadiness characteristic read handler
    TestFirmwareUpdateReadiness(&session, &accessoryServer);

    // Test FirmwareUpdateStatus characteristic read handler
    TestFirmwareUpdateStatus(&session, &accessoryServer);
    return 0;
}

#endif
