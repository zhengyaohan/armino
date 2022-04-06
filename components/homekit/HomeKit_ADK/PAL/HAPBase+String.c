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

#include "HAPPlatform.h"

HAP_PRINTFLIKE(3, 4)
HAP_RESULT_USE_CHECK
HAPError HAPStringWithFormat(char* bytes, size_t maxBytes, const char* format, ...) {
    va_list args;
    va_start(args, format);
    HAPError err = HAPStringWithFormatAndArguments(bytes, maxBytes, format, args);
    va_end(args);
    return err;
}

HAP_PRINTFLIKE(3, 0)
HAP_RESULT_USE_CHECK
HAPError HAPStringWithFormatAndArguments(char* bytes, size_t maxBytes, const char* format, va_list arguments) {
    HAPPrecondition(bytes);
    HAPPrecondition(format);

    char c = format[0];
    size_t i = 1;
    size_t n = 0;
    while (c) {
        if (c == '%') {
            // insert value
            c = format[i++];     // format defaults:
            char prefix = ' ';   // fill with space
            char sign = 0;       // do not us a positive sign
            uint32_t length = 0; // no long type
            uint32_t width = 0;  // minimum width
            // flags field
            while (c == '0' || c == '+' || c == ' ') {
                if (c == '0') {
                    prefix = '0';
                } else {
                    sign = c;
                }
                c = format[i++];
            }
            // width field
            while (HAPASCIICharacterIsNumber(c)) {
                width = width * 10 + (uint32_t)(c - '0');
                c = format[i++];
            }
            // length field
            if (c == 'l') {
                length = 1;
                c = format[i++];
                if (c == 'l') {
                    length = 2;
                    c = format[i++];
                }
            } else if (c == 'z') {
                length = 3;
                c = format[i++];
            } else {
                // No length field.
            }
            // type field
            char buffer[32]; // temporary string buffer
            char* string = NULL;
            size_t strLen = 0;
            int64_t sValue = 0;
            uint64_t uValue = 0;
            switch (c) {
                case '%': {
                    string = "%";
                    break;
                }
                case 'd':
                case 'i': {
                    if (length == 0) { // NOLINT(bugprone-branch-clone)
                        sValue = (int64_t) va_arg(arguments, int);
                    } else if (length == 1) { // 'l'
                        sValue = (int64_t) va_arg(arguments, long);
                    } else if (length == 2) { // 'll'
                        sValue = (int64_t) va_arg(arguments, long long);
                    } else { // 'z'
                        sValue = (int64_t) va_arg(arguments, size_t);
                    }
                    break;
                }
                case 'x':
                case 'X':
                case 'u': {
                    if (length == 0) { // NOLINT(bugprone-branch-clone)
                        uValue = (uint64_t) va_arg(arguments, unsigned int);
                    } else if (length == 1) { // 'l'
                        uValue = (uint64_t) va_arg(arguments, unsigned long);
                    } else if (length == 2) { // 'll'
                        uValue = (uint64_t) va_arg(arguments, long long);
                    } else { // 'z'
                        uValue = (uint64_t) va_arg(arguments, size_t);
                    }
                    break;
                }
                case 'p': {
                    uValue = (uint64_t)(uintptr_t) va_arg(arguments, void*);
                    break;
                }
                case 's': {
                    string = va_arg(arguments, char*);
                    if (string == NULL) {
                        string = "(null)";
                    }
                    break;
                }
                case 'c': {
                    buffer[0] = (char) va_arg(arguments, int);
                    buffer[1] = 0;
                    string = buffer;
                    strLen = 1;
                    break;
                }
                case 'g': {
                    double d = va_arg(arguments, double);
                    HAPError res = HAPFloatGetDescription(buffer, sizeof buffer, (float) d);
                    if (res != kHAPError_None) {
                        return res;
                    }
                    string = buffer;
                    break;
                }
                default: {
                    HAPLogError(&kHAPLog_Default, "Unsupported format string type specifier: %%%c", c);
                    HAPPreconditionFailure();
                }
            }
            if (!string) { // convert integer to string
                HAPError res;
                if (c == 'i' || c == 'd') { // signed
                    if (sValue < 0) {
                        sValue = -sValue;
                        sign = '-';
                    }
                    uValue = (uint64_t) sValue;
                }
                if (c == 'x') {
                    res = HAPUInt64GetHexDescription(uValue, buffer, sizeof buffer, kHAPLetterCase_Lowercase);
                } else if (c == 'X') {
                    res = HAPUInt64GetHexDescription(uValue, buffer, sizeof buffer, kHAPLetterCase_Uppercase);
                } else if (c == 'p') {
                    buffer[0] = '0';
                    buffer[1] = 'x';
                    res = HAPUInt64GetHexDescription(uValue, buffer + 2, sizeof buffer - 2, kHAPLetterCase_Lowercase);
                } else {
                    res = HAPUInt64GetDescription(uValue, buffer, sizeof buffer);
                }
                if (res != kHAPError_None) {
                    return res;
                }
                string = buffer;
            }
            if (strLen == 0) {
                strLen = HAPStringGetNumBytes(string);
            }
            size_t totLen = strLen;
            if (sign) {
                totLen++;
            }
            if (n + totLen >= maxBytes || n + width >= maxBytes) {
                return kHAPError_OutOfResources;
            }
            if (sign && prefix == '0') {
                bytes[n++] = sign; // write sign before zeros
            }
            while (totLen < width) {
                bytes[n++] = prefix;
                totLen++;
            }
            if (sign && prefix == ' ') {
                bytes[n++] = sign; // write sign after spaces
            }
            HAPRawBufferCopyBytes(&bytes[n], string, strLen);
            n += strLen;
        } else {
            // copy char to output
            if (n + 1 >= maxBytes) {
                return kHAPError_OutOfResources;
            }
            bytes[n++] = c;
        }
        c = format[i++];
    }
    bytes[n] = 0;
    return kHAPError_None;
}

HAP_NO_SIDE_EFFECTS
HAP_RESULT_USE_CHECK
size_t HAPStringGetNumBytes(const char* string) {
    HAPPrecondition(string);

    const char* stringEnd;
    for (stringEnd = string; *stringEnd; stringEnd++) {
    }

    HAPAssert(stringEnd >= string);
    return (size_t)(stringEnd - string);
}

HAP_NO_SIDE_EFFECTS
HAP_RESULT_USE_CHECK
bool HAPStringAreEqual(const char* string, const char* otherString) {
    HAPPrecondition(string);
    HAPPrecondition(otherString);

    if (string == otherString) {
        return true;
    }

    const char* c1;
    const char* c2;
    for (c1 = string, c2 = otherString; *c1 == *c2 && *c1; c1++, c2++) {
    }

    return !*c1 && !*c2;
}

HAP_NO_SIDE_EFFECTS
HAP_RESULT_USE_CHECK
bool HAPStringHasPrefix(const char* string, const char* prefix) {
    HAPPrecondition(string);
    HAPPrecondition(prefix);
    size_t stringNumBytes = HAPStringGetNumBytes(string);
    size_t prefixNumBytes = HAPStringGetNumBytes(prefix);

    if (prefixNumBytes > stringNumBytes) {
        return false;
    }

    bool prefixMatches = true;
    for (size_t i = 0; i < prefixNumBytes; i++) {
        prefixMatches &= string[i] == prefix[i];
    }

    return prefixMatches;
}
