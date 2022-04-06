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

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "OPACKWriter" };

// See HomeKit Accessory Protocol Specification R17
// Section 9.1.4 Data Format

void HAPOPACKWriterCreate(HAPOPACKWriter* writer, void* bytes, size_t maxBytes) {
    HAPPrecondition(writer);

    // Initialize writer.
    HAPRawBufferZero(writer, sizeof *writer);
    writer->bytes = bytes;
    writer->maxBytes = maxBytes;
    writer->numBytes = 0;
}

void HAPOPACKWriterGetBuffer(HAPOPACKWriter* writer, void* _Nonnull* _Nonnull bytes, size_t* numBytes) {
    HAPPrecondition(writer);
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes);

    *bytes = writer->bytes;
    *numBytes = writer->numBytes;
}

#define BEGIN_APPEND \
    uint8_t* b = &((uint8_t*) writer->bytes)[writer->numBytes]; \
    HAPAssert(writer->numBytes <= writer->maxBytes); \
    size_t maxBytes = writer->maxBytes - writer->numBytes; \
    size_t o = 0;

#define CHECK_SIZE_OR_RETURN(n, description) \
    do { \
        if (maxBytes - o < (n)) { \
            HAPLog(&logObject, "Not enough memory to write %s.", (description)); \
            return kHAPError_OutOfResources; \
        } \
    } while (0)

#define END_APPEND \
    HAPAssert(o <= maxBytes); \
    writer->numBytes += o;

HAP_RESULT_USE_CHECK
HAPError HAPOPACKWriterAppendBool(HAPOPACKWriter* writer, bool value) {
    HAPPrecondition(writer);

    BEGIN_APPEND {
        CHECK_SIZE_OR_RETURN(sizeof(HAPOPACKTag), "tag");
        b[o++] = value ? kHAPOPACKTag_True : kHAPOPACKTag_False;
    }
    END_APPEND
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPOPACKWriterAppendNull(HAPOPACKWriter* writer) {
    HAPPrecondition(writer);

    BEGIN_APPEND {
        CHECK_SIZE_OR_RETURN(sizeof(HAPOPACKTag), "tag");
        b[o++] = kHAPOPACKTag_Null;
    }
    END_APPEND
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPOPACKWriterAppendUUID(HAPOPACKWriter* writer, const HAPUUID* value) {
    HAPPrecondition(writer);
    HAPPrecondition(value);

    BEGIN_APPEND {
        CHECK_SIZE_OR_RETURN(sizeof(HAPOPACKTag), "tag");
        b[o++] = kHAPOPACKTag_UUID;

        CHECK_SIZE_OR_RETURN(sizeof value->bytes, "value");
        for (size_t i = 0; i < sizeof value->bytes; i++) {
            b[o++] = value->bytes[sizeof value->bytes - 1 - i];
        }
    }
    END_APPEND
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPOPACKWriterAppendDate(HAPOPACKWriter* writer, HAPOPACKDate value) {
    HAPPrecondition(writer);

    BEGIN_APPEND {
        CHECK_SIZE_OR_RETURN(sizeof(HAPOPACKTag), "tag");
        b[o++] = kHAPOPACKTag_Date;

        CHECK_SIZE_OR_RETURN(sizeof value, "value");
        HAPAssert(sizeof value == sizeof(double));
        uint64_t bitPattern = HAPDoubleGetBitPattern(value);
        HAPWriteLittleUInt64(&b[o], bitPattern);
        o += sizeof value;
    }
    END_APPEND
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPOPACKWriterAppendInt(HAPOPACKWriter* writer, int64_t value) {
    HAPPrecondition(writer);

    BEGIN_APPEND {
        CHECK_SIZE_OR_RETURN(sizeof(HAPOPACKTag), "tag");
        if (value == -1) {
            b[o++] = kHAPOPACKTag_Negative1;
        } else if (value >= 0 && value <= 38) {
            b[o++] = (HAPOPACKTag)(kHAPOPACKTag_0 + value);
        } else if (value == 39) {
            b[o++] = kHAPOPACKTag_39;
        } else if (value >= INT8_MIN && value <= INT8_MAX) {
            b[o++] = kHAPOPACKTag_Int8;

            CHECK_SIZE_OR_RETURN(sizeof(int8_t), "value");
            b[o++] = (uint8_t) value;
        } else if (value >= INT16_MIN && value <= INT16_MAX) {
            b[o++] = kHAPOPACKTag_Int16;

            CHECK_SIZE_OR_RETURN(sizeof(int16_t), "value");
            HAPWriteLittleInt16(&b[o], value);
            o += sizeof(int16_t);
        } else if (value >= INT32_MIN && value <= INT32_MAX) {
            b[o++] = kHAPOPACKTag_Int32;

            CHECK_SIZE_OR_RETURN(sizeof(int32_t), "value");
            HAPWriteLittleInt32(&b[o], value);
            o += sizeof(int32_t);
        } else {
            b[o++] = kHAPOPACKTag_Int64;

            CHECK_SIZE_OR_RETURN(sizeof(int64_t), "value");
            HAPWriteLittleInt64(&b[o], value);
            o += sizeof(int64_t);
        }
    }
    END_APPEND
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPOPACKWriterAppendFloat(HAPOPACKWriter* writer, double value) {
    HAPPrecondition(writer);

    BEGIN_APPEND {
        CHECK_SIZE_OR_RETURN(sizeof(HAPOPACKTag), "tag");
        b[o++] = kHAPOPACKTag_Double;

        CHECK_SIZE_OR_RETURN(sizeof(double), "value");
        uint64_t bitPattern = HAPDoubleGetBitPattern(value);
        HAPWriteLittleUInt64(&b[o], bitPattern);
        o += sizeof(double);
    }
    END_APPEND
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPOPACKWriterAppendNumber(HAPOPACKWriter* writer, const HAPOPACKNumber* value) {
    HAPPrecondition(writer);
    HAPPrecondition(value);

    HAPError err;

    if (HAPOPACKNumberIsFloatType(value)) {
        double val;
        bool exact = HAPOPACKNumberGetValue(value, kHAPOPACKNumberType_Double, &val);
        HAPAssert(exact);
        err = HAPOPACKWriterAppendFloat(writer, val);
    } else {
        int64_t val;
        bool exact = HAPOPACKNumberGetValue(value, kHAPOPACKNumberType_Int64, &val);
        HAPAssert(exact);
        err = HAPOPACKWriterAppendInt(writer, val);
    }
    return err;
}

HAP_RESULT_USE_CHECK
HAPError HAPOPACKWriterAppendString(HAPOPACKWriter* writer, const char* value) {
    HAPPrecondition(writer);
    HAPPrecondition(value);

    size_t numValueBytes = HAPStringGetNumBytes(value);

    HAPPrecondition(HAPUTF8IsValidData(value, numValueBytes));

    BEGIN_APPEND {
        CHECK_SIZE_OR_RETURN(sizeof(HAPOPACKTag), "tag");
        if (numValueBytes <= kHAPOPACKTag_String32 - kHAPOPACKTag_String0) {
            b[o++] = (HAPOPACKTag)(kHAPOPACKTag_String0 + numValueBytes);
        } else {
            b[o++] = kHAPOPACKTag_String;
        }

        CHECK_SIZE_OR_RETURN(numValueBytes, "value");
        HAPRawBufferCopyBytes(&b[o], value, numValueBytes);
        o += numValueBytes;

        if (numValueBytes > 32) {
            CHECK_SIZE_OR_RETURN(sizeof(char), "terminator");
            b[o++] = '\0';
        }
    }
    END_APPEND
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPOPACKWriterAppendData(HAPOPACKWriter* writer, const void* valueBytes, size_t numValueBytes) {
    HAPPrecondition(writer);
    HAPPrecondition(valueBytes);

    BEGIN_APPEND {
        CHECK_SIZE_OR_RETURN(sizeof(HAPOPACKTag), "tag");
        if (numValueBytes <= kHAPOPACKTag_Data32 - kHAPOPACKTag_Data0) {
            b[o++] = (HAPOPACKTag)(kHAPOPACKTag_Data0 + numValueBytes);
        } else if (numValueBytes <= UINT8_MAX) {
            b[o++] = kHAPOPACKTag_DataUInt8;

            CHECK_SIZE_OR_RETURN(sizeof(uint8_t), "length");
            b[o++] = (uint8_t) numValueBytes;
        } else if (numValueBytes <= UINT16_MAX) {
            b[o++] = kHAPOPACKTag_DataUInt16;

            CHECK_SIZE_OR_RETURN(sizeof(uint16_t), "length");
            HAPWriteLittleUInt16(&b[o], numValueBytes);
            o += sizeof(uint16_t);
        } else if (numValueBytes <= UINT32_MAX) {
            b[o++] = kHAPOPACKTag_DataUInt32;

            CHECK_SIZE_OR_RETURN(sizeof(uint32_t), "length");
            HAPWriteLittleUInt32(&b[o], numValueBytes);
            o += sizeof(uint32_t);
        } else {
            b[o++] = kHAPOPACKTag_DataUInt64;

            CHECK_SIZE_OR_RETURN(sizeof(uint64_t), "length");
            HAPWriteLittleUInt64(&b[o], (uint64_t) numValueBytes);
            o += sizeof(uint64_t);
        }

        CHECK_SIZE_OR_RETURN(numValueBytes, "value");
        HAPRawBufferCopyBytes(&b[o], valueBytes, numValueBytes);
        o += numValueBytes;
    }
    END_APPEND
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPOPACKWriterAppendArrayBegin(HAPOPACKWriter* writer) {
    HAPPrecondition(writer);

    BEGIN_APPEND {
        CHECK_SIZE_OR_RETURN(sizeof(HAPOPACKTag), "tag");
        b[o++] = kHAPOPACKTag_Array;
    }
    END_APPEND
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPOPACKWriterAppendDictionaryBegin(HAPOPACKWriter* writer) {
    HAPPrecondition(writer);

    BEGIN_APPEND {
        CHECK_SIZE_OR_RETURN(sizeof(HAPOPACKTag), "tag");
        b[o++] = kHAPOPACKTag_Dictionary;
    }
    END_APPEND
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPOPACKWriterAppendTerminator(HAPOPACKWriter* writer) {
    HAPPrecondition(writer);

    BEGIN_APPEND {
        CHECK_SIZE_OR_RETURN(sizeof(HAPOPACKTag), "tag");
        b[o++] = kHAPOPACKTag_Terminator;
    }
    END_APPEND
    return kHAPError_None;
}

#undef BEGIN_APPEND
#undef CHECK_SIZE_OR_RETURN
#undef END_APPEND
