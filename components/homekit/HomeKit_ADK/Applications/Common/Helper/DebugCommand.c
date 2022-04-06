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

// This debug command line serves development purposes only.
// It must not be included in production accessories and can be safely compiled out for production builds once no longer
// required for testing.

#include "HAP.h"
HAP_DIAGNOSTIC_PUSH
HAP_DIAGNOSTIC_IGNORED_GCC("-Wpedantic") // ISO C forbids an empty translation unit

#if (HAP_TESTING == 1)
#include <errno.h>
#include <string.h>
#include "AppBase.h"
#include "AppUserInput.h"
#include "ApplicationFeatures.h"
#include "DebugCommand.h"
#include "PairingDebugCommandHandler.h"

#if (HAVE_FIRMWARE_UPDATE == 1)
#include "FirmwareUpdateDebugCommandHandler.h"
#endif

static bool debugCommandIsSet = 0;
static char debugCommand[kDebugCommand_MaxBytes + 1] = { 0 };
static DebugCommandProcess appCb = NULL;

/**
 * VENDOR-TODO: ensure this file path points to a location with read/write
 * access.
 *
 * Application should ensure CreateCommandlineFile() is called
 * before DebugCommandFileMonitorStart().
 */
#define kDebugCommandDefaultFileName ".command.input"

extern HAPDataCharacteristic debugCommandCharacteristic;
extern void AppUserInputEventCallback(void* _Nullable context, size_t contextSize);
static void ParseCommandString(char*, size_t, int*, char* _Nonnull* _Nullable, size_t);

static void PrintButtonPressCommandUsage(void) {
    HAPLogInfo(
            &kHAPLog_Default,
            "\n"
            "button-press <number [1 - 4]>\n"
            "    1 Simulate clear accessory pairing on pressing button 1 or sending SIGUSR1\n"
            "    2 Simulate factory reset on pressing button 2 or sending SIGUSR2\n"
            "    3 Simulate trigger pairing mode on pressing button 3 or sending SIGTERM\n"
            "    4 Simulate custom behavior by the app on pressing button 4 or sending SIGQUIT\n");
}

/**
 * Converts signal to user input identifier
 */
static AppUserInputIdentifier ButtonNumberToUserInputId(uint8_t signum) {
    switch (signum) {
        case 1:
            return kAppUserInputIdentifier_1;
        case 2:
            return kAppUserInputIdentifier_2;
        case 3:
            return kAppUserInputIdentifier_3;
        case 4:
            return kAppUserInputIdentifier_4;
    }
    HAPLogInfo(&kHAPLog_Default, "Button number %d is ignored", signum);
    return kAppUserInputIdentifier_Invalid;
}

HAP_RESULT_USE_CHECK
static HAPError ProcessCommand(
        HAPAccessoryServer* server,
        HAPSession* session,
        const HAPAccessory* accessory,
        int argc,
        char* argv[]) {
    HAPPrecondition(argv);

    HAPError err;

    if (argc == 0) {
        return kHAPError_None;
    }

    if (appCb != NULL) {
        err = appCb(server, session, accessory, argc, argv);
        if (err == kHAPError_None) {
            // Command was process by app
            HAPLogInfo(&kHAPLog_Default, "Command processed by application");
            return err;
        } else if (err == kHAPError_InvalidData) {
            // App didn't know this command, try default commands
        } else {
            // Another error has occured, exit with error
            HAPLogInfo(&kHAPLog_Default, "Command processing failed.");
            return err;
        }
    }

    const char* command = argv[0];
    if (HAPStringAreEqual(command, "button-press")) {
        HAPLogInfo(&kHAPLog_Default, "Received command button-press");
        if (argc < 2) {
            HAPLogError(&kHAPLog_Default, "Invalid number of arguments provided");
            PrintButtonPressCommandUsage();
            return kHAPError_InvalidData;
        }
        uint8_t userInput;
        err = HAPUInt8FromString(argv[1], &userInput);
        if (err) {
            HAPLogError(&kHAPLog_Default, "Argument is of incorrect type");
            PrintButtonPressCommandUsage();
            return err;
        }

        HAPLogInfo(&kHAPLog_Default, "Triggering button press %d", userInput);
        AppUserInputEvent userInputEvent = { .id = ButtonNumberToUserInputId(userInput) };
        AppUserInputEventCallback(&userInputEvent, sizeof userInputEvent);
    } else if (HAPStringAreEqual(command, kHAPDebugCommand_Pairing)) {
        if (server && accessory) {
            return PairingProcessCommandLine(server, session, accessory, argc, argv);
        } else {
            return kHAPError_InvalidState;
        }
#if (HAVE_FIRMWARE_UPDATE == 1)
    } else if (HAPStringAreEqual(command, kHAPDebugCommand_FirmwareUpdate)) {
        if (server && accessory) {
            return FirmwareUpdateProcessCommandLine(server, session, accessory, argc, argv);
        } else {
            return kHAPError_InvalidState;
        }
#endif
    } else {
        HAPLogError(&kHAPLog_Default, "Unknown command: %s", command);
        return kHAPError_InvalidData;
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleShortDebugCommandRead(
        HAPAccessoryServer* server,
        const HAPStringCharacteristicReadRequest* request_,
        char* value,
        size_t maxValueBytes,
        void* _Nullable context) {
    HAPError err;

    if (!maxValueBytes) {
        return kHAPError_OutOfResources;
    }

    HAPDataCharacteristicReadRequest request = { .transportType = request_->transportType,
                                                 .session = request_->session,
                                                 .characteristic = &debugCommandCharacteristic,
                                                 .service = request_->service,
                                                 .accessory = request_->accessory };
    size_t numValueBytes;
    err = request.characteristic->callbacks.handleRead(
            server, &request, value, maxValueBytes - 1, &numValueBytes, context);
    if (err) {
        return err;
    }
    HAPAssert(numValueBytes < maxValueBytes);
    value[numValueBytes] = '\0';
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleShortDebugCommandWrite(
        HAPAccessoryServer* server,
        const HAPStringCharacteristicWriteRequest* request_,
        const char* value,
        void* _Nullable context) {
    HAPDataCharacteristicWriteRequest request = { .transportType = request_->transportType,
                                                  .session = request_->session,
                                                  .characteristic = &debugCommandCharacteristic,
                                                  .service = request_->service,
                                                  .accessory = request_->accessory,
                                                  .remote = request_->remote,
                                                  .authorizationData = {
                                                          .bytes = request_->authorizationData.bytes,
                                                          .numBytes = request_->authorizationData.numBytes } };
    return request.characteristic->callbacks.handleWrite(server, &request, value, HAPStringGetNumBytes(value), context);
}

HAP_RESULT_USE_CHECK
HAPError HandleDebugCommandRead(
        HAPAccessoryServer* server,
        const HAPDataCharacteristicReadRequest* request,
        void* valueBytes HAP_UNUSED,
        size_t maxValueBytes HAP_UNUSED,
        size_t* numValueBytes,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(numValueBytes);

    HAPError err;

    // Handle spurious reads (e.g., GET /accessories).
    if (!debugCommandIsSet) {
        *numValueBytes = 0;
        return kHAPError_None;
    }
    debugCommandIsSet = false;

    // Split commands into argc / argv.
    int argc = 0;
    char* argv[kDebugCommand_MaxArguments];
    ParseCommandString(debugCommand, sizeof debugCommand, &argc, argv, sizeof argv);

    // Process command line.
    err = ProcessCommand(server, request->session, request->accessory, argc, argv);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                err == kHAPError_OutOfResources);
        return kHAPError_Unknown;
    }

    // Write response.
    *numValueBytes = 0;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleDebugCommandWrite(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPDataCharacteristicWriteRequest* request HAP_UNUSED,
        const void* valueBytes,
        size_t numValueBytes,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(valueBytes);

    if (numValueBytes >= sizeof debugCommand) {
        return kHAPError_OutOfResources;
    }

    HAPAssert(!debugCommandIsSet);
    HAPRawBufferCopyBytes(debugCommand, valueBytes, numValueBytes);
    debugCommand[numValueBytes] = '\0';
    if (!HAPUTF8IsValidData(debugCommand, numValueBytes) || HAPStringGetNumBytes(debugCommand) != numValueBytes) {
        HAPLog(&kHAPLog_Default, "Malformed string value.");
        return kHAPError_InvalidData;
    }
    debugCommandIsSet = true;
    return kHAPError_None;
}

void RegisterDebugCommandCallback(DebugCommandProcess callback) {
    appCb = callback;
}

static void ParseCommandString(
        char* commandString,
        size_t commandSize,
        int* argc,
        char* _Nonnull* _Nullable argv,
        size_t maxArgs) {
    HAPAssert(argc);
    HAPAssert(argv);

    size_t commandStringIndex = 0;
    char* _Nullable argumentStart = NULL;
    char* c;
    for (c = commandString; *c && commandStringIndex < commandSize; c++) {
        if (*c == ' ') {
            if (argumentStart) {
                if ((size_t)(*argc) >= maxArgs) {
                    HAPLog(&kHAPLog_Default, "Debug Command Line has too many arguments (maximum %zu)", maxArgs);
                    return;
                }
                argv[(*argc)++] = argumentStart;
                *c = '\0';
                argumentStart = NULL;
            }
        } else {
            if (!argumentStart) {
                argumentStart = c;
            }
        }
        commandStringIndex++;
    }

    if (argumentStart) {
        if ((size_t)(*argc) >= maxArgs) {
            HAPLog(&kHAPLog_Default, "Debug Command Line has too many arguments (maximum %zu)", maxArgs);
            return;
        }
        argv[(*argc)++] = argumentStart;
        argumentStart = NULL;
    }

    // Log command.
    HAPLogInfo(&kHAPLog_Default, "argc = %d", *argc);
    for (int i = 0; i < *argc; i++) {
        HAPLogInfo(&kHAPLog_Default, "argv[%d] = %s", i, argv[i]);
    }
}

#include <stdio.h>
#include <sys/stat.h>

/**
 * Path to an externally specified command line file
 */
static char* commandFilePath = NULL;

/**
 * Changes directory the command line file is located in. Must be called before CreateCommandlineFile().
 *
 * @param filePath    The path for the command line file.
 */
void SetCommandLineFilePath(char* filePath) {
    commandFilePath = filePath;
}

/**
 * Returns the file path for the command line file.
 *
 * @return File path to be used in fopen
 */
void GetCommandLineFilePath(char* filePath, size_t maxFilePathLength) {
    HAPPrecondition(filePath);
    if (commandFilePath) {
        HAPAssert(maxFilePathLength > HAPStringGetNumBytes(commandFilePath));
        HAPRawBufferCopyBytes(filePath, commandFilePath, HAPStringGetNumBytes(commandFilePath));
        filePath[HAPStringGetNumBytes(commandFilePath)] = '\0';

    } else {
        HAPAssert(maxFilePathLength > sizeof(kDebugCommandDefaultFileName));
        HAPRawBufferCopyBytes(filePath, kDebugCommandDefaultFileName, sizeof(kDebugCommandDefaultFileName));
        filePath[sizeof(kDebugCommandDefaultFileName)] = '\0';
    }
}

void CreateCommandlineFile(void) {
    FILE* f;
    char debugCommandFileName[PATH_MAX];

    GetCommandLineFilePath((char*) &debugCommandFileName, sizeof(debugCommandFileName));
    f = fopen(debugCommandFileName, "w");
    if (f == NULL) {
        HAPLog(&kHAPLog_Default,
               "%s: Failed to create file %s (errno: %d, %s)",
               __func__,
               debugCommandFileName,
               errno,
               strerror(errno));
        HAPAssert(f);
    } else {
        HAPLog(&kHAPLog_Default, "Input command file %s created", debugCommandFileName);
        fclose(f);
        chmod(debugCommandFileName, 0777);
    }
}

void ProcessCommandsFromFile(void) {
    FILE* f;
    struct stat st_buf = { 0 };
    char debugCommandFileName[PATH_MAX];

    GetCommandLineFilePath((char*) &debugCommandFileName, sizeof(debugCommandFileName));

    if (stat(debugCommandFileName, &st_buf) != 0) {
        int _errno = errno;
        HAPLogError(&kHAPLog_Default, "stat %s has failed: %s (%d).", debugCommandFileName, strerror(_errno), _errno);
        return;
    }
    // If file is empty, no commands are in it.
    if (st_buf.st_size == 0) {
        HAPLogError(&kHAPLog_Default, "File %s is empty. No commands present.", debugCommandFileName);
        return;
    }

    f = fopen(debugCommandFileName, "r");
    if (f == NULL) {
        int _errno = errno;
        HAPLog(&kHAPLog_Default,
               "Failed to open file %s (errno: %d, %s)",
               debugCommandFileName,
               _errno,
               strerror(_errno));
        return;
    }
    char commandsString[kDebugCommand_MaxBytes] = { 0 };
    int numCommands = fscanf(f, "%[^\n]", commandsString);
    if (numCommands == -1) {
        HAPLogError(&kHAPLog_Default, "Input failure occurred before any conversion");
        return;
    } else {
        HAPLog(&kHAPLog_Default, "commands read = %s", commandsString);
    }
    fclose(f);

    // empty contents of the file
    f = fopen(debugCommandFileName, "w");
    if (f == NULL) {
        int _errno = errno;
        HAPLogError(
                &kHAPLog_Default,
                "Failed to open file %s (errno: %d, %s)",
                debugCommandFileName,
                _errno,
                strerror(_errno));
        return;
    }
    fclose(f);

    numCommands = 0;
    char* commands[kDebugCommand_MaxArguments];
    ParseCommandString(commandsString, sizeof commandsString, &numCommands, commands, sizeof commands);

    if (numCommands == 0) {
        HAPLog(&kHAPLog_Default, "No commands in command string");
        return;
    }

    // Process command line.
    HAPError err = ProcessCommand(AppGetAccessoryServer(), NULL, AppGetAccessoryInfo(), numCommands, commands);
    if (err) {
        HAPLog(&kHAPLog_Default, "Failed to process input commands");
    }
}

void ADKExecuteDebugCommand(char* commandString) {
    char* commandsArray[1];
    commandsArray[0] = commandString;
    HAPError err = ProcessCommand(AppGetAccessoryServer(), NULL, AppGetAccessoryInfo(), 1, commandsArray);
    HAPAssert(!err);
}

#endif // HAP_TESTING
HAP_DIAGNOSTIC_POP
