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
#include "HAPOPACK.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "OPACKReader" };

// See HomeKit Accessory Protocol Specification R17
// Section 9.1.4 Data Format

/**
 * Maximum recursion depth for received arrays and dictionaries.
 *
 * - Arbitrary choice, not backed by spec.
 */
#define kHAPOPACKReader_MaxRecursionDepth ((size_t) 20)

void HAPOPACKReaderCreate(HAPOPACKReader* reader, void* _Nullable bytes, size_t numBytes) {
    HAPPrecondition(reader);
    HAPPrecondition(!numBytes || bytes);

    // Initialize reader.
    HAPRawBufferZero(reader, sizeof *reader);
    reader->bytes = bytes;
    reader->numBytes = numBytes;
}

HAP_RESULT_USE_CHECK
HAPError HAPOPACKReaderPeekNextType(HAPOPACKReader* reader, bool* found, HAPOPACKItemType* _Nullable type) {
    HAPPrecondition(reader);
    HAPPrecondition(found);

    *found = false;

    if (!reader->numBytes) {
        return kHAPError_None;
    }
    HAPAssert(reader->bytes);

    uint8_t* b = reader->bytes;

    switch (b[0]) {
        case kHAPOPACKTag_True:
        case kHAPOPACKTag_False: {
            *found = true;
            if (type) {
                *type = kHAPOPACKItemType_Bool;
            }
        }
            return kHAPError_None;
        case kHAPOPACKTag_Terminator: {
            HAPLog(&logObject, "Unexpected OPACK terminator item.");
        }
            return kHAPError_InvalidData;
        case kHAPOPACKTag_Null: {
            *found = true;
            if (type) {
                *type = kHAPOPACKItemType_Null;
            }
        }
            return kHAPError_None;
        case kHAPOPACKTag_UUID: {
            *found = true;
            if (type) {
                *type = kHAPOPACKItemType_UUID;
            }
        }
            return kHAPError_None;
        case kHAPOPACKTag_Date: {
            *found = true;
            if (type) {
                *type = kHAPOPACKItemType_Date;
            }
        }
            return kHAPError_None;
        case kHAPOPACKTag_Negative1:
        case kHAPOPACKTag_0:
        case kHAPOPACKTag_1:
        case kHAPOPACKTag_2:
        case kHAPOPACKTag_3:
        case kHAPOPACKTag_4:
        case kHAPOPACKTag_5:
        case kHAPOPACKTag_6:
        case kHAPOPACKTag_7:
        case kHAPOPACKTag_8:
        case kHAPOPACKTag_9:
        case kHAPOPACKTag_10:
        case kHAPOPACKTag_11:
        case kHAPOPACKTag_12:
        case kHAPOPACKTag_13:
        case kHAPOPACKTag_14:
        case kHAPOPACKTag_15:
        case kHAPOPACKTag_16:
        case kHAPOPACKTag_17:
        case kHAPOPACKTag_18:
        case kHAPOPACKTag_19:
        case kHAPOPACKTag_20:
        case kHAPOPACKTag_21:
        case kHAPOPACKTag_22:
        case kHAPOPACKTag_23:
        case kHAPOPACKTag_24:
        case kHAPOPACKTag_25:
        case kHAPOPACKTag_26:
        case kHAPOPACKTag_27:
        case kHAPOPACKTag_28:
        case kHAPOPACKTag_29:
        case kHAPOPACKTag_30:
        case kHAPOPACKTag_31:
        case kHAPOPACKTag_32:
        case kHAPOPACKTag_33:
        case kHAPOPACKTag_34:
        case kHAPOPACKTag_35:
        case kHAPOPACKTag_36:
        case kHAPOPACKTag_37:
        case kHAPOPACKTag_38:
        case kHAPOPACKTag_39:
        case kHAPOPACKTag_Int8:
        case kHAPOPACKTag_Int16:
        case kHAPOPACKTag_Int32:
        case kHAPOPACKTag_Int64:
        case kHAPOPACKTag_Float:
        case kHAPOPACKTag_Double: {
            *found = true;
            if (type) {
                *type = kHAPOPACKItemType_Number;
            }
        }
            return kHAPError_None;
        case kHAPOPACKTag_String0:
        case kHAPOPACKTag_String1:
        case kHAPOPACKTag_String2:
        case kHAPOPACKTag_String3:
        case kHAPOPACKTag_String4:
        case kHAPOPACKTag_String5:
        case kHAPOPACKTag_String6:
        case kHAPOPACKTag_String7:
        case kHAPOPACKTag_String8:
        case kHAPOPACKTag_String9:
        case kHAPOPACKTag_String10:
        case kHAPOPACKTag_String11:
        case kHAPOPACKTag_String12:
        case kHAPOPACKTag_String13:
        case kHAPOPACKTag_String14:
        case kHAPOPACKTag_String15:
        case kHAPOPACKTag_String16:
        case kHAPOPACKTag_String17:
        case kHAPOPACKTag_String18:
        case kHAPOPACKTag_String19:
        case kHAPOPACKTag_String20:
        case kHAPOPACKTag_String21:
        case kHAPOPACKTag_String22:
        case kHAPOPACKTag_String23:
        case kHAPOPACKTag_String24:
        case kHAPOPACKTag_String25:
        case kHAPOPACKTag_String26:
        case kHAPOPACKTag_String27:
        case kHAPOPACKTag_String28:
        case kHAPOPACKTag_String29:
        case kHAPOPACKTag_String30:
        case kHAPOPACKTag_String31:
        case kHAPOPACKTag_String32:
        case kHAPOPACKTag_StringUInt8:
        case kHAPOPACKTag_StringUInt16:
        case kHAPOPACKTag_StringUInt32:
        case kHAPOPACKTag_StringUInt64:
        case kHAPOPACKTag_String: {
            *found = true;
            if (type) {
                *type = kHAPOPACKItemType_String;
            }
        }
            return kHAPError_None;
        case kHAPOPACKTag_Data0:
        case kHAPOPACKTag_Data1:
        case kHAPOPACKTag_Data2:
        case kHAPOPACKTag_Data3:
        case kHAPOPACKTag_Data4:
        case kHAPOPACKTag_Data5:
        case kHAPOPACKTag_Data6:
        case kHAPOPACKTag_Data7:
        case kHAPOPACKTag_Data8:
        case kHAPOPACKTag_Data9:
        case kHAPOPACKTag_Data10:
        case kHAPOPACKTag_Data11:
        case kHAPOPACKTag_Data12:
        case kHAPOPACKTag_Data13:
        case kHAPOPACKTag_Data14:
        case kHAPOPACKTag_Data15:
        case kHAPOPACKTag_Data16:
        case kHAPOPACKTag_Data17:
        case kHAPOPACKTag_Data18:
        case kHAPOPACKTag_Data19:
        case kHAPOPACKTag_Data20:
        case kHAPOPACKTag_Data21:
        case kHAPOPACKTag_Data22:
        case kHAPOPACKTag_Data23:
        case kHAPOPACKTag_Data24:
        case kHAPOPACKTag_Data25:
        case kHAPOPACKTag_Data26:
        case kHAPOPACKTag_Data27:
        case kHAPOPACKTag_Data28:
        case kHAPOPACKTag_Data29:
        case kHAPOPACKTag_Data30:
        case kHAPOPACKTag_Data31:
        case kHAPOPACKTag_Data32:
        case kHAPOPACKTag_DataUInt8:
        case kHAPOPACKTag_DataUInt16:
        case kHAPOPACKTag_DataUInt32:
        case kHAPOPACKTag_DataUInt64:
        case kHAPOPACKTag_Data: {
            *found = true;
            if (type) {
                *type = kHAPOPACKItemType_Data;
            }
        }
            return kHAPError_None;
        case kHAPOPACKTag_Array0:
        case kHAPOPACKTag_Array1:
        case kHAPOPACKTag_Array2:
        case kHAPOPACKTag_Array3:
        case kHAPOPACKTag_Array4:
        case kHAPOPACKTag_Array5:
        case kHAPOPACKTag_Array6:
        case kHAPOPACKTag_Array7:
        case kHAPOPACKTag_Array8:
        case kHAPOPACKTag_Array9:
        case kHAPOPACKTag_Array10:
        case kHAPOPACKTag_Array11:
        case kHAPOPACKTag_Array12:
        case kHAPOPACKTag_Array13:
        case kHAPOPACKTag_Array14:
        case kHAPOPACKTag_Array: {
            *found = true;
            if (type) {
                *type = kHAPOPACKItemType_Array;
            }
        }
            return kHAPError_None;
        case kHAPOPACKTag_Dictionary0:
        case kHAPOPACKTag_Dictionary1:
        case kHAPOPACKTag_Dictionary2:
        case kHAPOPACKTag_Dictionary3:
        case kHAPOPACKTag_Dictionary4:
        case kHAPOPACKTag_Dictionary5:
        case kHAPOPACKTag_Dictionary6:
        case kHAPOPACKTag_Dictionary7:
        case kHAPOPACKTag_Dictionary8:
        case kHAPOPACKTag_Dictionary9:
        case kHAPOPACKTag_Dictionary10:
        case kHAPOPACKTag_Dictionary11:
        case kHAPOPACKTag_Dictionary12:
        case kHAPOPACKTag_Dictionary13:
        case kHAPOPACKTag_Dictionary14:
        case kHAPOPACKTag_Dictionary: {
            *found = true;
            if (type) {
                *type = kHAPOPACKItemType_Dictionary;
            }
        }
            return kHAPError_None;
        default: {
            HAPLog(&logObject, "Unknown OPACK item type: 0x%02X", b[0]);
        }
            return kHAPError_InvalidData;
    }
}

HAP_RESULT_USE_CHECK
static HAPError HAPOPACKReaderGetNextArray_(
        HAPOPACKReader* reader,
        HAPOPACKReader* _Nullable const subReader,
        size_t recursionDepth);

HAP_RESULT_USE_CHECK
static HAPError HAPOPACKReaderGetNextDictionary_(
        HAPOPACKReader* reader,
        HAPOPACKReader* _Nullable const subReader,
        size_t recursionDepth);

HAP_RESULT_USE_CHECK
static HAPError HAPOPACKReaderGetNext_(
        HAPOPACKReader* reader,
        HAPOPACKReader* _Nullable const itemReader,
        size_t recursionDepth) {
    HAPPrecondition(reader);

    HAPError err;

    if (!recursionDepth) {
        HAPLog(&logObject, "OPACK item contains too many levels of recursion.");
        return kHAPError_InvalidData;
    }

    bool found;
    HAPOPACKItemType type;
    err = HAPOPACKReaderPeekNextType(reader, &found, &type);
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }
    if (!found) {
        HAPLog(&logObject, "End of OPACK data has been reached.");
        return kHAPError_InvalidData;
    }

    uint8_t* b = reader->bytes;

    switch (type) {
        case kHAPOPACKItemType_Bool: {
            err = HAPOPACKReaderGetNextBool(reader, NULL);
        }
            goto done;
        case kHAPOPACKItemType_Null: {
            err = HAPOPACKReaderGetNextNull(reader);
        }
            goto done;
        case kHAPOPACKItemType_UUID: {
            err = HAPOPACKReaderGetNextUUID(reader, NULL);
        }
            goto done;
        case kHAPOPACKItemType_Date: {
            err = HAPOPACKReaderGetNextDate(reader, NULL);
        }
            goto done;
        case kHAPOPACKItemType_Number: {
            err = HAPOPACKReaderGetNextNumber(reader, NULL);
        }
            goto done;
        case kHAPOPACKItemType_String: {
            err = HAPOPACKReaderGetNextString(reader, NULL);
        }
            goto done;
        case kHAPOPACKItemType_Data: {
            err = HAPOPACKReaderGetNextData(reader, NULL, NULL);
        }
            goto done;
        case kHAPOPACKItemType_Array: {
            err = HAPOPACKReaderGetNextArray_(reader, NULL, recursionDepth - 1);
        }
            goto done;
        case kHAPOPACKItemType_Dictionary: {
            err = HAPOPACKReaderGetNextDictionary_(reader, NULL, recursionDepth - 1);
        }
            goto done;
        done : {
            if (err) {
                HAPAssert(err == kHAPError_InvalidData);
                return err;
            }
            if (itemReader) {
                HAPOPACKReaderCreate(itemReader, b, (size_t)((uint8_t*) reader->bytes - b));
            }
        }
            return kHAPError_None;
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPOPACKReaderGetNext(HAPOPACKReader* reader, HAPOPACKReader* _Nullable itemReader) {
    return HAPOPACKReaderGetNext_(reader, itemReader, kHAPOPACKReader_MaxRecursionDepth);
}

#define CHECK_SIZE_OR_RETURN(n, description) \
    do { \
        if (reader->numBytes - o < (n)) { \
            HAPLog(&logObject, "Found incomplete OPACK item (%s).", (description)); \
            return kHAPError_InvalidData; \
        } \
    } while (0)

#define UPDATE_READER(n) \
    do { \
        HAPAssert(reader->numBytes >= (n)); \
        reader->numBytes -= (n); \
        reader->bytes = &((uint8_t*) reader->bytes)[n]; \
    } while (0)

HAP_RESULT_USE_CHECK
HAPError HAPOPACKReaderGetNextBool(HAPOPACKReader* reader, bool* _Nullable value) {
    HAPPrecondition(reader);

    size_t o = 0;
    CHECK_SIZE_OR_RETURN(sizeof(HAPOPACKTag), "tag");
    HAPAssert(reader->bytes);
    uint8_t* b = reader->bytes;

    switch (b[o++]) {
        case kHAPOPACKTag_True: {
            if (value) {
                *value = true;
            }
            break;
        }
        case kHAPOPACKTag_False: {
            if (value) {
                *value = false;
            }
            break;
        }
        default: {
            HAPLogError(&logObject, "OPACK item is not a boolean value.");
        }
            return kHAPError_InvalidData;
    }

    UPDATE_READER(o);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPOPACKReaderGetNextNull(HAPOPACKReader* reader) {
    HAPPrecondition(reader);

    size_t o = 0;
    CHECK_SIZE_OR_RETURN(sizeof(HAPOPACKTag), "tag");
    HAPAssert(reader->bytes);
    uint8_t* b = reader->bytes;

    switch (b[o++]) {
        case kHAPOPACKTag_Null: {
            break;
        }
        default: {
            HAPLogError(&logObject, "OPACK item is not a null value.");
        }
            return kHAPError_InvalidData;
    }

    UPDATE_READER(o);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPOPACKReaderGetNextUUID(HAPOPACKReader* reader, HAPUUID* _Nullable value) {
    HAPPrecondition(reader);

    size_t o = 0;
    CHECK_SIZE_OR_RETURN(sizeof(HAPOPACKTag), "tag");
    HAPAssert(reader->bytes);
    uint8_t* b = reader->bytes;

    switch (b[o++]) {
        case kHAPOPACKTag_UUID: {
            CHECK_SIZE_OR_RETURN(sizeof value->bytes, "value");
            if (value) {
                HAPRawBufferZero(HAPNonnull(value), sizeof *value);
                for (size_t i = 0; i < sizeof value->bytes; i++) {
                    value->bytes[i] = b[o + sizeof value->bytes - 1 - i];
                }
            }
            o += sizeof value->bytes;
            break;
        }
        default: {
            HAPLogError(&logObject, "OPACK item is not a UUID value.");
        }
            return kHAPError_InvalidData;
    }

    UPDATE_READER(o);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPOPACKReaderGetNextDate(HAPOPACKReader* reader, HAPOPACKDate* _Nullable value) {
    HAPPrecondition(reader);

    size_t o = 0;
    CHECK_SIZE_OR_RETURN(sizeof(HAPOPACKTag), "tag");
    HAPAssert(reader->bytes);
    uint8_t* b = reader->bytes;

    switch (b[o++]) {
        case kHAPOPACKTag_Date: {
            CHECK_SIZE_OR_RETURN(sizeof *value, "value");
            if (value) {
                HAPAssert(sizeof *value == sizeof(uint64_t));
                uint64_t bitPattern = HAPReadLittleUInt64(&b[o]);
                *value = HAPDoubleFromBitPattern(bitPattern);
            }
            o += sizeof *value;
            break;
        }
        default: {
            HAPLogError(&logObject, "OPACK item is not a date value.");
        }
            return kHAPError_InvalidData;
    }

    UPDATE_READER(o);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPOPACKReaderGetNextNumber(HAPOPACKReader* reader, HAPOPACKNumber* _Nullable value) {
    HAPPrecondition(reader);

    size_t o = 0;
    CHECK_SIZE_OR_RETURN(sizeof(HAPOPACKTag), "tag");
    HAPAssert(reader->bytes);
    uint8_t* b = reader->bytes;

    switch (b[o++]) {
        case kHAPOPACKTag_Negative1: {
            if (value) {
                int64_t val = -1;
                HAPOPACKNumberCreate(HAPNonnull(value), kHAPOPACKNumberType_Int64, &val);
            }
            break;
        }
        case kHAPOPACKTag_0:
        case kHAPOPACKTag_1:
        case kHAPOPACKTag_2:
        case kHAPOPACKTag_3:
        case kHAPOPACKTag_4:
        case kHAPOPACKTag_5:
        case kHAPOPACKTag_6:
        case kHAPOPACKTag_7:
        case kHAPOPACKTag_8:
        case kHAPOPACKTag_9:
        case kHAPOPACKTag_10:
        case kHAPOPACKTag_11:
        case kHAPOPACKTag_12:
        case kHAPOPACKTag_13:
        case kHAPOPACKTag_14:
        case kHAPOPACKTag_15:
        case kHAPOPACKTag_16:
        case kHAPOPACKTag_17:
        case kHAPOPACKTag_18:
        case kHAPOPACKTag_19:
        case kHAPOPACKTag_20:
        case kHAPOPACKTag_21:
        case kHAPOPACKTag_22:
        case kHAPOPACKTag_23:
        case kHAPOPACKTag_24:
        case kHAPOPACKTag_25:
        case kHAPOPACKTag_26:
        case kHAPOPACKTag_27:
        case kHAPOPACKTag_28:
        case kHAPOPACKTag_29:
        case kHAPOPACKTag_30:
        case kHAPOPACKTag_31:
        case kHAPOPACKTag_32:
        case kHAPOPACKTag_33:
        case kHAPOPACKTag_34:
        case kHAPOPACKTag_35:
        case kHAPOPACKTag_36:
        case kHAPOPACKTag_37:
        case kHAPOPACKTag_38: {
            if (value) {
                int64_t val = b[0] - kHAPOPACKTag_0;
                HAPOPACKNumberCreate(HAPNonnull(value), kHAPOPACKNumberType_Int64, &val);
            }
            break;
        }
        case kHAPOPACKTag_39: {
            if (value) {
                int64_t val = 39;
                HAPOPACKNumberCreate(HAPNonnull(value), kHAPOPACKNumberType_Int64, &val);
            }
            break;
        }
        case kHAPOPACKTag_Int8: {
            CHECK_SIZE_OR_RETURN(sizeof(int8_t), "value");
            if (value) {
                int64_t val = (int8_t) b[o];
                HAPOPACKNumberCreate(HAPNonnull(value), kHAPOPACKNumberType_Int64, &val);
            }
            o++;
            break;
        }
        case kHAPOPACKTag_Int16: {
            CHECK_SIZE_OR_RETURN(sizeof(int16_t), "value");
            if (value) {
                int64_t val = HAPReadLittleInt16(&b[o]);
                HAPOPACKNumberCreate(HAPNonnull(value), kHAPOPACKNumberType_Int64, &val);
            }
            o += sizeof(int16_t);
            break;
        }
        case kHAPOPACKTag_Int32: {
            CHECK_SIZE_OR_RETURN(sizeof(int32_t), "value");
            if (value) {
                int64_t val = HAPReadLittleInt32(&b[o]);
                HAPOPACKNumberCreate(HAPNonnull(value), kHAPOPACKNumberType_Int64, &val);
            }
            o += sizeof(int32_t);
            break;
        }
        case kHAPOPACKTag_Int64: {
            CHECK_SIZE_OR_RETURN(sizeof(int64_t), "value");
            if (value) {
                int64_t val = HAPReadLittleInt64(&b[o]);
                HAPOPACKNumberCreate(HAPNonnull(value), kHAPOPACKNumberType_Int64, &val);
            }
            o += sizeof(int64_t);
            break;
        }
        case kHAPOPACKTag_Float: {
            CHECK_SIZE_OR_RETURN(sizeof(float), "value");
            if (value) {
                uint32_t bitPattern = HAPReadLittleUInt32(&b[o]);
                double val = (double) HAPFloatFromBitPattern(bitPattern);
                HAPOPACKNumberCreate(HAPNonnull(value), kHAPOPACKNumberType_Double, &val);
            }
            o += sizeof(float);
            break;
        }
        case kHAPOPACKTag_Double: {
            CHECK_SIZE_OR_RETURN(sizeof(double), "value");
            if (value) {
                uint64_t bitPattern = HAPReadLittleUInt64(&b[o]);
                double val = HAPDoubleFromBitPattern(bitPattern);
                HAPOPACKNumberCreate(HAPNonnull(value), kHAPOPACKNumberType_Double, &val);
            }
            o += sizeof(double);
            break;
        }
        default: {
            HAPLogError(&logObject, "OPACK item is not a number value.");
        }
            return kHAPError_InvalidData;
    }

    UPDATE_READER(o);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPOPACKReaderGetNextInt(HAPOPACKReader* reader, int64_t* _Nullable value) {
    HAPPrecondition(reader);

    HAPError err;

    HAPOPACKNumber number;
    err = HAPOPACKReaderGetNextNumber(reader, &number);
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }

    if (value) {
        bool exact = HAPOPACKNumberGetValue(&number, kHAPOPACKNumberType_Int64, HAPNonnull(value));
        if (!exact) {
            HAPLog(&logObject, "OPACK number is not an integer value.");
            return kHAPError_InvalidData;
        }
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPOPACKReaderGetNextFloat(HAPOPACKReader* reader, double* _Nullable value) {
    HAPPrecondition(reader);

    HAPError err;

    HAPOPACKNumber number;
    err = HAPOPACKReaderGetNextNumber(reader, &number);
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }

    if (value) {
        bool exact = HAPOPACKNumberGetValue(&number, kHAPOPACKNumberType_Double, HAPNonnull(value));
        if (!exact) {
            HAPLog(&logObject, "OPACK number is not a floating point value.");
            return kHAPError_InvalidData;
        }
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPOPACKReaderGetNextString(HAPOPACKReader* reader, char* _Nonnull* _Nullable value) {
    HAPPrecondition(reader);

    size_t o = 0;
    CHECK_SIZE_OR_RETURN(sizeof(HAPOPACKTag), "tag");
    HAPAssert(reader->bytes);
    uint8_t* b = reader->bytes;

    uint64_t numBytes;

    switch (b[o++]) {
        case kHAPOPACKTag_String0:
        case kHAPOPACKTag_String1:
        case kHAPOPACKTag_String2:
        case kHAPOPACKTag_String3:
        case kHAPOPACKTag_String4:
        case kHAPOPACKTag_String5:
        case kHAPOPACKTag_String6:
        case kHAPOPACKTag_String7:
        case kHAPOPACKTag_String8:
        case kHAPOPACKTag_String9:
        case kHAPOPACKTag_String10:
        case kHAPOPACKTag_String11:
        case kHAPOPACKTag_String12:
        case kHAPOPACKTag_String13:
        case kHAPOPACKTag_String14:
        case kHAPOPACKTag_String15:
        case kHAPOPACKTag_String16:
        case kHAPOPACKTag_String17:
        case kHAPOPACKTag_String18:
        case kHAPOPACKTag_String19:
        case kHAPOPACKTag_String20:
        case kHAPOPACKTag_String21:
        case kHAPOPACKTag_String22:
        case kHAPOPACKTag_String23:
        case kHAPOPACKTag_String24:
        case kHAPOPACKTag_String25:
        case kHAPOPACKTag_String26:
        case kHAPOPACKTag_String27:
        case kHAPOPACKTag_String28:
        case kHAPOPACKTag_String29:
        case kHAPOPACKTag_String30:
        case kHAPOPACKTag_String31:
        case kHAPOPACKTag_String32: {
            numBytes = b[0] - kHAPOPACKTag_String0;
        }
            goto copyValue;
        case kHAPOPACKTag_StringUInt8: {
            CHECK_SIZE_OR_RETURN(sizeof(uint8_t), "length");
            numBytes = b[o++];
        }
            goto copyValue;
        case kHAPOPACKTag_StringUInt16: {
            CHECK_SIZE_OR_RETURN(sizeof(uint16_t), "length");
            numBytes = HAPReadLittleUInt16(&b[o]);
            o += sizeof(uint16_t);
        }
            goto copyValue;
        case kHAPOPACKTag_StringUInt32: {
            CHECK_SIZE_OR_RETURN(sizeof(uint32_t), "length");
            numBytes = HAPReadLittleUInt32(&b[o]);
            o += sizeof(uint32_t);
        }
            goto copyValue;
        case kHAPOPACKTag_StringUInt64: {
            CHECK_SIZE_OR_RETURN(sizeof(uint64_t), "length");
            numBytes = HAPReadLittleUInt64(&b[o]);
            o += sizeof(uint64_t);
        }
            goto copyValue;
        copyValue : {
            if (numBytes > SIZE_MAX) {
                HAPLog(&logObject, "OPACK String is too long.");
                return kHAPError_InvalidData;
            }
            CHECK_SIZE_OR_RETURN(numBytes, "value");
            if (value) {
                HAPRawBufferCopyBytes(&b[0], &b[o], (size_t) numBytes);
                b[numBytes] = '\0';
                if (HAPStringGetNumBytes((const char*) &b[0]) != (size_t) numBytes) {
                    HAPLog(&logObject, "Strings with NULL characters are not supported.");
                    return kHAPError_InvalidData;
                }
                if (!HAPUTF8IsValidData(&b[0], (size_t) numBytes)) {
                    HAPLog(&logObject, "OPACK String is not a valid UTF-8 string.");
                    return kHAPError_InvalidData;
                }
                *value = (char*) &b[0];
            }
            o += (size_t) numBytes;
            break;
        }
        case kHAPOPACKTag_String: {
            if (value) {
                *value = (char*) &b[o];
            }
            do {
                CHECK_SIZE_OR_RETURN(sizeof(char), "value");
            } while (b[o++]);
            break;
        }
        default: {
            HAPLogError(&logObject, "OPACK item is not a string value.");
        }
            return kHAPError_InvalidData;
    }

    UPDATE_READER(o);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPOPACKReaderGetNextData(
        HAPOPACKReader* reader,
        void* _Nonnull* _Nullable valueBytes,
        size_t* _Nullable numValueBytes) {
    HAPPrecondition(reader);
    HAPPrecondition((valueBytes == NULL) == (numValueBytes == NULL));

    size_t o = 0;
    CHECK_SIZE_OR_RETURN(sizeof(HAPOPACKTag), "tag");
    HAPAssert(reader->bytes);
    uint8_t* b = reader->bytes;

    uint64_t numBytes;

    size_t numExpectedTerminators = 0;
    if (valueBytes && numValueBytes) {
        *valueBytes = &b[0];
        *numValueBytes = 0;
    }
    do {
        switch (b[o++]) {
            case kHAPOPACKTag_Data0:
            case kHAPOPACKTag_Data1:
            case kHAPOPACKTag_Data2:
            case kHAPOPACKTag_Data3:
            case kHAPOPACKTag_Data4:
            case kHAPOPACKTag_Data5:
            case kHAPOPACKTag_Data6:
            case kHAPOPACKTag_Data7:
            case kHAPOPACKTag_Data8:
            case kHAPOPACKTag_Data9:
            case kHAPOPACKTag_Data10:
            case kHAPOPACKTag_Data11:
            case kHAPOPACKTag_Data12:
            case kHAPOPACKTag_Data13:
            case kHAPOPACKTag_Data14:
            case kHAPOPACKTag_Data15:
            case kHAPOPACKTag_Data16:
            case kHAPOPACKTag_Data17:
            case kHAPOPACKTag_Data18:
            case kHAPOPACKTag_Data19:
            case kHAPOPACKTag_Data20:
            case kHAPOPACKTag_Data21:
            case kHAPOPACKTag_Data22:
            case kHAPOPACKTag_Data23:
            case kHAPOPACKTag_Data24:
            case kHAPOPACKTag_Data25:
            case kHAPOPACKTag_Data26:
            case kHAPOPACKTag_Data27:
            case kHAPOPACKTag_Data28:
            case kHAPOPACKTag_Data29:
            case kHAPOPACKTag_Data30:
            case kHAPOPACKTag_Data31:
            case kHAPOPACKTag_Data32: {
                numBytes = b[o - 1] - kHAPOPACKTag_Data0;
            }
                goto copyValue;
            case kHAPOPACKTag_DataUInt8: {
                CHECK_SIZE_OR_RETURN(sizeof(uint8_t), "length");
                numBytes = b[o++];
            }
                goto copyValue;
            case kHAPOPACKTag_DataUInt16: {
                CHECK_SIZE_OR_RETURN(sizeof(uint16_t), "length");
                numBytes = HAPReadLittleUInt16(&b[o]);
                o += sizeof(uint16_t);
            }
                goto copyValue;
            case kHAPOPACKTag_DataUInt32: {
                CHECK_SIZE_OR_RETURN(sizeof(uint32_t), "length");
                numBytes = HAPReadLittleUInt32(&b[o]);
                o += sizeof(uint32_t);
            }
                goto copyValue;
            case kHAPOPACKTag_DataUInt64: {
                CHECK_SIZE_OR_RETURN(sizeof(uint64_t), "length");
                numBytes = HAPReadLittleUInt64(&b[o]);
                o += sizeof(uint64_t);
            }
                goto copyValue;
            copyValue : {
                if (numBytes > SIZE_MAX) {
                    HAPLog(&logObject, "OPACK Data is too long.");
                    return kHAPError_InvalidData;
                }
                CHECK_SIZE_OR_RETURN(numBytes, "value");
                if (valueBytes && numValueBytes) {
                    void* fragmentBytes = &b[o];
                    size_t numFragmentBytes = (size_t) numBytes;
                    if (SIZE_MAX - *numValueBytes < numFragmentBytes) {
                        HAPLog(&logObject, "Fragmented OPACK Data is too long.");
                        return kHAPError_InvalidData;
                    }
                    if (!*numValueBytes) {
                        *valueBytes = fragmentBytes;
                    } else {
                        HAPRawBufferCopyBytes(
                                &((uint8_t*) *valueBytes)[*numValueBytes], fragmentBytes, numFragmentBytes);
                    }
                    *numValueBytes += numFragmentBytes;
                }
                o += (size_t) numBytes;
                break;
            }
            case kHAPOPACKTag_Data: {
                numExpectedTerminators++;
                if (!numExpectedTerminators) {
                    HAPLog(&logObject, "OPACK Data contains too much recursion (%zu).", numExpectedTerminators - 1);
                    return kHAPError_InvalidData;
                }
                break;
            }
            case kHAPOPACKTag_Terminator: {
                if (numExpectedTerminators) {
                    numExpectedTerminators--;
                    break;
                }
                HAP_FALLTHROUGH;
            }
            default: {
                HAPLogError(&logObject, "OPACK item is not a data value.");
                return kHAPError_InvalidData;
            }
        }
    } while (numExpectedTerminators);

    UPDATE_READER(o);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static HAPError HAPOPACKReaderGetNextArray_(
        HAPOPACKReader* reader,
        HAPOPACKReader* _Nullable const subReader,
        size_t recursionDepth) {
    HAPPrecondition(reader);

    HAPError err;

    if (!recursionDepth) {
        HAPLog(&logObject, "OPACK item contains too many levels of recursion.");
        return kHAPError_InvalidData;
    }

    size_t o = 0;
    CHECK_SIZE_OR_RETURN(sizeof(HAPOPACKTag), "tag");
    HAPAssert(reader->bytes);
    uint8_t* b = reader->bytes;

    size_t numItems;

    switch (b[o++]) {
        case kHAPOPACKTag_Array0:
        case kHAPOPACKTag_Array1:
        case kHAPOPACKTag_Array2:
        case kHAPOPACKTag_Array3:
        case kHAPOPACKTag_Array4:
        case kHAPOPACKTag_Array5:
        case kHAPOPACKTag_Array6:
        case kHAPOPACKTag_Array7:
        case kHAPOPACKTag_Array8:
        case kHAPOPACKTag_Array9:
        case kHAPOPACKTag_Array10:
        case kHAPOPACKTag_Array11:
        case kHAPOPACKTag_Array12:
        case kHAPOPACKTag_Array13:
        case kHAPOPACKTag_Array14: {
            numItems = b[0] - kHAPOPACKTag_Array0;
        }
            goto readValue;
        case kHAPOPACKTag_Array: {
            numItems = SIZE_MAX;
        }
            goto readValue;
        readValue : {
            void* valueBytes = &b[o];
            size_t numValueBytes = 0;

            for (size_t i = 0; i < numItems || numItems == SIZE_MAX; i++) {
                CHECK_SIZE_OR_RETURN(sizeof(HAPOPACKTag), "sub-tag");
                if (b[o] == kHAPOPACKTag_Terminator) {
                    o++;
                    break;
                }

                UPDATE_READER(o);
                {
                    err = HAPOPACKReaderGetNext_(reader, NULL, recursionDepth);
                    if (err) {
                        HAPAssert(err == kHAPError_InvalidData);
                        return err;
                    }
                }
                b = reader->bytes;
                o = 0;

                numValueBytes = (size_t)(&b[o] - (uint8_t*) valueBytes);
            }

            if (subReader) {
                HAPOPACKReaderCreate(subReader, valueBytes, numValueBytes);
            }
            break;
        }
        default: {
            HAPLogError(&logObject, "OPACK item is not an array value.");
            return kHAPError_InvalidData;
        }
    }

    UPDATE_READER(o);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPOPACKReaderGetNextArray(HAPOPACKReader* reader, HAPOPACKReader* _Nullable subReader) {
    return HAPOPACKReaderGetNextArray_(reader, subReader, kHAPOPACKReader_MaxRecursionDepth);
}

HAP_RESULT_USE_CHECK
static HAPError HAPOPACKReaderGetNextDictionary_(
        HAPOPACKReader* reader,
        HAPOPACKReader* _Nullable const subReader,
        size_t recursionDepth) {
    HAPPrecondition(reader);

    HAPError err;

    if (!recursionDepth) {
        HAPLog(&logObject, "OPACK item contains too many levels of recursion.");
        return kHAPError_InvalidData;
    }

    size_t o = 0;
    CHECK_SIZE_OR_RETURN(sizeof(HAPOPACKTag), "tag");
    HAPAssert(reader->bytes);
    uint8_t* b = reader->bytes;

    size_t numItems;

    switch (b[o++]) {
        case kHAPOPACKTag_Dictionary0:
        case kHAPOPACKTag_Dictionary1:
        case kHAPOPACKTag_Dictionary2:
        case kHAPOPACKTag_Dictionary3:
        case kHAPOPACKTag_Dictionary4:
        case kHAPOPACKTag_Dictionary5:
        case kHAPOPACKTag_Dictionary6:
        case kHAPOPACKTag_Dictionary7:
        case kHAPOPACKTag_Dictionary8:
        case kHAPOPACKTag_Dictionary9:
        case kHAPOPACKTag_Dictionary10:
        case kHAPOPACKTag_Dictionary11:
        case kHAPOPACKTag_Dictionary12:
        case kHAPOPACKTag_Dictionary13:
        case kHAPOPACKTag_Dictionary14: {
            numItems = b[0] - kHAPOPACKTag_Dictionary0;
        }
            goto readValue;
        case kHAPOPACKTag_Dictionary: {
            numItems = SIZE_MAX;
        }
            goto readValue;
        readValue : {
            void* valueBytes = &b[o];
            size_t numValueBytes = 0;

            for (size_t i = 0; i < numItems || numItems == SIZE_MAX; i++) {
                CHECK_SIZE_OR_RETURN(sizeof(HAPOPACKTag), "sub-tag");
                if (b[o] == kHAPOPACKTag_Terminator) {
                    o++;
                    break;
                }

                UPDATE_READER(o);
                {
                    // Key.
                    err = HAPOPACKReaderGetNext_(reader, NULL, recursionDepth);
                    if (err) {
                        HAPAssert(err == kHAPError_InvalidData);
                        return err;
                    }

                    // Value.
                    err = HAPOPACKReaderGetNext_(reader, NULL, recursionDepth);
                    if (err) {
                        HAPAssert(err == kHAPError_InvalidData);
                        return err;
                    }
                }
                b = reader->bytes;
                o = 0;

                numValueBytes = (size_t)(&b[o] - (uint8_t*) valueBytes);
            }

            if (subReader) {
                HAPOPACKReaderCreate(subReader, valueBytes, numValueBytes);
            }
            break;
        }
        default: {
            HAPLogError(&logObject, "OPACK item is not a dictionary value.");
        }
            return kHAPError_InvalidData;
    }

    UPDATE_READER(o);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPOPACKReaderGetNextDictionary(HAPOPACKReader* reader, HAPOPACKReader* _Nullable subReader) {
    return HAPOPACKReaderGetNextDictionary_(reader, subReader, kHAPOPACKReader_MaxRecursionDepth);
}

#undef CHECK_SIZE_OR_RETURN
#undef UPDATE_READER

HAP_RESULT_USE_CHECK
HAPError HAPOPACKStringDictionaryReaderGetAll(
        HAPOPACKReader* reader,
        HAPOPACKStringDictionaryElement* _Nullable const* _Nonnull elements) {
    HAPPrecondition(reader);
    HAPPrecondition(elements);

    HAPError err;

    for (size_t i = 0; elements[i]; i++) {
        HAPOPACKStringDictionaryElement* element = elements[i];

        // Check for duplicate key.
        if (element->key) {
            for (size_t j = 0; j < i; j++) {
                const HAPOPACKStringDictionaryElement* otherElement = elements[j];

                if (otherElement->key) {
                    HAPPrecondition(!HAPStringAreEqual(HAPNonnull(element->key), HAPNonnull(otherElement->key)));
                }
            }
        }

        // Initialize value.
        element->value.exists = false;
        HAPOPACKReaderCreate(&element->value.reader, NULL, 0);
    }

    // Get dictionary contents.
    HAPOPACKReader subReader;
    err = HAPOPACKReaderGetNextDictionary(reader, &subReader);
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }

    for (;;) {
        // Check if there are more elements.
        bool found;
        err = HAPOPACKReaderPeekNextType(&subReader, &found, NULL);
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            return err;
        }
        if (!found) {
            break;
        }

        // Fetch key.
        char* key;
        err = HAPOPACKReaderGetNextString(&subReader, &key);
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            HAPLog(&logObject, "Dictionary key malformed.");
            return err;
        }

        // Match element.
        HAPOPACKStringDictionaryElement* _Nullable freeElement = NULL;
        size_t i;
        for (i = 0; elements[i]; i++) {
            HAPOPACKStringDictionaryElement* element = elements[i];

            if (element->key) {
                if (HAPStringAreEqual(HAPNonnull(element->key), key)) {
                    if (element->value.exists) {
                        // Duplicate dictionary element with same key found.
                        HAPLog(&logObject, "Duplicate dictionary key found: %s.", key);
                        return kHAPError_InvalidData;
                    }

                    // Dictionary element found. Save.
                    element->value.exists = true;
                    err = HAPOPACKReaderGetNext(&subReader, &element->value.reader);
                    if (err) {
                        HAPAssert(err == kHAPError_InvalidData);
                        HAPLog(&logObject, "Dictionary value malformed.");
                        return err;
                    }

                    break;
                }
            } else {
                if (!freeElement) {
                    freeElement = element;
                }
            }
        }
        if (!elements[i]) {
            if (freeElement) {
                freeElement->key = key;
                freeElement->value.exists = true;
                err = HAPOPACKReaderGetNext(&subReader, &freeElement->value.reader);
                if (err) {
                    HAPAssert(err == kHAPError_InvalidData);
                    HAPLog(&logObject, "Dictionary value malformed.");
                    return err;
                }
            } else {
                HAPLog(&logObject, "Dictionary element with key %s ignored.", key);
                // Skip value associated with this key
                err = HAPOPACKReaderGetNext(&subReader, NULL);
                if (err) {
                    HAPAssert(err == kHAPError_InvalidData);
                    HAPLog(&logObject, "Dictionary value malformed.");
                    return err;
                }
            }
        }
    }

    // Check that all items have been read.
    bool found;
    err = HAPOPACKReaderPeekNextType(reader, &found, NULL);
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }
    if (found) {
        HAPLog(&logObject, "Additional OPACK items present after dictionary.");
        return kHAPError_InvalidData;
    }

    return kHAPError_None;
}
