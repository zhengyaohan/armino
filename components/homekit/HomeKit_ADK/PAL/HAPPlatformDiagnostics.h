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

#ifndef HAP_PLATFORM_FILE_DIAGNOSTICS_H
#define HAP_PLATFORM_FILE_DIAGNOSTICS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "HAP.h"
#include "HAPDiagnostics.h"

#include "HAPPlatform.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)

#define kHAPPlatformDiagnosticsFolderPermissions 0766
#define kHAPPlatformDiagnosticsZipFileName       "diagnostics_upload.zip"
#define kHAPPlatformDiagnosticsTextFileName      "diagnostics_upload.txt"
#define kHAPPlatformDiagnosticsZipUtility        "/usr/bin/zip"
#define kHAPPlatformDiagnosticsNumLogFiles       10
#define kHAPPlatformDiagnosticsFolder            "Diagnostics"
#define kHAPPlatformDiagnosticsADKLogsFolder     "Diagnostics/ADK"
#define kHAPPlatformDiagnosticsAudioLogsFolder   "Diagnostics/Audio"
#define kHAPPlatformDiagnosticsBTLogsFolder      "Diagnostics/Bluetooth"
#define kHAPPlatformDiagnosticsCrashLogFile      "Diagnostics/ADK/crash.log"
#define kHAPPlatformDiagnosticsConfigFile        "Diagnostics/ADK/accessory_configuration.log"
#define kHAPPlatformDiagnosticsDmesgFile         "Diagnostics/ADK/dmesg.log"

/**
 * Initialize Diagnostics
 * This function creates diagnostics folders and starts capture
 * of PAL specific diagnostics
 *
 * @param config         Diagnostics configuration
 */
void HAPPlatformDiagnosticsInitialize(HAPAccessoryDiagnosticsConfig* _Nonnull config);

/**
 * Deinitialize Diagnostics
 * This function stops capture of PAL specific diagnostics
 */
void HAPPlatformDiagnosticsDeinitialize(void);

/**
 * Start capturing HAP log messages to file
 *
 * @param  folderName                     Folder where HAP logs will be saved.
 * @param  logFileName                    File name for HAP logs.
 * @param  maxLogFileSizeBytes            Maximum size of HAP logs.
 * @param  logContext                     HAP Diagnostics log context pointer.
 *
 * @return kHAPError_None                 If successful.
 * @return kHAPError_InvalidData          If configuration data is invalid.
 * @return kHAPError_Busy                 If log capture is in progress.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformDiagnosticsStartLogCaptureToFile(
        const char* _Nonnull folderName,
        const char* _Nonnull logFileName,
        const size_t maxLogFileSizeBytes,
        DiagnosticsLog* _Nonnull logContext);

/**
 * Stop capturing HAP log messages to file
 *
 * @param  logContext      HAP Diagnostics log context pointer.
 */
void HAPPlatformDiagnosticsStopLogCaptureToFile(DiagnosticsLog* _Nonnull logContext);

/**
 * Write HAP log messages to file
 *
 * @param      disableLogRotation  Do not perform log rotation.
 * @param      format              Message format.
 * @param      va_list             Variable number of arguments based on format
 */
HAP_PRINTFLIKE(2, 3)
void HAPPlatformDiagnosticsWriteToFile(bool disableLogRotation, const char* _Nullable format, ...);

/**
 * Write HAP log messages to diagnostics circular log buffer
 *
 * @param      format              Message format.
 * @param      va_list             Variable number of arguments based on format
 */
HAP_PRINTFLIKE(1, 2)
void HAPPlatformDiagnosticsWriteToLogBuffer(const char* _Nullable format, ...);

/**
 * Clear Crash log file
 */
void HAPPlatformDiagnosticsClearCrashLog(void);

/**
 * Flush diagnostics log file
 */
void HAPPlatformDiagnosticsFlushLog(void);

/**
 * Write crash log message
 *
 * @param      format       Message format.
 * @param      va_list      Variable number of arguments based on format
 */
HAP_PRINTFLIKE(1, 2)
void HAPPlatformDiagnosticsWriteToCrashLog(const char* _Nullable format, ...);

/**
 * Write accessory configuration log message
 *
 * @param      format       Message format.
 * @param      va_list      Variable number of arguments based on format
 */
HAP_PRINTFLIKE(1, 2)
void HAPPlatformDiagnosticsWriteToConfigLog(const char* _Nullable format, ...);

/**
 * Get the bytes to upload via HDS from Diagnostics logs
 *
 * @param[in]  buf                            Data read buffer.
 * @param      bufSize                        Data read buffer size in bytes.
 * @param[out] bytesCopied                    Bytes copied to buf.
 * @param[out] isEOF                          End of file flag.
 * @param[in]  prepareDiagnosticsDataContext  Prepare data context pointer.
 *
 * @return kHAPError_None             If successful.
 * @return kHAPError_Unknown          If any other error.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformDiagnosticsGetBytesToUpload(
        uint8_t* _Nonnull buf,
        size_t bufSize,
        size_t* _Nonnull bytesCopied,
        bool* _Nonnull isEOF,
        void* _Nonnull prepareDiagnosticsDataContext);

/**
 * Prepares the diagnostics data in a separate thread or process.
 * Return from this function immediately to prevent blocking main thread.
 *
 * @param  diagnosticsCallbacks  Callback functions
 * @param  metadata              HDS open request metadata
 *
 * @return prepareDiagnosticsDataContext  If successful.
 * @return NULL                           If any other error.
 */
void* _Nullable HAPPlatformDiagnosticsPrepareData(
        const HAPDiagnosticsCallbacks* _Nonnull diagnosticsCallbacks,
        const HAPDataSendDataStreamProtocolOpenMetadata* _Nonnull metadata);

/**
 * Called by HAP when diagnostics data transfer to controller is complete.
 *
 * @param prepareDiagnosticsDataContext    Prepare data context pointer
 */
void HAPPlatformDiagnosticsDataTransferComplete(void* _Nullable prepareDiagnosticsDataContext);

/**
 * Called by HAP when diagnostics data transfer to controller is aborted.
 *
 * @param prepareDiagnosticsDataContext    Prepare data context pointer
 */
void HAPPlatformDiagnosticsAbort(void* _Nullable prepareDiagnosticsDataContext);

#endif // HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
