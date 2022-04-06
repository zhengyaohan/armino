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
// Copyright (C) 2020 Apple Inc. All Rights Reserved.

#include "app_scheduler.h"
#include "app_timer.h"
#include "ble_conn_params.h"
#include "boards.h"
#include "nrf_ble_gatt.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"

#include "HAP.h"

#include "HAPPlatformBLEPeripheralManager+Init.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE)
/**
 * BLE platform is initialized for use by transport
 */
static bool bleIsInitialized = false;

/**
 * BLE stack is enabled
 */
static bool bleIsEnabled = false;

/**
 * Application's BLE observer priority. You shouldn't need to modify this value.
 */
#define APP_BLE_OBSERVER_PRIO 3

/**
 * GATT module instance.
 */
NRF_BLE_GATT_DEF(m_gatt);

/**
 * BLE peripheral manager reference
 */
static HAPPlatformBLEPeripheralManager* blePeripheralManager;

/**
 * Error handler
 */
static void ConnParamsErrorHandler(uint32_t nrf_error) {
    HAPLogError(&kHAPLog_Default, "%s: %lu.", __func__, (unsigned long) nrf_error);
    HAPFatalError();
}

/**
 * BLE event handler function
 */
static void HandleBLEEvent(ble_evt_t const* p_ble_evt, void* _Nullable p_context) {
    HAPPrecondition(p_ble_evt);

    HAPPlatformBLEPeripheralManagerHandleBLEEvent(blePeripheralManager, p_ble_evt);
}
#endif

bool HAPPlatformBLEIsInitialized(void) {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE)
    return bleIsInitialized;
#else
    return false;
#endif
}

void HAPPlatformBLEInitialize(HAPPlatformBLEPeripheralManager* manager) {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE)
    blePeripheralManager = manager;

    ret_code_t err;

    // With multiprotocol support, BLE cannot be disabled till system reboot
    // and enabling BLE again will fail.
    if (bleIsEnabled) {
        bleIsInitialized = true;
        return;
    }

    uint32_t ram_start = 0;
    err = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    HAPAssert(!err);
    err = nrf_sdh_ble_enable(&ram_start);
    HAPLogInfo(&kHAPLog_Default, "Actual RAM start: 0x%08lx", (unsigned long) ram_start);
    if (err) {
        HAPAssert(err == NRF_ERROR_NO_MEM);
        HAPLogError(
                &kHAPLog_Default,
                "nrf_sdh_ble_enable failed: %lu - Update linker settings to RAM origin = 0x%08lx",
                (unsigned long) err,
                (unsigned long) ram_start);
        HAPFatalError();
    }

    // Register a handler for BLE events
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, HandleBLEEvent, NULL);

    // Set BLE appearance
    err = sd_ble_gap_appearance_set(BLE_APPEARANCE_UNKNOWN);
    HAPAssert(!err);

    // Set peripheral preferred connection parameters.
    ble_gap_conn_params_t gap_conn_params = {
        .min_conn_interval = MSEC_TO_UNITS(30, UNIT_1_25_MS), // 30ms
        .max_conn_interval = MSEC_TO_UNITS(40, UNIT_1_25_MS), // 40ms
        .slave_latency = 0,
        .conn_sup_timeout = MSEC_TO_UNITS(4000, UNIT_10_MS), // 4s
    };
    err = sd_ble_gap_ppcp_set(&gap_conn_params);
    HAPAssert(!err);

    // Initialize connection parameters module
    ble_conn_params_init_t cp_init = {
        .p_conn_params = NULL,
        .first_conn_params_update_delay = APP_TIMER_TICKS(500), // 0.5s
        .next_conn_params_update_delay = APP_TIMER_TICKS(5000), // 5s
        .max_conn_params_update_count = 3,
        .start_on_notify_cccd_handle = BLE_GATT_HANDLE_INVALID,
        .disconnect_on_fail = true,
        .evt_handler = NULL,
        .error_handler = ConnParamsErrorHandler,
    };
    err = ble_conn_params_init(&cp_init);
    HAPAssert(!err);

    bleIsInitialized = true;

    bleIsEnabled = true;
#else
    HAPAssertionFailure();
#endif
}

void HAPPlatformBLEDeinitialize(void) {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE)
    if (bleIsInitialized) {
        // Need to clear BLE Peripheral Manager data in order to re-register UUID
        // next time BLE transport starts
        HAPPlatformBLEPeripheralManagerClear(blePeripheralManager);

        bleIsInitialized = false;
    }
#else
    HAPAssertionFailure();
#endif
}
