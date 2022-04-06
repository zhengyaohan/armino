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

#include "HAPCharacteristic.h"
#include "HAPLog+Attributes.h"

#include "HAPPlatform+Init.h"
#include "HAPPlatformSetup+Init.h"

#include "Harness/HAPAccessoryServerHelper.h"
#include "Harness/HAPPlatformClockHelper.h"
#include "Harness/HAPTestController.c"
#include "Harness/SecurityTestHelpers.h"
#include "Harness/TemplateDB.c"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
static HAPTransportType transportType = kHAPTransportType_IP;
#else
static HAPTransportType transportType = kHAPTransportType_BLE;
#endif

static AdaptiveLightTransitionStorage adaptiveLightStorage;

static void TestCharacteristicValueUpdate(uint64_t characteristicID, int32_t value, bool sendNotification) {
    if (characteristicID == kIID_LightBulbBrightness) {
        HAPLogInfo(&kHAPLog_Default, "Brightness changed to %d", (int) value);
    }
    if (characteristicID == kIID_LightBulbColorTemp) {
        HAPLogInfo(&kHAPLog_Default, "Color Temp changed to %d", (int) value);
    }
}

static void TestCharacteristicValueRequest(uint64_t characteristicID, int32_t* value) {
    if (characteristicID == kIID_LightBulbBrightness) {
        // ignore for test
    }
    if (characteristicID == kIID_LightBulbColorTemp) {
        // ignore for test
    }
}

static void TestTransitionExpiry() {
    // ignore for test
}

HAP_UNUSED static void TestInitializeAdaptiveLight() {

    // ensure brightness and color temp are supported
    AdaptiveLightSupportedTransition supportedTransitions[] = {
        { kIID_LightBulbBrightness,
          kHAPCharacteristicValue_SupportedTransitionConfiguration_SupportedTransition_Type_Linear },
        { kIID_LightBulbColorTemp,
          kHAPCharacteristicValue_SupportedTransitionConfiguration_SupportedTransition_Type_LinearDerived }
    };
    AdaptiveLightCallbacks callbacks = {
        .handleCharacteristicValueUpdate = TestCharacteristicValueUpdate,
        .handleTransitionExpiry = TestTransitionExpiry,
        .handleCharacteristicValueRequest = TestCharacteristicValueRequest,
    };

    HAPError err = InitializeAdaptiveLightParameters(
            platform.keyValueStore,
            &adaptiveLightStorage,
            supportedTransitions,
            HAPArrayCount(supportedTransitions),
            &callbacks);
    if (err) {
        HAPLogError(&kHAPLog_Default, "Failed to initialize Adaptive Light parameters");
    }
}

static const HAPAccessory accessory = { .aid = 1,
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
                                                                                  &lightBulbService,
                                                                                  NULL },
                                        .callbacks = { .identify = IdentifyAccessoryHelper } };

HAP_UNUSED static void TestSupportedTransitionControlConfiguration(HAPSession* session, HAPAccessoryServer* server) {

    HAPLog(&kHAPLog_Default, "Test SupportedTransitionConfiguration read handler");

    uint8_t bytes[2048];
    HAPError err;
    HAPTLVWriter responseWriter;
    HAPTLVWriterCreate(&responseWriter, bytes, sizeof bytes);
    const HAPTLV8CharacteristicReadRequest request = { .transportType = transportType,
                                                       .session = session,
                                                       .characteristic =
                                                               &lightBulbSupportedTransitionConfigurationCharacteristic,
                                                       .service = &lightBulbService,
                                                       .accessory = &accessory };
    err = request.characteristic->callbacks.handleRead(server, &request, &responseWriter, NULL);
    HAPAssert(!err);
    {
        void* bytes;
        size_t numBytes;
        HAPTLVWriterGetBuffer(&responseWriter, &bytes, &numBytes);
        HAPLogCharacteristicBufferDebug(
                &kHAPLog_Default, request.characteristic, request.service, request.accessory, bytes, numBytes, ">");
    }
}

HAP_UNUSED static HAPError
        TestTransitionControlRead(HAPSession* session, HAPAccessoryServer* server, size_t* numBytes) {

    HAPLog(&kHAPLog_Default, "Test TransitionControlState");

    uint8_t bytes[2048];
    size_t localNumBytes;
    HAPError err;
    HAPTLVWriter responseWriter;
    HAPTLVWriterCreate(&responseWriter, bytes, sizeof bytes);
    const HAPTLV8CharacteristicReadRequest request = { .transportType = transportType,
                                                       .session = session,
                                                       .characteristic = &lightBulbTransitionControlCharacteristic,
                                                       .service = &lightBulbService,
                                                       .accessory = &accessory };
    err = request.characteristic->callbacks.handleRead(server, &request, &responseWriter, NULL);

    {
        void* bytes;
        HAPTLVWriterGetBuffer(&responseWriter, &bytes, &localNumBytes);
        HAPLogCharacteristicBufferDebug(
                &kHAPLog_Default,
                request.characteristic,
                request.service,
                request.accessory,
                bytes,
                localNumBytes,
                ">");
    }

    if (numBytes) {
        *numBytes = localNumBytes;
    }
    return err;
}

HAP_UNUSED static HAPError TestTransitionControlFetch(HAPSession* session, HAPAccessoryServer* server, uint64_t id) {

    HAPLog(&kHAPLog_Default, "Test TransitionControl Fetch");

    HAPError err;
    uint8_t requestBuffer[2048];
    HAPTLVWriter requestWriter;
    HAPTLVWriterCreate(&requestWriter, requestBuffer, sizeof requestBuffer);
    const HAPTLV8CharacteristicWriteRequest request = { .transportType = transportType,
                                                        .session = session,
                                                        .characteristic = &lightBulbTransitionControlCharacteristic,
                                                        .service = &lightBulbService,
                                                        .accessory = &accessory };
    HAPCharacteristicValue_TransitionControl transition;
    HAPRawBufferZero(&transition, sizeof transition);
    transition.fetch.HAPInstanceID.bytes = &id;
    transition.fetch.HAPInstanceID.numBytes = sizeof id;
    transition.fetchIsSet = true;

    err = HAPTLVWriterEncode(&requestWriter, &kHAPCharacteristicTLVFormat_TransitionControl, &transition);

    void* requestBytes;
    size_t numRequestBytes;
    HAPTLVWriterGetBuffer(&requestWriter, &requestBytes, &numRequestBytes);

    HAPTLVReader requestReader;
    HAPTLVReaderCreate(&requestReader, requestBytes, numRequestBytes);
    err = request.characteristic->callbacks.handleWrite(server, &request, &requestReader, NULL);
    return err;
}

HAP_UNUSED static HAPError
        TransitionControlWrite(HAPSession* session, HAPAccessoryServer* server, uint8_t* bytes, size_t numBytes) {
    HAPPrecondition(bytes);
    HAPPrecondition(session);
    HAPPrecondition(server);

    HAPError err;
    const HAPTLV8CharacteristicWriteRequest request = { .transportType = transportType,
                                                        .session = session,
                                                        .characteristic = &lightBulbTransitionControlCharacteristic,
                                                        .service = &lightBulbService,
                                                        .accessory = &accessory };

    HAPTLVReader requestReader;
    HAPTLVReaderCreate(&requestReader, bytes, numBytes);
    err = request.characteristic->callbacks.handleWrite(server, &request, &requestReader, NULL);
    return err;
}

HAP_UNUSED static void
        TestTransitionControlWrite(HAPSession* session, HAPAccessoryServer* server, uint8_t* bytes, size_t numBytes) {
    HAPPrecondition(bytes);
    HAPPrecondition(session);
    HAPPrecondition(server);

    HAPError err;
    err = TransitionControlWrite(session, server, bytes, numBytes);
    HAPAssert(!err);
}

HAP_UNUSED static void TestTransitionControlWriteFailure(
        HAPSession* session,
        HAPAccessoryServer* server,
        uint8_t* bytes,
        size_t numBytes) {
    HAPPrecondition(bytes);
    HAPPrecondition(session);
    HAPPrecondition(server);

    HAPError err;
    err = TransitionControlWrite(session, server, bytes, numBytes);
    HAPAssert(err);
}

HAP_UNUSED void print(const void* requestBytes, size_t numRequestBytes) {
    const uint8_t* b = (const uint8_t*) requestBytes;
    for (size_t i = 0; i < numRequestBytes; i++) {
        HAPLogInfo(&kHAPLog_Default, "%02X", b[i]);
    }
}

/*
 * Test TLVs for Variable length Integers
 */
HAP_ENUM_BEGIN(uint8_t, TestCharacteristicTLVType_VariableLengthInteger) {
    kTestCharacteristicTLVType_VariableLengthInteger4 = 1,
    kTestCharacteristicTLVType_VariableLengthInteger8 = 2,
} HAP_ENUM_END(uint8_t, TestCharacteristicTLVType_VariableLengthInteger);

typedef struct {
    HAPDataTLVValue val4;
    bool val4IsSet;
    HAPDataTLVValue val8;
    bool val8IsSet;
} TestVariableLengthInteger;

HAP_STRUCT_TLV_SUPPORT(TestVariableLengthInteger, HAPCharacteristicTLVFormat_TestVariableLengthInteger)

const HAPCharacteristicTLVFormat_TestVariableLengthInteger kHAPCharacteristicTLVFormat_TestVariableLengthInteger;

const HAPStructTLVMember* const kHAPCharacteristicTLVMembers_VariableLengthInteger[] = {
    &(const HAPStructTLVMember) { .valueOffset = HAP_OFFSETOF(TestVariableLengthInteger, val4),
                                  .isSetOffset = HAP_OFFSETOF(TestVariableLengthInteger, val4IsSet),
                                  .tlvType = kTestCharacteristicTLVType_VariableLengthInteger4,
                                  .debugDescription = "Test Variable Length Int 4",
                                  .format = &kHAPCharacteristicTLVFormat_VariableLengthInteger4,
                                  .isOptional = true },
    &(const HAPStructTLVMember) { .valueOffset = HAP_OFFSETOF(TestVariableLengthInteger, val8),
                                  .isSetOffset = HAP_OFFSETOF(TestVariableLengthInteger, val8IsSet),
                                  .tlvType = kTestCharacteristicTLVType_VariableLengthInteger8,
                                  .debugDescription = "Test Variable Length Int 8",
                                  .format = &kHAPCharacteristicTLVFormat_VariableLengthInteger8,
                                  .isOptional = true },
    NULL
};

const HAPCharacteristicTLVFormat_TestVariableLengthInteger kHAPCharacteristicTLVFormat_TestVariableLengthInteger = {
    .type = kHAPTLVFormatType_Struct,
    .members = kHAPCharacteristicTLVMembers_VariableLengthInteger
};

HAP_UNUSED static void TestVariableIntLength4Encode(uint8_t* bytes, size_t numBytes) {

    HAPLog(&kHAPLog_Default, "Test Variable Length 4 encoding");

    HAPError err;
    uint8_t requestBuffer[1024];
    HAPTLVWriter requestWriter;
    HAPTLVWriterCreate(&requestWriter, requestBuffer, sizeof requestBuffer);
    TestVariableLengthInteger data;
    HAPRawBufferZero(&data, sizeof data);
    data.val4.numBytes = numBytes;
    data.val4.bytes = bytes;
    data.val4IsSet = true;

    err = HAPTLVWriterEncode(&requestWriter, &kHAPCharacteristicTLVFormat_TestVariableLengthInteger, &data);
    HAPAssert(numBytes > 4 ? err : !err);
}

HAP_UNUSED static uint32_t TestVariableIntLength4Decode(uint8_t* bytes, size_t numBytes) {

    HAPLog(&kHAPLog_Default, "Test Variable Length 4 decoding");

    HAPError err;
    HAPTLVReader requestReader;
    HAPTLVReaderCreate(&requestReader, bytes, numBytes);
    TestVariableLengthInteger data;
    HAPRawBufferZero(&data, sizeof data);
    err = HAPTLVReaderDecode(&requestReader, &kHAPCharacteristicTLVFormat_TestVariableLengthInteger, &data);
    HAPAssert(!err);
    HAPAssert(data.val4IsSet);
    HAPAssert(data.val4.numBytes <= 4);
    HAPAssert(data.val4.bytes != NULL);

    uint32_t res = 0;
    const uint8_t* v = data.val4.bytes;
    for (size_t i = 0; i < data.val4.numBytes; i++) {
        res += v[i] << (8 * i);
    }
    return res;
}

HAP_UNUSED static void TestVariableIntLength8Encode(uint8_t* bytes, size_t numBytes) {

    HAPLog(&kHAPLog_Default, "Test Variable Length 8 encoding");

    HAPError err;
    uint8_t requestBuffer[1024];
    HAPTLVWriter requestWriter;
    HAPTLVWriterCreate(&requestWriter, requestBuffer, sizeof requestBuffer);
    TestVariableLengthInteger data;
    HAPRawBufferZero(&data, sizeof data);
    data.val8.numBytes = numBytes;
    data.val8.bytes = bytes;
    data.val8IsSet = true;

    err = HAPTLVWriterEncode(&requestWriter, &kHAPCharacteristicTLVFormat_TestVariableLengthInteger, &data);
    HAPAssert(numBytes > 8 ? err : !err);
}

HAP_UNUSED static uint32_t TestVariableIntLength8Decode(uint8_t* bytes, size_t numBytes) {

    HAPLog(&kHAPLog_Default, "Test Variable Length 4 decoding");

    HAPError err;
    HAPTLVReader requestReader;
    HAPTLVReaderCreate(&requestReader, bytes, numBytes);
    TestVariableLengthInteger data;
    HAPRawBufferZero(&data, sizeof data);
    err = HAPTLVReaderDecode(&requestReader, &kHAPCharacteristicTLVFormat_TestVariableLengthInteger, &data);
    HAPAssert(!err);
    HAPAssert(data.val8IsSet);
    HAPAssert(data.val8.numBytes <= 4);
    HAPAssert(data.val8.bytes != NULL);

    uint32_t res = 0;
    const uint8_t* v = data.val8.bytes;
    for (size_t i = 0; i < data.val8.numBytes; i++) {
        res += v[i] << (8 * i);
    }
    return res;
}

HAP_UNUSED void TestPersistentState(void) {

    AdaptiveLightTransitionStorage tempAdaptiveLightStorage;
    HAPRawBufferCopyBytes(&tempAdaptiveLightStorage, &adaptiveLightStorage, sizeof(AdaptiveLightTransitionStorage));
    adaptiveLightStorage.numTransitions = 0;
    for (size_t i = 0; i < kLightbulb_MaxSupportedTransitions; i++) {
        HAPRawBufferZero(&adaptiveLightStorage.transitions[i], sizeof(adaptiveLightStorage.transitions[i]));
    }
    adaptiveLightStorage.numTransitionPoints = 0;
    for (size_t i = 0; i < kLightbulb_MaxSupportedTransitionPoints; i++) {
        HAPRawBufferZero(&adaptiveLightStorage.transitionPoints[i], sizeof(adaptiveLightStorage.transitionPoints[i]));
    }
    LoadTransitionsFromPersistentMemory();

    HAPAssert(adaptiveLightStorage.numTransitions == tempAdaptiveLightStorage.numTransitions);
    for (size_t i = 0; i < kLightbulb_MaxSupportedTransitions; i++) {
        AdaptiveLightTransition* ref = &(tempAdaptiveLightStorage.transitions[i]);
        AdaptiveLightTransition* new = &(adaptiveLightStorage.transitions[i]);
        HAPLogInfo(&kHAPLog_Default, "comparing transition #%zu", i);
        HAPAssert(ref->characteristicID == new->characteristicID);
        HAPAssert(ref->serviceID == new->serviceID);
        HAPAssert(ref->transitionType == new->transitionType);
        HAPAssert(ref->controllerContext.numBytes == new->controllerContext.numBytes);
        HAPAssert(HAPRawBufferAreEqual(
                ref->controllerContext.bytes,
                new->controllerContext.bytes,
                HAPMin(ref->controllerContext.numBytes, new->controllerContext.numBytes)));
        HAPAssert(ref->sourceCharacteristicID == new->sourceCharacteristicID);
        HAPAssert(ref->lowerValue == new->lowerValue);
        HAPAssert(ref->upperValue == new->upperValue);
        HAPAssert(ref->upperValueBufferLen == new->upperValueBufferLen);
        HAPAssert(ref->numPoints == new->numPoints);
        HAPAssert(ref->threshold == new->threshold);
        HAPAssert(ref->startCondition == new->startCondition);
        HAPAssert(ref->endBehavior == new->endBehavior);
        HAPAssert(ref->requestTime == new->requestTime);
        HAPAssert(ref->updateInterval == new->updateInterval);
        HAPAssert(ref->timeIntervalThresholdForNotification == new->timeIntervalThresholdForNotification);
        HAPAssert(ref->valueChangeThresholdForNotification == new->valueChangeThresholdForNotification);
    }
    HAPAssert(adaptiveLightStorage.numTransitionPoints == tempAdaptiveLightStorage.numTransitionPoints);
    for (size_t i = 0; i < kLightbulb_MaxSupportedTransitionPoints; i++) {
        AdaptiveLightTransitionPoint* ref = &(tempAdaptiveLightStorage.transitionPoints[i]);
        AdaptiveLightTransitionPoint* new = &(adaptiveLightStorage.transitionPoints[i]);
        HAPLogInfo(&kHAPLog_Default, "comparing transition point #%zu", i);
        HAPAssert(ref->scale == new->scale);
        HAPAssert(ref->offset == new->offset);
        HAPAssert(ref->target == new->target);
        HAPAssert(ref->targetBufferLen == new->targetBufferLen);
        HAPAssert(ref->rate == new->rate);
        HAPAssert(ref->startDelayDuration == new->startDelayDuration);
        HAPAssert(ref->completionDuration == new->completionDuration);
        HAPAssert(ref->isUsed == new->isUsed);
    }
}

HAP_UNUSED void PurgeAdaptiveLightKVS(void) {
    HAPError err;
    err = HAPPlatformKeyValueStorePurgeDomain(platform.keyValueStore, kAppKeyValueStoreDomain_AdaptiveLight);
    if (err) {
        HAPLogInfo(&kHAPLog_Default, "Unable to purge key value store.");
    }
}

int main() {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
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

#else

    HAPError err;
    HAPPlatformCreate();
    HAPPlatformSetupDrivers();

    // Prepare accessory server storage.
    static HAPBLEGATTTableElement gattTableElements[kAttributeCount];
    static HAPPairingBLESessionCacheEntry sessionCacheElements[kHAPBLESessionCache_MinElements];
    static HAPSession session;
    static uint8_t procedureBytes[2048];
    static HAPBLEProcedure procedures[1];
    static HAPBLEAccessoryServerStorage bleAccessoryServerStorage = {
        .gattTableElements = gattTableElements,
        .numGATTTableElements = HAPArrayCount(gattTableElements),
        .sessionCacheElements = sessionCacheElements,
        .numSessionCacheElements = HAPArrayCount(sessionCacheElements),
        .session = &session,
        .procedures = procedures,
        .numProcedures = HAPArrayCount(procedures),
        .procedureBuffer = { .bytes = procedureBytes, .numBytes = sizeof procedureBytes },
    };

    // Initialize accessory server.
    static HAPAccessoryServer accessoryServer;
    HAPAccessoryServerCreate(
            &accessoryServer,
            &(const HAPAccessoryServerOptions) {
                    .maxPairings = kHAPPairingStorage_MinElements,
                    .ble = { .transport = &kHAPAccessoryServerTransport_BLE,
                             .accessoryServerStorage = &bleAccessoryServerStorage,
                             .preferredAdvertisingInterval = kHAPBLEAdvertisingInterval_Minimum,
                             .preferredNotificationDuration = kHAPBLENotification_MinDuration } },
            &platform,
            &(const HAPAccessoryServerCallbacks) { .handleUpdatedState =
                                                           HAPAccessoryServerHelperHandleUpdatedAccessoryServerState },
            /* context: */ NULL);

    // Start accessory server.
    HAPAccessoryServerStart(&accessoryServer, &accessory);
    HAPPlatformClockAdvance(0);
    HAPAssert(HAPAccessoryServerGetState(&accessoryServer) == kHAPAccessoryServerState_Running);

    // Discover BLE accessory server.
    HAPAccessoryServerInfo serverInfo;
    HAPPlatformBLEPeripheralManagerDeviceAddress deviceAddress;
    err = HAPDiscoverBLEAccessoryServer(HAPNonnull(platform.ble.blePeripheralManager), &serverInfo, &deviceAddress);
    HAPAssert(!err);
    HAPAssert(serverInfo.statusFlags.isNotPaired);

#endif
    {
        uint8_t val[] = { 0, 1, 2, 3 };
        TestVariableIntLength4Encode(val, HAPArrayCount(val));
    }

    {
        uint8_t val[] = {};
        TestVariableIntLength4Encode(val, HAPArrayCount(val));
    }

    {
        uint8_t val[] = { 0, 1, 2, 3, 4 };
        TestVariableIntLength8Encode(val, HAPArrayCount(val));
    }

    {
        uint8_t val[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
        TestVariableIntLength8Encode(val, HAPArrayCount(val));
    }

    {
        uint8_t tlv[] = { 0x01, 0x01, 0x01 };
        HAPAssert(TestVariableIntLength4Decode(tlv, HAPArrayCount(tlv)) == 1);
    }

    {
        uint8_t tlv[] = { 0x01, 0x04, 0x01, 0x02, 0x03, 0x04 };
        HAPAssert(TestVariableIntLength4Decode(tlv, HAPArrayCount(tlv)) != 0x01020304);
    }

    {
        uint8_t tlv[] = { 0x02, 0x01, 0x01 };
        HAPAssert(TestVariableIntLength8Decode(tlv, HAPArrayCount(tlv)) == 1);
    }

    {
        uint8_t tlv[] = { 0x02, 0x04, 0x01, 0x02, 0x03, 0x04 };
        HAPAssert(TestVariableIntLength8Decode(tlv, HAPArrayCount(tlv)) != 0x01020304);
    }

    // Initialize Adaptive Light Storage
    TestInitializeAdaptiveLight();

    // Query supported configurations
    TestSupportedTransitionControlConfiguration(&session, &accessoryServer);

    {
        PurgeAdaptiveLightKVS();
        HAPLog(&kHAPLog_Default, "Linear transition with invalid end behavior and two transition points");
        // Linear transition with invalid end behavior  and two transition points
        // Note: Invalid end behavior means the enum value for the TLV  "End Behavior" does not equal the values for "no
        // change" or "loop"
        // @see HomeKit Accessory Protocol Specification R17
        //      Table 10‐96: Characteristic Value Transition Start TLV8 Definition
        uint8_t requestBytes[] = { 0x02, 0x24, 0x01, 0x22, 0x01, 0x01, 0xD4, 0x03, 0x01, 0x02, 0x04, 0x1A, 0x01,
                                   0x08, 0x01, 0x03, 0x11, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x01, 0x09,
                                   0x01, 0x03, 0x20, 0x00, 0x00, 0x02, 0x02, 0xA0, 0x0F, 0x02, 0x01, 0x00 };
        TestTransitionControlWriteFailure(&session, &accessoryServer, requestBytes, sizeof requestBytes);
    }

    {
        PurgeAdaptiveLightKVS();
        HAPLog(&kHAPLog_Default, "Linear transition with invalid start condition and two transition points");
        // Linear transition with invalid start condition and two transition points
        // Note: Invalid start condition means the enum value for the TLV  "Start Condition" does not equal the values
        // for "None", "TransitionValueAscends", or "TransitionValueDescends"
        // @see HomeKit Accessory Protocol Specification R17
        //      Table 10‐98: Characteristic Value Linear Transition TLV8 Definition
        uint8_t requestBytes[] = { 0x02, 0x24, 0x01, 0x22, 0x01, 0x01, 0xD4, 0x03, 0x01, 0x01, 0x04, 0x1A, 0x01,
                                   0x08, 0x01, 0x03, 0x11, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x01, 0x09,
                                   0x01, 0x03, 0x20, 0x00, 0x00, 0x02, 0x02, 0xA0, 0x0F, 0x02, 0x01, 0x03 };
        TestTransitionControlWriteFailure(&session, &accessoryServer, requestBytes, sizeof requestBytes);
    }

    {
        PurgeAdaptiveLightKVS();
        HAPLog(&kHAPLog_Default, "Linear transition with loop and two transition points");
        // Linear transition with loop and two transition points
        uint8_t requestBytes[] = { 0x02, 0x24, 0x01, 0x22, 0x01, 0x01, 0xD4, 0x03, 0x01, 0x01, 0x04, 0x1A, 0x01,
                                   0x08, 0x01, 0x03, 0x11, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x01, 0x09,
                                   0x01, 0x03, 0x20, 0x00, 0x00, 0x02, 0x02, 0xA0, 0x0F, 0x02, 0x01, 0x00 };
        TestTransitionControlWrite(&session, &accessoryServer, requestBytes, sizeof requestBytes);
    }
    // Read current transition state
    HAPAssert(TestTransitionControlRead(&session, &accessoryServer, NULL) == kHAPError_None);

    // Issue fetch command for Brightness
    HAPAssert(TestTransitionControlFetch(&session, &accessoryServer, kIID_LightBulbBrightness) == kHAPError_None);

    // Read transition
    HAPAssert(TestTransitionControlRead(&session, &accessoryServer, NULL) == kHAPError_None);

    {
        PurgeAdaptiveLightKVS();
        HAPLog(&kHAPLog_Default, "Reset transition for Brightness");
        // Reset transition for Brightness
        uint8_t requestBytes[] = { 0x02, 0x05, 0x01, 0x03, 0x01, 0x01, 0xD4 };
        TestTransitionControlWrite(&session, &accessoryServer, requestBytes, sizeof requestBytes);

        // Issue fetch command for Brightness
        HAPAssert(TestTransitionControlFetch(&session, &accessoryServer, kIID_LightBulbBrightness) == kHAPError_None);

        TestPersistentState();

        // Read transition.
        size_t numBytes;
        HAPAssert(TestTransitionControlRead(&session, &accessoryServer, &numBytes) == kHAPError_None);
        HAPAssert(numBytes == 0);
    }

    {
        PurgeAdaptiveLightKVS();
        HAPLog(&kHAPLog_Default, "Write transition for Brightness");
        // Write transition for Brightness
        uint8_t requestBytes[] = { 0x02, 0x5A, 0x01, 0x58, 0x01, 0x08, 0xD4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x02, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x01, 0x01, 0x04,
                                   0x3f, 0x01, 0x1a, 0x01, 0x04, 0x11, 0x00, 0x00, 0x00, 0x02, 0x08, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x01, 0x1a, 0x01, 0x04, 0x20, 0x00, 0x00, 0x00, 0x02, 0x08, 0xa0,
                                   0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x00 };
        TestTransitionControlWrite(&session, &accessoryServer, requestBytes, sizeof requestBytes);

        TestPersistentState();
        // Issue fetch command for Brightness
        HAPAssert(TestTransitionControlFetch(&session, &accessoryServer, kIID_LightBulbBrightness) == kHAPError_None);

        // Read transition
        HAPAssert(TestTransitionControlRead(&session, &accessoryServer, NULL) == kHAPError_None);

        // Remove transition for Brightness
        RemoveTransition(kIID_LightBulbBrightness);
    }

    {
        PurgeAdaptiveLightKVS();
        HAPLog(&kHAPLog_Default, "Test linear Derived transition with loop and 51 transition points");
        // Linear Derived transition with loop and 51 transition points
        uint8_t requestBytes[] = {
            0x02, 0xff, 0x01, 0xff, 0x01, 0x01, 0xD7, 0x02, 0x1c, 0x01, 0x10, 0xc4, 0xf3, 0x3d, 0xa9, 0x63, 0xc9, 0x4c,
            0x0f, 0x8b, 0x89, 0x6c, 0x36, 0xa3, 0x3e, 0xef, 0x9e, 0x02, 0x08, 0xe5, 0x1a, 0xde, 0x1e, 0x8d, 0x00, 0x00,
            0x00, 0x03, 0x01, 0x01, 0x05, 0xff, 0x01, 0x0f, 0x01, 0x04, 0x50, 0xfa, 0xe4, 0xbf, 0x02, 0x04, 0xc7, 0xf1,
            0xf0, 0x43, 0x03, 0x01, 0x00, 0x00, 0x00, 0x01, 0x12, 0x01, 0x04, 0xab, 0xaa, 0xea, 0xbf, 0x02, 0x04, 0xab,
            0xaa, 0xf3, 0x43, 0x03, 0x04, 0x38, 0x2c, 0x05, 0x00, 0x00, 0x00, 0x01, 0x12, 0x01, 0x04, 0x00, 0x00, 0x00,
            0xc0, 0x02, 0x04, 0x00, 0x00, 0xfe, 0x43, 0x03, 0x04, 0x40, 0x77, 0x1b, 0x00, 0x00, 0x00, 0x01, 0x12, 0x01,
            0x04, 0x5b, 0xb0, 0x05, 0xc0, 0x02, 0x04, 0xe4, 0x38, 0x02, 0x44, 0x03, 0x04, 0x40, 0x77, 0x1b, 0x00, 0x00,
            0x00, 0x01, 0x12, 0x01, 0x04, 0x22, 0x22, 0x02, 0xc0, 0x02, 0x04, 0x55, 0x15, 0x02, 0x44, 0x03, 0x04, 0x40,
            0x77, 0x1b, 0x00, 0x00, 0x00, 0x01, 0x12, 0x01, 0x04, 0xd2, 0x27, 0xfd, 0xbf, 0x02, 0x04, 0xc7, 0xf1, 0x01,
            0x44, 0x03, 0x04, 0x40, 0x77, 0x1b, 0x00, 0x00, 0x00, 0x01, 0x12, 0x01, 0x04, 0x06, 0x5b, 0xf0, 0xbf, 0x02,
            0x04, 0xc7, 0xf1, 0x00, 0x44, 0x03, 0x04, 0x40, 0x77, 0x1b, 0x00, 0x00, 0x00, 0x01, 0x12, 0x01, 0x04, 0xd8,
            0x82, 0xed, 0xbf, 0x02, 0x04, 0x8e, 0xa3, 0x01, 0x44, 0x03, 0x04, 0x40, 0x77, 0x1b, 0x00, 0x00, 0x00, 0x01,
            0x12, 0x01, 0x04, 0xd8, 0x82, 0xed, 0xbf, 0x02, 0x04, 0x8e, 0xa3, 0x01, 0x44, 0x03, 0x04, 0x40, 0x77, 0x1b,
            0x00, 0x00, 0x00, 0x01, 0x0f, 0x01, 0x04, 0xd8, 0x82, 0xed, 0xbf, 0x02, 0x04, 0x8e, 0xa3, 0x01, 0x44, 0x03,
            0x01, 0x64, 0x00, 0x00, 0x01, 0x02, 0xff, 0x12, 0x01, 0x01, 0xff, 0x04, 0xd8, 0x82, 0xed, 0xbf, 0x02, 0x04,
            0x8e, 0xa3, 0x01, 0x44, 0x03, 0x04, 0x40, 0x77, 0x1b, 0x00, 0x00, 0x00, 0x01, 0x12, 0x01, 0x04, 0xd8, 0x82,
            0xed, 0xbf, 0x02, 0x04, 0x8e, 0xa3, 0x01, 0x44, 0x03, 0x04, 0x40, 0x77, 0x1b, 0x05, 0xff, 0x00, 0x00, 0x00,
            0x01, 0x12, 0x01, 0x04, 0xd8, 0x82, 0xed, 0xbf, 0x02, 0x04, 0x8e, 0xa3, 0x01, 0x44, 0x03, 0x04, 0x40, 0x77,
            0x1b, 0x00, 0x00, 0x00, 0x01, 0x12, 0x01, 0x04, 0xd8, 0x82, 0xed, 0xbf, 0x02, 0x04, 0x8e, 0xa3, 0x01, 0x44,
            0x03, 0x04, 0x40, 0x77, 0x1b, 0x00, 0x00, 0x00, 0x01, 0x12, 0x01, 0x04, 0xd8, 0x82, 0xed, 0xbf, 0x02, 0x04,
            0x8e, 0xa3, 0x01, 0x44, 0x03, 0x04, 0x40, 0x77, 0x1b, 0x00, 0x00, 0x00, 0x01, 0x12, 0x01, 0x04, 0x7d, 0xd2,
            0xe7, 0xbf, 0x02, 0x04, 0x1c, 0x07, 0x00, 0x44, 0x03, 0x04, 0x40, 0x77, 0x1b, 0x00, 0x00, 0x00, 0x01, 0x12,
            0x01, 0x04, 0x0b, 0xb6, 0xe0, 0xbf, 0x02, 0x04, 0x1c, 0xc7, 0xfb, 0x43, 0x03, 0x04, 0x40, 0x77, 0x1b, 0x00,
            0x00, 0x00, 0x01, 0x12, 0x01, 0x04, 0x55, 0x55, 0xd5, 0xbf, 0x02, 0x04, 0x55, 0x55, 0xf5, 0x43, 0x03, 0x04,
            0x40, 0x77, 0x1b, 0x00, 0x00, 0x00, 0x01, 0x12, 0x01, 0x04, 0x9f, 0xf4, 0xc9, 0xbf, 0x02, 0x04, 0x8e, 0x63,
            0xee, 0x43, 0x03, 0x04, 0x40, 0x77, 0x1b, 0x00, 0x00, 0x00, 0x01, 0x12, 0x01, 0x04, 0xd2, 0x27, 0xbd, 0xbf,
            0x02, 0x04, 0x8e, 0x63, 0xe7, 0x43, 0x03, 0x04, 0x40, 0x77, 0x1b, 0x00, 0x00, 0x00, 0x01, 0x12, 0x01, 0x04,
            0xd8, 0x82, 0xad, 0xbf, 0x02, 0x04, 0x1c, 0xc7, 0xdf, 0x43, 0x03, 0x04, 0x40, 0x77, 0x1b, 0x00, 0x00, 0x00,
            0x01, 0x12, 0x01, 0x04, 0xf5, 0x49, 0x9f, 0xbf, 0x02, 0x04, 0x02, 0xff, 0xe4, 0xb8, 0xd8, 0x43, 0x01, 0xff,
            0x03, 0x04, 0x40, 0x77, 0x1b, 0x00, 0x00, 0x00, 0x01, 0x12, 0x01, 0x04, 0x3f, 0xe9, 0x93, 0xbf, 0x02, 0x04,
            0x1c, 0xc7, 0xd1, 0x43, 0x03, 0x04, 0x40, 0x77, 0x1b, 0x00, 0x00, 0x00, 0x01, 0x12, 0x01, 0x04, 0x11, 0x11,
            0x91, 0xbf, 0x02, 0x04, 0x05, 0xff, 0xab, 0xaa, 0xcb, 0x43, 0x03, 0x04, 0x40, 0x77, 0x1b, 0x00, 0x00, 0x00,
            0x01, 0x12, 0x01, 0x04, 0x83, 0x2d, 0x98, 0xbf, 0x02, 0x04, 0xc7, 0xf1, 0xc5, 0x43, 0x03, 0x04, 0x40, 0x77,
            0x1b, 0x00, 0x00, 0x00, 0x01, 0x12, 0x01, 0x04, 0x66, 0x66, 0xa6, 0xbf, 0x02, 0x04, 0x00, 0x80, 0xc0, 0x43,
            0x03, 0x04, 0x40, 0x77, 0x1b, 0x00, 0x00, 0x00, 0x01, 0x12, 0x01, 0x04, 0x61, 0x0b, 0xb6, 0xbf, 0x02, 0x04,
            0x72, 0x9c, 0xbb, 0x43, 0x03, 0x04, 0x40, 0x77, 0x1b, 0x00, 0x00, 0x00, 0x01, 0x12, 0x01, 0x04, 0x44, 0x44,
            0xc4, 0xbf, 0x02, 0x04, 0xab, 0xaa, 0xb7, 0x43, 0x03, 0x04, 0x40, 0x77, 0x1b, 0x00, 0x00, 0x00, 0x01, 0x12,
            0x01, 0x04, 0xcd, 0xcc, 0xcc, 0xbf, 0x02, 0x04, 0x00, 0x00, 0xb4, 0x43, 0x03, 0x04, 0x40, 0x77, 0x1b, 0x00,
            0x00, 0x00, 0x01, 0x12, 0x01, 0x04, 0x3f, 0xe9, 0xd3, 0xbf, 0x02, 0x04, 0x1c, 0xc7, 0xb1, 0x43, 0x03, 0x04,
            0x40, 0x77, 0x1b, 0x00, 0x00, 0x00, 0x01, 0x12, 0x01, 0x04, 0x6c, 0xc1, 0xd6, 0xbf, 0x02, 0x04, 0x8e, 0x63,
            0xb0, 0x43, 0x03, 0x04, 0x40, 0x77, 0x1b, 0x00, 0x00, 0x00, 0x01, 0x12, 0x01, 0x04, 0x83, 0x2d, 0xd8, 0xbf,
            0x02, 0x04, 0xc7, 0x71, 0xaf, 0x43, 0x03, 0x04, 0x40, 0x77, 0x1b, 0x00, 0x00, 0x00, 0x01, 0x12, 0x01, 0x04,
            0x83, 0x2d, 0xd8, 0xbf, 0x02, 0x04, 0xc7, 0xf1, 0xae, 0x43, 0x03, 0x04, 0x40, 0x77, 0x1b, 0x02, 0xff, 0x00,
            0x00, 0x00, 0x01, 0x12, 0x01, 0x01, 0xff, 0x04, 0x83, 0x2d, 0xd8, 0xbf, 0x02, 0x04, 0xc7, 0xf1, 0xae, 0x43,
            0x03, 0x04, 0x40, 0x77, 0x1b, 0x00, 0x00, 0x00, 0x01, 0x12, 0x01, 0x04, 0x83, 0x2d, 0xd8, 0xbf, 0x02, 0x04,
            0xc7, 0xf1, 0xae, 0x43, 0x03, 0x04, 0x40, 0x77, 0x1b, 0x00, 0x00, 0x00, 0x01, 0x05, 0xff, 0x12, 0x01, 0x04,
            0x6c, 0xc1, 0xd6, 0xbf, 0x02, 0x04, 0x8e, 0xe3, 0xae, 0x43, 0x03, 0x04, 0x40, 0x77, 0x1b, 0x00, 0x00, 0x00,
            0x01, 0x12, 0x01, 0x04, 0x83, 0x2d, 0xd8, 0xbf, 0x02, 0x04, 0xc7, 0xf1, 0xae, 0x43, 0x03, 0x04, 0x40, 0x77,
            0x1b, 0x00, 0x00, 0x00, 0x01, 0x12, 0x01, 0x04, 0x83, 0x2d, 0xd8, 0xbf, 0x02, 0x04, 0xc7, 0xf1, 0xae, 0x43,
            0x03, 0x04, 0x40, 0x77, 0x1b, 0x00, 0x00, 0x00, 0x01, 0x12, 0x01, 0x04, 0x83, 0x2d, 0xd8, 0xbf, 0x02, 0x04,
            0xc7, 0xf1, 0xae, 0x43, 0x03, 0x04, 0x40, 0x77, 0x1b, 0x00, 0x00, 0x00, 0x01, 0x12, 0x01, 0x04, 0x3f, 0xe9,
            0xd3, 0xbf, 0x02, 0x04, 0x1c, 0x47, 0xaf, 0x43, 0x03, 0x04, 0x40, 0x77, 0x1b, 0x00, 0x00, 0x00, 0x01, 0x12,
            0x01, 0x04, 0xe4, 0x38, 0xce, 0xbf, 0x02, 0x04, 0x39, 0x0e, 0xb0, 0x43, 0x03, 0x04, 0x40, 0x77, 0x1b, 0x00,
            0x00, 0x00, 0x01, 0x12, 0x01, 0x04, 0x72, 0x1c, 0xc7, 0xbf, 0x02, 0x04, 0x1c, 0xc7, 0xb1, 0x43, 0x03, 0x04,
            0x40, 0x77, 0x1b, 0x00, 0x00, 0x00, 0x01, 0x12, 0x01, 0x04, 0xbc, 0xbb, 0xbb, 0xbf, 0x02, 0x04, 0x55, 0x55,
            0xb3, 0x43, 0x03, 0x04, 0x40, 0x77, 0x1b, 0x00, 0x00, 0x00, 0x01, 0x12, 0x01, 0x04, 0x06, 0x5b, 0xb0, 0xbf,
            0x02, 0x04, 0x8e, 0x63, 0xb6, 0x43, 0x03, 0x04, 0x40, 0x77, 0x1b, 0x00, 0x00, 0x00, 0x01, 0x12, 0x01, 0x04,
            0x50, 0xfa, 0x02, 0xa7, 0xa4, 0xbf, 0x02, 0x04, 0xc7, 0xf1, 0xb9, 0x43, 0x01, 0x9d, 0x03, 0x04, 0x40, 0x77,
            0x1b, 0x00, 0x00, 0x00, 0x01, 0x12, 0x01, 0x04, 0xde, 0xdd, 0x9d, 0xbf, 0x02, 0x04, 0xab, 0x2a, 0xbf, 0x43,
            0x03, 0x04, 0x40, 0x77, 0x1b, 0x00, 0x00, 0x00, 0x01, 0x12, 0x01, 0x04, 0xf5, 0x49, 0x9f, 0xbf, 0x02, 0x04,
            0xe4, 0xb8, 0xc6, 0x43, 0x05, 0x6f, 0x03, 0x04, 0x40, 0x77, 0x1b, 0x00, 0x00, 0x00, 0x01, 0x12, 0x01, 0x04,
            0x7d, 0xd2, 0xa7, 0xbf, 0x02, 0x04, 0x39, 0x0e, 0xd0, 0x43, 0x03, 0x04, 0x40, 0x77, 0x1b, 0x00, 0x00, 0x00,
            0x01, 0x12, 0x01, 0x04, 0xa5, 0x4f, 0xba, 0xbf, 0x02, 0x04, 0x1c, 0xc7, 0xdb, 0x43, 0x03, 0x04, 0x40, 0x77,
            0x1b, 0x00, 0x00, 0x00, 0x01, 0x12, 0x01, 0x04, 0x11, 0x11, 0xd1, 0xbf, 0x02, 0x04, 0xab, 0xaa, 0xe7, 0x43,
            0x03, 0x04, 0x40, 0x77, 0x1b, 0x00, 0x00, 0x00, 0x01, 0x12, 0x01, 0x04, 0x50, 0xfa, 0xe4, 0xbf, 0x02, 0x04,
            0xc7, 0xf1, 0xf0, 0x43, 0x03, 0x04, 0x08, 0x4b, 0x16, 0x00, 0x02, 0x01, 0xD4, 0x03, 0x0c, 0x01, 0x04, 0x0a,
            0x00, 0x00, 0x00, 0x02, 0x04, 0x64, 0x00, 0x00, 0x00
        };
        TestTransitionControlWrite(&session, &accessoryServer, requestBytes, sizeof requestBytes);

        TestPersistentState();

        // Read current transition state
        HAPAssert(TestTransitionControlRead(&session, &accessoryServer, NULL) == kHAPError_None);

        // Issue fetch command for Color Temp
        HAPAssert(TestTransitionControlFetch(&session, &accessoryServer, kIID_LightBulbColorTemp) == kHAPError_None);

        // Read Color Temp transition
        HAPAssert(TestTransitionControlRead(&session, &accessoryServer, NULL) == kHAPError_None);

        // Remove Color Temp transition
        RemoveTransition(kIID_LightBulbColorTemp);
    }

    {
        PurgeAdaptiveLightKVS();
        HAPLog(&kHAPLog_Default, "Test two transitions together");
        uint8_t requestBytes[] = { 0x02, 0x91, 0x01, 0x55, 0x01, 0x08, 0xD7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x03, 0x01, 0x01, 0x05, 0x46, 0x01, 0x16, 0x01, 0x04, 0x85, 0xEB, 0x41, 0x41, 0x02,
                                   0x04, 0x14, 0x3E, 0x2E, 0x45, 0x03, 0x08, 0xD4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x01, 0x16, 0x01, 0x04, 0x9A, 0x99, 0x21, 0x41, 0x02, 0x04, 0x66,
                                   0x5E, 0x79, 0x45, 0x03, 0x08, 0xA0, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
                                   0x08, 0xD4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x08, 0x01, 0x02, 0x0A,
                                   0x00, 0x02, 0x02, 0x64, 0x00, 0x00, 0x00, 0x01, 0x36, 0x01, 0x08, 0xD4, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x01, 0x00, 0x04, 0x27, 0x01, 0x0F, 0x01, 0x03,
                                   0x32, 0x00, 0x00, 0x02, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x01, 0x0F, 0x01, 0x03, 0x5A, 0x00, 0x00, 0x02, 0x08, 0xA0, 0x0F, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x00 };
        TestTransitionControlWrite(&session, &accessoryServer, requestBytes, sizeof requestBytes);

        TestPersistentState();
        // Issue fetch command for Brightness
        HAPAssert(TestTransitionControlFetch(&session, &accessoryServer, kIID_LightBulbBrightness) == kHAPError_None);

        // Read transition
        HAPAssert(TestTransitionControlRead(&session, &accessoryServer, NULL) == kHAPError_None);

        // Issue fetch command for Color Temp
        HAPAssert(TestTransitionControlFetch(&session, &accessoryServer, kIID_LightBulbColorTemp) == kHAPError_None);

        // Read transition
        HAPAssert(TestTransitionControlRead(&session, &accessoryServer, NULL) == kHAPError_None);

        {
            // Erasing two transitions together
            uint8_t requestBytes[] = { 0x02, 0x0C, 0x01, 0x03, 0x01, 0x01, 0xD4,
                                       0x00, 0x00, 0x01, 0x03, 0x01, 0x01, 0xD7 };

            TestTransitionControlWrite(&session, &accessoryServer, requestBytes, sizeof requestBytes);
        }
    }

    // Remove Brightness transition
    RemoveTransition(0);

    // Remove Brightness transition, again and expect error
    RemoveTransition(0);

    // Remove Brightness
    RemoveTransition(3);

    // Issue fetch command for Color Temp
    HAPAssert(TestTransitionControlFetch(&session, &accessoryServer, kIID_LightBulbColorTemp) == kHAPError_None);

    // Read transition
    size_t numBytes;
    HAPAssert(TestTransitionControlRead(&session, &accessoryServer, &numBytes) == kHAPError_None);
    HAPAssert(numBytes == 0);

    // Ensure correct error is returned if characteristic does not support transitions.
    // See HomeKit Accessory Protocol Specification R17
    // Section 11.147 Characteristic Value Transition Control
    HAPAssert(TestTransitionControlFetch(&session, &accessoryServer, kIID_LightBulbName) == kHAPError_Unknown);

    return 0;
}
