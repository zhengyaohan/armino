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

#include <stdlib.h>

#include "app_error.h"
#include "nrf_delay.h"
#ifdef SOFTDEVICE_PRESENT
#include "nrf_nvic.h"
#endif
#if (SOFTDEVICE_PRESENT == 1)
#include "nrf_sdm.h"
#endif

#include "HAPPlatform.h"

void app_error_fault_handler(uint32_t id HAP_UNUSED, uint32_t pc HAP_UNUSED, uint32_t info) {
    __disable_irq();
    // Copy error info.
    static error_info_t* errorInfo;
    errorInfo = (error_info_t*) info;
    nrf_delay_ms(50);
    switch (id) {
#if (SOFTDEVICE_PRESENT == 1)
        case NRF_FAULT_ID_SD_ASSERT: {
            HAPLogFault(&kHAPLog_Default, "nRF SD assert, pc=0x%lx", pc);
            break;
        }
        case NRF_FAULT_ID_APP_MEMACC: {
            HAPLogFault(&kHAPLog_Default, "nRF mem fault, pc=0x%lx info=0x%lx", pc, info);
            break;
        }
#endif
        case NRF_FAULT_ID_SDK_ASSERT: {
            HAPLogFault(
                    &kHAPLog_Default,
                    "nRF sdk assert, pc=0x%lx %s:%lu 0x%lx",
                    pc,
                    errorInfo->p_file_name,
                    errorInfo->line_num,
                    errorInfo->err_code);
            break;
        }
        case NRF_FAULT_ID_SDK_ERROR: {
            HAPLogFault(
                    &kHAPLog_Default,
                    "nRF sdk error, pc=0x%lx %s:%lu 0x%lx",
                    pc,
                    errorInfo->p_file_name,
                    errorInfo->line_num,
                    errorInfo->err_code);
            break;
        }
        default: {
            HAPLogFault(&kHAPLog_Default, "nRF unknown fault id=0x%lx, pc=0x%lx, info=0x%lx", id, pc, info);
        }
    }
    nrf_delay_ms(50);
    NRF_BREAKPOINT_COND;
    NVIC_SystemReset();
}

HAP_NORETURN
void HAPPlatformAbort(void) {
    // Allow logs to flush.
    nrf_delay_ms(50);

    // Escalate error to SDK.
    APP_ERROR_CHECK_BOOL(false);
#ifdef SOFTDEVICE_PRESENT
    sd_nvic_SystemReset();
#else
    NVIC_SystemReset();
#endif
    for (;;)
        ;
}
