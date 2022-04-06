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

#include "util_http_reader.h"

#define CR 13
#define LF 10
#define SP 32
#define HT 9

#define SUBSTATE_NONE     0
#define SUBSTATE_READING  1
#define SUBSTATE_AFTER_CR 2
#define SUBSTATE_AFTER_LF 3
#define SUBSTATE_AFTER_SP 4

void util_http_reader_init(struct util_http_reader* r, int type) {
    HAPPrecondition(r != NULL);
    if (type == util_HTTP_READER_TYPE_REQUEST) {
        r->type = util_HTTP_READER_TYPE_REQUEST;
        r->state = util_HTTP_READER_STATE_EXPECTING_METHOD;
    } else {
        HAPPrecondition(type == util_HTTP_READER_TYPE_RESPONSE);
        r->type = util_HTTP_READER_TYPE_RESPONSE;
        r->state = util_HTTP_READER_STATE_EXPECTING_VERSION;
    }
    r->substate = SUBSTATE_NONE;
    r->in_quoted_pair = 0;
    r->in_quoted_string = 0;
    r->result_token = NULL;
    r->result_length = 0;
}

HAP_RESULT_USE_CHECK
static int is_digit(int c) {
    return ('0' <= c) && (c <= '9');
}

HAP_RESULT_USE_CHECK
static int is_whitespace(int c) {
    return (c == SP) || (c == HT);
}

HAP_RESULT_USE_CHECK
static int is_token_char(int c) {
    return (33 <= c) && (c < 127) && (c != '(') && (c != ')') && (c != '<') && (c != '>') && (c != '@') && (c != ',') &&
           (c != ';') && (c != ':') && (c != '\\') && (c != '"') && (c != '/') && (c != '[') && (c != ']') &&
           (c != '?') && (c != '=') && (c != '{') && (c != '}');
}

HAP_RESULT_USE_CHECK
static int is_uri_char(int c) {
    return (('A' <= c) && (c <= 'Z')) || (('a' <= c) && (c <= 'z')) || (('0' <= c) && (c <= '9')) || (c == '%') ||
           (c == '-') || (c == '.') || (c == '_') || (c == '~') || (c == ':') || (c == '/') || (c == '?') ||
           (c == '#') || (c == '[') || (c == ']') || (c == '@') || (c == '!') || (c == '$') || (c == '&') ||
           (c == '\'') || (c == '(') || (c == ')') || (c == '*') || (c == '+') || (c == ',') || (c == ';') ||
           (c == '=');
}

HAP_RESULT_USE_CHECK
static int is_version_char(int c) {
    return (c == 'H') || (c == 'T') || (c == 'P') || (c == '/') || (c == '.') || (('0' <= c) && (c <= '9'));
}

HAP_RESULT_USE_CHECK
static int is_text_char(int c) {
    return (c < 0) || ((32 <= c) && (c < 127)) || (128 <= c) || (c == HT);
}

HAP_RESULT_USE_CHECK
static size_t skip_whitespace(char* buffer, size_t length) {
    size_t n;
    HAPPrecondition(buffer != NULL);
    n = 0;
    HAPAssert(n <= length);
    while ((n < length) && is_whitespace(buffer[n])) {
        n++;
    }
    HAPAssert((n == length) || ((n < length) && !is_whitespace(buffer[n])));
    return n;
}

HAP_RESULT_USE_CHECK
static size_t read_octets(struct util_http_reader* r, char* buffer, size_t length, int (*predicate)(int)) {
    size_t n;
    HAPPrecondition(r != NULL);
    HAPPrecondition(buffer != NULL);
    HAPPrecondition(predicate != NULL);
    n = 0;
    HAPAssert(n <= length);
    while ((n < length) && predicate(buffer[n])) {
        n++;
    }
    HAPAssert((n == length) || ((n < length) && !predicate(buffer[n])));
    r->result_token = buffer;
    r->result_length = n;
    return n;
}

HAP_RESULT_USE_CHECK
static size_t read_octets_and_quotes(struct util_http_reader* r, char* buffer, size_t length, int (*predicate)(int)) {
    size_t n;
    HAPPrecondition(r != NULL);
    HAPPrecondition(buffer != NULL);
    HAPPrecondition(predicate != NULL);
    n = 0;
    HAPAssert(n <= length);
    while ((n < length) && (r->in_quoted_pair || predicate(buffer[n]))) {
        if (r->in_quoted_pair) {
            r->in_quoted_pair = 0;
        } else if (r->in_quoted_string) {
            if (buffer[n] == '\\') {
                r->in_quoted_pair = 1;
            } else if (buffer[n] == '"') {
                r->in_quoted_string = 0;
            } else {
                // No special character.
            }
        } else if (buffer[n] == '"') {
            r->in_quoted_string = 1;
        } else {
            // Not in a quote.
        }
        n++;
    }
    HAPAssert((n == length) || ((n < length) && !r->in_quoted_pair && !predicate(buffer[n])));
    r->result_token = buffer;
    r->result_length = n;
    return n;
}

HAP_RESULT_USE_CHECK
size_t util_http_reader_read(struct util_http_reader* r, char* buffer, size_t length) {
    size_t n;
    int post;
    HAPPrecondition(r != NULL);
    HAPPrecondition(buffer != NULL);
    r->result_token = NULL;
    r->result_length = 0;
    n = 0;
    HAPAssert(n <= length);
    if (n < length) {
        do {
            switch (r->state) {
                case util_HTTP_READER_STATE_EXPECTING_METHOD: {
                    if (r->substate == SUBSTATE_NONE) {
                        n += skip_whitespace(&buffer[n], length - n);
                        HAPAssert(n <= length);
                        if (n < length) {
                            if (buffer[n] == CR) {
                                n++;
                                r->substate = SUBSTATE_AFTER_CR;
                            } else if (buffer[n] == LF) {
                                n++;
                            } else {
                                r->state = util_HTTP_READER_STATE_READING_METHOD;
                            }
                        }
                    } else {
                        HAPAssert(r->substate == SUBSTATE_AFTER_CR);
                        if (buffer[n] == LF) {
                            n++;
                            r->substate = SUBSTATE_NONE;
                        } else {
                            r->state = util_HTTP_READER_STATE_ERROR;
                        }
                    }
                    break;
                }
                case util_HTTP_READER_STATE_READING_METHOD: {
                    if (r->substate == SUBSTATE_NONE) {
                        if (is_token_char(buffer[n])) {
                            r->substate = SUBSTATE_READING;
                        } else {
                            r->state = util_HTTP_READER_STATE_ERROR;
                        }
                    } else {
                        HAPAssert(r->substate == SUBSTATE_READING);
                        n += read_octets(r, &buffer[n], length - n, is_token_char);
                        HAPAssert(n <= length);
                        if (n < length) {
                            r->state = util_HTTP_READER_STATE_COMPLETED_METHOD;
                            r->substate = SUBSTATE_NONE;
                        }
                    }
                    break;
                }
                case util_HTTP_READER_STATE_COMPLETED_METHOD: {
                    r->state = util_HTTP_READER_STATE_EXPECTING_URI;
                    break;
                }
                case util_HTTP_READER_STATE_EXPECTING_URI: {
                    if (r->substate == SUBSTATE_NONE) {
                        if (is_whitespace(buffer[n])) {
                            n++;
                            r->substate = SUBSTATE_AFTER_SP;
                        } else {
                            r->state = util_HTTP_READER_STATE_ERROR;
                        }
                    } else {
                        HAPAssert(r->substate == SUBSTATE_AFTER_SP);
                        n += skip_whitespace(&buffer[n], length - n);
                        HAPAssert(n <= length);
                        if (n < length) {
                            r->state = util_HTTP_READER_STATE_READING_URI;
                            r->substate = SUBSTATE_NONE;
                        }
                    }
                    break;
                }
                case util_HTTP_READER_STATE_READING_URI: {
                    if (r->substate == SUBSTATE_NONE) {
                        if (is_uri_char(buffer[n])) {
                            r->substate = SUBSTATE_READING;
                        } else {
                            r->state = util_HTTP_READER_STATE_ERROR;
                        }
                    } else {
                        HAPAssert(r->substate == SUBSTATE_READING);
                        n += read_octets(r, &buffer[n], length - n, is_uri_char);
                        HAPAssert(n <= length);
                        if (n < length) {
                            r->state = util_HTTP_READER_STATE_COMPLETED_URI;
                            r->substate = SUBSTATE_NONE;
                        }
                    }
                    break;
                }
                case util_HTTP_READER_STATE_COMPLETED_URI: {
                    r->state = util_HTTP_READER_STATE_EXPECTING_VERSION;
                    break;
                }
                case util_HTTP_READER_STATE_EXPECTING_VERSION: {
                    if (r->type == util_HTTP_READER_TYPE_REQUEST) {
                        if (r->substate == SUBSTATE_NONE) {
                            if (is_whitespace(buffer[n])) {
                                n++;
                                r->substate = SUBSTATE_AFTER_SP;
                            } else {
                                r->state = util_HTTP_READER_STATE_ERROR;
                            }
                        } else {
                            HAPAssert(r->substate == SUBSTATE_AFTER_SP);
                            n += skip_whitespace(&buffer[n], length - n);
                            HAPAssert(n <= length);
                            if (n < length) {
                                r->state = util_HTTP_READER_STATE_READING_VERSION;
                                r->substate = SUBSTATE_NONE;
                            }
                        }
                    } else {
                        HAPAssert(r->type == util_HTTP_READER_TYPE_RESPONSE);
                        if (r->substate == SUBSTATE_NONE) {
                            n += skip_whitespace(&buffer[n], length - n);
                            HAPAssert(n <= length);
                            if (n < length) {
                                if (buffer[n] == CR) {
                                    n++;
                                    r->substate = SUBSTATE_AFTER_CR;
                                } else if (buffer[n] == LF) {
                                    n++;
                                } else {
                                    r->state = util_HTTP_READER_STATE_READING_VERSION;
                                }
                            }
                        } else {
                            HAPAssert(r->substate == SUBSTATE_AFTER_CR);
                            if (buffer[n] == LF) {
                                n++;
                                r->substate = SUBSTATE_NONE;
                            } else {
                                r->state = util_HTTP_READER_STATE_ERROR;
                            }
                        }
                    }
                    break;
                }
                case util_HTTP_READER_STATE_READING_VERSION: {
                    if (r->substate == SUBSTATE_NONE) {
                        if (is_version_char(buffer[n])) {
                            r->substate = SUBSTATE_READING;
                        } else {
                            r->state = util_HTTP_READER_STATE_ERROR;
                        }
                    } else {
                        HAPAssert(r->substate == SUBSTATE_READING);
                        n += read_octets(r, &buffer[n], length - n, is_version_char);
                        HAPAssert(n <= length);
                        if (n < length) {
                            r->state = util_HTTP_READER_STATE_COMPLETED_VERSION;
                            r->substate = SUBSTATE_NONE;
                        }
                    }
                    break;
                }
                case util_HTTP_READER_STATE_COMPLETED_VERSION: {
                    if (r->type == util_HTTP_READER_TYPE_REQUEST) {
                        r->state = util_HTTP_READER_STATE_EXPECTING_HEADER_NAME;
                    } else {
                        HAPAssert(r->type == util_HTTP_READER_TYPE_RESPONSE);
                        r->state = util_HTTP_READER_STATE_EXPECTING_STATUS;
                    }
                    break;
                }
                case util_HTTP_READER_STATE_EXPECTING_STATUS: {
                    if (r->substate == SUBSTATE_NONE) {
                        if (is_whitespace(buffer[n])) {
                            n++;
                            r->substate = SUBSTATE_AFTER_SP;
                        } else {
                            r->state = util_HTTP_READER_STATE_ERROR;
                        }
                    } else {
                        HAPAssert(r->substate == SUBSTATE_AFTER_SP);
                        n += skip_whitespace(&buffer[n], length - n);
                        HAPAssert(n <= length);
                        if (n < length) {
                            r->state = util_HTTP_READER_STATE_READING_STATUS;
                            r->substate = SUBSTATE_NONE;
                        }
                    }
                    break;
                }
                case util_HTTP_READER_STATE_READING_STATUS: {
                    if (r->substate == SUBSTATE_NONE) {
                        if (is_digit(buffer[n])) {
                            r->substate = SUBSTATE_READING;
                        } else {
                            r->state = util_HTTP_READER_STATE_ERROR;
                        }
                    } else {
                        HAPAssert(r->substate == SUBSTATE_READING);
                        n += read_octets(r, &buffer[n], length - n, is_digit);
                        HAPAssert(n <= length);
                        if (n < length) {
                            r->state = util_HTTP_READER_STATE_COMPLETED_STATUS;
                            r->substate = SUBSTATE_NONE;
                        }
                    }
                    break;
                }
                case util_HTTP_READER_STATE_COMPLETED_STATUS: {
                    r->state = util_HTTP_READER_STATE_EXPECTING_REASON;
                    break;
                }
                case util_HTTP_READER_STATE_EXPECTING_REASON: {
                    if (r->substate == SUBSTATE_NONE) {
                        if (is_whitespace(buffer[n])) {
                            n++;
                            r->substate = SUBSTATE_AFTER_SP;
                        } else {
                            r->state = util_HTTP_READER_STATE_ERROR;
                        }
                    } else {
                        HAPAssert(r->substate == SUBSTATE_AFTER_SP);
                        n += skip_whitespace(&buffer[n], length - n);
                        HAPAssert(n <= length);
                        if (n < length) {
                            r->state = util_HTTP_READER_STATE_READING_REASON;
                            r->substate = SUBSTATE_NONE;
                        }
                    }
                    break;
                }
                case util_HTTP_READER_STATE_READING_REASON: {
                    n += read_octets(r, &buffer[n], length - n, is_text_char);
                    HAPAssert(n <= length);
                    if (n < length) {
                        r->state = util_HTTP_READER_STATE_COMPLETED_REASON;
                    }
                    break;
                }
                case util_HTTP_READER_STATE_COMPLETED_REASON: {
                    r->state = util_HTTP_READER_STATE_EXPECTING_HEADER_NAME;
                    break;
                }
                case util_HTTP_READER_STATE_EXPECTING_HEADER_NAME: {
                    if (r->substate == SUBSTATE_NONE) {
                        if (buffer[n] == CR) {
                            n++;
                            r->substate = SUBSTATE_AFTER_CR;
                        } else if (buffer[n] == LF) {
                            n++;
                            r->state = util_HTTP_READER_STATE_READING_HEADER_NAME;
                        } else {
                            r->state = util_HTTP_READER_STATE_ERROR;
                        }
                    } else {
                        HAPAssert(r->substate == SUBSTATE_AFTER_CR);
                        if (buffer[n] == LF) {
                            n++;
                            r->state = util_HTTP_READER_STATE_READING_HEADER_NAME;
                            r->substate = SUBSTATE_NONE;
                        } else {
                            r->state = util_HTTP_READER_STATE_ERROR;
                        }
                    }
                    break;
                }
                case util_HTTP_READER_STATE_READING_HEADER_NAME: {
                    if (r->substate == SUBSTATE_NONE) {
                        if (is_token_char(buffer[n])) {
                            r->substate = SUBSTATE_READING;
                        } else {
                            r->state = util_HTTP_READER_STATE_ENDING_HEADER_LINES;
                        }
                    } else {
                        HAPAssert(r->substate == SUBSTATE_READING);
                        n += read_octets(r, &buffer[n], length - n, is_token_char);
                        HAPAssert(n <= length);
                        if (n < length) {
                            r->state = util_HTTP_READER_STATE_COMPLETED_HEADER_NAME;
                            r->substate = SUBSTATE_NONE;
                        }
                    }
                    break;
                }
                case util_HTTP_READER_STATE_COMPLETED_HEADER_NAME: {
                    r->state = util_HTTP_READER_STATE_EXPECTING_HEADER_VALUE;
                    break;
                }
                case util_HTTP_READER_STATE_EXPECTING_HEADER_VALUE: {
                    if (buffer[n] == ':') {
                        n++;
                        r->state = util_HTTP_READER_STATE_READING_HEADER_VALUE;
                    } else {
                        r->state = util_HTTP_READER_STATE_ERROR;
                    }
                    break;
                }
                case util_HTTP_READER_STATE_READING_HEADER_VALUE: {
                    n += read_octets_and_quotes(r, &buffer[n], length - n, is_text_char);
                    HAPAssert(n <= length);
                    if (n < length) {
                        r->state = util_HTTP_READER_STATE_COMPLETED_HEADER_VALUE;
                    }
                    break;
                }
                case util_HTTP_READER_STATE_COMPLETED_HEADER_VALUE: {
                    r->state = util_HTTP_READER_STATE_ENDING_HEADER_LINE;
                    break;
                }
                case util_HTTP_READER_STATE_ENDING_HEADER_LINE: {
                    if (r->substate == SUBSTATE_NONE) {
                        if (buffer[n] == CR) {
                            n++;
                            r->substate = SUBSTATE_AFTER_CR;
                        } else if (buffer[n] == LF) {
                            n++;
                            r->substate = SUBSTATE_AFTER_LF;
                        } else {
                            r->state = util_HTTP_READER_STATE_ERROR;
                        }
                    } else if (r->substate == SUBSTATE_AFTER_CR) {
                        if (buffer[n] == LF) {
                            n++;
                            r->substate = SUBSTATE_AFTER_LF;
                        } else {
                            r->state = util_HTTP_READER_STATE_ERROR;
                        }
                    } else {
                        HAPAssert(r->substate == SUBSTATE_AFTER_LF);
                        if (is_whitespace(buffer[n])) {
                            r->state = util_HTTP_READER_STATE_READING_HEADER_VALUE;
                            r->substate = SUBSTATE_NONE;
                        } else if (r->in_quoted_string) {
                            r->state = util_HTTP_READER_STATE_ERROR;
                        } else {
                            r->state = util_HTTP_READER_STATE_READING_HEADER_NAME;
                            r->substate = SUBSTATE_NONE;
                        }
                    }
                    break;
                }
                case util_HTTP_READER_STATE_ENDING_HEADER_LINES: {
                    if (r->substate == SUBSTATE_NONE) {
                        if (buffer[n] == CR) {
                            n++;
                            r->substate = SUBSTATE_AFTER_CR;
                        } else if (buffer[n] == LF) {
                            n++;
                            r->state = util_HTTP_READER_STATE_DONE;
                        } else {
                            r->state = util_HTTP_READER_STATE_ERROR;
                        }
                    } else {
                        HAPAssert(r->substate == SUBSTATE_AFTER_CR);
                        if (buffer[n] == LF) {
                            n++;
                            r->state = util_HTTP_READER_STATE_DONE;
                            r->substate = SUBSTATE_NONE;
                        } else {
                            r->state = util_HTTP_READER_STATE_ERROR;
                        }
                    }
                    break;
                }
                case util_HTTP_READER_STATE_DONE:
                case util_HTTP_READER_STATE_ERROR: {
                    break;
                }
                default:
                    HAPFatalError();
            }
        } while ((n < length) && (r->state != util_HTTP_READER_STATE_COMPLETED_METHOD) &&
                 (r->state != util_HTTP_READER_STATE_COMPLETED_URI) &&
                 (r->state != util_HTTP_READER_STATE_COMPLETED_VERSION) &&
                 (r->state != util_HTTP_READER_STATE_COMPLETED_STATUS) &&
                 (r->state != util_HTTP_READER_STATE_COMPLETED_REASON) &&
                 (r->state != util_HTTP_READER_STATE_COMPLETED_HEADER_NAME) &&
                 (r->state != util_HTTP_READER_STATE_COMPLETED_HEADER_VALUE) &&
                 (r->state != util_HTTP_READER_STATE_DONE) && (r->state != util_HTTP_READER_STATE_ERROR));
    }
    post = (n == length) ||
           ((n < length) &&
            ((r->state == util_HTTP_READER_STATE_ERROR) || (r->state == util_HTTP_READER_STATE_COMPLETED_METHOD) ||
             (r->state == util_HTTP_READER_STATE_COMPLETED_URI) ||
             (r->state == util_HTTP_READER_STATE_COMPLETED_VERSION) ||
             (r->state == util_HTTP_READER_STATE_COMPLETED_STATUS) ||
             (r->state == util_HTTP_READER_STATE_COMPLETED_REASON) ||
             (r->state == util_HTTP_READER_STATE_COMPLETED_HEADER_NAME) ||
             (r->state == util_HTTP_READER_STATE_COMPLETED_HEADER_VALUE) || (r->state == util_HTTP_READER_STATE_DONE)));
    HAPAssert(post);
    return n;
}
