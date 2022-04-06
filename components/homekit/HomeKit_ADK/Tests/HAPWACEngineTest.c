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

#include "HAPWACEngine.h"
#include "HAPWACEngine+Types.h"

#include "HAPPlatform+Init.h"

#include "Harness/HAPAccessoryServerHelper.h"
#include "Harness/HAPPlatformClockHelper.h"
#include "Harness/HAPPlatformWiFiManagerHelper.h"
#include "Harness/IPTestHelpers.h"
#include "Harness/SecurityTestHelpers.h"
#include "Harness/TemplateDB.c"

int main() {
    HAPError err;
    HAPPlatformCreate();

    // Prepare key-value store.
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    HAPControllerPublicKey controllerPublicKey;
    TestPrepareKeyValueStore(controllerPairingID, &controllerPublicKey);

    // Prepare accessory server storage.
    HAPIPSession ipSessions[kHAPIPSessionStorage_DefaultNumElements];
    IPSessionState ipSessionStates[HAPArrayCount(ipSessions)];
    HAPIPReadContext ipReadContexts[kAttributeCount];
    HAPIPWriteContext ipWriteContexts[kAttributeCount];
    uint8_t ipScratchBuffer[kHAPIPSession_DefaultScratchBufferSize];
    InitializeIPSessions(ipSessions, ipSessionStates, HAPArrayCount(ipSessions));
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
    HAPAccessoryServerCreate(
            &accessoryServer,
            &(const HAPAccessoryServerOptions) { .maxPairings = kHAPPairingStorage_MinElements,
                                                 .ip = { .transport = &kHAPAccessoryServerTransport_IP,
                                                         .accessoryServerStorage = &ipAccessoryServerStorage,
                                                         .wac = { .available = true } } },
            &platform,
            &(const HAPAccessoryServerCallbacks) { .handleUpdatedState =
                                                           HAPAccessoryServerHelperHandleUpdatedAccessoryServerState },
            /* context: */ NULL);

    // Start accessory server.
    const HAPAccessory accessory = { .aid = 1,
                                     .category = kHAPAccessoryCategory_Other,
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
                                                                               NULL },
                                     .callbacks = { .identify = IdentifyAccessoryHelper } };
    HAPAccessoryServerStart(&accessoryServer, &accessory);

    // Process events.
    HAPPlatformClockAdvance(0);
    HAPAssert(HAPAccessoryServerGetState(&accessoryServer) == kHAPAccessoryServerState_Running);

    // Create fake security session.
    HAPIPSecuritySession session = { .type = kHAPIPSecuritySessionType_HAP, .isOpen = true, .isSecured = true };
    session._.hap = (HAPSession) { .hap = { .active = true, .pairingID = controllerPairingID } };

    // Configure Wi-Fi settings.
    const char* ssid = "123456789";
    const char* _Nullable passphrase = "abcdefgh";
    const char* _Nullable regulatoryDomain = "GB";
    {
        // Create /config payload.
        uint8_t requestBytes[1024];
        HAPTLVWriter writer;
        HAPTLVWriterCreate(&writer, requestBytes, sizeof requestBytes);

        // SSID.
        err = HAPTLVWriterAppend(
                &writer,
                &(const HAPTLV) { .type = kHAPWACTLVType_WiFiSSID,
                                  .value = { .bytes = ssid, .numBytes = HAPStringGetNumBytes(ssid) } });
        HAPAssert(!err);

        // Passphrase.
        if (passphrase) {
            err = HAPTLVWriterAppend(
                    &writer,
                    &(const HAPTLV) { .type = kHAPWACTLVType_WiFiPSK,
                                      .value = { .bytes = passphrase,
                                                 .numBytes = HAPStringGetNumBytes(HAPNonnull(passphrase)) } });
            HAPAssert(!err);
        }

        // Regulatory domain.
        if (regulatoryDomain) {
            err = HAPTLVWriterAppend(
                    &writer,
                    &(const HAPTLV) { .type = kHAPWACTLVType_CountryCode,
                                      .value = { .bytes = regulatoryDomain,
                                                 .numBytes = HAPStringGetNumBytes(HAPNonnull(regulatoryDomain)) } });
            HAPAssert(!err);
        }

        // Process /config request.
        void* bytes;
        size_t numBytes;
        HAPTLVWriterGetBuffer(&writer, &bytes, &numBytes);
        uint8_t responseBytes[1024];
        size_t numResponseBytes;
        err = HAPWACEngineHandleConfig(
                &accessoryServer, &session, bytes, numBytes, responseBytes, sizeof responseBytes, &numResponseBytes);
        HAPAssert(!err);
    }

    // Trigger WAC timeout to trigger IP accessory server garbage collection.
    HAPPlatformClockAdvance(900 * HAPSecond);

    // Process events.
    HAPPlatformClockAdvance(0);

    // Check that Wi-Fi configuration has been applied.
    {
        HAPAssert(HAPPlatformWiFiManagerIsConfigured(HAPNonnull(platform.ip.wiFi.wiFiManager)));

        // SSID.
        const char* configuredSSID = HAPPlatformWiFiManagerGetConfiguredSSID(HAPNonnull(platform.ip.wiFi.wiFiManager));
        HAPLog(&kHAPLog_Default, "Configured SSID: %s", configuredSSID);
        HAPAssert(HAPStringAreEqual(configuredSSID, ssid));

        // Passphrase.
        const char* _Nullable configuredPassphrase =
                HAPPlatformWiFiManagerGetPassphrase(HAPNonnull(platform.ip.wiFi.wiFiManager));
        if (passphrase) {
            HAPLog(&kHAPLog_Default, "Configured passphrase: %s", configuredPassphrase);
            HAPAssert(configuredPassphrase);
            HAPAssert(HAPStringAreEqual(HAPNonnull(configuredPassphrase), HAPNonnull(passphrase)));
        } else {
            HAPAssert(!configuredPassphrase);
        }

        // Regulatory domain.
        HAPPlatformRegulatoryDomain configuredRegulatoryDomain;
        err = HAPPlatformWiFiManagerGetRegulatoryDomain(
                HAPNonnull(platform.ip.wiFi.wiFiManager), &configuredRegulatoryDomain);
        if (regulatoryDomain) {
            if (err) {
                HAPAssert(err == kHAPError_InvalidState || err == kHAPError_Unknown);
                HAPLog(&kHAPLog_Default, "Configured regulatory domain: (NULL)");
                HAPAssertionFailure();
            } else {
                HAPLog(&kHAPLog_Default, "Configured regulatory domain: %s", configuredRegulatoryDomain.stringValue);
                HAPAssert(HAPStringAreEqual(configuredRegulatoryDomain.stringValue, HAPNonnull(regulatoryDomain)));
            }
        } else {
            HAPAssert(err == kHAPError_InvalidState || err == kHAPError_Unknown);
        }
    }
}
