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

#include "HAPMP4BoxWriter.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)

/*
 * Basic box Structure:
 * From ISO/IEC 14496-12
 *
 * class Box {
 *     int(32) size; // Number of bytes in box, including size and type.
 *     int(32) type; // Box type, normally 4 printable characters.
 * }
 *
 * class FullBox extends Box {
 *     unsigned int(8) version; // Version of the format of the box.
 *     bit(24)         flags;   // Flags with options for the box.
 * }
 */

HAP_RESULT_USE_CHECK
HAPError HAPMP4BoxWriterOpen(HAPMP4BoxWriter* writer, void* bytes, size_t maxBytes, const char* type) {
    HAPPrecondition(writer);
    HAPPrecondition(bytes);
    HAPPrecondition(type);

    if (maxBytes < 8) {
        return kHAPError_OutOfResources;
    }

    // Initialize writer.
    HAPRawBufferZero(writer, sizeof *writer);
    writer->start = (uint8_t*) bytes;
    writer->position = (uint8_t*) bytes + 4; // skip size field
    writer->end = (uint8_t*) bytes + maxBytes;

    // Write type.
    return HAPMP4BoxWriterAppendBytes(writer, type, 4);
}

HAP_RESULT_USE_CHECK
HAPError HAPMP4BoxWriterOpenLocal(HAPMP4BoxWriter* writer, HAPMP4BoxWriter* container, const char* type) {
    HAPPrecondition(writer);
    HAPPrecondition(container);
    HAPPrecondition(type);

    if (container->end - container->position < 8) {
        return kHAPError_OutOfResources;
    }

    // Initialize writer.
    HAPRawBufferZero(writer, sizeof *writer);
    writer->start = container->position;
    writer->position = container->position + 4; // skip size field
    writer->end = container->end;
    writer->container = container;

    // Write type.
    return HAPMP4BoxWriterAppendBytes(writer, type, 4);
}

void HAPMP4BoxWriterClose(HAPMP4BoxWriter* writer, size_t* _Nullable numBytes) {
    HAPPrecondition(writer);
    HAPPrecondition(writer->start);

    // Fix up size field.
    size_t size = (size_t)(writer->position - writer->start);
    HAPAssert(size <= UINT32_MAX);
    HAPWriteBigUInt32(writer->start, (uint32_t) size);

    // Update container.
    HAPMP4BoxWriter* container = writer->container;
    if (container) {
        HAPAssert(container->position == writer->start);
        HAPAssert(writer->position <= container->end);
        container->position = writer->position;
    }
    // Invalidate writer.
    HAPRawBufferZero(writer, sizeof *writer);

    if (numBytes) {
        *numBytes = size;
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPMP4BoxWriterAppendFullBoxHeader(HAPMP4BoxWriter* writer, uint8_t version, uint32_t flags) {
    HAPPrecondition(writer);
    HAPPrecondition(writer->position);
    HAPPrecondition(flags <= 0x00FFFFFF);

    if ((size_t)(writer->end - writer->position) < 4) {
        return kHAPError_OutOfResources;
    }
    *writer->position++ = version;
    HAPWriteBigUInt24(writer->position, flags);
    writer->position += 3;

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPMP4BoxWriterAppendUInt8(HAPMP4BoxWriter* writer, uint8_t value) {
    HAPPrecondition(writer);
    HAPPrecondition(writer->position);

    if ((size_t)(writer->end - writer->position) < sizeof value) {
        return kHAPError_OutOfResources;
    }
    *writer->position++ = value;

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPMP4BoxWriterAppendUInt16(HAPMP4BoxWriter* writer, uint16_t value) {
    HAPPrecondition(writer);
    HAPPrecondition(writer->position);

    if ((size_t)(writer->end - writer->position) < sizeof value) {
        return kHAPError_OutOfResources;
    }
    HAPWriteBigUInt16(writer->position, value);
    writer->position += sizeof value;

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPMP4BoxWriterAppendUInt32(HAPMP4BoxWriter* writer, uint32_t value) {
    HAPPrecondition(writer);
    HAPPrecondition(writer->position);

    if ((size_t)(writer->end - writer->position) < sizeof value) {
        return kHAPError_OutOfResources;
    }
    HAPWriteBigUInt32(writer->position, value);
    writer->position += sizeof value;

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPMP4BoxWriterAppendUInt64(HAPMP4BoxWriter* writer, uint64_t value) {
    HAPPrecondition(writer);
    HAPPrecondition(writer->position);

    if ((size_t)(writer->end - writer->position) < sizeof value) {
        return kHAPError_OutOfResources;
    }
    HAPWriteBigUInt64(writer->position, value);
    writer->position += sizeof value;

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPMP4BoxWriterAppendBytes(HAPMP4BoxWriter* writer, const void* bytes, size_t numBytes) {
    HAPPrecondition(writer);
    HAPPrecondition(writer->position);
    HAPPrecondition(bytes);

    if ((size_t)(writer->end - writer->position) < numBytes) {
        return kHAPError_OutOfResources;
    }
    HAPRawBufferCopyBytes(HAPNonnull(writer->position), bytes, numBytes);
    writer->position += numBytes;

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPMP4BoxWriterAppendZero(HAPMP4BoxWriter* writer, size_t numBytes) {
    HAPPrecondition(writer);
    HAPPrecondition(writer->position);

    if ((size_t)(writer->end - writer->position) < numBytes) {
        return kHAPError_OutOfResources;
    }
    HAPRawBufferZero(HAPNonnull(writer->position), numBytes);
    writer->position += numBytes;

    return kHAPError_None;
}

#endif
