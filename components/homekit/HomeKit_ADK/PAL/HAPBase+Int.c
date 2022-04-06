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

#include "HAPPlatform.h"

enum base_t {
    BASE_10,
    BASE_16,
};

#define RETURN_INT_FROM_STRING(type, base, description, value, maxValue, minValue) \
    do { \
        HAPPrecondition(description); \
        HAPPrecondition(value); \
\
        const char* c = description; \
\
        /* Read optional sign. */ \
        bool isNegative = false; \
        if (*c == '+' || *c == '-') { \
            isNegative = *c == '-'; \
            c++; \
        } \
\
        /* Read value. */ \
        if (!*c) { \
            return kHAPError_InvalidData; \
        } \
        *(value) = 0; \
        int scalar = 0; \
        switch (base) { \
            case BASE_10: \
                scalar = 10; \
                break; \
            case BASE_16: \
                scalar = 16; \
                break; \
            default: \
                return kHAPError_InvalidData; \
        } \
\
        for (; *c; c++) { \
            if ((base == BASE_10 && (!HAPASCIICharacterIsNumber(*c))) || \
                (base == BASE_16 && (!HAPASCIICharacterIsUppercaseHexDigit(*c)))) { \
                return kHAPError_InvalidData; \
            } \
            type digit = (type)(*c - '0'); \
            if (base == BASE_16 && digit > 9) { \
                digit -= 'A' - '9' - 1; \
            } \
\
            if (!isNegative) { \
                if (*(value) > (maxValue) / scalar) { \
                    return kHAPError_InvalidData; \
                } \
                *(value) *= scalar; \
\
                if (*(value) > (maxValue) -digit) { \
                    return kHAPError_InvalidData; \
                } \
                *(value) += digit; \
            } else { \
                if (*(value) < (minValue) / scalar) { \
                    return kHAPError_InvalidData; \
                } \
                *(value) *= scalar; \
\
                if (*(value) < (minValue) + digit) { \
                    return kHAPError_InvalidData; \
                } \
                *(value) -= digit; \
            } \
        } \
\
        return kHAPError_None; \
    } while (0)

HAP_RESULT_USE_CHECK
HAPError HAPUInt64FromString(const char* description, uint64_t* value) {
    HAP_DIAGNOSTIC_PUSH
    HAP_DIAGNOSTIC_IGNORED_ARMCC(186)
    HAP_DIAGNOSTIC_IGNORED_GCC("-Wtype-limits")
    HAP_DIAGNOSTIC_IGNORED_CLANG("-Wsign-compare")
    HAP_DIAGNOSTIC_IGNORED_ICCARM(Pe186)
    RETURN_INT_FROM_STRING(uint64_t, BASE_10, description, value, UINT64_MAX, 0);
    HAP_DIAGNOSTIC_RESTORE_ICCARM(Pe186)
    HAP_DIAGNOSTIC_POP
}

HAP_RESULT_USE_CHECK
HAPError HAPUInt32FromString(const char* description, uint32_t* value) {
    HAP_DIAGNOSTIC_PUSH
    HAP_DIAGNOSTIC_IGNORED_ARMCC(186)
    HAP_DIAGNOSTIC_IGNORED_GCC("-Wtype-limits")
    HAP_DIAGNOSTIC_IGNORED_CLANG("-Wsign-compare")
    HAP_DIAGNOSTIC_IGNORED_ICCARM(Pe186)
    RETURN_INT_FROM_STRING(uint32_t, BASE_10, description, value, UINT32_MAX, 0);
    HAP_DIAGNOSTIC_RESTORE_ICCARM(Pe186)
    HAP_DIAGNOSTIC_POP
}

HAP_RESULT_USE_CHECK
HAPError HAPUInt16FromString(const char* description, uint16_t* value) {
    HAP_DIAGNOSTIC_PUSH
    HAP_DIAGNOSTIC_IGNORED_ARMCC(186)
    HAP_DIAGNOSTIC_IGNORED_GCC("-Wtype-limits")
    HAP_DIAGNOSTIC_IGNORED_CLANG("-Wsign-compare")
    HAP_DIAGNOSTIC_IGNORED_ICCARM(Pe186)
    RETURN_INT_FROM_STRING(uint16_t, BASE_10, description, value, UINT16_MAX, 0);
    HAP_DIAGNOSTIC_RESTORE_ICCARM(Pe186)
    HAP_DIAGNOSTIC_POP
}

HAP_RESULT_USE_CHECK
HAPError HAPUInt8FromString(const char* description, uint8_t* value) {
    HAP_DIAGNOSTIC_PUSH
    HAP_DIAGNOSTIC_IGNORED_ARMCC(186)
    HAP_DIAGNOSTIC_IGNORED_GCC("-Wtype-limits")
    HAP_DIAGNOSTIC_IGNORED_CLANG("-Wsign-compare")
    HAP_DIAGNOSTIC_IGNORED_ICCARM(Pe186)
    RETURN_INT_FROM_STRING(uint8_t, BASE_10, description, value, UINT8_MAX, 0);
    HAP_DIAGNOSTIC_RESTORE_ICCARM(Pe186)
    HAP_DIAGNOSTIC_POP
}

// Hex
HAP_RESULT_USE_CHECK
HAPError HAPUInt64FromHexString(const char* description, uint64_t* value) {
    HAP_DIAGNOSTIC_PUSH
    HAP_DIAGNOSTIC_IGNORED_ARMCC(186)
    HAP_DIAGNOSTIC_IGNORED_GCC("-Wtype-limits")
    HAP_DIAGNOSTIC_IGNORED_CLANG("-Wsign-compare")
    HAP_DIAGNOSTIC_IGNORED_ICCARM(Pe186)
    RETURN_INT_FROM_STRING(uint64_t, BASE_16, description, value, UINT64_MAX, 0);
    HAP_DIAGNOSTIC_RESTORE_ICCARM(Pe186)
    HAP_DIAGNOSTIC_POP
}

HAP_RESULT_USE_CHECK
HAPError HAPUInt32FromHexString(const char* description, uint32_t* value) {
    HAP_DIAGNOSTIC_PUSH
    HAP_DIAGNOSTIC_IGNORED_ARMCC(186)
    HAP_DIAGNOSTIC_IGNORED_GCC("-Wtype-limits")
    HAP_DIAGNOSTIC_IGNORED_CLANG("-Wsign-compare")
    HAP_DIAGNOSTIC_IGNORED_ICCARM(Pe186)
    RETURN_INT_FROM_STRING(uint32_t, BASE_16, description, value, UINT32_MAX, 0);
    HAP_DIAGNOSTIC_RESTORE_ICCARM(Pe186)
    HAP_DIAGNOSTIC_POP
}

HAP_RESULT_USE_CHECK
HAPError HAPUInt16FromHexString(const char* description, uint16_t* value) {
    HAP_DIAGNOSTIC_PUSH
    HAP_DIAGNOSTIC_IGNORED_ARMCC(186)
    HAP_DIAGNOSTIC_IGNORED_GCC("-Wtype-limits")
    HAP_DIAGNOSTIC_IGNORED_CLANG("-Wsign-compare")
    HAP_DIAGNOSTIC_IGNORED_ICCARM(Pe186)
    RETURN_INT_FROM_STRING(uint16_t, BASE_16, description, value, UINT16_MAX, 0);
    HAP_DIAGNOSTIC_RESTORE_ICCARM(Pe186)
    HAP_DIAGNOSTIC_POP
}

HAP_RESULT_USE_CHECK
HAPError HAPUInt8FromHexString(const char* description, uint8_t* value) {
    HAP_DIAGNOSTIC_PUSH
    HAP_DIAGNOSTIC_IGNORED_ARMCC(186)
    HAP_DIAGNOSTIC_IGNORED_GCC("-Wtype-limits")
    HAP_DIAGNOSTIC_IGNORED_CLANG("-Wsign-compare")
    HAP_DIAGNOSTIC_IGNORED_ICCARM(Pe186)
    RETURN_INT_FROM_STRING(uint8_t, BASE_16, description, value, UINT8_MAX, 0);
    HAP_DIAGNOSTIC_RESTORE_ICCARM(Pe186)
    HAP_DIAGNOSTIC_POP
}

HAP_RESULT_USE_CHECK
HAPError HAPInt64FromString(const char* description, int64_t* value) {
    RETURN_INT_FROM_STRING(int64_t, BASE_10, description, value, INT64_MAX, INT64_MIN);
}

#define RETURN_INT_NUM_DESCRIPTION_BYTES(value) \
    do { \
        size_t numBytes = 0; \
        if ((value) < 0) { \
            numBytes++; \
        } \
        do { \
            numBytes++; \
            (value) /= 10; \
        } while (value); \
        return numBytes; \
    } while (0)

HAP_RESULT_USE_CHECK
size_t HAPInt32GetNumDescriptionBytes(int32_t value) {
    RETURN_INT_NUM_DESCRIPTION_BYTES(value);
}

HAP_RESULT_USE_CHECK
size_t HAPUInt64GetNumDescriptionBytes(uint64_t value) {
    HAP_DIAGNOSTIC_PUSH
    HAP_DIAGNOSTIC_IGNORED_ARMCC(186)
    HAP_DIAGNOSTIC_IGNORED_GCC("-Wtype-limits")
    HAP_DIAGNOSTIC_IGNORED_ICCARM(Pe186)
    RETURN_INT_NUM_DESCRIPTION_BYTES(value);
    HAP_DIAGNOSTIC_RESTORE_ICCARM(Pe186)
    HAP_DIAGNOSTIC_POP
}

#define RETURN_INT_DESCRIPTION(value, bytes, maxBytes) \
    do { \
        char* b = (bytes) + (maxBytes); \
        size_t numBytes = 0; \
        if (numBytes++ >= (maxBytes)) { \
            return kHAPError_OutOfResources; \
        } \
        *--b = '\0'; \
        do { \
            if (numBytes++ >= (maxBytes)) { \
                return kHAPError_OutOfResources; \
            } \
            *--b = '0' + (value) % 10; \
            (value) /= 10; \
        } while (value); \
        HAPRawBufferCopyBytes(bytes, b, numBytes); \
        return kHAPError_None; \
    } while (0)

HAP_RESULT_USE_CHECK
HAPError HAPUInt64GetDescription(uint64_t value, char* bytes, size_t maxBytes) {
    HAP_DIAGNOSTIC_PUSH
    HAP_DIAGNOSTIC_IGNORED_ARMCC(186)
    HAP_DIAGNOSTIC_IGNORED_GCC("-Wtype-limits")
    RETURN_INT_DESCRIPTION(value, bytes, maxBytes);
    HAP_DIAGNOSTIC_POP
}

HAP_RESULT_USE_CHECK
HAPError HAPUInt64GetHexDescription(uint64_t value, char* bytes, size_t maxBytes, HAPLetterCase letterCase) {
    size_t chars = 16;
    int shift = 60;
    while (chars > 1 && ((value >> (unsigned int) shift) & 0xFU) == 0) {
        shift -= 4;
        chars--;
    }
    if (chars >= maxBytes) {
        return kHAPError_OutOfResources;
    }
    int i = 0;
    while (shift >= 0) {
        int digit = (int) ((value >> (unsigned int) shift) & 0xFU);
        bytes[i++] = (char) (digit + (digit < 10 ? '0' : (char) letterCase - 10));
        shift -= 4;
    }
    bytes[i] = 0;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
uint8_t HAPReadUIntMax8(const void* bytes_, size_t numBytes) {
    HAPPrecondition(bytes_);
    const uint8_t* bytes = bytes_;
    HAPPrecondition(numBytes <= sizeof(uint8_t));

    uint8_t value = 0;
    for (size_t i = 0; i < numBytes; i++) {
        value |= (uint8_t)((uint8_t) bytes[i] << (i * 8));
    }
    return value;
}

HAP_RESULT_USE_CHECK
HAPError HAPUInt8FromHexDigit(char description, uint8_t* value) {
    HAPPrecondition(value);

    if (HAPASCIICharacterIsNumber(description)) {
        *value = (uint8_t)(description - '0');
    } else if (HAPASCIICharacterIsUppercaseHexLetter(description)) {
        *value = ((uint8_t)(description - 'A')) + (uint8_t) 10;
    } else if (HAPASCIICharacterIsLowercaseHexLetter(description)) {
        *value = ((uint8_t)(description - 'a')) + (uint8_t) 10;
    } else {
        return kHAPError_InvalidData;
    }
    HAPAssert(*value <= 0xf);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPUInt8BufferFromHexString(const char* hexString, size_t byteBufferSize, uint8_t* byteBuffer) {
    HAPPrecondition(hexString);
    HAPPrecondition(byteBuffer);

    HAPAssert((HAPStringGetNumBytes(hexString) / 2) <= byteBufferSize);
    HAPRawBufferZero(byteBuffer, sizeof byteBuffer);

    HAPError err;

    for (size_t i = 0; i < byteBufferSize; i++) {
        uint8_t digit1;
        uint8_t digit2;
        size_t digit1Index = 2 * i + 0;
        size_t digit2Index = 2 * i + 1;

        // Reached end of hex string
        if (hexString[digit1Index] == '\0') {
            return kHAPError_None;
        }

        err = HAPUInt8FromHexDigit(hexString[digit1Index], &digit1);
        if (err != kHAPError_None) {
            return err;
        }

        // Malformed hex string
        if (hexString[digit2Index] == '\0') {
            return kHAPError_InvalidData;
        }

        err = HAPUInt8FromHexDigit(hexString[digit2Index], &digit2);
        if (err != kHAPError_None) {
            return err;
        }

        byteBuffer[i] = (uint8_t)(digit1 << 4U) | (uint8_t)(digit2 << 0U);
    }

    return kHAPError_None;
}

size_t HAPGetVariableIntEncodingLength(uint64_t value) {
    if (value > 0xFFFFFFFF) {
        return 8;
    }
    if (value > 0xFFFF) {
        return 4;
    }
    if (value > 0xFF) {
        return 2;
    }
    return 1;
}
