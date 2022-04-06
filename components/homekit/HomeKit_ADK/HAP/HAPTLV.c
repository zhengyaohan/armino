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

#include "HAPTLV+Internal.h"

HAP_RESULT_USE_CHECK
bool HAPTLVFormatIsAggregate(const HAPTLVFormat* format_) {
    HAPPrecondition(format_);
    const HAPBaseTLVFormat* format = format_;

    switch (format->type) {
        case kHAPTLVFormatType_None:
        case kHAPTLVFormatType_Enum:
        case kHAPTLVFormatType_UInt8:
        case kHAPTLVFormatType_UInt16:
        case kHAPTLVFormatType_UInt32:
        case kHAPTLVFormatType_UInt64:
        case kHAPTLVFormatType_Int8:
        case kHAPTLVFormatType_Int16:
        case kHAPTLVFormatType_Int32:
        case kHAPTLVFormatType_Int64:
        case kHAPTLVFormatType_Data:
        case kHAPTLVFormatType_String:
        case kHAPTLVFormatType_Value: {
            return false;
        }
        case kHAPTLVFormatType_Sequence:
        case kHAPTLVFormatType_Struct:
        case kHAPTLVFormatType_Union: {
            return true;
        }
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
bool HAPTLVFormatUsesType(const HAPTLVFormat* format_, HAPTLVType tlvType) {
    HAPPrecondition(format_);
    const HAPBaseTLVFormat* format = format_;

    if (!HAPTLVFormatIsAggregate(format_)) {
        return false;
    }
    if (format->type == kHAPTLVFormatType_Sequence) {
        const HAPSequenceTLVFormat* fmt = format_;
        if (fmt->item.isFlat) {
            if (HAPTLVFormatUsesType(fmt->item.format, tlvType)) {
                return true;
            }
        } else {
            if (tlvType == fmt->item.tlvType) {
                return true;
            }
        }
        if (tlvType == fmt->separator.tlvType) {
            return true;
        }
    } else if (format->type == kHAPTLVFormatType_Struct) {
        const HAPStructTLVFormat* fmt = format_;
        if (fmt->members) {
            for (size_t i = 0; fmt->members[i]; i++) {
                const HAPStructTLVMember* member = fmt->members[i];
                if (member->isFlat) {
                    if (HAPTLVFormatUsesType(member->format, tlvType)) {
                        return true;
                    }
                } else {
                    if (tlvType == member->tlvType) {
                        return true;
                    }
                }
            }
        }
    } else {
        HAPAssert(format->type == kHAPTLVFormatType_Union);
        const HAPUnionTLVFormat* fmt = format_;
        if (fmt->variants) {
            for (size_t i = 0; fmt->variants[i]; i++) {
                const HAPUnionTLVVariant* variant = fmt->variants[i];
                if (tlvType == variant->tlvType) {
                    return true;
                }
            }
        }
    }
    return false;
}

HAP_RESULT_USE_CHECK
bool HAPTLVFormatHaveConflictingTypes(const HAPTLVFormat* format_, const HAPTLVFormat* otherFormat) {
    HAPPrecondition(format_);
    const HAPBaseTLVFormat* format = format_;
    HAPPrecondition(otherFormat);

    if (!HAPTLVFormatIsAggregate(format_) || !HAPTLVFormatIsAggregate(otherFormat)) {
        return false;
    }
    if (format->type == kHAPTLVFormatType_Sequence) {
        const HAPSequenceTLVFormat* fmt = format_;
        if (fmt->item.isFlat) {
            if (HAPTLVFormatUsesType(otherFormat, fmt->item.tlvType)) {
                return true;
            }
        }
    } else if (format->type == kHAPTLVFormatType_Struct) {
        const HAPStructTLVFormat* fmt = format_;
        if (fmt->members) {
            for (size_t i = 0; fmt->members[i]; i++) {
                const HAPStructTLVMember* member = fmt->members[i];
                if (member->isFlat) {
                    if (HAPTLVFormatUsesType(otherFormat, member->tlvType)) {
                        return true;
                    }
                }
            }
        }
    } else {
        HAPAssert(format->type == kHAPTLVFormatType_Union);
    }
    return false;
}

HAP_RESULT_USE_CHECK
bool HAPTLVFormatIsValid(const HAPTLVFormat* format_) {
    HAPPrecondition(format_);
    const HAPBaseTLVFormat* format = format_;

    switch (format->type) {
        case kHAPTLVFormatType_None: {
            const HAPSeparatorTLVFormat* fmt HAP_UNUSED = format_;
        }
            return true;
        case kHAPTLVFormatType_Enum: {
            const HAPEnumTLVFormat* fmt = format_;
            if (!fmt->callbacks.isValid) {
                return false;
            }
            if (!fmt->callbacks.getDescription) {
                return false;
            }
        }
            return true;
#define PROCESS_INTEGER_FORMAT(formatName, typeName, printfFormat, printfTypeName) \
    do { \
        const formatName* fmt = format_; \
        if (fmt->constraints.maximumValue < fmt->constraints.minimumValue) { \
            return false; \
        } \
    } while (0)
        case kHAPTLVFormatType_UInt8: {
            PROCESS_INTEGER_FORMAT(HAPUInt8TLVFormat, uint8_t, "u", unsigned int);
        }
            return true;
        case kHAPTLVFormatType_UInt16: {
            PROCESS_INTEGER_FORMAT(HAPUInt16TLVFormat, uint16_t, "u", unsigned int);
        }
            return true;
        case kHAPTLVFormatType_UInt32: {
            PROCESS_INTEGER_FORMAT(HAPUInt32TLVFormat, uint32_t, "lu", unsigned long);
        }
            return true;
        case kHAPTLVFormatType_UInt64: {
            PROCESS_INTEGER_FORMAT(HAPUInt64TLVFormat, uint64_t, "llu", unsigned long long);
        }
            return true;
        case kHAPTLVFormatType_Int8: {
            PROCESS_INTEGER_FORMAT(HAPInt8TLVFormat, int8_t, "d", int);
        }
            return true;
        case kHAPTLVFormatType_Int16: {
            PROCESS_INTEGER_FORMAT(HAPInt16TLVFormat, int16_t, "d", int);
        }
            return true;
        case kHAPTLVFormatType_Int32: {
            PROCESS_INTEGER_FORMAT(HAPInt32TLVFormat, int32_t, "ld", long);
        }
            return true;
        case kHAPTLVFormatType_Int64: {
            PROCESS_INTEGER_FORMAT(HAPInt64TLVFormat, int64_t, "lld", long long);
        }
            return true;
#undef PROCESS_INTEGER_FORMAT
        case kHAPTLVFormatType_Data: {
            const HAPDataTLVFormat* fmt = format_;
            if (fmt->constraints.maxLength < fmt->constraints.minLength) {
                return false;
            }
        }
            return true;
        case kHAPTLVFormatType_String: {
            const HAPStringTLVFormat* fmt = format_;
            if (fmt->constraints.maxLength < fmt->constraints.minLength) {
                return false;
            }
        }
            return true;
        case kHAPTLVFormatType_Value: {
            const HAPValueTLVFormat* fmt = format_;
            if (!fmt->callbacks.decode) {
                return false;
            }
            if (!fmt->callbacks.encode) {
                return false;
            }
            if (!fmt->callbacks.getDescription) {
                return false;
            }
        }
            return true;
        case kHAPTLVFormatType_Sequence: {
            const HAPSequenceTLVFormat* fmt = format_;
            if (!fmt->item.format) {
                return false;
            }
            if (!HAPTLVFormatIsValid(fmt->item.format)) {
                return false;
            }
            if (!fmt->separator.format) {
                return false;
            }
            if (fmt->item.isFlat) {
                if (!HAPTLVFormatIsAggregate(fmt->item.format)) {
                    return false;
                }
                if (((const HAPBaseTLVFormat*) fmt->item.format)->type != kHAPTLVFormatType_Union) {
                    return false;
                }
                if (HAPTLVFormatUsesType(fmt->item.format, fmt->separator.tlvType)) {
                    return false;
                }
            } else {
                if (fmt->item.tlvType == fmt->separator.tlvType) {
                    return false;
                }
            }
        }
            return true;
        case kHAPTLVFormatType_Struct: {
            const HAPStructTLVFormat* fmt = format_;
            if (fmt->members) {
                for (size_t i = 0; fmt->members[i]; i++) {
                    const HAPStructTLVMember* member = fmt->members[i];
                    if (!member->format) {
                        return false;
                    }
                    if (!HAPTLVFormatIsValid(member->format)) {
                        return false;
                    }
                    for (size_t j = 0; j < i; j++) {
                        const HAPStructTLVMember* otherMember = fmt->members[j];
                        if (member->isFlat) {
                            if (!HAPTLVFormatIsAggregate(member->format)) {
                                return false;
                            }
                            if (member->isOptional) {
                                return false;
                            }
                            if (otherMember->isFlat) {
                                if (HAPTLVFormatHaveConflictingTypes(member->format, otherMember->format)) {
                                    return false;
                                }
                            } else {
                                if (HAPTLVFormatUsesType(member->format, otherMember->tlvType)) {
                                    return false;
                                }
                            }
                        } else {
                            if (otherMember->isFlat) {
                                if (HAPTLVFormatUsesType(otherMember->format, member->tlvType)) {
                                    return false;
                                }
                            } else {
                                if (member->tlvType == otherMember->tlvType) {
                                    return false;
                                }
                            }
                        }
                    }
                }
            }
        }
            return true;
        case kHAPTLVFormatType_Union: {
            const HAPUnionTLVFormat* fmt = format_;
            if (fmt->variants) {
                for (size_t i = 0; fmt->variants[i]; i++) {
                    const HAPUnionTLVVariant* variant = fmt->variants[i];
                    if (!variant->format) {
                        return false;
                    }
                    if (!HAPTLVFormatIsValid(variant->format)) {
                        return false;
                    }
                    for (size_t j = 0; j < i; j++) {
                        const HAPUnionTLVVariant* otherVariant = fmt->variants[j];
                        if (variant->tlvType == otherVariant->tlvType) {
                            return false;
                        }
                    }
                }
            }
        }
            return true;
        default:
            return false;
    }
}

void HAPTLVAppendToLog(
        HAPTLVType tlvType,
        const char* debugDescription,
        const HAPTLVFormat* format_,
        HAPTLVValue* _Nullable value_,
        HAPStringBuilder* stringBuilder,
        size_t nestingLevel) {
    HAPPrecondition(debugDescription);
    HAPPrecondition(format_);
    const HAPBaseTLVFormat* format = format_;
    HAPPrecondition(stringBuilder);

    HAPError err;

    HAPStringBuilderAppend(stringBuilder, "\n");
    for (size_t i = 0; i < nestingLevel; i++) {
        HAPStringBuilderAppend(stringBuilder, "  ");
    }
    HAPStringBuilderAppend(stringBuilder, "- [%02X %s] ", tlvType, debugDescription);

    switch (format->type) {
        case kHAPTLVFormatType_None: {
            return;
        }
        case kHAPTLVFormatType_Enum: {
            const HAPEnumTLVFormat* fmt = format_;
            HAPPrecondition(fmt->callbacks.getDescription);
            uint8_t* value = HAPNonnullVoid(value_);
            HAPStringBuilderAppend(stringBuilder, "%s (%u)", fmt->callbacks.getDescription(*value), *value);
            return;
        }
#define PROCESS_INTEGER_FORMAT(formatName, typeName, printfFormat, printfTypeName) \
    do { \
        const formatName* fmt = format_;          /* NOLINT(bugprone-macro-parentheses) */ \
        typeName* value = HAPNonnullVoid(value_); /* NOLINT(bugprone-macro-parentheses) */ \
        bool appended = false; \
        if (fmt->callbacks.getDescription) { \
            const char* _Nullable description = fmt->callbacks.getDescription(*value); \
            if (description) { \
                HAPStringBuilderAppend(stringBuilder, "%s (%" printfFormat ")", description, (printfTypeName) *value); \
                appended = true; \
            } \
        } \
        if (!appended && fmt->callbacks.getBitDescription) { \
            HAPStringBuilderAppend(stringBuilder, "["); \
            bool needsSeparator = false; \
            for (size_t i = 0; i < sizeof(typeName) * CHAR_BIT; i++) { \
                typeName optionValue = (typeName)(1U << i); \
                if (*value & optionValue) { \
                    if (needsSeparator) { \
                        HAPStringBuilderAppend(stringBuilder, ", "); \
                    } \
                    needsSeparator = true; \
                    const char* _Nullable bitDescription = fmt->callbacks.getBitDescription(optionValue); \
                    if (bitDescription) { \
                        HAPStringBuilderAppend(stringBuilder, "%s", bitDescription); \
                    } else { \
                        HAPStringBuilderAppend(stringBuilder, "<Unknown bit>"); \
                    } \
                    HAPStringBuilderAppend(stringBuilder, " (bit %zu)", i); \
                } \
            } \
            HAPStringBuilderAppend(stringBuilder, "]"); \
            appended = true; \
        } \
        if (!appended) { \
            HAPStringBuilderAppend(stringBuilder, "%" printfFormat, (printfTypeName) *value); \
        } \
    } while (0)
        case kHAPTLVFormatType_UInt8: {
            PROCESS_INTEGER_FORMAT(HAPUInt8TLVFormat, uint8_t, "u", unsigned int);
            return;
        }
        case kHAPTLVFormatType_UInt16: {
            PROCESS_INTEGER_FORMAT(HAPUInt16TLVFormat, uint16_t, "u", unsigned int);
            return;
        }
        case kHAPTLVFormatType_UInt32: {
            PROCESS_INTEGER_FORMAT(HAPUInt32TLVFormat, uint32_t, "lu", unsigned long);
            return;
        }
        case kHAPTLVFormatType_UInt64: {
            PROCESS_INTEGER_FORMAT(HAPUInt64TLVFormat, uint64_t, "llu", unsigned long long);
            return;
        }
#undef PROCESS_INTEGER_FORMAT
#define PROCESS_INTEGER_FORMAT(formatName, typeName, printfFormat, printfTypeName) \
    do { \
        const formatName* fmt = format_;          /* NOLINT(bugprone-macro-parentheses) */ \
        typeName* value = HAPNonnullVoid(value_); /* NOLINT(bugprone-macro-parentheses) */ \
        bool appended = false; \
        if (fmt->callbacks.getDescription) { \
            const char* _Nullable description = fmt->callbacks.getDescription(*value); \
            if (description) { \
                HAPStringBuilderAppend(stringBuilder, "%s (%" printfFormat ")", description, (printfTypeName) *value); \
                appended = true; \
            } \
        } \
        if (!appended) { \
            HAPStringBuilderAppend(stringBuilder, "%" printfFormat, (printfTypeName) *value); \
        } \
    } while (0)
        case kHAPTLVFormatType_Int8: {
            PROCESS_INTEGER_FORMAT(HAPInt8TLVFormat, int8_t, "d", int);
            return;
        }
        case kHAPTLVFormatType_Int16: {
            PROCESS_INTEGER_FORMAT(HAPInt16TLVFormat, int16_t, "d", int);
            return;
        }
        case kHAPTLVFormatType_Int32: {
            PROCESS_INTEGER_FORMAT(HAPInt32TLVFormat, int32_t, "ld", long);
            return;
        }
        case kHAPTLVFormatType_Int64: {
            PROCESS_INTEGER_FORMAT(HAPInt64TLVFormat, int64_t, "lld", long long);
            return;
        }
#undef PROCESS_INTEGER_FORMAT
        case kHAPTLVFormatType_Data: {
            const HAPDataTLVFormat* fmt HAP_UNUSED = format_;
            HAPDataTLVValue* value = HAPNonnullVoid(value_);
            HAPStringBuilderAppend(stringBuilder, "<");
            for (size_t i = 0; i < value->numBytes; i++) {
                const uint8_t* b = value->bytes;
                if (i && !(i % 4)) {
                    HAPStringBuilderAppend(stringBuilder, " ");
                }
                HAPStringBuilderAppend(stringBuilder, "%02X", b[i]);
            }
            HAPStringBuilderAppend(stringBuilder, ">");
            return;
        }
        case kHAPTLVFormatType_String: {
            const HAPStringTLVFormat* fmt HAP_UNUSED = format_;
            char** value = HAPNonnullVoid(value_);
            HAPStringBuilderAppend(stringBuilder, "%s", *value);
            return;
        }
        case kHAPTLVFormatType_Value: {
            const HAPValueTLVFormat* fmt = format_;
            HAPPrecondition(fmt->callbacks.getDescription);

            char descriptionBytes[kHAPTLVValue_MaxDescriptionBytes + 1];
            err = fmt->callbacks.getDescription(HAPNonnullVoid(value_), descriptionBytes, sizeof descriptionBytes);
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                HAPStringBuilderAppend(stringBuilder, "<Description too long>");
            } else {
                HAPStringBuilderAppend(stringBuilder, "%s", descriptionBytes);
            }
            return;
        }
        case kHAPTLVFormatType_Sequence: {
            const HAPSequenceTLVFormat* fmt HAP_UNUSED = format_;
            HAPStringBuilderAppend(stringBuilder, "<Sequence>");
            return;
        }
        case kHAPTLVFormatType_Struct: {
            const HAPStructTLVFormat* fmt HAP_UNUSED = format_;
            return;
        }
        case kHAPTLVFormatType_Union: {
            const HAPUnionTLVFormat* fmt HAP_UNUSED = format_;
            return;
        }
        default:
            HAPFatalError();
    }
}
