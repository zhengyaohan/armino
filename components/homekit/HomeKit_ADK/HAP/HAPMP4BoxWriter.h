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

#ifndef HAP_MP4_BOX_WRITER_H
#define HAP_MP4_BOX_WRITER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPBase.h"
#include "HAPPlatformFeatures.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * MP4 box writer.
 */
typedef struct HAPMP4BoxWriter {
    uint8_t* _Nullable start;                    // Start of box, address of size field.
    uint8_t* _Nullable position;                 // Actual writer position.
    uint8_t* _Nullable end;                      // End of buffer.
    struct HAPMP4BoxWriter* _Nullable container; // Container if local writer.
} HAPMP4BoxWriter;

/**
 * Opens an MP4 box writer on a buffer.
 *
 * @param[out] writer               The writer to initialize.
 * @param      bytes                The buffer to write to.
 * @param      maxBytes             The capacity of the buffer.
 * @param      type                 The box type (4 characters).
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to open the writer.
 */
HAP_RESULT_USE_CHECK
HAPError HAPMP4BoxWriterOpen(HAPMP4BoxWriter* writer, void* bytes, size_t maxBytes, const char* type);

/**
 * Opens an MP4 box writer local to an existing writer.
 *
 * @param[out] writer               The writer to initialize.
 * @param      container            The writer to append the new box to.
 * @param      type                 The box type (4 characters).
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to open the writer.
 */
HAP_RESULT_USE_CHECK
HAPError HAPMP4BoxWriterOpenLocal(HAPMP4BoxWriter* writer, HAPMP4BoxWriter* container, const char* type);

/**
 * Closes and invalidates an MP4 box writer.
 *
 * @param      writer               The writer to close.
 * @param[out] numBytes             The number of bytes written.
 */
void HAPMP4BoxWriterClose(HAPMP4BoxWriter* writer, size_t* _Nullable numBytes);

/**
 * Appends a full box header to an MP4 box.
 *
 * @param      writer               The writer.
 * @param      version              The version of the full box.
 * @param      flags                The flags of the full box.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the writer's buffer is not large enough to hold the data.
 */
HAP_RESULT_USE_CHECK
HAPError HAPMP4BoxWriterAppendFullBoxHeader(HAPMP4BoxWriter* writer, uint8_t version, uint32_t flags);

/**
 * Appends a 1 byte integer to an MP4 box.
 *
 * @param      writer               The writer.
 * @param      value                The value to append to the box.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the writer's buffer is not large enough to hold the data.
 */
HAP_RESULT_USE_CHECK
HAPError HAPMP4BoxWriterAppendUInt8(HAPMP4BoxWriter* writer, uint8_t value);

/**
 * Appends a 2 byte integer to an MP4 box.
 *
 * @param      writer               The writer.
 * @param      value                The value to append to the box.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the writer's buffer is not large enough to hold the data.
 */
HAP_RESULT_USE_CHECK
HAPError HAPMP4BoxWriterAppendUInt16(HAPMP4BoxWriter* writer, uint16_t value);

/**
 * Appends a 4 byte integer to an MP4 box.
 *
 * @param      writer               The writer.
 * @param      value                The value to append to the box.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the writer's buffer is not large enough to hold the data.
 */
HAP_RESULT_USE_CHECK
HAPError HAPMP4BoxWriterAppendUInt32(HAPMP4BoxWriter* writer, uint32_t value);

/**
 * Appends a 8 byte integer to an MP4 box.
 *
 * @param      writer               The writer.
 * @param      value                The value to append to the box.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the writer's buffer is not large enough to hold the data.
 */
HAP_RESULT_USE_CHECK
HAPError HAPMP4BoxWriterAppendUInt64(HAPMP4BoxWriter* writer, uint64_t value);

/**
 * Appends a number of bytes to an MP4 box.
 *
 * @param      writer               The writer.
 * @param      bytes                The bytes to append to the box.
 * @param      numBytes             The number of bytes to append.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the writer's buffer is not large enough to hold the data.
 */
HAP_RESULT_USE_CHECK
HAPError HAPMP4BoxWriterAppendBytes(HAPMP4BoxWriter* writer, const void* bytes, size_t numBytes);

/**
 * Appends a number of zero bytes to an MP4 box.
 *
 * @param      writer               The writer.
 * @param      numBytes             The number of bytes to append.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the writer's buffer is not large enough to hold the data.
 */
HAP_RESULT_USE_CHECK
HAPError HAPMP4BoxWriterAppendZero(HAPMP4BoxWriter* writer, size_t numBytes);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif
