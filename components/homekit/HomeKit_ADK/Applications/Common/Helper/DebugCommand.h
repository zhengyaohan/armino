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

#ifndef DEBUG_COMMAND_H
#define DEBUG_COMMAND_H

#if (HAP_TESTING == 1)
#ifdef __cplusplus
extern "C" {
#endif

#include "HAP.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/** Maximum number of command line arguments. */
#define kDebugCommand_MaxArguments ((size_t)(64))

typedef HAPError (*DebugCommandProcess)(
        HAPAccessoryServer* server,
        HAPSession* session,
        const HAPAccessory* accessory,
        int argc,
        char* _Nullable argv[_Nullable]);

/** Maximum length of a command. */
#define kDebugCommand_MaxBytes ((size_t)(8 * 1024 - 1))

/**
 * Register callback for application debug command processing
 */
void RegisterDebugCommandCallback(DebugCommandProcess callback);

/**
 * Changes directory the command line file is located in. Must be called before CreateCommandlineFile().
 *
 * @param filePath    The path for the command line file.
 */
void SetCommandLineFilePath(char* filePath);

void CreateCommandlineFile(void);

void ProcessCommandsFromFile(void);

/**
 * Returns the file path for the command line file.
 *
 * @return File path to be used in fopen
 */
void GetCommandLineFilePath(char* filePath, size_t maxFilePathLength);

/**
 * Execute a debug command as if the string had been inserted into the debug command line file
 *
 * @param       commandString       The debug command to process
 */
void ADKExecuteDebugCommand(char* commandString);
/**
 * Handle read request to the 'Short Debug Command Line' characteristic of the Accessory Information service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleShortDebugCommandRead(
        HAPAccessoryServer* server,
        const HAPStringCharacteristicReadRequest* request,
        char* value,
        size_t maxValueBytes,
        void* _Nullable context);

/**
 * Handle write request to the 'Short Debug Command Line' characteristic of the Accessory Information service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleShortDebugCommandWrite(
        HAPAccessoryServer* server,
        const HAPStringCharacteristicWriteRequest* request,
        const char* value,
        void* _Nullable context);

/**
 * Handle read request to the 'Debug Command Line' characteristic of the Accessory Information service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleDebugCommandRead(
        HAPAccessoryServer* server,
        const HAPDataCharacteristicReadRequest* request,
        void* valueBytes,
        size_t maxValueBytes,
        size_t* numValueBytes,
        void* _Nullable context);

/**
 * Handle write request to the 'Debug Command Line' characteristic of the Accessory Information service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleDebugCommandWrite(
        HAPAccessoryServer* server,
        const HAPDataCharacteristicWriteRequest* request,
        const void* valueBytes,
        size_t numValueBytes,
        void* _Nullable context);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif // HAP_TESTING
#endif // DEBUG_COMMAND_H
