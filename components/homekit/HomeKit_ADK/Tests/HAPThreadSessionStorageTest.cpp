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
// Copyright (C) 2020-2021 Apple Inc. All Rights Reserved.

#include "HAPPlatform+Init.h"

#include "HAP+API.h"
#include "HAPBitSet.h"
#include "HAPThreadSessionStorage.h"

#include "Harness/HAPAccessoryServerInternalMock.h"
#include "Harness/HAPPlatformCoAPManagerMock.h"
#include "Harness/HAPPlatformServiceDiscoveryMock.h"
#include "Harness/TemplateDB.h"
#include "Harness/UnitTestFramework.h"

#if !HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
int main() {
    return 0;
}
#else

// There must be at least 4 sessions for this test to work properly
#define kAppConfig_NumThreadSessions (8)

#define HAP_SESSION(server, index) (&server->thread.storage->sessions[index].hapSession)

// Test buffers
static const uint8_t SPORTS_BUFF[] = { 0xBA, 0x5E, 0xBA, 0x11,           // Baseball
                                       0xF0, 0x07, 0xBA, 0x11,           // Football
                                       0xBA, 0x5C, 0xE7, 0xBA, 0x11,     // Basketball
                                       0x70, 0x55 };                     // Toss
static const uint8_t FOOD_BUFF[] = { 0xBA, 0xD,  0xCA, 0xFE,             // Bad Cafe
                                     0xDE, 0xAD, 0xBE, 0xEF,             // Dead Beef
                                     0x5A, 0x1A, 0xD,                    // Salad
                                     0xFE, 0xED, 0xFA, 0xCE,             // Feed Face
                                     0x8B, 0xAD, 0xF0, 0x0D };           // ate bad food
static const uint8_t WORK_BUFF[] = { 0x0F, 0xF1, 0xCE,                   // Office
                                     0xC0, 0xDE, 0xAC, 0xE,              // Code Ace
                                     0xDE, 0xCA, 0xF,  0xC0, 0xFF, 0xEE, // Decaf Coffee
                                     0xDA, 0x7A, 0xBA, 0x5E };           // Database

static HAPAccessoryServer* CreateAccessoryServer() {
    static HAPAccessoryServer accServer;
    static HAPThreadSession sessions[kAppConfig_NumThreadSessions];

    HAPRawBufferZero(&accServer, sizeof(accServer));
    HAPRawBufferZero(sessions, sizeof(sessions));

    for (size_t sessionIndex = 0; sessionIndex < kAppConfig_NumThreadSessions; sessionIndex++) {
        sessions[sessionIndex].hapSession.transportType = kHAPTransportType_Thread;
    }

    static HAPThreadAccessoryServerStorage storage = {
        .sessions = sessions,
        .numSessions = kAppConfig_NumThreadSessions,
    };

    accServer.thread.storage = &storage;
    return &accServer;
}

static void TestBitset(size_t numCharacteristics) {
    // SessionStorage creates a bitset of the minimum number of bytes needed to store X bits
    //
    // X = C * S * N
    //    C = number of characters supporting event notification
    //    S = number of sessions.
    //    N = number of bit set types.
    // if X is not an even multiple of 8, an additional byte will be consumed with unused bits.
    HAP_ACCESSORY_SERVER_INTERNAL_MOCK(serverMock);

    EXPECT(serverMock, HAPAccessoryServerGetNumCharacteristicsSupportingEventNotification).Return(numCharacteristics);

    HAPAccessoryServer* server = CreateAccessoryServer();

    uint8_t buffer[384];
    HAPRawBufferZero(buffer, sizeof buffer);
    // Create Thread Session Storage
    HAPThreadSessionStorageCreate(server, buffer, 384);
    TEST_ASSERT_EQUAL(server->sessionStorage.maxBytes, (size_t) 384);
    TEST_ASSERT_EQUAL(server->sessionStorage.bytes, buffer);

    HAPThreadSessionStorageHandleAccessoryServerWillStart(server);

    // Ensure all sessions are empty.
    for (size_t sessionIndex = 0; sessionIndex < kAppConfig_NumThreadSessions; sessionIndex++) {
        HAPSession* session = HAP_SESSION(server, sessionIndex);
        TEST_ASSERT(HAPThreadSessionStorageIsEmpty(server, session));
    }

    size_t bitIndex = 0;
    // Poke some values into our bitsets.
    for (size_t sessionIndex = 0; sessionIndex < kAppConfig_NumThreadSessions; sessionIndex++) {
        HAPSession* session = HAP_SESSION(server, sessionIndex);
        for (size_t bitsetType = 0; bitsetType < kHAPSessionStorage_NumNotificationBitSets; bitsetType++) {
            uint8_t* bitSet;
            size_t numBytes;
            HAPThreadSessionStorageGetNotificationBitSet(
                    server, session, (HAPThreadSessionStorage_NotificationBitSet) bitsetType, &bitSet, &numBytes);

            // Ensure that the bitset is large enough to provide one bit per characteristic at minimum
            TEST_ASSERT(numBytes >= ((numCharacteristics + CHAR_BIT - 1) / CHAR_BIT));

            // Set some bits.
            HAPBitSetInsert(bitSet, numBytes, bitIndex % numCharacteristics);
            HAPBitSetInsert(bitSet, numBytes, (bitIndex + 3) % numCharacteristics);
            HAPBitSetInsert(bitSet, numBytes, (bitIndex + 7) % numCharacteristics);

            // This session is no longer empty
            TEST_ASSERT(!HAPThreadSessionStorageIsEmpty(server, session));

            bitIndex++;
        }
    }

    // Ensure that no data has been set outside of the bitset buffers:
    for (size_t index = server->sessionStorage.dataBuffer.startPosition; index < 384; index++) {
        TEST_ASSERT_EQUAL(buffer[index], 0);
    }

    // Set some garbage data to see if that stomps on any of our bitset data.
    buffer[server->sessionStorage.dataBuffer.startPosition] = 0xFF;

    // Ensure that the bitsets are retrievable and have the expected data
    bitIndex = 0;
    for (size_t sessionIndex = 0; sessionIndex < kAppConfig_NumThreadSessions; sessionIndex++) {
        HAPSession* session = HAP_SESSION(server, sessionIndex);
        for (size_t bitsetType = 0; bitsetType < kHAPSessionStorage_NumNotificationBitSets; bitsetType++) {
            uint8_t* bitSet;
            size_t numBytes;
            HAPThreadSessionStorageGetNotificationBitSet(
                    server, session, (HAPThreadSessionStorage_NotificationBitSet) bitsetType, &bitSet, &numBytes);

            // Ensure this session not empty
            TEST_ASSERT(!HAPThreadSessionStorageIsEmpty(server, session));

            // Check The Set Bits.
            TEST_ASSERT(HAPBitSetContains(bitSet, numBytes, bitIndex % numCharacteristics));
            TEST_ASSERT(HAPBitSetContains(bitSet, numBytes, (bitIndex + 3) % numCharacteristics));
            TEST_ASSERT(HAPBitSetContains(bitSet, numBytes, (bitIndex + 7) % numCharacteristics));

            // And the clear bits
            TEST_ASSERT(!HAPBitSetContains(bitSet, numBytes, (bitIndex + 1) % numCharacteristics));
            TEST_ASSERT(!HAPBitSetContains(bitSet, numBytes, (bitIndex + 2) % numCharacteristics));
            TEST_ASSERT(!HAPBitSetContains(bitSet, numBytes, (bitIndex + 4) % numCharacteristics));
            TEST_ASSERT(!HAPBitSetContains(bitSet, numBytes, (bitIndex + 5) % numCharacteristics));
            TEST_ASSERT(!HAPBitSetContains(bitSet, numBytes, (bitIndex + 6) % numCharacteristics));

            HAPBitSetRemove(bitSet, numBytes, bitIndex % numCharacteristics);
            HAPBitSetRemove(bitSet, numBytes, (bitIndex + 3) % numCharacteristics);
            HAPBitSetRemove(bitSet, numBytes, (bitIndex + 7) % numCharacteristics);
            bitIndex++;
        }
        // This session is now empty
        TEST_ASSERT(HAPThreadSessionStorageIsEmpty(server, session));
    }
    VERIFY_ALL(serverMock);
}

/**
 * Test checks bitsets stored in session storage.  Ensures that
 * bitsets are set and retrieved as expected for different
 * bitset sizes.
 *
 * Even sizes used here to ensure that bitsets are safe from
 * corruption "at the edges", as sizes that are multiples of 8
 * will use every bit in the byte.
 */
TEST(BitSet_CharacteristicsMult8) {
    // Test ensures that every bit in the byte given to the bitset is valid.  Useful to determine
    // whether the 'edges' are vulnerable.
    TestBitset(8);
    TestBitset(16);
    TestBitset(32);
}

/**
 * Test checks bitsets stored in session storage.  Ensures that
 * bitsets are set and retrieved as expected for different
 * bitset sizes. Odd bitset sizes are used here to ensure that
 * enough bytes are used to store the bitset
 */
TEST(BitSet_CharacteristicsOdd) {
    // Test checks to ensure that odd sized bitsets (not a nice multiple of 8) do not interfere with eachother
    TestBitset(7);
    TestBitset(13);
    TestBitset(22);
}

typedef struct {
    const void* bytes;
    size_t numBytes;
} TestDataSourceContext;

static HAPError SourceFunc(
        void* _Nullable context_,
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPThreadSessionStorage_DataBuffer dataBufferType,
        void* bytes,
        size_t maxBytes,
        size_t* numBytes) {
    HAPPrecondition(context_);
    TestDataSourceContext* context = (TestDataSourceContext*) context_;

    if (context->numBytes > maxBytes) {
        return kHAPError_OutOfResources;
    }

    HAPRawBufferCopyBytes(bytes, context->bytes, context->numBytes);
    *numBytes = context->numBytes;
    return kHAPError_None;
}

void SetSessionDataStatic(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPThreadSessionStorage_DataBuffer storage,
        const void* buffer,
        size_t buffSize) {
    TEST_ASSERT(HAPThreadSessionStorageSetData(server, session, storage, buffer, buffSize) == kHAPError_None);
    TEST_ASSERT(!HAPThreadSessionStorageIsEmpty(server, session));
}

void SetSessionDataDynamic(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPThreadSessionStorage_DataBuffer storage,
        const void* buffer,
        size_t buffSize) {
    TestDataSourceContext context;
    context.bytes = buffer;
    context.numBytes = buffSize;
    TEST_ASSERT(
            HAPThreadSessionStorageSetDynamicData(server, session, storage, SourceFunc, &context) == kHAPError_None);
    TEST_ASSERT(!HAPThreadSessionStorageIsEmpty(server, session));
}

void CheckDataBuffer(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPThreadSessionStorage_DataBuffer storage,
        const void* testBuff,
        size_t testBufSize) {
    void* bytes;
    size_t numBytes;
    HAPThreadSessionStorageGetData(server, session, storage, &bytes, &numBytes);
    TEST_ASSERT(testBufSize == numBytes);
    TEST_ASSERT(HAPRawBufferAreEqual(bytes, testBuff, numBytes));
}

static void TestDataStorage(bool isStaticData) {
    HAP_ACCESSORY_SERVER_INTERNAL_MOCK(serverMock);

    EXPECT(serverMock, HAPAccessoryServerGetNumCharacteristicsSupportingEventNotification).Return(8);

    HAPAccessoryServer* server = CreateAccessoryServer();

    uint8_t buffer[384];
    HAPRawBufferZero(buffer, sizeof buffer);
    // Create Thread Session Storage
    HAPThreadSessionStorageCreate(server, buffer, 384);
    TEST_ASSERT_EQUAL(server->sessionStorage.maxBytes, (size_t) 384);
    TEST_ASSERT_EQUAL(server->sessionStorage.bytes, buffer);

    HAPThreadSessionStorageHandleAccessoryServerWillStart(server);

    // Ensure all sessions are empty.
    for (size_t sessionIndex = 0; sessionIndex < kAppConfig_NumThreadSessions; sessionIndex++) {
        TEST_ASSERT(HAPThreadSessionStorageIsEmpty(server, HAP_SESSION(server, sessionIndex)));
    }

    if (isStaticData) {
        // Set static data.  Set it in reverse session order (leaving 0 blank)
        // to ensure inserts and removals work properly.
        for (size_t i = 0; i < kHAPSessionStorage_NumDataBuffers; i++) {
            SetSessionDataStatic(
                    server,
                    HAP_SESSION(server, 3),
                    (HAPThreadSessionStorage_DataBuffer) i,
                    SPORTS_BUFF,
                    sizeof(SPORTS_BUFF));
            SetSessionDataStatic(
                    server,
                    HAP_SESSION(server, 2),
                    (HAPThreadSessionStorage_DataBuffer) i,
                    FOOD_BUFF,
                    sizeof(FOOD_BUFF));
            SetSessionDataStatic(
                    server,
                    HAP_SESSION(server, 1),
                    (HAPThreadSessionStorage_DataBuffer) i,
                    WORK_BUFF,
                    sizeof(WORK_BUFF));
        }
    } else {
        // Set dynamic data.  Dynamic data is data set by functions

        for (size_t i = 0; i < kHAPSessionStorage_NumDataBuffers; i++) {

            SetSessionDataDynamic(
                    server,
                    HAP_SESSION(server, 3),
                    (HAPThreadSessionStorage_DataBuffer) i,
                    SPORTS_BUFF,
                    sizeof(SPORTS_BUFF));
            SetSessionDataDynamic(
                    server,
                    HAP_SESSION(server, 2),
                    (HAPThreadSessionStorage_DataBuffer) i,
                    FOOD_BUFF,
                    sizeof(FOOD_BUFF));
            SetSessionDataDynamic(
                    server,
                    HAP_SESSION(server, 1),
                    (HAPThreadSessionStorage_DataBuffer) i,
                    WORK_BUFF,
                    sizeof(WORK_BUFF));
        }
    }

    // Check data
    void* bytes;
    size_t numBytes;
    for (size_t i = 0; i < kHAPSessionStorage_NumDataBuffers; i++) {
        // Make sure the data is valid.
        CheckDataBuffer(
                server, HAP_SESSION(server, 1), (HAPThreadSessionStorage_DataBuffer) i, WORK_BUFF, sizeof(WORK_BUFF));
        CheckDataBuffer(
                server, HAP_SESSION(server, 2), (HAPThreadSessionStorage_DataBuffer) i, FOOD_BUFF, sizeof(FOOD_BUFF));
        CheckDataBuffer(
                server,
                HAP_SESSION(server, 3),
                (HAPThreadSessionStorage_DataBuffer) i,
                SPORTS_BUFF,
                sizeof(SPORTS_BUFF));

        // Get storage from a node that didn't have any
        HAPThreadSessionStorageGetData(
                server, HAP_SESSION(server, 0), (HAPThreadSessionStorage_DataBuffer) i, &bytes, &numBytes);
        TEST_ASSERT_EQUAL(numBytes, (size_t) 0);

        // Clear the data out
        HAPThreadSessionStorageClearData(server, HAP_SESSION(server, 1), (HAPThreadSessionStorage_DataBuffer) i);
        HAPThreadSessionStorageClearData(server, HAP_SESSION(server, 2), (HAPThreadSessionStorage_DataBuffer) i);
        HAPThreadSessionStorageClearData(server, HAP_SESSION(server, 3), (HAPThreadSessionStorage_DataBuffer) i);
    }

    TEST_ASSERT(HAPThreadSessionStorageIsEmpty(server, HAP_SESSION(server, 1)));
    TEST_ASSERT(HAPThreadSessionStorageIsEmpty(server, HAP_SESSION(server, 2)));
    TEST_ASSERT(HAPThreadSessionStorageIsEmpty(server, HAP_SESSION(server, 3)));

    VERIFY_ALL(serverMock);
}

/**
 * Test checks session storage using static buffers.  Ensures
 * that data can be set and retrieved for sessions as sessions
 * are added and removed.
 */
TEST(Static_Storage) {
    TestDataStorage(true);
}

/**
 * Test checks session storage using dynamic buffers.  Ensures
 * that data can be set and retrieved for sessions as sessions
 * are added and removed.
 */
TEST(Dynamic_Storage) {
    TestDataStorage(false);
}

/**
 * Test checks to see if Session Storage protects itself
 * appropriately.  Ensures that a session cannot store more than
 * one chunk of data per storage type and that requests to
 * insert too much data are safely handled
 */
TEST(Invalid_Cases) {
    HAP_ACCESSORY_SERVER_INTERNAL_MOCK(serverMock);

    EXPECT(serverMock, HAPAccessoryServerGetNumCharacteristicsSupportingEventNotification).Return(8);

    HAPAccessoryServer* server = CreateAccessoryServer();

    uint8_t buffer[32];
    HAPRawBufferZero(buffer, sizeof buffer);
    // Create Thread Session Storage
    HAPThreadSessionStorageCreate(server, buffer, 32);
    TEST_ASSERT_EQUAL(server->sessionStorage.maxBytes, (size_t) 32);
    TEST_ASSERT_EQUAL(server->sessionStorage.bytes, buffer);
    HAPThreadSessionStorageHandleAccessoryServerWillStart(server);

    // Each Session may only have one piece of data per buffer Add one
    HAPError err;
    SetSessionDataStatic(
            server,
            HAP_SESSION(server, 1),
            kHAPSessionStorage_DataBuffer_Notification,
            SPORTS_BUFF,
            sizeof(SPORTS_BUFF));

    TestDataSourceContext dataSourceContext;
    dataSourceContext.bytes = WORK_BUFF;
    dataSourceContext.numBytes = sizeof(WORK_BUFF);

    // Now try to add another.  Should fail.
    err = HAPThreadSessionStorageSetDynamicData(
            server, HAP_SESSION(server, 1), kHAPSessionStorage_DataBuffer_Notification, SourceFunc, &dataSourceContext);
    TEST_ASSERT_EQUAL(err, kHAPError_InvalidState);

    // Try to add more for session 2... but we will run out of space.
    dataSourceContext.bytes = FOOD_BUFF;
    dataSourceContext.numBytes = sizeof(FOOD_BUFF);
    err = HAPThreadSessionStorageSetDynamicData(
            server, HAP_SESSION(server, 2), kHAPSessionStorage_DataBuffer_Notification, SourceFunc, &dataSourceContext);
    TEST_ASSERT_EQUAL(err, kHAPError_OutOfResources);

    // Ensure Static runs out of space as well.
    err = HAPThreadSessionStorageSetData(
            server, HAP_SESSION(server, 2), kHAPSessionStorage_DataBuffer_Notification, FOOD_BUFF, sizeof(FOOD_BUFF));
    TEST_ASSERT_EQUAL(err, kHAPError_OutOfResources);

    // Make sure that, after all that, session 1's data is still intact.
    CheckDataBuffer(
            server,
            HAP_SESSION(server, 1),
            kHAPSessionStorage_DataBuffer_Notification,
            SPORTS_BUFF,
            sizeof(SPORTS_BUFF));

    // Clear Session 1
    HAPThreadSessionStorageClearData(server, HAP_SESSION(server, 1), kHAPSessionStorage_DataBuffer_Notification);
    TEST_ASSERT(HAPThreadSessionStorageIsEmpty(server, HAP_SESSION(server, 1)));

    VERIFY_ALL(serverMock);
}

int main(int argc, char** argv) {
    return EXECUTE_TESTS(argc, (const char**) argv);
}

#endif
