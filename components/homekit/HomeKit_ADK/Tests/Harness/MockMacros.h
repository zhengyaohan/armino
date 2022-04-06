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
// Copyright (C) 2020 Apple Inc. All Rights Reserved.

#ifndef __MOCK_MACROS_H_
#define __MOCK_MACROS_H_

#ifdef __cplusplus
extern "C" {
#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

// Argument list reformatting macros.
// Note that MOCK_ARG() is just to make sure the type and name are grouped correctly.
#define MOCK_ARG(_type, _name) _type, _name

#define CALL_N(fn, ...) \
    CALL_N_HELPER(fn, __VA_ARGS__, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1) \
    (__VA_ARGS__)
#define CALL_N_HELPER( \
        fn, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _n, ...) \
    fn##_n

#define MOCK_ARGS_DECL_2(_type0, _name0)                                 (_type0 _name0)
#define MOCK_ARGS_DECL_4(_type0, _name0, _type1, _name1)                 (_type0 _name0, _type1 _name1)
#define MOCK_ARGS_DECL_6(_type0, _name0, _type1, _name1, _type2, _name2) (_type0 _name0, _type1 _name1, _type2 _name2)
#define MOCK_ARGS_DECL_8(_type0, _name0, _type1, _name1, _type2, _name2, _type3, _name3) \
    (_type0 _name0, _type1 _name1, _type2 _name2, _type3 _name3)
#define MOCK_ARGS_DECL_10(_type0, _name0, _type1, _name1, _type2, _name2, _type3, _name3, _type4, _name4) \
    (_type0 _name0, _type1 _name1, _type2 _name2, _type3 _name3, _type4 _name4)
#define MOCK_ARGS_DECL_12( \
        _type0, _name0, _type1, _name1, _type2, _name2, _type3, _name3, _type4, _name4, _type5, _name5) \
    (_type0 _name0, _type1 _name1, _type2 _name2, _type3 _name3, _type4 _name4, _type5 _name5)
#define MOCK_ARGS_DECL_14( \
        _type0, \
        _name0, \
        _type1, \
        _name1, \
        _type2, \
        _name2, \
        _type3, \
        _name3, \
        _type4, \
        _name4, \
        _type5, \
        _name5, \
        _type6, \
        _name6) \
    (_type0 _name0, _type1 _name1, _type2 _name2, _type3 _name3, _type4 _name4, _type5 _name5, _type6 _name6)
#define MOCK_ARGS_DECL_16( \
        _type0, \
        _name0, \
        _type1, \
        _name1, \
        _type2, \
        _name2, \
        _type3, \
        _name3, \
        _type4, \
        _name4, \
        _type5, \
        _name5, \
        _type6, \
        _name6, \
        _type7, \
        _name_7) \
    (_type0 _name0, \
     _type1 _name1, \
     _type2 _name2, \
     _type3 _name3, \
     _type4 _name4, \
     _type5 _name5, \
     _type6 _name6, \
     _type7 _name7)
#define MOCK_ARGS_DECL_18( \
        _type0, \
        _name0, \
        _type1, \
        _name1, \
        _type2, \
        _name2, \
        _type3, \
        _name3, \
        _type4, \
        _name4, \
        _type5, \
        _name5, \
        _type6, \
        _name6, \
        _type7, \
        _name_7, \
        _type8, \
        _name8) \
    (_type0 _name0, \
     _type1 _name1, \
     _type2 _name2, \
     _type3 _name3, \
     _type4 _name4, \
     _type5 _name5, \
     _type6 _name6, \
     _type7 _name7, \
     _type8 _name8)
#define MOCK_ARGS_DECL_20( \
        _type0, \
        _name0, \
        _type1, \
        _name1, \
        _type2, \
        _name2, \
        _type3, \
        _name3, \
        _type4, \
        _name4, \
        _type5, \
        _name5, \
        _type6, \
        _name6, \
        _type7, \
        _name_7, \
        _type8, \
        _name8, \
        _type9, \
        _name9) \
    (_type0 _name0, \
     _type1 _name1, \
     _type2 _name2, \
     _type3 _name3, \
     _type4 _name4, \
     _type5 _name5, \
     _type6 _name6, \
     _type7 _name7, \
     _type8 _name8, \
     _type9 _name9)
#define MOCK_ARGS_DECL(...) CALL_N(MOCK_ARGS_DECL_, __VA_ARGS__)

#define MOCK_CALL_ARGS_2(_type0, _name0)                                 (_name0)
#define MOCK_CALL_ARGS_4(_type0, _name0, _type1, _name1)                 (_name0, _name1)
#define MOCK_CALL_ARGS_6(_type0, _name0, _type1, _name1, _type2, _name2) (_name0, _name1, _name2)
#define MOCK_CALL_ARGS_8(_type0, _name0, _type1, _name1, _type2, _name2, _type3, _name3) \
    (_name0, _name1, _name2, _name3)
#define MOCK_CALL_ARGS_10(_type0, _name0, _type1, _name1, _type2, _name2, _type3, _name3, _type4, _name4) \
    (_name0, _name1, _name2, _name3, _name4)
#define MOCK_CALL_ARGS_12( \
        _type0, _name0, _type1, _name1, _type2, _name2, _type3, _name3, _type4, _name4, _type5, _name5) \
    (_name0, _name1, _name2, _name3, _name4, _name5)
#define MOCK_CALL_ARGS_14( \
        _type0, \
        _name0, \
        _type1, \
        _name1, \
        _type2, \
        _name2, \
        _type3, \
        _name3, \
        _type4, \
        _name4, \
        _type5, \
        _name5, \
        _type6, \
        _name6) \
    (_name0, _name1, _name2, _name3, _name4, _name5, _name6)
#define MOCK_CALL_ARGS_16( \
        _type0, \
        _name0, \
        _type1, \
        _name1, \
        _type2, \
        _name2, \
        _type3, \
        _name3, \
        _type4, \
        _name4, \
        _type5, \
        _name5, \
        _type6, \
        _name6, \
        _type7, \
        _name_7) \
    (_name0, _name1, _name2, _name3, _name4, _name5, _name6, _name7)
#define MOCK_CALL_ARGS_18( \
        _type0, \
        _name0, \
        _type1, \
        _name1, \
        _type2, \
        _name2, \
        _type3, \
        _name3, \
        _type4, \
        _name4, \
        _type5, \
        _name5, \
        _type6, \
        _name6, \
        _type7, \
        _name_7, \
        _type8, \
        _name8) \
    (_name0, _name1, _name2, _name3, _name4, _name5, _name6, _name7, _name8)
#define MOCK_CALL_ARGS_20( \
        _type0, \
        _name0, \
        _type1, \
        _name1, \
        _type2, \
        _name2, \
        _type3, \
        _name3, \
        _type4, \
        _name4, \
        _type5, \
        _name5, \
        _type6, \
        _name6, \
        _type7, \
        _name_7, \
        _type8, \
        _name8, \
        _type9, \
        _name9) \
    (_name0, _name1, _name2, _name3, _name4, _name5, _name6, _name7, _name8, _name9)
#define MOCK_CALL_ARGS(...) CALL_N(MOCK_CALL_ARGS_, __VA_ARGS__)

#define MOCK_ARGS_TYPES_2(_type0, _name0)                                                 _type0
#define MOCK_ARGS_TYPES_4(_type0, _name0, _type1, _name1)                                 _type0, _type1
#define MOCK_ARGS_TYPES_6(_type0, _name0, _type1, _name1, _type2, _name2)                 _type0, _type1, _type2
#define MOCK_ARGS_TYPES_8(_type0, _name0, _type1, _name1, _type2, _name2, _type3, _name3) _type0, _type1, _type2, _type3
#define MOCK_ARGS_TYPES_10(_type0, _name0, _type1, _name1, _type2, _name2, _type3, _name3, _type4, _name4) \
    _type0, _type1, _type2, _type3, _type4
#define MOCK_ARGS_TYPES_12( \
        _type0, _name0, _type1, _name1, _type2, _name2, _type3, _name3, _type4, _name4, _type5, _name5) \
    _type0, _type1, _type2, _type3, _type4, _type5
#define MOCK_ARGS_TYPES_14( \
        _type0, \
        _name0, \
        _type1, \
        _name1, \
        _type2, \
        _name2, \
        _type3, \
        _name3, \
        _type4, \
        _name4, \
        _type5, \
        _name5, \
        _type6, \
        _name6) \
    _type0, _type1, _type2, _type3, _type4, _type5, _type6
#define MOCK_ARGS_TYPES_16( \
        _type0, \
        _name0, \
        _type1, \
        _name1, \
        _type2, \
        _name2, \
        _type3, \
        _name3, \
        _type4, \
        _name4, \
        _type5, \
        _name5, \
        _type6, \
        _name6, \
        _type7, \
        _name_7) \
    _type0, _type1, _type2, _type3, _type4, _type5, _type6, _type7
#define MOCK_ARGS_TYPES_18( \
        _type0, \
        _name0, \
        _type1, \
        _name1, \
        _type2, \
        _name2, \
        _type3, \
        _name3, \
        _type4, \
        _name4, \
        _type5, \
        _name5, \
        _type6, \
        _name6, \
        _type7, \
        _name_7, \
        _type8, \
        _name8) \
    _type0, _type1, _type2, _type3, _type4, _type5, _type6, _type7, _type8
#define MOCK_ARGS_TYPES_20( \
        _type0, \
        _name0, \
        _type1, \
        _name1, \
        _type2, \
        _name2, \
        _type3, \
        _name3, \
        _type4, \
        _name4, \
        _type5, \
        _name5, \
        _type6, \
        _name6, \
        _type7, \
        _name_7, \
        _type8, \
        _name8, \
        _type9, \
        _name9) \
    _type0, _type1, _type2, _type3, _type4, _type5, _type6, _type7, _type8, _type9
#define MOCK_ARGS_TYPES(...) CALL_N(MOCK_ARGS_TYPES_, __VA_ARGS__)

// Struct field list reformatting macros.
// Note that MOCK_STRUCT_FIELD() is just to make sure the type and name are grouped correctly.
#define MOCK_STRUCT_FIELD(_type, _name) _type, _name

#define MOCK_STRUCT_FIELDS_DECL_2(_type0, _name0) \
    { _type0 _name0; }
#define MOCK_STRUCT_FIELDS_DECL_4(_type0, _name0, _type1, _name1) \
    { \
        _type0 _name0; \
        _type1 _name1; \
    }
#define MOCK_STRUCT_FIELDS_DECL_6(_type0, _name0, _type1, _name1, _type2, _name2) \
    { \
        _type0 _name0; \
        _type1 _name1; \
        _type2 _name2; \
    }
#define MOCK_STRUCT_FIELDS_DECL_8(_type0, _name0, _type1, _name1, _type2, _name2, _type3, _name3) \
    { \
        _type0 _name0; \
        _type1 _name1; \
        _type2 _name2; \
        _type3 _name3; \
    }
#define MOCK_STRUCT_FIELDS_DECL_10(_type0, _name0, _type1, _name1, _type2, _name2, _type3, _name3, _type4, _name4) \
    { \
        _type0 _name0; \
        _type1 _name1; \
        _type2 _name2; \
        _type3 _name3; \
        _type4 _name4; \
    }
#define MOCK_STRUCT_FIELDS_DECL_12( \
        _type0, _name0, _type1, _name1, _type2, _name2, _type3, _name3, _type4, _name4, _type5, _name5) \
    { \
        _type0 _name0; \
        _type1 _name1; \
        _type2 _name2; \
        _type3 _name3; \
        _type4 _name4; \
        _type5 _name5; \
    }
#define MOCK_STRUCT_FIELDS_DECL_14( \
        _type0, \
        _name0, \
        _type1, \
        _name1, \
        _type2, \
        _name2, \
        _type3, \
        _name3, \
        _type4, \
        _name4, \
        _type5, \
        _name5, \
        _type6, \
        _name6) \
    { \
        _type0 _name0; \
        _type1 _name1; \
        _type2 _name2; \
        _type3 _name3; \
        _type4 _name4; \
        _type5 _name5; \
        _type6 _name6; \
    }
#define MOCK_STRUCT_FIELDS_DECL_16( \
        _type0, \
        _name0, \
        _type1, \
        _name1, \
        _type2, \
        _name2, \
        _type3, \
        _name3, \
        _type4, \
        _name4, \
        _type5, \
        _name5, \
        _type6, \
        _name6, \
        _type7, \
        _name_7) \
    { \
        _type0 _name0; \
        _type1 _name1; \
        _type2 _name2; \
        _type3 _name3; \
        _type4 _name4; \
        _type5 _name5; \
        _type6 _name6; \
        _type7 _name7; \
    }
#define MOCK_STRUCT_FIELDS_DECL_18( \
        _type0, \
        _name0, \
        _type1, \
        _name1, \
        _type2, \
        _name2, \
        _type3, \
        _name3, \
        _type4, \
        _name4, \
        _type5, \
        _name5, \
        _type6, \
        _name6, \
        _type7, \
        _name_7, \
        _type8, \
        _name8) \
    { \
        _type0 _name0; \
        _type1 _name1; \
        _type2 _name2; \
        _type3 _name3; \
        _type4 _name4; \
        _type5 _name5; \
        _type6 _name6; \
        _type7 _name7; \
        _type8 _name8; \
    }
#define MOCK_STRUCT_FIELDS_DECL_20( \
        _type0, \
        _name0, \
        _type1, \
        _name1, \
        _type2, \
        _name2, \
        _type3, \
        _name3, \
        _type4, \
        _name4, \
        _type5, \
        _name5, \
        _type6, \
        _name6, \
        _type7, \
        _name_7, \
        _type8, \
        _name8, \
        _type9, \
        _name9) \
    { \
        _type0 _name0; \
        _type1 _name1; \
        _type2 _name2; \
        _type3 _name3; \
        _type4 _name4; \
        _type5 _name5; \
        _type6 _name6; \
        _type7 _name7; \
        _type8 _name8; \
        _type9 _name9; \
    }
#define MOCK_STRUCT_FIELDS_DECL(...) CALL_N(MOCK_STRUCT_FIELDS_DECL_, __VA_ARGS__)

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif // __MOCK_MACROS_H_
