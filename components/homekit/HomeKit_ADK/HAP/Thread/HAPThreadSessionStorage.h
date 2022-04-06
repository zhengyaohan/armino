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

#ifndef HAP_THREAD_SESSION_STORAGE_H
#define HAP_THREAD_SESSION_STORAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP+API.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)

/**
 * Session specific event notifications related bit sets.
 *
 * - Bit sets are indexed by the characteristic's event notification index.
 */
HAP_ENUM_BEGIN(uint8_t, HAPThreadSessionStorage_NotificationBitSet) { /** Notification subscription status. */
                                                                      kHAPSessionStorage_NotificationBitSet_Status,

                                                                      /** Notification pending flags. */
                                                                      kHAPSessionStorage_NotificationBitSet_Pending
} HAP_ENUM_END(uint8_t, HAPThreadSessionStorage_NotificationBitSet);

/** Number of session specific event notifications related bit sets. */
#define kHAPSessionStorage_NumNotificationBitSets ((size_t) 2)

/**
 * Session specific data buffers.
 */
HAP_ENUM_BEGIN(uint8_t, HAPThreadSessionStorage_DataBuffer) {
    /** Pending PDU body from HAP-Characteristic-Timed-Write-Request. */
    kHAPSessionStorage_DataBuffer_TimedWrite,

    /** Pending PDU body for HAP-Notification-Event. */
    kHAPSessionStorage_DataBuffer_Notification
} HAP_ENUM_END(uint8_t, HAPThreadSessionStorage_DataBuffer);

/** Number of session specific data buffers. */
#define kHAPSessionStorage_NumDataBuffers ((size_t) 2)

//----------------------------------------------------------------------------------------------------------------------

/** Session specific data (stored in HAPAccessoryServer). */
typedef struct {
    void* bytes;                  /**< Buffer. */
    size_t maxBytes;              /**< Capacity of buffer. */
    size_t numBytes;              /**< Length of buffer. */
    size_t peakNumBytes;          /**< Maximum length used so far. */
    size_t peakPotentialNumBytes; /**< Maximum length potentially being used so far. */

    /** Event notification state. */
    struct {
        /** Length of each event notification related bit set. */
        size_t numBytesPerBitSet;
    } notifications;

    /** Data buffer state. */
    struct {
        /** Position in buffer at which data buffer storage begins. */
        size_t startPosition;

        /** Maximum data length that has been used for each data buffer on any session. */
        size_t peakNumBytes[kHAPSessionStorage_NumDataBuffers];
    } dataBuffer;

    bool isEditing : 1; /**< Whether session specific data is being modified. */
} HAPThreadSessionStorage;

/** Session specific data (stored in each HAPSession). */
typedef struct {
    /** Data buffer state. */
    struct {
        /** Current length of data for each data buffer. */
        size_t numBytes[kHAPSessionStorage_NumDataBuffers];
    } dataBuffer;
} HAPThreadSessionStorageState;

//----------------------------------------------------------------------------------------------------------------------

/**
 * Initializes storage for session specific data.
 *
 * @param      server               Accessory server.
 * @param      bytes                Session storage buffer.
 * @param      numBytes             Length of session storage buffer.
 */
void HAPThreadSessionStorageCreate(HAPAccessoryServer* server, void* bytes, size_t numBytes);

/**
 * Prepares storage for session specific data for a HomeKit attribute database and number of sessions.
 *
 * @param      server               Accessory server that is starting.
 */
void HAPThreadSessionStorageHandleAccessoryServerWillStart(HAPAccessoryServer* server);

//----------------------------------------------------------------------------------------------------------------------

/**
 * Indicates whether or not there is session specific data associated with a given session.
 *
 * @param      server               Accessory server.
 * @param      session              Session.
 *
 * @return true                     If no stored data is associated with the session (i.e., no bits set, 0 data length).
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPThreadSessionStorageIsEmpty(HAPAccessoryServer* server, HAPSession* session);

//----------------------------------------------------------------------------------------------------------------------

/**
 * Retrieves a session specific event notification related bit set.
 *
 * - The HAPBitSet APIs can be used to interact with the retrieved bit set.
 *
 * @param      server               Accessory server.
 * @param      session              Session.
 * @param      bitSetType           Type of event notification related bit set to retrieve.
 * @param[out] bitSet               Start of bit set.
 * @param[out] numBytes             Length of bit set.
 */
void HAPThreadSessionStorageGetNotificationBitSet(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPThreadSessionStorage_NotificationBitSet bitSetType,
        uint8_t* _Nonnull* _Nonnull bitSet,
        size_t* numBytes);

//----------------------------------------------------------------------------------------------------------------------

/**
 * Retrieves data from a session specific data buffer.
 *
 * @param      server               Accessory server.
 * @param      session              Session.
 * @param      dataBufferType       Type of data buffer.
 * @param[out] bytes                Data.
 * @param[out] numBytes             Length of data.
 */
void HAPThreadSessionStorageGetData(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPThreadSessionStorage_DataBuffer dataBufferType,
        void* _Nonnull* _Nonnull bytes,
        size_t* numBytes);

/**
 * Stores data to a session specific data buffer.
 *
 * @param      server               Accessory server.
 * @param      session              Session.
 * @param      dataBufferType       Type of data buffer.
 * @param      bytes                Data.
 * @param      numBytes             Length of data.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If data has already been set (Use HAPThreadSessionStorageClearData to clear).
 * @return kHAPError_OutOfResources If not enough memory to store data.
 */
HAP_RESULT_USE_CHECK
HAPError HAPThreadSessionStorageSetData(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPThreadSessionStorage_DataBuffer dataBufferType,
        const void* bytes,
        size_t numBytes);

/**
 * Callback to provide data for a session specific data buffer.
 *
 * - HAPThreadSessionStorageSetData, HAPThreadSessionStorageSetDynamicData and HAPThreadSessionStorageClearData APIs
 *   must not be called from this data source (directly or indirectly).
 *
 * @param      context              Context.
 * @param      server               Accessory server.
 * @param      session              Session.
 * @param      dataBufferType       Type of data buffer.
 * @param[out] bytes                Buffer to store result.
 * @param      maxBytes             Capacity of buffer.
 * @param[out] numBytes             Length of data in buffer.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the supplied buffer is not large enough.
 */
typedef HAPError (*HAPThreadSessionStorageDataSource)(
        void* _Nullable context,
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPThreadSessionStorage_DataBuffer dataBufferType,
        void* bytes,
        size_t maxBytes,
        size_t* numBytes);

/**
 * Stores dynamic data to a session specific data buffer.
 *
 * @param      server               Accessory server.
 * @param      session              Session.
 * @param      dataBufferType       Type of data buffer.
 * @param      dataSource           Callback to use to get dynamic data.
 * @param      context              Context that is passed to the data source callback.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If data has already been set (Use HAPThreadSessionStorageClearData to clear).
 * @return kHAPError_OutOfResources If not enough memory to store data.
 */
HAP_RESULT_USE_CHECK
HAPError HAPThreadSessionStorageSetDynamicData(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPThreadSessionStorage_DataBuffer dataBufferType,
        HAPThreadSessionStorageDataSource dataSource,
        void* _Nullable context);

/**
 * Clears data in a session specific data buffer.
 *
 * @param      server               Accessory server.
 * @param      session              Session.
 * @param      dataBufferType       Type of data buffer.
 */
void HAPThreadSessionStorageClearData(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPThreadSessionStorage_DataBuffer dataBufferType);

#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
