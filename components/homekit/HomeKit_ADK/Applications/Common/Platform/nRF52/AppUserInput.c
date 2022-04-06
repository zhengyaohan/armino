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

#if (HAP_TESTING == 1)

#include "app_button.h"
#include "boards.h"

#include "HAP.h"

#include "AppUserInput.h"

/**
 * Application button event callback function
 */
static HAPPlatformRunLoopCallback userInputEventCallback;

/**
 * Convert button PIN number to user input identifier
 *
 * @param pin  pin number of the button
 * @return user input identifier
 */
static AppUserInputIdentifier ButtonPinToId(uint8_t pin) {
    switch (pin) {
        case BUTTON_1:
            return kAppUserInputIdentifier_1;
        case BUTTON_2:
            return kAppUserInputIdentifier_2;
        case BUTTON_3:
            return kAppUserInputIdentifier_3;
        case BUTTON_4:
            return kAppUserInputIdentifier_4;
    }
    HAPFatalError();
    return UINT8_MAX;
}

/**
 * Button event callback function registered to app_button
 */
static void HandleButtonEvent(uint8_t pin_no, uint8_t button_action) {
    if (button_action != APP_BUTTON_RELEASE) {
        // Callback should be made only for release event
        return;
    }

    AppUserInputEvent userInputEvent = { .id = ButtonPinToId(pin_no) };
    if (userInputEvent.id == UINT8_MAX) {
        // Unsupported IO event
        return;
    }

    HAPError err = HAPPlatformRunLoopScheduleCallback(userInputEventCallback, &userInputEvent, sizeof userInputEvent);
    if (err) {
        HAPFatalError();
    }
}

void AppUserInputInitPlatform(HAPPlatformRunLoopCallback callback) {
    userInputEventCallback = callback;

    // app_button_init() should be called only once.
    static bool buttonsAreConfigured = false;

    if (!buttonsAreConfigured) {
        // Configure buttons
        static app_button_cfg_t buttonConfigs[] = {
            { .pin_no = BUTTON_1,
              .active_state = APP_BUTTON_ACTIVE_LOW,
              .pull_cfg = NRF_GPIO_PIN_PULLUP,
              .button_handler = HandleButtonEvent },
            { .pin_no = BUTTON_2,
              .active_state = APP_BUTTON_ACTIVE_LOW,
              .pull_cfg = NRF_GPIO_PIN_PULLUP,
              .button_handler = HandleButtonEvent },
            { .pin_no = BUTTON_3,
              .active_state = APP_BUTTON_ACTIVE_LOW,
              .pull_cfg = NRF_GPIO_PIN_PULLUP,
              .button_handler = HandleButtonEvent },
            { .pin_no = BUTTON_4,
              .active_state = APP_BUTTON_ACTIVE_LOW,
              .pull_cfg = NRF_GPIO_PIN_PULLUP,
              .button_handler = HandleButtonEvent },
        };

        uint32_t e = app_button_init(buttonConfigs, HAPArrayCount(buttonConfigs), /* detection_delay: */ 1000);
        if (e) {
            HAPLogError(&kHAPLog_Default, "app_button_init failed: 0x%04x.", (unsigned int) e);
            HAPFatalError();
        }
        e = app_button_enable();
        if (e) {
            HAPLogError(&kHAPLog_Default, "app_button_enable failed: 0x%04x.", (unsigned int) e);
            HAPFatalError();
        }

        buttonsAreConfigured = true;
    }
}

#endif // HAP_TESTING
