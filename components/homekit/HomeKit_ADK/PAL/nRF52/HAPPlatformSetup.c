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
// Copyright (C) 2015-2020 Apple Inc. All Rights Reserved.

#include <mbedtls/memory_buffer_alloc.h>
#include <mbedtls/platform.h>

#include "app_scheduler.h"
#include "app_timer.h"
#include "ble_conn_params.h"
#include "boards.h"
#include "mem_manager.h"
#include "nrf_ble_gatt.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"

#include "HAP.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD) && HAP_FEATURE_ENABLED(HAP_FEATURE_BLE)
#include "nrf_sdh_soc.h"
#endif

#include "HAPPlatformKeyValueStore+Init.h"
#include "HAPPlatformMFiHWAuth+Init.h"
#include "HAPPlatformRunLoop+Init.h"
#include "HAPPlatformSetup+Init.h"
#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
#include "HAPPlatformThreadUtils+Init.h"
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
#include <openthread/instance.h>
#include <openthread/thread.h>
#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE)
#include <openthread/platform/openthread-system.h>
#include <openthread/platform/platform-softdevice.h>
#endif
#include "HAPPlatformServiceDiscovery+Init.h"
#include "HAPPlatformThreadCoAPManager+Init.h"
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)

/**
 * CoAP manager
 */
static HAPPlatformThreadCoAPManager* setupCoapManager;

#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE)
/**
 * BLE peripheral manager reference
 */
static HAPPlatformBLEPeripheralManager* setupBlePeripheralManager;
#endif // HAP_FEATURE_BLE

#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD) && HAP_FEATURE_ENABLED(HAP_FEATURE_BLE)
/**
 * Soft device SOC event handler
 */
static void SocEvtHandler(uint32_t sys_evt, void* p_context) {
    otSysSoftdeviceSocEvtHandler(sys_evt);
}
#endif

#if defined(MBEDTLS_CONFIG_FILE)
static void* local_calloc(size_t n, size_t size) {
    void* p_ptr = NULL;
    p_ptr = nrf_calloc(n, size);
    return p_ptr;
}

static void local_free(void* p_ptr) {
    nrf_free(p_ptr);
}
#endif

void HAPPlatformSetupDrivers(void) {
    // Initialize event scheduler
    APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);

    // Initialize the timer module
    ret_code_t err = app_timer_init();
    HAPAssert(!err);

    // Enable NRF log module
    err = NRF_LOG_INIT(NULL);
    HAPAssert(!err);
    NRF_LOG_DEFAULT_BACKENDS_INIT();

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE)
    err = nrf_sdh_enable_request();
    HAPAssert(!err);
#endif

//  If MbedTLS crypto is being used then tell MbedTLS to use buffer allocator functions
#if defined(MBEDTLS_CONFIG_FILE)
    nrf_mem_init();
    int ret = mbedtls_platform_set_calloc_free(local_calloc, local_free);
    HAPAssert(ret == 0);
    ret = mbedtls_platform_setup(NULL);
    HAPAssert(ret == 0);
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE)
    NRF_SDH_SOC_OBSERVER(m_soc_observer, NRF_SDH_SOC_STACK_OBSERVER_PRIO, SocEvtHandler, NULL);
#endif // HAP_FEATURE_BLE
    // Perform platform specific initialization.
    HAPPlatformThreadInit();
#endif // HAP_FEATURE_THREAD
}

bool HAPPlatformSupportsBLE(void) {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE)
    return true;
#else
    return false;
#endif
}

bool HAPPlatformSupportsThread(void) {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    return true;
#else
    return false;
#endif
}

void HAPPlatformSetupInitBLE(
        HAPPlatformBLEPeripheralManager* blePeripheralManager,
        HAPPlatformKeyValueStoreRef keyValueStore) {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE)
    // Create BLE Peripheral Manager

    static HAPPlatformBLEPeripheralManagerOptions blePMOptions = { 0 };
    blePMOptions.keyValueStore = keyValueStore;
    static ble_uuid128_t uuidBases[NRF_SDH_BLE_VS_UUID_COUNT];
    blePMOptions.uuidBases = uuidBases;
    blePMOptions.numUUIDBases = HAPArrayCount(uuidBases);

    HAPPlatformBLEPeripheralManagerCreate(blePeripheralManager, &blePMOptions);

    // Set up peripheral manager refernce used by the handler
    setupBlePeripheralManager = blePeripheralManager;
#else
    HAPAssertionFailure();
#endif
}

void HAPPlatformSetupInitKeyValueStore(HAPPlatformKeyValueStoreRef keyValueStoreRef) {
    // Create key-value store with platform specific options
    HAP_ALIGNAS(4)
    static uint8_t keyValueStoreBytes[4096];

    HAPPlatformKeyValueStoreCreate(
            keyValueStoreRef,
            &(const HAPPlatformKeyValueStoreOptions) {
                    .baseFDSFileID = 0xBF00,
                    .baseFDSRecordID = 0xBF00,
                    .bytes = keyValueStoreBytes,
                    .maxBytes = sizeof keyValueStoreBytes,
            });
}

#if HAP_FEATURE_ENABLED(HAP_FEATURE_NFC)
void HAPPlatformSetupInitAccessorySetupNFC(HAPPlatformAccessorySetupNFCRef setupNFC) {
    HAPPlatformAccessorySetupNFCCreate(setupNFC);
}
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_MFI_HW_AUTH)
void HAPPlatformSetupInitMFiHWAuth(HAPPlatformMFiHWAuthRef mfiHWAuth) {
    HAPPlatformMFiHWAuthCreate(
            mfiHWAuth,
            &(const HAPPlatformMFiHWAuthOptions) {
                    .vccPin = 28, .sclPin = 3, .sdaPin = 4, .twiInstanceID = 0, .i2cAddress = 0x10 });
}
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
void HAPPlatformSetupInitThread(
        HAPAccessoryServer* accessoryServer,
        HAPPlatformThreadCoAPManager* coapManager,
        HAPPlatformThreadCoAPManagerResource* resources,
        size_t numResources,
        HAPPlatformThreadCoAPRequest* coapRequests,
        size_t numRequests,
        HAPPlatformServiceDiscovery* serviceDiscovery) {
    HAPPrecondition(coapManager);
    HAPPrecondition(resources);
    HAPPrecondition(serviceDiscovery);

    setupCoapManager = coapManager;

    HAPPlatformThreadCoAPManagerCreate(
            coapManager,
            &(const HAPPlatformThreadCoAPManagerOptions) { .port = OT_DEFAULT_COAP_PORT,
                                                           .resources = resources,
                                                           .numResources = numResources,
                                                           .requests = coapRequests,
                                                           .numRequests = numRequests });
    HAPPlatformServiceDiscoveryCreate(serviceDiscovery, &(const HAPPlatformServiceDiscoveryOptions) {});
}
#endif

HAPError HAPPlatformSetupHandleOpenThreadVersionRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPStringCharacteristicReadRequest* request,
        char* value,
        size_t maxValueBytes,
        void* _Nullable context HAP_UNUSED) {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    const char* version = otGetVersionString();
    size_t numBytes = HAPStringGetNumBytes(version);
    if (numBytes >= maxValueBytes) {
        HAPLog(&kHAPLog_Default, "Not enough space available to send version string.");
        return kHAPError_OutOfResources;
    }
    HAPRawBufferCopyBytes(value, version, numBytes);
    value[numBytes] = '\0';
    return kHAPError_None;
#else
    HAPFatalError();
#endif
}
