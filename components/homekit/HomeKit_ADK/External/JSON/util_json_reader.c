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

//
// See RFC 7159, http://www.rfc-editor.org/rfc/rfc7159.txt
//

#include "util_json_reader.h"

#define SUBSTATE_NONE 0

#define SUBSTATE_READING_STRING_AFTER_ESCAPE 1

#define SUBSTATE_READING_NUMBER_AFTER_MINUS               1
#define SUBSTATE_READING_NUMBER_AFTER_ZERO                2
#define SUBSTATE_READING_NUMBER_INTEGER_PART              3
#define SUBSTATE_READING_NUMBER_FRACTION_PART             4
#define SUBSTATE_READING_NUMBER_FRACTION_PART_AFTER_DIGIT 5
#define SUBSTATE_READING_NUMBER_EXPONENT_PART             6
#define SUBSTATE_READING_NUMBER_EXPONENT_PART_AFTER_SIGN  7
#define SUBSTATE_READING_NUMBER_EXPONENT_PART_AFTER_DIGIT 8

#define SUBSTATE_READING_FALSE_AFTER_F    1
#define SUBSTATE_READING_FALSE_AFTER_FA   2
#define SUBSTATE_READING_FALSE_AFTER_FAL  3
#define SUBSTATE_READING_FALSE_AFTER_FALS 4

#define SUBSTATE_READING_TRUE_AFTER_T   1
#define SUBSTATE_READING_TRUE_AFTER_TR  2
#define SUBSTATE_READING_TRUE_AFTER_TRU 3

#define SUBSTATE_READING_NULL_AFTER_N   1
#define SUBSTATE_READING_NULL_AFTER_NU  2
#define SUBSTATE_READING_NULL_AFTER_NUL 3

void util_json_reader_init(struct util_json_reader* r) {
    HAPPrecondition(r != NULL);
    r->state = util_JSON_READER_STATE_READING_WHITESPACE;
    r->substate = SUBSTATE_NONE;
}

HAP_RESULT_USE_CHECK
static size_t skip_digits(const char* buffer, size_t length) {
    size_t n;
    int x;
    HAPPrecondition(buffer != NULL);
    n = 0;
    HAPAssert(n <= length);
    if (n < length) {
        x = buffer[n];
        while (('0' <= x) && (x <= '9')) {
            n++;
            HAPAssert(n <= length);
            if (n < length) {
                x = buffer[n];
            } else {
                x = -1;
            }
        }
    }
    HAPAssert((n == length) || ((n < length) && ((buffer[n] < '0') || (buffer[n] > '9'))));
    return n;
}

HAP_RESULT_USE_CHECK
static size_t skip_whitespace(const char* buffer, size_t length) {
    size_t n;
    int x;
    HAPPrecondition(buffer != NULL);
    n = 0;
    HAPAssert(n <= length);
    if (n < length) {
        x = buffer[n];
        while ((x == ' ') || (x == '\t') || (x == '\n') || (x == '\r')) {
            n++;
            HAPAssert(n <= length);
            if (n < length) {
                x = buffer[n];
            } else {
                x = -1;
            }
        }
    }
    HAPAssert(
            (n == length) ||
            ((n < length) && (buffer[n] != ' ') && (buffer[n] != '\t') && (buffer[n] != '\n') && (buffer[n] != '\r')));
    return n;
}

HAP_RESULT_USE_CHECK
size_t util_json_reader_read(struct util_json_reader* r, const char* buffer, size_t length) {
    size_t n;
    int post;
    HAPPrecondition(r != NULL);
    HAPPrecondition(buffer != NULL);
    n = 0;
    HAPAssert(n <= length);
    if (n < length) {
        do {
            switch (r->state) {
                case util_JSON_READER_STATE_READING_WHITESPACE: {
                    n += skip_whitespace(&buffer[n], length - n);
                    HAPAssert(n <= length);
                    if (n < length) {
                        switch (buffer[n]) {
                            case '{': {
                                n++;
                                r->state = util_JSON_READER_STATE_BEGINNING_OBJECT;
                                break;
                            }
                            case '}': {
                                n++;
                                r->state = util_JSON_READER_STATE_COMPLETED_OBJECT;
                                break;
                            }
                            case '[': {
                                n++;
                                r->state = util_JSON_READER_STATE_BEGINNING_ARRAY;
                                break;
                            }
                            case ']': {
                                n++;
                                r->state = util_JSON_READER_STATE_COMPLETED_ARRAY;
                                break;
                            }
                            case '-':
                            case '0':
                            case '1':
                            case '2':
                            case '3':
                            case '4':
                            case '5':
                            case '6':
                            case '7':
                            case '8':
                            case '9': {
                                r->state = util_JSON_READER_STATE_BEGINNING_NUMBER;
                                break;
                            }
                            case '"': {
                                r->state = util_JSON_READER_STATE_BEGINNING_STRING;
                                break;
                            }
                            case 'f': {
                                r->state = util_JSON_READER_STATE_BEGINNING_FALSE;
                                break;
                            }
                            case 't': {
                                r->state = util_JSON_READER_STATE_BEGINNING_TRUE;
                                break;
                            }
                            case 'n': {
                                r->state = util_JSON_READER_STATE_BEGINNING_NULL;
                                break;
                            }
                            case ':': {
                                n++;
                                r->state = util_JSON_READER_STATE_AFTER_NAME_SEPARATOR;
                                break;
                            }
                            case ',': {
                                n++;
                                r->state = util_JSON_READER_STATE_AFTER_VALUE_SEPARATOR;
                                break;
                            }
                            default: {
                                r->state = util_JSON_READER_STATE_ERROR;
                                break;
                            }
                        }
                    }
                    break;
                }
                case util_JSON_READER_STATE_BEGINNING_OBJECT: { // NOLINT(bugprone-branch-clone)
                    r->state = util_JSON_READER_STATE_READING_WHITESPACE;
                    break;
                }
                case util_JSON_READER_STATE_COMPLETED_OBJECT: {
                    r->state = util_JSON_READER_STATE_READING_WHITESPACE;
                    break;
                }
                case util_JSON_READER_STATE_BEGINNING_ARRAY: {
                    r->state = util_JSON_READER_STATE_READING_WHITESPACE;
                    break;
                }
                case util_JSON_READER_STATE_COMPLETED_ARRAY: {
                    r->state = util_JSON_READER_STATE_READING_WHITESPACE;
                    break;
                }
                case util_JSON_READER_STATE_BEGINNING_NUMBER: {
                    switch (buffer[n]) {
                        case '-': {
                            n++;
                            r->state = util_JSON_READER_STATE_READING_NUMBER;
                            r->substate = SUBSTATE_READING_NUMBER_AFTER_MINUS;
                            break;
                        }
                        case '0': {
                            n++;
                            r->state = util_JSON_READER_STATE_READING_NUMBER;
                            r->substate = SUBSTATE_READING_NUMBER_AFTER_ZERO;
                            break;
                        }
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9': {
                            n++;
                            r->state = util_JSON_READER_STATE_READING_NUMBER;
                            r->substate = SUBSTATE_READING_NUMBER_INTEGER_PART;
                            break;
                        }
                        default: {
                            r->state = util_JSON_READER_STATE_ERROR;
                            break;
                        }
                    }
                    break;
                }
                case util_JSON_READER_STATE_READING_NUMBER: {
                    switch (r->substate) {
                        case SUBSTATE_READING_NUMBER_AFTER_MINUS: {
                            switch (buffer[n]) {
                                case '0': {
                                    n++;
                                    r->substate = SUBSTATE_READING_NUMBER_AFTER_ZERO;
                                    break;
                                }
                                case '1':
                                case '2':
                                case '3':
                                case '4':
                                case '5':
                                case '6':
                                case '7':
                                case '8':
                                case '9': {
                                    n++;
                                    r->substate = SUBSTATE_READING_NUMBER_INTEGER_PART;
                                    break;
                                }
                                default: {
                                    r->state = util_JSON_READER_STATE_ERROR;
                                    r->substate = SUBSTATE_NONE;
                                    break;
                                }
                            }
                            break;
                        }
                        case SUBSTATE_READING_NUMBER_AFTER_ZERO: {
                            switch (buffer[n]) {
                                case '.': {
                                    n++;
                                    r->substate = SUBSTATE_READING_NUMBER_FRACTION_PART;
                                    break;
                                }
                                case 'e':
                                case 'E': {
                                    n++;
                                    r->substate = SUBSTATE_READING_NUMBER_EXPONENT_PART;
                                    break;
                                }
                                case ' ':
                                case '\t':
                                case '\n':
                                case '\r':
                                case ']':
                                case '}':
                                case ',': {
                                    r->state = util_JSON_READER_STATE_COMPLETED_NUMBER;
                                    r->substate = SUBSTATE_NONE;
                                    break;
                                }
                                default: {
                                    r->state = util_JSON_READER_STATE_ERROR;
                                    r->substate = SUBSTATE_NONE;
                                    break;
                                }
                            }
                            break;
                        }
                        case SUBSTATE_READING_NUMBER_INTEGER_PART: {
                            n += skip_digits(&buffer[n], length - n);
                            HAPAssert(n <= length);
                            if (n < length) {
                                switch (buffer[n]) {
                                    case '.': {
                                        n++;
                                        r->substate = SUBSTATE_READING_NUMBER_FRACTION_PART;
                                        break;
                                    }
                                    case 'e':
                                    case 'E': {
                                        n++;
                                        r->substate = SUBSTATE_READING_NUMBER_EXPONENT_PART;
                                        break;
                                    }
                                    case ' ':
                                    case '\t':
                                    case '\n':
                                    case '\r':
                                    case ']':
                                    case '}':
                                    case ',': {
                                        r->state = util_JSON_READER_STATE_COMPLETED_NUMBER;
                                        r->substate = SUBSTATE_NONE;
                                        break;
                                    }
                                    default: {
                                        r->state = util_JSON_READER_STATE_ERROR;
                                        r->substate = SUBSTATE_NONE;
                                        break;
                                    }
                                }
                            }
                            break;
                        }
                        case SUBSTATE_READING_NUMBER_FRACTION_PART: {
                            switch (buffer[n]) {
                                case '0':
                                case '1':
                                case '2':
                                case '3':
                                case '4':
                                case '5':
                                case '6':
                                case '7':
                                case '8':
                                case '9': {
                                    n++;
                                    r->substate = SUBSTATE_READING_NUMBER_FRACTION_PART_AFTER_DIGIT;
                                    break;
                                }
                                default: {
                                    r->state = util_JSON_READER_STATE_ERROR;
                                    r->substate = SUBSTATE_NONE;
                                    break;
                                }
                            }
                            break;
                        }
                        case SUBSTATE_READING_NUMBER_FRACTION_PART_AFTER_DIGIT: {
                            n += skip_digits(&buffer[n], length - n);
                            HAPAssert(n <= length);
                            if (n < length) {
                                switch (buffer[n]) {
                                    case 'e':
                                    case 'E': {
                                        n++;
                                        r->substate = SUBSTATE_READING_NUMBER_EXPONENT_PART;
                                        break;
                                    }
                                    case ' ':
                                    case '\t':
                                    case '\n':
                                    case '\r':
                                    case ']':
                                    case '}':
                                    case ',': {
                                        r->state = util_JSON_READER_STATE_COMPLETED_NUMBER;
                                        r->substate = SUBSTATE_NONE;
                                        break;
                                    }
                                    default: {
                                        r->state = util_JSON_READER_STATE_ERROR;
                                        r->substate = SUBSTATE_NONE;
                                        break;
                                    }
                                }
                            }
                            break;
                        }
                        case SUBSTATE_READING_NUMBER_EXPONENT_PART: {
                            switch (buffer[n]) {
                                case '-':
                                case '+': {
                                    n++;
                                    r->substate = SUBSTATE_READING_NUMBER_EXPONENT_PART_AFTER_SIGN;
                                    break;
                                }
                                case '0':
                                case '1':
                                case '2':
                                case '3':
                                case '4':
                                case '5':
                                case '6':
                                case '7':
                                case '8':
                                case '9': {
                                    n++;
                                    r->substate = SUBSTATE_READING_NUMBER_EXPONENT_PART_AFTER_DIGIT;
                                    break;
                                }
                                default: {
                                    r->state = util_JSON_READER_STATE_ERROR;
                                    r->substate = SUBSTATE_NONE;
                                    break;
                                }
                            }
                            break;
                        }
                        case SUBSTATE_READING_NUMBER_EXPONENT_PART_AFTER_SIGN: {
                            switch (buffer[n]) {
                                case '0':
                                case '1':
                                case '2':
                                case '3':
                                case '4':
                                case '5':
                                case '6':
                                case '7':
                                case '8':
                                case '9': {
                                    n++;
                                    r->substate = SUBSTATE_READING_NUMBER_EXPONENT_PART_AFTER_DIGIT;
                                    break;
                                }
                                default: {
                                    r->state = util_JSON_READER_STATE_ERROR;
                                    r->substate = SUBSTATE_NONE;
                                    break;
                                }
                            }
                            break;
                        }
                        case SUBSTATE_READING_NUMBER_EXPONENT_PART_AFTER_DIGIT: {
                            n += skip_digits(&buffer[n], length - n);
                            HAPAssert(n <= length);
                            if (n < length) {
                                switch (buffer[n]) {
                                    case ' ':
                                    case '\t':
                                    case '\n':
                                    case '\r':
                                    case ']':
                                    case '}':
                                    case ',': {
                                        r->state = util_JSON_READER_STATE_COMPLETED_NUMBER;
                                        r->substate = SUBSTATE_NONE;
                                        break;
                                    }
                                    default: {
                                        r->state = util_JSON_READER_STATE_ERROR;
                                        r->substate = SUBSTATE_NONE;
                                        break;
                                    }
                                }
                            }
                            break;
                        }
                        default:
                            HAPFatalError();
                    }
                    break;
                }
                case util_JSON_READER_STATE_COMPLETED_NUMBER: {
                    r->state = util_JSON_READER_STATE_READING_WHITESPACE;
                    break;
                }
                case util_JSON_READER_STATE_BEGINNING_STRING: {
                    if (buffer[n] == '"') {
                        n++;
                        r->state = util_JSON_READER_STATE_READING_STRING;
                    } else {
                        r->state = util_JSON_READER_STATE_ERROR;
                    }
                    break;
                }
                case util_JSON_READER_STATE_READING_STRING: {
                    HAPAssert(n <= length);
                    while ((n < length) && ((r->substate != SUBSTATE_NONE) || (buffer[n] != '"'))) {
                        switch (r->substate) {
                            case SUBSTATE_NONE: {
                                if (buffer[n] == '\\') {
                                    r->substate = SUBSTATE_READING_STRING_AFTER_ESCAPE;
                                }
                                break;
                            }
                            case SUBSTATE_READING_STRING_AFTER_ESCAPE: {
                                r->substate = SUBSTATE_NONE;
                                break;
                            }
                            default:
                                HAPFatalError();
                        }
                        n++;
                    }
                    HAPAssert(n <= length);
                    if (n < length) {
                        n++;
                        r->state = util_JSON_READER_STATE_COMPLETED_STRING;
                    }
                    break;
                }
                case util_JSON_READER_STATE_COMPLETED_STRING: {
                    r->state = util_JSON_READER_STATE_READING_WHITESPACE;
                    break;
                }
                case util_JSON_READER_STATE_BEGINNING_FALSE: {
                    if (buffer[n] == 'f') {
                        n++;
                        r->state = util_JSON_READER_STATE_READING_FALSE;
                        r->substate = SUBSTATE_READING_FALSE_AFTER_F;
                    } else {
                        r->state = util_JSON_READER_STATE_ERROR;
                    }
                    break;
                }
                case util_JSON_READER_STATE_READING_FALSE: {
                    switch (r->substate) {
                        case SUBSTATE_READING_FALSE_AFTER_F: {
                            if (buffer[n] == 'a') {
                                n++;
                                r->substate = SUBSTATE_READING_FALSE_AFTER_FA;
                            } else {
                                r->state = util_JSON_READER_STATE_ERROR;
                                r->substate = SUBSTATE_NONE;
                            }
                            break;
                        }
                        case SUBSTATE_READING_FALSE_AFTER_FA: {
                            if (buffer[n] == 'l') {
                                n++;
                                r->substate = SUBSTATE_READING_FALSE_AFTER_FAL;
                            } else {
                                r->state = util_JSON_READER_STATE_ERROR;
                                r->substate = SUBSTATE_NONE;
                            }
                            break;
                        }
                        case SUBSTATE_READING_FALSE_AFTER_FAL: {
                            if (buffer[n] == 's') {
                                n++;
                                r->substate = SUBSTATE_READING_FALSE_AFTER_FALS;
                            } else {
                                r->state = util_JSON_READER_STATE_ERROR;
                                r->substate = SUBSTATE_NONE;
                            }
                            break;
                        }
                        case SUBSTATE_READING_FALSE_AFTER_FALS: {
                            if (buffer[n] == 'e') {
                                n++;
                                r->state = util_JSON_READER_STATE_COMPLETED_FALSE;
                                r->substate = SUBSTATE_NONE;
                            } else {
                                r->state = util_JSON_READER_STATE_ERROR;
                                r->substate = SUBSTATE_NONE;
                            }
                            break;
                        }
                        default:
                            HAPFatalError();
                    }
                    break;
                }
                case util_JSON_READER_STATE_COMPLETED_FALSE: {
                    r->state = util_JSON_READER_STATE_READING_WHITESPACE;
                    break;
                }
                case util_JSON_READER_STATE_BEGINNING_TRUE: {
                    if (buffer[n] == 't') {
                        n++;
                        r->state = util_JSON_READER_STATE_READING_TRUE;
                        r->substate = SUBSTATE_READING_TRUE_AFTER_T;
                    } else {
                        r->state = util_JSON_READER_STATE_ERROR;
                    }
                    break;
                }
                case util_JSON_READER_STATE_READING_TRUE: {
                    switch (r->substate) {
                        case SUBSTATE_READING_TRUE_AFTER_T: {
                            if (buffer[n] == 'r') {
                                n++;
                                r->substate = SUBSTATE_READING_TRUE_AFTER_TR;
                            } else {
                                r->state = util_JSON_READER_STATE_ERROR;
                                r->substate = SUBSTATE_NONE;
                            }
                            break;
                        }
                        case SUBSTATE_READING_TRUE_AFTER_TR: {
                            if (buffer[n] == 'u') {
                                n++;
                                r->substate = SUBSTATE_READING_TRUE_AFTER_TRU;
                            } else {
                                r->state = util_JSON_READER_STATE_ERROR;
                                r->substate = SUBSTATE_NONE;
                            }
                            break;
                        }
                        case SUBSTATE_READING_TRUE_AFTER_TRU: {
                            if (buffer[n] == 'e') {
                                n++;
                                r->state = util_JSON_READER_STATE_COMPLETED_TRUE;
                                r->substate = SUBSTATE_NONE;
                            } else {
                                r->state = util_JSON_READER_STATE_ERROR;
                                r->substate = SUBSTATE_NONE;
                            }
                            break;
                        }
                        default:
                            HAPFatalError();
                    }
                    break;
                }
                case util_JSON_READER_STATE_COMPLETED_TRUE: {
                    r->state = util_JSON_READER_STATE_READING_WHITESPACE;
                    break;
                }
                case util_JSON_READER_STATE_BEGINNING_NULL: {
                    if (buffer[n] == 'n') {
                        n++;
                        r->state = util_JSON_READER_STATE_READING_NULL;
                        r->substate = SUBSTATE_READING_NULL_AFTER_N;
                    } else {
                        r->state = util_JSON_READER_STATE_ERROR;
                    }
                    break;
                }
                case util_JSON_READER_STATE_READING_NULL: {
                    switch (r->substate) {
                        case SUBSTATE_READING_NULL_AFTER_N: {
                            if (buffer[n] == 'u') {
                                n++;
                                r->substate = SUBSTATE_READING_NULL_AFTER_NU;
                            } else {
                                r->state = util_JSON_READER_STATE_ERROR;
                                r->substate = SUBSTATE_NONE;
                            }
                            break;
                        }
                        case SUBSTATE_READING_NULL_AFTER_NU: {
                            if (buffer[n] == 'l') {
                                n++;
                                r->substate = SUBSTATE_READING_NULL_AFTER_NUL;
                            } else {
                                r->state = util_JSON_READER_STATE_ERROR;
                                r->substate = SUBSTATE_NONE;
                            }
                            break;
                        }
                        case SUBSTATE_READING_NULL_AFTER_NUL: {
                            if (buffer[n] == 'l') {
                                n++;
                                r->state = util_JSON_READER_STATE_COMPLETED_NULL;
                                r->substate = SUBSTATE_NONE;
                            } else {
                                r->state = util_JSON_READER_STATE_ERROR;
                                r->substate = SUBSTATE_NONE;
                            }
                            break;
                        }
                        default:
                            HAPFatalError();
                    }
                    break;
                }
                case util_JSON_READER_STATE_COMPLETED_NULL: { // NOLINT(bugprone-branch-clone)
                    r->state = util_JSON_READER_STATE_READING_WHITESPACE;
                    break;
                }
                case util_JSON_READER_STATE_AFTER_NAME_SEPARATOR: {
                    r->state = util_JSON_READER_STATE_READING_WHITESPACE;
                    break;
                }
                case util_JSON_READER_STATE_AFTER_VALUE_SEPARATOR: {
                    r->state = util_JSON_READER_STATE_READING_WHITESPACE;
                    break;
                }
                case util_JSON_READER_STATE_ERROR: {
                    break;
                }
                default:
                    HAPFatalError();
            }
        } while ((n < length) && (r->state != util_JSON_READER_STATE_BEGINNING_OBJECT) &&
                 (r->state != util_JSON_READER_STATE_COMPLETED_OBJECT) &&
                 (r->state != util_JSON_READER_STATE_BEGINNING_ARRAY) &&
                 (r->state != util_JSON_READER_STATE_COMPLETED_ARRAY) &&
                 (r->state != util_JSON_READER_STATE_BEGINNING_NUMBER) &&
                 (r->state != util_JSON_READER_STATE_COMPLETED_NUMBER) &&
                 (r->state != util_JSON_READER_STATE_BEGINNING_STRING) &&
                 (r->state != util_JSON_READER_STATE_COMPLETED_STRING) &&
                 (r->state != util_JSON_READER_STATE_BEGINNING_FALSE) &&
                 (r->state != util_JSON_READER_STATE_COMPLETED_FALSE) &&
                 (r->state != util_JSON_READER_STATE_BEGINNING_TRUE) &&
                 (r->state != util_JSON_READER_STATE_COMPLETED_TRUE) &&
                 (r->state != util_JSON_READER_STATE_BEGINNING_NULL) &&
                 (r->state != util_JSON_READER_STATE_COMPLETED_NULL) &&
                 (r->state != util_JSON_READER_STATE_AFTER_NAME_SEPARATOR) &&
                 (r->state != util_JSON_READER_STATE_AFTER_VALUE_SEPARATOR) &&
                 (r->state != util_JSON_READER_STATE_ERROR));
    }
    post = (n == length) || ((n < length) && ((r->state == util_JSON_READER_STATE_ERROR) ||
                                              (r->state == util_JSON_READER_STATE_BEGINNING_OBJECT) ||
                                              (r->state == util_JSON_READER_STATE_COMPLETED_OBJECT) ||
                                              (r->state == util_JSON_READER_STATE_BEGINNING_ARRAY) ||
                                              (r->state == util_JSON_READER_STATE_COMPLETED_ARRAY) ||
                                              (r->state == util_JSON_READER_STATE_BEGINNING_NUMBER) ||
                                              (r->state == util_JSON_READER_STATE_COMPLETED_NUMBER) ||
                                              (r->state == util_JSON_READER_STATE_BEGINNING_STRING) ||
                                              (r->state == util_JSON_READER_STATE_COMPLETED_STRING) ||
                                              (r->state == util_JSON_READER_STATE_BEGINNING_FALSE) ||
                                              (r->state == util_JSON_READER_STATE_COMPLETED_FALSE) ||
                                              (r->state == util_JSON_READER_STATE_BEGINNING_TRUE) ||
                                              (r->state == util_JSON_READER_STATE_COMPLETED_TRUE) ||
                                              (r->state == util_JSON_READER_STATE_BEGINNING_NULL) ||
                                              (r->state == util_JSON_READER_STATE_COMPLETED_NULL) ||
                                              (r->state == util_JSON_READER_STATE_AFTER_NAME_SEPARATOR) ||
                                              (r->state == util_JSON_READER_STATE_AFTER_VALUE_SEPARATOR)));
    HAPAssert(post);
    return n;
}
