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

#include "HAP.h"
HAP_DIAGNOSTIC_PUSH
HAP_DIAGNOSTIC_IGNORED_GCC("-Wpedantic") // ISO C forbids an empty translation unit

#if (HAP_TESTING == 1)
#if (HAVE_ACCESS_CODE == 1)
#include "AccessCodeHelper.h"
#endif
#if (HAVE_NFC_ACCESS == 1)
#include "HAPPlatformNfcAccess.h"
#endif
#include "App.h"

HAP_RESULT_USE_CHECK
HAPError ProcessCommandLine(
        HAPAccessoryServer* server HAP_UNUSED,
        HAPSession* session HAP_UNUSED,
        const HAPAccessory* accessory HAP_UNUSED,
        int argc,
        char** argv) {
    HAPPrecondition(argv);

    // Command
    const char* command = argv[0];

    if (HAPStringAreEqual(command, "setCurrentState")) {
        if (argc != 2) {
            HAPLog(&kHAPLog_Default, "Usage: %s [secured|unsecured|jammed|unknown]", command);
            return kHAPError_InvalidData;
        }

        const char* operation = argv[1];

        HAPCharacteristicValue_LockCurrentState state = kHAPCharacteristicValue_LockCurrentState_Unknown;
        if (HAPStringAreEqual(operation, "secured")) {
            state = kHAPCharacteristicValue_LockCurrentState_Secured;
        } else if (HAPStringAreEqual(operation, "unsecured")) {
            state = kHAPCharacteristicValue_LockCurrentState_Unsecured;
        } else if (HAPStringAreEqual(operation, "jammed")) {
            state = kHAPCharacteristicValue_LockCurrentState_Jammed;
        } else if (HAPStringAreEqual(operation, "unknown")) {
            state = kHAPCharacteristicValue_LockCurrentState_Unknown;
        } else {
            HAPLog(&kHAPLog_Default, "Unknown %s operation: %s", command, operation);
            return kHAPError_InvalidData;
        }

        SetLockCurrentState(state);
        return kHAPError_None;
    }

    if (HAPStringAreEqual(command, "toggleLockState")) {
        if (argc != 1) {
            HAPLog(&kHAPLog_Default, "Usage: %s", command);
            return kHAPError_InvalidData;
        }

        ToggleLockState();
        return kHAPError_None;
    }

    if (HAPStringAreEqual(command, "toggleLockJammed")) {
        if (argc != 1) {
            HAPLog(&kHAPLog_Default, "Usage: %s", command);
            return kHAPError_InvalidData;
        }

        ToggleLockJammed();
        return kHAPError_None;
    }

    if (HAPStringAreEqual(command, "batteryStatus")) {
        if (argc != 2) {
            HAPLog(&kHAPLog_Default, "Usage: %s [normal|low]", command);
            return kHAPError_InvalidData;
        }
        const char* state = argv[1];

        if (HAPStringAreEqual(state, "normal")) {
            SetStatusLowBatteryState(kHAPCharacteristicValue_StatusLowBattery_Normal);
        } else if (HAPStringAreEqual(state, "low")) {
            SetStatusLowBatteryState(kHAPCharacteristicValue_StatusLowBattery_Low);
        } else {
            HAPLog(&kHAPLog_Default, "Unknown %s state: %s", command, state);
            return kHAPError_InvalidData;
        }

        return kHAPError_None;
    }

#if (HAVE_ACCESS_CODE == 1)
    if (HAPStringAreEqual(command, "accessCode")) {
        if (argc < 2) {
            HAPLog(&kHAPLog_Default, "Usage: %s [keypad|supportedConfig]", command);
            return kHAPError_InvalidData;
        }

        const char* operation = argv[1];
        if (HAPStringAreEqual(operation, "keypad")) {
            if (argc != 3) {
                HAPLog(&kHAPLog_Default, "Usage: %s %s <value>", command, operation);
                return kHAPError_InvalidData;
            }

            ValidateAccessCode(argv[2]);
            return kHAPError_None;
        } else if (HAPStringAreEqual(operation, "supportedConfig")) {
            if (argc != 4) {
                HAPLog(&kHAPLog_Default, "Usage: %s %s <name> <value>", command, operation);
                return kHAPError_InvalidData;
            }

            const char* name = argv[2];
            if (HAPStringAreEqual(name, "characterSet")) {
                uint8_t value;
                if (HAPUInt8FromString(argv[3], &value) != kHAPError_None) {
                    HAPLog(&kHAPLog_Default, "Usage: %s %s %s <uint8>", command, operation, name);
                    return kHAPError_InvalidData;
                }

                if (AccessCodeSetCharacterSet(value) != kHAPError_None) {
                    HAPLog(&kHAPLog_Default, "Cannot set access code character set");
                    return kHAPError_InvalidData;
                }
            } else if (HAPStringAreEqual(name, "minimumLength")) {
                uint8_t value;
                if (HAPUInt8FromString(argv[3], &value) != kHAPError_None) {
                    HAPLog(&kHAPLog_Default, "Usage: %s %s %s <uint8>", command, operation, name);
                    return kHAPError_InvalidData;
                }

                if (AccessCodeSetMinimumLength(value) != kHAPError_None) {
                    HAPLog(&kHAPLog_Default, "Cannot set access code minimum length");
                    return kHAPError_InvalidData;
                }
            } else if (HAPStringAreEqual(name, "maximumLength")) {
                uint8_t value;
                if (HAPUInt8FromString(argv[3], &value) != kHAPError_None) {
                    HAPLog(&kHAPLog_Default, "Usage: %s %s %s <uint8>", command, operation, name);
                    return kHAPError_InvalidData;
                }

                if (AccessCodeSetMaximumLength(value) != kHAPError_None) {
                    HAPLog(&kHAPLog_Default, "Cannot set access code maximum length");
                    return kHAPError_InvalidData;
                }
            } else if (HAPStringAreEqual(name, "maximumAccessCodes")) {
                uint16_t value;
                if (HAPUInt16FromString(argv[3], &value) != kHAPError_None) {
                    HAPLog(&kHAPLog_Default, "Usage: %s %s %s <uint16>", command, operation, name);
                    return kHAPError_InvalidData;
                }

                if (AccessCodeSetMaximumAccessCodes(value) != kHAPError_None) {
                    HAPLog(&kHAPLog_Default, "Cannot set access code maximum number");
                    return kHAPError_InvalidData;
                }
            } else {
                HAPLog(&kHAPLog_Default,
                       "Usage: %s %s "
                       "[characterSet|minimumLength|maximumLength|maximumAccessCodes]",
                       command,
                       operation);
                return kHAPError_InvalidData;
            }
            return kHAPError_None;
        }
    }
#endif

#if (HAVE_NFC_ACCESS == 1)
    if (HAPStringAreEqual(command, "nfcAccess")) {
        if (argc < 2) {
            HAPLog(&kHAPLog_Default, "Usage: %s [addDeviceCredential|supportedConfig]", command);
            return kHAPError_InvalidData;
        }

        const char* operation = argv[1];
        if (HAPStringAreEqual(operation, "addDeviceCredential")) {
            if (argc != 5) {
                HAPLog(&kHAPLog_Default, "Usage: %s %s <key> <key_index> <issuer_key_identifier>", command, operation);
                return kHAPError_InvalidData;
            }

            HAPError err;

            // Get the device credential key
            const char* keyString = argv[2];
            HAPAssert((HAPStringGetNumBytes(keyString) / 2) == NIST256_PUBLIC_KEY_BYTES);
            uint8_t keyByteBuffer[NIST256_PUBLIC_KEY_BYTES];
            err = HAPUInt8BufferFromHexString(keyString, NIST256_PUBLIC_KEY_BYTES, keyByteBuffer);
            if (err != kHAPError_None) {
                HAPLog(&kHAPLog_Default, "Cannot convert device credential key");
                return err;
            }

            // Get the issuer key identifier
            const char* identifierString = argv[4];
            HAPAssert((HAPStringGetNumBytes(identifierString) / 2) == NFC_ACCESS_KEY_IDENTIFIER_BYTES);
            uint8_t identifierByteBuffer[NFC_ACCESS_KEY_IDENTIFIER_BYTES];
            err = HAPUInt8BufferFromHexString(identifierString, NFC_ACCESS_KEY_IDENTIFIER_BYTES, identifierByteBuffer);
            if (err != kHAPError_None) {
                HAPLog(&kHAPLog_Default, "Cannot convert issuer key identifier");
                return err;
            }

            HAPPlatformNfcAccessDeviceCredentialKey deviceCredentialKey = {
                .type = kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256,
                .key = keyByteBuffer,
                .keyNumBytes = NIST256_PUBLIC_KEY_BYTES,
                .issuerKeyIdentifier = identifierByteBuffer,
                .state = kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Active,
            };
            NfcAccessStatusCode statusCode;
            err = HAPPlatformNfcAccessDeviceCredentialKeyAdd(&deviceCredentialKey, &statusCode);
            if ((err != kHAPError_None) || (statusCode != NFC_ACCESS_STATUS_CODE_SUCCESS)) {
                HAPLog(&kHAPLog_Default, "Cannot add device credential key");
                return err;
            }

            // Verify the index of the added device credential key matches the expected value
            uint16_t index;
            err = FindDeviceCredentialKeyIndex(keyByteBuffer, &index);
            if (err != kHAPError_None) {
                HAPLog(&kHAPLog_Default, "Cannot find device credential key");
                return err;
            }
            uint16_t expectedIndex;
            err = HAPUInt16FromString(argv[3], &expectedIndex);
            if (err != kHAPError_None) {
                HAPLog(&kHAPLog_Default, "Cannot convert device credential key index");
                return err;
            }
            if (expectedIndex != index) {
                HAPLog(&kHAPLog_Default, "Key added has index=%d, expected=%d", index, expectedIndex);
            }
            return kHAPError_None;
        } else if (HAPStringAreEqual(operation, "supportedConfig")) {
            if (argc != 4) {
                HAPLog(&kHAPLog_Default, "Usage: %s %s <name> <value>", command, operation);
                return kHAPError_InvalidData;
            }

            const char* name = argv[2];
            if (HAPStringAreEqual(name, "maximumIssuerKeys")) {
                uint16_t value;
                if (HAPUInt16FromString(argv[3], &value) != kHAPError_None) {
                    HAPLog(&kHAPLog_Default, "Usage: %s %s %s <uint16>", command, operation, name);
                    return kHAPError_InvalidData;
                }

                if (HAPPlatformNfcAccessSetMaximumIssuerKeys(value) != kHAPError_None) {
                    HAPLog(&kHAPLog_Default, "Cannot set maximum issuer keys");
                    return kHAPError_InvalidData;
                }
            } else if (HAPStringAreEqual(name, "maximumSuspendedDeviceCredentialKeys")) {
                uint16_t value;
                if (HAPUInt16FromString(argv[3], &value) != kHAPError_None) {
                    HAPLog(&kHAPLog_Default, "Usage: %s %s %s <uint16>", command, operation, name);
                    return kHAPError_InvalidData;
                }

                if (HAPPlatformNfcAccessSetMaximumSuspendedDeviceCredentialKeys(value) != kHAPError_None) {
                    HAPLog(&kHAPLog_Default, "Cannot set maximum suspended device credential keys");
                    return kHAPError_InvalidData;
                }
            } else if (HAPStringAreEqual(name, "maximumActiveDeviceCredentialKeys")) {
                uint16_t value;
                if (HAPUInt16FromString(argv[3], &value) != kHAPError_None) {
                    HAPLog(&kHAPLog_Default, "Usage: %s %s %s <uint16>", command, operation, name);
                    return kHAPError_InvalidData;
                }

                if (HAPPlatformNfcAccessSetMaximumActiveDeviceCredentialKeys(value) != kHAPError_None) {
                    HAPLog(&kHAPLog_Default, "Cannot set maximum active device credential keys");
                    return kHAPError_InvalidData;
                }
            } else {
                HAPLog(&kHAPLog_Default,
                       "Usage: %s %s "
                       "[maximumIssuerKeys|maximumSuspendedDeviceCredentialKeys|maximumActiveDeviceCredentialKeys]",
                       command,
                       operation);
                return kHAPError_InvalidData;
            }
            return kHAPError_None;
        }
    }
#endif

    HAPLog(&kHAPLog_Default, "Unknown command: %s", command);
    return kHAPError_InvalidData;
}
#endif

HAP_DIAGNOSTIC_POP
