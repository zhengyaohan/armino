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

#include "HAPLogSubsystem.h"
#include "HAPTLV+Internal.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "TLVWriter" };

void HAPTLVWriterCreate(HAPTLVWriter* writer, void* bytes, size_t maxBytes) {
    HAPPrecondition(writer);
    HAPPrecondition(bytes);

    // Initialize writer.
    HAPRawBufferZero(writer, sizeof *writer);
    writer->bytes = bytes;
    writer->maxBytes = maxBytes;
    writer->numBytes = 0;
    writer->lastType = 0;
}

void HAPTLVCreateSubWriter(HAPTLVWriter* subWriter, HAPTLVWriter* outerWriter) {
    void* bytes;
    size_t maxBytes;
    HAPTLVWriterGetScratchBytesForTLVValue(outerWriter, &bytes, &maxBytes);

    // The smallest inner TLV supported would be 2 bytes (a 0-byte value).
    // If we can't satisfy that, then give nothing.
    if (maxBytes < 2) {
        maxBytes = 0;
    }

    HAPTLVWriterCreate(subWriter, bytes, maxBytes);
}

void HAPTLVWriterReset(HAPTLVWriter* writer) {
    HAPPrecondition(writer);
    writer->numBytes = 0;
    writer->lastType = 0;
}

HAP_RESULT_USE_CHECK
HAPError HAPTLVWriterAppend(HAPTLVWriter* writer, const HAPTLV* tlv) {
    HAPPrecondition(writer);
    HAPPrecondition(tlv);
    if (!tlv->value.bytes) {
        HAPPrecondition(!tlv->value.numBytes);
    }
    if (writer->numBytes) {
        HAPPrecondition(tlv->type != writer->lastType);
    }

    uint8_t* destinationBytes = HAPNonnullVoid(writer->bytes);
    size_t maxDestinationBytes = writer->maxBytes - writer->numBytes;

    const uint8_t* _Nullable valueBytes = tlv->value.bytes;
    size_t numValueBytes = tlv->value.numBytes;

    // Serialize TLV, fragment by fragment.
    do {
        size_t numFragmentBytes = numValueBytes > UINT8_MAX ? UINT8_MAX : numValueBytes;

        // Consume space needed for header.
        if (maxDestinationBytes < 2) {
            // TLV header does not fit into buffer.
            HAPLog(&logObject, "Not enough memory to write TLV header.");
            return kHAPError_OutOfResources;
        }
        maxDestinationBytes -= 2;

        if (valueBytes) {
            // Since the memory after serialized TLV data may have been used by the client,
            // move that data to accommodate the TLV header.
            if (maxDestinationBytes < numValueBytes) {
                // Value does not fit into buffer.
                HAPLog(&logObject, "Not enough memory to write TLV value.");
                return kHAPError_OutOfResources;
            }
            // Entire remaining value is copied, including followup fragments.
            HAPRawBufferCopyBytes(&destinationBytes[writer->numBytes + 2], HAPNonnull(valueBytes), numValueBytes);
            valueBytes = &destinationBytes[writer->numBytes + 2];
            maxDestinationBytes -= numFragmentBytes;
            numValueBytes -= numFragmentBytes;
            valueBytes += numFragmentBytes;
        } else {
            HAPAssert(!numValueBytes);
        }

        // Serialize fragment.
        destinationBytes[writer->numBytes++] = (uint8_t) tlv->type;
        destinationBytes[writer->numBytes++] = (uint8_t) numFragmentBytes;
        writer->numBytes += numFragmentBytes;
    } while (numValueBytes);

    writer->lastType = tlv->type;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPTLVWriterExtend(HAPTLVWriter* writer, void* _Nonnull bytes, size_t numBytes, HAPTLVType lastType) {
    HAPPrecondition(writer);
    HAPPrecondition(bytes);

    if (numBytes == 0) {
        return kHAPError_None;
    }

    size_t maxDestinationBytes = writer->maxBytes - writer->numBytes;
    if (maxDestinationBytes < numBytes) {
        // Cached TLVs does not fit into buffer
        HAPLog(&logObject, "Not enough memory to copy TLVs.");
        return kHAPError_OutOfResources;
    }

    HAPPrecondition(writer->bytes);
    HAPRawBufferCopyBytes(&((uint8_t*) writer->bytes)[writer->numBytes], bytes, numBytes);
    writer->numBytes += numBytes;
    writer->lastType = lastType;
    return kHAPError_None;
}

void HAPTLVWriterGetBuffer(const HAPTLVWriter* writer, void* _Nonnull* _Nonnull bytes, size_t* numBytes) {
    HAPPrecondition(writer);
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes);

    *bytes = writer->bytes;
    *numBytes = writer->numBytes;
}

void HAPTLVWriterPrintBuffer(const HAPTLVWriter* writer, char* writerName) {
    HAPPrecondition(writer);
    HAPPrecondition(writerName);

    HAPStringBuilder stringBuilder;
    size_t bufSize = 256;
    char printBuffer[bufSize];
    HAPStringBuilderCreate(&stringBuilder, printBuffer, bufSize);
    for (size_t index = 0; index < writer->numBytes; index++) {
        HAPStringBuilderAppend(&stringBuilder, " %02X", ((uint8_t*) writer->bytes)[index]);
    }
    HAPLogDebug(&logObject, "\n%s: %s", writerName, HAPStringBuilderGetString(&stringBuilder));
}

void HAPTLVWriterGetScratchBytes(
        const HAPTLVWriter* writer,
        void* _Nonnull* _Nonnull scratchBytes,
        size_t* numScratchBytes) {
    HAPPrecondition(writer);
    HAPPrecondition(scratchBytes);
    HAPPrecondition(numScratchBytes);

    uint8_t* bytes = writer->bytes;
    *scratchBytes = &bytes[writer->numBytes];
    *numScratchBytes = writer->maxBytes - writer->numBytes;
}

void HAPTLVWriterGetScratchBytesForTLVValue(
        const HAPTLVWriter* writer,
        void* _Nonnull* _Nonnull scratchBytes,
        size_t* numScratchBytes) {
    uint8_t* bytes;
    size_t maxBytes;
    HAPTLVWriterGetScratchBytes(writer, (void**) &bytes, &maxBytes);

    if (maxBytes < 2) {
        // If the clients writes the smallest thing possible, it'll take 2 bytes.
        // Therefore, if the client can't get even 2 bytes, then give nothing.
        *scratchBytes = bytes;
        *numScratchBytes = 0;
    } else if (maxBytes <= 257) {
        // Can fit one TLV just fine. Reserve the two bytes for the Type and Length.
        *scratchBytes = bytes + 2;
        *numScratchBytes = maxBytes - 2;
    } else {
        // Reserve 2 bytes for each possible fragment.
        // Each fragment is 257 bytes (T + L + 255 Value), so figure out how many
        // of those could fit in this buffer and reserve two bytes for each fragment
        // header.
        size_t maxFragments = (maxBytes + 257 - 1) / 257;

        // The final fragment will lose two bytes for its Type+Len.
        size_t lastFragmentSize = maxBytes - 257 * (maxFragments - 1);
        lastFragmentSize = (lastFragmentSize >= 2) ? (lastFragmentSize - 2) : 0;

        *numScratchBytes = 255 * (maxFragments - 1) + lastFragmentSize;
        // Keep the first fragment in-place (to handle the happy case of a short
        // inner TLV; in case of lots of fragments, things will have to move anyway).
        *scratchBytes = bytes + 2;
    }
}

HAPError HAPTLVFinalizeSubWriter(HAPTLVWriter* subWriter, HAPTLVWriter* outerWriter, HAPTLVType tlvType) {
    void* bytes;
    size_t numBytes;
    HAPTLVWriterGetBuffer(subWriter, &bytes, &numBytes);
    return HAPTLVWriterAppend(
            outerWriter, &(const HAPTLV) { .type = tlvType, .value = { .bytes = bytes, .numBytes = numBytes } });
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
static HAPError HAPTLVWriterEncodeScalar(
        void* bytes,
        size_t maxBytes,
        size_t* numBytes,
        HAPTLVType tlvType,
        const char* debugDescription,
        const HAPTLVFormat* format,
        HAPTLVValue* _Nullable value,
        HAPStringBuilder* stringBuilder,
        size_t nestingLevel);

HAP_RESULT_USE_CHECK
static HAPError HAPTLVWriterEncodeAggregate(
        HAPTLVWriter* writer,
        const HAPTLVFormat* format,
        HAPTLVValue* value,
        HAPStringBuilder* stringBuilder,
        size_t nestingLevel);

typedef struct {
    HAPTLVWriter* writer;
    const HAPSequenceTLVFormat* format;
    HAPError err;
    bool needsSeparator;
} EnumerateSequenceTLVContext;

static void EnumerateSequenceTLVCallback(void* _Nullable context_, HAPTLVValue* value, bool* shouldContinue) {
    HAPPrecondition(context_);
    EnumerateSequenceTLVContext* context = context_;
    HAPPrecondition(context->writer);
    HAPTLVWriter* writer = context->writer;
    HAPPrecondition(context->format);
    HAPPrecondition(HAPTLVFormatIsValid(context->format));
    const HAPSequenceTLVFormat* fmt = context->format;
    HAPPrecondition(!context->err);
    bool* needsSeparator = &context->needsSeparator;
    HAPPrecondition(value);
    HAPPrecondition(shouldContinue);
    HAPPrecondition(*shouldContinue);

    HAPError err;

    char logBytes[kHAPTLVValue_MaxLogBytes + 1];
    HAPStringBuilder stringBuilder;
    HAPStringBuilderCreate(&stringBuilder, logBytes, sizeof logBytes);

    if (!*needsSeparator) {
        HAPLogDebug(&logObject, "Encoding sequence TLV.");
        *needsSeparator = true;
    } else {
        void* bytes;
        size_t maxBytes;
        HAPTLVWriterGetScratchBytes(writer, &bytes, &maxBytes);

        size_t numBytes;
        err = HAPTLVWriterEncodeScalar(
                bytes,
                maxBytes,
                &numBytes,
                fmt->separator.tlvType,
                fmt->separator.debugDescription,
                fmt->separator.format,
                NULL,
                &stringBuilder,
                /* nestingLevel: */ 0);
        if (err) {
            HAPAssert(
                    err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                    err == kHAPError_Busy);
            context->err = err;
            *shouldContinue = false;
            return;
        }
        HAPAssert(numBytes <= maxBytes);

        err = HAPTLVWriterAppend(
                writer,
                &(const HAPTLV) { .type = fmt->separator.tlvType, .value = { .bytes = bytes, .numBytes = numBytes } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            context->err = err;
            *shouldContinue = false;
            return;
        }
    }

    {
        if (fmt->item.isFlat) {
            HAPAssert(HAPTLVFormatIsAggregate(fmt->item.format));
            HAPAssert(((const HAPBaseTLVFormat*) fmt->item.format)->type == kHAPTLVFormatType_Union);

            err = HAPTLVWriterEncodeAggregate(writer, fmt->item.format, value, &stringBuilder, /* nestingLevel: */ 0);
            if (err) {
                HAPAssert(
                        err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                        err == kHAPError_Busy);
                HAPLogTLV(&logObject, fmt->item.tlvType, fmt->item.debugDescription, "Value encoding failed.");
                context->err = err;
                *shouldContinue = false;
                return;
            }
        } else {
            void* bytes;
            size_t maxBytes;
            HAPTLVWriterGetScratchBytes(writer, &bytes, &maxBytes);

            size_t numBytes;
            if (HAPTLVFormatIsAggregate(fmt->item.format)) {
                HAPTLVAppendToLog(
                        fmt->item.tlvType,
                        fmt->item.debugDescription,
                        fmt,
                        NULL,
                        &stringBuilder,
                        /* nestingLevel: */ 0);
                HAPTLVWriter subWriter;
                HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
                err = HAPTLVWriterEncodeAggregate(
                        &subWriter, fmt->item.format, value, &stringBuilder, /* nestingLevel: */ 1);
                if (err) {
                    HAPAssert(
                            err == kHAPError_Unknown || err == kHAPError_InvalidState ||
                            err == kHAPError_OutOfResources || err == kHAPError_Busy);
                    HAPLogTLV(&logObject, fmt->item.tlvType, fmt->item.debugDescription, "Value encoding failed.");
                    context->err = err;
                    *shouldContinue = false;
                    return;
                }
                void* tlvBytes;
                HAPTLVWriterGetBuffer(&subWriter, &tlvBytes, &numBytes);
                HAPAssert(tlvBytes == bytes);
            } else {
                err = HAPTLVWriterEncodeScalar(
                        bytes,
                        maxBytes,
                        &numBytes,
                        fmt->item.tlvType,
                        fmt->item.debugDescription,
                        fmt->item.format,
                        value,
                        &stringBuilder,
                        /* nestingLevel: */ 0);
                if (err) {
                    HAPAssert(
                            err == kHAPError_Unknown || err == kHAPError_InvalidState ||
                            err == kHAPError_OutOfResources || err == kHAPError_Busy);
                    context->err = err;
                    *shouldContinue = false;
                    return;
                }
            }
            HAPAssert(numBytes <= maxBytes);

            err = HAPTLVWriterAppend(
                    writer,
                    &(const HAPTLV) { .type = fmt->item.tlvType, .value = { .bytes = bytes, .numBytes = numBytes } });
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                context->err = err;
                *shouldContinue = false;
                return;
            }
        }
    }

    if (HAPStringBuilderDidOverflow(&stringBuilder)) {
        HAPLogError(&logObject, "Logs were truncated.");
    }
    HAPLogDebug(&logObject, "Encoded sequence TLV:%s", HAPStringBuilderGetString(&stringBuilder));
}

HAP_RESULT_USE_CHECK
static HAPError HAPTLVWriterEncodeTLV(
        HAPTLVWriter* writer,
        HAPTLVType tlvType,
        const char* debugDescription,
        const HAPTLVFormat* format,
        HAPTLVValue* _Nullable value,
        HAPStringBuilder* stringBuilder,
        size_t nestingLevel) {
    HAPPrecondition(writer);
    HAPPrecondition(debugDescription);
    HAPPrecondition(format);
    HAPPrecondition(HAPTLVFormatIsValid(format));
    HAPPrecondition(stringBuilder);

    HAPError err;

    void* bytes;
    size_t maxBytes;
    HAPTLVWriterGetScratchBytes(writer, &bytes, &maxBytes);

    size_t numBytes;
    if (HAPTLVFormatIsAggregate(format)) {
        HAPTLVAppendToLog(tlvType, debugDescription, format, NULL, stringBuilder, nestingLevel);
        HAPTLVWriter subWriter;
        HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
        err = HAPTLVWriterEncodeAggregate(&subWriter, format, HAPNonnullVoid(value), stringBuilder, nestingLevel + 1);
        if (err) {
            HAPAssert(
                    err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                    err == kHAPError_Busy);
            HAPLogTLV(&logObject, tlvType, debugDescription, "Value encoding failed.");
            return err;
        }
        void* tlvBytes;
        HAPTLVWriterGetBuffer(&subWriter, &tlvBytes, &numBytes);
        HAPAssert(tlvBytes == bytes);
    } else {
        err = HAPTLVWriterEncodeScalar(
                bytes, maxBytes, &numBytes, tlvType, debugDescription, format, value, stringBuilder, nestingLevel);
        if (err) {
            HAPAssert(
                    err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                    err == kHAPError_Busy);
            return err;
        }
        HAPAssert(numBytes <= maxBytes);
    }

    err = HAPTLVWriterAppend(
            writer, &(const HAPTLV) { .type = tlvType, .value = { .bytes = bytes, .numBytes = numBytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static HAPError HAPTLVWriterEncodeScalar(
        void* bytes,
        size_t maxBytes,
        size_t* numBytes,
        HAPTLVType tlvType,
        const char* debugDescription,
        const HAPTLVFormat* format_,
        HAPTLVValue* _Nullable value_,
        HAPStringBuilder* stringBuilder,
        size_t nestingLevel) {
    HAPPrecondition(bytes);
    HAPPrecondition(debugDescription);
    HAPPrecondition(format_);
    HAPPrecondition(HAPTLVFormatIsValid(format_));
    HAPPrecondition(!HAPTLVFormatIsAggregate(format_));
    const HAPBaseTLVFormat* format = format_;
    HAPPrecondition(bytes);
    HAPPrecondition(stringBuilder);

    HAPError err;

    // For the TLVs exchanged during HomeKit pairing, a more compact representation is available for integers.
    // Integers should use the minimum number of bytes to encode the integer.
    // See HomeKit Accessory Protocol Pairing Specification R1
    // Section 18.1 TLVs
    //
    // Later, the various parts of the HomeKit Accessory Protocol Specification got merged into a single document.
    // During that merge, however, the specification about how to encode integer values for pairing TLVs was split off
    // into an entirely different section of the specification. This led to uncertainty whether those rules are now
    // meant to apply to all TLVs (instead of only applying to pairing TLVs).
    // See HomeKit Accessory Protocol Specification R17
    // Section 5 Pairing
    // See HomeKit Accessory Protocol Specification R17
    // Section 18.1 TLVs
    //
    // Newer TLVs introduced in later specifications ignored those pairing protocol specific TLV rules.
    // For example, when exchanging TLVs for Software Authentication the TLVs must use constant lengths.
    // On top of that, the CoreUtils TLV parser commonly used by controllers can only decode integer TLVs
    // with a power of two used as length. Lengths 1, 2, 4 and 8 work, but lengths 0, 3, 5, 6, 7 don't work.
    // It is also unclear whether or not enumerations, bit sets, and float values can and should be shortened.
    //
    // Because the full length encoding is compatible with all known controllers and also has the added benefit
    // of not exposing the range of transmitted data to an unauthorized observer, we opt to always send full length.
    // So far, benefiting TLVs were restricted to profiles exclusive to HAP over IP where latency impact is negligible.
    // When receiving, we support all lengths from 0 through 8 bytes for unsigned integers, enumerations, and bit sets.
    //
    // If this decision is revised and variable length encoding should be used, it must be determined for each TLV:
    // - Is there any released controller version that cannot process certain lengths?
    // - Does the TLV contain security critical data that benefits from length hiding padding?

    *numBytes = 0;
    switch (format->type) {
        case kHAPTLVFormatType_None: {
            const HAPSeparatorTLVFormat* fmt HAP_UNUSED = format_;
            HAPPrecondition(!value_);
            HAPTLVAppendToLog(tlvType, debugDescription, format_, value_, stringBuilder, nestingLevel);
            *numBytes = 0;
            HAPAssert(*numBytes <= maxBytes);
        }
            return kHAPError_None;
        case kHAPTLVFormatType_Enum: {
            const HAPEnumTLVFormat* fmt = format_;
            const uint8_t* value = HAPNonnullVoid(value_);
            HAPAssert(fmt->callbacks.isValid);
            HAPPrecondition(fmt->callbacks.isValid(*value));
            HAPTLVAppendToLog(tlvType, debugDescription, format_, value_, stringBuilder, nestingLevel);
            if (maxBytes < sizeof *value) {
                HAPLogTLV(&logObject, tlvType, debugDescription, "Not enough memory to encode enumeration value.");
                return kHAPError_OutOfResources;
            }
            /* Send with full length for maximum compatibility. See note above. */
            ((uint8_t*) bytes)[0] = *value;
            *numBytes = sizeof *value;
            HAPAssert(*numBytes <= maxBytes);
        }
            return kHAPError_None;
#define PROCESS_INTEGER_FORMAT(formatName, typeName, unsignedTypeName, printfFormat, printfTypeName) \
    do { \
        const formatName* fmt = format_;          /* NOLINT(bugprone-macro-parentheses) */ \
        typeName* value = HAPNonnullVoid(value_); /* NOLINT(bugprone-macro-parentheses) */ \
        HAPPrecondition(*value >= fmt->constraints.minimumValue); \
        HAPPrecondition(*value <= fmt->constraints.maximumValue); \
        HAPTLVAppendToLog(tlvType, debugDescription, format_, value_, stringBuilder, nestingLevel); \
        if (maxBytes < sizeof *value) { \
            HAPLogTLV(&logObject, tlvType, debugDescription, "Not enough memory to encode integer value."); \
            return kHAPError_OutOfResources; \
        } \
        /* Send with full length for maximum compatibility. See note above. */ \
        *numBytes = sizeof *value; \
        unsignedTypeName unsignedValue = (unsignedTypeName) *value; \
        for (size_t i = 0; i < *numBytes; i++) { \
            ((uint8_t*) bytes)[i] = (uint8_t)(unsignedValue >> (uint8_t)(i * CHAR_BIT)) & 0xFFU; \
        } \
        HAPAssert(*numBytes <= maxBytes); \
    } while (0)
        case kHAPTLVFormatType_UInt8: {
            PROCESS_INTEGER_FORMAT(HAPUInt8TLVFormat, uint8_t, uint8_t, "u", unsigned int);
        }
            return kHAPError_None;
        case kHAPTLVFormatType_UInt16: {
            PROCESS_INTEGER_FORMAT(HAPUInt16TLVFormat, uint16_t, uint16_t, "u", unsigned int);
        }
            return kHAPError_None;
        case kHAPTLVFormatType_UInt32: {
            PROCESS_INTEGER_FORMAT(HAPUInt32TLVFormat, uint32_t, uint32_t, "lu", unsigned long);
        }
            return kHAPError_None;
        case kHAPTLVFormatType_UInt64: {
            PROCESS_INTEGER_FORMAT(HAPUInt64TLVFormat, uint64_t, uint64_t, "llu", unsigned long long);
        }
            return kHAPError_None;
        case kHAPTLVFormatType_Int8: {
            PROCESS_INTEGER_FORMAT(HAPInt8TLVFormat, int8_t, uint8_t, "d", int);
        }
            return kHAPError_None;
        case kHAPTLVFormatType_Int16: {
            PROCESS_INTEGER_FORMAT(HAPInt16TLVFormat, int16_t, uint16_t, "d", int);
        }
            return kHAPError_None;
        case kHAPTLVFormatType_Int32: {
            PROCESS_INTEGER_FORMAT(HAPInt32TLVFormat, int32_t, uint32_t, "ld", long);
        }
            return kHAPError_None;
        case kHAPTLVFormatType_Int64: {
            PROCESS_INTEGER_FORMAT(HAPInt64TLVFormat, int64_t, uint64_t, "lld", long long);
        }
            return kHAPError_None;
#undef PROCESS_INTEGER_FORMAT
        case kHAPTLVFormatType_Data: {
            const HAPDataTLVFormat* fmt = format_;
            const HAPDataTLVValue* value = HAPNonnullVoid(value_);
            HAPPrecondition(value->numBytes >= fmt->constraints.minLength);
            HAPPrecondition(value->numBytes <= fmt->constraints.maxLength);
            HAPTLVAppendToLog(tlvType, debugDescription, format_, value_, stringBuilder, nestingLevel);
            if (maxBytes < value->numBytes) {
                HAPLogTLV(&logObject, tlvType, debugDescription, "Not enough memory to encode data value.");
                return kHAPError_OutOfResources;
            }
            HAPRawBufferCopyBytes(bytes, value->bytes, value->numBytes);
            *numBytes = value->numBytes;
            HAPAssert(*numBytes <= maxBytes);
        }
            return kHAPError_None;
        case kHAPTLVFormatType_String: {
            const HAPStringTLVFormat* fmt = format_;
            const char** value = HAPNonnullVoid(value_);
            size_t numValueBytes = HAPStringGetNumBytes(*value);
            HAPPrecondition(!fmt->callbacks.isValid || fmt->callbacks.isValid(*value));
            HAPPrecondition(HAPUTF8IsValidData(*value, numValueBytes));
            HAPPrecondition(numValueBytes >= fmt->constraints.minLength);
            HAPPrecondition(numValueBytes <= fmt->constraints.maxLength);
            HAPTLVAppendToLog(tlvType, debugDescription, format_, value_, stringBuilder, nestingLevel);
            if (maxBytes < numValueBytes) {
                HAPLogTLV(&logObject, tlvType, debugDescription, "Not enough memory to encode string value.");
                return kHAPError_OutOfResources;
            }
            HAPRawBufferCopyBytes(bytes, *value, numValueBytes);
            *numBytes = numValueBytes;
            HAPAssert(*numBytes <= maxBytes);
        }
            return kHAPError_None;
        case kHAPTLVFormatType_Value: {
            const HAPValueTLVFormat* fmt = format_;
            HAPTLVAppendToLog(tlvType, debugDescription, format_, value_, stringBuilder, nestingLevel);
            HAPAssert(fmt->callbacks.encode);
            err = fmt->callbacks.encode(HAPNonnullVoid(value_), bytes, maxBytes, numBytes);
            if (err) {
                HAPAssert(
                        err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                        err == kHAPError_Busy);
                HAPLogTLV(&logObject, tlvType, debugDescription, "Not enough memory to encode value.");
                return err;
            }
            HAPAssert(*numBytes <= maxBytes);
        }
            return kHAPError_None;
        case kHAPTLVFormatType_Sequence:
        case kHAPTLVFormatType_Struct:
        case kHAPTLVFormatType_Union: {
        }
            HAPFatalError();
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
static bool GetStructMemberIsSet(const HAPStructTLVMember* member, HAPTLVValue* value) {
    HAPPrecondition(member);
    HAPPrecondition(member->isOptional);
    HAPPrecondition(value);

    return *((bool*) &((char*) value)[member->isSetOffset]);
}

HAP_RESULT_USE_CHECK
static HAPTLVValue* GetStructMemberValue(const HAPStructTLVMember* member, HAPTLVValue* value) {
    HAPPrecondition(member);
    HAPPrecondition(value);

    return &((char*) value)[member->valueOffset];
}

HAP_RESULT_USE_CHECK
static HAPError HAPTLVWriterEncodeAggregate(
        HAPTLVWriter* writer,
        const HAPTLVFormat* format_,
        HAPTLVValue* value_,
        HAPStringBuilder* stringBuilder,
        size_t nestingLevel) {
    HAPPrecondition(writer);
    HAPPrecondition(format_);
    HAPPrecondition(HAPTLVFormatIsValid(format_));
    HAPPrecondition(HAPTLVFormatIsAggregate(format_));
    const HAPBaseTLVFormat* format = format_;
    HAPPrecondition(value_);
    HAPPrecondition(stringBuilder);

    HAPError err;

    if (format->type == kHAPTLVFormatType_Sequence) {
        const HAPSequenceTLVFormat* fmt HAP_UNUSED = format_;
        HAPSequenceTLVValue* value = value_;
        HAPPrecondition(value->enumerate);
        EnumerateSequenceTLVContext enumerateContext;
        HAPRawBufferZero(&enumerateContext, sizeof enumerateContext);
        enumerateContext.writer = writer;
        enumerateContext.format = format_;
        err = value->enumerate(&value->dataSource, EnumerateSequenceTLVCallback, &enumerateContext);
        if (!err) {
            err = enumerateContext.err;
        }
        if (err) {
            HAPAssert(
                    err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                    err == kHAPError_Busy);
            return err;
        }
    } else if (format->type == kHAPTLVFormatType_Struct) {
        const HAPStructTLVFormat* fmt = format_;
        HAPPrecondition(!fmt->callbacks.isValid || fmt->callbacks.isValid(HAPNonnullVoid(value_)));
        if (fmt->members) {
            for (size_t i = 0; fmt->members[i]; i++) {
                const HAPStructTLVMember* member = fmt->members[i];
                HAPTLVValue* memberValue = GetStructMemberValue(member, HAPNonnullVoid(value_));
                if (member->isFlat) {
                    HAPAssert(HAPTLVFormatIsAggregate(member->format));
                    HAPAssert(!member->isOptional);
                    err = HAPTLVWriterEncodeAggregate(writer, member->format, memberValue, stringBuilder, nestingLevel);
                    if (err) {
                        HAPAssert(
                                err == kHAPError_Unknown || err == kHAPError_InvalidState ||
                                err == kHAPError_OutOfResources || err == kHAPError_Busy);
                        return err;
                    }
                } else {
                    if (!member->isOptional || GetStructMemberIsSet(member, HAPNonnullVoid(value_))) {
                        err = HAPTLVWriterEncodeTLV(
                                writer,
                                member->tlvType,
                                member->debugDescription,
                                member->format,
                                memberValue,
                                stringBuilder,
                                nestingLevel);
                        if (err) {
                            HAPAssert(
                                    err == kHAPError_Unknown || err == kHAPError_InvalidState ||
                                    err == kHAPError_OutOfResources || err == kHAPError_Busy);
                            return err;
                        }
                    }
                }
            }
        }
    } else {
        const HAPUnionTLVFormat* fmt = format_;
        const HAPUnionTLVValue* value = value_;
        if (fmt->variants) {
            bool isValid = false;
            for (size_t i = 0; fmt->variants[i]; i++) {
                const HAPUnionTLVVariant* variant = fmt->variants[i];
                if (variant->tlvType != value->type) {
                    continue;
                }
                err = HAPTLVWriterEncodeTLV(
                        writer,
                        variant->tlvType,
                        variant->debugDescription,
                        variant->format,
                        &((char*) value_)[fmt->untaggedValueOffset],
                        stringBuilder,
                        nestingLevel);
                if (err) {
                    HAPAssert(
                            err == kHAPError_Unknown || err == kHAPError_InvalidState ||
                            err == kHAPError_OutOfResources || err == kHAPError_Busy);
                    return err;
                }
                isValid = true;
                break;
            }
            HAPPrecondition(isValid);
        }
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPTLVWriterEncodeVoid(HAPTLVWriter* writer, const HAPTLVFormat* format, HAPTLVValue* value) {
    HAPPrecondition(writer);
    HAPPrecondition(format);
    HAPPrecondition(HAPTLVFormatIsValid(format));
    HAPPrecondition(HAPTLVFormatIsAggregate(format));
    HAPPrecondition(value);

    HAPError err;

    char logBytes[kHAPTLVValue_MaxLogBytes + 1];
    HAPStringBuilder stringBuilder;
    HAPStringBuilderCreate(&stringBuilder, logBytes, sizeof logBytes);

    err = HAPTLVWriterEncodeAggregate(writer, format, value, &stringBuilder, /* nestingLevel: */ 0);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                err == kHAPError_Busy);
        return err;
    }

    if (HAPStringBuilderDidOverflow(&stringBuilder)) {
        HAPLogError(&logObject, "Logs were truncated.");
    }
    HAPLogDebug(&logObject, "Encoded TLV:%s", HAPStringBuilderGetString(&stringBuilder));
    return kHAPError_None;
}
