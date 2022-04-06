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

#ifndef HAP_PLATFORM_FILE_MANAGER_H
#define HAP_PLATFORM_FILE_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif


#include "HAPPlatform.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_FILE_STORAGE)

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif



HAP_RESULT_USE_CHECK
HAPError HAPPlatformFileManagerSetKV(HAPPlatformKeyValueStoreRef kv_moule);


/**
 * Creates a directory and all parent directories that don't exist.
 *
 * @param      dirPath              Path to the directory to be created.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If the file access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformFileManagerCreateDirectory(const char* dirPath);

/**
 * Writes a file atomically. If the file already exists, overwrites it.
 *
 * @param      filePath             Path to the file to be created.
 * @param      bytes                Buffer with the content of the file, if exists. numBytes != 0 implies bytes.
 * @param      numBytes             Effective length of the bytes buffer.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If the file access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformFileManagerWriteFile(const char* filePath, const void* _Nullable bytes, size_t numBytes);

/**
 * Writes a file. If the file already exists, appends to it.
 *
 * @param      filePath             Path to the file to be created.
 * @param      bytes                Buffer with the content of the file, if exists.
 * @param      numBytes             Effective length of the bytes buffer.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If the file access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformFileManagerAppendFile(const char* filePath, const void* bytes, size_t numBytes);

/**
 * Reads a file.
 *
 * @param      filePath             Path to the file to be read.
 * @param[out] bytes                Buffer for the content of the file, if exists.
 * @param      maxBytes             Capacity of the bytes buffer.
 * @param[out] numBytes             Effective length of the bytes buffer, if exists.
 * @param[out] valid                Whether the file path exists and could be read.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If the file access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformFileManagerReadFile(
        const char* filePath,
        void* _Nullable bytes,
        size_t maxBytes,
        size_t* _Nullable numBytes,
        bool* valid);

/**
 * Removes a file.
 *
 * @param      filePath             Path to the file to be removed.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If the file removal failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformFileManagerRemoveFile(const char* filePath);

/**
 * Retrieves the file size (in bytes).
 *
 * @param filePath                  Path to the file to be evaluated.
 * @param[out] fileSize             Size of the file.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If there was an error performing the operation.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformFileManagerGetFileSize(const char* filePath, int * fileSize);

/**
 * Callback that is invoked for each line of a text file.
 *
 * @param      context              Context.
 * @param      filePath             Path to the file that is being filtered.
 * @param      fileLine             Line of the file.
 *
 * @return true                     If the line should be included.
 * @return false                    If the line should be discarded.
 */
HAP_RESULT_USE_CHECK
typedef bool (*HAPPlatformFileManagerFilterTextFileCallback)(
        void* _Nullable context,
        const char* filePath,
        const char* fileLine);

/**
 * Filters a text file by applying a filter function to each line.
 *
 * - If the file does not exist, an empty file will be created.
 *
 * @param      filePath             Path to the file to be filtered.
 * @param      callback             Function to call on each line of the file.
 * @param      context              Context that is passed to the callback.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If the file access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformFileManagerFilterTextFile(
        const char* filePath,
        HAPPlatformFileManagerFilterTextFileCallback callback,
        void* _Nullable context);
#if 0
/**
 * Closes a directory with a given path and sets dir to NULL.
 *
 * @param      dir                  Directory to close (type: DIR *).
 */
#define HAPPlatformFileManagerCloseDirFreeSafe(dir) \
    do { \
        HAPAssert(dir); \
        int _e; \
        do { \
            _e = closedir(dir); \
        } while (_e == -1 && errno == EINTR); \
        if (_e) { \
            HAPAssert(_e == -1); \
        } \
        (dir) = NULL; \
    } while (0)
#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif
