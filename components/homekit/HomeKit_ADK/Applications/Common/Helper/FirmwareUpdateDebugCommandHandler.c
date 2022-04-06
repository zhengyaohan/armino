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

// This debug command line serves development purposes only.
// It must not be included in production accessories and can be safely compiled out for production builds once no longer
// required for testing.

#include "ApplicationFeatures.h"

#include "HAP.h"

HAP_DIAGNOSTIC_PUSH
HAP_DIAGNOSTIC_IGNORED_GCC("-Wpedantic") // ISO C forbids an empty translation unit

#if (HAP_TESTING == 1)
#if (HAVE_FIRMWARE_UPDATE == 1)

#include "FirmwareUpdate.h"
#include "FirmwareUpdateDebugCommandHandler.h"

static void PrintUsage(void) {
    HAPLogInfo(
            &kHAPLog_Default,
            "\n"
            "firmware-update\n"
            "    setHAPState [-updateDuration  <decimal number (uint16_t)>]\n"
            "                [-stagingNotReady <decimal number (uint32_t)>]\n"
            "                [-updateNotReady  <decimal number (uint32_t)>]\n"
            "                [-updateState     <decimal number  (uint8_t)>]\n"
            "    setStagingNotReadyBit <bit index>\n"
            "    clearStagingNotReadyBit <bit index>\n"
            "    setUpdateNotReadyBit <bit index>\n"
            "    clearUpdateNotReadyBit <bit index>\n"
            "    setApplyDelay <decimal number (uint16_t)>\n");
}

#define CHECK_PARSED_VALUE(e) \
    do { \
        if (e) { \
            PrintUsage(); \
            return e; \
        } \
    } while (0)

HAP_RESULT_USE_CHECK
HAPError FirmwareUpdateProcessCommandLine(
        HAPAccessoryServer* server,
        HAPSession* _Nullable session HAP_UNUSED,
        const HAPAccessory* accessory,
        int argc,
        char** argv) {
    HAPPrecondition(argv);

    if (argc < 2) {
        HAPLog(&kHAPLog_Default, "Invalid arguments.");
        PrintUsage();
        return kHAPError_InvalidData;
    }

    const char* command = argv[0];

    // Verify this command processor was invoked correctly
    if (HAPStringAreEqual(command, kHAPDebugCommand_FirmwareUpdate) == false) {
        return kHAPError_InvalidData;
    }

    const char* operation = argv[1];
    HAPError err;

    if (HAPStringAreEqual(operation, "setHAPState")) {
        HAPAccessoryFirmwareUpdateState newState;

        if (argc < 4) {
            HAPLog(&kHAPLog_Default, "Invalid arguments.");
            PrintUsage();
            return kHAPError_InvalidData;
        }

        // Initialize new state with current accessory state
        HAPAssert(accessory->callbacks.firmwareUpdate.getAccessoryState);
        err = accessory->callbacks.firmwareUpdate.getAccessoryState(server, accessory, &newState, NULL);
        if (err) {
            return kHAPError_Unknown;
        }

        // Populate new state fields supplied via debug command
        for (int i = 2; i + 1 < argc; i += 2) {
            if (HAPStringAreEqual(argv[i], "-updateDuration")) {
                err = HAPUInt16FromString(argv[i + 1], &newState.updateDuration);
                CHECK_PARSED_VALUE(err);
            } else if (HAPStringAreEqual(argv[i], "-stagingNotReady")) {
                err = HAPUInt32FromString(argv[i + 1], &newState.stagingNotReadyReason);
                CHECK_PARSED_VALUE(err);
            } else if (HAPStringAreEqual(argv[i], "-updateNotReady")) {
                err = HAPUInt32FromString(argv[i + 1], &newState.updateNotReadyReason);
                CHECK_PARSED_VALUE(err);
            } else if (HAPStringAreEqual(argv[i], "-updateState")) {
                err = HAPUInt8FromString(argv[i + 1], &newState.updateState);
                CHECK_PARSED_VALUE(err);
            } else {
                HAPLog(&kHAPLog_Default, "Invalid option.");
                return kHAPError_InvalidData;
            }
        }

        FirmwareUpdateSetAccessoryState(server, accessory, newState);
        return kHAPError_None;
    } else if (HAPStringAreEqual(operation, "setStagingNotReadyBit")) {
        if (argc < 3) {
            HAPLog(&kHAPLog_Default, "Invalid arguments.");
            PrintUsage();
            return kHAPError_InvalidData;
        }

        uint8_t bit;
        err = HAPUInt8FromString(argv[2], &bit);
        CHECK_PARSED_VALUE(err);

        FirmwareUpdateSetStagingNotReadyReason(server, accessory, bit);
        return kHAPError_None;
    } else if (HAPStringAreEqual(operation, "clearStagingNotReadyBit")) {
        if (argc < 3) {
            HAPLog(&kHAPLog_Default, "Invalid arguments.");
            PrintUsage();
            return kHAPError_InvalidData;
        }

        uint8_t bit;
        err = HAPUInt8FromString(argv[2], &bit);
        CHECK_PARSED_VALUE(err);

        FirmwareUpdateClearStagingNotReadyReason(server, accessory, bit);
        return kHAPError_None;
    } else if (HAPStringAreEqual(operation, "setUpdateNotReadyBit")) {
        if (argc < 3) {
            HAPLog(&kHAPLog_Default, "Invalid arguments.");
            PrintUsage();
            return kHAPError_InvalidData;
        }

        uint8_t bit;
        err = HAPUInt8FromString(argv[2], &bit);
        CHECK_PARSED_VALUE(err);

        FirmwareUpdateSetUpdateNotReadyReason(server, accessory, bit);
        return kHAPError_None;
    } else if (HAPStringAreEqual(operation, "clearUpdateNotReadyBit")) {
        if (argc < 3) {
            HAPLog(&kHAPLog_Default, "Invalid arguments.");
            PrintUsage();
            return kHAPError_InvalidData;
        }

        uint8_t bit;
        err = HAPUInt8FromString(argv[2], &bit);
        CHECK_PARSED_VALUE(err);

        FirmwareUpdateClearUpdateNotReadyReason(server, accessory, bit);
        return kHAPError_None;
    } else if (HAPStringAreEqual(operation, "setApplyDelay")) {
        if (argc < 3) {
            HAPLog(&kHAPLog_Default, "Invalid arguments.");
            PrintUsage();
            return kHAPError_InvalidData;
        }

        uint16_t applyDelay;
        err = HAPUInt16FromString(argv[2], &applyDelay);
        CHECK_PARSED_VALUE(err);

        FirmwareUpdateSetApplyDelay(server, accessory, applyDelay);
        return kHAPError_None;
    } else {
        HAPLog(&kHAPLog_Default, "Unknown operation: %s", operation);
        PrintUsage();
        return kHAPError_InvalidData;
    }
}

#endif // HAVE_FIRMWARE_UPDATE

#endif // HAP_TESTING
HAP_DIAGNOSTIC_POP
