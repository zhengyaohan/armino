/*
 * Disclaimer: IMPORTANT: This Apple software is supplied to you, by Apple Inc. ("Apple"), in your
 * capacity as a current, and in good standing, Licensee in the MFi Licensing Program. Use of this
 * Apple software is governed by and subject to the terms and conditions of your MFi License,
 * including, but not limited to, the restrictions specified in the provision entitled "Public
 * Software", and is further subject to your agreement to the following additional terms, and your
 * agreement that the use, installation, modification or redistribution of this Apple software
 * constitutes acceptance of these additional terms. If you do not agree with these additional terms,
 * you may not use, install, modify or redistribute this Apple software.
 *
 * Subject to all of these terms and in consideration of your agreement to abide by them, Apple grants
 * you, for as long as you are a current and in good-standing MFi Licensee, a personal, non-exclusive
 * license, under Apple's copyrights in this Apple software (the "Apple Software"), to use,
 * reproduce, and modify the Apple Software in source form, and to use, reproduce, modify, and
 * redistribute the Apple Software, with or without modifications, in binary form, in each of the
 * foregoing cases to the extent necessary to develop and/or manufacture "Proposed Products" and
 * "Licensed Products" in accordance with the terms of your MFi License. While you may not
 * redistribute the Apple Software in source form, should you redistribute the Apple Software in binary
 * form, you must retain this notice and the following text and disclaimers in all such redistributions
 * of the Apple Software. Neither the name, trademarks, service marks, or logos of Apple Inc. may be
 * used to endorse or promote products derived from the Apple Software without specific prior written
 * permission from Apple. Except as expressly stated in this notice, no other rights or licenses,
 * express or implied, are granted by Apple herein, including but not limited to any patent rights that
 * may be infringed by your derivative works or by other works in which the Apple Software may be
 * incorporated. Apple may terminate this license to the Apple Software by removing it from the list
 * of Licensed Technology in the MFi License, or otherwise in accordance with the terms of such MFi License.
 *
 * Unless you explicitly state otherwise, if you provide any ideas, suggestions, recommendations, bug
 * fixes or enhancements to Apple in connection with this software ("Feedback"), you hereby grant to
 * Apple a non-exclusive, fully paid-up, perpetual, irrevocable, worldwide license to make, use,
 * reproduce, incorporate, modify, display, perform, sell, make or have made derivative works of,
 * distribute (directly or indirectly) and sublicense, such Feedback in connection with Apple products
 * and services. Providing this Feedback is voluntary, but if you do provide Feedback to Apple, you
 * acknowledge and agree that Apple may exercise the license granted above without the payment of
 * royalties or further consideration to Participant.
 * The Apple Software is provided by Apple on an "AS IS" basis. APPLE MAKES NO WARRANTIES, EXPRESS OR
 * IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR
 * IN COMBINATION WITH YOUR PRODUCTS.
 *
 * IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION
 * AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
 * (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright (C) 2020 Apple Inc. All Rights Reserved.
 */

#ifndef CoreUARPPlatform_h
#define CoreUARPPlatform_h

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef UARP_DISABLE_LOGS
#define UARP_DISABLE_LOGS 0
#endif

#ifndef UARP_DISABLE_REQUIRE_LOGS
#define UARP_DISABLE_REQUIRE_LOGS 0
#endif

#ifndef UARP_DISABLE_VERIFY
#define UARP_DISABLE_VERIFY 0
#endif

#ifndef UARP_DISABLE_COMPRESSION
#define UARP_DISABLE_COMPRESSION 0
#endif

#ifndef UARP_DISABLE_HASH
#define UARP_DISABLE_HASH 0
#endif

#ifndef UARP_DISABLE_ASSET_CREATION
#define UARP_DISABLE_ASSET_CREATION 1
#endif

#ifndef UARP_DISABLE_PERSONALIZATION
#define UARP_DISABLE_PERSONALIZATION 1
#endif

typedef enum
{
    kUARPLoggingCategoryAccessory,
    kUARPLoggingCategoryController,
    kUARPLoggingCategoryPlatform,
    kUARPLoggingCategoryProduct,
    kUARPLoggingCategoryMemory,
    kUARPLoggingCategoryAssert,
    kUARPLoggingCategoryMax
}
UARPLoggingCategory;

#if !(UARP_DISABLE_LOGS)

void uarpLogError( UARPLoggingCategory category, const char *msg, ... );
void uarpLogDebug( UARPLoggingCategory category, const char *msg, ... );
void uarpLogInfo( UARPLoggingCategory category, const char *msg, ... );

#else

#define uarpLogError( category, msg, ... )
#define uarpLogDebug( category, msg, ... )
#define uarpLogInfo( category, msg, ... )

#endif

#if !(UARP_DISABLE_REQUIRE_LOGS)

void uarpLogFault( UARPLoggingCategory category, const char *msg, ... );

#else

#define uarpLogFault( category, msg, ... )

#endif

void * uarpZalloc( size_t length );
void uarpFree( void * pBuffer );

uint64_t uarpHtonll( uint64_t val64 );
uint64_t uarpNtohll( uint64_t val64 );

uint32_t uarpHtonl( uint32_t val32 );
uint32_t uarpNtohl( uint32_t val32 );

uint16_t uarpHtons( uint16_t val16 );
uint16_t uarpNtohs( uint16_t val16 );

#define __UARP_Require(assertion, exceptionLabel) \
    do \
    { \
        if ( !(assertion) ) \
        { \
            uarpLogFault(kUARPLoggingCategoryAssert, "AssertMacros: %s, file: %s:%d\n", #assertion,  __FILE__, __LINE__); \
            goto exceptionLabel; \
        } \
    } while ( 0 )

#define __UARP_Require_Action(assertion, exceptionLabel, action) \
    do \
    { \
        if ( !(assertion) ) \
        { \
            uarpLogFault(kUARPLoggingCategoryAssert, "AssertMacros: %s, file: %s:%d\n", #assertion,  __FILE__, __LINE__); \
            { \
                action; \
            } \
            goto exceptionLabel; \
        } \
    } while ( 0 )

#define __UARP_Require_Quiet(assertion, exceptionLabel) \
    do \
    { \
        if ( !(assertion) ) \
        { \
            goto exceptionLabel; \
        } \
    } while ( 0 )

#define __UARP_Require_Action_Quiet(assertion, exceptionLabel, action) \
    do \
    { \
        if ( !(assertion) ) \
        { \
            { \
                action; \
            } \
            goto exceptionLabel; \
        } \
    } while ( 0 )

#define __UARP_Check(assertion) \
    do \
    { \
        if ( !(assertion) ) \
        { \
            uarpLogFault(kUARPLoggingCategoryAssert, "AssertMacros: %s, file: %s:%d\n", #assertion,  __FILE__, __LINE__); \
        } \
    } while ( 0 )

#if !(UARP_DISABLE_VERIFY)

#define __UARP_Verify_Action(assertion, exceptionLabel, action) \
    do \
    { \
        if ( !(assertion) ) \
        { \
            uarpLogFault(kUARPLoggingCategoryAssert, "AssertMacros: %s, file: %s:%d\n", #assertion,  __FILE__, __LINE__); \
            { \
                action; \
            } \
            goto exceptionLabel; \
        } \
    } while ( 0 )

#define __UARP_Verify_exit exit:

#else

#define __UARP_Verify_Action(assertion, exceptionLabel, action)

#define __UARP_Verify_exit

#endif

#if !(UARP_DISABLE_COMPRESSION)

uint32_t uarpDecompressBuffer( int algorithm, const void *pCompressed, uint32_t lengthCompressed,
                              void *pDecompressed, uint32_t lengthDecompressed );

uint32_t uarpCompressBuffer( int algorithm, const void *pBuffer, uint32_t lengthBuffer,
                            void *pCompressed, uint32_t *pLengthCompressed );

#endif

#if !(UARP_DISABLE_HASH)

void uarpPayloadHashInit( int algorithm, void *context );

void uarpPayloadHashUpdate( int algorithm, void *context, const void *pBuffer, uint32_t length );

void uarpPayloadHashFinal( int algorithm, void *context, uint8_t *pHash, uint32_t length );

void uarpPayloadHashLog( int algorithm, void *context );

#endif

#ifdef __cplusplus
}
#endif

#endif /* CoreUARPPlatform_h */
