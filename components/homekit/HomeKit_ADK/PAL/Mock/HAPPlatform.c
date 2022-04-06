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

#include "HAPPlatformFeatures.h"

#include "HAP+API.h"
#include "HAPLogSubsystem.h"

#include "HAPPlatform+Init.h"
#include "HAPPlatformAccessorySetup+Init.h"
#include "HAPPlatformBLEPeripheralManager+Init.h"
#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)
#include "HAPPlatformCamera+Init.h"
#endif
#include "HAPPlatformKeyValueStore+Init.h"
#include "HAPPlatformMFiHWAuth+Init.h"
#include "HAPPlatformServiceDiscovery+Init.h"
#include "HAPPlatformSoftwareAccessPoint+Init.h"
#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
#include "HAPPlatformTCPStreamManager+Init.h"
#include "HAPPlatformWiFiManager+Init.h"
#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_ROUTER)
#include "HAPPlatformWiFiRouter+Init.h"
#endif
#endif

HAP_RESULT_USE_CHECK
const char* HAPPlatformGetIdentification(void) {
    return "Test";
}

HAP_RESULT_USE_CHECK
const char* HAPPlatformGetVersion(void) {
    return "Internal";
}

HAP_RESULT_USE_CHECK
const char* HAPPlatformGetBuild(void) {
    HAP_DIAGNOSTIC_PUSH
    HAP_DIAGNOSTIC_IGNORED_CLANG("-Wdate-time")
    const char* build = __DATE__ " " __TIME__;
    HAP_DIAGNOSTIC_POP

    return build;
}

/**
 * Number of attributes to allow BLE peripheral manager to use.
 */
#define kHAPPlatform_NumBLEPeripheralManagerAttributes ((size_t) 100)

static HAPPlatformKeyValueStore keyValueStore;
static HAPPlatformAccessorySetup accessorySetup;
#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
static HAPPlatformServiceDiscovery serviceDiscovery;
static HAPPlatformTCPStreamManager tcpStreamManager;
static HAPPlatformWiFiManager wiFiManager;
static HAPPlatformSoftwareAccessPoint softwareAccessPoint;
#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)
static HAPPlatformCamera camera;
#endif
#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_ROUTER)
static HAPPlatformWiFiRouter wiFiRouter;
#endif
#endif
static HAPPlatformBLEPeripheralManager blePeripheralManager;
static HAPPlatformMFiHWAuth mfiHWAuth;
HAPPlatform platform = { .keyValueStore = &keyValueStore,
                         .accessorySetup = &accessorySetup,
#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
                         .ip = { .tcpStreamManager = &tcpStreamManager,
                                 .serviceDiscovery = &serviceDiscovery,
                                 .wiFi = { .wiFiManager = &wiFiManager, .softwareAccessPoint = &softwareAccessPoint },
#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)
                                 .camera = &camera,
#endif
#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_ROUTER)
                                 .wiFiRouter = &wiFiRouter
#endif
                         },
#endif
                         .ble = { .blePeripheralManager = &blePeripheralManager },
                         .authentication = { .mfiHWAuth = &mfiHWAuth } };

void HAPPlatformCreate(void) {
    static bool initialized = false;
    HAPPrecondition(!initialized);
    initialized = true;

    // Key-value store.
    // The Adaptive light feature requires 50 additional
    // items to be saved in the key value store.
    static HAPPlatformKeyValueStoreItem keyValueStoreItems[72];
    HAPPlatformKeyValueStoreCreate(
            platform.keyValueStore,
            &(const HAPPlatformKeyValueStoreOptions) { .items = keyValueStoreItems,
                                                       .numItems = HAPArrayCount(keyValueStoreItems) });

    // Accessory setup manager. Does not require initialization.

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
    // TCP stream manager.
    static HAPPlatformTCPStream tcpStreams[kHAPIPSessionStorage_DefaultNumElements];
    HAPPlatformTCPStreamManagerCreate(
            HAPNonnull(platform.ip.tcpStreamManager),
            &(const HAPPlatformTCPStreamManagerOptions) { .tcpStreams = tcpStreams,
                                                          .numTCPStreams = HAPArrayCount(tcpStreams) });

    // Service discovery.
    HAPPlatformServiceDiscoveryCreate(HAPNonnull(platform.ip.serviceDiscovery));

    // Wi-Fi manager.
    HAPPlatformWiFiManagerCreate(
            HAPNonnull(platform.ip.wiFi.wiFiManager), &(const HAPPlatformWiFiManagerOptions) { .interfaceName = NULL });

    // Software access point manager.
    HAPPlatformSoftwareAccessPointCreate(HAPNonnull(platform.ip.wiFi.softwareAccessPoint));

#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)
    // Camera.
    HAPPlatformCameraCreate(
            HAPNonnull(platform.ip.camera),
            &(const HAPPlatformCameraOptions) { ._ = 0, .recording = { .isSupported = true } });
#else
    platform.ip.camera = NULL;
#endif

// Wi-Fi router.
#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_ROUTER)
    HAPPlatformWiFiRouterCreate(HAPNonnull(platform.ip.wiFiRouter), &(const HAPPlatformWiFiRouterOptions) { 0 });
#endif

#endif
    // BLE peripheral manager.
    static HAPPlatformBLEPeripheralManagerAttribute attributes[kHAPPlatform_NumBLEPeripheralManagerAttributes];
    HAPPlatformBLEPeripheralManagerCreate(
            HAPNonnull(platform.ble.blePeripheralManager),
            &(const HAPPlatformBLEPeripheralManagerOptions) { .attributes = attributes,
                                                              .numAttributes = HAPArrayCount(attributes) });
    // Apple Authentication Coprocessor provider.
    HAPPlatformMFiHWAuthCreate(HAPNonnull(platform.authentication.mfiHWAuth));
}
