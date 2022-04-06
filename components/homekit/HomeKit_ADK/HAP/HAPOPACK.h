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

#ifndef HAP_OPACK_H
#define HAP_OPACK_H

#include "HAP+API.h"
#include "HAPLogSubsystem.h"

#ifdef __cplusplus
extern "C" {
#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * Seconds since 2001-01-01 00:00:00.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 9.1.4 Data Format
 */
typedef double HAPOPACKDate;

/**
 * Number type.
 */
HAP_ENUM_BEGIN(uint8_t, HAPOPACKNumberType) {
    kHAPOPACKNumberType_Int64 = 1, /**< Int64. */
    kHAPOPACKNumberType_Double,    /**< Double. */
} HAP_ENUM_END(uint8_t, HAPOPACKNumberType);

/**
 * Checks that a value matches the claimed HAPOPACKNumberType.
 *
 * - Argument numbers start at 1.
 *
 * @param      valueArg             Argument number of the value.
 * @param      typeArg              Argument number of the value type.
 */
#if __has_attribute(pointer_with_type_tag) && __has_attribute(type_tag_for_datatype)
/**@cond */
__attribute__((type_tag_for_datatype(
        HAPOPACKNumber,
        int64_t*))) static const HAPOPACKNumberType _kHAPOPACKNumberType_Int64 HAP_UNUSED = kHAPOPACKNumberType_Int64;

__attribute__((type_tag_for_datatype(
        HAPOPACKNumber,
        double*))) static const HAPOPACKNumberType _kHAPOPACKNumberType_Double HAP_UNUSED = kHAPOPACKNumberType_Double;
/**@endcond */

#define HAP_PWT_OPACKNumberType(valueArg, typeArg) \
    __attribute__((pointer_with_type_tag(HAPOPACKNumber, valueArg, typeArg)))
#else
#define HAP_PWT_OPACKNumberType(valueArg, typeArg)
#endif

/**
 * Number.
 *
 * - Can hold signed integer values up to Int64.
 *
 * - Can hold floating point values up to Double.
 */
typedef struct {
    bool isFloatType : 1; /**< Whether the value is stored as a floating point number. */

    /**
     * Numeric value.
     */
    union {
        int64_t int64Value; /**< Int64. */
        double doubleValue; /**< Double. */
    } value;
} HAPOPACKNumber;
HAP_NONNULL_SUPPORT(HAPOPACKNumber)

/**
 * Creates a number.
 *
 * @param[out] number               Number.
 * @param      type                 Data type of value to convert.
 * @param      value                Numeric value.
 */
HAP_PWT_OPACKNumberType(
        3,
        2) void HAPOPACKNumberCreate(HAPOPACKNumber* number, HAPOPACKNumberType type, const void* value);

/**
 * Obtains the value of a number.
 *
 * @param      number               Number.
 * @param      type                 Data type to return.
 * @param[out] value                Numeric value.
 *
 * @return true                     If the value contains an exact representation of the number.
 * @return false                    If the conversion is lossy or the return value is out of range.
 */
HAP_PWT_OPACKNumberType(3, 2) HAP_RESULT_USE_CHECK
        bool HAPOPACKNumberGetValue(const HAPOPACKNumber* number, HAPOPACKNumberType type, void* value);

/**
 * Determines whether the value of a number is stored in a floating point format.
 *
 * @param      number               Number.
 *
 * @return true                     If number's value is a floating point type.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPOPACKNumberIsFloatType(const HAPOPACKNumber* number);

/**
 * OPACK tags.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 9.1.4 Data Format
 */
HAP_ENUM_BEGIN(uint8_t, HAPOPACKTag) {
    /** True value. */
    kHAPOPACKTag_True = 0x01,

    /** False value. */
    kHAPOPACKTag_False = 0x02,

    /** Terminator for variable-item objects. */
    kHAPOPACKTag_Terminator = 0x03,

    /** Null object. */
    kHAPOPACKTag_Null = 0x04,

    /** <16:big endian UUID>. */
    kHAPOPACKTag_UUID = 0x05,

    /** <8:little endian Float64 seconds since 2001-01-01 00:00:00>. */
    kHAPOPACKTag_Date = 0x06,

    /** Integer -1. */
    kHAPOPACKTag_Negative1 = 0x07,

    /** Integers 0-38 (value = tag - 0x08). */
    /**@{*/
    kHAPOPACKTag_0 = 0x08,
    kHAPOPACKTag_1,
    kHAPOPACKTag_2,
    kHAPOPACKTag_3,
    kHAPOPACKTag_4,
    kHAPOPACKTag_5,
    kHAPOPACKTag_6,
    kHAPOPACKTag_7,
    kHAPOPACKTag_8,
    kHAPOPACKTag_9,
    kHAPOPACKTag_10,
    kHAPOPACKTag_11,
    kHAPOPACKTag_12,
    kHAPOPACKTag_13,
    kHAPOPACKTag_14,
    kHAPOPACKTag_15,
    kHAPOPACKTag_16,
    kHAPOPACKTag_17,
    kHAPOPACKTag_18,
    kHAPOPACKTag_19,
    kHAPOPACKTag_20,
    kHAPOPACKTag_21,
    kHAPOPACKTag_22,
    kHAPOPACKTag_23,
    kHAPOPACKTag_24,
    kHAPOPACKTag_25,
    kHAPOPACKTag_26,
    kHAPOPACKTag_27,
    kHAPOPACKTag_28,
    kHAPOPACKTag_29,
    kHAPOPACKTag_30,
    kHAPOPACKTag_31,
    kHAPOPACKTag_32,
    kHAPOPACKTag_33,
    kHAPOPACKTag_34,
    kHAPOPACKTag_35,
    kHAPOPACKTag_36,
    kHAPOPACKTag_37,
    kHAPOPACKTag_38,
    /**@}*/

    /** Integer 39. */
    kHAPOPACKTag_39 = 0x2F,

    /** 8-bit signed integer. Range: -128 to 127. */
    kHAPOPACKTag_Int8 = 0x30,

    /** 16-bit, little endian, signed integer. Range: -32768 to 32767. */
    kHAPOPACKTag_Int16 = 0x31,

    /** 32-bit, little endian, signed integer. Range: -2147483648 to 2147483647. */
    kHAPOPACKTag_Int32 = 0x32,

    /** 64-bit, little endian, signed integer. Range: -9223372036854775807 to 9223372036854775807. */
    kHAPOPACKTag_Int64 = 0x33,

    /** Little endian Float32. Float32 is an IEEE single precision value. */
    kHAPOPACKTag_Float = 0x35,

    /** Little endian Float64. Float64 is an IEEE double precision value. */
    kHAPOPACKTag_Double = 0x36,

    /** 0-32 bytes of UTF-8 (length = tag - 0x40). Not NUL terminated. */
    /**@{*/
    kHAPOPACKTag_String0 = 0x40,
    kHAPOPACKTag_String1,
    kHAPOPACKTag_String2,
    kHAPOPACKTag_String3,
    kHAPOPACKTag_String4,
    kHAPOPACKTag_String5,
    kHAPOPACKTag_String6,
    kHAPOPACKTag_String7,
    kHAPOPACKTag_String8,
    kHAPOPACKTag_String9,
    kHAPOPACKTag_String10,
    kHAPOPACKTag_String11,
    kHAPOPACKTag_String12,
    kHAPOPACKTag_String13,
    kHAPOPACKTag_String14,
    kHAPOPACKTag_String15,
    kHAPOPACKTag_String16,
    kHAPOPACKTag_String17,
    kHAPOPACKTag_String18,
    kHAPOPACKTag_String19,
    kHAPOPACKTag_String20,
    kHAPOPACKTag_String21,
    kHAPOPACKTag_String22,
    kHAPOPACKTag_String23,
    kHAPOPACKTag_String24,
    kHAPOPACKTag_String25,
    kHAPOPACKTag_String26,
    kHAPOPACKTag_String27,
    kHAPOPACKTag_String28,
    kHAPOPACKTag_String29,
    kHAPOPACKTag_String30,
    kHAPOPACKTag_String31,
    kHAPOPACKTag_String32,
    /**@}*/

    /** <1:length> <n:UTF-8 bytes>. Not NUL terminated. */
    kHAPOPACKTag_StringUInt8 = 0x61,

    /** <2:little endian length> <n:UTF-8 bytes>. Not NUL terminated. */
    kHAPOPACKTag_StringUInt16 = 0x62,

    /** <4:little endian length> <n:UTF-8 bytes>. Not NUL terminated. */
    kHAPOPACKTag_StringUInt32 = 0x63,

    /** <8:little endian length> <n:UTF-8 bytes>. Not NUL terminated. */
    kHAPOPACKTag_StringUInt64 = 0x64,

    /** <n:UTF-8 bytes> <1:NUL>. */
    kHAPOPACKTag_String = 0x6F,

    /** 0-32 bytes of data (length = tag - 0x70). */
    /**@{*/
    kHAPOPACKTag_Data0 = 0x70,
    kHAPOPACKTag_Data1,
    kHAPOPACKTag_Data2,
    kHAPOPACKTag_Data3,
    kHAPOPACKTag_Data4,
    kHAPOPACKTag_Data5,
    kHAPOPACKTag_Data6,
    kHAPOPACKTag_Data7,
    kHAPOPACKTag_Data8,
    kHAPOPACKTag_Data9,
    kHAPOPACKTag_Data10,
    kHAPOPACKTag_Data11,
    kHAPOPACKTag_Data12,
    kHAPOPACKTag_Data13,
    kHAPOPACKTag_Data14,
    kHAPOPACKTag_Data15,
    kHAPOPACKTag_Data16,
    kHAPOPACKTag_Data17,
    kHAPOPACKTag_Data18,
    kHAPOPACKTag_Data19,
    kHAPOPACKTag_Data20,
    kHAPOPACKTag_Data21,
    kHAPOPACKTag_Data22,
    kHAPOPACKTag_Data23,
    kHAPOPACKTag_Data24,
    kHAPOPACKTag_Data25,
    kHAPOPACKTag_Data26,
    kHAPOPACKTag_Data27,
    kHAPOPACKTag_Data28,
    kHAPOPACKTag_Data29,
    kHAPOPACKTag_Data30,
    kHAPOPACKTag_Data31,
    kHAPOPACKTag_Data32,
    /**@}*/

    /** <1:length> <n:bytes>. */
    kHAPOPACKTag_DataUInt8 = 0x91,

    /** <2:little endian length> <n:bytes>. */
    kHAPOPACKTag_DataUInt16 = 0x92,

    /** <4:little endian length> <n:bytes>. */
    kHAPOPACKTag_DataUInt32 = 0x93,

    /** <8:little endian length> <n:bytes>. */
    kHAPOPACKTag_DataUInt64 = 0x94,

    /** [data object 1] [data object 2] ... [data object n] <1:terminator tag>. */
    kHAPOPACKTag_Data = 0x9F,

    /** 0-14 items (count = tag - 0xD0). <object 1> <object 2> ... <object n>. */
    /**@{*/
    kHAPOPACKTag_Array0 = 0xD0,
    kHAPOPACKTag_Array1,
    kHAPOPACKTag_Array2,
    kHAPOPACKTag_Array3,
    kHAPOPACKTag_Array4,
    kHAPOPACKTag_Array5,
    kHAPOPACKTag_Array6,
    kHAPOPACKTag_Array7,
    kHAPOPACKTag_Array8,
    kHAPOPACKTag_Array9,
    kHAPOPACKTag_Array10,
    kHAPOPACKTag_Array11,
    kHAPOPACKTag_Array12,
    kHAPOPACKTag_Array13,
    kHAPOPACKTag_Array14,
    /**@}*/

    /** [object 1] [object 2] ... [object n] <1:terminator tag>. */
    kHAPOPACKTag_Array = 0xDF,

    /** 0-14 entries (count = tag - 0xE0). <key 1><value 1> <key 2><value 2> ... <key n> <value n>. */
    /**@{*/
    kHAPOPACKTag_Dictionary0 = 0xE0,
    kHAPOPACKTag_Dictionary1,
    kHAPOPACKTag_Dictionary2,
    kHAPOPACKTag_Dictionary3,
    kHAPOPACKTag_Dictionary4,
    kHAPOPACKTag_Dictionary5,
    kHAPOPACKTag_Dictionary6,
    kHAPOPACKTag_Dictionary7,
    kHAPOPACKTag_Dictionary8,
    kHAPOPACKTag_Dictionary9,
    kHAPOPACKTag_Dictionary10,
    kHAPOPACKTag_Dictionary11,
    kHAPOPACKTag_Dictionary12,
    kHAPOPACKTag_Dictionary13,
    kHAPOPACKTag_Dictionary14,
    /**@}*/

    /** [key 1][value 1] [key 2][value 2] ... [key n][value n] <1:terminator tag>. */
    kHAPOPACKTag_Dictionary = 0xEF
} HAP_ENUM_END(uint8_t, HAPOPACKTag);
HAP_STATIC_ASSERT(kHAPOPACKTag_38 == kHAPOPACKTag_0 + 38, kHAPOPACKTag_SmallIntegers);
HAP_STATIC_ASSERT(kHAPOPACKTag_String32 == kHAPOPACKTag_String0 + 32, kHAPOPACKTag_ShortStrings);
HAP_STATIC_ASSERT(kHAPOPACKTag_Data32 == kHAPOPACKTag_Data0 + 32, kHAPOPACKTag_ShortDatas);
HAP_STATIC_ASSERT(kHAPOPACKTag_Array14 == kHAPOPACKTag_Array0 + 14, kHAPOPACKTag_ShortArrays);

/**
 * OPACK format reader.
 */
typedef struct {
    void* _Nullable bytes; /**< Buffer containing OPACK data. Modified while reading. */
    size_t numBytes;       /**< Length of data in buffer. */
} HAPOPACKReader;

/**
 * Creates a new OPACK reader.
 *
 * @param      reader               An uninitialized OPACK reader.
 * @param      bytes                Buffer containing raw OPACK data. Must remain valid.
 * @param      numBytes             Length of buffer.
 */
void HAPOPACKReaderCreate(HAPOPACKReader* reader, void* _Nullable bytes, size_t numBytes);

/**
 * OPACK item types.
 */
HAP_ENUM_BEGIN(uint8_t, HAPOPACKItemType) {
    kHAPOPACKItemType_Bool = 1,  /**< Boolean. */
    kHAPOPACKItemType_Null,      /**< Null. */
    kHAPOPACKItemType_UUID,      /**< UUID. */
    kHAPOPACKItemType_Date,      /**< Date. */
    kHAPOPACKItemType_Number,    /**< Number. */
    kHAPOPACKItemType_String,    /**< String. */
    kHAPOPACKItemType_Data,      /**< Data. */
    kHAPOPACKItemType_Array,     /**< Array. */
    kHAPOPACKItemType_Dictionary /**< Dictionary. */
} HAP_ENUM_END(uint8_t, HAPOPACKItemType);

/**
 * Fetches the next OPACK item from an OPACK reader's buffer.
 *
 * - The item content can only be read with the reader function that matches the item type.
 *
 * @param      reader               Reader.
 * @param[out] found                True if an OPACK item is available. False otherwise.
 * @param[out] type                 Type of next OPACK item.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If parsing failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPOPACKReaderPeekNextType(HAPOPACKReader* reader, bool* found, HAPOPACKItemType* _Nullable type);

/**
 * Fetches the next OPACK item from an OPACK reader's buffer.
 *
 * @param      reader               Reader.
 * @param[out] itemReader           Reader encompassing the OPACK item.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If parsing failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPOPACKReaderGetNext(HAPOPACKReader* reader, HAPOPACKReader* _Nullable itemReader);

/**
 * Fetches the next OPACK item from an OPACK reader's buffer.
 *
 * @param      reader               Reader.
 * @param[out] value                Value.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If parsing failed or the next item does not encode a boolean value.
 */
HAP_RESULT_USE_CHECK
HAPError HAPOPACKReaderGetNextBool(HAPOPACKReader* reader, bool* _Nullable value);

/**
 * Fetches the next OPACK item from an OPACK reader's buffer.
 *
 * @param      reader               Reader.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If parsing failed or the next item does not encode a null value.
 */
HAP_RESULT_USE_CHECK
HAPError HAPOPACKReaderGetNextNull(HAPOPACKReader* reader);

/**
 * Fetches the next OPACK item from an OPACK reader's buffer.
 *
 * @param      reader               Reader.
 * @param[out] value                Value.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If parsing failed or the next item does not encode a UUID value.
 */
HAP_RESULT_USE_CHECK
HAPError HAPOPACKReaderGetNextUUID(HAPOPACKReader* reader, HAPUUID* _Nullable value);

/**
 * Fetches the next OPACK item from an OPACK reader's buffer.
 *
 * @param      reader               Reader.
 * @param[out] value                Value.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If parsing failed or the next item does not encode a date value.
 */
HAP_RESULT_USE_CHECK
HAPError HAPOPACKReaderGetNextDate(HAPOPACKReader* reader, HAPOPACKDate* _Nullable value);

/**
 * Fetches the next OPACK item from an OPACK reader's buffer.
 *
 * @param      reader               Reader.
 * @param[out] value                Value.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If parsing failed or the next item does not encode a number value.
 */
HAP_RESULT_USE_CHECK
HAPError HAPOPACKReaderGetNextNumber(HAPOPACKReader* reader, HAPOPACKNumber* _Nullable value);

/**
 * Fetches the next OPACK item from an OPACK reader's buffer.
 *
 * @param      reader               Reader.
 * @param[out] value                Value.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If parsing failed or the next item does not encode an integer value.
 */
HAP_RESULT_USE_CHECK
HAPError HAPOPACKReaderGetNextInt(HAPOPACKReader* reader, int64_t* _Nullable value);

/**
 * Fetches the next OPACK item from an OPACK reader's buffer.
 *
 * @param      reader               Reader.
 * @param[out] value                Value.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If parsing failed or the next item does not encode a floating point value.
 */
HAP_RESULT_USE_CHECK
HAPError HAPOPACKReaderGetNextFloat(HAPOPACKReader* reader, double* _Nullable value);

/**
 * Fetches the next OPACK item from an OPACK reader's buffer.
 *
 * @param      reader               Reader. If @p value is non-NULL, reader buffer will be modified by the reader.
 * @param[out] value                Value. NULL-terminated.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If parsing failed or the next item does not encode a string value.
 */
HAP_RESULT_USE_CHECK
HAPError HAPOPACKReaderGetNextString(HAPOPACKReader* reader, char* _Nonnull* _Nullable value);

/**
 * Fetches the next OPACK item from an OPACK reader's buffer.
 *
 * - This function may only be used if the next item encodes a string value.
 *
 * @param      reader               Reader. If @p valueBytes is non-NULL, reader buffer will be modified by the reader.
 * @param[out] valueBytes           Value buffer.
 * @param[out] numValueBytes        Length of value buffer.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If parsing failed or the next item does not encode a data value.
 */
HAP_RESULT_USE_CHECK
HAPError HAPOPACKReaderGetNextData(
        HAPOPACKReader* reader,
        void* _Nonnull* _Nullable valueBytes,
        size_t* _Nullable numValueBytes);

/**
 * Fetches the next OPACK item from an OPACK reader's buffer.
 *
 * - This function may only be used if the next item encodes an array value.
 *
 * - Array elements can be read sequentially from the sub-reader.
 *
 * @param      reader               Reader.
 * @param[out] subReader            Sub-reader to read elements from.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If parsing failed or the next item does not encode an array value.
 */
HAP_RESULT_USE_CHECK
HAPError HAPOPACKReaderGetNextArray(HAPOPACKReader* reader, HAPOPACKReader* _Nullable subReader);

/**
 * Fetches the next OPACK item from an OPACK reader's buffer.
 *
 * - This function may only be used if the next item encodes a dictionary value.
 *
 * - Dictionary elements can be read sequentially from the sub-reader, alternating between keys and values.
 *
 * @param      reader               Reader.
 * @param[out] subReader            Sub-reader to read elements from.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If parsing failed or the next item does not encode a dictionary value.
 */
HAP_RESULT_USE_CHECK
HAPError HAPOPACKReaderGetNextDictionary(HAPOPACKReader* reader, HAPOPACKReader* _Nullable subReader);

/**
 * Element of an OPACK dictionary with String keys.
 */
typedef struct {
    /** Key. */
    const char* _Nullable key;

    /** Value. */
    struct {
        /** Whether a value exists. */
        bool exists : 1;

        /** Reader encompassing the value. Only valid if value exists. */
        HAPOPACKReader reader;
    } value;
} HAPOPACKStringDictionaryElement;

/**
 * Fetches all interesting elements of an OPACK dictionary with String keys.
 *
 * On input, @p elements is a NULL-terminated array to OPACK dictionary elements.
 * For each element, the key may be specified. Keys must be unique.
 *
 * - If the key is not specified, the element will be filled with an arbitrary OPACK dictionary element
 *   that has not been explicitly requested, if one is available. Elements are filled in order.
 *
 * On output, @p elements are updated to contain the actual OPACK dictionary elements.
 * If multiple OPACK dictionary elements with one of the requested keys are found, an error is returned.
 *
 * - This API must be used on a freshly initialized OPACK format reader. All OPACK items will be read.
 *
 * @param     reader                Reader.
 * @param[in,out] elements          NULL-terminated array to OPACK dictionary element structures.
 *                                  On input, keys are specified. On output, actual dictionary elements are filled in.
 *                                  Each key may only be requested once.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If parsing failed or the item does not encode a dictionary value.
 */
HAP_RESULT_USE_CHECK
HAPError HAPOPACKStringDictionaryReaderGetAll(
        HAPOPACKReader* reader,
        HAPOPACKStringDictionaryElement* _Nullable const* _Nonnull elements);

/**
 * OPACK format writer.
 */
typedef struct {
    void* bytes;     /**< Buffer containing serialized OPACK data. */
    size_t maxBytes; /**< Capacity of buffer. */
    size_t numBytes; /**< Length of serialized OPACK data. */
} HAPOPACKWriter;

/**
 * Creates a new OPACK writer.
 *
 * @param      writer               An uninitialized OPACK writer.
 * @param      bytes                Buffer to serialize OPACK data into. Must remain valid.
 * @param      maxBytes             Capacity of the buffer.
 */
void HAPOPACKWriterCreate(HAPOPACKWriter* writer, void* bytes, size_t maxBytes);

/**
 * Retrieves the buffer containing the serialized OPACK data.
 *
 * @param      writer               Writer.
 * @param[out] bytes                Buffer containing serialized OPACK data written so far.
 * @param[out] numBytes             Length of buffer.
 */
void HAPOPACKWriterGetBuffer(HAPOPACKWriter* writer, void* _Nonnull* _Nonnull bytes, size_t* numBytes);

/**
 * Appends a boolean value to an OPACK writer's buffer.
 *
 * @param      writer               Writer.
 * @param      value                Value to append.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the writer's buffer is not large enough to hold the serialized value.
 */
HAP_RESULT_USE_CHECK
HAPError HAPOPACKWriterAppendBool(HAPOPACKWriter* writer, bool value);

/**
 * Appends a null value to an OPACK writer's buffer.
 *
 * @param      writer               Writer.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the writer's buffer is not large enough to hold the serialized value.
 */
HAP_RESULT_USE_CHECK
HAPError HAPOPACKWriterAppendNull(HAPOPACKWriter* writer);

/**
 * Appends a UUID value to an OPACK writer's buffer.
 *
 * @param      writer               Writer.
 * @param      value                Value to append.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the writer's buffer is not large enough to hold the serialized value.
 */
HAP_RESULT_USE_CHECK
HAPError HAPOPACKWriterAppendUUID(HAPOPACKWriter* writer, const HAPUUID* value);

/**
 * Appends a date value to an OPACK writer's buffer.
 *
 * @param      writer               Writer.
 * @param      value                Value to append.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the writer's buffer is not large enough to hold the serialized value.
 */
HAP_RESULT_USE_CHECK
HAPError HAPOPACKWriterAppendDate(HAPOPACKWriter* writer, HAPOPACKDate value);

/**
 * Appends an integer value to an OPACK writer's buffer.
 *
 * @param      writer               Writer.
 * @param      value                Value to append.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the writer's buffer is not large enough to hold the serialized value.
 */
HAP_RESULT_USE_CHECK
HAPError HAPOPACKWriterAppendInt(HAPOPACKWriter* writer, int64_t value);

/**
 * Appends a floating point value to an OPACK writer's buffer.
 *
 * @param      writer               Writer.
 * @param      value                Value to append.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the writer's buffer is not large enough to hold the serialized value.
 */
HAP_RESULT_USE_CHECK
HAPError HAPOPACKWriterAppendFloat(HAPOPACKWriter* writer, double value);

/**
 * Appends a number value to an OPACK writer's buffer.
 *
 * @param      writer               Writer.
 * @param      value                Value to append.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the writer's buffer is not large enough to hold the serialized value.
 */
HAP_RESULT_USE_CHECK
HAPError HAPOPACKWriterAppendNumber(HAPOPACKWriter* writer, const HAPOPACKNumber* value);

/**
 * Appends a string value to an OPACK writer's buffer.
 *
 * @param      writer               Writer.
 * @param      value                Value to append. NULL-terminated.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the writer's buffer is not large enough to hold the serialized value.
 */
HAP_RESULT_USE_CHECK
HAPError HAPOPACKWriterAppendString(HAPOPACKWriter* writer, const char* value);

/**
 * Appends a data value to an OPACK writer's buffer.
 *
 * @param      writer               Writer.
 * @param      valueBytes           Value buffer to append.
 * @param      numValueBytes        Length of value buffer.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the writer's buffer is not large enough to hold the serialized value.
 */
HAP_RESULT_USE_CHECK
HAPError HAPOPACKWriterAppendData(HAPOPACKWriter* writer, const void* valueBytes, size_t numValueBytes);

/**
 * Appends a marker to begin an array to an OPACK writer's buffer.
 *
 * - After this marker, append all array elements in order, then append a terminator tag to finalize.
 *
 * @param      writer               Writer.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the writer's buffer is not large enough to hold the serialized value.
 *
 * **Example**

   @code{.c}

   HAPOPACKWriter writer;

   err = HAPOPACKWriterAppendArrayBegin(&writer);
   if (err) {
       // Handle error.
   }
   {
       err = HAPOPACKWriterAppendXxx(&writer, element1);
       if (err) {
           // Handle error.
       }

       err = HAPOPACKWriterAppendXxx(&writer, element2);
       if (err) {
           // Handle error.
       }
   }
   err = HAPOPACKWriterAppendTerminator(&writer);
   if (err) {
       // Handle error.
   }

   @endcode
 */
HAP_RESULT_USE_CHECK
HAPError HAPOPACKWriterAppendArrayBegin(HAPOPACKWriter* writer);

/**
 * Appends a marker to begin a dictionary to an OPACK writer's buffer.
 *
 * - After this marker, append all dictionary elements in order, then append a terminator tag to finalize.
 *
 * - Each dictionary element must be appended by first appending the key, then appending the value.
 *
 * @param      writer               Writer.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the writer's buffer is not large enough to hold the serialized value.
 *
 * **Example**

   @code{.c}

   HAPOPACKWriter writer;

   err = HAPOPACKWriterAppendDictionaryBegin(&writer);
   if (err) {
       // Handle error.
   }
   {
       err = HAPOPACKWriterAppendXxx(&writer, key1);
       if (err) {
           // Handle error.
       }
       err = HAPOPACKWriterAppendXxx(&writer, element1);
       if (err) {
           // Handle error.
       }

       err = HAPOPACKWriterAppendXxx(&writer, key2);
       if (err) {
           // Handle error.
       }
       err = HAPOPACKWriterAppendXxx(&writer, element2);
       if (err) {
           // Handle error.
       }
   }
   err = HAPOPACKWriterAppendTerminator(&writer);
   if (err) {
       // Handle error.
   }

   @endcode
 */
HAP_RESULT_USE_CHECK
HAPError HAPOPACKWriterAppendDictionaryBegin(HAPOPACKWriter* writer);

/**
 * Appends a terminator value to an OPACK writer's buffer.
 *
 * @param      writer               Writer.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the writer's buffer is not large enough to hold the serialized value.
 */
HAP_RESULT_USE_CHECK
HAPError HAPOPACKWriterAppendTerminator(HAPOPACKWriter* writer);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
