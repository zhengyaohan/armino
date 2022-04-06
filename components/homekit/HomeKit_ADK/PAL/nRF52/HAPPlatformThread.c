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
#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
#include <mbedtls/platform.h>
#include <openthread/heap.h>
#include <openthread/platform/openthread-system.h>
#include <openthread/tasklet.h>
#include <openthread/thread.h>

#include "HAP.h"
#include "HAPLogSubsystem.h"
#include "HAPPlatform.h"

#include "app_util_platform.h"
#include "mem_manager.h"
#include "nrf_sdh.h"

#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
static void fpu_sleep_prepare(void);
#endif

static void* ot_calloc(size_t n, size_t size) {
    void* p_ptr = NULL;

    p_ptr = nrf_calloc(n, size);
    return p_ptr;
}

static void ot_free(void* p_ptr) {
    nrf_free(p_ptr);
}

void HAPPlatformThreadInit() {
    APP_ERROR_CHECK(nrf_mem_init());

    otHeapSetCAllocFree(ot_calloc, ot_free);
    otSysInit(0, NULL);

    HAPPlatformThreadInitInstance();
}

// PLATFORM SPECIFIC SLEEP FUNCTION
void HAPPlatformThreadSleep(void) {
    otInstance* threadInstance = (otInstance*) HAPPlatformThreadGetHandle();
    if (HAPPlatformThreadIsInitialized()) {
        HAPAssert(threadInstance != NULL);
    }

#ifdef NRF52840_XXAA
    // Workaround for issue with nrf_security.
    // Issue:
    // The MCU is prevented from going to sleep mode
    // by the pending CryptoCell interrupt in the NVIC register.
    // Workaround:
    // The CRYPTOCELL_IRQn interrupt is cleared here manually by calling NVIC_ClearPendingIRQ().
    NVIC_ClearPendingIRQ(CRYPTOCELL_IRQn);
#endif

    // Enter sleep state if no more tasks are pending.
    if (!HAPPlatformThreadIsInitialized() || !otTaskletsArePending(threadInstance)) {
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
        fpu_sleep_prepare();
#endif

        if (nrf_sdh_is_enabled()) {
            sd_app_evt_wait();
        } else {
            __WFE();
        }
    }
}

#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
static void fpu_sleep_prepare(void) {
    uint32_t original_fpscr;

    CRITICAL_REGION_ENTER();
    original_fpscr = __get_FPSCR();
    /*
     * Clear FPU exceptions.
     * Without this step, the FPU interrupt is marked as pending,
     * preventing system from sleeping. Exceptions cleared:
     * - IOC - Invalid Operation cumulative exception bit.
     * - DZC - Division by Zero cumulative exception bit.
     * - OFC - Overflow cumulative exception bit.
     * - UFC - Underflow cumulative exception bit.
     * - IXC - Inexact cumulative exception bit.
     * - IDC - Input Denormal cumulative exception bit.
     */
    __set_FPSCR(original_fpscr & ~0x9Fu);
    __DMB();
    NVIC_ClearPendingIRQ(FPU_IRQn);
    CRITICAL_REGION_EXIT();

    /*
     * The last chance to indicate an error in FPU to the user
     * as the FPSCR is now cleared
     *
     * This assert is related to previous FPU operations
     * and not power management.
     *
     * Critical FPU exceptions signaled:
     * - IOC - Invalid Operation cumulative exception bit.
     * - DZC - Division by Zero cumulative exception bit.
     * - OFC - Overflow cumulative exception bit.
     */

    ASSERT((original_fpscr & 0x7) == 0);
}
#endif // (__FPU_PRESENT == 1) && (__FPU_USED == 1)

#endif // HAVE THREAD
